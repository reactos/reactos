
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <iostream.h>
#include <fstream.h>

#include <afx.h>
#include <afxtempl.h>
#include <objbase.h>
#include <afxwin.h>
#include <afxole.h>
#include <afxmt.h>
#include <wchar.h>
#include <process.h>
#include <objbase.h>
#include <initguid.h>

#include <bool.hpp>
#include <nString.hpp>
#include <ui.hpp>
#include <symbol.hpp>
#include <type.hpp>
#include <value.hpp>
#include <valueRef.hpp>
#include <typeRef.hpp>
#include <oidValue.hpp>
#include <objType.hpp>
#include <objTypV1.hpp>
#include <objTypV2.hpp>
#include <objId.hpp>
#include <trapType.hpp>
#include <notType.hpp>
#include <group.hpp>
#include <notGroup.hpp>
#include <module.hpp>
#include <sValues.hpp>
#include <lex_yy.hpp>
#include <ytab.hpp>
#include <errorMsg.hpp>
#include <errorCon.hpp>
#include <scanner.hpp>
#include <parser.hpp>
#include <apTree.hpp>
#include <oidTree.hpp>
#include <pTree.hpp>

#include "debug.hpp"
#include "Oid.hpp"
#include "OTScan.hpp"

typedef struct _oidBuiltIn
{
	const char * m_szSymbol;
	UINT m_nSubID;
	UINT m_nNextSibling;
} T_OID_BUILTIN;

#define BUILTIN_OID_COUNT	29
static T_OID_BUILTIN g_BuiltinOIDs[] = {
/*0 */	{"zero",           0, 29},	{"ccitt",           0,  2},	{"iso",           1, 28},
/*3 */	{"org",            3, 29},	{"dod",             6, 29},	{"internet",      1, 29},
/*6	*/	{"directory",      1,  7},	{"mgmt",            2, 12},	{"mib-2",         1, 29},
/*9	*/	{"interfaces",     2, 10},	{"ip",              4, 11},	{"transmission", 10, 29},
/*12*/	{"experimental",   3, 13},	{"private",         4, 15},	{"enterprises",   1, 29},
/*15*/	{"security",       5, 16},	{"snmpV2",          6, 29},	{"snmpDomains",   1, 24},
/*18*/	{"snmpUDPDomain",  1, 19},	{"snmpCLNSDomain",  2, 20}, {"snmpCONSDomain",3, 21},
/*21*/	{"snmpDDPDomain",  4, 22},  {"snmpIPXDomain",   5, 23}, {"rfc1157Domain", 6, 29},
/*24*/	{"snmpProxys",     2, 27},	{"rfc1157Proxy",    1, 29},	{"rfc1157Domain", 1, 29},
/*27*/	{"snmpModules",    3, 28},	{"joint-iso-ccitt", 2, 28}
};


