/*
 * Copyright (C) 2005 Trevor McCort
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2005 Steven Edwards
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

#include "MSVC.h"

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
	string filename = ProjectNode.name + ".sln";
	
	cout << "Creating MSVC project: " << filename << endl;

	ProcessModules();

	m_devFile.open(filename.c_str());

	if(!m_devFile.is_open())
	{
		cout << "Could not open file." << endl;
		return;
	}
	m_devFile << "Microsoft Visual Studio Solution File, Format Version 9.00" << endl;
	m_devFile << "# Visual C++ Express 2005" << endl;

	m_devFile << "# FIXME Project listings here" << endl;
	m_devFile << "EndProject" << endl;
	m_devFile << "Global" << endl;
	m_devFile << "	GlobalSection(SolutionConfigurationPlatforms) = preSolution" << endl;
	m_devFile << "		Debug|Win32 = Debug|Win32" << endl;
	m_devFile << "		Release|Win32 = Release|Win32" << endl;
	m_devFile << "	EndGlobalSection" << endl;
	m_devFile << "	GlobalSection(ProjectConfigurationPlatforms) = postSolution" << endl;
	m_devFile << "	#FIXME Project Listings Here" << endl;
	m_devFile << "	EndGlobalSection" << endl;
	m_devFile << "	GlobalSection(SolutionProperties) = preSolution" << endl;
	m_devFile << "		HideSolutionNode = FALSE" << endl;
	m_devFile << "	EndGlobalSection" << endl;
	m_devFile << "EndGlobal" << endl;

	OutputFolders();

	m_devFile << endl << endl;

	OutputFileUnits();

	m_devFile.close();
	
	// The MSVC build still needs the mingw backend.
	
	cout << "Done." << endl << endl;

	cout << "Don't expect the MSVC backend to work yet. "<< endl << endl;
}

void MSVCBackend::ProcessModules()
{
	for(size_t i = 0; i < ProjectNode.modules.size(); i++)
	{
		Module &module = *ProjectNode.modules[i];

		for(size_t k = 0; k < module.non_if_data.files.size(); k++)
		{
			File &file = *module.non_if_data.files[k];
			
			ProcessFile(file.name);
		}
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
	m_devFile << "Folders=";

	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(i > 0)
			m_devFile << ",";

		m_devFile << m_folders[i];
	}
}

void MSVCBackend::OutputFileUnits()
{
	for(size_t i = 0; i < m_fileUnits.size(); i++)
	{
		m_devFile << "[Unit" << i + 1 << "]" << endl;
		

		m_devFile << "FileName="			<< m_fileUnits[i].filename << endl;
		m_devFile << "CompileCpp=1" 		<< endl;
		m_devFile << "Folder=" 				<< m_fileUnits[i].folder << endl;
		m_devFile << "Compile=1"			<< endl;
		m_devFile << "Link=1" 				<< endl;
		m_devFile << "Priority=1000"		<< endl;
		m_devFile << "OverrideBuildCmd=0"	<< endl;
		m_devFile << "BuildCmd="			<< endl << endl;;
	}
}
