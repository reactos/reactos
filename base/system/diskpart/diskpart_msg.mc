
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
    Na dyskach zawierających formatowanie z głównym rekordem rozruchowym
    (MBR) oznacza partycję, na której jest ustawiony fokus, jako aktywną.

Składnia:  ACTIVE

    Zapisuje na dysku wartość odczytywaną podczas rozruchu przez podstawowy
    system wejścia/wyjścia (BIOS). Ta wartość oznacza, że dana partycja jest
    prawidłową partycją systemową.

    Aby ta operacja powiodła się, partycja musi być wybrana.

    Uwaga:

        Program DiskPart sprawdza tylko, czy partycja może zawierać pliki
        startowe systemu operacyjnego. Program DiskPart nie sprawdza zawartości
        partycji. Jeśli przez pomyłkę jako aktywna zostanie oznaczona partycja
        niezawierająca plików startowych systemu operacyjnego, komputer może się
        nie uruchomić.

Przykład:

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
    在具有主開機記錄 (MBR) 磁碟格式的磁碟上，
    將已選擇的磁碟分割標示為使用中。

語法:  ACTIVE

    寫入值到磁碟，而該磁碟會在開機時由基本輸入/輸出系統 (BIOS) 讀取。
    這個值會指定磁碟分割為有效的系統磁碟分割。

    您必須先選擇磁碟分割，才能完成這個操作。

    注意:

        DiskPart 只會檢查該磁碟分割是否可以包含作業系統的啟動檔案。
        DiskPart 不會檢查該磁碟分割的內容。
        如果您錯誤地將未包含作業系統的啟動檔案的磁碟分割標示為使用中，
        您的電腦可能會無法啟動。

範例:

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
    Usuwa wszelkie formatowanie partycji lub woluminów z dysku mającego
    fokus.

Składnia:  CLEAN [ALL]

    ALL         Określa, że każdy bajt/sektor na dysku zostaje ustawiony
                na zero, co całkowicie usuwa wszystkie dane zawarte na dysku.

    Na dyskach z głównym rekordem rozruchowym (MBR) są zastępowane tylko
    informacje o partycjonowaniu MBR i ukrytych sektorach. Na dyskach
    z tablicą partycji GUID (GPT) są zastępowane informacje
    o partycjonowaniu GPT, łącznie z ochronnym rekordem MBR. Jeśli parametr
    ALL nie zostanie użyty, 1 MB na początku i na końcu dysku jest zerowany. 
    Powoduje to wymazanie formatowania poprzednio stosowanego na dysku. 
    Stanem dysku po jego czyszczeniu jest stan 'NIEZAINICJOWANY'.

Przykład:

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
    從已選擇的磁碟上移除所有磁碟分割或磁碟區格式。

語法:  CLEAN [ALL]

    ALL         指定將磁碟上每個位元組/磁區設定為零，
                即完全刪除磁碟上包含的所有資料。

    在主開機記錄 (MBR) 磁碟上，只會覆寫 MBR 磁碟分割資訊和隱藏的磁區資訊。
    在 GUID 磁碟分割表格 (GPT) 磁碟上，會覆寫 GPT 磁碟分割資訊，
    包括保護性 MBR。
    如果沒有使用 ALL 參數，會將磁碟的最前 1MB 和最後 1MB 設定為零。
    這會清除先前套用到該磁碟的任何磁碟格式。
    清除磁碟以後，磁碟狀態會變為 '未初始化'。

範例:

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
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=German
    Erstellt eine erweiterte Partition auf dem Datenträger, der den Fokus
    besitzt.
    Gilt nur für MBR-Datenträger (Master Boot Record).

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    Die Größe der Partition in MB. Falls keine Größe angegeben
                ist, wird die Partition erweitert, bis auf der erweiterten
                Partition kein freier Speicherplatz mehr vorhanden ist.

    OFFSET=<N>  Das Offset, in Kilobyte (KB), in dem die Partition
                erstellt wird. Wird kein Offset angegeben, beginnt die
                Partition am Anfang des ersten freien Speicherplatz auf dem
                Datenträger, der eine ausreichende Größe für die neue Partition
                besitzt.

    ALIGN=<N>   Wird normalerweise bei Hardware-RAID-Arrays mit logischen
                Gerätenummern (LUN) zur Verbesserung der Leistung verwendet. Das
                Offset der Partition ist ein Vielfaches von <N>. Bei Angabe des
                Parameters OFFSET wird dieser auf das nächste Vielfache von <N>
                gerundet.

    NOERR       Nur für Skripting. Bei einem Fehler setzt DiskPart die
                Verarbeitung von Befehlen fort, als sei der Fehler nicht
                aufgetreten.
                Ohne den Parameter NOERR wird DiskPart bei einem Fehler mit
                dem entsprechenden Fehlercode beendet.

    Nachdem die Partition erstellt wurde, wird der Fokus automatisch auf die
    neue Partition gesetzt. Auf jedem Datenträger kann jeweils nur eine
    erw. Partition erstellt werden. Dieser Befehl kann nicht ausgeführt
    werden, wenn versucht wird, eine erweiterte Partition innerhalb einer
    anderen erweiterten Partition zu erstellen. Sie müssen zuerst eine
    erweiterte Partition erstellen, bevor logische Partitionen erstellt werden
    können.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein
    MBR-Basisdatenträger ausgewählt werden.

Beispiel:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Polish
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Portugese
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Romanian
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Russian
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Albanian
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Turkish
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Chinese
    Creates an extended partition on the disk with focus.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more free
                space in the extended partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is
                created. If no offset is given, the partition will start
                at the beginning of the first free space on the disk that
                is large enough to hold the new partition.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new partition. Only one extended partition can be created per disk. This
    command fails if you attempt to create an extended partition within another
    extended partition. You must create an extended partition before you can
    create logical partitions.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION EXTENDED SIZE=1000
.
Language=Taiwanese
    在已選擇的磁碟上建立一個延伸磁碟分割。
	只適用於主開機記錄 (MBR) 磁碟。

語法:  CREATE PARTITION EXTENDED [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    磁碟分割大小 (單位為 MB)。如果沒有提供大小，
                磁碟分割會繼續，直至目前區域沒有任何可用空間。

    OFFSET=<N>  建立磁碟分割所在的位移 (單位為 KB)。如果沒有指定位移，
                磁碟分割會從磁碟上可容納新磁碟分割的
                第一個可用空間的開頭開始。

    ALIGN=<N>   通常與硬體 RAID 邏輯單元編號 (LUN) 陣列搭配使用以增進效能。
                延伸位移將會是 <N> 的倍數。如果指定了 OFFSET 參數，
                它會取至最接近的 <N> 的倍數。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    在磁碟分割建立完成後，焦點將自動給予新磁碟分割。
    每個磁碟只能建立一個延伸磁碟分割。如果您嘗試在一個延伸磁碟分割中
    建立另一個延伸磁碟分割，命令將會執行失敗。
    您必須先建立一個延伸磁碟分割，然後才能建立邏輯磁碟分割。

    您必須先選擇基本 MBR 磁碟，才能完成這個操作。

範例:

    CREATE PARTITION EXTENDED SIZE=1000
.


MessageId=10013
SymbolicName=MSG_COMMAND_CREATE_PARTITION_LOGICAL
Severity=Informational
Facility=System
Language=English
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=German
    Erstellt eine logische Partition in einer erweiterten Partition.
    Gilt nur für MBR-Datenträger (Master Boot Record).

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    Die Größe der Partition in MB. Die Länge der Partition in MB.
                Die Länge der Partition in Byte entspricht mindestens dem durch
                N angegebenen Wert. Wenn Sie eine Größe für die logische
                Partition angeben, muss diese kleiner sein als die erweiterte
                Partition. Falls keine Größe angegeben ist, wird die Partition
                erweitert, bis sie den gesamten freien Speicherplatz im
                Bereich umfasst.

    OFFSET=<N>  Das Offset, in Kilobyte (KB), an dem die Partition erstellt
                wird. Falls kein Offset angegeben ist, wird die Partition im
                ersten Datenträgerbereich erstellt, der eine ausreichende Größe
                für die Partition hat.

    ALIGN=<N>   Wird normalerweise bei Hardware-RAID-Arrays mit logischen
                Gerätenummern (LUN) zur Verbesserung der Leistung verwendet. Das
                Offset der Partition ist ein Vielfaches von <N>. Bei Angabe des
                Parameters OFFSET wird dieser auf das nächste Vielfache von <N>
                gerundet.

    NOERR       Nur für Skripting. Bei einem Fehler setzt DiskPart die
                Verarbeitung von Befehlen fort, als sei der Fehler nicht
                aufgetreten. Ohne den Parameter NOERR wird DiskPart bei einem
                Fehler mit dem entsprechenden Fehlercode beendet.

    Nachdem die Partition erstellt wurde, wird der Fokus automatisch auf die
    neue logische Partition gesetzt.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein
    MBR-Basisdatenträger ausgewählt sein.

Beispiel:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Polish
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Portugese
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Romanian
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Russian
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Albanian
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Turkish
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Chinese
    Creates a logical partition in an extended partition.
    Applies to master boot record (MBR) disks only.

Syntax:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). The partition is
                at least as big in bytes as the number specified by N. If you
                specify a size for the logical partition, it must be smaller
                than the extended partition. If no size is given, the partition
                continues until there is no more free space in the extended
                partition.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After the partition has been created, the focus automatically shifts to the
    new logical partition.

    A basic MBR disk must be selected for this operation to succeed.

Example:

    CREATE PARTITION LOGICAL SIZE=1000
