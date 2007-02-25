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
#ifdef _MSC_VER
#include <errno.h>
#else
#include <dirent.h>
#endif//_MSC_VER

#ifdef WIN32
#define MKDIR(s) mkdir(s)
#else
#define MKDIR(s) mkdir(s, 0755)
#endif

using std::string;
using std::vector;

Directory::Directory ( const string& name_ )
	: name(name_)
{
}

void
Directory::Add ( const char* subdir )
{
	size_t i;
	string s1 = string ( subdir );
	if ( ( i = s1.find ( '$' ) ) != string::npos )
	{
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "No environment variables can be used here. Path was %s",
		                                  subdir );
	}

	const char* p = strpbrk ( subdir, "/\\" );
	if ( !p )
		p = subdir + strlen(subdir);
	string s ( subdir, p-subdir );
	if ( subdirs.find(s) == subdirs.end() )
		subdirs[s] = new Directory(s);
	if ( *p && *++p )
		subdirs[s]->Add ( p );
}

bool
Directory::mkdir_p ( const char* path )
{
#ifndef _MSC_VER
	DIR *directory;
	directory = opendir ( path );
	if ( directory != NULL )
	{
		closedir ( directory );
		return false;
	}
#endif//_MSC_VER

	if ( MKDIR ( path ) != 0 )
	{
#ifdef _MSC_VER
		if ( errno == EEXIST )
			return false;
#endif//_MSC_VER
		throw AccessDeniedException ( string ( path ) );
	}
	return true;
}

bool
Directory::CreateDirectory ( string path )
{
	size_t index = 0;
	size_t nextIndex;
	if ( isalpha ( path[0] ) && path[1] == ':' && path[2] == cSep )
	{
		nextIndex = path.find ( cSep, 3);
	}
	else
		nextIndex = path.find ( cSep );

	bool directoryWasCreated = false;
	while ( nextIndex != string::npos )
	{
		nextIndex = path.find ( cSep, index + 1 );
		directoryWasCreated = mkdir_p ( path.substr ( 0, nextIndex ).c_str () );
		index = nextIndex;
	}
	return directoryWasCreated;
}

string
Directory::ReplaceVariable ( string name,
                             string value,
                             string path )
{
	size_t i = path.find ( name );
	if ( i != string::npos )
		return path.replace ( i, name.length (), value );
	else
		return path;
}

void
Directory::ResolveVariablesInPath ( char* buf,
                                    string path )
{
	string s = ReplaceVariable ( "$(INTERMEDIATE)", Environment::GetIntermediatePath (), path );
	s = ReplaceVariable ( "$(OUTPUT)", Environment::GetOutputPath (), s );
	s = ReplaceVariable ( "$(INSTALL)", Environment::GetInstallPath (), s );
	strcpy ( buf, s.c_str () );
}

void
Directory::GenerateTree ( const string& parent,
                          bool verbose )
{
	string path;

	if ( parent.size () > 0 )
	{
		char buf[256];
		
		path = parent + sSep + name;
		ResolveVariablesInPath ( buf, path );
		if ( CreateDirectory ( buf ) && verbose )
			printf ( "Created %s\n", buf );
	}
	else
		path = name;

	for ( directory_map::iterator i = subdirs.begin ();
		i != subdirs.end ();
		++i )
	{
		i->second->GenerateTree ( path, verbose );
	}
}

string
Directory::EscapeSpaces ( string path )
{
	string newpath;
	char* p = &path[0];
	while ( *p != 0 )
	{
		if ( *p == ' ' )
			newpath = newpath + "\\ ";
		else
			newpath = newpath + *p;
		*p++;
	}
	return newpath;
}

void
Directory::CreateRule ( FILE* f,
                        const string& parent )
{
	string path;

	if ( parent.size() > 0 )
	{
		string escapedParent = EscapeSpaces ( parent );
		fprintf ( f,
			"%s%c%s: | %s\n",
			escapedParent.c_str (),
			cSep,
			EscapeSpaces ( name ).c_str (),
			escapedParent.c_str () );

		fprintf ( f,
			"\t$(ECHO_MKDIR)\n" );

		fprintf ( f,
			"\t${mkdir} $@\n" );

		path = parent + sSep + name;
	}
	else
		path = name;

	for ( directory_map::iterator i = subdirs.begin();
		i != subdirs.end();
		++i )
	{
		i->second->CreateRule ( f, path );
	}
}
