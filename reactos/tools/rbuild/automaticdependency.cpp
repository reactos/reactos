#include "pch.h"

#ifdef WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#define _MAX_PATH 255
#endif
#include <assert.h>

#include "rbuild.h"

/* Read at most this amount of bytes from each file and assume that all #includes are located within this block */
#define MAX_BYTES_TO_READ 4096
	
using std::string;
using std::vector;
using std::map;

SourceFile::SourceFile ( AutomaticDependency* automaticDependency,
                         Module& module,
                         const string& filename )
	: automaticDependency ( automaticDependency ),
	  module ( module ),
	  filename ( filename )
{
}
	  
void
SourceFile::Close ()
{
	buf.resize ( 0 );
	p = end = NULL;
}

void
SourceFile::Open ()
{
	Close ();
	FILE* f = fopen ( filename.c_str (), "rb" );
	if ( !f )
		throw FileNotFoundException ( filename );
	unsigned long len = (unsigned long) filelen ( f );
	if ( len > MAX_BYTES_TO_READ)
		len = MAX_BYTES_TO_READ;
	buf.resize ( len );
	fread ( &buf[0], 1, len, f );
	fclose ( f );
	p = buf.c_str ();
	end = p + len;
}

void
SourceFile::SkipWhitespace ()
{
	while ( p < end && isspace ( *p ))
		p++;
}

bool
SourceFile::ReadInclude ( string& filename )
{
	while ( p < end )
	{
		if ( ( *p == '#') && ( end - p > 8 ) )
		{
			if ( strncmp ( p, "#include", 8 ) == 0 )
			{
				p += 8;
				SkipWhitespace ();
				if ( p < end && *p == '<' || *p == '"' )
				{
					p++;
					filename.resize ( MAX_PATH );
					int i = 0;
					while ( p < end && *p != '>' && *p != '"' && *p != '\n' )
						filename[i++] = *p++;
					filename.resize ( i );
					return true;
				}
			}
		}
		p++;
	}
	filename = "";
	return false;
}

SourceFile*
SourceFile::ParseFile ( const string& normalizedFilename )
{
	string extension = GetExtension ( normalizedFilename );
	if ( extension == ".c" || extension == ".C" || extension == ".h" || extension == ".H")
	{
		SourceFile* sourceFile = automaticDependency->RetrieveFromCacheOrParse ( module,
		                                                                         normalizedFilename );
		return sourceFile;
	}
	return NULL;
}

void
SourceFile::Parse ()
{
	Open ();
	while ( p < end )
	{
		string includedFilename ( "" );
		//printf ( "Parsing '%s'\n", filename.c_str () );
		
		while ( ReadInclude ( includedFilename ))
		{
			string resolvedFilename ( "" );
			bool locatedFile = automaticDependency->LocateIncludedFile ( module,
			                                                             includedFilename,
			                                                             resolvedFilename );
			if ( locatedFile )
			{
				SourceFile* sourceFile = ParseFile ( resolvedFilename );
				files.push_back ( sourceFile );
			}
		}
		p++;
	}
	Close ();
}

string
SourceFile::Location () const
{
	int line = 1;
	const char* end_of_line = strchr ( buf.c_str (), '\n' );
	while ( end_of_line && end_of_line < p )
	{
		++line;
		end_of_line = strchr ( end_of_line + 1, '\n' );
	}
	return ssprintf ( "%s(%i)",
	                  filename.c_str (),
	                  line );
}


AutomaticDependency::AutomaticDependency ( const Project& project )
	: project ( project )
{
}

AutomaticDependency::~AutomaticDependency ()
{
	std::map<std::string, SourceFile*>::iterator theIterator;
	for ( theIterator = sourcefile_map.begin (); theIterator != sourcefile_map.end (); theIterator++ )
		delete theIterator->second;
}

void
AutomaticDependency::Process ()
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
		ProcessModule ( *project.modules[i] );
}

void
AutomaticDependency::ProcessModule ( Module& module )
{
	for ( size_t i = 0; i < module.files.size (); i++ )
		ProcessFile ( module, *module.files[i] );
}

void
AutomaticDependency::ProcessFile ( Module& module,
	                               const File& file )
{
	string normalizedFilename = NormalizeFilename ( file.name );
	SourceFile sourceFile = SourceFile ( this,
	                                     module,
	                                     normalizedFilename );
	sourceFile.Parse ();
}

bool
AutomaticDependency::LocateIncludedFile ( const string& directory,
	                                      const string& includedFilename,
	                                      string& resolvedFilename )
{
	string normalizedFilename = NormalizeFilename ( directory + SSEP + includedFilename );
	FILE* f = fopen ( normalizedFilename.c_str (), "rb" );
	if ( f != NULL )
	{
		fclose ( f );
		resolvedFilename = normalizedFilename;
		return true;
	}
	resolvedFilename = "";
	return false;
}

bool
AutomaticDependency::LocateIncludedFile ( Module& module,
	                                      const string& includedFilename,
	                                      string& resolvedFilename )
{
	for ( size_t i = 0; i < module.includes.size (); i++ )
	{
		Include* include = module.includes[0];
		if ( LocateIncludedFile ( include->directory,
		                          includedFilename,
		                          resolvedFilename ) )
			return true;
	}
	
	/* FIXME: Ifs */
	
	resolvedFilename = "";
	return false;
}

SourceFile*
AutomaticDependency::RetrieveFromCacheOrParse ( Module& module,
	                                            const string& filename )
{
	SourceFile* sourceFile = sourcefile_map[filename];
	if ( sourceFile == NULL )
	{
		sourceFile = new SourceFile ( this,
		                              module,
		                              filename );
		sourceFile->Parse ();
		sourcefile_map[filename] = sourceFile;
	}
	return sourceFile;
}

SourceFile*
AutomaticDependency::RetrieveFromCache ( Module& module,
	                                     const string& filename )
{
	return sourcefile_map[filename];
}
