/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a generic Test, needs to be used by a derived class
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CTest
{
private:
    virtual CTestInfo* GetNextTestInfo() = 0;

public:
    virtual void Run() = 0;

    /* All CTestList-derived classes need to access the private GetNextTestInfo method */
    friend class CJournaledTestList;
    friend class CVirtualTestList;
};
