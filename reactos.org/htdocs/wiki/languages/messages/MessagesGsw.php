<?php
/** Swiss German (Alemannisch)
 *
 * @ingroup Language
 * @file
 *
 * @author Hendergassler
 * @author J. 'mach' wust
 * @author MichaelFrey
 * @author Spacebirdy
 * @author לערי ריינהארט
 * @author 80686
 */

$fallback = 'de';
$linkTrail = '/^([äöüßa-z]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Links unterstryche',
'tog-highlightbroken'         => 'Links uf lääri Themene durestryche',
'tog-justify'                 => 'Tekscht als Blocksatz',
'tog-hideminor'               => 'Cheini «chlyni Änderige» aazeige',
'tog-extendwatchlist'         => 'Erwiterti Beobachtungslischte',
'tog-usenewrc'                => 'Erwytereti «letschti Änderige» (geit nid uf allne Browser)',
'tog-numberheadings'          => 'Überschrifte outomatisch numeriere',
'tog-showtoolbar'             => 'Editier-Wärchzüüg aazeige',
'tog-editondblclick'          => 'Syte ändere mit Doppelklick i d Syte (JavaScript)',
'tog-editsection'             => 'Gleicher aazeige für ds Bearbeite vo einzelnen Absätz',
'tog-editsectiononrightclick' => 'Einzelni Absätz ändere mit Rächtsclick (Javascript)',
'tog-showtoc'                 => 'Inhaltsverzeichnis aazeige bi Artikle mit meh als drei Überschrifte',
'tog-rememberpassword'        => 'Passwort spychere (Cookie)',
'tog-editwidth'               => 'Tekschtygabfäld mit voller Breiti',
'tog-watchcreations'          => 'Sälbr gmachti Sytene beobachte',
'tog-watchdefault'            => 'Vo dir nöi gmachti oder verändereti Syte beobachte',
'tog-watchmoves'              => 'Sälbr vrschobeni Sytene beobachte',
'tog-watchdeletion'           => 'Sälbr glöschti Sytene beobachte',
'tog-minordefault'            => 'Alli dyni Änderigen als «chlyni Änderige» markiere',
'tog-previewontop'            => 'Vorschou vor em Editierfänschter aazeige',
'tog-previewonfirst'          => 'Vorschou aazeige bim erschten Editiere',
'tog-nocache'                 => 'Syte-Cache deaktiviere',
'tog-enotifwatchlistpages'    => 'Benachrichtigungsmails by Änderigen a Wiki-Syte',
'tog-enotifusertalkpages'     => 'Benachrichtigungsmails bi Änderigen a dyne Benutzersyte',
'tog-enotifminoredits'        => 'Benachrichtigungsmail ou bi chlyne Sytenänderige',
'tog-enotifrevealaddr'        => 'Dyni E-Mail-Adrässe wird i Benachrichtigungsmails zeigt',
'tog-shownumberswatching'     => 'Aazahl Benutzer aazeige, wo ne Syten am Aaluege sy (i den Artikelsyte, i de «letschten Änderigen» und i der Beobachtigslischte)',
'tog-fancysig'                => 'Kei outomatischi Verlinkig vor Signatur uf d Benutzersyte',
'tog-externaleditor'          => 'Externen Editor als default',
'tog-externaldiff'            => 'Externi diff als default',
'tog-showjumplinks'           => '«Wächsle-zu»-Links ermügleche',
'tog-uselivepreview'          => 'Live preview benütze (JavaScript) (experimentell)',
'tog-forceeditsummary'        => 'Sei miers, wänn I s Zommefassungsfeld leer los',
'tog-watchlisthideown'        => 'Eigeni Änderige uf d Beobachtungslischt usblende',
'tog-watchlisthidebots'       => 'Bot-Änderige in d Beobachtungslischt usblende',
'tog-watchlisthideminor'      => 'Chlyni Änderige nit in de Beobachtigslischte aazeige',
'tog-nolangconversion'        => 'Konvertierig vu Sprachvariante abschalte',
'tog-ccmeonemails'            => "Schick mr Kopie vo de Boscht wo n'ich andere schicke due.",
'tog-diffonly'                => "Numme Versionunterschied aazeige, ohni d'Syte",
'tog-showhiddencats'          => 'Zeig fersteckdi Kategoria',

'underline-always'  => 'immer',
'underline-never'   => 'nie',
'underline-default' => 'Browser-Vorystellig',

'skinpreview' => '(Vorschou)',

# Dates
'sunday'        => 'Sundi',
'monday'        => 'Mändi',
'tuesday'       => 'Zischdi',
'wednesday'     => 'Mittwuch',
'thursday'      => 'Durschdi',
'friday'        => 'Fridi',
'saturday'      => 'Somschdi',
'sun'           => 'Sun',
'mon'           => 'Män',
'tue'           => 'Zys',
'wed'           => 'Mid',
'thu'           => 'Don',
'fri'           => 'Fry',
'sat'           => 'Sam',
'january'       => 'Jänner',
'february'      => 'Februar',
'march'         => 'März',
'april'         => 'Avrel',
'may_long'      => 'Mai',
'june'          => 'Jüni',
'july'          => 'Jüli',
'august'        => 'Ougschte',
'september'     => 'Septämber',
'october'       => "Oktow'r",
'november'      => 'Novämber',
'december'      => 'Dezämber',
'january-gen'   => 'Januar',
'february-gen'  => 'Februar',
'march-gen'     => 'März',
'april-gen'     => 'Avrel',
'may-gen'       => 'Mai',
'june-gen'      => 'Jüni',
'july-gen'      => 'Jüli',
'august-gen'    => 'Oïgscht',
'september-gen' => "Sepdamb'r",
'october-gen'   => "Okdow'r",
'november-gen'  => "Nowamb'r",
'december-gen'  => "Dezamb'r",
'jan'           => 'Jan.',
'feb'           => 'Feb.',
'mar'           => 'Mär.',
'apr'           => 'Apr.',
'may'           => 'Mei',
'jun'           => 'Jün.',
'jul'           => 'Jül.',
'aug'           => 'Oïg.',
'sep'           => 'Sep.',
'oct'           => 'Okt.',
'nov'           => 'Now.',
'dec'           => 'Dez.',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|Kategori|Kategorie}}',
'category_header'          => 'Artikel in de Kategori "$1"',
'subcategories'            => 'Unterkategorie',
'category-media-header'    => "Informationsàplàg in d'r Kategori „$1“",
'category-empty'           => "''Dia Kategori hät zorzyt ke Syda oder informationsàplàg''
''Diese Kategorie enthält zur Zeit keine Seiten oder Medien.''",
'hidden-category-category' => 'Fersteckdi Kategoria', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => '(Forts.)',

'mainpagetext'      => 'MediaWiki isch erfolgrich inschtalliert worre.',
'mainpagedocfooter' => 'Luege uf d [http://meta.wikimedia.org/wiki/MediaWiki_localisation Dokumentation fier d Onpassung vun de Bnutzeroberflächi] un s [http://meta.wikimedia.org/wiki/Help:Contents Bnutzerhondbuech] fier d Hilf yber d Bnutzung un s Ystelle.',

'about'          => 'Übr',
'article'        => 'Inhàlds syt',
'newwindow'      => '(imene nöie Fänschter)',
'cancel'         => 'Abbräche',
'qbfind'         => 'Finde',
'qbbrowse'       => 'Blättre',
'qbedit'         => 'Ändere',
'qbpageoptions'  => 'Sytenoptione',
'qbpageinfo'     => 'Sytedate',
'qbmyoptions'    => 'Ystellige',
'qbspecialpages' => 'Spezialsytene',
'moredotdotdot'  => 'Meh …',
'mypage'         => 'Minni Syte',
'mytalk'         => 'mini Diskussionsyte',
'anontalk'       => 'Diskussionssyste vo sellere IP',
'navigation'     => 'Nawigation',
'and'            => 'un',

# Metadata in edit box
'metadata_help' => 'Metadàda:',

