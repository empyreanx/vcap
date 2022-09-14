#ifndef VCAP_SETTINGS_H
#define VCAP_SETTINGS_H

#include "vcap.h"

int vcap_import_settings(vcap_device* vd, const char* json);
int vcap_export_settings(vcap_device* vd, char** json);

#endif // VCAP_SETTINGS_H

#ifdef VCAP_SETTINGS_IMPLEMENTATION

#include <jansson.h>

extern void* vcap_malloc(size_t size);
extern void  vcap_free(void* ptr);
extern void  vcap_set_error_str(const char* func, int line, vcap_device* vd, const char* fmt, ...);
extern void  vcap_set_error_errno_str(const char* func, int line, vcap_device* vd, const char* fmt, ...);

// Set error message
#define vcap_set_error(...) (vcap_set_error_str(__func__, __LINE__, __VA_ARGS__))

// Set message with errno information
#define vcap_set_error_errno(...) (vcap_set_error_errno_str( __func__, __LINE__,  __VA_ARGS__))

static json_t* vcap_build_size(vcap_device* vd, const vcap_size size);
static json_t* vcap_build_rate(vcap_device* vd, const vcap_rate rate);
static json_t* vcap_build_ctrl(vcap_device* vd, const vcap_control_info* info, int32_t value);

static int vcap_parse_size(vcap_device* vd, json_t* obj, vcap_size* size);
static int vcap_parse_rate(vcap_device* vd, json_t* obj, vcap_rate* rate);
static int vcap_parse_ctrl(vcap_device* vd, json_t* obj, vcap_control_id* id, int32_t* value);

int vcap_import_settings(vcap_device* vd, const char* json_str)
{

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);

    if (!root)
    {
        vcap_set_error(vd, "Parsing JSON failed (%d:%d): %s", error.line, error.column, error.text);
        return VCAP_ERROR;
    }

    //==========================================================================
    // Format/size
    //==========================================================================

    json_t* value = json_object_get(root, "format_id");

    if (!value)
    {
        vcap_set_error(vd, "Unable to read format ID");
        json_decref(root);
        return VCAP_ERROR;
    }

    if (json_typeof(value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid format ID (must be integer)");
        json_decref(root);
        return VCAP_ERROR;
    }

    vcap_format_id fmt = json_integer_value(value);

    json_t* obj = json_object_get(root, "size");

    if (!obj)
    {
        vcap_set_error(vd, "Unable to read size");
        json_decref(root);
        return VCAP_ERROR;
    }

    vcap_size size;

    if (vcap_parse_size(vd, obj, &size) == -1)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    if (vcap_set_format(vd, fmt, size) != VCAP_OK)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    //==========================================================================
    // Rate
    //==========================================================================

    obj = json_object_get(root, "rate");

    if (!obj)
    {
        vcap_set_error(vd, "Unable to read rate");
        json_decref(root);
        return VCAP_ERROR;
    }

    vcap_rate rate;

    if (vcap_parse_rate(vd, obj, &rate) == -1)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    if (vcap_set_rate(vd, rate) != VCAP_OK)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    //==========================================================================
    // Controls
    //==========================================================================

    json_t* array = json_object_get(root, "controls");

    if (!array)
    {
        vcap_set_error(vd, "Unable to read control array");
        json_decref(root);
        return VCAP_ERROR;
    }

    size_t index;

    json_array_foreach(array, index, obj)
    {
        uint32_t id;
        int32_t  value;

        if (vcap_parse_ctrl(vd, obj, &id, &value) == VCAP_ERROR)
        {
            json_decref(root);
            return VCAP_ERROR;
        }

        if (vcap_set_control(vd, id, value) != VCAP_OK)
        {
            json_decref(root);
            return VCAP_ERROR;
        }
    }

    json_decref(root);

    return VCAP_OK;
}

