/* Unit tests for the task dialog control.
 *
 * Copyright 2017 Fabian Maurer for the Wine project
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/heap.h"
#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

#ifdef __REACTOS__
#undef WM_KEYF1
#define WM_KEYF1 0x004d
#endif

#define WM_TD_CALLBACK (WM_APP) /* Custom dummy message to wrap callback notifications */

#define NUM_MSG_SEQUENCES     1
#define TASKDIALOG_SEQ_INDEX  0

#define TEST_NUM_BUTTONS 10 /* Number of custom buttons to test with */
#define TEST_NUM_RADIO_BUTTONS 3

#define ID_START 20 /* Lower IDs might be used by the system */
#define ID_START_BUTTON (ID_START + 0)
#define ID_START_RADIO_BUTTON (ID_START + 20)

static HRESULT (WINAPI *pTaskDialogIndirect)(const TASKDIALOGCONFIG *, int *, int *, BOOL *);
static HRESULT (WINAPI *pTaskDialog)(HWND, HINSTANCE, const WCHAR *, const WCHAR *, const WCHAR *,
        TASKDIALOG_COMMON_BUTTON_FLAGS, const WCHAR *, int *);

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

struct message_info
{
    UINT message;
    WPARAM wparam;
    LPARAM lparam;

    HRESULT callback_retval;
    const struct message_info *send;  /* Message to send to trigger the next callback message */
};

static const struct message_info *current_message_info;

/* Messages to send */
static const struct message_info msg_send_return[] =
{
    { WM_KEYDOWN, VK_RETURN, 0 },
    { 0 }
};

