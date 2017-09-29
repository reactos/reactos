/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a virtual test list for the tests to be ran
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CVirtualTestList : public CTestList
{
public:
    CVirtualTestList(CTest* Test);

    CTestInfo* GetNextTestInfo();
};
