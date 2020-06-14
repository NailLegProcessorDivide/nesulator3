#include "cpu.h"

#include <stdio.h>
#include <cstdlib>


/*
###################################--- PUBLIC FUNCTIONS ---#######################################
*/

void createCpu(mos6502& _cpu) {
	_cpu.A = 0;
	_cpu.PC = 0x8000;
	_cpu.SP = 0xFF;
	_cpu.flags = 0;
	_cpu.X = 0;
	_cpu.Y = 0;
	_cpu.interupts = 0;
	_cpu.devices = nullptr;
	_cpu.deviceCount = 0;
	printf("created cpu\n");
}

bool addDevice(mos6502& _cpu, device816& dev) {
	device816* newdevs;
	if (_cpu.deviceCount == 0) {
		_cpu.deviceCount++;
		newdevs = (device816*)malloc(_cpu.deviceCount * sizeof(device816));
	}
	else {
		_cpu.deviceCount++;
		newdevs = (device816*)realloc(_cpu.devices, _cpu.deviceCount * sizeof(device816));
	}
	if (newdevs == nullptr) {
		free(_cpu.devices);
		return false;
	}
	else {
		_cpu.devices = newdevs;
		_cpu.devices[_cpu.deviceCount - 1] = dev;
		return true;
	}
}

/*
###################################--- BASIC READ/WRITE ---#######################################
*/

uint8_t basicRead(mos6502& _cpu, uint16_t address) {
	for (size_t i = 0; i < _cpu.deviceCount; i++) {
		device816& dev = _cpu.devices[i];
		if (dev.start <= address && dev.start + dev.length > address) {
			return dev.readfun(dev.data, address - dev.start);
		}
	}
	return 0;
}

void basicWrite(mos6502& _cpu, uint16_t address, uint8_t value) {
	for (size_t i = 0; i < _cpu.deviceCount; i++) {
		device816& dev = _cpu.devices[i];
		if (dev.start <= address && dev.start + dev.length > address) {
			dev.writefun(dev.data, address - dev.start, value);
		}
	}
}

void push(mos6502& _cpu, uint8_t value) {
	basicWrite(_cpu, 0x100 + _cpu.SP--, value);
}

uint8_t pop(mos6502& _cpu) {
	return basicRead(_cpu, 0x100 + ++_cpu.SP);
}

/*
###################################--- FLAG FUNCTIONS ---#######################################
*/

const static struct _FLAGS {
	uint8_t N = 0b10000000;
	uint8_t V = 0b01000000;
	uint8_t B = 0b00010000;
	uint8_t D = 0b00001000;
	uint8_t I = 0b00000100;
	uint8_t Z = 0b00000010;
	uint8_t C = 0b00000001;
}FLAGS;

inline
void setFlag(mos6502& _cpu, uint8_t flag) {
	_cpu.flags |= flag;
}

inline
void setFlag(mos6502& _cpu, uint8_t flag, bool state) {
	if (state) _cpu.flags |= flag;
	else _cpu.flags &= ~flag;
}

inline
void unsetFlag(mos6502& _cpu, uint8_t flag) {
	_cpu.flags &= ~flag;
}

inline
bool testFlag(mos6502& _cpu, uint8_t flag) {
	return _cpu.flags & flag;
}

inline
void donz(mos6502& _cpu, uint8_t val) {
	setFlag(_cpu, FLAGS.N, val & 0x80);
	setFlag(_cpu, FLAGS.Z, !val);
}

inline
void donzc(mos6502& _cpu, uint16_t val) {
	setFlag(_cpu, FLAGS.N, val & 0x80);
	setFlag(_cpu, FLAGS.Z, !(val & 0xff));
	setFlag(_cpu, FLAGS.C, val & 0x100);
}

/*
###################################--- INTERUPT FUNCTIONS ---#######################################
*/

#define IRQ_VEC 0xFFFE
#define NMI_VEC 0xFFFA
#define BRK_VEC 0xFFFE
#define RST_VEC 0xFFFC

