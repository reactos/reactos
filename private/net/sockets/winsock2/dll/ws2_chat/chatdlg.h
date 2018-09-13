/*++

Copyright (c) 1995 Intel Corp

Module Name:

    chatdlg.h

Abstract:

    Header file containing function protoypes of functions in dialog.c.

Author:

    Dan Chou & Michael Grafton

--*/

BOOL APIENTRY
InetConnDlgProc(
    IN HWND DialogHandle,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

BOOL APIENTRY
DefaultConnDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

BOOL APIENTRY
ChooseFamilyDlgProc(
    IN HWND DialogHandle,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

BOOL APIENTRY
NameAndSubjectDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

BOOL APIENTRY
AcceptConnectionDlgProc(
    IN HWND DialogHandle,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

BOOL APIENTRY
InetListenPortDlgProc(
    IN HWND DialogHandle,
    IN UINT Message,
    IN WPARAM WordParam,
    IN LPARAM LongParam);

