#ifndef _QCAP_H
#define _QCAP_H

#include <vcap/vcap_controls.h>

#include <stdint.h>
#include <string.h>

#define VCAP_MAX_MMAP_BUFFERS 4
#define VCAP_MIN_MMAP_BUFFERS 2

typedef struct {
	uint32_t width;
	uint32_t height;
} vcap_size_t;

typedef struct {
	uint32_t code;
	char code_str[5];
	char desc[32];
	uint8_t num_sizes;
	vcap_size_t *sizes;
} vcap_format_t;

typedef struct {
	uint32_t value;
	char name[32];
} vcap_menu_item_t;

typedef struct {
	char name[32];
	vcap_control_id_t id;
	vcap_control_type_t type;
	int32_t min;
	int32_t max;
	int32_t step;
	int32_t default_value;
	uint8_t menu_length;
	vcap_menu_item_t *menu;
} vcap_control_t;

typedef struct {
	size_t size;
	void *data;
} vcap_buffer_t;

typedef struct {
	//public
	char device[64];
	char driver[64];
	char info[256];
	
	//private
	int fd;
	uint8_t opened, streaming;
	uint8_t num_buffers;
	vcap_buffer_t *buffers;
} vcap_camera_t;

char* vcap_error();

int vcap_cameras(vcap_camera_t** cameras);

vcap_camera_t* vcap_create_camera(const char* device);
int vcap_destroy_camera(vcap_camera_t* camera);
int vcap_destroy_cameras(vcap_camera_t* cameras, uint16_t num_cameras);

int vcap_open_camera(vcap_camera_t* camera);
int vcap_close_camera(vcap_camera_t* camera);

int vcap_auto_set_format(vcap_camera_t* camera);

int vcap_get_formats(vcap_camera_t* camera, vcap_format_t** formats);
int vcap_get_format(vcap_camera_t* camera, uint32_t *format_code, vcap_size_t* size);
int vcap_set_format(vcap_camera_t* camera, uint32_t format_code, vcap_size_t size);

int vcap_get_frame_rates(vcap_camera_t* camera, uint32_t format_code, vcap_size_t size, uint16_t** frame_rates);
int vcap_get_frame_rate(vcap_camera_t *camera, uint16_t* frame_rate);
int vcap_set_frame_rate(vcap_camera_t *camera, uint16_t frame_rate);

int vcap_get_controls(vcap_camera_t* camera, vcap_control_t** controls);
int vcap_get_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t* value);
int vcap_set_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t value);

int vcap_start_capture(vcap_camera_t *camera);
int vcap_stop_capture(vcap_camera_t *camera);

int vcap_grab_frame(vcap_camera_t *camera, uint8_t **buffer);

#endif
