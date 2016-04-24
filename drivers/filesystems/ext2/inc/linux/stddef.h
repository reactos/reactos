#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

enum {
    false	= 0,
    true	= 1
};

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif /* _LINUX_STDDEF_H */
