
;
; IMPORTANT: When a new language is added, all messages in this file need to be
; either translated or at least duplicated for the new language.
; This is a new requirement by MS mc.exe
; To do this, start with a regex replace:
; - In VS IDE: "Language=English\r\n(?<String>(?:[^\.].*\r\n)*\.\r\n)" -> "Language=English\r\n${String}Language=MyLanguage\r\n${String}"
;

MessageIdTypedef=ULONG

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               ITF=0x4:FACILITY_ITF
               WIN32=0x7:FACILITY_GENERAL
              )

LanguageNames=(English=0x409:MSG00409
               Russian=0x419:MSG00419
               Polish=0x415:MSG00415
               Romanian=0x418:MSG00418)

MessageId=0
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS
Language=English
The operation completed successfully.
.
Language=Russian
Операция успешно завершена.
.
Language=Polish
Operacja ukończona pomyślnie.
.
Language=Romanian
Operația a fost finalizată cu succes.
.

MessageId=1
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FUNCTION
Language=English
Incorrect function.
.
Language=Russian
Неверная функция.
.
Language=Polish
Niepoprawna funkcja.
.
Language=Romanian
Funcție eronată.
.

MessageId=2
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_NOT_FOUND
Language=English
The system cannot find the file specified.
.
Language=Russian
Не удается найти указанный файл.
.
Language=Polish
Nie można odnaleźć określonego pliku.
.
Language=Romanian
Fișierul specificat nu poate fi găsit.
.

MessageId=3
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_NOT_FOUND
Language=English
The system cannot find the path specified.
.
Language=Russian
Системе не удается найти указанный путь.
.
Language=Polish
System nie może odnaleźć określonej ścieżki.
.
Language=Romanian
Calea specificată nu a fost găsită.
.

MessageId=4
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_OPEN_FILES
Language=English
The system cannot open the file.
.
Language=Russian
Системе не удается открыть файл.
.
Language=Polish
System nie może otworzyć tego pliku.
.
Language=Romanian
Fișierul nu poate fi deschis.
.

MessageId=5
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DENIED
Language=English
Access is denied.
.
Language=Russian
Отказано в доступе.
.
Language=Polish
Odmowa dostępu.
.
Language=Romanian
Acces respins.
.

MessageId=6
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HANDLE
Language=English
The handle is invalid.
.
Language=Russian
Неверный дескриптор.
.
Language=Polish
Nieprawidłowe dojście.
.
Language=Romanian
Identificator de gestiune eronat.
.

MessageId=7
Severity=Success
Facility=System
SymbolicName=ERROR_ARENA_TRASHED
Language=English
The storage control blocks were destroyed.
.
Language=Russian
Повреждены управляющие блоки памяти.
.
Language=Polish
Bloki kontroli magazynu zostały zniszczone.
.
Language=Romanian
Blocurile de control al stocării au fost deteriorate.
.

MessageId=8
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_MEMORY
Language=English
Not enough storage is available to process this command.
.
Language=Russian
Недостаточно памяти для обработки команды.
.
Language=Polish
W magazynie brak miejsca dla wykonania tego polecenia.
.
Language=Romanian
Spațiul de stocare disponibil este insuficient pentru procesarea acestei comanzi.
.

MessageId=9
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK
Language=English
The storage control block address is invalid.
.
Language=Russian
Неверный адрес управляющего блока памяти.
.
Language=Polish
Adres bloku kontroli magazynu jest nieprawidłowy.
.
Language=Romanian
Adresa blocului de control al stocării nu este validă.
.

MessageId=10
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ENVIRONMENT
Language=English
The environment is incorrect.
.
Language=Russian
Недопустимая среда.
.
Language=Polish
Środowisko jest niepoprawne.
.
Language=Romanian
Mediul current nu corespunde.
.

MessageId=11
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_FORMAT
Language=English
An attempt was made to load a program with an incorrect format.
.
Language=Russian
Попытка запустить программу с недопустимым форматом.
.
Language=Polish
Próbowano załadować program w niepoprawnym formacie.
.
Language=Romanian
Program cu format necorespunzător.
.

MessageId=12
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCESS
Language=English
The access code is invalid.
.
Language=Russian
Неверный код доступа.
.
Language=Polish
Nieprawidłowy kod dostępu.
.
Language=Romanian
Codul de acces nu este valid.
.

MessageId=13
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATA
Language=English
The data is invalid.
.
Language=Russian
Недопустимые данные.
.
Language=Polish
Nieprawidłowe dane.
.
Language=Romanian
Date nevalide.
.

MessageId=14
Severity=Success
Facility=System
SymbolicName=ERROR_OUTOFMEMORY
Language=English
Not enough storage is available to complete this operation.
.
Language=Russian
Недостаточно памяти для завершения операции.
.
Language=Polish
W magazynie brak miejsca dla wykonania tej operacji.
.
Language=Romanian
Spațiul de stocare disponibil este insuficient pentru completarea acestei operații.
.

MessageId=15
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DRIVE
Language=English
The system cannot find the drive specified.
.
Language=Russian
Система не может найти указанное устройство.
.
Language=Polish
Nie można odnaleźć dysku.
.
Language=Romanian
Unitatea de stocare specificată a fost găsită.
.

MessageId=16
Severity=Success
Facility=System
SymbolicName=ERROR_CURRENT_DIRECTORY
Language=English
The directory cannot be removed.
.
Language=Russian
Этот каталог не может быть удален.
.
Language=Polish
Nie można usunąć katalogu.
.
Language=Romanian
Directorul nu poate fi eliminat.
.

MessageId=17
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAME_DEVICE
Language=English
The system cannot move the file to a different disk drive.
.
Language=Russian
Система не может переместить файл на другое дисковое устройство.
.
Language=Polish
Nie można przenieść pliku na inny dysk.
.
Language=Romanian
Fișierul nu poate fi mutat pe o altă unitate de stocare.
.

MessageId=18
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_FILES
Language=English
There are no more files.
.
Language=Russian
Файлов больше нет.
.
Language=Polish
Brak dalszych plików.
.
Language=Romanian
Fișierul nu poate fi mutat pe o altă unitate de stocare.
.

MessageId=19
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_PROTECT
Language=English
The media is write protected.
.
Language=Russian
Носитель защищен от записи.
.
Language=Polish
Nośnik jest zabezpieczony przed zapisem.
.
Language=Romanian
Mediul de stocare este protejat la scriere.
.

MessageId=20
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_UNIT
Language=English
The system cannot find the device specified.
.
Language=Russian
Система не может найти указанное устройство.
.
Language=Polish
Nie można odnaleźć urządzenia.
.
Language=Romanian
Dispozitivul specificat a fost găsit.
.

MessageId=21
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_READY
Language=English
The device is not ready.
.
Language=Russian
Устройство не готово.
.
Language=Polish
Urządzenie nie jest gotowe.
.
Language=Romanian
Dispozitivul nu este disponibil.
.

MessageId=22
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_COMMAND
Language=English
The device does not recognize the command.
.
Language=Russian
Устройство не распознает команду.
.
Language=Polish
Urządzenie nie rozpoznaje polecenia.
.
Language=Romanian
Dispozitivul nu recunoaște comanda.
.

MessageId=23
Severity=Success
Facility=System
SymbolicName=ERROR_CRC
Language=English
Data error (cyclic redundancy check).
.
Language=Russian
Ошибка в данных (циклический код с избыточностью).
.
Language=Polish
Błąd danych (CRC).
.
Language=Romanian
Eroare de date (sumă de control necorespunzătoare).
.

MessageId=24
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LENGTH
Language=English
The program issued a command but the command length is incorrect.
.
Language=Russian
Программа выдала команду с недопустимой длиной.
.
Language=Polish
Program wydał polecenie, ale długość polecenia jest niepoprawna.
.
Language=Romanian
Comanda emisă de program are o lungime necorespunzătoare.
.

MessageId=25
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK
Language=English
The drive cannot locate a specific area or track on the disk.
.
Language=Russian
Дисковое устройство не может обнаружить указанную область или дорожку.
.
Language=Polish
Nie można odnaleźć na dysku określonego obszaru lub ścieżki.
.
Language=Romanian
Unitatea de stocare nu poate localiza o anumită zonă sau pistă de pe disc.
.

MessageId=26
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_DOS_DISK
Language=English
The specified disk or diskette cannot be accessed.
.
Language=Russian
Нет доступа к диску или дискете.
.
Language=Polish
Nie można uzyskać dostępu do określonego dysku lub dyskietki.
.
Language=Romanian
Discul dur sau flexibil specificat nu poate fi accesat.
.

MessageId=27
Severity=Success
Facility=System
SymbolicName=ERROR_SECTOR_NOT_FOUND
Language=English
The drive cannot find the sector requested.
.
Language=Russian
Дисковое устройство не может обнаружить указанную область или дорожку.
.
Language=Polish
Nie można odnaleźć na dysku żądanego sektora.
.
Language=Romanian
Unitatea de stocare nu poate găsi sectorul solicitat.
.

MessageId=28
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_PAPER
Language=English
The printer is out of paper.
.
Language=Russian
В принтере закончилась бумага.
.
Language=Polish
Brak papieru w drukarce.
.
Language=Romanian
Imprimanta nu mai are hârtie.
.

MessageId=29
Severity=Success
Facility=System
SymbolicName=ERROR_WRITE_FAULT
Language=English
The system cannot write to the specified device.
.
Language=Russian
Система не может выполнить запись на указанное устройство.
.
Language=Polish
System nie może zapisywać do określonego urządzenia.
.
Language=Romanian
Imprimanta nu mai are hârtie.
.

MessageId=30
Severity=Success
Facility=System
SymbolicName=ERROR_READ_FAULT
Language=English
The system cannot read from the specified device.
.
Language=Russian
Системе не может выполнить чтение с указанного устройства.
.
Language=Polish
System nie może czytać z określonego urządzenia.
.
Language=Romanian
Eșec la citirea de pe dispozitivul specificat.
.

MessageId=31
Severity=Success
Facility=System
SymbolicName=ERROR_GEN_FAILURE
Language=English
A device attached to the system is not functioning.
.
Language=Russian
Устройство, подключенное к системе, не работает.
.
Language=Polish
Urządzenie dołączone do komputera nie działa.
.
Language=Romanian
Dispozitivul atașat nu funcționează.
.

MessageId=32
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_VIOLATION
Language=English
The process cannot access the file because it is being used by another process.
.
Language=Russian
Процесс не имеет доступа к файлу, поскольку файл используется другим процессом.
.
Language=Polish
Proces nie może uzyskać dostępu do pliku, ponieważ jest on używany przez inny proces.
.
Language=Romanian
Fișierul nu poate fi accesat în acest proces deoarece este utilizat într-un alt proces.
.

MessageId=33
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_VIOLATION
Language=English
The process cannot access the file because another process has locked a portion of the file.
.
Language=Russian
Процесс не имеет доступа к файлу, поскольку другой процесс заблокировал часть файла.
.
Language=Polish
Proces nie może uzyskać dostępu do pliku, ponieważ inny proces zablokował jego część.
.
Language=Romanian
Fișierul nu poate fi accesat în acest proces deoarece o porțiune a sa a fost blocată într-un alt proces.
.

MessageId=34
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_DISK
Language=English
The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1.
.
Language=Russian
Вставте другую дискету. Вставьте %2 (серийный номер тома: %3) в дисковод %1.
.
Language=Polish
W stacji umieszczono niewłaściwą dyskietkę. Włóż dysk %2 (numer seryjny woluminu: %3) do stacji: %1.
.
Language=Romanian
Discul flexibil din unitate nu este necorespunzător. Introduceți %2 (Număr Serial de Volum: %3) în unitatea %1.
.

MessageId=36
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_BUFFER_EXCEEDED
Language=English
Too many files opened for sharing.
.
Language=Russian
Слишком много файлов открыто для совместного использования.
.
Language=Polish
Za dużo plików otwartych do udostępniania.
.
Language=Romanian
Numărul de fișiere deschise pentru partajare este prea mare.
.

MessageId=38
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_EOF
Language=English
Reached the end of the file.
.
Language=Russian
Достигнут конец файла.
.
Language=Polish
Osiągnięto koniec pliku.
.
Language=Romanian
Sfârșitul fișierului a fost atins.
.

MessageId=39
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLE_DISK_FULL
Language=English
The disk is full.
.
Language=Russian
Отсутствует место на диске.
.
Language=Polish
Dysk jest zapełniony.
.
Language=Romanian
Discul este plin.
.

MessageId=50
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED
Language=English
The request is not supported.
.
Language=Russian
Запрос не поддерживается.
.
Language=Polish
Żądanie nie jest obsługiwane.
.
Language=Romanian
Solicitarea nu este acceptată.
.

MessageId=51
Severity=Success
Facility=System
SymbolicName=ERROR_REM_NOT_LIST
Language=English
ReactOS cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If ReactOS still cannot find the network path, contact your network administrator.
.
Language=Russian
Невозможно найти сетевой путь. Убедитесь, что сетевой путь указан верно, а конечный компьютер включен и не занят. Если система вновь не сможет найти путь, обратитесь к сетевому администратору.
.
Language=Polish
System ReactOS nie może odnaleźć ścieżki sieciowej. Sprawdź, czy ścieżka sieciowa jest poprawna i czy komputer docelowy nie jest zajęty lub wyłączony. Jeśli system ReactOS nadal nie będzie mógł odnaleźć ścieżki sieciowej, skontaktuj się z administratorem sieci.
.
Language=Romanian
Calea în rețea nu a fost găsită. Asigurați-vă că este corectă și că destinația în rețea este disponibilă fizic. Dacă veți întâmpina această problemă în continuare, contactați administratorul de rețea.
.

MessageId=52
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_NAME
Language=English
You were not connected because a duplicate name exists on the network. Go to System in the Control Panel to change the computer name and try again.
.
Language=Russian
Вы не подключены, поскольку такое же имя уже существует в этой сети. Для присоединения к домену откройте компонент панели управления "Система", измените имя компьютера и повторите попытку. Для присоединения к рабочей группе выберите другое имя рабочей группы.
.
Language=Polish
Połączenie nie zostało nawiązane, ponieważ w sieci istnieje duplikat nazwy. Jeśli przyłączasz się do domeny, przejdź do apletu System w Panelu sterowania, aby zmienić nazwę komputera, i spróbuj ponownie. Jeśli przyłączasz się do grupy roboczej, wybierz inną nazwę grupy.
.
Language=Romanian
Nu poate fi realizată o conexiune deoarece un nume duplicat există deja în rețea. Puteți schimba numele calculatorului din Proprietăți pentru Sistem (în Panoul de Control) înainte de a reîncerca.
.

MessageId=53
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NETPATH
Language=English
The network path was not found.
.
Language=Russian
Не найден сетевой путь.
.
Language=Polish
Nie można odnaleźć ścieżki sieciowej.
.
Language=Romanian
Calea în rețea a fost găsită.
.

MessageId=54
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_BUSY
Language=English
The network is busy.
.
Language=Russian
Сеть занята.
.
Language=Polish
Sieć jest zajęta.
.
Language=Romanian
Rețeaua este ocupată.
.

MessageId=55
Severity=Success
Facility=System
SymbolicName=ERROR_DEV_NOT_EXIST
Language=English
The specified network resource or device is no longer available.
.
Language=Russian
Сетевой ресурс или устройство более недоступно.
.
Language=Polish
Określone zasoby sieciowe lub urządzenie są już niedostępne.
.
Language=Romanian
Dispozitivul specificat sau resursa în rețea nu mai este disponibilă.
.

MessageId=56
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CMDS
Language=English
The network BIOS command limit has been reached.
.
Language=Russian
Достигнут предел числа команд NetBIOS.
.
Language=Polish
Osiągnięto limit poleceń systemu BIOS dla sieci.
.
Language=Romanian
Limita pentru comanda BIOS de rețea a fost atinsă.
.

MessageId=57
Severity=Success
Facility=System
SymbolicName=ERROR_ADAP_HDW_ERR
Language=English
A network adapter hardware error occurred.
.
Language=Russian
Аппаратная ошибка сетевой платы.
.
Language=Polish
Wystąpił błąd sprzętowy karty sieciowej.
.
Language=Romanian
Dispozitivul de rețea a raportat o eroare.
.

MessageId=58
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_RESP
Language=English
The specified server cannot perform the requested operation.
.
Language=Russian
Указанный сервер не может выполнить требуемую операцию.
.
Language=Polish
Określony serwer nie może wykonać żądanej operacji.
.
Language=Romanian
Serverul specificat nu poate îndeplini operația solicitată.
.

MessageId=59
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXP_NET_ERR
Language=English
An unexpected network error occurred.
.
Language=Russian
Непредвиденная сетевая ошибка.
.
Language=Polish
Wystąpił nieoczekiwany błąd sieciowy.
.
Language=Romanian
Eroare neașteptată de rețea.
.

MessageId=60
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_REM_ADAP
Language=English
The remote adapter is not compatible.
.
Language=Russian
Несовместимый удаленный контроллер.
.
Language=Polish
Zdalna karta sieciowa jest niezgodna.
.
Language=Romanian
Adaptor de rețea necompatibil.
.

MessageId=61
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTQ_FULL
Language=English
The printer queue is full.
.
Language=Russian
Очередь печати переполнена.
.
Language=Polish
Kolejka wydruku jest zapełniona.
.
Language=Romanian
Lista comenzilor de imprimare este plină.
.

MessageId=62
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SPOOL_SPACE
Language=English
Space to store the file waiting to be printed is not available on the server.
.
Language=Russian
На сервере нет места для хранения ожидающего печати файла.
.
Language=Polish
Na serwerze nie ma miejsca na przechowywanie pliku oczekującego na wydruk.
.
Language=Romanian
Serviciul de imprimare nu mai dispune de spațiu pentru fișierele în așteptare.
.

MessageId=63
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_CANCELLED
Language=English
Your file waiting to be printed was deleted.
.
Language=Russian
Ваш файл, находившийся в очереди вывода на печать, был удален.
.
Language=Polish
Plik oczekujący na wydruk został usunięty.
.
Language=Romanian
Fișierul dvs. în așteptarea imprimării a fost eliminat.
.

MessageId=64
Severity=Success
Facility=System
SymbolicName=ERROR_NETNAME_DELETED
Language=English
The specified network name is no longer available.
.
Language=Russian
Указанное сетевое имя более недоступно.
.
Language=Polish
Określona nazwa sieciowa już jest niedostępna.
.
Language=Romanian
Numele de rețea specificat nu mai este disponibil.
.

MessageId=65
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_ACCESS_DENIED
Language=English
Network access is denied.
.
Language=Russian
Нет доступа к сети.
.
Language=Polish
Odmowa dostępu do sieci.
.
Language=Romanian
Accesul la rețea este respins.
.

MessageId=66
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEV_TYPE
Language=English
The network resource type is not correct.
.
Language=Russian
Неверно указан тип сетевого ресурса.
.
Language=Polish
Typ zasobu sieciowego jest niepoprawny.
.
Language=Romanian
Tipul resursei de rețea nu este corespunzător.
.

MessageId=67
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_NET_NAME
Language=English
The network name cannot be found.
.
Language=Russian
Не найдено сетевое имя.
.
Language=Polish
Nie można odnaleźć nazwy sieciowej.
.
Language=Romanian
Numele de rețea nu este găsit.
.

MessageId=68
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_NAMES
Language=English
The name limit for the local computer network adapter card was exceeded.
.
Language=Russian
Превышен предел числа имен для сетевого адаптера локального компьютера.
.
Language=Polish
Przekroczono ograniczenie nazwy dla karty sieci lokalnej komputera.
.
Language=Romanian
Limita pentru numele plăcii adaptorului de rețea al calculatorului local a fost depășită.
.

MessageId=69
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SESS
Language=English
The network BIOS session limit was exceeded.
.
Language=Russian
Превышен предел по числу сеансов NetBIOS.
.
Language=Polish
Przekroczono limit sesji systemu BIOS dla sieci.
.
Language=Romanian
Limita pentru sesiunea BIOS de rețea a fost depășită.
.

MessageId=70
Severity=Success
Facility=System
SymbolicName=ERROR_SHARING_PAUSED
Language=English
The remote server has been paused or is in the process of being started.
.
Language=Russian
Сервер сети был остановлен или находится в процессе запуска.
.
Language=Polish
Zdalny serwer przerwał pracę lub jest w trakcie procesu uruchamiania.
.
Language=Romanian
Serverul accesat la distanță fie este în pauză fie este în curs de repornire.
.

MessageId=71
Severity=Success
Facility=System
SymbolicName=ERROR_REQ_NOT_ACCEP
Language=English
No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.
.
Language=Russian
Дополнительные подключения к этому удаленному компьютеру сейчас невозможны, так как их число достигло предела.
.
Language=Polish
Do tego komputera zdalnego nie można obecnie uzyskać więcej połączeń, ponieważ istnieje już maksymalna akceptowalna liczba połączeń.
.
Language=Romanian
Calculatorul accesat la distanță nu acceptă mai multe conexiuni deoarece limita în acest sens a fost deja atinsă.
.

MessageId=72
Severity=Success
Facility=System
SymbolicName=ERROR_REDIR_PAUSED
Language=English
The specified printer or disk device has been paused.
.
Language=Russian
Работа указанного принтера или дискового накопителя была остановлена.
.
Language=Polish
Określona drukarka lub urządzenie przerwały pracę.
.
Language=Romanian
Dispozitivul de disc sau imprimanta specificată este în pauză.
.

MessageId=80
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_EXISTS
Language=English
The file exists.
.
Language=Russian
Файл существует.
.
Language=Polish
Plik istnieje.
.
Language=Romanian
Fișierul există.
.

MessageId=82
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_MAKE
Language=English
The directory or file cannot be created.
.
Language=Russian
Не удается создать файл или папку.
.
Language=Polish
Nie można utworzyć katalogu lub pliku.
.
Language=Romanian
Fișierul sau directorul dat nu poate fi creat.
.

MessageId=83
Severity=Success
Facility=System
SymbolicName=ERROR_FAIL_I24
Language=English
Fail on INT 24.
.
Language=Russian
Сбой прерывания INT 24.
.
Language=Polish
Błąd przerwania INT 24.
.
Language=Romanian
Eroare la INT 24.
.

MessageId=84
Severity=Success
Facility=System
SymbolicName=ERROR_OUT_OF_STRUCTURES
Language=English
Storage to process this request is not available.
.
Language=Russian
Недостаточно памяти для обработки запроса.
.
Language=Polish
Pamięć do przetworzenia tego żądania jest niedostępna.
.
Language=Romanian
Necesarul de spațiu pentru această solicitare nu este disponibil.
.

MessageId=85
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_ASSIGNED
Language=English
The local device name is already in use.
.
Language=Russian
Имя локального устройства уже используется.
.
Language=Polish
Nazwa lokalnego urządzenia jest już w użyciu.
.
Language=Romanian
Acest nume de dispozitiv local este deja în uz.
.

MessageId=86
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORD
Language=English
The specified network password is not correct.
.
Language=Russian
Сетевой пароль указан неверно.
.
Language=Polish
Określone hasło sieciowe jest niepoprawne.
.
Language=Romanian
Parola specificată de rețea nu este corectă.
.

MessageId=87
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PARAMETER
Language=English
The parameter is incorrect.
.
Language=Russian
Параметр задан неверно.
.
Language=Polish
Parametr jest niepoprawny.
.
Language=Romanian
Parametru necorespunzător.
.

MessageId=88
Severity=Success
Facility=System
SymbolicName=ERROR_NET_WRITE_FAULT
Language=English
A write fault occurred on the network.
.
Language=Russian
Ошибка записи в сети.
.
Language=Polish
Wystąpił błąd zapisu w sieci.
.
Language=Romanian
Eroare de scriere în rețea.
.

MessageId=89
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PROC_SLOTS
Language=English
The system cannot start another process at this time.
.
Language=Russian
В настоящее время системе не удается запустить другой процесс.
.
Language=Polish
System nie może teraz uruchomić innego procesu.
.
Language=Romanian
La moment în sistem nu pot fi lansate noi procese.
.

MessageId=100
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEMAPHORES
Language=English
Cannot create another system semaphore.
.
Language=Russian
Не удается создать еще один системный семафор.
.
Language=Polish
Nie można utworzyć innego semafora systemowego.
.
Language=Romanian
Un semafor nou de sistem nu poate fi creat.
.

MessageId=101
Severity=Success
Facility=System
SymbolicName=ERROR_EXCL_SEM_ALREADY_OWNED
Language=English
The exclusive semaphore is owned by another process.
.
Language=Russian
Семафор эксклюзивного доступа занят другим процессом.
.
Language=Polish
Semafor wyłączny jest własnością innego procesu.
.
Language=Romanian
Semaforul exclusiv aparține unui alt proces.
.

MessageId=102
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_IS_SET
Language=English
The semaphore is set and cannot be closed.
.
Language=Russian
Семафор установлен и не может быть закрыт.
.
Language=Polish
Semafor jest ustawiony i nie można go zamknąć.
.
Language=Romanian
Semaforul este activ și nu poate fi închis.
.

MessageId=103
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SEM_REQUESTS
Language=English
The semaphore cannot be set again.
.
Language=Russian
Семафор не может быть установлен повторно.
.
Language=Polish
Nie można ponownie zamknąć semafora.
.
Language=Romanian
Semaforul nu poate fi reactivat.
.

MessageId=104
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_AT_INTERRUPT_TIME
Language=English
Cannot request exclusive semaphores at interrupt time.
.
Language=Russian
Запросы к семафорам эксклюзивного доступа на время выполнения прерываний не допускаются.
.
Language=Polish
Nie można żądać semaforów wyłącznych w czasie przerwania.
.
Language=Romanian
Cererea de acces exclusiv la semafoare nu este permisă în timpul unei întreruperi.
.

MessageId=105
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_OWNER_DIED
Language=English
The previous ownership of this semaphore has ended.
.
Language=Russian
Этот семафор более не принадлежит использовавшему его процессу.
.
Language=Polish
Poprzednia przynależność tego semafora skończyła się.
.
Language=Romanian
Apartenența acestui semafor la procesul său a luat sfârșit.
.

MessageId=106
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_USER_LIMIT
Language=English
Insert the diskette for drive %1.
.
Language=Russian
Вставте дискету в дисковод %1.
.
Language=Polish
Włóż dyskietkę do stacji dysków %1.
.
Language=Romanian
Introduceți discul flexibil pentru unitatea %1.
.

MessageId=107
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CHANGE
Language=English
The program stopped because an alternate diskette was not inserted.
.
Language=Russian
Программа была остановлена, так как нужный диск вставлен не был.
.
Language=Polish
Program przestał działać, ponieważ nie włożono innej dyskietki.
.
Language=Romanian
Programul a fost oprit deoarece nu a fost introdus un disc flexibil alternativ.
.

MessageId=108
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVE_LOCKED
Language=English
The disk is in use or locked by another process.
.
Language=Russian
Диск занят или заблокирован другим процессом.
.
Language=Polish
Dysk jest w użyciu lub zablokowany przez inny proces.
.
Language=Romanian
Discul este în uz sau este blocat de un alt proces.
.

MessageId=109
Severity=Success
Facility=System
SymbolicName=ERROR_BROKEN_PIPE
Language=English
The pipe has been ended.
.
Language=Russian
Канал был закрыт.
.
Language=Polish
Potok został zakończony.
.
Language=Romanian
Canalul a fost închis.
.

MessageId=110
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FAILED
Language=English
The system cannot open the device or file specified.
.
Language=Russian
Системе не удается открыть указанное устройство или файл.
.
Language=Polish
System nie może otworzyć określonego urządzenia lub pliku.
.
Language=Romanian
Dispozitivul sau fișierul specificat nu poate fi deschis.
.

MessageId=111
Severity=Success
Facility=System
SymbolicName=ERROR_BUFFER_OVERFLOW
Language=English
The file name is too long.
.
Language=Russian
Указано слишком длинное имя файла.
.
Language=Polish
Nazwa pliku jest za długa.
.
Language=Romanian
Numele de fișier este prea lung.
.

MessageId=112
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_FULL
Language=English
There is not enough space on the disk.
.
Language=Russian
Недостаточно места на диске.
.
Language=Polish
Za mało miejsca na dysku.
.
Language=Romanian
Nu mai este spațiu suficient pe disc.
.

MessageId=113
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_SEARCH_HANDLES
Language=English
No more internal file identifiers available.
.
Language=Russian
Исчерпаны внутренние идентификаторы файлов.
.
Language=Polish
Brak dostępnych wewnętrznych identyfikatorów plików.
.
Language=Romanian
Nu mai sunt disponibili identificatori interni de fișier.
.

MessageId=114
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TARGET_HANDLE
Language=English
The target internal file identifier is incorrect.
.
Language=Russian
Результирующий внутренний идентификатор файла неправилен.
.
Language=Polish
Wewnętrzny identyfikator pliku docelowego jest niepoprawny.
.
Language=Romanian
Identificatorul intern de fișier țintă este necorespunzător.
.

MessageId=117
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CATEGORY
Language=English
The IOCTL call made by the application program is not correct.
.
Language=Russian
Вызов IOCTL приложением произведен неверно.
.
Language=Polish
Wywołanie IOCTL wykonane przez program aplikacji jest niepoprawne.
.
Language=Romanian
Apelul IOCTL efectuat de programul aplicație nu este corespunzător.
.

MessageId=118
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_VERIFY_SWITCH
Language=English
The verify-on-write switch parameter value is not correct.
.
Language=Russian
Параметр проверки записи данных имеет неверное значение.
.
Language=Polish
Wartość parametru przełącznika sprawdź-przy-zapisie jest niepoprawna.
.
Language=Romanian
Valoarea parametrului de verificare la scriere nu este corespunzătoare.
.

MessageId=119
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER_LEVEL
Language=English
The system does not support the command requested.
.
Language=Russian
Система не может обработать полученную команду.
.
Language=Polish
System nie obsługuje żądanego polecenia.
.
Language=Romanian
Comanda solicitată nu este acceptată.
.

MessageId=120
Severity=Success
Facility=System
SymbolicName=ERROR_CALL_NOT_IMPLEMENTED
Language=English
This function is not supported on this system.
.
Language=Russian
Эта функция не поддерживается для этой системы.
.
Language=Polish
Ta funkcja nie jest obsługiwana w tym systemie.
.
Language=Romanian
Această funcție nu este acceptată în acest sistem.
.

MessageId=121
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_TIMEOUT
Language=English
The semaphore timeout period has expired.
.
Language=Russian
Превышен таймаут семафора.
.
Language=Polish
Przekroczono limit czasu semafora.
.
Language=Romanian
Perioada de valabilitate a semaforului a expirat.
.

MessageId=122
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_BUFFER
Language=English
The data area passed to a system call is too small.
.
Language=Russian
Область данных, переданная по системному вызову, слишком мала.
.
Language=Polish
Obszar danych przekazany do wywołania systemowego jest za mały.
.
Language=Romanian
Zona de date pasată unui apelul de sistem este prea mică.
.

MessageId=123
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NAME
Language=English
The filename, directory name, or volume label syntax is incorrect.
.
Language=Russian
Синтаксическая ошибка в имени файла, имени папки или метке тома.
.
Language=Polish
Nazwa pliku, nazwa katalogu lub składnia etykiety woluminu jest niepoprawna.
.
Language=Romanian
Zona de date pasată unui apelul de sistem este prea mică.
.

MessageId=124
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LEVEL
Language=English
The system call level is not correct.
.
Language=Russian
Неверный уровень системного вызова.
.
Language=Polish
Poziom wywołania systemowego jest niepoprawny.
.
Language=Romanian
Nivelul apelului de sistem nu este corespunzător.
.

MessageId=125
Severity=Success
Facility=System
SymbolicName=ERROR_NO_VOLUME_LABEL
Language=English
The disk has no volume label.
.
Language=Russian
У диска отсутствует метка тома.
.
Language=Polish
Dysk nie ma etykiety woluminu.
.
Language=Romanian
Discul nu are etichetă de volum.
.

MessageId=126
Severity=Success
Facility=System
SymbolicName=ERROR_MOD_NOT_FOUND
Language=English
The specified module could not be found.
.
Language=Russian
Не найден указанный модуль.
.
Language=Polish
Nie można odnaleźć określonego modułu.
.
Language=Romanian
Modulul specificat nu a fost găsit.
.

MessageId=127
Severity=Success
Facility=System
SymbolicName=ERROR_PROC_NOT_FOUND
Language=English
The specified procedure could not be found.
.
Language=Russian
Не найдена указанная процедура.
.
Language=Polish
Nie można odnaleźć określonej procedury.
.
Language=Romanian
Procedura specificată nu a fost găsită.
.

MessageId=128
Severity=Success
Facility=System
SymbolicName=ERROR_WAIT_NO_CHILDREN
Language=English
There are no child processes to wait for.
.
Language=Russian
Дочерние процессы, окончания которых требуется ожидать, отсутствуют.
.
Language=Polish
Nie ma procesów podrzędnych, na które trzeba by czekać.
.
Language=Romanian
Nu a mai rămas de așteptat nici un sub-proces.
.

MessageId=129
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_NOT_COMPLETE
Language=English
The %1 application cannot be run in Win32 mode.
.
Language=Russian
Приложение "%1" не может быть запущено в режиме Win32.
.
Language=Polish
Nie można uruchomić %1 w trybie Win32.
.
Language=Romanian
Aplicația «%1» nu poate fi lansată în mod Win32.
.

MessageId=130
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECT_ACCESS_HANDLE
Language=English
Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.
.
Language=Russian
Попытка использовать дескриптор файла для открытия раздела диска и выполнения операции, отличающейся от ввода/вывода нижнего уровня.
.
Language=Polish
Próbowano użyć dojścia do pliku do otwarcia partycji dysku dla operacji innej niż czysta dyskowa operacja We/Wy.
.
Language=Romanian
Încercare de a utiliza pentru o partiție de disc un identificator de gestiune de fișiere pentru operații în afara celor de acces brut de In/Ex.
.

MessageId=131
Severity=Success
Facility=System
SymbolicName=ERROR_NEGATIVE_SEEK
Language=English
An attempt was made to move the file pointer before the beginning of the file.
.
Language=Russian
Попытка поместить указатель на файл перед началом файла.
.
Language=Polish
Wykonano próbę przesunięcia wskaźnika pliku przed początek pliku.
.
Language=Romanian
Încercare de a plasa un indicator de fișier către o valoare negativă.
.

MessageId=132
Severity=Success
Facility=System
SymbolicName=ERROR_SEEK_ON_DEVICE
Language=English
The file pointer cannot be set on the specified device or file.
.
Language=Russian
Указатель на файл не может быть установлен на заданное устройство или файл.
.
Language=Polish
Wskaźnik plików nie może być ustawiony na określonym urządzeniu lub pliku.
.
Language=Romanian
Indicatorul de fișier nu poate fi plasat pe fișierul sau dispozitivul specificat.
.

MessageId=133
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_TARGET
Language=English
A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.
.
Language=Russian
Команды JOIN и SUBST не могут быть использованы для дисков, содержащих уже объединенные диски.
.
Language=Polish
Polecenia JOIN lub SUBST nie mogą być użyte na dysku zawierającym poprzednio sprzężone dyski.
.
Language=Romanian
O comandă JOIN sau SUBST nu poate fi lansată pentru o unitate de stocare care deja conține asocieri de alte unități de stocare.
.

MessageId=134
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOINED
Language=English
An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.
.
Language=Russian
Попытка использовать команду JOIN или SUBST для диска, уже включенного в набор объединенных дисков.
.
Language=Polish
Wykonano próbę użycia polecenia JOIN lub SUBST dla dysku, który został już sprzęgnięty.
.
Language=Romanian
Încercare de a lansa comanda JOIN sau SUBST pentru o unitate de stocare care deja a fost asociată.
.

MessageId=135
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBSTED
Language=English
An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.
.
Language=Russian
Попытка использовать команду JOIN или SUBST для диска, который уже был отображен.
.
Language=Polish
Wykonano próbę użycia polecenia JOIN lub SUBST na dysku, który uległ już podstawieniu.
.
Language=Romanian
Încercare de a lansa comanda JOIN sau SUBST pentru o unitate de stocare care a fost deja substituită.
.

MessageId=136
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_JOINED
Language=English
The system tried to delete the JOIN of a drive that is not joined.
.
Language=Russian
Попытка снять признак объединения с диска, для которого команда JOIN не выполнялась.
.
Language=Polish
System próbował usunąć stan JOIN dysku, który nie jest sprzęgnięty (JOIN).
.
Language=Romanian
Încercare de a elimina o unitate de stocare care nu este rezultat al unei asocieri.
.

MessageId=137
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUBSTED
Language=English
The system tried to delete the substitution of a drive that is not substituted.
.
Language=Russian
Попытка снять признак отображения с диска, для которого команда SUBST не выполнялась.
.
Language=Polish
System próbował usunąć podstawienie dysku, który nie uległ podstawieniu.
.
Language=Romanian
Încercare de a elimina o unitate de stocare care nu este un rezultat al unei substituții.
.

MessageId=138
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_JOIN
Language=English
The system tried to join a drive to a directory on a joined drive.
.
Language=Russian
Попытка объединить диск с папкой на объединенном диске.
.
Language=Polish
System próbował sprzęgnąć dysk z katalogiem na dysku sprzęgniętym.
.
Language=Romanian
Încercare de a asocia o unitate de stocare într-un director al unei unități de stocare asociate.
.

MessageId=139
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_SUBST
Language=English
The system tried to substitute a drive to a directory on a substituted drive.
.
Language=Russian
Попытка отобразить диск на папку, находящуюся на отображенном диске.
.
Language=Polish
System próbował dokonać podstawienia dysku przez katalog na dysku poddanym podstawieniu.
.
Language=Romanian
Încercare de substituție a unei unități de stocare într-un director al unei unități de stocare substituite.
.

MessageId=140
Severity=Success
Facility=System
SymbolicName=ERROR_JOIN_TO_SUBST
Language=English
The system tried to join a drive to a directory on a substituted drive.
.
Language=Russian
Попытка объединить диск с папкой на отображенном диске.
.
Language=Polish
System próbował sprzęgnąć dysk z katalogiem na dysku poddanym podstawieniu.
.
Language=Romanian
Încercare de asociere a unei unități de stocare la un director dintr-o unitate de stocare substituită.
.

MessageId=141
Severity=Success
Facility=System
SymbolicName=ERROR_SUBST_TO_JOIN
Language=English
The system tried to SUBST a drive to a directory on a joined drive.
.
Language=Russian
Попытка отобразить диск на папку, находящуюся на объединенном диске.
.
Language=Polish
System próbował dokonać podstawienia dysku przez katalog na dysku sprzęgniętym.
.
Language=Romanian
Încercare de substituție a unei unități de stocare la un director dintr-o unitate de stocare asociată.
.

MessageId=142
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY_DRIVE
Language=English
The system cannot perform a JOIN or SUBST at this time.
.
Language=Russian
В настоящее время выполнить команду JOIN или SUBST невозможно.
.
Language=Polish
System nie może teraz wykonać polecenia JOIN ani SUBST.
.
Language=Romanian
Deocamdată nu pot fi efectuate operații de asociere sau substituție.
.

MessageId=143
Severity=Success
Facility=System
SymbolicName=ERROR_SAME_DRIVE
Language=English
The system cannot join or substitute a drive to or for a directory on the same drive.
.
Language=Russian
Невозможно объединить (или отобразить) диск с папкой (или на папку) этого же диска.
.
Language=Polish
System nie może sprzęgnąć lub dokonać podstawienia dysku (JOIN lub SUBST) przy użyciu katalogu na tym samym dysku.
.
Language=Romanian
O unitate de stocare la sau pentru un director de pe aceiași unitate de stocare nu poate fi asociată sau substituită.
.

MessageId=144
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_ROOT
Language=English
The directory is not a subdirectory of the root directory.
.
Language=Russian
Эта папка не является подпапкой корневой папки.
.
Language=Polish
Katalog nie jest podkatalogiem katalogu głównego.
.
Language=Romanian
Directorul nu este subdirector al directorului rădăcină.
.

MessageId=145
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_NOT_EMPTY
Language=English
The directory is not empty.
.
Language=Russian
Папка не пуста.
.
Language=Polish
Katalog nie jest pusty.
.
Language=Romanian
Directorul nu este gol.
.

MessageId=146
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_PATH
Language=English
The path specified is being used in a substitute.
.
Language=Russian
Указанный путь используется для отображенного диска.
.
Language=Polish
Określona ścieżka jest używana w podstawieniu.
.
Language=Romanian
Calea specificată este deja utilizată într-o substituție.
.

MessageId=147
Severity=Success
Facility=System
SymbolicName=ERROR_IS_JOIN_PATH
Language=English
Not enough resources are available to process this command.
.
Language=Russian
Недостаточно ресурсов для обработки команды.
.
Language=Polish
Za mało zasobów do przetworzenia tego polecenia.
.
Language=Romanian
Nu există suficiente resurse disponibile pentru a executa această comandă.
.

MessageId=148
Severity=Success
Facility=System
SymbolicName=ERROR_PATH_BUSY
Language=English
The path specified cannot be used at this time.
.
Language=Russian
Указанный путь невозможно использовать сейчас.
.
Language=Polish
Nie można teraz użyć określonej ścieżki.
.
Language=Romanian
Calea specificată nu poate fi utilizată deocamdată.
.

MessageId=149
Severity=Success
Facility=System
SymbolicName=ERROR_IS_SUBST_TARGET
Language=English
An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.
.
Language=Russian
Попытка объединить или отобразить диск, папка на котором уже используется для отображения.
.
Language=Polish
Wykonano próbę sprzęgnięcia (JOIN) lub podstawienia (SUBST) dysku, dla którego katalog na dysku jest katalogiem docelowym poprzedniego podstawienia.
.
Language=Romanian
Încercare de a asocia sau substitui o unitate de stocare care conține un director substituit.
.

MessageId=150
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_TRACE
Language=English
System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.
.
Language=Russian
Сведения о трассировке в файле CONFIG.SYS не найдены, либо трассировка запрещена.
.
Language=Polish
СInformacje o śledzeniu systemu nie zostały określone w pliku CONFIG.SYS lub śledzenie jest niedozwolone.
.
Language=Romanian
În fișierul CONFIG.SYS nu sunt specificate informații de trasare pentru sistem, sau trasarea este dezactivată.
.

MessageId=151
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENT_COUNT
Language=English
The number of specified semaphore events for DosMuxSemWait is not correct.
.
Language=Russian
Число семафоров для DosMuxSemWait задано неверно.
.
Language=Polish
Liczba określonych zdarzeń semafora dla DosMuxSemWait jest niepoprawna.
.
Language=Romanian
Numărul evenimentelor de semafor specificate pentru DosMuxSemWait nu este corespunzător.
.

MessageId=152
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MUXWAITERS
Language=English
DosMuxSemWait did not execute; too many semaphores are already set.
.
Language=Russian
Не выполнен вызов DosMuxSemWait. Установлено слишком много семафоров.
.
Language=Polish
Nie wykonano funkcji DosMuxSemWait; za dużo semaforów jest już ustawionych.
.
Language=Romanian
DosMuxSemWait nu a fost lansat; deja sunt prea multe semafoare active.
.

MessageId=153
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LIST_FORMAT
Language=English
The DosMuxSemWait list is not correct.
.
Language=Russian
Некорректный вызов DosMuxSemWait.
.
Language=Polish
Lista DosMuxSemWait jest niepoprawna.
.
Language=Romanian
Lista DosMuxSemWait nu este corespunzătoare.
.

MessageId=154
Severity=Success
Facility=System
SymbolicName=ERROR_LABEL_TOO_LONG
Language=English
The volume label you entered exceeds the label character limit of the target file system.
.
Language=Russian
Длина метки тома превосходит предел, установленный для файловой системы.
.
Language=Polish
Wprowadzona etykieta woluminu przekracza limit znaków etykiety docelowego systemu plików.
.
Language=Romanian
Lungimea pentru eticheta de volum depășește limita corespunzătoare etichetei pentru sistemul de fișiere destinație.
.

MessageId=155
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_TCBS
Language=English
Cannot create another thread.
.
Language=Russian
Не удается создать еще один поток команд.
.
Language=Polish
Nie można utworzyć innego wątku.
.
Language=Romanian
Un alt fir de execuție nu poate fi creat.
.

MessageId=156
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_REFUSED
Language=English
The recipient process has refused the signal.
.
Language=Russian
Принимающий процесс отклонил сигнал.
.
Language=Polish
Proces odbiorczy odrzucił sygnał.
.
Language=Romanian
Procesul recipient a refuzat semnalul.
.

MessageId=157
Severity=Success
Facility=System
SymbolicName=ERROR_DISCARDED
Language=English
The segment is already discarded and cannot be locked.
.
Language=Russian
Сегмент уже освобожден и не может быть заблокирован.
.
Language=Polish
Segment jest już zarzucony i nie można go zablokować.
.
Language=Romanian
Segmentul deja a fost înlăturat și nu poate fi blocat.
.

MessageId=158
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOCKED
Language=English
The segment is already unlocked.
.
Language=Russian
Блокировка с сегмента уже снята.
.
Language=Polish
Segment jest już odblokowany.
.
Language=Romanian
Segmentul deja este deblocat.
.

MessageId=159
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_THREADID_ADDR
Language=English
The address for the thread ID is not correct.
.
Language=Russian
Адрес идентификатора потока команд задан неверно.
.
Language=Polish
Adres identyfikatora wątku jest niepoprawny.
.
Language=Romanian
Adresa pentru ID de fir de execuție nu este corespunzător.
.

MessageId=160
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ARGUMENTS
Language=English
The argument string passed to DosExecPgm is not correct.
.
Language=Russian
Неверны один или несколько аргументов.
.
Language=Polish
Co najmniej jeden argument jest niepoprawny.
.
Language=Romanian
Șirul de argumente pasat către DosExecPgm nu este corespunzător.
.

MessageId=161
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PATHNAME
Language=English
The specified path is invalid.
.
Language=Russian
Указан недопустимый путь.
.
Language=Polish
Określona ścieżka jest nieprawidłowa.
.
Language=Romanian
Calea specificată nu este validă.
.

MessageId=162
Severity=Success
Facility=System
SymbolicName=ERROR_SIGNAL_PENDING
Language=English
A signal is already pending.
.
Language=Russian
Сигнал уже находится в состоянии обработки.
.
Language=Polish
Sygnał jest już w stanie oczekiwania.
.
Language=Romanian
Semnalul deja este în așteptare.
.

MessageId=164
Severity=Success
Facility=System
SymbolicName=ERROR_MAX_THRDS_REACHED
Language=English
No more threads can be created in the system.
.
Language=Russian
Создание дополнительных потоков команд невозможно.
.
Language=Polish
W systemie nie można utworzyć dalszych wątków.
.
Language=Romanian
Mai multe fire de execuție nu pot fi create.
.

MessageId=167
Severity=Success
Facility=System
SymbolicName=ERROR_LOCK_FAILED
Language=English
Unable to lock a region of a file.
.
Language=Russian
Не удается снять блокировку с области файла.
.
Language=Polish
Nie można zablokować regionu pliku.
.
Language=Romanian
O parte din fișier nu a putut fi blocată.
.

MessageId=170
Severity=Success
Facility=System
SymbolicName=ERROR_BUSY
Language=English
The requested resource is in use.
.
Language=Russian
Требуемый ресурс занят.
.
Language=Polish
Żądane zasoby są w użyciu.
.
Language=Romanian
Resursa solicitată este deja în uz.
.

MessageId=173
Severity=Success
Facility=System
SymbolicName=ERROR_CANCEL_VIOLATION
Language=English
A lock request was not outstanding for the supplied cancel region.
.
Language=Russian
Запрос на блокировку соответствует определенной области.
.
Language=Polish
Żądanie zablokowania nie było zaległe dla podanego regionu anulowania.
.
Language=Romanian
Pentru zona de anulare furnizată există o cerere de blocare neadresată.
.

MessageId=174
Severity=Success
Facility=System
SymbolicName=ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
Language=English
The file system does not support atomic changes to the lock type.
.
Language=Russian
Файловая система не поддерживает указанные изменения типа блокировки.
.
Language=Polish
System plików nie obsługuje zmian częściowych dotyczących typu blokady.
.
Language=Romanian
Sistemul de fișiere nu permite blocări pentru modificări atomice.
.

MessageId=180
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGMENT_NUMBER
Language=English
The system detected a segment number that was not correct.
.
Language=Russian
Система обнаружила неверный номер сегмента.
.
Language=Polish
System wykrył niepoprawny numer segmentu.
.
Language=Romanian
Un număr de segment a fost identificat ca fiind necorespunzător.
.

MessageId=182
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ORDINAL
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
«%1» nu poate fi executat.
.

MessageId=183
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_EXISTS
Language=English
Cannot create a file when that file already exists.
.
Language=Russian
Невозможно создать файл, так как он уже существует.
.
Language=Polish
Nie można utworzyć pliku, który już istnieje.
.
Language=Romanian
Nu poate fi creat un fișier când acesta deja există.
.

MessageId=186
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAG_NUMBER
Language=English
The flag passed is not correct.
.
Language=Russian
Передан неверный флаг.
.
Language=Polish
Przekazana flaga jest niepoprawna.
.
Language=Romanian
Fanionul furnizat nu este corespunzător.
.

MessageId=187
Severity=Success
Facility=System
SymbolicName=ERROR_SEM_NOT_FOUND
Language=English
The specified system semaphore name was not found.
.
Language=Russian
Не найдено указанное имя системного семафора.
.
Language=Polish
Nie odnaleziono określonej nazwy semafora systemowego.
.
Language=Romanian
Numele de semafor specificat nu a fost găsit.
.

MessageId=188
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STARTING_CODESEG
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=189
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STACKSEG
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=190
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MODULETYPE
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=191
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EXE_SIGNATURE
Language=English
Cannot run %1 in Win32 mode.
.
Language=Russian
Невозможно запустить "%1" в режиме Win32.
.
Language=Polish
Nie można uruchomić %1 w trybie Win32.
.
Language=Romanian
«%1» nu poate fi lansată în mod Win32.
.

MessageId=192
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_MARKED_INVALID
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=193
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_EXE_FORMAT
Language=English
%1 is not a valid Win32 application.
.
Language=Russian
"%1" не является приложением Win32.
.
Language=Polish
%1 nie jest prawidłową aplikacją systemu Win32.
.
Language=Romanian
«%1» nu este o aplicație Win32.
.

MessageId=194
Severity=Success
Facility=System
SymbolicName=ERROR_ITERATED_DATA_EXCEEDS_64k
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=195
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MINALLOCSIZE
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=196
Severity=Success
Facility=System
SymbolicName=ERROR_DYNLINK_FROM_INVALID_RING
Language=English
The operating system cannot run this application program.
.
Language=Russian
Операционная система не может запустить это приложение.
.
Language=Polish
System operacyjny nie może uruchomić tej aplikacji programu.
.
Language=Romanian
Aplicația program nu poate fi lansată.
.

MessageId=197
Severity=Success
Facility=System
SymbolicName=ERROR_IOPL_NOT_ENABLED
Language=English
The operating system is not presently configured to run this application.
.
Language=Russian
Конфигурация операционной системы не рассчитана на запуск этого приложения.
.
Language=Polish
System operacyjny nie jest obecnie skonfigurowany do uruchomienia tej aplikacji.
.
Language=Romanian
Sistemul de operare nu este deocamdată configurat pentru a putea lansa această aplicație.
.

MessageId=198
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEGDPL
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=199
Severity=Success
Facility=System
SymbolicName=ERROR_AUTODATASEG_EXCEEDS_64k
Language=English
The operating system cannot run this application program.
.
Language=Russian
Операционная система не может запустить это приложение.
.
Language=Polish
System operacyjny nie może uruchomić tej aplikacji programu.
.
Language=Romanian
Aplicația program nu poate fi lansată.
.

MessageId=200
Severity=Success
Facility=System
SymbolicName=ERROR_RING2SEG_MUST_BE_MOVABLE
Language=English
The code segment cannot be greater than or equal to 64K.
.
Language=Russian
Сегмент кода должен быть меньше 64 КБ.
.
Language=Polish
Segment kodu nie może być większy niż lub równy 64 KB.
.
Language=Romanian
Lungimea codului de segment nu poate fi egală cu sau depăși 64 Кo.
.

MessageId=201
Severity=Success
Facility=System
SymbolicName=ERROR_RELOC_CHAIN_XEEDS_SEGLIM
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=202
Severity=Success
Facility=System
SymbolicName=ERROR_INFLOOP_IN_RELOC_CHAIN
Language=English
The operating system cannot run %1.
.
Language=Russian
Операционная система не может выполнить "%1".
.
Language=Polish
System operacyjny nie może uruchomić %1.
.
Language=Romanian
Sistemul de operare nu poate lansa «%1».
.

MessageId=203
Severity=Success
Facility=System
SymbolicName=ERROR_ENVVAR_NOT_FOUND
Language=English
The system could not find the environment option that was entered.
.
Language=Russian
Системе не удается найти указанный параметр среды.
.
Language=Polish
System nie mógł znaleźć opcji środowiska, która została wprowadzona.
.
Language=Romanian
Opțiunea de mediu introdusă nu a fost găsită.
.

MessageId=205
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SIGNAL_SENT
Language=English
No process in the command subtree has a signal handler.
.
Language=Russian
Ни один из процессов в дереве команды не имеет обработчика сигналов.
.
Language=Polish
Żaden proces w poddrzewie polecenia nie ma obsługi sygnałów.
.
Language=Romanian
Nici un proces din subarborele de comandă nu are un identificator de gestiune pentru semnale.
.

MessageId=206
Severity=Success
Facility=System
SymbolicName=ERROR_FILENAME_EXCED_RANGE
Language=English
The filename or extension is too long.
.
Language=Russian
Имя файла или его расширение имеет слишком большую длину.
.
Language=Polish
Nazwa pliku lub jej rozszerzenie są za długie.
.
Language=Romanian
Lungimea numelui de fișier sau a extensiei sale este prea mare.
.

MessageId=207
Severity=Success
Facility=System
SymbolicName=ERROR_RING2_STACK_IN_USE
Language=English
The ring 2 stack is in use.
.
Language=Russian
Кольцо 2 стека занято.
.
Language=Polish
Stos ring 2 jest w użyciu.
.
Language=Romanian
Stiva pentru inelul 2 este în uz.
.

MessageId=208
Severity=Success
Facility=System
SymbolicName=ERROR_META_EXPANSION_TOO_LONG
Language=English
The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.
.
Language=Russian
Подстановочные знаки * и/или ? заданы неверно или образуют неверный шаблон имени.
.
Language=Polish
Znaki globalne w nazwach plików, * lub ?, są niepoprawnie wprowadzone lub określono za dużo znaków globalnych.
.
Language=Romanian
Fie are loc o utilizare greșită a caracterelor-mască «*» sau «?», fie sunt utilizate prea multe astfel de caractere.
.

MessageId=209
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SIGNAL_NUMBER
Language=English
The signal being posted is not correct.
.
Language=Russian
Отправляемый сигнал неверен.
.
Language=Polish
Ogłaszany sygnał jest nieprawidłowy.
.
Language=Romanian
Semnalul postat nu este corespunzător.
.

MessageId=210
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_1_INACTIVE
Language=English
The signal handler cannot be set.
.
Language=Russian
Не удается установить обработчик сигналов.
.
Language=Polish
Nie można ustawić programu obsługi sygnałów.
.
Language=Romanian
Identificatorul de gestiune de semnale nu poate fi activat.
.

MessageId=212
Severity=Success
Facility=System
SymbolicName=ERROR_LOCKED
Language=English
The segment is locked and cannot be reallocated.
.
Language=Russian
Сегмент заблокирован и не может быть перемещен.
.
Language=Polish
Segment jest zablokowany i nie można przydzielić go ponownie.
.
Language=Romanian
Segmentul este blocat și nu poate fi realocat.
.

MessageId=214
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_MODULES
Language=English
Too many dynamic-link modules are attached to this program or dynamic-link module.
.
Language=Russian
К этой программе или модулю присоединено слишком много динамически подключаемых модулей.
.
Language=Polish
Do tego programu lub modułu dołączono za dużo modułów dołączanych dynamicznie.
.
Language=Romanian
Către acest program sau modul dinamic au fost atașate prea multe module dinamice.
.

MessageId=215
Severity=Success
Facility=System
SymbolicName=ERROR_NESTING_NOT_ALLOWED
Language=English
Cannot nest calls to LoadModule.
.
Language=Russian
Вызовы LoadModule не могут быть вложены.
.
Language=Polish
Nie można zagnieżdżać wywołań funkcji LoadModule.
.
Language=Romanian
LoadModule nu poate conține apeluri imbricate.
.

MessageId=216
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_MACHINE_TYPE_MISMATCH
Language=English
The image file %1 is valid, but is for a machine type other than the current machine.
.
Language=Russian
The image file %1 is valid, but is for a machine type other than the current machine.
.
Language=Polish
Plik obrazu %1 jest prawidłowy, ale jest przeznaczony na komputer innego typu niż obecny.
.
Language=Romanian
Fișierul imagine «%1» este valid, însă nu pentru tipul mașinii de calcul curente.
.

MessageId=217
Severity=Success
Facility=System
SymbolicName=ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY
Language=English
The image file %1 is signed, unable to modify.
.
Language=Russian
The image file %1 is signed, unable to modify.
.
Language=Polish
Plik obrazu %1 jest podpisany, nie można go zmodyfikować.
.
Language=Romanian
Fișierul imagine «%1» este semnat electronic, nu poate fi modificat.
.

MessageId=218
Severity=Success
Facility=System
SymbolicName=ERRO_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY
Language=English
The image file %1 is strong signed, unable to modify.
.
Language=Russian
The image file %1 is strong signed, unable to modify.
.
Language=Polish
Plik obrazu %1 ma silny podpis, nie można go zmodyfikować.
.
Language=Romanian
Fișierul imagine «%1» este semnat electronic puternic, nu poate fi modificat.
.

MessageId=230
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PIPE
Language=English
The pipe state is invalid.
.
Language=Russian
The pipe state is invalid.
.
Language=Polish
Stan potoku jest nieprawidłowy.
.
Language=Romanian
Starea canalului nu este validă.
.

MessageId=231
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_BUSY
Language=English
All pipe instances are busy.
.
Language=Russian
All pipe instances are busy.
.
Language=Polish
Wszystkie wystąpienia potoku są zajęte.
.
Language=Romanian
Toate instanțele canalului sunt ocupate.
.

MessageId=232
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA
Language=English
The pipe is being closed.
.
Language=Russian
The pipe is being closed.
.
Language=Polish
Trwa zamykanie potoku.
.
Language=Romanian
Canalul este închis.
.

MessageId=233
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_NOT_CONNECTED
Language=English
No process is on the other end of the pipe.
.
Language=Russian
No process is on the other end of the pipe.
.
Language=Polish
Na drugim końcu potoku nie ma żadnego procesu.
.
Language=Romanian
La celălalt capăt al canalului nu se află nici un proces.
.

MessageId=234
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_DATA
Language=English
More data is available.
.
Language=Russian
More data is available.
.
Language=Polish
Dostępnych jest więcej danych.
.
Language=Romanian
Sunt disponibile mai multe date.
.

MessageId=240
Severity=Success
Facility=System
SymbolicName=ERROR_VC_DISCONNECTED
Language=English
The session was canceled.
.
Language=Russian
The session was canceled.
.
Language=Polish
Sesja została anulowana.
.
Language=Romanian
Sesiunea a fost anulată.
.

MessageId=254
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_NAME
Language=English
The specified extended attribute name was invalid.
.
Language=Russian
The specified extended attribute name was invalid.
.
Language=Polish
Określona nazwa atrybutu rozszerzonego jest nieprawidłowa.
.
Language=Romanian
Numele atributului extins care a fost specificat nu este corespunzător.
.

MessageId=255
Severity=Success
Facility=System
SymbolicName=ERROR_EA_LIST_INCONSISTENT
Language=English
The extended attributes are inconsistent.
.
Language=Russian
The extended attributes are inconsistent.
.
Language=Polish
Atrybuty rozszerzone są niezgodne.
.
Language=Romanian
Atributele extinse nu sunt consistente.
.

MessageId=258
Severity=Success
Facility=System
SymbolicName=WAIT_TIMEOUT
Language=English
The wait operation timed out.
.
Language=Russian
The wait operation timed out.
.
Language=Polish
Upłynął limit czasu operacji oczekiwania.
.
Language=Romanian
Operația de așteptare a expirat.
.

MessageId=259
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_ITEMS
Language=English
No more data is available.
.
Language=Russian
No more data is available.
.
Language=Polish
Brak dalszych danych.
.
Language=Romanian
Nu sunt disponibile mai multe date.
.

MessageId=266
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_COPY
Language=English
The copy functions cannot be used.
.
Language=Russian
The copy functions cannot be used.
.
Language=Polish
Nie można używać funkcji kopiowania.
.
Language=Romanian
Funcțiile de copiere nu pot fi utilizate.
.

MessageId=267
Severity=Success
Facility=System
SymbolicName=ERROR_DIRECTORY
Language=English
The directory name is invalid.
.
Language=Russian
The directory name is invalid.
.
Language=Polish
Nazwa katalogu jest nieprawidłowa.
.
Language=Romanian
Numele directorului nu este corespunzător.
.

MessageId=275
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_DIDNT_FIT
Language=English
The extended attributes did not fit in the buffer.
.
Language=Russian
The extended attributes did not fit in the buffer.
.
Language=Polish
Atrybuty rozszerzone nie zmieściły się w buforze.
.
Language=Romanian
Atributul extins nu a încăput în memorie.
.

MessageId=276
Severity=Success
Facility=System
SymbolicName=ERROR_EA_FILE_CORRUPT
Language=English
The extended attribute file on the mounted file system is corrupt.
.
Language=Russian
The extended attribute file on the mounted file system is corrupt.
.
Language=Polish
Plik atrybutów rozszerzonych w zainstalowanym systemie plików jest uszkodzony.
.
Language=Romanian
Fișierul cu tabela de atribute extinse de pe sistemul de fișiere montat este deteriorat.
.

MessageId=277
Severity=Success
Facility=System
SymbolicName=ERROR_EA_TABLE_FULL
Language=English
The extended attribute table file is full.
.
Language=Russian
The extended attribute table file is full.
.
Language=Polish
Tabela atrybutów rozszerzonych jest zapełniona.
.
Language=Romanian
Fișierul cu tabela de atribute extinse este plin.
.

MessageId=278
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EA_HANDLE
Language=English
The specified extended attribute handle is invalid.
.
Language=Russian
The specified extended attribute handle is invalid.
.
Language=Polish
Określone dojście atrybutu rozszerzonego jest nieprawidłowe.
.
Language=Romanian
Identificatorul de gestiune al atributului extins nu este valid.
.

MessageId=282
Severity=Success
Facility=System
SymbolicName=ERROR_EAS_NOT_SUPPORTED
Language=English
The mounted file system does not support extended attributes.
.
Language=Russian
The mounted file system does not support extended attributes.
.
Language=Polish
Zainstalowany system plików nie obsługuje atrybutów rozszerzonych.
.
Language=Romanian
Sistemul de fișiere atașat nu permite atribute extinse.
.

MessageId=288
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_OWNER
Language=English
Attempt to release mutex not owned by caller.
.
Language=Russian
Attempt to release mutex not owned by caller.
.
Language=Polish
Próbowano zwolnić mutex nie będący własnością wywołującego.
.
Language=Romanian
Tentativă de cedare a unui mutex în afara posesiei curente.
.

MessageId=298
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_POSTS
Language=English
Too many posts were made to a semaphore.
.
Language=Russian
Too many posts were made to a semaphore.
.
Language=Polish
Wykonano za dużo przesłań do semafora.
.
Language=Romanian
Număr de plasamente peste limită adresate unui semafor.
.

MessageId=299
Severity=Success
Facility=System
SymbolicName=ERROR_PARTIAL_COPY
Language=English
Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
.
Language=Russian
Only part of a ReadProcessMemory or WriteProcessMemory request was completed.
.
Language=Polish
Ukończono tylko część żądania ReadProcessMemory lub WriteProcessMemory.
.
Language=Romanian
Cererea ReadProcessMemory sau WriteProcessMemory a fost îndeplinită doar parțial.
.

MessageId=300
Severity=Success
Facility=System
SymbolicName=ERROR_OPLOCK_NOT_GRANTED
Language=English
The oplock request is denied.
.
Language=Russian
The oplock request is denied.
.
Language=Polish
Odmowa żądania operacji oplock.
.
Language=Romanian
Cererea pentru blocare oportună (oplock) a fost respintă.
.

MessageId=301
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPLOCK_PROTOCOL
Language=English
An invalid oplock acknowledgment was received by the system.
.
Language=Russian
An invalid oplock acknowledgment was received by the system.
.
Language=Polish
System odebrał nieprawidłowe potwierdzenie zablokowania operacji.
.
Language=Romanian
Confirmarea primită pentru blocarea oportună (oplock) nu este validă.
.

MessageId=302
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_TOO_FRAGMENTED
Language=English
The volume is too fragmented to complete this operation.
.
Language=Russian
The volume is too fragmented to complete this operation.
.
Language=Polish
Wolumin jest zbyt pofragmentowany, aby ukończyć tę operację.
.
Language=Romanian
Volumul este prea fragmentat pentru a îndeplini această operație.
.

MessageId=303
Severity=Success
Facility=System
SymbolicName=ERROR_DELETE_PENDING
Language=English
The file cannot be opened because it is in the process of being deleted.
.
Language=Russian
The file cannot be opened because it is in the process of being deleted.
.
Language=Polish
Nie można otworzyć pliku, ponieważ trwa proces jego usuwania.
.
Language=Romanian
Fișierul nu poate fi deschis deoarece este în curs de ștergere.
.

MessageId=317
Severity=Success
Facility=System
SymbolicName=ERROR_MR_MID_NOT_FOUND
Language=English
The system cannot find message text for message number 0x%1 in the message file for %2.
.
Language=Russian
The system cannot find message text for message number 0x%1 in the message file for %2.
.
Language=Polish
System nie może znaleźć komunikatu dla numeru komunikatu 0x%1 w pliku komunikatów dla %2.
.
Language=Romanian
Textul pentru mesajul cu numărul 0x%1 din fișierul de mesaja pentru %2 nu a putut fi găsit.
.

MessageId=318
Severity=Success
Facility=System
SymbolicName=ERROR_SCOPE_NOT_FOUND
Language=English
The scope specified was not found.
.
Language=Russian
The scope specified was not found.
.
Language=Polish
Nie można odnaleźć określonego zakresu.
.
Language=Romanian
Domeniul de acțiune specificat nu a fost găsit.
.

MessageId=487
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ADDRESS
Language=English
Attempt to access invalid address.
.
Language=Russian
Attempt to access invalid address.
.
Language=Polish
Próbowano uzyskać dostęp do nieprawidłowego adresu.
.
Language=Romanian
Tentativă de acces a unei adrese nevalide.
.

MessageId=534
Severity=Success
Facility=System
SymbolicName=ERROR_ARITHMETIC_OVERFLOW
Language=English
Arithmetic result exceeded 32 bits.
.
Language=Russian
Arithmetic result exceeded 32 bits.
.
Language=Polish
Wynik arytmetyczny przekroczył 32 bity.
.
Language=Romanian
Rezultatul aritmetic depășește 32 de biți.
.

MessageId=535
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_CONNECTED
Language=English
There is a process on other end of the pipe.
.
Language=Russian
There is a process on other end of the pipe.
.
Language=Polish
Na drugim końcu potoku jest proces.
.
Language=Romanian
La celălalt capăt al conectorului (pipe) deja există un proces.
.

MessageId=536
Severity=Success
Facility=System
SymbolicName=ERROR_PIPE_LISTENING
Language=English
Waiting for a process to open the other end of the pipe.
.
Language=Russian
Waiting for a process to open the other end of the pipe.
.
Language=Polish
Oczekiwanie na otwarcie przez proces drugiego końca potoku.
.
Language=Romanian
Un proces este așteptat la capătul complementar al conectorului (pipe).
.

MessageId=537
Severity=Success
Facility=System
SymbolicName=ERROR_ACPI_ERROR
Language=English
An error occurred in the ACPI subsystem.
.
Language=Russian
An error occurred in the ACPI subsystem.
.
Language=Polish
Wystąpił błąd w podsystemie ACPI.
.
Language=Romanian
În subsistemul ACPI a apărut o eroare.
.

MessageId=538
Severity=Success
Facility=System
SymbolicName=ERROR_ABIOS_ERROR
Language=English
An error occurred in the ABIOS subsystem
.
Language=Russian
An error occurred in the ABIOS subsystem
.
Language=Polish
Wystąpił błąd w podsystemie ABIOS.
.
Language=Romanian
În subsistemul ABIOS a apărut o eroare.
.

MessageId=539
Severity=Success
Facility=System
SymbolicName=ERROR_WX86_WARNING
Language=English
A warning occurred in the WX86 subsystem.
.
Language=Russian
A warning occurred in the WX86 subsystem.
.
Language=Polish
Wystąpiło ostrzeżenie w podsystemie WX86.
.
Language=Romanian
Există un avertisment referitor la subsistemul WX86.
.

MessageId=540
Severity=Success
Facility=System
SymbolicName=ERROR_WX86_ERROR
Language=English
An error occurred in the WX86 subsystem.
.
Language=Russian
An error occurred in the WX86 subsystem.
.
Language=Polish
Wystąpił błąd w podsystemie WX86.
.
Language=Romanian
În subsistemul WX86 a apărut o eroare.
.

MessageId=541
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_NOT_CANCELED
Language=English
An attempt was made to cancel or set a timer that has an associated APC and the subject thread is not the thread that originally set the timer with an associated APC routine.
.
Language=Russian
An attempt was made to cancel or set a timer that has an associated APC and the subject thread is not the thread that originally set the timer with an associated APC routine.
.
Language=Polish
Podjęto próbę anulowania lub ustawienia czasomierza ze związaną procedurą APC, a aktualny wątek nie jest tym wątkiem, który pierwotnie ustawił czasomierz ze związaną procedurą APC.
.
Language=Romanian
A avut loc o încercare de a institui sau destitui un cronometru cu un APC asociat, însă firul de execuție nu este cel care a creat cronometrul cu APC asociat.
.

MessageId=542
Severity=Success
Facility=System
SymbolicName=ERROR_UNWIND
Language=English
Unwind exception code.
.
Language=Russian
Unwind exception code.
.
Language=Polish
Kod wyjątku operacji unwind.
.
Language=Romanian
Eșec la deducția codului de eroare.
.

MessageId=543
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_STACK
Language=English
An invalid or unaligned stack was encountered during an unwind operation.
.
Language=Russian
An invalid or unaligned stack was encountered during an unwind operation.
.
Language=Polish
Podczas operacji unwind napotkano nieprawidłowy lub niewyrównany stos.
.
Language=Romanian
Stivă nealiniată sau nevalidă în operația de deducție a unui cod de eroare.
.

MessageId=544
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_UNWIND_TARGET
Language=English
An invalid unwind target was encountered during an unwind operation.
.
Language=Russian
An invalid unwind target was encountered during an unwind operation.
.
Language=Polish
Podczas operacji unwind napotkano nieprawidłowy obiekt docelowy operacji.
.
Language=Romanian
Destinație nevalidă în operația de deducție a unui cod de eroare.
.

MessageId=545
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PORT_ATTRIBUTES
Language=English
Invalid Object Attributes specified to NtCreatePort or invalid Port Attributes specified to NtConnectPort
.
Language=Russian
Invalid Object Attributes specified to NtCreatePort or invalid Port Attributes specified to NtConnectPort
.
Language=Polish
Określono nieprawidłowe atrybuty obiektu dla NtCreatePort albo nieprawidłowe atrybuty portu dla NtConnectPort
.
Language=Romanian
Atribute nevalide de obiect sau de port specificate către NtCreatePort sau respectiv NtConnectPort.
.

MessageId=546
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_MESSAGE_TOO_LONG
Language=English
Length of message passed to NtRequestPort or NtRequestWaitReplyPort was longer than the maximum message allowed by the port.
.
Language=Russian
Length of message passed to NtRequestPort or NtRequestWaitReplyPort was longer than the maximum message allowed by the port.
.
Language=Polish
Długość komunikatu przekazanego dla NtRequestPort lub NtRequestWaitReplyPort była większa niż maksimum dozwolone dla tego portu.
.
Language=Romanian
Lungimea mesajului specificat în NtRequestPort sau NtRequestWaitReplyPort depășește limita maximă permisă de port.
.

MessageId=547
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_QUOTA_LOWER
Language=English
An attempt was made to lower a quota limit below the current usage.
.
Language=Russian
An attempt was made to lower a quota limit below the current usage.
.
Language=Polish
Podjęto próbę obniżenia ograniczeń zasobów poniżej aktualnego wykorzystania.
.
Language=Romanian
A avut loc o încercare de a scădea limita cotei sub cea utilizată.
.

MessageId=548
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ALREADY_ATTACHED
Language=English
An attempt was made to attach to a device that was already attached to another device.
.
Language=Russian
An attempt was made to attach to a device that was already attached to another device.
.
Language=Polish
Podjęto próbę dołączenia do urządzenia, które było już dołączone do innego urządzenia.
.
Language=Romanian
A avut loc o încercare de a atașa un dispozitiv deja atașat de un alt dispozitiv.
.

MessageId=549
Severity=Success
Facility=System
SymbolicName=ERROR_INSTRUCTION_MISALIGNMENT
Language=English
An attempt was made to execute an instruction at an unaligned address and the host system does not support unaligned instruction references.
.
Language=Russian
An attempt was made to execute an instruction at an unaligned address and the host system does not support unaligned instruction references.
.
Language=Polish
Podjęto próbę wykonania instrukcji pod niewyrównanym adresem, a system hosta nie obsługuje odwołań do niewyrównanych instrukcji.
.
Language=Romanian
A avut loc o încercare de a executa o instrucțiune de la o adresă nealiniată dar sistemul gazdă nu permite referințe la instrucțiuni nealiniate.
.

MessageId=550
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_NOT_STARTED
Language=English
Profiling not started.
.
Language=Russian
Profiling not started.
.
Language=Polish
Nie rozpoczęto profilowania.
.
Language=Romanian
Profilarea nu este pornită.
.

MessageId=551
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_NOT_STOPPED
Language=English
Profiling not stopped.
.
Language=Russian
Profiling not stopped.
.
Language=Polish
Nie zatrzymano profilowania.
.
Language=Romanian
Profiling not stopped.
.

MessageId=552
Severity=Success
Facility=System
SymbolicName=ERROR_COULD_NOT_INTERPRET
Language=English
The passed ACL did not contain the minimum required information.
.
Language=Russian
The passed ACL did not contain the minimum required information.
.
Language=Polish
Przekazane ACL nie zawierało wymaganego minimum informacji.
.
Language=Romanian
The passed ACL did not contain the minimum required information.
.

MessageId=553
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILING_AT_LIMIT
Language=English
The number of active profiling objects is at the maximum and no more may be started.
.
Language=Russian
The number of active profiling objects is at the maximum and no more may be started.
.
Language=Polish
Liczba aktywnych obiektów profilowania jest maksymalna i nie można uruchomić ich więcej.
.
Language=Romanian
The number of active profiling objects is at the maximum and no more may be started.
.

MessageId=554
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_WAIT
Language=English
Used to indicate that an operation cannot continue without blocking for I/O.
.
Language=Russian
Used to indicate that an operation cannot continue without blocking for I/O.
.
Language=Polish
Wskazuje, że kontynuowanie operacji nie jest możliwe bez zablokowania jej dla We/Wy.
.
Language=Romanian
Used to indicate that an operation cannot continue without blocking for I/O.
.

MessageId=555
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_TERMINATE_SELF
Language=English
Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process.
.
Language=Russian
Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process.
.
Language=Polish
Wskazuje, że wątek próbował sam się zakończyć w domyślny sposób (tzw. NtTerminateThread z wartością NULL) i że był to ostatni wątek bieżącego procesu.
.
Language=Romanian
Indicates that a thread attempted to terminate itself by default (called NtTerminateThread with NULL) and it was the last thread in the current process.
.

MessageId=556
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_CREATE_ERR
Language=English
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Russian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Polish
Jeśli zwracany jest błąd MM, który nie jest zdefiniowany w standardowym filtrze FsRtl, to błąd jest konwertowany na jeden z poniższych błędów, który na pewno występuje w filtrze. W tym wypadku informacje ulegają utracie, ale filtr właściwie obsługuje dany wyjątek.
.
Language=Romanian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=557
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_MAP_ERROR
Language=English
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Russian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Polish
Jeśli zwracany jest błąd MM, który nie jest zdefiniowany w standardowym filtrze FsRtl, to błąd jest konwertowany na jeden z poniższych błędów, który na pewno występuje w filtrze. W tym wypadku informacje ulegają utracie, ale filtr właściwie obsługuje dany wyjątek.
.
Language=Romanian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=558
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_MM_EXTEND_ERR
Language=English
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Russian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.
Language=Polish
Jeśli zwracany jest błąd MM, który nie jest zdefiniowany w standardowym filtrze FsRtl, to błąd jest konwertowany na jeden z poniższych błędów, który na pewno występuje w filtrze. W tym wypadku informacje ulegają utracie, ale filtr właściwie obsługuje dany wyjątek.
.
Language=Romanian
If an MM error is returned which is not defined in the standard FsRtl filter, it is converted to one of the following errors which is guaranteed to be in the filter. In this case information is lost, however, the filter correctly handles the exception.
.

MessageId=559
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_FUNCTION_TABLE
Language=English
A malformed function table was encountered during an unwind operation.
.
Language=Russian
A malformed function table was encountered during an unwind operation.
.
Language=Polish
Podczas operacji unwind napotkano wadliwie sformułowaną tabelę funkcji.
.
Language=Romanian
A malformed function table was encountered during an unwind operation.
.

MessageId=560
Severity=Success
Facility=System
SymbolicName=ERROR_NO_GUID_TRANSLATION
Language=English
Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail.
.
Language=Russian
Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail.
.
Language=Polish
Wskazuje, że podjęto próbę przypisania zabezpieczenia dla pliku lub katalogu systemu plików. Przetłumaczenie jednego z identyfikatorów SID w deskryptorze zabezpieczeń na identyfikator GUID, który mógłby być przechowywany w systemie plików nie było możliwe. Powoduje to, że próba zabezpieczenia nie może się powieść i w konsekwencji nie może się powieść próba utworzenia pliku.
.
Language=Romanian
Indicates that an attempt was made to assign protection to a file system file or directory and one of the SIDs in the security descriptor could not be translated into a GUID that could be stored by the file system. This causes the protection attempt to fail, which may cause a file creation attempt to fail.
.

MessageId=561
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_SIZE
Language=English
Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors.
.
Language=Russian
Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors.
.
Language=Polish
Wskazuje, że podjęto próbę powiększenia LDT przez ustawienie rozmiaru, albo że rozmiar nie odpowiadał parzystej liczbie selektorów.
.
Language=Romanian
Indicates that an attempt was made to grow an LDT by setting its size, or that the size was not an even number of selectors.
.

MessageId=563
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_OFFSET
Language=English
Indicates that the starting value for the LDT information was not an integral multiple of the selector size.
.
Language=Russian
Indicates that the starting value for the LDT information was not an integral multiple of the selector size.
.
Language=Polish
Wskazuje, że początkowa wartość dla informacji LDT nie była całkowitą wielokrotnością rozmiaru selektora.
.
Language=Romanian
Indicates that the starting value for the LDT information was not an integral multiple of the selector size.
.

MessageId=564
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LDT_DESCRIPTOR
Language=English
Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors.
.
Language=Russian
Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors.
.
Language=Polish
Wskazuje, że użytkownik podał nieprawidłowy deskryptor próbując ustawić deskryptory LDT.
.
Language=Romanian
Indicates that the user supplied an invalid descriptor when trying to set up Ldt descriptors.
.

MessageId=565
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_THREADS
Language=English
Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads.
.
Language=Russian
Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads.
.
Language=Polish
Wskazuje, że proces ma zbyt wiele wątków, aby wykonać żądaną akcję. Na przykład przypisanie podstawowego tokenu jest możliwe do wykonania tylko wtedy, gdy dany proces nie ma wątków lub ma jeden wątek.
.
Language=Romanian
Indicates a process has too many threads to perform the requested action. For example, assignment of a primary token may only be performed when a process has zero or one threads.
.

MessageId=566
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_NOT_IN_PROCESS
Language=English
An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.
.
Language=Russian
An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.
.
Language=Polish
Podjęto próbę operowania na wątku w pewnym procesie, ale określony wątek nie znajduje się w określonym procesie.
.
Language=Romanian
An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.
.

MessageId=567
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_QUOTA_EXCEEDED
Language=English
Page file quota was exceeded.
.
Language=Russian
Page file quota was exceeded.
.
Language=Polish
Przekroczono ograniczenie rozmiaru pliku stronicowania.
.
Language=Romanian
Page file quota was exceeded.
.

MessageId=568
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SERVER_CONFLICT
Language=English
The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role.
.
Language=Russian
The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role.
.
Language=Polish
Uruchomienie tej usługi Netlogon nie było możliwe, ponieważ inna usługa Netlogon działająca w tej domenie wchodzi w konflikt z określoną funkcją.
.
Language=Romanian
The Netlogon service cannot start because another Netlogon service running in the domain conflicts with the specified role.
.

MessageId=569
Severity=Success
Facility=System
SymbolicName=ERROR_SYNCHRONIZATION_REQUIRED
Language=English
The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required.
.
Language=Russian
The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required.
.
Language=Polish
Baza danych SAM na serwerze Windows jest znacząco rozsynchronizowana z kopią znajdującą się na kontrolerze domeny. Wymagana jest pełna synchronizacja.
.
Language=Romanian
The SAM database on a Windows Server is significantly out of synchronization with the copy on the Domain Controller. A complete synchronization is required.
.

MessageId=570
Severity=Success
Facility=System
SymbolicName=ERROR_NET_OPEN_FAILED
Language=English
The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines.
.
Language=Russian
The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines.
.
Language=Polish
Wykonanie funkcji API o nazwie NtCreateFile nie powiodło się. Ten błąd nie powinien być nigdy zwracany do aplikacji, jest używany przez program Windows Lan Manager Redirector w procedurach mapowania błędów wewnętrznych.
.
Language=Romanian
The NtCreateFile API failed. This error should never be returned to an application, it is a place holder for the Windows Lan Manager Redirector to use in its internal error mapping routines.
.

MessageId=571
Severity=Success
Facility=System
SymbolicName=ERROR_IO_PRIVILEGE_FAILED
Language=English
The I/O permissions for the process could not be changed.
.
Language=Russian
The I/O permissions for the process could not be changed.
.
Language=Polish
Nie udało się zmienić uprawnień We/Wy dla tego procesu.
.
Language=Romanian
The I/O permissions for the process could not be changed.
.

MessageId=572
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROL_C_EXIT
Language=English
The application terminated as a result of a CTRL+C.
.
Language=Russian
The application terminated as a result of a CTRL+C.
.
Language=Polish
Aplikacja została zakończona w wyniku naciśnięcia klawiszy CTRL+C.
.
Language=Romanian
The application terminated as a result of a CTRL+C.
.

MessageId=573
Severity=Success
Facility=System
SymbolicName=ERROR_MISSING_SYSTEMFILE
Language=English
The required system file %hs is bad or missing.
.
Language=Russian
The required system file %hs is bad or missing.
.
Language=Polish
Wymagany plik systemowy %hs jest zły albo brak go.
.
Language=Romanian
The required system file %hs is bad or missing.
.

MessageId=574
Severity=Success
Facility=System
SymbolicName=ERROR_UNHANDLED_EXCEPTION
Language=English
The exception %s (0x%08lx) occurred in the application at location 0x%08lx.
.
Language=Russian
The exception %s (0x%08lx) occurred in the application at location 0x%08lx.
.
Language=Polish
W aplikacji wystąpił wyjątek %s (0x%08lx) na pozycji 0x%08lx.
.
Language=Romanian
The exception %s (0x%08lx) occurred in the application at location 0x%08lx.
.

MessageId=575
Severity=Success
Facility=System
SymbolicName=ERROR_APP_INIT_FAILURE
Language=English
The application failed to initialize properly (0x%lx). Click on OK to terminate the application.
.
Language=Russian
The application failed to initialize properly (0x%lx). Click on OK to terminate the application.
.
Language=Polish
Aplikacja nie została właściwie uruchomiona (0x%lx). Kliknij przycisk OK, aby zakończyć aplikację.
.
Language=Romanian
The application failed to initialize properly (0x%lx). Click on OK to terminate the application.
.

MessageId=576
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_CREATE_FAILED
Language=English
The creation of the paging file %hs failed (%lx). The requested size was %ld.
.
Language=Russian
The creation of the paging file %hs failed (%lx). The requested size was %ld.
.
Language=Polish
Utworzenie pliku stronicowania %hs nie powiodło się (%lx). Żądanym rozmiarem było %ld.
.
Language=Romanian
The creation of the paging file %hs failed (%lx). The requested size was %ld.
.

MessageId=578
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PAGEFILE
Language=English
No paging file was specified in the system configuration.
.
Language=Russian
No paging file was specified in the system configuration.
.
Language=Polish
W konfiguracji systemu nie określono pliku stronicowania.
.
Language=Romanian
No paging file was specified in the system configuration.
.

MessageId=579
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_FLOAT_CONTEXT
Language=English
A real-mode application issued a floating-point instruction and floating-point hardware is not present.
.
Language=Russian
A real-mode application issued a floating-point instruction and floating-point hardware is not present.
.
Language=Polish
Aplikacja trybu rzeczywistego wydała instrukcję zmiennoprzecinkową, tymczasem nie ma urządzenia zmiennoprzecinkowego.
.
Language=Romanian
A real-mode application issued a floating-point instruction and floating-point hardware is not present.
.

MessageId=580
Severity=Success
Facility=System
SymbolicName=ERROR_NO_EVENT_PAIR
Language=English
An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread.
.
Language=Russian
An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread.
.
Language=Polish
Wykonano operację synchronizacji pary zdarzeń używając właściwego dla wątku obiektu pary zdarzeń klient/serwer, ale żaden obiekt pary zdarzeń nie był związany z tym wątkiem.
.
Language=Romanian
An event pair synchronization operation was performed using the thread specific client/server event pair object, but no event pair object was associated with the thread.
.

MessageId=581
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CTRLR_CONFIG_ERROR
Language=English
A Windows Server has an incorrect configuration.
.
Language=Russian
A Windows Server has an incorrect configuration.
.
Language=Polish
Serwer Windows ma niepoprawną konfigurację.
.
Language=Romanian
A Windows Server has an incorrect configuration.
.

MessageId=582
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_CHARACTER
Language=English
An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE.
.
Language=Russian
An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE.
.
Language=Polish
Napotkano niedozwolony znak. W przypadku wielobajtowego zestawu znaków dotyczy to także wiodącego bajtu, po którym nie występuje żaden bajt pomocniczy. W przypadku zestawu znaków Unicode obejmuje to 0xFFFF i 0xFFFE.
.
Language=Romanian
An illegal character was encountered. For a multi-byte character set this includes a lead byte without a succeeding trail byte. For the Unicode character set this includes the characters 0xFFFF and 0xFFFE.
.

MessageId=583
Severity=Success
Facility=System
SymbolicName=ERROR_UNDEFINED_CHARACTER
Language=English
The Unicode character is not defined in the Unicode character set installed on the system.
.
Language=Russian
The Unicode character is not defined in the Unicode character set installed on the system.
.
Language=Polish
Ten znak Unicode nie jest zdefiniowany w zestawie znaków Unicode zainstalowanym w tym systemie.
.
Language=Romanian
The Unicode character is not defined in the Unicode character set installed on the system.
.

MessageId=584
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_VOLUME
Language=English
The paging file cannot be created on a floppy diskette.
.
Language=Russian
The paging file cannot be created on a floppy diskette.
.
Language=Polish
Nie można utworzyć pliku stronicowania na dyskietce.
.
Language=Romanian
The paging file cannot be created on a floppy diskette.
.

MessageId=585
Severity=Success
Facility=System
SymbolicName=ERROR_BIOS_FAILED_TO_CONNECT_INTERRUPT
Language=English
The system bios failed to connect a system interrupt to the device or bus for which the device is connected.
.
Language=Russian
The system bios failed to connect a system interrupt to the device or bus for which the device is connected.
.
Language=Polish
System BIOS nie połączył przerwania systemowego z urządzeniem lub magistralą, do której urządzenie jest podłączone.
.
Language=Romanian
The system bios failed to connect a system interrupt to the device or bus for which the device is connected.
.

MessageId=586
Severity=Success
Facility=System
SymbolicName=ERROR_BACKUP_CONTROLLER
Language=English
This operation is only allowed for the Primary Domain Controller of the domain.
.
Language=Russian
This operation is only allowed for the Primary Domain Controller of the domain.
.
Language=Polish
Ta operacja jest dozwolona tylko dla podstawowego kontrolera domeny.
.
Language=Romanian
This operation is only allowed for the Primary Domain Controller of the domain.
.

MessageId=587
Severity=Success
Facility=System
SymbolicName=ERROR_MUTANT_LIMIT_EXCEEDED
Language=English
An attempt was made to acquire a mutant such that its maximum count would have been exceeded.
.
Language=Russian
An attempt was made to acquire a mutant such that its maximum count would have been exceeded.
.
Language=Polish
Podjęto próbę uzyskania zmienionego obiektu w taki sposób, że zostałaby przekroczona maksymalna liczba takich obiektów.
.
Language=Romanian
An attempt was made to acquire a mutant such that its maximum count would have been exceeded.
.

MessageId=588
Severity=Success
Facility=System
SymbolicName=ERROR_FS_DRIVER_REQUIRED
Language=English
A volume has been accessed for which a file system driver is required that has not yet been loaded.
.
Language=Russian
A volume has been accessed for which a file system driver is required that has not yet been loaded.
.
Language=Polish
Uzyskano dostęp do woluminu, dla którego wymagany jest jeszcze nie załadowany sterownik systemu plików.
.
Language=Romanian
A volume has been accessed for which a file system driver is required that has not yet been loaded.
.

MessageId=589
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_LOAD_REGISTRY_FILE
Language=English
The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable.
.
Language=Russian
The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable.
.
Language=Polish
Nie jest możliwe załadowanie przez Rejestr gałęzi (pliku): %hs lub jego dziennika bądź drugiej kopii. Jest on uszkodzony, brak go lub jest nie do zapisania.
.
Language=Romanian
The registry cannot load the hive (file): %hs or its log or alternate. It is corrupt, absent, or not writable.
.

MessageId=590
Severity=Success
Facility=System
SymbolicName=ERROR_DEBUG_ATTACH_FAILED
Language=English
An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error.
.
Language=Russian
An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error.
.
Language=Polish
Niespodziewanie nie powiodło się przetwarzanie żądania procesu API DebugActiveProcess. Możesz wybrać "OK", aby zakończyć ten proces, albo "Anuluj", aby zignorować błąd.
.
Language=Romanian
An unexpected failure occurred while processing a DebugActiveProcess API request. You may choose OK to terminate the process, or Cancel to ignore the error.
.

MessageId=591
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_PROCESS_TERMINATED
Language=English
The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down.
.
Language=Russian
The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down.
.
Language=Polish
Proces systemowy %hs zakończył się niespodziewanie ze stanem 0x%08x (0x%08x 0x%08x). System zostanie zamknięty.
.
Language=Romanian
The %hs system process terminated unexpectedly with a status of 0x%08x (0x%08x 0x%08x). The system has been shut down.
.

MessageId=592
Severity=Success
Facility=System
SymbolicName=ERROR_DATA_NOT_ACCEPTED
Language=English
The TDI client could not handle the data received during an indication.
.
Language=Russian
The TDI client could not handle the data received during an indication.
.
Language=Polish
Obsługa danych otrzymanych przez klienta TDI podczas wskazywania nie jest możliwa.
.
Language=Romanian
The TDI client could not handle the data received during an indication.
.

MessageId=593
Severity=Success
Facility=System
SymbolicName=ERROR_VDM_HARD_ERROR
Language=English
NTVDM encountered a hard error.
.
Language=Russian
NTVDM encountered a hard error.
.
Language=Polish
NTVDM napotkał poważny błąd.
.
Language=Romanian
NTVDM encountered a hard error.
.

MessageId=594
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_CANCEL_TIMEOUT
Language=English
The driver %hs failed to complete a cancelled I/O request in the allotted time.
.
Language=Russian
The driver %hs failed to complete a cancelled I/O request in the allotted time.
.
Language=Polish
Sterownik %hs nie zdołał zakończyć anulowanego żądania We/Wy w przypisanym czasie.
.
Language=Romanian
The driver %hs failed to complete a cancelled I/O request in the allotted time.
.

MessageId=595
Severity=Success
Facility=System
SymbolicName=ERROR_REPLY_MESSAGE_MISMATCH
Language=English
An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message.
.
Language=Russian
An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message.
.
Language=Polish
Próbowano odpowiedzieć na komunikat LPC, ale wątek określony przez identyfikator klienta w komunikacie nie oczekiwał na tę odpowiedź.
.
Language=Romanian
An attempt was made to reply to an LPC message, but the thread specified by the client ID in the message was not waiting on that message.
.

MessageId=596
Severity=Success
Facility=System
SymbolicName=ERROR_LOST_WRITEBEHIND_DATA
Language=English
ReactOS was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.
.
Language=Russian
ReactOS was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.
.
Language=Polish
System ReactOS nie mógł zapisać wszystkich danych dla pliku %hs. Dane zostały utracone. Powodem tego błędu mogła być awaria sprzętu komputerowego lub połączenia sieciowego. Spróbuj zapisać ten plik w innym miejscu.
.
Language=Romanian
ReactOS was unable to save all the data for the file %hs. The data has been lost. This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.
.

MessageId=597
Severity=Success
Facility=System
SymbolicName=ERROR_CLIENT_SERVER_PARAMETERS_INVALID
Language=English
The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window.
.
Language=Russian
The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window.
.
Language=Polish
Parametry przekazane do serwera we wspólnym oknie pamięci klient/serwer były nieprawidłowe. Zbyt wiele danych zostało umieszczonych we wspólnym oknie pamięci.
.
Language=Romanian
The parameter(s) passed to the server in the client/server shared memory window were invalid. Too much data may have been put in the shared memory window.
.

MessageId=598
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_TINY_STREAM
Language=English
The stream is not a tiny stream.
.
Language=Russian
The stream is not a tiny stream.
.
Language=Polish
Ten strumień nie jest mały.
.
Language=Romanian
The stream is not a tiny stream.
.

MessageId=599
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_OVERFLOW_READ
Language=English
The request must be handled by the stack overflow code.
.
Language=Russian
The request must be handled by the stack overflow code.
.
Language=Polish
To żądanie musi być obsługiwane przez kod przepełnienia stosu.
.
Language=Romanian
The request must be handled by the stack overflow code.
.

MessageId=600
Severity=Success
Facility=System
SymbolicName=ERROR_CONVERT_TO_LARGE
Language=English
Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream.
.
Language=Russian
Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream.
.
Language=Polish
Wewnętrzne kody stanu OFS wskazują, jak jest obsługiwana operacja alokacji. Próba jest ponawiana po przeniesieniu węzła zawierającego onode, albo istniejący strumień jest konwertowany na duży potok.
.
Language=Romanian
Internal OFS status codes indicating how an allocation operation is handled. Either it is retried after the containing onode is moved or the extent stream is converted to a large stream.
.

MessageId=601
Severity=Success
Facility=System
SymbolicName=ERROR_FOUND_OUT_OF_SCOPE
Language=English
The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation.
.
Language=Russian
The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation.
.
Language=Polish
Próba znalezienia obiektu dała w efekcie obiekt na tym woluminie, mający odpowiedni identyfikator, ale znajdujący się poza zakresem dojścia użytego dla tej operacji.
.
Language=Romanian
The attempt to find the object found an object matching by ID on the volume but it is out of the scope of the handle used for the operation.
.

MessageId=602
Severity=Success
Facility=System
SymbolicName=ERROR_ALLOCATE_BUCKET
Language=English
The bucket array must be grown. Retry transaction after doing so.
.
Language=Russian
The bucket array must be grown. Retry transaction after doing so.
.
Language=Polish
Należy powiększyć tablicę typu "bucket array", a następnie spróbować ponownie wykonać transakcję.
.
Language=Romanian
The bucket array must be grown. Retry transaction after doing so.
.

MessageId=603
Severity=Success
Facility=System
SymbolicName=ERROR_MARSHALL_OVERFLOW
Language=English
The user/kernel marshalling buffer has overflowed.
.
Language=Russian
The user/kernel marshalling buffer has overflowed.
.
Language=Polish
Bufor kierujący użytkownika/jądra został przepełniony.
.
Language=Romanian
The user/kernel marshalling buffer has overflowed.
.

MessageId=604
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_VARIANT
Language=English
The supplied variant structure contains invalid data.
.
Language=Russian
The supplied variant structure contains invalid data.
.
Language=Polish
Podana struktura wariantu zawiera nieprawidłowe dane.
.
Language=Romanian
The supplied variant structure contains invalid data.
.

MessageId=605
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_COMPRESSION_BUFFER
Language=English
The specified buffer contains ill-formed data.
.
Language=Russian
The specified buffer contains ill-formed data.
.
Language=Polish
Określony bufor zawiera źle sformułowane dane.
.
Language=Romanian
The specified buffer contains ill-formed data.
.

MessageId=606
Severity=Success
Facility=System
SymbolicName=ERROR_AUDIT_FAILED
Language=English
An attempt to generate a security audit failed.
.
Language=Russian
An attempt to generate a security audit failed.
.
Language=Polish
Próba wygenerowania inspekcji zabezpieczeń nie powiodła się.
.
Language=Romanian
An attempt to generate a security audit failed.
.

MessageId=607
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_RESOLUTION_NOT_SET
Language=English
The timer resolution was not previously set by the current process.
.
Language=Russian
The timer resolution was not previously set by the current process.
.
Language=Polish
Rozdzielczość czasomierza nie została wcześniej ustawiona przez bieżący proces.
.
Language=Romanian
The timer resolution was not previously set by the current process.
.

MessageId=608
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_LOGON_INFO
Language=English
There is insufficient account information to log you on.
.
Language=Russian
There is insufficient account information to log you on.
.
Language=Polish
Nie podano informacji dotyczących konta niezbędnych do zalogowania.
.
Language=Romanian
There is insufficient account information to log you on.
.

MessageId=609
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DLL_ENTRYPOINT
Language=English
The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly.
.
Language=Russian
The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly.
.
Language=Polish
Biblioteka dołączana dynamicznie %hs nie jest napisana poprawnie. Wskaźnik stosu pozostawiono w niespójnym stanie. Punkt wejścia powinien być zadeklarowany jako WINAPI lub STDCALL. Wybierz "TAK", aby zrezygnować z ładowania DLL. Wybierz "NIE", aby kontynuować wykonywanie DLL. Wybór "NIE" może spowodować niewłaściwe działanie aplikacji.
.
Language=Romanian
The dynamic link library %hs is not written correctly. The stack pointer has been left in an inconsistent state. The entrypoint should be declared as WINAPI or STDCALL. Select YES to fail the DLL load. Select NO to continue execution. Selecting NO may cause the application to operate incorrectly.
.

MessageId=610
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_SERVICE_ENTRYPOINT
Language=English
The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly.
.
Language=Russian
The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly.
.
Language=Polish
Usługa %hs nie jest napisana poprawnie. Wskaźnik stosu pozostawiono w niespójnym stanie. Punkt wejścia wywołania zwrotnego usługi powinien być zadeklarowany jako WINAPI lub STDCALL. Wybierz "OK", aby kontynuować operacje usługi. Proces usługi może jednak działać niewłaściwie.
.
Language=Romanian
The %hs service is not written correctly. The stack pointer has been left in an inconsistent state. The callback entrypoint should be declared as WINAPI or STDCALL. Selecting OK will cause the service to continue operation. However, the service process may operate incorrectly.
.

MessageId=611
Severity=Success
Facility=System
SymbolicName=ERROR_IP_ADDRESS_CONFLICT1
Language=English
There is an IP address conflict with another system on the network
.
Language=Russian
There is an IP address conflict with another system on the network
.
Language=Polish
Istnieje konflikt adresów IP z innym systemem w sieci.
.
Language=Romanian
There is an IP address conflict with another system on the network
.

MessageId=612
Severity=Success
Facility=System
SymbolicName=ERROR_IP_ADDRESS_CONFLICT2
Language=English
There is an IP address conflict with another system on the network
.
Language=Russian
There is an IP address conflict with another system on the network
.
Language=Polish
Istnieje konflikt adresów IP z innym systemem w sieci.
.
Language=Romanian
There is an IP address conflict with another system on the network
.

MessageId=613
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_QUOTA_LIMIT
Language=English
The system has reached the maximum size allowed for the system part of the registry. Additional storage requests will be ignored.
.
Language=Russian
The system has reached the maximum size allowed for the system part of the registry. Additional storage requests will be ignored.
.
Language=Polish
System osiągnął maksymalny rozmiar dozwolony dla części systemowej rejestru. Dodatkowe żądania przydziału miejsca będą ignorowane.
.
Language=Romanian
The system has reached the maximum size allowed for the system part of the registry. Additional storage requests will be ignored.
.

MessageId=614
Severity=Success
Facility=System
SymbolicName=ERROR_NO_CALLBACK_ACTIVE
Language=English
A callback return system service cannot be executed when no callback is active.
.
Language=Russian
A callback return system service cannot be executed when no callback is active.
.
Language=Polish
Nie można wykonać usługi wywołania zwrotnego, gdy żadne wywołanie zwrotne nie jest aktywne.
.
Language=Romanian
A callback return system service cannot be executed when no callback is active.
.

MessageId=615
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_TOO_SHORT
Language=English
The password provided is too short to meet the policy of your user account. Please choose a longer password.
.
Language=Russian
The password provided is too short to meet the policy of your user account. Please choose a longer password.
.
Language=Polish
Podane hasło jest zbyt krótkie, co jest niezgodne z zasadami wykorzystywania tego konta. Wybierz dłuższe hasło.
.
Language=Romanian
The password provided is too short to meet the policy of your user account. Please choose a longer password.
.

MessageId=616
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_TOO_RECENT
Language=English
The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned.
.
Language=Russian
The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned.
.
Language=Polish
Zasady wykorzystywania tego konta użytkownika nie pozwalają na zbyt częste zmiany hasła. Ma to zapobiec zmianom hasła na wyrażenie łatwe do zapamiętania, ale i do złamania. Jeśli jednak podejrzewasz, że złamano twoje hasło, skontaktuj się niezwłocznie z administratorem, aby otrzymać nowe.
.
Language=Romanian
The policy of your user account does not allow you to change passwords too frequently. This is done to prevent users from changing back to a familiar, but potentially discovered, password. If you feel your password has been compromised then please contact your administrator immediately to have a new one assigned.
.

MessageId=617
Severity=Success
Facility=System
SymbolicName=ERROR_PWD_HISTORY_CONFLICT
Language=English
You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used.
.
Language=Russian
You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used.
.
Language=Polish
Próbowano zmienić hasło na takie, które było już używane w przeszłości. Jest to niezgodne z zasadami wykorzystywania tego konta. Wybierz inne hasło.
.
Language=Romanian
You have attempted to change your password to one that you have used in the past. The policy of your user account does not allow this. Please select a password that you have not previously used.
.

MessageId=618
Severity=Success
Facility=System
SymbolicName=ERROR_UNSUPPORTED_COMPRESSION
Language=English
The specified compression format is unsupported.
.
Language=Russian
The specified compression format is unsupported.
.
Language=Polish
Określony format kompresji nie jest obsługiwany.
.
Language=Romanian
The specified compression format is unsupported.
.

MessageId=619
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HW_PROFILE
Language=English
The specified hardware profile configuration is invalid.
.
Language=Russian
The specified hardware profile configuration is invalid.
.
Language=Polish
Określona konfiguracja profilu sprzętu jest nieprawidłowa.
.
Language=Romanian
The specified hardware profile configuration is invalid.
.

MessageId=620
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PLUGPLAY_DEVICE_PATH
Language=English
The specified Plug and Play registry device path is invalid.
.
Language=Russian
The specified Plug and Play registry device path is invalid.
.
Language=Polish
Określona ścieżka Rejestru urządzenia typu „Plug and Play” jest nieprawidłowa.
.
Language=Romanian
The specified Plug and Play registry device path is invalid.
.

MessageId=621
Severity=Success
Facility=System
SymbolicName=ERROR_QUOTA_LIST_INCONSISTENT
Language=English
The specified quota list is internally inconsistent with its descriptor.
.
Language=Russian
The specified quota list is internally inconsistent with its descriptor.
.
Language=Polish
Określona lista zasobów jest wewnętrznie niezgodna ze swoim deskryptorem.
.
Language=Romanian
The specified quota list is internally inconsistent with its descriptor.
.

MessageId=622
Severity=Success
Facility=System
SymbolicName=ERROR_EVALUATION_EXPIRATION
Language=English
The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product.
.
Language=Russian
The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product.
.
Language=Polish
Okres tymczasowego użytkowania tej instalacji systemu Windows wygasł. System zostanie zamknięty za godzinę. Aby przywrócić dostęp do tej instalacji systemu Windows, uaktualnij instalację używając licencjonowanego produktu znajdującego się w dystrybucji.
.
Language=Romanian
The evaluation period for this installation of Windows has expired. This system will shutdown in 1 hour. To restore access to this installation of Windows, please upgrade this installation using a licensed distribution of this product.
.

MessageId=623
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_DLL_RELOCATION
Language=English
The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for ReactOS system DLLs. The vendor supplying the DLL should be contacted for a new DLL.
.
Language=Russian
The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for ReactOS system DLLs. The vendor supplying the DLL should be contacted for a new DLL.
.
Language=Polish
Biblioteka systemowa DLL %hs została zrelokowana w pamięci. Aplikacja nie będzie działać prawidłowo. Powodem relokacji było to, że biblioteka DLL %hs zajmowała zakres adresów zarezerwowany dla ReactOS systemowej biblioteki DLL. Należy skontaktować się z dostawcą w sprawie nowej biblioteki DLL.
.
Language=Romanian
The system DLL %hs was relocated in memory. The application will not run properly. The relocation occurred because the DLL %hs occupied an address range reserved for ReactOS system DLLs. The vendor supplying the DLL should be contacted for a new DLL.
.

MessageId=624
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_INIT_FAILED_LOGOFF
Language=English
The application failed to initialize because the window station is shutting down.
.
Language=Russian
The application failed to initialize because the window station is shutting down.
.
Language=Polish
Zainicjowanie aplikacji nie udało się, gdyż stacja jest właśnie zamykana.
.
Language=Romanian
The application failed to initialize because the window station is shutting down.
.

MessageId=625
Severity=Success
Facility=System
SymbolicName=ERROR_VALIDATE_CONTINUE
Language=English
The validation process needs to continue on to the next step.
.
Language=Russian
The validation process needs to continue on to the next step.
.
Language=Polish
Proces sprawdzania poprawności musi być kontynuowany do następnego kroku.
.
Language=Romanian
The validation process needs to continue on to the next step.
.

MessageId=626
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_MATCHES
Language=English
There are no more matches for the current index enumeration.
.
Language=Russian
There are no more matches for the current index enumeration.
.
Language=Polish
Nie ma więcej odpowiedników dla bieżącego wyliczania indeksu.
.
Language=Romanian
There are no more matches for the current index enumeration.
.

MessageId=627
Severity=Success
Facility=System
SymbolicName=ERROR_RANGE_LIST_CONFLICT
Language=English
The range could not be added to the range list because of a conflict.
.
Language=Russian
The range could not be added to the range list because of a conflict.
.
Language=Polish
Z powodu konfliktu nie można dodać zakresu do listy zakresów.
.
Language=Romanian
The range could not be added to the range list because of a conflict.
.

MessageId=628
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_SID_MISMATCH
Language=English
The server process is running under a SID different than that required by client.
.
Language=Russian
The server process is running under a SID different than that required by client.
.
Language=Polish
Proces serwera działa z innym identyfikatorem SID niż wymaga to klient.
.
Language=Romanian
The server process is running under a SID different than that required by client.
.

MessageId=629
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ENABLE_DENY_ONLY
Language=English
A group marked use for deny only cannot be enabled.
.
Language=Russian
A group marked use for deny only cannot be enabled.
.
Language=Polish
Nie można włączyć grupy oznaczonej do użycia tylko dla odmowy.
.
Language=Romanian
A group marked use for deny only cannot be enabled.
.

MessageId=630
Severity=Success
Facility=System
SymbolicName=ERROR_FLOAT_MULTIPLE_FAULTS
Language=English
Multiple floating point faults.
.
Language=Russian
Multiple floating point faults.
.
Language=Polish
Wiele błędów zmiennoprzecinkowych.
.
Language=Romanian
Multiple floating point faults.
.

MessageId=631
Severity=Success
Facility=System
SymbolicName=ERROR_FLOAT_MULTIPLE_TRAPS
Language=English
Multiple floating point traps.
.
Language=Russian
Multiple floating point traps.
.
Language=Polish
Wiele pułapek zmiennoprzecinkowych.
.
Language=Romanian
Multiple floating point traps.
.

MessageId=632
Severity=Success
Facility=System
SymbolicName=ERROR_NOINTERFACE
Language=English
The requested interface is not supported.
.
Language=Russian
The requested interface is not supported.
.
Language=Polish
Żądany interfejs nie jest obsługiwany.
.
Language=Romanian
The requested interface is not supported.
.

MessageId=633
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_FAILED_SLEEP
Language=English
The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode.
.
Language=Russian
The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode.
.
Language=Polish
Sterownik %hs nie obsługuje trybu stanu wstrzymania. Aby system mógł się znaleźć w stanie wstrzymania należy zaktualizować sterownik.
.
Language=Romanian
The driver %hs does not support standby mode. Updating this driver may allow the system to go to standby mode.
.

MessageId=634
Severity=Success
Facility=System
SymbolicName=ERROR_CORRUPT_SYSTEM_FILE
Language=English
The system file %1 has become corrupt and has been replaced.
.
Language=Russian
The system file %1 has become corrupt and has been replaced.
.
Language=Polish
Plik systemowy %1 był uszkodzony i został zastąpiony.
.
Language=Romanian
The system file %1 has become corrupt and has been replaced.
.

MessageId=635
Severity=Success
Facility=System
SymbolicName=ERROR_COMMITMENT_MINIMUM
Language=English
Your system is low on virtual memory. ReactOS is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help.
.
Language=Russian
Your system is low on virtual memory. ReactOS is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help.
.
Language=Polish
System ma za mało pamięci wirtualnej. System ReactOS zwiększa rozmiar pliku stronicowania pamięci wirtualnej. W czasie trwania tego procesu, może wystąpić odmowa na żądania pamięci niektórych aplikacji. Więcej informacji możesz znaleźć w Pomocy.
.
Language=Romanian
Your system is low on virtual memory. ReactOS is increasing the size of your virtual memory paging file. During this process, memory requests for some applications may be denied. For more information, see Help.
.

MessageId=636
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_RESTART_ENUMERATION
Language=English
A device was removed so enumeration must be restarted.
.
Language=Russian
A device was removed so enumeration must be restarted.
.
Language=Polish
Urządzenie zostało usunięte, a więc wyliczanie musi zostać uruchomione ponownie.
.
Language=Romanian
A device was removed so enumeration must be restarted.
.

MessageId=637
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_IMAGE_BAD_SIGNATURE
Language=English
The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down.
.
Language=Russian
The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down.
.
Language=Polish
Obraz systemu %s nie jest poprawnie podpisany. Plik został zastąpiony plikiem podpisanym. System został zamknięty.
.
Language=Romanian
The system image %s is not properly signed. The file has been replaced with the signed file. The system has been shut down.
.

MessageId=638
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_REBOOT_REQUIRED
Language=English
Device will not start without a reboot.
.
Language=Russian
Device will not start without a reboot.
.
Language=Polish
Urządzenie nie zostanie uruchomione bez ponownego rozruchu komputera.
.
Language=Romanian
Device will not start without a reboot.
.

MessageId=639
Severity=Success
Facility=System
SymbolicName=ERROR_INSUFFICIENT_POWER
Language=English
There is not enough power to complete the requested operation.
.
Language=Russian
There is not enough power to complete the requested operation.
.
Language=Polish
Za mało energii, aby zakończyć żądaną operację
.
Language=Romanian
There is not enough power to complete the requested operation.
.

MessageId=641
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_SHUTDOWN
Language=English
The system is in the process of shutting down.
.
Language=Russian
The system is in the process of shutting down.
.
Language=Polish
Trwa proces zamykania systemu.
.
Language=Romanian
The system is in the process of shutting down.
.

MessageId=642
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_NOT_SET
Language=English
An attempt to remove a processes DebugPort was made, but a port was not already associated with the process.
.
Language=Russian
An attempt to remove a processes DebugPort was made, but a port was not already associated with the process.
.
Language=Polish
Podjęto próbę usunięcia portu DebugPort, ale żaden port nie był już skojarzony z danym procesem
.
Language=Romanian
An attempt to remove a processes DebugPort was made, but a port was not already associated with the process.
.

MessageId=643
Severity=Success
Facility=System
SymbolicName=ERROR_DS_VERSION_CHECK_FAILURE
Language=English
This version of ReactOS is not compatible with the behavior version of directory forest, domain or domain controller.
.
Language=Russian
This version of ReactOS is not compatible with the behavior version of directory forest, domain or domain controller.
.
Language=Polish
Ta wersja systemu ReactOS jest niezgodna z wersją zachowania lasu katalogów, domeny lub kontrolera domeny.
.
Language=Romanian
This version of ReactOS is not compatible with the behavior version of directory forest, domain or domain controller.
.

MessageId=644
Severity=Success
Facility=System
SymbolicName=ERROR_RANGE_NOT_FOUND
Language=English
The specified range could not be found in the range list.
.
Language=Russian
The specified range could not be found in the range list.
.
Language=Polish
Na liście zakresów nie można znaleźć określonego zakresu.
.
Language=Romanian
The specified range could not be found in the range list.
.

MessageId=646
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAFE_MODE_DRIVER
Language=English
The driver was not loaded because the system is booting into safe mode.
.
Language=Russian
The driver was not loaded because the system is booting into safe mode.
.
Language=Polish
Sterownik nie został załadowany, ponieważ system jest uruchamiany w trybie awaryjnym.
.
Language=Romanian
The driver was not loaded because the system is booting into safe mode.
.

MessageId=647
Severity=Success
Facility=System
SymbolicName=ERROR_FAILED_DRIVER_ENTRY
Language=English
The driver was not loaded because it failed it's initialization call.
.
Language=Russian
The driver was not loaded because it failed it's initialization call.
.
Language=Polish
Sterownik nie został załadowany, ponieważ nie powiodło się wywołanie jego inicjalizacji.
.
Language=Romanian
The driver was not loaded because it failed it's initialization call.
.

MessageId=648
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ENUMERATION_ERROR
Language=English
The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection.
.
Language=Russian
The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection.
.
Language=Polish
	Sterownik "%hs" napotkał błąd, stosując zasilanie lub odczytując konfigurację urządzenia. Przyczyną może być awaria sprzętu lub złe połączenie.
.
Language=Romanian
The \"%hs\" encountered an error while applying power or reading the device configuration. This may be caused by a failure of your hardware or by a poor connection.
.

MessageId=649
Severity=Success
Facility=System
SymbolicName=ERROR_MOUNT_POINT_NOT_RESOLVED
Language=English
The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached.
.
Language=Russian
The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached.
.
Language=Polish
Operacja tworzenia nie powiodła się, ponieważ dana nazwa zawierała przynajmniej jeden punkt instalacji rozpoznawany jako wolumin, do którego określone urządzenie nie jest dołączone.
.
Language=Romanian
The create operation failed because the name contained at least one mount point which resolves to a volume to which the specified device object is not attached.
.

MessageId=650
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DEVICE_OBJECT_PARAMETER
Language=English
The device object parameter is either not a valid device object or is not attached to the volume specified by the file name.
.
Language=Russian
The device object parameter is either not a valid device object or is not attached to the volume specified by the file name.
.
Language=Polish
Parametr obiektu urządzenia określa urządzenie, które albo nie jest prawidłowym obiektem urządzenia, albo nie jest dołączone do woluminu określonego przez nazwę pliku.
.
Language=Romanian
The device object parameter is either not a valid device object or is not attached to the volume specified by the file name.
.

MessageId=651
Severity=Success
Facility=System
SymbolicName=ERROR_MCA_OCCURED
Language=English
A Machine Check Error has occurred. Please check the system eventlog for additional information.
.
Language=Russian
A Machine Check Error has occurred. Please check the system eventlog for additional information.
.
Language=Polish
Wystąpił błąd sprawdzania komputera. Dodatkowe informacje można znaleźć w dzienniku zdarzeń systemowych.
.
Language=Romanian
A Machine Check Error has occurred. Please check the system eventlog for additional information.
.

MessageId=652
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_DATABASE_ERROR
Language=English
There was error [%2] processing the driver database.
.
Language=Russian
There was error [%2] processing the driver database.
.
Language=Polish
Wystąpił błąd [%2] podczas przetwarzania bazy danych sterowników.
.
Language=Romanian
There was error [%2] processing the driver database.
.

MessageId=653
Severity=Success
Facility=System
SymbolicName=ERROR_SYSTEM_HIVE_TOO_LARGE
Language=English
System hive size has exceeded its limit.
.
Language=Russian
System hive size has exceeded its limit.
.
Language=Polish
Przekroczono limit rozmiaru gałęzi systemu.
.
Language=Romanian
System hive size has exceeded its limit.
.

MessageId=654
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_FAILED_PRIOR_UNLOAD
Language=English
The driver could not be loaded because a previous version of the driver is still in memory.
.
Language=Russian
The driver could not be loaded because a previous version of the driver is still in memory.
.
Language=Polish
Nie można załadować sterownika, ponieważ jego poprzednia wersja nadal znajduje się w pamięci.
.
Language=Romanian
The driver could not be loaded because a previous version of the driver is still in memory.
.

MessageId=655
Severity=Success
Facility=System
SymbolicName=ERROR_VOLSNAP_PREPARE_HIBERNATE
Language=English
Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation.
.
Language=Russian
Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation.
.
Language=Polish
Zaczekaj, aż usługa kopii woluminu w tle przygotuje wolumin %hs do hibernacji.
.
Language=Romanian
Please wait while the Volume Shadow Copy Service prepares volume %hs for hibernation.
.

MessageId=656
Severity=Success
Facility=System
SymbolicName=ERROR_HIBERNATION_FAILURE
Language=English
The system has failed to hibernate (The error code is %hs). Hibernation will be disabled until the system is restarted.
.
Language=Russian
The system has failed to hibernate (The error code is %hs). Hibernation will be disabled until the system is restarted.
.
Language=Polish
System nie może wykonać hibernacji (kod błędu: %hs). Hibernacja będzie wyłączona do czasu ponownego uruchomienia systemu.
.
Language=Romanian
The system has failed to hibernate (The error code is %hs). Hibernation will be disabled until the system is restarted.
.

MessageId=657
Severity=Success
Facility=System
SymbolicName=ERROR_HUNG_DISPLAY_DRIVER_THREAD
Language=English
The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft.
.
Language=Russian
The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft.
.
Language=Polish
Sterownik ekranu %hs przestał normalnie pracować. Zapisz pracę i dokonaj ponownego rozruchu systemu, aby przywrócić wszystkie funkcje ekranu. Przy następnym ponownym rozruchu komputera zostanie wyświetlone okno dialogowe umożliwiające zgłoszenie tego błędu firmie Microsoft.
.
Language=Romanian
The %hs display driver has stopped working normally. Save your work and reboot the system to restore full display functionality. The next time you reboot the machine a dialog will be displayed giving you a chance to report this failure to Microsoft.
.

MessageId=665
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_SYSTEM_LIMITATION
Language=English
The requested operation could not be completed due to a file system limitation.
.
Language=Russian
The requested operation could not be completed due to a file system limitation.
.
Language=Polish
Nie można ukończyć żądanej operacji z powodu ograniczenia systemu plików.
.
Language=Romanian
The requested operation could not be completed due to a file system limitation.
.

MessageId=668
Severity=Success
Facility=System
SymbolicName=ERROR_ASSERTION_FAILURE
Language=English
An assertion failure has occurred.
.
Language=Russian
An assertion failure has occurred.
.
Language=Polish
Wystąpił błąd potwierdzenia.
.
Language=Romanian
An assertion failure has occurred.
.

MessageId=669
Severity=Success
Facility=System
SymbolicName=ERROR_VERIFIER_STOP
Language=English
Application verifier has found an error in the current process.
.
Language=Russian
Application verifier has found an error in the current process.
.
Language=Polish
Weryfikator aplikacji napotkał błąd w bieżącym procesie.
.
Language=Romanian
Application verifier has found an error in the current process.
.

MessageId=670
Severity=Success
Facility=System
SymbolicName=ERROR_WOW_ASSERTION
Language=English
WOW Assertion Error.
.
Language=Russian
WOW Assertion Error.
.
Language=Polish
Błąd potwierdzenia WOW.
.
Language=Romanian
WOW Assertion Error.
.

MessageId=671
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_BAD_MPS_TABLE
Language=English
A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update.
.
Language=Russian
A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update.
.
Language=Polish
Brak urządzenia w tabeli MPS systemu BIOS. Urządzenie nie zostanie użyte. Skontaktuj się z producentem, aby otrzymać aktualizację systemu BIOS.
.
Language=Romanian
A device is missing in the system BIOS MPS table. This device will not be used. Please contact your system vendor for system BIOS update.
.

MessageId=672
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_TRANSLATION_FAILED
Language=English
A translator failed to translate resources.
.
Language=Russian
A translator failed to translate resources.
.
Language=Polish
Translator nie może zinterpretować zasobów.
.
Language=Romanian
A translator failed to translate resources.
.

MessageId=673
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_IRQ_TRANSLATION_FAILED
Language=English
A IRQ translator failed to translate resources.
.
Language=Russian
A IRQ translator failed to translate resources.
.
Language=Polish
Translator IRQ nie może zinterpretować zasobów.
.
Language=Romanian
A IRQ translator failed to translate resources.
.

MessageId=674
Severity=Success
Facility=System
SymbolicName=ERROR_PNP_INVALID_ID
Language=English
Driver %2 returned invalid ID for a child device (%3).
.
Language=Russian
Driver %2 returned invalid ID for a child device (%3).
.
Language=Polish
Sterownik %2 zwrócił nieprawidłowy identyfikator urządzenia podrzędnego (%3).
.
Language=Romanian
Driver %2 returned invalid ID for a child device (%3).
.

MessageId=675
Severity=Success
Facility=System
SymbolicName=ERROR_WAKE_SYSTEM_DEBUGGER
Language=English
The system debugger was awakened by an interrupt.
.
Language=Russian
The system debugger was awakened by an interrupt.
.
Language=Polish
Przerwanie uaktywniło debuger systemu.
.
Language=Romanian
The system debugger was awakened by an interrupt.
.

MessageId=676
Severity=Success
Facility=System
SymbolicName=ERROR_HANDLES_CLOSED
Language=English
Handles to objects have been automatically closed as a result of the requested operation.
.
Language=Russian
Handles to objects have been automatically closed as a result of the requested operation.
.
Language=Polish
Dojścia do obiektów zostały automatycznie zamknięte w wyniku żądanej operacji.
.
Language=Romanian
Handles to objects have been automatically closed as a result of the requested operation.
.

MessageId=677
Severity=Success
Facility=System
SymbolicName=ERROR_EXTRANEOUS_INFORMATION
Language=English
The specified access control list (ACL) contained more information than was expected.
.
Language=Russian
The specified access control list (ACL) contained more information than was expected.
.
Language=Polish
Określona lista kontroli dostępu (ACL) zawiera więcej informacji niż oczekiwano.
.
Language=Romanian
The specified access control list (ACL) contained more information than was expected.
.

MessageId=678
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMIT_NECESSARY
Language=English
This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired).
.
Language=Russian
This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired).
.
Language=Polish
Ten stan poziomu ostrzeżenia wskazuje, że dla danego poddrzewa Rejestru istnieje już stan transakcji, ale zlecenie transakcji zostało wcześniej przerwane. Transakcja NIE została zlecona, ale nie była też zwrócona (zatem może być z łatwością zlecona w razie potrzeby).
.
Language=Romanian
This warning level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has NOT been completed, but has not been rolled back either (so it may still be committed if desired).
.

MessageId=679
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_CHECK
Language=English
The media may have changed.
.
Language=Russian
The media may have changed.
.
Language=Polish
Nośnik mógł się zmienić.
.
Language=Romanian
The media may have changed.
.

MessageId=680
Severity=Success
Facility=System
SymbolicName=ERROR_GUID_SUBSTITUTION_MADE
Language=English
During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended.
.
Language=Russian
During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended.
.
Language=Polish
Podczas tłumaczenia globalnego identyfikatora (GUID) na identyfikator zabezpieczeń Windows NT nie znaleziono zdefiniowanego administracyjnie prefiksu GUID. Użyto zamiast tego zastępczego prefiksu, który nie wpłynie na bezpieczeństwo systemu. Może to jednak ograniczyć dostęp bardziej niż zamierzano.
.
Language=Romanian
During the translation of a global identifier (GUID) to a Windows security ID (SID), no administratively-defined GUID prefix was found. A substitute prefix was used, which will not compromise system security. However, this may provide a more restrictive access than intended.
.

MessageId=681
Severity=Success
Facility=System
SymbolicName=ERROR_STOPPED_ON_SYMLINK
Language=English
The create operation stopped after reaching a symbolic link.
.
Language=Russian
The create operation stopped after reaching a symbolic link.
.
Language=Polish
Operacja tworzenia została zatrzymana po napotkaniu łącza symbolicznego.
.
Language=Romanian
The create operation stopped after reaching a symbolic link.
.

MessageId=682
Severity=Success
Facility=System
SymbolicName=ERROR_LONGJUMP
Language=English
A long jump has been executed.
.
Language=Russian
A long jump has been executed.
.
Language=Polish
Wykonano długi skok.
.
Language=Romanian
A long jump has been executed.
.

MessageId=683
Severity=Success
Facility=System
SymbolicName=ERROR_PLUGPLAY_QUERY_VETOED
Language=English
The Plug and Play query operation was not successful.
.
Language=Russian
The Plug and Play query operation was not successful.
.
Language=Polish
Operacja zapytania Plug and Play zakończyła się niepomyślnie.
.
Language=Romanian
The Plug and Play query operation was not successful.
.

MessageId=684
Severity=Success
Facility=System
SymbolicName=ERROR_UNWIND_CONSOLIDATE
Language=English
A frame consolidation has been executed.
.
Language=Russian
A frame consolidation has been executed.
.
Language=Polish
Przeprowadzono konsolidację ramek.
.
Language=Romanian
A frame consolidation has been executed.
.

MessageId=685
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_HIVE_RECOVERED
Language=English
Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost.
.
Language=Russian
Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost.
.
Language=Polish
Gałąź rejestru (plik): %hs była uszkodzona i została odzyskana. Niektóre dane mogły zostać utracone.
.
Language=Romanian
Registry hive (file): %hs was corrupted and it has been recovered. Some data might have been lost.
.

MessageId=686
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_MIGHT_BE_INSECURE
Language=English
The application is attempting to run executable code from the module %hs. This may be insecure. An alternative, %hs, is available. Should the application use the secure module %hs?
.
Language=Russian
The application is attempting to run executable code from the module %hs. This may be insecure. An alternative, %hs, is available. Should the application use the secure module %hs?
.
Language=Polish
Aplikacja próbuje uruchomić kod wykonywalny z modułu %hs. Może to być niebezpieczne. Dostępna jest alternatywa, %hs. Czy aplikacja ma użyć bezpiecznego modułu %hs?
.
Language=Romanian
The application is attempting to run executable code from the module %hs. This may be insecure. An alternative, %hs, is available. Should the application use the secure module %hs?
.

MessageId=687
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_MIGHT_BE_INCOMPATIBLE
Language=English
The application is loading executable code from the module %hs. This is secure, but may be incompatible with previous releases of the operating system. An alternative, %hs, is available. Should the application use the secure module %hs?
.
Language=Russian
The application is loading executable code from the module %hs. This is secure, but may be incompatible with previous releases of the operating system. An alternative, %hs, is available. Should the application use the secure module %hs?
.
Language=Polish
Aplikacja ładuje kod wykonywalny z modułu %hs. Jest to bezpieczne, ale może być niezgodne z poprzednimi wydaniami systemu operacyjnego. Dostępna jest alternatywa, %hs. Czy aplikacja ma użyć bezpiecznego modułu %hs?
.
Language=Romanian
The application is loading executable code from the module %hs. This is secure, but may be incompatible with previous releases of the operating system. An alternative, %hs, is available. Should the application use the secure module %hs?
.

MessageId=688
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_EXCEPTION_NOT_HANDLED
Language=English
Debugger did not handle the exception.
.
Language=Russian
Debugger did not handle the exception.
.
Language=Polish
Debuger nie obsłużył wyjątku.
.
Language=Romanian
Debugger did not handle the exception.
.

MessageId=689
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_REPLY_LATER
Language=English
Debugger will reply later.
.
Language=Russian
Debugger will reply later.
.
Language=Polish
Debuger udzieli odpowiedź później.
.
Language=Romanian
Debugger will reply later.
.

MessageId=690
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_UNABLE_TO_PROVIDE_HANDLE
Language=English
Debugger can not provide handle.
.
Language=Russian
Debugger can not provide handle.
.
Language=Polish
Debuger nie może udostępnić uchwytu.
.
Language=Romanian
Debugger can not provide handle.
.

MessageId=691
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_TERMINATE_THREAD
Language=English
Debugger terminated thread.
.
Language=Russian
Debugger terminated thread.
.
Language=Polish
Debuger przerwał wątek.
.
Language=Romanian
Debugger terminated thread.
.

MessageId=692
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_TERMINATE_PROCESS
Language=English
Debugger terminated process.
.
Language=Russian
Debugger terminated process.
.
Language=Polish
Debuger przerwał proces.
.
Language=Romanian
Debugger terminated process.
.

MessageId=693
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTROL_C
Language=English
Debugger got control C.
.
Language=Russian
Debugger got control C.
.
Language=Polish
Debuger przejął kontrolę C.
.
Language=Romanian
Debugger got control C.
.

MessageId=694
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_PRINTEXCEPTION_C
Language=English
Debugger printed exception on control C.
.
Language=Russian
Debugger printed exception on control C.
.
Language=Polish
Debuger wydrukował wyjątek dla kontroli C.
.
Language=Romanian
Debugger printed exception on control C.
.

MessageId=695
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_RIPEXCEPTION
Language=English
Debugger received RIP exception.
.
Language=Russian
Debugger received RIP exception.
.
Language=Polish
Debuger otrzymał wyjątek RIP.
.
Language=Romanian
Debugger received RIP exception.
.

MessageId=696
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTROL_BREAK
Language=English
Debugger received control break.
.
Language=Russian
Debugger received control break.
.
Language=Polish
Debuger otrzymał kombinację control break.
.
Language=Romanian
Debugger received control break.
.

MessageId=697
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_COMMAND_EXCEPTION
Language=English
Debugger command communication exception.
.
Language=Russian
Debugger command communication exception.
.
Language=Polish
Wyjątek komunikacji polecenia debugera.
.
Language=Romanian
Debugger command communication exception.
.

MessageId=698
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_NAME_EXISTS
Language=English
An attempt was made to create an object and the object name already existed.
.
Language=Russian
An attempt was made to create an object and the object name already existed.
.
Language=Polish
Podjęto próbę utworzenia obiektu, podczas gdy taka nazwa obiektu już istniała.
.
Language=Romanian
An attempt was made to create an object and the object name already existed.
.

MessageId=699
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_WAS_SUSPENDED
Language=English
A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
.
Language=Russian
A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
.
Language=Polish
Nastąpiło zamknięcie wątku, gdy wątek ten został zawieszony. Wątek został wznowiony i wznowiono operację zamykania.
.
Language=Romanian
A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
.

MessageId=700
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_NOT_AT_BASE
Language=English
An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image.
.
Language=Russian
An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image.
.
Language=Polish
Zamapowanie obrazu pod adresem określonym w pliku obrazu nie było możliwe. Na tym obrazie trzeba wykonać lokalną naprawę.
.
Language=Romanian
An image file could not be mapped at the address specified in the image file. Local fixups must be performed on this image.
.

MessageId=701
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_STATE_CREATED
Language=English
This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created.
.
Language=Russian
This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created.
.
Language=Polish
Ten stan poziomu informacji wskazuje, że określony stan transakcji poddrzewa Rejestru jeszcze nie istniał i musiał być utworzony.
.
Language=Romanian
This informational level status indicates that a specified registry sub-tree transaction state did not yet exist and had to be created.
.

MessageId=702
Severity=Success
Facility=System
SymbolicName=ERROR_SEGMENT_NOTIFICATION
Language=English
A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments.
.
Language=Russian
A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments.
.
Language=Polish
Wirtualna maszyna DOS ładuje, zwalnia lub przenosi obraz segmentu programu MS-DOS lub Win16. Tworzony jest wyjątek, dzięki czemu debuger może ładować zwalniać i śledzić symbole i punkty przerwania w tych 16-bitowych segmentach.
.
Language=Romanian
A virtual DOS machine (VDM) is loading, unloading, or moving an MS-DOS or Win16 program segment image. An exception is raised so a debugger can load, unload or track symbols and breakpoints within these 16-bit segments.
.

MessageId=703
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_CURRENT_DIRECTORY
Language=English
The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit.
.
Language=Russian
The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit.
.
Language=Polish
Proces nie może przełączyć się do bieżącego katalogu startowego %hs. Wybierz OK, aby ustawić bieżący katalog na %hs, albo wybierz "Anuluj", aby zakończyć.
.
Language=Romanian
The process cannot switch to the startup current directory %hs. Select OK to set current directory to %hs, or select CANCEL to exit.
.

MessageId=704
Severity=Success
Facility=System
SymbolicName=ERROR_FT_READ_RECOVERY_FROM_BACKUP
Language=English
To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device.
.
Language=Russian
To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device.
.
Language=Polish
Aby wykonać żądanie odczytu, odporny na błędy system plików NT pomyślnie odczytał dane z dodatkowej kopii. Powodem było to, że wystąpiła awaria w elemencie odpornego na błędy woluminu, ale nie było możliwe ponowne przypisanie odpowiedzialnego za awarię obszaru urządzenia.
.
Language=Romanian
To satisfy a read request, the NT fault-tolerant file system successfully read the requested data from a redundant copy. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was unable to reassign the failing area of the device.
.

MessageId=705
Severity=Success
Facility=System
SymbolicName=ERROR_FT_WRITE_RECOVERY
Language=English
To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device.
.
Language=Russian
To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device.
.
Language=Polish
Aby wykonać żądanie zapisu odporny na błędy system plików NT pomyślnie zapisał dodatkową kopię danych. Powodem było to, że wystąpiła awaria w elemencie odpornego na błędy woluminu, ale nie było możliwe ponowne przypisanie odpowiedzialnego za awarię obszaru urządzenia.
.
Language=Romanian
To satisfy a write request, the NT fault-tolerant file system successfully wrote a redundant copy of the information. This was done because the file system encountered a failure on a member of the fault-tolerant volume, but was not able to reassign the failing area of the device.
.

MessageId=706
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_MACHINE_TYPE_MISMATCH
Language=English
The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.
.
Language=Russian
The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.
.
Language=Polish
Plik obrazu %hs jest prawidłowy, ale jest dla innego typu komuptera niż bieżący. Wybierz OK, aby kontynuować, albo ANULUJ, aby anulować ładowanie biblioteki DLL.
.
Language=Romanian
The image file %hs is valid, but is for a machine type other than the current machine. Select OK to continue, or CANCEL to fail the DLL load.
.

MessageId=707
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_PARTIAL
Language=English
The network transport returned partial data to its client. The remaining data will be sent later.
.
Language=Russian
The network transport returned partial data to its client. The remaining data will be sent later.
.
Language=Polish
Transport sieciowy zwrócił do klienta częściowe dane. Pozostałe dane zostaną wysłane później.
.
Language=Romanian
The network transport returned partial data to its client. The remaining data will be sent later.
.

MessageId=708
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_EXPEDITED
Language=English
The network transport returned data to its client that was marked as expedited by the remote system.
.
Language=Russian
The network transport returned data to its client that was marked as expedited by the remote system.
.
Language=Polish
Transport sieciowy zwrócił do klienta dane oznaczone przez zdalny system jako przyspieszone.
.
Language=Romanian
The network transport returned data to its client that was marked as expedited by the remote system.
.

MessageId=709
Severity=Success
Facility=System
SymbolicName=ERROR_RECEIVE_PARTIAL_EXPEDITED
Language=English
The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later.
.
Language=Russian
The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later.
.
Language=Polish
Transport sieciowy zwrócił do klienta częściowe dane, oznaczone przez zdalny system jako przyspieszone. Pozostałe dane zostaną wysłane później.
.
Language=Romanian
The network transport returned partial data to its client and this data was marked as expedited by the remote system. The remaining data will be sent later.
.

MessageId=710
Severity=Success
Facility=System
SymbolicName=ERROR_EVENT_DONE
Language=English
The TDI indication has completed successfully.
.
Language=Russian
The TDI indication has completed successfully.
.
Language=Polish
Wskazywanie TDI zostało ukończone pomyślnie.
.
Language=Romanian
The TDI indication has completed successfully.
.

MessageId=711
Severity=Success
Facility=System
SymbolicName=ERROR_EVENT_PENDING
Language=English
The TDI indication has entered the pending state.
.
Language=Russian
The TDI indication has entered the pending state.
.
Language=Polish
Wskazywanie TDI przeszło w stan oczekiwania.
.
Language=Romanian
The TDI indication has entered the pending state.
.

MessageId=712
Severity=Success
Facility=System
SymbolicName=ERROR_CHECKING_FILE_SYSTEM
Language=English
Checking file system on %wZ.
.
Language=Russian
Checking file system on %wZ.
.
Language=Polish
Sprawdzanie systemu plików na %wZ
.
Language=Romanian
Checking file system on %wZ.
.

MessageId=714
Severity=Success
Facility=System
SymbolicName=ERROR_PREDEFINED_HANDLE
Language=English
The specified registry key is referenced by a predefined handle.
.
Language=Russian
The specified registry key is referenced by a predefined handle.
.
Language=Polish
Do określonego klucza Rejestru odwołuje się dojście uprzednio zdefiniowane.
.
Language=Romanian
The specified registry key is referenced by a predefined handle.
.

MessageId=715
Severity=Success
Facility=System
SymbolicName=ERROR_WAS_UNLOCKED
Language=English
The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process.
.
Language=Russian
The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process.
.
Language=Polish
Zabezpieczenie zablokowanej strony zmieniono na "Brak dostępu" i strona została odblokowana z pamięci i z procesu.
.
Language=Romanian
The page protection of a locked page was changed to 'No Access' and the page was unlocked from memory and from the process.
.

MessageId=717
Severity=Success
Facility=System
SymbolicName=ERROR_WAS_LOCKED
Language=English
One of the pages to lock was already locked.
.
Language=Russian
One of the pages to lock was already locked.
.
Language=Polish
Jedna ze stron przewidzianych do zablokowania była już zablokowana.
.
Language=Romanian
One of the pages to lock was already locked.
.

MessageId=720
Severity=Success
Facility=System
SymbolicName=ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE
Language=English
The image file %hs is valid, but is for a machine type other than the current machine.
.
Language=Russian
The image file %hs is valid, but is for a machine type other than the current machine.
.
Language=Polish
Plik obrazu %hs jest prawidłowy, ale jest dla innego typu komputera niż bieżący.
.
Language=Romanian
The image file %hs is valid, but is for a machine type other than the current machine.
.

MessageId=721
Severity=Success
Facility=System
SymbolicName=ERROR_NO_YIELD_PERFORMED
Language=English
A yield execution was performed and no thread was available to run.
.
Language=Russian
A yield execution was performed and no thread was available to run.
.
Language=Polish
Wykonano przekazanie wykonywania i żaden wątek nie był dostępny do uruchomienia.
.
Language=Romanian
A yield execution was performed and no thread was available to run.
.

MessageId=722
Severity=Success
Facility=System
SymbolicName=ERROR_TIMER_RESUME_IGNORED
Language=English
The resumable flag to a timer API was ignored.
.
Language=Russian
The resumable flag to a timer API was ignored.
.
Language=Polish
Zignorowano wznawialną flagę czasomierza API.
.
Language=Romanian
The resumable flag to a timer API was ignored.
.

MessageId=723
Severity=Success
Facility=System
SymbolicName=ERROR_ARBITRATION_UNHANDLED
Language=English
The arbiter has deferred arbitration of these resources to its parent.
.
Language=Russian
The arbiter has deferred arbitration of these resources to its parent.
.
Language=Polish
Arbiter wstrzymał rozstrzyganie o przynależności zasobów do obiektów nadrzędnych
.
Language=Romanian
The arbiter has deferred arbitration of these resources to its parent.
.

MessageId=724
Severity=Success
Facility=System
SymbolicName=ERROR_CARDBUS_NOT_SUPPORTED
Language=English
The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller.
.
Language=Russian
The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller.
.
Language=Polish
Urządzenie \"%hs"\ wykryło w gnieździe kartę CardBus, ale oprogramowanie firmware w systemie nie jest skonfigurowane w sposób zezwalający na uruchamianie kontrolera CardBus w trybie CardBus. System operacyjny będzie obecnie akceptować tylko 16 bitowe karty pc (R2) dla tego kontrolera.
.
Language=Romanian
The device \"%hs\" has detected a CardBus card in its slot, but the firmware on this system is not configured to allow the CardBus controller to be run in CardBus mode. The operating system will currently accept only 16-bit (R2) pc-cards on this controller.
.

MessageId=725
Severity=Success
Facility=System
SymbolicName=ERROR_MP_PROCESSOR_MISMATCH
Language=English
The CPUs in this multiprocessor system are not all the same revision level. To use all processors the operating system restricts itself to the features of the least capable processor in the system. Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported.
.
Language=Russian
The CPUs in this multiprocessor system are not all the same revision level. To use all processors the operating system restricts itself to the features of the least capable processor in the system. Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported.
.
Language=Polish
Jednostki CPU w tym systemie wieloprocesorowym mają różne poziomy wersji. Aby używać wszystkich procesorów, system operacyjny ograniczy możliwości zestawu do funkcji obsługiwanych przez procesor o najmniejszych możliwościach. Jeśli wystąpią problemy, skontaktuj się z producentem jednostki CPU, aby upewnić się czy zestaw procesorów jest obsługiwany.
.
Language=Romanian
The CPUs in this multiprocessor system are not all the same revision level. To use all processors the operating system restricts itself to the features of the least capable processor in the system. Should problems occur with this system, contact the CPU manufacturer to see if this mix of processors is supported.
.

MessageId=726
Severity=Success
Facility=System
SymbolicName=ERROR_HIBERNATED
Language=English
The system was put into hibernation.
.
Language=Russian
The system was put into hibernation.
.
Language=Polish
System został wprowadzony w stan hibernacji.
.
Language=Romanian
The system was put into hibernation.
.

MessageId=727
Severity=Success
Facility=System
SymbolicName=ERROR_RESUME_HIBERNATION
Language=English
The system was resumed from hibernation.
.
Language=Russian
The system was resumed from hibernation.
.
Language=Polish
System wyszedł ze stanu hibernacji.
.
Language=Romanian
The system was resumed from hibernation.
.

MessageId=728
Severity=Success
Facility=System
SymbolicName=ERROR_FIRMWARE_UPDATED
Language=English
ReactOS has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3].
.
Language=Russian
ReactOS has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3].
.
Language=Polish
System ReactOS wykrył, że oprogramowanie układowe systemu (BIOS) zostało zaktualizowane [poprzednia data oprogramowania układowego = %2, bieżąca data oprogramowania układowego %3].
.
Language=Romanian
ReactOS has detected that the system firmware (BIOS) was updated [previous firmware date = %2, current firmware date %3].
.

MessageId=729
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVERS_LEAKING_LOCKED_PAGES
Language=English
A device driver is leaking locked I/O pages causing system degradation. The system has automatically enabled tracking code in order to try and catch the culprit.
.
Language=Russian
A device driver is leaking locked I/O pages causing system degradation. The system has automatically enabled tracking code in order to try and catch the culprit.
.
Language=Polish
W sterowniku urządzenia powstają przecieki zablokowanych stron We/Wy, które powodują degradację systemu. System włączy automatycznie kod śledzenia, aby podjąć próbę odnalezienia programu, który powoduje ten problem.
.
Language=Romanian
A device driver is leaking locked I/O pages causing system degradation. The system has automatically enabled tracking code in order to try and catch the culprit.
.

MessageId=730
Severity=Success
Facility=System
SymbolicName=ERROR_WAKE_SYSTEM
Language=English
The system has awoken
.
Language=Russian
The system has awoken
.
Language=Polish
System został obudzony.
.
Language=Romanian
The system has awoken
.

MessageId=741
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE
Language=English
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.
Language=Russian
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.
Language=Polish
Menedżer obiektów powinien przeprowadzić ponowną analizę składni, ponieważ nazwa pliku dała w efekcie łącze symboliczne.
.
Language=Romanian
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.

MessageId=742
Severity=Success
Facility=System
SymbolicName=ERROR_OPLOCK_BREAK_IN_PROGRESS
Language=English
An open/create operation completed while an oplock break is underway.
.
Language=Russian
An open/create operation completed while an oplock break is underway.
.
Language=Polish
Zakończono operację otwierania/tworzenia podczas trwania "oplock break".
.
Language=Romanian
An open/create operation completed while an oplock break is underway.
.

MessageId=743
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_MOUNTED
Language=English
A new volume has been mounted by a file system.
.
Language=Russian
A new volume has been mounted by a file system.
.
Language=Polish
Nowy wolumin został zainstalowany przez system plików.
.
Language=Romanian
A new volume has been mounted by a file system.
.

MessageId=744
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMITTED
Language=English
This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed.
.
Language=Russian
This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed.
.
Language=Polish
Ten stan poziomu sukcesu wskazuje, że dla danego poddrzewa Rejestru istnieje już stan transakcji, ale zlecenie transakcji zostało wcześniej przerwane. Teraz zlecenie zostało zakończone.
.
Language=Romanian
This success level status indicates that the transaction state already exists for the registry sub-tree, but that a transaction commit was previously aborted. The commit has now been completed.
.

MessageId=745
Severity=Success
Facility=System
SymbolicName=ERROR_NOTIFY_CLEANUP
Language=English
This indicates that a notify change request has been completed due to closing the handle which made the notify change request.
.
Language=Russian
This indicates that a notify change request has been completed due to closing the handle which made the notify change request.
.
Language=Polish
Wskazuje, że żądanie potwierdzenia zmiany zostało zakończone z powodu zamknięcia dojścia, które zgłosiło to żądanie.
.
Language=Romanian
This indicates that a notify change request has been completed due to closing the handle which made the notify change request.
.

MessageId=746
Severity=Success
Facility=System
SymbolicName=ERROR_PRIMARY_TRANSPORT_CONNECT_FAILED
Language=English
An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport.
.
Language=Russian
An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport.
.
Language=Polish
Podjęto próbę podłączenia do zdalnego serwera %hs przez transport podstawowy, ale próba nie powiodła się. Uzyskano połączenie przez transport pomocniczy.
.
Language=Romanian
An attempt was made to connect to the remote server %hs on the primary transport, but the connection failed. The computer WAS able to connect on a secondary transport.
.

MessageId=747
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_TRANSITION
Language=English
Page fault was a transition fault.
.
Language=Russian
Page fault was a transition fault.
.
Language=Polish
Błąd strony był błędem przejścia.
.
Language=Romanian
Page fault was a transition fault.
.

MessageId=748
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_DEMAND_ZERO
Language=English
Page fault was a demand zero fault.
.
Language=Russian
Page fault was a demand zero fault.
.
Language=Polish
Błąd strony był błędem typu wymagane zero.
.
Language=Romanian
Page fault was a demand zero fault.
.

MessageId=749
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_COPY_ON_WRITE
Language=English
Page fault was a demand zero fault.
.
Language=Russian
Page fault was a demand zero fault.
.
Language=Polish
Błąd strony był błędem typu wymagane zero.
.
Language=Romanian
Page fault was a demand zero fault.
.

MessageId=750
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_GUARD_PAGE
Language=English
Page fault was a demand zero fault.
.
Language=Russian
Page fault was a demand zero fault.
.
Language=Polish
Błąd strony był błędem typu wymagane zero.
.
Language=Romanian
Page fault was a demand zero fault.
.

MessageId=751
Severity=Success
Facility=System
SymbolicName=ERROR_PAGE_FAULT_PAGING_FILE
Language=English
Page fault was satisfied by reading from a secondary storage device.
.
Language=Russian
Page fault was satisfied by reading from a secondary storage device.
.
Language=Polish
Błąd strony został naprawiony przez odczyt z zapasowego urządzenia magazynującego.
.
Language=Romanian
Page fault was satisfied by reading from a secondary storage device.
.

MessageId=752
Severity=Success
Facility=System
SymbolicName=ERROR_CACHE_PAGE_LOCKED
Language=English
Cached page was locked during operation.
.
Language=Russian
Cached page was locked during operation.
.
Language=Polish
Buforowana strona została zablokowana podczas operacji.
.
Language=Romanian
Cached page was locked during operation.
.

MessageId=753
Severity=Success
Facility=System
SymbolicName=ERROR_CRASH_DUMP
Language=English
Crash dump exists in paging file.
.
Language=Russian
Crash dump exists in paging file.
.
Language=Polish
W pliku stronicowania istnieje zrzut awaryjny.
.
Language=Romanian
Crash dump exists in paging file.
.

MessageId=754
Severity=Success
Facility=System
SymbolicName=ERROR_BUFFER_ALL_ZEROS
Language=English
Specified buffer contains all zeros.
.
Language=Russian
Specified buffer contains all zeros.
.
Language=Polish
Określony bufor zawiera same zera.
.
Language=Romanian
Specified buffer contains all zeros.
.

MessageId=755
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_OBJECT
Language=English
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.
Language=Russian
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.
Language=Polish
Menedżer obiektów powinien przeprowadzić ponowną analizę składni, ponieważ nazwa pliku dała w efekcie łącze symboliczne.
.
Language=Romanian
A reparse should be performed by the Object Manager since the name of the file resulted in a symbolic link.
.

MessageId=756
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_REQUIREMENTS_CHANGED
Language=English
The device has succeeded a query-stop and its resource requirements have changed.
.
Language=Russian
The device has succeeded a query-stop and its resource requirements have changed.
.
Language=Polish
Urządzenie pomyślnie wykonało operację zatrzymania zapytania i wymagania dotyczące zasobów uległy zmianie.
.
Language=Romanian
The device has succeeded a query-stop and its resource requirements have changed.
.

MessageId=757
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSLATION_COMPLETE
Language=English
The translator has translated these resources into the global space and no further translations should be performed.
.
Language=Russian
The translator has translated these resources into the global space and no further translations should be performed.
.
Language=Polish
Translator zinterpretował zasoby w przestrzeni globalnej i nie należy wykonywać więcej interpretacji.
.
Language=Romanian
The translator has translated these resources into the global space and no further translations should be performed.
.

MessageId=758
Severity=Success
Facility=System
SymbolicName=ERROR_NOTHING_TO_TERMINATE
Language=English
A process being terminated has no threads to terminate.
.
Language=Russian
A process being terminated has no threads to terminate.
.
Language=Polish
Kończony proces nie ma wątków do zakończenia.
.
Language=Romanian
A process being terminated has no threads to terminate.
.

MessageId=759
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_NOT_IN_JOB
Language=English
The specified process is not part of a job.
.
Language=Russian
The specified process is not part of a job.
.
Language=Polish
Określony proces nie jest częścią zadania.
.
Language=Romanian
The specified process is not part of a job.
.

MessageId=760
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_IN_JOB
Language=English
The specified process is part of a job.
.
Language=Russian
The specified process is part of a job.
.
Language=Polish
Określony proces jest częścią zadania.
.
Language=Romanian
The specified process is part of a job.
.

MessageId=761
Severity=Success
Facility=System
SymbolicName=ERROR_VOLSNAP_HIBERNATE_READY
Language=English
The system is now ready for hibernation.
.
Language=Russian
The system is now ready for hibernation.
.
Language=Polish
System jest teraz gotowy do hibernacji.
.
Language=Romanian
The system is now ready for hibernation.
.

MessageId=762
Severity=Success
Facility=System
SymbolicName=ERROR_FSFILTER_OP_COMPLETED_SUCCESSFULLY
Language=English
A file system or file system filter driver has successfully completed an FsFilter operation.
.
Language=Russian
A file system or file system filter driver has successfully completed an FsFilter operation.
.
Language=Polish
System plików lub sterownik filtru systemu plików ukończył pomyślnie operację FsFilter.
.
Language=Romanian
A file system or file system filter driver has successfully completed an FsFilter operation.
.

MessageId=763
Severity=Success
Facility=System
SymbolicName=ERROR_INTERRUPT_VECTOR_ALREADY_CONNECTED
Language=English
The specified interrupt vector was already connected.
.
Language=Russian
The specified interrupt vector was already connected.
.
Language=Polish
Określony wektor przerwania był już połączony.
.
Language=Romanian
The specified interrupt vector was already connected.
.

MessageId=764
Severity=Success
Facility=System
SymbolicName=ERROR_INTERRUPT_STILL_CONNECTED
Language=English
The specified interrupt vector is still connected.
.
Language=Russian
The specified interrupt vector is still connected.
.
Language=Polish
Określony wektor przerwania jest wciąż połączony.
.
Language=Romanian
The specified interrupt vector is still connected.
.

MessageId=765
Severity=Success
Facility=System
SymbolicName=ERROR_WAIT_FOR_OPLOCK
Language=English
An operation is blocked waiting for an oplock.
.
Language=Russian
An operation is blocked waiting for an oplock.
.
Language=Polish
Operacja jest zablokowana i czeka na operację oplock.
.
Language=Romanian
An operation is blocked waiting for an oplock.
.

MessageId=766
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_EXCEPTION_HANDLED
Language=English
Debugger handled exception.
.
Language=Russian
Debugger handled exception.
.
Language=Polish
Debuger obsłużył wyjątek
.
Language=Romanian
Debugger handled exception.
.

MessageId=767
Severity=Success
Facility=System
SymbolicName=ERROR_DBG_CONTINUE
Language=English
Debugger continued
.
Language=Russian
Debugger continued
.
Language=Polish
Debuger kontynuuje pracę
.
Language=Romanian
Debugger continued
.

MessageId=768
Severity=Success
Facility=System
SymbolicName=ERROR_CALLBACK_POP_STACK
Language=English
An exception occurred in a user mode callback and the kernel callback frame should be removed.
.
Language=Russian
An exception occurred in a user mode callback and the kernel callback frame should be removed.
.
Language=Polish
Wystąpił wyjątek w wywołaniu zwrotnym trybu użytkownika, a ramka wywołania zwrotnego jądra powinna zostać usunięta.
.
Language=Romanian
An exception occurred in a user mode callback and the kernel callback frame should be removed.
.

MessageId=769
Severity=Success
Facility=System
SymbolicName=ERROR_COMPRESSION_DISABLED
Language=English
Compression is disabled for this volume.
.
Language=Russian
Compression is disabled for this volume.
.
Language=Polish
Kompresja jest wyłączona dla tego woluminu.
.
Language=Romanian
Compression is disabled for this volume.
.

MessageId=770
Severity=Success
Facility=System
SymbolicName=ERROR_CANTFETCHBACKWARDS
Language=English
The data provider cannot fetch backwards through a result set.
.
Language=Russian
The data provider cannot fetch backwards through a result set.
.
Language=Polish
Dostawca danych nie może pobierać zestawu wyników wstecz.
.
Language=Romanian
The data provider cannot fetch backwards through a result set.
.

MessageId=771
Severity=Success
Facility=System
SymbolicName=ERROR_CANTSCROLLBACKWARDS
Language=English
The data provider cannot scroll backwards through a result set.
.
Language=Russian
The data provider cannot scroll backwards through a result set.
.
Language=Polish
Dostawca danych nie może przewijać zestawu wyników wstecz.
.
Language=Romanian
The data provider cannot scroll backwards through a result set.
.

MessageId=772
Severity=Success
Facility=System
SymbolicName=ERROR_ROWSNOTRELEASED
Language=English
The data provider requires that previously fetched data is released before asking for more data.
.
Language=Russian
The data provider requires that previously fetched data is released before asking for more data.
.
Language=Polish
Dostawca danych wymaga zwolnienia poprzednio pobranych danych przed żądaniem kolejnych danych.
.
Language=Romanian
The data provider requires that previously fetched data is released before asking for more data.
.

MessageId=773
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_ACCESSOR_FLAGS
Language=English
The data provider was not able to interpret the flags set for a column binding in an accessor.
.
Language=Russian
The data provider was not able to interpret the flags set for a column binding in an accessor.
.
Language=Polish
Dostawca danych nie może zinterpretować zestawu flag dla wiązania kolumn w metodzie dostępu.
.
Language=Romanian
The data provider was not able to interpret the flags set for a column binding in an accessor.
.

MessageId=774
Severity=Success
Facility=System
SymbolicName=ERROR_ERRORS_ENCOUNTERED
Language=English
One or more errors occurred while processing the request.
.
Language=Russian
One or more errors occurred while processing the request.
.
Language=Polish
Wystąpił co najmniej jeden błąd podczas przetwarzania żądania.
.
Language=Romanian
One or more errors occurred while processing the request.
.

MessageId=775
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CAPABLE
Language=English
The implementation is not capable of performing the request.
.
Language=Russian
The implementation is not capable of performing the request.
.
Language=Polish
Bieżąca implementacja nie może wykonać żądania.
.
Language=Romanian
The implementation is not capable of performing the request.
.

MessageId=776
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_OUT_OF_SEQUENCE
Language=English
The client of a component requested an operation which is not valid given the state of the component instance.
.
Language=Russian
The client of a component requested an operation which is not valid given the state of the component instance.
.
Language=Polish
Klient składnika zażądał nieprawidłowej operacji podającej stan wystąpienia składnika.
.
Language=Romanian
The client of a component requested an operation which is not valid given the state of the component instance.
.

MessageId=777
Severity=Success
Facility=System
SymbolicName=ERROR_VERSION_PARSE_ERROR
Language=English
A version number could not be parsed.
.
Language=Russian
A version number could not be parsed.
.
Language=Polish
Nie można przeanalizować numeru wersji.
.
Language=Romanian
A version number could not be parsed.
.

MessageId=778
Severity=Success
Facility=System
SymbolicName=ERROR_BADSTARTPOSITION
Language=English
The iterator's start position is invalid.
.
Language=Russian
The iterator's start position is invalid.
.
Language=Polish
Pozycja początkowa iteratora jest nieprawidłowa.
.
Language=Romanian
The iterator's start position is invalid.
.

MessageId=994
Severity=Success
Facility=System
SymbolicName=ERROR_EA_ACCESS_DENIED
Language=English
Access to the extended attribute was denied.
.
Language=Russian
Access to the extended attribute was denied.
.
Language=Polish
Dostęp do atrybutu rozszerzonego został zabroniony.
.
Language=Romanian
Access to the extended attribute was denied.
.

MessageId=995
Severity=Success
Facility=System
SymbolicName=ERROR_OPERATION_ABORTED
Language=English
The I/O operation has been aborted because of either a thread exit or an application request.
.
Language=Russian
The I/O operation has been aborted because of either a thread exit or an application request.
.
Language=Polish
Operacja We/Wy została przerwana z powodu zakończenia wątku lub żądania aplikacji.
.
Language=Romanian
The I/O operation has been aborted because of either a thread exit or an application request.
.

MessageId=996
Severity=Success
Facility=System
SymbolicName=ERROR_IO_INCOMPLETE
Language=English
Overlapped I/O event is not in a signaled state.
.
Language=Russian
Overlapped I/O event is not in a signaled state.
.
Language=Polish
Pokrywające się zdarzenie We/Wy nie jest w stanie sygnalizacji.
.
Language=Romanian
Overlapped I/O event is not in a signaled state.
.

MessageId=997
Severity=Success
Facility=System
SymbolicName=ERROR_IO_PENDING
Language=English
Overlapped I/O operation is in progress.
.
Language=Russian
Overlapped I/O operation is in progress.
.
Language=Polish
Pokrywająca się operacja We/Wy jest w toku.
.
Language=Romanian
Overlapped I/O operation is in progress.
.

MessageId=998
Severity=Success
Facility=System
SymbolicName=ERROR_NOACCESS
Language=English
Invalid access to memory location.
.
Language=Russian
Invalid access to memory location.
.
Language=Polish
Nieprawidłowy dostęp do lokalizacji w pamięci.
.
Language=Romanian
Invalid access to memory location.
.

MessageId=999
Severity=Success
Facility=System
SymbolicName=ERROR_SWAPERROR
Language=English
Error performing inpage operation.
.
Language=Russian
Error performing inpage operation.
.
Language=Polish
Błąd wykonania operacji inpage.
.
Language=Romanian
Error performing inpage operation.
.

MessageId=1001
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_OVERFLOW
Language=English
Recursion too deep; the stack overflowed.
.
Language=Russian
Recursion too deep; the stack overflowed.
.
Language=Polish
Zbyt głęboka rekursja, stos przepełniony.
.
Language=Romanian
Recursion too deep; the stack overflowed.
.

MessageId=1002
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGE
Language=English
The window cannot act on the sent message.
.
Language=Russian
The window cannot act on the sent message.
.
Language=Polish
Okno nie może operować na wysłanym komunikacie.
.
Language=Romanian
The window cannot act on the sent message.
.

MessageId=1003
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_COMPLETE
Language=English
Cannot complete this function.
.
Language=Russian
Cannot complete this function.
.
Language=Polish
Nie można ukończyć wykonywania tej funkcji.
.
Language=Romanian
Cannot complete this function.
.

MessageId=1004
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FLAGS
Language=English
Invalid flags.
.
Language=Russian
Invalid flags.
.
Language=Polish
Nieprawidłowe flagi.
.
Language=Romanian
Invalid flags.
.

MessageId=1005
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_VOLUME
Language=English
The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted.
.
Language=Russian
The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted.
.
Language=Polish
Wolumin nie zawiera rozpoznawanego systemu plików. Sprawdź, czy załadowano wszystkie wymagane sterowniki systemu plików i czy wolumin nie jest uszkodzony.
.
Language=Romanian
The volume does not contain a recognized file system. Please make sure that all required file system drivers are loaded and that the volume is not corrupted.
.

MessageId=1006
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_INVALID
Language=English
The volume for a file has been externally altered so that the opened file is no longer valid.
.
Language=Russian
The volume for a file has been externally altered so that the opened file is no longer valid.
.
Language=Polish
Wolumin pliku został zewnętrznie zmieniony w taki sposób, że otwarty plik nie jest już prawidłowy.
.
Language=Romanian
The volume for a file has been externally altered so that the opened file is no longer valid.
.

MessageId=1007
Severity=Success
Facility=System
SymbolicName=ERROR_FULLSCREEN_MODE
Language=English
The requested operation cannot be performed in full-screen mode.
.
Language=Russian
The requested operation cannot be performed in full-screen mode.
.
Language=Polish
Nie można wykonać żądanej operacji w trybie pełnoekranowym.
.
Language=Romanian
The requested operation cannot be performed in full-screen mode.
.

MessageId=1008
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TOKEN
Language=English
An attempt was made to reference a token that does not exist.
.
Language=Russian
An attempt was made to reference a token that does not exist.
.
Language=Polish
Wykonano próbę odwołania się do tokena, który nie istnieje.
.
Language=Romanian
An attempt was made to reference a token that does not exist.
.

MessageId=1009
Severity=Success
Facility=System
SymbolicName=ERROR_BADDB
Language=English
The configuration registry database is corrupt.
.
Language=Russian
The configuration registry database is corrupt.
.
Language=Polish
Baza danych rejestru konfiguracji jest uszkodzona.
.
Language=Romanian
The configuration registry database is corrupt.
.

MessageId=1010
Severity=Success
Facility=System
SymbolicName=ERROR_BADKEY
Language=English
The configuration registry key is invalid.
.
Language=Russian
The configuration registry key is invalid.
.
Language=Polish
Klucz rejestru konfiguracji jest nieprawidłowy.
.
Language=Romanian
The configuration registry key is invalid.
.

MessageId=1011
Severity=Success
Facility=System
SymbolicName=ERROR_CANTOPEN
Language=English
The configuration registry key could not be opened.
.
Language=Russian
The configuration registry key could not be opened.
.
Language=Polish
Nie można otworzyć klucza rejestru konfiguracji.
.
Language=Romanian
The configuration registry key could not be opened.
.

MessageId=1012
Severity=Success
Facility=System
SymbolicName=ERROR_CANTREAD
Language=English
The configuration registry key could not be read.
.
Language=Russian
The configuration registry key could not be read.
.
Language=Polish
Nie można odczytać klucza rejestru konfiguracji.
.
Language=Romanian
The configuration registry key could not be read.
.

MessageId=1013
Severity=Success
Facility=System
SymbolicName=ERROR_CANTWRITE
Language=English
The configuration registry key could not be written.
.
Language=Russian
The configuration registry key could not be written.
.
Language=Polish
Nie można zapisać klucza rejestru konfiguracji.
.
Language=Romanian
The configuration registry key could not be written.
.

MessageId=1014
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_RECOVERED
Language=English
One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.
.
Language=Russian
One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.
.
Language=Polish
Jeden z plików w bazie danych rejestru musiał zostać odzyskany za pomocą dziennika lub kopii alternatywnej. Odzyskiwanie zakończyło się pomyślnie.
.
Language=Romanian
One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful.
.

MessageId=1015
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_CORRUPT
Language=English
The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.
.
Language=Russian
The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.
.
Language=Polish
Rejestr jest uszkodzony. Uszkodzona jest struktura jednego z plików zawierających dane Rejestru, uszkodzony jest systemowy obraz pliku w pamięci lub też nie można odzyskać pliku, ponieważ alternatywna kopia lub dziennik były nieobecne lub uszkodzone.
.
Language=Romanian
The registry is corrupted. The structure of one of the files containing registry data is corrupted, or the system's memory image of the file is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted.
.

MessageId=1016
Severity=Success
Facility=System
SymbolicName=ERROR_REGISTRY_IO_FAILED
Language=English
An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.
.
Language=Russian
An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.
.
Language=Polish
Operacja We/Wy zainicjowana przez rejestr nie powiodła się w sposób nieodwracalny. Rejestr nie może wczytać, wypisać lub opróżnić jednego z plików zawierających obraz rejestru systemu.
.
Language=Romanian
An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry.
.

MessageId=1017
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_REGISTRY_FILE
Language=English
The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.
.
Language=Russian
The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.
.
Language=Polish
System próbował załadować lub przywrócić plik do Rejestru, ale określony plik nie jest w formacie plików Rejestru.
.
Language=Romanian
The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format.
.

MessageId=1018
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_DELETED
Language=English
Illegal operation attempted on a registry key that has been marked for deletion.
.
Language=Russian
Illegal operation attempted on a registry key that has been marked for deletion.
.
Language=Polish
Wykonano próbę niedozwolonej operacji na kluczu Rejestru, który został oznaczony do usunięcia.
.
Language=Romanian
Illegal operation attempted on a registry key that has been marked for deletion.
.

MessageId=1019
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOG_SPACE
Language=English
System could not allocate the required space in a registry log.
.
Language=Russian
System could not allocate the required space in a registry log.
.
Language=Polish
System nie mógł przydzielić wymaganego miejsca w dzienniku Rejestru.
.
Language=Romanian
System could not allocate the required space in a registry log.
.

MessageId=1020
Severity=Success
Facility=System
SymbolicName=ERROR_KEY_HAS_CHILDREN
Language=English
Cannot create a symbolic link in a registry key that already has subkeys or values.
.
Language=Russian
Cannot create a symbolic link in a registry key that already has subkeys or values.
.
Language=Polish
W Rejestrze kluczy, który ma już podklucze lub wartości, nie można utworzyć łącza symbolicznego.
.
Language=Romanian
Cannot create a symbolic link in a registry key that already has subkeys or values.
.

MessageId=1021
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_MUST_BE_VOLATILE
Language=English
Cannot create a stable subkey under a volatile parent key.
.
Language=Russian
Cannot create a stable subkey under a volatile parent key.
.
Language=Polish
Dla usuwalnego klucza nadrzędnego nie można utworzyć trwałego podklucza.
.
Language=Romanian
Cannot create a stable subkey under a volatile parent key.
.

MessageId=1022
Severity=Success
Facility=System
SymbolicName=ERROR_NOTIFY_ENUM_DIR
Language=English
A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.
.
Language=Russian
A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.
.
Language=Polish
Żądanie zmiany powiadomienia jest już kończone, a informacje nie są zwracane do buforu komputera wywołującego. Wywołujący musi teraz wyliczyć pliki w celu odnalezienia zmian.
.
Language=Romanian
A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes.
.

MessageId=1051
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENT_SERVICES_RUNNING
Language=English
A stop control has been sent to a service that other running services are dependent on.
.
Language=Russian
A stop control has been sent to a service that other running services are dependent on.
.
Language=Polish
Sygnał kontrolny Stop został wysłany do usługi, od której są zależne inne działające usługi.
.
Language=Romanian
A stop control has been sent to a service that other running services are dependent on.
.

MessageId=1052
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_CONTROL
Language=English
The requested control is not valid for this service.
.
Language=Russian
The requested control is not valid for this service.
.
Language=Polish
Żądany sygnał sterujący jest nieprawidłowy dla tej usługi.
.
Language=Romanian
The requested control is not valid for this service.
.

MessageId=1053
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_REQUEST_TIMEOUT
Language=English
The service did not respond to the start or control request in a timely fashion.
.
Language=Russian
The service did not respond to the start or control request in a timely fashion.
.
Language=Polish
Usługa nie odpowiada na sygnał uruchomienia lub sygnał sterujący w oczekiwanym czasie.
.
Language=Romanian
The service did not respond to the start or control request in a timely fashion.
.

MessageId=1054
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NO_THREAD
Language=English
A thread could not be created for the service.
.
Language=Russian
A thread could not be created for the service.
.
Language=Polish
Nie można utworzyć wątku dla tej usługi.
.
Language=Romanian
A thread could not be created for the service.
.

MessageId=1055
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DATABASE_LOCKED
Language=English
The service database is locked.
.
Language=Russian
The service database is locked.
.
Language=Polish
Baza danych usługi jest zablokowana.
.
Language=Romanian
The service database is locked.
.

MessageId=1056
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_ALREADY_RUNNING
Language=English
An instance of the service is already running.
.
Language=Russian
An instance of the service is already running.
.
Language=Polish
Jedno wystąpienie usługi już działa.
.
Language=Romanian
An instance of the service is already running.
.

MessageId=1057
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_ACCOUNT
Language=English
The account name is invalid or does not exist, or the password is invalid for the account name specified.
.
Language=Russian
The account name is invalid or does not exist, or the password is invalid for the account name specified.
.
Language=Polish
Nazwa konta jest nieprawidłowa lub nie istnieje albo hasło dla podanej nazwy konta jest nieprawidłowe.
.
Language=Romanian
The account name is invalid or does not exist, or the password is invalid for the account name specified.
.

MessageId=1058
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DISABLED
Language=English
The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.
.
Language=Russian
The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.
.
Language=Polish
Nie można uruchomić określonej usługi, ponieważ jest ona wyłączona lub ponieważ nie są włączone skojarzone z nią urządzenia.
.
Language=Romanian
The service cannot be started, either because it is disabled or because it has no enabled devices associated with it.
.

MessageId=1059
Severity=Success
Facility=System
SymbolicName=ERROR_CIRCULAR_DEPENDENCY
Language=English
Circular service dependency was specified.
.
Language=Russian
Circular service dependency was specified.
.
Language=Polish
Określono cykliczną zależność usługi.
.
Language=Romanian
Circular service dependency was specified.
.

MessageId=1060
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DOES_NOT_EXIST
Language=English
The specified service does not exist as an installed service.
.
Language=Russian
The specified service does not exist as an installed service.
.
Language=Polish
Określona usługa nie istnieje jako usługa zainstalowana.
.
Language=Romanian
The specified service does not exist as an installed service.
.

MessageId=1061
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_CANNOT_ACCEPT_CTRL
Language=English
The service cannot accept control messages at this time.
.
Language=Russian
The service cannot accept control messages at this time.
.
Language=Polish
Usługa nie może teraz zaakceptować komunikatów sterujących.
.
Language=Romanian
The service cannot accept control messages at this time.
.

MessageId=1062
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_ACTIVE
Language=English
The service has not been started.
.
Language=Russian
The service has not been started.
.
Language=Polish
Usługa nie została uruchomiona.
.
Language=Romanian
The service has not been started.
.

MessageId=1063
Severity=Success
Facility=System
SymbolicName=ERROR_FAILED_SERVICE_CONTROLLER_CONNECT
Language=English
The service process could not connect to the service controller.
.
Language=Russian
The service process could not connect to the service controller.
.
Language=Polish
Proces usługi nie mógł połączyć się z kontrolerem usługi.
.
Language=Romanian
The service process could not connect to the service controller.
.

MessageId=1064
Severity=Success
Facility=System
SymbolicName=ERROR_EXCEPTION_IN_SERVICE
Language=English
An exception occurred in the service when handling the control request.
.
Language=Russian
An exception occurred in the service when handling the control request.
.
Language=Polish
W usłudze wystąpił wyjątek podczas obsługi żądania kontroli.
.
Language=Romanian
An exception occurred in the service when handling the control request.
.

MessageId=1065
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_DOES_NOT_EXIST
Language=English
The database specified does not exist.
.
Language=Russian
The database specified does not exist.
.
Language=Polish
Określona baza danych nie istnieje.
.
Language=Romanian
The database specified does not exist.
.

MessageId=1066
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_SPECIFIC_ERROR
Language=English
The service has returned a service-specific error code.
.
Language=Russian
The service has returned a service-specific error code.
.
Language=Polish
Usługa zwróciła kod błędu specyficzny dla tej usługi.
.
Language=Romanian
The service has returned a service-specific error code.
.

MessageId=1067
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_ABORTED
Language=English
The process terminated unexpectedly.
.
Language=Russian
The process terminated unexpectedly.
.
Language=Polish
Proces zakończył się nieoczekiwanie.
.
Language=Romanian
The process terminated unexpectedly.
.

MessageId=1068
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_FAIL
Language=English
The dependency service or group failed to start.
.
Language=Russian
The dependency service or group failed to start.
.
Language=Polish
Uruchomienie usługi zależności lub grupy nie powiodło się.
.
Language=Romanian
The dependency service or group failed to start.
.

MessageId=1069
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_LOGON_FAILED
Language=English
The service did not start due to a logon failure.
.
Language=Russian
The service did not start due to a logon failure.
.
Language=Polish
Usługa nie została uruchomiona z powodu nieudanego logowania.
.
Language=Romanian
The service did not start due to a logon failure.
.

MessageId=1070
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_START_HANG
Language=English
After starting, the service hung in a start-pending state.
.
Language=Russian
After starting, the service hung in a start-pending state.
.
Language=Polish
Po uruchomieniu usługa uległa zawieszeniu w stanie startowym.
.
Language=Romanian
After starting, the service hung in a start-pending state.
.

MessageId=1071
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICE_LOCK
Language=English
The specified service database lock is invalid.
.
Language=Russian
The specified service database lock is invalid.
.
Language=Polish
Określona blokada bazy danych usługi jest nieprawidłowa.
.
Language=Romanian
The specified service database lock is invalid.
.

MessageId=1072
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_MARKED_FOR_DELETE
Language=English
The specified service has been marked for deletion.
.
Language=Russian
The specified service has been marked for deletion.
.
Language=Polish
Określona usługa została oznaczona do usunięcia.
.
Language=Romanian
The specified service has been marked for deletion.
.

MessageId=1073
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_EXISTS
Language=English
The specified service already exists.
.
Language=Russian
The specified service already exists.
.
Language=Polish
Określona usługa już istnieje.
.
Language=Romanian
The specified service already exists.
.

MessageId=1074
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_RUNNING_LKG
Language=English
The system is currently running with the last-known-good configuration.
.
Language=Russian
The system is currently running with the last-known-good configuration.
.
Language=Polish
System działa obecnie w ostatniej znanej dobrej konfiguracji.
.
Language=Romanian
The system is currently running with the last-known-good configuration.
.

MessageId=1075
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_DEPENDENCY_DELETED
Language=English
The dependency service does not exist or has been marked for deletion.
.
Language=Russian
The dependency service does not exist or has been marked for deletion.
.
Language=Polish
Usługa zależności nie istnieje lub została oznaczona do usunięcia.
.
Language=Romanian
The dependency service does not exist or has been marked for deletion.
.

MessageId=1076
Severity=Success
Facility=System
SymbolicName=ERROR_BOOT_ALREADY_ACCEPTED
Language=English
The current boot has already been accepted for use as the last-known-good control set.
.
Language=Russian
The current boot has already been accepted for use as the last-known-good control set.
.
Language=Polish
Bieżące uruchomienie zostało już zaakceptowane do użycia jako ostatni znany dobry zestaw sterujący.
.
Language=Romanian
The current boot has already been accepted for use as the last-known-good control set.
.

MessageId=1077
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NEVER_STARTED
Language=English
No attempts to start the service have been made since the last boot.
.
Language=Russian
No attempts to start the service have been made since the last boot.
.
Language=Polish
Od czasu ostatniego uruchomienia komputera nie podejmowano prób uruchomienia usługi.
.
Language=Romanian
No attempts to start the service have been made since the last boot.
.

MessageId=1078
Severity=Success
Facility=System
SymbolicName=ERROR_DUPLICATE_SERVICE_NAME
Language=English
The name is already in use as either a service name or a service display name.
.
Language=Russian
The name is already in use as either a service name or a service display name.
.
Language=Polish
Nazwa jest już w użyciu jako nazwa usługi lub wyświetlana nazwa usługi.
.
Language=Romanian
The name is already in use as either a service name or a service display name.
.

MessageId=1079
Severity=Success
Facility=System
SymbolicName=ERROR_DIFFERENT_SERVICE_ACCOUNT
Language=English
The account specified for this service is different from the account specified for other services running in the same process.
.
Language=Russian
The account specified for this service is different from the account specified for other services running in the same process.
.
Language=Polish
Konto podane dla tej usługi różni się od konta podanego dla innych usług działających w tym samym procesie.
.
Language=Romanian
The account specified for this service is different from the account specified for other services running in the same process.
.

MessageId=1080
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_DETECT_DRIVER_FAILURE
Language=English
Failure actions can only be set for Win32 services, not for drivers.
.
Language=Russian
Failure actions can only be set for Win32 services, not for drivers.
.
Language=Polish
Akcje przypisane do błędów można ustawić tylko dla usług Win32, a nie dla sterowników.
.
Language=Romanian
Failure actions can only be set for Win32 services, not for drivers.
.

MessageId=1081
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_DETECT_PROCESS_ABORT
Language=English
This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.
.
Language=Russian
This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.
.
Language=Polish
Ta usługa działa w tym samym procesie, co Menedżer sterowania usługami. Z tego powodu Menedżer sterowania usługami nie będzie mógł podjąć działań, gdy proces tej usługi niespodziewanie się zakończy.
.
Language=Romanian
This service runs in the same process as the service control manager. Therefore, the service control manager cannot take action if this service's process terminates unexpectedly.
.

MessageId=1082
Severity=Success
Facility=System
SymbolicName=ERROR_NO_RECOVERY_PROGRAM
Language=English
No recovery program has been configured for this service.
.
Language=Russian
No recovery program has been configured for this service.
.
Language=Polish
Żaden program odzyskiwania nie został skonfigurowany dla tej usługi.
.
Language=Romanian
No recovery program has been configured for this service.
.

MessageId=1083
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_IN_EXE
Language=English
The executable program that this service is configured to run in does not implement the service.
.
Language=Russian
The executable program that this service is configured to run in does not implement the service.
.
Language=Polish
Program wykonywalny, w którym ta usługa (zgodnie z jej konfiguracją) ma być uruchomiona, nie implementuje usługi.
.
Language=Romanian
The executable program that this service is configured to run in does not implement the service.
.

MessageId=1084
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SAFEBOOT_SERVICE
Language=English
This service cannot be started in Safe Mode.
.
Language=Russian
This service cannot be started in Safe Mode.
.
Language=Polish
Tej usługi nie można uruchomić w trybie awaryjnym
.
Language=Romanian
This service cannot be started in Safe Mode.
.

MessageId=1100
Severity=Success
Facility=System
SymbolicName=ERROR_END_OF_MEDIA
Language=English
The physical end of the tape has been reached.
.
Language=Russian
The physical end of the tape has been reached.
.
Language=Polish
Osiągnięto fizyczny koniec taśmy.
.
Language=Romanian
The physical end of the tape has been reached.
.

MessageId=1101
Severity=Success
Facility=System
SymbolicName=ERROR_FILEMARK_DETECTED
Language=English
A tape access reached a filemark.
.
Language=Russian
A tape access reached a filemark.
.
Language=Polish
Osiągnięto znacznik pliku na taśmie.
.
Language=Romanian
A tape access reached a filemark.
.

MessageId=1102
Severity=Success
Facility=System
SymbolicName=ERROR_BEGINNING_OF_MEDIA
Language=English
The beginning of the tape or a partition was encountered.
.
Language=Russian
The beginning of the tape or a partition was encountered.
.
Language=Polish
Napotkano początek taśmy lub partycji.
.
Language=Romanian
The beginning of the tape or a partition was encountered.
.

MessageId=1103
Severity=Success
Facility=System
SymbolicName=ERROR_SETMARK_DETECTED
Language=English
A tape access reached the end of a set of files.
.
Language=Russian
A tape access reached the end of a set of files.
.
Language=Polish
Osiągnięto koniec zestawu plików na taśmie.
.
Language=Romanian
A tape access reached the end of a set of files.
.

MessageId=1104
Severity=Success
Facility=System
SymbolicName=ERROR_NO_DATA_DETECTED
Language=English
No more data is on the tape.
.
Language=Russian
No more data is on the tape.
.
Language=Polish
Na taśmie brak dalszych danych.
.
Language=Romanian
No more data is on the tape.
.

MessageId=1105
Severity=Success
Facility=System
SymbolicName=ERROR_PARTITION_FAILURE
Language=English
Tape could not be partitioned.
.
Language=Russian
Tape could not be partitioned.
.
Language=Polish
Nie można podzielić taśmy na partycje.
.
Language=Romanian
Tape could not be partitioned.
.

MessageId=1106
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BLOCK_LENGTH
Language=English
When accessing a new tape of a multivolume partition, the current block size is incorrect.
.
Language=Russian
When accessing a new tape of a multivolume partition, the current block size is incorrect.
.
Language=Polish
W trakcie uzyskiwania dostępu do nowej taśmy w partycji o wielu woluminach, bieżący rozmiar bloku jest niepoprawny.
.
Language=Romanian
When accessing a new tape of a multivolume partition, the current block size is incorrect.
.

MessageId=1107
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_PARTITIONED
Language=English
Tape partition information could not be found when loading a tape.
.
Language=Russian
Tape partition information could not be found when loading a tape.
.
Language=Polish
Podczas ładowania taśmy nie odnaleziono informacji o jej partycjach.
.
Language=Romanian
Tape partition information could not be found when loading a tape.
.

MessageId=1108
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_LOCK_MEDIA
Language=English
Unable to lock the media eject mechanism.
.
Language=Russian
Unable to lock the media eject mechanism.
.
Language=Polish
Nie można zablokować mechanizmu wysuwu nośnika.
.
Language=Romanian
Unable to lock the media eject mechanism.
.

MessageId=1109
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_UNLOAD_MEDIA
Language=English
Unable to unload the media.
.
Language=Russian
Unable to unload the media.
.
Language=Polish
Nie można usunąć nośnika z pamięci.
.
Language=Romanian
Unable to unload the media.
.

MessageId=1110
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_CHANGED
Language=English
The media in the drive may have changed.
.
Language=Russian
The media in the drive may have changed.
.
Language=Polish
Nośnik w stacji mógł się zmienić.
.
Language=Romanian
The media in the drive may have changed.
.

MessageId=1111
Severity=Success
Facility=System
SymbolicName=ERROR_BUS_RESET
Language=English
The I/O bus was reset.
.
Language=Russian
The I/O bus was reset.
.
Language=Polish
Magistrala We/Wy została zresetowana.
.
Language=Romanian
The I/O bus was reset.
.

MessageId=1112
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MEDIA_IN_DRIVE
Language=English
No media in drive.
.
Language=Russian
No media in drive.
.
Language=Polish
Brak nośnika w stacji.
.
Language=Romanian
No media in drive.
.

MessageId=1113
Severity=Success
Facility=System
SymbolicName=ERROR_NO_UNICODE_TRANSLATION
Language=English
No mapping for the Unicode character exists in the target multi-byte code page.
.
Language=Russian
No mapping for the Unicode character exists in the target multi-byte code page.
.
Language=Polish
Brak mapowania dla tego znaku Unicode w docelowej wielobajtowej stronie kodowej.
.
Language=Romanian
No mapping for the Unicode character exists in the target multi-byte code page.
.

MessageId=1114
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_INIT_FAILED
Language=English
A dynamic link library (DLL) initialization routine failed.
.
Language=Russian
A dynamic link library (DLL) initialization routine failed.
.
Language=Polish
Procedura inicjowania biblioteki dołączanej dynamicznie (DLL) nie powiodła się.
.
Language=Romanian
A dynamic link library (DLL) initialization routine failed.
.

MessageId=1115
Severity=Success
Facility=System
SymbolicName=ERROR_SHUTDOWN_IN_PROGRESS
Language=English
A system shutdown is in progress.
.
Language=Russian
A system shutdown is in progress.
.
Language=Polish
Trwa proces zamykania systemu.
.
Language=Romanian
A system shutdown is in progress.
.

MessageId=1116
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SHUTDOWN_IN_PROGRESS
Language=English
Unable to abort the system shutdown because no shutdown was in progress.
.
Language=Russian
Unable to abort the system shutdown because no shutdown was in progress.
.
Language=Polish
Nie można przerwać procesu zamykania systemu, ponieważ taki proces nie jest w toku.
.
Language=Romanian
Unable to abort the system shutdown because no shutdown was in progress.
.

MessageId=1117
Severity=Success
Facility=System
SymbolicName=ERROR_IO_DEVICE
Language=English
The request could not be performed because of an I/O device error.
.
Language=Russian
The request could not be performed because of an I/O device error.
.
Language=Polish
Nie można wykonać żądania z powodu błędu urządzenia We/Wy.
.
Language=Romanian
The request could not be performed because of an I/O device error.
.

MessageId=1118
Severity=Success
Facility=System
SymbolicName=ERROR_SERIAL_NO_DEVICE
Language=English
No serial device was successfully initialized. The serial driver will unload.
.
Language=Russian
No serial device was successfully initialized. The serial driver will unload.
.
Language=Polish
Żadne urządzenie szeregowe nie zostało pomyślnie zainicjowane. Sterownik szeregowy zostanie usunięty z pamięci.
.
Language=Romanian
No serial device was successfully initialized. The serial driver will unload.
.

MessageId=1119
Severity=Success
Facility=System
SymbolicName=ERROR_IRQ_BUSY
Language=English
Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.
.
Language=Russian
Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.
.
Language=Polish
Nie można otworzyć urządzenia, które współużytkowało przerwanie (IRQ) z innymi urządzeniami. Co najmniej jedno inne urządzenie używające tego IRQ zostało już otwarte.
.
Language=Romanian
Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.
.

MessageId=1120
Severity=Success
Facility=System
SymbolicName=ERROR_MORE_WRITES
Language=English
A serial I/O operation was completed by another write to the serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.)
.
Language=Russian
A serial I/O operation was completed by another write to the serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.)
.
Language=Polish
Operacja szeregowego We/Wy została zakończona przez inny zapis do portu szeregowego. (Licznik IOCTL_SERIAL_XOFF_COUNTER osiągnął zero.)
.
Language=Romanian
A serial I/O operation was completed by another write to the serial port. (The IOCTL_SERIAL_XOFF_COUNTER reached zero.)
.

MessageId=1121
Severity=Success
Facility=System
SymbolicName=ERROR_COUNTER_TIMEOUT
Language=English
A serial I/O operation completed because the timeout period expired. (The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)
.
Language=Russian
A serial I/O operation completed because the timeout period expired. (The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)
.
Language=Polish
Operacja szeregowego We/Wy została zakończona z powodu przekroczenia limitu czasu. (Licznik IOCTL_SERIAL_XOFF_COUNTER nie osiągnął wartości zero.)
.
Language=Romanian
A serial I/O operation completed because the timeout period expired. (The IOCTL_SERIAL_XOFF_COUNTER did not reach zero.)
.

MessageId=1122
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_ID_MARK_NOT_FOUND
Language=English
No ID address mark was found on the floppy disk.
.
Language=Russian
No ID address mark was found on the floppy disk.
.
Language=Polish
Na dyskietce nie znaleziono znacznika adresu identyfikatora.
.
Language=Romanian
No ID address mark was found on the floppy disk.
.

MessageId=1123
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_WRONG_CYLINDER
Language=English
Mismatch between the floppy disk sector ID field and the floppy disk controller track address.
.
Language=Russian
Mismatch between the floppy disk sector ID field and the floppy disk controller track address.
.
Language=Polish
Niedopasowanie między polem identyfikatora sektora dyskietki i adresem ścieżki kontrolera stacji dyskietek.
.
Language=Romanian
Mismatch between the floppy disk sector ID field and the floppy disk controller track address.
.

MessageId=1124
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_UNKNOWN_ERROR
Language=English
The floppy disk controller reported an error that is not recognized by the floppy disk driver.
.
Language=Russian
The floppy disk controller reported an error that is not recognized by the floppy disk driver.
.
Language=Polish
Kontroler stacji dyskietek zgłosił błąd, który nie został rozpoznany przez sterownik stacji dyskietek.
.
Language=Romanian
The floppy disk controller reported an error that is not recognized by the floppy disk driver.
.

MessageId=1125
Severity=Success
Facility=System
SymbolicName=ERROR_FLOPPY_BAD_REGISTERS
Language=English
The floppy disk controller returned inconsistent results in its registers.
.
Language=Russian
The floppy disk controller returned inconsistent results in its registers.
.
Language=Polish
Kontroler stacji dyskietek zwrócił w swych rejestrach niezgodne wyniki.
.
Language=Romanian
The floppy disk controller returned inconsistent results in its registers.
.

MessageId=1126
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RECALIBRATE_FAILED
Language=English
While accessing the hard disk, a recalibrate operation failed, even after retries.
.
Language=Russian
While accessing the hard disk, a recalibrate operation failed, even after retries.
.
Language=Polish
Podczas uzyskiwania dostępu do dysku twardego operacja rekalibracji nie powiodła się, mimo ponawiania prób.
.
Language=Romanian
While accessing the hard disk, a recalibrate operation failed, even after retries.
.

MessageId=1127
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_OPERATION_FAILED
Language=English
While accessing the hard disk, a disk operation failed even after retries.
.
Language=Russian
While accessing the hard disk, a disk operation failed even after retries.
.
Language=Polish
Podczas uzyskiwania dostępu do dysku twardego operacja dyskowa nie powiodła się, mimo ponawiania prób.
.
Language=Romanian
While accessing the hard disk, a disk operation failed even after retries.
.

MessageId=1128
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_RESET_FAILED
Language=English
While accessing the hard disk, a disk controller reset was needed, but even that failed.
.
Language=Russian
While accessing the hard disk, a disk controller reset was needed, but even that failed.
.
Language=Polish
Podczas uzyskiwania dostępu do dysku twardego, niezbędna była operacja resetowania kontrolera dysku; nawet to nie przyniosło oczekiwanego rezultatu.
.
Language=Romanian
While accessing the hard disk, a disk controller reset was needed, but even that failed.
.

MessageId=1129
Severity=Success
Facility=System
SymbolicName=ERROR_EOM_OVERFLOW
Language=English
Physical end of tape encountered.
.
Language=Russian
Physical end of tape encountered.
.
Language=Polish
Napotkano fizyczny koniec taśmy.
.
Language=Romanian
Physical end of tape encountered.
.

MessageId=1130
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_SERVER_MEMORY
Language=English
Not enough server storage is available to process this command.
.
Language=Russian
Not enough server storage is available to process this command.
.
Language=Polish
Za mało pamięci serwera do przetworzenia tego polecenia.
.
Language=Romanian
Not enough server storage is available to process this command.
.

MessageId=1131
Severity=Success
Facility=System
SymbolicName=ERROR_POSSIBLE_DEADLOCK
Language=English
A potential deadlock condition has been detected.
.
Language=Russian
A potential deadlock condition has been detected.
.
Language=Polish
Wykryto możliwość wystąpienia stanu zakleszczenia (deadlock).
.
Language=Romanian
A potential deadlock condition has been detected.
.

MessageId=1132
Severity=Success
Facility=System
SymbolicName=ERROR_MAPPED_ALIGNMENT
Language=English
The base address or the file offset specified does not have the proper alignment.
.
Language=Russian
The base address or the file offset specified does not have the proper alignment.
.
Language=Polish
Adres bazowy określonego offsetu pliku nie ma odpowiedniego wyrównania.
.
Language=Romanian
The base address or the file offset specified does not have the proper alignment.
.

MessageId=1140
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_VETOED
Language=English
An attempt to change the system power state was vetoed by another application or driver.
.
Language=Russian
An attempt to change the system power state was vetoed by another application or driver.
.
Language=Polish
Próba zmiany stanu zasilania systemu została zablokowana przez inną aplikację lub sterownik.
.
Language=Romanian
An attempt to change the system power state was vetoed by another application or driver.
.

MessageId=1141
Severity=Success
Facility=System
SymbolicName=ERROR_SET_POWER_STATE_FAILED
Language=English
The system BIOS failed an attempt to change the system power state.
.
Language=Russian
The system BIOS failed an attempt to change the system power state.
.
Language=Polish
Próba zmiany stanu zasilania systemu przez systemowy BIOS nie powiodła się.
.
Language=Romanian
The system BIOS failed an attempt to change the system power state.
.

MessageId=1142
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LINKS
Language=English
An attempt was made to create more links on a file than the file system supports.
.
Language=Russian
An attempt was made to create more links on a file than the file system supports.
.
Language=Polish
Podjęto próbę utworzenia większej liczby łączy na pliku niż obsługuje system plików.
.
Language=Romanian
An attempt was made to create more links on a file than the file system supports.
.

MessageId=1150
Severity=Success
Facility=System
SymbolicName=ERROR_OLD_WIN_VERSION
Language=English
The specified program requires a newer version of ReactOS.
.
Language=Russian
The specified program requires a newer version of ReactOS.
.
Language=Polish
Określony program wymaga nowszej wersji systemu ReactOS.
.
Language=Romanian
The specified program requires a newer version of ReactOS.
.

MessageId=1151
Severity=Success
Facility=System
SymbolicName=ERROR_APP_WRONG_OS
Language=English
The specified program is not a Windows or MS-DOS program.
.
Language=Russian
The specified program is not a Windows or MS-DOS program.
.
Language=Polish
Określony program nie jest programem środowiska Windows ani MS-DOS.
.
Language=Romanian
The specified program is not a Windows or MS-DOS program.
.

MessageId=1152
Severity=Success
Facility=System
SymbolicName=ERROR_SINGLE_INSTANCE_APP
Language=English
Cannot start more than one instance of the specified program.
.
Language=Russian
Cannot start more than one instance of the specified program.
.
Language=Polish
Nie można uruchomić więcej niż jednego wystąpienia określonego programu.
.
Language=Romanian
Cannot start more than one instance of the specified program.
.

MessageId=1153
Severity=Success
Facility=System
SymbolicName=ERROR_RMODE_APP
Language=English
The specified program was written for an earlier version of ReactOS.
.
Language=Russian
The specified program was written for an earlier version of ReactOS.
.
Language=Polish
Określony program został napisany dla starszej wersji systemu ReactOS.
.
Language=Romanian
The specified program was written for an earlier version of ReactOS.
.

MessageId=1154
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DLL
Language=English
One of the library files needed to run this application is damaged.
.
Language=Russian
One of the library files needed to run this application is damaged.
.
Language=Polish
Jeden z plików bibliotek potrzebnych do uruchomienia tej aplikacji jest uszkodzony.
.
Language=Romanian
One of the library files needed to run this application is damaged.
.

MessageId=1155
Severity=Success
Facility=System
SymbolicName=ERROR_NO_ASSOCIATION
Language=English
No application is associated with the specified file for this operation.
.
Language=Russian
No application is associated with the specified file for this operation.
.
Language=Polish
Z określonym plikiem nie skojarzono dla tej operacji żadnej aplikacji.
.
Language=Romanian
No application is associated with the specified file for this operation.
.

MessageId=1156
Severity=Success
Facility=System
SymbolicName=ERROR_DDE_FAIL
Language=English
An error occurred in sending the command to the application.
.
Language=Russian
Ошибка при пересылке команды приложению.
.
Language=Polish
W trakcie wysyłania polecenia do aplikacji wystąpił błąd.
.
Language=Romanian
An error occurred in sending the command to the application.
.

MessageId=1157
Severity=Success
Facility=System
SymbolicName=ERROR_DLL_NOT_FOUND
Language=English
One of the library files needed to run this application cannot be found.
.
Language=Russian
Не найден один из файлов библиотек, необходимых для выполнения данного приложения.
.
Language=Polish
Nie można odnaleźć jednego z plików bibliotek potrzebnych do uruchomienia tej aplikacji.
.
Language=Romanian
One of the library files needed to run this application cannot be found.
.

MessageId=1158
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_USER_HANDLES
Language=English
The current process has used all of its system allowance of handles for Window Manager objects.
.
Language=Russian
Текущий процесс использовал все системные разрешения по управлению объектами диспетчера окон.
.
Language=Polish
Bieżący proces wykorzystał wszystkie dozwolone przez system dojścia do obiektów Window Manager.
.
Language=Romanian
The current process has used all of its system allowance of handles for Window Manager objects.
.

MessageId=1159
Severity=Success
Facility=System
SymbolicName=ERROR_MESSAGE_SYNC_ONLY
Language=English
The message can be used only with synchronous operations.
.
Language=Russian
Сообщение может быть использовано только с операциями синхронизации.
.
Language=Polish
Komunikat może być użyty tylko przy operacjach synchronicznych.
.
Language=Romanian
The message can be used only with synchronous operations.
.

MessageId=1160
Severity=Success
Facility=System
SymbolicName=ERROR_SOURCE_ELEMENT_EMPTY
Language=English
The indicated source element has no media.
.
Language=Russian
Указанный исходный элемент не имеет носителя.
.
Language=Polish
Wskazany element źródłowy nie ma nośnika.
.
Language=Romanian
The indicated source element has no media.
.

MessageId=1161
Severity=Success
Facility=System
SymbolicName=ERROR_DESTINATION_ELEMENT_FULL
Language=English
The indicated destination element already contains media.
.
Language=Russian
Указанный конечный элемент уже содержит носитель.
.
Language=Polish
Wskazany element docelowy już ma nośnik.
.
Language=Romanian
The indicated destination element already contains media.
.

MessageId=1162
Severity=Success
Facility=System
SymbolicName=ERROR_ILLEGAL_ELEMENT_ADDRESS
Language=English
The indicated element does not exist.
.
Language=Russian
Указанный элемент не существует.
.
Language=Polish
Wskazany element nie istnieje.
.
Language=Romanian
The indicated element does not exist.
.

MessageId=1163
Severity=Success
Facility=System
SymbolicName=ERROR_MAGAZINE_NOT_PRESENT
Language=English
The indicated element is part of a magazine that is not present.
.
Language=Russian
Указанный элемент является частью отсутствующего журнала.
.
Language=Polish
Wskazany element stanowi część magazynu, którego nie ma.
.
Language=Romanian
The indicated element is part of a magazine that is not present.
.

MessageId=1164
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REINITIALIZATION_NEEDED
Language=English
The indicated device requires reinitialization due to hardware errors.
.
Language=Russian
Указанный элемент требует повторной инициализации из-за аппаратных ошибок.
.
Language=Polish
Wskazane urządzenie wymaga ponownego zainicjowania wskutek błędów sprzętowych.
.
Language=Romanian
The indicated device requires reinitialization due to hardware errors.
.

MessageId=1165
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REQUIRES_CLEANING
Language=English
The device has indicated that cleaning is required before further operations are attempted.
.
Language=Russian
Устройство требует проведение чистки перед его дальнейшим использованием.
.
Language=Polish
Urządzenie sygnalizuje, że przed dalszymi operacjami jest wymagane czyszczenie.
.
Language=Romanian
The device has indicated that cleaning is required before further operations are attempted.
.

MessageId=1166
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_DOOR_OPEN
Language=English
The device has indicated that its door is open.
.
Language=Russian
Устройство сообщает, что открыта дверца.
.
Language=Polish
Urządzenie sygnalizuje, że jest otwarte.
.
Language=Romanian
The device has indicated that its door is open.
.

MessageId=1167
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_CONNECTED
Language=English
The device is not connected.
.
Language=Russian
Устройство не подключено.
.
Language=Polish
Urządzenie nie jest podłączone.
.
Language=Romanian
The device is not connected.
.

MessageId=1168
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_FOUND
Language=English
Element not found.
.
Language=Russian
Элемент не найден.
.
Language=Polish
Nie można odnaleźć elementu.
.
Language=Romanian
Element not found.
.

MessageId=1169
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MATCH
Language=English
There was no match for the specified key in the index.
.
Language=Russian
В индексе не найдены соответствия указанному ключу.
.
Language=Polish
W indeksie nie znaleziono pozycji odpowiadającej podanemu kluczowi.
.
Language=Romanian
There was no match for the specified key in the index.
.

MessageId=1170
Severity=Success
Facility=System
SymbolicName=ERROR_SET_NOT_FOUND
Language=English
The property set specified does not exist on the object.
.
Language=Russian
Указанный набор свойств не существует для объекта.
.
Language=Polish
Określony zestaw właściwości nie istnieje na tym obiekcie.
.
Language=Romanian
The property set specified does not exist on the object.
.

MessageId=1171
Severity=Success
Facility=System
SymbolicName=ERROR_POINT_NOT_FOUND
Language=English
The point passed to GetMouseMovePointsEx is not in the buffer.
.
Language=Russian
Переданная в GetMouseMovePoints точка не находится в буфере.
.
Language=Polish
Punkt przekazany do instrukcji GetMouseMovePoints nie znajduje się w buforze.
.
Language=Romanian
The point passed to GetMouseMovePointsEx is not in the buffer.
.

MessageId=1172
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRACKING_SERVICE
Language=English
The tracking (workstation) service is not running.
.
Language=Russian
Служба слежения (на рабочей станции) не запущена.
.
Language=Polish
Usługa śledzenia (stacja robocza) nie jest uruchomiona.
.
Language=Romanian
The tracking (workstation) service is not running.
.

MessageId=1173
Severity=Success
Facility=System
SymbolicName=ERROR_NO_VOLUME_ID
Language=English
The Volume ID could not be found.
.
Language=Russian
Не удается найти идентификатор тома.
.
Language=Polish
Nie można znaleźć identyfikatora woluminu.
.
Language=Romanian
The Volume ID could not be found.
.

MessageId=1175
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_REMOVE_REPLACED
Language=English
Unable to remove the file to be replaced.
.
Language=Russian
Не удается удалить заменяемый файл.
.
Language=Polish
Nie można usunąć pliku, który ma być zastąpiony.
.
Language=Romanian
Unable to remove the file to be replaced.
.

MessageId=1176
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT
Language=English
Unable to move the replacement file to the file to be replaced. The file to be replaced has retained its original name.
.
Language=Russian
Не удается заместить файл. Замещаемый файл сохранил свое первоначальное имя.
.
Language=Polish
Nie można przenieść pliku zastępującego do pliku, który ma być zamieniony. Plik, który miał ulec zamianie, zachował swoją oryginalną nazwę.
.
Language=Romanian
Unable to move the replacement file to the file to be replaced. The file to be replaced has retained its original name.
.

MessageId=1177
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_MOVE_REPLACEMENT_2
Language=English
Unable to move the replacement file to the file to be replaced. The file to be replaced has been renamed using the backup name.
.
Language=Russian
Не удается заместить файл. Замещаемый файл был переименован с использованием резервного имени.
.
Language=Polish
Nie można przenieść pliku zastępującego do pliku, który ma być zamieniony. Nazwa pliku, który miał ulec zamianie, została zmieniona na nazwę kopii zapasowej.
.
Language=Romanian
Unable to move the replacement file to the file to be replaced. The file to be replaced has been renamed using the backup name.
.

MessageId=1178
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_DELETE_IN_PROGRESS
Language=English
The volume change journal is being deleted.
.
Language=Russian
Журнал изменений тома удален.
.
Language=Polish
Dziennik zmian woluminu jest usuwany.
.
Language=Romanian
The volume change journal is being deleted.
.

MessageId=1179
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_NOT_ACTIVE
Language=English
The volume change journal is not active.
.
Language=Russian
Журнал изменений тома не активен.
.
Language=Polish
Dziennik zmiany woluminu nie jest aktywny.
.
Language=Romanian
The volume change journal is not active.
.

MessageId=1180
Severity=Success
Facility=System
SymbolicName=ERROR_POTENTIAL_FILE_FOUND
Language=English
A file was found, but it may not be the correct file.
.
Language=Russian
Файл найден, но это может быть неверный файл.
.
Language=Polish
Plik został znaleziony, ale może to nie być właściwy plik.
.
Language=Romanian
A file was found, but it may not be the correct file.
.

MessageId=1181
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_ENTRY_DELETED
Language=English
The journal entry has been deleted from the journal.
.
Language=Russian
Из журнала удалена запись.
.
Language=Polish
Wpis dziennika został usunięty z dziennika.
.
Language=Romanian
The journal entry has been deleted from the journal.
.

MessageId=1200
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DEVICE
Language=English
The specified device name is invalid.
.
Language=Russian
Указано неверное имя устройства.
.
Language=Polish
Określona nazwa urządzenia jest nieprawidłowa.
.
Language=Romanian
The specified device name is invalid.
.

MessageId=1201
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_UNAVAIL
Language=English
The device is not currently connected but it is a remembered connection.
.
Language=Russian
Устройство сейчас не подключено, но сведения о нем в конфигурации присутствуют.
.
Language=Polish
Urządzenie nie jest obecnie podłączone, ale istnieje jako zapamiętane połączenie.
.
Language=Romanian
The device is not currently connected but it is a remembered connection.
.

MessageId=1202
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_ALREADY_REMEMBERED
Language=English
The local device name has a remembered connection to another network resource.
.
Language=Russian
Локальное имя устройства уже используется для подключения к другому сетевому ресурсу.
.
Language=Polish
Nazwa urządzenia lokalnego pamięta połączenie z innym zasobem sieciowym.
.
Language=Romanian
The local device name has a remembered connection to another network resource.
.

MessageId=1203
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NET_OR_BAD_PATH
Language=English
The network path was either typed incorrectly, does not exist, or the network provider is not currently available. Please try retyping the path or contact your network administrator.
.
Language=Russian
Сетевой путь введен неправильно, не существует, или сеть сейчас недоступна. Попробуйте ввести путь заново или обратитесь к администратору сети.
.
Language=Polish
Ścieżka sieciowa została wpisana niepoprawnie, nie istnieje lub dostawca sieci jest obecnie niedostępny. Spróbuj ponownie wpisać ścieżkę lub skontaktuj się z administratorem sieci.
.
Language=Romanian
The network path was either typed incorrectly, does not exist, or the network provider is not currently available. Please try retyping the path or contact your network administrator.
.

MessageId=1204
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROVIDER
Language=English
The specified network provider name is invalid.
.
Language=Russian
Имя службы доступа к сети задано неверно.
.
Language=Polish
Określona nazwa dostawcy sieciowego jest nieprawidłowa.
.
Language=Romanian
The specified network provider name is invalid.
.

MessageId=1205
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_OPEN_PROFILE
Language=English
Unable to open the network connection profile.
.
Language=Russian
Не удается открыть конфигурацию подключения к сети.
.
Language=Polish
Nie można otworzyć profilu połączenia sieciowego.
.
Language=Romanian
Unable to open the network connection profile.
.

MessageId=1206
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_PROFILE
Language=English
The network connection profile is corrupted.
.
Language=Russian
Конфигурация подключения к сети повреждена.
.
Language=Polish
Profil połączenia sieciowego jest uszkodzony.
.
Language=Romanian
The network connection profile is corrupted.
.

MessageId=1207
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONTAINER
Language=English
Cannot enumerate a noncontainer.
.
Language=Russian
Перечисление объектов, не являющихся контейнерами, невозможно.
.
Language=Polish
Nie można wyliczać obiektu nie będącego kontenerem.
.
Language=Romanian
Cannot enumerate a noncontainer.
.

MessageId=1208
Severity=Success
Facility=System
SymbolicName=ERROR_EXTENDED_ERROR
Language=English
An extended error has occurred.
.
Language=Russian
Ошибка расширенного типа.
.
Language=Polish
Wystąpił błąd rozszerzony.
.
Language=Romanian
An extended error has occurred.
.

MessageId=1209
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUPNAME
Language=English
The format of the specified group name is invalid.
.
Language=Russian
Неверный формат имени группы.
.
Language=Polish
Format określonej nazwy grupy jest nieprawidłowy.
.
Language=Romanian
The format of the specified group name is invalid.
.

MessageId=1210
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMPUTERNAME
Language=English
The format of the specified computer name is invalid.
.
Language=Russian
Неверный формат имени компьютера.
.
Language=Polish
Format określonej nazwy komputera jest nieprawidłowy.
.
Language=Romanian
The format of the specified computer name is invalid.
.

MessageId=1211
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EVENTNAME
Language=English
The format of the specified event name is invalid.
.
Language=Russian
Неверный формат имени события.
.
Language=Polish
Format określonej nazwy zdarzenia jest nieprawidłowy.
.
Language=Romanian
The format of the specified event name is invalid.
.

MessageId=1212
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAINNAME
Language=English
The format of the specified domain name is invalid.
.
Language=Russian
Неверный формат имени домена.
.
Language=Polish
Format określonej nazwy domeny jest nieprawidłowy.
.
Language=Romanian
The format of the specified domain name is invalid.
.

MessageId=1213
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVICENAME
Language=English
The format of the specified service name is invalid.
.
Language=Russian
Неверный формат имени службы.
.
Language=Polish
Format określonej nazwy usługi jest nieprawidłowy.
.
Language=Romanian
The format of the specified service name is invalid.
.

MessageId=1214
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_NETNAME
Language=English
The format of the specified network name is invalid.
.
Language=Russian
Неверный формат сетевого имени.
.
Language=Polish
Format określonej nazwy sieci jest nieprawidłowy.
.
Language=Romanian
The format of the specified network name is invalid.
.

MessageId=1215
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHARENAME
Language=English
The format of the specified share name is invalid.
.
Language=Russian
Недопустимый формат имени общего ресурса.
.
Language=Polish
Format określonej nazwy udziału jest nieprawidłowy.
.
Language=Romanian
The format of the specified share name is invalid.
.

MessageId=1216
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PASSWORDNAME
Language=English
The format of the specified password is invalid.
.
Language=Russian
Неверный формат пароля.
.
Language=Polish
Format określonego hasła jest nieprawidłowy.
.
Language=Romanian
The format of the specified password is invalid.
.

MessageId=1217
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGENAME
Language=English
The format of the specified message name is invalid.
.
Language=Russian
Неверный формат имени сообщения.
.
Language=Polish
Format określonej nazwy komunikatu jest nieprawidłowy.
.
Language=Romanian
The format of the specified message name is invalid.
.

MessageId=1218
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MESSAGEDEST
Language=English
The format of the specified message destination is invalid.
.
Language=Russian
Неверный формат задания адреса, по которому отправляется сообщение.
.
Language=Polish
Format określonego miejsca docelowego komunikatu jest nieprawidłowy.
.
Language=Romanian
The format of the specified message destination is invalid.
.

MessageId=1219
Severity=Success
Facility=System
SymbolicName=ERROR_SESSION_CREDENTIAL_CONFLICT
Language=English
Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again.
.
Language=Russian
Множественное подключение к серверу или к общим ресурсам одним пользователем с использованием более одного имени пользователя не разрешено. Отключите все предыдущие подключения к серверу или общим ресурсам и повторите попытку.
.
Language=Polish
Wielokrotne połączenia z serwerem lub udostępnionym zasobem przez tego samego użytkownika przy użyciu więcej niż jednej nazwy użytkownika są niedozwolone. Rozłącz wszystkie poprzednie połączenia z serwerem lub udostępnionym zasobem i spróbuj ponownie.
.
Language=Romanian
Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again.
.

MessageId=1220
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_SESSION_LIMIT_EXCEEDED
Language=English
An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.
.
Language=Russian
Попытка установки сеанса связи с сервером сети, для которого достигнут предел по числу таких сеансов.
.
Language=Polish
Podjęto próbę ustanowienia sesji z serwerem sieci, ale jest już ustanowionych zbyt wiele sesji z tym serwerem.
.
Language=Romanian
An attempt was made to establish a session to a network server, but there are already too many sessions established to that server.
.

MessageId=1221
Severity=Success
Facility=System
SymbolicName=ERROR_DUP_DOMAINNAME
Language=English
The workgroup or domain name is already in use by another computer on the network.
.
Language=Russian
Имя рабочей группы или домена уже используется другим компьютером в сети.
.
Language=Polish
Nazwa domeny lub grupy roboczej jest już używana przez inny komputer w sieci.
.
Language=Romanian
The workgroup or domain name is already in use by another computer on the network.
.

MessageId=1222
Severity=Success
Facility=System
SymbolicName=ERROR_NO_NETWORK
Language=English
The network is not present or not started.
.
Language=Russian
Сеть отсутствует или не запущена.
.
Language=Polish
Brak sieci lub nie została ona uruchomiona.
.
Language=Romanian
The network is not present or not started.
.

MessageId=1223
Severity=Success
Facility=System
SymbolicName=ERROR_CANCELLED
Language=English
The operation was canceled by the user.
.
Language=Russian
Операция была отменена пользователем.
.
Language=Polish
Operacja została anulowana przez użytkownika.
.
Language=Romanian
The operation was canceled by the user.
.

MessageId=1224
Severity=Success
Facility=System
SymbolicName=ERROR_USER_MAPPED_FILE
Language=English
The requested operation cannot be performed on a file with a user-mapped section open.
.
Language=Russian
Запрошенную операцию нельзя выполнить для файла с открытой пользователем сопоставленной секцией.
.
Language=Polish
Nie można wykonać żądanej operacji na pliku z otwartą sekcją mapowania użytkownika.
.
Language=Romanian
The requested operation cannot be performed on a file with a user-mapped section open.
.

MessageId=1225
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_REFUSED
Language=English
The remote system refused the network connection.
.
Language=Russian
Удаленный компьютер отклонил это сетевое подключение.
.
Language=Polish
Komputer zdalny odrzucił połączenie sieciowe.
.
Language=Romanian
The remote system refused the network connection.
.

MessageId=1226
Severity=Success
Facility=System
SymbolicName=ERROR_GRACEFUL_DISCONNECT
Language=English
The network connection was gracefully closed.
.
Language=Russian
Сетевое подключение было закрыто.
.
Language=Polish
Połączenie sieciowe zostało bezpiecznie zamknięte.
.
Language=Romanian
The network connection was gracefully closed.
.

MessageId=1227
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_ALREADY_ASSOCIATED
Language=English
The network transport endpoint already has an address associated with it.
.
Language=Russian
Конечной точке сетевого транспорта уже сопоставлен адрес.
.
Language=Polish
Z punktem końcowym transportu sieciowego jest już skojarzony adres.
.
Language=Romanian
The network transport endpoint already has an address associated with it.
.

MessageId=1228
Severity=Success
Facility=System
SymbolicName=ERROR_ADDRESS_NOT_ASSOCIATED
Language=English
An address has not yet been associated with the network endpoint.
.
Language=Russian
Конечной точке сети еще не сопоставлен адрес.
.
Language=Polish
Adres nie został jeszcze skojarzony z punktem końcowym sieci.
.
Language=Romanian
An address has not yet been associated with the network endpoint.
.

MessageId=1229
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_INVALID
Language=English
An operation was attempted on a nonexistent network connection.
.
Language=Russian
Попытка выполнить операцию для несуществующего сетевого подключения.
.
Language=Polish
Próbowano wykonać operację na nieistniejącym połączeniu sieciowym.
.
Language=Romanian
An operation was attempted on a nonexistent network connection.
.

MessageId=1230
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ACTIVE
Language=English
An invalid operation was attempted on an active network connection.
.
Language=Russian
Попытка выполнить недопустимую операцию для активного сетевого подключения.
.
Language=Polish
Na aktywnym połączeniu sieciowym próbowano wykonać nieprawidłową operację.
.
Language=Romanian
An invalid operation was attempted on an active network connection.
.

MessageId=1231
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.
Language=Russian
Сетевая папка недоступна. За информацией о разрешении проблем в сети обратитесь к справочной системе ReactOS.
.
Language=Polish
Lokalizacja sieciowa jest nieosiągalna. Informacje na temat rozwiązywania problemów z siecią można znaleźć w Pomocy systemu ReactOS.
.
Language=Romanian
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.

MessageId=1232
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.
Language=Russian
Сетевая папка недоступна. За информацией о разрешении проблем в сети обратитесь к справочной системе ReactOS.
.
Language=Polish
Lokalizacja sieciowa jest nieosiągalna. Informacje na temat rozwiązywania problemów z siecią można znaleźć w Pomocy systemu ReactOS.
.
Language=Romanian
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.

MessageId=1233
Severity=Success
Facility=System
SymbolicName=ERROR_PROTOCOL_UNREACHABLE
Language=English
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.
Language=Russian
Сетевая папка недоступна. За информацией о разрешении проблем в сети обратитесь к справочной системе ReactOS.
.
Language=Polish
Lokalizacja sieciowa jest nieosiągalna. Informacje na temat rozwiązywania problemów z siecią można znaleźć w Pomocy systemu ReactOS.
.
Language=Romanian
The network location cannot be reached. For information about network troubleshooting, see ReactOS Help.
.

MessageId=1234
Severity=Success
Facility=System
SymbolicName=ERROR_PORT_UNREACHABLE
Language=English
No service is operating at the destination network endpoint on the remote system.
.
Language=Russian
На конечном звене нужной сети удаленной системы не запущена ни одна служба.
.
Language=Polish
W docelowym punkcie końcowym sieci systemu zdalnego nie działa żadna usługa.
.
Language=Romanian
No service is operating at the destination network endpoint on the remote system.
.

MessageId=1235
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_ABORTED
Language=English
The request was aborted.
.
Language=Russian
Запрос был прерван.
.
Language=Polish
Żądanie zostało przerwane.
.
Language=Romanian
The request was aborted.
.

MessageId=1236
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_ABORTED
Language=English
The network connection was aborted by the local system.
.
Language=Russian
Подключение к сети было разорвано локальной системой.
.
Language=Polish
Połączenie sieciowe zostało przerwane przez system lokalny.
.
Language=Romanian
The network connection was aborted by the local system.
.

MessageId=1237
Severity=Success
Facility=System
SymbolicName=ERROR_RETRY
Language=English
The operation could not be completed. A retry should be performed.
.
Language=Russian
Не удалось завершить операцию.  Следует повторить ее.
.
Language=Polish
Operacja nie zakończyła się pomyślnie. Należy ponowić próbę.
.
Language=Romanian
The operation could not be completed. A retry should be performed.
.

MessageId=1238
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTION_COUNT_LIMIT
Language=English
A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.
.
Language=Russian
Подключение к серверу невозможно, так как для данной учетной записи уже достигнут предел по числу одновременных подключений.
.
Language=Polish
Nie można ustanowić połączenia z serwerem z powodu wyczerpania limitu jednoczesnych połączeń dla tego konta.
.
Language=Romanian
A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached.
.

MessageId=1239
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_TIME_RESTRICTION
Language=English
Attempting to log in during an unauthorized time of day for this account.
.
Language=Russian
Попытка входа в сеть в непредусмотренное для этой учетной записи время дня.
.
Language=Polish
Próbowano zalogować się w porze dnia niedozwolonej dla tego konta.
.
Language=Romanian
Attempting to log in during an unauthorized time of day for this account.
.

MessageId=1240
Severity=Success
Facility=System
SymbolicName=ERROR_LOGIN_WKSTA_RESTRICTION
Language=English
The account is not authorized to log in from this station.
.
Language=Russian
Данная учетная запись не может быть использована для входа в сеть с этой станции.
.
Language=Polish
Konto nie ma uprawnień do logowania z tej stacji.
.
Language=Romanian
The account is not authorized to log in from this station.
.

MessageId=1241
Severity=Success
Facility=System
SymbolicName=ERROR_INCORRECT_ADDRESS
Language=English
The network address could not be used for the operation requested.
.
Language=Russian
Не удалось использовать сетевой адрес для запрошенной операции.
.
Language=Polish
Adresu sieciowego nie można użyć do żądanej operacji.
.
Language=Romanian
The network address could not be used for the operation requested.
.

MessageId=1242
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_REGISTERED
Language=English
The service is already registered.
.
Language=Russian
Служба уже зарегистрирована.
.
Language=Polish
Usługa jest już zarejestrowana.
.
Language=Romanian
The service is already registered.
.

MessageId=1243
Severity=Success
Facility=System
SymbolicName=ERROR_SERVICE_NOT_FOUND
Language=English
The specified service does not exist.
.
Language=Russian
Указанная служба не существует.
.
Language=Polish
Określona usługa nie istnieje.
.
Language=Romanian
The specified service does not exist.
.

MessageId=1244
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_AUTHENTICATED
Language=English
The operation being requested was not performed because the user has not been authenticated.
.
Language=Russian
Запрошенная операция не была выполнена, так как пользователь не зарегистрирован.
.
Language=Polish
Żądana operacja nie została wykonana, ponieważ użytkownik nie został uwierzytelniony.
.
Language=Romanian
The operation being requested was not performed because the user has not been authenticated.
.

MessageId=1245
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGGED_ON
Language=English
The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist.
.
Language=Russian
Запрошенная операция не была выполнена, так как пользователь не выполнил вход в сеть. Указанная служба не существует.
.
Language=Polish
Żądana operacja nie została wykonana, ponieważ użytkownik nie zalogował się do sieci. Określona usługa nie istnieje.
.
Language=Romanian
The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist.
.

MessageId=1246
Severity=Success
Facility=System
SymbolicName=ERROR_CONTINUE
Language=English
Continue with work in progress.
.
Language=Russian
Требуется продолжить выполняющуюся операцию.
.
Language=Polish
Kontynuuj wykonywaną pracę.
.
Language=Romanian
Continue with work in progress.
.

MessageId=1247
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_INITIALIZED
Language=English
An attempt was made to perform an initialization operation when initialization has already been completed.
.
Language=Russian
Попытка выполнить операцию инициализации, которая уже проведена.
.
Language=Polish
Wykonano próbę wykonania operacji inicjalizacji po tym, gdy inicjalizacja została już wykonana.
.
Language=Romanian
An attempt was made to perform an initialization operation when initialization has already been completed.
.

MessageId=1248
Severity=Success
Facility=System
SymbolicName=ERROR_NO_MORE_DEVICES
Language=English
No more local devices.
.
Language=Russian
Больше локальных устройств не найдено.
.
Language=Polish
Brak dalszych urządzeń lokalnych.
.
Language=Romanian
No more local devices.
.

MessageId=1249
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_SITE
Language=English
The specified site does not exist.
.
Language=Russian
Указанный сайт не существует.
.
Language=Polish
Podana lokacja nie istnieje.
.
Language=Romanian
The specified site does not exist.
.

MessageId=1250
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CONTROLLER_EXISTS
Language=English
A domain controller with the specified name already exists.
.
Language=Russian
Контроллер домена с указанным именем уже существует.
.
Language=Polish
Kontroler domeny o podanej nazwie już istnieje.
.
Language=Romanian
A domain controller with the specified name already exists.
.

MessageId=1251
Severity=Success
Facility=System
SymbolicName=ERROR_ONLY_IF_CONNECTED
Language=English
This operation is supported only when you are connected to the server.
.
Language=Russian
Эта операция поддерживается только при наличии подключения к серверу.
.
Language=Polish
Ta operacja jest obsługiwana tylko wtedy, gdy jest nawiązane połączenie z serwerem.
.
Language=Romanian
This operation is supported only when you are connected to the server.
.

MessageId=1252
Severity=Success
Facility=System
SymbolicName=ERROR_OVERRIDE_NOCHANGES
Language=English
The group policy framework should call the extension even if there are no changes.
.
Language=Russian
Основной модуль групповой политики должен вызвать расширение даже в случае отсутствия изменений.
.
Language=Polish
Zasady grupowe powinny wywołać rozszerzenie, nawet jeśli nie ma zmian.
.
Language=Romanian
The group policy framework should call the extension even if there are no changes.
.

MessageId=1253
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_USER_PROFILE
Language=English
The specified user does not have a valid profile.
.
Language=Russian
Выбранный пользователь не имеет допустимого профиля.
.
Language=Polish
Nie ma prawidłowego profilu dla podanego użytkownika.
.
Language=Romanian
The specified user does not have a valid profile.
.

MessageId=1254
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED_ON_SBS
Language=English
This operation is not supported on a computer running Windows Server 2003 for Small Business Server.
.
Language=Russian
Эта операция не поддерживается на Windows Server 2003 for Small Business Server.
.
Language=Polish
Ta operacja nie jest obsługiwana na komputerze z uruchomionym systemem Windows Server 2003 for Small Business Server.
.
Language=Romanian
This operation is not supported on a computer running Windows Server 2003 for Small Business Server.
.

MessageId=1255
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_SHUTDOWN_IN_PROGRESS
Language=English
The server machine is shutting down.
.
Language=Russian
Идет завершение работы компьютера-сервера.
.
Language=Polish
Trwa zamykanie serwera.
.
Language=Romanian
The server machine is shutting down.
.

MessageId=1256
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_DOWN
Language=English
The remote system is not available. For information about network troubleshooting, see ReactOS Help.
.
Language=Russian
Удаленная система недоступна. За информацией о разрешении проблем в сети, обратитесь к справочной системе ReactOS.
.
Language=Polish
System zdalny jest niedostępny. Aby uzyskać informacje dotyczące rozwiązywania problemów z siecią, zobacz Pomoc systemu ReactOS.
.
Language=Romanian
The remote system is not available. For information about network troubleshooting, see ReactOS Help.
.

MessageId=1257
Severity=Success
Facility=System
SymbolicName=ERROR_NON_ACCOUNT_SID
Language=English
The security identifier provided is not from an account domain.
.
Language=Russian
Был указан идентификатор безопасности не из того домена.
.
Language=Polish
Podany identyfikator zabezpieczeń nie pochodzi z domeny konta.
.
Language=Romanian
The security identifier provided is not from an account domain.
.

MessageId=1258
Severity=Success
Facility=System
SymbolicName=ERROR_NON_DOMAIN_SID
Language=English
The security identifier provided does not have a domain component.
.
Language=Russian
В указанном идентификаторе безопасности отсутствует компонент для домена.
.
Language=Polish
Podany identyfikator zabezpieczeń nie ma składnika określającego domenę.
.
Language=Romanian
The security identifier provided does not have a domain component.
.

MessageId=1259
Severity=Success
Facility=System
SymbolicName=ERROR_APPHELP_BLOCK
Language=English
AppHelp dialog canceled thus preventing the application from starting.
.
Language=Russian
Окно AppHelp закрыто, из-за чего приложение не было запущено.
.
Language=Polish
Okno dialogowe pomocy aplikacji zostało anulowane i uniemożliwia to uruchomienie aplikacji.
.
Language=Romanian
AppHelp dialog canceled thus preventing the application from starting.
.

MessageId=1260
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_BY_POLICY
Language=English
ReactOS cannot open this program because it has been prevented by a software restriction policy. For more information, open Event Viewer or contact your system administrator.
.
Language=Russian
Эта программа заблокирована групповой политикой. За дополнительными сведениями обращайтесь к системному администратору.
.
Language=Polish
Ten program jest blokowany przez zasady grupy. Aby uzyskać więcej informacji, otwórz Podgląd zdarzeń lub skontaktuj się z administratorem systemu.
.
Language=Romanian
ReactOS cannot open this program because it has been prevented by a software restriction policy. For more information, open Event Viewer or contact your system administrator.
.

MessageId=1261
Severity=Success
Facility=System
SymbolicName=ERROR_REG_NAT_CONSUMPTION
Language=English
A program attempt to use an invalid register value. Normally caused by an uninitialized register. This error is Itanium specific.
.
Language=Russian
Попытка программы использовать неправильное значение регистра. Обычно это вызвано неинициализированным регистром.
.
Language=Polish
Program próbuje użyć nieprawidłowej wartości rejestru. Zwykle przyczyną jest niezainicjowany rejestr. Ten błąd jest specyficzny dla procesora Itanium.
.
Language=Romanian
A program attempt to use an invalid register value. Normally caused by an uninitialized register. This error is Itanium specific.
.

MessageId=1262
Severity=Success
Facility=System
SymbolicName=ERROR_CSCSHARE_OFFLINE
Language=English
The share is currently offline or does not exist.
.
Language=Russian
Общий ресурс недоступен или не существует.
.
Language=Polish
Udział jest aktualnie w trybie offline lub nie istnieje.
.
Language=Romanian
The share is currently offline or does not exist.
.

MessageId=1263
Severity=Success
Facility=System
SymbolicName=ERROR_PKINIT_FAILURE
Language=English
The kerberos protocol encountered an error while validating the KDC certificate during smartcard logon.
.
Language=Russian
Ошибка протокола Kerberos при проверке сертификата KDC во время входа в систему со смарт-картой. Дополнительные сведения см. в журнале системных событий.
.
Language=Polish
Protokół Kerberos napotkał błąd, sprawdzając poprawność certyfikatu KDC podczas logowania karty inteligentnej. Więcej informacji można znaleźć w dzienniku zdarzeń systemu.
.
Language=Romanian
The kerberos protocol encountered an error while validating the KDC certificate during smartcard logon.
.

MessageId=1264
Severity=Success
Facility=System
SymbolicName=ERROR_SMARTCARD_SUBSYSTEM_FAILURE
Language=English
The kerberos protocol encountered an error while attempting to utilize the smartcard subsystem.
.
Language=Russian
Ошибка протокола Kerberos при попытке использовать подсистему для смарт-карт.
.
Language=Polish
Protokół Kerberos napotkał błąd podczas próby użycia podsystemu karty inteligentnej.
.
Language=Romanian
The kerberos protocol encountered an error while attempting to utilize the smartcard subsystem.
.

MessageId=1265
Severity=Success
Facility=System
SymbolicName=ERROR_DOWNGRADE_DETECTED
Language=English
The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you.
.
Language=Russian
Системе не удается установить связь с контроллером домена, чтобы обработать запрос на проверку подлинности. Попробуйте еще раз позже.
.
Language=Polish
System wykrył możliwe zagrożenie bezpieczeństwa. Upewnij się, że możesz skontaktować się z serwerem, który Cię uwierzytelnił.
.
Language=Romanian
The system detected a possible attempt to compromise security. Please ensure that you can contact the server that authenticated you.
.

MessageId=1266
Severity=Success
Facility=System
SymbolicName=SEC_E_SMARTCARD_CERT_REVOKED
Language=English
The smartcard certificate used for authentication has been revoked. Please contact your system administrator. There may be additional information in the event log.
.
Language=Russian
The smartcard certificate used for authentication has been revoked. Please contact your system administrator. There may be additional information in the event log.
.
Language=Polish
Certyfikat karty inteligentnej użyty do uwierzytelnienia został odwołany. Skontaktuj się z administratorem systemu. Dodatkowe informacje może zawierać dziennik zdarzeń.
.
Language=Romanian
The smartcard certificate used for authentication has been revoked. Please contact your system administrator. There may be additional information in the event log.
.

MessageId=1267
Severity=Success
Facility=System
SymbolicName=SEC_E_ISSUING_CA_UNTRUSTED
Language=English
An untrusted certificate authority was detected while processing the smartcard certificate used for authentication. Please contact your system administrator.
.
Language=Russian
An untrusted certificate authority was detected while processing the smartcard certificate used for authentication. Please contact your system administrator.
.
Language=Polish
Podczas przetwarzania certyfikatu karty inteligentnej używanej do uwierzytelniania został wykryty niezaufany urząd certyfikacji. Skontaktuj się z administratorem systemu.
.
Language=Romanian
An untrusted certificate authority was detected while processing the smartcard certificate used for authentication. Please contact your system administrator.
.

MessageId=1268
Severity=Success
Facility=System
SymbolicName=SEC_E_REVOCATION_OFFLINE_C
Language=English
The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator.
.
Language=Russian
The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator.
.
Language=Polish
Ustalenie stanu odwołania certyfikatu karty inteligentnej używanego do uwierzytelniania nie było możliwe. Skontaktuj się z administratorem systemu.
.
Language=Romanian
The revocation status of the smartcard certificate used for authentication could not be determined. Please contact your system administrator.
.

MessageId=1269
Severity=Success
Facility=System
SymbolicName=SEC_E_PKINIT_CLIENT_FAILUR
Language=English
The smartcard certificate used for authentication was not trusted. Please contact your system administrator.
.
Language=Russian
The smartcard certificate used for authentication was not trusted. Please contact your system administrator.
.
Language=Polish
Certyfikat karty inteligentnej używany do uwierzytelniania nie był zaufany. Skontaktuj się z administratorem systemu.
.
Language=Romanian
The smartcard certificate used for authentication was not trusted. Please contact your system administrator.
.

MessageId=1270
Severity=Success
Facility=System
SymbolicName=SEC_E_SMARTCARD_CERT_EXPIRED
Language=English
The smartcard certificate used for authentication has expired. Please contact your system administrator.
.
Language=Russian
The smartcard certificate used for authentication has expired. Please contact your system administrator.
.
Language=Polish
Certyfikat karty inteligentnej używany do uwierzytelniania wygasł. Skontaktuj się z administratorem systemu.
.
Language=Romanian
The smartcard certificate used for authentication has expired. Please contact your system administrator.
.

MessageId=1271
Severity=Success
Facility=System
SymbolicName=ERROR_MACHINE_LOCKED
Language=English
The machine is locked and cannot be shut down without the force option.
.
Language=Russian
Компьютер заблокирован и не может завершить работу без режима принудительного завершения.
.
Language=Polish
Komputer jest zablokowany i nie można go zamknąć bez opcji wymuszenia.
.
Language=Romanian
The machine is locked and cannot be shut down without the force option.
.

MessageId=1273
Severity=Success
Facility=System
SymbolicName=ERROR_CALLBACK_SUPPLIED_INVALID_DATA
Language=English
An application-defined callback gave invalid data when called.
.
Language=Russian
Определенный в приложении ответный вызов вернул неверные данные.
.
Language=Polish
Określone przez aplikację wywołanie zwrotne dało po wywołaniu nieprawidłowe dane.
.
Language=Romanian
An application-defined callback gave invalid data when called.
.

MessageId=1274
Severity=Success
Facility=System
SymbolicName=ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED
Language=English
The group policy framework should call the extension in the synchronous foreground policy refresh.
.
Language=Russian
Система групповой политики должна вызывать расширения в синхронном, не фоновом режиме обновления.
.
Language=Polish
Ogólna struktura zasad grupy powinna wywołać rozszerzenie podczas synchronicznego, pierwszoplanowego odświeżania zasad.
.
Language=Romanian
The group policy framework should call the extension in the synchronous foreground policy refresh.
.

MessageId=1275
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVER_BLOCKED
Language=English
This driver has been blocked from loading.
.
Language=Russian
Загрузка драйвера была заблокирована.
.
Language=Polish
Nastąpiło zablokowanie ładowania sterownika
.
Language=Romanian
This driver has been blocked from loading.
.

MessageId=1276
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_IMPORT_OF_NON_DLL
Language=English
A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image.
.
Language=Russian
Библиотека, на которую ссылается модуль, не является библиотекой динамической компоновки (DLL) или исполняемым модулем.
.
Language=Polish
Biblioteka dołączana dynamicznie DLL odwoływała się do modułu, który nie był ani biblioteką DLL, ani obrazem wykonywalnym procesu.
.
Language=Romanian
A dynamic link library (DLL) referenced a module that was neither a DLL nor the process's executable image.
.

MessageId=1277
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_WEBBLADE
Language=English
ReactOS cannot open this program since it has been disabled.
.
Language=Russian
ReactOS не удается запустить эту программу, так как она отключена.
.
Language=Polish
System ReactOS nie może otworzyć tego programu, ponieważ został on wyłączony.
.
Language=Romanian
ReactOS cannot open this program since it has been disabled.
.

MessageId=1278
Severity=Success
Facility=System
SymbolicName=ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER
Language=English
ReactOS cannot open this program because the license enforcement system has been tampered with or become corrupted.
.
Language=Russian
ReactOS не удается открыть эту программу, так как система учета лицензий изменена или повреждена.
.
Language=Polish
System ReactOS nie może otworzyć tego programu, ponieważ system wymuszania licencji został zmieniony lub uszkodzony.
.
Language=Romanian
ReactOS cannot open this program because the license enforcement system has been tampered with or become corrupted.
.

MessageId=1279
Severity=Success
Facility=System
SymbolicName=ERROR_RECOVERY_FAILURE
Language=English
A transaction recovery failed.
.
Language=Russian
Неудача при восстановлении транзакции.
.
Language=Polish
Odzyskanie transakcji nie powiodło się.
.
Language=Romanian
A transaction recovery failed.
.

MessageId=1280
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_FIBER
Language=English
The current thread has already been converted to a fiber.
.
Language=Russian
Текущий поток уже преобразован в нить.
.
Language=Polish
Bieżący wątek został już przekonwertowany do włókna.
.
Language=Romanian
The current thread has already been converted to a fiber.
.

MessageId=1281
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_THREAD
Language=English
The current thread has already been converted from a fiber.
.
Language=Russian
Текущий поток уже преобразован из нити.
.
Language=Polish
Bieżący wątek został już przekonwertowany z włókna.
.
Language=Romanian
The current thread has already been converted from a fiber.
.

MessageId=1282
Severity=Success
Facility=System
SymbolicName=ERROR_STACK_BUFFER_OVERRUN
Language=English
The system detected an overrun of a stack-based buffer in this application. This overrun could potentially allow a malicious user to gain control of this application.
.
Language=Russian
Обнаружено переполнение стекового буфера в данном приложении. Это переполнение может позволить злоумышленнику получить управление над данным приложением.
.
Language=Polish
System wykrył w tej aplikacji przekroczenie buforu opartego na stosie. Przekroczenie może umożliwić złośliwemu użytkownikowi uzyskanie kontroli nad tą aplikacją.
.
Language=Romanian
The system detected an overrun of a stack-based buffer in this application. This overrun could potentially allow a malicious user to gain control of this application.
.

MessageId=1283
Severity=Success
Facility=System
SymbolicName=ERROR_PARAMETER_QUOTA_EXCEEDED
Language=English
Data present in one of the parameters is more than the function can operate on.
.
Language=Russian
В одном из параметров задано больше данных, чем эта функция может обработать.
.
Language=Polish
Ilość danych w jednym z parametrów jest większa niż ilość, którą może obsłużyć funkcja.
.
Language=Romanian
Data present in one of the parameters is more than the function can operate on.
.

MessageId=1284
Severity=Success
Facility=System
SymbolicName=ERROR_DEBUGGER_INACTIVE
Language=English
An attempt to do an operation on a debug object failed because the object is in the process of being deleted.
.
Language=Russian
Не удалось выполнить операцию над объектом отладки, так как он удаляется.
.
Language=Polish
Próba wykonania operacji na obiekcie debugowania nie powiodła się, ponieważ obiekt jest właśnie usuwany.
.
Language=Romanian
An attempt to do an operation on a debug object failed because the object is in the process of being deleted.
.

MessageId=1285
Severity=Success
Facility=System
SymbolicName=ERROR_DELAY_LOAD_FAILED
Language=English
An attempt to delay-load a .dll or get a function address in a delay-loaded .dll failed.
.
Language=Russian
Не удалось загрузить с задержкой библиотеку DLL или получить из нее адрес функции.
.
Language=Polish
Próba załadowania z opóźnieniem biblioteki .dll lub uzyskania adresu funkcji z biblioteki .dll załadowanej z opóźnieniem nie powiodła się.
.
Language=Romanian
An attempt to delay-load a .dll or get a function address in a delay-loaded .dll failed.
.

MessageId=1286
Severity=Success
Facility=System
SymbolicName=ERROR_VDM_DISALLOWED
Language=English
%1 is a 16-bit application. You do not have permissions to execute 16-bit applications. Check your permissions with your system administrator.
.
Language=Russian
"%1" является 16-битным приложением. Вы не имеете прав доступа для выполнения 16-битных приложений. Проверьте ваши права доступа с вашим системным администратором.
.
Language=Polish
%1 jest aplikacją 16-bitową. Nie masz uprawnień do wykonywania aplikacji 16-bitowych. Skontaktuj się z administratorem, aby uzyskać informacje o uprawnieniach.
.
Language=Romanian
%1 is a 16-bit application. You do not have permissions to execute 16-bit applications. Check your permissions with your system administrator.
.

MessageId=1287
Severity=Success
Facility=System
SymbolicName=ERROR_UNIDENTIFIED_ERROR
Language=English
Insufficient information exists to identify the cause of failure.
.
Language=Russian
Недостаточно сведений для установки причины сбоя.
.
Language=Polish
Za mało informacji do zidentyfikowania przyczyny błędu.
.
Language=Romanian
Insufficient information exists to identify the cause of failure.
.

MessageId=1288
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_BANDWIDTH_PARAMETERS
Language=English
An invalid budget or period parameter was specified.
.
Language=Russian
В динамическую функцию C передан неверный параметр.
.
Language=Polish
An invalid budget or period parameter was specified.
.
Language=Romanian
An invalid budget or period parameter was specified.
.

MessageId=1289
Severity=Success
Facility=System
SymbolicName=ERROR_AFFINITY_NOT_COMPATIBLE
Language=English
An attempt was made to join a thread to a reserve whose affinity did not intersect the reserve affinity or an attempt was made to associate a process with a reserve whose affinity did not intersect the reserve affinity.
.
Language=Russian
Операция выполнена за пределами допустимой длины данных файла.
.
Language=Polish
An attempt was made to join a thread to a reserve whose affinity did not intersect the reserve affinity or an attempt was made to associate a process with a reserve whose affinity did not intersect the reserve affinity.
.
Language=Romanian
An attempt was made to join a thread to a reserve whose affinity did not intersect the reserve affinity or an attempt was made to associate a process with a reserve whose affinity did not intersect the reserve affinity.
.

MessageId=1290
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_ALREADY_IN_RESERVE
Language=English
An attempt was made to join a thread to a reserve which was already joined to another reserve.
.
Language=Russian
Не удалось запустить эту службу, так как одна или несколько служб одного процесса имеют несовместимый параметр типа SID службы. Служба с ограниченным типом SID может сосуществовать в одном и том же процессе только с другими службами с ограниченным типом SID. Если тип SID для этой службы только что настроен, необходимо перезапустить хост-процесс, чтобы запустить эту службу.
.
Language=Polish
An attempt was made to join a thread to a reserve which was already joined to another reserve.
.
Language=Romanian
An attempt was made to join a thread to a reserve which was already joined to another reserve.
.

MessageId=1291
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_NOT_IN_RESERVE
Language=English
An attempt was made to disjoin a thread from a reserve, but the thread was not joined to the reserve.
.
Language=Russian
Процесс, использующий драйвер для этого устройства, прерван.
.
Language=Polish
An attempt was made to disjoin a thread from a reserve, but the thread was not joined to the reserve.
.
Language=Romanian
An attempt was made to disjoin a thread from a reserve, but the thread was not joined to the reserve.
.

MessageId=1292
Severity=Success
Facility=System
SymbolicName=ERROR_THREAD_PROCESS_IN_RESERVE
Language=English
An attempt was made to disjoin a thread from a reserve whose process is associated with a reserve.
.
Language=Russian
Операция попыталась превысить установленный предел.
.
Language=Polish
An attempt was made to disjoin a thread from a reserve whose process is associated with a reserve.
.
Language=Romanian
An attempt was made to disjoin a thread from a reserve whose process is associated with a reserve.
.

MessageId=1293
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_ALREADY_IN_RESERVE
Language=English
An attempt was made to associate a process with a reserve that was already associated with a reserve.
.
Language=Russian
Целевой процесс или процесс целевого потока является защищенным.
.
Language=Polish
An attempt was made to associate a process with a reserve that was already associated with a reserve.
.
Language=Romanian
An attempt was made to associate a process with a reserve that was already associated with a reserve.
.

MessageId=1294
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_NOT_IN_RESERVE
Language=English
An attempt was made to disassociate a process from a reserve, but the process did not have an associated reserve.
.
Language=Russian
Клиент уведомлений службы значительно отстает от текущего состояния служб в системе.
.
Language=Polish
An attempt was made to disassociate a process from a reserve, but the process did not have an associated reserve.
.
Language=Romanian
An attempt was made to disassociate a process from a reserve, but the process did not have an associated reserve.
.

MessageId=1295
Severity=Success
Facility=System
SymbolicName=ERROR_PROCESS_THREADS_IN_RESERVE
Language=English
An attempt was made to associate a process with a reserve, but the process contained thread joined to a reserve.
.
Language=Russian
Требуемая операция с файлами завершилась сбоем из-за превышения квоты на использование места на диске. Чтобы освободить место на диске, переместите файлы в другое место или удалите ненужные файлы. За дополнительными сведениями обратитесь к системному администратору.
.
Language=Polish
An attempt was made to associate a process with a reserve, but the process contained thread joined to a reserve.
.
Language=Romanian
An attempt was made to associate a process with a reserve, but the process contained thread joined to a reserve.
.

MessageId=1296
Severity=Success
Facility=System
SymbolicName=ERROR_AFFINITY_NOT_SET_IN_RESERVE
Language=English
An attempt was made to set the affinity of a thread or a process, but the thread or process was joined or associated with a reserve.
.
Language=Russian
Требуемая операция с файлами завершилась сбоем, так как политика хранилища блокирует этот тип файлов. За дополнительными сведениями обратитесь к системному администратору.
.
Language=Polish
An attempt was made to set the affinity of a thread or a process, but the thread or process was joined or associated with a reserve.
.
Language=Romanian
An attempt was made to set the affinity of a thread or a process, but the thread or process was joined or associated with a reserve.
.

MessageId=1297
Severity=Success
Facility=System
SymbolicName=ERROR_IMPLEMENTATION_LIMIT
Language=English
An operation attempted to exceed an implementation-defined limit.
.
Language=Russian
Права, необходимые службе для правильной работы, не существуют в конфигурации учетной записи службы.
.
Language=Polish
Operacja usiłowała przekroczyć ograniczenie zdefiniowane w implementacji.
.
Language=Romanian
An operation attempted to exceed an implementation-defined limit.
.

MessageId=1298
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CACHE_ONLY
Language=English
The requested object is for internal DS operations only.
.
Language=Russian
Поток, задействованный в данной операции, не отвечает.
.
Language=Polish
The requested object is for internal DS operations only.
.
Language=Romanian
The requested object is for internal DS operations only.
.

MessageId=1300
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ALL_ASSIGNED
Language=English
Not all privileges referenced are assigned to the caller.
.
Language=Russian
Вызывающая сторона не обладает всеми необходимыми правами доступа.
.
Language=Polish
Nie wszystkie wywoływane uprawnienia lub grupy są przypisane komputerowi wywołującemu.
.
Language=Romanian
Not all privileges referenced are assigned to the caller.
.

MessageId=1301
Severity=Success
Facility=System
SymbolicName=ERROR_SOME_NOT_MAPPED
Language=English
Some mapping between account names and security IDs was not done.
.
Language=Russian
Некоторые соответствия между именами пользователей и идентификаторами безопасности не были установлены.
.
Language=Polish
Nie wykonano pewnych mapowań między nazwami kont i identyfikatorami zabezpieczeń.
.
Language=Romanian
Some mapping between account names and security IDs was not done.
.

MessageId=1302
Severity=Success
Facility=System
SymbolicName=ERROR_NO_QUOTAS_FOR_ACCOUNT
Language=English
No system quota limits are specifically set for this account.
.
Language=Russian
Системные квоты для данной учетной записи не установлены.
.
Language=Polish
Dla tego konta nie ustawiono żadnych szczególnych ograniczeń przydziałów zasobów systemowych.
.
Language=Romanian
No system quota limits are specifically set for this account.
.

MessageId=1303
Severity=Success
Facility=System
SymbolicName=ERROR_LOCAL_USER_SESSION_KEY
Language=English
No encryption key is available. A well-known encryption key was returned.
.
Language=Russian
Ключ шифрования недоступен. Возвращен общедоступный ключ.
.
Language=Polish
Brak klucza szyfrowania. Został zwrócony dobrze znany klucz szyfrowania.
.
Language=Romanian
No encryption key is available. A well-known encryption key was returned.
.

MessageId=1304
Severity=Success
Facility=System
SymbolicName=ERROR_NULL_LM_PASSWORD
Language=English
The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.
.
Language=Russian
Пароль слишком сложен и не может быть преобразован в пароль LAN Manager. Вместо пароля LAN Manager была возвращена пустая строка.
.
Language=Polish
Hasło jest zbyt złożone, aby mogło być przekształcone na hasło programu LAN Manager. Zwrócone hasło programu LAN Manager jest ciągiem pustym (NULL).
.
Language=Romanian
The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a NULL string.
.

MessageId=1305
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_REVISION
Language=English
The revision level is unknown.
.
Language=Russian
Уровень редакции неизвестен.
.
Language=Polish
Poziom wydania jest nieznany.
.
Language=Romanian
The revision level is unknown.
.

MessageId=1306
Severity=Success
Facility=System
SymbolicName=ERROR_REVISION_MISMATCH
Language=English
Indicates two revision levels are incompatible.
.
Language=Russian
Два уровня редакции являются несовместимыми.
.
Language=Polish
Wskazuje, że dwa poziomy wydania są niezgodne.
.
Language=Romanian
Indicates two revision levels are incompatible.
.

MessageId=1307
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OWNER
Language=English
This security ID may not be assigned as the owner of this object.
.
Language=Russian
Этот идентификатор безопасности не может быть назначен владельцем этого объекта.
.
Language=Polish
Ten identyfikator zabezpieczeń nie może być przypisany jako właściciel tego obiektu.
.
Language=Romanian
This security ID may not be assigned as the owner of this object.
.

MessageId=1308
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIMARY_GROUP
Language=English
This security ID may not be assigned as the primary group of an object.
.
Language=Russian
Этот идентификатор безопасности не может быть назначен основной группой объекта.
.
Language=Polish
Ten identyfikator zabezpieczeń nie może być przypisany jako grupa podstawowa obiektu.
.
Language=Romanian
This security ID may not be assigned as the primary group of an object.
.

MessageId=1309
Severity=Success
Facility=System
SymbolicName=ERROR_NO_IMPERSONATION_TOKEN
Language=English
An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.
.
Language=Russian
Предпринята попытка использования элемента олицетворения потоком команд, который в данное время не олицетворяет клиента.
.
Language=Polish
Na tokenie personifikacji podjął próbę działania wątek, który obecnie nie personifikuje żadnego klienta.
.
Language=Romanian
An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client.
.

MessageId=1310
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_DISABLE_MANDATORY
Language=English
The group may not be disabled.
.
Language=Russian
Эту группу невозможно отключить.
.
Language=Polish
Grupa nie może być wyłączona.
.
Language=Romanian
The group may not be disabled.
.

MessageId=1311
Severity=Success
Facility=System
SymbolicName=ERROR_NO_LOGON_SERVERS
Language=English
There are currently no logon servers available to service the logon request.
.
Language=Russian
Отсутствуют серверы, которые могли бы обработать запрос на вход в сеть.
.
Language=Polish
Nie ma obecnie serwerów logowania dostępnych do obsługi żądania logowania.
.
Language=Romanian
There are currently no logon servers available to service the logon request.
.

MessageId=1312
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_LOGON_SESSION
Language=English
A specified logon session does not exist. It may already have been terminated.
.
Language=Russian
Указанный сеанс работы не существует. Возможно, он уже  завершен.
.
Language=Polish
Określona sesja logowania nie istnieje. Być może została już zakończona.
.
Language=Romanian
A specified logon session does not exist. It may already have been terminated.
.

MessageId=1313
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PRIVILEGE
Language=English
A specified privilege does not exist.
.
Language=Russian
Указанная привилегия не существует.
.
Language=Polish
Określone uprawnienie nie istnieje.
.
Language=Romanian
A specified privilege does not exist.
.

MessageId=1314
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVILEGE_NOT_HELD
Language=English
A required privilege is not held by the client.
.
Language=Russian
Клиент не обладает требуемыми правами.
.
Language=Polish
Klient nie ma wymaganych uprawnień.
.
Language=Romanian
A required privilege is not held by the client.
.

MessageId=1315
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCOUNT_NAME
Language=English
The name provided is not a properly formed account name.
.
Language=Russian
Указанное имя не является корректным именем пользователя.
.
Language=Polish
Podana nazwa nie jest właściwie sformułowaną nazwą konta.
.
Language=Romanian
The name provided is not a properly formed account name.
.

MessageId=1316
Severity=Success
Facility=System
SymbolicName=ERROR_USER_EXISTS
Language=English
The specified user already exists.
.
Language=Russian
Указанная учетная запись уже существует.
.
Language=Polish
Określone konto już istnieje.
.
Language=Romanian
The specified user already exists.
.

MessageId=1317
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_USER
Language=English
The specified user does not exist.
.
Language=Russian
Указанная учетная запись не существует.
.
Language=Polish
Określone konto nie istnieje.
.
Language=Romanian
The specified user does not exist.
.

MessageId=1318
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_EXISTS
Language=English
The specified group already exists.
.
Language=Russian
Указанная группа уже существует.
.
Language=Polish
Określona grupa już istnieje.
.
Language=Romanian
The specified group already exists.
.

MessageId=1319
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_GROUP
Language=English
The specified group does not exist.
.
Language=Russian
Указанная группа не существует.
.
Language=Polish
Określona grupa nie istnieje.
.
Language=Romanian
The specified group does not exist.
.

MessageId=1320
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_GROUP
Language=English
Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member.
.
Language=Russian
Указанный пользователь уже является членом заданной группы, либо группа не может быть удалена, так как содержит как минимум одного пользователя.
.
Language=Polish
Określone konto użytkownika jest już członkiem określonej grupy albo określona grupa nie może być usunięta, ponieważ zawiera członka grupy.
.
Language=Romanian
Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member.
.

MessageId=1321
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_GROUP
Language=English
The specified user account is not a member of the specified group account.
.
Language=Russian
Указанный пользователь не является членом заданной группы.
.
Language=Polish
Określone konto użytkownika nie jest członkiem określonego konta grupowego.
.
Language=Romanian
The specified user account is not a member of the specified group account.
.

MessageId=1322
Severity=Success
Facility=System
SymbolicName=ERROR_LAST_ADMIN
Language=English
The last remaining administration account cannot be disabled or deleted.
.
Language=Russian
Эта операция запрещена, так как может привести к отключению, удалению или невозможности входа учетной записи администратора.
.
Language=Polish
Ostatnie pozostałe konto administracyjne nie może zostać wyłączone ani usunięte.
.
Language=Romanian
The last remaining administration account cannot be disabled or deleted.
.

MessageId=1323
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_PASSWORD
Language=English
Unable to update the password. The value provided as the current password is incorrect.
.
Language=Russian
Не удается обновить пароль. Текущий пароль был задан неверно.
.
Language=Polish
Nie można zaktualizować hasła. Wartość podana jako bieżące hasło jest niepoprawna.
.
Language=Romanian
Unable to update the password. The value provided as the current password is incorrect.
.

MessageId=1324
Severity=Success
Facility=System
SymbolicName=ERROR_ILL_FORMED_PASSWORD
Language=English
Unable to update the password. The value provided for the new password contains values that are not allowed in passwords.
.
Language=Russian
Не удается обновить пароль. Новый пароль содержит недопустимые символы.
.
Language=Polish
Nie można zaktualizować hasła. Wartość podana jako nowe hasło zawiera wartości niedozwolone w hasłach.
.
Language=Romanian
Unable to update the password. The value provided for the new password contains values that are not allowed in passwords.
.

MessageId=1325
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_RESTRICTION
Language=English
Unable to update the password. The value provided for the new password does not meet the length, complexity, or history requirement of the domain.
.
Language=Russian
Не удается обновить пароль. Введенный пароль не обеспечивает требований домена к длине пароля, его сложности или истории обновления.
.
Language=Polish
Nie można zaktualizować hasła. Podana wartość nowego hasła nie spełnia wymagań domeny dotyczących długości, złożoności lub historii hasła.
.
Language=Romanian
Unable to update the password. The value provided for the new password does not meet the length, complexity, or history requirement of the domain.
.

MessageId=1326
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_FAILURE
Language=English
Logon failure: unknown user name or bad password.
.
Language=Russian
Неверное имя пользователя или пароль.
.
Language=Polish
Błąd logowania: nieznana nazwa użytkownika lub nieprawidłowe hasło.
.
Language=Romanian
Logon failure: unknown user name or bad password.
.

MessageId=1327
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_RESTRICTION
Language=English
Logon failure: user account restriction. Possible reasons are blank passwords not allowed, logon hour restrictions, or a policy restriction has been enforced.
.
Language=Russian
Вход этого пользователя в систему не выполнен из-за ограничений учетной записи. Например: пустые пароли не разрешены, ограничено число входов или включено ограничение политики.
.
Language=Polish
Błąd logowania: Ograniczenie konta użytkownika. Do możliwych przyczyn należą: niedozwolone puste hasła, ograniczenia godzin logowania lub wymuszanie ograniczenia zasad.
.
Language=Romanian
Logon failure: user account restriction. Possible reasons are blank passwords not allowed, logon hour restrictions, or a policy restriction has been enforced.
.

MessageId=1328
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_HOURS
Language=English
Logon failure: account logon time restriction violation.
.
Language=Russian
Вы не можете войти в систему сейчас из-за ограничений вашей учетной записи.
.
Language=Polish
Błąd logowania: przekroczenie ograniczenia czasu logowania.
.
Language=Romanian
Logon failure: account logon time restriction violation.
.

MessageId=1329
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WORKSTATION
Language=English
Logon failure: user not allowed to log on to this computer.
.
Language=Russian
Этому пользователю не разрешен вход в систему на этом компьютере.
.
Language=Polish
Błąd logowania: użytkownik nie ma zezwolenia na logowanie się w tym komputerze.
.
Language=Romanian
Logon failure: user not allowed to log on to this computer.
.

MessageId=1330
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_EXPIRED
Language=English
Logon failure: the specified account password has expired.
.
Language=Russian
Срок действия пароля для этой учетной записи истек.
.
Language=Polish
Błąd logowania: określone hasło konta wygasło.
.
Language=Romanian
Logon failure: the specified account password has expired.
.

MessageId=1331
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_DISABLED
Language=English
Logon failure: account currently disabled.
.
Language=Russian
Вход этого пользователя в систему невозможен, так как эта учетная запись сейчас отключена.
.
Language=Polish
Błąd logowania: konto jest obecnie wyłączone.
.
Language=Romanian
Logon failure: account currently disabled.
.

MessageId=1332
Severity=Success
Facility=System
SymbolicName=ERROR_NONE_MAPPED
Language=English
No mapping between account names and security IDs was done.
.
Language=Russian
Сопоставление между именами пользователей и идентификаторами безопасности не было произведено.
.
Language=Polish
Nie zostało wykonane mapowanie między nazwami kont a identyfikatorami zabezpieczeń.
.
Language=Romanian
No mapping between account names and security IDs was done.
.

MessageId=1333
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_LUIDS_REQUESTED
Language=English
Too many local user identifiers (LUIDs) were requested at one time.
.
Language=Russian
Одновременно запрошено слишком много локальных кодов пользователей.
.
Language=Polish
Wystąpiło za dużo równoczesnych żądań identyfikatorów użytkowników lokalnych (LUID).
.
Language=Romanian
Too many local user identifiers (LUIDs) were requested at one time.
.

MessageId=1334
Severity=Success
Facility=System
SymbolicName=ERROR_LUIDS_EXHAUSTED
Language=English
No more local user identifiers (LUIDs) are available.
.
Language=Russian
Дополнительные локальные коды пользователей недоступны.
.
Language=Polish
Brak dostępnych identyfikatorów użytkowników lokalnych (LUID).
.
Language=Romanian
No more local user identifiers (LUIDs) are available.
.

MessageId=1335
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SUB_AUTHORITY
Language=English
The subauthority part of a security ID is invalid for this particular use.
.
Language=Russian
Часть "subauthority" идентификатора безопасности недействительна для этого конкретного использования.
.
Language=Polish
Podrzędna część identyfikatora zabezpieczeń jest nieprawidłowa dla tego szczególnego zastosowania.
.
Language=Romanian
The subauthority part of a security ID is invalid for this particular use.
.

MessageId=1336
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACL
Language=English
The access control list (ACL) structure is invalid.
.
Language=Russian
Список управления доступом (ACL) имеет неверную структуру.
.
Language=Polish
Struktura listy kontroli dostępu (ACL) jest nieprawidłowa.
.
Language=Romanian
The access control list (ACL) structure is invalid.
.

MessageId=1337
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SID
Language=English
The security ID structure is invalid.
.
Language=Russian
Идентификатор безопасности имеет неверную структуру.
.
Language=Polish
Struktura identyfikatora zabezpieczenia jest nieprawidłowa.
.
Language=Romanian
The security ID structure is invalid.
.

MessageId=1338
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SECURITY_DESCR
Language=English
The security descriptor structure is invalid.
.
Language=Russian
Дескриптор защиты данных имеет неверную структуру.
.
Language=Polish
Struktura deskryptora zabezpieczeń jest nieprawidłowa.
.
Language=Romanian
The security descriptor structure is invalid.
.

MessageId=1340
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_INHERITANCE_ACL
Language=English
The inherited access control list (ACL) or access control entry (ACE) could not be built.
.
Language=Russian
Не удается построить список управления доступом (ACL) или элемент этого списка (ACE).
.
Language=Polish
Nie można zbudować dziedziczonej listy kontroli dostępu (ACL) lub wpisu kontroli dostępu (ACE).
.
Language=Romanian
The inherited access control list (ACL) or access control entry (ACE) could not be built.
.

MessageId=1341
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_DISABLED
Language=English
The server is currently disabled.
.
Language=Russian
Сервер в настоящее время отключен.
.
Language=Polish
Serwer jest obecnie wyłączony.
.
Language=Romanian
The server is currently disabled.
.

MessageId=1342
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_NOT_DISABLED
Language=English
The server is currently enabled.
.
Language=Russian
Сервер в настоящее время включен.
.
Language=Polish
Serwer jest obecnie włączony.
.
Language=Romanian
The server is currently enabled.
.

MessageId=1343
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ID_AUTHORITY
Language=English
The value provided was an invalid value for an identifier authority.
.
Language=Russian
Указано недопустимое значение для защитного кода.
.
Language=Polish
Podana wartość była nieprawidłowa dla urzędu identyfikatora.
.
Language=Romanian
The value provided was an invalid value for an identifier authority.
.

MessageId=1344
Severity=Success
Facility=System
SymbolicName=ERROR_ALLOTTED_SPACE_EXCEEDED
Language=English
No more memory is available for security information updates.
.
Language=Russian
Недостаточно памяти для обновления сведений, относящихся к защите данных.
.
Language=Polish
Za mało pamięci do aktualizacji informacji o zabezpieczeniach.
.
Language=Romanian
No more memory is available for security information updates.
.

MessageId=1345
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GROUP_ATTRIBUTES
Language=English
The specified attributes are invalid, or incompatible with the attributes for the group as a whole.
.
Language=Russian
Указанные атрибуты неверны или несовместимы с атрибутами группы в целом.
.
Language=Polish
Określone atrybuty są nieprawidłowe lub niezgodne z atrybutami całości grupy.
.
Language=Romanian
The specified attributes are invalid, or incompatible with the attributes for the group as a whole.
.

MessageId=1346
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_IMPERSONATION_LEVEL
Language=English
Either a required impersonation level was not provided, or the provided impersonation level is invalid.
.
Language=Russian
Требуемый уровень олицетворения не обеспечен, или обеспеченный уровень неверен.
.
Language=Polish
Nie został podany poziom personifikacji albo podany poziom jest nieprawidłowy.
.
Language=Romanian
Either a required impersonation level was not provided, or the provided impersonation level is invalid.
.

MessageId=1347
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_OPEN_ANONYMOUS
Language=English
Cannot open an anonymous level security token.
.
Language=Russian
Не удается открыть токен безопасности анонимного уровня.
.
Language=Polish
Nie można otworzyć tokenu o anonimowym poziomie zabezpieczenia.
.
Language=Romanian
Cannot open an anonymous level security token.
.

MessageId=1348
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_VALIDATION_CLASS
Language=English
The validation information class requested was invalid.
.
Language=Russian
Запрошен неправильный класс сведений для проверки.
.
Language=Polish
Żądana klasa informacji sprawdzania poprawności była nieprawidłowa.
.
Language=Romanian
The validation information class requested was invalid.
.

MessageId=1349
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_TOKEN_TYPE
Language=English
The type of the token is inappropriate for its attempted use.
.
Language=Russian
Тип токена не соответствует выполняемой операции.
.
Language=Polish
Typ tokena jest nieodpowiedni dla podjętej próby jego użycia.
.
Language=Romanian
The type of the token is inappropriate for its attempted use.
.

MessageId=1350
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SECURITY_ON_OBJECT
Language=English
Unable to perform a security operation on an object that has no associated security.
.
Language=Russian
Операция, связанная с защитой данных, не может быть выполнена для незащищенного объекта.
.
Language=Polish
Nie można wykonać operacji zabezpieczenia na obiekcie, z którym nie ma skojarzonego zabezpieczenia.
.
Language=Romanian
Unable to perform a security operation on an object that has no associated security.
.

MessageId=1351
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ACCESS_DOMAIN_INFO
Language=English
Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied.
.
Language=Russian
Не удалось получить данные о конфигурации от контроллера домена. Либо он отключен, либо к нему нет доступа.
.
Language=Polish
Nie można odczytać informacji o konfiguracji z kontrolera domeny, ponieważ urządzenie jest niedostępne lub dostęp jest zabroniony.
.
Language=Romanian
Configuration information could not be read from the domain controller, either because the machine is unavailable, or access has been denied.
.

MessageId=1352
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SERVER_STATE
Language=English
The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation.
.
Language=Russian
Диспетчер защиты (SAM) или локальный сервер (LSA) не смог выполнить требуемую операцию.
.
Language=Polish
Menedżer kont zabezpieczeń (SAM) lub lokalny serwer urzędu zabezpieczeń (LSA) był w niewłaściwym stanie do wykonania operacji zabezpieczania.
.
Language=Romanian
The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation.
.

MessageId=1353
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_STATE
Language=English
The domain was in the wrong state to perform the security operation.
.
Language=Russian
Состояние домена не позволило выполнить нужную операцию.
.
Language=Polish
Domena była w niewłaściwym stanie do wykonania operacji zabezpieczania.
.
Language=Romanian
The domain was in the wrong state to perform the security operation.
.

MessageId=1354
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DOMAIN_ROLE
Language=English
This operation is only allowed for the Primary Domain Controller of the domain.
.
Language=Russian
Операция разрешена только для основного контроллера домена.
.
Language=Polish
Ta operacja jest dozwolona tylko dla podstawowego kontrolera domeny.
.
Language=Romanian
This operation is only allowed for the Primary Domain Controller of the domain.
.

MessageId=1355
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_DOMAIN
Language=English
The specified domain either does not exist or could not be contacted.
.
Language=Russian
Указанный домен не существует или к нему невозможно подключиться.
.
Language=Polish
Określona domena nie istnieje lub nie można się z nią skontaktować.
.
Language=Romanian
The specified domain either does not exist or could not be contacted.
.

MessageId=1356
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_EXISTS
Language=English
The specified domain already exists.
.
Language=Russian
Указанный домен уже существует.
.
Language=Polish
Określona domena już istnieje.
.
Language=Romanian
The specified domain already exists.
.

MessageId=1357
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_LIMIT_EXCEEDED
Language=English
An attempt was made to exceed the limit on the number of domains per server.
.
Language=Russian
Была сделана попытка превысить предел на число доменов, обслуживаемых одним сервером.
.
Language=Polish
Podjęto próbę przekroczenia limitu liczby domen na serwer.
.
Language=Romanian
An attempt was made to exceed the limit on the number of domains per server.
.

MessageId=1358
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_CORRUPTION
Language=English
Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk.
.
Language=Russian
Не удается завершить требуемую операцию из-за сбоев в данных на диске или неустранимой ошибки носителя.
.
Language=Polish
Nie można wykonać żądanej operacji, ponieważ wystąpiła katastrofalna awaria nośnika lub uszkodzenie struktury danych na dysku.
.
Language=Romanian
Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk.
.

MessageId=1359
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_ERROR
Language=English
An internal error occurred.
.
Language=Russian
Внутренняя ошибка.
.
Language=Polish
Wystąpił błąd wewnętrzny.
.
Language=Romanian
An internal error occurred.
.

MessageId=1360
Severity=Success
Facility=System
SymbolicName=ERROR_GENERIC_NOT_MAPPED
Language=English
Generic access types were contained in an access mask which should already be mapped to nongeneric types.
.
Language=Russian
Универсальные типы доступа содержатся в маске доступа, которая должна была уже быть связана с нестандартными типами.
.
Language=Polish
Rodzajowe typy dostępu były zawarte w masce dostępu, która powinna być już zamapowana na typy nierodzajowe.
.
Language=Romanian
Generic access types were contained in an access mask which should already be mapped to nongeneric types.
.

MessageId=1361
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DESCRIPTOR_FORMAT
Language=English
A security descriptor is not in the right format (absolute or self-relative).
.
Language=Russian
Дескриптор защиты имеет неверный формат.
.
Language=Polish
Deskryptor zabezpieczeń nie ma prawidłowego formatu (bezwzględnego lub autorelacyjnego).
.
Language=Romanian
A security descriptor is not in the right format (absolute or self-relative).
.

MessageId=1362
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_LOGON_PROCESS
Language=English
The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.
.
Language=Russian
Выполнение запрошенной операции разрешено только для процессов входа в систему. Вызывающий процесс не зарегистрирован как процесс входа в систему.
.
Language=Polish
Żądana akcja jest ograniczona do używania wyłącznie przez procesy logowania. Proces wywołujący nie jest zarejestrowany jako proces logowania.
.
Language=Romanian
The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process.
.

MessageId=1363
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_EXISTS
Language=English
Cannot start a new logon session with an ID that is already in use.
.
Language=Russian
Запуск нового сеанса работы с уже использующимся кодом невозможен.
.
Language=Polish
Nie można uruchomić nowej sesji logowania z identyfikatorem, który jest już w użyciu.
.
Language=Romanian
Cannot start a new logon session with an ID that is already in use.
.

MessageId=1364
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_PACKAGE
Language=English
A specified authentication package is unknown.
.
Language=Russian
Пакет проверки подлинности не опознан.
.
Language=Polish
Określony pakiet uwierzytelniania jest nieznany.
.
Language=Romanian
A specified authentication package is unknown.
.

MessageId=1365
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_LOGON_SESSION_STATE
Language=English
The logon session is not in a state that is consistent with the requested operation.
.
Language=Russian
Текущее состояние сеанса входа в систему не подходит для запрошенной операции.
.
Language=Polish
Sesja logowania jest w stanie niezgodnym z żądaną operacją.
.
Language=Romanian
The logon session is not in a state that is consistent with the requested operation.
.

MessageId=1366
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_SESSION_COLLISION
Language=English
The logon session ID is already in use.
.
Language=Russian
Код сеанса входа в систему уже используется.
.
Language=Polish
Identyfikator sesji logowania jest już w użyciu.
.
Language=Romanian
The logon session ID is already in use.
.

MessageId=1367
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LOGON_TYPE
Language=English
A logon request contained an invalid logon type value.
.
Language=Russian
Режим входа в систему задан неверно.
.
Language=Polish
Żądanie logowania zawierało nieprawidłowy typ wartości logowania.
.
Language=Romanian
A logon request contained an invalid logon type value.
.

MessageId=1368
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_IMPERSONATE
Language=English
Unable to impersonate using a named pipe until data has been read from that pipe.
.
Language=Russian
Невозможно обеспечить олицетворение через именованный канал до тех пор, пока данные не считаны из этого канала.
.
Language=Polish
Dopóki dane są odczytywane z nazwanego potoku, nie można przeprowadzić personifikacji przy jego użyciu.
.
Language=Romanian
Unable to impersonate using a named pipe until data has been read from that pipe.
.

MessageId=1369
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_INVALID_STATE
Language=English
The transaction state of a registry subtree is incompatible with the requested operation.
.
Language=Russian
Операция несовместима с состоянием транзакции для ветви реестра.
.
Language=Polish
Stan transakcji poddrzewa rejestru jest niezgodny z żądaną operacją.
.
Language=Romanian
The transaction state of a registry subtree is incompatible with the requested operation.
.

MessageId=1370
Severity=Success
Facility=System
SymbolicName=ERROR_RXACT_COMMIT_FAILURE
Language=English
An internal security database corruption has been encountered.
.
Language=Russian
База данных защиты повреждена.
.
Language=Polish
Wystąpiło uszkodzenie wewnętrznej bazy danych zabezpieczeń.
.
Language=Romanian
An internal security database corruption has been encountered.
.

MessageId=1371
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_ACCOUNT
Language=English
Cannot perform this operation on built-in accounts.
.
Language=Russian
Операция не предназначена для встроенных учетных записей.
.
Language=Polish
Nie można wykonać tej operacji na kontach wbudowanych.
.
Language=Romanian
Cannot perform this operation on built-in accounts.
.

MessageId=1372
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_GROUP
Language=English
Cannot perform this operation on this built-in special group.
.
Language=Russian
Операция не предназначена для встроенной специальной группы.
.
Language=Polish
Nie można wykonać tej operacji na tej wbudowanej grupie specjalnej.
.
Language=Romanian
Cannot perform this operation on this built-in special group.
.

MessageId=1373
Severity=Success
Facility=System
SymbolicName=ERROR_SPECIAL_USER
Language=English
Cannot perform this operation on this built-in special user.
.
Language=Russian
Операция не предназначена для встроенного специального пользователя.
.
Language=Polish
Nie można wykonać tej operacji na tym wbudowanym użytkowniku specjalnym.
.
Language=Romanian
Cannot perform this operation on this built-in special user.
.

MessageId=1374
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBERS_PRIMARY_GROUP
Language=English
The user cannot be removed from a group because the group is currently the user's primary group.
.
Language=Russian
Невозможно удалить пользователя из группы, так как она является для него основной.
.
Language=Polish
Nie można usunąć użytkownika z grupy, ponieważ grupa jest obecnie podstawową grupą użytkownika.
.
Language=Romanian
The user cannot be removed from a group because the group is currently the user's primary group.
.

MessageId=1375
Severity=Success
Facility=System
SymbolicName=ERROR_TOKEN_ALREADY_IN_USE
Language=English
The token is already in use as a primary token.
.
Language=Russian
Токен уже используется в качестве основного токена.
.
Language=Polish
Token jest już w użyciu jako token podstawowy.
.
Language=Romanian
The token is already in use as a primary token.
.

MessageId=1376
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_ALIAS
Language=English
The specified local group does not exist.
.
Language=Russian
Указанная локальная группа не существует.
.
Language=Polish
Określona grupa lokalna nie istnieje.
.
Language=Romanian
The specified local group does not exist.
.

MessageId=1377
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_NOT_IN_ALIAS
Language=English
The specified account name is not a member of the local group.
.
Language=Russian
Указанная учетная запись не входит в эту группу.
.
Language=Polish
Określona nazwa konta nie jest członkiem grupy.
.
Language=Romanian
The specified account name is not a member of the local group.
.

MessageId=1378
Severity=Success
Facility=System
SymbolicName=ERROR_MEMBER_IN_ALIAS
Language=English
The specified account name is already a member of the local group.
.
Language=Russian
Указанная учетная запись уже входит в эту группу.
.
Language=Polish
Określona nazwa konta jest już członkiem grupy.
.
Language=Romanian
The specified account name is already a member of the local group.
.

MessageId=1379
Severity=Success
Facility=System
SymbolicName=ERROR_ALIAS_EXISTS
Language=English
The specified local group already exists.
.
Language=Russian
Указанная локальная группа уже существует.
.
Language=Polish
Określona grupa lokalna już istnieje.
.
Language=Romanian
The specified local group already exists.
.

MessageId=1380
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_NOT_GRANTED
Language=English
Logon failure: the user has not been granted the requested logon type at this computer.
.
Language=Russian
Вход в систему не произведен: выбранный режим входа для данного пользователя на этом компьютере не предусмотрен.
.
Language=Polish
Błąd logowania: użytkownikowi nie przyznano żądanego typu logowania na tym komputerze.
.
Language=Romanian
Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1381
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SECRETS
Language=English
The maximum number of secrets that may be stored in a single system has been exceeded.
.
Language=Russian
Достигнут предел по количеству защищенных данных/ресурсов для одной системы.
.
Language=Polish
Przekroczono maksymalną liczbę haseł, które mogą być przechowywane w pojedynczym systemie.
.
Language=Romanian
The maximum number of secrets that may be stored in a single system has been exceeded.
.

MessageId=1382
Severity=Success
Facility=System
SymbolicName=ERROR_SECRET_TOO_LONG
Language=English
The length of a secret exceeds the maximum length allowed.
.
Language=Russian
Длина защищенных данных превышает максимально возможную.
.
Language=Polish
Długość hasła przekracza maksymalną dopuszczalną wartość.
.
Language=Romanian
The length of a secret exceeds the maximum length allowed.
.

MessageId=1383
Severity=Success
Facility=System
SymbolicName=ERROR_INTERNAL_DB_ERROR
Language=English
The local security authority database contains an internal inconsistency.
.
Language=Russian
Локальная база данных защиты содержит внутренние несоответствия.
.
Language=Polish
Baza danych urzędu zabezpieczeń lokalnych zawiera wewnętrzną niezgodność.
.
Language=Romanian
The local security authority database contains an internal inconsistency.
.

MessageId=1384
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_CONTEXT_IDS
Language=English
During a logon attempt, the user's security context accumulated too many security IDs.
.
Language=Russian
При попытке входа в систему контекст безопасности пользователя накопил слишком много идентификаторов безопасности.
.
Language=Polish
Podczas próby logowania kontekst zabezpieczeń użytkownika zakumulował za dużo identyfikatorów zabezpieczeń.
.
Language=Romanian
During a logon attempt, the user's security context accumulated too many security IDs.
.

MessageId=1385
Severity=Success
Facility=System
SymbolicName=ERROR_LOGON_TYPE_NOT_GRANTED
Language=English
Logon failure: the user has not been granted the requested logon type at this computer.
.
Language=Russian
Вход в систему не произведен: выбранный режим входа для данного пользователя на этом компьютере не предусмотрен.
.
Language=Polish
Błąd logowania: użytkownikowi nie przyznano żądanego typu logowania na tym komputerze.
.
Language=Romanian
Logon failure: the user has not been granted the requested logon type at this computer.
.

MessageId=1386
Severity=Success
Facility=System
SymbolicName=ERROR_NT_CROSS_ENCRYPTION_REQUIRED
Language=English
A cross-encrypted password is necessary to change a user password.
.
Language=Russian
Для смены пароля необходим зашифрованный пароль.
.
Language=Polish
Do zmiany hasła użytkownika konieczne jest hasło zaszyfrowane krzyżowo.
.
Language=Romanian
A cross-encrypted password is necessary to change a user password.
.

MessageId=1387
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUCH_MEMBER
Language=English
A new member could not be added to or removed from the local group because the member does not exist.
.
Language=Russian
Не удалось добавить или удалить члена локальной группы, так как он не существует.
.
Language=Polish
Nie można dodać nowego członka grupy do grupy lokalnej ani usunąć go z niej, ponieważ ten członek grupy nie istnieje.
.
Language=Romanian
A new member could not be added to or removed from the local group because the member does not exist.
.

MessageId=1388
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEMBER
Language=English
A new member could not be added to a local group because the member has the wrong account type.
.
Language=Russian
Добавление нового члена в локальную группу невозможно, так как он имеет неправильный тип учетной записи.
.
Language=Polish
Nie można dodać nowego członka grupy do grupy lokalnej, ponieważ ten członek grupy ma zły typ konta.
.
Language=Romanian
A new member could not be added to a local group because the member has the wrong account type.
.

MessageId=1389
Severity=Success
Facility=System
SymbolicName=ERROR_TOO_MANY_SIDS
Language=English
Too many security IDs have been specified.
.
Language=Russian
Задано слишком много идентификаторов безопасности.
.
Language=Polish
Określono za dużo identyfikatorów zabezpieczeń.
.
Language=Romanian
Too many security IDs have been specified.
.

MessageId=1390
Severity=Success
Facility=System
SymbolicName=ERROR_LM_CROSS_ENCRYPTION_REQUIRED
Language=English
A cross-encrypted password is necessary to change this user password.
.
Language=Russian
Для смены пароля необходим зашифрованный пароль.
.
Language=Polish
Do zmiany tego hasła użytkownika konieczne jest hasło zaszyfrowane krzyżowo.
.
Language=Romanian
A cross-encrypted password is necessary to change this user password.
.

MessageId=1391
Severity=Success
Facility=System
SymbolicName=ERROR_NO_INHERITANCE
Language=English
Indicates an ACL contains no inheritable components.
.
Language=Russian
Список управления доступом (ACL) не содержит наследуемых компонентов.
.
Language=Polish
Wskazuje, że ACL nie zawiera składników dziedzicznych.
.
Language=Romanian
Indicates an ACL contains no inheritable components.
.

MessageId=1392
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_CORRUPT
Language=English
The file or directory is corrupted and unreadable.
.
Language=Russian
Файл или папка повреждены. Чтение невозможно.
.
Language=Polish
Plik lub katalog jest uszkodzony i nieczytelny.
.
Language=Romanian
The file or directory is corrupted and unreadable.
.

MessageId=1393
Severity=Success
Facility=System
SymbolicName=ERROR_DISK_CORRUPT
Language=English
The disk structure is corrupted and unreadable.
.
Language=Russian
Структура диска повреждена. Чтение невозможно.
.
Language=Polish
Struktura dysku jest uszkodzona i nieczytelna.
.
Language=Romanian
The disk structure is corrupted and unreadable.
.

MessageId=1394
Severity=Success
Facility=System
SymbolicName=ERROR_NO_USER_SESSION_KEY
Language=English
There is no user session key for the specified logon session.
.
Language=Russian
Для заданного сеанса входа в систему отсутствует раздел сеанса пользователя.
.
Language=Polish
Brak klucza sesji użytkownika dla określonej sesji logowania.
.
Language=Romanian
There is no user session key for the specified logon session.
.

MessageId=1395
Severity=Success
Facility=System
SymbolicName=ERROR_LICENSE_QUOTA_EXCEEDED
Language=English
The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept.
.
Language=Russian
Для вызываемой службы действует лицензия на определенное число подключений. В настоящее время создание дополнительных подключений к службе невозможно, так как уже существует максимально допустимое число подключений.
.
Language=Polish
Usługa, do której próbujesz uzyskać dostęp, ma licencję tylko na określoną liczbę połączeń. Nie można obecnie połączyć się z tą usługą, gdyż istnieje już tyle połączeń, ile usługa może zaakceptować.
.
Language=Romanian
The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept.
.

MessageId=1396
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_TARGET_NAME
Language=English
Logon Failure: The target account name is incorrect.
.
Language=Russian
Конечная учетная запись указана неверно.
.
Language=Polish
Błąd logowania: niepoprawna nazwa docelowego konta.
.
Language=Romanian
Logon Failure: The target account name is incorrect.
.

MessageId=1397
Severity=Success
Facility=System
SymbolicName=ERROR_MUTUAL_AUTH_FAILED
Language=English
Mutual Authentication failed. The server's password is out of date at the domain controller.
.
Language=Russian
Ошибка взаимной проверки подлинности. Пароль сервера на контроллере домена устарел.
.
Language=Polish
Wzajemne uwierzytelnienie nie powiodło się. Hasło serwera w kontrolerze domeny jest nieaktualne.
.
Language=Romanian
Mutual Authentication failed. The server's password is out of date at the domain controller.
.

MessageId=1398
Severity=Success
Facility=System
SymbolicName=ERROR_TIME_SKEW
Language=English
There is a time and/or date difference between the client and server.
.
Language=Russian
Существует разница настройки времени и/или даты между клиентом и сервером.
.
Language=Polish
Występuje różnica czasu i/lub daty między klientem i serwerem.
.
Language=Romanian
There is a time and/or date difference between the client and server.
.

MessageId=1399
Severity=Success
Facility=System
SymbolicName=ERROR_CURRENT_DOMAIN_NOT_ALLOWED
Language=English
This operation cannot be performed on the current domain.
.
Language=Russian
Эта операция не может быть выполнена над текущим доменом.
.
Language=Polish
Tej operacji nie można wykonać na bieżącej domenie.
.
Language=Romanian
This operation cannot be performed on the current domain.
.

MessageId=1400
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_HANDLE
Language=English
Invalid window handle.
.
Language=Russian
Недопустимый дескриптор окна.
.
Language=Polish
Nieprawidłowe dojście okna.
.
Language=Romanian
Invalid window handle.
.

MessageId=1401
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MENU_HANDLE
Language=English
Invalid menu handle.
.
Language=Russian
Неверный дескриптор меню.
.
Language=Polish
Nieprawidłowe dojście menu.
.
Language=Romanian
Invalid menu handle.
.

MessageId=1402
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CURSOR_HANDLE
Language=English
Invalid cursor handle.
.
Language=Russian
Неверный дескриптор указателя.
.
Language=Polish
Nieprawidłowe dojście kursora.
.
Language=Romanian
Invalid cursor handle.
.

MessageId=1403
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ACCEL_HANDLE
Language=English
Invalid accelerator table handle.
.
Language=Russian
Неверный дескриптор таблицы сочетаний клавиш.
.
Language=Polish
Nieprawidłowe dojście tabeli przyspieszacza.
.
Language=Romanian
Invalid accelerator table handle.
.

MessageId=1404
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_HANDLE
Language=English
Invalid hook handle.
.
Language=Russian
Неверный дескриптор обработчика.
.
Language=Polish
Nieprawidłowe dojście haka.
.
Language=Romanian
Invalid hook handle.
.

MessageId=1405
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DWP_HANDLE
Language=English
Invalid handle to a multiple-window position structure.
.
Language=Russian
Неверный дескриптор многооконной структуры.
.
Language=Polish
Nieprawidłowe dojście do struktury wielooknowej.
.
Language=Romanian
Invalid handle to a multiple-window position structure.
.

MessageId=1406
Severity=Success
Facility=System
SymbolicName=ERROR_TLW_WITH_WSCHILD
Language=English
Cannot create a top-level child window.
.
Language=Russian
Не удается создать дочернее окно верхнего уровня.
.
Language=Polish
Nie można utworzyć okna podrzędnego najwyższego poziomu.
.
Language=Romanian
Cannot create a top-level child window.
.

MessageId=1407
Severity=Success
Facility=System
SymbolicName=ERROR_CANNOT_FIND_WND_CLASS
Language=English
Cannot find window class.
.
Language=Russian
Не удается найти класс окна.
.
Language=Polish
Nie można odnaleźć klasy okna.
.
Language=Romanian
Cannot find window class.
.

MessageId=1408
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_OF_OTHER_THREAD
Language=English
Invalid window; it belongs to other thread.
.
Language=Russian
Окно принадлежит другому потоку команд.
.
Language=Polish
Nieprawidłowe okno, należy ono do innego wątku.
.
Language=Romanian
Invalid window; it belongs to other thread.
.

MessageId=1409
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_ALREADY_REGISTERED
Language=English
Hot key is already registered.
.
Language=Russian
Назначенная клавиша уже зарегистрирована.
.
Language=Polish
Klawisz dostępu jest już zarejestrowany.
.
Language=Romanian
Hot key is already registered.
.

MessageId=1410
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_ALREADY_EXISTS
Language=English
Class already exists.
.
Language=Russian
Класс уже существует.
.
Language=Polish
Klasa już istnieje.
.
Language=Romanian
Class already exists.
.

MessageId=1411
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_DOES_NOT_EXIST
Language=English
Class does not exist.
.
Language=Russian
Класс не существует.
.
Language=Polish
Klasa nie istnieje.
.
Language=Romanian
Class does not exist.
.

MessageId=1412
Severity=Success
Facility=System
SymbolicName=ERROR_CLASS_HAS_WINDOWS
Language=English
Class still has open windows.
.
Language=Russian
Class still has open windows.
.
Language=Polish
Klasa ma wciąż otwarte okna.
.
Language=Romanian
Class still has open windows.
.

MessageId=1413
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_INDEX
Language=English
Invalid index.
.
Language=Russian
Invalid index.
.
Language=Polish
Nieprawidłowy indeks.
.
Language=Romanian
Invalid index.
.

MessageId=1414
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ICON_HANDLE
Language=English
Invalid icon handle.
.
Language=Russian
Invalid icon handle.
.
Language=Polish
Nieprawidłowe dojście ikony.
.
Language=Romanian
Invalid icon handle.
.

MessageId=1415
Severity=Success
Facility=System
SymbolicName=ERROR_PRIVATE_DIALOG_INDEX
Language=English
Using private DIALOG window words.
.
Language=Russian
Using private DIALOG window words.
.
Language=Polish
Używane są słowa prywatnego okna DIALOG.
.
Language=Romanian
Using private DIALOG window words.
.

MessageId=1416
Severity=Success
Facility=System
SymbolicName=ERROR_LISTBOX_ID_NOT_FOUND
Language=English
The list box identifier was not found.
.
Language=Russian
The list box identifier was not found.
.
Language=Polish
Nie znaleziono identyfikatora pola listy.
.
Language=Romanian
The list box identifier was not found.
.

MessageId=1417
Severity=Success
Facility=System
SymbolicName=ERROR_NO_WILDCARD_CHARACTERS
Language=English
No wildcards were found.
.
Language=Russian
No wildcards were found.
.
Language=Polish
Nie znaleziono symboli wieloznacznych.
.
Language=Romanian
No wildcards were found.
.

MessageId=1418
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPBOARD_NOT_OPEN
Language=English
Thread does not have a clipboard open.
.
Language=Russian
Thread does not have a clipboard open.
.
Language=Polish
Wątek nie ma otwartego Schowka.
.
Language=Romanian
Thread does not have a clipboard open.
.

MessageId=1419
Severity=Success
Facility=System
SymbolicName=ERROR_HOTKEY_NOT_REGISTERED
Language=English
Hot key is not registered.
.
Language=Russian
Hot key is not registered.
.
Language=Polish
Klawisz dostępu nie jest zarejestrowany.
.
Language=Romanian
Hot key is not registered.
.

MessageId=1420
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_DIALOG
Language=English
The window is not a valid dialog window.
.
Language=Russian
The window is not a valid dialog window.
.
Language=Polish
Okno nie jest prawidłowym oknem dialogowym.
.
Language=Romanian
The window is not a valid dialog window.
.

MessageId=1421
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROL_ID_NOT_FOUND
Language=English
Control ID not found.
.
Language=Russian
Control ID not found.
.
Language=Polish
Nie można odnaleźć identyfikatora formantu.
.
Language=Romanian
Control ID not found.
.

MessageId=1422
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMBOBOX_MESSAGE
Language=English
Invalid message for a combo box because it does not have an edit control.
.
Language=Russian
Invalid message for a combo box because it does not have an edit control.
.
Language=Polish
Nieprawidłowy komunikat dla pola kombi, ponieważ nie ma ono formantu edycyjnego.
.
Language=Romanian
Invalid message for a combo box because it does not have an edit control.
.

MessageId=1423
Severity=Success
Facility=System
SymbolicName=ERROR_WINDOW_NOT_COMBOBOX
Language=English
The window is not a combo box.
.
Language=Russian
The window is not a combo box.
.
Language=Polish
Okno nie jest polem kombi.
.
Language=Romanian
The window is not a combo box.
.

MessageId=1424
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_EDIT_HEIGHT
Language=English
Height must be less than 256.
.
Language=Russian
Height must be less than 256.
.
Language=Polish
Wysokość musi być mniejsza niż 256.
.
Language=Romanian
Height must be less than 256.
.

MessageId=1425
Severity=Success
Facility=System
SymbolicName=ERROR_DC_NOT_FOUND
Language=English
Invalid device context (DC) handle.
.
Language=Russian
Invalid device context (DC) handle.
.
Language=Polish
Nieprawidłowe dojście kontekstu urządzenia (DC).
.
Language=Romanian
Invalid device context (DC) handle.
.

MessageId=1426
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HOOK_FILTER
Language=English
Invalid hook procedure type.
.
Language=Russian
Invalid hook procedure type.
.
Language=Polish
Nieprawidłowy typ procedury haka.
.
Language=Romanian
Invalid hook procedure type.
.

MessageId=1427
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FILTER_PROC
Language=English
Invalid hook procedure.
.
Language=Russian
Invalid hook procedure.
.
Language=Polish
Nieprawidłowa procedura haka.
.
Language=Romanian
Invalid hook procedure.
.

MessageId=1428
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NEEDS_HMOD
Language=English
Cannot set nonlocal hook without a module handle.
.
Language=Russian
Cannot set nonlocal hook without a module handle.
.
Language=Polish
Nie można ustawić haka nielokalnego bez dojścia modułu.
.
Language=Romanian
Cannot set nonlocal hook without a module handle.
.

MessageId=1429
Severity=Success
Facility=System
SymbolicName=ERROR_GLOBAL_ONLY_HOOK
Language=English
This hook procedure can only be set globally.
.
Language=Russian
This hook procedure can only be set globally.
.
Language=Polish
Ta procedura haka może być ustawiona tylko globalnie.
.
Language=Romanian
This hook procedure can only be set globally.
.

MessageId=1430
Severity=Success
Facility=System
SymbolicName=ERROR_JOURNAL_HOOK_SET
Language=English
The journal hook procedure is already installed.
.
Language=Russian
The journal hook procedure is already installed.
.
Language=Polish
Procedura haka dziennika jest już zainstalowana.
.
Language=Romanian
The journal hook procedure is already installed.
.

MessageId=1431
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_NOT_INSTALLED
Language=English
The hook procedure is not installed.
.
Language=Russian
The hook procedure is not installed.
.
Language=Polish
Procedura haka nie jest zainstalowana.
.
Language=Romanian
The hook procedure is not installed.
.

MessageId=1432
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LB_MESSAGE
Language=English
Invalid message for single-selection list box.
.
Language=Russian
Invalid message for single-selection list box.
.
Language=Polish
Nieprawidłowy komunikat dla pola listy z pojedynczym wyborem.
.
Language=Romanian
Invalid message for single-selection list box.
.

MessageId=1433
Severity=Success
Facility=System
SymbolicName=ERROR_SETCOUNT_ON_BAD_LB
Language=English
LB_SETCOUNT sent to non-lazy list box.
.
Language=Russian
LB_SETCOUNT sent to non-lazy list box.
.
Language=Polish
Polecenie LB_SETCOUNT przesłane do pola listy non-lazy.
.
Language=Romanian
LB_SETCOUNT sent to non-lazy list box.
.

MessageId=1434
Severity=Success
Facility=System
SymbolicName=ERROR_LB_WITHOUT_TABSTOPS
Language=English
This list box does not support tab stops.
.
Language=Russian
This list box does not support tab stops.
.
Language=Polish
To pole listy nie obsługuje tabulatorów.
.
Language=Romanian
This list box does not support tab stops.
.

MessageId=1435
Severity=Success
Facility=System
SymbolicName=ERROR_DESTROY_OBJECT_OF_OTHER_THREAD
Language=English
Cannot destroy object created by another thread.
.
Language=Russian
Cannot destroy object created by another thread.
.
Language=Polish
Nie można zniszczyć obiektu utworzonego przez inny wątek.
.
Language=Romanian
Cannot destroy object created by another thread.
.

MessageId=1436
Severity=Success
Facility=System
SymbolicName=ERROR_CHILD_WINDOW_MENU
Language=English
Child windows cannot have menus.
.
Language=Russian
Child windows cannot have menus.
.
Language=Polish
Okna podrzędne nie mogą mieć menu.
.
Language=Romanian
Child windows cannot have menus.
.

MessageId=1437
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_MENU
Language=English
The window does not have a system menu.
.
Language=Russian
The window does not have a system menu.
.
Language=Polish
Okno nie ma menu systemowego.
.
Language=Romanian
The window does not have a system menu.
.

MessageId=1438
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MSGBOX_STYLE
Language=English
Invalid message box style.
.
Language=Russian
Invalid message box style.
.
Language=Polish
Nieprawidłowy styl okna komunikatu.
.
Language=Romanian
Invalid message box style.
.

MessageId=1439
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SPI_VALUE
Language=English
Invalid system-wide (SPI_*) parameter.
.
Language=Russian
Invalid system-wide (SPI_*) parameter.
.
Language=Polish
Nieprawidłowy parametr systemowy (SPI_*).
.
Language=Romanian
Invalid system-wide (SPI_*) parameter.
.

MessageId=1440
Severity=Success
Facility=System
SymbolicName=ERROR_SCREEN_ALREADY_LOCKED
Language=English
Screen already locked.
.
Language=Russian
Screen already locked.
.
Language=Polish
Ekran jest już zablokowany.
.
Language=Romanian
Screen already locked.
.

MessageId=1441
Severity=Success
Facility=System
SymbolicName=ERROR_HWNDS_HAVE_DIFF_PARENT
Language=English
All handles to windows in a multiple-window position structure must have the same parent.
.
Language=Russian
All handles to windows in a multiple-window position structure must have the same parent.
.
Language=Polish
Wszystkie dojścia okien w strukturze o wielu pozycjach okien muszą mieć to samo okno nadrzędne.
.
Language=Romanian
All handles to windows in a multiple-window position structure must have the same parent.
.

MessageId=1442
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CHILD_WINDOW
Language=English
The window is not a child window.
.
Language=Russian
The window is not a child window.
.
Language=Polish
Okno nie jest oknem podrzędnym.
.
Language=Romanian
The window is not a child window.
.

MessageId=1443
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_GW_COMMAND
Language=English
Invalid GW_* command.
.
Language=Russian
Invalid GW_* command.
.
Language=Polish
Nieprawidłowe polecenie GW_* .
.
Language=Romanian
Invalid GW_* command.
.

MessageId=1444
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_THREAD_ID
Language=English
Invalid thread identifier.
.
Language=Russian
Invalid thread identifier.
.
Language=Polish
Nieprawidłowy identyfikator wątku.
.
Language=Romanian
Invalid thread identifier.
.

MessageId=1445
Severity=Success
Facility=System
SymbolicName=ERROR_NON_MDICHILD_WINDOW
Language=English
Cannot process a message from a window that is not a multiple document interface (MDI) window.
.
Language=Russian
Cannot process a message from a window that is not a multiple document interface (MDI) window.
.
Language=Polish
Nie można przetworzyć komunikatu z okna, które nie jest oknem interfejsu dokumentu wielokrotnego (MDI).
.
Language=Romanian
Cannot process a message from a window that is not a multiple document interface (MDI) window.
.

MessageId=1446
Severity=Success
Facility=System
SymbolicName=ERROR_POPUP_ALREADY_ACTIVE
Language=English
Popup menu already active.
.
Language=Russian
Popup menu already active.
.
Language=Polish
Menu podręczne jest już aktywne.
.
Language=Romanian
Popup menu already active.
.

MessageId=1447
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SCROLLBARS
Language=English
The window does not have scroll bars.
.
Language=Russian
The window does not have scroll bars.
.
Language=Polish
Okno nie ma pasków przewijania.
.
Language=Romanian
The window does not have scroll bars.
.

MessageId=1448
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SCROLLBAR_RANGE
Language=English
Scroll bar range cannot be greater than MAXLONG.
.
Language=Russian
Scroll bar range cannot be greater than MAXLONG.
.
Language=Polish
Zakres paska przewijania nie może być większy niż wartość MAXLONG.
.
Language=Romanian
Scroll bar range cannot be greater than MAXLONG.
.

MessageId=1449
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SHOWWIN_COMMAND
Language=English
Cannot show or remove the window in the way specified.
.
Language=Russian
Cannot show or remove the window in the way specified.
.
Language=Polish
Nie można wyświetić ani usunąć okna określoną metodą.
.
Language=Romanian
Cannot show or remove the window in the way specified.
.

MessageId=1450
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SYSTEM_RESOURCES
Language=English
Insufficient system resources exist to complete the requested service.
.
Language=Russian
Insufficient system resources exist to complete the requested service.
.
Language=Polish
Zasoby systemowe nie wystarczają do ukończenia żądanej usługi.
.
Language=Romanian
Insufficient system resources exist to complete the requested service.
.

MessageId=1451
Severity=Success
Facility=System
SymbolicName=ERROR_NONPAGED_SYSTEM_RESOURCES
Language=English
Insufficient system resources exist to complete the requested service.
.
Language=Russian
Insufficient system resources exist to complete the requested service.
.
Language=Polish
Zasoby systemowe nie wystarczają do ukończenia żądanej usługi.
.
Language=Romanian
Insufficient system resources exist to complete the requested service.
.

MessageId=1452
Severity=Success
Facility=System
SymbolicName=ERROR_PAGED_SYSTEM_RESOURCES
Language=English
Insufficient system resources exist to complete the requested service.
.
Language=Russian
Insufficient system resources exist to complete the requested service.
.
Language=Polish
Zasoby systemowe nie wystarczają do ukończenia żądanej usługi.
.
Language=Romanian
Insufficient system resources exist to complete the requested service.
.

MessageId=1453
Severity=Success
Facility=System
SymbolicName=ERROR_WORKING_SET_QUOTA
Language=English
Insufficient quota to complete the requested service.
.
Language=Russian
Insufficient quota to complete the requested service.
.
Language=Polish
Przydział jest niewystarczający do ukończenia żądanej usługi.
.
Language=Romanian
Insufficient quota to complete the requested service.
.

MessageId=1454
Severity=Success
Facility=System
SymbolicName=ERROR_PAGEFILE_QUOTA
Language=English
Insufficient quota to complete the requested service.
.
Language=Russian
Insufficient quota to complete the requested service.
.
Language=Polish
Przydział jest niewystarczający do ukończenia żądanej usługi.
.
Language=Romanian
Insufficient quota to complete the requested service.
.

MessageId=1455
Severity=Success
Facility=System
SymbolicName=ERROR_COMMITMENT_LIMIT
Language=English
The paging file is too small for this operation to complete.
.
Language=Russian
The paging file is too small for this operation to complete.
.
Language=Polish
Plik stronicowania jest za mały do ukończenia tej operacji.
.
Language=Romanian
The paging file is too small for this operation to complete.
.

MessageId=1456
Severity=Success
Facility=System
SymbolicName=ERROR_MENU_ITEM_NOT_FOUND
Language=English
A menu item was not found.
.
Language=Russian
A menu item was not found.
.
Language=Polish
Nie odnaleziono elementu menu.
.
Language=Romanian
A menu item was not found.
.

MessageId=1457
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_KEYBOARD_HANDLE
Language=English
Invalid keyboard layout handle.
.
Language=Russian
Invalid keyboard layout handle.
.
Language=Polish
Nieprawidłowe dojście układu klawiatury.
.
Language=Romanian
Invalid keyboard layout handle.
.

MessageId=1458
Severity=Success
Facility=System
SymbolicName=ERROR_HOOK_TYPE_NOT_ALLOWED
Language=English
Hook type not allowed.
.
Language=Russian
Hook type not allowed.
.
Language=Polish
Niedozwolony typ haka.
.
Language=Romanian
Hook type not allowed.
.

MessageId=1459
Severity=Success
Facility=System
SymbolicName=ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION
Language=English
This operation requires an interactive window station.
.
Language=Russian
This operation requires an interactive window station.
.
Language=Polish
Ta operacja wymaga interakcyjnej stacji z systemem Windows.
.
Language=Romanian
This operation requires an interactive window station.
.

MessageId=1460
Severity=Success
Facility=System
SymbolicName=ERROR_TIMEOUT
Language=English
This operation returned because the timeout period expired.
.
Language=Russian
This operation returned because the timeout period expired.
.
Language=Polish
Operacja została zwrócona, ponieważ przekroczono limit czasu.
.
Language=Romanian
This operation returned because the timeout period expired.
.

MessageId=1461
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MONITOR_HANDLE
Language=English
Invalid monitor handle.
.
Language=Russian
Invalid monitor handle.
.
Language=Polish
Nieprawidłowe dojście do monitora.
.
Language=Romanian
Invalid monitor handle.
.

MessageId=1500
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CORRUPT
Language=English
The event log file is corrupted.
.
Language=Russian
The event log file is corrupted.
.
Language=Polish
Plik dziennika zdarzeń jest uszkodzony.
.
Language=Romanian
The event log file is corrupted.
.

MessageId=1501
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_CANT_START
Language=English
No event log file could be opened, so the event logging service did not start.
.
Language=Russian
No event log file could be opened, so the event logging service did not start.
.
Language=Polish
Nie można uruchomić usługi rejestrowania zdarzeń, ponieważ nie można otworzyć pliku dziennika zdarzeń.
.
Language=Romanian
No event log file could be opened, so the event logging service did not start.
.

MessageId=1502
Severity=Success
Facility=System
SymbolicName=ERROR_LOG_FILE_FULL
Language=English
The event log file is full.
.
Language=Russian
The event log file is full.
.
Language=Polish
Plik dziennika zdarzeń jest zapełniony.
.
Language=Romanian
The event log file is full.
.

MessageId=1503
Severity=Success
Facility=System
SymbolicName=ERROR_EVENTLOG_FILE_CHANGED
Language=English
The event log file has changed between read operations.
.
Language=Russian
The event log file has changed between read operations.
.
Language=Polish
Plik dziennika zdarzeń zmienił się między operacjami odczytu.
.
Language=Romanian
The event log file has changed between read operations.
.

MessageId=1601
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SERVICE_FAILURE
Language=English
The ReactOS Installer service could not be accessed. This can occur if you are running ReactOS in safe mode, or if the ReactOS Installer is not correctly installed. Contact your support personnel for assistance.
.
Language=Russian
The ReactOS Installer service could not be accessed. This can occur if you are running ReactOS in safe mode, or if the ReactOS Installer is not correctly installed. Contact your support personnel for assistance.
.
Language=Polish
Nie można uzyskać dostępu do usługi Instalator ReactOS. Może mieć to miejsce, jeśli system ReactOS jest uruchomiony w trybie awaryjnym lub Instalator ReactOS jest niepoprawnie zainstalowany. Skontaktuj się z pomocą techniczną, aby uzyskać pomoc.
.
Language=Romanian
The ReactOS Installer service could not be accessed. This can occur if you are running ReactOS in safe mode, or if the ReactOS Installer is not correctly installed. Contact your support personnel for assistance.
.

MessageId=1602
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_USEREXIT
Language=English
User cancelled installation.
.
Language=Russian
User cancelled installation.
.
Language=Polish
Użytkownik anulował instalację.
.
Language=Romanian
User cancelled installation.
.

MessageId=1603
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_FAILURE
Language=English
Fatal error during installation.
.
Language=Russian
Fatal error during installation.
.
Language=Polish
Błąd krytyczny podczas instalacji.
.
Language=Romanian
Fatal error during installation.
.

MessageId=1604
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SUSPEND
Language=English
Installation suspended, incomplete.
.
Language=Russian
Installation suspended, incomplete.
.
Language=Polish
Instalacja wstrzymana, nieukończona.
.
Language=Romanian
Installation suspended, incomplete.
.

MessageId=1605
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRODUCT
Language=English
This action is only valid for products that are currently installed.
.
Language=Russian
This action is only valid for products that are currently installed.
.
Language=Polish
Akcja ta jest prawidłowa tylko w odniesieniu do produktów, które są obecnie zainstalowane.
.
Language=Romanian
This action is only valid for products that are currently installed.
.

MessageId=1606
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_FEATURE
Language=English
Feature ID not registered.
.
Language=Russian
Feature ID not registered.
.
Language=Polish
Niezarejestrowany identyfikator cechy.
.
Language=Romanian
Feature ID not registered.
.

MessageId=1607
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_COMPONENT
Language=English
Component ID not registered.
.
Language=Russian
Component ID not registered.
.
Language=Polish
Niezarejestrowany identyfikator składnika.
.
Language=Romanian
Component ID not registered.
.

MessageId=1608
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PROPERTY
Language=English
Unknown property.
.
Language=Russian
Unknown property.
.
Language=Polish
Nieznana właściwość.
.
Language=Romanian
Unknown property.
.

MessageId=1609
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_HANDLE_STATE
Language=English
Handle is in an invalid state.
.
Language=Russian
Handle is in an invalid state.
.
Language=Polish
Nieprawidłowy stan dojścia.
.
Language=Romanian
Handle is in an invalid state.
.

MessageId=1610
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_CONFIGURATION
Language=English
The configuration data for this product is corrupt. Contact your support personnel.
.
Language=Russian
The configuration data for this product is corrupt. Contact your support personnel.
.
Language=Polish
Dane konfiguracyjne tego produktu są uszkodzone. Skontaktuj się z działem Pomocy technicznej.
.
Language=Romanian
The configuration data for this product is corrupt. Contact your support personnel.
.

MessageId=1611
Severity=Success
Facility=System
SymbolicName=ERROR_INDEX_ABSENT
Language=English
Component qualifier not present.
.
Language=Russian
Component qualifier not present.
.
Language=Polish
Brak kwalifikatora składnika.
.
Language=Romanian
Component qualifier not present.
.

MessageId=1612
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_SOURCE_ABSENT
Language=English
The installation source for this product is not available. Verify that the source exists and that you can access it.
.
Language=Russian
The installation source for this product is not available. Verify that the source exists and that you can access it.
.
Language=Polish
Źródło instalacji dla tego produktu nie jest dostępne. Sprawdź, czy źródło istnieje i czy możesz uzyskać do niego dostęp.
.
Language=Romanian
The installation source for this product is not available. Verify that the source exists and that you can access it.
.

MessageId=1613
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_VERSION
Language=English
This installation package cannot be installed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.
Language=Russian
This installation package cannot be installed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.
Language=Polish
Usługa Instalator ReactOS nie może zainstalować tego pakietu instalacyjnego. Musisz zainstalować dodatek ReactOS Service Pack, zawierający nowszą wersję usługi Instalator ReactOS.
.
Language=Romanian
This installation package cannot be installed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.

MessageId=1614
Severity=Success
Facility=System
SymbolicName=ERROR_PRODUCT_UNINSTALLED
Language=English
Product is uninstalled.
.
Language=Russian
Product is uninstalled.
.
Language=Polish
Produkt jest odinstalowany.
.
Language=Romanian
Product is uninstalled.
.

MessageId=1615
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_QUERY_SYNTAX
Language=English
SQL query syntax invalid or unsupported.
.
Language=Russian
SQL query syntax invalid or unsupported.
.
Language=Polish
Nieprawidłowa lub nieobsługiwana składnia zapytania SQL.
.
Language=Romanian
SQL query syntax invalid or unsupported.
.

MessageId=1616
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FIELD
Language=English
Record field does not exist.
.
Language=Russian
Record field does not exist.
.
Language=Polish
Pole rekordu nie istnieje.
.
Language=Romanian
Record field does not exist.
.

MessageId=1617
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_REMOVED
Language=English
The device has been removed.
.
Language=Russian
The device has been removed.
.
Language=Polish
Urządzenie zostało usunięte.
.
Language=Romanian
The device has been removed.
.

MessageId=1618
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_ALREADY_RUNNING
Language=English
Another installation is already in progress. Complete that installation before proceeding with this install.
.
Language=Russian
Another installation is already in progress. Complete that installation before proceeding with this install.
.
Language=Polish
Trwa inna instalacja. Ukończ ją, zanim zaczniesz kontynuować bieżącą.
.
Language=Romanian
Another installation is already in progress. Complete that installation before proceeding with this install.
.

MessageId=1619
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_OPEN_FAILED
Language=English
This installation package could not be opened. Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
.
Language=Russian
This installation package could not be opened. Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
.
Language=Polish
Nie można otworzyć tego pakietu instalacyjnego. Sprawdź, czy ten pakiet istnieje i czy masz do niego dostęp, albo skontaktuj się z dostawcą aplikacji w celu sprawdzenia, czy jest to prawidłowy pakiet Instalatora Windows.
.
Language=Romanian
This installation package could not be opened. Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1620
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_INVALID
Language=English
This installation package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer package.
.
Language=Russian
This installation package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer package.
.
Language=Polish
Nie można otworzyć tego pakietu instalacyjnego. Skontaktuj się z dostawcą aplikacji w celu sprawdzenia, czy jest to prawidłowy pakiet poprawek Instalatora Windows.
.
Language=Romanian
This installation package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer package.
.

MessageId=1621
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_UI_FAILURE
Language=English
There was an error starting the ReactOS Installer service user interface. Contact your support personnel.
.
Language=Russian
There was an error starting the ReactOS Installer service user interface. Contact your support personnel.
.
Language=Polish
Podczas uruchamiania interfejsu użytkownika usługi Instalator ReactOS wystąpił błąd. Skontaktuj się z personelem technicznym.
.
Language=Romanian
There was an error starting the ReactOS Installer service user interface. Contact your support personnel.
.

MessageId=1622
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_LOG_FAILURE
Language=English
Error opening installation log file. Verify that the specified log file location exists and that you can write to it.
.
Language=Russian
Error opening installation log file. Verify that the specified log file location exists and that you can write to it.
.
Language=Polish
Błąd podczas otwierania pliku dziennika instalacji. Sprawdź, czy istnieje określona lokalizacja pliku dziennika i czy można w niej zapisywać.
.
Language=Romanian
Error opening installation log file. Verify that the specified log file location exists and that you can write to it.
.

MessageId=1623
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_LANGUAGE_UNSUPPORTED
Language=English
The language of this installation package is not supported by your system.
.
Language=Russian
The language of this installation package is not supported by your system.
.
Language=Polish
Język stosowany w tym pakiecie instalacyjnym nie jest obsługiwany przez system.
.
Language=Romanian
The language of this installation package is not supported by your system.
.

MessageId=1624
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TRANSFORM_FAILURE
Language=English
Error applying transforms. Verify that the specified transform paths are valid.
.
Language=Russian
Error applying transforms. Verify that the specified transform paths are valid.
.
Language=Polish
Błąd podczas przeprowadzania transformacji. Sprawdź, czy podane ścieżki transformacji są prawidłowe.
.
Language=Romanian
Error applying transforms. Verify that the specified transform paths are valid.
.

MessageId=1625
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PACKAGE_REJECTED
Language=English
This installation is forbidden by system policy. Contact your system administrator.
.
Language=Russian
This installation is forbidden by system policy. Contact your system administrator.
.
Language=Polish
Ta instalacja jest zabroniona przez zasady systemowe. Skontaktuj się z administratorem systemu.
.
Language=Romanian
This installation is forbidden by system policy. Contact your system administrator.
.

MessageId=1626
Severity=Success
Facility=System
SymbolicName=ERROR_FUNCTION_NOT_CALLED
Language=English
Function could not be executed.
.
Language=Russian
Function could not be executed.
.
Language=Polish
Nie można wykonać funkcji.
.
Language=Romanian
Function could not be executed.
.

MessageId=1627
Severity=Success
Facility=System
SymbolicName=ERROR_FUNCTION_FAILED
Language=English
Function failed during execution.
.
Language=Russian
Function failed during execution.
.
Language=Polish
Niepowodzenie funkcji podczas jej wykonywania.
.
Language=Romanian
Function failed during execution.
.

MessageId=1628
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TABLE
Language=English
Invalid or unknown table specified.
.
Language=Russian
Invalid or unknown table specified.
.
Language=Polish
Podano nieprawidłową lub nieznaną tabelę.
.
Language=Romanian
Invalid or unknown table specified.
.

MessageId=1629
Severity=Success
Facility=System
SymbolicName=ERROR_DATATYPE_MISMATCH
Language=English
Data supplied is of wrong type.
.
Language=Russian
Data supplied is of wrong type.
.
Language=Polish
Dostarczono dane nieprawidłowego typu.
.
Language=Romanian
Data supplied is of wrong type.
.

MessageId=1630
Severity=Success
Facility=System
SymbolicName=ERROR_UNSUPPORTED_TYPE
Language=English
Data of this type is not supported.
.
Language=Russian
Data of this type is not supported.
.
Language=Polish
Dane tego typu nie są obsługiwane.
.
Language=Romanian
Data of this type is not supported.
.

MessageId=1631
Severity=Success
Facility=System
SymbolicName=ERROR_CREATE_FAILED
Language=English
The ReactOS Installer service failed to start. Contact your support personnel.
.
Language=Russian
The ReactOS Installer service failed to start. Contact your support personnel.
.
Language=Polish
Nie można uruchomić usługi Instalator ReactOS. Skontaktuj się z działem Pomocy technicznej.
.
Language=Romanian
The ReactOS Installer service failed to start. Contact your support personnel.
.

MessageId=1632
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TEMP_UNWRITABLE
Language=English
The Temp folder is on a drive that is full or inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
.
Language=Russian
The Temp folder is on a drive that is full or inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
.
Language=Polish
Folder Temp znajduje się na dysku, który jest albo zapełniony, albo niedostępny. Zwolnij miejsce na dysku lub zweryfikuj, że masz uprawnienia do zapisu w folderze Temp.
.
Language=Romanian
The Temp folder is on a drive that is full or inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
.

MessageId=1633
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_PLATFORM_UNSUPPORTED
Language=English
This installation package is not supported by this processor type. Contact your product vendor.
.
Language=Russian
This installation package is not supported by this processor type. Contact your product vendor.
.
Language=Polish
Ten pakiet instalacyjny nie jest obsługiwany przez ten typ procesora. Skontaktuj się z dostawcą produktu.
.
Language=Romanian
This installation package is not supported by this processor type. Contact your product vendor.
.

MessageId=1634
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_NOTUSED
Language=English
Component not used on this computer.
.
Language=Russian
Component not used on this computer.
.
Language=Polish
Składnik nieużywany w tym komputerze.
.
Language=Romanian
Component not used on this computer.
.

MessageId=1635
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_OPEN_FAILED
Language=English
This patch package could not be opened. Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
.
Language=Russian
This patch package could not be opened. Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
.
Language=Polish
Nie można otworzyć tego pakietu aktualizacji. Sprawdź, czy ten pakiet istnieje i czy masz do niego dostęp, albo skontaktuj się z dostawcą aplikacji w celu sprawdzenia, czy jest to prawidłowy pakiet aktualizacji Instalatora Windows.
.
Language=Romanian
This patch package could not be opened. Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1636
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_INVALID
Language=English
This patch package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer patch package.
.
Language=Russian
This patch package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer patch package.
.
Language=Polish
Nie można otworzyć tego pakietu aktualizacji. Skontaktuj się z dostawcą aplikacji w celu sprawdzenia, czy jest to prawidłowy pakiet aktualizacji Instalatora Windows.
.
Language=Romanian
This patch package could not be opened. Contact the application vendor to verify that this is a valid Windows Installer patch package.
.

MessageId=1637
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_UNSUPPORTED
Language=English
This patch package cannot be processed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.
Language=Russian
This patch package cannot be processed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.
Language=Polish
Usługa Instalator ReactOS nie może przetworzyć tego pakietu aktualizacji. Musisz zainstalować dodatek ReactOS Service Pack, zawierający nowszą wersję usługi Instalator ReactOS.
.
Language=Romanian
This patch package cannot be processed by the ReactOS Installer service. You must install a ReactOS service pack that contains a newer version of the ReactOS Installer service.
.

MessageId=1638
Severity=Success
Facility=System
SymbolicName=ERROR_PRODUCT_VERSION
Language=English
Another version of this product is already installed. Installation of this version cannot continue. To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
.
Language=Russian
Another version of this product is already installed. Installation of this version cannot continue. To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
.
Language=Polish
Inna wersja tego produktu jest już zainstalowana na tym komputerze. Nie można kontynuować instalowania tej wersji. Aby skonfigurować lub usunąć istniejącą wersję tego produktu, użyj aplikacji Dodaj/Usuń Programy z Panelu sterowania.
.
Language=Romanian
Another version of this product is already installed. Installation of this version cannot continue. To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
.

MessageId=1639
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COMMAND_LINE
Language=English
Invalid command line argument. Consult the Windows Installer SDK for detailed command line help.
.
Language=Russian
Invalid command line argument. Consult the Windows Installer SDK for detailed command line help.
.
Language=Polish
Nieprawidłowy argument wiersza polecenia. Szczegółowe informacje na temat wiersza polecenia można znaleźć w pakiecie SDK Instalatora Windows.
.
Language=Romanian
Invalid command line argument. Consult the Windows Installer SDK for detailed command line help.
.

MessageId=1640
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_REMOTE_DISALLOWED
Language=English
Only administrators have permission to add, remove, or configure server software during a Terminal Services remote session. If you want to install or configure software on the server, contact your network administrator.
.
Language=Russian
Only administrators have permission to add, remove, or configure server software during a Terminal Services remote session. If you want to install or configure software on the server, contact your network administrator.
.
Language=Polish
Tylko administratorzy są uprawnieni do dodawania, usuwania lub konfigurowania oprogramowania serwera podczas zdalnych sesji usług Terminala. Jeśli chcesz zainstalować lub skonfigurować jakieś oprogramowanie na serwerze, musisz skontaktować się z administratorem sieci.
.
Language=Romanian
Only administrators have permission to add, remove, or configure server software during a Terminal Services remote session. If you want to install or configure software on the server, contact your network administrator.
.

MessageId=1641
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_REBOOT_INITIATED
Language=English
The requested operation completed successfully. The system will be restarted so the changes can take effect.
.
Language=Russian
The requested operation completed successfully. The system will be restarted so the changes can take effect.
.
Language=Polish
Żądana operacja została pomyślnie ukończona. Aby zmiany zostały wprowadzone, nastąpi ponowne uruchomienie systemu.
.
Language=Romanian
The requested operation completed successfully. The system will be restarted so the changes can take effect.
.

MessageId=1642
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_TARGET_NOT_FOUND
Language=English
The upgrade patch cannot be installed by the ReactOS Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch.
.
Language=Russian
The upgrade patch cannot be installed by the ReactOS Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch.
.
Language=Polish
Uaktualnienia nie można zainstalować przez usługę Instalator ReactOS, ponieważ nie ma programu do uaktualnienia albo uaktualnienie jest przeznaczone do innej wersji tego programu. Sprawdź, czy program, który ma być uaktualniony, znajduje się na dysku i czy masz prawidłowe uaktualnienie.
.
Language=Romanian
The upgrade patch cannot be installed by the ReactOS Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch.
.

MessageId=1643
Severity=Success
Facility=System
SymbolicName=ERROR_PATCH_PACKAGE_REJECTED
Language=English
The patch package is not permitted by software restriction policy.
.
Language=Russian
The patch package is not permitted by software restriction policy.
.
Language=Polish
Zasady ograniczeń oprogramowania nie zezwalają na dany pakiet aktualizacji.
.
Language=Romanian
The patch package is not permitted by software restriction policy.
.

MessageId=1644
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_TRANSFORM_REJECTED
Language=English
One or more customizations are not permitted by software restriction policy.
.
Language=Russian
One or more customizations are not permitted by software restriction policy.
.
Language=Polish
Zasady ograniczeń oprogramowania nie zezwalają na jedno lub kilka z dostosowań.
.
Language=Romanian
One or more customizations are not permitted by software restriction policy.
.

MessageId=1645
Severity=Success
Facility=System
SymbolicName=ERROR_INSTALL_REMOTE_PROHIBITED
Language=English
The ReactOS Installer does not permit installation from a Remote Desktop Connection.
.
Language=Russian
The ReactOS Installer does not permit installation from a Remote Desktop Connection.
.
Language=Polish
Instalator ReactOS nie zezwala na instalację przy użyciu podłączania pulpitu zdalnego.
.
Language=Romanian
The ReactOS Installer does not permit installation from a Remote Desktop Connection.
.

MessageId=1700
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_BINDING
Language=English
The string binding is invalid.
.
Language=Russian
The string binding is invalid.
.
Language=Polish
Powiązanie ciągu jest nieprawidłowe.
.
Language=Romanian
The string binding is invalid.
.

MessageId=1701
Severity=Success
Facility=System
SymbolicName=RPC_S_WRONG_KIND_OF_BINDING
Language=English
The binding handle is not the correct type.
.
Language=Russian
The binding handle is not the correct type.
.
Language=Polish
Dojście powiązania nie jest poprawnego typu.
.
Language=Romanian
The binding handle is not the correct type.
.

MessageId=1702
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BINDING
Language=English
The binding handle is invalid.
.
Language=Russian
The binding handle is invalid.
.
Language=Polish
Dojście powiązania jest nieprawidłowe.
.
Language=Romanian
The binding handle is invalid.
.

MessageId=1703
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_SUPPORTED
Language=English
The RPC protocol sequence is not supported.
.
Language=Russian
The RPC protocol sequence is not supported.
.
Language=Polish
Sekwencja protokołu RPC nie jest obsługiwana.
.
Language=Romanian
The RPC protocol sequence is not supported.
.

MessageId=1704
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_RPC_PROTSEQ
Language=English
The RPC protocol sequence is invalid.
.
Language=Russian
The RPC protocol sequence is invalid.
.
Language=Polish
Sekwencja protokołu RPC jest nieprawidłowa.
.
Language=Romanian
The RPC protocol sequence is invalid.
.

MessageId=1705
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_STRING_UUID
Language=English
The string universal unique identifier (UUID) is invalid.
.
Language=Russian
The string universal unique identifier (UUID) is invalid.
.
Language=Polish
Uniwersalny, unikatowy identyfikator ciągu (UUID) jest nieprawidłowy.
.
Language=Romanian
The string universal unique identifier (UUID) is invalid.
.

MessageId=1706
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ENDPOINT_FORMAT
Language=English
The endpoint format is invalid.
.
Language=Russian
The endpoint format is invalid.
.
Language=Polish
Format punktu końcowego jest nieprawidłowy.
.
Language=Romanian
The endpoint format is invalid.
.

MessageId=1707
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NET_ADDR
Language=English
The network address is invalid.
.
Language=Russian
The network address is invalid.
.
Language=Polish
Adres sieciowy jest nieprawidłowy.
.
Language=Romanian
The network address is invalid.
.

MessageId=1708
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENDPOINT_FOUND
Language=English
No endpoint was found.
.
Language=Russian
No endpoint was found.
.
Language=Polish
Nie odnaleziono punktu końcowego.
.
Language=Romanian
No endpoint was found.
.

MessageId=1709
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TIMEOUT
Language=English
The timeout value is invalid.
.
Language=Russian
The timeout value is invalid.
.
Language=Polish
Wartość limitu czasu jest nieprawidłowa.
.
Language=Romanian
The timeout value is invalid.
.

MessageId=1710
Severity=Success
Facility=System
SymbolicName=RPC_S_OBJECT_NOT_FOUND
Language=English
The object universal unique identifier (UUID) was not found.
.
Language=Russian
The object universal unique identifier (UUID) was not found.
.
Language=Polish
Uniwersalny, unikatowy identyfikator obiektu (UUID) nie został znaleziony.
.
Language=Romanian
The object universal unique identifier (UUID) was not found.
.

MessageId=1711
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_REGISTERED
Language=English
The object universal unique identifier (UUID) has already been registered.
.
Language=Russian
The object universal unique identifier (UUID) has already been registered.
.
Language=Polish
Uniwersalny, unikatowy identyfikator obiektu (UUID) został już zarejestrowany.
.
Language=Romanian
The object universal unique identifier (UUID) has already been registered.
.

MessageId=1712
Severity=Success
Facility=System
SymbolicName=RPC_S_TYPE_ALREADY_REGISTERED
Language=English
The type universal unique identifier (UUID) has already been registered.
.
Language=Russian
The type universal unique identifier (UUID) has already been registered.
.
Language=Polish
Uniwersalny, unikatowy identyfikator typu (UUID) został już zarejestrowany.
.
Language=Romanian
The type universal unique identifier (UUID) has already been registered.
.

MessageId=1713
Severity=Success
Facility=System
SymbolicName=RPC_S_ALREADY_LISTENING
Language=English
The RPC server is already listening.
.
Language=Russian
The RPC server is already listening.
.
Language=Polish
Serwer RPC już nasłuchuje.
.
Language=Romanian
The RPC server is already listening.
.

MessageId=1714
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS_REGISTERED
Language=English
No protocol sequences have been registered.
.
Language=Russian
No protocol sequences have been registered.
.
Language=Polish
Żadna sekwencja protokołu nie została zarejestrowana.
.
Language=Romanian
No protocol sequences have been registered.
.

MessageId=1715
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_LISTENING
Language=English
The RPC server is not listening.
.
Language=Russian
The RPC server is not listening.
.
Language=Polish
Serwer RPC nie nasłuchuje.
.
Language=Romanian
The RPC server is not listening.
.

MessageId=1716
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_MGR_TYPE
Language=English
The manager type is unknown.
.
Language=Russian
The manager type is unknown.
.
Language=Polish
Typ menedżera jest nieznany.
.
Language=Romanian
The manager type is unknown.
.

MessageId=1717
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_IF
Language=English
The interface is unknown.
.
Language=Russian
The interface is unknown.
.
Language=Polish
Interfejs jest nieznany.
.
Language=Romanian
The interface is unknown.
.

MessageId=1718
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_BINDINGS
Language=English
There are no bindings.
.
Language=Russian
There are no bindings.
.
Language=Polish
Nie ma powiązań.
.
Language=Romanian
There are no bindings.
.

MessageId=1719
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PROTSEQS
Language=English
There are no protocol sequences.
.
Language=Russian
There are no protocol sequences.
.
Language=Polish
Nie ma sekwencji protokołów.
.
Language=Romanian
There are no protocol sequences.
.

MessageId=1720
Severity=Success
Facility=System
SymbolicName=RPC_S_CANT_CREATE_ENDPOINT
Language=English
The endpoint cannot be created.
.
Language=Russian
The endpoint cannot be created.
.
Language=Polish
Nie można utworzyć punktu końcowego.
.
Language=Romanian
The endpoint cannot be created.
.

MessageId=1721
Severity=Success
Facility=System
SymbolicName=RPC_S_OUT_OF_RESOURCES
Language=English
Not enough resources are available to complete this operation.
.
Language=Russian
Not enough resources are available to complete this operation.
.
Language=Polish
Za mało dostępnych zasobów do ukończenia tej operacji.
.
Language=Romanian
Not enough resources are available to complete this operation.
.

MessageId=1722
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_UNAVAILABLE
Language=English
The RPC server is unavailable.
.
Language=Russian
The RPC server is unavailable.
.
Language=Polish
Serwer RPC jest niedostępny.
.
Language=Romanian
The RPC server is unavailable.
.

MessageId=1723
Severity=Success
Facility=System
SymbolicName=RPC_S_SERVER_TOO_BUSY
Language=English
The RPC server is too busy to complete this operation.
.
Language=Russian
The RPC server is too busy to complete this operation.
.
Language=Polish
Serwer RPC jest zbyt zajęty, aby ukończyć tę operację.
.
Language=Romanian
The RPC server is too busy to complete this operation.
.

MessageId=1724
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NETWORK_OPTIONS
Language=English
The network options are invalid.
.
Language=Russian
The network options are invalid.
.
Language=Polish
Opcje sieciowe są nieprawidłowe.
.
Language=Romanian
The network options are invalid.
.

MessageId=1725
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CALL_ACTIVE
Language=English
There are no remote procedure calls active on this thread.
.
Language=Russian
There are no remote procedure calls active on this thread.
.
Language=Polish
W tym wątku nie ma aktywnego żadnego zdalnego wywołania procedury.
.
Language=Romanian
There are no remote procedure calls active on this thread.
.

MessageId=1726
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED
Language=English
The remote procedure call failed.
.
Language=Russian
The remote procedure call failed.
.
Language=Polish
Zdalne wywołanie procedury nie powiodło się.
.
Language=Romanian
The remote procedure call failed.
.

MessageId=1727
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_FAILED_DNE
Language=English
The remote procedure call failed and did not execute.
.
Language=Russian
The remote procedure call failed and did not execute.
.
Language=Polish
Zdalne wywołanie procedury nie powiodło się i nie zostało wykonane.
.
Language=Romanian
The remote procedure call failed and did not execute.
.

MessageId=1728
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTOCOL_ERROR
Language=English
A remote procedure call (RPC) protocol error occurred.
.
Language=Russian
A remote procedure call (RPC) protocol error occurred.
.
Language=Polish
Wystąpił błąd protokołu zdalnego wywołania procedury (RPC).
.
Language=Romanian
A remote procedure call (RPC) protocol error occurred.
.

MessageId=1730
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TRANS_SYN
Language=English
The transfer syntax is not supported by the RPC server.
.
Language=Russian
The transfer syntax is not supported by the RPC server.
.
Language=Polish
Składnia transferu nie jest obsługiwana przez serwer RPC.
.
Language=Romanian
The transfer syntax is not supported by the RPC server.
.

MessageId=1732
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_TYPE
Language=English
The universal unique identifier (UUID) type is not supported.
.
Language=Russian
The universal unique identifier (UUID) type is not supported.
.
Language=Polish
Typ uniwersalnego, unikatowego identyfikatora (UUID) nie jest obsługiwany.
.
Language=Romanian
The universal unique identifier (UUID) type is not supported.
.

MessageId=1733
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_TAG
Language=English
The tag is invalid.
.
Language=Russian
The tag is invalid.
.
Language=Polish
Tag jest nieprawidłowy.
.
Language=Romanian
The tag is invalid.
.

MessageId=1734
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_BOUND
Language=English
The array bounds are invalid.
.
Language=Russian
The array bounds are invalid.
.
Language=Polish
Granice tablicy są nieprawidłowe.
.
Language=Romanian
The array bounds are invalid.
.

MessageId=1735
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_ENTRY_NAME
Language=English
The binding does not contain an entry name.
.
Language=Russian
The binding does not contain an entry name.
.
Language=Polish
Powiązanie nie zawiera nazwy wpisu.
.
Language=Romanian
The binding does not contain an entry name.
.

MessageId=1736
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAME_SYNTAX
Language=English
The name syntax is invalid.
.
Language=Russian
The name syntax is invalid.
.
Language=Polish
Składnia nazwy jest nieprawidłowa.
.
Language=Romanian
The name syntax is invalid.
.

MessageId=1737
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_NAME_SYNTAX
Language=English
The name syntax is not supported.
.
Language=Russian
The name syntax is not supported.
.
Language=Polish
Składnia nazwy nie jest obsługiwana.
.
Language=Romanian
The name syntax is not supported.
.

MessageId=1739
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_NO_ADDRESS
Language=English
No network address is available to use to construct a universal unique identifier (UUID).
.
Language=Russian
No network address is available to use to construct a universal unique identifier (UUID).
.
Language=Polish
Brak dostępnych adresów sieciowych do utworzenia uniwersalnego unikatowego identyfikatora (UUID).
.
Language=Romanian
No network address is available to use to construct a universal unique identifier (UUID).
.

MessageId=1740
Severity=Success
Facility=System
SymbolicName=RPC_S_DUPLICATE_ENDPOINT
Language=English
The endpoint is a duplicate.
.
Language=Russian
The endpoint is a duplicate.
.
Language=Polish
Punkt końcowy jest duplikatem.
.
Language=Romanian
The endpoint is a duplicate.
.

MessageId=1741
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_TYPE
Language=English
The authentication type is unknown.
.
Language=Russian
The authentication type is unknown.
.
Language=Polish
Typ uwierzytelniania jest nieznany.
.
Language=Romanian
The authentication type is unknown.
.

MessageId=1742
Severity=Success
Facility=System
SymbolicName=RPC_S_MAX_CALLS_TOO_SMALL
Language=English
The maximum number of calls is too small.
.
Language=Russian
The maximum number of calls is too small.
.
Language=Polish
Maksymalna liczba wywołań jest za mała.
.
Language=Romanian
The maximum number of calls is too small.
.

MessageId=1743
Severity=Success
Facility=System
SymbolicName=RPC_S_STRING_TOO_LONG
Language=English
The string is too long.
.
Language=Russian
The string is too long.
.
Language=Polish
Ciąg jest za długi.
.
Language=Romanian
The string is too long.
.

MessageId=1744
Severity=Success
Facility=System
SymbolicName=RPC_S_PROTSEQ_NOT_FOUND
Language=English
The RPC protocol sequence was not found.
.
Language=Russian
The RPC protocol sequence was not found.
.
Language=Polish
Nie odnaleziono sekwencji protokołu RPC.
.
Language=Romanian
The RPC protocol sequence was not found.
.

MessageId=1745
Severity=Success
Facility=System
SymbolicName=RPC_S_PROCNUM_OUT_OF_RANGE
Language=English
The procedure number is out of range.
.
Language=Russian
The procedure number is out of range.
.
Language=Polish
Numer procedury jest spoza zakresu.
.
Language=Romanian
The procedure number is out of range.
.

MessageId=1746
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_HAS_NO_AUTH
Language=English
The binding does not contain any authentication information.
.
Language=Russian
The binding does not contain any authentication information.
.
Language=Polish
Powiązanie nie zawiera żadnych informacji o uwierzytelnianiu.
.
Language=Romanian
The binding does not contain any authentication information.
.

MessageId=1747
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_SERVICE
Language=English
The authentication service is unknown.
.
Language=Russian
The authentication service is unknown.
.
Language=Polish
Usługa uwierzytelniania jest nieznana.
.
Language=Romanian
The authentication service is unknown.
.

MessageId=1748
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHN_LEVEL
Language=English
The authentication level is unknown.
.
Language=Russian
The authentication level is unknown.
.
Language=Polish
Poziom uwierzytelniania jest nieznany.
.
Language=Romanian
The authentication level is unknown.
.

MessageId=1749
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_AUTH_IDENTITY
Language=English
The security context is invalid.
.
Language=Russian
The security context is invalid.
.
Language=Polish
Kontekst zabezpieczeń jest nieprawidłowy.
.
Language=Romanian
The security context is invalid.
.

MessageId=1750
Severity=Success
Facility=System
SymbolicName=RPC_S_UNKNOWN_AUTHZ_SERVICE
Language=English
The authorization service is unknown.
.
Language=Russian
The authorization service is unknown.
.
Language=Polish
Usługa autoryzowania jest nieznana.
.
Language=Romanian
The authorization service is unknown.
.

MessageId=1751
Severity=Success
Facility=System
SymbolicName=EPT_S_INVALID_ENTRY
Language=English
The entry is invalid.
.
Language=Russian
The entry is invalid.
.
Language=Polish
Wpis jest nieprawidłowy.
.
Language=Romanian
The entry is invalid.
.

MessageId=1752
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_PERFORM_OP
Language=English
The server endpoint cannot perform the operation.
.
Language=Russian
The server endpoint cannot perform the operation.
.
Language=Polish
Punkt końcowy serwera nie może wykonać operacji.
.
Language=Romanian
The server endpoint cannot perform the operation.
.

MessageId=1753
Severity=Success
Facility=System
SymbolicName=EPT_S_NOT_REGISTERED
Language=English
There are no more endpoints available from the endpoint mapper.
.
Language=Russian
There are no more endpoints available from the endpoint mapper.
.
Language=Polish
Nie ma więcej dostępnych punktów końcowych z programu mapowania punktów końcowych.
.
Language=Romanian
There are no more endpoints available from the endpoint mapper.
.

MessageId=1754
Severity=Success
Facility=System
SymbolicName=RPC_S_NOTHING_TO_EXPORT
Language=English
No interfaces have been exported.
.
Language=Russian
No interfaces have been exported.
.
Language=Polish
Żaden interfejs nie został wyeksportowany.
.
Language=Romanian
No interfaces have been exported.
.

MessageId=1755
Severity=Success
Facility=System
SymbolicName=RPC_S_INCOMPLETE_NAME
Language=English
The entry name is incomplete.
.
Language=Russian
The entry name is incomplete.
.
Language=Polish
Nazwa wpisu jest niekompletna.
.
Language=Romanian
The entry name is incomplete.
.

MessageId=1756
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_VERS_OPTION
Language=English
The version option is invalid.
.
Language=Russian
The version option is invalid.
.
Language=Polish
Opcja wersji jest nieprawidłowa.
.
Language=Romanian
The version option is invalid.
.

MessageId=1757
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_MEMBERS
Language=English
There are no more members.
.
Language=Russian
There are no more members.
.
Language=Polish
Nie ma więcej członków grupy.
.
Language=Romanian
There are no more members.
.

MessageId=1758
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_ALL_OBJS_UNEXPORTED
Language=English
There is nothing to unexport.
.
Language=Russian
There is nothing to unexport.
.
Language=Polish
Nie ma nic, na czym można by wykonać cofnięcie eksportowania.
.
Language=Romanian
There is nothing to unexport.
.

MessageId=1759
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERFACE_NOT_FOUND
Language=English
The interface was not found.
.
Language=Russian
The interface was not found.
.
Language=Polish
Nie odnaleziono interfejsu.
.
Language=Romanian
The interface was not found.
.

MessageId=1760
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_ALREADY_EXISTS
Language=English
The entry already exists.
.
Language=Russian
The entry already exists.
.
Language=Polish
Wpis już istnieje.
.
Language=Romanian
The entry already exists.
.

MessageId=1761
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_NOT_FOUND
Language=English
The entry is not found.
.
Language=Russian
The entry is not found.
.
Language=Polish
Nie można odnaleźć wpisu.
.
Language=Romanian
The entry is not found.
.

MessageId=1762
Severity=Success
Facility=System
SymbolicName=RPC_S_NAME_SERVICE_UNAVAILABLE
Language=English
The name service is unavailable.
.
Language=Russian
The name service is unavailable.
.
Language=Polish
Usługa nazw jest niedostępna.
.
Language=Romanian
The name service is unavailable.
.

MessageId=1763
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_NAF_ID
Language=English
The network address family is invalid.
.
Language=Russian
The network address family is invalid.
.
Language=Polish
Rodzina adresów sieciowych jest nieprawidłowa.
.
Language=Romanian
The network address family is invalid.
.

MessageId=1764
Severity=Success
Facility=System
SymbolicName=RPC_S_CANNOT_SUPPORT
Language=English
The requested operation is not supported.
.
Language=Russian
The requested operation is not supported.
.
Language=Polish
Żądana operacja nie jest obsługiwana.
.
Language=Romanian
The requested operation is not supported.
.

MessageId=1765
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_CONTEXT_AVAILABLE
Language=English
No security context is available to allow impersonation.
.
Language=Russian
No security context is available to allow impersonation.
.
Language=Polish
Nie jest dostępny kontekst zabezpieczeń umożliwiający personifikację.
.
Language=Romanian
No security context is available to allow impersonation.
.

MessageId=1766
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERNAL_ERROR
Language=English
An internal error occurred in a remote procedure call (RPC).
.
Language=Russian
An internal error occurred in a remote procedure call (RPC).
.
Language=Polish
Wystąpił błąd wewnętrzny w zdalnym wywołaniu procedury (RPC).
.
Language=Romanian
An internal error occurred in a remote procedure call (RPC).
.

MessageId=1767
Severity=Success
Facility=System
SymbolicName=RPC_S_ZERO_DIVIDE
Language=English
The RPC server attempted an integer division by zero.
.
Language=Russian
The RPC server attempted an integer division by zero.
.
Language=Polish
Serwer RPC próbował wykonać dzielenie liczby całkowitej przez zero.
.
Language=Romanian
The RPC server attempted an integer division by zero.
.

MessageId=1768
Severity=Success
Facility=System
SymbolicName=RPC_S_ADDRESS_ERROR
Language=English
An addressing error occurred in the RPC server.
.
Language=Russian
An addressing error occurred in the RPC server.
.
Language=Polish
Wystąpił błąd adresowania na serwerze RPC.
.
Language=Romanian
An addressing error occurred in the RPC server.
.

MessageId=1769
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_DIV_ZERO
Language=English
A floating-point operation at the RPC server caused a division by zero.
.
Language=Russian
A floating-point operation at the RPC server caused a division by zero.
.
Language=Polish
Operacja zmiennoprzecinkowa serwera RPC spowodowała dzielenie przez zero.
.
Language=Romanian
A floating-point operation at the RPC server caused a division by zero.
.

MessageId=1770
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_UNDERFLOW
Language=English
A floating-point underflow occurred at the RPC server.
.
Language=Russian
A floating-point underflow occurred at the RPC server.
.
Language=Polish
Na serwerze RPC wystąpił niedomiar zmiennoprzecinkowy.
.
Language=Romanian
A floating-point underflow occurred at the RPC server.
.

MessageId=1771
Severity=Success
Facility=System
SymbolicName=RPC_S_FP_OVERFLOW
Language=English
A floating-point overflow occurred at the RPC server.
.
Language=Russian
A floating-point overflow occurred at the RPC server.
.
Language=Polish
Na serwerze RPC wystąpił nadmiar zmiennoprzecinkowy.
.
Language=Romanian
A floating-point overflow occurred at the RPC server.
.

MessageId=1772
Severity=Success
Facility=System
SymbolicName=RPC_X_NO_MORE_ENTRIES
Language=English
The list of RPC servers available for the binding of auto handles has been exhausted.
.
Language=Russian
The list of RPC servers available for the binding of auto handles has been exhausted.
.
Language=Polish
Lista serwerów RPC dostępnych do powiązania autodojść została wyczerpana.
.
Language=Romanian
The list of RPC servers available for the binding of auto handles has been exhausted.
.

MessageId=1773
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_OPEN_FAIL
Language=English
Unable to open the character translation table file.
.
Language=Russian
Unable to open the character translation table file.
.
Language=Polish
Nie można otworzyć pliku tabeli translacji znaków.
.
Language=Romanian
Unable to open the character translation table file.
.

MessageId=1774
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CHAR_TRANS_SHORT_FILE
Language=English
The file containing the character translation table has fewer than 512 bytes.
.
Language=Russian
The file containing the character translation table has fewer than 512 bytes.
.
Language=Polish
Plik zawierający tabelę translacji znaków ma mniej niż 512 bajtów.
.
Language=Romanian
The file containing the character translation table has fewer than 512 bytes.
.

MessageId=1775
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_IN_NULL_CONTEXT
Language=English
A null context handle was passed from the client to the host during a remote procedure call.
.
Language=Russian
A null context handle was passed from the client to the host during a remote procedure call.
.
Language=Polish
Dojście z zerowym kontekstem (null context handle) zostało przekazane od klienta do hosta w czasie zdalnego wywołania procedury.
.
Language=Romanian
A null context handle was passed from the client to the host during a remote procedure call.
.

MessageId=1777
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CONTEXT_DAMAGED
Language=English
The context handle changed during a remote procedure call.
.
Language=Russian
The context handle changed during a remote procedure call.
.
Language=Polish
Dojście kontekstu zmieniło się podczas zdalnego wywołania procedury.
.
Language=Romanian
The context handle changed during a remote procedure call.
.

MessageId=1778
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_HANDLES_MISMATCH
Language=English
The binding handles passed to a remote procedure call do not match.
.
Language=Russian
The binding handles passed to a remote procedure call do not match.
.
Language=Polish
Dojścia powiązania przekazane do zdalnego wywołania procedury nie pasują do siebie.
.
Language=Romanian
The binding handles passed to a remote procedure call do not match.
.

MessageId=1779
Severity=Success
Facility=System
SymbolicName=RPC_X_SS_CANNOT_GET_CALL_HANDLE
Language=English
The stub is unable to get the remote procedure call handle.
.
Language=Russian
The stub is unable to get the remote procedure call handle.
.
Language=Polish
Procedura wejścia nie może uzyskać dojścia do zdalnego wywołania procedury.
.
Language=Romanian
The stub is unable to get the remote procedure call handle.
.

MessageId=1780
Severity=Success
Facility=System
SymbolicName=RPC_X_NULL_REF_POINTER
Language=English
A null reference pointer was passed to the stub.
.
Language=Russian
A null reference pointer was passed to the stub.
.
Language=Polish
Do procedury wejścia został przekazany wskaźnik odwołania zerowego.
.
Language=Romanian
A null reference pointer was passed to the stub.
.

MessageId=1781
Severity=Success
Facility=System
SymbolicName=RPC_X_ENUM_VALUE_OUT_OF_RANGE
Language=English
The enumeration value is out of range.
.
Language=Russian
The enumeration value is out of range.
.
Language=Polish
Wartość wyliczenia jest spoza zakresu.
.
Language=Romanian
The enumeration value is out of range.
.

MessageId=1782
Severity=Success
Facility=System
SymbolicName=RPC_X_BYTE_COUNT_TOO_SMALL
Language=English
The byte count is too small.
.
Language=Russian
The byte count is too small.
.
Language=Polish
Liczba bajtów jest za mała.
.
Language=Romanian
The byte count is too small.
.

MessageId=1783
Severity=Success
Facility=System
SymbolicName=RPC_X_BAD_STUB_DATA
Language=English
The stub received bad data.
.
Language=Russian
The stub received bad data.
.
Language=Polish
Procedura wejścia odebrała złe dane.
.
Language=Romanian
The stub received bad data.
.

MessageId=1784
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_USER_BUFFER
Language=English
The supplied user buffer is not valid for the requested operation.
.
Language=Russian
The supplied user buffer is not valid for the requested operation.
.
Language=Polish
Podany bufor użytkownika jest nieodpowiedni dla żądanej operacji.
.
Language=Romanian
The supplied user buffer is not valid for the requested operation.
.

MessageId=1785
Severity=Success
Facility=System
SymbolicName=ERROR_UNRECOGNIZED_MEDIA
Language=English
The disk media is not recognized. It may not be formatted.
.
Language=Russian
The disk media is not recognized. It may not be formatted.
.
Language=Polish
Nie rozpoznany nośnik dysku. Może być nie sformatowany.
.
Language=Romanian
The disk media is not recognized. It may not be formatted.
.

MessageId=1786
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_LSA_SECRET
Language=English
The workstation does not have a trust secret.
.
Language=Russian
The workstation does not have a trust secret.
.
Language=Polish
Stacja robocza nie ma hasła zaufania.
.
Language=Romanian
The workstation does not have a trust secret.
.

MessageId=1787
Severity=Success
Facility=System
SymbolicName=ERROR_NO_TRUST_SAM_ACCOUNT
Language=English
The security database on the server does not have a computer account for this workstation trust relationship.
.
Language=Russian
The security database on the server does not have a computer account for this workstation trust relationship.
.
Language=Polish
Baza danych zabezpieczeń na serwerze nie ma konta komputera dla relacji zaufania tej stacji roboczej.
.
Language=Romanian
The security database on the server does not have a computer account for this workstation trust relationship.
.

MessageId=1788
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_DOMAIN_FAILURE
Language=English
The trust relationship between the primary domain and the trusted domain failed.
.
Language=Russian
The trust relationship between the primary domain and the trusted domain failed.
.
Language=Polish
Relacje zaufania między domeną podstawową a domeną zaufaną nie powiodły się.
.
Language=Romanian
The trust relationship between the primary domain and the trusted domain failed.
.

MessageId=1789
Severity=Success
Facility=System
SymbolicName=ERROR_TRUSTED_RELATIONSHIP_FAILURE
Language=English
The trust relationship between this workstation and the primary domain failed.
.
Language=Russian
The trust relationship between this workstation and the primary domain failed.
.
Language=Polish
Relacje zaufania między tą stacją roboczą a domeną podstawową nie powiodły się.
.
Language=Romanian
The trust relationship between this workstation and the primary domain failed.
.

MessageId=1790
Severity=Success
Facility=System
SymbolicName=ERROR_TRUST_FAILURE
Language=English
The network logon failed.
.
Language=Russian
The network logon failed.
.
Language=Polish
Logowanie w sieci nie powiodło się.
.
Language=Romanian
The network logon failed.
.

MessageId=1791
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_IN_PROGRESS
Language=English
A remote procedure call is already in progress for this thread.
.
Language=Russian
A remote procedure call is already in progress for this thread.
.
Language=Polish
Zdalne wywołanie procedury jest już w toku dla tego wątku.
.
Language=Romanian
A remote procedure call is already in progress for this thread.
.

MessageId=1792
Severity=Success
Facility=System
SymbolicName=ERROR_NETLOGON_NOT_STARTED
Language=English
An attempt was made to logon, but the network logon service was not started.
.
Language=Russian
An attempt was made to logon, but the network logon service was not started.
.
Language=Polish
Podjęto próbę zalogowania, ale sieciowa usługa logowania nie została uruchomiona.
.
Language=Romanian
An attempt was made to logon, but the network logon service was not started.
.

MessageId=1793
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_EXPIRED
Language=English
The user's account has expired.
.
Language=Russian
The user's account has expired.
.
Language=Polish
Konto użytkownika wygasło.
.
Language=Romanian
The user's account has expired.
.

MessageId=1794
Severity=Success
Facility=System
SymbolicName=ERROR_REDIRECTOR_HAS_OPEN_HANDLES
Language=English
The redirector is in use and cannot be unloaded.
.
Language=Russian
The redirector is in use and cannot be unloaded.
.
Language=Polish
Readresator jest w użyciu i nie można usunąć go z pamięci.
.
Language=Romanian
The redirector is in use and cannot be unloaded.
.

MessageId=1795
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_ALREADY_INSTALLED
Language=English
The specified printer driver is already installed.
.
Language=Russian
The specified printer driver is already installed.
.
Language=Polish
Określony sterownik drukarki jest już zainstalowany.
.
Language=Romanian
The specified printer driver is already installed.
.

MessageId=1796
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PORT
Language=English
The specified port is unknown.
.
Language=Russian
The specified port is unknown.
.
Language=Polish
Określony port jest nieznany.
.
Language=Romanian
The specified port is unknown.
.

MessageId=1797
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTER_DRIVER
Language=English
The printer driver is unknown.
.
Language=Russian
The printer driver is unknown.
.
Language=Polish
Sterownik drukarki jest nieznany.
.
Language=Romanian
The printer driver is unknown.
.

MessageId=1798
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINTPROCESSOR
Language=English
The print processor is unknown.
.
Language=Russian
The print processor is unknown.
.
Language=Polish
Procesor wydruku jest nieznany.
.
Language=Romanian
The print processor is unknown.
.

MessageId=1799
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_SEPARATOR_FILE
Language=English
The specified separator file is invalid.
.
Language=Russian
The specified separator file is invalid.
.
Language=Polish
Określony plik separatora jest nieprawidłowy.
.
Language=Romanian
The specified separator file is invalid.
.

MessageId=1800
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRIORITY
Language=English
The specified priority is invalid.
.
Language=Russian
The specified priority is invalid.
.
Language=Polish
Określony priorytet jest nieprawidłowy.
.
Language=Romanian
The specified priority is invalid.
.

MessageId=1801
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_NAME
Language=English
The printer name is invalid.
.
Language=Russian
The printer name is invalid.
.
Language=Polish
Nazwa drukarki jest nieprawidłowa.
.
Language=Romanian
The printer name is invalid.
.

MessageId=1802
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_ALREADY_EXISTS
Language=English
The printer already exists.
.
Language=Russian
The printer already exists.
.
Language=Polish
Drukarka już istnieje.
.
Language=Romanian
The printer already exists.
.

MessageId=1803
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_COMMAND
Language=English
The printer command is invalid.
.
Language=Russian
The printer command is invalid.
.
Language=Polish
Polecenie drukarki jest nieprawidłowe.
.
Language=Romanian
The printer command is invalid.
.

MessageId=1804
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DATATYPE
Language=English
The specified datatype is invalid.
.
Language=Russian
The specified datatype is invalid.
.
Language=Polish
Określony typ danych jest nieprawidłowy.
.
Language=Romanian
The specified datatype is invalid.
.

MessageId=1805
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_ENVIRONMENT
Language=English
The environment specified is invalid.
.
Language=Russian
The environment specified is invalid.
.
Language=Polish
Określone środowisko jest nieprawidłowe.
.
Language=Romanian
The environment specified is invalid.
.

MessageId=1806
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_MORE_BINDINGS
Language=English
There are no more bindings.
.
Language=Russian
There are no more bindings.
.
Language=Polish
Nie ma więcej powiązań.
.
Language=Romanian
There are no more bindings.
.

MessageId=1807
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
Language=English
The account used is an interdomain trust account. Use your global user account or local user account to access this server.
.
Language=Russian
The account used is an interdomain trust account. Use your global user account or local user account to access this server.
.
Language=Polish
Użyte konto jest międzydomenowym kontem zaufania. Aby uzyskać dostęp do tego serwera, użyj globalnego lub lokalnego konta użytkownika.
.
Language=Romanian
The account used is an interdomain trust account. Use your global user account or local user account to access this server.
.

MessageId=1808
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT
Language=English
The account used is a computer account. Use your global user account or local user account to access this server.
.
Language=Russian
The account used is a computer account. Use your global user account or local user account to access this server.
.
Language=Polish
Użyte konto jest kontem komputera. Aby uzyskać dostęp do tego serwera, użyj globalnego lub lokalnego konta użytkownika.
.
Language=Romanian
The account used is a computer account. Use your global user account or local user account to access this server.
.

MessageId=1809
Severity=Success
Facility=System
SymbolicName=ERROR_NOLOGON_SERVER_TRUST_ACCOUNT
Language=English
The account used is a server trust account. Use your global user account or local user account to access this server.
.
Language=Russian
The account used is a server trust account. Use your global user account or local user account to access this server.
.
Language=Polish
Użyte konto jest kontem zaufania serwera. Aby uzyskać dostęp do tego serwera, użyj globalnego lub lokalnego konta użytkownika.
.
Language=Romanian
The account used is a server trust account. Use your global user account or local user account to access this server.
.

MessageId=1810
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_TRUST_INCONSISTENT
Language=English
The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.
.
Language=Russian
The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.
.
Language=Polish
Nazwa lub identyfikator zabezpieczeń (SID) określonej domeny jest niezgodny z informacją zaufania dla tej domeny.
.
Language=Romanian
The name or security ID (SID) of the domain specified is inconsistent with the trust information for that domain.
.

MessageId=1811
Severity=Success
Facility=System
SymbolicName=ERROR_SERVER_HAS_OPEN_HANDLES
Language=English
The server is in use and cannot be unloaded.
.
Language=Russian
The server is in use and cannot be unloaded.
.
Language=Polish
Serwer jest w użyciu i nie można usunąć go z pamięci.
.
Language=Romanian
The server is in use and cannot be unloaded.
.

MessageId=1812
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_DATA_NOT_FOUND
Language=English
The specified image file did not contain a resource section.
.
Language=Russian
The specified image file did not contain a resource section.
.
Language=Polish
Określony plik obrazu nie zawierał sekcji zasobów.
.
Language=Romanian
The specified image file did not contain a resource section.
.

MessageId=1813
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_TYPE_NOT_FOUND
Language=English
The specified resource type cannot be found in the image file.
.
Language=Russian
The specified resource type cannot be found in the image file.
.
Language=Polish
Nie można znaleźć określonego typu zasobu w pliku obrazu.
.
Language=Romanian
The specified resource type cannot be found in the image file.
.

MessageId=1814
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NAME_NOT_FOUND
Language=English
The specified resource name cannot be found in the image file.
.
Language=Russian
The specified resource name cannot be found in the image file.
.
Language=Polish
Nie można znaleźć określonej nazwy zasobu w pliku obrazu.
.
Language=Romanian
The specified resource name cannot be found in the image file.
.

MessageId=1815
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_LANG_NOT_FOUND
Language=English
The specified resource language ID cannot be found in the image file.
.
Language=Russian
The specified resource language ID cannot be found in the image file.
.
Language=Polish
Nie można odnaleźć identyfikatora języka zasobu w pliku obrazu.
.
Language=Romanian
The specified resource language ID cannot be found in the image file.
.

MessageId=1816
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_ENOUGH_QUOTA
Language=English
Not enough quota is available to process this command.
.
Language=Russian
Not enough quota is available to process this command.
.
Language=Polish
Za mały przydział do przetworzenia tego polecenia.
.
Language=Romanian
Not enough quota is available to process this command.
.

MessageId=1817
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_INTERFACES
Language=English
No interfaces have been registered.
.
Language=Russian
No interfaces have been registered.
.
Language=Polish
Żaden interfejs nie został zarejestrowany.
.
Language=Romanian
No interfaces have been registered.
.

MessageId=1818
Severity=Success
Facility=System
SymbolicName=RPC_S_CALL_CANCELLED
Language=English
The remote procedure call was cancelled.
.
Language=Russian
The remote procedure call was cancelled.
.
Language=Polish
Zdalne wywołanie procedury zostało anulowane.
.
Language=Romanian
The remote procedure call was cancelled.
.

MessageId=1819
Severity=Success
Facility=System
SymbolicName=RPC_S_BINDING_INCOMPLETE
Language=English
The binding handle does not contain all required information.
.
Language=Russian
The binding handle does not contain all required information.
.
Language=Polish
Dojście powiązania nie zawiera wszystkich wymaganych informacji.
.
Language=Romanian
The binding handle does not contain all required information.
.

MessageId=1820
Severity=Success
Facility=System
SymbolicName=RPC_S_COMM_FAILURE
Language=English
A communications failure occurred during a remote procedure call.
.
Language=Russian
A communications failure occurred during a remote procedure call.
.
Language=Polish
Podczas zdalnego wywoływania procedury wystąpił błąd komunikacji.
.
Language=Romanian
A communications failure occurred during a remote procedure call.
.

MessageId=1821
Severity=Success
Facility=System
SymbolicName=RPC_S_UNSUPPORTED_AUTHN_LEVEL
Language=English
The requested authentication level is not supported.
.
Language=Russian
The requested authentication level is not supported.
.
Language=Polish
Żądany poziom uwierzytelniania nie jest obsługiwany.
.
Language=Romanian
The requested authentication level is not supported.
.

MessageId=1822
Severity=Success
Facility=System
SymbolicName=RPC_S_NO_PRINC_NAME
Language=English
No principal name registered.
.
Language=Russian
No principal name registered.
.
Language=Polish
Nie zarejestrowano nazwy głównej.
.
Language=Romanian
No principal name registered.
.

MessageId=1823
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_RPC_ERROR
Language=English
The error specified is not a valid Windows RPC error code.
.
Language=Russian
The error specified is not a valid Windows RPC error code.
.
Language=Polish
Określony błąd nie jest prawidłowym kodem błędu protokołu zdalnego wywołania procedury (RPC).
.
Language=Romanian
The error specified is not a valid Windows RPC error code.
.

MessageId=1824
Severity=Success
Facility=System
SymbolicName=RPC_S_UUID_LOCAL_ONLY
Language=English
A UUID that is valid only on this computer has been allocated.
.
Language=Russian
A UUID that is valid only on this computer has been allocated.
.
Language=Polish
Przydzielono identyfikator UUID, który jest prawidłowy tylko na tym komputerze.
.
Language=Romanian
A UUID that is valid only on this computer has been allocated.
.

MessageId=1825
Severity=Success
Facility=System
SymbolicName=RPC_S_SEC_PKG_ERROR
Language=English
A security package specific error occurred.
.
Language=Russian
A security package specific error occurred.
.
Language=Polish
Wystąpił błąd specyficzny dla pakietu zabezpieczeń.
.
Language=Romanian
A security package specific error occurred.
.

MessageId=1826
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_CANCELLED
Language=English
Thread is not canceled.
.
Language=Russian
Thread is not canceled.
.
Language=Polish
Wątek nie został anulowany.
.
Language=Romanian
Thread is not canceled.
.

MessageId=1827
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_ES_ACTION
Language=English
Invalid operation on the encoding/decoding handle.
.
Language=Russian
Invalid operation on the encoding/decoding handle.
.
Language=Polish
Nieprawidłowa operacja na dojściu kodowania/dekodowania.
.
Language=Romanian
Invalid operation on the encoding/decoding handle.
.

MessageId=1828
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_ES_VERSION
Language=English
Incompatible version of the serializing package.
.
Language=Russian
Incompatible version of the serializing package.
.
Language=Polish
Niezgodna wersja pakietu szeregującego.
.
Language=Romanian
Incompatible version of the serializing package.
.

MessageId=1829
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_STUB_VERSION
Language=English
Incompatible version of the RPC stub.
.
Language=Russian
Incompatible version of the RPC stub.
.
Language=Polish
Niezgodna wersja procedury wejścia RPC.
.
Language=Romanian
Incompatible version of the RPC stub.
.

MessageId=1830
Severity=Success
Facility=System
SymbolicName=RPC_X_INVALID_PIPE_OBJECT
Language=English
The RPC pipe object is invalid or corrupted.
.
Language=Russian
The RPC pipe object is invalid or corrupted.
.
Language=Polish
Obiekt potoku RPC jest nieprawidłowy lub uszkodzony.
.
Language=Romanian
The RPC pipe object is invalid or corrupted.
.

MessageId=1831
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_ORDER
Language=English
An invalid operation was attempted on an RPC pipe object.
.
Language=Russian
An invalid operation was attempted on an RPC pipe object.
.
Language=Polish
Ta operacja nie jest prawidłowa dla danego obiektu potoku RPC.
.
Language=Romanian
An invalid operation was attempted on an RPC pipe object.
.

MessageId=1832
Severity=Success
Facility=System
SymbolicName=RPC_X_WRONG_PIPE_VERSION
Language=English
Unsupported RPC pipe version.
.
Language=Russian
Unsupported RPC pipe version.
.
Language=Polish
Ta wersja potoku RPC nie jest obsługiwana.
.
Language=Romanian
Unsupported RPC pipe version.
.

MessageId=1898
Severity=Success
Facility=System
SymbolicName=RPC_S_GROUP_MEMBER_NOT_FOUND
Language=English
The group member was not found.
.
Language=Russian
The group member was not found.
.
Language=Polish
Nie odnaleziono członka grupy.
.
Language=Romanian
The group member was not found.
.

MessageId=1899
Severity=Success
Facility=System
SymbolicName=EPT_S_CANT_CREATE
Language=English
The endpoint mapper database entry could not be created.
.
Language=Russian
The endpoint mapper database entry could not be created.
.
Language=Polish
Nie można utworzyć bazy danych mapowania punktu końcowego.
.
Language=Romanian
The endpoint mapper database entry could not be created.
.

MessageId=1900
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_OBJECT
Language=English
The object universal unique identifier (UUID) is the nil UUID.
.
Language=Russian
The object universal unique identifier (UUID) is the nil UUID.
.
Language=Polish
Uniwersalny, unikatowy identyfikator obiektu (UUID) jest zerowym identyfikatorem UUID.
.
Language=Romanian
The object universal unique identifier (UUID) is the nil UUID.
.

MessageId=1901
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TIME
Language=English
The specified time is invalid.
.
Language=Russian
The specified time is invalid.
.
Language=Polish
Określony czas jest nieprawidłowy.
.
Language=Romanian
The specified time is invalid.
.

MessageId=1902
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_NAME
Language=English
The specified form name is invalid.
.
Language=Russian
The specified form name is invalid.
.
Language=Polish
Określona nazwa formularza jest nieprawidłowa.
.
Language=Romanian
The specified form name is invalid.
.

MessageId=1903
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_FORM_SIZE
Language=English
The specified form size is invalid.
.
Language=Russian
The specified form size is invalid.
.
Language=Polish
Określony rozmiar formularza jest nieprawidłowy
.
Language=Romanian
The specified form size is invalid.
.

MessageId=1904
Severity=Success
Facility=System
SymbolicName=ERROR_ALREADY_WAITING
Language=English
The specified printer handle is already being waited on
.
Language=Russian
The specified printer handle is already being waited on
.
Language=Polish
Określone dojście drukarki jest już obsługiwane.
.
Language=Romanian
The specified printer handle is already being waited on
.

MessageId=1905
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DELETED
Language=English
The specified printer has been deleted.
.
Language=Russian
The specified printer has been deleted.
.
Language=Polish
Określona drukarka została usunięta.
.
Language=Romanian
The specified printer has been deleted.
.

MessageId=1906
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINTER_STATE
Language=English
The state of the printer is invalid.
.
Language=Russian
The state of the printer is invalid.
.
Language=Polish
Stan drukarki jest nieprawidłowy.
.
Language=Romanian
The state of the printer is invalid.
.

MessageId=1907
Severity=Success
Facility=System
SymbolicName=ERROR_PASSWORD_MUST_CHANGE
Language=English
The user's password must be changed before logging on the first time.
.
Language=Russian
The user's password must be changed before logging on the first time.
.
Language=Polish
Hasło użytkownika musi zostać zmienione przed pierwszym zalogowaniem.
.
Language=Romanian
The user's password must be changed before logging on the first time.
.

MessageId=1908
Severity=Success
Facility=System
SymbolicName=ERROR_DOMAIN_CONTROLLER_NOT_FOUND
Language=English
Could not find the domain controller for this domain.
.
Language=Russian
Could not find the domain controller for this domain.
.
Language=Polish
Nie można odnaleźć kontrolera tej domeny.
.
Language=Romanian
Could not find the domain controller for this domain.
.

MessageId=1909
Severity=Success
Facility=System
SymbolicName=ERROR_ACCOUNT_LOCKED_OUT
Language=English
The referenced account is currently locked out and may not be used to log on.
.
Language=Russian
The referenced account is currently locked out and may not be used to log on.
.
Language=Polish
Wywoływane konto jest obecnie zablokowane i nie można logować się za jego pomocą.
.
Language=Romanian
The referenced account is currently locked out and may not be used to log on.
.

MessageId=1910
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OXID
Language=English
The object exporter specified was not found.
.
Language=Russian
The object exporter specified was not found.
.
Language=Polish
Określony eksporter obiektu nie został odnaleziony.
.
Language=Romanian
The object exporter specified was not found.
.

MessageId=1911
Severity=Success
Facility=System
SymbolicName=OR_INVALID_OID
Language=English
The object specified was not found.
.
Language=Russian
The object specified was not found.
.
Language=Polish
Określony obiekt nie został odnaleziony.
.
Language=Romanian
The object specified was not found.
.

MessageId=1912
Severity=Success
Facility=System
SymbolicName=OR_INVALID_SET
Language=English
The object resolver set specified was not found.
.
Language=Russian
The object resolver set specified was not found.
.
Language=Polish
Określony zestaw programu rozpoznawania nazw obiektów nie został odnaleziony.
.
Language=Romanian
The object resolver set specified was not found.
.

MessageId=1913
Severity=Success
Facility=System
SymbolicName=RPC_S_SEND_INCOMPLETE
Language=English
Some data remains to be sent in the request buffer.
.
Language=Russian
Some data remains to be sent in the request buffer.
.
Language=Polish
Pewne dane pozostają w buforze żądania, oczekując na wysłanie.
.
Language=Romanian
Some data remains to be sent in the request buffer.
.

MessageId=1914
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ASYNC_HANDLE
Language=English
Invalid asynchronous remote procedure call handle.
.
Language=Russian
Invalid asynchronous remote procedure call handle.
.
Language=Polish
Nieprawidłowe dojście do asynchronicznego zdalnego wywołania procedury.
.
Language=Romanian
Invalid asynchronous remote procedure call handle.
.

MessageId=1915
Severity=Success
Facility=System
SymbolicName=RPC_S_INVALID_ASYNC_CALL
Language=English
Invalid asynchronous RPC call handle for this operation.
.
Language=Russian
Invalid asynchronous RPC call handle for this operation.
.
Language=Polish
Nieprawidłowe dojście do asynchronicznego zdalnego wywołania procedury (RPC) dla tej operacji.
.
Language=Romanian
Invalid asynchronous RPC call handle for this operation.
.

MessageId=1916
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_CLOSED
Language=English
The RPC pipe object has already been closed.
.
Language=Russian
The RPC pipe object has already been closed.
.
Language=Polish
Obiekt potoku RPC został już zamknięty.
.
Language=Romanian
The RPC pipe object has already been closed.
.

MessageId=1917
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_DISCIPLINE_ERROR
Language=English
The RPC call completed before all pipes were processed.
.
Language=Russian
The RPC call completed before all pipes were processed.
.
Language=Polish
Wywołanie RPC zostało ukończone przed przetworzeniem wszystkich potoków.
.
Language=Romanian
The RPC call completed before all pipes were processed.
.

MessageId=1918
Severity=Success
Facility=System
SymbolicName=RPC_X_PIPE_EMPTY
Language=English
No more data is available from the RPC pipe.
.
Language=Russian
No more data is available from the RPC pipe.
.
Language=Polish
Nie ma więcej dostępnych danych z potoku RPC.
.
Language=Romanian
No more data is available from the RPC pipe.
.

MessageId=1919
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SITENAME
Language=English
No site name is available for this machine.
.
Language=Russian
No site name is available for this machine.
.
Language=Polish
Żadna nazwa serwisu nie jest dostępna dla tego komputera.
.
Language=Romanian
No site name is available for this machine.
.

MessageId=1920
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_ACCESS_FILE
Language=English
The file cannot be accessed by the system.
.
Language=Russian
The file cannot be accessed by the system.
.
Language=Polish
System nie może uzyskać dostępu do pliku.
.
Language=Romanian
The file cannot be accessed by the system.
.

MessageId=1921
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_RESOLVE_FILENAME
Language=English
The name of the file cannot be resolved by the system.
.
Language=Russian
The name of the file cannot be resolved by the system.
.
Language=Polish
System nie może rozpoznać nazwy pliku.
.
Language=Romanian
The name of the file cannot be resolved by the system.
.

MessageId=1922
Severity=Success
Facility=System
SymbolicName=RPC_S_ENTRY_TYPE_MISMATCH
Language=English
The entry is not of the expected type.
.
Language=Russian
The entry is not of the expected type.
.
Language=Polish
Wpis nie jest oczekiwanego typu.
.
Language=Romanian
The entry is not of the expected type.
.

MessageId=1923
Severity=Success
Facility=System
SymbolicName=RPC_S_NOT_ALL_OBJS_EXPORTED
Language=English
Not all object UUIDs could be exported to the specified entry.
.
Language=Russian
Not all object UUIDs could be exported to the specified entry.
.
Language=Polish
Nie można wyeksportować niektórych identyfikatorów UUID obiektu do podanego wpisu.
.
Language=Romanian
Not all object UUIDs could be exported to the specified entry.
.

MessageId=1924
Severity=Success
Facility=System
SymbolicName=RPC_S_INTERFACE_NOT_EXPORTED
Language=English
Interface could not be exported to the specified entry.
.
Language=Russian
Interface could not be exported to the specified entry.
.
Language=Polish
Nie można wyeksportować interfejsu do podanego wpisu.
.
Language=Romanian
Interface could not be exported to the specified entry.
.

MessageId=1925
Severity=Success
Facility=System
SymbolicName=RPC_S_PROFILE_NOT_ADDED
Language=English
The specified profile entry could not be added.
.
Language=Russian
The specified profile entry could not be added.
.
Language=Polish
Nie można dodać podanego wpisu profilu.
.
Language=Romanian
The specified profile entry could not be added.
.

MessageId=1926
Severity=Success
Facility=System
SymbolicName=RPC_S_PRF_ELT_NOT_ADDED
Language=English
The specified profile element could not be added.
.
Language=Russian
The specified profile element could not be added.
.
Language=Polish
Nie można dodać podanego elementu profilu.
.
Language=Romanian
The specified profile element could not be added.
.

MessageId=1927
Severity=Success
Facility=System
SymbolicName=RPC_S_PRF_ELT_NOT_REMOVED
Language=English
The specified profile element could not be removed.
.
Language=Russian
The specified profile element could not be removed.
.
Language=Polish
Nie można usunąć podanego elementu profilu.
.
Language=Romanian
The specified profile element could not be removed.
.

MessageId=1928
Severity=Success
Facility=System
SymbolicName=RPC_S_GRP_ELT_NOT_ADDED
Language=English
The group element could not be added.
.
Language=Russian
The group element could not be added.
.
Language=Polish
Nie można dodać elementu grupy.
.
Language=Romanian
The group element could not be added.
.

MessageId=1929
Severity=Success
Facility=System
SymbolicName=RPC_S_GRP_ELT_NOT_REMOVED
Language=English
The group element could not be removed.
.
Language=Russian
The group element could not be removed.
.
Language=Polish
Nie można usunąć elementu grupy.
.
Language=Romanian
The group element could not be removed.
.

MessageId=1930
Severity=Success
Facility=System
SymbolicName=ERROR_KM_DRIVER_BLOCKED
Language=English
The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers.
.
Language=Russian
The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers.
.
Language=Polish
Sterownik drukarki jest niezgodny z włączonymi na danym komputerze zasadami, które blokują sterowniki systemu NT 4.0.
.
Language=Romanian
The printer driver is not compatible with a policy enabled on your computer that blocks NT 4.0 drivers.
.

MessageId=1931
Severity=Success
Facility=System
SymbolicName=ERROR_CONTEXT_EXPIRED
Language=English
The context has expired and can no longer be used.
.
Language=Russian
The context has expired and can no longer be used.
.
Language=Polish
Kontekst wygasł i nie może być już dłużej używany.
.
Language=Romanian
The context has expired and can no longer be used.
.

MessageId=1932
Severity=Success
Facility=System
SymbolicName=ERROR_PER_USER_TRUST_QUOTA_EXCEEDED
Language=English
The current user's delegated trust creation quota has been exceeded.
.
Language=Russian
The current user's delegated trust creation quota has been exceeded.
.
Language=Polish
Delegowany przydział tworzenia zaufania bieżącego użytkownika został przekroczony.
.
Language=Romanian
The current user's delegated trust creation quota has been exceeded.
.

MessageId=1933
Severity=Success
Facility=System
SymbolicName=ERROR_ALL_USER_TRUST_QUOTA_EXCEEDED
Language=English
The total delegated trust creation quota has been exceeded.
.
Language=Russian
The total delegated trust creation quota has been exceeded.
.
Language=Polish
Całkowity delegowany przydział tworzenia zaufania został przekroczony.
.
Language=Romanian
The total delegated trust creation quota has been exceeded.
.

MessageId=1934
Severity=Success
Facility=System
SymbolicName=ERROR_USER_DELETE_TRUST_QUOTA_EXCEEDED
Language=English
The current user's delegated trust deletion quota has been exceeded.
.
Language=Russian
The current user's delegated trust deletion quota has been exceeded.
.
Language=Polish
Delegowany przydział usuwania zaufania bieżącego użytkownika został przekroczony.
.
Language=Romanian
The current user's delegated trust deletion quota has been exceeded.
.

MessageId=2000
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PIXEL_FORMAT
Language=English
The pixel format is invalid.
.
Language=Russian
The pixel format is invalid.
.
Language=Polish
Format piksela jest nieprawidłowy.
.
Language=Romanian
The pixel format is invalid.
.

MessageId=2001
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_DRIVER
Language=English
The specified driver is invalid.
.
Language=Russian
The specified driver is invalid.
.
Language=Polish
Określony sterownik jest nieprawidłowy.
.
Language=Romanian
The specified driver is invalid.
.

MessageId=2002
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_WINDOW_STYLE
Language=English
The window style or class attribute is invalid for this operation.
.
Language=Russian
The window style or class attribute is invalid for this operation.
.
Language=Polish
Styl okna lub atrybut klasy jest nieodpowiedni dla tej operacji.
.
Language=Romanian
The window style or class attribute is invalid for this operation.
.

MessageId=2003
Severity=Success
Facility=System
SymbolicName=ERROR_METAFILE_NOT_SUPPORTED
Language=English
The requested metafile operation is not supported.
.
Language=Russian
The requested metafile operation is not supported.
.
Language=Polish
Żądana operacja metapliku nie jest obsługiwana.
.
Language=Romanian
The requested metafile operation is not supported.
.

MessageId=2004
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSFORM_NOT_SUPPORTED
Language=English
The requested transformation operation is not supported.
.
Language=Russian
The requested transformation operation is not supported.
.
Language=Polish
Żądana operacja transformacji nie jest obsługiwana.
.
Language=Romanian
The requested transformation operation is not supported.
.

MessageId=2005
Severity=Success
Facility=System
SymbolicName=ERROR_CLIPPING_NOT_SUPPORTED
Language=English
The requested clipping operation is not supported.
.
Language=Russian
The requested clipping operation is not supported.
.
Language=Polish
Żądana operacja clipping nie jest obsługiwana.
.
Language=Romanian
The requested clipping operation is not supported.
.

MessageId=2010
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CMM
Language=English
The specified color management module is invalid.
.
Language=Russian
The specified color management module is invalid.
.
Language=Polish
Podany moduł zarządzania kolorami jest nieprawidłowy.
.
Language=Romanian
The specified color management module is invalid.
.

MessageId=2011
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PROFILE
Language=English
The specified color profile is invalid.
.
Language=Russian
The specified color profile is invalid.
.
Language=Polish
Podany profil kolorów jest nieprawidłowy.
.
Language=Romanian
The specified color profile is invalid.
.

MessageId=2012
Severity=Success
Facility=System
SymbolicName=ERROR_TAG_NOT_FOUND
Language=English
The specified tag was not found.
.
Language=Russian
The specified tag was not found.
.
Language=Polish
Nie znaleziono podanej etykiety.
.
Language=Romanian
The specified tag was not found.
.

MessageId=2013
Severity=Success
Facility=System
SymbolicName=ERROR_TAG_NOT_PRESENT
Language=English
A required tag is not present.
.
Language=Russian
A required tag is not present.
.
Language=Polish
Brakuje wymaganej etykiety.
.
Language=Romanian
A required tag is not present.
.

MessageId=2014
Severity=Success
Facility=System
SymbolicName=ERROR_DUPLICATE_TAG
Language=English
The specified tag is already present.
.
Language=Russian
The specified tag is already present.
.
Language=Polish
Podana etykieta już istnieje.
.
Language=Romanian
The specified tag is already present.
.

MessageId=2015
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILE_NOT_ASSOCIATED_WITH_DEVICE
Language=English
The specified color profile is not associated with any device.
.
Language=Russian
The specified color profile is not associated with any device.
.
Language=Polish
Podany profil kolorów nie jest skojarzony z żadnym urządzeniem.
.
Language=Romanian
The specified color profile is not associated with any device.
.

MessageId=2016
Severity=Success
Facility=System
SymbolicName=ERROR_PROFILE_NOT_FOUND
Language=English
The specified color profile was not found.
.
Language=Russian
The specified color profile was not found.
.
Language=Polish
Nie znaleziono podanego profilu kolorów.
.
Language=Romanian
The specified color profile was not found.
.

MessageId=2017
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COLORSPACE
Language=English
The specified color space is invalid.
.
Language=Russian
The specified color space is invalid.
.
Language=Polish
Podana przestrzeń kolorów jest nieprawidłowa.
.
Language=Romanian
The specified color space is invalid.
.

MessageId=2018
Severity=Success
Facility=System
SymbolicName=ERROR_ICM_NOT_ENABLED
Language=English
Image Color Management is not enabled.
.
Language=Russian
Image Color Management is not enabled.
.
Language=Polish
Zarządzanie kolorami obrazu nie jest włączone.
.
Language=Romanian
Image Color Management is not enabled.
.

MessageId=2019
Severity=Success
Facility=System
SymbolicName=ERROR_DELETING_ICM_XFORM
Language=English
There was an error while deleting the color transform.
.
Language=Russian
There was an error while deleting the color transform.
.
Language=Polish
Podczas usuwania transformacji koloru wystąpił błąd.
.
Language=Romanian
There was an error while deleting the color transform.
.

MessageId=2020
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_TRANSFORM
Language=English
The specified color transform is invalid.
.
Language=Russian
The specified color transform is invalid.
.
Language=Polish
Podana transformacja kolorów jest nieprawidłowa.
.
Language=Romanian
The specified color transform is invalid.
.

MessageId=2021
Severity=Success
Facility=System
SymbolicName=ERROR_COLORSPACE_MISMATCH
Language=English
The specified transform does not match the bitmap's color space.
.
Language=Russian
The specified transform does not match the bitmap's color space.
.
Language=Polish
Podana transformacja nie jest zgodna z przestrzenią kolorów mapy bitowej.
.
Language=Romanian
The specified transform does not match the bitmap's color space.
.

MessageId=2022
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_COLORINDEX
Language=English
The specified named color index is not present in the profile.
.
Language=Russian
The specified named color index is not present in the profile.
.
Language=Polish
Podany indeks nazwanych kolorów nie występuje w profilu.
.
Language=Romanian
The specified named color index is not present in the profile.
.

MessageId=2108
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD
Language=English
The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.
.
Language=Russian
The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.
.
Language=Polish
Połączenie sieciowe zostało nawiązane, ale użytkownik był wezwany do podania hasła innego niż pierwotnie podane.
.
Language=Romanian
The network connection was made successfully, but the user had to be prompted for a password other than the one originally specified.
.

MessageId=2109
Severity=Success
Facility=System
SymbolicName=ERROR_CONNECTED_OTHER_PASSWORD_DEFAULT
Language=English
The network connection was made successfully using default credentials.
.
Language=Russian
The network connection was made successfully using default credentials.
.
Language=Polish
Ustanowienie połączenia przy użyciu poświadczeń domyślnych powiodło się.
.
Language=Romanian
The network connection was made successfully using default credentials.
.

MessageId=2202
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_USERNAME
Language=English
The specified username is invalid.
.
Language=Russian
The specified username is invalid.
.
Language=Polish
Określona nazwa użytkownika jest nieprawidłowa.
.
Language=Romanian
The specified username is invalid.
.

MessageId=2250
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_CONNECTED
Language=English
This network connection does not exist.
.
Language=Russian
This network connection does not exist.
.
Language=Polish
To połączenie sieciowe nie istnieje.
.
Language=Romanian
This network connection does not exist.
.

MessageId=2401
Severity=Success
Facility=System
SymbolicName=ERROR_OPEN_FILES
Language=English
This network connection has files open or requests pending.
.
Language=Russian
This network connection has files open or requests pending.
.
Language=Polish
To połączenie sieciowe ma otwarte pliki lub aktywne żądania.
.
Language=Romanian
This network connection has files open or requests pending.
.

MessageId=2402
Severity=Success
Facility=System
SymbolicName=ERROR_ACTIVE_CONNECTIONS
Language=English
Active connections still exist.
.
Language=Russian
Active connections still exist.
.
Language=Polish
Nadal istnieją aktywne połączenia.
.
Language=Romanian
Active connections still exist.
.

MessageId=2404
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_IN_USE
Language=English
The device is in use by an active process and cannot be disconnected.
.
Language=Russian
The device is in use by an active process and cannot be disconnected.
.
Language=Polish
Urządzenie jest używane przez aktywny proces i nie można go odłączyć.
.
Language=Romanian
The device is in use by an active process and cannot be disconnected.
.

MessageId=3000
Severity=Success
Facility=System
SymbolicName=ERROR_UNKNOWN_PRINT_MONITOR
Language=English
The specified print monitor is unknown.
.
Language=Russian
The specified print monitor is unknown.
.
Language=Polish
Określony monitor wydruku jest nieznany.
.
Language=Romanian
The specified print monitor is unknown.
.

MessageId=3001
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_IN_USE
Language=English
The specified printer driver is currently in use.
.
Language=Russian
The specified printer driver is currently in use.
.
Language=Polish
Określony sterownik drukarki jest obecnie w użyciu.
.
Language=Romanian
The specified printer driver is currently in use.
.

MessageId=3002
Severity=Success
Facility=System
SymbolicName=ERROR_SPOOL_FILE_NOT_FOUND
Language=English
The spool file was not found.
.
Language=Russian
The spool file was not found.
.
Language=Polish
Nie znaleziono pliku buforowania.
.
Language=Romanian
The spool file was not found.
.

MessageId=3003
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_STARTDOC
Language=English
A StartDocPrinter call was not issued.
.
Language=Russian
A StartDocPrinter call was not issued.
.
Language=Polish
Wywołanie StartDocPrinter nie zostało wysłane.
.
Language=Romanian
A StartDocPrinter call was not issued.
.

MessageId=3004
Severity=Success
Facility=System
SymbolicName=ERROR_SPL_NO_ADDJOB
Language=English
An AddJob call was not issued.
.
Language=Russian
An AddJob call was not issued.
.
Language=Polish
Wywołanie AddJob nie zostało wysłane.
.
Language=Romanian
An AddJob call was not issued.
.

MessageId=3005
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED
Language=English
The specified print processor has already been installed.
.
Language=Russian
The specified print processor has already been installed.
.
Language=Polish
Określony procesor wydruku jest już zainstalowany.
.
Language=Romanian
The specified print processor has already been installed.
.

MessageId=3006
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_MONITOR_ALREADY_INSTALLED
Language=English
The specified print monitor has already been installed.
.
Language=Russian
The specified print monitor has already been installed.
.
Language=Polish
Określony monitor wydruku jest już zainstalowany.
.
Language=Romanian
The specified print monitor has already been installed.
.

MessageId=3007
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_PRINT_MONITOR
Language=English
The specified print monitor does not have the required functions.
.
Language=Russian
The specified print monitor does not have the required functions.
.
Language=Polish
Określony monitor wydruku nie ma wymaganych funkcji.
.
Language=Romanian
The specified print monitor does not have the required functions.
.

MessageId=3008
Severity=Success
Facility=System
SymbolicName=ERROR_PRINT_MONITOR_IN_USE
Language=English
The specified print monitor is currently in use.
.
Language=Russian
The specified print monitor is currently in use.
.
Language=Polish
Określony monitor wydruku jest już w użyciu.
.
Language=Romanian
The specified print monitor is currently in use.
.

MessageId=3009
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_HAS_JOBS_QUEUED
Language=English
The requested operation is not allowed when there are jobs queued to the printer.
.
Language=Russian
The requested operation is not allowed when there are jobs queued to the printer.
.
Language=Polish
Żądana operacja nie jest dopuszczalna, gdy w kolejce drukarki znajdują się zadania.
.
Language=Romanian
The requested operation is not allowed when there are jobs queued to the printer.
.

MessageId=3010
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_REBOOT_REQUIRED
Language=English
The requested operation is successful. Changes will not be effective until the system is rebooted.
.
Language=Russian
The requested operation is successful. Changes will not be effective until the system is rebooted.
.
Language=Polish
Żądana operacja powiodła się. Zmiany nie odniosą skutku aż do ponownego uruchomienia systemu.
.
Language=Romanian
The requested operation is successful. Changes will not be effective until the system is rebooted.
.

MessageId=3011
Severity=Success
Facility=System
SymbolicName=ERROR_SUCCESS_RESTART_REQUIRED
Language=English
The requested operation is successful. Changes will not be effective until the service is restarted.
.
Language=Russian
The requested operation is successful. Changes will not be effective until the service is restarted.
.
Language=Polish
Żądana operacja powiodła się. Zmiany nie odniosą skutku aż do ponownego uruchomienia usługi.
.
Language=Romanian
The requested operation is successful. Changes will not be effective until the service is restarted.
.

MessageId=3012
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_NOT_FOUND
Language=English
No printers were found.
.
Language=Russian
No printers were found.
.
Language=Polish
Nie znaleziono żadnych drukarek.
.
Language=Romanian
No printers were found.
.

MessageId=3013
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_WARNED
Language=English
The printer driver is known to be unreliable.
.
Language=Russian
The printer driver is known to be unreliable.
.
Language=Polish
Sterownik drukarki jest znany jako zawodny.
.
Language=Romanian
The printer driver is known to be unreliable.
.

MessageId=3014
Severity=Success
Facility=System
SymbolicName=ERROR_PRINTER_DRIVER_BLOCKED
Language=English
The printer driver is known to harm the system.
.
Language=Russian
The printer driver is known to harm the system.
.
Language=Polish
Sterownik drukarki jest znany jako szkodliwy dla systemu.
.
Language=Romanian
The printer driver is known to harm the system.
.

MessageId=3100
Severity=Success
Facility=System
SymbolicName=ERROR_XML_UNDEFINED_ENTITY
Language=English
The XML contains an entity reference to an undefined entity.
.
Language=Russian
The XML contains an entity reference to an undefined entity.
.
Language=Polish
Kod XML zawiera odwołanie do niezdefiniowanej encji.
.
Language=Romanian
The XML contains an entity reference to an undefined entity.
.

MessageId=3101
Severity=Success
Facility=System
SymbolicName=ERROR_XML_MALFORMED_ENTITY
Language=English
The XML contains a malformed entity reference.
.
Language=Russian
The XML contains a malformed entity reference.
.
Language=Polish
Kod XML zawiera nieprawidłowe odwołanie do encji.
.
Language=Romanian
The XML contains a malformed entity reference.
.

MessageId=3102
Severity=Success
Facility=System
SymbolicName=ERROR_XML_CHAR_NOT_IN_RANGE
Language=English
The XML contains a character which is not permitted in XML.
.
Language=Russian
The XML contains a character which is not permitted in XML.
.
Language=Polish
Kod XML zawiera znak, który nie jest dozwolony w języku w XML.
.
Language=Romanian
The XML contains a character which is not permitted in XML.
.

MessageId=3200
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_EXTERNAL_PROXY
Language=English
The manifest contained a duplicate definition for external proxy stub %1 at (%1:%2,%3)
.
Language=Russian
The manifest contained a duplicate definition for external proxy stub %1 at (%1:%2,%3)
.
Language=Polish
Manifest zawiera zduplikowaną definicję dla zewnętrznej procedury wejścia obiektu proxy %1 w (%1:%2,%3)
.
Language=Romanian
The manifest contained a duplicate definition for external proxy stub %1 at (%1:%2,%3)
.

MessageId=3201
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_ASSEMBLY_REFERENCE
Language=English
The manifest already contains a reference to %4 - a second reference was found at (%1:%2,%3)
.
Language=Russian
The manifest already contains a reference to %4 - a second reference was found at (%1:%2,%3)
.
Language=Polish
Manifest zawiera już odwołanie do %4 - drugie odwołanie odnaleziono w (%1:%2,%3)
.
Language=Romanian
The manifest already contains a reference to %4 - a second reference was found at (%1:%2,%3)
.

MessageId=3202
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ASSEMBLY_REFERENCE
Language=English
The assembly reference at (%1:%2,%3) is invalid.
.
Language=Russian
The assembly reference at (%1:%2,%3) is invalid.
.
Language=Polish
Nieprawidłowe odwołanie do zestawu w (%1:%2,%3).
.
Language=Romanian
The assembly reference at (%1:%2,%3) is invalid.
.

MessageId=3203
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ASSEMBLY_DEFINITION
Language=English
The assembly definition at (%1:%2,%3) is invalid.
.
Language=Russian
The assembly definition at (%1:%2,%3) is invalid.
.
Language=Polish
Nieprawidłowa definicja zestawu w (%1:%2,%3).
.
Language=Romanian
The assembly definition at (%1:%2,%3) is invalid.
.

MessageId=3204
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_WINDOW_CLASS
Language=English
The manifest already contained the window class %4, found a second declaration at (%1:%2,%3)
.
Language=Russian
The manifest already contained the window class %4, found a second declaration at (%1:%2,%3)
.
Language=Polish
Manifest zawiera już klasę okna %4, drugą deklarację odnaleziono w (%1:%2,%3).
.
Language=Romanian
The manifest already contained the window class %4, found a second declaration at (%1:%2,%3)
.

MessageId=3205
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_PROGID
Language=English
The manifest already declared the progId %4, found a second declaration at (%1:%2,%3)
.
Language=Russian
The manifest already declared the progId %4, found a second declaration at (%1:%2,%3)
.
Language=Polish
Manifest już zadeklarował identyfikator progId %4, drugą deklarację odnaleziono w (%1:%2,%3)
.
Language=Romanian
The manifest already declared the progId %4, found a second declaration at (%1:%2,%3)
.

MessageId=3206
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_NOINHERIT
Language=English
Only one noInherit tag may be present in a manifest, found a second tag at (%1:%2,%3)
.
Language=Russian
Only one noInherit tag may be present in a manifest, found a second tag at (%1:%2,%3)
.
Language=Polish
W manifeście może występować tylko jeden tag noInherit, drugi tag odnaleziono w (%1:%2,%3)
.
Language=Romanian
Only one noInherit tag may be present in a manifest, found a second tag at (%1:%2,%3)
.

MessageId=3207
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_NOINHERITABLE
Language=English
Only one noInheritable tag may be present in a manifest, found a second tag at (%1:%2,%3)
.
Language=Russian
Only one noInheritable tag may be present in a manifest, found a second tag at (%1:%2,%3)
.
Language=Polish
W manifeście może występować tylko jeden tag noInheritable, drugi tag odnaleziono w (%1:%2,%3)
.
Language=Romanian
Only one noInheritable tag may be present in a manifest, found a second tag at (%1:%2,%3)
.

MessageId=3208
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_COM_CLASS
Language=English
The manifest contained a duplicate declaration of COM class %4 at (%1:%2,%3)
.
Language=Russian
The manifest contained a duplicate declaration of COM class %4 at (%1:%2,%3)
.
Language=Polish
Manifest zawiera zduplikowaną deklarację klasy COM %4 w (%1:%2,%3)
.
Language=Romanian
The manifest contained a duplicate declaration of COM class %4 at (%1:%2,%3)
.

MessageId=3209
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_FILE_NAME
Language=English
The manifest already declared the file %4, a second definition was found at (%1:%2,%3)
.
Language=Russian
The manifest already declared the file %4, a second definition was found at (%1:%2,%3)
.
Language=Polish
Manifest już zadeklarował plik %4, drugą definicję odnaleziono w (%1:%2,%3)
.
Language=Romanian
The manifest already declared the file %4, a second definition was found at (%1:%2,%3)
.

MessageId=3210
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_CLR_SURROGATE
Language=English
CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Russian
CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Polish
CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Romanian
CLR surrogate %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3211
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_TYPE_LIBRARY
Language=English
Type library %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Russian
Type library %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Polish
Type library %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Romanian
Type library %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3212
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_PROXY_STUB
Language=English
Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Russian
Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Polish
Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid.
.
Language=Romanian
Proxy stub definition %1 was already defined, second definition at (%1:%2,%3) is invalid.
.

MessageId=3213
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_CATEGORY_NAME
Language=English
Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid.
.
Language=Russian
Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid.
.
Language=Polish
Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid.
.
Language=Romanian
Category friendly name %4 was already used, second definition was found at (%1:%2,%3) is invalid.
.

MessageId=3214
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_DUPLICATE_TOP_LEVEL_IDENTITY_FOUND
Language=English
Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3)
.
Language=Russian
Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3)
.
Language=Polish
Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3)
.
Language=Romanian
Only one top-level assemblyIdentity tag may be present in a manifest. A second tag with identity %4 was found at (%1:%2,%3)
.

MessageId=3215
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_UNKNOWN_ROOT_ELEMENT
Language=English
The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version.
.
Language=Russian
The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version.
.
Language=Polish
The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version.
.
Language=Romanian
The root element for a manifest found at (%1:%2,%3) was not expected or was of the wrong version.
.

MessageId=3216
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ELEMENT
Language=English
The element found at (%1:%2,%3) was not expected according to the manifest schema.
.
Language=Russian
The element found at (%1:%2,%3) was not expected according to the manifest schema.
.
Language=Polish
The element found at (%1:%2,%3) was not expected according to the manifest schema.
.
Language=Romanian
The element found at (%1:%2,%3) was not expected according to the manifest schema.
.

MessageId=3217
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_MISSING_REQUIRED_ATTRIBUTE
Language=English
The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information
.
Language=Russian
The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information
.
Language=Polish
The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information
.
Language=Romanian
The element found at (%1:%2,%3) was missing the required attribute '%4'. See the manifest schema for more information
.

MessageId=3218
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_INVALID_ATTRIBUTE_VALUE
Language=English
The attribute value %4 at (%1:%2,%3) was invalid according to the schema.
.
Language=Russian
The attribute value %4 at (%1:%2,%3) was invalid according to the schema.
.
Language=Polish
The attribute value %4 at (%1:%2,%3) was invalid according to the schema.
.
Language=Romanian
The attribute value %4 at (%1:%2,%3) was invalid according to the schema.
.

MessageId=3219
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_COMPILER_UNEXPECTED_PCDATA
Language=English
PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4.
.
Language=Russian
PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4.
.
Language=Polish
PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4.
.
Language=Romanian
PCDATA or CDATA found at (%1:%2,%3) in the source document was not expected in the parent element %4.
.

MessageId=3220
Severity=Success
Facility=System
SymbolicName=ERROR_PCM_DUPLICATE_STRING_TABLE_ENT
Language=English
The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry.
.
Language=Russian
The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry.
.
Language=Polish
The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry.
.
Language=Romanian
The string table entry with culture %4, name %5, and value '%6' at (%1:%2,%3) duplicated a previous entry.
.

MessageId=4000
Severity=Success
Facility=System
SymbolicName=ERROR_WINS_INTERNAL
Language=English
WINS encountered an error while processing the command.
.
Language=Russian
WINS encountered an error while processing the command.
.
Language=Polish
WINS napotkał na błąd podczas przetwarzania polecenia.
.
Language=Romanian
WINS encountered an error while processing the command.
.

MessageId=4001
Severity=Success
Facility=System
SymbolicName=ERROR_CAN_NOT_DEL_LOCAL_WINS
Language=English
The local WINS cannot be deleted.
.
Language=Russian
The local WINS cannot be deleted.
.
Language=Polish
Lokalny WINS nie może być usunięty.
.
Language=Romanian
The local WINS cannot be deleted.
.

MessageId=4002
Severity=Success
Facility=System
SymbolicName=ERROR_STATIC_INIT
Language=English
The importation from the file failed.
.
Language=Russian
The importation from the file failed.
.
Language=Polish
Importowanie z pliku nie powiodło się.
.
Language=Romanian
The importation from the file failed.
.

MessageId=4003
Severity=Success
Facility=System
SymbolicName=ERROR_INC_BACKUP
Language=English
The backup failed. Was a full backup done before?
.
Language=Russian
The backup failed. Was a full backup done before?
.
Language=Polish
Wykonanie kopii zapasowej nie powiodło się. Czy wcześniej wykonywano pełną kopię zapasową?
.
Language=Romanian
The backup failed. Was a full backup done before?
.

MessageId=4004
Severity=Success
Facility=System
SymbolicName=ERROR_FULL_BACKUP
Language=English
The backup failed. Check the directory to which you are backing the database.
.
Language=Russian
The backup failed. Check the directory to which you are backing the database.
.
Language=Polish
Wykonanie kopii zapasowej nie powiodło się. Sprawdź katalog, do którego jest wykonywana kopia zapasowa bazy danych.
.
Language=Romanian
The backup failed. Check the directory to which you are backing the database.
.

MessageId=4005
Severity=Success
Facility=System
SymbolicName=ERROR_REC_NON_EXISTENT
Language=English
The name does not exist in the WINS database.
.
Language=Russian
The name does not exist in the WINS database.
.
Language=Polish
Nazwa nie istnieje w bazie danych WINS.
.
Language=Romanian
The name does not exist in the WINS database.
.

MessageId=4006
Severity=Success
Facility=System
SymbolicName=ERROR_RPL_NOT_ALLOWED
Language=English
Replication with a nonconfigured partner is not allowed.
.
Language=Russian
Replication with a nonconfigured partner is not allowed.
.
Language=Polish
Replikowanie z nie skonfigurowanym partnerem jest niedozwolone.
.
Language=Romanian
Replication with a nonconfigured partner is not allowed.
.

MessageId=4100
Severity=Success
Facility=System
SymbolicName=ERROR_DHCP_ADDRESS_CONFLICT
Language=English
The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.
.
Language=Russian
The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.
.
Language=Polish
Klient DHCP otrzymał adres IP, który jest już używany w sieci. Lokalny interfejs zostanie wyłączony do chwili, gdy klient otrzyma nowy adres.
.
Language=Romanian
The DHCP client has obtained an IP address that is already in use on the network. The local interface will be disabled until the DHCP client can obtain a new address.
.

MessageId=4200
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_GUID_NOT_FOUND
Language=English
The GUID passed was not recognized as valid by a WMI data provider.
.
Language=Russian
The GUID passed was not recognized as valid by a WMI data provider.
.
Language=Polish
Przekazany identyfikator GUID nie został uznany przez dostawcę danych WMI za prawidłowy.
.
Language=Romanian
The GUID passed was not recognized as valid by a WMI data provider.
.

MessageId=4201
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INSTANCE_NOT_FOUND
Language=English
The instance name passed was not recognized as valid by a WMI data provider.
.
Language=Russian
The instance name passed was not recognized as valid by a WMI data provider.
.
Language=Polish
Przekazana nazwa wystąpienia nie została uznana przez dostawcę danych WMI za prawidłową.
.
Language=Romanian
The instance name passed was not recognized as valid by a WMI data provider.
.

MessageId=4202
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ITEMID_NOT_FOUND
Language=English
The data item ID passed was not recognized as valid by a WMI data provider.
.
Language=Russian
The data item ID passed was not recognized as valid by a WMI data provider.
.
Language=Polish
Przekazany identyfikator elementu danych nie został uznany przez dostawcę danych WMI za prawidłowy.
.
Language=Romanian
The data item ID passed was not recognized as valid by a WMI data provider.
.

MessageId=4203
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_TRY_AGAIN
Language=English
The WMI request could not be completed and should be retried.
.
Language=Russian
The WMI request could not be completed and should be retried.
.
Language=Polish
Nie można ukończyć żądania WMI; należy je powtórzyć.
.
Language=Romanian
The WMI request could not be completed and should be retried.
.

MessageId=4204
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_DP_NOT_FOUND
Language=English
The WMI data provider could not be located.
.
Language=Russian
The WMI data provider could not be located.
.
Language=Polish
Nie można zlokalizować dostawcy danych WMI.
.
Language=Romanian
The WMI data provider could not be located.
.

MessageId=4205
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_UNRESOLVED_INSTANCE_REF
Language=English
The WMI data provider references an instance set that has not been registered.
.
Language=Russian
The WMI data provider references an instance set that has not been registered.
.
Language=Polish
Dostawca danych WMI odwołuje się do zestawu wystąpień, który nie został zarejestrowany.
.
Language=Romanian
The WMI data provider references an instance set that has not been registered.
.

MessageId=4206
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ALREADY_ENABLED
Language=English
The WMI data block or event notification has already been enabled.
.
Language=Russian
The WMI data block or event notification has already been enabled.
.
Language=Polish
Blok danych WMI lub powiadamianie o zdarzeniach WMI jest już włączone.
.
Language=Romanian
The WMI data block or event notification has already been enabled.
.

MessageId=4207
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_GUID_DISCONNECTED
Language=English
The WMI data block is no longer available.
.
Language=Russian
The WMI data block is no longer available.
.
Language=Polish
Blok danych WMI nie jest już dostępny.
.
Language=Romanian
The WMI data block is no longer available.
.

MessageId=4208
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_SERVER_UNAVAILABLE
Language=English
The WMI data service is not available.
.
Language=Russian
The WMI data service is not available.
.
Language=Polish
Usługa danych WMI nie jest dostępna.
.
Language=Romanian
The WMI data service is not available.
.

MessageId=4209
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_DP_FAILED
Language=English
The WMI data provider failed to carry out the request.
.
Language=Russian
The WMI data provider failed to carry out the request.
.
Language=Polish
Dostawca danych WMI nie może spełnić żądania.
.
Language=Romanian
The WMI data provider failed to carry out the request.
.

MessageId=4210
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INVALID_MOF
Language=English
The WMI MOF information is not valid.
.
Language=Russian
The WMI MOF information is not valid.
.
Language=Polish
Informacje WMI MOF są nieprawidłowe.
.
Language=Romanian
The WMI MOF information is not valid.
.

MessageId=4211
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_INVALID_REGINFO
Language=English
The WMI registration information is not valid.
.
Language=Russian
The WMI registration information is not valid.
.
Language=Polish
Informacje rejestracyjne WMI są nieprawidłowe.
.
Language=Romanian
The WMI registration information is not valid.
.

MessageId=4212
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_ALREADY_DISABLED
Language=English
The WMI data block or event notification has already been disabled.
.
Language=Russian
The WMI data block or event notification has already been disabled.
.
Language=Polish
Blok danych WMI lub powiadamianie o zdarzeniach WMI jest już wyłączone.
.
Language=Romanian
The WMI data block or event notification has already been disabled.
.

MessageId=4213
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_READ_ONLY
Language=English
The WMI data item or data block is read only.
.
Language=Russian
The WMI data item or data block is read only.
.
Language=Polish
Element danych WMI lub blok danych są tylko do odczytu.
.
Language=Romanian
The WMI data item or data block is read only.
.

MessageId=4214
Severity=Success
Facility=System
SymbolicName=ERROR_WMI_SET_FAILURE
Language=English
The WMI data item or data block could not be changed.
.
Language=Russian
The WMI data item or data block could not be changed.
.
Language=Polish
Nie można zmienić elementu danych WMI lub bloku danych.
.
Language=Romanian
The WMI data item or data block could not be changed.
.

MessageId=4300
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEDIA
Language=English
The media identifier does not represent a valid medium.
.
Language=Russian
The media identifier does not represent a valid medium.
.
Language=Polish
Identyfikator nośnika nie reprezentuje prawidłowej oczyszczarki.
.
Language=Romanian
The media identifier does not represent a valid medium.
.

MessageId=4301
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_LIBRARY
Language=English
The library identifier does not represent a valid library.
.
Language=Russian
The library identifier does not represent a valid library.
.
Language=Polish
Identyfikator biblioteki nie reprezentuje prawidłowej biblioteki.
.
Language=Romanian
The library identifier does not represent a valid library.
.

MessageId=4302
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_MEDIA_POOL
Language=English
The media pool identifier does not represent a valid media pool.
.
Language=Russian
The media pool identifier does not represent a valid media pool.
.
Language=Polish
Identyfikator zestawu nośników nie reprezentuje prawidłowej puli nośników.
.
Language=Romanian
The media pool identifier does not represent a valid media pool.
.

MessageId=4303
Severity=Success
Facility=System
SymbolicName=ERROR_DRIVE_MEDIA_MISMATCH
Language=English
The drive and medium are not compatible or exist in different libraries.
.
Language=Russian
The drive and medium are not compatible or exist in different libraries.
.
Language=Polish
Stacja i nośnik nie są zgodne lub znajdują się w innych bibliotekach.
.
Language=Romanian
The drive and medium are not compatible or exist in different libraries.
.

MessageId=4304
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_OFFLINE
Language=English
The medium currently exists in an offline library and must be online to perform this operation.
.
Language=Russian
The medium currently exists in an offline library and must be online to perform this operation.
.
Language=Polish
Nośnik znajduje się obecnie w bibliotece będącej w trybie offline. Aby ta operacja została wykonana, ta biblioteka musi być w trybie online.
.
Language=Romanian
The medium currently exists in an offline library and must be online to perform this operation.
.

MessageId=4305
Severity=Success
Facility=System
SymbolicName=ERROR_LIBRARY_OFFLINE
Language=English
The operation cannot be performed on an offline library.
.
Language=Russian
The operation cannot be performed on an offline library.
.
Language=Polish
Nie można wykonać operacji na bibliotece będącej w trybie offline.
.
Language=Romanian
The operation cannot be performed on an offline library.
.

MessageId=4306
Severity=Success
Facility=System
SymbolicName=ERROR_EMPTY
Language=English
The library, drive, or media pool is empty.
.
Language=Russian
The library, drive, or media pool is empty.
.
Language=Polish
Biblioteka, stacja dysków lub pula nośników są puste.
.
Language=Romanian
The library, drive, or media pool is empty.
.

MessageId=4307
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_EMPTY
Language=English
The library, drive, or media pool must be empty to perform this operation.
.
Language=Russian
The library, drive, or media pool must be empty to perform this operation.
.
Language=Polish
Biblioteka, stacja dysków lub pula nośników muszą być puste, aby można było wykonać tę operację.
.
Language=Romanian
The library, drive, or media pool must be empty to perform this operation.
.

MessageId=4308
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_UNAVAILABLE
Language=English
No media is currently available in this media pool or library.
.
Language=Russian
No media is currently available in this media pool or library.
.
Language=Polish
Żaden nośnik nie jest obecnie dostępny w tej puli nośników lub w bibliotece.
.
Language=Romanian
No media is currently available in this media pool or library.
.

MessageId=4309
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_DISABLED
Language=English
A resource required for this operation is disabled.
.
Language=Russian
A resource required for this operation is disabled.
.
Language=Polish
Zasób wymagany dla tej operacji jest wyłączony.
.
Language=Romanian
A resource required for this operation is disabled.
.

MessageId=4310
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_CLEANER
Language=English
The media identifier does not represent a valid cleaner.
.
Language=Russian
The media identifier does not represent a valid cleaner.
.
Language=Polish
Identyfikator nośnika nie reprezentuje prawidłowego nośnika czyszczącego.
.
Language=Romanian
The media identifier does not represent a valid cleaner.
.

MessageId=4311
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_CLEAN
Language=English
The drive cannot be cleaned or does not support cleaning.
.
Language=Russian
The drive cannot be cleaned or does not support cleaning.
.
Language=Polish
Nie można oczyścić stacji lub nie obsługuje ona funkcji czyszczenia.
.
Language=Romanian
The drive cannot be cleaned or does not support cleaning.
.

MessageId=4312
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_NOT_FOUND
Language=English
The object identifier does not represent a valid object.
.
Language=Russian
The object identifier does not represent a valid object.
.
Language=Polish
Identyfikator obiektu nie reprezentuje prawidłowego obiektu.
.
Language=Romanian
The object identifier does not represent a valid object.
.

MessageId=4313
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_FAILURE
Language=English
Unable to read from or write to the database.
.
Language=Russian
Unable to read from or write to the database.
.
Language=Polish
Nie można odczytać z bazy danych lub do niej zapisać.
.
Language=Romanian
Unable to read from or write to the database.
.

MessageId=4314
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_FULL
Language=English
The database is full.
.
Language=Russian
The database is full.
.
Language=Polish
Baza danych jest zapełniona.
.
Language=Romanian
The database is full.
.

MessageId=4315
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_INCOMPATIBLE
Language=English
The medium is not compatible with the device or media pool.
.
Language=Russian
The medium is not compatible with the device or media pool.
.
Language=Polish
Nośnik nie jest zgodny z urządzeniem lub pulą nośników.
.
Language=Romanian
The medium is not compatible with the device or media pool.
.

MessageId=4316
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_PRESENT
Language=English
The resource required for this operation does not exist.
.
Language=Russian
The resource required for this operation does not exist.
.
Language=Polish
Wymagany dla tej operacji zasób nie istnieje.
.
Language=Romanian
The resource required for this operation does not exist.
.

MessageId=4317
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPERATION
Language=English
The operation identifier is not valid.
.
Language=Russian
The operation identifier is not valid.
.
Language=Polish
Identyfikator operacji jest nieprawidłowy.
.
Language=Romanian
The operation identifier is not valid.
.

MessageId=4318
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIA_NOT_AVAILABLE
Language=English
The media is not mounted or ready for use.
.
Language=Russian
The media is not mounted or ready for use.
.
Language=Polish
Nośnik nie jest zainstalowany lub nie jest gotowy do użycia.
.
Language=Romanian
The media is not mounted or ready for use.
.

MessageId=4319
Severity=Success
Facility=System
SymbolicName=ERROR_DEVICE_NOT_AVAILABLE
Language=English
The device is not ready for use.
.
Language=Russian
The device is not ready for use.
.
Language=Polish
Urządzenie nie jest gotowe do użycia.
.
Language=Romanian
The device is not ready for use.
.

MessageId=4320
Severity=Success
Facility=System
SymbolicName=ERROR_REQUEST_REFUSED
Language=English
The operator or administrator has refused the request.
.
Language=Russian
The operator or administrator has refused the request.
.
Language=Polish
Operator lub administrator odrzucił żądanie.
.
Language=Romanian
The operator or administrator has refused the request.
.

MessageId=4321
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_DRIVE_OBJECT
Language=English
The drive identifier does not represent a valid drive.
.
Language=Russian
The drive identifier does not represent a valid drive.
.
Language=Polish
Identyfikator stacji nie reprezentuje prawidłowej stacji.
.
Language=Romanian
The drive identifier does not represent a valid drive.
.

MessageId=4322
Severity=Success
Facility=System
SymbolicName=ERROR_LIBRARY_FULL
Language=English
Library is full. No slot is available for use.
.
Language=Russian
Library is full. No slot is available for use.
.
Language=Polish
Biblioteka jest zapełniona. Nie ma żadnego wolnego gniazda do użycia.
.
Language=Romanian
Library is full. No slot is available for use.
.

MessageId=4323
Severity=Success
Facility=System
SymbolicName=ERROR_MEDIUM_NOT_ACCESSIBLE
Language=English
The transport cannot access the medium.
.
Language=Russian
The transport cannot access the medium.
.
Language=Polish
Transport nie może uzyskać dostępu do nośnika.
.
Language=Romanian
The transport cannot access the medium.
.

MessageId=4324
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_LOAD_MEDIUM
Language=English
Unable to load the medium into the drive.
.
Language=Russian
Unable to load the medium into the drive.
.
Language=Polish
Nie można włożyć nośnika do stacji.
.
Language=Romanian
Unable to load the medium into the drive.
.

MessageId=4325
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_DRIVE
Language=English
Unable to retrieve status about the drive.
.
Language=Russian
Unable to retrieve status about the drive.
.
Language=Polish
Nie można pobrać danych o stanie stacji.
.
Language=Romanian
Unable to retrieve status about the drive.
.

MessageId=4326
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_SLOT
Language=English
Unable to retrieve status about the slot.
.
Language=Russian
Unable to retrieve status about the slot.
.
Language=Polish
Nie można pobrać danych o stanie gniazda.
.
Language=Romanian
Unable to retrieve status about the slot.
.

MessageId=4327
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_INVENTORY_TRANSPORT
Language=English
Unable to retrieve status about the transport.
.
Language=Russian
Unable to retrieve status about the transport.
.
Language=Polish
Nie można pobrać danych o stanie transportu.
.
Language=Romanian
Unable to retrieve status about the transport.
.

MessageId=4328
Severity=Success
Facility=System
SymbolicName=ERROR_TRANSPORT_FULL
Language=English
Cannot use the transport because it is already in use.
.
Language=Russian
Cannot use the transport because it is already in use.
.
Language=Polish
Nie można użyć transportu, ponieważ jest już używany.
.
Language=Romanian
Cannot use the transport because it is already in use.
.

MessageId=4329
Severity=Success
Facility=System
SymbolicName=ERROR_CONTROLLING_IEPORT
Language=English
Unable to open or close the inject/eject port.
.
Language=Russian
Unable to open or close the inject/eject port.
.
Language=Polish
Nie można otworzyć lub zamknąć portu wsuwania/wysuwania.
.
Language=Romanian
Unable to open or close the inject/eject port.
.

MessageId=4330
Severity=Success
Facility=System
SymbolicName=ERROR_UNABLE_TO_EJECT_MOUNTED_MEDIA
Language=English
Unable to eject the media because it is in a drive.
.
Language=Russian
Unable to eject the media because it is in a drive.
.
Language=Polish
Nie można wsunąć nośnika, ponieważ jest w stacji.
.
Language=Romanian
Unable to eject the media because it is in a drive.
.

MessageId=4331
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_SLOT_SET
Language=English
A cleaner slot is already reserved.
.
Language=Russian
A cleaner slot is already reserved.
.
Language=Polish
Gniazdo oczyszczarki jest już zarezerwowane.
.
Language=Romanian
A cleaner slot is already reserved.
.

MessageId=4332
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_SLOT_NOT_SET
Language=English
A cleaner slot is not reserved.
.
Language=Russian
A cleaner slot is not reserved.
.
Language=Polish
Gniazdo oczyszczarki nie jest zarezerwowane.
.
Language=Romanian
A cleaner slot is not reserved.
.

MessageId=4333
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_CARTRIDGE_SPENT
Language=English
The cleaner cartridge has performed the maximum number of drive cleanings.
.
Language=Russian
The cleaner cartridge has performed the maximum number of drive cleanings.
.
Language=Polish
Kaseta czyszcząca wykonała maksymalną liczbę operacji czyszczenia.
.
Language=Romanian
The cleaner cartridge has performed the maximum number of drive cleanings.
.

MessageId=4334
Severity=Success
Facility=System
SymbolicName=ERROR_UNEXPECTED_OMID
Language=English
Unexpected on-medium identifier.
.
Language=Russian
Unexpected on-medium identifier.
.
Language=Polish
Nieoczekiwany identyfikator nośnika.
.
Language=Romanian
Unexpected on-medium identifier.
.

MessageId=4335
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_DELETE_LAST_ITEM
Language=English
The last remaining item in this group or resource cannot be deleted.
.
Language=Russian
The last remaining item in this group or resource cannot be deleted.
.
Language=Polish
Nie można usunąć ostatniego pozostałego elementu z tej grupy lub nie można usunąć ostatniego pozostałego zasobu.
.
Language=Romanian
The last remaining item in this group or resource cannot be deleted.
.

MessageId=4336
Severity=Success
Facility=System
SymbolicName=ERROR_MESSAGE_EXCEEDS_MAX_SIZE
Language=English
The message provided exceeds the maximum size allowed for this parameter.
.
Language=Russian
The message provided exceeds the maximum size allowed for this parameter.
.
Language=Polish
Dostarczony komunikat przekracza maksymalny rozmiar dozwolony dla tego parametru.
.
Language=Romanian
The message provided exceeds the maximum size allowed for this parameter.
.

MessageId=4337
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_CONTAINS_SYS_FILES
Language=English
The volume contains system or paging files.
.
Language=Russian
The volume contains system or paging files.
.
Language=Polish
Wolumin zawiera pliki systemowe lub pliki stronicowania.
.
Language=Romanian
The volume contains system or paging files.
.

MessageId=4338
Severity=Success
Facility=System
SymbolicName=ERROR_INDIGENOUS_TYPE
Language=English
The media type cannot be removed from this library since at least one drive in the library reports it can support this media type.
.
Language=Russian
The media type cannot be removed from this library since at least one drive in the library reports it can support this media type.
.
Language=Polish
Nie można usunąć tego typu nośnika z tej biblioteki, ponieważ co najmniej jedna stacja z tej biblioteki sygnalizuje, że obsługuje ten typ nośnika.
.
Language=Romanian
The media type cannot be removed from this library since at least one drive in the library reports it can support this media type.
.

MessageId=4339
Severity=Success
Facility=System
SymbolicName=ERROR_NO_SUPPORTING_DRIVES
Language=English
This offline media cannot be mounted on this system since no enabled drives are present which can be used.
.
Language=Russian
This offline media cannot be mounted on this system since no enabled drives are present which can be used.
.
Language=Polish
Nośnik pracujący w trybie offline nie może być zainstalowany w tym systemie, ponieważ nie są włączone żadne stacje, których można by użyć.
.
Language=Romanian
This offline media cannot be mounted on this system since no enabled drives are present which can be used.
.

MessageId=4340
Severity=Success
Facility=System
SymbolicName=ERROR_CLEANER_CARTRIDGE_INSTALLED
Language=English
A cleaner cartridge is present in the tape library.
.
Language=Russian
A cleaner cartridge is present in the tape library.
.
Language=Polish
Kaseta czyszcząca znajduje się w bibliotece taśm.
.
Language=Romanian
A cleaner cartridge is present in the tape library.
.

MessageId=4350
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_OFFLINE
Language=English
The remote storage service was not able to recall the file.
.
Language=Russian
The remote storage service was not able to recall the file.
.
Language=Polish
The remote storage service was not able to recall the file.
.
Language=Romanian
The remote storage service was not able to recall the file.
.

MessageId=4351
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_STORAGE_NOT_ACTIVE
Language=English
The remote storage service is not operational at this time.
.
Language=Russian
The remote storage service is not operational at this time.
.
Language=Polish
Usługa Magazyn zdalny teraz nie działa.
.
Language=Romanian
The remote storage service is not operational at this time.
.

MessageId=4352
Severity=Success
Facility=System
SymbolicName=ERROR_REMOTE_STORAGE_MEDIA_ERROR
Language=English
The remote storage service encountered a media error.
.
Language=Russian
The remote storage service encountered a media error.
.
Language=Polish
Usługa Magazyn zdalny napotkała błąd nośnika.
.
Language=Romanian
The remote storage service encountered a media error.
.

MessageId=4390
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_A_REPARSE_POINT
Language=English
The file or directory is not a reparse point.
.
Language=Russian
The file or directory is not a reparse point.
.
Language=Polish
Plik lub katalog nie jest punktem ponownej analizy.
.
Language=Romanian
The file or directory is not a reparse point.
.

MessageId=4391
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_ATTRIBUTE_CONFLICT
Language=English
The reparse point attribute cannot be set because it conflicts with an existing attribute.
.
Language=Russian
The reparse point attribute cannot be set because it conflicts with an existing attribute.
.
Language=Polish
Nie można ustawić atrybutu punktu ponownej analizy, ponieważ wchodzi w konflikt z istniejącym atrybutem.
.
Language=Romanian
The reparse point attribute cannot be set because it conflicts with an existing attribute.
.

MessageId=4392
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_REPARSE_DATA
Language=English
The data present in the reparse point buffer is invalid.
.
Language=Russian
The data present in the reparse point buffer is invalid.
.
Language=Polish
Dane występujące w buforze punktu ponownej analizy są nieprawidłowe.
.
Language=Romanian
The data present in the reparse point buffer is invalid.
.

MessageId=4393
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_TAG_INVALID
Language=English
The tag present in the reparse point buffer is invalid.
.
Language=Russian
The tag present in the reparse point buffer is invalid.
.
Language=Polish
Etykieta występująca w buforze punktu ponownej analizy jest nieprawidłowa.
.
Language=Romanian
The tag present in the reparse point buffer is invalid.
.

MessageId=4394
Severity=Success
Facility=System
SymbolicName=ERROR_REPARSE_TAG_MISMATCH
Language=English
There is a mismatch between the tag specified in the request and the tag present in the reparse point.
.
Language=Russian
There is a mismatch between the tag specified in the request and the tag present in the reparse point.
.
Language=Polish
Istnieje niezgodność między etykietą podaną w żądaniu, a etykietą występującą w punkcie ponownej analizy.
.
Language=Romanian
There is a mismatch between the tag specified in the request and the tag present in the reparse point.
.

MessageId=4500
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_NOT_SIS_ENABLED
Language=English
Single Instance Storage is not available on this volume.
.
Language=Russian
Single Instance Storage is not available on this volume.
.
Language=Polish
Wolumin ten nie może być woluminem typu SIS (Single Instance Storage).
.
Language=Romanian
Single Instance Storage is not available on this volume.
.

MessageId=5001
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENT_RESOURCE_EXISTS
Language=English
The cluster resource cannot be moved to another group because other resources are dependent on it.
.
Language=Russian
The cluster resource cannot be moved to another group because other resources are dependent on it.
.
Language=Polish
The cluster resource cannot be moved to another group because other resources are dependent on it.
.
Language=Romanian
The cluster resource cannot be moved to another group because other resources are dependent on it.
.

MessageId=5002
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_NOT_FOUND
Language=English
The cluster resource dependency cannot be found.
.
Language=Russian
The cluster resource dependency cannot be found.
.
Language=Polish
Nie można znaleźć zależności zasobów klastra.
.
Language=Romanian
The cluster resource dependency cannot be found.
.

MessageId=5003
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_ALREADY_EXISTS
Language=English
The cluster resource cannot be made dependent on the specified resource because it is already dependent.
.
Language=Russian
The cluster resource cannot be made dependent on the specified resource because it is already dependent.
.
Language=Polish
Nie można uczynić zasobu klastra zależnym od podanego zasobu, ponieważ jest on już zależny.
.
Language=Romanian
The cluster resource cannot be made dependent on the specified resource because it is already dependent.
.

MessageId=5004
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_ONLINE
Language=English
The cluster resource is not online.
.
Language=Russian
The cluster resource is not online.
.
Language=Polish
Zasób klastra nie jest w trybie online.
.
Language=Romanian
The cluster resource is not online.
.

MessageId=5005
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_AVAILABLE
Language=English
A cluster node is not available for this operation.
.
Language=Russian
A cluster node is not available for this operation.
.
Language=Polish
Węzeł klastra nie jest dostępny dla tej operacji.
.
Language=Romanian
A cluster node is not available for this operation.
.

MessageId=5006
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_AVAILABLE
Language=English
The cluster resource is not available.
.
Language=Russian
The cluster resource is not available.
.
Language=Polish
Zasób klastra nie jest dostępny.
.
Language=Romanian
The cluster resource is not available.
.

MessageId=5007
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_NOT_FOUND
Language=English
The cluster resource could not be found.
.
Language=Russian
The cluster resource could not be found.
.
Language=Polish
Nie można znaleźć zasobu klastra.
.
Language=Romanian
The cluster resource could not be found.
.

MessageId=5008
Severity=Success
Facility=System
SymbolicName=ERROR_SHUTDOWN_CLUSTER
Language=English
The cluster is being shut down.
.
Language=Russian
The cluster is being shut down.
.
Language=Polish
Trwa zamykanie klastra.
.
Language=Romanian
The cluster is being shut down.
.

MessageId=5009
Severity=Success
Facility=System
SymbolicName=ERROR_CANT_EVICT_ACTIVE_NODE
Language=English
A cluster node cannot be evicted from the cluster unless the node is down.
.
Language=Russian
A cluster node cannot be evicted from the cluster unless the node is down.
.
Language=Polish
Węzła klastra nie można wykluczyć z klastra, chyba że węzeł nie działa.
.
Language=Romanian
A cluster node cannot be evicted from the cluster unless the node is down.
.

MessageId=5010
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_ALREADY_EXISTS
Language=English
The object already exists.
.
Language=Russian
The object already exists.
.
Language=Polish
Obiekt już istnieje.
.
Language=Romanian
The object already exists.
.

MessageId=5011
Severity=Success
Facility=System
SymbolicName=ERROR_OBJECT_IN_LIST
Language=English
The object is already in the list.
.
Language=Russian
The object is already in the list.
.
Language=Polish
Obiekt już występuje na liście.
.
Language=Romanian
The object is already in the list.
.

MessageId=5012
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_AVAILABLE
Language=English
The cluster group is not available for any new requests.
.
Language=Russian
The cluster group is not available for any new requests.
.
Language=Polish
Grupa klastrów nie jest dostępna dla żadnych nowych żądań.
.
Language=Romanian
The cluster group is not available for any new requests.
.

MessageId=5013
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_FOUND
Language=English
The cluster group could not be found.
.
Language=Russian
The cluster group could not be found.
.
Language=Polish
Nie można znaleźć grupy klastrów.
.
Language=Romanian
The cluster group could not be found.
.

MessageId=5014
Severity=Success
Facility=System
SymbolicName=ERROR_GROUP_NOT_ONLINE
Language=English
The operation could not be completed because the cluster group is not online.
.
Language=Russian
The operation could not be completed because the cluster group is not online.
.
Language=Polish
Nie można ukończyć operacji, ponieważ grupa klastrów nie pracuje w trybie online.
.
Language=Romanian
The operation could not be completed because the cluster group is not online.
.

MessageId=5015
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_RESOURCE_OWNER
Language=English
The cluster node is not the owner of the resource.
.
Language=Russian
The cluster node is not the owner of the resource.
.
Language=Polish
Określony węzeł klastra nie jest właścicielem zasobu.
.
Language=Romanian
The cluster node is not the owner of the resource.
.

MessageId=5016
Severity=Success
Facility=System
SymbolicName=ERROR_HOST_NODE_NOT_GROUP_OWNER
Language=English
The cluster node is not the owner of the group.
.
Language=Russian
The cluster node is not the owner of the group.
.
Language=Polish
Określony węzeł klastra nie jest właścicielem zasobu.
.
Language=Romanian
The cluster node is not the owner of the group.
.

MessageId=5017
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_CREATE_FAILED
Language=English
The cluster resource could not be created in the specified resource monitor.
.
Language=Russian
The cluster resource could not be created in the specified resource monitor.
.
Language=Polish
Nie można utworzyć zasobu klastra za pomocą podanego monitora zasobów.
.
Language=Romanian
The cluster resource could not be created in the specified resource monitor.
.

MessageId=5018
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_ONLINE_FAILED
Language=English
The cluster resource could not be brought online by the resource monitor.
.
Language=Russian
The cluster resource could not be brought online by the resource monitor.
.
Language=Polish
Nie można przełączyć zasobu klastra do trybu online za pomocą monitora zasobów.
.
Language=Romanian
The cluster resource could not be brought online by the resource monitor.
.

MessageId=5019
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_ONLINE
Language=English
The operation could not be completed because the cluster resource is online.
.
Language=Russian
The operation could not be completed because the cluster resource is online.
.
Language=Polish
Nie można ukończyć operacji, ponieważ zasób klastra jest w trybie online.
.
Language=Romanian
The operation could not be completed because the cluster resource is online.
.

MessageId=5020
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_RESOURCE
Language=English
The cluster resource could not be deleted or brought offline because it is the quorum resource.
.
Language=Russian
The cluster resource could not be deleted or brought offline because it is the quorum resource.
.
Language=Polish
Nie można usunąć zasobu klastra ani przełączyć go do trybu offline, ponieważ jest to zasób kworum.
.
Language=Romanian
The cluster resource could not be deleted or brought offline because it is the quorum resource.
.

MessageId=5021
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_QUORUM_CAPABLE
Language=English
The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.
.
Language=Russian
The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.
.
Language=Polish
Klaster nie może uczynić podanego zasobu zasobem kworum, ponieważ nie ma on możliwości bycia nim.
.
Language=Romanian
The cluster could not make the specified resource a quorum resource because it is not capable of being a quorum resource.
.

MessageId=5022
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_SHUTTING_DOWN
Language=English
The cluster software is shutting down.
.
Language=Russian
The cluster software is shutting down.
.
Language=Polish
Trwa zamykanie oprogramowania klastra.
.
Language=Romanian
The cluster software is shutting down.
.

MessageId=5023
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_STATE
Language=English
The group or resource is not in the correct state to perform the requested operation.
.
Language=Russian
The group or resource is not in the correct state to perform the requested operation.
.
Language=Polish
Grupa lub zasób nie jest w odpowiednim stanie, aby można było wykonać żądaną operację.
.
Language=Romanian
The group or resource is not in the correct state to perform the requested operation.
.

MessageId=5024
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_PROPERTIES_STORED
Language=English
The properties were stored but not all changes will take effect until the next time the resource is brought online.
.
Language=Russian
The properties were stored but not all changes will take effect until the next time the resource is brought online.
.
Language=Polish
Właściwości zostały zapisane, lecz niektóre zmiany zostaną wprowadzone dopiero wtedy, gdy zasób zostanie ponownie przełączony do trybu online.
.
Language=Romanian
The properties were stored but not all changes will take effect until the next time the resource is brought online.
.

MessageId=5025
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_QUORUM_CLASS
Language=English
The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.
.
Language=Russian
The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.
.
Language=Polish
Klaster nie może uczynić podanego zasobu zasobem kworum, ponieważ nie należy on do współdzielonej klasy magazynów.
.
Language=Romanian
The cluster could not make the specified resource a quorum resource because it does not belong to a shared storage class.
.

MessageId=5026
Severity=Success
Facility=System
SymbolicName=ERROR_CORE_RESOURCE
Language=English
The cluster resource could not be deleted since it is a core resource.
.
Language=Russian
The cluster resource could not be deleted since it is a core resource.
.
Language=Polish
Nie można usunąć zasobu klastra, ponieważ jest to zasób główny.
.
Language=Romanian
The cluster resource could not be deleted since it is a core resource.
.

MessageId=5027
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_RESOURCE_ONLINE_FAILED
Language=English
The quorum resource failed to come online.
.
Language=Russian
The quorum resource failed to come online.
.
Language=Polish
Nie udało się przełączyć zasobu kworum do trybu online.
.
Language=Romanian
The quorum resource failed to come online.
.

MessageId=5028
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUMLOG_OPEN_FAILED
Language=English
The quorum log could not be created or mounted successfully.
.
Language=Russian
The quorum log could not be created or mounted successfully.
.
Language=Polish
Nie można utworzyć lub pomyślnie zainstalować dziennika kworum.
.
Language=Romanian
The quorum log could not be created or mounted successfully.
.

MessageId=5029
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_CORRUPT
Language=English
The cluster log is corrupt.
.
Language=Russian
The cluster log is corrupt.
.
Language=Polish
Dziennik klastrów jest uszkodzony.
.
Language=Romanian
The cluster log is corrupt.
.

MessageId=5030
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE
Language=English
The record could not be written to the cluster log since it exceeds the maximum size.
.
Language=Russian
The record could not be written to the cluster log since it exceeds the maximum size.
.
Language=Polish
Nie można zapisać rekordu do dziennika klastrów, ponieważ wielkość rekordu przekracza maksymalny rozmiar.
.
Language=Romanian
The record could not be written to the cluster log since it exceeds the maximum size.
.

MessageId=5031
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE
Language=English
The cluster log exceeds its maximum size.
.
Language=Russian
The cluster log exceeds its maximum size.
.
Language=Polish
Dziennik klastrów przekracza maksymalny rozmiar.
.
Language=Romanian
The cluster log exceeds its maximum size.
.

MessageId=5032
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND
Language=English
No checkpoint record was found in the cluster log.
.
Language=Russian
No checkpoint record was found in the cluster log.
.
Language=Polish
W dzienniku klastrów nie znaleziono żadnego rekordu punktu kontrolnego.
.
Language=Romanian
No checkpoint record was found in the cluster log.
.

MessageId=5033
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE
Language=English
The minimum required disk space needed for logging is not available.
.
Language=Russian
The minimum required disk space needed for logging is not available.
.
Language=Polish
Nie jest dostępna minimalna ilość miejsca wymagana do rejestrowania w dzienniku.
.
Language=Romanian
The minimum required disk space needed for logging is not available.
.

MessageId=5034
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_OWNER_ALIVE
Language=English
The cluster node failed to take control of the quorum resource because the resource is owned by another active node.
.
Language=Russian
The cluster node failed to take control of the quorum resource because the resource is owned by another active node.
.
Language=Polish
Węzeł klastra nie może przejąć kontroli nad zasobem kworum, ponieważ zasób ten jest posiadany przez inny aktywny węzeł.
.
Language=Romanian
The cluster node failed to take control of the quorum resource because the resource is owned by another active node.
.

MessageId=5035
Severity=Success
Facility=System
SymbolicName=ERROR_NETWORK_NOT_AVAILABLE
Language=English
A cluster network is not available for this operation.
.
Language=Russian
A cluster network is not available for this operation.
.
Language=Polish
Sieć klastrów nie jest dostępna dla tej operacji.
.
Language=Romanian
A cluster network is not available for this operation.
.

MessageId=5036
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_NOT_AVAILABLE
Language=English
A cluster node is not available for this operation.
.
Language=Russian
A cluster node is not available for this operation.
.
Language=Polish
Węzeł klastra nie jest dostępny dla tej operacji.
.
Language=Romanian
A cluster node is not available for this operation.
.

MessageId=5037
Severity=Success
Facility=System
SymbolicName=ERROR_ALL_NODES_NOT_AVAILABLE
Language=English
All cluster nodes must be running to perform this operation.
.
Language=Russian
All cluster nodes must be running to perform this operation.
.
Language=Polish
Aby można było wykonać tę operację, muszą być uruchomione wszystkie węzły klastra.
.
Language=Romanian
All cluster nodes must be running to perform this operation.
.

MessageId=5038
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_FAILED
Language=English
A cluster resource failed.
.
Language=Russian
A cluster resource failed.
.
Language=Polish
Błąd zasobu klastra.
.
Language=Romanian
A cluster resource failed.
.

MessageId=5039
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NODE
Language=English
The cluster node is not valid.
.
Language=Russian
The cluster node is not valid.
.
Language=Polish
Węzeł klastra jest nieprawidłowy.
.
Language=Romanian
The cluster node is not valid.
.

MessageId=5040
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_EXISTS
Language=English
The cluster node already exists.
.
Language=Russian
The cluster node already exists.
.
Language=Polish
Węzeł klastra już istnieje.
.
Language=Romanian
The cluster node already exists.
.

MessageId=5041
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_IN_PROGRESS
Language=English
A node is in the process of joining the cluster.
.
Language=Russian
A node is in the process of joining the cluster.
.
Language=Polish
Trwa proces przyłączania węzła do klastra.
.
Language=Romanian
A node is in the process of joining the cluster.
.

MessageId=5042
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_FOUND
Language=English
The cluster node was not found.
.
Language=Russian
The cluster node was not found.
.
Language=Polish
Nie znaleziono węzła klastra.
.
Language=Romanian
The cluster node was not found.
.

MessageId=5043
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND
Language=English
The cluster local node information was not found.
.
Language=Russian
The cluster local node information was not found.
.
Language=Polish
Nie znaleziono informacji o lokalnym węźle klastra.
.
Language=Romanian
The cluster local node information was not found.
.

MessageId=5044
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_EXISTS
Language=English
The cluster network already exists.
.
Language=Russian
The cluster network already exists.
.
Language=Polish
Sieć klastrów już istnieje.
.
Language=Romanian
The cluster network already exists.
.

MessageId=5045
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND
Language=English
The cluster network was not found.
.
Language=Russian
The cluster network was not found.
.
Language=Polish
Nie znaleziono sieci klastrów.
.
Language=Romanian
The cluster network was not found.
.

MessageId=5046
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETINTERFACE_EXISTS
Language=English
The cluster network interface already exists.
.
Language=Russian
The cluster network interface already exists.
.
Language=Polish
Interfejs sieci klastrów już istnieje.
.
Language=Romanian
The cluster network interface already exists.
.

MessageId=5047
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETINTERFACE_NOT_FOUND
Language=English
The cluster network interface was not found.
.
Language=Russian
The cluster network interface was not found.
.
Language=Polish
Nie znaleziono interfejsu sieci klastrów.
.
Language=Romanian
The cluster network interface was not found.
.

MessageId=5048
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_REQUEST
Language=English
The cluster request is not valid for this object.
.
Language=Russian
The cluster request is not valid for this object.
.
Language=Polish
Żądanie klastra jest nieprawidłowe w odniesieniu do tego obiektu.
.
Language=Romanian
The cluster request is not valid for this object.
.

MessageId=5049
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NETWORK_PROVIDER
Language=English
The cluster network provider is not valid.
.
Language=Russian
The cluster network provider is not valid.
.
Language=Polish
Dostawca sieci klastrów jest nieprawidłowy.
.
Language=Romanian
The cluster network provider is not valid.
.

MessageId=5050
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_DOWN
Language=English
The cluster node is down.
.
Language=Russian
The cluster node is down.
.
Language=Polish
Węzeł klastra nie działa.
.
Language=Romanian
The cluster node is down.
.

MessageId=5051
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_UNREACHABLE
Language=English
The cluster node is not reachable.
.
Language=Russian
The cluster node is not reachable.
.
Language=Polish
Węzeł klastra jest nieosiągalny.
.
Language=Romanian
The cluster node is not reachable.
.

MessageId=5052
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_MEMBER
Language=English
The cluster node is not a member of the cluster.
.
Language=Russian
The cluster node is not a member of the cluster.
.
Language=Polish
Węzeł klastra nie należy do klastra.
.
Language=Romanian
The cluster node is not a member of the cluster.
.

MessageId=5053
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS
Language=English
A cluster join operation is not in progress.
.
Language=Russian
A cluster join operation is not in progress.
.
Language=Polish
Operacja dołączania klastra nie jest realizowana.
.
Language=Romanian
A cluster join operation is not in progress.
.

MessageId=5054
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INVALID_NETWORK
Language=English
The cluster network is not valid.
.
Language=Russian
The cluster network is not valid.
.
Language=Polish
Sieć klastrów jest nieprawidłowa.
.
Language=Romanian
The cluster network is not valid.
.

MessageId=5056
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_UP
Language=English
The cluster node is up.
.
Language=Russian
The cluster node is up.
.
Language=Polish
Węzeł klastra działa.
.
Language=Romanian
The cluster node is up.
.

MessageId=5057
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_IPADDR_IN_USE
Language=English
The cluster IP address is already in use.
.
Language=Russian
The cluster IP address is already in use.
.
Language=Polish
Adres IP klastra jest już używany.
.
Language=Romanian
The cluster IP address is already in use.
.

MessageId=5058
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_PAUSED
Language=English
The cluster node is not paused.
.
Language=Russian
The cluster node is not paused.
.
Language=Polish
Węzeł klastra nie jest wstrzymany.
.
Language=Romanian
The cluster node is not paused.
.

MessageId=5059
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_SECURITY_CONTEXT
Language=English
No cluster security context is available.
.
Language=Russian
No cluster security context is available.
.
Language=Polish
Nie jest dostępny żaden kontekst zabezpieczenia klastra.
.
Language=Romanian
No cluster security context is available.
.

MessageId=5060
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_INTERNAL
Language=English
The cluster network is not configured for internal cluster communication.
.
Language=Russian
The cluster network is not configured for internal cluster communication.
.
Language=Polish
Sieć klastrów nie jest skonfigurowana tak, aby zapewniała wewnętrzną komunikację w klastrach.
.
Language=Romanian
The cluster network is not configured for internal cluster communication.
.

MessageId=5061
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_UP
Language=English
The cluster node is already up.
.
Language=Russian
The cluster node is already up.
.
Language=Polish
Węzeł klastra już działa.
.
Language=Romanian
The cluster node is already up.
.

MessageId=5062
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_DOWN
Language=English
The cluster node is already down.
.
Language=Russian
The cluster node is already down.
.
Language=Polish
Węzeł klastra już nie działa.
.
Language=Romanian
The cluster node is already down.
.

MessageId=5063
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_ONLINE
Language=English
The cluster network is already online.
.
Language=Russian
The cluster network is already online.
.
Language=Polish
Sieć klastrów już jest w trybie online.
.
Language=Romanian
The cluster network is already online.
.

MessageId=5064
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE
Language=English
The cluster network is already offline.
.
Language=Russian
The cluster network is already offline.
.
Language=Polish
Sieć klastrów już jest w trybie offline.
.
Language=Romanian
The cluster network is already offline.
.

MessageId=5065
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_MEMBER
Language=English
The cluster node is already a member of the cluster.
.
Language=Russian
The cluster node is already a member of the cluster.
.
Language=Polish
Węzeł klastra już należy do klastra.
.
Language=Romanian
The cluster node is already a member of the cluster.
.

MessageId=5066
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_LAST_INTERNAL_NETWORK
Language=English
The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network.
.
Language=Russian
The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network.
.
Language=Polish
Sieć klastrów jest jedyną siecią skonfigurowaną tak, aby zapewniała wewnętrzną komunikację między dwoma (lub więcej) aktywnymi węzłami klastra. Nie można wyłączyć funkcji wewnętrznego komunikowania się w ramach klastra.
.
Language=Romanian
The cluster network is the only one configured for internal cluster communication between two or more active cluster nodes. The internal communication capability cannot be removed from the network.
.

MessageId=5067
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS
Language=English
One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network.
.
Language=Russian
One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network.
.
Language=Polish
Co najmniej jeden z zasobów klastra świadczy usługi klientom poprzez sieć. Nie można usunąć z sieci możliwości dostępu dla klientów.
.
Language=Romanian
One or more cluster resources depend on the network to provide service to clients. The client access capability cannot be removed from the network.
.

MessageId=5068
Severity=Success
Facility=System
SymbolicName=ERROR_INVALID_OPERATION_ON_QUORUM
Language=English
This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list.
.
Language=Russian
This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list.
.
Language=Polish
Tej operacji nie można wykonać na zasobie klastra, ponieważ jest to zasób kworum. Nie można przełączyć zasobu kworum do trybu offline ani modyfikować listy jego właścicieli (jeśli istnieje).
.
Language=Romanian
This operation cannot be performed on the cluster resource as it the quorum resource. You may not bring the quorum resource offline or modify its possible owners list.
.

MessageId=5069
Severity=Success
Facility=System
SymbolicName=ERROR_DEPENDENCY_NOT_ALLOWED
Language=English
The cluster quorum resource is not allowed to have any dependencies.
.
Language=Russian
The cluster quorum resource is not allowed to have any dependencies.
.
Language=Polish
Zasób klastra kworum nie może mieć żadnych zależności.
.
Language=Romanian
The cluster quorum resource is not allowed to have any dependencies.
.

MessageId=5070
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_PAUSED
Language=English
The cluster node is paused.
.
Language=Russian
The cluster node is paused.
.
Language=Polish
Węzeł klastra jest wstrzymany.
.
Language=Romanian
The cluster node is paused.
.

MessageId=5071
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_CANT_HOST_RESOURCE
Language=English
The cluster resource cannot be brought online. The owner node cannot run this resource.
.
Language=Russian
The cluster resource cannot be brought online. The owner node cannot run this resource.
.
Language=Polish
Nie można przełączyć zasobu klastra do trybu online. Węzeł-właściciel nie może uruchomić tego zasobu.
.
Language=Romanian
The cluster resource cannot be brought online. The owner node cannot run this resource.
.

MessageId=5072
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_NOT_READY
Language=English
The cluster node is not ready to perform the requested operation.
.
Language=Russian
The cluster node is not ready to perform the requested operation.
.
Language=Polish
Węzeł klastra nie jest gotowy wykonać żądaną operację.
.
Language=Romanian
The cluster node is not ready to perform the requested operation.
.

MessageId=5073
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_SHUTTING_DOWN
Language=English
The cluster node is shutting down.
.
Language=Russian
The cluster node is shutting down.
.
Language=Polish
Węzeł klastra jest zamykany.
.
Language=Romanian
The cluster node is shutting down.
.

MessageId=5074
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_JOIN_ABORTED
Language=English
The cluster join operation was aborted.
.
Language=Russian
The cluster join operation was aborted.
.
Language=Polish
Operacja łączenia klastra została przerwana.
.
Language=Romanian
The cluster join operation was aborted.
.

MessageId=5075
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INCOMPATIBLE_VERSIONS
Language=English
The cluster join operation failed due to incompatible software versions between the joining node and its sponsor.
.
Language=Russian
The cluster join operation failed due to incompatible software versions between the joining node and its sponsor.
.
Language=Polish
Operacja przyłączenia do klastra nie powiodła się z powodu niezgodności wersji oprogramowania dołączanego węzła i jego sponsora.
.
Language=Romanian
The cluster join operation failed due to incompatible software versions between the joining node and its sponsor.
.

MessageId=5076
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED
Language=English
This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor.
.
Language=Russian
This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor.
.
Language=Polish
Nie można utworzyć tego zasobu, ponieważ klaster osiągnął limit liczby zasobów, które może monitorować.
.
Language=Romanian
This resource cannot be created because the cluster has reached the limit on the number of resources it can monitor.
.

MessageId=5077
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_SYSTEM_CONFIG_CHANGED
Language=English
The system configuration changed during the cluster join or form operation. The join or form operation was aborted.
.
Language=Russian
The system configuration changed during the cluster join or form operation. The join or form operation was aborted.
.
Language=Polish
Konfiguracja systemu zmieniła się podczas wykonywania operacji łączenia lub formowania klastrów. Operacja łączenia lub formowania klastrów została przerwana.
.
Language=Romanian
The system configuration changed during the cluster join or form operation. The join or form operation was aborted.
.

MessageId=5078
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND
Language=English
The specified resource type was not found.
.
Language=Russian
The specified resource type was not found.
.
Language=Polish
Nie znaleziono podanego typu zasobu.
.
Language=Romanian
The specified resource type was not found.
.

MessageId=5079
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESTYPE_NOT_SUPPORTED
Language=English
The specified node does not support a resource of this type. This may be due to version inconsistencies or due to the absence of the resource DLL on this node.
.
Language=Russian
The specified node does not support a resource of this type. This may be due to version inconsistencies or due to the absence of the resource DLL on this node.
.
Language=Polish
Podany węzeł nie obsługuje zasobów tego typu. Może być to spowodowane niezgodnością wersji lub nieobecnością biblioteki DLL zasobów w tym węźle.
.
Language=Romanian
The specified node does not support a resource of this type. This may be due to version inconsistencies or due to the absence of the resource DLL on this node.
.

MessageId=5080
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_RESNAME_NOT_FOUND
Language=English
The specified resource name is supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL.
.
Language=Russian
The specified resource name is supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL.
.
Language=Polish
Podana nazwa zasobu nie jest obsługiwana przez tę bibliotekę DLL. Może to być spowodowane złą (lub zmienioną) nazwą dostarczoną do biblioteki DLL zasobów.
.
Language=Romanian
The specified resource name is supported by this resource DLL. This may be due to a bad (or changed) name supplied to the resource DLL.
.

MessageId=5081
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED
Language=English
No authentication package could be registered with the RPC server.
.
Language=Russian
No authentication package could be registered with the RPC server.
.
Language=Polish
Na serwerze RPC nie można zarejestrować żadnego pakietu uwierzytelnień.
.
Language=Romanian
No authentication package could be registered with the RPC server.
.

MessageId=5082
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_OWNER_NOT_IN_PREFLIST
Language=English
You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group.
.
Language=Russian
You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group.
.
Language=Polish
Nie można przełączyć grupy do trybu online, ponieważ właściciel grupy nie występuje na liście preferowanych przez grupę. Aby zmienić węzeł będący właścicielem grupy, przenieś grupę.
.
Language=Romanian
You cannot bring the group online because the owner of the group is not in the preferred list for the group. To change the owner node for the group, move the group.
.

MessageId=5083
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_DATABASE_SEQMISMATCH
Language=English
The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join.
.
Language=Russian
The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join.
.
Language=Polish
Operacja łączenia nie powiodła się, ponieważ numer sekwencyjny bazy danych klastra zmienił się lub jest niezgodny z węzłem blokującym. Może się to zdarzyć podczas operacji łączenia, jeżeli w jej trakcie ulegnie zmianie baza danych klastra.
.
Language=Romanian
The join operation failed because the cluster database sequence number has changed or is incompatible with the locker node. This may happen during a join operation if the cluster database was changing during the join.
.

MessageId=5084
Severity=Success
Facility=System
SymbolicName=ERROR_RESMON_INVALID_STATE
Language=English
The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state.
.
Language=Russian
The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state.
.
Language=Polish
Monitorów zasobów nie pozwoli na wykonanie błędnej operacji, gdy zasób jest w swoim bieżącym stanie. Może się to zdarzyć, jeśli zasób jest w stanie oczekiwania.
.
Language=Romanian
The resource monitor will not allow the fail operation to be performed while the resource is in its current state. This may happen if the resource is in a pending state.
.

MessageId=5085
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_GUM_NOT_LOCKER
Language=English
A non locker code got a request to reserve the lock for making global updates.
.
Language=Russian
A non locker code got a request to reserve the lock for making global updates.
.
Language=Polish
Kod nieblokujący odebrał żądanie zarezerwowania tej blokady w celu przeprowadzenia globalnej aktualizacji.
.
Language=Romanian
A non locker code got a request to reserve the lock for making global updates.
.

MessageId=5086
Severity=Success
Facility=System
SymbolicName=ERROR_QUORUM_DISK_NOT_FOUND
Language=English
The quorum disk could not be located by the cluster service.
.
Language=Russian
The quorum disk could not be located by the cluster service.
.
Language=Polish
Usługa klastrowania nie może zlokalizować dysku kworum.
.
Language=Romanian
The quorum disk could not be located by the cluster service.
.

MessageId=5087
Severity=Success
Facility=System
SymbolicName=ERROR_DATABASE_BACKUP_CORRUPT
Language=English
The backup up cluster database is possibly corrupt.
.
Language=Russian
The backup up cluster database is possibly corrupt.
.
Language=Polish
Kopia zapasowa bazy danych klastra prawdopodobnie jest uszkodzona.
.
Language=Romanian
The backup up cluster database is possibly corrupt.
.

MessageId=5088
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT
Language=English
A DFS root already exists in this cluster node.
.
Language=Russian
A DFS root already exists in this cluster node.
.
Language=Polish
W tym węźle klastra już występuje system DFS.
.
Language=Romanian
A DFS root already exists in this cluster node.
.

MessageId=5089
Severity=Success
Facility=System
SymbolicName=ERROR_RESOURCE_PROPERTY_UNCHANGEABLE
Language=English
An attempt to modify a resource property failed because it conflicts with another existing property.
.
Language=Russian
An attempt to modify a resource property failed because it conflicts with another existing property.
.
Language=Polish
Nie można zmodyfikować właściwości zasobu, ponieważ nowa właściwość wchodzi w konflikt z inną istniejącą właściwością.
.
Language=Romanian
An attempt to modify a resource property failed because it conflicts with another existing property.
.

MessageId=5890
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE
Language=English
An operation was attempted that is incompatible with the current membership state of the node.
.
Language=Russian
An operation was attempted that is incompatible with the current membership state of the node.
.
Language=Polish
Próbowano wykonać operację, która jest niezgodna z bieżącym stanem członkostwa węzła.
.
Language=Romanian
An operation was attempted that is incompatible with the current membership state of the node.
.

MessageId=5891
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_QUORUMLOG_NOT_FOUND
Language=English
The quorum resource does not contain the quorum log.
.
Language=Russian
The quorum resource does not contain the quorum log.
.
Language=Polish
Zasób kworum nie zawiera dziennika kworum.
.
Language=Romanian
The quorum resource does not contain the quorum log.
.

MessageId=5892
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MEMBERSHIP_HALT
Language=English
The membership engine requested shutdown of the cluster service on this node.
.
Language=Russian
The membership engine requested shutdown of the cluster service on this node.
.
Language=Polish
Aparat członkostwa zażądał zamknięcia usługi klastrowania na tym węźle.
.
Language=Romanian
The membership engine requested shutdown of the cluster service on this node.
.

MessageId=5893
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_INSTANCE_ID_MISMATCH
Language=English
The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node.
.
Language=Russian
The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node.
.
Language=Polish
Operacja dołączania nie powiodła się, ponieważ identyfikator wystąpienia klastra węzła dołączającego nie pasuje do identyfikatora wystąpienia klastra węzła sponsorującego.
.
Language=Romanian
The join operation failed because the cluster instance ID of the joining node does not match the cluster instance ID of the sponsor node.
.

MessageId=5894
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP
Language=English
A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.
Language=Russian
A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.
Language=Polish
A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.
Language=Romanian
A matching network for the specified IP address could not be found. Please also specify a subnet mask and a cluster network.
.

MessageId=5895
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_PROPERTY_DATA_TYPE_MISMATCH
Language=English
The actual data type of the property did not match the expected data type of the property.
.
Language=Russian
The actual data type of the property did not match the expected data type of the property.
.
Language=Polish
Rzeczywisty typ danych właściwości nie odpowiada oczekiwanemu typowi danych właściwości.
.
Language=Romanian
The actual data type of the property did not match the expected data type of the property.
.

MessageId=5896
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP
Language=English
The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available.
.
Language=Russian
The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available.
.
Language=Polish
The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available.
.
Language=Romanian
The cluster node was evicted from the cluster successfully, but the node was not cleaned up. Extended status information explaining why the node was not cleaned up is available.
.

MessageId=5897
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_PARAMETER_MISMATCH
Language=English
Two or more parameter values specified for a resource's properties are in conflict.
.
Language=Russian
Two or more parameter values specified for a resource's properties are in conflict.
.
Language=Polish
Dwie lub więcej wartości parametrów, określonych dla właściwości zasobu, kolidują ze sobą.
.
Language=Romanian
Two or more parameter values specified for a resource's properties are in conflict.
.

MessageId=5898
Severity=Success
Facility=System
SymbolicName=ERROR_NODE_CANNOT_BE_CLUSTERED
Language=English
This computer cannot be made a member of a cluster.
.
Language=Russian
This computer cannot be made a member of a cluster.
.
Language=Polish
Ten komputer nie może być członkiem klastra.
.
Language=Romanian
This computer cannot be made a member of a cluster.
.

MessageId=5899
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_WRONG_OS_VERSION
Language=English
This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed.
.
Language=Russian
This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed.
.
Language=Polish
Ten komputer nie może być członkiem klastra, ponieważ nie ma na nim zainstalowanej właściwej wersji systemu Windows.
.
Language=Romanian
This computer cannot be made a member of a cluster because it does not have the correct version of Windows installed.
.

MessageId=5900
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_CANT_CREATE_DUP_CLUSTER_NAME
Language=English
A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster.
.
Language=Russian
A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster.
.
Language=Polish
Nie można utworzyć klastra o określonej nazwie, ponieważ ta nazwa klastra jest już używana. Określ inną nazwę dla klastra.
.
Language=Romanian
A cluster cannot be created with the specified cluster name because that cluster name is already in use. Specify a different name for the cluster.
.

MessageId=5901
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_ALREADY_COMMITTED
Language=English
The cluster configuration action has already been committed.
.
Language=Russian
The cluster configuration action has already been committed.
.
Language=Polish
Akcja konfiguracji klastra została już wykonana.
.
Language=Romanian
The cluster configuration action has already been committed.
.

MessageId=5902
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_ROLLBACK_FAILED
Language=English
The cluster configuration action could not be rolled back.
.
Language=Russian
The cluster configuration action could not be rolled back.
.
Language=Polish
Nie można wycofać akcji konfiguracji klastra.
.
Language=Romanian
The cluster configuration action could not be rolled back.
.

MessageId=5903
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSCFG_SYSTEM_DISK_DRIVE_LETTER_CONFLICT
Language=English
The drive letter assigned to a system disk on one node conflicted with the driver letter assigned to a disk on another node.
.
Language=Russian
The drive letter assigned to a system disk on one node conflicted with the driver letter assigned to a disk on another node.
.
Language=Polish
Litera dysku przypisana do dysku systemowego w jednym z węzłów jest w konflikcie z literą dysku przypisaną do dysku w innym węźle.
.
Language=Romanian
The drive letter assigned to a system disk on one node conflicted with the driver letter assigned to a disk on another node.
.

MessageId=5904
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_OLD_VERSION
Language=English
One or more nodes in the cluster are running a version of Windows that does not support this operation.
.
Language=Russian
One or more nodes in the cluster are running a version of Windows that does not support this operation.
.
Language=Polish
Jeden lub kilka węzłów w klastrze korzysta z wersji systemu Windows, która nie obsługuje tej operacji.
.
Language=Romanian
One or more nodes in the cluster are running a version of Windows that does not support this operation.
.

MessageId=5905
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME
Language=English
The name of the corresponding computer account doesn't match the Network Name for this resource.
.
Language=Russian
The name of the corresponding computer account doesn't match the Network Name for this resource.
.
Language=Polish
Nazwa odpowiedniego konta komputera nie pasuje do nazwy sieciowej tego zasobu.
.
Language=Romanian
The name of the corresponding computer account doesn't match the Network Name for this resource.
.

MessageId=5906
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_NO_NET_ADAPTERS
Language=English
No network adapters are available.
.
Language=Russian
No network adapters are available.
.
Language=Polish
Nie są dostępne żadne karty sieciowe.
.
Language=Romanian
No network adapters are available.
.

MessageId=5907
Severity=Success
Facility=System
SymbolicName=ERROR_CLUSTER_POISONED
Language=English
The cluster node has been poisoned.
.
Language=Russian
The cluster node has been poisoned.
.
Language=Polish
Węzeł klastra uległ awarii.
.
Language=Romanian
The cluster node has been poisoned.
.

MessageId=6000
Severity=Success
Facility=System
SymbolicName=ERROR_ENCRYPTION_FAILED
Language=English
The specified file could not be encrypted.
.
Language=Russian
The specified file could not be encrypted.
.
Language=Polish
Nie można zaszyfrować podanego pliku.
.
Language=Romanian
The specified file could not be encrypted.
.

MessageId=6001
Severity=Success
Facility=System
SymbolicName=ERROR_DECRYPTION_FAILED
Language=English
The specified file could not be decrypted.
.
Language=Russian
The specified file could not be decrypted.
.
Language=Polish
Nie można odszyfrować podanego pliku.
.
Language=Romanian
The specified file could not be decrypted.
.

MessageId=6002
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_ENCRYPTED
Language=English
The specified file is encrypted and the user does not have the ability to decrypt it.
.
Language=Russian
The specified file is encrypted and the user does not have the ability to decrypt it.
.
Language=Polish
Określony plik jest zaszyfrowany i użytkownik nie ma możliwości odszyfrowania go.
.
Language=Romanian
The specified file is encrypted and the user does not have the ability to decrypt it.
.

MessageId=6003
Severity=Success
Facility=System
SymbolicName=ERROR_NO_RECOVERY_POLICY
Language=English
There is no valid encryption recovery policy configured for this system.
.
Language=Russian
There is no valid encryption recovery policy configured for this system.
.
Language=Polish
Brak skonfigurowanych prawidłowych zasad odzyskiwania szyfrowania dla tego systemu.
.
Language=Romanian
There is no valid encryption recovery policy configured for this system.
.

MessageId=6004
Severity=Success
Facility=System
SymbolicName=ERROR_NO_EFS
Language=English
The required encryption driver is not loaded for this system.
.
Language=Russian
The required encryption driver is not loaded for this system.
.
Language=Polish
Wymagany sterownik szyfrowania nie jest załadowany w systemie.
.
Language=Romanian
The required encryption driver is not loaded for this system.
.

MessageId=6005
Severity=Success
Facility=System
SymbolicName=ERROR_WRONG_EFS
Language=English
The file was encrypted with a different encryption driver than is currently loaded.
.
Language=Russian
The file was encrypted with a different encryption driver than is currently loaded.
.
Language=Polish
Plik został zaszyfrowany za pomocą sterownika szyfrowania innego niż obecnie załadowany.
.
Language=Romanian
The file was encrypted with a different encryption driver than is currently loaded.
.

MessageId=6006
Severity=Success
Facility=System
SymbolicName=ERROR_NO_USER_KEYS
Language=English
There are no EFS keys defined for the user.
.
Language=Russian
There are no EFS keys defined for the user.
.
Language=Polish
Brak zdefiniowanych kluczy EFS dla użytkownika.
.
Language=Romanian
There are no EFS keys defined for the user.
.

MessageId=6007
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_NOT_ENCRYPTED
Language=English
The specified file is not encrypted.
.
Language=Russian
The specified file is not encrypted.
.
Language=Polish
Określony plik nie jest zaszyfrowany.
.
Language=Romanian
The specified file is not encrypted.
.

MessageId=6008
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_EXPORT_FORMAT
Language=English
The specified file is not in the defined EFS export format.
.
Language=Russian
The specified file is not in the defined EFS export format.
.
Language=Polish
Określony plik nie występuje w zdefiniowanym formacie eksportu EFS.
.
Language=Romanian
The specified file is not in the defined EFS export format.
.

MessageId=6009
Severity=Success
Facility=System
SymbolicName=ERROR_FILE_READ_ONLY
Language=English
The specified file is read only.
.
Language=Russian
The specified file is read only.
.
Language=Polish
Podany plik jest tylko do odczytu.
.
Language=Romanian
The specified file is read only.
.

MessageId=6010
Severity=Success
Facility=System
SymbolicName=ERROR_DIR_EFS_DISALLOWED
Language=English
The directory has been disabled for encryption.
.
Language=Russian
The directory has been disabled for encryption.
.
Language=Polish
Katalog został wyłączony z szyfrowania.
.
Language=Romanian
The directory has been disabled for encryption.
.

MessageId=6011
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_SERVER_NOT_TRUSTED
Language=English
The server is not trusted for remote encryption operation.
.
Language=Russian
The server is not trusted for remote encryption operation.
.
Language=Polish
Serwer nie jest zaufany dla zdalnej operacji szyfrowania.
.
Language=Romanian
The server is not trusted for remote encryption operation.
.

MessageId=6012
Severity=Success
Facility=System
SymbolicName=ERROR_BAD_RECOVERY_POLICY
Language=English
Recovery policy configured for this system contains invalid recovery certificate.
.
Language=Russian
Recovery policy configured for this system contains invalid recovery certificate.
.
Language=Polish
Zasady odzyskiwania skonfigurowane dla tego systemu zawierają nieprawidłowy certyfikat odzyskiwania.
.
Language=Romanian
Recovery policy configured for this system contains invalid recovery certificate.
.

MessageId=6013
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_ALG_BLOB_TOO_BIG
Language=English
The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file.
.
Language=Russian
The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file.
.
Language=Polish
Algorytm szyfrowania użyty dla pliku źródłowego wymaga większego buforu klucza niż określony dla pliku docelowego.
.
Language=Romanian
The encryption algorithm used on the source file needs a bigger key buffer than the one on the destination file.
.

MessageId=6014
Severity=Success
Facility=System
SymbolicName=ERROR_VOLUME_NOT_SUPPORT_EFS
Language=English
The disk partition does not support file encryption.
.
Language=Russian
The disk partition does not support file encryption.
.
Language=Polish
Dana partycja dysku nie obsługuje szyfrowania plików.
.
Language=Romanian
The disk partition does not support file encryption.
.

MessageId=6015
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_DISABLED
Language=English
This machine is disabled for file encryption.
.
Language=Russian
This machine is disabled for file encryption.
.
Language=Polish
Dla tego komputera szyfrowanie plików jest wyłączone.
.
Language=Romanian
This machine is disabled for file encryption.
.

MessageId=6016
Severity=Success
Facility=System
SymbolicName=ERROR_EFS_VERSION_NOT_SUPPORT
Language=English
A newer system is required to decrypt this encrypted file.
.
Language=Russian
A newer system is required to decrypt this encrypted file.
.
Language=Polish
Do odszyfrowania tego pliku zaszyfrowanego potrzebny jest nowszy system.
.
Language=Romanian
A newer system is required to decrypt this encrypted file.
.

MessageId=6118
Severity=Success
Facility=System
SymbolicName=ERROR_NO_BROWSER_SERVERS_FOUND
Language=English
The list of servers for this workgroup is not currently available.
.
Language=Russian
The list of servers for this workgroup is not currently available.
.
Language=Polish
Lista serwerów dla tej grupy roboczej jest obecnie niedostępna.
.
Language=Romanian
The list of servers for this workgroup is not currently available.
.

MessageId=6200
Severity=Success
Facility=System
SymbolicName=SCHED_E_SERVICE_NOT_LOCALSYSTEM
Language=English
The Task Scheduler service must be configured to run in the System account to function properly. Individual tasks may be configured to run in other accounts.
.
Language=Russian
The Task Scheduler service must be configured to run in the System account to function properly. Individual tasks may be configured to run in other accounts.
.
Language=Polish
Usługa harmonogramu zadań musi być skonfiguorawana do uruchomienia w systemie konta, aby funkcjonować poprawnie. Indywidualne zadania mogą być skierowane do uruchamiania w innych kontach.
.
Language=Romanian
The Task Scheduler service must be configured to run in the System account to function properly. Individual tasks may be configured to run in other accounts.
.

MessageId=7001
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_NAME_INVALID
Language=English
The specified session name is invalid.
.
Language=Russian
The specified session name is invalid.
.
Language=Polish
Podana nazwa sesji jest nieprawidłowa.
.
Language=Romanian
The specified session name is invalid.
.

MessageId=7002
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_PD
Language=English
The specified protocol driver is invalid.
.
Language=Russian
The specified protocol driver is invalid.
.
Language=Polish
Podany sterownik protokołu jest nieprawidłowy.
.
Language=Romanian
The specified protocol driver is invalid.
.

MessageId=7003
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_PD_NOT_FOUND
Language=English
The specified protocol driver was not found in the system path.
.
Language=Russian
The specified protocol driver was not found in the system path.
.
Language=Polish
Podany sterownik protokołu nie został znaleziony w ścieżce systemu.
.
Language=Romanian
The specified protocol driver was not found in the system path.
.

MessageId=7004
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WD_NOT_FOUND
Language=English
The specified terminal connection driver was not found in the system path.
.
Language=Russian
The specified terminal connection driver was not found in the system path.
.
Language=Polish
Podany sterownik połączenia terminali nie został znaleziony w ścieżce systemu.
.
Language=Romanian
The specified terminal connection driver was not found in the system path.
.

MessageId=7005
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CANNOT_MAKE_EVENTLOG_ENTRY
Language=English
A registry key for event logging could not be created for this session.
.
Language=Russian
A registry key for event logging could not be created for this session.
.
Language=Polish
Dla tej sesji nie można utworzyć klucza Rejestru obejmującego rejestrowanie w dzienniku zdarzeń.
.
Language=Romanian
A registry key for event logging could not be created for this session.
.

MessageId=7006
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SERVICE_NAME_COLLISION
Language=English
A service with the same name already exists on the system.
.
Language=Russian
A service with the same name already exists on the system.
.
Language=Polish
Usługa o tej samej nazwie już istnieje w systemie.
.
Language=Romanian
A service with the same name already exists on the system.
.

MessageId=7007
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLOSE_PENDING
Language=English
A close operation is pending on the session.
.
Language=Russian
A close operation is pending on the session.
.
Language=Polish
Operacja zamykania czeka na wykonanie (w tej sesji).
.
Language=Romanian
A close operation is pending on the session.
.

MessageId=7008
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NO_OUTBUF
Language=English
There are no free output buffers available.
.
Language=Russian
There are no free output buffers available.
.
Language=Polish
Brak dostępnych wolnych buforów wyjściowych.
.
Language=Romanian
There are no free output buffers available.
.

MessageId=7009
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_INF_NOT_FOUND
Language=English
The MODEM.INF file was not found.
.
Language=Russian
The MODEM.INF file was not found.
.
Language=Polish
Nie znaleziono pliku MODEM.INF.
.
Language=Romanian
The MODEM.INF file was not found.
.

MessageId=7010
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_MODEMNAME
Language=English
The modem name was not found in MODEM.INF.
.
Language=Russian
The modem name was not found in MODEM.INF.
.
Language=Polish
Nazwa modemu nie została znaleziona w pliku MODEM.INF.
.
Language=Romanian
The modem name was not found in MODEM.INF.
.

MessageId=7011
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_ERROR
Language=English
The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem.
.
Language=Russian
The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem.
.
Language=Polish
Modem nie akceptuje wysłanego do niego polecenia. Upewnij się, czy nazwa skonfigurowanego modemu jest zgodna z nazwą dołączonego modemu.
.
Language=Romanian
The modem did not accept the command sent to it. Verify that the configured modem name matches the attached modem.
.

MessageId=7012
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_TIMEOUT
Language=English
The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on.
.
Language=Russian
The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on.
.
Language=Polish
Modem nie odpowiada na wysłane do niego polecenie. Upewnij się, czy modem jest właściwie połączony i czy jest zasilany.
.
Language=Romanian
The modem did not respond to the command sent to it. Verify that the modem is properly cabled and powered on.
.

MessageId=7013
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_CARRIER
Language=English
Carrier detect has failed or carrier has been dropped due to disconnect.
.
Language=Russian
Carrier detect has failed or carrier has been dropped due to disconnect.
.
Language=Polish
Nie można wykryć nośnej lub nośna została utracona z powodu rozłączenia.
.
Language=Romanian
Carrier detect has failed or carrier has been dropped due to disconnect.
.

MessageId=7014
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE
Language=English
Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional.
.
Language=Russian
Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional.
.
Language=Polish
W wymaganym czasie nie wykryto sygnału wybierania. Sprawdź, czy kabel telefoniczny jest poprawnie podłączony i czy nie jest uszkodzony.
.
Language=Romanian
Dial tone not detected within the required time. Verify that the phone cable is properly attached and functional.
.

MessageId=7015
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_BUSY
Language=English
Busy signal detected at remote site on callback.
.
Language=Russian
Busy signal detected at remote site on callback.
.
Language=Polish
Podczas oddzwaniania, w lokacji zdalnej wykryto sygnał zajętości.
.
Language=Romanian
Busy signal detected at remote site on callback.
.

MessageId=7016
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_MODEM_RESPONSE_VOICE
Language=English
Voice detected at remote site on callback.
.
Language=Russian
Voice detected at remote site on callback.
.
Language=Polish
Podczas oddzwaniania, w lokacji zdalnej wykryto głos.
.
Language=Romanian
Voice detected at remote site on callback.
.

MessageId=7017
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_TD_ERROR
Language=English
Transport driver error
.
Language=Russian
Transport driver error
.
Language=Polish
Błąd sterownika transportu.
.
Language=Romanian
Transport driver error
.

MessageId=7022
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_NOT_FOUND
Language=English
The specified session cannot be found.
.
Language=Russian
The specified session cannot be found.
.
Language=Polish
Nie można odnaleźć określonej sesji.
.
Language=Romanian
The specified session cannot be found.
.

MessageId=7023
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_ALREADY_EXISTS
Language=English
The specified session name is already in use.
.
Language=Russian
The specified session name is already in use.
.
Language=Polish
Określona nazwa sesji jest już używana.
.
Language=Romanian
The specified session name is already in use.
.

MessageId=7024
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_BUSY
Language=English
The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation.
.
Language=Russian
The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation.
.
Language=Polish
Nie można ukończyć żądanej operacji, ponieważ połączenie Terminala jest zajęte przetwarzaniem operacji łączenia, rozłączania, resetowania lub usuwania.
.
Language=Romanian
The requested operation cannot be completed because the terminal connection is currently busy processing a connect, disconnect, reset, or delete operation.
.

MessageId=7025
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_BAD_VIDEO_MODE
Language=English
An attempt has been made to connect to a session whose video mode is not supported by the current client.
.
Language=Russian
An attempt has been made to connect to a session whose video mode is not supported by the current client.
.
Language=Polish
Podjęto próbę połączenia z sesją, której tryb wideo nie jest obsługiwany przez bieżącego klienta.
.
Language=Romanian
An attempt has been made to connect to a session whose video mode is not supported by the current client.
.

MessageId=7035
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_GRAPHICS_INVALID
Language=English
The application attempted to enable DOS graphics mode. DOS graphics mode is not supported.
.
Language=Russian
The application attempted to enable DOS graphics mode. DOS graphics mode is not supported.
.
Language=Polish
Aplikacja próbowała włączyć tryb graficzny DOS. Tryb graficzny DOS nie jest obsługiwany.
.
Language=Romanian
The application attempted to enable DOS graphics mode. DOS graphics mode is not supported.
.

MessageId=7037
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LOGON_DISABLED
Language=English
Your interactive logon privilege has been disabled. Please contact your administrator.
.
Language=Russian
Your interactive logon privilege has been disabled. Please contact your administrator.
.
Language=Polish
Twoje uprawnienie do logowania interakcyjnego zostało wyłączone. Skontaktuj się z administratorem systemu.
.
Language=Romanian
Your interactive logon privilege has been disabled. Please contact your administrator.
.

MessageId=7038
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NOT_CONSOLE
Language=English
The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access.
.
Language=Russian
The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access.
.
Language=Polish
Żądana operacja może być wykonana jedynie za pomocą konsoli systemu. Jest to spowodowane najczęściej tym, że sterownik lub biblioteka DLL wymaga bezpośredniego dostępu do konsoli.
.
Language=Romanian
The requested operation can be performed only on the system console. This is most often the result of a driver or system DLL requiring direct console access.
.

MessageId=7040
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_QUERY_TIMEOUT
Language=English
The client failed to respond to the server connect message.
.
Language=Russian
The client failed to respond to the server connect message.
.
Language=Polish
Klient nie odpowiada na komunikat połączenia wysłany przez serwer.
.
Language=Romanian
The client failed to respond to the server connect message.
.

MessageId=7041
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CONSOLE_DISCONNECT
Language=English
Disconnecting the console session is not supported.
.
Language=Russian
Disconnecting the console session is not supported.
.
Language=Polish
Odłączanie sesji konsoli nie jest obsługiwane.
.
Language=Romanian
Disconnecting the console session is not supported.
.

MessageId=7042
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CONSOLE_CONNECT
Language=English
Reconnecting a disconnected session to the console is not supported.
.
Language=Russian
Reconnecting a disconnected session to the console is not supported.
.
Language=Polish
Ponowne podłączanie odłączonej sesji do konsoli nie jest obsługiwane.
.
Language=Romanian
Reconnecting a disconnected session to the console is not supported.
.

MessageId=7044
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_DENIED
Language=English
The request to control another session remotely was denied.
.
Language=Russian
The request to control another session remotely was denied.
.
Language=Polish
Żądanie kontrolowania zdalnego jeszcze jednej sesji zostało odrzucone.
.
Language=Romanian
The request to control another session remotely was denied.
.

MessageId=7045
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATION_ACCESS_DENIED
Language=English
The requested session access is denied.
.
Language=Russian
The requested session access is denied.
.
Language=Polish
Odmowa dostępu do żądanej sesji.
.
Language=Romanian
The requested session access is denied.
.

MessageId=7049
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_INVALID_WD
Language=English
The specified terminal connection driver is invalid.
.
Language=Russian
The specified terminal connection driver is invalid.
.
Language=Polish
Podany sterownik połączenia terminali jest nieprawidłowy.
.
Language=Romanian
The specified terminal connection driver is invalid.
.

MessageId=7050
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_INVALID
Language=English
The requested session cannot be controlled remotely. This may be because the session is disconnected or does not currently have a user logged on.
.
Language=Russian
The requested session cannot be controlled remotely. This may be because the session is disconnected or does not currently have a user logged on.
.
Language=Polish
Żądana sesja nie może być kontrolowana zdalnie. Może się to zdarzyć, ponieważ sesja jest odłączona lub użytkownik nie jest zalogowany.
.
Language=Romanian
The requested session cannot be controlled remotely. This may be because the session is disconnected or does not currently have a user logged on.
.

MessageId=7051
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_DISABLED
Language=English
The requested session is not configured to allow remote control.
.
Language=Russian
The requested session is not configured to allow remote control.
.
Language=Polish
Żądana sesja nie jest skonfigurowana tak, aby umożliwiać kontrolę zdalną.
.
Language=Romanian
The requested session is not configured to allow remote control.
.

MessageId=7052
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_LICENSE_IN_USE
Language=English
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user. Please call your system administrator to obtain a unique license number.
.
Language=Russian
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user. Please call your system administrator to obtain a unique license number.
.
Language=Polish
Żądanie połączenia z tym serwerem terminali zostało odrzucone. Numer licencji Twojego klienta serwera terminali jest obecnie używany przez innego użytkownika. Skontaktuj się z administratorem systemu, aby uzyskać unikatowy numer licencji.
.
Language=Romanian
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number is currently being used by another user. Please call your system administrator to obtain a unique license number.
.

MessageId=7053
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CLIENT_LICENSE_NOT_SET
Language=English
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client. Please contact your system administrator.
.
Language=Russian
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client. Please contact your system administrator.
.
Language=Polish
Żądanie połączenia z tym serwerem terminali zostało odrzucone. Numer licencji klienta serwera terminali nie został wprowadzony dla tej kopii klienta serwera terminali. Skontaktuj się z administratorem systemu.
.
Language=Romanian
Your request to connect to this Terminal Server has been rejected. Your Terminal Server client license number has not been entered for this copy of the Terminal Server client. Please contact your system administrator.
.

MessageId=7054
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_NOT_AVAILABLE
Language=English
The system has reached its licensed logon limit. Please try again later.
.
Language=Russian
The system has reached its licensed logon limit. Please try again later.
.
Language=Polish
System osiągnął ograniczony licencją limit logowania. Spróbuj ponownie później.
.
Language=Romanian
The system has reached its licensed logon limit. Please try again later.
.

MessageId=7055
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_CLIENT_INVALID
Language=English
The client you are using is not licensed to use this system. Your logon request is denied.
.
Language=Russian
The client you are using is not licensed to use this system. Your logon request is denied.
.
Language=Polish
Klient, którego używasz, nie ma licencji na używanie systemu. Żądanie zalogowania zostało odrzucone.
.
Language=Romanian
The client you are using is not licensed to use this system. Your logon request is denied.
.

MessageId=7056
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_LICENSE_EXPIRED
Language=English
The system license has expired. Your logon request is denied.
.
Language=Russian
The system license has expired. Your logon request is denied.
.
Language=Polish
Licencja na używanie systemu wygasła. Żądanie zalogowania zostało odrzucone.
.
Language=Romanian
The system license has expired. Your logon request is denied.
.

MessageId=7057
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_NOT_RUNNING
Language=English
Remote control could not be terminated because the specified session is not currently being remotely controlled.
.
Language=Russian
Remote control could not be terminated because the specified session is not currently being remotely controlled.
.
Language=Polish
Nie można przerwać zdalnego sterowania, ponieważ określona sesja nie jest sterowana zdalnie.
.
Language=Romanian
Remote control could not be terminated because the specified session is not currently being remotely controlled.
.

MessageId=7058
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE
Language=English
The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.
.
Language=Russian
The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.
.
Language=Polish
Zdalne sterowanie konsolą zostało przerwane z powodu zmiany trybu wyświetlania. Zmiana trybu wyświetlania w trakcie sesji zdalnego sterowania jest nieobsługiwana.
.
Language=Romanian
The remote control of the console was terminated because the display mode was changed. Changing the display mode in a remote control session is not supported.
.

MessageId=7059
Severity=Success
Facility=System
SymbolicName=ERROR_ACTIVATION_COUNT_EXCEEDED
Language=English
Activation has already been reset the maximum number of times for this installation. Your activation timer will not be cleared.
.
Language=Russian
Activation has already been reset the maximum number of times for this installation. Your activation timer will not be cleared.
.
Language=Polish
Aktywacja została zresetowana maksymalną liczbę razy na tę instalację. Czasomierz aktywacji nie zostanie wyczyszczony.
.
Language=Romanian
Activation has already been reset the maximum number of times for this installation. Your activation timer will not be cleared.
.

MessageId=7060
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_WINSTATIONS_DISABLED
Language=English
Remote logins are currently disabled.
.
Language=Russian
Remote logins are currently disabled.
.
Language=Polish
Logowania zdalne są w tej chwili wyłączone.
.
Language=Romanian
Remote logins are currently disabled.
.

MessageId=7061
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_ENCRYPTION_LEVEL_REQUIRED
Language=English
You do not have the proper encryption level to access this Session.
.
Language=Russian
You do not have the proper encryption level to access this Session.
.
Language=Polish
Nie masz odpowiedniego poziomu szyfrowania, aby uzyskać dostęp do tej sesji.
.
Language=Romanian
You do not have the proper encryption level to access this Session.
.

MessageId=7062
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_SESSION_IN_USE
Language=English
The user %s\\%s is currently logged on to this computer. Only the current user or an administrator can log on to this computer.
.
Language=Russian
The user %s\\%s is currently logged on to this computer. Only the current user or an administrator can log on to this computer.
.
Language=Polish
Użytkownik %s\\%s jest aktualnie zalogowany na tym komputerze. Tylko bieżący użytkownik lub administrator mogą zalogować się na tym komputerze.
.
Language=Romanian
The user %s\\%s is currently logged on to this computer. Only the current user or an administrator can log on to this computer.
.

MessageId=7063
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_NO_FORCE_LOGOFF
Language=English
The user %s\\%s is already logged on to the console of this computer. You do not have permission to log in at this time. To resolve this issue, contact %s\\%s and have them log off.
.
Language=Russian
The user %s\\%s is already logged on to the console of this computer. You do not have permission to log in at this time. To resolve this issue, contact %s\\%s and have them log off.
.
Language=Polish
Użytkownik %s\\%s jest już zalogowany do konsoli tego komputera. Nie masz uprawnienia do logowania w tym czasie. Aby rozwiązać ten problem, skontaktuj się z użytkownikiem %s\\%s i poproś o wylogowanie.
.
Language=Romanian
The user %s\\%s is already logged on to the console of this computer. You do not have permission to log in at this time. To resolve this issue, contact %s\\%s and have them log off.
.

MessageId=7064
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_ACCOUNT_RESTRICTION
Language=English
Unable to log you on because of an account restriction.
.
Language=Russian
Unable to log you on because of an account restriction.
.
Language=Polish
Nie możesz się zalogować z powodu ograniczeń konta.
.
Language=Romanian
Unable to log you on because of an account restriction.
.

MessageId=7065
Severity=Success
Facility=System
SymbolicName=ERROR_RDP_PROTOCOL_ERROR
Language=English
The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client.
.
Language=Russian
The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client.
.
Language=Polish
Składnik protokołu RDP %2 wykrył błąd w strumieniu protokołu i rozłączył klienta.
.
Language=Romanian
The RDP protocol component %2 detected an error in the protocol stream and has disconnected the client.
.

MessageId=7066
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CDM_CONNECT
Language=English
The Client Drive Mapping Service Has Connected on Terminal Connection.
.
Language=Russian
The Client Drive Mapping Service Has Connected on Terminal Connection.
.
Language=Polish
Usługa klienta mapowania dysków została połączona prze użyciu połączenia Terminal.
.
Language=Romanian
The Client Drive Mapping Service Has Connected on Terminal Connection.
.

MessageId=7067
Severity=Success
Facility=System
SymbolicName=ERROR_CTX_CDM_DISCONNECT
Language=English
The Client Drive Mapping Service Has Disconnected on Terminal Connection.
.
Language=Russian
The Client Drive Mapping Service Has Disconnected on Terminal Connection.
.
Language=Polish
Usługa klienta mapowania dysków została rozłączona prze użyciu połączenia Terminal.
.
Language=Romanian
The Client Drive Mapping Service Has Disconnected on Terminal Connection.
.

MessageId=8001
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INVALID_API_SEQUENCE
Language=English
The file replication service API was called incorrectly.
.
Language=Russian
The file replication service API was called incorrectly.
.
Language=Polish
Interfejs API usługi replikacji plików został niepoprawnie wywołany.
.
Language=Romanian
The file replication service API was called incorrectly.
.

MessageId=8002
Severity=Success
Facility=System
SymbolicName=FRS_ERR_STARTING_SERVICE
Language=English
The file replication service cannot be started.
.
Language=Russian
The file replication service cannot be started.
.
Language=Polish
Nie można uruchomić usługi replikacji plików.
.
Language=Romanian
The file replication service cannot be started.
.

MessageId=8003
Severity=Success
Facility=System
SymbolicName=FRS_ERR_STOPPING_SERVICE
Language=English
The file replication service cannot be stopped.
.
Language=Russian
The file replication service cannot be stopped.
.
Language=Polish
Nie można zatrzymać usługi replikacji plików.
.
Language=Romanian
The file replication service cannot be stopped.
.

MessageId=8004
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INTERNAL_API
Language=English
The file replication service API terminated the request. The event log may have more information.
.
Language=Russian
The file replication service API terminated the request. The event log may have more information.
.
Language=Polish
Interfejs API usługi replikacji plików przerwał żądanie. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service API terminated the request. The event log may have more information.
.

MessageId=8005
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INTERNAL
Language=English
The file replication service terminated the request. The event log may have more information.
.
Language=Russian
The file replication service terminated the request. The event log may have more information.
.
Language=Polish
Usługa replikacji plików przerwała żądanie. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service terminated the request. The event log may have more information.
.

MessageId=8006
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SERVICE_COMM
Language=English
The file replication service cannot be contacted. The event log may have more information.
.
Language=Russian
The file replication service cannot be contacted. The event log may have more information.
.
Language=Polish
Nie można skontaktować się z usługą replikacji plików. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot be contacted. The event log may have more information.
.

MessageId=8007
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INSUFFICIENT_PRIV
Language=English
The file replication service cannot satisfy the request because the user has insufficient privileges. The event log may have more information.
.
Language=Russian
The file replication service cannot satisfy the request because the user has insufficient privileges. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może spełnić żądania, ponieważ użytkownik ma niewystarczające uprawnienia. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot satisfy the request because the user has insufficient privileges. The event log may have more information.
.

MessageId=8008
Severity=Success
Facility=System
SymbolicName=FRS_ERR_AUTHENTICATION
Language=English
The file replication service cannot satisfy the request because authenticated RPC is not available. The event log may have more information.
.
Language=Russian
The file replication service cannot satisfy the request because authenticated RPC is not available. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może spełnić żądania, ponieważ uwierzytelniony serwer RPC nie jest dostępny. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot satisfy the request because authenticated RPC is not available. The event log may have more information.
.

MessageId=8009
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_INSUFFICIENT_PRIV
Language=English
The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller. The event log may have more information.
.
Language=Russian
The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może spełnić żądania, ponieważ użytkownik ma niewystarczające uprawnienia w odniesieniu do kontrolera domeny. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot satisfy the request because the user has insufficient privileges on the domain controller. The event log may have more information.
.

MessageId=8010
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_AUTHENTICATION
Language=English
The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller. The event log may have more information.
.
Language=Russian
The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może spełnić żądania, ponieważ uwierzytelniony serwer RPC nie jest dostępny w kontrolerze domeny. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot satisfy the request because authenticated RPC is not available on the domain controller. The event log may have more information.
.

MessageId=8011
Severity=Success
Facility=System
SymbolicName=FRS_ERR_CHILD_TO_PARENT_COMM
Language=English
The file replication service cannot communicate with the file replication service on the domain controller. The event log may have more information.
.
Language=Russian
The file replication service cannot communicate with the file replication service on the domain controller. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może skomunikować się z usługą replikacji w kontrolerze domeny. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot communicate with the file replication service on the domain controller. The event log may have more information.
.

MessageId=8012
Severity=Success
Facility=System
SymbolicName=FRS_ERR_PARENT_TO_CHILD_COMM
Language=English
The file replication service on the domain controller cannot communicate with the file replication service on this computer. The event log may have more information.
.
Language=Russian
The file replication service on the domain controller cannot communicate with the file replication service on this computer. The event log may have more information.
.
Language=Polish
Usługa replikacji plików w kontrolerze domeny nie może skomunikować się z usługą replikacji plików na tym komputerze. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service on the domain controller cannot communicate with the file replication service on this computer. The event log may have more information.
.

MessageId=8013
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_POPULATE
Language=English
The file replication service cannot populate the system volume because of an internal error. The event log may have more information.
.
Language=Russian
The file replication service cannot populate the system volume because of an internal error. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może wypełnić woluminu systemowego, ponieważ wystąpił wewnętrzny błąd. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot populate the system volume because of an internal error. The event log may have more information.
.

MessageId=8014
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_POPULATE_TIMEOUT
Language=English
The file replication service cannot populate the system volume because of an internal timeout. The event log may have more information.
.
Language=Russian
The file replication service cannot populate the system volume because of an internal timeout. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może wypełnić woluminu systemowego, ponieważ upłynął wewnętrzny limit czasu. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot populate the system volume because of an internal timeout. The event log may have more information.
.

MessageId=8015
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_IS_BUSY
Language=English
The file replication service cannot process the request. The system volume is busy with a previous request.
.
Language=Russian
The file replication service cannot process the request. The system volume is busy with a previous request.
.
Language=Polish
Usługa replikacji plików nie może przetworzyć żądania. Wolumin systemowy jest zajęty poprzednim żądaniem.
.
Language=Romanian
The file replication service cannot process the request. The system volume is busy with a previous request.
.

MessageId=8016
Severity=Success
Facility=System
SymbolicName=FRS_ERR_SYSVOL_DEMOTE
Language=English
The file replication service cannot stop replicating the system volume because of an internal error. The event log may have more information.
.
Language=Russian
The file replication service cannot stop replicating the system volume because of an internal error. The event log may have more information.
.
Language=Polish
Usługa replikacji plików nie może zatrzymać replikacji woluminu systemowego, ponieważ wystąpił błąd wewnętrzny. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
The file replication service cannot stop replicating the system volume because of an internal error. The event log may have more information.
.

MessageId=8017
Severity=Success
Facility=System
SymbolicName=FRS_ERR_INVALID_SERVICE_PARAMETER
Language=English
The file replication service detected an invalid parameter.
.
Language=Russian
The file replication service detected an invalid parameter.
.
Language=Polish
Usługa replikacji plików wykryła nieprawidłowy parametr.
.
Language=Romanian
The file replication service detected an invalid parameter.
.

MessageId=8200
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_INSTALLED
Language=English
An error occurred while installing the directory service. For more information, see the event log.
.
Language=Russian
An error occurred while installing the directory service. For more information, see the event log.
.
Language=Polish
Podczas instalowania usługi katalogowej wystąpił błąd. Więcej informacji można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
An error occurred while installing the directory service. For more information, see the event log.
.

MessageId=8201
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY
Language=English
The directory service evaluated group memberships locally.
.
Language=Russian
The directory service evaluated group memberships locally.
.
Language=Polish
Usługa katalogowa oceniła lokalnie członkostwo w grupach.
.
Language=Romanian
The directory service evaluated group memberships locally.
.

MessageId=8202
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_ATTRIBUTE_OR_VALUE
Language=English
The specified directory service attribute or value does not exist.
.
Language=Russian
The specified directory service attribute or value does not exist.
.
Language=Polish
Określony atrybut lub wartość usługi katalogowej nie istnieje.
.
Language=Romanian
The specified directory service attribute or value does not exist.
.

MessageId=8203
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_ATTRIBUTE_SYNTAX
Language=English
The attribute syntax specified to the directory service is invalid.
.
Language=Russian
The attribute syntax specified to the directory service is invalid.
.
Language=Polish
Składnia atrybutu podana dla usługi katalogowej jest nieprawidłowa.
.
Language=Romanian
The attribute syntax specified to the directory service is invalid.
.

MessageId=8204
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED
Language=English
The attribute type specified to the directory service is not defined.
.
Language=Russian
The attribute type specified to the directory service is not defined.
.
Language=Polish
Typ atrybutu podany dla usługi katalogowej nie jest zdefiniowany.
.
Language=Romanian
The attribute type specified to the directory service is not defined.
.

MessageId=8205
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS
Language=English
The specified directory service attribute or value already exists.
.
Language=Russian
The specified directory service attribute or value already exists.
.
Language=Polish
Określony atrybut lub wartość usługi katalogowej już istnieje.
.
Language=Romanian
The specified directory service attribute or value already exists.
.

MessageId=8206
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BUSY
Language=English
The directory service is busy.
.
Language=Russian
The directory service is busy.
.
Language=Polish
Usługa katalogowa jest zajęta.
.
Language=Romanian
The directory service is busy.
.

MessageId=8207
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNAVAILABLE
Language=English
The directory service is unavailable.
.
Language=Russian
The directory service is unavailable.
.
Language=Polish
Usługa katalogowa jest niedostępna.
.
Language=Romanian
The directory service is unavailable.
.

MessageId=8208
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RIDS_ALLOCATED
Language=English
The directory service was unable to allocate a relative identifier.
.
Language=Russian
The directory service was unable to allocate a relative identifier.
.
Language=Polish
Usługa katalogowa nie może przydzielić identyfikatora względnego.
.
Language=Romanian
The directory service was unable to allocate a relative identifier.
.

MessageId=8209
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_MORE_RIDS
Language=English
The directory service has exhausted the pool of relative identifiers.
.
Language=Russian
The directory service has exhausted the pool of relative identifiers.
.
Language=Polish
Usługa katalogowa wyczerpała pulę identyfikatorów względnych.
.
Language=Romanian
The directory service has exhausted the pool of relative identifiers.
.

MessageId=8210
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCORRECT_ROLE_OWNER
Language=English
The requested operation could not be performed because the directory service is not the master for that type of operation.
.
Language=Russian
The requested operation could not be performed because the directory service is not the master for that type of operation.
.
Language=Polish
Nie można wykonać żądanej operacji, ponieważ usługa katalogowa nie jest usługą wzorcową dla tego typu operacji.
.
Language=Romanian
The requested operation could not be performed because the directory service is not the master for that type of operation.
.

MessageId=8211
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RIDMGR_INIT_ERROR
Language=English
The directory service was unable to initialize the subsystem that allocates relative identifiers.
.
Language=Russian
The directory service was unable to initialize the subsystem that allocates relative identifiers.
.
Language=Polish
Usługa katalogowa nie może zainicjować podsystemu przydzielającego identyfikatory względne.
.
Language=Romanian
The directory service was unable to initialize the subsystem that allocates relative identifiers.
.

MessageId=8212
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_VIOLATION
Language=English
The requested operation did not satisfy one or more constraints associated with the class of the object.
.
Language=Russian
The requested operation did not satisfy one or more constraints associated with the class of the object.
.
Language=Polish
Żądana operacja nie spełnia jednego lub więcej warunków ograniczających skojarzonych z klasą obiektu.
.
Language=Romanian
The requested operation did not satisfy one or more constraints associated with the class of the object.
.

MessageId=8213
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ON_NON_LEAF
Language=English
The directory service can perform the requested operation only on a leaf object.
.
Language=Russian
The directory service can perform the requested operation only on a leaf object.
.
Language=Polish
Usługa katalogowa może przeprowadzić żądaną operację tylko na obiekcie typu liść.
.
Language=Romanian
The directory service can perform the requested operation only on a leaf object.
.

MessageId=8214
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ON_RDN
Language=English
The directory service cannot perform the requested operation on the RDN attribute of an object.
.
Language=Russian
The directory service cannot perform the requested operation on the RDN attribute of an object.
.
Language=Polish
Usługa katalogowa nie może przeprowadzić żądanej operacji na atrybucie RDN obiektu.
.
Language=Romanian
The directory service cannot perform the requested operation on the RDN attribute of an object.
.

MessageId=8215
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_OBJ_CLASS
Language=English
The directory service detected an attempt to modify the object class of an object.
.
Language=Russian
The directory service detected an attempt to modify the object class of an object.
.
Language=Polish
Usługa katalogowa wykryła próbę modyfikacji klasy obiektu.
.
Language=Romanian
The directory service detected an attempt to modify the object class of an object.
.

MessageId=8216
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_DOM_MOVE_ERROR
Language=English
The requested cross-domain move operation could not be performed.
.
Language=Russian
The requested cross-domain move operation could not be performed.
.
Language=Polish
Nie można wykonać żądanej operacji przeniesienia poza domenę.
.
Language=Romanian
The requested cross-domain move operation could not be performed.
.

MessageId=8217
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GC_NOT_AVAILABLE
Language=English
Unable to contact the global catalog server.
.
Language=Russian
Unable to contact the global catalog server.
.
Language=Polish
Nie można skontaktować się z serwerem wykazu globalnego.
.
Language=Romanian
Unable to contact the global catalog server.
.

MessageId=8218
Severity=Success
Facility=System
SymbolicName=ERROR_SHARED_POLICY
Language=English
The policy object is shared and can only be modified at the root.
.
Language=Russian
The policy object is shared and can only be modified at the root.
.
Language=Polish
Obiekt zasad jest udostępniany i może być tylko modyfikowany na poziomie głównym.
.
Language=Romanian
The policy object is shared and can only be modified at the root.
.

MessageId=8219
Severity=Success
Facility=System
SymbolicName=ERROR_POLICY_OBJECT_NOT_FOUND
Language=English
The policy object does not exist.
.
Language=Russian
The policy object does not exist.
.
Language=Polish
Obiekt zasad nie istnieje.
.
Language=Romanian
The policy object does not exist.
.

MessageId=8220
Severity=Success
Facility=System
SymbolicName=ERROR_POLICY_ONLY_IN_DS
Language=English
The requested policy information is only in the directory service.
.
Language=Russian
The requested policy information is only in the directory service.
.
Language=Polish
Żądane informacje o zasadach występują tylko w usłudze katalogowej.
.
Language=Romanian
The requested policy information is only in the directory service.
.

MessageId=8221
Severity=Success
Facility=System
SymbolicName=ERROR_PROMOTION_ACTIVE
Language=English
A domain controller promotion is currently active.
.
Language=Russian
A domain controller promotion is currently active.
.
Language=Polish
Proces promocji kontrolera domeny jest obecnie aktywny.
.
Language=Romanian
A domain controller promotion is currently active.
.

MessageId=8222
Severity=Success
Facility=System
SymbolicName=ERROR_NO_PROMOTION_ACTIVE
Language=English
A domain controller promotion is not currently active
.
Language=Russian
A domain controller promotion is not currently active
.
Language=Polish
Proces promocji kontrolera domeny nie jest obecnie aktywny.
.
Language=Romanian
A domain controller promotion is not currently active
.

MessageId=8224
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OPERATIONS_ERROR
Language=English
An operations error occurred.
.
Language=Russian
An operations error occurred.
.
Language=Polish
Wystąpił błąd operacji.
.
Language=Romanian
An operations error occurred.
.

MessageId=8225
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PROTOCOL_ERROR
Language=English
A protocol error occurred.
.
Language=Russian
A protocol error occurred.
.
Language=Polish
Wystąpił błąd protokołu.
.
Language=Romanian
A protocol error occurred.
.

MessageId=8226
Severity=Success
Facility=System
SymbolicName=ERROR_DS_TIMELIMIT_EXCEEDED
Language=English
The time limit for this request was exceeded.
.
Language=Russian
The time limit for this request was exceeded.
.
Language=Polish
Limit czasu dla tego żądania został przekroczony.
.
Language=Romanian
The time limit for this request was exceeded.
.

MessageId=8227
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SIZELIMIT_EXCEEDED
Language=English
The size limit for this request was exceeded.
.
Language=Russian
The size limit for this request was exceeded.
.
Language=Polish
Limit rozmiaru dla tego żądania został przekroczony.
.
Language=Romanian
The size limit for this request was exceeded.
.

MessageId=8228
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ADMIN_LIMIT_EXCEEDED
Language=English
The administrative limit for this request was exceeded.
.
Language=Russian
The administrative limit for this request was exceeded.
.
Language=Polish
Limit administracyjny dla tego żądania został przekroczony.
.
Language=Romanian
The administrative limit for this request was exceeded.
.

MessageId=8229
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COMPARE_FALSE
Language=English
The compare response was false.
.
Language=Russian
The compare response was false.
.
Language=Polish
Wynik porównania: fałsz.
.
Language=Romanian
The compare response was false.
.

MessageId=8230
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COMPARE_TRUE
Language=English
The compare response was true.
.
Language=Russian
The compare response was true.
.
Language=Polish
Wynik porównania: prawda.
.
Language=Romanian
The compare response was true.
.

MessageId=8231
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTH_METHOD_NOT_SUPPORTED
Language=English
The requested authentication method is not supported by the server.
.
Language=Russian
The requested authentication method is not supported by the server.
.
Language=Polish
Żądana metoda uwierzytelniania nie jest obsługiwana przez serwer.
.
Language=Romanian
The requested authentication method is not supported by the server.
.

MessageId=8232
Severity=Success
Facility=System
SymbolicName=ERROR_DS_STRONG_AUTH_REQUIRED
Language=English
A more secure authentication method is required for this server.
.
Language=Russian
A more secure authentication method is required for this server.
.
Language=Polish
Dla tego serwera jest wymagana bardziej bezpieczna metoda uwierzytelniania.
.
Language=Romanian
A more secure authentication method is required for this server.
.

MessageId=8233
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INAPPROPRIATE_AUTH
Language=English
Inappropriate authentication.
.
Language=Russian
Inappropriate authentication.
.
Language=Polish
Nieodpowiednie uwierzytelnienia.
.
Language=Romanian
Inappropriate authentication.
.

MessageId=8234
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTH_UNKNOWN
Language=English
The authentication mechanism is unknown.
.
Language=Russian
The authentication mechanism is unknown.
.
Language=Polish
Mechanizm uwierzytelniania jest nieznany.
.
Language=Romanian
The authentication mechanism is unknown.
.

MessageId=8235
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFERRAL
Language=English
A referral was returned from the server.
.
Language=Russian
A referral was returned from the server.
.
Language=Polish
Odniesienie zostało zwrócone z serwera.
.
Language=Romanian
A referral was returned from the server.
.

MessageId=8236
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNAVAILABLE_CRIT_EXTENSION
Language=English
The server does not support the requested critical extension.
.
Language=Russian
The server does not support the requested critical extension.
.
Language=Polish
Serwer nie obsługuje żądanego rozszerzenia krytycznego.
.
Language=Romanian
The server does not support the requested critical extension.
.

MessageId=8237
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONFIDENTIALITY_REQUIRED
Language=English
This request requires a secure connection.
.
Language=Russian
This request requires a secure connection.
.
Language=Polish
To żądanie wymaga bezpiecznego połączenia.
.
Language=Romanian
This request requires a secure connection.
.

MessageId=8238
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INAPPROPRIATE_MATCHING
Language=English
Inappropriate matching.
.
Language=Russian
Inappropriate matching.
.
Language=Polish
Nieodpowiednie dopasowanie.
.
Language=Romanian
Inappropriate matching.
.

MessageId=8239
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONSTRAINT_VIOLATION
Language=English
A constraint violation occurred.
.
Language=Russian
A constraint violation occurred.
.
Language=Polish
Wystąpiło naruszenie więzów.
.
Language=Romanian
A constraint violation occurred.
.

MessageId=8240
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_SUCH_OBJECT
Language=English
There is no such object on the server.
.
Language=Russian
There is no such object on the server.
.
Language=Polish
Nie ma takiego obiektu na serwerze.
.
Language=Romanian
There is no such object on the server.
.

MessageId=8241
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_PROBLEM
Language=English
There is an alias problem.
.
Language=Russian
There is an alias problem.
.
Language=Polish
Problem z aliasem.
.
Language=Romanian
There is an alias problem.
.

MessageId=8242
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_DN_SYNTAX
Language=English
An invalid dn syntax has been specified.
.
Language=Russian
An invalid dn syntax has been specified.
.
Language=Polish
Użyto niepoprawnej składni nazwy domeny.
.
Language=Romanian
An invalid dn syntax has been specified.
.

MessageId=8243
Severity=Success
Facility=System
SymbolicName=ERROR_DS_IS_LEAF
Language=English
The object is a leaf object.
.
Language=Russian
The object is a leaf object.
.
Language=Polish
Obiekt jest obiektem typu liść.
.
Language=Romanian
The object is a leaf object.
.

MessageId=8244
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_DEREF_PROBLEM
Language=English
There is an alias dereferencing problem.
.
Language=Russian
There is an alias dereferencing problem.
.
Language=Polish
Występuje problem z usunięciem odwołania do aliasu.
.
Language=Romanian
There is an alias dereferencing problem.
.

MessageId=8245
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNWILLING_TO_PERFORM
Language=English
The server is unwilling to process the request.
.
Language=Russian
The server is unwilling to process the request.
.
Language=Polish
Serwer odmawia przetwarzania żądania.
.
Language=Romanian
The server is unwilling to process the request.
.

MessageId=8246
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOOP_DETECT
Language=English
A loop has been detected.
.
Language=Russian
A loop has been detected.
.
Language=Polish
Została wykryta pętla.
.
Language=Romanian
A loop has been detected.
.

MessageId=8247
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAMING_VIOLATION
Language=English
There is a naming violation.
.
Language=Russian
There is a naming violation.
.
Language=Polish
Naruszenie zasad nazewnictwa.
.
Language=Romanian
There is a naming violation.
.

MessageId=8248
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_RESULTS_TOO_LARGE
Language=English
The result set is too large.
.
Language=Russian
The result set is too large.
.
Language=Polish
Zestaw wynikowy jest zbyt duży.
.
Language=Romanian
The result set is too large.
.

MessageId=8249
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AFFECTS_MULTIPLE_DSAS
Language=English
The operation affects multiple DSAs
.
Language=Russian
The operation affects multiple DSAs
.
Language=Polish
Operacja wpływa na wielu agentów DSA.
.
Language=Romanian
The operation affects multiple DSAs
.

MessageId=8250
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SERVER_DOWN
Language=English
The server is not operational.
.
Language=Russian
The server is not operational.
.
Language=Polish
Serwer nie działa.
.
Language=Romanian
The server is not operational.
.

MessageId=8251
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_ERROR
Language=English
A local error has occurred.
.
Language=Russian
A local error has occurred.
.
Language=Polish
Wstąpił błąd lokalny.
.
Language=Romanian
A local error has occurred.
.

MessageId=8252
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ENCODING_ERROR
Language=English
An encoding error has occurred.
.
Language=Russian
An encoding error has occurred.
.
Language=Polish
Wystąpił błąd kodowania.
.
Language=Romanian
An encoding error has occurred.
.

MessageId=8253
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DECODING_ERROR
Language=English
A decoding error has occurred.
.
Language=Russian
A decoding error has occurred.
.
Language=Polish
Wystąpił błąd dekodowania.
.
Language=Romanian
A decoding error has occurred.
.

MessageId=8254
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FILTER_UNKNOWN
Language=English
The search filter cannot be recognized.
.
Language=Russian
The search filter cannot be recognized.
.
Language=Polish
Nieznany filtr wyszukiwania.
.
Language=Romanian
The search filter cannot be recognized.
.

MessageId=8255
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PARAM_ERROR
Language=English
One or more parameters are illegal.
.
Language=Russian
One or more parameters are illegal.
.
Language=Polish
Co najmniej jeden z parametrów jest niedozwolony.
.
Language=Romanian
One or more parameters are illegal.
.

MessageId=8256
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_SUPPORTED
Language=English
The specified method is not supported.
.
Language=Russian
The specified method is not supported.
.
Language=Polish
Podana metoda nie jest obsługiwana.
.
Language=Romanian
The specified method is not supported.
.

MessageId=8257
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RESULTS_RETURNED
Language=English
No results were returned.
.
Language=Russian
No results were returned.
.
Language=Polish
Nie zwrócono żadnych wyników.
.
Language=Romanian
No results were returned.
.

MessageId=8258
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONTROL_NOT_FOUND
Language=English
The specified control is not supported by the server.
.
Language=Russian
The specified control is not supported by the server.
.
Language=Polish
Podany formant nie jest obsługiwany przez serwer.
.
Language=Romanian
The specified control is not supported by the server.
.

MessageId=8259
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLIENT_LOOP
Language=English
A referral loop was detected by the client.
.
Language=Russian
A referral loop was detected by the client.
.
Language=Polish
Klient wykrył pętlę odniesień.
.
Language=Romanian
A referral loop was detected by the client.
.

MessageId=8260
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFERRAL_LIMIT_EXCEEDED
Language=English
The preset referral limit was exceeded.
.
Language=Russian
The preset referral limit was exceeded.
.
Language=Polish
Ustalony wstępnie limit odniesień został przekroczony.
.
Language=Romanian
The preset referral limit was exceeded.
.

MessageId=8261
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SORT_CONTROL_MISSING
Language=English
The search requires a SORT control.
.
Language=Russian
The search requires a SORT control.
.
Language=Polish
Wyszukiwanie wymaga elementu sterującego SORT.
.
Language=Romanian
The search requires a SORT control.
.

MessageId=8262
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OFFSET_RANGE_ERROR
Language=English
The search results exceed the offset range specified.
.
Language=Russian
The search results exceed the offset range specified.
.
Language=Polish
Wyniki wyszukiwania przekraczają określony zakres przesunięcia.
.
Language=Romanian
The search results exceed the offset range specified.
.

MessageId=8301
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_MUST_BE_NC
Language=English
The root object must be the head of a naming context. The root object cannot have an instantiated parent.
.
Language=Russian
The root object must be the head of a naming context. The root object cannot have an instantiated parent.
.
Language=Polish
Obiekt główny musi być na początku kontekstu nazewnictwa. Rodzicem obiektu głównego nie może być wystąpienie obiektu.
.
Language=Romanian
The root object must be the head of a naming context. The root object cannot have an instantiated parent.
.

MessageId=8302
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ADD_REPLICA_INHIBITED
Language=English
The add replica operation cannot be performed. The naming context must be writeable in order to create the replica.
.
Language=Russian
The add replica operation cannot be performed. The naming context must be writeable in order to create the replica.
.
Language=Polish
Nie można wykonać operacji dodania repliki. Kontekst nazewnictwa musi mieć możliwość zapisu, Aby można było utworzyć replikę, musi istnieć możliwość zapisywania w kontekście nazewnictwa.
.
Language=Romanian
The add replica operation cannot be performed. The naming context must be writeable in order to create the replica.
.

MessageId=8303
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_NOT_DEF_IN_SCHEMA
Language=English
A reference to an attribute that is not defined in the schema occurred.
.
Language=Russian
A reference to an attribute that is not defined in the schema occurred.
.
Language=Polish
Wystąpiło odwołanie do atrybutu, który nie jest zdefiniowany w schemacie.
.
Language=Romanian
A reference to an attribute that is not defined in the schema occurred.
.

MessageId=8304
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MAX_OBJ_SIZE_EXCEEDED
Language=English
The maximum size of an object has been exceeded.
.
Language=Russian
The maximum size of an object has been exceeded.
.
Language=Polish
Został przekroczony maksymalny rozmiar obiektu.
.
Language=Romanian
The maximum size of an object has been exceeded.
.

MessageId=8305
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_STRING_NAME_EXISTS
Language=English
An attempt was made to add an object to the directory with a name that is already in use.
.
Language=Russian
An attempt was made to add an object to the directory with a name that is already in use.
.
Language=Polish
Została podjęta próba dodania do katalogu obiektu o już istniejącej nazwie.
.
Language=Romanian
An attempt was made to add an object to the directory with a name that is already in use.
.

MessageId=8306
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_RDN_DEFINED_IN_SCHEMA
Language=English
An attempt was made to add an object of a class that does not have an RDN defined in the schema.
.
Language=Russian
An attempt was made to add an object of a class that does not have an RDN defined in the schema.
.
Language=Polish
Została podjęta próba dodania obiektu klasy, która w schemacie nie ma zdefiniowanej nazwy RDN.
.
Language=Romanian
An attempt was made to add an object of a class that does not have an RDN defined in the schema.
.

MessageId=8307
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RDN_DOESNT_MATCH_SCHEMA
Language=English
An attempt was made to add an object using an RDN that is not the RDN defined in the schema.
.
Language=Russian
An attempt was made to add an object using an RDN that is not the RDN defined in the schema.
.
Language=Polish
Została podjęta próba dodania obiektu za pomocą nazwy RDN, która nie jest nazwą RDN zdefiniowaną w schemacie.
.
Language=Romanian
An attempt was made to add an object using an RDN that is not the RDN defined in the schema.
.

MessageId=8308
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_REQUESTED_ATTS_FOUND
Language=English
None of the requested attributes were found on the objects.
.
Language=Russian
None of the requested attributes were found on the objects.
.
Language=Polish
Nie znaleziono w obiektach żadnych z wymaganych atrybutów.
.
Language=Romanian
None of the requested attributes were found on the objects.
.

MessageId=8309
Severity=Success
Facility=System
SymbolicName=ERROR_DS_USER_BUFFER_TO_SMALL
Language=English
The user buffer is too small.
.
Language=Russian
The user buffer is too small.
.
Language=Polish
Bufor użytkownika jest za mały.
.
Language=Romanian
The user buffer is too small.
.

MessageId=8310
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_IS_NOT_ON_OBJ
Language=English
The attribute specified in the operation is not present on the object.
.
Language=Russian
The attribute specified in the operation is not present on the object.
.
Language=Polish
Atrybut podany w operacji nie występuje w obiekcie.
.
Language=Romanian
The attribute specified in the operation is not present on the object.
.

MessageId=8311
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_MOD_OPERATION
Language=English
Illegal modify operation. Some aspect of the modification is not permitted.
.
Language=Russian
Illegal modify operation. Some aspect of the modification is not permitted.
.
Language=Polish
Niedozwolona operacja modyfikowania. Niektóre aspekty modyfikacji nie są dozwolone.
.
Language=Romanian
Illegal modify operation. Some aspect of the modification is not permitted.
.

MessageId=8312
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_TOO_LARGE
Language=English
The specified object is too large.
.
Language=Russian
The specified object is too large.
.
Language=Polish
Podany obiekt jest zbyt duży.
.
Language=Romanian
The specified object is too large.
.

MessageId=8313
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_INSTANCE_TYPE
Language=English
The specified instance type is not valid.
.
Language=Russian
The specified instance type is not valid.
.
Language=Polish
Podany typ wystąpienia jest nieprawidłowy.
.
Language=Romanian
The specified instance type is not valid.
.

MessageId=8314
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MASTERDSA_REQUIRED
Language=English
The operation must be performed at a master DSA.
.
Language=Russian
The operation must be performed at a master DSA.
.
Language=Polish
Operacja musi być dokonana na głównym serwerze DSA.
.
Language=Romanian
The operation must be performed at a master DSA.
.

MessageId=8315
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_CLASS_REQUIRED
Language=English
The object class attribute must be specified.
.
Language=Russian
The object class attribute must be specified.
.
Language=Polish
Musi być podany atrybut klasy obiektu.
.
Language=Romanian
The object class attribute must be specified.
.

MessageId=8316
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_REQUIRED_ATT
Language=English
A required attribute is missing.
.
Language=Russian
A required attribute is missing.
.
Language=Polish
Brak wymaganego atrybutu.
.
Language=Romanian
A required attribute is missing.
.

MessageId=8317
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_NOT_DEF_FOR_CLASS
Language=English
An attempt was made to modify an object to include an attribute that is not legal for its class
.
Language=Russian
An attempt was made to modify an object to include an attribute that is not legal for its class
.
Language=Polish
Nastąpiła próba modyfikacji obiektu w celu dołączenia atrybutu, który nie jest dozwolony dla klasy obiektu.
.
Language=Romanian
An attempt was made to modify an object to include an attribute that is not legal for its class
.

MessageId=8318
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_ALREADY_EXISTS
Language=English
The specified attribute is already present on the object.
.
Language=Russian
The specified attribute is already present on the object.
.
Language=Polish
Podany atrybut już występuje w obiekcie.
.
Language=Romanian
The specified attribute is already present on the object.
.

MessageId=8320
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_ATT_VALUES
Language=English
The specified attribute is not present, or has no values.
.
Language=Russian
The specified attribute is not present, or has no values.
.
Language=Polish
Podany atrybut jest nieobecny lub nie ma nadanej wartości.
.
Language=Romanian
The specified attribute is not present, or has no values.
.

MessageId=8321
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SINGLE_VALUE_CONSTRAINT
Language=English
Multiple values were specified for an attribute that can have only one value.
.
Language=Russian
Multiple values were specified for an attribute that can have only one value.
.
Language=Polish
Dla atrybutu, który może mieć tylko jedną wartość, zostały podane wielokrotne wartości.
.
Language=Romanian
Multiple values were specified for an attribute that can have only one value.
.

MessageId=8322
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RANGE_CONSTRAINT
Language=English
A value for the attribute was not in the acceptable range of values.
.
Language=Russian
A value for the attribute was not in the acceptable range of values.
.
Language=Polish
Wartość atrybutu nie mieści się w przedziale dozwolonych wartości.
.
Language=Romanian
A value for the attribute was not in the acceptable range of values.
.

MessageId=8323
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_VAL_ALREADY_EXISTS
Language=English
The specified value already exists.
.
Language=Russian
The specified value already exists.
.
Language=Polish
Podana wartość już istnieje.
.
Language=Romanian
The specified value already exists.
.

MessageId=8324
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT
Language=English
The attribute cannot be removed because it is not present on the object.
.
Language=Russian
The attribute cannot be removed because it is not present on the object.
.
Language=Polish
Nie można usunąć atrybutu, ponieważ nie występuje on w obiekcie.
.
Language=Romanian
The attribute cannot be removed because it is not present on the object.
.

MessageId=8325
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REM_MISSING_ATT_VAL
Language=English
The attribute value cannot be removed because it is not present on the object.
.
Language=Russian
The attribute value cannot be removed because it is not present on the object.
.
Language=Polish
Nie można usunąć wartości atrybutu, ponieważ nie występuje ona w obiekcie.
.
Language=Romanian
The attribute value cannot be removed because it is not present on the object.
.

MessageId=8326
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_CANT_BE_SUBREF
Language=English
The specified root object cannot be a subref.
.
Language=Russian
The specified root object cannot be a subref.
.
Language=Polish
Podany obiekt główny nie może być odniesieniem podrzędnym.
.
Language=Romanian
The specified root object cannot be a subref.
.

MessageId=8327
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHAINING
Language=English
Chaining is not permitted.
.
Language=Russian
Chaining is not permitted.
.
Language=Polish
Tworzenie łańcucha nie jest dozwolone.
.
Language=Romanian
Chaining is not permitted.
.

MessageId=8328
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHAINED_EVAL
Language=English
Chained evaluation is not permitted.
.
Language=Russian
Chained evaluation is not permitted.
.
Language=Polish
Ocena łańcuchowa nie jest dozwolona.
.
Language=Romanian
Chained evaluation is not permitted.
.

MessageId=8329
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_PARENT_OBJECT
Language=English
The operation could not be performed because the object's parent is either uninstantiated or deleted.
.
Language=Russian
The operation could not be performed because the object's parent is either uninstantiated or deleted.
.
Language=Polish
Operacja nie może być wykonana, ponieważ usunięto albo wystąpienie rodzica obiektu, albo samego rodzica.
.
Language=Romanian
The operation could not be performed because the object's parent is either uninstantiated or deleted.
.

MessageId=8330
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PARENT_IS_AN_ALIAS
Language=English
Having a parent that is an alias is not permitted. Aliases are leaf objects.
.
Language=Russian
Having a parent that is an alias is not permitted. Aliases are leaf objects.
.
Language=Polish
Posiadanie rodzica, który jest aliasem jest niedozwolone. Aliasy są obiektami typu liść.
.
Language=Romanian
Having a parent that is an alias is not permitted. Aliases are leaf objects.
.

MessageId=8331
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MIX_MASTER_AND_REPS
Language=English
The object and parent must be of the same type, either both masters or both replicas.
.
Language=Russian
The object and parent must be of the same type, either both masters or both replicas.
.
Language=Polish
Obiekt i rodzic muszą być tego samego typu - muszą być albo obiektami głównymi, albo replikami.
.
Language=Romanian
The object and parent must be of the same type, either both masters or both replicas.
.

MessageId=8332
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CHILDREN_EXIST
Language=English
The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object.
.
Language=Russian
The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object.
.
Language=Polish
Operacja nie może być wykonana, ponieważ istnieje obiekt podrzędny. Ta operacja może jedynie być wykonana na obiekcie typu liść.
.
Language=Romanian
The operation cannot be performed because child objects exist. This operation can only be performed on a leaf object.
.

MessageId=8333
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_NOT_FOUND
Language=English
Directory object not found.
.
Language=Russian
Directory object not found.
.
Language=Polish
Nie znaleziono obiektu katalogu.
.
Language=Romanian
Directory object not found.
.

MessageId=8334
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIASED_OBJ_MISSING
Language=English
The aliased object is missing.
.
Language=Russian
The aliased object is missing.
.
Language=Polish
Brakuje obiektu, dla którego utworzono alias.
.
Language=Romanian
The aliased object is missing.
.

MessageId=8335
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_NAME_SYNTAX
Language=English
The object name has bad syntax.
.
Language=Russian
The object name has bad syntax.
.
Language=Polish
Nazwa obiektu ma złą składnię.
.
Language=Romanian
The object name has bad syntax.
.

MessageId=8336
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ALIAS_POINTS_TO_ALIAS
Language=English
It is not permitted for an alias to refer to another alias.
.
Language=Russian
It is not permitted for an alias to refer to another alias.
.
Language=Polish
Odwoływanie się aliasu do innego aliasu jest niedozwolone.
.
Language=Romanian
It is not permitted for an alias to refer to another alias.
.

MessageId=8337
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEREF_ALIAS
Language=English
The alias cannot be dereferenced.
.
Language=Russian
The alias cannot be dereferenced.
.
Language=Polish
Nie można usunąć odwołania do aliasu.
.
Language=Romanian
The alias cannot be dereferenced.
.

MessageId=8338
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OUT_OF_SCOPE
Language=English
The operation is out of scope.
.
Language=Russian
The operation is out of scope.
.
Language=Polish
Operacja wykracza poza zakres.
.
Language=Romanian
The operation is out of scope.
.

MessageId=8339
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJECT_BEING_REMOVED
Language=English
The operation cannot continue because the object is in the process of being removed.
.
Language=Russian
The operation cannot continue because the object is in the process of being removed.
.
Language=Polish
Nie można kontynuować operacji, ponieważ trwa proces usuwania danego obiektu.
.
Language=Romanian
The operation cannot continue because the object is in the process of being removed.
.

MessageId=8340
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DELETE_DSA_OBJ
Language=English
The DSA object cannot be deleted.
.
Language=Russian
The DSA object cannot be deleted.
.
Language=Polish
Nie można usunąć obiektu DSA.
.
Language=Romanian
The DSA object cannot be deleted.
.

MessageId=8341
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GENERIC_ERROR
Language=English
A directory service error has occurred.
.
Language=Russian
A directory service error has occurred.
.
Language=Polish
Wystąpił błąd usługi katalogowej.
.
Language=Romanian
A directory service error has occurred.
.

MessageId=8342
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DSA_MUST_BE_INT_MASTER
Language=English
The operation can only be performed on an internal master DSA object.
.
Language=Russian
The operation can only be performed on an internal master DSA object.
.
Language=Polish
Operacja może być dokonana jedynie na wewnętrznym głównym obiekcie DSA.
.
Language=Romanian
The operation can only be performed on an internal master DSA object.
.

MessageId=8343
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLASS_NOT_DSA
Language=English
The object must be of class DSA.
.
Language=Russian
The object must be of class DSA.
.
Language=Polish
Obiekt musi być klasy DSA.
.
Language=Romanian
The object must be of class DSA.
.

MessageId=8344
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSUFF_ACCESS_RIGHTS
Language=English
Insufficient access rights to perform the operation.
.
Language=Russian
Insufficient access rights to perform the operation.
.
Language=Polish
Niewystarczające prawa dostępu, aby wykonać tę operację.
.
Language=Romanian
Insufficient access rights to perform the operation.
.

MessageId=8345
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_SUPERIOR
Language=English
The object cannot be added because the parent is not on the list of possible superiors.
.
Language=Russian
The object cannot be added because the parent is not on the list of possible superiors.
.
Language=Polish
Nie można dodać obiektu, ponieważ rodzic nie występuje na liście możliwych obiektów nadrzędnych.
.
Language=Romanian
The object cannot be added because the parent is not on the list of possible superiors.
.

MessageId=8346
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATTRIBUTE_OWNED_BY_SAM
Language=English
Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM).
.
Language=Russian
Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM).
.
Language=Polish
Dostęp do atrybutu nie jest dozwolony, ponieważ atrybut jest posiadany przez Menedżera kont zabezpieczeń (SAM).
.
Language=Romanian
Access to the attribute is not permitted because the attribute is owned by the Security Accounts Manager (SAM).
.

MessageId=8347
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TOO_MANY_PARTS
Language=English
The name has too many parts.
.
Language=Russian
The name has too many parts.
.
Language=Polish
Nazwa składa się ze zbyt wielu części.
.
Language=Romanian
The name has too many parts.
.

MessageId=8348
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TOO_LONG
Language=English
The name is too long.
.
Language=Russian
The name is too long.
.
Language=Polish
Nazwa jest zbyt długa.
.
Language=Romanian
The name is too long.
.

MessageId=8349
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_VALUE_TOO_LONG
Language=English
The name value is too long.
.
Language=Russian
The name value is too long.
.
Language=Polish
Wartość nazwy jest zbyt długa.
.
Language=Romanian
The name value is too long.
.

MessageId=8350
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_UNPARSEABLE
Language=English
The directory service encountered an error parsing a name.
.
Language=Russian
The directory service encountered an error parsing a name.
.
Language=Polish
Usługa katalogowa napotkała błąd podczas analizy nazwy.
.
Language=Romanian
The directory service encountered an error parsing a name.
.

MessageId=8351
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_TYPE_UNKNOWN
Language=English
The directory service cannot get the attribute type for a name.
.
Language=Russian
The directory service cannot get the attribute type for a name.
.
Language=Polish
Usługa katalogowa nie może uzyskać typu atrybutu dla nazwy.
.
Language=Romanian
The directory service cannot get the attribute type for a name.
.

MessageId=8352
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_AN_OBJECT
Language=English
The name does not identify an object; the name identifies a phantom.
.
Language=Russian
The name does not identify an object; the name identifies a phantom.
.
Language=Polish
Nazwa nie określa obiektu; nazwa określa fantom.
.
Language=Romanian
The name does not identify an object; the name identifies a phantom.
.

MessageId=8353
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEC_DESC_TOO_SHORT
Language=English
The security descriptor is too short.
.
Language=Russian
The security descriptor is too short.
.
Language=Polish
Deskryptor zabezpieczenia jest za krótki.
.
Language=Romanian
The security descriptor is too short.
.

MessageId=8354
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEC_DESC_INVALID
Language=English
The security descriptor is invalid.
.
Language=Russian
The security descriptor is invalid.
.
Language=Polish
Deskryptor zabezpieczenia jest nieprawidłowy.
.
Language=Romanian
The security descriptor is invalid.
.

MessageId=8355
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_DELETED_NAME
Language=English
Failed to create name for deleted object.
.
Language=Russian
Failed to create name for deleted object.
.
Language=Polish
Nie można utworzyć nazwy dla usuniętego obiektu.
.
Language=Romanian
Failed to create name for deleted object.
.

MessageId=8356
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUBREF_MUST_HAVE_PARENT
Language=English
The parent of a new subref must exist.
.
Language=Russian
The parent of a new subref must exist.
.
Language=Polish
Musi istnieć rodzic nowego odwołania podrzędnego.
.
Language=Romanian
The parent of a new subref must exist.
.

MessageId=8357
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NCNAME_MUST_BE_NC
Language=English
The object must be a naming context.
.
Language=Russian
The object must be a naming context.
.
Language=Polish
Obiekt musi być kontekstem nazewnictwa.
.
Language=Romanian
The object must be a naming context.
.

MessageId=8358
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_SYSTEM_ONLY
Language=English
It is not permitted to add an attribute which is owned by the system.
.
Language=Russian
It is not permitted to add an attribute which is owned by the system.
.
Language=Polish
Nie jest dozwolone dodawanie atrybutu, który jest w posiadaniu systemu.
.
Language=Romanian
It is not permitted to add an attribute which is owned by the system.
.

MessageId=8359
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CLASS_MUST_BE_CONCRETE
Language=English
The class of the object must be structural; you cannot instantiate an abstract class.
.
Language=Russian
The class of the object must be structural; you cannot instantiate an abstract class.
.
Language=Polish
Klasa obiektu musi być strukturalna; nie możesz utworzyć wystąpienia klasy abstrakcyjnej.
.
Language=Romanian
The class of the object must be structural; you cannot instantiate an abstract class.
.

MessageId=8360
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_DMD
Language=English
The schema object could not be found.
.
Language=Russian
The schema object could not be found.
.
Language=Polish
Nie znaleziono obiekt schematu.
.
Language=Romanian
The schema object could not be found.
.

MessageId=8361
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_GUID_EXISTS
Language=English
A local object with this GUID (dead or alive) already exists.
.
Language=Russian
A local object with this GUID (dead or alive) already exists.
.
Language=Polish
Już istnieje obiekt lokalny o tym identyfikatorze GUID (aktywny lub nieaktywny).
.
Language=Romanian
A local object with this GUID (dead or alive) already exists.
.

MessageId=8362
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_ON_BACKLINK
Language=English
The operation cannot be performed on a back link.
.
Language=Russian
The operation cannot be performed on a back link.
.
Language=Polish
Operacja nie może być wykonana na odsyłaczu wstecznym.
.
Language=Romanian
The operation cannot be performed on a back link.
.

MessageId=8363
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CROSSREF_FOR_NC
Language=English
The cross reference for the specified naming context could not be found.
.
Language=Russian
The cross reference for the specified naming context could not be found.
.
Language=Polish
Nie znaleziono odwołania do podanego kontekstu nazewnictwa.
.
Language=Romanian
The cross reference for the specified naming context could not be found.
.

MessageId=8364
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SHUTTING_DOWN
Language=English
The operation could not be performed because the directory service is shutting down.
.
Language=Russian
The operation could not be performed because the directory service is shutting down.
.
Language=Polish
Operacja nie może być wykonana ponieważ usługa katalogowa jest zamykana.
.
Language=Romanian
The operation could not be performed because the directory service is shutting down.
.

MessageId=8365
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNKNOWN_OPERATION
Language=English
The directory service request is invalid.
.
Language=Russian
The directory service request is invalid.
.
Language=Polish
Żądanie usługi katalogowej jest nieprawidłowe.
.
Language=Romanian
The directory service request is invalid.
.

MessageId=8366
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_ROLE_OWNER
Language=English
The role owner attribute could not be read.
.
Language=Russian
The role owner attribute could not be read.
.
Language=Polish
Nie można odczytać atrybutu właściciela roli.
.
Language=Romanian
The role owner attribute could not be read.
.

MessageId=8367
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_CONTACT_FSMO
Language=English
The requested FSMO operation failed. The current FSMO holder could not be reached.
.
Language=Russian
The requested FSMO operation failed. The current FSMO holder could not be reached.
.
Language=Polish
Żądana operacja FSMO nie powiodła się. Nie można połączyć się z bieżącym posiadaczem FSMO.
.
Language=Romanian
The requested FSMO operation failed. The current FSMO holder could not be reached.
.

MessageId=8368
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_NC_DN_RENAME
Language=English
Modification of a DN across a naming context is not permitted.
.
Language=Russian
Modification of a DN across a naming context is not permitted.
.
Language=Polish
Modyfikowanie nazwy domen poza kontekstem nazewnictwa jest niedozwolone.
.
Language=Romanian
Modification of a DN across a naming context is not permitted.
.

MessageId=8369
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_SYSTEM_ONLY
Language=English
The attribute cannot be modified because it is owned by the system.
.
Language=Russian
The attribute cannot be modified because it is owned by the system.
.
Language=Polish
Atrybut nie może być modyfikowany, ponieważ jest w posiadaniu systemu.
.
Language=Romanian
The attribute cannot be modified because it is owned by the system.
.

MessageId=8370
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPLICATOR_ONLY
Language=English
Only the replicator can perform this function.
.
Language=Russian
Only the replicator can perform this function.
.
Language=Polish
Tylko replikator może wykonać tę funkcję.
.
Language=Romanian
Only the replicator can perform this function.
.

MessageId=8371
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_NOT_DEFINED
Language=English
The specified class is not defined.
.
Language=Russian
The specified class is not defined.
.
Language=Polish
Podana klasa nie jest zdefiniowana.
.
Language=Romanian
The specified class is not defined.
.

MessageId=8372
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OBJ_CLASS_NOT_SUBCLASS
Language=English
The specified class is not a subclass.
.
Language=Russian
The specified class is not a subclass.
.
Language=Polish
Podana klasa nie jest podklasą.
.
Language=Romanian
The specified class is not a subclass.
.

MessageId=8373
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_REFERENCE_INVALID
Language=English
The name reference is invalid.
.
Language=Russian
The name reference is invalid.
.
Language=Polish
Nazwa odwołania jest nieprawidłowa.
.
Language=Romanian
The name reference is invalid.
.

MessageId=8374
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_REF_EXISTS
Language=English
A cross reference already exists.
.
Language=Russian
A cross reference already exists.
.
Language=Polish
Odwołanie już istnieje.
.
Language=Romanian
A cross reference already exists.
.

MessageId=8375
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEL_MASTER_CROSSREF
Language=English
It is not permitted to delete a master cross reference.
.
Language=Russian
It is not permitted to delete a master cross reference.
.
Language=Polish
Usuwanie głównego odwołania jest niedozwolone.
.
Language=Romanian
It is not permitted to delete a master cross reference.
.

MessageId=8376
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUBTREE_NOTIFY_NOT_NC_HEAD
Language=English
Subtree notifications are only supported on NC heads.
.
Language=Russian
Subtree notifications are only supported on NC heads.
.
Language=Polish
Powiadomienia poddrzewa są dostarczane tylko do węzłów NC.
.
Language=Romanian
Subtree notifications are only supported on NC heads.
.

MessageId=8377
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOTIFY_FILTER_TOO_COMPLEX
Language=English
Notification filter is too complex.
.
Language=Russian
Notification filter is too complex.
.
Language=Polish
Filtr powiadamiania jest zbyt skomplikowany.
.
Language=Romanian
Notification filter is too complex.
.

MessageId=8378
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_RDN
Language=English
Schema update failed: duplicate RDN.
.
Language=Russian
Schema update failed: duplicate RDN.
.
Language=Polish
Nie można zaktualizować schematu: zduplikowana nazwa RDN.
.
Language=Romanian
Schema update failed: duplicate RDN.
.

MessageId=8379
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_OID
Language=English
Schema update failed: duplicate OID
.
Language=Russian
Schema update failed: duplicate OID
.
Language=Polish
Nie można zaktualizować schematu: zduplikowany identyfikator OID.
.
Language=Romanian
Schema update failed: duplicate OID
.

MessageId=8380
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_MAPI_ID
Language=English
Schema update failed: duplicate MAPI identifier.
.
Language=Russian
Schema update failed: duplicate MAPI identifier.
.
Language=Polish
Nie można zaktualizować schematu: zduplikowany identyfikator MAPI.
.
Language=Romanian
Schema update failed: duplicate MAPI identifier.
.

MessageId=8381
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_SCHEMA_ID_GUID
Language=English
Schema update failed: duplicate schema-id GUID.
.
Language=Russian
Schema update failed: duplicate schema-id GUID.
.
Language=Polish
Nie można zaktualizować schematu: zduplikowany identyfikator schematu GUID.
.
Language=Romanian
Schema update failed: duplicate schema-id GUID.
.

MessageId=8382
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_LDAP_DISPLAY_NAME
Language=English
Schema update failed: duplicate LDAP display name.
.
Language=Russian
Schema update failed: duplicate LDAP display name.
.
Language=Polish
Nie można zaktualizować schematu: zduplikowana wyświetlana nazwa LDAP.
.
Language=Romanian
Schema update failed: duplicate LDAP display name.
.

MessageId=8383
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SEMANTIC_ATT_TEST
Language=English
Schema update failed: range-lower less than range upper
.
Language=Russian
Schema update failed: range-lower less than range upper
.
Language=Polish
Nie można zaktualizować schematu: zakres niższy mniejszy niż zakres górny.
.
Language=Romanian
Schema update failed: range-lower less than range upper
.

MessageId=8384
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SYNTAX_MISMATCH
Language=English
Schema update failed: syntax mismatch
.
Language=Russian
Schema update failed: syntax mismatch
.
Language=Polish
Nie można zaktualizować schematu: niezgodność składni.
.
Language=Romanian
Schema update failed: syntax mismatch
.

MessageId=8385
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_MUST_HAVE
Language=English
Schema deletion failed: attribute is used in must-contain
.
Language=Russian
Schema deletion failed: attribute is used in must-contain
.
Language=Polish
Nie można zaktualizować schematu: atrybut jest używany w aspekcie „musi zawierać”.
.
Language=Romanian
Schema deletion failed: attribute is used in must-contain
.

MessageId=8386
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_MAY_HAVE
Language=English
Schema deletion failed: attribute is used in may-contain
.
Language=Russian
Schema deletion failed: attribute is used in may-contain
.
Language=Polish
Nie można zaktualizować schematu: atrybut jest używany w aspekcie „może zawierać”.
.
Language=Romanian
Schema deletion failed: attribute is used in may-contain
.

MessageId=8387
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_MAY_HAVE
Language=English
Schema update failed: attribute in may-contain does not exist
.
Language=Russian
Schema update failed: attribute in may-contain does not exist
.
Language=Polish
Nie można zaktualizować schematu: nie istnieje atrybut w aspekcie „może zawierać”.
.
Language=Romanian
Schema update failed: attribute in may-contain does not exist
.

MessageId=8388
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_MUST_HAVE
Language=English
Schema update failed: attribute in must-contain does not exist
.
Language=Russian
Schema update failed: attribute in must-contain does not exist
.
Language=Polish
Nie można zaktualizować schematu: nie istnieje atrybut w aspekcie „musi zawierać”.
.
Language=Romanian
Schema update failed: attribute in must-contain does not exist
.

MessageId=8389
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUX_CLS_TEST_FAIL
Language=English
Schema update failed: class in aux-class list does not exist or is not an auxiliary class
.
Language=Russian
Schema update failed: class in aux-class list does not exist or is not an auxiliary class
.
Language=Polish
Nie można zaktualizować schematu: klasa z listy klas pomocniczych nie istnieje lub nie jest klasą pomocniczą.
.
Language=Romanian
Schema update failed: class in aux-class list does not exist or is not an auxiliary class
.

MessageId=8390
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONEXISTENT_POSS_SUP
Language=English
Schema update failed: class in poss-superiors does not exist
.
Language=Russian
Schema update failed: class in poss-superiors does not exist
.
Language=Polish
Nie można zaktualizować schematu: klasa z listy klas zwierzchnich nie istnieje.
.
Language=Romanian
Schema update failed: class in poss-superiors does not exist
.

MessageId=8391
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SUB_CLS_TEST_FAIL
Language=English
Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules
.
Language=Russian
Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules
.
Language=Polish
Nie można zaktualizować schematu: klasa z listy podklas nie istnieje lub nie spełnia reguł hierarchii.
.
Language=Romanian
Schema update failed: class in subclassof list does not exist or does not satisfy hierarchy rules
.

MessageId=8392
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_RDN_ATT_ID_SYNTAX
Language=English
Schema update failed: Rdn-Att-Id has wrong syntax
.
Language=Russian
Schema update failed: Rdn-Att-Id has wrong syntax
.
Language=Polish
Nie można zaktualizować schematu: nieprawidłowa składnia Rdn-Att-Id.
.
Language=Romanian
Schema update failed: Rdn-Att-Id has wrong syntax
.

MessageId=8393
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_AUX_CLS
Language=English
Schema deletion failed: class is used as auxiliary class
.
Language=Russian
Schema deletion failed: class is used as auxiliary class
.
Language=Polish
Nie można usunąć schematu: klasa jest używana jako klasa pomocnicza.
.
Language=Romanian
Schema deletion failed: class is used as auxiliary class
.

MessageId=8394
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_SUB_CLS
Language=English
Schema deletion failed: class is used as sub class
.
Language=Russian
Schema deletion failed: class is used as sub class
.
Language=Polish
Nie można usunąć schematu: klasa jest używana jako podklasa.
.
Language=Romanian
Schema deletion failed: class is used as sub class
.

MessageId=8395
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_POSS_SUP
Language=English
Schema deletion failed: class is used as poss superior
.
Language=Russian
Schema deletion failed: class is used as poss superior
.
Language=Polish
Nie można usunąć schematu: klasa jest używana jako zwierzchnia.
.
Language=Romanian
Schema deletion failed: class is used as poss superior
.

MessageId=8396
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RECALCSCHEMA_FAILED
Language=English
Schema update failed in recalculating validation cache.
.
Language=Russian
Schema update failed in recalculating validation cache.
.
Language=Polish
Nie można zaktualizować schematu: błąd w przetwarzaniu pamięci podręcznej procesu sprawdzania poprawności.
.
Language=Romanian
Schema update failed in recalculating validation cache.
.

MessageId=8397
Severity=Success
Facility=System
SymbolicName=ERROR_DS_TREE_DELETE_NOT_FINISHED
Language=English
The tree deletion is not finished.
.
Language=Russian
The tree deletion is not finished.
.
Language=Polish
Usuwanie drzewa nie zostało zakończone.
.
Language=Romanian
The tree deletion is not finished.
.

MessageId=8398
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DELETE
Language=English
The requested delete operation could not be performed.
.
Language=Russian
The requested delete operation could not be performed.
.
Language=Polish
Nie można wykonać żądanej operacji usuwania.
.
Language=Romanian
The requested delete operation could not be performed.
.

MessageId=8399
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_ID
Language=English
Cannot read the governs class identifier for the schema record.
.
Language=Russian
Cannot read the governs class identifier for the schema record.
.
Language=Polish
Nie można odczytać identyfikatora klasy rządzącej dla rekordu schematu.
.
Language=Romanian
Cannot read the governs class identifier for the schema record.
.

MessageId=8400
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_ATT_SCHEMA_SYNTAX
Language=English
The attribute schema has bad syntax.
.
Language=Russian
The attribute schema has bad syntax.
.
Language=Polish
Schemat atrybutu ma złą składnię.
.
Language=Romanian
The attribute schema has bad syntax.
.

MessageId=8401
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CACHE_ATT
Language=English
The attribute could not be cached.
.
Language=Russian
The attribute could not be cached.
.
Language=Polish
Nie można umieścić atrybutu w pamięci podręcznej.
.
Language=Romanian
The attribute could not be cached.
.

MessageId=8402
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CACHE_CLASS
Language=English
The class could not be cached.
.
Language=Russian
The class could not be cached.
.
Language=Polish
Nie można umieścić klasy w pamięci podręcznej.
.
Language=Romanian
The class could not be cached.
.

MessageId=8403
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REMOVE_ATT_CACHE
Language=English
The attribute could not be removed from the cache.
.
Language=Russian
The attribute could not be removed from the cache.
.
Language=Polish
Nie można usunąć atrybutu z pamięci podręcznej.
.
Language=Romanian
The attribute could not be removed from the cache.
.

MessageId=8404
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REMOVE_CLASS_CACHE
Language=English
The class could not be removed from the cache.
.
Language=Russian
The class could not be removed from the cache.
.
Language=Polish
Nie można usunąć klasy z pamięci podręcznej.
.
Language=Romanian
The class could not be removed from the cache.
.

MessageId=8405
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_DN
Language=English
The distinguished name attribute could not be read.
.
Language=Russian
The distinguished name attribute could not be read.
.
Language=Polish
Nie można odczytać atrybutu nazwy wyróżniającej.
.
Language=Romanian
The distinguished name attribute could not be read.
.

MessageId=8406
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_SUPREF
Language=English
No superior reference has been configured for the directory service. The directory service is therefore unable to issue referrals to objects outside this forest.
.
Language=Russian
No superior reference has been configured for the directory service. The directory service is therefore unable to issue referrals to objects outside this forest.
.
Language=Polish
Nie skonfigurowano nadrzędnego odwołania usługi katalogowej. Z tego powodu usługa katalogowa nie może przydzielić odwołań obiektom poza tym lasem.
.
Language=Romanian
No superior reference has been configured for the directory service. The directory service is therefore unable to issue referrals to objects outside this forest.
.

MessageId=8407
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_INSTANCE
Language=English
The instance type attribute could not be retrieved.
.
Language=Russian
The instance type attribute could not be retrieved.
.
Language=Polish
Nie można pobrać atrybutu typu wystąpienia.
.
Language=Romanian
The instance type attribute could not be retrieved.
.

MessageId=8408
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CODE_INCONSISTENCY
Language=English
An internal error has occurred.
.
Language=Russian
An internal error has occurred.
.
Language=Polish
Wystąpił błąd wewnętrzny.
.
Language=Romanian
An internal error has occurred.
.

MessageId=8409
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DATABASE_ERROR
Language=English
A database error has occurred.
.
Language=Russian
A database error has occurred.
.
Language=Polish
Wystąpił błąd bazy danych.
.
Language=Romanian
A database error has occurred.
.

MessageId=8410
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GOVERNSID_MISSING
Language=English
The attribute GOVERNSID is missing.
.
Language=Russian
The attribute GOVERNSID is missing.
.
Language=Polish
Brakuje atrybutu GOVERNSID.
.
Language=Romanian
The attribute GOVERNSID is missing.
.

MessageId=8411
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_EXPECTED_ATT
Language=English
An expected attribute is missing.
.
Language=Russian
An expected attribute is missing.
.
Language=Polish
Brakuje oczekiwanego atrybutu.
.
Language=Romanian
An expected attribute is missing.
.

MessageId=8412
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NCNAME_MISSING_CR_REF
Language=English
The specified naming context is missing a cross reference.
.
Language=Russian
The specified naming context is missing a cross reference.
.
Language=Polish
W podanym kontekście nazewnictwa brakuje odwołania.
.
Language=Romanian
The specified naming context is missing a cross reference.
.

MessageId=8413
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SECURITY_CHECKING_ERROR
Language=English
A security checking error has occurred.
.
Language=Russian
A security checking error has occurred.
.
Language=Polish
Podczas sprawdzania zabezpieczeń wystąpił błąd.
.
Language=Romanian
A security checking error has occurred.
.

MessageId=8414
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_NOT_LOADED
Language=English
The schema is not loaded.
.
Language=Russian
The schema is not loaded.
.
Language=Polish
Schemat nie został załadowany.
.
Language=Romanian
The schema is not loaded.
.

MessageId=8415
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_ALLOC_FAILED
Language=English
Schema allocation failed. Please check if the machine is running low on memory.
.
Language=Russian
Schema allocation failed. Please check if the machine is running low on memory.
.
Language=Polish
Nie można przydzielić pamięci dla schematu. Sprawdź, czy w komputerze nie brakuje pamięci.
.
Language=Romanian
Schema allocation failed. Please check if the machine is running low on memory.
.

MessageId=8416
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ATT_SCHEMA_REQ_SYNTAX
Language=English
Failed to obtain the required syntax for the attribute schema.
.
Language=Russian
Failed to obtain the required syntax for the attribute schema.
.
Language=Polish
Nie można uzyskać wymaganej składni schematu atrybutów.
.
Language=Romanian
Failed to obtain the required syntax for the attribute schema.
.

MessageId=8417
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GCVERIFY_ERROR
Language=English
The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available.
.
Language=Russian
The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available.
.
Language=Polish
Nie można zweryfikować wykazu globalnego. Wykaz globalny nie jest dostępny lub nie obsługuje operacji. Część katalogu nie jest obecnie dostępna.
.
Language=Romanian
The global catalog verification failed. The global catalog is not available or does not support the operation. Some part of the directory is currently not available.
.

MessageId=8418
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_MISMATCH
Language=English
The replication operation failed because of a schema mismatch between the servers involved.
.
Language=Russian
The replication operation failed because of a schema mismatch between the servers involved.
.
Language=Polish
Nie można wykonać replikacji, ponieważ występuje niezgodność schematów pomiędzy serwerami biorącymi udział w operacji.
.
Language=Romanian
The replication operation failed because of a schema mismatch between the servers involved.
.

MessageId=8419
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_DSA_OBJ
Language=English
The DSA object could not be found.
.
Language=Russian
The DSA object could not be found.
.
Language=Polish
Nie można znaleźć obiektu DSA.
.
Language=Romanian
The DSA object could not be found.
.

MessageId=8420
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_EXPECTED_NC
Language=English
The naming context could not be found.
.
Language=Russian
The naming context could not be found.
.
Language=Polish
Nie można znaleźć kontekstu nazewnictwa.
.
Language=Romanian
The naming context could not be found.
.

MessageId=8421
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_NC_IN_CACHE
Language=English
The naming context could not be found in the cache.
.
Language=Russian
The naming context could not be found in the cache.
.
Language=Polish
Nie można znaleźć kontekstu nazewnictwa w pamięci podręcznej.
.
Language=Romanian
The naming context could not be found in the cache.
.

MessageId=8422
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_CHILD
Language=English
The child object could not be retrieved.
.
Language=Russian
The child object could not be retrieved.
.
Language=Polish
Nie można pobrać obiektu podrzędnego.
.
Language=Romanian
The child object could not be retrieved.
.

MessageId=8423
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SECURITY_ILLEGAL_MODIFY
Language=English
The modification was not permitted for security reasons.
.
Language=Russian
The modification was not permitted for security reasons.
.
Language=Polish
Modyfikacja nie została dozwolona z powodów bezpieczeństwa.
.
Language=Romanian
The modification was not permitted for security reasons.
.

MessageId=8424
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_REPLACE_HIDDEN_REC
Language=English
The operation cannot replace the hidden record.
.
Language=Russian
The operation cannot replace the hidden record.
.
Language=Polish
Operacja nie może zamienić ukrytego rekordu.
.
Language=Romanian
The operation cannot replace the hidden record.
.

MessageId=8425
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BAD_HIERARCHY_FILE
Language=English
The hierarchy file is invalid.
.
Language=Russian
The hierarchy file is invalid.
.
Language=Polish
Plik hierarchii jest nieprawidłowy.
.
Language=Romanian
The hierarchy file is invalid.
.

MessageId=8426
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BUILD_HIERARCHY_TABLE_FAILED
Language=English
The attempt to build the hierarchy table failed.
.
Language=Russian
The attempt to build the hierarchy table failed.
.
Language=Polish
Nie można utworzyć tablicy hierarchii.
.
Language=Romanian
The attempt to build the hierarchy table failed.
.

MessageId=8427
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONFIG_PARAM_MISSING
Language=English
The directory configuration parameter is missing from the registry.
.
Language=Russian
The directory configuration parameter is missing from the registry.
.
Language=Polish
W Rejestrze brakuje parametru konfiguracji katalogu.
.
Language=Romanian
The directory configuration parameter is missing from the registry.
.

MessageId=8428
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COUNTING_AB_INDICES_FAILED
Language=English
The attempt to count the address book indices failed.
.
Language=Russian
The attempt to count the address book indices failed.
.
Language=Polish
Nie można policzyć indeksów książki adresowej.
.
Language=Romanian
The attempt to count the address book indices failed.
.

MessageId=8429
Severity=Success
Facility=System
SymbolicName=ERROR_DS_HIERARCHY_TABLE_MALLOC_FAILED
Language=English
The allocation of the hierarchy table failed.
.
Language=Russian
The allocation of the hierarchy table failed.
.
Language=Polish
Nie można przydzielić pamięci dla tabeli hierarchii.
.
Language=Romanian
The allocation of the hierarchy table failed.
.

MessageId=8430
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INTERNAL_FAILURE
Language=English
The directory service encountered an internal failure.
.
Language=Russian
The directory service encountered an internal failure.
.
Language=Polish
Usługa katalogowa napotkała błąd wewnętrzny.
.
Language=Romanian
The directory service encountered an internal failure.
.

MessageId=8431
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNKNOWN_ERROR
Language=English
The directory service encountered an unknown failure.
.
Language=Russian
The directory service encountered an unknown failure.
.
Language=Polish
Usługa katalogowa napotkała nieznany błąd.
.
Language=Romanian
The directory service encountered an unknown failure.
.

MessageId=8432
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROOT_REQUIRES_CLASS_TOP
Language=English
A root object requires a class of 'top'.
.
Language=Russian
A root object requires a class of 'top'.
.
Language=Polish
Obiekt główny wymaga klasy „top”.
.
Language=Romanian
A root object requires a class of 'top'.
.

MessageId=8433
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REFUSING_FSMO_ROLES
Language=English
This directory server is shutting down, and cannot take ownership of new floating single-master operation roles.
.
Language=Russian
This directory server is shutting down, and cannot take ownership of new floating single-master operation roles.
.
Language=Polish
Serwer katalogu jest zamykany i nie może przejąć w posiadanie nowych zmiennych prostych operacji głównych (FSMO).
.
Language=Romanian
This directory server is shutting down, and cannot take ownership of new floating single-master operation roles.
.

MessageId=8434
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_FSMO_SETTINGS
Language=English
The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles.
.
Language=Russian
The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles.
.
Language=Polish
Usłudze katalogowej brakuje koniecznych informacji o konfiguracji - nie można określić posiadaczy zmiennych prostych operacji głównych (FSMO).
.
Language=Romanian
The directory service is missing mandatory configuration information, and is unable to determine the ownership of floating single-master operation roles.
.

MessageId=8435
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNABLE_TO_SURRENDER_ROLES
Language=English
The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers.
.
Language=Russian
The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers.
.
Language=Polish
Usługa katalogowa nie może przetransferować do innych serwerów praw własności do zmiennych prostych operacji głównych (FSMO).
.
Language=Romanian
The directory service was unable to transfer ownership of one or more floating single-master operation roles to other servers.
.

MessageId=8436
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_GENERIC
Language=English
The replication operation failed.
.
Language=Russian
The replication operation failed.
.
Language=Polish
Operacja replikacji nie powiodła się.
.
Language=Romanian
The replication operation failed.
.

MessageId=8437
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INVALID_PARAMETER
Language=English
An invalid parameter was specified for this replication operation.
.
Language=Russian
An invalid parameter was specified for this replication operation.
.
Language=Polish
Podano nieprawidłowy parametr dla tej operacji replikacji.
.
Language=Romanian
An invalid parameter was specified for this replication operation.
.

MessageId=8438
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BUSY
Language=English
The directory service is too busy to complete the replication operation at this time.
.
Language=Russian
The directory service is too busy to complete the replication operation at this time.
.
Language=Polish
Usługa katalogowa jest zbyt zajęta, aby dokończyć teraz operację replikacji.
.
Language=Romanian
The directory service is too busy to complete the replication operation at this time.
.

MessageId=8439
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_DN
Language=English
The distinguished name specified for this replication operation is invalid.
.
Language=Russian
The distinguished name specified for this replication operation is invalid.
.
Language=Polish
Nazwa wyróżniająca, podana dla tej operacji replikacji, jest nieprawidłowa.
.
Language=Romanian
The distinguished name specified for this replication operation is invalid.
.

MessageId=8440
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_NC
Language=English
The naming context specified for this replication operation is invalid.
.
Language=Russian
The naming context specified for this replication operation is invalid.
.
Language=Polish
Kontekst nazewnictwa, podany dla tej operacji replikacji, jest nieprawidłowy.
.
Language=Romanian
The naming context specified for this replication operation is invalid.
.

MessageId=8441
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_DN_EXISTS
Language=English
The distinguished name specified for this replication operation already exists.
.
Language=Russian
The distinguished name specified for this replication operation already exists.
.
Language=Polish
Nazwa wyróżniająca, podana dla tej operacji replikacji, już istnieje.
.
Language=Romanian
The distinguished name specified for this replication operation already exists.
.

MessageId=8442
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INTERNAL_ERROR
Language=English
The replication system encountered an internal error.
.
Language=Russian
The replication system encountered an internal error.
.
Language=Polish
System replikacji napotkał błąd wewnętrzny.
.
Language=Romanian
The replication system encountered an internal error.
.

MessageId=8443
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INCONSISTENT_DIT
Language=English
The replication operation encountered a database inconsistency.
.
Language=Russian
The replication operation encountered a database inconsistency.
.
Language=Polish
Podczas operacji replikacji wykryto niespójność bazy danych.
.
Language=Romanian
The replication operation encountered a database inconsistency.
.

MessageId=8444
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_CONNECTION_FAILED
Language=English
The server specified for this replication operation could not be contacted.
.
Language=Russian
The server specified for this replication operation could not be contacted.
.
Language=Polish
Nie można skontaktować się z serwerem podanym dla tej operacji replikacji.
.
Language=Romanian
The server specified for this replication operation could not be contacted.
.

MessageId=8445
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_BAD_INSTANCE_TYPE
Language=English
The replication operation encountered an object with an invalid instance type.
.
Language=Russian
The replication operation encountered an object with an invalid instance type.
.
Language=Polish
Operacja replikacji napotkała obiekt z nieprawidłowym typem wystąpienia.
.
Language=Romanian
The replication operation encountered an object with an invalid instance type.
.

MessageId=8446
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OUT_OF_MEM
Language=English
The replication operation failed to allocate memory.
.
Language=Russian
The replication operation failed to allocate memory.
.
Language=Polish
Nie można przydzielić pamięci dla operacji replikacji.
.
Language=Romanian
The replication operation failed to allocate memory.
.

MessageId=8447
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_MAIL_PROBLEM
Language=English
The replication operation encountered an error with the mail system.
.
Language=Russian
The replication operation encountered an error with the mail system.
.
Language=Polish
Operacja replikacji napotkała błąd systemu poczty.
.
Language=Romanian
The replication operation encountered an error with the mail system.
.

MessageId=8448
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REF_ALREADY_EXISTS
Language=English
The replication reference information for the target server already exists.
.
Language=Russian
The replication reference information for the target server already exists.
.
Language=Polish
Informacja dla serwera docelowego o odniesieniu replikacji już istnieje.
.
Language=Romanian
The replication reference information for the target server already exists.
.

MessageId=8449
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REF_NOT_FOUND
Language=English
The replication reference information for the target server does not exist.
.
Language=Russian
The replication reference information for the target server does not exist.
.
Language=Polish
Informacja dla serwera docelowego o odniesieniu replikacji nie istnieje.
.
Language=Romanian
The replication reference information for the target server does not exist.
.

MessageId=8450
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OBJ_IS_REP_SOURCE
Language=English
The naming context cannot be removed because it is replicated to another server.
.
Language=Russian
The naming context cannot be removed because it is replicated to another server.
.
Language=Polish
Kontekst nazewnictwa nie może być usunięty, ponieważ jest replikowany do innego serwera.
.
Language=Romanian
The naming context cannot be removed because it is replicated to another server.
.

MessageId=8451
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_DB_ERROR
Language=English
The replication operation encountered a database error.
.
Language=Russian
The replication operation encountered a database error.
.
Language=Polish
Operacja replikacji napotkała błąd bazy danych.
.
Language=Romanian
The replication operation encountered a database error.
.

MessageId=8452
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NO_REPLICA
Language=English
The naming context is in the process of being removed or is not replicated from the specified server.
.
Language=Russian
The naming context is in the process of being removed or is not replicated from the specified server.
.
Language=Polish
Kontekst nazewnictwa jest właśnie usuwany lub nie jest replikowany z podanego serwera.
.
Language=Romanian
The naming context is in the process of being removed or is not replicated from the specified server.
.

MessageId=8453
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_ACCESS_DENIED
Language=English
Replication access was denied.
.
Language=Russian
Replication access was denied.
.
Language=Polish
Odmówiono dostępu dla replikacji.
.
Language=Romanian
Replication access was denied.
.

MessageId=8454
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NOT_SUPPORTED
Language=English
The requested operation is not supported by this version of the directory service.
.
Language=Russian
The requested operation is not supported by this version of the directory service.
.
Language=Polish
Żądana operacja nie jest obsługiwana przez tą wersję usługi katalogowej.
.
Language=Romanian
The requested operation is not supported by this version of the directory service.
.

MessageId=8455
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_RPC_CANCELLED
Language=English
The replication remote procedure call was cancelled.
.
Language=Russian
The replication remote procedure call was cancelled.
.
Language=Polish
Zdalne wywołanie procedury replikacji zostało anulowane.
.
Language=Romanian
The replication remote procedure call was cancelled.
.

MessageId=8456
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_DISABLED
Language=English
The source server is currently rejecting replication requests.
.
Language=Russian
The source server is currently rejecting replication requests.
.
Language=Polish
Serwer źródłowy obecnie odrzuca żądania replikacji.
.
Language=Romanian
The source server is currently rejecting replication requests.
.

MessageId=8457
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SINK_DISABLED
Language=English
The destination server is currently rejecting replication requests.
.
Language=Russian
The destination server is currently rejecting replication requests.
.
Language=Polish
Serwer docelowy obecnie odrzuca żądania replikacji.
.
Language=Romanian
The destination server is currently rejecting replication requests.
.

MessageId=8458
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_NAME_COLLISION
Language=English
The replication operation failed due to a collision of object names.
.
Language=Russian
The replication operation failed due to a collision of object names.
.
Language=Polish
Nie można wykonać operacji replikacji z powodu konfliktu nazw obiektów.
.
Language=Romanian
The replication operation failed due to a collision of object names.
.

MessageId=8459
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_REINSTALLED
Language=English
The replication source has been reinstalled.
.
Language=Russian
The replication source has been reinstalled.
.
Language=Polish
Źródło replikacji zostało ponownie zainstalowane.
.
Language=Romanian
The replication source has been reinstalled.
.

MessageId=8460
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_MISSING_PARENT
Language=English
The replication operation failed because a required parent object is missing.
.
Language=Russian
The replication operation failed because a required parent object is missing.
.
Language=Polish
Nie można wykonać operacji replikacji, ponieważ brakuje obiektu nadrzędnego.
.
Language=Romanian
The replication operation failed because a required parent object is missing.
.

MessageId=8461
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_PREEMPTED
Language=English
The replication operation was preempted.
.
Language=Russian
The replication operation was preempted.
.
Language=Polish
Operacja replikacji została udaremniona.
.
Language=Romanian
The replication operation was preempted.
.

MessageId=8462
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_ABANDON_SYNC
Language=English
The replication synchronization attempt was abandoned because of a lack of updates.
.
Language=Russian
The replication synchronization attempt was abandoned because of a lack of updates.
.
Language=Polish
Próba synchronizacji replikacji została zaniechana z powodu braku aktualizacji.
.
Language=Romanian
The replication synchronization attempt was abandoned because of a lack of updates.
.

MessageId=8463
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SHUTDOWN
Language=English
The replication operation was terminated because the system is shutting down.
.
Language=Russian
The replication operation was terminated because the system is shutting down.
.
Language=Polish
Operacja replikacji została przerwana, ponieważ system jest zamykany.
.
Language=Romanian
The replication operation was terminated because the system is shutting down.
.

MessageId=8464
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET
Language=English
Synchronization attempt failed because the destination DC is currently waiting to synchronize new partial attributes from source. This condition is normal if a recent schema change modified the partial attribute set. The destination partial attribute set is not a subset of the source partial attribute set.
.
Language=Russian
Synchronization attempt failed because the destination DC is currently waiting to synchronize new partial attributes from source. This condition is normal if a recent schema change modified the partial attribute set. The destination partial attribute set is not a subset of the source partial attribute set.
.
Language=Polish
Próba synchronizacji nie powiodła się, ponieważ docelowy kontroler domeny oczekuje na synchronizację nowych atrybutów częściowych ze źródła. Ta sytuacja jest normalna, jeśli bieżąca zmiana schematu zmodyfikowała zbiór częściowy atrybutów. Zbiór częściowy atrybutów docelowych nie jest podzbiorem zbioru częściowego atrybutów źródłowych.
.
Language=Romanian
Synchronization attempt failed because the destination DC is currently waiting to synchronize new partial attributes from source. This condition is normal if a recent schema change modified the partial attribute set. The destination partial attribute set is not a subset of the source partial attribute set.
.

MessageId=8465
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA
Language=English
The replication synchronization attempt failed because a master replica attempted to sync from a partial replica.
.
Language=Russian
The replication synchronization attempt failed because a master replica attempted to sync from a partial replica.
.
Language=Polish
Próba synchronizacji replikacji nie powiodła się, ponieważ główna replika próbowała zsynchronizować się z repliką częściową.
.
Language=Romanian
The replication synchronization attempt failed because a master replica attempted to sync from a partial replica.
.

MessageId=8466
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_EXTN_CONNECTION_FAILED
Language=English
The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation.
.
Language=Russian
The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation.
.
Language=Polish
Skontaktowano się z serwerem podanym dla tej operacji replikacji, ale serwer ten nie zdołał się połączyć z dodatkowym serwerem potrzebnym do ukończenia operacji.
.
Language=Romanian
The server specified for this replication operation was contacted, but that server was unable to contact an additional server needed to complete the operation.
.

MessageId=8467
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_SCHEMA_MISMATCH
Language=English
The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer.
.
Language=Russian
The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer.
.
Language=Polish
Wersja schematu usługi katalogowej lasu źródłowego jest niezgodna z wersją usługi katalogowej na tym komputerze.
.
Language=Romanian
The version of the Active Directory schema of the source forest is not compatible with the version of Active Directory on this computer.
.

MessageId=8468
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_LINK_ID
Language=English
Schema update failed: An attribute with the same link identifier already exists.
.
Language=Russian
Schema update failed: An attribute with the same link identifier already exists.
.
Language=Polish
Nie można zaktualizować schematu: atrybut z tym samym identyfikatorem łącza już istnieje.
.
Language=Romanian
Schema update failed: An attribute with the same link identifier already exists.
.

MessageId=8469
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_RESOLVING
Language=English
Name translation: Generic processing error.
.
Language=Russian
Name translation: Generic processing error.
.
Language=Polish
Tłumaczenie nazw: ogólny błąd przetwarzania.
.
Language=Romanian
Name translation: Generic processing error.
.

MessageId=8470
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NOT_FOUND
Language=English
Name translation: Could not find the name or insufficient right to see name.
.
Language=Russian
Name translation: Could not find the name or insufficient right to see name.
.
Language=Polish
Tłumaczenie nazw: nie można znaleźć nazwy lub brak wystarczających uprawnień, aby ją zobaczyć.
.
Language=Romanian
Name translation: Could not find the name or insufficient right to see name.
.

MessageId=8471
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NOT_UNIQUE
Language=English
Name translation: Input name mapped to more than one output name.
.
Language=Russian
Name translation: Input name mapped to more than one output name.
.
Language=Polish
Tłumaczenie nazw: nazwa wejściowa mapowana na więcej niż jedną nazwę wynikową.
.
Language=Romanian
Name translation: Input name mapped to more than one output name.
.

MessageId=8472
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NO_MAPPING
Language=English
Name translation: Input name found, but not the associated output format.
.
Language=Russian
Name translation: Input name found, but not the associated output format.
.
Language=Polish
Tłumaczenie nazw: znaleziono nazwę wejściową, lecz nie znaleziono skojarzonego formatu nazwy wynikowej.
.
Language=Romanian
Name translation: Input name found, but not the associated output format.
.

MessageId=8473
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_DOMAIN_ONLY
Language=English
Name translation: Unable to resolve completely, only the domain was found.
.
Language=Russian
Name translation: Unable to resolve completely, only the domain was found.
.
Language=Polish
Tłumaczenie nazw: nie można w pełni rozpoznać - znaleziono tylko domenę.
.
Language=Romanian
Name translation: Unable to resolve completely, only the domain was found.
.

MessageId=8474
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING
Language=English
Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire.
.
Language=Russian
Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire.
.
Language=Polish
Tłumaczenie nazw: nie można przeprowadzić czysto syntaktycznego odwzorowania u klienta nie mając połączenia z siecią.
.
Language=Romanian
Name translation: Unable to perform purely syntactical mapping at the client without going out to the wire.
.

MessageId=8475
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CONSTRUCTED_ATT_MOD
Language=English
Modification of a constructed attribute is not allowed.
.
Language=Russian
Modification of a constructed attribute is not allowed.
.
Language=Polish
Modyfikowanie złożonego atrybutu jest niedozwolone.
.
Language=Romanian
Modification of a constructed attribute is not allowed.
.

MessageId=8476
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WRONG_OM_OBJ_CLASS
Language=English
The OM-Object-Class specified is incorrect for an attribute with the specified syntax.
.
Language=Russian
The OM-Object-Class specified is incorrect for an attribute with the specified syntax.
.
Language=Polish
Podany ciąg OM-Obiekt-Klasa jest niepoprawny dla atrybutu o podanej składni.
.
Language=Romanian
The OM-Object-Class specified is incorrect for an attribute with the specified syntax.
.

MessageId=8477
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_REPL_PENDING
Language=English
The replication request has been posted; waiting for reply.
.
Language=Russian
The replication request has been posted; waiting for reply.
.
Language=Polish
Żądanie replikacji zostało wysłane; oczekiwanie na odpowiedź.
.
Language=Romanian
The replication request has been posted; waiting for reply.
.

MessageId=8478
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DS_REQUIRED
Language=English
The requested operation requires a directory service, and none was available.
.
Language=Russian
The requested operation requires a directory service, and none was available.
.
Language=Polish
Żądana operacja wymaga usługi katalogowej, ale żadna usługa katalogowa nie jest dostępna.
.
Language=Romanian
The requested operation requires a directory service, and none was available.
.

MessageId=8479
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_LDAP_DISPLAY_NAME
Language=English
The LDAP display name of the class or attribute contains non-ASCII characters.
.
Language=Russian
The LDAP display name of the class or attribute contains non-ASCII characters.
.
Language=Polish
Wyświetlana nazwa LDAP danej klasy lub atrybutu zawiera znaki inne niż ASCII.
.
Language=Romanian
The LDAP display name of the class or attribute contains non-ASCII characters.
.

MessageId=8480
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NON_BASE_SEARCH
Language=English
The requested search operation is only supported for base searches.
.
Language=Russian
The requested search operation is only supported for base searches.
.
Language=Polish
Żądana operacja wyszukiwania jest obsługiwana tylko dla przeszukiwań bazowych.
.
Language=Romanian
The requested search operation is only supported for base searches.
.

MessageId=8481
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_ATTS
Language=English
The search failed to retrieve attributes from the database.
.
Language=Russian
The search failed to retrieve attributes from the database.
.
Language=Polish
Operacja wyszukiwania nie może pobrać atrybutów z bazy danych.
.
Language=Romanian
The search failed to retrieve attributes from the database.
.

MessageId=8482
Severity=Success
Facility=System
SymbolicName=ERROR_DS_BACKLINK_WITHOUT_LINK
Language=English
The schema update operation tried to add a backward link attribute that has no corresponding forward link.
.
Language=Russian
The schema update operation tried to add a backward link attribute that has no corresponding forward link.
.
Language=Polish
Operacja zaktualizowania schematu próbowała dodać atrybut odsyłacza wstecznego, któremu nie towarzyszy odsyłacz do przodu.
.
Language=Romanian
The schema update operation tried to add a backward link attribute that has no corresponding forward link.
.

MessageId=8483
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EPOCH_MISMATCH
Language=English
Source and destination of a cross domain move do not agree on the object's epoch number. Either source or destination does not have the latest version of the object.
.
Language=Russian
Source and destination of a cross domain move do not agree on the object's epoch number. Either source or destination does not have the latest version of the object.
.
Language=Polish
Źródło i miejsce docelowe operacji przenoszenia poza domenę nie mogą uzgodnić numeru epoki obiektu. Albo źródło, albo miejsce docelowe nie ma najnowszej wersji obiektu.
.
Language=Romanian
Source and destination of a cross domain move do not agree on the object's epoch number. Either source or destination does not have the latest version of the object.
.

MessageId=8484
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_NAME_MISMATCH
Language=English
Source and destination of a cross domain move do not agree on the object's current name. Either source or destination does not have the latest version of the object.
.
Language=Russian
Source and destination of a cross domain move do not agree on the object's current name. Either source or destination does not have the latest version of the object.
.
Language=Polish
Źródło i miejsce docelowe operacji przenoszenia poza domenę nie mogą uzgodnić aktualnej nazwy obiektu. Albo źródło, albo miejsce docelowe nie ma najnowszej wersji obiektu.
.
Language=Romanian
Source and destination of a cross domain move do not agree on the object's current name. Either source or destination does not have the latest version of the object.
.

MessageId=8485
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_AND_DST_NC_IDENTICAL
Language=English
Source and destination of a cross domain move operation are identical. Caller should use local move operation instead of cross domain move operation.
.
Language=Russian
Source and destination of a cross domain move operation are identical. Caller should use local move operation instead of cross domain move operation.
.
Language=Polish
Źródło i miejsce docelowe operacji przenoszenia poza domenę są identyczne. Wywołujący powinien użyć operacji przenoszenia lokalnego zamiast operacji przenoszenia poza domenę.
.
Language=Romanian
Source and destination of a cross domain move operation are identical. Caller should use local move operation instead of cross domain move operation.
.

MessageId=8486
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DST_NC_MISMATCH
Language=English
Source and destination for a cross domain move are not in agreement on the naming contexts in the forest. Either source or destination does not have the latest version of the Partitions container.
.
Language=Russian
Source and destination for a cross domain move are not in agreement on the naming contexts in the forest. Either source or destination does not have the latest version of the Partitions container.
.
Language=Polish
Źródło i miejsce docelowe operacji przenoszenia poza domenę nie mogą uzgodnić kontekstów nazewnictwa w lesie. Albo źródło, albo miejsce docelowe nie ma najnowszej wersji kontenera partycji.
.
Language=Romanian
Source and destination for a cross domain move are not in agreement on the naming contexts in the forest. Either source or destination does not have the latest version of the Partitions container.
.

MessageId=8487
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_AUTHORITIVE_FOR_DST_NC
Language=English
Destination of a cross domain move is not authoritative for the destination naming context.
.
Language=Russian
Destination of a cross domain move is not authoritative for the destination naming context.
.
Language=Polish
Miejsce docelowe operacji przenoszenia poza domenę nie jest autorytatywne dla docelowego kontekstu nazewnictwa.
.
Language=Romanian
Destination of a cross domain move is not authoritative for the destination naming context.
.

MessageId=8488
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_GUID_MISMATCH
Language=English
Source and destination of a cross domain move do not agree on the identity of the source object. Either source or destination does not have the latest version of the source object.
.
Language=Russian
Source and destination of a cross domain move do not agree on the identity of the source object. Either source or destination does not have the latest version of the source object.
.
Language=Polish
Źródło i miejsce docelowe operacji przenoszenia poza domenę nie mogą uzgodnić tożsamości obiektu źródłowego. Albo źródło, albo miejsce docelowe nie ma najnowszej wersji obiektu źródłowego.
.
Language=Romanian
Source and destination of a cross domain move do not agree on the identity of the source object. Either source or destination does not have the latest version of the source object.
.

MessageId=8489
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_DELETED_OBJECT
Language=English
Object being moved across domains is already known to be deleted by the destination server. The source server does not have the latest version of the source object.
.
Language=Russian
Object being moved across domains is already known to be deleted by the destination server. The source server does not have the latest version of the source object.
.
Language=Polish
Obiekt przenoszony poza domenę jest zaznaczony do usunięcia przez serwer docelowy. Serwer źródłowy nie ma najnowszej wersji obiektu źródłowego.
.
Language=Romanian
Object being moved across domains is already known to be deleted by the destination server. The source server does not have the latest version of the source object.
.

MessageId=8490
Severity=Success
Facility=System
SymbolicName=ERROR_DS_PDC_OPERATION_IN_PROGRESS
Language=English
Another operation, which requires exclusive access to the PDC PSMO, is already in progress.
.
Language=Russian
Another operation, which requires exclusive access to the PDC PSMO, is already in progress.
.
Language=Polish
Inna operacja, która wymaga wyłącznego dostępu do PSMO PDC jest już wykonywana.
.
Language=Romanian
Another operation, which requires exclusive access to the PDC PSMO, is already in progress.
.

MessageId=8491
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD
Language=English
A cross domain move operation failed such that the two versions of the moved object exist - one each in the source and destination domains. The destination object needs to be removed to restore the system to a consistent state.
.
Language=Russian
A cross domain move operation failed such that the two versions of the moved object exist - one each in the source and destination domains. The destination object needs to be removed to restore the system to a consistent state.
.
Language=Polish
Operacja przenoszenia poza domenę nie powiodła się, ponieważ istnieją już dwie takie wersje przenoszonych obiektów - po jednym w domenie źródłowej i docelowej. Obiekt docelowy musi być usunięty, aby został przywrócony spójny stan systemu.
.
Language=Romanian
A cross domain move operation failed such that the two versions of the moved object exist - one each in the source and destination domains. The destination object needs to be removed to restore the system to a consistent state.
.

MessageId=8492
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION
Language=English
This object may not be moved across domain boundaries either because cross domain moves for this class are disallowed, or the object has some special characteristics, e.g.: trust account or restricted RID, which prevent its move.
.
Language=Russian
This object may not be moved across domain boundaries either because cross domain moves for this class are disallowed, or the object has some special characteristics, e.g.: trust account or restricted RID, which prevent its move.
.
Language=Polish
Nie można przenieść tego obiektu poza granice domeny, ponieważ przenoszenie poza domenę nie jest dozwolone dla tej klasy albo obiekt ma pewne specjalne właściwości, takie jak konto zaufania czy ograniczony RID, uniemożliwiające przeniesienie.
.
Language=Romanian
This object may not be moved across domain boundaries either because cross domain moves for this class are disallowed, or the object has some special characteristics, e.g.: trust account or restricted RID, which prevent its move.
.

MessageId=8493
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS
Language=English
Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group. Remove the object from any account group memberships and retry.
.
Language=Russian
Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group. Remove the object from any account group memberships and retry.
.
Language=Polish
Nie można przenieść obiektów wraz z członkostwem poza granice domeny, ponieważ naruszyłoby to warunki członkostwa grupy kont. Anuluj członkostwo obiektu we wszystkich grupach kont, po czym ponów próbę.
.
Language=Romanian
Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group. Remove the object from any account group memberships and retry.
.

MessageId=8494
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NC_MUST_HAVE_NC_PARENT
Language=English
A naming context head must be the immediate child of another naming context head, not of an interior node.
.
Language=Russian
A naming context head must be the immediate child of another naming context head, not of an interior node.
.
Language=Polish
Początek kontekstu nazewnictwa musi być bezpośrednim potomkiem innego początku kontekstu nazewnictwa, a nie potomkiem wewnętrznego węzła.
.
Language=Romanian
A naming context head must be the immediate child of another naming context head, not of an interior node.
.

MessageId=8495
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE
Language=English
The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context. Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters)
.
Language=Russian
The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context. Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters)
.
Language=Polish
Katalog nie może sprawdzać poprawności nazwy z proponowanego kontekstu nazewnictwa, ponieważ nie zawiera repliki kontekstu nazewnictwa ponad proponowanym kontekstem nazewnictwa. Upewnij się, że rola wzorca operacji nazw domen jest pełniona przez serwer, który jest skonfigurowany jako serwer wykazu globalnego, i serwer ten jest zaktualizowany względem jego partnerów replikacji. (Ma zastosowanie tylko dla wzorców nazw domen systemu Windows 2000)
.
Language=Romanian
The directory cannot validate the proposed naming context name because it does not hold a replica of the naming context above the proposed naming context. Please ensure that the domain naming master role is held by a server that is configured as a global catalog server, and that the server is up to date with its replication partners. (Applies only to Windows 2000 Domain Naming masters)
.

MessageId=8496
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DST_DOMAIN_NOT_NATIVE
Language=English
Destination domain must be in native mode.
.
Language=Russian
Destination domain must be in native mode.
.
Language=Polish
Domena docelowa musi być w trybie macierzystym.
.
Language=Romanian
Destination domain must be in native mode.
.

MessageId=8497
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER
Language=English
The operation cannot be performed because the server does not have an infrastructure container in the domain of interest.
.
Language=Russian
The operation cannot be performed because the server does not have an infrastructure container in the domain of interest.
.
Language=Polish
Nie można wykonać operacji, ponieważ serwer nie ma kontenera infrastruktury w domenie zainteresowań.
.
Language=Romanian
The operation cannot be performed because the server does not have an infrastructure container in the domain of interest.
.

MessageId=8498
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_ACCOUNT_GROUP
Language=English
Cross-domain move of non-empty account groups is not allowed.
.
Language=Russian
Cross-domain move of non-empty account groups is not allowed.
.
Language=Polish
Przenoszenie niepustych grup kont poza granice domeny nie jest dozwolone.
.
Language=Romanian
Cross-domain move of non-empty account groups is not allowed.
.

MessageId=8499
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_RESOURCE_GROUP
Language=English
Cross-domain move of non-empty resource groups is not allowed.
.
Language=Russian
Cross-domain move of non-empty resource groups is not allowed.
.
Language=Polish
Przenoszenie niepustych grup zasobów poza granice domeny nie jest dozwolone.
.
Language=Romanian
Cross-domain move of non-empty resource groups is not allowed.
.

MessageId=8500
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_SEARCH_FLAG
Language=English
The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings.
.
Language=Russian
The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings.
.
Language=Polish
Flagi wyszukiwania dla atrybutów są nieprawidłowe. Bit ANR jest ważny tylko dla atrybutów Unicode lub łańcuchów Teletekstu.
.
Language=Romanian
The search flags for the attribute are invalid. The ANR bit is valid only on attributes of Unicode or Teletex strings.
.

MessageId=8501
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_TREE_DELETE_ABOVE_NC
Language=English
Tree deletions starting at an object which has an NC head as a descendant are not allowed.
.
Language=Russian
Tree deletions starting at an object which has an NC head as a descendant are not allowed.
.
Language=Polish
Usuwanie drzewa poczynając od obiektu-potomka będącego węzłem NC nie jest dozwolone.
.
Language=Romanian
Tree deletions starting at an object which has an NC head as a descendant are not allowed.
.

MessageId=8502
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_LOCK_TREE_FOR_DELETE
Language=English
The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use.
.
Language=Russian
The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use.
.
Language=Polish
Usługa katalogowa, przygotowując operację drzewa, nie mogła zablokować drzewa, ponieważ było ono wówczas używane.
.
Language=Romanian
The directory service failed to lock a tree in preparation for a tree deletion because the tree was in use.
.

MessageId=8503
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_IDENTIFY_OBJECTS_FOR_TREE_DELETE
Language=English
The directory service failed to identify the list of objects to delete while attempting a tree deletion.
.
Language=Russian
The directory service failed to identify the list of objects to delete while attempting a tree deletion.
.
Language=Polish
Usłudze katalogowej nie udało się zidentyfikować listy obiektów do usunięcia podczas próby usuwania drzewa.
.
Language=Romanian
The directory service failed to identify the list of objects to delete while attempting a tree deletion.
.

MessageId=8504
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_INIT_FAILURE
Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Directory Services Restore Mode. Check the event log for detailed information.
.
Language=Russian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Directory Services Restore Mode. Check the event log for detailed information.
.
Language=Polish
Nie można zainicjować Menedżera kont zabezpieczeń z powodu następującego błędu: %1.
Stan błędu: 0x%2. Zamknij system i uruchom go w trybie odtwarzania usługi katalogowej. Szczegółowe informacje można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Directory Services Restore Mode. Check the event log for detailed information.
.

MessageId=8505
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SENSITIVE_GROUP_VIOLATION
Language=English
Only an administrator can modify the membership list of an administrative group.
.
Language=Russian
Only an administrator can modify the membership list of an administrative group.
.
Language=Polish
Tylko administrator może modyfikować listę członków grupy administrującej.
.
Language=Romanian
Only an administrator can modify the membership list of an administrative group.
.

MessageId=8506
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOD_PRIMARYGROUPID
Language=English
Cannot change the primary group ID of a domain controller account.
.
Language=Russian
Cannot change the primary group ID of a domain controller account.
.
Language=Polish
Nie można zmienić identyfikatora grupy podstawowej konta kontrolera domeny.
.
Language=Romanian
Cannot change the primary group ID of a domain controller account.
.

MessageId=8507
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD
Language=English
An attempt is made to modify the base schema.
.
Language=Russian
An attempt is made to modify the base schema.
.
Language=Polish
Została podjęta próba zmodyfikowania schematu podstawowego.
.
Language=Romanian
An attempt is made to modify the base schema.
.

MessageId=8508
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NONSAFE_SCHEMA_CHANGE
Language=English
Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed.
.
Language=Russian
Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed.
.
Language=Polish
Nie jest dozwolone: dodawanie do istniejącej klasy nowego obowiązkowego atrybutu, usuwanie obowiązkowego atrybutu z istniejącej klasy lub dodawanie opcjonalnego atrybutu do specjalnej klasy Top, który nie jest atrybutem odsyłacza wstecznego (bezpośrednio lub wskutek dziedziczenia, na przykład przez dodanie lub usunięcie klasy pomocniczej).
.
Language=Romanian
Adding a new mandatory attribute to an existing class, deleting a mandatory attribute from an existing class, or adding an optional attribute to the special class Top that is not a backlink attribute (directly or through inheritance, for example, by adding or deleting an auxiliary class) is not allowed.
.

MessageId=8509
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SCHEMA_UPDATE_DISALLOWED
Language=English
Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner.
.
Language=Russian
Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner.
.
Language=Polish
Nie można zaktualizować schematu w bieżącym kontrolerze domeny, ponieważ kontroler domeny nie jest właścicielem schematu ról FSMO.
.
Language=Romanian
Schema update is not allowed on this DC because the DC is not the schema FSMO Role Owner.
.

MessageId=8510
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CREATE_UNDER_SCHEMA
Language=English
An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container.
.
Language=Russian
An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container.
.
Language=Polish
Obiektu tej klasy nie można utworzyć w kontenerze schematów. W kontenerze schematów możesz utworzyć tylko obiekty typu schemat atrybutów i schemat klas.
.
Language=Romanian
An object of this class cannot be created under the schema container. You can only create attribute-schema and class-schema objects under the schema container.
.

MessageId=8511
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_NO_SRC_SCH_VERSION
Language=English
The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it.
.
Language=Russian
The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it.
.
Language=Polish
Instalacja repliki/obiektu potomnego nie mogła pobrać atrybutu wersji obiektu z kontenera schematów ze źródłowego kontrolera domeny. Albo brakuje atrybutu w kontenerze schematów, albo dostarczone poświadczenia nie nadają uprawnień odczytu.
.
Language=Romanian
The replica/child install failed to get the objectVersion attribute on the schema container on the source DC. Either the attribute is missing on the schema container or the credentials supplied do not have permission to read it.
.

MessageId=8512
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE
Language=English
The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory.
.
Language=Russian
The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory.
.
Language=Polish
Instalacja repliki/obiektu potomnego nie mogła odczytać atrybutu wersji obiektu z sekcji SCHEMA pliku schema.ini znajdującego się w katalogu system32.
.
Language=Romanian
The replica/child install failed to read the objectVersion attribute in the SCHEMA section of the file schema.ini in the system32 directory.
.

MessageId=8513
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_GROUP_TYPE
Language=English
The specified group type is invalid.
.
Language=Russian
The specified group type is invalid.
.
Language=Polish
Określony typ grupy jest nieprawidłowy.
.
Language=Romanian
The specified group type is invalid.
.

MessageId=8514
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN
Language=English
Cannot nest global groups in a mixed domain if the group is security-enabled.
.
Language=Russian
Cannot nest global groups in a mixed domain if the group is security-enabled.
.
Language=Polish
Nie można zagnieździć grup globalnych w domenie mieszanej, jeżeli zabezpieczenie grup jest włączone.
.
Language=Romanian
Cannot nest global groups in a mixed domain if the group is security-enabled.
.

MessageId=8515
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN
Language=English
Cannot nest local groups in a mixed domain if the group is security-enabled.
.
Language=Russian
Cannot nest local groups in a mixed domain if the group is security-enabled.
.
Language=Polish
Nie można zagnieździć grup lokalnych w domenie mieszanej, jeżeli zabezpieczenie grup jest włączone.
.
Language=Romanian
Cannot nest local groups in a mixed domain if the group is security-enabled.
.

MessageId=8516
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER
Language=English
A global group cannot have a local group as a member.
.
Language=Russian
A global group cannot have a local group as a member.
.
Language=Polish
Grupa lokalna nie może być członkiem grupy globalnej.
.
Language=Romanian
A global group cannot have a local group as a member.
.

MessageId=8517
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER
Language=English
A global group cannot have a universal group as a member.
.
Language=Russian
A global group cannot have a universal group as a member.
.
Language=Polish
Grupa uniwersalna nie może być członkiem grupy globalnej.
.
Language=Romanian
A global group cannot have a universal group as a member.
.

MessageId=8518
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER
Language=English
A universal group cannot have a local group as a member.
.
Language=Russian
A universal group cannot have a local group as a member.
.
Language=Polish
Grupa globalna nie może być członkiem grupy uniwersalnej.
.
Language=Romanian
A universal group cannot have a local group as a member.
.

MessageId=8519
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER
Language=English
A global group cannot have a cross-domain member.
.
Language=Russian
A global group cannot have a cross-domain member.
.
Language=Polish
Członek grupy globalnej nie może pochodzić z innej domeny.
.
Language=Romanian
A global group cannot have a cross-domain member.
.

MessageId=8520
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER
Language=English
A local group cannot have another cross-domain local group as a member.
.
Language=Russian
A local group cannot have another cross-domain local group as a member.
.
Language=Polish
Członkiem grupy globalnej nie może być grupa lokalna z innej domeny.
.
Language=Romanian
A local group cannot have another cross-domain local group as a member.
.

MessageId=8521
Severity=Success
Facility=System
SymbolicName=ERROR_DS_HAVE_PRIMARY_MEMBERS
Language=English
A group with primary members cannot change to a security-disabled group.
.
Language=Russian
A group with primary members cannot change to a security-disabled group.
.
Language=Polish
Grupa z członkami głównymi nie może zostać zmieniona w grupę z wyłączonymi zabezpieczeniami.
.
Language=Romanian
A group with primary members cannot change to a security-disabled group.
.

MessageId=8522
Severity=Success
Facility=System
SymbolicName=ERROR_DS_STRING_SD_CONVERSION_FAILED
Language=English
The schema cache load failed to convert the string default SD on a class-schema object.
.
Language=Russian
The schema cache load failed to convert the string default SD on a class-schema object.
.
Language=Polish
Podczas ładowania schematu z pamięci podręcznej nie udało się przekształcić domyślnego SD na obiekt schematu klasy.
.
Language=Romanian
The schema cache load failed to convert the string default SD on a class-schema object.
.

MessageId=8523
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAMING_MASTER_GC
Language=English
Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers)
.
Language=Russian
Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers)
.
Language=Polish
Tylko serwery DSA skonfigurowane jako serwery wykazu globalnego mogą pełnić rolę wzorca operacji FSMO nazw domen. (Ma zastosowanie tylko dla serwerów systemu Windows 2000)
.
Language=Romanian
Only DSAs configured to be Global Catalog servers should be allowed to hold the Domain Naming Master FSMO role. (Applies only to Windows 2000 servers)
.

MessageId=8524
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOOKUP_FAILURE
Language=English
The DSA operation is unable to proceed because of a DNS lookup failure.
.
Language=Russian
The DSA operation is unable to proceed because of a DNS lookup failure.
.
Language=Polish
Operacja DSA nie może być kontynuowana wskutek błędu wyszukiwania DNS.
.
Language=Romanian
The DSA operation is unable to proceed because of a DNS lookup failure.
.

MessageId=8525
Severity=Success
Facility=System
SymbolicName=ERROR_DS_COULDNT_UPDATE_SPNS
Language=English
While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync.
.
Language=Russian
While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync.
.
Language=Polish
Podczas przetwarzania zmian nazwy hosta DNS dla danego obiektu, wartości głównej nazwy usługi (SPN) nie zostały zsynchronizowane.
.
Language=Romanian
While processing a change to the DNS Host Name for an object, the Service Principal Name values could not be kept in sync.
.

MessageId=8526
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_RETRIEVE_SD
Language=English
The Security Descriptor attribute could not be read.
.
Language=Russian
The Security Descriptor attribute could not be read.
.
Language=Polish
Nie można odczytać atrybutu deskryptora zabezpieczeń.
.
Language=Romanian
The Security Descriptor attribute could not be read.
.

MessageId=8527
Severity=Success
Facility=System
SymbolicName=ERROR_DS_KEY_NOT_UNIQUE
Language=English
The object requested was not found, but an object with that key was found.
.
Language=Russian
The object requested was not found, but an object with that key was found.
.
Language=Polish
Żądany obiekt nie został znaleziony, ale znaleziono obiekt z tym kluczem.
.
Language=Romanian
The object requested was not found, but an object with that key was found.
.

MessageId=8528
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WRONG_LINKED_ATT_SYNTAX
Language=English
The syntax of the linked attributed being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1.
.
Language=Russian
The syntax of the linked attributed being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1.
.
Language=Polish
Składnia dodawanego atrybutu łącza jest niepoprawna. Odsyłacze do przodu mogą mieć składnię tylko 2.5.5.1, 2.5.5.7 i 2.5.5.14, a odsyłacze wsteczne mogą mieć składnię tylko 2.5.5.1.
.
Language=Romanian
The syntax of the linked attributed being added is incorrect. Forward links can only have syntax 2.5.5.1, 2.5.5.7, and 2.5.5.14, and backlinks can only have syntax 2.5.5.1.
.

MessageId=8529
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD
Language=English
Security Account Manager needs to get the boot password.
.
Language=Russian
Security Account Manager needs to get the boot password.
.
Language=Polish
Menedżer kont zabezpieczeń musi uzyskać hasło rozruchowe.
.
Language=Romanian
Security Account Manager needs to get the boot password.
.

MessageId=8530
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY
Language=English
Security Account Manager needs to get the boot key from floppy disk.
.
Language=Russian
Security Account Manager needs to get the boot key from floppy disk.
.
Language=Polish
Menedżer kont zabezpieczeń musi uzyskać klucz rozruchowy z dyskietki.
.
Language=Romanian
Security Account Manager needs to get the boot key from floppy disk.
.

MessageId=8531
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_START
Language=English
Directory Service cannot start.
.
Language=Russian
Directory Service cannot start.
.
Language=Polish
Nie można uruchomić usługi katalogowej.
.
Language=Romanian
Directory Service cannot start.
.

MessageId=8532
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INIT_FAILURE
Language=English
Directory Services could not start.
.
Language=Russian
Directory Services could not start.
.
Language=Polish
Nie można uruchomić usług katalogowych.
.
Language=Romanian
Directory Services could not start.
.

MessageId=8533
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION
Language=English
The connection between client and server requires packet privacy or better.
.
Language=Russian
The connection between client and server requires packet privacy or better.
.
Language=Polish
Połączenie między klientem i serwerem wymaga zabezpieczeń zapewniających co najmniej prywatność pakietów.
.
Language=Romanian
The connection between client and server requires packet privacy or better.
.

MessageId=8534
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SOURCE_DOMAIN_IN_FOREST
Language=English
The source domain may not be in the same forest as destination.
.
Language=Russian
The source domain may not be in the same forest as destination.
.
Language=Polish
Domena źródłowa nie może występować w tym samym lesie co docelowa.
.
Language=Romanian
The source domain may not be in the same forest as destination.
.

MessageId=8535
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST
Language=English
The destination domain must be in the forest.
.
Language=Russian
The destination domain must be in the forest.
.
Language=Polish
Domena docelowa musi występować w lesie.
.
Language=Romanian
The destination domain must be in the forest.
.

MessageId=8536
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED
Language=English
The operation requires that destination domain auditing be enabled.
.
Language=Russian
The operation requires that destination domain auditing be enabled.
.
Language=Polish
Operacja wymaga włączenia inspekcji domeny docelowej.
.
Language=Romanian
The operation requires that destination domain auditing be enabled.
.

MessageId=8537
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN
Language=English
The operation couldn't locate a DC for the source domain.
.
Language=Russian
The operation couldn't locate a DC for the source domain.
.
Language=Polish
Operacja nie może zlokalizować kontrolera domeny dla domeny źródłowej.
.
Language=Romanian
The operation couldn't locate a DC for the source domain.
.

MessageId=8538
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER
Language=English
The source object must be a group or user.
.
Language=Russian
The source object must be a group or user.
.
Language=Polish
Obiekt źródłowy musi być grupą lub użytkownikiem.
.
Language=Romanian
The source object must be a group or user.
.

MessageId=8539
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_SID_EXISTS_IN_FOREST
Language=English
The source object's SID already exists in destination forest.
.
Language=Russian
The source object's SID already exists in destination forest.
.
Language=Polish
Identyfikator SID obiektu źródłowego już istnieje w lesie docelowym.
.
Language=Romanian
The source object's SID already exists in destination forest.
.

MessageId=8540
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH
Language=English
The source and destination object must be of the same type.
.
Language=Russian
The source and destination object must be of the same type.
.
Language=Polish
Obiekty źródłowy i docelowy muszą być tego samego typu.
.
Language=Romanian
The source and destination object must be of the same type.
.

MessageId=8541
Severity=Success
Facility=System
SymbolicName=ERROR_SAM_INIT_FAILURE
Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Safe Mode. Check the event log for detailed information.
.
Language=Russian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Safe Mode. Check the event log for detailed information.
.
Language=Polish
Nie można zainicjować Menedżera kont zabezpieczeń z powodu następującego błędu: %1.
Stan błędu: 0x%2. Kliknij przycisk OK, aby zamknąć system i uruchomić go w trybie awaryjnym. Szczegółowe informacje można znaleźć w dzienniku zdarzeń.
.
Language=Romanian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Click OK to shut down the system and reboot into Safe Mode. Check the event log for detailed information.
.

MessageId=8542
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_INFO_SHIP
Language=English
Schema information could not be included in the replication request.
.
Language=Russian
Schema information could not be included in the replication request.
.
Language=Polish
Informacje o schemacie nie mogły być zawarte w żądaniu replikacji.
.
Language=Romanian
Schema information could not be included in the replication request.
.

MessageId=8543
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_SCHEMA_CONFLICT
Language=English
The replication operation could not be completed due to a schema incompatibility.
.
Language=Russian
The replication operation could not be completed due to a schema incompatibility.
.
Language=Polish
Nie można ukończyć operacji replikacji z powodu niezgodności schematów.
.
Language=Romanian
The replication operation could not be completed due to a schema incompatibility.
.

MessageId=8544
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_EARLIER_SCHEMA_CONLICT
Language=English
The replication operation could not be completed due to a previous schema incompatibility.
.
Language=Russian
The replication operation could not be completed due to a previous schema incompatibility.
.
Language=Polish
Nie można ukończyć operacji replikacji z powodu poprzedniej niezgodności schematów.
.
Language=Romanian
The replication operation could not be completed due to a previous schema incompatibility.
.

MessageId=8545
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OBJ_NC_MISMATCH
Language=English
The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation.
.
Language=Russian
The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation.
.
Language=Polish
Nie można dokonać aktualizacji replikacji, ponieważ miejsce źródłowe lub docelowe nie otrzymało informacji odnośnie ostatniej operacji przenoszenia poza domenę.
.
Language=Romanian
The replication update could not be applied because either the source or the destination has not yet received information regarding a recent cross-domain move operation.
.

MessageId=8546
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NC_STILL_HAS_DSAS
Language=English
The requested domain could not be deleted because there exist domain controllers that still host this domain.
.
Language=Russian
The requested domain could not be deleted because there exist domain controllers that still host this domain.
.
Language=Polish
Żądana domena nie mogła być usunięta ponieważ istnieją kontrolery domeny, które nadal są jej hostem.
.
Language=Romanian
The requested domain could not be deleted because there exist domain controllers that still host this domain.
.

MessageId=8547
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GC_REQUIRED
Language=English
The requested operation can be performed only on a global catalog server.
.
Language=Russian
The requested operation can be performed only on a global catalog server.
.
Language=Polish
Żądana operacja może być wykonana tylko na serwerze wykazu globalnego.
.
Language=Romanian
The requested operation can be performed only on a global catalog server.
.

MessageId=8548
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY
Language=English
A local group can only be a member of other local groups in the same domain.
.
Language=Russian
A local group can only be a member of other local groups in the same domain.
.
Language=Polish
Grupa lokalna może być tylko członkiem innej grupy lokalnej w tej samej domenie.
.
Language=Romanian
A local group can only be a member of other local groups in the same domain.
.

MessageId=8549
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS
Language=English
Foreign security principals cannot be members of universal groups.
.
Language=Russian
Foreign security principals cannot be members of universal groups.
.
Language=Polish
Konta lub grupy kont obcych zasad zabezpieczeń nie mogą być członkami grup uniwersalnych.
.
Language=Romanian
Foreign security principals cannot be members of universal groups.
.

MessageId=8550
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ADD_TO_GC
Language=English
The attribute is not allowed to be replicated to the GC because of security reasons.
.
Language=Russian
The attribute is not allowed to be replicated to the GC because of security reasons.
.
Language=Polish
Atrybut nie może być replikowany do GC z powodów bezpieczeństwa.
.
Language=Romanian
The attribute is not allowed to be replicated to the GC because of security reasons.
.

MessageId=8551
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_CHECKPOINT_WITH_PDC
Language=English
The checkpoint with the PDC could not be taken because there are too many modifications being processed currently.
.
Language=Russian
The checkpoint with the PDC could not be taken because there are too many modifications being processed currently.
.
Language=Polish
Nie można przejąć punktu kontrolnego PDC, ponieważ obecnie jest przetwarzanych zbyt wiele modyfikacji.
.
Language=Romanian
The checkpoint with the PDC could not be taken because there are too many modifications being processed currently.
.

MessageId=8552
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SOURCE_AUDITING_NOT_ENABLED
Language=English
The operation requires that source domain auditing be enabled.
.
Language=Russian
The operation requires that source domain auditing be enabled.
.
Language=Polish
Operacja wymaga włączenia inspekcji domeny źródłowej.
.
Language=Romanian
The operation requires that source domain auditing be enabled.
.

MessageId=8553
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC
Language=English
Security principal objects can only be created inside domain naming contexts.
.
Language=Russian
Security principal objects can only be created inside domain naming contexts.
.
Language=Polish
Obiekty główne zabezpieczeń mogą być tworzone jedynie wewnątrz kontekstów nazewnictwa domen.
.
Language=Romanian
Security principal objects can only be created inside domain naming contexts.
.

MessageId=8554
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_NAME_FOR_SPN
Language=English
A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format.
.
Language=Russian
A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format.
.
Language=Polish
Nie można utworzyć głównej nazwy usługi (SPN), ponieważ podana nazwa hosta ma nieodpowiedni format.
.
Language=Romanian
A Service Principal Name (SPN) could not be constructed because the provided hostname is not in the necessary format.
.

MessageId=8555
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS
Language=English
A Filter was passed that uses constructed attributes.
.
Language=Russian
A Filter was passed that uses constructed attributes.
.
Language=Polish
Został przekazany filtr, który używa złożonych atrybutów.
.
Language=Romanian
A Filter was passed that uses constructed attributes.
.

MessageId=8556
Severity=Success
Facility=System
SymbolicName=ERROR_DS_UNICODEPWD_NOT_IN_QUOTES
Language=English
The unicodePwd attribute value must be enclosed in double quotes.
.
Language=Russian
The unicodePwd attribute value must be enclosed in double quotes.
.
Language=Polish
Wartość atrybutu unicodePwd musi być ujęta w cudzysłów.
.
Language=Romanian
The unicodePwd attribute value must be enclosed in double quotes.
.

MessageId=8557
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED
Language=English
Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.
.
Language=Russian
Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.
.
Language=Polish
Nie można dołączyć komputera do domeny. Została przekroczona maksymalna liczba kont, które możesz utworzyć w tej domenie. Skontaktuj się z administratorem systemu, aby ten limit zresetować lub zwiększyć.
.
Language=Romanian
Your computer could not be joined to the domain. You have exceeded the maximum number of computer accounts you are allowed to create in this domain. Contact your system administrator to have this limit reset or increased.
.

MessageId=8558
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MUST_BE_RUN_ON_DST_DC
Language=English
For security reasons, the operation must be run on the destination DC.
.
Language=Russian
For security reasons, the operation must be run on the destination DC.
.
Language=Polish
Aby zapewnić bezpieczeństwo, operacja musi być przeprowadzona na docelowym kontrolerze domeny.
.
Language=Romanian
For security reasons, the operation must be run on the destination DC.
.

MessageId=8559
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER
Language=English
For security reasons, the source DC must be NT4SP4 or greater.
.
Language=Russian
For security reasons, the source DC must be NT4SP4 or greater.
.
Language=Polish
Aby zapewnić bezpieczeństwo, źródłowy kontroler domeny musi pochodzić z dodatku NT4SP4 lub wyższego.
.
Language=Romanian
For security reasons, the source DC must be NT4SP4 or greater.
.

MessageId=8560
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_TREE_DELETE_CRITICAL_OBJ
Language=English
Critical Directory Service System objects cannot be deleted during tree delete operations. The tree delete may have been partially performed.
.
Language=Russian
Critical Directory Service System objects cannot be deleted during tree delete operations. The tree delete may have been partially performed.
.
Language=Polish
Nie można usunąć kluczowych obiektów usługi katalogowej podczas operacji usuwania drzewa. Być może drzewo zostało usunięte częściowo.
.
Language=Romanian
Critical Directory Service System objects cannot be deleted during tree delete operations. The tree delete may have been partially performed.
.

MessageId=8561
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INIT_FAILURE_CONSOLE
Language=English
Directory Services could not start because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.
Language=Russian
Directory Services could not start because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.
Language=Polish
Nie można uruchomić usług katalogowych z powodu następującego błędu: %1.
Stan błędu: 0x%2. Kliknij przycisk OK, aby zamknąć system. Możesz użyć konsoli odzyskiwania do diagnozy systemu.
.
Language=Romanian
Directory Services could not start because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8562
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SAM_INIT_FAILURE_CONSOLE
Language=English
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.
Language=Russian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.
Language=Polish
Zainicjowanie Menedżera kont zabezpieczeń nie powiodło się z powodu następującego błędu: %1.
Stan błędu: 0x%2. Kliknij przycisk OK, aby zamknąć system. Możesz użyć konsoli odzyskiwania do diagnozy systemu.
.
Language=Romanian
Security Accounts Manager initialization failed because of the following error: %1.
Error Status: 0x%2. Please click OK to shutdown the system. You can use the recovery console to diagnose the system further.
.

MessageId=8563
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FOREST_VERSION_TOO_HIGH
Language=English
The version of the operating system installed is incompatible with the current forest functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this forest.
.
Language=Russian
The version of the operating system installed is incompatible with the current forest functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this forest.
.
Language=Polish
Wersja zainstalowanego systemu operacyjnego jest niezgodna z bieżącym poziomem funkcjonalności lasu. Aby ten serwer mógł stać się kontrolerem domeny w tym lesie, trzeba uaktualnić system operacyjny do nowszej wersji.
.
Language=Romanian
The version of the operating system installed is incompatible with the current forest functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this forest.
.

MessageId=8564
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_HIGH
Language=English
The version of the operating system installed is incompatible with the current domain functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this domain.
.
Language=Russian
The version of the operating system installed is incompatible with the current domain functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this domain.
.
Language=Polish
Wersja zainstalowanego systemu operacyjnego jest niezgodna z bieżącym poziomem funkcjonalności domeny. Aby ten serwer mógł stać się kontrolerem domeny w tym lesie, trzeba uaktualnić system operacyjny do nowszej wersji.
.
Language=Romanian
The version of the operating system installed is incompatible with the current domain functional level. You must upgrade to a new version of the operating system before this server can become a domain controller in this domain.
.

MessageId=8565
Severity=Success
Facility=System
SymbolicName=ERROR_DS_FOREST_VERSION_TOO_LOW
Language=English
This version of the operating system installed on this server no longer supports the current forest functional level. You must raise the forest functional level before this server can become a domain controller in this forest.
.
Language=Russian
This version of the operating system installed on this server no longer supports the current forest functional level. You must raise the forest functional level before this server can become a domain controller in this forest.
.
Language=Polish
Wersja systemu operacyjnego zainstalowana na tym serwerze nie obsługuje już bieżącego poziomu funkcjonalności lasu. Aby ten serwer mógł stać się kontrolerem domeny w tym lesie, trzeba zwiększyć poziom funkcjonalności lasu.
.
Language=Romanian
This version of the operating system installed on this server no longer supports the current forest functional level. You must raise the forest functional level before this server can become a domain controller in this forest.
.

MessageId=8566
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_VERSION_TOO_LOW
Language=English
This version of the operating system installed on this server no longer supports the current domain functional level. You must raise the domain functional level before this server can become a domain controller in this domain.
.
Language=Russian
This version of the operating system installed on this server no longer supports the current domain functional level. You must raise the domain functional level before this server can become a domain controller in this domain.
.
Language=Polish
Wersja systemu operacyjnego zainstalowana na tym serwerze nie obsługuje już bieżącego poziomu funkcjonalności domeny. Aby ten serwer mógł stać się kontrolerem domeny w tym lesie, trzeba zwiększyć poziom funkcjonalności domeny.
.
Language=Romanian
This version of the operating system installed on this server no longer supports the current domain functional level. You must raise the domain functional level before this server can become a domain controller in this domain.
.

MessageId=8567
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCOMPATIBLE_VERSION
Language=English
The version of the operating system installed on this server is incompatible with the functional level of the domain or forest.
.
Language=Russian
The version of the operating system installed on this server is incompatible with the functional level of the domain or forest.
.
Language=Polish
Wersja zainstalowanego systemu operacyjnego na tym serwerze jest niezgodna z poziomem funkcjonalności domeny lub lasu
.
Language=Romanian
The version of the operating system installed on this server is incompatible with the functional level of the domain or forest.
.

MessageId=8568
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LOW_DSA_VERSION
Language=English
The functional level of the domain (or forest) cannot be raised to the requested value, because there exist one or more domain controllers in the domain (or forest) that are at a lower incompatible functional level.
.
Language=Russian
The functional level of the domain (or forest) cannot be raised to the requested value, because there exist one or more domain controllers in the domain (or forest) that are at a lower incompatible functional level.
.
Language=Polish
Poziomu funkcjonalności domeny (lub lasu) nie można zwiększyć do żądanej wartości, ponieważ w domenie (lub lesie) istnieje co najmniej jeden kontroler domeny z niższym niezgodnym poziomem funkcjonalności.
.
Language=Romanian
The functional level of the domain (or forest) cannot be raised to the requested value, because there exist one or more domain controllers in the domain (or forest) that are at a lower incompatible functional level.
.

MessageId=8569
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_BEHAVIOR_VERSION_IN_MIXEDDOMAIN
Language=English
The forest functional level cannot be raised to the requested level since one or more domains are still in mixed domain mode. All domains in the forest must be in native mode before you can raise the forest functional level.
.
Language=Russian
The forest functional level cannot be raised to the requested level since one or more domains are still in mixed domain mode. All domains in the forest must be in native mode before you can raise the forest functional level.
.
Language=Polish
Poziomu funkcjonalności lasu nie można zwiększyć do żądanej wartości, ponieważ co najmniej jedna domena jest nadal w trybie domeny mieszanej. Przed zwiększeniem poziomu funkcjonalności lasu wszystkie domeny w lesie muszą być w trybie macierzystym.
.
Language=Romanian
The forest functional level cannot be raised to the requested level since one or more domains are still in mixed domain mode. All domains in the forest must be in native mode before you can raise the forest functional level.
.

MessageId=8570
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_SUPPORTED_SORT_ORDER
Language=English
The sort order requested is not supported.
.
Language=Russian
The sort order requested is not supported.
.
Language=Polish
Żądana kolejność sortowania nie jest obsługiwana.
.
Language=Romanian
The sort order requested is not supported.
.

MessageId=8571
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_NOT_UNIQUE
Language=English
The requested name already exists as a unique identifier.
.
Language=Russian
The requested name already exists as a unique identifier.
.
Language=Polish
Żądana nazwa już istnieje, jako unikatowy identyfikator.
.
Language=Romanian
The requested name already exists as a unique identifier.
.

MessageId=8572
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4
Language=English
The machine account was created pre-NT4. The account needs to be recreated.
.
Language=Russian
The machine account was created pre-NT4. The account needs to be recreated.
.
Language=Polish
Konto komputera zostało utworzone przed systemem NT4. Konto musi być utworzone ponownie.
.
Language=Romanian
The machine account was created pre-NT4. The account needs to be recreated.
.

MessageId=8573
Severity=Success
Facility=System
SymbolicName=ERROR_DS_OUT_OF_VERSION_STORE
Language=English
The database is out of version store.
.
Language=Russian
The database is out of version store.
.
Language=Polish
Baza danych działa bez magazynu wersji.
.
Language=Romanian
The database is out of version store.
.

MessageId=8574
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INCOMPATIBLE_CONTROLS_USED
Language=English
Unable to continue operation because multiple conflicting controls were used.
.
Language=Russian
Unable to continue operation because multiple conflicting controls were used.
.
Language=Polish
Nie można kontynuować operacji, ponieważ użyto wielu konfliktowych elementów sterujących.
.
Language=Romanian
Unable to continue operation because multiple conflicting controls were used.
.

MessageId=8575
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_REF_DOMAIN
Language=English
Unable to find a valid security descriptor reference domain for this partition.
.
Language=Russian
Unable to find a valid security descriptor reference domain for this partition.
.
Language=Polish
Nie można odnaleźć prawidłowej domeny odniesienia deskryptora zabezpieczeń dla tej partycji.
.
Language=Romanian
Unable to find a valid security descriptor reference domain for this partition.
.

MessageId=8576
Severity=Success
Facility=System
SymbolicName=ERROR_DS_RESERVED_LINK_ID
Language=English
Schema update failed: The link identifier is reserved.
.
Language=Russian
Schema update failed: The link identifier is reserved.
.
Language=Polish
Nie można zaktualizować schematu: identyfikator łącza jest zarezerwowany.
.
Language=Romanian
Schema update failed: The link identifier is reserved.
.

MessageId=8577
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LINK_ID_NOT_AVAILABLE
Language=English
Schema update failed: There are no link identifiers available.
.
Language=Russian
Schema update failed: There are no link identifiers available.
.
Language=Polish
Nie można zaktualizować schematu: brak dostępnych identyfikatorów łącza.
.
Language=Romanian
Schema update failed: There are no link identifiers available.
.

MessageId=8578
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER
Language=English
An account group cannot have a universal group as a member.
.
Language=Russian
An account group cannot have a universal group as a member.
.
Language=Polish
Grupa uniwersalna nie może być członkiem grupy kont.
.
Language=Romanian
An account group cannot have a universal group as a member.
.

MessageId=8579
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE
Language=English
Rename or move operations on naming context heads or read-only objects are not allowed.
.
Language=Russian
Rename or move operations on naming context heads or read-only objects are not allowed.
.
Language=Polish
Operacje zmiany nazwy i przenoszenia dla początków kontekstu nazewnictwa i obiektów tylko do odczytu są niedozwolone.
.
Language=Romanian
Rename or move operations on naming context heads or read-only objects are not allowed.
.

MessageId=8580
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC
Language=English
Move operations on objects in the schema naming context are not allowed.
.
Language=Russian
Move operations on objects in the schema naming context are not allowed.
.
Language=Polish
Operacje przenoszenia obiektów w kontekście nazewnictwa schematu są niedozwolone.
.
Language=Romanian
Move operations on objects in the schema naming context are not allowed.
.

MessageId=8581
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG
Language=English
A system flag has been set on the object and does not allow the object to be moved or renamed.
.
Language=Russian
A system flag has been set on the object and does not allow the object to be moved or renamed.
.
Language=Polish
Dla obiektu ustawiono flagę systemową, która nie pozwala na przeniesienie obiektu ani zmianę jego nazwy.
.
Language=Romanian
A system flag has been set on the object and does not allow the object to be moved or renamed.
.

MessageId=8582
Severity=Success
Facility=System
SymbolicName=ERROR_DS_MODIFYDN_WRONG_GRANDPARENT
Language=English
This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers.
.
Language=Russian
This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers.
.
Language=Polish
Dla tego obiektu nie jest dozwolona zmiana jego kontenera nadrzędnego. Przenoszenie tego obiektu nie jest zabronione, ale jest ograniczone do kontenerów tego samego poziomu.
.
Language=Romanian
This object is not allowed to change its grandparent container. Moves are not forbidden on this object, but are restricted to sibling containers.
.

MessageId=8583
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NAME_ERROR_TRUST_REFERRAL
Language=English
Unable to resolve completely, a referral to another forest is generated.
.
Language=Russian
Unable to resolve completely, a referral to another forest is generated.
.
Language=Polish
Nie można w pełni rozpoznać; generowane jest odwołanie do innego lasu.
.
Language=Romanian
Unable to resolve completely, a referral to another forest is generated.
.

MessageId=8584
Severity=Success
Facility=System
SymbolicName=ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER
Language=English
The requested action is not supported on standard server.
.
Language=Russian
The requested action is not supported on standard server.
.
Language=Polish
Żądana akcja nie jest obsługiwana na serwerze standardowym.
.
Language=Romanian
The requested action is not supported on standard server.
.

MessageId=8585
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_ACCESS_REMOTE_PART_OF_AD
Language=English
Could not access a partition of the Active Directory located on a remote server. Make sure at least one server is running for the partition in question.
.
Language=Russian
Could not access a partition of the Active Directory located on a remote server. Make sure at least one server is running for the partition in question.
.
Language=Polish
Nie można uzyskać dostępu do partycji usługi katalogowej znajdującej się na serwerze zdalnym. Upewnij się, że dla danej partycji jest uruchomiony przynajmniej jeden serwer.
.
Language=Romanian
Could not access a partition of the Active Directory located on a remote server. Make sure at least one server is running for the partition in question.
.

MessageId=8586
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CR_IMPOSSIBLE_TO_VALIDATE_V2
Language=English
The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context. Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master.
.
Language=Russian
The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context. Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master.
.
Language=Polish
Katalog nie może sprawdzić poprawności proponowanej nazwy kontekstu nazewnictwa (lub partycji), ponieważ nie przechowuje on repliki ani nie może skontaktować się z repliką kontekstu nazewnictwa ponad proponowanym kontekstem nazewnictwa. Upewnij się, że nadrzędny kontekst nazewnictwa jest poprawnie zarejestrowany w systemie DNS i przynajmniej jedna replika tego kontekstu nazewnictwa jest osiągalna dla wzorca nazw domen.
.
Language=Romanian
The directory cannot validate the proposed naming context (or partition) name because it does not hold a replica nor can it contact a replica of the naming context above the proposed naming context. Please ensure that the parent naming context is properly registered in DNS, and at least one replica of this naming context is reachable by the Domain Naming master.
.

MessageId=8587
Severity=Success
Facility=System
SymbolicName=ERROR_DS_THREAD_LIMIT_EXCEEDED
Language=English
The thread limit for this request was exceeded.
.
Language=Russian
The thread limit for this request was exceeded.
.
Language=Polish
Przekroczono limit wątków dla tego żądania.
.
Language=Romanian
The thread limit for this request was exceeded.
.

MessageId=8588
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NOT_CLOSEST
Language=English
The Global catalog server is not in the closet site.
.
Language=Russian
The Global catalog server is not in the closet site.
.
Language=Polish
Serwer wykazu globalnego nie znajduje się w najbliższej witrynie.
.
Language=Romanian
The Global catalog server is not in the closet site.
.

MessageId=8589
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DERIVE_SPN_WITHOUT_SERVER_REF
Language=English
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute.
.
Language=Russian
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute.
.
Language=Polish
Usługa katalogowa nie może uzyskać głównej nazwy usługi (SPN) używanej z serwerem docelowym do wzajemnego uwierzytelniania, ponieważ odpowiedni obiekt serwera w lokalnej bazie danych usługi katalogowej nie ma atrybutu serverReference.
.
Language=Romanian
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the corresponding server object in the local DS database has no serverReference attribute.
.

MessageId=8590
Severity=Success
Facility=System
SymbolicName=ERROR_DS_SINGLE_USER_MODE_FAILED
Language=English
The Directory Service failed to enter single user mode.
.
Language=Russian
The Directory Service failed to enter single user mode.
.
Language=Polish
Nie można przełączyć usługi katalogowej do trybu jednego użytkownika.
.
Language=Romanian
The Directory Service failed to enter single user mode.
.

MessageId=8591
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NTDSCRIPT_SYNTAX_ERROR
Language=English
The Directory Service cannot parse the script because of a syntax error.
.
Language=Russian
The Directory Service cannot parse the script because of a syntax error.
.
Language=Polish
Usługa katalogowa nie może zanalizować skryptu z powodu błędu składniowego.
.
Language=Romanian
The Directory Service cannot parse the script because of a syntax error.
.

MessageId=8592
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NTDSCRIPT_PROCESS_ERROR
Language=English
The Directory Service cannot process the script because of an error.
.
Language=Russian
The Directory Service cannot process the script because of an error.
.
Language=Polish
Usługa katalogowa nie może przetworzyć skryptu z powodu błędu.
.
Language=Romanian
The Directory Service cannot process the script because of an error.
.

MessageId=8593
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DIFFERENT_REPL_EPOCHS
Language=English
The directory service cannot perform the requested operation because the servers involved are of different replication epochs (which is usually related to a domain rename that is in progress).
.
Language=Russian
The directory service cannot perform the requested operation because the servers involved are of different replication epochs (which is usually related to a domain rename that is in progress).
.
Language=Polish
Usługa katalogowa nie może wykonać żądanej operacji, ponieważ serwery, których ona dotyczy, należą do różnych epok replikacji (zwykle wiąże się to z trwającą operacją zmiany nazwy domeny).
.
Language=Romanian
The directory service cannot perform the requested operation because the servers involved are of different replication epochs (which is usually related to a domain rename that is in progress).
.

MessageId=8594
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRS_EXTENSIONS_CHANGED
Language=English
The directory service binding must be renegotiated due to a change in the server extensions information.
.
Language=Russian
The directory service binding must be renegotiated due to a change in the server extensions information.
.
Language=Polish
Powiązanie usługi katalogowej musi być ponownie negocjowane z powodu zmiany informacji o rozszerzeniach serwera.
.
Language=Romanian
The directory service binding must be renegotiated due to a change in the server extensions information.
.

MessageId=8595
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPLICA_SET_CHANGE_NOT_ALLOWED_ON_DISABLED_CR
Language=English
Operation not allowed on a disabled cross ref.
.
Language=Russian
Operation not allowed on a disabled cross ref.
.
Language=Polish
Operacja niedozwolona dla wyłączonego odwołania krzyżowego.
.
Language=Romanian
Operation not allowed on a disabled cross ref.
.

MessageId=8596
Severity=Success
Facility=System
SymbolicName=ERROR_DS_NO_MSDS_INTID
Language=English
Schema update failed: No values for msDS-IntId are available.
.
Language=Russian
Schema update failed: No values for msDS-IntId are available.
.
Language=Polish
Nie można zaktualizować schematu: brak dostępnych wartości dla identyfikatora msDS-IntId.
.
Language=Romanian
Schema update failed: No values for msDS-IntId are available.
.

MessageId=8597
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUP_MSDS_INTID
Language=English
Schema update failed: Duplicate msDS-IntId. Retry the operation.
.
Language=Russian
Schema update failed: Duplicate msDS-IntId. Retry the operation.
.
Language=Polish
Nie można zaktualizować schematu: zduplikowany identyfikator msDS-INtId. Ponów próbę wykonania operacji.
.
Language=Romanian
Schema update failed: Duplicate msDS-IntId. Retry the operation.
.

MessageId=8598
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTS_IN_RDNATTID
Language=English
Schema deletion failed: attribute is used in rDNAttID.
.
Language=Russian
Schema deletion failed: attribute is used in rDNAttID.
.
Language=Polish
Usunięcie schematu nie powiodło się: atrybut jest używany w parametrze rDNAttID.
.
Language=Romanian
Schema deletion failed: attribute is used in rDNAttID.
.

MessageId=8599
Severity=Success
Facility=System
SymbolicName=ERROR_DS_AUTHORIZATION_FAILED
Language=English
The directory service failed to authorize the request.
.
Language=Russian
The directory service failed to authorize the request.
.
Language=Polish
Usługa katalogowa nie może autoryzować żądania.
.
Language=Romanian
The directory service failed to authorize the request.
.

MessageId=8600
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INVALID_SCRIPT
Language=English
The Directory Service cannot process the script because it is invalid.
.
Language=Russian
The Directory Service cannot process the script because it is invalid.
.
Language=Polish
Usługa katalogowa nie może przetworzyć skryptu, ponieważ jest on nieprawidłowy.
.
Language=Romanian
The Directory Service cannot process the script because it is invalid.
.

MessageId=8601
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REMOTE_CROSSREF_OP_FAILED
Language=English
The remote create cross reference operation failed on the Domain Naming Master FSMO. The operation's error is in the extended data.
.
Language=Russian
The remote create cross reference operation failed on the Domain Naming Master FSMO. The operation's error is in the extended data.
.
Language=Polish
Operacja zdalnego utworzenia odwołania krzyżowego nie powiodła się dla operacji FSMO wzorca nazw domen. Błąd operacji występuje w danych rozszerzonych.
.
Language=Romanian
The remote create cross reference operation failed on the Domain Naming Master FSMO. The operation's error is in the extended data.
.

MessageId=8602
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CROSS_REF_BUSY
Language=English
A cross reference is in use locally with the same name.
.
Language=Russian
A cross reference is in use locally with the same name.
.
Language=Polish
Odsyłacz jest używany lokalnie z tą samą nazwą.
.
Language=Romanian
A cross reference is in use locally with the same name.
.

MessageId=8603
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DERIVE_SPN_FOR_DELETED_DOMAIN
Language=English
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the server's domain has been deleted from the forest.
.
Language=Russian
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the server's domain has been deleted from the forest.
.
Language=Polish
Usługa katalogowa nie może uzyskać głównej nazwy usługi (SPN) używanej z serwerem docelowym do wzajemnego uwierzytelniania, ponieważ domena serwera została usunięta z lasu.
.
Language=Romanian
The DS cannot derive a service principal name (SPN) with which to mutually authenticate the target server because the server's domain has been deleted from the forest.
.

MessageId=8604
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC
Language=English
Writeable NCs prevent this DC from demoting.
.
Language=Russian
Writeable NCs prevent this DC from demoting.
.
Language=Polish
Zapisywalne klastry węzłów nie pozwalają na obniżenie poziomu tego kontrolera domeny.
.
Language=Romanian
Writeable NCs prevent this DC from demoting.
.

MessageId=8605
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DUPLICATE_ID_FOUND
Language=English
The requested object has a non-unique identifier and cannot be retrieved.
.
Language=Russian
The requested object has a non-unique identifier and cannot be retrieved.
.
Language=Polish
Żądany obiekt ma identyfikator, który nie jest unikatowy, i nie można go pobrać.
.
Language=Romanian
The requested object has a non-unique identifier and cannot be retrieved.
.

MessageId=8606
Severity=Success
Facility=System
SymbolicName=ERROR_DS_INSUFFICIENT_ATTR_TO_CREATE_OBJECT
Language=English
Insufficient attributes were given to create an object. This object may not exist because it may have been deleted and already garbage collected.
.
Language=Russian
Insufficient attributes were given to create an object. This object may not exist because it may have been deleted and already garbage collected.
.
Language=Polish
Nadano atrybuty, które nie wystarczają do utworzenia obiektu. Ten obiekt może nie istnieć, ponieważ mógł zostać usunięty i wyrzucony jako element bezużyteczny.
.
Language=Romanian
Insufficient attributes were given to create an object. This object may not exist because it may have been deleted and already garbage collected.
.

MessageId=8607
Severity=Success
Facility=System
SymbolicName=ERROR_DS_GROUP_CONVERSION_ERROR
Language=English
The group cannot be converted due to attribute restrictions on the requested group type.
.
Language=Russian
The group cannot be converted due to attribute restrictions on the requested group type.
.
Language=Polish
Nie można konwertować grupy z powodu ograniczeń atrybutów w żądanym typie grupy.
.
Language=Romanian
The group cannot be converted due to attribute restrictions on the requested group type.
.

MessageId=8608
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_APP_BASIC_GROUP
Language=English
Cross-domain move of non-empty basic application groups is not allowed.
.
Language=Russian
Cross-domain move of non-empty basic application groups is not allowed.
.
Language=Polish
Przeniesienie poza domenę niepustych grup podstawowych aplikacji jest niedozwolone.
.
Language=Romanian
Cross-domain move of non-empty basic application groups is not allowed.
.

MessageId=8609
Severity=Success
Facility=System
SymbolicName=ERROR_DS_CANT_MOVE_APP_QUERY_GROUP
Language=English
Cross-domain move on non-empty query based application groups is not allowed.
.
Language=Russian
Cross-domain move on non-empty query based application groups is not allowed.
.
Language=Polish
Przeniesienie poza domenę niepustych grup podstawowych aplikacji opartych na zapytaniach jest niedozwolone.
.
Language=Romanian
Cross-domain move on non-empty query based application groups is not allowed.
.

MessageId=8610
Severity=Success
Facility=System
SymbolicName=ERROR_DS_ROLE_NOT_VERIFIED
Language=English
The role owner could not be verified because replication of its partition has not occurred recently.
.
Language=Russian
The role owner could not be verified because replication of its partition has not occurred recently.
.
Language=Polish
Nie można zweryfikować własności roli FSMO, ponieważ jej partycja katalogu nie replikowała pomyślnie z co najmniej jednym partnerem replikacji.
.
Language=Romanian
The role owner could not be verified because replication of its partition has not occurred recently.
.

MessageId=8611
Severity=Success
Facility=System
SymbolicName=ERROR_DS_WKO_CONTAINER_CANNOT_BE_SPECIAL
Language=English
The target container for a redirection of a well-known object container cannot already be a special container.
.
Language=Russian
The target container for a redirection of a well-known object container cannot already be a special container.
.
Language=Polish
Kontener docelowy przekierowania dobrze znanych obiektów nie może już być kontenerem specjalnym.
.
Language=Romanian
The target container for a redirection of a well-known object container cannot already be a special container.
.

MessageId=8612
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DOMAIN_RENAME_IN_PROGRESS
Language=English
The Directory Service cannot perform the requested operation because a domain rename operation is in progress.
.
Language=Russian
The Directory Service cannot perform the requested operation because a domain rename operation is in progress.
.
Language=Polish
Usługa katalogowa nie może wykonać żądanej operacji, ponieważ trwa operacja zmiany nazwy domeny.
.
Language=Romanian
The Directory Service cannot perform the requested operation because a domain rename operation is in progress.
.

MessageId=8613
Severity=Success
Facility=System
SymbolicName=ERROR_DS_EXISTING_AD_CHILD_NC
Language=English
The Active Directory detected an Active Directory child partition below the requested new partition name. The Active Directory's partition hierarchy must be created in a top-down method.
.
Language=Russian
The Active Directory detected an Active Directory child partition below the requested new partition name. The Active Directory's partition hierarchy must be created in a top-down method.
.
Language=Polish
Usługa katalogowa wykryła partycję podrzędną poniżej żądanej nazwy partycji. Hierarchię partycji należy tworzyć metodą zstępującą.
.
Language=Romanian
The Active Directory detected an Active Directory child partition below the requested new partition name. The Active Directory's partition hierarchy must be created in a top-down method.
.

MessageId=8614
Severity=Success
Facility=System
SymbolicName=ERROR_DS_REPL_LIFETIME_EXCEEDED
Language=English
The Active Directory cannot replicate with this server because the time since the last replication with this server has exceeded the tombstone lifetime.
.
Language=Russian
The Active Directory cannot replicate with this server because the time since the last replication with this server has exceeded the tombstone lifetime.
.
Language=Polish
Usługa katalogowa nie może replikować z tym serwerem, ponieważ czas, który upłynął od ostatniej replikacji z tym serwerem, przekroczył okres istnienia reliktu.
.
Language=Romanian
The Active Directory cannot replicate with this server because the time since the last replication with this server has exceeded the tombstone lifetime.
.

MessageId=8615
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DISALLOWED_IN_SYSTEM_CONTAINER
Language=English
The requested operation is not allowed on an object under the system container.
.
Language=Russian
The requested operation is not allowed on an object under the system container.
.
Language=Polish
Żądana operacja jest niedozwolona na obiekcie znajdującym się w kontenerze systemowym.
.
Language=Romanian
The requested operation is not allowed on an object under the system container.
.

MessageId=8616
Severity=Success
Facility=System
SymbolicName=ERROR_DS_LDAP_SEND_QUEUE_FULL
Language=English
The LDAP servers network send queue has filled up because the client is not processing the results of it's requests fast enough. No more requests will be processed until the client catches up. If the client does not catch up then it will be disconnected.
.
Language=Russian
The LDAP servers network send queue has filled up because the client is not processing the results of it's requests fast enough. No more requests will be processed until the client catches up. If the client does not catch up then it will be disconnected.
.
Language=Polish
Kolejka odbiorcza sieci serwerów LDAP wypełniła się, ponieważ klient nie przetwarza odpowiednio szybko wyników swoich żądań. Żadne żądania nie będą przetwarzane do czasu wyrównania klienta. Jeżeli klient nie zostanie wyrównany, zostanie rozłączony.
.
Language=Romanian
The LDAP servers network send queue has filled up because the client is not processing the results of it's requests fast enough. No more requests will be processed until the client catches up. If the client does not catch up then it will be disconnected.
.

MessageId=8617
Severity=Success
Facility=System
SymbolicName=ERROR_DS_DRA_OUT_SCHEDULE_WINDOW
Language=English
The scheduled replication did not take place because the system was too busy to execute the request within the schedule window. The replication queue is overloaded. Consider reducing the number of partners or decreasing the scheduled replication frequency.
.
Language=Russian
The scheduled replication did not take place because the system was too busy to execute the request within the schedule window. The replication queue is overloaded. Consider reducing the number of partners or decreasing the scheduled replication frequency.
.
Language=Polish
Zaplanowana replikacja nie została przeprowadzona, ponieważ system był zbyt zajęty, aby wykonać żądanie w zaplanowanym czasie. Kolejka replikacji jest przeciążona. Rozważ ograniczenie liczby partnerów lub zmniejszenie częstotliwości zaplanowanych replikacji.
.
Language=Romanian
The scheduled replication did not take place because the system was too busy to execute the request within the schedule window. The replication queue is overloaded. Consider reducing the number of partners or decreasing the scheduled replication frequency.
.

MessageId=9001
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_FORMAT_ERROR
Language=English
DNS server unable to interpret format.
.
Language=Russian
DNS server unable to interpret format.
.
Language=Polish
DNS server unable to interpret format.
.
Language=Romanian
DNS server unable to interpret format.
.

MessageId=9002
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_SERVER_FAILURE
Language=English
DNS server failure.
.
Language=Russian
DNS server failure.
.
Language=Polish
DNS server failure.
.
Language=Romanian
DNS server failure.
.

MessageId=9003
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NAME_ERROR
Language=English
DNS name does not exist.
.
Language=Russian
DNS name does not exist.
.
Language=Polish
DNS name does not exist.
.
Language=Romanian
DNS name does not exist.
.

MessageId=9004
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOT_IMPLEMENTED
Language=English
DNS request not supported by name server.
.
Language=Russian
DNS request not supported by name server.
.
Language=Polish
DNS request not supported by name server.
.
Language=Romanian
DNS request not supported by name server.
.

MessageId=9005
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_REFUSED
Language=English
DNS operation refused.
.
Language=Russian
DNS operation refused.
.
Language=Polish
DNS operation refused.
.
Language=Romanian
DNS operation refused.
.

MessageId=9006
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_YXDOMAIN
Language=English
DNS name that ought not exist, does exist.
.
Language=Russian
DNS name that ought not exist, does exist.
.
Language=Polish
DNS name that ought not exist, does exist.
.
Language=Romanian
DNS name that ought not exist, does exist.
.

MessageId=9007
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_YXRRSET
Language=English
DNS RR set that ought not exist, does exist.
.
Language=Russian
DNS RR set that ought not exist, does exist.
.
Language=Polish
DNS RR set that ought not exist, does exist.
.
Language=Romanian
DNS RR set that ought not exist, does exist.
.

MessageId=9008
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NXRRSET
Language=English
DNS RR set that ought to exist, does not exist.
.
Language=Russian
DNS RR set that ought to exist, does not exist.
.
Language=Polish
DNS RR set that ought to exist, does not exist.
.
Language=Romanian
DNS RR set that ought to exist, does not exist.
.

MessageId=9009
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOTAUTH
Language=English
DNS server not authoritative for zone.
.
Language=Russian
DNS server not authoritative for zone.
.
Language=Polish
DNS server not authoritative for zone.
.
Language=Romanian
DNS server not authoritative for zone.
.

MessageId=9010
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_NOTZONE
Language=English
DNS name in update or prereq is not in zone.
.
Language=Russian
DNS name in update or prereq is not in zone.
.
Language=Polish
DNS name in update or prereq is not in zone.
.
Language=Romanian
DNS name in update or prereq is not in zone.
.

MessageId=9016
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADSIG
Language=English
DNS signature failed to verify.
.
Language=Russian
DNS signature failed to verify.
.
Language=Polish
DNS signature failed to verify.
.
Language=Romanian
DNS signature failed to verify.
.

MessageId=9017
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADKEY
Language=English
DNS bad key.
.
Language=Russian
DNS bad key.
.
Language=Polish
DNS bad key.
.
Language=Romanian
DNS bad key.
.

MessageId=9018
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE_BADTIME
Language=English
DNS signature validity expired.
.
Language=Russian
DNS signature validity expired.
.
Language=Polish
DNS signature validity expired.
.
Language=Romanian
DNS signature validity expired.
.

MessageId=9501
Severity=Success
Facility=System
SymbolicName=DNS_INFO_NO_RECORDS
Language=English
No records found for given DNS query.
.
Language=Russian
No records found for given DNS query.
.
Language=Polish
No records found for given DNS query.
.
Language=Romanian
No records found for given DNS query.
.

MessageId=9502
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_BAD_PACKET
Language=English
Bad DNS packet.
.
Language=Russian
Bad DNS packet.
.
Language=Polish
Bad DNS packet.
.
Language=Romanian
Bad DNS packet.
.

MessageId=9503
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_PACKET
Language=English
No DNS packet.
.
Language=Russian
No DNS packet.
.
Language=Polish
No DNS packet.
.
Language=Romanian
No DNS packet.
.

MessageId=9504
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RCODE
Language=English
DNS error, check rcode.
.
Language=Russian
DNS error, check rcode.
.
Language=Polish
DNS error, check rcode.
.
Language=Romanian
DNS error, check rcode.
.

MessageId=9505
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_UNSECURE_PACKET
Language=English
Unsecured DNS packet.
.
Language=Russian
Unsecured DNS packet.
.
Language=Polish
Unsecured DNS packet.
.
Language=Romanian
Unsecured DNS packet.
.

MessageId=9551
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_TYPE
Language=English
Invalid DNS type.
.
Language=Russian
Invalid DNS type.
.
Language=Polish
Invalid DNS type.
.
Language=Romanian
Invalid DNS type.
.

MessageId=9552
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_IP_ADDRESS
Language=English
Invalid IP address.
.
Language=Russian
Invalid IP address.
.
Language=Polish
Invalid IP address.
.
Language=Romanian
Invalid IP address.
.

MessageId=9553
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_PROPERTY
Language=English
Invalid property.
.
Language=Russian
Invalid property.
.
Language=Polish
Invalid property.
.
Language=Romanian
Invalid property.
.

MessageId=9554
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_TRY_AGAIN_LATER
Language=English
Try DNS operation again later.
.
Language=Russian
Try DNS operation again later.
.
Language=Polish
Try DNS operation again later.
.
Language=Romanian
Try DNS operation again later.
.

MessageId=9555
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_UNIQUE
Language=English
Record for given name and type is not unique.
.
Language=Russian
Record for given name and type is not unique.
.
Language=Polish
Record for given name and type is not unique.
.
Language=Romanian
Record for given name and type is not unique.
.

MessageId=9556
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NON_RFC_NAME
Language=English
DNS name does not comply with RFC specifications.
.
Language=Russian
DNS name does not comply with RFC specifications.
.
Language=Polish
DNS name does not comply with RFC specifications.
.
Language=Romanian
DNS name does not comply with RFC specifications.
.

MessageId=9557
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_FQDN
Language=English
DNS name is a fully-qualified DNS name.
.
Language=Russian
DNS name is a fully-qualified DNS name.
.
Language=Polish
DNS name is a fully-qualified DNS name.
.
Language=Romanian
DNS name is a fully-qualified DNS name.
.

MessageId=9558
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_DOTTED_NAME
Language=English
DNS name is dotted (multi-label).
.
Language=Russian
DNS name is dotted (multi-label).
.
Language=Polish
DNS name is dotted (multi-label).
.
Language=Romanian
DNS name is dotted (multi-label).
.

MessageId=9559
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_SINGLE_PART_NAME
Language=English
DNS name is a single-part name.
.
Language=Russian
DNS name is a single-part name.
.
Language=Polish
DNS name is a single-part name.
.
Language=Romanian
DNS name is a single-part name.
.

MessageId=9560
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_NAME_CHAR
Language=English
DSN name contains an invalid character.
.
Language=Russian
DSN name contains an invalid character.
.
Language=Polish
DSN name contains an invalid character.
.
Language=Romanian
DSN name contains an invalid character.
.

MessageId=9561
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NUMERIC_NAME
Language=English
DNS name is entirely numeric.
.
Language=Russian
DNS name is entirely numeric.
.
Language=Polish
DNS name is entirely numeric.
.
Language=Romanian
DNS name is entirely numeric.
.

MessageId=9562
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER
Language=English
The operation requested is not permitted on a DNS root server.
.
Language=Russian
The operation requested is not permitted on a DNS root server.
.
Language=Polish
The operation requested is not permitted on a DNS root server.
.
Language=Romanian
The operation requested is not permitted on a DNS root server.
.

MessageId=9563
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NOT_ALLOWED_UNDER_DELEGATION
Language=English
The record could not be created because this part of the DNS namespace has been delegated to another server.
.
Language=Russian
The record could not be created because this part of the DNS namespace has been delegated to another server.
.
Language=Polish
The record could not be created because this part of the DNS namespace has been delegated to another server.
.
Language=Romanian
The record could not be created because this part of the DNS namespace has been delegated to another server.
.

MessageId=9564
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CANNOT_FIND_ROOT_HINTS
Language=English
The DNS server could not find a set of root hints.
.
Language=Russian
The DNS server could not find a set of root hints.
.
Language=Polish
The DNS server could not find a set of root hints.
.
Language=Romanian
The DNS server could not find a set of root hints.
.

MessageId=9565
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INCONSISTENT_ROOT_HINTS
Language=English
The DNS server found root hints but they were not consistent across all adapters.
.
Language=Russian
The DNS server found root hints but they were not consistent across all adapters.
.
Language=Polish
The DNS server found root hints but they were not consistent across all adapters.
.
Language=Romanian
The DNS server found root hints but they were not consistent across all adapters.
.

MessageId=9601
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_DOES_NOT_EXIST
Language=English
DNS zone does not exist.
.
Language=Russian
DNS zone does not exist.
.
Language=Polish
DNS zone does not exist.
.
Language=Romanian
DNS zone does not exist.
.

MessageId=9602
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_ZONE_INFO
Language=English
DNS zone information not available.
.
Language=Russian
DNS zone information not available.
.
Language=Polish
DNS zone information not available.
.
Language=Romanian
DNS zone information not available.
.

MessageId=9603
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_ZONE_OPERATION
Language=English
Invalid operation for DNS zone.
.
Language=Russian
Invalid operation for DNS zone.
.
Language=Polish
Invalid operation for DNS zone.
.
Language=Romanian
Invalid operation for DNS zone.
.

MessageId=9604
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_CONFIGURATION_ERROR
Language=English
Invalid DNS zone configuration.
.
Language=Russian
Invalid DNS zone configuration.
.
Language=Polish
Invalid DNS zone configuration.
.
Language=Romanian
Invalid DNS zone configuration.
.

MessageId=9605
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_HAS_NO_SOA_RECORD
Language=English
DNS zone has no start of authority (SOA) record.
.
Language=Russian
DNS zone has no start of authority (SOA) record.
.
Language=Polish
DNS zone has no start of authority (SOA) record.
.
Language=Romanian
DNS zone has no start of authority (SOA) record.
.

MessageId=9606
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_HAS_NO_NS_RECORDS
Language=English
DNS zone has no name server (NS) record.
.
Language=Russian
DNS zone has no name server (NS) record.
.
Language=Polish
DNS zone has no name server (NS) record.
.
Language=Romanian
DNS zone has no name server (NS) record.
.

MessageId=9607
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_LOCKED
Language=English
DNS zone is locked.
.
Language=Russian
DNS zone is locked.
.
Language=Polish
DNS zone is locked.
.
Language=Romanian
DNS zone is locked.
.

MessageId=9608
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_CREATION_FAILED
Language=English
DNS zone creation failed.
.
Language=Russian
DNS zone creation failed.
.
Language=Polish
DNS zone creation failed.
.
Language=Romanian
DNS zone creation failed.
.

MessageId=9609
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_ALREADY_EXISTS
Language=English
DNS zone already exists.
.
Language=Russian
DNS zone already exists.
.
Language=Polish
DNS zone already exists.
.
Language=Romanian
DNS zone already exists.
.

MessageId=9610
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_AUTOZONE_ALREADY_EXISTS
Language=English
DNS automatic zone already exists.
.
Language=Russian
DNS automatic zone already exists.
.
Language=Polish
DNS automatic zone already exists.
.
Language=Romanian
DNS automatic zone already exists.
.

MessageId=9611
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_ZONE_TYPE
Language=English
Invalid DNS zone type.
.
Language=Russian
Invalid DNS zone type.
.
Language=Polish
Invalid DNS zone type.
.
Language=Romanian
Invalid DNS zone type.
.

MessageId=9612
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP
Language=English
Secondary DNS zone requires master IP address.
.
Language=Russian
Secondary DNS zone requires master IP address.
.
Language=Polish
Secondary DNS zone requires master IP address.
.
Language=Romanian
Secondary DNS zone requires master IP address.
.

MessageId=9613
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_NOT_SECONDARY
Language=English
DNS zone not secondary.
.
Language=Russian
DNS zone not secondary.
.
Language=Polish
DNS zone not secondary.
.
Language=Romanian
DNS zone not secondary.
.

MessageId=9614
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NEED_SECONDARY_ADDRESSES
Language=English
Need secondary IP address.
.
Language=Russian
Need secondary IP address.
.
Language=Polish
Need secondary IP address.
.
Language=Romanian
Need secondary IP address.
.

MessageId=9615
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_WINS_INIT_FAILED
Language=English
WINS initialization failed.
.
Language=Russian
WINS initialization failed.
.
Language=Polish
WINS initialization failed.
.
Language=Romanian
WINS initialization failed.
.

MessageId=9616
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NEED_WINS_SERVERS
Language=English
Need WINS servers.
.
Language=Russian
Need WINS servers.
.
Language=Polish
Need WINS servers.
.
Language=Romanian
Need WINS servers.
.

MessageId=9617
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NBSTAT_INIT_FAILED
Language=English
NBTSTAT initialization call failed.
.
Language=Russian
NBTSTAT initialization call failed.
.
Language=Polish
NBTSTAT initialization call failed.
.
Language=Romanian
NBTSTAT initialization call failed.
.

MessageId=9618
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SOA_DELETE_INVALID
Language=English
Invalid delete of start of authority (SOA)
.
Language=Russian
Invalid delete of start of authority (SOA)
.
Language=Polish
Invalid delete of start of authority (SOA)
.
Language=Romanian
Invalid delete of start of authority (SOA)
.

MessageId=9619
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_FORWARDER_ALREADY_EXISTS
Language=English
A conditional forwarding zone already exists for that name.
.
Language=Russian
A conditional forwarding zone already exists for that name.
.
Language=Polish
A conditional forwarding zone already exists for that name.
.
Language=Romanian
A conditional forwarding zone already exists for that name.
.

MessageId=9620
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_REQUIRES_MASTER_IP
Language=English
This zone must be configured with one or more master DNS server IP addresses.
.
Language=Russian
This zone must be configured with one or more master DNS server IP addresses.
.
Language=Polish
This zone must be configured with one or more master DNS server IP addresses.
.
Language=Romanian
This zone must be configured with one or more master DNS server IP addresses.
.

MessageId=9621
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_ZONE_IS_SHUTDOWN
Language=English
The operation cannot be performed because this zone is shutdown.
.
Language=Russian
The operation cannot be performed because this zone is shutdown.
.
Language=Polish
The operation cannot be performed because this zone is shutdown.
.
Language=Romanian
The operation cannot be performed because this zone is shutdown.
.

MessageId=9651
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_PRIMARY_REQUIRES_DATAFILE
Language=English
Primary DNS zone requires datafile.
.
Language=Russian
Primary DNS zone requires datafile.
.
Language=Polish
Primary DNS zone requires datafile.
.
Language=Romanian
Primary DNS zone requires datafile.
.

MessageId=9652
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_INVALID_DATAFILE_NAME
Language=English
Invalid datafile name for DNS zone.
.
Language=Russian
Invalid datafile name for DNS zone.
.
Language=Polish
Invalid datafile name for DNS zone.
.
Language=Romanian
Invalid datafile name for DNS zone.
.

MessageId=9653
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DATAFILE_OPEN_FAILURE
Language=English
Failed to open datafile for DNS zone.
.
Language=Russian
Failed to open datafile for DNS zone.
.
Language=Polish
Failed to open datafile for DNS zone.
.
Language=Romanian
Failed to open datafile for DNS zone.
.

MessageId=9654
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_FILE_WRITEBACK_FAILED
Language=English
Failed to write datafile for DNS zone.
.
Language=Russian
Failed to write datafile for DNS zone.
.
Language=Polish
Failed to write datafile for DNS zone.
.
Language=Romanian
Failed to write datafile for DNS zone.
.

MessageId=9655
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DATAFILE_PARSING
Language=English
Failure while reading datafile for DNS zone.
.
Language=Russian
Failure while reading datafile for DNS zone.
.
Language=Polish
Failure while reading datafile for DNS zone.
.
Language=Romanian
Failure while reading datafile for DNS zone.
.

MessageId=9701
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_DOES_NOT_EXIST
Language=English
DNS record does not exist.
.
Language=Russian
DNS record does not exist.
.
Language=Polish
DNS record does not exist.
.
Language=Romanian
DNS record does not exist.
.

MessageId=9702
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_FORMAT
Language=English
DNS record format error.
.
Language=Russian
DNS record format error.
.
Language=Polish
DNS record format error.
.
Language=Romanian
DNS record format error.
.

MessageId=9703
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NODE_CREATION_FAILED
Language=English
Node creation failure in DNS.
.
Language=Russian
Node creation failure in DNS.
.
Language=Polish
Node creation failure in DNS.
.
Language=Romanian
Node creation failure in DNS.
.

MessageId=9704
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_UNKNOWN_RECORD_TYPE
Language=English
Unknown DNS record type.
.
Language=Russian
Unknown DNS record type.
.
Language=Polish
Unknown DNS record type.
.
Language=Romanian
Unknown DNS record type.
.

MessageId=9705
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_TIMED_OUT
Language=English
DNS record timed out.
.
Language=Russian
DNS record timed out.
.
Language=Polish
DNS record timed out.
.
Language=Romanian
DNS record timed out.
.

MessageId=9706
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NAME_NOT_IN_ZONE
Language=English
Name not in DNS zone.
.
Language=Russian
Name not in DNS zone.
.
Language=Polish
Name not in DNS zone.
.
Language=Romanian
Name not in DNS zone.
.

MessageId=9707
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CNAME_LOOP
Language=English
CNAME loop detected.
.
Language=Russian
CNAME loop detected.
.
Language=Polish
CNAME loop detected.
.
Language=Romanian
CNAME loop detected.
.

MessageId=9708
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NODE_IS_CNAME
Language=English
Node is a CNAME DNS record.
.
Language=Russian
Node is a CNAME DNS record.
.
Language=Polish
Node is a CNAME DNS record.
.
Language=Romanian
Node is a CNAME DNS record.
.

MessageId=9709
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_CNAME_COLLISION
Language=English
A CNAME record already exists for given name.
.
Language=Russian
A CNAME record already exists for given name.
.
Language=Polish
A CNAME record already exists for given name.
.
Language=Romanian
A CNAME record already exists for given name.
.

MessageId=9710
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT
Language=English
Record only at DNS zone root.
.
Language=Russian
Record only at DNS zone root.
.
Language=Polish
Record only at DNS zone root.
.
Language=Romanian
Record only at DNS zone root.
.

MessageId=9711
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_RECORD_ALREADY_EXISTS
Language=English
DNS record already exists.
.
Language=Russian
DNS record already exists.
.
Language=Polish
DNS record already exists.
.
Language=Romanian
DNS record already exists.
.

MessageId=9712
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_SECONDARY_DATA
Language=English
Secondary DNS zone data error.
.
Language=Russian
Secondary DNS zone data error.
.
Language=Polish
Secondary DNS zone data error.
.
Language=Romanian
Secondary DNS zone data error.
.

MessageId=9713
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_CREATE_CACHE_DATA
Language=English
Could not create DNS cache data.
.
Language=Russian
Could not create DNS cache data.
.
Language=Polish
Could not create DNS cache data.
.
Language=Romanian
Could not create DNS cache data.
.

MessageId=9714
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NAME_DOES_NOT_EXIST
Language=English
DNS name does not exist.
.
Language=Russian
DNS name does not exist.
.
Language=Polish
DNS name does not exist.
.
Language=Romanian
DNS name does not exist.
.

MessageId=9715
Severity=Success
Facility=System
SymbolicName=DNS_WARNING_PTR_CREATE_FAILED
Language=English
Could not create pointer (PTR) record.
.
Language=Russian
Could not create pointer (PTR) record.
.
Language=Polish
Could not create pointer (PTR) record.
.
Language=Romanian
Could not create pointer (PTR) record.
.

MessageId=9716
Severity=Success
Facility=System
SymbolicName=DNS_WARNING_DOMAIN_UNDELETED
Language=English
DNS domain was undeleted.
.
Language=Russian
DNS domain was undeleted.
.
Language=Polish
DNS domain was undeleted.
.
Language=Romanian
DNS domain was undeleted.
.

MessageId=9717
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DS_UNAVAILABLE
Language=English
The directory service is unavailable.
.
Language=Russian
The directory service is unavailable.
.
Language=Polish
The directory service is unavailable.
.
Language=Romanian
The directory service is unavailable.
.

MessageId=9718
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DS_ZONE_ALREADY_EXISTS
Language=English
DNS zone already exists in the directory service.
.
Language=Russian
DNS zone already exists in the directory service.
.
Language=Polish
DNS zone already exists in the directory service.
.
Language=Romanian
DNS zone already exists in the directory service.
.

MessageId=9719
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE
Language=English
DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.
Language=Russian
DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.
Language=Polish
DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.
Language=Romanian
DNS server not creating or reading the boot file for the directory service integrated DNS zone.
.

MessageId=9751
Severity=Success
Facility=System
SymbolicName=DNS_INFO_AXFR_COMPLETE
Language=English
DNS AXFR (zone transfer) complete.
.
Language=Russian
DNS AXFR (zone transfer) complete.
.
Language=Polish
DNS AXFR (zone transfer) complete.
.
Language=Romanian
DNS AXFR (zone transfer) complete.
.

MessageId=9752
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_AXFR
Language=English
DNS zone transfer failed.
.
Language=Russian
DNS zone transfer failed.
.
Language=Polish
DNS zone transfer failed.
.
Language=Romanian
DNS zone transfer failed.
.

MessageId=9753
Severity=Success
Facility=System
SymbolicName=DNS_INFO_ADDED_LOCAL_WINS
Language=English
Added local WINS server.
.
Language=Russian
Added local WINS server.
.
Language=Polish
Added local WINS server.
.
Language=Romanian
Added local WINS server.
.

MessageId=9801
Severity=Success
Facility=System
SymbolicName=DNS_STATUS_CONTINUE_NEEDED
Language=English
Secure update call needs to continue update request.
.
Language=Russian
Secure update call needs to continue update request.
.
Language=Polish
Secure update call needs to continue update request.
.
Language=Romanian
Secure update call needs to continue update request.
.

MessageId=9851
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_TCPIP
Language=English
TCP/IP network protocol not installed.
.
Language=Russian
TCP/IP network protocol not installed.
.
Language=Polish
TCP/IP network protocol not installed.
.
Language=Romanian
TCP/IP network protocol not installed.
.

MessageId=9852
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_NO_DNS_SERVERS
Language=English
No DNS servers configured for local system.
.
Language=Russian
No DNS servers configured for local system.
.
Language=Polish
No DNS servers configured for local system.
.
Language=Romanian
No DNS servers configured for local system.
.

MessageId=9901
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_DOES_NOT_EXIST
Language=English
The specified directory partition does not exist.
.
Language=Russian
The specified directory partition does not exist.
.
Language=Polish
The specified directory partition does not exist.
.
Language=Romanian
The specified directory partition does not exist.
.

MessageId=9902
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_ALREADY_EXISTS
Language=English
The specified directory partition already exists.
.
Language=Russian
The specified directory partition already exists.
.
Language=Polish
The specified directory partition already exists.
.
Language=Romanian
The specified directory partition already exists.
.

MessageId=9903
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_NOT_ENLISTED
Language=English
The DNS server is not enlisted in the specified directory partition.
.
Language=Russian
The DNS server is not enlisted in the specified directory partition.
.
Language=Polish
The DNS server is not enlisted in the specified directory partition.
.
Language=Romanian
The DNS server is not enlisted in the specified directory partition.
.

MessageId=9904
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_ALREADY_ENLISTED
Language=English
The DNS server is already enlisted in the specified directory partition.
.
Language=Russian
The DNS server is already enlisted in the specified directory partition.
.
Language=Polish
The DNS server is already enlisted in the specified directory partition.
.
Language=Romanian
The DNS server is already enlisted in the specified directory partition.
.

MessageId=9905
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_NOT_AVAILABLE
Language=English
The directory partition is not available at this time. Please wait a few minutes and try again.
.
Language=Russian
The directory partition is not available at this time. Please wait a few minutes and try again.
.
Language=Polish
The directory partition is not available at this time. Please wait a few minutes and try again.
.
Language=Romanian
The directory partition is not available at this time. Please wait a few minutes and try again.
.

MessageId=9906
Severity=Success
Facility=System
SymbolicName=DNS_ERROR_DP_FSMO_ERROR
Language=English
The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003.
.
Language=Russian
The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003.
.
Language=Polish
The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003.
.
Language=Romanian
The application directory partition operation failed. The domain controller holding the domain naming master role is down or unable to service the request or is not running Windows Server 2003.
.

MessageId=10004
Severity=Success
Facility=System
SymbolicName=WSAEINTR
Language=English
A blocking operation was interrupted by a call to WSACancelBlockingCall.
.
Language=Russian
A blocking operation was interrupted by a call to WSACancelBlockingCall.
.
Language=Polish
A blocking operation was interrupted by a call to WSACancelBlockingCall.
.
Language=Romanian
A blocking operation was interrupted by a call to WSACancelBlockingCall.
.

MessageId=10009
Severity=Success
Facility=System
SymbolicName=WSAEBADF
Language=English
The file handle supplied is not valid.
.
Language=Russian
The file handle supplied is not valid.
.
Language=Polish
The file handle supplied is not valid.
.
Language=Romanian
The file handle supplied is not valid.
.

MessageId=10013
Severity=Success
Facility=System
SymbolicName=WSAEACCES
Language=English
An attempt was made to access a socket in a way forbidden by its access permissions.
.
Language=Russian
An attempt was made to access a socket in a way forbidden by its access permissions.
.
Language=Polish
An attempt was made to access a socket in a way forbidden by its access permissions.
.
Language=Romanian
An attempt was made to access a socket in a way forbidden by its access permissions.
.

MessageId=10014
Severity=Success
Facility=System
SymbolicName=WSAEFAULT
Language=English
The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.
Language=Russian
The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.
Language=Polish
The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.
Language=Romanian
The system detected an invalid pointer address in attempting to use a pointer argument in a call.
.

MessageId=10022
Severity=Success
Facility=System
SymbolicName=WSAEINVAL
Language=English
An invalid argument was supplied.
.
Language=Russian
An invalid argument was supplied.
.
Language=Polish
An invalid argument was supplied.
.
Language=Romanian
An invalid argument was supplied.
.

MessageId=10024
Severity=Success
Facility=System
SymbolicName=WSAEMFILE
Language=English
Too many open sockets.
.
Language=Russian
Too many open sockets.
.
Language=Polish
Too many open sockets.
.
Language=Romanian
Too many open sockets.
.

MessageId=10035
Severity=Success
Facility=System
SymbolicName=WSAEWOULDBLOCK
Language=English
A non-blocking socket operation could not be completed immediately.
.
Language=Russian
A non-blocking socket operation could not be completed immediately.
.
Language=Polish
A non-blocking socket operation could not be completed immediately.
.
Language=Romanian
A non-blocking socket operation could not be completed immediately.
.

MessageId=10036
Severity=Success
Facility=System
SymbolicName=WSAEINPROGRESS
Language=English
A blocking operation is currently executing.
.
Language=Russian
A blocking operation is currently executing.
.
Language=Polish
A blocking operation is currently executing.
.
Language=Romanian
A blocking operation is currently executing.
.

MessageId=10037
Severity=Success
Facility=System
SymbolicName=WSAEALREADY
Language=English
An operation was attempted on a non-blocking socket that already had an operation in progress.
.
Language=Russian
An operation was attempted on a non-blocking socket that already had an operation in progress.
.
Language=Polish
An operation was attempted on a non-blocking socket that already had an operation in progress.
.
Language=Romanian
An operation was attempted on a non-blocking socket that already had an operation in progress.
.

MessageId=10038
Severity=Success
Facility=System
SymbolicName=WSAENOTSOCK
Language=English
An operation was attempted on something that is not a socket.
.
Language=Russian
An operation was attempted on something that is not a socket.
.
Language=Polish
An operation was attempted on something that is not a socket.
.
Language=Romanian
An operation was attempted on something that is not a socket.
.

MessageId=10039
Severity=Success
Facility=System
SymbolicName=WSAEDESTADDRREQ
Language=English
A required address was omitted from an operation on a socket.
.
Language=Russian
A required address was omitted from an operation on a socket.
.
Language=Polish
A required address was omitted from an operation on a socket.
.
Language=Romanian
A required address was omitted from an operation on a socket.
.

MessageId=10040
Severity=Success
Facility=System
SymbolicName=WSAEMSGSIZE
Language=English
A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.
Language=Russian
A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.
Language=Polish
A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.
Language=Romanian
A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
.

MessageId=10041
Severity=Success
Facility=System
SymbolicName=WSAEPROTOTYPE
Language=English
A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.
Language=Russian
A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.
Language=Polish
A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.
Language=Romanian
A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
.

MessageId=10042
Severity=Success
Facility=System
SymbolicName=WSAENOPROTOOPT
Language=English
An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.
Language=Russian
An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.
Language=Polish
An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.
Language=Romanian
An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
.

MessageId=10043
Severity=Success
Facility=System
SymbolicName=WSAEPROTONOSUPPORT
Language=English
The requested protocol has not been configured into the system, or no implementation for it exists.
.
Language=Russian
The requested protocol has not been configured into the system, or no implementation for it exists.
.
Language=Polish
The requested protocol has not been configured into the system, or no implementation for it exists.
.
Language=Romanian
The requested protocol has not been configured into the system, or no implementation for it exists.
.

MessageId=10044
Severity=Success
Facility=System
SymbolicName=WSAESOCKTNOSUPPORT
Language=English
The support for the specified socket type does not exist in this address family.
.
Language=Russian
The support for the specified socket type does not exist in this address family.
.
Language=Polish
The support for the specified socket type does not exist in this address family.
.
Language=Romanian
The support for the specified socket type does not exist in this address family.
.

MessageId=10045
Severity=Success
Facility=System
SymbolicName=WSAEOPNOTSUPP
Language=English
The attempted operation is not supported for the type of object referenced.
.
Language=Russian
The attempted operation is not supported for the type of object referenced.
.
Language=Polish
The attempted operation is not supported for the type of object referenced.
.
Language=Romanian
The attempted operation is not supported for the type of object referenced.
.

MessageId=10046
Severity=Success
Facility=System
SymbolicName=WSAEPFNOSUPPORT
Language=English
The protocol family has not been configured into the system or no implementation for it exists.
.
Language=Russian
The protocol family has not been configured into the system or no implementation for it exists.
.
Language=Polish
The protocol family has not been configured into the system or no implementation for it exists.
.
Language=Romanian
The protocol family has not been configured into the system or no implementation for it exists.
.

MessageId=10047
Severity=Success
Facility=System
SymbolicName=WSAEAFNOSUPPORT
Language=English
An address incompatible with the requested protocol was used.
.
Language=Russian
An address incompatible with the requested protocol was used.
.
Language=Polish
An address incompatible with the requested protocol was used.
.
Language=Romanian
An address incompatible with the requested protocol was used.
.

MessageId=10048
Severity=Success
Facility=System
SymbolicName=WSAEADDRINUSE
Language=English
Only one usage of each socket address (protocol/network address/port) is normally permitted.
.
Language=Russian
Only one usage of each socket address (protocol/network address/port) is normally permitted.
.
Language=Polish
Only one usage of each socket address (protocol/network address/port) is normally permitted.
.
Language=Romanian
Only one usage of each socket address (protocol/network address/port) is normally permitted.
.

MessageId=10049
Severity=Success
Facility=System
SymbolicName=WSAEADDRNOTAVAIL
Language=English
The requested address is not valid in its context.
.
Language=Russian
The requested address is not valid in its context.
.
Language=Polish
The requested address is not valid in its context.
.
Language=Romanian
The requested address is not valid in its context.
.

MessageId=10050
Severity=Success
Facility=System
SymbolicName=WSAENETDOWN
Language=English
A socket operation encountered a dead network.
.
Language=Russian
A socket operation encountered a dead network.
.
Language=Polish
A socket operation encountered a dead network.
.
Language=Romanian
A socket operation encountered a dead network.
.

MessageId=10051
Severity=Success
Facility=System
SymbolicName=WSAENETUNREACH
Language=English
A socket operation was attempted to an unreachable network.
.
Language=Russian
A socket operation was attempted to an unreachable network.
.
Language=Polish
A socket operation was attempted to an unreachable network.
.
Language=Romanian
A socket operation was attempted to an unreachable network.
.

MessageId=10052
Severity=Success
Facility=System
SymbolicName=WSAENETRESET
Language=English
The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.
Language=Russian
The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.
Language=Polish
The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.
Language=Romanian
The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
.

MessageId=10053
Severity=Success
Facility=System
SymbolicName=WSAECONNABORTED
Language=English
An established connection was aborted by the software in your host machine.
.
Language=Russian
An established connection was aborted by the software in your host machine.
.
Language=Polish
An established connection was aborted by the software in your host machine.
.
Language=Romanian
An established connection was aborted by the software in your host machine.
.

MessageId=10054
Severity=Success
Facility=System
SymbolicName=WSAECONNRESET
Language=English
An existing connection was forcibly closed by the remote host.
.
Language=Russian
An existing connection was forcibly closed by the remote host.
.
Language=Polish
An existing connection was forcibly closed by the remote host.
.
Language=Romanian
An existing connection was forcibly closed by the remote host.
.

MessageId=10055
Severity=Success
Facility=System
SymbolicName=WSAENOBUFS
Language=English
An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.
Language=Russian
An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.
Language=Polish
An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.
Language=Romanian
An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
.

MessageId=10056
Severity=Success
Facility=System
SymbolicName=WSAEISCONN
Language=English
A connect request was made on an already connected socket.
.
Language=Russian
A connect request was made on an already connected socket.
.
Language=Polish
A connect request was made on an already connected socket.
.
Language=Romanian
A connect request was made on an already connected socket.
.

MessageId=10057
Severity=Success
Facility=System
SymbolicName=WSAENOTCONN
Language=English
A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.
Language=Russian
A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.
Language=Polish
A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.
Language=Romanian
A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
.

MessageId=10058
Severity=Success
Facility=System
SymbolicName=WSAESHUTDOWN
Language=English
A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.
Language=Russian
A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.
Language=Polish
A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.
Language=Romanian
A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
.

MessageId=10059
Severity=Success
Facility=System
SymbolicName=WSAETOOMANYREFS
Language=English
Too many references to some kernel object.
.
Language=Russian
Too many references to some kernel object.
.
Language=Polish
Too many references to some kernel object.
.
Language=Romanian
Too many references to some kernel object.
.

MessageId=10060
Severity=Success
Facility=System
SymbolicName=WSAETIMEDOUT
Language=English
A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.
Language=Russian
A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.
Language=Polish
A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.
Language=Romanian
A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
.

MessageId=10061
Severity=Success
Facility=System
SymbolicName=WSAECONNREFUSED
Language=English
No connection could be made because the target machine actively refused it.
.
Language=Russian
No connection could be made because the target machine actively refused it.
.
Language=Polish
No connection could be made because the target machine actively refused it.
.
Language=Romanian
No connection could be made because the target machine actively refused it.
.

MessageId=10062
Severity=Success
Facility=System
SymbolicName=WSAELOOP
Language=English
Cannot translate name.
.
Language=Russian
Cannot translate name.
.
Language=Polish
Cannot translate name.
.
Language=Romanian
Cannot translate name.
.

MessageId=10063
Severity=Success
Facility=System
SymbolicName=WSAENAMETOOLONG
Language=English
Name component or name was too long.
.
Language=Russian
Name component or name was too long.
.
Language=Polish
Name component or name was too long.
.
Language=Romanian
Name component or name was too long.
.

MessageId=10064
Severity=Success
Facility=System
SymbolicName=WSAEHOSTDOWN
Language=English
A socket operation failed because the destination host was down.
.
Language=Russian
A socket operation failed because the destination host was down.
.
Language=Polish
A socket operation failed because the destination host was down.
.
Language=Romanian
A socket operation failed because the destination host was down.
.

MessageId=10065
Severity=Success
Facility=System
SymbolicName=WSAEHOSTUNREACH
Language=English
A socket operation was attempted to an unreachable host.
.
Language=Russian
A socket operation was attempted to an unreachable host.
.
Language=Polish
A socket operation was attempted to an unreachable host.
.
Language=Romanian
A socket operation was attempted to an unreachable host.
.

MessageId=10066
Severity=Success
Facility=System
SymbolicName=WSAENOTEMPTY
Language=English
Cannot remove a directory that is not empty.
.
Language=Russian
Cannot remove a directory that is not empty.
.
Language=Polish
Cannot remove a directory that is not empty.
.
Language=Romanian
Cannot remove a directory that is not empty.
.

MessageId=10067
Severity=Success
Facility=System
SymbolicName=WSAEPROCLIM
Language=English
A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.
Language=Russian
A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.
Language=Polish
A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.
Language=Romanian
A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.
.

MessageId=10068
Severity=Success
Facility=System
SymbolicName=WSAEUSERS
Language=English
Ran out of quota.
.
Language=Russian
Ran out of quota.
.
Language=Polish
Ran out of quota.
.
Language=Romanian
Ran out of quota.
.

MessageId=10069
Severity=Success
Facility=System
SymbolicName=WSAEDQUOT
Language=English
Ran out of disk quota.
.
Language=Russian
Ran out of disk quota.
.
Language=Polish
Ran out of disk quota.
.
Language=Romanian
Ran out of disk quota.
.

MessageId=10070
Severity=Success
Facility=System
SymbolicName=WSAESTALE
Language=English
File handle reference is no longer available.
.
Language=Russian
File handle reference is no longer available.
.
Language=Polish
File handle reference is no longer available.
.
Language=Romanian
File handle reference is no longer available.
.

MessageId=10071
Severity=Success
Facility=System
SymbolicName=WSAEREMOTE
Language=English
Item is not available locally.
.
Language=Russian
Item is not available locally.
.
Language=Polish
Item is not available locally.
.
Language=Romanian
Item is not available locally.
.

MessageId=10091
Severity=Success
Facility=System
SymbolicName=WSASYSNOTREADY
Language=English
WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.
Language=Russian
WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.
Language=Polish
WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.
Language=Romanian
WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.
.

MessageId=10092
Severity=Success
Facility=System
SymbolicName=WSAVERNOTSUPPORTED
Language=English
The Windows Sockets version requested is not supported.
.
Language=Russian
The Windows Sockets version requested is not supported.
.
Language=Polish
The Windows Sockets version requested is not supported.
.
Language=Romanian
The Windows Sockets version requested is not supported.
.

MessageId=10093
Severity=Success
Facility=System
SymbolicName=WSANOTINITIALISED
Language=English
Either the application has not called WSAStartup, or WSAStartup failed.
.
Language=Russian
Either the application has not called WSAStartup, or WSAStartup failed.
.
Language=Polish
Either the application has not called WSAStartup, or WSAStartup failed.
.
Language=Romanian
Either the application has not called WSAStartup, or WSAStartup failed.
.

MessageId=10101
Severity=Success
Facility=System
SymbolicName=WSAEDISCON
Language=English
Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.
Language=Russian
Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.
Language=Polish
Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.
Language=Romanian
Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.
.

MessageId=10102
Severity=Success
Facility=System
SymbolicName=WSAENOMORE
Language=English
No more results can be returned by WSALookupServiceNext.
.
Language=Russian
No more results can be returned by WSALookupServiceNext.
.
Language=Polish
No more results can be returned by WSALookupServiceNext.
.
Language=Romanian
No more results can be returned by WSALookupServiceNext.
.

MessageId=10103
Severity=Success
Facility=System
SymbolicName=WSAECANCELLED
Language=English
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Russian
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Polish
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Romanian
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10104
Severity=Success
Facility=System
SymbolicName=WSAEINVALIDPROCTABLE
Language=English
The procedure call table is invalid.
.
Language=Russian
The procedure call table is invalid.
.
Language=Polish
The procedure call table is invalid.
.
Language=Romanian
The procedure call table is invalid.
.

MessageId=10105
Severity=Success
Facility=System
SymbolicName=WSAEINVALIDPROVIDER
Language=English
The requested service provider is invalid.
.
Language=Russian
The requested service provider is invalid.
.
Language=Polish
The requested service provider is invalid.
.
Language=Romanian
The requested service provider is invalid.
.

MessageId=10106
Severity=Success
Facility=System
SymbolicName=WSAEPROVIDERFAILEDINIT
Language=English
The requested service provider could not be loaded or initialized.
.
Language=Russian
The requested service provider could not be loaded or initialized.
.
Language=Polish
The requested service provider could not be loaded or initialized.
.
Language=Romanian
The requested service provider could not be loaded or initialized.
.

MessageId=10107
Severity=Success
Facility=System
SymbolicName=WSASYSCALLFAILURE
Language=English
A system call that should never fail has failed.
.
Language=Russian
A system call that should never fail has failed.
.
Language=Polish
A system call that should never fail has failed.
.
Language=Romanian
A system call that should never fail has failed.
.

MessageId=10108
Severity=Success
Facility=System
SymbolicName=WSASERVICE_NOT_FOUND
Language=English
No such service is known. The service cannot be found in the specified name space.
.
Language=Russian
No such service is known. The service cannot be found in the specified name space.
.
Language=Polish
No such service is known. The service cannot be found in the specified name space.
.
Language=Romanian
No such service is known. The service cannot be found in the specified name space.
.

MessageId=10109
Severity=Success
Facility=System
SymbolicName=WSATYPE_NOT_FOUND
Language=English
The specified class was not found.
.
Language=Russian
The specified class was not found.
.
Language=Polish
The specified class was not found.
.
Language=Romanian
The specified class was not found.
.

MessageId=10110
Severity=Success
Facility=System
SymbolicName=WSA_E_NO_MORE
Language=English
No more results can be returned by WSALookupServiceNext.
.
Language=Russian
No more results can be returned by WSALookupServiceNext.
.
Language=Polish
No more results can be returned by WSALookupServiceNext.
.
Language=Romanian
No more results can be returned by WSALookupServiceNext.
.

MessageId=10111
Severity=Success
Facility=System
SymbolicName=WSA_E_CANCELLED
Language=English
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Russian
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Polish
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.
Language=Romanian
A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.
.

MessageId=10112
Severity=Success
Facility=System
SymbolicName=WSAEREFUSED
Language=English
A database query failed because it was actively refused.
.
Language=Russian
A database query failed because it was actively refused.
.
Language=Polish
A database query failed because it was actively refused.
.
Language=Romanian
A database query failed because it was actively refused.
.

MessageId=11001
Severity=Success
Facility=System
SymbolicName=WSAHOST_NOT_FOUND
Language=English
No such host is known.
.
Language=Russian
No such host is known.
.
Language=Polish
No such host is known.
.
Language=Romanian
No such host is known.
.

MessageId=11002
Severity=Success
Facility=System
SymbolicName=WSATRY_AGAIN
Language=English
This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.
Language=Russian
This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.
Language=Polish
This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.
Language=Romanian
This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.
.

MessageId=11003
Severity=Success
Facility=System
SymbolicName=WSANO_RECOVERY
Language=English
A non-recoverable error occurred during a database lookup.
.
Language=Russian
A non-recoverable error occurred during a database lookup.
.
Language=Polish
A non-recoverable error occurred during a database lookup.
.
Language=Romanian
A non-recoverable error occurred during a database lookup.
.

MessageId=11004
Severity=Success
Facility=System
SymbolicName=WSANO_DATA
Language=English
The requested name is valid, but no data of the requested type was found.
.
Language=Russian
The requested name is valid, but no data of the requested type was found.
.
Language=Polish
The requested name is valid, but no data of the requested type was found.
.
Language=Romanian
The requested name is valid, but no data of the requested type was found.
.

MessageId=11005
Severity=Success
Facility=System
SymbolicName=WSA_QOS_RECEIVERS
Language=English
At least one reserve has arrived.
.
Language=Russian
At least one reserve has arrived.
.
Language=Polish
At least one reserve has arrived.
.
Language=Romanian
At least one reserve has arrived.
.

MessageId=11006
Severity=Success
Facility=System
SymbolicName=WSA_QOS_SENDERS
Language=English
At least one path has arrived.
.
Language=Russian
At least one path has arrived.
.
Language=Polish
At least one path has arrived.
.
Language=Romanian
At least one path has arrived.
.

MessageId=11007
Severity=Success
Facility=System
SymbolicName=WSA_QOS_NO_SENDERS
Language=English
There are no senders.
.
Language=Russian
There are no senders.
.
Language=Polish
There are no senders.
.
Language=Romanian
There are no senders.
.

MessageId=11008
Severity=Success
Facility=System
SymbolicName=WSA_QOS_NO_RECEIVERS
Language=English
There are no receivers.
.
Language=Russian
There are no receivers.
.
Language=Polish
There are no receivers.
.
Language=Romanian
There are no receivers.
.

MessageId=11009
Severity=Success
Facility=System
SymbolicName=WSA_QOS_REQUEST_CONFIRMED
Language=English
Reserve has been confirmed.
.
Language=Russian
Reserve has been confirmed.
.
Language=Polish
Reserve has been confirmed.
.
Language=Romanian
Reserve has been confirmed.
.

MessageId=11010
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ADMISSION_FAILURE
Language=English
Error due to lack of resources.
.
Language=Russian
Error due to lack of resources.
.
Language=Polish
Error due to lack of resources.
.
Language=Romanian
Error due to lack of resources.
.

MessageId=11011
Severity=Success
Facility=System
SymbolicName=WSA_QOS_POLICY_FAILURE
Language=English
Rejected for administrative reasons - bad credentials.
.
Language=Russian
Rejected for administrative reasons - bad credentials.
.
Language=Polish
Rejected for administrative reasons - bad credentials.
.
Language=Romanian
Rejected for administrative reasons - bad credentials.
.

MessageId=11012
Severity=Success
Facility=System
SymbolicName=WSA_QOS_BAD_STYLE
Language=English
Unknown or conflicting style.
.
Language=Russian
Unknown or conflicting style.
.
Language=Polish
Unknown or conflicting style.
.
Language=Romanian
Unknown or conflicting style.
.

MessageId=11013
Severity=Success
Facility=System
SymbolicName=WSA_QOS_BAD_OBJECT
Language=English
Problem with some part of the filterspec or providerspecific buffer in general.
.
Language=Russian
Problem with some part of the filterspec or providerspecific buffer in general.
.
Language=Polish
Problem with some part of the filterspec or providerspecific buffer in general.
.
Language=Romanian
Problem with some part of the filterspec or providerspecific buffer in general.
.

MessageId=11014
Severity=Success
Facility=System
SymbolicName=WSA_QOS_TRAFFIC_CTRL_ERROR
Language=English
Problem with some part of the flowspec.
.
Language=Russian
Problem with some part of the flowspec.
.
Language=Polish
Problem with some part of the flowspec.
.
Language=Romanian
Problem with some part of the flowspec.
.

MessageId=11015
Severity=Success
Facility=System
SymbolicName=WSA_QOS_GENERIC_ERROR
Language=English
General QOS error.
.
Language=Russian
General QOS error.
.
Language=Polish
General QOS error.
.
Language=Romanian
General QOS error.
.

MessageId=11016
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESERVICETYPE
Language=English
An invalid or unrecognized service type was found in the flowspec.
.
Language=Russian
An invalid or unrecognized service type was found in the flowspec.
.
Language=Polish
An invalid or unrecognized service type was found in the flowspec.
.
Language=Romanian
An invalid or unrecognized service type was found in the flowspec.
.

MessageId=11017
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWSPEC
Language=English
An invalid or inconsistent flowspec was found in the QOS structure.
.
Language=Russian
An invalid or inconsistent flowspec was found in the QOS structure.
.
Language=Polish
An invalid or inconsistent flowspec was found in the QOS structure.
.
Language=Romanian
An invalid or inconsistent flowspec was found in the QOS structure.
.

MessageId=11018
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPROVSPECBUF
Language=English
Invalid QOS provider-specific buffer.
.
Language=Russian
Invalid QOS provider-specific buffer.
.
Language=Polish
Invalid QOS provider-specific buffer.
.
Language=Romanian
Invalid QOS provider-specific buffer.
.

MessageId=11019
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERSTYLE
Language=English
An invalid QOS filter style was used.
.
Language=Russian
An invalid QOS filter style was used.
.
Language=Polish
An invalid QOS filter style was used.
.
Language=Romanian
An invalid QOS filter style was used.
.

MessageId=11020
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERTYPE
Language=English
An invalid QOS filter type was used.
.
Language=Russian
An invalid QOS filter type was used.
.
Language=Polish
An invalid QOS filter type was used.
.
Language=Romanian
An invalid QOS filter type was used.
.

MessageId=11021
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFILTERCOUNT
Language=English
An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.
Language=Russian
An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.
Language=Polish
An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.
Language=Romanian
An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.
.

MessageId=11022
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EOBJLENGTH
Language=English
An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.
Language=Russian
An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.
Language=Polish
An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.
Language=Romanian
An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.
.

MessageId=11023
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWCOUNT
Language=English
An incorrect number of flow descriptors was specified in the QOS structure.
.
Language=Russian
An incorrect number of flow descriptors was specified in the QOS structure.
.
Language=Polish
An incorrect number of flow descriptors was specified in the QOS structure.
.
Language=Romanian
An incorrect number of flow descriptors was specified in the QOS structure.
.

MessageId=11024
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EUNKNOWNPSOBJ
Language=English
An unrecognized object was found in the QOS provider-specific buffer.
.
Language=Russian
An unrecognized object was found in the QOS provider-specific buffer.
.
Language=Polish
An unrecognized object was found in the QOS provider-specific buffer.
.
Language=Romanian
An unrecognized object was found in the QOS provider-specific buffer.
.

MessageId=11025
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPOLICYOBJ
Language=English
An invalid policy object was found in the QOS provider-specific buffer.
.
Language=Russian
An invalid policy object was found in the QOS provider-specific buffer.
.
Language=Polish
An invalid policy object was found in the QOS provider-specific buffer.
.
Language=Romanian
An invalid policy object was found in the QOS provider-specific buffer.
.

MessageId=11026
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EFLOWDESC
Language=English
An invalid QOS flow descriptor was found in the flow descriptor list.
.
Language=Russian
An invalid QOS flow descriptor was found in the flow descriptor list.
.
Language=Polish
An invalid QOS flow descriptor was found in the flow descriptor list.
.
Language=Romanian
An invalid QOS flow descriptor was found in the flow descriptor list.
.

MessageId=11027
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPSFLOWSPEC
Language=English
An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.
.
Language=Russian
An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.
.
Language=Polish
An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.
.
Language=Romanian
An invalid or inconsistent flowspec was found in the QOS provider-specific buffer.
.

MessageId=11028
Severity=Success
Facility=System
SymbolicName=WSA_QOS_EPSFILTERSPEC
Language=English
An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.
Language=Russian
An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.
Language=Polish
An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.
Language=Romanian
An invalid FILTERSPEC was found in the QOS provider-specific buffer.
.

MessageId=11029
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESDMODEOBJ
Language=English
An invalid shape discard mode object was found in the QOS provider-specific buffer.
.
Language=Russian
An invalid shape discard mode object was found in the QOS provider-specific buffer.
.
Language=Polish
An invalid shape discard mode object was found in the QOS provider-specific buffer.
.
Language=Romanian
An invalid shape discard mode object was found in the QOS provider-specific buffer.
.

MessageId=11030
Severity=Success
Facility=System
SymbolicName=WSA_QOS_ESHAPERATEOBJ
Language=English
An invalid shaping rate object was found in the QOS provider-specific buffer.
.
Language=Russian
An invalid shaping rate object was found in the QOS provider-specific buffer.
.
Language=Polish
An invalid shaping rate object was found in the QOS provider-specific buffer.
.
Language=Romanian
An invalid shaping rate object was found in the QOS provider-specific buffer.
.

MessageId=11031
Severity=Success
Facility=System
SymbolicName=WSA_QOS_RESERVED_PETYPE
Language=English
A reserved policy element was found in the QOS provider-specific buffer.
.
Language=Russian
A reserved policy element was found in the QOS provider-specific buffer.
.
Language=Polish
A reserved policy element was found in the QOS provider-specific buffer.
.
Language=Romanian
A reserved policy element was found in the QOS provider-specific buffer.
.

MessageId=12000
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_IO_COMPLETE
Language=English
The IO was completed by a filter.
.
Language=Russian
The IO was completed by a filter.
.
Language=Polish
The IO was completed by a filter.
.
Language=Romanian
The IO was completed by a filter.
.

MessageId=12001
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_BUFFER_TOO_SMALL
Language=English
The buffer is too small to contain the entry. No information has been written to the buffer.
.
Language=Russian
The buffer is too small to contain the entry. No information has been written to the buffer.
.
Language=Polish
The buffer is too small to contain the entry. No information has been written to the buffer.
.
Language=Romanian
The buffer is too small to contain the entry. No information has been written to the buffer.
.

MessageId=12002
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_HANDLER_DEFINED
Language=English
A handler was not defined by the filter for this operation.
.
Language=Russian
A handler was not defined by the filter for this operation.
.
Language=Polish
A handler was not defined by the filter for this operation.
.
Language=Romanian
A handler was not defined by the filter for this operation.
.

MessageId=12003
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CONTEXT_ALREADY_DEFINED
Language=English
A context is already defined for this object.
.
Language=Russian
A context is already defined for this object.
.
Language=Polish
A context is already defined for this object.
.
Language=Romanian
A context is already defined for this object.
.

MessageId=12004
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_ASYNCHRONOUS_REQUEST
Language=English
Asynchronous requests are not valid for this operation.
.
Language=Russian
Asynchronous requests are not valid for this operation.
.
Language=Polish
Asynchronous requests are not valid for this operation.
.
Language=Romanian
Asynchronous requests are not valid for this operation.
.

MessageId=12005
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DISALLOW_FAST_IO
Language=English
Disallow the Fast IO path for this operation.
.
Language=Russian
Disallow the Fast IO path for this operation.
.
Language=Polish
Disallow the Fast IO path for this operation.
.
Language=Romanian
Disallow the Fast IO path for this operation.
.

MessageId=12006
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_NAME_REQUEST
Language=English
An invalid name request was made. The name requested cannot be retrieved at this time.
.
Language=Russian
An invalid name request was made. The name requested cannot be retrieved at this time.
.
Language=Polish
An invalid name request was made. The name requested cannot be retrieved at this time.
.
Language=Romanian
An invalid name request was made. The name requested cannot be retrieved at this time.
.

MessageId=12007
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NOT_SAFE_TO_POST_OPERATION
Language=English
Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.
.
Language=Russian
Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.
.
Language=Polish
Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.
.
Language=Romanian
Posting this operation to a worker thread for further processing is not safe at this time because it could lead to a system deadlock.
.

MessageId=12008
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NOT_INITIALIZED
Language=English
The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver.
.
Language=Russian
The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver.
.
Language=Polish
The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver.
.
Language=Romanian
The Filter Manager was not initialized when a filter tried to register. Make sure that the Filter Manager is getting loaded as a driver.
.

MessageId=12009
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_FILTER_NOT_READY
Language=English
The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).
.
Language=Russian
The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).
.
Language=Polish
The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).
.
Language=Romanian
The filter is not ready for attachment to volumes because it has not finished initializing (FltStartFiltering has not been called).
.

MessageId=12010
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_POST_OPERATION_CLEANUP
Language=English
The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.
.
Language=Russian
The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.
.
Language=Polish
The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.
.
Language=Romanian
The filter must cleanup any operation specific context at this time because it is being removed from the system before the operation is completed by the lower drivers.
.

MessageId=12011
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INTERNAL_ERROR
Language=English
The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback.
.
Language=Russian
The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback.
.
Language=Polish
The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback.
.
Language=Romanian
The Filter Manager had an internal error from which it cannot recover, therefore the operation has been failed. This is usually the result of a filter returning an invalid value from a pre-operation callback.
.

MessageId=12012
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DELETING_OBJECT
Language=English
The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.
.
Language=Russian
The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.
.
Language=Polish
The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.
.
Language=Romanian
The object specified for this action is in the process of being deleted, therefore the action requested cannot be completed at this time.
.

MessageId=12013
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_MUST_BE_NONPAGED_POOL
Language=English
Non-paged pool must be used for this type of context.
.
Language=Russian
Non-paged pool must be used for this type of context.
.
Language=Polish
Non-paged pool must be used for this type of context.
.
Language=Romanian
Non-paged pool must be used for this type of context.
.

MessageId=12014
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DUPLICATE_ENTRY
Language=English
A duplicate handler definition has been provided for an operation.
.
Language=Russian
A duplicate handler definition has been provided for an operation.
.
Language=Polish
A duplicate handler definition has been provided for an operation.
.
Language=Romanian
A duplicate handler definition has been provided for an operation.
.

MessageId=12015
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CBDQ_DISABLED
Language=English
The callback data queue has been disabled.
.
Language=Russian
The callback data queue has been disabled.
.
Language=Polish
The callback data queue has been disabled.
.
Language=Romanian
The callback data queue has been disabled.
.

MessageId=12016
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DO_NOT_ATTACH
Language=English
Do not attach the filter to the volume at this time.
.
Language=Russian
Do not attach the filter to the volume at this time.
.
Language=Polish
Do not attach the filter to the volume at this time.
.
Language=Romanian
Do not attach the filter to the volume at this time.
.

MessageId=12017
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_DO_NOT_DETACH
Language=English
Do not detach the filter from the volume at this time.
.
Language=Russian
Do not detach the filter from the volume at this time.
.
Language=Polish
Do not detach the filter from the volume at this time.
.
Language=Romanian
Do not detach the filter from the volume at this time.
.

MessageId=12018
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_ALTITUDE_COLLISION
Language=English
An instance already exists at this altitude on the volume specified.
.
Language=Russian
An instance already exists at this altitude on the volume specified.
.
Language=Polish
An instance already exists at this altitude on the volume specified.
.
Language=Romanian
An instance already exists at this altitude on the volume specified.
.

MessageId=12019
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_NAME_COLLISION
Language=English
An instance already exists with this name on the volume specified.
.
Language=Russian
An instance already exists with this name on the volume specified.
.
Language=Polish
An instance already exists with this name on the volume specified.
.
Language=Romanian
An instance already exists with this name on the volume specified.
.

MessageId=12020
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_FILTER_NOT_FOUND
Language=English
The system could not find the filter specified.
.
Language=Russian
The system could not find the filter specified.
.
Language=Polish
The system could not find the filter specified.
.
Language=Romanian
The system could not find the filter specified.
.

MessageId=12021
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_VOLUME_NOT_FOUND
Language=English
The system could not find the volume specified.
.
Language=Russian
The system could not find the volume specified.
.
Language=Polish
The system could not find the volume specified.
.
Language=Romanian
The system could not find the volume specified.
.

MessageId=12022
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INSTANCE_NOT_FOUND
Language=English
The system could not find the instance specified.
.
Language=Russian
The system could not find the instance specified.
.
Language=Polish
The system could not find the instance specified.
.
Language=Romanian
The system could not find the instance specified.
.

MessageId=12023
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_CONTEXT_ALLOCATION_NOT_FOUND
Language=English
No registered context allocation definition was found for the given request.
.
Language=Russian
No registered context allocation definition was found for the given request.
.
Language=Polish
No registered context allocation definition was found for the given request.
.
Language=Romanian
No registered context allocation definition was found for the given request.
.

MessageId=12024
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_INVALID_CONTEXT_REGISTRATION
Language=English
An invalid parameter was specified during context registration.
.
Language=Russian
An invalid parameter was specified during context registration.
.
Language=Polish
An invalid parameter was specified during context registration.
.
Language=Romanian
An invalid parameter was specified during context registration.
.

MessageId=12025
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NAME_CACHE_MISS
Language=English
The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.
.
Language=Russian
The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.
.
Language=Polish
The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.
.
Language=Romanian
The name requested was not found in Filter Manager's name cache and could not be retrieved from the file system.
.

MessageId=12026
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_DEVICE_OBJECT
Language=English
The requested device object does not exist for the given volume.
.
Language=Russian
The requested device object does not exist for the given volume.
.
Language=Polish
The requested device object does not exist for the given volume.
.
Language=Romanian
The requested device object does not exist for the given volume.
.

MessageId=12027
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_VOLUME_ALREADY_MOUNTED
Language=English
The specified volume is already mounted.
.
Language=Russian
The specified volume is already mounted.
.
Language=Polish
The specified volume is already mounted.
.
Language=Romanian
The specified volume is already mounted.
.

MessageId=12028
Severity=Success
Facility=System
SymbolicName=ERROR_FLT_NO_WAITER_FOR_REPLY
Language=English
No waiter is present for the filter's reply to this message.
.
Language=Russian
No waiter is present for the filter's reply to this message.
.
Language=Polish
No waiter is present for the filter's reply to this message.
.
Language=Romanian
No waiter is present for the filter's reply to this message.
.

MessageId=13000
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_EXISTS
Language=English
The specified quick mode policy already exists.
.
Language=Russian
The specified quick mode policy already exists.
.
Language=Polish
The specified quick mode policy already exists.
.
Language=Romanian
The specified quick mode policy already exists.
.

MessageId=13001
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_NOT_FOUND
Language=English
The specified quick mode policy was not found.
.
Language=Russian
The specified quick mode policy was not found.
.
Language=Polish
The specified quick mode policy was not found.
.
Language=Romanian
The specified quick mode policy was not found.
.

MessageId=13002
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_IN_USE
Language=English
The specified quick mode policy is being used.
.
Language=Russian
The specified quick mode policy is being used.
.
Language=Polish
The specified quick mode policy is being used.
.
Language=Romanian
The specified quick mode policy is being used.
.

MessageId=13003
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_EXISTS
Language=English
The specified main mode policy already exists.
.
Language=Russian
The specified main mode policy already exists.
.
Language=Polish
The specified main mode policy already exists.
.
Language=Romanian
The specified main mode policy already exists.
.

MessageId=13004
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_NOT_FOUND
Language=English
The specified main mode policy was not found.
.
Language=Russian
The specified main mode policy was not found.
.
Language=Polish
The specified main mode policy was not found.
.
Language=Romanian
The specified main mode policy was not found.
.

MessageId=13005
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_IN_USE
Language=English
The specified main mode policy is being used.
.
Language=Russian
The specified main mode policy is being used.
.
Language=Polish
The specified main mode policy is being used.
.
Language=Romanian
The specified main mode policy is being used.
.

MessageId=13006
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_EXISTS
Language=English
The specified main mode filter already exists.
.
Language=Russian
The specified main mode filter already exists.
.
Language=Polish
The specified main mode filter already exists.
.
Language=Romanian
The specified main mode filter already exists.
.

MessageId=13007
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_NOT_FOUND
Language=English
The specified main mode filter was not found.
.
Language=Russian
The specified main mode filter was not found.
.
Language=Polish
The specified main mode filter was not found.
.
Language=Romanian
The specified main mode filter was not found.
.

MessageId=13008
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_EXISTS
Language=English
The specified transport mode filter already exists.
.
Language=Russian
The specified transport mode filter already exists.
.
Language=Polish
The specified transport mode filter already exists.
.
Language=Romanian
The specified transport mode filter already exists.
.

MessageId=13009
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_NOT_FOUND
Language=English
The specified transport mode filter does not exist.
.
Language=Russian
The specified transport mode filter does not exist.
.
Language=Polish
The specified transport mode filter does not exist.
.
Language=Romanian
The specified transport mode filter does not exist.
.

MessageId=13010
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_EXISTS
Language=English
The specified main mode authentication list exists.
.
Language=Russian
The specified main mode authentication list exists.
.
Language=Polish
The specified main mode authentication list exists.
.
Language=Romanian
The specified main mode authentication list exists.
.

MessageId=13011
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_NOT_FOUND
Language=English
The specified main mode authentication list was not found.
.
Language=Russian
The specified main mode authentication list was not found.
.
Language=Polish
The specified main mode authentication list was not found.
.
Language=Romanian
The specified main mode authentication list was not found.
.

MessageId=13012
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_IN_USE
Language=English
The specified quick mode policy is being used.
.
Language=Russian
The specified quick mode policy is being used.
.
Language=Polish
The specified quick mode policy is being used.
.
Language=Romanian
The specified quick mode policy is being used.
.

MessageId=13013
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND
Language=English
The specified main mode policy was not found.
.
Language=Russian
The specified main mode policy was not found.
.
Language=Polish
The specified main mode policy was not found.
.
Language=Romanian
The specified main mode policy was not found.
.

MessageId=13014
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND
Language=English
The specified quick mode policy was not found.
.
Language=Russian
The specified quick mode policy was not found.
.
Language=Polish
The specified quick mode policy was not found.
.
Language=Romanian
The specified quick mode policy was not found.
.

MessageId=13015
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND
Language=English
The manifest file contains one or more syntax errors.
.
Language=Russian
The manifest file contains one or more syntax errors.
.
Language=Polish
The manifest file contains one or more syntax errors.
.
Language=Romanian
The manifest file contains one or more syntax errors.
.

MessageId=13016
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_EXISTS
Language=English
The application attempted to activate a disabled activation context.
.
Language=Russian
The application attempted to activate a disabled activation context.
.
Language=Polish
The application attempted to activate a disabled activation context.
.
Language=Romanian
The application attempted to activate a disabled activation context.
.

MessageId=13017
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_NOT_FOUND
Language=English
The requested lookup key was not found in any active activation context.
.
Language=Russian
The requested lookup key was not found in any active activation context.
.
Language=Polish
The requested lookup key was not found in any active activation context.
.
Language=Romanian
The requested lookup key was not found in any active activation context.
.

MessageId=13018
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_FILTER_PENDING_DELETION
Language=English
The Main Mode filter is pending deletion.
.
Language=Russian
The Main Mode filter is pending deletion.
.
Language=Polish
The Main Mode filter is pending deletion.
.
Language=Romanian
The Main Mode filter is pending deletion.
.

MessageId=13019
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TRANSPORT_FILTER_PENDING_DELETION
Language=English
The transport filter is pending deletion.
.
Language=Russian
The transport filter is pending deletion.
.
Language=Polish
The transport filter is pending deletion.
.
Language=Romanian
The transport filter is pending deletion.
.

MessageId=13020
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_TUNNEL_FILTER_PENDING_DELETION
Language=English
The tunnel filter is pending deletion.
.
Language=Russian
The tunnel filter is pending deletion.
.
Language=Polish
The tunnel filter is pending deletion.
.
Language=Romanian
The tunnel filter is pending deletion.
.

MessageId=13021
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_POLICY_PENDING_DELETION
Language=English
The Main Mode policy is pending deletion.
.
Language=Russian
The Main Mode policy is pending deletion.
.
Language=Polish
The Main Mode policy is pending deletion.
.
Language=Romanian
The Main Mode policy is pending deletion.
.

MessageId=13022
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_MM_AUTH_PENDING_DELETION
Language=English
The Main Mode authentication bundle is pending deletion.
.
Language=Russian
The Main Mode authentication bundle is pending deletion.
.
Language=Polish
The Main Mode authentication bundle is pending deletion.
.
Language=Romanian
The Main Mode authentication bundle is pending deletion.
.

MessageId=13023
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_QM_POLICY_PENDING_DELETION
Language=English
The Quick Mode policy is pending deletion.
.
Language=Russian
The Quick Mode policy is pending deletion.
.
Language=Polish
The Quick Mode policy is pending deletion.
.
Language=Romanian
The Quick Mode policy is pending deletion.
.

MessageId=13024
Severity=Success
Facility=System
SymbolicName=WARNING_IPSEC_MM_POLICY_PRUNED
Language=English
The Main Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Russian
The Main Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Polish
The Main Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Romanian
The Main Mode policy was successfully added, but some of the requested offers are not supported.
.

MessageId=13025
Severity=Success
Facility=System
SymbolicName=WARNING_IPSEC_QM_POLICY_PRUNED
Language=English
The Quick Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Russian
The Quick Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Polish
The Quick Mode policy was successfully added, but some of the requested offers are not supported.
.
Language=Romanian
The Quick Mode policy was successfully added, but some of the requested offers are not supported.
.

MessageId=13801
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_AUTH_FAIL
Language=English
IKE authentication credentials are unacceptable.
.
Language=Russian
IKE authentication credentials are unacceptable.
.
Language=Polish
IKE authentication credentials are unacceptable.
.
Language=Romanian
IKE authentication credentials are unacceptable.
.

MessageId=13802
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ATTRIB_FAIL
Language=English
IKE security attributes are unacceptable.
.
Language=Russian
IKE security attributes are unacceptable.
.
Language=Polish
IKE security attributes are unacceptable.
.
Language=Romanian
IKE security attributes are unacceptable.
.

MessageId=13803
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_PENDING
Language=English
IKE Negotiation in progress.
.
Language=Russian
IKE Negotiation in progress.
.
Language=Polish
IKE Negotiation in progress.
.
Language=Romanian
IKE Negotiation in progress.
.

MessageId=13804
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR
Language=English
General processing error.
.
Language=Russian
General processing error.
.
Language=Polish
General processing error.
.
Language=Romanian
General processing error.
.

MessageId=13805
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_TIMED_OUT
Language=English
Negotiation timed out.
.
Language=Russian
Negotiation timed out.
.
Language=Polish
Negotiation timed out.
.
Language=Romanian
Negotiation timed out.
.

MessageId=13806
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_CERT
Language=English
IKE failed to find valid machine certificate.
.
Language=Russian
IKE failed to find valid machine certificate.
.
Language=Polish
IKE failed to find valid machine certificate.
.
Language=Romanian
IKE failed to find valid machine certificate.
.

MessageId=13807
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SA_DELETED
Language=English
IKE SA deleted by peer before establishment completed.
.
Language=Russian
IKE SA deleted by peer before establishment completed.
.
Language=Polish
IKE SA deleted by peer before establishment completed.
.
Language=Romanian
IKE SA deleted by peer before establishment completed.
.

MessageId=13808
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SA_REAPED
Language=English
IKE SA deleted before establishment completed.
.
Language=Russian
IKE SA deleted before establishment completed.
.
Language=Polish
IKE SA deleted before establishment completed.
.
Language=Romanian
IKE SA deleted before establishment completed.
.

MessageId=13809
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_ACQUIRE_DROP
Language=English
Negotiation request sat in Queue too long.
.
Language=Russian
Negotiation request sat in Queue too long.
.
Language=Polish
Negotiation request sat in Queue too long.
.
Language=Romanian
Negotiation request sat in Queue too long.
.

MessageId=13810
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QM_ACQUIRE_DROP
Language=English
Negotiation request sat in Queue too long.
.
Language=Russian
Negotiation request sat in Queue too long.
.
Language=Polish
Negotiation request sat in Queue too long.
.
Language=Romanian
Negotiation request sat in Queue too long.
.

MessageId=13811
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_MM
Language=English
Negotiation request sat in Queue too long.
.
Language=Russian
Negotiation request sat in Queue too long.
.
Language=Polish
Negotiation request sat in Queue too long.
.
Language=Romanian
Negotiation request sat in Queue too long.
.

MessageId=13812
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QUEUE_DROP_NO_MM
Language=English
Negotiation request sat in Queue too long.
.
Language=Russian
Negotiation request sat in Queue too long.
.
Language=Polish
Negotiation request sat in Queue too long.
.
Language=Romanian
Negotiation request sat in Queue too long.
.

MessageId=13813
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DROP_NO_RESPONSE
Language=English
No response from peer.
.
Language=Russian
No response from peer.
.
Language=Polish
No response from peer.
.
Language=Romanian
No response from peer.
.

MessageId=13814
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_DELAY_DROP
Language=English
Negotiation took too long.
.
Language=Russian
Negotiation took too long.
.
Language=Polish
Negotiation took too long.
.
Language=Romanian
Negotiation took too long.
.

MessageId=13815
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_QM_DELAY_DROP
Language=English
Negotiation took too long.
.
Language=Russian
Negotiation took too long.
.
Language=Polish
Negotiation took too long.
.
Language=Romanian
Negotiation took too long.
.

MessageId=13816
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ERROR
Language=English
Unknown error occurred.
.
Language=Russian
Unknown error occurred.
.
Language=Polish
Unknown error occurred.
.
Language=Romanian
Unknown error occurred.
.

MessageId=13817
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_CRL_FAILED
Language=English
Certificate Revocation Check failed.
.
Language=Russian
Certificate Revocation Check failed.
.
Language=Polish
Certificate Revocation Check failed.
.
Language=Romanian
Certificate Revocation Check failed.
.

MessageId=13818
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_KEY_USAGE
Language=English
Invalid certificate key usage.
.
Language=Russian
Invalid certificate key usage.
.
Language=Polish
Invalid certificate key usage.
.
Language=Romanian
Invalid certificate key usage.
.

MessageId=13819
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_TYPE
Language=English
Invalid certificate type.
.
Language=Russian
Invalid certificate type.
.
Language=Polish
Invalid certificate type.
.
Language=Romanian
Invalid certificate type.
.

MessageId=13820
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PRIVATE_KEY
Language=English
No private key associated with machine certificate.
.
Language=Russian
No private key associated with machine certificate.
.
Language=Polish
No private key associated with machine certificate.
.
Language=Romanian
No private key associated with machine certificate.
.

MessageId=13822
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DH_FAIL
Language=English
Failure in Diffie-Hellman computation.
.
Language=Russian
Failure in Diffie-Hellman computation.
.
Language=Polish
Failure in Diffie-Hellman computation.
.
Language=Romanian
Failure in Diffie-Hellman computation.
.

MessageId=13824
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HEADER
Language=English
Invalid header.
.
Language=Russian
Invalid header.
.
Language=Polish
Invalid header.
.
Language=Romanian
Invalid header.
.

MessageId=13825
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_POLICY
Language=English
No policy configured.
.
Language=Russian
No policy configured.
.
Language=Polish
No policy configured.
.
Language=Romanian
No policy configured.
.

MessageId=13826
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SIGNATURE
Language=English
Failed to verify signature.
.
Language=Russian
Failed to verify signature.
.
Language=Polish
Failed to verify signature.
.
Language=Romanian
Failed to verify signature.
.

MessageId=13827
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_KERBEROS_ERROR
Language=English
Failed to authenticate using Kerberos.
.
Language=Russian
Failed to authenticate using Kerberos.
.
Language=Polish
Failed to authenticate using Kerberos.
.
Language=Romanian
Failed to authenticate using Kerberos.
.

MessageId=13828
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PUBLIC_KEY
Language=English
Peer's certificate did not have a public key.
.
Language=Russian
Peer's certificate did not have a public key.
.
Language=Polish
Peer's certificate did not have a public key.
.
Language=Romanian
Peer's certificate did not have a public key.
.

MessageId=13829
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR
Language=English
Error processing error payload.
.
Language=Russian
Error processing error payload.
.
Language=Polish
Error processing error payload.
.
Language=Romanian
Error processing error payload.
.

MessageId=13830
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SA
Language=English
Error processing SA payload.
.
Language=Russian
Error processing SA payload.
.
Language=Polish
Error processing SA payload.
.
Language=Romanian
Error processing SA payload.
.

MessageId=13831
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_PROP
Language=English
Error processing Proposal payload.
.
Language=Russian
Error processing Proposal payload.
.
Language=Polish
Error processing Proposal payload.
.
Language=Romanian
Error processing Proposal payload.
.

MessageId=13832
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_TRANS
Language=English
Error processing Transform payload.
.
Language=Russian
Error processing Transform payload.
.
Language=Polish
Error processing Transform payload.
.
Language=Romanian
Error processing Transform payload.
.

MessageId=13833
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_KE
Language=English
Error processing KE payload.
.
Language=Russian
Error processing KE payload.
.
Language=Polish
Error processing KE payload.
.
Language=Romanian
Error processing KE payload.
.

MessageId=13834
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_ID
Language=English
Error processing ID payload.
.
Language=Russian
Error processing ID payload.
.
Language=Polish
Error processing ID payload.
.
Language=Romanian
Error processing ID payload.
.

MessageId=13835
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT
Language=English
Error processing Cert payload.
.
Language=Russian
Error processing Cert payload.
.
Language=Polish
Error processing Cert payload.
.
Language=Romanian
Error processing Cert payload.
.

MessageId=13836
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_CERT_REQ
Language=English
Error processing Certificate Request payload.
.
Language=Russian
Error processing Certificate Request payload.
.
Language=Polish
Error processing Certificate Request payload.
.
Language=Romanian
Error processing Certificate Request payload.
.

MessageId=13837
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_HASH
Language=English
Error processing Hash payload.
.
Language=Russian
Error processing Hash payload.
.
Language=Polish
Error processing Hash payload.
.
Language=Romanian
Error processing Hash payload.
.

MessageId=13838
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_SIG
Language=English
Error processing Signature payload.
.
Language=Russian
Error processing Signature payload.
.
Language=Polish
Error processing Signature payload.
.
Language=Romanian
Error processing Signature payload.
.

MessageId=13839
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NONCE
Language=English
Error processing Nonce payload.
.
Language=Russian
Error processing Nonce payload.
.
Language=Polish
Error processing Nonce payload.
.
Language=Romanian
Error processing Nonce payload.
.

MessageId=13840
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_NOTIFY
Language=English
Error processing Notify payload.
.
Language=Russian
Error processing Notify payload.
.
Language=Polish
Error processing Notify payload.
.
Language=Romanian
Error processing Notify payload.
.

MessageId=13841
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_DELETE
Language=English
Error processing Delete Payload.
.
Language=Russian
Error processing Delete Payload.
.
Language=Polish
Error processing Delete Payload.
.
Language=Romanian
Error processing Delete Payload.
.

MessageId=13842
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PROCESS_ERR_VENDOR
Language=English
Error processing VendorId payload.
.
Language=Russian
Error processing VendorId payload.
.
Language=Polish
Error processing VendorId payload.
.
Language=Romanian
Error processing VendorId payload.
.

MessageId=13843
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_PAYLOAD
Language=English
Invalid payload received.
.
Language=Russian
Invalid payload received.
.
Language=Polish
Invalid payload received.
.
Language=Romanian
Invalid payload received.
.

MessageId=13844
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_LOAD_SOFT_SA
Language=English
Soft SA loaded.
.
Language=Russian
Soft SA loaded.
.
Language=Polish
Soft SA loaded.
.
Language=Romanian
Soft SA loaded.
.

MessageId=13845
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SOFT_SA_TORN_DOWN
Language=English
Soft SA torn down.
.
Language=Russian
Soft SA torn down.
.
Language=Polish
Soft SA torn down.
.
Language=Romanian
Soft SA torn down.
.

MessageId=13846
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_COOKIE
Language=English
Invalid cookie received.
.
Language=Russian
Invalid cookie received.
.
Language=Polish
Invalid cookie received.
.
Language=Romanian
Invalid cookie received.
.

MessageId=13847
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_PEER_CERT
Language=English
Peer failed to send valid machine certificate.
.
Language=Russian
Peer failed to send valid machine certificate.
.
Language=Polish
Peer failed to send valid machine certificate.
.
Language=Romanian
Peer failed to send valid machine certificate.
.

MessageId=13848
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_PEER_CRL_FAILED
Language=English
Certification Revocation check of peer's certificate failed.
.
Language=Russian
Certification Revocation check of peer's certificate failed.
.
Language=Polish
Certification Revocation check of peer's certificate failed.
.
Language=Romanian
Certification Revocation check of peer's certificate failed.
.

MessageId=13849
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_POLICY_CHANGE
Language=English
New policy invalidated SAs formed with old policy.
.
Language=Russian
New policy invalidated SAs formed with old policy.
.
Language=Polish
New policy invalidated SAs formed with old policy.
.
Language=Romanian
New policy invalidated SAs formed with old policy.
.

MessageId=13850
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NO_MM_POLICY
Language=English
There is no available Main Mode IKE policy.
.
Language=Russian
There is no available Main Mode IKE policy.
.
Language=Polish
There is no available Main Mode IKE policy.
.
Language=Romanian
There is no available Main Mode IKE policy.
.

MessageId=13851
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NOTCBPRIV
Language=English
Failed to enabled TCB privilege.
.
Language=Russian
Failed to enabled TCB privilege.
.
Language=Polish
Failed to enabled TCB privilege.
.
Language=Romanian
Failed to enabled TCB privilege.
.

MessageId=13852
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SECLOADFAIL
Language=English
Failed to load SECURITY.DLL.
.
Language=Russian
Failed to load SECURITY.DLL.
.
Language=Polish
Failed to load SECURITY.DLL.
.
Language=Romanian
Failed to load SECURITY.DLL.
.

MessageId=13853
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_FAILSSPINIT
Language=English
Failed to obtain security function table dispatch address from SSPI.
.
Language=Russian
Failed to obtain security function table dispatch address from SSPI.
.
Language=Polish
Failed to obtain security function table dispatch address from SSPI.
.
Language=Romanian
Failed to obtain security function table dispatch address from SSPI.
.

MessageId=13854
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_FAILQUERYSSP
Language=English
Failed to query Kerberos package to obtain max token size.
.
Language=Russian
Failed to query Kerberos package to obtain max token size.
.
Language=Polish
Failed to query Kerberos package to obtain max token size.
.
Language=Romanian
Failed to query Kerberos package to obtain max token size.
.

MessageId=13855
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SRVACQFAIL
Language=English
Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup.
.
Language=Russian
Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup.
.
Language=Polish
Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup.
.
Language=Romanian
Failed to obtain Kerberos server credentials for ISAKMP/ERROR_IPSEC_IKE service. Kerberos authentication will not function. The most likely reason for this is lack of domain membership. This is normal if your computer is a member of a workgroup.
.

MessageId=13856
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_SRVQUERYCRED
Language=English
Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.
Language=Russian
Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.
Language=Polish
Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.
Language=Romanian
Failed to determine SSPI principal name for ISAKMP/ERROR_IPSEC_IKE service (QueryCredentialsAttributes).
.

MessageId=13857
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_GETSPIFAIL
Language=English
Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters.
.
Language=Russian
Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters.
.
Language=Polish
Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters.
.
Language=Romanian
Failed to obtain new SPI for the inbound SA from IPSec driver. The most common cause for this is that the driver does not have the correct filter. Check your policy to verify the filters.
.

MessageId=13858
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_FILTER
Language=English
Given filter is invalid.
.
Language=Russian
Given filter is invalid.
.
Language=Polish
Given filter is invalid.
.
Language=Romanian
Given filter is invalid.
.

MessageId=13859
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_OUT_OF_MEMORY
Language=English
Memory allocation failed.
.
Language=Russian
Memory allocation failed.
.
Language=Polish
Memory allocation failed.
.
Language=Romanian
Memory allocation failed.
.

MessageId=13860
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ADD_UPDATE_KEY_FAILED
Language=English
Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine.
.
Language=Russian
Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine.
.
Language=Polish
Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine.
.
Language=Romanian
Failed to add Security Association to IPSec Driver. The most common cause for this is if the IKE negotiation took too long to complete. If the problem persists, reduce the load on the faulting machine.
.

MessageId=13861
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_POLICY
Language=English
Invalid policy.
.
Language=Russian
Invalid policy.
.
Language=Polish
Invalid policy.
.
Language=Romanian
Invalid policy.
.

MessageId=13862
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_UNKNOWN_DOI
Language=English
Invalid DOI.
.
Language=Russian
Invalid DOI.
.
Language=Polish
Invalid DOI.
.
Language=Romanian
Invalid DOI.
.

MessageId=13863
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SITUATION
Language=English
Invalid situation.
.
Language=Russian
Invalid situation.
.
Language=Polish
Invalid situation.
.
Language=Romanian
Invalid situation.
.

MessageId=13864
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DH_FAILURE
Language=English
Diffie-Hellman failure.
.
Language=Russian
Diffie-Hellman failure.
.
Language=Polish
Diffie-Hellman failure.
.
Language=Romanian
Diffie-Hellman failure.
.

MessageId=13865
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_GROUP
Language=English
Invalid Diffie-Hellman group.
.
Language=Russian
Invalid Diffie-Hellman group.
.
Language=Polish
Invalid Diffie-Hellman group.
.
Language=Romanian
Invalid Diffie-Hellman group.
.

MessageId=13866
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_ENCRYPT
Language=English
Error encrypting payload.
.
Language=Russian
Error encrypting payload.
.
Language=Polish
Error encrypting payload.
.
Language=Romanian
Error encrypting payload.
.

MessageId=13867
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_DECRYPT
Language=English
Error decrypting payload.
.
Language=Russian
Error decrypting payload.
.
Language=Polish
Error decrypting payload.
.
Language=Romanian
Error decrypting payload.
.

MessageId=13868
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_POLICY_MATCH
Language=English
Policy match error.
.
Language=Russian
Policy match error.
.
Language=Polish
Policy match error.
.
Language=Romanian
Policy match error.
.

MessageId=13869
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_UNSUPPORTED_ID
Language=English
Unsupported ID.
.
Language=Russian
Unsupported ID.
.
Language=Polish
Unsupported ID.
.
Language=Romanian
Unsupported ID.
.

MessageId=13870
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH
Language=English
Hash verification failed.
.
Language=Russian
Hash verification failed.
.
Language=Polish
Hash verification failed.
.
Language=Romanian
Hash verification failed.
.

MessageId=13871
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_ALG
Language=English
Invalid hash algorithm.
.
Language=Russian
Invalid hash algorithm.
.
Language=Polish
Invalid hash algorithm.
.
Language=Romanian
Invalid hash algorithm.
.

MessageId=13872
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_HASH_SIZE
Language=English
Invalid hash size.
.
Language=Russian
Invalid hash size.
.
Language=Polish
Invalid hash size.
.
Language=Romanian
Invalid hash size.
.

MessageId=13873
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_ENCRYPT_ALG
Language=English
Invalid encryption algorithm.
.
Language=Russian
Invalid encryption algorithm.
.
Language=Polish
Invalid encryption algorithm.
.
Language=Romanian
Invalid encryption algorithm.
.

MessageId=13874
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_AUTH_ALG
Language=English
Invalid authentication algorithm.
.
Language=Russian
Invalid authentication algorithm.
.
Language=Polish
Invalid authentication algorithm.
.
Language=Romanian
Invalid authentication algorithm.
.

MessageId=13875
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_SIG
Language=English
Invalid certificate signature.
.
Language=Russian
Invalid certificate signature.
.
Language=Polish
Invalid certificate signature.
.
Language=Romanian
Invalid certificate signature.
.

MessageId=13876
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_LOAD_FAILED
Language=English
Load failed.
.
Language=Russian
Load failed.
.
Language=Polish
Load failed.
.
Language=Romanian
Load failed.
.

MessageId=13877
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_RPC_DELETE
Language=English
Deleted via RPC call.
.
Language=Russian
Deleted via RPC call.
.
Language=Polish
Deleted via RPC call.
.
Language=Romanian
Deleted via RPC call.
.

MessageId=13878
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_BENIGN_REINIT
Language=English
Temporary state created to perform reinit. This is not a real failure.
.
Language=Russian
Temporary state created to perform reinit. This is not a real failure.
.
Language=Polish
Temporary state created to perform reinit. This is not a real failure.
.
Language=Romanian
Temporary state created to perform reinit. This is not a real failure.
.

MessageId=13879
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_RESPONDER_LIFETIME_NOTIFY
Language=English
The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine.
.
Language=Russian
The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine.
.
Language=Polish
The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine.
.
Language=Romanian
The lifetime value received in the Responder Lifetime Notify is below the Windows 2000 configured minimum value. Please fix the policy on the peer machine.
.

MessageId=13881
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_INVALID_CERT_KEYLEN
Language=English
Key length in certificate is too small for configured security requirements.
.
Language=Russian
Key length in certificate is too small for configured security requirements.
.
Language=Polish
Key length in certificate is too small for configured security requirements.
.
Language=Romanian
Key length in certificate is too small for configured security requirements.
.

MessageId=13882
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_MM_LIMIT
Language=English
Max number of established MM SAs to peer exceeded.
.
Language=Russian
Max number of established MM SAs to peer exceeded.
.
Language=Polish
Max number of established MM SAs to peer exceeded.
.
Language=Romanian
Max number of established MM SAs to peer exceeded.
.

MessageId=13883
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEGOTIATION_DISABLED
Language=English
IKE received a policy that disables negotiation.
.
Language=Russian
IKE received a policy that disables negotiation.
.
Language=Polish
IKE received a policy that disables negotiation.
.
Language=Romanian
IKE received a policy that disables negotiation.
.

MessageId=13884
Severity=Success
Facility=System
SymbolicName=ERROR_IPSEC_IKE_NEG_STATUS_END
Language=English
ERROR_IPSEC_IKE_NEG_STATUS_END
.
Language=Russian
ERROR_IPSEC_IKE_NEG_STATUS_END
.
Language=Polish
ERROR_IPSEC_IKE_NEG_STATUS_END
.
Language=Romanian
ERROR_IPSEC_IKE_NEG_STATUS_END
.

MessageId=14000
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_SECTION_NOT_FOUND
Language=English
The requested section was not present in the activation context.
.
Language=Russian
The requested section was not present in the activation context.
.
Language=Polish
The requested section was not present in the activation context.
.
Language=Romanian
The requested section was not present in the activation context.
.

MessageId=14001
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CANT_GEN_ACTCTX
Language=English
This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.
Language=Russian
This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.
Language=Polish
This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.
Language=Romanian
This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem.
.

MessageId=14002
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ACTCTXDATA_FORMAT
Language=English
The application binding data format is invalid.
.
Language=Russian
The application binding data format is invalid.
.
Language=Polish
The application binding data format is invalid.
.
Language=Romanian
The application binding data format is invalid.
.

MessageId=14003
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ASSEMBLY_NOT_FOUND
Language=English
The referenced assembly is not installed on your system.
.
Language=Russian
The referenced assembly is not installed on your system.
.
Language=Polish
The referenced assembly is not installed on your system.
.
Language=Romanian
The referenced assembly is not installed on your system.
.

MessageId=14004
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_FORMAT_ERROR
Language=English
The manifest file does not begin with the required tag and format information.
.
Language=Russian
The manifest file does not begin with the required tag and format information.
.
Language=Polish
The manifest file does not begin with the required tag and format information.
.
Language=Romanian
The manifest file does not begin with the required tag and format information.
.

MessageId=14005
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_PARSE_ERROR
Language=English
The manifest file contains one or more syntax errors.
.
Language=Russian
The manifest file contains one or more syntax errors.
.
Language=Polish
The manifest file contains one or more syntax errors.
.
Language=Romanian
The manifest file contains one or more syntax errors.
.

MessageId=14006
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ACTIVATION_CONTEXT_DISABLED
Language=English
The application attempted to activate a disabled activation context.
.
Language=Russian
The application attempted to activate a disabled activation context.
.
Language=Polish
The application attempted to activate a disabled activation context.
.
Language=Romanian
The application attempted to activate a disabled activation context.
.

MessageId=14007
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_KEY_NOT_FOUND
Language=English
The requested lookup key was not found in any active activation context.
.
Language=Russian
The requested lookup key was not found in any active activation context.
.
Language=Polish
The requested lookup key was not found in any active activation context.
.
Language=Romanian
The requested lookup key was not found in any active activation context.
.

MessageId=14008
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_VERSION_CONFLICT
Language=English
A component version required by the application conflicts with another component version already active.
.
Language=Russian
A component version required by the application conflicts with another component version already active.
.
Language=Polish
A component version required by the application conflicts with another component version already active.
.
Language=Romanian
A component version required by the application conflicts with another component version already active.
.

MessageId=14009
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_WRONG_SECTION_TYPE
Language=English
The type requested activation context section does not match the query API used.
.
Language=Russian
The type requested activation context section does not match the query API used.
.
Language=Polish
The type requested activation context section does not match the query API used.
.
Language=Romanian
The type requested activation context section does not match the query API used.
.

MessageId=14010
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_THREAD_QUERIES_DISABLED
Language=English
Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.
Language=Russian
Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.
Language=Polish
Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.
Language=Romanian
Lack of system resources has required isolated activation to be disabled for the current thread of execution.
.

MessageId=14011
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET
Language=English
An attempt to set the process default activation context failed because the process default activation context was already set.
.
Language=Russian
An attempt to set the process default activation context failed because the process default activation context was already set.
.
Language=Polish
An attempt to set the process default activation context failed because the process default activation context was already set.
.
Language=Romanian
An attempt to set the process default activation context failed because the process default activation context was already set.
.

MessageId=14012
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNKNOWN_ENCODING_GROUP
Language=English
The encoding group identifier specified is not recognized.
.
Language=Russian
The encoding group identifier specified is not recognized.
.
Language=Polish
The encoding group identifier specified is not recognized.
.
Language=Romanian
The encoding group identifier specified is not recognized.
.

MessageId=14013
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNKNOWN_ENCODING
Language=English
The encoding requested is not recognized.
.
Language=Russian
The encoding requested is not recognized.
.
Language=Polish
The encoding requested is not recognized.
.
Language=Romanian
The encoding requested is not recognized.
.

MessageId=14014
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_XML_NAMESPACE_URI
Language=English
The manifest contains a reference to an invalid URI.
.
Language=Russian
The manifest contains a reference to an invalid URI.
.
Language=Polish
The manifest contains a reference to an invalid URI.
.
Language=Romanian
The manifest contains a reference to an invalid URI.
.

MessageId=14015
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ROOT_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=English
The application manifest contains a reference to a dependent assembly which is not installed.
.
Language=Russian
The application manifest contains a reference to a dependent assembly which is not installed.
.
Language=Polish
The application manifest contains a reference to a dependent assembly which is not installed.
.
Language=Romanian
The application manifest contains a reference to a dependent assembly which is not installed.
.

MessageId=14016
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_LEAF_MANIFEST_DEPENDENCY_NOT_INSTALLED
Language=English
The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed.
.
Language=Russian
The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed.
.
Language=Polish
The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed.
.
Language=Romanian
The manifest for an assembly used by the application has a reference to a dependent assembly which is not installed.
.

MessageId=14017
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=English
The manifest contains an attribute for the assembly identity which is not valid.
.
Language=Russian
The manifest contains an attribute for the assembly identity which is not valid.
.
Language=Polish
The manifest contains an attribute for the assembly identity which is not valid.
.
Language=Romanian
The manifest contains an attribute for the assembly identity which is not valid.
.

MessageId=14018
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_MISSING_REQUIRED_DEFAULT_NAMESPACE
Language=English
The manifest is missing the required default namespace specification on the assembly element.
.
Language=Russian
The manifest is missing the required default namespace specification on the assembly element.
.
Language=Polish
The manifest is missing the required default namespace specification on the assembly element.
.
Language=Romanian
The manifest is missing the required default namespace specification on the assembly element.
.

MessageId=14019
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MANIFEST_INVALID_REQUIRED_DEFAULT_NAMESPACE
Language=English
The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\".
.
Language=Russian
The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\".
.
Language=Polish
The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\".
.
Language=Romanian
The manifest has a default namespace specified on the assembly element but its value is not \"urn:schemas-microsoft-com:asm.v1\".
.

MessageId=14020
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PRIVATE_MANIFEST_CROSS_PATH_WITH_REPARSE_POINT
Language=English
The private manifest probe has crossed the reparse-point-associated path.
.
Language=Russian
The private manifest probe has crossed the reparse-point-associated path.
.
Language=Polish
The private manifest probe has crossed the reparse-point-associated path.
.
Language=Romanian
The private manifest probe has crossed the reparse-point-associated path.
.

MessageId=14021
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_DLL_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have files by the same name.
.

MessageId=14022
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_WINDOWCLASS_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have window classes with the same name.
.

MessageId=14023
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_CLSID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have the same COM server CLSIDs.
.

MessageId=14024
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_IID
Language=English
Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have proxies for the same COM interface IIDs.
.

MessageId=14025
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_TLBID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have the same COM type library TLBIDs.
.

MessageId=14026
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_PROGID
Language=English
Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest have the same COM ProgIDs.
.

MessageId=14027
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_DUPLICATE_ASSEMBLY_NAME
Language=English
Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.
Language=Russian
Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.
Language=Polish
Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.
Language=Romanian
Two or more components referenced directly or indirectly by the application manifest are different versions of the same component which is not permitted.
.

MessageId=14028
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_FILE_HASH_MISMATCH
Language=English
A component's file does not match the verification information present in the component manifest.
.
Language=Russian
A component's file does not match the verification information present in the component manifest.
.
Language=Polish
A component's file does not match the verification information present in the component manifest.
.
Language=Romanian
A component's file does not match the verification information present in the component manifest.
.

MessageId=14029
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_POLICY_PARSE_ERROR
Language=English
The policy manifest contains one or more syntax errors.
.
Language=Russian
The policy manifest contains one or more syntax errors.
.
Language=Polish
The policy manifest contains one or more syntax errors.
.
Language=Romanian
The policy manifest contains one or more syntax errors.
.

MessageId=14030
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGQUOTE
Language=English
Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.
Language=Russian
Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.
Language=Polish
Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.
Language=Romanian
Manifest Parse Error : A string literal was expected, but no opening quote character was found.
.

MessageId=14031
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_COMMENTSYNTAX
Language=English
Manifest Parse Error : Incorrect syntax was used in a comment.
.
Language=Russian
Manifest Parse Error : Incorrect syntax was used in a comment.
.
Language=Polish
Manifest Parse Error : Incorrect syntax was used in a comment.
.
Language=Romanian
Manifest Parse Error : Incorrect syntax was used in a comment.
.

MessageId=14032
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADSTARTNAMECHAR
Language=English
Manifest Parse Error : A name was started with an invalid character.
.
Language=Russian
Manifest Parse Error : A name was started with an invalid character.
.
Language=Polish
Manifest Parse Error : A name was started with an invalid character.
.
Language=Romanian
Manifest Parse Error : A name was started with an invalid character.
.

MessageId=14033
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADNAMECHAR
Language=English
Manifest Parse Error : A name contained an invalid character.
.
Language=Russian
Manifest Parse Error : A name contained an invalid character.
.
Language=Polish
Manifest Parse Error : A name contained an invalid character.
.
Language=Romanian
Manifest Parse Error : A name contained an invalid character.
.

MessageId=14034
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADCHARINSTRING
Language=English
Manifest Parse Error : A string literal contained an invalid character.
.
Language=Russian
Manifest Parse Error : A string literal contained an invalid character.
.
Language=Polish
Manifest Parse Error : A string literal contained an invalid character.
.
Language=Romanian
Manifest Parse Error : A string literal contained an invalid character.
.

MessageId=14035
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_XMLDECLSYNTAX
Language=English
Manifest Parse Error : Invalid syntax for an XML declaration.
.
Language=Russian
Manifest Parse Error : Invalid syntax for an XML declaration.
.
Language=Polish
Manifest Parse Error : Invalid syntax for an XML declaration.
.
Language=Romanian
Manifest Parse Error : Invalid syntax for an XML declaration.
.

MessageId=14036
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADCHARDATA
Language=English
Manifest Parse Error : An invalid character was found in text content.
.
Language=Russian
Manifest Parse Error : An invalid character was found in text content.
.
Language=Polish
Manifest Parse Error : An invalid character was found in text content.
.
Language=Romanian
Manifest Parse Error : An invalid character was found in text content.
.

MessageId=14037
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGWHITESPACE
Language=English
Manifest Parse Error : Required white space was missing.
.
Language=Russian
Manifest Parse Error : Required white space was missing.
.
Language=Polish
Manifest Parse Error : Required white space was missing.
.
Language=Romanian
Manifest Parse Error : Required white space was missing.
.

MessageId=14038
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_EXPECTINGTAGEND
Language=English
Manifest Parse Error : The character '>' was expected.
.
Language=Russian
Manifest Parse Error : The character '>' was expected.
.
Language=Polish
Manifest Parse Error : The character '>' was expected.
.
Language=Romanian
Manifest Parse Error : The character '>' was expected.
.

MessageId=14039
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGSEMICOLON
Language=English
Manifest Parse Error : A semi colon character was expected.
.
Language=Russian
Manifest Parse Error : A semi colon character was expected.
.
Language=Polish
Manifest Parse Error : A semi colon character was expected.
.
Language=Romanian
Manifest Parse Error : A semi colon character was expected.
.

MessageId=14040
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNBALANCEDPAREN
Language=English
Manifest Parse Error : Unbalanced parentheses.
.
Language=Russian
Manifest Parse Error : Unbalanced parentheses.
.
Language=Polish
Manifest Parse Error : Unbalanced parentheses.
.
Language=Romanian
Manifest Parse Error : Unbalanced parentheses.
.

MessageId=14041
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INTERNALERROR
Language=English
Manifest Parse Error : Internal error.
.
Language=Russian
Manifest Parse Error : Internal error.
.
Language=Polish
Manifest Parse Error : Internal error.
.
Language=Romanian
Manifest Parse Error : Internal error.
.

MessageId=14042
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_WHITESPACE
Language=English
Manifest Parse Error : White space is not allowed at this location.
.
Language=Russian
Manifest Parse Error : White space is not allowed at this location.
.
Language=Polish
Manifest Parse Error : White space is not allowed at this location.
.
Language=Romanian
Manifest Parse Error : White space is not allowed at this location.
.

MessageId=14043
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INCOMPLETE_ENCODING
Language=English
Manifest Parse Error : End of file reached in invalid state for current encoding.
.
Language=Russian
Manifest Parse Error : End of file reached in invalid state for current encoding.
.
Language=Polish
Manifest Parse Error : End of file reached in invalid state for current encoding.
.
Language=Romanian
Manifest Parse Error : End of file reached in invalid state for current encoding.
.

MessageId=14044
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSING_PAREN
Language=English
Manifest Parse Error : Missing parenthesis.
.
Language=Russian
Manifest Parse Error : Missing parenthesis.
.
Language=Polish
Manifest Parse Error : Missing parenthesis.
.
Language=Romanian
Manifest Parse Error : Missing parenthesis.
.

MessageId=14045
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_EXPECTINGCLOSEQUOTE
Language=English
Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.
Language=Russian
Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.
Language=Polish
Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.
Language=Romanian
Manifest Parse Error : A single or double closing quote character (\' or \") is missing.
.

MessageId=14046
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MULTIPLE_COLONS
Language=English
Manifest Parse Error : Multiple colons are not allowed in a name.
.
Language=Russian
Manifest Parse Error : Multiple colons are not allowed in a name.
.
Language=Polish
Manifest Parse Error : Multiple colons are not allowed in a name.
.
Language=Romanian
Manifest Parse Error : Multiple colons are not allowed in a name.
.

MessageId=14047
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_DECIMAL
Language=English
Manifest Parse Error : Invalid character for decimal digit.
.
Language=Russian
Manifest Parse Error : Invalid character for decimal digit.
.
Language=Polish
Manifest Parse Error : Invalid character for decimal digit.
.
Language=Romanian
Manifest Parse Error : Invalid character for decimal digit.
.

MessageId=14048
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_HEXIDECIMAL
Language=English
Manifest Parse Error : Invalid character for hexadecimal digit.
.
Language=Russian
Manifest Parse Error : Invalid character for hexadecimal digit.
.
Language=Polish
Manifest Parse Error : Invalid character for hexadecimal digit.
.
Language=Romanian
Manifest Parse Error : Invalid character for hexadecimal digit.
.

MessageId=14049
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_UNICODE
Language=English
Manifest Parse Error : Invalid Unicode character value for this platform.
.
Language=Russian
Manifest Parse Error : Invalid Unicode character value for this platform.
.
Language=Polish
Manifest Parse Error : Invalid Unicode character value for this platform.
.
Language=Romanian
Manifest Parse Error : Invalid Unicode character value for this platform.
.

MessageId=14050
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_WHITESPACEORQUESTIONMARK
Language=English
Manifest Parse Error : Expecting white space or '?'.
.
Language=Russian
Manifest Parse Error : Expecting white space or '?'.
.
Language=Polish
Manifest Parse Error : Expecting white space or '?'.
.
Language=Romanian
Manifest Parse Error : Expecting white space or '?'.
.

MessageId=14051
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDENDTAG
Language=English
Manifest Parse Error : End tag was not expected at this location.
.
Language=Russian
Manifest Parse Error : End tag was not expected at this location.
.
Language=Polish
Manifest Parse Error : End tag was not expected at this location.
.
Language=Romanian
Manifest Parse Error : End tag was not expected at this location.
.

MessageId=14052
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDTAG
Language=English
Manifest Parse Error : The following tags were not closed: %1.
.
Language=Russian
Manifest Parse Error : The following tags were not closed: %1.
.
Language=Polish
Manifest Parse Error : The following tags were not closed: %1.
.
Language=Romanian
Manifest Parse Error : The following tags were not closed: %1.
.

MessageId=14053
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_DUPLICATEATTRIBUTE
Language=English
Manifest Parse Error : Duplicate attribute.
.
Language=Russian
Manifest Parse Error : Duplicate attribute.
.
Language=Polish
Manifest Parse Error : Duplicate attribute.
.
Language=Romanian
Manifest Parse Error : Duplicate attribute.
.

MessageId=14054
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MULTIPLEROOTS
Language=English
Manifest Parse Error : Only one top level element is allowed in an XML document.
.
Language=Russian
Manifest Parse Error : Only one top level element is allowed in an XML document.
.
Language=Polish
Manifest Parse Error : Only one top level element is allowed in an XML document.
.
Language=Romanian
Manifest Parse Error : Only one top level element is allowed in an XML document.
.

MessageId=14055
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDATROOTLEVEL
Language=English
Manifest Parse Error : Invalid at the top level of the document.
.
Language=Russian
Manifest Parse Error : Invalid at the top level of the document.
.
Language=Polish
Manifest Parse Error : Invalid at the top level of the document.
.
Language=Romanian
Manifest Parse Error : Invalid at the top level of the document.
.

MessageId=14056
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADXMLDECL
Language=English
Manifest Parse Error : Invalid XML declaration.
.
Language=Russian
Manifest Parse Error : Invalid XML declaration.
.
Language=Polish
Manifest Parse Error : Invalid XML declaration.
.
Language=Romanian
Manifest Parse Error : Invalid XML declaration.
.

MessageId=14057
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGROOT
Language=English
Manifest Parse Error : XML document must have a top level element.
.
Language=Russian
Manifest Parse Error : XML document must have a top level element.
.
Language=Polish
Manifest Parse Error : XML document must have a top level element.
.
Language=Romanian
Manifest Parse Error : XML document must have a top level element.
.

MessageId=14058
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTEDEOF
Language=English
Manifest Parse Error : Unexpected end of file.
.
Language=Russian
Manifest Parse Error : Unexpected end of file.
.
Language=Polish
Manifest Parse Error : Unexpected end of file.
.
Language=Romanian
Manifest Parse Error : Unexpected end of file.
.

MessageId=14059
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADPEREFINSUBSET
Language=English
Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.
Language=Russian
Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.
Language=Polish
Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.
Language=Romanian
Manifest Parse Error : Parameter entities cannot be used inside markup declarations in an internal subset.
.

MessageId=14060
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTARTTAG
Language=English
Manifest Parse Error : Element was not closed.
.
Language=Russian
Manifest Parse Error : Element was not closed.
.
Language=Polish
Manifest Parse Error : Element was not closed.
.
Language=Romanian
Manifest Parse Error : Element was not closed.
.

MessageId=14061
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDENDTAG
Language=English
Manifest Parse Error : End element was missing the character '>'.
.
Language=Russian
Manifest Parse Error : End element was missing the character '>'.
.
Language=Polish
Manifest Parse Error : End element was missing the character '>'.
.
Language=Romanian
Manifest Parse Error : End element was missing the character '>'.
.

MessageId=14062
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDSTRING
Language=English
Manifest Parse Error : A string literal was not closed.
.
Language=Russian
Manifest Parse Error : A string literal was not closed.
.
Language=Polish
Manifest Parse Error : A string literal was not closed.
.
Language=Romanian
Manifest Parse Error : A string literal was not closed.
.

MessageId=14063
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCOMMENT
Language=English
Manifest Parse Error : A comment was not closed.
.
Language=Russian
Manifest Parse Error : A comment was not closed.
.
Language=Polish
Manifest Parse Error : A comment was not closed.
.
Language=Romanian
Manifest Parse Error : A comment was not closed.
.

MessageId=14064
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDDECL
Language=English
Manifest Parse Error : A declaration was not closed.
.
Language=Russian
Manifest Parse Error : A declaration was not closed.
.
Language=Polish
Manifest Parse Error : A declaration was not closed.
.
Language=Romanian
Manifest Parse Error : A declaration was not closed.
.

MessageId=14065
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNCLOSEDCDATA
Language=English
Manifest Parse Error : A CDATA section was not closed.
.
Language=Russian
Manifest Parse Error : A CDATA section was not closed.
.
Language=Polish
Manifest Parse Error : A CDATA section was not closed.
.
Language=Romanian
Manifest Parse Error : A CDATA section was not closed.
.

MessageId=14066
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_RESERVEDNAMESPACE
Language=English
Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\".
.
Language=Russian
Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\".
.
Language=Polish
Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\".
.
Language=Romanian
Manifest Parse Error : The namespace prefix is not allowed to start with the reserved string \"xml\".
.

MessageId=14067
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDENCODING
Language=English
Manifest Parse Error : System does not support the specified encoding.
.
Language=Russian
Manifest Parse Error : System does not support the specified encoding.
.
Language=Polish
Manifest Parse Error : System does not support the specified encoding.
.
Language=Romanian
Manifest Parse Error : System does not support the specified encoding.
.

MessageId=14068
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALIDSWITCH
Language=English
Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.
Language=Russian
Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.
Language=Polish
Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.
Language=Romanian
Manifest Parse Error : Switch from current encoding to specified encoding not supported.
.

MessageId=14069
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_BADXMLCASE
Language=English
Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.
Language=Russian
Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.
Language=Polish
Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.
Language=Romanian
Manifest Parse Error : The name 'xml' is reserved and must be lower case.
.

MessageId=14070
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_STANDALONE
Language=English
Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.
Language=Russian
Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.
Language=Polish
Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.
Language=Romanian
Manifest Parse Error : The standalone attribute must have the value 'yes' or 'no'.
.

MessageId=14071
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_UNEXPECTED_STANDALONE
Language=English
Manifest Parse Error : The standalone attribute cannot be used in external entities.
.
Language=Russian
Manifest Parse Error : The standalone attribute cannot be used in external entities.
.
Language=Polish
Manifest Parse Error : The standalone attribute cannot be used in external entities.
.
Language=Romanian
Manifest Parse Error : The standalone attribute cannot be used in external entities.
.

MessageId=14072
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_INVALID_VERSION
Language=English
Manifest Parse Error : Invalid version number.
.
Language=Russian
Manifest Parse Error : Invalid version number.
.
Language=Polish
Manifest Parse Error : Invalid version number.
.
Language=Romanian
Manifest Parse Error : Invalid version number.
.

MessageId=14073
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_XML_E_MISSINGEQUALS
Language=English
Manifest Parse Error : Missing equals sign between attribute and attribute value.
.
Language=Russian
Manifest Parse Error : Missing equals sign between attribute and attribute value.
.
Language=Polish
Manifest Parse Error : Missing equals sign between attribute and attribute value.
.
Language=Romanian
Manifest Parse Error : Missing equals sign between attribute and attribute value.
.

MessageId=14074
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_RECOVERY_FAILED
Language=English
Assembly Protection Error: Unable to recover the specified assembly.
.
Language=Russian
Assembly Protection Error: Unable to recover the specified assembly.
.
Language=Polish
Assembly Protection Error: Unable to recover the specified assembly.
.
Language=Romanian
Assembly Protection Error: Unable to recover the specified assembly.
.

MessageId=14075
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_PUBLIC_KEY_TOO_SHORT
Language=English
Assembly Protection Error: The public key for an assembly was too short to be allowed.
.
Language=Russian
Assembly Protection Error: The public key for an assembly was too short to be allowed.
.
Language=Polish
Assembly Protection Error: The public key for an assembly was too short to be allowed.
.
Language=Romanian
Assembly Protection Error: The public key for an assembly was too short to be allowed.
.

MessageId=14076
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_CATALOG_NOT_VALID
Language=English
Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest.
.
Language=Russian
Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest.
.
Language=Polish
Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest.
.
Language=Romanian
Assembly Protection Error: The catalog for an assembly is not valid, or does not match the assembly's manifest.
.

MessageId=14077
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_UNTRANSLATABLE_HRESULT
Language=English
An HRESULT could not be translated to a corresponding Win32 error code.
.
Language=Russian
An HRESULT could not be translated to a corresponding Win32 error code.
.
Language=Polish
An HRESULT could not be translated to a corresponding Win32 error code.
.
Language=Romanian
An HRESULT could not be translated to a corresponding Win32 error code.
.

MessageId=14078
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROTECTION_CATALOG_FILE_MISSING
Language=English
Assembly Protection Error: The catalog for an assembly is missing.
.
Language=Russian
Assembly Protection Error: The catalog for an assembly is missing.
.
Language=Polish
Assembly Protection Error: The catalog for an assembly is missing.
.
Language=Romanian
Assembly Protection Error: The catalog for an assembly is missing.
.

MessageId=14079
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE
Language=English
The supplied assembly identity is missing one or more attributes which must be present in this context.
.
Language=Russian
The supplied assembly identity is missing one or more attributes which must be present in this context.
.
Language=Polish
The supplied assembly identity is missing one or more attributes which must be present in this context.
.
Language=Romanian
The supplied assembly identity is missing one or more attributes which must be present in this context.
.

MessageId=14080
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME
Language=English
The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.
Language=Russian
The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.
Language=Polish
The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.
Language=Romanian
The supplied assembly identity has one or more attribute names that contain characters not permitted in XML names.
.

MessageId=14081
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_ASSEMBLY_MISSING
Language=English
The referenced assembly could not be found.
.
Language=Russian
The referenced assembly could not be found.
.
Language=Polish
The referenced assembly could not be found.
.
Language=Romanian
The referenced assembly could not be found.
.

MessageId=14082
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CORRUPT_ACTIVATION_STACK
Language=English
The activation context activation stack for the running thread of execution is corrupt.
.
Language=Russian
The activation context activation stack for the running thread of execution is corrupt.
.
Language=Polish
The activation context activation stack for the running thread of execution is corrupt.
.
Language=Romanian
The activation context activation stack for the running thread of execution is corrupt.
.

MessageId=14083
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_CORRUPTION
Language=English
The application isolation metadata for this process or thread has become corrupt.
.
Language=Russian
The application isolation metadata for this process or thread has become corrupt.
.
Language=Polish
The application isolation metadata for this process or thread has become corrupt.
.
Language=Romanian
The application isolation metadata for this process or thread has become corrupt.
.

MessageId=14084
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_EARLY_DEACTIVATION
Language=English
The activation context being deactivated is not the most recently activated one.
.
Language=Russian
The activation context being deactivated is not the most recently activated one.
.
Language=Polish
The activation context being deactivated is not the most recently activated one.
.
Language=Romanian
The activation context being deactivated is not the most recently activated one.
.

MessageId=14085
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_DEACTIVATION
Language=English
The activation context being deactivated is not active for the current thread of execution.
.
Language=Russian
The activation context being deactivated is not active for the current thread of execution.
.
Language=Polish
The activation context being deactivated is not active for the current thread of execution.
.
Language=Romanian
The activation context being deactivated is not active for the current thread of execution.
.

MessageId=14086
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_MULTIPLE_DEACTIVATION
Language=English
The activation context being deactivated has already been deactivated.
.
Language=Russian
The activation context being deactivated has already been deactivated.
.
Language=Polish
The activation context being deactivated has already been deactivated.
.
Language=Romanian
The activation context being deactivated has already been deactivated.
.

MessageId=14087
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_PROCESS_TERMINATION_REQUESTED
Language=English
A component used by the isolation facility has requested to terminate the process.
.
Language=Russian
A component used by the isolation facility has requested to terminate the process.
.
Language=Polish
A component used by the isolation facility has requested to terminate the process.
.
Language=Romanian
A component used by the isolation facility has requested to terminate the process.
.

MessageId=14088
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_RELEASE_ACTIVATION_CONTEXT
Language=English
A kernel mode component is releasing a reference on an activation context.
.
Language=Russian
A kernel mode component is releasing a reference on an activation context.
.
Language=Polish
A kernel mode component is releasing a reference on an activation context.
.
Language=Romanian
A kernel mode component is releasing a reference on an activation context.
.

MessageId=14089
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY
Language=English
The activation context of system default assembly could not be generated.
.
Language=Russian
The activation context of system default assembly could not be generated.
.
Language=Polish
The activation context of system default assembly could not be generated.
.
Language=Romanian
The activation context of system default assembly could not be generated.
.

MessageId=14090
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE
Language=English
The value of an attribute in an identity is not within the legal range.
.
Language=Russian
The value of an attribute in an identity is not within the legal range.
.
Language=Polish
The value of an attribute in an identity is not within the legal range.
.
Language=Romanian
The value of an attribute in an identity is not within the legal range.
.

MessageId=14091
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME
Language=English
The name of an attribute in an identity is not within the legal range.
.
Language=Russian
The name of an attribute in an identity is not within the legal range.
.
Language=Polish
The name of an attribute in an identity is not within the legal range.
.
Language=Romanian
The name of an attribute in an identity is not within the legal range.
.

MessageId=14092
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE
Language=English
An identity contains two definitions for the same attribute.
.
Language=Russian
An identity contains two definitions for the same attribute.
.
Language=Polish
An identity contains two definitions for the same attribute.
.
Language=Romanian
An identity contains two definitions for the same attribute.
.

MessageId=14093
Severity=Success
Facility=System
SymbolicName=ERROR_SXS_IDENTITY_PARSE_ERROR
Language=English
The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.
.
Language=Russian
The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.
.
Language=Polish
The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.
.
Language=Romanian
The identity string is malformed. This may be due to a trailing comma, more than two unnamed attributes, missing attribute name or missing attribute value.
.

MessageId=15000
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_INVALID_CHANNEL_PATH
Language=English
The specified channel path is invalid. See extended error info for more details.
.
Language=Russian
The specified channel path is invalid. See extended error info for more details.
.
Language=Polish
The specified channel path is invalid. See extended error info for more details.
.
Language=Romanian
The specified channel path is invalid. See extended error info for more details.
.

MessageId=15001
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_INVALID_QUERY
Language=English
The specified query is invalid. See extended error info for more details.
.
Language=Russian
The specified query is invalid. See extended error info for more details.
.
Language=Polish
The specified query is invalid. See extended error info for more details.
.
Language=Romanian
The specified query is invalid. See extended error info for more details.
.

MessageId=15002
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_PUBLISHER_MANIFEST_NOT_FOUND
Language=English
The publisher did indicate they have a manifest/resource but a manifest/resource could not be found.
.
Language=Russian
The publisher did indicate they have a manifest/resource but a manifest/resource could not be found.
.
Language=Polish
The publisher did indicate they have a manifest/resource but a manifest/resource could not be found.
.
Language=Romanian
The publisher did indicate they have a manifest/resource but a manifest/resource could not be found.
.

MessageId=15003
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_PUBLISHER_MANIFEST_NOT_SPECIFIED
Language=English
The publisher does not have a manifest and is performing an operation which requires they have a manifest.
.
Language=Russian
The publisher does not have a manifest and is performing an operation which requires they have a manifest.
.
Language=Polish
The publisher does not have a manifest and is performing an operation which requires they have a manifest.
.
Language=Romanian
The publisher does not have a manifest and is performing an operation which requires they have a manifest.
.

MessageId=15004
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_NO_REGISTERED_TEMPLATE
Language=English
There is no registered template for specified event id.
.
Language=Russian
There is no registered template for specified event id.
.
Language=Polish
There is no registered template for specified event id.
.
Language=Romanian
There is no registered template for specified event id.
.

MessageId=15005
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_EVENT_CHANNEL_MISMATCH
Language=English
The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to.
.
Language=Russian
The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to.
.
Language=Polish
The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to.
.
Language=Romanian
The specified event was declared in the manifest to go a different channel than the one this publisher handle is bound to.
.

MessageId=15006
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_UNEXPECTED_VALUE_TYPE
Language=English
The type of a specified substitution value does not match the type expected from the template definition.
.
Language=Russian
The type of a specified substitution value does not match the type expected from the template definition.
.
Language=Polish
The type of a specified substitution value does not match the type expected from the template definition.
.
Language=Romanian
The type of a specified substitution value does not match the type expected from the template definition.
.

MessageId=15007
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_UNEXPECTED_NUM_VALUES
Language=English
The number of specified substitution values does not match the number expected from the template definition.
.
Language=Russian
The number of specified substitution values does not match the number expected from the template definition.
.
Language=Polish
The number of specified substitution values does not match the number expected from the template definition.
.
Language=Romanian
The number of specified substitution values does not match the number expected from the template definition.
.

MessageId=15008
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_CHANNEL_NOT_FOUND
Language=English
The specified channel could not be found. Check channel configuration.
.
Language=Russian
The specified channel could not be found. Check channel configuration.
.
Language=Polish
The specified channel could not be found. Check channel configuration.
.
Language=Romanian
The specified channel could not be found. Check channel configuration.
.

MessageId=15009
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_MALFORMED_XML_TEXT
Language=English
The specified xml text was not well-formed. See Extended Error for more details.
.
Language=Russian
The specified xml text was not well-formed. See Extended Error for more details.
.
Language=Polish
The specified xml text was not well-formed. See Extended Error for more details.
.
Language=Romanian
The specified xml text was not well-formed. See Extended Error for more details.
.

MessageId=15010
Severity=Success
Facility=System
SymbolicName=ERROR_EVT_CHANNEL_PATH_TOO_GENERAL
Language=English
The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance.
.
Language=Russian
The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance.
.
Language=Polish
The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance.
.
Language=Romanian
The specified channel path selects more than one instance of a channel. The operation requires that only one channel be selected. It may be necessary to scope channel path to version / publicKeyToken to select only one instance.
.

MessageId=0x000E
Severity=Warning
Facility=WIN32
SymbolicName=E_OUTOFMEMORY
Language=English
Out of memory
.
Language=Russian
Out of memory
.
Language=Polish
Out of memory
.
Language=Romanian
Out of memory
.

MessageId=0x0057
Severity=Warning
Facility=WIN32
SymbolicName=E_INVALIDARG
Language=English
One or more arguments are invalid
.
Language=Russian
One or more arguments are invalid
.
Language=Polish
One or more arguments are invalid
.
Language=Romanian
One or more arguments are invalid
.

MessageId=0x0006
Severity=Warning
Facility=WIN32
SymbolicName=E_HANDLE
Language=English
Invalid handle
.
Language=Russian
Invalid handle
.
Language=Polish
Invalid handle
.
Language=Romanian
Invalid handle
.

MessageId=0x0005
Severity=Warning
Facility=WIN32
SymbolicName=E_ACCESSDENIED
Language=English
WIN32 access denied error
.
Language=Russian
WIN32 access denied error
.
Language=Polish
WIN32 access denied error
.
Language=Romanian
WIN32 access denied error
.


MessageId=0x0000
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_OLEVERB
Language=English
Invalid OLEVERB structure
.
Language=Russian
Invalid OLEVERB structure
.
Language=Polish
Invalid OLEVERB structure
.
Language=Romanian
Invalid OLEVERB structure
.

MessageId=0x0001
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ADVF
Language=English
Invalid advise flags
.
Language=Russian
Invalid advise flags
.
Language=Polish
Invalid advise flags
.
Language=Romanian
Invalid advise flags
.

MessageId=0x0002
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ENUM_NOMORE
Language=English
Can't enumerate any more, because the associated data is missing
.
Language=Russian
Can't enumerate any more, because the associated data is missing
.
Language=Polish
Can't enumerate any more, because the associated data is missing
.
Language=Romanian
Can't enumerate any more, because the associated data is missing
.

MessageId=0x0003
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_ADVISENOTSUPPORTED
Language=English
This implementation doesn't take advises
.
Language=Russian
This implementation doesn't take advises
.
Language=Polish
This implementation doesn't take advises
.
Language=Romanian
This implementation doesn't take advises
.

MessageId=0x0004
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOCONNECTION
Language=English
There is no connection for this connection ID
.
Language=Russian
There is no connection for this connection ID
.
Language=Polish
There is no connection for this connection ID
.
Language=Romanian
There is no connection for this connection ID
.

MessageId=0x0005
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOTRUNNING
Language=English
Need to run the object to perform this operation
.
Language=Russian
Need to run the object to perform this operation
.
Language=Polish
Need to run the object to perform this operation
.
Language=Romanian
Need to run the object to perform this operation
.

MessageId=0x0006
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOCACHE
Language=English
There is no cache to operate on
.
Language=Russian
There is no cache to operate on
.
Language=Polish
There is no cache to operate on
.
Language=Romanian
There is no cache to operate on
.

MessageId=0x0007
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_BLANK
Language=English
Uninitialized object
.
Language=Russian
Uninitialized object
.
Language=Polish
Uninitialized object
.
Language=Romanian
Uninitialized object
.

MessageId=0x0008
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CLASSDIFF
Language=English
Linked object's source class has changed
.
Language=Russian
Linked object's source class has changed
.
Language=Polish
Linked object's source class has changed
.
Language=Romanian
Linked object's source class has changed
.

MessageId=0x0009
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANT_GETMONIKER
Language=English
Not able to get the moniker of the object
.
Language=Russian
Not able to get the moniker of the object
.
Language=Polish
Not able to get the moniker of the object
.
Language=Romanian
Not able to get the moniker of the object
.

MessageId=0x000A
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANT_BINDTOSOURCE
Language=English
Not able to bind to the source
.
Language=Russian
Not able to bind to the source
.
Language=Polish
Not able to bind to the source
.
Language=Romanian
Not able to bind to the source
.

MessageId=0x000B
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_STATIC
Language=English
Object is static; operation not allowed
.
Language=Russian
Object is static; operation not allowed
.
Language=Polish
Object is static; operation not allowed
.
Language=Romanian
Object is static; operation not allowed
.

MessageId=0x000C
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_PROMPTSAVECANCELLED
Language=English
User canceled out of save dialog
.
Language=Russian
User canceled out of save dialog
.
Language=Polish
User canceled out of save dialog
.
Language=Romanian
User canceled out of save dialog
.

MessageId=0x000D
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_INVALIDRECT
Language=English
Invalid rectangle
.
Language=Russian
Invalid rectangle
.
Language=Polish
Invalid rectangle
.
Language=Romanian
Invalid rectangle
.

MessageId=0x000E
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_WRONGCOMPOBJ
Language=English
compobj.dll is too old for the ole2.dll initialized
.
Language=Russian
compobj.dll is too old for the ole2.dll initialized
.
Language=Polish
compobj.dll is too old for the ole2.dll initialized
.
Language=Romanian
compobj.dll is too old for the ole2.dll initialized
.

MessageId=0x000F
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_INVALIDHWND
Language=English
Invalid window handle
.
Language=Russian
Invalid window handle
.
Language=Polish
Invalid window handle
.
Language=Romanian
Invalid window handle
.

MessageId=0x0010
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOT_INPLACEACTIVE
Language=English
Object is not in any of the inplace active states
.
Language=Russian
Object is not in any of the inplace active states
.
Language=Polish
Object is not in any of the inplace active states
.
Language=Romanian
Object is not in any of the inplace active states
.

MessageId=0x0011
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_CANTCONVERT
Language=English
Not able to convert object
.
Language=Russian
Not able to convert object
.
Language=Polish
Not able to convert object
.
Language=Romanian
Not able to convert object
.

MessageId=0x0012
Severity=Warning
Facility=ITF
SymbolicName=OLE_E_NOSTORAGE
Language=English
Not able to perform the operation because object is not given storage yet
.
Language=Russian
Not able to perform the operation because object is not given storage yet
.
Language=Polish
Not able to perform the operation because object is not given storage yet
.
Language=Romanian
Not able to perform the operation because object is not given storage yet
.

MessageId=0x0064
Severity=Warning
Facility=ITF
SymbolicName=DV_E_FORMATETC
Language=English
Invalid FORMATETC structure
.
Language=Russian
Invalid FORMATETC structure
.
Language=Polish
Invalid FORMATETC structure
.
Language=Romanian
Invalid FORMATETC structure
.

MessageId=0x0065
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVTARGETDEVICE
Language=English
Invalid DVTARGETDEVICE structure
.
Language=Russian
Invalid DVTARGETDEVICE structure
.
Language=Polish
Invalid DVTARGETDEVICE structure
.
Language=Romanian
Invalid DVTARGETDEVICE structure
.

MessageId=0x0066
Severity=Warning
Facility=ITF
SymbolicName=DV_E_STGMEDIUM
Language=English
Invalid STDGMEDIUM structure
.
Language=Russian
Invalid STDGMEDIUM structure
.
Language=Polish
Invalid STDGMEDIUM structure
.
Language=Romanian
Invalid STDGMEDIUM structure
.

MessageId=0x0067
Severity=Warning
Facility=ITF
SymbolicName=DV_E_STATDATA
Language=English
Invalid STATDATA structure
.
Language=Russian
Invalid STATDATA structure
.
Language=Polish
Invalid STATDATA structure
.
Language=Romanian
Invalid STATDATA structure
.

MessageId=0x0068
Severity=Warning
Facility=ITF
SymbolicName=DV_E_LINDEX
Language=English
Invalid lindex
.
Language=Russian
Invalid lindex
.
Language=Polish
Invalid lindex
.
Language=Romanian
Invalid lindex
.

MessageId=0x0069
Severity=Warning
Facility=ITF
SymbolicName=DV_E_TYMED
Language=English
Invalid tymed
.
Language=Russian
Invalid tymed
.
Language=Polish
Invalid tymed
.
Language=Romanian
Invalid tymed
.

MessageId=0x006A
Severity=Warning
Facility=ITF
SymbolicName=DV_E_CLIPFORMAT
Language=English
Invalid clipboard format
.
Language=Russian
Invalid clipboard format
.
Language=Polish
Invalid clipboard format
.
Language=Romanian
Invalid clipboard format
.

MessageId=0x006B
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVASPECT
Language=English
Invalid aspect(s)
.
Language=Russian
Invalid aspect(s)
.
Language=Polish
Invalid aspect(s)
.
Language=Romanian
Invalid aspect(s)
.

MessageId=0x006C
Severity=Warning
Facility=ITF
SymbolicName=DV_E_DVTARGETDEVICE_SIZE
Language=English
tdSize parameter of the DVTARGETDEVICE structure is invalid
.
Language=Russian
tdSize parameter of the DVTARGETDEVICE structure is invalid
.
Language=Polish
tdSize parameter of the DVTARGETDEVICE structure is invalid
.
Language=Romanian
tdSize parameter of the DVTARGETDEVICE structure is invalid
.

MessageId=0x006D
Severity=Warning
Facility=ITF
SymbolicName=DV_E_NOIVIEWOBJECT
Language=English
Object doesn't support IViewObject interface
.
Language=Russian
Object doesn't support IViewObject interface
.
Language=Polish
Object doesn't support IViewObject interface
.
Language=Romanian
Object doesn't support IViewObject interface
.

MessageId=0x0100
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_NOTREGISTERED
Language=English
Trying to revoke a drop target that has not been registered
.
Language=Russian
Trying to revoke a drop target that has not been registered
.
Language=Polish
Trying to revoke a drop target that has not been registered
.
Language=Romanian
Trying to revoke a drop target that has not been registered
.

MessageId=0x0101
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_ALREADYREGISTERED
Language=English
This window has already been registered as a drop target
.
Language=Russian
This window has already been registered as a drop target
.
Language=Polish
This window has already been registered as a drop target
.
Language=Romanian
This window has already been registered as a drop target
.

MessageId=0x0102
Severity=Warning
Facility=ITF
SymbolicName=DRAGDROP_E_INVALIDHWND
Language=English
Invalid window handle
.
Language=Russian
Invalid window handle
.
Language=Polish
Invalid window handle
.
Language=Romanian
Invalid window handle
.

MessageId=0x0110
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_NOAGGREGATION
Language=English
Class does not support aggregation (or class object is remote)
.
Language=Russian
Class does not support aggregation (or class object is remote)
.
Language=Polish
Class does not support aggregation (or class object is remote)
.
Language=Romanian
Class does not support aggregation (or class object is remote)
.

MessageId=0x0111
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_CLASSNOTAVAILABLE
Language=English
ClassFactory cannot supply requested class
.
Language=Russian
ClassFactory cannot supply requested class
.
Language=Polish
ClassFactory cannot supply requested class
.
Language=Romanian
ClassFactory cannot supply requested class
.

MessageId=0x0112
Severity=Warning
Facility=ITF
SymbolicName=CLASS_E_NOTLICENSED
Language=English
Class is not licensed for use
.
Language=Russian
Class is not licensed for use
.
Language=Polish
Class is not licensed for use
.
Language=Romanian
Class is not licensed for use
.
