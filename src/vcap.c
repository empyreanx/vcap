#include <vcap/vcap.h>
#include <vcap/vcap_formats.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>

#define VCAP_CLEAR(arg) memset(&arg, 0, sizeof(arg))

/*
 * Contains the most recent error message.
 */
static char vcap_error_msg[1024];

/*
 * Sets the most recent error message.
 */
static void vcap_set_error(const char *fmt, ...);

/*
 * Wraps the 'ioctl' function so that the IO control request is retried if an interrupt occurs.
 */
static int vcap_ioctl(int fd, int request, void *arg);

/*
 * Filters device list so that 'scandir' returns only video devices.
 */
static int vcap_video_device_filter(const struct dirent *a);

/*
 * Converts a FOURCC character code to a string.
 */
static void vcap_fourcc_str(uint32_t code, char* str);

/*
 * Retrieves the auto set priority associated with a particular format.
 */
static int vcap_format_priority(uint32_t code);

/*
 * Retrieves frame sizes for a given format code.
 */
static int vcap_get_sizes(vcap_camera_t* camera, uint32_t format_code, vcap_size_t** sizes);

/*
 * Retrieves a given control's menu.
 */
static int vcap_get_control_menu(vcap_camera_t* camera, vcap_control_id_t control_id, vcap_menu_item_t** menu);\

/*
 * Maps internal buffers.
 */
static int vcap_map_buffers(vcap_camera_t* camera);

/*
 * Unmaps internal buffers.
 */
static int vcap_umap_buffers(vcap_camera_t* camera);

/*
 * V4L2 control IDs are indexed by VCap control IDs.
 */
static uint32_t control_map[] = {
	V4L2_CID_BRIGHTNESS,
	V4L2_CID_CONTRAST,
	V4L2_CID_SATURATION,
	V4L2_CID_HUE,
	V4L2_CID_AUTO_WHITE_BALANCE,
	V4L2_CID_DO_WHITE_BALANCE,
	V4L2_CID_RED_BALANCE,
	V4L2_CID_BLUE_BALANCE,
	V4L2_CID_GAMMA,
	V4L2_CID_EXPOSURE,
	V4L2_CID_AUTOGAIN,
	V4L2_CID_GAIN,
	V4L2_CID_HFLIP,
	V4L2_CID_VFLIP,
	V4L2_CID_EXPOSURE_AUTO,
	V4L2_CID_EXPOSURE_ABSOLUTE,
	V4L2_CID_EXPOSURE_AUTO_PRIORITY,
	V4L2_CID_FOCUS_ABSOLUTE,
	V4L2_CID_FOCUS_RELATIVE,
	V4L2_CID_FOCUS_AUTO,
	V4L2_CID_ZOOM_ABSOLUTE,
	V4L2_CID_ZOOM_RELATIVE,
	0
};

/*
 * The index of this array represents the associated format's auto set priorty.
 */
static uint32_t format_priority_map[] = {
	VCAP_FMT_RGB24,
	VCAP_FMT_BGR24,
	VCAP_FMT_RGB565,
	VCAP_FMT_YUYV,
	VCAP_FMT_YVYU,
	VCAP_FMT_UYVY,
	VCAP_FMT_YUV420,
	VCAP_FMT_YVU420,
	VCAP_FMT_SPCA501,
	VCAP_FMT_SPCA505,
	VCAP_FMT_SPCA508,
	0
};

/*
 * Returns the VCap control ID corresponding to the V4L2 control ID.
 */
static vcap_control_id_t vcap_convert_control_id(uint32_t id);

/*
 * Returns the VCap control type corresponding to the V4L2 control type.
 */
static vcap_control_type_t vcap_convert_control_type(uint32_t type);

/*
 * Allocates and initializes a camera handle.
 */
vcap_camera_t* vcap_create_camera(const char* device) {
	vcap_camera_t* camera = (vcap_camera_t*)malloc(sizeof(vcap_camera_t));
	
	if (camera == NULL)
		return NULL;
	
	strncpy(camera->device, device, sizeof(camera->device));
	
	camera->opened = camera->streaming = 0;
	
	return camera;
}

