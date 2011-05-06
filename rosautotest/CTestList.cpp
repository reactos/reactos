/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class implementing a generic Test list, needs to be used by a derived class
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Constructor serving for CTestList-derived classes.
 *
 * @param Test
 * Pointer to a CTest-derived object, for which this test list shall serve.
 */
CTestList::CTestList(CTest* Test) :
    m_Test(Test)
{
}
