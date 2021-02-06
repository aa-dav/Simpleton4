#include "simpleton.h"
#include <conio.h>

namespace Simpleton
{

/*static*/ bool Instruction::isInplaceImmediate( mTag cmd )
{
	return (cmd == OP_ADDI) || (cmd == OP_ADDIS) || (cmd == OP_RRCI);
}

static const char *NameCmds[] = {
"ADDIS",
"ADDI ",
"ADDS ",
"ADD  ",
"ADC  ",
"SUB  ",
"SBC  ",
"AND  ",
"OR   ",
"XOR  ",
"CMP  ",
"CADD ",
"RRCI ",
"RRC  " };

static const char *NameRegs[] = {
"R0",
"R1",
"R2",
"R3",
"R4",
"SP",
"PC",
"PW",
"[ R0 ]",
"[ R1 ]",
"[ R2 ]",
"[ R3 ]",
"[ R4 ]",
"[ SP ]",
"[ PC ]",
"[ PW ]"
};

std::string Machine::operandToStr( mTag r, mTag i, int &addr )
{
	std::stringstream ss;
	if ( (i == 1) && (r == REG_PC) )
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
	sr = operandToStr( instr.r, instr.ri, addr );
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
	case OP_CMP:	// cmp
			a = y;	// to keep Y intact...
			tmp = y - x;
			setFlag( FLAG_CARRY, tmp & 0x10000 );
			setFlag( FLAG_ZERO, (tmp & 0xFFFF) == 0 );
			break;
	case OP_CADD:	// conditional add
			cond = (x >> 13) & 0b111;
			x = x & 0b1111111111111; // 13 bit
			if ( x & 0b1000000000000 )
				x = x | 0b1110000000000000; // negative extension
			if ( 	((cond == COND_ZERO) && (getFlag( FLAG_ZERO ))) ||
				((cond == COND_NZERO) && (!getFlag( FLAG_ZERO ))) ||
				((cond == COND_CARRY) && (getFlag( FLAG_CARRY ))) ||
				((cond == COND_NCARRY) && (!getFlag( FLAG_CARRY ))) )
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

void Assembler::reset()
{
	org = 0;
}

void Assembler::parseStart()
{
	lastLabel.clear();
	lineNum = 0;
	errorMessage.clear();

	identifiers.clear();
	identifiers[ "r0" ]		= Identifier( Identifier::Register, REG_R0 );
	identifiers[ "r1" ]		= Identifier( Identifier::Register, REG_R1 );
	identifiers[ "r2" ]		= Identifier( Identifier::Register, REG_R2 );
	identifiers[ "r3" ]		= Identifier( Identifier::Register, REG_R3 );
	identifiers[ "r4" ]		= Identifier( Identifier::Register, REG_R4 );
	identifiers[ "r5" ]		= Identifier( Identifier::Register, REG_R5 );
	identifiers[ "r6" ]		= Identifier( Identifier::Register, REG_R6 );
	identifiers[ "r7" ]		= Identifier( Identifier::Register, REG_R7 );
	identifiers[ "pc" ]		= Identifier( Identifier::Register, REG_PC );
	identifiers[ "sp" ]		= Identifier( Identifier::Register, REG_SP );
	identifiers[ "psw" ]		= Identifier( Identifier::Register, REG_PSW );

	identifiers[ "addi" ]		= Identifier( Identifier::Command, OP_ADDI );
	identifiers[ "addis" ]		= Identifier( Identifier::Command, OP_ADDIS );	
	identifiers[ "add" ]		= Identifier( Identifier::Command, OP_ADD );
	identifiers[ "adds" ]		= Identifier( Identifier::Command, OP_ADDS );
	identifiers[ "adc" ]		= Identifier( Identifier::Command, OP_ADC );
	identifiers[ "sub" ]		= Identifier( Identifier::Command, OP_SUB );
	identifiers[ "sbc" ]		= Identifier( Identifier::Command, OP_SBC );
	identifiers[ "cmp" ]		= Identifier( Identifier::Command, OP_CMP );
	identifiers[ "and" ]		= Identifier( Identifier::Command, OP_AND );
	identifiers[ "or" ]		= Identifier( Identifier::Command, OP_OR );
	identifiers[ "xor" ]		= Identifier( Identifier::Command, OP_XOR );
	identifiers[ "cadd" ]		= Identifier( Identifier::Command, OP_CADD );
	identifiers[ "rrci" ]		= Identifier( Identifier::Command, OP_RRCI );
	identifiers[ "rrc" ]		= Identifier( Identifier::Command, OP_RRC );

	identifiers[ "move" ]		= Identifier( Identifier::Command, OP_ADDIS );	// shortcut for addis (move)
	identifiers[ "movet" ]		= Identifier( Identifier::Command, OP_ADDI );	// shortcut for addi (move with test)

	identifiers[ "jz" ]		= Identifier( Identifier::CondBranch, COND_ZERO );
	identifiers[ "jnz" ]		= Identifier( Identifier::CondBranch, COND_NZERO );
	identifiers[ "jc" ]		= Identifier( Identifier::CondBranch, COND_CARRY );
	identifiers[ "jnc" ]		= Identifier( Identifier::CondBranch, COND_NCARRY );

	forwards.clear();
}

void Assembler::parseEnd()
{
	for ( auto &fwd : forwards )
	{
		auto it = identifiers.find( fwd.name );
		if ( it == identifiers.end() )
			throw ParseError( fwd.lineNum, "unknown forward reference '" + fwd.name + "'!" );
		Identifier &iden = it->second;
		if ( iden.type != Identifier::Symbol )
			throw ParseError( fwd.lineNum, "forward reference '" + fwd.name + "' is not symbol!" );
		if ( fwd.cadd )
		{
			int offs = (iden.value & 0xFFFF) - fwd.addr - 1;
			if ( (offs < -4096) || (offs > 4095) )
				throw ParseError( fwd.lineNum, "conditional jump offset is too big (" + std::to_string( offs ) + ")!" );
			machine->mem[ fwd.addr ] |= (offs & 0x1FFF);
		}
		else
			machine->mem[ fwd.addr ] = iden.value;
	};
}

std::string Assembler::extractNextLexem( const std::string &parseString, int &parsePos )
{
	std::string res;

	if ( parsePos >= parseString.length() )
	{
		//std::cout << "end of lexems\n";
		return res;
	};

	while ( isspace( parseString[ parsePos ] ) )
	{
		if ( ++parsePos >= parseString.length() )
		{
			return res;
		}
	};
	if ( parseString[ parsePos ] == '"' )
	{
		while( true )
		{
			res += parseString[ parsePos ];
			if ( ++parsePos >= parseString.length() )
				throw ParseError( lineNum, "unexpected end of string '" + res + "'!" );
			if ( parseString[ parsePos ] == '"' )
			{
				parsePos++;
				break;
			}
		};
	}
	else
	{
		while ( !isspace( parseString[ parsePos ] ) )
		{
			res += parseString[ parsePos ];
			if ( ++parsePos >= parseString.length() )
				break;
		};
	};
	if ( res == ";" )
	{
		// comment terminates parsing...
		res.clear();
		parsePos = parseString.length();
	};
	return res;
};

void Assembler::extractLexems( const std::string &parseString )
{
	lexems.clear();
	curLexem = 0;
	curLabel.clear();
	bool first = true;
	int parsePos = 0;
	while ( true )
	{
		std::string cur = extractNextLexem( parseString, parsePos );
		if ( cur.empty() || (cur == ";") )	
			break;
		if ( first && !isspace( parseString[ 0 ] ) )
		{
			curLabel = cur;
		}
		else
		{
			lexems.push_back( cur );
		}
	        first = false;
	}
	if ( !curLabel.empty() )
	{
		if ( curLabel[ 0 ] == '.' )
		{
			if ( lastLabel.empty() )
				throw ParseError( lineNum, "local label '" + curLabel + "' before global label!" );
			curLabel = lastLabel + curLabel;
		}
		else
		{
			if ( (lexems.size() > 0) && (lexems[ 0 ] == "=") )
			{
				// supress lastLabel change for equatations...
			}
			else
			{
				lastLabel = curLabel;
			}
				
		}
	}
	for ( int i = 0; i < lexems.size(); i++ )
	{
		if ( lexems[ i ][ 0 ] == '.' )
		{
			if ( lastLabel.empty() )
				throw ParseError( lineNum, "local label '" + lexems[ i ] + "' before global label!" );
			lexems[ i ] = lastLabel + lexems[ i ];
		}
	}
	/*
	if ( !curLabel.empty() || lexems.size() )
	{
		if ( !curLabel.empty() )
			std::cout << curLabel << ": ";
		for ( auto &it: lexems )
			std::cout << it << " ";
		std::cout << "\n";
	};
	*/
}

std::string Assembler::getNextLexem()
{
	if ( curLexem < lexems.size() )
		return lexems[ curLexem++ ];
	else
		return std::string();
};

mWord Assembler::parseConstExpr( const std::string &expr, int addrForForward )
{
	int value = 0;
	if ( expr.empty() )
		throw ParseError( lineNum, "constexpr expected!" );
	if ( (expr[ 0 ] == '$') || isdigit( expr[ 0 ] ) || ( expr[ 0 ] == '-' ) )
	{
		// Literal
		if ( expr[ 0 ] == '$' )
			value = strtol( expr.c_str() + 1, nullptr, 16 );
		else
			value = strtol( expr.c_str(), nullptr, 10 );
	}
	else
	{
		// try to find symbol
		auto it = identifiers.find( expr );
		if ( it != identifiers.end() )
		{
			Identifier &iden = it->second;
			if ( iden.type == Identifier::Symbol )
			{
				value = iden.value;
			}
			else
				throw ParseError( lineNum, "identifier for constexpr '" + expr + "' is not symbol!" );
		}
		else if ( addrForForward != -1 )
		{
			forwards.emplace_back( expr, org, lineNum );
		}
		else
			throw ParseError( lineNum, "symbol for constexpr '" + expr + "' is not exists!" );
	};
	return value;
};

void Assembler::parseLine( const std::string &line )
{
	bool indirect = false;
	int emitX = 0;
	int emitY = 0;
	int emitR = 0;
	int stage = 0;
	int r = -1, x = -1, y = -1, cmd = -1, cond = -1;
	std::string fwdR, fwdX, fwdY;
	bool emitForCall = false;

	lineNum++;
	extractLexems( line );

	if ( !curLabel.empty() )
	{
		if ( identifiers.find( curLabel ) != identifiers.end() )
			throw ParseError( lineNum, "identifier " + curLabel + " is redefined!" );
		identifiers[ curLabel ] = Identifier( Identifier::Symbol, org );
	}
	std::string lexem;
	bool first = true;
	while ( !(lexem = getNextLexem()).empty() )
	{
		if ( first && (lexem == "org") )
		{
			org = parseConstExpr( getNextLexem() );
			if ( !curLabel.empty() )
				identifiers[ curLabel ].value = org;
			return;	// no futher actions required
		}
		else if ( first && (lexem == "=") )
		{
			if ( curLabel.empty() )
				throw ParseError( lineNum, "new symbol is required for equatation!" );
			identifiers[ curLabel ].value = parseConstExpr( getNextLexem() );
			return;	// no futher actions required
		}
		else if ( first && (lexem == "dw") )
		{
			while ( true )
			{
				lexem = getNextLexem();
				if ( lexem.empty() || (lexem == ";") )
					return;	// end of data
				if ( lexem[ 0 ] == '"' )
				{
					for ( int i = 1; i < lexem.length(); i++ )
					{
						data( lexem[ i ] );
					}
				}
				else
				{
					mWord val = parseConstExpr( lexem, org );
					data( val );
				};
			};
			return;	// no futher actions required
		}
		else if ( first && (lexem == "ds") )
		{
			lexem = getNextLexem();
			if ( lexem.empty() )
				throw ParseError( lineNum, "DS requres argument!" );
		        mWord size = parseConstExpr( lexem );
		        mWord fill = 0;
		        lexem = getNextLexem();
		        if ( !lexem.empty() && (lexem != ";") )
		        {
		        	fill = parseConstExpr( lexem );
		        };
		        for ( int i = 0; i < size; i++ )
		        	data( fill );
			return;	// no futher actions required
		}
		else if ( (lexem[ 0 ] == '$') || isdigit( lexem[ 0 ] ) || (lexem[ 0 ] == '-') )
		{
			// Literal
			int value;
			if ( lexem[ 0 ] == '$' )
				value = strtol( lexem.c_str() + 1, nullptr, 16 );
			else
				value = strtol( lexem.c_str(), nullptr, 10 );
			if ( (stage == 1) && indirect )
			{
				emitR = value;
				r = IND_IMMED;
			}
			else if ( stage == 2 )
			{
				emitY = value;
				y = indirect ? IND_IMMED : IMMED;
			}
			else if ( stage == 3 )
			{
				if ( Instruction::isInplaceImmediate( cmd ) && indirect )
					throw ParseError( lineNum, "Inplace immediate cannot be indirect!" );
				emitX = value;
				x = indirect ? IND_IMMED : IMMED;
			}
			else
			{
				throw ParseError( lineNum, "literal '" + lexem + "' at wrong place!" );
			};
			stage++;
		}
		else if ( lexem == "[" )
		{
			indirect = true;
		}
		else if ( lexem == "]" )
		{
			indirect = false;
		}
		else if ( lexem == "ret" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "RET in wrong place!" );
			cmd = OP_ADDIS;
			r = REG_PC;
			y = IND_SP;
			x = IMMED;
			emitX = 0;
			stage = 4;
		}
		else if ( lexem == "call" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "CALL in wrong place!" );
			emitForCall = true;
			cmd = OP_ADDIS;
			r = REG_PC;
			stage = 2;
		}
		else
		{
			auto it = identifiers.find( lexem );
			if ( it != identifiers.end() )
			{
				Identifier &iden = it->second;
				if ( iden.type == Identifier::Symbol )
				{
					if ( (stage == 1) && indirect )
					{
						emitR = iden.value;
						r = IND_IMMED;
					}
					else if ( stage == 2 )
					{
						emitY = iden.value;
						y = indirect ? IND_IMMED : IMMED;
					}
					else if ( stage == 3 )
					{
						if ( Instruction::isInplaceImmediate( cmd ) && indirect )
							throw ParseError( lineNum, "Inplace immediate cannot be indirect!" );
						emitX = iden.value;
						x = indirect ? IND_IMMED : IMMED;
					}
					else
					{
						throw ParseError( lineNum, "symbol '" + lexem + "' at wrong place!" );
					};
				}
				else if ( iden.type == Identifier::Register )
				{
					if ( stage == 1 )
					{
						r = iden.value + (indirect ? 8 : 0);
					}
					else if ( stage == 2 )
					{
						y = iden.value + (indirect ? 8 : 0);
					}
					else if ( stage == 3 )
					{
						if ( Instruction::isInplaceImmediate( cmd ) )
							throw ParseError( lineNum, "Inplace immediate cannot be register!" );
						x = iden.value + (indirect ? 8 : 0);
					}
					else
					{
						throw ParseError( lineNum, "register '" + lexem + "' at wrong place!" );
					}
				}
				else if ( iden.type == Identifier::Command )
				{
					if ( stage == 0 )
					{
						cmd = iden.value;
					}
					else
					{
						throw ParseError( lineNum, "command '" + lexem + "' at wrong place!" );
					}
				}
				else if ( iden.type == Identifier::CondBranch )
				{
					if ( stage != 0 )
						throw ParseError( lineNum, " conditional branch '" + lexem + "' at wrong place!" );
					cmd = OP_CADD;
					r = REG_PC;
					y = REG_PC;
					stage = 2;
					cond = iden.value;
				}
				else
				{
					throw ParseError( lineNum, "unknown identifier '" + lexem + "' type!" );
				};
			}
			else
			{
				if ( (stage == 1) && indirect )
				{
					emitR = 0;
					fwdR = lexem;
					r = IND_IMMED;
				}
				else if ( stage == 2 )
				{
					emitY = 0;
					fwdY = lexem;
					y = indirect ? IND_IMMED : IMMED;
				}
				else if ( stage == 3 )
				{
					emitX = 0;
					fwdX = lexem;
					x = indirect ? IND_IMMED : IMMED;
				}
				else
				{
					throw ParseError( lineNum, "unknown symbol '" + lexem + "' at wrong place!" );
				};
			}
			stage++;
		}
		first = false;
	};

