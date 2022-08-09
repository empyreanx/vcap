//==============================================================================
// Vcap - A Video4Linux2 capture library
//
// Copyright (C) 2018 James McLean
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
//==============================================================================

#include "priv.h"

#include <libv4l2.h>
#include <assert.h>

static uint32_t ctrl_map[] = {
    V4L2_CID_BRIGHTNESS,                  // Integer
    V4L2_CID_CONTRAST,                    // Integer
    V4L2_CID_SATURATION,                  // Integer
    V4L2_CID_HUE,                         // Integer
    V4L2_CID_AUTO_WHITE_BALANCE,          // Boolean
    V4L2_CID_DO_WHITE_BALANCE,            // Button
    V4L2_CID_RED_BALANCE,                 // Integer
    V4L2_CID_BLUE_BALANCE,                // Integer
    V4L2_CID_GAMMA,                       // Integer
    V4L2_CID_EXPOSURE,                    // Integer
    V4L2_CID_AUTOGAIN,                    // Boolean
    V4L2_CID_GAIN,                        // Integer
    V4L2_CID_HFLIP,                       // Boolean
    V4L2_CID_VFLIP,                       // Boolean
    V4L2_CID_POWER_LINE_FREQUENCY,        // Enum
    V4L2_CID_HUE_AUTO,                    // Boolean
    V4L2_CID_WHITE_BALANCE_TEMPERATURE,   // Integer
    V4L2_CID_SHARPNESS,                   // Integer
    V4L2_CID_BACKLIGHT_COMPENSATION,      // Integer
    V4L2_CID_CHROMA_AGC,                  // Boolean
    V4L2_CID_CHROMA_GAIN,                 // Integer
    V4L2_CID_COLOR_KILLER,                // Boolean
    V4L2_CID_AUTOBRIGHTNESS,              // Boolean
    V4L2_CID_ROTATE,                      // Integer
    V4L2_CID_BG_COLOR,                    // Integer
    V4L2_CID_ILLUMINATORS_1,              // Boolean
    V4L2_CID_ILLUMINATORS_2,              // Boolean
    V4L2_CID_ALPHA_COMPONENT,             // Integer
    V4L2_CID_EXPOSURE_AUTO,               // Enum
    V4L2_CID_EXPOSURE_ABSOLUTE,           // Integer
    V4L2_CID_EXPOSURE_AUTO_PRIORITY,      // Boolean
    V4L2_CID_AUTO_EXPOSURE_BIAS,          // Integer Menu
    V4L2_CID_EXPOSURE_METERING,           // Enum
    V4L2_CID_PAN_RELATIVE,                // Integer
    V4L2_CID_TILT_RELATIVE,               // Integer
    V4L2_CID_PAN_RESET,                   // Button
    V4L2_CID_TILT_RESET,                  // Button
    V4L2_CID_PAN_ABSOLUTE,                // Integer
    V4L2_CID_TILT_ABSOLUTE,               // Integer
    V4L2_CID_FOCUS_ABSOLUTE,              // Integer
    V4L2_CID_FOCUS_RELATIVE,              // Integer
    V4L2_CID_FOCUS_AUTO,                  // Boolean
    V4L2_CID_AUTO_FOCUS_START,            // Button
    V4L2_CID_AUTO_FOCUS_STOP,             // Button
    V4L2_CID_AUTO_FOCUS_RANGE,            // Enum
    V4L2_CID_ZOOM_ABSOLUTE,               // Integer
    V4L2_CID_ZOOM_RELATIVE,               // Integer
    V4L2_CID_ZOOM_CONTINUOUS,             // Integer
    V4L2_CID_IRIS_ABSOLUTE,               // Integer
    V4L2_CID_IRIS_RELATIVE,               // Integer
    V4L2_CID_BAND_STOP_FILTER,            // Integer
    V4L2_CID_WIDE_DYNAMIC_RANGE,          // Boolean
    V4L2_CID_IMAGE_STABILIZATION,         // Boolean
    V4L2_CID_PAN_SPEED,                   // Integer
    V4L2_CID_TILT_SPEED                   // Integer
};

static uint32_t type_map[] = {
    V4L2_CTRL_TYPE_INTEGER,
    V4L2_CTRL_TYPE_BOOLEAN,
    V4L2_CTRL_TYPE_MENU,
    V4L2_CTRL_TYPE_INTEGER_MENU,
    V4L2_CTRL_TYPE_BUTTON
};

static char* type_name_map[] = {
    "Integer",
    "Boolean",
    "Menu",
    "Integer Menu",
    "Button",
    "Unknown"
};

