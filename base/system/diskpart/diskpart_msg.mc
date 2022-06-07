
MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409
               German=0x407:MSG00407
               Polish=0x415:MSG00415
               Portugese=0x816:MSG00416
               Romanian=0x418:MSG00418
               Russian=0x419:MSG00419
               Albanian=0x41C:MSG0041C
               Turkish=0x41F:MSG0041F
               Chinese=0x804:MSG00804
               Taiwanese=0x404:MSG00404
              )


MessageId=10000
SymbolicName=MSG_COMMAND_ACTIVE
Severity=Informational
Facility=System
Language=English
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=German
    Markiert auf Datenträgern mit MBR-Datenträgerformat (Master Boot
    Record) die Partition, die den Fokus hat, als aktive Partition.

Syntax:  ACTIVE

    Schreibt einen Wert auf den Datenträger, der Beim Start vom BIOS
    (Basic Input/Output System) gelesen wird. Durch diesen Wert wird
    angegeben, dass die Partition eine gültige Systempartition ist.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss eine
    Partition ausgewählt sein.

    Vorsicht:

        DiskPart überprüft nur, ob die Partition in der Lage ist, die
        Startdateien des Betriebssystems aufzunehmen. Der Inhalt der Partition
        wird hierbei nicht überprüft. Falls versehentlich eine Partition als
        aktiv markiert wird, die nicht die Startdateien des Betriebssystems
        enthält, kann der Computer möglicherweise nicht gestartet werden.

Beispiel:

    ACTIVE
.
Language=Polish
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Portugese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Romanian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Russian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Albanian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Turkish
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Chinese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.
Language=Taiwanese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as active.

Syntax:  ACTIVE

    Writes a value to the disk which is read by the basic input/output
    system (BIOS) at boot. This value specifies that the partition is
    a valid system partition.

    A partition must be selected for this operation to succeed.

    Caution:

        DiskPart verifies only that the partition is capable of containing the
        operating system startup files. DiskPart does not check the contents of
        the partition. If you mistakenly mark a partition as active and it does
        not contain the operating system startup files, your computer might not
        start.

Example:

    ACTIVE
.


MessageId=10001
SymbolicName=MSG_COMMAND_ADD
Severity=Informational
Facility=System
Language=English
<Add ADD command help text here>
.
Language=German
<Add ADD command help text here>
.
Language=Polish
<Add ADD command help text here>
.
Language=Portugese
<Add ADD command help text here>
.
Language=Romanian
<Add ADD command help text here>
.
Language=Russian
<Add ADD command help text here>
.
Language=Albanian
<Add ADD command help text here>
.
Language=Turkish
<Add ADD command help text here>
.
Language=Chinese
<Add ADD command help text here>
.
Language=Taiwanese
<Add ADD command help text here>
.


MessageId=10002
SymbolicName=MSG_COMMAND_ASSIGN
Severity=Informational
Facility=System
Language=English
<Add ASSIGN command help text here>
.
Language=German
<Add ASSIGN command help text here>
.
Language=Polish
<Add ASSIGN command help text here>
.
Language=Portugese
<Add ASSIGN command help text here>
.
Language=Romanian
<Add ASSIGN command help text here>
.
Language=Russian
<Add ASSIGN command help text here>
.
Language=Albanian
<Add ASSIGN command help text here>
.
Language=Turkish
<Add ASSIGN command help text here>
.
Language=Chinese
<Add ASSIGN command help text here>
.
Language=Taiwanese
<Add ASSIGN command help text here>
.


MessageId=10003
SymbolicName=MSG_COMMAND_ATTACH
Severity=Informational
Facility=System
Language=English
<Add ATTACH command help text here>
.
Language=German
<Add ATTACH command help text here>
.
Language=Polish
<Add ATTACH command help text here>
.
Language=Portugese
<Add ATTACH command help text here>
.
Language=Romanian
<Add ATTACH command help text here>
.
Language=Russian
<Add ATTACH command help text here>
.
Language=Albanian
<Add ATTACH command help text here>
.
Language=Turkish
<Add ATTACH command help text here>
.
Language=Chinese
<Add ATTACH command help text here>
.
Language=Taiwanese
<Add ATTACH command help text here>
.


MessageId=10004
SymbolicName=MSG_COMMAND_ATTRIBUTES
Severity=Informational
Facility=System
Language=English
<Add ATTRIBUTES command help text here>
.
Language=German
<Add ATTRIBUTES command help text here>
.
Language=Polish
<Add ATTRIBUTES command help text here>
.
Language=Portugese
<Add ATTRIBUTES command help text here>
.
Language=Romanian
<Add ATTRIBUTES command help text here>
.
Language=Russian
<Add ATTRIBUTES command help text here>
.
Language=Albanian
<Add ATTRIBUTES command help text here>
.
Language=Turkish
<Add ATTRIBUTES command help text here>
.
Language=Chinese
<Add ATTRIBUTES command help text here>
.
Language=Taiwanese
<Add ATTRIBUTES command help text here>
.


MessageId=10005
SymbolicName=MSG_COMMAND_AUTOMOUNT
Severity=Informational
Facility=System
Language=English
<Add AUTOMOUNT command help text here>
.
Language=German
<Add AUTOMOUNT command help text here>
.
Language=Polish
<Add AUTOMOUNT command help text here>
.
Language=Portugese
<Add AUTOMOUNT command help text here>
.
Language=Romanian
<Add AUTOMOUNT command help text here>
.
Language=Russian
<Add AUTOMOUNT command help text here>
.
Language=Albanian
<Add AUTOMOUNT command help text here>
.
Language=Turkish
<Add AUTOMOUNT command help text here>
.
Language=Chinese
<Add AUTOMOUNT command help text here>
.
Language=Taiwanese
<Add AUTOMOUNT command help text here>
.



