#include "config.h"

#ifdef MPI3501

#include "spi.h"

#include <memory.h>
#include <stdio.h>

void ChipSelectHigh()
{
  WAIT_SPI_FINISHED();
  CLEAR_GPIO(GPIO_SPI0_CE0); // Enable Touch
  SET_GPIO(GPIO_SPI0_CE0); // Disable Touch
  __sync_synchronize();
  SET_GPIO(GPIO_SPI0_CE1); // Disable Display
  CLEAR_GPIO(GPIO_SPI0_CE1); // Enable Display
  __sync_synchronize();
}


void InitKeDeiV63()
{
  // If a Reset pin is defined, toggle it briefly high->low->high to enable the device. Some devices do not have a reset pin, in which case compile with GPIO_TFT_RESET_PIN left undefined.
#if defined(GPIO_TFT_RESET_PIN) && GPIO_TFT_RESET_PIN >= 0
  printf("Resetting display at reset GPIO pin %d\n", GPIO_TFT_RESET_PIN);
  SET_GPIO_MODE(GPIO_TFT_RESET_PIN, 1);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  CLEAR_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
  SET_GPIO(GPIO_TFT_RESET_PIN);
  usleep(120 * 1000);
#endif

  // For sanity, start with both Chip selects high to ensure that the display will see a high->low enable transition when we start.
  SET_GPIO(GPIO_SPI0_CE0); // Disable Touch
  SET_GPIO(GPIO_SPI0_CE1); // Disable Display
  usleep(1000);

  // Do the initialization with a very low SPI bus speed, so that it will succeed even if the bus speed chosen by the user is too high.
  spi->clk = 34;
  __sync_synchronize();

  BEGIN_SPI_COMMUNICATION();
  {
    CLEAR_GPIO(GPIO_SPI0_CE0); // Enable Touch
    CLEAR_GPIO(GPIO_SPI0_CE1); // Enable Display

    BEGIN_SPI_COMMUNICATION();

    usleep(25*1000);

    SET_GPIO(GPIO_SPI0_CE0); // Disable Touch
    usleep(25*1000);

    SPI_TRANSFER(0x00000000); // This command seems to be Reset
    usleep(120*1000);

    SPI_TRANSFER(0x00000100);
    usleep(50*1000);
    SPI_TRANSFER(0x00001100);
    usleep(60*1000);

    SPI_TRANSFER(0xB9001100, 0x00, 0xFF, 0x00, 0x83, 0x00, 0x57);
    usleep(5*1000);

    SPI_TRANSFER(0xB6001100, 0x00, 0x2C);
    SPI_TRANSFER(0x11001100/*Sleep Out*/);
    usleep(150*1000);

    SPI_TRANSFER(0x3A001100/*Interface Pixel Format*/, 0x00, 0x55);
    SPI_TRANSFER(0xB0001100, 0x00, 0x68);
    SPI_TRANSFER(0xCC001100, 0x00, 0x09);
    SPI_TRANSFER(0xB3001100, 0x00, 0x43, 0x00, 0x00, 0x00, 0x06, 0x00, 0x06);
    SPI_TRANSFER(0xB1001100, 0x00, 0x00, 0x00, 0x15, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x83, 0x00, 0x44);
    SPI_TRANSFER(0xC0001100, 0x00, 0x24, 0x00, 0x24, 0x00, 0x01, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x08);
    SPI_TRANSFER(0xB4001100, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x2A, 0x00, 0x0D, 0x00, 0x4F);
    SPI_TRANSFER(0xE0001100, 0x00, 0x02, 0x00, 0x08, 0x00, 0x11, 0x00, 0x23, 0x00, 0x2C, 0x00, 0x40, 0x00, 0x4A, 0x00, 0x52, 0x00, 0x48, 0x00, 0x41, 0x00, 0x3C, 0x00, 0x33, 0x00, 0x2E, 0x00, 0x28, 0x00, 0x27, 0x00, 0x1B, 0x00, 0x02, 0x00, 0x08, 0x00, 0x11, 0x00, 0x23, 0x00, 0x2C, 0x00, 0x40, 0x00, 0x4A, 0x00, 0x52, 0x00, 0x48, 0x00, 0x41, 0x00, 0x3C, 0x00, 0x33, 0x00, 0x2E, 0x00, 0x28, 0x00, 0x27, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01);

#define MADCTL_BGR_PIXEL_ORDER (1<<3)
#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
#define MADCTL_ROW_ADDRESS_ORDER_SWAP (1<<7)
#define MADCTL_ROTATE_180_DEGREES (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_ADDRESS_ORDER_SWAP)

    uint8_t madctl = 0;
#ifndef DISPLAY_SWAP_BGR
    madctl |= MADCTL_BGR_PIXEL_ORDER;
#endif
#if defined(DISPLAY_FLIP_ORIENTATION_IN_HARDWARE)
    madctl |= MADCTL_ROW_COLUMN_EXCHANGE;
#endif
#ifdef DISPLAY_ROTATE_180_DEGREES
    madctl ^= MADCTL_ROTATE_180_DEGREES;
#endif
    SPI_TRANSFER(0x36001100/*MADCTL: Memory Access Control*/, 0x00, madctl);

    SPI_TRANSFER(0x29001100/*Display ON*/);

    usleep(200*1000);

    ClearScreen();
  }
#ifndef USE_DMA_TRANSFERS // For DMA transfers, keep SPI CS & TA active.
  END_SPI_COMMUNICATION();
#endif

  // And speed up to the desired operation speed finally after init is done.
  usleep(10 * 1000); // Delay a bit before restoring CLK, or otherwise this has been observed to cause the display not init if done back to back after the clear operation above.
  spi->clk = SPI_BUS_CLOCK_DIVISOR;
}

