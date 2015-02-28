/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Simple exception during test execution that can be skipped over
 * COPYRIGHT:   Copyright 2015 Thomas Faber <thomas.faber@reactos.org>
 */

class CTestException : public CSimpleException
{
public:
    CTestException(const string& Message) : CSimpleException(Message) { }
};