static int enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index);
static int enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index);

static vcap_ctrl_id convert_ctrl_id(uint32_t id);
static vcap_ctrl_type convert_ctrl_type(uint32_t type);

int vcap_get_ctrl_info(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_info* info)
{
    assert(vd);

    if (ctrl < 0 || ctrl >= VCAP_CTRL_UNKNOWN)
    {
        vcap_set_error(vd, "Invalid control (out of range)");
        return VCAP_CTRL_ERROR;
    }

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return VCAP_CTRL_ERROR;
    }

    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl_map[ctrl];

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_CTRL_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to read control descriptor on device %s", vd->path);
            return VCAP_CTRL_ERROR;
        }
    }

    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        return VCAP_CTRL_INVALID;

    // Copy name
    vcap_ustrcpy(info->name, qctrl.name, sizeof(info->name));

    // Copy control ID
    info->id = convert_ctrl_id(qctrl.id);

    // Convert type
    vcap_ctrl_type type = convert_ctrl_type(qctrl.type);

    // Copy type
    info->type = type;

    // Copy type string
    strncpy((char*)info->type_name, type_name_map[type], sizeof(info->type_name));

    // Min/Max/Step/Default
    info->min = qctrl.minimum;
    info->max = qctrl.maximum;
    info->step = qctrl.step;
    info->default_value = qctrl.default_value;

    // Read-only flag
    if (qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY)
        info->read_only = true;
    else
        info->read_only = false;

    return VCAP_CTRL_OK;
}

int vcap_ctrl_status(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl_map[ctrl];

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYCTRL, &qctrl) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_CTRL_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to check control status on device %s", vd->path);
            return VCAP_CTRL_ERROR;
        }
    }

    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        return VCAP_CTRL_INVALID;

    if (qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY || qctrl.flags & V4L2_CTRL_FLAG_GRABBED)
        return VCAP_CTRL_READ_ONLY;

    if (qctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
        return VCAP_CTRL_INACTIVE;

    return VCAP_CTRL_OK;
}

vcap_ctrl_itr vcap_new_ctrl_itr(vcap_dev* vd)
{
    assert(vd);

    vcap_ctrl_itr itr = { 0 };
    itr.vd = vd;
    itr.index = 0;
    itr.result = enum_ctrls(vd, &itr.info, 0);

    return itr;
}

bool vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_info* info)
{
    assert(itr);

    if (!info)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *info = itr->info;

    itr->result = enum_ctrls(itr->vd, &itr->info, ++itr->index);

    return true;
}

vcap_menu_itr vcap_new_menu_itr(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    vcap_menu_itr itr = { 0 };
    itr.vd = vd;
    itr.ctrl = ctrl;
    itr.index = 0;

    if (ctrl < 0 || ctrl >= VCAP_CTRL_UNKNOWN)
    {
        vcap_set_error(vd, "Invalid control (out of range)");
        itr.result = VCAP_ENUM_ERROR;
    }
    else
    {
        itr.result = enum_menu(vd, ctrl, &itr.item, 0);
    }

    return itr;
}

bool vcap_menu_itr_next(vcap_menu_itr* itr, vcap_menu_item* item)
{
    assert(itr);

    if (!item)
    {
        vcap_set_error(itr->vd, "Parameter can't be null");
        itr->result = VCAP_ENUM_ERROR;
        return false;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return false;

    *item = itr->item;

    itr->result = enum_menu(itr->vd, itr->ctrl, &itr->item, ++itr->index);

    return true;
}

int vcap_get_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t* value)
{
    assert(vd);

    if (ctrl < 0 || ctrl >= VCAP_CTRL_UNKNOWN)
    {
        vcap_set_error(vd, "Invalid control (out of range)");
        return -1;
    }

    if (!value)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    struct v4l2_control gctrl;

    VCAP_CLEAR(gctrl);
    gctrl.id = ctrl_map[ctrl];

    if (vcap_ioctl(vd->fd, VIDIOC_G_CTRL, &gctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not get control (%d) value on device %s", ctrl, vd->path);
        return -1;
    }

    *value = gctrl.value;

    return 0;
}

int vcap_set_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t value)
{
    assert(vd);

    if (ctrl < 0 || ctrl >= VCAP_CTRL_UNKNOWN)
    {
        vcap_set_error(vd, "Invalid control (out of range)");
        return -1;
    }

    struct v4l2_control sctrl;

    VCAP_CLEAR(sctrl);
    sctrl.id = ctrl_map[ctrl];
    sctrl.value = value;

    if (vcap_ioctl(vd->fd, VIDIOC_S_CTRL, &sctrl) == -1)
    {
        vcap_set_error_errno(vd, "Could not set control (%d) value on device %s", ctrl, vd->path);
        return -1;
    }

    return 0;
}

