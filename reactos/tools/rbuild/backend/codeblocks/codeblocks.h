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
#ifndef __CODEBLOCKS_H__
#define __CODEBLOCKS_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

class FileUnit
{
	public:
		std::string filename;
		std::string folder;
};

enum OptimizationType
{
	Debug,
	Release
};

class CBConfiguration
{
	public:
		CBConfiguration(const OptimizationType optimization,
		                  const std::string &name = "");
		virtual ~CBConfiguration() {}
		std::string name;
		OptimizationType optimization;
};

class CBBackend : public Backend
{
	public:

		CBBackend(Project &project,
		              Configuration& configuration);
		virtual ~CBBackend() {}

		virtual void Process();

	private:

		void ProcessModules();
		void ProcessFile(std::string &filename);

		bool CheckFolderAdded(std::string &folder);
		void AddFolders(std::string &folder);

		void OutputFolders();
		void OutputFileUnits();

		std::string CbpFileName ( const Module& module ) const;
		std::string LayoutFileName ( const Module& module ) const;
		std::string DependFileName ( const Module& module ) const;
		std::string GenerateProjectLinkerFlags () const;
		void MingwAddImplicitLibraries( Module &module );
		bool IsSpecDefinitionFile ( const Module& module ) const;
		std::vector<CBConfiguration*> m_configurations;

		std::vector<FileUnit> m_fileUnits;
		std::vector<std::string> m_folders;

		int m_unitCount;

		FILE* m_wrkspaceFile;

		std::string _replace_str(
			std::string string1,
			const std::string &find_str,
			const std::string &replace_str);

		void _generate_workspace ( FILE* OUT );
		void _generate_cbproj ( const Module& module );

		void _clean_project_files ( void );
		void _get_object_files ( const Module& module, std::vector<std::string>& out ) const;
		void _install_files ( const std::string& vcdir, const std::string& config );
		bool _copy_file ( const std::string& inputname, const std::string& targetname ) const;
		const Property* _lookup_property ( const Module& module, const std::string& name ) const;
};


#endif // __MSVC_H__

