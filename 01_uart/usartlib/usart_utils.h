#ifndef USART_UTILS_H
#define USART_UTILS_H

#include <stdint.h>
#include <memory.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/usart.h"
#include "libopencm3/cm3/nvic.h"

#define USART_BUF_DEPTH 32

typedef struct {
	volatile uint16_t head;
	volatile uint16_t tail;
	uint8_t buf[USART_BUF_DEPTH];
} uart_data_t;

typedef struct {
	uint32_t usart;
	uint32_t rcc;
	uint32_t irq;
	int32_t (*getc)(void);
	void (*putc)(char ch);
} uart_handler_t;

extern uart_handler_t uart_hlr[3];
extern uart_data_t* uart_data[3];

int32_t getc_uart_nb(uint32_t uartno);

int8_t open_uart(uint32_t uartno, uint32_t baud, const char* cfg,
				const char* mode, uint32_t rts, uint32_t cts);

int32_t getline_uart(uint32_t uartno, char* buf, uint32_t bufsize);

void puts_uart(uint32_t uartno, const char* buf);

void putc_uart(uint32_t uartno, char ch);

#endif // !USART_UTILS_H