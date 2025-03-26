;
; netevent.mc MESSAGE resources for netevent.dll
;
;
; IMPORTANT: When a new language is added, all messages in this file need to be
; either translated or at least duplicated for the new language.
; This is a new requirement by MS mc.exe
; To do this, start with a regex replace:
; - In VS IDE: "Language=English\r\n(?<String>(?:[^\.].*\r\n)*\.\r\n)" -> "Language=English\r\n${String}Language=MyLanguage\r\n${String}"
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
               French=0x040c:MSG0040c
               Polish=0x415:MSG00415
               Romanian=0x418:MSG00418)

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
Language=French
Le fichier de journalisation %1 est plein.
.
Language=Polish
Plik dziennika %1 jest zapełniony.
.
Language=Romanian
Fișierul de jurnalizare %1 e plin.
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
Language=French
Le fichier de journalisation %1 ne peut pas être ouvert.
.
Language=Polish
Nie można otworzyć pliku dziennika %1.
.
Language=Romanian
Fișierul de jurnalizare %1 nu poate fi deschis.
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
Language=French
Le fichier de journalisation %1 est corrompu et doit être vidé.
.
Language=Polish
Plik dziennika "%1" jest uszkodzony i zostanie wyczyszczony.
.
Language=Romanian
Fișierul de jurnalizare %1 este corupt și va fi golit.
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
Language=French
Le fichier de journalisation des application ne peut pas être ouvert. %1 sera utilisé comme fichier de journalisation par défaut.
.
Language=Polish
Nie można otworzyć pliku dziennika aplikacji. Jako domyślny plik dziennika zostanie użyty plik %1.
.
Language=Romanian
Fișierul de jurnalizare al aplicației nu a putut fi deschis.  %1 va fi folosit ca fișier de jurnalizare implicit.
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
Language=French
Un paquet de pilote reçu par le sous-système IO était invalide. La donnée est le paquet.
.
Language=Polish
Pakiet sterownika otrzymany z podsystemu wejścia/wyjścia jest niepoprawny. Przedstawione dane to otrzymany pakiet.
.
Language=Romanian
Un pachet al driverului primit de la subsistemul de In/Ex a fost nevalid. Datele sunt pachetul însuși.
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
Language=French
Le service de journalisation des évènements a été démarré.
.
Language=Polish
Uruchomiono usługę Dziennik zdarzeń.
.
Language=Romanian
Serviciul fișierului de jurnalizare al evenimentelor a fost pornit.
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
Language=French
Le service de journalisation des évènements a été arrêté.
.
Language=Polish
Zatrzymano usługę Dziennik zdarzeń.
.
Language=Romanian
Serviciul fișierului de jurnalizare al evenimentelor a fost oprit.
.

MessageId=6007
Severity=Warning
Facility=System
SymbolicName=TITLE_EventlogMessageBox
Language=English
Eventlog Service %0
.
Language=Russian

;
; IMPORTANT: When a new language is added, all messages in this file need to be
; either translated or at least duplicated for the new language.
; This is a new requirement by MS mc.exe
; To do this, start with a regex replace:
; - In VS IDE: "Language=English\r\n(?<String>(?:[^\.].*\r\n)*\.\r\n)" -> "Language=English\r\n${String}Language=MyLanguage\r\n${String}"
;

Служба журнала событий %0
.
Language=French
Service de journalisation des évènements %0
.
Language=Polish
Usługa Dziennik zdarzeń "%0"
.
Language=Romanian
Serviciul fișierului de jurnalizare %0
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
Language=French
L'arrêt précédent du système à %1 le %2 n'était pas prévu.
.
Language=Polish
Poprzednie zamknięcie systemu o godzinie %1 na %2 było nieoczekiwane.
.
Language=Romanian
Închiderea anterioară a sistemului la %1 pe %2 a fost neașteptată.
.

