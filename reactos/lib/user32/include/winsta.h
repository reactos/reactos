/* $Id: winsta.h,v 1.2 2004/08/21 19:50:39 gvg Exp $
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
