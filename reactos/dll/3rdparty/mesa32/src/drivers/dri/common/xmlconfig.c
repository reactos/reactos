/*
 * XML DRI client-side driver configuration
 * Copyright (C) 2003 Felix Kuehling
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * FELIX KUEHLING, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
/**
 * \file xmlconfig.c
 * \brief Driver-independent client-side part of the XML configuration
 * \author Felix Kuehling
 */

#include "glheader.h"

#include <string.h>
#include <assert.h>
#include <expat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "imports.h"
#include "dri_util.h"
#include "xmlconfig.h"

/*
 * OS dependent ways of getting the name of the running program
 */
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#undef GET_PROGRAM_NAME

#if (defined(__GNU_LIBRARY__) || defined(__GLIBC__)) && !defined(__UCLIBC__)
#    if !defined(__GLIBC__) || (__GLIBC__ < 2)
/* These aren't declared in any libc5 header */
extern char *program_invocation_name, *program_invocation_short_name;
#    endif
#    define GET_PROGRAM_NAME() program_invocation_short_name
#elif defined(__FreeBSD__) && (__FreeBSD__ >= 2)
#    include <osreldate.h>
#    if (__FreeBSD_version >= 440000)
#        include <stdlib.h>
#        define GET_PROGRAM_NAME() getprogname()
#    endif
#elif defined(__NetBSD__) && defined(__NetBSD_Version) && (__NetBSD_Version >= 106000100)
#    include <stdlib.h>
#    define GET_PROGRAM_NAME() getprogname()
#endif

#if !defined(GET_PROGRAM_NAME)
#    if defined(OpenBSD) || defined(NetBSD) || defined(__UCLIBC__)
/* This is a hack. It's said to work on OpenBSD, NetBSD and GNU.
 * Rogelio M.Serrano Jr. reported it's also working with UCLIBC. It's
 * used as a last resort, if there is no documented facility available. */
static const char *__getProgramName () {
    extern const char *__progname;
    char * arg = strrchr(__progname, '/');
    if (arg)
        return arg+1;
    else
        return __progname;
}
#        define GET_PROGRAM_NAME() __getProgramName()
#    else
#        define GET_PROGRAM_NAME() ""
#        warning "Per application configuration won't work with your OS version."
#    endif
#endif

/** \brief Find an option in an option cache with the name as key */
static GLuint findOption (const driOptionCache *cache, const char *name) {
    GLuint len = strlen (name);
    GLuint size = 1 << cache->tableSize, mask = size - 1;
    GLuint hash = 0;
    GLuint i, shift;

  /* compute a hash from the variable length name */
    for (i = 0, shift = 0; i < len; ++i, shift = (shift+8) & 31)
	hash += (GLuint)name[i] << shift;
    hash *= hash;
    hash = (hash >> (16-cache->tableSize/2)) & mask;

  /* this is just the starting point of the linear search for the option */
    for (i = 0; i < size; ++i, hash = (hash+1) & mask) {
      /* if we hit an empty entry then the option is not defined (yet) */
	if (cache->info[hash].name == 0)
	    break;
	else if (!strcmp (name, cache->info[hash].name))
	    break;
    }
  /* this assertion fails if the hash table is full */
    assert (i < size);

    return hash;
}

/** \brief Count the real number of options in an option cache */
static GLuint countOptions (const driOptionCache *cache) {
    GLuint size = 1 << cache->tableSize;
    GLuint i, count = 0;
    for (i = 0; i < size; ++i)
	if (cache->info[i].name)
	    count++;
    return count;
}

/** \brief Like strdup but using MALLOC and with error checking. */
#define XSTRDUP(dest,source) do { \
    GLuint len = strlen (source); \
    if (!(dest = MALLOC (len+1))) { \
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__); \
	abort(); \
    } \
    memcpy (dest, source, len+1); \
} while (0)

static int compare (const void *a, const void *b) {
    return strcmp (*(char *const*)a, *(char *const*)b);
}
/** \brief Binary search in a string array. */
static GLuint bsearchStr (const XML_Char *name,
			  const XML_Char *elems[], GLuint count) {
    const XML_Char **found;
    found = bsearch (&name, elems, count, sizeof (XML_Char *), compare);
    if (found)
	return found - elems;
    else
	return count;
}

