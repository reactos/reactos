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
    HWND m_hWnd;
    UINT m_uCaretBlinkTimerID;
    POINT m_pt;
    SIZE m_size;
    BOOL m_bCaretBlinking;
    BOOL m_bCaretVisible;

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