MessageId=10006
SymbolicName=MSG_COMMAND_BREAK
Severity=Informational
Facility=System
Language=English
<Add BREAK command help text here>
.
Language=German
<Add BREAK command help text here>
.
Language=Polish
<Add BREAK command help text here>
.
Language=Portugese
<Add BREAK command help text here>
.
Language=Romanian
<Add BREAK command help text here>
.
Language=Russian
<Add BREAK command help text here>
.
Language=Albanian
<Add BREAK command help text here>
.
Language=Turkish
<Add BREAK command help text here>
.
Language=Chinese
<Add BREAK command help text here>
.
Language=Taiwanese
<Add BREAK command help text here>
.


MessageId=10007
SymbolicName=MSG_COMMAND_CLEAN
Severity=Informational
Facility=System
Language=English
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=German
    Entfernt alle Partitions- oder Volumeformatierungen von dem Datenträger
    mit dem Fokus.

Syntax:  CLEAN [ALL]

    ALL         Gibt an, dass jedes Byte\jeder Sektor auf dem Datenträger
                auf Null gesetzt wird. Damit werden alle auf dem Datenträger
                enthaltenen Daten vollständig gelöscht.

    Auf MBR-Datenträgern (Master Boot Record) werden nur MBR-
    Partitionierungsinformationen und Informationen zu ausgeblendeten
    Sektoren überschrieben. Auf GUID-Partitionstabellen-Datenträgern
    (GPT) werden die GPT-Partitionierungsinformationen (einschließlich
    Schutz-MBR) überschrieben. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Beispiel:

    CLEAN
.
Language=Polish
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Portugese
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Romanian
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Russian
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Albanian
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Turkish
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Chinese
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.
Language=Taiwanese
    Removes any and all partition or volume formatting from the disk with
    focus.

Syntax:  CLEAN [ALL]

    ALL         Specifies that each and every byte\sector on the disk is set to
                zero, which completely deletes all data contained on the disk.

    On master boot record (MBR) disks, only the MBR partitioning information
    and hidden sector information are overwritten. On GUID partition table
    (GPT) disks, the GPT partitioning information, including the Protective
    MBR, is overwritten. If the ALL parameter is not used, the first 1MB
    and the last 1MB of the disk are zeroed. This erases any disk formatting 
    that had been previously applied to the disk. The disk's state after 
    cleaning the disk is 'UNINITIALIZED'.

Example:

    CLEAN
.


MessageId=10008
SymbolicName=MSG_COMMAND_COMPACT
Severity=Informational
Facility=System
Language=English
<Add COMPACT command help text here>
.
Language=German
<Add COMPACT command help text here>
.
Language=Polish
<Add COMPACT command help text here>
.
Language=Portugese
<Add COMPACT command help text here>
.
Language=Romanian
<Add COMPACT command help text here>
.
Language=Russian
<Add COMPACT command help text here>
.
Language=Albanian
<Add COMPACT command help text here>
.
Language=Turkish
<Add COMPACT command help text here>
.
Language=Chinese
<Add COMPACT command help text here>
.
Language=Taiwanese
<Add COMPACT command help text here>
.


MessageId=10010
SymbolicName=MSG_COMMAND_CONVERT
Severity=Informational
Facility=System
Language=English
<Add CONVERT command help text here>
.
Language=German
<Add CONVERT command help text here>
.
Language=Polish
<Add CONVERT command help text here>
.
Language=Portugese
<Add CONVERT command help text here>
.
Language=Romanian
<Add CONVERT command help text here>
.
Language=Russian
<Add CONVERT command help text here>
.
Language=Albanian
<Add CONVERT command help text here>
.
Language=Turkish
<Add CONVERT command help text here>
.
Language=Chinese
<Add CONVERT command help text here>
.
Language=Taiwanese
<Add CONVERT command help text here>
.


MessageId=10011
SymbolicName=MSG_COMMAND_CREATE_PARTITION_EFI
Severity=Informational
Facility=System
Language=English
<Add CREATE PARTITION EFI command help text here>
.
Language=German
<Add CREATE PARTITION EFI command help text here>
.
Language=Polish
<Add CREATE PARTITION EFI command help text here>
.
Language=Portugese
<Add CREATE PARTITION EFI command help text here>
.
Language=Romanian
<Add CREATE PARTITION EFI command help text here>
.
Language=Russian
<Add CREATE PARTITION EFI command help text here>
.
Language=Albanian
<Add CREATE PARTITION EFI command help text here>
.
Language=Turkish
<Add CREATE PARTITION EFI command help text here>
.
Language=Chinese
<Add CREATE PARTITION EFI command help text here>
.
Language=Taiwanese
<Add CREATE PARTITION EFI command help text here>
.


MessageId=10012
SymbolicName=MSG_COMMAND_CREATE_PARTITION_EXTENDED
Severity=Informational
Facility=System
Language=English
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=German
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Polish
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Portugese
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Romanian
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Russian
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Albanian
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Turkish
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Chinese
<Add CREATE PARTITION EXTENDED command help text here>
.
Language=Taiwanese
<Add CREATE PARTITION EXTENDED command help text here>
.


