MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
              )

LanguageNames=(English=0x409:MSG00409
               Polish=0x415:MSG00415
               Romanian=0x418:MSG00418
               Russian=0x419:MSG00419
               Spanish=0x40A:MSG0040A
               Turkish=0x41F:MSG0041F
               Chinese=0x804:MSG00804
               Taiwanese=0x404:MSG00404
              )


MessageId=10000
SymbolicName=MSG_ACCOUNTS_SYNTAX
Severity=Success
Facility=System
Language=English
NET ACCOUNTS [/FORCELOGOFF:{Minutes|NO}] [/MINPWLEN:Length]
             [/MAXPWAGE:{Days|UNLIMITED}] [/MINPWAGE:Days]
             [/UNIQUEPW:Count] [/DOMAIN]
.
Language=Polish
NET ACCOUNTS [/FORCELOGOFF:{minuty|NO}] [/MINPWLEN:długość]
             [/MAXPWAGE:{dni|UNLIMITED}] [/MINPWAGE:dni]
             [/UNIQUEPW:liczba] [/DOMAIN]
.
Language=Romanian
NET ACCOUNTS [/FORCELOGOFF:{Minute|NO}] [/MINPWLEN:Lungime]
             [/MAXPWAGE:{Zile|UNLIMITED}] [/MINPWAGE:Zile]
             [/UNIQUEPW:Număr] [/DOMAIN]
.
Language=Russian
NET ACCOUNTS [/FORCELOGOFF:{минуты | NO}] [/MINPWLEN:длина]
             [/MAXPWAGE:{дни | UNLIMITED}] [/MINPWAGE:дни]
             [/UNIQUEPW:число] [/DOMAIN]
.
Language=Spanish
NET ACCOUNTS [/FORCELOGOFF:{minutos | NO}] [/MINPWLEN:longitud]
             [/MAXPWAGE:{días | UNLIMITED}] [/MINPWAGE:días]
             [/UNIQUEPW:número] [/DOMAIN]
.
Language=Turkish
NET ACCOUNTS [/FORCELOGOFF:{Dakîka|NO}] [/MINPWLEN:Uzunluk]
             [/MAXPWAGE:{Gün|UNLIMITED}] [/MINPWAGE:Gün]
             [/UNIQUEPW:Sayı] [/DOMAIN]
.
Language=Chinese
NET ACCOUNTS [/FORCELOGOFF:{Minutes|NO}] [/MINPWLEN:Length]
             [/MAXPWAGE:{Days|UNLIMITED}] [/MINPWAGE:Days]
             [/UNIQUEPW:Count] [/DOMAIN]
.
Language=Taiwanese
NET ACCOUNTS [/FORCELOGOFF:{分鐘|NO}] [/MINPWLEN:長度]
             [/MAXPWAGE:{天|UNLIMITED}] [/MINPWAGE:天]
             [/UNIQUEPW:計數] [/DOMAIN]
.


MessageId=10001
SymbolicName=MSG_ACCOUNTS_HELP
Severity=Success
Facility=System
Language=English
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts have been set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started automatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Polish
NET ACCOUNTS uaktualnia bazę kont użytkowników i zmienia hasło oraz wymagania
logowania dla wszystkich kont.
Użyte bez opcji, NET ACCOUNTS wyświetla bieżące ustawienia
hasła i ograniczeń logowania oraz informacje o domenie.

Aby opcje użyte z NET ACCOUNTS odniosły skutek, muszą być
spełnione dwa warunki.

- Wymagania hasła i logowania odnoszą skutek tylko wtedy, gdy
  zostało utworzone konto użytkownika (w tym celu użyj
  Menedżera użytkowników lub polecenia NET USER).

- Usługa logowania (Net Logon) musi być uruchomiona na wszystkich
  serwerach w domenie, które weryfikują logowanie. Usługa logowania
  jest uruchamiana automatycznie podczas uruchamiania systemu.

