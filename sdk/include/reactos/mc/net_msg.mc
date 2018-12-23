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
               Romanian=0x018:MSG00018
               Russian=0x419:MSG00419
               Spanish=0x00A:MSG0000A
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
