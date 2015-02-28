/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Simple exception containing just a message
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CSimpleException
{
private:
    string m_Message;

public:
    CSimpleException(const string& Message);

    const string& GetMessage() const { return m_Message; }
};
