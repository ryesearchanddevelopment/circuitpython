// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Ryan Gass
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/openthread/__init__.h"

//| """
//| The `openthread` module provides necessary low-level functionality for managing
//| thread connections. Use `socketpool` for communicating over the network."""
//|
//| radio: Radio
//| """thread radio used to manage both station and AP modes.
//| This object is the sole instance of `thread.Radio`."""

// Called when thread is imported.
static mp_obj_t openthread___init__(void) {
    //common_hal_openthread_init(true);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(openthread___init___obj, openthread___init__);

static const mp_rom_map_elem_t openthread_module_globals_table[] = {
    // Name
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_openthread) },

    // Initialization
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&openthread___init___obj) },

    // Properties
};
static MP_DEFINE_CONST_DICT(openthread_module_globals, openthread_module_globals_table);

const mp_obj_module_t openthread_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&openthread_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_openthread, openthread_module);
