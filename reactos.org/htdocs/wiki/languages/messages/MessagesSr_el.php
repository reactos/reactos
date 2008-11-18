<?php
/** latinica (latinica)
 *
 * @ingroup Language
 * @file
 *
 * @author Red Baron
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => "Medija",
	NS_SPECIAL          => "Posebno",
	NS_MAIN             => "",
	NS_TALK             => "Razgovor",
	NS_USER             => "Korisnik",
	NS_USER_TALK        => "Razgovor_sa_korisnikom",
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => "Razgovor_o_$1",
	NS_IMAGE            => "Slika",
	NS_IMAGE_TALK       => "Razgovor_o_slici",
	NS_MEDIAWIKI        => "MedijaViki",
	NS_MEDIAWIKI_TALK   => "Razgovor_o_MedijaVikiju",
	NS_TEMPLATE         => 'Šablon',
	NS_TEMPLATE_TALK    => 'Razgovor_o_šablonu',
	NS_HELP             => 'Pomoć',
	NS_HELP_TALK        => 'Razgovor_o_pomoći',
	NS_CATEGORY         => 'Kategorija',
	NS_CATEGORY_TALK    => 'Razgovor_o_kategoriji',
);

# Aliases to cyrillic namespaces 
$namespaceAliases = array(
	"Медија"                  => NS_MEDIA,
	"Посебно"                 => NS_SPECIAL,
	"Разговор"                => NS_TALK,
	"Корисник"                => NS_USER,
	"Разговор_са_корисником"  => NS_USER_TALK,
	"Разговор_о_$1"           => NS_PROJECT_TALK,
	"Слика"                   => NS_IMAGE,
	"Разговор_о_слици"        => NS_IMAGE_TALK,
	"МедијаВики"              => NS_MEDIAWIKI,
	"Разговор_о_МедијаВикију" => NS_MEDIAWIKI_TALK,
	'Шаблон'                  => NS_TEMPLATE,
	'Разговор_о_шаблону'      => NS_TEMPLATE_TALK,
	'Помоћ'                   => NS_HELP,
	'Разговор_о_помоћи'       => NS_HELP_TALK,
	'Категорија'              => NS_CATEGORY,
	'Разговор_о_категорији'   => NS_CATEGORY_TALK,
);

$skinNames = array(
 "Obična", "Nostalgija", "Kelnsko plavo"
);

$extraUserToggles = array(
	'nolangconversion',
);

$datePreferenceMigrationMap = array(
	'default',
	'hh:mm d. month y.',
	'hh:mm d month y',
	'hh:mm dd.mm.yyyy',
	'hh:mm d.m.yyyy',
	'hh:mm d. mon y.',
	'hh:mm d mon y',
	'h:mm d. month y.',
	'h:mm d month y',
	'h:mm dd.mm.yyyy',
	'h:mm d.m.yyyy',
	'h:mm d. mon y.',
	'h:mm d mon y',
);

$datePreferences = array(
	'default',
	'hh:mm d. month y.',
	'hh:mm d month y',
	'hh:mm dd.mm.yyyy',
	'hh:mm d.m.yyyy',
	'hh:mm d. mon y.',
	'hh:mm d mon y',
	'h:mm d. month y.',
	'h:mm d month y',
	'h:mm dd.mm.yyyy',
	'h:mm d.m.yyyy',
	'h:mm d. mon y.',
	'h:mm d mon y',
);

$defaultDateFormat = 'hh:mm d. month y.';

$dateFormats = array(
	/*
	'Није битно',
	'06:12, 5. јануар 2001.',
	'06:12, 5 јануар 2001',
	'06:12, 05.01.2001.',
	'06:12, 5.1.2001.',
	'06:12, 5. јан 2001.',
	'06:12, 5 јан 2001',
	'6:12, 5. јануар 2001.',
	'6:12, 5 јануар 2001',
	'6:12, 05.01.2001.',
	'6:12, 5.1.2001.',
	'6:12, 5. јан 2001.',
	'6:12, 5 јан 2001',
	 */
	
	'hh:mm d. month y. time'    => 'H:i',
	'hh:mm d month y time'      => 'H:i',
	'hh:mm dd.mm.yyyy time'     => 'H:i',
	'hh:mm d.m.yyyy time'       => 'H:i',
	'hh:mm d. mon y. time'      => 'H:i',
	'hh:mm d mon y time'        => 'H:i',
	'h:mm d. month y. time'     => 'G:i',
	'h:mm d month y time'       => 'G:i',
	'h:mm dd.mm.yyyy time'      => 'G:i',
	'h:mm d.m.yyyy time'        => 'G:i',
	'h:mm d. mon y. time'       => 'G:i',
	'h:mm d mon y time'         => 'G:i',

	'hh:mm d. month y. date'    => 'j. F Y.',
	'hh:mm d month y date'      => 'j F Y',  
	'hh:mm dd.mm.yyyy date'     => 'd.m.Y',  
	'hh:mm d.m.yyyy date'       => 'j.n.Y',  
	'hh:mm d. mon y. date'      => 'j. M Y.',
	'hh:mm d mon y date'        => 'j M Y',  
	'h:mm d. month y. date'     => 'j. F Y.',
	'h:mm d month y date'       => 'j F Y',  
	'h:mm dd.mm.yyyy date'      => 'd.m.Y',  
	'h:mm d.m.yyyy date'        => 'j.n.Y',  
	'h:mm d. mon y. date'       => 'j. M Y.',
	'h:mm d mon y date'         => 'j M Y',  

	'hh:mm d. month y. both'    =>'H:i, j. F Y.', 
	'hh:mm d month y both'      =>'H:i, j F Y',   
	'hh:mm dd.mm.yyyy both'     =>'H:i, d.m.Y',   
	'hh:mm d.m.yyyy both'       =>'H:i, j.n.Y',   
	'hh:mm d. mon y. both'      =>'H:i, j. M Y.', 
	'hh:mm d mon y both'        =>'H:i, j M Y',   
	'h:mm d. month y. both'     =>'G:i, j. F Y.', 
	'h:mm d month y both'       =>'G:i, j F Y',   
	'h:mm dd.mm.yyyy both'      =>'G:i, d.m.Y',   
	'h:mm d.m.yyyy both'        =>'G:i, j.n.Y',   
	'h:mm d. mon y. both'       =>'G:i, j. M Y.', 
	'h:mm d mon y both'         =>'G:i, j M Y',   
);


/* NOT USED IN STABLE VERSION */
$magicWords = array(
#	ID                                CASE SYNONYMS
	'redirect'               => array( 0, '#Preusmeri', '#redirect', '#preusmeri', '#PREUSMERI' ),
	'notoc'                  => array( 0, '__NOTOC__', '__BEZSADRŽAJA__' ),
	'forcetoc'               => array( 0, '__FORCETOC__', '__FORSIRANISADRŽAJ__' ),
	'toc'                    => array( 0, '__TOC__', '__SADRŽAJ__' ),
	'noeditsection'          => array( 0, '__NOEDITSECTION__', '__BEZ_IZMENA__', '__BEZIZMENA__' ),
	'currentmonth'           => array( 1, 'CURRENTMONTH', 'TRENUTNIMESEC' ),
	'currentmonthname'       => array( 1, 'CURRENTMONTHNAME', 'TRENUTNIMESECIME' ),
	'currentmonthnamegen'    => array( 1, 'CURRENTMONTHNAMEGEN', 'TRENUTNIMESECGEN' ),
	'currentmonthabbrev'     => array( 1, 'CURRENTMONTHABBREV', 'TRENUTNIMESECSKR' ),
	'currentday'             => array( 1, 'CURRENTDAY', 'TRENUTNIDAN' ),
	'currentdayname'         => array( 1, 'CURRENTDAYNAME', 'TRENUTNIDANIME' ),
	'currentyear'            => array( 1, 'CURRENTYEAR', 'TRENUTNAGODINA' ),
	'currenttime'            => array( 1, 'CURRENTTIME', 'TRENUTNOVREME' ),
	'numberofarticles'       => array( 1, 'NUMBEROFARTICLES', 'BROJČLANAKA' ),
	'numberoffiles'          => array( 1, 'NUMBEROFFILES', 'BROJDATOTEKA', 'BROJFAJLOVA' ),
	'pagename'               => array( 1, 'PAGENAME', 'STRANICA' ),
	'pagenamee'              => array( 1, 'PAGENAMEE', 'STRANICE' ),
	'namespace'              => array( 1, 'NAMESPACE', 'IMENSKIPROSTOR' ),
	'namespacee'             => array( 1, 'NAMESPACEE', 'IMENSKIPROSTORI' ),
	'fullpagename'           => array( 1, 'FULLPAGENAME', 'PUNOIMESTRANE' ),
	'fullpagenamee'          => array( 1, 'FULLPAGENAMEE', 'PUNOIMESTRANEE' ),
	'msg'                    => array( 0, 'MSG:', 'POR:' ),
	'subst'                  => array( 0, 'SUBST:', 'ZAMENI:' ),
	'msgnw'                  => array( 0, 'MSGNW:', 'NVPOR:' ),
	'img_thumbnail'          => array( 1, 'thumbnail', 'thumb', 'mini' ),
	'img_manualthumb'        => array( 1, 'thumbnail=$1', 'thumb=$1', 'mini=$1' ),
	'img_right'              => array( 1, 'right', 'desno', 'd' ),
	'img_left'               => array( 1, 'left', 'levo', 'l' ),
	'img_none'               => array( 1, 'none', 'n', 'bez' ),
	'img_width'              => array( 1, '$1px', '$1piskel' , '$1p' ),
	'img_center'             => array( 1, 'center', 'centre', 'centar', 'c' ),
	'img_framed'             => array( 1, 'framed', 'enframed', 'frame', 'okvir', 'ram' ),
	'sitename'               => array( 1, 'SITENAME', 'IMESAJTA' ),
	'ns'                     => array( 0, 'NS:', 'IP:' ),
	'localurl'               => array( 0, 'LOCALURL:', 'LOKALNAADRESA:' ),
	'localurle'              => array( 0, 'LOCALURLE:', 'LOKALNEADRESE:' ),
	'servername'             => array( 0, 'SERVERNAME', 'IMESERVERA' ),
	'scriptpath'             => array( 0, 'SCRIPTPATH', 'SKRIPTA' ),
	'grammar'                => array( 0, 'GRAMMAR:', 'GRAMATIKA:' ),
	'notitleconvert'         => array( 0, '__NOTITLECONVERT__', '__NOTC__', '__БЕЗКН__', '__BEZKN__' ),
	'nocontentconvert'       => array( 0, '__NOCONTENTCONVERT__', '__NOCC__', '__BEZCC__' ),
	'currentweek'            => array( 1, 'CURRENTWEEK', 'TRENUTNANEDELJA' ),
	'currentdow'             => array( 1, 'CURRENTDOW', 'TRENUTNIDOV' ),
	'revisionid'             => array( 1, 'REVISIONID', 'IDREVIZIJE' ),
	'plural'                 => array( 0, 'PLURAL:', 'MNOŽINA:' ),
	'fullurl'                => array( 0, 'FULLURL:', 'PUNURL:' ),
	'fullurle'               => array( 0, 'FULLURLE:', 'PUNURLE:' ),
	'lcfirst'                => array( 0, 'LCFIRST:', 'LCPRVI:' ),
	'ucfirst'                => array( 0, 'UCFIRST:', 'UCPRVI:' ),
);

$separatorTransformTable = array(',' => '.', '.' => ',' );

