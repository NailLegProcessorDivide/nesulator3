#include "cpu.h"

#include <stdio.h>

namespace J6502{
	/*
	###################################--- FLAG FUNCTIONS ---#######################################
	*/

	const static struct _FLAGS {
		uint8_t N=0b10000000;
		uint8_t V=0b01000000;
		uint8_t B=0b00010000;
		uint8_t D=0b00001000;
		uint8_t I=0b00000100;
		uint8_t Z=0b00000010;
		uint8_t C=0b00000001;
	}FLAGS;

	inline
	void setFlag(cpu6502& _cpu, uint8_t flag) {
		_cpu.Flags |= flag;
	}

	inline
	void setFlag(cpu6502& _cpu, uint8_t flag, bool state) {
		if(state) _cpu.Flags |= flag;
		else _cpu.Flags &= ~flag;
	}

	inline
	void unsetFlag(cpu6502& _cpu, uint8_t flag) {
		_cpu.Flags &= ~flag;
	}

	inline
	bool testFlag(cpu6502& _cpu, uint8_t flag) {
		return _cpu.Flags & flag;
	}

	inline
	void donz(cpu6502& _cpu, uint8_t val) {
		setFlag(_cpu, FLAGS.N, val & 0x80);
		setFlag(_cpu, FLAGS.Z, !val);
	}

	inline
	void donzc(cpu6502& _cpu, uint16_t val) {
		setFlag(_cpu, FLAGS.N, val & 0x80);
		setFlag(_cpu, FLAGS.Z, !(val&0xff));
		setFlag(_cpu, FLAGS.C, val & 0x100);
	}

	/*
	###################################--- BASIC READ/WRITE ---#######################################
	*/

	uint8_t basicRead(cpu6502& _cpu, uint16_t address) {
		for (size_t i = 0; i < _cpu.deviceCount; i++) {
			device& dev = _cpu.devices[i];
			if (dev.start <= address && dev.start+dev.length > address) {
				return dev.readfun(dev.data, address-dev.start);
			}
		}
		return 0;
	}

	void basicWrite(cpu6502& _cpu, uint16_t address, uint8_t value){
		for (size_t i = 0; i < _cpu.deviceCount; i++) {
			device& dev = _cpu.devices[i];
			if (dev.start <= address && dev.start+dev.length > address) {
				dev.writefun(dev.data, address-dev.start, value);
			}
		}
	}

	void push(cpu6502& _cpu, uint8_t value){
		basicWrite(_cpu, 0x100 + _cpu.SP--, value);
	}

	uint8_t pop(cpu6502 _cpu) {
		return basicRead(_cpu, 0x100 + ++_cpu.SP);
	}

	/*
	###################################--- ADDRESS MODES ---#######################################
	*/

	uint8_t absRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("absr\n");
	#endif
		uint8_t ret = basicRead(_cpu, basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		_cpu.PC += 2;
		return ret;

	}

