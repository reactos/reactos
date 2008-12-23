/*
 * Copyright (C) 2005 Trevor McCort
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2005 Steven Edwards
 * Copyright (C) 2005 Royce Mitchell
 * Copyright (C) 2006 Christoph von Wittich
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
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "msvc.h"

using std::string;
using std::vector;
using std::ifstream;

static class MSVCFactory : public Backend::Factory
{
	public:

		MSVCFactory() : Factory("MSVC", "Microsoft Visual C") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new MSVCBackend(project, configuration);
		}

} factory;


MSVCBackend::MSVCBackend(Project &project,
	Configuration& configuration) : Backend(project, configuration)
{
	m_unitCount = 0;
}

void MSVCBackend::Process()
{
	// TODO FIXME wine hack?
	bool only_msvc_headers = false;

	while ( m_configurations.size () > 0 )
	{
		const MSVCConfiguration* cfg = m_configurations.back();
		m_configurations.pop_back();
		delete cfg;
	}

	m_configurations.push_back ( new MSVCConfiguration( Debug ));
	m_configurations.push_back ( new MSVCConfiguration( Release ));
	m_configurations.push_back ( new MSVCConfiguration( Speed ));

	if (!only_msvc_headers)
	{
		m_configurations.push_back ( new MSVCConfiguration( Debug, ReactOSHeaders ));
		m_configurations.push_back ( new MSVCConfiguration( Release, ReactOSHeaders ));
		m_configurations.push_back ( new MSVCConfiguration( Speed, ReactOSHeaders ));
	}

	if ( configuration.CleanAsYouGo ) {
		_clean_project_files();
		return;
	}
	if ( configuration.InstallFiles ) {
		_install_files( _get_vc_dir(),  configuration.VSConfigurationType );
		return;
	}
	string filename_sln ( ProjectNode.name );

	if ( configuration.VSProjectVersion == "6.00" )
		filename_sln += "_auto.dsw";
	else
		filename_sln += "_auto.sln";

	printf ( "Creating MSVC workspace: %s\n", filename_sln.c_str() );

	ProcessModules();
	m_slnFile = fopen ( filename_sln.c_str(), "wb" );

	if ( !m_slnFile )
	{
		printf ( "Could not create file '%s'.\n", filename_sln.c_str() );
		return;
	}

	if ( configuration.VSProjectVersion == "6.00" )
		_generate_wine_dsw ( m_slnFile );
	else
		_generate_sln ( m_slnFile );

	fclose ( m_slnFile );
	printf ( "Done.\n" );
}

void MSVCBackend::ProcessModules()
{
	for(std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p)
	{
		Module &module = *p->second;

		module.guid = _gen_guid();

		if (configuration.VSProjectVersion == "6.00")
			_generate_dsp ( module );
		else
			_generate_vcproj ( module );
	}
}

static bool FileExists(string &filename)
{
	ifstream file(filename.c_str());

	if(!file.is_open())
		return false;

	file.close();
	return true;
}

void MSVCBackend::ProcessFile(string &filepath)
{
	// Remove the .\ at the start of the filenames
	if ( filepath[0] == '.' && strchr ( "/\\", filepath[1] ) )
		filepath.erase(0, 2);

	if(!FileExists(filepath))
		return;

	// Change the \ to /
	for(size_t i = 0; i < filepath.length(); i++)
	{
		if(filepath[i] == '\\')
			filepath[i] = '/';
	}

	// Remove the filename from the path
	string folder = "";

	size_t pos = filepath.rfind(string("/"), filepath.length() - 1);

	if(pos != string::npos)
	{
		folder = filepath;
		folder.erase(pos, folder.length() - pos);
	}

	FileUnit fileUnit;
	fileUnit.filename = filepath;
	fileUnit.folder = folder;

	m_fileUnits.push_back(fileUnit);

	if(folder != "")
		AddFolders(folder);

	m_unitCount++;
}

bool MSVCBackend::CheckFolderAdded(string &folder)
{
	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(m_folders[i] == folder)
			return true;
	}

	return false;
}

void MSVCBackend::AddFolders(string &folder)
{
	// Check if this folder was already added. true if it was, false otherwise.
	if(CheckFolderAdded(folder))
		return;

	m_folders.push_back(folder);

	size_t pos = folder.rfind(string("/"), folder.length() - 1);

	if(pos == string::npos)
		return;

	folder.erase(pos, folder.length() - pos);
	AddFolders(folder);
}

void MSVCBackend::OutputFolders()
{
#if 0
	m_devFile << "Folders=";

	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(i > 0)
			m_devFile << ",";

		m_devFile << m_folders[i];
	}
#endif
}

std::string
MSVCBackend::OptFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_" + _get_vc_dir() + "_auto.opt" )
		);
}

std::string
MSVCBackend::SuoFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_" + _get_vc_dir() + "_auto.suo" )
		);
}

std::string
MSVCBackend::DswFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_auto.dsw" )
		);
}

std::string
MSVCBackend::SlnFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_" + _get_vc_dir() + "_auto.sln" )
		);
}

std::string
MSVCBackend::NcbFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_" + _get_vc_dir() + "_auto.ncb" )
		);
}

std::string
MSVCBackend::DspFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_auto.dsp" )
		);
}

std::string
MSVCBackend::VcprojFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
			ReplaceExtension ( module.output->relative_path + "\\" + module.name, "_" + _get_vc_dir() + "_auto.vcproj" )
			);
}

std::string MSVCBackend::_get_vc_dir ( void ) const
{
	if ( configuration.VSProjectVersion == "6.00" )
		return "vc6";
	else if ( configuration.VSProjectVersion == "7.00" )
		return "vc70";
	else if ( configuration.VSProjectVersion == "7.10" )
		return "vc71";
	else if ( configuration.VSProjectVersion == "9.00" )
		return "vc9";
	else /* must be VS2005 */
		return "vc8";


}