/** \brief Locale-independent integer parser.
 *
 * Works similar to strtol. Leading space is NOT skipped. The input
 * number may have an optional sign. Radix is specified by base. If
 * base is 0 then decimal is assumed unless the input number is
 * prefixed by 0x or 0X for hexadecimal or 0 for octal. After
 * returning tail points to the first character that is not part of
 * the integer number. If no number was found then tail points to the
 * start of the input string. */
static GLint strToI (const XML_Char *string, const XML_Char **tail, int base) {
    GLint radix = base == 0 ? 10 : base;
    GLint result = 0;
    GLint sign = 1;
    GLboolean numberFound = GL_FALSE;
    const XML_Char *start = string;

    assert (radix >= 2 && radix <= 36);

    if (*string == '-') {
	sign = -1;
	string++;
    } else if (*string == '+')
	string++;
    if (base == 0 && *string == '0') {
	numberFound = GL_TRUE; 
	if (*(string+1) == 'x' || *(string+1) == 'X') {
	    radix = 16;
	    string += 2;
	} else {
	    radix = 8;
	    string++;
	}
    }
    do {
	GLint digit = -1;
	if (radix <= 10) {
	    if (*string >= '0' && *string < '0' + radix)
		digit = *string - '0';
	} else {
	    if (*string >= '0' && *string <= '9')
		digit = *string - '0';
	    else if (*string >= 'a' && *string < 'a' + radix - 10)
		digit = *string - 'a' + 10;
	    else if (*string >= 'A' && *string < 'A' + radix - 10)
		digit = *string - 'A' + 10;
	}
	if (digit != -1) {
	    numberFound = GL_TRUE;
	    result = radix*result + digit;
	    string++;
	} else
	    break;
    } while (GL_TRUE);
    *tail = numberFound ? string : start;
    return sign * result;
}

/** \brief Locale-independent floating-point parser.
 *
 * Works similar to strtod. Leading space is NOT skipped. The input
 * number may have an optional sign. '.' is interpreted as decimal
 * point and may occor at most once. Optionally the number may end in
 * [eE]<exponent>, where <exponent> is an integer as recognized by
 * strToI. In that case the result is number * 10^exponent. After
 * returning tail points to the first character that is not part of
 * the floating point number. If no number was found then tail points
 * to the start of the input string.
 *
 * Uses two passes for maximum accuracy. */
static GLfloat strToF (const XML_Char *string, const XML_Char **tail) {
    GLint nDigits = 0, pointPos, exponent;
    GLfloat sign = 1.0f, result = 0.0f, scale;
    const XML_Char *start = string, *numStart;

    /* sign */
    if (*string == '-') {
	sign = -1.0f;
	string++;
    } else if (*string == '+')
	string++;

    /* first pass: determine position of decimal point, number of
     * digits, exponent and the end of the number. */
    numStart = string;
    while (*string >= '0' && *string <= '9') {
	string++;
	nDigits++;
    }
    pointPos = nDigits;
    if (*string == '.') {
	string++;
	while (*string >= '0' && *string <= '9') {
	    string++;
	    nDigits++;
	}
    }
    if (nDigits == 0) {
	/* no digits, no number */
	*tail = start;
	return 0.0f;
    }
    *tail = string;
    if (*string == 'e' || *string == 'E') {
	const XML_Char *expTail;
	exponent = strToI (string+1, &expTail, 10);
	if (expTail == string+1)
	    exponent = 0;
	else
	    *tail = expTail;
    } else
	exponent = 0;
    string = numStart;

    /* scale of the first digit */
    scale = sign * (GLfloat)pow (10.0, (GLdouble)(pointPos-1 + exponent));

    /* second pass: parse digits */
    do {
	if (*string != '.') {
	    assert (*string >= '0' && *string <= '9');
	    result += scale * (GLfloat)(*string - '0');
	    scale *= 0.1f;
	    nDigits--;
	}
	string++;
    } while (nDigits > 0);

    return result;
}

