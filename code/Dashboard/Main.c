#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <HAL/Interrupt.h>
#include <HAL/SPI.h>

#include "Runtime/Audio.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"
#include "Runtime/Kernel.h"

#include "Runtime/USB/Max3420.h"
#include "Runtime/USB/UsbMassStorage.h"
#include "Runtime/USB/ScsiCommands.h"

/*! Main USB event loop
 */
void usb_thread()
{
    max3420_init_usb();
 	scsi_write_enable(1);
    
    /* get and process USB event while Vbus is present */
    while (vBusHi())
    {
        const usbEvent_t event = max3420_get_usb_event();
        switch (event)
        {
            case USB_VBUS_LOST:
				printf("USB_VBUS_LOST\n");
                break;
                
            case BUS_RESET:
				printf("BUS_RESET\n");
                usb_mass_process_bus_reset();
                break;
                
            case SETUP_PACKET_AVAILABLE:
				printf("SETUP_PACKET_AVAILABLE\n");
                usb_mass_process_setup_packet();
                break;
                
            case EP1_OUT_DATA:
				printf("EP1_OUT_DATA\n");
                usb_mass_process_bulk_out_transaction();
                break;
                
            case USB_SUSPEND:
				printf("USB_SUSPEND\n");
                max3420_usb_suspend();
                break;
                
            default:
                break;
        }
    }
    
    max3420_terminate_usb();
}

int main()
{
	runtime_init();

	printf("initialize MAX3420...\n");
	max3420_init_device();

    rt_kernel_create_thread(&usb_thread);

	printf("running...\n");
	for (;;)
	{
        // if (vBusHi())
        // {
        //     usbConnect();
        // }

		// toggle GPIO outputs...
		// max3421_write_byte(MAX_IOPINS1, 0x1 | 0x2 | 0x4 | 0x8);
		// max3421_write_byte(MAX_IOPINS1, 0x0);

        printf("<tick>\n");
        rt_kernel_sleep(1000);
	}


	return 0;
}
