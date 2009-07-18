/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/winesup.h
 * PURPOSE:         Wine supporting functions
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#ifndef _INCLUDE_INTERNAL_WINESUP_H
#define _INCLUDE_INTERNAL_WINESUP_H

/* INCLUDES ******************************************************************/

#define assert ASSERT
#define tolowerW(n) towlower((n))
#define strncmpiW(s1,s2,n) _wcsnicmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define set_win32_error(x) SetLastWin32Error(x)

void set_error( unsigned int err );
static inline void clear_error(void)             { set_error(0); }
struct window_class* get_window_class( user_handle_t window );

/* gets the discretionary access control list from a security descriptor */
static inline const ACL *sd_get_dacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_DACL_PRESENT ? TRUE : FALSE);

    if (sd->dacl_len)
        return (const ACL *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len + sd->sacl_len);
    else
        return NULL;
}

/* gets the system access control list from a security descriptor */
static inline const ACL *sd_get_sacl( const struct security_descriptor *sd, int *present )
{
    *present = (sd->control & SE_SACL_PRESENT ? TRUE : FALSE);

    if (sd->sacl_len)
        return (const ACL *)((const char *)(sd + 1) +
            sd->owner_len + sd->group_len);
    else
        return NULL;
}

/* gets the owner from a security descriptor */
static inline const SID *sd_get_owner( const struct security_descriptor *sd )
{
    if (sd->owner_len)
        return (const SID *)(sd + 1);
    else
        return NULL;
}

/* gets the primary group from a security descriptor */
static inline const SID *sd_get_group( const struct security_descriptor *sd )
{
    if (sd->group_len)
        return (const SID *)((const char *)(sd + 1) + sd->owner_len);
    else
        return NULL;
}

int dump_strW( const WCHAR *str, data_size_t len, FILE *f, const char escape[2] );
const SID *token_get_user( void *token );

// misc stuff, should be moved elsewhere
#define DESKTOP_ATOM  ((atom_t)32769)

#endif
