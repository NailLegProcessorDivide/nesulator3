#ifndef memory
#define memory

#include "emulatorGlue.h"

bool createRamDevice816(device816&, uint16_t, uint16_t);
void destroyRamDevice816(device816&);

bool createRomDevice816(device816&, uint16_t, uint16_t);
void destroyRomDevice816(device816&);

void clearMem(device816& dev);
void load(device816& dev);
#endif //memory