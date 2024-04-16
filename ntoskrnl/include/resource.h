/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation - Standard Bitmap Resources
 * COPYRIGHT:   Copyright 2010 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2015-2022 Hermès Bélusca-Maïto
 *
 * COMPATIBILITY NOTICE
 *
 * This is the list of all the bitmap overlay resources present in the
 * NT kernel of Windows XP, Windows Server 2003 and their variations.
 * See "How to change Windows XP boot logo" at
 * https://web.archive.org/web/20220926055242/https://www.reversing.be/article.php?story=20061209171938444
 * as well as the "Boot Editor for WinXP" program for more details.
 */

#pragma once

/* Options for SOS mode UI */
#define SOS_UI_NONE         0
#define SOS_UI_NEW          1
#define SOS_UI_LEGACY       2

#define IDB_BOOT_SCREEN     1
#define IDB_HIBERNATE_BAR   2
#define IDB_SHUTDOWN_MSG    3
#define IDB_BAR_DEFAULT     4
#define IDB_LOGO_DEFAULT    5

#define IDB_BAR_WKSTA       8
#define IDB_BAR_HOME        9

#define IDB_SERVER_LOGO     13

/* Header and Footer for SOS UI */
#if SOS_UI == SOS_UI_LEGACY
#define IDB_WKSTA_HEADER    6
#define IDB_WKSTA_FOOTER    7
#define IDB_SERVER_HEADER   14
#define IDB_SERVER_FOOTER   15
#elif SOS_UI != SOS_UI_NONE
#define IDB_HEADER_NEW      6
#endif

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
#define IDB_LOGO_XMAS       19
#define IDB_ROTATING_LINE   20
#define IDB_PROGRESS_BAR    21
#define IDB_COPYRIGHT       22

#define IDB_MAX_RESOURCES   IDB_COPYRIGHT
