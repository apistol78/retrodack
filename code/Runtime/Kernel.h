/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <HAL/Common.h>

typedef struct
{
	uint32_t counter;
}
kernel_cs_t;

typedef struct
{
	uint32_t counter;
}
kernel_sig_t;

typedef void (*kernel_thread_fn_t)();

EXTERN_C void rt_kernel_init();

EXTERN_C uint32_t rt_kernel_create_thread(kernel_thread_fn_t fn);

EXTERN_C void rt_kernel_destroy_thread(uint32_t tid);

EXTERN_C uint32_t rt_kernel_current_thread();

EXTERN_C void rt_kernel_yield();

EXTERN_C void rt_kernel_sleep(uint32_t ms);

EXTERN_C void rt_kernel_enter_critical();

EXTERN_C void rt_kernel_leave_critical();

EXTERN_C void rt_kernel_cs_init(volatile kernel_cs_t* cs);

EXTERN_C void rt_kernel_cs_lock(volatile kernel_cs_t* cs);

EXTERN_C void rt_kernel_cs_unlock(volatile kernel_cs_t* cs);

EXTERN_C void rt_kernel_sig_init(volatile kernel_sig_t* sig);

EXTERN_C void rt_kernel_sig_raise(volatile kernel_sig_t* sig);

EXTERN_C void rt_kernel_sig_wait(volatile kernel_sig_t* sig);

EXTERN_C int32_t rt_kernel_sig_try_wait(volatile kernel_sig_t* sig, uint32_t timeout);
