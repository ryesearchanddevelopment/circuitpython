// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2026 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"
#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Adapter.h"
#include "supervisor/shared/bluetooth/bluetooth.h"

// The singleton _bleio.Adapter object
bleio_adapter_obj_t common_hal_bleio_adapter_obj;

void common_hal_bleio_init(void) {
    common_hal_bleio_adapter_obj.base.type = &bleio_adapter_type;
}

void bleio_user_reset(void) {
    common_hal_bleio_adapter_stop_scan(&common_hal_bleio_adapter_obj);
    common_hal_bleio_adapter_stop_advertising(&common_hal_bleio_adapter_obj);
    bleio_adapter_reset(&common_hal_bleio_adapter_obj);

    if (supervisor_bluetooth_workflow_is_enabled()) {
        supervisor_bluetooth_background();
    }
}

void bleio_reset(void) {
    common_hal_bleio_adapter_obj.base.type = &bleio_adapter_type;

    common_hal_bleio_adapter_stop_scan(&common_hal_bleio_adapter_obj);
    common_hal_bleio_adapter_stop_advertising(&common_hal_bleio_adapter_obj);

    // Keep Zephyr BLE transport up, but present a disabled adapter state.
    common_hal_bleio_adapter_set_enabled(&common_hal_bleio_adapter_obj, false);
    bleio_adapter_reset(&common_hal_bleio_adapter_obj);

    if (supervisor_bluetooth_workflow_is_enabled()) {
        supervisor_start_bluetooth();
    }
}

void common_hal_bleio_gc_collect(void) {
    // Nothing to do for stubs
}

void common_hal_bleio_check_connected(uint16_t conn_handle) {
    mp_raise_NotImplementedError(NULL);
}

uint16_t common_hal_bleio_device_get_conn_handle(mp_obj_t device) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_device_discover_remote_services(mp_obj_t device, mp_obj_t service_uuids_whitelist) {
    mp_raise_NotImplementedError(NULL);
}

size_t common_hal_bleio_gatts_read(uint16_t handle, uint16_t conn_handle, uint8_t *buf, size_t len) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_gatts_write(uint16_t handle, uint16_t conn_handle, mp_buffer_info_t *bufinfo) {
    mp_raise_NotImplementedError(NULL);
}

size_t common_hal_bleio_gattc_read(uint16_t handle, uint16_t conn_handle, uint8_t *buf, size_t len) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_gattc_write(uint16_t handle, uint16_t conn_handle, mp_buffer_info_t *bufinfo, bool write_no_response) {
    mp_raise_NotImplementedError(NULL);
}
