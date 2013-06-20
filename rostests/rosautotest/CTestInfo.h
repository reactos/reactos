/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing a bucket for Test information
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CTestInfo
{
public:
    wstring CommandLine;
    string Module;
    string Test;
    string Log;
};
