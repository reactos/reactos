
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 4 - Utility functions
//
// Created by Damon Chandler <dmc27@ee.cornell.edu>
// Updates can be downloaded at: <www.coriolis.com>
//
// Please do not hesistate to e-mail me at dmc27@ee.cornell.edu 
// if you have any questions about this code.
// ------------------------------------------------------------------

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#ifndef CH4_UTILS_H
#define CH4_UTILS_H

#include <windows.h>
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// change namespace name appropriately to suit your needs
namespace font {

// font options
static const ULONG FS_NONE      = 0x00000000;
static const ULONG FS_BOLD      = 0x00000001;
static const ULONG FS_ITALIC    = 0x00000002;
static const ULONG FS_UNDERLINE = 0x00000004;
static const ULONG FS_STRIKEOUT = 0x00000008;

// creates a logical font
HFONT MakeFont(IN HDC hDestDC, IN LPCSTR typeface_name, 
   IN int point_size, IN const BYTE charset = ANSI_CHARSET, 
   IN const DWORD style = FS_NONE);

}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#endif // CH4_UTILS_H
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
