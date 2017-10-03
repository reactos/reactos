/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a generic Test list, needs to be used by a derived class
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CTestList
{
protected:
    CTest* m_Test;

    CTestList(CTest* Test);

public:
    virtual CTestInfo* GetNextTestInfo() = 0;
};
