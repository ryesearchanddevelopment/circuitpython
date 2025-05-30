// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2018 Artur Pacholec
// SPDX-FileCopyrightText: Copyright (c) 2016 Glenn Ruben Bakke
//
// SPDX-License-Identifier: MIT

#include <string.h>

#include "py/runtime.h"
#include "shared/runtime/interrupt_char.h"

#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Adapter.h"
#include "shared-bindings/_bleio/Characteristic.h"
#include "shared-bindings/_bleio/Connection.h"
#include "shared-bindings/_bleio/Descriptor.h"
#include "shared-bindings/_bleio/Service.h"
#include "shared-bindings/_bleio/UUID.h"
#include "shared-bindings/time/__init__.h"
#include "supervisor/shared/bluetooth/bluetooth.h"

#include "common-hal/_bleio/__init__.h"
#include "common-hal/_bleio/ble_events.h"

#include "nvs_flash.h"

static volatile int _completion_status;
static uint64_t _timeout_start_time;

background_callback_t bleio_background_callback;

void bleio_user_reset(void) {
    // Stop any user scanning or advertising.
    common_hal_bleio_adapter_stop_scan(&common_hal_bleio_adapter_obj);
    common_hal_bleio_adapter_stop_advertising(&common_hal_bleio_adapter_obj);

    ble_event_remove_heap_handlers();

    // Maybe start advertising the BLE workflow.
    supervisor_bluetooth_background();
}

// Turn off BLE on a reset or reload.
void bleio_reset(void) {
    // Set this explicitly to save data.
    if (!common_hal_bleio_adapter_get_enabled(&common_hal_bleio_adapter_obj)) {
        return;
    }

    supervisor_stop_bluetooth();
    ble_event_reset();
    bleio_adapter_reset(&common_hal_bleio_adapter_obj);
    common_hal_bleio_adapter_set_enabled(&common_hal_bleio_adapter_obj, false);
    supervisor_start_bluetooth();
}

// The singleton _bleio.Adapter object, bound to _bleio.adapter
// It currently only has properties and no state. Inited by bleio_reset
bleio_adapter_obj_t common_hal_bleio_adapter_obj;

void bleio_background(void *data) {
    (void)data;
    supervisor_bluetooth_background();
}

void common_hal_bleio_init(void) {
    common_hal_bleio_adapter_obj.base.type = &bleio_adapter_type;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);


    bleio_background_callback.fun = bleio_background;
    bleio_background_callback.data = NULL;
}

void common_hal_bleio_gc_collect(void) {
    bleio_adapter_gc_collect(&common_hal_bleio_adapter_obj);
}

void check_nimble_error(int rc, const char *file, size_t line) {
    if (rc == NIMBLE_OK) {
        return;
    }
    switch (rc) {
        case BLE_HS_ENOMEM:
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Nimble out of memory"));
            return;
        case BLE_HS_ETIMEOUT:
            mp_raise_msg(&mp_type_TimeoutError, NULL);
            return;
        case BLE_HS_EINVAL:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid BLE parameter"));
            return;
        case BLE_HS_ENOTCONN:
            mp_raise_ConnectionError(MP_ERROR_TEXT("Not connected"));
            return;
        case BLE_HS_EALREADY:
            mp_raise_bleio_BluetoothError(MP_ERROR_TEXT("Already in progress"));
            return;
        default:
            #if CIRCUITPY_VERBOSE_BLE || CIRCUITPY_DEBUG
            if (file) {
                mp_raise_bleio_BluetoothError(MP_ERROR_TEXT("Unknown system firmware error at %s:%d: %d"), file, line, rc);
            }
            #else
            (void)file;
            (void)line;
            mp_raise_bleio_BluetoothError(MP_ERROR_TEXT("Unknown system firmware error: %d"), rc);
            #endif

            break;
    }
}

