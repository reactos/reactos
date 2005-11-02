/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

/* Read at most this amount of bytes from each file and assume that all #includes are located within this block */
#define MAX_BYTES_TO_READ 4096

using std::string;
using std::vector;
using std::map;

SourceFile::SourceFile ( AutomaticDependency* automaticDependency,
                         const Module& module,
                         const string& filename,
                         SourceFile* parent,
                         bool isNonAutomaticDependency )
	: automaticDependency ( automaticDependency ),
	  module ( module ),
	  filename ( filename ),
	  isNonAutomaticDependency ( isNonAutomaticDependency ),
	  youngestLastWriteTime ( 0 )
{
	if ( parent != NULL )
		parents.push_back ( parent );
	GetDirectoryAndFilenameParts ();
}

void
SourceFile::GetDirectoryAndFilenameParts ()
{
	size_t index = filename.find_last_of ( cSep );
	if ( index != string::npos )
	{
		directoryPart = filename.substr ( 0, index );
		filenamePart = filename.substr ( index + 1, filename.length () - index );
	}
	else
	{
		directoryPart = "";
		filenamePart = filename;
	}
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
	struct stat statbuf;

	Close ();
	FILE* f = fopen ( filename.c_str (), "rb" );
	if ( !f )
		throw FileNotFoundException ( filename );

	if ( fstat ( fileno ( f ), &statbuf ) != 0 )
	{
		fclose ( f );
		throw AccessDeniedException ( filename );
	}
	lastWriteTime = statbuf.st_mtime;

	unsigned long len = (unsigned long) filelen ( f );
	if ( len > MAX_BYTES_TO_READ )
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
SourceFile::ReadInclude ( string& filename,
                          bool& searchCurrentDirectory,
                          bool& includeNext)
{
	while ( p < end )
	{
		if ( ( *p == '#') && ( end - p > 13 ) )
		{
			bool include = false;
			p++;
			SkipWhitespace ();
			if ( *p == 'i' )
			{
				if ( strncmp ( p, "include ", 8 ) == 0 )
				{
					p += 8;
					includeNext = false;
					include = true;
				}
			        if ( strncmp ( p, "include_next ", 13 ) == 0 )
			        {
					p += 13;
					includeNext = true;
		        		include = true;
			        }
	
	       			if ( include )
				{
					SkipWhitespace ();
					if ( p < end )
					{
						register char ch = *p;
						if ( ch == '<' || ch == '"' )
						{
							searchCurrentDirectory = (ch == '"');
							p++;
							filename.resize ( MAX_PATH );
							int i = 0;
							ch = *p;
							while ( p < end && ch != '>' && ch != '"' && ch != '\n' )
							{
								filename[i++] = *p;
								p++;
								if ( p < end )
									ch = *p;
							}
							filename.resize ( i );
							return true;
						}
					}
				}
			}
		}
		p++;
	}
	filename = "";
	searchCurrentDirectory = false;
	includeNext = false;
	return false;
}

bool
SourceFile::IsParentOf ( const SourceFile* parent,
                         const SourceFile* child )
{
	size_t i;
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( child->parents[i] == parent )
				return true;
		}
	}
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( IsParentOf ( parent,
			                  child->parents[i] ) )
				return true;
		}
	}
	return false;
}

bool
SourceFile::IsIncludedFrom ( const string& normalizedFilename )
{
	if ( normalizedFilename == filename )
		return true;
	
	SourceFile* sourceFile = automaticDependency->RetrieveFromCache ( normalizedFilename );
	if ( sourceFile == NULL )
		return false;

	return IsParentOf ( sourceFile,
	                    this );
}

SourceFile*
SourceFile::GetParentSourceFile ()
{
	if ( isNonAutomaticDependency )
		return NULL;
	return this;
}

bool
SourceFile::CanProcessFile ( const string& extension )
{
	if ( extension == ".h" || extension == ".H" )
		return true;
	if ( extension == ".c" || extension == ".C" )
		return true;
	if ( extension == ".cpp" || extension == ".CPP" )
		return true;
	if ( extension == ".rc" || extension == ".RC" )
		return true;
	if ( extension == ".s" || extension == ".S" )
		return true;
	return false;
}

