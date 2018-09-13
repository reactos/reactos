//+---------------------------------------------------------------------
//
//  File:       dump.cxx
//
//  Contents:   Diagnostic output routines
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#if DBG == 1 || defined(DUMPTREE)

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

DeclareTagOther(tagSaveDocOnDump, "TreeDump", "Save Doc in Tree Dump");
DeclareTagOther(tagSaveLBOnDump, "TreeDump", "Save LineBreaks in Tree Dump");
DeclareTagOther(tagDumpAlwaysOverwrite, "TreeDump", "Always create new file");
DeclareTagOther(tagDumpXML, "TreeDump", "Dump XML");

#define NODE_TYPE_BEGIN  -1
#define NODE_TYPE_NEUTRAL 0
#define NODE_TYPE_END     1

void
CMarkup::PrintNodeTag ( CTreeNode * pNode, int type )
{
    CElement * pElement = pNode->Element();
    
    if (type != NODE_TYPE_NEUTRAL)
    {
        if (type == NODE_TYPE_BEGIN)
            WriteHelp( g_f, _T("<<") );
        else
            WriteHelp( g_f, _T("<</") );
    }

    if (pElement->Namespace())
        WriteHelp( g_f, _T("<0s>:"), pElement->Namespace() );

    if (pNode->Tag() == ETAG_UNKNOWN)
        WriteHelp( g_f, _T("\"<0s>\""), pElement->TagName() );
    else
        WriteHelp( g_f, _T("<0s>"), pElement->TagName() );

    if (type != NODE_TYPE_NEUTRAL)
        WriteHelp( g_f, _T(">") );

    WriteHelp( g_f, _T(" [E<0d> N<1d>"), long( pElement->SN() ), pNode->SN() );

    if (pElement->_fBreakOnEmpty)
        WriteString( g_f, _T(",BreakOnEmpty"));

    WriteString( g_f, _T("]"));
}

void
CMarkup::PrintNode ( CTreeNode * pNode, BOOL fSimple, int type )
{
    PrintNodeTag( pNode, type );

    if (!fSimple)
    {
        WriteHelp( g_f, _T(" (") );

        WriteHelp(
            g_f, _T("ERefs=<0d>, EAllRefs=<1d>, <2d> <3d> <4d>"),
            (long)pNode->Element()->GetObjectRefs(), (long)pNode->Element()->GetRefs(),
            (long)pNode->_iCF, (long)pNode->_iPF, (long)pNode->_iFF);

        if (pNode->Parent())
        {
            WriteString( g_f, _T(", Parent = ") );

            PrintNodeTag( pNode->Parent() );
        }

        WriteString( g_f, _T(")") );
    }
}

void
CMarkup::DumpTree ( )
{
    if (IsTagEnabled(tagDumpXML))
    {
        DumpTreeInXML();
    }
    else
    {
        if (!InitDumpFile(IsTagEnabled(tagDumpAlwaysOverwrite)))
            return;

        DumpTreeWithMessage();

        CloseDumpFile();
    }
}

void
CMarkup::DumpTreeOverwrite ( )
{
    if (!InitDumpFile(TRUE))
        return;

    DumpTreeWithMessage();

    CloseDumpFile();
}

void
DumpTreeOverwrite()
{
    if (g_pDebugMarkup)
        g_pDebugMarkup->DumpTreeOverwrite();
}

void 
CMarkup::SetDebugMarkup ( )
{ 
    g_pDebugMarkup = this; 
}


