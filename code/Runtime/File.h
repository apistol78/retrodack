/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <hal/Common.h>

#define FILE_MODE_READ  0
#define FILE_MODE_WRITE 1

#define FILE_SEEK_SET   0
#define FILE_SEEK_CUR   1
#define FILE_SEEK_END   2

typedef void (*fn_enum_t)(void* user, const char* filename, uint32_t size, uint8_t directory);

EXTERN_C int32_t file_init();

EXTERN_C int32_t file_open(const char* name, int32_t mode);

EXTERN_C void file_close(int32_t fd);

EXTERN_C int32_t file_size(int32_t fd);

EXTERN_C int32_t file_seek(int32_t file, int32_t offset, int32_t from);

EXTERN_C int32_t file_write(int32_t file, const uint8_t* ptr, int32_t len);

EXTERN_C int32_t file_read(int32_t file, uint8_t* ptr, int32_t len);

EXTERN_C int32_t file_enumerate(const char* path, void* user, fn_enum_t fnen);

EXTERN_C int32_t file_remove(const char* filename);