

struct spi_pico_pio_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pin_cfg;
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec mosi_gpio;
	struct gpio_dt_spec miso_gpio;
	struct gpio_dt_spec sio_gpio;
};