MessageId=6009
Severity=Warning
Facility=System
SymbolicName=EVENT_EventLogProductInfo
Language=English
ReactOS %1 %2 %3 %4.
.
Language=Russian
ReactOS %1 %2 %3 %4.
.
Language=French
ReactOS %1 %2 %3 %4.
.
Language=Polish
ReactOS %1 %2 %3 %4.
.
Language=Romanian
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
Language=French
Le service %1 n'a pas réussi à installer une source d'évènement.
.
Language=Polish
Usługa %1 nie mogła skonfigurować źródła zdarzenia.
.
Language=Romanian
Serviciul %1 nu a putut configura o sursă de evenimente.
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
Language=French
Le nom NetBIOS et le nom d'hôte DNS de cette machine ont été changés de %1 à %2.
.
Language=Polish
Nazwa NetBIOS i nazwa hosta DNS tego komputera została zmieniona z %1 na %2.
.
Language=Romanian
Numele NetBIOS și numele gazdei DNS de la acest calculator au fost schimbate din %1 în %2.
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
Language=French
Le domaine DNS assigné à cet ordinateur a été changé de %1 à %2.
.
Language=Polish
Domena DNS przydzielona do tego komputera uległa zmianie z %1 na %2.
.
Language=Romanian
Domeniul DNS atribuit acestui calculator a fost schimbat din %1 în %2.
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
Language=French
Un pilote spécifique monoprocesseur a été chargé sur un système multiprocesseurs. Le pilote n'a pas pu être chargé.
.
Language=Polish
Próbowano załadować sterownik jednoprocesorowy na komputerze wieloprocesorowym. Nie można załadować sterownika.
.
Language=Romanian
Un driver specific unui uniprocesor a fost încărcat într-un sistem de tip multiprocesor. Driverul nu a putut fi încărcat.
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
Language=French
Le service %1 n'a pas pu démarrer en raison de l'erreur suivante : %n%2
.
Language=Polish
Nie można uruchomić usługi %1 z powodu następującego błędu: %n%2
.
Language=Romanian
Serviciul %1 nu a putut porni din cauza următoarei erori: %n%2
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
Language=French
Le serveur %1 dépend du service %2 qui n'a pas pu démarrer en raison de l'erreur suivante : %n%3
.
Language=Polish
Usługa %1 zależy od usługi %2, której nie można uruchomić z powodu następującego błędu: %n%3
.
Language=Romanian
Serviciul %1 depinde de serviciul %2 care nu a putut porni din cauza următoarei erori: %n%3
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
Language=French
Le service %1 dépend du groupe %2 et aucun membre de ce groupe n'a démarré.
.
Language=Polish
Usługa %1 zależy od grupy %2, a nie uruchomiono żadnego członka tej grupy.
.
Language=Romanian
Serviciul %1 depinde de grupul %2 și niciun membru al acestui grup nu a pornit.
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
Language=French
Le service %1 dépend du service non existant suivant : %2
.
Language=Polish
Usługa %1 zależy od następującej nieistniejącej usługi: %2
.
Language=Romanian
Serviciul %1 depinde de următorul serviciu neexistent: %2
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
Language=French
L'appel %1 a échoué avec l'erreur suivante : %n%2
.
Language=Polish
Wywołanie %1 nie powiodło się i wystąpił następujący błąd: %n%2
.
Language=Romanian
Apelul %1 a eșuat cu următoarea eroare: %n%2
.

MessageId=7006
Severity=Error
Facility=System
SymbolicName=EVENT_CALL_TO_FUNCTION_FAILED_II
Language=English
The %1 call failed for %2 with the following error: %n%3
.
Language=Russian
The %1 call failed for %2 with the following error: %n%3
.
Language=French
L'appel %1 a échoué pour %2 avec l'erreur suivante : %n%3
.
Language=Polish
Wywołanie %1 dla %2 nie powiodło się i wystąpił następujący błąd: %n%3
.
Language=Romanian
Apelul %1 a eșuat pentru %2 cu următoarea eroare: %n%3
.

MessageId=7007
Severity=Error
Facility=System
SymbolicName=EVENT_REVERTED_TO_LASTKNOWNGOOD
Language=English
The system reverted to its last known good configuration.  The system is restarting....
.
Language=Russian
The system reverted to its last known good configuration.  The system is restarting....
.
Language=French
Le système a restauré sa dernière bonne configuration connue. Le système redémarre...
.
Language=Polish
System powrócił do ostatniej znanej dobrej konfiguracji. Trwa ponowne uruchamianie systemu...
.
Language=Romanian
Sistemul a revenit la cea mai bună configurație anterioară. Se repornește sistemul....
.