.
Language=Taiwanese
    在延伸磁碟分割內建立一個邏輯磁碟分割。
    只適用於主開機記錄 (MBR) 磁碟。

語法:  CREATE PARTITION LOGICAL [SIZE=<N>] [OFFSET=<N>] [ALIGN=<N>] [NOERR]

    SIZE=<N>    磁碟分割大小 (單位為 MB)。磁碟分割的大小至少為 N 所指定的
                大小 (位元組)。如果您指定邏輯磁碟分割的大小，它必須小於
                延伸磁碟分割。如果沒有提供大小，磁碟分割會繼續，
                直至延伸磁碟分割沒有任何可用空間。

    OFFSET=<N>  建立磁碟分割所在的位移 (單位為 KB)。如果沒有指定位移，
                磁碟分割會從磁碟上可容納新磁碟分割的
                第一個可用空間的開頭開始。

    ALIGN=<N>   通常與硬體 RAID 邏輯單元編號 (LUN) 陣列搭配使用以增進效能。
                延伸位移將會是 <N> 的倍數。如果指定了 OFFSET 參數，
                它會取至最接近的 <N> 的倍數。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    在磁碟分割建立完成後，焦點將自動給予新的邏輯磁碟分割。

    您必須先選擇基本 MBR 磁碟，才能完成這個操作。

範例:

    CREATE PARTITION LOGICAL SIZE=1000
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
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=German
    Erstellt eine primäre Partition auf dem Basisdatenträger, der den Fokus
    hat.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>]
            [ID={<BYTE> | <GUID>}] [ALIGN=<N>] [NOERR]

    SIZE=<N>    Die Größe der Partition in MB. Falls keine Größe angegeben ist,
                wird die Partition erweitert, bis sie den gesamten
                verfügbaren Speicherplatz im aktuellen Bereich umfasst.

    OFFSET=<N>  Das Offset, in Kilobyte (KB), an dem die Partition erstellt
                werden soll. Falls kein Offset angegeben ist, wird die
                Partition in der ersten Datenträgererweiterung platziert, die
                eine ausreichende Größe besitzt.

    ID={<BYTE> | <GUID>}

                Gibt den Partitionstyp an.

                Zur ausschließlichen Verwendung durch Originalgerätehersteller.

                Für MBR-Datenträger (Master Boot Record) können Sie für
                die Partition ein Partitionstypbyte im Hexadezimalformat
                angeben. Falls dieser Parameter für einen MBR-Datenträger
                nicht angegeben ist, erstellt der Befehl eine Partition des
                Typs 0x06 (gibt an, dass kein Dateisystem installiert ist).

                    LDM-Datenpartition:
                        0x42

                    Wiederherstellungspartition:
                        0x27

                    Erkannte OEM-IDs:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                Für GPT-Datenträger (GPT = GUID-Partitionstabelle) können Sie
                einen Partitionstyp-GUID für die zu erstellende Partition
                angeben. Zu den erkannten GUIDs gehören:

                    EFI-Systempartition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    MSR-Partition (Microsoft Reserved):
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basisdatenpartition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM-Metadatenpartition auf einem dynamischen Datenträger:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM-Datenpartition auf einem dynamischen Datenträger:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Wiederherstellungspartition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                Wenn dieser Parameter für einen GPT-Datenträger nicht angegeben
                ist, wird mit diesem Befehl eine Basisdatenpartition erstellt.

                Mit diesem Parameter kann ein beliebiger Partitionstyp
                oder ein beliebiger GUID angegeben werden. DiskPart überprüft
                die Gültigkeit dieses Partitions-GUIDs nicht. Es wird lediglich
                sichergestellt, dass es sich um ein Byte im Hexadezimalformat
                oder um einen GUID handelt.

                Vorsicht:

                    Das Erstellen von Partitionen mit diesem Parameter kann
                    dazu führen, dass der Computer fehlerhaft arbeitet oder
                    nicht mehr gestartet werden kann. Sofern Sie kein Original-
                    gerätehersteller oder IT-Fachmann sind, der mit GPT-Daten-
                    trägern vertraut ist, sollten Sie keine Partitionen auf
                    GPT-Datenträgern mit diesem Parameter erstellen.
                    Verwenden Sie stattdessen immer den Befehl
                    CREATE PARTITION EFI zum Erstellen von EFI-System-
                    Partitionen, den Befehl CREATE PARTITION MSR zum Erstellen
                    von MSR-Partitionen und den Befehl
                    CREATE PARTITION PRIMARY (ohne diesen Parameter) zum
                    Erstellen von primären Partitionen auf GPT-Datenträgern.

    ALIGN=<N>   Wird normalerweise bei Hardware-RAID-Arrays mit logischen
                Gerätenummern (LUN) zur Verbesserung der Leistung verwendet. Das
                Offset der Partition ist ein Vielfaches von <N>. Bei Angabe des
                Parameters OFFSET wird dieser auf das nächste Vielfache von <N>
                gerundet.

    NOERR       Nur für Skripting. Bei einem Fehler setzt DiskPart die
                Verarbeitung von Befehlen fort, als sei der Fehler nicht
                aufgetreten. Ohne den Parameter NOERR wird DiskPart bei
                einem Fehler mit dem entsprechenden Fehlercode beendet.

    Nachdem Sie die Partition erstellt haben, wird der Fokus automatisch auf
    die neue Partition gesetzt. Diese Partition erhält keinen Laufwerkbuch-
    staben. Sie müssen der Partition mit dem Befehl ASSIGN einen Laufwerkbuch-
    staben zuweisen.

    Damit dieser Vorgang erfolgreich durchgeführt werden kann, muss ein Basis-
    datenträger ausgewählt sein. Falls kein Partitionstyp angegeben ist, wird die Initialisierung
    des Datenträgers aufgehoben, und wenn der Datenträger größer ist als 2 TB,
    wird er für GPT initialisiert.

Beispiel:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Polish
    Tworzy partycję podstawową na dysku podstawowym mającym fokus.

Składnia:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    Rozmiar partycji w megabajtach (MB). Jeśli nie podano
                rozmiaru, partycja zajmuje całe nieprzydzielone miejsce w
                bieżącym obszarze.

    OFFSET=<N>  Przesunięcie w kilobajtach, przy którym zostanie utworzona.
                partycja. Jeśli nie podano przesunięcia, partycja zostanie
                umieszczona w pierwszym zakresie dysku, w którym się zmieści.

    ID={<BYTE> | <GUID>}

                Określa typ partycji.

                Parametr przeznaczony tylko do użytku przez producentów
                oryginalnego sprzętu (OEM).

                W przypadku dysków z głównym rekordem rozruchowym (MBR) można
                dla partycji określić bajt typu partycji (szesnastkowo). Jeśli
                ten parametr nie zostanie określony dla dysku MBR, polecenie
                powoduje utworzenie partycji typu 0x06 (określa, że brak jest
                zainstalowanego systemu plików).

                    Partycja danych LDM:
                        0x42

                    Partycja odzyskiwania:
                        0x27

                    Rozpoznawane identyfikatory OEM:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                W przypadku dysków z tablicą partycji GUID (GPT) można
                określić identyfikator GUID typu partycji, która ma zostać
                utworzona. Rozpoznawane identyfikatory GUID są następujące:

                    Partycja systemowa EFI:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Partycja zastrzeżona firmy Microsoft:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Podstawowa partycja danych:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    Partycja metadanych LDM na dysku dynamicznym:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    Partycja danych LDM na dysku dynamicznym:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Partycja odzyskiwania:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                Jeśli ten parametr nie zostanie określony dla dysku GPT,
                polecenie utworzy podstawową partycję danych.

                Za pomocą tego parametru można określić dowolny bajt
                typu partycji lub identyfikator GUID. Program DiskPart nie
                sprawdza poprawności typu partycji. Sprawdza jedynie, czy
                podana wartość ma postać szesnastkową lub jest
                identyfikatorem GUID.

                Uwaga:

                    Tworzenie partycji z tym parametrem może sprawić, że
                    komputer ulegnie awarii lub nie będzie można go
                    uruchomić. Jeśli użytkownik nie jest producentem OEM ani
                    informatykiem dysponującym doświadczeniem w zakresie
                    dysków GPT, nie powinien tworzyć partycji na dyskach GPT
                    przy użyciu tego parametru. Zamiast tego należy zawsze
                    używać polecenia CREATE PARTITION EFI do tworzenia
                    partycji systemowych EFI, polecenia CREATE PARTITION MSR
                    do tworzenia partycji zastrzeżonej firmy Microsoft
                    i polecenia CREATE PARTITION PRIMARY (bez opisywanego
                    parametru) do tworzenia partycji podstawowych
                    na dyskach GPT.

    ALIGN=<N>   Zazwyczaj używany z numerami jednostek logicznych (LUN)
                sprzętowych macierzy RAID w celu poprawienia wydajności.
                Przesunięcie partycji będzie wielokrotnością liczby N.
                Jeśli nie określono parametru OFFSET, jest ono zaokrąglane
                do najbliższej wielokrotności liczby N.

    NOERR       Tylko do obsługi skryptów. Po wystąpieniu błędu program
                DiskPart kontynuuje przetwarzanie poleceń tak, jakby błąd
                nie wystąpił. W przypadku braku parametru NOERR błąd powoduje
                zakończenie działania programu DiskPart i zwrócenie
                kodu błędu.

    Po utworzeniu partycji automatycznie otrzymuje ona fokus. Partycja nie
    otrzymuje litery dysku. Aby przypisać partycji literę dysku,
    należy użyć polecenia ASSIGN.

    Aby ta operacja się powiodła, musi być wybrany dysk podstawowy.

    Jeśli typ partycji nie jest określony, dysk jest niezainicjowany a rozmiar
    rozmiar większy niż 2 TB, zostanie zainicjowany do GPT.