/FORECELOGOFF:{minuty | NO}    Ustawia liczbę minut, przez które użytkownik
                               może być zalogowany przed wymuszeniem wylogowania
                               wskutek wygaśnięcia lub ważności godzin logowania.
                               NO, wartość domyślna, zapobiega wymuszaniu
                               wylogowania.
/MINPWLEN:długość              Ustawia minimalną liczbę znaków w haśle.
                               Zakres długości hasła wynosi od 0 do 14 znaków;
                               wartość domyślna to 6 znaków.
/MAXPWAGE:{dni | UNLIMITED}    Ustawia maksymalną liczbę dni ważności
                               hasła. UNLIMITED ustala nieograniczony
                               czas ważności hasła. Wartość /MAXPWAGE nie
                               może być mniejsza od /MINPWAGE. Zakres wynosi
                               od 1 do 999; domyślnie wartość się nie zmienia.
/MINPWAGE:dni                  Ustawia minimalną liczbę dni, które muszą
                               minąć, zanim użytkownik może zmienić hasło.
                               Wartość 0 ustawia brak tego ograniczenia.
                               Zakres wynosi od 0 do 999; wartość domyślna
                               to 0 dni. Wartość /MINPWAGE nie może być
                               większa od wartości /MAXPWAGE.
/UNIQUEPW:liczba               Wymaga, aby hasło użytkownika było unikatowe,
                               poprzez określoną liczbę zmian hasła.
                               Największa wartość to 24.
/DOMAIN                        Wykonuje operacje na kontrolerze domeny
                               w bieżącej domenie. W innym wypadku,
                               operacje te są dokonywane na komputerze
                               lokalnym.
.
Language=Romanian
NET ACCOUNTS actualizează baza de utilizatori și modifică parola și
cerințele de autentificare pentru toate conturile.
Utilizat fără parametri, NET ACCOUNTS afișează configurația curentă pentru
parole, limitări de autentificare, și informații de domeniu.

Sunt necesare două condiții pentru ca opțiunile utilizate cu
NET ACCOUNTS să aibă efect.

- Cerințele de autentificare și parolele au sens doar când conturile
  sunt instituite (cu Gestionarul de Utilizatori sau comanda NET USER).

- Este necesar ca serviciul Net Logon să fie activ în toate servele din
  domeniul de autentificare. Serviciul Net Logon este lansat automat la
  pornirea sistemului de operare.

/FORECELOGOFF:{minute | NO}    Definește numărul minutelor precursoare unei
                               deautentificări forțate la expirarea contului
                               sau a numărului valid de ore de autentificare.
                               NO (implicit) previne deautentificarea forțată.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Russian
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts havebeen set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started automatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Spanish
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts have been set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started automatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Turkish
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts havebeen set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started autmatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Chinese
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts havebeen set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started automatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.
Language=Taiwanese
NET ACCOUNTS updates the user accounts database and modifies password
and logon requirements for all accounts.
When used without parameters, NET ACCOUNTS displays the current settings for
passwords, logon limitations, and domain information.

Two conditions are required in order for options used with
NET ACCOUNTS to take effect.

- The password and logon requirements are only effective if user
  accounts havebeen set up (user User Manager or the NET USER command).

- The Net Logon service must be running on all servers in the domain
  that verify logon. Net Logon is started automatically when the
  operating system starts.

/FORECELOGOFF:{minutes | NO}   Sets the number of minutes a user has before
                               being forced to log off when the
                               account expires or valid logon hours expire.
                               NO, the default, prevents forced logoff.
/MINPWLEN:length               Sets the minimum number of characters for
                               a password. The range is 0-14 characters;
                               the default is 6 characters.
/MAXPWAGE:{days | UNLIMITED}   Sets the maximum numer of days that a
                               password is valid. No limit is specified
                               by using UNLIMITED. /MAXPWAGE cannot be less
                               than /MINPWAGE. The range is 1-999; the
                               default is to leave the value unchanged.