MessageId=7008
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_ACCOUNT_NAME
Language=English
No backslash is in the account name.
.
Language=Russian
No backslash is in the account name.
.
Language=French
Aucun backslash n'est présent dans le nom de compte.
.
Language=Polish
W nazwie konta brak znaku ukośnika odwrotnego.
.
Language=Romanian
Lipsește solidusul întors din numele contului.
.

MessageId=7009
Severity=Error
Facility=System
SymbolicName=EVENT_CONNECTION_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for the %2 service to connect.
.
Language=Russian
Timeout (%1 milliseconds) waiting for the %2 service to connect.
.
Language=French
Expiration du délai (%1 millisecondes) lors de l'attente du service %2 pour se connecter.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na połączenie się z usługą %2.
.
Language=Romanian
Pauza (de %1 (de) milisecundă(e)) așteaptă serviciul %2 pentru conectare.
.

MessageId=7010
Severity=Error
Facility=System
SymbolicName=EVENT_READFILE_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for ReadFile.
.
Language=Russian
Timeout (%1 milliseconds) waiting for ReadFile.
.
Language=French
Expiration du délai (%1 millisecondes) lors de l'attente de ReadFile.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na operację ReadFile.
.
Language=Romanian
Pauza (de %1 (de) milisecundă(e)) așteaptă fișierul de citire ReadFile.
.

MessageId=7011
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_TIMEOUT
Language=English
Timeout (%1 milliseconds) waiting for a transaction response from the %2 service.
.
Language=Russian
Timeout (%1 milliseconds) waiting for a transaction response from the %2 service.
.
Language=French
Expiration du délai (%1 millisecondes) lors de l'attente pour une transaction réponse du service %2.
.
Language=Polish
Upłynął limit czasu (%1 milisekund) podczas oczekiwania na odpowiedź transakcji z usługi %2.
.
Language=Romanian
Pauza (de %1 (de) milisecundă(e)) așteaptă un răspuns de tranzacție de la serviciul %2.
.

MessageId=7012
Severity=Error
Facility=System
SymbolicName=EVENT_TRANSACT_INVALID
Language=English
Message returned in transaction has incorrect size.
.
Language=Russian
Message returned in transaction has incorrect size.
.
Language=French
Le message retourné dans la transaction a une taille incorrecte.
.
Language=Polish
Wiadomość zwrócona w transakcji ma niepoprawny rozmiar.
.
Language=Romanian
Mesajul returnat în tranzacție are o dimensiune incorectă.
.

MessageId=7013
Severity=Error
Facility=System
SymbolicName=EVENT_FIRST_LOGON_FAILED
Language=English
Logon attempt with current password failed with the following error: %n%1
.
Language=Russian
Logon attempt with current password failed with the following error: %n%1
.
Language=French
La tentative de connexion avec le mot de passe actuel a échoué avec l'erreur suivante : %n%1
.
Language=Polish
Próba logowania za pomocą aktualnego hasła nie powiodła się; wystąpił następujący błąd: %n%1
.
Language=Romanian
Încercarea de autentificare cu parola curentă a eșuat cu următoarea eroare: %n%1
.

MessageId=7014
Severity=Error
Facility=System
SymbolicName=EVENT_SECOND_LOGON_FAILED
Language=English
Second logon attempt with old password also failed with the following error: %n%1
.
Language=Russian
Second logon attempt with old password also failed with the following error: %n%1
.
Language=French
La seconde tentative de connexion avec l'ancien mot de passe a également échoué avec l'erreur suivante : %n%1
.
Language=Polish
Druga próba logowania za pomocą starego hasła także się nie powiodła; wystąpił następujący błąd: %n%1
.
Language=Romanian
A doua încercare de autentificare cu parola veche a eșuat de asemenea, cu următoarea eroare: %n%1
.

MessageId=7015
Severity=Error
Facility=System
SymbolicName=EVENT_INVALID_DRIVER_DEPENDENCY
Language=English
Boot-start or system-start driver (%1) must not depend on a service.
.
Language=Russian
Boot-start or system-start driver (%1) must not depend on a service.
.
Language=French
Un pilote au démarrage ou système (%1) ne doit pas dépendre d'un service.
.
Language=Polish
Sterowniki startu rozruchowego oraz systemowego (%1) nie mogą zależeć od usługi.
.
Language=Romanian
Driverul de pornire a inițializării sau al sistemului (%1) nu trebuie să depindă de un serviciu.
.