MessageId=10013
SymbolicName=MSG_COMMAND_CREATE_PARTITION_LOGICAL
Severity=Informational
Facility=System
Language=English
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=German
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Polish
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Portugese
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Romanian
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Russian
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Albanian
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Turkish
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Chinese
<Add CREATE PARTITION LOGICAL command help text here>
.
Language=Taiwanese
<Add CREATE PARTITION LOGICAL command help text here>
.


MessageId=10014
SymbolicName=MSG_COMMAND_CREATE_PARTITION_MSR
Severity=Informational
Facility=System
Language=English
<Add CREATE PARTITION MSR command help text here>
.
Language=German
<Add CREATE PARTITION MSR command help text here>
.
Language=Polish
<Add CREATE PARTITION MSR command help text here>
.
Language=Portugese
<Add CREATE PARTITION MSR command help text here>
.
Language=Romanian
<Add CREATE PARTITION MSR command help text here>
.
Language=Russian
<Add CREATE PARTITION MSR command help text here>
.
Language=Albanian
<Add CREATE PARTITION MSR command help text here>
.
Language=Turkish
<Add CREATE PARTITION MSR command help text here>
.
Language=Chinese
<Add CREATE PARTITION MSR command help text here>
.
Language=Taiwanese
<Add CREATE PARTITION MSR command help text here>
.


MessageId=10015
SymbolicName=MSG_COMMAND_CREATE_PARTITION_PRIMARY
Severity=Informational
Facility=System
Language=English
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=German
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Polish
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Portugese
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Romanian
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Russian
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Albanian
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Turkish
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Chinese
<Add CREATE PARTITION PRIMARY command help text here>
.
Language=Taiwanese
<Add CREATE PARTITION PRIMARY command help text here>
.


MessageId=10016
SymbolicName=MSG_COMMAND_DELETE_DISK
Severity=Informational
Facility=System
Language=English
<Add DELETE DISK command help text here>
.
Language=German
<Add DELETE DISK command help text here>
.
Language=Polish
<Add DELETE DISK command help text here>
.
Language=Portugese
<Add DELETE DISK command help text here>
.
Language=Romanian
<Add DELETE DISK command help text here>
.
Language=Russian
<Add DELETE DISK command help text here>
.
Language=Albanian
<Add DELETE DISK command help text here>
.
Language=Turkish
<Add DELETE DISK command help text here>
.
Language=Chinese
<Add DELETE DISK command help text here>
.
Language=Taiwanese
<Add DELETE DISK command help text here>
.


MessageId=10017
SymbolicName=MSG_COMMAND_DELETE_PARTITION
Severity=Informational
Facility=System
Language=English
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=German
Löscht die Partition, die den Fokus hat.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       Nur für Skripting. Bei einem Fehler setzt DiskPart die
                Verarbeitung von Befehlen fort, als sei der Fehler nicht
                aufgetreten. Ohne den Parameter NOERR wird DiskPart bei
                einem Fehler mit dem entsprechenden Fehlercode beendet.

    OVERRIDE    Ermöglicht DiskPart das Löschen einer beliebigen Partition
                unabhängig von deren Typ. Normalerweise gestattet DiskPart
                nur das Löschen bekannter Datenpartitionen.

    Sie können keine Systempartition, Startpartition oder Partition löschen,
    die eine aktive Auslagerungsdatei oder ein Absturzabbild (Speicherabbild)
    enthält.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss eine
    Partition ausgewählt sein.

    Partitionen könnnen nicht von dynamischen Datenträgern gelöscht oder auf
    dynamischen Datenträgern erstellt werden.

Beispiel:

    DELETE PARTITION
.
Language=Polish
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Portugese
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Romanian
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Russian
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Albanian
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Turkish
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Chinese
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.
Language=Taiwanese
Deletes the partition with focus.

Syntax:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    OVERRIDE    Enables DiskPart to delete any partition regardless of type.
                Typically, DiskPart only permits you to delete known data
                partitions.

    You cannot delete the system partition, boot partition, or any partition
    that contains the active paging file or crash dump (memory dump) filed.

    A partition must be selected for this operation to succeed.

    Partitions cannot be deleted from dynamic disks or created on dynamic
    disks.

Example:

    DELETE PARTITION
.


MessageId=10018
SymbolicName=MSG_COMMAND_DELETE_VOLUME
Severity=Informational
Facility=System
Language=English
<Add DELETE VOLUME command help text here>
.
Language=German
<Add DELETE VOLUME command help text here>
.
Language=Polish
<Add DELETE VOLUME command help text here>
.
Language=Portugese
<Add DELETE VOLUME command help text here>
.
Language=Romanian
<Add DELETE VOLUME command help text here>
.
Language=Russian
<Add DELETE VOLUME command help text here>
.
Language=Albanian
<Add DELETE VOLUME command help text here>
.
Language=Turkish
<Add DELETE VOLUME command help text here>
.
Language=Chinese
<Add DELETE VOLUME command help text here>
.
Language=Taiwanese
<Add DELETE VOLUME command help text here>
.


MessageId=10019
SymbolicName=MSG_COMMAND_DETAIL_DISK
Severity=Informational
Facility=System
Language=English
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=German
    Zeigt die Eigenschaften des ausgewählten Datenträgers und die Liste der
    Volumes auf dem Datenträger an.

Syntax:  DETAIL DISK

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein
    Datenträger ausgewählt sein.

Beispiel:

    DETAIL DISK
.
Language=Polish
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Portugese
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Romanian
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Russian
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Albanian
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Turkish
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Chinese
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.
Language=Taiwanese
    Displays the properties of the selected disk and the list of volumes on
    the disk.

