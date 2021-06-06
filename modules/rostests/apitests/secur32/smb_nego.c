
#include "client_server.h"

void smbAddParamU1(UCHAR value, PBYTE* pOffset)
{
    *(PUCHAR)*pOffset = value;
    *pOffset += sizeof(value);
}

void smbAddParamU2(USHORT value, PBYTE* pOffset)
{
    *(PUSHORT)*pOffset = value;
    *pOffset += sizeof(value);
}

void smbAddParamU4(ULONG value, PBYTE* pOffset)
{
    *(PULONG)*pOffset = value;
    *pOffset += sizeof(value);
}

void smbAddParamStrW(WCHAR* value, PBYTE* pOffset)
{
    int bytelen;
    /* align to 2-byte boundary */
    *pOffset = (PBYTE)((ULONG)(*pOffset + 1) & 0xfffffffe);
    /* include terminating 0 */
    bytelen = (wcslen(value) + 1) * sizeof(WCHAR);
    memcpy((WCHAR*)*pOffset, value, bytelen);
    *pOffset += bytelen;
}

void smbAddParamBin(PBYTE value, int bytelen, PBYTE* pOffset)
{
    memcpy(*pOffset, value, bytelen);
    *pOffset += bytelen;
}

/* sets the size for for asn1
 * if < 128 one byte is used - this byte is usally
 * reserved
 * if > 128 two or more bytes are used. for these extra
 * bytes we have to move memory :-(
 *
 * buf is the begin of the buffer
 * buflen is the length of the buffer
 * sizeidx is the index of the byte for size
 * usedcount is the ammount of bytes used.
 * of the buffer (first unused byte)
 * size is the size to set to buf[sizeidx]
 */
void asn1_SetSize(
    IN PBYTE buf,
    IN ULONG buflen,
    IN ULONG sizeidx,
    IN ULONG size,
    PULONG usedcount)
{
    ULONG toMove, b;

    if (size < 128)
    {
        buf[sizeidx] = (BYTE)size;
        return;
    }
    // b = log2(size) / 8 + 1;
    b = log(size) / log(2) / 8 + 1;
    if (b > 4)
    {
        sync_err("FIXME: size to long!\n");
        buf[sizeidx] = 0x00;//
        return;
    }
    if (*usedcount + b > buflen)
    {
        sync_err("buffer overflow (used 0x%x, buflen 0x%x)!\n",
                 *usedcount, buflen);
        buf[sizeidx] = 0x00;//
        return;
    }
    /* make one byte more room */
    toMove = *usedcount - sizeidx;
    memmove(&buf[sizeidx + b + 1], &buf[sizeidx + 1], toMove);
    *usedcount += b;
    /* set size
     * bit 8 = 1 = size >= 128
     * bit 1-7 = additional bytes
     */
    buf[sizeidx] = (BYTE)0x80 + b;
    while (b > 0)
    {
        buf[sizeidx + b] = (BYTE)(size >> (8 * (b-1))) & 0xff;
        b--;
    }
}

ULONG asn1_GetSize(
    IN OUT PBYTE* pOffset,
    IN BOOL doSkip)
{
    ULONG b, b2;
    ULONG size = **pOffset;

    if (size < 128)
    {
        if (doSkip)
        {
            *pOffset += 1;
            //printf("skipping %i bytes (0x%x)\n", 1, **pOffset);
        }
        return size;
    }

    if (!(size & 0x80))
    {
        sync_err("invalid asn1 size!\n");
        return 0;
    }

    b = size & 0x7f;
    if (b > 4)
    {
        sync_err("b > 4 not implemented!");
        return 0;
    }

    size = 0;
    b2 = b;
    while (b > 0)
    {
        //printf("byte (pos %lx) 0x%x\n", b, (*pOffset)[b]);
        size = size | ((*pOffset)[b] << (8 * (b-1)));
        //printf("size 0x%lx\n", size);
        b--;
    }

    if (doSkip)
    {
        *pOffset += b2 + 1;
        //printf("skipping %li bytes (0x%x)\n", b2+1, **pOffset);
    }

    return size;
}

BOOL
asn1_SkipToken(
    IN OUT PBYTE* pOffset,
    IN BYTE expectedToken,
    IN BOOL skipTokenSize)
{
    ULONG tokenSize;

    if (**pOffset != expectedToken)
    {
        sync_err("unexpected Token (is 0x%x, expected 0x%x)!\n",
                 **pOffset, expectedToken);
        return FALSE;
    }

    (*pOffset)++;
    tokenSize = asn1_GetSize(pOffset, TRUE);

    if (skipTokenSize)
    {
        (*pOffset) += tokenSize;
        //printf("skipping %li bytes (0x%x)\n", tokenSize, **pOffset);
    }

    return TRUE;
}

