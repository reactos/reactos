#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

string
FixSeparator ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], CBAD_SEP );
	while ( p )
	{
		*p++ = CSEP;
		p = strchr ( p, CBAD_SEP );
	}
	return s2;
}

string
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
GetDirectory ( const string& filename )
{
	size_t index = filename.find_last_of ( CSEP );
	if ( index == string::npos )
		return filename;
	else
		return filename.substr ( 0, index );
}

string
NormalizeFilename ( const string& filename )
{
	Path path;
	string normalizedPath = path.Fixup ( filename, true );
	string relativeNormalizedPath = path.RelativeFromWorkingDirectory ( normalizedPath );
	return FixSeparator ( relativeNormalizedPath );
}

IfableData::~IfableData()
{
	size_t i;
	for ( i = 0; i < files.size(); i++ )
		delete files[i];
	for ( i = 0; i < includes.size(); i++ )
		delete includes[i];
	for ( i = 0; i < defines.size(); i++ )
		delete defines[i];
	for ( i = 0; i < libraries.size(); i++ )
		delete libraries[i];
	for ( i = 0; i < properties.size(); i++ )
		delete properties[i];
	for ( i = 0; i < ifs.size(); i++ )
		delete ifs[i];
}

void IfableData::ProcessXML ()
{
	size_t i;
	for ( i = 0; i < files.size (); i++ )
		files[i]->ProcessXML ();
	for ( i = 0; i < includes.size (); i++ )
		includes[i]->ProcessXML ();
	for ( i = 0; i < defines.size (); i++ )
		defines[i]->ProcessXML ();
	for ( i = 0; i < libraries.size (); i++ )
		libraries[i]->ProcessXML ();
	for ( i = 0; i < properties.size(); i++ )
		properties[i]->ProcessXML ();
	for ( i = 0; i < ifs.size (); i++ )
		ifs[i]->ProcessXML ();
}

Module::Module ( const Project& project,
                 const XMLElement& moduleNode,
                 const string& modulePath )
	: project (project),
	  node (moduleNode),
	  importLibrary (NULL),
	  bootstrap (NULL)
{
	if ( node.name != "module" )
		throw Exception ( "internal tool error: Module created with non-<module> node" );

	path = FixSeparator ( modulePath );

	const XMLAttribute* att = moduleNode.GetAttribute ( "name", true );
	assert(att);
	name = att->value;

	att = moduleNode.GetAttribute ( "type", true );
	assert(att);
	type = GetModuleType ( node.location, *att );

	att = moduleNode.GetAttribute ( "extension", false );
	if ( att != NULL )
		extension = att->value;
	else
		extension = GetDefaultModuleExtension ();

	att = moduleNode.GetAttribute ( "entrypoint", false );
	if ( att != NULL )
		entrypoint = att->value;
	else
		entrypoint = GetDefaultModuleEntrypoint ();

	att = moduleNode.GetAttribute ( "baseaddress", false );
	if ( att != NULL )
		baseaddress = att->value;
	else
		baseaddress = GetDefaultModuleBaseaddress ();

	att = moduleNode.GetAttribute ( "mangledsymbols", false );
	if ( att != NULL )
		mangledSymbols = att->value != "false";
	else
		mangledSymbols = false;
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
}

void
Module::ProcessXML()
{
	size_t i;
	for ( i = 0; i < node.subElements.size(); i++ )
		ProcessXMLSubElement ( *node.subElements[i], path );
	for ( i = 0; i < invocations.size(); i++ )
		invocations[i]->ProcessXML ();
	for ( i = 0; i < dependencies.size(); i++ )
		dependencies[i]->ProcessXML ();
	for ( i = 0; i < compilerFlags.size(); i++ )
		compilerFlags[i]->ProcessXML();
	for ( i = 0; i < linkerFlags.size(); i++ )
		linkerFlags[i]->ProcessXML();
	non_if_data.ProcessXML();
}

