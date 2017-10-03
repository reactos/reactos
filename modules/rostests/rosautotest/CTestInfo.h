/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a bucket for Test information
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CTestInfo
{
public:
    wstring CommandLine;
    string Module;
    string Test;
    string Log;
};
