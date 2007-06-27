
#ifndef _NTNLS_
#define _NTNLS_


#ifdef __cplusplus
extern "C" {
#endif
#define MAXIMUM_LEADBYTES 12
typedef struct _CPTABLEINFO 
{
   USHORT  CodePage;
   USHORT  MaximumCharacterSize;
   USHORT  DefaultChar;
   USHORT  UniDefaultChar;
   USHORT  TransDefaultChar;
   USHORT  TransUniDefaultChar;
   USHORT  DBCSCodePage;
   UCHAR LeadByte[MAXIMUM_LEADBYTES];
   PUSHORT MultiByteTable;
   PVOID WideCharTable;
   PUSHORT DBCSRanges;
   PUSHORT DBCSOffsets;
} CPTABLEINFO, *PCPTABLEINFO;

typedef struct _NLSTABLEINFO 
{
   CPTABLEINFO OemTableInfo;
   CPTABLEINFO AnsiTableInfo;
   PUSHORT UpperCaseTable;
   PUSHORT LowerCaseTable;
} NLSTABLEINFO, *PNLSTABLEINFO;

#ifdef __cplusplus
}
#endif
#endif 
