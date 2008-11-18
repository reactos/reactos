<?php
/** Croatian (Hrvatski)
 *
 * @ingroup Language
 * @file
 *
 * @author Dalibor Bosits
 * @author Demicx
 * @author Dnik
 * @author Luka Krstulovic
 * @author MayaSimFan
 * @author Roberta F.
 * @author SpeedyGonsales
 * @author Suradnik13
 * @author Treecko
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard'  => 'Standardna',
	'nostalgia'  => 'Nostalgija',
	'cologneblue'  => 'Kölnska plava',
	'monobook'  => 'MonoBook',
	'myskin'  => 'MySkin',
	'chick'  => 'Chick'
);

$namespaceNames = array(
	NS_MEDIA           => 'Mediji',
	NS_SPECIAL         => 'Posebno',
	NS_MAIN            => '',
	NS_TALK            => 'Razgovor',
	NS_USER            => 'Suradnik',
	NS_USER_TALK       => 'Razgovor_sa_suradnikom',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK    => 'Razgovor_$1',
	NS_IMAGE           => 'Slika',
	NS_IMAGE_TALK      => 'Razgovor_o_slici',
	NS_MEDIAWIKI       => 'MediaWiki',
	NS_MEDIAWIKI_TALK  => 'MediaWiki_razgovor',
	NS_TEMPLATE        => 'Predložak',
	NS_TEMPLATE_TALK   => 'Razgovor_o_predlošku',
	NS_HELP            => 'Pomoć',
	NS_HELP_TALK       => 'Razgovor_o_pomoći',
	NS_CATEGORY        => 'Kategorija',
	NS_CATEGORY_TALK   => 'Razgovor_o_kategoriji'
);

$datePreferences = false;

$defaultDateFormat = 'dmy';

$dateFormats = array(
	'dmy time' => 'H:i',
	'dmy date' => 'j. F Y.',
	'dmy both' => 'H:i, j. F Y.',
);

$separatorTransformTable = array(',' => '.', '.' => ',' );

$fallback8bitEncoding = 'iso-8859-2';

$linkTrail = '/^([čšžćđßa-z]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Podcrtane poveznice',
'tog-highlightbroken'         => 'Istakni prazne poveznice <font color="#bb0000"><u>crvenom bojom</u></font> (inače, upitnikom na kraju).',
'tog-justify'                 => 'Poravnaj odlomke i zdesna',
'tog-hideminor'               => 'Sakrij manje izmjene na stranici Nedavnih promjena',
'tog-extendwatchlist'         => 'Proširi popis praćenih stranica tako da prikaže sve odgovarajuće promjene',
'tog-usenewrc'                => 'Poboljšan izgled Nedavnih promjena (nije za sve preglednike)',
'tog-numberheadings'          => 'Automatski označi naslove brojevima',
'tog-showtoolbar'             => 'Prikaži traku s alatima za uređivanje',
'tog-editondblclick'          => 'Dvoklik otvara uređivanje stranice (JavaScript)',
'tog-editsection'             => 'Prikaži poveznice za uređivanje pojedinih odlomaka',
'tog-editsectiononrightclick' => 'Pritiskom na desnu tipku miša otvori uređivanje pojedinih odlomaka (JavaScript)',
'tog-showtoc'                 => 'U člancima s više od tri odlomka prikaži tablicu sadržaja.',
'tog-rememberpassword'        => 'Zapamti lozinku između prijava',
'tog-editwidth'               => 'Okvir za uređivanje zauzima cijelu širinu',
'tog-watchcreations'          => 'Dodaj članke koje kreiram na moj popis praćenja',
'tog-watchdefault'            => 'Dodaj sve nove i izmijenjene stranice u popis praćenja',
'tog-watchmoves'              => 'Dodaj sve stranice koje premjestim na popis praćenja',
'tog-watchdeletion'           => 'Dodaj sve stranice koje izbrišem na popis praćenja',
'tog-minordefault'            => 'Normalno označavaj sve moje izmjene kao manje',
'tog-previewontop'            => 'Prikaži kako će stranica izgledati iznad okvira za uređivanje',
'tog-previewonfirst'          => 'Prikaži kako će stranica izgledati čim otvorim uređivanje',
'tog-nocache'                 => 'Isključi međuspremnik (cache) stranica.',
'tog-enotifwatchlistpages'    => 'Pošalji mi e-mail kod izmjene stranice u popisu praćenja',
'tog-enotifusertalkpages'     => 'Pošalji mi e-mail kod izmjene moje stranice za razgovor',
'tog-enotifminoredits'        => 'Pošalji mi e-mail i kod manjih izmjena',
'tog-enotifrevealaddr'        => 'Prikaži moju e-mail adresu u obavijestima o izmjeni',
'tog-shownumberswatching'     => 'Prikaži broj suradnika koji prate stranicu (u nedavnim izmjenama, popisu praćenja i samim člancima)',
'tog-fancysig'                => 'Običan potpis (bez automatske poveznice)',
'tog-externaleditor'          => 'Uvijek koristi vanjski editor',
'tog-externaldiff'            => 'Uvijek koristi vanjski program za usporedbu',
'tog-showjumplinks'           => 'Uključi pomoćne poveznice "Skoči na"',
'tog-uselivepreview'          => 'Uključi trenutačni pretpregled (JavaScript) (eksperimentalno)',
'tog-forceeditsummary'        => 'Podsjeti me ako sažetak uređivanja ostavljam praznim',
'tog-watchlisthideown'        => 'Sakrij moja uređivanja s popisa praćenja',
'tog-watchlisthidebots'       => 'Sakrij uređivanja botova s popisa praćenja',
'tog-watchlisthideminor'      => 'Sakrij manje promjene s popisa praćenja',
'tog-nolangconversion'        => 'Isključi pretvaranje pisma (latinica-ćirilica, kineske varijante itd.) ako to wiki podržava',
'tog-ccmeonemails'            => 'Pošalji mi kopiju e-maila kojeg pošaljem drugim suradnicima',
'tog-diffonly'                => 'Ne prikazuj sadržaj stranice prilikom usporedbe inačica',
'tog-showhiddencats'          => 'Prikaži skrivene kategorije',

'underline-always'  => 'Uvijek',
'underline-never'   => 'Nikad',
'underline-default' => 'Prema postavkama preglednika',

'skinpreview' => '(Pregled)',

# Dates
'sunday'        => 'nedjelja',
'monday'        => 'ponedjeljak',
'tuesday'       => 'utorak',
'wednesday'     => 'srijeda',
'thursday'      => 'četvrtak',
'friday'        => 'petak',
'saturday'      => 'subota',
'sun'           => 'Ned',
'mon'           => 'Pon',
'tue'           => 'Uto',
'wed'           => 'Sri',
'thu'           => 'Čet',
'fri'           => 'Pet',
'sat'           => 'Sub',
'january'       => 'siječnja',
'february'      => 'veljače',
'march'         => 'ožujka',
'april'         => 'travnja',
'may_long'      => 'svibnja',
'june'          => 'lipnja',
'july'          => 'srpnja',
'august'        => 'kolovoza',
'september'     => 'rujna',
'october'       => 'listopada',
'november'      => 'studenog',
'december'      => 'prosinca',
'january-gen'   => 'siječnja',
'february-gen'  => 'veljače',
'march-gen'     => 'ožujka',
'april-gen'     => 'travnja',
'may-gen'       => 'svibnja',
'june-gen'      => 'lipnja',
'july-gen'      => 'srpnja',
'august-gen'    => 'kolovoza',
'september-gen' => 'rujna',
'october-gen'   => 'listopada',
'november-gen'  => 'studenog',
'december-gen'  => 'prosinca',
'jan'           => 'sij',
'feb'           => 'velj',
'mar'           => 'ožu',
'apr'           => 'tra',
'may'           => 'svi',
'jun'           => 'lip',
'jul'           => 'srp',
'aug'           => 'kol',
'sep'           => 'ruj',
'oct'           => 'lis',
'nov'           => 'stu',
'dec'           => 'pro',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorija|Kategorije}}',
'category_header'                => 'Članci u kategoriji "$1"',
'subcategories'                  => 'Potkategorije',
'category-media-header'          => 'Mediji u kategoriji "$1":',
'category-empty'                 => "''U ovoj kategoriji trenutačno nema članaka ni medija.''",
'hidden-categories'              => '{{PLURAL:$1|Skrivena kategorija|Skrivene kategorije|Skrivenih kategorija}}',
'hidden-category-category'       => 'Skrivene kategorije', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ova kategorija ima samo sljedeću podkategoriju.|Ova kategorija ima {{PLURAL:$1|podkategoriju|$1 podkategorije|$1 podkategorija}}, od njih $2 ukupno.}}',
'category-subcat-count-limited'  => 'Ova kategorija ima {{PLURAL:$1|podkategoriju|$1 podkategorije|$1 podkategorija}}.',
'category-article-count'         => '{{PLURAL:$2|Ova kategorija sadrži samo sljedeću stranicu.|{{PLURAL:$1|stranica je|$1 stranice su|$1 stranica su}} u ovoj kategoriji, od njih $2 ukupno.}}',
'category-article-count-limited' => '{{PLURAL:$1|stranica je|$1 stranice su|$1 stranica su}} u ovoj kategoriji.',
'category-file-count'            => '{{PLURAL:$2|Ova kategorija sadrži samo sljedeću datoteku.|{{PLURAL:$1|datoteka je|$1 datoteke su|$1 datoteka su}} u ovoj kategoriji, od njih $2 ukupno.}}',
'category-file-count-limited'    => '{{PLURAL:$1|datoteka je|$1 datoteke su|$1 datoteka su}} u ovoj kategoriji.',
'listingcontinuesabbrev'         => 'nast.',

'mainpagetext'      => 'Softver Wiki je uspješno instaliran.',
'mainpagedocfooter' => 'Pogledajte [http://meta.wikimedia.org/wiki/MediaWiki_localisation dokumentaciju o prilagodbi sučelja]
i [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide Vodič za suradnike] za pomoć pri uporabi i podešavanju.',

'about'          => 'O',
'article'        => 'Članak',
'newwindow'      => '(otvara se u novom prozoru)',
'cancel'         => 'Odustani',
'qbfind'         => 'Nađi',
'qbbrowse'       => 'Pregledaj',
'qbedit'         => 'Uredi',
'qbpageoptions'  => 'Postavke stranice',
'qbpageinfo'     => 'O stranici',
'qbmyoptions'    => 'Moje stranice',
'qbspecialpages' => 'Posebne stranice',
'moredotdotdot'  => 'Više...',
'mypage'         => 'Moja stranica',
'mytalk'         => 'Moj razgovor',
'anontalk'       => 'Razgovor za ovu IP adresu',
'navigation'     => 'Orijentacija',
'and'            => 'i',

# Metadata in edit box
'metadata_help' => 'Metapodaci:',

'errorpagetitle'    => 'Pogreška',
'returnto'          => 'Vrati se na $1.',
'tagline'           => 'Izvor: {{SITENAME}}',
'help'              => 'Pomoć',
'search'            => 'Traži',
'searchbutton'      => 'Traži',
'go'                => 'Kreni',
'searcharticle'     => 'Kreni',
'history'           => 'Stare izmjene',
'history_short'     => 'Stare izmjene',
'updatedmarker'     => 'obnovljeno od zadnjeg posjeta',
'info_short'        => 'Informacija',
'printableversion'  => 'Verzija za ispis',
'permalink'         => 'Trajna poveznica',
'print'             => 'Ispiši',
'edit'              => 'Uredi',
'create'            => 'Započni',
'editthispage'      => 'Uredi ovu stranicu',
'create-this-page'  => 'Započni ovu stranicu',
'delete'            => 'Izbriši',
'deletethispage'    => 'Izbriši ovu stranicu',
'undelete_short'    => 'Vrati {{PLURAL:$1|$1 uređivanje|$1 uređivanja}}',
'protect'           => 'Zaštiti',
'protect_change'    => 'promijeni',
'protectthispage'   => 'Zaštiti ovu stranicu',
'unprotect'         => 'Ukloni zaštitu',
'unprotectthispage' => 'Ukloni zaštitu s ove stranice',
'newpage'           => 'Nova stranica',
'talkpage'          => 'Razgovor o ovoj stranici',
'talkpagelinktext'  => 'Razgovor',
'specialpage'       => 'Posebna stranica',
'personaltools'     => 'Osobni alati',
'postcomment'       => 'Napiši komentar',
'articlepage'       => 'Vidi članak',
'talk'              => 'Razgovor',
'views'             => 'Pogledi',
'toolbox'           => 'Traka s alatima',
'userpage'          => 'Vidi suradnikovu stranicu',
'projectpage'       => 'Vidi stranicu o projektu',
'imagepage'         => 'Vidi stranicu slike',
'mediawikipage'     => 'Vidi stranicu za razgovor',
'templatepage'      => 'Vidi ovaj predložak',
'viewhelppage'      => 'Vidi stranicu pomoći',
'categorypage'      => 'Vidi stranicu s kategorijama',
'viewtalkpage'      => 'Vidi razgovor',
'otherlanguages'    => 'Drugi jezici',
'redirectedfrom'    => '(Preusmjereno s $1)',
'redirectpagesub'   => 'Preusmjeravanje',
'lastmodifiedat'    => 'Datum zadnje promjene na ovoj stranici: $2, $1', # $1 date, $2 time
'viewcount'         => 'Ova stranica je pogledana {{PLURAL:$1|$1 put|$1 puta}}.',
'protectedpage'     => 'Zaštićena stranica',
'jumpto'            => 'Skoči na:',
'jumptonavigation'  => 'orijentacija',
'jumptosearch'      => 'traži',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'O projektu {{SITENAME}}',
'aboutpage'            => 'Project:O_projektu_{{SITENAME}}',
'bugreports'           => 'Poruke o programskim pogreškama',
'bugreportspage'       => 'Project:Poruke_o_programskim_pogreškama',
'copyright'            => 'Sadržaji se koriste u skladu s $1.',
'copyrightpagename'    => 'Autorska prava na projektu {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Autorska prava',
'currentevents'        => 'Aktualno',
'currentevents-url'    => 'Project:Novosti',
'disclaimers'          => 'Odricanje od odgovornosti',
'disclaimerpage'       => 'Project:General_disclaimer',
'edithelp'             => 'Kako uređivati stranicu',
'edithelppage'         => 'Help:Kako_uređivati_stranicu',
'faq'                  => 'Najčešća pitanja',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Pomoć',
'mainpage'             => 'Glavna stranica',
'mainpage-description' => 'Glavna stranica',
'policy-url'           => 'Project:Pravila',
'portal'               => 'Portal zajednice',
'portal-url'           => 'Project:Portal zajednice',
'privacy'              => 'Zaštita privatnosti',
'privacypage'          => 'Project:Zaštita privatnosti',

'badaccess'        => 'Pogreška u ovlaštenjima',
'badaccess-group0' => 'Nije vam dopušteno izvršiti ovaj zahvat.',
'badaccess-group1' => 'Ovaj zahvat mogu izvršiti samo suradnici iz grupe $1.',
'badaccess-group2' => 'Ovaj zahvat mogu izvršiti samo suradnici iz jedne od grupa $1.',
'badaccess-groups' => 'Ovaj zahvat mogu izvršiti samo suradnici iz jedne od grupa $1.',

'versionrequired'     => 'Potrebna inačica $1 MediaWikija',
'versionrequiredtext' => 'Za korištenje ove stranice potrebna je inačica $1 MediaWiki softvera. Pogledaj [[Special:Version|inačice]]',

'ok'                      => 'U redu',
'retrievedfrom'           => 'Dobavljeno iz "$1"',
'youhavenewmessages'      => 'Imate $1 ($2).',
'newmessageslink'         => 'nove poruke',
'newmessagesdifflink'     => 'zadnja promjena na stranici za razgovor',
'youhavenewmessagesmulti' => 'Imate nove poruke na $1',
'editsection'             => 'uredi',
'editold'                 => 'uredi',
'viewsourceold'           => 'vidi izvor',
'editsectionhint'         => 'Uređivanje odlomka: $1',
'toc'                     => 'Sadržaj',
'showtoc'                 => 'prikaži',
'hidetoc'                 => 'sakrij',
'thisisdeleted'           => 'Vidi ili vrati $1?',
'viewdeleted'             => 'Vidi $1?',
'restorelink'             => '{{PLURAL:$1|$1 pobrisanu izmjenu|$1 pobrisane izmjene|$1 pobrisanih izmjena}}',
'feedlinks'               => 'Izvor:',
'feed-invalid'            => 'Tip izvora nije valjan.',
'feed-unavailable'        => 'RSS izvori nisu dostupni na {{SITENAME}}',
'site-rss-feed'           => '$1 RSS izvor',
'site-atom-feed'          => '$1 Atom izvor',
'page-rss-feed'           => '"$1" RSS izvor',
'page-atom-feed'          => '"$1" Atom izvor',
'red-link-title'          => '$1 (još nije napisano)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Članak',
'nstab-user'      => 'Stranica suradnika',
'nstab-media'     => 'Mediji',
'nstab-special'   => 'Posebno',
'nstab-project'   => 'Stranica o projektu',
'nstab-image'     => 'Slika',
'nstab-mediawiki' => 'Poruka',
'nstab-template'  => 'Predložak',
'nstab-help'      => 'Pomoć',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Nema takve naredbe',
'nosuchactiontext'  => 'Navedeni URL označava
nepostojeću naredbu',
'nosuchspecialpage' => 'Posebna stranica ne postoji',
'nospecialpagetext' => "<big>'''Takva posebna stranica ne postoji.'''</big>

Za popis svih posebnih stranica posjetite [[Special:SpecialPages|ovdje]].",

# General errors
'error'                => 'Pogreška',
'databaseerror'        => 'Pogreška baze podataka',
'dberrortext'          => 'Došlo je do sintaksne pogreške u upitu bazi.
Možda se radi o bugu u softveru.
Posljednji pokušaj upita je glasio:
<blockquote><tt>$1</tt></blockquote>
iz funkcije "<tt>$2</tt>".
MySQL je vratio pogrešku "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Došlo je do sintaksne pogreške s upitom bazi.
Možda se radi o bugu u softveru.
Posljednji pokušaj upita je glasio:
"$1"
iz funkcije "<tt>$2</tt>".
MySQL je vratio pogrešku "<tt>$3: $4</tt>".',
'noconnect'            => 'Oprostite! Wiki trenutačno ima tehničkih problema i ne može se povezati s bazom podataka.<br />
$1',
'nodb'                 => 'Nije bilo moguće odabrati bazu podataka $1',
'cachederror'          => 'Ova je verzija stranice iz međuspremnika i možda ne sadrži sve promjene.',
'laggedslavemode'      => 'Upozorenje: na stranici se možda ne nalaze najnovije promjene.',
'readonly'             => 'Baza podataka je zaključana',
'enterlockreason'      => 'Upiši razlog zaključavanja i procjenu vremena otključavanja',
'readonlytext'         => 'Baza podataka je trenutačno zaključana, nije ju moguće uređivati ili mijenjati. Ovo je obično pokazatelj tekućeg redovitog održavanja. Nakon što se potonja privremena akcija završi, baza podataka će se vratiti u uobičajeno stanje.

Administrator koji je izvršio zaključavanje naveo je ovaj razlog: $1',
'missing-article'      => 'U bazi podataka nije pronađen tekst stranice koji je trebao biti pronađen, nazvane "$1" $2.

Ovo se najčešće događa zbog poveznice na zastarjelu usporedbu ili staru promjenu stranice koja je u međuvremenu izbrisana.

Ako to nije slučaj, možda se radi o softverskoj grešci. Molimo da u tom slučaju pošaljete poruku [[Special:ListUsers/sysop|administratoru]] navodeći URL.',
'missingarticle-rev'   => '(izmjena#: $1)',
'missingarticle-diff'  => '(razlika: $1, $2)',
'readonly_lag'         => 'Baza podataka je automatski zaključana dok se sekundarni bazni poslužitelji ne usklade s glavnim',
'internalerror'        => 'Pogreška sustava',
'internalerror_info'   => 'Interna pogreška: $1',
'filecopyerror'        => 'Ne mogu kopirati datoteku "$1" u "$2".',
'filerenameerror'      => 'Ne mogu preimenovati datoteku "$1" u "$2".',
'filedeleteerror'      => 'Ne mogu obrisati datoteku "$1".',
'directorycreateerror' => 'Nije moguće kreirati direktorij "$1".',
'filenotfound'         => 'Datoteka "$1" nije nađena.',
'fileexistserror'      => 'Ne mogu stvoriti datoteku "$1": datoteka s tim imenom već postoji',
'unexpected'           => 'Neočekivana vrijednost: "$1"="$2".',
'formerror'            => 'Pogreška: Ne mogu poslati podatke',
'badarticleerror'      => 'Ovu radnju nije moguće izvesti s tom stranicom.',
'cannotdelete'         => 'Ne mogu obrisati navedenu stranicu ili sliku. (Moguće da je već obrisana.)',
'badtitle'             => 'Loš naslov',
'badtitletext'         => 'Navedeni naslov stranice nepravilan ili loše formirana interwiki poveznica.',
'perfdisabled'         => 'Ova mogućnost je privremeno isključena jer usporava bazu podataka do mjere koja svima onemogućava korištenje wikija.',
'perfcached'           => 'Sljedeći podaci su iz međuspremnika i možda nisu najsvježiji:',
'perfcachedts'         => 'Sljedeći podaci su iz međuspremnika i zadnji puta su ažurirani u $1.',
'querypage-no-updates' => 'Osvježavanje ove stranice je trenutačno onemogućeno. Nove promjene neće biti vidljive.',
'wrong_wfQuery_params' => 'Neispravni parametri poslani u wfQuery()<br />
Funkcija: $1<br />
Upit: $2',
'viewsource'           => 'Vidi izvornik',
'viewsourcefor'        => 'za $1',
'actionthrottled'      => 'Uređivanje je usporeno',
'actionthrottledtext'  => 'Kao anti-spam mjeru, ograničeni ste u broju ovih akcija u određenom vremenu, i trenutačno ste dosegli to ograničenje. Pokušajte opet za koju minutu.',
'protectedpagetext'    => 'Ova stranica je zaključana da bi se onemogućile izmjene.',
'viewsourcetext'       => 'Možete pogledati i kopirati izvorni sadržaj ove stranice:',
'protectedinterface'   => 'Ova stranica je zaštićena od izmjena jer sadrži tekst MediaWiki softvera.',
'editinginterface'     => "'''Upozorenje:''' Uređujete stranicu koja se rabi za prikaz teksta u sučelju softvera. Promjene učinjene na ovoj stranici će se odraziti na izgled korisničkog sučelja kod drugih suradnika. Za prijevod, razmotrite korištenje [http://translatewiki.net/wiki/Main_Page?setlang=hr Betawiki], projekta lokalizacije MedijeWiki.",
'sqlhidden'            => '(SQL upit sakriven)',
'cascadeprotected'     => 'Ova je stranica zaključana za uređivanja jer je uključena u {{PLURAL:$1|slijedeću stranicu|slijedeće stranice}}, koje su zaštićene "prenosivom zaštitom":
$2',
'namespaceprotected'   => "Ne možete uređivati stranice u imenskom prostoru '''$1'''.",
'customcssjsprotected' => 'Ne možete uređivati ovu stranicu zato što ona sadrži osobne postavke drugog suradnika.',
'ns-specialprotected'  => "Stranice u imenskom prostoru ''{{ns:special}}'' ne mogu se uređivati.",
'titleprotected'       => "Ovaj naslov je od kreiranja zaštitio suradnik [[User:$1|$1]], uz razlog: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Loša konfiguracija: nepoznati skener za viruse: <i>$1</i>',
'virus-scanfailed'     => 'skeniranje neuspješno (kod $1)',
'virus-unknownscanner' => 'nepoznati antivirus:',

# Login and logout pages
'logouttitle'                => 'Odjava suradnika',
'logouttext'                 => 'Odjavili ste se.<br />
Možete nastaviti s korištenjem projekta {{SITENAME}} neprijavljeni,
ili se možete ponovo prijaviti pod istim ili drugim imenom. Neke se stranice mogu
prikazivati kao da ste još uvijek prijavljeni, sve dok ne očistite međuspremnik svog preglednika.',
'welcomecreation'            => '== Dobrodošli, $1! ==
Vaš je suradnički račun otvoren. 

Ne zaboravite prilagoditi [[Special:Preferences|{{SITENAME}} postavke]].',
'loginpagetitle'             => 'Prijava suradnika',
'yourname'                   => 'Suradničko ime',
'yourpassword'               => 'Lozinka:',
'yourpasswordagain'          => 'Ponovno upišite lozinku',
'remembermypassword'         => 'Trajno zapamti moju lozinku.',
'yourdomainname'             => 'Vaša domena',
'externaldberror'            => 'Došlo je do pogreške s vanjskom autorizacijom ili vam nije dopušteno osvježavanje vanjskog suradničkog računa.',
'loginproblem'               => '<b>Došlo je do pogreške s vašom prijavom.</b><br />Pokušajte iznova!',
'login'                      => 'Prijavi se',
'nav-login-createaccount'    => 'Prijavi se',
'loginprompt'                => 'Za prijavu na sustav {{SITENAME}} morate u pregledniku uključiti kolačiće (cookies).',
'userlogin'                  => 'Prijavi se',
'logout'                     => 'Odjavi se',
'userlogout'                 => 'Odjavi se',
'notloggedin'                => 'Niste prijavljeni',
'nologin'                    => 'Nemate suradnički račun? $1.',
'nologinlink'                => 'Otvorite račun',
'createaccount'              => 'Otvori novi suradnički račun',
'gotaccount'                 => 'Već imate suradnički račun? $1.',
'gotaccountlink'             => 'Prijavite se',
'createaccountmail'          => 'poštom',
'badretype'                  => 'Unesene lozinke nisu istovjetne.',
'userexists'                 => 'Uneseno suradničko ime već je u upotrebi.
Unesite neko drugo ime.',
'youremail'                  => 'Vaša elektronska pošta *',
'username'                   => 'Suradničko ime:',
'uid'                        => 'Suradnički ID-broj:',
'prefs-memberingroups'       => 'Član {{PLURAL:$1|skupine|skupina}}:',
'yourrealname'               => 'Pravo ime (nije obvezno)*',
'yourlanguage'               => 'Jezik:',
'yourvariant'                => 'Inačica:',
'yournick'                   => 'Vaš nadimak (za potpisivanje)',
'badsig'                     => 'Kôd vašeg potpisa nije valjan; provjerite HTML tagove.',
'badsiglength'               => 'Suradnički potpis je predugačak. Mora imati manje od $1 {{PLURAL:$1|znaka|znakova}}.',
'email'                      => 'Adresa elektroničke pošte *',
'prefs-help-realname'        => '* Pravo ime (nije obvezno): za pravnu atribuciju vaših doprinosa.',
'loginerror'                 => 'Pogreška u prijavi',
'prefs-help-email'           => 'E-mail adresa nije obvezna: ali omogućuje slanje nove lozinke e-mailom u slučaju da zaboravite svoju.
Možete omogućiti drugima da vas kontaktiraju na suradničkoj stranici ili stranici za razgovor bez javnog otkrivanja vaše e-mail adrese.',
'prefs-help-email-required'  => 'Potrebno je navesti adresu e-pošte (e-mail).',
'nocookiesnew'               => "Suradnički račun je otvoren, ali niste uspješno prijavljeni. Naime, {{SITENAME}} koristi kolačiće (''cookies'') u procesu prijave. Isključili ste kolačiće. Molim uključite ih i pokušajte ponovo s vašim novim imenom i lozinkom.",
'nocookieslogin'             => "{{SITELOGIN}} koristi kolačiće (''cookies'') u procesu prijave. Isključili ste kolačiće. Molim uključite ih i pokušajte ponovo.",
'noname'                     => 'Niste unijeli valjano suradničko ime.',
'loginsuccesstitle'          => 'Prijava uspješna',
'loginsuccess'               => 'Prijavili ste se na wiki kao "$1".',
'nosuchuser'                 => 'Ne postoji suradnik s imenom "$1". Provjerite jeste li točno utipkali, ili [[Special:Userlogin/signup|otvorite novi suradnički račun]].',
'nosuchusershort'            => 'Ne postoji suradnik s imenom "<nowiki>$1</nowiki>". Provjerite vaš unos.',
'nouserspecified'            => 'Molimo navedite suradničko ime.',
'wrongpassword'              => 'Lozinka koju ste unijeli nije ispravna. Pokušajte ponovno.',
'wrongpasswordempty'         => 'Niste unijeli lozinku. Pokušajte ponovno.',
'passwordtooshort'           => 'Vaša je lozinka nevaljana ili prekratka. Lozinka mora sadržavati najmanje {{PLURAL:$1|1 znak|$1 znakova}} i mora biti različita od imena.',
'mailmypassword'             => 'Pošalji mi novu lozinku',
'passwordremindertitle'      => '{{SITENAME}}: nova lozinka.',
'passwordremindertext'       => 'Netko je (vjerojatno vi, s IP adrese $1)
zatražio da vam pošaljemo novu lozinku za projekt {{SITENAME}} ($4).
Privremena lozinka za suradnika "$2" je postavljena na "$3".
Molimo vas da se odmah prijavite i promijenite lozinku.

Ukoliko niste zatražili novu lozinku, ili ste se sjetili stare lozinke i
više ju ne želite promijeniti, slobodno zanemarite ovu poruku i nastavite
koristiti staru lozinku.',
'noemail'                    => 'Suradnik "$1" nema zapisanu e-mail adresu.',
'passwordsent'               => 'Nova je lozinka poslana na e-mail adresu suradnika "$1"',
'blocked-mailpassword'       => 'Vašoj IP adresi je blokirano uređivanje stranica, a da bi se spriječila nedopuštena akcija, mogućnost zahtijevanja nove lozinke je također onemogućena.',
'eauthentsent'               => 'Na navedenu adresu poslan je e-mail s potvrdom. Prije nego što pošaljemo daljnje poruke,
molimo vas da otvorite e-mail i slijedite u njemu sadržana uputstva.',
'throttled-mailpassword'     => 'Već Vam je poslan e-mail za promjenu lozinke, u {{PLURAL:$1|zadnjih sat vremena|zadnja $1 sata|zadnjih $1 sati}}.
Da bi spriječili zloupotrebu, moguće je poslati samo jedan e-mail za promjenu lozinke {{PLURAL:$1|svakih sat vremena|svaka $1 sata|svakih $1 sati}}.',
'mailerror'                  => 'Pogreška pri slanju e-maila: $1',
'acct_creation_throttle_hit' => 'Nažalost, ne možete otvoriti nove suradničke račune. Već ste otvorili $1.',
'emailauthenticated'         => 'Vaša e-mail adresa je ovjerena $1.',
'emailnotauthenticated'      => 'Vaša e-mail adresa još nije ovjerena.
Ne možemo poslati e-mail ni u jednoj od sljedećih naredbi.',
'noemailprefs'               => '<strong>Nije navedena e-mail adresa</strong>, stoga sljedeće naredbe neće raditi.',
'emailconfirmlink'           => 'Potvrdite svoju e-mail adresu',
'invalidemailaddress'        => 'Ne mogu prihvatiti e-mail adresu jer nije valjano oblikovana.
Molim unesite ispravno oblikovanu adresu ili ostavite polje praznim.',
'accountcreated'             => 'Suradnički račun otvoren',
'accountcreatedtext'         => 'Suradnički račun za $1 je otvoren.',
'createaccount-title'        => 'Otvaranje suradničkog računa za {{SITENAME}}',
'createaccount-text'         => 'Netko je stvorio suradnički račun s vašom adresom elektronske pošte na {{SITENAME}} ($4) nazvan "$2", s lozinkom "$3". Trebali biste se prijaviti i odmah promijeniti lozinku.

Možete zanemariti ovu poruku, ako je suradnički račun stvoren nenamjerno.',
'loginlanguagelabel'         => 'Jezik: $1',

# Password reset dialog
'resetpass'               => 'Postavi novu lozinku',
'resetpass_announce'      => 'Prijavljeni ste s privremenom lozinkom. Da završite proces mijenjanja lozinke, upišite ovdje novu lozinku:',
'resetpass_header'        => 'Resetiraj lozinku',
'resetpass_submit'        => 'Postavite lozinku i prijavite se',
'resetpass_success'       => 'Lozinka uspješno postavljena! Prijava u tijeku...',
'resetpass_bad_temporary' => 'Nevažeća privremena lozinka. Možda ste već uspješno promijenili svoju lozinku ili ste zatražili novu privremenu lozinku.',
'resetpass_forbidden'     => 'Na ovom wikiju ({{SITENAME}}) lozinka ne može biti promijenjena',
'resetpass_missing'       => 'Forma ne sadrži tražene podatke.',

# Edit page toolbar
'bold_sample'     => 'Podebljani tekst',
'bold_tip'        => 'Podebljani tekst',
'italic_sample'   => 'Kurzivni tekst',
'italic_tip'      => 'Kurzivni tekst',
'link_sample'     => 'Tekst poveznice',
'link_tip'        => 'Unutarnja poveznica',
'extlink_sample'  => 'http://www.example.com Tekst poveznice',
'extlink_tip'     => 'Vanjska poveznica (pazi, nužan je prefiks http://)',
'headline_sample' => 'Tekst naslova',
'headline_tip'    => 'Podnaslov',
'math_sample'     => 'Ovdje unesi formulu',
'math_tip'        => 'Matematička formula (LaTeX)',
'nowiki_sample'   => 'Ovdje unesite neoblikovani tekst',
'nowiki_tip'      => 'Neoblikovani tekst',
'image_sample'    => 'Primjer.jpg',
'image_tip'       => 'Uložena slika',
'media_sample'    => 'Primjer.ogg',
'media_tip'       => 'Uloženi medij',
'sig_tip'         => 'Vaš potpis s datumom',
'hr_tip'          => 'Vodoravna crta (koristiti rijetko)',

# Edit pages
'summary'                          => 'Sažetak',
'subject'                          => 'Predmet',
'minoredit'                        => 'Ovo je manja promjena',
'watchthis'                        => 'Prati ovaj članak',
'savearticle'                      => 'Sačuvaj stranicu',
'preview'                          => 'Pregled kako će stranica izgledati',
'showpreview'                      => 'Prikaži kako će izgledati',
'showlivepreview'                  => 'Pregled kako će izgledati, uživo',
'showdiff'                         => 'Prikaži promjene',
'anoneditwarning'                  => "'''Upozorenje:''' Niste prijavljeni pod suradničkim imenom. Vaša IP adresa bit će zabilježena u popisu izmjena ove stranice.",
'missingsummary'                   => "'''Napomena:''' Niste unijeli sažetak promjena. Ako ponovno kliknete na 'Sačuvaj', vaše će promjene biti snimljene bez sažetka.",
'missingcommenttext'               => 'Molim unesite sažetak.',
'missingcommentheader'             => "'''Upozorenje:''' Niste napisali sažetak ovog predmeta. Ako ponovno kliknete \"Sačuvaj stranicu\", vaš će predmet biti snimljen bez sažetka.",
'summary-preview'                  => 'Pregled sažetka',
'subject-preview'                  => 'Pregled predmeta',
'blockedtitle'                     => 'Suradnik je blokiran',
'blockedtext'                      => '<big>\'\'\'Vaše suradničko ime ili IP adresa je blokirana\'\'\'</big>

Blokirao vas je $1.
Iz sljedećeg razloga: \'\'$2\'\'.

* Početak bloka: $8
* Istek bloka: $6
* Ime blokiranog suradnika: $7

Ako želite raspraviti blokiranje javite se administratoru $1 ili nekom drugom [[{{MediaWiki:Grouppage-sysop}}|administratoru]].

Ne možete se koristiti naredbom "piši suradniku" ako niste upisali valjanu e-mail adresu u svojim [[Special:Preferences|postavkama]].

Vaša IP adresa je $3, oznaka bloka je #$5. Molimo vas da je spomenete u porukama o ovom predmetu.',
'autoblockedtext'                  => 'Vaša IP adresa automatski je blokirana zbog toga što ju je koristio drugi suradnik, kojeg je blokirao $1.
Razlog blokiranja je sljedeći:

:\'\'$2\'\'

* Početak blokade: $8
* Blokada istječe: $6
* Ime blokiranog suradnika: $7

Možete kontaktirati $1 ili jednog od [[{{MediaWiki:Grouppage-sysop}}|administratora]] kako bi vam pojasnili razlog blokiranja.

Primijetite da ne možete koristiti opciju "Pošalji mu e-mail" ukoliko niste upisali valjanu e-mail adresu u vašim [[Special:Preferences|suradničkim postavkama]] i ako niste u tome onemogućeni prilikom blokiranja.

Vaša trenutačna IP adresa je $3, a oznaka bloka #$5. Molimo navedite ovaj broj kod svakog upita vezano za razlog blokiranja.',
'blockednoreason'                  => 'bez obrazloženja',
'blockedoriginalsource'            => "Izvorni tekst članka '''$1''' prikazan je ispod:",
'blockededitsource'                => "Tekst '''vaše izmjene''' na članku '''$1''' prikazan je ispod:",
'whitelistedittitle'               => 'Za uređivanje stranice morate se prijaviti',
'whitelistedittext'                => 'Za uređivanje stranice morate se $1.',
'confirmedittitle'                 => 'Ovjera e-mail adrese nužna za uređivanje',
'confirmedittext'                  => 'Morate ovjeriti vašu e-mail adresu prije nego što vam bude omogućeno uređivanje. Molim unesite i ovjerite vašu e-mail adresu u [[Special:Preferences|suradničkim postavkama]].',
'nosuchsectiontitle'               => 'Odlomak ne postoji',
'nosuchsectiontext'                => 'Pokušali ste uređivati odlomak koji ne postoji (moguće je nedavno obrisan). Pošto odlomak $1 ne postoji, nije moguće snimiti vaše promjene.',
'loginreqtitle'                    => 'Nužna prijava',
'loginreqlink'                     => 'prijavite se',
'loginreqpagetext'                 => 'Morate se $1 da biste vidjeli ostale stranice.',
'accmailtitle'                     => 'Lozinka poslana.',
'accmailtext'                      => "Lozinka za suradnika '$1' poslana je na adresu $2.",
'newarticle'                       => '(Novo)',
'newarticletext'                   => 'Došli ste na stranicu koja još nema sadržaja.<br />
*Ako želite unijeti sadržaj, počnite tipkati u prozor ispod ovog teksta.
*Ako vam treba pomoć, idite na [[{{MediaWiki:Helppage}}|stranicu za pomoć]].
*Ako ste ovamo dospjeli slučajno, kliknite "Natrag" (Back) u svom programu.',
'anontalkpagetext'                 => "----''Ovo je stranica za razgovor s neprijavljenim suradnikom koji nije otvorio suradnički račun ili se njime ne koristi. Zbog toga se moramo služiti brojčanom IP adresom kako bismo ga identificirali. Takvu adresu često koristi više ljudi. Ako ste neprijavljeni suradnik i smatrate da su vam upućeni irelevantni komentari, molimo vas da [[Special:UserLogin|otvorite suradnički račun ili se prijavite]] te tako u budućnosti izbjegnete zamjenu s drugim neprijavljenim suradnicima.''",
'noarticletext'                    => 'Na ovoj stranici trenutačno nema sadržaja, možete [[Special:Search/{{PAGENAME}}|potražiti ovaj naslov]] u drugim stranicama ili [{{fullurl:{{FULLPAGENAME}}|action=edit}} urediti ovu stranicu].',
'userpage-userdoesnotexist'        => 'Suradničko ime "$1" nije prijavljeno. Jeste li sigurni da želite stvoriti/uređivati ovu stranicu?',
'clearyourcache'                   => "'''Napomena:''' Nakon snimanja trebate očistiti međuspremnik svog preglednika kako biste vidjeli promjene.
'''Mozilla / Firefox / Safari:''' držite ''Shift'' i pritisnite ''Reload'', ili pritisnite ''Ctrl-F5'' ili ''Ctrl-R'' (''Cmd-R'' na Apple Macu); '''Konqueror:''' samo pritisnite dugme ''Reload'' ili pritisnite ''F5''; '''Opera:''' očistiti cache u ''Tools → Preferences;'' '''Internet Explorer:''' držite ''Ctrl'' i pritisnite ''Refresh'', ili pritisnite ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Savjet:</strong> Koristite dugme 'Pokaži kako će izgledati' za testiranje svog CSS/JS prije snimanja.",
'usercsspreview'                   => "'''Ne zaboravite: samo isprobavate/pregledavate svoj suradnički CSS. Još nije snimljen!'''",
'userjspreview'                    => "'''Ne zaboravite: samo isprobavate/pregledavate svoj suradnički JavaScript, i da još nije snimljen!'''",
'userinvalidcssjstitle'            => "'''Upozorenje:''' Nema sučelja pod imenom \"\$1\". Ne zaboravite da imena stranica s .css and .js kodom počinju malim slovom, npr. {{ns:user}}:Mate/monobook.css, a ne {{ns:user}}:Mate/Monobook.css.",
'updated'                          => '(Ažurirano)',
'note'                             => '<strong>Napomena:</strong>',
'previewnote'                      => '<strong>Ne zaboravite da je ovo samo pregled kako će stranica izgledati i da stranica još nije snimljena!</strong>',
'previewconflict'                  => 'Ovaj pregled odražava stanje u gornjem polju za unos koje će biti sačuvano
ako pritisnete "Sačuvaj stranicu".',
'session_fail_preview'             => '<strong>Ispričavamo se! Nismo mogli obraditi vašu izmjenu zbog gubitka podataka o prijavi.
Molimo pokušajte ponovno. Ako i dalje ne bude radilo, pokušajte se odjaviti i ponovno prijaviti.</strong>',
'session_fail_preview_html'        => "<strong>Oprostite! Pretpregled nije moguć jer je ''session'' istekao.</strong>

''Budući da je na ovom wikiju ({{SITENAME}}) omogućen unos HTML oznaka (tagova), pretpregled je skriven kao mjera predstrožnosti protiv napada pomoću JavaScripta.''

<strong>Ukoliko ste pokušali vidjeti kako stranica izgleda, molimo probajte opet. Ako ne uspije, [[Special:UserLogout|odjavite se]] i prijavite se ponovo.</strong>",
'token_suffix_mismatch'            => '<strong>Vaše uređivanje je odbačeno jer je vaš web preglednik ubacio znak/znakove interpunkcije u token uređivanja.
Stoga je uređivanje odbačeno da se spriječi uništavanje stranice.
To se događa ponekad kad rabite neispravan web-baziran anonimni proxy.</strong>',
'editing'                          => 'Uređujete $1',
'editingsection'                   => 'Uređujete $1 (odlomak)',
'editingcomment'                   => 'Uređujete $1 (komentar)',
'editconflict'                     => 'Istovremeno uređivanje: $1',
'explainconflict'                  => 'Netko je u međuvremenu promijenio stranicu. Gornje polje sadrži sadašnji tekst stranice.
U donjem polju prikazane su vaše promjene. Morat ćete unijeti vaše promjene u sadašnji tekst. <b>Samo</b> će tekst
u u gornjem polju biti sačuvan kad pritisnete "Snimi stranicu".',
'yourtext'                         => 'Vaš tekst',
'storedversion'                    => 'Pohranjena inačica',
'nonunicodebrowser'                => '<strong>UPOZORENJE: Vaš preglednik ne podržava Unicode zapis znakova, molimo promijenite ga prije sljedećeg uređivanja članaka.</strong>',
'editingold'                       => '<strong>UPOZORENJE: Uređujete stariju inačicu
ove stranice. Ako je sačuvate, sve će promjene učinjene nakon ove inačice biti izgubljene.</strong>',
'yourdiff'                         => 'Razlike',
'copyrightwarning'                 => 'Molimo uočite da se svi doprinosi {{SITENAME}} smatraju objavljenima pod uvjetima $2 (vidi $1 za detalje). Ako ne želite da se vaše pisanje nemilosrdno uređuje i slobodno raspačava, nemojte ga ovamo slati. <br />
Također nam obećavate da ste ovo sami napisali, ili da ste to prepisali iz nečeg što je u javnom vlasništvu ili pod sličnom slobodnom licencijom.<br />
<strong>NE POSTAVLJAJTE RADOVE ZAŠTIĆENE AUTORSKIM PRAVIMA BEZ DOPUŠTENJA!</strong>',
'copyrightwarning2'                => 'Svi doprinosi {{SITENAME}} mogu biti mijenjani od strane svih suradnika. Ako ne želite da se vaše pisanje nemilosrdno uređuje, nemojte ga slati ovdje.<br /> Također nam obećavate da ste ovo sami napisali, ili da ste to prepisali iz nečeg što je u javnom vlasništvu ili pod sličnom slobodnom licencijom (vidi $1 za detalje). <strong>NE STAVLJAJTE ZAŠTIĆENE RADOVE BEZ DOPUŠTENJA!</strong>',
'longpagewarning'                  => 'PAŽNJA: Ova stranica je dugačka $1 kilobajta; neki bi preglednici mogli imati problema pri uređivanju stranica koje se približavaju ili su duže od 32 kb.
Molimo razmislite o rastavljanju stranice na manje odjeljke.',
'longpageerror'                    => '<strong>POGRJEŠKA: Tekst koji ste unijeli dug je $1 kilobajta, što je više od maksimalnih $2 kilobajta. Nije ga moguće snimiti.</strong>',
'readonlywarning'                  => '<strong>UPOZORENJE: Baza podataka je zaključana zbog održavanja, pa trenutačno ne možete sačuvati svoje
promjene. Najbolje je da kopirate i zaljepite tekst u tekstualnu datoteku te je snimite za kasnije.</strong>',
'protectedpagewarning'             => '<strong>UPOZORENJE: ova stranica je zaključana i mogu je uređivati samo suradnici s administratorskim pravima.</strong>',
'semiprotectedpagewarning'         => "'''Napomena:''' Ovu stranicu mogu uređivati samo prijavljeni suradnici.",
'cascadeprotectedwarning'          => "'''UPOZORENJE:''' Ova stranica je zaključana i mogu je uređivati samo suradnici s administratorskim pravima, jer je uključena u {{PLURAL:\$1|slijedeću stranicu|slijedeće stranice}} koje su zaštićene \"prenosivom\" zaštitom:",
'titleprotectedwarning'            => '<strong>UPOZORENJE:  Ova stranica je zaključana i samo je neki suradnici mogu stvoriti.</strong>',
'templatesused'                    => 'Predlošci korišteni na ovoj stranici:',
'templatesusedpreview'             => 'Predlošci koji se koriste u ovom predpregledu:',
'templatesusedsection'             => 'Predlošci koji se koriste u odjeljku:',
'template-protected'               => '(zaštićen)',
'template-semiprotected'           => '(djelomično zaštićen)',
'hiddencategories'                 => 'Ova stranica je član {{PLURAL:$1|1 skrivene kategorija|$1 skrivene kategorije|$1 skrivenih kategorija}}:',
'nocreatetitle'                    => 'Otvaranje novih stranica ograničeno',
'nocreatetext'                     => 'Na ovom je projektu ograničeno otvaranje novih stranica.
Možete se vratiti i uređivati već postojeće stranice ili se [[Special:UserLogin|prijaviti ili otvoriti suradnički račun]].',
'nocreate-loggedin'                => 'Nemate ovlasti za stvaranje novih stranica na {{SITENAME}}.',
'permissionserrors'                => 'Pogreška u pravima',
'permissionserrorstext'            => 'Nemate ovlasti za tu radnju iz sljedećih {{PLURAL:$1|razlog|razloga}}:',
'permissionserrorstext-withaction' => 'Nemate dopuštenje za $2, iz {{PLURAL:$1|razloga|razloga}}:',
'recreate-deleted-warn'            => "'''Upozorenje: Postavljate stranicu koja je prethodno brisana.'''

Razmotrite je li nastavljanje uređivanja ove stranice u skladu s pravilima.
Za vašu informaciju slijedi evidencija brisanja s obrazloženjem za prethodno brisanje:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Upozorenje: Ova stranica sadrži previše opterećujućih poziva parserskih funkcija

Trebala bi imati manje od $2 {{PLURAL:$2|poziva|poziva}}, sada ima {{PLURAL:$1|$1 poziv|$1 poziva}}.',
'expensive-parserfunction-category'       => 'Stranice s previše poziva opterećujućih parserskih funkcija',
'post-expand-template-inclusion-warning'  => 'Upozorenje: Veličina uključenih predložaka je prevelika.
Neki predlošci neće biti uključeni.',
'post-expand-template-inclusion-category' => 'Stranice gdje su uključeni predlošci preveliki',
'post-expand-template-argument-warning'   => 'Upozorenje: Ova stranica sadrži najmanje jedan argument predložaka koji ima preveliko proširenje. Ovi su argumenti izostavljeni.',
'post-expand-template-argument-category'  => 'Stranice koje sadrže izostavljene argumente za predloške',

# "Undo" feature
'undo-success' => 'Izmjena je uklonjena (tekst u okviru ispod ne sadrži zadnju izmjenu). Molim sačuvajte stranicu (provjerite sažetak).',
'undo-failure' => 'Ova izmjena ne može biti uklonjena zbog postojanja međuinačica.',
'undo-norev'   => 'Izmjena nije mogla biti uklonjena jer ne postoji ili je obrisana.',
'undo-summary' => 'Uklanjanje izmjene $1 što ju je unio/unijela [[Special:Contributions/$2|$2]] ([[User talk:$2|razgovor]])',

# Account creation failure
'cantcreateaccounttitle' => 'Nije moguće stvoriti suradnički račun',
'cantcreateaccount-text' => "Otvaranje suradničkog računa ove IP adrese ('''$1''') blokirao/la je [[User:$3|$3]].

Razlog koji je dao/la $3 je ''$2''",

# History pages
'viewpagelogs'        => 'Vidi evidencije za ovu stranicu',
'nohistory'           => 'Ova stranica nema starijih izmjena.',
'revnotfound'         => 'Stara izmjena nije nađena.',
'revnotfoundtext'     => 'Ne mogu pronaći staru izmjenu stranice koju ste zatražili.
Molimo provjerite URL koji vas je doveo ovamo.',
'currentrev'          => 'Trenutačna inačica',
'revisionasof'        => 'Inačica od $1',
'revision-info'       => 'Inačica od $1 koju je unio/unijela $2',
'previousrevision'    => '←Starija inačica',
'nextrevision'        => 'Novija inačica→',
'currentrevisionlink' => 'vidi trenutačnu inačicu',
'cur'                 => 'sad',
'next'                => 'sljed',
'last'                => 'pret',
'page_first'          => 'prva',
'page_last'           => 'zadnja',
'histlegend'          => 'Uputa: (sad) = razlika od trenutačne inačice,
(pret) = razlika od prethodne inačice, m = manja promjena',
'deletedrev'          => '[izbrisano]',
'histfirst'           => 'Najstarije',
'histlast'            => 'Najnovije',
'historysize'         => '({{PLURAL:$1|$1 bajt|$1 bajta|$1 bajtova}})',
'historyempty'        => '(prazna stranica)',

# Revision feed
'history-feed-title'          => 'Povijest promjena',
'history-feed-description'    => 'Povijest promjena ove stranice na wikiju',
'history-feed-item-nocomment' => '$1 u (test) $2', # user at time
'history-feed-empty'          => 'Tražena stranica ne postoji.
Stranica je vjerojatno prethodno izbrisana s wikija, ili preimenovana.
Pokušajte [[Special:Search|pretražiti]] važnije nove stranice na wikiju.',

# Revision deletion
'rev-deleted-comment'         => '(komentar uklonjen)',
'rev-deleted-user'            => '(suradničko ime uklonjeno)',
'rev-deleted-event'           => '(zapis uklonjen)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Ova je izmjena uklonjena iz javnoga arhiva.
Detalji se vjerojatno nalaze u [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} evidenciji brisanja].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Ova je izmjena uklonjena iz javnoga arhiva.
Kao administrator na ovom projektu možete ju vidjeti;
detalji se vjerojatno nalaze u [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} evidenciji brisanja].
</div>',
'rev-delundel'                => 'pokaži/skrij',
'revisiondelete'              => 'Izbriši/vrati izmjene',
'revdelete-nooldid-title'     => 'Nema tražene izmjene',
'revdelete-nooldid-text'      => 'Niste naveli željenu izmjenu (izmjene), željena izmjena ne postoji, ili  pokušavate sakriti trenutačnu izmjenu.',
'revdelete-selected'          => '{{PLURAL:$2|Odabrana izmjena|Odabrane izmjene|Odabrane izmjene}} stranice [[$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Odabrani zapis u evidenciji|Odabrani zapisi u evidenciji}}:',
'revdelete-text'              => 'Obrisane će se izmjene i dalje nalaziti u javnom popisu izmjena,
ali njihov sadržaj neće biti dostupan javnosti.

Drugi administratori ovoga projekta moći će i dalje pristupiti skrivenom sadržaju i
vratiti ga u javni pristup putem ovog sučelja, osim ako operateri na projektu nisu
postavili dodatna ograničenja.',
'revdelete-legend'            => 'Postavi ograničenja na izmjenu:',
'revdelete-hide-text'         => 'Sakrij tekst izmjene',
'revdelete-hide-name'         => 'Sakrij uređivanje i njegov predmet',
'revdelete-hide-comment'      => 'Sakrij komentar (sažetak)',
'revdelete-hide-user'         => 'Sakrij suradnikovo ime/IP adresu',
'revdelete-hide-restricted'   => 'Postavi ograničenja i za administratore kao i za ostale suradnike',
'revdelete-suppress'          => 'Sakrij podatke od administratora i ostalih suradnika',
'revdelete-hide-image'        => 'Sakrij sadržaj datoteke (sakrij sliku)',
'revdelete-unsuppress'        => 'Ukloni ograničenja na vraćenim izmjenama',
'revdelete-log'               => 'Komentar za evidenciju:',
'revdelete-submit'            => 'Izvrši brisanje/sakrivanje',
'revdelete-logentry'          => 'promijenjena vidljivost izmjene za [[$1]]',
'logdelete-logentry'          => 'promijenjena vidljivost uređivanja [[$1]]',
'revdelete-success'           => "'''Vidljivost promjene uspješno postavljena.'''",
'logdelete-success'           => "'''Vidljivost uređivanja uspješno postavljena.'''",
'revdel-restore'              => 'Promijeni dostupnost',
'pagehist'                    => 'Povijest stranice',
'deletedhist'                 => 'Obrisana povijest',
'revdelete-content'           => 'sadržaj',
'revdelete-summary'           => 'sažetak',
'revdelete-uname'             => 'suradničko ime',
'revdelete-restricted'        => 'primijenjeno ograničenje za administratore',
'revdelete-unrestricted'      => 'uklonjeno ograničenje za administratore',
'revdelete-hid'               => 'sakrij $1',
'revdelete-unhid'             => 'otkrij $1',
'revdelete-log-message'       => '$1 za $2 {{PLURAL:$2|izmjenu|izmjene}}',
'logdelete-log-message'       => '$1 za $2 {{PLURAL:$2|slučaj|slučaja}}',

# Suppression log
'suppressionlog'     => 'Evidencije sakrivanja',
'suppressionlogtext' => 'Slijedi popis brisanja i blokiranja koji uključuje sadržaj skriven za administratore.<br />
Vidi [[Special:IPBlockList|Popis blokiranih IP adresa]] za popis trenutačno aktivnih blokiranih adresa.',

# History merging
'mergehistory'                     => 'Spoji povijesti starih izmjena stranice',
'mergehistory-header'              => 'Na ovoj stranici spajate povijest jedne stranice u drugu (noviju) stranicu.
Budite sigurni da ta promjena čuva kontinuitet stranice.',
'mergehistory-box'                 => 'Spoji povijesti starih izmjena dvije stranice:',
'mergehistory-from'                => 'Izvorna stranica:',
'mergehistory-into'                => 'Ciljna stranica:',
'mergehistory-list'                => 'Spojiva povijest uređivanja',
'mergehistory-merge'               => 'Slijedeće promjene stranice [[:$1|$1]] mogu biti spojene u [[:$2|$2]].
Rabite kolonu s radio gumbima za spajanje samo određenih promjena.
Primijetite da uporaba navigacijskih poveznica resetira vaše izbore u koloni.',
'mergehistory-go'                  => 'Pokaži spojivu povijest uređivanja',
'mergehistory-submit'              => 'Spoji povijesti uređivanja stranica',
'mergehistory-empty'               => 'Nema spojivih promjena (spajanje nije moguće).',
'mergehistory-success'             => '$3 {{PLURAL:$3|izmjena|izmjene}} stranice [[:$1|$1]] uspješno spojene u povijest stranice [[:$2|$2]].',
'mergehistory-fail'                => 'Nemoguće spojiti povijest stranica, molimo provjerite stranice i vremenske parametre.',
'mergehistory-no-source'           => 'Izvorna stranica $1 ne postoji.',
'mergehistory-no-destination'      => 'Ciljna stranica $1 ne postoji.',
'mergehistory-invalid-source'      => 'Izvorna stranica mora imati valjani naziv.',
'mergehistory-invalid-destination' => 'Ciljna stranica mora imati valjani naziv.',
'mergehistory-autocomment'         => 'Stranica [[:$1]] je spojena u [[:$2]]',
'mergehistory-comment'             => 'Stranica [[:$1]] je spojena u [[:$2]]: $3',

# Merge log
'mergelog'           => 'Evidencija spajanja povijesti stranica',
'pagemerge-logentry' => 'spojeno [[$1]] u [[$2]] (promjene do $3)',
'revertmerge'        => 'Razdvoji',
'mergelogpagetext'   => 'Slijedi popis posljednjih spajanja povijesti stranica.',

# Diffs
'history-title'           => 'Povijest izmjena stranice "$1"',
'difference'              => '(Usporedba među inačicama)',
'lineno'                  => 'Redak $1:',
'compareselectedversions' => 'Usporedi odabrane inačice',
'editundo'                => 'ukloni ovu izmjenu',
'diff-multi'              => '({{PLURAL:$1|Nije prikazana jedna međuinačica|Nisu prikazane $1 međuinačice|Nije prikazano $1 međuinačica}})',

# Search results
'searchresults'             => 'Rezultati pretrage',
'searchresulttext'          => 'Za više obavijesti o pretraživanju projekta {{SITENAME}} vidi [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Tražili ste \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|sve stranice koje počinju sa "$1"]] | [[Special:WhatLinksHere/$1|sve stranice koje povezuju na "$1"]])',
'searchsubtitleinvalid'     => 'Za upit "$1"',
'noexactmatch'              => "'''Ne postoji stranica naziva \"\$1\".''' Možete [[:\$1|kreirati tu stranicu]].",
'noexactmatch-nocreate'     => "'''Nema stranice s imenom: \"\$1\".'''",
'toomanymatches'            => 'Preveliki broj rezultata, molimo probajte drukčiji upit',
'titlematches'              => 'Pronađene stranice prema naslovu',
'notitlematches'            => 'Nema pronađenih stranica prema naslovu',
'textmatches'               => 'Pronađene stranice prema tekstu članka',
'notextmatches'             => 'Nema pronađenih stranica prema tekstu članka',
'prevn'                     => 'prethodnih $1',
'nextn'                     => 'sljedećih $1',
'viewprevnext'              => 'Vidi ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 riječ|$2 riječi}})',
'search-result-score'       => 'Povezanost: $1%',
'search-redirect'           => '(preusmjeravanje $1)',
'search-section'            => '(odlomak $1)',
'search-suggest'            => 'Mislili ste: $1',
'search-interwiki-caption'  => 'Sestrinski projekti',
'search-interwiki-default'  => '$1 rezultati:',
'search-interwiki-more'     => '(više)',
'search-mwsuggest-enabled'  => 's prijedlozima',
'search-mwsuggest-disabled' => 'nema prijedloga',
'search-relatedarticle'     => 'Povezano',
'mwsuggest-disable'         => 'Isključi AJAX prijedloge',
'searchrelated'             => 'povezano',
'searchall'                 => 'sve',
'showingresults'            => "Dolje {{PLURAL:$1|je prikazan '''$1''' rezultat|su prikazana '''$1''' rezultata|je prikazano '''$1''' rezultata}}, počevši od '''$2'''.",
'showingresultsnum'         => "Dolje {{PLURAL:$3|je prikazan '''$3''' rezultat|su prikazana '''$3''' rezultata|je prikazano '''$3''' rezultata}}, počevši s brojem '''$2'''.",
'showingresultstotal'       => "Rezultati pretraživanja {{PLURAL:$3| '''$1''' od '''$3'''| '''$1 - $2''' od '''$3'''}}",
'nonefound'                 => '<b>Napomena</b>: pretrage su neuspješne ako tražite česte riječi koje ne indeksiramo, ili u upitu navedete previše pojmova (u rezultatu se pojavlju samo stranice koje sadrže sve tražene pojmove).',
'powersearch'               => 'Traženje',
'powersearch-legend'        => 'Napredno pretraživanje',
'powersearch-ns'            => 'Traži u imenskom prostoru:',
'powersearch-redir'         => 'Prikaži preusmjerenja',
'powersearch-field'         => 'Traži za',
'search-external'           => 'Vanjski pretraživač',
'searchdisabled'            => '<p>Oprostite! Pretraga po cjelokupnoj bazi je zbog bržeg rada projekta {{SITENAME}} trenutačno onemogućena. Možete se poslužiti tražilicom Google.</p>',

# Preferences page
'preferences'              => 'Postavke',
'mypreferences'            => 'Moje postavke',
'prefs-edits'              => 'Broj vaših uređivanja:',
'prefsnologin'             => 'Niste prijavljeni',
'prefsnologintext'         => 'Morate biti <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} prijavljeni]</span> za podešavanje suradničkih postavki.',
'prefsreset'               => 'Postavke su vraćene na prvotne vrijednosti.',
'qbsettings'               => 'Traka',
'qbsettings-none'          => 'Bez',
'qbsettings-fixedleft'     => 'Lijevo nepomično',
'qbsettings-fixedright'    => 'Desno nepomično',
'qbsettings-floatingleft'  => 'Lijevo leteće',
'qbsettings-floatingright' => 'Desno leteće',
'changepassword'           => 'Promjena lozinke',
'skin'                     => 'Izgled',
'math'                     => 'Prikaz matematičkih formula',
'dateformat'               => 'Format datuma',
'datedefault'              => 'Nemoj postaviti',
'datetime'                 => 'Datum i vrijeme',
'math_failure'             => 'Obrada nije uspjela.',
'math_unknown_error'       => 'nepoznata pogreška',
'math_unknown_function'    => 'nepoznata funkcija',
'math_lexing_error'        => 'rječnička pogreška (lexing error)',
'math_syntax_error'        => 'sintaksna pogreška',
'math_image_error'         => 'Konverzija u PNG nije uspjela; provjerite jesu li dobro instalirani latex, dvips, gs, i convert',
'math_bad_tmpdir'          => 'Ne mogu otvoriti ili pisati u privremeni direktorij za matematiku',
'math_bad_output'          => 'Ne mogu otvoriti ili pisati u odredišni direktorij za matematiku',
'math_notexvc'             => 'Nedostaje izvršna datoteka texvc-a; pogledajte math/README za postavke.',
'prefs-personal'           => 'Podaci o suradniku',
'prefs-rc'                 => 'Nedavne promjene i kratki članci',
'prefs-watchlist'          => 'Praćene stranice',
'prefs-watchlist-days'     => 'Broj dana koji će se prikazati na popisu praćenja:',
'prefs-watchlist-edits'    => 'Broj uređivanja koji će se prikazati na proširenom popisu praćenja:',
'prefs-misc'               => 'Razno',
'saveprefs'                => 'Spremi',
'resetprefs'               => 'Vrati na prvotne postavke',
'oldpassword'              => 'Stara lozinka',
'newpassword'              => 'Nova lozinka',
'retypenew'                => 'Ponovno unesite lozinku',
'textboxsize'              => 'Širina okvira za uređivanje',
'rows'                     => 'Redova',
'columns'                  => 'Stupaca',
'searchresultshead'        => 'Prikaz rezultata pretrage',
'resultsperpage'           => 'Koliko pogodaka na jednoj stranici',
'contextlines'             => 'Koliko redova teksta po pogotku',
'contextchars'             => 'Koliko znakova po retku',
'stub-threshold'           => 'Prag za formatiranje poput <a href="#" class="stub">poveznice mrve</a>:',
'recentchangesdays'        => 'Broj dana prikazanih u nedavnim promjenama:',
'recentchangescount'       => 'Broj naslova u nedavnim izmjenama',
'savedprefs'               => 'Vaše postavke su sačuvane.',
'timezonelegend'           => 'Vremenska zona',
'timezonetext'             => 'Unesite razliku između vašeg lokalnog vremena i vremena na poslužitelju (UTC).',
'localtime'                => 'Lokalno vrijeme',
'timezoneoffset'           => 'Razlika',
'servertime'               => 'Vrijeme na poslužitelju',
'guesstimezone'            => 'Vrijeme dobiveno od preglednika',
'allowemail'               => 'Omogući primanje e-maila od drugih suradnika',
'prefs-searchoptions'      => 'Način traženja',
'prefs-namespaces'         => 'Imenski prostori',
'defaultns'                => 'Ako ne navedem drugačije, traži u ovim prostorima:',
'default'                  => 'prvotno',
'files'                    => 'Datoteke',

# User rights
'userrights'                  => 'Upravljanje suradničkim pravima', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Upravljaj skupinama suradnika',
'userrights-user-editname'    => 'Unesite suradničko ime:',
'editusergroup'               => 'Uredi suradničke skupine',
'editinguser'                 => "Promjena suradničkog statusa za suradnika '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Uredi skupine suradnika',
'saveusergroups'              => 'Snimi skupine suradnika',
'userrights-groupsmember'     => 'Član:',
'userrights-groups-help'      => 'Možete promijeniti skupine za ovog suradnika.
* Označena kućica pokazuje da suradnik pripada skupini.
* Neoznačena kućica pokazuje da suradnik ne pripada skupini.
* Zvjezdica * označava da ne možete ukloniti skupinu kad ju jednom dodate, ili obratno.',
'userrights-reason'           => 'Razlog za promjenu:',
'userrights-no-interwiki'     => 'Nemate prava da uređujete prava suradnika na drugim wikijima.',
'userrights-nodatabase'       => 'Baza podataka $1 ne postoji ili nije lokalno dostupna.',
'userrights-nologin'          => 'Trebate se [[Special:UserLogin|prijaviti]] s administratorskim računom da bi mogli dodijeliti suradnička prava.',
'userrights-notallowed'       => 'Vaš trenutačni suradnički račun nema ovlasti mijenjanja suradničkih prava.',
'userrights-changeable-col'   => 'Skupine koje možete promijeniti',
'userrights-unchangeable-col' => 'Skupine koje ne možete promijeniti',

# Groups
'group'               => 'Skupina:',
'group-user'          => 'Suradnici',
'group-autoconfirmed' => 'Automatski potvrđeni suradnici',
'group-bot'           => 'Botovi',
'group-sysop'         => 'Administratori',
'group-bureaucrat'    => 'Birokrati',
'group-suppress'      => 'Nadzornici',
'group-all'           => '(svi)',

'group-user-member'          => 'Suradnik',
'group-autoconfirmed-member' => 'Automatski potvrđen suradnik',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Birokrat',
'group-suppress-member'      => 'Nadzornik',

'grouppage-user'          => '{{ns:project}}:Suradnici',
'grouppage-autoconfirmed' => '{{ns:project}}:Automatski potvrđeni suradnici',
'grouppage-bot'           => '{{ns:project}}:Botovi',
'grouppage-sysop'         => '{{ns:project}}:Administratori',
'grouppage-bureaucrat'    => '{{ns:project}}:Birokrati',
'grouppage-suppress'      => '{{ns:project}}:Nadzor',

# Rights
'right-read'                 => 'Čitanje stranica',
'right-edit'                 => 'Uređivanje stranica',
'right-createpage'           => 'Stvaranje stranica (stranica koje nisu razgovor)',
'right-createtalk'           => 'Stvaranje stranica za razgovor',
'right-createaccount'        => 'Stvaranje novog suradničkog računa',
'right-minoredit'            => 'Obilježiti izmjenu kao manju',
'right-move'                 => 'Premještanje stranica',
'right-move-subpages'        => 'Premještanje stranica s njihovim podstranicama',
'right-suppressredirect'     => 'Ne radi preusmjeravanje od starog imena prilikom premještanja stranice',
'right-upload'               => 'Postavljanje datoteka',
'right-reupload'             => 'Postavljanje nove inačice datoteke',
'right-reupload-own'         => 'Postavljanje nove inačice vlastite datoteke',
'right-reupload-shared'      => 'Lokalno postavljanje novih inačica datoteka na zajedničkom poslužitelju',
'right-upload_by_url'        => 'Postavljanje datoteke s URL adrese',
'right-purge'                => 'Čišćenje priručne memorije stranice bez stranice za potvrdu',
'right-autoconfirmed'        => 'Uređivanje stranica zaštićenih za neprijavljene suradnike',
'right-bot'                  => 'Izmjene su tretirane kao automatski proces (bot)',
'right-nominornewtalk'       => 'Bez manjih izmjena na novim stranicama za razgovor',
'right-apihighlimits'        => 'Korištenje viših granica kod API upita',
'right-writeapi'             => 'Mogućnost pisanja API',
'right-delete'               => 'Brisanje stranica',
'right-bigdelete'            => 'Brisanje stranica koje imaju veliku povijest',
'right-deleterevision'       => 'Brisanje i vraćanje određene izmjene na stranici',
'right-deletedhistory'       => 'Gledanje povijesti izmjena izbrisane stranice',
'right-browsearchive'        => 'Traženje obrisanih stranica',
'right-undelete'             => 'Vraćanje stranica',
'right-suppressrevision'     => 'Pregledavanje i vraćanje izmjena skrivenih od administratora',
'right-suppressionlog'       => 'Gledanje privatnih evidencija',
'right-block'                => 'Blokiranje suradnika u uređivanju',
'right-blockemail'           => 'Blokiranje suradnika u slanju elektroničke pošte',
'right-hideuser'             => 'Blokiranje suradničkog imena, skrivajući ga od javnosti',
'right-ipblock-exempt'       => 'Imunitet na IP blokiranje, auto-blok i blokiranje opsega',
'right-proxyunbannable'      => 'Imunitet na automatska blokiranja proxya',
'right-protect'              => 'Mijenjanje razina zaštićivanja i uređivanje zaštićenih stranica',
'right-editprotected'        => 'Uređivanje zaštićenih stranica (s prenosivom zaštitom)',
'right-editinterface'        => 'Uređivanje suradničkog sučelja',
'right-editusercssjs'        => 'Uređivanje CSS i JS stranica drugih suradnika',
'right-rollback'             => 'Brzo uklanjanje izmjena zadnjeg suradnika na određenoj stranici',
'right-markbotedits'         => 'Označavanje uklonjenih izmjena kao izmjenu bota',
'right-noratelimit'          => 'Bez vremenskog ograničenja uređivanja',
'right-import'               => 'Uvoženje stranica s drugih wikija',
'right-importupload'         => 'Uvoženje stranica kao datoteke',
'right-patrol'               => 'Označavanje izmjena pregledanim',
'right-autopatrol'           => 'Izmjene su automatski označene kao pregledane',
'right-patrolmarks'          => 'Vidljive oznake pregledavanja u nedavnim promjenama',
'right-unwatchedpages'       => 'Vidljiv popis negledanih stranica',
'right-trackback'            => 'Podnijeti trackback',
'right-mergehistory'         => 'Spajanje povijesti stranica',
'right-userrights'           => 'Uređivanje svih suradničkih prava',
'right-userrights-interwiki' => 'Uređivanje suradničkih prava na drugim Wikijima',
'right-siteadmin'            => 'Zaključavanje i otključavanje baze podataka',

# User rights log
'rightslog'      => 'Evidencija suradničkih prava',
'rightslogtext'  => 'Ovo je evidencija promjena suradničkih prava.',
'rightslogentry' => 'promijenjena suradnička prava za $1 iz $2 u $3',
'rightsnone'     => '(suradnik)',

# Recent changes
'nchanges'                          => '{{PLURAL:$1|$1 promjena|$1 promjene|$1 promjena}}',
'recentchanges'                     => 'Nedavne promjene',
'recentchangestext'                 => 'Na ovoj stranici možete pratiti nedavne promjene u wikiju.',
'recentchanges-feed-description'    => 'Na ovoj stranici možete pratiti nedavne promjene u wikiju.',
'rcnote'                            => "{{PLURAL:$1|Slijedi zadnja '''$1''' promjena|Slijede zadnje '''$1''' promjene|Slijedi zadnjih '''$1''' promjena}} u {{PLURAL:$2|zadnjem '''$2''' danu|zadnja '''$2''' dana|zadnjih '''$2''' dana}}, od $5, $4.",
'rcnotefrom'                        => 'Slijede promjene od <b>$2</b> (prikazano ih je do <b>$1</b>).',
'rclistfrom'                        => 'Prikaži nove promjene počevši od $1',
'rcshowhideminor'                   => '$1 manje promjene',
'rcshowhidebots'                    => '$1 botove',
'rcshowhideliu'                     => '$1 prijavljene suradnike',
'rcshowhideanons'                   => '$1 neprijavljene suradnike',
'rcshowhidepatr'                    => '$1 provjerene promjene',
'rcshowhidemine'                    => '$1 moje promjene',
'rclinks'                           => 'Prikaži zadnjih $1 promjena u zadnjih $2 dana; $3',
'diff'                              => 'razl',
'hist'                              => 'pov',
'hide'                              => 'sakrij',
'show'                              => 'prikaži',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|suradnik|suradnika|suradnika}} prati ovu stranicu]',
'rc_categories'                     => 'Ograniči na kategorije (odvojene znakom  "|")',
'rc_categories_any'                 => 'Sve',
'newsectionsummary'                 => '/* $1 */ Novi odlomak',

