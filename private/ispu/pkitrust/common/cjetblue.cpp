//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cJetBlue.cpp
//
//  Contents:   Microsoft Internet Security Common
//
//  History:    23-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cjetblue.hxx"

#define     CHK_FUNC(fp, nm)  this->CheckOrLoadFunc((void **)&fp, nm)

cJetBlue_::cJetBlue_(void)
{
    SetErrorMode(SEM_NOOPENFILEERRORBOX);

    hJet = LoadLibrary("ESE.DLL");
                                    
    fp_JetInit                      = NULL;
    fp_JetTerm                      = NULL;
    fp_JetSetSystemParameter        = NULL;
    fp_JetBeginSession              = NULL;
    fp_JetCreateDatabase            = NULL;
    fp_JetAttachDatabase            = NULL;
    fp_JetDetachDatabase            = NULL;
    fp_JetCreateTable               = NULL;
    fp_JetCreateTableColumnIndex    = NULL;
    fp_JetCloseDatabase             = NULL;
    fp_JetCloseTable                = NULL;
    fp_JetOpenDatabase              = NULL;
    fp_JetOpenTable                 = NULL;
    fp_JetBeginTransaction          = NULL;
    fp_JetCommitTransaction         = NULL;
    fp_JetRetrieveColumns           = NULL;
    fp_JetSetColumns                = NULL;
    fp_JetPrepareUpdate             = NULL;
    fp_JetSetCurrentIndex2          = NULL;
    fp_JetMove                      = NULL;
    fp_JetMakeKey                   = NULL;
    fp_JetSeek                      = NULL;

}

cJetBlue_::~cJetBlue_(void)
{
    if (hJet)
    {
        FreeLibrary(hJet);
    }
}

BOOL cJetBlue_::CheckOrLoadFunc(void **fp, char *pszfunc)
{
    if (*fp)
    {
        return(TRUE);
    }

    if (this->hJet)
    {
        *fp = GetProcAddress(this->hJet, pszfunc);
    }

    return((*fp) ? TRUE : FALSE);
}

JET_ERR cJetBlue_::JetInit(JET_INSTANCE *pinstance)    
{ 
    if (!(CHK_FUNC(fp_JetInit, "JetInit")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetInit)(pinstance)); 
}

JET_ERR cJetBlue_::JetTerm(JET_INSTANCE instance)      
{ 
    if (!(CHK_FUNC(fp_JetTerm, "JetTerm")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetTerm)(instance)); 
}

JET_ERR cJetBlue_::JetSetSystemParameter(JET_INSTANCE *pinstance, JET_SESID sesid, 
                            unsigned long paramid, unsigned long lParam, 
                            const char *sz)
{ 
    if (!(CHK_FUNC(fp_JetSetSystemParameter, "JetSetSystemParameter")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetSetSystemParameter)(pinstance, sesid, paramid, lParam, sz)); 
}

JET_ERR cJetBlue_::JetBeginSession(JET_INSTANCE instance, JET_SESID *psesid,
                        const char *szUserName, const char *szPassword)
{ 
    if (!(CHK_FUNC(fp_JetBeginSession, "JetBeginSession")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetBeginSession)(instance, psesid, szUserName, szPassword)); 
}

JET_ERR cJetBlue_::JetCreateDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect,
                        JET_DBID *pdbid, JET_GRBIT grbit)
{ 
    if (!(CHK_FUNC(fp_JetCreateDatabase, "JetCreateDatabase")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetCreateDatabase)(sesid, szFilename, szConnect, pdbid, grbit)); 
}

JET_ERR cJetBlue_::JetAttachDatabase(JET_SESID sesid, const char *szFilename, JET_GRBIT grbit)
{ 
    if (!(CHK_FUNC(fp_JetAttachDatabase, "JetAttachDatabase")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetAttachDatabase)(sesid, szFilename, grbit)); 
}

JET_ERR cJetBlue_::JetDetachDatabase(JET_SESID sesid, const char *szFilename)
{ 
    if (!(CHK_FUNC(fp_JetDetachDatabase, "JetDetachDatabase")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetDetachDatabase)(sesid, szFilename)); 
}

