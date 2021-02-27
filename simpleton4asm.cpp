#include "simpleton4asm.h"
#include <conio.h>

namespace Simpleton
{

void Assembler::reset()
{
	org = 0;
	files.clear();
	lines.clear();
	identifiers.clear();
	forwards.clear();
}

void Assembler::parseStart()
{
	lastLabel.clear();
	curLabel.clear();
	lineNum = 0;
	errorMessage.clear();

	identifiers.clear();
	identifiers.emplace_back( "r0",		Identifier::Register, REG_R0,	Identifier::AsmBoth );
	identifiers.emplace_back( "r1",		Identifier::Register, REG_R1,	Identifier::AsmBoth );
	identifiers.emplace_back( "r2",		Identifier::Register, REG_R2,	Identifier::AsmBoth );
	identifiers.emplace_back( "r3",		Identifier::Register, REG_R3,	Identifier::AsmBoth );
	identifiers.emplace_back( "r4",		Identifier::Register, REG_R4,	Identifier::AsmBoth );
	identifiers.emplace_back( "r5",		Identifier::Register, REG_R5,	Identifier::AsmBoth );
	identifiers.emplace_back( "r6",		Identifier::Register, REG_R6,	Identifier::AsmBoth );
	identifiers.emplace_back( "r7",		Identifier::Register, REG_R7,	Identifier::AsmBoth );
	identifiers.emplace_back( "pc",		Identifier::Register, REG_PC,	Identifier::AsmBoth );
	identifiers.emplace_back( "sp",		Identifier::Register, REG_SP,	Identifier::AsmBoth );
	identifiers.emplace_back( "psw",	Identifier::Register, REG_PSW,	Identifier::AsmBoth );

	identifiers.emplace_back( "addi",	Identifier::Command, OP_ADDI,	Identifier::AsmClassic );
	identifiers.emplace_back( "addis",	Identifier::Command, OP_ADDIS,	Identifier::AsmClassic );	
	identifiers.emplace_back( "adds",	Identifier::Command, OP_ADDS,	Identifier::AsmClassic );
	identifiers.emplace_back( "add",	Identifier::Command, OP_ADD,	Identifier::AsmClassic );
	identifiers.emplace_back( "adc",	Identifier::Command, OP_ADC,	Identifier::AsmClassic );
	identifiers.emplace_back( "sub",	Identifier::Command, OP_SUB,	Identifier::AsmClassic );
	identifiers.emplace_back( "sbc",	Identifier::Command, OP_SBC,	Identifier::AsmClassic );
	identifiers.emplace_back( "cmp",	Identifier::Command, OP_CMP,	Identifier::AsmClassic );
	identifiers.emplace_back( "and",	Identifier::Command, OP_AND,	Identifier::AsmClassic );
	identifiers.emplace_back( "or",		Identifier::Command, OP_OR,	Identifier::AsmClassic );
	identifiers.emplace_back( "xor",	Identifier::Command, OP_XOR,	Identifier::AsmClassic );
	identifiers.emplace_back( "cadd",	Identifier::Command, OP_CADD,	Identifier::AsmClassic );
	identifiers.emplace_back( "rrci",	Identifier::Command, OP_RRCI,	Identifier::AsmClassic );
	identifiers.emplace_back( "rrc",	Identifier::Command, OP_RRC,	Identifier::AsmClassic );

	identifiers.emplace_back( "move",	Identifier::Command, OP_ADDIS,	Identifier::AsmClassic );	// shortcut for addis (move)
	identifiers.emplace_back( "movet",	Identifier::Command, OP_ADDI,	Identifier::AsmClassic );	// shortcut for addi (move with test)

	identifiers.emplace_back( "+s",		Identifier::Command, OP_ADDS,	Identifier::AsmNew );
	identifiers.emplace_back( "+",		Identifier::Command, OP_ADD,	Identifier::AsmNew );
	identifiers.emplace_back( "+c",		Identifier::Command, OP_ADC,	Identifier::AsmNew );
	identifiers.emplace_back( "-",		Identifier::Command, OP_SUB,	Identifier::AsmNew );
	identifiers.emplace_back( "-c",		Identifier::Command, OP_SBC,	Identifier::AsmNew );
	identifiers.emplace_back( "?",		Identifier::Command, OP_CMP,	Identifier::AsmNew );
	identifiers.emplace_back( "&",		Identifier::Command, OP_AND,	Identifier::AsmNew );
	identifiers.emplace_back( "|",		Identifier::Command, OP_OR,	Identifier::AsmNew );
	identifiers.emplace_back( "^",		Identifier::Command, OP_XOR,	Identifier::AsmNew );
	identifiers.emplace_back( "+?",		Identifier::Command, OP_CADD,	Identifier::AsmNew );
	identifiers.emplace_back( ">>",		Identifier::Command, OP_RRC,	Identifier::AsmNew );
	// addis, addi, rrci

	identifiers.emplace_back( "jz",		Identifier::CondBranch, COND_ZERO,	Identifier::AsmBoth );
	identifiers.emplace_back( "jnz",	Identifier::CondBranch, COND_NZERO,	Identifier::AsmBoth );
	identifiers.emplace_back( "jc",		Identifier::CondBranch, COND_CARRY,	Identifier::AsmBoth );
	identifiers.emplace_back( "jnc",	Identifier::CondBranch, COND_NCARRY,	Identifier::AsmBoth );

	forwards.clear();
}

Assembler::Identifier *Assembler::findIdentifier( const std::string &name, bool newSyntax )
{
	Identifier *res = nullptr;
	for ( int i = 0; i < identifiers.size(); i++ )
	{
		Identifier &ident = identifiers[ i ];
		if ( !newSyntax && (ident.mode == Identifier::AsmNew) )
			continue;
		if ( newSyntax && (ident.mode == Identifier::AsmClassic) )
			continue;
		if ( ident.name == name )
		{
			res = &ident;
			break;
		}
	}
	return res;
};

void Assembler::parseEnd()
{
	for ( auto &fwd : forwards )
	{
		Identifier *iden = findIdentifier( fwd.name, false );
		if ( iden == nullptr )
			throw ParseError( fwd.lineNum, "unknown forward reference '" + fwd.name + "'!" );
		if ( iden->type != Identifier::Symbol )
			throw ParseError( fwd.lineNum, "forward reference '" + fwd.name + "' is not symbol!" );
		if ( fwd.cadd )
		{
			int offs = (iden->value & 0xFFFF) - fwd.addr - 1;
			if ( (offs < -4096) || (offs > 4095) )
				throw ParseError( fwd.lineNum, "conditional jump offset is too big (" + std::to_string( offs ) + ")!" );
			machine->mem[ fwd.addr ] |= (offs & 0x1FFF);
		}
		else
			machine->mem[ fwd.addr ] = iden->value;
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

void Assembler::extractLexems( const std::string &parseString, SourceLine &line )
{
	line.lexems.clear();
	line.label = false;
	bool first = true;
	int parsePos = 0;
	while ( true )
	{
		std::string cur = extractNextLexem( parseString, parsePos );
		if ( cur.empty() || (cur == ";") )	
			break;
		if ( first && !isspace( parseString[ 0 ] ) )
		{
			line.label = true;
		}
		line.lexems.push_back( cur );
	        first = false;
	}
}

std::string Assembler::getNextLexem()
{
	if ( curLexem < lines[ lineNum - 1 ].lexems.size() )
		return lines[ lineNum - 1 ].lexems[ curLexem++ ];
	else
		return std::string();
};

bool lexemIsNumberLiteral( const std::string &lexem  )
{
	return (lexem[ 0 ] == '$') || isdigit( lexem[ 0 ] ) || ( (lexem[ 0 ] == '-') && (lexem.size() > 1) );
}

int parseNumberLiteral( const std::string &lexem )
{
	int res;
	if ( lexem[ 0 ] == '$' )
		res = strtol( lexem.c_str() + 1, nullptr, 16 );
	else
		res = strtol( lexem.c_str(), nullptr, 10 );
	return res;
}

mWord Assembler::parseConstExpr( const std::string &expr, int addrForForward )
{
	int value = 0;
	if ( expr.empty() )
		throw ParseError( lineNum, "constexpr expected!" );
	if ( lexemIsNumberLiteral( expr ) )
	{
		value = parseNumberLiteral( expr );
	}
	else
	{
		// try to find symbol
		Identifier *iden = findIdentifier( expr, false );
		if ( iden != nullptr )
		{
			if ( iden->type == Identifier::Symbol )
			{
				value = iden->value;
			}
			else
				throw ParseError( lineNum, "identifier for constexpr '" + expr + "' is not symbol!" );
		}
		else if ( addrForForward != -1 )
		{
			forwards.emplace_back( expr, addrForForward, lineNum );
		}
		else
			throw ParseError( lineNum, "symbol for constexpr '" + expr + "' does not exist!" );
	};
	return value;
};

void Assembler::processArgument( const std::string &kind, const std::string &lexem, const int reg, const int value, const bool fwd )
{
	if (	(!newSyntax && (stage == 1)) ||
		(newSyntax && (stage == 0)) )
	{
		if ( reg == IMMED )
			throw ParseError( lineNum, "R cannot be immediate!" );
		r = reg;
		emitR = value;
		if ( fwd )
			fwdR = lexem;
	}
	else if (	(!newSyntax && (stage == 2)) ||
			(newSyntax && (stage == 2)) )
	{
		y = reg;
		emitY = value;
		if ( fwd )
			fwdY = lexem;
	}
	else if (	(!newSyntax && (stage == 3)) ||
			(newSyntax && (stage == 4)) )
	{
		if ( Instruction::isInplaceImmediate( cmd ) && indirect )
			throw ParseError( lineNum, "Inplace immediate cannot be indirect!" );
		x = reg;
		emitX = value;
		if ( fwd )
			fwdX = lexem;
	}
	else
	{
		throw ParseError( lineNum, kind + " '" + lexem + "' at wrong place!" );
	};
}

void Assembler::parseLine()
{
	bool invertX = false;
	bool emitForCall = false;
	int eqSign = 0; // 0 - "=", 1 - "<=", 2 - "<-" in new syntax

	// Preset common state variables...
	indirect = false;
	newSyntax = newSyntaxMode;
	stage = 0;
	cmd = -1; 
	cond = -1;
	emitX = 0; emitY = 0; emitR = 0;
	r = -1; x = -1; y = -1; 
	fwdR.clear();
	fwdX.clear();
	fwdY.clear();

	if ( !curLabel.empty() )
	{
		if ( findIdentifier( curLabel, newSyntax ) != nullptr )
			throw ParseError( lineNum, "identifier " + curLabel + " is redefined!" );
		identifiers.emplace_back( curLabel, Identifier::Symbol, org, Identifier::AsmBoth );
		//std::cout << "New identif added: " << curLabel << "\n";
	}
	std::string lexem;
	bool first = true;
	while ( !(lexem = getNextLexem()).empty() )
	{
		if ( first && (lexem == "org") )
		{
			org = parseConstExpr( getNextLexem() );
			if ( !curLabel.empty() )
			{
				Identifier *iden = findIdentifier( curLabel, newSyntax );
				if ( iden == nullptr )
					throw ParseError( lineNum, "Current label does not exist!" );
				iden->value = org;
			}
			return;	// no futher actions required
		}
		else if ( first && (lexem == "=") )
		{
			if ( curLabel.empty() )
				throw ParseError( lineNum, "new symbol is required for equatation!" );
			mWord word = parseConstExpr( getNextLexem() );	
			Identifier *iden = findIdentifier( curLabel, newSyntax );
			if ( iden == nullptr )
				throw ParseError( lineNum, "Current label does not exist!" );
			iden->value = word;
			return;	// no futher actions required
		}
		else if ( first && (lexem == "mode") )
		{
			lexem = getNextLexem();
			if ( lexem == "classic" )
			{
				newSyntaxMode = false;
			}
			else if ( lexem == "new" )
			{
				newSyntaxMode = true;
			}
			else
				throw ParseError( lineNum, "Wrong assembler mode operand ('classic' or 'new' expected)!" );
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
		else if ( lexemIsNumberLiteral( lexem ) )
		{
			// Literal
			int value = parseNumberLiteral( lexem );

			processArgument( "literal", lexem, indirect ? IND_IMMED : IMMED, value, false );

			stage++;
		}
		else if ( (lexem == "=") && newSyntax )
		{
			if ( stage != 1 )
				throw ParseError( lineNum, "keyword '=' at wrong place!" );
			stage++;
		}
		else if ( (lexem == "<=") && newSyntax )
		{
			if ( stage != 1 )
				throw ParseError( lineNum, "keyword '<=' at wrong place!" );
			cmd = OP_ADDI;
			eqSign = 1;
			stage++;
		}
		else if ( (lexem == "<-") && newSyntax )
		{
			if ( stage != 1 )
				throw ParseError( lineNum, "keyword '<-' at wrong place!" );
			cmd = OP_ADDIS;
			eqSign = 2;
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
			newSyntax = false;	// revert to classic mode
		}
		else if ( lexem == "call" )
		{
			if ( stage != 0 )
				throw ParseError( lineNum, "CALL in wrong place!" );
			emitForCall = true;
			cmd = OP_ADDIS;
			r = REG_PC;
			stage = 2;
			newSyntax = false;	// revert to classic mode
		}
		else
		{
			Identifier *iden = findIdentifier( lexem, newSyntax );
			if ( iden != nullptr )
			{
				if ( iden->type == Identifier::Symbol )
				{
					processArgument( "symbol", lexem, indirect ? IND_IMMED : IMMED, iden->value, false );
				}
				else if ( iden->type == Identifier::Register )
				{
					processArgument( "register", lexem, iden->value + (indirect ? 8 : 0), -1, false );
				}
				else if ( iden->type == Identifier::Command )
				{
					// C 1 2 3
					// 0 = 2 + 4
					if (	(!newSyntax && (stage == 0)) ||
						(newSyntax && (stage == 3)) )
					{
						cmd = iden->value;
						if ( newSyntax )
						{
							// corrections for eqSign
							if ( eqSign == 1 ) {
								if ( cmd == OP_ADD ) {
									cmd = OP_ADDI;
								} else if ( cmd == OP_RRC ) {
									cmd = OP_RRCI;
								} else
									ParseError( lineNum, "'<=' is used with wrong operator '" + lexem + "'!" );
							} else if ( eqSign == 2 ) {
								if ( cmd == OP_ADD ) {
									cmd = OP_ADDIS;
								} else if ( cmd == OP_SUB ) {
									cmd = OP_ADDIS;
									invertX = true;
								} else
									ParseError( lineNum, "'<-' is used with wrong operator '" + lexem + "'!" );
							}
						}
					}
					else
					{
						throw ParseError( lineNum, "command '" + lexem + "' at wrong place!" );
					}
				}
				else if ( iden->type == Identifier::CondBranch )
				{
					if ( stage != 0 )
						throw ParseError( lineNum, " conditional branch '" + lexem + "' at wrong place!" );
					cmd = OP_CADD;
					r = REG_PC;
					y = REG_PC;
					stage = 2;
					newSyntax = false;	// revert to classic syntax
					cond = iden->value;
				}
				else
				{
					throw ParseError( lineNum, "unknown identifier '" + lexem + "' type!" );
				};
			}
			else
			{
				processArgument( "unknown symbol", lexem, indirect ? IND_IMMED : IMMED, 0, true );
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
		if ( invertX )
			emitX = -emitX;
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

void Assembler::preProcessFile( const std::string &fileName )
{
	std::string line;
	int innerLineNum = 1;
	int fileNum = files.size();
	files.emplace_back( fileName );
	std::ifstream ifs( fileName );
	if ( ifs.fail() )
		throw PreProcessorError( fileNum - 1, lineNum, "cannot open file '" + fileName + "'!" );
	while ( std::getline( ifs, line ) )
	{
		lineNum++;
		innerLineNum++;
		SourceLine l;
		l.file = fileNum;
		l.num = innerLineNum;
		extractLexems( line, l );
		if ( (l.lexems.size() > 0) && (l.lexems[ 0 ][ 0 ] == '#') )
		{
			if ( l.lexems[ 0 ] == "#include" )
			{
				if ( l.lexems.size() != 2 )
					throw PreProcessorError( fileNum, innerLineNum, "#include directive must has one string parameter!" );
				if ( l.lexems[ 1 ][ 0 ] != '"' )
					throw PreProcessorError( fileNum, innerLineNum, "#include directive parameter must be quoted string!" );
				preProcessFile( &l.lexems[ 1 ][ 1 ] );
			}
			else
			{
				throw PreProcessorError( fileNum, innerLineNum, "unknown preprocessor directive '" + l.lexems[ 0 ] + "'!" );
			}
		}
		else
		{
			if ( l.lexems.size() > 0 )
				lines.push_back( l );
		}
	};
}

bool Assembler::parseFile( const std::string &fileName )
{
	try
	{
		lineNum = 0;
		preProcessFile( fileName ); // preprocess
		// process local labels:
		for ( int i = 0; i < lines.size(); i++ )
		{
			std::cout << "File " << lines[ i ].file << " line " << lines[ i ].num << ":";
			if ( !lines[ i ].label ) std::cout << "    ";
			for ( int j = 0; j < lines[ i ].lexems.size(); j++ )
			{
				std::cout << " " << lines[ i ].lexems[ j ];
			}
			std::cout << "\n";
		}
		parseStart();
		std::string line;
		for ( int i = 0; i < lines.size(); i++ )
		{
			lineNum++;
			curLexem = 0;
			curLabel.clear();

			std::vector< std::string > &lexems = lines[ i ].lexems;
			if ( lines[ i ].label )
			{
				if ( lexems[ 0 ][ 0 ] != '.' )
					lastLabel = lexems[ 0 ];	// update last label if it is not local
			}
			for ( int j = 0; j < lexems.size(); j++ )	// modify local ifentifiers
			{
				if ( lexems[ j ][ 0 ] == '.' )
				{
					if ( lastLabel.empty() )
						throw ParseError( lineNum, "local label '" + lexems[ j ] + "' before global label!" );
					lexems[ j ] = lastLabel + lexems[ j ];
				}
			}
			if ( lines[ i ].label )	// assign current label if needed
			{
				curLabel = lines[ i ].lexems[ 0 ];
				curLexem++;
			}
			parseLine();
		}
		parseEnd();
		// Dump identifiers...
		for ( auto &i : identifiers )
		{
			if ( i.type == Identifier::Symbol )
				std::cout << "Symbol: " << i.name << " value: " << i.value << "\n";
		}
	}
	catch ( const Simpleton::PreProcessorError &error )
	{
		int errorLine = error.getLine();
		std::string fname;
		if ( error.getFile() >= 0 )
			fname = files[ error.getFile() ].name;
		errorMessage = std::string( "Preprocessor error at file '" ) + fname + "' line " + std::to_string( error.getLine() ) + " Reason: " + error.getReason();
		return false;
	}
	catch ( const Simpleton::ParseError &error )
	{
		int errorLine = error.getLine();
		SourceLine line;
		line.file = -1;
		line.num = 0;
		line.label = false;
		SourceFile file{ "<unknown>" };
		if ( (errorLine > 0) && (errorLine <= lines.size()) )
			line = lines[ errorLine - 1 ];
		if ( (line.file >= 0) && (line.file < files.size()) )
			file = files[ line.file ];
		
		errorMessage = std::string( "Parse error at file '" ) + file.name + "' line " + std::to_string( line.num ) + " Reason: " + error.getReason();
		return false;
	}
	catch ( const std::exception &error )
	{
		errorMessage = std::string( "Unknown error: " ) + error.what();
		return false;
	}
	return true;
};

}	// namespace Simpleton
