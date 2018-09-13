#ifndef _OIDTREESCANNER_HPP
#define _OIDTREESCANNER_HPP

class Oid;

class OidTreeScanner
{
protected:
	SIMCOidTree		*m_pOidTree;

	// DESCRIPTION:
	//		Adds list of nodes from lstChildren	to the tail of m_recursionTrace,
	//		constructing at the same time the OID lexicographically order.
	//		the list received as parameter should not be modified;
	// PARAMETERS:
	//		(in) list of nodes to add
	void UpdateRecursionTrace(SIMCNodeList *pLstChildren, SIMCNodeList *pLstTrace);

	// DESCRIPTION:
	//		Gets the first symbol from the symbol list of the node pOidNode
	// PARAMETERS:
	//		(in) pOidNode whose symbol is to be returned
	//		(out) cszSymbol - pointer to the symbol (do not alter or free)
	// RETURN VALUE:
	//		0 on success, -1 on failure
	int GetNodeSymbol(const SIMCOidTreeNode *pOidNode, const char * & cszSymbol);

	// DESCRIPTION:
	//		Gets the complete OID information for the given pOidNode.
	//		It supplies both the numeric value and symbolic name for each
	//		component of the OID.
	// PARAMETERS:
	//      (in) pOidNode - the node whose OID is to be found
	//      (out) oid - the Oid object who stores the data
	// RETURN VALUE:
	//      0 on success, -1 on failure
	int GetNodeOid(const SIMCOidTreeNode *pOidNode, Oid &oid);

public:
	// initializes the OidTreeScanner
	OidTreeScanner();

	// DESCRIPTION:
	//		scans lexicographically the oid tree;
	// RETURN VALUE:
	//		0 on success
	//		-1 on failure;
	virtual int Scan();

	// DESCRIPTION:
	//		"callback" function, called each time a
	//		tree node passes through the scan. The user
	//		should redefine this function in the derived
	//		object to perform the action desired.
	// PARAMETERS:
	//		(in) Pointer to the current node in the tree.
	// RETURN VALUE:
	//		0 - the scanner should continue
	//		1 - the scanner should abort.
	virtual int OnScanNode(const SIMCOidTreeNode *pOidNode) = 0;

	// DESCRIPTION:
	//		Fills the symbols of the built-in objects from the static table
	// RETURN VALUE:
	//		0 - on success, -1 on failure
	int MergeBuiltIn();

	// DESCRIPTION:
	//		initializes the m_pOidTree.
	// PARAMETERS:
	//		(in) pointer to the SIMCOidTree to scan.
	void SetOidTree(SIMCOidTree *pOidTree);

	~OidTreeScanner();
};

#endif
