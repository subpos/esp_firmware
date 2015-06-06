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
at_testCmdSCwmode(uint8_t id)
{
    char temp[32];
    os_sprintf(temp, "%s:(1-3)\r\n", at_custom_cmd[id].at_cmdName);
    at_port_print(temp);
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_queryCmdSCwmode(uint8_t id)
{
    char temp[32];

    at_wifiMode = wifi_get_opmode();
    os_sprintf(temp, "%s:%d\r\n", at_custom_cmd[id].at_cmdName, at_wifiMode);
    at_port_print(temp);
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdSCwmode(uint8_t id, char *pPara)
{
    uint8_t mode;
    char temp[32];

    pPara++;
    mode = atoi(pPara);
    if(mode == at_wifiMode)
    {
        at_response_ok();
        return;
    }
    if((mode >= 1) && (mode <= 3))
    {
        ETS_UART_INTR_DISABLE();
        wifi_set_opmode_current(mode);
        ETS_UART_INTR_ENABLE();
        at_wifiMode = mode;
        at_response_ok();
    }
    else
    {
        at_response_error();
    }
}


void ICACHE_FLASH_ATTR
at_queryCmdSCwsap(uint8_t id)
{
    struct softap_config apConfig;
    char temp[128];

    if(at_wifiMode == STATION_MODE)
    {
        at_response_error();
        return;
    }
    wifi_softap_get_config(&apConfig);
    os_sprintf(temp,"%s:\"%s\",\"%s\",%d,%d\r\n",
                         at_custom_cmd[id].at_cmdName,
                         apConfig.ssid,
                         apConfig.password,
                         apConfig.channel,
                         apConfig.authmode);
    at_port_print(temp);
    at_response_ok();
}

void ICACHE_FLASH_ATTR
at_setupCmdSCwsap(uint8_t id, char *pPara)
{
    char temp[64];
    int8_t len,passLen;
    struct softap_config apConfig;

    os_bzero(&apConfig, sizeof(struct softap_config));
    wifi_softap_get_config(&apConfig);

    if(at_wifiMode == STATION_MODE)
    {
        at_response_error();
        return;
    }
    pPara++;
    len = at_dataStrCpy(apConfig.ssid, pPara, 32);
    apConfig.ssid_len = len;
    
    if(len < 1)
    {
        at_response_error();
        return;
    }
    pPara += (len+3);
    passLen = at_dataStrCpy(apConfig.password, pPara, 64);
    if(passLen == -1 )
    {
        at_response_error();
        return;
    }
    pPara += (passLen+3);
    apConfig.channel = atoi(pPara);
    if(apConfig.channel<1 || apConfig.channel>13)
    {
        at_response_error();
        return;
    }
    pPara++;
    pPara = strchr(pPara, ',');
    pPara++;
    apConfig.authmode = atoi(pPara);
    if(apConfig.authmode >= 5)
    {
        at_response_error();
        return;
    }
    if((apConfig.authmode != 0)&&(passLen < 5))
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
//AT+CWMODESP- wifi mode           | AT+CWMODE=? | AT+CWMODE=<mode>                  | 1= Sta, 2= AP, 3=both, Sta is the default 
//                                                                                     mode of router, AP is a normal mode for devices 
//AT+CWSAPSP - set parameters of AP| AT+CWSAP?   | AT+CWSAP=<ssid>,<pwd>,<chl>,<ecn> | ssid, pwd, chl = channel, ecn = encryption

extern void at_exeCmdCiupdate(uint8_t id);
at_funcationType at_custom_cmd[] = {
        {"+CWMODESP", 9, at_testCmdSCwmode, at_queryCmdSCwmode, at_setupCmdSCwmode, NULL},
        {"+CWSAPSP", 8, NULL, at_queryCmdSCwsap, at_setupCmdSCwsap, NULL}
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
