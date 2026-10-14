/* sd_spi.c - driver de la tarjeta SD por SPI1 (CS en PB6) */
#include "main.h"
#include "sd_spi.h"

#if SD_FATFS_AVAILABLE                       /* solo si FATFS esta activado en CubeMX */

extern SPI_HandleTypeDef hspi1;              /* lo genera CubeMX en main.c */

#define SD_CS_LOW()   HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET)

/* Comandos */
#define CMD0   (0)          /* GO_IDLE_STATE        */
#define CMD1   (1)          /* SEND_OP_COND (MMC)   */
#define ACMD41 (0x80 + 41)  /* SEND_OP_COND (SD)    */
#define CMD8   (8)          /* SEND_IF_COND         */
#define CMD9   (9)          /* SEND_CSD             */
#define CMD12  (12)         /* STOP_TRANSMISSION    */
#define CMD16  (16)         /* SET_BLOCKLEN         */
#define CMD17  (17)         /* READ_SINGLE_BLOCK    */
#define CMD24  (24)         /* WRITE_BLOCK          */
#define CMD55  (55)         /* APP_CMD              */
#define CMD58  (58)         /* READ_OCR             */

/* Tipo de tarjeta */
#define CT_MMC   0x01
#define CT_SD1   0x02
#define CT_SD2   0x04
#define CT_BLOCK 0x08       /* direcciones por sector (SDHC/SDXC) */

static volatile DSTATUS stat = STA_NOINIT;
static uint8_t card_type = 0;

/* Paso donde fallo la SD: 0 ok, 1 CMD0, 2 ACMD41, 3 CMD58/CMD16 */
uint8_t sd_init_step = 0;

/* ---------------------------------------------------------------------- */
/* SPI                                                                     */
/* ---------------------------------------------------------------------- */
static void spi_speed(uint32_t prescaler)
{
    hspi1.Init.BaudRatePrescaler = prescaler;
    HAL_SPI_Init(&hspi1);
}

static uint8_t spi_xchg(uint8_t b)
{
    uint8_t r = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &b, &r, 1, 50);
    return r;
}

static void spi_rx_block(uint8_t *buf, UINT n)
{
    while (n--) *buf++ = spi_xchg(0xFF);
}

/* Espera a que la tarjeta deje de estar ocupada (responde 0xFF) */
static int wait_ready(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();
    do {
        if (spi_xchg(0xFF) == 0xFF) return 1;
    } while (HAL_GetTick() - t0 < ms);
    return 0;
}

static void deselect(void)
{
    SD_CS_HIGH();
    spi_xchg(0xFF);                          /* un ciclo extra para soltar la linea MISO */
}

static int select(void)
{
    SD_CS_LOW();
    spi_xchg(0xFF);
    if (wait_ready(500)) return 1;
    deselect();
    return 0;
}

/* Envia un comando y devuelve la respuesta R1 */
static uint8_t send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t res;
    if (cmd & 0x80) {                        /* ACMDxx = CMD55 + CMDxx */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1) return res;
    }
    if (cmd != CMD12) {
        deselect();
        if (!select()) return 0xFF;
    }
    spi_xchg(0x40 | cmd);
    spi_xchg((uint8_t)(arg >> 24));
    spi_xchg((uint8_t)(arg >> 16));
    spi_xchg((uint8_t)(arg >> 8));
    spi_xchg((uint8_t)arg);
    uint8_t crc = 0x01;
    if (cmd == CMD0) crc = 0x95;             /* CRC valido de CMD0(0)     */
    if (cmd == CMD8) crc = 0x87;             /* CRC valido de CMD8(0x1AA) */
    spi_xchg(crc);
    if (cmd == CMD12) spi_xchg(0xFF);

    uint8_t n = 10;                          /* la respuesta llega en <= 8 bytes */
    do { res = spi_xchg(0xFF); } while ((res & 0x80) && --n);
    return res;
}

static int rx_datablock(uint8_t *buf, UINT n)
{
    uint8_t token;
    uint32_t t0 = HAL_GetTick();
    do { token = spi_xchg(0xFF); } while (token == 0xFF && HAL_GetTick() - t0 < 200);
    if (token != 0xFE) return 0;
    spi_rx_block(buf, n);
    spi_xchg(0xFF); spi_xchg(0xFF);          /* CRC (no se usa) */
    return 1;
}

static int tx_datablock(const uint8_t *buf, uint8_t token)
{
    if (!wait_ready(500)) return 0;
    spi_xchg(token);
    if (token == 0xFD) return 1;             /* token de fin (no se usa) */
    for (UINT i = 0; i < 512; i++) spi_xchg(buf[i]);
    spi_xchg(0xFF); spi_xchg(0xFF);          /* CRC falso */
    return (spi_xchg(0xFF) & 0x1F) == 0x05;  /* dato aceptado */
}