/** \brief Parse a value of a given type. */
static GLboolean parseValue (driOptionValue *v, driOptionType type,
			     const XML_Char *string) {
    const XML_Char *tail;
  /* skip leading white-space */
    string += strspn (string, " \f\n\r\t\v");
    switch (type) {
      case DRI_BOOL:
	if (!strcmp (string, "false")) {
	    v->_bool = GL_FALSE;
	    tail = string + 5;
	} else if (!strcmp (string, "true")) {
	    v->_bool = GL_TRUE;
	    tail = string + 4;
	}
	else
	    return GL_FALSE;
	break;
      case DRI_ENUM: /* enum is just a special integer */
      case DRI_INT:
	v->_int = strToI (string, &tail, 0);
	break;
      case DRI_FLOAT:
	v->_float = strToF (string, &tail);
	break;
    }

    if (tail == string)
	return GL_FALSE; /* empty string (or containing only white-space) */
  /* skip trailing white space */
    if (*tail)
	tail += strspn (tail, " \f\n\r\t\v");
    if (*tail)
	return GL_FALSE; /* something left over that is not part of value */

    return GL_TRUE;
}

/** \brief Parse a list of ranges of type info->type. */
static GLboolean parseRanges (driOptionInfo *info, const XML_Char *string) {
    XML_Char *cp, *range;
    GLuint nRanges, i;
    driOptionRange *ranges;

    XSTRDUP (cp, string);
  /* pass 1: determine the number of ranges (number of commas + 1) */
    range = cp;
    for (nRanges = 1; *range; ++range)
	if (*range == ',')
	    ++nRanges;

    if ((ranges = MALLOC (nRanges*sizeof(driOptionRange))) == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }

  /* pass 2: parse all ranges into preallocated array */
    range = cp;
    for (i = 0; i < nRanges; ++i) {
	XML_Char *end, *sep;
	assert (range);
	end = strchr (range, ',');
	if (end)
	    *end = '\0';
	sep = strchr (range, ':');
	if (sep) { /* non-empty interval */
	    *sep = '\0';
	    if (!parseValue (&ranges[i].start, info->type, range) ||
		!parseValue (&ranges[i].end, info->type, sep+1))
	        break;
	    if (info->type == DRI_INT &&
		ranges[i].start._int > ranges[i].end._int)
		break;
	    if (info->type == DRI_FLOAT &&
		ranges[i].start._float > ranges[i].end._float)
		break;
	} else { /* empty interval */
	    if (!parseValue (&ranges[i].start, info->type, range))
		break;
	    ranges[i].end = ranges[i].start;
	}
	if (end)
	    range = end+1;
	else
	    range = NULL;
    }
    FREE (cp);
    if (i < nRanges) {
	FREE (ranges);
	return GL_FALSE;
    } else
	assert (range == NULL);

    info->nRanges = nRanges;
    info->ranges = ranges;
    return GL_TRUE;
}

/** \brief Check if a value is in one of info->ranges. */
static GLboolean checkValue (const driOptionValue *v, const driOptionInfo *info) {
    GLuint i;
    assert (info->type != DRI_BOOL); /* should be caught by the parser */
    if (info->nRanges == 0)
	return GL_TRUE;
    switch (info->type) {
      case DRI_ENUM: /* enum is just a special integer */
      case DRI_INT:
	for (i = 0; i < info->nRanges; ++i)
	    if (v->_int >= info->ranges[i].start._int &&
		v->_int <= info->ranges[i].end._int)
		return GL_TRUE;
	break;
      case DRI_FLOAT:
	for (i = 0; i < info->nRanges; ++i)
	    if (v->_float >= info->ranges[i].start._float &&
		v->_float <= info->ranges[i].end._float)
		return GL_TRUE;
	break;
      default:
	assert (0); /* should never happen */
    }
    return GL_FALSE;
}

