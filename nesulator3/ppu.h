#ifndef nesppu
#define nesppu

#include "emulatorGlue.h"

struct ppu {
	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;
	uint8_t OAMDATA;
	uint8_t PPUSCROLLX;
	uint8_t PPUSCROLLY;
	uint16_t PPUADDR;
	uint8_t PPUDATA;
	uint8_t OAMDMA;
	uint8_t* oamram;
	uint32_t frameCounter;
	uint16_t frameRow;
	uint16_t frameCol;
	uint8_t scrollWriteNo;
	uint8_t PPUADDRWriteNo;
};

void createPPU(ppu&);
void stepPPU(ppu&);
void createPPUDevice(device816&, ppu&);
void destroyPPU(ppu&);

#endif