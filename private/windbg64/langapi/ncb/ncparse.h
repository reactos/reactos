#ifndef __NCPARSE_H__
#define __NCPARSE_H__

#include <pdb.h>
#include <vcbudefs.h>
#include <bsc.h>
#include <time.h>
#include "bscext.h"

// values of kinds:
#define NCB_KIND_BASECLASS		0x01
#define NCB_KIND_CONTAINMENT	0x02
#define NCB_KIND_IDL_ATTR		0x03
#define NCB_KIND_IDL_CLASSTYPE	0x04

// PROP definition/declaration.
// This information is hidden in last four bits of ATR
#define INST_NCB_ATR_DECL	0x1000
#define INST_NCB_ATR_DEFN	0x2000
#define INST_NCB_ATR_JAVA   0x4000
#define INST_NCB_ATR_FINAL	0x8000

#define NCB_MOD_ATR_NODEP	0x01

#define NCBAttr(x)	((USHORT) (x & 0x0fff))


interface NcbParse
{
	// same as Bsc interface
	virtual BOOL close() pure; // close the ncbparse interface

// INTERFACE FOR MODULE LEVEL
	// primitives for getting module information
	
	// same as Bsc interface
	virtual BOOL getModuleByName(SZ sz, OUT IMOD *pimod) pure;
	// same as Bsc interface
	virtual BOOL getModuleContents(IMOD imod, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
	// same as Bsc interface
	virtual void disposeArray(void *pAnyArray) pure;

	virtual BOOL openMod (SZ szMod, BOOL bCreate, OUT IMOD * pimod) pure;
	virtual BOOL closeMod (IMOD imod, BOOL bSave) pure;
	virtual BOOL clearModContent (IMOD imod) pure;
	virtual BOOL setModTime (IMOD imod, time_t tStamp) pure;
	virtual BOOL getModTime (IMOD imod, time_t *ptStamp) pure;
	// set module attributes
	virtual BOOL setModAtr (IMOD imod, BYTE bAtr) pure;
	virtual BOOL getModAtr (IMOD imod, BYTE * pbAtr) pure;

	// check if module is member of a specific target
	virtual BOOL isModInTarget (HTARGET hTarget, IMOD imod) pure;
	virtual BOOL setModAsSrc (HTARGET hTarget, IMOD imod, BOOL bSource) pure;
	virtual BOOL isModTargetSource (HTARGET hTarget, IMOD imod) pure;
	// primitives for adding a target to a module
	virtual BOOL addModToTarget (HTARGET hTarget, IMOD imod, BOOL bProjSrc) pure;

	// primitives for adding an include file
	virtual BOOL addInclToMod (IMOD inclimod, HTARGET hTarget, IMOD imod) pure; 
	virtual BOOL isInclInMod (IMOD inclimod, HTARGET hTarget, IMOD imod) pure;
	// primitives for deleting an include file
	virtual BOOL delInclFrMod (IMOD inclimod, HTARGET hTarget, IMOD imod) pure;

	// primitives for deleting all include files
	virtual BOOL delAllInclFrMod (HTARGET hTarget, IMOD imod) pure;

	// primitives for deleting target from the database
	virtual BOOL delTarget (HTARGET hTarget) pure;

	// primitives for adding a target to the database
	virtual BOOL addTarget (HTARGET hTarget) pure;

	// primitives for deleting file from target
	virtual BOOL delModFrTarget (IMOD imod, HTARGET hTarget) pure;

	// primitives for setting all the include files:
	virtual BOOL getAllInclMod (HTARGET hTarget, IMOD imod, OUT IMOD ** ppimod, OUT ULONG * pcmod) pure;
	virtual BOOL getAllTarget (IMOD imod, OUT HTARGET ** ppTarget, OUT ULONG * pcTarget) pure;
	virtual BOOL getAllFlattenDeps (HTARGET hTarget, IMOD imod, OUT IMOD ** ppimod, OUT ULONG * pcmod, BOOL &bNotifyBuild) pure;
	// primitives for initializing target (ie: needs to do this
	// when target name change, first open a target)
	virtual BOOL mapTargetToSz (HTARGET hTarget, SZ szTarget) pure;
	virtual BOOL mapSzToTarget (SZ szTarget, HTARGET hTarget) pure;
	virtual BOOL imodInfo(IMOD imod, OUT SZ *pszModule) pure;
// INTERFACE FOR OBJECT LEVEL
	// primitives for adding an info
	// IINST is used for containment
	virtual BOOL addProp (SZ szName, TYP typ, ATR atr, IMOD imod, OUT IINST * pinst) pure;
	virtual BOOL setKind (IINST iinst, IINST iinstP, BYTE kind) pure;
	virtual BOOL setLine (IINST iinst, LINE lnStart) pure;
	virtual BOOL setDefn (IINST iinst) pure;
	virtual BOOL delProp (IINST iinst) pure;
	// For function, the 1st param is always return type followed by real params.
	// For variable, the 1st param is always type.
	virtual BOOL addParam (IINST iinst, SZ szName) pure;
	virtual BOOL iinstInfo(IINST iinst, OUT SZ *psz, OUT TYP *ptyp, OUT ATR *patr) pure ;
	virtual BOOL getAllGlobalsArray(MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
	virtual BOOL getAllGlobalsArray(MBF mbf, OUT IinstInfo **ppiinstinfo, OUT ULONG *pciinst) pure;
	virtual BOOL getGlobalsArray (MBF mbf, IMOD imod, OUT IinstInfo ** ppiinstinfo, OUT ULONG * pciinst) pure;
// Locking mechanism:
	virtual BOOL lock() pure;
	virtual BOOL unlock() pure;
	virtual BOOL notify() pure; // flush out notification queue!!
	virtual BOOL suspendNotify () pure;
	virtual BOOL resumeNotify() pure;
	virtual void graphBuilt() pure;
	virtual BOOL delUnreachable(HTARGET hTarget) pure;
	virtual BOOL isInit (HTARGET hTarget, IMOD imod) pure;
	virtual BOOL setInit (HTARGET hTarget, IMOD imod, BOOL bInit) pure;
	virtual BOOL notifyImod (OPERATION op, IMOD imod, HTARGET hTarget) pure;
	virtual BOOL notifyIinst (NiQ qItem, HTARGET hTarget, BYTE bLanguage) pure;
	virtual BOOL getBsc (HTARGET hTarget, SZ szTarget, Bsc ** ppBsc) pure;
	virtual BOOL delUninitTarget () pure;
    virtual BOOL imodFrSz (SZ szName, OUT IMOD *pimod) pure;
	virtual BOOL irefInfo(IREF iref, OUT SZ *pszModule, OUT LINE *piline) pure;
	virtual BOOL targetFiles (HTARGET hTarget, BOOL bSrcProjOnly, OUT IMOD ** ppimod, OUT ULONG * pcimod) pure;
	virtual BOOL setAllInit (HTARGET hTarget, BOOL bInit) pure;
	virtual void setNotifyBuild (IMOD imod, BOOL bNotifyBuild) pure;
	virtual BOOL isNotifyBuild (IMOD imod) pure;
	virtual BOOL notifyArrIinst (NiQ * pArrQ, ULONG uSize, HTARGET hTarget, BYTE bLanguage) pure;

};

/*
PDBAPI (BOOL) OpenNcb (SZ szName, HTARGET hTarget, SZ szTarget, BOOL bOverwrite, Bsc ** ppBsc);
PDBAPI (BOOL) OpenNcb (PDB * ppdb, HTARGET hTarget, SZ szTarget, Bsc ** ppBsc);
PDBAPI (BOOL) OpenNcb (SZ szName, BOOL bOverwrite, NcbParse ** ppNcParse);
PDBAPI (BOOL) OpenNcb (PDB * ppdb, NcbParse **ppNcParse);
*/

#endif
