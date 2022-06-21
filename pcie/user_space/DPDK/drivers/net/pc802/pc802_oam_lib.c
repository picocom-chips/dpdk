/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018-2020 Picocom Corporation
 */
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#include "rte_pmd_pc802.h"
#include "pcxx_ipc.h"
#include "pc802_oam_lib.h"

#define _UNUSED_ __attribute__((__unused__))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OamMsgType_Log = 128,       /* lower 128 reserved for driver for special handing */
    OamMsgType_Trace,
    OamMsgType_Memdump,
    OamMsgType_Num
} OamMsgType_e;

typedef enum{
    PICO_OAM_MSG=0x00,
    PICO_P19_MSG=0x01,
    PICO_DEBUG_MSG=0x02,
}OamMessageType_e;

#define OAM_START_FLAG      0xd1c2b3a4
#define OAM_END_FLAG        0xa4b3c2d1

typedef struct{
    uint32_t StartFlag;         //0xd1c2b3a4
    uint32_t MsgType;
    uint32_t SubMsgNum;		    //Message burst
}OamMessageHeader_t;

typedef struct{
    OamMessageHeader_t Head;
    OamSubMessage_t SubMsg[0];
    uint32_t EndFlag;           //0xa4b3c2d1
}OamMessage_t;

typedef struct tMemdumpReq{
    uint32_t startAddr;
    uint32_t totalLength;
}MemdumpReq_t;

typedef struct{
    PC802_OAM_CALLBACK_FUNTION fun;
    void *arg;
}OamCallbackInfo;

OamCallbackInfo gOamCbInfo = {0};
OamCallbackInfo gSubCbInfo[128];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void* oam_recv( _UNUSED_ void *arg  );
static uint32_t pc802_oam_recv( const char* buf, uint32_t payloadSize );

int pc802_oam_init(void)
{
    pthread_t tid;
    pcxxInfo_s cb_info = { pc802_oam_recv, NULL };

    pcxxOamOpen(&cb_info);

    pthread_create( &tid, NULL, oam_recv, NULL);

    return 0;
}

int pc802_oam_send_msg( _UNUSED_ uint16_t port_id, const OamSubMessage_t **sub_msg, uint32_t msg_num )
{
    OamMessage_t *msg = NULL;
    OamSubMessage_t *sub = NULL;
    uint32_t *end = NULL;
    uint32_t len = 0;
    uint32_t i;

    pthread_mutex_lock(&lock);

    pcxxSendStart( );
    pcxxOamAlloc( (char **)&msg, &len);
    if ( NULL==msg ){
        pthread_mutex_unlock(&lock);
        return -1;
    }
    msg->Head.StartFlag = OAM_START_FLAG;
    msg->Head.MsgType = PICO_OAM_MSG;
    msg->Head.SubMsgNum = 0;

    for ( i=0; i<msg_num; i++ ){
        sub = NULL;
        pcxxOamAlloc( (char **)&sub, &len );
        if ( NULL==sub || len<sub_msg[i]->Head.MsgSize ){
            printf( "Not enough space(%u) for message(%d:%d)\n", len, sub_msg[i]->Head.MsgId, sub_msg[i]->Head.MsgSize );
            pthread_mutex_unlock(&lock);
            return -1;
        }
        memcpy( (void*)sub, (const void *)(sub_msg[i]), sub_msg[i]->Head.MsgSize );
        pcxxOamSend( (void*)sub, sub_msg[i]->Head.MsgSize );
        msg->Head.SubMsgNum++;
    }

    pcxxOamAlloc( (char **)&end, &len );
    *end = OAM_END_FLAG;
    pcxxOamSend( (void*)end, sizeof(*end) );

    pcxxSendEnd( );

    pthread_mutex_unlock(&lock);

    return 0;
}

int pc802_oam_register( PC802_OAM_CALLBACK_FUNTION recv_fun, void *arg )
{
    gOamCbInfo.fun = recv_fun;
    gOamCbInfo.arg = arg;
    return 0;
}

int pc802_oam_unregister(void)
{
    gOamCbInfo.fun = NULL;
    gOamCbInfo.arg = NULL;
    return 0;
}

int pc802_oam_sub_msg_register( uint16_t sub_msg_id, PC802_OAM_CALLBACK_FUNTION recv_fun, void *arg )
{
    uint16_t index = sub_msg_id-BASIC_CFG_GET_REQ;
    if ( index<ARRAY_SIZE(gSubCbInfo) ) {
        gSubCbInfo[index].fun = recv_fun;
        gSubCbInfo[index].arg = arg;
        return 0;
    }
    return -1;
}

int pc802_oam_sub_msg_unregister( uint16_t sub_msg_id )
{
    uint16_t index = sub_msg_id-BASIC_CFG_GET_REQ;
     if ( index<ARRAY_SIZE(gSubCbInfo) ) {
        gSubCbInfo[index].fun = NULL;
        gSubCbInfo[index].arg = NULL;
    }
    return 0;
}

static uint32_t pc802_process_oam_msg( uint16_t port_id, const OamMessage_t *msg, uint32_t len )
{
    const OamSubMessage_t *cur = NULL;
    const OamSubMessage_t *sub[32] = {NULL};
    uint32_t sub_num = 0;
    uint32_t i = 0;
    uint16_t sub_index = 0;
    PC802_OAM_CALLBACK_FUNTION cb_fun =NULL;

    if ( OAM_START_FLAG != msg->Head.StartFlag || len<sizeof(OamMessage_t) )
        return -1;
    len -= sizeof(OamMessageHeader_t);
    cur = msg->SubMsg;
    for ( i=0; (i<msg->Head.SubMsgNum)&&(len>sizeof(OamSubMessageHeader_t)); i++ ) {
        if ( len<cur->Head.MsgSize )
            break;
        sub_index = cur->Head.MsgId-BASIC_CFG_GET_REQ;
        cb_fun = sub_index<ARRAY_SIZE(gSubCbInfo)?gSubCbInfo[sub_index].fun:NULL;
        if ( NULL != cb_fun )
            cb_fun( gSubCbInfo[sub_index].arg, port_id, &cur, 1 );
        else
            sub[sub_num++] = cur;
        len -= cur->Head.MsgSize;
        cur = (const OamSubMessage_t *)((const char *)cur+cur->Head.MsgSize);
    }

    cb_fun = gOamCbInfo.fun;
    if ( sub_num>0 && NULL!=cb_fun )
        cb_fun( gOamCbInfo.arg, port_id, sub, sub_num );

    if ( OAM_END_FLAG != *((const uint32_t *)cur) )
        printf( "Oam %d msg end flag(%u) err!\n", port_id, *((const uint32_t *)cur) );

    return 0;
}

static uint32_t pc802_oam_recv( const char* buf, uint32_t payloadSize )
{
    assert(buf);
    assert(payloadSize > 0);
    const PC802_Mem_Block_t* mbuf = (const PC802_Mem_Block_t*)(buf - sizeof(PC802_Mem_Block_t));
    const OamMessage_t *msg = (const OamMessage_t*)buf;

    switch (mbuf->pkt_type)
    {
        case OamMsgType_Log:
        case OamMsgType_Trace:
            break;
        case OamMsgType_Memdump:
            break;
        default:
            pc802_process_oam_msg( 0, msg, payloadSize);
            break;
    }

    return payloadSize;
}

static void* oam_recv( _UNUSED_ void *arg  )
{
    while (1)
        pcxxOamRecv();
    return NULL;
}

