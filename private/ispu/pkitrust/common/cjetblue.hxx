//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cjetblue.hxx
//
//  Contents:   Microsoft Internet Security Common
//
//  History:    23-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CJETBLUE_HXX
#define CJETBLUE_HXX

#include    "jet.h"


typedef JET_ERR     (JET_API *td_JetInit)(JET_INSTANCE *pinstance);
typedef JET_ERR     (JET_API *td_JetTerm)(JET_INSTANCE instance);
typedef JET_ERR     (JET_API *td_JetSetSystemParameter)(JET_INSTANCE *pinstance, JET_SESID sesid, unsigned long paramid,
	                                                    unsigned long lParam, const char *sz);
typedef JET_ERR     (JET_API *td_JetBeginSession)(  JET_INSTANCE instance, JET_SESID *psesid, const char *szUserName, 
                                                    const char *szPassword);
typedef JET_ERR     (JET_API *td_JetCreateDatabase)(JET_SESID sesid, const char *szFilename, const char *szConnect,
	                                                JET_DBID *pdbid, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetAttachDatabase)(JET_SESID sesid, const char *szFilename, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetDetachDatabase)(JET_SESID sesid, const char *szFilename);
typedef JET_ERR     (JET_API *td_JetCreateTable)(JET_SESID sesid, JET_DBID dbid, const char *szTableName, 
                                                 unsigned long lPages, unsigned long lDensity, JET_TABLEID *ptableid);
typedef JET_ERR     (JET_API *td_JetCreateTableColumnIndex)(JET_SESID sesid, JET_DBID dbid, JET_TABLECREATE *ptablecreate);

typedef JET_ERR     (JET_API *td_JetCloseDatabase)(JET_SESID sesid, JET_DBID dbid, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetCloseTable)(JET_SESID sesid, JET_TABLEID tableid);
typedef JET_ERR     (JET_API *td_JetOpenDatabase)(JET_SESID sesid, const char *szFilename, const char *szConnect, 
                                                  JET_DBID *pdbid, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetOpenTable)(JET_SESID sesid, JET_DBID dbid, const char *szTableName, 
                                               const void *pvParameters, unsigned long cbParameters, 
                                               JET_GRBIT grbit, JET_TABLEID *ptableid);

typedef JET_ERR     (JET_API *td_JetBeginTransaction)(JET_SESID sesid);
typedef JET_ERR     (JET_API *td_JetCommitTransaction)(JET_SESID sesid, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetRetrieveColumns)(JET_SESID sesid, JET_TABLEID tableid, 
                                                     JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn);
typedef JET_ERR     (JET_API *td_JetSetColumns)(JET_SESID sesid, JET_TABLEID tableid, JET_SETCOLUMN *psetcolumn, 
                                                unsigned long csetcolumn);
typedef JET_ERR     (JET_API *td_JetPrepareUpdate)(JET_SESID sesid, JET_TABLEID tableid, unsigned long prep);
typedef JET_ERR     (JET_API *td_JetSetCurrentIndex2)(JET_SESID sesid, JET_TABLEID tableid,	const char *szIndexName, 
                                                      JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetMove)(JET_SESID sesid, JET_TABLEID tableid,	long cRow, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetMakeKey)(JET_SESID sesid, JET_TABLEID tableid, const void *pvData, 
                                             unsigned long cbData, JET_GRBIT grbit);
typedef JET_ERR     (JET_API *td_JetSeek)(JET_SESID sesid, JET_TABLEID tableid,	JET_GRBIT grbit);


class cJetBlue_
{
    public:
        cJetBlue_(void);
        virtual ~cJetBlue_(void);