void triggerNMI(mos6502& _cpu) {
	_cpu.PC += 2;
	push(_cpu, _cpu.PC >> 8);
	push(_cpu, _cpu.PC & 0xf);
	push(_cpu, _cpu.flags);
	setFlag(_cpu, FLAGS.I);
	_cpu.PC = basicRead(_cpu, NMI_VEC);
	_cpu.PC |= basicRead(_cpu, NMI_VEC + 1) << 8;
}
void triggerRST(mos6502& _cpu) {
	_cpu.PC += 2;
	push(_cpu, _cpu.PC >> 8);
	push(_cpu, _cpu.PC & 0xf);
	push(_cpu, _cpu.flags);
	setFlag(_cpu, FLAGS.I);
	_cpu.PC = basicRead(_cpu, RST_VEC);
	_cpu.PC |= basicRead(_cpu, RST_VEC + 1) << 8;
}
void triggerIRQ(mos6502& _cpu) {
	_cpu.PC += 2;
	push(_cpu, _cpu.PC >> 8);
	push(_cpu, _cpu.PC & 0xf);
	push(_cpu, _cpu.flags);
	setFlag(_cpu, FLAGS.I);
	_cpu.PC = basicRead(_cpu, IRQ_VEC);
	_cpu.PC |= basicRead(_cpu, IRQ_VEC + 1) << 8;
}

/*
###################################--- ADDRESS MODES ---#######################################
*/

uint16_t abs(mos6502& _cpu) {
	uint16_t address = basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8);
	_cpu.PC += 2;
	return address;

}

uint8_t accRead(mos6502& _cpu) {
	return _cpu.A;
}

void accWrite(mos6502& _cpu, uint8_t val) {
	_cpu.A = val;
}

uint16_t absx(mos6502& _cpu) {
	uint16_t ret = (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X;
	_cpu.PC += 2;
	return ret;
}

uint16_t absy(mos6502& _cpu) {
	uint16_t ret = (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.Y;
	_cpu.PC += 2;
	return ret;
}

uint16_t imm(mos6502& _cpu) {
	return _cpu.PC++;
}

uint16_t ind(mos6502& _cpu) {
	uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
	uint16_t v = basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8);
	_cpu.PC += 2;
	return v;
}

uint16_t xind(mos6502& _cpu) {
	uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X;
	uint16_t v = basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8);
	_cpu.PC += 2;
	return v;
}

uint16_t indy(mos6502& _cpu) {
	uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
	uint16_t v = (uint8_t)(basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8) + _cpu.Y);
	_cpu.PC += 2;
	return v;
}

uint16_t inline zpg(mos6502& _cpu) {
	uint16_t val = basicRead(_cpu, _cpu.PC);
	_cpu.PC += 1;
	return val;
}

uint16_t inline zpgx(mos6502& _cpu) {
	uint16_t val = (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.X);
	_cpu.PC += 1;
	return val;
}

uint16_t inline zpgy(mos6502& _cpu) {
	uint16_t val =  (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.Y);
	_cpu.PC += 1;
	return val;
}

uint16_t inline rel(mos6502& _cpu) {
	return (int8_t)basicRead(_cpu, _cpu.PC++) + _cpu.PC;
}

/*
###################################--- INSTRUCTIONS ---#######################################
*/

template <int clockcycles>
int nop(mos6502& _cpu) {
	if (clockcycles == 0) {
		printf("nop or undefined at %X\n", _cpu.PC);
		getchar();
	}
	return clockcycles;
}

template <int clockcycles>
int BRK(mos6502& _cpu) {
	_cpu.PC += 2;
	push(_cpu, _cpu.PC >> 8);
	push(_cpu, _cpu.PC & 0xf);
	setFlag(_cpu, FLAGS.B);
	push(_cpu, _cpu.flags);
	setFlag(_cpu, FLAGS.I);
	return clockcycles;
	//exit(1);
}//7 cycles ----------------IMPLEMENT THIS WHEN I KNOW WHAT IT DOES---------------------------

template <int clockcycles>
int PHP(mos6502& _cpu) {
	push(_cpu, _cpu.flags);
	return clockcycles;
}//3 cycles

template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BPL(mos6502& _cpu) {
	uint16_t loc = readPrim(_cpu);
	if (!testFlag(_cpu, FLAGS.N)) _cpu.PC = loc;
	return clockcycles;
}//2+(1 or 2 - depending on if in block or not) cycles

template <int clockcycles>
int CLC(mos6502& _cpu) {
	unsetFlag(_cpu, FLAGS.C);
	return clockcycles;
}// 2cycles

template<uint16_t(addMode)(mos6502&), int clockcycles>
int ORA(mos6502& _cpu) {
	uint8_t v = _cpu.A | basicRead(_cpu, addMode(_cpu));
	_cpu.A = v;
	donz(_cpu, v);
	return clockcycles;
}//4+1 cycles

