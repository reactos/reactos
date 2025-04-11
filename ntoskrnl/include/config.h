#pragma once

// Enable global page support.
#if defined(_M_AMD64) && !defined(CONFIG_SMP)
#define _GLOBAL_PAGES_ARE_AWESOME_
#endif
