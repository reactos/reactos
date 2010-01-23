
#pragma once
#pragma pack(push)

#ifdef __midl
  typedef struct
  {
    WORD Bits;
  } PID_BITS_MIDL;

  typedef struct
  {
    WORD Bits;
  } MPEG_HEADER_BITS_MIDL;

  typedef struct
  {
    BYTE Bits;
  } MPEG_HEADER_VERSION_BITS_MIDL;

#else

  typedef struct
  {
    WORD Reserved : 3;
    WORD ProgramId : 13;
  } PID_BITS, *PPID_BITS;
  typedef struct
 {
    WORD SectionLength              : 12;
    WORD Reserved                   : 2;
    WORD PrivateIndicator           : 1;
    WORD SectionSyntaxIndicator     : 1;
  } MPEG_HEADER_BITS, *PMPEG_HEADER_BITS;

  typedef struct
  {
    BYTE CurrentNextIndicator : 1;
    BYTE VersionNumber        : 5;
    BYTE Reserved             : 2;
  } MPEG_HEADER_VERSION_BITS, *PMPEG_HEADER_VERSION_BITS;
#endif

#pragma pack(pop)

