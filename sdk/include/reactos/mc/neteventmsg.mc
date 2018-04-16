;
; netevent.mc MESSAGE resources for netevent.dll
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409
               Russian=0x419:MSG00419
               Polish=0x415:MSG00415)


;
; message definitions
;

; Facility=System

;
; eventlog events 6000-6099
;

MessageId=6000
Severity=Warning
Facility=System
SymbolicName=EVENT_LOG_FULL
Language=English
The %1 log file is full.
.
Language=Russian
Файл журнала "%1" заполнен.
.
Language=Polish
Plik dziennika %1 jest zapełniony.
.

MessageId=6001
Severity=Warning
Facility=System
SymbolicName=EVENT_LogFileNotOpened
Language=English
The %1 log file cannot be opened.
.
Language=Russian
Файл журнала "%1" не может быть открыт.
.
Language=Polish
Nie można otworzyć pliku dziennika %1.
.

MessageId=6002
Severity=Warning
Facility=System
SymbolicName=EVENT_LogFileCorrupt
Language=English
The %1 log file is corrupted and will be cleared.
.
Language=Russian
Файл журана "%1" поврежден и будет очищен.
.
Language=Polish
Plik dziennika "%1" jest uszkodzony i zostanie wyczyszczony.
.

MessageId=6003
Severity=Warning
Facility=System
SymbolicName=EVENT_DefaultLogCorrupt
Language=English
The Application log file could not be opened.  %1 will be used as the default log file.
.
Language=Russian
Файл журнала "Приложения" не может быть открыт. "%1" будет использоваться в качестве файла журана по умолчанию.
.
Language=Polish
Nie można otworzyć pliku dziennika aplikacji. Jako domyślny plik dziennika zostanie użyty plik %1.
.

MessageId=6004
Severity=Warning
Facility=System
SymbolicName=EVENT_BadDriverPacket
Language=English
A driver packet received from the I/O subsystem was invalid.  The data is the packet.
.
Language=Russian
Пакет драйвера, полученный от подсистемы ввода-вывода недопустим. Данные являются пакетом.
.
Language=Polish
Pakiet sterownika otrzymany z podsystemu wejścia/wyjścia jest niepoprawny. Przedstawione dane to otrzymany pakiet.
.

MessageId=6005
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogStarted
Language=English
The Event log service was started.
.
Language=Russian
Служба журана событий была запущена.
.
Language=Polish
Uruchomiono usługę Dziennik zdarzeń.
.

MessageId=6006
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogStopped
Language=English
The Event log service was stopped.
.
Language=Russian
Служба журнала событий была остановлена.
.
Language=Polish
Zatrzymano usługę Dziennik zdarzeń.
.

MessageId=6007
Severity=Warning
Facility=System
SymbolicName=TITLE_EventlogMessageBox
Language=English
Eventlog Service %0
.
Language=Russian
Служба журнала событий %0
.
Language=Polish
Usługa Dziennik zdarzeń "%0"
.

MessageId=6008
Severity=Warning
Facility=System
SymbolicName=EVENT_EventlogAbnormalShutdown
Language=English
The previous system shutdown at %1 on %2 was unexpected.
.
Language=Russian
Предыдущее завершение работы системы в %1 %2 было неожиданным.
.
Language=Polish
Poprzednie zamknięcie systemu o godzinie %1 na %2 było nieoczekiwane.
.

MessageId=6009
Severity=Warning
Facility=System
SymbolicName=EVENT_EventLogProductInfo
Language=English
ReactOS %1 %2 %3 %4.
.

MessageId=6010
Severity=Error
Facility=System
SymbolicName=EVENT_ServiceNoEventLog
Language=English
The %1 service was unable to set up an event source.
.
Language=Russian
Служба "%1" не смогла установить источник события.
.
Language=Polish
Usługa %1 nie mogła skonfigurować źródła zdarzenia.
.

MessageId=6011
Severity=Error
Facility=System
SymbolicName=EVENT_ComputerNameChange
Language=English
The NetBIOS name and DNS host name of this machine have been changed from %1 to %2.
.
Language=Russian
NetBIOS и DNS имена этого компьютера были изменены с "%1" на "%2".
.
Language=Polish
Nazwa NetBIOS i nazwa hosta DNS tego komputera została zmieniona z %1 na %2.
.

