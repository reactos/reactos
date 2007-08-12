/*
 * Copyright (C) 2007 Marc Piulachs (marc.piulachs [at] codexchange [dot] net)
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
#ifndef __VREPORT_H__
#define __VREPORT_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

class VReportConfiguration
{
	public:
		VReportConfiguration(const std::string &name = "");
		virtual ~VReportConfiguration() {}
		std::string name;
};

class VReportBackend : public Backend
{
	public:

		VReportBackend(Project &project,
		              Configuration& configuration);
		virtual ~VReportBackend() {}

		virtual void Process();

	private:

		FILE* m_VReportFile;

		std::vector<VReportConfiguration*> m_configurations;
		
		void GenerateReport ( FILE* OUT );
		void CleanFiles ( void );

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


#endif // __VREPORT_H__