/* Messages to test against */
static const struct message_info msg_return_press_ok[] =
{
    { TDN_CREATED,        0,    0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_yes[] =
{
    { TDN_CREATED,        0,    0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDYES, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_no[] =
{
    { TDN_CREATED,        0,    0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDNO, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_cancel[] =
{
    { TDN_CREATED,        0,    0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDCANCEL, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_retry[] =
{
    { TDN_CREATED,        0,       0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, IDRETRY, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_custom1[] =
{
    { TDN_CREATED,        0,               0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, ID_START_BUTTON, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_custom4[] =
{
    { TDN_CREATED,        0,                   0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, ID_START_BUTTON + 3, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_press_custom10[] =
{
    { TDN_CREATED,        0,                   0, S_OK, msg_send_return },
    { TDN_BUTTON_CLICKED, -1, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_click_ok[] =
{
    { TDM_CLICK_BUTTON, IDOK, 0 },
    { 0 }
};

static const struct message_info msg_send_f1[] =
{
    { WM_KEYF1, 0, 0, 0},
    { 0 }
};

static const struct message_info msg_got_tdn_help[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_f1 },
    { TDN_HELP, 0, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

/* Three radio buttons */
static const struct message_info msg_return_default_radio_button_1[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_default_radio_button_2[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON + 1, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_default_radio_button_3[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, -2, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_select_first_radio_button[] =
{
    { TDM_CLICK_RADIO_BUTTON, ID_START_RADIO_BUTTON, 0 },
    { 0 }
};

static const struct message_info msg_return_first_radio_button[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON + 1, 0, S_OK, msg_select_first_radio_button },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_select_first_disabled_radio_button_and_press_ok[] =
{
    { TDM_ENABLE_RADIO_BUTTON, ID_START_RADIO_BUTTON, 0 },
    { TDM_CLICK_RADIO_BUTTON, ID_START_RADIO_BUTTON, 0 },
    { TDM_CLICK_BUTTON, IDOK, 0 },
    { 0 }
};

static const struct message_info msg_return_default_radio_button_clicking_disabled[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON + 1, 0, S_OK, msg_select_first_disabled_radio_button_and_press_ok },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, NULL },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_no_default_radio_button_flag[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_click_ok },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, NULL },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_no_default_radio_button_id_and_flag[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_select_negative_id_radio_button[] =
{
    { TDM_CLICK_RADIO_BUTTON, -2, 0 },
    { 0 }
};

static const struct message_info msg_return_press_negative_id_radio_button[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_select_negative_id_radio_button },
    { TDN_RADIO_BUTTON_CLICKED, -2, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_all_common_button_click[] =
{
    { TDM_CLICK_BUTTON, IDOK, 0 },
    { TDM_CLICK_BUTTON, IDYES, 0 },
    { TDM_CLICK_BUTTON, IDNO, 0 },
    { TDM_CLICK_BUTTON, IDCANCEL, 0 },
    { TDM_CLICK_BUTTON, IDRETRY, 0 },
    { TDM_CLICK_BUTTON, IDCLOSE, 0 },
    { TDM_CLICK_BUTTON, ID_START_BUTTON + 99, 0 },
    { 0 }
};

static const struct message_info msg_press_nonexistent_buttons[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_all_common_button_click },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, IDYES, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, IDNO, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, IDCANCEL, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, IDRETRY, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, IDCLOSE, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, ID_START_BUTTON + 99, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_all_common_button_click_with_command[] =
{
    { WM_COMMAND, MAKEWORD(IDOK, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDYES, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDNO, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDCANCEL, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDRETRY, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDCLOSE, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(ID_START_BUTTON + 99, BN_CLICKED), 0 },
    { WM_COMMAND, MAKEWORD(IDOK, BN_CLICKED), 0 },
    { 0 }
};

static const struct message_info msg_press_nonexistent_buttons_with_command[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_all_common_button_click_with_command },
    { TDN_BUTTON_CLICKED, ID_START_BUTTON, 0, S_FALSE, NULL },
    { TDN_BUTTON_CLICKED, ID_START_BUTTON, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_nonexistent_radio_button_click[] =
{
    { TDM_CLICK_RADIO_BUTTON, ID_START_RADIO_BUTTON + 99, 0 },
    { TDM_CLICK_BUTTON, IDOK, 0 },
    { 0 }
};

static const struct message_info msg_press_nonexistent_radio_button[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_nonexistent_radio_button_click },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_default_verification_unchecked[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_return_default_verification_checked[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_uncheck_verification[] =
{
    { TDM_CLICK_VERIFICATION, FALSE, 0 },
    { 0 }
};

static const struct message_info msg_return_verification_unchecked[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_uncheck_verification },
    { TDN_VERIFICATION_CLICKED, FALSE, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_check_verification[] =
{
    { TDM_CLICK_VERIFICATION, TRUE, 0 },
    { 0 }
};

static const struct message_info msg_return_verification_checked[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_check_verification },
    { TDN_VERIFICATION_CLICKED, TRUE, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static TASKDIALOGCONFIG navigated_info = {0};

static const struct message_info msg_send_navigate[] =
{
    { TDM_NAVIGATE_PAGE, 0, (LPARAM)&navigated_info, 0},
    { 0 }
};

static const struct message_info msg_return_navigated_page[] =
{
    { TDN_CREATED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, msg_send_navigate },
    { TDN_DIALOG_CONSTRUCTED, 0, 0, S_OK, NULL },
    { TDN_RADIO_BUTTON_CLICKED, ID_START_RADIO_BUTTON, 0, S_OK, NULL },
    { TDN_NAVIGATED, 0, 0, S_OK, msg_send_click_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_close[] =
{
    { WM_CLOSE, 0, 0, 0},
    { 0 }
};

static const struct message_info msg_handle_wm_close[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_close },
    { TDN_BUTTON_CLICKED, IDCANCEL, 0, S_FALSE, msg_send_close },
    { TDN_BUTTON_CLICKED, IDCANCEL, 0, S_OK, NULL },
    { 0 }
};

static const struct message_info msg_send_close_then_ok[] =
{
    { WM_CLOSE, 0, 0, 0},
    { TDM_CLICK_BUTTON, IDOK, 0 },
    { 0 }
};

static const struct message_info msg_handle_wm_close_without_cancel_button[] =
{
    { TDN_CREATED, 0, 0, S_OK, msg_send_close_then_ok },
    { TDN_BUTTON_CLICKED, IDOK, 0, S_OK, NULL },
    { 0 }
};

static void init_test_message(UINT message, WPARAM wParam, LPARAM lParam, struct message *msg)
{
    msg->message = WM_TD_CALLBACK;
    msg->flags = sent|wparam|lparam|id;
    msg->wParam = wParam;
    msg->lParam = lParam;
    msg->id = message;
    msg->stage = 0;
}

#define run_test(info, expect_button, expect_radio_button, verification_checked, seq, context) \
        run_test_(info, expect_button, expect_radio_button, verification_checked, seq, context, \
                   ARRAY_SIZE(seq) - 1, __FILE__, __LINE__)

static void run_test_(TASKDIALOGCONFIG *info, int expect_button, int expect_radio_button, BOOL verification_checked,
                      const struct message_info *test_messages, const char *context, int test_messages_len,
                      const char *file, int line)
{
    struct message *msg, *msg_start;
    int ret_button = 0;
    int ret_radio = 0;
    BOOL ret_verification = FALSE;
    HRESULT hr;
    int i;

    /* Allocate messages to test against, plus 2 implicit and 1 empty */
    msg_start = msg = heap_alloc_zero(sizeof(*msg) * (test_messages_len + 3));

    /* Always needed, thus made implicit */
    init_test_message(TDN_DIALOG_CONSTRUCTED, 0, 0, msg++);
    for (i = 0; i < test_messages_len; i++)
        init_test_message(test_messages[i].message, test_messages[i].wparam, test_messages[i].lparam, msg++);
    /* Always needed, thus made implicit */
    init_test_message(TDN_DESTROYED, 0, 0, msg++);

    current_message_info = test_messages;
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hr = pTaskDialogIndirect(info, &ret_button, &ret_radio, &ret_verification);
    ok_(file, line)(hr == S_OK, "TaskDialogIndirect() failed, got %#x.\n", hr);

    ok_sequence_(sequences, TASKDIALOG_SEQ_INDEX, msg_start, context, FALSE, file, line);
    ok_(file, line)(ret_button == expect_button,
                     "Wrong button. Expected %d, got %d\n", expect_button, ret_button);
    ok_(file, line)(ret_radio == expect_radio_button,
                     "Wrong radio button. Expected %d, got %d\n", expect_radio_button, ret_radio);

    heap_free(msg_start);
}

static const LONG_PTR test_ref_data = 123456;

static HRESULT CALLBACK taskdialog_callback_proc(HWND hwnd, UINT notification,
    WPARAM wParam, LPARAM lParam, LONG_PTR ref_data)
{
    int msg_pos = sequences[TASKDIALOG_SEQ_INDEX]->count - 1; /* Skip implicit message */
    const struct message_info *msg_send;
    struct message msg;

    ok(test_ref_data == ref_data, "Unexpected ref data %lu.\n", ref_data);

    init_test_message(notification, (short)wParam, lParam, &msg);
    add_message(sequences, TASKDIALOG_SEQ_INDEX, &msg);

    if (notification == TDN_DIALOG_CONSTRUCTED || notification == TDN_DESTROYED) /* Skip implicit messages */
        return S_OK;

    msg_send = current_message_info[msg_pos].send;
    for(; msg_send && msg_send->message; msg_send++)
        PostMessageW(hwnd, msg_send->message, msg_send->wparam, msg_send->lparam);

    return current_message_info[msg_pos].callback_retval;
}

static void test_invalid_parameters(void)
{
    TASKDIALOGCONFIG info = { 0 };
    HRESULT hr;

    hr = pTaskDialogIndirect(NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = 0;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = sizeof(TASKDIALOGCONFIG) - 1;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);

    info.cbSize = sizeof(TASKDIALOGCONFIG) + 1;
    hr = pTaskDialogIndirect(&info, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected return value %#x.\n", hr);
}

static void test_callback(void)
{
    TASKDIALOGCONFIG info = {0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;

    run_test(&info, IDOK, 0, FALSE, msg_return_press_ok, "Press VK_RETURN.");
}

static void test_buttons(void)
{
    TASKDIALOGCONFIG info = {0};
    static const DWORD command_link_flags[] = {0, TDF_USE_COMMAND_LINKS, TDF_USE_COMMAND_LINKS_NO_ICON};
    TASKDIALOG_BUTTON custom_buttons[TEST_NUM_BUTTONS], radio_buttons[TEST_NUM_RADIO_BUTTONS];
    const WCHAR button_format[] = {'%','0','2','d',0};
    /* Each button has two digits as title, plus null-terminator */
    WCHAR button_titles[TEST_NUM_BUTTONS * 3], radio_button_titles[TEST_NUM_BUTTONS * 3];
    int i;

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;

    /* Init custom buttons */
    for (i = 0; i < TEST_NUM_BUTTONS; i++)
    {
        WCHAR *text = &button_titles[i * 3];
        wsprintfW(text, button_format, i);

        custom_buttons[i].pszButtonText = text;
        custom_buttons[i].nButtonID = ID_START_BUTTON + i;
    }
    custom_buttons[TEST_NUM_BUTTONS - 1].nButtonID = -1;

    /* Init radio buttons */
    for (i = 0; i < TEST_NUM_RADIO_BUTTONS; i++)
    {
        WCHAR *text = &radio_button_titles[i * 3];
        wsprintfW(text, button_format, i);

        radio_buttons[i].pszButtonText = text;
        radio_buttons[i].nButtonID = ID_START_RADIO_BUTTON + i;
    }
    radio_buttons[TEST_NUM_RADIO_BUTTONS - 1].nButtonID = -2;

    /* Test nDefaultButton */

    /* Test common buttons with invalid default ID */
    info.nDefaultButton = 0; /* Should default to first created button */
    info.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_YES_BUTTON | TDCBF_NO_BUTTON
            | TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    run_test(&info, IDOK, 0, FALSE, msg_return_press_ok, "default button: unset default");
    info.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON
            | TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    run_test(&info, IDYES, 0, FALSE, msg_return_press_yes, "default button: unset default");
    info.dwCommonButtons = TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    run_test(&info, IDNO, 0, FALSE, msg_return_press_no, "default button: unset default");
    info.dwCommonButtons = TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    run_test(&info, IDRETRY, 0, FALSE, msg_return_press_retry, "default button: unset default");
    info.dwCommonButtons = TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON;
    run_test(&info, IDCANCEL, 0, FALSE, msg_return_press_cancel, "default button: unset default");

    /* Custom buttons could be command links */
    for (i = 0; i < ARRAY_SIZE(command_link_flags); i++)
    {
        info.dwFlags = command_link_flags[i];

        /* Test with all common and custom buttons and invalid default ID */
        info.nDefaultButton = 0xff; /* Random ID, should also default to first created button */
        info.cButtons = TEST_NUM_BUTTONS;
        info.pButtons = custom_buttons;
        run_test(&info, ID_START_BUTTON, 0, FALSE, msg_return_press_custom1,
                 "default button: invalid default, with common buttons - 1");

        info.nDefaultButton = -1; /* Should work despite button ID -1 */
        run_test(&info, -1, 0, FALSE, msg_return_press_custom10, "default button: invalid default, with common buttons - 2");

        info.nDefaultButton = -2; /* Should also default to first created button */
        run_test(&info, ID_START_BUTTON, 0, FALSE, msg_return_press_custom1,
                 "default button: invalid default, with common buttons - 3");

        /* Test with only custom buttons and invalid default ID */
        info.dwCommonButtons = 0;
        run_test(&info, ID_START_BUTTON, 0, FALSE, msg_return_press_custom1,
                 "default button: invalid default, no common buttons");

        /* Test with common and custom buttons and valid default ID */
        info.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON
                               | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
        info.nDefaultButton = IDRETRY;
        run_test(&info, IDRETRY, 0, FALSE, msg_return_press_retry, "default button: valid default - 1");

        /* Test with common and custom buttons and valid default ID */
        info.nDefaultButton = ID_START_BUTTON + 3;
        run_test(&info, ID_START_BUTTON + 3, 0, FALSE, msg_return_press_custom4, "default button: valid default - 2");
    }

    /* Test radio buttons */
    info.nDefaultButton = 0;
    info.cButtons = 0;
    info.pButtons = 0;
    info.dwCommonButtons = TDCBF_OK_BUTTON;
    info.cRadioButtons = TEST_NUM_RADIO_BUTTONS;
    info.pRadioButtons = radio_buttons;

    /* Test default first radio button */
    run_test(&info, IDOK, ID_START_RADIO_BUTTON, FALSE, msg_return_default_radio_button_1,
             "default radio button: default first radio button");

    /* Test default radio button */
    info.nDefaultRadioButton = ID_START_RADIO_BUTTON + 1;
    run_test(&info, IDOK, info.nDefaultRadioButton, FALSE, msg_return_default_radio_button_2,
             "default radio button: default radio button");

    /* Test default radio button with -2 */
    info.nDefaultRadioButton = -2;
    run_test(&info, IDOK, info.nDefaultRadioButton, FALSE, msg_return_default_radio_button_3,
             "default radio button: default radio button with id -2");

    /* Test default radio button after clicking the first, messages still work even radio button is disabled */
    info.nDefaultRadioButton = ID_START_RADIO_BUTTON + 1;
    run_test(&info, IDOK, ID_START_RADIO_BUTTON, FALSE, msg_return_first_radio_button,
             "default radio button: radio button after clicking");

    /* Test radio button after disabling and clicking the first */
    info.nDefaultRadioButton = ID_START_RADIO_BUTTON + 1;
    run_test(&info, IDOK, ID_START_RADIO_BUTTON, FALSE, msg_return_default_radio_button_clicking_disabled,
             "default radio button: disable radio button before clicking");

    /* Test no default radio button, TDF_NO_DEFAULT_RADIO_BUTTON is set, TDN_RADIO_BUTTON_CLICKED will still be received, just radio button not selected */
    info.nDefaultRadioButton = ID_START_RADIO_BUTTON;
    info.dwFlags = TDF_NO_DEFAULT_RADIO_BUTTON;
    run_test(&info, IDOK, info.nDefaultRadioButton, FALSE, msg_return_no_default_radio_button_flag,
             "default radio button: no default radio flag");

    /* Test no default radio button, TDF_NO_DEFAULT_RADIO_BUTTON is set and nDefaultRadioButton is 0.
     * TDN_RADIO_BUTTON_CLICKED will not be sent, and just radio button not selected */
    info.nDefaultRadioButton = 0;
    info.dwFlags = TDF_NO_DEFAULT_RADIO_BUTTON;
    run_test(&info, IDOK, 0, FALSE, msg_return_no_default_radio_button_id_and_flag,
             "default radio button: no default radio id and flag");

    /* Test no default radio button, TDF_NO_DEFAULT_RADIO_BUTTON is set and nDefaultRadioButton is invalid.
     * TDN_RADIO_BUTTON_CLICKED will not be sent, and just radio button not selected */
    info.nDefaultRadioButton = 0xff;
    info.dwFlags = TDF_NO_DEFAULT_RADIO_BUTTON;
    run_test(&info, IDOK, 0, FALSE, msg_return_no_default_radio_button_id_and_flag,
             "default radio button: no default flag, invalid id");

    info.nDefaultRadioButton = 0;
    info.dwFlags = TDF_NO_DEFAULT_RADIO_BUTTON;
    run_test(&info, IDOK, -2, FALSE, msg_return_press_negative_id_radio_button,
             "radio button: manually click radio button with negative id");

    /* Test sending clicks to non-existent buttons. Notification of non-existent buttons will be sent */
    info.cButtons = TEST_NUM_BUTTONS;
    info.pButtons = custom_buttons;
    info.cRadioButtons = TEST_NUM_RADIO_BUTTONS;
    info.pRadioButtons = radio_buttons;
    info.dwCommonButtons = 0;
    info.dwFlags = TDF_NO_DEFAULT_RADIO_BUTTON;
    run_test(&info, ID_START_BUTTON + 99, 0, FALSE, msg_press_nonexistent_buttons, "sends click to non-existent buttons");

    /* Non-existent button clicks sent by WM_COMMAND won't generate TDN_BUTTON_CLICKED except IDOK.
     * And will get the first existent button identifier instead of IDOK */
    run_test(&info, ID_START_BUTTON, 0, FALSE, msg_press_nonexistent_buttons_with_command,
             "sends click to non-existent buttons with WM_COMMAND");

    /* Non-existent radio button won't get notifications */
    run_test(&info, IDOK, 0, FALSE, msg_press_nonexistent_radio_button, "sends click to non-existent radio buttons");
}

static void test_help(void)
{
    TASKDIALOGCONFIG info = {0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;
    info.dwCommonButtons = TDCBF_OK_BUTTON;

    run_test(&info, IDOK, 0, FALSE, msg_got_tdn_help, "send f1");
}

struct timer_notification_data
{
    DWORD last_elapsed_ms;
    DWORD num_fired;
};

static HRESULT CALLBACK taskdialog_callback_proc_timer(HWND hwnd, UINT notification,
        WPARAM wParam, LPARAM lParam, LONG_PTR ref_data)
{
    struct timer_notification_data *data = (struct timer_notification_data *)ref_data;

    if (notification == TDN_TIMER)
    {
        DWORD elapsed_ms;
        int delta;

        elapsed_ms = (DWORD)wParam;

        if (data->num_fired == 3)
            ok(data->last_elapsed_ms > elapsed_ms, "Expected reference time update.\n");
        else
        {
            delta = elapsed_ms - data->last_elapsed_ms;
            ok(delta > 0, "Expected positive time tick difference.\n");
        }
        data->last_elapsed_ms = elapsed_ms;

        if (data->num_fired == 3)
            PostMessageW(hwnd, TDM_CLICK_BUTTON, IDOK, 0);

        ++data->num_fired;
        return data->num_fired == 3 ? S_FALSE : S_OK;
    }

    return S_OK;
}

static void test_timer(void)
{
    struct timer_notification_data data = { 0 };
    TASKDIALOGCONFIG info = { 0 };

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc_timer;
    info.lpCallbackData = (LONG_PTR)&data;
    info.dwFlags = TDF_CALLBACK_TIMER;
    info.dwCommonButtons = TDCBF_OK_BUTTON;

    pTaskDialogIndirect(&info, NULL, NULL, NULL);
}

static HRESULT CALLBACK taskdialog_callback_proc_progress_bar(HWND hwnd, UINT notification, WPARAM wParam,
                                                              LPARAM lParam, LONG_PTR ref_data)
{
    unsigned long ret;
    LONG flags = (LONG)ref_data;
    if (notification == TDN_CREATED)
    {
        /* TDM_SET_PROGRESS_BAR_STATE */
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
        ok(ret == PBST_NORMAL, "Expect state: %d got state: %lx\n", PBST_NORMAL, ret);
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_PAUSED, 0);
        ok(ret == PBST_NORMAL, "Expect state: %d got state: %lx\n", PBST_NORMAL, ret);
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_ERROR, 0);
        /* Progress bar has fixme on handling PBM_SETSTATE message */
        todo_wine ok(ret == PBST_PAUSED, "Expect state: %d got state: %lx\n", PBST_PAUSED, ret);
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
        todo_wine ok(ret == PBST_ERROR, "Expect state: %d got state: %lx\n", PBST_ERROR, ret);

        /* TDM_SET_PROGRESS_BAR_RANGE */
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM(0, 200));
        ok(ret == MAKELONG(0, 100), "Expect range:%x got:%lx\n", MAKELONG(0, 100), ret);
        ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM(0, 200));
        ok(ret == MAKELONG(0, 200), "Expect range:%x got:%lx\n", MAKELONG(0, 200), ret);

        /* TDM_SET_PROGRESS_BAR_POS */
        if (flags & TDF_SHOW_MARQUEE_PROGRESS_BAR)
        {
            ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, 1, 0);
            ok(ret == 0, "Expect position:%x got:%lx\n", 0, ret);
            ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, 2, 0);
            ok(ret == 0, "Expect position:%x got:%lx\n", 0, ret);
        }
        else
        {
            ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, 1, 0);
            ok(ret == 0, "Expect position:%x got:%lx\n", 0, ret);
            ret = SendMessageW(hwnd, TDM_SET_PROGRESS_BAR_POS, 2, 0);
            ok(ret == 1, "Expect position:%x got:%lx\n", 1, ret);
        }

        SendMessageW(hwnd, TDM_CLICK_BUTTON, IDOK, 0);
    }

    return S_OK;
}

static void test_progress_bar(void)
{
    TASKDIALOGCONFIG info = {0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.dwFlags = TDF_SHOW_PROGRESS_BAR;
    info.pfCallback = taskdialog_callback_proc_progress_bar;
    info.lpCallbackData = (LONG_PTR)info.dwFlags;
    info.dwCommonButtons = TDCBF_OK_BUTTON;
    pTaskDialogIndirect(&info, NULL, NULL, NULL);

    info.dwFlags = TDF_SHOW_MARQUEE_PROGRESS_BAR;
    info.lpCallbackData = (LONG_PTR)info.dwFlags;
    pTaskDialogIndirect(&info, NULL, NULL, NULL);

    info.dwFlags = TDF_SHOW_PROGRESS_BAR | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    info.lpCallbackData = (LONG_PTR)info.dwFlags;
    pTaskDialogIndirect(&info, NULL, NULL, NULL);
}

static void test_verification_box(void)
{
    TASKDIALOGCONFIG info = {0};
    WCHAR textW[] = {'t', 'e', 'x', 't', 0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;
    info.dwCommonButtons = TDCBF_OK_BUTTON;

    /* TDF_VERIFICATION_FLAG_CHECKED works even if pszVerificationText is not set */
    run_test(&info, IDOK, 0, FALSE, msg_return_default_verification_unchecked, "default verification box: unchecked");

    info.dwFlags = TDF_VERIFICATION_FLAG_CHECKED;
    run_test(&info, IDOK, 0, FALSE, msg_return_default_verification_checked, "default verification box: checked");

    info.pszVerificationText = textW;
    run_test(&info, IDOK, 0, FALSE, msg_return_default_verification_unchecked, "default verification box: unchecked");

    info.dwFlags = TDF_VERIFICATION_FLAG_CHECKED;
    run_test(&info, IDOK, 0, FALSE, msg_return_default_verification_checked, "default verification box: checked");

    run_test(&info, IDOK, 0, FALSE, msg_return_verification_unchecked,
             "default verification box: default checked and then unchecked");

    info.dwFlags = 0;
    run_test(&info, IDOK, 0, FALSE, msg_return_verification_checked,
             "default verification box: default unchecked and then checked");
}

static void test_navigate_page(void)
{
    TASKDIALOGCONFIG info = {0};
    static const WCHAR textW[] = {'t', 'e', 'x', 't', 0};
    static const WCHAR button_format[] = {'%', '0', '2', 'd', 0};
    TASKDIALOG_BUTTON radio_buttons[TEST_NUM_RADIO_BUTTONS];
    WCHAR radio_button_titles[TEST_NUM_BUTTONS * 3];
    int i;

    /* Init radio buttons */
    for (i = 0; i < TEST_NUM_RADIO_BUTTONS; i++)
    {
        WCHAR *text = &radio_button_titles[i * 3];
        wsprintfW(text, button_format, i);

        radio_buttons[i].pszButtonText = text;
        radio_buttons[i].nButtonID = ID_START_RADIO_BUTTON + i;
    }

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;
    info.dwCommonButtons = TDCBF_OK_BUTTON;
    info.cRadioButtons = TEST_NUM_RADIO_BUTTONS;
    info.pRadioButtons = radio_buttons;

    navigated_info = info;
    navigated_info.pszVerificationText = textW;
    navigated_info.dwFlags = TDF_VERIFICATION_FLAG_CHECKED;

    run_test(&info, IDOK, ID_START_RADIO_BUTTON, TRUE, msg_return_navigated_page, "navigate page: default");

    /* TDM_NAVIGATE_PAGE doesn't check cbSize.
     * And null taskconfig pointer crash applicatioin, thus doesn't check pointer either */
    navigated_info.cbSize = 0;
    run_test(&info, IDOK, ID_START_RADIO_BUTTON, TRUE, msg_return_navigated_page, "navigate page: invalid taskconfig cbSize");
}

static void test_wm_close(void)
{
    TASKDIALOGCONFIG info = {0};

    info.cbSize = sizeof(TASKDIALOGCONFIG);
    info.pfCallback = taskdialog_callback_proc;
    info.lpCallbackData = test_ref_data;

    /* WM_CLOSE can end the dialog only when a cancel button is present or dwFlags has TDF_ALLOW_DIALOG_CANCELLATION */
    info.dwCommonButtons = TDCBF_OK_BUTTON;
    run_test(&info, IDOK, 0, FALSE, msg_handle_wm_close_without_cancel_button, "send WM_CLOSE without cancel button");

    info.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    run_test(&info, IDCANCEL, 0, FALSE, msg_handle_wm_close, "send WM_CLOSE with TDF_ALLOW_DIALOG_CANCELLATION");

    info.dwFlags = 0;
    info.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    run_test(&info, IDCANCEL, 0, FALSE, msg_handle_wm_close, "send WM_CLOSE with a cancel button");
}

START_TEST(taskdialog)
{
    ULONG_PTR ctx_cookie;
    void *ptr_ordinal;
    HINSTANCE hinst;
    HANDLE hCtx;

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    /* Check if task dialogs are available */
    hinst = LoadLibraryA("comctl32.dll");

    pTaskDialogIndirect = (void *)GetProcAddress(hinst, "TaskDialogIndirect");
    if (!pTaskDialogIndirect)
    {
        win_skip("TaskDialogIndirect not exported by name\n");
        unload_v6_module(ctx_cookie, hCtx);
        return;
    }

    pTaskDialog = (void *)GetProcAddress(hinst, "TaskDialog");

    ptr_ordinal = GetProcAddress(hinst, (const char *)344);
    ok(pTaskDialog == ptr_ordinal, "got wrong pointer for ordinal 344, %p expected %p\n",
                                            ptr_ordinal, pTaskDialog);

    ptr_ordinal = GetProcAddress(hinst, (const char *)345);
    ok(pTaskDialogIndirect == ptr_ordinal, "got wrong pointer for ordinal 345, %p expected %p\n",
                                            ptr_ordinal, pTaskDialogIndirect);

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_invalid_parameters();
    test_callback();
    test_buttons();
    test_help();
    test_timer();
    test_progress_bar();
    test_verification_box();
    test_navigate_page();
    test_wm_close();

    unload_v6_module(ctx_cookie, hCtx);
}