MessageId=7016
Severity=Error
Facility=System
SymbolicName=EVENT_BAD_SERVICE_STATE
Language=English
The %1 service has reported an invalid current state %2.
.
Language=Russian
The %1 service has reported an invalid current state %2.
.
Language=French
Le service %1 a reporté un état courant invalide %2.
.
Language=Polish
Usługa %1 zaraportowała nieprawidłowy stan bieżący %2.
.
Language=Romanian
Serviciul %1 a raportat o stare curentă nevalidă %2.
.

MessageId=7017
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_DEMAND
Language=English
Detected circular dependencies demand starting %1.
.
Language=Russian
Detected circular dependencies demand starting %1.
.
Language=French
Dépendances circulaires détectées lors du démarrage de %1.
.
Language=Polish
Wykryte zależności cykliczne wymagają uruchomienia %1.
.
Language=Romanian
A fost detectată o cerere de dependențe circulare, începând cu %1.
.

MessageId=7018
Severity=Error
Facility=System
SymbolicName=EVENT_CIRCULAR_DEPENDENCY_AUTO
Language=English
Detected circular dependencies auto-starting services.
.
Language=Russian
Detected circular dependencies auto-starting services.
.
Language=French
Dépendances circulaires détectées lors du démarrage automatique des services.
.
Language=Polish
Wykryto automatycznie uruchamiane usługi zależności cyklicznych.
.
Language=Romanian
Au fost detectate servicii de pornire automată de dependențe circulare.
.

MessageId=7019
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_SERVICE
Language=English
Circular dependency: The %1 service depends on a service in a group which starts later.
.
Language=Russian
Circular dependency: The %1 service depends on a service in a group which starts later.
.
Language=French
Dépendance circulaire : le service %1 dépend d'un service dans un groupe qui démarre plus tard.
.
Language=Polish
Zależność cykliczna: usługa %1 zależy od usługi z grupy uruchamianej później.
.
Language=Romanian
Dependență circulară: Serviciul %1 depinde de un serviciu dintr-un grup ce pornește mai târziu.
.

MessageId=7020
Severity=Error
Facility=System
SymbolicName=EVENT_DEPEND_ON_LATER_GROUP
Language=English
Circular dependency: The %1 service depends on a group which starts later.
.
Language=Russian
Circular dependency: The %1 service depends on a group which starts later.
.
Language=French
Dépendance circulaire : le service %1 dépend d'un groupe qui démarre plus tard.
.
Language=Polish
Zależność cykliczna: usługa %1 zależy od grupy uruchamianej później.
.
Language=Romanian
Dependență circulară: Serviciul %1 depinde de un grup ce pornește mai târziu.
.

MessageId=7021
Severity=Error
Facility=System
SymbolicName=EVENT_SEVERE_SERVICE_FAILED
Language=English
About to revert to the last known good configuration because the %1 service failed to start.
.
Language=Russian
About to revert to the last known good configuration because the %1 service failed to start.
.
Language=French
Sur le point de revenir à la dernière bonne configuration connue car le service %1 n'a pas réussi à démarrer.
.
Language=Polish
System powróci do ostatniej znanej dobrej konfiguracji, ponieważ uruchomienie usługi %1 nie powiodło się.
.
Language=Romanian
Sunteți pe cale să anulați revenirea la ultima configurație bună cunoscută din cauză că serviciul %1 nu a putut porni.
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
Language=French
Le serveur %1 service s'est gelé au démarrage.
.
Language=Polish
Usługa %1 zawiesiła się podczas uruchamiania.
.
Language=Romanian
Serviciul %1 a fost blocat la pornire.
.

MessageId=7023
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED
Language=English
The %1 service terminated with the following error: %n%2
.
Language=Russian
The %1 service terminated with the following error: %n%2
.
Language=French
Le service %1 s'est terminé avec l'erreur suivante : %n%2
.
Language=Polish
Usługa %1 zakończyła działanie; wystąpił następujący błąd: %n%2
.
Language=Romanian
Serviciul %1 a fost închis cu următoarea eroare: %n%2
.

MessageId=7024
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_EXIT_FAILED_SPECIFIC
Language=English
The %1 service terminated with service-specific error %2.
.
Language=Russian
The %1 service terminated with service-specific error %2.
.
Language=French
Le service %1 s'est terminé avec une erreur spécifique au service %2.
.
Language=Polish
Usługa %1 zakończyła działanie; wystąpił specyficzny dla niej błąd %2.
.
Language=Romanian
Serviciul %1 s-a terminat cu o eroare specifică unui serviciu %2.
.