Syntax:  DETAIL DISK

    A disk must be selected for this operation to succeed.

Example:

    DETAIL DISK
.

MessageId=10020
SymbolicName=MSG_COMMAND_DETAIL_PARTITION
Severity=Informational
Facility=System
Language=English
<Add DETAIL PARTITION command help text here>
.
Language=German
<Add DETAIL PARTITION command help text here>
.
Language=Polish
<Add DETAIL PARTITION command help text here>
.
Language=Portugese
<Add DETAIL PARTITION command help text here>
.
Language=Romanian
<Add DETAIL PARTITION command help text here>
.
Language=Russian
<Add DETAIL PARTITION command help text here>
.
Language=Albanian
<Add DETAIL PARTITION command help text here>
.
Language=Turkish
<Add DETAIL PARTITION command help text here>
.
Language=Chinese
<Add DETAIL PARTITION command help text here>
.
Language=Taiwanese
<Add DETAIL PARTITION command help text here>
.

MessageId=10021
SymbolicName=MSG_COMMAND_DETAIL_VOLUME
Severity=Informational
Facility=System
Language=English
<Add DETAIL VOLUME command help text here>
.
Language=German
<Add DETAIL VOLUME command help text here>
.
Language=Polish
<Add DETAIL VOLUME command help text here>
.
Language=Portugese
<Add DETAIL VOLUME command help text here>
.
Language=Romanian
<Add DETAIL VOLUME command help text here>
.
Language=Russian
<Add DETAIL VOLUME command help text here>
.
Language=Albanian
<Add DETAIL VOLUME command help text here>
.
Language=Turkish
<Add DETAIL VOLUME command help text here>
.
Language=Chinese
<Add DETAIL VOLUME command help text here>
.
Language=Taiwanese
<Add DETAIL VOLUME command help text here>
.


MessageId=10022
SymbolicName=MSG_COMMAND_DETACH
Severity=Informational
Facility=System
Language=English
<Add DETACH command help text here>
.
Language=German
<Add DETACH command help text here>
.
Language=Polish
<Add DETACH command help text here>
.
Language=Portugese
<Add DETACH command help text here>
.
Language=Romanian
<Add DETACH command help text here>
.
Language=Russian
<Add DETACH command help text here>
.
Language=Albanian
<Add DETACH command help text here>
.
Language=Turkish
<Add DETACH command help text here>
.
Language=Chinese
<Add DETACH command help text here>
.
Language=Taiwanese
<Add DETACH command help text here>
.


MessageId=10023
SymbolicName=MSG_COMMAND_EXIT
Severity=Informational
Facility=System
Language=English
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=German
    Beendet den Befehlsinterpreter DiskPart.

Syntax:  EXIT

Beispiel:

    EXIT
.
Language=Polish
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Portugese
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Romanian
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Russian
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Albanian
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Turkish
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Chinese
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.
Language=Taiwanese
    Exits the DiskPart command interpreter.

Syntax:  EXIT

Example:

    EXIT
.


MessageId=10024
SymbolicName=MSG_COMMAND_EXPAND
Severity=Informational
Facility=System
Language=English
<Add EXPAND command help text here>
.
Language=German
<Add EXPAND command help text here>
.
Language=Polish
<Add EXPAND command help text here>
.
Language=Portugese
<Add EXPAND command help text here>
.
Language=Romanian
<Add EXPAND command help text here>
.
Language=Russian
<Add EXPAND command help text here>
.
Language=Albanian
<Add EXPAND command help text here>
.
Language=Turkish
<Add EXPAND command help text here>
.
Language=Chinese
<Add EXPAND command help text here>
.
Language=Taiwanese
<Add EXPAND command help text here>
.


MessageId=10025
SymbolicName=MSG_COMMAND_EXTEND
Severity=Informational
Facility=System
Language=English
<Add EXTEND command help text here>
.
Language=German
<Add EXTEND command help text here>
.
Language=Polish
<Add EXTEND command help text here>
.
Language=Portugese
<Add EXTEND command help text here>
.
Language=Romanian
<Add EXTEND command help text here>
.
Language=Russian
<Add EXTEND command help text here>
.
Language=Albanian
<Add EXTEND command help text here>
.
Language=Turkish
<Add EXTEND command help text here>
.
Language=Chinese
<Add EXTEND command help text here>
.
Language=Taiwanese
<Add EXTEND command help text here>
.


MessageId=10026
SymbolicName=MSG_COMMAND_FILESYSTEMS
Severity=Informational
Facility=System
Language=English
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=German
    Zeigt Informationen zum aktuelle Dateisystem für das ausgewählte
    Volume und die unterstützten Dateisysteme zum Formatieren des
    Volumes an.

Syntax:  FILESYSTEMS

    Damit diesr Vorgang erfolgreich ausgeführt werden kann, muss ein
    Volume ausgewählt sein.

Beispiel:

    FILESYSTEMS
.
Language=Polish
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Portugese
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Romanian
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Russian
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Albanian
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Turkish
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Chinese
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.
Language=Taiwanese
    Displays information about the current file system for the selected
    volume, and the supported file systems for formatting the volume.

Syntax:  FILESYSTEMS

    A volume must be selected for this operation to succeed.

Example:

    FILESYSTEMS
.


