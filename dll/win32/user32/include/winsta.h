/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/winsta.h
 * PURPOSE:     Window stations
 */

#ifndef USER32_WINSTA_H_INCLUDED
#define USER32_WINSTA_H_INCLUDED

extern BOOL FASTCALL EnumNamesA(HWINSTA WindowStation, NAMEENUMPROCA EnumFunc, LPARAM Context, BOOL Desktops);
extern BOOL FASTCALL EnumNamesW(HWINSTA WindowStation, NAMEENUMPROCW EnumFunc, LPARAM Context, BOOL Desktops);

#endif /* USER32_WINSTA_H_INCLUDED */