/*
 * Destroys a camera handle, releasing all resources in the process.
 */
int vcap_destroy_camera(vcap_camera_t* camera) {
	if (NULL == camera) {
		vcap_set_error("Cannot destroy null camera handle");
		return -1;
	}
	
	if (camera->streaming) {
		if (-1 == vcap_stop_capture(camera))
			return -1;
	}
	
	if (camera->opened) {
		if (-1 == vcap_close_camera(camera))
			return -1;
	}
	
	free(camera);
	
	return 0;
}

/*
 * Destroys an array of cameras handles, releasing all resources in the process.
 */
int vcap_destroy_cameras(vcap_camera_t** cameras, uint16_t num_cameras) {
	for (int i = 0; i < num_cameras; i++) {
		if (-1 == vcap_destroy_camera(cameras[i]))
			return -1;
	}
	
	return 0;
}

/*
 * Retrieves all supported cameras connected to the system.
 */
int vcap_cameras(vcap_camera_t** cameras) {
	int num = 0;
		
	struct dirent **names;
	
	int n = scandir("/dev", &names, vcap_video_device_filter, alphasort);
	
	if (n < 0) {
		vcap_set_error("Failed to scan '/dev' directory");
		return -1;
	}
	char device[64];
	
	*cameras = NULL;	
	
	for (int i = 0; i < n; i++) {
		snprintf(device, sizeof(device), "/dev/%s", names[i]->d_name);
		
		int fd = open(device, O_RDWR, 0);
	
		if (fd == -1)
			continue;
		
		struct v4l2_capability cap;
		
		//query capabilities
		if (-1 == vcap_ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
			close(fd);
			continue;
		}
		
		if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
			close(fd);
			continue;
		}
		
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			close(fd);
			continue;
		}
		
		*cameras = realloc(*cameras, (num + 1) * sizeof(vcap_camera_t));
		
		vcap_camera_t* camera = &(*cameras)[num];
		
		strncpy(camera->device, device, sizeof(camera->device));
		strncpy(camera->driver, (char*)cap.driver, sizeof(camera->driver));
		snprintf(camera->info, sizeof(camera->info), "%s (%s)", cap.card, cap.bus_info);
		
		camera->opened = camera->streaming = 0;
		
		num++;
	}
	
	return num;
}

/*
 * Opens the underlying device.
 */
int vcap_open_camera(vcap_camera_t* camera) {
	struct stat st;
	
	if (camera->opened) {
		vcap_set_error("Device %s is already open", camera->device);
		return -1;
	}
	
	//identify device
	if (-1 == stat(camera->device, &st)) {
		vcap_set_error("Cannot find device %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	if (!S_ISCHR(st.st_mode)) {
		vcap_set_error("Invalid device %s", camera->device);
		return -1;
	}
	
	//open device
	camera->fd = open(camera->device, O_RDWR, 0);
	
	if (camera->fd == -1) {
		vcap_set_error("Unable to open device %s", camera->device);
		return -1;
	}
	
	struct v4l2_capability cap;
	
	//query capabilities
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno)
			vcap_set_error("Device %s is not a V4L2 device", camera->device);
		else
			vcap_set_error("Error querying capabilities on device %s (%s)", camera->device, strerror(errno));
		
		close(camera->fd);
		return -1;
	}
	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		vcap_set_error("Device %s is not a video capture device", camera->device);	
		close(camera->fd);
		return -1;
	}
	
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		vcap_set_error("Device %s does not support streaming", camera->device);
		close(camera->fd);
		return -1;
	}
	
	//some cameras do not set a default format which results in a crash upon mapping buffers
	if (-1 == vcap_auto_set_format(camera)) {
		close(camera->fd);
		return -1;
	}
	
	strncpy(camera->driver, (char*)cap.driver, sizeof(camera->driver));
	snprintf(camera->info, sizeof(camera->info), "%s (%s)", cap.card, cap.bus_info);
	
	camera->opened = 1;
	
	return 0;
}

