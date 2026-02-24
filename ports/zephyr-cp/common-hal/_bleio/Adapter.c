// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2018 Dan Halbert for Adafruit Industries
// SPDX-FileCopyrightText: Copyright (c) 2026 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include "py/runtime.h"
#include "bindings/zephyr_kernel/__init__.h"
#include "shared-bindings/_bleio/__init__.h"
#include "shared-bindings/_bleio/Adapter.h"
#include "shared-bindings/_bleio/Address.h"
#include "shared-module/_bleio/Address.h"
#include "shared-module/_bleio/ScanResults.h"
#include "supervisor/shared/tick.h"

bleio_connection_internal_t bleio_connections[BLEIO_TOTAL_CONNECTION_COUNT];

static bool scan_callbacks_registered = false;
static bleio_scanresults_obj_t *active_scan_results = NULL;
static struct bt_le_scan_cb scan_callbacks;
static bool ble_advertising = false;
static bool ble_adapter_enabled = true;

#define BLEIO_ADV_MAX_FIELDS 16
#define BLEIO_ADV_MAX_DATA_LEN 31
static struct bt_data adv_data[BLEIO_ADV_MAX_FIELDS];
static struct bt_data scan_resp_data[BLEIO_ADV_MAX_FIELDS];
static uint8_t adv_data_storage[BLEIO_ADV_MAX_DATA_LEN];
static uint8_t scan_resp_storage[BLEIO_ADV_MAX_DATA_LEN];