/MINPWAGE:days                 Sets the minimum number of days that must
                               pass before a user can change a password.
                               A value of 0 sets no minimum time. The range
                               is 0-999; the default is 0 days. /MINPWAGE
                               cannot be more than /MAXPWAGE.
/UNIQUEPW:number               Requires that a users passwords be unique
                               through the specified number of password
                               changes. The maximum value is 24.
/DOMAIN                        Performs the operation on a domain
                               controller of the current domain. Otherwise,
                               the operation is performed on the local
                               computer.
.


MessageId=10002
SymbolicName=MSG_COMPUTER_SYNTAX
Severity=Success
Facility=System
Language=English
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Polish
NET COMPUTER \\nazwa_komputera {/ADD | /DEL}
.
Language=Romanian
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Russian
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Spanish
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Turkish
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Chinese
NET COMPUTER \\computername {/ADD | /DEL}
.
Language=Taiwanese
NET COMPUTER \\computername {/ADD | /DEL}
.


MessageId=10003
SymbolicName=MSG_COMPUTER_HELP
Severity=Success
Facility=System
Language=English
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Polish
NET COMPUTER dodaje lub usuwa komputer z bazy danych domeny.
To polecenie jest dostępne tylko na serwerowych systemach opoeracyjnych.

\\nazwa_komputera   Określa komputer dodawany lub usuwany
                    z domeny.
/ADD                Dodaje określony komputer do domeny.
/DEL                Usuwa określony komputer z domeny.
.
Language=Romanian
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Russian
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Spanish
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Turkish
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Chinese
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.
Language=Taiwanese
NET COMPUTER adds or deletes computers from a domain database. This
command is available only on server operating systems.

\\computername   Specifies the computer to add or delete from
                 the domain.
/ADD             Adds the specified computer to the domain.
/DEL             Removes the specified computer from the domain.
.


MessageId=10004
SymbolicName=MSG_CONFIG_SYNTAX
Severity=Success
Facility=System
Language=English
NET CONFIG [SERVER | WORKSTATION]
.
Language=Polish
NET CONFIG [SERVER | WORKSTATION]
.
Language=Romanian
NET CONFIG [SERVER | WORKSTATION]
.
Language=Russian
NET CONFIG [SERVER | WORKSTATION]
.
Language=Spanish
NET CONFIG [SERVER | WORKSTATION]
.
Language=Turkish
NET CONFIG [SERVER | WORKSTATION]
.
Language=Chinese
NET CONFIG [SERVER | WORKSTATION]
.
Language=Taiwanese
NET CONFIG [SERVER | WORKSTATION]
.


MessageId=10005
SymbolicName=MSG_CONFIG_HELP
Severity=Success
Facility=System
Language=English
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Polish
NET CONFIG wyświetla informacje o konfiguracji usług Stacja robocza lub
Serwer. Polecenie użyte bez przełącznika SERVER lub WORKSTATION wyświetla,
listę usług dostępnych do konfiguracji. Aby uzyskać pomoc na temat
konfigurowania usługi, wpisz polecenie NET HELP CONFIG usługa.

SERVER        Wyświetla informacje o konfiguracji usługi Serwer
WORKSTATION   Wyświetla informacje o konfiguracji usługi Stacja robocza.
.
Language=Romanian
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Russian
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Spanish
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Turkish
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Chinese
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.
Language=Taiwanese
NET CONFIG displays coniguration information of the Workstation or
Server service. When used without the WORKSTATION or SERVER switch,
it displays a list of configurable services. To get help with
configuring a service, type NET HELP CONFIG service.

SERVER        Displays information about the configuration of the
              Server service.
WORKSTATION   Displays information about the configuration of the
              Workstation service.
.


