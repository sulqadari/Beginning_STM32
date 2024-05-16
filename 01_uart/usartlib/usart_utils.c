
#include "usart_utils.h"

static int32_t
read_char(void)
{
	return uart_getc();
}


static void
write_char(char ch)
{
	if (ch == '\n')
		uart_putc('\r');
	
	uart_putc(ch);
}

cyclic_buff_t* uart_data = NULL;
uart_handler_t uart_hlr = {
	USART1,
	RCC_USART1,
	NVIC_USART1_IRQ,
	read_char,
	write_char,
};

static int32_t
parse_line(char* buf, uint32_t bufsize, int32_t (*get)(void), void (*put)(char ch))
{

#define CONTROL(c) ((c) & 0x1F)

	char ch = 0;
	uint32_t buff_start = 0, buflen = 0;

	if (bufsize <= 1)
		return (-1);

	--bufsize;	// leave room for null byte

	while (ch != '\n') {
		ch = get();

		switch (ch) {
			case CONTROL('U'):	// kill line
				
				for ( ; buff_start > 0; --buff_start)
					put('\b');
				for ( ; buff_start < buflen; ++buff_start)
					put(' ');
				buflen = 0;
			// Fall through
			case CONTROL('A'):	// begin line
				for ( ; buff_start > 0; --buff_start)
					put('\b');
			break;
			case CONTROL('B'):	// backward char
				if (buff_start > 0) {
					--buff_start;
					put('\b');
				}
			break;
			case CONTROL('F'):	// forward char
				if (buff_start < bufsize && buff_start < buflen)
					put(buf[++buff_start]);
			break;
			case CONTROL('E'):	// end line
				for ( ; buff_start < buflen; ++buff_start)
					put(buf[buff_start]);
			break;
			case CONTROL('H'):	// backspace char
			case 0x7F:			// rub out
				if (buff_start <= 0)
					break;
				--buff_start;
				put('\b');
			// fall through
			case CONTROL('D'):	// delete char
				if (buff_start < buflen) {
					memmove((buf + buff_start), (buf + buff_start + 1), (buflen - buff_start - 1));
					--buflen;

					for (uint32_t x = buff_start; x < buflen; ++x)
						put(buf[x]);
					put(' ');
					for (uint32_t x = buflen + 1; x > buff_start; --x)
						put('\b');
				}
			break;
			case CONTROL('I'):	// insert chars (TAB)
				if ((buff_start < buflen) && ((buflen + 1) < bufsize)) {
					memmove((buf + buff_start + 1), (buf + buff_start), (buflen - buff_start));
					buf[buff_start] = ' ';
					++buflen;
					put(' ');

					for (uint32_t x = buff_start + 1; x < buflen; ++x)
						put(buf[x]);
					for (uint32_t x = buff_start; x < buflen; ++x)
						put('\b');
				}
			break;
			case '\r':
			case '\n':			// end line
				ch = '\n';
			break;
			default: {			// overtype
				if (buff_start >= bufsize) {
					put(0x07);	// bell
					continue;	// no room left
				}

				buf[buff_start++] = ch;
				put(ch);
				if (buff_start > buflen)
					buflen = buff_start;
			}
		}

		if (buff_start > buflen)
			buflen = buff_start;
	}

#undef CONTROL

	buf[buflen] = 0;
	put('\n');
	put('\r');
	return buff_start;
}

int8_t
uart_open(uint32_t baud, const char* cfg, const char* mode,
								uint32_t rts, uint32_t cts)
{
	uint32_t uart, stopb, iomode, parity, fc;
	uart_handler_t* info;
	bool rxintf = false;
	
	info = &uart_hlr;				// USART params
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
		if (uart_data == NULL)
			uart_data = pvPortMalloc(sizeof(cyclic_buff_t));
		
		if (NULL == uart_data)
			return (-5);
		
		uart_data->head = uart_data->tail = 0;
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

int32_t
uart_read_keystrokes(char* buf, uint32_t bufsize)
{
	uart_handler_t* uart = &uart_hlr;
	return parse_line(buf, bufsize, uart->read_char, uart->write_char);
}


static int8_t
next_char(cyclic_buff_t* buff)
{
	char rch;

	if (buff->head == buff->tail)
		return (-1);
	
	rch = buff->buf[buff->head];
	buff->head = (buff->head + 1) % USART_BUF_DEPTH;

	return rch;
}

int8_t
uart_getc_nb(void)
{
	cyclic_buff_t* buff = uart_data;

	if (!buff)
		return (-1);

	return next_char(buff);
}

int8_t
uart_getc(void)
{
	cyclic_buff_t* uart = uart_data;
	char rch;

	if (!uart)
		return (-1);
	
	while ( (rch = next_char(uart)) == -1 )
		taskYIELD();
	
	return rch;
}

void
uart_putc(char ch)
{
	uint32_t uart = uart_hlr.usart;

	while ((USART_SR(uart) & USART_SR_TXE) == 0)
		taskYIELD();
	
	usart_send_blocking(uart, ch);
}

void
uart_puts(const char* buf)
{
	uint32_t uart = uart_hlr.usart;

	while (*buf) {
		while ( (USART_SR(uart) & USART_SR_TXE) == 0 )
			taskYIELD();

		usart_send_blocking(uart, *buf++);
	}
}