# Recent changes linked
'recentchangeslinked'          => 'Povezane stranice',
'recentchangeslinked-title'    => 'Povezane promjene sa "$1"',
'recentchangeslinked-noresult' => 'Nema promjena na povezanim stranicama u zadanom periodu.',
'recentchangeslinked-summary'  => "Ova posebna stranica pokazuje nedavne promjene na povezanim stranicama (ili stranicama određene kategorije). Stranice koje su na [[Special:Watchlist|vašem popisu praćenja]] su '''podebljane'''.",
'recentchangeslinked-page'     => 'Naslov stranice:',
'recentchangeslinked-to'       => 'Pokaži promjene na stranicama s poveznicom na ovu stranicu',

# Upload
'upload'                      => 'Postavi datoteku',
'uploadbtn'                   => 'Postavi datoteku',
'reupload'                    => 'Ponovno postavi',
'reuploaddesc'                => 'Vratite se u obrazac za postavljanje.',
'uploadnologin'               => 'Niste prijavljeni',
'uploadnologintext'           => 'Za postavljanje datoteka morate biti  [[Special:UserLogin|prijavljeni]].',
'upload_directory_missing'    => 'Mapa za datoteke ($1) nedostaje i webserver ju ne može napraviti.',
'upload_directory_read_only'  => 'Server ne može pisati u direktorij za postavljanje ($1).',
'uploaderror'                 => 'Pogreška kod postavljanja',
'uploadtext'                  => "Ovaj obrazac služi za postavljanje slika. 
Za pregledavanje i pretraživanje već postavljenih slika vidi [[Special:ImageList|popis postavljenih datoteka]]. Postavljanja i brisanja bilježe se i u [[Special:Log|evidenciji]].

