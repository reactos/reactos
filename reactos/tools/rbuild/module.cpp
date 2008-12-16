/*
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2008 Hervé Poussineau
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

using std::string;
using std::vector;

string
Right ( const string& s, size_t n )
{
	if ( n > s.size() )
		return s;
	return string ( &s[s.size()-n] );
}

string
Replace ( const string& s, const string& find, const string& with )
{
	string ret;
	const char* p = s.c_str();
	while ( p )
	{
		const char* p2 = strstr ( p, find.c_str() );
		if ( !p2 )
			break;
		if ( p2 > p )
			ret += string ( p, p2-p );
		ret += with;
		p = p2 + find.size();
	}
	if ( *p )
		ret += p;
	return ret;
}

string
ChangeSeparator ( const string& s,
                  const char fromSeparator,
                  const char toSeparator )
{
	string s2(s);
	char* p = strchr ( &s2[0], fromSeparator );
	while ( p )
	{
		*p++ = toSeparator;
		p = strchr ( p, fromSeparator );
	}
	return s2;
}

string
FixSeparator ( const string& s )
{
	return ChangeSeparator ( s, cBadSep, cSep );
}

string
FixSeparatorForSystemCommand ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], DEF_CBAD_SEP );
	while ( p )
	{
		*p++ = DEF_CSEP;
		p = strchr ( p, DEF_CBAD_SEP );
	}
	return s2;
}

string
DosSeparator ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], '/' );
	while ( p )
	{
		*p++ = '\\';
		p = strchr ( p, '/' );
	}
	return s2;
}

string
ReplaceExtension (
	const string& filename,
	const string& newExtension )
{
	size_t index = filename.find_last_of ( '/' );
	if ( index == string::npos )
		index = 0;
	size_t index2 = filename.find_last_of ( '\\' );
	if ( index2 != string::npos && index2 > index )
		index = index2;
	string tmp = filename.substr( index /*, filename.size() - index*/ );
	size_t ext_index = tmp.find_last_of( '.' );
	if ( ext_index != string::npos )
		return filename.substr ( 0, index + ext_index ) + newExtension;
	return filename + newExtension;
}

string
GetSubPath (
	const Project& project,
	const string& location,
	const string& path,
	const string& att_value )
{
	if ( !att_value.size() )
		throw XMLInvalidBuildFileException (
			location,
			"<directory> tag has empty 'name' attribute" );
	if ( strpbrk ( att_value.c_str (), "/\\?*:<>|" ) )
		throw XMLInvalidBuildFileException (
			location,
			"<directory> tag has invalid characters in 'name' attribute" );
	if ( !path.size() )
		return att_value;

	return FixSeparator(path + cSep + att_value);
}

static string
GetExtension ( const string& filename )
{
	size_t index = filename.find_last_of ( '/' );
	if (index == string::npos) index = 0;
	string tmp = filename.substr( index, filename.size() - index );
	size_t ext_index = tmp.find_last_of( '.' );
	if (ext_index != string::npos)
		return filename.substr ( index + ext_index, filename.size() );
	return "";
}

string
GetExtension ( const FileLocation& file )
{
	return GetExtension ( file.name );
}

string
NormalizeFilename ( const string& filename )
{
	if ( filename == "" )
		return "";
	Path path;
	string normalizedPath = path.Fixup ( filename, true );
	string relativeNormalizedPath = path.RelativeFromWorkingDirectory ( normalizedPath );
	return FixSeparator ( relativeNormalizedPath );
}

bool
GetBooleanValue ( const string& value )
{
	if ( value == "1" )
		return true;
	else
		return false;
}

string
ToLower ( string filename )
{
	for ( size_t i = 1; i < filename.length (); i++ )
		filename[i] = tolower ( filename[i] );
	return filename;
}

IfableData::IfableData( )
	: asmFiles ( 0 )
{
}

void IfableData::ExtractModules( std::map<std::string, Module*> &modules )
{
	size_t i;
	for ( i = 0; i < this->modules.size (); i++ )
		modules.insert(std::make_pair(this->modules[i]->name, this->modules[i]));
}

IfableData::~IfableData()
{
	size_t i;
	for ( i = 0; i < includes.size (); i++ )
		delete includes[i];
	for ( i = 0; i < defines.size (); i++ )
		delete defines[i];
	for ( i = 0; i < libraries.size (); i++ )
		delete libraries[i];
	for ( std::map<std::string, Property*>::const_iterator p = properties.begin(); p != properties.end(); ++ p )
		delete p->second;
	for ( i = 0; i < compilerFlags.size (); i++ )
		delete compilerFlags[i];
	for ( i = 0; i < modules.size(); i++ )
		delete modules[i];
	for ( i = 0; i < compilationUnits.size (); i++ )
		delete compilationUnits[i];
}

void IfableData::ProcessXML ()
{
	size_t i;
	for ( i = 0; i < includes.size (); i++ )
		includes[i]->ProcessXML ();
	for ( i = 0; i < defines.size (); i++ )
		defines[i]->ProcessXML ();
	for ( i = 0; i < libraries.size (); i++ )
		libraries[i]->ProcessXML ();
	for ( std::map<std::string, Property*>::const_iterator p = properties.begin(); p != properties.end(); ++ p )
		p->second->ProcessXML ();
	for ( i = 0; i < compilerFlags.size(); i++ )
		compilerFlags[i]->ProcessXML ();
	for ( i = 0; i < compilationUnits.size (); i++ )
		compilationUnits[i]->ProcessXML ();
}