Przykład:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Portugese
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Romanian
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Russian
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Albanian
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Turkish
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Chinese
    Creates a primary partition on the basic disk with focus.

Syntax:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    The size of the partition in megabytes (MB). If no size is
                given, the partition continues until there is no more
                unallocated space in the current region.

    OFFSET=<N>  The offset, in kilobytes (KB), at which the partition is created.
                If no offset is given, the partition is placed in the first disk
                extent that is large enough to hold it.

    ID={<BYTE> | <GUID>}

                Specifies the partition type.

                Intended for Original Equipment Manufacturer (OEM) use only.

                For master boot record (MBR) disks, you can specify a partition
                type byte, in hexadecimal form, for the partition. If this
                parameter is not specified for an MBR disk, the command creates
                a partition of type 0x06 (specifies no file system is installed).

                    LDM data partition:
                        0x42

                    Recovery partition:
                        0x27

                    Recognized OEM Ids:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                For GUID partition table (GPT) disks you can specify a
                partition type GUID for the partition you want to create.
                Recognized GUIDs include:

                    EFI System partition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft Reserved partition:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    Basic data partition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    LDM Metadata partition on a dynamic disk:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM Data partition on a dynamic disk:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Recovery partition:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                If this parameter is not specified for a GPT disk, the command
                creates a basic data partition.

                Any partition type byte or GUID can be specified with this
                parameter. DiskPart does not check the partition type for
                validity except to ensure that it is a byte in hexadecimal form
                or a GUID.

                Caution:

                    Creating partitions with this parameter might cause your
                    computer to fail or be unable to start up. Unless you are
                    an OEM or an IT professional experienced with GPT disks, do
                    not create partitions on GPT disks using this parameter.
                    Instead, always use the CREATE PARTITION EFI command to
                    create EFI System partitions, the CREATE PARTITION MSR
                    command to create Microsoft Reserved partitions, and the
                    CREATE PARTITION PRIMARY command without this parameter to
                    create primary partitions on GPT disks.

    ALIGN=<N>   Typically used with hardware RAID Logical Unit Number (LUN)
                arrays to improve performance. The partition offset will be
                a multiple of <N>. If the OFFSET parameter is specified, it
                will be rounded to the closest multiple of <N>.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    After you create the partition, the focus automatically shifts to the new
    partition. The partition does not receive a drive letter. You must use the
    assign command to assign a drive letter to the partition.

    A basic disk must be selected for this operation to succeed.

    If a partition type is not specified, the disk is uninitialized and disk
    size is greater than 2TB, it will be initialized to GPT.

Example:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
.
Language=Taiwanese
    在已選擇的基本磁碟上建立主要磁碟分割。

語法:  CREATE PARTITION PRIMARY [SIZE=<N>] [OFFSET=<N>] [ID={<BYTE> | <GUID>}]
            [ALIGN=<N>] [NOERR]

    SIZE=<N>    磁碟分割大小 (單位為 MB)。如果沒有提供大小，磁碟分割會繼續，
                直至目前區域沒有任何可用空間。

    OFFSET=<N>  建立磁碟分割所在的位移 (單位為 KB)。如果沒有指定位移，
                磁碟分割會從磁碟上可容納新磁碟分割的
                第一個可用空間的開頭開始。

    ID={<BYTE> | <GUID>}

                指定磁碟分割類型。

                只供原始設備製造商 (OEM) 使用。

                對於主開機記錄 (MBR) 磁碟，您可以使用十六進位格式來指定該
                磁碟分割的磁碟分割類型位元組。如果沒有為 MBR 磁碟指定
                這個參數，這個命令會建立類型為 06 的磁碟分割。
                (指定沒有安裝檔案系統)。

                    LDM 資料磁碟分割:
                        0x42

                    修復磁碟分割:
                        0x27

                    可識別的 OEM 識別碼:
                        0x12
                        0x84
                        0xDE
                        0xFE
                        0xA0

                對於 GUID 磁碟分割表格 (GPT) 磁碟，您可以為想要建立的
                磁碟分割指定磁碟分割類型 GUID。
                可識別的 GUID 包括:

                    EFI 系統磁碟分割:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Microsoft 保留磁碟分割:
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    基本資料磁碟分割:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                    動態磁碟上的 LDM 中繼資料磁碟分割:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    動態磁碟上的 LDM 資料磁碟分割:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    修復磁碟分割:
                        de94bba4-06d1-4d40-a16a-bfd50179d6ac

                如果沒有為 GPT 磁碟指定這個參數，
                這個命令會建立基本資料磁碟分割。

                您可以使用這個參數指定任何磁碟分割類型位元組或 GUID。
                DiskPart 不會檢查磁碟分割類型的有效性，但會確保
                磁碟分割類型是否為以十六進位格式或 GUID 表示的位元組。 

                注意:

                    使用這個參數建立磁碟分割，可能會造成您的電腦失敗或
                    無法啟動。除非您是熟悉 GPT 磁碟的 OEM 或 IT 專業人員，
                    否則請勿在 GPT 磁碟上建立磁碟分割時使用這個參數。
                    相反地，您應該使用 CREATE PARTITION EFI 命令來建立
                    EFI 系統磁碟分割、使用 CREATE PARTITION MSR 命令來建立
                    Microsoft 保留磁碟分割，及使用 CREATE PARTITION PRIMARY
                    命令 (而不使用此參數) 在 GPT 磁碟上建立主要磁碟分割。

    ALIGN=<N>   通常與硬體 RAID 邏輯單元編號 (LUN) 陣列搭配使用以增進效能。
                延伸位移將會是 <N> 的倍數。如果指定了 OFFSET 參數，
                它會取至最接近的 <N> 的倍數。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    在磁碟分割建立完成後，焦點將自動給予新的磁碟分割。該磁碟分割尚未指定
    磁碟機代號。您必須使用 ASSIGN 命令來分配磁碟機代號到該磁碟分割。

    您必須先選擇基本磁碟，才能完成這個操作。

    如果未有指定磁碟分割類型，而磁碟尚未初始化且磁碟大小大於 2TB，
    磁碟將會初始化為 GPT 磁碟。

範例:

    CREATE PARTITION PRIMARY SIZE=1000
    CREATE PARTITION PRIMARY SIZE=128 ID=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    CREATE PARTITION PRIMARY SIZE=10000 ID=12
    CREATE PARTITION PRIMARY SIZE=10000 ID=DE
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
Usuwa partycję, na której jest ustawiony fokus.

Składnia:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       Tylko do obsługi skryptów. Po wystąpieniu błędu program
                DiskPart kontynuuje przetwarzanie poleceń tak, jakby błąd
                nie wystąpił. W przypadku braku parametru NOERR błąd powoduje
                zakończenie działania programu DiskPart i zwrócenie
                kodu błędu.

    OVERRIDE    Umożliwia usunięcie przez program DiskPart każdej partycji bez
                względu na jej typ. Zazwyczaj program DiskPart zezwala
                na usunięcie wyłącznie znanych partycji zawierających dane.

    Nie można usunąć partycji systemowej, partycji rozruchowej ani żadnej
    partycji zawierającej aktywny plik stronicowania lub plik zrzutu
    awaryjnego (zrzutu pamięci).

    Aby ta operacja się powiodła, musi być wybrana partycja.

    Partycji nie można usuwać z dysków dynamicznych ani tworzyć ich na takich
    dyskach.

Przykład:

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
刪除已選擇的磁碟分割。

語法:  DELETE PARTITION [NOERR] [OVERRIDE]

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    OVERRIDE    讓 DiskPart 刪除所有類型的磁碟分割。
                一般來說，DiskPart 只允許您刪除已知的資料磁碟分割。

    您不能刪除系統磁碟分割、開機磁碟分割，或任何包含使用中的
    分頁檔或損毀傾印 (記憶體傾印) 檔案的磁碟分割。

    您必須先選擇一個磁碟分割，才能完成這個操作。

    您無法從動態磁碟刪除磁碟分割，也無法在動態磁碟上建立磁碟分割。

範例:

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
    Wyświetla właściwości wybranego dysku oraz listę znajdujących się na nim
    woluminów.

Składnia:  DETAIL DISK

    Do pomyślnego ukończenia operacji konieczne jest wybranie dysku.

Przykład:

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
    顯示在該磁碟上已選擇之磁碟的內容及列出磁碟區。 

語法:  DETAIL DISK

    您必須先選擇一個磁碟，才能完成這個操作。

範例:

    DETAIL DISK
.

MessageId=10020
SymbolicName=MSG_COMMAND_DETAIL_PARTITION
Severity=Informational
Facility=System
Language=English
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=German
    Zeigt die Eigenschaften für die ausgewählte Partition an.

Syntax:  DETAIL PARTITION

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss eine
    Partition ausgewählt sein.

Beispiel:

    DETAIL PARTITION
.
Language=Polish
    Wyświetla właściwości wybranej partycji.

Składnia:  DETAIL PARTITION

    Do pomyślnego ukończenia operacji konieczne jest wybranie partycji.

Przykład:

    DETAIL PARTITION
.
Language=Portugese
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Romanian
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Russian
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Albanian
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Turkish
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Chinese
    Displays the properties for the selected partition.

Syntax:  DETAIL PARTITION

    A partition must be selected for this operation to succeed.

Example:

    DETAIL PARTITION
.
Language=Taiwanese
    顯示已選擇之磁碟分割的內容。 

語法:  DETAIL PARTITION

    您必須先選擇一個磁碟分割，才能完成這個操作。

範例:

    DETAIL PARTITION
.

