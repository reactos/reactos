#include "OTScan.hpp"

#ifndef _OIDTOFILE_HPP
#define _OIDTOFILE_HPP

class SIMCOidTreeNode;

class OidToFile : public OidTreeScanner
{
	typedef struct _FileNode
	{
		long	lNextOffset;
		UINT	uNumChildren;
		UINT	uStrLen;
		LPSTR	lpszTextSubID;
		UINT	uNumSubID;
	} T_FILE_NODE;

	HFILE		m_hMibFile;
	const char	*m_pszMibFilename;
public:
	OidToFile();

	// DESCRIPTION:
	//		wrapper for the base class Scan();
	//		it find first the sizes of subtrees;
	// RETURN VALUE:
	//		0 on success
	//		-1 on failure;
	virtual int Scan();

	// DESCRIPTION:
	//		Creates the output file, containing the OID encoding
	// PARAMETERS:
	//		(in) pointer to the output file name
	// RETURN VALUE:
	//		0 on success, -1 on failure
	int SetMibFilename(const char * pszMibFilename);

	// DESCRIPTION:
	//		"callback" function, called each time a
	//		tree node passes through the scan. The user
	//		should redefine this function in the derived
	//		object to perform the action desired.
	// PARAMETERS:
	//		(in) Pointer to the current node in the tree.
	//			 Nodes are supplied in lexicographic order.
	// RETURN VALUE:
	//		0  - the scanner should continue
	//		-1 - the scanner should abort.
	int OnScanNode(const SIMCOidTreeNode *pOidNode);

	~OidToFile();
};

#endif
