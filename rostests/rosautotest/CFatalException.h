/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Fatal program exception with automatically added information
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CFatalException
{
private:
    string m_File;
    string m_Line;
    string m_Message;

public:
    CFatalException(const char* File, const char* Line, const char* Message);

    const string& GetFile() const { return m_File; }
    const string& GetLine() const { return m_Line; }
    const string& GetMessage() const { return m_Message; }
};
