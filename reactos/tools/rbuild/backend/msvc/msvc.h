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
#ifndef __MSVC_H__
#define __MSVC_H__

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

class MSVCBackend : public Backend
{
	public:

		MSVCBackend(Project &project,
		              Configuration& configuration);
		virtual ~MSVCBackend() {}

		virtual void Process();

	private:

		void ProcessModules();
		void ProcessFile(std::string &filename);
		
		bool CheckFolderAdded(std::string &folder);
		void AddFolders(std::string &folder);

		void OutputFolders();
		void OutputFileUnits();

		std::string DspFileName ( const Module& module ) const;
		
		std::vector<FileUnit> m_fileUnits;
		std::vector<std::string> m_folders;

		int m_unitCount;

		FILE* m_dswFile;

		// functions in msvcmaker.cpp:

		void _generate_dsw_header ( FILE* OUT );

		void _generate_dsw_project (
			FILE* OUT,
			const Module& module,
			std::string dsp_file,
			const std::vector<Dependency*>& dependencies );

		void _generate_dsw_footer ( FILE* OUT );

		void _generate_wine_dsw ( FILE* OUT );
};

#endif // __DEVCPP_H__

