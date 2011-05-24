/*
 * Copyright 2003, 2004 Martin Fuchs
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
 // externals.h
 //
 // Martin Fuchs, 07.06.2003
 //


#ifdef __cplusplus
extern "C" {
#endif


 // explorer main routine
extern int explorer_main(HINSTANCE hinstance, LPTSTR lpCmdLine, int cmdshow);

 // display explorer/file manager window
extern void explorer_show_frame(int cmdshow, LPTSTR lpCmdLine=NULL);

 // display explorer "About" dialog
extern void explorer_about(HWND hwndParent);

 // test for already running desktop instance
extern BOOL IsAnyDesktopRunning(void);

 // show shutdown dialog
extern void ShowExitWindowsDialog(HWND hwndOwner);

#ifdef __cplusplus
} // extern "C"
#endif

