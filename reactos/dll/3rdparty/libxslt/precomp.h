#ifndef _LIBXSLT_PCH_
#define _LIBXSLT_PCH_

#define IN_LIBXSLT
#include <libxslt.h>

#ifndef	XSLT_NEED_TRIO
#include <stdio.h>
#else
#include <trio.h>
#endif

#include <string.h>
#include <limits.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/valid.h>
#include <libxml/encoding.h>
#include <libxml/dict.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>

#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "attributes.h"
#include "namespaces.h"
#include "templates.h"
#include "imports.h"
#include "transform.h"
#include "preproc.h"
#include "documents.h"
#include "keys.h"
#include "security.h"
#include "extensions.h"
#include "variables.h"
#include "extra.h"
#include "pattern.h"
#include "numbersInternals.h"
#include "functions.h"

#endif /* _LIBXSLT_PCH_ */
