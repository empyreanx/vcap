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

#include "../priv.h"

#include <jansson.h>

json_t* build_size_obj(const vcap_size* size) {
    json_t* obj = json_object();

    if (!obj) {
        VCAP_ERROR("Out of memory while allocating JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "width", json_integer(size->width)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "height", json_integer(size->height)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return NULL;
    }

    return obj;
}

json_t* build_rate_obj(const vcap_rate* rate) {
    json_t* obj = json_object();

    if (!obj) {
        VCAP_ERROR("Out of memory while allocating JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "numerator", json_integer(rate->numerator)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "denominator", json_integer(rate->denominator)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return NULL;
    }

    return obj;
}

int vcap_export_settings(vcap_fg* fg, const char* path) {
    json_set_alloc_funcs(vcap_malloc, vcap_free);

    json_t* root = json_object();

    if (!root) {
        VCAP_ERROR("Out of memory while allocating JSON object");
        return -1;
    }

    vcap_fmt_id fid;
    vcap_size size;

    // Format

    if (vcap_get_fmt(fg, &fid, &size) == -1)
        return -1;

    if (json_object_set_new(root, "fid", json_integer(fid)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return -1;
    }

    // Size

    json_t* size_obj = build_size_obj(&size);

    if (!size_obj)
        return -1;

    if (json_object_set_new(root, "size", size_obj) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return -1;
    }

    // Rate

    vcap_rate rate;

    if (vcap_get_rate(fg, &rate) == -1)
        return -1;

    json_t* rate_obj = build_rate_obj(&rate);

    if (!rate_obj)
        return -1;

    if (json_object_set_new(root, "rate", rate_obj) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return -1;
    }

    // Controls

    json_t* ctrls = json_array();

    if (!ctrls) {
        VCAP_ERROR("Out of memory while allocating JSON array");
        return -1;
    }

    for (int cid = 0; cid < VCAP_CTRL_UNKNOWN; cid++) {
        vcap_ctrl_desc desc;

        if (vcap_get_ctrl_desc(fg, cid, &desc) != VCAP_CTRL_OK)
            continue;

        int32_t value;

        if (vcap_get_ctrl(fg, cid, &value) == -1)
            return -1;

        json_t* obj = json_object();

        if (!obj) {
            VCAP_ERROR("Out of memory while allocating JSON object");
            return -1;
        }

        if (json_object_set_new(obj, "name", json_string((char*)desc.name)) == -1) {
            VCAP_ERROR("Unable to set new JSON object");
            return -1;
        }

        if (json_object_set_new(obj, "cid", json_integer(value)) == -1) {
            VCAP_ERROR("Unable to set new JSON object");
            return -1;
        }

        if (json_array_append_new(ctrls, obj) == -1) {
            VCAP_ERROR("Unable to append new object to JSON array");
            return -1;
        }
    }

    if (json_object_set_new(root, "ctrls", ctrls) == -1) {
        VCAP_ERROR("Unable to append new object to JSON array");
        return -1;
    }

    // Save to file
    char* jsonStr = json_dumps(root, JSON_INDENT(4) | JSON_PRESERVE_ORDER);

    FILE* file = fopen(path, "w");

    if (!file) {
        VCAP_ERROR_ERRNO("Unable to open file");
        return -1;
    }

    if (fputs(jsonStr, file) == EOF && ferror(file)) {
        VCAP_ERROR("Error writing to file");
        return -1;
    }

    fclose(file);
    vcap_free(jsonStr);

    // Free memory
    json_decref(root);
    return 0;
}

int vcap_import_settings(vcap_fg* fg, const char* path) {
    json_set_alloc_funcs(vcap_malloc, vcap_free);
    return 0;
}
