;// Romanian translation by Ștefan Fulea (stefan dot fulea at mail dot com)
;// Russian translation by Kudratov Olimjon (olim98@bk.ru)
;// Spanish translation by Ismael Ferreras Morezuelas <2014-11-07>
;// Turkish translation by 2015 Erdem Ersoy (eersoy93) (erdemersoy [at] erdemersoy [dot] net)
;// Simplified Chinese translation by Henry Tang Ih 2016 (henrytang2@hotmail.com)
;// Traditional Chinese translation by Henry Tang Ih 2016 (henrytang2@hotmail.com)
;// Polish translation Updated by pithwz - Piotr Hetnarowicz (piotrhwz@gmail.com) (April, 2020)

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

NET HELP command | MORE displays one screen at a time.
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

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
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

NET HELP comandă | MORE (pentru afișare paginată).
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

NET HELP имя_команды | MORE - постраничный просмотр справки.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET CONFIG wyświetla informacje o konfiguracji usług Stacja robocza lub
Serwer. Polecenie użyte bez przełącznika SERVER lub WORKSTATION wyświetla,
listę usług dostępnych do konfiguracji. Aby uzyskać pomoc na temat
konfigurowania usługi, wpisz polecenie NET HELP CONFIG usługa.

SERVER        Wyświetla informacje o konfiguracji usługi Serwer
WORKSTATION   Wyświetla informacje o konfiguracji usługi Stacja robocza.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
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

NET HELP comandă | MORE (pentru afișare paginată).
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

NET HELP имя_команды | MORE - постраничный просмотр справки.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
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

NET HELP comandă | MORE (pentru afișare paginată).
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

NET HELP имя_команды | MORE - постраничный просмотр справки.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
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

NET HELP comandă | MORE (pentru afișare paginată).
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

NET HELP имя_команды | MORE - постраничный просмотр справки.
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

NET HELP command | MORE displays one screen at a time.
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
NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
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

NET HELP command | MORE displays one screen at a time.
.


MessageId=10010
SymbolicName=MSG_FILE_SYNTAX
Severity=Success
Facility=System
Language=English
NET FILE [id [/CLOSE]]
.
Language=Polish
NET FILE [identyfikator [/CLOSE]]
.
Language=Romanian
NET FILE [id [/CLOSE]]
.
Language=Russian
NET FILE [id [/CLOSE]]
.
Language=Spanish
NET FILE [id [/CLOSE]]
.
Language=Turkish
NET FILE [id [/CLOSE]]
.
Language=Chinese
NET FILE [id [/CLOSE]]
.
Language=Taiwanese
NET FILE [id [/CLOSE]]
.


MessageId=10011
SymbolicName=MSG_FILE_HELP
Severity=Success
Facility=System
Language=English
NET FILE
...
.
Language=Polish
NET FILE
...
.
Language=Romanian
NET FILE
...
.
Language=Russian
NET FILE
...
.
Language=Spanish
NET FILE
...
.
Language=Turkish
NET FILE
...
.
Language=Chinese
NET FILE
...
.
Language=Taiwanese
NET FILE
...
.


