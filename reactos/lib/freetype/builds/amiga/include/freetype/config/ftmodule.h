// TetiSoft: To specify which modules you need,
// insert the following in your source file and uncomment as needed:

/*
//#define FT_USE_AUTOHINT       // autohinter
//#define FT_USE_RASTER         // monochrome rasterizer
//#define FT_USE_SMOOTH         // anti-aliasing rasterizer
//#define FT_USE_TT             // truetype font driver
//#define FT_USE_T1             // type1 font driver
//#define FT_USE_T42            // type42 font driver
//#define FT_USE_T1CID          // cid-keyed type1 font driver  // no cmap support
//#define FT_USE_CFF            // opentype font driver
//#define FT_USE_BDF            // bdf bitmap font driver
//#define FT_USE_PCF            // pcf bitmap font driver
//#define FT_USE_PFR            // pfr font driver
//#define FT_USE_WINFNT         // windows .fnt|.fon bitmap font driver
#include "FT:src/base/ftinit.c"
*/

// TetiSoft: make sure that needed support modules are built in.
// Dependencies can be found by searching for FT_Get_Module.

#ifdef FT_USE_T42
#define FT_USE_TT
#endif

#ifdef FT_USE_TT
#define FT_USE_SFNT
#endif

#ifdef FT_USE_CFF
#define FT_USE_SFNT
#define FT_USE_PSHINT
#define FT_USE_PSNAMES
#endif

#ifdef FT_USE_T1
#define FT_USE_PSAUX
#define FT_USE_PSHINT
#define FT_USE_PSNAMES
#endif

#ifdef FT_USE_T1CID
#define FT_USE_PSAUX
#define FT_USE_PSHINT
#define FT_USE_PSNAMES
#endif

#ifdef FT_USE_PSAUX
#define FT_USE_PSNAMES
#endif

#ifdef FT_USE_SFNT
#define FT_USE_PSNAMES
#endif

// TetiSoft: Now include the modules

#ifdef FT_USE_AUTOHINT
FT_USE_MODULE(autohint_module_class)
#endif

#ifdef FT_USE_PSHINT
FT_USE_MODULE(pshinter_module_class)
#endif

#ifdef FT_USE_CFF
FT_USE_MODULE(cff_driver_class)
#endif

#ifdef FT_USE_T1CID
FT_USE_MODULE(t1cid_driver_class)
#endif

#ifdef FT_USE_BDF
FT_USE_MODULE(bdf_driver_class)
#endif

#ifdef FT_USE_PCF
FT_USE_MODULE(pcf_driver_class)
#endif

#ifdef FT_USE_PFR
FT_USE_MODULE(pfr_driver_class)
#endif

#ifdef FT_USE_PSAUX
FT_USE_MODULE(psaux_module_class)
#endif

#ifdef FT_USE_PSNAMES
FT_USE_MODULE(psnames_module_class)
#endif

#ifdef FT_USE_RASTER
FT_USE_MODULE(ft_raster1_renderer_class)
#endif

#ifdef FT_USE_SFNT
FT_USE_MODULE(sfnt_module_class)
#endif

#ifdef FT_USE_SMOOTH
FT_USE_MODULE(ft_smooth_renderer_class)
FT_USE_MODULE(ft_smooth_lcd_renderer_class)
FT_USE_MODULE(ft_smooth_lcdv_renderer_class)
#endif

#ifdef FT_USE_TT
FT_USE_MODULE(tt_driver_class)
#endif

#ifdef FT_USE_T1
FT_USE_MODULE(t1_driver_class)
#endif

#ifdef FT_USE_T42
FT_USE_MODULE(t42_driver_class)
#endif

#ifdef FT_USE_WINFNT
FT_USE_MODULE(winfnt_driver_class)
#endif
