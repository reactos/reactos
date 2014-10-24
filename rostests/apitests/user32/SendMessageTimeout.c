/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SendMessageTimeout
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winuser.h>

static
void
TestSendMessageTimeout(HWND hWnd, UINT Msg)
{
    LRESULT ret;
    DWORD_PTR result;

    ret = SendMessageTimeoutW(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, NULL);
    ok(ret == 0, "ret = %Id\n", ret);

    result = 0x55555555;
    ret = SendMessageTimeoutW(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, &result);
    ok(ret == 0, "ret = %Id\n", ret);
    ok(result == 0, "result = %Iu\n", result);

    ret = SendMessageTimeoutA(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, NULL);
    ok(ret == 0, "ret = %Id\n", ret);

    result = 0x55555555;
    ret = SendMessageTimeoutA(hWnd, Msg, 0, 0, SMTO_NORMAL, 0, &result);
    ok(ret == 0, "ret = %Id\n", ret);
    ok(result == 0, "result = %Iu\n", result);
}

START_TEST(SendMessageTimeout)
{
    TestSendMessageTimeout(NULL, WM_USER);
    TestSendMessageTimeout(NULL, WM_PAINT);
    TestSendMessageTimeout(NULL, WM_GETICON);
}
