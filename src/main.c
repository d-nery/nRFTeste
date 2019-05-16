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

    rf24_dump_registers(&rf24);

    for (;;) {
        rf24_print_status(&rf24);

        HAL_Delay(5000);
    }
}
