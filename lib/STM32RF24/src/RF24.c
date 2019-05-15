#include "RF24.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef DEBUG
#define PRINTF printf
#else
#define PRINTF(...)
#endif

rf24_t rf24_get_default_config(void) {
    return (rf24_t) {
               .spi_timeout = 1000,
               .payload_size = 32,
               .addr_width = 5,
               .datarate = RF24_1MBPS,
               .channel = 76,

               .regs = {
                   .config = {0x08},
                   .en_aa = {0x3F},
                   .en_rxaddr = {0x03},
                   .setup_aw = {0x03},
                   .setup_retr = {0x03},
                   .rf_ch = {0x02},
                   .rf_setup = {0x0E},
                   .status = {0x0E},
                   .observe_tx = {0x00},
                   .rpd = {0x00},
                   .rx_addr_p0 = {{0xE7, 0xE7, 0xE7, 0xE7, 0xE7}},
                   .rx_addr_p1 = {{0xC2, 0xC2, 0xC2, 0xC2, 0xC2}},
                   .rx_addr_p2 = {0xC3},
                   .rx_addr_p3 = {0xC4},
                   .rx_addr_p4 = {0xC5},
                   .rx_addr_p5 = {0xC6},
                   .tx_addr = {{0xE7, 0xE7, 0xE7, 0xE7, 0xE7}},
                   .rx_pw_p0 = {0x00},
                   .rx_pw_p1 = {0x00},
                   .rx_pw_p2 = {0x00},
                   .rx_pw_p3 = {0x00},
                   .rx_pw_p4 = {0x00},
                   .rx_pw_p5 = {0x00},
                   .fifo_status = {0x11},
                   .dynpd = {0x00},
                   .feature = {0x00},
               },
    };
}

void rf24_enable(rf24_t* rf24) {
    HAL_GPIO_WritePin(rf24->ce_port, rf24->ce_pin, GPIO_PIN_SET);
}

void rf24_disable(rf24_t* rf24) {
    HAL_GPIO_WritePin(rf24->ce_port, rf24->ce_pin, GPIO_PIN_RESET);
}

void rf24_set_channel(rf24_t* rf24, uint8_t ch) {
    ch = ch > 125 ? 125 : ch; // TODO define
    rf24_write_reg8(rf24, NRF24L01_REG_RF_CH, ch);
    rf24->channel = ch;
}

uint8_t rf24_get_channel(rf24_t* rf24) {
    rf24->regs.rf_ch.value = rf24_read_reg8(rf24, NRF24L01_REG_RF_CH);
    return rf24->regs.rf_ch.rf_ch;
}

bool rf24_init(rf24_t* rf24) {
    PRINTF("Initializing RF24\r\n");
    rf24_enable(rf24);
    rf24_end_transaction(rf24);

    HAL_Delay(5);

    rf24_write_reg8(rf24, NRF24L01_REG_CONFIG, 0x0C);

    rf24_set_retries(rf24, 5, 15);

    rf24_set_datarate(rf24, rf24->datarate);

    uint8_t setup = rf24_read_reg8(rf24, NRF24L01_REG_RF_SETUP);

    rf24_set_datarate(rf24, RF24_1MBPS);

    rf24->regs.feature.value = 0x00;
    rf24->regs.dynpd.value = 0x00;
    rf24_write_reg8(rf24, NRF24L01_REG_FEATURE, rf24->regs.feature.value);
    rf24_write_reg8(rf24, NRF24L01_REG_DYNPD, rf24->regs.dynpd.value);

    rf24_set_channel(rf24, rf24->channel);

    rf24_flush_rx(rf24);
    rf24_flush_tx(rf24);

    rf24_power_up(rf24);

    rf24->regs.config.value = rf24_read_reg8(rf24, NRF24L01_REG_CONFIG);
    rf24->regs.config.prim_rx = 0;
    rf24_write_reg8(rf24, NRF24L01_REG_CONFIG, rf24->regs.config.value);

    return setup != 0 && setup != 0xff;
}

void rf24_set_retries(rf24_t* rf24, uint8_t delay, uint8_t count) {
    rf24->regs.setup_retr.ard = delay;
    rf24->regs.setup_retr.arc = count;
    rf24_write_reg8(rf24, NRF24L01_REG_SETUP_RETR, rf24->regs.setup_retr.value);
}

