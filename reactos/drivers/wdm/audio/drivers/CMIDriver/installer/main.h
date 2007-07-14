/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include <windows.h>
#include <newdev.h>
#include <tchar.h>
#include "resource.h"

HINSTANCE    hInst;

const TCHAR* devIDs[] = {"PCI\\VEN_13F6&DEV_0111",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_011013F6",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_011113F6",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_1144153B",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_3731584D",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_87681092",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_020110B0",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_020210B0",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_020410B0",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_009C145F",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_39201462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_39801462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_50701462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_52801462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_53201462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_53401462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_54501462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_56501462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_59001462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_59201462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_70201462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_70401462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_70411462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_71011462",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_A016147A",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_30021919",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_0577A0A0",
                         "PCI\\VEN_13F6&DEV_0111&SUBSYS_060417AB",
                         };

const unsigned int devArraySize = sizeof(devIDs) / sizeof(devIDs[0]);

const char* CMIKeys[] = { "Cmaudio", "CmPCIaudio", "C-Media Mixer" };
const unsigned int NumberOfCMIKeys =  sizeof(CMIKeys) / sizeof(CMIKeys[0]);

//uninstaller stuff
const char DisplayName[] = _T("CMI 8738/8768 Audio Driver (remove only)");
const char DisplayIcon[] = _T("\\cmicontrol.exe,0");
const char Uninstaller[] = _T("\\cmicontrol.exe /uninstall");
const char Publisher[] = _T("Dogbert <dogber1@gmail.com>");
const char URLInfoAbout[] = _T("http://cmediadrivers.googlepages.com/");


#endif //_MAIN_H_