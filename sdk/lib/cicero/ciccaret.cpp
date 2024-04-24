/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Displaying Cicero caret
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "ciccaret.h"

/**
 * @implemented
 */
CicCaret::CicCaret()
{
    m_bCaretBlinking = FALSE;
    m_bCaretVisible = FALSE;
    m_uCaretBlinkTimerID = 0;
    m_pt.x = m_pt.y = 0;
    m_size.cx = m_size.cy = 0;
}

/**
 * @implemented
 */
CicCaret::~CicCaret()
{
    HideCaret();
    ::KillTimer(m_hWnd, m_uCaretBlinkTimerID);
    m_uCaretBlinkTimerID = 0;
}

/**
 * @implemented
 */
void CicCaret::CreateCaret(HWND hWnd, SIZE size)
{
    m_hWnd = hWnd;
    m_size = size;
    if (::IsWindow(hWnd))
        m_uCaretBlinkTimerID = ::SetTimer(m_hWnd, TIMER_ID, ::GetCaretBlinkTime(), NULL);
}

/**
 * @implemented
 */
void CicCaret::DestroyCaret()
{
    HideCaret();
    ::KillTimer(m_hWnd, m_uCaretBlinkTimerID);
    m_uCaretBlinkTimerID = 0;
}

/**
 * @implemented
 */
void CicCaret::HideCaret()
{
    if (m_bCaretVisible)
    {
        m_bCaretVisible = FALSE;
        InvertCaret();
    }
    m_bCaretBlinking = FALSE;
}

/**
 * @implemented
 */
void CicCaret::InvertCaret()
{
    HDC hDC = ::GetDC(m_hWnd);
    ::PatBlt(hDC, m_pt.x, m_pt.y, m_size.cx, m_size.cy, DSTINVERT);
    ::ReleaseDC(m_hWnd, hDC);
}

/**
 * @implemented
 */
void CicCaret::OnTimer()
{
    if (m_bCaretBlinking)
    {
        m_bCaretVisible = !m_bCaretVisible;
        InvertCaret();
    }
}

/**
 * @implemented
 */
void CicCaret::SetCaretPos(POINT pt)
{
    BOOL bCaretVisible = m_bCaretVisible;
    if (bCaretVisible)
        InvertCaret();

    m_pt = pt;

    if (bCaretVisible)
        InvertCaret();
}

/**
 * @implemented
 */
void CicCaret::SetBlinking(BOOL bBlinking)
{
    m_bCaretBlinking = bBlinking;
}