void
MSVCBackend::_get_object_files ( const Module& module, vector<string>& out) const
{
	string basepath = module.output->relative_path;
	string vcdir = _get_vc_dir ();
	size_t i;
	string intenv = Environment::GetIntermediatePath () + DEF_SSEP + basepath + DEF_SSEP;
	string outenv = Environment::GetOutputPath () + DEF_SSEP + basepath + DEF_SSEP;

	if ( configuration.UseVSVersionInPath )
	{
		intenv += vcdir + DEF_SSEP;
		outenv += vcdir + DEF_SSEP;
	}

	string dbg = vcdir.substr ( 0, 3 );

	vector<string> cfgs;

	if ( configuration.UseConfigurationInPath )
	{
		cfgs.push_back ( intenv + "Debug" );
		cfgs.push_back ( intenv + "Release" );
		cfgs.push_back ( intenv + "Speed" );
		cfgs.push_back ( outenv + "Debug" );
		cfgs.push_back ( outenv + "Release" );
		cfgs.push_back ( outenv + "Speed" );
	}
	else
	{
		cfgs.push_back ( intenv );
		cfgs.push_back ( outenv );
	}

	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );
	while ( ifs_list.size () )
	{
		const IfableData& data = *ifs_list.back();
		ifs_list.pop_back();
		const vector<File*>& files = data.files;
		for ( i = 0; i < files.size (); i++ )
		{
			string file = files[i]->file.relative_path + sSep + files[i]->file.name;
			string::size_type pos = file.find_last_of (DEF_SSEP);
			if ( pos != string::npos )
				file.erase ( 0, pos+1 );
			if ( !_stricmp ( Right(file,3).c_str(), ".rc" ) )
				file = ReplaceExtension ( file, ".res" );
			else
				file = ReplaceExtension ( file, ".obj" );
			for ( size_t j = 0; j < cfgs.size () / 2; j++ )
				out.push_back ( cfgs[j] + file );
		}

	}
	//common files in intermediate dir
	for ( i = 0; i < cfgs.size () / 2; i++)
	{
		out.push_back ( cfgs[i] + "BuildLog.htm" );
		out.push_back ( cfgs[i] + dbg + "0.pdb" );
		out.push_back ( cfgs[i] + dbg + "0.idb" );
		out.push_back ( cfgs[i] + module.name + ".pch" );
	}
	//files in the output dir
	for ( i = cfgs.size () / 2; i < cfgs.size (); i++ )
	{
		out.push_back ( cfgs[i] + module.output->name );
		out.push_back ( cfgs[i] + module.name + ".pdb" );
		out.push_back ( cfgs[i] + module.name + ".lib" );
		out.push_back ( cfgs[i] + module.name + ".exp" );
		out.push_back ( cfgs[i] + module.name + ".ilk" );
	}
}

