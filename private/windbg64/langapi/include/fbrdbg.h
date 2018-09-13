/* fbrdbg.h 
 * 
 * Definitions used for the FIBER DEBUG system service
 *
 * Steven Steiner 
 * 9/26/96
 */

typedef enum _OFBR {
	OFBR_QUERY_LIST_SIZE =0,
	OFBR_GET_LIST,
	OFBR_SET_FBRCNTX,
	OFBR_DISABLE_FBRS,
	OFBR_ENABLE_FBRS
} OFBR;

typedef struct _OFBRS {
	OFBR	op;
	LPVOID	FbrCntx;
} OFBRS;


