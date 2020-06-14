#ifndef emulatorGlue
#define emulatorGlue

#include <stdint.h>

struct device816 {
	uint8_t(*readfun)(void*, uint16_t);//data, address
	void(*writefun)(void*, uint16_t, uint8_t);//data, address, value
	uint16_t start;
	uint16_t length;
	void* data;
};
#endif // !emulatorGlue