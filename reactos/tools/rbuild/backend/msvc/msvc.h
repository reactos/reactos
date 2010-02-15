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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef __MSVC_H__
#define __MSVC_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"

#ifdef OUT
#undef OUT
#endif//OUT


class FileUnit
{
	public:
		std::string filename;
		std::string folder;
};

enum OptimizationType
{
	RosBuild,
	Debug,
	Release,
	Speed,
};

enum ConfigurationType
{
	ConfigUnknown,
	ConfigApp,
	ConfigDll,
	ConfigEmpty,
	ConfigLib
};

enum BinaryType
{
	BinUnknown,
	Lib,
	Dll,
	Exe,
	Sys
};

enum HeadersType
{
	MSVCHeaders,
	ReactOSHeaders
};

class MSVCConfiguration
{
	public:
		MSVCConfiguration(const OptimizationType optimization,
		                  const HeadersType headers = MSVCHeaders,
		                  const std::string &name = "");
		virtual ~MSVCConfiguration() {}
		std::string name;
		OptimizationType optimization;
		HeadersType headers;
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

		std::string VcprojFileName ( const Module& module ) const;
		std::string VcxprojFileName ( const Module& module ) const;
		std::string SlnFileName ( const Module& module ) const;
		std::string SuoFileName ( const Module& module ) const;
		std::string NcbFileName ( const Module& module ) const;

		std::vector<MSVCConfiguration*> m_configurations;

		std::vector<FileUnit> m_fileUnits;
		std::vector<std::string> m_folders;

		int m_unitCount;

		std::string _gen_guid();

		std::string _get_solution_version ( void );
		std::string _get_studio_version ( void );
		std::string _get_vc_dir ( void ) const;

		void _clean_project_files ( void );
		void _get_object_files ( const Module& module, std::vector<std::string>& out ) const;
		void _get_def_files ( const Module& module, std::vector<std::string>& out ) const;
		void _install_files ( const std::string& vcdir, const std::string& config );
		bool _copy_file ( const std::string& inputname, const std::string& targetname ) const;
		const Property* _lookup_property ( const Module& module, const std::string& name ) const;
};


// Abstract class
class ProjMaker
{
	public:
		ProjMaker ( );
		ProjMaker ( Configuration& buildConfig, const std::vector<MSVCConfiguration*>& msvc_configs, std::string filename );
		virtual ~ProjMaker() {}

		virtual void _generate_proj_file ( const Module& module ) = 0;
		virtual void _generate_user_configuration ();

	protected:
		Configuration configuration;
		std::vector<MSVCConfiguration*> m_configurations;
		std::string vcproj_file;
		FILE* OUT;

		std::vector<std::string> header_files;
		std::vector<std::string> includes;
		std::vector<std::string> includes_ros;
		std::vector<std::string> libraries;
		std::set<std::string> common_defines;
		std::string baseaddr;

		std::string VcprojFileName ( const Module& module ) const;
		std::string _get_vc_dir ( void ) const;

		std::string _strip_gcc_deffile(std::string Filename, std::string sourcedir, std::string objdir);
		std::string _get_solution_version ( void );
		std::string _get_studio_version ( void );
		std::string _replace_str( std::string string1, const std::string &find_str, const std::string &replace_str);

		void _generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType );
		void _generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg );
};

class VCProjMaker : public ProjMaker
{
	public:
		VCProjMaker ( );
		VCProjMaker ( Configuration& buildConfig, const std::vector<MSVCConfiguration*>& msvc_configs, std::string filename );
		virtual ~VCProjMaker ();

		void _generate_proj_file ( const Module& module );
		void _generate_user_configuration ();

	private:
		void _generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType );
		void _generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg );
		std::string _get_file_path( FileLocation* file, std::string relative_path);
};

class VCXProjMaker : public ProjMaker
{
	public:
		VCXProjMaker ( );
		VCXProjMaker ( Configuration& buildConfig, const std::vector<MSVCConfiguration*>& msvc_configs, std::string filename );
		virtual ~VCXProjMaker ();

		void _generate_proj_file ( const Module& module );
		void _generate_user_configuration ();

	private:
		void _generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType );
		void _generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg );
};

class SlnMaker
{
	public:
		SlnMaker ( Configuration& buildConfig, Project& ProjectNode, const std::vector<MSVCConfiguration*>& configurations, std::string filename_sln );
		~SlnMaker ();

		void _generate_sln ( std::string solution_version, std::string studio_version );

	private:
		Configuration m_configuration;
		Project* m_ProjectNode;
		std::vector<MSVCConfiguration*> m_configurations;
		FILE* OUT;

		void _generate_sln_header ( std::string solution_version, std::string studio_version );
		void _generate_sln_footer ( );
		//void _generate_rules_file ( FILE* OUT );
		void _generate_sln_project (
			const Module& module,
			std::string vcproj_file,
			std::string sln_guid,
			std::string vcproj_guid,
			const std::vector<Library*>& libraries );
		void _generate_sln_configurations ( std::string vcproj_guid );
};

class PropsMaker
{
	public:
		PropsMaker ( Configuration& buildConfig, 
			 Project* ProjectNode,  
			 std::string filename_props,
			 MSVCConfiguration* msvc_configs);

		~PropsMaker ();

		void _generate_props ( std::string solution_version, std::string studio_version );

	private:
		Configuration m_configuration;
		Project* m_ProjectNode;
		FILE* OUT;
		MSVCConfiguration* m_msvc_config;
		bool debug;
		bool release;
		bool speed;
		bool use_ros_headers;

		void _generate_header();
		void _generate_tools_defaults();
		void _generate_macro(std::string Name, std::string Value, bool EvairomentVariable);
		void _generate_global_includes();
		void _generate_global_definitions();
		void _generate_footer();

};
#endif // __MSVC_H__
