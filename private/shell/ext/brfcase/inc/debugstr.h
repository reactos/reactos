/*
 * debugstr.h - Debug message strings.
 */


#ifdef DEBUG

/* TWINRESULT strings */

/*
 * N.b., this array of strings must match the TWINRESULTs defined in synceng.h.
 * The index of the pointer to the string corresponding to a TWINRESULT tr may
 * be determined as rgcpcszTwinResult[tr].
 */

CONST LPCTSTR rgcpcszTwinResult[] =
{
   TEXT("TR_SUCCESS"),
   TEXT("TR_RH_LOAD_FAILED"),
   TEXT("TR_SRC_OPEN_FAILED"),
   TEXT("TR_SRC_READ_FAILED"),
   TEXT("TR_DEST_OPEN_FAILED"),
   TEXT("TR_DEST_WRITE_FAILED"),
   TEXT("TR_ABORT"),
   TEXT("TR_UNAVAILABLE_VOLUME"),
   TEXT("TR_OUT_OF_MEMORY"),
   TEXT("TR_FILE_CHANGED"),
   TEXT("TR_DUPLICATE_TWIN"),
   TEXT("TR_DELETED_TWIN"),
   TEXT("TR_HAS_FOLDER_TWIN_SRC"),
   TEXT("TR_INVALID_PARAMETER"),
   TEXT("TR_REENTERED"),
   TEXT("TR_SAME_FOLDER"),
   TEXT("TR_SUBTREE_CYCLE_FOUND"),
   TEXT("TR_NO_MERGE_HANDLER"),
   TEXT("TR_MERGE_INCOMPLETE"),
   TEXT("TR_TOO_DIFFERENT"),
   TEXT("TR_BRIEFCASE_LOCKED"),
   TEXT("TR_BRIEFCASE_OPEN_FAILED"),
   TEXT("TR_BRIEFCASE_READ_FAILED"),
   TEXT("TR_BRIEFCASE_WRITE_FAILED"),
   TEXT("TR_CORRUPT_BRIEFCASE"),
   TEXT("TR_NEWER_BRIEFCASE"),
   TEXT("TR_NO_MORE")
};

/* CREATERECLISTPROCMSG strings */

/*
 * N.b., this array of strings must match the CREATERECLISTPROCs defined in
 * synceng.h.  The index of the pointer to the string corresponding to a
 * CREATERECLISTPROCMSG crlpm may be determined as
 * rgcpcszCreateRecListMsg[crlpm].
 */

const LPCTSTR rgcpcszCreateRecListMsg[] =
{
   TEXT("CRLS_BEGIN_CREATE_REC_LIST"),
   TEXT("CRLS_DELTA_CREATE_REC_LIST"),
   TEXT("CRLS_END_CREATE_REC_LIST")
};

/* RECSTATUSPROCMSGs strings */

/*
 * N.b., this array of strings must match the RECSTATUSPROCMSGs defined in
 * synceng.h.  The index of the pointer to the string corresponding to a
 * RECSTATUSPROCMSG rspm may be determined as rgcpcszRecStatusMsg[rspm].
 */

CONST LPCTSTR rgcpcszRecStatusMsg[] =
{
   TEXT("RS_BEGIN_COPY"),
   TEXT("RS_DELTA_COPY"),
   TEXT("RS_END_COPY"),
   TEXT("RS_BEGIN_MERGE"),
   TEXT("RS_DELTA_MERGE"),
   TEXT("RS_END_MERGE"),
   TEXT("RS_BEGIN_DELETE"),
   TEXT("RS_DELTA_DELETE"),
   TEXT("RS_END_DELETE")
};

#endif

