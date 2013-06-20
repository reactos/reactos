/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Class for managing all the configuration parameters
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

class CConfiguration
{
private:
    bool m_CrashRecovery;
    bool m_IsReactOS;
    bool m_Shutdown;
    bool m_Submit;
    string m_Comment;
    wstring m_Module;
    string m_Test;

    string m_AuthenticationRequestString;
    string m_SystemInfoRequestString;

public:
    CConfiguration();
    void ParseParameters(int argc, wchar_t* argv[]);
    void GetSystemInformation();
    void GetConfigurationFromFile();

    bool DoCrashRecovery() const { return m_CrashRecovery; }
    bool DoShutdown() const { return m_Shutdown; }
    bool DoSubmit() const { return m_Submit; }
    bool IsReactOS() const { return m_IsReactOS; }
    const string& GetComment() const { return m_Comment; }
    const wstring& GetModule() const { return m_Module; }
    const string& GetTest() const { return m_Test; }

    const string& GetAuthenticationRequestString() const { return m_AuthenticationRequestString; }
    const string& GetSystemInfoRequestString() const { return m_SystemInfoRequestString; }
};
