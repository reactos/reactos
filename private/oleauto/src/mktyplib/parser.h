// PARSER.H
//
// parser-specific data

// attributes that must be the consistent among all property functions with
// the same name
#define fPropFuncBits	(fSOURCE | fBINDABLE | fREQUESTEDIT | fDISPLAYBIND | fDEFAULTBIND | fHIDDEN)

// Common bits
#define fHelpBits 	(fHELPSTRING | fHELPCONTEXT)
#define fFuncBits 	(fVARARG | fSTRING | fPropBits | fPropFuncBits)
#define fParmBits 	(fOPTIONAL | fIN | fOUT | fSTRING)
#define fTypeBits	(fHelpBits | fHIDDEN | fUUID)
#define fElemBits	(fHelpBits | fHIDDEN)


// *************************************
// attributes on typelibs
// *************************************
#define	VALID_LIBRARY_ATTR	(fHelpBits | fUUID | fVERSION | fHELPFILE | fLCID | fRESTRICTED | fHIDDEN)
#define	VALID_LIBRARY_ATTR2	(f2CONTROL)

// *************************************
// attributes on typeinfos
// *************************************
#define	VALID_TYPEDEF_ATTR	(fTypeBits | fPUBLIC)
#define	VALID_TYPEDEF_ATTR2	(0)

#define VALID_STRUCT_ENUM_UNION_ATTR	  (fTypeBits | fVERSION)
#define VALID_STRUCT_ENUM_UNION_ATTR2	  (0)

#define VALID_MODULE_ATTR	(fTypeBits | fVERSION | fDLLNAME)
#define VALID_MODULE_ATTR2	(0)

#define VALID_DISPINTER_ATTR	(fTypeBits | fVERSION)
#define VALID_DISPINTER_ATTR2	(f2NONEXTENSIBLE)

#define VALID_INTERFACE_ATTR	(fTypeBits | fVERSION | fODL)
#define VALID_INTERFACE_ATTR2	(f2DUAL | f2NONEXTENSIBLE | f2OLEAUTOMATION)

#define VALID_COCLASS_ATTR	(fTypeBits | fVERSION | fAPPOBJECT | fLICENSED | fPREDECLID)
#define VALID_COCLASS_ATTR2	(f2CONTROL)

// *************************************
// attributes on members of typeinfos
// *************************************
#define	VALID_DISPINTER_PROP_ATTR (fElemBits | fID | fSTRING | fREADONLY | fPropFuncBits)
#define	VALID_DISPINTER_PROP_ATTR2 (0)

#define	VALID_MODULE_FUNC_ATTR	  (fElemBits | fFuncBits | fRESTRICTED | fENTRY)
#define	VALID_MODULE_FUNC_ATTR2   (0)

#define	VALID_MODULE_CONST_ATTR	   (fElemBits)
#define	VALID_MODULE_CONST_ATTR2   (0)

#define	VALID_INTERFACE_FUNC_ATTR  (fElemBits | fFuncBits | fRESTRICTED | fID)
#define	VALID_INTERFACE_FUNC_ATTR2 (0)

#define	VALID_DISPINTER_FUNC_ATTR  (fElemBits | fFuncBits | fID)
#define	VALID_DISPINTER_FUNC_ATTR2 (0)

#define	VALID_COCLASS_INTER_ATTR  (fDEFAULT | fRESTRICTED | fSOURCE)
#define	VALID_COCLASS_INTER_ATTR2 (0)

#define	VALID_ENUM_ELEM_ATTR	   (fHelpBits)
#define	VALID_ENUM_ELEM_ATTR2	   (0)

#define	VALID_STRUCT_UNION_ELEM_ATTR	  (fHelpBits | fSTRING)
#define	VALID_STRUCT_UNION_ELEM_ATTR2	  (0)

// *************************************
// attributes on parameters
// *************************************
#define	VALID_MODULE_PARM_ATTR	  (fParmBits | fLCID | fRETVAL)
#define	VALID_MODULE_PARM_ATTR2	  (0)

#define	VALID_INTERFACE_PARM_ATTR  (fParmBits | fLCID | fRETVAL)
#define	VALID_INTERFACE_PARM_ATTR2 (0)

#define	VALID_DISPINTER_PARM_ATTR  (fParmBits)
#define	VALID_DISPINTER_PARM_ATTR2 (0)


// Bits for ParseKnownType(), to control what special types it will accept
#define	fAllowSAFEARRAY	0x01
#define	fAllowCARRAY	0x02
#define fAllowMODULE	0x04	
#define fAllowCOCLASS	0x08	
#define fAllowINTERFACE	0x10	
#define fAllowDISPINTER	0x20	

#define fAllowArray	(fAllowSAFEARRAY | fAllowCARRAY)
#define	fAllowInter	(fAllowINTERFACE | fAllowDISPINTER | fAllowCOCLASS)