int vcap_reset_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    if (ctrl < 0 || ctrl >= VCAP_CTRL_UNKNOWN)
    {
        vcap_set_error(vd, "Invalid control (out of range)");
        return -1;
    }

    vcap_ctrl_info info;

    int result = vcap_get_ctrl_info(vd, ctrl, &info);

    if (result == VCAP_CTRL_ERROR)
        return -1;

    if (result == VCAP_CTRL_INVALID)
    {
        vcap_set_error(vd, "Invalid control");
        return -1;
    }

    if (result == VCAP_CTRL_OK)
    {
        if (vcap_set_ctrl(vd, ctrl, info.default_value) == -1)
            return -1;
    }

    return 0;
}

int vcap_reset_all_ctrls(vcap_dev* vd)
{
    assert(vd);

    for (int ctrl = 0; ctrl < VCAP_CTRL_UNKNOWN; ctrl++)
    {
        if (vcap_ctrl_status(vd, ctrl) != VCAP_CTRL_OK)
            continue;

        if (vcap_reset_ctrl(vd, ctrl) == -1)
            return -1;
    }

    return 0;
}

int enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index)
{
    assert(vd);
    assert(info);

    int count = 0;

    for (int ctrl = 0; ctrl < VCAP_CTRL_UNKNOWN; ctrl++)
    {
        int result = vcap_get_ctrl_info(vd, ctrl, info);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count)
        {
            return VCAP_ENUM_OK;
        }
        else
        {
            count++;
        }
    }

    return VCAP_ENUM_INVALID;
}

int enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index)
{
    assert(vd);
    assert(item);

    // Check if supported and a menu
    vcap_ctrl_info info;

    int result = vcap_get_ctrl_info(vd, ctrl, &info);

    if (result == VCAP_CTRL_ERROR)
        return VCAP_ENUM_ERROR;

    if (result == VCAP_CTRL_INVALID)
    {
        vcap_set_error(vd, "Can't enumerate menu of an invalid control");
        return VCAP_ENUM_ERROR;
    }

    assert(result == VCAP_CTRL_OK || result == VCAP_CTRL_INACTIVE);

    if (info.read_only)
    {
        vcap_set_error(vd, "Can't enumerate menu of a read-only control");
        return VCAP_ENUM_ERROR;
    }

    if (info.type != VCAP_CTRL_TYPE_MENU && info.type != VCAP_CTRL_TYPE_INTEGER_MENU)
    {
        vcap_set_error(vd, "Control is not a menu");
        return VCAP_ENUM_ERROR;
    }

    if (index + info.min > info.min + info.max)
    {
        return VCAP_ENUM_INVALID;
    }

    // Query menu
    struct v4l2_querymenu qmenu;

    VCAP_CLEAR(qmenu);
    qmenu.id = ctrl_map[ctrl];
    qmenu.index = info.min + index;

    if (vcap_ioctl(vd->fd, VIDIOC_QUERYMENU, &qmenu) == -1)
    {
        if (errno == EINVAL)
        {
            return VCAP_ENUM_INVALID;
        }
        else
        {
            vcap_set_error_errno(vd, "Unable to enumerate menu on device '%s'", vd->path);
            return VCAP_ENUM_ERROR;
        }
    }

    item->index = index;

    if (info.type == VCAP_CTRL_TYPE_MENU)
    {
        vcap_ustrcpy(item->name, qmenu.name, sizeof(item->name));
    }
    else
    {
        item->value = qmenu.value;
    }

    return VCAP_ENUM_OK;
}

static vcap_ctrl_id convert_ctrl_id(uint32_t id)
{
    for (int i = 0; i < VCAP_CTRL_UNKNOWN; i++)
    {
        if (ctrl_map[i] == id)
            return (vcap_ctrl_id)i;
    }

    return VCAP_CTRL_UNKNOWN;
}

static vcap_ctrl_type convert_ctrl_type(uint32_t type)
{
    for (int i = 0; i < VCAP_CTRL_TYPE_UNKNOWN; i++)
    {
        if (type_map[i] == type)
            return (vcap_ctrl_type)i;
    }

    return VCAP_CTRL_TYPE_UNKNOWN;
}