Da biste na stranicu stavili sliku, koristite poveznice tipa
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Datoteka.jpg]]</nowiki></tt>''' za punu verziju datoteke
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Datoteka.png|200px|thumb|left|tekst]]</nowiki></tt>''' za datoteku širine 200 px u okviru s popratnim tekstom
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Datoteka.ogg]]</nowiki></tt>''' za direktno povezivanje na datoteku bez njenog pokazivanja",
'upload-permitted'            => 'Dopušteni tipovi datoteka: $1.',
'upload-preferred'            => 'Poželjni tipovi datoteka: $1.',
'upload-prohibited'           => 'Zabranjeni tipovi datoteka: $1.',
'uploadlog'                   => 'evidencija postavljanja',
'uploadlogpage'               => 'Evidencija_postavljanja',
'uploadlogpagetext'           => 'Dolje je popis nedavno postavljenih slika.',
'filename'                    => 'Ime datoteke',
'filedesc'                    => 'Opis',
'fileuploadsummary'           => 'Opis:',
'filestatus'                  => 'Status autorskih prava:',
'filesource'                  => 'Izvor:',
'uploadedfiles'               => 'Postavljene datoteke',
'ignorewarning'               => 'Zanemari upozorenja i snimi datoteku.',
'ignorewarnings'              => 'Zanemari sva upozorenja',
'minlength1'                  => 'Ime datoteke mora imati barem jedno slovo.',
'illegalfilename'             => 'Ime datoteke "$1" sadrži znakove koji nisu dopušteni u imenima stranica. Preimenujte datoteku i ponovno je postavite.',
'badfilename'                 => 'Ime slike automatski je promijenjeno u "$1".',
'filetype-badmime'            => 'Datoteke MIME tipa "$1" ne mogu se snimati.',
'filetype-unwanted-type'      => "'''\".\$1\"''' je neželjena vrsta datoteke. {{PLURAL:\$3|Preporučena vrsta je|Preporučene vrste su}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' je nedopuštena vrsta datoteke. {{PLURAL:\$3|Dopuštena vrsta je|Dopuštene vrste su}} \$2.",
'filetype-missing'            => 'Datoteka nema nastavak koji određuje tip (poput ".jpg").',
'large-file'                  => 'Preporučljivo je da datoteke ne prelaze $1; Ova datoteka je $2.',
'largefileserver'             => 'Veličina ove datoteke veća je od one dopuštene postavkama poslužitelja.',
'emptyfile'                   => 'Datoteka koju ste postavili je prazna. Možda se radi o krivo utipkanom imenu datoteke. Provjerite želite li zaista postaviti ovu datoteku.',
'fileexists'                  => 'Datoteka s ovim imenom već postoji, pogledajte <strong><tt>$1</tt></strong> ako niste sigurni želite li je uistinu promijeniti.',
'filepageexists'              => 'Stranica s opisom za ovu datoteku je već napravljena na <strong><tt>$1</tt></strong>, ali trenutačno ne postoji datoteka s ovim imenom. Sažetak koji unesete se neće pojaviti na stranici s opisom. Kako bi se sažetak pojavio, morate ručno urediti stranicu.',
'fileexists-extension'        => 'Već postoji datoteka sa sličnim imenom:<br />
Ime datoteke koju postavljate: <strong><tt>$1</tt></strong><br />
Ime postojeće datoteke: <strong><tt>$2</tt></strong><br />
Molimo da izaberete drugo ime.',
'fileexists-thumb'            => "<center>'''Postojeća slika'''</center>",
'fileexists-thumbnail-yes'    => 'Datoteka je najvjerojatnije slika u smanjenoj veličini <i>(thumbnail)</i>. Molimo provjerite datoteku <strong><tt>$1</tt></strong>.<br />
Ukoliko je ta datoteka ista kao i ova koju ste upravo pokušali snimiti, samo u višoj rezoluciji, nije nužno snimanje smanjenje slike<br />
<i>(thumbnaila)</i>, prikazivanje smanjene slike iz izvornika radi se softverski.',
'file-thumbnail-no'           => 'Ime datoteke počinje s <strong><tt>$1</tt></strong>.
Čini se da je to slika smanjene veličine <i>(minijatura)</i>.
Ukoliko imate ovu sliku u punoj razlučljivosti (rezoluciji) postavite tu sliku, u protivnom, molimo promijenite ime datoteke.',
'fileexists-forbidden'        => 'Datoteka s ovim imenom već postoji; molim postavite ju pod drugim imenom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Datoteka s ovim imenom već postoji u središnjem poslužitelju datoteka. 
Ako još uvijek želite postaviti svoju datoteku, idite nazad i postavite ju pod drugim imenom. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Ova datoteka je duplikat {{PLURAL:$1|sljedeće datoteke|sljedećih datoteka}}:',
'successfulupload'            => 'Postavljanje uspješno.',
'uploadwarning'               => 'Upozorenje kod postavljanja',
'savefile'                    => 'Sačuvaj datoteku',
'uploadedimage'               => 'postavljeno "$1"',
'overwroteimage'              => 'postavljena nova inačica od "[[$1]]"',
'uploaddisabled'              => 'Postavljanje je onemogućeno',
'uploaddisabledtext'          => 'Postavljanje datoteka na ovom je wikiju onemogućeno.',
'uploadscripted'              => 'Ova datoteka sadrži HTML ili skriptu, što može dovesti do grešaka u web pregledniku.',
'uploadcorrupt'               => 'Ova je datoteka oštećena ili ima nepravilan nastavak. Provjerite i pokušajte ponovo.',
'uploadvirus'                 => 'Datoteka sadrži virus! Podrobnije: $1',
'sourcefilename'              => 'Ime datoteke na vašem računalu:',
'destfilename'                => 'Ime datoteke na wikiju:',
'upload-maxfilesize'          => 'Maksimalna veličina datoteke: $1',
'watchthisupload'             => 'Prati ovu stranicu',
'filewasdeleted'              => 'Datoteka istog imena već je bila postavljena, a kasnije i obrisana. Trebali bi provjeriti $1 prije nego što ponovno postavite datoteku.',
'upload-wasdeleted'           => "'''Upozorenje: Pokušavate postaviti datoteku koja je prethodno obrisana.'''

