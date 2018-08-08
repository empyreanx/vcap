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

static json_t* build_size_obj(const vcap_size* size) {
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

static json_t* build_rate_obj(const vcap_rate* rate) {
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

static int parse_size(json_t* obj, vcap_size* size) {
    if (!obj) {
        VCAP_ERROR("Size cannot be null");
        return -1;
    }

    if (json_typeof(obj) != JSON_OBJECT) {
        VCAP_ERROR("Invalid type of size");
        return -1;
    }

    // Width

    json_t* value = json_object_get(obj, "width");

    if (!value || json_typeof(value) != JSON_INTEGER) {
        VCAP_ERROR("Invalid integer type");
        return -1;
    }

    size->width = json_integer_value(value);

    // Height

    value = json_object_get(obj, "height");

    if (!value || json_typeof(value) != JSON_INTEGER) {
        VCAP_ERROR("Invalid integer type");
        return -1;
    }

    size->height = json_integer_value(value);

    return 0;
}

static int parse_rate(json_t* obj, vcap_rate* rate) {
    if (!obj) {
        VCAP_ERROR("Rate cannot be null");
        return -1;
    }

    if (json_typeof(obj) != JSON_OBJECT) {
        VCAP_ERROR("Invalid type of rate");
        return -1;
    }

    // Numerator

    json_t* value = json_object_get(obj, "numerator");

    if (!value || json_typeof(value) != JSON_INTEGER) {
        VCAP_ERROR("Invalid integer type");
        return -1;
    }

    rate->numerator = json_integer_value(value);

    // Denominator

    value = json_object_get(obj, "denominator");

    if (!value || json_typeof(value) != JSON_INTEGER) {
        VCAP_ERROR("Invalid integer type");
        return -1;
    }

    rate->denominator = json_integer_value(value);

    return 0;
}

int vcap_export_settings(vcap_fg* fg, const char* path) {
    json_set_alloc_funcs(vcap_malloc, vcap_free);

    int code;
    FILE* file = NULL;
    char* jsonStr = NULL;

    json_t* root = json_object();

    if (!root) {
        VCAP_ERROR("Out of memory while allocating JSON object");
        return -1;
    }

    vcap_fmt_id fmt;
    vcap_size size;

    // Format

    if (vcap_get_fmt(fg, &fmt, &size) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    if (json_object_set_new(root, "fmt", json_integer(fmt)) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        VCAP_ERROR_GOTO(code, finally);
    }

    // Size

    json_t* size_obj = build_size_obj(&size);

    if (!size_obj) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    if (json_object_set_new(root, "size", size_obj) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        VCAP_ERROR_GOTO(code, finally);
    }

    // Rate

    vcap_rate rate;

    if (vcap_get_rate(fg, &rate) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    json_t* rate_obj = build_rate_obj(&rate);

    if (!rate_obj) {
        VCAP_ERROR("Out of memory while allocating JSON object");
        VCAP_ERROR_GOTO(code, finally);
    }

    if (json_object_set_new(root, "rate", rate_obj) == -1) {
        VCAP_ERROR("Unable to set new JSON object");
        return -1;
    }

    // Controls

    json_t* ctrls = json_array();

    if (!ctrls) {
        VCAP_ERROR("Out of memory while allocating JSON array");
        VCAP_ERROR_GOTO(code, finally);
    }

    for (int ctrl = 0; ctrl < VCAP_CTRL_UNKNOWN; ctrl++) {
        vcap_ctrl_desc desc;

        if (vcap_get_ctrl_desc(fg, ctrl, &desc) != VCAP_CTRL_OK)
            continue;

        int32_t value;

        if (vcap_get_ctrl(fg, ctrl, &value) == -1) {
            VCAP_ERROR_LAST();
            VCAP_ERROR_GOTO(code, finally);
        }

        json_t* obj = json_object();

        if (!obj) {
            VCAP_ERROR("Out of memory while allocating JSON object");
            VCAP_ERROR_GOTO(code, finally);
        }

        if (json_object_set_new(obj, "name", json_string((char*)desc.name)) == -1) {
            VCAP_ERROR("Unable to set new JSON object");
            VCAP_ERROR_GOTO(code, finally);
        }

        if (json_object_set_new(obj, "ctrl", json_integer(ctrl)) == -1) {
            VCAP_ERROR("Unable to set new JSON object");
            VCAP_ERROR_GOTO(code, finally);
        }

        if (json_object_set_new(obj, "value", json_integer(value)) == -1) {
            VCAP_ERROR("Unable to set new JSON object");
            VCAP_ERROR_GOTO(code, finally);
        }

        if (json_array_append_new(ctrls, obj) == -1) {
            VCAP_ERROR("Unable to append new object to JSON array");
            VCAP_ERROR_GOTO(code, finally);
        }
    }

    if (json_object_set_new(root, "ctrls", ctrls) == -1) {
        VCAP_ERROR("Unable to append new object to JSON array");
        VCAP_ERROR_GOTO(code, finally);
    }

    // Save to file
    jsonStr = json_dumps(root, JSON_INDENT(4) | JSON_PRESERVE_ORDER);

    file = fopen(path, "w");

    if (!file) {
        VCAP_ERROR_ERRNO("Unable to open file");
        VCAP_ERROR_GOTO(code, finally);
    }

    if (fputs(jsonStr, file) == EOF && ferror(file)) {
        VCAP_ERROR("Error writing to file");
        VCAP_ERROR_GOTO(code, finally);
    }

finally:

    if (file)
        fclose(file);

    if (jsonStr)
        vcap_free(jsonStr);

    json_decref(root);

    return code;
}

int vcap_import_settings(vcap_fg* fg, const char* path) {
    json_set_alloc_funcs(vcap_malloc, vcap_free);

    int code = 0;
    json_t* root = NULL;
    char* str = NULL;

    // Read file

    FILE *file = fopen(path, "r");

    if (!file) {
        VCAP_ERROR("Unable to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    str = vcap_malloc(file_size + 1);

    if (!str) {
        VCAP_ERROR("Out of memory allocating string");
        VCAP_ERROR_GOTO(code, finally);
    }

    size_t bytes_read = fread(str, 1, file_size, file);

    if (bytes_read != file_size) {
        VCAP_ERROR("Error reading file (expected: %lu, read: %lu)", file_size, bytes_read);
        VCAP_ERROR_GOTO(code, finally);
    }

    str[file_size] = 0;

    // Parse JSON

    json_error_t error;
    root = json_loads(str, 0, &error);

    if (!root) {
        VCAP_ERROR("Parsing JSON failed (%d:%d): %s", error.line, error.column, error.text);
        VCAP_ERROR_GOTO(code, finally);
    }

    // Format

    json_t* value = json_object_get(root, "fmt");

    if (!value) {
        VCAP_ERROR("Unable to read format ID");
        VCAP_ERROR_GOTO(code, finally);
    }

    if (json_typeof(value) != JSON_INTEGER) {
        VCAP_ERROR("Invalid format ID type");
        VCAP_ERROR_GOTO(code, finally);
    }

    vcap_fmt_id fmt = json_integer_value(value);

    // Size

    json_t* obj = json_object_get(root, "size");

    vcap_size size;

    if (parse_size(obj, &size) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    if (vcap_set_fmt(fg, fmt, size) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    // Rate

    obj = json_object_get(root, "rate");

    vcap_rate rate;

    if (parse_rate(obj, &rate) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    if (vcap_set_rate(fg, rate) == -1) {
        VCAP_ERROR_LAST();
        VCAP_ERROR_GOTO(code, finally);
    }

    // Controls

    json_t* array = json_object_get(root, "ctrls");

    if (!array) {
        VCAP_ERROR("Unable to get control array");
        VCAP_ERROR_GOTO(code, finally);
    }

    size_t index;

    json_array_foreach(array, index, obj) {
        // Control ID

        value = json_object_get(obj, "ctrl");

        if (!value || json_typeof(value) != JSON_INTEGER) {
            VCAP_ERROR("Invalid ctrl");
            VCAP_ERROR_GOTO(code, finally);
        }

        vcap_ctrl_id ctrl = json_integer_value(value);

        // Value

        if (vcap_ctrl_status(fg, ctrl) == VCAP_CTRL_OK) {
            value = json_object_get(obj, "value");

            if (!value || json_typeof(value) != JSON_INTEGER) {
                VCAP_ERROR("Invalid value");
                VCAP_ERROR_GOTO(code, finally);
            }

            if (vcap_set_ctrl(fg, ctrl, json_integer_value(value)) == -1) {
                VCAP_ERROR_LAST();
                VCAP_ERROR_GOTO(code, finally);
            }
        }
    }

finally:
    if (file)
        fclose(file);

    if (str)
        vcap_free(str);

    if (root)
        json_decref(root);

    return code;
}