MessageId=10027
SymbolicName=MSG_COMMAND_FORMAT
Severity=Informational
Facility=System
Language=English
<Add FORMAT command help text here>
.
Language=German
<Add FORMAT command help text here>
.
Language=Polish
<Add FORMAT command help text here>
.
Language=Portugese
<Add FORMAT command help text here>
.
Language=Romanian
<Add FORMAT command help text here>
.
Language=Russian
<Add FORMAT command help text here>
.
Language=Albanian
<Add FORMAT command help text here>
.
Language=Turkish
<Add FORMAT command help text here>
.
Language=Chinese
<Add FORMAT command help text here>
.
Language=Taiwanese
<Add FORMAT command help text here>
.


MessageId=10028
SymbolicName=MSG_COMMAND_GPT
Severity=Informational
Facility=System
Language=English
<Add GPT command help text here>
.
Language=German
<Add GPT command help text here>
.
Language=Polish
<Add GPT command help text here>
.
Language=Portugese
<Add GPT command help text here>
.
Language=Romanian
<Add GPT command help text here>
.
Language=Russian
<Add GPT command help text here>
.
Language=Albanian
<Add GPT command help text here>
.
Language=Turkish
<Add GPT command help text here>
.
Language=Chinese
<Add GPT command help text here>
.
Language=Taiwanese
<Add GPT command help text here>
.


MessageId=10029
SymbolicName=MSG_COMMAND_HELP
Severity=Informational
Facility=System
Language=English
<Add HELP command help text here>
.
Language=German
<Add HELP command help text here>
.
Language=Polish
<Add HELP command help text here>
.
Language=Portugese
<Add HELP command help text here>
.
Language=Romanian
<Add HELP command help text here>
.
Language=Russian
<Add HELP command help text here>
.
Language=Albanian
<Add HELP command help text here>
.
Language=Turkish
<Add HELP command help text here>
.
Language=Chinese
<Add HELP command help text here>
.
Language=Taiwanese
<Add HELP command help text here>
.


MessageId=10030
SymbolicName=MSG_COMMAND_IMPORT
Severity=Informational
Facility=System
Language=English
<Add IMPORT command help text here>
.
Language=German
<Add IMPORT command help text here>
.
Language=Polish
<Add IMPORT command help text here>
.
Language=Portugese
<Add IMPORT command help text here>
.
Language=Romanian
<Add IMPORT command help text here>
.
Language=Russian
<Add IMPORT command help text here>
.
Language=Albanian
<Add IMPORT command help text here>
.
Language=Turkish
<Add IMPORT command help text here>
.
Language=Chinese
<Add IMPORT command help text here>
.
Language=Taiwanese
<Add IMPORT command help text here>
.


MessageId=10031
SymbolicName=MSG_COMMAND_INACTIVE
Severity=Informational
Facility=System
Language=English
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=German
    Markiert auf Datenträgern mit MBR-Datenträgerformat (Master Boot
    Record) die Partition, die den Fokus hat, als inaktive Partition.

Syntax:  INACTIVE

    Der Computer kann über die nächste im BIOS angegebene Option gestartet
    werden, z.B. von einem CD-ROM-Laufwerk oder aus einer PXE-basierten
    (Pre-Boot eXecution Environment) Startumgebung (z.B.
    Remoteinstallationsdienste, RIS), wenn der Computer neu startet.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss eine
    Partition ausgewählt sein.

    Vorsicht:

        Möglicherweise kann der Computer nicht gestartet werden, wenn es
        keine aktive Partition gibt. Markieren Sie eine System- oder
        Startpartition nur dann als inaktiv, wenn Sie ein erfahrener
        Benutzer und mit der ReactOS-Speicherverwaltung umfassend vertraut
        sind.

Beispiel:

    INACTIVE
.
Language=Polish
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Portugese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Romanian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Russian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Albanian
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Turkish
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Chinese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.
Language=Taiwanese
    On disks with master boot record (MBR) disk formatting, marks
    the partition with focus as inactive.

Syntax:  INACTIVE

    The computer may start from the next option specified in the BIOS such as a
    CD-ROM drive or a Pre-Boot eXecution Environment (PXE)-based boot
    environment (such as Remote Installation Services (RIS)) when you restart
    the computer.

    A partition must be selected for this operation to succeed.

    Caution:

        Your computer might not start without an active partition. Do not mark
        a system or boot partition as inactive unless you are an experienced
        user with a thorough understanding of Windows storage management.

Example:

    INACTIVE
.


MessageId=10032
SymbolicName=MSG_COMMAND_LIST_DISK
Severity=Informational
Facility=System
Language=English
<Add LIST DISK command help text here>
.
Language=German
<Add LIST DISK command help text here>
.
Language=Polish
<Add LIST DISK command help text here>
.
Language=Portugese
<Add LIST DISK command help text here>
.
Language=Romanian
<Add LIST DISK command help text here>
.
Language=Russian
<Add LIST DISK command help text here>
.
Language=Albanian
<Add LIST DISK command help text here>
.
Language=Turkish
<Add LIST DISK command help text here>
.
Language=Chinese
<Add LIST DISK command help text here>
.
Language=Taiwanese
<Add LIST DISK command help text here>
.

MessageId=10033
SymbolicName=MSG_COMMAND_LIST_PARTITION
Severity=Informational
Facility=System
Language=English
<Add LIST PARTITION command help text here>
.
Language=German
<Add LIST PARTITION command help text here>
.
Language=Polish
<Add LIST PARTITION command help text here>
.
Language=Portugese
<Add LIST PARTITION command help text here>
.
Language=Romanian
<Add LIST PARTITION command help text here>
.
Language=Russian
<Add LIST PARTITION command help text here>
.
Language=Albanian
<Add LIST PARTITION command help text here>
.
Language=Turkish
<Add LIST PARTITION command help text here>
.
Language=Chinese
<Add LIST PARTITION command help text here>
.
Language=Taiwanese
<Add LIST PARTITION command help text here>
.


