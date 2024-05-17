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

static int8_t
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
	int8_t next;
	char kbuf[256], current;

	uart_puts("\n\ruart_task() has begun:\n\r");

	for (;;) {
		next = uart_getc_nb();
		if (next != -1) {
			uart_puts("\r\n\nENTER INPUT: ");

			current = next;
			if (current != '\r' && current != '\n') {
				kbuf[0] = current;
				uart_putc(current);
				uart_read_keystrokes((kbuf + 1), sizeof(kbuf - 1));
			} else {
				// read the entire line.
				uart_read_keystrokes(kbuf, sizeof(kbuf));
			}

			uart_puts("\r\nReceived input: '");
			uart_puts(kbuf);
			uart_puts("'\n\r");
			doPrintHint = true;
		}

		// Receive a char to be transmitted.
		if (xQueueReceive(uart_txq, &current, 10) == pdPASS)
			uart_putc(current);
	}
}

static inline void
demo_print_string(const char* str)
{
	for ( ; *str; ++str)
		xQueueSend(uart_txq, str, portMAX_DELAY);	// blocks when queue is full
}

static void
task_demo(void* args __attribute__((unused)))
{
	for (;;) {
		
		if (doPrintHint) {
			demo_print_string("demo:/$ > ");
			doPrintHint = false;
		}

		vTaskDelay(pdMS_TO_TICKS(1));
		gpio_toggle(GPIOC, GPIO13);
	}
}

int
main(void)
{
	asm("NOP");
	gpio_setup();
	uart_setup();

	xTaskCreate(task_uart, "UART", 200, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(task_demo, "DEMO", 100, NULL, configMAX_PRIORITIES - 2, NULL);
	
	vTaskStartScheduler();
	
	for (;;);

	return (0);
}