'errorpagetitle'    => 'Fähler',
'returnto'          => 'Zrügg zur Syte $1.',
'tagline'           => 'Us {{SITENAME}}',
'help'              => 'Hilf',
'search'            => 'Suech',
'searchbutton'      => 'Suech',
'go'                => 'Suara',
'searcharticle'     => 'Suacha',
'history'           => 'Versione',
'history_short'     => 'Versione/Autore',
'updatedmarker'     => "(geändert) sid'r minra ledscht wisit",
'info_short'        => 'Information',
'printableversion'  => 'Druck-Aasicht',
'permalink'         => 'Bschtändigi URL',
'print'             => 'Drucke',
'edit'              => 'ändere',
'create'            => 'Erstela',
'editthispage'      => 'Syte bearbeite',
'create-this-page'  => 'Dia Syt erstela',
'delete'            => 'lösche',
'deletethispage'    => 'Syte lösche',
'undelete_short'    => '{{PLURAL:$1|1 Version|$1 Versione}} widerherstelle',
'protect'           => 'schütze',
'protect_change'    => 'andara',
'protectthispage'   => 'Artikel schütze',
'unprotect'         => 'nümm schütze',
'unprotectthispage' => 'Schutz ufhebe',
'newpage'           => 'Nöji Syte',
'talkpage'          => "Ew'r Dia Syt handala",
'talkpagelinktext'  => 'Diskussion',
'specialpage'       => 'Spezialsyte',
'personaltools'     => 'Persönlichi Wärkzüg',
'postcomment'       => 'Kommentar abgeh',
'articlepage'       => 'Syte',
'talk'              => 'Diskussion',
'views'             => 'Asecht',
'toolbox'           => 'Wärkzügkäschtli',
'userpage'          => 'Benutzersyte',
'projectpage'       => 'Projaktsyt àzaïga',
'imagepage'         => 'Bildsyte',
'viewhelppage'      => 'Helf sah',
'otherlanguages'    => 'Andere Schprôche',
'redirectedfrom'    => '(Witergleitet vun $1)',
'redirectpagesub'   => 'Umgleiteti Syte',
'lastmodifiedat'    => 'Letschti Änderig vo dere Syte: $2, $1<br />', # $1 date, $2 time
'viewcount'         => 'Selli Syte isch {{PLURAL:$1|eimol|$1 Mol}} bsuecht worde.',
'protectedpage'     => 'Gschützt Syte',
'jumpto'            => 'Hops zue:',
'jumptonavigation'  => 'Navigation',
'jumptosearch'      => 'Suech',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Übr {{GRAMMAR:akkusativ|{{SITENAME}}}}',
'aboutpage'            => 'Project:Übr {{UCFIRST:{{GRAMMAR:akkusativ|{{SITENAME}}}}}}',
'bugreports'           => 'Falermaldong',
'bugreportspage'       => 'Project:Kontakt',
'copyright'            => 'Der Inhalt vo dere Syte steht unter der $1.',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Aktuelli Mäldige',
'currentevents-url'    => 'Project:Aktuelli Termin',
'disclaimers'          => 'Impressum',
'disclaimerpage'       => 'Project:Impressum',
'edithelp'             => 'Ratschläg fiers Bearbeite',
'edithelppage'         => 'Help:Ändere',
'faq'                  => 'Filmol Gsteldi Froïa',
'helppage'             => 'Help:Hilf',
'mainpage'             => 'Houptsyte',
'mainpage-description' => 'Houptsyte',
'policy-url'           => 'Project:Leitlinien',
'portal'               => 'Gmeinschaftsportal',
'portal-url'           => 'Project:Gemeinschafts-Portal',
'privacy'              => 'Daateschutz',
'privacypage'          => 'Project:Daateschutz',

'badaccess' => 'Kei usreichendi Rechte.',

'versionrequired'     => 'Version $1 vun MediaWiki wird bnötigt',
'versionrequiredtext' => 'Version $1 vun MediaWiki wird bnötigt um diä Syte zue nutze. Luege [[Special:Version]]',

'retrievedfrom'           => 'Vun "$1"',
'youhavenewmessages'      => 'Du hesch $1 ($2).',
'newmessageslink'         => 'nöji Nachrichte',
'newmessagesdifflink'     => 'Unterschid',
'youhavenewmessagesmulti' => 'Si hen neui Nochrichte: $1',
'editsection'             => 'ändere',
'editold'                 => 'Andara',
'viewsourceold'           => 'Qualltext àzaïga',
'editsectionhint'         => 'Abschnitt ändere: $1',
'toc'                     => 'Inhaltsverzeichnis',
'showtoc'                 => 'ufklappe',
'hidetoc'                 => 'zueklappe',
'thisisdeleted'           => 'Onluege oder widrherstelle vun $1?',
'viewdeleted'             => '$1 onluege?',
'restorelink'             => '{{PLURAL:$1|glöschti Änderig|$1 glöschti Ändrige}}',
'site-rss-feed'           => "RSS-fiad'r fer $1",
'site-atom-feed'          => 'Atom-Feed für $1',
'page-rss-feed'           => 'RSS-Feed für „$1“',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Syt',
'nstab-user'      => 'Benutzersyte',
'nstab-media'     => 'informationsàplàg syt',
'nstab-project'   => 'Projektsyte',
'nstab-image'     => 'Bildli',
'nstab-mediawiki' => 'Nochricht',
'nstab-template'  => 'Vorlag',
'nstab-help'      => 'Hilf',
'nstab-category'  => 'Kategorie',

# Main script and global functions
'nosuchaction'      => 'Di Aktion gibts nit',
'nosuchactiontext'  => 'Di Aktion wird vun de MediaWiki-Software nit unterschtützt',
'nosuchspecialpage' => 'Di Spezialsyte gibts nit',
'nospecialpagetext' => 'Diese Spezialseite wird von der MediaWiki-Software nicht unterstützt',

# General errors
'error'                => 'Fähler',
'databaseerror'        => 'Fähler in dr Datebonk',
'dberrortext'          => 'S het ä Syntaxfähler in dr Datenbonkabfrôg gebä.

D letzscht Datebonkabfrôg het ghiesse: "$1" us de Funktion "<tt>$2</tt>".

MySQL het den Fähler gmeldet: "<tt>$3: $4</tt>".',
'noconnect'            => 'Hab kei Vobindung zuer Datebonk uf $1 herschtelle kinne',
'nodb'                 => 'Hab d Datebonk $1 nit uswähle kinne',
'cachederror'          => 'D folgende isch ä Kopie usm Cache un möglicherwis nit aktuell.',
'laggedslavemode'      => 'Obacht: Kürzlich vorgnommene Änderunge wärdet u.U. no nit aazaigt!',
'readonly'             => 'Datebonk isch gsperrt',
'enterlockreason'      => 'Bitte gib ä Grund i, worum Datebonk gsperrt werre soll un ä Yschätzung yber d Dur vum Sperre',
'readonlytext'         => 'Diä {{SITENAME}}-Datebonk isch vorybergehend fier Neijyträg un Änderige gsperrt. Bitte vosuechs s später no mol.

Grund vun de Sperrung: $1',
'readonly_lag'         => 'Datebonk isch automatisch gschperrt worre, wil d Sklavedatebonkserver ihr Meischter yhole miesse',
'internalerror'        => 'Interner Fähler',
'filecopyerror'        => 'Datei "$1" het nit noch "$2" kopiert werre kinne.',
'filerenameerror'      => 'Datei "$1" het nit noch "$2" umbnennt werre kinne.',
'filedeleteerror'      => 'Datei "$1" het nit glöscht werre kinne.',
'filenotfound'         => 'Datei "$1" isch nit gfunde worre.',
'formerror'            => 'Fähler: Ds Formular het nid chönne verarbeitet wärde',
'badarticleerror'      => 'D Aktion konn uf denne Artikel nit ongwendet werre.',
'cannotdelete'         => 'Konn d spezifiziert Syte odr Artikel nit lösche. (Isch möglicherwis schu vun ebr ondrem glöscht worre.)',
'badtitle'             => 'Ugültiger Titel',
'badtitletext'         => 'Dr Titel vun de ongfordert Syte isch ugültig gsi, leer, odr ä ugültiger Sprochlink vun nm ondre Wiki.',
'perfdisabled'         => "Leider isch die Funktion momentan usgschalte, wil's d Datebank eso starch würd belaschte, dass mer s Wiki nümm chönnti benütze.",
'perfcached'           => 'Selli Informatione chömme usem Zwüschespeicher un sin derwiil viilliecht nid aktuell.
----',
'perfcachedts'         => 'D folgendi Date stomme usm Cache un sin om $1 s letzscht mol aktualisiert worre.',
'wrong_wfQuery_params' => 'Falschi Parameter fier wfQuery()<br />
Funktion: $1<br />
Abfrog: $2',
'viewsource'           => 'Quelltext aaluege',
'viewsourcefor'        => 'fier $1',
'viewsourcetext'       => 'Quelltekst vo dere Syte:',
'protectedinterface'   => 'Die Syte enthält Text fiers Sproch-Interface vun de Software un isch gsperrt, um Missbrouch zue vohindre.',
'editinginterface'     => "'''Obacht:''' Du bisch e Syten am Verändere wo zum user interface ghört. We du die Syte veränderisch, de änderet sech ds user interface o für di andere Benutzer.",
'sqlhidden'            => '(SQL-Abfrog voschteckt)',

