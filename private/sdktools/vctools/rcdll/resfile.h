typedef struct tagRESADDITIONAL
{
    DWORD       DataSize;               // size of data without header
    DWORD       HeaderSize;     // Length of the header
    // [Ordinal or Name TYPE]
    // [Ordinal or Name NAME]
    DWORD       DataVersion;    // version of data struct
    WORD        MemoryFlags;    // state of the resource
    WORD        LanguageId;     // Unicode support for NLS
    DWORD       Version;        // Version of the resource data
    DWORD       Characteristics;        // Characteristics of the data
} RESADDITIONAL, *PRESADDITIONAL;