MessageId=10021
SymbolicName=MSG_COMMAND_DETAIL_VOLUME
Severity=Informational
Facility=System
Language=English
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=German
    Zeigt die Eigenschaften für das ausgewählte Volume und die Liste der
    Datenträger, die sich auf dem Volume befinden, an.

Syntax:  DETAIL VOLUME

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein Volume
    ausgewählt sein.

Beispiel:

    DETAIL VOLUME
.
Language=Polish
    Wyświetla właściwości wybranego woluminu oraz listę dysków, na których
    wolumin się znajduje.

Składnia:  DETAIL VOLUME

    Do pomyślnego ukończenia operacji konieczne jest wybranie woluminu.

Przykład:

    DETAIL VOLUME
.
Language=Portugese
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Romanian
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Russian
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Albanian
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Turkish
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Chinese
    Displays the properties for the selected volume and the list of disks on
    which the volume resides.

Syntax:  DETAIL VOLUME

    A volume must be selected for this operation to succeed.

Example:

    DETAIL VOLUME
.
Language=Taiwanese
    顯示已選擇之磁碟區的內容，及磁碟區所在的磁碟清單。

語法:  DETAIL VOLUME

    您必須先選擇一個磁碟區，才能完成這個操作。

範例:

    DETAIL VOLUME
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
    Kończy działanie interpretera poleceń programu DiskPart.

Składnia:  EXIT

Przykład:

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
    結束 DiskPart 命令直譯器。

語法:  EXIT

範例:

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
    Wyświetla informacje o bieżącym systemie plików w wybranym woluminie
    i systemach plików obsługiwanych przy formatowaniu woluminu.

Składnia:  FILESYSTEMS

    Aby ta operacja się powiodła, musi być wybrany wolumin.

Przykład:

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
    顯示所選取磁碟區之目前檔案系統的相關資訊，
    及可支援的檔案系統，以格式化該磁碟區。

語法:  FILESYSTEMS

    您必須先選擇一個磁碟區，才能完成這個操作。

範例:

    FILESYSTEMS
.


MessageId=10027
SymbolicName=MSG_COMMAND_FORMAT
Severity=Informational
Facility=System
Language=English
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=German
    Formatiert das angegebene Volume für die Verwendung mit ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Gibt den Typ des Dateisystems an. Falls kein Dateisystem
                angegeben wurde, wird das durch den Befehl
                "FILESYSTEMS" angezeigte Standarddateisystem
                verwendet.

    REVISION=<X.XX>

                Gibt die Dateisystemversion an (sofern zutreffend).

    RECOMMENDED Sofern angegeben, werden anstelle der Standardein-
                stellung das empfohlene Dateisystem und die Version verwendet,
                falls eine Empfehlung vorhanden ist. Das empfohlene Dateisystem
                (sofern vorhanden) wird durch den Befehl "FILESYSTEMS"
                angezeigt.

    LABEL=<"label">
                Gibt die Volumebezeichnung an.

    UNIT=<N>    Überschreibt die standardmäßige Größe der
                Zuteilungseinheit. Für die allgemeine Verwendung
                werden dringend die Standardeinstellungen
                empfohlen.
                Die standardmäßige Größe der Zuteilungseinheit
                für ein bestimmtes Dateisystem wird durch den
                Befehl "FILESYSTEMS" angezeigt.

                NTFS-Komprimierung wird für Zuteilungseinheitsgrößen
                über 4096 nicht unterstützt.

    QUICK       Führt eine Schnellformatierung aus.

    COMPRESS    nur NTFS: Auf dem neuen Volume erstellte Dateien
                werden standardmäßig komprimiert.

    OVERRIDE    Erzwingt ggf. die Aufhebung der Bereitstellung des
                Dateisystems als ersten Schritt. Alle geöffneten Handles für
                das Volume besäßen keine Gültigkeit mehr.

    DUPLICATE   nur UDF: Dieses Kennzeichen gilt für das UDF-Format,
                Version 2.5 oder höher. Durch dieses Kennzeichen wird der
                Formatvorgang angewiesen, die Dateisystem-Metadaten
                auf einem zweiten Sektorensatz auf dem Datenträger zu
                duplizieren. Die duplizierten Metadaten werden von
                Anwendungen verwendet, beispielsweise für Reparatur-
                oder Wiederherstellungsanwendungen.
                Wird festgestellt, dass primäre Metadatensektoren beschädigt
                sind, werden die Dateisystem-Metadaten aus den doppelten
                Sektoren gelesen.

    NOWAIT      Erzwingt die sofortige Rückgabe des Befehls, während der
                Formatierungsprozess noch stattfindet. Falls "NOWAIT" nicht
                angegeben wurde, wird der Fortschritt des Formatierungs-
                vorgangs von DiskPart in Prozent angezeigt.

    NOERR       Nur für Skripting. Wird ein Fehler festgestellt, werden
                Befehle weiterhin so verarbeitet, als wäre der Fehler
                nicht aufgetreten. Ohne den Parameter "NOERR"
                wird DiskPart aufgrund eines Fehlers mit einem
                Fehlercode beendet.

    Damit dieser Vorgang erfolgreich ist, muss ein Volume ausgewählt werden.

Beispiele:

    FORMAT FS=NTFS LABEL="Neues Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Polish
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Portugese
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Romanian
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Russian
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Albanian
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Turkish
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Chinese
    Formats the specified volume for use with ReactOS.

Syntax:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     Specifies the type of file system. If no file system is given,
                the default file system displayed by the FILESYSTEMS command is
                used.

    REVISION=<X.XX>

                Specifies the file system revision (if applicable).

    RECOMMENDED If specified, use the recommended file system and revision
                instead of the default if a recommendation exists. The
                recommended file system (if one exists) is displayed by the
                FILESYSTEMS command.

    LABEL=<"label">

                Specifies the volume label.

    UNIT=<N>    Overrides the default allocation unit size. Default settings
                are strongly recommended for general use. The default
                allocation unit size for a particular file system is displayed
                by the FILESYSTEMS command.

                NTFS compression is not supported for allocation unit sizes
                above 4096.

    QUICK       Performs a quick format.

    COMPRESS    NTFS only: Files created on the new volume will be compressed
                by default.

    OVERRIDE    Forces the file system to dismount first if necessary. All
                opened handles to the volume would no longer be valid.

    DUPLICATE   UDF Only: This flag applies to UDF format, version 2.5 or
                higher.
                This flag instructs the format operation to duplicate the file
                system meta-data to a second set of sectors on the disk. The
                duplicate meta-data is used by applications, for example repair
                or recovery applications. If the primary meta-data sectors are
                found to be corrupted, the file system meta-data will be read
                from the duplicate sectors.

    NOWAIT      Forces the command to return immediately while the format
                process is still in progress. If NOWAIT is not specified,
                DiskPart will display format progress in percentage.

    NOERR       For scripting only. When an error is encountered, DiskPart
                continues to process commands as if the error did not occur.
                Without the NOERR parameter, an error causes DiskPart to exit
                with an error code.

    A volume must be selected for this operation to succeed.

Examples:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
.
Language=Taiwanese
    格式化指定的磁碟區以供 ReactOS 使用。

語法:  FORMAT [[FS=<FS>] [REVISION=<X.XX>] | RECOMMENDED] [LABEL=<"label">]
                [UNIT=<N>] [QUICK] [COMPRESS] [OVERRIDE] [DUPLICATE] [NOWAIT]
                [NOERR]

    FS=<FS>     指定檔案系統的類型。如果沒有提供檔案系統，則會使用
                FILESYSTEMS 命令顯示的預設檔案系統。

    REVISION=<X.XX>

                指定檔案系統修訂編號 (如適用)。

    RECOMMENDED 如果指定這個參數，請使用建議的檔案系統及修訂編號以取代
                預設的修訂編號 (如果有建議的話)。
                FILESYSTEMS 命令可顯示建議的檔案系統 (如果有建議設定)。

    LABEL=<"label">

                指定磁碟區標籤。

    UNIT=<N>    覆寫預設的配置單位大小。在一般使用情況下，強烈建議使用預設
                設定。您可以使用 FILESYSTEMS 命令來顯示特定檔案系統的
                預設配置單位大小。

                NTFS 壓縮不支援超過 4096 的配置單位大小。

    QUICK       進行快速格式化。

    COMPRESS    只限 NTFS: 預設情況下，在新磁碟區上建立的檔案將會被壓縮。

    OVERRIDE    如果有必要，會強制先解下磁碟區。所有已開啟的磁碟區
                控制代碼將會失效。

    DUPLICATE   只限 UDF: 這個旗標適用於 UDF 格式 2.5 或更高版本。
                這個旗標會指示格式化操作將檔案系統的中繼資料複製到磁碟上的
                第二組扇區。複製的中繼資料可被應用程式使用，例如修復或恢復
                應用程式。如果系統發現主要中繼資料扇區損壞，則會從複製扇區
                中讀取檔案系統中繼資料。

    NOWAIT      當格式化程序仍在進行中的時候，強制該命令立刻返回。
                如果沒有指定 NOWAIT，DiskPart 將會以百分比顯示格式化進度。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    您必須先選擇一個磁碟區，才能完成這個操作。

範例:

    FORMAT FS=NTFS LABEL="New Volume" QUICK COMPRESS
    FORMAT RECOMMENDED OVERRIDE
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
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=German
    Zeigt eine Liste der verfügbaren Befehle oder detaillierte
    Hilfeinformationen zu einem bestimmten Befehl an.

Syntax:  HELP [<BEFEHL>]

    <BEFEHL>   Der Befehl, zu dem eine detaillierte Hilfe angezeigt
               werden soll.

    Wenn kein Befehl angegeben ist, werden mit HELP alle verfügbaren Befehle
    angezeigt.