Razmislite je li prigodno nastaviti s postavljanjem ove datoteke.
Slijedi evidencija brisanja ove datoteke s obrazloženjem prethodnog brisanja:",
'filename-bad-prefix'         => 'Ime datoteke koju snimate počinje s <strong>"$1"</strong>, što je ime koje slikama tipično dodjeljuju digitalni fotoaparati. Molimo izaberite bolje ime (neko koje bolje opisuje sliku nego $1).',

'upload-proto-error'      => 'Protokol nije valjan',
'upload-proto-error-text' => 'Udaljeno snimanje zahtijeva URL-ove koji počinju sa <code>http://</code> ili <code>ftp://</code>.',
'upload-file-error'       => 'Interna pogreška',
'upload-file-error-text'  => 'Interna pogreška se dogodila pri pokušaju stvaranja privremene datoteke na poslužitelju. Molimo javite se [[Special:ListUsers/sysop|administratoru]].',
'upload-misc-error'       => 'Nepoznata pogreška pri snimanju',
'upload-misc-error-text'  => 'Dogodila se nepoznata pogreška pri snimanju.
Provjerite valjanost i dostupnost URL-a i pokušajte opet. 
Ukoliko se problem ponovi, javite to [[Special:ListUsers/sysop|administratoru]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL nije dostupan',
'upload-curl-error6-text'  => 'Dani URL nije dostupan. Provjerite da li je URL ispravno upisan i da li su web stranice dostupne.',
'upload-curl-error28'      => "Istek vremena snimanja (''timeout'')",
'upload-curl-error28-text' => 'Poslužitelj ne odgovara na upit. Provjerite da li su web stranice dostupne, pričekajte i pokušajte ponovo. Možete pokušati kasnije, kad bude manja gužva.',

'license'            => 'Licencija:',
'nolicense'          => 'Molim odaberite:',
'license-nopreview'  => '(Prikaz nije moguć)',
'upload_source_url'  => ' (valjani, javno dostupni URL)',
'upload_source_file' => ' (datoteka na vašem računalu)',

# Special:ImageList
'imagelist-summary'     => 'Ova posebna stranica pokazuje sve postavljene datoteke.
Na vrhu popisa se nalaze najnovije postavljene datoteke.
Poredak datoteka mijenja se pritiskom na naslov stupca.',
'imagelist_search_for'  => 'Traži ime slike:',
'imgfile'               => 'datoteka',
'imagelist'             => 'Popis slika',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Naziv slike',
'imagelist_user'        => 'Suradnik',
'imagelist_size'        => 'Veličina (u bajtovima)',
'imagelist_description' => 'Opis',

# Image description page
'filehist'                       => 'Povijest datoteke',
'filehist-help'                  => 'Kliknite na datum/vrijeme kako biste vidjeli datoteku kakva je tada bila.',
'filehist-deleteall'             => 'izbriši sve',
'filehist-deleteone'             => 'izbriši',
'filehist-revert'                => 'vrati',
'filehist-current'               => 'sadašnja',
'filehist-datetime'              => 'Datum/Vrijeme',
'filehist-user'                  => 'Suradnik',
'filehist-dimensions'            => 'Dimenzije',
'filehist-filesize'              => 'Veličina datoteke',
'filehist-comment'               => 'Komentar',
'imagelinks'                     => 'Poveznice slike',
'linkstoimage'                   => '{{PLURAL:$1|Sljedeća stranica povezuje|$1 Sljedeće stranice povezuju}} na ovu datoteku:',
'nolinkstoimage'                 => 'Nijedna stranica ne povezuje na ovu sliku.',
'morelinkstoimage'               => 'Pogledaj [[Special:WhatLinksHere/$1|više poveznica]] za ovu datoteku.',
'redirectstofile'                => '{{PLURAL:$1|Sljedeća datoteka preusmjerava|$1 Sljedeće datoteke preusmjeravaju}} na ovu datoteku:',
'duplicatesoffile'               => '{{PLURAL:$1|Sljedeća datoteka je kopija|$1 Sljedeće datoteke su kopije}} ove datoteke:',
'sharedupload'                   => 'Ova je datoteka postavljena na zajedničkom poslužitelju i mogu je koristiti ostali wikiji',
'shareduploadwiki'               => 'Za podrobnije informacije vidi $1.',
'shareduploadwiki-desc'          => 'Opis datoteke $1 na zajedničkom poslužitelju je prikazan ispod',
'shareduploadwiki-linktext'      => 'stranica s opisom datoteke',
'shareduploadduplicate'          => 'Ova datoteka je kopija od $1 sa zajedničkog poslužitelja.',
'shareduploadduplicate-linktext' => 'druga datoteka',
'shareduploadconflict'           => 'Ova datoteka ima isto ime kao i $1 sa zajedničkog poslužitelja.',
'shareduploadconflict-linktext'  => 'druga datoteka',
'noimage'                        => 'Ne postoji datoteka s ovim imenom. Možete ju $1.',
'noimage-linktext'               => 'postaviti',
'uploadnewversion-linktext'      => 'Postavi novu inačicu datoteke',
'imagepage-searchdupe'           => 'Traži kopiju datoteke',

# File reversion
'filerevert'                => 'Ukloni ← $1',
'filerevert-legend'         => 'Vrati datoteku',
'filerevert-intro'          => "Vraćate '''[[Media:$1|$1]]''' na [$4 promjenu od $3, $2].",
'filerevert-comment'        => 'Komentar:',
'filerevert-defaultcomment' => 'Vraćeno na inačicu od $2, $1',
'filerevert-submit'         => 'Vrati',
'filerevert-success'        => "'''[[Media:$1|$1]]''' je vraćena na [$4 promjenu od $3, $2].",
'filerevert-badversion'     => 'Nema prethodne lokalne inačice datoteke s zadanim datumom i vremenom.',

# File deletion
'filedelete'                  => 'Izbriši $1',
'filedelete-legend'           => 'Izbriši datoteku',
'filedelete-intro'            => "Brišete datoteku '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Brišete inačicu '''[[Media:$1|$1]]''' od [$4 $3, $2].",
'filedelete-comment'          => 'Komentar:',
'filedelete-submit'           => 'Izbriši',
'filedelete-success'          => "Datoteka '''$1''' je izbrisana.",
'filedelete-success-old'      => "Inačica datoteke '''[[Media:$1|$1]]''' od $3, $2 je obrisana.",
'filedelete-nofile'           => "'''$1''' ne postoji na {{SITENAME}}.",
'filedelete-nofile-old'       => "Nema arhivirane verzije datoteke '''$1''' s zadanim parametrima.",
'filedelete-iscurrent'        => 'Pokušavate obrisati najnoviju inačicu ove datoteke. Molimo vas da prije toga vratite na stariju inačicu.',
'filedelete-otherreason'      => 'Drugi/dodatni razlog:',
'filedelete-reason-otherlist' => 'Drugi razlog',
'filedelete-reason-dropdown'  => '*Česti razlozi brisanja
** Kršenje autorskih prava
** Dupla datoteka
** Nekorištena datoteka',
'filedelete-edit-reasonlist'  => 'Uredi razloge za brisanje',

# MIME search
'mimesearch'         => 'MIME tražilica',
'mimesearch-summary' => 'Ova stranica omogućuje pretraživanje datoteka prema njihovim MIME zaglavljima. Ulazni parametar: tip_datoteke/podtip, npr. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME tip datoteke:',
'download'           => 'skidanje',

# Unwatched pages
'unwatchedpages' => 'Nenadgledane stranice',

# List redirects
'listredirects' => 'Popis preusmjeravanja',

# Unused templates
'unusedtemplates'     => 'Nekorišteni predlošci',
'unusedtemplatestext' => 'Slijedi popis svih stranica imenskog prostora "Predlošci", koje nisu umetnute na drugim stranicama. Pripazite da prije brisanja provjerite druge poveznice koje vode na te predloške.',
'unusedtemplateswlh'  => 'druge poveznice',

# Random page
'randompage'         => 'Slučajna stranica',
'randompage-nopages' => 'Nema stranica u ovom imenskom prostoru.',

# Random redirect
'randomredirect'         => 'Slučajno preusmjeravanje',
'randomredirect-nopages' => 'Nema preusmjeravanja u ovom imenskom prostoru.',

# Statistics
'statistics'             => 'Statistika',
'sitestats'              => 'Statistika ovog wikija',
'userstats'              => 'Statistika suradnika',
'sitestatstext'          => "U bazi podataka ukupno {{PLURAL:$1|je '''$1''' članak|su '''$1''' članka|je '''$1''' članaka}}.
Ovaj broj uključuje stranice za raspravu, stranice o projektu u prostoru {{SITENAME}}, kratke članke,
preusmjerene stranice, i sve ostale članke koje najvjerojatnije ne možemo računati kao sadržaj.

Trenutačno je '''$2''' članaka koji predstavljaju valjan sadržaj (nalaze se u glavnom prostoru i sadrže
barem jednu unutarnju poveznicu).

Snimljeno je '''$8''' datoteka.

Ukupno je '''$3''' pregleda stranica, i '''$4''' uređivanja članaka od pokretanja projekta {{SITENAME}}.
U prosjeku to iznosi '''$5''' uređivanja po stranici, i '''$6''' pregleda po uređivanju.

Duljina [http://www.mediawiki.org/wiki/Manual:Job_queue zadataka za izvršavanje] je '''$7'''.",
'userstatstext'          => "Broj prijavljenih [[Special:ListUsers|suradnika]] je '''$1'''. Od toga {{PLURAL:$2|je '''$2''' (ili '''$4%''') administrator|su '''$2''' (ili '''$4%''') administratora|su '''$2''' (ili '''$4%''') administratori}}, tj. imaju $5 prava.",
'statistics-mostpopular' => 'Najposjećenije stranice',

'disambiguations'      => 'Razdvojbene stranice',
'disambiguationspage'  => 'Template:Razdvojba',
'disambiguations-text' => "Sljedeće stranice povezuju na '''razdvojbenu stranicu'''.
Umjesto toga bi trebale povezivati na prikladnu temu.<br />
Stranica se tretira kao razdvojbena stranica ako koristi predložak na kojega vodi [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Dvostruka preusmjeravanja',
'doubleredirectstext'        => 'Ovo je popis preusmjeravanja na stranice za preusmjeravanje.
Svaki redak sadrži poveznice na prvo i drugo preusmjeravanje, te na prvi redak teksta drugog preusmjeravanja
koja obično ukazuje na "pravu" odredišnu stranicu, na koju bi trebalo pokazivati prvo preusmjeravanje.',
'double-redirect-fixed-move' => '[[$1]] je premješten, sada je preusmjeravanje na [[$2]]',
'double-redirect-fixer'      => 'Popravljač preusmjeravanja',

'brokenredirects'        => 'Kriva preusmjeravanja',
'brokenredirectstext'    => 'Sljedeća preusmjeravanja pokazuju na nepostojeće članke.',
'brokenredirects-edit'   => '(uredi)',
'brokenredirects-delete' => '(izbriši)',

'withoutinterwiki'         => 'Stranice bez međuwiki poveznica',
'withoutinterwiki-summary' => 'Sljedeće stranice nemaju poveznice na projekte na drugim jezicima:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Prikaži',

'fewestrevisions' => 'Članci s najmanje izmjena',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajta|bajtova}}',
'ncategories'             => '$1 {{PLURAL:$1|kategorija|kategorije|kategorija}}',
'nlinks'                  => '$1 {{PLURAL:$1|poveznica|poveznice|poveznica}}',
'nmembers'                => '$1 {{PLURAL:$1|član|članova}}',
'nrevisions'              => '$1 {{PLURAL:$1|inačica|inačice|inačica}}',
'nviews'                  => '$1 {{PLURAL:$1|put pogledano|puta pogledano|puta pogledano}}',
'specialpage-empty'       => 'Nema rezultata za traženi izvještaj.',
'lonelypages'             => 'Stranice siročad',
'lonelypagestext'         => 'Na sljedeće članke ne vode poveznice s drugih stranica na ovom wikiju ({{SITENAME}}).',
'uncategorizedpages'      => 'Nekategorizirane stranice',
'uncategorizedcategories' => 'Nekategorizirane kategorije',
'uncategorizedimages'     => 'Nekategorizirane datoteke',
'uncategorizedtemplates'  => 'Nekategorizirani predlošci',
'unusedcategories'        => 'Nekorištene kategorije',
'unusedimages'            => 'Nekorištene slike',
'popularpages'            => 'Popularne stranice',
'wantedcategories'        => 'Tražene kategorije',
'wantedpages'             => 'Tražene stranice',
'missingfiles'            => 'Datoteke koje nedostaju',
'mostlinked'              => 'Stranice na koje vodi najviše poveznica',
'mostlinkedcategories'    => 'Kategorije na koje vodi najviše poveznica',
'mostlinkedtemplates'     => 'Predlošci na koje vodi najviše poveznica',
'mostcategories'          => 'Popis članaka po broju kategorija',
'mostimages'              => 'Slike na koje vodi najviše poveznica',
'mostrevisions'           => 'Popis članaka po broju uređivanja',
'prefixindex'             => 'Kazalo prema početku naslova',
'shortpages'              => 'Kratke stranice',
'longpages'               => 'Duge stranice',
'deadendpages'            => 'Slijepe ulice',
'deadendpagestext'        => 'Slijedeće stranice nemaju poveznice na druge stranice na ovom wikiju ({{SITENAME}}).',
'protectedpages'          => 'Zaštićene stranice',
'protectedpages-indef'    => 'Samo neograničene zaštite',
'protectedpagestext'      => 'Slijedeće stranice su zaštićene od premještanja ili uređivanja',
'protectedpagesempty'     => 'Nema zaštićenih stranica koje ispunjavaju uvjete koje ste postavili.',
'protectedtitles'         => 'Zaštićeni naslovi',
'protectedtitlestext'     => 'Sljedeći naslovi su zaštićeni od kreiranja',
'protectedtitlesempty'    => 'Nijedan naslov nije trenutačno zaštićen s tim parametrima.',
'listusers'               => 'Popis suradnika',
'newpages'                => 'Nove stranice',
'newpages-username'       => 'Suradničko ime:',
'ancientpages'            => 'Najstarije stranice',
'move'                    => 'Premjesti',
'movethispage'            => 'Premjesti ovu stranicu',
'unusedimagestext'        => '<p>Moguće je da su druge mrežne stranice izvan ovog
wikija povezane na sliku neposrednim URLom, a nisu ovdje navedene unatoč aktivnoj uporabi.</p>',
'unusedcategoriestext'    => 'Na navedenim stranicama kategorija nema ni jednog članka ili potkategorije.',
'notargettitle'           => 'Nema odredišta',
'notargettext'            => 'Niste naveli ciljnu stranicu ili suradnika za izvršavanje ove funkcije.',
'nopagetitle'             => 'Nema ciljane stranice',
'nopagetext'              => 'Ciljana stranica koju ste odabrali ne postoji.',
'pager-newer-n'           => '{{PLURAL:$1|novija $1|novije $1|novijih $1}}',
'pager-older-n'           => '{{PLURAL:$1|starija $1|starije $1|starijih $1}}',
'suppress'                => 'Nadzor',

# Book sources
'booksources'               => 'Pretraživanje po ISBN-u',
'booksources-search-legend' => 'Traženje izvora za knjigu',
'booksources-go'            => 'Kreni',
'booksources-text'          => 'Ovdje je popis vanjskih poveznica na internetskim stranicama koje prodaju nove i rabljene knjige, ali mogu sadržavati i ostale podatke o knjigama koje tražite:',

# Special:Log
'specialloguserlabel'  => 'Suradnik:',
'speciallogtitlelabel' => 'Naslov:',
'log'                  => 'Evidencije',
'all-logs-page'        => 'Sve evidencije',
'log-search-legend'    => 'Pretraži evidencije',
'log-search-submit'    => 'Kreni',
'alllogstext'          => 'Skupni prikaz svih dostupnih evidencija za {{SITENAME}}.
Možete suziti prikaz odabirući tip evidencije, suradničko ime ili stranicu u upitu.',
'logempty'             => 'Nema pronađenih stavki.',
'log-title-wildcard'   => 'Traži stranice koje počinju s navedenim izrazom',

# Special:AllPages
'allpages'          => 'Sve stranice',
'alphaindexline'    => '$1 do $2',
'nextpage'          => 'Sljedeća stranica ($1)',
'prevpage'          => 'Prethodna stranica ($1)',
'allpagesfrom'      => 'Pokaži stranice počevši od:',
'allarticles'       => 'Svi članci',
'allinnamespace'    => 'Svi članci (prostor $1)',
'allnotinnamespace' => 'Sve stranice koje nisu u prostoru $1',
'allpagesprev'      => 'Prijašnje',
'allpagesnext'      => 'Sljedeće',
'allpagessubmit'    => 'Kreni',
'allpagesprefix'    => 'Stranice čiji naslov počinje s:',
'allpagesbadtitle'  => 'Zadana stranica nije valjana, ili je imala međuwiki predmetak. Možda sadrži jedan ili više znakova koji ne mogu biti uporabljeni u nazivu stranice.',
'allpages-bad-ns'   => '{{SITENAME}} nema imenski prostor "$1".',

# Special:Categories
'categories'                    => 'Kategorije',
'categoriespagetext'            => 'Sljedeće kategorije sadrže stranice ili datoteke.
[[Special:UnusedCategories|Nekorištene kategorije]] i [[Special:WantedCategories|tražene kategorije]] ovdje nisu prikazane.',
'categoriesfrom'                => 'Prikaži kategorije počevši od:',
'special-categories-sort-count' => 'razvrstavanje po broju',
'special-categories-sort-abc'   => 'abecedno razvrstavanje',

# Special:ListUsers
'listusersfrom'      => 'Prikaži suradnike počevši od:',
'listusers-submit'   => 'Prikaži',
'listusers-noresult' => 'Nema takvih suradnika.',

# Special:ListGroupRights
'listgrouprights'          => 'Prava suradničkih skupina',
'listgrouprights-summary'  => 'Ovo je popis suradničkih skupina određenih na ovoj wiki, s njihovim pripadajućim pravima.
Dodatne informacije o pojedinim pravim se mogu pronaći [[{{MediaWiki:Listgrouprights-helppage}}|ovdje]].',
'listgrouprights-group'    => 'Skupina',
'listgrouprights-rights'   => 'Prava',
'listgrouprights-helppage' => 'Help:Suradničke skupine',
'listgrouprights-members'  => '(popis članova)',

# E-mail user
'mailnologin'     => 'Nema adrese pošiljaoca',
'mailnologintext' => 'Morate biti [[Special:UserLogin|prijavljeni]]
i imati valjanu adresu e-pošte u svojim [[Special:Preferences|postavkama]]
da bi mogli slati poštu drugim suradnicima.',
'emailuser'       => 'Pošalji e-poštu ovom suradniku',
'emailpage'       => 'Pošalji e-poštu suradniku',
'emailpagetext'   => 'Ako je suradnik unio valjanu e-mail adresu u svojim postavkama, bit će mu poslana poruka s tekstom iz donjeg obrasca.
E-mail adresa iz vaših [[Special:Preferences|postavki]] nalazit će se u "From" polju poruke i primatelj će vam moći odgovoriti.',
'usermailererror' => 'Sustav pošte javio je pogrešku:',
'defemailsubject' => '{{SITENAME}} elektronička pošta (e-mail)',
'noemailtitle'    => 'Nema adrese primaoca',
'noemailtext'     => 'Ovaj suradnik nije unio valjanu e-mail adresu ili se odlučio na neće primati poštu od drugih suradnika.',
'emailfrom'       => 'Od:',
'emailto'         => 'Za:',
'emailsubject'    => 'Tema:',
'emailmessage'    => 'Poruka:',
'emailsend'       => 'Pošalji',
'emailccme'       => 'Pošalji mi e-mailom kopiju moje poruke.',
'emailccsubject'  => 'Kopija vaše poruke suradniku $1: $2',
'emailsent'       => 'E-mail poslan',
'emailsenttext'   => 'Vaša poruka je poslana.',
'emailuserfooter' => 'Ovaj e-mail je poslan od $1 za $2 korištenjem "elektroničke pošte" s projekta {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Moj popis praćenja',
'mywatchlist'          => 'Moj popis praćenja',
'watchlistfor'         => "(suradnika '''$1''')",
'nowatchlist'          => 'Na vašem popisu praćenja nema nijednog članka.',
'watchlistanontext'    => 'Molimo Vas $1 kako bi mogli vidjeti ili uređivati vaš popis praćenih stranica.',
'watchnologin'         => 'Niste prijavljeni',
'watchnologintext'     => 'Morate biti [[Special:UserLogin|prijavljeni]]
za promjene u popisu praćenja.',
'addedwatch'           => 'Dodano u popis praćenja',
'addedwatchtext'       => 'Stranica "<nowiki>$1</nowiki>" je dodana na vaš [[Special:Watchlist|popis praćenja]].
Promjene na ovoj stranici i njenoj stranici za razgovor bit će tamo prikazani, a stranica će biti ispisana
<b>podebljano</b> u [[Special:RecentChanges|popisu nedavnih promjena]], da biste je lakše primijetili.
<p>Ako poželite ukloniti stranicu s popisa praćenja, pritisnite "Prekini praćenje" u traci s naredbama.</p>',
'removedwatch'         => 'Odstranjena s popisa praćenja',
'removedwatchtext'     => 'Stranica "<nowiki>$1</nowiki>" je odstranjena s vašeg popisa praćenja.',
'watch'                => 'Prati',
'watchthispage'        => 'Prati ovu stranicu',
'unwatch'              => 'Prekini praćenje',
'unwatchthispage'      => 'Prekini praćenje',
'notanarticle'         => 'Nije članak',
'notvisiblerev'        => 'Izmjena je obrisana',
'watchnochange'        => 'Niti jedna od praćenih stranica nije promijenjena od vašeg zadnjeg posjeta.',
'watchlist-details'    => '{{PLURAL:$1|$1 stranica|$1 stranice|$1 stranica}} se nalazi na popisu praćenja, ne brojeći stranice za razgovor.',
'wlheader-enotif'      => '* Uključeno je izvješćivanje e-mailom.',
'wlheader-showupdated' => "* Stranice koje su promijenjene od vašeg zadnjeg posjeta prikazane su '''podebljano'''",
'watchmethod-recent'   => 'provjera nedavnih promjena praćenih stranica',
'watchmethod-list'     => 'provjera praćanih stranica za nedavne promjene',
'watchlistcontains'    => 'Vaš popis praćenja sadrži $1 {{PLURAL:$1|stranicu|stranice|stranica}}.',
'iteminvalidname'      => "Problem s izborom '$1', ime nije valjano...",
'wlnote'               => "Ovdje {{PLURAL:$1|je posljednja $1 promjena|su posljednje $1 promjene|je posljednjih $1 promjena}} u {{PLURAL:$2|posljednjem '''$2''' satu|posljednja '''$2''' sata|posljednjih '''$2''' sati}}.",
'wlshowlast'           => 'Pokaži zadnjih $1 sati $2 dana $3',
'watchlist-show-bots'  => 'Prikaži botovske promjene',
'watchlist-hide-bots'  => 'Sakrij botovske promjene',
'watchlist-show-own'   => 'Prikaži moje promjene',
'watchlist-hide-own'   => 'Sakrij moje promjene',
'watchlist-show-minor' => 'Prikaži manje promjene',
'watchlist-hide-minor' => 'Sakrij manje promjene',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Pratim...',
'unwatching' => 'Prestajem pratiti...',

'enotif_mailer'                => '{{SITENAME}} - izvješća o promjenama',
'enotif_reset'                 => 'Označi sve stranice kao već posjećene',
'enotif_newpagetext'           => 'Ovo je nova stranica.',
'enotif_impersonal_salutation' => '{{SITENAME}} suradnik',
'changed'                      => 'promijenio',
'created'                      => 'stvorio',
'enotif_subject'               => '{{SITENAME}}: Stranicu $PAGETITLE je $CHANGEDORCREATED suradnik $PAGEEDITOR',
'enotif_lastvisited'           => 'Pogledaj $1 za promjene od zadnjeg posjeta.',
'enotif_lastdiff'              => 'Pogledajte $1 kako biste mogli vidjeti tu izmjenu.',
'enotif_anon_editor'           => 'neprijavljeni suradnik $1',
'enotif_body'                  => '$WATCHINGUSERNAME,

stranicu na projektu {{SITENAME}} s naslovom $PAGETITLE je dana $PAGEEDITDATE $CHANGEDORCREATED suradnik $PAGEEDITOR,
pogledajte $PAGETITLE_URL za trenutačnu inačicu.

$NEWPAGE

Sažetak urednika: $PAGESUMMARY $PAGEMINOREDIT

Možete se javiti uredniku:
mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Do vašeg ponovnog posjeta stranici nećete dobivati daljnja izviješća.
Postavke za izvješćivanje možete resetirati na svom popisu praćenja.

            Vaš sustav izvješćivanja - hrvatska {{SITENAME}}.

--
Za promjene svog popisa praćenja posjetite
{{fullurl:Special:Watchlist|edit=yes}}

Za pomoć posjetite:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Izbriši stranicu',
'confirm'                     => 'Potvrdi',
'excontent'                   => "sadržaj je bio: '$1'",
'excontentauthor'             => "sadržaj je bio: '$1' (a jedini urednik '$2')",
'exbeforeblank'               => "sadržaj prije brisanja je bio: '$1'",
'exblank'                     => 'stranica je bila prazna',
'delete-confirm'              => 'Obriši "$1"',
'delete-legend'               => 'Izbriši',
'historywarning'              => 'UPOZORENJE: Stranica koju želite obrisati ima prijašnje inačice:',
'confirmdeletetext'           => 'Zauvijek ćete izbrisati stranicu ili sliku zajedno s prijašnjim inačicama.
Molim potvrdite svoju namjeru, da razumijete posljedice i da ovo radite u skladu s [[{{MediaWiki:Policy-url}}|pravilima]].',
'actioncomplete'              => 'Zahvat završen',
'deletedtext'                 => '"<nowiki>$1</nowiki>" je izbrisana.
Vidi $2 za evidenciju nedavnih brisanja.',
'deletedarticle'              => 'izbrisano "$1"',
'suppressedarticle'           => 'sakriven "[[$1]]"',
'dellogpage'                  => 'Evidencija_brisanja',
'dellogpagetext'              => 'Dolje je popis nedavnih brisanja.
Sva vremena su prema poslužiteljevom vremenu.',
'deletionlog'                 => 'evidencija brisanja',
'reverted'                    => 'Vraćeno na prijašnju inačicu',
'deletecomment'               => 'Razlog za brisanje',
'deleteotherreason'           => 'Drugi/dodatni razlog:',
'deletereasonotherlist'       => 'Drugi razlog',
'deletereason-dropdown'       => '*Razlozi brisanja stranica
** Zahtjev autora
** Kršenje autorskih prava
** Vandalizam',
'delete-edit-reasonlist'      => 'Uredi razloge brisanja',
'delete-toobig'               => 'Ova stranica ima veliku povijest uređivanja, preko $1 {{PLURAL:$1|promjene|promjena}}. Brisanje takvih stranica je ograničeno da se onemoguće slučajni problemi u radu {{SITENAME}}.',
'delete-warning-toobig'       => 'Ova stranica ima veliku povijest uređivanja, preko $1 {{PLURAL:$1|promjene|promjena}}. Brisanje može poremetiti bazu podataka {{SITENAME}}; postupajte s oprezom.',
'rollback'                    => 'Ukloni posljednju promjenu',
'rollback_short'              => 'Ukloni',
'rollbacklink'                => 'ukloni',
'rollbackfailed'              => 'Uklanjanje neuspješno',
'cantrollback'                => 'Ne mogu ukloniti posljednju promjenu, postoji samo jedna promjena.',
'alreadyrolled'               => 'Ne mogu ukloniti posljednju promjenu članka [[:$1]] koju je napravio suradnik [[User:$2|$2]]
([[User talk:$2|Talk]]); netko je već promijenio stranicu ili uklonio promjenu.

Posljednju promjenu napravio je suradnik [[User:$3|$3]] ([[User talk:$3|Talk]]).',
'editcomment'                 => 'Komentar promjene je: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Uklonjena promjena suradnika $2, vraćeno na zadnju inačicu suradnika $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Uklonjeno uređivanje suradnika $1; vraćeno na zadnju inačicu suradnika $2.',
'sessionfailure'              => 'Uočili smo problem s vašom prijavom. Zadnja naredba nije izvršena
kako bi izbjegla zloupotreba. Molimo vas da u pregledniku pritisnete "Natrag" (Back) i ponovno učitate stranicu
s koje ste stigli.',
'protectlogpage'              => 'Evidencija zaštićivanja',
'protectlogtext'              => 'Ispod je evidencija zaštićivanja i uklanjanja zaštite pojedinih stranica.
Pogledajte [[Special:ProtectedPages|zaštićene stranice]] za popis trenutačno zaštićenih stranica.',
'protectedarticle'            => 'članak "[[$1]]" je zaštićen',
'modifiedarticleprotection'   => 'promijenjen stupanj zaštite za "[[$1]]"',
'unprotectedarticle'          => 'uklonjena zaštita članka "[[$1]]"',
'protect-title'               => 'Zaštićujem "$1"',
'protect-legend'              => 'Potvrda zaštite',
'protectcomment'              => 'Komentar:',
'protectexpiry'               => 'Trajanje zaštite:',
'protect_expiry_invalid'      => 'Upisani vremenski rok nije valjan.',
'protect_expiry_old'          => 'Vrijeme isteka je u prošlosti.',
'protect-unchain'             => 'Otključaj ovlaštenja za premještanje',
'protect-text'                => 'Ovdje možete pregledati i promijeniti razinu zaštite za stranicu <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Ne možete mijenjati nivo zaštite dok ste blokirani.
Slijede postavke stranice <strong>$1</strong>:',
'protect-locked-dblock'       => 'Razina zaštite ne može biti promijenjena jer je baza zaključana.
Slijede postavke stranice <strong>$1</strong>:',
'protect-locked-access'       => 'Nemate ovlasti za mijenjanje razine zaštite.
Slijede trenutačne postavke stranice <strong>$1</strong>:',
'protect-cascadeon'           => 'Ova stranica je zaštićena jer je uključena u {{PLURAL:$1|stranicu, koja ima|stranice, koje imaju|stranice, koje imaju}} uključenu prenosivu zaštitu. Možete promijeniti stupanj zaštite ove stranice, no to neće utjecati na prenosivu zaštitu.',
'protect-default'             => '(bez zaštite)',
'protect-fallback'            => 'Potrebno je imati "$1" ovlasti',
'protect-level-autoconfirmed' => 'Blokiraj neprijavljene suradnike',
'protect-level-sysop'         => 'Samo administratori',
'protect-summary-cascade'     => 'prenosiva zaštita',
'protect-expiring'            => 'istječe $1 (UTC)',
'protect-cascade'             => 'Prenosiva zaštita - zaštiti sve stranice koje su uključene u ovu.',
'protect-cantedit'            => 'Ne možete mijenjati razinu zaštite ove stranice, jer nemate prava uređivati ju.',
'restriction-type'            => 'Dopuštenje:',
'restriction-level'           => 'Stupanj ograničenja:',
'minimum-size'                => 'Najmanja veličina',
'maximum-size'                => 'Najveća veličina:',
'pagesize'                    => '(bajtova)',

# Restrictions (nouns)
'restriction-edit'   => 'Uređivanje',
'restriction-move'   => 'Premještanje',
'restriction-create' => 'Stvori',
'restriction-upload' => 'Postavi',

# Restriction levels
'restriction-level-sysop'         => 'samo administratori',
'restriction-level-autoconfirmed' => 'samo prijavljeni suradnici',
'restriction-level-all'           => 'sve razine',

# Undelete
'undelete'                     => 'Vrati izbrisanu stranicu',
'undeletepage'                 => 'Vidi i/ili vrati izbrisane stranice',
'undeletepagetitle'            => "'''Sljedeći sadržaj se sastoji od izbrisanih izmjena [[:$1]]'''.",
'viewdeletedpage'              => 'Pogledaj izbrisanu stranicu',
'undeletepagetext'             => 'Sljedeće su stranice izbrisane, ali se još uvijek nalaze u bazi i mogu se obnoviti. Baza se povremeno čisti od ovakvih stranica.',
'undelete-fieldset-title'      => 'Vrati izmjene',
'undeleteextrahelp'            => "Da biste vratili cijelu stranicu, ostavite sve ''kućice'' neoznačene i kliknite '''Vrati!'''. Ako želite vratiti određene izmjene, označite ih i kliknite '''Vrati!'''. Klik na gumb '''Očisti''' će odznačiti sve ''kućice'' i obrisati polje za komentar.",
'undeleterevisions'            => '$1 {{PLURAL:$1|inačica je arhivirana|inačice su arhivirane|inačica je arhivirano}}',
'undeletehistory'              => 'Ako vratite izbrisanu stranicu, bit će vraćene i sve prijašnje promjene. Ako je u međuvremenu stvorena nova stranica s istim imenom, vraćena stranica bit će upisana kao prijašnja promjena sadašnje.',
'undeleterevdel'               => 'Vraćanje stranice neće biti izvršeno ako je rezultat toga djelomično brisanje zadnjeg uređivanja.
U takvim slučajevima morate isključiti ili otkriti najnovije obrisane promjene.',
'undeletehistorynoadmin'       => 'Ovaj je članak izbrisan. Razlog za brisanje prikazan je u donjem sažetku, zajedno s
detaljima o suradnicima koji su uređivali ovu stranicu prije brisanja.
Tekst izbrisanih inačica dostupan je samo administratorima.',
'undelete-revision'            => 'Izbrisana inačica članka $1 (dana $2), obrisao $3:',
'undeleterevision-missing'     => 'Nevaljana ili nepostojeća promjena. Poveznica je nevaljana,
ili je promjena vraćena ili uklonjena iz arhive.',
'undelete-nodiff'              => 'Prethodne promjene nisu nađene.',
'undeletebtn'                  => 'Vrati!',
'undeletelink'                 => 'vrati',
'undeletereset'                => 'Očisti',
'undeletecomment'              => 'Komentar:',
'undeletedarticle'             => 'vraćena stranica "$1"',
'undeletedrevisions'           => '{{PLURAL:$1|$1 inačica vraćena|$1 inačice vraćene|$1 inačica vraćeno}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|$1 promjena|$1 promjene|$1 promjena}} i {{PLURAL:$2|$2 datoteka vraćena|$2 datototeke vraćene|$2 datoteka vraćeno}}',
'undeletedfiles'               => '{{PLURAL:$1|$1 datoteka vraćena|$1 datoteke vraćene|$1 datoteka vraćeno}}',
'cannotundelete'               => 'Vraćanje obrisane inačice nije uspjelo; netko drugi je stranicu već vratio.',
'undeletedpage'                => "<big>'''$1 je vraćena'''</big>

Pogledajte [[Special:Log/delete|evidenciju brisanja]] za zapise nedavnih brisanja i vraćanja.",
'undelete-header'              => 'Pogledaj [[Special:Log/delete|evidenciju brisanja]] za nedavno obrisane stranice.',
'undelete-search-box'          => 'Pretraži obrisane stranice',
'undelete-search-prefix'       => 'Pretraži stranice koje počinju s:',
'undelete-search-submit'       => 'Pretraži',
'undelete-no-results'          => 'Nije pronađena odgovarajuća stranica u arhivu brisanja.',
'undelete-filename-mismatch'   => "Ne mogu vratiti inačicu datoteke s vremenom i datumom $1: imena se ne slažu (''filename mismatch'')",
'undelete-bad-store-key'       => 'Ne mogu vratiti inačicu datoteke s vremenom i datumom $1: datoteka ne postoji (obrisana je) prije vašeg pokušaja brisanja.',
'undelete-cleanup-error'       => 'Pogreška pri brisanju nekorištene arhivske datoteke "$1".',
'undelete-missing-filearchive' => 'Vraćanje arhivske datoteke s oznakom $1 nije moguće jer ne postoji u bazi podataka. Moguće je već vraćena.',
'undelete-error-short'         => 'Pogreška pri vraćanju datoteke: $1',
'undelete-error-long'          => 'Dogodila se pogreška pri vraćanju datoteke:

$1',

# Namespace form on various pages
'namespace'      => 'Imenski prostor:',
'invert'         => 'Sve osim odabranog',
'blanknamespace' => '(Glavni)',

# Contributions
'contributions' => 'Doprinosi suradnika',
'mycontris'     => 'Moji doprinosi',
'contribsub2'   => 'Za $1 ($2)',
'nocontribs'    => 'Nema promjena koje udovoljavaju ovim kriterijima.',
'uctop'         => ' (vrh)',
'month'         => 'Od mjeseca (i ranije):',
'year'          => 'Od godine (i ranije):',

'sp-contributions-newbies'     => 'Prikaži samo doprinose novih suradnika',
'sp-contributions-newbies-sub' => 'Za nove suradnike',
'sp-contributions-blocklog'    => 'Evidencija blokiranja',
'sp-contributions-search'      => 'Pretraži doprinose',
'sp-contributions-username'    => 'IP adresa ili suradnik:',
'sp-contributions-submit'      => 'Traži',

# What links here
'whatlinkshere'            => 'Što vodi ovamo',
'whatlinkshere-title'      => 'Stranice koje vode na "$1"',
'whatlinkshere-page'       => 'Stranica:',
'linklistsub'              => '(Popis poveznica)',
'linkshere'                => 'Sljedeće stranice povezuju ovamo ([[:$1]]):',
'nolinkshere'              => 'Nijedna stranica ne vodi ovamo (tj. nema poveznica na stranicu [[:$1]]).',
'nolinkshere-ns'           => "Nijedna stranica ne vodi na '''[[:$1]]''' u odabranom imenskom prostoru.",
'isredirect'               => 'stranica za preusmjeravanje',
'istemplate'               => 'kao predložak',
'isimage'                  => 'poveznica slike',
'whatlinkshere-prev'       => '{{PLURAL:$1|prethodna|prethodne|prethodnih}} $1',
'whatlinkshere-next'       => '{{PLURAL:$1|slijedeća|slijedeće|slijedećih}} $1',
'whatlinkshere-links'      => '← poveznice',
'whatlinkshere-hideredirs' => '$1 preusmjeravanja',
'whatlinkshere-hidetrans'  => '$1 transkluzije',
'whatlinkshere-hidelinks'  => '$1 poveznice',
'whatlinkshere-hideimages' => '$1 poveznice slike',
'whatlinkshere-filters'    => 'Filteri',

# Block/unblock
'blockip'                         => 'Blokiraj suradnika',
'blockip-legend'                  => 'Blokiraj suradnika',
'blockiptext'                     => 'Koristite donji obrazac za blokiranje pisanja pojedinih suradnika ili IP adresa .
To biste trebali raditi samo zbog sprječavanja vandalizma i u skladu
sa [[{{MediaWiki:Policy-url}}|smjernicama]].
Upišite i razlog za ovo blokiranje (npr. stranice koje su
vandalizirane).',
'ipaddress'                       => 'IP adresa',
'ipadressorusername'              => 'IP adresa ili suradničko ime',
'ipbexpiry'                       => 'Rok (na engleskom)',
'ipbreason'                       => 'Razlog',
'ipbreasonotherlist'              => 'Drugi razlog',
'ipbreason-dropdown'              => "*Najčešći razlozi za blokiranje
** Netočne informacije
** Uklanjanje sadržaja stranica
** Postavljanje ''spam'' vanjskih poveznica
** Grafiti
** Osobni napadi (ili napadačko ponašanje)
** Čarapare (zloporaba više suradničkih računa)
** Neprihvatljivo suradničko ime",
'ipbanononly'                     => 'Blokiraj samo neprijavljene suradnike',
'ipbcreateaccount'                => 'Spriječi otvaranje suradničkih računa',
'ipbemailban'                     => 'Onemogući blokiranom suradniku slanje e-mailova',
'ipbenableautoblock'              => 'Automatski blokiraj IP adrese koje koristi ovaj suradnik',
'ipbsubmit'                       => 'Blokiraj ovog suradnika',
'ipbother'                        => 'Neki drugi rok (na engleskom, npr. 6 days):',
'ipboptions'                      => '2 sata:2 hours,6 sati:6 hours,1 dan:1 day,3 dana:3 days,1 tjedan:1 week,2 tjedna:2 weeks,1 mjesec:1 month,3 mjeseca:3 months,6 mjeseci:6 months,1 godine:1 year,neograničeno:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'drugo',
'ipbotherreason'                  => 'Drugi/dodatni razlog:',
'ipbhidename'                     => 'Sakrij suradničko ime iz evidencije blokiranja, popisa blokiranja i popisa suradnika',
'ipbwatchuser'                    => 'Prati suradničku stranicu i stranicu za razgovor ovog suradnika',
'badipaddress'                    => 'Nevaljana IP adresa.',
'blockipsuccesssub'               => 'Uspješno blokirano',
'blockipsuccesstext'              => 'Suradnik [[Special:Contributions/$1|$1]] je blokiran.<br />
Pogledaj [[Special:IPBlockList|popis blokiranih IP adresa]] za pregled.',
'ipb-edit-dropdown'               => 'Uredi razloge blokiranja',
'ipb-unblock-addr'                => 'Odblokiraj $1',
'ipb-unblock'                     => 'Odblokiraj suradničko ime ili IP adresu',
'ipb-blocklist-addr'              => 'Vidi postojeća blokiranja za $1',
'ipb-blocklist'                   => 'Vidi postojeća blokiranja',
'unblockip'                       => 'Deblokiraj suradnika',
'unblockiptext'                   => 'Ovaj se obrazac koristi za vraćanje prava na pisanje prethodno blokiranoj IP adresi.',
'ipusubmit'                       => 'Deblokiraj ovu adresu',
'unblocked'                       => '[[User:$1|$1]] je deblokiran',
'unblocked-id'                    => 'Blok $1 je uklonjen',
'ipblocklist'                     => 'Popis blokiranih IP adresa i suradničkih računa',
'ipblocklist-legend'              => 'Pronađi blokiranog suradnika',
'ipblocklist-username'            => 'Ime suradnika ili IP adresa:',
'ipblocklist-submit'              => 'Traži',
'blocklistline'                   => '$1, $2 je blokirao $3 ($4)',
'infiniteblock'                   => 'neograničeno',
'expiringblock'                   => 'istječe $1',
'anononlyblock'                   => 'samo IP adrese',
'noautoblockblock'                => 'blokiranje samoga sebe je onemogućeno',
'createaccountblock'              => 'blokirano stvaranje suradničkog računa',
'emailblock'                      => 'e-mail je blokiran',
'ipblocklist-empty'               => 'Popis blokiranja je prazan.',
'ipblocklist-no-results'          => 'Tražena IP adresa ili suradničko ime nije blokirano.',
'blocklink'                       => 'blokiraj',
'unblocklink'                     => 'deblokiraj',
'contribslink'                    => 'doprinosi',
'autoblocker'                     => 'Automatski ste blokirani jer je vašu IP adresu nedavno koristio "[[User:$1|$1]]" koji je blokiran zbog: "$2".',
'blocklogpage'                    => 'Evidencija blokiranja',
'blocklogentry'                   => 'Blokiran je "[[$1]]" na rok $2 $3',
'blocklogtext'                    => 'Ovo je evidencija blokiranja i deblokiranja. Na popisu nema automatski blokiranih IP adresa. Za popis trenutačnih zabrana i blokiranja vidi [[Special:IPBlockList|popis IP blokiranja]].',
'unblocklogentry'                 => 'Deblokiran "$1"',
'block-log-flags-anononly'        => 'samo za neprijavljene suradnike',
'block-log-flags-nocreate'        => 'otvaranje novih suradničkih imena nije moguće',
'block-log-flags-noautoblock'     => 'autoblok je onemogućen',
'block-log-flags-noemail'         => 'e-mail je blokiran',
'block-log-flags-angry-autoblock' => 'Poboljšan autoblok uključen',
'range_block_disabled'            => 'Isključena je administratorska naredba za blokiranje raspona IP adresa.',
'ipb_expiry_invalid'              => 'Vremenski rok nije valjan.',
'ipb_expiry_temp'                 => 'Sakriveni računi bi trebali biti trajno blokirani.',
'ipb_already_blocked'             => '"$1" je već blokiran',
'ipb_cant_unblock'                => 'Pogreška: blok ID $1 nije nađen. Moguće je da je suradnik već odblokiran.',
'ipb_blocked_as_range'            => 'Pogreška: IP adresa $1 nije blokirana direktno te stoga ne može biti odblokirana. Blokirana je kao dio opsega $2, koji može biti odblokiran.',
'ip_range_invalid'                => 'Raspon IP adresa nije valjan.',
'blockme'                         => 'Blokiraj me',
'proxyblocker'                    => 'Zaštita od otvorenih posrednika (proxyja)',
'proxyblocker-disabled'           => 'Ova funkcija je onemogućena.',
'proxyblockreason'                => 'Vaša je IP adresa blokirana jer se radi o otvorenom posredniku (proxyju). Molim stupite u vezu s vašim davateljem internetskih usluga (ISP-om) ili službom tehničke podrške i obavijestite ih o ovom ozbiljnom sigurnosnom problemu.',
'proxyblocksuccess'               => 'Napravljeno.',
'sorbsreason'                     => 'Vaša IP adresa je na popisu otvorenih posrednika na poslužitelju DNSBL.',
'sorbs_create_account_reason'     => 'Vaša IP adresa je na popisu otvorenih posrednika na poslužitelju DNSBL. Ne možete otvoriti račun.',

# Developer tools
'lockdb'              => 'Zaključaj bazu podataka',
'unlockdb'            => 'Otključaj bazu podataka',
'lockdbtext'          => 'Zaključavanjem baze će se suradnicima onemogućiti uređivanje stranica, mijenjanje postavki i popisa praćenja, i sve drugo što zahtijeva promjene u bazi podataka.
Molim potvrdite svoju namjeru zaključavanja, te da ćete otključati bazu čim završite s održavanjem.',
'unlockdbtext'        => 'Otključavanjem baze omogućit ćete suradnicima uređivanje stranica,
mijenjanje postavki, uređivanje popisa praćenja i druge stvari koje zahtijevaju promjene u bazi. Molim potvrdite svoju namjeru.',
'lockconfirm'         => 'Da, sigurno želim zaključati bazu.',
'unlockconfirm'       => 'Da, sigurno želim otključati bazu.',
'lockbtn'             => 'Zaključaj bazu podataka',
'unlockbtn'           => 'Otključaj bazu podataka',
'locknoconfirm'       => 'Niste potvrdili svoje namjere.',
'lockdbsuccesssub'    => 'Zaključavanje baze podataka uspjelo',
'unlockdbsuccesssub'  => 'Otključavanje baze podataka uspjelo',
'lockdbsuccesstext'   => 'Baza podataka je zaključana.
<br />Ne zaboravite otključati po završetku održavanja.',
'unlockdbsuccesstext' => 'Baza podataka je otključana.',
'lockfilenotwritable' => "Web poslužitelj ne može pisati u ''lock'' datoteku. Za zaključavanje ili otključavanje baze podataka, web poslužitelj mora moći pisati u ovu datoteku.",
'databasenotlocked'   => 'Baza podataka nije zaključana.',

# Move page
'move-page'               => 'Premjesti $1',
'move-page-legend'        => 'Premjesti stranicu',
'movepagetext'            => "Korištenjem ovog obrasca ćete preimenovati stranicu i premjestiti sve stare izmjene na novo ime.
Stari će se naslov pretvoriti u stranicu koja automatski preusmjerava na novi naslov.
Možete odabrati automatsko ažuriranje preusmjeravanja na originalni naslov.
Ako se ne odlučite na to, provjerite [[Special:DoubleRedirects|dvostruka]] ili [[Special:BrokenRedirects|neispravna]] preusmjeravanja.

Stranica se '''neće''' premjestiti ako već postoji stranica s novim naslovom, osim u slučaju prazne stranice ili stranice za preusmjeravanje koja nema nikakvih starih izmjena.
To znači: 1. ako pogriješite, možete opet preimenovati stranicu na stari naslov, 2. ne može vam se dogoditi da izbrišete neku postojeću stranicu.

'''OPREZ!'''
Ovo može biti drastična i neočekivana promjena kad su u pitanju popularne stranice, i zato dobro razmislite prije nego što preimenujete stranicu.",
'movepagetalktext'        => "Stranica za razgovor, ako postoji, automatski će se premjestiti zajedno sa stranicom koju premještate. '''Stranica za razgovor neće se premjestiti ako:'''
*premještate stranicu iz jednog prostora u drugi,
*pod novim imenom već postoji stranica za razgovor s nekim sadržajem, ili
*maknete kvačicu u kućici na dnu ove stranice.

U tim slučajevima ćete morati sami premjestiti ili iskopirati stranicu za razgovor,
ako to želite.",
'movearticle'             => 'Premjesti stranicu',
'movenotallowed'          => 'Nemate pravo premještanja stranica na ovom projektu ({{SITENAME}}).',
'newtitle'                => 'Na novi naslov',
'move-watch'              => 'Prati ovu stranicu',
'movepagebtn'             => 'Premjesti stranicu',
'pagemovedsub'            => 'Premještanje uspjelo',
'movepage-moved'          => '<big>\'\'\'"$1" je premješteno na "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Stranica pod tim imenom već postoji ili ime koje ste odabrali nije u skladu s pravilima.
Molimo odaberite drugo ime.',
'cantmove-titleprotected' => 'Ne možete premjestiti ovu stranicu na ovo mjesto, jer je novi naslov zaštićen od kreiranja',
'talkexists'              => "'''Sama stranica je uspješno prenesena, ali stranicu za razgovor nije bilo moguće prenijeti jer na odredištu već postoji stranica za razgovor. Molimo da ih ručno spojite.'''",
'movedto'                 => 'premješteno na',
'movetalk'                => 'Premjesti i njezinu stranicu za razgovor ako je moguće.',
'move-subpages'           => 'Premjesti sve podstranice, ako je moguće',
'move-talk-subpages'      => 'Premjesti sve podstranice od stranice za razgovor, ako je moguće',
'movepage-page-exists'    => 'Stranica $1 već postoji i ne može biti automatski prepisana',
'movepage-page-moved'     => 'Stranica $1 je premještena na $2.',
'movepage-page-unmoved'   => 'Stranica $1 nije mogla biti premještena na $2.',
'movepage-max-pages'      => 'Najveća količina od $1 {{PLURAL:$1|stranice|stranica}} je premještena i više od toga neće biti automatski premješteno.',
'1movedto2'               => '$1 premješteno na $2',
'1movedto2_redir'         => '$1 premješteno na $2 preko postojećeg preusmjeravanja',
'movelogpage'             => 'Evidencija premještanja',
'movelogpagetext'         => 'Ispod je popis premještenih stranica.',
'movereason'              => 'Razlog',
'revertmove'              => 'vrati',
'delete_and_move'         => 'Izbriši i premjesti',
'delete_and_move_text'    => '==Nužno brisanje==

Odredišni članak "[[:$1]]" već postoji. Želite li ga obrisati da biste napravili mjesto za premještaj?',
'delete_and_move_confirm' => 'Da, izbriši stranicu',
'delete_and_move_reason'  => 'Obrisano kako bi se napravilo mjesta za premještaj.',
'selfmove'                => 'Izvorni i odredišni naslov su isti; ne mogu premjestiti stranicu na nju samu.',
'immobile_namespace'      => 'Odredišni naslov pripada posebnom tipu; u taj prostor ne mogu pomicati stranice.',
'imagenocrossnamespace'   => 'Datoteka ne može biti premještena u imenski prostor koji nije za datoteke',
'imagetypemismatch'       => 'Ekstenzija nove datoteke se ne poklapa sa svojim tipom.',
'imageinvalidfilename'    => 'Ciljano ime datoteke je nevaljano',
'fix-double-redirects'    => 'Ažuriraj sva preusmjeravanja koja vode na originalni naslov',

# Export
'export'            => 'Izvezi stranice',
'exporttext'        => 'Možete izvesti tekst i prijašnje promjene jedne ili više stranica uklopljene u XML kod. U budućim verzijama MediaWiki softvera bit će moguće uvesti ovakvu stranicu u neki drugi wiki. Trenutačna verzija to još ne podržava.

Za izvoz stranica unesite njihove naslove u polje ispod, jedan naslov po retku, i označite želite li trenutačnu inačicu zajedno sa svim prijašnjima, ili samo trenutačnu inačicu s informacijom o zadnjoj promjeni.

U potonjem slučaju možete koristiti i poveznicu, npr. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] za članak [[{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Uključi samo trenutačnu inačicu, ne i sve prijašnje',
'exportnohistory'   => "----
'''Napomena:''' izvoz cjelokupne stranice sa svim prethodnim izmjenama onemogućen je zbog opterećenja poslužitelja.",
'export-submit'     => 'Izvezi',
'export-addcattext' => 'Dodaj stranice iz kategorije:',
'export-addcat'     => 'Dodaj',
'export-download'   => 'Ponudi opciju snimanja u datoteku',
'export-templates'  => 'Uključi predloške',

# Namespace 8 related
'allmessages'               => 'Sve sistemske poruke',
'allmessagesname'           => 'Ime',
'allmessagesdefault'        => 'Prvotni tekst',
'allmessagescurrent'        => 'Trenutačni tekst',
'allmessagestext'           => 'Ovo je popis svih sistemskih poruka u prostoru MediaWiki: .',
'allmessagesnotsupportedDB' => "Ova stranica ne može biti korištena jer je isključen parametar '''\$wgUseDatabaseMessages'''.",
'allmessagesfilter'         => 'Filter imena poruka:',
'allmessagesmodified'       => 'Prikaži samo promijenjene',

# Thumbnails
'thumbnail-more'           => 'Povećaj',
'filemissing'              => 'Nedostaje datoteka',
'thumbnail_error'          => 'Pogrješka pri izradbi sličice: $1',
'djvu_page_error'          => "DjVu stranica nije dohvatljiva (''out of range'')",
'djvu_no_xml'              => 'Ne mogu dohvatiti XML za DjVu datoteku',
'thumbnail_invalid_params' => "Nevaljani parametri za smanjenu sliku (''thumbnail'')",
'thumbnail_dest_directory' => 'Ne mogu stvoriti ciljni direktorij',

# Special:Import
'import'                     => 'Uvezi stranice',
'importinterwiki'            => 'Transwiki uvoz',
'import-interwiki-text'      => 'Izaberite wiki i ime stranice za uvoz.
Povijest stranice i imena suradnika će biti sačuvani.
Transwiki uvoz stranica je zabilježen u [[Special:Log/import|evidenciji uvoza stranica]].',
'import-interwiki-history'   => 'Prenesi sve inačice ove stranice',
'import-interwiki-submit'    => 'Uvezi',
'import-interwiki-namespace' => 'Prenesi stranice u imenski prostor:',
'importtext'                 => 'Molim da izvezete ovu datoteku iz izvorišnog wikija koristeći pomagalo Special:Export, snimite je na svoj disk i postavite je ovdje.',
'importstart'                => 'Uvozim stranice...',
'import-revision-count'      => '$1 {{PLURAL:$1|izmjena|izmjene|izmjena}}',
'importnopages'              => 'Nema stranica za uvoz.',
'importfailed'               => 'Uvoz nije uspio: $1',
'importunknownsource'        => 'Nepoznat tip stranica za uvoz',
'importcantopen'             => 'Ne mogu otvoriti datoteku za uvoz',
'importbadinterwiki'         => 'Neispravna međuwiki poveznica',
'importnotext'               => 'Prazno ili bez teksta',
'importsuccess'              => 'Uvoz je uspio!',
'importhistoryconflict'      => 'Došlo je do konflikta među prijašnjim inačicama (ova je stranica možda već uvezena)',
'importnosources'            => 'Nije unesen nijedan izvor za transwiki uvoz i neposredno postavljanje povijesti je onemogućeno.',
'importnofile'               => 'Nije postavljena uvozna datoteka.',
'importuploaderrorsize'      => 'Uvoz datoteke nije uspio. Datoteka je veća od dopuštene veličine.',
'importuploaderrorpartial'   => 'Uvoz datoteke nije uspio. Datoteka je djelomično uvezena/snimljena.',
'importuploaderrortemp'      => 'Uvoz datoteke nije uspio. Nema privremenog direktorija.',
'import-parse-failure'       => 'Pogreška u parsiranju kod uvoza XML-a',
'import-noarticle'           => 'Nema stranice za uvoz!',
'import-nonewrevisions'      => 'Sve inačice su bile prethodno uvezene.',
'xml-error-string'           => '$1 u retku $2, stupac $3 (bajt $4): $5',
'import-upload'              => 'Postavljanje XML datoteka',

# Import log
'importlogpage'                    => 'Evidencija uvoza članaka',
'importlogpagetext'                => 'Administrativni uvoz stranica s poviješću uređivanja s drugih wikija.',
'import-logentry-upload'           => 'uvezeno [[$1]] uvozom datoteke',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|izmjena|izmjene|izmjena}}',
'import-logentry-interwiki'        => 'transwiki uvezeno $1',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|promjena|promjene|promjena}} od $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Moja suradnička stranica',
'tooltip-pt-anonuserpage'         => 'Suradnička stranica za IP adresu pod kojom uređujete',
'tooltip-pt-mytalk'               => 'Moja stranica za razgovor',
'tooltip-pt-anontalk'             => 'Razgovor o suradnicima s ove IP adrese',
'tooltip-pt-preferences'          => 'Moje postavke',
'tooltip-pt-watchlist'            => 'Popis stranica koje pratite.',
'tooltip-pt-mycontris'            => 'Popis mojih doprinosa',
'tooltip-pt-login'                => 'Predlažemo vam da se prijavite, ali nije obvezno.',
'tooltip-pt-anonlogin'            => 'Predlažemo vam da se prijavite, ali nije obvezno.',
'tooltip-pt-logout'               => 'Odjavi se',
'tooltip-ca-talk'                 => 'Razgovor o stranici',
'tooltip-ca-edit'                 => 'Možete uređivati ovu stranicu. Koristite Pregled kako će izgledati prije nego što snimite.',
'tooltip-ca-addsection'           => 'Dodaj komentar ovom razgovoru.',
'tooltip-ca-viewsource'           => 'Ova stranica je zaštićena. Možete pogledati izvorni kod.',
'tooltip-ca-history'              => 'Ranije izmjene na ovoj stranici.',
'tooltip-ca-protect'              => 'Zaštiti ovu stranicu',
'tooltip-ca-delete'               => 'Izbriši ovu stranicu',
'tooltip-ca-undelete'             => 'Vrati uređivanja na ovoj stranici prije nego što je izbrisana',
'tooltip-ca-move'                 => 'Premjesti ovu stranicu',
'tooltip-ca-watch'                => 'Dodaj ovu stranicu na svoj popis praćenja',
'tooltip-ca-unwatch'              => 'Ukloni ovu stranicu s popisa praćenja',
'tooltip-search'                  => 'Pretraži ovaj wiki',
'tooltip-search-go'               => 'Idi na stranicu s ovim imenom ako ona postoji',
'tooltip-search-fulltext'         => 'Traži ovaj tekst na svim stranicama',
'tooltip-p-logo'                  => 'Glavna stranica',
'tooltip-n-mainpage'              => 'Posjeti glavnu stranicu',
'tooltip-n-portal'                => 'O projektu, što možete učiniti, gdje je što',
'tooltip-n-currentevents'         => 'O trenutačnim događajima',
'tooltip-n-recentchanges'         => 'Popis nedavnih promjena u wikiju.',
'tooltip-n-randompage'            => 'Učitaj slučajnu stranicu',
'tooltip-n-help'                  => 'Mjesto za pomoć suradnicima.',
'tooltip-t-whatlinkshere'         => 'Popis svih stranica koje sadrže poveznice ovamo',
'tooltip-t-recentchangeslinked'   => 'Nedavne promjene na stranicama na koje vode ovdašnje poveznice',
'tooltip-feed-rss'                => 'RSS feed za ovu stranicu',
'tooltip-feed-atom'               => 'Atom feed za ovu stranicu',
'tooltip-t-contributions'         => 'Pogledaj popis suradnikovih doprinosa',
'tooltip-t-emailuser'             => 'Pošalji suradniku e-mail',
'tooltip-t-upload'                => 'Postavi slike i druge medije',
'tooltip-t-specialpages'          => 'Popis posebnih stranica',
'tooltip-t-print'                 => 'Verzija za ispis ove stranice',
'tooltip-t-permalink'             => 'Trajna poveznica na ovu verziju stranice',
'tooltip-ca-nstab-main'           => 'Pogledaj sadržaj',
'tooltip-ca-nstab-user'           => 'Pogledaj suradničku stranicu',
'tooltip-ca-nstab-media'          => 'Pogledaj stranicu s opisom medija',
'tooltip-ca-nstab-special'        => 'Ovo je posebna stranica koju nije moguće izravno uređivati.',
'tooltip-ca-nstab-project'        => 'Pogledaj stranicu o projektu',
'tooltip-ca-nstab-image'          => 'Pogledaj stranicu o slici',
'tooltip-ca-nstab-mediawiki'      => 'Pogledaj sistemske poruke',
'tooltip-ca-nstab-template'       => 'Pogledaj predložak',
'tooltip-ca-nstab-help'           => 'Pogledaj stranicu za pomoć',
'tooltip-ca-nstab-category'       => 'Pogledaj stranicu kategorije',
'tooltip-minoredit'               => 'Označi kao manju promjenu',
'tooltip-save'                    => 'Sačuvaj promjene',
'tooltip-preview'                 => 'Prikaži kako će izgledati, molimo koristite prije snimanja!',
'tooltip-diff'                    => 'Prikaži promjene učinjene u tekstu.',
'tooltip-compareselectedversions' => 'Prikaži usporedbu izabranih inačica ove stranice.',
'tooltip-watch'                   => 'Dodaj na popis praćenja',
'tooltip-recreate'                => 'Vrati stranicu unatoč tome što je obrisana',
'tooltip-upload'                  => "Pokreni snimanje (''upload'')",

