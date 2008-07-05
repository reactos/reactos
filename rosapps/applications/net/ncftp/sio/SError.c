#include "syshdrs.h"

#if defined(WIN32) || defined(_WINDOWS)

const char *wsaerrlist[128] = {
	/*   0 */	"Unknown error #0",
	/*   1 */	"Unknown error #1",
	/*   2 */	"Unknown error #2",
	/*   3 */	"Unknown error #3",
	/*   4 */	"Interrupted system call",
	/*   5 */	"Unknown error #5",
	/*   6 */	"Unknown error #6",
	/*   7 */	"Unknown error #7",
	/*   8 */	"Unknown error #8",
	/*   9 */	"Bad file descriptor",
	/*  10 */	"Unknown error #10",
	/*  11 */	"Unknown error #11",
	/*  12 */	"Unknown error #12",
	/*  13 */	"Permission denied",
	/*  14 */	"Bad address",
	/*  15 */	"Unknown error #15",
	/*  16 */	"Unknown error #16",
	/*  17 */	"Unknown error #17",
	/*  18 */	"Unknown error #18",
	/*  19 */	"Unknown error #19",
	/*  20 */	"Unknown error #20",
	/*  21 */	"Unknown error #21",
	/*  22 */	"Invalid argument",
	/*  23 */	"Unknown error #23",
	/*  24 */	"Too many open files",
	/*  25 */	"Unknown error #25",
	/*  26 */	"Unknown error #26",
	/*  27 */	"Unknown error #27",
	/*  28 */	"Unknown error #28",
	/*  29 */	"Unknown error #29",
	/*  30 */	"Unknown error #30",
	/*  31 */	"Unknown error #31",
	/*  32 */	"Unknown error #32",
	/*  33 */	"Unknown error #33",
	/*  34 */	"Unknown error #34",
	/*  35 */	"Resource temporarily unavailable",
	/*  36 */	"Operation now in progress",
	/*  37 */	"Operation already in progress",
	/*  38 */	"Socket operation on non-socket",
	/*  39 */	"Destination address required",
	/*  40 */	"Message too long",
	/*  41 */	"Protocol wrong type for socket",
	/*  42 */	"Protocol not available",
	/*  43 */	"Protocol not supported",
	/*  44 */	"Socket type not supported",
	/*  45 */	"Operation not supported",
	/*  46 */	"Protocol family not supported",
	/*  47 */	"Address family not supported by protocol",
	/*  48 */	"Address already in use",
	/*  49 */	"Cannot assign requested address",
	/*  50 */	"Network is down",
	/*  51 */	"Network is unreachable",
	/*  52 */	"Network dropped connection on reset",
	/*  53 */	"Software caused connection abort",
	/*  54 */	"Connection reset by peer",
	/*  55 */	"No buffer space available",
	/*  56 */	"Transport endpoint is already connected",
	/*  57 */	"Transport endpoint is not connected",
	/*  58 */	"Cannot send after transport endpoint shutdown",
	/*  59 */	"Too many references: cannot splice",
	/*  60 */	"Connection timed out",
	/*  61 */	"Connection refused",
	/*  62 */	"Too many levels of symbolic links",
	/*  63 */	"File name too long",
	/*  64 */	"Host is down",
	/*  65 */	"No route to host",
	/*  66 */	"Directory not empty",
	/*  67 */	"WSAEPROCLIM error",
	/*  68 */	"Too many users",
	/*  69 */	"Disc quota exceeded",
	/*  70 */	"Stale NFS file handle",
	/*  71 */	"Object is remote",
	/*  72 */	"Unknown error #72",
	/*  73 */	"Unknown error #73",
	/*  74 */	"Unknown error #74",
	/*  75 */	"Unknown error #75",
	/*  76 */	"Unknown error #76",
	/*  77 */	"Unknown error #77",
	/*  78 */	"Unknown error #78",
	/*  79 */	"Unknown error #79",
	/*  80 */	"Unknown error #80",
	/*  81 */	"Unknown error #81",
	/*  82 */	"Unknown error #82",
	/*  83 */	"Unknown error #83",
	/*  84 */	"Unknown error #84",
	/*  85 */	"Unknown error #85",
	/*  86 */	"Unknown error #86",
	/*  87 */	"Unknown error #87",
	/*  88 */	"Unknown error #88",
	/*  89 */	"Unknown error #89",
	/*  90 */	"Unknown error #90",
	/*  91 */	"WSASYSNOTREADY error",
	/*  92 */	"Version not supported",
	/*  93 */	"Winsock not initialised",
	/*  94 */	"Unknown error #94",
	/*  95 */	"Unknown error #95",
	/*  96 */	"Unknown error #96",
	/*  97 */	"Unknown error #97",
	/*  98 */	"Unknown error #98",
	/*  99 */	"Unknown error #99",
	/* 100 */	"Unknown error #100",
	/* 101 */	"WSAEDISCON error",
	/* 102 */	"Unknown error #102",
	/* 103 */	"Unknown error #103",
	/* 104 */	"Unknown error #104",
	/* 105 */	"Unknown error #105",
	/* 106 */	"Unknown error #106",
	/* 107 */	"Unknown error #107",
	/* 108 */	"Unknown error #108",
	/* 109 */	"Unknown error #109",
	/* 110 */	"Unknown error #110",
	/* 111 */	"Unknown error #111",
	/* 112 */	"Unknown error #112",
	/* 113 */	"Unknown error #113",
	/* 114 */	"Unknown error #114",
	/* 115 */	"Unknown error #115",
	/* 116 */	"Unknown error #116",
	/* 117 */	"Unknown error #117",
	/* 118 */	"Unknown error #118",
	/* 119 */	"Unknown error #119",
	/* 120 */	"Unknown error #120",
	/* 121 */	"Unknown error #121",
	/* 122 */	"Unknown error #122",
	/* 123 */	"Unknown error #123",
	/* 124 */	"Unknown error #124",
	/* 125 */	"Unknown error #125",
	/* 126 */	"Unknown error #126",
	/* 127 */	"Unknown error #127",
};

#endif	/* Windows */



const char *
SError(int e)
{
#if defined(WIN32) || defined(_WINDOWS)
	const char *cp;
	static char estr[32];

	if (e == 0)
		e = WSAGetLastError();

	if ((e >= WSABASEERR) && (e < (WSABASEERR + (sizeof(wsaerrlist) / sizeof(const char *))))) {
		return wsaerrlist[e - WSABASEERR];
	}

	cp = strerror(e);
	if ((cp == NULL) || (cp[0] == '\0') || (strcmp(cp, "Unknown error") == 0)) {
		wsprintf(estr, "Error #%d", e);
		cp = estr;
	}
	return cp;
#elif defined(HAVE_STRERROR)
	if (e == 0)
		e = errno;
	return strerror(e);
#else
	static char estr[32];
	if (e == 0)
		e = errno;
	sprintf(estr, "Error #%d", e);
	return (estr);
#endif
}	/* SError */
