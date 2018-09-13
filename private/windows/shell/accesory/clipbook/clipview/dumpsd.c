



#ifdef DEBUG


/*
 *      HexDumpBytes
 */

void HexDumpBytes(
    char        *pv,
    unsigned    cb)
{
char        achHex[]="0123456789ABCDEF";
char        achOut[80];
unsigned    iOut;



    iOut = 0;

    while (cb)
        {
        if (iOut >= 78)
            {
            PINFO(achOut);
            iOut = 0;
            }

        achOut[iOut++] = achHex[(*pv >> 4) & 0x0f];
        achOut[iOut++] = achHex[*pv++ & 0x0f];
        achOut[iOut]   = '\0';
        cb--;
        }


    if (iOut)
        {
        PINFO(achOut);
        }

}





/*
 *      PrintSid
 */

void PrintSid(
    PSID    sid)
{
DWORD   cSubAuth;
DWORD   i;

    PINFO(TEXT("\r\nSID: "));

    if (sid)
        {
        HexDumpBytes((char *)GetSidIdentifierAuthority(sid), sizeof(SID_IDENTIFIER_AUTHORITY));

        SetLastError(0);
        cSubAuth = *GetSidSubAuthorityCount(sid);
        if (GetLastError())
            {
            PINFO(TEXT("Invalid SID\r\n"));
            }
        else
            {
            for (i = 0;i < cSubAuth; i++)
                {
                PINFO(TEXT("-"));
                HexDumpBytes((char *)GetSidSubAuthority(sid, i), sizeof(DWORD));
                }
            PINFO(TEXT("\r\n"));
            }
        }
    else
        {
        PINFO(TEXT("NULL SID\r\n"));
        }

}






/*
 *      PrintAcl
 *
 *  Purpose: Print out the entries in an access-control list.
 */

void PrintAcl(
    PACL    pacl)
{
ACL_SIZE_INFORMATION    aclsi;
ACCESS_ALLOWED_ACE      *pace;
unsigned                i;


    if (pacl)
        {
        if (GetAclInformation (pacl, &aclsi, sizeof(aclsi), AclSizeInformation))
            {
            for (i = 0;i < aclsi.AceCount;i++)
                {
                GetAce(pacl, i, &pace);

                PINFO(TEXT("Type(%x) Flags(%x) Access(%lx)\r\nSID:"),
                      (int)pace->Header.AceType,
                      (int)pace->Header.AceFlags,
                      pace->Mask);
                PrintSid((PSID)&(pace->SidStart));
                }
            }
        }
    else
        {
        PINFO(TEXT("NULL PACL\r\n"));
        }

}






/*
 *      PrintSD
 */

void PrintSD(
    PSECURITY_DESCRIPTOR    pSD)
{
DWORD   dwRev;
WORD    wSDC;
BOOL    fDefault, fAcl;
PACL    pacl;
PSID    sid;



    if (NULL == pSD)
        {
        PINFO(TEXT("NULL sd\r\n"));
        return;
        }

    if (!IsValidSecurityDescriptor(pSD))
        {
        PINFO(TEXT("Bad SD %p"), pSD);
        return;
        }

    // Drop control info and revision
    if (GetSecurityDescriptorControl(pSD, &wSDC, &dwRev))
        {
        PINFO(TEXT("SD - Length: [%ld] Control: [%x] [%lx]\r\nGroup:"),
              GetSecurityDescriptorLength(pSD), wSDC, dwRev);
        }
    else
        {
        PINFO(TEXT("Couldn't get control\r\nGroup"));
        }

    // Show group and owner
    if (GetSecurityDescriptorGroup(pSD, &sid, &fDefault) &&
        sid &&
        IsValidSid(sid))
        {
        PrintSid(sid);
        PINFO(TEXT(" %s default.\r\nOwner:"), fDefault ? TEXT(" ") : TEXT("Not"));
        }
    else
        {
        PINFO(TEXT("Couldn't get group\r\n"));
        }

    if (GetSecurityDescriptorOwner(pSD, &sid, &fDefault) &&
        sid &&
        IsValidSid(sid))
        {
        PrintSid(sid);
        PINFO(TEXT(" %s default.\r\n"), fDefault ? TEXT(" ") : TEXT("Not"));
        }
    else
        {
        PINFO(TEXT("Couldn't get owner\r\n"));
        }

    // Print DACL and SACL
    if (GetSecurityDescriptorDacl(pSD, &fAcl, &pacl, &fDefault))
        {
        PINFO(TEXT("DACL: %s %s\r\n"), fAcl ? "Yes" : "No",
              fDefault ? "Default" : " ");
        if (fAcl)
            {
            if (pacl && IsValidAcl(pacl))
                {
                PrintAcl(pacl);
                }
            else
                {
                PINFO(TEXT("Invalid Acl %p\r\n"), pacl);
                }
            }
        }
    else
        {
        PINFO(TEXT("Couldn't get DACL\r\n"));
        }

    if (GetSecurityDescriptorSacl(pSD, &fAcl, &pacl, &fDefault))
        {
        PINFO(TEXT("SACL: %s %s\r\n"), fAcl ? "Yes" : "No", fDefault ? "Default" : " ");
        if (fAcl)
            {
            if (pacl && IsValidAcl(pacl))
                {
                PrintAcl(pacl);
                }
            else
                {
                PINFO(TEXT("Invalid ACL %p\r\n"), pacl);
                }
            }
        }
    else
        {
        PINFO(TEXT("Couldn't get SACL\r\n"));
        }

}


#else
#define PrintSid(x)
#define PrintSD(x)
#endif
