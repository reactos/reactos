
PTSTR
StringRevChar(
    IN PTSTR String,
    IN TCHAR Char
    );

BOOL
InitDiamond(
    VOID
    );

VOID
TermDiamond(
    VOID
    );

BOOL
IsDiamondFile(
    IN PSTR FileName,
    OUT PBOOL ContainsMultipleFiles
    );

INT
ExpandDiamondFile(
    IN  NOTIFYPROC ExpandNotify,
    IN  PSTR       SourceFileName,
    IN  PSTR       TargetFileName,
    IN  BOOL       RenameTarget,
    IN  PSTR       SelectiveFilesSpec,
    OUT PLZINFO    pLZI
    );
