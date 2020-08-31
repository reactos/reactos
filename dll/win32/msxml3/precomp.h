
#ifndef _MSXML3_PCH_
#define _MSXML3_PCH_

#include <config.h>
#include <wine/port.h>

#include <assert.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION

#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
# include <libxml/xpathInternals.h>
# include <libxml/xmlsave.h>
# include <libxml/xmlschemas.h>
# include <libxml/schemasInternals.h>
# include <libxml/HTMLtree.h>
# include <libxml/SAX2.h>
# include <libxml/parserInternals.h>
# ifdef SONAME_LIBXSLT
#  ifdef HAVE_LIBXSLT_PATTERN_H
#   include <libxslt/pattern.h>
#  endif
#  ifdef HAVE_LIBXSLT_TRANSFORM_H
#   include <libxslt/transform.h>
#  endif
#  include <libxslt/imports.h>
#  include <libxslt/xsltutils.h>
#  include <libxslt/variables.h>
#  include <libxslt/xsltInternals.h>
#  include <libxslt/documents.h>
#  include <libxslt/extensions.h>
#  include <libxslt/extra.h>
# endif
#endif

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <ole2.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <rpcproxy.h>
#include <msxml6.h>
#include <msxml6did.h>
#include <wininet.h>
#include <shlwapi.h>
#include <objsafe.h>
#include <olectl.h>
#include <docobj.h>
#include <asptlb.h>
#include <perhist.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/library.h>

#include "msxml_private.h"

#endif /* !_MSXML3_PCH_ */
