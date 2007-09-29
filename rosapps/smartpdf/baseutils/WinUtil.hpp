/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef WINUTIL_HPP__
#define WINUTIL_HPP__

class WinProcess {
public:
    static WinProcess* Create(const char *cmd, char *args="");
    
private:
    WinProcess(PROCESS_INFORMATION *);  // we don't want just anyone to make us
    PROCESS_INFORMATION m_processInfo;
};
#endif