/*
 * Closes the underlying device.
 */
int vcap_close_camera(vcap_camera_t* camera) {
	if (!camera->opened) {
		vcap_set_error("Device %s is already closed", camera->device);
		return -1;
	}
	
	if (camera->streaming) {
		if (-1 == vcap_stop_capture(camera))
			return -1;
	}
	
	if (-1 == close(camera->fd)) {
		vcap_set_error("Unable to close device %s", camera->device);
		return -1;
	} else {
		camera->opened = 0;
		return 0;
	}
}

/*
 * Automatically sets the format on a camera based on the format's priority.
 */
int vcap_auto_set_format(vcap_camera_t* camera) {
	vcap_format_t* formats;
	
	int num_formats = vcap_get_formats(camera, &formats);
	
	if (num_formats <= 0)
		return -1;
	
	int min_priority = (sizeof(format_priority_map) / sizeof(uint32_t)) - 1;
	int min_index;
	
	for (int i = 0; i < num_formats; i++) {
		int priority = vcap_format_priority(formats[i].code);
		
		if (priority >= 0 && priority < min_priority) {
			min_priority = priority;
			min_index = i;
		}
	}
	
	if (format_priority_map[min_priority]) {
		if (-1 == vcap_set_format(camera, formats[min_index].code, formats[min_index].sizes[0]))
			return -1;
	} else {
		if (-1 == vcap_set_format(camera, formats[0].code, formats[0].sizes[0])) 
			return -1;
	}
	
	return 0;
}

/*
 * Retrieves all formats supported by the camera.
 */
int vcap_get_formats(vcap_camera_t* camera, vcap_format_t** formats) {
	struct v4l2_fmtdesc fmtd;
	
	int index, num = 0;
	
	*formats = NULL;
	
	VCAP_CLEAR(fmtd);
	fmtd.index = index = 0;
	fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	while (-1 != vcap_ioctl(camera->fd, VIDIOC_ENUM_FMT, &fmtd)) {
		*formats = (vcap_format_t*)realloc(*formats, (num + 1) * sizeof(vcap_format_t));
		
		vcap_format_t *format = &(*formats)[num];
		
		//copy pixel format
		format->code = fmtd.pixelformat;
		
		//convert fourcc code to string representation
		vcap_fourcc_str(fmtd.pixelformat, format->code_str);
		
		//copy description
		strncpy(format->desc, (char*)fmtd.description, 32 * sizeof(char));
		
		uint8_t num_sizes = vcap_get_sizes(camera, format->code, &format->sizes);
		
		if (-1 == num_sizes) {
			vcap_set_error("Could not get frame sizes for format %d on device %s", format->code, camera->device);
			free(*formats);
			return -1;
		}
		
		format->num_sizes = num_sizes;
		
		num++;
		
		VCAP_CLEAR(fmtd);
		fmtd.index = ++index;
		fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}

	return num;
}

/*
 * Retrieves the camera's format.
 */
