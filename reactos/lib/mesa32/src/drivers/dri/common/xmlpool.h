/* -*- mode:C; coding: mult-utf-8-unix -*-
 *
 * !!! Important: This file is encoded in UTF-8 !!!
 *
 * Note (Emacs): You need Mule. In Debian the package is called
 * mule-ucs.
 *
 * Note (Emacs): You may have to enable multibyte characters in the
 * Mule customization group or by setting
 * default-enable-multibyte-characters to t in your .emacs:
 */
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
 * This file defines macros that can be used to construct driConfigOptions
 * in the drivers.
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
 * predefined option sections and options with multi-lingual descriptions
 */

/** \brief Debugging options */
#define DRI_CONF_SECTION_DEBUG \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Debugging") \
	DRI_CONF_DESC(de,"Fehlersuche")

#define DRI_CONF_NO_RAST(def) \
DRI_CONF_OPT_BEGIN(no_rast,bool,def) \
        DRI_CONF_DESC(en,"Disable 3D acceleration") \
        DRI_CONF_DESC(de,"3D-Beschleunigung abschalten") \
DRI_CONF_OPT_END

#define DRI_CONF_PERFORMANCE_BOXES(def) \
DRI_CONF_OPT_BEGIN(performance_boxes,bool,def) \
        DRI_CONF_DESC(en,"Show performance boxes") \
        DRI_CONF_DESC(de,"Zeige Performanceboxen") \
DRI_CONF_OPT_END

#define DRI_CONF_DEBUG_DMA(def) \
DRI_CONF_OPT_BEGIN(debug_dma,bool,def) \
	DRI_CONF_DESC(en,"Debug DMA buffers") \
	DRI_CONF_DESC(de,"DMA Puffer debuggen") \
DRI_CONF_OPT_END


/** \brief Texture-related options */
#define DRI_CONF_SECTION_QUALITY \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Image Quality") \
	DRI_CONF_DESC(de,"Bildqualität")

#define DRI_CONF_TEXTURE_DEPTH_FB       0
#define DRI_CONF_TEXTURE_DEPTH_32       1
#define DRI_CONF_TEXTURE_DEPTH_16       2
#define DRI_CONF_TEXTURE_DEPTH_FORCE_16 3
#define DRI_CONF_TEXTURE_DEPTH(def) \
DRI_CONF_OPT_BEGIN_V(texture_depth,enum,def,"0:3") \
	DRI_CONF_DESC_BEGIN(en,"Texture color depth") \
                DRI_CONF_ENUM(0,"Prefer frame buffer color depth") \
                DRI_CONF_ENUM(1,"Prefer 32 bits") \
                DRI_CONF_ENUM(2,"Prefer 16 bits") \
                DRI_CONF_ENUM(3,"Force 16 bits") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Texturfarbtiefe") \
                DRI_CONF_ENUM(0,"Bevorzugt so wie Framebuffer") \
                DRI_CONF_ENUM(1,"Bevorzugt 32 Bits") \
                DRI_CONF_ENUM(2,"Bevorzugt 16 Bits") \
                DRI_CONF_ENUM(3,"Höchstens 16 Bits") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DEF_MAX_ANISOTROPY(def,range) \
DRI_CONF_OPT_BEGIN_V(def_max_anisotropy,float,def,range) \
        DRI_CONF_DESC(en,"Default maximum value for anisotropic texture filtering") \
        DRI_CONF_DESC(de,"Standard Maximalwert für anisotropische Texturfilterung") \
DRI_CONF_OPT_END

#define DRI_CONF_NO_NEG_LOD_BIAS(def) \
DRI_CONF_OPT_BEGIN(no_neg_lod_bias,bool,def) \
        DRI_CONF_DESC(en,"Forbid negative texture LOD bias") \
        DRI_CONF_DESC(de,"Verbiete negativen Textur-LOD-Bias") \
DRI_CONF_OPT_END

#define DRI_CONF_COLOR_REDUCTION_ROUND 0
#define DRI_CONF_COLOR_REDUCTION_DITHER 1
#define DRI_CONF_COLOR_REDUCTION(def) \
DRI_CONF_OPT_BEGIN_V(color_reduction,enum,def,"0:1") \
        DRI_CONF_DESC_BEGIN(en,"Default color reduction method") \
                DRI_CONF_ENUM(0,"Round or truncate") \
                DRI_CONF_ENUM(1,"Dither") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Standardmethode zur Farbreduktion") \
                DRI_CONF_ENUM(0,"Runden oder Abschneiden") \
                DRI_CONF_ENUM(1,"Rastern") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_ROUND_TRUNC 0
