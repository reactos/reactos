/*
	Just to ensure that libraries and programs get their separate
	compatibility object. There should be a compatibility library,
	I presume, but I don't want to create another glib, I just want
	some internal functions to ease coding.

	I'll sort it out properly sometime.

	I smell symbol conflicts, anyway. Actually wondering why it
	worked so far.
*/

#include "config.h"
#include "intsym.h"
#define NO_CATCHSIGNAL
#include "compat/compat_impl.h"
