/**
 * @file main.c
 *
 * @brief Main function
 */

#include "mcu.h"
#include "RF24.h"
#include "spi.h"
#include "usart.h"

/*****************************************
 * Private Constant Definitions
 *****************************************/

/*****************************************
 * Main Function
 *****************************************/

int main(void) {
    mcu_init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();

    rf24_t rf24 = rf24_get_default_config();
    rf24.hspi = &hspi1,
    rf24.csn_port = GPIOC,
    rf24.csn_pin = GPIO_PIN_7,

    rf24.ce_port = GPIOA,
    rf24.ce_pin = GPIO_PIN_9,

    rf24.irq_port = GPIOB,
    rf24.irq_pin = GPIO_PIN_6,

    HAL_Delay(10);

    printf("================ NRF24 TEST ================\r\n");
    printf("Calling init [....]");

    if (rf24_init(&rf24)) {
        printf("\b\b\b\b\b\b[ OK ]\r\n");
    } else {
        printf("\b\b\b\b\b\b[FAIL]\r\n");
    }

    // .regs = {
    // .config = {0x08},
    // .en_aa = {0x3F},
    // .en_rxaddr = {0x03},
    // .setup_aw = {0x03},
    // .setup_retr = {0x03},
    // .rf_ch = {0x02},
    // .rf_setup = {0x0E},
    // .status = {0x0E},
    // .observe_tx = {0x00},
    // .rpd = {0x00},
    // .rx_addr_p0 = {{0xE7, 0xE7, 0xE7, 0xE7, 0xE7}},
    // .rx_addr_p1 = {{0xC2, 0xC2, 0xC2, 0xC2, 0xC2}},
    // .rx_addr_p2 = {0xC3},
    // .rx_addr_p3 = {0xC4},
    // .rx_addr_p4 = {0xC5},
    // .rx_addr_p5 = {0xC6},
    // .tx_addr = {{0xE7, 0xE7, 0xE7, 0xE7, 0xE7}},
    // .rx_pw_p0 = {0x00},
    // .rx_pw_p1 = {0x00},
    // .rx_pw_p2 = {0x00},
    // .rx_pw_p3 = {0x00},
    // .rx_pw_p4 = {0x00},
    // .rx_pw_p5 = {0x00},
    // .fifo_status = {0x11},
    // .dynpd = {0x00},
    // .feature = {0x00},

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    printf("Registers after init\r\n");
    printf("-- CONFIG      %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_CONFIG));
    printf("-- EN_AA       %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_EN_AA));
    printf("-- EN_RXADDR   %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_EN_RXADDR));
    printf("-- SETUP_AW    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_SETUP_AW));
    printf("-- SETUP_RETR  %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_SETUP_RETR));
    printf("-- RF_CH       %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RF_CH));
    printf("-- RF_SETUP    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RF_SETUP));
    printf("-- STATUS      %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_STATUS));
    printf("-- OBSERVE_TX  %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_OBSERVE_TX));
    printf("-- RPD         %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RPD));
    printf("-- RX_PW_P0    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P0));
    printf("-- RX_PW_P1    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P1));
    printf("-- RX_PW_P2    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P2));
    printf("-- RX_PW_P3    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P3));
    printf("-- RX_PW_P4    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P4));
    printf("-- RX_PW_P5    %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_RX_PW_P5));
    printf("-- FEATURE     %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_FEATURE));
    printf("\r\n");

    for (;;) {
        printf("-- STATUS      %02X\r\n", rf24_read_reg8(&rf24, NRF24L01_REG_STATUS));

        HAL_Delay(5000);
    }
}

#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

int _write(int file, char* data, int len) {
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)) {
        errno = EBADF;
        return -1;
    }

    // arbitrary timeout 1000
    HAL_StatusTypeDef status =
        HAL_UART_Transmit(&huart2, (uint8_t*) data, len, 1000);

    // return # of bytes written - as best we can tell
    return status == HAL_OK ? len : 0;
}
