
#include <string>
#include <vector>
#include <iostream>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define _FINDDATA_T_DEFINED
#include <io.h>
#include <time.h>

	typedef std::basic_string<TCHAR> string;
	typedef std::basic_istringstream<TCHAR> istringstream;

#ifdef UNICODE

	using std::wcout;
	using std::wcerr;
	using std::endl;

#define cout wcout
#define cerr wcerr

#else

	using std::cout;
	using std::cerr;
	using std::endl;

#endif

using std::vector;
using std::endl;


bool scan_dir(string current_dir, vector<string> & def_files)
{
	vector<string> vect;
	string val = current_dir;
	val.insert (val.length(), _T("\\*"));

	struct _tfinddatai64_t c_file;
	intptr_t hFile = _tfindfirsti64(val.c_str(), &c_file);

	if (hFile == -1L)
	{
		cerr << "scan_dir failed" << val << endl;
		return false;
	}

	do
	{

		do
		{
			TCHAR * pos;
			if ((pos = _tcsstr(c_file.name, _T(".def"))))
			{
				string modulename = c_file.name;
				if (modulename.find (_T(".spec.def")) != string::npos)
				{
					///
					/// ignore spec files
					///
					continue;
				}
				string fname = current_dir;
				fname.insert (fname.length (), _T("\\"));
				fname.insert (fname.length (), modulename);
				cout << "adding file to scan list: "<< fname << endl;
				def_files.push_back(fname);
			}
			if (c_file.attrib & _A_SUBDIR)
			{
				if (!_tcscmp(c_file.name, _T(".svn")))
				{
					///
					/// ignore .svn directories
					///
					continue;
				}
				if (c_file.name[0] != _T('.'))
				{
					string path = current_dir;
					path.insert (path.length(), _T("\\"));
					path.insert (path.length(), c_file.name);
					vect.push_back (path);
				}
			}

		}while(_tfindnexti64(hFile, &c_file) == 0);

		_findclose(hFile);
		hFile = -1L;

		while(!vect.empty ())
		{
			current_dir = vect.front ();
			vect.erase (vect.begin());
			val = current_dir;
			val.insert (val.length(), _T("\\*"));
			hFile = _tfindfirsti64(val.c_str(), &c_file);
			if (hFile != -1L)
			{
				break;
			}
		}

		if (hFile == -1L)
		{
			break;
		}

	}while(1);

	return !def_files.empty ();
}

bool readFile(string filename, vector<string> & file)
{
	FILE * fd = _tfopen(filename.c_str(), _T("rt"));
	if (!fd)
	{
		cerr << "Error: failed to open file " << filename << endl;
		return false;
	}

	do
	{
		TCHAR szBuffer[256];
		memset(szBuffer, 0x0, sizeof(szBuffer));

		if(_fgetts(szBuffer, sizeof(szBuffer) / sizeof(char), fd))
		{
			string line = szBuffer;
			file.push_back (line);
		}
	}while(!feof(fd));

	fclose(fd);
	return true;
}

bool has_cdecl_convention(string & line)
{
	if (line[0] == _T('_'))
	{
		return true;
	}
	return false;
}

bool has_fastcall_convention(string & line)
{
	if (line[0] == _T('@'))
	{
		return true;
	}
	return false;
}

bool has_stdcall_convention(string & line)
{
	if (!has_cdecl_convention(line) && !has_fastcall_convention(line))
	{
		size_t pos = line.find (_T("@"));
		if (pos == string::npos)
		{
			///
			/// the stdcall decorate is removed
			///
			return false;
		}
		assert(pos > 1);
		if (line[pos-1] == _T(' '))
		{
			///
			/// its an export ordinal 
			///
			return false;
		}

		return true;
	}
	else
	{
		return false;
	}
}


bool has_export_ordinal(string & line)
{
	if (line.find_last_of (_T(" @")) != string::npos)
	{
		return true;
	}
	return false;
}

void remove_stdcall_convention(string & line)
{
	size_t pos = line.find (_T("@"));
	assert(pos != string::npos);
	line.erase (pos, 1);
	
	while(_istdigit(line[pos]))
		line.erase (pos, 1);
}




bool convertFile(string filename, vector<string> & file)
{
	bool modified = false;
	cerr << "entered with: "<< filename << " size: " << file.size () << endl;

	for(size_t i = 0; i < file.size(); i++)
	{
		string & line = file[i];
		if (line[0] == _T(';'))
		{
			///
			/// line is a comment ignore
			///
			continue;
		}
		size_t pos = line.find (_T("="));
		if (pos != string::npos)
		{
			string part1 = line.substr (0, pos);
			string part2 = line.substr (pos+1, line.length () - pos - 1);
			if (has_stdcall_convention(part1))
			{
				modified = true;
				remove_stdcall_convention(part1);
			}
			if (has_stdcall_convention(part2))
			{
				modified = true;
				remove_stdcall_convention(part2);
			}
			line = part1;
			line.insert (line.length (), _T("="));
			line.insert (line.length (), part2);
		}
		else if (has_stdcall_convention(line))
		{
			modified = true;
			remove_stdcall_convention(line);
		}
		cout << line;
	}
	exit(0);
	return modified;
}

bool writeFile(string filename, vector<string> & file)
{
	FILE * fd = _tfopen(filename.c_str(), _T("wt"));
	if (!fd)
	{
		cerr << "Error: failed to open file " << filename << endl;
		return false;
	}

	for(size_t i = 0; i < file.size (); i++)
	{
		string & line = file[i];
		_fputts(line.c_str (), fd);
	}
	return true;
}


int _tmain(int argc, TCHAR ** argv)
{
	string current_dir;
	vector<string> def_files;

	if (argc < 2)
	{
		cout << "Usage: " << argv[0] << "path to source" << endl;
		return -1;
	}
	
	if (!scan_dir(argv[1], def_files) || def_files.size() == 0)
	{
		cout << "Error: found no def files or invalid directory" << endl;
		return -1;
	}

	for (size_t i = 0; i < def_files.size(); i++)
	{
		vector<string> file;
		file.clear ();
		if (readFile(def_files[i], file))
		{
			if (convertFile(def_files[i], file))
			{
				writeFile(def_files[i], file);
			}
		}
	}

	return 0;
}
