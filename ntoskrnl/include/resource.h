#pragma once

/*
 * List of all the bitmap overlay resources present in the NT kernel
 * of Windows XP, Windows Server 2003 and their variations.
 * See "How to change Windows XP boot logo" at
 * http://www.reversing.be/article.php?story=20061209171938444
 * as well as the "Boot Editor for WinXP" program for more details.
 */

#define IDB_BOOT_SCREEN     1
#define IDB_HIBERNATE_BAR   2
#define IDB_SHUTDOWN_MSG    3
#define IDB_BAR_DEFAULT     4
#define IDB_LOGO_DEFAULT    5

#define IDB_WKSTA_HEADER    6
#define IDB_WKSTA_FOOTER    7

#define IDB_BAR_WKSTA       8
#define IDB_BAR_HOME        9

#define IDB_SERVER_LOGO     13
#define IDB_SERVER_HEADER   14
#define IDB_SERVER_FOOTER   15

/* Workstation editions Overlays */
#define IDB_TEXT_PROF       10  // Professional
#define IDB_TEXT_HOME       11  // Home Edition
#define IDB_TEXT_EMBEDDED   12  // Embedded
#define IDB_TEXT_SVRFAMILY  13  // Server Family
#define IDB_TEXT_DOTNET     16  // .NET 2003
#define IDB_TEXT_TABLETPC   17  // Tablet PC Edition
#define IDB_TEXT_MEDIACTR   18  // Media Center Edition

/* Server editions Overlays */
#define IDB_STORAGE_SERVER  16  // Storage Server
#define IDB_CLUSTER_SERVER  17  // Compute Cluster Edition
#define IDB_STORAGE_SERVER2 18

/* ReactOS additions */
#define IDB_ROTATING_LINE   19

#define IDB_MAX_RESOURCE    IDB_ROTATING_LINE
