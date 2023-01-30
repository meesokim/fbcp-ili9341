#pragma once

#include "config.h"

#ifdef KEDEI_TRASH

// Data specific to the KeDei v6.3 display
#define DISPLAY_SET_CURSOR_X 0x2A
#define DISPLAY_SET_CURSOR_Y 0x2B
#define DISPLAY_WRITE_PIXELS 0x2C

#define DISPLAY_NATIVE_WIDTH 320
#define DISPLAY_NATIVE_HEIGHT 480

#define SPI_3WIRE_PROTOCOL
#define DISPLAY_USES_CS1

#define DISPLAY_SPI_DRIVE_SETTINGS (1 | BCM2835_SPI0_CS_CPOL | BCM2835_SPI0_CS_CPHA)

void InitKeDeiTrash(void);
#define InitSPIDisplay InitKeDeiTrash
#endif 