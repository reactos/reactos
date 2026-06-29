/***
*crtmbox.c - CRT MessageBoxA wrapper.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Wrap MessageBoxA.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>



namespace
{
    template <typename Character>
    struct message_box_arguments
    {
        Character const* _caption;
        Character const* _text;
        unsigned int     _type;
        int              _result;
    };
}

template <typename Character>
static int __cdecl common_show_message_box(
    Character const* const text,
    Character const* const caption,
    unsigned         const type
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    bool const debugger_attached = IsDebuggerPresent();
    if (debugger_attached)
    {
        // Output the message to the attached debugger.  This is useful when
        // remotely debugging.
        if (text)
        {
            traits::output_debug_string(text);
        }

        // We do not want to display a message box if a debugger
        // is already attached, unless it's a desktop app.
        // Instead, let the caller know that we want to
        // directly break into the debugger:
        bool const is_desktop_app = __acrt_get_windowing_model_policy() == windowing_model_policy_hwnd;
        if (!is_desktop_app)
        {
            return IDRETRY; // Retry = Break into the debugger
        }
    }

    bool const show_ui = __acrt_get_developer_information_policy() == developer_information_policy_ui;
    if (!show_ui || !__acrt_can_show_message_box())
    {
        // If we can't get the message box pointers (perhaps because running on CoreSys),
        // just abort, unless a debugger is attached--then break into the debugger instead.
        // The message string was already output to the debugger above.
        return debugger_attached ? IDRETRY : IDABORT;
    }

    // If the current process isn't attached to a visible window station (e.g.
    // a non-interactive service), then we need to set the MB_SERVICE_NOTIFICATION
    // flag, otherwise the message box will be invisible, stalling the program.
    if (!__acrt_is_interactive())
    {
        return traits::message_box(nullptr, text, caption, type | MB_SERVICE_NOTIFICATION);
    }

    return traits::message_box(__acrt_get_parent_window(), text, caption, type);
}

extern "C" int __cdecl __acrt_show_narrow_message_box(
    char const* const text,
    char const* const caption,
    unsigned    const type
    )
{
    return common_show_message_box(text, caption, type);
}

extern "C" int __cdecl __acrt_show_wide_message_box(
    wchar_t const* const text,
    wchar_t const* const caption,
    unsigned       const type
    )
{
    return common_show_message_box(text, caption, type);
}
