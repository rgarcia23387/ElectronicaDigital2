/*
 * PN532.c
 *
 * Created: 10/08/2026 02:20:13
 *  Author: dinog
 */ 

/*
 * PN532.c
 *
 * Created: 06/08/2026
 * Author: Rodrigo García
 * Description: Implementación del protocolo de tramas del PN532 sobre
 *              SPI. Si al probarlo los datos no coinciden con lo
 *              esperado, revisar con UART_print_hex() los bytes crudos
 *              recibidos -- la temporización del SPI del PN532 es
 *              sensible y puede requerir ajuste fino.
 */

#include "PN532.h"

static void PN532_cs_low(void)
{
    SPI_PORT &= ~(1 << SPI_SS);
    _delay_us(10);
}

static void PN532_cs_high(void)
{
    _delay_us(10);
    SPI_PORT |= (1 << SPI_SS);
}

// Espera a que el PN532 indique que tiene una respuesta lista (bit0 = 1)
static uint8_t PN532_wait_ready(uint16_t timeout_ms)
{
    uint16_t t = 0;
    uint8_t status;

    while (t < timeout_ms)
    {
        PN532_cs_low();
        SPI_transfer(0x02); // comando: leer estado
        status = SPI_transfer(0x00);
        PN532_cs_high();

        if (status & 0x01) return 1;

        _delay_ms(1);
        t++;
    }
    return 0; // timeout
}

// Envia una trama de comando. data[0] debe ser 0xD4 (host->PN532)
static void PN532_write_frame(const uint8_t *data, uint8_t len)
{
    uint8_t checksum = 0;
    uint8_t i;

    PN532_cs_low();
    SPI_transfer(0x01); // comando: escribir datos

    SPI_transfer(0x00); // preambulo
    SPI_transfer(0x00); // start code
    SPI_transfer(0xFF);
    SPI_transfer(len);
    SPI_transfer((uint8_t)(~len + 1)); // LCS

    for (i = 0; i < len; i++)
    {
        SPI_transfer(data[i]);
        checksum += data[i];
    }
    SPI_transfer((uint8_t)(~checksum + 1)); // DCS
    SPI_transfer(0x00); // postambulo

    PN532_cs_high();
}

// Lee y descarta la trama ACK de 6 bytes (00 00 FF 00 FF 00)
static void PN532_read_ack(void)
{
    uint8_t i;
    PN532_cs_low();
    SPI_transfer(0x03); // comando: leer datos
    for (i = 0; i < 6; i++)
        SPI_transfer(0x00);
    PN532_cs_high();
}

// Lee la trama de respuesta. Devuelve la cantidad de bytes utiles
// (sin contar el TFI) copiados a buf.
static uint8_t PN532_read_response(uint8_t *buf, uint8_t maxlen)
{
    uint8_t len;
    uint8_t i;

    PN532_cs_low();
    SPI_transfer(0x03); // comando: leer datos

    SPI_transfer(0x00); // preambulo (descartar)
    SPI_transfer(0x00); // start code byte 1 (descartar)
    SPI_transfer(0x00); // start code byte 2 -- deberia ser 0xFF, se descarta igual
    len = SPI_transfer(0x00);   // LEN
    SPI_transfer(0x00);         // LCS (descartar)
    SPI_transfer(0x00);         // TFI (descartar, siempre 0xD5 en respuestas)

    if (len == 0)
    {
        PN532_cs_high();
        return 0;
    }

    for (i = 0; i < (len - 1) && i < maxlen; i++)
        buf[i] = SPI_transfer(0x00);

    SPI_transfer(0x00); // DCS (descartar)
    SPI_transfer(0x00); // postambulo (descartar)

    PN532_cs_high();
    return (len > 0) ? (len - 1) : 0;
}

void PN532_init(void)
{
    SPI_init();

    // Pulso de reset (activo en bajo)
    PN532_RESET_DDR |= (1 << PN532_RESET_PIN);
    PN532_RESET_PORT &= ~(1 << PN532_RESET_PIN);
    _delay_ms(100);
    PN532_RESET_PORT |= (1 << PN532_RESET_PIN);
    _delay_ms(200);
}

uint8_t PN532_sam_config(void)
{
    uint8_t cmd[5] = {0xD4, 0x14, 0x01, 0x14, 0x01};
    uint8_t response[8];

    PN532_write_frame(cmd, 5);
    _delay_ms(2);
    if (!PN532_wait_ready(1000)) return 0;
    PN532_read_ack();

    if (!PN532_wait_ready(1000)) return 0;
    PN532_read_response(response, 8);
    return 1;
}

uint8_t PN532_leer_uid(uint8_t *uid, uint8_t *uid_len, uint16_t timeout_ms)
{
    uint8_t cmd[4] = {0xD4, 0x4A, 0x01, 0x00}; // InListPassiveTarget, 1 tarjeta, 106kbps tipo A
    uint8_t response[20];
    uint8_t len;

    PN532_write_frame(cmd, 4);
    _delay_ms(2);
    if (!PN532_wait_ready(50)) return 0; // el ACK deberia llegar casi de inmediato
    PN532_read_ack();

    // La respuesta tarda mas si esta esperando detectar una tarjeta
    if (!PN532_wait_ready(timeout_ms)) return 0;
    len = PN532_read_response(response, sizeof(response));

    // response[0] = CommandCode+1 (0x4B), response[1] = NbTg (num. tarjetas)
    // response[2] = Tg, response[3..4] = SENS_RES, response[5] = SEL_RES,
    // response[6] = NFCIDLength, response[7..] = NFCID (el UID)
    if (len < 7 || response[1] == 0) return 0;

    *uid_len = response[6];
    if (*uid_len > 7) *uid_len = 7;

    for (uint8_t i = 0; i < *uid_len; i++)
        uid[i] = response[7 + i];

    return 1;
}