MessageId=10034
SymbolicName=MSG_COMMAND_LIST_VOLUME
Severity=Informational
Facility=System
Language=English
<Add LIST VOLUME command help text here>
.
Language=German
<Add LIST VOLUME command help text here>
.
Language=Polish
<Add LIST VOLUME command help text here>
.
Language=Portugese
<Add LIST VOLUME command help text here>
.
Language=Romanian
<Add LIST VOLUME command help text here>
.
Language=Russian
<Add LIST VOLUME command help text here>
.
Language=Albanian
<Add LIST VOLUME command help text here>
.
Language=Turkish
<Add LIST VOLUME command help text here>
.
Language=Chinese
<Add LIST VOLUME command help text here>
.
Language=Taiwanese
<Add LIST VOLUME command help text here>
.


MessageId=10035
SymbolicName=MSG_COMMAND_LIST_VDISK
Severity=Informational
Facility=System
Language=English
<Add LIST VDISK command help text here>
.
Language=German
<Add LIST VDISK command help text here>
.
Language=Polish
<Add LIST VDISK command help text here>
.
Language=Portugese
<Add LIST VDISK command help text here>
.
Language=Romanian
<Add LIST VDISK command help text here>
.
Language=Russian
<Add LIST VDISK command help text here>
.
Language=Albanian
<Add LIST VDISK command help text here>
.
Language=Turkish
<Add LIST VDISK command help text here>
.
Language=Chinese
<Add LIST VDISK command help text here>
.
Language=Taiwanese
<Add LIST VDISK command help text here>
.


MessageId=10036
SymbolicName=MSG_COMMAND_MERGE
Severity=Informational
Facility=System
Language=English
<Add MERGE command help text here>
.
Language=German
<Add MERGE command help text here>
.
Language=Polish
<Add MERGE command help text here>
.
Language=Portugese
<Add MERGE command help text here>
.
Language=Romanian
<Add MERGE command help text here>
.
Language=Russian
<Add MERGE command help text here>
.
Language=Albanian
<Add MERGE command help text here>
.
Language=Turkish
<Add MERGE command help text here>
.
Language=Chinese
<Add MERGE command help text here>
.
Language=Taiwanese
<Add MERGE command help text here>
.


MessageId=10037
SymbolicName=MSG_COMMAND_OFFLINE
Severity=Informational
Facility=System
Language=English
<Add OFFLINE command help text here>
.
Language=German
<Add OFFLINE command help text here>
.
Language=Polish
<Add OFFLINE command help text here>
.
Language=Portugese
<Add OFFLINE command help text here>
.
Language=Romanian
<Add OFFLINE command help text here>
.
Language=Russian
<Add OFFLINE command help text here>
.
Language=Albanian
<Add OFFLINE command help text here>
.
Language=Turkish
<Add OFFLINE command help text here>
.
Language=Chinese
<Add OFFLINE command help text here>
.
Language=Taiwanese
<Add OFFLINE command help text here>
.


MessageId=10038
SymbolicName=MSG_COMMAND_ONLINE
Severity=Informational
Facility=System
Language=English
<Add ONLINE command help text here>
.
Language=German
<Add ONLINE command help text here>
.
Language=Polish
<Add ONLINE command help text here>
.
Language=Portugese
<Add ONLINE command help text here>
.
Language=Romanian
<Add ONLINE command help text here>
.
Language=Russian
<Add ONLINE command help text here>
.
Language=Albanian
<Add ONLINE command help text here>
.
Language=Turkish
<Add ONLINE command help text here>
.
Language=Chinese
<Add ONLINE command help text here>
.
Language=Taiwanese
<Add ONLINE command help text here>
.


MessageId=10039
SymbolicName=MSG_COMMAND_RECOVER
Severity=Informational
Facility=System
Language=English
<Add RECOVER command help text here>
.
Language=German
<Add RECOVER command help text here>
.
Language=Polish
<Add RECOVER command help text here>
.
Language=Portugese
<Add RECOVER command help text here>
.
Language=Romanian
<Add RECOVER command help text here>
.
Language=Russian
<Add RECOVER command help text here>
.
Language=Albanian
<Add RECOVER command help text here>
.
Language=Turkish
<Add RECOVER command help text here>
.
Language=Chinese
<Add RECOVER command help text here>
.
Language=Taiwanese
<Add RECOVER command help text here>
.


MessageId=10040
SymbolicName=MSG_COMMAND_REM
Severity=Informational
Facility=System
Language=English
<Add REM command help text here>
.
Language=German
<Add REM command help text here>
.
Language=Polish
<Add REM command help text here>
.
Language=Portugese
<Add REM command help text here>
.
Language=Romanian
<Add REM command help text here>
.
Language=Russian
<Add REM command help text here>
.
Language=Albanian
<Add REM command help text here>
.
Language=Turkish
<Add REM command help text here>
.
Language=Chinese
<Add REM command help text here>
.
Language=Taiwanese
<Add REM command help text here>
.


MessageId=10041
SymbolicName=MSG_COMMAND_REMOVE
Severity=Informational
Facility=System
Language=English
<Add REMOVE command help text here>
.
Language=German
<Add REMOVE command help text here>
.
Language=Polish
<Add REMOVE command help text here>
.
Language=Portugese
<Add REMOVE command help text here>
.
Language=Romanian
<Add REMOVE command help text here>
.
Language=Russian
<Add REMOVE command help text here>
.
Language=Albanian
<Add REMOVE command help text here>
.
Language=Turkish
<Add REMOVE command help text here>
.
Language=Chinese
<Add REMOVE command help text here>
.
Language=Taiwanese
<Add REMOVE command help text here>
.