bool Module::GetBooleanAttribute ( const XMLElement& moduleNode, const char * name, bool default_value )
{
	const XMLAttribute* att = moduleNode.GetAttribute ( name, false );
	if ( att != NULL )
	{
		const char* p = att->value.c_str();
		if ( !stricmp ( p, "true" ) || !stricmp ( p, "yes" ) )
			return true;
		else if ( !stricmp ( p, "false" ) || !stricmp ( p, "no" ) )
			return false;
		else
		{
			throw InvalidAttributeValueException (
				moduleNode.location,
				name,
				att->value );
		}
	}
	else
		return default_value;
}

Module::Module ( const Project& project,
                 const XMLElement& moduleNode,
                 const string& modulePath )
	: project (project),
	  node (moduleNode),
	  importLibrary (NULL),
	  metadata (NULL),
	  bootSector (NULL),
	  bootstrap (NULL),
	  autoRegister(NULL),
	  linkerScript (NULL),
	  pch (NULL),
	  cplusplus (false),
	  output (NULL),
	  install (NULL)
{
	if ( node.name != "module" )
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "Module created with non-<module> node" );

	xmlbuildFile = FixSeparator ( Path::RelativeFromWorkingDirectory ( moduleNode.xmlFile->filename () ) );

	const XMLAttribute* att = moduleNode.GetAttribute ( "name", true );
	assert(att);
	name = att->value;

	enabled = true;

	att = moduleNode.GetAttribute ( "if", false );
	if ( att != NULL )
		enabled = GetBooleanValue ( project.ResolveProperties ( att->value ) );

	att = moduleNode.GetAttribute ( "ifnot", false );
	if ( att != NULL )
		enabled = !GetBooleanValue ( project.ResolveProperties ( att->value ) );

	if ( !enabled && project.configuration.Verbose )
		printf("Module '%s' has been disabled.\n", name.c_str () );

	att = moduleNode.GetAttribute ( "type", true );
	assert(att);
	type = GetModuleType ( node.location, *att );

	att = moduleNode.GetAttribute ( "extension", false );
	if ( att != NULL )
		extension = att->value;
	else
		extension = GetDefaultModuleExtension ();

	isUnicode = GetBooleanAttribute ( moduleNode, "unicode" );

	if (isUnicode)
	{
		// Always define UNICODE and _UNICODE
		Define* pDefine = new Define ( project, this, "UNICODE" );
		non_if_data.defines.push_back ( pDefine );

		pDefine = new Define ( project, this, "_UNICODE" );
		non_if_data.defines.push_back ( pDefine );
	}

	att = moduleNode.GetAttribute ( "entrypoint", false );
	if ( att != NULL )
	{
		if ( att->value == "" )
		{
			throw InvalidAttributeValueException (
				moduleNode.location,
				"entrypoint",
				att->value );
		}

		entrypoint = att->value;
		isDefaultEntryPoint = false;
	}
	else
	{
		entrypoint = GetDefaultModuleEntrypoint ();
		isDefaultEntryPoint = true;
	}

	att = moduleNode.GetAttribute ( "baseaddress", false );
	if ( att != NULL )
		baseaddress = att->value;
	else
		baseaddress = GetDefaultModuleBaseaddress ();

	mangledSymbols = GetBooleanAttribute ( moduleNode, "mangledsymbols" );

	att = moduleNode.GetAttribute ( "underscoresymbols", false );
	if ( att != NULL )
		underscoreSymbols = att->value == "true";
	else
		underscoreSymbols = false;

	isStartupLib = GetBooleanAttribute ( moduleNode, "isstartuplib" );
	isCRT = GetBooleanAttribute ( moduleNode, "iscrt", GetDefaultModuleIsCRT () );

	att = moduleNode.GetAttribute ( "crt", false );
	if ( att != NULL)
	{
		CRT = att->value;

		if ( stricmp ( CRT.c_str (), "auto" ) == 0 )
			CRT = GetDefaultModuleCRT ();
	}
	else
		CRT = GetDefaultModuleCRT ();

	const char * crtAttr = CRT.c_str ();
	if ( crtAttr == NULL || stricmp ( crtAttr, "none" ) == 0 )
		dynamicCRT = false;
	else if ( stricmp ( crtAttr, "libc" ) == 0 )
		dynamicCRT = false;
	else if ( stricmp ( crtAttr, "msvcrt" ) == 0 )
		dynamicCRT = true;
	else if ( stricmp ( crtAttr, "libcntpr" ) == 0 )
		dynamicCRT = false;
	else if ( stricmp ( crtAttr, "ntdll" ) == 0 )
		dynamicCRT = true;
	else if ( stricmp ( crtAttr, "static" ) == 0 )
		dynamicCRT = false;
	else if ( stricmp ( crtAttr, "dll" ) == 0 )
		dynamicCRT = true;
	else
	{
		throw InvalidAttributeValueException (
			moduleNode.location,
			"crt",
			std::string ( crtAttr ) );
	}

	if ( isCRT && dynamicCRT )
	{
		throw XMLInvalidBuildFileException (
			moduleNode.location,
			"C runtime module '%s' cannot be compiled for a dynamically-linked C runtime",
			name.c_str() );
	}

	att = moduleNode.GetAttribute ( "prefix", false );
	if ( att != NULL )
		prefix = att->value;

	att = moduleNode.GetAttribute ( "installname", false );
	if ( att != NULL )
	{
		const XMLAttribute* installbase = moduleNode.GetAttribute ( "installbase", false );
		install = new FileLocation ( InstallDirectory,
		                             installbase ? installbase->value : "",
		                             att->value,
		                             &moduleNode );

		output = new FileLocation ( GetTargetDirectoryTree (),
		                            modulePath,
		                            att->value,
		                            &moduleNode );
	}

	att = moduleNode.GetAttribute ( "output", false );
	if ( att != NULL )
	{
		if (output != NULL)
		{
			printf ( "%s: WARNING: 'installname' overrides 'output' also defined for this module.\n",
				moduleNode.location.c_str() );
		}
		else
		{
			output = new FileLocation ( GetTargetDirectoryTree (),
										modulePath,
										att->value,
										&moduleNode );
		}
	}

	/* If no one has set the output file for this module set it automatically */
	if (output == NULL)
	{
		output = new FileLocation ( GetTargetDirectoryTree (),
									modulePath,
									name + extension,
									&moduleNode );
	}

	att = moduleNode.GetAttribute ( "allowwarnings", false );
	if ( att == NULL )
	{
		att = moduleNode.GetAttribute ( "warnings", false );
		if ( att != NULL )
		{
			printf ( "%s: WARNING: 'warnings' attribute of <module> is deprecated, use 'allowwarnings' instead\n",
				moduleNode.location.c_str() );
		}
	}
	if ( att != NULL )
		allowWarnings = att->value == "true";
	else
		allowWarnings = false;

	att = moduleNode.GetAttribute ( "aliasof", false );
	if ( type == Alias && att != NULL )
		aliasedModuleName = att->value;
	else
		aliasedModuleName = "";

	if ( type == BootProgram )
	{
		att = moduleNode.GetAttribute ( "payload", true );
		payload = att->value;
	}

	if ( type == BootProgram || type == ElfExecutable )
	{
		att = moduleNode.GetAttribute ( "buildtype", false );
		if ( att != NULL )
		{
			buildtype = att->value;
		}
		else
		{
			buildtype = "BOOTPROG";
		}
	}

	att = moduleNode.GetAttribute ( "description", false );
	if (att != NULL )
	{
		description = project.ResolveProperties(att->value);
	}
	else
		description = "";

	att = moduleNode.GetAttribute ( "lcid", false );
	if (type == KeyboardLayout && att != NULL )
		lcid = att->value;
	else
		lcid = "";

	att = moduleNode.GetAttribute ( "layoutid", false );
	if (type == KeyboardLayout && att != NULL )
		layoutId = att->value;
	else
		layoutId = "";

	att = moduleNode.GetAttribute ( "layoutnameresid", false );
	if (type == KeyboardLayout && att != NULL )
		layoutNameResId = att->value;
	else
		layoutNameResId = "";

	SetImportLibrary ( NULL );
}

