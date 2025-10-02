#include <stdio.h>

#include "Runtime/Disk.h"
#include "Runtime/USB/AccessIO.h"

#define BUS_RW_BUFFER_SIZE      64

uint8_t aio_sector_read(uint32_t address, uint16_t count, uint8_t (*write)(const uint8_t *, uint8_t))
{
	uint8_t buffer[BYTES_PER_SECTOR];
	for (int32_t i = 0; i < count; ++i)
	{
		if (rt_disk_read_block512(address + i, buffer, 512) != 512)
			return FLASH_HW_ERROR;

		for (int32_t j = 0; j < BYTES_PER_SECTOR; j += BUS_RW_BUFFER_SIZE)
		{
			if (write(&buffer[j], BUS_RW_BUFFER_SIZE) != 0)
			{
				printf("... failed to write to bus\n");
				return FLASH_BUS_ERROR;
			}
		}
	}
	return FLASH_CMD_OK;
}

uint8_t aio_sector_erase(uint32_t address, uint16_t count)
{
	return FLASH_CMD_OK;
}

uint8_t aio_sector_write(uint32_t address, uint16_t count, uint8_t (*read)(uint8_t *))
{
	uint8_t buffer[BYTES_PER_SECTOR];
    for (int32_t i = 0; i < count; ++i)
    {
		for (int32_t j = 0; j < BYTES_PER_SECTOR; j += BUS_RW_BUFFER_SIZE)
		{
			if (read(&buffer[j]) != BUS_RW_BUFFER_SIZE)
			{
				printf("... failed to read from bus\n");
				return FLASH_BUS_ERROR;
			}
		}		
        if (rt_disk_write_block512(address + i, buffer, 512) != 512)
			return FLASH_HW_ERROR;
	}	
	return FLASH_CMD_OK;
}
