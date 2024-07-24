// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2024 Ryan Gass
//
// SPDX-License-Identifier: MIT

#include "shared-bindings/openthread/PendingDataset.h"

#include <string.h>
#include <openthread/instance.h>

#include "py/objproperty.h"
#include "py/runtime.h"

//| class PendingDataset:
//|     """The pending thread network dataset."""
//|

//|     def __init__(self) -> None:
//|         """You cannot create an instance of `openthread.PendingDataset`.
//|         Use `openthread.pending_dataset` to access the sole instance available."""
//|         ...

//|     channel: int
//|     """
//|     """
static mp_obj_t openthread_pending_dataset_get_channel(mp_obj_t self_in)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->aDataset->mChannel);
}
MP_DEFINE_CONST_FUN_OBJ_1(openthread_pending_dataset_get_channel_obj, openthread_pending_dataset_get_channel);

static mp_obj_t openthread_pending_dataset_set_channel(mp_obj_t self_in, mp_obj_t channel_in)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->aDataset->mChannel = mp_obj_get_int(channel_in);
    self->aDataset->mComponents.mIsChannelPresent = true;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(openthread_pending_dataset_set_channel_obj, openthread_pending_dataset_set_channel);

MP_PROPERTY_GETSET(openthread_pending_dataset_channel_obj,
                   (mp_obj_t)&openthread_pending_dataset_get_channel_obj,
                   (mp_obj_t)&openthread_pending_dataset_set_channel_obj);

//|     pan_id: int
//|     """
//|     """
static mp_obj_t openthread_pending_dataset_get_pan_id(mp_obj_t self_in)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->aDataset->mPanId);
}
MP_DEFINE_CONST_FUN_OBJ_1(openthread_pending_dataset_get_pan_id_obj, openthread_pending_dataset_get_pan_id);

static mp_obj_t openthread_pending_dataset_set_pan_id(mp_obj_t self_in, mp_obj_t pan_id_in)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->aDataset->mPanId = (otPanId)mp_obj_get_int(pan_id_in);
    self->aDataset->mComponents.mIsPanIdPresent = true;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(openthread_pending_dataset_set_pan_id_obj, openthread_pending_dataset_set_pan_id);

MP_PROPERTY_GETSET(openthread_pending_dataset_pan_id_obj,
                   (mp_obj_t)&openthread_pending_dataset_get_pan_id_obj,
                   (mp_obj_t)&openthread_pending_dataset_set_pan_id_obj);

//|     pan_id_extended: ReadableBuffer
//|     """
//|     """
static mp_obj_t openthread_pending_dataset_get_pan_id_extended(mp_obj_t self_in)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_FROM_PTR(self->aDataset->mExtendedPanId.m8);
}
MP_DEFINE_CONST_FUN_OBJ_1(openthread_pending_dataset_get_pan_id_extended_obj, openthread_pending_dataset_get_pan_id_extended);

static mp_obj_t openthread_pending_dataset_set_pan_id_extended(mp_obj_t self_in, mp_obj_t mac_address_in)
{
    mp_buffer_info_t mac_address;
    mp_get_buffer_raise(mac_address_in, &mac_address, MP_BUFFER_READ);

    if (mac_address.len != OT_EXT_PAN_ID_SIZE)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid PAN ID"));
    }

    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(self_in);
    memcpy(self->aDataset->mExtendedPanId.m8, mac_address.buf, sizeof(self->aDataset->mExtendedPanId));
    self->aDataset->mComponents.mIsExtendedPanIdPresent = true;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(openthread_pending_dataset_set_pan_id_extended_obj, openthread_pending_dataset_set_pan_id_extended);

MP_PROPERTY_GETSET(openthread_pending_dataset_pan_id_extended_obj,
                   (mp_obj_t)&openthread_pending_dataset_get_pan_id_extended_obj,
                   (mp_obj_t)&openthread_pending_dataset_set_pan_id_extended_obj);

//|     def set_active():
//|         """
//|         """
//|         ...
//|
static mp_obj_t openthread_pending_dataset_set_active(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    openthread_pending_dataset_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    otDatasetSetActive(self->instance, self->aDataset);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(openthread_pending_dataset_set_active_obj, 0, openthread_pending_dataset_set_active);

static const mp_rom_map_elem_t openthread_pending_dataset_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&openthread_pending_dataset_channel_obj)},
    {MP_ROM_QSTR(MP_QSTR_pan_id), MP_ROM_PTR(&openthread_pending_dataset_pan_id_obj)},
    {MP_ROM_QSTR(MP_QSTR_pan_id_extended), MP_ROM_PTR(&openthread_pending_dataset_pan_id_extended_obj)},

    {MP_ROM_QSTR(MP_QSTR_set_active), MP_ROM_PTR(&openthread_pending_dataset_set_active_obj)},
};

static MP_DEFINE_CONST_DICT(openthread_pending_dataset_locals_dict, openthread_pending_dataset_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    openthread_pending_dataset_type,
    MP_QSTR_PendingDataset,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    locals_dict, &openthread_pending_dataset_locals_dict);