# Login and logout pages
'logouttitle'                => 'Benutzer-Abmäldig',
'logouttext'                 => '<div align="center" style="background-color:white;">
<b>Du bisch jitz abgmäldet!</b>
</div><br />
We du jitz öppis uf der {{SITENAME}} änderisch, de wird dyni IP-Adrässen als Urhäber regischtriert u nid dy Benutzername. Du chasch di mit em glychen oder emnen andere Benutzername nöi aamälde.',
'welcomecreation'            => '==Willkomme, $1!==
Dys Benutzerkonto isch aagleit worde.
Vergis nid, dyni Ystelligen aazpasse.',
'loginpagetitle'             => 'Benutzer-Aamelde',
'yourname'                   => 'Dii Benutzername',
'yourpassword'               => 'Basswort',
'yourpasswordagain'          => 'Basswort nommol iitipe',
'remembermypassword'         => 'Passwort spychere',
'yourdomainname'             => 'Diini Domäne',
'externaldberror'            => 'Entwedr s ligt ä Fähler bi dr extern Authentifizierung vor, odr du derfsch din externs Benutzerkonto nit aktualisiere.',
'loginproblem'               => "'''S het ä Problem mit dinre Onmeldung gäbe.'''<br />Bitte vosuechs grad nomal!",
'login'                      => 'Aamälde',
'nav-login-createaccount'    => 'Amälde/Regischtriere',
'loginprompt'                => '<small>Für di bir {{SITENAME}} aazmälde, muesch Cookies erloube!</small>',
'userlogin'                  => 'Aamälde',
'logout'                     => 'Abmälde',
'userlogout'                 => 'Abmälde',
'notloggedin'                => 'Nit aagmäldet',
'nologin'                    => 'No chei Benutzerchonto? $1.',
'nologinlink'                => '»Chonto aaleege«',
'createaccount'              => 'Nöis Benutzerkonto aalege',
'gotaccount'                 => 'Du häsch scho a Chonto? $1',
'gotaccountlink'             => '»Login für beryts aagmeldete Benutzer«',
'createaccountmail'          => 'yber eMail',
'badretype'                  => 'Di beidi Passwörter stimme nit yberi.',
'userexists'                 => 'Dä Benutzername git’s scho.
Bitte lis en anderen uus.',
'youremail'                  => 'Ihri E-Bost-Adräss**',
'username'                   => 'Benutzernome:',
'yourrealname'               => 'Ihre Name*',
'yourlanguage'               => 'Sproch:',
'yourvariant'                => 'Variante:',
'yournick'                   => 'Spitzname (zuem Untrschriibe):',
'badsig'                     => 'Dr Syntax vun de Signatur isch ungültig; luege uffs HTML.',
'email'                      => 'E-Bost',
'prefs-help-realname'        => '* <strong>Dy ächt Name</strong> (optional): We du wosch, das dyni Änderigen uf di chöi zrüggfüert wärde.',
'loginerror'                 => 'Fähler bir Aamäldig',
'prefs-help-email'           => "* <strong>E-Bost-Adräss</strong> (optional): Dodemit chönne anderi Lüt übr Ihri Benutzersyte mitene Kontakt uffneh, ohni dass Si muen Ihri E-Bost-Adräss z'veröffentliche.
Im Fall dass Si mol Ihr Basswort vergässe hen cha Ihne au e ziitwiiligs Eimol-Basswort gschickt wärde.",
'nocookieslogin'             => '{{SITENAME}} bruucht Cookies für nen Aamäldig. Du hesch Cookies deaktiviert. Aktivier se bitte u versuech’s nomal.',
'noname'                     => 'Du muesch ä Benutzername aagebe.',
'loginsuccesstitle'          => 'Aamäldig erfolgrych',
'loginsuccess'               => "'''Du bisch jetz als \"\$1\" bi {{SITENAME}} aagmäldet.'''",
'nosuchuser'                 => 'Dr Benutzername "$1" exischtiert nit.

Yberprüf d Schribwis, odr meld dich als [[Special:Userlogin/signup|neijer Benutzer ô]].',
'nosuchusershort'            => 'S gibt kei Benutzername „<nowiki>$1</nowiki>“. Bitte yberprüf mol d Schribwis.',
'nouserspecified'            => 'Bitte gib ä Benutzername ii.',
'wrongpassword'              => "Sell Basswort isch falsch (odr fählt). Bitte versuech's nomol.",
'wrongpasswordempty'         => 'Du hesch vagässe diin Basswort iizgeh. Bitte probiers nomol.',
'passwordtooshort'           => 'Dys Passwort isch ungültig oder z churz.
Es mues mindischtens {{PLURAL:$1|1 Zeiche|$1 Zeiche}} ha u sech vom Benutzernamen underscheide.',
'mailmypassword'             => 'Es nöis Passwort schicke',
'passwordremindertitle'      => 'Neijs Password fier {{SITENAME}}',
'passwordremindertext'       => 'Ebber mit dr IP-Adress $1 het ä neijs Passwort fier d Anmeldung bi {{SITENAME}} ($4) ongfordert.

S automatisch generiert Passwort fier de Benutzer $2 lutet jetzert: $3

Du sottsch dich jetzt onmelde un s Passwort ändere: {{fullurl:Special:UserLogin}}

Bitte ignorier die E-Mail, wenn du s nit selber ongfordert hesch. S alt Passwort blibt witerhin gültig.',
'noemail'                    => 'Dr Benutzer "$1" het kei E-Mail-Adress ongebe.',
'passwordsent'               => 'Ä zytwilligs Passwort isch on d E-Mail-Adress vum Benutzer "$1" gschickt worre.
Bitte meld dich domit ô, wenns bekumme hesch.',
'eauthentsent'               => 'Es Bestätigungs-Mail isch a die Adrässe gschickt worde, wo du hesch aaggä. 

Bevor das wyteri Mails yber d {{SITENAME}}-Mailfunktion a die Adrässe gschickt wärde, muesch du d Instruktionen i däm Mail befolge, für z bestätige, das es würklech dys isch.',
'mailerror'                  => 'Fähler bim Sende vun de Mail: $1',
'acct_creation_throttle_hit' => 'Duet mr leid, so hän scho $1 Benutzer. Si chönne cheini meh aalege.',
'emailauthenticated'         => 'Di E-Bost-Adräss isch am $1 bschtätigt worde.',
'emailnotauthenticated'      => 'Dyni e-Mail-Adrässen isch no nid bestätiget. Drum göh di erwytereten e-Mail-Funktione no nid.
Für d Bestätigung muesch du em Link folge, wo dir isch gmailet worde. Du chasch ou e nöie söttige Link aafordere:',
'noemailprefs'               => '<strong>Du hesch kei E-Mail-Adrässen aaggä</strong>, drum sy di folgende Funktione nid müglech.',
'emailconfirmlink'           => 'E-Bost-Adräss bschtätige',
'invalidemailaddress'        => 'Diä E-Mail-Adress isch nit akzeptiert worre, wil s ä ugültigs Format ghet het.
Bitte gib ä neiji Adress in nem gültige Format ii, odr tue s Feld leere.',
'accountcreated'             => 'De Benutzer isch agleit worre.',
'accountcreatedtext'         => 'De Benutzer $1 isch aagleit worre.',

# Edit page toolbar
'bold_sample'     => 'fetti Schrift',
'bold_tip'        => 'Fetti Schrift',
'italic_sample'   => 'kursiv gschribe',
'italic_tip'      => 'Kursiv gschribe',
'link_sample'     => 'Stichwort',
'link_tip'        => 'Interne Link',
'extlink_sample'  => 'http://www.example.com Linktekscht',
'extlink_tip'     => 'Externer Link (http:// beachte)',
'headline_sample' => 'Abschnitts-Überschrift',
'headline_tip'    => 'Überschrift Äbeni 2',
'math_sample'     => 'Formel do yfüge',
'math_tip'        => 'Mathematisch Formel (LaTeX)',
'nowiki_sample'   => 'Was da inne staht wird nid formatiert',
'nowiki_tip'      => 'Wiki-Formatierige ignoriere',
'image_sample'    => 'Byschpil.jpg',
'image_tip'       => 'Bildvoweis',
'media_sample'    => 'Byschpil.mp3',
'media_tip'       => 'Dateie-Link',
'sig_tip'         => 'Dyni Signatur mit Zytagab',
'hr_tip'          => 'Horizontal Linie (sparsom vowende)',

# Edit pages
'summary'                  => 'Zämefassig',
'subject'                  => 'Beträff',
'minoredit'                => 'Numen es birebitzeli gänderet',
'watchthis'                => 'Dä Artikel beobachte',
'savearticle'              => 'Syte spychere',
'preview'                  => 'Vorschou',
'showpreview'              => 'Vorschau aaluege',
'showdiff'                 => 'Zeig Änderige',
'anoneditwarning'          => "'''Warnig:''' Si sin nit agmolde. Ihri IP-Adrässe wird in de Gschicht vo sellem Artikel gspeicheret.",
'missingsummary'           => "'''Obacht:''' Du hesch kei Zämefassig ongebe. Wenn du erneijt uf Spacher durcksch, wird d Änderung ohni gspychert.",
'missingcommenttext'       => 'Bitte gib dinr Kommentar unte ii.',
'summary-preview'          => 'Vorschou vor Zämefassig',
'blockedtitle'             => "Benutz'r esch gspertd",
'blockedtext'              => "<big>'''Dy Benutzernamen oder dyni IP-Adrässen isch gsperrt worde.'''</big>

Du chasch $1 oder en anderen [[{{MediaWiki:Grouppage-sysop}}|Administrator]] kontaktiere, für die Sperrig z diskutiere. Vergis i däm Fall bitte keni vo de folgenden Agabe:

