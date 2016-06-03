#ifndef _ZEIN_PERI_
#define _ZEIN_PERI_

#define S7219CLK_MUX    PERIPHS_IO_MUX_MTCK_U
#define S7219CS_MUX     PERIPHS_IO_MUX_MTDI_U
#define S7219DATA_MUX   PERIPHS_IO_MUX_GPIO4_U

#define S7219CLK_GPIO   13
#define S7219CS_GPIO    12
#define S7219DATA_GPIO  4

#define S7219CLK_FUNC   FUNC_GPIO13
#define S7219CS_FUNC    FUNC_GPIO12
#define S7219DATA_FUNC  FUNC_GPIO4

#define S7219CLK    (1<<S7219CLK_GPIO)
#define S7219CS     (1<<S7219CS_GPIO)
#define S7219DATA   (1<<S7219DATA_GPIO)

/////////////
void Init7219(void);

void Set7219Reg(short reg, short value);

#define S7219CLK_LOW()  gpio_output_conf(0, S7219CLK, S7219CLK, 0)
#define S7219CLK_HIGH() gpio_output_conf(S7219CLK, 0, S7219CLK, 0)
#define S7219CS_LOW()   gpio_output_conf(0, S7219CS, S7219CS, 0)
#define S7219CS_HIGH()  gpio_output_conf(S7219CS, 0, S7219CS, 0)
#define S7219DATA_LOW() gpio_output_conf(0, S7219DATA, S7219DATA, 0)
#define S7219DATA_HIGH() gpio_output_conf(S7219DATA, 0, S7219DATA, 0)

#define Set7219Digit(pos, v)    Set7219Reg(((pos)&0x0f)+1, (v) & 0xff)

#define Set7219Decode(v)        Set7219Reg(0x09, v)    
#define Set7219Intensity(v)     Set7219Reg(0x0a, (v) & 0x0f)
#define Set7219ScanLimit(v)     Set7219Reg(0x0b, (v) & 0x0f)
// 1 Normal, 0 Shutdown
#define Set7219Op(v)            Set7219Reg(0x0c, v)

void Set7219Number(unsigned long l, int dot);

////////////

int Init8591(int i2cAddr);
unsigned short Read8591(int no);
int Set8591Value(short d);

#endif