template<uint16_t(addMode)(mos6502&), int clockcycles>
int ASL(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	uint16_t v = basicRead(_cpu, address) << 1;
	basicWrite(_cpu, address, (uint8_t)v);
	donzc(_cpu, v);
	return clockcycles;
}// 7 cycles

template<int clockcycles>
int ASLA(mos6502& _cpu) {
	uint16_t v = _cpu.A << 1;
	_cpu.A = (uint8_t)v;
	donzc(_cpu, v);
	return clockcycles;
}// 7 cycles

template<uint16_t(readPrim)(mos6502&), int clockcycles>
int JSR(mos6502& _cpu) {
	uint16_t v = readPrim(_cpu);//not sure if this gets a 16 bit value llhh
	_cpu.PC += 2;
	push(_cpu, _cpu.PC & 0xf);
	push(_cpu, _cpu.PC >> 8);
	_cpu.PC = v;
	return clockcycles;
}// 6 cycles

template <int clockcycles>
int PLP(mos6502& _cpu) {
	_cpu.flags = pop(_cpu);
	return clockcycles;
}//4 cycles

template<uint16_t(addMode)(mos6502&), int clockcycles>
int BIT(mos6502& _cpu) {
	uint8_t res = _cpu.A & basicRead(_cpu, addMode(_cpu));
	donz(_cpu, res);
	setFlag(_cpu, FLAGS.V, res&0x40);
	return clockcycles;
}//4 cycles

template <uint16_t(ReadPrim)(mos6502&),int clockcycles>
int BMI(mos6502& _cpu) {
	uint16_t add = ReadPrim(_cpu);
	if (testFlag(_cpu, FLAGS.N)) _cpu.PC = add;
	return clockcycles;
}

template <int clockcycles>
int SEC(mos6502& _cpu) {
	setFlag(_cpu, FLAGS.C);
	return clockcycles;
}

template<uint16_t(addMode)(mos6502&), int clockcycles>
int AND(mos6502& _cpu) {
	uint8_t v = _cpu.A & basicRead(_cpu, addMode(_cpu));
	_cpu.A = v;
	donz(_cpu, v);
	return clockcycles;
}

template<uint16_t(addMode)(mos6502&), int clockcycles>
int ROL(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	uint8_t v = basicRead(_cpu, address);
	v = (v << 1) | (v >> 7);
	basicWrite(_cpu, address, v);
	donz(_cpu, v);
	setFlag(_cpu, FLAGS.C, v & 1);
	return clockcycles;
}
template<int clockcycles>
int ROLA(mos6502& _cpu) {
	uint8_t v = _cpu.A;
	v = (v << 1) | (v >> 7);
	_cpu.A = v;
	donz(_cpu, v);
	setFlag(_cpu, FLAGS.C, v & 1);
	return clockcycles;
}
template <int clockcycles>
int RTI(mos6502& _cpu) {
	//printf("rti###################################################");
	_cpu.flags = pop(_cpu);
	_cpu.PC = (pop(_cpu) << 8) | pop(_cpu);
	return clockcycles;
}

template <int clockcycles>
int PHA(mos6502& _cpu) {
	push(_cpu, _cpu.A);
	return clockcycles;
}

template<uint16_t(readPrim)(mos6502&), int clockcycles>
int JMP(mos6502& _cpu) {
	_cpu.PC = readPrim(_cpu);
	return clockcycles;
}

template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BVC(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (!testFlag(_cpu, FLAGS.V)) _cpu.PC = add;
	return clockcycles;
}

template<uint16_t(addMode)(mos6502&), int clockcycles>
int LSR(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	uint8_t rval = basicRead(_cpu, address);
	uint8_t wval = rval >> 1;
	basicWrite(_cpu, address, wval);
	donz(_cpu, wval);
	setFlag(_cpu, FLAGS.C, rval & 1);
	return clockcycles;
}

template<int clockcycles>
int LSRA(mos6502& _cpu) {
	uint8_t rval = _cpu.A;
	uint8_t wval = rval >> 1;
	_cpu.A = wval;
	donz(_cpu, wval);
	setFlag(_cpu, FLAGS.C, rval & 1);
	return clockcycles;
}

