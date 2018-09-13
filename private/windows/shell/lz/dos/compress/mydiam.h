
INT
DiamondCompressFile(
    IN  NOTIFYPROC CompressNotify,
    IN  PSTR       SourceFile,
    IN  PSTR       TargetFile,
    IN  BOOL       Rename,
    OUT PLZINFO    pLZI
    );

extern TCOMP DiamondCompressionType;  // 0 if not diamond (ie, LZ)