Beispiel:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Polish
    Wyświetla listę dostępnych poleceń lub szczegółową pomoc dotyczącą
    danego polecenia.

Składnia:  HELP [<POLECENIE>]

    <POLECENIE>   Polecenie, dla którego ma zostać wyświetlona szczegółowa
                  pomoc.

    Jeśli polecenie nie jest określone, polecenie HELP wyświetla
    wszystkie możliwe polecenia.

Przykład:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Portugese
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Romanian
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Russian
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Albanian
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Turkish
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Chinese
    Displays a list of the available commands or detailed help information for a
    specified command.

Syntax:  HELP [<COMMAND>]

    <COMMAND>   The command for which to display detail help.

    If no command is specified, HELP will display all possible commands.

Example:

    HELP
    HELP CREATE PARTITION PRIMARY
.
Language=Taiwanese
    顯示可用的命令清單，或指定之命令的詳細說明。

語法:  HELP [<COMMAND>]

    <COMMAND>   要顯示其詳細說明的命令。

    如果沒有指定命令，HELP 將會顯示所有可能的命令。

範例:

    HELP
    HELP CREATE PARTITION PRIMARY
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
        user with a thorough understanding of ReactOS storage management.

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
    Na dyskach zawierających formatowanie z głównym rekordem rozruchowym (MBR)
    oznacza partycję, na której jest ustawiony fokus, jako nieaktywną.

Składnia:  INACTIVE

    Podczas ponownego uruchamiania komputer może zostać uruchomiony według
    następnej opcji określonej w systemie BIOS, takiej jak stacja dysków
    CD-ROM lub środowisko rozruchowe oparte na środowisku PXE (Pre-Boot
    eXecution Environment), na przykład usługi instalacji zdalnej (RIS).

    Aby operacja się powiodła, musi być wybrana partycja.

    Uwaga:

        Komputer może nie zostać uruchomiony bez aktywnej partycji. Jeśli
        użytkownik nie dysponuje doświadczeniem w zakresie zarządzania
        magazynami systemu Windows, nie powinien oznaczać partycji systemowej
        ani rozruchowej jako nieaktywnej.

Przykład:

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
        user with a thorough understanding of ReactOS storage management.

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
        user with a thorough understanding of ReactOS storage management.

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
        user with a thorough understanding of ReactOS storage management.

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
        user with a thorough understanding of ReactOS storage management.

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
        user with a thorough understanding of ReactOS storage management.

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
        user with a thorough understanding of ReactOS storage management.

Example:

    INACTIVE
.
Language=Taiwanese
    在具有主開機記錄 (MBR) 磁碟格式的磁碟上，
    將已選擇的磁碟分割標示為非使用中。

語法:  INACTIVE

    當您重新啟動電腦時，電腦可以從您在 BIOS 中指定的下一個選項啟動，
    例如 CD-ROM 光碟機或開機前執行環境 (PXE) 的開機環境
    (例如，遠端安裝服務\r\n (RIS))。

    您必須先選擇一個磁碟分割，才能完成這個操作。

    注意:

        如果沒有使用中的磁碟分割，您的電腦將無法啟動。
        除非您是非常熟悉 ReactOS 存放管理的進階使用者，
        否則請勿將系統或開機磁碟分割標示為非使用中。

範例:

    INACTIVE
.


MessageId=10032
SymbolicName=MSG_COMMAND_LIST_DISK
Severity=Informational
Facility=System
Language=English
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=German
    Zeigt eine Liste der Datenträger an.

Syntax:  LIST DISK

    Zeigt eine Liste mit Datenträgern und entsprechenden Informa-
    tionen an - beispielsweise Größe, verfügbarer Speicherplatz sowie
    Angaben dazu, ob es sich bei dem Datenträger um einen
    Basisdatenträger oder um einen dynamischer Datenträger handelt
    und ob der MBR- oder der GUID-Partitionsstil (Master Boot Record
    oder GUID-Partitionstabelle) verwendet wird.
    Den Fokus besitzt der mit einem Sternchen (*) markierte Datenträger.

    In der Spalte "FREE" wird nicht der gesamte freie Speicherplatz
    auf dem Datenträger, sondern der verbleibende, verwendbare freie
    Speicherplatz angezeigt. Bei einem Datenträger mit 10 GB mit
    4 primären Partitionen zu 5 GB verbleibt beispielsweise kein freier
    verwendbarer Speicherplatz (es können keine weiteren Partitionen
    erstellt werden). Ein weiteres Beispiel ist ein 10 GB Datenträger mit
    3 primären Partitionen und einer erweiterten Partition mit 8 GB. Die
    erweiterte Partition besitzt eine Größe von 3 GB mit einem logischen
    Laufwerk (2 GB). Für den Datenträger wird nur 1 GB als frei
    angezeigt - 1 GB freier Speicherplatz in der erweiterten Partition.

Beispiel:

    LIST DISK
.
Language=Polish
    Wyświetla listę dysków.

Składnia:  LIST DISK

    Wyświetla listę dysków oraz dotyczące ich informacje, takie jak rozmiar,
    ilość dostępnego wolnego miejsca, informację, czy dysk jest podstawowy
    lub dynamiczny oraz czy dysk używa głównego rekordu rozruchowego (MBR)
    lub stylu partycji tabeli partycji identyfikatora GUID (GPT). Fokus jest
    ustawiony na dysku oznaczonym gwiazdką (*).

    Należy zauważyć, że w kolumnie WOLNE nie jest wyświetlana całkowita
    ilość wolnego miejsca na dysku, ale ilość pozostałego wolnego miejsca
    na dysku zdatnego do użytku. Jeśli na przykład dysk o pojemności 10 GB
    zawiera 4 partycje główne zajmujące 5 GB, na dysku nie ma wolnego
    miejsca (nie można utworzyć następnych partycji). W innym przykładzie
    dysk o pojemności 10 GB zawiera 3 partycje główne oraz partycję
    rozszerzoną zajmujące 8 GB. Partycja rozszerzona ma rozmiar 3 GB
    przy jednym dysku logicznym o rozmiarze 2 GB. Na dysku zostanie
    pokazany tylko 1 GB wolnego miejsca i jest to 1 GB wolnego miejsca
    w partycji rozszerzonej.

Przykład:

    LIST DISK
.
Language=Portugese
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Romanian
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Russian
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Albanian
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Turkish
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Chinese
    Displays a list of disks.

Syntax:  LIST DISK

    Displays a list of disks and information about them, such as their
    size, amount of available free space, whether the disk is a basic
    or dynamic disk, and whether the disk uses the master boot record
    (MBR) or GUID partition table (GPT) partition style. The disk marked
    with an asterisk (*) has focus.

    Note that the FREE column does not display the total amount of free
    space on the disk, but rather the amount of usable free space left
    on the disk. For example, if you have a 10GB disk with 4 primary
    partitions covering 5GB, there is no usable free space left (no
    more partitions may be created). Another example would be you have
    a 10GB disk with 3 primary partitions and an extended partition
    covering 8GB. The exended partition is of size 3GB with one logical
    drive of size 2GB. The disk will show only 1GB as free - the
    1GB of free space in the extended partition.

Example:

    LIST DISK
.
Language=Taiwanese
    顯示磁碟清單。

語法:  LIST DISK

    顯示磁碟清單，以及它們的相關資訊，例如大小資訊、可用磁碟空間大小
    (無論該磁碟是基本磁碟或動態磁碟)，及該磁碟是使用主開機記錄 (MBR) 或
    GUID 磁碟分割表格 (GPT) 磁碟分割樣式。有標示星號 (*) 的磁碟是已選擇
    的磁碟。

    請注意，可用 行所標示的不是磁碟上的空間總大小，而是磁碟上剩餘的可用
    空間大小。例如，您有一個 10GB 的磁碟，而磁碟包含了 4 個主要磁碟分割
    (共使用了 5GB)，磁碟將沒有任何可用空間 (不能建立更多磁碟分割)。
    再例如，您有一個 10GB 的磁碟，而磁碟包含了 3 個主要磁碟分割
    和 1 個延伸磁碟分割 (共使用了 8GB)。如該延伸磁碟分割的大小是 3GB，
    而且有 1 個 2GB 的邏輯磁碟分割。這時會顯示磁碟只有 1GB 的可用空間
    (即延伸磁碟分割內的 1GB 可用空間)。

範例:

    LIST DISK
.

MessageId=10033
SymbolicName=MSG_COMMAND_LIST_PARTITION
Severity=Informational
Facility=System
Language=English
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=German
    Zeigt eine Liste der Partitionen in der Partitionstabelle für den
    ausgewählten Datenträger an.

Syntax:  LIST PARTITION

    Auf dynamischen Datenträgern entsprechen die Partitionen nicht unbedingt
    den dynamischen Volumes auf dem Datenträger. Partitionen werden auf
    dynamischen Datenträgern möglicherweise nicht erstellt oder gelöscht.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss ein
    Datenträger ausgewählt werden.

Beispiel:

    LIST PARTITION
.
Language=Polish
    Wyświetla listę partycji znajdujących się w tabeli partycji wybranego
    dysku.

Składnia:  LIST PARTITION

    Na dyskach dynamicznych partycje nie muszą być zgodne z woluminami
    dynamicznymi na dysku. Partycji nie można tworzyć na dyskach dynamicznych
    ani ich usuwać z takich dysków.

    Aby ta operacja się powiodła, musi być wybrany dysk.

Przykład:

    LIST PARTITION
.
Language=Portugese
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Romanian
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Russian
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Albanian
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Turkish
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Chinese
    Displays a list of partitions in the partition table for the selected disk.

