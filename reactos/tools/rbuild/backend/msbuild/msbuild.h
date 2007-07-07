/*
 * Copyright (C) 2007 Christoph von Wittich
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
#ifndef __MSBUILD_H__
#define __MSBUILD_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

class MsBuildConfiguration
{
	public:
		MsBuildConfiguration(const std::string &name = "");
		virtual ~MsBuildConfiguration() {}
		std::string name;
};

class MsBuildBackend : public Backend
{
	public:

		MsBuildBackend(Project &project,
		              Configuration& configuration);
		virtual ~MsBuildBackend() {}

		virtual void Process();

	private:

		FILE* m_MsBuildFile;

		std::vector<MsBuildConfiguration*> m_configurations;
		void _generate_makefile ( const Module& module );
		void _generate_sources ( const Module& module );
		void _clean_project_files ( void );
		void ProcessModules();
		const Property* _lookup_property ( const Module& module, const std::string& name ) const;
		std::string _replace_str(std::string string1, const std::string &find_str, const std::string &replace_str);

		struct module_data
		{
			std::vector <std::string> libraries;
			std::vector <std::string> references;
		
			module_data()
			{}
			~module_data()
			{}
		};

};


#endif // __MsBuild_H__

