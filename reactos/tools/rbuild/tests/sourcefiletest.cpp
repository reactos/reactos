#include "test.h"

using std::string;

bool
SourceFileTest::IsParentOf ( const SourceFile* parent,
	                         const SourceFile* child )
{
	size_t i;
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( child->parents[i] == parent )
			{
				return true;
			}
		}
	}
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( IsParentOf ( parent,
			                  child->parents[i] ) )
			{
				return true;
			}
		}
	}
	return false;
}

void
SourceFileTest::IncludeTest ()
{
	const Project project ( RBUILD_BASE "tests" SSEP "data" SSEP "automaticdependency_include.xml" );
	AutomaticDependency automaticDependency ( project );
	automaticDependency.Process ();
	ARE_EQUAL( 4, automaticDependency.sourcefile_map.size () );
	const SourceFile* include = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile_include.h" );
	IS_NOT_NULL( include );
	const SourceFile* includenext = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1" SSEP "sourcefile_includenext.h" );
	IS_NOT_NULL( includenext );
}

void
SourceFileTest::FullParseTest ()
{
	const Project project ( RBUILD_BASE "tests" SSEP "data" SSEP "automaticdependency.xml" );
	AutomaticDependency automaticDependency ( project );
	automaticDependency.Process ();
	ARE_EQUAL( 5, automaticDependency.sourcefile_map.size () );
	const SourceFile* header1 = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1_header1.h" );
	IS_NOT_NULL( header1 );
	const SourceFile* recurse = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1_recurse.h" );
	IS_NOT_NULL( recurse );
	IS_TRUE( IsParentOf ( header1,
	                      recurse ) );
	IS_FALSE( IsParentOf ( recurse,
	                       header1 ) );
	
}

void
SourceFileTest::Run ()
{
	IncludeTest ();
	FullParseTest ();
}