Module::~Module ()
{
	size_t i;
	for ( i = 0; i < invocations.size(); i++ )
		delete invocations[i];
	for ( i = 0; i < dependencies.size(); i++ )
		delete dependencies[i];
	for ( i = 0; i < compilerFlags.size(); i++ )
		delete compilerFlags[i];
	for ( i = 0; i < linkerFlags.size(); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < stubbedComponents.size(); i++ )
		delete stubbedComponents[i];
	if ( linkerScript )
		delete linkerScript;
	if ( pch )
		delete pch;
	if ( install )
		delete install;
	if ( metadata )
		delete metadata;
	if ( bootstrap )
		delete bootstrap;
	if ( importLibrary )
		delete importLibrary;
	if ( bootSector )
		delete 	bootSector;
	if ( dependency )
		delete 	dependency;
	if ( autoRegister )
		delete autoRegister;
	if ( output )
		delete output;
}

void
Module::ProcessXML()
{
	if ( type == Alias )
	{
		aliasedModuleName = project.ResolveProperties ( aliasedModuleName );
		if ( aliasedModuleName == name )
		{
			throw XMLInvalidBuildFileException (
				node.location,
				"module '%s' cannot link against itself",
				name.c_str() );
		}
		const Module* m = project.LocateModule ( aliasedModuleName );
		if ( !m && enabled )
		{
			throw XMLInvalidBuildFileException (
				node.location,
				"module '%s' trying to alias non-existant module '%s'",
				name.c_str(),
				aliasedModuleName.c_str() );
		}
	}

	size_t i;
	for ( i = 0; i < node.subElements.size(); i++ )
	{
		ParseContext parseContext;
		ProcessXMLSubElement ( *node.subElements[i], SourceDirectory, output->relative_path, parseContext );
	}
	for ( i = 0; i < invocations.size(); i++ )
		invocations[i]->ProcessXML ();
	for ( i = 0; i < dependencies.size(); i++ )
		dependencies[i]->ProcessXML ();
	for ( i = 0; i < compilerFlags.size(); i++ )
		compilerFlags[i]->ProcessXML();
	for ( i = 0; i < linkerFlags.size(); i++ )
		linkerFlags[i]->ProcessXML();
	for ( i = 0; i < stubbedComponents.size(); i++ )
		stubbedComponents[i]->ProcessXML();
	non_if_data.ProcessXML();
	if ( linkerScript )
		linkerScript->ProcessXML();
	if ( pch )
		pch->ProcessXML();
	if ( autoRegister )
		autoRegister->ProcessXML();
}

