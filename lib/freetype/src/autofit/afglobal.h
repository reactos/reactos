#ifndef __AF_GLOBAL_H__
#define __AF_GLOBAL_H__

#include "aftypes.h"

FT_BEGIN_HEADER

 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                F A C E   G L O B A L S                         *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/


 /*
  *  models the global hints data for a given face, decomposed into
  *  script-specific items..
  *
  */
  typedef struct AF_FaceGlobalsRec_*   AF_FaceGlobals;


  FT_LOCAL( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals );

  FT_LOCAL( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               AF_ScriptMetrics  *ametrics );

  FT_LOCAL( void )
  af_face_globals_free( AF_FaceGlobals  globals );

 /* */

FT_END_HEADER

#endif /* __AF_GLOBALS_H__ */
