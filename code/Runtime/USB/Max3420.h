#pragma once

#include <HAL/Common.h>

#define vBusHi() 1

typedef enum usbEvent
{
    USB_NO_EVENT,
    USB_VBUS_LOST,
    BUS_RESET,
    SETUP_PACKET_AVAILABLE,
    EP0_OUT_DATA,
    EP1_OUT_DATA,
    USB_SUSPEND,
    USB_EVENT_ERROR
}
usbEvent_t;

void max3420_init_device();

void max3420_init_usb();

void max3420_terminate_usb();

void max3420_power_down();

usbEvent_t max3420_get_usb_event();

uint8_t max3420_get_setup_packet(uint8_t *buffer);

void max3420_set_device_address(uint8_t address);

void max3420_usb_suspend();

void max3420_stall_endpoint(uint8_t ep);

void max3420_clear_stall_endpoint(uint8_t ep);

uint8_t max3420_is_endpoint_stalled(uint8_t ep);

void max3420_ack_status();

uint8_t max3420_write_endpoint_0(const uint8_t *buffer, uint8_t length);

uint8_t max3420_read_endpoint_1(uint8_t *buffer);

uint8_t max3420_write_endpoint_2(const uint8_t *buffer, uint8_t length);
