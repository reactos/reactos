/* Ch-Drive command for Windows NT and OS/2
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
   
   Bug:
   	the code will not work if you have more drives than those that
   	can fit in a panel.  
   */

#include <config.h>
#ifdef _OS_NT
#include <windows.h>
#include "util_win32.h"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../src/tty.h"
#include "../src/mad.h"
#include "../src/util.h"
#include "../src/win.h"
#include "../src/color.h"
#include "../src/dlg.h"
#include "../src/widget.h"
#include "../src/dialog.h"
#include "../src/dir.h"
#include "../src/panel.h"
#include "../src/main.h"
#include "../src/cmd.h"

struct Dlg_head *drive_dlg;
WPanel *this_panel;

static int drive_dlg_callback (Dlg_head *h, int Par, int Msg);
static void drive_dlg_refresh (void);
static void drive_cmd(void);

#define B_DRIVE_BASE 100
#define MAX_LGH 13		/* Length for drives */

static void drive_cmd()			
{
    int  i, nNewDrive, nDrivesAvail;
    char szTempBuf[7], szDrivesAvail[27*4], *p;

	/* Dialogbox position */
	int  x_pos;
	int  y_pos = (LINES-6)/2-3;
	int  y_height;
	int  x_width;

	int  m_drv;

    /* Get drives name and count */
#ifdef _OS_NT
    GetLogicalDriveStrings (255, szDrivesAvail);
    for (nDrivesAvail = 0, p = szDrivesAvail; *p; nDrivesAvail++)
	p+=4;
#else
    unsigned long uDriveNum, uDriveMap;
    nDrivesAvail = 0;
    p = szDrivesAvail;
    DosQueryCurrentDisk(&uDriveNum, &uDriveMap);
    for (i = 0; i < 26; i++) {
      if ( uDriveMap & (1 << i) ) {
	*p = 'A' + i;
	p += 4;
	nDrivesAvail++;
      }
    }
    *p = 0;
#endif

    /* Create Dialog */
    do_refresh ();
	
    m_drv = ((nDrivesAvail > MAX_LGH) ? MAX_LGH: nDrivesAvail);
    /* Center on x, relative to panel */
    x_pos = this_panel->widget.x + (this_panel->widget.cols - m_drv*3)/2 + 2;

    if (nDrivesAvail > MAX_LGH) {
	y_height = 8;
	x_width  = 33;
    } else {
	y_height = 6;
	x_width  = (nDrivesAvail - 1) * 2 + 9;
    }

    drive_dlg = create_dlg (y_pos, x_pos, y_height, x_width, dialog_colors,
	drive_dlg_callback, _("[ChDrive]"),_("drive"), DLG_NONE);

    x_set_dialog_title (drive_dlg, _("Change Drive") );

	if (nDrivesAvail>MAX_LGH) {
	    for (i = 0; i < nDrivesAvail - MAX_LGH; i++) {
		p -= 4;
		sprintf(szTempBuf, "&%c", *p);
		add_widgetl(drive_dlg,
		    button_new (5,
			(m_drv-i-1)*2+4 - (MAX_LGH*2 - nDrivesAvail) * 2,
			B_DRIVE_BASE + nDrivesAvail - i - 1,
			HIDDEN_BUTTON,
			szTempBuf, 0, NULL, NULL),
		    XV_WLAY_RIGHTOF);
		}
	}

    /* Add a button for each drive */
    for (i = 0; i < m_drv; i++) {
    	p -= 4;
    	sprintf (szTempBuf, "&%c", *p);
	    add_widgetl(drive_dlg,
		button_new (3, (m_drv-i-1)*2+4, B_DRIVE_BASE+m_drv-i-1,
		HIDDEN_BUTTON, szTempBuf, 0, NULL, NULL),
		XV_WLAY_RIGHTOF);
    }

    run_dlg(drive_dlg);   

    /* do action */
    if (drive_dlg->ret_value != B_CANCEL) {
	int  errocc = 0; /* no error */
	int  rtn;
	char drvLetter;
	
	/* Set the Panel to Directory listing mode first */
	int is_right=(this_panel==right_panel);

	set_display_type (is_right?1:0, view_listing);
	this_panel=is_right?right_panel:left_panel;
	/* */

	nNewDrive = drive_dlg->ret_value - B_DRIVE_BASE;
	drvLetter =  (char) *(szDrivesAvail + (nNewDrive*4));
#ifdef _OS_NT
	if (win32_GetPlatform() == OS_WinNT) {	/* Windows NT */
		rtn = _chdrive(drvLetter - 'A' + 1);
	} else {				/* Windows 95 */
		rtn = 1;
		SetCurrentDirectory(szDrivesAvail+(nNewDrive*4));
	}
#else
	rtn = DosSetDefaultDisk(nNewDrive + 1);
#endif
	if (rtn == -1) 
		errocc = 1;
	else {
		getcwd (this_panel->cwd, sizeof (this_panel->cwd)-2);
		if (toupper(drvLetter) == toupper(*(this_panel->cwd))) {
			clean_dir (&this_panel->dir, this_panel->count);
			this_panel->count = do_load_dir(&this_panel->dir,
				this_panel->sort_type,
				this_panel->reverse,
				this_panel->case_sensitive,
				this_panel->filter);
			this_panel->top_file = 0;
			this_panel->selected = 0;
			this_panel->marked = 0;
			this_panel->total = 0;
			show_dir(this_panel);
			reread_cmd();
		} else
			errocc = 1;
	}
    if (errocc)
	    message (1, _(" Error "), _(" Can't access drive %c: "),
		*(szDrivesAvail+(nNewDrive*4)) );
    }
    destroy_dlg (drive_dlg);
    repaint_screen ();
}


void drive_cmd_a()
{
    this_panel = left_panel;
    drive_cmd();
}

void drive_cmd_b()
{                                  
    this_panel = right_panel;
    drive_cmd();
}

void drive_chg(WPanel *panel)
{
    this_panel  = panel;
    drive_cmd();
}

static int drive_dlg_callback (Dlg_head *h, int Par, int Msg)
{
    switch (Msg) {
#ifndef HAVE_X
    case DLG_DRAW:
	drive_dlg_refresh ();
	break;
#endif
    }
    return 0;
}

static void drive_dlg_refresh (void)
{
    attrset (dialog_colors[0]);
    dlg_erase (drive_dlg);
    draw_box (drive_dlg, 1, 1, drive_dlg->lines-2, drive_dlg->cols-2);

    attrset (dialog_colors[2]);
    dlg_move (drive_dlg, 1, drive_dlg->cols/2 - 7);
    addstr (_(" Change Drive "));
}
