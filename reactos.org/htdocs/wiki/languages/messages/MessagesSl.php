<?php
/** Slovenian (Slovenščina)
 *
 * @ingroup Language
 * @file
 *
 * @author XJamRastafire
 * @author romanm
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Posebno',
	NS_MAIN           => '',
	NS_TALK           => 'Pogovor',
	NS_USER           => 'Uporabnik',
	NS_USER_TALK      => 'Uporabniški_pogovor',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Pogovor_{{grammar:mestnik|$1}}',
	NS_IMAGE          => 'Slika',
	NS_IMAGE_TALK     => 'Pogovor_o_sliki',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Pogovor_o_MediaWiki',
	NS_TEMPLATE       => 'Predloga',
	NS_TEMPLATE_TALK  => 'Pogovor_o_predlogi',
	NS_HELP           => 'Pomoč',
	NS_HELP_TALK      => 'Pogovor_o_pomoči',
	NS_CATEGORY       => 'Kategorija',
	NS_CATEGORY_TALK  => 'Pogovor_o_kategoriji'
);

$datePreferences = array(
	'default',
	'dmy short',
	'dmy full',
	'ISO 8601',
);

/**
 * The date format to use for generated dates in the user interface.
 * This may be one of the above date preferences, or the special value
 * "dmy or mdy", which uses mdy if $wgAmericanDates is true, and dmy
 * if $wgAmericanDates is false.
 */
$defaultDateFormat = 'dmy full';

$dateFormats = array(
	'dmy short time' => 'H:i',
	'dmy short date' => 'j. F Y',
	'dmy short both' => 'H:i, j. M Y',

	'dmy full time' => 'H:i',
	'dmy full date' => 'j. F Y',
	'dmy full both' => 'H:i, j. F Y',
);

