/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <hal/Common.h>

EXTERN_C int32_t runtime_init();

EXTERN_C void runtime_update();

EXTERN_C void runtime_warm_restart();

EXTERN_C void runtime_cold_restart();