Syntax:  LIST PARTITION

    On dynamic disks, the partitions do not neccessarily correspond to the
    dynamic volumes on the disk. Partitions may not be created or deleted
    on dynamic disks.

    A disk must be selected for this operation to succeed.

Example:

    LIST PARTITION
.
Language=Taiwanese
    顯示已選擇的磁碟之磁碟分割表格中的磁碟分割清單。

語法:  LIST PARTITION

    在動態磁碟上，磁碟分割不一定會對應到磁碟上的動態磁碟區。
    您無法在動態磁碟上建立或刪除磁碟分割。

    您必須先選擇一個磁碟，才能完成這個操作。

範例:

    LIST PARTITION
.


MessageId=10034
SymbolicName=MSG_COMMAND_LIST_VOLUME
Severity=Informational
Facility=System
Language=English
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=German
    Zeigt eine Liste der Basisvolumes und dynamischen Volumes an, die auf dem
    lokalen Computer installiert sind.

Syntax:  LIST VOLUME

Beispiel:

    LIST VOLUME
.
Language=Polish
    Wyświetla listę woluminów podstawowych i dynamicznych zainstalowanych
    w komputerze lokalnym.

Składnia:  LIST VOLUME

Przykład:

    LIST VOLUME
.
Language=Portugese
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Romanian
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Russian
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Albanian
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Turkish
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Chinese
    Displays a list of basic and dynamic volumes which are installed on the local
    machine.

Syntax:  LIST VOLUME

Example:

    LIST VOLUME
.
Language=Taiwanese
    顯示已安裝在本機上的基本磁碟區與動態磁碟區清單。

語法:  LIST VOLUME

範例:

    LIST VOLUME
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
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=German
    Bietet eine Möglichkeit, einem Skript Kommentare hinzuzufügen.

Syntax:  REM

Beispiel:

    In diesem Beispielskript wird mit REM ein Kommentar eingeleitet, mit
    dem die Funktion des Skripts erläutert wird.

    REM Mit diesen Befehlen werden drei Laufwerke eingerichtet.
    CREATE PARITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Polish
    Umożliwia dodawanie komentarzy do skryptu.

Składnia:  REM

Przykład:

    W tym przykładowym skrypcie polecenie REM zostało użyte w celu dodania
    komentarza opisującego działanie skryptu.

    REM Te polecenia konfigurują trzy dyski.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Portugese
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Romanian
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Russian
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Albanian
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Turkish
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Chinese
    Provides a way to add comments to a script.

Syntax:  REM

Example:

    In this example script, REM is used to provide a comment about what the
    script does.

    REM These commands set up 3 drives.
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
.
Language=Taiwanese
    提供將命令加入指令碼的方法。

語法:  REM

範例:

    在這個範例指令碼中，REM 是用來提供關於該指令碼之功能的註解。

    REM 這些命令可建立 3 部磁碟機。
    CREATE PARTITION PRIMARY SIZE=2048
    ASSIGN d:
    CREATE PARTITION EXTEND
    CREATE PARTITION LOGICAL SIZE=2048
    ASSIGN e:
    CREATE PARTITION LOGICAL
    ASSIGN f:
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
    Lokalizuje nowe dyski, które można dodać do komputera.

Składnia:  RESCAN

Przykład:

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
    找出可能已加入電腦的新磁碟。

語法:  RESCAN

範例:

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
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=German
    Dient zum Auswählen des angegebenen Datenträgers sowie zum
    Festlegen des Fokus auf den Datenträger.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                Die DiskPart-Datenträgerindexnummer des Datenträgers, auf
                den der Fokus festgelegt werden soll.

    DISK=<Pfad>
                Der Speicherortpfad des Datenträgers, auf den der Fokus
                festgelegt werden soll.

    DISK=SYSTEM
                Bei BIOS-Computern erhält der BIOS-Datenträger "0" den
                Fokus. Bei EFI-Computern erhält der Datenträger mit der
                ESP-Partition, der für den aktuellen Startvorgang verwendet
                wird, den Fokus. Bei EFI-Computern ohne ESP oder mit
                mehreren ESPs (oder beim Start des Computers unter Ver-
                wendung von Windows PE) tritt bei Verwendung des Befehls
                ein Fehler auf.

    DISK=NEXT
                Nach Auswahl eines Datenträgers erfolgt mithilfe dieses
                Befehls eine Iteration durch alle Datenträger in der Daten-
                trägerliste. Der nächste Datenträger in der Liste erhält den
                Fokus. Handelt es sich beim nächsten Datenträger um den
                Beginn der Aufzählung, tritt bei dem Befehl ein Fehler auf, und
                keiner der Datenträger erhält den Fokus.

    Ohne Angabe von Optionen wird mithilfe des Befehls "Select" der
    Datenträger angegeben, der derzeit den Fokus besitzt. Verwenden
    Sie zum Anzeigen der DiskPart-Indexnummern für alle Datenträger
    des Computers den Befehl "LIST DISK".

Beispiele:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Polish
    Wybiera określony dysk i przenosi na niego fokus.

Składnia:  SELECT DISK=<N>
           SELECT DISK=SYSTEM
           SELECT DISK=NEXT
           SELECT DISK=<Ścieżka>

    DISK=<N>
                Numer indeksu dysku w programie DiskPart odpowiadający
                dyskowi, na którym ma zostać ustawiony fokus.

    DISK=<Ścieżka>
                Ścieżka lokalizacji dysku, na którym ma zostać
                ustawiony fokus.

    DISK=SYSTEM
                Na komputerach z systemem BIOS fokus zostanie ustawiony na
                dysku 0 systemu BIOS. Na komputerach z interfejsem EFI
                fokus zostanie ustawiony na dysku zawierającym partycję ESP
                użytą do wykonania bieżącego rozruchu. Wykonanie polecenia
                nie powiedzie się na komputerach z interfejsem EFI,
                na których nie ma partycji ESP lub jest kilka partycji ESP,
                oraz gdy rozruch komputera nastąpił w środowisku Windows PE.

    DISK=NEXT
                Po wybraniu dysku to polecenie jest używane do przejścia
                przez wszystkie dyski znajdujące się na liście dysków.
                Fokus zostanie ustawiony na następnym dysku na liście.
                Jeśli następny dysk jest na początku wyliczania, wykonanie
                polecenia nie powiedzie się i fokus nie zostanie ustawiony
                na żadnym dysku.

    Jeśli żadne opcje nie są określone, polecenie SELECT wyświetla dysk,
    na którym obecnie jest ustawiony fokus. Numery indeksu w programie
    DiskPart odpowiadające wszystkim dyskom w komputerze można wyświetlić
    za pomocą polecenia LIST DISK.

Przykłady:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Portugese
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Romanian
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Russian
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Albanian
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Turkish
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Chinese
    Selects the specified disk and shifts the focus to it.

Syntax:  SELECT DISK=<N>
         SELECT DISK=SYSTEM
         SELECT DISK=NEXT
         SELECT DISK=<Path>

    DISK=<N>
                The DiskPart disk index number of the disk to receive
                focus.

    DISK=<Path>
                The location path of the disk to receive focus.

    DISK=SYSTEM
                On BIOS machines, BIOS disk 0 will receive focus.
                On EFI machines, the disk containing the ESP partition
                used for the current boot will receive focus. On EFI
                machines, if there is no ESP, or there is more than
                one ESP present, or the machine is booted from Windows PE,
                the command will fail.

    DISK=NEXT
                Once a disk is selected, this command is used to iterate
                over all disks in the disk list. The next disk in the list
                will receive focus. If the next disk is the start of the
                enumeration, the command will fail and no disk will have
                focus.

    If no options are specified, the select command lists the disk that
    currently has the focus. You can view the DiskPart index numbers
    for all disks on the computer by using the LIST DISK command.

Example:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.
Language=Taiwanese
    選擇指定的磁碟，並將焦點轉移到該磁碟。

語法:   SELECT DISK=<N>
        SELECT DISK=SYSTEM
        SELECT DISK=NEXT
        SELECT DISK=<Path>

    DISK=<N>
                要接收焦點之磁碟的磁碟號碼。

    DISK=<Path>
                要接收焦點之磁碟的位置的路徑。

    DISK=SYSTEM
                在使用 BIOS 的電腦中，BIOS 磁碟 0 會接收焦點。
                在使用 EFI 的電腦中，包含用於目前啟動程序的 ESP 分區的
                磁碟會接收焦點。如果在使用 EFI 的電腦中沒有 ESP 分區、
                有多於一個 ESP 分區，或者電腦是從 Windows PE 啟動，
                命令將會失敗。

    DISK=NEXT
                在選擇磁碟後，這個命令是用以逐一查看磁碟清單中的
                所有磁碟。清單中的下一個磁碟將會接收焦點。
                如果下一個磁碟是列舉的開頭，命令將會失敗，且沒有任何
                磁碟會有焦點。

    如果未指定任何選項，Select 命令會列出目前聚焦中的磁碟。
    您可以使用 LIST DISK 命令，檢視電腦上所有磁碟的 DiskPart 索引號碼。

範例:

    SELECT DISK=1
    SELECT DISK=SYSTEM
    SELECT DISK=NEXT
    SELECT DISK=PCIROOT(0)#PCI(0100)#ATA(C00T00L01)
.