static void 
DumpFormat( FORMATETC * pfetc, TCHAR * szFormat, BOOL fUnicode )
{
#ifndef WIN16
    char          abBuf[256];
    ULONG         cbRead;
    HRESULT       hr;
    STGMEDIUM     medium = {TYMED_NULL, NULL, NULL};
    HGLOBAL       hglobal = NULL;
    LPSTREAM      pIStream = NULL;   // IStream pointer
    IDataObject*  pdo;               // Pointer to the data object on the clipboard

    WriteHelp(g_f, _T("----- FORMAT: <0s> -----\r\n"), szFormat);

    hr = OleGetClipboard(&pdo);
    if ( hr )
        goto Cleanup;

    hr = pdo->GetData(pfetc, &medium);
    if( hr )
        goto Cleanup;

    // STGFIX: t-gpease 8-13-97
    Assert(medium.tymed == TYMED_HGLOBAL);

    hglobal = medium.hGlobal;
    if( !GlobalLock(hglobal) )
    {
        hglobal = NULL;
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr  = THR(CreateStreamOnHGlobal(hglobal, FALSE, &pIStream));
    if (hr)
        goto Cleanup;

    while (S_OK == pIStream->Read(abBuf, ARRAY_SIZE(abBuf), &cbRead))
    {
        if (fUnicode)
        {
            TCHAR *     pchBuf;
            ULONG       cchLength;
            ULONG       cchRead = cbRead/2;

            for( pchBuf = (TCHAR*) abBuf, cchLength = 0;
                 cchLength < cchRead && *pchBuf;
                 pchBuf++, cchLength++ )
                ;

            if (!cchLength)
                break;

            WriteFileAnsi( g_f, abBuf, cchLength );

            if (cchLength < cchRead)
                break;
        }
        else
        {
            char *        pbBuf;
            ULONG         cchLength;
            TCHAR         achBuffer[256];
            long          cchWideChar;

            for (pbBuf = abBuf, cchLength = 0;
                 cchLength < cbRead && *pbBuf;
                 pbBuf++, cchLength++);

            if (!cchLength)
                break;

            cchWideChar = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    abBuf,
                    cchLength,
                    achBuffer,
                    ARRAY_SIZE(achBuffer));

            Assert(cchWideChar > 0);

            WriteFileAnsi( g_f, achBuffer, cchWideChar );

            if (cchLength < cbRead)
                break;
        }
    }

    WriteString( g_f, _T("\r\n") );

Cleanup:

    ReleaseInterface(pIStream);

    if(hglobal)
    {
        GlobalUnlock(hglobal);
    }

    ReleaseStgMedium(&medium);

    pdo->Release();
#endif //!WIN16
}

void
CMarkup::DumpClipboardText( )
{
#ifndef WIN16
    if( !InitDumpFile())
        goto Cleanup;

    WriteString(g_f, _T("----- DUMP OF CLIPBOARD CONTENTS -----\r\n"));

    DumpFormat( &g_rgFETC[iHTML], _T("CF_HTML"), FALSE );
//    DumpFormat( &g_rgFETC[iRtfFETC], _T("RTF") );
    DumpFormat( &g_rgFETC[iUnicodeFETC], _T("Unicode plain text"), TRUE );
    DumpFormat( &g_rgFETC[iAnsiFETC], _T("ANSI plain text"), FALSE );

    WriteString(g_f, _T("----- END DUMP OF CLIPBOARD CONTENTS -----\r\n\r\n"));

Cleanup:
    CloseDumpFile( );
#endif //!WIN16
}

static void DumpNumber ( int n )
{
    if (n < 10)
        WriteHelp( g_f, _T("   ") );
    else if (n < 100)
        WriteHelp( g_f, _T("  ") );
    else if (n < 1000)
        WriteHelp( g_f, _T(" ") );
        
    WriteHelp( g_f, _T("<0d>"), long( n ) );
}

static void DumpSerialNumber ( TCHAR * pchType, int n )
{
    WriteHelp( g_f, _T("[<0s>"), pchType );
    DumpNumber( n );
    WriteHelp( g_f, _T("]") );
}

void
CMarkup::DumpTextTreePos ( CTreePos * ptpCurr )
{
    long cch;

    cch = ptpCurr->Cch();

    WriteString( g_f, _T("  ") );
    
    WriteString( g_f, _T("'") );
    
    if (cch)
    {
        CTxtPtr rpTX(this);
        TCHAR chBuff [ 30 ];
        long cpCurr = ptpCurr->GetCp();
        BOOL fNotEnoughCharacters = FALSE;

        if( cpCurr + cch > rpTX.GetTextLength() )
        {
            cch = rpTX.GetTextLength() - cpCurr;
            fNotEnoughCharacters = TRUE;
        }

        
        if(cch > 0)
        {
            rpTX.BindToCp(cpCurr);

            if (cch <= ARRAY_SIZE( chBuff ))
            {
                rpTX.GetRawText( cch, chBuff );

                WriteFormattedString( g_f, chBuff, cch );
            }
            else
            {
                long cchDiv2 = ARRAY_SIZE( chBuff ) / 2;

                rpTX.GetRawText( cchDiv2, chBuff );

                WriteFormattedString( g_f, chBuff, cchDiv2 );

                WriteString( g_f, _T( "..." ) );

                rpTX.AdvanceCp( cch - cchDiv2 );

                rpTX.GetRawText( cchDiv2, chBuff );

                WriteFormattedString( g_f, chBuff, cchDiv2 );

                rpTX.AdvanceCp( - (cch - cchDiv2) );
            }
        }


        if (fNotEnoughCharacters)
        {
            WriteString( g_f, _T(", Not enough text in the story") );
        }
    }
    
    WriteString( g_f, _T("'"));
    
    WriteHelp( g_f, _T(", _sid(<0s>)"), SidName( ptpCurr->Sid() ) );

    if( ptpCurr->TextID() )
    {
        WriteHelp( g_f, _T(", _lTextID(<0d>)"), long( ptpCurr->TextID() ) );
    }
}