void
Module::ProcessXMLSubElement ( const XMLElement& e,
                               DirectoryLocation directory,
	                           const string& relative_path,
                               ParseContext& parseContext )
{
	CompilationUnit* pOldCompilationUnit = parseContext.compilationUnit;
	bool subs_invalid = false;
	string subpath ( relative_path );
	DirectoryLocation subdirectory = SourceDirectory;
	if ( e.name == "file" && e.value.size () > 0 )
	{
		bool first = false;
		const XMLAttribute* att = e.GetAttribute ( "first", false );
		if ( att != NULL )
		{
			if ( !stricmp ( att->value.c_str(), "true" ) )
				first = true;
			else if ( stricmp ( att->value.c_str(), "false" ) )
			{
				throw XMLInvalidBuildFileException (
					e.location,
					"attribute 'first' of <file> element can only be 'true' or 'false'" );
			}
		}
		string switches = "";
		att = e.GetAttribute ( "switches", false );
		if ( att != NULL )
			switches = att->value;
		if ( !cplusplus )
		{
			// check for c++ file
			string ext = GetExtension ( e.value );
			if ( !stricmp ( ext.c_str(), ".cpp" ) )
				cplusplus = true;
			else if ( !stricmp ( ext.c_str(), ".cc" ) )
				cplusplus = true;
			else if ( !stricmp ( ext.c_str(), ".cxx" ) )
				cplusplus = true;
		}
		File* pFile = new File ( directory,
		                         relative_path,
		                         e.value,
		                         first,
		                         switches,
		                         false );
		if ( parseContext.compilationUnit )
			parseContext.compilationUnit->AddFile ( pFile );
		else
		{
			CompilationUnit* pCompilationUnit = new CompilationUnit ( pFile );
			string ext = ToLower ( GetExtension ( e.value ) );
			if ( ext == ".idl" )
			{
				// put .idl files at the start of the module
				non_if_data.compilationUnits.insert (
					non_if_data.compilationUnits.begin(),
					pCompilationUnit );
			}
			else if ( ext == ".asm" || ext == ".s" )
			{
				// put .asm files at the end of the module
				non_if_data.compilationUnits.push_back ( pCompilationUnit );
				non_if_data.asmFiles++;
			}
			else
			{
				// put other files in the middle
				non_if_data.compilationUnits.insert (
					non_if_data.compilationUnits.end() - non_if_data.asmFiles,
					pCompilationUnit );
			}
		}
		non_if_data.files.push_back ( pFile );
		subs_invalid = true;
	}
	else if ( e.name == "library" && e.value.size () )
	{
		Library* pLibrary = new Library ( e, *this, e.value );
		non_if_data.libraries.push_back ( pLibrary );
		subs_invalid = true;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		const XMLAttribute* root = e.GetAttribute ( "root", false );
		assert(att);
		if ( root )
		{
			if ( root->value == "intermediate" )
				subdirectory = IntermediateDirectory;
			else if ( root->value == "output" )
				subdirectory = OutputDirectory;
			else
			{
				throw InvalidAttributeValueException (
					e.location,
					"root",
					root->value );
			}
		}
		subpath = GetSubPath ( this->project, e.location, relative_path, att->value );
	}
	else if ( e.name == "include" )
	{
		Include* include = new Include ( project, &e, this );
		non_if_data.includes.push_back ( include );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		Define* pDefine = new Define ( project, this, e );
		non_if_data.defines.push_back ( pDefine );
		subs_invalid = true;
	}
	else if ( e.name == "metadata" )
	{
		metadata = new Metadata ( e, *this );
		subs_invalid = false;
	}
	else if ( e.name == "invoke" )
	{
		invocations.push_back ( new Invoke ( e, *this ) );
		subs_invalid = false;
	}
	else if ( e.name == "dependency" )
	{
		dependencies.push_back ( new Dependency ( e, *this ) );
		subs_invalid = true;
	}
	else if ( e.name == "bootsector" )
	{
		bootSector = new Bootsector ( e, this );
		subs_invalid = true;
	}
	else if ( e.name == "importlibrary" )
	{
		if ( importLibrary )
		{
			throw XMLInvalidBuildFileException (
				e.location,
				"Only one <importlibrary> is valid per module" );
		}
		SetImportLibrary ( new ImportLibrary ( project, e, this ) );
		subs_invalid = true;
	}
	else if ( e.name == "if" || e.name == "ifnot" )
	{
		const XMLAttribute* name;
		name = e.GetAttribute ( "property", true );
		assert( name );
		const Property *property = project.LookupProperty( name->value );
		const string *PropertyValue;
		const string EmptyString;

		if (property)
		{
			PropertyValue = &property->value;
		}
		else
		{
			// Property does not exist, treat it as being empty
			PropertyValue = &EmptyString;
		}

		const XMLAttribute* value;
		value = e.GetAttribute ( "value", true );
		assert( value );

		bool negate = ( e.name == "ifnot" );
		bool equality = ( *PropertyValue == value->value );
		if ( equality == negate )
		{
			// Failed, skip this element
			if ( project.configuration.Verbose )
				printf("Skipping 'If' at %s\n", e.location.c_str () );
			return;
		}
		subs_invalid = false;
	}
	else if ( e.name == "compilerflag" )
	{
		CompilerFlag* pCompilerFlag = new CompilerFlag ( project, this, e );
		non_if_data.compilerFlags.push_back ( pCompilerFlag );
		subs_invalid = true;
	}
	else if ( e.name == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( project, this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "linkerscript" )
	{
		if ( linkerScript )
		{
			throw XMLInvalidBuildFileException (
				e.location,
				"Only one <linkerscript> is valid per module" );
		}
		size_t pos = e.value.find_last_of ( "/\\" );
		if ( pos == string::npos )
		{
			linkerScript = new LinkerScript (
				e, *this, new FileLocation ( SourceDirectory, relative_path, e.value, &e ) );
		}
		else
		{
			string dir = e.value.substr ( 0, pos );
			string name = e.value.substr ( pos + 1);
			linkerScript = new LinkerScript (
				e, *this, new FileLocation ( SourceDirectory, relative_path + sSep + dir, name, &e ) );
		}
		subs_invalid = true;
	}
	else if ( e.name == "component" )
	{
		stubbedComponents.push_back ( new StubbedComponent ( this, e ) );
		subs_invalid = false;
	}
	else if ( e.name == "property" )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<property> is not a valid sub-element of <module>" );
	}
	else if ( e.name == "bootstrap" )
	{
		bootstrap = new Bootstrap ( project, this, e );
		subs_invalid = true;
	}
	else if ( e.name == "pch" )
	{
		if ( pch )
		{
			throw XMLInvalidBuildFileException (
				e.location,
				"Only one <pch> is valid per module" );
		}
		size_t pos = e.value.find_last_of ( "/\\" );
		if ( pos == string::npos )
		{
			pch = new PchFile (
				e, *this, new FileLocation ( SourceDirectory, relative_path, e.value, &e ) );
		}
		else
		{
			string dir = e.value.substr ( 0, pos );
			string name = e.value.substr ( pos + 1);
			pch = new PchFile (
				e, *this, new FileLocation ( SourceDirectory, relative_path + sSep + dir, name, &e ) );
		}
		subs_invalid = true;
	}
	else if ( e.name == "compilationunit" )
	{
		if ( project.configuration.CompilationUnitsEnabled )
		{
			CompilationUnit* pCompilationUnit = new CompilationUnit ( &project, this, &e );
			non_if_data.compilationUnits.push_back ( pCompilationUnit );
			parseContext.compilationUnit = pCompilationUnit;
		}
		subs_invalid = false;
	}
	else if ( e.name == "autoregister" )
	{
		if ( autoRegister != NULL)
		{
			throw XMLInvalidBuildFileException (
				e.location,
				"there can be only one <%s> element for a module",
				e.name.c_str() );
		}
		autoRegister = new AutoRegister ( project, this, e );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subdirectory, subpath, parseContext );
	parseContext.compilationUnit = pOldCompilationUnit;
}

