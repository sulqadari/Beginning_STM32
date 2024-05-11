#include "FreeRTOS.h"
#include "task.h"

#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"

static void
uart_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);

	gpio_set_mode(
		GPIOA,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO_USART1_TX
	);

	usart_set_baudrate(USART1, 9600);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
}

static inline void
uart_putc(char ch)
{
	usart_send_blocking(USART1, ch);
}

static void
task1(void* args __attribute__((unused)))
{
	int c = '0' - 1;

	for (;;) {
		gpio_toggle(GPIOC, GPIO13);
		vTaskDelay(pdMS_TO_TICKS(2500));

		if (++c >= 'Z') {
			uart_putc(c);
			uart_putc('\r');
			uart_putc('\n');
			c = '0' - 1;
		} else {
			uart_putc(c);
		}
	}
}

int
main(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	rcc_periph_clock_enable(RCC_GPIOC);
	
	gpio_set_mode(
		GPIOC,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO13
	);

	uart_setup();
	
	xTaskCreate(task1, "task1", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	vTaskStartScheduler();

	for (;;);

	return (0);
}