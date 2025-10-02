#pragma once

#include <HAL/Common.h>

typedef struct
{
    uint8_t cmdStatus;                          ///< SCSI command status: 0 ok, 1 fail
    uint8_t *cdb;                               ///< pointer to beginning of Command Descriptor Block
    uint8_t (*write)(const uint8_t *, uint8_t); ///< function pointer to write to data bus
    uint8_t (*read)(uint8_t *);                 ///< function pointer to read from data bus
}
scsiTransaction_t;

void scsi_reset();

void scsi_write_enable(uint8_t we);

void scsi_process_command(scsiTransaction_t *x);
