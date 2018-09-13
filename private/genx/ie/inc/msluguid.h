//*********************************************************************
//*                  Microsoft Internet Explorer                     **
//*            Copyright(c) Microsoft Corp., 1996-1998               **
//*********************************************************************

#ifndef _MSLUGUID_H_
#define _MSLUGUID_H_

// 95D0F020-451D-11CF-8DAB-00AA006C1A01
DEFINE_GUID(CLSID_LocalUsers, 0x95D0F020L, 0x451D, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01);

// 95D0F023-451D-11CF-8DAB-00AA006C1A01
DEFINE_GUID(IID_IUser,0x95D0F023L, 0x451D, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01);

// 95D0F022-451D-11CF-8DAB-00AA006C1A01
DEFINE_GUID(IID_IUserDatabase,0x95D0F023L, 0x451D, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01);

// 95D0F024-451D-11CF-8DAB-00AA006C1A01
DEFINE_GUID(IID_IUserProfileInit,0x95D0F024L, 0x451D, 0x11CF, 0x8D, 0xAB, 0x00, 0xAA, 0x00, 0x6C, 0x1A, 0x01);

#ifdef USER_SETTINGS_IMPLEMENTED
// EA7364C0-0730-11D0-83B1-00C04FD705B2
DEFINE_GUID(IID_IUserSettings,0xEA7364C0L, 0x0730, 0x11D0, 0x83, 0xB1, 0x00, 0xC0, 0x4F, 0xD7, 0x05, 0xB2);
#endif

#endif  // _MSLUGUID_H_
