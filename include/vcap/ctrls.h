/*! \file ctrls.h */

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

#ifndef VCAP_CTRLS_H
#define VCAP_CTRLS_H

#include <linux/videodev2.h>
#include <stdint.h>

///
/// \brief Control IDs
///
/*typedef enum
{
    VCAP_CTRL_BRIGHTNESS,                  // Integer
    VCAP_CTRL_CONTRAST,                    // Integer
    VCAP_CTRL_SATURATION,                  // Integer
    VCAP_CTRL_HUE,                         // Integer
    VCAP_CTRL_AUTO_WHITE_BALANCE,          // Boolean
    VCAP_CTRL_DO_WHITE_BALANCE,            // Button
    VCAP_CTRL_RED_BALANCE,                 // Integer
    VCAP_CTRL_BLUE_BALANCE,                // Integer
    VCAP_CTRL_GAMMA,                       // Integer
    VCAP_CTRL_EXPOSURE,                    // Integer
    VCAP_CTRL_AUTOGAIN,                    // Boolean
    VCAP_CTRL_GAIN,                        // Integer
    VCAP_CTRL_HFLIP,                       // Boolean
    VCAP_CTRL_VFLIP,                       // Boolean
    VCAP_CTRL_POWER_LINE_FREQUENCY,        // Enum
    VCAP_CTRL_HUE_AUTO,                    // Boolean
    VCAP_CTRL_WHITE_BALANCE_TEMPERATURE,   // Integer
    VCAP_CTRL_SHARPNESS,                   // Integer
    VCAP_CTRL_BACKLIGHT_COMPENSATION,      // Integer
    VCAP_CTRL_CHROMA_AGC,                  // Boolean
    VCAP_CTRL_CHROMA_GAIN,                 // Integer
    VCAP_CTRL_COLOR_KILLER,                // Boolean
    VCAP_CTRL_AUTOBRIGHTNESS,              // Boolean
    VCAP_CTRL_ROTATE,                      // Integer
    VCAP_CTRL_BG_COLOR,                    // Integer
    VCAP_CTRL_ILLUMINATORS_1,              // Boolean
    VCAP_CTRL_ILLUMINATORS_2,              // Boolean
    VCAP_CTRL_ALPHA_COMPONENT,             // Integer
    VCAP_CTRL_EXPOSURE_AUTO,               // Enum
    VCAP_CTRL_EXPOSURE_ABSOLUTE,           // Integer
    VCAP_CTRL_EXPOSURE_AUTO_PRIORITY,      // Boolean
    VCAP_CTRL_AUTO_EXPOSURE_BIAS,          // Integer Menu
    VCAP_CTRL_EXPOSURE_METERING,           // Enum
    VCAP_CTRL_PAN_RELATIVE,                // Integer
    VCAP_CTRL_TILT_RELATIVE,               // Integer
    VCAP_CTRL_PAN_RESET,                   // Button
    VCAP_CTRL_TILT_RESET,                  // Button
    VCAP_CTRL_PAN_ABSOLUTE,                // Integer
    VCAP_CTRL_TILT_ABSOLUTE,               // Integer
    VCAP_CTRL_FOCUS_ABSOLUTE,              // Integer
    VCAP_CTRL_FOCUS_RELATIVE,              // Integer
    VCAP_CTRL_FOCUS_AUTO,                  // Boolean
    VCAP_CTRL_AUTO_FOCUS_START,            // Button
    VCAP_CTRL_AUTO_FOCUS_STOP,             // Button
    VCAP_CTRL_AUTO_FOCUS_RANGE,            // Enum
    VCAP_CTRL_ZOOM_ABSOLUTE,               // Integer
    VCAP_CTRL_ZOOM_RELATIVE,               // Integer
    VCAP_CTRL_ZOOM_CONTINUOUS,             // Integer
    VCAP_CTRL_IRIS_ABSOLUTE,               // Integer
    VCAP_CTRL_IRIS_RELATIVE,               // Integer
    VCAP_CTRL_BAND_STOP_FILTER,            // Integer
    VCAP_CTRL_WIDE_DYNAMIC_RANGE,          // Boolean
    VCAP_CTRL_IMAGE_STABILIZATION,         // Boolean
    VCAP_CTRL_PAN_SPEED,                   // Integer
    VCAP_CTRL_TILT_SPEED,                  // Integer
    VCAP_CTRL_UNKNOWN
} vcap_ctrl_id;*/

typedef uint32_t vcap_ctrl_id;
typedef uint8_t vcap_ctrl_type;

#endif // VCAP_CTRLS_H
