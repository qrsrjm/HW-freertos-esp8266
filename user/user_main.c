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

#include "esp_common.h"
#include "driver/gpio.h"
#include "driver/key.h"
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user_config.h"
#include "peri.h"

/*

LOCAL volatile scStatus LEDType = 0;
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[SENSOR_KEY_NUM];

LOCAL void 
UserLongPress(void)
{
    // SetActive ?
    system_restore();
    system_restart();
}
*/


volatile int gDis2 = 0;
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[1];

LOCAL void
FlashKeyShortPress(void)
{
    printf("Flash pressed\n");    
}

LOCAL void 
FlashKeyLongPress(void)
{
    system_restore();
    system_restart();
}

void GPIOPreInit(void)
{
    single_key[0] = key_init_single(0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0, 
                                    FlashKeyLongPress, FlashKeyShortPress);
    keys.key_num = 1;
    keys.single_key = single_key;
    key_init(&keys);
        
    gpio16_output_conf();
    
    i2c_master_gpio_init();
    Init7219();
    Init8591(0x90);
    gDis2 = 0;
    
}

void IRAM_ATTR 
LEDTask(void *para)
{
    int i = 0, d;
    unsigned long t;
    static long count = 0;
    while(1){
        switch(LEDType) {
        case scConnecting: // connect to AP
            d = 200 / portTICK_RATE_MS;
            break;
        case scConDone: // AP connected
            d = 500 / portTICK_RATE_MS;
            break;
        case scConSmart: // Station
            if (i) d = 100 / portTICK_RATE_MS;
            else d = 500 / portTICK_RATE_MS;
            break;
        default: 
            d = 80 / portTICK_RATE_MS;
            break;   
        }
        vTaskDelay(d);
        
        gpio16_output_set(i);
        //GPIO_OUTPUT(LIVEPORT, i);
        i = 1-i;
        
        count++;
        t = count*100 + ((gDis2 != 0) ?gDis2 : Read8591(0));
        Set7219Number(t, 0x04);
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    //ssInitConf();
    GPIOPreInit();
    
    uart_div_modify(0, UART_CLK_FREQ / (115200));
    SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST | UART_TXFIFO_RST);
    
    printf("SDK version:%s\n", system_get_sdk_version());
    printf("Chip ID: 0x%x\n", system_get_chip_id());
    
    wifi_set_opmode_current(STATION_MODE);

    xTaskCreate(LEDTask, "LEDTask", 192, NULL, 2, NULL);
    InitDaemon();
}