MessageId=7025
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_START_AT_BOOT_FAILED
Language=English
At least one service or driver failed during system startup.  Use Event Viewer to examine the event log for details.
.
Language=Russian
At least one service or driver failed during system startup.  Use Event Viewer to examine the event log for details.
.
Language=French
Au moins un service ou pilote a échoué durant le démarrage du système. Utilisez la visionneuse d'évènements pour examiner les journaux d'évènements pour plus de détails.
.
Language=Polish
Przynajmniej jedna usługa lub jeden sterownik nie dały się uruchomić podczas uruchamiania systemu. Użyj Podglądu zdarzeń, aby znaleźć szczegółowe informacje w dzienniku zdarzeń.
.
Language=Romanian
Cel puțin un serviciu sau un driver a eșuat, în timpul pornirii sistemului. Folosiți Vizualizatorul de evenimente pentru a examina jurnalul pentru detalii.
.

MessageId=7026
Severity=Error
Facility=System
SymbolicName=EVENT_BOOT_SYSTEM_DRIVERS_FAILED
Language=English
The following boot-start or system-start driver(s) failed to load: %1
.
Language=Russian
The following boot-start or system-start driver(s) failed to load: %1
.
Language=French
Le pilote au démarrage ou système suivant n'a pas pu être chargé : %1
.
Language=Polish
Nie można załadować następujących sterowników startu rozruchowego lub systemowego: %1
.
Language=Romanian
Următorul(oarele) driver(e) de pornire pentru inițializare sau de pornire a sistemului nu a(u) putut fi încărcat(e): %1
.

MessageId=7027
Severity=Error
Facility=System
SymbolicName=EVENT_RUNNING_LASTKNOWNGOOD
Language=English
ReactOS could not be started as configured.  A previous working configuration was used instead.
.
Language=Russian
ReactOS could not be started as configured.  A previous working configuration was used instead.
.
Language=French
ReactOS n'a pas pu démarrer tel que configuré. Une précédente configuration fonctionnelle a été utilisé à la place.
.
Language=Polish
Nie można uruchomić systemu ReactOS zgodnie z aktualną konfiguracją. Zamiast niej użyto poprzedniej działającej konfiguracji.
.
Language=Romanian
ReactOS nu a putut fi pornit după configurație. În locul ei, a fost folosită o configurație funcțională anterioară.
.

MessageId=7028
Severity=Error
Facility=System
SymbolicName=EVENT_TAKE_OWNERSHIP
Language=English
The %1 Registry key denied access to SYSTEM account programs so the Service Control Manager took ownership of the Registry key.
.
Language=Russian
The %1 Registry key denied access to SYSTEM account programs so the Service Control Manager took ownership of the Registry key.
.
Language=French
La clé de registre %1 a refusé l'accès aux programmes du compte SYSTEM. Le gestionnaire de contrôle des services s'est approprié la clé de registre.
.
Language=Polish
Klucz Rejestru %1 nie przyznał dostępu do programów konta SYSTEM, w związku z czym Menedżer sterowania usługami przejął ten klucz na własność.
.
Language=Romanian
Cheia de registru %1 a blocat accesul la programele contului de sistem (SYSTEM), astfel încât Administratorul de control al serviciului a preluat proprietatea la cheia de registru.
.

MessageId=7029
Severity=Error
Facility=System
SymbolicName=TITLE_SC_MESSAGE_BOX
Language=English
Service Control Manager %0
.
Language=Russian
Service Control Manager %0
.
Language=French
Gestionnaire de contrôle des services %0
.
Language=Polish
Menedżer sterowania usługami %0
.
Language=Romanian
Administratorul de control al serviciului %0
.