void check_ble_error(int error_code, const char *file, size_t line) {
    // 0 means success. For BLE_HS_* codes, there is no defined "SUCCESS" value.
    if (error_code == 0) {
        return;
    }
    switch (error_code) {
        case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
            mp_raise_bleio_SecurityError(MP_ERROR_TEXT("Insufficient authentication"));
            return;
        case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
            mp_raise_bleio_SecurityError(MP_ERROR_TEXT("Insufficient encryption"));
            return;
        default:
            #if CIRCUITPY_VERBOSE_BLE || CIRCUITPY_DEBUG
            if (file) {
                mp_raise_bleio_BluetoothError(MP_ERROR_TEXT("Unknown BLE error at %s:%d: %d"), file, line, error_code);
            }
            #else
            (void)file;
            (void)line;
            mp_raise_bleio_BluetoothError(MP_ERROR_TEXT("Unknown BLE error: %d"), error_code);
            #endif

            break;
    }
}

void check_notify(BaseType_t result) {
    if (result == pdTRUE) {
        return;
    }
    mp_raise_msg(&mp_type_TimeoutError, NULL);
}

void common_hal_bleio_check_connected(uint16_t conn_handle) {
    if (conn_handle == BLEIO_HANDLE_INVALID) {
        mp_raise_ConnectionError(MP_ERROR_TEXT("Not connected"));
    }
}

static void _reset_completion_status(void) {
    _completion_status = 0;
}

// Wait for a status change, recorded in a callback.
// Try twice because sometimes we get a BLE_HS_EAGAIN.
// Maybe we should try more than twice.
static int _wait_for_completion(uint32_t timeout_msecs) {
    for (int tries = 1; tries <= 2; tries++) {
        _timeout_start_time = common_hal_time_monotonic_ms();
        while ((_completion_status == 0) &&
               (common_hal_time_monotonic_ms() < _timeout_start_time + timeout_msecs) &&
               !mp_hal_is_interrupted()) {
            RUN_BACKGROUND_TASKS;
        }
        if (_completion_status != BLE_HS_EAGAIN) {
            // Quit, because either the status is either zero (OK) or it's an error.
            break;
        }
    }
    return _completion_status;
}

typedef struct {
    uint8_t *buf;
    uint16_t len;
} _read_info_t;

static int _read_cb(uint16_t conn_handle,
    const struct ble_gatt_error *error,
    struct ble_gatt_attr *attr,
    void *arg) {
    _read_info_t *read_info = (_read_info_t *)arg;
    switch (error->status) {
        case 0: {
            int len = MIN(read_info->len, OS_MBUF_PKTLEN(attr->om));
            os_mbuf_copydata(attr->om, attr->offset, len, read_info->buf);
            read_info->len = len;
        }
            MP_FALLTHROUGH;

        default:
            #if CIRCUITPY_VERBOSE_BLE
            // For debugging.
            mp_printf(&mp_plat_print, "Read status: %d\n", error->status);
            #endif
            break;
    }
    _completion_status = error->status;

    return 0;
}

int bleio_gattc_read(uint16_t conn_handle, uint16_t value_handle, uint8_t *buf, size_t len) {
    _read_info_t read_info = {
        .buf = buf,
        .len = len
    };
    _reset_completion_status();
    CHECK_NIMBLE_ERROR(ble_gattc_read(conn_handle, value_handle, _read_cb, &read_info));
    CHECK_NIMBLE_ERROR(_wait_for_completion(2000));
    return read_info.len;
}


static int _write_cb(uint16_t conn_handle,
    const struct ble_gatt_error *error,
    struct ble_gatt_attr *attr,
    void *arg) {
    _completion_status = error->status;

    return 0;
}

void bleio_gattc_write(uint16_t conn_handle, uint16_t value_handle, uint8_t *buf, size_t len) {
    _reset_completion_status();
    CHECK_NIMBLE_ERROR(ble_gattc_write_flat(conn_handle, value_handle, buf, len, _write_cb, NULL));
    CHECK_NIMBLE_ERROR(_wait_for_completion(2000));
}