*Administrator, wo het gsperrt: $1
*Grund für d Sperrig: $2
*Afang vor Sperrig: $8
*Ändi vor Sperrig: $6
*IP-Adrässe: $3
*Sperrig betrifft: $7
*ID vor Sperrig: #$5",
'whitelistedittext'        => 'Sie müssen sich $1, um Artikel bearbeiten zu können.',
'confirmedittitle'         => 'Zuem Ändere isch e bschtätigti E-Bost-Adräss nötig.',
'confirmedittext'          => 'Si muen Ihri E-Bost-Adräss erscht bstätige bevor Si Syte go ändere chönne. Bitte setze Si in [[Special:Preferences|Ihre Iistellige]] Ihri E-Bost Adräss ii un löhn Si si pruefe.',
'accmailtitle'             => 'S Bassword isch verschickt worre.',
'accmailtext'              => 'S Basswort für "$1" isch uf $2 gschickt worde.',
'newarticle'               => '(Nöu)',
'newarticletext'           => '<div id="newarticletext">
{{MediaWiki:Newarticletext/{{NAMESPACE}}}}
</div>',
'anontalkpagetext'         => "----''Sell isch e Diskussionssyte vome anonyme Benutzer wo chei Zuegang aaglegt het odr wo ihn nit bruucht. Sälleweg muen mir di numerischi IP-Adräss bruuche um ihn odr si z'identifiziere. Sone IP-Adräss cha au vo mehrere Benutzer deilt werde. Wenn Si en anonyme Benutzer sin un 's Gfuehl hen, dass do irrelevanti Kommentar an Si grichtet wärde, denn [[Special:UserLogin|lege Si sich bitte en Zuegang aa odr mälde sich aa]] go in Zuekunft Verwirrige mit andere anonyme Benutzer z'vermeide.''",
'noarticletext'            => "Uf dere Syte het's no kei Tekscht. Du chasch uf anderne Syte [[Special:Search/{{PAGENAME}}|dä Ytrag sueche]] oder [{{fullurl:{{FULLPAGENAME}}|action=edit}} die Syte bearbeite].",
'clearyourcache'           => "'''Hywys:''' Nôch dyner Änderig muess no der Browser-Cache gleert wärde!<br />'''Mozilla/Safari/Konqueror:''' ''Strg-Umschalttaste-R'' (oder ''Umschalttaste'' drückt halte und uf’s ''Neu-Laden''-Symbol klicke), '''IE:''' ''Strg-F5'', '''Opera/Firefox:''' ''F5''",
'usercsspreview'           => "== Vorschau ihres Benutzer-CSS. ==
'''Beachten Sie:''' Nach dem Speichern müssen Sie ihrem Browser sagen, die neue Version zu laden: '''Mozilla:''' ''Strg-Shift-R'', '''IE:''' ''Strg-F5'', '''Safari:''' ''Cmd-Shift-R'', '''Konqueror:''' ''F5''.",
'userjspreview'            => "== Vorschau Ihres Benutzer-Javascript. ==
'''Beachten Sie:''' Nach dem Speichern müssen Sie ihrem Browser sagen, die neue Version zu laden: '''Mozilla:''' ''Strg-Shift-R'', '''IE:''' ''Strg-F5'', '''Safari:''' ''Cmd-Shift-R'', '''Konqueror:''' ''F5''.",
'note'                     => '<strong>Achtung: </strong>',
'previewnote'              => '<strong>Das isch numen e Vorschau und nonig gspycheret!</strong>',
'editing'                  => 'Bearbeite vo «$1»',
'editingsection'           => 'Bearbeite vo «$1» (Absatz)',
'editconflict'             => 'Bearbeitigs-Konflikt: «$1»',
'explainconflict'          => "Öpper anders het dä Artikel gänderet, wo du ne sälber am Ändere bisch gsy.
Im obere Tekschtfäld steit der jitzig Artikel.
Im untere Tekschtfält stöh dyni Änderige.
Bitte überträg dyni Änderigen i ds obere Tekschtfäld.
We du «Syte spychere» drücksch, de wird '''nume''' der Inhalt vom obere Tekschtfäld gspycheret.",
'yourtext'                 => 'Ihre Tekscht',
'storedversion'            => 'Gspychereti Version',
'editingold'               => '<strong>Obacht: Du bisch en alti Version vo däm Artikel am Bearbeite.
Alli nöiere Versione wärden überschribe, we du uf «Syte spychere» drücksch.</strong>',
'yourdiff'                 => 'Untrschied',
'copyrightwarning'         => "<strong>Bitte <big>kopier kener Internetsyte</big>, wo nid dyner eigete sy, bruuch <big>kener urhäberrächtlech gschützte Wärch</big> ohni Erloubnis vor Copyright-Inhaberschaft!</strong><br />
Hiemit gisch du zue, das du dä Tekscht <strong>sälber gschribe</strong> hesch, das der Tekscht Allgmeinguet (<strong>public domain</strong>) isch, oder das der <strong>Copyright-Inhaberschaft</strong> iri <strong>Zuestimmig</strong> het 'gä. Falls dä Tekscht scho nöumen anders isch veröffentlecht worde, de schryb das bitte uf d Diskussionssyte.
<i>Bis dir bewusst, dass alli {{SITENAME}}-Byträg outomatisch under der „$2“ stöh (für Details vgl. $1). We du nid wosch, das anderi dy Bytrag chöu veränderen u wyterverbreite, de drück nid uf „Syte spychere“.</i>",
'copyrightwarning2'        => 'Dängge Si dra, dass alli Änderige {{GRAMMAR:dativ {{SITENAME}}}} vo andere Benutzer wiedr gänderet odr glöscht wärde chönne. Wenn Si nit wänn, dass ander Lüt an Ihrem tekscht ummedoktere denn schicke Si ihn jetz nit ab.<br />
Si verspräche uns usserdäm, dass Si des alles selber gschriebe oder vo nere Quälle kopiert hen, wo Public Domain odr sunscht frei isch (lueg $1 für Details).
<strong>SETZE SI DO OHNI ERLAUBNIS CHEINI URHEBERRÄCHTLICH GSCHÜTZTI WÄRK INE!</strong>',
'longpagewarning'          => '<span style="color:#ff0000">WARNIG:</span> Die Syten isch $1KB groß; elteri Browser chönnte Problem ha, Sytene z bearbeite wo gröser sy als 32KB. Überleg bitte, öb du Abschnitte vo dere Syte zu eigete Sytene chönntsch usboue.',
'protectedpagewarning'     => '<strong>WARNIG: Die Syten isch gsperrt worde, so das se nume Benutzer mit Sysop-Rechten chöi verändere.</strong>',
'semiprotectedpagewarning' => "'''''Halbsperrung''': Diese Seite kann von angemeldeten Benutzern bearbeitet werden. Für nicht angemeldete oder gerade eben erst angemeldete Benutzer ist der Schreibzugang gesperrt.''",
'templatesused'            => 'Selli Vorlage wärde in sellem Artikel bruucht:',
'templatesusedpreview'     => 'Vorlage wo i dere Vorschou vorchöme:',
'template-protected'       => '(schrybgschützt)',
'template-semiprotected'   => '(schrybgschützt für unaagmoldeni un neui Benutzer)',
'edittools'                => '<!-- Selle Text wird untr em "ändere"-Formular un bim "Uffelade"-Formular aagzeigt. -->',
'nocreatetext'             => "Uf {{SITENAME}} isch d Erstellig vo nöue Syten ygschränkt.
Du chasch nur Syten ändere, wo's scho git, oder muesch di [[Special:UserLogin|amälde]].",
'recreate-deleted-warn'    => "'''Obacht: Du bisch e Syten am kreiere, wo scho einisch isch glösche worde.'''

Bitte überprüeff, öb's sinnvoll isch, mit em Bearbeite wyter z mache.
Hie gesehsch ds Lösch-Logbuech vo dere Syte:",

# History pages
'viewpagelogs'        => 'Logbüecher für die Syten azeige',
'currentrev'          => 'Itzigi Version',
'revisionasof'        => 'Version vo $1',
'revision-info'       => 'Alti Bearbeitig vom $1 dür $2',
'previousrevision'    => '← Vorderi Version',
'nextrevision'        => 'Nächschti Version →',
'currentrevisionlink' => 'Itzigi Version',
'cur'                 => 'Jetz',
'next'                => 'Nächschti',
'last'                => 'vorane',
'page_first'          => 'Afang',
'page_last'           => 'Ändi',
'histlegend'          => 'Du chasch zwei Versionen uswähle und verglyche.<br />
Erklärig: (aktuell) = Underschid zu jetz,
(vorane) = Underschid zur alte Version, <strong>K</strong> = chlyni Änderig',
'histfirst'           => 'Eltischti',
'histlast'            => 'Nöischti',

# Revision feed
'history-feed-item-nocomment' => '$1 um $2', # user at time

# Diffs
'history-title'           => 'Versionsgschicht vo „$1“',
'difference'              => '(Unterschide zwüsche Versione)',
'lineno'                  => 'Zyle $1:',
'compareselectedversions' => 'Usgwählti Versione verglyche',
'editundo'                => 'rückgängig',
'diff-multi'              => '(Der Versioneverglych zeigt ou d Änderige vo {{PLURAL:$1|1 Version|$1 Versione}} derzwüsche.)',

# Search results
'searchresults'         => 'Suech-Ergäbnis',
'searchresulttext'      => 'Für wiiteri Informatione zuem Sueche uff {{SITENAME}} chönne Si mol uff [[{{MediaWiki:Helppage}}|{{int:help}}]] luege.',
'searchsubtitle'        => 'Für d Suechaafrag «[[:$1]]»',
'searchsubtitleinvalid' => 'Für d Suechaafrag «$1»',
'noexactmatch'          => "'''Es git kei Syte mit em Tiel „$1“.'''
Du chasch die [[:$1|Syte nöu schrybe]].",
'prevn'                 => 'vorderi $1',
'nextn'                 => 'nächschti $1',
'viewprevnext'          => '($1) ($2) aazeige; ($3) uf ds Mal',
'powersearch'           => 'Erwytereti Suechi',
'searchdisabled'        => '<p>Die Volltextsuche wurde wegen Überlastung temporär deaktiviert. Derweil können Sie entweder folgende Google- oder Yahoo-Suche verwenden, die allerdings nicht den aktuellen Stand widerspiegeln.</p>',

# Preferences page
'preferences'        => 'Iistellige',
'mypreferences'      => 'Ystellige',
'prefsnologin'       => 'Nid aagmäldet',
'prefsnologintext'   => 'Du muesch <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} aagmäldet]</span> sy, für Benutzerystellige chönne z ändere',
'prefsreset'         => 'Du hesch itz wider Standardystellige',
'changepassword'     => 'Passwort ändere',
'datedefault'        => 'kei Aagab',
'datetime'           => 'Datum un Zit',
'prefs-personal'     => 'Benutzerdate',
'prefs-rc'           => 'Letschti Änderige',
'prefs-watchlist'    => 'Beobachtigslischte',
'prefs-misc'         => 'Verschidnigs',
'saveprefs'          => 'Änderige spychere',
'resetprefs'         => 'Änderige doch nid spychere',
'oldpassword'        => 'Alts Passwort',
'newpassword'        => 'Nöis Passwort',
'retypenew'          => 'Nöis Passwort (es zwöits Mal)',
'textboxsize'        => 'Tekscht-Ygab',
'rows'               => 'Zylene',
'columns'            => 'Spaltene',
'searchresultshead'  => 'Suech-Ergäbnis',
'resultsperpage'     => 'Träffer pro Syte',
'contextlines'       => 'Zyle pro Träffer',
'contextchars'       => 'Zeiche pro Zyle',
'recentchangescount' => 'Aazahl «letschti Änderige»',
'savedprefs'         => 'Dyni Ystellige sy gspycheret worde.',
'timezonelegend'     => 'Zytzone',
'timezonetext'       => 'Zytdifferänz i Stunden aagä zwüsche der Serverzyt u dyre Lokalzyt',
'localtime'          => 'Ortszyt',
'timezoneoffset'     => 'Unterschid¹',
'servertime'         => 'Aktuelli Serverzyt',
'guesstimezone'      => 'Vom Browser la ysetze',
'allowemail'         => 'andere Benutzer erlaube, dass si Ihne E-Bost schicke chönne',
'defaultns'          => 'Namensrüüm wo standardmäässig söll gsuecht wärde:',
'files'              => 'Bilder',