BOOL
smb_GenSPNEGOSecurityBlob(
    IN PBYTE ntlmMsg,
    IN ULONG ntlmMsgLen,
    OUT PBYTE pSecurityBlob,
    OUT PULONG pSecurityBlobLen)
{
    PBYTE spNego = pSecurityBlob;
    ULONG spNegoLen = 0;
    ULONG MsgTyp = ((PNTLM_MESSAGE_HEAD)ntlmMsg)->MsgType;

    if (MsgTyp == NtlmNegotiate)
    {
        // Build SPNEGO / security blob (NTMLSSP NEGOTIATE message)
        // http://www.rfc-editor.org/rfc/rfc2743.txt (3.1)
        // 0x60 Application 0 sequence
        spNego[0] = ASN1_APPLICATION(0);
        // länge ... wenn < 128 ein Byte
        //  sonst Bit 8 = 1 und der rest gibt an wie viele byte die
        //  laenge enthalten..
        // later ... lets hope we get less than 128 byte
        spNego[1] = 0;
        // 0x06 = Object identifier
        spNego[2] = ASN1_OID;
        // Object indetifier length ...
        spNego[3] = 0x06;
        // ??? OID: 1.3.6.1.5.5.2 (SPNEGO - Simple Protected Negotiation)
        // 2b0601050502
        // https://www.rfc-editor.org/rfc/rfc4178.txt (4.2)
        spNego[4] = 0x2b; // 00101011
        spNego[5] = 0x06;
        spNego[6] = 0x01;
        spNego[7] = 0x05;
        spNego[8] = 0x05;
        spNego[9] = 0x02;
        // ??? negTokenInit
        // a03e
        spNego[10] = ASN1_CONTEXT(0); // 0110 0000
                           // ^ Token init
                           //
        spNego[11] = 0x00; // 3e länge ??
        // mechtype ??
        // MechType: 1.3.6.1.4.1.311.2.2.10 (NTLMSSP)
        // 303ca00e300c
        spNego[12] = ASN1_SEQUENCE(0); // 0x30  -- identifier octet for constructed SEQUENCE (NegTokenInit)
        spNego[13] = 0x3c; // ??? Länge

        spNego[14] = ASN1_CONTEXT(0); // 0xA0 -- identifier octet for constructed [0]
        spNego[15] = 0x0e; // 14 Byte = länge

        spNego[16] = ASN1_SEQUENCE(0); // 0x30 identifier octet for constructed SEQUENCE
        spNego[17] = 0x0c; // 12 Byte = länge
        // mechtypes 1 item
        spNego[18] = ASN1_OID; // 0x06 identifier octet for primitive OBJECT IDENTIFIER
        spNego[19] = 0x0a; // 0xa Länge
        // NTLM ... (OID - String)
        spNego[20] = 0x2b;
        spNego[21] = 0x06;
        spNego[22] = 0x01;
        spNego[23] = 0x04;
        spNego[24] = 0x01;
        spNego[25] = 0x82;
        spNego[26] = 0x37;
        spNego[27] = 0x02;
        spNego[28] = 0x02;
        spNego[29] = 0x0a;

        // align??
        spNego[30] = ASN1_CONTEXT(2);
        spNego[31] = 0x2a; // 0x28 - Länge der NEGOTIATE_MESSAGE + 2
        spNego[32] = 0x04;
        spNego[33] = 0x28;

        spNegoLen = 34;
    }
    else if (MsgTyp == NtlmAuthenticate)
    {
        //a1 81d7 30 81d4 a2 81d1 04 81ce
        spNego[0] = ASN1_CONTEXT(1);
        spNego[1] = 0;//size!
        spNego[2] = ASN1_SEQUENCE(0);
        spNego[3] = 0;//size
        spNego[4] = ASN1_CONTEXT(2);
        spNego[5] = 0;//size
        spNego[6] = 0x4;//??
        spNego[7] = 0;//size

        spNegoLen = 8;
    }
    else
    {
        sync_err("message typ not supported\n");
        return FALSE;
    }

    memcpy(&spNego[spNegoLen], ntlmMsg, ntlmMsgLen);

    spNegoLen += ntlmMsgLen;

    // setting sizes ...
    // be aware spNegoLen can increase, so begin with the last ...
    if (MsgTyp == NtlmNegotiate)
    {
        asn1_SetSize(spNego, *pSecurityBlobLen, 33, ntlmMsgLen, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen, 31, ntlmMsgLen + 2, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen, 13, spNegoLen - 2 - 12, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen, 11, spNegoLen - 2 - 10, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen,  1, spNegoLen - 2, &spNegoLen);
    }
    else if (MsgTyp == NtlmAuthenticate)
    {
        asn1_SetSize(spNego, *pSecurityBlobLen,  7, spNegoLen - 8, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen,  5, spNegoLen - 6, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen,  3, spNegoLen - 4, &spNegoLen);
        asn1_SetSize(spNego, *pSecurityBlobLen,  1, spNegoLen - 2, &spNegoLen);
    }

    *pSecurityBlobLen = spNegoLen;
    return TRUE;
}

