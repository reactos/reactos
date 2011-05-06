/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing a generic Test list, needs to be used by a derived class
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CTestList
{
protected:
    CTest* m_Test;

    CTestList(CTest* Test);

public:
    virtual CTestInfo* GetNextTestInfo() = 0;
};