MessageId=6012
Severity=Error
Facility=System
SymbolicName=EVENT_DNSDomainNameChange
Language=English
The DNS domain assigned to this computer has been changed from %1 to %2.
.
Language=Russian
DNS-домен присвоенный этому компьютеру был изменен с "%1" на "%2".
.
Language=Polish
Domena DNS przydzielona do tego komputera uległa zmianie z %1 na %2.
.


;
; system events 6100 - 6199
;

MessageId=6100
Severity=Error
Facility=System
SymbolicName=EVENT_UP_DRIVER_ON_MP
Language=English
A uniprocessor-specific driver was loaded on a multiprocessor system.  The driver could not load.
.
Language=Russian
Однопроцессорный драйвер был загружен на многопроцессорной системе. Драйвер не может быть загружен.
.
Language=Polish
Próbowano załadować sterownik jednoprocesorowy na komputerze wieloprocesorowym. Nie można załadować sterownika.
.


;
; service controller events 7000-7899
;

MessageId=7000
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED
Language=English
The %1 service failed to start due to the following error: %n%2
.
Language=Russian
Службе "%1" не удалось запуститься из-за следующей ошибки: %n%2
.
Language=Polish
Nie można uruchomić usługi %1 z powodu następującego błędu: %n%2
.

MessageId=7001
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_II
Language=English
The %1 service depends on the %2 service which failed to start because of the following error: %n%3
.
Language=Russian
Служба "%1" зависит от службы "%2", которой не удалось запуститься из-за следующей ошибки: %n%3
.
Language=Polish
Usługa %1 zależy od usługi %2, której nie można uruchomić z powodu następującego błędu: %n%3
.

MessageId=7002
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_GROUP
Language=English
The %1 service depends on the %2 group and no member of this group started.
.
Language=Russian
Служба "%1" зависит от группы "%2" и ни один элемент этой группы не запущен.
.
Language=Polish
Usługa %1 zależy od grupy %2, a nie uruchomiono żadnego członka tej grupy.
.

MessageId=7003
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_FAILED_NONE
Language=English
The %1 service depends on the following nonexistent service: %2
.
Language=Russian
Служба "%1" зависит от следующей несуществующей службы: "%2"
.
Language=Polish
Usługa %1 zależy od następującej nieistniejącej usługi: %2
.

MessageId=7005
Severity=Error
Facility=System
SymbolicName=EVENT_CALL_TO_FUNCTION_FAILED
Language=English
The %1 call failed with the following error: %n%2
.
Language=Russian
Вызов "%1" завершился ошибкой: %n%2
.
Language=Polish
Wywołanie %1 nie powiodło się i wystąpił następujący błąd: %n%2
.

MessageId=7006
Severity=Error
Facility=System
SymbolicName=EVENT_CALL_TO_FUNCTION_FAILED_II
Language=English
The %1 call failed for %2 with the following error: %n%3
.
Language=Polish
Wywołanie %1 dla %2 nie powiodło się i wystąpił następujący błąd: %n%3
.

MessageId=7007
Severity=Error
Facility=System
SymbolicName=EVENT_REVERTED_TO_LASTKNOWNGOOD
Language=English
The system reverted to its last known good configuration.  The system is restarting....
.
Language=Polish
System powrócił do ostatniej znanej dobrej konfiguracji. Trwa ponowne uruchamianie systemu...
.

MessageId=7008
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_ACCOUNT_NAME
Language=English
No backslash is in the account name.
.
Language=Polish
W nazwie konta brak znaku ukośnika odwrotnego.
.

MessageId=7009
Severity=Error
Facility=System
SymbolicName=EVENT_CONNECTION_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for the %2 service to connect.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na połączenie się z usługą %2.
.

MessageId=7010
Severity=Error
Facility=System
SymbolicName=EVENT_READFILE_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for ReadFile.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na operację ReadFile.
.

MessageId=7011
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for a transaction response from the %2 service.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na odpowiedź transakcji z usługi %2.
.

MessageId=7012
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_INVALID
Language=English
Message returned in transaction has incorrect size.
.
Language=Polish
Wiadomość zwrócona w transakcji ma niepoprawny rozmiar.
.

MessageId=7013
Severity=Error
Facility=System
SymbolicName=EVENT_FIRST_LOGON_FAILED
Language=English
Logon attempt with current password failed with the following error: %n%1
.
Language=Polish
Próba logowania za pomocą aktualnego hasła nie powiodła się; wystąpił następujący błąd: %n%1
.