BOOL
smb_GenComNegoMsg(
    OUT PBYTE pOutBuf,
    IN OUT PULONG pBufLen)
{
    PSMB_Header psh;
    PSMB_Parameters psp;
    PSMB_Data psd;
    DWORD ClientPID = GetCurrentProcessId();
    ULONG msgsize, datasize;
    char* data;

    psh = (PSMB_Header)pOutBuf;
    psp = (PSMB_Parameters)((PBYTE)psh + sizeof(SMB_Header));
    psd = (PSMB_Data)((PBYTE)psp + sizeof(SMB_Parameters));

    psh->Protocol[0] = 0xFF;
    psh->Protocol[1] = 'S';
    psh->Protocol[2] = 'M';
    psh->Protocol[3] = 'B';
    psh->Command = 0x72; //SMB_COM_MSG
    //sh.Status = 0;//??
    psh->Flags = 0x18;
    //0xC807;//SMB_FLAGS2_EXTENDED_SECURITY;
    psh->Flags2 = 0xc853 |
                  SMB_FLAGS2_NT_STATUS;
    //UCHAR SecurityFeatures[8];
    //USHORT Reserved;
    psh->TID = 0;//(TID & 0xffff);
    psh->PIDHigh = (ClientPID >> 16);
    psh->PIDLow = (ClientPID & 0xffff);
    psh->UID = 0;
    //USHORT MID;
    msgsize = sizeof(SMB_Header);

    psp->WordCount = 0;

    msgsize += sizeof(SMB_Parameters);

    datasize = 24+11+29+11+11+12;
    data = "\2PC NETWORK PROGRAM 1.0\0"
           "\2LANMAN1.0\0"
           "\2Windows for Workgroups 3.1a\0"
           "\2LM1.2X002\0"
           "\2LANMAN2.1\0"
           "\2NT LM 0.12\0";

    msgsize += sizeof(SMB_Data) + datasize  -1;
    psd->ByteCount = datasize;

    memcpy((char*)&psd->Data, (char*)data,
           datasize);

    *pBufLen = msgsize;
    return TRUE;
}

BOOL
smb_GenComSessionSetupMsg(
    OUT PBYTE pOutBuf,
    IN OUT PULONG pBufLen,
    IN PBYTE ntlmMsg,
    IN ULONG ntlmMsgLen,
    IN USHORT smbSessionId,
    IN USHORT smbRequestCounter)
{
    DWORD cbSPNego = 0;
    BYTE bufSPNego[5 * 1024];
    PSMB_Header psh;
    PSMB_Parameters psp;
    PSMB_Data psd;
    PBYTE pOffset;
    ULONG msgsize;
    USHORT AndXOffset;
    DWORD ClientPID = GetCurrentProcessId();

    /* generate Security Blob (SPNEGO) */
    cbSPNego = sizeof(bufSPNego);
    if (!smb_GenSPNEGOSecurityBlob(ntlmMsg, ntlmMsgLen, bufSPNego, &cbSPNego))
    {
        sync_err("Cant genrate SMB Negotiation message\n");
        return FALSE;
    }

    if (((PNTLM_MESSAGE_HEAD)ntlmMsg)->MsgType == NtlmChallenge)
        AndXOffset = 266;
    else
        AndXOffset = 410; // NtlmAuth

    // Session Setup AndX Request (0x73 NTLMSSP_NEGOTIATE)
    RtlZeroMemory(pOutBuf, *pBufLen);
    psh = (PSMB_Header)pOutBuf;
    psp = (PSMB_Parameters)((PBYTE)psh + sizeof(SMB_Header));

    psh->Protocol[0] = 0xFF;
    psh->Protocol[1] = 'S';
    psh->Protocol[2] = 'M';
    psh->Protocol[3] = 'B';
    psh->Status.NT_Status = 0;
    psh->Command = 0x73;
    psh->Flags = 0x18;
    psh->Flags2 = 0xc807 |
                  SMB_FLAGS2_NT_STATUS;
    psh->PIDHigh = (ClientPID >> 16);

    psh->PIDLow = (ClientPID & 0xffff);
    psh->MID = smbRequestCounter;
    /* session Id - received from server with first response! */
    psh->UID = smbSessionId;

    psp->WordCount = 0xc; // must be 0xc
    pOffset = (PBYTE)psp + sizeof(SMB_Parameters);

    // AndXCommand: No further commands (0xff)
    smbAddParamU1(0xff, &pOffset);
    // Reserved
    smbAddParamU1(0, &pOffset);
    // AndXOffset 266
    smbAddParamU2(AndXOffset, &pOffset);
    // Max Buffer: 4356
    smbAddParamU2(4356, &pOffset);
    // Max Mpx Count: 50
    smbAddParamU2(50, &pOffset);
    // VC Number: 0
    smbAddParamU2(0, &pOffset);
    // Session Key: 0x00000000
    smbAddParamU4(0x00000000, &pOffset);
    // Security Blob Length: 74
    smbAddParamU2(cbSPNego, &pOffset);
    // Reserved: 00000000
    smbAddParamU4(0, &pOffset);
    // Capabilities: 0xa00000d4, Unicode, NT SMBs, NT Status Codes, Level 2 Oplocks, Dynamic Reauth, Extended Security
    smbAddParamU4(0xa00000d4, &pOffset);

    psd = (PSMB_Data)pOffset;
    pOffset = (PBYTE)psd + sizeof(SMB_Data) - 1;

    smbAddParamBin(bufSPNego, cbSPNego, &pOffset);

    //HACK
    // TODO: Native OS: Windows Server 2003 3790 Service Pack 2
    smbAddParamStrW(L"Windows Server 2003 3790 Service Pack 2", &pOffset);
    // TODO: Native LAN Manager:
    smbAddParamStrW(L"", &pOffset);
    // TODO: Primary Domain: Windows Server 2003 5.2
    smbAddParamStrW(L"Windows Server 2003 5.2", &pOffset);
    // Extra byte parameters: 0000
    smbAddParamU2(0, &pOffset);

    msgsize = (ULONG)pOffset - (ULONG)pOutBuf;
    psd->ByteCount = (ULONG)(PBYTE*)pOffset - (ULONG)(PBYTE*)&psd->Data;
    PrintHexDumpMax(msgsize, (PBYTE)psh, msgsize);

    *pBufLen = msgsize;
    return TRUE;
}

