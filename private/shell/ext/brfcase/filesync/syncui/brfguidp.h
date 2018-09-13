//
//  brfguidp.h
//

// Briefcase Extension OLE object class ID.  This is different
// from the CLSID_Briefcase, which is the container CLSID.  We
// have two separate CLSIDs because they exist in two DLLs: 
// SYNCUI and SHELL232.
//
DEFINE_GUID(CLSID_BriefcaseExt, 0x0B399E01L, 0x0129, 0x101B, 0x9A, 0x4B, 0x00, 0xDD, 0x01, 0x0C, 0xCC, 0x48);

