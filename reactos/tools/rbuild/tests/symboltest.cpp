#include "test.h"

using std::string;

void SymbolTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/symbol.xml" );
	Project project ( projectFilename );
	
	ARE_EQUAL ( 1, project.modules.size () );
	Module& module1 = *project.modules[0];
	
	ARE_EQUAL ( 1, module1.stubbedComponents.size () );
	StubbedComponent& component1 = *module1.stubbedComponents[0];
	ARE_EQUAL ( "ntdll.dll", component1.name );
	
	ARE_EQUAL ( 2, component1.symbols.size () );
	StubbedSymbol& symbol1 = *component1.symbols[0];
	ARE_EQUAL ( "HeapAlloc@12", symbol1.symbol );
	ARE_EQUAL ( "RtlAllocateHeap", symbol1.newname );
	
	StubbedSymbol& symbol2 = *component1.symbols[1];
	ARE_EQUAL ( "LdrAccessResource@16", symbol2.symbol );
	ARE_EQUAL ( "LdrAccessResource@16", symbol2.newname );
}
