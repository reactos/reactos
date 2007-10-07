/*
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2007 Hervé Poussineau
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
#include <algorithm> 

#include "rbuild.h"

/* Read at most this amount of bytes from each file and assume that all #includes are located within this block */
#define MAX_BYTES_TO_READ 4096

using std::string;
using std::vector;
using std::map;

SourceFile::SourceFile ( AutomaticDependency* automaticDependency,
                         const Module& module,
                         const File& file,
                         SourceFile* parent )
	: file ( file ),
	  automaticDependency ( automaticDependency),
	  module ( module ),
	  youngestLastWriteTime ( 0 )
{
	if (parent != NULL )
		parents.push_back ( parent );
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
	string filename = file.GetFullPath ();

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
		if ( p != buf.c_str () )
		{
			/* Go to end of line */
			while ( *p != '\n' && p < end )
				p++;
			SkipWhitespace ();
		}
		if ( ( end - p > 13 ) && ( *p == '#') )
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
SourceFile::IsIncludedFrom ( const File& file )
{
	SourceFile* sourceFile = automaticDependency->RetrieveFromCache ( file );
	if ( sourceFile == NULL )
		return false;

	if ( sourceFile == this )
		return true;

	return IsParentOf ( sourceFile,
	                    this );
}

bool
SourceFile::CanProcessFile ( const File& file )
{
	string extension = GetExtension ( file.file );
	std::transform ( extension.begin (), extension.end (), extension.begin (), tolower );
	if ( extension == ".h" )
		return true;
	if ( extension == ".c" )
		return true;
	if ( extension == ".cpp" )
		return true;
	if ( extension == ".rc" )
		return true;
	if ( extension == ".s" )
		return true;
	if ( extension == ".nls" )
		return true;
	if ( extension == ".idl" )
		return true;
	if ( automaticDependency->project.configuration.Verbose )
		printf ( "Skipping file %s, as its extension is not recognized\n", file.file.name.c_str () );
	return false;
}

SourceFile*
SourceFile::ParseFile ( const File& file )
{
	if ( !CanProcessFile ( file ) )
		return NULL;

	if ( IsIncludedFrom ( file ) )
		return NULL;

	SourceFile* sourceFile = automaticDependency->RetrieveFromCacheOrParse ( module,
	                                                                         file,
	                                                                         this );
	return sourceFile;
}

void
SourceFile::Parse ()
{
	Open ();
	while ( p < end )
	{
		string includedFilename;

		bool searchCurrentDirectory;
		bool includeNext;
		while ( ReadInclude ( includedFilename,
		                      searchCurrentDirectory,
		                      includeNext ) )
		{
			File resolvedFile ( SourceDirectory, "", "", false, "", false ) ;
			bool locatedFile = automaticDependency->LocateIncludedFile ( this,
			                                                             module,
			                                                             includedFilename,
			                                                             searchCurrentDirectory,
			                                                             includeNext,
			                                                             resolvedFile );
			if ( locatedFile )
			{
				SourceFile* sourceFile = ParseFile ( *new File ( resolvedFile ) );
				if ( sourceFile != NULL )
					files.push_back ( sourceFile );
			} else if ( automaticDependency->project.configuration.Verbose )
				printf ( "Unable to find %c%s%c, included in %s%c%s\n",
				         searchCurrentDirectory ? '\"' : '<',
				         includedFilename.c_str (),
				         searchCurrentDirectory ? '\"' : '>',
				         this->file.file.relative_path.c_str (),
				         cSep,
				         this->file.file.name.c_str () );
		}
		p++;
	}
	Close ();
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
	{
		const FileLocation& pch = module.pch->file;
		File *file = new File ( pch.directory, pch.relative_path, pch.name , false, "", true );
		files.push_back ( file );
	}
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
	RetrieveFromCacheOrParse ( module,
	                           file,
	                           NULL );
}

bool
AutomaticDependency::LocateIncludedFile ( const FileLocation& directory,
                                          const string& includedFilename )
{
	string path;
	switch ( directory.directory )
	{
		case SourceDirectory:
			path = "";
			break;
		case IntermediateDirectory:
			path = Environment::GetIntermediatePath ();
			break;
		case OutputDirectory:
			path = Environment::GetOutputPath ();
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  directory.directory );
	}
	if ( directory.relative_path.length () > 0 )
	{
		if ( path.length () > 0 )
			path += sSep;
		path += directory.relative_path;
	}

	string normalizedFilename = NormalizeFilename ( path + sSep + includedFilename );
	FILE* f = fopen ( normalizedFilename.c_str (), "rb" );
	if ( f != NULL )
	{
		fclose ( f );
		return true;
	}
	return false;
}