	if ( stage == 0 )
		return;	// just comment?

	if ( cmd == -1 )
		throw ParseError( lineNum, "CMD is undefined!" );
	if ( r == -1 )
		throw ParseError( lineNum, "R is undefined!" );
	if ( y == -1 )
		throw ParseError( lineNum, "Y is undefined!" );
	if ( x == -1 )
	{
		x = IMMED;
		emitX = 0;
	};

	if ( Instruction::isInplaceImmediate( cmd ) )
	{
		// inplace immediate mode
		if ( (emitX < -8) || (emitX > +7) )
			throw ParseError( lineNum, "Inplace immediate X is too big!" );
		x = emitX & 0b1111;
	}

	if ( emitForCall )
		op( OP_ADDIS, IND_SP, REG_PC, 2 );

	op( cmd, r, y, x );

	// immediates...
	if ( !Instruction::isInplaceImmediate( cmd ) && ((x == IMMED) || (x == IND_IMMED)) )
	{
		if ( cond != -1 )
		{
			if ( !fwdX.empty() )
			{
				data( (cond << 13) ); // place condition
				forwards.emplace_back( fwdX, org - 1, lineNum, true ); // conditional forward!!!
			}
			else
			{
				int offs = (emitX & 0xFFFF) - org - 1;
				if ( (offs < -4096) || (offs > 4095) )
					throw ParseError( lineNum, "conditional jump offset is too big (" + std::to_string( offs ) + ")!" );
				data( (cond << 13) | (offs & 0x1FFF) );
			}
		}
		else
		{
			data( emitX );
			if ( !fwdX.empty() )
				forwards.emplace_back( fwdX, org - 1, lineNum );
		}
	}
	if ( (y == IMMED) || (y == IND_IMMED) )
	{
		data( emitY );
		if ( !fwdY.empty() )
			forwards.emplace_back( fwdY, org - 1, lineNum );
	}
	if ( r == IND_IMMED )
	{
		data( emitR );
		if ( !fwdR.empty() )
			forwards.emplace_back( fwdR, org - 1, lineNum );
	}

}

bool Assembler::parseFile( const std::string &fileName )
{
	try
	{
		std::ifstream ifs( fileName );
		parseStart();
		std::string line;
		while ( std::getline( ifs, line ) )
		{
			parseLine( line );
		};
		parseEnd();
	}
	catch ( const Simpleton::ParseError &error )
	{
		// fix for mingw error in using std::to_string...
		//char buf[ 32 ];
		//itoa( error.getLine(), buf, 10 );
		//errorMessage = std::string( "Parse error at line " ) + buf + " reason: " + error.getReason();

		errorMessage = std::string( "Parse error at line " ) + std::to_string( error.getLine() ) + " reason: " + error.getReason();
		return false;
	}
	return true;
};


}	// namespace Simpleton