MessageId=10012
SymbolicName=MSG_GROUP_SYNTAX
Severity=Success
Facility=System
Language=English
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Polish
NET GROUP [nazwa_grupy [/COMMENT:"tekst"]] [/DOMAIN]
          nazwa_grupy {/ADD [/COMMENT:"tekst"] | /DELETE} [/DOMAIN]
          nazwa_grupy nazwa_użytkownika [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Romanian
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Russian
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Spanish
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Turkish
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Chinese
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Taiwanese
NET GROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
          groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
          groupname username [...] {/ADD | /DELETE} [/DOMAIN]
.


MessageId=10013
SymbolicName=MSG_GROUP_HELP
Severity=Success
Facility=System
Language=English
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET GROUP dodaje, wyświetla lub modyfikuje grupy globalne na serwerach.
Użyte bez parametrów wyświetla nazwy grup na serwerze.

nazwa_grupy              Nazwa grupy dodawanej, rozszerzanej lub usuwanej.
                         Aby zobaczyć listę użytkowników w grupie, podaj
                         tylko nazwę grupy.
/COMMENT:"tekst"         Dodaje komentarz dla nowej lub istniejącej grupy.
                         Tekst musi być ujęty w cudzysłów.
/DOMAIN                  Wykonuje operację na kontrolerze domeny.
                         Bez tego przełącznika operacje są wykonywane na
                         komputerze lokalnym.
nazwa_użytkownika[ ...]  Lista zawierająca nazwy jednego lub kilku
                         użytkowników dodawanych lub usuwanych z grupy.
                         Rozdziel kolejne nazwy znakiem spacji.
/ADD                     Dodaje grupę lub dodaje nazwę użytkownika do grupy.
/DELETE                  Usuwa grupę lub usuwa nazwę użytkownika z grupy.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET GROUP adds, displays or modifies global groups on servers. When
used without parameters, it displays the groupnames on the server.

groupname        Is the name of the group to add, expand, or delete.
                 Supply only a groupname to view a list of users
                 in a group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
username[ ...]   List one or more usernames to add to or remove from
                 a group. Separate multiple username entries with a space.
/ADD             Adds a group, or adds a username to a group.
/DELETE          Removes a group, or removes a username from a group.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10014
SymbolicName=MSG_HELP_SYNTAX
Severity=Success
Facility=System
Language=English
NET HELP command
   - or -
NET command /HELP
.
Language=Polish
NET HELP polecenie
   - lub -
NET polecenie /HELP
.
Language=Romanian
NET HELP <comandă>
   - sau -
NET <comandă> /HELP
.
Language=Russian
NET HELP <Команда>
   - или -
NET <Команда> /HELP
.
Language=Spanish
NET HELP <comando>
   - o -
NET <comando> /HELP
.
Language=Turkish
NET HELP <Komut>
   - ya da -
NET <Komut> /HELP
.
Language=Chinese
NET HELP <Command>
   - or -
NET <Command> /HELP
.
Language=Taiwanese
NET HELP <命令>
   - 或 -
NET <命令> /HELP
.


MessageId=10015
SymbolicName=MSG_HELP_HELP
Severity=Success
Facility=System
Language=English
   The following commands are available:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP command | MORE displays Help one screen at a time.
.
Language=Polish
   Dostępne polecenia to:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX wyświetla, jak odczytywać linie składni NET HELP.
   NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
   Sunt disponibile următoarele comenzi:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
   Доступны следующие команды:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
   Éstos son los argumentos de línea de comandos disponibles:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP command | MORE displays Help one screen at a time.
.
Language=Turkish
   Aşağıdaki komutlar kullanılabilir:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP command | MORE displays Help one screen at a time.
.
Language=Chinese
   以下命令可用:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP command | MORE displays Help one screen at a time.
.
Language=Taiwanese
   以下命令是可用的:

   NET ACCOUNTS             NET HELP                 NET STOP
   NET COMPUTER             NET HELPMSG              NET TIME
   NET CONFIG               NET LOCALGROUP           NET USE
   NET CONFIG SERVER        NET PAUSE                NET USER
   NET CONFIG WORKSTATION   NET SESSION              NET VIEW
   NET CONTINUE             NET SHARE
   NET FILE                 NET START
   NET GROUP                NET STATISTCS

   NET HELP SYNTAX explains how to read NET HELP syntax lines.
   NET HELP command | MORE displays Help one screen at a time.
.


MessageId=10016
SymbolicName=MSG_HELPMSG_SYNTAX
Severity=Success
Facility=System
Language=English
NET HELPMSG message#
.
Language=Polish
NET HELPMSG komunikat#
.
Language=Romanian
NET HELPMSG <Error Code>
.
Language=Russian
NET HELPMSG <Код ошибки>
.
Language=Spanish
NET HELPMSG <código de error>
.
Language=Turkish
NET HELPMSG <Yanlışlık Kodu>
.
Language=Chinese
NET HELPMSG message#
.
Language=Taiwanese
NET HELPMSG <錯誤程式碼>
.


MessageId=10017
SymbolicName=MSG_HELPMSG_HELP
Severity=Success
Facility=System
Language=English
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Polish
NET HELPMSG wyświetla informacje o komunikatach sieci (takich
jak błąd, ostrzeżenie lub alarm). Gdy wpiszesz NET HELPMSG i błąd numeryczny
(na przykład "net helpmsg 2182"), system
objaśni komunikat i zasugeruje rozwiązanie problemu.

komunikat#   Numeryczny błąd systemu, o którym chcesz uzyskać informacje.
.
Language=Romanian
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Russian
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Spanish
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Turkish
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Chinese
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.
Language=Taiwanese
NET HELPMSG displays information about network messages (such as
error, warning and alert messages). When you type NET HELPMSG and the numeric
error (for example, "net helpmsg 2182"), you will get information about the
message and suggested actions you can take to solve the problem.

message#   Is the numerical error with which you need help.
.


MessageId=10018
SymbolicName=MSG_LOCALGROUP_SYNTAX
Severity=Success
Facility=System
Language=English
NET LOCALGROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
               groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
               groupname name [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Polish
NET LOCALGROUP [nazwa_grupy [/COMMENT:"tekst"]] [/DOMAIN]
               nazwa_grupy {/ADD [/COMMENT:"tekst"] | /DELETE} [/DOMAIN]
               nazwa_grupy nazwa [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Romanian
NET LOCALGROUP [nume-de-grup [/COMMENT:"text"]] [/DOMAIN]
               nume-de-grup {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
               nume-de-grup nume [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Russian
NET LOCALGROUP [имя_группы [/COMMENT:"текст"]] [/DOMAIN]
               имя_группы {/ADD [/COMMENT:"текст"] | /DELETE}  [/DOMAIN]
               имя_группы имя [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Spanish
NET LOCALGROUP [grupo [/COMMENT:"texto"]] [/DOMAIN]
               grupo {/ADD [/COMMENT:"texto"] | /DELETE} [/DOMAIN]
               grupo nombre [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Turkish
NET LOCALGROUP [takım adı [/COMMENT:"metin"]] [/DOMAIN]
               takım adı {/ADD [/COMMENT:"metin"] | /DELETE} [/DOMAIN]
               takım adı ad [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Chinese
NET LOCALGROUP [groupname [/COMMENT:"text"]] [/DOMAIN]
               groupname {/ADD [/COMMENT:"text"] | /DELETE} [/DOMAIN]
               groupname name [...] {/ADD | /DELETE} [/DOMAIN]
.
Language=Taiwanese
NET LOCALGROUP [組名 [/COMMENT:"文字"]] [/DOMAIN]
               組名 {/ADD [/COMMENT:"文字"] | /DELETE} [/DOMAIN]
               組名稱 [...] {/ADD | /DELETE} [/DOMAIN]
.


MessageId=10019
SymbolicName=MSG_LOCALGROUP_HELP
Severity=Success
Facility=System
Language=English
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET LOCALGROUP dodaje, wyświetla, lub modyfikuje grupy lokalne na komputerach
Polecenie użyte bez parametrów wyświetla grupy lokalne na komputerze.

nazwa_grupy       Nazwa grupy dodawanej, rozszerzanej lub usuwanej.
                  Aby zobaczyć listę użytkowników lub grup globalnych
                  w grupie lokalnej, podaj tylko nazwę grupy.
/COMMENT:"tekst"  Dodaje komentarz dla nowej lub istniejącej grupy.
                  Tekst musi być ujęty w cudzysłów.
/DOMAIN           Wykonuje operację na kontrolerze bieżącej domeny.
                  Bez tego przełącznika operacje są wykonywane na
                  komputerze lokalnym.
nazwa[ ...]       Lista zawierająca nazwy jednego lub kilku użytkowników
                  lub grup, dodawanych lub usuwanych z grupy lokalnej.
                  Rozdziel kolejne wpisy znakiem spacji. Lista może zawierać
                  nazwy użytkowników lub grup globalnych, lecz nie może
                  zawierać nazw innych grup lokalnych. Podając nazwę
                  użytkownika z innej domeny poprzedź ją nazwą domeny
                  (na przykład: WARSZAWA\PIOTRS).
/ADD              Dodaje nazwę grupy lub użytkownika do grupy lokalnej.
                  Dla użytkowników lub grup globalnych dodawanych tym
                  poleceniem do grupy lokalnej należy wcześniej utworzyć
                  odpowiednie konto.
/DELETE           Usuwa nazwę grupy lub użytkownika z grupy lokalnej.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name [ ...]      List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET LOCALGROUP adds, displays or modifies local groups on computers. When
used without parameters, it displays the local groups on the computer.

groupname        Is the name of the local group to add, expand, or
                 delete. Supply only a groupname to view a list of
                 users or global groups in a local group.
/COMMENT:"text"  Adds a comment for a new or existing group.
                 Enclose the text in quotation marks.
/DOMAIN          Performs the operation on a domain controller
                 of the current domain. Otherwise, the operation is
                 performed on the local computer.
name[ ...]       List one or more usernames or groupnams to add to or
                 remove from a local group. Separate multiple entries with
                 a space. Names may be users or global groups, but not
                 other local groups. If a user is from another doamin,
                 preface the username with the domain name (for
                 example, SALES\RALPHR).
/ADD             Adds a groupname or username to a local group. An account
                 must be established for users or global groups added to a
                 local group with this command.
/DELETE          Removes a groupname or username from a local group.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10020
SymbolicName=MSG_PAUSE_SYNTAX
Severity=Success
Facility=System
Language=English
NET PAUSE service
.
Language=Polish
NET PAUSE usługa
.
Language=Romanian
NET PAUSE <nume serviciu>
.
Language=Russian
NET PAUSE <имя_службы>
.
Language=Spanish
NET PAUSE <nombre del servicio>
.
Language=Turkish
NET PAUSE service
.
Language=Chinese
NET PAUSE service
.
Language=Taiwanese
NET PAUSE <服務名稱>
.


MessageId=10021
SymbolicName=MSG_PAUSE_HELP
Severity=Success
Facility=System
Language=English
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET PAUSE wstrzymuje usługę.

usługa   Nazwa wstrzymywanej usługi.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET PAUSE suspends a service. Pausing a service puts it on hold.
service   The name of the service to be paused.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET PAUSE suspends a service. Pausing a service puts it on hold.

service   The name of the service to be paused.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10022
SymbolicName=MSG_SESSION_SYNTAX
Severity=Success
Facility=System
Language=English
NET SESSION ...
.
Language=Polish
NET SESSION ...
.
Language=Romanian
NET SESSION ...
.
Language=Russian
NET SESSION ...
.
Language=Spanish
NET SESSION ...
.
Language=Turkish
NET SESSION ...
.
Language=Chinese
NET SESSION ...
.
Language=Taiwanese
NET SESSION ...
.


MessageId=10023
SymbolicName=MSG_SESSION_HELP
Severity=Success
Facility=System
Language=English
SESSION
...
.
Language=Polish
SESSION
...
.
Language=Romanian
SESSION
...
.
Language=Russian
SESSION
...
.
Language=Spanish
SESSION
...
.
Language=Turkish
SESSION
...
.
Language=Chinese
SESSION
...
.
Language=Taiwanese
SESSION
...
.


MessageId=10024
SymbolicName=MSG_SHARE_SYNTAX
Severity=Success
Facility=System
Language=English
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Polish
NET SHARE nazwa_udziału=dysk:ścieżka [/GRANT:użytkownik,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"tekst"]
                               [/CACHE:Manual | Documents| Programs | None]
          nazwa_udziału [/USERS:liczba | /UNLIMITED]
                    [/REMARK:"tekst"]
                    [/CACHE:Manual | Documents | Programs | None]
          {nazwa_udziału | nazwa_urządzenia | dysk:ścieżka} /DELETE
.
Language=Romanian
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Russian
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Spanish
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Turkish
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Chinese
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.
Language=Taiwanese
NET SHARE sharename=drive:path [/GRANT:user,[READ | CHANGE | FULL]]
                               [/USERS:number | /UNLIMITED]
                               [/REMARK:"text"]
                               [/CACHE:Manual | Documents| Programs | None]
          sharename [/USERS:number | /UNLIMITED]
                    [/REMARK:"text"]
                    [/CACHE:Manual | Documents | Programs | None]
          {sharename | devicename | drive:path} /DELETE
.


MessageId=10025
SymbolicName=MSG_SHARE_HELP
Severity=Success
Facility=System
Language=English
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Polish
NET SHARE udostępnia zasoby serwera użytkownikom sieci. Użyte
bez opcji wyświetla informacje o wszystkich zasobach udostępnionych
na komputerze. Dla każdego zasobu, system ReactOS zgłasza
nazwa_urządzenia lub nazwa_ścieżki i powiązany z nim komentarz opisowy.

nazwa_udziału
  Określa nazwę sieciową udostępnionego zasobu. Aby wyświetlić informacje dotyczące tylko danego zasobu, należy wpisać polecenie net share z parametrem.

dysk:ścieżka
  Określa ścieżkę absolutną katalogu do udostępnienia.

/GRANT:user,perm
  Tworzy udział z deskryptorem zabezpieczeń, który nadaje wymagane uprawnienia określonemu użytkownikowi.
  Ta opcja może być używana więcej niż raz w celu nadania uprawnień udziału wielu użytkownikom

/USERS:liczba_użytkowników
  Ustawia maksymalną liczbę użytkowników, którzy mogą jednocześnie korzystać z udostępnionego zasobu.

/UNLIMITED
  Określa nieograniczoną liczbę użytkowników, którzy mogą jednocześnie korzystać z udostępnionego zasobu.

/REMARK:"tekst"
  Dodaje opisowy komentarz dotyczący zasobu. Tekst należy wpisać w cudzysłowie.

nazwa_urządzenia
  Jest jedną lub kilkoma drukarkami (LPT1: do LPT9:).

/DELETE
  Zatrzymuje udostępnianie danego zasobu.

/CACHE:Manual
  Włącza buforowanie klientów w trybie offline z ręczną ponowną integracją.

/CACHE:Documents
  Włącza automatyczne buforowanie dokumentów z tego udziału.

/CACHE:Programs
  Włącza automatyczne buforowanie klientów programów.

/CACHE:None
  Wyłącza buforowanie.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Romanian
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Russian
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Spanish
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Turkish
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Chinese
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Taiwanese
NET SHARE makes a server's resources available to network users. When
used without options, it lists information about all resources being
shared on the computer. For each resource, Windows reports the
devicename(s) or pathname(s) and a descriptive comment associated with it.

sharename          Is the network name of the shared resource. Type
                   NET SHARE with a sharename only to display information
                   about that share.
drive:path         Specifies the absolute path of the directory to
                   be shared.
/GRANT:user,perm   Creates the share with a security descriptor that gives
                   the requested permissions to the specified user. This
                   option may be used more than once to give share permissions
                   to multiple users.
/USERS:number      Sets the maximum number of users who can
                   simultaneously access the shared resource.
/UNLIMITED         Specifies an unlimited number of users can
                   simultaneously access the shared resource.
/REMARK:"text"     Adds a descriptive comment about the resource.
                   Enclose the text in quotation marks.
devicename         Is one or more printers (LPT1: through LPT9:)
                   shared by sharename.
/DELETE            Stops sharing the resource.
/CACHE:Manual      Enables manual client caching of programs and documents
                   from this share.
/CACHE:Documents   Enables automatic caching of documents from this share.
/CACHE:Programs    Enables automatic caching of documents and programs
                   from this share.
/CACHE:None        Disables caching from this share.

NET HELP command | MORE displays Help one screen at a time.
.


MessageId=10026
SymbolicName=MSG_START_SYNTAX
Severity=Success
Facility=System
Language=English
NET START [service]
.
Language=Polish
NET START [usługa]
.
Language=Romanian
NET START [service]
.
Language=Russian
NET START [service]
.
Language=Spanish
NET START [service]
.
Language=Turkish
NET START [service]
.
Language=Chinese
NET START [service]
.
Language=Taiwanese
NET START [service]
.


MessageId=10027
SymbolicName=MSG_START_HELP
Severity=Success
Facility=System
Language=English
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET START uruchamia usługi lub wyświetla uruchomione usługi.

usługa   Nazwa uruchamianej usługi.

Nazwy usług mające dwa lub więcej słów wpisywane w.
wierszu polecenia muszą być ujęte w cudzysłów. Przykładowo,
NET START "NET LOGON" uruchamia usługę logowania w sieci.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET START starts services or lists running services.

service   The name of the service to be started.

When typed at the command prompt, service name of two words or more must
be enclosed in quotation marks. For example, NET START "NET LOGON"
starts the net logon service.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10028
SymbolicName=MSG_STATISTICS_SYNTAX
Severity=Success
Facility=System
Language=English
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Polish
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Romanian
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Russian
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Spanish
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Turkish
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Chinese
NET STATISTICS [SERVER | WORKSTATION]
.
Language=Taiwanese
NET STATISTICS [SERVER | WORKSTATION]
.


MessageId=10029
SymbolicName=MSG_STATISTICS_HELP
Severity=Success
Facility=System
Language=English
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET STATISTICS wyświetla dziennik statystyki dla lokalnej usługi
Stacja robocza lub Serwer. Użyte bez parametrów, NET STATISTICS wyświetla
usługi dla których statystyka jest dostępna.

SERVER        Wyświetla statystykę usługi Serwer.
WORKSTATION   Wyświetla statystykę usługi Stacja robocza.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET STATISTICS displays the statisticslog for the local Workstation or
Server service. Used without parameters, NET STATISTICS displays
the services for which statistics are available.

SERVER        Displays the Server service statistics.
WORKSTATION   Displays the Workstation service statistics.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10030
SymbolicName=MSG_STOP_SYNTAX
Severity=Success
Facility=System
Language=English
NET STOP service
.
Language=Polish
NET STOP usługa
.
Language=Romanian
NET STOP <nume serviciu>
.
Language=Russian
NET STOP <имя_службы>
.
Language=Spanish
NET STOP <nombre del servicio>
.
Language=Turkish
NET STOP <Hizmet Adı>
.
Language=Chinese
NET STOP service
.
Language=Taiwanese
NET STOP <服務名稱>
.


MessageId=10031
SymbolicName=MSG_STOP_HELP
Severity=Success
Facility=System
Language=English
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP command | MORE displays one screen at a time.
.
Language=Polish
NET STOP zatrzymuje usługi.

usługa   Nazwa zatrzymywanej usługi.

Zatrzymanie usługi rozłącza wszystkie połączenia sieciowe
używane przez usługę. Niektóre usługi zależą od innych. Zatrzymanie jednej
może też zatrzymać inne. Niektóre usługi nie mogą być zatrzymane.

NET HELP polecenie | MORE wyświetla informacje na jednym ekranie na raz.
.
Language=Romanian
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP comandă | MORE (pentru afișare paginată).
.
Language=Russian
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP имя_команды | MORE - постраничный просмотр справки.
.
Language=Spanish
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP command | MORE displays one screen at a time.
.
Language=Turkish
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP command | MORE displays one screen at a time.
.
Language=Chinese
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP command | MORE displays one screen at a time.
.
Language=Taiwanese
NET STOP stops services.

service   The name of the service to be stopped.

Stopping a service cancels any network connection the service is
using. Also, some services are dependent on others. Stopping one
service can stop others. Some services cannot be stopped.

NET HELP command | MORE displays one screen at a time.
.


MessageId=10032
SymbolicName=MSG_TIME_SYNTAX
Severity=Success
Facility=System
Language=English
NET TIME ...
.
Language=Polish
NET TIME ...
.
Language=Romanian
NET TIME ...
.
Language=Russian
NET TIME ...
.
Language=Spanish
NET TIME ...
.
Language=Turkish
NET TIME ...
.
Language=Chinese
NET TIME ...
.
Language=Taiwanese
NET TIME ...
.


MessageId=10033
SymbolicName=MSG_TIME_HELP
Severity=Success
Facility=System
Language=English
TIME
...
.
Language=Polish
TIME
...
.
Language=Romanian
TIME
...
.
Language=Russian
TIME
...
.
Language=Spanish
TIME
...
.
Language=Turkish
TIME
...
.
Language=Chinese
TIME
...
.
Language=Taiwanese
TIME
...
.


MessageId=10034
SymbolicName=MSG_USE_SYNTAX
Severity=Success
Facility=System
Language=English
NET USE ...
.
Language=Polish
NET USE ...
.
Language=Romanian
NET USE ...
.
Language=Russian
NET USE ...
.
Language=Spanish
NET USE ...
.
Language=Turkish
NET USE ...
.
Language=Chinese
NET USE ...
.
Language=Taiwanese
NET USE ...
.


MessageId=10035
SymbolicName=MSG_USE_HELP
Severity=Success
Facility=System
Language=English
USE
...
.
Language=Polish
USE
...
.
Language=Romanian
USE
...
.
Language=Russian
USE
...
.
Language=Spanish
USE
...
.
Language=Turkish
USE
...
.
Language=Chinese
USE
...
.
Language=Taiwanese
USE
...
.


MessageId=10036
SymbolicName=MSG_USER_SYNTAX
Severity=Success
Facility=System
Language=English
NET USER [username [password | *] [options]] [/DOMAIN]
         username {password | *} /ADD [options] [/DOMAIN]
         username [/DELETE] [/DOMAIN]
.
Language=Polish
NET USER [nazwa_użytkownika [hasło | *] [opcje]] [/DOMAIN]
         nazwa_użytkownika {hasło | *} /ADD [opcje] [/DOMAIN]
         nazwa_użytkownika [/DELETE] [/DOMAIN]
.
Language=Romanian
NET USER [nume-utilizator [parolă | *] [opțiuni]] [/DOMAIN]
         nume-utilizator {parolă | *} /ADD [opțiuni] [/DOMAIN]
         nume-utilizator [/DELETE] [/DOMAIN]
.
Language=Russian
NET USER [имя_пользователя [пароль | *] [параметры]] [/DOMAIN]
         имя_пользователя {пароль | *} /ADD [параметры] [/DOMAIN]
         имя_пользователя [/DELETE] [/DOMAIN]
.
Language=Spanish
NET USER [usuario [contraseña | *] [opciones]] [/DOMAIN]
         usuario {contraseña | *} /ADD [opciones] [/DOMAIN]
         usuario [/DELETE] [/DOMAIN]
.
Language=Turkish
NET USER [kullanıcı adı [şifre | *] [seçenekler]] [/DOMAIN]
         kullanıcı adı {şifre | *} /ADD [seçenekler] [/DOMAIN]
         kullanıcı adı [/DELETE] [/DOMAIN]
.
Language=Chinese
NET USER [username [password | *] [options]] [/DOMAIN]
         username {password | *} /ADD [options] [/DOMAIN]
         username [/DELETE] [/DOMAIN]
.
Language=Taiwanese
NET USER [使用者名 [密碼 | *] [選項]] [/DOMAIN]
         使用者名 {密碼 | *} /ADD [選項] [/DOMAIN]
         使用者名 [/DELETE] [/DOMAIN]
.


MessageId=10037
SymbolicName=MSG_USER_HELP
Severity=Success
Facility=System
Language=English
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Polish
NET USER tworzy i modyfikuje konta użytkowników na komputerach. Użyte bez
przełączników wyświetla listę kont użytkowników na komputerze. Informacja
o kontach użytkowników przechowywana jest w bazie danych kont użytkowników.

nazwa_użytkownika
  Nazwa konta użytkownika do dodania, usunięcia, modyfikacji
  lub wyświetlenia. Nazwa konta użytkownika może składać się
  maksymalnie z 20 znaków.

hasło
  Przypisuje lub zmienia hasło dla konta użytkownika.
  Hasło musi spełnić warunek minimalnej długości określony
  opcją /MINPWLEN polecenia NET ACCOUNTS. Może ono się
  składać z maksymalnie 14 znaków.

*
  Wyświetla monit o hasło. Podczas wpisywania hasła.


/DOMAIN
  Wykonuje te operacje na kontrolerze bieżącej domeny.

/ADD
  Dodaje konto użytkownika do bazy danych użytkowników.

/DELETE
  Usuwa konto użytkownika z bazy danych użytkowników.

Opcje      Polecenie posiada następujące opcje:

   Opcje                      Opis
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Aktywuje lub dezaktywuje konto. Jeśli konto
                              nie jest aktywne, użytkownik nie ma dostępu do
                              serwera. Wartością domyślną jest YES.

   /COMMENT:"tekst"           Opis konta użytkownika.
                              Tekst musi być ujęty w cudzysłów.

   /COUNTRYCODE:nnn           Określa użycie kodu kraju w celu zastosowania
                              specyficznych dla języka plików pomocy
                              użytkownika i komunikatów o błędach. Wartość
                              0 określa domyślny kod kraju.

   /EXPIRES:{date | NEVER}    Powoduje wygaśnięcie konta, jeśli data jest
                              ustawiona. Wartość NEVER określa brak limitu
                              czasu dla konta. Data wygaśnięcia konta może
                              być podana w formacie mm/dd/rr(rr). Miesiące
                              mogą być podane jako liczby, pełne nazwy,
                              lub nazwy skrócone składające się z trzech
                              liter. Rok może być podany jako dwie lub
                              cztery cyfry. Użyj ukośników (/) (a nie
                              spacji) do oddzielenia składników daty.

   /FULLNAME:"nazwisko"       Pełne imię i nazwisko użytkownika
                              (a nie tylko nazwa użytkownika). Wpisz imię i
                              nazwisko w cudzysłowie.

   /HOMEDIR:ścieżka           Ustawia ścieżkę do katalogu macierzystego.
                              Ścieżka musi istnieć.

   /PASSWORDCHG:{YES | NO}    Określa, czy użytkownik może zmienić własne
                              hasło. Wartością domyślną jest YES.

   /PASSWORDREQ:{YES | NO}    Określa czy konto użytkownika musi posiadać
                              hasło. Wartością domyślną jest YES.

   /PROFILEPATH[:ścieżka]     Ustawia ścieżkę dla profilu logowania
                              użytkownika.

   /SCRIPTPATH:ścieżka        Określa lokalizację skryptu logowania dla
                              użytkownika.

   /TIMES:{czas | ALL}        Określa godziny logowania. Parametr TIMES jest
                              wyrażony jako: dzień[-dzień][,dzień[-dzień]],
                              czas[-czas],[,czas[-czas]], ograniczony jest do
                              godzinnych przyrostów. Wpisz pełne lub skrócone
                              nazwy dni tygodnia. Czas może być opisany
                              w notacji 12- lub 24-godzinnej. Dla notacji
                              12-godzinnej, użyj symbolu: am, pm, a.m. lub
                              p.m. Parametr ALL oznacza, że użytkownik zawsze
                              może się logować, brak wartości oznacza, że
                              użytkownik nigdy nie może się logować.
                              Oddziel wpisy dnia i godziny przecinkiem;
                              wielokrotne wpisy dnia i godziny oddziel
                              średnikiem.

   /USERCOMMENT:"tekst"       Pozwala administratorom na dodanie lub zmianę
                              komentarza dla konta użytkownika.

   /WORKSTATIONS:{nazwa_komputera[,...] | *}
                              Lista maksymalnie ośmiu komputerów, z których
                              użytkownik może zalogować się do sieci. Jeśli
                              parametr /WORKSTATIONS nie posiada listy lub
                              jest on równy *, użytkownik może zalogować się
                              z dowolnego komputera.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Romanian
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Russian
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Spanish
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Turkish
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Chinese
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.
Language=Taiwanese
NET USER creates and modifies user accounts on computers. When used
without switches, it lists the user accounts for the computer. The
user account information is stored in the user accounts database.

username     Is the name of the user account to add, delete, modify, or
             view. The name of the user account can have as many as
             20 characters.
password     Assigns or changes a password for the user's account.
             A password must satisfy the minimum length set with the
             /MINPWLEN option of the NET ACCOUNTS command. It can have as
             many as 14 characters.
*            Produces a prompt for the password. The password is not
             displayed when you type it at a password prompt.
/DOMAIN      Performs the operation on a domain controller of
             the current domain.
/ADD         Adds a user account to the user accounts database.
/DELETE      Removes a user account from the user accounts database.

Options      Are as follows:

   Options                    Description
   --------------------------------------------------------------------
   /ACTIVE:{YES | NO}         Activates or deactivates the account. If
                              the account is not active, the user cannot
                              access the server. The default is YES.
   /COMMENT:"text"            Provides a descriptive comment about the
                              user's account.  Enclose the text in
                              quotation marks.
   /COUNTRYCODE:nnn           Uses the operating system country code to
                              implement the specified language files for a
                              user's help and error messages. A value of
                              0 signifies the default country code.
   /EXPIRES:{date | NEVER}    Causes the account to expire if date is
                              set. NEVER sets no time limit on the
                              account. An expiration date is in the
                              form mm/dd/yy(yy). Months can be a number,
                              spelled out, or abbreviated with three
                              letters. Year can be two or four numbers.
                              Use slashes(/) (no spaces) to separate
                              parts of the date.
   /FULLNAME:"name"           Is a user's full name (rather than a
                              username). Enclose the name in quotation
                              marks.
   /HOMEDIR:pathname          Sets the path for the user's home directory.
                              The path must exist.
   /PASSWORDCHG:{YES | NO}    Specifies whether users can change their
                              own password. The default is YES.
   /PASSWORDREQ:{YES | NO}    Specifies whether a user account must have
                              a password. The default is YES.
   /PROFILEPATH[:path]        Sets a path for the user's logon profile.
   /SCRIPTPATH:pathname       Is the location of the user's logon
                              script.
   /TIMES:{times | ALL}       Is the logon hours. TIMES is expressed as
                              day[-day][,day[-day]],time[-time][,time
                              [-time]], limited to 1-hour increments.
                              Days can be spelled out or abbreviated.
                              Hours can be 12- or 24-hour notation. For
                              12-hour notation, use am, pm, a.m., or
                              p.m. ALL means a user can always log on,
                              and a blank value means a user can never
                              log on. Separate day and time entries with
                              a comma, and separate multiple day and time
                              entries with a semicolon.
   /USERCOMMENT:"text"        Lets an administrator add or change the User
                              Comment for the account.
   /WORKSTATIONS:{computername[,...] | *}
                              Lists as many as eight computers from
                              which a user can log on to the network. If
                              /WORKSTATIONS has no list or if the list is *,
                              the user can log on from any computer.

NET HELP command | MORE displays Help one screen at a time.
.


MessageId=10038
SymbolicName=MSG_VIEW_SYNTAX
Severity=Success
Facility=System
Language=English
NET VIEW ...
.
Language=Polish
NET VIEW ...
.
Language=Romanian
NET VIEW ...
.
Language=Russian
NET VIEW ...
.
Language=Spanish
NET VIEW ...
.
Language=Turkish
NET VIEW ...
.
Language=Chinese
NET VIEW ...
.
Language=Taiwanese
NET VIEW ...
.


MessageId=10039
SymbolicName=MSG_VIEW_HELP
Severity=Success
Facility=System
Language=English
VIEW
...
.
Language=Polish
VIEW
...
.
Language=Romanian
VIEW
...
.
Language=Russian
VIEW
...
.
Language=Spanish
VIEW
...
.
Language=Turkish
VIEW
...
.
Language=Chinese
VIEW
...
.
Language=Taiwanese
VIEW
...
.


MessageId=10040
SymbolicName=MSG_NET_SYNTAX
Severity=Success
Facility=System
Language=English
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Polish
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Romanian
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Russian
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Spanish
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Turkish
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Chinese
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.
Language=Taiwanese
NET [ ACCOUNTS | COMPUTER | CONFIG | CONTINUE | FILE | GROUP | HELP |
      HELPMSG | LOCALGROUP | PAUSE | SESSION | SHARE | START |
      STATISTICS | STOP | TIME | USE | USER | VIEW ]
.


MessageId=10041
SymbolicName=MSG_SYNTAX_HELP
Severity=Success
Facility=System
Language=English
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Polish
SYNTAX
Do opisu składni poleceń użyto następujących zasad:

-  Wielkich liter użyto do przedstawienia słów kluczowych, które trzeba
   wpisać tak jak pokazano. Małych liter użyto do oznaczenia nazw elementów,
   które są zmienne, np. nazw plików.

-  Elementy opcjonalnie występujące w poleceniu zawarto w znakach [ i ].

-  Listy elementów zawarto w znakach { i }, które oznaczają, że
   w poleceniu musi zostać zastosowany jeden z elementów listy.

-  Znak | rozdziela elementy listy. Oznacza on, że w poleceniu może
   zostać użyty tylko jeden z rozdzielanych elementów.

   Na przykład, poniższy zapis oznacza, że należy wpisać NET POLECENIE
   oraz jeden z przełączników: PRZEŁĄCZNIK1 lub PRZEŁĄCZNIK2. Użycie
   parametru nazwa jest opcjonalne.
   NET POLECENIE [nazwa] {PRZEŁĄCZNIK1 | PRZEŁĄCZNIK2}

-  Znaki [...] oznaczają, że dozwolone jest powtarzanie poprzedniego
  elementu. Powtarzane elementy należy rozdzielić spacjami.

-  Znaki [,...] oznaczają, że dozwolone jest powtarzanie poprzedniego
   elementu, lecz elementy muszą być rozdzielone przecinkami lub średnikami,
   a nie spacjami.

-  Wpisywana w wierszu polecenia nazwa usługi składająca się z dwóch
   lub więcej wyrazów musi być ujęta w cudzysłów. Na przykład,
   NET START "COMPUTER BROWSER" uruchamia usługę przeglądarki
   komputera (computer browser).
.
Language=Romanian
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Russian
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Spanish
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Turkish
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Chinese
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
Language=Taiwanese
SYNTAX
The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NET COMMAND and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NET COMMAND [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, service names of two words or
   more must be enclosed in quotation marks. For example,
   NET START "COMPUTER BROWSER" starts the computer browser service.
.