MessageId=10006
SymbolicName=MSG_CONFIG_SERVER_SYNTAX
Severity=Success
Facility=System
Language=English
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Polish
NET CONFIG SERVER [/AUTODISCONNECT:czas] [/SRVCOMMENT:"tekst"]
                  [/HIDDEN:{YES | NO}]
.
Language=Romanian
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Russian
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Spanish
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Turkish
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Chinese
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.
Language=Taiwanese
NET CONFIG SERVER [/AUTODISCONNECT:time] [/SRVCOMMENT:"text"]
                  [/HIDDEN:{YES | NO}]
.


MessageId=10007
SymbolicName=MSG_CONFIG_SERVER_HELP
Severity=Success
Facility=System
Language=English
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Polish
NET CONFIG SERVER wyświetla lub zmienia ustawienia usługi Serwer.

/AUTODISCONNECT:czas    Ustawia maksymalną liczbę minut, przez
                        które sesja użytkownika może być nieaktywna, zanim
                        nastąpi jej rozłączenie. Użyj wartości -1, aby
                        zapobiec rozłączaniu. Zakres wynosi od -1 do 65535
                        minut, domyślnie 15.
/SRVCOMMENT:"tekst"     Dodaje komentarz dla serwera wyświetlany na
                        ekranie komputera i przez polecenie NET VIEW.
                        Tekst musi być ujęty w cudzysłów.
/HIDDEN:{YES | NO}      Określa, czy nazwa serwera pojawia się
                        podczas wyświetlania listy serwerów. Należy pamiętać,
                        że ukrycie serwera nie zmienia uprawnień na tym
                        serwerze. Wartość domyślna: NO (nie ukrywaj).

Aby wyświetlić bieżącą konfigurację usługi Serwer,
wpisz NET CONFIG SERVER bez parametrów.
.
Language=Romanian
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Russian
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Spanish
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Turkish
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Chinese
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.
Language=Taiwanese
NET CONFIG SERVER displays or changes settings for the Server service.

/AUTODISCONNECT:time    Sets the maximum number of minutes a user's
                        session can be inactive before it is disconected.
                        You can specify -1 to never disconnect. The range
                        is -1-65535 minutes; the default is 15.
/SRVCOMMENT:"text"      Adds a comment for the server that is displayed on
                        screen and with the NET VIEW command.
                        Enclose the text in quotation marks.
/HIDDEN:{YES | NO}      Specifies whether the server's computer name
                        appears on displays listings of servers. Note that
                        hiding a serverdoes not alter the permissions
                        on that server. The default is NO.

To display the current configuration for the Server service,
type NET CONFIG SERVER without parameters.
.


MessageId=10008
SymbolicName=MSG_CONTINUE_SYNTAX
Severity=Success
Facility=System
Language=English
NET CONTINUE service
.
Language=Polish
NET CONTINUE usługa
.
Language=Romanian
NET CONTINUE <nume serviciu>
.
Language=Russian
NET CONTINUE <имя_службы>
.
Language=Spanish
NET CONTINUE <nombre del servicio>
.
Language=Turkish
NET CONTINUE <Hizmet Adı>
.
Language=Chinese
NET CONTINUE <Service Name>
.
Language=Taiwanese
NET CONTINUE <服務名稱>
.


MessageId=10009
SymbolicName=MSG_CONTINUE_HELP
Severity=Success
Facility=System
Language=English
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Polish
NET CONTINUE wznawia działanie usługi, która została.
wstrzymana poleceniem NET PAUSE.

usługa              Wstrzymana usługa
                    Może to być jedna z następujących usług:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Romanian
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Russian
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Spanish
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Turkish
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Chinese
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
Language=Taiwanese
NET CONTINUE reactivates a service that has been suspended by NET PAUSE.

service             Is the paused service
                    For example, one of the following:
                    NET LOGON
                    NT LM SECURITY SUPPORT PROVIDER
                    SCHEDULE
                    SERVER
                    WORKSTATION
.