#define DRI_CONF_ROUND_ROUND 1
#define DRI_CONF_ROUND_MODE(def) \
DRI_CONF_OPT_BEGIN_V(round_mode,enum,def,"0:1") \
	DRI_CONF_DESC_BEGIN(en,"Round or truncate colors") \
                DRI_CONF_ENUM(0,"Truncate") \
                DRI_CONF_ENUM(1,"Round") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farben runden oder abschneiden") \
                DRI_CONF_ENUM(0,"Abschneiden") \
                DRI_CONF_ENUM(1,"Runden") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DITHER_XERRORDIFF 0
#define DRI_CONF_DITHER_XERRORDIFFRESET 1
#define DRI_CONF_DITHER_ORDERED 2
#define DRI_CONF_DITHER_MODE(def) \
DRI_CONF_OPT_BEGIN_V(dither_mode,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Color dithering") \
                DRI_CONF_ENUM(0,"Horizontal error diffusion") \
                DRI_CONF_ENUM(1,"Horizontal error diffusion, reset error at line start") \
                DRI_CONF_ENUM(2,"Ordered 2D dithering") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farben rastern") \
                DRI_CONF_ENUM(0,"Horizontale Fehlerstreuung") \
                DRI_CONF_ENUM(1,"Horizontale Fehlerstreuung, Fehler am Zeilenanfang zurücksetzen") \
                DRI_CONF_ENUM(2,"Geordnete 2D Farbrasterung") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

/** \brief Performance-related options */
#define DRI_CONF_SECTION_PERFORMANCE \
DRI_CONF_SECTION_BEGIN \
        DRI_CONF_DESC(en,"Performance") \
        DRI_CONF_DESC(de,"Leistung")

#define DRI_CONF_TCL_SW 0
#define DRI_CONF_TCL_PIPELINED 1
#define DRI_CONF_TCL_VTXFMT 2
#define DRI_CONF_TCL_CODEGEN 3
#define DRI_CONF_TCL_MODE(def) \
DRI_CONF_OPT_BEGIN_V(tcl_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"TCL mode (Transformation, Clipping, Lighting)") \
                DRI_CONF_ENUM(0,"Software") \
                DRI_CONF_ENUM(1,"TCL stage in MESA pipeline") \
                DRI_CONF_ENUM(2,"Bypass MESA's pipeline") \
                DRI_CONF_ENUM(3,"Bypass MESA's pipeline with state-based code generation") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"TCL Modus (Transformation, Clipping, Licht)") \
                DRI_CONF_ENUM(0,"Software") \
                DRI_CONF_ENUM(1,"TCL Stufe in MESA Pipeline") \
                DRI_CONF_ENUM(2,"Umgehe MESAs Pipeline") \
                DRI_CONF_ENUM(3,"Umgehe MESAs Pipeline mit zustandsbasierter Codegenerierung") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_FTHROTTLE_BUSY 0
#define DRI_CONF_FTHROTTLE_USLEEPS 1
#define DRI_CONF_FTHROTTLE_IRQS 2
#define DRI_CONF_FTHROTTLE_MODE(def) \
DRI_CONF_OPT_BEGIN_V(fthrottle_mode,enum,def,"0:2") \
        DRI_CONF_DESC_BEGIN(en,"Frame throttling") \
                DRI_CONF_ENUM(0,"Busy waiting") \
                DRI_CONF_ENUM(1,"Usleeps") \
                DRI_CONF_ENUM(2,"Software interrupts") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Framethrottling") \
                DRI_CONF_ENUM(0,"Aktives Warten") \
                DRI_CONF_ENUM(1,"Usleeps") \
                DRI_CONF_ENUM(2,"Software Interrutps") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3
#define DRI_CONF_VBLANK_MODE(def) \
DRI_CONF_OPT_BEGIN_V(vblank_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"Synchronization with vertical refresh (swap intervals)") \
                DRI_CONF_ENUM(0,"Never, FPS rulez!") \
                DRI_CONF_ENUM(1,"Application preference, default interval 0") \
                DRI_CONF_ENUM(2,"Application preference, default interval 1") \
                DRI_CONF_ENUM(3,"Application preference, always synchronize with refresh") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Synchronisation mit dem vertikalen Bildaufbau (swap intervals)") \
                DRI_CONF_ENUM(0,"Niemals, immer die maximale Framerate") \
                DRI_CONF_ENUM(1,"Anwendung entscheidet, Standardinterval 0") \
                DRI_CONF_ENUM(2,"Anwendung entscheidet, Standardinterval 1") \
                DRI_CONF_ENUM(3,"Anwendung entscheidet, immer mit Bildaufbau synchronisieren") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#endif