BOOL
smb_GetNTMLMsg(
    IN PBYTE pSMBBuf,
    IN ULONG smbBufLen,
    OUT PBYTE* pNegoBuf,
    OUT PULONG pNegoBufLen)
{
    PSMB_Header psh;
    PSMB_Parameters psp;
    PSMB_Data psd;
    PBYTE pdata, pblob, pblobOfs;
    int bloblen, tokensize;

    psh = (PSMB_Header)pSMBBuf;

    if ((psh->Protocol[0] != 0xff) ||
        (psh->Protocol[1] != 'S') ||
        (psh->Protocol[2] != 'M') ||
        (psh->Protocol[3] != 'B'))
    {
        sync_err("wrong signature (SMB expected!)");
        return FALSE;
    }

    psp = (PSMB_Parameters)(psh+1);
    if ((psh->Command != 0x73) || (psp->WordCount != 0x4))
    {
        sync_err("wrong wordcount (cmd 0x%x, wc 0x%x)\n",
                 psh->Command, psp->WordCount);
        return FALSE;
    }
    pdata = (PBYTE)psp + sizeof(psp->WordCount);

    bloblen = *(PSHORT)&pdata[6];

    psd = (PSMB_Data)&pdata[8];

    pblob = (PBYTE)&psd->Data;

    if (bloblen > psd->ByteCount)
    {
        sync_err("invalid blob len 0x%x\n (> bc 0x%x)", bloblen, psd->ByteCount);
        return FALSE;
    }

    //printf("blob len %d\n", bloblen);
    //printf("byte count %d\n", psd->ByteCount);
    //printf("first byte blob %x\n", *pblob);

    pblobOfs = pblob;
    asn1_SkipToken(&pblobOfs, ASN1_CONTEXT(1), FALSE);
    asn1_SkipToken(&pblobOfs, ASN1_SEQUENCE(0), FALSE);
    asn1_SkipToken(&pblobOfs, ASN1_CONTEXT(0), TRUE);
    asn1_SkipToken(&pblobOfs, ASN1_CONTEXT(1), TRUE);
    asn1_SkipToken(&pblobOfs, ASN1_CONTEXT(2), FALSE);
    /* now we have NTLMSSP-DATA */
    //TODO check *blobOfs == 0x4,
    pblobOfs++; // skip 0x04
    tokensize = asn1_GetSize(&pblobOfs, TRUE);
    /* pblobOfs should now point to the begin and tokensize
       is the length! */
    if (strncmp((char*)pblobOfs, "NTLMSSP", 7) != 0)
    {
        sync_err("failt to get NTLMSSP-Data!\n");
        return FALSE;
    }

    if ((PBYTE)pblobOfs - (PBYTE)pblob + tokensize > bloblen)
    {
        sync_err("data out of bounds!\n");
        return FALSE;
    }

    *pNegoBuf = pblobOfs;
    *pNegoBufLen = tokensize;
    printf("next byte blob %x\n", *pblobOfs);
    printf("tokensize %d\n", tokensize);

    return TRUE;
}
