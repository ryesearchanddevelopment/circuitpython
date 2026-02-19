// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2026 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>

#include "py/obj.h"
#include "py/objlist.h"

#include "common-hal/_bleio/__init__.h"
#include "shared-module/_bleio/Address.h"
#include "common-hal/_bleio/Service.h"

typedef enum {
    PAIR_NOT_PAIRED,
    PAIR_WAITING,
    PAIR_PAIRED,
} pair_status_t;

typedef struct {
    uint16_t conn_handle;
    bool is_central;
    mp_obj_list_t *remote_service_list;
    uint16_t ediv;
    volatile pair_status_t pair_status;
    uint8_t sec_status;
    mp_obj_t connection_obj;
    volatile bool conn_params_updating;
    uint16_t mtu;
    volatile bool do_bond_cccds;
    volatile bool do_bond_keys;
    uint64_t do_bond_cccds_request_time;
} bleio_connection_internal_t;

typedef struct {
    mp_obj_base_t base;
    bleio_connection_internal_t *connection;
    uint8_t disconnect_reason;
} bleio_connection_obj_t;

void bleio_connection_clear(bleio_connection_internal_t *self);
uint16_t bleio_connection_get_conn_handle(bleio_connection_obj_t *self);
mp_obj_t bleio_connection_new_from_internal(bleio_connection_internal_t *connection);
bleio_connection_internal_t *bleio_conn_handle_to_connection(uint16_t conn_handle);