int vcap_export_settings(vcap_device* vd, char** json_str)
{
    json_set_alloc_funcs(vcap_malloc, vcap_free);

    if (!json_str)
    {
        vcap_set_error(vd, "Argument can't be NULL");
        return VCAP_ERROR;
    }

    json_t* root = json_object();

    if (!root)
    {
        vcap_set_error(vd, "Unable to create JSON object");
        return VCAP_ERROR;
    }

    //==========================================================================
    // Format/size
    //==========================================================================

    vcap_format_id fmt;
    vcap_size size;

    if (vcap_get_format(vd, &fmt, &size) == VCAP_ERROR)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    if (json_object_set_new(root, "format_id", json_integer(fmt)) == -1)
    {
        vcap_set_error(vd, "Unable to set format ID");
        json_decref(root);
        return VCAP_ERROR;
    }

    json_t* size_obj = vcap_build_size(vd, size);

    if (!size_obj)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    if (json_object_set_new(root, "size", size_obj) == -1)
    {
        vcap_set_error(vd, "Unable to set size");
        json_decref(root);
        json_decref(size_obj);
        return VCAP_ERROR;
    }

    //==========================================================================
    // Frame rate
    //==========================================================================

    vcap_rate rate;

    if (vcap_get_rate(vd, &rate) == VCAP_ERROR)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    json_t* rate_obj = vcap_build_rate(vd, rate);

    if (!rate_obj)
    {
        json_decref(root);
        return VCAP_ERROR;
    }

    if (json_object_set_new(root, "rate", rate_obj) == -1)
    {
        vcap_set_error(vd, "Unable to set rate");
        json_decref(root);
        json_decref(rate_obj);
        return VCAP_ERROR;
    }

    //==========================================================================
    // Controls
    //==========================================================================

    json_t* ctrls = json_array();

    vcap_iterator* itr = vcap_control_iterator(vd);

    //FIXME: check if itr is NULL

    vcap_control_info info;

    while (vcap_next_control(itr, &info))
    {
        vcap_control_status status;

        if (vcap_get_control_status(vd, info.id, &status) != VCAP_OK)
        {
            vcap_free_iterator(itr);
            json_decref(root);
            json_decref(ctrls);
            return VCAP_ERROR;
        }

        if (status.read_only || status.write_only || status.disabled)
            continue;

        int32_t value = 0;

        if (vcap_get_control(vd, info.id, &value) != VCAP_OK)
        {
            vcap_free_iterator(itr);
            json_decref(root);
            json_decref(ctrls);
            return VCAP_ERROR;
        }

        json_t* ctrl_obj = vcap_build_ctrl(vd, &info, value);

        if (!ctrl_obj)
        {
            vcap_free_iterator(itr);
            json_decref(root);
            json_decref(ctrls);
            return VCAP_ERROR;
        }

        if (json_array_append_new(ctrls, ctrl_obj) == -1)
        {
            vcap_set_error(vd, "Unable to append control");
            json_decref(root);
            json_decref(ctrls);
            json_decref(ctrl_obj);
            return VCAP_ERROR;
        }
    }

    if (vcap_iterator_error(itr))
    {
        vcap_free_iterator(itr);
        json_decref(root);
        json_decref(ctrls);
        return VCAP_ERROR;
    }

    vcap_free_iterator(itr);

    if (json_object_set_new(root, "controls", ctrls) == -1)
    {
        vcap_set_error(vd, "Unable to set controls");
        json_decref(root);
        json_decref(ctrls);
        return VCAP_ERROR;
    }

    char* str = json_dumps(root, JSON_INDENT(4) | JSON_PRESERVE_ORDER);

    if (!str)
    {
        vcap_set_error(vd, "Unable to write JSON");
        json_decref(root);
        return VCAP_ERROR;
    }

    *json_str = str;

    json_decref(root);

    return VCAP_OK;
}