/* ---------------------------------------------------------------------- */
/* Funciones para FatFs                                                    */
/* ---------------------------------------------------------------------- */
DSTATUS SD_disk_initialize(BYTE drv)
{
    uint8_t ocr[4], ty = 0;
    if (drv) return STA_NOINIT;

    /* Pull-up en MISO (PA6) por si el shield no trae la resistencia */
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_6;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &g);

    HAL_Delay(20);                                 /* la tarjeta se estabiliza al encender */
    spi_speed(SPI_BAUDRATEPRESCALER_256);          /* 80 MHz / 256 = 312 kHz (<= 400 kHz) */
    SD_CS_HIGH();
    for (int i = 0; i < 10; i++) spi_xchg(0xFF);   /* 80 pulsos de reloj */

    uint8_t r = 0xFF;
    for (int i = 0; i < 5 && r != 1; i++) r = send_cmd(CMD0, 0);   /* varios intentos */
    sd_init_step = 1;
    if (r == 1) {                                  /* entro en modo SPI */
        uint32_t t0 = HAL_GetTick();
        sd_init_step = 2;
        if (send_cmd(CMD8, 0x1AA) == 1) {          /* SD version 2 */
            spi_rx_block(ocr, 4);
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
                while (HAL_GetTick() - t0 < 2000 && send_cmd(ACMD41, 1UL << 30)) ;
                sd_init_step = 3;
                if (HAL_GetTick() - t0 < 2000 && send_cmd(CMD58, 0) == 0) {
                    spi_rx_block(ocr, 4);
                    ty = (ocr[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
                }
            }
        } else {                                   /* SD version 1 o MMC */
            uint8_t cmd;
            if (send_cmd(ACMD41, 0) <= 1) { ty = CT_SD1; cmd = ACMD41; }
            else                          { ty = CT_MMC; cmd = CMD1;   }
            while (HAL_GetTick() - t0 < 2000 && send_cmd(cmd, 0)) ;
            sd_init_step = 3;
            if (HAL_GetTick() - t0 >= 2000 || send_cmd(CMD16, 512) != 0) ty = 0;
        }
    }
    if (ty) sd_init_step = 0;
    card_type = ty;
    deselect();

    if (ty) {
        spi_speed(SPI_BAUDRATEPRESCALER_8);        /* 80 MHz / 8 = 10 MHz */
        stat &= ~STA_NOINIT;
    } else {
        stat = STA_NOINIT;
    }
    return stat;
}

DSTATUS SD_disk_status(BYTE drv)
{
    return drv ? STA_NOINIT : stat;
}

DRESULT SD_disk_read(BYTE drv, BYTE *buff, DWORD sector, UINT count)
{
    if (drv || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    if (!(card_type & CT_BLOCK)) sector *= 512;    /* tarjetas viejas: direccion en bytes */

    while (count) {
        if (send_cmd(CMD17, sector) != 0 || !rx_datablock(buff, 512)) break;
        buff += 512;
        sector += (card_type & CT_BLOCK) ? 1 : 512;
        count--;
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_write(BYTE drv, const BYTE *buff, DWORD sector, UINT count)
{
    if (drv || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    if (stat & STA_PROTECT) return RES_WRPRT;
    if (!(card_type & CT_BLOCK)) sector *= 512;

    while (count) {
        if (send_cmd(CMD24, sector) != 0 || !tx_datablock(buff, 0xFE)) break;
        buff += 512;
        sector += (card_type & CT_BLOCK) ? 1 : 512;
        count--;
    }
    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_ioctl(BYTE drv, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;
    uint8_t csd[16];
    if (drv) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
    case CTRL_SYNC:
        if (select()) res = RES_OK;
        break;
    case GET_SECTOR_COUNT:                         /* capacidad, leida del registro CSD */
        if (send_cmd(CMD9, 0) == 0 && rx_datablock(csd, 16)) {
            if ((csd[0] >> 6) == 1) {              /* CSD version 2 (SDHC) */
                DWORD csize = csd[9] + ((DWORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
                *(DWORD *)buff = csize << 10;
            } else {                               /* CSD version 1 */
                BYTE n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                DWORD csize = (csd[8] >> 6) + ((DWORD)csd[7] << 2) + ((DWORD)(csd[6] & 3) << 10) + 1;
                *(DWORD *)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512;
        res = RES_OK;
        break;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        res = RES_OK;
        break;
    default:
        res = RES_PARERR;
        break;
    }
    deselect();
    return res;
}

#endif /* SD_FATFS_AVAILABLE */