$messages = array(
# User preference toggles
'tog-underline'               => 'Podvuci veze',
'tog-highlightbroken'         => 'Formatiraj pokvarene veze <a href="" class="new">ovako</a> (alternativa: ovako<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Uravnaj pasuse',
'tog-hideminor'               => 'Sakrij male izmene u spisku skorašnjih izmena',
'tog-extendwatchlist'         => 'Poboljšan spisak nadgledanja',
'tog-usenewrc'                => 'Poboljšan spisak skorašnjih izmena (zahteva JavaScript)',
'tog-numberheadings'          => 'Automatski numeriši podnaslove',
'tog-showtoolbar'             => 'Prikaži dugmiće za izmene (zahteva JavaScript)',
'tog-editondblclick'          => 'Menjaj stranice dvostrukim klikom (zahteva JavaScript)',
'tog-editsection'             => 'Omogući izmenu delova [uredi] vezama',
'tog-editsectiononrightclick' => 'Omogući izmenu delova desnim klikom<br />na njihove naslove (zahteva JavaScript)',
'tog-showtoc'                 => 'Prikaži sadržaj (u člancima sa više od 3 podnaslova)',
'tog-rememberpassword'        => 'Pamti lozinku kroz više seansi',
'tog-editwidth'               => 'Polje za izmene ima punu širinu',
'tog-watchcreations'          => 'Dodaj stranice koje pravim u moj spisak nadgledanja',
'tog-watchdefault'            => 'Dodaj stranice koje menjam u moj spisak nadgledanja',
'tog-minordefault'            => 'Označi sve izmene malim isprva',
'tog-previewontop'            => 'Prikaži pretpregled pre polja za izmenu',
'tog-previewonfirst'          => 'Prikaži pretpregled pri prvoj izmeni',
'tog-nocache'                 => 'Onemogući keširanje stranica',
'tog-enotifwatchlistpages'    => 'Pošalji mi e-poštu kada se promeni strana koju nadgledam',
'tog-enotifusertalkpages'     => 'Pošalji mi e-poštu kada se promeni moja korisnička strana za razgovor',
'tog-enotifminoredits'        => 'Pošalji mi e-poštu takođe za male izmene strana',
'tog-enotifrevealaddr'        => 'Otkrij adresu moje e-pošte u pošti obaveštenja',
'tog-shownumberswatching'     => 'Prikaži broj korisnika koji nadgledaju',
'tog-fancysig'                => 'Čist potpis (bez automatskih veza)',
'tog-externaleditor'          => 'Koristi spoljašnji uređivač po podrazumevanim podešavanjima',
'tog-externaldiff'            => 'Koristi spoljašnji program za prikaz razlika po podrazumevanim podešavanjima',
'tog-showjumplinks'           => 'Omogući "skoči na" veze',
'tog-uselivepreview'          => 'Koristi živi pretpregled (zahteva JavaScript) (eksperimentalno)',
'tog-forceeditsummary'        => 'Upozori me kad ne unesem opis izmene',
'tog-watchlisthideown'        => 'Sakrij moje izmene sa spiska nadgledanja',
'tog-watchlisthidebots'       => 'Sakrij izmene botova sa spiska nadgledanja',
'tog-nolangconversion'        => 'Isključi konverziju varijanti',

'underline-always'  => 'Uvek',
'underline-never'   => 'Nikad',
'underline-default' => 'Po podešavanjima brauzera',

'skinpreview' => '(Pregled)',

# Dates
'sunday'        => 'nedelja',
'monday'        => 'ponedeljak',
'tuesday'       => 'utorak',
'wednesday'     => 'sreda',
'thursday'      => 'četvrtak',
'friday'        => 'petak',
'saturday'      => 'subota',
'sun'           => 'ned',
'mon'           => 'pon',
'tue'           => 'uto',
'wed'           => 'sre',
'thu'           => 'čet',
'fri'           => 'pet',
'sat'           => 'sub',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'mart',
'april'         => 'april',
'may_long'      => 'maj',
'june'          => 'jun',
'july'          => 'jul',
'august'        => 'avgust',
'september'     => 'septembar',
'october'       => 'oktobar',
'november'      => 'novembar',
'december'      => 'decembar',
'january-gen'   => 'januara',
'february-gen'  => 'februara',
'march-gen'     => 'marta',
'april-gen'     => 'aprila',
'may-gen'       => 'maja',
'june-gen'      => 'juna',
'july-gen'      => 'jula',
'august-gen'    => 'avgusta',
'september-gen' => 'septembra',
'october-gen'   => 'oktobra',
'november-gen'  => 'novembra',
'december-gen'  => 'decembra',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'maj',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'avg',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'         => '{{PLURAL:$1|Kategorija|Kategorije|Kategorije}} stranica',
'category_header'        => 'Članaka u kategoriji "$1"',
'subcategories'          => 'Potkategorije',
'listingcontinuesabbrev' => 'nast.',

'mainpagetext'      => "<big>'''MedijaViki je uspešno instaliran.'''</big>",
'mainpagedocfooter' => 'Molimo vidite [http://meta.wikimedia.org/wiki/Help:Contents korisnički vodič] za informacije o upotrebi viki softvera.

== Za početak ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Pomoć u vezi sa podešavanjima]
* [http://www.mediawiki.org/wiki/Manual:FAQ Najčešće postavljena pitanja]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Mejling lista o izdanjima MedijaVikija]',

'about'          => 'O...',
'article'        => 'Članak',
'newwindow'      => '(novi prozor)',
'cancel'         => 'Poništi',
'qbfind'         => 'Pronađi',
'qbbrowse'       => 'Prelistavaj',
'qbedit'         => 'Izmeni',
'qbpageoptions'  => 'Opcije stranice',
'qbpageinfo'     => 'Informacije o stranici',
'qbmyoptions'    => 'Moje opcije',
'qbspecialpages' => 'Posebne stranice',
'moredotdotdot'  => 'Još...',
'mypage'         => 'Moja stranica',
'mytalk'         => 'Moj razgovor',
'anontalk'       => 'Razgovor za ovu IP adresu',
'navigation'     => 'Navigacija',
'and'            => 'i',

# Metadata in edit box
'metadata_help' => 'Metapodaci:',

'errorpagetitle'    => 'Greška',
'returnto'          => 'Povratak na $1.',
'tagline'           => 'Iz projekta {{SITENAME}}',
'help'              => 'Pomoć',
'search'            => 'pretraga',
'searchbutton'      => 'Traži',
'go'                => 'Idi',
'searcharticle'     => 'Idi',
'history'           => 'Istorija stranice',
'history_short'     => 'istorija',
'updatedmarker'     => 'ažurirano od moje poslednje posete',
'info_short'        => 'Informacije',
'printableversion'  => 'Verzija za štampu',
'permalink'         => 'Permalink',
'print'             => 'Štampa',
'edit'              => 'Uredi',
'editthispage'      => 'Uredi ovu stranicu',
'delete'            => 'obriši',
'deletethispage'    => 'Obriši ovu stranicu',
'undelete_short'    => 'vrati {{PLURAL:$1|jednu obrisanu izmenu|$1 obrisane izmene|$1 obrisanih izmena}}',
'protect'           => 'zaštiti',
'protectthispage'   => 'Zaštiti ovu stranicu',
'unprotect'         => 'Skloni zaštitu',
'unprotectthispage' => 'Skloni zaštitu sa ove stranice',
'newpage'           => 'Nova stranica',
'talkpage'          => 'Razgovor o ovoj stranici',
'specialpage'       => 'Posebna stranica',
'personaltools'     => 'Lični alati',
'postcomment'       => 'Pošalji komentar',
'articlepage'       => 'Pogledaj članak',
'talk'              => 'Razgovor',
'views'             => 'Pregledi',
'toolbox'           => 'alati',
'userpage'          => 'Pogledaj korisničku stranu',
'projectpage'       => 'Pogledaj stranu projekta',
'imagepage'         => 'Pogledaj stranu slike',
'mediawikipage'     => 'Vidi stranicu poruke',
'templatepage'      => 'Vidi stranicu šablona',
'viewhelppage'      => 'Vidi stranicu pomoći',
'categorypage'      => 'Vidi stranicu kategorije',
'viewtalkpage'      => 'Pogledaj razgovor',
'otherlanguages'    => 'Ostali jezici',
'redirectedfrom'    => '(Preusmereno sa $1)',
'redirectpagesub'   => 'Strana preusmerenja',
'lastmodifiedat'    => 'Ova stranica je poslednji put izmenjena $2, $1.', # $1 date, $2 time
'viewcount'         => 'Ovoj stranici je pristupljeno {{PLURAL:$1|jednom|$1 puta|$1 puta}}.',
'protectedpage'     => 'Zaštićena stranica',
'jumpto'            => 'Skoči na:',
'jumptonavigation'  => 'navigacija',
'jumptosearch'      => 'pretraga',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'O projektu {{SITENAME}}',
'aboutpage'            => 'Project:O',
'bugreports'           => 'Prijave grešaka',
'bugreportspage'       => 'Project:Prijave_grešaka',
'copyright'            => 'Sadržaj je objavljen pod $1.',
'copyrightpagename'    => 'Autorska prava projekta {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Autorska prava',
'currentevents'        => 'Trenutni događaji',
'currentevents-url'    => 'Project:Trenutni događaji',
'disclaimers'          => 'Odricanje odgovornosti',
'disclaimerpage'       => 'Project:Odricanje odgovornosti',
'edithelp'             => 'Pomoć oko uređivanja',
'edithelppage'         => 'Help:Uređivanje',
'faq'                  => 'NPP',
'faqpage'              => 'Project:NPP',
'helppage'             => 'Help:Sadržaj',
'mainpage'             => 'Glavna strana',
'mainpage-description' => 'Glavna strana',
'portal'               => 'Radionica',
'portal-url'           => 'Project:Radionica',
'privacy'              => 'Politika privatnosti',
'privacypage'          => 'Project:Politika privatnosti',

'badaccess'        => 'Greška u dozvolama',
'badaccess-group0' => 'Nije vam dozvoljeno da izvršite akciju koju ste pokrenuli.',
'badaccess-group1' => 'Akcija koju ste pokrenuli je rezerevisana za korisnike u grupi $1.',
'badaccess-group2' => 'Akcija koju ste pokrenuli je rezervisana za korisnike iz jedne od grupa $1.',
'badaccess-groups' => 'Akcija koju ste pokrenuli je rezervisana za korisnike iz jedne od grupa $1.',

'versionrequired'     => 'Verzija $1 MedijaVikija je potrebna',
'versionrequiredtext' => 'Verzija $1 MedijaVikija je potrebna da bi se koristila ova strana. Pogledajte [[Special:Version|verziju]]',

'ok'                      => 'da',
'retrievedfrom'           => 'Dobavljeno iz "$1"',
'youhavenewmessages'      => 'Imate $1 ($2).',
'newmessageslink'         => 'novih poruka',
'newmessagesdifflink'     => 'najsvežije izmene',
'youhavenewmessagesmulti' => 'Imate novih poruka na $1',
'editsection'             => 'uredi',
'editold'                 => 'uredi',
'editsectionhint'         => 'Uredi deo: $1',
'toc'                     => 'Sadržaj',
'showtoc'                 => 'prikaži',
'hidetoc'                 => 'sakrij',
'thisisdeleted'           => 'Pogledaj ili vrati $1?',
'viewdeleted'             => 'Pogledaj $1?',
'restorelink'             => '{{PLURAL:$1|jedna obrisana izmena|$1 obrisane izmene|$1 obrisanih izmena}}',
'feedlinks'               => 'Fid:',
'feed-invalid'            => 'Loš tip fida prijave.',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Članak',
'nstab-user'      => 'Korisnička strana',
'nstab-media'     => 'Medij',
'nstab-special'   => 'Posebna',
'nstab-project'   => 'Strana projekta',
'nstab-image'     => 'Slika',
'nstab-mediawiki' => 'Poruka',
'nstab-template'  => 'Šablon',
'nstab-help'      => 'Pomoć',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Nema takve akcije',
'nosuchactiontext'  => 'Akciju navedenu u URL-u viki softver
nije prepoznao.',
'nosuchspecialpage' => 'Nema takve posebne stranice',
'nospecialpagetext' => "<big>'''Tražili ste nepostojeću posebnu stranicu.'''</big>

Spisak svih posebnih stranica se može naći na [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Greška',
'databaseerror'        => 'Greška u bazi',
'dberrortext'          => 'Desila se sintaksna greška upita baze.
Ovo možda ukazuje na greške u softveru.
Poslednji pokušani upit je bio:
<blockquote><tt>$1</tt></blockquote>
iz funkcije "<tt>$2</tt>".
MySQL je vratio grešku "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Desila se sintaksna greška upita baze.
Poslednji pokušani upit je bio:
"$1"
iz funkcije "$2".
MySQL je vratio grešku "$3: $4".',
'noconnect'            => 'Žao nam je! Viki ima tehničke poteškoće, i ne može da se poveže se serverom baze.',
'nodb'                 => 'Ne mogu da izaberem bazu $1',
'cachederror'          => 'Ovo je keširana kopija zahtevane stranice, i možda nije ažurirana.',
'laggedslavemode'      => 'Upozorenje: moguće je da strana nije skoro ažurirana.',
'readonly'             => 'Baza je zaključana',
'enterlockreason'      => 'Unesite razlog za zaključavanje, uključujući procenu
vremena otključavanja',
'readonlytext'         => 'Baza podataka je trenutno zaključana za nove
unose i ostale izmene, verovatno zbog rutinskog održavanja,
posle čega će biti vraćena u uobičajeno stanje.
Administrator koji ju je zaključao dao je ovo objašnjenje: $1',
'readonly_lag'         => 'Baza podataka je automatski zaključana dok slejv serveri ne sustignu master',
'internalerror'        => 'Interna greška',
'filecopyerror'        => 'Ne mogu da iskopiram fajl "$1" na "$2".',
'filerenameerror'      => 'Ne mogu da preimenujem fajl "$1" u "$2".',
'filedeleteerror'      => 'Ne mogu da obrišem fajl "$1".',
'filenotfound'         => 'Ne mogu da nađem fajl "$1".',
'unexpected'           => 'Neočekivana vrednost: "$1"="$2".',
'formerror'            => 'Greška: ne mogu da pošaljem upitnik',
'badarticleerror'      => 'Ova akcija ne može biti izvršena na ovoj stranici.',
'cannotdelete'         => 'Ne mogu da obrišem navedenu stranicu ili fajl. (Moguće je da je neko drugi već obrisao.)',
'badtitle'             => 'Loš naslov',
'badtitletext'         => 'Zahtevani naslov stranice je bio neispravan, prazan ili
neispravno povezan međujezički ili interviki naslov. Možda sadrži jedan ili više karaktera koji ne mogu da se upotrebljavaju u naslovima.',
'perfdisabled'         => 'Žao nam je! Ova mogućnost je privremeno onemogućena jer usporava bazu do te mere da više niko ne može da koristi viki.',
'perfcached'           => 'Sledeći podaci su keširani i ne moraju biti u potpunosti ažurirani.',
'perfcachedts'         => 'Sledeći podaci su keširani i poslednji put su ažurirani: $1',
'wrong_wfQuery_params' => 'Netačni parametri za wfQuery()<br />
Funkcija: $1<br />
Pretraga: $2',
'viewsource'           => 'pogledaj kod',
'viewsourcefor'        => 'za $1',
'protectedinterface'   => "'''Upozorenje:''' Menjate stranu koja se koristi da pruži tekst interfejsa za softver. Izmene na ovoj strani će uticati na izgled korisničkog interfejsa za ostale korisnike.",
'editinginterface'     => "'''Upozorenje:''' Uređujete stranicu čija je namena upisivanje teksta za interfejs softvera. Izmene u ovoj stranici će promeniti izgled korisničkog intefejsa svih korisnika.",
'sqlhidden'            => '(SQL pretraga sakrivena)',

# Login and logout pages
'logouttitle'                => 'Odjavi se',
'logouttext'                 => '<strong>Sada ste odjavljeni.</strong><br />
Možete da nastavite da koristite projekat {{SITENAME}} anonimno, ili se ponovo prijaviti kao drugi korisnik. Obratite pažnju da neke stranice mogu nastaviti da se prikazuju kao da ste još uvek prijavljeni, dok ne očistite keš svog brauzera.',
'welcomecreation'            => '== Dobrodošli, $1! ==

Vaš nalog je napravljen.
Ne zaboravite da prilagodite sebi svoja {{SITENAME}} podešavanja.',
'loginpagetitle'             => 'Prijavljivanje',
'yourname'                   => 'Korisničko ime',
'yourpassword'               => 'Vaša lozinka',
'yourpasswordagain'          => 'Ponovite lozinku',
'remembermypassword'         => 'Zapamti me',
'yourdomainname'             => 'Vaš domen',
'externaldberror'            => 'Došlo je ili do greške pri spoljašnjoj autentifikaciji baze podataka ili vam nije dozvoljeno da ažurirate svoj spoljašnji nalog.',
'loginproblem'               => '<b>Bilo je problema sa vašim prijavljivanjem.</b><br />Pokušajte ponovo!',
'login'                      => 'Prijavi se',
'loginprompt'                => "Morate da imate omogućene kolačiće (''cookies'') da biste se prijavili na {{SITENAME}}.",
'userlogin'                  => 'Registruj se / Prijavi se',
'logout'                     => 'Odjavi se',
'userlogout'                 => 'Odjavi se',
'notloggedin'                => 'Niste prijavljeni',
'nologin'                    => 'Nemate nalog? $1.',
'nologinlink'                => 'Napravite nalog',
'createaccount'              => 'Napravi nalog',
'gotaccount'                 => 'Već imate nalog? $1.',
'gotaccountlink'             => 'Prijavi se',
'createaccountmail'          => 'e-poštom',
'badretype'                  => 'Lozinke koje ste uneli se ne poklapaju.',
'userexists'                 => 'Korisničko ime koje ste uneli već je u upotrebi. Molimo izaberite drugo ime.',
'youremail'                  => 'Adresa vaše e-pošte *',
'username'                   => 'Korisničko ime:',
'uid'                        => 'Korisnički ID:',
'yourrealname'               => 'Vaše pravo ime *',
'yourlanguage'               => 'Jezik:',
'yourvariant'                => 'Varijanta:',
'yournick'                   => 'Nadimak:',
'badsig'                     => 'Greška u potpisu; proverite HTML tagove.',
'email'                      => 'E-pošta',
'prefs-help-realname'        => '* Pravo ime (opciono): ako izaberete da date ime, ovo će biti korišćeno za pripisivanje za vaš rad.',
'loginerror'                 => 'Greška pri prijavljivanju',
'prefs-help-email'           => '* E-pošta (opciono): Omogućuje ostalima da vas kontaktiraju preko vaše korisničke strane ili strane za razgovora bez potrebe da odajete svoj identitet.',
'nocookiesnew'               => "Korisnički nalog je napravljen, ali niste prijavljeni. {{SITENAME}} koristi kolačiće (''cookies'') da bi se korisnici prijavili. Vi ste onemogućili kolačiće na svom računaru. Molimo omogućite ih, a onda se prijavite sa svojim novim korisničkim imenom i lozinkom.",
'nocookieslogin'             => "{{SITENAME}} koristi kolačiće (''cookies'') da bi se korisnici prijavili. Vi ste onemogućili kolačiće na svom računaru. Molimo omogućite ih i pokušajte ponovo sa prijavom.",
'noname'                     => 'Niste izabrali ispravno korisničko ime.',
'loginsuccesstitle'          => 'Prijavljivanje uspešno',
'loginsuccess'               => "'''Sada ste prijavljeni na {{SITENAME}} kao \"\$1\".'''",
'nosuchuser'                 => 'Ne postoji korisnik sa imenom "$1". Proverite da li ste dobro napisali ili napravite novi korisnički nalog.',
'nosuchusershort'            => 'Ne postoji korisnik sa imenom "<nowiki>$1</nowiki>". Proverite da li ste dobro napisali.',
'nouserspecified'            => 'Morate da naznačite korisničko ime.',
'wrongpassword'              => 'Lozinka koju ste uneli je neispravna. Molimo pokušajte ponovo.',
'wrongpasswordempty'         => 'Lozinka koju ste uneli je prazna. Molimo pokušajte ponovo.',
'passwordtooshort'           => 'Vaša šifra je previše kratka.
Mora da ima bar $1 karaktera.',
'mailmypassword'             => 'Pošalji mi lozinku',
'passwordremindertitle'      => '{{SITENAME}} podsetnik za šifru',
'passwordremindertext'       => 'Neko (verovatno vi, sa IP adrese $1)
je zahtevao da vam pošaljemo novu lozinku za {{SITENAME}} ($4).
Lozinka za korisnika "$2" je sada "$3".
Sada treba da se prijavite i promenite svoju lozinku.

Ako je neko drugi podneo ovaj zahtev ili ukoliko ste se setili svoje lozinke i više ne želite da je menjate, možete da ignorišete ovu poruku i nastavite da koristite svoju staru šifru.',
'noemail'                    => 'Ne postoji adresa e-pošte za korisnika "$1".',
'passwordsent'               => 'Nova šifra je poslata na adresu e-pošte korisnika "$1".
Molimo prijavite se pošto je primite.',
'blocked-mailpassword'       => 'Vašoj IP adresi je blokiran pristup uređivanju, iz kog razloga nije moguće koristiti funkciju podsećanja lozinke, radi prevencije izvršenja nedozvoljene akcije.',
'eauthentsent'               => 'E-pošta za potvrdu je poslata na naznačenu adresu e-pošte. Pre nego što se bilo koja druga e-pošta pošalje na nalog, moraćete da pratite uputstva u e-pošti, da biste potvrdili da je nalog zaista vaš.',
'throttled-mailpassword'     => 'Podsetnik lozinke vam je već poslao jednu poruku u zadnjih $1 sati.
Radi prevencije izvršenja nedozvoljene akcije, podsetnik šalje samo jednu poruku u roku od $1 sati.',
'mailerror'                  => 'Greška pri slanju e-pošte: $1',
'acct_creation_throttle_hit' => 'Žao nam je, već ste napravili $1 korisnička imena. Više nije dozvoljeno.',
'emailauthenticated'         => 'Vaša adresa e-pošte je potvrđena: $1.',
'emailnotauthenticated'      => 'Vaša adresa e-pošte još uvek nije potvrđena. E-pošta neće biti poslata ni za jednu od sledećih mogućnosti.',
'noemailprefs'               => 'Naznačite adresu e-pošte kako bi ove mogućnosti radile.',
'emailconfirmlink'           => 'Potvrdite vašu adresu e-pošte',
'invalidemailaddress'        => 'Adresa e-pošte ne može biti primljena jer izgleda nije pravilnog formata. Molimo unesite dobro-formatiranu adresu ili ispraznite to polje.',
'accountcreated'             => 'Nalog je napravljen',
'accountcreatedtext'         => 'Korisnički nalog za $1 je napravljen.',
'loginlanguagelabel'         => 'Jezik: $1',

# Edit page toolbar
'bold_sample'     => 'podebljan tekst',
'bold_tip'        => 'podebljan tekst',
'italic_sample'   => 'kurzivan tekst',
'italic_tip'      => 'kurzivan tekst',
'link_sample'     => 'naslov veze',
'link_tip'        => 'unutrašnja veza',
'extlink_sample'  => 'http://www.example.com opis adrese',
'extlink_tip'     => 'spoljašnja veza (ne zaboravite prefiks http://)',
'headline_sample' => 'Naslov',
'headline_tip'    => 'Naslov drugog nivoa',
'math_sample'     => 'Ovde unesite formulu',
'math_tip'        => 'Matematička formula (LaTeX)',
'nowiki_sample'   => 'Dodaj neformatirani tekst ovde',
'nowiki_tip'      => 'Ignoriši viki formatiranje',
'image_sample'    => 'ime_slike.jpg',
'image_tip'       => 'Uklopljena slika',
'media_sample'    => 'ime_medija_fajla.mp3',
'media_tip'       => 'Putanja ka multimedijalnom fajlu',
'sig_tip'         => 'Vaš potpis sa trenutnim vremenom',
'hr_tip'          => 'Horizontalna linija',

# Edit pages
'summary'                   => 'Opis izmene',
'subject'                   => 'Tema/naslov',
'minoredit'                 => 'Ovo je mala izmena',
'watchthis'                 => 'Nadgledaj ovaj članak',
'savearticle'               => 'Snimi stranicu',
'preview'                   => 'Pretpregled',
'showpreview'               => 'Prikaži pretpregled',
'showlivepreview'           => 'Živi pretpregled',
'showdiff'                  => 'Prikaži promene',
'anoneditwarning'           => 'Niste prijavljeni. Vaša IP adresa će biti zabeležena u istoriji izmena ove strane.',
'missingsummary'            => "'''Podsetnik:''' Niste uneli opis izmene. Ukoliko kliknete Snimi stranicu ponovo, vaše izmene će biti snimljene bez opisa.",
'missingcommenttext'        => 'Molimo unestite komentar ispod.',
'missingcommentheader'      => "'''Podsetnik:''' Niste naveli naslov ovog komentara. Ukoliko kliknete ''Snimi ponovo'', vaš komentar će biti snimljen bez naslova.",
'blockedtitle'              => 'Korisnik je blokiran',
'blockedtext'               => "<big>'''Vaše korisničko ime ili IP adresa je blokirano.'''</big>

Blokirao vas je korisnik \$1. Razlog za blokiranje je ''\$2''.

Možete kontaktirati korisnika \$1 ili nekog drugog [[{{MediaWiki:Grouppage-sysop}}|administratora]] kako biste razgovarali o blokadi. Ne možete da koristite opciju \"Pošalji e-poštu ovom korisniku\" ukoliko nemate valjanu adresu e-pošte navedenu u vašim [[Special:Preferences|podešavanjima]]. Vaša trenutna IP adresa je \$3. Molimo uključite ovo u svaki vaš zahtev.",
'blockedoriginalsource'     => "Izvor '''$1''' je prikazan ispod:",
'blockededitsource'         => "Tekst '''vaših izmena''' za '''$1''' je prikazan ispod:",
'whitelistedittitle'        => 'Obavezno je prijavljivanje za uređivanje',
'whitelistedittext'         => 'Morate da se [[{{ns:special}}:Userlogin|prijavite]] da biste menjali članke.',
'confirmedittitle'          => 'Potrebna je potvrda adrese e-pošte za uređivanje',
'confirmedittext'           => 'Morate potvrditi vašu adresu e-pošte pre uređivanja strana.
Molimo postavite i potvrdite adresu vaše e-pošte preko vaših [[Special:Preferences|korisničkih podešavanja]].',
'loginreqtitle'             => 'Potrebno [[{{ns:special}}:Userlogin|prijavljivanje]]',
'loginreqlink'              => 'prijava',
'loginreqpagetext'          => 'Morate $1 da biste videli ostale strane.',
'accmailtitle'              => 'Lozinka je poslata.',
'accmailtext'               => 'Lozinka za nalog "$1" je poslata na adresu $2.',
'newarticle'                => '(Novi)',
'newarticletext'            => "Pratili ste vezu ka stranici koja još ne postoji.
Da biste je napravili, počnite da kucate u polju ispod
(pogledajte [[{{ns:help}}:Sadržaj|pomoć]] za više informacija).
Ako ste došli ovde greškom, samo kliknite dugme '''back''' dugme vašeg brauzera.",
'anontalkpagetext'          => "----''Ovo je stranica za razgovor za anonimnog korisnika koji još nije napravio nalog ili ga ne koristi.
Zbog toga moramo da koristimo brojčanu IP adresu kako bismo identifikovali njega ili nju.
Takvu adresu može deliti više korisnika.
Ako ste anonimni korisnik i mislite da su vam upućene nebitne primedbe, molimo vas da [[Special:UserLogin|napravite nalog ili se prijavite]] da biste izbegli buduću zabunu sa ostalim anonimnim korisnicima.''",
'noarticletext'             => 'Trenutno nema teksta na ovoj stranici. Možete [[Special:Search/{{PAGENAME}}|pretražiti ovaj naziv]] u ostalim stranicama ili [{{fullurl:{{FULLPAGENAME}}|action=edit}} urediti ovu stranicu].',
'clearyourcache'            => "'''Zapamtite:''' Nakon snimanja, možda morate očistiti keš vašeg brauzera da biste videli promene. '''Mozilla / Firefox / Safari:''' držite ''Shift'' dok klikćete ''Reload'' ili pritisnite  ''Shift+Ctrl+R'' (''Cmd-Shift-R'' na ''Apple Mac'' mašini); '''IE:''' držite ''Ctrl'' dok klikćete ''Refresh'' ili pritisnite ''Ctrl-F5''; '''Konqueror:''': samo kliknite ''Reload'' dugme ili pritisnite ''F5''; korisnici '''Opera''' brauzera možda moraju da u potpunosti očiste svoj keš preko ''Tools→Preferences''.",
'usercssjsyoucanpreview'    => "<strong>Savet:</strong> Korisitite 'Prikaži pretpregled' dugme da testirate svoj novi CSS/JS pre snimanja.",
'usercsspreview'            => "'''Zapamtite ovo je samo pretpregled vašeg CSS i da još uvek nije snimljen!'''",
'userjspreview'             => "'''Zapamtite ovo je samo pretpregled vaše JavaScript-e i da još uvek nije snimljen!'''",
'userinvalidcssjstitle'     => "'''Pažnja:''' Ne postoji koža \"\$1\". Zapamtite da lične .css i .js koriste mala početna slova, npr. {{ns:user}}:Petar/monobook.css a ne {{ns:user}}:Petar/Monobook.css.",
'updated'                   => '(Ažurirano)',
'note'                      => '<strong>Napomena:</strong>',
'previewnote'               => '<strong>Ovo samo pretpregled; izmene još nisu sačuvane!</strong>',
'previewconflict'           => 'Ovaj pretpregled oslikava kako će tekst u
tekstualnom polju izgledati ako se odlučite da ga snimite.',
'session_fail_preview'      => '<strong>Žao nam je! Nismo mogli da obradimo vašu izmenu zbog gubitka podataka seanse. Molimo pokušajte kasnije. Ako i dalje ne radi, pokušajte da se odjavite i ponovo prijavite.</strong>',
'session_fail_preview_html' => "<strong>Žao nam je! Nismo mogli da obradimo vašu izmenu zbog gubitka podataka seanse.</strong>

''Zbog toga što ova viki ima omogućen sirov HTML, pretpregled je sakriven kao predostrožnost protiv JavaScript napada.''

<strong>Ako ste pokušali da napravite pravu izmenu, molimo pokušajte ponovo. Ako i dalje ne radi, pokušajte da se odjavite i ponovo prijavite.</strong>",
'editing'                   => 'Uređujete $1',
'editingsection'            => 'Uređujete $1 (deo)',
'editingcomment'            => 'Uređujete $1 (komentar)',
'editconflict'              => 'Sukobljene izmene: $1',
'explainconflict'           => 'Neko drugi je promenio ovu stranicu otkad ste vi počeli da je menjate.
Gornje tekstualno polje sadrži tekst stranice kakav trenutno postoji.
Vaše izmene su prikazane u donjem tekstu.
Moraćete da unesete svoje promene u postojeći tekst.
<b>Samo</b> tekst u gornjem tekstualnom polju će biti snimljen kada
pritisnete "Snimi stranicu".<br />',
'yourtext'                  => 'Vaš tekst',
'storedversion'             => 'Uskladištena verzija',
'nonunicodebrowser'         => '<strong>UPOZORENJE: Vaš brauzer ne podržava unikod. Molimo promenite ga pre nego što počnete sa uređivanjem članka.</strong>',
'editingold'                => '<strong>PAŽNJA: Vi menjate stariju reviziju ove stranice.
Ako je snimite, sve promene učinjene od ove revizije biće izgubljene.</strong>',
'yourdiff'                  => 'Razlike',
'copyrightwarning'          => 'Napomena: Za sve vaše doprinose se smatra da su izdati pod $2 (vidite $1 za detalje). Ako ne želite da se vaši doprinosi nemilosrdno menjaju, ne šaljite ih ovde.<br />
Takođe nam obećavate da ste ovo sami napisali ili prekopirali iz izvora u javnom vlasništvu ili sličnog slobodnog izvora.
<strong>NE ŠALJITE RADOVE ZAŠTIĆENE AUTORSKIM PRAVIMA BEZ DOZVOLE!</strong>',
'copyrightwarning2'         => 'Napomena: Sve vaše doprinose ostali korisnici mogu da menjaju ili uklone. Ako ne želite da se vaši doprinosi nemilosrdno menjaju, ne šaljite ih ovde.<br />
Takođe nam obećavate da ste ovo sami napisali ili prekopirali iz izvora u javnom vlasništvu ili sličnog slobodnog izvora (vidite $1 za detalje).
<strong>NE ŠALJITE RADOVE ZAŠTIĆENE AUTORSKIM PRAVIMA BEZ DOZVOLE!</strong>',
'longpagewarning'           => '<strong>PAŽNJA: Ova stranica ima $1 kilobajta; neki brauzeri imaju problema sa uređivanjem strana koje imaju blizu ili više od 32 kilobajta. Molimo vas da razmotrite razbijanje stranice na manje delove.</strong>',
'longpageerror'             => '<strong>GREŠKA: Tekst koji snimate je velik $1 kilobajta, što je veće od maksimalno dozvoljene veličine koja iznosi $2 kilobajta. Nemoguće je snimiti stranicu.</strong>',
'readonlywarning'           => '<strong>PAŽNJA: Baza je upravo zaključana zbog održavanja,
tako da sada nećete moći da snimite svoje izmene. Možda bi bilo dobro da iskopirate tekst u neki editor teksta i snimite ga za kasnije.</strong>',
'protectedpagewarning'      => '<strong>PAŽNJA: Ova stranica je zaključana tako da samo korisnici sa
administratorskim privilegijama mogu da je menjaju. Uverite se
da pratite [[{{ns:project}}:Pravila o zaštiti stranica|pravila o zaštiti stranica]].</strong>',
'semiprotectedpagewarning'  => "'''Napomena:''' Ova stranica je zaključana tako da je samo registrovani korisnici mogu uređivati.",
'templatesused'             => 'Šabloni koji se koriste na ovoj stranici:',
'edittools'                 => '<!-- Tekst odavde će biti pokazan ispod formulara za uređivanje i slanje slika. -->',
'nocreatetitle'             => 'Pravljenje stranice ograničeno',
'nocreatetext'              => 'Na ovom sajtu je ograničeno pravljenje novih stranica.
Možete se vratiti i urediti već postojeću stranu ili [[Special:UserLogin|se prijaviti ili napraviti nalog]].',

# Account creation failure
'cantcreateaccounttitle' => 'Ne može da se napravi nalog',

# History pages
'viewpagelogs'        => 'Pogledaj protokole za ovu stranu',
'nohistory'           => 'Ne postoji istorija izmena za ovu stranicu.',
'revnotfound'         => 'Revizija nije pronađena',
'revnotfoundtext'     => 'Starija revizija ove stranice koju ste zatražili nije nađena.
Molimo vas da proverite URL koji ste upotrebili da biste pristupili ovoj stranici.',
'currentrev'          => 'Trenutna revizija',
'revisionasof'        => 'Revizija od $1',
'revision-info'       => 'Revizija od $1; $2',
'previousrevision'    => '← Prethodna revizija',
'nextrevision'        => 'Sledeća revizija →',
'currentrevisionlink' => 'Trenutna revizija',
'cur'                 => 'tren',
'next'                => 'sled',
'last'                => 'posl',
'histlegend'          => 'Odabiranje razlika: odaberite kutijice revizija za upoređivanje i pritisnite enter ili dugme na dnu.<br />
Objašnjenje: (tren) = razlika sa trenutnom verzijom,
(posl) = razlika sa prethodnom verzijom, M = mala izmena',
'deletedrev'          => '[obrisan]',
'histfirst'           => 'Najranije',
'histlast'            => 'Poslednje',

# Revision feed
'history-feed-title'          => 'Istorija revizija',
'history-feed-description'    => 'Istorija revizija za ovu stranu na vikiju',
'history-feed-item-nocomment' => '$1, $2', # user at time
'history-feed-empty'          => 'Tražena stranica ne postoji.
Moguće da je obrisana iz vikija ili preimenovana.
Pokušajte [[Special:Search|da pretražite viki]] za relevantne nove strane.',

# Revision deletion
'rev-deleted-comment'         => '(komentar uklonjen)',
'rev-deleted-user'            => '(korisničko ime uklonjeno)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Revizija ove stranice je uklonjena iz javnih arhiva.
Moguće da ima više detalja u [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} istoriji brisanja].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Revizija ove stranice je uklonjena iz javnih arhiva.
Kao administrator, možete da je pogledate;
Moguće da ima više detalja u [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} istoriji brisanja].
</div>',
'rev-delundel'                => 'pokaži/sakrij',
'revisiondelete'              => 'Obriši/vrati reviziju',
'revdelete-nooldid-title'     => 'Nema odabrane revizije',
'revdelete-nooldid-text'      => 'Niste odabrali željenu reviziju ili revizije kako biste uključili ove funkcije.',
'revdelete-selected'          => 'Izabrano revizija od [[:$1]]:',
'revdelete-text'              => 'Obrisane revizije će se i dalje pojavljivati na istoriji stranice,
ali će njihov sadržaj biti skriven javnosti.

