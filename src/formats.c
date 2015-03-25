/*
 * Vcap - a Video4Linux2 capture library
 * 
 * Copyright (C) 2014 James McLean
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <vcap/vcap.h>

/*
 * Converts a FOURCC character code to a string.
 */
void vcap_fourcc_str(uint32_t code, char* str) {
	str[0] = (code >> 0) & 0xFF;
	str[1] = (code >> 8) & 0xFF;
	str[2] = (code >> 16) & 0xFF;
	str[3] = (code >> 24) & 0xFF;
	str[4] = '\0';
}
