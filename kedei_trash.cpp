#include <stdio.h>
#include <stdint.h>
#include <bcm2835.h>
#include <unistd.h>
#include "kedei_trash.h"

int spi_open(void) {
	int r;
	uint32_t v;
	
	r = bcm2835_init(); // return 0 on fail
	if (r != 1) return -1;
	
	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // MSB bit goes first
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3); // mode 0
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8	); // SPI_CLK 31.25MHz - MAX
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1); // CS1 - for now, with TP CS0 will be used too
	// setup both SPI.CSx
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // CS0 - active Low
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW); // CS1 - active Low
	
	v = bcm2835_version();
	fprintf(stdout, "bcm2835 library version: %u (0x%08x)\n", v,v);
	
	return 0; // OK!
	
}

#define bcm2835_peri_set_bits(paddr, a, b) (*paddr = a | *paddr)
#define bcm2835_peri_read_nb(paddr) (*paddr)

void spi_write2(const char* tbuf, const char *tbuf2)
{
	volatile static uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
	volatile static uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;

    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA | BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_TA);
	*fifo = tbuf[1];
	*fifo = tbuf[0];
	*fifo = tbuf[2];
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA | BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_TA);
	*fifo = tbuf2[0];
	*fifo = tbuf2[1];
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));
 
}

void lcd_data8(uint8_t *data) {
	static char b1[3] = { 0, 0, 0x15 }, b2[2] = { 0, 0x1f};
    b1[1] = data[0];
    b1[0] = data[1];
    // printf("%02x", b1[1]);
    // printf("%02x,", b1[0]);
	spi_write2(b1, b2);
}

void lcd_data(uint16_t data) {
	static char b1[3] = { 0, 0, 0x15 }, b2[2] = { 0, 0x1f};
    *(uint16_t *)b1 = data;
    // printf("%02x", b1[1]);
    // printf("%02x.", b1[0]);
	spi_write2(b1, b2);
}

void lcd_cmd(uint8_t cmd) {
	static char b1[3] = { 0, 0, 0x11 }, b2[2] = { 0, 0x1B};
	b1[0] = cmd;
    // printf("\n%02x:", cmd);
	// b1[0] = ((char *)&data)[1];
	// b1[1] = ((char *)&data)[0];
	spi_write2(b1, b2);
}

#define SPI_TRANSFER(cmd, ...) do \
{ \
 	int16_t dbuf[] = { __VA_ARGS__ }; \
	int len = sizeof(dbuf)/(sizeof(int16_t)); \
	lcd_cmd(cmd); \
	for(int ix = 0; ix < len; ix++) \
		lcd_data(dbuf[ix]); \
} while(0);

void lcd_setptr(void) {
	lcd_cmd(0x002b);
	lcd_data(0x0000); 
	lcd_data(0x0000); // 0
	lcd_data(0x0001);
	lcd_data(0x003f); //319
	
	lcd_cmd(0x002a);
	lcd_data(0x0000);
	lcd_data(0x0000); // 0
	lcd_data(0x0001);
	lcd_data(0x00df); // 479
	
	lcd_cmd(0x002c);
}
	
void lcd_setarea(uint16_t x, uint16_t y) {
	lcd_cmd(0x002b);
	lcd_data(y>>8);
	lcd_data(0x00ff&y);
	lcd_data(0x0001);
	lcd_data(0x003f);

	lcd_cmd(0x002a);
	lcd_data(x>>8) ;
	lcd_data(0x00ff&x) ;
	lcd_data(0x0001);
	lcd_data(0x00df);
	lcd_cmd(0x002c);
}

void lcd_setarea2(uint16_t sx, uint16_t sy, uint16_t x, uint16_t y) {
	
	if (sx>479)	sx=0;
	if (sy>319) sy=0;
	if (x>479) x=479;
	if (y>319) y=319;
	
	lcd_cmd(0x002b);
	lcd_data(sy>>8) ;
	lcd_data(0x00ff&sy);
	lcd_data(y>>8);
	lcd_data(0x00ff&y);

	lcd_cmd(0x002a);
	lcd_data(sx>>8) ;
	lcd_data(0x00ff&sx) ;
	lcd_data(x>>8);
	lcd_data(0x00ff&x);
	
	lcd_cmd(0x002c);
}

void lcd_fill(uint16_t color565) {
	// static char b1[3] = { 0, 0, 0x15 }, b2[2] = { 0, 0x1f};
	// b1[0] = ((char *)&color565)[1];
	// b1[1] = ((char *)&color565)[0];
	lcd_setptr();
	for(int x=0; x<153601;x++) {
		lcd_data(color565);
		// bcm2835_spi_writenb(b1, 3);
		// bcm2835_spi_writenb(b2, 2);
	}
}