# User rights
'userrights'               => 'Benutzerrechtsverwaltung', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Verwalte Gruppenzugehörigkeit',
'editusergroup'            => 'Ändere vo Benutzerrächt',
'editinguser'              => "Bearbeite vo '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Bearbeite Gruppenzugehörigkeit des Benutzers',
'saveusergroups'           => 'Speichere Gruppenzugehörigkeit',

'grouppage-sysop' => '{{ns:project}}:Administratore',

# User rights log
'rightslog'     => 'Benutzerrächt-Logbuech',
'rightslogtext' => 'Des ischs Logbuech vun de Änderunge on Bnutzerrechte.',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|Änderig|Änderige}}',
'recentchanges'                  => 'Letschti Änderige',
'recentchangestext'              => 'Uff sellere Syte chönne Si die letschte Änderige in sellem Wiki aaluege.',
'recentchanges-feed-description' => 'Di letschten Änderige vo {{SITENAME}} i däm Feed abonniere.',
'rcnote'                         => "Azeigt {{PLURAL:$1|wird '''1''' Änderig|wärde di letschte '''$1''' Änderige}} {{PLURAL:$2|vom letschte Tag|i de letschte '''$2''' Täg}} (Stand: $4, $5)",
'rcnotefrom'                     => 'Dies sind die Änderungen seit <b>$2</b> (bis zu <b>$1</b> gezeigt).',
'rclistfrom'                     => '<small>Nöji Änderige ab $1 aazeige (UTC)</small>',
'rcshowhideminor'                => 'Chlynigkeite $1',
'rcshowhidebots'                 => 'Bots $1',
'rcshowhideliu'                  => 'Aagmoldene Benützer $1',
'rcshowhideanons'                => 'Uuaagmoldene Benützer $1',
'rcshowhidepatr'                 => 'Patrulyrtes $1',
'rcshowhidemine'                 => 'Eigeni Änderige $1',
'rclinks'                        => 'Zeig di letschte $1 Änderige vo de vergangene $2 Täg.<br />$3',
'diff'                           => 'Unterschid',
'hist'                           => 'Versione',
'hide'                           => 'usblände',
'show'                           => 'yblände',
'minoreditletter'                => 'C',
'newpageletter'                  => 'N',
'boteditletter'                  => 'B',

# Recent changes linked
'recentchangeslinked'          => 'Verlinktes prüefe',
'recentchangeslinked-title'    => 'Änderigen a Sytene, wo „$1“ druf verlinkt',
'recentchangeslinked-noresult' => 'Kener Änderigen a verlinkte Sytenen im usgwählte Zytruum.',
'recentchangeslinked-summary'  => "Die Spezialsyte zeigt d Änderige vo allne Syte, wo ei vo dir bestimmti Syte druf verlinkt, bzw. vo allne Syte, wo zu eire vo dir bestimmte Kategorie ghöre.
Sytene, wo zu dyre [[Special:Watchlist|Beobachtigslischte]] ghöre, erschyne '''fett'''.",

# Upload
'upload'            => 'Datei uffelade',
'uploadbtn'         => 'Bild lokal ufelade',
'uploadnologintext' => 'Sie müssen [[Special:UserLogin|angemeldet sein]], um Dateien hochladen zu können.',
'uploadtext'        => "Bruuche Si sell Formular unte go Dateie uffelade. Zuem aaluege odr fruener uffegladeni Bilder go sueche lueg uff de [[Special:ImageList|Lischte vo uffegladene Dateie]], Uffeladige un Löschige sin au protokolliert uff [[Special:Log/upload|Uffeladige Protokoll]].

