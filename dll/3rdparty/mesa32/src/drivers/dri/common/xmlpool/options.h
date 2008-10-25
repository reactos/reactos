/***********************************************************************
 ***        THIS FILE IS GENERATED AUTOMATICALLY. DON'T EDIT!        ***
 ***********************************************************************/
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
 * \file t_options.h
 * \brief Templates of common options
 * \author Felix Kuehling
 *
 * This file defines macros for common options that can be used to
 * construct driConfigOptions in the drivers. This file is only a
 * template containing English descriptions for options wrapped in
 * gettext(). xgettext can be used to extract translatable
 * strings. These strings can then be translated by anyone familiar
 * with GNU gettext. gen_xmlpool.py takes this template and fills in
 * all the translations. The result (options.h) is included by
 * xmlpool.h which in turn can be included by drivers.
 *
 * The macros used to describe otions in this file are defined in
 * ../xmlpool.h.
 */

/* This is needed for xgettext to extract translatable strings.
 * gen_xmlpool.py will discard this line. */
/* #include <libintl.h>
 * commented out by gen_xmlpool.py */

/*
 * predefined option sections and options with multi-lingual descriptions
 */

/** \brief Debugging options */
#define DRI_CONF_SECTION_DEBUG \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Debugging") \
	DRI_CONF_DESC(de,"Fehlersuche") \
	DRI_CONF_DESC(es,"Depurando") \
	DRI_CONF_DESC(nl,"Debuggen") \
	DRI_CONF_DESC(fr,"Debogage") \
	DRI_CONF_DESC(sv,"Felsökning")

#define DRI_CONF_NO_RAST(def) \
DRI_CONF_OPT_BEGIN(no_rast,bool,def) \
        DRI_CONF_DESC(en,"Disable 3D acceleration") \
        DRI_CONF_DESC(de,"3D-Beschleunigung abschalten") \
        DRI_CONF_DESC(es,"Desactivar aceleración 3D") \
        DRI_CONF_DESC(nl,"3D versnelling uitschakelen") \
        DRI_CONF_DESC(fr,"Désactiver l'accélération 3D") \
        DRI_CONF_DESC(sv,"Inaktivera 3D-accelerering") \
DRI_CONF_OPT_END

#define DRI_CONF_PERFORMANCE_BOXES(def) \
DRI_CONF_OPT_BEGIN(performance_boxes,bool,def) \
        DRI_CONF_DESC(en,"Show performance boxes") \
        DRI_CONF_DESC(de,"Zeige Performanceboxen") \
        DRI_CONF_DESC(es,"Mostrar cajas de rendimiento") \
        DRI_CONF_DESC(nl,"Laat prestatie boxjes zien") \
        DRI_CONF_DESC(fr,"Afficher les boîtes de performance") \
        DRI_CONF_DESC(sv,"Visa prestandarutor") \
DRI_CONF_OPT_END


/** \brief Texture-related options */
#define DRI_CONF_SECTION_QUALITY \
DRI_CONF_SECTION_BEGIN \
	DRI_CONF_DESC(en,"Image Quality") \
	DRI_CONF_DESC(de,"Bildqualität") \
	DRI_CONF_DESC(es,"Calidad de imagen") \
	DRI_CONF_DESC(nl,"Beeldkwaliteit") \
	DRI_CONF_DESC(fr,"Qualité d'image") \
	DRI_CONF_DESC(sv,"Bildkvalitet")

#define DRI_CONF_EXCESS_MIPMAP(def) \
DRI_CONF_OPT_BEGIN(excess_mipmap,bool,def) \
	DRI_CONF_DESC(en,"Enable extra mipmap level") \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_DEPTH_FB       0