void lcd_fill2(uint16_t sx, uint16_t sy, uint16_t x, uint16_t y, uint16_t color565) {
	uint16_t tmp=0;
	int cnt;
	if (sx>479) sx=0;
	if (sy>319) sy=0;
	if (x>479)  x=479;
	if (y>319)  y=319;
	
	if (sx>x) {
		tmp=sx;
		sx=x;
		x=tmp;
	}
	
	if (sy>y) {
		tmp=sy;
		sy=y;
		y=tmp;
	}
	
	cnt = (y-sy) * (x-sx);
	lcd_setarea2(sx,sy,x,y);
	for(int t=0;t<cnt;t++) {
		lcd_data(color565);
	}	
}

void lcd_init()
{
	SPI_TRANSFER(0x0);
	usleep(50);
	SPI_TRANSFER(0x1);
	usleep(1000);
	SPI_TRANSFER(0x00B0, 0x0);
	SPI_TRANSFER(0x0011);
	// lcd_cmd(0x0011);
	// SPI_TRANSFER(0x11);
	usleep(50);

	SPI_TRANSFER(0x00B0, 0x02, 0, 0, 0);
	// lcd_cmd(0x00B3);
	// lcd_data(0x0002);
	// lcd_data(0x0000);
	// lcd_data(0x0000);
	// lcd_data(0x0000);

	SPI_TRANSFER(0xc0, 0x10, 0x003B, 0x0000, 0x0002, 0x0000, 0x0001, 0x0000, 0x0043);
	// lcd_cmd(0x00C0);
	// lcd_data(0x0010);//13
	// lcd_data(0x003B);//480
	// lcd_data(0x0000);
	// lcd_data(0x0002);
	// lcd_data(0x0000);
	// lcd_data(0x0001);
	// lcd_data(0x0000);//NW
	// lcd_data(0x0043);

	SPI_TRANSFER(0x00C1, 0x0008, 0x0016, 0x0008, 0x0008);
	// lcd_cmd(0x00C1);
	// lcd_data(0x0008);//w_data(0x0008);
	// lcd_data(0x0016);//w_data(0x0016);//CLOCK
	// lcd_data(0x0008);
	// lcd_data(0x0008);

	SPI_TRANSFER(0x00C4, 0x0011, 0x0007, 0x0003, 0x0003);
	// lcd_cmd(0x00C4);
	// lcd_data(0x0011);
	// lcd_data(0x0007);
	// lcd_data(0x0003);
	// lcd_data(0x0003);

	SPI_TRANSFER(0x00C6, 0x0000);
	// lcd_cmd(0x00C6);
	// lcd_data(0x0000);

	SPI_TRANSFER(0x00C8, 0x0003, 0x0003, 0x0013, 0x005C, \
				 0x0003, 0x0007, 0x0014, 0x0008, 0x0000, \
				 0x0021, 0x0008, 0x0014, 0x0007, 0x0053, \
				 0x000C, 0x0013, 0x0003, 0x0021, 0x0000);
	lcd_cmd(0x00C8); //GAMMA
	// lcd_data(0x0003);
	// lcd_data(0x0003);
	// lcd_data(0x0013);
	// lcd_data(0x005C);
	// lcd_data(0x0003);
	// lcd_data(0x0007);
	// lcd_data(0x0014);
	// lcd_data(0x0008);
	// lcd_data(0x0000);
	// lcd_data(0x0021);
	// lcd_data(0x0008);
	// lcd_data(0x0014);
	// lcd_data(0x0007);
	// lcd_data(0x0053);
	// lcd_data(0x000C);
	// lcd_data(0x0013);
	// lcd_data(0x0003);
	// lcd_data(0x0003);
	// lcd_data(0x0021);
	// lcd_data(0x0000);

	SPI_TRANSFER(0x0035, 0x0000);
	// lcd_cmd(0x0035);
	// lcd_data(0x0000);

	SPI_TRANSFER(0x0036, 0x0028);
	// lcd_cmd(0x0036);  
	// lcd_data(0x0028);

	SPI_TRANSFER(0x003A, 0x0055);
	// lcd_cmd(0x003A);
	// lcd_data(0x0055); //55 lgh

	SPI_TRANSFER(0x0044, 0x0000, 0x0001);
	// lcd_cmd(0x0044);
	// lcd_data(0x0000);
	// lcd_data(0x0001);

	SPI_TRANSFER(0x00B6, 0x0000, 0x0002, 0x003B);
	// lcd_cmd(0x00B6);
	// lcd_data(0x0000);
	// lcd_data(0x0002); //220 GS SS SM ISC[3:0]
	// lcd_data(0x003B);

	SPI_TRANSFER(0x00D0, 0x0007, 0x0007, 0x001D);
	// lcd_cmd(0x00D0);
	// lcd_data(0x0007);
	// lcd_data(0x0007); //VCI1
	// lcd_data(0x001D); //VRH

	SPI_TRANSFER(0x00D1, 0x0000, 0x0003, 0x0000);
	// lcd_cmd(0x00D1);
	// lcd_data(0x0000);
	// lcd_data(0x0003); //VCM
	// lcd_data(0x0000); //VDV

	SPI_TRANSFER(0x00D2, 0x0003, 0x0014, 0x0004);
	// lcd_cmd(0x00D2);
	// lcd_data(0x0003);
	// lcd_data(0x0014);
	// lcd_data(0x0004);

	SPI_TRANSFER(0xE0, 0x1f, 0x2C, 0x2C, 0x0B, 0x0C, 0x04, 0x4C, 0x64, 0x36, 0x03, 0x0E, 0x01, 0x10, 0x01, 0x00);
	// lcd_cmd(0xE0);  
	// lcd_data(0x1f);  
	// lcd_data(0x2C);  
	// lcd_data(0x2C);  
	// lcd_data(0x0B);  
	// lcd_data(0x0C);  
	// lcd_data(0x04);  
	// lcd_data(0x4C);  
	// lcd_data(0x64);  
	// lcd_data(0x36);  
	// lcd_data(0x03);  
	// lcd_data(0x0E);  
	// lcd_data(0x01);  
	// lcd_data(0x10);  
	// lcd_data(0x01);  
	// lcd_data(0x00);  

	SPI_TRANSFER(0XE1, 0x1f, 0x3f, 0x3f, 0x0f, 0x1f, 0x0f, 0x7f, 0x32, 0x36, 0x04, 0x0B, 0x00, 0x19, 0x14, 0x0F);
	// lcd_cmd(0XE1);  
	// lcd_data(0x1f);  
	// lcd_data(0x3f);  
	// lcd_data(0x3f);  
	// lcd_data(0x0f);  
	// lcd_data(0x1f);  
	// lcd_data(0x0f);  
	// lcd_data(0x7f);  
	// lcd_data(0x32);  
	// lcd_data(0x36);  
	// lcd_data(0x04);  
	// lcd_data(0x0B);  
	// lcd_data(0x00);  
	// lcd_data(0x19);  
	// lcd_data(0x14);  
	// lcd_data(0x0F);  

	SPI_TRANSFER(0xE2, 0x0f, 0x0f, 0x0f);
	// lcd_cmd(0xE2);
	// lcd_data(0x0f);
	// lcd_data(0x0f);
	// lcd_data(0x0f);

	SPI_TRANSFER(0xE3, 0x0f, 0x0f, 0x0f);
	// lcd_cmd(0xE3);
	// lcd_data(0x0f);
	// lcd_data(0x0f);
	// lcd_data(0x0f);

	SPI_TRANSFER(0x13);
	// lcd_cmd(0x13);

	SPI_TRANSFER(0x0029);
	// lcd_cmd(0x0029);
	usleep(20); //mdelay(20);

	SPI_TRANSFER(0x00B4, 0x0000);
	// lcd_cmd(0x00B4);
	// lcd_data(0x0000);
	usleep(20); //mdelay(20);

	SPI_TRANSFER(0x002C);
	// lcd_cmd(0x002C);

	SPI_TRANSFER(0x00D2, 0x0003, 0x0014, 0x0004);
	// lcd_cmd(0x002A); 
	// lcd_data(0x0000);
	// lcd_data(0x0000);
	// lcd_data(0x0001);
	// lcd_data(0x000dF);

	SPI_TRANSFER(0x002B, 0x0000, 0x0000, 0x0001, 0x003f);
	// lcd_cmd(0x002B);  
	// lcd_data(0x0000);
	// lcd_data(0x0000);
	// lcd_data(0x0001);
	// lcd_data(0x003f); 

	SPI_TRANSFER(0x002c);
	// lcd_cmd(0x002c);       
}

extern void ClearScreen();
void InitKeDeiTrash() 
{
    spi_open();
    lcd_init();
    // lcd_fill(0xfff);
    // ClearScreen();
}

void DeinitSPIDisplay()
{
    ClearScreen();
}