static int vcap_parse_size(vcap_device* vd, json_t* obj, vcap_size* size)
{
    if (json_typeof(obj) != JSON_OBJECT)
    {
        vcap_set_error(vd, "Invalid object");
        return -1;
    }

    // Width

    json_t* value = json_object_get(obj, "width");

    if (!value || json_typeof(value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid integer type");
        return VCAP_ERROR;
    }

    size->width = json_integer_value(value);

    // Height

    value = json_object_get(obj, "height");

    if (!value || json_typeof(value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid integer type");
        return VCAP_ERROR;
    }

    size->height = json_integer_value(value);

    return VCAP_OK;
}

static int vcap_parse_rate(vcap_device* vd, json_t* obj, vcap_rate* rate)
{
    if (json_typeof(obj) != JSON_OBJECT)
    {
        vcap_set_error(vd, "Invalid object");
        return VCAP_ERROR;
    }

    // Numerator

    json_t* value = json_object_get(obj, "numerator");

    if (!value || json_typeof(value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid integer type");
        return VCAP_ERROR;
    }

    rate->numerator = json_integer_value(value);

    // Denominator
    value = json_object_get(obj, "denominator");

    if (!value || json_typeof(value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid integer type");
        return VCAP_ERROR;
    }

    rate->denominator = json_integer_value(value);

    return VCAP_OK;
}

static int vcap_parse_ctrl(vcap_device* vd, json_t* obj, vcap_control_id* id, int32_t* value)
{
    //TODO: validate inputs

    if (json_typeof(obj) != JSON_OBJECT)
    {
        vcap_set_error(vd, "Invalid object");
        return VCAP_ERROR;
    }

    json_t* json_value = json_object_get(obj, "id");

    if (!json_value || json_typeof(json_value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid control ID type");
        return VCAP_ERROR;
    }

    *id = json_integer_value(json_value);

    json_value = json_object_get(obj, "value");

    if (!json_value || json_typeof(json_value) != JSON_INTEGER)
    {
        vcap_set_error(vd, "Invalid control value type");
        return VCAP_ERROR;
    }

    *value = json_integer_value(json_value);

    return VCAP_OK;
}

static json_t* vcap_build_size(vcap_device* vd, const vcap_size size)
{
    json_t* obj = json_object();

    if (!obj)
    {
        vcap_set_error(vd, "Unable to create JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "width", json_integer(size.width)) == -1)
    {
        vcap_set_error(vd, "Unable to set width");
        json_decref(obj);
        return NULL;
    }

    if (json_object_set_new(obj, "height", json_integer(size.height)) == -1)
    {
        vcap_set_error(vd, "Unable to set height");
        json_decref(obj);
        return NULL;
    }

    return obj;
}

static json_t* vcap_build_rate(vcap_device* vd, const vcap_rate rate)
{
    json_t* obj = json_object();

    if (!obj)
    {
        vcap_set_error(vd, "Unable to create JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "numerator", json_integer(rate.numerator)) == -1)
    {
        vcap_set_error(vd, "Unable to set numerator");
        json_decref(obj);
        return NULL;
    }

    if (json_object_set_new(obj, "denominator", json_integer(rate.denominator)) == -1)
    {
        vcap_set_error(vd, "Unable to set denominator");
        json_decref(obj);
        return NULL;
    }

    return obj;
}

static json_t* vcap_build_ctrl(vcap_device* vd, const vcap_control_info* info, int32_t value)
{
    json_t* obj = json_object();

    if (!obj)
    {
        vcap_set_error(vd, "Unable to create JSON object");
        return NULL;
    }

    if (json_object_set_new(obj, "id", json_integer(info->id)) == -1)
    {
        vcap_set_error(vd, "Unable to set id");
        json_decref(obj);
        return NULL;
    }

    if (json_object_set_new(obj, "name", json_string((char*)info->name)) == -1)
    {
        vcap_set_error(vd, "Unable to set name");
        json_decref(obj);
        return NULL;
    }

    if (json_object_set_new(obj, "value", json_integer(value)) == -1)
    {
        vcap_set_error(vd, "Unable to set value");
        json_decref(obj);
        return NULL;
    }

    return obj;
}

#endif // VCAP_SETTINGS_IMPLEMENTATION
