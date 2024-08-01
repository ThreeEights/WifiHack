/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>

#include "wifi-hack.h"

const struct device *const wifi_bus = DEVICE_DT_GET(DT_NODELABEL(pio0_spi0));

#define WIFI_NODE DT_NODELABEL(airoc_wifi)
#define WIFI_ON_GPIO DT_GPIO_LABEL(WIFI_NODE, wifi_reg_on_gpios)
#define WIFI_ON_PIN DT_GPIO_PIN(WIFI_NODE, wifi_reg_on_gpios)
#define DO_PRINT(TEXT) printf("WIFI_NODE=" #TEXT "\n")

static const struct gpio_dt_spec wifi_on = GPIO_DT_SPEC_GET(WIFI_NODE, wifi_reg_on_gpios);

#define AIROC_WIFI_SPI_OPERATION (SPI_WORD_SET(8) | SPI_HALF_DUPLEX | SPI_TRANSFER_MSB)
#define SPI_NODE DT_NODELABEL(airoc_wifi)

static const struct spi_dt_spec bus_spi = SPI_DT_SPEC_GET(SPI_NODE, AIROC_WIFI_SPI_OPERATION, 0);

void dump_regs() {
    uint32_t gpio_out = *((uint32_t *)(0xD0000010U));
    uint32_t gpio_oe = *((uint32_t *)(0xD0000020U));
    uint32_t pin23_status = *((uint32_t *)(0X400140B8u));
    uint32_t pin23_ctrl = *((uint32_t *)(0X400140BCu));
    uint32_t pin24_status = *((uint32_t *)(0X400140C0u));
    uint32_t pin24_ctrl = *((uint32_t *)(0X400140C4u));
    uint32_t pin25_status = *((uint32_t *)(0X400140C8u));
    uint32_t pin25_ctrl = *((uint32_t *)(0X400140CCu));
    uint32_t pin29_status = *((uint32_t *)(0X400140E8u));
    uint32_t pin29_ctrl = *((uint32_t *)(0X400140ECu));

    printf("GPIO OUT=0x%08X, OE=0x%08X\n", gpio_out, gpio_oe);
    printf("Pin 23 status=0x%08X, ctrl=0x%08X\n", pin23_status, pin23_ctrl);
    printf("Pin 24 status=0x%08X, ctrl=0x%08X\n", pin24_status, pin24_ctrl);
    printf("Pin 25 status=0x%08X, ctrl=0x%08X\n", pin25_status, pin25_ctrl);
    printf("Pin 29 status=0x%08X, ctrl=0x%08X\n", pin29_status, pin29_ctrl);
}

int main(void)
{
	const struct spi_pico_pio_config *dev_cfg = bus_spi.bus->config;

    char tx_data[] = { 0xA0, 0x04, 0x40, 0x00 };
    char rx_data[] = { 0x00, 0x00, 0x00, 0x00 };
	const struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = 4
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = 4
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};
    int ret = 0;

    ret = gpio_pin_configure_dt(&wifi_on, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
            return ret;
    }

    printf("checkWiFi() called\n");

    /* Pull data line low before enabling WiFi chip */
    printf("  Set data line low - pin %d\n", dev_cfg->sio_gpio.pin);
    gpio_pin_set_dt(&dev_cfg->sio_gpio, 0);

	DO_PRINT(WIFI_NODE);
	printf("On pin=%d\n", WIFI_ON_PIN);

    printf("Sleep 150 msec\n");
    k_msleep(150);

    printf("Turn on WiFi chip\n");
    gpio_pin_set_dt(&wifi_on, 1);

    for (int count = 0; count < 4; count++) {
        printf("Add a delay of 50 msec\n");
        k_msleep(250);

//        dump_regs();

        printf("Request chip ID\n");
        ret = spi_transceive_dt(&bus_spi, &tx, &rx);

        printf("Chip ID=");
        for (int index = 0; index < 4; index++) {
            printf("0x%02X ", rx_data[index]);
        }
        printf("\n");
    }

    printf("Turn WiFi off\n");
    ret = gpio_pin_set_dt(&wifi_on, 0);

	return ret;
}