# Stylesheets
'common.css'   => '/** Uređivanje ove CSS datoteke će se odraziti na sve skinove */',
'monobook.css' => '/** Ovdje idu izmjene monobook stylesheeta */',

# Scripts
'common.js'   => '/* JavaScript kod na ovoj stranici će biti izvršen kod svakog suradnika pri svakom učitavanju svake stranice wikija. */',
'monobook.js' => '/* Ne rabi se više; molimo rabite [[MediaWiki:common.js]] */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metapodaci su isključeni na ovom serveru.',
'nocreativecommons' => 'Creative Commons RDF metapodaci su isključeni na ovom serveru.',
'notacceptable'     => 'Wiki server ne može dobaviti podatke u obliku kojega vaš klijent može pročitati.',

# Attribution
'anonymous'        => 'Anonimni suradnik projekta {{SITENAME}}',
'siteuser'         => 'Suradnik $1 na projektu {{SITENAME}}',
'lastmodifiedatby' => 'Ovu je stranicu zadnji put mijenjao dana $2, $1 suradnik $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Temelji se na doprinosu suradnika $1.',
'others'           => 'drugih',
'siteusers'        => '{{SITENAME}} suradnik(ci) $1',
'creditspage'      => 'Autori stranice',
'nocredits'        => 'Za ovu stranicu nema podataka o autorima.',