Go e Datei odr en Bild innere Syte iizbaue schriibe Si eifach ane:
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.jpg]]</nowiki>'''
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.png|alt text]]</nowiki>'''
or
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:file.ogg]]</nowiki>'''
go direkt e Gleich uff d Datei z'mache.",
'uploadlogpage'     => 'Ufegladnigs-Logbuech',
'uploadedimage'     => 'het „[[$1]]“ ufeglade',

# Special:ImageList
'imagelist' => 'Lischte vo Bilder',

# Image description page
'filehist'                  => 'Dateiversione',
'filehist-help'             => "Klick uf'ne Zytpunkt für azzeige, wie's denn het usgseh.",
'filehist-current'          => 'aktuell',
'filehist-datetime'         => 'Version vom',
'filehist-user'             => 'Benutzer',
'filehist-dimensions'       => 'Mäß',
'filehist-filesize'         => 'Dateigrößi',
'filehist-comment'          => 'Kommentar',
'imagelinks'                => 'Bildverweise',
'linkstoimage'              => 'Di {{PLURAL:$1|folgendi Syte|$1 folgende Sytene}} händ en Link zu dem Bildli:',
'nolinkstoimage'            => 'Kein Artikel benutzt dieses Bild.',
'sharedupload'              => 'Selli Datei wird vo verschiedene Projekt bruucht.',
'noimage'                   => 'Es git kei Datei mit däm Name, aber du chasch se $1.',
'noimage-linktext'          => 'ufelade',
'uploadnewversion-linktext' => 'E nöui Version vo dere Datei ufelade',

# MIME search
'mimesearch' => 'MIME-Suechi',

# Unwatched pages
'unwatchedpages' => 'Unbeobachteti Sytene',

# List redirects
'listredirects' => 'Lischte vo Wyterleitige (Redirects)',

# Unused templates
'unusedtemplates' => 'Nid ’bruuchti Vorlage',

# Random page
'randompage' => 'Zuefalls-Artikel',

# Random redirect
'randomredirect' => 'Zuefälligi Wyterleitig',

# Statistics
'statistics'    => 'Statistik',
'sitestats'     => 'Statistik',
'userstats'     => 'Benützer-Statistik',
'sitestatstext' => "Zuer Ziit git's '''$2''' Artikel in {{SITENAME}}.

Insgsamt sin '''$1''' Syte in de Datebank. Selli sin au alli Sytene wo usserhalb vom Hauptnamensruum exischtiere (z.B. Diskussionssyte) odr wo cheini interne Gleicher hen odr wo au numme Weiterleitige sin.

Insgesamt wurden '''$8''' Dateien hochgeladen.

Es isch insgsamt häts '''$3''' {{PLURAL:$3|Seiteabruf|Seiteabruf}} gäh, '''$4''' mol öbbis gänderet worde un drmit jedi Syte im Durchschnitt '''$5''' mol und '''$6''' Seitenabrufe pro Bearbeitung.

Es het '''$8''' uffegladeni Dateie.

Längi vo de [http://www.mediawiki.org/wiki/Manual:Job_queue „Job queue“]: '''$7'''",
'userstatstext' => "S git '''$1''' regischtriirte Benutzer. Dodrvo sin '''$2''' (also '''$4 %''') Administratore (lueg au uff $3).",

'disambiguations'     => 'Begriffsklärigssytene',
'disambiguationspage' => 'Template:Begriffsklärig',

'doubleredirects' => 'Doppelte Redirects',

'brokenredirects'     => 'Kaputti Wyterleitige',
'brokenredirectstext' => "Di folgende Wyterleitige füered zu Artikel wo's gar nid git.",

'withoutinterwiki' => 'Sytenen ohni Links zu andere Sprache',

'fewestrevisions' => 'Syte mit de wenigschte Bearbeitige',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|Byte|Bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|Kategori|Kategorie}}',
'nlinks'                  => '$1 {{PLURAL:$1|Gleich|Gleicher}}',
'nmembers'                => '$1 {{PLURAL:$1|Syte|Sytene}}',
'nrevisions'              => '$1 {{PLURAL:$1|Revision|Revisione}}',
'nviews'                  => '$1 {{PLURAL:$1|Betrachtig|Betrachtige}}',
'lonelypages'             => 'Verwaisti Sytene',
'uncategorizedpages'      => 'Nit kategorisierte Sytene',
'uncategorizedcategories' => 'Nit kategorisierte Kategorie',
'uncategorizedimages'     => 'Nid kategorisierti Dateie',
'uncategorizedtemplates'  => 'Nid kategorisierti Vorlage',
'unusedcategories'        => 'Nid ’bruuchti Kategorië',
'unusedimages'            => 'Verwaiste Bilder',
'popularpages'            => 'Beliebti Artikel',
'wantedcategories'        => '’Bruuchti Kategorië, wo’s no nid git',
'wantedpages'             => 'Artikel wo fähle',
'mostlinked'              => 'Meistverlinke Seiten',
'mostlinkedcategories'    => 'Am meischte verlinkti Kategorië',
'mostlinkedtemplates'     => 'Am meischten y’bouti Vorlage',
'mostcategories'          => 'Sytene mit de meischte Kategorië',
'mostimages'              => 'Am meischte verlinkti Dateie',
'mostrevisions'           => 'Syte mit de meischte Bearbeitige',
'prefixindex'             => 'Alli Artikle (mit Präfix)',
'shortpages'              => 'Churzi Artikel',
'longpages'               => 'Langi Artikel',
'deadendpages'            => 'Artikel ohni Links («Sackgasse»)',
'protectedpages'          => 'Gschützti Sytene',
'listusers'               => 'Lischte vo Benutzer',
'newpages'                => 'Nöji Artikel',
'ancientpages'            => 'alti Sytene',
'move'                    => 'verschiebe',
'movethispage'            => 'Artikel verschiebe',

# Book sources
'booksources' => 'ISBN-Suech',

# Special:Log
'specialloguserlabel'  => 'Benutzer:',
'speciallogtitlelabel' => 'Titel:',
'log'                  => 'Logbüecher',
'all-logs-page'        => 'Alli Logbüecher',
'alllogstext'          => "Kombinierti Aasicht vo alle i {{SITENAME}} gführte Protokoll.
D'Aazeig cha  durch d'Auswahl vo emne Protokoll, emne Benutzername odr emne Sytename iischränkt werde (Gross- u Chlischribig beachte).",
'logempty'             => 'Kei passendi Yträg gfunde.',

# Special:AllPages
'allpages'          => 'alli Sytene',
'alphaindexline'    => 'vo $1 bis $2',
'nextpage'          => 'Nächscht Syte ($1)',
'prevpage'          => 'Vorderi Syte ($1)',
'allpagesfrom'      => 'Syte aazeige vo:',
'allarticles'       => 'alli Artikel',
'allinnamespace'    => 'alli Sytene im Namensruum $1',
'allnotinnamespace' => 'alli Sytene wo nit im $1 Namensruum sin',
'allpagesprev'      => 'Füehrigs',
'allpagesnext'      => 'nächschts',
'allpagessubmit'    => 'gang',
'allpagesprefix'    => 'Alli Sytene mit em Präfix:',

# Special:Categories
'categories'         => 'Kategorie',
'categoriespagetext' => 'Selli Kategorie gits in dem Wiki:',

# E-mail user
'mailnologin'     => 'Du bisch nid aagmäldet oder hesch keis Mail aaggä',
'mailnologintext' => 'Du muesch [[Special:UserLogin|aagmäldet sy]] und e bestätigeti e-Mail-Adrässen i dynen [[Special:Preferences|Ystelligen]] aaggä ha, für das du öpper anderem es e-Mail chasch schicke.',
'emailuser'       => 'Es Mail schrybe',
'emailpage'       => 'e-Mail ane BenutzerIn',
'emailpagetext'   => 'Öpperem, wo sälber e bestätigeti e-Mail-Adrässe het aaggä, chasch du mit däm Formular es Mail schicke.
Im Absänder steit dyni eigeti e-Mail-Adrässe us dine [[Special:Preferences|Istellige]], so das me dir cha antworte.',
'usermailererror' => 'Das Mail-Objekt gab einen Fehler zurück:',
'noemailtitle'    => 'Kei e-Mail-Adrässe',
'noemailtext'     => 'Dä Benutzer het kei bestätigeti e-Mail-Adrässen aaggä oder wot kei e-Mails vo anderne Benutzer empfa.',
'emailfrom'       => 'Vo',
'emailto'         => 'Empfänger',
'emailsubject'    => 'Titel',
'emailmessage'    => 'E-Bost',
'emailsend'       => 'Abschicke',
'emailsent'       => 'E-Bost furtgschickt',
'emailsenttext'   => 'Dys e-Mail isch verschickt worde.',

# Watchlist
'watchlist'            => 'Beobachtigslischte',
'mywatchlist'          => 'Beobachtigslischte',
'watchlistfor'         => "(für '''$1''')",
'nowatchlist'          => 'Du hesch ke Yträg uf dyre Beobachtigslischte.',
'watchnologintext'     => 'Du musst [[Special:UserLogin|angemeldet]] sein, um deine Beobachtungsliste zu bearbeiten.',
'addedwatch'           => 'zue de Beobachtigslischte drzue do',
'addedwatchtext'       => 'D Syte "[[:$1]]" stoht jetz uf Ihre [[Special:Watchlist|Beobachtigslischte]].
Neui Änderige an de Syte odr de Diskussionssyte drvo chasch jetz dört seh. Usserdem sin selli Änderige uf de [[Special:RecentChanges|letschte Änderige]] fett gschriibe, dass Si s schneller finde.

Wenn Si d Syte spöter wiedr vo de Lischte striiche wenn, denn drucke Si eifach uf "nümm beobachte".',
'removedwatch'         => 'Us der Beobachtigsliste glösche',
'removedwatchtext'     => 'D Syte «[[:$1]]» isch us dyre [[Special:Watchlist|Beobachtigsliste]] glösche worde.',
'watch'                => 'beobachte',
'watchthispage'        => 'Die Syte beobachte',
'unwatch'              => 'nümm beobachte',
'watchnochange'        => 'Vo den Artikle, wo du beobachtisch, isch im aazeigte Zytruum kene veränderet worde.',
'watchlist-details'    => '{{PLURAL:$1|1 Syte wird|$1 Sytene wärde}} beobachtet (Diskussionssyte nid zelt, aber ou beobachtet).',
'wlshowlast'           => 'Zeig di letschte $1 Stunde $2 Tage $3',
'watchlist-hide-bots'  => 'Bot-Änderige verstecke',
'watchlist-hide-own'   => 'Eigeti Änderige verstecke',
'watchlist-hide-minor' => 'Chlyni Änderige verstecke',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Am beobachte …',
'unwatching' => 'Nümm am beobachten …',

'enotif_subject'     => 'Die {{SITENAME}} Seite $PAGETITLE wurde von $PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited' => '$1 zeigt alle Änderungen auf einen Blick.',
'enotif_body'        => 'Liebe/r $WATCHINGUSERNAME,

d {{SITENAME}} Syte $PAGETITLE isch vom $PAGEEDITOR am $PAGEEDITDATE $CHANGEDORCREATED,
di aktuelli Version isch: $PAGETITLE_URL

$NEWPAGE

Zämmenfassig vom Autor: $PAGESUMMARY $PAGEMINOREDIT
Kontakt zuem Autor:
Mail $PAGEEDITOR_EMAIL
Wiki $PAGEEDITOR_WIKI

Es wird chei wiiteri Benochrichtigungsbost gschickt bis Si selli Syte wiedr bsueche. Uf de Beobachtigssyte chönne Si d Beobachtigsmarker zrucksetze.

             Ihr fründlichs {{SITENAME}} Benochrichtigssyschtem

---
Ihri Beobachtigslischte {{fullurl:Special:Watchlist/edit}}
Hilf zue de Benutzig gits uff {{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Syte lösche',
'confirm'                     => 'Bestätige',
'excontentauthor'             => "einzigen Inhalt: '$1' (bearbeitet worde nume dür '$2')",
'historywarning'              => '<span style="color:#ff0000">WARNUNG:</span> Die Seite die Sie zu löschen gedenken hat eine Versionsgeschichte:',
'confirmdeletetext'           => 'Du bisch drann, en Artikel oder es Bild mitsamt Versionsgschicht permanänt us der Datebank z lösche.
Bitte bis dir über d Konsequänze bewusst, u bis sicher, das du di a üsi [[{{MediaWiki:Policy-url}}|Leitlinien]] haltisch.',
'actioncomplete'              => 'Uftrag usgfuehrt.',
'deletedtext'                 => '«<nowiki>$1</nowiki>» isch glösche worde.
Im $2 het’s e Lischte vo de letschte Löschige.',
'deletedarticle'              => '„[[$1]]“ glösche',
'dellogpage'                  => 'Lösch-Logbuech',
'deletionlog'                 => 'Lösch-Logbuech',
'deletecomment'               => 'Löschigsgrund',
'deleteotherreason'           => 'Andere/zuesätzleche Grund:',
'deletereasonotherlist'       => 'Andere Grund',
'rollback_short'              => 'Zrüggsetze',
'rollbacklink'                => 'Zrüggsetze',
'alreadyrolled'               => 'Cha d Änderig uf [[:$1]] wo [[User:$2|$2]] ([[User talk:$2|Talk]]) gmacht het nit zruckneh will des öbber anderscht scho gmacht het.

Di letschti Änderig het [[User:$3|$3]] ([[User talk:$3|Talk]]) gmacht.',
'revertpage'                  => 'Rückgängig gmacht zuer letschte Änderig vo [[Special:Contributions/$2|$2]] ([[User talk:$2|Diskussion]]) mit de letzte version vo [[User:$1|$1]] wiederhergstellt', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Syteschutz-Logbuech',
'protectlogtext'              => 'Dies ist eine Liste der blockierten Seiten. Siehe [[Special:ProtectedPages|Geschützte Seiten]] für mehr Informationen.',
'protectcomment'              => 'Grund der Sperrung',
'protectexpiry'               => 'Gsperrt bis:',
'protect_expiry_invalid'      => 'Di gwählti Duur isch nid gültig.',
'protect_expiry_old'          => 'Di gwählti Duur isch scho vergange.',
'protect-unchain'             => 'Verschiebschutz ändere',
'protect-text'                => 'Hie chasch der Schutzstatus vor Syte <strong><nowiki>$1</nowiki></strong> azeigen und ändere.',
'protect-locked-access'       => 'Dys Konto het nid di nötige Rächt, für der Schutzstatus z ändere.
Hie sy di aktuelle Schutzystellige vor Syte <strong>$1</strong>:',
'protect-cascadeon'           => 'Die Syten isch gschützt, wil si {{PLURAL:$1|zur folgende Syte|zu de folgende Syte}} ghört, wo derfür e Kaskadesperrig gilt.
Der Schutzstatus vo dere Syte lat sech la ändere, aber das het kei Yfluss uf d Kaskadesperrig.',
'protect-default'             => 'Alli (Standard)',
'protect-fallback'            => '«$1»-Berächtigung nötig',
'protect-level-autoconfirmed' => 'Nid regischtrierti Benutzer sperre',
'protect-level-sysop'         => 'Nur Adminischtratore',
'protect-summary-cascade'     => 'Kaskade',
'protect-expiring'            => 'bis $1 (UTC)',
'protect-cascade'             => 'Kaskadesperrig – alli y’bundnige Vorlage sy mitgsperrt.',
'protect-cantedit'            => 'Du chasch der Schutzstatus vo dere Syte nid ändere, wil du kener Berächtigunge hesch, für se z bearbeite.',
'restriction-type'            => 'Schutzstatus',
'restriction-level'           => 'Schutzhöchi:',

# Undelete
'undeletehistorynoadmin' => 'Dieser Artikel wurde gelöscht. Der Grund für die Löschung ist in der Zusammenfassung angegeben,
genauso wie Details zum letzten Benutzer der diesen Artikel vor der Löschung bearbeitet hat.
Der aktuelle Text des gelöschten Artikels ist nur Administratoren zugänglich.',
'undeletebtn'            => 'Widerhärstelle',
'undeletedrevisions'     => '{{PLURAL:$1|ei Revision|$1 Revisione}} wiedr zruckgholt.',

# Namespace form on various pages
'namespace'      => 'Namensruum:',
'invert'         => 'Uswahl umkehre',
'blanknamespace' => '(Haupt-)',

# Contributions
'contributions' => 'Benutzer-Byträg',
'mycontris'     => 'mini Biiträg',
'contribsub2'   => 'Für $1 ($2)',
'uctop'         => '(aktuell)',
'month'         => 'u Monet:',
'year'          => 'bis Jahr:',

'sp-contributions-newbies-sub' => 'Für Nöui',
'sp-contributions-blocklog'    => 'Sperrlogbuech',

# What links here
'whatlinkshere'       => 'Was linkt da ane?',
'whatlinkshere-title' => 'Sytene, wo uf „$1“ verlinke',
'linkshere'           => "Di folgende Sytene händ en Link wo zu '''„[[:$1]]“''' führe:",
'nolinkshere'         => "Kein Artikel verwiest zu '''„[[:$1]]“'''.",
'isredirect'          => 'Wyterleitigssyte',
'istemplate'          => 'Vorlageybindig',
'whatlinkshere-prev'  => '{{PLURAL:$1|vorder|vorderi $1}}',
'whatlinkshere-next'  => '{{PLURAL:$1|nächscht|nächschti $1}}',
'whatlinkshere-links' => '← Links',

# Block/unblock
'blockip'         => 'Benutzer bzw. IP blockyre',
'ipbsubmit'       => 'Adresse blockieren',
'ipboptions'      => '1 Stunde:1 hour,2 Stunden:2 hours,6 Stunden:6 hours,1 Tag:1 day,3 Tage:3 days,1 Woche:1 week,2 Wochen:2 weeks,1 Monat:1 month,3 Monate:3 months,1 Jahr:1 year,Für immer:infinite', # display1:time1,display2:time2,...
'ipblocklist'     => 'Liste vo blockierten IP-Adrässen u Benutzernäme',
'blocklistline'   => '$1, $2 het $3 ($4) gschperrt',
'blocklink'       => 'spärre',
'unblocklink'     => 'freigä',
'contribslink'    => 'Byträg',
'blocklogpage'    => 'Sperrigs-Protokoll',
'blocklogentry'   => 'sperrt [[$1]] für d Ziit vo: $2 $3',
'blocklogtext'    => 'Des ischs Logbuech yber Sperrunge un Entsperrunge vun Bnutzer. Automatisch blockti IP-Adresse werre nit erfasst. Lueg au [[Special:IPBlockList|IP-Block Lischt]] fyr ä Lischt vun gsperrti Bnutzer.',
'unblocklogentry' => 'Blockade von $1 aufgehoben',

# Move page
'move-page-legend' => 'Artikel verschiebe',
'movepagetext'     => 'Mit däm Forumlar chasch du en Artikel verschiebe, u zwar mit syre komplette Versionsgschicht. Der alt Titel leitet zum nöie wyter, aber Links ufen alt Titel blyben unveränderet.',
'movepagetalktext' => "D Diskussionssyte wird mitverschobe, '''ussert:'''
*Du verschiebsch d Syten i nen andere Namensruum, oder
*es git scho ne Diskussionssyte mit däm Namen oder
*du wählsch unte d Option, se nid z verschiebe.

I söttigne Fäll müessti d Diskussionssyten allefalls vo Hand kopiert wärde.",
'movearticle'      => 'Artikel verschiebe',
'newtitle'         => 'Zum nöie Titel',
'move-watch'       => 'Die Syte beobachte',
'movepagebtn'      => 'Artikel verschiebe',
'pagemovedsub'     => 'Verschiebig erfolgrych',
'movepage-moved'   => "<big>'''«$1» isch verschobe worde nach «$2»'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'    => 'A Syte mit sellem Name gits scho odr de Name isch ungültigt. Bitte nimm en andere.',
'talkexists'       => 'D Syte sälber isch erfolgrych verschobe worde, nid aber d Diskussionssyte, wil’s under em nöue Titel scho eini het ’gä. Bitte setz se vo Hand zäme.',
'movedto'          => 'verschoben uf',
'movetalk'         => 'Diskussionssyte nach Müglechkeit mitverschiebe',
'1movedto2'        => '[[$1]] isch uf [[$2]] verschobe worde.',
'1movedto2_redir'  => '[[$1]] isch uf [[$2]] verschobe worre un het drbii e Wiiterleitig übrschriebe.',
'movelogpage'      => 'Verschiebigs-Logbuech',
'movereason'       => 'Grund',
'revertmove'       => 'zrügg verschiebe',
'selfmove'         => 'Der nöi Artikelname mues en andere sy als der alt!',