void
Module::ProcessXMLSubElement ( const XMLElement& e,
                               const string& path,
                               If* pIf /*= NULL*/ )
{
	bool subs_invalid = false;
	string subpath ( path );
	if ( e.name == "file" && e.value.size () > 0 )
	{
		bool first = false;
		const XMLAttribute* att = e.GetAttribute ( "first", false );
		if ( att )
		{
			if ( !stricmp ( att->value.c_str(), "true" ) )
				first = true;
			else if ( stricmp ( att->value.c_str(), "false" ) )
				throw InvalidBuildFileException (
					e.location,
					"attribute 'first' of <file> element can only be 'true' or 'false'" );
		}
		File* pFile = new File ( FixSeparator ( path + CSEP + e.value ), first );
		if ( pIf )
			pIf->data.files.push_back ( pFile );
		else
			non_if_data.files.push_back ( pFile );
		subs_invalid = true;
	}
	else if ( e.name == "library" && e.value.size () )
	{
		Library* pLibrary = new Library ( e, *this, e.value );
		if ( pIf )
			pIf->data.libraries.push_back ( pLibrary );
		else
			non_if_data.libraries.push_back ( pLibrary );
		subs_invalid = true;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = FixSeparator ( path + CSEP + att->value );
	}
	else if ( e.name == "include" )
	{
		Include* include = new Include ( project, this, e );
		if ( pIf )
			pIf->data.includes.push_back ( include );
		else
			non_if_data.includes.push_back ( include );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		Define* pDefine = new Define ( project, this, e );
		if ( pIf )
			pIf->data.defines.push_back ( pDefine );
		else
			non_if_data.defines.push_back ( pDefine );
		subs_invalid = true;
	}
	else if ( e.name == "invoke" )
	{
		if ( pIf )
			throw InvalidBuildFileException (
				e.location,
				"<invoke> is not a valid sub-element of <if>" );
		invocations.push_back ( new Invoke ( e, *this ) );
		subs_invalid = false;
	}
	else if ( e.name == "dependency" )
	{
		if ( pIf )
			throw InvalidBuildFileException (
				e.location,
				"<dependency> is not a valid sub-element of <if>" );
		dependencies.push_back ( new Dependency ( e, *this ) );
		subs_invalid = true;
	}
	else if ( e.name == "importlibrary" )
	{
		if ( pIf )
			throw InvalidBuildFileException (
				e.location,
				"<importlibrary> is not a valid sub-element of <if>" );
		if ( importLibrary )
			throw InvalidBuildFileException (
				e.location,
				"Only one <importlibrary> is valid per module" );
		importLibrary = new ImportLibrary ( e, *this );
		subs_invalid = true;
	}
	else if ( e.name == "if" )
	{
		If* pOldIf = pIf;
		pIf = new If ( e, project, this );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( pIf );
		else
			non_if_data.ifs.push_back ( pIf );
		subs_invalid = false;
	}
	else if ( e.name == "compilerflag" )
	{
		compilerFlags.push_back ( new CompilerFlag ( project, this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( project, this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "property" )
	{
		throw InvalidBuildFileException (
			e.location,
			"<property> is not a valid sub-element of <module>" );
	}
	else if ( e.name == "bootstrap" )
	{
		bootstrap = new Bootstrap ( project, this, e );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
		throw InvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subpath, pIf );
}

ModuleType
Module::GetModuleType ( const string& location, const XMLAttribute& attribute )
{
	if ( attribute.value == "buildtool" )
		return BuildTool;
	if ( attribute.value == "staticlibrary" )
		return StaticLibrary;
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
	if ( attribute.value == "win32dll" )
		return Win32DLL;
	if ( attribute.value == "win32cui" )
		return Win32CUI;
	if ( attribute.value == "win32gui" )
		return Win32GUI;
	if ( attribute.value == "bootloader" )
		return BootLoader;
	if ( attribute.value == "bootsector" )
		return BootSector;
	if ( attribute.value == "iso" )
		return Iso;
	throw InvalidAttributeValueException ( location,
	                                       attribute.name,
	                                       attribute.value );
}

string
Module::GetDefaultModuleExtension () const
{
	switch (type)
	{
		case BuildTool:
			return EXEPOSTFIX;
		case StaticLibrary:
			return ".a";
		case ObjectLibrary:
			return ".o";
		case Kernel:
		case NativeCUI:
		case Win32CUI:
		case Win32GUI:
			return ".exe";
		case KernelModeDLL:
		case NativeDLL:
		case Win32DLL:
			return ".dll";
		case KernelModeDriver:
		case BootLoader:
			return ".sys";
		case BootSector:
			return ".o";
		case Iso:
			return ".iso";
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

string
Module::GetDefaultModuleEntrypoint () const
{
	switch (type)
	{
		case Kernel:
			return "_NtProcessStartup";
		case KernelModeDLL:
			return "_DriverEntry@8";
		case NativeDLL:
			return "_DllMainCRTStartup@12";
		case NativeCUI:
			return "_NtProcessStartup@4";
		case Win32DLL:
			return "_DllMain@12";
		case Win32CUI:
			return "_mainCRTStartup";
		case Win32GUI:
			return "_WinMainCRTStartup";
		case KernelModeDriver:
			return "_DriverEntry@8";
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
			return "";
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

string
Module::GetDefaultModuleBaseaddress () const
{
	switch (type)
	{
		case Kernel:
			return "0xc0000000";
		case KernelModeDLL:
			return "0x10000";
		case NativeDLL:
			return "0x10000";
		case NativeCUI:
			return "0x10000";
		case Win32DLL:
			return "0x10000";
		case Win32CUI:
			return "0x00400000";
		case Win32GUI:
			return "0x00400000";
		case KernelModeDriver:
			return "0x10000";
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
			return "";
	}
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

bool
Module::HasImportLibrary () const
{
	return importLibrary != NULL;
}

string
Module::GetTargetName () const
{
	return name + extension;
}

string
Module::GetDependencyPath () const
{
	if ( HasImportLibrary () )
	{
		return ssprintf ( "dk%snkm%slib%slib%s.a",
		                  SSEP,
		                  SSEP,
		                  SSEP,
		                  name.c_str () );
	}
	else
		return GetPath();
}

string
Module::GetBasePath () const
{
	return path;
}

string
Module::GetPath () const
{
	return path + CSEP + GetTargetName ();
}

string
Module::GetPathWithPrefix ( const string& prefix ) const
{
	return path + CSEP + prefix + GetTargetName ();
}

string
Module::GetTargets () const
{
	if ( invocations.size () > 0 )
	{
		string targets ( "" );
		for ( size_t i = 0; i < invocations.size (); i++ )
		{
			Invoke& invoke = *invocations[i];
			if ( targets.length () > 0 )
				targets += " ";
			targets += invoke.GetTargets ();
		}
		return targets;
	}
	else
		return GetPath ();
}

string
Module::GetInvocationTarget ( const int index ) const
{
	return ssprintf ( "%s_invoke_%d",
	                  name.c_str (),
	                  index );
}

bool
Module::HasFileWithExtension (
	const IfableData& data,
	const std::string& extension ) const
{
	size_t i;
	for ( i = 0; i < data.files.size (); i++ )
	{
		File& file = *data.files[i];
		string file_ext = GetExtension ( file.name );
		if ( !stricmp ( file_ext.c_str (), extension.c_str () ) )
			return true;
	}
	for ( i = 0; i < data.ifs.size (); i++ )
	{
		if ( HasFileWithExtension ( data.ifs[i]->data, extension ) )
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
		string command = invoke.invokeModule->GetPath () + " " + invoke.GetParameters ();
		printf ( "Executing '%s'\n\n", command.c_str () );
		int exitcode = system ( command.c_str () );
		if ( exitcode != 0 )
			throw InvocationFailedException ( command,
			                                  exitcode );
	}
}


File::File ( const string& _name, bool _first )
	: name(_name), first(_first)
{
}

void
File::ProcessXML()
{
}


Library::Library ( const XMLElement& _node,
                   const Module& _module,
                   const string& _name )
	: node(_node),
	  module(_module),
	  name(_name),
	  imported_module(_module.project.LocateModule(_name))
{
	if ( module.name == name )
		throw InvalidBuildFileException (
			node.location,
			"module '%s' cannot link against itself",
			name.c_str() );
	if ( !imported_module )
		throw InvalidBuildFileException (
			node.location,
			"module '%s' trying to import non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
}

void
Library::ProcessXML()
{
	if ( !module.project.LocateModule ( name ) )
		throw InvalidBuildFileException (
			node.location,
			"module '%s' is trying to link against non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
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
			throw InvalidBuildFileException (
				node.location,
				"module '%s' is trying to invoke non-existant module '%s'",
				module.name.c_str(),
				att->value.c_str() );
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
		throw InvalidBuildFileException ( e.location,
		                                  "<%s> cannot have sub-elements",
		                                  e.name.c_str() );
}

void
Invoke::ProcessXMLSubElementInput ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "inputfile" && e.value.size () > 0 )
	{
		input.push_back ( new InvokeFile ( e, FixSeparator ( module.path + CSEP + e.value ) ) );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
		throw InvalidBuildFileException ( e.location,
		                                  "<%s> cannot have sub-elements",
		                                  e.name.c_str() );
}

void
Invoke::ProcessXMLSubElementOutput ( const XMLElement& e )
{
	bool subs_invalid = false;
	if ( e.name == "outputfile" && e.value.size () > 0 )
	{
		output.push_back ( new InvokeFile ( e, FixSeparator ( module.path + CSEP + e.value ) ) );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() > 0 )
		throw InvalidBuildFileException ( e.location,
		                                  "<%s> cannot have sub-elements",
		                                  e.name.c_str() );
}

string
Invoke::GetTargets () const
{
	string targets ( "" );
	for ( size_t i = 0; i < output.size (); i++ )
	{
		InvokeFile& file = *output[i];
		if ( targets.length () > 0 )
			targets += " ";
		targets += NormalizeFilename ( file.name );
	}
	return targets;
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
			parameters += invokeFile.switches;
			parameters += " ";
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
		throw InvalidBuildFileException ( node.location,
		                                  "module '%s' depend on non-existant module '%s'",
		                                  module.name.c_str(),
		                                  node.value.c_str() );
}


ImportLibrary::ImportLibrary ( const XMLElement& _node,
                               const Module& _module )
	: node (_node),
	  module (_module)
{
	const XMLAttribute* att = _node.GetAttribute ( "basename", false );
	if (att != NULL)
		basename = att->value;
	else
		basename = module.name;

	att = _node.GetAttribute ( "definition", true );
	assert (att);
	definition = FixSeparator(att->value);
}


If::If ( const XMLElement& node_,
         const Project& project_,
         const Module* module_ )
	: node(node_), project(project_), module(module_)
{
	const XMLAttribute* att;

	att = node.GetAttribute ( "property", true );
	assert(att);
	property = att->value;

	att = node.GetAttribute ( "value", true );
	assert(att);
	value = att->value;
}

If::~If ()
{
}

void
If::ProcessXML()
{
}


Property::Property ( const XMLElement& node_,
                     const Project& project_,
                     const Module* module_ )
	: node(node_), project(project_), module(module_)
{
	const XMLAttribute* att;

	att = node.GetAttribute ( "name", true );
	assert(att);
	name = att->value;

	att = node.GetAttribute ( "value", true );
	assert(att);
	value = att->value;
}

void
Property::ProcessXML()
{
}
