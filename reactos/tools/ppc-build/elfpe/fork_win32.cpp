#include <windows.h>
#include "fork_execvp.h"

class Win32ProcessHolder : public ProcessHolder {
public:
    Win32ProcessHolder( const std::string &args )
	: ErrorRead(INVALID_HANDLE_VALUE), 
	  ProcessHandle(INVALID_HANDLE_VALUE),
	  StreamEnded(false) {
	HANDLE ErrorWrite = NULL, ErrorReadTemp = NULL;
	PROCESS_INFORMATION pi = { };
	SECURITY_ATTRIBUTES sa = { };
	STARTUPINFO si = { };
	
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	if(!CreatePipe(&ErrorReadTemp, &ErrorWrite, &sa, 0)) return;
	
	if(!DuplicateHandle(GetCurrentProcess(), ErrorReadTemp,
			    GetCurrentProcess(), &ErrorRead,
			    0, FALSE,
			    DUPLICATE_SAME_ACCESS)) return;

	CloseHandle(ErrorReadTemp);
	
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = ErrorWrite;
	
	if(!CreateProcess(NULL, (char *)args.c_str(), NULL, NULL, TRUE,
			  0, NULL, NULL, &si, &pi)) return;

	ProcessHandle = pi.hProcess;

	CloseHandle(ErrorWrite);
    }

    ~Win32ProcessHolder() {
	CloseHandle( ErrorRead );
	CloseHandle( ProcessHandle );
    }

    std::string ReadStdError() {
	char Buf[1024];
	DWORD ReadBytes;

	if( StreamEnded ) return "";

	StreamEnded = 
	    !ReadFile(ErrorRead, Buf, sizeof(Buf), &ReadBytes, NULL) ||
	    !ReadBytes;

	if( !StreamEnded ) return std::string(Buf, ReadBytes);
	else return "";
    }

    bool EndOfStream() const {
	return StreamEnded;
    }

    bool ProcessStarted() const {
	return ProcessHandle != INVALID_HANDLE_VALUE;
    }

    int GetStatus() const { 
	DWORD Status = 1;
	if( ProcessHandle != INVALID_HANDLE_VALUE ) {
	    WaitForSingleObject( ProcessHandle, INFINITE );
	    GetExitCodeProcess( ProcessHandle, &Status );
	}
	return Status;
    }

private:
    bool StreamEnded;
    HANDLE ErrorRead, ProcessHandle;
};

std::string quote_escape( const std::string &_str ) {
    std::string str = _str;
    size_t q;

    q = str.find('\"');
    while( q != std::string::npos ) {
	str.replace(q, 1, "\\\"");
	q = str.find('\"');
    }

    return std::string("\"") + str + "\"";
}

ProcessHolder *fork_execvp( const std::vector<std::string> &args ) {
    std::string argstring;
    ProcessHolder *holder;

    for( std::vector<std::string>::const_iterator i = args.begin();
	 i != args.end();
	 i++ ) {
	if( i != args.begin() ) 
	    argstring += " ";
	argstring += quote_escape(*i);
    }
    
    return new Win32ProcessHolder( argstring );
}
