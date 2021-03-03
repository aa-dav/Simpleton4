#include "simpleton4asm.h"

int main( int argc, char *argv[] )
{
	Simpleton::Machine m;
	Simpleton::Assembler a( &m );

	if ( a.parseFile( "source.asm" ) )
	{
		while ( m.currentOp() != 0 )	// nop as stop
		{
			if ( (argc > 1) && (std::string( "d" ) == argv[ 1 ]) )
				m.showDisasm( m.getPC() );
			m.step();
		}
		m.show();
	}
	else
	{
		std::cout << a.getErrorMessage() << "\n";
	}

	return 0;
};
