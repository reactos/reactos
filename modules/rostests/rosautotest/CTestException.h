/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Simple exception during test execution that can be skipped over
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 */

class CTestException : public CSimpleException
{
public:
    CTestException(const string& Message) : CSimpleException(Message) { }
};
