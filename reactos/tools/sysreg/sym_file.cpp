
/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     source symbol lookup
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "sym_file.h"
#include "env_var.h"
#include "pipe_reader.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>


namespace System_
{
	using std::cout;
	using std::endl;
	using std::cerr;

	string SymbolFile::VAR_ROS_OUTPUT = _T("ROS_OUTPUT");
//---------------------------------------------------------------------------------------
	SymbolFile::SymbolFile()
	{

	}

//---------------------------------------------------------------------------------------
	SymbolFile::~SymbolFile()
	{

	}

//---------------------------------------------------------------------------------------
	bool SymbolFile::initialize(const System_::string &Path) 
	{
		char szBuffer[260];
//		vector<string> vect;
		EnvironmentVariable envvar;

		string val = _T("output-i386");

		envvar.getValue(SymbolFile::VAR_ROS_OUTPUT, val);

		struct _finddata_t c_file;
		strcpy(szBuffer, "D:\\reactos\\output-i386\\*");
		intptr_t hFile = _findfirst(szBuffer, &c_file);

		if (hFile == -1L)
		{
			cerr << "SymbolFile::initialize> failed" <<endl;
			return false;
		}

		do
		{
			if (strstr(c_file.name, ".nostrip."))
			{
				cerr << c_file.name << endl;
			}

		}while(_findnext(hFile, &c_file) == 0);




		return false;
	}

//---------------------------------------------------------------------------------------
	bool SymbolFile::resolveAddress(const string &module_name, const string &module_address, string &Buffer) 
	{
		SymbolMap::const_iterator it = m_Map.find (module_name);
/*
		if (it == m_Map.end ())
		{
#ifdef NDEBUG
			cerr << "SymbolFile::resolveAddress> no symbol file found for module " << module_name << endl;
#endif
			return false;
		}
*/
		///
		/// fetch environment path
		///
		EnvironmentVariable envvar;
#if 1
		string pipe_cmd = _T("");//D:\\reactos\\output-i386");
#else
		string path = _T("output-i386");
		envvar.getValue(SymbolFile::VAR_ROS_OUTPUT, path);
#endif
		pipe_cmd += _T("addr2line.exe "); //FIXXME file extension
		pipe_cmd += _T("--exe=");
#if 1
		pipe_cmd += _T("D:\\reactos\\output-i386\\dll\\win32\\kernel32\\kernel32.nostrip.dll");
#else
		path += it->second;
#endif

		pipe_cmd += _T(" ");
		pipe_cmd += module_address;

 
		PipeReader pipe_reader;

		if (!pipe_reader.openPipe (pipe_cmd))
		{
#ifdef NDEBUG
			_tprintf(_T("SymbolFile::resolveAddress> failed to open pipe %s"), pipe_cmd);
#endif
			return false;
		}

		if (Buffer.capacity () < 100)
		{
			Buffer.reserve (500);
		}

		return pipe_reader.readPipe (Buffer);
	}

//---------------------------------------------------------------------------------------
	bool SymbolFile::getSymbolFilePath(const System_::string &ModuleName, System_::string &FilePath)
	{
		cerr << "SymbolFile::getSymbolFilePath is not yet implemented" <<endl;
		return false;

	}






} // end of namespace System_