void
AutomaticDependency::GetIncludeDirectories ( vector<Include*>& includes,
                                             const Module& module )
{
	size_t i;
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
                                          File& resolvedFile )
{
	vector<Include*> includes;
	string includedFileDir;
	Include currentDirectory ( module.project, SourceDirectory, sourceFile->file.file.relative_path );
	if ( searchCurrentDirectory )
		includes.push_back ( &currentDirectory );
	GetIncludeDirectories ( includes, module );

	for ( size_t j = 0; j < includes.size (); j++ )
	{
		Include& include = *includes[j];
		if ( LocateIncludedFile ( *include.directory,
		                          includedFilename ) )
		{
			if ( includeNext && 
			     include.directory->directory == sourceFile->file.file.directory &&
			     include.directory->relative_path == sourceFile->file.file.relative_path )
			{
				includeNext = false;
				continue;
			}
			resolvedFile.file.directory = include.directory->directory;
			size_t index = includedFilename.find_last_of ( "/\\" );
			if ( index == string::npos )
			{
				resolvedFile.file.name = includedFilename;
				resolvedFile.file.relative_path = include.directory->relative_path;
			}
			else
			{
				resolvedFile.file.relative_path = NormalizeFilename (
					include.directory->relative_path + sSep +
					includedFilename.substr ( 0, index ) );
				resolvedFile.file.name = includedFilename.substr ( index + 1 );
			}
			return true;
		}
	}
	return false;
}

SourceFile*
AutomaticDependency::RetrieveFromCacheOrParse ( const Module& module,
                                                const File& file,
                                                SourceFile* parentSourceFile )
{
	string filename = file.GetFullPath();
	SourceFile* sourceFile = sourcefile_map[filename];
	if ( sourceFile == NULL )
	{
		sourceFile = new SourceFile ( this,
		                              module,
		                              file,
		                              parentSourceFile );
		sourcefile_map[filename] = sourceFile;
		sourceFile->Parse ();
	}
	else if ( parentSourceFile != NULL )
		sourceFile->parents.push_back ( parentSourceFile );
	return sourceFile;
}

SourceFile*
AutomaticDependency::RetrieveFromCache ( const File& file )
{
	string filename = file.GetFullPath();
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
		SourceFile* sourceFile = RetrieveFromCache ( file );
		if ( sourceFile != NULL )
		{
			CheckAutomaticDependenciesForFile ( sourceFile );
			assert ( sourceFile->youngestLastWriteTime != 0 );
			if ( sourceFile->youngestLastWriteTime > sourceFile->lastWriteTime )
			{
				if ( verbose )
				{
					printf ( "Marking %s%c%s for rebuild due to younger file %s%c%s\n",
					         sourceFile->file.file.relative_path.c_str (),
					         cSep, sourceFile->file.file.name.c_str (),
					         sourceFile->youngestFile->file.file.relative_path.c_str (),
					         cSep, sourceFile->youngestFile->file.file.name.c_str () );
				}
				timebuf.actime = sourceFile->youngestLastWriteTime;
				timebuf.modtime = sourceFile->youngestLastWriteTime;
				utime ( sourceFile->file.GetFullPath ().c_str (),
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
