/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
.align 6

.globl _start
.globl _start_no_args

_start_no_args:
    addi    sp, sp, -8
    sw      x10, 0(sp)
    sw      x10, 4(sp)
    j       _start