int vcap_get_format(vcap_camera_t* camera, uint32_t *format_code, vcap_size_t* size) {
	struct v4l2_format fmt;
	
	VCAP_CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_G_FMT, &fmt)) {
		vcap_set_error("Could not get format on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	*format_code = fmt.fmt.pix.pixelformat;
	size->width = fmt.fmt.pix.width;
	size->height = fmt.fmt.pix.height;
	
	return 0;
}

/*
 * Sets the camera's format.
 */
int vcap_set_format(vcap_camera_t* camera, uint32_t format_code, vcap_size_t size) {
	struct v4l2_format fmt;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = size.width;
	fmt.fmt.pix.height = size.height;
	fmt.fmt.pix.pixelformat = format_code;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_S_FMT, &fmt)) {
		vcap_set_error("Could not get format on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	return 0;
}

/*
 * Retrieves all frame rates supported by the camera for a given format and frame size.
 */
int vcap_get_frame_rates(vcap_camera_t* camera, int format_code, vcap_size_t size, uint16_t** frame_rates) {
	struct v4l2_frmivalenum frenum;
	
	int index, num = 0;
	
	*frame_rates = NULL;
	
	VCAP_CLEAR(frenum);
	frenum.index = index = 0;
	frenum.pixel_format = format_code;
	frenum.width = size.width;
	frenum.height = size.height;
	
	while (-1 != vcap_ioctl(camera->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frenum)) {
		if (V4L2_FRMIVAL_TYPE_DISCRETE == frenum.type) {
			*frame_rates = (uint16_t*)realloc(*frame_rates, (num + 1) * sizeof(uint16_t));
			(*frame_rates)[num] = frenum.discrete.denominator / frenum.discrete.numerator;
			num++;
		}
		
		VCAP_CLEAR(frenum);
		frenum.index = ++index;
		frenum.pixel_format = format_code;
		frenum.width = size.width;
		frenum.height = size.height;
	}
	
	return num;
}

/*
 * Retrieves the camera's current frame rate.
 */
int vcap_get_frame_rate(vcap_camera_t *camera, uint16_t* frame_rate) {
	struct v4l2_streamparm parm;
	
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if(-1 == vcap_ioctl(camera->fd, VIDIOC_G_PARM, &parm)) {
		vcap_set_error("Could not get framerate on deivce %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	*frame_rate = parm.parm.capture.timeperframe.denominator / parm.parm.capture.timeperframe.numerator;
	
	return 0;
}

/*
 * Sets the camera's current frame rate.
 */
int vcap_set_frame_rate(vcap_camera_t *camera, uint16_t frame_rate) {
	struct v4l2_streamparm parm;
	
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = frame_rate;
	
	if(-1 == vcap_ioctl(camera->fd, VIDIOC_S_PARM, &parm)) {
		vcap_set_error("Could not set framerate on deivce %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	return 0;
}

/*
 * Retrieves all supported camera controls.
 */
int vcap_get_controls(vcap_camera_t* camera, vcap_control_t** controls) {
	if (!camera->opened)
		return -1;
	
	struct v4l2_queryctrl qctrl;
	
	int num = 0;
	
	*controls = NULL;
	
	VCAP_CLEAR(qctrl);
	qctrl.id = V4L2_CTRL_CLASS_USER | V4L2_CTRL_FLAG_NEXT_CTRL;

	while (0 == vcap_ioctl(camera->fd, VIDIOC_QUERYCTRL, &qctrl)) {
		if (!(qctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
			*controls = (vcap_control_t*)realloc(*controls, (num + 1) * sizeof(vcap_control_t));
			
			//convert control id/type
			vcap_control_id_t id = vcap_convert_control_id(qctrl.id);
			vcap_control_type_t type = vcap_convert_control_type(qctrl.type);
			
			if (VCAP_CTRL_INVALID != id && VCAP_CTRL_TYPE_INVALID != type) {
				vcap_control_t* control = &(*controls)[num];
				control->id = id;
				control->type = type;
				control->default_value = qctrl.default_value;
				control->min = qctrl.minimum;
				control->max = qctrl.maximum;
				control->step = qctrl.step;
				strncpy(control->name, (char*)qctrl.name, 32 * sizeof(char));
				
				//retrieve menu
				if (VCAP_CTRL_TYPE_MENU == type) {
					uint8_t menu_length = vcap_get_control_menu(camera, id, &control->menu);
					
					if (-1 == menu_length) {
						vcap_set_error("Could not get menu for control %d on device %s", id, camera->device);
						free(*controls);
						return -1;
					}
					
					control->menu_length = menu_length;
				} else {
					control->menu_length = 0;
					control->menu = NULL;
				}
				
				num++;
			}
		}
		
		qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}
	
	if (EINVAL != errno) {
		vcap_set_error("Error enumerating controls on device %s", camera->device);
		free(*controls);
		return -1;
	}
	
	return num;
}

/*
 * Retrieves a control's value.
 */
int vcap_get_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t* value) {
	struct v4l2_control ctrl;
	
	VCAP_CLEAR(ctrl);
	ctrl.id = control_map[control_id];
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_G_CTRL, &ctrl)) {
		vcap_set_error("Could not get control (%d) on device %s", control_id, camera->device);
		return -1;
	}
	
	*value = ctrl.value;
	
	return 0;
}

/*
 * Set a control's value.
 */
int vcap_set_control_value(vcap_camera_t* camera, vcap_control_id_t control_id, int32_t value) {
	struct v4l2_control ctrl;
	
	VCAP_CLEAR(ctrl);
	ctrl.id = control_map[control_id];
	ctrl.value = value;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_S_CTRL, &ctrl)) {
		vcap_set_error("Could not set control (%d) on device %s", control_id, camera->device);
		return -1;
	}
		
	return 0;
}

/*
 * Start stream.
 */
int vcap_start_capture(vcap_camera_t *camera) {
	if (camera->streaming) {
		vcap_set_error("Device %s is already streaming", camera->device);
		return -1;
	}
	
	if (-1 == vcap_map_buffers(camera)) {
		vcap_set_error("Unable to map buffers on device %s", camera->device);
		return -1;
	}
	
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	//turn on stream
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_STREAMON, &type)) {
		vcap_set_error("Unable to turn on stream on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}	
	
	return 0;
}

/*
 * Stop stream.
 */
int vcap_stop_capture(vcap_camera_t *camera) {
	if (!camera->streaming) {
		vcap_set_error("Device %s is not streaming", camera->device);
		return -1;
	}
	
	if (-1 == vcap_umap_buffers(camera)) {
		vcap_set_error("Unable to unmap buffers on device %s", camera->device);
		return -1;
	}
	
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_STREAMOFF, &type)) {
		vcap_set_error("Unable to turn off stream on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	camera->streaming = 0;
	
	return 0;
}

/*
 * Allocates a buffer, grabs an images from the camera, and stores it in the buffer.
 */
int vcap_grab_frame(vcap_camera_t *camera, uint8_t **buffer) {
	struct v4l2_buffer buf;
	
	//dequeue buffer
	VCAP_CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_DQBUF, &buf)) {
		vcap_set_error("Could not dequeue buffer on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}
	
	*buffer = (uint8_t*)malloc(buf.bytesused);
	
	if (*buffer == NULL) {
		vcap_set_error("Unable to allocate buffer!");
		return -1;
	}
	
	memcpy(*buffer, camera->buffers[buf.index].data, buf.bytesused);

	//requeue buffer
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_QBUF, &buf)) {
		vcap_set_error("Could not requeue buffer on device %s (%s)", camera->device, strerror(errno));
		return -1;
	}

	return buf.bytesused;
}