MessageId=7030
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_NOT_INTERACTIVE
Language=English
The %1 service is marked as an interactive service.  However, the system is configured to not allow interactive services.  This service may not function properly.
.
Language=Russian
The %1 service is marked as an interactive service.  However, the system is configured to not allow interactive services.  This service may not function properly.
.
Language=French
Le service %1 est marqué comme service interactif. Cependant, le système est configuré pour ne pas autoriser les services interactifs. Ce service pourrait ne pas fonctionner correctement.
.
Language=Polish
Usługa %1 jest oznaczona jako usługa interakcyjna. System jest jednak skonfigurowany tak, aby nie zezwalać na usługi interakcyjne, dlatego ta usługa może nie działać właściwie.
.
Language=Romanian
Serviciul %1 este marcat ca un serviciu inactiv. Totuși, sistemul este configurat să nu permită serviciile interactive. Acest serviciu nu poate funcționa corespunzător.
.

MessageId=7031
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_CRASH
Language=English
The %1 service terminated unexpectedly.  It has done this %2 time(s).  The following corrective action will be taken in %3 milliseconds: %5.
.
Language=Russian
The %1 service terminated unexpectedly.  It has done this %2 time(s).  The following corrective action will be taken in %3 milliseconds: %5.
.
Language=French
Le service %1 s'est terminée de façon non prévue. Il l'a fait %2 fois. L'action corrective suivante va être prise dans %3 millisecondes : %5.
.
Language=Polish
Usługa %1 niespodziewanie zakończyła pracę. Wystąpiło to razy: %2. W przeciągu %3 milisekund zostanie podjęta następująca czynność korekcyjna: %5.
.
Language=Romanian
Serviciul %1 a fost închis în mod neașteptat. Acest lucru a fost făcut (de) %2 dată(ori). Următoarea măsură corectivă va fi luată în %3 milisecondă(e): %5.
.

MessageId=7032
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_RECOVERY_FAILED
Language=English
The Service Control Manager tried to take a corrective action (%2) after the unexpected termination of the %3 service, but this action failed with the following error: %n%4
.
Language=Russian
The Service Control Manager tried to take a corrective action (%2) after the unexpected termination of the %3 service, but this action failed with the following error: %n%4
.
Language=French
Le gestionnaire de contrôle des services a tenté de prendre une action corrective (%2) après la fin inattendue du service %3, mais cette action a échoué avec l'erreur suivante : %n%4
.
Language=Polish
Menedżer sterowania usługami próbował podjąć akcję korekcyjną (%2) po nieoczekiwanym zakończeniu usługi %3, ale ta akcja nie powiodła się przy następującym błędzie: %n%4.
.
Language=Romanian
Administratorul de control al serviciului a încercat să ia o acțiune corectivă (%2) după o închidere neașteptată a serviciului %3, dar această acțiune a eșuat cu următoarea eroare: %n%4
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
Language=Russian
The Service Control Manager did not initialize successfully. The security
configuration server (scesrv.dll) failed to initialize with error %1.  The
system is restarting...
.
Language=French
Le gestionnaire de contrôle des services ne s'est pas initialisé correctement.
Le serveur de configuration de sécurité (scesrv.dll) n'a pas réussi à s'initialiser
avec l'erreur %1. Le système redémarre...
.
Language=Polish
Menedżer sterowania usługami nie został zainicjowany pomyślnie. Nie udało się zainicjować serwera
konfiguracji zabezpieczeń (scesrv.dll) z powodu błędu %1. Trwa ponowne uruchamianie systemu...
.
Language=Romanian
Administratorul de control al serviciului nu s-a putut inițializa cu succes. Securitatea
serverului de configurație (scesrv.dll) a eșuat să se inițializeze cu eroarea %1.
Se repornește sistemul...
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
Language=French
Le service %1 s'est terminée de façon non prévue. Il l'a fait %2 fois.
.
Language=Polish
Usługa %1 niespodziewanie zakończyła pracę. Wystąpiło to razy: %2.
.
Language=Romanian
Serviciul %1 a fost închis în mod neașteptat. Acest lucru a fost făcut %2 dată(ori).
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
Language=French
Le contrôle %2 a été envoyé avec succès au service %1.
.
Language=Polish
Do usługi %1 został pomyślnie wysłany kod sterowania %2.
.
Language=Romanian
Serviciul %1 a trimis cu succes un control %2.
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
Language=French
Le service %1 est entré dans l'état %2.
.
Language=Polish
Usługa %1 weszła w stan %2.
.
Language=Romanian
Serviciul %1 a intrat în stadiul %2.
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
Language=Russian
The Service Control Manager encountered an error undoing a configuration change
to the %1 service.  The service's %2 is currently in an unpredictable state.