bool rf24_set_datarate(rf24_t* rf24, rf24_datarate_t datarate) {
    rf24->datarate = datarate;
    rf24->regs.rf_setup.value = rf24_read_reg8(rf24, NRF24L01_REG_RF_SETUP);

    // TODO Coisas com txDelay

    switch (datarate) {
        case RF24_1MBPS: {
            rf24->regs.rf_setup.rf_dr_low = 0;
            rf24->regs.rf_setup.rf_dr_high = 0;
            break;
        }

        case RF24_2MBPS: {
            rf24->regs.rf_setup.rf_dr_low = 0;
            rf24->regs.rf_setup.rf_dr_high = 1;
            break;
        }

        case RF24_250KBPS: {
            rf24->regs.rf_setup.rf_dr_low = 1;
            rf24->regs.rf_setup.rf_dr_high = 0;
            break;
        }
    }

    rf24_write_reg8(rf24, NRF24L01_REG_RF_SETUP, rf24->regs.rf_setup.value);

    uint8_t temp_reg;

    if ((temp_reg = rf24_read_reg8(rf24, NRF24L01_REG_RF_SETUP)) == rf24->regs.rf_setup.value) {
        return true;
    }

    rf24->regs.rf_setup.value = temp_reg;
    return false;
}

void rf24_flush_rx(rf24_t* rf24) {
    rf24_send_command(rf24, NRF24L01_COMM_FLUSH_RX);
}

void rf24_flush_tx(rf24_t* rf24) {
    rf24_send_command(rf24, NRF24L01_COMM_FLUSH_TX);
}

void rf24_power_up(rf24_t* rf24) {
    rf24->regs.config.value = rf24_read_reg8(rf24, NRF24L01_REG_CONFIG);

    if (rf24->regs.config.pwr_up == 1) {
        return;
    }

    rf24->regs.config.pwr_up = 1;
    rf24_write_reg8(rf24, NRF24L01_REG_CONFIG, rf24->regs.config.value);
    HAL_Delay(5);
}

void rf24_begin_transaction(rf24_t* rf24) {
    HAL_GPIO_WritePin(rf24->csn_port, rf24->csn_pin, GPIO_PIN_RESET);
}

void rf24_end_transaction(rf24_t* rf24) {
    HAL_GPIO_WritePin(rf24->csn_port, rf24->csn_pin, GPIO_PIN_SET);
}

void rf24_send_command(rf24_t* rf24, nrf24l01_spi_commands_t command) {
    rf24_begin_transaction(rf24);
    HAL_SPI_TransmitReceive(rf24->hspi, &command, &(rf24->regs.status.value), 1, rf24->spi_timeout);
    rf24_end_transaction(rf24);
}

void rf24_read_register(rf24_t* rf24, nrf24l01_registers_t reg, uint8_t* buf, uint8_t len) {
    rf24_begin_transaction(rf24);

    // Transmit command byte
    uint8_t command = NRF24L01_COMM_R_REGISTER | (NRF24L01_COMM_RW_REGISTER_MASK & reg);
    HAL_SPI_TransmitReceive(rf24->hspi, &command, &(rf24->regs.status.value), 1, rf24->spi_timeout);

    HAL_SPI_Receive(rf24->hspi, buf, len, rf24->spi_timeout);

    rf24_end_transaction(rf24);
}

uint8_t rf24_read_reg8(rf24_t* rf24, nrf24l01_registers_t reg) {
    uint8_t val;
    rf24_read_register(rf24, reg, &val, 1);

    return val;
}

void rf24_write_register(rf24_t* rf24, nrf24l01_registers_t reg, uint8_t* buf, uint8_t len) {
    rf24_begin_transaction(rf24);

    uint8_t command = NRF24L01_COMM_W_REGISTER | (NRF24L01_COMM_RW_REGISTER_MASK & reg);
    HAL_SPI_TransmitReceive(rf24->hspi, &command, &(rf24->regs.status.value), 1, rf24->spi_timeout);

    HAL_SPI_Transmit(rf24->hspi, buf, len, rf24->spi_timeout);

    rf24_end_transaction(rf24);
}

void rf24_write_reg8(rf24_t* rf24, nrf24l01_registers_t reg, uint8_t value) {
    rf24_write_register(rf24, reg, &value, 1);
}

