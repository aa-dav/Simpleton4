#ifndef SIMPLETON_4_ASM_H
#define SIMPLETON_4_ASM_H

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include "simpleton4.h"

namespace Simpleton
{

class ParseError
{
	std::string reason;
	int line;
public:
	ParseError( int _line, const std::string &_reason ): line( _line ), reason( _reason ) {};
	int getLine() const { return line; };
	const std::string &getReason() const { return reason; };
};

class PreProcessorError
{
	std::string reason;
	int line;
	int file;
public:
	PreProcessorError( int _file, int _line, const std::string &_reason ): file( _file ), line( _line ), reason( _reason ) {};
	int getLine() const { return line; };
	int getFile() const { return file; };
	const std::string &getReason() const { return reason; };
};

class Assembler
{
private:
	struct SourceFile
	{
		std::string name;
		SourceFile( const std::string &src ): name( src ) {};
	};
	struct SourceLine
	{
		int file;
		int num;
		bool label;
		std::vector< std::string > lexems;

		SourceLine() {};
		SourceLine(	int f, int n, bool lb,
				const std::vector< std::string > &lx ): 
				file( f ), num( n ), label( lb ), lexems( lx ) {};
	};
	std::vector< SourceFile > files;
	std::vector< SourceLine > lines;

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
	std::vector< Identifier >	identifiers;
	std::vector< ForwardReference >	forwards;
	bool newSyntaxMode = false;
	// Current state of line parsing
	bool newSyntax, indirect;
	std::string fwdR, fwdY, fwdX;
	int emitR, emitY, emitX, r, x, y, cmd, cond, stage;

	void processArgument( const std::string &kind, const std::string &lexem, const int reg, const int value, const bool fwd );

	Identifier *findIdentifier( const std::string &name, bool newSyntex );

	std::string extractNextLexem( const std::string &parseString, int &parsePos );
	void extractLexems( const std::string &parseString, std::vector< std::string > &data, bool &hasLabel );

	void parseLine();
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

	void preProcessFile( const std::string &fileName );

	bool parseFile( const std::string &fileName );
	std::string getErrorMessage() { return errorMessage; };

};

}	// namespace Simpleton

#endif // SIMPLETON_4_ASM_H