/*
 * Copyright (C) 2005 Trevor McCort
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
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <iostream>
#include <fstream>
#include <string>

#include "devcpp.h"

using namespace std;

static class DevCppFactory : public Backend::Factory
{
	public:

		DevCppFactory() : Factory("devcpp", "Dev C++") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new DevCppBackend(project, configuration);
		}

} factory;


DevCppBackend::DevCppBackend(Project &project,
                             Configuration& configuration) : Backend(project, configuration)
{
	m_unitCount = 0;
}

void DevCppBackend::Process()
{
	string filename = ProjectNode.name + ".dev";

	cout << "Creating Dev-C++ project: " << filename << endl;

	ProcessModules();

	m_devFile.open(filename.c_str());

	if(!m_devFile.is_open())
	{
		cout << "Could not open file." << endl;
		return;
	}

	m_devFile << "[Project]" << endl;

	m_devFile	<< "FileName="				<< filename 		<< endl
				<< "Name="					<< ProjectNode.name	<< endl
				<< "UnitCount="				<< m_unitCount		<< endl
				<< "Type=1"					<< endl
				<< "Ver=1"					<< endl
				<< "ObjFiles="				<< endl
				<< "Includes="				<< endl
				<< "Libs="					<< endl
				<< "PrivateResource="		<< endl
				<< "ResourceIncludes="		<< endl
				<< "MakeIncludes="			<< endl
				<< "Compiler="				<< endl
				<< "CppCompiler="			<< endl
				<< "Linker="				<< endl
				<< "IsCpp=1"				<< endl
				<< "Icon="					<< endl
				<< "ExeOutput="				<< endl
				<< "ObjectOutput="			<< endl
				<< "OverrideOutput=0"		<< endl
				<< "OverrideOutputName="	<< endl
				<< "HostApplication="		<< endl
				<< "CommandLine="			<< endl
				<< "UseCustomMakefile=1"	<< endl
				<< "CustomMakefile="		<< ProjectNode.makefile << endl
				<< "IncludeVersionInto=0"	<< endl
				<< "SupportXPThemes=0"		<< endl
				<< "CompilerSet=0"			<< endl

				<< "CompilerSettings=0000000000000000000000" << endl;

	OutputFolders();

	m_devFile << endl << endl;

	OutputFileUnits();

	m_devFile.close();

	// Dev-C++ needs a makefile, so use the MinGW backend to create one.

	cout << "Creating Makefile: " << ProjectNode.makefile << endl;

	Backend *backend = Backend::Factory::Create("mingw",
	                                            ProjectNode,
	                                            configuration );
	backend->Process();
	delete backend;

	cout << "Done." << endl << endl;

	cout	<< "You may want to disable Class browsing (see below) before you open this project in Dev-C++, as the "
			<< "parsing required for large projects can take quite awhile."
			<< endl << endl
			<< "(Tools->Editor Options->Class browsing->Enable class browsing check box)"
			<< endl << endl;
}

void DevCppBackend::ProcessModules()
{
	for(std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p)
	{
		Module &module = *p->second;

		for(size_t k = 0; k < module.non_if_data.files.size(); k++)
		{
			File &file = *module.non_if_data.files[k];

			ProcessFile( file.file.relative_path + sSep + file.file.name );
		}
	}
}

bool FileExists(string &filename)
{
	ifstream file(filename.c_str());

	if(!file.is_open())
		return false;

	file.close();
	return true;
}

void DevCppBackend::ProcessFile(string filepath)
{
	// Remove the .\ at the start of the filenames
	if ((filepath[0] == '.') && (filepath[1] == '\\')) filepath.erase(0, 2);

	if(!FileExists(filepath))
		return;

	// Change the \ to /

	for(size_t i = 0; i < filepath.length(); i++)
	{
		if(filepath[i] == '/')
			filepath[i] = '\\';
	}


	// Remove the filename from the path
	string folder = "";

	size_t pos = filepath.rfind(string("\\"), filepath.length() - 1);

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

bool DevCppBackend::CheckFolderAdded(string &folder)
{
	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(m_folders[i] == folder)
			return true;
	}

	return false;
}

void DevCppBackend::AddFolders(string &folder)
{
	// Check if this folder was already added. true if it was, false otherwise.
	if(CheckFolderAdded(folder))
		return;

	m_folders.push_back(folder);

	size_t pos = folder.rfind(string("\\"), folder.length() - 1);

	if(pos == string::npos)
		return;

	folder.erase(pos, folder.length() - pos);
	AddFolders(folder);
}

void DevCppBackend::OutputFolders()
{
	m_devFile << "Folders=";

	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(i > 0)
			m_devFile << ",";

		m_devFile << m_folders[i];
	}
}

void DevCppBackend::OutputFileUnits()
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
		m_devFile << "BuildCmd="			<< endl << endl;
	}
}
