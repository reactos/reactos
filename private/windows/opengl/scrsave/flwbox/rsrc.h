/**********************************Module**********************************\
*
* rsrc.h
*
* Resource definitions
*
* History:
*  Wed Jul 19 14:50:27 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef __RSRC_H__
#define __RSRC_H__

#define ID_COMPLEXITY              401
#define ID_IMAGE_SIZE              402

#define ID_COL_PICK_FIRST          ID_COL_CHECKER
#define ID_COL_CHECKER             500
#define ID_COL_PER_SIDE            501
#define ID_COL_SINGLE              502
#define ID_COL_PICK_LAST           ID_COL_SINGLE
#define ID_COL_PICK_COUNT          (ID_COL_PICK_LAST-ID_COL_PICK_FIRST+1)

#define ID_COL_SMOOTH              550
#define ID_COL_TRIANGLE            551
#define ID_COL_CYCLE               552

#define ID_SPIN                    600
#define ID_BLOOM                   601
#define ID_TWO_SIDED               602

#define ID_GEOM                    650

#define IDS_CONFIG_SMOOTH_COLORS        1000
#define IDS_CONFIG_TRIANGLE_COLORS      1001
#define IDS_CONFIG_CYCLE_COLORS         1002
#define IDS_CONFIG_SPIN                 1003
#define IDS_CONFIG_BLOOM                1004
#define IDS_CONFIG_SUBDIV               1005
#define IDS_CONFIG_COLOR_PICK           1006
#define IDS_CONFIG_IMAGE_SIZE           1007
#define IDS_CONFIG_GEOM                 1008
#define IDS_CONFIG_TWO_SIDED            1009

#define IDS_GEOM_FIRST                  IDS_GEOM_CUBE
#define IDS_GEOM_CUBE                   1025
#define IDS_GEOM_TETRA                  1026
#define IDS_GEOM_PYRAMIDS               1027
#define IDS_GEOM_CYLINDER               1028
#define IDS_GEOM_SPRING                 1029
#define IDS_GEOM_LAST                   IDS_GEOM_SPRING
#define IDS_GEOM_COUNT                  (IDS_GEOM_LAST-IDS_GEOM_FIRST+1)

#define IDS_INI_SECTION                 1051

#endif // __RSRC_H__