#define DRI_CONF_TEXTURE_DEPTH_32       1
#define DRI_CONF_TEXTURE_DEPTH_16       2
#define DRI_CONF_TEXTURE_DEPTH_FORCE_16 3
#define DRI_CONF_TEXTURE_DEPTH(def) \
DRI_CONF_OPT_BEGIN_V(texture_depth,enum,def,"0:3") \
	DRI_CONF_DESC_BEGIN(en,"Texture color depth") \
                DRI_CONF_ENUM(0,"Prefer frame buffer color depth") \
                DRI_CONF_ENUM(1,"Prefer 32 bits per texel") \
                DRI_CONF_ENUM(2,"Prefer 16 bits per texel") \
                DRI_CONF_ENUM(3,"Force 16 bits per texel") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Texturfarbtiefe") \
                DRI_CONF_ENUM(0,"Bevorzuge Farbtiefe des Framebuffers") \
                DRI_CONF_ENUM(1,"Bevorzuge 32 bits pro Texel") \
                DRI_CONF_ENUM(2,"Bevorzuge 16 bits pro Texel") \
                DRI_CONF_ENUM(3,"Erzwinge 16 bits pro Texel") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(es,"Profundidad de color de textura") \
                DRI_CONF_ENUM(0,"Preferir profundidad de color del ”framebuffer“") \
                DRI_CONF_ENUM(1,"Preferir 32 bits por texel") \
                DRI_CONF_ENUM(2,"Preferir 16 bits por texel") \
                DRI_CONF_ENUM(3,"Forzar a 16 bits por texel") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(nl,"Textuurkleurendiepte") \
                DRI_CONF_ENUM(0,"Prefereer kaderbufferkleurdiepte") \
                DRI_CONF_ENUM(1,"Prefereer 32 bits per texel") \
                DRI_CONF_ENUM(2,"Prefereer 16 bits per texel") \
                DRI_CONF_ENUM(3,"Dwing 16 bits per texel af") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(fr,"Profondeur de texture") \
                DRI_CONF_ENUM(0,"Profondeur de couleur") \
                DRI_CONF_ENUM(1,"Préférer 32 bits par texel") \
                DRI_CONF_ENUM(2,"Prérérer 16 bits par texel") \
                DRI_CONF_ENUM(3,"Forcer 16 bits par texel") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(sv,"Färgdjup för texturer") \
                DRI_CONF_ENUM(0,"Föredra färgdjupet för framebuffer") \
                DRI_CONF_ENUM(1,"Föredra 32 bitar per texel") \
                DRI_CONF_ENUM(2,"Föredra 16 bitar per texel") \
                DRI_CONF_ENUM(3,"Tvinga 16 bitar per texel") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DEF_MAX_ANISOTROPY(def,range) \
DRI_CONF_OPT_BEGIN_V(def_max_anisotropy,float,def,range) \
        DRI_CONF_DESC(en,"Initial maximum value for anisotropic texture filtering") \
        DRI_CONF_DESC(de,"Initialer Maximalwert für anisotropische Texturfilterung") \
        DRI_CONF_DESC(es,"Valor máximo inicial para filtrado anisotrópico de textura") \
        DRI_CONF_DESC(nl,"Initïele maximum waarde voor anisotrophische textuur filtering") \
        DRI_CONF_DESC(fr,"Valeur maximale initiale pour le filtrage anisotropique de texture") \
        DRI_CONF_DESC(sv,"Initialt maximalt värde för anisotropisk texturfiltrering") \
DRI_CONF_OPT_END

#define DRI_CONF_NO_NEG_LOD_BIAS(def) \
DRI_CONF_OPT_BEGIN(no_neg_lod_bias,bool,def) \
        DRI_CONF_DESC(en,"Forbid negative texture LOD bias") \
        DRI_CONF_DESC(de,"Verbiete negative Textur-Detailgradverschiebung") \
        DRI_CONF_DESC(es,"Prohibir valores negativos de Nivel De Detalle (LOD) de texturas") \
        DRI_CONF_DESC(nl,"Verbied negatief niveau detailonderscheid (LOD) van texturen") \
        DRI_CONF_DESC(fr,"Interdire le LOD bias negatif") \
        DRI_CONF_DESC(sv,"Förbjud negativ LOD-kompensation för texturer") \
DRI_CONF_OPT_END

#define DRI_CONF_FORCE_S3TC_ENABLE(def) \
DRI_CONF_OPT_BEGIN(force_s3tc_enable,bool,def) \
        DRI_CONF_DESC(en,"Enable S3TC texture compression even if software support is not available") \
        DRI_CONF_DESC(de,"Aktiviere S3TC Texturkomprimierung auch wenn die nötige Softwareunterstützung fehlt") \
        DRI_CONF_DESC(es,"Activar la compresión de texturas S3TC incluso si el soporte por software no está disponible") \
        DRI_CONF_DESC(nl,"Schakel S3TC textuurcompressie in, zelfs als softwareondersteuning niet aanwezig is") \
        DRI_CONF_DESC(fr,"Activer la compression de texture S3TC même si le support logiciel est absent") \
        DRI_CONF_DESC(sv,"Aktivera S3TC-texturkomprimering även om programvarustöd saknas") \
