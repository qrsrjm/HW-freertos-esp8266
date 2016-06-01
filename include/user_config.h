/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define KEYNO       3
#define LIVENO      4
#define LIVEPORT    (1<<LIVENO)
#define KEYPORT     (1<<KEYNO)

#define ESP_PARAM_START_SEC		0x1FD
#define CONFIG_MAGIC    0x5a45

struct ip_all {
    ip_addr_t   ip_addr;
    ip_addr_t   netmask;
    ip_addr_t   gw;
    ip_addr_t   pri_dns;
    ip_addr_t   sec_dns;
};

struct ss_saved_param {
    char   ssid[34];
    uint16  cfgValid;
    char   pwd[66];
    //struct ip_all ip;
    uint16  pad;
};

uint16 ssGetConfB(char **ssid, char **pwd);
/*
uint16 ssGetConf(char *ssid, char *pwd);
uint16 ssSSIDOK(void);
void ssSetConf(char *ssid, char *pwd);
void ssInitConf(void);
int ssSetSta(void);
*/

extern void uart_div_modify(int, int);

void  InitDaemon(void);

//void UserScan(void);
//void GoSmartConfig(void);

enum scStatus {
    scConnecting = 0,
    scConDone,
    scConSmart,
    scError
};
volatile enum scStatus LEDType;

enum scWifiCon {
    DEVICE_INIT = 0,
    DEVICE_CONNECTING,
    DEVICE_SMART,
    DEVICE_LOST,
    DEVICE_GOTIP
};
//extern struct station_config s_staconf;

#endif