void
CMarkup::DumpTreeWithMessage ( TCHAR * szMessage )
{
    if (szMessage && szMessage[0])
        WriteHelp( g_f, _T("<0s>\r\n\r\n"), szMessage );

    if (IsTagEnabled(tagSaveDocOnDump))
    {
        //
        // Print out the saved version of the tree
        //

        HGLOBAL     hg;
        IStream *   pStm;

        THR(CreateStreamOnHGlobal(NULL, TRUE, &pStm));
        {
            Doc()->SaveToStream( pStm, 0, CP_1252 );
        }

        THR(GetHGlobalFromStream(pStm, &hg));

        if (hg)
        {
            long nChars = GlobalSize( hg );

            char * pChar = (char *) GlobalLock( hg );

            for ( ; nChars > 0 ; nChars--, pChar++ )
            {
                WriteChar(g_f, *pChar);
            }

            GlobalUnlock( hg );
            GlobalFree( hg );
        }

        WriteString( g_f, _T("\r\n\r\n"));
    }

    CTreePos * ptpCurr = FirstTreePos();
    CLineBreakCompat * pLineBreaks = NULL;
    CTreePosGap tpg;
    MarkupTextFrag *ptf = NULL;
    long            ctf = 0;

    if (IsTagEnabled(tagSaveLBOnDump))
    {
        pLineBreaks = new CLineBreakCompat;
    }

    if (HasTextFragContext())
    {
        CMarkupTextFragContext * ptfc = GetTextFragContext();

        Assert( ptfc );

        ctf = ptfc->_aryMarkupTextFrag.Size();
        ptf = ptfc->_aryMarkupTextFrag;
    }
        
    WriteHelp(
        g_f, _T("Tree Dump For Markup: M<0d>, TreeVer=<1d>, ContentsVer=<2d>"),
        SN(), GetMarkupTreeVersion(), GetMarkupContentsVersion() );

    WriteHelp( g_f, _T(" Cch = <0d>" ), long( GetTextLength() ) );
    

    WriteString( g_f, _T("\r\n\r\n") );
    if (pLineBreaks)
        WriteString( g_f, _T("Type   Pos   Cp  Cch Brk\r\n") );
    else
        WriteString( g_f, _T("Type   Pos   Cp  Cch\r\n") );


    while (ptpCurr)
    {
        long cch = 999;
        long depth = 0;
        
        switch( ptpCurr->Type() )
        {
        case CTreePos::NodeBeg :
            WriteHelp( g_f, _T("Begin ") );
            cch = 1;
            depth = ptpCurr->Branch()->Depth();
            break;
            
        case CTreePos::NodeEnd :
            WriteHelp( g_f, _T("End   ") );
            cch = 1;
            depth = ptpCurr->Branch()->Depth();
            break;
            
        case CTreePos::Text    :
            WriteHelp( g_f, _T("Text  ") );
            cch = ptpCurr->Cch();
            depth = ptpCurr->GetInterNode()->Depth();
            break;
            
        case CTreePos::Pointer :
            cch = 0;
            depth = ptpCurr->GetInterNode()->Depth();
            WriteString( g_f, _T("Ptr ") );
            WriteString( g_f, ptpCurr->Gravity() == POINTER_GRAVITY_Left ? _T("<") : _T(">") );
            WriteString( g_f, ptpCurr->Cling() ? _T("*") : _T(" ") );
            break;
            
        default:
            AssertSz( 0, "Yikes!" );
        }

        DumpNumber( ptpCurr->_nSerialNumber );
        
        WriteHelp( g_f, _T(" ") );

        if (cch > 0)
            DumpNumber( ptpCurr->GetCp() );
        else
            WriteHelp( g_f, _T("   -") );
        
        WriteHelp( g_f, _T(" ") );
        
        if (ptpCurr->Type() == CTreePos::NodeBeg || ptpCurr->Type() == CTreePos::NodeEnd)
            WriteHelp( g_f, _T("   *") );
        else if (cch > 0 || ptpCurr->Type() == CTreePos::Text)
            DumpNumber( cch );
        else
            WriteHelp( g_f, _T("   -") );

        if (pLineBreaks)
        {
            WriteHelp( g_f, _T(" ") );
            
            if (ptpCurr->Type() == CTreePos::NodeBeg || ptpCurr->Type() == CTreePos::NodeEnd)
            {
                DWORD dwBreaks = BREAK_NONE;

                if (pLineBreaks && ptpCurr->Branch()->Tag() != ETAG_ROOT)
                {
                    tpg.MoveTo( ptpCurr, TPG_LEFT );
            
                    IGNORE_HR( pLineBreaks->QueryBreaks( & tpg, & dwBreaks ) );
                }


                WriteHelp( g_f, (dwBreaks & BREAK_BLOCK_BREAK) ? _T("B") : _T(" ") );
                WriteHelp( g_f, (dwBreaks & BREAK_SITE_BREAK)  ? _T("S") : _T(" ") );
                WriteHelp( g_f, (dwBreaks & BREAK_SITE_END)    ? _T("E") : _T(" ") );
            }
            else
            {
                WriteHelp( g_f, _T("   ") );
            }
        }
        
        WriteHelp( g_f, _T(" | ") );

        for ( int i = depth - 1 ; i > 0 ;i-- )
            WriteString( g_f, _T("  "));
        
        switch( ptpCurr->Type() )
        {
        case CTreePos::NodeBeg :
        case CTreePos::NodeEnd :
            PrintNode(
                ptpCurr->Branch(), 
                ! ptpCurr->IsBeginNode(),
                ptpCurr->IsEdgeScope()
                    ? (ptpCurr->IsBeginNode() 
                        ? NODE_TYPE_BEGIN 
                        : NODE_TYPE_END) 
                    : NODE_TYPE_NEUTRAL );

            if (CTxtPtr( this, ptpCurr->GetCp() ).GetChar() != WCH_NODE)
            {
                TCHAR ch = CTxtPtr( this, ptpCurr->GetCp() ).GetChar();
                WriteString( g_f, _T(" '") );
                WriteFormattedString( g_f, & ch, 1 );
                WriteString( g_f, _T("'") );
            }

            break;
            
        case CTreePos::Text    :
            DumpTextTreePos(ptpCurr);
            break;
            
        case CTreePos::Pointer :
                
            WriteHelp( g_f, _T("  <<=== "));

            if (ctf > 0 && ptf->_ptpTextFrag == ptpCurr)
            {
                TCHAR chBuff [ 30 ];
                long  cchTextFrag = _tcslen(ptf->_pchTextFrag);

                Assert( !ptpCurr->MarkupPointer() );

                WriteHelp( g_f, _T("TextFrag: '") );

                if (cchTextFrag <= ARRAY_SIZE( chBuff ))
                {
                    WriteFormattedString( g_f, ptf->_pchTextFrag, cchTextFrag );
                }
                else
                {
                    long cchDiv2 = ARRAY_SIZE( chBuff ) / 2;

                    _tcsncpy(chBuff, ptf->_pchTextFrag, cchDiv2);

                    WriteFormattedString( g_f, chBuff, cchDiv2 );

                    WriteString( g_f, _T( "..." ) );

                    _tcsncpy( chBuff, ptf->_pchTextFrag + cchTextFrag - cchDiv2, cchDiv2 );

                    WriteFormattedString( g_f, chBuff, cchDiv2 );
                }

                WriteHelp( g_f, _T("'") );

                ctf--;
                ptf++;
            }
            else
            {
                CMarkupPointer * pmp = ptpCurr->MarkupPointer();
            
            if (pmp)
            {
                WriteHelp( g_f, _T(" P<0d>"), pmp->SN() );

                if (pmp->CpIsCached())
                    WriteHelp( g_f, _T(" cp=<0d>, ver=<1d>"), pmp->_cpCache, pmp->_verCp );
                
                if (LPTSTR( pmp->_cstrDbgName ))
                    WriteHelp( g_f, _T(" <0s>"), LPTSTR( pmp->_cstrDbgName ) );
            }

            break;
        }
        }
        
        WriteString( g_f, _T("\r\n"));

        CMarkupPointer * pmp;

        for ( pmp = _pmpFirst ; pmp ; pmp = pmp = pmp->_pmpNext )
        {
            Assert( ! pmp->_fEmbedded );
            
            if (pmp->_ptpRef != ptpCurr)
                continue;

            WriteString( g_f, _T("ptr ") );
            WriteString( g_f, pmp->Gravity() == POINTER_GRAVITY_Left ? _T("<") : _T(">") );
            WriteString( g_f, pmp->Cling() ? _T("*") : _T(" ") );
            
            if (IsTagEnabled(tagSaveLBOnDump))
                WriteString( g_f, _T("   -    -    -     | "));
            else
                WriteString( g_f, _T("   -    -    - | "));

            for ( i = depth - 1 ; i > 0 ;i-- )
                WriteString( g_f, _T("  "));

            if (ptpCurr->IsText())
            {
                for ( i = 0 ; i < pmp->_ichRef ; i++ )
                    WriteHelp( g_f, _T(" ") );
            
                WriteHelp( g_f, _T("   ^===") );
            }
            else
            {
                WriteHelp( g_f, _T("^===") );
            }

            WriteHelp( g_f, _T(" ") );
            
            WriteHelp( g_f, _T("  P<0d>"), pmp->SN() );

            if (pmp->CpIsCached())
                WriteHelp( g_f, _T("  cp=<0d>, ver=<1d>"), pmp->_cpCache, pmp->_verCp );
            
            WriteHelp( g_f, _T(" ich=<0d>"), pmp->_ichRef );

            if (LPTSTR( pmp->_cstrDbgName ))
                WriteHelp( g_f, _T(" <0s>"), LPTSTR( pmp->_cstrDbgName ) );
            
            WriteString( g_f, _T("\r\n"));
        }

        ptpCurr = ptpCurr->NextTreePos();
    }
    
    WriteString( g_f, _T("\r\n"));

    if (pLineBreaks)
        delete pLineBreaks;
}

