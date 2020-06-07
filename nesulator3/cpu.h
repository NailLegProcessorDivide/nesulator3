#pragma once
#include <stdint.h>
namespace J6502{
	struct device {
		uint8_t(*readfun)(void*, uint16_t);//data, address
		void(*writefun)(void*, uint16_t, uint8_t);//data, address, value
		uint16_t start;
		uint16_t length;
		void* data;
	};

	struct cpu6502 {
		uint8_t A, X, Y, SP;
		uint16_t PC;
		uint8_t Flags;
		uint8_t Interupts;
		device* devices;
		size_t deviceCount;
		//int (*opmap[256])(cpu6502&);
	};

	struct cpu6502State {
		uint8_t A, X, Y;
		uint16_t PC;
		uint8_t FLAGS;
	};

	cpu6502* createCpu();
	void deleteCpu(cpu6502&);
	void addDevice(cpu6502&, device*);
	void stepCpu(cpu6502&);
}