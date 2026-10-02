/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Displaying Cicero caret
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class CicCaret
{
protected:
    HWND m_hWnd = NULL;
    UINT m_uCaretBlinkTimerID = 0;
    POINT m_pt = { 0 };
    SIZE m_size = { 0 };
    BOOL m_bCaretBlinking = FALSE;
    BOOL m_bCaretVisible = FALSE;

public:
    enum { TIMER_ID = 0x4F83AF91 };
    CicCaret();
    virtual ~CicCaret();
    operator HWND() const { return m_hWnd; }

    void CreateCaret(HWND hWnd, SIZE size);
    void DestroyCaret();
    void HideCaret();
    void InvertCaret();
    void OnTimer();
    void SetCaretPos(POINT pt);
    void SetBlinking(BOOL bBlinking);
};
