#include "espressif/esp_common.h"

#include "driver/gpio.h"
#include "peri.h"

#define wait1() do { waste+=mask; waste+=mask;} while(0)

void Set7219Reg(short reg, short value)
{
    unsigned short mask = 0x8000;
    unsigned int waste = 0;
    S7219CLK_LOW();
    S7219CS_LOW();
    reg = ((reg & 0x0f) << 8) | (value & 0xff);
    
    do {
        S7219CLK_LOW();
        wait1();
        if (reg & mask) S7219DATA_HIGH();
        else S7219DATA_LOW();
        mask >>= 1;
        wait1();
        S7219CLK_HIGH();
        wait1();
    } while (mask != 0);
    
    S7219CS_HIGH();
    wait1();        
    S7219CLK_LOW();
    wait1();
}

void Init7219GPIO(void)
{
    ETS_INTR_LOCK();
    
    PIN_FUNC_SELECT(S7219CLK_MUX, S7219CLK_FUNC);
    PIN_FUNC_SELECT(S7219CS_MUX, S7219CS_FUNC);
    PIN_FUNC_SELECT(S7219DATA_MUX, S7219DATA_FUNC);

//    GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(S7219CLK_GPIO)), 
//        GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(S7219CLK_GPIO))) 
//            | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)); //open drain;
 
    GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, 
        GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | S7219CLK | S7219CS | S7219DATA);
    
    
//    GPIO_AS_OUTPUT(S7219CLK);
//    GPIO_AS_OUTPUT(S7219CS);
//    GPIO_AS_OUTPUT(S7219DATA);
    
    ETS_INTR_UNLOCK();
}


void Set7219Number(unsigned long l, int dot)
{
    int i=0, j, mask = 1;
    Set7219Decode(0xff);
    do {
        j = l % 10;
        l /= 10;
        if (i != 0 && l == 0 && j == 0) j = 0x0f;
        if ((dot & mask) != 0) j |= 0x80;
        Set7219Digit(i, j); 
        ++i;
        mask <<= 1;
    } while(i < 8);
}

void Init7219(void)
{
    Init7219GPIO();
    
    S7219CS_HIGH();
    S7219CLK_LOW();
    
    os_delay_us(1);
    
    Set7219ScanLimit(7);
    Set7219Intensity(5);
    Set7219Decode(0xff);
    Set7219Number(0, 0); // Clear Screen
    Set7219Op(1);
}