    protected:
        JET_ERR                     JetInit(JET_INSTANCE *pinstance);
        JET_ERR                     JetTerm(JET_INSTANCE instance);
        JET_ERR                     JetSetSystemParameter(JET_INSTANCE *pinstance, JET_SESID sesid, 
                                                        unsigned long paramid, unsigned long lParam, 
                                                        const char *sz);
        JET_ERR                     JetBeginSession(JET_INSTANCE instance, JET_SESID *psesid,
	                                                const char *szUserName, const char *szPassword);
        JET_ERR                     JetCreateDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect,
	                                                JET_DBID *pdbid, JET_GRBIT grbit);
        JET_ERR                     JetAttachDatabase(JET_SESID sesid, const char *szFilename, JET_GRBIT grbit);
        JET_ERR                     JetDetachDatabase(JET_SESID sesid, const char *szFilename);
        JET_ERR                     JetCreateTable(JET_SESID sesid, JET_DBID dbid,
                                                    const char *szTableName, unsigned long lPages, unsigned long lDensity,
                                                    JET_TABLEID *ptableid);
        JET_ERR                     JetCreateTableColumnIndex(JET_SESID sesid, JET_DBID dbid,
	                                                        JET_TABLECREATE *ptablecreate);
        JET_ERR                     JetCloseDatabase(JET_SESID sesid, JET_DBID dbid, JET_GRBIT grbit);
        JET_ERR                     JetCloseTable(JET_SESID sesid, JET_TABLEID tableid);
        JET_ERR                     JetOpenDatabase(JET_SESID sesid, const char *szFilename, const char *szConnect, 
                                                     JET_DBID *pdbid, JET_GRBIT grbit);
        JET_ERR                     JetOpenTable(JET_SESID sesid, JET_DBID dbid, const char *szTableName, 
                                               const void *pvParameters, unsigned long cbParameters, 
                                               JET_GRBIT grbit, JET_TABLEID *ptableid);
        JET_ERR                     JetBeginTransaction(JET_SESID sesid);
        JET_ERR                     JetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);
        JET_ERR                     JetRetrieveColumns(JET_SESID sesid, JET_TABLEID tableid, 
                                                        JET_RETRIEVECOLUMN *pretrievecolumn, 
                                                        unsigned long cretrievecolumn);
        JET_ERR                     JetSetColumns(JET_SESID sesid, JET_TABLEID tableid, JET_SETCOLUMN *psetcolumn, 
                                                   unsigned long csetcolumn);
        JET_ERR                     JetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid, unsigned long prep);
        JET_ERR                     JetSetCurrentIndex2(JET_SESID sesid, JET_TABLEID tableid,	const char *szIndexName, 
                                                         JET_GRBIT grbit);
        JET_ERR                     JetMove(JET_SESID sesid, JET_TABLEID tableid,	long cRow, JET_GRBIT grbit);
        JET_ERR                     JetMakeKey(JET_SESID sesid, JET_TABLEID tableid, const void *pvData, 
                                                unsigned long cbData, JET_GRBIT grbit);
        JET_ERR                     JetSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit);

    private:
        HINSTANCE                       hJet;

        td_JetInit                      fp_JetInit;
        td_JetTerm                      fp_JetTerm;
        td_JetSetSystemParameter        fp_JetSetSystemParameter;
        td_JetBeginSession              fp_JetBeginSession;
        td_JetCreateDatabase            fp_JetCreateDatabase;
        td_JetAttachDatabase            fp_JetAttachDatabase;
        td_JetDetachDatabase            fp_JetDetachDatabase;
        td_JetCreateTable               fp_JetCreateTable;
        td_JetCreateTableColumnIndex    fp_JetCreateTableColumnIndex;
        td_JetCloseDatabase             fp_JetCloseDatabase;
        td_JetCloseTable                fp_JetCloseTable;
        td_JetOpenDatabase              fp_JetOpenDatabase;
        td_JetOpenTable                 fp_JetOpenTable;
        td_JetBeginTransaction          fp_JetBeginTransaction;
        td_JetCommitTransaction         fp_JetCommitTransaction;
        td_JetRetrieveColumns           fp_JetRetrieveColumns;
        td_JetSetColumns                fp_JetSetColumns;
        td_JetPrepareUpdate             fp_JetPrepareUpdate;
        td_JetSetCurrentIndex2          fp_JetSetCurrentIndex2;
        td_JetMove                      fp_JetMove;
        td_JetMakeKey                   fp_JetMakeKey;
        td_JetSeek                      fp_JetSeek;

        BOOL                            CheckOrLoadFunc(void **fp, char *pszfunc);
};

#endif // CJETBLUE_HXX