DRI_CONF_OPT_END

#define DRI_CONF_COLOR_REDUCTION_ROUND 0
#define DRI_CONF_COLOR_REDUCTION_DITHER 1
#define DRI_CONF_COLOR_REDUCTION(def) \
DRI_CONF_OPT_BEGIN_V(color_reduction,enum,def,"0:1") \
        DRI_CONF_DESC_BEGIN(en,"Initial color reduction method") \
                DRI_CONF_ENUM(0,"Round colors") \
                DRI_CONF_ENUM(1,"Dither colors") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Initiale Farbreduktionsmethode") \
                DRI_CONF_ENUM(0,"Farben runden") \
                DRI_CONF_ENUM(1,"Farben rastern") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(es,"Método inicial de reducción de color") \
                DRI_CONF_ENUM(0,"Colores redondeados") \
                DRI_CONF_ENUM(1,"Colores suavizados") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(nl,"Initïele kleurreductie methode") \
                DRI_CONF_ENUM(0,"Rond kleuren af") \
                DRI_CONF_ENUM(1,"Rasteriseer kleuren") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(fr,"Technique de réduction de couleurs") \
                DRI_CONF_ENUM(0,"Arrondir les valeurs de couleur") \
                DRI_CONF_ENUM(1,"Tramer les couleurs") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(sv,"Initial färgminskningsmetod") \
                DRI_CONF_ENUM(0,"Avrunda färger") \
                DRI_CONF_ENUM(1,"Utjämna färger") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_ROUND_TRUNC 0
#define DRI_CONF_ROUND_ROUND 1
#define DRI_CONF_ROUND_MODE(def) \
DRI_CONF_OPT_BEGIN_V(round_mode,enum,def,"0:1") \
	DRI_CONF_DESC_BEGIN(en,"Color rounding method") \
                DRI_CONF_ENUM(0,"Round color components downward") \
                DRI_CONF_ENUM(1,"Round to nearest color") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farbrundungsmethode") \
                DRI_CONF_ENUM(0,"Farbkomponenten abrunden") \
                DRI_CONF_ENUM(1,"Zur ähnlichsten Farbe runden") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(es,"Método de redondeo de colores") \
                DRI_CONF_ENUM(0,"Redondear hacia abajo los componentes de color") \
                DRI_CONF_ENUM(1,"Redondear al color más cercano") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(nl,"Kleurafrondingmethode") \
                DRI_CONF_ENUM(0,"Rond kleurencomponenten af naar beneden") \
                DRI_CONF_ENUM(1,"Rond af naar dichtsbijzijnde kleur") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(fr,"Méthode d'arrondi des couleurs") \
                DRI_CONF_ENUM(0,"Arrondi à l'inférieur") \
                DRI_CONF_ENUM(1,"Arrondi au plus proche") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(sv,"Färgavrundningsmetod") \
                DRI_CONF_ENUM(0,"Avrunda färdkomponenter nedåt") \
                DRI_CONF_ENUM(1,"Avrunda till närmsta färg") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_DITHER_XERRORDIFF 0
