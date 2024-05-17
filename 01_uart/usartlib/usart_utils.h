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

#define USART_BUF_DEPTH 64

typedef struct {
	volatile uint16_t head;
	volatile uint16_t tail;
	char buf[USART_BUF_DEPTH];
} cyclic_buff_t;

typedef struct {
	uint32_t usart;
	uint32_t rcc;
	uint32_t irq;
	int32_t (*read_char)(void);
	void (*write_char)(char ch);
} uart_handler_t;

int8_t  uart_open(uint32_t baud, const char* cfg, const char* mode, uint32_t rts, uint32_t cts);
int32_t uart_read_keystrokes(char* buf, uint32_t bufsize);
int8_t uart_getc_nb(void);
int8_t uart_getc(void);
void uart_putc(char ch);
void uart_puts(const char* buf);
void USART1_IRQHandler(void);

#endif // !USART_UTILS_H