# Spam protection
'spamprotectiontitle' => 'Zaštita od spama',
'spamprotectiontext'  => 'Stranicu koju ste željeli snimiti blokirao je filter spama.
Razlog je vjerojatno vanjska poveznica koja se nalazi na crnom popisu.',
'spamprotectionmatch' => 'Naš filter spama reagirao je na sljedeći tekst: $1',
'spambot_username'    => 'MediaWiki zaštita od spama',
'spam_reverting'      => 'Vraćam na zadnju inačicu koja ne sadrži poveznice na $1',
'spam_blanking'       => 'Sve inačice sadrže poveznice na $1, brišem cjelokupni sadržaj',

# Info page
'infosubtitle'   => 'Podaci o stranici',
'numedits'       => 'Broj promjena (članak): $1',
'numtalkedits'   => 'Broj promjena (stranica za razgovor): $1',
'numwatchers'    => 'Broj pratitelja: $1',
'numauthors'     => 'Broj autora (članak): $1',
'numtalkauthors' => 'Broj autora (stranica za razgovor): $1',

# Math options
'mw_math_png'    => 'Uvijek kao PNG',
'mw_math_simple' => 'Ako je vrlo jednostavno HTML, inače PNG',
'mw_math_html'   => 'Ako je moguće HTML, inače PNG',
'mw_math_source' => 'Ostavi u formatu TeX (za tekstualne preglednike)',
'mw_math_modern' => 'Preporučeno za današnje preglednike',
'mw_math_mathml' => 'Ako je moguće MathML (u pokusnoj fazi)',

