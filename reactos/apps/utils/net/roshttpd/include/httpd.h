/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/httpd.h
 */
#ifndef __HTTPD_H
#define __HTTPD_H

#include <thread.h>
#include <socket.h>
#include <http.h>

#define HTTPD_START     WM_USER + 1
#define HTTPD_STOP      WM_USER + 2
#define HTTPD_SUSPEND   WM_USER + 3
#define HTTPD_RESUME    WM_USER + 4

enum HTTPdState {
    hsStopped = 0,
    hsRunning,
    hsSuspended
};

class CHttpDaemon;

class CHttpClient : public CServerClientSocket {
public:
	CHttpClient();
    CHttpClient(LPCServerSocket lpServerSocket);
	virtual void OnRead();
	//virtual void OnWrite();
	virtual void OnClose();
    HANDLE ThreadHandle;
    DWORD ThreadId;
    CHttpParser Parser;
    void SplitUri(const LPSTR lpsUri, LPSTR lpsHost, LPSTR lpsResource, LPSTR lpsParams);
	void SplitResource(const LPSTR lpsResource, LPSTR lpsPath, LPSTR lpsFilename, LPSTR lpsExtension);
    void ProcessRequest();
    void SendFile(const LPSTR lpsFilename);
    void Report(const LPSTR lpsCode, const LPSTR lpsStr);
private:
	BOOL bStop;
	LPSTR lpsBuffer;
	LONG nBufferSize;
	//    unsigned __int64 nTotalRead;
	unsigned long long nTotalRead;
	//	unsigned __int64 nFileSize;
	unsigned long long nFileSize;
    HANDLE hFile;
};
typedef CHttpClient* LPCHttpClient;

class CHttpClientThread : public CServerClientThread {
public:
	CHttpClientThread() {};
	CHttpClientThread(LPCServerClientSocket Socket);
	virtual void Execute();
};
typedef CHttpClientThread* LPCHttpClientThread;

class CHttpDaemon : public CServerSocket {
public:
    CHttpDaemon();
    virtual ~CHttpDaemon();
	HTTPdState GetState() const;
	virtual BOOL Start();
	virtual BOOL Stop();
	virtual LPCServerClientSocket OnGetSocket(LPCServerSocket lpServerSocket);
	virtual LPCServerClientThread OnGetThread(LPCServerClientSocket Socket);
	virtual void OnAccept(const LPCServerClientThread lpThread);
private:
	HTTPdState State;
};
typedef CHttpDaemon* LPCHttpDaemon;

class CHttpDaemonThread : public CThread {
public:
	CHttpDaemonThread() {};
	virtual void Execute();
private:
	CHttpDaemon *Daemon;
};
typedef CHttpDaemonThread* LPCHttpDaemonThread;

#endif /* __HTTPD_H */
