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
 // Explorer clone
 //
 // externals.h
 //
 // Martin Fuchs, 07.06.2003
 //


#ifdef __cplusplus
extern "C" {
#endif


 // launch start programs
extern int startup(int argc, char *argv[]);

 // explorer main routine
extern int explorer_main(HINSTANCE hinstance, HWND hwndParent, int cmdshow);

 // display explorer/file manager window
extern void explorer_show_frame(HWND hWndParent, int cmdshow);

 // create desktop window
extern HWND create_desktop_window(HINSTANCE hInstance);

 // test for already running desktop instance
extern BOOL IsAnyDesktopRunning();

 // start desktop bar
extern HWND InitializeExplorerBar(HINSTANCE hInstance);

 // load plugins
extern int LoadAvailablePlugIns(HWND ExplWnd);

 // shut down plugins
extern int ReleaseAvailablePlugIns();


#ifdef __cplusplus
} // extern "C"
#endif