ModuleType
Module::GetModuleType ( const string& location, const XMLAttribute& attribute )
{
	if ( attribute.value == "buildtool" )
		return BuildTool;
	if ( attribute.value == "staticlibrary" )
		return StaticLibrary;
	if ( attribute.value == "hoststaticlibrary" )
		return HostStaticLibrary;
	if ( attribute.value == "objectlibrary" )
		return ObjectLibrary;
	if ( attribute.value == "kernel" )
		return Kernel;
	if ( attribute.value == "kernelmodedll" )
		return KernelModeDLL;
	if ( attribute.value == "kernelmodedriver" )
		return KernelModeDriver;
	if ( attribute.value == "nativedll" )
		return NativeDLL;
	if ( attribute.value == "nativecui" )
		return NativeCUI;
	if ( attribute.value == "keyboardlayout" )
		return KeyboardLayout;
	if ( attribute.value == "win32dll" )
		return Win32DLL;
	if ( attribute.value == "win32ocx" )
		return Win32OCX;
	if ( attribute.value == "win32cui" )
		return Win32CUI;
	if ( attribute.value == "win32gui" )
		return Win32GUI;
	if ( attribute.value == "win32scr" )
		return Win32SCR;
	if ( attribute.value == "bootloader" )
		return BootLoader;
	if ( attribute.value == "bootsector" )
		return BootSector;
	if ( attribute.value == "bootprogram" )
		return BootProgram;
	if ( attribute.value == "iso" )
		return Iso;
	if ( attribute.value == "liveiso" )
		return LiveIso;
	if ( attribute.value == "isoregtest" )
		return IsoRegTest;
	if ( attribute.value == "liveisoregtest" )
		return LiveIsoRegTest;
	if ( attribute.value == "test" )
		return Test;
	if ( attribute.value == "rpcserver" )
		return RpcServer;
	if ( attribute.value == "rpcclient" )
		return RpcClient;
	if ( attribute.value == "rpcproxy" )
		return RpcProxy;
	if ( attribute.value == "alias" )
		return Alias;
	if ( attribute.value == "idlheader" )
		return IdlHeader;
	if ( attribute.value == "embeddedtypelib" )
		return EmbeddedTypeLib;
	if ( attribute.value == "elfexecutable" )
		return ElfExecutable;
	if ( attribute.value == "cabinet" )
		return Cabinet;
	if ( attribute.value == "messageheader" )
		return MessageHeader;
	throw InvalidAttributeValueException ( location,
	                                       attribute.name,
	                                       attribute.value );
}

DirectoryLocation
Module::GetTargetDirectoryTree () const
{
	switch ( type )
	{
		case Kernel:
		case KernelModeDLL:
		case KeyboardLayout:
		case NativeDLL:
		case Win32DLL:
		case Win32OCX:
		case KernelModeDriver:
		case NativeCUI:
		case Win32CUI:
		case Test:
		case Win32SCR:
		case Win32GUI:
		case BuildTool:
		case BootLoader:
		case BootSector:
		case BootProgram:
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
		case ElfExecutable:
		case Cabinet:
			return OutputDirectory;
		case EmbeddedTypeLib:
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case Alias:
		case IdlHeader:
		case MessageHeader:
			return IntermediateDirectory;
		case TypeDontCare:
			break;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__,
	                                  "Invalid module type %d.",
	                                  type );
}