/*
 * Static functions:
 */
static void vcap_set_error(const char *fmt, ...) {
	va_list args;
		
	va_start(args, fmt);
		
	vsnprintf(vcap_error_msg, sizeof(vcap_error_msg), fmt, args);
	
	va_end(args);
}

static int vcap_ioctl(int fd, int request, void *arg) {
	int r;
	
	do {
		r = ioctl(fd, request, arg);	
	} while (-1 == r && EINTR == errno);
	
	return r;
}

static int vcap_video_device_filter(const struct dirent *a) {
	if (0 == strncmp(a->d_name, "video", 5))
		return 1;
	else
		return 0;
}

static void vcap_fourcc_str(uint32_t code, char* str) {
	str[0] = (code >> 0) & 0xFF;
	str[1] = (code >> 8) & 0xFF;
	str[2] = (code >> 16) & 0xFF;
	str[3] = (code >> 24) & 0xFF;
	str[4] = '\0';
}

static int vcap_format_priority(uint32_t code) {
	for (int i = 0; format_priority_map[i]; i++) {
		if (format_priority_map[i] == code)
			return i;
	}
	
	return -1;
}

static vcap_control_id_t vcap_convert_control_id(uint32_t id) {
	for (int i = 0; control_map[i]; i++) {
		if (control_map[i] == id)
			return (vcap_control_id_t)i;
	}
	
	return VCAP_CTRL_INVALID;
}