Ostali administratori na ovoj Vikipediji će i dalje imati mogućnost da vide skriveni sadržaj i moći će da ga vrate ponovo putem ove iste komande, sve ukoliko nisu primenjene dodatne restrikcije operatora sajta.',
'revdelete-legend'            => 'Postavi restrikcije revizija',
'revdelete-hide-text'         => 'Sakrij tekst revizije',
'revdelete-hide-comment'      => 'Sakrij opis izmene',
'revdelete-hide-user'         => 'Sakrij korisničko ime/IP adresu korisnika koji je uređivao stranicu',
'revdelete-hide-restricted'   => 'Primeni ove restrikcije za administratore isto kao i za ostale',
'revdelete-log'               => 'Komentar protokola:',
'revdelete-submit'            => 'Primeni na izabrane revizije',
'revdelete-logentry'          => 'promenjen prikaz revizije za [[$1]]',

# Diffs
'difference'              => '(Razlika između revizija)',
'lineno'                  => 'Linija $1:',
'compareselectedversions' => 'Uporedi označene verzije',

# Search results
'searchresults'         => 'Rezultati pretrage',
'searchresulttext'      => 'Za više informacija o pretraživanju sajta {{SITENAME}}, pogledajte [[{{ns:project}}:Pretraživanje|Pretraživanje sajta {{SITENAME}}]].',
'searchsubtitle'        => "Tražili ste '''[[:$1]]'''",
'searchsubtitleinvalid' => "Tražili ste '''$1'''",
'noexactmatch'          => 'Ne postoji stranica sa naslovom "$1". Možete [[$1|napraviti ovu stranicu]].',
'titlematches'          => 'Naslov stranice odgovara',
'notitlematches'        => 'Nijedan naslov stranice ne odgovara',
'textmatches'           => 'Tekst stranice odgovara',
'notextmatches'         => 'Nijedan tekst stranice ne odgovara',
'prevn'                 => 'prethodnih $1',
'nextn'                 => 'sledećih $1',
'viewprevnext'          => 'Pogledaj ($1) ($2) ($3).',
'showingresults'        => "Prikazujem ispod '''$1''' rezultata počev od #'''$2'''.",
'showingresultsnum'     => "Prikazujem ispod '''$3''' rezultate počev od #'''$2'''.",
'nonefound'             => "'''Napomena''': neuspešne pretrage su
često izazvane traženjem čestih reči kao \"je\" ili \"od\",
koje nisu indeksirane, ili navođenjem više od jednog izraza za traženje (samo stranice
koje sadrže sve izraze koji se traže će se pojaviti u rezultatu).",
'powersearch'           => 'Traži',
'searchdisabled'        => 'Pretraga za sajt {{SITENAME}} je onemogućena. U međuvremenu, možete koristiti Gugl pretragu. Imajte na umu da indeksi Gugla za sajt {{SITENAME}} mogu biti zastareli.',

