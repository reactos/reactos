#include "types.r"
#include "systypes.r"
//#include "ebappids.h"
//#include "resids.h"
#include "verstamp.h"

#define STRING(x) #x


#ifdef MKTYPLIB2		// only if special build

resource 'ALRT' (128) {
	{38, 44, 260, 438},
	128,
	{	/* array: 4 elements */
		/* [1] */
		OK, visible, sound1,
		/* [2] */
		OK, visible, sound1,
		/* [3] */
		OK, visible, sound1,
		/* [4] */
		OK, visible, sound1
	}
};

resource 'DITL' (128) {
	{	/* array DITLarray: 2 elements */
		/* [1] */
		{194, 150, 214, 208},
		Button {
			enabled,
			"Ok"
		},
		/* [2] */
		{14, 17, 184, 380},
		StaticText {
			disabled,
			"^0"
		}
	}
};
#endif //MKTYPLIB2


resource 'SIZE' (-1) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,				/* we can background; we don't currently, but our sleep value */
								/* guarantees we don't hog the Mac while we are in the background */
	multiFinderAware,			/* this says we do our own activate/deactivate; don't fake us out */
	backgroundAndForeground,	/* this is definitely not a background-only application! */
	dontGetFrontClicks,			/* change this is if you want "do first click" behavior like the Finder */
	ignoreChildDiedEvents,		/* essentially, I'm not a debugger (sub-launching) */
	is32BitCompatible,			/* this app should not be run in 32-bit address space */
	isHighLevelEventAware,
	reserved,
	reserved,
	useTextEditServices,
	reserved,
	reserved,
	reserved,
	2048 * 1024,		// preferred size
	512 * 1024		// min size
};


#ifdef MKTYPLIB2		// only if special build
resource 'BNDL' (1009) {
	'mktl',
	0,
	{	/* array TypeArray: 2 elements */
		/* [1] */
		'FREF',
		{	/* array IDArray: 2 elements */
			/* [1] */
			0, 128,
			/* [2] */
			1, 129
		},
		/* [2] */
		'ICN#',
		{	/* array IDArray: 2 elements */
			/* [1] */
			0, 128,
			/* [2] */
			1, 129
		}
	}
};

data 'mktl' (0, "Owner resource") {
	$"00"                                                 /* . */
};

resource 'FREF' (128) {
	'APPL',
	0,
	""
};

data 'FREF' (129) {
	$"5445 5854 0001 7F"                                  /* TEXT... */
};

resource 'ICN#' (128) {
	{	/* array: 2 elements */
		/* [1] */
		$"0000 0000 0000 0000 0000 0000 0000 0000"
		$"0048 C000 0023 8000 0031 8003 0009 0803"
		$"000C 1FFE 0006 03C6 0006 01EC 0006 0078"
		$"0006 0070 00FD E050 077F F030 0DEF F870"
		$"0B1B 1A70 1B80 04F0 1384 03F0 1A00 01F0"
		$"1800 0030 0C80 80B0 0F80 2138 0700 FDF8"
		$"0FF6 FFF0 3FFF FFF4 7FFF FDFC 3FFF DFFE",
		/* [2] */
		$"0000 0000 0000 0000 0000 0000 0000 0000"
		$"006A C000 0033 8000 003F 8003 000F 1C07"
		$"000E 1FFE 0006 07FE 0006 01FC 0006 00F8"
		$"0006 0078 00FF E078 07FF F870 0FFF FC70"
		$"0FFF FE70 1FFF FFF0 1FFF FFF0 1FFF FFF0"
		$"1FFF FFF0 0FFF FFF8 0FFF FFF8 07FF FFF8"
		$"0FFF FFF0 3FFF FFF4 7FFF FFFC 3FFF DFFE"
	}
};

resource 'ICN#' (129) {
	{	/* array: 2 elements */
		/* [1] */
		$"0000 0000 0000 0000 01FF FF80 0100 00C0"
		$"0100 00A0 0100 0090 0103 E088 0106 30FC"
		$"010C 1004 0108 1004 0108 1004 0108 13E4"
		$"0108 1634 0108 2414 010C 6C14 0107 C814"
		$"0100 0814 0100 0FE4 0100 1864 0100 1024"
		$"0100 2024 0100 2024 0100 6064 0100 40C4"
		$"0100 7F84 0100 0004 0100 0004 0100 0004"
		$"01FF FFFC",
		/* [2] */
		$"0000 0000 0000 0000 01FF FF80 01FF FFC0"
		$"01FF FFE0 01FF FFF0 01FF FFF8 01FF FFFC"
		$"01FF FFFC 01FF FFFC 01FF FFFC 01FF FFFC"
		$"01FF FFFC 01FF FFFC 01FF FFFC 01FF FFFC"
		$"01FF FFFC 01FF FFFC 01FF FFFC 01FF FFFC"
		$"01FF FFFC 01FF FFFC 01FF FFFC 01FF FFFC"
		$"01FF FFFC 01FF FFFC 01FF FFFC 01FF FFFC"
		$"01FF FFFC"
	}
};
#endif //MKTYPLIB2


// version info
// "MkTypLib App, (c) Microsoft Corp. 1993-1994"
#define VERSTRINGX(maj,min,rev) STRING(maj ## . ## min ## . ## rev)
#define VERSTRING VERSTRINGX(rmj,rmm,rup)
resource 'vers' (1, purgeable) {
	0x00,
	0x00,
	development,
	0x00,
	verUS,
	VERSTRING,
	VERSTRING,
};
