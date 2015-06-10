/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *         2015/1/23, v1.0 create this file.
 *         2015/6/6     Added SubPos Node Code
 *         Based on https://github.com/espressif/esp8266_at/blob/master/at/user/at_wifiCmd.c
*******************************************************************************/

#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"
#include "c_types.h"
#include "mem.h"

extern at_funcationType at_custom_cmd[];

uint8_t at_wifiMode;

int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen)
{

    char *pTempD = pDest;
    const char *pTempS = pSrc;
    int8_t len;

    if(*pTempS != '\"')
    {
        return -1;
    }
    pTempS++;
    for(len=0; len<maxLen; len++)
    {
        if(*pTempS == '\"')
        {
            *pTempD = '\0';
            break;
        }
        else
        {
            *pTempD++ = *pTempS++;
        }
    }
    if(len == maxLen)
    {
        return -1;
    }
    return len;
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapID(uint8_t id, char *pPara)
{
    int8_t len,passLen;
    struct softap_config apConfig;

    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);
    
    if(at_wifiMode == STATION_MODE)
    {
        at_response_error();
        return;
    }
    //These values are already set if in AP mode.
    //apConfig.password = "";
    //apConfig.authmode = 0;
    pPara++;
    len = at_dataStrCpy(apConfig.ssid, pPara, 32);
    apConfig.ssid_len = len;
    
    if(len < 1)
    {
        at_response_error();
        return;
    }
    pPara += (len+3);

    apConfig.channel = atoi(pPara);
    if(apConfig.channel<1 || apConfig.channel>13)
    {
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    wifi_softap_set_config_current(&apConfig);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapCH(uint8_t id, char *pPara)
{
    int8_t len;
    struct softap_config apConfig;

    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);

    if(at_wifiMode == STATION_MODE)
    {
        at_response_error();
        return;
    }
    pPara++;
    apConfig.channel = atoi(pPara);
    if(apConfig.channel<1 || apConfig.channel>13)
    {
        at_response_error();
        return;
    }
    ETS_UART_INTR_DISABLE();
    wifi_softap_set_config_current(&apConfig);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdCwsapRI(uint8_t id)
{
    struct softap_config apConfig;
    
    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);
    
    if(at_wifiMode == STATION_MODE)
    {
        at_response_error();
        return;
    }

    ETS_UART_INTR_DISABLE();
    wifi_softap_set_config_current(&apConfig);
    ETS_UART_INTR_ENABLE();
    at_response_ok();
}

//These commands are the same as the regular AT commands, except they don't write the parameters to flash
//AT+CWSAPID:
//Set parameters of AP with existing password and encryption.
//AT+CWSAPID="<ssid>",<channel num>

//AT+CWSAPCH: 
//Change AP channel.
//AT+CWSAPCH=<channel num> 

//AT+CWSAPRI: 
//AT+CWSAPRI
//Re-Init AP.

extern void at_exeCmdCiupdate(uint8_t id);
at_funcationType at_custom_cmd[] = {
        {"+CWSAPID", 8, NULL, NULL, at_setupCmdCwsapID, NULL},
        {"+CWSAPRI", 8, NULL, NULL, NULL, at_setupCmdCwsapRI},
        {"+CWSAPCH", 8, NULL, NULL, at_setupCmdCwsapCH, NULL}
};

void user_rf_pre_init(void)
{
}

void user_init(void)
{
        char buf[64] = {0};
        at_customLinkMax = 5;
        at_init();
        os_sprintf(buf,"compile time:%s %s",__DATE__,__TIME__);
        at_set_custom_info(buf);
        at_port_print("\r\nready\r\n");
        at_cmd_array_regist(&at_custom_cmd[0], sizeof(at_custom_cmd)/sizeof(at_custom_cmd[0]));
}
