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
 * \file xmlpool.h
 * \brief Pool of common options
 * \author Felix Kuehling
 *
 * This file defines macros that can be used to construct
 * driConfigOptions in the drivers. Common options are defined in
 * xmlpool/t_options.h from which xmlpool/options.h is generated with
 * translations. This file defines generic helper macros and includes
 * xmlpool/options.h.
 */

#ifndef __XMLPOOL_H
#define __XMLPOOL_H

/*
 * generic macros
 */

/** \brief Begin __driConfigOptions */
#define DRI_CONF_BEGIN \
"<driinfo>\n"

/** \brief End __driConfigOptions */
#define DRI_CONF_END \
"</driinfo>\n"

/** \brief Begin a section of related options */
#define DRI_CONF_SECTION_BEGIN \
"<section>\n"

/** \brief End a section of related options */
#define DRI_CONF_SECTION_END \
"</section>\n"

/** \brief Begin an option definition */
#define DRI_CONF_OPT_BEGIN(name,type,def) \
"<option name=\""#name"\" type=\""#type"\" default=\""#def"\">\n"

/** \brief Begin an option definition with restrictions on valid values */
#define DRI_CONF_OPT_BEGIN_V(name,type,def,valid) \
"<option name=\""#name"\" type=\""#type"\" default=\""#def"\" valid=\""valid"\">\n"

/** \brief End an option description */
#define DRI_CONF_OPT_END \
"</option>\n"

/** \brief A verbal description in a specified language (empty version) */
#define DRI_CONF_DESC(lang,text) \
"<description lang=\""#lang"\" text=\""text"\"/>\n"

/** \brief A verbal description in a specified language */
#define DRI_CONF_DESC_BEGIN(lang,text) \
"<description lang=\""#lang"\" text=\""text"\">\n"

/** \brief End a description */
#define DRI_CONF_DESC_END \
"</description>\n"

/** \brief A verbal description of an enum value */
#define DRI_CONF_ENUM(value,text) \
"<enum value=\""#value"\" text=\""text"\"/>\n"


/*
 * Predefined option sections and options with multi-lingual descriptions
 * are now automatically generated.
 */
#include "xmlpool/options.h"

#endif