#define DRI_CONF_DITHER_XERRORDIFFRESET 1
#define DRI_CONF_DITHER_ORDERED 2
#define DRI_CONF_DITHER_MODE(def) \
DRI_CONF_OPT_BEGIN_V(dither_mode,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Color dithering method") \
                DRI_CONF_ENUM(0,"Horizontal error diffusion") \
                DRI_CONF_ENUM(1,"Horizontal error diffusion, reset error at line start") \
                DRI_CONF_ENUM(2,"Ordered 2D color dithering") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Farbrasterungsmethode") \
                DRI_CONF_ENUM(0,"Horizontale Fehlerstreuung") \
                DRI_CONF_ENUM(1,"Horizontale Fehlerstreuung, Fehler am Zeilenanfang zurücksetzen") \
                DRI_CONF_ENUM(2,"Geordnete 2D Farbrasterung") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(es,"Método de suavizado de color") \
                DRI_CONF_ENUM(0,"Difusión de error horizontal") \
                DRI_CONF_ENUM(1,"Difusión de error horizontal, reiniciar error al comienzo de línea") \
                DRI_CONF_ENUM(2,"Suavizado de color 2D ordenado") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(nl,"Kleurrasteriseringsmethode") \
                DRI_CONF_ENUM(0,"Horizontale foutdiffusie") \
                DRI_CONF_ENUM(1,"Horizontale foutdiffusie, zet fout bij lijnbegin terug") \
                DRI_CONF_ENUM(2,"Geordende 2D kleurrasterisering") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(fr,"Méthode de tramage") \
                DRI_CONF_ENUM(0,"Diffusion d'erreur horizontale") \
                DRI_CONF_ENUM(1,"Diffusion d'erreur horizontale, réinitialisé pour chaque ligne") \
                DRI_CONF_ENUM(2,"Tramage ordonné des couleurs") \
        DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(sv,"Färgutjämningsmetod") \
                DRI_CONF_ENUM(0,"Horisontell felspridning") \
                DRI_CONF_ENUM(1,"Horisontell felspridning, återställ fel vid radbörjan") \
                DRI_CONF_ENUM(2,"Ordnad 2D-färgutjämning") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_FLOAT_DEPTH(def) \
DRI_CONF_OPT_BEGIN(float_depth,bool,def) \
        DRI_CONF_DESC(en,"Floating point depth buffer") \
        DRI_CONF_DESC(de,"Fließkomma z-Puffer") \
        DRI_CONF_DESC(es,"Búfer de profundidad en coma flotante") \
        DRI_CONF_DESC(nl,"Dieptebuffer als commagetal") \
        DRI_CONF_DESC(fr,"Z-buffer en virgule flottante") \
        DRI_CONF_DESC(sv,"Buffert för flytande punktdjup") \
DRI_CONF_OPT_END

/** \brief Performance-related options */
#define DRI_CONF_SECTION_PERFORMANCE \
DRI_CONF_SECTION_BEGIN \
        DRI_CONF_DESC(en,"Performance") \
        DRI_CONF_DESC(de,"Leistung") \
        DRI_CONF_DESC(es,"Rendimiento") \
        DRI_CONF_DESC(nl,"Prestatie") \
        DRI_CONF_DESC(fr,"Performance") \
        DRI_CONF_DESC(sv,"Prestanda")

#define DRI_CONF_TCL_SW 0
#define DRI_CONF_TCL_PIPELINED 1
#define DRI_CONF_TCL_VTXFMT 2
#define DRI_CONF_TCL_CODEGEN 3
#define DRI_CONF_TCL_MODE(def) \
DRI_CONF_OPT_BEGIN_V(tcl_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"TCL mode (Transformation, Clipping, Lighting)") \
                DRI_CONF_ENUM(0,"Use software TCL pipeline") \
                DRI_CONF_ENUM(1,"Use hardware TCL as first TCL pipeline stage") \
                DRI_CONF_ENUM(2,"Bypass the TCL pipeline") \
                DRI_CONF_ENUM(3,"Bypass the TCL pipeline with state-based machine code generated on-the-fly") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"TCL-Modus (Transformation, Clipping, Licht)") \
                DRI_CONF_ENUM(0,"Benutze die Software-TCL-Pipeline") \
                DRI_CONF_ENUM(1,"Benutze Hardware TCL als erste Stufe der TCL-Pipeline") \
                DRI_CONF_ENUM(2,"Umgehe die TCL-Pipeline") \
                DRI_CONF_ENUM(3,"Umgehe die TCL-Pipeline mit zur Laufzeit erzeugtem, zustandsbasiertem Maschinencode") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(es,"Modo TCL (Transformación, Recorte, Iluminación)") \
                DRI_CONF_ENUM(0,"Usar tubería TCL por software") \
                DRI_CONF_ENUM(1,"Usar TCL por hardware en la primera fase de la tubería TCL") \
                DRI_CONF_ENUM(2,"Pasar por alto la tubería TCL") \
                DRI_CONF_ENUM(3,"Pasar por alto la tubería TCL con código máquina basado en estados generado al vuelo") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(nl,"TCL-modus (Transformatie, Clipping, Licht)") \
                DRI_CONF_ENUM(0,"Gebruik software TCL pijpleiding") \
                DRI_CONF_ENUM(1,"Gebruik hardware TCL as eerste TCL pijpleiding trap") \
                DRI_CONF_ENUM(2,"Omzeil de TCL pijpleiding") \
                DRI_CONF_ENUM(3,"Omzeil de TCL pijpleiding met staatgebaseerde machinecode die tijdens executie gegenereerd wordt") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(fr,"Mode de TCL (Transformation, Clipping, Eclairage)") \
                DRI_CONF_ENUM(0,"Utiliser un pipeline TCL logiciel") \
                DRI_CONF_ENUM(1,"Utiliser le TCL matériel pour le premier niveau de pipeline") \
                DRI_CONF_ENUM(2,"Court-circuiter le pipeline TCL") \
                DRI_CONF_ENUM(3,"Court-circuiter le pipeline TCL par une machine à états qui génère le codede TCL à la volée") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(sv,"TCL-läge (Transformation, Clipping, Lighting)") \
                DRI_CONF_ENUM(0,"Använd programvaru-TCL-rörledning") \
                DRI_CONF_ENUM(1,"Använd maskinvaru-TCL som första TCL-rörledningssteg") \
                DRI_CONF_ENUM(2,"Kringgå TCL-rörledningen") \
                DRI_CONF_ENUM(3,"Kringgå TCL-rörledningen med tillståndsbaserad maskinkod som direktgenereras") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_FTHROTTLE_BUSY 0
