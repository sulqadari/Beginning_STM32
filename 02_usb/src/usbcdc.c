#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>

static volatile bool isInitialized = false;
static QueueHandle_t usb_txq;
static QueueHandle_t usb_rxq;

static const struct usb_device_descriptor dev = {};
static const struct usb_config_descriptor config = {};
static const char* usb_strings[] = {};
static uint8_t usbd_control_buffer[128];


static void
cdcacm_set_config(usbd_device* usbd_dev, uint16_t wValue __attribute__((unused)))
{

}

static void
usb_task(void* arg)
{
	usbd_device* udev = (usbd_device*)arg;
	char txbuf[32];
	uint8_t txlen = 0;

	for (;;) {
		
		usbd_poll(udev);
		if (!isInitialized)
			continue;
		
		while (txlen < sizeof txbuf && xQueueReceive(usb_txq, &txbuf[txlen], 0) == pdPASS)
			++txlen;
		
		if (txlen > 0) {
			if (usbd_ep_write_packet(udev, 0x82, txbuf, txlen) != 0)
				txlen = 0;
		} else {
			taskYIELD();
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

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USB);

	/* PA11 = USB_DM, PA12 = USB_DP. */
	udev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config,
					usb_strings, 3,
					usbd_control_buffer, sizeof(usbd_control_buffer));
	
	usbd_register_set_config_callback(udev, cdcacm_set_config);
	xTaskCreate(usb_task, "USB", 200, udev, configMAX_PRIORITIES - 1, NULL);
}