void TurnBacklightOff()
{
}

void TurnBacklightOn()
{
}

void TurnDisplayOff()
{
}

void TurnDisplayOn()
{
}

void DeinitSPIDisplay()
{
  ClearScreen();
  TurnDisplayOff();
}

#endif

#ifdef KEDEI_TRASH

#include "spi.h"

#include <memory.h>
#include <stdio.h>

#include <bcm2835.h>

void inline spi_write(const char* tbuf, int len)
{
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t i;

    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);
    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);
	// *fifo = tbuf[1];
	*fifo = tbuf[0];
	if (len > 2)
		*fifo = tbuf[2];
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}
/* Writes an number of bytes to SPI */

void inline spi_write2(const char* tbuf, const char *tbuf2)
{
	volatile static uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
	volatile static uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);
    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);
	*fifo = tbuf[1];
	*fifo = tbuf[0];
	*fifo = tbuf[2];
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));
    // bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);
    /* Set TA = 1 */
    // bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);
	*fifo = tbuf2[0];
	*fifo = tbuf2[1];
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));
    // bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}

void inline lcd_data(uint16_t data) {
	static char b1[3] = { 0, 0, 0x15 }, b2[2] = { 0, 0x1f};
	*(unsigned short *)b1 = data;
	spi_write2(b1, b2);
}

void inline lcd_cmd(uint16_t cmd) {
	static char b1[3] = { 0, 0, 0x11 }, b2[2] = { 0, 0x1B};
	*(unsigned short *)b1 = cmd;
	spi_write2(b1, b2);
}

#ifdef SPI_TRANSFER
#undef SPI_TRANSFER
#endif
#define SPI_TRANSFER(cmd, ...) do \
{ \
 	unsigned short dbuf[] = { __VA_ARGS__ }; \
	int len = sizeof(dbuf)/(sizeof(unsigned short)); \
	lcd_cmd(cmd); \
	for(int ix = 0; ix < len; ix++) \
		lcd_data(dbuf[ix]); \
} while(0);

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
	// fprintf(stdout, "bcm2835 library version: %u (0x%08x)\n", v,v);
	
	return 0; // OK!
	
}


void InitKeDeiTrash()
{
 	spi_open();
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
	// lcd_cmd(0x00C8); //GAMMA
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

void DeinitSPIDisplay()
{
  ClearScreen();
//   TurnDisplayOff();
}
#endif