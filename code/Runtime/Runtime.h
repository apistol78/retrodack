/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

#include "Runtime/Audio.h"
#include "Runtime/Battery.h"
#include "Runtime/Console.h"
#include "Runtime/CRT.h"
#include "Runtime/Disk.h"
#include "Runtime/ELF.h"
#include "Runtime/File.h"
#include "Runtime/Graphics.h"
#include "Runtime/I2C.h"
#include "Runtime/Input.h"
#include "Runtime/Kernel.h"
#include "Runtime/RTC.h"
#include "Runtime/Timer.h"
#include "Runtime/Video.h"

EXTERN_C int32_t runtime_init();

EXTERN_C void runtime_warm_restart();
