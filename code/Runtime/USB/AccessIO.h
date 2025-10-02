#pragma once

#include <HAL/Common.h>

#define NUMBER_OF_SECTORS       31257600
#define BYTES_PER_SECTOR        512

enum flashCommandStatus
{
    FLASH_CMD_OK    = 0,    ///< command completed OK
    FLASH_BUS_ERROR = 1,    ///< error reading/writing data bus
    FLASH_HW_ERROR  = 2,    ///< possible flash hardware error
};

uint8_t aio_sector_read(uint32_t address, uint16_t count, uint8_t (*write)(const uint8_t *, uint8_t));

uint8_t aio_sector_erase(uint32_t address, uint16_t count);

uint8_t aio_sector_write(uint32_t address, uint16_t count, uint8_t (*read)(uint8_t *));