# Patrolling
'markaspatrolleddiff'                 => 'Označi za pregledano',
'markaspatrolledtext'                 => 'Označi ovaj članak pregledanim',
'markedaspatrolled'                   => 'Pregledano',
'markedaspatrolledtext'               => 'Odabrana promjena već je pregledana.',
'rcpatroldisabled'                    => 'Nadzor nedavnih promjena isključen',
'rcpatroldisabledtext'                => 'Naredba "Nadziri nedavne promjene" trenutačno je isključena.',
'markedaspatrollederror'              => 'Ne mogu označiti za pregledano',
'markedaspatrollederrortext'          => 'Morate odabrati inačicu koju treba označiti za pregledanu.',
'markedaspatrollederror-noautopatrol' => 'Ne možete vlastite promjene označiti patroliranima.',

# Patrol log
'patrol-log-page'   => 'Evidencija pregledavanja promjena',
'patrol-log-header' => 'Ovo su evidencije patroliranih izmjena.',
'patrol-log-line'   => 'promjena broj $1 stranice $2 pregledana $3',
'patrol-log-auto'   => '(automatski pregledano)',

# Image deletion
'deletedrevision'                 => 'Izbrisana stara inačica $1',
'filedeleteerror-short'           => 'Pogreška u brisanju datoteke: $1',
'filedeleteerror-long'            => 'Dogodila se pogreška prilikom brisanja datoteke:

$1',
'filedelete-missing'              => 'Datoteka "$1" ne može biti obrisana, jer ne postoji.',
'filedelete-old-unregistered'     => 'Navedena promjena datoteke "$1" ne postoji u bazi podataka.',
'filedelete-current-unregistered' => 'Navedene datoteke "$1" nema u bazi podataka.',
'filedelete-archive-read-only'    => 'Web poslužitelj nema pravo pisanja u direktorij "$1".',

# Browsing diffs
'previousdiff' => '← Starija izmjena',
'nextdiff'     => 'Novija izmjena →',

# Media information
'mediawarning'         => "'''Upozorenje''': Ova datoteka možda sadrži zlonamjerni program čije bi izvršavanje moglo ugroziti vaš računalni sustav.
<hr />",
'imagemaxsize'         => 'Ograniči veličinu slike na stranici s opisom:',
'thumbsize'            => 'Veličina sličice (umanjene inačice slike):',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|stranica|stranice}}',
'file-info'            => '(veličina datoteke: $1, MIME tip: $2)',
'file-info-size'       => '($1 × $2 piksela, veličina datoteke: $3, MIME tip: $4)',
'file-nohires'         => '<small>Viša rezolucija nije dostupna.</small>',
'svg-long-desc'        => '(SVG datoteka, nominalno $1 × $2 piksela, veličina datoteke: $3)',
'show-big-image'       => 'Vidi sliku u punoj veličini (rezoluciji)',
'show-big-image-thumb' => '<small>Veličina pretpregleda: $1 × $2 piksela</small>',

# Special:NewImages
'newimages'             => 'Galerija novih datoteka',
'imagelisttext'         => 'Ispod je popis {{PLURAL:$1|$1 slike|$1 slike|$1 slika}} složen $2.',
'newimages-summary'     => 'Ova posebna stranica pokazuje zadnje nedavno postavljene datoteke.',
'showhidebots'          => '($1 botova)',
'noimages'              => 'Nema slika.',
'ilsubmit'              => 'Traži',
'bydate'                => 'po datumu',
'sp-newimages-showfrom' => 'Prikaži nove slike počevši od $2, $1',

# Bad image list
'bad_image_list' => "Rabi se sljedeći format:

Samo retci koji počinju sa zvjezdicom su prikazani. Prva poveznica u retku mora biti poveznica na nevaljanu sliku.
Svaka sljedeća poveznica u istom retku je izuzetak, npr. kod stranica gdje se slike pojavljuju ''inline''.",

# Variants for Serbian language
'variantname-sr-ec' => 'ћирилица',
'variantname-sr-el' => 'latinica',