#define DRI_CONF_FTHROTTLE_USLEEPS 1
#define DRI_CONF_FTHROTTLE_IRQS 2
#define DRI_CONF_FTHROTTLE_MODE(def) \
DRI_CONF_OPT_BEGIN_V(fthrottle_mode,enum,def,"0:2") \
        DRI_CONF_DESC_BEGIN(en,"Method to limit rendering latency") \
                DRI_CONF_ENUM(0,"Busy waiting for the graphics hardware") \
                DRI_CONF_ENUM(1,"Sleep for brief intervals while waiting for the graphics hardware") \
                DRI_CONF_ENUM(2,"Let the graphics hardware emit a software interrupt and sleep") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Methode zur Begrenzung der Bildverzögerung") \
                DRI_CONF_ENUM(0,"Aktives Warten auf die Grafikhardware") \
                DRI_CONF_ENUM(1,"Kurze Schlafintervalle beim Warten auf die Grafikhardware") \
                DRI_CONF_ENUM(2,"Die Grafikhardware eine Softwareunterbrechnung erzeugen lassen und schlafen") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(es,"Método para limitar la latencia de rénder") \
                DRI_CONF_ENUM(0,"Esperar activamente al hardware gráfico") \
                DRI_CONF_ENUM(1,"Dormir en intervalos cortos mientras se espera al hardware gráfico") \
                DRI_CONF_ENUM(2,"Permitir que el hardware gráfico emita una interrupción de software y duerma") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(nl,"Methode om beeldopbouwvertraging te onderdrukken") \
                DRI_CONF_ENUM(0,"Actief wachten voor de grafische hardware") \
                DRI_CONF_ENUM(1,"Slaap voor korte intervallen tijdens het wachten op de grafische hardware") \
                DRI_CONF_ENUM(2,"Laat de grafische hardware een software onderbreking uitzenden en in slaap vallen") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(fr,"Méthode d'attente de la carte graphique") \
                DRI_CONF_ENUM(0,"Attente active de la carte graphique") \
                DRI_CONF_ENUM(1,"Attente utilisant usleep()") \
                DRI_CONF_ENUM(2,"Utiliser les interruptions") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(sv,"Metod för att begränsa renderingslatens") \
                DRI_CONF_ENUM(0,"Upptagen med att vänta på grafikhårdvaran") \
                DRI_CONF_ENUM(1,"Sov i korta intervall under väntan på grafikhårdvaran") \
                DRI_CONF_ENUM(2,"Låt grafikhårdvaran sända ut ett programvaruavbrott och sov") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3
