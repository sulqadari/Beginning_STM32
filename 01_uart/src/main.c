#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"
#include <libopencm3/cm3/nvic.h>

#define USART_BUF_DEPTH 32

typedef struct {
	volatile uint16_t head;
	volatile uint16_t tail;
	uint8_t buf[USART_BUF_DEPTH];
} uart_t;

typedef struct {
	uint32_t usart;
	uint32_t rcc;
	uint32_t irq;
	int32_t (*getc)(void);
	void (*putc)(char ch);
} uart_info_t;

static uart_info_t uarts[3] = {
	{ USART1, RCC_USART1, NVIC_USART1_IRQ, uart1_getc, uart1_putc },
	{ USART2, RCC_USART2, NVIC_USART2_IRQ, uart2_getc, uart2_putc },
	{ USART3, RCC_USART3, NVIC_USART3_IRQ, uart3_getc, uart3_putc }
};

static uart_t* uart_data[3] = {0, 0, 0};

static QueueHandle_t uart_txq;

static int32_t
get_char(uart_t* uart)
{
	char rch;

	if (uart->head == uart->tail)
		return (-1);
	
	rch = uart->buf[uart->head];
	uart->head = (uart->head + 1) % USART_BUF_DEPTH;

	return rch;
}

static char
getc_uart(uint32_t uartno)
{
	uart_t* uart = uart_data[uartno - 1];
	int32_t rch;

	if (!uart)
		return (-1);
	
	while ( (rch = get_char(uart)) == -1 )
		taskYIELD();
	
	return (char)rch;
}

static int32_t
uart1_getc(void)
{
	return getc_uart(1);
}

static int32_t
uart2_getc(void)
{
	return getc_uart(2);
}

static int32_t
uart3_getc(void)
{
	return getc_uart(3);
}


static void
putc_uart(uint32_t uartno, char ch)
{
	uint32_t uart = uarts[uartno - 1].usart;

	while ((USART_SR(uart) & USART_SR_TXE) == 0)
		taskYIELD();
	
	usart_send_blocking(uart, ch);
}

static void
uart1_putc(char ch)
{
	if (ch == '\n')
		putc_uart(1, '\r');
	
	putc_uart(1, ch);
}

static void
uart2_putc(char ch)
{
	if (ch == '\n')
		putc_uart(2, '\r');
	
	putc_uart(2, ch);
}

static void
uart3_putc(char ch)
{
	if (ch == '\n')
		putc_uart(3, '\r');
	
	putc_uart(3, ch);
}

/*

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

	usart_set_baudrate(USART1, 38400);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);

	uart_txq = xQueueCreate(256, sizeof(char));
}

static void
uart_task(void* args __attribute__((unused)))
{
	char ch;

	for (;;) {
		// Receive a char to be transmitted
		if (xQueueReceive(uart_txq, &ch, 500) == pdPASS) {
			
			while (!usart_get_flag(USART1, USART_SR_TXE))
				taskYIELD();
			
			usart_send(USART1, ch);
		}

		gpio_toggle(GPIOC, GPIO13);
	}
}

static void
uart_puts(const char* str)
{
	for ( ; *str; ++str) {
		xQueueSend(uart_txq, str, portMAX_DELAY);
	}
}

static void
demo_task(void* args __attribute__((unused)))
{
	for (;;) {
		uart_puts("Now this is a message..\n\r");
		uart_puts("  sent via FreeRTOS queues.\n\n\r");
		vTaskDelay(pdMS_TO_TICKS(1000));
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
	
	xTaskCreate(uart_task, "uart_task", 32, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(demo_task, "demo_task", 32, NULL, configMAX_PRIORITIES - 1, NULL);
	vTaskStartScheduler();

	for (;;);

	return (0);
}
*/

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
open_uart(uint32_t uartno, uint32_t baud,
			const char* cfg, const char* mode,
			uint32_t rts, uint32_t cts)
{
	uint32_t uart, ux, stopb, iomode, parity, fc;
	uart_info_t* info;
	bool rxintf = false;

	if (uartno < 1 || uartno > 3)
		return (-1);
	
	info = &uarts[ux = uartno - 1];
	uart = info->usart;
	usart_disable_rx_interrupt(uart);
}

static void
uart_setup(void)
{
	open_uart(1, 115200, "8N1", "rw", 1, 1);
	uart_txq = xQueueCreate(256, sizeof(char));
}

static void
uart_task(void* args __attribute__((unused)))
{

}

static void
demo_task(void* args __attribute__((unused)))
{

}

int
main(void)
{
	gpio_setup();
	uart_setup();

	xTaskCreate(uart_task, "UART", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(demo_task, "UART", 100, NULL, configMAX_PRIORITIES - 2, NULL);
	
	vTaskStartScheduler();
	
	return (0);
}