/** \brief Output a warning message. */
#define XML_WARNING1(msg) do {\
    __driUtilMessage ("Warning in %s line %d, column %d: "msg, data->name, \
                      XML_GetCurrentLineNumber(data->parser), \
                      XML_GetCurrentColumnNumber(data->parser)); \
} while (0)
#define XML_WARNING(msg,args...) do { \
    __driUtilMessage ("Warning in %s line %d, column %d: "msg, data->name, \
                      XML_GetCurrentLineNumber(data->parser), \
                      XML_GetCurrentColumnNumber(data->parser), \
                      args); \
} while (0)
/** \brief Output an error message. */
#define XML_ERROR1(msg) do { \
    __driUtilMessage ("Error in %s line %d, column %d: "msg, data->name, \
                      XML_GetCurrentLineNumber(data->parser), \
                      XML_GetCurrentColumnNumber(data->parser)); \
} while (0)
#define XML_ERROR(msg,args...) do { \
    __driUtilMessage ("Error in %s line %d, column %d: "msg, data->name, \
                      XML_GetCurrentLineNumber(data->parser), \
                      XML_GetCurrentColumnNumber(data->parser), \
                      args); \
} while (0)
/** \brief Output a fatal error message and abort. */
#define XML_FATAL1(msg) do { \
    fprintf (stderr, "Fatal error in %s line %d, column %d: "msg"\n", \
             data->name, \
             XML_GetCurrentLineNumber(data->parser), \
             XML_GetCurrentColumnNumber(data->parser)); \
    abort();\
} while (0)
#define XML_FATAL(msg,args...) do { \
    fprintf (stderr, "Fatal error in %s line %d, column %d: "msg"\n", \
             data->name, \
             XML_GetCurrentLineNumber(data->parser), \
             XML_GetCurrentColumnNumber(data->parser), \
             args); \
    abort();\
} while (0)

/** \brief Parser context for __driConfigOptions. */
struct OptInfoData {
    const char *name;
    XML_Parser parser;
    driOptionCache *cache;
    GLboolean inDriInfo;
    GLboolean inSection;
    GLboolean inDesc;
    GLboolean inOption;
    GLboolean inEnum;
    int curOption;
};

/** \brief Elements in __driConfigOptions. */
enum OptInfoElem {
    OI_DESCRIPTION = 0, OI_DRIINFO, OI_ENUM, OI_OPTION, OI_SECTION, OI_COUNT
};
static const XML_Char *OptInfoElems[] = {
    "description", "driinfo", "enum", "option", "section"
};

/** \brief Parse attributes of an enum element.
 *
 * We're not actually interested in the data. Just make sure this is ok
 * for external configuration tools.
 */
static void parseEnumAttr (struct OptInfoData *data, const XML_Char **attr) {
    GLuint i;
    const XML_Char *value = NULL, *text = NULL;
    driOptionValue v;
    GLuint opt = data->curOption;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "value")) value = attr[i+1];
	else if (!strcmp (attr[i], "text")) text = attr[i+1];
	else XML_FATAL("illegal enum attribute: %s.", attr[i]);
    }
    if (!value) XML_FATAL1 ("value attribute missing in enum.");
    if (!text) XML_FATAL1 ("text attribute missing in enum.");
     if (!parseValue (&v, data->cache->info[opt].type, value))
	XML_FATAL ("illegal enum value: %s.", value);
    if (!checkValue (&v, &data->cache->info[opt]))
	XML_FATAL ("enum value out of valid range: %s.", value);
}

/** \brief Parse attributes of a description element.
 *
 * We're not actually interested in the data. Just make sure this is ok
 * for external configuration tools.
 */
static void parseDescAttr (struct OptInfoData *data, const XML_Char **attr) {
    GLuint i;
    const XML_Char *lang = NULL, *text = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "lang")) lang = attr[i+1];
	else if (!strcmp (attr[i], "text")) text = attr[i+1];
	else XML_FATAL("illegal description attribute: %s.", attr[i]);
    }
    if (!lang) XML_FATAL1 ("lang attribute missing in description.");
    if (!text) XML_FATAL1 ("text attribute missing in description.");
}