If you do not correct this configuration, you may not be able to restart the %1
service or may encounter other errors.  To ensure that the service is configured
properly, use the Services snap-in in Microsoft Management Console (MMC).
.
Language=French
Le gestionnaire de contrôle des services a rencontré une erreur lors de l'annulation
d'un changement de configuration sur le service %1. Le %2 du service est actuellement
dans un état imprévisible.

Si vous ne corrigez pas cette configuration, vous pourriez ne plus être capable de
redémarrer le service %1 ou pourriez rencontrer d'autres erreurs. Pour vous assurer
que le service est configuré correctement, utilisez la console de gestion des services
dans la console de gestion Microsoft (MMC).
.
Language=Polish
Menedżer sterowania usługami napotkał błąd podczas cofania zmiany konfiguracji
usługi %1. Element usługi: %2 jest obecnie w stanie nieprzewidywalnym.

Jeśli ta konfiguracja nie zostanie naprawiona, ponowne uruchomienie usługi
%1 może okazać się niemożliwe lub mogą wystąpić inne błędy. Aby upewnić się,
że usługa jest skonfigurowana właściwie, użyj przystawki Usługi w programie
ReactOS Management Console (MMC).
.
Language=Romanian
Administratorul de control al serviciului a întâlnit o eroare în încercarea de a anula o schimbare de configurație
la serviciul %1. %2-ul serviciului este deocamdată într-un stadiu neprevăzut.

Dacă nu corectați această configurație, nu veți putea reporni serviciul %1
sau veți întâmpina alte erori. Pentru a vă asigura că serviciul este configurat
corect, folosiți Administratorul de servicii în Consola de administrare (MMC).
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
Language=Russian
The %1 service was unable to log on as %2 with the currently configured
password due to the following error: %n%3%n%nTo ensure that the service is
configured properly, use the Services snap-in in Microsoft Management
Console (MMC).
.
Language=French
Le service %1 n'a pas réussi à s'identifier en tant que %2 avec le mot de passe
actuellement configuré en raison de l'erreur suivante : %n%3%n%nPour vous assurer
que le service est correctement, utilisez la console de gestion des services
dans la console de gestion Microsoft (MMC).
.
Language=Polish
Usługa %1 nie mogła zalogować się jako %2 z aktualnie skonfigurowanym
hasłem z powodu następującego błędu: %n%3%n%nAby upewnić się, że usługa
jest skonfigurowana właściwie, użyj przystawki Usługi w programie
ReactOS Management Console (MMC).
.
Language=Romanian
Serviciul %1 nu a putut fi logat la %2 cu parola configurată din prezent,
din cauza următoarei erori: %n%3%n%nPentru a vă asigura că serviciul este
configurat în mod corespunzător, folosiți Administratorul de servicii în Consola de
administrare (MMC).
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
Language=Russian
A service process other than the one launched by the Service Control Manager
connected when starting the %1 service.  The Service Control Manager launched
process %2 and process %3 connected instead.%n%n

Note that if this service is configured to start under a debugger, this behavior is expected.
.
Language=French
Un processus du service, autre que celui lancé par le gestionnaire de contrôle des services
s'est connecté lors du démarrage du service %1. Le gestionnaire de contrôle des services a
lancé le processus %2 et le processus %3 s'est connecté à la place.%n%n

Veuillez noter que si ce service est configurer pour démarrer dans un débogueur, ce comportement est attendu.
.
Language=Polish
Proces usługi innej niż uruchomiona przez Menedżer sterowania usługami
połączył się podczas uruchamiania usługi %1. Menedżer sterowania usługami uruchomił
proces %2, a połączył się proces %3.%n%n

Jeśli ta usługa jest skonfigurowana do uruchomienia z debugerem, takie zachowanie jest oczekiwane.
.
Language=Romanian
Un proces de tip serviciu, altul decât ceea ce este pornit în Administratorul de control al serviciilor
s-a conectat odată cu pornirea serviciului %1. Administratorul de control al serviciilor a lansat
procesul %2 și în schimb, s-a conectat procesul %3. %n%n

