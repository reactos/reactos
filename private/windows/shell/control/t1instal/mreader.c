/***
**
**   Module: MReader
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font metrics file, by parsing
**      the data/commands found in PFM and AFM files.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <string.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"

/* Module dependent types and prototypes. */
#include "pfm.h"



/***** CONSTANTS */
/*-none-*/



/***** LOCAL TYPES */
enum MType {t1_afm, t1_pfm, t1_unknown};



/***** MACROS */
/*-none-*/


/***** STATIC FUNCTIONS */

/***
** Function: MetricsType
**
** Description:
**   This function determines the type of the
**   metrics file that is associated to the 
**   main Adobe Type 1 outline file. 
***/
static enum MType MetricsType(const char *metrics)
{
   enum MType type;

   if (metrics==NULL || strlen(metrics)<5)
      type = t1_unknown;
   else if (!_strnicmp(&metrics[strlen(metrics)-3], "AFM", 3))
      type = t1_afm;
   else if (!_strnicmp(&metrics[strlen(metrics)-3], "PFM", 3))
      type = t1_pfm;
   else
      type = t1_unknown;

   return type;
}

/***** FUNCTIONS */

/***
** Function: ReadFontMetrics
**
** Description:
**  Read a font metrics file that associated to a type 1 font.
***/
errcode ReadFontMetrics(const char *metrics, struct T1Metrics *t1m)
{
   errcode status = SUCCESS;

   switch(MetricsType(metrics)) {
      case t1_pfm:
         status = ReadPFMMetrics(metrics, t1m);
         break;
      case t1_afm:
         /* status = ReadAFMMetrics(metrics, t1m); */
         break;
      case t1_unknown:
      default:
         status = BADMETRICS;
         break;
   }

   return status;
}

