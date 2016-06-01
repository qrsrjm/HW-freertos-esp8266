#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"

#include "cJSON.h"
#include "j1stio.h"
#include "MQTTClient.h"

#include "user_config.h"

#define MAXFILENAME 80
#define BUFSIZE		8000

#define DEFAULTHOST "139.198.0.174"
#define DEFAULTAGENT "574e61736097e903b1c5f0fb"
#define DEFAULTTOKEN "sOoOoKAiPrRVEggwstnMCbINFdezjFmP"

extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);

int gInterval=300, gEInterval, gFinish=0;
char gHost[MAXFILENAME+1];
char gAgent[MAXFILENAME+1], gToken[MAXFILENAME+1];
char gTopicUp[MAXFILENAME+1], gTopicDown[MAXFILENAME+1];
int gPort=1883;
volatile gUpdate = 0;

jNet *gJnet;
jTimer *gTmr;

double gMean = 25;
int gVari = 10;

int GetRandTemp(void)
{
    return (int) (gMean + 2*(random() % gVari)) - gVari;
}

int PublishData(jNet *pJnet)
{
    int res;

    cJSON *root, *son1, *son2, *son3;
    char *out;

    root = cJSON_CreateArray();

    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());
    cJSON_AddStringToObject(son1, "hwid", gAgent);
    cJSON_AddStringToObject(son1, "type", "AGENT");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateArray());
    cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
    cJSON_AddStringToObject(son3, "key", "temperature");
    cJSON_AddNumberToObject(son3, "value", GetRandTemp());
    cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
    cJSON_AddStringToObject(son3, "key", "error");
    cJSON_AddFalseToObject(son3, "value");

    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());
    cJSON_AddStringToObject(son1, "hwid", "ABCDEF0123456");
    cJSON_AddStringToObject(son1, "type", "SENSOR");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateArray());
    cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
    cJSON_AddStringToObject(son3, "key", "temperature");
    cJSON_AddNumberToObject(son3, "value", 25);
    cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
    cJSON_AddStringToObject(son3, "key", "humidity");
    cJSON_AddNumberToObject(son3, "value", 55);

    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("Publishing %s\n", out);

    res = jNetPublishT(pJnet, gTopicUp, out);
    printf("result %d\n", res);
    free(out);
    return res;
}

void SetParas(void)
{
    strcpy(gHost, DEFAULTHOST);
    strcpy(gAgent, DEFAULTAGENT);
    strcpy(gToken, DEFAULTTOKEN);
        
    sprintf(gTopicDown, "power/agents/%s/downstream", gAgent);
    sprintf(gTopicUp, "power/agents/%s/upstream", gAgent);
    gEInterval = gInterval;    
}

void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;
    char *payload =  (char*)message->payload;

    // TODO: Safer
    payload[(int)message->payloadlen] = 0;

    printf("Rcvd: %s \n", payload);

//    ParseMsg(payload);
}


void MQTTWork(void *pv)
{
    int rc, delayS = 1;
    jTimer	tmrInt;
    
    SetParas();
    
    jNet * pJnet = jNetInit();
    if (NULL == pJnet)
    {
        puts("Cannot allocate jnet resources");
        return -1;
    }

    gJnet = pJnet;
    gTmr = &tmrInt;
    
    while(!gFinish)
    {
        if (gWifiStatus != DEVICE_GOTIP)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }
        rc = jNetConnect(pJnet, gHost, gPort, gAgent, gToken);
        if (rc < 0)
        {
            printf("Cannot connect %s: %d, Waiting for %d seconds and retry\n", gHost, rc, delayS);
            vTaskDelay(delayS * 1000 / portTICK_RATE_MS);
            delayS *= 2;
            if (delayS > 30) delayS = 30;
            continue;
        }
        delayS = 1;
        printf("Connect to J1ST.IO server %s:%d succeeded.\n",
               gHost, gPort);
       
        rc = jNetSubscribeT(pJnet, gTopicDown, 2, messageArrived);
        printf("Subscribion result %d\n", rc);

		if (rc != 0) goto clean;

        /* Initial publish */
        if (PublishData(pJnet) != 0) goto clean;
        jTmrStart(&tmrInt, gEInterval);
        do
        {
            if (jTmrExpired(&tmrInt) || gUpdate)
            {
                /* Publish new data every gPubInterval seconds */
                if (PublishData(pJnet) != 0) goto clean;
                gUpdate = 0;
                jTmrStart(&tmrInt, gEInterval);
            }
            /* Make jNet library do background tasks */
            rc = jNetYield(pJnet);
            //printf("Yield %d\n", rc);
            if (rc < 0) break;
        }  while (!gFinish);
        printf("Stopping\n");
        /* Cleanup */
	clean:
        jNetDisconnect(pJnet);
    }
    jNetFree(pJnet);        
}
