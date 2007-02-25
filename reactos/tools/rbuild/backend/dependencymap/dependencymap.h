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
#ifndef __DEPMAP_H__
#define __DEPMAP_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

class DepMapConfiguration
{
	public:
		DepMapConfiguration(const std::string &name = "");
		virtual ~DepMapConfiguration() {}
		std::string name;
};

class DepMapBackend : public Backend
{
	public:

		DepMapBackend(Project &project,
		              Configuration& configuration);
		virtual ~DepMapBackend() {}

		virtual void Process();

	private:

		FILE* m_DepMapFile;

		std::vector<DepMapConfiguration*> m_configurations;
		void _generate_depmap ( FILE* OUT );
		void _clean_project_files ( void );

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


#endif // __DEPMAP_H__

