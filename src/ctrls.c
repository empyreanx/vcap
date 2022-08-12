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

static bool vcap_type_supported(uint32_t type);
static const char* vcap_type_str(vcap_ctrl_type type);
static int vcap_enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index);
static int vcap_enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index);

int vcap_get_ctrl_info(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_ctrl_info* info)
{
    assert(vd);

    if (!info)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return VCAP_CTRL_ERROR;
    }

    struct v4l2_queryctrl qctrl;

    VCAP_CLEAR(qctrl);
    qctrl.id = ctrl;

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

    if (!vcap_type_supported(qctrl.type))
        return VCAP_CTRL_INVALID;

    if (qctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        return VCAP_CTRL_INVALID;

    // Copy name
    vcap_ustrcpy(info->name, qctrl.name, sizeof(info->name));

    // Copy control ID
    info->id = qctrl.id;

    // Copy type
    info->type = qctrl.type;

    // Copy type string
    vcap_ustrcpy(info->type_name, (uint8_t*)vcap_type_str(info->type), sizeof(info->type_name));

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
    qctrl.id = ctrl;

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

    if (!vcap_type_supported(qctrl.type))
        return VCAP_CTRL_INVALID;

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
    itr.result = vcap_enum_ctrls(vd, &itr.info, 0);

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

    itr->result = vcap_enum_ctrls(itr->vd, &itr->info, ++itr->index);

    return true;
}

vcap_menu_itr vcap_new_menu_itr(vcap_dev* vd, vcap_ctrl_id ctrl)
{
    assert(vd);

    vcap_menu_itr itr = { 0 };
    itr.vd = vd;
    itr.ctrl = ctrl;
    itr.index = 0;
    itr.result = vcap_enum_menu(vd, ctrl, &itr.item, 0);

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

    itr->result = vcap_enum_menu(itr->vd, itr->ctrl, &itr->item, ++itr->index);

    return true;
}

int vcap_get_ctrl(vcap_dev* vd, vcap_ctrl_id ctrl, int32_t* value)
{
    assert(vd);

    if (!value)
    {
        vcap_set_error(vd, "Parameter can't be null");
        return -1;
    }

    struct v4l2_control gctrl;

    VCAP_CLEAR(gctrl);
    gctrl.id = ctrl;

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

    struct v4l2_control sctrl;

    VCAP_CLEAR(sctrl);
    sctrl.id = ctrl;
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

    for (vcap_ctrl_id ctrl = V4L2_CID_BASE; ctrl < V4L2_CID_LASTP1; ctrl++)
    {
        if (vcap_ctrl_status(vd, ctrl) != VCAP_CTRL_OK)
            continue;

        if (vcap_reset_ctrl(vd, ctrl) == -1)
            return -1;
    }

    for (vcap_ctrl_id ctrl = V4L2_CID_CAMERA_CLASS_BASE; ctrl < V4L2_CID_CAMERA_CLASS_BASE + 36; ctrl++)
    {
        if (vcap_ctrl_status(vd, ctrl) != VCAP_CTRL_OK)
            continue;

        if (vcap_reset_ctrl(vd, ctrl) == -1)
            return -1;
    }

    return 0;
}

static bool vcap_type_supported(uint32_t type)
{
    switch (type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BOOLEAN:
        case V4L2_CTRL_TYPE_MENU:
        case V4L2_CTRL_TYPE_INTEGER_MENU:
        case V4L2_CTRL_TYPE_BUTTON:
            return true;
    }

    return false;
}

static const char* vcap_type_str(vcap_ctrl_type type)
{
    switch (type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
            return "Integer";

        case V4L2_CTRL_TYPE_BOOLEAN:
            return "Boolean";

        case V4L2_CTRL_TYPE_MENU:
            return "Menu";

        case V4L2_CTRL_TYPE_INTEGER_MENU:
            return "Integer Menu";

        case V4L2_CTRL_TYPE_BUTTON:
            return "Button";
    }

    return "Unknown";
}

static int vcap_enum_ctrls(vcap_dev* vd, vcap_ctrl_info* info, uint32_t index)
{
    assert(vd);
    assert(info);

    int count = 0;

    for (vcap_ctrl_id ctrl = V4L2_CID_BASE; ctrl < V4L2_CID_LASTP1; ctrl++)
    {
        int result = vcap_get_ctrl_info(vd, ctrl, info);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count)
            return VCAP_ENUM_OK;
        else
            count++;
    }

    for (vcap_ctrl_id ctrl = V4L2_CID_CAMERA_CLASS_BASE; ctrl < V4L2_CID_CAMERA_CLASS_BASE + 36; ctrl++)
    {
        int result = vcap_get_ctrl_info(vd, ctrl, info);

        if (result == VCAP_CTRL_ERROR)
            return VCAP_ENUM_ERROR;

        if (result == VCAP_CTRL_INVALID)
            continue;

        if (index == count)
            return VCAP_ENUM_OK;
        else
            count++;
    }

    return VCAP_ENUM_INVALID;
}

static int vcap_enum_menu(vcap_dev* vd, vcap_ctrl_id ctrl, vcap_menu_item* item, uint32_t index)
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

    if (info.type != V4L2_CTRL_TYPE_MENU && info.type != V4L2_CTRL_TYPE_INTEGER_MENU)
    {
        vcap_set_error(vd, "Control is not a menu");
        return VCAP_ENUM_ERROR;
    }

    if (index + info.min > info.min + info.max)
    {
        return VCAP_ENUM_INVALID;
    }

    // Query menu

    int count = 0;

    for (int i = info.min; i <= info.min + info.max; i++)
    {
        struct v4l2_querymenu qmenu;

        VCAP_CLEAR(qmenu);
        qmenu.id = ctrl;
        qmenu.index = i;

        if (vcap_ioctl(vd->fd, VIDIOC_QUERYMENU, &qmenu) == -1)
        {
            if (errno == EINVAL)
            {
                continue;
            }
            else
            {
                vcap_set_error_errno(vd, "Unable to enumerate menu on device %s", vd->path);
                return VCAP_ENUM_ERROR;
            }
        }

        if (index == count)
        {
            item->index = i;

            if (info.type == V4L2_CTRL_TYPE_MENU)
                vcap_ustrcpy(item->name, qmenu.name, sizeof(item->name));
            else
                item->value = qmenu.value;

            return VCAP_ENUM_OK;
        }
        else
        {
            count++;
        }
    }

    return VCAP_ENUM_INVALID;
}

