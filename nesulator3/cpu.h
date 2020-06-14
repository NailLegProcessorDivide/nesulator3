#ifndef cpu
#define cpu

#include <stdint.h>
#include "emulatorGlue.h"

struct mos6502 {
public:
	uint8_t A, X, Y, SP;
	uint16_t PC;
	uint8_t flags;
	uint8_t interupts;
	device816* devices;
	size_t deviceCount;
};

struct cpuState {
	uint8_t A, X, Y;
	uint16_t PC;
	uint8_t FLAGS;
};
void createCpu(mos6502&);
bool addDevice(mos6502&, device816&);
int stepCpu(mos6502&);

void triggerNMI(mos6502& _cpu);
void triggerRST(mos6502& _cpu);
void triggerIRQ(mos6502& _cpu);


typedef int (*mos6502instruction)(mos6502&);
#endif // !cpu