
#include <string>
#include <vector>
#include <iostream>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
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


bool convertFile(string filename, vector<string> & file)
{
	bool modified = false;

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
		if (line.find(_T("@")) == string::npos)
		{
			// file has no @
			continue;
		}

		///
		/// TODO implement algorithm
		///

		//cout << ">" << line;
	}

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

	fclose(fd);
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