void WriteXMLTagBegin( HANDLE hFile, int &nIndent, TCHAR *pchTagName, BOOL fCR, TCHAR *pchFormat, ...);
void WriteXMLTagEnd( HANDLE hFile, int &nIndent, TCHAR *pchTagName, BOOL fDoIndent = FALSE, BOOL fCR = TRUE );
void WriteXMLNoScope( HANDLE hFile, int &nIndex, TCHAR *pchTagName, TCHAR *pchFormat, ... );
void WriteXMLElement( HANDLE hFile, int &nIndent, TCHAR *pchTagName, TCHAR *pchFormat, ... );
void WriteXMLFormattedString( HANDLE hFile, TCHAR * pch, long cch );

void 
WriteXMLTagBegin( HANDLE hFile, int &nIndent, TCHAR *pchTagName, BOOL fCR, TCHAR *pchFormat, ...)
{
    va_list arg;

    va_start( arg, pchFormat );

    WriteChar( hFile, _T(' '), nIndent );
    WriteHelp( hFile, _T("<<<0s>"), pchTagName );

    if (pchFormat)
    {
        WriteChar( hFile, _T(' ') );
        WriteHelpV( hFile, pchFormat, &arg );
    }

    WriteChar( hFile, _T('>') );
    
    if (fCR)
        WriteString( hFile, _T("\r\n") );

    nIndent++;
}