void
MSVCBackend::_get_def_files ( const Module& module, vector<string>& out) const
{
	if (module.HasImportLibrary ())
	{
#if 0
		string modulename = module.GetBasePath ();
		string file = module.importLibrary->definition;
		size_t pos = file.find (".def");
		if (pos != string::npos)
		{
			file.insert (pos, "_msvc");
		}
		modulename += DEF_SSEP + file;
		out.push_back (modulename);
#endif
	}
}

void
MSVCBackend::_clean_project_files ( void )
{
	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;
		vector<string> out;
		printf("Cleaning project %s %s %s\n", module.name.c_str (), module.output->relative_path.c_str (), NcbFileName ( module ).c_str () );

		string basepath = module.output->relative_path;
		remove ( NcbFileName ( module ).c_str () );
		remove ( DspFileName ( module ).c_str () );
		remove ( DswFileName ( module ).c_str () );
		remove ( OptFileName ( module ).c_str () );
		remove ( SlnFileName ( module ).c_str () );
		remove ( SuoFileName ( module ).c_str () );
		remove ( VcprojFileName ( module ).c_str () );

		string username = getenv ( "USERNAME" );
		string computername = getenv ( "COMPUTERNAME" );
		string vcproj_file_user = "";

		if ((computername != "") && (username != ""))
			vcproj_file_user = VcprojFileName ( module ) + "." + computername + "." + username + ".user";

		remove ( vcproj_file_user.c_str () );

		_get_object_files ( module, out );
		_get_def_files ( module, out );
		for ( size_t j = 0; j < out.size (); j++)
		{
			printf("Cleaning file %s\n", out[j].c_str () );
			remove ( out[j].c_str () );
		}
	}

	string filename_sln = ProjectNode.name + ".sln";
	string filename_dsw = ProjectNode.name + ".dsw";

	remove ( filename_sln.c_str () );
	remove ( filename_dsw.c_str () );
}

bool
MSVCBackend::_copy_file ( const std::string& inputname, const std::string& targetname ) const
{
	FILE * input = fopen ( inputname.c_str (), "rb" );
	if ( !input )
		return false;

	FILE * output = fopen ( targetname.c_str (), "wb+" );
	if ( !output )
	{
		fclose ( input );
		return false;
	}

	char buffer[256];
	int num_read;
	while ( (num_read = fread( buffer, sizeof(char), 256, input) ) || !feof( input ) )
		fwrite( buffer, sizeof(char), num_read, output );

	fclose ( input );
	fclose ( output );
	return true;
}

void
MSVCBackend::_install_files (const std::string& vcdir, const::string& config)
{
	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;
		if ( !module.install )
			continue;

		string inputname = Environment::GetOutputPath () + DEF_SSEP + module.output->relative_path + DEF_SSEP + vcdir + DEF_SSEP + config + DEF_SSEP + module.output->name;
		string installdir = Environment::GetInstallPath () + DEF_SSEP + module.install->relative_path + DEF_SSEP + module.install->name;
		if ( _copy_file( inputname, installdir ) )
			printf ("Installed File :'%s'\n",installdir.c_str () );
	}
}