# Export
'export'     => 'Sytenen exportiere',
'exporttext' => 'Sie können den Text und die Bearbeitungshistorie einer bestimmten oder einer Auswahl von Seiten nach XML exportieren. Das Ergebnis kann in ein anderes Wiki mit Mediawiki Software eingespielt werden, bearbeitet oder archiviert werden.',

# Namespace 8 related
'allmessages'               => 'Systemnochrichte',
'allmessagesname'           => 'Name',
'allmessagesdefault'        => 'Standard-Tekscht',
'allmessagescurrent'        => 'jetzige Tekscht',
'allmessagestext'           => 'Sell isch e Lischte vo alle mögliche Systemnochrichte ussem MediaWiki Namensruum.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:Allmessages''' cha nit bruucht wärde will '''\$wgUseDatabaseMessages''' abgschalte isch.",
'allmessagesfilter'         => 'Nochrichte nochem Name filtere:',
'allmessagesmodified'       => 'numme gänderti aazeige',

# Thumbnails
'thumbnail-more'  => 'vergrösere',
'thumbnail_error' => "Fähler bir Härstellig vo're Vorschou: $1",

# Special:Import
'importtext' => 'Bitte speichere Si selli Syte vom Quellwiki met em Special:Export Wärkzüg ab un lade Si denn di Datei denn do uffe.',

