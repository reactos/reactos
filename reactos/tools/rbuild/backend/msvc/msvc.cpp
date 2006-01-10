/*
 * Copyright (C) 2005 Trevor McCort
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2005 Steven Edwards
 * Copyright (C) 2005 Royce Mitchell
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
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::ifstream;

static class MSVCFactory : public Backend::Factory
{
	public:

		MSVCFactory() : Factory("MSVC") {}
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
	if ( configuration.CleanAsYouGo ) {
		_clean_project_files();
		return;
	}

	string filename_sln ( ProjectNode.name );
	//string filename_rules = "gccasm.rules";
	
	if ( configuration.VSProjectVersion == "6.00" )
		filename_sln += ".dsw";
	else {
		filename_sln += ".sln";

		//m_rulesFile = fopen ( filename_rules.c_str(), "wb" );
		//if ( m_rulesFile )
		//{
		//	_generate_rules_file ( m_rulesFile );
		//}
		//fclose ( m_rulesFile );
	}

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
	for(size_t i = 0; i < ProjectNode.modules.size(); i++)
	{
		Module &module = *ProjectNode.modules[i];

		module.guid = _gen_guid();

		if (configuration.VSProjectVersion == "6.00")
			this->_generate_dsp ( module );
		else
			this->_generate_vcproj ( module );


		/*for(size_t k = 0; k < module.non_if_data.files.size(); k++)
		{
			File &file = *module.non_if_data.files[k];
			
			ProcessFile(file.name);
		}*/
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
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_" + _get_vc_dir() + "_auto.opt" )
		);
}

std::string
MSVCBackend::SuoFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_" + _get_vc_dir() + "_auto.suo" )
		);
}

std::string
MSVCBackend::DswFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_auto.dsw" )
		);
}

std::string
MSVCBackend::SlnFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_" + _get_vc_dir() + "_auto.sln" )
		);
}

std::string
MSVCBackend::NcbFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_" + _get_vc_dir() + "_auto.ncb" )
		);
}

std::string
MSVCBackend::DspFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), "_auto.dsp" )
		);
}

std::string
MSVCBackend::VcprojFileName ( const Module& module ) const
{
	return DosSeparator(
			ReplaceExtension ( module.GetPath(), "_" + _get_vc_dir() + "_auto.vcproj" )
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
	else /* must be VS2005 */
		return "vc8";


}

void 
MSVCBackend::_get_object_files ( const Module& module, vector<string>& out) const
{
	string basepath = module.GetBasePath ();
	string vcdir = _get_vc_dir ();
	size_t i;
	string intenv = Environment::GetIntermediatePath () + "\\" + basepath + "\\" + vcdir + "\\";
	string outenv = Environment::GetOutputPath () + "\\" + basepath + "\\" + vcdir + "\\";
	string dbg = vcdir.substr ( 0, 3 );

	vector<string> cfgs;
	cfgs.push_back ( intenv + "Debug" );
	cfgs.push_back ( intenv + "Release" );
	cfgs.push_back ( intenv + "Speed" );
	cfgs.push_back ( outenv + "Debug" );
	cfgs.push_back ( outenv + "Release" );
	cfgs.push_back ( outenv + "Speed" );


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
			string file = files[i]->name;
			string::size_type pos = file.find_last_of ("\\");
			if ( pos != string::npos )
				file.erase ( 0, pos+1 );
			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				file = ReplaceExtension ( file, ".res" );
			else
				file = ReplaceExtension ( file, ".obj" );
			for ( size_t j = 0; j < cfgs.size () / 2; j++ )
				out.push_back ( cfgs[j] + "\\" + file );
		}

	}
	//common files in intermediate dir
	for ( i = 0; i < cfgs.size () / 2; i++)
	{
		out.push_back ( cfgs[i] + "\\" + "BuildLog.htm" );
		out.push_back ( cfgs[i] + "\\" + dbg + "0.pdb" );
		out.push_back ( cfgs[i] + "\\" + dbg + "0.idb" );
		out.push_back ( cfgs[i] + "\\" + module.name + ".pch" );
	}
	//files in the output dir
	for ( i = cfgs.size () / 2; i < cfgs.size (); i++ )
	{
		out.push_back ( cfgs[i] + "\\" + module.GetTargetName () );
		out.push_back ( cfgs[i] + "\\" + module.name + ".pdb" );
		out.push_back ( cfgs[i] + "\\" + module.name + ".lib" );
		out.push_back ( cfgs[i] + "\\" + module.name + ".exp" );
		out.push_back ( cfgs[i] + "\\" + module.name + ".ilk" );
		out.push_back ( cfgs[i] + "\\" + "(InputName).obj" ); //MSVC2003 build bug 
	}
}

void
MSVCBackend::_clean_project_files ( void )
{
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		vector<string> out;
		printf("Cleaning project %s %s %s\n", module.name.c_str (), module.GetBasePath ().c_str (), NcbFileName ( module ).c_str () );
		
		string basepath = module.GetBasePath ();
		remove ( NcbFileName ( module ).c_str () );
		remove ( DspFileName ( module ).c_str () );
		remove ( DswFileName ( module ).c_str () );
		remove ( OptFileName ( module ).c_str () );
		remove ( SlnFileName ( module ).c_str () );
		remove ( SuoFileName ( module ).c_str () );
		remove ( VcprojFileName ( module ).c_str () );	

		_get_object_files ( module, out );
		for ( size_t j = 0; j < out.size (); j++)
			remove ( out[j].c_str () );
	}
	string filename_sln = ProjectNode.name + ".sln";
	string filename_dsw = ProjectNode.name + ".dsw";

	remove ( filename_sln.c_str () );
	remove ( filename_dsw.c_str () );
}

