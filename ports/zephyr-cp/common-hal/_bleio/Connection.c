// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2026 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"
#include "shared-bindings/_bleio/Connection.h"

void common_hal_bleio_connection_pair(bleio_connection_internal_t *self, bool bond) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_connection_disconnect(bleio_connection_internal_t *self) {
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_bleio_connection_get_connected(bleio_connection_obj_t *self) {
    return false;
}

mp_int_t common_hal_bleio_connection_get_max_packet_length(bleio_connection_internal_t *self) {
    return 20;
}

bool common_hal_bleio_connection_get_paired(bleio_connection_obj_t *self) {
    return false;
}

mp_obj_tuple_t *common_hal_bleio_connection_discover_remote_services(bleio_connection_obj_t *self, mp_obj_t service_uuids_whitelist) {
    mp_raise_NotImplementedError(NULL);
}

mp_float_t common_hal_bleio_connection_get_connection_interval(bleio_connection_internal_t *self) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_connection_set_connection_interval(bleio_connection_internal_t *self, mp_float_t new_interval) {
    mp_raise_NotImplementedError(NULL);
}

void bleio_connection_clear(bleio_connection_internal_t *self) {
    // Nothing to do
}

uint16_t bleio_connection_get_conn_handle(bleio_connection_obj_t *self) {
    return self->connection->conn_handle;
}

mp_obj_t bleio_connection_new_from_internal(bleio_connection_internal_t *connection) {
    mp_raise_NotImplementedError(NULL);
}

bleio_connection_internal_t *bleio_conn_handle_to_connection(uint16_t conn_handle) {
    return NULL;
}
