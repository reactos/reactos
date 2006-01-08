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

#include "msvc.h"
#include "../mingw/mingw.h"

using namespace std;

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
		ReplaceExtension ( module.GetPath(), "_auto.vcproj" )
		);
}