MessageId=10047
SymbolicName=MSG_COMMAND_SELECT_PARTITION
Severity=Informational
Facility=System
Language=English
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=German
    Wählt die angegebene Partition aus und setzt den Fokus auf diese
    Partition.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

            Die Nummer der Partition, die den Fokus erhalten soll.

    Wenn keine Partition angegeben ist, listet der Befehl SELECT die
    aktuelle Partition auf, die den Fokus hat. Sie können die Partition
    mit ihrer Nummer angeben. Mit dem Befehl LIST PARTITION können die
    Nummern aller auf dem aktuellen Datenträger enthaltenen Partitionen
    angezeigt werden.

    Bevor Sie eine Partition auswählen können, müssen Sie zunächst mit dem
    DiskPart-Befehl SELECT DISK einen Datenträger auswählen.

Beispiel:

    SELECT PARTITION=1
.
Language=Polish
    Wybiera określoną partycję i przenosi na nią fokus.

Składnia:  SELECT PARTITION=<N>

    PARTITION=<N>

                Numer partycji, na której ma zostać ustawiony fokus.

    Jeśli żadna partycja nie jest określona, polecenie SELECT wyświetla
    partycję, na której obecnie jest ustawiony fokus. Partycję można
    określić, podając jej numer. Wszystkie numery partycji na bieżącym dysku
    można wyświetlić za pomocą polecenia LIST PARTITION.

    Przed wybraniem partycji należy wybrać dysk przy użyciu polecenia
    SELECT DISK programu DiskPart.

Przykład:

    SELECT PARTITION=1
.
Language=Portugese
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Romanian
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Russian
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Albanian
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Turkish
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Chinese
    Selects the specified partition and shifts the focus to it.

Syntax:  SELECT PARTITION=<N>

    PARTITION=<N>

                The number of the partition to receive the focus.

    If no partition is specified, the select command lists the current
    partition with focus. You can specify the partition by its number. You can
    view the numbers of all partitions on the current disk by using the list
    partition command.

    You must first select a disk using the DiskPart select disk command before
    you can select a partition.

Example:

    SELECT PARTITION=1
.
Language=Taiwanese
    選取指定的磁碟分割，並將焦點轉移到該磁碟分割。

語法:  SELECT PARTITION=<N>

    PARTITION=<N>

                要接收焦點之磁碟分割的磁碟分割號碼。

    如果未指定任何選項，Select 命令會列出目前聚焦中的磁碟分割。
    您可以使用磁碟分割的號碼來指定它。您可以使用 list partition 命令
    來檢視目前磁碟上之所有磁碟分割的號碼。

    在選擇磁碟分割之前，您必須先使用 DiskPart 的 select disk 命令
    來選擇磁碟。

範例:

    SELECT PARTITION=1
.


MessageId=10048
SymbolicName=MSG_COMMAND_SELECT_VOLUME
Severity=Informational
Facility=System
Language=English
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=German
    Wählt das angegebene Volume aus und setzt den Fokus auf dieses Volume.

Syntax:  SELECT VOLUME={<N> | <L>}

    VOLUME=<N>  Die Nummer des Volumes, das den Fokus erhalten soll.

    VOLUME=<L>  Der Laufwerkbuchstabe oder Pfad des eingebundenen Ordners des
                Volumes, das den Fokus erhalten soll.

    Wenn kein Volume angegeben ist, listet der Befehl SELECT das aktuelle
    Volume auf, das den Fokus besitzt. Sie können das Volume mit einer Nummer,
    einem Laufwerkbuchstaben oder dem Pfad des eingebundenen
    Ordners angeben.
    Auf einem Basisdatenträger erhält bei Auswahl eines Volumes auch die
    entsprechende Partition den Fokus. Mit dem Befehl LIST VOLUME können Sie
    die Nummern aller auf dem Computer vorhandenen Volumes anzeigen.

Beispiel:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Polish
    Wybiera określony wolumin i ustawia na nim fokus.

Składnia:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  Numer woluminu, na którym ma zostać ustawiony fokus.

    VOLUME=<D>  Litera dysku lub ścieżka zainstalowanego folderu woluminu,
                na którym ma zostać ustawiony fokus.

    Jeśli żaden wolumin nie jest określony, polecenie SELECT wyświetla
    wolumin, na którym obecnie jest ustawiony fokus. Wolumin można określić,
    podając jego numer, literę dysku lub ścieżkę zainstalowanego woluminu.
    Na dysku podstawowym fokus jest ustawiany również na partycji
    odpowiadającej woluminowi. Numery wszystkich woluminów na komputerze można
    wyświetlić przy użyciu polecenia LIST VOLUME.

Przykład:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Portugese
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Romanian
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Russian
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Albanian
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Turkish
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Chinese
    Selects the specified volume and shifts the focus to it.

Syntax:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  The number of the volume to receive the focus.

    VOLUME=<D>  The drive letter or mounted folder path of the volume
                to receive the focus.

    If no volume is specified, the select command lists the current volume with
    focus. You can specify the volume by number, drive letter, or mounted folder
    path. On a basic disk, selecting a volume also gives the corresponding
    partition focus. You can view the numbers of all volumes on the computer by
    using the list volume command.

Example:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
.
Language=Taiwanese
    選取指定的磁碟區，並將焦點移到該磁碟區。

語法:  SELECT VOLUME={<N> | <D>}

    VOLUME=<N>  要接收焦點的磁碟區編號。

    VOLUME=<D>  要接收焦點的磁碟區代號或掛接點路徑。

    如果沒有指定磁碟區，select 命令將列出目前聚焦中的磁碟區。您可以根據編號、
    磁碟機代號或掛接點路徑指定磁碟區。在基本磁碟中，選擇磁碟區同時也會給予
    相對的磁碟分割焦點。您可以使用 list volume 命令以檢視所有電腦中磁碟區的
    編號。

範例:

    SELECT VOLUME=1
    SELECT VOLUME=C
    SELECT VOLUME=C:\\MountH
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

    Ändert das Partitionstypfeld für die Partition im Fokus.

Syntax:  SET ID={<BYTE> | <GUID>} [OVERRIDE] [NOERR]

    ID={<BYTE> | <GUID>}

                Gibt den neuen Partitionstyp an.

                Für MBR-Datenträger (Master Boot Record) können Sie ein
                Partitionstypbyte im Hexadezimalformat angeben. Mit
                diesem Parameter können Sie jedes Partitionstypbyte angeben,
                außer Typ "0x42" (LDM-Partition). Das voranstehende "0x" wird
                bei Angabe des hexadezimalen Partitionstyps ausgelassen.

                Für GPT-Datenträger (GPT = GUID-Partitionstabelle) können
                Sie einen Partitionstyp-GUID für die zu erstellende Partition
                angeben: Zu den erkannten GUIDs gehören:

                    EFI-Systempartition:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Basisdatenpartition:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                Mit diesem Parameter kann jeder Partitionstyp-GUID
                angegeben werden, mit Ausnahme der folgenden:

                    MSR-Partition (Microsoft Reserved):
                        e3c9e316-0b5c-4db8-817d-f92df00215ae

                    LDM-Metadatenpartition auf einem dynamischen Datenträger:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    LDM-Datenpartition auf einem dynamischen Datenträger:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Clustermetadaten-Partition:
                        db97dba9-0840-4bae-97f0-ffb9a327c7e1


                Abgesehen von den erwähnten Einschränkungen überprüft
                DiskPart die Gültigkeit des Partitionstyps nicht. Es wird
                lediglich sichergestellt, dass es sich um ein Byte im Hexadezi-
                malformat oder um einen GUID handelt

    OVERRIDE    Ermöglicht DiskPart die Aufhebung der Bereitstellung
                des Dateisystems auf dem Volume zu erzwingen, bevor der
                Partitionstyp geändert wird. Beim Ändern des Partitionstyps
                wird das Dateisystem auf dem Volume gesperrt und dessen
                Bereitstellung aufgehoben. Wird dieser Parameter nicht an-
                gegeben und schlägt der Aufruf zum Sperren des Dateisys-
                tems fehl (da einige andere Anwendungen über ein geöffne-
                tes Handle für das Volume verfügen), schlägt der gesamte
                Vorgang fehl. Bei Angabe dieses Parameters wird die Auf-
                hebung der Bereitstellung auch dann erzwungen, wenn der
                Aufruf des Dateisystems fehlschlägt. Wird die Bereitstellung
                eines Dateisystems aufgehoben, werden alle geöffneten
                Handles für das Volume ungültig.

    NOERR       Nur für Skripting. Bei einem Fehler wird die Verarbeitung von
                Befehlen fortgesetzt, als sei der Fehler nicht aufgetreten.
                Ohne NOERR-Parameter wird DiskPart bei einem Fehler mit
                dem entsprechenden Fehlercode beendet.

    Ist nur für die Verwendung durch Originalgerätehersteller
    (OEM, Original Equipment Manufacturer) vorgesehen.

    Damit dieser Vorgang erfolgreich ausgeführt werden kann, muss
    eine Partition ausgewählt sein.

    Vorsicht:

        Das Ändern von Partitionstypfeldern mit diesem Parameter kann
        dazu führen, dass der Computer nicht mehr ordnungsgemäß
        funktioniert oder nicht mehr gestartet werden kann. Sofern Sie
        kein Originalgerätehersteller oder IT-Fachmann sind, der mit GPT-
        Datenträgern vertraut ist, sollten Sie keine Partitionstypfelder auf
        GPT-Datenträgern mit diesem Parameter ändern. Verwenden Sie
        stattdessen immer den Befehl "CREATE PARTITION EFI" zum
        Erstellen von EFI-Systempartitionen, den Befehl "CREATE
        PARTITION MSR" zum Erstellen von MSR-Partitionen und den
        Befehl "CREATE PARTITION PRIMARY" (ohne diesen Parameter)
        zum Erstellen von primären Partitionen auf GPT-Datenträgern.

    Dieser Befehl funktioniert weder auf dynamischen Datenträgern noch
    auf  MSR-Partitionen (Microsoft Reserved).

