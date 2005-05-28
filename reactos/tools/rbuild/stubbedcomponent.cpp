#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

StubbedComponent::StubbedComponent ( const Module* module_,
                                     const XMLElement& stubbedComponentNode )
	: module(module_),
	  node(stubbedComponentNode)
{
	const XMLAttribute* att = node.GetAttribute ( "name", true );
	assert ( att );
	name = att->value;
}

StubbedComponent::~StubbedComponent ()
{
	for ( size_t i = 0; i < symbols.size(); i++ )
		delete symbols[i];
}

void
StubbedComponent::ProcessXML ()
{
	size_t i;
	for ( i = 0; i < node.subElements.size (); i++ )
		ProcessXMLSubElement ( *node.subElements[i] );
	for ( i = 0; i < symbols.size (); i++ )
		symbols[i]->ProcessXML ();
}

void
StubbedComponent::ProcessXMLSubElement ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "symbol" )
	{
		symbols.push_back ( new StubbedSymbol ( e ) );
		subs_invalid = false;
	}
	if ( subs_invalid && e.subElements.size () > 0 )
		throw InvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i] );
}



StubbedSymbol::StubbedSymbol ( const XMLElement& stubbedSymbolNode )
	: node(stubbedSymbolNode)
{
}

StubbedSymbol::~StubbedSymbol ()
{
}

void
StubbedSymbol::ProcessXML ()
{
	if ( node.value.size () == 0 )
	{
		throw InvalidBuildFileException (
			node.location,
			"<symbol> is empty." );
	}
	symbol = node.value;

	strippedName = StripSymbol ( symbol );

	const XMLAttribute* att = node.GetAttribute ( "newname", false );
	if ( att != NULL )
		newname = att->value;
	else
		newname = strippedName;
}

string
StubbedSymbol::StripSymbol ( string symbol )
{
	size_t start = 0;
	while ( start < symbol.length () && symbol[start] == '@')
		start++;
	size_t end = symbol.length () - 1;
	while ( end > 0 && isdigit ( symbol[end] ) )
		end--;
	if ( end > 0 and symbol[end] == '@' )
		end--;
	if ( end > 0 )
		return symbol.substr ( start, end - start + 1 );
	else
		return "";
}
