/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Simple exception containing just a message
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

class CSimpleException
{
private:
    string m_Message;

public:
    CSimpleException(const string& Message);

    const string& GetMessage() const { return m_Message; }
};
