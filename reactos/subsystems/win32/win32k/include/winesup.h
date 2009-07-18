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

/* from wine/unicode.h */
#define memicmpW(s1,s2,n) _wcsnicmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strlenW(s) wcslen((const wchar_t *)(s))
#define strcpyW(d,s) wcscpy((wchar_t *)(d),(const wchar_t *)(s))
#define strcatW(d,s) wcscat((wchar_t *)(d),(const wchar_t *)(s))
#define strcspnW(d,s) wcscspn((wchar_t *)(d),(const wchar_t *)(s))
#define strstrW(d,s) wcsstr((const wchar_t *)(d),(const wchar_t *)(s))
#define strtolW(s,e,b) wcstol((const wchar_t *)(s),(wchar_t **)(e),(b))
#define strchrW(s,c) wcschr((const wchar_t *)(s),(wchar_t)(c))
#define strrchrW(s,c) wcsrchr((const wchar_t *)(s),(wchar_t)(c))
#define strncmpW(s1,s2,n) wcsncmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strcmpW(s1,s2) wcscmp((const wchar_t *)(s1),(const wchar_t *)(s2))
#define strcmpiW(s1,s2) _wcsicmp((const wchar_t *)(s1),(const wchar_t *)(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((const wchar_t *)(s1),(const wchar_t *)(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((const wchar_t *)(s1),(wchar_t **)(s2),(b))
#define strspnW(str, accept) wcsspn((const wchar_t *)(str), (const wchar_t *)(accept))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define atoiW(s) _wtoi((const wchar_t *)(s))
#define atolW(s) _wtol((const wchar_t *)(s))
#define strlwrW(s) _wcslwr((wchar_t *)(s))
#define struprW(s) _wcsupr((wchar_t *)(s))
#define sprintfW swprintf
#define vsprintfW vswprintf
#define snprintfW _snwprintf
#define vsnprintfW _vsnwprintf

#define set_win32_error(x) SetLastWin32Error(x)
#define assert ASSERT

/* HACK */
int memcmp(const void *s1, const void *s2, size_t n);

/* from winbase.h */
#define HANDLE_FLAG_INHERIT             0x00000001
#define HANDLE_FLAG_PROTECT_FROM_CLOSE  0x00000002

/* from winuser.h */
#define XBUTTON1            0x0001
#define XBUTTON2            0x0002

// misc stuff, should be moved elsewhere
void set_error( unsigned int err );
unsigned int get_error(void);
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

static inline wchar_t *memchrW( const wchar_t *ptr, wchar_t ch, size_t n )
{
    const wchar_t *end;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) return (wchar_t *)(ULONG_PTR)ptr;
    return NULL;
}


int dump_strW( const WCHAR *str, data_size_t len, FILE *f, const char escape[2] );
const SID *token_get_user( void *token );
static inline int sd_is_valid( const struct security_descriptor *sd, data_size_t size ) { return TRUE; };
struct object;
static inline int check_object_access(struct object *obj, unsigned int *access) { return TRUE; };

// timeout stuff
struct timeout_user;
enum timeout_t;
typedef PKDEFERRED_ROUTINE timeout_callback;
#define TICKS_PER_SEC 10000000
void remove_timeout_user( struct timeout_user *user );
struct timeout_user *add_timeout_user( timeout_t when, timeout_callback func, void *private );

void wake_up( struct object *obj, int max );

thread_id_t get_thread_id (PTHREADINFO Thread);
process_id_t get_process_id(PPROCESSINFO Process);

// fd stuff
struct wait_queue_entry;

struct fd {};

struct event {};

struct async_queue {};

unsigned int default_fd_map_access( struct object *obj, unsigned int access );
void set_fd_events( struct fd *fd, int events );
int check_fd_events( struct fd *fd, int events );
void *get_fd_user( struct fd *fd );
int add_queue( struct object *obj, struct wait_queue_entry *entry );
void remove_queue( struct object *obj, struct wait_queue_entry *entry );

#define POLLIN		0x0001
#define POLLPRI		0x0002
#define POLLOUT		0x0004
#define POLLERR		0x0008
#define POLLHUP		0x0010
#define POLLNVAL	0x0020

/* operations valid on file descriptor objects */
struct fd_ops
{
    /* get the events we want to poll() for on this object */
    int  (*get_poll_events)(struct fd *);
    /* a poll() event occurred */
    void (*poll_event)(struct fd *,int event);
    /* flush the object buffers */
    void (*flush)(struct fd *, struct event **);
    /* get file information */
    enum server_fd_type (*get_fd_type)(struct fd *fd);
    /* perform an ioctl on the file */
    obj_handle_t (*ioctl)(struct fd *fd, ioctl_code_t code, const async_data_t *async, int blocking,
                          const void *data, data_size_t size);
    /* queue an async operation */
    void (*queue_async)(struct fd *, const async_data_t *data, int type, int count);
    /* selected events for async i/o need an update */
    void (*reselect_async)( struct fd *, struct async_queue *queue );
    /* cancel an async operation */
    void (*cancel_async)(struct fd *);
};

PVOID NTAPI ExReallocPool(PVOID OldPtr, ULONG NewSize, ULONG OldSize);

#endif
