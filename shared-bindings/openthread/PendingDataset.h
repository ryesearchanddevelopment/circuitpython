// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Ryan Gass
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>

#include <openthread/dataset_ftd.h>

#include "py/objstr.h"

typedef struct
{
    mp_obj_base_t base;
    otInstance *instance;
    otOperationalDataset *dataset;
} openthread_pending_dataset_obj_t;

extern openthread_pending_dataset_obj_t openthread_pending_dataset_obj;

extern const mp_obj_type_t openthread_pending_dataset_type;