$fallback8bitEncoding = "iso-8859-2";
$separatorTransformTable = array(',' => '.', '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Podčrtavanje povezav:',
'tog-highlightbroken'         => 'Oblikuj pretrgane povezave <a href="" class="new">kot</a> (druga možnost: kot<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Poravnavaj odstavke',
'tog-hideminor'               => 'Skrij manjše popravke v zadnjih spremembah',
'tog-extendwatchlist'         => 'Izboljšan spisek nadzorov',
'tog-usenewrc'                => 'Izboljšane zadnje spremembe (ni za vse brskalnike)',
'tog-numberheadings'          => 'Samodejno številči poglavja',
'tog-showtoolbar'             => 'Prikaži urejevalno orodno vrstico',
'tog-editondblclick'          => 'Omogoči urejanje strani z dvojnim klikom (JavaScript)',
'tog-editsection'             => 'Omogoči urejanje delov prek povezav [spremeni]',
'tog-editsectiononrightclick' => 'Omogoči urejanje delov z desnim klikanjem naslovov delov (JavaScript)',
'tog-showtoc'                 => 'Prikaži vsebino (strani z več kot tremi naslovi)',
'tog-rememberpassword'        => 'Geslo si zapomni skozi vse seje',
'tog-editwidth'               => 'Urejevalno polje naj ima vso širino',
'tog-watchcreations'          => 'Vse ustvarjene strani dodaj na spisek nadzorov',
'tog-watchdefault'            => 'Dodaj na spisek nadzorov vse članke, ki sem jih ustvaril/-a ali spremenil/-a',
'tog-minordefault'            => 'Vsa urejanja označi kot manjša',
'tog-previewontop'            => 'Prikaži predogled pred urejevalnim poljem in ne za njim',
'tog-previewonfirst'          => 'Ob začetku urejanja prikaži predogled',
'tog-nocache'                 => 'Onemogoči predpomnenje strani',
'tog-enotifwatchlistpages'    => 'Ob spremembah strani mi pošlji e-pošto',
'tog-enotifusertalkpages'     => 'Pošlji e-pošto ob spremembah moje pogovorne strani',
'tog-enotifminoredits'        => 'Pošlji e-pošto tudi za manjše spremembe strani',
'tog-enotifrevealaddr'        => 'V sporočilih z obvestili o spremembah razkrij moj e-poštni naslov',
'tog-shownumberswatching'     => 'Prikaži število uporabnikov, ki spremljajo temo',
'tog-fancysig'                => 'Surovi podpisi (brez samodejne povezave; <b>ne</b> uporabljajte <b>predlog</b> ali <b>zunanjih povezav</b>)',
'tog-externaleditor'          => 'Po privzetem uporabljaj zunanji urejevalnik',
'tog-externaldiff'            => 'Po privzetem uporabljaj zunanje primerjanje',
'tog-showjumplinks'           => 'Prikaži pomožni povezavi »Skoči na«',
'tog-uselivepreview'          => 'Uporabi hitri predogled (JavaScript) (preizkusno)',
'tog-forceeditsummary'        => 'Ob vpisu praznega povzetka urejanja me opozori',
'tog-watchlisthideown'        => 'Na spisku nadzorov skrij moja urejanja',
'tog-watchlisthidebots'       => 'Na spisku nadzorov skrij urejanja botov',

'underline-always'  => 'Vedno',
'underline-never'   => 'Nikoli',
'underline-default' => 'Privzeto (brskalnik)',

'skinpreview' => '(Predogled)',

# Dates
'sunday'        => 'nedelja',
'monday'        => 'ponedeljek',
'tuesday'       => 'torek',
'wednesday'     => 'sreda',
'thursday'      => 'četrtek',
'friday'        => 'petek',
'saturday'      => 'sobota',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'marec',
'april'         => 'april',
'may_long'      => 'maj',
'june'          => 'junij',
'july'          => 'julij',
'august'        => 'avgust',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'december',
'january-gen'   => 'januarja',
'february-gen'  => 'februarja',
'march-gen'     => 'marca',
'april-gen'     => 'aprila',
'may-gen'       => 'maja',
'june-gen'      => 'junija',
'july-gen'      => 'julija',
'august-gen'    => 'avgusta',
'september-gen' => 'septembra',
'october-gen'   => 'oktobra',
'november-gen'  => 'novembra',
'december-gen'  => 'decembra',
'jan'           => 'jan.',
'feb'           => 'feb.',
'mar'           => 'mar.',
'apr'           => 'apr.',
'may'           => 'maj',
'jun'           => 'jun.',
'jul'           => 'jul.',
'aug'           => 'avg.',
'sep'           => 'sep.',
'oct'           => 'okt.',
'nov'           => 'nov.',
'dec'           => 'dec.',

# Categories related messages
'pagecategories'         => '{{PLURAL:$1|Kategorija|Kategoriji|Kategorije|Kategorije|Kategorije}}',
'category_header'        => 'Strani v kategoriji »$1«',
'subcategories'          => 'Podkategorije',
'listingcontinuesabbrev' => 'nadalj.',

'mainpagetext'      => 'Wikiprogramje ste uspešno naložili!',
'mainpagedocfooter' => 'Za uporabo in pomoč pri nastavitvi, prosimo, preglejte [http://meta.wikimedia.org/wiki/MediaWiki_localisation dokumentacijo za prilagajanje vmesnika]
in [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide Uporabniški priročnik].',

'about'          => 'O projektu',
'article'        => 'Članek',
'newwindow'      => '(odpre se novo okno)',
'cancel'         => 'Prekliči',
'qbfind'         => 'Poišči',
'qbbrowse'       => 'Prebrskaj',
'qbedit'         => 'Uredi',
'qbpageoptions'  => 'Možnosti strani',
'qbpageinfo'     => 'Podatki o strani',
'qbmyoptions'    => 'Moje možnosti',
'qbspecialpages' => 'Posebne strani',
'moredotdotdot'  => 'Več ...',
'mypage'         => 'Moja stran',
'mytalk'         => 'Pogovor',
'anontalk'       => 'Pogovorna stran IP',
'navigation'     => 'Navigacija',
'and'            => 'in',

# Metadata in edit box
'metadata_help' => 'Metapodatki:',

'errorpagetitle'    => 'Napaka',
'returnto'          => 'Vrnitev na: $1.',
'tagline'           => 'Iz {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'help'              => 'Pomoč',
'search'            => 'Iskanje',
'searchbutton'      => 'Iskanje',
'go'                => 'Pojdi na',
'searcharticle'     => 'Pojdi na',
'history'           => 'Zgodovina strani',
'history_short'     => 'Zgodovina strani',
'updatedmarker'     => 'Posodobljeno od mojega zadnjega obiska',
'info_short'        => 'Sporočilo',
'printableversion'  => 'Različica za tisk',
'permalink'         => 'Trajna povezava',
'print'             => 'Tisk',
'edit'              => 'Uredite stran',
'editthispage'      => 'Uredi stran',
'delete'            => 'Briši',
'deletethispage'    => 'Briši stran',
'undelete_short'    => 'Vrni $1 {{PLURAL:$1|izbrisano urejanje|izbrisani urejanji|izbrisana urejanja|izbrisanih urejanj|izbrisanih urejanj}}',
'protect'           => 'Zaščiti',
'protectthispage'   => 'Zaščiti stran',
'unprotect'         => 'Odstrani zaščito',
'unprotectthispage' => 'Odstrani zaščito strani',
'newpage'           => 'Nova stran',
'talkpage'          => 'Pogovorite se o strani',
'specialpage'       => 'Posebna stran',
'personaltools'     => 'Osebna orodja',
'postcomment'       => 'Objavite pripombo',
'articlepage'       => 'Prikaže članek',
'talk'              => 'Pogovor',
'views'             => 'Pogled',
'toolbox'           => 'Pripomočki',
'userpage'          => 'Prikaži uporabnikovo stran',
'projectpage'       => 'Prikaži projektno stran',
'imagepage'         => 'Prikaže stran z datoteko',
'viewtalkpage'      => '< Pogovor',
'otherlanguages'    => 'V drugih jezikih',
'redirectedfrom'    => '(Preusmerjeno z $1)',
'redirectpagesub'   => 'Preusmeritvena stran',
'lastmodifiedat'    => 'Čas zadnje spremembe: $2, $1.', # $1 date, $2 time
'viewcount'         => 'Stran je bila naložena $1-krat.',
'protectedpage'     => 'Zaščitena stran',
'jumpto'            => 'Skoči na:',
'jumptonavigation'  => 'navigacija',
'jumptosearch'      => 'iskanje',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{UCFIRST:{{GRAMMAR:orodnik|{{SITENAME}}}}}}',
'aboutpage'            => 'Project:{{UCFIRST:{{GRAMMAR:orodnik|{{SITENAME}}}}}}',
'bugreports'           => 'Poročila o hroščih',
'bugreportspage'       => 'Project:Poročila o hroščih',
'copyright'            => 'Besedilo je na razpolago pod pogoji $1.',
'copyrightpagename'    => 'Avtorske pravice {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'copyrightpage'        => '{{ns:project}}:Avtorske pravice {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'currentevents'        => 'Trenutni dogodki',
'currentevents-url'    => 'Project:Trenutni dogodki',
'disclaimers'          => 'Zanikanja odgovornosti',
'disclaimerpage'       => 'Project:Splošno_zanikanje_odgovornosti',
'edithelp'             => 'Pomoč pri urejanju',
'edithelppage'         => 'Help:Urejanje slovenskih strani',
'faq'                  => 'Najpogostejša vprašanja',
'faqpage'              => 'Project:Najpogostejša vprašanja',
'helppage'             => 'Help:Vsebina',
'mainpage'             => 'Glavna stran',
'mainpage-description' => 'Glavna stran',
'portal'               => 'Portal občestva',
'portal-url'           => 'Project:Portal občestva',
'privacy'              => 'Politika zasebnosti',

'badaccess' => 'Napaka pri dovoljenju',

'versionrequired'     => 'Potrebna je različica MediaWiki $1',
'versionrequiredtext' => 'Za uporabo strani je potrebna različica MediaWiki $1. Glejte [[Special:Version]].',

'ok'                      => 'V redu',
'retrievedfrom'           => 'Vzpostavljeno iz »$1«',
'youhavenewmessages'      => 'Imate $1 ($2)',
'newmessageslink'         => 'novo sporočilo',
'newmessagesdifflink'     => 'zadnja sprememba',
'youhavenewmessagesmulti' => 'Na $1 imate novo sporočilo',
'editsection'             => 'spremeni',
'editold'                 => 'spremeni',
'editsectionhint'         => 'Spremeni razdelek: $1',
'toc'                     => 'Vsebina',
'showtoc'                 => 'prikaži',
'hidetoc'                 => 'skrij',
'thisisdeleted'           => 'Prikažem ali vrnem $1?',
'viewdeleted'             => 'Prikažem $1?',
'restorelink'             => '$1 {{PLURAL:$1|izbrisana redakcija|izbrisani redakciji|izbrisane redakcije|izbrisanih redakcij|izbrisanih redakcij}}',
'feedlinks'               => 'Podajanje:',
'feed-invalid'            => 'Neveljavna vrsta naročniškega dovoda.',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Članek',
'nstab-user'      => 'Uporabniška stran',
'nstab-media'     => 'Predstavnostna stran',
'nstab-special'   => 'Posebno',
'nstab-project'   => 'Projektna stran',
'nstab-image'     => 'Datoteka',
'nstab-mediawiki' => 'Sporočilo',
'nstab-template'  => 'Predloga',
'nstab-help'      => 'Pomoč',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Tako dejanje ne obstaja',
'nosuchactiontext'  => 'Dejanja, ki ga je označil spletni naslov, wiki ne prepozna.',
'nosuchspecialpage' => 'Posebna stran, ki ste jo zahtevali, ne obstaja',
'nospecialpagetext' => 'Posebne strani, ki ste jo zahtevali, programje {{GRAMMAR:rodilnik|{{SITENAME}}}} ne prepozna ali pa nimate dostopa do nje. Seznam vseh prepoznanih posebnih strani je na razpolago na strani [[Special:SpecialPages]].',

# General errors
'error'                => 'Napaka',
'databaseerror'        => 'Napaka zbirke podatkov',
'dberrortext'          => 'Iskanje ali drugo želeno dejanje je verjetno zahtevalo preveč časa. Mogoči razlogi so: <ul> <li>Iskanje z vsemi besedami v narekovajih. Poskusite najprej iskati brez njih. Kadar to ni zadosti, vpišite še nekaj iskalnih pojmov ali dodajte besedo ali dve zunaj narekovajev, s čimer boste označili splošno predmetno področje. <li>Zelo obsežen spisek nadzorov (verjetno čez 10.000 strani) ob obremenjenih strežnikih. <li>Morda ste poskušali dejanje izvesti ob začasni izredni obremenitvi strežnikov. Ob izrednih obremenitvah lahko pride do nenadnega končanja številnih pomembnih opravil, da se dosežejo običajni odzivni časi. Če se zahtevano dejanje sicer izvede brez težav, poskusite znova v nekaj minutah. </ul> <p>Splošno sporočilo o napaki je:</p> <p> Prišlo je do napake zbirke podatkov. Vzrok bi lahko bil nesprejemljiv iskalni niz (glejte $5) ali programski hrošč. Zadnje poskušano iskanje:</p> <blockquote><tt>$1</tt></blockquote> znotraj funkcije »<tt>$2</tt>«. MySQL je vrnil napako »<tt>$3: $4</tt>«. <p>Če je zgornja napaka 2013, je vaš postopek končal upravljavec podatkovnega bremena. Prosimo, poskusite z ustreznejšim iskanjem ali iskanje ponovite ob manj obremenjenih strežnikih. Opravičujemo se za nevšečnosti.</p>',
'dberrortextcl'        => 'Pri iskanju v zbirki podatkov je prišlo do skladenjske napake. Zadnje iskanje v zbirki podatkov: »$1« iz funkcije »$2«. MySQL je vrnil napako »$3: $4«.',
'noconnect'            => 'S PB na $1 se ne morem povezati.',
'nodb'                 => "Zbirke podatkov '$1' ne morem izbrati",
'cachederror'          => 'To je shranjen in morda neposodobljen prepis želene strani.',
'laggedslavemode'      => 'Opozorilo: stran morda ne vsebuje najnovejših posodobitev',
'readonly'             => 'Zbirka podatkov je zaklenjena',
'enterlockreason'      => 'Vnesite razlog za zaklenitev in oceno, kdaj bo urejanje spet mogoče',
'readonlytext'         => "Zbirka podatkov je za urejanja in druge spremembe začasno zaklenjena. To navadno pomeni, da nadgrajujejo programje strežnikov ali pa rutinsko vzdrževanje zbirke.

Sistemski skrbnik, ki jo je zaklenil, je podal naslednjo razlago: ''\"\$1\"''",
'readonly_lag'         => 'Podatkovna zbirka se je samodejno zaklenila, dokler se podrejeni strežniki ne uskladijo z glavnim.',
'internalerror'        => 'Notranja napaka',
'filecopyerror'        => 'Datoteke »$1« ni mogoče prepisati v »$2«.',
'filerenameerror'      => 'Datoteke »$1« ni mogoče preimenovati v »$2«.',
'filedeleteerror'      => 'Datoteke »$1« ni mogoče izbrisati.',
'filenotfound'         => 'Datoteke »$1« ne najdem.',
'unexpected'           => 'Nepričakovana vrednost: "$1"="$2".',
'formerror'            => 'Napaka: obrazca ni mogoče predložiti',
'badarticleerror'      => 'Na tej strani dejanja ne morem izvesti. Morda je bila stran med predložitvijo vaše zahteve že izbrisana.',
'cannotdelete'         => 'Navedene strani ali datoteke ni mogoče izbrisati. Morda jo je izbrisal že kdo drug.',
'badtitle'             => 'Nepravilen naslov',
'badtitletext'         => 'Navedeni naslov strani je neveljaven, prazen, napačno povezan k drugim jezikom oziroma wikiprojektom ali pa vsebuje nepodprte znake.',
'perfdisabled'         => 'Naveden je shranjeni prepis.',
'perfcached'           => 'Navedeni podatki morda niso popolnoma posodobljeni.',
'perfcachedts'         => 'Prikazani podatki so shranjeni v predpomnilniku. Čas zadnje osvežitve: $1.',
'wrong_wfQuery_params' => 'Nepravilni parametri za wfQuery()<br />
Funkcija: $1<br />
Iskanje: $2',
'viewsource'           => 'Izvorno besedilo',
'viewsourcefor'        => 'za $1',
'protectedinterface'   => 'Prikazana stran vsebuje besedilo ali drug element uporabniškega vmesnika programja. Zaradi preprečevanja zlorabe je zaščitena.',
'editinginterface'     => "'''Opozorilo:''' Urejate stran, ki vsebuje besedilo ali drug element uporabniškega vmesnika programja.
Spremembe te strani bodo vplivale na podobo uporabniškega vmesnika.
For translations, please consider using [http://translatewiki.net/wiki/Main_Page?setlang=en Betawiki], the MediaWiki localisation project.",
'sqlhidden'            => '(SQL-poizvedovanje je skrito)',

# Login and logout pages
'logouttitle'                => 'Odjava uporabnika',
'logouttext'                 => 'Odjavili ste se. {{GRAMMAR:tožilnik|{{SITENAME}}}} lahko zdaj uporabljate neprijavljeni ali pa se ponovno prijavite. Morda bodo nekatere strani še naprej prikazane, kot da ste prijavljeni. To lahko popravite z izpraznitvijo predpomnilnika.',
'welcomecreation'            => '== Dobrodošli, $1! ==
Ustvarili ste račun.
Če želite, si lahko prilagodite nastavitve za delo v {{GRAMMAR:dajalnik|{{SITENAME}}}}.',
'loginpagetitle'             => 'Prijava uporabnika',
'yourname'                   => 'Uporabniško ime',
'yourpassword'               => 'Geslo',
'yourpasswordagain'          => 'Ponovno vpišite geslo',
'remembermypassword'         => 'Zapomni si me (samodejna prijava)',
'yourdomainname'             => 'Domena',
'externaldberror'            => 'Pri potrjevanju istovetnosti je prišlo do notranje napake ali pa za osveževanje zunanjega računa nimate dovoljenja.',
'loginproblem'               => '<b>Prijava ni uspela.</b><br />Prosimo, poskusite znova!',
'login'                      => 'Prijava',
'loginprompt'                => '<!--Za prijavo v {{GRAMMAR:tožilnik|{{SITENAME}}}} omogočite piškotke.-->',
'userlogin'                  => 'Prijavite se / registrirajte se',
'logout'                     => 'Odjava',
'userlogout'                 => 'Odjava',
'notloggedin'                => 'Niste prijavljeni',
'nologin'                    => 'Še nimate uporabniškega računa? $1!',
'nologinlink'                => 'Registrirajte se',
'createaccount'              => 'Ustvari račun',
'gotaccount'                 => 'Račun že imate? $1.',
'gotaccountlink'             => 'Prijavite se',
'createaccountmail'          => 'Po e-pošti',
'badretype'                  => 'Gesli, ki ste ju vnesli, se ne ujemata.',
'userexists'                 => 'Uporabniško ime, ki ste ga vnesli, je že zasedeno. Prosimo, izberite si drugo.',
'youremail'                  => 'E-pošta (neobvezno)*',
'username'                   => 'Uporabniško ime:',
'uid'                        => 'ID-številka:',
'yourrealname'               => 'Vaše pravo ime*',
'yourlanguage'               => 'Jezik vmesnika:',
'yourvariant'                => 'Jezikovna različica',
'yournick'                   => 'Podpis',
'badsig'                     => 'Neveljaven surovi podpis; preverite oznake HTML.',
'email'                      => 'E-pošta',
'prefs-help-realname'        => '<!-- ¹ Pravo ime (neobvezno): če se odločite, da ga boste navedli, bo uporabljeno za priznavanje vašega dela. -->',
'loginerror'                 => 'Napaka ob prijavi',
'prefs-help-email'           => '<nowiki>*</nowiki>E-pošta (neobvezno): Vpisan e-poštni naslov omogoča drugim, da vam lahko pošiljajo elektronsko pošto brez razkritja vaše istovetnosti.',
'nocookiesnew'               => 'Uporabniški račun je ustvarjen, vendar niste prijavljeni. {{SITENAME}} za prijavo uporabnikov uporablja piškotke, ki pa so pri vas onemogočeni. Prosimo, omogočite jih, nato pa se s svojim uporabniškim imenom in geslom ponovno poskusite prijaviti.',
'nocookieslogin'             => '{{SITENAME}} za prijavljanje uporabnikov uporablja piškotke. Ker jih imate onemogočene, vas prosimo, da jih omogočite in se ponovno prijavite.',
'noname'                     => 'Niste vnesli veljavnega uporabniškega imena.',
'loginsuccesstitle'          => 'Uspešno ste se prijavili',
'loginsuccess'               => 'Sedaj ste vpisani v {{GRAMMAR:tožilnik|{{SITENAME}}}} kot "$1".',
'nosuchuser'                 => 'Uporabnik z imenom »$1« ne obstaja.
Preverite črkovanje ali pa si z uporabo prikazanega obrazca ustvarite nov uporabniški račun.',
'nosuchusershort'            => 'Uporabnik z imenom »<nowiki>$1</nowiki>« ne obstaja. Preverite črkovanje.',
'nouserspecified'            => 'Prosimo, vpišite uporabniško ime.',
'wrongpassword'              => 'Vnesli ste napačno geslo. Prosimo, poskusite znova.',
'wrongpasswordempty'         => 'Vpisali ste prazno geslo. Prosimo, poskusite znova.',
'passwordtooshort'           => 'Vaše geslo je prekratko. Imeti mora najmanj $1 {{PLURAL:$1|znak|znaka|znake|znakov|znakov}}.',
'mailmypassword'             => 'Pošlji mi novo geslo',
'passwordremindertitle'      => 'Geselski opomnik iz {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'passwordremindertext'       => "Nekdo (verjetno vi, z IP-naslova $1)
je zahteval, da vam pošljemo novo prijavno geslo za {{GRAMMAR:tožilnik|{{SITENAME}}}} ($4).
Geslo uporabnika ''$2'' je odslej ''$3''.
Z njim se lahko prijavite in ga spremenite.

Če je geslo zahteval nekdo drug ali ste se spomnili starega in ga ne želite več spremeniti, lahko sporočilo prezrete in se še naprej prijavljate s starim.",
'noemail'                    => 'Elektronska pošta uporabnika »$1« ni zapisana.',
'passwordsent'               => 'Na naslov elektronske pošte, vpisane za "$1", smo poslali novo geslo. Ko ga boste prejeli, se lahko ponovno prijavite.',
'eauthentsent'               => 'E-sporočilo je poslano na navedeni e-naslov. Če želite tja poslati še katero, po v omenjenem sporočilu navedenih navodilih potrdite lastništvo naslova.',
'mailerror'                  => 'Napaka pri pošiljanju pošte: $1',
'acct_creation_throttle_hit' => 'Obiskovalci {{GRAMMAR:rodilnik|{{SITENAME}}}} so s tem [[IP-naslov]]om v zadnjih 24 urah ustvarili že $1 {{PLURAL:$1|uporabniški račun|uporabniška računa|uporabniške račune|uporabniških računov|uporabniških računov}} in s tem dosegli največje dopustno število v omenjenem časovnem obdobju. Novih računov zato s tem IP-naslovom trenutno žal ne morete več ustvariti.

== Urejate prek posredniškega strežnika? ==

Če urejate prek AOL ali iz Bližnjega vzhoda, Afrike, Avstralije, Nove Zelandije ali iz šole, knjižnice ali podjetja, si IP-naslov morda delite z drugimi uporabniki. Če je tako, ste to sporočilo morda prejeli, čeprav niste ustvarili še nobenega računa. Znova se lahko poskusite registrirati po nekaj urah.',
'emailauthenticated'         => 'Čas potrditve vašega e-poštnega naslova: $1',
'emailnotauthenticated'      => 'Vaš e-poštni naslov še ni potrjen. Za navedene
možnosti se e-pošte ne bo pošiljalo.',
'noemailprefs'               => '<strong>E-poštnega naslova niste vnesli</strong>, zato naslednje možnosti ne bodo delovale.',
'emailconfirmlink'           => 'Potrdite svoj e-poštni naslov',
'invalidemailaddress'        => 'E-poštni naslov zaradi neveljavne oblike ni sprejemljiv. Prosimo, vpišite pravilno oblikovanega ali polje izpraznite.',
'accountcreated'             => 'Račun je ustvarjen',
'accountcreatedtext'         => 'Uporabniški račun za »$1« je ustvarjen.',

# Edit page toolbar
'bold_sample'     => 'Krepko besedilo',
'bold_tip'        => 'Krepko besedilo',
'italic_sample'   => 'Ležeče besedilo',
'italic_tip'      => 'Ležeče besedilo',
'link_sample'     => 'Naslov povezave',
'link_tip'        => 'Notranja povezava',
'extlink_sample'  => 'http://www.example.com naslov povezave',
'extlink_tip'     => 'Zunanja povezava (ne pozabite na predpono http://)',
'headline_sample' => 'Besedilo naslovne vrstice',
'headline_tip'    => 'Naslovna vrstica druge ravni',
'math_sample'     => 'Tu vnesite enačbo',
'math_tip'        => 'Matematična enačba (TeX/LaTeX)',
'nowiki_sample'   => 'Tu vnesite neoblikovano besedilo',
'nowiki_tip'      => 'Prezri wikioblikovanje',
'image_sample'    => 'Zgled.jpg',
'image_tip'       => 'Povezava na sliko',
'media_sample'    => 'Zgled.ogg',
'media_tip'       => 'Povezava na predstavnostno datoteko',
'sig_tip'         => 'Vaš podpis z datumom',
'hr_tip'          => 'Vodoravna črta (uporabljajte zmerno)',

# Edit pages
'summary'                   => 'Povzetek urejanja',
'subject'                   => 'Tema/naslov',
'minoredit'                 => 'Manjše urejanje',
'watchthis'                 => 'Opazuj članek',
'savearticle'               => 'Shrani stran',
'preview'                   => 'Predogled',
'showpreview'               => 'Prikaži predogled',
'showlivepreview'           => 'Predogled v živo',
'showdiff'                  => 'Prikaži spremembe',
'anoneditwarning'           => "'''Opozorilo''': niste prijavljeni. V zgodovino strani se bo zapisal vaš IP-naslov.",
'missingsummary'            => "'''Opozorilo:''' Niste napisali povzetka urejanja. Ob ponovnem kliku gumba ''Shrani'' se bo vaše urejanje shranilo brez njega.",
'missingcommenttext'        => 'Prosimo, vpišite v spodnje polje komentar.',
'blockedtitle'              => 'Uporabnik je blokiran.',
'blockedtext'               => "Urejanje z vašim uporabniškim imenom oziroma IP-naslovom je $1 onemogočil(-a).
Vzrok:<br />''$2''<br />O blokiranju se lahko pogovorite z $1 ali katerim drugim
[[{{MediaWiki:Grouppage-sysop}}|administratorjem]].

Vedite, da lahko ukaz »Pošlji uporabniku e-pismo« uporabite le, če ste v nastavitvah vpisali in potrdili svoj elektronski naslov. 

Vaš IP-naslov je $3. Prosimo, vključite ga v vse morebitne poizvedbe.",
'blockedoriginalsource'     => "Izvorno besedilo strani '''$1''' je na razpolago spodaj:",
'blockededitsource'         => "Besedilo '''vaših urejanj''' strani '''$1''' je prikazano spodaj:",
'whitelistedittitle'        => 'Za urejanje se morate prijaviti',
'whitelistedittext'         => 'Za urejanje strani se $1.',
'confirmedittitle'          => 'Za urejanje je potrebna e-poštna potrditev',
'confirmedittext'           => 'Pred urejanjem strani morate potrditi svoj e-poštni naslov. Prosimo, da ga z uporabo [[Special:Preferences|uporabniških nastavitev]] vpišete in potrdite.',
'loginreqtitle'             => 'Treba se je prijaviti',
'loginreqlink'              => 'prijava',
'loginreqpagetext'          => 'Za ogled drugih strani morate $1.',
'accmailtitle'              => 'Geslo je poslano.',
'accmailtext'               => "Geslo za '$1' je poslano na $2.",
'newarticle'                => '(Nov)',
'newarticletext'            => "Sledili ste povezavi na stran, ki še ne obstaja.
Da bi stran ustvarili, vnesite v spodnji obrazec besedilo
(za več informacij glej [[{{MediaWiki:Helppage}}|pomoč]]).
Če ste sem prišli po pomoti, v svojem brskalniku kliknite gumb ''Nazaj''.",
'anontalkpagetext'          => "---- ''To je pogovorna stran za nepodpisanega uporabnika, ki še ni ustvaril računa ali, ki ga ne uporablja. Zaradi tega moramo uporabiti števčen IP address za njegovo/njeno ugotavljanje istovetnosti. Takšen IP naslov si lahko deli več uporabnikov. Če ste nepodpisan uporabnik in če menite, da so nepomembne pripombe namenjene vam, prosimo [[Special:UserLogin|ustvarite račun ali pa se vpišite]], da preprečite naslednje zmede z drugimi nepodpisanimi uporabniki.''",
'noarticletext'             => 'Na tej strani ni trenutno nobenega besedila. Naslov strani lahko poskusite [[Special:Search/{{PAGENAME}}|poiskati]] na drugih straneh ali pa [{{fullurl:{{FULLPAGENAME}}|action=edit}} stran uredite].',
'clearyourcache'            => "'''Opomba:''' Da bodo spremembe prišle do veljave, po shranitvi izpraznite predpomnilnik svojega brskalnika: '''Mozilla/Safari:''' držite ''Shift'' in kliknite ''Reload'' (ali pritisnite ''Ctrl-Shift-R''), '''Internet Explorer:''' ''Ctrl-F5'', '''Opera/Konqueror:''' ''F5''.",
'usercssjsyoucanpreview'    => "<strong>Nasvet:</strong> Za preizkušanje svojega novega CSS/JS pred shranjevanjem uporabite gumb ''Prikaži predogled''.",
'usercsspreview'            => "'''OPOZORILO: svoj uporabniški CSS le predogledujete in ga še niste shranili!'''",
'userjspreview'             => "'''Svoj uporabniški Javascript le predogledujete in še ni shranjen!'''",
'userinvalidcssjstitle'     => "'''Opozorilo:''' koža »$1« ne obstaja. Vedite, da .css in .js strani po meri uporabljajo naslov z malo začetnico, npr. {{ns:user}}:Blabla/monobook.css namesto {{ns:user}}:Blabla/Monobook.css.",
'updated'                   => '(Posodobljeno)',
'note'                      => '<strong>Opomba:</strong>',
'previewnote'               => '<strong>Stran le predogledujete in še ni shranjena!</strong>',
'previewconflict'           => 'V prikazanem predogledu je v zgornjem predelu urejanja navedeno besedilo, kakor se bo prikazalo, če ga boste shranili.',
'session_fail_preview'      => '<strong>Zaradi izgube podatkov o seji nam vašega urejanja žal ni uspelo obdelati. Prosimo, poskusite znova. Če bo spet prišlo do napake, se odjavite in ponovno prijavite. Za nevšečnosti se opravičujemo.</strong>',
'session_fail_preview_html' => "<strong>Zaradi izgube podatkov o seji nam vašega urejanja žal ni uspelo obdelati.</strong>

''Ker ima ta wiki vklopljen surovi HTML, je predogled zaradi preprečevanja napadov z JavaScriptom skrit.''

<strong>Če gre za dobronameren poskus urejanja, vas prosimo, da poskusite znova. Če bo spet prišlo do napake, se odjavite in ponovno prijavite. Za nevšečnosti se opravičujemo.</strong>",
'editing'                   => 'Urejanje $1',
'editingsection'            => 'Urejanje $1 (razdelek)',
'editingcomment'            => 'Urejanje $1 (pripomba)',
'editconflict'              => 'Navzkrižje urejanj: $1',
'explainconflict'           => 'Med vašim urejanjem je stran spremenil nekdo drug. Zgornje besedilno območje vsebuje njeno trenutno vsebino in bo edino, ki se bo ob izbiri ukaza »Shrani stran« shranilo. V spodnjem območju so prikazane vaše spremembe, ki jih boste morali vključiti v zgornje.<br />',
'yourtext'                  => 'Vaše besedilo',
'storedversion'             => 'Shranjena različica',
'nonunicodebrowser'         => '<strong>OPOMBA</strong>: Vaš brskalnik ne podpira Unicode, zato boste pri urejanju strani z nelatiničnimi znaki morda imeli težave. Za obhod te težave se bodo <strong>ne-ASCII-znaki v urejevalnem polju spodaj pojavili kot šestnajstiške kode</strong>.',
'editingold'                => '<strong>Urejate staro redakcijo strani.
Če jo boste shranili, bodo vse poznejše spremembe razveljavljene.</strong>',
'yourdiff'                  => 'Primerjava',
'copyrightwarning'          => 'Vsi prispevki k {{GRAMMAR:dajalnik|{{SITENAME}}}} se obravnavajo kot objave pod pogoji $2 (za podrobnosti glej $1). Če niste pripravljeni na neusmiljeno urejanje in prosto razširjanje vašega gradiva, ga ne prispevajte.

Poleg tega zagotavljate, da ste prispevke napisali oziroma ustvarili sami ali pa prepisali iz javno dostopnega ali podobnega prostega vira oziroma da pri tem ne kršite avtorskih pravic.
<strong>NE DODAJAJTE AVTORSKO ZAŠČITENEGA DELA BREZ DOVOLJENJA !</strong>',
'copyrightwarning2'         => 'Vsi prispevki k {{GRAMMAR:dajalnik|{{SITENAME}}}} se lahko urejajo, spreminjajo ali odstranijo s strani drugih uporabnikov. Če niste pripravljeni na neusmiljeno urejanje in prosto razširjanje vašega gradiva, ga ne prispevajte.

Poleg tega zagotavljate, da ste prispevke napisali oziroma ustvarili sami ali pa prepisali iz javno dostopnega ali podobnega prostega vira oziroma da pri tem ne kršite avtorskih pravic ($1).
<strong>NE DODAJAJTE AVTORSKO ZAŠČITENEGA DELA BREZ DOVOLJENJA !</strong>',
'longpagewarning'           => 'Stran je dolga $1 {{PLURAL:$1|kilobajt|kilobajta|kilobajte|kilobajtov|kilobajtov}}. To je morda več, kot bi želeli, zato premislite o razdelitvi na podstrani oziroma arhiviranju.',
'longpageerror'             => '<strong>NAPAKA: Predloženo besedilo je dolgo $1 {{PLURAL:$1|kilobajt|kilobajta|kilobajte|kilobajtov|kilobajtov}}, s čimer presega največjo dovoljeno dolžino $2 {{PLURAL:$2|kilobajta|kilobajtov|kilobajtov|kilobajtov|kilobajtov}}. Zato ga žal ni mogoče shraniti.</strong>',
'readonlywarning'           => '<strong>OPOZORILO: Zbirka podatkov je zaradi vzdrževanja začasno  zaklenjena, kar pomeni, da sprememb ne morete shraniti. Prosimo, prenesite besedilo v urejevalnik in ga dodajte pozneje.</strong>',
'protectedpagewarning'      => '<strong>OPOMBA:</strong> Stran je zaklenjena in jo lahko urejajo le sodelavci z vzdrževalnimi pravicami. Pri urejanju sledite [[Project:Smernice_zaščitenih_strani|smernicam zaščitenih strani]].',
'semiprotectedpagewarning'  => "'''Opomba:''' Stran je [[Project:Delna zaščita|zaščitena]] in jo lahko urejajo le uveljavljeni uporabniki.",
'templatesused'             => 'Na strani uporabljene predloge:',
'edittools'                 => '<!-- To besedilo bo prikazano pod urejevalnim poljem in poljem za nalaganje. -->',
'nocreatetitle'             => 'Članka nisem našel',
'nocreatetext'              => "Članka s tem naslovom - '''{{PAGENAME}}''' - še nimamo, lahko pa ga [[Special:Search/{{PAGENAME}}|poskusite poiskati]] ali pa se [[Special:UserLogin|prijavite]] in ga ustvarite.",

# History pages
'nohistory'           => 'Stran nima zgodovine urejanja.',
'revnotfound'         => 'Redakcije ne najdem',
'revnotfoundtext'     => 'Redakcije strani, ki ste jo poskušali pridobiti, ni mogoče najti. Prosimo, preverite spletni naslov, ki ste ga uporabili za dostop do strani.',
'currentrev'          => 'Trenutna redakcija',
'revisionasof'        => 'Redakcija: $1',
'previousrevision'    => '← Starejša redakcija',
'nextrevision'        => 'Novejša redakcija →',
'currentrevisionlink' => 'poglejte trenutno redakcijo',
'cur'                 => 'tren',
'next'                => 'nasl',
'last'                => 'prej',
'histlegend'          => 'Za ogled redakcije kliknite njen datum.

Napotek: (tren) = primerjava s trenutno redakcijo,
(prej) = primerjava s prejšnjo redakcijo, <b>m</b> = manjše urejanje',
'deletedrev'          => '[izbrisano]',
'histfirst'           => 'Najstarejše',
'histlast'            => 'Najnovejše',

# Revision feed
'history-feed-title'          => 'Zgodovina strani',
'history-feed-description'    => 'Zgodovina navedene strani {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'history-feed-item-nocomment' => '$1 ob $2', # user at time
'history-feed-empty'          => 'Želena stran ne obstaja. Morda je bila iz {{GRAMMAR:rodilnik|{{SITENAME}}}} izbrisana ali pa jo je kdo preimenoval. Prosimo, poskusite v {{GRAMMAR:dajalnik|{{SITENAME}}}} [[Special:Search|poiskati]] ustrezajoče nove strani.',

# Revision deletion
'rev-deleted-comment'         => '(pripomba je bila odstranjena)',
'rev-deleted-user'            => '(uporabniško ime je bilo odstranjeno)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Prikazana redakcija je bila iz javnih arhivov odstranjena. 
Podrobnosti so morda na razpolago v [{{fullurl:Special:Log/delete|page=Rev-deleted-text-permission dnevniku brisanja}}].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Prikazana redakacija strani je bila iz javnih arhivov odstranjena. Ogledate si jo lahko, ker ste administrator spletišča. Podrobnosti so morda navedene v [{{fullurl:Special:Log/delete|page=Rev-deleted-text-view dnevniku brisanja}}].
</div>',
'rev-delundel'                => 'pokaži/skrij',
'revisiondelete'              => 'Izbriši/obnovi redakcije',
'revdelete-selected'          => 'Izbrana redakcija strani [[:$1]]:',
'revdelete-text'              => 'Izbrisane redakcije bodo v zgodovini strani še vedno navedene, vendar bo njihova vsebina za javnost nedostopna.

Do skrite vsebine bodo še vedno lahko dostopali drugi administratorji in jo z uporabo istega vmesnika tudi obnovili, razen kjer bodo operaterji spletišča uveljavili dodatne omejitve.',
'revdelete-legend'            => 'Nastavitve z redakcijami povezanih omejitev:',
'revdelete-hide-text'         => 'Skrij besedilo redakcije',
'revdelete-hide-comment'      => 'Skrij povzetek urejanja',
'revdelete-hide-user'         => 'Skrij urejevalčevo uporabniško ime/IP-naslov',
'revdelete-hide-restricted'   => 'Omejitve naj veljajo za vse uporabnike, z administratorji vred',
'revdelete-log'               => 'Dnevniški komentar:',
'revdelete-submit'            => 'Uporabi za izbrano redakcijo',
'revdelete-logentry'          => 'sprememba vidnosti redakcij strani [[$1]]',

# Diffs
'difference'              => '(Primerjava redakcij)',
'lineno'                  => 'Vrstica $1:',
'compareselectedversions' => 'Primerjaj izbrani redakciji',

# Search results
'searchresults'         => 'Izid iskanja',
'searchresulttext'      => 'Za več sporočil o iskanju v {{GRAMMAR:dajalnik|{{SITENAME}}}} glej [[{{MediaWiki:Helppage}}|Iščem v {{GRAMMAR:dajalnik|{{SITENAME}}}}]].',
'searchsubtitleinvalid' => 'Za povpraševanje "$1"',
'noexactmatch'          => "'''Stran z naslovom ''$1'' ne obstaja.''' Lahko [[:$1|jo ustvarite]].",
'titlematches'          => 'Ujemanje z naslovom članka',
'notitlematches'        => 'Iskanih besed ne vsebuje noben naslov članka',
'textmatches'           => 'Ujemanje z besedilom članka',
'notextmatches'         => 'Iskanih besed ne vsebuje nobeno besedilo članka',
'prevn'                 => '{{PLURAL:$1|prejšnja|prejšnji|prejšnje|prejšnjih|prejšnjih}} $1',
'nextn'                 => '{{PLURAL:$1|naslednja|naslednji|naslednjih|naslednjih|naslednjih}} $1',
'viewprevnext'          => 'Prikazujem ($1) ($2) ($3).',
'showingresults'        => 'Prikazujem <strong>$1</strong> {{PLURAL:$1|zadetek|zadetka|zadetke|zadetkov|zadetkov}}, začenši s št. <strong>$2</strong>.',
'showingresultsnum'     => "Prikazujem '''$3''' {{PLURAL:$1|zadetek|zadetka|zadetke|zadetkov|zadetkov}}, začenši s št. '''$2'''.",
'nonefound'             => "'''Opomba''': neuspešna poizvedovanja so pogosta ob iskanju vsakdanjih besed, na primer ''imeti'' in ''iz'', ki jih ni na seznamu. Ker gre za zelo pogoste besede, boste skoraj zagotovo iskali uspešneje z zožitvijo tematskega področja.",
'powersearch'           => 'Iskanje',
'searchdisabled'        => '<p>Zaradi hitrejšega delovanja {{GRAMMAR:rodilnik|{{SITENAME}}}} je iskanje po vsej zbirki podatkov začasno onemogočeno. Uporabite lahko Googlov ali Yahoojev iskalnik, vendar so njihovi podatki morda že zastareli.</p>',

# Preferences page
'preferences'              => 'Nastavitve',
'prefsnologin'             => 'Niste prijavljeni',
'prefsnologintext'         => 'Za spreminjanje uporabniških nastavitev se [[Special:UserLogin|prijavite]].',
'prefsreset'               => 'Nastavitve so ponastavljene.',
'qbsettings'               => 'Nastavitve hitre vrstice',
'qbsettings-none'          => 'Brez',
'qbsettings-fixedleft'     => 'Levo nepomično',
'qbsettings-fixedright'    => 'Desno nepomično',
'qbsettings-floatingleft'  => 'Levo leteče',
'qbsettings-floatingright' => 'Desno leteče',
'changepassword'           => 'Zamenjava gesla',
'skin'                     => 'Koža',
'math'                     => 'Prikaz matematičnega besedila',
'dateformat'               => 'Zapis datuma',
'datedefault'              => 'Kakor koli',
'datetime'                 => 'Datum in čas',
'math_failure'             => 'Ni mi uspelo razčleniti',
'math_unknown_error'       => 'neznana napaka',
'math_unknown_function'    => 'neznana funkcija',
'math_lexing_error'        => 'slovarska napaka',
'math_syntax_error'        => 'skladenjska napaka',
'math_image_error'         => 'Pretvarjanje v PNG ni uspelo; preverite, ali so latex, dvips, gs, in convert pravilno nameščeni.',
'math_bad_tmpdir'          => 'Začasne mape za matematiko ne morem ustvariti ali pisati vanjo.',
'math_bad_output'          => 'Izhodne mape za matematiko ne morem ustvariti ali pisati vanjo.',
'math_notexvc'             => "Manjka izvedbena datoteka 'texvc'; za njeno namestitev si poglejte math/README.",
'prefs-personal'           => 'Podatki o uporabniku',
'prefs-rc'                 => 'Zadnje spremembe',
'prefs-watchlist'          => 'Spisek nadzorov',
'prefs-watchlist-days'     => 'Število dni za prikaz na spisku nadzorov:',
'prefs-watchlist-edits'    => 'Število urejanj za prikaz na razširjenem spisku nadzorov:',
'prefs-misc'               => 'Druge nastavitve',
'saveprefs'                => 'Shrani',
'resetprefs'               => 'Ponastavi',
'oldpassword'              => 'Staro geslo:',
'newpassword'              => 'Novo geslo:',
'retypenew'                => 'Ponovno vpišite geslo:',
'textboxsize'              => 'Urejanje',
'rows'                     => 'Razsežnosti urejevalnega polja: vrstic:',
'columns'                  => 'stolpcev:',
'searchresultshead'        => 'Nastavitve poizvedovanja',
'resultsperpage'           => 'Prikazanih zadetkov na stran:',
'contextlines'             => 'Vrstic na zadetek:',
'contextchars'             => 'Znakov na vrstico:',
'recentchangescount'       => 'Število naslovov v zadnjih spremembah:',
'savedprefs'               => 'Spremembe ste uspešno shranili!',
'timezonelegend'           => 'Časovni pas',
'timezonetext'             => 'Vnesite časovno razliko med vašim krajevnim in strežniškim časom (UTC).',
'localtime'                => 'Krajevni čas:',
'timezoneoffset'           => 'Izravnava¹:',
'servertime'               => 'Strežniški čas:',
'guesstimezone'            => 'Izpolni iz brskalnika',
'allowemail'               => 'Drugim uporabnikom omogoči pošiljanje e-pošte',
'defaultns'                => 'Navadno išči v naslednjih imenskih prostorih:',
'default'                  => 'privzeto',
'files'                    => 'Datoteke',

# User rights
'userrights'               => 'Upravljanje s pravicami uporabnikov', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Upravljanje z uporabniškimi skupinami',
'userrights-user-editname' => 'Vpišite uporabniško ime:',
'editusergroup'            => 'Uredi uporabniške skupine',
'editinguser'              => "Urejanje '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Urejanje uporabniških skupin',
'saveusergroups'           => 'Shrani spremembe',
'userrights-groupsmember'  => 'Član skupine:',

# Groups
'group'            => 'Skupina:',
'group-bot'        => 'Boti',
'group-sysop'      => 'Administratorji',
'group-bureaucrat' => 'Birokrati',
'group-all'        => '(vsi)',

'group-sysop-member'      => 'Administrator',
'group-bureaucrat-member' => 'Birokrat',

'grouppage-bot'        => '{{ns:project}}:Boti',
'grouppage-sysop'      => '{{ns:project}}:Administratorji',
'grouppage-bureaucrat' => '{{ns:project}}:Birokrati',

# User rights log
'rightslog'      => 'Dnevnik_uporabniških_pravic',
'rightslogtext'  => 'Prikazan je dnevnik sprememb uporabniških pravic.',
'rightslogentry' => '- sprememba pravic uporabnika $1 iz $2 v $3',
'rightsnone'     => '(nobeno)',

# Recent changes
'recentchanges'                     => 'Zadnje spremembe',
'recentchangestext'                 => 'Na tej strani lahko spremljajte najnedavnejše spremembe wikija.',
'rcnote'                            => 'Prikazujem {{PLURAL:$1|zadnjo|zadnji|zadnje|zadnjih|zadnjih}} <strong>$1</strong> {{PLURAL:$1|spremembo|spremembi|spremembe|sprememb|sprememb}} v {{PLURAL:$2|zadnjem|zadnjih|zadnjih|zadnjih|zadnjih}} <strong>$2</strong> {{PLURAL:$2|dnevu|dneh|dneh|dneh|dneh}}, kot v $3.',
'rcnotefrom'                        => 'Navedene so spremembe od <b>$2</b> dalje (prikazujem jih do <b>$1</b>).',
'rclistfrom'                        => 'Prikaži spremembe od $1 naprej.',
'rcshowhideminor'                   => '$1 manjša urejanja',
'rcshowhidebots'                    => '$1 bote',
'rcshowhideliu'                     => '$1 prijavljene uporabnike',
'rcshowhideanons'                   => '$1 brezimne uporabnike',
'rcshowhidepatr'                    => '$1 pregledana urejanja',
'rcshowhidemine'                    => '$1 moja urejanja',
'rclinks'                           => 'Prikaži {{PLURAL:$1|zadnjo|zadnji|zadnje|zadnjih|zadnjih}} $1 {{PLURAL:$1|spremembo|spremembi|spremembe|sprememb|sprememb}} v {{PLURAL:$2|zadnjem $2 dnevu|zadnjih $2 dneh|zadnjih $2 dneh|zadnjih $2 dneh|zadnjih $2 dneh}};<br />$3',
'diff'                              => 'prim',
'hist'                              => 'zgod',
'hide'                              => 'skrij',
'show'                              => 'prikaži',
'number_of_watching_users_pageview' => '[temo {{PLURAL:$1|spremlja|spremljata|spremljajo|spremlja|spremlja}} $1 {{PLURAL:$1|uporabnik|uporabnika|uporabniki|uporabnikov|uporabnikov}}]',
'rc_categories'                     => 'Omejitev na kategorije (ločite jih z »|«)',
'rc_categories_any'                 => 'Katero koli',

# Recent changes linked
'recentchangeslinked' => 'Sorodne spremembe',

# Upload
'upload'                      => 'Naloži datoteko',
'uploadbtn'                   => 'Naloži datoteko',
'reupload'                    => 'Naloži drugo',
'reuploaddesc'                => 'Vrnitev na obrazec za nalaganje.',
'uploadnologin'               => 'Niste prijavljeni',
'uploadnologintext'           => 'Za nalaganje datotek se [[Special:UserLogin|prijavite]].',
'upload_directory_read_only'  => 'V mapo za nalaganje datotek ($1) spletni strežnik ne more pisati.',
'uploaderror'                 => 'Napaka',
'uploadtext'                  => "Spodnji obrazec lahko uporabite za nalaganje datotek;
za ogled ali iskanje že naloženih pojdite na [[Special:ImageList|seznam naloženih datotek]].
Naložitve in izbrisi so zapisani tudi v [[Special:Log/upload|dnevniku nalaganja]].

Datoteko lahko na želeno stran vključite z naslednjo skladnjo
* '''<nowiki>[[</nowiki>Image:File.jpg<nowiki>]]</nowiki>'''
* '''<nowiki>[[</nowiki>Image:File.png|alt text<nowiki>]]</nowiki>'''
ali za neposredno povezavo z datoteko
* '''<nowiki>[[</nowiki>Media:File.ogg<nowiki>]]</nowiki>'''",
'uploadlog'                   => 'dnevnik nalaganja',
'uploadlogpage'               => 'Dnevnik nalaganja datotek',
'uploadlogpagetext'           => 'Prikazan je seznam nedavno naloženih predstavnostnih datotek. Vsi navedeni časi so strežniški čas. Vedite, da polje »Uporabnik« razlikuje med malimi in velikimi črkami.',
'filename'                    => 'Ime datoteke',
'filedesc'                    => 'Povzetek',
'fileuploadsummary'           => 'Povzetek (avtor, spletni naslov vira ipd.):',
'filestatus'                  => 'Položaj avtorskih pravic:',
'filesource'                  => 'Vir:',
'uploadedfiles'               => 'Naložene datoteke',
'ignorewarning'               => 'Naloži kljub opozorilu',
'ignorewarnings'              => 'Prezri vsa opozorila',
'illegalfilename'             => 'Ime datoteke »$1« vsebuje v naslovih strani prepovedane znake. Prosimo, poskusite datoteko naložiti pod drugim imenom.',
'badfilename'                 => 'Ime datoteke se je samodejno popravilo v »$1«.',
'largefileserver'             => 'Velikost datoteke presega strežnikove nastavitve.',
'emptyfile'                   => 'Naložena datoteka je morda prazna. Do tega bi lahko prišlo zaradi slovnične napake v imenu. Ali datoteko resnično želite naložiti?',
'fileexists'                  => 'Datoteka s tem imenom že obstaja. Preden jo povozite, preverite stran <strong><tt>$1</tt></strong>. Da preprečite navzkrižja z že obstoječimi datotekami, uporabljajte za datoteke opisna imena (npr. »Eifflov stolp, Pariz, ponoči.jpg«).',
'fileexists-forbidden'        => 'Datoteka s tem imenom že obstaja in je ni mogoče prepisati. Poskusite svojo datoteko naložiti pod drugim imenom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Datoteka s tem imenom že obstaja v skupnem repozitoriju datotek. Prosimo, vrnite se in naložite svojo datoteko pod drugim imenom. 
[[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Datoteko ste uspešno naložili',
'uploadwarning'               => 'Opozorilo!',
'savefile'                    => 'Shrani datoteko',
'uploadedimage'               => '- naložena datoteka »[[$1]]«',
'uploaddisabled'              => 'Nalaganje je onemogočeno',
'uploaddisabledtext'          => 'Nalaganje datotek je na tem wikiju onemogočeno.',
'uploadscripted'              => 'Datoteka vsebuje HTML- ali skriptno kodo, ki bi jo lahko brskalnik razlagal napačno.',
'uploadcorrupt'               => 'Datoteka je poškodovana ali pa ima napačno končnico. Prosimo, preverite jo in znova naložite.',
'uploadvirus'                 => 'Datoteka morda vsebuje virus! Podrobnosti: $1',
'sourcefilename'              => 'Ime izvorne datoteke:',
'destfilename'                => 'Ime ciljne datoteke:',
'filewasdeleted'              => 'Datoteka s tem imenom je bila nekoč že naložena in potem izbrisana. Preden jo znova naložite, preverite $1.',

'license'   => 'Dovoljenje:',
'nolicense' => 'Nobeno (opomba: datoteka bo morda izbrisana)',

# Special:ImageList
'imagelist' => 'Seznam datotek',

# Image description page
'imagelinks'                => 'Strani z datoteko',
'linkstoimage'              => 'Datoteka je del naslednjih strani slovenske {{GRAMMAR:rodilnik|{{SITENAME}}}} (strani drugih projektov niso navedene):',
'nolinkstoimage'            => 'Z datoteko se ne povezuje nobena stran.',
'sharedupload'              => 'Datoteka se s tega mesta lahko uporabi tudi v drugih projektih.',
'shareduploadwiki'          => 'Nadaljnje informacije najdete na strani $1.',
'shareduploadwiki-linktext' => 'Opisna stran datoteke',
'noimage'                   => 'Datoteka s tem imenom ne obstaja; lahko jo $1.',
'noimage-linktext'          => 'naložite',
'uploadnewversion-linktext' => 'Naložite novo različico datoteke',

# MIME search
'mimesearch' => 'Iskanje po MIME-tipu',
'mimetype'   => 'MIME-tip:',
'download'   => 'naloži',

# Unwatched pages
'unwatchedpages' => 'Nespremljane strani',

# List redirects
'listredirects' => 'Seznam preusmeritev',

# Unused templates
'unusedtemplates'     => 'Osirotele predloge',
'unusedtemplatestext' => 'Naslednji seznam podaja vse strani v imenskem prostoru predlog, ki niso vključene v nobeno stran. Preden jih izbrišete, preverite še druge povezave nanje.',
'unusedtemplateswlh'  => 'druge povezave',

# Random page
'randompage' => 'Naključni članek',

# Random redirect
'randomredirect' => 'Naključna preusmeritev',

# Statistics
'statistics'    => 'Statistika',
'sitestats'     => 'Statistika {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'userstats'     => 'Uporabniška statistika',
'sitestatstext' => "V podatkovni zbirki je skupno '''$1''' strani.
Med te so vštete tudi ''pogovorne'' strani, strani o {{GRAMMAR:mestnik|{{SITENAME}}}}, najmanjše ''škrbinske'' strani, preusmeritve in še druge, ki niso članki. Če izključimo te zadnje, obstaja '''$2''' strani; ki so po vsej verjetnosti prave strani z vsebino.

Naloženih je '''$8''' {{PLURAL:{{NUMBEROFFILES}}|datoteka|datoteki|datoteke|datotek|datotek}}.

Od postavitve wikija je bilo opravljenih '''$3''' pregledov in '''$4''' urejanj strani.
To da skupaj povprečno '''$5''' urejanj na stran in '''$6''' pogledov na urejanje.

Dolžina [http://www.mediawiki.org/wiki/Manual:Job_queue čakalne vrste delovnih nalog]: '''$7'''.",
'userstatstext' => "Registriralo se je '''$1''' uporabnikov. Med temi je '''$2''' (oz. '''$4%''') administratorjev (glej $3).",

'disambiguations'     => 'Razločitvene strani',
'disambiguationspage' => 'Template:Razločitev',

'doubleredirects'     => 'Dvojne preusmeritve',
'doubleredirectstext' => '<b>Pozor:</b> seznam morda vsebuje neprave člane. To navadno pomeni, da pod prvim ukazom #REDIRECT obstaja dodatno besedilo s povezavami.<br />
Vsaka vrstica vsebuje povezave k prvi in drugi preusmeritvi ter prvo vrstico besedila druge preusmeritve. To navadno da pravi ciljni članek, h kateremu naj kaže prva preusmeritev.',

'brokenredirects'     => 'Pretrgane preusmeritve',
'brokenredirectstext' => 'Naslednje preusmeritve kažejo na neobstoječe strani.',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|zlog|zloga|zlogi|zlogov|zlogov}}',
'ncategories'             => '$1 {{PLURAL:$1|category|kategorij}}',
'nlinks'                  => '$1 {{PLURAL:$1|povezava|povezavi|povezave|povezav|povezav}}',
'nmembers'                => '$1 {{PLURAL:$1|element|elementa|elementi|elementov|elementov}}',
'nrevisions'              => '$1 {{PLURAL:$1|redakcija|redakciji|redakcije|redakcij|redakcij}}',
'nviews'                  => '$1-krat pregledano',
'lonelypages'             => 'Osirotele strani',
'uncategorizedpages'      => 'Nekategorizirane strani',
'uncategorizedcategories' => 'Nekategorizirane kategorije',
'uncategorizedimages'     => 'Nekategorizirane slike',
'unusedcategories'        => 'Osirotele kategorije',
'unusedimages'            => 'Osirotele datoteke',
'popularpages'            => 'Priljubljene strani',
'wantedcategories'        => 'Želene kategorije',
'wantedpages'             => 'Želene strani',
'mostlinked'              => 'Strani, na katere se največ povezuje',
'mostlinkedcategories'    => 'Kategorije z največ elementi',
'mostcategories'          => 'Članki z največ kategorijami',
'mostimages'              => 'Najbolj uporabljane datoteke',
'mostrevisions'           => 'Največkrat urejane strani',
'prefixindex'             => 'Iskanje po začetnih črkah',
'shortpages'              => 'Kratke strani',
'longpages'               => 'Dolge strani',
'deadendpages'            => 'Članki brez delujočih povezav',
'listusers'               => 'Seznam uporabnikov',
'newpages'                => 'Nove strani',
'ancientpages'            => 'Najdlje nespremenjeni članki',
'move'                    => 'Prestavi',
'movethispage'            => 'Prestavi stran',
'unusedimagestext'        => '<p>Prosimo, upoštevajte, da lahko druge spletne strani datoteko uporabljajo neposredno z navedbo spletnega naslova. Zato so datoteke lahko navedene, čeprav se uporabljajo.</p>',
'unusedcategoriestext'    => 'Naslednje strani kategorij obstajajo, vendar jih ne uporablja noben članek ali druga kategorija.',
'notargettitle'           => 'Ni cilja',
'notargettext'            => 'Niste navedli ciljne strani ali uporabnika za izvedbo ukaza.',

# Book sources
'booksources' => 'Prepoznava ISBN-številk',

# Special:Log
'specialloguserlabel'  => 'Uporabnik:',
'speciallogtitlelabel' => 'Naslov:',
'log'                  => 'Dnevniki',
'alllogstext'          => 'Združeno so prikazani dnevniki sprememb uporabniških pravic, preimenovanj uporabnikov, nalaganja predstavnostnih datotek, prestavljanja in zaščite strani, brisanja, registracij uporabnikov, sprememb položaja botov ter blokiranja in deblokiranja uporabnikov. Pogled lahko zožite z izbiro dnevnika, uporabniškega imena ali strani. Vedite, da polje »Uporabnik« razlikuje med malimi in velikimi črkami.',
'logempty'             => 'O tej strani ni v dnevniku ničesar.',

# Special:AllPages
'allpages'          => 'Vse strani',
'alphaindexline'    => '$1 do $2',
'nextpage'          => 'Naslednja stran ($1)',
'allpagesfrom'      => 'Prikaži strani, ki se začnejo na:',
'allarticles'       => 'Vsi članki',
'allinnamespace'    => 'Vse strani (imenski prostor $1)',
'allnotinnamespace' => 'Vse strani (brez imenskega prostora $1)',
'allpagesprev'      => 'Predhodna',
'allpagesnext'      => 'Naslednja',
'allpagessubmit'    => 'Pojdi',
'allpagesprefix'    => 'Prikaži strani z začetnimi črkami:',

# Special:Categories
'categories'         => 'Kategorije',
'categoriespagetext' => 'V wikiju obstajajo naslednje kategorije.',

# E-mail user
'mailnologin'     => 'Manjka naslov pošiljatelja',
'mailnologintext' => "Za pošiljanje pošte se [[Special:UserLogin|prijavite]] in v [[Special:Preferences|nastavitvah]] vpišite veljaven '''overjen''' e-poštni naslov.",
'emailuser'       => 'Pošlji uporabniku e-pismo',
'emailpage'       => 'Pošlji uporabniku e-pismo',
'emailpagetext'   => "S spodnjim obrazcem lahko uporabniku pošljete e-poštno sporočilo.
Da bo prejemnik lahko odgovoril neposredno vam, bo v glavi sporočila zapisan '''vaš e-poštni naslov''' (kot ste ga vpisali v uporabniških nastavitvah).",
'usermailererror' => 'Predmet e-pošte je vrnil napako:',
'defemailsubject' => 'Elektronska pošta {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'noemailtitle'    => 'Ni e-poštnega naslova.',
'noemailtext'     => 'Uporabnik ni navedel veljavnega e-poštnega naslova ali pa elektronske pošte ne želi prejemati.',
'emailfrom'       => 'Od',
'emailto'         => 'Za',
'emailsubject'    => 'Predmet',
'emailmessage'    => 'Sporočilo',
'emailsend'       => 'Pošlji',
'emailsent'       => 'E-pismo je poslano!',
'emailsenttext'   => 'E-pismo je poslano.',

# Watchlist
'watchlist'            => 'Spisek nadzorov',
'mywatchlist'          => 'Spisek nadzorov',
'nowatchlist'          => 'Vaš spisek nadzorov je prazen.',
'watchnologin'         => 'Niste prijavljeni',
'watchnologintext'     => 'Za urejanje spiska nadzorov se [[Special:UserLogin|prijavite]].',
'addedwatch'           => 'Dodano na spisek nadzorov',
'addedwatchtext'       => "Stran »'''<nowiki>$1</nowiki>'''« je bila dodana na vaš [[Special:Watchlist|spisek nadzorov]], kjer bodo odslej navedene njene morebitne spremembe in spremembe pripadajoče pogovorne strani. Za lažjo izbiro bodo tudi v [[Special:RecentChanges|seznamu zadnjih sprememb]] prikazane <b>krepko</b>. Če jo želite odstraniti s spiska, kliknite zavihek »Prenehaj opazovati«.",
'removedwatch'         => 'Odstranjena s spiska nadzorov',
'removedwatchtext'     => 'Stran »<nowiki>$1</nowiki>« je odstranjena z vašega spiska nadzorov.',
'watch'                => 'Opazuj',
'watchthispage'        => 'Opazuj stran',
'unwatch'              => 'Prenehaj opazovati',
'unwatchthispage'      => 'Prenehaj opazovati stran',
'notanarticle'         => 'Ni članek',
'watchnochange'        => 'V prikazanem časovnem obdobju se ni spremenila nobena med nadzorovanimi stranmi.',
'watchlist-details'    => 'Spremljate <strong>$1</strong> (pogovorne strani niso vštete).',
'wlheader-enotif'      => '* Obveščanje po elektronski pošti je omogočeno.',
'wlheader-showupdated' => "* Od vašega zadnjega ogleda spremenjene strani so prikazanje '''krepko'''.",
'watchmethod-recent'   => 'med nedavnimi urejanji iščem spremljane strani',
'watchmethod-list'     => 'med spremljanimi stranmi iščem nedavna urejanja',
'watchlistcontains'    => 'Spremljate $1 {{PLURAL:$1|stran|strani|strani|strani|strani}}.',
'iteminvalidname'      => "Težava z izbiro '$1', neveljavno ime ...",
'wlnote'               => 'Navedenih je {{PLURAL:$1|zadnja|zadnji|zadnje|zadnjih|zadnjih}} $1 {{PLURAL:$1|sprememba|spremembi|spremembe|sprememb}} v {{PLURAL:$2|zadnji|zadnjih|zadnjih|zadnjih|zadnjih}} <b>$2</b> {{PLURAL:$2|uri|urah|urah|urah|urah}}.',
'wlshowlast'           => 'Prikaži zadnjih $1 ur; $2 dni; $3;',

'enotif_mailer'      => '{{SITENAME}} - obvestilni poštar',
'enotif_reset'       => 'Označi vse strani kot prebrane',
'enotif_newpagetext' => 'To je nova stran.',
'changed'            => 'spremenjena',
'created'            => 'ustvarjena',
'enotif_subject'     => 'Stran {{GRAMMAR:rodilnik|{{SITENAME}}}} $PAGETITLE je $CHANGEDORCREATED $PAGEEDITOR',
'enotif_lastvisited' => 'Za spremembe po vašem zadnjem obisku glejte $1.',
'enotif_body'        => '$WATCHINGUSERNAME,

stran v {{GRAMMAR:dajalnik|{{SITENAME}}}} $PAGETITLE je bila $PAGEEDITDATE $CHANGEDORCREATED s strani $PAGEEDITOR,
za trenutno redakcijo glejte $PAGETITLE_URL

$NEWPAGE

Urejevalčev povzetek: $PAGESUMMARY $PAGEMINOREDIT

Navežite stik z urejevalcem:
e-pošta $PAGEEDITOR_EMAIL
wiki $PAGEEDITOR_WIKI

Nadaljnjih obvestil do obiska strani ne boste prejemali. Na spisku nadzorov lahko znova nastavite zastavice obveščanj za vse spremljane strani.

             Vaš opozorilni sistem slovenskega {{GRAMMAR:rodilnik|{{SITENAME}}}}

--
Za spremembo nastavitev spiska nadzorov obiščite
{{fullurl:Special:Watchlist/edit}}

Povratna sporočila in pomoč:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Briši stran',
'confirm'                     => 'Potrdi',
'excontent'                   => "vsebina: '$1'",
'excontentauthor'             => "vsebina: '$1' (edini urejevalec pa '$2')",
'exbeforeblank'               => "vsebina pred brisanjem: '$1'",
'exblank'                     => 'prazna stran',
'historywarning'              => 'OPOZORILO: stran, ki jo želite izbrisati, ima zgodovino:',
'confirmdeletetext'           => "Iz zbirke podatkov boste izbrisali stran ali sliko skupaj z vso njeno zgodovino. 
Prosimo, '''potrdite''', da to resnično želite, da razumete posledice dejanja in da se ravnate po [[{{MediaWiki:Policy-url}}|pravilih]].",
'actioncomplete'              => 'Poseg je končan',
'deletedtext'                 => "»'''<nowiki>$1</nowiki>'''« je bila izbrisana. 
Za zapise nedavnih brisanj glej $2.",
'deletedarticle'              => 'je izbrisal(-a) »$1«',
'dellogpage'                  => 'Dnevnik brisanja',
'dellogpagetext'              => 'Prikazan je seznam nedavnih brisanj z navedenim strežniškim časom. Vedite, da polje »Uporabnik« razlikuje med malimi in velikimi črkami.',
'deletionlog'                 => 'dnevnik brisanja',
'reverted'                    => 'Obnova prejšnje redakcije',
'deletecomment'               => 'Razlog za brisanje',
'rollback'                    => 'Vrni spremembe',
'rollback_short'              => 'Vrni',
'rollbacklink'                => 'vrni',
'rollbackfailed'              => 'Vrnitev ni uspela.',
'cantrollback'                => 'Urejanja ne morem vrniti; zadnji urejevalec je hkrati edini.',
'alreadyrolled'               => 'Ne morem vrniti zadnje spremembe [[:$1]]
od uporabnika [[User:$2|$2]] ([[User talk:$2|Pogovor]]); nekdo drug je že spremenil ali vrnil članek.

Zadnja sprememba od uporabnika [[User:$3|$3]] ([[User talk:$3|Pogovor]]).',
'editcomment'                 => "Pripomba k spremembi: »''$1''«.", # only shown if there is an edit comment
'revertpage'                  => 'vrnitev sprememb uporabnika »[[Special:Contributions/$2|$2]]« ([[User talk:$2|pogovor]]) na zadnje urejanje uporabnika »$1«', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'sessionfailure'              => 'Vaša prijava ni uspela; da bi preprečili ugrabitev seje, je bilo dejanje preklicano. Prosimo, izberite »Nazaj« in ponovno naložite stran, s katere prihajate, nato poskusite znova.',
'protectlogpage'              => 'Dnevnik zaščit strani',
'protectlogtext'              => 'Prikazan je seznam zaščit in odstranitev zaščit strani. Za več podatkov glejte [[Project:Zaščitena stran]] in [[Project:Pravila zaščite]]. Vedite, da polje »Uporabnik« razlikuje med malimi in velikimi črkami.',
'protectedarticle'            => 'Zaščita strani "[[$1]]"',
'unprotectedarticle'          => 'Zaščita strani $1 je odstranjena.',
'protect-title'               => 'Zaščita strani »$1«',
'protect-legend'              => 'Potrdite zaščito',
'protectcomment'              => 'Razlog:',
'protect-unchain'             => 'Deblokiraj dovoljenja za premikanje',
'protect-text'                => "Tu si lahko ogledate in spremenite raven zaščitenosti strani '''<nowiki>$1</nowiki>'''.",
'protect-default'             => 'Omogoči urejanje vsem uporabnikom',
'protect-level-autoconfirmed' => 'Blokiraj neregistrirane uporabnike',
'protect-level-sysop'         => 'Blokiraj vse uporabnike (razen administratorjev)',

# Restrictions (nouns)
'restriction-edit' => 'Urejanje',
'restriction-move' => 'Prestavljanje',

# Undelete
'undelete'               => 'Obnovi izbrisano stran',
'undeletepage'           => 'Prikaži izbrisane strani in jih obnovi',
'viewdeletedpage'        => 'Pregled izbrisanih strani',
'undeletepagetext'       => 'Naslednje strani so izbrisane, vendar so še vedno v arhivu in jih lahko obnovite. Arhiv je treba občasno počistiti.',
'undeleteextrahelp'      => "Da bi obnovili celotno stran z vso njeno zgodovino, pustite vsa potrditvena polja prazna in kliknite '''''Obnovi'''''.
Če želite obnoviti le določene redakcije strani, pred klikom gumba '''''Obnovi''''' označite ustrezna potrditvena polja.
Klik gumba '''''Ponastavi''''' bo izpraznil polje za vnos razloga in vsa potrditvena polja.",
'undeleterevisions'      => '{{PLURAL:$1|Arhivirana je|Arhivirani sta|Arhivirane so|Arhiviranih je|Arhiviranih ni}} $1 {{PLURAL:$1|redakcija|redakciji|redakcije|redakcij|redakcij}}',
'undeletehistory'        => 'Z obnovo strani se bodo po privzetem obnovile tudi vse pripadajoče redakcije. Če se želite temu izogniti, označite le želene.

Kjer je bila po brisanju ustvarjena nova stran s tem imenom, se trenutna redakcija obstoječe strani ne bo samodejno zamenjala, temveč se bodo obnovljene redakcije pojavile v prejšnji zgodovini. Pazite, da se temu izognete, razen seveda, kadar resnično nameravate združiti zgodovini obeh strani.',
'undeletehistorynoadmin' => 'Stran je izbrisana. Razlog za izbris je skupaj s podrobnostmi o uporabnikih, ki so jo urejali pred izbrisom, naveden v prikazanem povzetku. Dejansko besedilo izbrisanih redakcij je dostopno le administratorjem.',
'undeletebtn'            => 'Obnovi',
'undeletereset'          => 'Ponastavi',
'undeletecomment'        => 'Razlog:',
'undeletedarticle'       => 'je obnovil(-a) »$1«',
'undeletedrevisions'     => 'obnovljeno: $1 {{PLURAL:$1|redakcija|redakciji|redakcije|redakcij|redakcij}}',
'undeletedfiles'         => 'Obnovili ste $1 {{PLURAL:datoteka|datoteki|datoteke|datotek|datotek}}',
'cannotundelete'         => 'Obnova ni uspela. Morda je stran obnovil že kdo drug.',
'undeletedpage'          => "<big>'''Obnovili ste stran '$1'.'''</big>

Nedavna brisanja in obnove so zapisani v [[Special:Log/delete|dnevniku brisanja]].",

# Namespace form on various pages
'namespace'      => 'Imenski prostor:',
'invert'         => 'Obrni izbor',
'blanknamespace' => '(Osnovno)',

# Contributions
'contributions' => 'Uporabnikovi prispevki',
'mycontris'     => 'Prispevki',
'contribsub2'   => 'Uporabnik: $1 ($2)',
'nocontribs'    => 'Ne najdem nobene merilom ustrezajoče spremembe.',
'uctop'         => ' (vrh)',

'sp-contributions-newbies-sub' => 'Prispevki novincev',

# What links here
'whatlinkshere' => 'Kaj se povezuje sem',
'linklistsub'   => '(Seznam povezav)',
'linkshere'     => 'Sem kažejo naslednje strani:',
'nolinkshere'   => 'Sem ne kaže nobena stran.',
'isredirect'    => 'preusmeritvena stran',
'istemplate'    => 'vključitev',

# Block/unblock
'blockip'                     => 'Blokiranje IP-naslova ali uporabniškega imena',
'blockiptext'                 => "Naslednji obrazec vam omogoča, da določenemu IP-naslovu ali uporabniškemu imenu preprečite urejanje. To storimo le zaradi zaščite pred nepotrebnim uničevanjem in po [[{{MediaWiki:Policy-url}}|pravilih]]. Vnesite tudi razlog, ''na primer'' seznam strani, ki jih je uporabnik po nepotrebnem kvaril.",
'ipaddress'                   => 'IP-naslov',
'ipadressorusername'          => 'IP-naslov ali uporabniško ime',
'ipbexpiry'                   => 'Pretek',
'ipbreason'                   => 'Razlog',
'ipbsubmit'                   => 'Blokiraj naslov',
'ipbother'                    => 'Drugačen čas',
'ipboptions'                  => 'nedoločen čas:infinite,15 minut:15 minutes,1 uro:1 hour,3 ure:3 hours,1 dan:1 day,2 dni:2 days,3 dni:3 days,1 teden:1 week,2 tedna:2 weeks,1 mesec:1 month,3 mesece:3 months,6 mesecev:6 months,1 leto:1 year,neomejeno dolgo:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'drugo',
'badipaddress'                => 'Neveljaven IP-naslov ali uporabniško ime.',
'blockipsuccesssub'           => 'Blokiranje je uspelo',
'blockipsuccesstext'          => 'IP-naslov ali uporabniški račun »[[Special:Contributions/$1|$1]]« je blokiran.<br />
Preglejte [[Special:IPBlockList|seznam blokiranih IP-naslovov]].',
'unblockip'                   => 'Omogočite urejanje IP-naslovu',
'unblockiptext'               => 'Z naslednjim obrazcem lahko obnovite možnost urejanja z blokiranega IP-naslova ali uporabniškega računa.',
'ipusubmit'                   => 'Deblokiraj naslov',
'ipblocklist'                 => 'Seznam blokiranih IP-naslovov',
'blocklistline'               => '$1, $2 je blokiral(-a) $3 ($4)',
'infiniteblock'               => 'neomejen čas',
'expiringblock'               => 'preteče: $1',
'blocklink'                   => 'blokiraj',
'unblocklink'                 => 'deblokiraj',
'contribslink'                => 'prispevki',
'autoblocker'                 => 'Ker si delite IP-naslov z »$1«, vam je urejanje samodejno onemogočeno. Razlog: »$2«.',
'blocklogpage'                => 'Dnevnik blokiranja',
'blocklogentry'               => 'uporabnika »$1« sem blokiral(-a) za $2',
'blocklogtext'                => 'Prikazan je dnevnik blokiranja in deblokiranja uporabnikov. Samodejno blokirani IP-naslovi niso navedeni. Trenutno veljavna blokiranja so navedena na [[Special:IPBlockList|seznamu blokiranih IP-naslovov]].',
'unblocklogentry'             => 'je deblokiral(-a) »$1«',
'range_block_disabled'        => 'Možnost administratorjev za blokiranje urejanja IP-razponom je onemogočena.',
'ipb_expiry_invalid'          => 'Neveljaven čas preteka',
'ip_range_invalid'            => 'Neveljaven IP-razpon.',
'proxyblocker'                => 'Blokator posredniških strežnikov',
'proxyblockreason'            => 'Ker uporabljate odprti posredniški strežnik, je urejanje z vašega IP-naslova preprečeno. Gre za resno varnostno težavo, o kateri obvestite svojega internetnega ponudnika.',
'proxyblocksuccess'           => 'Storjeno.',
'sorbsreason'                 => 'Vaš IP-naslov je v DNSBL uvrščen med odprte posredniške strežnike.',
'sorbs_create_account_reason' => 'Vaš IP-naslov je v DNSBL naveden kot odprti posredniški strežnik. Računa zato žal ne morete ustvariti.',

# Developer tools
'lockdb'              => 'Zakleni zbirko podatkov',
'unlockdb'            => 'Odkleni zbirko podatkov',
'lockdbtext'          => 'Zaklenitev zbirke podatkov bo vsem uporabnikom preprečila možnost urejanja strani, spreminjanja nastavitev, urejanja spiska nadzorov in drugih stvari, ki zahtevajo spremembe zbirke podatkov. Prosimo, potrdite, da jo resnično želite zakleniti in da jo boste po končanem vzdrževanju spet odklenili.',
'unlockdbtext'        => 'Odklenitev zbirke podatkov bo vsem uporabnikom obnovila možnost urejanja strani, spreminjanja nastavitev, urejanja seznamov nadzorov in drugih stvari, ki zahtevajo spremembe zbirke. Prosimo, potrdite nedvomni namen.',
'lockconfirm'         => 'Da, zbirko podatkov želim zakleniti.',
'unlockconfirm'       => 'Da, zbirko podatkov želim odkleniti.',
'lockbtn'             => 'Zakleni zbirko podatkov',
'unlockbtn'           => 'Odkleni zbirko podatkov',
'locknoconfirm'       => 'Namere niste potrdili.',
'lockdbsuccesssub'    => 'Zbirko podatkov ste uspešno zaklenili',
'unlockdbsuccesssub'  => 'Zbirka podatkov je odklenjena',
'lockdbsuccesstext'   => 'Podatkovna baza {{GRAMMAR:rodilnik|{{SITENAME}}}} je bila zaklenjena.
<br />Ne pozabite odkleniti, ko boste končali z vzdrževanjem.',
'unlockdbsuccesstext' => 'Zbirka podatkov {{GRAMMAR:rodilnik|{{SITENAME}}}} je spet odklenjena.',

# Move page
'move-page-legend'        => 'Prestavitev strani',
'movepagetext'            => "Z naslednjim obrazcem lahko stran preimenujete in hkrati prestavite tudi vso njeno zgodovino. Dosedanja stran se bo spremenila v preusmeritev na prihodnje mesto. 

'''Povezave na dosedanji naslov strani se ne bodo spremenile, zato vas prosimo, da po prestavitvi strani z uporabo pripomočka »Kaj se povezuje sem« popravite vse dvojne preusmeritve, ki bodo morda nastale.''' Odgovorni ste, da bodo povezave še naprej kazale na prava mesta.

Kjer stran z izbranim novim imenom že obstaja, dejanje '''ne''' bo izvedeno, razen če je sedanja stran prazna ali preusmeritvena in brez zgodovine urejanj. To pomeni, da lahko, če se zmotite, strani vrnete prvotno ime, ne morete pa prepisati že obstoječe strani.

<b>OPOZORILO!</b>
Prestavitev strani je lahko za priljubljeno stran velika in nepričakovana sprememba, zato pred izbiro ukaza dobro premislite.",
'movepagetalktext'        => "Če obstaja, bo samodejno prestavljena tudi pripadajoča pogovorna stran, '''razen kadar'''
*stran prestavljate prek imenskih prostorov,
*pod novim imenom že obstaja neprazna pogovorna stran ali
*ste odkljukali spodnji okvirček.

Če je tako, boste morali pogovorno stran, če želite, prestaviti ali povezati ročno. Če tega ne morete storiti, predlagajte prestavitev na strani [[Project:Želene prestavitve]], vsekakor pa tega '''''ne''''' počnite s preprostim izrezanjem in prilepljenjem vsebine, saj bi tako pokvarili zgodovino urejanja strani.",
'movearticle'             => 'Prestavi stran',
'newtitle'                => 'Na naslov',
'movepagebtn'             => 'Prestavi stran',
'pagemovedsub'            => 'Uspešno prestavljeno',
'articleexists'           => "'''Stran ni bilo mogoče prestaviti!''' Izbrano ime je že zasedeno ali pa ni veljavno.",
'talkexists'              => 'Sama stran je bila uspešno prestavljena, pripadajoča pogovorna stran pa ne, ker že obstaja na novem naslovu. Prosimo, združite ju ročno. Če tega ne morete storiti, prosite za pomoč katerega izmed administratorjev, nikakor pa tega NE počnite z izrezanjem in prilepljenjem vsebine.',
'movedto'                 => 'prestavljeno na',
'movetalk'                => 'Če je mogoče, prestavi tudi pogovorno stran.',
'1movedto2'               => '- prestavitev [[$1]] na [[$2]]',
'1movedto2_redir'         => '- prestavitev [[$1]] na [[$2]] čez preusmeritev',
'movelogpage'             => 'Dnevnik prestavljanja strani',
'movelogpagetext'         => 'Prikazujem seznam prestavljenih strani. Vedite, da polje »Uporabnik« razlikuje med malimi in velikimi črkami.',
'movereason'              => 'Razlog',
'revertmove'              => 'vrni',
'delete_and_move'         => 'Briši in prestavi',
'delete_and_move_text'    => '==Treba bi bilo brisati==

Ciljna stran »[[:$1]]« že obstaja. Ali jo želite, da bi pripravili prostor za prestavitev, izbrisati?',
'delete_and_move_confirm' => 'Da, izbriši stran',
'delete_and_move_reason'  => 'Izbrisano z namenom pripraviti prostor za prestavitev.',
'selfmove'                => "'''Naslova vira in cilja sta enaka; stran ni mogoče prestaviti samo vase.''' Prosimo, preverite, ali niste naslova cilja namesto v polje »Na naslov« vpisali v polje »Razlog«.",
'immobile_namespace'      => 'Kot cilj ste določili naslov posebne vrste. V ta imenski prostor stran ni mogoče prestaviti.',

# Export
'export'        => 'Izvoz strani',
'exporttext'    => "Besedilo in urejevalno zgodovino ene ali več strani lahko izvozite v obliki XML. V prihodnosti bo to vsebino morda mogoče izvoziti v drug wiki, ki ga bo poganjalo programje MediaWiki, v trenutni različici pa so možnosti za to zelo omejene (kjer je omogočeno orodje ''Special:Import'', lahko vsebino z njegovo uporabo uvozijo administratorji).

Če želite izvoziti članke, v spodnje polje vpišite njihove naslove (enega v vsako vrstico) in označite, ali želite le trenutno različico s podatki o trenutnem urejanju ali tudi vse prejšnje z vrsticami o zgodovini strani.

Če gre za slednje, lahko uporabite tudi povezavo, npr. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] za 
članek \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly' => 'Vključi le trenutno redakcijo, ne pa celotne zgodovine.',
'export-submit' => 'Izvozi',

# Namespace 8 related
'allmessages'               => 'Sistemska sporočila',
'allmessagesname'           => 'Ime',
'allmessagesdefault'        => 'Prednastavljeno besedilo',
'allmessagescurrent'        => 'Trenutno besedilo',
'allmessagestext'           => 'Navedena so v imenskem prostoru MediaWiki dostopna sistemska sporočila.
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "Ker je '''\$wgUseDatabaseMessages''' izključeno, '''{{ns:special}}:Allmessages''' ni podprto.",
'allmessagesfilter'         => 'Filter pogostih izrazov:',
'allmessagesmodified'       => 'Prikaži le spremenjeno',

# Thumbnails
'thumbnail-more'  => 'Povečaj',
'filemissing'     => 'Datoteka manjka',
'thumbnail_error' => 'Napaka pri izdelavi sličice: $1',

# Special:Import
'import'                => 'Uvoz strani',
'importinterwiki'       => 'Transwikiuvoz',
'importtext'            => 'Z uporabo orodja Special:Export izvozite datoteko iz izvornega wikija, shranite jo na disk in naložite tu.',
'importfailed'          => 'Uvoz ni uspel: $1',
'importnotext'          => 'Prazno ali brez besedila',
'importsuccess'         => 'Uspešno uvoženo!',
'importhistoryconflict' => 'Zgodovina strani vključuje navzkrižno redakcijo (morda je bila stran naložena že prej)',
'importnosources'       => 'Na tem wikiju je ta možnost onemogočena.',
'importnofile'          => 'Uvožena ni bila nobena datoteka.',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Vaša uporabniška stran',
'tooltip-pt-anonuserpage'         => 'Uporabniška stran IP-naslova, ki ga uporabljate',
'tooltip-pt-mytalk'               => 'Vaša pogovorna stran',
'tooltip-pt-anontalk'             => 'Pogovor o urejanjih s tega IP-naslova',
'tooltip-pt-preferences'          => 'Vaše nastavitve',
'tooltip-pt-watchlist'            => 'Seznam strani, katerih spremembe spremljate',
'tooltip-pt-mycontris'            => 'Seznam vaših prispevkov',
'tooltip-pt-login'                => 'Prijava ni obvezna, vendar je zaželena',
'tooltip-pt-anonlogin'            => 'Prijava ni obvezna, vendar je zaželena',
'tooltip-pt-logout'               => 'Odjavite se',
'tooltip-ca-talk'                 => 'Pogovor o strani',
'tooltip-ca-edit'                 => 'Stran lahko uredite. Preden jo shranite, uporabite gumb za predogled.',
'tooltip-ca-addsection'           => 'Začnite novo razpravo',
'tooltip-ca-viewsource'           => 'Stran je zaščitena, ogledate si lahko njeno izvorno kodo',
'tooltip-ca-history'              => 'Prejšnje redakcije strani',
'tooltip-ca-protect'              => 'Zaščitite stran',
'tooltip-ca-delete'               => 'Brišite stran',
'tooltip-ca-undelete'             => 'Obnovite pred izbrisom napravljena urejanja strani.',
'tooltip-ca-move'                 => 'Preimenujte stran',
'tooltip-ca-watch'                => 'Dodajte stran na seznam nadzorov',
'tooltip-ca-unwatch'              => 'Odstranite stran s seznama nadzorov',
'tooltip-search'                  => 'Preiščite wiki',
'tooltip-p-logo'                  => 'Glavna stran',
'tooltip-n-mainpage'              => 'Obiščite Glavno stran',
'tooltip-n-portal'                => 'O projektu, kaj lahko storite, kje lahko kaj najdete',
'tooltip-n-currentevents'         => 'Spoznajte ozadje trenutnih dogodkov',
'tooltip-n-recentchanges'         => 'Seznam zadnjih sprememb {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'tooltip-n-randompage'            => 'Naložite naključno stran',
'tooltip-n-help'                  => 'Kraj za pomoč',
'tooltip-t-whatlinkshere'         => 'Seznam vseh s trenutno povezanih strani',
'tooltip-t-recentchangeslinked'   => 'Zadnje spremembe na s trenutno povezanih straneh',
'tooltip-feed-rss'                => 'RSS-dovod strani',
'tooltip-feed-atom'               => 'Atom-dovod strani',
'tooltip-t-contributions'         => 'Preglejte seznam uporabnikovih prispevkov',
'tooltip-t-emailuser'             => 'Pošljite uporabniku e-pismo',
'tooltip-t-upload'                => 'Naložite slike ali predstavnostne datoteke',
'tooltip-t-specialpages'          => 'Preglejte seznam vseh posebnih strani',
'tooltip-ca-nstab-main'           => 'Prikaže članek',
'tooltip-ca-nstab-user'           => 'Prikaže uporabniško stran',
'tooltip-ca-nstab-media'          => 'Prikaže stran s predstavnostno vsebino',
'tooltip-ca-nstab-special'        => 'Te posebne strani ne morete urejati',
'tooltip-ca-nstab-project'        => 'Prikaže stran projekta',
'tooltip-ca-nstab-image'          => 'Prikaže stran s sliko ali drugo datoteko',
'tooltip-ca-nstab-mediawiki'      => 'Prikaže sistemsko sporočilo',
'tooltip-ca-nstab-template'       => 'Prikaže stran predloge',
'tooltip-ca-nstab-help'           => 'Prikaže stran s pomočjo',
'tooltip-ca-nstab-category'       => 'Prikaže stran kategorije',
'tooltip-minoredit'               => 'Označite kot manjše urejanje',
'tooltip-save'                    => 'Shranite vnesene spremembe (ste si jih predogledali?)',
'tooltip-preview'                 => 'Pred shranjevanjem si, prosimo, predoglejte stran!',
'tooltip-diff'                    => 'Preglejte spremembe, ki ste jih vnesli.',
'tooltip-compareselectedversions' => 'Preglejte razlike med izbranima redakcijama.',
'tooltip-watch'                   => 'Dodajte stran na svoj spisek nadzorov.',
'tooltip-recreate'                => 'Ta stran je namenoma (skoraj) prazna.',

# Metadata
'nodublincore'      => 'Metapodatki Dublin Core RDF so na tem strežniku onemogočeni.',
'nocreativecommons' => 'Metapodatki Creative Commons RDF so za ta strežnik onemogočeni.',
'notacceptable'     => 'V obliki, ki jo lahko bere vaš odjemalec, wikistrežnik podatkov ne more ponuditi.',

# Attribution
'anonymous'        => 'Brezimni uporabnik(i) {{GRAMMAR:rodilnik|{{SITENAME}}}}',
'siteuser'         => 'Uporabnik $1',
'lastmodifiedatby' => 'Stran je zadnjič urejal(a) $3 (čas spremembe: $2, $1).', # $1 date, $2 time, $3 user
'othercontribs'    => '<!--Temelji na delu $1. Ne deluje pravilno-->Prispevki uporabnika.',
'others'           => 'drugi',
'siteusers'        => 'Uporabnik(i) $1',
'creditspage'      => 'Zasluge za stran',
'nocredits'        => 'Ni dostopnih podatkov o zaslugah za stran.',

# Spam protection
'spamprotectiontitle' => 'Zaščita pred neželenimi sporočili',
'spamprotectiontext'  => "Stran, ki ste jo želeli shraniti, je zaščita pred spamom blokirala, saj je vsebovala povezavo na zunanjo stran, navedeno na črni listi spama. Če povezave (glejte spodaj) niste dodali vi, je verjetno obstajala že v prejšnji redakciji ali pa jo je dodalo vohunsko programje (''spyware'') na vašem računalniku.",
'spamprotectionmatch' => 'Naslednje besedilo je sprožilo naš filter neželenih sporočil: $1',
'spambot_username'    => 'MediaWiki čiščenje navlake',
'spam_reverting'      => 'Vračanje na zadnjo redakcijo brez povezav na $1',
'spam_blanking'       => 'Vse redakcije so vsebovale povezave na $1, izpraznjujem',

# Info page
'infosubtitle'   => 'Podatki o strani',
'numedits'       => 'Število urejanj (članek): $1',
'numtalkedits'   => 'Število urejanj (pogovorna stran): $1',
'numwatchers'    => 'Število oseb, ki spremljajo stran: $1',
'numauthors'     => 'Število avtorjev: $1',
'numtalkauthors' => 'Število avtorjev (pogovorna stran): $1',

# Math options
'mw_math_png'    => 'Vedno prikaži PNG',
'mw_math_simple' => 'Kadar je dovolj preprosto, uporabi HTML, sicer pa PNG',
'mw_math_html'   => 'Kadar je mogoče, uporabi HTML, sicer pa PNG',
'mw_math_source' => 'Pusti v TeX-ovi obliki (za besedilne brskalnike)',
'mw_math_modern' => 'Priporočeno za sodobne brskalnike',
'mw_math_mathml' => 'če je le mogoče, uporabi MathML (preizkusno)',

# Patrolling
'markaspatrolleddiff'        => 'Označite kot nadzorovano',
'markaspatrolledtext'        => 'Označite članek kot nadzorovan',
'markedaspatrolled'          => 'Označeno kot nadzorovano',
'markedaspatrolledtext'      => 'Izbrano različico ste označili kot nadzorovano.',
'rcpatroldisabled'           => 'Spremljanje zadnjih sprememb je onemogočeno.',
'rcpatroldisabledtext'       => 'Spremljanje zadnjih sprememb je začasno onemogočeno.',
'markedaspatrollederror'     => 'Ni mogoče označiti kot pregledano',
'markedaspatrollederrortext' => 'Določite redakcijo, ki jo želite označiti kot pregledano.',

# Image deletion
'deletedrevision' => 'Prejšnja redakcija $1 je izbrisana',

# Browsing diffs
'previousdiff' => '← Pojdi na prejšnje urejanje',
'nextdiff'     => 'Novejše urejanje →',

# Media information
'mediawarning' => "'''Opozorilo''': Tovrstni tip datotek lahko vsebuje kodo, ki bi mogla ogroziti vaš sistem.
<hr />",
'imagemaxsize' => 'Slike na opisnih straneh omeji na:',
'thumbsize'    => 'Velikost sličice (thumbnail):',

# Special:NewImages
'newimages'             => 'Galerija novih datotek',
'imagelisttext'         => 'Prikazujem $1 $2 {{PLURAL:$1|razvrščeno datoteko|razvrščeni datoteki|razvrščene datoteke|razvrščenih datotek|razvrščenih datotek}}.',
'showhidebots'          => '($1 bote)',
'noimages'              => 'Nič ni videti/datoteke ni.',
'ilsubmit'              => 'Išči',
'bydate'                => 'po datumu',
'sp-newimages-showfrom' => 'Prikaži datoteke, naložene od $1 naprej',

# Metadata
'metadata'          => 'Metapodatki',
'metadata-help'     => 'Datoteka vsebuje še druge podatke, ki jih je verjetno dodal za njeno ustvaritev oziroma digitalizacijo uporabljeni fotografski aparat ali optični bralnik. Če je bila datoteka pozneje spremenjena, podatki sprememb morda ne izražajo popolnoma.',
'metadata-expand'   => 'Razširi seznam',
'metadata-collapse' => 'Skrči seznam',

# EXIF tags
'exif-imagewidth'                  => 'Širina',
'exif-imagelength'                 => 'Višina',
'exif-bitspersample'               => 'Bitov na barvni gradnik',
'exif-compression'                 => 'Shema stiskanja',
'exif-photometricinterpretation'   => 'Sestava točke',
'exif-orientation'                 => 'Usmerjenost',
'exif-samplesperpixel'             => 'Število gradnikov',
'exif-planarconfiguration'         => 'Poravnava podatkov',
'exif-ycbcrsubsampling'            => 'Podvzorčno razmerje med Y in C',
'exif-ycbcrpositioning'            => 'Razmestitev Y in C',
'exif-xresolution'                 => 'Vodoravna ločljivost',
'exif-yresolution'                 => 'Navpična ločljivost',
'exif-resolutionunit'              => 'Enota ločljivosti X in Y',
'exif-stripoffsets'                => 'Mesto podatkov slike',
'exif-rowsperstrip'                => 'Število vrstic na pas',
'exif-stripbytecounts'             => 'Zlogov na pas stiskanja.',
'exif-jpeginterchangeformat'       => 'Odtis na JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Zlogov JPEG-podatkov',
'exif-transferfunction'            => 'Funkcija prenosa',
'exif-whitepoint'                  => 'Kromatičnost bele točke',
'exif-primarychromaticities'       => 'Kromatičnosti osnovnih barv',
'exif-ycbcrcoefficients'           => 'Koeficient matrice transformacije barvnega prostora',
'exif-referenceblackwhite'         => 'Par črnih in belih referenčnih vrednosti',
'exif-datetime'                    => 'Datum in čas spremembe datoteke',
'exif-imagedescription'            => 'Naslov slike',
'exif-make'                        => 'Proizvajalec fotoaparata',
'exif-model'                       => 'Model fotoaparata',
'exif-software'                    => 'Uporabljeno programje',
'exif-artist'                      => 'Fotograf',
'exif-copyright'                   => 'Imetnik avtorskih pravic',
'exif-exifversion'                 => 'Različica Exif',
'exif-flashpixversion'             => 'Podprta različica Flashpix',
'exif-colorspace'                  => 'Barvni prostor',
'exif-componentsconfiguration'     => 'Pomen posameznih gradnikov',
'exif-compressedbitsperpixel'      => 'Velikost točke po stiskanju (v bitih)',
'exif-pixelydimension'             => 'Veljavna širina slike',
'exif-pixelxdimension'             => 'Veljavna višina slike',
'exif-makernote'                   => 'Opombe proizvajalca',
'exif-usercomment'                 => 'Uporabniške pripombe',
'exif-relatedsoundfile'            => 'Pripadajoča zvočna datoteka',
'exif-datetimeoriginal'            => 'Datum in čas ustvaritve podatkov',
'exif-datetimedigitized'           => 'Datum in čas digitalizacije',
'exif-subsectime'                  => 'Čas pomnilnika (1/100 s)',
'exif-subsectimeoriginal'          => 'Čas zajema',
'exif-subsectimedigitized'         => 'Digitalizacijski čas (1/100 s)',
'exif-exposuretime'                => 'Čas osvetlitve',
'exif-exposuretime-format'         => '$1 s ($2)',
'exif-fnumber'                     => 'Goriščno razmerje',
'exif-exposureprogram'             => 'Program osvetlitve',
'exif-spectralsensitivity'         => 'Spektralna občutljivost',
'exif-isospeedratings'             => 'Občutljivost filma ali tipala (ISO)',
'exif-oecf'                        => 'Optoelektronski pretvorbeni faktor',
'exif-shutterspeedvalue'           => 'Hitrost zaklopa',
'exif-aperturevalue'               => 'Zaslonka',
'exif-brightnessvalue'             => 'Svetlost',
'exif-exposurebiasvalue'           => 'Popravek osvetlitve',
'exif-maxaperturevalue'            => 'Največja vrednost zaslonke',
'exif-subjectdistance'             => 'Oddaljenost predmeta',
'exif-meteringmode'                => 'Način merjenja svetlobe',
'exif-lightsource'                 => 'Svetlobni vir',
'exif-flash'                       => 'Bliskavica',
'exif-focallength'                 => 'Goriščna razdalja leč',
'exif-subjectarea'                 => 'Površina predmeta',
'exif-flashenergy'                 => 'Energija bliskavice',
'exif-spatialfrequencyresponse'    => 'Odziv prostorske frekvence',
'exif-focalplanexresolution'       => 'Ločljivost goriščne ravnine X',
'exif-focalplaneyresolution'       => 'Ločljivost goriščne ravnine Y',
'exif-focalplaneresolutionunit'    => 'Enota ločljivosti goriščne ravnine',
'exif-subjectlocation'             => 'Položaj predmeta',
'exif-exposureindex'               => 'Indeks osvetlitve',
'exif-sensingmethod'               => 'Zaznavni postopek',
'exif-filesource'                  => 'Vir datoteke',
'exif-scenetype'                   => 'Vrsta prizora',
'exif-cfapattern'                  => 'Matrica filtracije barv',
'exif-customrendered'              => 'Obdelava slike po meri',
'exif-exposuremode'                => 'Nastavitev osvetlitve',
'exif-whitebalance'                => 'Ravnotežje belega',
'exif-digitalzoomratio'            => 'Razmerje digitalne povečave',
'exif-focallengthin35mmfilm'       => 'Goriščna razdalja pri 35-milimetrskem filmu',
'exif-scenecapturetype'            => 'Način zajema prizora',
'exif-gaincontrol'                 => 'Ojačanje',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Nasičenost',
'exif-sharpness'                   => 'Ostrina',
'exif-devicesettingdescription'    => 'Opis nastavitev naprave',
'exif-subjectdistancerange'        => 'Območje oddaljenosti predmeta',
'exif-imageuniqueid'               => 'ID slike',
'exif-gpsversionid'                => 'Različica GPS-oznake',
'exif-gpslatituderef'              => 'Severna ali južna zemljepisna širina',
'exif-gpslatitude'                 => 'Zemljepisna širina',
'exif-gpslongituderef'             => 'Vzhodna ali zahodna zemljepisna dolžina',
'exif-gpslongitude'                => 'Zemljepisna dolžina',
'exif-gpsaltituderef'              => 'Referenca višine',
'exif-gpsaltitude'                 => 'Višina',
'exif-gpstimestamp'                => 'GPS-čas (atomska ura)',
'exif-gpssatellites'               => 'Za merjenje uporabljeni sateliti',
'exif-gpsstatus'                   => 'Položaj sprejemnika',
'exif-gpsmeasuremode'              => 'Način merjenja',
'exif-gpsdop'                      => 'Natančnost merjenja',
'exif-gpsspeedref'                 => 'Enota hitrosti',
'exif-gpsspeed'                    => 'Hitrost GPS-sprejemnika',
'exif-gpstrackref'                 => 'Referenca smeri gibanja',
'exif-gpstrack'                    => 'Smer merjenja',
'exif-gpsimgdirectionref'          => 'Referenca smeri slike',
'exif-gpsimgdirection'             => 'Smer slike',
'exif-gpsmapdatum'                 => 'Uporabljeni geodetski podatki',
'exif-gpsdestlatituderef'          => 'Referenca zemljepisne širine cilja',
'exif-gpsdestlatitude'             => 'Zemljepisna širina cilja',
'exif-gpsdestlongituderef'         => 'Referenca zemljepisne dolžine cilja',
'exif-gpsdestlongitude'            => 'Zemljepisna dolžina cilja',
'exif-gpsdestbearingref'           => 'Referenca smeri cilja',
'exif-gpsdestbearing'              => 'Smer cilja',
'exif-gpsdestdistanceref'          => 'Referenca razdalje do cilja',
'exif-gpsdestdistance'             => 'Razdalja do cilja',
'exif-gpsprocessingmethod'         => 'Ime postopka obdelave GPS-opazovanj',
'exif-gpsareainformation'          => 'Ime GPS-območja',
'exif-gpsdatestamp'                => 'GPS-datum',
'exif-gpsdifferential'             => 'Diferencialni popravek GPS',

# EXIF attributes
'exif-compression-1' => 'Nestisnjeno',

'exif-orientation-1' => 'Navadna', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Vodoravno zrcaljeno', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Zasukano za 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Navpično zrcaljeno', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Zasukano za 90° v levo in navpično zrcaljeno', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Zasukano za 90° v desno', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Zasukano za 90° v desno in navpično zrcaljeno', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Zasukano za 90° v levo', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'grudast format',
'exif-planarconfiguration-2' => 'ravninski format',

'exif-componentsconfiguration-0' => 'ne obstaja',

'exif-exposureprogram-0' => 'Ni določen',
'exif-exposureprogram-1' => 'Ročno',
'exif-exposureprogram-2' => 'Navaden',
'exif-exposureprogram-3' => 'Prednost zaslonke',
'exif-exposureprogram-4' => 'Prednost zaklopa',
'exif-exposureprogram-5' => 'Ustvarjalni program (prednost globinske ostrine)',
'exif-exposureprogram-6' => 'Akcijski program (prednost kratke osvetlitve)',
'exif-exposureprogram-7' => 'Portretna nastavitev (fotografije od blizu, ozadje ni ostro)',
'exif-exposureprogram-8' => 'Pokrajinska nastavitev (fotografije pokrajine, ostro ozadje)',

'exif-subjectdistance-value' => '$1 {{PLURAL:$1|meter|metra|metre|metrov|metrov}}',

'exif-meteringmode-0'   => 'Neznan',
'exif-meteringmode-1'   => 'Povprečno',
'exif-meteringmode-2'   => 'Središčno obteženo povprečno',
'exif-meteringmode-3'   => 'Točkovno',
'exif-meteringmode-4'   => 'Večtočkovno',
'exif-meteringmode-5'   => 'Vzorčno',
'exif-meteringmode-6'   => 'Delno',
'exif-meteringmode-255' => 'Drugače',

'exif-lightsource-0'   => 'Neznan',
'exif-lightsource-1'   => 'Dnevna svetloba',
'exif-lightsource-2'   => 'Fluorescenčen',
'exif-lightsource-3'   => 'Volfram (žarnica)',
'exif-lightsource-4'   => 'Bliskavica',
'exif-lightsource-9'   => 'Lepo vreme',
'exif-lightsource-10'  => 'Oblačno',
'exif-lightsource-11'  => 'Senca',
'exif-lightsource-12'  => 'Dnevni fluorescenčen (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Dnevni bel fluorescenčen (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Hladen bel fluorescenčen (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Bel fluorescenčen (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Običajna svetloba A',
'exif-lightsource-18'  => 'Običajna svetloba B',
'exif-lightsource-19'  => 'Običajna svetloba C',
'exif-lightsource-24'  => 'ISO-ateljejski volfram',
'exif-lightsource-255' => 'Drugačen',

'exif-focalplaneresolutionunit-2' => 'palcev',

'exif-sensingmethod-1' => 'Nedoločen',
'exif-sensingmethod-2' => 'Enočipno barvno ploskovno tipalo',
'exif-sensingmethod-3' => 'Dvočipno barvno ploskovno tipalo',
'exif-sensingmethod-4' => 'Tričipno barvno ploskovno tipalo',
'exif-sensingmethod-5' => 'Zaporedno barvno ploskovno tipalo',
'exif-sensingmethod-7' => 'Trikratno tipalo',
'exif-sensingmethod-8' => 'Zaporedno barvno črtno tipalo',

'exif-scenetype-1' => 'Neposredno fotografirana slika',

'exif-customrendered-0' => 'Navaden postopek',
'exif-customrendered-1' => 'Prilagojen postopek',

'exif-exposuremode-0' => 'Samodejno',
'exif-exposuremode-1' => 'Ročno',
'exif-exposuremode-2' => 'Samodejna konzola',

'exif-whitebalance-0' => 'Samodejno',
'exif-whitebalance-1' => 'Ročno',

'exif-scenecapturetype-0' => 'Navadni',
'exif-scenecapturetype-1' => 'Pokrajina',
'exif-scenecapturetype-2' => 'Portret',
'exif-scenecapturetype-3' => 'Nočni prizor',

'exif-gaincontrol-0' => 'Brez',
'exif-gaincontrol-1' => 'Nizko ojačanje zgoraj',
'exif-gaincontrol-2' => 'Visoko ojačanje zgoraj',
'exif-gaincontrol-3' => 'Nizko ojačanje spodaj',
'exif-gaincontrol-4' => 'Visoko ojačanje spodaj',

'exif-contrast-0' => 'Navaden',
'exif-contrast-1' => 'Nizek',
'exif-contrast-2' => 'Visok',

'exif-saturation-0' => 'Navadna',
'exif-saturation-1' => 'Nizka nasičenost',
'exif-saturation-2' => 'Visoka nasičenost',

'exif-sharpness-0' => 'Navadna',
'exif-sharpness-1' => 'Mehka',
'exif-sharpness-2' => 'Trda',

'exif-subjectdistancerange-0' => 'Neznano',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Pogled od blizu',
'exif-subjectdistancerange-3' => 'Pogled od daleč',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Severna zemljepisna širina',
'exif-gpslatitude-s' => 'Južna zemljepisna širina',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Vzhodna zemljepisna dolžina',
'exif-gpslongitude-w' => 'Zahodna zemljepisna dolžina',

'exif-gpsstatus-a' => 'Merjenje poteka',
'exif-gpsstatus-v' => 'Interoperabilnost merjenja',

'exif-gpsmeasuremode-2' => 'Dvorazsežnostno merjenje',
'exif-gpsmeasuremode-3' => 'Trirazsežnostno merjenje',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometri na uro',
'exif-gpsspeed-m' => 'Milje na uro',
'exif-gpsspeed-n' => 'Vozli',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Pravi azimut',
'exif-gpsdirection-m' => 'Magnetni azimut',

# External editor support
'edit-externally'      => 'Uredite datoteko z uporabo zunanjega orodja',
'edit-externally-help' => 'Za več informacij glej [http://www.mediawiki.org/wiki/Manual:External_editors navodila za namestitev].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'vse',
'imagelistall'     => 'vse',
'watchlistall2'    => 'vse',
'namespacesall'    => 'vse',

# E-mail address confirmation
'confirmemail'            => 'Potrditev naslova elektronske pošte',
'confirmemail_text'       => 'Za uporabo e-poštnih možnosti {{GRAMMAR:rodilnik|{{SITENAME}}}} morate najprej potrditi svoj e-poštni naslov.
S klikom spodnjega gumba pošljite nanj potrditveno sporočilo in sledite prejetim navodilom.
Ali ste svoj e-poštni naslov že potrdili, lahko preverite v nastavitvah.',
'confirmemail_send'       => 'Pošlji mi potrditveno sporočilo',
'confirmemail_sent'       => 'Potrditveno e-sporočilo je bilo poslano.',
'confirmemail_sendfailed' => 'Potrditvenega sporočila ni bilo mogoče poslati. Prosimo, preverite, če niste naslova vnesli napačno.',
'confirmemail_invalid'    => 'Potrditveno geslo je neveljavno. Morda je poteklo.',
'confirmemail_needlogin'  => 'Za potrditev svojega e-poštnega se morate $1.',
'confirmemail_success'    => 'Vaš e-poštni naslov je potrjen. Zdaj se lahko prijavite in uporabljate wiki.',
'confirmemail_loggedin'   => 'Svoj elektronski naslov ste uspešno potrdili.',
'confirmemail_error'      => 'Vaša potrditev se žal ni shranila.',
'confirmemail_subject'    => 'Potrditev e-poštnega naslova',
'confirmemail_body'       => 'Nekdo z IP-naslovom »$1« (verjetno vi) je v {{GRAMMAR:dajalnik|{{SITENAME}}}} ustvaril račun »$2« in zanj vpisal vaš elektronski naslov. Da bi potrdili, da ta resnično pripada vam in s tem lahko začeli uporabljati e-poštne storitve {{GRAMMAR:rodilnik|{{SITENAME}}}}, odprite naslednjo povezavo: 

$3

Če tega niste napravili vi, povezavi ne sledite. Potrditveno geslo bo poteklo do $4.',

# Scary transclusion
'scarytranscludedisabled' => '[prevključevanje med wikiji je onemogočeno]',
'scarytranscludefailed'   => '[pridobivanje predloge za $1 žal ni uspelo]',
'scarytranscludetoolong'  => '[Spletni naslov je žal predolg; se opravičujemo]',

# Trackbacks
'trackbackbox'      => "<div id='mw_trackbacks'>
Sledilniki članka:<br />
$1
</div>",
'trackbackremove'   => ' ([Izbris $1])',
'trackbacklink'     => 'Sledilnik',
'trackbackdeleteok' => 'Sledilnik je uspešno izbrisan.',

# Delete conflict
'deletedwhileediting' => 'Opozorilo: Med vašim urejanjem je eden izmed administratorjev stran izbrisal.',
'confirmrecreate'     => "Medtem ko ste stran urejali, jo je uporabnik [[User:$1|$1]] ([[User talk:$1|pogovor]]) izbrisal z razlogom: 
:''$2'' 
Prosimo, potrdite, da jo resnično želite znova ustvariti.",
'recreate'            => 'Ponovno ustvari',

'unit-pixel' => ' točk',

# HTML dump
'redirectingto' => 'Preusmerjam na »[[:$1]]« ...',

# action=purge
'confirm_purge'        => 'Osvežim predpomnjenje strani? 
 
$1',
'confirm_purge_button' => 'Osveži',

# AJAX search
'searchcontaining' => "Poišči članke, ki vsebujejo ''$1''.",
'searchnamed'      => "Poišči članke, ki se imenujejo ''$1''.",
'articletitles'    => "Članki, ki se začnejo na ''$1''",
'hideresults'      => 'Skrij zadetke',

# Auto-summaries
'autoredircomment' => 'preusmeritev na [[$1]]',

# Special:Version
'version' => 'Različica', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Posebne strani',

);
