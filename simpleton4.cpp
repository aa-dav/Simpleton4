#include "simpleton4.h"
#include <conio.h>

namespace Simpleton
{

/*static*/ bool Instruction::isInplaceImmediate( mTag cmd )
{
	return (cmd == OP_ADDI) || (cmd == OP_ADDIS) || (cmd == OP_RRCI);
}

static const char *NameCmds[] = {
"addis",
"addi ",
"adds ",
"add  ",
"adc  ",
"sub  ",
"sbc  ",
"and  ",
"or   ",
"xor  ",
"cadd ",
"rrci ",
"rrc  " };

static const char *NameRegs[] = {
"r0",
"r1",
"r2",
"r3",
"r4",
"sp",
"pc",
"pw",
"[ r0 ]",
"[ r1 ]",
"[ r2 ]",
"[ r3 ]",
"[ r4 ]",
"[ sp ]",
"[ pc ]",
"[ pw ]"
};

std::string Machine::operandToStr( mTag r, mTag i, int &addr, bool result )
{
	std::stringstream ss;
	if ( result && (i == 1) && (r == REG_PC) )
	{
		ss << "void";
	}
	else if ( (i == 1) && (r == REG_PC) )
	{
		ss << "$" << std::uppercase << std::hex << getMem( addr++ );
	}
	else if ( (i == 1) && (r == REG_PSW) )
	{
		ss << "[ $" << std::uppercase << std::hex << std::setw( 4 ) << std::setfill( '0' ) << getMem( addr++ ) << " ]";
	}
	else
	{
		ss << NameRegs[ i * 8 + r ];
	}
	return ss.str();
};

void Machine::showDisasm( int addr )
{
	Instruction instr;
	std::string sr, sy, sx;
	std::cout << std::uppercase << std::hex << std::setw( 4 ) << std::setfill( '0' ) << addr  << ": ";
	instr.decode( getMem( addr++ ) );
	std::cout << NameCmds[ instr.cmd ] << " ";
	if ( instr.isInplaceImmediate( instr.cmd ) )
	{
		sx = std::to_string( (int) ((instr.xi == 1) ? instr.x - 8 : instr.x) );
	}
	else
	{
		sx = operandToStr( instr.x, instr.xi, addr );
	}
	sy = operandToStr( instr.y, instr.yi, addr );
	sr = operandToStr( instr.r, instr.ri, addr, true );
	std::cout << sr << " " << sy << " " << sx << "\n";
};


void Machine::reset()
{
	for ( int i = 0; i < 65536; i++ )
		mem[ i ] = 0;
	for ( int i = 0; i < 8; i++ )
		reg[ i ] = 0;
}

mWord Machine::getMem( mWord addr )
{
	if ( addr < PORT_START )
	{
		return mem[ addr ];
	}
	if ( addr == PORT_CONSOLE )
	{
		if ( !kbhit() )
			return 0;
		return _getch();
	}
	return 0;
};

void Machine::setMem( mWord addr, mWord data )
{
	if ( addr < PORT_START )
	{
		mem[ addr ] = data;
	}
	else
	{
		if ( addr == PORT_CONSOLE )
		{
			std::cout << static_cast< char >( data );
		}
	}
};

mWord Machine::read( mTag r, mTag i )
{
	if ( i )
	{
		mWord addr;
		if ( r == REG_PSW )
			addr = fetch();
		else
			addr = reg[ r ];
		if ( (r == REG_PC) || (r == REG_SP) )
			reg[ r ]++;
		//std::cout << "addr:" << addr << " read:" << getMem( addr ) << "\n";
		return getMem( addr );
	}
	else
	{
		//std::cout << "reg:" << r << " read:" << reg[ r ] << "\n";
		return reg[ r ];
	}
}

void Machine::step()
{
	mWord cond;
	// fetch & decode instruction
	instr.decode( fetch() );

	// read x
	if ( instr.isInplaceImmediate( instr.cmd ) )
	{
		if ( instr.xi )
			x = 0xFFF8 | instr.x;
		else
			x = instr.x;
	}
	else
	{
		x = read( instr.x, instr.xi );
	}
	y = read( instr.y, instr.yi );

	//std::cout << "cmd:" << (int) instr.cmd << " y:" << y << " x:" << x << "\n";
	// ALU
	tmp = 0;
	switch ( instr.cmd )	// combined opcode
	{
	case OP_ADDIS:	// addis
	case OP_ADDS:	// adds
			a = y + x;
			//std::cout << "acc:" << a << "\n";
			break;
	case OP_ADD:	// add
	case OP_ADDI:	// addi
			tmp = y + x;
			mathTempApply();
			break;
	case OP_ADC:	// adc
			tmp = y + x + (getFlag( FLAG_CARRY ) ? 1 : 0);
			mathTempApply();
			break;
	case OP_SUB:	// sub
			tmp = y - x;
			mathTempApply();
			break;
	case OP_SBC:	// sbc
			tmp = y - x - (getFlag( FLAG_CARRY ) ? 1 : 0);
			mathTempApply();
			break;
	case OP_AND:	// and
			tmp = x & y;
			mathTempApply();
			break;
	case OP_OR:	// or
			tmp = x | y;
			mathTempApply();
			break;
	case OP_XOR:	// xor
			tmp = x ^ y;
			mathTempApply();
			break;
	case OP_CADD:	// conditional add
			cond = (x >> 13) & 0b111;
			x = x & 0b1111111111111; // 13 bit
			if ( x & 0b1000000000000 )
				x = x | 0b1110000000000000; // negative extension
			if ( 	((cond == COND_ZERO) && (getFlag( FLAG_ZERO ))) ||
				((cond == COND_NZERO) && (!getFlag( FLAG_ZERO ))) ||
				((cond == COND_CARRY) && (getFlag( FLAG_CARRY ))) ||
				((cond == COND_NCARRY) && (!getFlag( FLAG_CARRY ))) ||
				((cond == COND_SIGN) && (getFlag( FLAG_SIGN ))) ||
				((cond == COND_NSIGN) && (!getFlag( FLAG_SIGN ))) )
			{
				a = y + x;
				//std::cout << "COND:" << a << "\n";
			}
			else
			{
				a = y;
				//std::cout << "NOT COND:" << a << "\n";
			}
			break;
	case OP_RRCI:	//
			break;
	case OP_RRC:	//
			break;
	};
	// store
	if ( instr.ri )
	{
		if ( instr.r != REG_PC )	// Indirect writes to PC are ignored (destination 'void')
		{
			mWord addr;
			if ( instr.r == REG_SP )
				reg[ instr.r ]--;
			if ( instr.r == REG_PSW )
				addr = fetch();
			else
				addr = reg[ instr.r ];
			//std::cout << "addr:" << addr << " writ:" << a << "\n";
			setMem( addr, a );
		}
	}
	else
	{
		reg[ instr.r ] = a;
	}
};

void Machine::show()
{
	for ( int i = 0; i < 8; i++ )
	{
		if ( i == REG_SP )
			std::cout << "SP";
		else if ( i == REG_PC )
			std::cout << "PC";
		else if ( i == REG_PSW )
			std::cout << "PW";
		else
			std::cout << "R" << i;
		std::cout << ":" << std::uppercase << std::hex << std::setw( 4 ) << std::setfill( '0' ) << reg[ i ];
		if ( i != 7 )
			std::cout << "  ";
	};
	std::cout << "\n";
	mWord start = 0;
	int cols = 8;
	int rows = 16;
	for ( int y = 0; y < rows; y++ )
	{
		for ( int x = 0; x < cols; x++ )
		{
			if ( x )
				std::cout << "  ";
			mWord cell = start + y + x * rows;
			std::cout << std::hex << std::setw( 4 ) << std::setfill( '0' ) << cell  << ":";
			std::cout << std::hex << std::setw( 4 ) << std::setfill( '0' ) << mem[ cell ];
		};
		std::cout << "\n";
	};
}

}	// namespace Simpleton
