/*
 * @File  gps_parse.c
 * @Brief 
 * 
 */

#include "gps_parse.h"
#include "string.h"
#include "stdlib.h"
#include "minmea.h"
#include "stdint.h"
#include "stdbool.h"


#define INDENT_SPACES "  "


GPS_Info_t g_gps_info;

/**
 * 
 * @param  nmea: one nmea message. e.g.
 *                  $GNRMC,084257.000,A,2234.7758,N,11354.9654,E,0.032,306.43,140618,,,D*46
 * @param flag: alway the same if this one message from the same message frame
 */
bool ParseOneNmea(uint8_t* nmea, uint8_t flag)
{
    static uint8_t flag_old = 0xff;
    static uint8_t gsv_count = 0;
    static uint8_t gsa_count = 0;
    char* line = nmea;

    //first line of multiple line NMEA message
    if(flag != flag_old){
        flag_old = flag;
        gsv_count = 0;
        gsa_count = 0;
    }

    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            if (minmea_parse_rmc(&g_gps_info.rmc, line)) {

		       printf(INDENT_SPACES "$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
                           g_gps_info.rmc.latitude.value, g_gps_info.rmc.latitude.scale,
                            g_gps_info.rmc.longitude.value, g_gps_info.rmc.longitude.scale,
                            g_gps_info.rmc.speed.value, g_gps_info.rmc.speed.scale);
            }
            else {
                printf("$xxRMC sentence is not parsed\n");
            }
        } break;

        case MINMEA_SENTENCE_GGA: {
            if (minmea_parse_gga(&g_gps_info.gga, line)) {
		     printf("GGA \n");
            }
            else {
                printf("$xxGGA sentence is not parsed\n");
            }
        } break;

        case MINMEA_SENTENCE_GST: {
            if (minmea_parse_gst(&g_gps_info.gst, line)) {
		     printf("GST \n");
            }
            else {
                printf("$xxGST sentence is not parsed\n");
            }
        } break;

        case MINMEA_SENTENCE_GSV: {
            if(gsv_count < GPS_PARSE_MAX_GSV_NUMBER){
                if (minmea_parse_gsv(&g_gps_info.gsv[gsv_count++], line)) {
		     printf("GSV \n");
                }
                else {
                    printf("$xxGSV sentence is not parsed\n");
                }
            }
        } break;

        case MINMEA_SENTENCE_VTG: {
            if (minmea_parse_vtg(&g_gps_info.vtg, line)) {
		     printf("VTG \n");
            }
            else {
                printf("$xxVTG sentence is not parsed\n");
            }
        } break;

        case MINMEA_SENTENCE_ZDA: {
            if (minmea_parse_zda(&g_gps_info.zda, line)) {
		     printf("ZDA \n");
            }
            else {
                printf("$xxZDA sentence is not parsed\n");
            }
        } break;
        case MINMEA_SENTENCE_GSA:{
            if(gsa_count < GPS_PARSE_MAX_GSA_NUMBER){
                if (minmea_parse_gsa(&g_gps_info.gsa[gsa_count++], line)) {
		     printf("GSA \n");
                }
                else {
                    printf("$xxGSA sentence is not parsed\n");
                }
            }
        } break;
        case MINMEA_SENTENCE_GLL:{
            if (minmea_parse_gll(&g_gps_info.gll, line)) {
		     printf("GLL\n");
            }
            else {
                printf("$xxGLL sentence is not parsed\n");
            }
        } break;
        case MINMEA_INVALID:
        default:
            printf("$xxxxx sentence is not parsed:%s\n",line);
            return false;
    }
    return true;
}


/**
 * Parse a full frame gps NMEA message.
 * @param nmeas: A full GPA NMEA message frame. e.g.
 *                $GNGGA,084257.000,2234.7758,N,11354.9654,E,2,12,1.00,59.4,M,-2.8,M,,*56
 *                $GPGSA,A,3,19,28,09,03,23,193,,,,,,,1.28,1.00,0.80*32
 *                $BDGSA,A,3,04,01,07,03,06,09,,,,,,,1.28,1.00,0.80*1F
 *                $GPGSV,4,1,14,193,60,100,40,17,54,020,14,28,53,165,42,06,52,308,*43
 *                $GPGSV,4,2,14,19,46,346,13,42,46,122,33,02,23,268,,03,21,041,18*75
 *                $GPGSV,4,3,14,09,17,125,32,23,13,088,35,30,04,180,34,05,02,211,23*7B
 *                $GPGSV,4,4,14,24,01,292,,12,01,325,*74
 *                $BDGSV,3,1,12,03,65,189,37,10,55,226,,01,51,128,35,08,49,000,*67
 *                $BDGSV,3,2,12,13,49,322,,02,48,238,,17,44,136,,07,40,185,40*68
 *                $BDGSV,3,3,12,04,33,110,33,06,27,160,36,05,24,256,,09,12,183,34*6B
 *                $GNRMC,084257.000,A,2234.7758,N,11354.9654,E,0.032,306.43,140618,,,D*46
 *                $GNVTG,306.43,T,,M,0.032,N,0.059,K,D*29
 * @return bool: Parse success of not
 */
bool GPS_Parse(uint8_t* nmeas)
{
    bool retFlag = false;
    static uint8_t flag  = 0;
    uint8_t tmpStore;

    while(nmeas)
    {
        uint8_t* index1 = strstr(nmeas,"$");
        if(index1)
        {
            uint8_t* index2 = strstr(index1,"\r\n");
            if(index2)
            {
                retFlag = true;
                tmpStore = index2[2];
                index2[2] = '\0';
                nmeas += index2-index1+2;
                retFlag = ParseOneNmea(index1,flag);
                index2[2] = tmpStore;
            }
            else //$GNGGA,084257.000
                break;
        }
        else
            break;
    }
    ++ flag;
    return retFlag;
}

/**
 * Get address of global gps infomatioin variable 
 * @return GPS_Info_t*: Address of global gps infomatioin variable 
 */
GPS_Info_t* Gps_GetInfo()
{
    return &g_gps_info;
}