Rețineți că, dacă acest serviciu este configurat să pornească sub un depanator, acest comportament este de așteptat.
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
Language=French
Le type de démarrage du service %1 a été changé de %2 à %3.
.
Language=Polish
Typ startowy usługi %1 został zmieniony z %2 do %3.
.
Language=Romanian
Modul de pornire al serviciului %1 a fost schimbat din %2 în %3.
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
Language=Russian
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
Language=French
Le service %1 n'a pas réussi à s'identifier en tant que %2 avec le mot de passe
actuellement configuré en raison de l'erreur suivante : %n
Echec de connexion : l'utilisateur n'a pas reçu l'autorisation de réaliser ce type de connexions sur cet ordinateur.%n%n
Service : %1%n
Domaine et compte : %2%n%n
Ce compte de service n'a pas les droits utilisateurs nécessaires "Se connecter en tant que service".%n%n
Action utilisateur%n%n
Assignez le droit "Se connecter en tant que service" au compte de service sur cet ordinateur.
Vous pouvez utiliser les paramètres locaux de sécurité (secpol.msc) pour le faire. Si cet ordinateur
est un nœud dans une grappe, vérifiez que ce droit d'utilisateur est assigné au compte de service
sur tous les nœuds du cluster.%n%n
Si vous avez déjà assigné ce droit d'utilisateur au compte de service, et que ce droit semble avoir été supprimé,
un objet de stratégie de groupe (GPO) associé avec ce nœud peut avoir supprimé le droit.
Contactez votre administrateur de domaine pour vérifier si c'est ce qui se produit.
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
Language=Romanian
Serviciul %1 nu a putut fi autentificat ca %2 cu parola configurată în prezent, din cauza următoarei erori:%n
Autentificare eșuată: utilizatorului nu i-a fost acordat tipul de conectare solicitat la acest calculatorn%n
Serviciu: %1%n
Domeniu sau cont: %2%n%n
Acest cont de serviciu nu are drepturile de utilizator necesare "Autentificare ca un serviciu."%n%n
Acțiunea utilizatorului%n%n
Atribuiți "Autentificare ca un serviciu" la contul serviciului de pe acest calculator. Pentru a face asta
puteți folosi Setările de securitate locale (Secpol.msc). Dacă acest calculator este un
nod dintr-un grup, verificați dacă drepturile de utilizator sunt atribuite în
Grupul de conturi de serviciu pe toate nodurile din grup.%n%n
Dacă deja ați atribuit acest drept de utilizator la contul serviciului și
dreptul utilizatorului apare ca fiind șters, un obiect al Politicii grupului asociat cu acest
nod poate fi ștergerea dreptului. Verificați cu domeniul dumneavoastră de administrator pentru
a afla dacă se întâmplă acest lucru.
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
Language=Russian
The %1 service was successfully sent a %2 control.%n%n
The reason specified was: %3 [%4]%n%n
Comment: %5
.
Language=French
Le contrôle %2 a été envoyé avec succès au service %1.%n%n
La raison spécifiée était : %3 [%4]%n%n
Commentaire : %5
.
Language=Polish
Do usługi %1 został pomyślnie wysłany kod sterowania %2.%n%n
Powód: %3 [%4]%n%n
Komentarz: %5
.
Language=Romanian
Serviciul %1 a trimis cu succes un control %2.%n%n
Motivul specificat a fost: %3 [%4]%n%n
Comentariu: %5
.

MessageId=7043
Severity=Error
Facility=System
SymbolicName=EVENT_SERVICE_SHUTDOWN_FAILED
Language=English
The %1 service did not shut down properly after receiving a preshutdown control.
.
Language=Russian
The %1 service did not shut down properly after receiving a preshutdown control.
.
Language=French
Le service %1 ne s'est pas arrêté proprement après avoir reçu un contrôle de pré-arrêt.
.
Language=Polish
Usługa %1 nie została poprawnie wyłączona po otrzymaniu kodu sterownika przed zamknięciem systemu.
.
Language=Romanian
Serviciul %1 s-a oprit corect după primirea controlului de preoprire.
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
Language=Russian
%2 failed to register itself with the NDIS wrapper.
.
Language=French
%2 n'a pas réussi à s'enregistrer avec le wrapper NDIS.
.
Language=Polish
%2: nie można zarejestrować w otoce NDIS.
.
Language=Romanian
%2 nu s-a putut înregistra cu emulatorul NDIS.
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
Language=French
%2 n'a pas pu trouver l'adaptateur %3.
.
Language=Polish
%2: nie można odnaleźć karty %3.
.
Language=Romanian
%2 nu a putut găsi adaptorul %3.
.
