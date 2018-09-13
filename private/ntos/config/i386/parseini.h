PVOID
CmpOpenInfFile(
    IN  PVOID   InfImage,
    IN  ULONG   ImageSize
   );
   
VOID
CmpCloseInfFile(
    PVOID   InfHandle
    );   

PCHAR
CmpGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );
    
BOOLEAN
CmpSearchInfSection(
    IN PVOID InfHandle,
    IN PCHAR SectionName
    );
    
BOOLEAN
CmpSearchInfLine(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );
    
PCHAR
CmpGetSectionLineIndex (
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex
    );

ULONG
CmpGetSectionLineIndexValueCount(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );

BOOLEAN
CmpGetIntField(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PULONG Data
    );

BOOLEAN
CmpGetBinaryField(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PULONG ActualSize
    );