void 
WriteXMLTagEnd( HANDLE hFile, int &nIndent, TCHAR *pchTagName, BOOL fDoIndent, BOOL fCR)
{
    nIndent--;

    if (fDoIndent)
        WriteChar( hFile, _T(' '), nIndent );

    WriteHelp( hFile, _T("<</<0s>>"), pchTagName );
    
    if (fCR)
        WriteString( hFile, _T("\r\n") );
}

void 
WriteXMLNoScope( HANDLE hFile, int &nIndent, TCHAR *pchTagName, TCHAR *pchFormat, ... )
{
    va_list arg;

    va_start( arg, pchFormat );

    WriteChar( hFile, _T(' '), nIndent );
    WriteHelp( hFile, _T("<<<0s>"), pchTagName );

    if (pchFormat)
    {
        WriteChar( hFile, _T(' ') );
        WriteHelpV( hFile, pchFormat, &arg );
    }
    
    WriteString( hFile, _T("/>\r\n") );
}


void 
WriteXMLElement( HANDLE hFile, int &nIndent, TCHAR *pchTagName, TCHAR *pchFormat, ... )
{
    va_list arg;

    va_start( arg, pchFormat );

    WriteXMLTagBegin( hFile, nIndent, pchTagName, FALSE, NULL );

    if (pchFormat)
        WriteHelpV( hFile, pchFormat, &arg );

    WriteXMLTagEnd( hFile, nIndent, pchTagName );
}