void rf24_read_all_registers(rf24_t* rf24) {
    rf24->regs.config.value = rf24_read_reg8(rf24, NRF24L01_REG_CONFIG);
    rf24->regs.en_aa.value = rf24_read_reg8(rf24, NRF24L01_REG_EN_AA);
    rf24->regs.en_rxaddr.value = rf24_read_reg8(rf24, NRF24L01_REG_EN_RXADDR);
    rf24->regs.setup_aw.value = rf24_read_reg8(rf24, NRF24L01_REG_SETUP_AW);
    rf24->regs.setup_retr.value = rf24_read_reg8(rf24, NRF24L01_REG_SETUP_RETR);
    rf24->regs.rf_ch.value = rf24_read_reg8(rf24, NRF24L01_REG_RF_CH);
    rf24->regs.rf_setup.value = rf24_read_reg8(rf24, NRF24L01_REG_RF_SETUP);
    rf24->regs.status.value = rf24_read_reg8(rf24, NRF24L01_REG_STATUS);
    rf24->regs.observe_tx.value = rf24_read_reg8(rf24, NRF24L01_REG_OBSERVE_TX);
    rf24->regs.rpd.value = rf24_read_reg8(rf24, NRF24L01_REG_RPD);

    rf24_read_register(rf24, NRF24L01_REG_RX_ADDR_P0, &(rf24->regs.rx_addr_p0.value), rf24->addr_width);
    rf24_read_register(rf24, NRF24L01_REG_RX_ADDR_P1, &(rf24->regs.rx_addr_p1.value), rf24->addr_width);
    rf24_read_register(rf24, NRF24L01_REG_TX_ADDR, &(rf24->regs.tx_addr.value), rf24->addr_width);

    rf24->regs.rx_addr_p2.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_ADDR_P2);
    rf24->regs.rx_addr_p3.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_ADDR_P3);
    rf24->regs.rx_addr_p4.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_ADDR_P4);
    rf24->regs.rx_addr_p5.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_ADDR_P5);
    rf24->regs.rx_pw_p0.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P0);
    rf24->regs.rx_pw_p1.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P1);
    rf24->regs.rx_pw_p2.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P2);
    rf24->regs.rx_pw_p3.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P3);
    rf24->regs.rx_pw_p4.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P4);
    rf24->regs.rx_pw_p5.value = rf24_read_reg8(rf24, NRF24L01_REG_RX_PW_P5);
    rf24->regs.fifo_status.value = rf24_read_reg8(rf24, NRF24L01_REG_FIFO_STATUS);
    rf24->regs.dynpd.value = rf24_read_reg8(rf24, NRF24L01_REG_DYNPD);
    rf24->regs.feature.value = rf24_read_reg8(rf24, NRF24L01_REG_FEATURE);
}