template <int clockcycles>
int CLI(mos6502& _cpu) {
	printf("cli ");
	unsetFlag(_cpu, FLAGS.I);
	return clockcycles;
}

template<uint16_t(addMode)(mos6502&), int clockcycles>
int EOR(mos6502& _cpu) {
	_cpu.A ^= basicRead(_cpu, addMode(_cpu));
	donz(_cpu, _cpu.A);
	return clockcycles;
}

template <int clockcycles>
int RTS(mos6502& _cpu) {
	_cpu.PC = (pop(_cpu)<<8) + pop(_cpu);
	return clockcycles;
}

template<uint16_t(addMode)(mos6502&), int clockcycles>
int ADC(mos6502& _cpu) {
	uint8_t v = basicRead(_cpu, addMode(_cpu));
	uint8_t mayover = ~(v^_cpu.A);
	uint16_t total = v + _cpu.A + (testFlag(_cpu, FLAGS.C)?1:0);
	_cpu.A = (uint8_t)total;
	donzc(_cpu, total);
	setFlag(_cpu, FLAGS.V, mayover&(v^_cpu.A)&0x80);
	//printf("%ddi", ((total >> 8) & 1));
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int ROR(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	uint8_t v = basicRead(_cpu, address);
	v = (v >> 1) | (v << 7);
	basicWrite(_cpu, address, v);
	donz(_cpu, v);
	setFlag(_cpu, FLAGS.C, v & 0x80);
	return clockcycles;
}
template<int clockcycles>
int RORA(mos6502& _cpu) {
	uint8_t v = _cpu.A;
	v = (v >> 1) | (v << 7);
	_cpu.A = v;
	donz(_cpu, v);
	setFlag(_cpu, FLAGS.C, v & 0x80);
	return clockcycles;
}
template <int clockcycles>
int PLA(mos6502& _cpu) {
	_cpu.A = pop(_cpu);
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BVS(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (testFlag(_cpu, FLAGS.V)) _cpu.PC = add;

	return clockcycles;
}
template <int clockcycles>
int SEI(mos6502& _cpu) {
	setFlag(_cpu, FLAGS.I);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int STA(mos6502& _cpu) {
	basicWrite(_cpu, addMode(_cpu), _cpu.A);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int STX(mos6502& _cpu) {
	basicWrite(_cpu, addMode(_cpu), _cpu.X);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int STY(mos6502& _cpu) {
	basicWrite(_cpu, addMode(_cpu), _cpu.Y);
	return clockcycles;
}
template <int clockcycles>
int DEY(mos6502& _cpu) {
	_cpu.Y--;
	donz(_cpu, _cpu.Y);
	return clockcycles;
}
template <int clockcycles>
int TXA(mos6502& _cpu) {
	_cpu.A = _cpu.X;
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template <int clockcycles>
int TYA(mos6502& _cpu) {
	_cpu.A = _cpu.Y;
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template <int clockcycles>
int TXS(mos6502& _cpu) {
	_cpu.SP = _cpu.X;
	return clockcycles;
}
template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BCC(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (!testFlag(_cpu, FLAGS.C)) { _cpu.PC = add; }
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int LDY(mos6502& _cpu) {
	_cpu.Y = basicRead(_cpu, addMode(_cpu));
	donz(_cpu, _cpu.Y);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int LDA(mos6502& _cpu) {
	_cpu.A = basicRead(_cpu, addMode(_cpu));
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int LDX(mos6502& _cpu) {
	_cpu.X = basicRead(_cpu, addMode(_cpu));
	donz(_cpu, _cpu.X);
	return clockcycles;
}
template <int clockcycles>
int TAY(mos6502& _cpu) {
	_cpu.Y = _cpu.A;
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template <int clockcycles>
int CLV(mos6502& _cpu) {
	unsetFlag(_cpu, FLAGS.V);
	return clockcycles;
}
template <int clockcycles>
int TAX(mos6502& _cpu) {
	_cpu.X = _cpu.A;
	donz(_cpu, _cpu.A);
	return clockcycles;
}
template <int clockcycles>
int TSX(mos6502& _cpu) {
	_cpu.X = _cpu.SP;
	donz(_cpu, _cpu.X);
	return clockcycles;
}
template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BCS(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (testFlag(_cpu, FLAGS.C)) { _cpu.PC = add; }
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int CMP(mos6502& _cpu) {
	uint16_t v = _cpu.A - basicRead(_cpu, addMode(_cpu));
	donzc(_cpu, v);
	return clockcycles;
}
template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BNE(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (!testFlag(_cpu, FLAGS.Z)) { _cpu.PC = add; }
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int CPY(mos6502& _cpu) {
	uint16_t v = _cpu.Y - basicRead(_cpu, addMode(_cpu));
	donzc(_cpu, v);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int DEC(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	uint8_t val = basicRead(_cpu, address) - 1;
	basicWrite(_cpu, address, val);
	donz(_cpu, val);
	return clockcycles;
}
template <int clockcycles>
int DEX(mos6502& _cpu) {
	_cpu.X--;
	donz(_cpu, _cpu.X);
	return clockcycles;
}
template <int clockcycles>
int INY(mos6502& _cpu) {
	_cpu.Y++;
	donz(_cpu, _cpu.Y);
	return clockcycles;
}
template <int clockcycles>
int CLD(mos6502& _cpu) {
	unsetFlag(_cpu, FLAGS.D);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int CPX(mos6502& _cpu) {
	uint16_t v = _cpu.X - basicRead(_cpu, addMode(_cpu));
	donzc(_cpu, v);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int SBC(mos6502& _cpu) {
	uint8_t v = ~basicRead(_cpu, addMode(_cpu));
	uint8_t mayover = ~(v^_cpu.A);
	uint16_t val = _cpu.A + v + testFlag(_cpu, FLAGS.C)?1:0;
	_cpu.A = (uint8_t)val;
	donzc(_cpu, val);
	setFlag(_cpu, FLAGS.V, mayover&(v^_cpu.A)&0x80);
	return clockcycles;
}
template<uint16_t(addMode)(mos6502&), int clockcycles>
int INC(mos6502& _cpu) {
	uint16_t address = addMode(_cpu);
	basicWrite(_cpu, address, basicRead(_cpu, address)+1);
	return clockcycles;
}
template<uint16_t(readPrim)(mos6502&), int clockcycles>
int BEQ(mos6502& _cpu) {
	uint16_t add = readPrim(_cpu);
	if (FLAGS.Z == 1) { _cpu.PC = add; }
	return clockcycles;
}
template <int clockcycles>
int INX(mos6502& _cpu) {
	_cpu.X++;
	donz(_cpu, _cpu.X);
	return clockcycles;
}
template <int clockcycles>
int SED(mos6502& _cpu) {
	setFlag(_cpu, FLAGS.D);
	return clockcycles;
}

static int (*cpuopmap[256])(mos6502&) = {
	//  0      , 1            , 2          , 3     , 4           , 5           , 6           , 7     , 8     , 9           , A          , B     , C           , D           , E           , F
	BRK<7>     , ORA<xind , 6>, nop<0>     , nop<0>, nop<0>      , ORA<zpg, 3> , ASL<zpg, 5> , nop<0>, PHP<3>, ORA<imm, 2> , ASLA<2>, nop<0>, nop<0>      , ORA<abs, 4> , ASL<abs, 6> , nop<0>,//0
	BPL<rel, 2>, ORA<indy , 5>, nop<0>     , nop<0>, nop<0>      , ORA<zpgx, 4>, ASL<zpgx, 6>, nop<0>, CLC<2>, ORA<absy, 4>, nop<0> , nop<0>, nop<0>      , ORA<absx, 4>, ASL<absx, 7>, nop<0>,//1
	JSR<abs, 6>, AND<xind , 6>, nop<0>     , nop<0>, BIT<zpg, 3> , AND<zpg, 3> , ROL<zpg, 5> , nop<0>, PLP<4>, AND<imm, 2> , ROLA<2>, nop<0>, BIT<abs, 4> , AND<abs, 4> , ROL<abs, 6> , nop<0>,//2
	BMI<rel, 2>, AND<indy , 5>, nop<0>     , nop<0>, nop<0>      , AND<zpgx, 4>, ROL<zpgx, 6>, nop<0>, SEC<2>, AND<absy, 4>, nop<0> , nop<0>, nop<0>      , AND<absx, 4>, ROL<absx, 7>, nop<0>,//3
	RTI<6>     , EOR<xind , 6>, nop<0>     , nop<0>, nop<0>      , EOR<zpg, 3> , LSR<zpg, 5> , nop<0>, PHA<3>, EOR<imm, 2> , LSRA<2>, nop<0>, JMP<abs, 3> , EOR<abs, 4> , LSR<abs, 6> , nop<0>,//4
	BVC<rel, 2>, EOR<indy , 5>, nop<0>     , nop<0>, nop<0>      , EOR<zpgx, 4>, LSR<zpgx, 6>, nop<0>, CLI<2>, EOR<absy, 4>, nop<0> , nop<0>, nop<0>      , EOR<absx, 4>, LSR<absx, 7>, nop<0>,//5
	RTS<6>     , ADC<xind , 6>, nop<0>     , nop<0>, nop<0>      , ADC<zpg, 3> , ROR<zpg, 5> , nop<0>, PLA<4>, ADC<imm, 2> , RORA<2>, nop<0>, JMP<ind, 5> , ADC<abs, 4> , ROR<abs, 6> , nop<0>,//6
	BVS<rel, 2>, ADC<indy , 5>, nop<0>     , nop<0>, nop<0>      , ADC<zpgx, 4>, ROR<zpgx, 6>, nop<0>, SEI<2>, ADC<absy, 4>, nop<0> , nop<0>, nop<0>      , ADC<absx, 4>, ROR<absx, 7>, nop<0>,//7
	nop<0>     , STA<xind , 6>, nop<0>     , nop<0>, STY<zpg, 3> , STA<zpg, 3> , STX<zpg, 3> , nop<0>, DEY<2>, nop<0>      , TXA<2> , nop<0>, STY<abs, 3> , STA<abs, 4> , STX<abs, 4> , nop<0>,//8
	BCC<rel, 2>, STA<indy , 6>, nop<0>     , nop<0>, STY<zpgx, 4>, STA<zpgx, 4>, STX<zpgy, 4>, nop<0>, TYA<2>, STA<absy, 5>, TXS<2> , nop<0>, nop<0>      , STA<absx, 5>, nop<0>      , nop<0>,//9
	LDY<imm, 2>, LDA<xind , 6>, LDX<imm, 2>, nop<0>, LDY<zpg, 3> , LDA<zpg, 3> , LDX<zpg, 3> , nop<0>, TAY<2>, LDA<imm, 2> , TAX<2> , nop<0>, LDY<abs, 4> , LDA<abs, 4> , LDX<abs, 4> , nop<0>,//a
	BCS<rel, 2>, LDA<indy , 5>, nop<0>     , nop<0>, LDY<zpgx, 4>, LDA<zpgx, 4>, LDX<zpgy, 4>, nop<0>, CLV<2>, LDA<absy, 4>, TSX<2> , nop<0>, LDY<absx, 4>, LDA<absx, 4>, LDX<absy, 4>, nop<0>,//b
	CPY<imm, 2>, CMP<xind , 6>, nop<0>     , nop<0>, CPY<zpg, 4> , CMP<zpg, 3> , DEC<zpg, 5> , nop<0>, INY<2>, CMP<imm, 2> , DEX<2> , nop<0>, CPY<abs, 4> , CMP<abs, 4> , DEC<abs, 3> , nop<0>,//c
	BNE<rel, 2>, CMP<indy , 5>, nop<0>     , nop<0>, nop<0>      , CMP<zpgx, 4>, DEC<zpg, 6> , nop<0>, CLD<2>, CMP<absy, 4>, nop<0> , nop<0>, nop<0>      , CMP<absx, 4>, DEC<absx, 7>, nop<0>,//d
	CPX<imm, 2>, SBC<xind , 6>, nop<0>     , nop<0>, CPX<zpg, 3> , SBC<zpg, 3> , INC<zpg, 5> , nop<0>, INX<2>, SBC<imm, 2> , nop<2> , nop<0>, CPX<abs, 4> , SBC<abs, 4> , INC<abs, 6> , nop<0>,//e
	BEQ<rel, 2>, SBC<indy , 5>, nop<0>     , nop<0>, nop<0>      , SBC<zpgx, 4>, INC<zpgx, 6>, nop<0>, CLV<2>, SBC<absy, 4>, nop<0> , nop<0>, nop<0>      , SBC<absx, 4>, INC<absx, 7>, nop<0>,//f
};

int stepCpu(mos6502& _cpu) {
	printf("running instruction from 0x%04X\n", _cpu.PC);
	uint8_t opcode = basicRead(_cpu, _cpu.PC++);
	printf("running 0x%02X\n", opcode);
	return cpuopmap[opcode](_cpu);
}