Beispiel:

    SET ID=07 OVERRIDE
    SET ID=ebd0a0a2-b9e5-4433-87c0-68b6b72699c7
.
Language=Polish
    Zmienia zawartość pola typu partycji dla partycji, na której ustawiony
    jest fokus.

Składnia:  SET ID={<BYTE> | <GUID>} [OVERRIDE] [NOERR]

    ID={<BYTE> | <GUID>}

                Określa nowy typ partycji.
                W przypadku dysków z głównym rekordem rozruchowym (MBR)
                można określić bajt typu partycji w formie szesnastkowej.
                Za pomocą tego parametru można określić dowolny bajt typu
                partycji z wyjątkiem typu 0x42 (partycja LDM). W przypadku
                określania szesnastkowego typu partycji znaki '0x'
                na początku są pomijane.

                W przypadku dysków z tablicą partycji GUID (GPT) można
                określić identyfikator GUID typu partycji. Rozpoznawane
                identyfikatory GUID są następujące:

                    Partycja systemowa EFI:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    Podstawowa partycja danych:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                Za pomocą tego parametru można określić dowolny identyfikator
                GUID typu partycji z wyjątkiem następujących:

                    Partycja zastrzeżona firmy Microsoft:
                        e3c9e316-0b5c-4db8-817d-f92df00215a

                    Partycja metadanych LDM na dysku dynamicznym:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    Partycja danych LDM na dysku dynamicznym:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    Partycja metadanych klastra:
                        db97dba9-0840-4bae-97f0-ffb9a327c7e1


                Oprócz wspomnianych ograniczeń program DiskPart nie sprawdza
                poprawności typu partycji. Sprawdza jedynie, czy podana
                wartość ma postać szesnastkową lub jest identyfikatorem GUID.

    OVERRIDE    Powoduje, że przed zmianą typu partycji program DiskPart
                wymusza odinstalowanie systemu plików w woluminie. Podczas
                zmiany typu partycji program DiskPart podejmie próbę
                zablokowania i odinstalowania systemu plików w woluminie.
                Jeśli nie określono tego parametru, a wywołanie w celu
                zablokowania systemu plików nie powiedzie się (ponieważ inna
                aplikacja korzysta z otwartego dojścia do woluminu), cała
                operacja zakończy się niepowodzeniem. Jeśli określono ten
                parametr, odinstalowanie jest wymuszane nawet wtedy,
                gdy wywołanie w celu zablokowania systemu plików nie
                powiedzie się. Po odinstalowaniu systemu plików wszystkie
                otwarte dojścia do woluminu staną się nieprawidłowe.

    NOERR       Tylko do obsługi skryptów. Po wystąpieniu błędu program
                DiskPart kontynuuje przetwarzanie poleceń, tak jakby błąd.
                nie wystąpił. W przypadku braku parametru NOERR błąd
                powoduje zakończenie działania programu DiskPart
                i zwrócenie kodu błędu.

    Polecenie przeznaczone tylko do użytku przez producentów oryginalnego
    sprzętu (OEM).

    Aby ta operacja się powiodła, musi być wybrana partycja.

    Uwaga:

        Zmiana pól typu partycji z tym parametrem może sprawić, że komputer
        ulegnie awarii lub nie będzie można go uruchomić. Jeśli użytkownik
        nie jest producentem OEM ani informatykiem dysponującym
        doświadczeniem w zakresie dysków GPT, nie powinien zmieniać pól typu
        partycji na dyskach GPT przy użyciu tego parametru. Zamiast tego
        należy zawsze używać polecenia CREATE PARTITION EFI do tworzenia
        partycji systemowych EFI, polecenia CREATE PARTITION MSR
        do tworzenia partycji zastrzeżonej firmy Microsoft oraz polecenia
        CREATE PARTITION PRIMARY bez parametru ID do tworzenia partycji
        podstawowych na dyskach GPT.

    To polecenie nie działa na dyskach dynamicznych ani na partycjach
    zastrzeżonych firmy Microsoft.

Przykłady:
    SET ID=07 OVERRIDE
    SET ID=ebd0a0a2-b9e5-4433-87c0-68b6b72699c7
.
Language=Portugese
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
Language=Romanian
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
Language=Russian
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
Language=Albanian
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
Language=Turkish
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
Language=Chinese
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
Language=Taiwanese
    更改已選擇的磁碟分割的磁碟分割類型欄位。

語法:  SET ID={<BYTE> | <GUID>} [OVERRIDE] [NOERR]

    ID={<BYTE> | <GUID>}

                指定新磁碟分割類型。
                對於主開機記錄 (MBR) 磁碟，您可以使用十六進位格式指定該磁碟分割
                的磁碟分割類型位元組。您可以使用此參數指定任何磁碟分割類型位元組，
                但類型 42 (LDM 磁碟分割) 除外。
                請注意，在指定十六進位格式的磁碟分割類型會略過開頭的 '0x'。

                對於 GUID 磁碟分割表格 (GPT) 磁碟，您可以為想要建立的磁碟分割
                指定磁碟分割類型 GUID。可識別的 GUID 包括:

                    EFI 系統磁碟分割:
                        c12a7328-f81f-11d2-ba4b-00a0c93ec93b

                    基本資料磁碟分割:
                        ebd0a0a2-b9e5-4433-87c0-68b6b72699c7

                您可以使用這個參數指定任何磁碟分割類型 GUID，
                但下列磁碟分割類型除外:

                    Microsoft 保留磁碟分割:
                        e3c9e316-0b5c-4db8-817d-f92df00215a

                    動態磁碟上的 LDM 中繼資料磁碟分割:
                        5808c8aa-7e8f-42e0-85d2-e1e90434cfb3

                    動態磁碟上的 LDM 資料磁碟分割:
                        af9b60a0-1431-4f62-bc68-3311714a69ad

                    叢集中繼資料磁碟分割:
                        db97dba9-0840-4bae-97f0-ffb9a327c7e1


                除了上述限制之外，DiskPart 不會檢查磁碟分割類型的有效性，
                但會確保磁碟分割類型是否為以十六進位格式或 GUID 表示的位元組。

    OVERRIDE    讓 DiskPart 在更改磁碟分割類型之前先強制卸載磁碟區上的檔案系統。
                更改磁碟分割類型時，DiskPart 會嘗試鎖定並卸載磁碟區上的檔案系統。
                若未指定這個參數，而且鎖定檔案系統的呼叫失敗 (因為某些其他
                應用程式擁有磁碟區的已開啟控制代碼)，整個操作將會失敗。
                指定這個參數時，即使鎖定檔案系統的呼叫失敗，仍會強制執行卸載。
                卸載檔案系統之後，磁碟區的所有已開啟控制代碼會變為無效。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    預期只供原始設備製造商 (OEM) 使用。

    您必須先選擇磁碟分割，才能完成這個操作。

    注意:

        使用這個參數更改磁碟分割類型欄位，可能會造成您的電腦失敗或無法啟動。
        除非您是熟悉 GPT 磁碟的 OEM 或 IT 專業人員，否則請勿在 GPT 磁碟上
        更改磁碟分割類型欄位時使用這個參數。相反地，您應該使用
        CREATE PARTITION EFI 命令來建立 EFI 系統磁碟分割、使用
        CREATE PARTITION MSR 命令來建立 Microsoft 保留磁碟分割，及使用
        CREATE PARTITION PRIMARY 命令 (不使用 ID 參數) 在 GPT 磁碟上
        建立主要磁碟分割。

    這個命令不適用於動態磁碟或 Microsoft 保留磁碟分割。

範例:
    SET ID=07 OVERRIDE
    SET ID=ebd0a0a2-b9e5-4433-87c0-68b6b72699c7
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
    Wyświetla lub ustawia identyfikator tabeli partycji GUID (GPT) lub podpis
    głównego rekordu rozruchowego (MBR) dla dysku z fokusem.

Składnia:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                Dla dysków MBR można określić czterobajtową wartość (DWORD)
                podpisu w postaci szesnastkowej.

                Dla dysków GPT należy określić ident. GUID jako identyfikator.

    NOERR       Tylko dla skryptów. W przypadku wystąpienia błędu program
                DiskPart kontynuuje przetwarzanie poleceń, tak jakby błąd
                nie wystąpił. W przypadku braku parametru NOERR błąd
                powoduje zakończenie działania programu DiskPart i zwrócenie
                kodu błędu.

    Aby ta operacja powiodła się, musi być wybrany dysk. To polecenie działa
    dla dysków podstawowych i dynamicznych.

Przykład:

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
    顯示或設定焦點所在磁碟的 GUID 磁碟分割表格 (GPT) 識別碼
    或主開機記錄 (MBR) 簽章。

語法:  UNIQUEID DISK [ID={<DWORD> | <GUID>}]  [NOERR]

    ID={<DWORD> | <GUID>}

                在主開機記錄 (MBR) 磁碟上，您可以對簽章指定十六進位形式的
                四位元組 (DWORD) 值。

                在 GUID 磁碟分割表格 (GPT) 磁碟上，指定識別元的 GUID。

    NOERR       只限指令碼。當發生錯誤時，DiskPart 會繼續處理命令，
                如同沒有發生任何錯誤一樣。如果沒有使用 NOERR 參數，
                錯誤會導致 DiskPart 結束，並傳回錯誤碼。

    您必須先選擇一個磁碟，才能完成這個操作。這個命令適用於基本磁碟及
    動態磁碟。

範例:

    UNIQUEID DISK
    UNIQUEID DISK ID=5f1b2c36
    UNIQUEID DISK ID=baf784e7-6bbd-4cfb-aaac-e86c96e166ee
.