void 
WriteXMLFormattedString( HANDLE hFile, TCHAR * pch, long cch )
{
    if (!pch)
        return;

    for ( int i = 0 ; i < cch ; i++ )
    {
        TCHAR ch = pch[i];

        if (ch >= 1 && ch <= 26)
        {
            if (ch == _T('\r'))
                WriteString( hFile,  _T("\\r"));
            else if (ch == _T('\n'))
                WriteString( hFile, _T("\\n"));
            else
            {
                WriteHelp( hFile, _T("[<0d>]"), (long)int(ch) );
            }
        }
        else
        {
            switch ( ch )
            {
            case 0 :
                WriteString( hFile, _T("[NULL]"));
                break;

            case _T('<'):
                WriteString( hFile, _T("&lt;"));
                break;

            case _T('>'):
                WriteString( hFile, _T("&gt;"));
                break;

            case _T('&'):
                WriteString( hFile, _T("&amp;"));
                break;

            case WCH_NODE:
                WriteString( hFile, _T("[Node]"));
                break;

            case WCH_NBSP :
                WriteString( hFile, _T("[NBSP]"));
                break;

            default :
                if (ch < 256 && _istprint(ch))
                {
                    WriteChar(hFile, ch);
                }
                else
                {
                    TCHAR achHex[9];

                    Format( 0, achHex, ARRAY_SIZE(achHex), _T("<0x>"), ch);

                    StrCpy( achHex, TEXT("[U+") );
                    StrCpy( achHex + 3, achHex + 4 );
                    StrCpy( achHex + 7, TEXT("]") );

                    WriteString( hFile, achHex );
                }

                break;
            }
        }
    }
}

