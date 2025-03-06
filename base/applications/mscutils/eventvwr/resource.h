/*
 * PROJECT:     ReactOS Event Log Viewer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resources header.
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *              Copyright 2008-2016 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2016-2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* Icon IDs */
#define IDI_EVENTVWR          10
#define IDI_EVENTLOG          11
#define IDI_CLOSED_CATEGORY   12
#define IDI_OPENED_CATEGORY   13
#define IDI_INFORMATIONICON   14
#define IDI_WARNINGICON       15
#define IDI_ERRORICON         16
#define IDI_AUDITSUCCESSICON  17
#define IDI_AUDITFAILUREICON  18
#define IDI_NEXT              19
#define IDI_PREV              20
#define IDI_COPY              21


/* Accelerator IDs */
#define IDA_EVENTVWR          50


/* Dialog IDs */
#define IDD_EVENTDETAILS_DLG    101
#define IDD_EVENTDETAILS_CTRL   102
#define IDD_LOGPROPERTIES_GENERAL   103


/* Control IDs */
#define IDC_STATIC              -1
#define IDC_EVENTDATESTATIC     1000
#define IDC_EVENTSOURCESTATIC   1001
#define IDC_EVENTTIMESTATIC     1002
#define IDC_EVENTCATEGORYSTATIC 1003
#define IDC_EVENTTYPESTATIC     1004
#define IDC_EVENTIDSTATIC       1005
#define IDC_EVENTUSERSTATIC     1006
#define IDC_EVENTCOMPUTERSTATIC 1007
#define IDC_PREVIOUS            1008
#define IDC_NEXT                1009
#define IDC_COPY                1010
#define IDC_EVENTTEXTEDIT       1011
#define IDC_DETAILS_STATIC      -2
#define IDC_BYTESRADIO          1012
#define IDC_WORDRADIO           1013
#define IDC_EVENTDATAEDIT       1014
#define IDC_DISPLAYNAME         1015
#define IDC_LOGNAME             1016
#define IDC_LOGFILE             1017
#define IDC_SIZE_LABEL          1018
#define IDC_CREATED_LABEL       1019
#define IDC_MODIFIED_LABEL      1020
#define IDC_ACCESSED_LABEL      1021
#define IDC_EDIT_MAXLOGSIZE     1022
#define IDC_UPDOWN_MAXLOGSIZE   1023
#define IDC_OVERWRITE_AS_NEEDED 1024
#define IDC_OVERWRITE_OLDER_THAN 1025
#define IDC_EDIT_EVENTS_AGE     1026
#define IDC_UPDOWN_EVENTS_AGE   1027
#define IDC_NO_OVERWRITE        1028
#define IDC_RESTOREDEFAULTS     1029
#define IDC_LOW_SPEED_CONNECTION 1030
#define ID_CLEARLOG             1031

/* Menu IDs */
#define IDM_EVENTVWR            32770
#define IDM_OPEN_EVENTLOG       32771
#define IDM_SAVE_EVENTLOG       32772
#define IDM_CLOSE_EVENTLOG      32773
#define IDM_CLEAR_EVENTS        32774
#define IDM_RENAME_EVENTLOG     32775
#define IDM_EVENTLOG_SETTINGS   32776
#define IDM_EXIT                32777
#define IDM_LIST_NEWEST         32778
#define IDM_LIST_OLDEST         32779
#define IDM_EVENT_DETAILS       32780
#define IDM_REFRESH             32781
#define IDM_EVENT_DETAILS_VIEW  32782
#define IDM_LIST_GRID_LINES     32783
#define IDM_SAVE_SETTINGS       32784
#define IDM_HELP                32785
#define IDM_ABOUT               32786
#define IDM_EVENTWR_CTX         32787


/* String IDs */
#define IDS_COPYRIGHT                   100
#define IDS_APP_TITLE                   101
#define IDS_APP_TITLE_EX                102
#define IDS_STATUS_MSG                  103
#define IDS_LOADING_WAIT                104
#define IDS_NO_ITEMS                    105
#define IDS_EVENTLOG_SYSTEM             106
#define IDS_EVENTLOG_APP                107
#define IDS_EVENTLOG_USER               108
#define IDS_SAVE_FILTER                 109
#define IDS_CLEAREVENTS_MSG             110
#define IDS_EVENTSTRINGIDNOTFOUND       111
#define IDS_RESTOREDEFAULTS             112
#define IDS_CONTFROMBEGINNING           113
#define IDS_CONTFROMEND                 114

#define IDS_USAGE                       120
#define IDS_EVENTLOGFILE                121

#define IDS_EVENTLOG_ERROR_TYPE         200
#define IDS_EVENTLOG_WARNING_TYPE       201
#define IDS_EVENTLOG_INFORMATION_TYPE   202
#define IDS_EVENTLOG_AUDIT_SUCCESS      203
#define IDS_EVENTLOG_AUDIT_FAILURE      204
#define IDS_EVENTLOG_SUCCESS            205
#define IDS_EVENTLOG_UNKNOWN_TYPE       206

#define IDS_BYTES_FORMAT                210

#define IDS_COLUMNTYPE      220
#define IDS_COLUMNDATE      221
#define IDS_COLUMNTIME      222
#define IDS_COLUMNSOURCE    223
#define IDS_COLUMNCATEGORY  224
#define IDS_COLUMNEVENT     225
#define IDS_COLUMNUSER      226
#define IDS_COLUMNCOMPUTER  227

#define IDS_COPY            240

#define IDS_NONE            250
#define IDS_NOT_AVAILABLE   251