// DESCRIPTION:
//		Adds list of nodes from lstChildren	to the tail of m_recursionTrace,
//		constructing at the same time the OID lexicographically order.
//		the list received as parameter should not be modified;
// PARAMETERS:
//		(in) list of nodes to add
void OidTreeScanner::UpdateRecursionTrace(SIMCNodeList *pLstChildren, SIMCNodeList *pLstTrace)
{
	int nChld;
	POSITION posChld;

	_ASSERT((pLstChildren != NULL) && (pLstTrace != NULL), "NULL parameter error!", NULL);

	// for each node from the children list
	for (posChld = pLstChildren->GetHeadPosition(), nChld=0;
	     posChld != NULL;
		 nChld++)
	{
		int nList;
		POSITION posList;
		SIMCOidTreeNode *nodeChld, *nodeList;
		
		nodeChld = pLstChildren->GetNext(posChld);
		nodeList = NULL;

		// if it's the first child to add, it goes at the head of the list
		if (nChld == 0)
		{
			_ASSERT(pLstTrace->AddHead(nodeChld)!=NULL,
				    "Memory Allocation error",
					NULL);
		}
		// otherwise, the head of the list should be an ordered list of
		// maximum nChld nodes. The new node is inserted in this list
		// with respect to it's value.
		else
		{
			// there are at least nChld nodes in m_recursionTrace
			for (nList=0, posList = pLstTrace->GetHeadPosition();
				 nList < nChld;
				 nList++)
			{
				POSITION posBackup = posList;

				nodeList = (SIMCOidTreeNode *)pLstTrace->GetNext(posBackup);
				_ASSERT(nodeList != NULL, "Internal OidNode List error!", NULL);

				// if the node to add has the value less then the current node, it
				// should be inserted in the list right before it.
				if (nodeChld->GetValue() < nodeList->GetValue())
					break;
				posList = posBackup;
			}
			if (posList != NULL)
			{
				_ASSERT(pLstTrace->InsertBefore(posList, nodeChld)!=NULL,
					    "Memory allocation error",
						NULL);
			}
			else
			{
				_ASSERT(pLstTrace->AddTail(nodeChld)!=NULL,
						"Memory allocation error",
						NULL);
			}
		}
	}
}

// DESCRIPTION:
//		Gets the first symbol from the symbol list of the node pOidNode
// PARAMETERS:
//		(in) pOidNode whose symbol is to be returned
//		(out) cszSymbol - pointer to the symbol (do not alter or free)
// RETURN VALUE:
//		0 on success, -1 on failure
int OidTreeScanner::GetNodeSymbol(const SIMCOidTreeNode *pOidNode, const char * & cszSymbol)
{
	const SIMCSymbolList *pSymbolList;

	_VERIFY(pOidNode != NULL, -1);
	pSymbolList = pOidNode->GetSymbolList();
	_VERIFY(pSymbolList != NULL, -1);

	if (pSymbolList->IsEmpty())
	{
		cszSymbol = (pOidNode->_pParam != NULL) ? (const char *)pOidNode->_pParam : NULL;
	}
	else
	{
		const SIMCSymbol **ppSymbol;

		ppSymbol = pSymbolList->GetHead();
		_VERIFY(ppSymbol != NULL && *ppSymbol != NULL, -1);

		cszSymbol = (*ppSymbol)->GetSymbolName();
	}
	return 0;
}

// DESCRIPTION:
//		Gets the complete OID information for the given pOidNode.
//		It supplies both the numeric value and symbolic name for each
//		component of the OID.
// PARAMETERS:
//      (in) pOidNode - the node whose OID is to be found
//      (out) oid - the Oid object who stores the data
// RETURN VALUE:
//      0 on success 
//		-1 on failure
int OidTreeScanner::GetNodeOid(const SIMCOidTreeNode *pOidNode, Oid &oid)
{
	_VERIFY(pOidNode != NULL, -1);
	_VERIFY(m_pOidTree != NULL, -1);
	do
	{
		const char * cszSymbol = NULL;

		_VERIFY(GetNodeSymbol(pOidNode, cszSymbol)==0, -1);

		_VERIFY(oid.AddComponent(pOidNode->GetValue(), cszSymbol)==0, -1);

		pOidNode = m_pOidTree->GetParentOf(pOidNode);
	} while (pOidNode != NULL);
	oid.ReverseComponents();

	return 0;
}


// initializes the OidTreeScanner
OidTreeScanner::OidTreeScanner()
{
	m_pOidTree = NULL;
}

