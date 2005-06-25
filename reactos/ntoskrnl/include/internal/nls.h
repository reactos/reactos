#ifndef __NTOSKRNL_INCLUDE_INTERNAL_NLS_H
#define __NTOSKRNL_INCLUDE_INTERNAL_NLS_H

extern PSECTION_OBJECT NlsSectionObject;

extern ULONG NlsAnsiTableOffset;
extern ULONG NlsOemTableOffset;
extern ULONG NlsUnicodeTableOffset;

extern PUSHORT NlsUnicodeUpcaseTable;
extern PUSHORT NlsUnicodeLowercaseTable;

VOID 
STDCALL 
RtlpInitNls(VOID);

VOID
RtlpImportAnsiCodePage(
    PUSHORT TableBase, 
    ULONG Size
);

VOID
RtlpImportOemCodePage(
    PUSHORT TableBase, 
    ULONG Size
);

VOID
RtlpImportUnicodeCasemap(
    PUSHORT TableBase, 
    ULONG Size
);

VOID
RtlpCreateInitialNlsTables(VOID);

VOID
RtlpCreateNlsSection(VOID);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_NLS_H */

/* EOF */