MessageId=10042
SymbolicName=MSG_COMMAND_REPAIR
Severity=Informational
Facility=System
Language=English
<Add REPAIR command help text here>
.
Language=German
<Add REPAIR command help text here>
.
Language=Polish
<Add REPAIR command help text here>
.
Language=Portugese
<Add REPAIR command help text here>
.
Language=Romanian
<Add REPAIR command help text here>
.
Language=Russian
<Add REPAIR command help text here>
.
Language=Albanian
<Add REPAIR command help text here>
.
Language=Turkish
<Add REPAIR command help text here>
.
Language=Chinese
<Add REPAIR command help text here>
.
Language=Taiwanese
<Add REPAIR command help text here>
.


MessageId=10043
SymbolicName=MSG_COMMAND_RESCAN
Severity=Informational
Facility=System
Language=English
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=German
    Sucht nach neuen Datenträgern, die dem Computer möglicherweise
    hinzugefügt wurden.

Syntax:  RESCAN

Beispiel:

    RESCAN
.
Language=Polish
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Portugese
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Romanian
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Russian
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Albanian
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Turkish
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Chinese
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.
Language=Taiwanese
    Locates new disks that may have been added to the computer.

Syntax:  RESCAN

Example:

    RESCAN
.


MessageId=10044
SymbolicName=MSG_COMMAND_RETAIN
Severity=Informational
Facility=System
Language=English
<Add RETAIN command help text here>
.
Language=German
<Add RETAIN command help text here>
.
Language=Polish
<Add RETAIN command help text here>
.
Language=Portugese
<Add RETAIN command help text here>
.
Language=Romanian
<Add RETAIN command help text here>
.
Language=Russian
<Add RETAIN command help text here>
.
Language=Albanian
<Add RETAIN command help text here>
.
Language=Turkish
<Add RETAIN command help text here>
.
Language=Chinese
<Add RETAIN command help text here>
.
Language=Taiwanese
<Add RETAIN command help text here>
.


MessageId=10045
SymbolicName=MSG_COMMAND_SAN
Severity=Informational
Facility=System
Language=English
<Add SAN command help text here>
.
Language=German
<Add SAN command help text here>
.
Language=Polish
<Add SAN command help text here>
.
Language=Portugese
<Add SAN command help text here>
.
Language=Romanian
<Add SAN command help text here>
.
Language=Russian
<Add SAN command help text here>
.
Language=Albanian
<Add SAN command help text here>
.
Language=Turkish
<Add SAN command help text here>
.
Language=Chinese
<Add SAN command help text here>
.
Language=Taiwanese
<Add SAN command help text here>
.


MessageId=10046
SymbolicName=MSG_COMMAND_SELECT_DISK
Severity=Informational
Facility=System
Language=English
<Add SELECT DISK command help text here>
.
Language=German
<Add SELECT DISK command help text here>
.
Language=Polish
<Add SELECT DISK command help text here>
.
Language=Portugese
<Add SELECT DISK command help text here>
.
Language=Romanian
<Add SELECT DISK command help text here>
.
Language=Russian
<Add SELECT DISK command help text here>
.
Language=Albanian
<Add SELECT DISK command help text here>
.
Language=Turkish
<Add SELECT DISK command help text here>
.
Language=Chinese
<Add SELECT DISK command help text here>
.
Language=Taiwanese
<Add SELECT DISK command help text here>
.


MessageId=10047
SymbolicName=MSG_COMMAND_SELECT_PARTITION
Severity=Informational
Facility=System
Language=English
<Add SELECT PARTITION command help text here>
.
Language=German
<Add SELECT PARTITION command help text here>
.
Language=Polish
<Add SELECT PARTITION command help text here>
.
Language=Portugese
<Add SELECT PARTITION command help text here>
.
Language=Romanian
<Add SELECT PARTITION command help text here>
.
Language=Russian
<Add SELECT PARTITION command help text here>
.
Language=Albanian
<Add SELECT PARTITION command help text here>
.
Language=Turkish
<Add SELECT PARTITION command help text here>
.
Language=Chinese
<Add SELECT PARTITION command help text here>
.
Language=Taiwanese
<Add SELECT PARTITION command help text here>
.


MessageId=10048
SymbolicName=MSG_COMMAND_SELECT_VOLUME
Severity=Informational
Facility=System
Language=English
<Add SELECT VOLUME command help text here>
.
Language=German
<Add SELECT VOLUME command help text here>
.
Language=Polish
<Add SELECT VOLUME command help text here>
.
Language=Portugese
<Add SELECT VOLUME command help text here>
.
Language=Romanian
<Add SELECT VOLUME command help text here>
.
Language=Russian
<Add SELECT VOLUME command help text here>
.
Language=Albanian
<Add SELECT VOLUME command help text here>
.
Language=Turkish
<Add SELECT VOLUME command help text here>
.
Language=Chinese
<Add SELECT VOLUME command help text here>
.
Language=Taiwanese
<Add SELECT VOLUME command help text here>
.


