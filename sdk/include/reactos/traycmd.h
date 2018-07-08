/*
 * Tray Commands
 *
 * Copyright 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * this library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * this library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TRAYCMD_H_
#define TRAYCMD_H_

/* TODO: Add more and implement them */
#define TRAYCMD_STARTMENU           305     /*              Same as IDMA_START. */
#define TRAYCMD_RUN_DIALOG          401     /* Implemented. Same as IDM_RUN. */
#define TRAYCMD_LOGOFF_DIALOG       402     /* Implemented. Same as IDM_LOGOFF. */
#define TRAYCMD_CASCADE             403     /* */
#define TRAYCMD_TILE_H              404     /* */
#define TRAYCMD_TILE_V              405     /* */
#define TRAYCMD_TOGGLE_DESKTOP      407     /* Implemented. */
#define TRAYCMD_DATE_AND_TIME       408     /* Implemented. */
#define TRAYCMD_TASKBAR_PROPERTIES  413     /* Implemented. Same as IDM_TASKBARANDSTARTMENU. */
#define TRAYCMD_MINIMIZE_ALL        415     /* Implemented. */
#define TRAYCMD_RESTORE_ALL         416     /* Implemented. Same as IDMA_RESTORE_OPEN. */
#define TRAYCMD_SHOW_DESKTOP        419     /* Implemented. */
#define TRAYCMD_SHOW_TASK_MGR       420     /* Implemented. */
#define TRAYCMD_CUSTOMIZE_TASKBAR   421     /* */
#define TRAYCMD_LOCK_TASKBAR        424     /* Implemented. */
#define TRAYCMD_HELP_AND_SUPPORT    503     /* Implemented. Same as IDM_HELPANDSUPPORT. */
#define TRAYCMD_CONTROL_PANEL       505     /*              Same as IDM_CONTROLPANEL. */
#define TRAYCMD_SHUTDOWN_DIALOG     506     /* Implemented. Same as IDM_SHUTDOWN. */
#define TRAYCMD_PRINTERS_AND_FAXES  510     /*              Same as IDM_PRINTERSANDFAXES. */
#define TRAYCMD_LOCK_DESKTOP        517     /* */
#define TRAYCMD_SWITCH_USER_DIALOG  5000    /* */
#define TRAYCMD_SEARCH_FILES        41093   /* Implemented. Same as IDMA_SEARCH. */
#define TRAYCMD_SEARCH_COMPUTERS    41094   /* Implemented. */

#endif  /* ndef TRAYCMD_H_ */