# Preferences page
'preferences'              => 'Podešavanja',
'mypreferences'            => 'Moja podešavanja',
'prefsnologin'             => 'Niste prijavljeni',
'prefsnologintext'         => 'Morate biti [[Special:UserLogin|prijavljeni]] da biste podešavali korisnička podešavanja.',
'prefsreset'               => 'Vraćena su uskladištena podešavanja.',
'qbsettings'               => 'Brza paleta',
'qbsettings-none'          => 'Nikakva',
'qbsettings-fixedleft'     => 'Pričvršćena levo',
'qbsettings-fixedright'    => 'Pričvršćena desno',
'qbsettings-floatingleft'  => 'Plutajuća levo',
'qbsettings-floatingright' => 'Plutajuća desno',
'changepassword'           => 'Promeni lozinku',
'skin'                     => 'Koža',
'math'                     => 'Matematike',
'dateformat'               => 'Format datuma',
'datedefault'              => 'Nije bitno',
'datetime'                 => 'Datum i vreme',
'math_failure'             => 'Neuspeh pri parsiranju',
'math_unknown_error'       => 'nepoznata greška',
'math_unknown_function'    => 'nepoznata funkcija',
'math_lexing_error'        => 'rečnička greška',
'math_syntax_error'        => 'sintaksna greška',
'math_image_error'         => 'PNG konverzija neuspešna; proverite tačnu instalaciju latex-a, dvips-a, gs-a i convert-a',
'math_bad_tmpdir'          => 'Ne mogu da napišem ili napravim privremeni math direktorijum',
'math_bad_output'          => 'Ne mogu da napišem ili napravim direktorijum za math izlaz.',
'math_notexvc'             => 'Nedostaje izvršno texvc; molimo pogledajte math/README da biste podesili.',
'prefs-personal'           => 'Korisnička podešavanja',
'prefs-rc'                 => 'Skorašnje izmene',
'prefs-watchlist'          => 'Spisak nadgledanja',
'prefs-watchlist-days'     => 'Broj dana koji treba da se vidi na spisku nadgledanja:',
'prefs-watchlist-edits'    => 'Broj izmena koji treba da se vidi na proširenom spisku nadgledanja:',
'prefs-misc'               => 'Razno',
'saveprefs'                => 'Sačuvaj',
'resetprefs'               => 'Vrati',
'oldpassword'              => 'Stara lozinka:',
'newpassword'              => 'Nova lozinka:',
'retypenew'                => 'Ponovo otkucajte novu lozinku:',
'textboxsize'              => 'Veličine tekstualnog polja',
'rows'                     => 'Redova',
'columns'                  => 'Kolona',
'searchresultshead'        => 'Pretraga',
'resultsperpage'           => 'Pogodaka po stranici:',
'contextlines'             => 'Linija po pogotku:',
'contextchars'             => 'Karaktera konteksta po liniji:',
'recentchangescount'       => 'Broj naslova u skorašnjim izmenama:',
'savedprefs'               => 'Vaša podešavanja su sačuvana.',
'timezonelegend'           => 'Vremenska zona',
'timezonetext'             => 'Broj sati za koji se vaše lokalno vreme razlikuje od serverskog vremena (UTC).',
'localtime'                => 'Lokalno vreme',
'timezoneoffset'           => 'Odstupanje¹',
'servertime'               => 'Vreme na serveru',
'guesstimezone'            => 'Popuni iz brauzera',
'allowemail'               => 'Omogući e-poštu od drugih korisnika',
'defaultns'                => 'Po standardu traži u ovim imenskim prostorima:',
'default'                  => 'standard',
'files'                    => 'Fajlovi',

# User rights
'userrights'               => 'Upravljanje korisničkim pravima', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Upravljaj korisničkim grupama',
'userrights-user-editname' => 'Unesite korisničko ime:',
'editusergroup'            => 'Menjaj grupe korisnika',
'editinguser'              => "Uređujete '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Promeni korisničke grupe',
'saveusergroups'           => 'Sačuvaj korisničke grupe',
'userrights-groupsmember'  => 'Član:',

# Groups
'group'            => 'Grupa:',
'group-bot'        => 'botovi',
'group-sysop'      => 'administratori',
'group-bureaucrat' => 'birokrate',
'group-all'        => '(svi)',

'group-bot-member'        => 'bot',
'group-sysop-member'      => 'administrator',
'group-bureaucrat-member' => 'birokrata',

'grouppage-bot'        => '{{ns:project}}:Botovi',
'grouppage-sysop'      => '{{ns:project}}:Spisak administratora',
'grouppage-bureaucrat' => '{{ns:project}}:Birokrate',

# User rights log
'rightslog'      => 'istorija korisničkih prava',
'rightslogtext'  => 'Ovo je istorija izmena korisničkih prava.',
'rightslogentry' => 'je promenio prava za $1 sa $2 na $3',
'rightsnone'     => '(nema)',

# Recent changes
'recentchanges'                     => 'Skorašnje izmene',
'recentchangestext'                 => 'Ovde pratite najskorije izmene na vikiju.',
'rcnote'                            => 'Ispod je poslednjih <strong>$1</strong> promena u poslednjih <strong>$2</strong> dana.',
'rcnotefrom'                        => 'Ispod su promene od <b>$2</b> (do <b>$1</b> prikazano).',
'rclistfrom'                        => 'Pokaži nove promene počev od $1',
'rcshowhideminor'                   => '$1 male izmene',
'rcshowhidebots'                    => '$1 botove',
'rcshowhideliu'                     => '$1 prijavljene korisnike',
'rcshowhideanons'                   => '$1 anonimne korisnike',
'rcshowhidepatr'                    => '$1 patrolirane izmene',
'rcshowhidemine'                    => '$1 sopstvene izmene',
'rclinks'                           => 'Pokaži poslednjih $1 promena u poslednjih $2 dana<br />$3',
'diff'                              => 'razl',
'hist'                              => 'ist',
'hide'                              => 'sakrij',
'show'                              => 'pokaži',
'number_of_watching_users_pageview' => '[$1 korisnik/a koji nadgleda/ju]',
'rc_categories'                     => 'Ograniči na kategorije (razdvoji sa "|")',
'rc_categories_any'                 => 'Bilo koji',

# Recent changes linked
'recentchangeslinked' => 'Srodne promene',

# Upload
'upload'                      => 'Pošalji fajl',
'uploadbtn'                   => 'Pošalji fajl',
'reupload'                    => 'Ponovo pošalji',
'reuploaddesc'                => 'Vrati se na upitnik za slanje.',
'uploadnologin'               => 'Niste prijavljeni',
'uploadnologintext'           => 'Morate biti [[Special:UserLogin|prijavljeni]] da biste slali fajlove.',
'upload_directory_read_only'  => 'Na direktorijum za slanje ($1) server ne može da piše.',
'uploaderror'                 => 'Greška pri slanju',
'uploadtext'                  => "Koristite donji obrazac da pošaljete fajlove.
Za gledanje ili pretraživanje već poslatih slika, idite na [[Special:ImageList|spisak poslatih fajlova]].
Slanja i brisanja se beleže u [[Special:Log/upload|istoriji slanja]]

Da biste ubacili sliku na stranu, koristite vezu u obliku
'''<nowiki>[[</nowiki>{{ns:image}}:Fajl.jpg<nowiki>]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}:Fajl.png|opis slike<nowiki>]]</nowiki>''' ili
'''<nowiki>[[</nowiki>{{ns:media}}:Fajl.ogg<nowiki>]]</nowiki>''' za direktno povezivanje na fajl.",
'uploadlog'                   => 'istorija slanja',
'uploadlogpage'               => 'istorija slanja',
'uploadlogpagetext'           => 'Ispod je spisak najskorijih slanja.',
'filename'                    => 'Ime fajla',
'filedesc'                    => 'Opis',
'fileuploadsummary'           => 'Opis:',
'filestatus'                  => 'Status autorskog prava:',
'filesource'                  => 'Izvor:',
'uploadedfiles'               => 'Poslati fajlovi',
'ignorewarning'               => 'Ignoriši upozorenja i snimi datoteku.',
'ignorewarnings'              => 'Ignoriši sva upozorenja',
'illegalfilename'             => 'Fajl "$1" sadrži karaktere koji nisu dozvoljeni u nazivima stranica. Molimo Vas promenite ime fajla i ponovo ga pošaljite.',
'badfilename'                 => 'Ime slike je promenjeno u "$1".',
'largefileserver'             => 'Ovaj fajl je veći nego što je podešeno da server dozvoli.',
'emptyfile'                   => 'Fajl koji ste poslali deluje da je prazan. Ovo je moguće zbog greške u imenu fajla. Molimo proverite da li stvarno želite da pošaljete ovaj fajl.',
'fileexists'                  => 'Fajl sa ovim imenom već postoji. Molimo proverite <strong><tt>$1</tt></strong> ako niste sigurni da li želite da ga promenite.',
'fileexists-forbidden'        => 'Fajl sa ovim imenom već postoji;
molimo vratite se i pošaljite ovaj fajl pod novim imenom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Fajl sa ovim imenom već postoji u zajedničkoj ostavi;
molimo vratite se i pošaljite ovaj fajl pod novim imenom. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Uspešno slanje',
'uploadwarning'               => 'Upozorenje pri slanju',
'savefile'                    => 'Snimi fajl',
'uploadedimage'               => 'poslao "[[$1]]"',
'uploaddisabled'              => 'Slanje fajlova je isključeno.',
'uploaddisabledtext'          => 'Slanja fajlova su onemogućena na ovom vikiju.',
'uploadscripted'              => 'Ovaj fajl sadrži HTML ili kod skripte koje internet brauzer može pogrešno da interpretira.',
'uploadcorrupt'               => 'Fajl je neispravan ili ima netačnu ekstenziju. Molimo proverite fajl i pošaljite ga ponovo.',
'uploadvirus'                 => 'Fajl sadrži virus! Detalji: $1',
'sourcefilename'              => 'Ime fajla izvora:',
'destfilename'                => 'Ciljano ime fajla:',
'watchthisupload'             => 'Nadgledaj stranicu',
'filewasdeleted'              => 'Fajl sa ovim imenom je ranije poslat, a kasnije obrisan. Trebalo bi da proverite $1 pre nego što nastavite sa ponovnim slanjem.',

'upload-proto-error'      => 'Nekorektni protokol',
'upload-proto-error-text' => 'Slanje eksternih fajlova zahteva URLove koji počinju sa <code>http://</code> ili <code>ftp://</code>.',
'upload-file-error'       => 'Interna greška',
'upload-file-error-text'  => 'Desila se interna greška pri pokušaju pravljenja privremenog fajla na serveru. Kontaktirajte sistem administratora.',
'upload-misc-error'       => 'Nepoznata greška pri slanju fajla',
'upload-misc-error-text'  => 'Nepoznata greška pri slanju fajla. Proverite da li je URL ispravan i pokušajte ponovo. Ako problem ostane, kontaktirajte sistem administratora.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL nije dostupan',
'upload-curl-error6-text'  => 'URL koji ste uneli nije dostupan. Uradite dupli klik na URL da proverite da li je adresa dostupna.',
'upload-curl-error28'      => 'Tajmaut greška',
'upload-curl-error28-text' => 'Sajtu je trebalo previše vremena da odgovori. Proverite da li sajt radi, ili sačekajte malo i pokušajte ponovo.',