# Metadata
'metadata'          => 'Metapodaci',
'metadata-help'     => 'Ova datoteka sadržava dodatne podatke koje je vjerojatno dodala digitalna kamera ili skener u procesu snimanja odnosno digitalizacije. Ako je datoteka mijenjana, podatci možda nisu u skladu sa stvarnim stanjem.',
'metadata-expand'   => 'Pokaži sve podatke',
'metadata-collapse' => 'Sakrij dodatne podatke',
'metadata-fields'   => "Slijedeći EXIF metapodaci će biti prikazani ispod slike u tablici s metapodacima. Ostali će biti sakriveni (možete ih vidjeti ako kliknete na poveznicu ''Pokaži sve podatke'').
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Širina',
'exif-imagelength'                 => 'Visina',
'exif-bitspersample'               => 'Dubina boje',
'exif-compression'                 => 'Način sažimanja',
'exif-photometricinterpretation'   => 'Kolor model',
'exif-orientation'                 => 'Orijentacija kadra',
'exif-samplesperpixel'             => 'Broj kolor komponenata',
'exif-planarconfiguration'         => 'Princip rasporeda podataka',
'exif-ycbcrsubsampling'            => 'Omjer kompnente Y prema C',
'exif-ycbcrpositioning'            => 'Razmještaj komponenata Y i C',
'exif-xresolution'                 => 'Vodoravna razlučivost',
'exif-yresolution'                 => 'Okomita razlučivost',
'exif-resolutionunit'              => 'Jedinica razlučivosti',
'exif-stripoffsets'                => 'Položaj bloka podataka',
'exif-rowsperstrip'                => 'Broj redova u bloku',
'exif-stripbytecounts'             => 'Veličina komprimiranog bloka',
'exif-jpeginterchangeformat'       => 'Udaljenost JPEG previewa od početka datoteke',
'exif-jpeginterchangeformatlength' => 'Količina bajtova JPEG previewa',
'exif-transferfunction'            => 'Funkcija preoblikovanja kolor prostora',
'exif-whitepoint'                  => 'Kromaticitet bijele točke',
'exif-primarychromaticities'       => 'Kromaticitet primarnih boja',
'exif-ycbcrcoefficients'           => 'Matrični koeficijenti preobrazbe kolor prostora',
'exif-referenceblackwhite'         => 'Mjesto bijele i crne točke',
'exif-datetime'                    => 'Datum zadnje promjene datoteke',
'exif-imagedescription'            => 'Ime slike',
'exif-make'                        => 'Proizvođač kamere',
'exif-model'                       => 'Model kamere',
'exif-software'                    => 'Korišteni softver',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Nositelj prava',
'exif-exifversion'                 => 'Exif verzija',
'exif-flashpixversion'             => 'Podržana verzija Flashpixa',
'exif-colorspace'                  => 'Kolor prostor',
'exif-componentsconfiguration'     => 'Značenje pojedinih komponenti',
'exif-compressedbitsperpixel'      => 'Dubina boje poslije sažimanja',
'exif-pixelydimension'             => 'Puna visina slike',
'exif-pixelxdimension'             => 'Puna širina slike',
'exif-makernote'                   => 'Napomene proizvođača',
'exif-usercomment'                 => 'Suradnički komentar',
'exif-relatedsoundfile'            => 'Povezani zvučni zapis',
'exif-datetimeoriginal'            => 'Datum i vrijeme slikanja',
'exif-datetimedigitized'           => 'Datum i vrijeme digitalizacije',
'exif-subsectime'                  => 'Dio sekunde u kojem je slikano',
'exif-subsectimeoriginal'          => 'Dio sekunde u kojem je fotografirano',
'exif-subsectimedigitized'         => 'Dio sekunde u kojem je digitalizirano',
'exif-exposuretime'                => 'Ekspozicija',
'exif-exposuretime-format'         => '$1 sekunda ($2)',
'exif-fnumber'                     => 'F broj dijafragme',
'exif-exposureprogram'             => 'Program ekspozicije',
'exif-spectralsensitivity'         => 'Spektralna osjetljivost',
'exif-isospeedratings'             => 'ISO vrijednost',
'exif-oecf'                        => 'Optoelektronski faktor konverzije',
'exif-shutterspeedvalue'           => 'Brzina zatvarača',
'exif-aperturevalue'               => 'Dijafragma',
'exif-brightnessvalue'             => 'Osvijetljenost',
'exif-exposurebiasvalue'           => 'Kompenzacija ekspozicije',
'exif-maxaperturevalue'            => 'Minimalni broj dijafragme',
'exif-subjectdistance'             => 'Udaljenost do objekta',
'exif-meteringmode'                => 'Režim mjerača vremena',
'exif-lightsource'                 => 'Izvor svjetlosti',
'exif-flash'                       => 'Bljeskalica',
'exif-focallength'                 => 'Žarišna duljina leće',
'exif-subjectarea'                 => 'Položaj i površina objekta snimke',
'exif-flashenergy'                 => 'Energija bljeskalice',
'exif-spatialfrequencyresponse'    => 'Prostorna frekvencijska karakteristika',
'exif-focalplanexresolution'       => 'Vodoravna razlučivost žarišne ravnine',
'exif-focalplaneyresolution'       => 'Okomita razlučivost žarišne ravnine',
'exif-focalplaneresolutionunit'    => 'Jedinica razlučivosti žarišne ravnine',
'exif-subjectlocation'             => 'Položaj subjekta',
'exif-exposureindex'               => 'Indeks ekspozicije',
'exif-sensingmethod'               => 'Tip senzora',
'exif-filesource'                  => 'Izvorna datoteka',
'exif-scenetype'                   => 'Tip scene',
'exif-cfapattern'                  => 'Tip kolor filtera',
'exif-customrendered'              => 'Dodatna obrada slike',
'exif-exposuremode'                => 'Režim izbora ekspozicije',
'exif-whitebalance'                => 'Balans bijele',
'exif-digitalzoomratio'            => 'Razmjer digitalnog zooma',
'exif-focallengthin35mmfilm'       => 'Ekvivalent žarišne daljine za 35 mm film',
'exif-scenecapturetype'            => 'Tip scene na snimci',
'exif-gaincontrol'                 => 'Kontrola osvijetljenosti',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Zasićenje',
'exif-sharpness'                   => 'Oštrina',
'exif-devicesettingdescription'    => 'Opis postavki uređaja',
'exif-subjectdistancerange'        => 'Raspon udaljenosti subjekata',
'exif-imageuniqueid'               => 'Jedinstveni identifikator slike',
'exif-gpsversionid'                => 'Verzija bloka GPS-informacije',
'exif-gpslatituderef'              => 'Sjeverna ili južna širina',
'exif-gpslatitude'                 => 'Širina',
'exif-gpslongituderef'             => 'Istočna ili zapadna dužina',
'exif-gpslongitude'                => 'Dužina',
'exif-gpsaltituderef'              => 'Visina ispod ili iznad mora',
'exif-gpsaltitude'                 => 'Visina',
'exif-gpstimestamp'                => 'Vrijeme po GPS-u (atomski sat)',
'exif-gpssatellites'               => 'Korišteni sateliti',
'exif-gpsstatus'                   => 'Status prijemnika',
'exif-gpsmeasuremode'              => 'Režim mjerenja',
'exif-gpsdop'                      => 'Preciznost mjerenja',
'exif-gpsspeedref'                 => 'Jedinica brzine',
'exif-gpsspeed'                    => 'Brzina GPS prijemnika',
'exif-gpstrackref'                 => 'Tip azimuta prijemnika (pravi ili magnetni)',
'exif-gpstrack'                    => 'Azimut prijemnika',
'exif-gpsimgdirectionref'          => 'Tip azimuta slike (pravi ili magnetni)',
'exif-gpsimgdirection'             => 'Azimut slike',
'exif-gpsmapdatum'                 => 'Korišteni geodetski koordinatni sustav',
'exif-gpsdestlatituderef'          => 'Indeks zemlj. širine objekta',
'exif-gpsdestlatitude'             => 'Zemlj. širina objekta',
'exif-gpsdestlongituderef'         => 'Indeks zemlj. dužine objekta',
'exif-gpsdestlongitude'            => 'Zemljopisna dužina objekta',
'exif-gpsdestbearingref'           => 'Indeks pelenga objekta',
'exif-gpsdestbearing'              => 'Peleng objekta',
'exif-gpsdestdistanceref'          => 'Mjerne jedinice udaljenosti objekta',
'exif-gpsdestdistance'             => 'Udaljenost objekta',
'exif-gpsprocessingmethod'         => 'Ime metode obrade GPS podataka',
'exif-gpsareainformation'          => 'Ime GPS područja',
'exif-gpsdatestamp'                => 'GPS datum',
'exif-gpsdifferential'             => 'GPS diferencijalna korekcija',

# EXIF attributes
'exif-compression-1' => 'Nesažeto',

'exif-unknowndate' => 'Datum nepoznat',

'exif-orientation-1' => 'Normalno', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Zrcaljeno po horizontali', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Zaokrenuto 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Zrcaljeno po vertikali', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Zaokrenuto 90° suprotno od sata i zrcaljeno po vertikali', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Zaokrenuto 90° u smjeru sata', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Zaokrenuto 90° u smjeru sata i zrcaljeno po vertikali', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Zaokrenuto 90° suprotno od sata', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'zrnasti format',
'exif-planarconfiguration-2' => 'planarni format',

'exif-componentsconfiguration-0' => 'ne postoji',

'exif-exposureprogram-0' => 'Nepoznato',
'exif-exposureprogram-1' => 'Ručno',
'exif-exposureprogram-2' => 'Normalni program',
'exif-exposureprogram-3' => 'Prioritet dijafragme',
'exif-exposureprogram-4' => 'Prioritet zatvarača',
'exif-exposureprogram-5' => 'Umjetnički program (na temelju nužne dubine polja)',
'exif-exposureprogram-6' => 'Sportski program (na temelju što bržeg zatvarača)',
'exif-exposureprogram-7' => 'Portretni režim (za krupne planove s neoštrom pozadinom)',
'exif-exposureprogram-8' => 'Režim krajolika (za slike krajolika s oštrom pozadinom)',

'exif-subjectdistance-value' => '$1 metara',

'exif-meteringmode-0'   => 'Nepoznato',
'exif-meteringmode-1'   => 'Prosjek',
'exif-meteringmode-2'   => 'Prosjek s težištem na sredini',
'exif-meteringmode-3'   => 'Točka',
'exif-meteringmode-4'   => 'Više točaka',
'exif-meteringmode-5'   => 'Matrični',
'exif-meteringmode-6'   => 'Djelomični',
'exif-meteringmode-255' => 'Drugo',

'exif-lightsource-0'   => 'Nepoznato',
'exif-lightsource-1'   => 'Dnevna svjetlost',
'exif-lightsource-2'   => 'Fluorescentno',
'exif-lightsource-3'   => 'Volframska žarulja',
'exif-lightsource-4'   => 'Bljeskalica',
'exif-lightsource-9'   => 'Lijepo vrijeme',
'exif-lightsource-10'  => 'Oblačno vrijeme',
'exif-lightsource-11'  => 'Sjena',
'exif-lightsource-12'  => 'Fluorescentna svjetlost (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Fluorescentna svjetlost (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fluorescentna svjetlost (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Bijela fluorescencija (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardno svjetlo A',
'exif-lightsource-18'  => 'Standardno svjetlo B',
'exif-lightsource-19'  => 'Standardno svjetlo C',
'exif-lightsource-24'  => 'ISO studijska svjetiljka',
'exif-lightsource-255' => 'Drugi izvor svjetla',

'exif-focalplaneresolutionunit-2' => 'inči',

'exif-sensingmethod-1' => 'Nedefinirano',
'exif-sensingmethod-2' => 'Jednokristalni matrični senzor',
'exif-sensingmethod-3' => 'Dvokristalni matrični senzor',
'exif-sensingmethod-4' => 'Trokristalni matrični senzor',
'exif-sensingmethod-5' => 'Sekvencijalni matrični senzor',
'exif-sensingmethod-7' => 'Trobojni linearni senzor',
'exif-sensingmethod-8' => 'Sekvencijalni linearni senzor',

'exif-filesource-3' => 'Digitalni fotoaparat',

'exif-scenetype-1' => 'Izravno fotografirana slika',

'exif-customrendered-0' => 'Normalni proces',
'exif-customrendered-1' => 'Nestadardni proces',

'exif-exposuremode-0' => 'Automatski',
'exif-exposuremode-1' => 'Ručno',
'exif-exposuremode-2' => 'Automatski sa zadanim rasponom',

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
'exif-subjectdistancerange-1' => 'Krupni plan',
'exif-subjectdistancerange-2' => 'Bliski plan',
'exif-subjectdistancerange-3' => 'Udaljeno',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Sjever',
'exif-gpslatitude-s' => 'Jug',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Istok',
'exif-gpslongitude-w' => 'Zapad',

'exif-gpsstatus-a' => 'Mjerenje u tijeku',
'exif-gpsstatus-v' => 'Spreman za prijenos',

'exif-gpsmeasuremode-2' => 'Dvodimenzionalno mjerenje',
'exif-gpsmeasuremode-3' => 'Trodimenzionalno mjerenje',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'kmh',
'exif-gpsspeed-m' => 'mph',
'exif-gpsspeed-n' => 'čv',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Pravi sjever',
'exif-gpsdirection-m' => 'Magnetni sjever',

# External editor support
'edit-externally'      => 'Uredi koristeći se vanjskom aplikacijom',
'edit-externally-help' => 'Vidi [http://www.mediawiki.org/wiki/Manual:External_editors setup upute] za više informacija.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'sve',
'imagelistall'     => 'sve',
'watchlistall2'    => 'sve',
'namespacesall'    => 'sve',
'monthsall'        => 'sve',

# E-mail address confirmation
'confirmemail'             => 'Potvrda e-mail adrese',
'confirmemail_noemail'     => 'Niste unijeli važeću e-mail adresu u vaše [[Special:Preferences|suradničke postavke]].',
'confirmemail_text'        => 'U ovom wikiju morate prije korištenja e-mail naredbi verificirati svoju e-mail adresu. Kliknite na dugme ispod kako biste
poslali poruku s potvrdom na vašu adresu. U poruci će biti poveznica koju morate otvoriti u
svom web pregledniku da biste verificirali adresu.',
'confirmemail_pending'     => '<div class="error">
Već vam je e-mailom poslan potvrdni kôd; ako ste upravo otvorili suradnički račun, molimo pričekajte još nekoliko minuta na e-mailu, prije nego što postavite zahtjev za novi kôd.
</div>',
'confirmemail_send'        => 'Pošalji kôd za potvrdu e-mail adrese',
'confirmemail_sent'        => 'Poruka s potvrdom je poslana.',
'confirmemail_oncreate'    => 'Potvrdni kôd poslan je na vašu elektroničku adresu.
Ovaj kôd nije potreban za prijavljivanje, no bit će vam potreban kako biste osposobili neke od postavki na Wikipediji koje uključuju elektroničku poštu.',
'confirmemail_sendfailed'  => 'Projekt {{SITENAME}} nije uspio poslati vaš potvrdni email.
Provjerite pravilnost adrese.
Poruka o pogrešci e-mail poslužitelja: $1',
'confirmemail_invalid'     => 'Pogrešna potvrda. Kôd je možda istekao.',
'confirmemail_needlogin'   => 'Trebate se $1, kako bi se potvrdila vaša e-mail adresa.',
'confirmemail_success'     => 'Vaša je e-mail adresa potvrđena. Možete se prijaviti i uživati u wikiju.',
'confirmemail_loggedin'    => 'Vaša je e-mail adresa potvrđena.',
'confirmemail_error'       => 'Došlo je do greške kod snimanja vaše potvrde.',
'confirmemail_subject'     => '{{SITENAME}}: potvrda e-mail adrese',
'confirmemail_body'        => 'Vi ili netko drugi s IP adrese $1 ste otvorili
suradnički račun pod imenom "$2" s ovom e-mail adresom na Wikipediji.

Kako biste potvrdili da je ovaj suradnički račun uistinu vaš i
uključili e-mail naredbe na Wikipediji, otvorite u vašem
pregledniku sljedeću poveznicu:

$3

Ako ovo *niste* vi, slijedite ovaj link za poništavanje potvrde:

$5

Valjanost ovog potvrdnog koda istječe $4.',
'confirmemail_invalidated' => 'Potvrda E-mail adrese je otkazana',
'invalidateemail'          => 'Poništi potvrđivanje elektroničke pošte',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki transkluzija isključena]',
'scarytranscludefailed'   => '[Dobava predloška nije uspjela za $1]',
'scarytranscludetoolong'  => '[URL je predug]',

# Trackbacks
'trackbackbox'      => "<div id='mw_trackbacks'>
''Trackbackovi'' za ovaj članak:<br />
$1
</div>",
'trackbackremove'   => ' ([$1 izbrisati])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback izbrisan.',

# Delete conflict
'deletedwhileediting' => "'''Upozorenje''': Ova stranica je obrisana nakon što ste počeli uređivati!",
'confirmrecreate'     => "Suradnik [[User:$1|$1]] ([[User talk:$1|talk]]) izbrisao je ovaj članak nakon što ste ga počeli uređivati. Razlog brisanja
: ''$2''
Potvrdite namjeru vraćanja ovog članka.",
'recreate'            => 'Vrati',

# HTML dump
'redirectingto' => 'Preusmjeravam na [[:$1]]...',

# action=purge
'confirm_purge'        => 'Isprazniti međuspremnik stranice?

$1',
'confirm_purge_button' => 'U redu',

# AJAX search
'searchcontaining' => "Traži članke koji sadržavaju ''$1''.",
'searchnamed'      => "Traži članke po imenu ''$1''.",
'articletitles'    => "Članci koji počinju s ''$1''",
'hideresults'      => 'Sakrij rezultate',
'useajaxsearch'    => 'Koristite AJAX-pretragu',

# Multipage image navigation
'imgmultipageprev' => '← prethodna slika',
'imgmultipagenext' => 'slijedeća slika →',
'imgmultigo'       => 'Kreni!',
'imgmultigoto'     => 'Idi na stranicu $1',

# Table pager
'ascending_abbrev'         => 'rast',
'descending_abbrev'        => 'pad',
'table_pager_next'         => 'Sljedeća stranica',
'table_pager_prev'         => 'Prethodna stranica',
'table_pager_first'        => 'Prva stranica',
'table_pager_last'         => 'Zadnja stranica',
'table_pager_limit'        => 'Prikaži $1 slika po stranici',
'table_pager_limit_submit' => 'Idi',
'table_pager_empty'        => 'Nema rezultata',

# Auto-summaries
'autosumm-blank'   => 'Uklonjen cjelokupni sadržaj stranice',
'autosumm-replace' => "Tekst stranice se zamjenjuje s '$1'",
'autoredircomment' => 'Preusmjeravanje na [[$1]]',
'autosumm-new'     => 'Nova stranica: $1',

# Live preview
'livepreview-loading' => 'Učitavam…',
'livepreview-ready'   => 'Učitavam… gotovo!',
'livepreview-failed'  => 'Lokalni (JavaScript) pretpregled nije uspio! Pokušajte normalni pretpregled.',
'livepreview-error'   => 'Spajanje nije uspjelo: $1 "$2". Pokušajte normalni pretpregled.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Moguće je da izmjene nastale zadnjih $1 {{PLURAL:$1|sekundu|sekundi}} neće biti vidljive na ovom popisu.',
'lag-warn-high'   => 'Zbog kašnjenja baze podataka, promjene napravljene zadnjih $1 {{PLURAL:$1|sekundu|sekundi}} moguće nisu prikazane u popisu.',

# Watchlist editor
'watchlistedit-numitems'       => 'Vaš popis praćenja sadrži {{PLURAL:$1|1 stranicu|$1 stranica}}, bez stranica za razgovor.',
'watchlistedit-noitems'        => 'Vaš popis praćenja je prazan.',
'watchlistedit-normal-title'   => 'Uredi popis praćenih stranica',
'watchlistedit-normal-legend'  => 'Ukloni stranice iz popisa praćenja',
'watchlistedit-normal-explain' => "Prikazane su stranice na vašem popisu praćenja. Da uklonite neku s popisa praćenja, označite kućicu kraj nje, i kliknite na gumb '''Ukloni stranice''' na dnu ove stranice. Možete također [[Special:Watchlist/raw|uređivati ovaj popis u okviru za uređivanje]].",
'watchlistedit-normal-submit'  => 'Ukloni stranice',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 stranica je uklonjena|$1 stranice su uklonjene}} iz vašeg popisa praćenja. Slijedi popis uklonjenih:',
'watchlistedit-raw-title'      => 'Uredi praćene stranice u okviru za uređivanje',
'watchlistedit-raw-legend'     => 'Uredi praćene stranice',
'watchlistedit-raw-explain'    => "Imena stranica na vašem popisu praćenja su prikazana ispod, možete uređivati taj popis dodavanjem novih stranica,
ili brisanjem postojećih; u jednom retku je ime jedne stranice.

Kad završite s uređivanjem, kliknite na '''Snimi promjene'''.
Također možete koristiti [[Special:Watchlist/edit|uređivanje popisa putem ''kućica za označivanje (checkboxova)'']].",
'watchlistedit-raw-titles'     => 'Imena stranica:',
'watchlistedit-raw-submit'     => 'Snimi promjene',
'watchlistedit-raw-done'       => 'Vaš popis praćenja je snimljen.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 stranica je dodana|$1 stranice su dodane}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 stranica je uklonjena|$1 stranice su ukonjene}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Pregled promjena praćenih stranica',
'watchlisttools-edit' => 'Pregled i uređivanje praćenih stranica',
'watchlisttools-raw'  => 'Uređivanje praćenih stranica u okviru za uređivanje',

# Core parser functions
'unknown_extension_tag' => "Nepoznat ''tag'' ekstenzije \"\$1\"",

# Special:Version
'version'                          => 'Verzija softvera', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Instalirana proširenja',
'version-specialpages'             => 'Posebne stranice',
'version-parserhooks'              => 'Kuke parsera',
'version-variables'                => 'Varijable',
'version-other'                    => 'Ostalo',
'version-mediahandlers'            => 'Rukovatelji medijima',
'version-hooks'                    => 'Kuke',
'version-extension-functions'      => 'Funkcije proširenja',
'version-parser-extensiontags'     => 'Oznake proširenja parsera',
'version-parser-function-hooks'    => 'Kuke funkcija parsera',
'version-skin-extension-functions' => 'Funkcije proširenja izgleda',
'version-hook-name'                => 'Ime kuke',
'version-hook-subscribedby'        => 'Pretplaćeno od',
'version-version'                  => 'Inačica',
'version-license'                  => 'Licencija',
'version-software'                 => 'Instalirani softver',
'version-software-product'         => 'Proizvod',
'version-software-version'         => 'Verzija',

# Special:FilePath
'filepath'         => 'Putanja datoteke',
'filepath-page'    => 'Datoteka:',
'filepath-submit'  => 'Putanja',
'filepath-summary' => "Ova posebna stranica daje Vam kompletnu putanju do neke datoteke. Slike se na taj način prikazuju u punoj rezoluciji, drugi tipovi datoteka se otvaraju na klik (kako je već namješteno u vašem operacijskom sustavu).

Unesite ime datoteke bez predmetka (''prefiksa'') imenskog prostora \"{{ns:image}}:\".",

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Traži kopije datoteka',
'fileduplicatesearch-summary'  => 'Traži kopije datoteka na temelju njihove hash vrijednosti.

Unesite ime datoteke bez prefiksa "{{ns:image}}:"',
'fileduplicatesearch-legend'   => 'Traži kopije datoteka',
'fileduplicatesearch-filename' => 'Ime datoteke:',
'fileduplicatesearch-submit'   => 'Traži',
'fileduplicatesearch-info'     => '$1 × $2 piksela<br />Veličina datoteke: $3<br />MIME tip: $4',
'fileduplicatesearch-result-1' => 'Datoteka "$1" nema identičnih kopija.',
'fileduplicatesearch-result-n' => 'Datoteka "$1" ima {{PLURAL:$2|1 identičnu kopiju|$2 identične kopije}}.',

# Special:SpecialPages
'specialpages'                   => 'Posebne stranice',
'specialpages-note'              => '----
* Normalne posebne stranice
*<span class="mw-specialpagerestricted">Posebne stranice s ograničenim pristupom</span>',
'specialpages-group-maintenance' => 'Izvještaji za održavanje',
'specialpages-group-other'       => 'Ostale posebne stranice',
'specialpages-group-login'       => 'Prijava / Otvaranje računa',
'specialpages-group-changes'     => 'Nedavne promjene i evidencije',
'specialpages-group-media'       => 'Izvještaji i postavljanje datoteka',
'specialpages-group-users'       => 'Suradnici i suradnička prava',
'specialpages-group-highuse'     => 'Najčešće korištene stranice',
'specialpages-group-pages'       => 'Popisi stranica',
'specialpages-group-pagetools'   => 'Alati za stranice',
'specialpages-group-wiki'        => 'Wiki podaci i alati',
'specialpages-group-redirects'   => 'Preusmjeravajuće posebne stranice',
'specialpages-group-spam'        => 'Spam alati',

# Special:BlankPage
'blankpage'              => 'Prazna stranica',
'intentionallyblankpage' => 'Ova stranica je namjerno ostavljena praznom',

);
