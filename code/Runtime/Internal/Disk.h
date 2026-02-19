/*
 RetroDÄCK
 Copyright (c) 2025-2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

EXTERN_C void rt_disk_init();

EXTERN_C void rt_disk_shutdown();

EXTERN_C int32_t rt_disk_read_block512(uint32_t block, uint8_t* buffer, uint32_t bufferLen);

EXTERN_C int32_t rt_disk_write_block512(uint32_t block, const uint8_t* buffer, uint32_t bufferLen);