#define DRI_CONF_VBLANK_MODE(def) \
DRI_CONF_OPT_BEGIN_V(vblank_mode,enum,def,"0:3") \
        DRI_CONF_DESC_BEGIN(en,"Synchronization with vertical refresh (swap intervals)") \
                DRI_CONF_ENUM(0,"Never synchronize with vertical refresh, ignore application's choice") \
                DRI_CONF_ENUM(1,"Initial swap interval 0, obey application's choice") \
                DRI_CONF_ENUM(2,"Initial swap interval 1, obey application's choice") \
                DRI_CONF_ENUM(3,"Always synchronize with vertical refresh, application chooses the minimum swap interval") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(de,"Synchronisation mit der vertikalen Bildwiederholung") \
                DRI_CONF_ENUM(0,"Niemals mit der Bildwiederholung synchronisieren, Anweisungen der Anwendung ignorieren") \
                DRI_CONF_ENUM(1,"Initiales Bildinterval 0, Anweisungen der Anwendung gehorchen") \
                DRI_CONF_ENUM(2,"Initiales Bildinterval 1, Anweisungen der Anwendung gehorchen") \
                DRI_CONF_ENUM(3,"Immer mit der Bildwiederholung synchronisieren, Anwendung wählt das minimale Bildintervall") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(es,"Sincronización con el refresco vertical (intervalos de intercambio)") \
                DRI_CONF_ENUM(0,"No sincronizar nunca con el refresco vertical, ignorar la elección de la aplicación") \
                DRI_CONF_ENUM(1,"Intervalo de intercambio inicial 0, obedecer la elección de la aplicación") \
                DRI_CONF_ENUM(2,"Intervalo de intercambio inicial 1, obedecer la elección de la aplicación") \
                DRI_CONF_ENUM(3,"Sincronizar siempre con el refresco vertical, la aplicación elige el intervalo de intercambio mínimo") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(nl,"Synchronisatie met verticale verversing (interval omwisselen)") \
                DRI_CONF_ENUM(0,"Nooit synchroniseren met verticale verversing, negeer de keuze van de applicatie") \
                DRI_CONF_ENUM(1,"Initïeel omwisselingsinterval 0, honoreer de keuze van de applicatie") \
                DRI_CONF_ENUM(2,"Initïeel omwisselingsinterval 1, honoreer de keuze van de applicatie") \
                DRI_CONF_ENUM(3,"Synchroniseer altijd met verticale verversing, de applicatie kiest het minimum omwisselingsinterval") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(fr,"Synchronisation de l'affichage avec le balayage vertical") \
                DRI_CONF_ENUM(0,"Ne jamais synchroniser avec le balayage vertical, ignorer le choix de l'application") \
                DRI_CONF_ENUM(1,"Ne pas synchroniser avec le balayage vertical par défaut, mais obéir au choix de l'application") \
                DRI_CONF_ENUM(2,"Synchroniser avec le balayage vertical par défaut, mais obéir au choix de l'application") \
                DRI_CONF_ENUM(3,"Toujours synchroniser avec le balayage vertical, l'application choisit l'intervalle minimal") \
        DRI_CONF_DESC_END \
        DRI_CONF_DESC_BEGIN(sv,"Synkronisering med vertikal uppdatering (växlingsintervall)") \
                DRI_CONF_ENUM(0,"Synkronisera aldrig med vertikal uppdatering, ignorera programmets val") \
                DRI_CONF_ENUM(1,"Initialt växlingsintervall 0, följ programmets val") \
                DRI_CONF_ENUM(2,"Initialt växlingsintervall 1, följ programmets val") \
                DRI_CONF_ENUM(3,"Synkronisera alltid med vertikal uppdatering, programmet väljer den minsta växlingsintervallen") \
        DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_HYPERZ_DISABLED 0
#define DRI_CONF_HYPERZ_ENABLED 1
#define DRI_CONF_HYPERZ(def) \
DRI_CONF_OPT_BEGIN(hyperz,bool,def) \
        DRI_CONF_DESC(en,"Use HyperZ to boost performance") \
        DRI_CONF_DESC(de,"HyperZ zur Leistungssteigerung verwenden") \
        DRI_CONF_DESC(es,"Usar HyperZ para potenciar rendimiento") \
        DRI_CONF_DESC(nl,"Gebruik HyperZ om de prestaties te verbeteren") \
        DRI_CONF_DESC(fr,"Utiliser le HyperZ pour améliorer les performances") \
        DRI_CONF_DESC(sv,"Använd HyperZ för att maximera prestandan") \
DRI_CONF_OPT_END

