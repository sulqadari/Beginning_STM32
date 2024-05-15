#include "usart_utils.h"

static QueueHandle_t uart_txq;

static void
gpio_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOC);
	// Setup LED
	gpio_set_mode(
		GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, GPIO13
	);

	rcc_periph_clock_enable(RCC_GPIOA);
	// Setup USART1 TX
	gpio_set_mode(
		GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX
	);

	// set USART1 CTS as output in alternate push-pull mode with speed 50 MHz
	gpio_set_mode(
		GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO11
	);

	// Setup USART RX
	gpio_set_mode(
		GPIOA, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX
	);

	// set USART1 RTS as input in floating mode
	gpio_set_mode(
		GPIOA, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT, GPIO12
	);

}

static int8_t
uart_setup(void)
{
	if (open_uart(1, 115200, "8N1", "rw", 1, 1) != 0)
		return (-1);
	
	uart_txq = xQueueCreate(256, sizeof(char));
	return (0);
}

static void
uart_task(void* args __attribute__((unused)))
{
	int32_t gc;
	char kbuf[256], ch;

	puts_uart(1, "\n\ruart_task() has begun:\n\r");

	for (;;) {
		if ((gc = getc_uart_nb(1)) != -1) {
			puts_uart(1, "\r\nENTER INPUT: ");

			ch = (char)gc;
			if (ch != '\r' && ch != '\n') {
				kbuf[0] = ch;
				putc_uart(1, ch);
				getline_uart(1, (kbuf + 1), sizeof (kbuf - 1));
			} else {
				// read the entire line.
				getline_uart(1, kbuf, sizeof(kbuf));
			}
		}

		// Receive a char to be transmitted.
		if (xQueueReceive(uart_txq, &ch, 10) == pdPASS)
			putc_uart(1, ch);
		gpio_toggle(GPIOC, GPIO13);
	}
}

static inline void
uart_puts(const char* str)
{
	for ( ; *str; ++str)
		xQueueSend(uart_txq, str, portMAX_DELAY);	// blocks when queue is full
}

static void
demo_task(void* args __attribute__((unused)))
{
	for (;;) {
		uart_puts("Just start typing to enter a line, or..\n\r"
			"hit Enter first, then enter your input.\n\n\r");

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int
main(void)
{
	asm("NOP");
	gpio_setup();
	uart_setup();

	xTaskCreate(uart_task, "UART", 200, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(demo_task, "DEMO", 100, NULL, configMAX_PRIORITIES - 2, NULL);
	
	vTaskStartScheduler();
	
	for (;;);

	return (0);
}