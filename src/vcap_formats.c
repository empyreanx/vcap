#include <vcap/vcap_formats.h>

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
