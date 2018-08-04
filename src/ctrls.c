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
    "Integer Menu"
    "Button",
    "Unknown"
};

static int enum_ctrls(vcap_fg* fg, vcap_ctrl_desc* desc, uint32_t index);
static int enum_menu(vcap_fg* fg, vcap_ctrl_id cid, vcap_menu_item* item, uint32_t index);

static vcap_ctrl_id convert_ctrl_id(uint32_t id);
static vcap_ctrl_type convert_ctrl_type(uint32_t type);

int vcap_get_ctrl_desc(vcap_fg* fg, vcap_ctrl_id cid, vcap_ctrl_desc* desc) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return VCAP_ENUM_ERROR;
    }

    if (!desc) {
        VCAP_ERROR("Parameter 'desc' cannot be null");
        return VCAP_ENUM_ERROR;
    }

    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl_map[cid];

    if (vcap_ioctl(fg->fd, VIDIOC_QUERYCTRL, &qctrl) == -1) {
        if (errno == EINVAL) {
            return VCAP_CTRL_INVALID;
        } else {
            VCAP_ERROR_ERRNO("Unable to read control descriptor on device '%s'", fg->device.path);
            return VCAP_CTRL_ERROR;
        }
    }

    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        return VCAP_CTRL_INVALID;

    // Copy name
    assert(sizeof(desc->name) == sizeof(qctrl.name));
    memcpy(desc->name, qctrl.name, sizeof(desc->name));

    // Copy control ID
    desc->id = convert_ctrl_id(qctrl.id);

    // Convert type
    vcap_ctrl_type type = convert_ctrl_type(qctrl.type);

    // Copy type
    desc->type = type;

    // Copy type string
    strncpy((char*)desc->type_name, type_name_map[type], sizeof(desc->type_name));

    // Read-only (TODO: consider handling grabbed devices differently)
    if (qctrl.flags & V4L2_CTRL_FLAG_GRABBED || qctrl.flags & V4L2_CTRL_FLAG_READ_ONLY)
        desc->read_only = VCAP_TRUE;
    else
        desc->read_only = VCAP_FALSE;

    // Check if inactive
    if (qctrl.flags & V4L2_CTRL_FLAG_INACTIVE)
        return VCAP_CTRL_INACTIVE;
    else
        return VCAP_CTRL_OK;
}

vcap_ctrl_itr vcap_new_ctrl_itr(vcap_fg* fg) {
    vcap_ctrl_itr itr;

    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        itr.result = VCAP_ENUM_ERROR;
        return itr;
    }

    itr.fg = fg;
    itr.index = 0;
    itr.result = enum_ctrls(fg, &itr.desc, 0);

    return itr;
}

int vcap_ctrl_itr_next(vcap_ctrl_itr* itr, vcap_ctrl_desc* desc) {
    if (!itr)
        return 0; // TODO: Find another way of handling this

    if (!desc) {
        VCAP_ERROR("Parameter 'desc' cannot be null");
        itr->result = VCAP_ENUM_ERROR;
        return 0;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return 0;

    *desc = itr->desc;

    itr->result = enum_ctrls(itr->fg, &itr->desc, ++itr->index);

    return 1;
}

int vcap_ctrl_itr_error(vcap_ctrl_itr* itr) {
    if (!itr) {
        VCAP_ERROR("Parameter 'itr' cannot be null");
        return 1;
    }

    if (itr->result == VCAP_ENUM_ERROR)
        return VCAP_TRUE;
    else
        return VCAP_FALSE;
}

vcap_menu_itr vcap_new_menu_itr(vcap_fg* fg, vcap_ctrl_id cid) {
    vcap_menu_itr itr;

    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        itr.result = VCAP_ENUM_ERROR;
        return itr;
    }

    if (cid < 0 || cid >= VCAP_CTRL_UNKNOWN) {
        VCAP_ERROR("Invalid control (out of range)");
        itr.result = VCAP_ENUM_ERROR;
        return itr;
    }

    itr.fg = fg;
    itr.cid = cid;
    itr.index = 0;
    itr.result = enum_menu(fg, cid, &itr.item, 0);

    return itr;
}

int vcap_menu_itr_next(vcap_menu_itr* itr, vcap_menu_item* item) {
    if (!itr)
        return 0; // TODO: Find another way of handling this

    if (!item) {
        VCAP_ERROR("Parameter 'item' cannot be null");
        itr->result = VCAP_ENUM_ERROR;
        return 0;
    }

    if (itr->result == VCAP_ENUM_INVALID || itr->result == VCAP_ENUM_ERROR)
        return 0;

    *item = itr->item;

    itr->result = enum_menu(itr->fg, itr->cid, &itr->item, ++itr->index);

    return 1;
}

int vcap_menu_itr_error(vcap_menu_itr* itr) {
    if (!itr) {
        VCAP_ERROR("Parameter 'itr' cannot be null");
        return 1;
    }

    if (itr->result == VCAP_ENUM_ERROR)
        return VCAP_TRUE;
    else
        return VCAP_FALSE;
}