// DESCRIPTION:
//		scans lexicographically the oid tree;
// RETURN VALUE:
//		0 on success
//		-1 on failure;
int OidTreeScanner::Scan()
{
	SIMCOidTreeNode *pOidNode;
	SIMCNodeList recursionTrace;

	// get the root node from the oid tree,
	// return "error" if no tree or no root
	_VERIFY(m_pOidTree != NULL, -1);
	pOidNode = (SIMCOidTreeNode *)m_pOidTree->GetRoot();
	_VERIFY(pOidNode != NULL, -1);

	// initialize the recursion trace list with the root node
	_VERIFY(recursionTrace.AddHead(pOidNode)!=NULL, -1);

	// start to scan the tree
	while (!recursionTrace.IsEmpty())
	{
		// list of current node's children
		SIMCNodeList *lstChildren;

		// allways pick up the node in the head of the list
		pOidNode = recursionTrace.GetHead();

		// then erase it from the list
		recursionTrace.RemoveAt(recursionTrace.GetHeadPosition());

		// check to see if the scanner should stop (with error code)
		_VERIFY(OnScanNode(pOidNode)==0, -1);

		// get the list of children
		lstChildren = (SIMCNodeList *)pOidNode->GetListOfChildNodes();

		// if there are children
		if (lstChildren != NULL)
			// add children to the head of the trace list
			UpdateRecursionTrace(lstChildren, &recursionTrace);
	}
	return 0;
}

// DESCRIPTION:
//		Fills the symbols of the built-in objects from the static table
// RETURN VALUE:
//		0 - on success, -1 on failure
int OidTreeScanner::MergeBuiltIn()
{
	SIMCNodeList lstOidStack;
	SIMCOidTreeNode	*pOid;
	CList <unsigned int, unsigned int> wlstBuiltinStack;
	unsigned int nBuiltin;
	
	// initialize the two stacks with the root nodes from 
	// the oid tree and from the builtin symbols
	pOid = (SIMCOidTreeNode *)m_pOidTree->GetRoot();
	_VERIFY(pOid != NULL, -1);
	_VERIFY(lstOidStack.AddHead(pOid)!=NULL, -1);
	_VERIFY(wlstBuiltinStack.AddHead((unsigned int)0)!=NULL, -1);

	// as long as there are items on the stack, process each item
	// item is processed only if it doesn't have a symbol associated
	while(!lstOidStack.IsEmpty())
	{
		const SIMCSymbolList *pSymbolList;

		pOid = lstOidStack.RemoveHead();
		nBuiltin = wlstBuiltinStack.RemoveHead();
		pSymbolList = pOid->GetSymbolList();
		_VERIFY(pSymbolList != NULL, -1);

		// if node already has a symbol attached, no need to dive deeper
		if (!pSymbolList->IsEmpty())
			continue;
		else
		{
			const SIMCNodeList *pChildList;

			pOid->_pParam = (void *)g_BuiltinOIDs[nBuiltin].m_szSymbol;

			// now push new nodes on the stacks
			pChildList = pOid->GetListOfChildNodes();
			_VERIFY(pChildList != NULL, -1);
			for (POSITION p = pChildList->GetHeadPosition(); p != NULL;)
			{
				unsigned int i;

				pOid = pChildList->GetNext(p);
				_VERIFY(pOid != NULL, -1);

				// the child nodes always begin at index nBuiltin+1 (if they exist)
				// and end before the parent's first sibling node.
				for (i = nBuiltin+1;
					 i < g_BuiltinOIDs[nBuiltin].m_nNextSibling;
					 i = g_BuiltinOIDs[i].m_nNextSibling)
				{
					// when the match is found, push both nodes on their stacks (in sync)
					// and go to another child
					if (g_BuiltinOIDs[i].m_nSubID == (UINT)pOid->GetValue())
					{
						_VERIFY(lstOidStack.AddHead(pOid)!=NULL, -1);
						_VERIFY(wlstBuiltinStack.AddHead(i)!=NULL, -1);
						break;
					}
				}
			}
		}
	}
	return 0;
}

// DESCRIPTION:
//		initializes the m_pOidTree.
// PARAMETERS:
//		(in) pointer to the SIMCOidTree to scan.
void OidTreeScanner::SetOidTree(SIMCOidTree *pOidTree)
{
	m_pOidTree = pOidTree;
}

OidTreeScanner::~OidTreeScanner()
{
}