'license'            => 'Licenca:',
'nolicense'          => 'Nema',
'upload_source_url'  => ' (validan, javno dostupan URL)',
'upload_source_file' => ' (fajl na vašem računaru)',

# Special:ImageList
'imagelist_search_for'  => 'Traži ime slike:',
'imgfile'               => 'fajl',
'imagelist'             => 'Spisak slika',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Ime',
'imagelist_user'        => 'Korisnik',
'imagelist_size'        => 'Veličina (bajtovi)',
'imagelist_description' => 'Opis slike',

# Image description page
'imagelinks'                => 'Upotreba slike',
'linkstoimage'              => 'Sledeće stranice koriste ovaj fajl:',
'nolinkstoimage'            => 'Nema stranica koje koriste ovaj fajl.',
'sharedupload'              => 'Ova slika je sa zajedničke ostave i možda je koriste ostali projekti.',
'shareduploadwiki'          => 'Molimo pogledajte $1 za dalje informacije.',
'shareduploadwiki-linktext' => 'strana za opis fajla',
'noimage'                   => 'Ne postoji fajl sa ovim imenom, možete ga $1',
'noimage-linktext'          => 'poslati',
'uploadnewversion-linktext' => 'Pošaljite noviju verziju ovog fajla',

# MIME search
'mimesearch' => 'MIME pretraga',
'mimetype'   => 'MIME tip:',
'download'   => 'Preuzmi',

# Unwatched pages
'unwatchedpages' => 'Nenadgledane stranice',

# List redirects
'listredirects' => 'Spisak preusmerenja',

# Unused templates
'unusedtemplates'     => 'Neiskorišćeni šabloni',
'unusedtemplatestext' => 'Ova strana navodi sve stranice u imenskom prostoru šablona koje nisu uključene ni na jednoj drugoj strani. Ne zaboravite da proverite ostale veze ka šablonima pre nego što ih obrišete.',
'unusedtemplateswlh'  => 'ostale veze',

# Random page
'randompage' => 'Slučajna stranica',

# Random redirect
'randomredirect' => 'Slučajno preusmerenje',

# Statistics
'statistics'             => 'Statistike',
'sitestats'              => 'Statistike sajta',
'userstats'              => 'Statistike korisnika',
'sitestatstext'          => "Postoji ukupno '''$1''' stranica u bazi podataka. Ovaj broj uključuje strane za razgovor, stranice o sajtu, preusmerenja, članke bez ijedne poveznice i ostale stranice koje se ne mogu računati kao članci. Ne računajući njih, postoji '''$2''' stranica koje su verovatno legitimni članci.

Na ovaj sajt je poslato '''$8''' slika.

Stranice su ukupno pogledane '''$3''' puta i '''$4''' izmena od postavljanja vikija. Ovo znači da je bilo u proseku '''$5''' izmena po stranici i '''$6''' pogleda po stranici.

Dužina reda za poslove iznosi '''$7'''",
'userstatstext'          => "Postoji '''$1''' registrovanih korisnika, od kojih su '''$2''' (ili $4%) administratori (pogledajte $3).",
'statistics-mostpopular' => 'Najposećenije stranice',

'disambiguations'     => 'Stranice za višeznačne odrednice',
'disambiguationspage' => '{{ns:template}}:Višeznačna odrednica',

'doubleredirects'     => 'Dvostruka preusmerenja',
'doubleredirectstext' => 'Svaki red sadrži veze na prvo i drugo preusmerenje, kao i na prvu liniju teksta drugog preusmerenja, što obično daje "pravi" ciljni članak, na koji bi prvo preusmerenje i trebalo da pokazuje.',

'brokenredirects'     => 'Pokvarena preusmerenja',
'brokenredirectstext' => 'Sledeća preusmerenja su povezana na nepostojeći članak.',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajta|bajtova}}',
'ncategories'             => '$1 {{PLURAL:$1|kategorija|kategorije|kategorija}}',
'nlinks'                  => '$1 {{PLURAL:$1|veza|veze|veza}}',
'nmembers'                => '$1 {{PLURAL:$1|članak|članka|članaka}}',
'nrevisions'              => '$1 {{PLURAL:$1|revizija|revizije|revizija}}',
'nviews'                  => '$1 puta pogledano',
'lonelypages'             => 'Siročići',
'lonelypagestext'         => 'Sledeće stranice nisu povezane sa drugih stranica na ovom vikiju.',
'uncategorizedpages'      => 'Stranice bez kategorije',
'uncategorizedcategories' => 'Kategorije bez kategorija',
'uncategorizedimages'     => 'Slike bez kategorija',
'unusedcategories'        => 'Neiskorišćene kategorije',
'unusedimages'            => 'Neiskorišćeni fajlovi',
'popularpages'            => 'Popularne stranice',
'wantedcategories'        => 'Tražene kategorije',
'wantedpages'             => 'Tražene stranice',
'mostlinked'              => 'Najviše povezane strane',
'mostlinkedcategories'    => 'Najviše povezane kategorije',
'mostcategories'          => 'Članci sa najviše kategorija',
'mostimages'              => 'Najviše povezane slike',
'mostrevisions'           => 'Članci sa najviše revizija',
'prefixindex'             => 'Spisak prefiksa',
'shortpages'              => 'Kratke stranice',
'longpages'               => 'Dugačke stranice',
'deadendpages'            => 'Stranice bez internih veza',
'deadendpagestext'        => 'Sledeće stranice ne vežu na druge stranice na ovom vikiju.',
'listusers'               => 'Spisak korisnika',
'newpages'                => 'Nove stranice',
'newpages-username'       => 'Korisničko ime:',
'ancientpages'            => 'Najstariji članci',
'move'                    => 'premesti',
'movethispage'            => 'premesti ovu stranicu',
'unusedimagestext'        => '<p>Obratite pažnju da se drugi veb sajtovi
mogu povezivati na sliku direktnim URL-om, i tako mogu još uvek biti prikazani ovde uprkos
aktivnoj upotrebi.',
'unusedcategoriestext'    => 'Naredne strane kategorija postoje iako ih ni jedan drugi članak ili kategorija ne koriste.',
'notargettitle'           => 'Nema cilja',
'notargettext'            => 'Niste naveli ciljnu stranicu ili korisnika
na kome bi se izvela ova funkcija.',

# Book sources
'booksources' => 'Štampani izvori',

# Special:Log
'specialloguserlabel'  => 'Korisnik:',
'speciallogtitlelabel' => 'Naslov:',
'log'                  => 'Protokoli',
'alllogstext'          => 'Kombinovani prikaz istorija slanja, brisanja, zaštite, blokiranja i administratorskih prava.
Možete suziti pregled odabirom tipa istorije, korisničkog imena ili tražene stranice.',
'logempty'             => 'Protokol je prazan.',

# Special:AllPages
'allpages'          => 'Sve stranice',
'alphaindexline'    => '$1 u $2',
'nextpage'          => 'Sledeća stranica ($1)',
'allpagesfrom'      => 'Prikaži stranice početno sa:',
'allarticles'       => 'Svi članci',
'allinnamespace'    => 'Sve stranice ($1 imenski prostor)',
'allnotinnamespace' => 'Sve stranice (koje nisu u $1 imenskom prostoru)',
'allpagesprev'      => 'Prethodna',
'allpagesnext'      => 'Sledeća',
'allpagessubmit'    => 'Idi',
'allpagesprefix'    => 'Prikaži strane sa prefiksom:',
'allpagesbadtitle'  => 'Dati naziv stranice nije dobar ili sadrži međujezički ili interviki prefiks. Moguće je da sadrži karaktere koji ne mogu da se koriste u nazivima.',

# Special:Categories
'categories'         => 'Kategorije stranica',
'categoriespagetext' => 'Sledeće kategorije već postoje na vikiju',

# Special:ListUsers
'listusersfrom' => 'Prikaži korisnike počevši od:',

# E-mail user
'mailnologin'     => 'Nema adrese za slanje',
'mailnologintext' => 'Morate biti [[Special:UserLogin|prijavljeni]] i imati ispravnu adresu e-pošte u vašim [[Special:Preferences|podešavanjima]]
da biste slali e-poštu drugim korisnicima.',
'emailuser'       => 'Pošalji e-poštu ovom korisniku',
'emailpage'       => 'Pošalji e-pismo korisniku',
'emailpagetext'   => 'Ako je ovaj korisnik uneo ispravnu adresu e-pošte u
svoja korisnička podešavanja, upitnik ispod će poslati jednu poruku.
Adresa e-pošte koju ste vi uneli u svojim korisničkim podešavanjima će se pojaviti
kao "From" adresa poruke, tako da će primalac moći da odgovori.',
'usermailererror' => 'Objekat pošte je vratio grešku:',
'defemailsubject' => '{{SITENAME}} e-pošta',
'noemailtitle'    => 'Nema adrese e-pošte',
'noemailtext'     => 'Ovaj korisnik nije naveo ispravnu adresu e-pošte,
ili je izabrao da ne prima e-poštu od drugih korisnika.',
'emailfrom'       => 'Od',
'emailto'         => 'Za',
'emailsubject'    => 'Tema',
'emailmessage'    => 'Poruka',
'emailsend'       => 'Pošalji',
'emailccme'       => 'Pošalji mi kopiju moje poruke u moje sanduče e-pošte.',
'emailccsubject'  => 'Kopija vaše poruke na $1: $2',
'emailsent'       => 'Poruka poslata',
'emailsenttext'   => 'Vaša poruka je poslata elektronskom poštom.',

# Watchlist
'watchlist'            => 'Moj spisak nadgledanja',
'mywatchlist'          => 'Moj spisak nadgledanja',
'watchlistfor'         => "(za '''$1''')",
'nowatchlist'          => 'Nemate ništa na svom spisku nadgledanja.',
'watchlistanontext'    => 'Molimo $1 da biste gledali ili menjali stavke na vašem spisku nadgledanja.',
'watchnologin'         => 'Niste prijavljeni',
'watchnologintext'     => 'Morate biti [[Special:UserLogin|prijavljeni]] da biste menjali spisak nadgledanja.',
'addedwatch'           => 'Dodato spisku nadgledanja',
'addedwatchtext'       => "Stranica \"[[:\$1]]\" je dodata vašem [[{{ns:special}}:Watchlist|spisku nadgledanja]] .
Buduće promene na ovoj stranici i njoj pridruženoj stranici za razgovor biće navedene ovde, i stranica će biti '''podebljana''' u [[{{ns:special}}:Recentchanges|spisku skorašnjih izmena]] da bi se lakše uočila.

Ako kasnije želite da uklonite stranicu sa vašeg spiska nadgledanja, kliknite na \"ne nadgledaj\" na bočnoj paleti.",
'removedwatch'         => 'Uklonjeno sa spiska nadgledanja',
'removedwatchtext'     => 'Stranica "[[:$1]]" je uklonjena sa vašeg spiska nadgledanja.',
'watch'                => 'nadgledaj',
'watchthispage'        => 'Nadgledaj ovu stranicu',
'unwatch'              => 'Prekini nadgledanje',
'unwatchthispage'      => 'Prekini nadgledanje',
'notanarticle'         => 'Nije članak',
'watchnochange'        => 'Ništa što nadgledate nije promenjeno u prikazanom vremenu.',
'watchlist-details'    => '$1 stranica nadgledano ne računajući stranice za razgovor.',
'wlheader-enotif'      => '* Obaveštavanje e-poštom je omogućeno.',
'wlheader-showupdated' => "* Stranice koje su izmenjene od kada ste ih poslednji put posetili su prikazane '''podebljano'''",
'watchmethod-recent'   => 'proveravam ima li nadgledanih stranica u skorašnjim izmenama',
'watchmethod-list'     => 'proveravam ima li skorašnjih izmena u nadgledanim stranicama',
'watchlistcontains'    => 'Vaš spisak nadgledanja sadrži $1 stranica.',
'iteminvalidname'      => "Problem sa stavkom '$1', neispravno ime...",
'wlnote'               => 'Ispod je poslednjih $1 izmena u poslednjih <b>$2</b> sati.',
'wlshowlast'           => 'Prikaži poslednjih $1 sati $2 dana $3',

'enotif_mailer'      => '{{SITENAME}} pošta obaveštenja',
'enotif_reset'       => 'Označi sve strane kao posećene',
'enotif_newpagetext' => 'Ovo je novi članak.',
'changed'            => 'promenjena',
'created'            => 'napravljena',
'enotif_subject'     => '{{SITENAME}} stranica $PAGETITLE je bila $CHANGEDORCREATED od strane $PAGEEDITOR',
'enotif_lastvisited' => 'Pogledajte $1 za sve promene od vaše poslednje posete.',
'enotif_body'        => 'Dragi $WATCHINGUSERNAME,

{{SITENAME}} stranicaa $PAGETITLE je bila $CHANGEDORCREATED ($PAGEEDITDATE) od strane $PAGEEDITOR,
pogledajte $PAGETITLE_URL za trenutnu verziju.

$NEWPAGE

Opis izmene urednika: $PAGESUMMARY $PAGEMINOREDIT

Kontaktirajte urednika:
pošta: $PAGEEDITOR_EMAIL
viki: $PAGEEDITOR_WIKI

Neće biti drugih obaveštenja u slučaju daljih promena ukoliko ne posetite ovu stranu.
Takođe možete da resetujete zastavice za obaveštenja za sve vaše nadgledane strane na vašem spisku nadgledanja.

             Vaš prijateljski {{SITENAME}} sistem obaveštavanja

--
Da promenite podešavanja vezana za spisak nadgledanja posetite
{{fullurl:{{ns:special}}:Watchlist/edit}}

Fidbek i dalja pomoć:
{{fullurl:{{ns:help}}:Sadržaj}}',

