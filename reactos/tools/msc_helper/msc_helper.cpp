/*
	Copyright (c) 2009 KJK::Hyperion

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#include <functional>
#include <iterator>

#include <tchar.h>
#include <limits.h>

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#undef RtlMoveMemory
extern "C" DECLSPEC_IMPORT void NTAPI RtlMoveMemory(void UNALIGNED *, const void UNALIGNED *, SIZE_T);

#ifndef ARRAYSIZE
#define ARRAYSIZE(X_) (sizeof(X_) / sizeof((X_)[0]))
#endif

#include <kjk/argv_parser.h>
#include <kjk/stringz_iterator.h>

using namespace kjk;

namespace
{
	bool WriteAll(HANDLE hFile, const void * p, size_t cb)
	{
		const char * pb = static_cast<const char *>(p);
		bool ret = cb == 0;

		while(cb)
		{
			DWORD cbToWrite;

			if(cb > MAXLONG)
				cbToWrite = MAXLONG;
			else
				cbToWrite = static_cast<DWORD>(cb);

			DWORD cbWritten;
			ret = !!WriteFile(hFile, pb, cbToWrite, &cbWritten, NULL);

			if(!ret)
				break;

			cb -= cbWritten;
			pb += cbWritten;
		}

		return ret;
	}

	DECLSPEC_NORETURN
	void Exit(DWORD dwExitCode)
	{
		for(;;) TerminateProcess(GetCurrentProcess(), dwExitCode);
	}

	DECLSPEC_NORETURN
	void Die(DWORD dwExitCode, const char * pszFunction, DWORD dwErrCode)
	{
		DWORD_PTR args[] = { (DWORD_PTR)pszFunction, dwErrCode };

		char * pszMessage;
		DWORD cchMessage = FormatMessageA
		(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
			"msc_helper: %1!s!() failed: error %2!lu!\n",
			0,
			0,
			(LPSTR)&pszMessage,
			0,
			(va_list *)args
		);

		if(cchMessage)
		{
			(void)WriteAll(GetStdHandle(STD_ERROR_HANDLE), pszMessage, cchMessage);
			LocalFree((HLOCAL)pszMessage);
		}

		Exit(dwExitCode);
	}

	DECLSPEC_NORETURN
	void Die(const char * pszFunction)
	{
		Die(1, pszFunction, GetLastError());
	}

	DECLSPEC_NORETURN
	void Die(const char * pszFunction, DWORD dwExitCode)
	{
		Die(1, pszFunction, dwExitCode);
	}

	template<size_t N>
	DECLSPEC_NORETURN
	void DieMessage(const char (& msg)[N])
	{
		(void)WriteAll(GetStdHandle(STD_ERROR_HANDLE), msg, (N) - 1);
		Exit(1);
	}

	DECLSPEC_NORETURN
	void OutOfMemory()
	{
		DieMessage("msc_helper: out of memory\n");
	}

	class store_argument
	{
	private:
		char * m_arg;
		size_t m_argLen;

	public:
		// Pretend we are an STL container
		typedef TCHAR value_type;
		typedef TCHAR * pointer;
		typedef TCHAR& reference;
		typedef const TCHAR * const_pointer;
		typedef const TCHAR& const_reference;

		void push_back(TCHAR x)
		{
#ifdef UNICODE
			if(x > CHAR_MAX)
				DieMessage("msc_helper: invalid character in command line\n");
#endif
			m_arg = static_cast<char *>(HeapReAlloc(GetProcessHeap(), 0, m_arg, m_argLen + 1));

			if(m_arg == NULL)
				OutOfMemory();

			m_arg[m_argLen] = static_cast<char>(x);
			++ m_argLen;
		}

	public:
		store_argument(): m_arg(static_cast<char *>(HeapAlloc(GetProcessHeap(), 0, 1))), m_argLen(0)
		{
			if(m_arg == NULL)
				OutOfMemory();
		}

		const char * get_arg() const { return m_arg; }
		size_t get_arg_len() const { return m_argLen; }
	};

	class filter_output
	{
	private:
		const char * m_pFilterLine;
		size_t m_cbFilterLine;
		HANDLE m_hOutput;
		bool& m_fDone;

		static DWORD s_cbDummy;
		static DWORD s_pipeId;

		char m_buffer[1024];
		LPTSTR m_pszPipeName;
		HANDLE m_hReadFrom;
		OVERLAPPED m_asyncRead;
		char * m_pLineBuffer;
		size_t m_cbLineBuffer;

		bool begin_read()
		{
			bool bRet = !!ReadFile(m_hReadFrom, m_buffer, sizeof(m_buffer), &s_cbDummy, &m_asyncRead);

			if(!bRet)
			{
				bRet = GetLastError() != ERROR_BROKEN_PIPE;

				if(bRet)
				{
					bRet = GetLastError() == ERROR_IO_PENDING;

					if(!bRet)
						Die("ReadFile");
				}
			}

			return bRet;
		}

		void push_input(size_t cbBufferValid)
		{
			if(m_fDone)
			{
				if(!WriteAll(m_hOutput, m_buffer, cbBufferValid))
					Die("WriteFile");

				return;
			}

			// Add the data read from output to the line buffer
			size_t cbLineBufferNew = m_cbLineBuffer + cbBufferValid;

			if(m_pLineBuffer)
				m_pLineBuffer = static_cast<char *>(HeapReAlloc(GetProcessHeap(), 0, m_pLineBuffer, cbLineBufferNew));
			else
				m_pLineBuffer = static_cast<char *>(HeapAlloc(GetProcessHeap(), 0, cbLineBufferNew));

			if(m_pLineBuffer == NULL)
				OutOfMemory();

			RtlMoveMemory(m_pLineBuffer + m_cbLineBuffer, m_buffer, cbBufferValid);

			// Parse all complete lines in the line buffer
			size_t cbLineStart = 0;

			for(size_t i = m_cbLineBuffer; i < cbLineBufferNew; ++ i)
			{
				// We have a complete line
				if(m_pLineBuffer[i] == '\n' || m_pLineBuffer[i] == '\r')
				{
					size_t cbLine = i - cbLineStart;

					++ i;

					if(i < cbLineBufferNew && m_pLineBuffer[i - 1] == '\r' && m_pLineBuffer[i] == '\n')
						++ i;

					size_t cbLineFull = i - cbLineStart;

					bool fMatched = cbLine == m_cbFilterLine && memcmp(m_pLineBuffer + cbLineStart, m_pFilterLine, m_cbFilterLine) == 0;

					// The line doesn't match: dump it
					if(!fMatched)
					{
						if(!WriteAll(m_hOutput, m_pLineBuffer + cbLineStart, cbLineFull))
							Die("WriteFile");
					}
					// The line matches: we are done
					else
						m_fDone = fMatched;

					cbLineStart = i;
				}

				// Filtering is complete: from now on, just dump everything
				if(m_fDone)
				{
					if(!WriteAll(m_hOutput, m_buffer + i, cbLineBufferNew - i))
						Die("WriteFile");

					HeapFree(GetProcessHeap(), 0, m_pLineBuffer);
					return;
				}
			}

			// Re-buffer what's left
			m_cbLineBuffer = cbLineBufferNew - cbLineStart;
			RtlMoveMemory(m_pLineBuffer, m_pLineBuffer + cbLineStart, m_cbLineBuffer);
		}

	public:
		filter_output(const char * pFilterLine, size_t cbFilterLine, HANDLE hOutput, bool * pfDone):
			m_pFilterLine(pFilterLine),
			m_cbFilterLine(cbFilterLine),
			m_hOutput(hOutput),
			m_fDone(*pfDone),
			m_asyncRead(),
			m_pLineBuffer(0),
			m_cbLineBuffer(0)
		{
			for(;;)
			{
				DWORD_PTR args[] = { GetCurrentProcessId(), s_pipeId };

				DWORD cchPipeName = FormatMessage
				(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
					TEXT("\\\\.\\pipe\\msc_helper-%1!08X!-%2!08X!"),
					0,
					0,
					(LPTSTR)&m_pszPipeName,
					0,
					(va_list *)args
				);

				if(cchPipeName == 0)
					Die("FormatMessage");

				m_hReadFrom = CreateNamedPipe(m_pszPipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 0, 0, 0, NULL);

				++ s_pipeId;

				if(m_hReadFrom != INVALID_HANDLE_VALUE)
					break;

				if(GetLastError() != ERROR_PIPE_BUSY)
					Die("CreateNamedPipe");
			}

			// Just in case
			bool fEmptyFilter = m_cbFilterLine == 0;

			if(fEmptyFilter)
				m_fDone = fEmptyFilter;
		}

		HANDLE open_write_end(LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
		{
			HANDLE hWritePipe = CreateFile(m_pszPipeName, GENERIC_WRITE, FILE_SHARE_READ, lpSecurityAttributes, OPEN_EXISTING, dwFlagsAndAttributes, hTemplateFile);

			if(hWritePipe == INVALID_HANDLE_VALUE)
				Die("CreateFile");

			(void)begin_read();
			return hWritePipe;
		}

		HANDLE get_read_completion() const
		{
			return m_hReadFrom;
		}

		bool operator()()
		{
			DWORD cbBufferValid;

			if(!GetOverlappedResult(m_hReadFrom, &m_asyncRead, &cbBufferValid, TRUE))
			{
				bool b = GetLastError() != ERROR_BROKEN_PIPE;

				if(!b)
					return b;

				Die("ReadFile");
			}

			push_input(cbBufferValid);
			return begin_read();
		}
	};

	DWORD filter_output::s_cbDummy = 0;
	DWORD filter_output::s_pipeId = 0;
}

int main()
{
	DWORD dwExitCode = 1;

	// Parse the command line
	LPCTSTR pszCommandLine = GetCommandLine();

	// Skip argv[0]
	pszCommandLine = skip_argument(stringz_begin(pszCommandLine), stringz_end(pszCommandLine)).base();

	// Get argv[1]: the line that should be filtered out of the output
	// argv[2..N] will become argv[0..N-2] of the compiler
	store_argument filterLine;
	pszCommandLine = copy_argument(stringz_begin(pszCommandLine), stringz_end(pszCommandLine), std::back_inserter(filterLine)).base();

	// Initialize the output filters
	bool fDone = false;
	filter_output filterStdOutput(filterLine.get_arg(), filterLine.get_arg_len(), GetStdHandle(STD_OUTPUT_HANDLE), &fDone);
	filter_output filterStdError(filterLine.get_arg(), filterLine.get_arg_len(), GetStdHandle(STD_ERROR_HANDLE), &fDone);

	// A handle that will never be signaled
	HANDLE hNever;

	if(!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &hNever, SYNCHRONIZE, FALSE, 0))
		Die("DuplicateHandle");

	// Fix the environment
	static TCHAR szPath[32768];
	DWORD cchPath = GetEnvironmentVariable(TEXT("MSC_HELPER_PATH"), szPath, ARRAYSIZE(szPath));

	if(cchPath > ARRAYSIZE(szPath))
		DieMessage("msc_helper: %MSC_HELPER_PATH% variable too big\n");

	if(cchPath > 0 && !SetEnvironmentVariable(TEXT("PATH"), szPath))
		Die("SetEnvironmentVariable");

	// Run the sub-process
	STARTUPINFO si;
	GetStartupInfo(&si);

	si.dwFlags |= STARTF_USESTDHANDLES;

	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	SECURITY_ATTRIBUTES pipeAttributes = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	si.hStdOutput = filterStdOutput.open_write_end(&pipeAttributes, 0, NULL);
	si.hStdError = filterStdError.open_write_end(&pipeAttributes, 0, NULL);

	PROCESS_INFORMATION pi = {};

	if(!CreateProcess(NULL, const_cast<LPTSTR>(pszCommandLine), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		Die("CreateProcess");

	CloseHandle(pi.hThread);
	CloseHandle(si.hStdOutput);
	CloseHandle(si.hStdError);

	(void)SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

	HANDLE waitHandles[3] =
	{
		pi.hProcess,
		filterStdOutput.get_read_completion(),
		filterStdError.get_read_completion()
	};

	unsigned done = 0;

	char * pLineBuffer = NULL;
	size_t cbLineBuffer = 0;

	while(done < ARRAYSIZE(waitHandles))
	{
		switch(WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0:
			GetExitCodeProcess(pi.hProcess, &dwExitCode);
			waitHandles[WAIT_OBJECT_0] = hNever;
			++ done;
			break;

		case WAIT_OBJECT_0 + 1:
			if(!filterStdOutput())
			{
				waitHandles[WAIT_OBJECT_0 + 1] = hNever;
				++ done;
			}

			break;

		case WAIT_OBJECT_0 + 2:
			if(!filterStdError())
			{
				waitHandles[WAIT_OBJECT_0 + 2] = hNever;
				++ done;
			}

			break;

		default:
			Die("WaitForMultipleObjects");
		}
	}

	Exit(dwExitCode);
}

// EOF