/** \brief Parse attributes of an option element. */
static void parseOptInfoAttr (struct OptInfoData *data, const XML_Char **attr) {
    enum OptAttr {OA_DEFAULT = 0, OA_NAME, OA_TYPE, OA_VALID, OA_COUNT};
    static const XML_Char *optAttr[] = {"default", "name", "type", "valid"};
    const XML_Char *attrVal[OA_COUNT] = {NULL, NULL, NULL, NULL};
    const char *defaultVal;
    driOptionCache *cache = data->cache;
    GLuint opt, i;
    for (i = 0; attr[i]; i += 2) {
	GLuint attrName = bsearchStr (attr[i], optAttr, OA_COUNT);
	if (attrName >= OA_COUNT)
	    XML_FATAL ("illegal option attribute: %s", attr[i]);
	attrVal[attrName] = attr[i+1];
    }
    if (!attrVal[OA_NAME]) XML_FATAL1 ("name attribute missing in option.");
    if (!attrVal[OA_TYPE]) XML_FATAL1 ("type attribute missing in option.");
    if (!attrVal[OA_DEFAULT]) XML_FATAL1 ("default attribute missing in option.");

    opt = findOption (cache, attrVal[OA_NAME]);
    if (cache->info[opt].name)
	XML_FATAL ("option %s redefined.", attrVal[OA_NAME]);
    data->curOption = opt;

    XSTRDUP (cache->info[opt].name, attrVal[OA_NAME]);

    if (!strcmp (attrVal[OA_TYPE], "bool"))
	cache->info[opt].type = DRI_BOOL;
    else if (!strcmp (attrVal[OA_TYPE], "enum"))
	cache->info[opt].type = DRI_ENUM;
    else if (!strcmp (attrVal[OA_TYPE], "int"))
	cache->info[opt].type = DRI_INT;
    else if (!strcmp (attrVal[OA_TYPE], "float"))
	cache->info[opt].type = DRI_FLOAT;
    else
	XML_FATAL ("illegal type in option: %s.", attrVal[OA_TYPE]);

    defaultVal = getenv (cache->info[opt].name);
    if (defaultVal != NULL) {
      /* don't use XML_WARNING, we want the user to see this! */
	fprintf (stderr,
		 "ATTENTION: default value of option %s overridden by environment.\n",
		 cache->info[opt].name);
    } else
	defaultVal = attrVal[OA_DEFAULT];
    if (!parseValue (&cache->values[opt], cache->info[opt].type, defaultVal))
	XML_FATAL ("illegal default value: %s.", defaultVal);

    if (attrVal[OA_VALID]) {
	if (cache->info[opt].type == DRI_BOOL)
	    XML_FATAL1 ("boolean option with valid attribute.");
	if (!parseRanges (&cache->info[opt], attrVal[OA_VALID]))
	    XML_FATAL ("illegal valid attribute: %s.", attrVal[OA_VALID]);
	if (!checkValue (&cache->values[opt], &cache->info[opt]))
	    XML_FATAL ("default value out of valid range '%s': %s.",
		       attrVal[OA_VALID], defaultVal);
    } else if (cache->info[opt].type == DRI_ENUM) {
	XML_FATAL1 ("valid attribute missing in option (mandatory for enums).");
    } else {
	cache->info[opt].nRanges = 0;
	cache->info[opt].ranges = NULL;
    }
}

/** \brief Handler for start element events. */
static void optInfoStartElem (void *userData, const XML_Char *name,
			      const XML_Char **attr) {
    struct OptInfoData *data = (struct OptInfoData *)userData;
    enum OptInfoElem elem = bsearchStr (name, OptInfoElems, OI_COUNT);
    switch (elem) {
      case OI_DRIINFO:
	if (data->inDriInfo)
	    XML_FATAL1 ("nested <driinfo> elements.");
	if (attr[0])
	    XML_FATAL1 ("attributes specified on <driinfo> element.");
	data->inDriInfo = GL_TRUE;
	break;
      case OI_SECTION:
	if (!data->inDriInfo)
	    XML_FATAL1 ("<section> must be inside <driinfo>.");
	if (data->inSection)
	    XML_FATAL1 ("nested <section> elements.");
	if (attr[0])
	    XML_FATAL1 ("attributes specified on <section> element.");
	data->inSection = GL_TRUE;
	break;
      case OI_DESCRIPTION:
	if (!data->inSection && !data->inOption)
	    XML_FATAL1 ("<description> must be inside <description> or <option.");
	if (data->inDesc)
	    XML_FATAL1 ("nested <description> elements.");
	data->inDesc = GL_TRUE;
	parseDescAttr (data, attr);
	break;
      case OI_OPTION:
	if (!data->inSection)
	    XML_FATAL1 ("<option> must be inside <section>.");
	if (data->inDesc)
	    XML_FATAL1 ("<option> nested in <description> element.");
	if (data->inOption)
	    XML_FATAL1 ("nested <option> elements.");
	data->inOption = GL_TRUE;
	parseOptInfoAttr (data, attr);
	break;
      case OI_ENUM:
	if (!(data->inOption && data->inDesc))
	    XML_FATAL1 ("<enum> must be inside <option> and <description>.");
	if (data->inEnum)
	    XML_FATAL1 ("nested <enum> elements.");
	data->inEnum = GL_TRUE;
	parseEnumAttr (data, attr);
	break;
      default:
	XML_FATAL ("unknown element: %s.", name);
    }
}

