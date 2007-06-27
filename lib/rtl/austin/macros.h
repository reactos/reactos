/*
 * Austin---Astonishing Universal Search Tree Interface Novelty
 * Copyright (C) 2000 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: macros.h,v 1.1 1999/11/26 05:59:49 kaz Exp $
 * $Name: austin_0_2 $
 */
/*
 * Modified for use in ReactOS by arty
 */

/*
 * Macros which give short, convenient internal names to public structure
 * members. These members have prefixed names to reduce the possiblity of
 * clashes with foreign macros.
 */

#define left LeftChild
#define right RightChild
#define parent Parent
#define next RightChild
#define prev LeftChild
#define data(x) ((void *)&((x)[1]))
#define key(x) ((void *)&((x)[1]))
#define rb_color udict_rb_color
#define algo_specific udict_algo_specific

#define optable udict_optable
#define nodecount NumberGenericTableElements
#define maxcount udict_maxcount
#define dupes_allowed udict_dupes_allowed
#define sentinel BalancedRoot
#define compare CompareRoutine
#define nodealloc AllocateRoutine
#define nodefree FreeRoutine
#define context TableContext

#define assert(x) { if(!(x)) { RtlAssert(#x, __FILE__, __LINE__, NULL); } }

