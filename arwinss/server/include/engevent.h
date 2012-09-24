#ifndef __WIN32K_ENGEVENT_H
#define __WIN32K_ENGEVENT_H

#define ENG_EVENT_USERMAPPED 1

typedef struct _ENG_EVENT
{
    /* Public part */
    PVOID  pKEvent;
    ULONG  fFlags;

    /* Private part */
    KEVENT KEvent;
} ENG_EVENT, *PENG_EVENT;

#endif
