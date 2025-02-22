/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Class implementing a virtual test list for the tests to be ran
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

/**
 * Constructs a CVirtualTestList object for an associated CTest-derived object.
 *
 * @param Test
 * Pointer to a CTest-derived object, for which this test list shall serve.
 */
CVirtualTestList::CVirtualTestList(CTest* Test)
    : CTestList(Test)
{
}

/**
 * Interface to other classes for receiving information about the next test to be run.
 *
 * @return
 * A pointer to a CTestInfo object, which contains all available information about the next test.
 * The caller needs to free that object.
 */
CTestInfo*
CVirtualTestList::GetNextTestInfo()
{
    return m_Test->GetNextTestInfo();
}