/** \brief Handler for end element events. */
static void optInfoEndElem (void *userData, const XML_Char *name) {
    struct OptInfoData *data = (struct OptInfoData *)userData;
    enum OptInfoElem elem = bsearchStr (name, OptInfoElems, OI_COUNT);
    switch (elem) {
      case OI_DRIINFO:
	data->inDriInfo = GL_FALSE;
	break;
      case OI_SECTION:
	data->inSection = GL_FALSE;
	break;
      case OI_DESCRIPTION:
	data->inDesc = GL_FALSE;
	break;
      case OI_OPTION:
	data->inOption = GL_FALSE;
	break;
      case OI_ENUM:
	data->inEnum = GL_FALSE;
	break;
      default:
	assert (0); /* should have been caught by StartElem */
    }
}

void driParseOptionInfo (driOptionCache *info,
			 const char *configOptions, GLuint nConfigOptions) {
    XML_Parser p;
    int status;
    struct OptInfoData userData;
    struct OptInfoData *data = &userData;
    GLuint realNoptions;

  /* determine hash table size and allocate memory:
   * 3/2 of the number of options, rounded up, so there remains always
   * at least one free entry. This is needed for detecting undefined
   * options in configuration files without getting a hash table overflow.
   * Round this up to a power of two. */
    GLuint minSize = (nConfigOptions*3 + 1) / 2;
    GLuint size, log2size;
    for (size = 1, log2size = 0; size < minSize; size <<= 1, ++log2size);
    info->tableSize = log2size;
    info->info = CALLOC (size * sizeof (driOptionInfo));
    info->values = CALLOC (size * sizeof (driOptionValue));
    if (info->info == NULL || info->values == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }

    p = XML_ParserCreate ("UTF-8"); /* always UTF-8 */
    XML_SetElementHandler (p, optInfoStartElem, optInfoEndElem);
    XML_SetUserData (p, data);

    userData.name = "__driConfigOptions";
    userData.parser = p;
    userData.cache = info;
    userData.inDriInfo = GL_FALSE;
    userData.inSection = GL_FALSE;
    userData.inDesc = GL_FALSE;
    userData.inOption = GL_FALSE;
    userData.inEnum = GL_FALSE;
    userData.curOption = -1;

    status = XML_Parse (p, configOptions, strlen (configOptions), 1);
    if (!status)
	XML_FATAL ("%s.", XML_ErrorString(XML_GetErrorCode(p)));

    XML_ParserFree (p);

  /* Check if the actual number of options matches nConfigOptions.
   * A mismatch is not fatal (a hash table overflow would be) but we
   * want the driver developer's attention anyway. */
    realNoptions = countOptions (info);
    if (realNoptions != nConfigOptions) {
	fprintf (stderr,
		 "Error: nConfigOptions (%u) does not match the actual number of options in\n"
		 "       __driConfigOptions (%u).\n",
		 nConfigOptions, realNoptions);
    }
}

/** \brief Parser context for configuration files. */
struct OptConfData {
    const char *name;
    XML_Parser parser;
    driOptionCache *cache;
    GLint screenNum;
    const char *driverName, *execName;
    GLuint ignoringDevice;
    GLuint ignoringApp;
    GLuint inDriConf;
    GLuint inDevice;
    GLuint inApp;
    GLuint inOption;
};

/** \brief Elements in configuration files. */
enum OptConfElem {
    OC_APPLICATION = 0, OC_DEVICE, OC_DRICONF, OC_OPTION, OC_COUNT
};
static const XML_Char *OptConfElems[] = {
    "application", "device", "driconf", "option"
};

/** \brief Parse attributes of a device element. */
static void parseDeviceAttr (struct OptConfData *data, const XML_Char **attr) {
    GLuint i;
    const XML_Char *driver = NULL, *screen = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "driver")) driver = attr[i+1];
	else if (!strcmp (attr[i], "screen")) screen = attr[i+1];
	else XML_WARNING("unkown device attribute: %s.", attr[i]);
    }
    if (driver && strcmp (driver, data->driverName))
	data->ignoringDevice = data->inDevice;
    else if (screen) {
	driOptionValue screenNum;
	if (!parseValue (&screenNum, DRI_INT, screen))
	    XML_WARNING("illegal screen number: %s.", screen);
	else if (screenNum._int != data->screenNum)
	    data->ignoringDevice = data->inDevice;
    }
}

