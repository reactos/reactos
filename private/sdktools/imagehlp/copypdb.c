BOOL
CopyPdb(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate
    )
{
    LONG ErrorCode;
    ULONG Sig = 0;
    char ErrorString[1024];
    BOOL rc;
    PDB * pSrcPdb;
    HINSTANCE hMsPdb;

    // Add a short circut.  PdbCopy fails miserably if the source and destination are the same.
    // If StripPrivate isn't set, check for this case and just return.  If StripPrivate is set,
    // bummer.

    if (!StripPrivate) {
        if (!_stricmp(szSrcPdb, szDestPdb)) {
            rc = TRUE;
        } else {
            rc = CopyFile(szSrcPdb, szDestPdb, FALSE);
        }
    } else {
        rc = PDBOpen((char *)szSrcPdb, "r", Sig, &ErrorCode, ErrorString, &pSrcPdb);
        if (rc) {
            rc = DeleteFile(szDestPdb);
            if (rc || (GetLastError() == ERROR_FILE_NOT_FOUND)) {
                rc = PDBCopyTo(pSrcPdb, szDestPdb, StripPrivate ? 0x00000001 : 0x0000000, 0);
            }
            if (!rc) {
                // PdbCopyTo doesn't cleanup on failure.  Do it here.
                DeleteFile(szDestPdb);
            }
            PDBClose(pSrcPdb);
        }
    }

    return(rc);
}