MessageId=7014
Severity=Error
Facility=System
SymbolicName=EVENT_SECOND_LOGON_FAILED
Language=English
Second logon attempt with old password also failed with the following error: %n%1
.
Language=Polish
Druga próba logowania za pomocą starego hasła także się nie powiodła; wystąpił następujący błąd: %n%1
.

MessageId=7015
Severity=Error
Facility=System
SymbolicName=EVENT_INVALID_DRIVER_DEPENDENCY
Language=English
Boot-start or system-start driver (%1) must not depend on a service.
.
Language=Polish
Sterowniki startu rozruchowego oraz systemowego (%1) nie mogą zależeć od usługi.
.

MessageId=7016
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_SERVICE_STATE
Language=English
The %1 service has reported an invalid current state %2.
.
Language=Polish
Usługa %1 zaraportowała nieprawidłowy stan bieżący %2.
.

MessageId=7017
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_DEMAND
Language=English
Detected circular dependencies demand starting %1.
.
Language=Polish
Wykryte zależności cykliczne wymagają uruchomienia %1.
.

MessageId=7018
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_AUTO
Language=English
Detected circular dependencies auto-starting services.
.
Language=Polish
Wykryto automatycznie uruchamiane usługi zależności cyklicznych.
.

MessageId=7019
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_SERVICE
Language=English
Circular dependency: The %1 service depends on a service in a group which starts later.
.
Language=Polish
Zależność cykliczna: usługa %1 zależy od usługi z grupy uruchamianej później.
.

MessageId=7020
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_GROUP
Language=English
Circular dependency: The %1 service depends on a group which starts later.
.
Language=Polish
Zależność cykliczna: usługa %1 zależy od grupy uruchamianej później.
.

MessageId=7021
Severity=Error
Facility=System
SymbolicName=EVENT_SEVERE_SERVICE_FAILED
Language=English
About to revert to the last known good configuration because the %1 service failed to start.
.
Language=Polish
System powróci do ostatniej znanej dobrej konfiguracji, ponieważ uruchomienie usługi %1 nie powiodło się.
.

MessageId=7022
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_HUNG
Language=English
The %1 service hung on starting.
.
Language=Russian
Служба "%1" зависла при запуске.
.
Language=Polish
Usługa %1 zawiesiła się podczas uruchamiania.
.

MessageId=7023
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED
Language=English
The %1 service terminated with the following error: %n%2
.
Language=Polish
Usługa %1 zakończyła działanie; wystąpił następujący błąd: %n%2
.

MessageId=7024
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED_SPECIFIC
Language=English
The %1 service terminated with service-specific error %2.
.
Language=Polish
Usługa %1 zakończyła działanie; wystąpił specyficzny dla niej błąd %2.
.

MessageId=7025
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_AT_BOOT_FAILED
Language=English
At least one service or driver failed during system startup.  Use Event Viewer to examine the event log for details.
.
Language=Polish
Przynajmniej jedna usługa lub jeden sterownik nie dały się uruchomić podczas uruchamiania systemu. Użyj Podglądu zdarzeń, aby znaleźć szczegółowe informacje w dzienniku zdarzeń.
.

MessageId=7026
Severity=Error
Facility=System
SymbolicName=EVENT_BOOT_SYSTEM_DRIVERS_FAILED
Language=English
The following boot-start or system-start driver(s) failed to load: %1
.
Language=Polish
Nie można załadować następujących sterowników startu rozruchowego lub systemowego: %1
.

MessageId=7027
Severity=Error
Facility=System
SymbolicName=EVENT_RUNNING_LASTKNOWNGOOD
Language=English
ReactOS could not be started as configured.  A previous working configuration was used instead.
.
Language=Polish
Nie można uruchomić systemu ReactOS zgodnie z aktualną konfiguracją. Zamiast niej użyto poprzedniej działającej konfiguracji.
.

MessageId=7028
Severity=Error
Facility=System
SymbolicName=EVENT_TAKE_OWNERSHIP
Language=English
The %1 Registry key denied access to SYSTEM account programs so the Service Control Manager took ownership of the Registry key.
.
Language=Polish
Klucz Rejestru %1 nie przyznał dostępu do programów konta SYSTEM, w związku z czym Menedżer sterowania usługami przejął ten klucz na własność.
.

