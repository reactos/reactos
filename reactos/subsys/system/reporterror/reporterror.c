/*
 * ReactOS Error Reporting Assistant
 * Copyright (C) 2004-2005 Casper S. Hornstrup <chorns@users.sourceforge.net>
 * Copyright (C) 2003-2004 Peter Willis <psyphreak@phreaker.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Error Reporting Assistant
 * FILE:        subsys/system/reporterror/reporterror.c
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "reporterror.h"

static LPSTR ErrorReportingServerName = "errors.reactos.com";
static HINSTANCE hAppInstance;
static HANDLE hSubmissionThread = NULL;
static HWND hSubmissionNotifyWnd = NULL;
static LONG AbortSubmission = 0;
static LPERROR_REPORT ErrorReport = NULL;

#define WM_ABORT_SUBMISSION (WM_USER + 2)
#define WM_CONTACTING_SERVER (WM_USER + 3)
#define WM_SUBMISSION_COMPLETE (WM_USER + 4)

#define MAX_REQUEST_BUFFER_SIZE 20480

LPSTR
UrlEncode(LPSTR in, LPSTR out)
{
  CHAR buffer[4];
  UCHAR iu;
  INT i;

  for (i = 0; i < strlen(in); i++)
  {
    iu = (UCHAR)in[i];
    memset(buffer, '\0', sizeof(buffer));
    if ((iu < 33 || iu > 126))
      sprintf(buffer, "%%%02x", iu);
    else
      sprintf(buffer, "%c", iu);
    strcat(out, buffer);
  }
  return out;
}

LPERROR_REPORT
FillErrorReport(HWND hwndDlg)
{
  INT size;
  LPERROR_REPORT errorReport = malloc(sizeof(ERROR_REPORT));

  size = 300;
  errorReport->YourEmail = malloc(size);
  GetDlgItemTextA(hwndDlg,
    IDE_SUBMIT_REPORT_YOUR_EMAIL,
    errorReport->YourEmail,
    size);

  size = 10240;
  errorReport->ProblemDescription = malloc(size);
  GetDlgItemTextA(hwndDlg,
    IDE_SUBMIT_REPORT_PROBLEM_DESCRIPTION,
    errorReport->ProblemDescription,
    size);

  return errorReport;
}

ReleaseErrorReport(LPERROR_REPORT errorReport)
{
  if (errorReport->YourEmail)
    free(errorReport->YourEmail);
  if (errorReport->ProblemDescription)
    free(errorReport->ProblemDescription);
  free(errorReport);
}

BOOL
ProcessMessage(VOID)
{
  MSG msg;
  if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return TRUE;
  }
  return FALSE;
}

VOID
ProcessMessages(VOID)
{
  while (ProcessMessage());
}

VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DWORD Flags, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT | Flags;
  psp->hInstance = hAppInstance;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}

INT_PTR CALLBACK
PageFirstPageProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch (pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
          break;
        }
        case PSN_WIZNEXT:
        {
          SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SUBMIT_REPORT);
          return TRUE;
        }
      }
      break;
    }
  }
  return FALSE;
}

INT_PTR CALLBACK
PageSubmitReportProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch (pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
          break;
        }
        case PSN_WIZNEXT:
        {
          ErrorReport = FillErrorReport(hwndDlg);
          SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SUBMITTING_REPORT);
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

VOID
TerminateSubmission(BOOL Wait)
{
  if (hSubmissionThread != NULL)
  {
    if (Wait)
    {
      InterlockedExchange((LONG*)&AbortSubmission, 2);
      WaitForSingleObject(hSubmissionThread, INFINITE);
    }
    else
      InterlockedExchange((LONG*)&AbortSubmission, 1);
  }
}

INT
ConnectToServer(LPSTR host,
  SOCKET *clientSocket,
  LPWSTR errorMessage)
{
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;
	INT error;
  SOCKET s;

  *clientSocket = 0;

	hp = gethostbyname(host);
	if (hp == NULL)
  {
    error = WSAGetLastError();
    wsprintf(errorMessage, L"Could not resolve DNS for %S (windows error code %d)", host, error);
    return error;
	}

	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0)
  {
    error = WSAGetLastError();
    wsprintf(errorMessage, L"Could not create socket (windows error code %d)", error);
    return error;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = hp->h_addrtype;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    error = WSAGetLastError();
    wsprintf(errorMessage, L"Could not resolve DNS for %S (windows error code %d)", host, error);
    closesocket(s);
    return error;
	}

	memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	sp = getservbyname("www", "tcp");
	if (sp == NULL)
  {
    error = WSAGetLastError();
    wsprintf(errorMessage, L"Could not get service (windows error code %d)", error);
    closesocket(s);
    return error;
	}

	sin.sin_port = sp->s_port;

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    error = WSAGetLastError();
    wsprintf(errorMessage, L"Could not connect to server (windows error code %d)", error);
    closesocket(s);
    return error;
	}

  *clientSocket = s;
  return NO_ERROR;
}

VOID
CreateHTTPPostRequest(LPSTR requestBuffer,
  LPSTR hostname,
  LPERROR_REPORT errorReport)
{
  LPSTR parameterBuffer;
  LPSTR urlencodeBuffer;

  parameterBuffer = malloc(MAX_REQUEST_BUFFER_SIZE);
  memset(parameterBuffer, '\0', MAX_REQUEST_BUFFER_SIZE);

  strcat(parameterBuffer, "errorReport=");
  strcat(parameterBuffer, "<ErrorReportRequest>");
  strcat(parameterBuffer, "<YourEmail>");
  strcat(parameterBuffer, errorReport->YourEmail);
  strcat(parameterBuffer, "</YourEmail>");
  strcat(parameterBuffer, "<ProblemDescription>");
  strcat(parameterBuffer, errorReport->ProblemDescription);
  strcat(parameterBuffer, "</ProblemDescription>");
  strcat(parameterBuffer, "</ErrorReportRequest>");
  strcat(parameterBuffer, "\r\n");

  urlencodeBuffer = malloc(MAX_REQUEST_BUFFER_SIZE);
  memset(urlencodeBuffer, '\0', MAX_REQUEST_BUFFER_SIZE);

  UrlEncode(parameterBuffer, urlencodeBuffer);
  sprintf(requestBuffer, "POST /Report.asmx/SubmitErrorReport HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Content-Type: application/x-www-form-urlencoded\r\n"
                         "Content-Length: %d\r\n"
                         "\r\n"
                         "%s",
    hostname,
    strlen(urlencodeBuffer),
    urlencodeBuffer);

  free(urlencodeBuffer);
  free(parameterBuffer);
}

#define CONTENT_LENGTH "Content-Length:"

BOOL
WasErrorReportDelivered(LPSTR httpResponse)
{
  return strstr(httpResponse, "&lt;/ErrorReportResponse&gt;") != NULL;
}

BOOL
ReceiveResponse(SOCKET socket, LPSTR responseBuffer, PULONG responseBufferSize)
{
  PCHAR pch = (PCHAR)responseBuffer;
  ULONG length = 0;
  INT contentLength = 0;
  fd_set fdset[1];
  struct timeval timeout;
  CHAR buf[4000];

  FD_ZERO(&fdset);
  FD_SET(socket, &fdset);
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
	while (select(1, fdset, NULL, NULL, &timeout) > 0)
  {
    if (recv(socket, pch, 1, 0) == 1)
    {
      pch++;
      length++;
      if (WasErrorReportDelivered(responseBuffer))
      {
        *responseBufferSize = length;
        return TRUE;
      }
    }
    else
      break;
  }
  *responseBufferSize = 0;
  return FALSE;
}

BOOL
SubmitErrorReport(SOCKET socket, LPERROR_REPORT errorReport)
{
  BOOL wasErrorReportDelivered;
  LPSTR requestBuffer;
  LPSTR responseBuffer;
  ULONG requestBufferSize = MAX_REQUEST_BUFFER_SIZE;

  requestBuffer = malloc(requestBufferSize);
  memset(requestBuffer, '\0', requestBufferSize);
  CreateHTTPPostRequest(requestBuffer, ErrorReportingServerName, errorReport);
	send(socket, requestBuffer, strlen(requestBuffer), 0);
  responseBuffer = malloc(requestBufferSize);
  wasErrorReportDelivered = ReceiveResponse(socket, responseBuffer, IN OUT &requestBufferSize);
  free(responseBuffer);
  free(requestBuffer);
  return wasErrorReportDelivered;
}

VOID
DisconnectFromServer(SOCKET socket)
{
  closesocket(socket);
}

DWORD STDCALL
SubmissionThread(LPVOID lpParameter)
{
  WCHAR errorMessage[1024];
  SOCKET socket;
  HANDLE hThread;
  INT error;
  INT i;

  if (AbortSubmission != 0)
    goto done;

  PostMessage(hSubmissionNotifyWnd, WM_CONTACTING_SERVER, IDS_CONTACTING_SERVER, 0);

  error = ConnectToServer(ErrorReportingServerName, &socket, errorMessage);
  if (error != NO_ERROR)
  {
    MessageBox(NULL, errorMessage, NULL, MB_ICONWARNING);

    PostMessage(hSubmissionNotifyWnd, WM_ABORT_SUBMISSION, IDS_FAILED_TO_LOCATE_SERVER, 0);
    goto cleanup;
  }

  if (!SubmitErrorReport(socket, ErrorReport))
  {
    PostMessage(hSubmissionNotifyWnd, WM_ABORT_SUBMISSION, IDS_FAILED_TO_DELIVER_ERROR_REPORT, 0);
    goto cleanup;
  }

  DisconnectFromServer(socket);
done:
  switch (AbortSubmission)
  {
    case 0:
      SendMessage(hSubmissionNotifyWnd, WM_SUBMISSION_COMPLETE, 0, 0);
      break;
    case 1:
      SendMessage(hSubmissionNotifyWnd, WM_ABORT_SUBMISSION, 0, 0);
      break;
  }

cleanup:
  hThread = (HANDLE)InterlockedExchange((LONG*)&hSubmissionThread, 0);
  if (hThread != NULL)
    CloseHandle(hThread);
  ReleaseErrorReport(ErrorReport);
  ErrorReport = NULL;
  return 0;
}

BOOL
StartSubmissionThread(HWND hWndNotify)
{
  if (hSubmissionThread == NULL)
  {
    DWORD ThreadId;
    hSubmissionNotifyWnd = hWndNotify;
    AbortSubmission = 0;
    hSubmissionThread = CreateThread(NULL,
                                     0,
                                     SubmissionThread,
                                     NULL,
                                     CREATE_SUSPENDED,
                                     &ThreadId);
    if (hSubmissionThread == NULL)
      return FALSE;

    ResumeThread(hSubmissionThread);
    return TRUE;
  }

  return FALSE;
}

INT_PTR CALLBACK
PageSubmittingReportProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch (pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          SetDlgItemText(hwndDlg, IDC_SUBMISSION_STATUS, NULL);
          SendDlgItemMessage(hwndDlg, IDC_SUBMITTING_IN_PROGRESS, PBM_SETMARQUEE, TRUE, 50);
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
          StartSubmissionThread(hwndDlg);
          break;
        }
        case PSN_RESET:
        {
          TerminateSubmission(TRUE);
          break;
        }
        case PSN_WIZBACK:
          if (hSubmissionThread != NULL)
          {
            PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
            TerminateSubmission(FALSE);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
            return -1;
          }
          else
          {
            SendDlgItemMessage(hwndDlg, IDC_SUBMITTING_IN_PROGRESS, PBM_SETMARQUEE, FALSE, 0);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SUBMIT_REPORT);
          }
          break;
      }
      break;
    }
    case WM_CONTACTING_SERVER:
    {
      WCHAR Msg[1024];
      LoadString(hAppInstance, wParam, Msg, sizeof(Msg) / sizeof(WCHAR));
      SetDlgItemText(hwndDlg, IDC_SUBMISSION_STATUS, Msg);
      break;
    }
    case WM_SUBMISSION_COMPLETE:
    {
      SendDlgItemMessage(hwndDlg, IDC_SUBMITTING_IN_PROGRESS, PBM_SETMARQUEE, FALSE, 0);
      PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
      PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_SUBMITTED_REPORT);
      break;
    }
    case WM_ABORT_SUBMISSION:
    {
      /* Go back in case we aborted the submission thread */
      SendDlgItemMessage(hwndDlg, IDC_SUBMITTING_IN_PROGRESS, PBM_SETMARQUEE, FALSE, 0);
        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_SUBMIT_REPORT);
      if (wParam != 0)
      {
        WCHAR Msg[1024];
        LoadString(hAppInstance, wParam, Msg, sizeof(Msg) / sizeof(WCHAR));
        MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
      }
      break;
    }
  }
  return FALSE;
}