void
CMarkup::DumpTreeInXML ( )
{
    HANDLE  hFile = 0;
    int     nIndent = 0;

    // BUGBUG I don't handle unembeded pointers yet.
    Verify( ! EmbedPointers() );

    hFile = CreateFile(
            _T("c:\\tridump.xml"),
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        AssertSz(0, "CreateFile failed for DumpTreeInXML");
        return;
    }

    // Write out the root XML element
    WriteXMLTagBegin( hFile, nIndent, _T("Markup"), TRUE, NULL );

    WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), SN() );
    WriteXMLElement( hFile, nIndent, _T("Length"), _T("<0d>"), long( GetTextLength() ) );

    // Walk the tree, dumping info for each tree pos
    WriteXMLTagBegin( hFile, nIndent, _T("TreePoses"), TRUE, NULL );

    {
        CTreePos * ptp;

        ptp = FirstTreePos();

        while(ptp)
        {
            switch (ptp->Type())
            {
            case CTreePos::NodeBeg:
            case CTreePos::NodeEnd:
                {
                    TCHAR * pchType;

                    if (ptp->IsBeginNode())
                        pchType = _T("NodeBeg");
                    else
                        pchType = _T("NodeEnd");

                    WriteXMLTagBegin( hFile, nIndent, pchType, TRUE, NULL );

                    WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), ptp->SN() );
                    if (ptp->IsEdgeScope())
                        WriteXMLNoScope( hFile, nIndent, _T("Edge"), NULL );
                    WriteXMLElement( hFile, nIndent, _T("NodeSN"), _T("<0d>"), ptp->Branch()->SN() );

                    WriteXMLTagEnd( hFile, nIndent, pchType, TRUE );
                }

                break;
            case CTreePos::Text:
                WriteXMLTagBegin( hFile, nIndent, _T("Text"), TRUE, NULL );

                WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), ptp->SN() );
                WriteXMLElement( hFile, nIndent, _T("Length"), _T("<0d>"), ptp->Cch() );
                WriteXMLElement( hFile, nIndent, _T("ScriptID"), _T("<0s>"), SidName( ptp->Sid() ) );
                if (ptp->TextID())
                    WriteXMLElement( hFile, nIndent, _T("TextID"), _T("<0d>"), ptp->TextID() );

                WriteXMLTagBegin( hFile, nIndent, _T("Content"), FALSE, NULL );
                {
                    TCHAR * pch;
                    long    cch = ptp->Cch();
                    long    cp = ptp->GetCp();
                    CTxtPtr tp(this, cp);

                    pch = (TCHAR*)MemAlloc(Mt(Mem), sizeof(TCHAR)*cch);
                    Assert(pch);

                    Assert( cch == tp.GetRawText( cch, pch ) );

                    WriteXMLFormattedString( hFile, pch, cch );

                    MemFree( pch );
                }

                WriteXMLTagEnd( hFile, nIndent, _T("Content"), FALSE );

                WriteXMLTagEnd( hFile, nIndent, _T("Text"), TRUE );

                break;
            case CTreePos::Pointer:
                {
                    CMarkupPointer * pmp = ptp->MarkupPointer();
                    LPTSTR pchDbgName = LPTSTR( pmp->_cstrDbgName );

                    WriteXMLTagBegin( hFile, nIndent, _T("Pointer"), TRUE, NULL );

                    WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), ptp->SN() );
                    WriteXMLElement( hFile, nIndent, _T("Gravity"), _T("<0s>"), ptp->Gravity() ? _T("Right") : _T("Left") );
                    if (ptp->Cling())
                        WriteXMLNoScope( hFile, nIndent, _T("Cling"), NULL );
                    if (pchDbgName)
                        WriteXMLElement( hFile, nIndent, _T("DebugName"), _T("<0s>"), pchDbgName );

                    WriteXMLTagEnd( hFile, nIndent, _T("Pointer"), TRUE );
                }
                break;
            }

            ptp = ptp->NextTreePos();
        }
    }
    WriteXMLTagEnd( hFile, nIndent, _T("TreePoses"), TRUE );

    // Walk the tree, dumping info for each tree node
    WriteXMLTagBegin( hFile, nIndent, _T("TreeNodes"), TRUE, NULL );
    {
        CTreePos * ptp;

        ptp = FirstTreePos();

        while(ptp)
        {
            if (ptp->IsBeginNode())
            {
                CTreeNode * pNode = ptp->Branch();
                CElement * pElement = pNode->Element();

                WriteXMLTagBegin( hFile, nIndent, _T("Node"), TRUE, NULL );

                WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), pNode->SN() );
                if (!pNode->IsLastBranch())
                    WriteXMLElement( hFile, nIndent, _T("NextBranchSN"), _T("<0d>"), pNode->NextBranch()->SN() );
                if (!pNode->IsFirstBranch())
                    WriteXMLElement( hFile, nIndent, _T("PreviousBranchSN"), _T("<0d>"), pNode->PreviousBranch()->SN() );
                if (pNode->Parent())
                    WriteXMLElement( hFile, nIndent, _T("ParentSN"), _T("<0d>"), pNode->Parent()->SN() );
                if (pNode->_iCF != -1)
                    WriteXMLElement( hFile, nIndent, _T("CharFormat"), _T("<0d>"), pNode->_iCF );
                if (pNode->_iPF != -1)
                    WriteXMLElement( hFile, nIndent, _T("ParaFormat"), _T("<0d>"), pNode->_iPF );
                if (pNode->_iFF != -1)
                    WriteXMLElement( hFile, nIndent, _T("FancyFormat"), _T("<0d>"), pNode->_iFF );
                WriteXMLElement( hFile, nIndent, _T("ElementSN"), _T("<0d>"), pElement->SN() );
                WriteXMLElement( hFile, nIndent, _T("BeginPosSN"), _T("<0d>"), pNode->GetBeginPos()->SN() );
                WriteXMLElement( hFile, nIndent, _T("EndPosSN"), _T("<0d>"), pNode->GetEndPos()->SN() );

                WriteXMLTagEnd( hFile, nIndent, _T("Node"), TRUE );
            }

            ptp = ptp->NextTreePos();
        }
    }
    WriteXMLTagEnd( hFile, nIndent, _T("TreeNodes"), TRUE );

    // Walk the tree, dumping info for each element
    WriteXMLTagBegin( hFile, nIndent, _T("Elements"), TRUE, NULL );
    {
        CTreePos * ptp;

        ptp = FirstTreePos();

        while(ptp)
        {
            if (ptp->IsBeginElementScope())
            {
                CTreeNode * pNode = ptp->Branch();
                CElement * pElement = pNode->Element();

                WriteXMLTagBegin( hFile, nIndent, _T("Element"), TRUE, NULL );

                WriteXMLElement( hFile, nIndent, _T("SN"), _T("<0d>"), pElement->SN() );
                WriteXMLElement( hFile, nIndent, _T("TagName"), _T("<0s>"), pElement->TagName() );
                if (pElement->Namespace())
                    WriteXMLElement( hFile, nIndent, _T("Namespace"), _T("<0s>"), pElement->Namespace() );
                if (pElement->Tag() == ETAG_UNKNOWN)
                    WriteXMLNoScope( hFile, nIndent, _T("Unknown"), NULL );
                if (pElement->_fBreakOnEmpty)
                    WriteXMLNoScope( hFile, nIndent, _T("BreakOnEmpty"), NULL );

                WriteXMLTagBegin( hFile, nIndent, _T("ElementNodes"), TRUE, NULL );
                {
                    CTreeNode * pNode = pElement->GetFirstBranch();
                    while( pNode )
                    {
                        WriteXMLElement( hFile, nIndent, _T("NodeSN"), _T("<0d>"), pNode->SN() );
                        pNode = pNode->NextBranch();
                    }
                }
                WriteXMLTagEnd( hFile, nIndent, _T("ElementNodes"), TRUE );
                

                WriteXMLTagEnd( hFile, nIndent, _T("Element"), TRUE );
            }

            ptp = ptp->NextTreePos();
        }
    }
    WriteXMLTagEnd( hFile, nIndent, _T("Elements"), TRUE );

    // Write out the end of the root XML element
    WriteXMLTagEnd( hFile, nIndent, _T("Markup"), TRUE );

    CloseHandle( hFile );    
}