static vcap_control_type_t vcap_convert_control_type(uint32_t type) {
	switch (type) {
		case V4L2_CTRL_TYPE_INTEGER:
			return VCAP_CTRL_TYPE_RANGE;
			
		case V4L2_CTRL_TYPE_BOOLEAN:
			return VCAP_CTRL_TYPE_BOOLEAN;
			
		case V4L2_CTRL_TYPE_MENU:
			return VCAP_CTRL_TYPE_MENU;
			
		case V4L2_CTRL_TYPE_BUTTON:
			return VCAP_CTRL_TYPE_BUTTON;
		
		default:
			return VCAP_CTRL_TYPE_INVALID;
	}
}

static int vcap_get_sizes(vcap_camera_t* camera, uint32_t format_code, vcap_size_t** sizes) {
	struct v4l2_frmsizeenum fenum;
	
	int index, num = 0;
	
	*sizes = NULL;

	VCAP_CLEAR(fenum);
	fenum.index = index = 0;
	fenum.pixel_format = format_code;
	
	while (-1 != vcap_ioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES, &fenum)) {
		if (fenum.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
			*sizes = (vcap_size_t*)realloc(*sizes, (num + 1) * sizeof(vcap_size_t));
			
			(*sizes)[num].width = fenum.discrete.width;
			(*sizes)[num].height = fenum.discrete.height;
			
			num++;
		}
		
		VCAP_CLEAR(fenum); 
		fenum.pixel_format = format_code;
		fenum.index = ++index;
	}
	
	return num;
}

static int vcap_get_control_menu(vcap_camera_t* camera, vcap_control_id_t control_id, vcap_menu_item_t** menu) {
	struct v4l2_querymenu qmenu;
	
	*menu = NULL;
	
	int num = 0;
	
	VCAP_CLEAR(qmenu);
	qmenu.id = control_map[control_id];
	qmenu.index = num;
	
	while (0 == vcap_ioctl(camera->fd, VIDIOC_QUERYMENU, &qmenu)) {
		*menu = realloc(*menu, (num + 1) * sizeof(vcap_menu_item_t));
		
		(*menu)[num].value = num;
		strncpy((*menu)[num].name, (char*)qmenu.name, sizeof((*menu)[num].name));
		
		num++;
		
		VCAP_CLEAR(qmenu);
		qmenu.id = control_map[control_id];
		qmenu.index = num;
	}
	
	if (EINVAL != errno) {
		free(*menu);
		return -1;
	}

	return num;
}

static int vcap_map_buffers(vcap_camera_t* camera) {
	//init mmap
	struct v4l2_requestbuffers req;
	
	VCAP_CLEAR(req);
	req.count = VCAP_MAX_MMAP_BUFFERS;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	
	if (-1 == vcap_ioctl(camera->fd, VIDIOC_REQBUFS, &req))
		return -1;
	
	if (req.count < VCAP_MIN_MMAP_BUFFERS)
		return -1;
	
	//allocate buffers
	camera->buffers = (vcap_buffer_t*)malloc(req.count * sizeof(vcap_buffer_t));
	
	if (!camera->buffers)
		return -1;
	
	//map buffers
	for (uint i = 0; i < req.count; i++) {
		struct v4l2_buffer buf;
		
		VCAP_CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		
		if (-1 == vcap_ioctl(camera->fd, VIDIOC_QUERYBUF, &buf))
			return -1;
			
		camera->buffers[i].size = buf.length;
		camera->buffers[i].data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camera->fd, buf.m.offset);
		
		if (MAP_FAILED == camera->buffers[i].data)
			return -1;
	}
	
	camera->num_buffers = req.count;
	
	return 0;
}

static int vcap_umap_buffers(vcap_camera_t* camera) {
	//unmap buffers
	for (uint i = 0; i < camera->num_buffers; i++) {
		if (-1 == munmap(camera->buffers[i].data, camera->buffers[i].size))
			return -1;
	}
	
	return 0;
}
