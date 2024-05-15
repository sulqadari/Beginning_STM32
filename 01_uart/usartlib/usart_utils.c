
#include "usart_utils.h"

uart_data_t* uart_data[3] = {0, 0, 0};

static int32_t
get_char(uart_data_t* uart)
{
	char rch;

	if (uart->head == uart->tail)
		return (-1);
	
	rch = uart->buf[uart->head];
	uart->head = (uart->head + 1) % USART_BUF_DEPTH;

	return rch;
}

int32_t
getc_uart_nb(uint32_t uartno)
{
	uart_data_t* uart = uart_data[uartno - 1];

	if (!uart)
		return (-1);
	
	return get_char(uart);
}

static char
getc_uart(uint32_t uartno)
{
	uart_data_t* uart = uart_data[uartno - 1];
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


void
putc_uart(uint32_t uartno, char ch)
{
	uint32_t uart = uart_hlr[uartno - 1].usart;

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

uart_handler_t uart_hlr[3] = {
	{ USART1, RCC_USART1, NVIC_USART1_IRQ, uart1_getc, uart1_putc },
	{ USART2, RCC_USART2, NVIC_USART2_IRQ, uart2_getc, uart2_putc },
	{ USART3, RCC_USART3, NVIC_USART3_IRQ, uart3_getc, uart3_putc }
};

int8_t
open_uart(uint32_t uartno, uint32_t baud,
			const char* cfg, const char* mode,
			uint32_t rts, uint32_t cts)
{
	uint32_t uart, ux, stopb, iomode, parity, fc;
	uart_handler_t* info;
	bool rxintf = false;

	if (uartno < 1 || uartno > 3)
		return (-1);
	
	info = &uart_hlr[ux = uartno - 1];	// USART params
	uart = info->usart;				// USART address
	usart_disable_rx_interrupt(uart);

	// Parity
	switch (cfg[1]) {
		case 'O': parity = USART_PARITY_ODD; break;
		case 'E': parity = USART_PARITY_EVEN; break;
		case 'N': parity = USART_PARITY_NONE; break;
		default: return (-2);
	}

	// Stop bits
	stopb = USART_STOPBITS_1;
	switch (cfg[2]) {
		case '.':
		case '0':
			stopb = USART_STOPBITS_0_5;
		break;
		case '1':
			if (cfg[3] == '.')
				stopb = USART_STOPBITS_1_5;
			else
				stopb = USART_STOPBITS_1;
		break;
		case '2':
			stopb = USART_STOPBITS_2;
		break;
		default: return (-3);
	}

	// Transmit mode: "r", "w" or "rw"
	if (mode[0] == 'r' && mode[1] == 'w') {
		iomode = USART_MODE_TX_RX;
		rxintf = true;
	} else if (mode[0] == 'r') {
		iomode = USART_MODE_RX;
		rxintf = true;
	} else if (mode[0] == 'w') {
		iomode = USART_MODE_TX;
	} else
		return (-4);
	
	// Setup RX ISR
	if (rxintf) {
		if (uart_data[ux] == 0)
			uart_data[ux] = pvPortMalloc(sizeof(uart_data_t));
		
		if (NULL == uart_data[ux])
			return (-5);
		
		uart_data[ux]->head = uart_data[ux]->tail = 0;
	}

	// Flow control mode
	fc = USART_FLOWCONTROL_NONE;
	if (rts) {
		if (cts)
			fc = USART_FLOWCONTROL_RTS_CTS;
		else
			fc = USART_FLOWCONTROL_RTS;
	} else if (cts)
		fc = USART_FLOWCONTROL_CTS;
	
	// Establish settings

	rcc_periph_clock_enable(info->rcc);
	usart_set_baudrate(uart, baud);
	usart_set_databits(uart, cfg[0] & 0x0F);
	usart_set_stopbits(uart, stopb);
	usart_set_mode(uart, iomode);
	usart_set_parity(uart, parity);
	usart_set_flow_control(uart, fc);

	nvic_enable_irq(info->irq);
	usart_enable(uart);
	usart_enable_rx_interrupt(uart);

	return (0);
}

static int32_t
getline(char* buf, uint32_t bufsize, int32_t (*get)(void), void (*put)(char ch))
{
#define CONTROL(c) ((c) & 0x1F)
	char ch = 0;
	uint32_t bufx = 0, buflen = 0;

	if (bufsize <= 1)
		return (-1);
	
	--bufsize;	// leave room for null byte

	while (ch != '\n') {
		ch = get();

		switch (ch) {
			case CONTROL('U'):	// kill line
				
				for ( ; bufx > 0; --bufx)
					put('\b');
				for ( ; bufx < buflen; ++bufx)
					put(' ');
				buflen = 0;
			// Fall through
			case CONTROL('A'):	// begin line
				for ( ; bufx > 0; --bufx)
					put('\b');
			break;
			case CONTROL('B'):	// backward char
				if (bufx > 0) {
					--bufx;
					put('\b');
				}
			break;
			case CONTROL('F'):	// forward char
				if (bufx < bufsize && bufx < buflen)
					put(buf[++bufx]);
			break;
			case CONTROL('E'):	// end line
				for ( ; bufx < buflen; ++bufx)
					put(buf[bufx]);
			break;
			case CONTROL('H'):	// backspace char
			case 0x7F:			// rub out
				if (bufx <= 0)
					break;
				--bufx;
				put('\b');
			// fall through
			case CONTROL('D'):	// delete char
				if (bufx < buflen) {
					memmove((buf + bufx), (buf + bufx + 1), (buflen - bufx - 1));
					--buflen;

					for (uint32_t x = bufx; x < buflen; ++x)
						put(buf[x]);
					put(' ');
					for (uint32_t x = buflen + 1; x > bufx; --x)
						put('\b');
				}
			break;
			case CONTROL('I'):	// insert chars (TAB)
				if ((bufx < buflen) && ((buflen + 1) < bufsize)) {
					memmove((buf + bufx + 1), (buf + bufx), (buflen - bufx));
					buf[bufx] = ' ';
					++buflen;
					put(' ');

					for (uint32_t x = bufx + 1; x < buflen; ++x)
						put(buf[x]);
					for (uint32_t x = bufx; x < buflen; ++x)
						put('\b');
				}
			case '\r':
			case '\n':			// end line
				ch = '\n';
			break;
			default:			// overtype
				if (bufx >= bufsize) {
					put(0x07);	// bell
					continue;	// no room left
				}

				buf[bufx++] = ch;
				put(ch);
				if (bufx > buflen)
					buflen = bufx;
		}

		if (bufx > buflen)
			buflen = bufx;
	}

#undef CONTROL

	buf[buflen] = 0;
	put('\n');
	put('\r');
	return bufx;
}

int32_t
getline_uart(uint32_t uartno, char* buf, uint32_t bufsize)
{
	uart_handler_t* uart = &uart_hlr[uartno - 1];
	return getline(buf, bufsize, uart->getc, uart->putc);
}

void
puts_uart(uint32_t uartno, const char* buf)
{
	uint32_t uart = uart_hlr[uartno - 1].usart;

	while (*buf) {
		while ( (USART_SR(uart) & USART_SR_TXE) == 0 )
			taskYIELD();

		usart_send_blocking(uart, *buf++);
	}
}