SourceFile*
SourceFile::ParseFile ( const string& normalizedFilename )
{
	string extension = GetExtension ( normalizedFilename );
	if ( CanProcessFile ( extension ) )
	{
		if ( IsIncludedFrom ( normalizedFilename ) )
			return NULL;
		
		SourceFile* sourceFile = automaticDependency->RetrieveFromCacheOrParse ( module,
		                                                                         normalizedFilename,
		                                                                         GetParentSourceFile () );
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
		
		bool searchCurrentDirectory;
		bool includeNext;
		while ( ReadInclude ( includedFilename,
		                      searchCurrentDirectory,
		                      includeNext ) )
		{
			string resolvedFilename ( "" );
			bool locatedFile = automaticDependency->LocateIncludedFile ( this,
			                                                             module,
			                                                             includedFilename,
			                                                             searchCurrentDirectory,
			                                                             includeNext,
			                                                             resolvedFilename );
			if ( locatedFile )
			{
				SourceFile* sourceFile = ParseFile ( resolvedFilename );
				if ( sourceFile != NULL )
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
AutomaticDependency::ParseFiles ()
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
		ParseFiles ( *project.modules[i] );
}

void
AutomaticDependency::GetModuleFiles ( const Module& module,
                                      vector<File*>& files ) const
{
	for ( size_t i = 0; i < module.non_if_data.files.size (); i++ )
		files.push_back ( module.non_if_data.files[i] );

	/* FIXME: Collect files in IFs here */

	if ( module.pch != NULL )
		files.push_back ( &module.pch->file );
}

void
AutomaticDependency::ParseFiles ( const Module& module )
{
	vector<File*> files;
	GetModuleFiles ( module, files );
	for ( size_t i = 0; i < files.size (); i++ )
		ParseFile ( module, *files[i] );
}

void
AutomaticDependency::ParseFile ( const Module& module,
                                 const File& file )
{
	string normalizedFilename = NormalizeFilename ( file.name );
	RetrieveFromCacheOrParse ( module,
	                           normalizedFilename,
	                           NULL );
}

bool
AutomaticDependency::LocateIncludedFile ( const string& directory,
                                          const string& includedFilename,
                                          string& resolvedFilename )
{
	string normalizedFilename = NormalizeFilename ( directory + sSep + includedFilename );
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

string
AutomaticDependency::GetFilename ( const string& filename )
{
	size_t index = filename.find_last_of ( cSep );
	if (index == string::npos)
		return filename;
	else
		return filename.substr ( index + 1,
		                         filename.length () - index - 1);
}

void
AutomaticDependency::GetIncludeDirectories ( vector<Include*>& includes,
                                             const Module& module,
                                             Include& currentDirectory,
                                             bool searchCurrentDirectory )
{
	size_t i;
	if ( searchCurrentDirectory )
		includes.push_back( &currentDirectory );
	for ( i = 0; i < module.non_if_data.includes.size (); i++ )
		includes.push_back( module.non_if_data.includes[i] );
	for ( i = 0; i < module.project.non_if_data.includes.size (); i++ )
		includes.push_back( module.project.non_if_data.includes[i] );
}

bool
AutomaticDependency::LocateIncludedFile ( SourceFile* sourceFile,
                                          const Module& module,
                                          const string& includedFilename,
                                          bool searchCurrentDirectory,
                                          bool includeNext,
                                          string& resolvedFilename )
{
	vector<Include*> includes;
	Include currentDirectory ( module.project, ".", sourceFile->directoryPart );
	GetIncludeDirectories ( includes, module, currentDirectory, searchCurrentDirectory );
	for ( size_t j = 0; j < includes.size (); j++ )
	{
		Include& include = *includes[j];
		if ( LocateIncludedFile ( include.directory,
		                          includedFilename,
		                          resolvedFilename ) )
		{
			if ( includeNext && stricmp ( resolvedFilename.c_str (),
			                              sourceFile->filename.c_str () ) == 0 )
				continue;
			return true;
		}
	}
	resolvedFilename = "";
	return false;
}

SourceFile*
AutomaticDependency::RetrieveFromCacheOrParse ( const Module& module,
                                                const string& filename,
                                                SourceFile* parentSourceFile )
{
	SourceFile* sourceFile = sourcefile_map[filename];
	if ( sourceFile == NULL )
	{
		sourceFile = new SourceFile ( this,
		                              module,
		                              filename,
		                              parentSourceFile,
		                              false );
		sourcefile_map[filename] = sourceFile;
		sourceFile->Parse ();
	}
	else if ( parentSourceFile != NULL )
		sourceFile->parents.push_back ( parentSourceFile );
	return sourceFile;
}

SourceFile*
AutomaticDependency::RetrieveFromCache ( const string& filename )
{
	return sourcefile_map[filename];
}

void
AutomaticDependency::CheckAutomaticDependencies ( bool verbose )
{
	ParseFiles ();
	for ( size_t mi = 0; mi < project.modules.size (); mi++ )
	{
		Module& module = *project.modules[mi];
		CheckAutomaticDependencies ( module, verbose );
	}
}

void
AutomaticDependency::GetModulesToCheck ( Module& module, vector<const Module*>& modules )
{
	modules.push_back ( &module );
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type != ObjectLibrary )
			break;
		modules.push_back ( library.importedModule );
	}

	/* FIXME: Collect libraries in IFs here */
}

void
AutomaticDependency::CheckAutomaticDependenciesForModule ( Module& module,
                                                           bool verbose )
{
	size_t mi;
	vector<const Module*> modules;
	GetModulesToCheck ( module, modules );
	for ( mi = 0; mi < modules.size (); mi++ )
		ParseFiles ( *modules[mi] );
	for ( mi = 0; mi < modules.size (); mi++ )
		CheckAutomaticDependencies ( *modules[mi], verbose );
}

void
AutomaticDependency::CheckAutomaticDependencies ( const Module& module,
                                                  bool verbose )
{
	struct utimbuf timebuf;
	vector<File*> files;
	GetModuleFiles ( module, files );
	for ( size_t fi = 0; fi < files.size (); fi++ )
	{
		File& file = *files[fi];
		string normalizedFilename = NormalizeFilename ( file.name );

		SourceFile* sourceFile = RetrieveFromCache ( normalizedFilename );
		if ( sourceFile != NULL )
		{
			CheckAutomaticDependenciesForFile ( sourceFile );
			assert ( sourceFile->youngestLastWriteTime != 0 );
			if ( sourceFile->youngestLastWriteTime > sourceFile->lastWriteTime )
			{
				if ( verbose )
				{
					printf ( "Marking %s for rebuild due to younger file %s\n",
					         sourceFile->filename.c_str (),
					         sourceFile->youngestFile->filename.c_str () );
				}
				timebuf.actime = sourceFile->youngestLastWriteTime;
				timebuf.modtime = sourceFile->youngestLastWriteTime;
				utime ( sourceFile->filename.c_str (),
				        &timebuf );
			}
		}
	}
}

void
AutomaticDependency::CheckAutomaticDependenciesForFile ( SourceFile* sourceFile )
{
	if ( sourceFile->youngestLastWriteTime > 0 )
		return;

	if ( sourceFile->files.size () == 0 )
	{
		sourceFile->youngestLastWriteTime = sourceFile->lastWriteTime;
		sourceFile->youngestFile = sourceFile;
		return;
	}

	for ( size_t i = 0; i < sourceFile->files.size (); i++ )
	{
		SourceFile* childSourceFile = sourceFile->files[i];
		
		CheckAutomaticDependenciesForFile ( childSourceFile );
		if ( ( childSourceFile->youngestLastWriteTime > sourceFile->youngestLastWriteTime ) ||
		     ( childSourceFile->lastWriteTime > sourceFile->youngestLastWriteTime ) )
		{
			if ( childSourceFile->youngestLastWriteTime > childSourceFile->lastWriteTime )
			{
				sourceFile->youngestLastWriteTime = childSourceFile->youngestLastWriteTime;
				sourceFile->youngestFile = childSourceFile->youngestFile;
			}
			else
			{
				sourceFile->youngestLastWriteTime = childSourceFile->lastWriteTime;
				sourceFile->youngestFile = childSourceFile;
			}
		}
	}
}
