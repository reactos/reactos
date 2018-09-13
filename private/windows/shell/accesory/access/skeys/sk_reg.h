/*--------------------------------------------------------------
 *
 * FILE:			SK_reg.H							   
 *
 * PURPOSE:			Function prototypes for Serial registry Routines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

#define REG_FLAGS		TEXT("Flags")
#define REG_ACTIVEPORT	TEXT("ActivePort")
#define REG_PORT		TEXT("Port")
#define REG_BAUD		TEXT("Baud")

// Public Function ProtoTypes ----------------------------------
BOOL GetUserValues(int User);
BOOL SetUserValues();



