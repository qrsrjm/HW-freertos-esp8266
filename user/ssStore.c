#include "esp_common.h"
#include "user_config.h"

static struct ss_saved_param sStored;

uint16 ICACHE_FLASH_ATTR
ssGetConf(char *ssid, char *pwd)
{
    if (ssid == NULL || pwd == NULL) return 0;
    if (sStored.cfgValid != CONFIG_MAGIC) return 0;
    
    strcpy(ssid, sStored.ssid);
    strcpy(pwd, sStored.pwd);
    //memcpy(ip, &sStored.ip, sizeof(sStored.ip));
    
    return 1;
}

uint16 ICACHE_FLASH_ATTR
ssGetConfB(char **ssid, char **pwd)
{
    if (ssid == NULL || pwd == NULL) return 0;
    if (sStored.cfgValid != CONFIG_MAGIC) return 0;
    
    *ssid = sStored.ssid;
    *pwd = sStored.pwd;
    //*ip = &sStored.ip;
   
    return 1;
}

uint16 ICACHE_FLASH_ATTR
ssSSIDOK(void)
{
   if (sStored.cfgValid != CONFIG_MAGIC) return 0;
   return 1; 
} 

void ICACHE_FLASH_ATTR
ssSetConf(char *ssid, char *pwd)
{
    if (ssid == NULL || pwd == NULL) return;
    
    strcpy(sStored.ssid, ssid);
    strcpy(sStored.pwd, pwd);
    //memcpy(&sStored.ip, ip, sizeof(sStored.ip));    
    sStored.cfgValid = CONFIG_MAGIC;
    
    system_param_save_with_protect(ESP_PARAM_START_SEC, 
        &sStored, sizeof(sStored));    
}

void ICACHE_FLASH_ATTR
ssInitConf(void)
{
    sStored.cfgValid = 0;
    system_param_load(ESP_PARAM_START_SEC, 0, &sStored, 
           sizeof(sStored));
}