string
Module::GetDefaultModuleExtension () const
{
	switch (type)
	{
		case BuildTool:
			return ExePostfix;
		case BootProgram:
		case StaticLibrary:
		case HostStaticLibrary:
			return ".a";
		case ObjectLibrary:
			return ".o";
		case Kernel:
		case NativeCUI:
		case Win32CUI:
		case Win32GUI:
			return ".exe";
		case Win32SCR:
			return ".scr";

		case KernelModeDLL:
		case NativeDLL:
		case KeyboardLayout:
		case Win32DLL:
			return ".dll";
		case Win32OCX:
			return ".ocx";
		case KernelModeDriver:
		case BootLoader:
			return ".sys";
		case Cabinet:
			return ".cab";
		case BootSector:
			return ".o";
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
			return ".iso";
		case Test:
			return ".exe";
		case RpcServer:
		case RpcClient:
		case RpcProxy:
			return ".o";
		case Alias:
		case ElfExecutable:
		case IdlHeader:
		case MessageHeader:
			return "";
		case EmbeddedTypeLib:
			return ".tlb";
		case TypeDontCare:
			break;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

string
Module::GetDefaultModuleEntrypoint () const
{
	switch ( type )
	{
		case Kernel:
			return "KiSystemStartup";
		case KeyboardLayout:
		case KernelModeDLL:
		case KernelModeDriver:
            if (Environment::GetArch() == "arm") return "DriverEntry";
			return "DriverEntry@8";
		case NativeDLL:
            if (Environment::GetArch() == "arm") return "DllMainCRTStartup";
            return "DllMainCRTStartup@12";
		case NativeCUI:
            if (Environment::GetArch() == "arm") return "NtProcessStartup";
            return "NtProcessStartup@4";
		case Win32DLL:
		case Win32OCX:
            if (Environment::GetArch() == "arm") return "DllMain";
			return "DllMain@12";
		case Win32CUI:
		case Test:
			if ( isUnicode )
				return "wmainCRTStartup";
			else
				return "mainCRTStartup";
		case Win32SCR:
		case Win32GUI:
			if ( isUnicode )
				return "wWinMainCRTStartup";
			else
				return "WinMainCRTStartup";
		case BuildTool:
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case Alias:
		case BootProgram:
		case IdlHeader:
		case MessageHeader:
		case ElfExecutable:
		case EmbeddedTypeLib:
		case Cabinet:
			return "";
		case TypeDontCare:
			break;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

string
Module::GetDefaultModuleBaseaddress () const
{
	switch ( type )
	{
		case Kernel:
			return "0x80800000";
		case Win32DLL:
		case Win32OCX:
			return "0x10000000";
		case NativeDLL:
		case NativeCUI:
		case Win32CUI:
		case Test:
			return "0x00400000";
		case Win32SCR:
		case Win32GUI:
			return "0x00400000";
		case KeyboardLayout:
		case KernelModeDLL:
		case KernelModeDriver:
			return "0x00010000";
		case ElfExecutable:
			return "0xe00000";
		case BuildTool:
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case Alias:
		case BootProgram:
		case IdlHeader:
		case MessageHeader:
		case EmbeddedTypeLib:
		case Cabinet:
			return "";
		case TypeDontCare:
			break;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

std::string
Module::GetDefaultModuleCRT () const
{
	if ( isCRT )
		return "static";

	switch ( type )
	{
		case Kernel:
			return "static";
		case Win32DLL:
		case Win32OCX:
			return "msvcrt";
		case NativeDLL:
		case NativeCUI:
			return "ntdll";
		case Win32CUI:
		case Win32SCR:
		case Win32GUI:
			return "msvcrt";
		case Test:
			return "msvcrt"; // BUGBUG: not sure about this
		case KeyboardLayout:
			return "none";
		case KernelModeDLL:
		case KernelModeDriver:
			return "dll";
		case BootLoader:
			return "libcntpr";
		case ElfExecutable:
		case BuildTool:
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case BootSector:
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case Alias:
		case BootProgram:
		case IdlHeader:
		case MessageHeader:
		case EmbeddedTypeLib:
		case Cabinet:
		case TypeDontCare:
			return "none";
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

bool
Module::GetDefaultModuleIsCRT () const
{
	return type == Kernel;
}

bool
Module::HasImportLibrary () const
{
	return importLibrary != NULL && type != StaticLibrary && type != HostStaticLibrary;
}

bool
Module::IsDLL () const
{
	switch ( type )
	{
		case Kernel:
		case KernelModeDLL:
		case NativeDLL:
		case KeyboardLayout:
		case Win32DLL:
		case Win32OCX:
		case KernelModeDriver:
			return true;
		case NativeCUI:
		case Win32CUI:
		case Test:
		case Win32SCR:
		case Win32GUI:
		case BuildTool:
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case BootProgram:
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case Alias:
		case IdlHeader:
		case MessageHeader:
		case EmbeddedTypeLib:
		case ElfExecutable:
		case Cabinet:
			return false;
		case TypeDontCare:
			break;
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

string
Module::GetPathWithPrefix ( const string& prefix ) const
{
	return output->relative_path + cSep + prefix + output->name;
}

string
Module::GetPathToBaseDir () const
{
	string temp_path = output->relative_path;
	string result = "..\\";
	while(temp_path.find ('\\') != string::npos)
	{
		temp_path.erase (0, temp_path.find('\\')+1);
		result += "..\\";
	}
	return result;
}

string
Module::GetInvocationTarget ( const int index ) const
{
	return ssprintf ( "%s_invoke_%d",
	                  name.c_str (),
	                  index );
}

string
Module::GetEntryPoint(bool leadingUnderscore) const
{
	string result = "";
	if (entrypoint == "0" || entrypoint == "0x0")
		return "0";
	if (leadingUnderscore)
		result = "_";

	result += entrypoint;
	return result;
}

bool
Module::HasFileWithExtension (
	const IfableData& data,
	const std::string& extension ) const
{
	size_t i;
	for ( i = 0; i < data.compilationUnits.size (); i++ )
	{
		CompilationUnit* compilationUnit = data.compilationUnits[i];
		if ( compilationUnit->HasFileWithExtension ( extension ) )
			return true;
	}
	return false;
}

void
Module::InvokeModule () const
{
	for ( size_t i = 0; i < invocations.size (); i++ )
	{
		Invoke& invoke = *invocations[i];
		string command = FixSeparatorForSystemCommand(invoke.invokeModule->output->relative_path + "/" + invoke.invokeModule->output->name ) + " " + invoke.GetParameters ();
		printf ( "Executing '%s'\n\n", command.c_str () );
		int exitcode = system ( command.c_str () );
		if ( exitcode != 0 )
			throw InvocationFailedException ( command,
			                                  exitcode );
	}
}


void
Module::SetImportLibrary ( ImportLibrary* importLibrary )
{
	this->importLibrary = importLibrary;
	dependency = new FileLocation ( HasImportLibrary () ? IntermediateDirectory : output->directory,
	                                output->relative_path,
	                                HasImportLibrary () ? "lib" + name + ".a" : output->name );
}

std::string
Module::GetDllName () const
{
	if ( importLibrary && !importLibrary->dllname.empty() )
		return importLibrary->dllname;
	else if ( output )
		return output->name;
	else
		throw new InvalidOperationException ( __FILE__, __LINE__, "Module %s has no dllname." );
}

File::File ( DirectoryLocation directory,
             const string& relative_path,
             const string& name,
             bool _first,
             const string& _switches,
             bool _isPreCompiledHeader )
	: file ( directory, relative_path, name ),
	  first(_first),
	  switches(_switches),
	  isPreCompiledHeader(_isPreCompiledHeader)
{
}


void
File::ProcessXML()
{
}


std::string File::GetFullPath () const
{
	string directory ( "" );
	switch ( file.directory )
	{
		case SourceDirectory:
			break;
		case IntermediateDirectory:
			directory = Environment::GetIntermediatePath () + sSep;
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file.directory );
	}

	if ( file.relative_path.length () > 0 )
		directory += file.relative_path + sSep;


	return directory + file.name;
}


Library::Library ( const XMLElement& _node,
                   const Module& _module,
                   const string& _name )
	: node(&_node),
	  module(_module),
	  name(_name),
	  importedModule(_module.project.LocateModule(_name))
{
	if ( module.name == name )
	{
		throw XMLInvalidBuildFileException (
			node->location,
			"module '%s' cannot link against itself",
			name.c_str() );
	}
	if ( !importedModule )
	{
		throw XMLInvalidBuildFileException (
			node->location,
			"module '%s' trying to import non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
	}
}

Library::Library ( const Module& _module,
                   const string& _name )
	: node(NULL),
	  module(_module),
	  name(_name),
	  importedModule(_module.project.LocateModule(_name))
{
	if ( !importedModule )
	{
		throw XMLInvalidBuildFileException (
			"<internal>",
			"module '%s' trying to import non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
	}
}

void
Library::ProcessXML()
{
	if ( node && !module.project.LocateModule ( name ) )
	{
		throw XMLInvalidBuildFileException (
			node->location,
			"module '%s' is trying to link against non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
	}
}


Invoke::Invoke ( const XMLElement& _node,
                 const Module& _module )
	: node (_node),
	  module (_module)
{
}

void
Invoke::ProcessXML()
{
	const XMLAttribute* att = node.GetAttribute ( "module", false );
	if (att == NULL)
		invokeModule = &module;
	else
	{
		invokeModule = module.project.LocateModule ( att->value );
		if ( invokeModule == NULL )
		{
			throw XMLInvalidBuildFileException (
				node.location,
				"module '%s' is trying to invoke non-existant module '%s'",
				module.name.c_str(),
				att->value.c_str() );
		}
	}

	for ( size_t i = 0; i < node.subElements.size (); i++ )
		ProcessXMLSubElement ( *node.subElements[i] );
}

void
Invoke::ProcessXMLSubElement ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "input" )
	{
		for ( size_t i = 0; i < e.subElements.size (); i++ )
			ProcessXMLSubElementInput ( *e.subElements[i] );
	}
	else if ( e.name == "output" )
	{
		for ( size_t i = 0; i < e.subElements.size (); i++ )
			ProcessXMLSubElementOutput ( *e.subElements[i] );
	}
	if ( subs_invalid && e.subElements.size() > 0 )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
}

void
Invoke::ProcessXMLSubElementInput ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "inputfile" && e.value.size () > 0 )
	{
		input.push_back ( new InvokeFile (
			e, FixSeparator ( module.output->relative_path + cSep + e.value ) ) );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
}

void
Invoke::ProcessXMLSubElementOutput ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "outputfile" && e.value.size () > 0 )
	{
		output.push_back ( new InvokeFile (
			e, FixSeparator ( module.output->relative_path + cSep + e.value ) ) );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
}

void
Invoke::GetTargets ( string_list& targets ) const
{
	for ( size_t i = 0; i < output.size (); i++ )
	{
		InvokeFile& file = *output[i];
		targets.push_back ( NormalizeFilename ( file.name ) );
	}
}

string
Invoke::GetParameters () const
{
	string parameters ( "" );
	size_t i;
	for ( i = 0; i < output.size (); i++ )
	{
		if ( parameters.length () > 0)
			parameters += " ";
		InvokeFile& invokeFile = *output[i];
		if ( invokeFile.switches.length () > 0 )
		{
			parameters += invokeFile.switches + " ";
		}
		parameters += invokeFile.name;
	}

	for ( i = 0; i < input.size (); i++ )
	{
		if ( parameters.length () > 0 )
			parameters += " ";
		InvokeFile& invokeFile = *input[i];
		if ( invokeFile.switches.length () > 0 )
		{
			parameters += invokeFile.switches;
			parameters += " ";
		}
		parameters += invokeFile.name ;
	}

	return parameters;
}


InvokeFile::InvokeFile ( const XMLElement& _node,
                         const string& _name )
	: node (_node),
      name (_name)
{
	const XMLAttribute* att = _node.GetAttribute ( "switches", false );
	if (att != NULL)
		switches = att->value;
	else
		switches = "";
}

void
InvokeFile::ProcessXML()
{
}


Dependency::Dependency ( const XMLElement& _node,
                         const Module& _module )
	: node (_node),
	  module (_module),
	  dependencyModule (NULL)
{
}

void
Dependency::ProcessXML()
{
	dependencyModule = module.project.LocateModule ( node.value );
	if ( dependencyModule == NULL )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"module '%s' depend on non-existant module '%s'",
			module.name.c_str(),
			node.value.c_str() );
	}
}

Bootsector::Bootsector ( const XMLElement& _node,
                         const Module* _module )
	: node (_node),
	  module (_module),
	  bootSectorModule (NULL)
{
	if ( !IsSupportedModuleType ( module->type ) )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<bootsector> is not applicable for this module type." );
	}

	bootSectorModule = module->project.LocateModule ( node.value );
	if ( bootSectorModule == NULL )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"module '%s' depend on non-existant module '%s'",
			module->name.c_str(),
			node.value.c_str() );
	}

	if (bootSectorModule->type != BootSector)
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"module '%s' is referencing non BootSector module '%s'",
			module->name.c_str(),
			node.value.c_str() );
	}
}

void
Bootsector::ProcessXML()
{
}

bool
Bootsector::IsSupportedModuleType ( ModuleType type )
{
	if ( type == Iso ||
	     type == LiveIso ||
 	     type == IsoRegTest ||
		 type == LiveIsoRegTest )
	{
		return true;
	}

	return false;
}

Metadata::Metadata ( const XMLElement& _node,
                     const Module& _module )
	: node (_node),
	  module (_module)
{
	/* The module name */
	const XMLAttribute* att = _node.GetAttribute ( "name", false );
	if (att != NULL)
		name = att->value;
	else
		name = module.name;

	/* The module description */
	att = _node.GetAttribute ( "description", false );
	if (att != NULL)
		description = att->value;
	else
		description = "";

	/* The module version */
	att = _node.GetAttribute ( "version", false );
	if (att != NULL)
		version = att->value;
	else
		version = "";

	/* The module copyright */
	att = _node.GetAttribute ( "copyright", false );
	if (att != NULL)
		copyright = att->value;
	else
		copyright = "";

	att = _node.GetAttribute ( "url", false );
	if (att != NULL)
		url = att->value;
	else
		url = "";

	/* When was this module updated */
	att = _node.GetAttribute ( "date", false );
	if (att != NULL)
		date = att->value;
	else
		date = "?";

	/* When was this module updated */
	att = _node.GetAttribute ( "owner", false );
	if (att != NULL)
		owner = att->value;
	else
		owner = "ReactOS";
}


ImportLibrary::~ImportLibrary ()
{
	delete source;
}


ImportLibrary::ImportLibrary ( const Project& project,
                               const XMLElement& node,
                               const Module* module )
	: XmlNode ( project, node ),
	  module (module)
{
	DirectoryLocation directory = SourceDirectory;
	const Module* base = module;
	const XMLAttribute* dllname = node.GetAttribute ( "dllname", false );
	const XMLAttribute* definition = node.GetAttribute ( "definition", true );
	assert ( definition );

	string relative_path;
	const XMLAttribute* att = node.GetAttribute ( "base", false );
	if ( att )
	{
			base = project.LocateModule ( att->value );
			if ( !base )
				throw XMLInvalidBuildFileException (
					node.location,
					"<importlibrary> attribute 'base' references non-existant module '%s'",
					att->value.c_str() );

	}

	if ( base )
	{
		relative_path = base->output->relative_path;
		if ( node.value.length () > 0 && node.value != "." )
			relative_path += sSep + node.value;
	}
	else
		relative_path = node.value;

	att = node.GetAttribute ( "root", false );
	if ( att )
	{
		if ( att->value == "intermediate" )
			directory = IntermediateDirectory;
		else
			throw InvalidAttributeValueException ( node.location,
			                                       "root",
			                                       att->value );
	}

	if ( dllname )
		this->dllname = dllname->value;
	else if ( module->type == StaticLibrary || module->type == HostStaticLibrary )
		throw XMLInvalidBuildFileException (
		    node.location,
		    "<importlibrary> dllname attribute required." );

	size_t index = definition->value.find_last_of ( "/\\" );
	if ( index == string::npos )
	{
		source = new FileLocation ( directory,
		                            base->output->relative_path,
		                            definition->value,
		                            &node );
	}
	else
	{
		string dir = definition->value.substr ( 0, index );
		string name = definition->value.substr ( index + 1);
		source = new FileLocation ( directory,
		                            NormalizeFilename ( base->output->relative_path + sSep + dir ),
		                            name,
		                            &node );
	}
}


Property::Property ( const XMLElement& node_,
                     const Project& project_,
                     const Module* module_ )
	: project(project_), module(module_)
{
	const XMLAttribute* att;

	att = node_.GetAttribute ( "name", true );
	assert(att);
	name = project.ResolveProperties ( att->value );

	att = node_.GetAttribute ( "value", true );
	assert(att);
	value = att->value;

	att = node_.GetAttribute ( "internal", false );
	if ( att != NULL )
	{
		const char* p = att->value.c_str();
		if ( !stricmp ( p, "true" ) || !stricmp ( p, "yes" ) )
			isInternal = true;
		else if ( !stricmp ( p, "false" ) || !stricmp ( p, "no" ) )
			isInternal = false;
		else
		{
			throw InvalidAttributeValueException (
				node_.location,
				"internal",
				att->value );
		}
	}
	else
		isInternal = false;
}

Property::Property ( const Project& project_,
                     const Module* module_,
                     const std::string& name_,
                     const std::string& value_ )
	: project(project_), module(module_), name(name_), value(value_)
{
}

void
Property::ProcessXML()
{
}


PchFile::PchFile (
	const XMLElement& node_,
	const Module& module_,
	const FileLocation *file_ )
	: node(node_), module(module_), file(file_)
{
}

PchFile::~PchFile()
{
	delete file;
}

void
PchFile::ProcessXML()
{
}
