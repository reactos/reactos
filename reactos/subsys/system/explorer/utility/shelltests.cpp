/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 // 
 // shelltests.cpp
 //
 // Examples for usage of shellclasses.cpp, shellclasses.h
 //
 // Martin Fuchs, 20.07.2003
 //


//#define WIN32_LEAN_AND_MEAN
//#define WIN32_EXTRA_LEAN
//#include <windows.h>

#include "utility.h" // for String
#include "shellclasses.h"


static void dump_shell_namespace(ShellFolder& folder)
{
	ShellItemEnumerator enumerator(folder, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN|SHCONTF_SHAREABLE|SHCONTF_STORAGE);

	LPITEMIDLIST pidl;
	HRESULT hr = S_OK;

	do {
		ULONG cnt = 0;

		HRESULT hr = enumerator->Next(1, &pidl, &cnt);

		if (!SUCCEEDED(hr))
			break;

		if (hr == S_FALSE)	// no more entries?
			break;

		 if (pidl) {
			ULONG attribs = -1;

			HRESULT hr = folder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &attribs);

			if (SUCCEEDED(hr)) {
				if (attribs == -1)
					attribs = 0;

				const String& name = folder.get_name(pidl);

				if (attribs & (SFGAO_FOLDER|SFGAO_HASSUBFOLDER))
					cout << "folder: ";
				 else
					cout << "file: ";

				cout << "\"" << name << "\"\n attribs=" << hex << attribs << endl;
			}
		}
	} while(SUCCEEDED(hr));
}


int main()
{
	 // initialize COM
	ComInit usingCOM;


	HWND hwnd = 0;


	try {

		 // example for retrieval of special folder paths

		SpecialFolderFSPath programs(CSIDL_PROGRAM_FILES, hwnd);
		SpecialFolderFSPath autostart(CSIDL_STARTUP, hwnd);

		cout << "program files path = " << (LPCTSTR)programs << endl;
		cout << "autostart folder path = " << (LPCTSTR)autostart << endl;

		cout << endl;


		 // example for enumerating shell namespace objects

		cout << "Desktop:\n";
		dump_shell_namespace(GetDesktopFolder());
		cout << endl;

		cout << "C:\\\n";
		dump_shell_namespace(ShellPath("C:\\").get_folder());
		cout << endl;


		 // example for calling a browser dialog for the whole desktop

		FolderBrowser desktop_browser(hwnd,
									  BIF_RETURNONLYFSDIRS|BIF_EDITBOX|BIF_NEWDIALOGSTYLE,
									  TEXT("Please select the path:"));

		if (desktop_browser.IsOK())
			MessageBox(hwnd, desktop_browser, TEXT("Your selected path"), MB_OK);


		 // example for calling a rooted browser dialog

		ShellPath browseRoot("C:\\");
		FolderBrowser rooted_browser(hwnd,
									 BIF_RETURNONLYFSDIRS|BIF_EDITBOX|BIF_VALIDATE,
									 TEXT("Please select the path:"),
									 browseRoot);

		if (rooted_browser.IsOK())
			MessageBox(hwnd, rooted_browser, TEXT("Your selected path"), MB_OK);

	} catch(COMException& e) {

		//HandleException(e, hwnd);
		cerr << e.ErrorMessage() << endl;

	}

	return 0;
}
