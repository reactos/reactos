/*
* PROJECT:     ReactOS Device Managment
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/mscutils/devmgmt/devmgmt.c
* PURPOSE:     Bootstrap for the device manager
* COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
*/

#include "precomp.h"
#include "MainWindow.h"

int WINAPI
wWinMain(HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nCmdShow)
{
    CCharMapWindow CharMap;
    return CharMap.Create(hThisInstance, nCmdShow);
}
