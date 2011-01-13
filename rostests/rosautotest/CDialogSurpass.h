/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class for managing all the configuration parameters
 * COPYRIGHT:   Copyright 2011
 */

class CDialogSurpass
{
private:

    DWORD ThreadID;
    HANDLE hThread;
public:
    CDialogSurpass();
    ~CDialogSurpass();
};