MessageId=7029
Severity=Error
Facility=System
SymbolicName=TITLE_SC_MESSAGE_BOX
Language=English
Service Control Manager %0
.
Language=Polish
Menedżer sterowania usługami %0
.

MessageId=7030
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_NOT_INTERACTIVE
Language=English
The %1 service is marked as an interactive service.  However, the system is configured to not allow interactive services.  This service may not function properly.
.
Language=Polish
Usługa %1 jest oznaczona jako usługa interakcyjna. System jest jednak skonfigurowany tak, aby nie zezwalać na usługi interakcyjne, dlatego ta usługa może nie działać właściwie.
.

MessageId=7031
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CRASH
Language=English
The %1 service terminated unexpectedly.  It has done this %2 time(s).  The following corrective action will be taken in %3 milliseconds: %5.
.
Language=Polish
Usługa %1 niespodziewanie zakończyła pracę. Wystąpiło to razy: %2. W przeciągu %3 milisekund zostanie podjęta następująca czynność korekcyjna: %5.
.

MessageId=7032
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_RECOVERY_FAILED
Language=English
The Service Control Manager tried to take a corrective action (%2) after the unexpected termination of the %3 service, but this action failed with the following error: %n%4
.
Language=Polish
Menedżer sterowania usługami próbował podjąć akcję korekcyjną (%2) po nieoczekiwanym zakończeniu usługi %3, ale ta akcja nie powiodła się przy następującym błędzie: %n%4.
.

MessageId=7033
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_SCESRV_FAILED
Language=English
The Service Control Manager did not initialize successfully. The security
configuration server (scesrv.dll) failed to initialize with error %1.  The
system is restarting...
.
Language=Polish
Menedżer sterowania usługami nie został zainicjowany pomyślnie.\r\nNie udało się zainicjować serwera
konfiguracji zabezpieczeń (scesrv.dll)\r\nz powodu błędu %1. Trwa ponowne uruchamianie systemu...
.

MessageId=7034
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CRASH_NO_ACTION
Language=English
The %1 service terminated unexpectedly.  It has done this %2 time(s).
.
Language=Russian
Служба "%1" неожиданно завершилась. Это произошло %2 раз(а).
.
Language=Polish
Usługa %1 niespodziewanie zakończyła pracę. Wystąpiło to razy: %2.
.

MessageId=7035
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_CONTROL_SUCCESS
Language=English
The %1 service was successfully sent a %2 control.
.
Language=Russian
Служба "%1" успешно отправила "%2".
.
Language=Polish
Do usługi %1 został pomyślnie wysłany kod sterowania %2.
.

MessageId=7036
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_STATUS_SUCCESS
Language=English
The %1 service entered the %2 state.
.
Language=Russian
Служба "%1" перешла в состояние "%2".
.
Language=Polish
Usługa %1 weszła w stan %2.
.

MessageId=7037
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CONFIG_BACKOUT_FAILED
Language=English
The Service Control Manager encountered an error undoing a configuration change
to the %1 service.  The service's %2 is currently in an unpredictable state.

If you do not correct this configuration, you may not be able to restart the %1
service or may encounter other errors.  To ensure that the service is configured
properly, use the Services snap-in in Microsoft Management Console (MMC).
.
Language=Polish
Menedżer sterowania usługami napotkał błąd podczas cofania zmiany konfiguracji
usługi %1. Element usługi: %2 jest obecnie w stanie nieprzewidywalnym.

Jeśli ta konfiguracja nie zostanie naprawiona, ponowne uruchomienie usługi
%1 może okazać się niemożliwe lub mogą wystąpić inne błędy. Aby upewnić się, 
że usługa jest skonfigurowana właściwie, użyj przystawki Usługi w programie
ReactOS Management Console (MMC).
.

MessageId=7038
Severity=Error
Facility=System
SymbolicName=EVENT_FIRST_LOGON_FAILED_II
Language=English
The %1 service was unable to log on as %2 with the currently configured
password due to the following error: %n%3%n%nTo ensure that the service is
configured properly, use the Services snap-in in Microsoft Management
Console (MMC).
.
Language=Polish
Usługa %1 nie mogła zalogować się jako %2 z aktualnie skonfigurowanym 
hasłem z powodu następującego błędu: %n%3%n%nAby upewnić się, że usługa 
jest skonfigurowana właściwie, użyj przystawki Usługi w programie
ReactOS Management Console (MMC).
.