# Import log
'importlogpage' => 'Import-Logbuech',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Myni Benutzersyte',
'tooltip-pt-mytalk'               => 'Myni Diskussionssyte',
'tooltip-pt-preferences'          => 'Myni Ystellige',
'tooltip-pt-watchlist'            => 'Lischte vo de beobachtete Syte.',
'tooltip-pt-mycontris'            => 'Lischte vo myne Byträg',
'tooltip-pt-login'                => 'Ylogge',
'tooltip-pt-logout'               => 'Uslogge',
'tooltip-ca-talk'                 => 'Diskussion zum Artikelinhalt',
'tooltip-ca-edit'                 => 'Syte bearbeite. Bitte vor em Spychere d Vorschou aaluege.',
'tooltip-ca-addsection'           => 'E Kommentar zu dere Syte derzuetue.',
'tooltip-ca-viewsource'           => 'Die Syte isch geschützt. Du chasch der Quelltext aaluege.',
'tooltip-ca-history'              => 'Früecheri Versione vo dere Syte.',
'tooltip-ca-protect'              => 'Seite beschütze',
'tooltip-ca-delete'               => 'Syten entsorge',
'tooltip-ca-undelete'             => 'Sodeli, da isch es wider.',
'tooltip-ca-move'                 => 'Dür ds Verschiebe gits e nöie Name.',
'tooltip-ca-watch'                => 'Tue die Syten uf dyni Beobachtigslischte.',
'tooltip-ca-unwatch'              => 'Nim die Syte us dyre Beobachtungslischte furt.',
'tooltip-search'                  => 'Dürchsuech {{SITENAME}}',
'tooltip-p-logo'                  => 'Houptsyte',
'tooltip-n-mainpage'              => 'Gang uf d Houptsyte',
'tooltip-n-portal'                => 'Über ds Projekt, was du chasch mache, wo du was findsch',
'tooltip-n-currentevents'         => 'Hindergrundinformatione zu aktuellen Ereignis finde',
'tooltip-n-recentchanges'         => 'Lischte vo de letschten Änderige i däm Wiki.',
'tooltip-n-randompage'            => 'E zuefälligi Syte',
'tooltip-n-help'                  => 'Ds Ort zum Usefinde.',
'tooltip-t-whatlinkshere'         => 'Lischte vo allne Sytene, wo do ane linke',
'tooltip-t-recentchangeslinked'   => 'Letschti Änderige vo de Syte, wo vo do verlinkt sin',
'tooltip-feed-rss'                => 'RSS-Feed für selli Syte',
'tooltip-feed-atom'               => 'Atom-Feed für selli Syte',
'tooltip-t-contributions'         => 'Lischte vo de Byträg vo däm Benutzer',
'tooltip-t-emailuser'             => 'Schick däm Benutzer e E-Bost',
'tooltip-t-upload'                => 'Dateien ufelade',
'tooltip-t-specialpages'          => 'Lischte vo allne Spezialsyte',
'tooltip-ca-nstab-main'           => 'Artikelinhalt aaluege',
'tooltip-ca-nstab-user'           => 'Benutzersyte aaluege',
'tooltip-ca-nstab-media'          => 'Mediasyte aaluege',
'tooltip-ca-nstab-special'        => 'Sell isch e Spezialsyte, du chasch se nid bearbeite.',
'tooltip-ca-nstab-project'        => 'D Projektsyte aaluege',
'tooltip-ca-nstab-image'          => 'Die Bildsyten aaluege',
'tooltip-ca-nstab-mediawiki'      => 'D Systemmäldige aaluege',
'tooltip-ca-nstab-template'       => 'D Vorlag aaluege',
'tooltip-ca-nstab-help'           => 'D Hilfssyten aaluege',
'tooltip-ca-nstab-category'       => 'D Kategoryesyten aaluege',
'tooltip-minoredit'               => 'Die Änderig als chly markiere.',
'tooltip-save'                    => 'Änderige spychere',
'tooltip-preview'                 => 'Vorschou vo dynen Änderige. Bitte vor em Spycheren aluege!',
'tooltip-diff'                    => 'Zeigt a, was du am Tekscht hesch veränderet.',
'tooltip-compareselectedversions' => 'Underschide zwüsche zwo usgwählte Versione vo dere Syten azeige.',
'tooltip-watch'                   => 'Tue die Syten uf dyni Beobachtigslischte.',

# Attribution
'anonymous'        => 'Anonyme Benutzer uff {{SITENAME}}',
'lastmodifiedatby' => 'Diese Seite wurde zuletzt geändert um $2, $1 von $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Basiert auf der Arbeit von $1.',

# Spam protection
'spamprotectiontitle' => 'Spamschutz-Filter',

# Math options
'mw_math_png'    => 'Immer als PNG aazeige',
'mw_math_simple' => 'Eifachs TeX als HTML aazeige, süsch als PNG',
'mw_math_html'   => 'Falls müglech als HTML aazeige, süsch als PNG',
'mw_math_source' => 'Als TeX la sy (für Tekschtbrowser)',
'mw_math_modern' => 'Empfolnigi Ystellig für modärni Browser',

# Patrolling
'markaspatrolleddiff'   => 'Als geprüft markiere',
'markaspatrolledtext'   => 'Den Artikel als geprüft markiere',
'markedaspatrolledtext' => 'Die usgwählte Artikeländerung isch als geprüft markiert worre.',

# Browsing diffs
'previousdiff' => '← Vorderi Änderig',
'nextdiff'     => 'Nächschti Änderig →',

# Media information
'mediawarning'         => '
===Warnung!===
Diese Art von Datei kann böswilligen Programmcode enthalten.
Durch das Herunterladen oder Öffnen der Datei kann der Computer beschädigt werden.
Bereits das Anklicken des Links kann dazu führen dass der Browser die Datei öffnet
und unbekannter Programmcode zur Ausführung kommt.

Die Betreiber dieses Wikis können keine Verantwortung für den Inhalte
dieser Datei übernehmen. Sollte diese Datei tatsächlich böswilligen Programmcode enthalten,
sollte umgehend ein Administrator informiert werden!',
'imagemaxsize'         => 'Maximali Gröössi vo de Bilder uf de Bildbeschrybigs-Sytene:',
'thumbsize'            => 'Bildvorschou-Gröössi:',
'file-info-size'       => '($1 × $2 Pixel, Dateigrößi: $3, MIME-Typ: $4)',
'file-nohires'         => '<small>Kei höcheri Uflösig verfüegbar.</small>',
'svg-long-desc'        => '(SVG-Datei, Basisgrößi: $1 × $2 Pixel, Dateigrößi: $3)',
'show-big-image'       => 'Originalgrößi',
'show-big-image-thumb' => '<small>Größi vo dere Vorschou: $1 × $2 Pixel</small>',

# Special:NewImages
'newimages'     => 'Gallery vo noie Bilder',
'imagelisttext' => "Hie isch e Lischte vo '''$1''' {{PLURAL:$1|Datei|Dateie}}, sortiert $2.",
'ilsubmit'      => 'Suech',

# Bad image list
'bad_image_list' => 'Format:

Nume Zylene, wo mit emne * afö, wärde berücksichtigt.
Nach em * mues zersch e Link zuren Unerwünschte Datei cho.
Wyteri Links uf der glyche Zyle wärden als Usnahme behandlet, wo die Datei trotzdäm darff vorcho.',

# Metadata
'metadata'          => 'Metadate',
'metadata-help'     => "Die Datei het wyteri Informatione, allwäg vor Digitalkamera oder vom Scanner wo se het gschaffe.
We die Datei isch veränderet worde, de cha's sy, das die zuesätzlechi Informatin für di verändereti Datei nümm richtig zuetrifft.",
'metadata-expand'   => 'Erwytereti Details azeige',
'metadata-collapse' => 'Erwytereti Details verstecke',
'metadata-fields'   => 'Die EXIF-Metadate wärden ir Bildbeschrybig ou denn azeigt, we d Metadate-Tabälle versteckt isch.
Anderi Metadate sy standardmäßig versteckt.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-orientation'     => 'Orientierung',
'exif-pixelxdimension' => 'Valind image height',
'exif-fnumber'         => 'F-Wert',
'exif-isospeedratings' => 'Filmempfindlichkeit (ISO)',

# External editor support
'edit-externally'      => 'Die Datei mit emnen externe Programm bearbeite',
'edit-externally-help' => 'Siehe [http://meta.wikimedia.org/wiki/Hilfe:Externe_Editoren Installations-Anweisungen] für weitere Informationen',

# 'all' in various places, this might be different for inflected languages
'watchlistall2' => 'alli',
'namespacesall' => 'alli',
'monthsall'     => 'alli',

# E-mail address confirmation
'confirmemail'          => 'Bschtätigung vo Ihre E-Bost-Adräss',
'confirmemail_text'     => 'Dermit du di erwyterete Mailfunktione chasch bruuche, muesch du die e-Mail-Adrässe, wo du hesch aaggä, la bestätige. Klick ufe Chnopf unte; das schickt dir es Mail. I däm Mail isch e Link; we du däm Link folgsch, de tuesch dadermit bestätige, das die e-Mail-Adrässe dyni isch.',
'confirmemail_send'     => 'Bestätigungs-Mail verschicke',
'confirmemail_sent'     => 'Es isch dir es Mail zur Adrässbestätigung gschickt worde.',
'confirmemail_success'  => 'Dyni e-Mail-Adrässen isch bestätiget worde. Du chasch di jitz ylogge.',
'confirmemail_loggedin' => 'Dyni e-Mail-Adrässen isch jitz bestätiget.',
'confirmemail_subject'  => '{{SITENAME}} e-Mail-Adrässbestätigung',
'confirmemail_body'     => "Hallo

{{SITENAME}}-BenutzerIn «$2» — das bisch allwäg du — het sech vor IP-Adrässen $1 uus mit deren e-Mail-Adrässe bi {{SITENAME}} aagmäldet.

Für z bestätige, das die Adrässe würklech dir isch, u für dyni erwytereten e-Mail-Funktionen uf {{SITENAME}} yzschalte, tue bitte der folgend Link i dym Browser uuf:

$3

Falls du *nid* $2 sötsch sy, de tue bitte de  Link unte dra uf um d'e-Mail-Bestätigung abzbreche:

$5

De Bestätigung Code isch gültug bis $4.

Fründtlechi Grüess",

# action=purge
'confirm_purge' => "Die Zwischeschpoicherung vo der Syte „{{FULLPAGENAME}}“ lösche?

\$1

<div style=\"font-size: 95%; margin-top: 2em;\">
'''''Erklärig:'''''

''Zwüschespycherige (Cache) sy temporäri Kopye vore Websyten uf dym Computer. We ne Syte us em Zwüschespycher abgrüefft wird, de bruucht das weniger Rächeleischtig füre {{SITENAME}}-Server als en Abrueff vor Originalsyte.''

''Falls du e Syte scho nes Wyli am Aaluege bisch, de het dy Computer sone Zwüschespycherig gmacht. Derby chönnt die Syten unter Umständ scho i dere Zyt liecht veraltere.''

''Ds Lösche vor Zwüschespycherig zwingt der Server, dir di aktuellschti Version vor Syte z gä!''
</div>",

# Multipage image navigation
'imgmultipageprev' => '← vorderi Syte',

# Table pager
'table_pager_prev' => 'Vorderi Syte',

# Watchlist editing tools
'watchlisttools-view' => 'Beobachtigsliste: Änderige',
'watchlisttools-edit' => 'normal bearbeite',
'watchlisttools-raw'  => 'imene große Textfäld bearbeite',

# Special:Version
'version' => 'Version', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Spezialsytene',

);