#define DRI_CONF_MAX_TEXTURE_UNITS(def,min,max) \
DRI_CONF_OPT_BEGIN_V(texture_units,int,def, # min ":" # max ) \
        DRI_CONF_DESC(en,"Number of texture units used") \
        DRI_CONF_DESC(de,"Anzahl der benutzten Textureinheiten") \
        DRI_CONF_DESC(es,"Número de unidades de textura usadas") \
        DRI_CONF_DESC(nl,"Aantal textuureenheden in gebruik") \
        DRI_CONF_DESC(fr,"Nombre d'unités de texture") \
        DRI_CONF_DESC(sv,"Antal använda texturenheter") \
DRI_CONF_OPT_END

#define DRI_CONF_ALLOW_LARGE_TEXTURES(def) \
DRI_CONF_OPT_BEGIN_V(allow_large_textures,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Support larger textures not guaranteed to fit into graphics memory") \
		DRI_CONF_ENUM(0,"No") \
		DRI_CONF_ENUM(1,"At least 1 texture must fit under worst-case assumptions") \
		DRI_CONF_ENUM(2,"Announce hardware limits") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Unterstütze grosse Texturen die evtl. nicht in den Grafikspeicher passen") \
		DRI_CONF_ENUM(0,"Nein") \
		DRI_CONF_ENUM(1,"Mindestens 1 Textur muss auch im schlechtesten Fall Platz haben") \
		DRI_CONF_ENUM(2,"Benutze Hardware-Limits") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(es,"Support larger textures not guaranteed to fit into graphics memory") \
		DRI_CONF_ENUM(0,"No") \
		DRI_CONF_ENUM(1,"At least 1 texture must fit under worst-case assumptions") \
		DRI_CONF_ENUM(2,"Announce hardware limits") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(nl,"Support larger textures not guaranteed to fit into graphics memory") \
		DRI_CONF_ENUM(0,"No") \
		DRI_CONF_ENUM(1,"At least 1 texture must fit under worst-case assumptions") \
		DRI_CONF_ENUM(2,"Announce hardware limits") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(fr,"Support larger textures not guaranteed to fit into graphics memory") \
		DRI_CONF_ENUM(0,"No") \
		DRI_CONF_ENUM(1,"At least 1 texture must fit under worst-case assumptions") \
		DRI_CONF_ENUM(2,"Announce hardware limits") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(sv,"Stöd för större texturer är inte garanterat att passa i grafikminnet") \
		DRI_CONF_ENUM(0,"Nej") \
		DRI_CONF_ENUM(1,"Åtminstone en textur måste passa för antaget sämsta förhållande") \
		DRI_CONF_ENUM(2,"Annonsera hårdvarubegränsningar") \
	DRI_CONF_DESC_END \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_BLEND_QUALITY(def,range) \
DRI_CONF_OPT_BEGIN_V(texture_blend_quality,float,def,range) \
	DRI_CONF_DESC(en,"Texture filtering quality vs. speed, AKA “brilinear” texture filtering") \
	DRI_CONF_DESC(de,"Texturfilterqualität versus -geschwindigkeit, auch bekannt als „brilineare“ Texturfilterung") \
	DRI_CONF_DESC(es,"Calidad de filtrado de textura vs. velocidad, alias filtrado ”brilinear“ de textura") \
	DRI_CONF_DESC(nl,"Textuurfilterkwaliteit versus -snelheid, ookwel bekend als “brilineaire” textuurfiltering") \
	DRI_CONF_DESC(fr,"Qualité/performance du filtrage trilinéaire de texture (filtrage brilinéaire)") \
	DRI_CONF_DESC(sv,"Texturfiltreringskvalitet mot hastighet, även kallad ”brilinear”-texturfiltrering") \
DRI_CONF_OPT_END

#define DRI_CONF_TEXTURE_HEAPS_ALL 0
#define DRI_CONF_TEXTURE_HEAPS_CARD 1
#define DRI_CONF_TEXTURE_HEAPS_GART 2
#define DRI_CONF_TEXTURE_HEAPS(def) \
DRI_CONF_OPT_BEGIN_V(texture_heaps,enum,def,"0:2") \
	DRI_CONF_DESC_BEGIN(en,"Used types of texture memory") \
		DRI_CONF_ENUM(0,"All available memory") \
		DRI_CONF_ENUM(1,"Only card memory (if available)") \
		DRI_CONF_ENUM(2,"Only GART (AGP/PCIE) memory (if available)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(de,"Benutzte Arten von Texturspeicher") \
		DRI_CONF_ENUM(0,"Aller verfügbarer Speicher") \
		DRI_CONF_ENUM(1,"Nur Grafikspeicher (falls verfügbar)") \
		DRI_CONF_ENUM(2,"Nur GART-Speicher (AGP/PCIE) (falls verfügbar)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(es,"Tipos de memoria de textura usados") \
		DRI_CONF_ENUM(0,"Toda la memoria disponible") \
		DRI_CONF_ENUM(1,"Sólo la memoria de la tarjeta (si disponible)") \
		DRI_CONF_ENUM(2,"Sólo memoria GART (AGP/PCIE) (si disponible)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(nl,"Gebruikte soorten textuurgeheugen") \
		DRI_CONF_ENUM(0,"Al het beschikbaar geheugen") \
		DRI_CONF_ENUM(1,"Alleen geheugen op de kaart (als het aanwezig is)") \
		DRI_CONF_ENUM(2,"Alleen GART (AGP/PCIE) geheugen (als het aanwezig is)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(fr,"Types de mémoire de texture") \
		DRI_CONF_ENUM(0,"Utiliser toute la mémoire disponible") \
		DRI_CONF_ENUM(1,"Utiliser uniquement la mémoire graphique (si disponible)") \
		DRI_CONF_ENUM(2,"Utiliser uniquement la mémoire GART (AGP/PCIE) (si disponible)") \
	DRI_CONF_DESC_END \
	DRI_CONF_DESC_BEGIN(sv,"Använda typer av texturminne") \
		DRI_CONF_ENUM(0,"Allt tillgängligt minne") \
		DRI_CONF_ENUM(1,"Endast kortminne (om tillgängligt)") \
		DRI_CONF_ENUM(2,"Endast GART-minne (AGP/PCIE) (om tillgängligt)") \
	DRI_CONF_DESC_END \
DRI_CONF_OPT_END

/* Options for features that are not done in hardware by the driver (like GL_ARB_vertex_program
   On cards where there is no documentation (r200) or on rasterization-only hardware). */
#define DRI_CONF_SECTION_SOFTWARE \
DRI_CONF_SECTION_BEGIN \
        DRI_CONF_DESC(en,"Features that are not hardware-accelerated") \
        DRI_CONF_DESC(de,"Funktionalität, die nicht hardwarebeschleunigt ist") \
        DRI_CONF_DESC(es,"Características no aceleradas por hardware") \
        DRI_CONF_DESC(nl,"Eigenschappen die niet hardwareversneld zijn") \
        DRI_CONF_DESC(fr,"Fonctionnalités ne bénéficiant pas d'une accélération matérielle") \
        DRI_CONF_DESC(sv,"Funktioner som inte är hårdvaruaccelererade")

#define DRI_CONF_ARB_VERTEX_PROGRAM(def) \
DRI_CONF_OPT_BEGIN(arb_vertex_program,bool,def) \
        DRI_CONF_DESC(en,"Enable extension GL_ARB_vertex_program") \
        DRI_CONF_DESC(de,"Erweiterung GL_ARB_vertex_program aktivieren") \
        DRI_CONF_DESC(es,"Activar la extensión GL_ARB_vertex_program") \
        DRI_CONF_DESC(nl,"Zet uitbreiding GL_ARB_vertex_program aan") \
        DRI_CONF_DESC(fr,"Activer l'extension GL_ARB_vertex_program") \
        DRI_CONF_DESC(sv,"Aktivera tillägget GL_ARB_vertex_program") \
DRI_CONF_OPT_END

#define DRI_CONF_NV_VERTEX_PROGRAM(def) \
DRI_CONF_OPT_BEGIN(nv_vertex_program,bool,def) \
        DRI_CONF_DESC(en,"Enable extension GL_NV_vertex_program") \
        DRI_CONF_DESC(de,"Erweiterung GL_NV_vertex_program aktivieren") \
        DRI_CONF_DESC(es,"Activar extensión GL_NV_vertex_program") \
        DRI_CONF_DESC(nl,"Zet uitbreiding GL_NV_vertex_program aan") \
        DRI_CONF_DESC(fr,"Activer l'extension GL_NV_vertex_program") \
        DRI_CONF_DESC(sv,"Aktivera tillägget GL_NV_vertex_program") \
DRI_CONF_OPT_END