MessageId=7039
Severity=Warning
Facility=System
SymbolicName=EVENT_SERVICE_DIFFERENT_PID_CONNECTED
Language=English
A service process other than the one launched by the Service Control Manager
connected when starting the %1 service.  The Service Control Manager launched
process %2 and process %3 connected instead.%n%n

Note that if this service is configured to start under a debugger, this behavior is expected.
.
Language=Polish
Proces usługi innej niż uruchomiona przez Menedżer sterowania usługami
połączył się podczas uruchamiania usługi %1. Menedżer sterowania usługami uruchomił
proces %2, a połączył się proces %3.%n%n

Jeśli ta usługa jest skonfigurowana do uruchomienia z debugerem, takie zachowanie jest oczekiwane.
.

MessageId=7040
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_START_TYPE_CHANGED
Language=English
The start type of the %1 service was changed from %2 to %3.
.
Language=Russian
Тип запуска службы "%1" изменен с "%2" на "%3".
.
Language=Polish
Typ startowy usługi %1 został zmieniony z %2 do %3.
.

MessageId=7041
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_LOGON_TYPE_NOT_GRANTED
Language=English
The %1 service was unable to log on as %2 with the currently configured password due to the following error:%n
Logon failure: the user has not been granted the requested logon type at this computer.%n%n
Service: %1%n
Domain and account: %2%n%n
This service account does not have the necessary user right "Log on as a service."%n%n
User Action%n%n
Assign "Log on as a service" to the service account on this computer. You can
use Local Security Settings (Secpol.msc) to do this. If this computer is a
node in a cluster, check that this user right is assigned to the Cluster
service account on all nodes in the cluster.%n%n
If you have already assigned this user right to the service account, and the
user right appears to be removed, a Group Policy object associated with this
node might be removing the right. Check with your domain administrator to find
out if this is happening.
.
Language=Polish
Usługa %1 nie mogła się zalogować jako %2 z aktualnie skonfigurowanym hasłem z powodu następującego błędu:%n
Błąd logowania: użytkownik nie otrzymał wymaganego typu logowania na tym komputerze.%n%n
Usługa: %1%n
Domena i użytkownik: %2%n%n
To konto usługi nie zawiera potrzebnego prawa użytkownika "Zaloguj się jako usługa."%n%n
Akcje użytkownika%n%n
Przypisz "Zaloguj się jako usługa" do konta usługi na tym komputerze. Możesz użyć
Ustawień zabezpieczeń lokalnych (Secpol.msc), by to wykonać. Jeśli ten komputer jest
węzłem w klastrze, sprawdź czy to prawo użytkownika jest przypisane do konta usługi
Klaster na wszystkich węzłach tego klastra.%n%n
Jeśli już przypisałeś to prawo użytkownika do konta usługi
i prawo użytkownika wygląda na usunięte, obiekt zasad grupy powiązany z tym
węzłem może usuwać to prawo. Skontaktuj się z administratorem domeny by sprawdzić,
czy to się dokonuje.
.

MessageId=7042
Severity=Informational
Facility=System
SymbolicName=EVENT_SERVICE_STOP_SUCCESS_WITH_REASON
Language=English
The %1 service was successfully sent a %2 control.%n%n
The reason specified was: %3 [%4]%n%n
Comment: %5
.
Language=Polish
Do usługi %1 został pomyślnie wysłany kod sterowania %2.%n%n
Powód: %3 [%4]%n%n
Komentarz: %5
.

MessageId=7043
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_SHUTDOWN_FAILED
Language=English
The %1 service did not shut down properly after receiving a preshutdown control.
.
Language=Polish
Usługa %1 nie została poprawnie wyłączona po otrzymaniu kodu sterownika przed zamknięciem systemu.
.

;
; transport events 9000-9499
;

MessageId=9004
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSPORT_REGISTER_FAILED
Language=English
%2 failed to register itself with the NDIS wrapper.
.
Language=Polish
%2: nie można zarejestrować w otoce NDIS.
.

MessageId=9006
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSPORT_ADAPTER_NOT_FOUND
Language=English
%2 could not find adapter %3.
.
Language=Russian
"%2" не смогла найти адаптер "%3".
.
Language=Polish
%2: nie można odnaleźć karty %3.
.
