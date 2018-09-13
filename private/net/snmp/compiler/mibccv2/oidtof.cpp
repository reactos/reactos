#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

#include "Debug.hpp"
#include "Oid.hpp"
#include "OidToF.hpp"
#include "Configs.hpp"

extern Configs theConfigs;

OidToFile::OidToFile()
{
	m_hMibFile = NULL;
	m_pszMibFilename = NULL;
}

// DESCRIPTION:
//		wrapper for the base class Scan();
//		it find first the sizes of subtrees (subtree's root node included)
// RETURN VALUE:
//		0 on success
//		-1 on failure;
int OidToFile::Scan()
{
	SIMCOidTreeNode *pOidNode;
	SIMCNodeList recursionTrace;

	// get the root node from the oid tree,
	// return "error" if no tree or no root
	_VERIFY(m_pOidTree != NULL, -1);
	pOidNode = (SIMCOidTreeNode *)m_pOidTree->GetRoot();
	_VERIFY(pOidNode != NULL, -1);

	// node hasn't been scanned; _lParam=number of dependencies
	pOidNode->_lParam = (DWORD)pOidNode->GetListOfChildNodes()->GetCount();
	// initialize the recursionTree with the root node
	_VERIFY(recursionTrace.AddHead(pOidNode)!=NULL, -1);

	// start to scan the tree
	while (!recursionTrace.IsEmpty())
	{
		const SIMCNodeList *pLstChildren;

		// allways pick up the node in the head of the list
		pOidNode = recursionTrace.GetHead();

		// get the dependencies list
		pLstChildren = pOidNode->GetListOfChildNodes();

		// if there are no more dependencies to process
		// sum the children values and add to its own
		if (pOidNode->_lParam == 0)
		{
			SIMCOidTreeNode *pParent;
			const char * cszSymbol = NULL;

			_VERIFY(GetNodeSymbol(pOidNode, cszSymbol)==0, -1);

			pOidNode->_lParam = sizeof(T_FILE_NODE) + (cszSymbol != NULL ? strlen(cszSymbol) : 0);

			// compute the cumulated size of the subtree
			for (POSITION p = pLstChildren->GetHeadPosition(); p != NULL;)
			{
				const SIMCOidTreeNode *pChild;

				pChild = pLstChildren->GetNext(p);
				_VERIFY(pChild!=NULL, -1);

				// Modify here!!!!
				pOidNode->_lParam += pChild->_lParam;
			}

			// decrease the number of dependencies from the parent node
			pParent = m_pOidTree->GetParentOf(pOidNode);
			if ( pParent != NULL)
			{
				pParent->_lParam--;
			}

			// delete the node from the list
			recursionTrace.RemoveHead();
		}
		else
		{
			// add the children list to the front of the recursionTrace
			for (POSITION p = pLstChildren->GetHeadPosition(); p != NULL;)
			{
				SIMCOidTreeNode *pChild;

				pChild = pLstChildren->GetNext(p);
				_VERIFY(pChild!=NULL, -1);

				pChild->_lParam = (DWORD)pChild->GetListOfChildNodes()->GetCount();
				_VERIFY(recursionTrace.AddHead(pChild)!=NULL, -1);
			}
		}
	}
	return OidTreeScanner::Scan();
}

// DESCRIPTION:
//		Creates the output file, containing the OID encoding
// PARAMETERS:
//		(in) pointer to the output file name
// RETURN VALUE:
//		0 on success, -1 on failure
int OidToFile::SetMibFilename(const char * pszMibFilename)
{
	if (m_pszMibFilename != NULL)
	{
		delete (void *)m_pszMibFilename;
		m_pszMibFilename = NULL;
	}

	if (pszMibFilename != NULL)
	{
		_VERIFY(strlen(pszMibFilename) != 0, -1);
		m_pszMibFilename = new char [strlen(pszMibFilename) + 1];
		_VERIFY(m_pszMibFilename != NULL, -1);
		strcpy((char *)m_pszMibFilename, pszMibFilename);
	}
	return 0;
}

// DESCRIPTION:
//		"callback" function, called each time a
//		tree node passes through the scan.
//		in pOidNode->_lParam there is the cumulated size
//		of the hole subtree, root included.
//		size = sizeof(T_FILE_NODE) + strlen(node symbol)
// PARAMETERS:
//		(in) Pointer to the current node in the tree.
//			 Nodes are supplied in lexicographic order.
// RETURN VALUE:
//		0  - the scanner should continue
//		-1 - the scanner should abort.
int OidToFile::OnScanNode(const SIMCOidTreeNode *pOidNode)
{
	T_FILE_NODE fileNode;
	char *nodeSymbol = NULL;

	// skip the '0' root of the OID tree
	if (m_pOidTree->GetParentOf(pOidNode) == NULL)
		return 0;

	if (theConfigs.m_dwFlags & CFG_PRINT_TREE)
	{
		Oid oid;

		_VERIFY(GetNodeOid(pOidNode, oid)==0, -1);
		cout << oid << "\n";
	}
	if (theConfigs.m_dwFlags & CFG_PRINT_NODE)
	{
		cout << "."; cout.flush();
	}

	// if no need to write output file, return success
	if (m_pszMibFilename == NULL)
		return 0;

	_VERIFY(GetNodeSymbol(pOidNode, nodeSymbol) == 0, -1);

	// build the T_FILE_NODE structure
	fileNode.uNumChildren = (UINT)pOidNode->GetListOfChildNodes()->GetCount();
	fileNode.lpszTextSubID = NULL;
	fileNode.uNumSubID = pOidNode->GetValue();

	if (nodeSymbol == NULL)
	{
		fileNode.lNextOffset = pOidNode->_lParam - sizeof(T_FILE_NODE);
		fileNode.uStrLen = 0;
	}
	else
	{
		fileNode.uStrLen = strlen(nodeSymbol);
		fileNode.lNextOffset = pOidNode->_lParam - fileNode.uStrLen - sizeof(T_FILE_NODE);
	}

	// create / write_open file if not already created
	if (m_hMibFile == NULL)
	{
		OFSTRUCT of;

		_VERIFY(m_pszMibFilename != NULL, -1);
		m_hMibFile = OpenFile(m_pszMibFilename, &of, OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE);
		_VERIFY(m_hMibFile != -1, -1);
	}

	// write fileNode to file
	_VERIFY(_lwrite(m_hMibFile, (LPSTR)&fileNode, sizeof(T_FILE_NODE)) == sizeof(T_FILE_NODE), -1);

	// write node's symbol if exists
	if (fileNode.uStrLen != 0)
	{
		_VERIFY(_lwrite(m_hMibFile, nodeSymbol, fileNode.uStrLen) == fileNode.uStrLen, -1);
	}

	return 0;
}

OidToFile::~OidToFile()
{
	if (m_pszMibFilename != NULL)
		delete (void *)m_pszMibFilename;
	if (m_hMibFile != NULL)
		_lclose(m_hMibFile);
}