# Delete/protect/revert
'deletepage'                  => 'Obriši stranicu',
'confirm'                     => 'Potvrdi',
'excontent'                   => "sadržaj je bio: '$1'",
'excontentauthor'             => "sadržaj je bio: '$1' (a jedinu izmenu je napravio '$2')",
'exbeforeblank'               => "sadržaj pre brisanja je bio: '$1'",
'exblank'                     => 'stranica je bila prazna',
'historywarning'              => 'Pažnja: stranica koju želite da obrišete ima istoriju:',
'confirmdeletetext'           => 'Na putu ste da trajno obrišete stranicu
ili sliku zajedno sa njenom istorijom iz baze podataka.
Molimo vas potvrdite da nameravate da uradite ovo, da razumete
posledice, i da ovo radite u skladu sa
[[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Akcija završena',
'deletedtext'                 => 'Članak "<nowiki>$1</nowiki>" je obrisan.
Pogledajte $2 za zapis o skorašnjim brisanjima.',
'deletedarticle'              => 'obrisan "[[$1]]"',
'dellogpage'                  => 'istorija brisanja',
'dellogpagetext'              => 'Ispod je spisak najskorijih brisanja.',
'deletionlog'                 => 'istorija brisanja',
'reverted'                    => 'Vraćeno na raniju reviziju',
'deletecomment'               => 'Razlog za brisanje',
'rollback'                    => 'Vrati izmene',
'rollback_short'              => 'Vrati',
'rollbacklink'                => 'vrati',
'rollbackfailed'              => 'Vraćanje nije uspelo',
'cantrollback'                => 'Ne mogu da vratim izmenu; poslednji autor je ujedno i jedini.',
'alreadyrolled'               => 'Ne mogu da vratim poslednju izmenu [[:$1]]
od korisnika [[User:$2|$2]] ([[User_talk:$2|razgovor]]); neko drugi je već izmenio ili vratio članak.

Poslednju izmenu je napravio korisnik [[User:$3|$3]] ([[User_talk:$3|razgovor]]).',
'editcomment'                 => 'Komentar izmene je: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Vraćene izmene od [[{{ns:special}}:Contributions/$2|$2]] ([[User_talk:$2|razgovor]]) na poslednju izmenu od korisnika [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'sessionfailure'              => 'Izgleda da postoji problem sa vašom seansom prijave;
ova akcija je prekinuta kao predostrožnost protiv preotimanja seansi.
Molimo kliknite "back" i ponovo učitajte stranu odakle ste došli, a onda pokušajte ponovo.',
'protectlogpage'              => 'istorija zaključavanja',
'protectlogtext'              => 'Ispod je spisak zaključavanja i otključavanja stranica.',
'protectedarticle'            => 'zaštitio $1',
'unprotectedarticle'          => 'skinuo zaštitu sa $1',
'protect-title'               => 'stavljanje zaštite "$1"',
'protect-legend'              => 'Potvrdite zaštitu',
'protectcomment'              => 'Razlog zaštite',
'protect-unchain'             => 'Otključaj dozvole premeštanja',
'protect-text'                => 'Ovde možete pogledati i menjati nivo zaštite za stranicu <strong><nowiki>$1</nowiki></strong>.',
'protect-default'             => '(standard)',
'protect-level-autoconfirmed' => 'Blokiraj neregistrovane korisnike',
'protect-level-sysop'         => 'Samo za administratore',

# Restrictions (nouns)
'restriction-edit' => 'Uređivanje',
'restriction-move' => 'Premeštanje',

# Undelete
'undelete'                 => 'Pogledaj obrisane stranice',
'undeletepage'             => 'Pogledaj i vrati obrisane stranice',
'viewdeletedpage'          => 'Pogledaj obrisane strane',
'undeletepagetext'         => 'Sledeće stranice su obrisane ali su još uvek u arhivi i
mogu biti vraćene. Arhiva može biti periodično čišćena.',
'undeleteextrahelp'        => "Da vratite celu stranu, ostavite sve kućice neotkačenim i kliknite na '''''Vrati'''''. Da izvršite selektivno vraćanje, otkačite kućice koje odgovaraju reviziji koja treba da se vrati i kliknite na '''''Vrati'''''. Klikom na '''''Poništi''''' ćete obrisati polje za komentar i sve kućice.",
'undeleterevisions'        => '$1 revizija arhivirano',
'undeletehistory'          => 'Ako vratite stranicu, sve revizije će biti vraćene njenoj istoriji.
Ako je nova stranica istog imena napravljena od brisanja, vraćene
revizije će se pojaviti u ranijoj istoriji, a trenutna revizija sadašnje stranice
neće biti automatski zamenjena.',
'undeletehistorynoadmin'   => 'Ova strana je obrisana. Razlog za brisanje se nalazi u opisu ispod, zajedno sa detaljima o korisniku koji je menjao ovu stranu pre brisanja. Stvarni tekst ovih obrisanih revizija je dostupan samo administratorima.',
'undeleterevision-missing' => 'Nekorektna ili nepostojeća revizija. Možda je vaš link pogrešan, ili je revizija restaurirana, ili obrisana iz arhive.',
'undeletebtn'              => 'Vrati!',
'undeletereset'            => 'Poništi',
'undeletecomment'          => 'Komentar:',
'undeletedarticle'         => 'vratio "[[$1]]"',
'undeletedrevisions'       => '$1 revizija vraćeno',
'undeletedrevisions-files' => '$1 {{PLURAL:$1|revizija|revizije|revizija}} i $2 {{PLURAL:$2|fajl|fajla|fajlova}} vraćeno',
'undeletedfiles'           => '$1 {{PLURAL:$1|fajl vraćen|fajla vraćena|fajlova vraćeno}}',
'cannotundelete'           => 'Vraćanje obrisane verzije nije uspelo; neko drugi je vratio stranicu pre vas.',
'undeletedpage'            => "<big>'''Strana $1 je vraćena'''</big>

Pogledajte [[{{ns:special}}:Log/delete|istoriju brisanja]] za spisak skorašnjih brisanja i vraćanja.",

# Namespace form on various pages
'namespace'      => 'Imenski prostor:',
'invert'         => 'Obrni selekciju',
'blanknamespace' => '(Glavno)',

# Contributions
'contributions' => 'Prilozi korisnika',
'mycontris'     => 'Moji prilozi',
'contribsub2'   => 'Za $1 ($2)',
'nocontribs'    => 'Nisu nađene promene koje zadovoljavaju ove uslove.',
'uctop'         => ' (vrh)',

'sp-contributions-newbies-sub' => 'Za novajlije',

# What links here
'whatlinkshere' => 'Šta je povezano ovde',
'linklistsub'   => '(spisak veza)',
'linkshere'     => 'Sledeće stranice su povezane ovde:',
'nolinkshere'   => 'Ni jedna stranica nije povezana ovde.',
'isredirect'    => 'preusmerivač',
'istemplate'    => 'uključivanje',

# Block/unblock
'blockip'                     => 'Blokiraj korisnika',
'blockiptext'                 => 'Upotrebite donji upitnik da biste uklonili pravo pisanja
sa određene IP adrese ili korisničkog imena.
Ovo bi trebalo da bude urađeno samo da bi se sprečio vandalizam, i u skladu
sa [[{{MediaWiki:Policy-url}}|politikom]].
Unesite konkretan razlog ispod (na primer, navodeći koje
stranice su vandalizovane).',
'ipaddress'                   => 'IP adresa',
'ipadressorusername'          => 'IP adresa ili korisničko ime',
'ipbexpiry'                   => 'Trajanje',
'ipbreason'                   => 'Razlog',
'ipbanononly'                 => 'Blokiraj samo anonimne korisnike',
'ipbcreateaccount'            => 'Spreči pravljenje naloga',
'ipbenableautoblock'          => 'Automatski blokiraj poslednju IP adresu ovog korisnika, i svaku sledeću adresu sa koje se pokuša uređivanje.',
'ipbsubmit'                   => 'Blokiraj ovog korisnika',
'ipbother'                    => 'Ostalo vreme',
'ipboptions'                  => '2 sata:2 hours,1 dan:1 day,3 dana:3 days,1 nedelja:1 week,2 nedelje:2 weeks,1 mesec:1 month,3 meseca:3 months,6 meseci:6 months,1 godina:1 year,beskonačno:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'ostalo',
'badipaddress'                => 'Loša IP adresa',
'blockipsuccesssub'           => 'Blokiranje je uspelo',
'blockipsuccesstext'          => '[[{{ns:special}}:Contributions/$1|$1]] je blokiran.
<br />Vidite [[{{ns:special}}:Ipblocklist|spisak blokiranja]] da biste pregledali blokiranja.',
'unblockip'                   => 'Odblokiraj korisnika',
'unblockiptext'               => 'Upotrebite donji upitnik da biste vratili pravo pisanja
ranije blokiranoj IP adresi ili korisničkom imenu.',
'ipusubmit'                   => 'Odblokiraj ovu adresu',
'unblocked'                   => '[[User:$1|$1]] je odblokiran',
'ipblocklist'                 => 'Spisak blokiranih IP adresa i korisnika',
'blocklistline'               => '$1, $2 blokirao korisnika [[User:$3|$3]], (ističe $4)',
'infiniteblock'               => 'beskonačan',
'expiringblock'               => 'ističe $1',
'anononlyblock'               => 'samo anonimni',
'noautoblockblock'            => 'Autoblokiranje je onemogućeno',
'createaccountblock'          => 'blokirano pravljenje naloga',
'blocklink'                   => 'blokiraj',
'unblocklink'                 => 'odblokiraj',
'contribslink'                => 'prilozi',
'autoblocker'                 => 'Automatski ste blokirani jer je vašu IP adresu skoro koristio "[[User:$1|$1]]". Razlog za blokiranje korisnika $1 je: "\'\'\'$2\'\'\'".',
'blocklogpage'                => 'istorija blokiranja',
'blocklogentry'               => 'je blokirao "[[$1]]" sa vremenom isticanja blokade od $2',
'blocklogtext'                => 'Ovo je istorija blokiranja i odblokiranja korisnika. Automatski
blokirane IP adrese nisu navedene. Pogledajte [[{{ns:special}}:Ipblocklist|spisak blokiranih IP adresa]] za spisak trenutnih zabrana i blokiranja.',
'unblocklogentry'             => 'odblokirao "$1"',
'range_block_disabled'        => 'Administratorska mogućnost da blokira blokove IP adresa je isključena.',
'ipb_expiry_invalid'          => 'Pogrešno vreme trajanja.',
'ipb_already_blocked'         => '"$1" je već blokiran',
'ipb_cant_unblock'            => 'Greška: ID bloka $1 nije nađen. Moguće je da je već odblokiran.',
'ip_range_invalid'            => 'Netačan blok IP adresa.',
'proxyblocker'                => 'Bloker proksija',
'proxyblockreason'            => 'Vaša IP adresa je blokirana jer je ona otvoreni proksi. Molimo kontaktirajte vašeg Internet servis provajdera ili tehničku podršku i obavestite ih o ovom ozbiljnom sigurnosnom problemu.',
'proxyblocksuccess'           => 'Urađeno.',
'sorbsreason'                 => 'Vaša IP adresa je na spisku kao otvoren proksi na DNSBL.',
'sorbs_create_account_reason' => 'Vaša IP adresa se nalazi na spisku kao otvoreni proksi na DNSBL. Ne možete da napravite nalog',

# Developer tools
'lockdb'              => 'Zaključaj bazu',
'unlockdb'            => 'Otključaj bazu',
'lockdbtext'          => 'Zaključavanje baze će svim korisnicima ukinuti mogućnost izmene stranica,
promene korisničkih podešavanja, izmene spiska nadgledanja, i svega ostalog
što zahteva promene u bazi.
Molimo potvrdite da je ovo zaista ono što nameravate da uradite i da ćete
otključati bazu kada završite posao oko njenog održavanja.',
'unlockdbtext'        => 'Otključavanje baze će svim korisnicima vratiti mogućnost izmene stranica,
promene korisničkih podešavanja, izmene spiska nadgledanja, i svega ostalog
što zahteva promene u bazi.
Molimo potvrdite da je ovo zaista ono što nameravate da uradite.',
'lockconfirm'         => 'Da, zaista želim da zaključam bazu.',
'unlockconfirm'       => 'Da, zaista želim da otključam bazu.',
'lockbtn'             => 'Zaključaj bazu',
'unlockbtn'           => 'Otključaj bazu',
'locknoconfirm'       => 'Niste potvrdili svoju nameru.',
'lockdbsuccesssub'    => 'Baza je zaključana',
'unlockdbsuccesssub'  => 'Baza je otključana',
'lockdbsuccesstext'   => 'Baza podataka je zaključana.
<br />Ne zaboravite da je [[{{ns:special}}:Unlockdb|otključate]] kada završite sa održavanjem.',
'unlockdbsuccesstext' => 'Baza podataka je otključana.',
'lockfilenotwritable' => 'Po fajlu za zaključavanje baze podataka ne može da se piše. Da biste zaključali ili otključali bazu, po ovom fajlu mora da bude omogućeno pisanje od strane veb servera.',
'databasenotlocked'   => 'Baza podataka nije zaključana.',

# Move page
'move-page-legend'        => 'Premeštanje stranice',
'movepagetext'            => "Donji upitnik će preimenovati stranicu, premeštajući svu
njenu istoriju na novo ime.
Stari naslov će postati preusmerenje na novi naslov.
Veze ka starom naslovu neće biti promenjene; obavezno
potražite [[{{ns:special}}:DoubleRedirects|dvostruka]] ili [[{{ns:special}}:BrokenRedirects|pokvarena preusmerenja]].
Na vama je odgovornost da veze i dalje idu tamo gde bi i trebalo da idu.

Obratite pažnju da stranica '''neće''' biti pomerena ako već postoji
stranica sa novim naslovom, osim ako je ona prazna ili preusmerenje i nema
istoriju promena. Ovo znači da ne možete preimenovati stranicu na ono ime
sa koga ste je preimenovali ako pogrešite, i ne možete prepisati
postojeću stranicu.

<b>PAŽNJA!</b>
Ovo može biti drastična i neočekivana promena za popularnu stranicu;
molimo da budete sigurni da razumete posledice ovoga pre nego što
nastavite.",
'movepagetalktext'        => "Odgovarajuća stranica za razgovor, ako postoji, biće automatski premeštena istovremeno '''osim ako:'''
*Neprazna stranica za razgovor već postoji pod novim imenom, ili
*Odbeležite donju kućicu.

U tim slučajevima, moraćete ručno da premestite ili spojite stranicu ukoliko to želite.",
'movearticle'             => 'Premesti stranicu',
'newtitle'                => 'Novi naslov',
'movepagebtn'             => 'premesti stranicu',
'pagemovedsub'            => 'Premeštanje uspelo',
'articleexists'           => 'Stranica pod tim imenom već postoji, ili je
ime koje ste izabrali neispravno.
Molimo izaberite drugo ime.',
'talkexists'              => "'''Sama stranica je uspešno premeštena, ali
stranica za razgovor nije mogla biti premeštena jer takva već postoji na novom naslovu. Molimo vas da ih spojite ručno.'''",
'movedto'                 => 'premeštena na',
'movetalk'                => 'Premesti "stranicu za razgovor" takođe, ako je moguće.',
'1movedto2'               => 'je promenio ime članku [[$1]] u [[$2]]',
'1movedto2_redir'         => 'je promenio ime članku [[$1]] u [[$2]] putem preusmerenja',
'movelogpage'             => 'istorija premeštanja',
'movelogpagetext'         => 'Ispod je spisak premeštanja članaka.',
'movereason'              => 'Razlog',
'revertmove'              => 'vrati',
'delete_and_move'         => 'Obriši i premesti',
'delete_and_move_text'    => '==Potrebno brisanje==

Ciljani članak "[[:$1]]" već postoji. Da li želite da ga obrišete da biste napravili mesto za premeštanje?',
'delete_and_move_confirm' => 'Da, obriši stranicu',
'delete_and_move_reason'  => 'Obrisano kako bi se napravilo mesto za premeštanje',
'selfmove'                => 'Izvorni i ciljani naziv su isti; strana ne može da se premesti preko same sebe.',
'immobile_namespace'      => 'Ciljani naziv je posebnog tipa; ne mogu da premeste strane u taj imenski prostor.',

# Export
'export'          => 'Izvezi stranice',
'exporttext'      => 'Možete izvoziti tekst i istoriju promena određene
stranice ili grupe stranica u XML formatu. Ovo onda može biti uvezeno u drugi
viki koji koristi MedijaViki softver preko {{ns:special}}:Import stranice.

Da biste izvozili stranice, unesite nazive u tekstualnom polju ispod, sa jednim naslovom po redu, i odaberite da li želite trenutnu verziju sa svim starim verzijama ili samo trenutnu verziju sa informacijama o poslednjoj izmeni.

U drugom slučaju, možete takođe koristiti vezu, npr. [[{{ns:special}}:Export/{{int:mainpage}}]] za stranicu {{int:mainpage}}.',
'exportcuronly'   => 'Uključi samo trenutnu reviziju, ne celu istoriju',
'exportnohistory' => "----
'''Napomena:''' izvoženje pune istorije strana preko ovog formulara je onemogućeno zbog serverskih razloga.",
'export-submit'   => 'Izvoz',

# Namespace 8 related
'allmessages'               => 'Sistemske poruke',
'allmessagesname'           => 'Ime',
'allmessagesdefault'        => 'Standardni tekst',
'allmessagescurrent'        => 'Trenutni tekst',
'allmessagestext'           => 'Ovo je spisak svih poruka koje su u {{ns:mediawiki}} imenskom prostoru',
'allmessagesnotsupportedDB' => "Stranica '''{{ns:special}}:Allmessages''' ne može da se koristi zato što je '''\$wgUseDatabaseMessages''' isključen.",
'allmessagesfilter'         => 'Filter za imena poruka:',
'allmessagesmodified'       => 'Prikaži samo izmenjene',

# Thumbnails
'thumbnail-more'  => 'uvećaj',
'filemissing'     => 'Nedostaje fajl',
'thumbnail_error' => 'Greška pri pravljenju umanjene slike: $1',

# Special:Import
'import'                     => 'Uvoz stranica',
'importinterwiki'            => 'Transviki uvoženje',
'import-interwiki-text'      => 'Odaberite viki i naziv strane za uvoz.
Datumi revizije i imena urednika će biti sačuvani.
Svi transviki uvozi su zabeleženi u [[Posebno:Log/import|istoriji uvoza]].',
'import-interwiki-history'   => 'Kopiraj sve revizije ove strane',
'import-interwiki-submit'    => 'Uvezi',
'import-interwiki-namespace' => 'Prebaci stranice u imenski prostor:',
'importtext'                 => 'Molimo izvezite fajl iz izvornog vikija koristeći {{ns:special}}:Export, sačuvajte ga kod sebe i pošaljite ovde.',
'importstart'                => 'Uvoženje strana u toku...',
'import-revision-count'      => '$1 {{PLURAL:$1|revizija|revizije|revizija}}',
'importnopages'              => 'Nema strana za uvoz.',
'importfailed'               => 'Uvoz nije uspeo: $1',
'importunknownsource'        => 'Nepoznati tip izvora unosa',
'importcantopen'             => 'Neuspešno otvaranje fajla za uvoz',
'importbadinterwiki'         => 'Loša interviki veza',
'importnotext'               => 'Stranica je prazna ili bez teksta.',
'importsuccess'              => 'Uspešan uvoz!',
'importhistoryconflict'      => 'Postoji konfliktna istorija revizija (možda je ova stranica već uvezena ranije)',
'importnosources'            => 'Nije definisan nijedan izvor transviki uvoženja i direktna slanja istorija su onemogućena.',
'importnofile'               => 'Nije poslat nijedan uvozni fajl.',

# Import log
'importlogpage'                    => 'istorija uvoza',
'importlogpagetext'                => 'Administrativni uvozi stranica sa istorijama izmena sa drugih vikija.',
'import-logentry-upload'           => 'uvezao [[$1]] putem slanja fajla',
'import-logentry-upload-detail'    => '$1 revizija/e',
'import-logentry-interwiki'        => 'premestio sa drugog vikija: $1',
'import-logentry-interwiki-detail' => '$1 revizija/e od $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Moja korisnička stranica',
'tooltip-pt-anonuserpage'         => 'Korisnička stranica IP adrese sa koje uređujete',
'tooltip-pt-mytalk'               => 'Moja stranica za razgovor',
'tooltip-pt-anontalk'             => 'Razgovor o prilozima sa ove IP adrese',
'tooltip-pt-preferences'          => 'Moja korisnička podešavanja',
'tooltip-pt-watchlist'            => 'Spisak članaka koje nadgledate',
'tooltip-pt-mycontris'            => 'Spisak mojih priloga',
'tooltip-pt-login'                => 'Preporučuje se da se prijavite, ali nije obavezno',
'tooltip-pt-anonlogin'            => 'Preporučuje se da se prijavite, ali nije obavezno',
'tooltip-pt-logout'               => 'Odjavi se',
'tooltip-ca-talk'                 => 'Razgovor o članku',
'tooltip-ca-edit'                 => 'Možete urediti ovu stranicu. Molimo koristite pretpregled pre sačuvavanja.',
'tooltip-ca-addsection'           => 'Dodajte komentar na ovu diskusiju',
'tooltip-ca-viewsource'           => 'Ova stranica je zaključana. Možete videti njen izvor',
'tooltip-ca-history'              => 'Prethodne verzije ove stranice',
'tooltip-ca-protect'              => 'Zaštiti ovu stranicu',
'tooltip-ca-delete'               => 'Obriši ovu stranicu',
'tooltip-ca-undelete'             => 'Vraćati izmene koje su načinjene pre brisanja stranice',
'tooltip-ca-move'                 => 'Premesti ovu stranicu',
'tooltip-ca-watch'                => 'Dodajte ovu stranicu na Vaš spisak nadgledanja',
'tooltip-ca-unwatch'              => 'Uklonite ovu stranicu sa Vašeg spiska nadgledanja',
'tooltip-search'                  => 'Pretražite ovaj viki',
'tooltip-p-logo'                  => 'Glavna strana',
'tooltip-n-mainpage'              => 'Posetite glavnu stranu',
'tooltip-n-portal'                => 'O projektu, šta možete da radite i gde da pronađete stvari',
'tooltip-n-currentevents'         => 'Saznajte više o aktuelnostima',
'tooltip-n-recentchanges'         => 'Spisak skorašnjih izmena na vikiju',
'tooltip-n-randompage'            => 'Učitavaj slučajnu stranicu',
'tooltip-n-help'                  => 'Mesto gde možete da naučite nešto',
'tooltip-t-whatlinkshere'         => 'Spisak svih stranica koje vezuju na ovu',
'tooltip-t-recentchangeslinked'   => 'Skorašnje izmene na člancima povezanim sa ove stranice',
'tooltip-feed-rss'                => 'RSS fid za ovu stranicu',
'tooltip-feed-atom'               => 'Atom fid za ovu stranicu',
'tooltip-t-contributions'         => 'Pogledaj spisak priloga ovog korisnika',
'tooltip-t-emailuser'             => 'Pošalji elektronsku poštu ovom korisniku',
'tooltip-t-upload'                => 'Pošalji slike i medija fajlove',
'tooltip-t-specialpages'          => 'Spisak svih posebnih stranica',
'tooltip-ca-nstab-main'           => 'Pogledajte članak',
'tooltip-ca-nstab-user'           => 'Pogledajte korisničku stranicu',
'tooltip-ca-nstab-media'          => 'Pogledajte medija stranicu',
'tooltip-ca-nstab-special'        => 'Ovo je posebna stranica, ne možete je menjati',
'tooltip-ca-nstab-image'          => 'Pogledajte stranicu slike',
'tooltip-ca-nstab-mediawiki'      => 'Pogledajte sistemsku poruku',
'tooltip-ca-nstab-template'       => 'Pogledajte šablon',
'tooltip-ca-nstab-help'           => 'Pogledajte stranicu za pomoć',
'tooltip-ca-nstab-category'       => 'Pogledajte stranicu kategorije',
'tooltip-minoredit'               => 'Naznačite da se radi o maloj izmeni',
'tooltip-save'                    => 'Snimite Vaše izmene',
'tooltip-preview'                 => 'Pretpregled Vaših izmena, molimo koristite ovo pre snimanja!',
'tooltip-diff'                    => 'Prikaži koje promene ste napravili na tekstu.',
'tooltip-compareselectedversions' => 'Pogledaj razlike između dve odabrane verzije ove stranice.',
'tooltip-watch'                   => 'Dodajte ovu stranicu na Vaš spisak nadgledanja',
'tooltip-recreate'                => 'Ponovo napravite ovu stranu uprkos tome što je obrisana',

# Stylesheets
'common.css'   => '/** CSS stavljen ovde će se odnositi na sve kože */',
'monobook.css' => '/* CSS stavljen ovde će se odnositi na korisnike Monobuk kože */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metapodaci onemogućeni za ovaj server.',
'nocreativecommons' => 'Creative Commons RDF metapodaci onemogućeni za ovaj server.',
'notacceptable'     => 'Viki server ne može da pruži podatke u onom formatu koji vaš klijent može da pročita.',

# Attribution
'anonymous'        => 'Anonimni korisnik sajta {{SITENAME}}',
'siteuser'         => '{{SITENAME}} korisnik $1',
'lastmodifiedatby' => 'Ovu stranicu je poslednji put promenio $3 u $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Bazirano na radu korisnika $1.',
'others'           => 'ostali',
'siteusers'        => '{{SITENAME}} korisnik (korisnici) $1',
'creditspage'      => 'Zasluge za stranicu',
'nocredits'        => 'Nisu dostupne informacije o zaslugama za ovu stranicu.',

# Spam protection
'spamprotectiontitle' => 'Filter za zaštitu od neželjenih poruka',
'spamprotectiontext'  => 'Strana koju želite da sačuvate je blokirana od strane filtera za neželjene poruke. Ovo je verovatno izazvano vezom ka spoljašnjem sajtu.',
'spamprotectionmatch' => 'Sledeći tekst je izazvao naš filter za neželjene poruke: $1',
'spambot_username'    => 'Čišćenje neželjenih poruka u MedijaVikiju',
'spam_reverting'      => 'Vraćanje na staru reviziju koja ne sadrži veze ka $1',
'spam_blanking'       => 'Sve revizije su sadržale veze ka $1, pražnjenje',

# Info page
'infosubtitle'   => 'Informacije za stranicu',
'numedits'       => 'Broj promena (članak): $1',
'numtalkedits'   => 'Broj promena (stranica za razgovor): $1',
'numwatchers'    => 'Broj korisnika koji nadgledaju: $1',
'numauthors'     => 'Broj različitih autora (članak): $1',
'numtalkauthors' => 'Broj različitih autora (stranica za razgovor): $1',

# Math options
'mw_math_png'    => 'Uvek prikaži PNG',
'mw_math_simple' => 'HTML ako je vrlo jednostavno, inače PNG',
'mw_math_html'   => 'HTML ako je moguće, inače PNG',
'mw_math_source' => 'Ostavi kao TeH (za tekstualne brauzere)',
'mw_math_modern' => 'Preporučeno za savremene brauzere',
'mw_math_mathml' => 'MathML ako je moguće (eksperimentalno)',

# Patrolling
'markaspatrolleddiff'        => 'Označi kao patroliran',
'markaspatrolledtext'        => 'Označi ovaj članak kao patroliran',
'markedaspatrolled'          => 'Označen kao patroliran',
'markedaspatrolledtext'      => 'Izabrana revizija je označena kao patrolirana.',
'rcpatroldisabled'           => 'Patrola skorašnjih izmena onemogućena',
'rcpatroldisabledtext'       => 'Patrola skorašnjih izmena je trenutno onemogućena.',
'markedaspatrollederror'     => 'Nemoguće označiti kao patrolirano',
'markedaspatrollederrortext' => 'Morate izabrati reviziju da biste označili kao patrolirano.',

# Image deletion
'deletedrevision' => 'Obrisana stara revizija $1',

# Browsing diffs
'previousdiff' => '← Prethodna izmena',
'nextdiff'     => 'Sledeća izmena →',

# Media information
'mediawarning' => "'''Upozorenje''': Ovaj fajl sadrži loš kod, njegovim izvršavanjem možete da ugrozite vaš sistem.<hr />",
'imagemaxsize' => 'Ograniči slike na stranama za razgovor o slikama na:',
'thumbsize'    => 'Veličina umanjenog prikaza :',

# Special:NewImages
'newimages'             => 'Galerija novih slika',
'imagelisttext'         => "Ispod je spisak od '''$1''' {{PLURAL:$1|fajla|fajla|fajlova}} poređanih $2.",
'showhidebots'          => '($1 botove)',
'noimages'              => 'Nema ništa da se vidi',
'ilsubmit'              => 'Traži',
'bydate'                => 'po datumu',
'sp-newimages-showfrom' => 'Prikaži nove slike počevši od $1',

# Variants for Serbian language
'variantname-sr-ec' => 'ћирилица',
'variantname-sr-el' => 'latinica',
'variantname-sr'    => 'disable',

# Metadata
'metadata'          => 'Metapodaci',
'metadata-help'     => 'Ovaj fajl sadrži dodatne informacije, koje su verovatno dodali digitalni fotoaparat ili skener koji su korišćeni da bi se napravila ili digitalizovala slika. Ako je prvobitno stanje fajla promenjeno, moguće je da neki detalji ne opisuju u potpunosti izmenjenu sliku.',
'metadata-expand'   => 'Pokaži detalje',
'metadata-collapse' => 'Sakrij detalje',
'metadata-fields'   => 'Polja EXIF metapodataka navedena u ovoj poruci će biti ubačena na stranu o slici kada se raširi tabela za metapodatke. Ostala će biti sakrivena po podrazumevanom.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Širina',
'exif-imagelength'                 => 'Visina',
'exif-bitspersample'               => 'Bitova po komponenti',
'exif-compression'                 => 'Šema kompresije',
'exif-photometricinterpretation'   => 'Kompozicija piksela',
'exif-orientation'                 => 'Orijentacija',
'exif-samplesperpixel'             => 'Broj komponenti',
'exif-planarconfiguration'         => 'Princip rasporeda podataka',
'exif-ycbcrsubsampling'            => 'Odnos komponente Y prema C',
'exif-ycbcrpositioning'            => 'Razmeštaj komponenata Y i C',
'exif-xresolution'                 => 'Horizonatalna rezolucija',
'exif-yresolution'                 => 'Vertikalna rezolucija',
'exif-resolutionunit'              => 'Jedinica rezolucije',
'exif-stripoffsets'                => 'Položaj bloka podataka',
'exif-rowsperstrip'                => 'Broj redova u bloku',
'exif-stripbytecounts'             => 'Veličina kompresovanog bloka',
'exif-jpeginterchangeformat'       => 'Udaljenost JPEG pregleda od početka fajla',
'exif-jpeginterchangeformatlength' => 'Količina bajtova JPEG pregleda',
'exif-transferfunction'            => 'Funkcija preoblikovanja kolor prostora',
'exif-whitepoint'                  => 'Hromacitet bele tačke',
'exif-primarychromaticities'       => 'Hromacitet primarnih boja',
'exif-ycbcrcoefficients'           => 'Matrični koeficijenti transformacije kolor prostora',
'exif-referenceblackwhite'         => 'Mesto bele i crne tačke',
'exif-datetime'                    => 'Datum poslednje promene fajla',
'exif-imagedescription'            => 'Ime slike',
'exif-make'                        => 'Proizvođač kamere',
'exif-model'                       => 'Model kamere',
'exif-software'                    => 'Korišćen softver',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Nosilac prava',
'exif-exifversion'                 => 'Exif verzija',
'exif-flashpixversion'             => 'Podržana verzija Flešpiksa',
'exif-colorspace'                  => 'Prostor boje',
'exif-componentsconfiguration'     => 'Značenje svake od komponenti',
'exif-compressedbitsperpixel'      => 'Mod kompresije slike',
'exif-pixelydimension'             => 'Puna visina slike',
'exif-pixelxdimension'             => 'Puna širina slike',
'exif-makernote'                   => 'Napomene proizvođača',
'exif-usercomment'                 => 'Korisnički komentar',
'exif-relatedsoundfile'            => 'Povezani zvučni zapis',
'exif-datetimeoriginal'            => 'Datum i vreme slikanja',
'exif-datetimedigitized'           => 'Datum i vreme digitalizacije',
'exif-subsectime'                  => 'Deo sekunde u kojem je slikano',
'exif-subsectimeoriginal'          => 'Deo sekunde u kojem je fotografisano',
'exif-subsectimedigitized'         => 'Deo sekunde u kojem je digitalizovano',
'exif-exposuretime'                => 'Ekspozicija',
'exif-exposuretime-format'         => '$1 sek ($2)',
'exif-fnumber'                     => 'F broj otvora blende',
'exif-exposureprogram'             => 'Program ekspozicije',
'exif-spectralsensitivity'         => 'Spektralna osetljivost',
'exif-isospeedratings'             => 'ISO vrednost',
'exif-oecf'                        => 'Optoelektronski faktor konverzije',
'exif-shutterspeedvalue'           => 'Brzina zatvarača',
'exif-aperturevalue'               => 'Otvor blende',
'exif-brightnessvalue'             => 'Svetlost',
'exif-exposurebiasvalue'           => 'Kompenzacija ekspozicije',
'exif-maxaperturevalue'            => 'Minimalni broj otvora blende',
'exif-subjectdistance'             => 'Udaljenost do objekta',
'exif-meteringmode'                => 'Režim merača vremena',
'exif-lightsource'                 => 'Izvor svetlosti',
'exif-flash'                       => 'Blic',
'exif-focallength'                 => 'Fokusna daljina sočiva',
'exif-subjectarea'                 => 'Položaj i površina objekta snimka',
'exif-flashenergy'                 => 'Energija blica',
'exif-spatialfrequencyresponse'    => 'Prostorna frekvencijska karakteristika',
'exif-focalplanexresolution'       => 'Vodoravna rezolucija fokusne ravni',
'exif-focalplaneyresolution'       => 'Horizonatlna rezolucija fokusne ravni',
'exif-focalplaneresolutionunit'    => 'Jedinica rezolucije fokusne ravni',
'exif-subjectlocation'             => 'Položaj subjekta',
'exif-exposureindex'               => 'Indeks ekspozicije',
'exif-sensingmethod'               => 'Tip senzora',
'exif-filesource'                  => 'Izvorni fajl',
'exif-scenetype'                   => 'Tip scene',
'exif-cfapattern'                  => 'CFA šablon',
'exif-customrendered'              => 'Dodatna obrada slike',
'exif-exposuremode'                => 'Režim izbora ekspozicije',
'exif-whitebalance'                => 'Balans bele boje',
'exif-digitalzoomratio'            => 'Odnos digitalnog zuma',
'exif-focallengthin35mmfilm'       => 'Ekvivalent fokusne daljine za 35 mm film',
'exif-scenecapturetype'            => 'Tip scene na snimku',
'exif-gaincontrol'                 => 'Kontrola osvetljenosti',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Saturacija',
'exif-sharpness'                   => 'Oštrina',
'exif-devicesettingdescription'    => 'Opis podešavanja uređaja',
'exif-subjectdistancerange'        => 'Raspon udaljenosti subjekata',
'exif-imageuniqueid'               => 'Jedinstveni identifikator slike',
'exif-gpsversionid'                => 'Verzija bloka GPS-informacije',
'exif-gpslatituderef'              => 'Severna ili južna širina',
'exif-gpslatitude'                 => 'Širina',
'exif-gpslongituderef'             => 'Istočna ili zapadna dužina',
'exif-gpslongitude'                => 'Dužina',
'exif-gpsaltituderef'              => 'Visina ispod ili iznad mora',
'exif-gpsaltitude'                 => 'Visina',
'exif-gpstimestamp'                => 'Vreme po GPS-u (atomski sat)',
'exif-gpssatellites'               => 'Upotrebljeni sateliti',
'exif-gpsstatus'                   => 'Status prijemnika',
'exif-gpsmeasuremode'              => 'Režim merenja',
'exif-gpsdop'                      => 'Preciznost merenja',
'exif-gpsspeedref'                 => 'Jedinica brzine',
'exif-gpsspeed'                    => 'Brzina GPS prijemnika',
'exif-gpstrackref'                 => 'Tip azimuta prijemnika (pravi ili magnetni)',
'exif-gpstrack'                    => 'Azimut prijemnika',
'exif-gpsimgdirectionref'          => 'Tip azimuta slike (pravi ili magnetni)',
'exif-gpsimgdirection'             => 'Azimut slike',
'exif-gpsmapdatum'                 => 'Korišćeni geodetski koordinatni sistem',
'exif-gpsdestlatituderef'          => 'Indeks geografske širine objekta',
'exif-gpsdestlatitude'             => 'Geografska širina objekta',
'exif-gpsdestlongituderef'         => 'Indeks geografske dužine objekta',
'exif-gpsdestlongitude'            => 'Geografska dužina objekta',
'exif-gpsdestbearingref'           => 'Indeks azimuta objekta',
'exif-gpsdestbearing'              => 'Azimut objekta',
'exif-gpsdestdistanceref'          => 'Merne jedinice udaljenosti objekta',
'exif-gpsdestdistance'             => 'Udaljenost objekta',
'exif-gpsprocessingmethod'         => 'Ime metode obrade GPS podataka',
'exif-gpsareainformation'          => 'Ime GPS područja',
'exif-gpsdatestamp'                => 'GPS datum',
'exif-gpsdifferential'             => 'GPS diferencijalna korekcija',

# EXIF attributes
'exif-compression-1' => 'Nekompresovan',

'exif-orientation-1' => 'Normalno', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Obrnuto po horizontali', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Zaokrenuto 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Obrnuto po vertikali', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Zaokrenuto 90° suprotno od smera kazaljke na satu i obrnuto po vertikali', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Zaokrenuto 90° u smeru kazaljke na satu', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Zaokrenuto 90° u smeru kazaljke na satu i obrnuto po vertikali', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Zaokrenuto 90° suprotno od smera kazaljke na satu', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'delimični format',
'exif-planarconfiguration-2' => 'planarni format',

'exif-componentsconfiguration-0' => 'ne postoji',

'exif-exposureprogram-0' => 'Nepoznato',
'exif-exposureprogram-1' => 'Ručno',
'exif-exposureprogram-2' => 'Normalni program',
'exif-exposureprogram-3' => 'Prioritet otvora blende',
'exif-exposureprogram-4' => 'Prioritet zatvarača',
'exif-exposureprogram-5' => 'Umetnički program (na bazi nužne dubine polja)',
'exif-exposureprogram-6' => 'Sportski program (na bazi što bržeg zatvarača)',
'exif-exposureprogram-7' => 'Portretni režim (za krupne kadrove sa neoštrom pozadinom)',
'exif-exposureprogram-8' => 'Režim pejzaža (za slike pejzaža sa oštrom pozadinom)',

'exif-subjectdistance-value' => '$1 metara',

'exif-meteringmode-0'   => 'Nepoznato',
'exif-meteringmode-1'   => 'Prosek',
'exif-meteringmode-2'   => 'Prosek sa težištem na sredini',
'exif-meteringmode-3'   => 'Tačka',
'exif-meteringmode-4'   => 'Više tačaka',
'exif-meteringmode-5'   => 'Matrični',
'exif-meteringmode-6'   => 'Delimični',
'exif-meteringmode-255' => 'Drugo',

'exif-lightsource-0'   => 'Nepoznato',
'exif-lightsource-1'   => 'Dnevna svetlost',
'exif-lightsource-2'   => 'Fluorescentno',
'exif-lightsource-3'   => 'Volfram (svetlo)',
'exif-lightsource-4'   => 'Blic',
'exif-lightsource-9'   => 'Lepo vreme',
'exif-lightsource-10'  => 'Oblačno vreme',
'exif-lightsource-11'  => 'Senka',
'exif-lightsource-12'  => 'Fluorescentna svetlost (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescentna svetlost (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescentna svetlost (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Bela fluorescencija (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardno svetlo A',
'exif-lightsource-18'  => 'Standardno svetlo B',
'exif-lightsource-19'  => 'Standardno svetlo C',
'exif-lightsource-24'  => 'ISO studijski volfram',
'exif-lightsource-255' => 'Drugi izvor svetla',

'exif-focalplaneresolutionunit-2' => 'inči',

'exif-sensingmethod-1' => 'Nedefinisano',
'exif-sensingmethod-2' => 'Jednokristalni matrični senzor',
'exif-sensingmethod-3' => 'Dvokristalni matrični senzor',
'exif-sensingmethod-4' => 'Trokristalni matrični senzor',
'exif-sensingmethod-5' => 'Sekvencijalni matrični senzor',
'exif-sensingmethod-7' => 'Trobojni linearni senzor',
'exif-sensingmethod-8' => 'Sekvencijalni linearni senzor',

'exif-filesource-3' => 'Digitalni fotoaparat',

'exif-scenetype-1' => 'Direktno fotografisana slika',

'exif-customrendered-0' => 'Normalni proces',
'exif-customrendered-1' => 'Nestadardni proces',

'exif-exposuremode-0' => 'Automatski',
'exif-exposuremode-1' => 'Ručno',
'exif-exposuremode-2' => 'Automatski sa zadatim rasponom',

'exif-whitebalance-0' => 'Automatski',
'exif-whitebalance-1' => 'Ručno',

'exif-scenecapturetype-0' => 'Standardno',
'exif-scenecapturetype-1' => 'Pejzaž',
'exif-scenecapturetype-2' => 'Portret',
'exif-scenecapturetype-3' => 'Noćno',

'exif-gaincontrol-0' => 'Nema',
'exif-gaincontrol-1' => 'Malo povećanje',
'exif-gaincontrol-2' => 'Veliko povećanje',
'exif-gaincontrol-3' => 'Malo smanjenje',
'exif-gaincontrol-4' => 'Veliko smanjenje',

'exif-contrast-0' => 'Normalno',
'exif-contrast-1' => 'Meko',
'exif-contrast-2' => 'Tvrdo',

'exif-saturation-0' => 'Normalno',
'exif-saturation-1' => 'Niska saturacija',
'exif-saturation-2' => 'Visoka saturacija',

'exif-sharpness-0' => 'Normalno',
'exif-sharpness-1' => 'Meko',
'exif-sharpness-2' => 'Tvrdo',

'exif-subjectdistancerange-0' => 'Nepoznato',
'exif-subjectdistancerange-1' => 'Krupni kadar',
'exif-subjectdistancerange-2' => 'Bliski kadar',
'exif-subjectdistancerange-3' => 'Daleki kadar',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Sever',
'exif-gpslatitude-s' => 'Jug',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Istok',
'exif-gpslongitude-w' => 'Zapad',

'exif-gpsstatus-a' => 'Merenje u toku',
'exif-gpsstatus-v' => 'Spreman za prenos',

'exif-gpsmeasuremode-2' => 'Dvodimenzionalno merenje',
'exif-gpsmeasuremode-3' => 'Trodimenzionalno merenje',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometri na čas',
'exif-gpsspeed-m' => 'Milje na čas',
'exif-gpsspeed-n' => 'Čvorovi',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Pravi pravac',
'exif-gpsdirection-m' => 'Magnetni pravac',

# External editor support
'edit-externally'      => 'Izmenite ovaj fajl koristeći spoljašnju aplikaciju',
'edit-externally-help' => 'Pogledajte [http://www.mediawiki.org/wiki/Manual:External_editors uputstvo za podešavanje] za više informacija.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'sve',
'imagelistall'     => 'sve',
'watchlistall2'    => 'sve',
'namespacesall'    => 'svi',

# E-mail address confirmation
'confirmemail'            => 'Potvrdite adresu e-pošte',
'confirmemail_noemail'    => 'Nemate potvrđenu adresu vaše e-pošte u vašim [[Special:Preferences|korisničkim podešavanjima interfejsa]].',
'confirmemail_text'       => 'Ova viki zahteva da potvrdite adresu vaše e-pošte pre nego što koristite mogućnosti e-pošte. Aktivirajte dugme ispod kako biste poslali poštu za potvrdu na vašu adresu. Pošta uključuje vezu koja sadrži kod; učitajte tu vezu u vaš brauzer da biste potvrdili da je adresa vaše e-pošte validna.',
'confirmemail_send'       => 'Pošalji kod za potvrdu',
'confirmemail_sent'       => 'E-pošta za potvrđivanje poslata.',
'confirmemail_sendfailed' => 'Pošta za potvrđivanje nije poslata. Proverita adresu zbog nepravilnih karaktera.',
'confirmemail_invalid'    => 'Netačan kod za potvrdu. Moguće je da je kod istekao.',
'confirmemail_needlogin'  => 'Morate da se $1 da biste potvrdili adresu vaše e-pošte.',
'confirmemail_success'    => 'Adresa vaše e-pošte je potvrđena. Možete sada da se prijavite i uživate u vikiju.',
'confirmemail_loggedin'   => 'Adresa vaše e-pošte je sada potvrđena.',
'confirmemail_error'      => 'Nešto je pošlo po zlu prilikom snimanja vaše potvrde.',
'confirmemail_subject'    => '{{SITENAME}} adresa e-pošte za potvrđivanje',
'confirmemail_body'       => 'Neko, verovatno vi, je sa IP adrese $1 registrovao nalog "$2" sa ovom adresom e-pošte na sajtu {{SITENAME}}.

Da potvrdite da ovaj nalog stvarno pripada vama i da aktivirate mogućnost e-pošte na sajtu {{SITENAME}}, otvorite ovu vezu u vašem brauzeru:

$3

Ako ovo *niste* vi, ne pratite vezu. Ovaj kod za potvrdu će isteći u $4.',

# Scary transclusion
'scarytranscludedisabled' => '[Interviki uključivanje je onemogućeno]',
'scarytranscludefailed'   => '[Donošenje šablona neuspešno; žao nam je]',
'scarytranscludetoolong'  => '[URL je predugačak; žao nam je]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Vraćanja za ovaj članak:<br />
$1
</div>',
'trackbackremove'   => '([$1 Brisanje])',
'trackbacklink'     => 'Vraćanje',
'trackbackdeleteok' => 'Vraćanje je uspešno obrisano.',

# Delete conflict
'deletedwhileediting' => 'Upozorenje: Ova strana je obrisana pošto ste počeli uređivanje!',
'confirmrecreate'     => "Korisnik [[User:$1|$1]] ([[User_talk:$1|razgovor]]) je obrisao ovaj članak pošto ste počeli uređivanje sa razlogom:
: ''$2''
Molimo potvrdite da stvarno želite da ponovo napravite ovaj članak.",
'recreate'            => 'Ponovo napravi',

# HTML dump
'redirectingto' => 'Preusmeravam na [[:$1]]...',

# action=purge
'confirm_purge'        => 'Da li želite očistiti keš ove stranice?

$1',
'confirm_purge_button' => 'Da',

# AJAX search
'searchcontaining' => "Pretraži članke koji sadrže ''$1''.",
'searchnamed'      => "Pretraži članke koji se zovu ''$1''.",
'articletitles'    => "Članci počevši od ''$1''",
'hideresults'      => 'Sakrij rezultate',

# Multipage image navigation
'imgmultipageprev' => '&larr; prethodna stranica',
'imgmultipagenext' => 'sledeća stranica &rarr;',
'imgmultigo'       => 'Idi!',

# Table pager
'ascending_abbrev'         => 'rast',
'descending_abbrev'        => 'opad',
'table_pager_next'         => 'Sledeća stranica',
'table_pager_prev'         => 'Prethodna stranica',
'table_pager_first'        => 'Prva stranica',
'table_pager_last'         => 'Poslednja stranica',
'table_pager_limit'        => 'Prikaži $1 delova informacije po stranici',
'table_pager_limit_submit' => 'Idi',
'table_pager_empty'        => 'Bez rezultata',

# Auto-summaries
'autoredircomment' => 'Preusmerenje na [[$1]]',

# Special:Version
'version' => 'Verzija', # Not used as normal message but as header for the special page itself

# Special:FilePath
'filepath'        => 'Putanja fajla',
'filepath-page'   => 'Fajl:',
'filepath-submit' => 'Putanja',

# Special:SpecialPages
'specialpages' => 'Posebne stranice',

);
