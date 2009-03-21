/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing a virtual test list for the tests to be ran
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CVirtualTestList : public CTestList
{
public:
    CVirtualTestList(CTest* Test);

    CTestInfo* GetNextTestInfo();
};
