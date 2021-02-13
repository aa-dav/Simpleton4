#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

namespace Simpleton
{

typedef	uint16_t	mWord;
typedef	uint8_t		mTag;

const int REG_R0	=	0;
const int REG_R1	=	1;
const int REG_R2	=	2;
const int REG_R3	=	3;
const int REG_R4	=	4;
const int REG_R5	=	5;
const int REG_R6	=	6;
const int REG_R7	=	7;
// aliases
const int REG_SP	=	5;
const int REG_PC	=	6;
const int REG_PSW	=	7;

const int IND_R0	=	0 + 8;
const int IND_R1	=	1 + 8;
const int IND_R2	=	2 + 8;
const int IND_R3	=	3 + 8;
const int IND_R4	=	4 + 8;
const int IND_R5	=	5 + 8;
const int IND_R6	=	6 + 8;
const int IND_R7	=	7 + 8;
// aliases
const int IND_SP	=	5 + 8;
const int IND_PC	=	6 + 8;
const int IMMED		=	6 + 8;
const int IND_IMMED	=	7 + 8;

const int FLAG_ZERO	=	0;
const int FLAG_CARRY	=	1;
const int FLAG_SIGN	=	2;
const int FLAG_OVERFLOW	=	3;
const int FLAG_IRQ_ENABLE = 	15;

const int COND_ZERO	=	0;
const int COND_NZERO	=	1;
const int COND_CARRY	=	2;
const int COND_NCARRY	=	3;
const int COND_GT	=	4;
const int COND_GTE	=	5;

const int OP_ADDIS	=	0x00;
const int OP_ADDI	=	0x01;
const int OP_ADDS	=	0x02;
const int OP_ADD	=	0x03;
const int OP_ADC	=	0x04;
const int OP_SUB	=	0x05;
const int OP_SBC	=	0x06;
const int OP_AND	=	0x07;
const int OP_OR		=	0x08;
const int OP_XOR	=	0x09;
const int OP_CMP	=	0x0A;
const int OP_CADD	=	0x0B;
const int OP_RRCI	=	0x0C;
const int OP_RRC	=	0x0D;

const int PORT_CONSOLE	=	0xFFFF;
const int PORT_START	=	0xFFFF;

struct Instruction
{
	mTag	x;
	bool	xi;
	mTag	y;
	bool	yi;
	mTag	r;
	bool	ri;
	mTag	cmd;

	void decode( mWord word )
	{
		// 0b0000000000000000
		x	= (word & 0b0000000000000111) >> 0;
		xi	= (word & 0b0000000000001000) >> 3;
		y	= (word & 0b0000000001110000) >> 4;
		yi	= (word & 0b0000000010000000) >> 7;
		r	= (word & 0b0000011100000000) >> 8;
		ri	= (word & 0b0000100000000000) >> 11;
		cmd	= (word & 0b1111000000000000) >> 12;
	};

	static mWord encode( mTag _cmd, mTag _r, mTag _y, mTag _x )
	{
		return (_cmd << 12) | (_r << 8) | (_y << 4) | (_x << 0);
	};
	
	static bool isInplaceImmediate( mTag cmd );
};

class ParseError
{
	std::string reason;
	int line;
public:
	ParseError( int _line, const std::string &_reason ): line( _line ), reason( _reason ) {};
	int getLine() const { return line; };
	const std::string &getReason() const { return reason; };
};

class Machine
{
private:
	mWord		mem[ 65536 ];
	mWord		reg[ 8 ];
	Instruction	instr;
	mWord		x, y, a;
	uint32_t	tmp;

	mWord getMem( mWord addr );
	void setMem( mWord addr, mWord data );
	mWord fetch() 
	{ 
		return getMem( reg[ REG_PC ]++ );
	}
	bool getFlag( mTag flag ) 
	{
		return (reg[ REG_PSW ] & (1 << flag)) != 0;
	}
	void setFlag( mTag flag, bool value ) 
	{
		if ( value )
			reg[ REG_PSW ] |= (1 << flag);
		else
			reg[ REG_PSW ] &= ~(1 << flag);
	}
	void mathTempApply()
	{
		a = tmp & 0xFFFF;
		setFlag( FLAG_CARRY, (tmp & 0x10000) != 0 );
		setFlag( FLAG_ZERO, (a == 0) );
		setFlag( FLAG_SIGN, (a & 0x8000) != 0 );
	}
	mWord read( mTag r, mTag i );

public:
	Machine()
	{
		reset();
	}

	mWord currentOp()
	{
		return mem[ reg[ REG_PC ] ];
	}
	mWord getPC()
	{
		return reg[ REG_PC ];
	}

	void reset();
	void step();
	void show();

	std::string operandToStr( mTag r, mTag i, int &addr );
	void showDisasm( int addr );

	friend class Assembler;
};

class Assembler
{
private:
	struct Identifier
	{
		enum Type
		{
			Register,
			Symbol,
			Command,
			CondBranch
		};
		enum Mode
		{
			AsmClassic,
			AsmNew,
			AsmBoth
		};
		
		std::string name;
		Mode mode;
		Type type;
		int value;

		Identifier() {};
		Identifier( const std::string &_name, Type _type, int _value, Mode _mode ): name( _name ), type( _type ), value( _value ), mode( _mode ) {};
		Identifier( const Identifier &src ): name( src.name ), type( src.type ), value( src.value ), mode( src.mode ) {};
	};

	struct ForwardReference
	{
		std::string name;
		mWord addr;
		int lineNum;
		bool cadd = false;
		ForwardReference() {};
		ForwardReference( const std::string _name, mWord _addr, int _lineNum, bool _cadd = false ): name( _name ), addr( _addr ), lineNum( _lineNum ), cadd( _cadd ) {};
		ForwardReference( const ForwardReference &src ): name( src.name ), addr( src.addr ), lineNum( src.lineNum ), cadd( src.cadd ) {};
	};

	Machine		*machine;
	mWord		org = 0;
	std::string	errorMessage;
	int		lineNum;
	std::string	lastLabel;
	std::string	curLabel;
	int		curLexem;
	std::vector< std::string >	lexems;
	std::vector< Identifier >	identifiers;
	std::vector< ForwardReference >	forwards;
	bool newSyntaxMode = false;

	Identifier *findIdentifier( const std::string &name, bool newSyntex );
	std::string extractNextLexem( const std::string &parseString, int &parsePos );
	void extractLexems( const std::string &parseString );
	void parseLine( const std::string &line );
	std::string getNextLexem();

public:
	Assembler() = delete;
	Assembler( const Assembler &src ) = delete;
	Assembler( Machine *m ): machine( m ) {};

	void setOrg( mWord newOrg )
	{
		org = newOrg;
	};
	void op( mTag _cmd, mTag _r, mTag _y, mTag _x, int addr = -1 )
	{	
		if ( addr == -1 )
			addr = org++;
		machine->mem[ addr ] = machine->instr.encode( _cmd, _r, _y, _x );
	};
	void data( mWord _data, int addr = -1 )
	{
		if ( addr == -1 )
			addr = org++;
		machine->mem[ addr ] = _data;
	};

	void reset();
	void parseStart();
	void parseEnd();
	mWord parseConstExpr( const std::string &expr, int addrForForward = -1 );
	bool parseFile( const std::string &fileName );
	std::string getErrorMessage() { return errorMessage; };

};

}	// namespace Simpleton