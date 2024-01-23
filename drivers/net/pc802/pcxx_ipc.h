#ifndef __PC802_ATL_H__
#define __PC802_ATL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_INDEX_MAX       PC802_INDEX_MAX     /**< Max number of baseband devices */
#ifdef MULTI_PC802
#define CELL_NUM_PRE_DEV    2                   /**< Number of cells per baseband device */
#define LEGACY_CELL_INDEX   CELL_NUM_PRE_DEV    /**< The LEGACY cell index of the baseband device, only supports one */
#else
#define CELL_NUM_PRE_DEV    1                   /**< Number of cells per baseband device */
#endif

#define PCXX_MAX_TX_DATAS    16
#define PCXX_MAX_TX_TTIS     32

/**< Get the total number of baseband devices that have been successfully initialised */
#define pcxxGetDevCount()   pc802_get_count()

#ifndef MULTI_PC802
typedef uint32_t (*PCXX_RW_CALLBACK)(const char* buf, uint32_t payloadSize);
#else
typedef uint32_t (*PCXX_RW_CALLBACK)(const char* buf, uint32_t payloadSize, uint16_t dev_index, uint16_t cell_index );
#endif
typedef struct {
    PCXX_RW_CALLBACK readHandle;
    PCXX_RW_CALLBACK writeHandle;
} pcxxInfo_s;

/**
* @brief Create control  queues for Tx and Rx
*
* @param[in] info Register Tx and Rx callback functions
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxCtrlOpen(const pcxxInfo_s* info, ...);
#else
int pcxxCtrlOpen(const pcxxInfo_s* info, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Close and free the control shared memory.
*
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return	none
*/
#ifndef MULTI_PC802
void pcxxCtrlClose(void);
#else
void pcxxCtrlClose(uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Create data queues  for Rx and Tx
*
* @param[in] info Register Tx and Rx callback functions
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxDataOpen(const pcxxInfo_s* info, ...);
#else
int pcxxDataOpen(const pcxxInfo_s* info, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Close and free the data queue.
*
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return  none
*/
#ifndef MULTI_PC802
void pcxxDataClose(void);
#else
void pcxxDataClose(uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Called before sending a messages list, mainly is used to select the block .
*
* @param[in] dev_index baseband device index
* @return returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxSendStart(void);
#else
int pcxxSendStart(uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Notify Rx side that new messages are arrived.
*
* @param[in] dev_index baseband device index
* @return  returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxSendEnd(void);
#else
int pcxxSendEnd(uint16_t dev_index, uint16_t cell_index );;
#endif

/**
* @brief Allocate one control message memory from the current block in use.
*
* @param[out] buf the allocated memory address
* @param[out] availableSize the current available size in this block
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxCtrlAlloc(char** buf, uint32_t* availableSize, ...);
#else
int pcxxCtrlAlloc(char** buf, uint32_t* availableSize, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Update block header when the content of one control message is completed.
*
* @param[in] buf   write memory data
* @param[in] bufLen length of data written
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if opened successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxCtrlSend(const char* buf, uint32_t bufLen, ...);
#else
int pcxxCtrlSend(const char* buf, uint32_t bufLen, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Check the number of received control messages. Application thread may poll this function till it detects messages from Tx side.
*
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return	returns 0 if the received messaged are handled successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxCtrlRecv(void);
#else
int pcxxCtrlRecv(uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Allocate one data message memory from the current block in use.
*
* @param[in] bufSize the alloc memory size
* @param[out] buf the available memory address
* @param[out] offset the data memory offset from the first address
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if allocated successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxDataAlloc(uint32_t bufSize, char** buf, uint32_t* offset, ...);
#else
int pcxxDataAlloc(uint32_t bufSize, char** buf, uint32_t* offset, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Update data queue context when the content of one data message is completed.
*
* @param[in] offset the offset value from the first address of data queue
* @param[in] bufLen the length of the written data
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns 0 if sent successfully, otherwise returns error
*/
#ifndef MULTI_PC802
int pcxxDataSend(uint32_t offset, uint32_t bufLen, ...);
#else
int pcxxDataSend(uint32_t offset, uint32_t bufLen, uint16_t dev_index, uint16_t cell_index );
#endif

#ifndef MULTI_PC802
int pcxxDataReSend(char *buf, uint32_t bufLen, uint32_t *offset, ...);
#else
int pcxxDataReSend(char *buf, uint32_t bufLen, uint32_t *offset, uint16_t dev_index, uint16_t cell_index );
#endif

/**
* @brief Receive data  from queue by offset
*
* @param[in] offset offset of the read data queue, data memory offset from the first address
* @param[in] len the length of the read data queue
* @param[in] dev_index baseband device index
* @param[in] cell_index baseband device cell index
* @return returns pointer to the data if received successfully, otherwise returns NULL
*/
#ifndef MULTI_PC802
void* pcxxDataRecv(uint32_t offset, uint32_t len, ...);
#else
void* pcxxDataRecv(uint32_t offset, uint32_t len, uint16_t dev_index, uint16_t cell_index );
#endif

#ifndef MULTI_PC802
int pcxxCtrlDestroy(void);
int pcxxDataDestroy(void);
#else
int pcxxCtrlDestroy(uint16_t dev_index, uint16_t cell_index );
int pcxxDataDestroy(uint16_t dev_index, uint16_t cell_index );
#endif

#ifdef __cplusplus
}
#endif

#endif