int vcap_get_ctrl(vcap_fg* fg, vcap_ctrl_id cid, int32_t* value) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (cid < 0 || cid >= VCAP_CTRL_UNKNOWN) {
        VCAP_ERROR("Invalid control (out of range)");
        return -1;
    }

    if (!value) {
        VCAP_ERROR("Parameter 'value' cannot be null");
        return -1;
    }

    struct v4l2_control ctrl;

	VCAP_CLEAR(ctrl);
	ctrl.id = ctrl_map[cid];

	if (vcap_ioctl(fg->fd, VIDIOC_G_CTRL, &ctrl) == -1) {
		VCAP_ERROR_ERRNO("Could not get control (%d) value on device '%s'", cid, fg->device.path);
		return -1;
	}

	*value = ctrl.value;

    return 0;
}

int vcap_set_ctrl(vcap_fg* fg, vcap_ctrl_id cid, int32_t value) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (cid < 0 || cid >= VCAP_CTRL_UNKNOWN) {
        VCAP_ERROR("Invalid control (out of range)");
        return -1;
    }

    struct v4l2_control ctrl;

	VCAP_CLEAR(ctrl);
	ctrl.id = ctrl_map[cid];
	ctrl.value = value;

	if (vcap_ioctl(fg->fd, VIDIOC_S_CTRL, &ctrl) == -1) {
		VCAP_ERROR_ERRNO("Could not set control (%d) value on device '%s'", cid, fg->device.path);
		return -1;
	}

	return 0;
}

int vcap_reset_ctrl(vcap_fg* fg, vcap_ctrl_id cid) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    if (cid < 0 || cid >= VCAP_CTRL_UNKNOWN) {
        VCAP_ERROR("Invalid control (out of range)");
        return -1;
    }

    vcap_ctrl_desc desc;

    int result = vcap_get_ctrl_desc(fg, cid, &desc);

    if (result == VCAP_CTRL_ERROR || result == VCAP_CTRL_INVALID)
        return -1;

    if (result == VCAP_CTRL_OK) {
        if (vcap_set_ctrl(fg, cid, desc.default_value) == -1)
            return -1;
    }

    return 0;
}

int vcap_reset_all_ctrls(vcap_fg* fg) {
    if (!fg) {
        VCAP_ERROR("Parameter 'fg' cannot be null");
        return -1;
    }

    for (int i = 0; i < VCAP_CTRL_UNKNOWN; i++)
        vcap_reset_ctrl(fg, i);

    return 0;
}

int enum_ctrls(vcap_fg* fg, vcap_ctrl_desc* desc, uint32_t index) {
    int count = 0;

    for (int i = 0; i < VCAP_CTRL_UNKNOWN; i++) {
        int result = vcap_get_ctrl_desc(fg, i, desc);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count) {
            return VCAP_ENUM_OK;
        } else {
            count++;
        }
    }

    return VCAP_ENUM_INVALID;
}

int enum_menu(vcap_fg* fg, vcap_ctrl_id cid, vcap_menu_item* item, uint32_t index) {
    // Check if supported and a menu
    vcap_ctrl_desc desc;

    int result = vcap_get_ctrl_desc(fg, cid, &desc);

    if (result == VCAP_CTRL_ERROR)
        return VCAP_ENUM_ERROR;

    if (result == VCAP_CTRL_INVALID) {
        VCAP_ERROR("Can't enumerate menu of an invalid control");
        return VCAP_ENUM_ERROR;
    }

    assert(result == VCAP_CTRL_OK || result == VCAP_CTRL_INACTIVE);

    if (desc.read_only) {
        VCAP_ERROR("Can't enumerate menu of a read-only control");
        return VCAP_ENUM_ERROR;
    }

    if (desc.type != VCAP_CTRL_TYPE_MENU && desc.type != VCAP_CTRL_TYPE_INTEGER_MENU) {
        VCAP_ERROR("Control is not a menu");
        return VCAP_ENUM_ERROR;
    }

    // Query menu
    struct v4l2_querymenu qmenu;

    VCAP_CLEAR(qmenu);
	qmenu.id = ctrl_map[cid];
	qmenu.index = index;

    if (vcap_ioctl(fg->fd, VIDIOC_QUERYMENU, &qmenu) == -1) {
        if (errno == EINVAL) {
            return VCAP_ENUM_INVALID;
        } else {
            VCAP_ERROR_ERRNO("Unable to enumerate menu on device '%s'", fg->device.path);
            return VCAP_ENUM_ERROR;
        }
    }

    item->index = index;


    if (desc.type == VCAP_CTRL_TYPE_MENU) {
        assert(sizeof(item->name) == sizeof(qmenu.name));
        memcpy(item->name, qmenu.name, sizeof(item->name));
    } else {
        item->value = qmenu.value;
    }

    return VCAP_ENUM_OK;
}

static vcap_ctrl_id convert_ctrl_id(uint32_t id) {
	for (int i = 0; i < VCAP_CTRL_UNKNOWN; i++) {
		if (ctrl_map[i] == id)
			return (vcap_ctrl_id)i;
	}

	return VCAP_CTRL_UNKNOWN;
}

static vcap_ctrl_type convert_ctrl_type(uint32_t type) {
    for (int i = 0; i < VCAP_CTRL_TYPE_UNKNOWN; i++) {
		if (type_map[i] == type)
			return (vcap_ctrl_type)i;
	}

	return VCAP_CTRL_TYPE_UNKNOWN;
}
