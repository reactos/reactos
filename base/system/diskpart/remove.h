/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/remove.h
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#ifndef REMOVE_H_INCLUDED
#define REMOVE_H_INCLUDED

/* FUNCTIONS *****************************************************************/
BOOL remove_main(INT argc, WCHAR **argv);
VOID help_remove(INT argc, WCHAR **argv);

#endif // REMOVE_H_INCLUDED