#ifdef DEBUG
void rf24_dump_registers(rf24_t* rf24) {
    PRINTF("=== REGISTER DUMP\r\n");
    PRINTF(
        "[00] CONFIG     = 0x%02X | MASK_RX_DR=%d  MASK_TX_DS=%d  MASK_MAX_RT=%d EN_CRC=%d      CRCO=%d        PWR_UP=%d      PRIM_RX=%d\r\n",
        rf24->regs.config.value, rf24->regs.config.mask_rx_dr, rf24->regs.config.mask_tx_ds, rf24->regs.config.mask_max_rt,
        rf24->regs.config.en_crc, rf24->regs.config.crco, rf24->regs.config.pwr_up, rf24->regs.config.prim_rx);
    PRINTF(
        "[01] EN_AA      = 0x%02X | ENAA_P5=%d     ENAA_P4=%d     ENAA_P3=%d     ENAA_P2=%d     ENAA_P1=%d     ENAA_P0=%d\r\n",
        rf24->regs.en_aa.value, rf24->regs.en_aa.enaa_p5, rf24->regs.en_aa.enaa_p4, rf24->regs.en_aa.enaa_p3,
        rf24->regs.en_aa.enaa_p2, rf24->regs.en_aa.enaa_p1, rf24->regs.en_aa.enaa_p0);
    PRINTF(
        "[02] EN_RXADDR  = 0x%02X | ERX_P5=%d      ERX_P4=%d      ERX_P3=%d      ERX_P2=%d      ERX_P1=%d      ERX_P0=%d\r\n",
        rf24->regs.en_rxaddr.value, rf24->regs.en_rxaddr.erx_p5, rf24->regs.en_rxaddr.erx_p4, rf24->regs.en_rxaddr.erx_p3,
        rf24->regs.en_rxaddr.erx_p2, rf24->regs.en_rxaddr.erx_p1, rf24->regs.en_rxaddr.erx_p0);
    PRINTF("[03] SETUP_AW   = 0x%02X | AW=%d (%s)\r\n", rf24->regs.setup_aw.value, rf24->regs.setup_aw.aw,
           (rf24->regs.setup_aw.aw == 0b00) ? "Illegal" :
           (rf24->regs.setup_aw.aw == 0b01) ? "3 bytes" :
           (rf24->regs.setup_aw.aw == 0b10) ? "4 bytes" :
           (rf24->regs.setup_aw.aw == 0b11) ? "5 bytes" : "???????");
    PRINTF("[04] SETUP_RETR = 0x%02X | ARD=%d (%d us)  ARC=%d (retransmits)\r\n", rf24->regs.setup_retr.value,
           rf24->regs.setup_retr.ard, 250 * (1 + rf24->regs.setup_retr.ard), rf24->regs.setup_retr.arc);
    PRINTF("[05] RF_CH      = 0x%02X | RF_CH=%d (%d MHz)\r\n", rf24->regs.rf_ch.value,
           rf24->regs.rf_ch.rf_ch, rf24->regs.rf_ch.rf_ch + 2400);
    PRINTF("[06] RF_SETUP   = 0x%02X | CONT_WAVE=%d   RF_DR=%d%d (%s) PLL_LOCK=%d  RF_PWR=%d (%sdBm)\r\n",
           rf24->regs.rf_setup.value, rf24->regs.rf_setup.cont_wave, rf24->regs.rf_setup.rf_dr_low, rf24->regs.rf_setup.rf_dr_high,
           (rf24->regs.rf_setup.rf_dr_low == 0 && rf24->regs.rf_setup.rf_dr_high == 0) ? "1Mbps" :
           (rf24->regs.rf_setup.rf_dr_low == 0 && rf24->regs.rf_setup.rf_dr_high == 1) ? "2Mbps" :
           (rf24->regs.rf_setup.rf_dr_low == 1 && rf24->regs.rf_setup.rf_dr_high == 0) ? "250kbps" : "?????",
           rf24->regs.rf_setup.pll_lock, rf24->regs.rf_setup.rf_pwr,
           (rf24->regs.rf_setup.rf_pwr == 0b00) ? "-18" :
           (rf24->regs.rf_setup.rf_pwr == 0b01) ? "-12" :
           (rf24->regs.rf_setup.rf_pwr == 0b10) ? " -6" :
           (rf24->regs.rf_setup.rf_pwr == 0b11) ? "  0" : "???????");
    rf24_print_status(rf24);
    PRINTF("[08] OBSERVE_TX = 0x%02X | PLOS_CNT=%d    ARC_CNT=%d\r\n", rf24->regs.observe_tx.value,
           rf24->regs.observe_tx.plos_cnt, rf24->regs.observe_tx.arc_cnt);
    PRINTF("[09] RPD        = 0x%02X | RPD=%d\r\n", rf24->regs.rpd.value, rf24->regs.rpd.rpd);
    PRINTF("[0A] RX_ADDR_P0 = [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]\r\n", rf24->regs.rx_addr_p0.value[0], rf24->regs.rx_addr_p0.value[1],
           rf24->regs.rx_addr_p0.value[2], rf24->regs.rx_addr_p0.value[3], rf24->regs.rx_addr_p0.value[4]);
    PRINTF("[0B] RX_ADDR_P1 = [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]\r\n", rf24->regs.rx_addr_p1.value[0], rf24->regs.rx_addr_p1.value[1],
           rf24->regs.rx_addr_p1.value[2], rf24->regs.rx_addr_p1.value[3], rf24->regs.rx_addr_p1.value[4]);
    PRINTF("[0C] RX_ADDR_P2 = 0x%02X\r\n", rf24->regs.rx_addr_p2.value);
    PRINTF("[0D] RX_ADDR_P3 = 0x%02X\r\n", rf24->regs.rx_addr_p3.value);
    PRINTF("[0E] RX_ADDR_P4 = 0x%02X\r\n", rf24->regs.rx_addr_p4.value);
    PRINTF("[0F] RX_ADDR_P5 = 0x%02X\r\n", rf24->regs.rx_addr_p5.value);
}

void rf24_print_status(rf24_t* rf24) {
    PRINTF("[07] STATUS     = 0x%02X | RX_DR=%d  TX_DS=%d  MAX_RT=%d  RX_P_NO=%d  TX_FULL=%d", rf24->regs.status.value,
           rf24->regs.status.rx_dr, rf24->regs.status.tx_ds, rf24->regs.status.max_rt, rf24->regs.status.rx_p_no,
           rf24->regs.status.tx_full);
}

#endif