	void absWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("abse\n");
	#endif
		basicWrite(_cpu, basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8), val);
		_cpu.PC += 2;

	}

	uint8_t accRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("accr\n");
	#endif
		return _cpu.A;
	}

	void accWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("accw\n");
	#endif
		_cpu.A = val;
	}

	uint8_t absxRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("absxr\n");
	#endif
		uint8_t ret = basicRead(_cpu, (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X);
		_cpu.PC += 2;
		return ret;
	}

	void absxWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("absxw\n");
	#endif
		basicWrite(_cpu, (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X, val);
		_cpu.PC += 2;
	}

	uint8_t absyRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("absxr\n");
	#endif
		uint8_t ret = basicRead(_cpu, (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.Y);
		_cpu.PC += 2;
		return ret;
	}

	void absyWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("absxw\n");
	#endif
		basicWrite(_cpu, (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.Y, val);
		_cpu.PC += 2;
	}

	uint16_t abs16(cpu6502& _cpu) {
		return basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8);
	}

	uint8_t imm(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("imm\n");
	#endif
		return basicRead(_cpu, _cpu.PC++);
	}

	uint8_t indRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("ind");
	#endif
		uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		uint8_t v =  basicRead(_cpu, basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8));
		_cpu.PC += 2;
		return v;
	}

	void indWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("ind");
	#endif
		uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		basicWrite(_cpu, basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8), val);
		_cpu.PC += 2;
	}

	uint8_t xindRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("xindr");
	#endif
		uint8_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X;
		uint8_t v =  basicRead(_cpu, basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8));
		_cpu.PC += 2;
		return v;
	}

	void xindWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("xinw");
	#endif
		uint8_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8)) + _cpu.X;
		basicWrite(_cpu, basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8), val);
		_cpu.PC += 2;
	}

	uint8_t indyRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("indyr");
	#endif
		uint8_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		uint8_t v =  basicRead(_cpu, (uint8_t)(basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8) + _cpu.Y));
		_cpu.PC += 2;
		return v;
	}

	void indyWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("indyw");
	#endif
		uint8_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		basicWrite(_cpu, (uint8_t)(basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8) + _cpu.Y), val);
		_cpu.PC += 1;
	}

	uint16_t inline ind16(cpu6502& _cpu) {
		uint16_t address =  (basicRead(_cpu, _cpu.PC) | (basicRead(_cpu, _cpu.PC+1)<<8));
		return basicRead(_cpu, address) | (basicRead(_cpu, address+1)<<8);
	}

	uint8_t inline zpgRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("zpgr");
	#endif
		uint8_t val =  basicRead(_cpu, basicRead(_cpu, _cpu.PC));
		_cpu.PC += 1;
		return val;
	}

	void inline zpgWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("zpgw");
	#endif
		basicWrite(_cpu, basicRead(_cpu, _cpu.PC), val);
		_cpu.PC += 1;
	}

	uint8_t inline zpgxRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("zpgxr");
	#endif
		uint8_t val =  basicRead(_cpu, (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.X));
		_cpu.PC += 1;
		return val;
	}

	void inline zpgxWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("zpgxw");
	#endif
		basicWrite(_cpu, (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.X), val);
		_cpu.PC += 1;
	}

	uint8_t inline zpgyRead(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("zpgxr");
	#endif
		uint8_t val =  basicRead(_cpu, (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.Y));
		_cpu.PC += 1;
		return val;
	}

	void inline zpgyWrite(cpu6502& _cpu, uint8_t val) {
	#ifdef debugPrint
		printf("zpgxw");
	#endif
		basicWrite(_cpu, (uint8_t)(basicRead(_cpu, _cpu.PC)+_cpu.Y), val);
		_cpu.PC += 1;
	}

	uint16_t inline rel(cpu6502& _cpu) {
	#ifdef debugPrint
		printf("rel\n");
	#endif
		return (int8_t)basicRead(_cpu, _cpu.PC++) + _cpu.PC;
	}

	/*
	###################################--- INSTRUCTIONS ---#######################################
	*/

	template <int clockcycles>
	int nop(cpu6502& _cpu) {
		if (clockcycles == 0) {
			printf("nop or undefined at %X\n", _cpu.PC);
			getchar();
		}
		return clockcycles;
	}

	template <int clockcycles>
	int BRK(cpu6502& _cpu) {
		_cpu.Interupts |= 2;
		_cpu.PC += 2;
		push(_cpu, _cpu.PC & 0xf);
		push(_cpu, _cpu.PC >> 8);
		push(_cpu, _cpu.Flags);
		setFlag(_cpu, FLAGS.B);
		return clockcycles;
		//exit(1);
	}//7 cycles ----------------IMPLEMENT THIS WHEN I KNOW WHAT IT DOES---------------------------

	template <int clockcycles>
	int PHP(cpu6502& _cpu) {
		push(_cpu, _cpu.Flags);
		return clockcycles;
	}//3 cycles

	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BPL(cpu6502& _cpu) {
		uint16_t loc = readPrim(_cpu);
		if (!testFlag(_cpu, FLAGS.N)) _cpu.PC = loc;
		return clockcycles;
	}//2+(1 or 2 - depending on if in block or not) cycles

	template <int clockcycles>
	int CLC(cpu6502& _cpu) {
		unsetFlag(_cpu, FLAGS.C);
		return clockcycles;
	}// 2cycles

	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int ORA(cpu6502& _cpu) {
		uint8_t v = _cpu.A | readPrim(_cpu);
		_cpu.A = v;
		donz(_cpu, v);
		return clockcycles;
	}//4+1 cycles

	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int ASL(cpu6502& _cpu) {
		uint16_t v = readPrim(_cpu) << 1;
		writePrim(_cpu, (uint8_t)v);
		donzc(_cpu, v);
		return clockcycles;
	}// 7 cycles

	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int JSR(cpu6502& _cpu) {
		uint16_t v = readPrim(_cpu);//not sure if this gets a 16 bit value llhh
		_cpu.PC += 2;
		push(_cpu, _cpu.PC & 0xf);
		push(_cpu, _cpu.PC >> 8);
		_cpu.PC = v;
		return clockcycles;
	}// 6 cycles

	template <int clockcycles>
	int PLP(cpu6502& _cpu) {
		_cpu.Flags = pop(_cpu);
		return clockcycles;
	}//4 cycles

	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int BIT(cpu6502& _cpu) {
		uint8_t res = _cpu.A & readPrim(_cpu);
		donz(_cpu, res);
		setFlag(_cpu, FLAGS.V, res&0x40);
		return clockcycles;
	}//4 cycles

	template <uint16_t(ReadPrim)(cpu6502&),int clockcycles>
	int BMI(cpu6502& _cpu) {
		uint16_t add = ReadPrim(_cpu);
		if (testFlag(_cpu, FLAGS.N)) _cpu.PC = add;
		return clockcycles;
	}

	template <int clockcycles>
	int SEC(cpu6502& _cpu) {
		printf("sec ");
		setFlag(_cpu, FLAGS.C);
		return clockcycles;
	}

	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int AND(cpu6502& _cpu) {
		printf("and ");
		uint8_t v = _cpu.A & readPrim(_cpu);
		_cpu.A = v;
		donz(_cpu, v);
		return clockcycles;
	}

	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int ROL(cpu6502& _cpu) {
		printf("rol ");
		uint8_t v = readPrim(_cpu);
		v = (v << 1) | (v >> 7);
		writePrim(_cpu, v);
		donz(_cpu, v);
		setFlag(_cpu, FLAGS.C, v&1);
		return clockcycles;
	}

	template <int clockcycles>
	int RTI(cpu6502& _cpu) {
		printf("rti ");
		//printf("rti###################################################");
		_cpu.Flags = pop(_cpu);
		_cpu.PC = (pop(_cpu) << 8) | pop(_cpu);
		return clockcycles;
	}

	template <int clockcycles>
	int PHA(cpu6502& _cpu) {
		printf("pha ");
		push(_cpu, _cpu.A);
		return clockcycles;
	}

	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int JMP(cpu6502& _cpu) {
		printf("jmp ");
		_cpu.PC = readPrim(_cpu);
		return clockcycles;
	}

	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BVC(cpu6502& _cpu) {
		printf("bvc ");
		uint16_t add = readPrim(_cpu);
		if (!testFlag(_cpu, FLAGS.V)) _cpu.PC = add;
		return clockcycles;
	}

	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int LSR(cpu6502& _cpu) {
		printf("lsr ");
		uint8_t rval = readPrim(_cpu);
		uint8_t wval = rval>>1;
		writePrim(_cpu, wval);
		donz(_cpu, wval);
		setFlag(_cpu, FLAGS.C, rval&1);
		return clockcycles;
	}

	template <int clockcycles>
	int CLI(cpu6502& _cpu) {
		printf("cli ");
		unsetFlag(_cpu, FLAGS.I);
		return clockcycles;
	}

	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int EOR(cpu6502& _cpu) {
		printf("eor ");
		_cpu.A ^= readPrim(_cpu);
		donz(_cpu, _cpu.A);
		return clockcycles;
	}

	template <int clockcycles>
	int RTS(cpu6502& _cpu) {
		printf("rts ");
		_cpu.PC = (pop(_cpu)<<8) + pop(_cpu);
		return clockcycles;
	}

	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int ADC(cpu6502& _cpu) {
		printf("adc ");
		uint8_t v = readPrim(_cpu);
		uint8_t mayover = ~(v^_cpu.A);
		uint16_t total = v + _cpu.A + (testFlag(_cpu, FLAGS.C)?1:0);
		_cpu.A = (uint8_t)total;
		donzc(_cpu, total);
		setFlag(_cpu, FLAGS.V, mayover&(v^_cpu.A)&0x80);
		//printf("%ddi", ((total >> 8) & 1));
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int ROR(cpu6502& _cpu) {
		printf("ror ");
		uint8_t v = readPrim(_cpu);
		v = (v >> 1) | (v << 7);
		writePrim(_cpu, v);
		donz(_cpu, v);
		setFlag(_cpu, FLAGS.C, v&0x80);
		return clockcycles;
	}
	template <int clockcycles>
	int PLA(cpu6502& _cpu) {
		printf("pla ");
		_cpu.A = pop(_cpu);
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BVS(cpu6502& _cpu) {
		printf("bvs ");
		uint16_t add = readPrim(_cpu);
		if (testFlag(_cpu, FLAGS.V)) _cpu.PC = add;

		return clockcycles;
	}
	template <int clockcycles>
	int SEI(cpu6502& _cpu) {
		printf("sei ");
		setFlag(_cpu, FLAGS.I);
		return clockcycles;
	}
	template<void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int STA(cpu6502& _cpu) {
		printf("sta ");
		writePrim(_cpu, _cpu.A);
		return clockcycles;
	}
	template<void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int STX(cpu6502& _cpu) {
		printf("stx ");
		writePrim(_cpu, _cpu.X);
		return clockcycles;
	}
	template<void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int STY(cpu6502& _cpu) {
		printf("sty ");
		writePrim(_cpu, _cpu.Y);
		return clockcycles;
	}
	template <int clockcycles>
	int DEY(cpu6502& _cpu) {
		printf("dey ");
		_cpu.Y--;
		donz(_cpu, _cpu.Y);
		return clockcycles;
	}
	template <int clockcycles>
	int TXA(cpu6502& _cpu) {
		printf("txa ");
		_cpu.A = _cpu.X;
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template <int clockcycles>
	int TYA(cpu6502& _cpu) {
		printf("tya ");
		_cpu.A = _cpu.Y;
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template <int clockcycles>
	int TXS(cpu6502& _cpu) {
		printf("txs ");
		_cpu.SP = _cpu.X;
		return clockcycles;
	}
	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BCC(cpu6502& _cpu) {
		printf("bcc ");
		uint16_t add = readPrim(_cpu);
		if (!testFlag(_cpu, FLAGS.C)) { _cpu.PC = add; }

		if (_cpu.PC < 0x4000) {
			printf("%i pc has weird value.", _cpu.PC);
			while (true);
		}
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int LDY(cpu6502& _cpu) {
		printf("ldy ");
		_cpu.Y = readPrim(_cpu);
		donz(_cpu, _cpu.Y);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int LDA(cpu6502& _cpu) {
		printf("lda ");
		_cpu.A = readPrim(_cpu);
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int LDX(cpu6502& _cpu) {
		printf("ldx ");
		_cpu.X = readPrim(_cpu);
		donz(_cpu, _cpu.X);
		return clockcycles;
	}
	template <int clockcycles>
	int TAY(cpu6502& _cpu) {
		printf("tay ");
		_cpu.Y = _cpu.A;
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template <int clockcycles>
	int CLV(cpu6502& _cpu) {
		printf("clv ");
		unsetFlag(_cpu, FLAGS.V);
		return clockcycles;
	}
	template <int clockcycles>
	int TAX(cpu6502& _cpu) {
		printf("tax ");
		_cpu.X = _cpu.A;
		donz(_cpu, _cpu.A);
		return clockcycles;
	}
	template <int clockcycles>
	int TSX(cpu6502& _cpu) {
		printf("tsx");
		_cpu.X = _cpu.SP;
		donz(_cpu, _cpu.X);
		return clockcycles;
	}
	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BCS(cpu6502& _cpu) {
		printf("bcs ");
		uint16_t add = readPrim(_cpu);
		if (testFlag(_cpu, FLAGS.C)) { _cpu.PC = add; }
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int CMP(cpu6502& _cpu) {
		printf("cmp ");
		uint16_t v = _cpu.A - readPrim(_cpu);
		donzc(_cpu, v);
		return clockcycles;
	}
	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BNE(cpu6502& _cpu) {
		printf("bne ");
		uint16_t add = readPrim(_cpu);
		if (!testFlag(_cpu, FLAGS.Z)) { _cpu.PC = add; }
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int CPY(cpu6502& _cpu) {
		printf("cpy ");
		uint16_t v = _cpu.Y - readPrim(_cpu);
		donzc(_cpu, v);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int DEC(cpu6502& _cpu) {
		printf("dec ");
		uint8_t val = readPrim(_cpu) - 1;
		writePrim(_cpu, val);
		donz(_cpu, val);
		return clockcycles;
	}
	template <int clockcycles>
	int DEX(cpu6502& _cpu) {
		printf("dex ");
		_cpu.X--;
		donz(_cpu, _cpu.X);
		return clockcycles;
	}
	template <int clockcycles>
	int INY(cpu6502& _cpu) {
		printf("iny ");
		_cpu.Y++;
		donz(_cpu, _cpu.Y);
		return clockcycles;
	}
	template <int clockcycles>
	int CLD(cpu6502& _cpu) {
		printf("cld");
		unsetFlag(_cpu, FLAGS.D);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int CPX(cpu6502& _cpu) {
		printf("cpx ");
		uint16_t v = _cpu.X - readPrim(_cpu);
		donzc(_cpu, v);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), int clockcycles>
	int SBC(cpu6502& _cpu) {
		printf("sbc ");
		uint8_t v = ~readPrim(_cpu);
		uint8_t mayover = ~(v^_cpu.A);
		uint16_t val = _cpu.A + v + testFlag(_cpu, FLAGS.C)?1:0;
		_cpu.A = (uint8_t)val;
		donzc(_cpu, val);
		setFlag(_cpu, FLAGS.V, mayover&(v^_cpu.A)&0x80);
		return clockcycles;
	}
	template<uint8_t(readPrim)(cpu6502&), void(writePrim)(cpu6502&, uint8_t), int clockcycles>
	int INC(cpu6502& _cpu) {
		printf("inc ");
		writePrim(_cpu, readPrim(_cpu)+1);
		return clockcycles;
	}
	template<uint16_t(readPrim)(cpu6502&), int clockcycles>
	int BEQ(cpu6502& _cpu) {
		printf("beq ");
		uint16_t add = readPrim(_cpu);
		if (FLAGS.Z == 1) { _cpu.PC = add; }
		return clockcycles;
	}
	template <int clockcycles>
	int INX(cpu6502& _cpu) {
		printf("inx ");
		_cpu.X++;
		donz(_cpu, _cpu.X);
		return clockcycles;
	}
	template <int clockcycles>
	int SED(cpu6502& _cpu) {
		printf("sed ");
		setFlag(_cpu, FLAGS.D);
		return clockcycles;
	}

	static int (*cpu6502opmap[256])(cpu6502&) = {
		//  0        , 1                , 2          , 3     , 4                , 5                , 6                          , 7     , 8     , 9                , A                        , B     , C               , D                , E                          , F
		BRK<7>       , ORA<xindRead , 6>, nop<0>     , nop<0>, nop<0>           , ORA<zpgRead, 3>  , ASL<zpgRead, zpgWrite, 5>  , nop<0>, PHP<3>, ORA<imm, 2>      , ASL<accRead, accWrite, 2>, nop<0>, nop<0>          , ORA<absRead, 4>  , ASL<absRead, absWrite, 6>  , nop<0>,//0
		BPL<rel, 2>  , ORA<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , ORA<zpgxRead, 4> , ASL<zpgxRead, zpgxWrite, 6>, nop<0>, CLC<2>, ORA<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , ORA<absxRead, 4> , ASL<absxRead, absxWrite, 7>, nop<0>,//1
		JSR<abs16, 6>, AND<xindRead , 6>, nop<0>     , nop<0>, BIT<zpgRead, 3>  , AND<zpgRead, 3>  , ROL<zpgRead, zpgWrite, 5>  , nop<0>, PLP<4>, AND<imm, 2>      , ROL<accRead, accWrite, 2>, nop<0>, BIT<absRead, 4> , AND<absRead, 4>  , ROL<absRead, absWrite, 6>  , nop<0>,//2
		BMI<rel, 2>  , AND<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , AND<zpgxRead, 4> , ROL<zpgxRead, zpgxWrite, 6>, nop<0>, SEC<2>, AND<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , AND<absxRead, 4> , ROL<absxRead, absxWrite, 7>, nop<0>,//3
		RTI<6>       , EOR<xindRead , 6>, nop<0>     , nop<0>, nop<0>           , EOR<zpgRead, 3>  , LSR<zpgRead, zpgWrite, 5>  , nop<0>, PHA<3>, EOR<imm, 2>      , LSR<accRead, accWrite, 2>, nop<0>, JMP<abs16, 3>   , EOR<absRead, 4>  , LSR<absRead, absWrite, 6>  , nop<0>,//4
		BVC<rel, 2>  , EOR<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , EOR<zpgxRead, 4> , LSR<zpgxRead, zpgxWrite, 6>, nop<0>, CLI<2>, EOR<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , EOR<absxRead, 4> , LSR<absxRead, absxWrite, 7>, nop<0>,//5
		RTS<6>       , ADC<xindRead , 6>, nop<0>     , nop<0>, nop<0>           , ADC<zpgRead, 3>  , ROR<zpgRead, zpgWrite, 5>  , nop<0>, PLA<4>, ADC<imm, 2>      , ROR<accRead, accWrite, 2>, nop<0>, JMP<ind16, 5>   , ADC<absRead, 4>  , ROR<absRead, absWrite, 6>  , nop<0>,//6
		BVS<rel, 2>  , ADC<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , ADC<zpgxRead, 4> , ROR<zpgxRead, zpgxWrite, 6>, nop<0>, SEI<2>, ADC<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , ADC<absxRead, 4> , ROR<absxRead, absxWrite, 7>, nop<0>,//7
		nop<0>       , STA<xindWrite, 6>, nop<0>     , nop<0>, STY<zpgWrite, 3> , STA<zpgWrite, 3> , STX<zpgWrite, 3>           , nop<0>, DEY<2>, nop<0>           , TXA<2>                   , nop<0>, STY<absWrite, 3>, STA<absWrite, 4> , STX<absWrite, 4>           , nop<0>,//8
		BCC<rel, 2>  , STA<indyWrite, 6>, nop<0>     , nop<0>, STY<zpgxWrite, 4>, STA<zpgxWrite, 4>, STX<zpgyWrite, 4>          , nop<0>, TYA<2>, STA<absyWrite, 5>, TXS<2>                   , nop<0>, nop<0>          , STA<absxWrite, 5>, nop<0>                     , nop<0>,//9
		LDY<imm, 2>  , LDA<xindRead , 6>, LDX<imm, 2>, nop<0>, LDY<zpgRead, 3>  , LDA<zpgRead, 3>  , LDX<zpgRead, 3>            , nop<0>, TAY<2>, LDA<imm, 2>      , TAX<2>                   , nop<0>, LDY<absRead, 4> , LDA<absRead, 4>  , LDX<absRead, 4>            , nop<0>,//a
		BCS<rel, 2>  , LDA<indyRead , 5>, nop<0>     , nop<0>, LDY<zpgxRead, 4> , LDA<zpgxRead, 4> , LDX<zpgyRead, 4>           , nop<0>, CLV<2>, LDA<absyRead, 4> , TSX<2>                   , nop<0>, LDY<absxRead, 4>, LDA<absxRead, 4> , LDX<absyRead, 4>           , nop<0>,//b
		CPY<imm, 2>  , CMP<xindRead , 6>, nop<0>     , nop<0>, CPY<zpgRead, 4>  , CMP<zpgRead, 3>  , DEC<zpgRead, zpgWrite, 5>  , nop<0>, INY<2>, CMP<imm, 2>      , DEX<2>                   , nop<0>, CPY<absRead, 4> , CMP<absRead, 4>  , DEC<absRead, absWrite, 3>  , nop<0>,//c
		BNE<rel, 2>  , CMP<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , CMP<zpgxRead, 4> , DEC<zpgxRead, zpgxWrite, 6>, nop<0>, CLD<2>, CMP<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , CMP<absxRead, 4> , DEC<absxRead, absxWrite, 7>, nop<0>,//d
		CPX<imm, 2>  , SBC<xindRead , 6>, nop<0>     , nop<0>, CPX<zpgRead, 3>  , SBC<zpgRead, 3>  , INC<zpgRead, zpgxWrite, 5> , nop<0>, INX<2>, SBC<imm, 2>      , nop<2>                   , nop<0>, CPX<absRead, 4> , SBC<absRead, 4>  , INC<absRead, absWrite, 6>  , nop<0>,//e
		BEQ<rel, 2>  , SBC<indyRead , 5>, nop<0>     , nop<0>, nop<0>           , SBC<zpgxRead, 4> , INC<zpgxRead, zpgxWrite, 6>, nop<0>, CLV<2>, SBC<absyRead, 4> , nop<0>                   , nop<0>, nop<0>          , SBC<absxRead, 4> , INC<absxRead, absxWrite, 7>, nop<0>,//f
	};
}