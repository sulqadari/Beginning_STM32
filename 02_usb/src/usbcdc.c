#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

static volatile bool isInitialized = false;
static QueueHandle_t usb_txq;	// FreeRTOS tx buffer to communicate to the USB stream
static QueueHandle_t usb_rxq;	// FreeRTOS rx buffer to communicate from the USB stream

static const struct usb_device_descriptor dev = {};
static const struct usb_config_descriptor config = {};
static const char* usb_strings[] = {};
static uint8_t usbd_control_buffer[128];

typedef void (**FnAccomplish)(usbd_device* usbd_dev, struct usb_setup_data* req);

/**
 * Used to act on specialized messages. This driver reacts to two type of messages,
 * the first of which is to satisfy a Linux deficiency, and the second one checks the
 * length of a structure.
 */
static int32_t
cdcacm_control_request(
	usbd_device* usbd_dev __attribute__((unused)),
	struct usb_setup_data* req,
	uint8_t** buf __attribute((unused)),
	uint16_t* len, FnAccomplish complete __attribute__((unused)))
{
	switch (req->bRequest) {
		case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		return (1);

		case USB_CDC_REQ_SET_LINE_CODING:
			if (*len < sizeof(struct usb_cdc_line_coding))
				return (0);
			else
				return (1);
	}

	return (0);
}

/**
 * This callbak is invoked by the USB infrastructure when data has been sent over
 * the bus to the STM32 MCU.
 */
static void
cdcacm_data_rx_cb(usbd_device* usbd_dev, uint8_t ep __attribute__((unused)))
{
	uint32_t rx_avail = uxQueueSpacesAvailable(usb_rxq);
	char buf[64];
	int32_t len;

	if (rx_avail == 0)
		return;	// No space to receive a data.
	
	// Bytes to read.
	len = sizeof buf < rx_avail ? sizeof buf : rx_avail;

	// Read what we can, leave the rest.
	len = usbd_ep_read_packet(usbd_dev, 0x01, buf, len);

	for (uint32_t x = 0; x < len; ++x) {
		// send data to the rx queue
		xQueueSend(usb_rxq, &buf[x], 0);
	}
}

/**
 * A callback function used to handle specislized messages.
 * This driver reacts to two req->bRequest message types.
 * @return '1' on successfull handling. '0' otherwise.
 */
static int32_t
cdcacm_control_request(
	usbd_device* sbd_dev __attribute((unused)),
	struct usb_setup_data* req,
	uint8_t** buf __attribute((unused)),
	uint16_t* len,
	FnComplete complete __attribute__((unused)))
{
	switch (req->bRequest) {
		case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		/* The linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functinoal description. */
		return (1);

		case USB_CDC_REQ_SET_LINE_CODING:
			if (*len < sizeof(struct usb_cdc_line_coding)) {
				return (0);
			}
		return (1);
	}

	return (0);
}

/**
 * This callback function is invoked by the USB infrastructure when data 
 * has been sent over the bus to the STM32 MCU.
 */
static void
cdcacm_data_rx_cb(usbd_device* usbd_dev, uint8_t ep __attribute__((unused)))
{
	// How much queue capacity left?
	int32_t rx_avail = uxQueueSpacesAvailable(usb_rxq);
	char buf[64];	// rx buffer.
	int32_t len;

	if (rx_avail <= 0)
		return;	// No space available to RX.

	// Bytes to read
	len = sizeof buf < rx_avail ? sizeof buf : rx_avail;

	// Read what we can, leave the rest.
	for (int32_t x = 0; x < len; ++x) {
		// Send data to the rx queue
		xQueueSend(usb_rxq, &buf[x], 0);
	}
}

/**
 * Called by the Host system upon USB peripheral connection.
 * This callback function configures/reconfigures the USB CDC device. Its signature
 * must match with the one defined in libopencm3/include/libopencm3/usb/usbd.h:
 * 
 * typedef void (*usbd_set_config_callback)(usbd_device *usbd_dev, uint16_t wValue);
 * 
 * Upon return, the 'isInitialized' global variable is set to inform that FreeRTOS task
 * can know the ready state of the USB infrastructure.
 */
static void
cdcacm_set_config(usbd_device* usbd_dev, uint16_t wValue __attribute__((unused)))
{
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
											USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
											cdcacm_control_request);
	isInitialized = true;
}

/** Sends queued bytes of data to the USB Host. */
static void
usb_task(void* arg)
{
	usbd_device* udev = (usbd_device*)arg;
	char txbuf[32];
	int32_t txlen = 0;

	for (;;) {
		
		/* Called frequently enough that the USB link is maintained by the Host. */
		usbd_poll(udev);
		if (!isInitialized)
			continue;
		
		while (txlen < sizeof txbuf && xQueueReceive(usb_txq, &txbuf[txlen], 0) == pdPASS)
			++txlen;	// Count the number of bytes to be sent
		
		if (txlen > 0) {
			if (usbd_ep_write_packet(udev, 0x82, txbuf, txlen) != 0)
				txlen = 0;	// Reset if data have been sent successfully
		} else {
			taskYIELD();	// No data to send. Give up the CPU.
		}
	}
}

/** Start USB driver. */
void
usb_start(void)
{
	usbd_device* udev = NULL;

	usb_txq = xQueueCreate(128, sizeof(char));
	usb_rxq = xQueueCreate(128, sizeof(char));

	/* Since enabling the USB peripheral automatically takes over
	 * the GPIOs PA11 and PA12, all we have to do is enable the GPIO
	 * and USB clocks. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USB);

	/* PA11 = USB_DM, PA12 = USB_DP. */
	udev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config,
					usb_strings, 3,
					usbd_control_buffer, sizeof(usbd_control_buffer));
	
	usbd_register_set_config_callback(udev, cdcacm_set_config);

	/* Create the FreeRTOS task to service the USB events. */
	xTaskCreate(usb_task, "USB", 200, udev, configMAX_PRIORITIES - 1, NULL);
}
