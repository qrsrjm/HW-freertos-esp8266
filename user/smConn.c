#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "user_config.h"

#include "espressif/espconn.h"
#include "espressif/airkiss.h"


#define AP_SSID     "ZEN_%06x"
#define AP_PASSWORD "12345678" 

#define DEMO_SSID     "ZEN_DEMO"
#define DEMO_PASSWORD "12345678" 

volatile enum scWifiCon gWifiStatus = DEVICE_INIT;
volatile enum scStatus LEDType = scConnecting;

xQueueHandle QueueStop = NULL;

void wifi_event_cb(System_Event_t *evt)
{
    printf("Event %x: ", evt->event_id);
    switch (evt->event_id) {
    case EVENT_STAMODE_CONNECTED:
        printf("Connect to ssid %s, channel %d\n",
            evt->event_info.connected.ssid,
            evt->event_info.connected.channel);
        break;
    case EVENT_STAMODE_DISCONNECTED:
        printf("Disconnect from ssid %s, reason %d\n",
            evt->event_info.disconnected.ssid,
            evt->event_info.disconnected.reason);
        gWifiStatus = DEVICE_LOST;
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        printf("Mode: %d -> %d\n",
            evt->event_info.auth_change.old_mode,
            evt->event_info.auth_change.new_mode);
        break;
    case EVENT_STAMODE_GOT_IP:
        printf("IP:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
            IP2STR(&evt->event_info.got_ip.ip),
            IP2STR(&evt->event_info.got_ip.mask),
            IP2STR(&evt->event_info.got_ip.gw));
        printf("\n");
        gWifiStatus = DEVICE_GOTIP;
        break;
    case EVENT_STAMODE_DHCP_TIMEOUT:
        printf("DHCP timed out.\n");
        break;        
    case EVENT_SOFTAPMODE_STACONNECTED:
        printf("Station: " MACSTR "join, AID = %d\n",
            MAC2STR(evt->event_info.sta_connected.mac),
            evt->event_info.sta_connected.aid);
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
        printf("Station: " MACSTR "leave, AID = %d\n",
            MAC2STR(evt->event_info.sta_disconnected.mac),
            evt->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
}

// Set the device in AP mode and waiting for config
void ssSetAP(void)
{
    wifi_set_opmode(SOFTAP_MODE);
    struct softap_config *config = (struct softap_config *)
        zalloc(sizeof(struct softap_config)); // initialization
    wifi_softap_get_config(config); // Get soft-AP config first.
    sprintf(config->ssid, AP_SSID, system_get_chip_id());
    sprintf(config->password, AP_PASSWORD);
    config->authmode = AUTH_WPA_WPA2_PSK;
    config->ssid_len = 0;           // or its actual SSID length
    config->max_connection = 4;
    wifi_softap_set_config(config); // Set ESP8266 soft-AP config
    free(config);
    
    wifi_set_event_handler_cb(wifi_event_cb);
    
    // More work here
    //  Set a Listener, 
    //  Accept connections
    //  And Reboot    
}

int ssSetSta(void)
{
    char *sSSID, *sPwd;
    //struct ip_all *sIP;
    //struct ip_info info;
    
    uint16 res = ssGetConfB(&sSSID, &sPwd);
    if (res <= 0) 
    {
        printf("Config error.\n");
        return -1;
    }
       
    struct station_config * config = (struct station_config *)
        zalloc(sizeof(struct station_config));
#if 0
    strcpy((char *)config->ssid, sSSID);
    strcpy((char *)config->password, sPwd);
#else
    strcpy((char *)config->ssid, DEMO_SSID);
    strcpy((char *)config->password, DEMO_PASSWORD);
#endif    
    wifi_station_set_config(config);
    free(config);
    
#if 0
    if (sIP->ip_addr == 0)
    {
        wifi_station_dhcpc_stop();
        info.ip = sIP->ip_addr;
        info.gw = sIP->gw;
        info.netmask = sIP->betmask;
        // More on DNS
        wifi_set_ip_info(STATION_IF, &info);
    }
#endif    
    
    wifi_station_connect(); 
    wifi_set_event_handler_cb(wifi_event_cb);
    // More work on callback 
    return 0;      
}


void smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;
	
	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};

                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            } else {
            	//SC_TYPE_AIRKISS - support airkiss v2.0
                // Not implemented yet
				//airkiss_start_discover();
			}
            smartconfig_stop();
            gWifiStatus = DEVICE_GOTIP;
            break;
    }
}

void MQTTWork(void *pv);


LOCAL struct station_config s_staconf;

void WifiDaemon(void *pv)
{
    vTaskDelay(500 / portTICK_RATE_MS); // ensure user_init is done
    wifi_set_opmode(STATION_MODE);
    gWifiStatus = DEVICE_INIT;
        
    if (wifi_station_get_config_default(&s_staconf) == true)
    {
        if (strlen(s_staconf.ssid) != 0) {
            gWifiStatus = DEVICE_CONNECTING;
            LEDType = scConnecting;
            /*
            {
                struct station_config config;
                memset(&config, 0, sizeof(config));
                strncpy(config.ssid, s_staconf.ssid, 32);
                strncpy(config.password, s_staconf.password, 64);
                config.bssid_set = 0;
                wifi_station_set_config(&config);
            }
            */
            printf("Connecting to %s\n", s_staconf.ssid);
            wifi_set_event_handler_cb(wifi_event_cb);
            wifi_station_connect();
            while (gWifiStatus != DEVICE_GOTIP)
            {
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
            goto ConfigDone;
        }             
    }
    // Do smart Config
    LEDType = scConSmart;
    gWifiStatus = DEVICE_SMART;
    printf("SmartCfg\n");
    smartconfig_start(smartconfig_done);
    while (gWifiStatus != DEVICE_GOTIP)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
        
ConfigDone:
    LEDType = scConDone;
    printf("Starting MQTT Client!\n");
    
    xTaskCreate(MQTTWork, "MQTT worker", 1024, NULL, 5, NULL);
    while(1)
    {
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    
}

void  InitDaemon(void)
{
    if (QueueStop == NULL)  QueueStop = xQueueCreate(1, 2);

    if (QueueStop != NULL){
        xTaskCreate(WifiDaemon, "Wifi Daemon", 256, NULL, 5, NULL);
    } 
}
    