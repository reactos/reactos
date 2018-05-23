/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dialogs.h
 * PURPOSE:     Window procedures of the dialog windows plus launching functions
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class CMirrorRotateDialog : public CDialogImpl<CMirrorRotateDialog>
{
public:
    enum { IDD = IDD_MIRRORROTATE };

    BEGIN_MSG_MAP(CMirrorRotateDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDD_MIRRORROTATERB3, OnRadioButton3)
        COMMAND_ID_HANDLER(IDD_MIRRORROTATERB1, OnRadioButton12)
        COMMAND_ID_HANDLER(IDD_MIRRORROTATERB2, OnRadioButton12)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButton3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButton12(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

class CAttributesDialog : public CDialogImpl<CAttributesDialog>
{
public:
    enum { IDD = IDD_ATTRIBUTES };

    BEGIN_MSG_MAP(CAttributesDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESSTANDARD, OnDefault)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESRB1, OnRadioButton1)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESRB2, OnRadioButton2)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESRB3, OnRadioButton3)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESEDIT1, OnEdit1)
        COMMAND_ID_HANDLER(IDD_ATTRIBUTESEDIT2, OnEdit2)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButton1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButton2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRadioButton3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnEdit1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnEdit2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
    int newWidth;
    int newHeight;
};

class CStretchSkewDialog : public CDialogImpl<CStretchSkewDialog>
{
public:
    enum { IDD = IDD_STRETCHSKEW };

    BEGIN_MSG_MAP(CStretchSkewDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
    POINT percentage;
    POINT angle;
};