MessageId=10049
SymbolicName=MSG_COMMAND_SELECT_VDISK
Severity=Informational
Facility=System
Language=English
<Add SELECT VDISK command help text here>
.
Language=German
<Add SELECT VDISK command help text here>
.
Language=Polish
<Add SELECT VDISK command help text here>
.
Language=Portugese
<Add SELECT VDISK command help text here>
.
Language=Romanian
<Add SELECT VDISK command help text here>
.
Language=Russian
<Add SELECT VDISK command help text here>
.
Language=Albanian
<Add SELECT VDISK command help text here>
.
Language=Turkish
<Add SELECT VDISK command help text here>
.
Language=Chinese
<Add SELECT VDISK command help text here>
.
Language=Taiwanese
<Add SELECT VDISK command help text here>
.


MessageId=10050
SymbolicName=MSG_COMMAND_SETID
Severity=Informational
Facility=System
Language=English
    Changes the partition type field for the partition with focus.

Syntax:  SET ID={<BYTE> | <GUID>} [OVERRIDE] [NOERR]

    ID={<BYTE> | <GUID>}

                Specifies the new partition type.
                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. Any
                partition type byte can be specified with this parameter except
                for type 0x42 (LDM partition). Note that the leading '0x' is 
                omitted when specifying the hexadecimal partition type.

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition. Recognized GUIDs
                include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                Any partition type GUID can be specified with this parameter
                except for the following:

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215a

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Cluster Metadata partition:
                        db97dba9-0840-4bae-97f0-ffb9a327c7e1


                Other than the limitations mentioned, DiskPart otherwise does
                not check the partition type for validity except to ensure that
                it is a byte in hexadecimal form or a GUID.

    OVERRIDE    Enables DiskPart to force the file system on the volume to
                dismount before changing the partition type. When changing
                the partition type, DiskPart will attempt to lock and dismount
                the file system on the volume. If this parameter is not specified,
                and the call to lock the file system fails, (because some other
                application has an open handle to the volume), the entire
                operation will fail. When this parameter is specified, the
                dismount is forced even if the call to lock the file system
                fails. When a file system is dismounted, all opened handles to
                the volume will become invalid.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    Intended for Original Equipment Manufacturer (OEM) use only.

    A partition must be selected for this operation to succeed.

    Caution:

        Changing partition type fields with this parameter might cause your
        computer to fail or be unable to start up. Unless you are an OEM or an
        IT professional experienced with GPT disks, do not change partition
        type fields on GPT disks using this parameter. Instead, always use the
        CREATE PARTITION EFI command to create EFI System partitions, the
        CREATE PARTITION MSR command to create Microsoft Reserved partitions,
        and the CREATE PARTITION PRIMARY command without the ID parameter to
        create primary partitions on GPT disks.

    This command does not work on dynamic disks nor on Microsoft Reserved
    partitions.

Example:
    SET ID=07 OVERRIDE
    SET ID=ebd0a0a2-b9e5-4433-87c0-68b6b72699c7
.
Language=German
<Add SETID command help text here>
.
Language=Polish
<Add SETID command help text here>
.
Language=Portugese
<Add SETID command help text here>
.
Language=Romanian
<Add SETID command help text here>
.
Language=Russian
<Add SETID command help text here>
.
Language=Albanian
<Add SETID command help text here>
.
Language=Turkish
<Add SETID command help text here>
.
Language=Chinese
<Add SETID command help text here>
.
Language=Taiwanese
<Add SETID command help text here>
.


MessageId=10051
SymbolicName=MSG_COMMAND_SHRINK
Severity=Informational
Facility=System
Language=English
<Add SHRINK command help text here>
.
Language=German
<Add SHRINK command help text here>
.
Language=Polish
<Add SHRINK command help text here>
.
Language=Portugese
<Add SHRINK command help text here>
.
Language=Romanian
<Add SHRINK command help text here>
.
Language=Russian
<Add SHRINK command help text here>
.
Language=Albanian
<Add SHRINK command help text here>
.
Language=Turkish
<Add SHRINK command help text here>
.
Language=Chinese
<Add SHRINK command help text here>
.
Language=Taiwanese
<Add SHRINK command help text here>
.


MessageId=10052
SymbolicName=MSG_COMMAND_UNIQUEID_DISK
Severity=Informational
Facility=System
Language=English
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=German
    Dient zum Anzeigen oder Festlegen des GPT-Bezeichners (GPT =
    GUID-Partitionstabelle) oder der MBR-Signatur (Master Boot Record)
    für den Datenträger, der den Fokus hat.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                Für MBR-Datenträger können Sie einen Wert mit einer
                Länge von vier Bytes (DWORD) im Hexadezimalformat für die
                Signatur angeben.

                Für GPT-Datenträger können Sie einen GUID für den
                Bezeichner angeben.

    NOERR       Nur für Skripting. Wenn ein Fehler auftritt, setzt DiskPart
                die Verarbeitung von Befehlen fort, als sei der Fehler nicht
                aufgetreten. Ohne den Parameter NOERR wird DiskPart bei
                einem Fehler mit dem entsprechenden Fehlercode beendet.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein
    Datenträger ausgewählt sein. Der Befehl kann für Basisdatenträger und
    dynamische Datenträger verwendet werden.

Beispiel:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Polish
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Portugese
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Romanian
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Russian
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Albanian
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Turkish
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Chinese
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
Language=Taiwanese
    Displays or sets the GUID partition table (GPT) identifier or master boot
    record (MBR) signature for the disk with focus.

Syntax:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                For MBR disks, you can specify a four-byte (DWORD) value in
                hexadecimal form for the signature.

                For GPT disks, specify a GUID for the identifier.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A disk must be selected for this operation to succeed.  This command works
    on basic and dynamic disks.

Example:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
