#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "stdlib.h"

#include "lwip/sockets.h"

#include "cJSON.h"
#include "j1stio.h"
#include "MQTTClient.h"

#include "user_config.h"
#include "peri.h"

#define MAXFILENAME 80
#define BUFSIZE		2000

#define DEFAULTHOST "139.198.0.174"
#define DEFAULTAGENT "574e61736097e903b1c5f0fb"
#define DEFAULTTOKEN "sOoOoKAiPrRVEggwstnMCbINFdezjFmP"

extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);

int gEInterval=300, gFinish=0;
char gHost[MAXFILENAME+1];
char gAgent[MAXFILENAME+1], gToken[MAXFILENAME+1];
char gTopicUp[MAXFILENAME+1], gTopicDown[MAXFILENAME+1];
int gPort=1883;
volatile int gUpdate = 0;
extern xQueueHandle QueueStop;

jNet *gJnet;

int gMean = 25, gVari = 10;

int GetRandTemp(void)
{
    return (int) (gMean + 2*(rand() % gVari)) - gVari;
}

int PublishData(jNet *pJnet, int source)
{
    int res;

    cJSON *root, *son1, *son2;
    char *out;

    root = cJSON_CreateArray();

    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());
    cJSON_AddStringToObject(son1, "hwid", gAgent);
    cJSON_AddStringToObject(son1, "type", "AGENT");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());
    cJSON_AddNumberToObject(son2, "temperature", GetRandTemp());
    if (source == 0)  cJSON_AddFalseToObject(son2, "error");
    else cJSON_AddTrueToObject(son2, "error");

    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());
    cJSON_AddStringToObject(son1, "hwid", "ABCDEF0123456");
    cJSON_AddStringToObject(son1, "type", "SENSOR");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());
    cJSON_AddNumberToObject(son2, "lumen", Read8591(0));
    cJSON_AddNumberToObject(son2, "Volt", Read8591(3));

    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("Publishing %s\n", out);

    res = jNetPublishT(pJnet, gTopicUp, out);
    //printf("result %d\n", res);
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
}


void UpdateInterval(int newInterval)
{
    if (newInterval > 0 && newInterval < 3000) {
        gEInterval = newInterval;
    }
}


void AnaDisplay(cJSON *item)
{
    cJSON *value = cJSON_GetObjectItem(item, "data");
    if (value != NULL && value->type == cJSON_Number)
    {
        int data  = (int)(value->valuedouble + 0.0000001);
        printf("Rcv: Display: %d\n", data);
        if (data >= 0 && data < 100) gDis2 = data;   
    }    
}

void AnaInterval(cJSON *item)
{
    cJSON *value = cJSON_GetObjectItem(item, "sec");
    if (value != NULL && value->type == cJSON_Number)
    {
        int interval  = (int)(value->valuedouble + 0.0000001);
        printf("Rcv: Interval: %d\n", interval);
        UpdateInterval(interval);
    }
}

void AnaGap(cJSON *item)
{
    cJSON *value = cJSON_GetObjectItem(item, "mean");
    if (value != NULL && value->type == cJSON_Number)
        gMean = (int)(value->valuedouble);
    value = cJSON_GetObjectItem(item, "vari");
    if (value != NULL && value->type == cJSON_Number)
        gVari = (int)(value->valuedouble+0.00001);
    if (gVari < 1) gVari = 1;
    printf("TempGen set: %d + (%d)\n", gMean, gVari);
}

void CheckCmd(cJSON *root, const char *key, void (*func)(cJSON *))
{
    cJSON *item;

    cJSON * cmdArray = cJSON_GetObjectItem(root, key);
    if (cmdArray == NULL) return;
        
    cJSON_ArrayForEach(item, cmdArray)
    {
        cJSON *sub = cJSON_GetObjectItem(item, "hwid");
        if (sub != NULL && sub->type == cJSON_String &&
                !strcmp(gAgent, sub->valuestring))
            func(item);        
    }
}

void ParseMsg(char *payload)
{
    cJSON * root = cJSON_Parse(payload);
    if (!root) return;

    CheckCmd(root, "SetInterval", AnaInterval);
    CheckCmd(root, "SetRand", AnaGap);
    CheckCmd(root, "SetDisplay", AnaDisplay);

    if (root) cJSON_Delete(root);
}

void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;
    char *payload =  (char*)message->payload;

    // TODO: Safer
    payload[(int)message->payloadlen] = 0;

    printf("Rcvd: %s \n", payload);

    ParseMsg(payload);
}


void MQTTWork(void *pv)
{
    int rc, delayS = 1;
    
    SetParas();
    
    jNet * pJnet = jNetInit();
    if (NULL == pJnet)
    {
        puts("Cannot allocate jnet resources");
        return;
    }

    gJnet = pJnet;
    
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
        if (PublishData(pJnet, 0) != 0) goto clean;
        do
        {
            short ret;
            if (xQueueReceive(QueueStop, &ret, 1) == pdPASS)
            {
                printf("Queue %d\n", ret);
                if (PublishData(pJnet, ret)) goto clean;
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