void
CMarkup::DumpTextChanges ( )
{
    if (!InitDumpFile())
        return;

    WriteString( g_f, _T("Text Change Dump For ped: " ) );

    WriteHelp( g_f, _T("Tree Dump For Markup: M<0d>"), SN() );

    //
    //  BUGBUG: Do this through a DEBUG only notification sent through the tree
    //  BUGBUG: Make this routine into "dump dirty ranges" and have all things
    //          that track ranges respond (e.g., collections, CFlowLayout).
    //

    WriteString( g_f, _T("Unimplemented"));

    WriteString( g_f, _T("\r\n\r\n") );

    CloseDumpFile();
}

void
CMarkup::DumpSplayTree(CTreePos *ptpStart, CTreePos *ptpFinish)
{
    if (!InitDumpFile())
        return;

    if (ptpStart == NULL)
        ptpStart = FirstTreePos();
    if (ptpFinish == NULL)
        ptpFinish = LastTreePos();

    CTreePos *ptpCurr = ptpStart;
    long cDepth = ptpCurr->Depth();

    // for each TreePos in the desired range...
    for (;;)
    {
        // print structure
        for (long i=0; i<cDepth-1; ++i)
            WriteString(g_f, _T("  "));

        if (cDepth > 0)
            WriteString(g_f, ptpCurr->IsLeftChild() ? _T("v ") : _T("^ "));

        // print content
        switch (ptpCurr->Type())
        {
        case CTreePos::NodeBeg:
        case CTreePos::NodeEnd:
            WriteChar(g_f, _T('N'));
            WriteChar(g_f, ptpCurr->IsBeginNode() ? _T('+') : _T('-'));
            if (ptpCurr->IsEdgeScope())
                WriteChar(g_f, _T('*'));
            WriteHelp(g_f, _T(" <0x>"), ptpCurr->Branch());
            break;
        case CTreePos::Text:
            WriteChar(g_f, _T('T'));
            WriteHelp(g_f, _T("<0d> "), ptpCurr->Cch());
            break;
        case CTreePos::Pointer:
            WriteChar(g_f, _T('P'));
            break;
        }

        // finish printing
        WriteHelp(g_f, _T("  <0x>\r\n"), ptpCurr);

        // terminate loop
        if (ptpCurr == ptpFinish)
            break;

        // advance to next TreePos
        CTreePos *ptpChild = ptpCurr->RightChild();
        if (ptpChild)
        {
            while (ptpChild)
            {
                ++ cDepth;
                ptpCurr = ptpChild;
                ptpChild = ptpCurr->LeftChild();
            }
        }
        else
        {
            while (!ptpCurr->IsLeftChild())
            {
                -- cDepth;
                ptpCurr = ptpCurr->Parent();
            }
            -- cDepth;
            ptpCurr = ptpCurr->Parent();
        }
    }

    CloseDumpFile();
}
#endif


#if DBG==1

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

void
CMarkup::DumpDisplayTree()
{
    Doc()->GetView()->DumpDisplayTree();
}
#endif
