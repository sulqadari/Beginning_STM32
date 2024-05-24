#include "usart_utils.h"

static QueueHandle_t uart_txq;
bool doPrintHint = true;

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

static int32_t
uart_setup(void)
{
	if (uart_open(115200, "8N1", "rw", 1, 1) != 0)
		return (-1);
	
	uart_txq = xQueueCreate(256, sizeof(char));
	return (0);
}

static void
task_uart(void* args __attribute__((unused)))
{
	int32_t fetched;
	char kbuf[256], ch;

	uart_puts("\r\n***FunTerm***\r\n");
	for (;;) {

		if ((fetched = uart_getc_nb()) != -1) {
			
			uart_puts(">>>: ");
			ch = (char)fetched;

			if (ch != '\r' && ch != '\n') {
				kbuf[0] = ch;
				uart_putc(ch);
				uart_read_keystrokes((kbuf + 1), sizeof kbuf - 1);
			} else {
				// read the entire line.
				uart_read_keystrokes(kbuf, sizeof kbuf);
			}

			uart_puts("\n\r<<<'");
			uart_puts(kbuf);
			uart_puts("'\n\r");
		}

		// Receive a char to be transmitted.
		if (xQueueReceive(uart_txq, &ch, 10) == pdPASS) {
			uart_putc(ch);
			gpio_toggle(GPIOC, GPIO13);
		}
	}
}

static inline void
demo_print_string(const char* str)
{
	for ( ; *str; ++str)
		xQueueSend(uart_txq, str, portMAX_DELAY);	// blocks when queue is full
}

static void
task_blink(void* args __attribute__((unused)))
{
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(2000));
		gpio_toggle(GPIOC, GPIO13);
	}
}

int
main(void)
{
	gpio_setup();
	uart_setup();

	xTaskCreate(task_uart, "UART_RTX", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(task_blink, "Blink", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	
	vTaskStartScheduler();
	
	for (;;);

	return (0);
}