JET_ERR cJetBlue_::JetCreateTable(JET_SESID sesid, JET_DBID dbid, const char *szTableName, 
                                  unsigned long lPages, unsigned long lDensity, JET_TABLEID *ptableid)
{ 
    if (!(CHK_FUNC(fp_JetCreateTable, "JetCreateTable")))
    {
        return(JET_wrnNyi);
    }

    return((*fp_JetCreateTable)(sesid, dbid, szTableName, lPages, lDensity, ptableid)); 
}

JET_ERR cJetBlue_::JetCreateTableColumnIndex(JET_SESID sesid, JET_DBID dbid,
                                JET_TABLECREATE *ptablecreate)
                { return((*fp_JetCreateTableColumnIndex)(sesid, dbid, ptablecreate)); }
JET_ERR cJetBlue_::JetCloseDatabase(JET_SESID sesid, JET_DBID dbid, JET_GRBIT grbit)
                { return((*fp_JetCloseDatabase)(sesid, dbid, grbit)); }
JET_ERR cJetBlue_::JetCloseTable(JET_SESID sesid, JET_TABLEID tableid)
                { return((*fp_JetCloseTable)(sesid, tableid)); }
JET_ERR cJetBlue_::JetOpenDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect, 
                         JET_DBID *pdbid, JET_GRBIT grbit)
                { return((*fp_JetOpenDatabase)(sesid, szFilename, szConnect, 
                                                pdbid, grbit)); }
JET_ERR cJetBlue_::JetOpenTable(JET_SESID sesid, JET_DBID dbid, const char *szTableName, 
                   const void *pvParameters, unsigned long cbParameters, 
                   JET_GRBIT grbit, JET_TABLEID *ptableid)
                { return((*fp_JetOpenTable)(sesid, dbid, szTableName, pvParameters, 
                                            cbParameters, grbit, ptableid)); }
JET_ERR cJetBlue_::JetBeginTransaction(JET_SESID sesid)
                { return((*fp_JetBeginTransaction)(sesid)); }
JET_ERR cJetBlue_::JetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit)
                { return((*fp_JetCommitTransaction)(sesid, grbit)); }
JET_ERR cJetBlue_::JetRetrieveColumns(JET_SESID sesid, JET_TABLEID tableid, 
                            JET_RETRIEVECOLUMN *pretrievecolumn, 
                            unsigned long cretrievecolumn)
                { return((*fp_JetRetrieveColumns)(sesid, tableid, pretrievecolumn, 
                                                  cretrievecolumn)); }
JET_ERR cJetBlue_::JetSetColumns(JET_SESID sesid, JET_TABLEID tableid, JET_SETCOLUMN *psetcolumn, 
                       unsigned long csetcolumn)
                { return((*fp_JetSetColumns)(sesid, tableid, psetcolumn, csetcolumn)); }
JET_ERR cJetBlue_::JetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid, unsigned long prep)
                { return((*fp_JetPrepareUpdate)(sesid, tableid, prep)); }
JET_ERR cJetBlue_::JetSetCurrentIndex2(JET_SESID sesid, JET_TABLEID tableid,	const char *szIndexName, 
                             JET_GRBIT grbit)
                { return((*fp_JetSetCurrentIndex2)(sesid, tableid, szIndexName, grbit)); }
JET_ERR cJetBlue_::JetMove(JET_SESID sesid, JET_TABLEID tableid,	long cRow, JET_GRBIT grbit)
                { return((*fp_JetMove)(sesid, tableid, cRow, grbit)); }
JET_ERR cJetBlue_::JetMakeKey(JET_SESID sesid, JET_TABLEID tableid, const void *pvData, 
                    unsigned long cbData, JET_GRBIT grbit)
                { return((*fp_JetMakeKey)(sesid, tableid, pvData, cbData, grbit)); }
JET_ERR cJetBlue_::JetSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit)
                        { return((*fp_JetSeek)(sesid, tableid, grbit)); }


