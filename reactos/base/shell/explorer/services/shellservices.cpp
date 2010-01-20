/*
 * Copyright 2005 Martin Fuchs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // shellservices.cpp
 //
 // Martin Fuchs, 28.03.2005
 //


#include <precomp.h>

#include "shellservices.h"


int SSOThread::Run()
{
	ComInit usingCOM(COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE|COINIT_SPEED_OVER_MEMORY);

	HKEY hkey;
	CLSID clsid;
	WCHAR name[MAX_PATH], value[MAX_PATH];

	typedef vector<SIfacePtr<IOleCommandTarget>*> SSOVector;
	SSOVector sso_ptrs;

	if (!RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad"), &hkey)) {
		for(int idx=0; ; ++idx) {
			DWORD name_len = MAX_PATH;
			DWORD value_len = sizeof(value);

			if (RegEnumValueW(hkey, idx, name, &name_len, 0, NULL, (LPBYTE)&value, &value_len))
				break;

			if (!_alive)
				break;

			SIfacePtr<IOleCommandTarget>* sso_ptr = new SIfacePtr<IOleCommandTarget>;

			if (CLSIDFromString(value, &clsid) == NOERROR) {
				if (SUCCEEDED(sso_ptr->CreateInstance(clsid, IID_IOleCommandTarget))) {
					if (SUCCEEDED((*sso_ptr)->Exec(&CGID_ShellServiceObject, OLECMDID_NEW, OLECMDEXECOPT_DODEFAULT, NULL, NULL)))
						sso_ptrs.push_back(sso_ptr);
				}
			}
		}

		RegCloseKey(hkey);
	}

	if (!sso_ptrs.empty()) {
		MSG msg;

		while(_alive) {
			if (MsgWaitForMultipleObjects(1, &_evtFinish, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0+0)
				break;	// _evtFinish has been set.

			while(_alive) {
				if (!PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
					break;

				if (msg.message == WM_QUIT)
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		 // shutdown all running Shell Service Objects
		for(SSOVector::iterator it=sso_ptrs.begin(); it!=sso_ptrs.end(); ++it) {
			SIfacePtr<IOleCommandTarget>* sso_ptr = *it;
			(*sso_ptr)->Exec(&CGID_ShellServiceObject, OLECMDID_SAVE, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
			delete sso_ptr;
		}
	}

	return 0;
}
