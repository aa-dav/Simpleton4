#include "simpleton.h"

int main( int argc, char *argv[] )
{
	Simpleton::Machine m;
	Simpleton::Assembler a( &m );

	if ( a.parseFile( "source.asm" ) )
	{
		//m.show();
		while ( m.currentOp() != 0 )	// nop as stop
		{
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
