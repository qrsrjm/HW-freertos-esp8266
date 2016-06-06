#include "espressif/esp_common.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

static short s8591Addr = 0x90;

int Set8591Ch(int i)
{
    i2c_master_start();
    i2c_master_writeByte(s8591Addr);
    if (i2c_master_getAck())  goto Fail;
    
    i2c_master_writeByte(0x40 | (i & 3));
    if (i2c_master_getAck())  goto Fail;
    return 0;
    
Fail:
    printf("No ack while writing\n");
    return -1;   
}

int Set8591Value(short d)
{
    if (Set8591Ch(0)) goto Fail;
    
    i2c_master_writeByte(d);
    if (i2c_master_getAck())  goto Fail;
    
    i2c_master_stop();
    return 0;
Fail:
    i2c_master_stop();
    return -1;
}

int Init8591(int i2cAddr)
{
    s8591Addr = i2cAddr & 0xfffe;
    int ret = Set8591Ch(0);
    printf("Init8591: %d\n", ret);
    i2c_master_stop();
    return ret;    
}

unsigned short Read8591(int no)
{
    if (no < 0 || no > 3) return 0xffff;
    if (Set8591Ch(no)) goto Fail;
    i2c_master_stop();
 
    vTaskDelay(2);
       
    i2c_master_start();
    i2c_master_writeByte(s8591Addr+1);
    if (i2c_master_getAck())  goto Fail;
    
    unsigned short t = i2c_master_readByte();
    i2c_master_setAck(0);
    t = i2c_master_readByte();
    i2c_master_setAck(1);
    i2c_master_stop();
    return t;
       
Fail:
    printf("Read8591 Failed\n");
    i2c_master_stop();
    return 0xffff;    
}