static uint8_t bleio_address_type_from_zephyr(const bt_addr_le_t *addr) {
    if (addr == NULL) {
        return BLEIO_ADDRESS_TYPE_PUBLIC;
    }

    switch (addr->type) {
        case BT_ADDR_LE_PUBLIC:
        case BT_ADDR_LE_PUBLIC_ID:
            return BLEIO_ADDRESS_TYPE_PUBLIC;
        case BT_ADDR_LE_RANDOM:
        case BT_ADDR_LE_RANDOM_ID:
        case BT_ADDR_LE_UNRESOLVED:
            if (BT_ADDR_IS_RPA(&addr->a)) {
                return BLEIO_ADDRESS_TYPE_RANDOM_PRIVATE_RESOLVABLE;
            }
            if (BT_ADDR_IS_NRPA(&addr->a)) {
                return BLEIO_ADDRESS_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE;
            }
            return BLEIO_ADDRESS_TYPE_RANDOM_STATIC;
        default:
            return BLEIO_ADDRESS_TYPE_PUBLIC;
    }
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf) {
    if (active_scan_results == NULL || info == NULL || buf == NULL) {
        return;
    }

    const bool connectable = (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0;
    const bool scan_response = (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0;
    const bt_addr_le_t *addr = info->addr;

    uint8_t addr_bytes[NUM_BLEIO_ADDRESS_BYTES] = {0};
    if (addr != NULL) {
        memcpy(addr_bytes, addr->a.val, sizeof(addr_bytes));
    }

    shared_module_bleio_scanresults_append(active_scan_results,
        supervisor_ticks_ms64(),
        connectable,
        scan_response,
        info->rssi,
        addr_bytes,
        bleio_address_type_from_zephyr(addr),
        buf->data,
        buf->len);
}

static void scan_timeout_cb(void) {
    if (active_scan_results == NULL) {
        return;
    }
    shared_module_bleio_scanresults_set_done(active_scan_results, true);
    active_scan_results = NULL;
}

// We need to disassemble the full advertisement packet because the Zephyr takes
// in each ADT in an array.
static size_t bleio_parse_adv_data(const uint8_t *raw, size_t raw_len, struct bt_data *out,
    size_t out_len, uint8_t *storage, size_t storage_len) {
    size_t count = 0;
    size_t offset = 0;
    size_t storage_offset = 0;

    while (offset < raw_len) {
        uint8_t field_len = raw[offset];
        if (field_len == 0) {
            offset++;
            continue;
        }
        uint8_t data_len = field_len - 1;
        if (offset + field_len + 1 > raw_len ||
            count >= out_len ||
            field_len < 1 ||
            storage_offset + data_len > storage_len) {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid advertising data"));
        }
        uint8_t type = raw[offset + 1];
        memcpy(storage + storage_offset, raw + offset + 2, data_len);
        out[count].type = type;
        out[count].data_len = data_len;
        out[count].data = storage + storage_offset;
        storage_offset += data_len;
        count++;
        offset += field_len + 1;
    }

    return count;
}

void common_hal_bleio_adapter_set_enabled(bleio_adapter_obj_t *self, bool enabled) {
    if (enabled) {
        if (!bt_is_ready()) {
            int err = bt_enable(NULL);
            if (err != 0) {
                raise_zephyr_error(err);
            }
        }
        ble_adapter_enabled = true;
        return;
    }

    // On Zephyr bsim + HCI IPC, disabling and immediately re-enabling BLE can
    // race endpoint rebinding during soft reload. Keep the controller running,
    // but present adapter.enabled=False to CircuitPython code.
    common_hal_bleio_adapter_stop_scan(self);
    common_hal_bleio_adapter_stop_advertising(self);
    ble_adapter_enabled = false;
}

bool common_hal_bleio_adapter_get_enabled(bleio_adapter_obj_t *self) {
    return ble_adapter_enabled;
}

mp_int_t common_hal_bleio_adapter_get_tx_power(bleio_adapter_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_adapter_set_tx_power(bleio_adapter_obj_t *self, mp_int_t tx_power) {
    mp_raise_NotImplementedError(NULL);
}

bleio_address_obj_t *common_hal_bleio_adapter_get_address(bleio_adapter_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_bleio_adapter_set_address(bleio_adapter_obj_t *self, bleio_address_obj_t *address) {
    mp_raise_NotImplementedError(NULL);
}

mp_obj_str_t *common_hal_bleio_adapter_get_name(bleio_adapter_obj_t *self) {
    (void)self;
    const char *name = bt_get_name();
    return mp_obj_new_str(name, strlen(name));
}

void common_hal_bleio_adapter_set_name(bleio_adapter_obj_t *self, const char *name) {
    (void)self;
    size_t len = strlen(name);
    int err = 0;
    if (len > CONFIG_BT_DEVICE_NAME_MAX) {
        char truncated[CONFIG_BT_DEVICE_NAME_MAX + 1];
        memcpy(truncated, name, CONFIG_BT_DEVICE_NAME_MAX);
        truncated[CONFIG_BT_DEVICE_NAME_MAX] = '\0';
        err = bt_set_name(truncated);
    } else {
        err = bt_set_name(name);
    }
    if (err != 0) {
        raise_zephyr_error(err);
    }
}

void common_hal_bleio_adapter_start_advertising(bleio_adapter_obj_t *self,
    bool connectable, bool anonymous, uint32_t timeout, mp_float_t interval,
    mp_buffer_info_t *advertising_data_bufinfo,
    mp_buffer_info_t *scan_response_data_bufinfo,
    mp_int_t tx_power, const bleio_address_obj_t *directed_to) {
    (void)tx_power;
    (void)directed_to;
    (void)interval;

    if (advertising_data_bufinfo->len > BLEIO_ADV_MAX_DATA_LEN ||
        scan_response_data_bufinfo->len > BLEIO_ADV_MAX_DATA_LEN) {
        mp_raise_NotImplementedError(NULL);
    }

    if (timeout != 0) {
        mp_raise_NotImplementedError(NULL);
    }

    if (ble_advertising) {
        raise_zephyr_error(-EALREADY);
    }

    bt_addr_le_t id_addrs[CONFIG_BT_ID_MAX];
    size_t id_count = CONFIG_BT_ID_MAX;
    bt_id_get(id_addrs, &id_count);
    if (id_count == 0 || bt_addr_le_eq(&id_addrs[BT_ID_DEFAULT], BT_ADDR_LE_ANY)) {
        int id = bt_id_create(NULL, NULL);
        if (id < 0) {
            printk("Failed to create identity address: %d\n", id);
            raise_zephyr_error(id);
        }
    }

    size_t adv_count = bleio_parse_adv_data(advertising_data_bufinfo->buf,
        advertising_data_bufinfo->len,
        adv_data,
        BLEIO_ADV_MAX_FIELDS,
        adv_data_storage,
        sizeof(adv_data_storage));

    size_t scan_resp_count = 0;
    if (scan_response_data_bufinfo->len > 0) {
        scan_resp_count = bleio_parse_adv_data(scan_response_data_bufinfo->buf,
            scan_response_data_bufinfo->len,
            scan_resp_data,
            BLEIO_ADV_MAX_FIELDS,
            scan_resp_storage,
            sizeof(scan_resp_storage));
    }

    if (anonymous) {
        mp_raise_NotImplementedError(NULL);
    }

    struct bt_le_adv_param adv_params;
    if (connectable) {
        adv_params = (struct bt_le_adv_param)BT_LE_ADV_PARAM_INIT(
            BT_LE_ADV_OPT_CONN,
            BT_GAP_ADV_FAST_INT_MIN_1,
            BT_GAP_ADV_FAST_INT_MAX_1,
            NULL);
    } else if (scan_resp_count > 0) {
        adv_params = (struct bt_le_adv_param)BT_LE_ADV_PARAM_INIT(
            BT_LE_ADV_OPT_SCANNABLE,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL);
    } else {
        adv_params = (struct bt_le_adv_param)BT_LE_ADV_PARAM_INIT(
            0,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL);
    }

    raise_zephyr_error(bt_le_adv_start(&adv_params,
        adv_data,
        adv_count,
        scan_resp_count > 0 ? scan_resp_data : NULL,
        scan_resp_count));

    ble_advertising = true;
}

void common_hal_bleio_adapter_stop_advertising(bleio_adapter_obj_t *self) {
    (void)self;
    if (!ble_advertising) {
        return;
    }
    bt_le_adv_stop();
    ble_advertising = false;
}

bool common_hal_bleio_adapter_get_advertising(bleio_adapter_obj_t *self) {
    (void)self;
    return ble_advertising;
}

mp_obj_t common_hal_bleio_adapter_start_scan(bleio_adapter_obj_t *self, uint8_t *prefixes, size_t prefix_length, bool extended, mp_int_t buffer_size, mp_float_t timeout, mp_float_t interval, mp_float_t window, mp_int_t minimum_rssi, bool active) {
    (void)extended;

    if (self->scan_results != NULL) {
        if (!shared_module_bleio_scanresults_get_done(self->scan_results)) {
            common_hal_bleio_adapter_stop_scan(self);
        } else {
            self->scan_results = NULL;
        }
    }

    int err = 0;

    self->scan_results = shared_module_bleio_new_scanresults(buffer_size, prefixes, prefix_length, minimum_rssi);
    active_scan_results = self->scan_results;

    if (!scan_callbacks_registered) {
        scan_callbacks.recv = scan_recv_cb;
        scan_callbacks.timeout = scan_timeout_cb;
        err = bt_le_scan_cb_register(&scan_callbacks);
        if (err != 0) {
            self->scan_results = NULL;
            active_scan_results = NULL;
            raise_zephyr_error(err);
        }
        scan_callbacks_registered = true;
    }

    uint16_t interval_units = (uint16_t)((interval / 0.000625f) + 0.5f);
    uint16_t window_units = (uint16_t)((window / 0.000625f) + 0.5f);
    uint32_t timeout_units = 0;

    if (timeout > 0.0f) {
        timeout_units = (uint32_t)(timeout * 100.0f + 0.5f);
        if (timeout_units > UINT16_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("timeout must be < 655.35 secs"));
        }
        if (timeout_units == 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("non-zero timeout must be > 0.01"));
        }
    }

    struct bt_le_scan_param scan_params = {
        .type = active ? BT_LE_SCAN_TYPE_ACTIVE : BT_LE_SCAN_TYPE_PASSIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = interval_units,
        .window = window_units,
        .timeout = (uint16_t)timeout_units,
        .interval_coded = 0,
        .window_coded = 0,
    };

    err = bt_le_scan_start(&scan_params, NULL);
    if (err != 0) {
        self->scan_results = NULL;
        active_scan_results = NULL;
        raise_zephyr_error(err);
    }

    return MP_OBJ_FROM_PTR(self->scan_results);
}

void common_hal_bleio_adapter_stop_scan(bleio_adapter_obj_t *self) {
    if (self->scan_results == NULL) {
        return;
    }
    bt_le_scan_stop();
    shared_module_bleio_scanresults_set_done(self->scan_results, true);
    active_scan_results = NULL;
    self->scan_results = NULL;
}

bool common_hal_bleio_adapter_get_connected(bleio_adapter_obj_t *self) {
    return false;
}

mp_obj_t common_hal_bleio_adapter_get_connections(bleio_adapter_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

mp_obj_t common_hal_bleio_adapter_connect(bleio_adapter_obj_t *self, bleio_address_obj_t *address, mp_float_t timeout) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_bleio_adapter_erase_bonding(bleio_adapter_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_bleio_adapter_is_bonded_to_central(bleio_adapter_obj_t *self) {
    return false;
}

void bleio_adapter_gc_collect(bleio_adapter_obj_t *adapter) {
    // Nothing to do for now.
}

void bleio_adapter_reset(bleio_adapter_obj_t *adapter) {
    if (adapter == NULL) {
        return;
    }
    adapter->scan_results = NULL;
    adapter->connection_objs = NULL;
    active_scan_results = NULL;
    ble_advertising = false;
    ble_adapter_enabled = bt_is_ready();
}

bleio_adapter_obj_t *common_hal_bleio_allocate_adapter_or_raise(void) {
    return &common_hal_bleio_adapter_obj;
}

uint16_t bleio_adapter_get_name(char *buf, uint16_t len) {
    const char *name = bt_get_name();
    uint16_t full_len = strlen(name);
    if (len > 0) {
        uint16_t copy_len = len < full_len ? len : full_len;
        memcpy(buf, name, copy_len);
    }
    return full_len;
}
