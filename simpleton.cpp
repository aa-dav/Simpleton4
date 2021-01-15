#include "simpleton.h"
#include <conio.h>

namespace Simpleton
{

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

void Machine::step()
{
	bool skip = false;
	// fetch & decode instruction
	instr.decode( fetch() );
	//instr.show( reg[ REG_PC ] - 1 );
	
	// get src to x
	if ( instr.srcInd && ((instr.src == REG_FLAGS) || (instr.src == REG_PC)) )
		x = fetch();
	else
		x = reg[ instr.src ];
	
	// get dst to y
	if ( instr.dstInd && (instr.dst == REG_FLAGS) )
		y = fetch();
	else
		y = reg[ instr.dst ];
		// condition...
	switch ( instr.cond )
	{
	case COND_ZERO:
		skip = !getFlag( FLAG_ZERO );
		break;
	case COND_NZERO:
		skip = getFlag( FLAG_ZERO );
		break;
	case COND_CARRY:
		skip = !getFlag( FLAG_CARRY );
		break;
	case COND_NCARRY:
		skip = getFlag( FLAG_CARRY );
		break;
	case COND_GT:
		break;
	case COND_GTE:
		break;
	default:	// COND_ALWAYS
		break;
	};
	if ( skip )
		return;
		// indirect mode
	if ( instr.srcInd && (instr.src != REG_PC) )
	{
		x = getMem( x );
		if ( instr.src == REG_SP )
			reg[ instr.src ]++;
	};
	if ( instr.dstInd )
	{
		if ( instr.dst == REG_SP )
		{
			reg[ instr.dst ]--;
			y = reg[ instr.dst ];
	        };
		dstAddr = y;
		if ( instr.twoOps )
			y = getMem( y );
	};
	// ALU
	tmp = 0;
	switch ( instr.cmd + (instr.twoOps ? 16 : 0) )	// combined opcode
	{
			// *** SINGLE OPERAND ***
	case OP_MOV:	// mov
			a = x;
			break;
	case OP_MOVF:	// movf
			tmp = x;
			mathTempApply();
			break;
	case OP_INC:	// inc
			tmp = x + 1;
			mathTempApply();
			break;
	case OP_DEC:	// dec
			tmp = x - 1;
			mathTempApply();
			break;
	case OP_INC2:	// inc2
			tmp = x + 2;
			mathTempApply();
			break;
	case OP_DEC2:	// dec2
			tmp = x - 2;
			mathTempApply();
			break;
			
			// *** TWO OPERANDS ***
	case OP_CMP:	// cmp
			a = y;	// to keep Y intact...
			tmp = y - x;
			setFlag( FLAG_CARRY, tmp & 0x10000 );
			setFlag( FLAG_ZERO, (tmp & 0xFFFF) == 0 );
			break;
	case OP_ADD:	// add
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
	case OP_AND:	// and
			tmp = x & y;
			mathTempApply();
	case OP_OR:		// or
			tmp = x | y;
			mathTempApply();
	case OP_XOR:	// xor
			tmp = x ^ y;
			mathTempApply();
		break;
	};
	// store
	if ( instr.dstInd )
		setMem( dstAddr , a );
	else
		reg[ instr.dst ] = a;
};

void Machine::show()
{
	for ( int i = 0; i < 8; i++ )
	{
		if ( i == REG_SP )
			std::cout << "SP";
		else if ( i == REG_PC )
			std::cout << "PC";
		else if ( i == REG_FLAGS )
			std::cout << "FL";
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
	identifiers[ "flags" ]	= Identifier( Identifier::Register, REG_FLAGS );

	identifiers[ "=" ]		= Identifier( Identifier::Command, OP_MOV );
	identifiers[ "=?" ]		= Identifier( Identifier::Command, OP_MOVF );
	identifiers[ "=+1" ]	= Identifier( Identifier::Command, OP_INC );
	identifiers[ "=-1" ]	= Identifier( Identifier::Command, OP_DEC );
	identifiers[ "=+2" ]	= Identifier( Identifier::Command, OP_INC2 );
	identifiers[ "=-2" ]	= Identifier( Identifier::Command, OP_DEC2 );
	
	identifiers[ "<?>" ]	= Identifier( Identifier::Command, OP_CMP );
	identifiers[ "+=" ]		= Identifier( Identifier::Command, OP_ADD );
	identifiers[ "+c=" ]	= Identifier( Identifier::Command, OP_ADC );
	identifiers[ "-=" ]		= Identifier( Identifier::Command, OP_SUB );
	identifiers[ "-c=" ]	= Identifier( Identifier::Command, OP_SBC );
	identifiers[ "&=" ]		= Identifier( Identifier::Command, OP_AND );
	identifiers[ "|=" ]		= Identifier( Identifier::Command, OP_OR );
	identifiers[ "^=" ]		= Identifier( Identifier::Command, OP_XOR );

	identifiers[ "@@" ]		= Identifier( Identifier::Condition, COND_ALWAYS );
	identifiers[ "@z" ]		= Identifier( Identifier::Condition, COND_ZERO );
	identifiers[ "@nz" ]	= Identifier( Identifier::Condition, COND_NZERO );
	identifiers[ "@=" ]		= Identifier( Identifier::Condition, COND_ZERO );
	identifiers[ "@!=" ]	= Identifier( Identifier::Condition, COND_NZERO );
	identifiers[ "@c" ]		= Identifier( Identifier::Condition, COND_CARRY );
	identifiers[ "@nc" ]	= Identifier( Identifier::Condition, COND_NCARRY );
	identifiers[ "@>" ]		= Identifier( Identifier::Condition, COND_GT );
	identifiers[ "@>=" ]	= Identifier( Identifier::Condition, COND_GTE );

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
	if ( (expr[ 0 ] == '$') || isdigit( expr[ 0 ] ) )
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
	int emitX = -1;
	int emitY = -1;
	int stage = 0;
	int dst = -1, src = -1, cmd = -1, cond = 0;
	int yForwRefInd = -1;
	enum EmitSome
	{
		EmitNone = 0,
		EmitForCall = 1,
		EmitForQCall = 2
	};
	EmitSome emitSome = EmitNone;

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
		else if ( (lexem[ 0 ] == '$') || isdigit( lexem[ 0 ] ) )
		{
			// Literal
			int value;
			if ( lexem[ 0 ] == '$' )
				value = strtol( lexem.c_str() + 1, nullptr, 16 );
			else
				value = strtol( lexem.c_str(), nullptr, 10 );
			if ( (stage == 0) && indirect )
			{
				emitY = value;
				dst = IND_IMMED;
			}
			else if ( stage == 2 )
			{
				emitX = value;
				src = indirect ? IND_IMMED : IMMED;
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
			stage = 3;
			dst = REG_PC;
			src = REG_SP + 8;
			cmd = OP_MOV;
		}
		else if ( lexem == "qret" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "QRET in wrong place!" );
			stage = 3;
			dst = REG_PC;
			src = REG_R4;
			cmd = OP_MOV;
		}
		else if ( lexem == "call" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "CALL in wrong place!" );
			emitSome = EmitForCall;
			stage = 2;
			dst = REG_PC;
			cmd = OP_MOV;
			org++; // primary instruction will be placed next
		}
		else if ( lexem == "qcall" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "QCALL in wrong place!" );
			emitSome = EmitForQCall;
			stage = 2;
			dst = REG_PC;
			cmd = OP_MOV;
			org++; // primary instruction will be placed next
		}
		else
		{
			auto it = identifiers.find( lexem );
			if ( it != identifiers.end() )
			{
				Identifier &iden = it->second;
				if ( iden.type == Identifier::Symbol )
				{
					if ( (stage == 0) && indirect )
					{
						emitY = iden.value;
						dst = IND_IMMED;
					}
					else if ( stage == 2 )
					{
						emitX = iden.value;
						src = indirect ? IND_IMMED : IMMED;
					}
					else
					{
						throw ParseError( lineNum, "literal '" + lexem + "' at wrong place!" );
					};
				}
				else if ( iden.type == Identifier::Register )
				{
					if ( stage == 0 )
					{
						dst = iden.value + (indirect ? 8 : 0);
					}
					else if ( stage == 2 )
					{
						src = iden.value + (indirect ? 8 : 0);
					}
					else
					{
						throw ParseError( lineNum, "register '" + lexem + "' at wrong place!" );
					}
				}
				else if ( iden.type == Identifier::Command )
				{
					if ( stage == 1 )
					{
						cmd = iden.value;
					}
					else
					{
						throw ParseError( lineNum, "command '" + lexem + "' at wrong place!" );
					}
				}
				else if ( iden.type == Identifier::Condition )
				{       	
					cond = iden.value;
					stage--;	// condition must keep current stage...
				}
				else
				{
					throw ParseError( lineNum, "unknown identifier '" + lexem + "' type!" );
				};
			}
			else
			{
				if ( (stage == 0) && indirect )
				{
					emitY = 0;
					forwards.emplace_back( lexem, org + 1, lineNum );
					yForwRefInd = forwards.size() - 1;
					dst = IND_IMMED;
				}
				else if ( stage == 2 )
				{
					emitX = 0;
					forwards.emplace_back( lexem, org + 1, lineNum );
					src = indirect ? IND_IMMED : IMMED;
					//std::cout << "Forward to resolve: " << lexem << "\n";
				}
				else
				{
					throw ParseError( lineNum, "unknown literal '" + lexem + "' at wrong place!" );
				};
			}
			stage++;
		}
		first = false;
	};

	if ( stage == 0 )
		return;	// just comment?

	if ( dst == -1 )
		throw ParseError( lineNum, "DST is undefined!" );
	if ( src == -1 )
		throw ParseError( lineNum, "SRC is undefined!" );
	if ( cmd == -1 )
		throw ParseError( lineNum, "CMD is undefined!" );
	if ( emitSome == EmitForCall )
		op( REG_SP + 8, OP_INC2, REG_PC, cond, org - 1 );
	else if ( emitSome == EmitForQCall )
		op( REG_R4, OP_INC2, REG_PC, cond, org - 1 );
	op( dst, cmd, src, cond );
	// immediates...
	if ( emitX != -1 )
	{
		data( emitX );
		if ( yForwRefInd != -1 )
			forwards[ yForwRefInd ].addr++;
	};
	if ( emitY != -1 )
		data( emitY );
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