/** \brief Parse attributes of an application element. */
static void parseAppAttr (struct OptConfData *data, const XML_Char **attr) {
    GLuint i;
    const XML_Char *name = NULL, *exec = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "name")) name = attr[i+1];
	else if (!strcmp (attr[i], "executable")) exec = attr[i+1];
	else XML_WARNING("unkown application attribute: %s.", attr[i]);
    }
    if (exec && strcmp (exec, data->execName))
	data->ignoringApp = data->inApp;
}

/** \brief Parse attributes of an option element. */
static void parseOptConfAttr (struct OptConfData *data, const XML_Char **attr) {
    GLuint i;
    const XML_Char *name = NULL, *value = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "name")) name = attr[i+1];
	else if (!strcmp (attr[i], "value")) value = attr[i+1];
	else XML_WARNING("unkown option attribute: %s.", attr[i]);
    }
    if (!name) XML_WARNING1 ("name attribute missing in option.");
    if (!value) XML_WARNING1 ("value attribute missing in option.");
    if (name && value) {
	driOptionCache *cache = data->cache;
	GLuint opt = findOption (cache, name);
	if (cache->info[opt].name == NULL)
	    XML_WARNING ("undefined option: %s.", name);
	else if (getenv (cache->info[opt].name))
	  /* don't use XML_WARNING, we want the user to see this! */
	    fprintf (stderr, "ATTENTION: option value of option %s ignored.\n",
		     cache->info[opt].name);
	else if (!parseValue (&cache->values[opt], cache->info[opt].type, value))
	    XML_WARNING ("illegal option value: %s.", value);
    }
}

/** \brief Handler for start element events. */
static void optConfStartElem (void *userData, const XML_Char *name,
			      const XML_Char **attr) {
    struct OptConfData *data = (struct OptConfData *)userData;
    enum OptConfElem elem = bsearchStr (name, OptConfElems, OC_COUNT);
    switch (elem) {
      case OC_DRICONF:
	if (data->inDriConf)
	    XML_WARNING1 ("nested <driconf> elements.");
	if (attr[0])
	    XML_WARNING1 ("attributes specified on <driconf> element.");
	data->inDriConf++;
	break;
      case OC_DEVICE:
	if (!data->inDriConf)
	    XML_WARNING1 ("<device> should be inside <driconf>.");
	if (data->inDevice)
	    XML_WARNING1 ("nested <device> elements.");
	data->inDevice++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseDeviceAttr (data, attr);
	break;
      case OC_APPLICATION:
	if (!data->inDevice)
	    XML_WARNING1 ("<application> should be inside <device>.");
	if (data->inApp)
	    XML_WARNING1 ("nested <application> elements.");
	data->inApp++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseAppAttr (data, attr);
	break;
      case OC_OPTION:
	if (!data->inApp)
	    XML_WARNING1 ("<option> should be inside <application>.");
	if (data->inOption)
	    XML_WARNING1 ("nested <option> elements.");
	data->inOption++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseOptConfAttr (data, attr);
	break;
      default:
	XML_WARNING ("unknown element: %s.", name);
    }
}

/** \brief Handler for end element events. */
static void optConfEndElem (void *userData, const XML_Char *name) {
    struct OptConfData *data = (struct OptConfData *)userData;
    enum OptConfElem elem = bsearchStr (name, OptConfElems, OC_COUNT);
    switch (elem) {
      case OC_DRICONF:
	data->inDriConf--;
	break;
      case OC_DEVICE:
	if (data->inDevice-- == data->ignoringDevice)
	    data->ignoringDevice = 0;
	break;
      case OC_APPLICATION:
	if (data->inApp-- == data->ignoringApp)
	    data->ignoringApp = 0;
	break;
      case OC_OPTION:
	data->inOption--;
	break;
      default:
	/* unknown element, warning was produced on start tag */;
    }
}

