#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"

static QueueHandle_t uart_txq;

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
	
	xTaskCreate(uart_task, "task1", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(demo_task, "task1", 100, NULL, configMAX_PRIORITIES - 1, NULL);
	vTaskStartScheduler();

	for (;;);

	return (0);
}