/* sd_spi.h - funciones de la SD que FatFs llama desde user_diskio.c */
#ifndef INC_SD_SPI_H_
#define INC_SD_SPI_H_

#if __has_include("diskio.h")
#include "diskio.h"
#define SD_FATFS_AVAILABLE 1

DSTATUS SD_disk_initialize(BYTE drv);
DSTATUS SD_disk_status(BYTE drv);
DRESULT SD_disk_read(BYTE drv, BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_write(BYTE drv, const BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl(BYTE drv, BYTE cmd, void *buff);

#else
#define SD_FATFS_AVAILABLE 0
#endif

#endif /* INC_SD_SPI_H_ */
