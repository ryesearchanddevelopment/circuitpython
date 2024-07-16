// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Ryan Gass
//
// SPDX-License-Identifier: MIT

#pragma once

#include "py/objstr.h"

void common_hal_thread_init(bool user_initiated);
void common_hal_thread_gc_collect(void);

void thread_user_reset(void);
