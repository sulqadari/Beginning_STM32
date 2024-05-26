#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/common/usart_common_all.h>

static QueueHandle_t queue_TX, queue_RX;

static bool inline
isLower(int32_t ch)
{
	return (ch >= 'a' && ch <= 'z');
}

static bool inline
isUpper(int32_t ch)
{
	return (ch >= 'A' && ch <= 'Z');
}

static bool inline
toLower(int32_t ch)
{
	return ch + 32;
}

static bool inline
toUpper(int32_t ch)
{
	return ch - 32;
}

static void
init_clock(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_USART1);
}

static void
init_LED(void)
{
	gpio_set_mode(
		GPIOC,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO13
	);

	gpio_set(GPIOC, GPIO13);
}

static void
init_uart(void)
{
	queue_TX = xQueueCreate(256, sizeof(char));
	queue_RX = xQueueCreate(256, sizeof(char));

	nvic_enable_irq(NVIC_USART1_IRQ);
	
	gpio_set_mode(
		GPIOA,
		GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		GPIO_USART1_TX
	);

	gpio_set_mode(
		GPIOA,
		GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT,
		GPIO_USART1_RX
	);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);

	usart_enable_rx_interrupt(USART1);
	usart_enable(USART1);
}

void
USART1_IRQHandler(void)
{
	int32_t ch;
	BaseType_t hpTask = pdFALSE;

	while (((USART_SR(USART1) & USART_SR_RXNE) != 0)) {
		ch = usart_recv(USART1);

		xQueueSendToBackFromISR(queue_RX, &ch, &hpTask);
	}
}

static void
task_transmit(void* args __attribute((unused)))
{
	int32_t ch;

	for (;;) {
		while (xQueueReceive(queue_TX, &ch, portMAX_DELAY) != pdPASS)
			taskYIELD();
		
		while (!usart_get_flag(USART1, USART_SR_TXE))
			taskYIELD();

		usart_send(USART1, ch);
	}
}

static int32_t
read_char(void)
{
	int32_t ch;

	while (xQueueReceive(queue_RX, &ch, 0) != pdPASS)
		taskYIELD();
	
	gpio_toggle(GPIOC, GPIO13);
	return ch;
}

static void
write_char(int32_t ch)
{
_again:
	while (xQueueSend(queue_TX, &ch, 0) != pdPASS)
		taskYIELD();
	
	if (ch == '\n') {
		ch = '\r';
		goto _again;
	}
}

static void
task_main(void* args __attribute((unused)))
{
	for (;;) {
		int32_t ch = read_char();

		if (isLower(ch))
			ch = toUpper(ch);
		else if (isUpper(ch))
			ch = toLower(ch);
		
		write_char(ch);
	}
}

int
main(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	init_clock();
	init_LED();
	init_uart();

	xTaskCreate(task_main, "MAIN", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(task_transmit, "TRANSMIT", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	
	vTaskStartScheduler();
	
	for (;;);

	return (0);
}