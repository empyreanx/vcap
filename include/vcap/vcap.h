#ifndef _QCAP_H
#define _QCAP_H

#include <vcap/vcap_controls.h>

#include <stdint.h>
#include <string.h>

#define VCAP_MAX_MMAP_BUFFERS 4
#define VCAP_MIN_MMAP_BUFFERS 2

/**
 * \file
 * \brief Main header.
 */

/**
 * \brief Camera frame size.
 */
typedef struct {
	uint32_t width;				//!< Frame width
	uint32_t height;			//!< Frame height
} vcap_size_t;

/**
 * \brief Camera format descriptor.
 */
typedef struct {
	uint32_t code;				//!< FOURCC pixel format code
	char code_str[5];			//!< FOURCC pixel format string
	char desc[32];				//!< Format description
	uint8_t num_sizes;			//!< Number of frame sizes for this format
	vcap_size_t *sizes;			//!< Frame sizes associated with this format
} vcap_format_t;

/**
 * \brief Control menu item.
 */
typedef struct {
	uint32_t value;				//!< Menu item value
	char name[32];				//!< Menu item name
} vcap_menu_item_t;

/**
 * \brief Camera control descriptor.
 */
typedef struct {
	char name[32];				//!< Menu item value
	vcap_control_id_t id;		//!< Control identifier
	vcap_control_type_t type;	//!< Control type
	int32_t min;				//!< Minimum control value
	int32_t max;				//!< Maximum control value
	int32_t step;				//!< Control step
	int32_t default_value;		//!< Default control value
	uint8_t menu_length;		//!< Control menu length
	vcap_menu_item_t *menu;		//!< Control menu
} vcap_control_t;

/**
 * \brief Internal camera buffer.
 */
typedef struct {
	size_t size;				//!< Buffer size
	void *data;					//!< Buffer data
} vcap_buffer_t;

/**
 * \brief Camera device handle.
 */
typedef struct {
	//public
	char device[64];			//!< Camera device name
	char driver[64];			//!< Device driver name
	char info[256];				//!< Additional device information
	
	//private
	int fd;						//!< Internal
	uint8_t opened;				//!< Internal
	uint8_t streaming;			//!< Internal
	uint8_t num_buffers;		//!< Internal
	vcap_buffer_t *buffers;		//!< Internal
} vcap_camera_t;

/**
 * \brief Returns the last VCap error message. This function is not thread-safe.
 */
char* vcap_error();

/**
 * \brief Retrieves all supported cameras connected to the system.
 */
int vcap_cameras(vcap_camera_t** cameras);

/**
 * \brief Allocates and initializes a camera handle.
 */
vcap_camera_t* vcap_create_camera(const char* device);

/**
 * \brief Destroys a camera handle, releasing all resources in the process.
 */
int vcap_destroy_camera(vcap_camera_t* camera);

/**
 * \brief Destroys an array of cameras handles, releasing all resources in the process.
 */
int vcap_destroy_cameras(vcap_camera_t* cameras, uint16_t num_cameras);

/**
 * \brief Opens the underlying device.
 */
int vcap_open_camera(vcap_camera_t* camera);

/**
 * \brief Closes the underlying device.
 */
int vcap_close_camera(vcap_camera_t* camera);

/**
 * \brief Automatically sets the format on a camera based on the format's priority.
 */
int vcap_auto_set_format(vcap_camera_t* camera);

/**
 * \brief Retrieves all formats supported by the camera.
 */
int vcap_get_formats(vcap_camera_t* camera, vcap_format_t** formats);

/**
 * \brief Retrieves the camera's current format.
 */
int vcap_get_format(vcap_camera_t* camera, uint32_t *format_code, vcap_size_t* size);

/**
 * \brief Sets the camera's format.
 */
int vcap_set_format(vcap_camera_t* camera, uint32_t format_code, vcap_size_t size);

/**
 * \brief Retrieves all frame rates supported by the camera for a given format and frame size.
 */
int vcap_get_frame_rates(vcap_camera_t* camera, uint32_t format_code, vcap_size_t size, uint16_t** frame_rates);

/**
 * \brief Retrieves the camera's current frame rate.
 */
int vcap_get_frame_rate(vcap_camera_t *camera, uint16_t* frame_rate);

/**
 * \brief Sets the camera's frame rate.
 */
int vcap_set_frame_rate(vcap_camera_t *camera, uint16_t frame_rate);

/**
 * \brief Retrieves all supported camera controls.
 */
int vcap_get_controls(vcap_camera_t* camera, vcap_control_t** controls);

/**
 * \brief Retrieves a control's current value.
 */
int vcap_get_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t* value);

/**
 * \brief Set a control's value.
 */
int vcap_set_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t value);

/**
 * \brief Start stream.
 */
int vcap_start_capture(vcap_camera_t *camera);

/**
 * \brief Stop stream.
 */
int vcap_stop_capture(vcap_camera_t *camera);

/**
 * \brief Allocates a buffer, grabs an image from the camera and stores it in the buffer.
 */
int vcap_grab_frame(vcap_camera_t *camera, uint8_t **buffer);

#endif