INT_PTR CALLBACK
PageSubmittedReportProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch (pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          EnableWindow(GetDlgItem(GetParent(hwndDlg), IDCANCEL), FALSE);
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

static LONG
CreateWizard(VOID)
{
  PROPSHEETPAGE psp[8];
  PROPSHEETHEADER psh;
  WCHAR Caption[1024];

  LoadString(hAppInstance, IDS_WIZARD_NAME, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
  psh.hwndParent = NULL;
  psh.hInstance = hAppInstance;
  psh.hIcon = 0;
  psh.pszCaption = Caption;
  psh.nPages = 4;
  psh.nStartPage = 0;
  psh.ppsp = psp;
  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  InitPropSheetPage(&psp[0], IDD_FIRSTPAGE, PSP_HIDEHEADER, PageFirstPageProc);
  InitPropSheetPage(&psp[1], IDD_SUBMIT_REPORT, PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, PageSubmitReportProc);
  InitPropSheetPage(&psp[2], IDD_SUBMITTING_REPORT, PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, PageSubmittingReportProc);
  InitPropSheetPage(&psp[3], IDD_SUBMITTED_REPORT, PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE, PageSubmittedReportProc);

  return (LONG)(PropertySheet(&psh) != -1);
}

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	INT error;
  INT version;
  WCHAR *lc;

  hAppInstance = hInstance;

  wVersionRequested = MAKEWORD(1, 1);
  error = WSAStartup(wVersionRequested, &wsaData);
  if (error != NO_ERROR)
  {
    WCHAR format[1024];
    WCHAR message[1024];
    LoadString(hAppInstance, IDS_FAILED_TO_INITIALIZE_WINSOCK, format, sizeof(format) / sizeof(WCHAR));
    wsprintf(message, format, error);
    MessageBox(NULL, message, NULL, MB_ICONWARNING);
    return;
  }
  
  CreateWizard();

 	WSACleanup();
  
  return 0;
}