/** \brief Initialize an option cache based on info */
static void initOptionCache (driOptionCache *cache, const driOptionCache *info) {
    cache->info = info->info;
    cache->tableSize = info->tableSize;
    cache->values = MALLOC ((1<<info->tableSize) * sizeof (driOptionValue));
    if (cache->values == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }
    memcpy (cache->values, info->values,
	    (1<<info->tableSize) * sizeof (driOptionValue));
}

/** \brief Parse the named configuration file */
static void parseOneConfigFile (XML_Parser p) {
#define BUF_SIZE 0x1000
    struct OptConfData *data = (struct OptConfData *)XML_GetUserData (p);
    int status;
    int fd;

    if ((fd = open (data->name, O_RDONLY)) == -1) {
	__driUtilMessage ("Can't open configuration file %s: %s.",
			  data->name, strerror (errno));
	return;
    }

    while (1) {
	int bytesRead;
	void *buffer = XML_GetBuffer (p, BUF_SIZE);
	if (!buffer) {
	    __driUtilMessage ("Can't allocate parser buffer.");
	    break;
	}
	bytesRead = read (fd, buffer, BUF_SIZE);
	if (bytesRead == -1) {
	    __driUtilMessage ("Error reading from configuration file %s: %s.",
			      data->name, strerror (errno));
	    break;
	}
	status = XML_ParseBuffer (p, bytesRead, bytesRead == 0);
	if (!status) {
	    XML_ERROR ("%s.", XML_ErrorString(XML_GetErrorCode(p)));
	    break;
	}
	if (bytesRead == 0)
	    break;
    }

    close (fd);
#undef BUF_SIZE
}

void driParseConfigFiles (driOptionCache *cache, const driOptionCache *info,
			  GLint screenNum, const char *driverName) {
    char *filenames[2] = {"/etc/drirc", NULL};
    char *home;
    GLuint i;
    struct OptConfData userData;

    initOptionCache (cache, info);

    userData.cache = cache;
    userData.screenNum = screenNum;
    userData.driverName = driverName;
    userData.execName = GET_PROGRAM_NAME();

    if ((home = getenv ("HOME"))) {
	GLuint len = strlen (home);
	filenames[1] = MALLOC (len + 7+1);
	if (filenames[1] == NULL)
	    __driUtilMessage ("Can't allocate memory for %s/.drirc.", home);
	else {
	    memcpy (filenames[1], home, len);
	    memcpy (filenames[1] + len, "/.drirc", 7+1);
	}
    }

    for (i = 0; i < 2; ++i) {
	XML_Parser p;
	if (filenames[i] == NULL)
	    continue;

	p = XML_ParserCreate (NULL); /* use encoding specified by file */
	XML_SetElementHandler (p, optConfStartElem, optConfEndElem);
	XML_SetUserData (p, &userData);
	userData.parser = p;
	userData.name = filenames[i];
	userData.ignoringDevice = 0;
	userData.ignoringApp = 0;
	userData.inDriConf = 0;
	userData.inDevice = 0;
	userData.inApp = 0;
	userData.inOption = 0;

	parseOneConfigFile (p);
	XML_ParserFree (p);
    }

    if (filenames[1])
	FREE (filenames[1]);
}

void driDestroyOptionInfo (driOptionCache *info) {
    driDestroyOptionCache (info);
    if (info->info) {
	GLuint i, size = 1 << info->tableSize;
	for (i = 0; i < size; ++i) {
	    if (info->info[i].name) {
		FREE (info->info[i].name);
		if (info->info[i].ranges)
		    FREE (info->info[i].ranges);
	    }
	}
	FREE (info->info);
    }
}

void driDestroyOptionCache (driOptionCache *cache) {
    if (cache->values)
	FREE (cache->values);
}

GLboolean driCheckOption (const driOptionCache *cache, const char *name,
			  driOptionType type) {
    GLuint i = findOption (cache, name);
    return cache->info[i].name != NULL && cache->info[i].type == type;
}

GLboolean driQueryOptionb (const driOptionCache *cache, const char *name) {
    GLuint i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_BOOL);
    return cache->values[i]._bool;
}

GLint driQueryOptioni (const driOptionCache *cache, const char *name) {
    GLuint i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_INT || cache->info[i].type == DRI_ENUM);
    return cache->values[i]._int;
}

GLfloat driQueryOptionf (const driOptionCache *cache, const char *name) {
    GLuint i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_FLOAT);
    return cache->values[i]._float;
}
