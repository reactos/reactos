#ifndef LSTFLOW_DEFINED
#define LSTFLOW_DEFINED

#include "lsdefs.h"

typedef DWORD  LSTFLOW;

#define lstflowDefault	0  

#define lstflowES		0  
#define lstflowEN		1  
#define lstflowSE		2  
#define lstflowSW		3  
#define lstflowWS		4  
#define lstflowWN		5  
#define lstflowNE		6  
#define lstflowNW		7  

/*
 *	The eight possible text flows are listed clockwise starting with default (Latin) one.
 *
 *	lstflowES is the coordinate system used when line grows to East and text grows to South.
 *	(Next letter is to the right (east) of previous, next line is created below (south) the previous.) 
 *
 *	For lstflowES positive u moves to the right, positive v moves up. (V axis is always in the direction
 *	of ascender, opposite to text growing direction.
 *
 *	Notice it is not the way axes are pointing in the default Windows mapping mode MM_TEXT. 
 *	In MM_TEXT vertical (y) axis increase from top to bottom, 
 *	in lstflowES vertical (v) axis increase from bottom to top.
 */
 
#define fUDirection			0x00000004L
#define fVDirection			0x00000001L
#define fUVertical			0x00000002L

/*
 *	The three bits that constitute lstflow happens to have well defined meanings.
 *
 *	Middle bit: on for vertical writing, off for horizontal.
 *	First (low value) bit: "on" means v-axis points right or down (positive).
 *	Third bit: "off" means u-axis points right or down (positive).
 *
 * See examples of usage in lstfset.c
 *
 */


#endif /* !LSTFLOW_DEFINED */

