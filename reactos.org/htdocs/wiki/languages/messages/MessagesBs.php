<?php
/** Bosnian (Bosanski)
 *
 * @ingroup Language
 * @file
 *
 * @author CERminator
 * @author Demicx
 * @author Kal-El
 * @author Seha
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => 'Medija',
	NS_SPECIAL          => 'Posebno',
	NS_MAIN             => '',
	NS_TALK             => 'Razgovor',
	NS_USER             => 'Korisnik',
	NS_USER_TALK        => 'Razgovor_sa_korisnikom',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Razgovor_{{grammar:instrumental|$1}}',
	NS_IMAGE            => 'Slika',
	NS_IMAGE_TALK       => 'Razgovor_o_slici',
	NS_MEDIAWIKI        => 'MedijaViki',
	NS_MEDIAWIKI_TALK   => 'Razgovor_o_MedijaVikiju',
	NS_TEMPLATE         => 'Šablon',
	NS_TEMPLATE_TALK    => 'Razgovor_o_šablonu',
	NS_HELP             => 'Pomoć',
	NS_HELP_TALK        => 'Razgovor_o_pomoći',
	NS_CATEGORY         => 'Kategorija',
	NS_CATEGORY_TALK    => 'Razgovor_o_kategoriji',
);

$skinNames = array(
	'Obična', 'Nostalgija', 'Kelnsko plavo'
);

$magicWords = array(
	# ID                              CASE SYNONYMS
	'redirect'               => array( 0, '#Preusmjeri', '#redirect', '#preusmjeri', '#PREUSMJERI' ),
	'notoc'                  => array( 0, '__NOTOC__', '__BEZSADRŽAJA__' ),
	'forcetoc'               => array( 0, '__FORCETOC__', '__FORSIRANISADRŽAJ__' ),
	'toc'                    => array( 0, '__TOC__', '__SADRŽAJ__' ),
	'noeditsection'          => array( 0, '__NOEDITSECTION__', '__BEZ_IZMENA__', '__BEZIZMENA__' ),
	'currentmonth'           => array( 1, 'CURRENTMONTH', 'TRENUTNIMJESEC' ),
	'currentmonthname'       => array( 1, 'CURRENTMONTHNAME', 'TRENUTNIMJESECIME' ),
	'currentmonthnamegen'    => array( 1, 'CURRENTMONTHNAMEGEN', 'TRENUTNIMJESECROD' ),
	'currentmonthabbrev'     => array( 1, 'CURRENTMONTHABBREV', 'TRENUTNIMJESECSKR' ),
	'currentday'             => array( 1, 'CURRENTDAY', 'TRENUTNIDAN' ),
	'currentdayname'         => array( 1, 'CURRENTDAYNAME', 'TRENUTNIDANIME' ),
	'currentyear'            => array( 1, 'CURRENTYEAR', 'TRENUTNAGODINA' ),
	'currenttime'            => array( 1, 'CURRENTTIME', 'TRENUTNOVRIJEME' ),
	'numberofarticles'       => array( 1, 'NUMBEROFARTICLES', 'BROJČLANAKA' ),
	'numberoffiles'          => array( 1, 'NUMBEROFFILES', 'BROJDATOTEKA', 'BROJFAJLOVA' ),
	'pagename'               => array( 1, 'PAGENAME', 'STRANICA' ),
	'pagenamee'              => array( 1, 'PAGENAMEE', 'STRANICE' ),
	'namespace'              => array( 1, 'NAMESPACE', 'IMENSKIPROSTOR' ),
	'namespacee'             => array( 1, 'NAMESPACEE', 'IMENSKIPROSTORI' ),
	'fullpagename'           => array( 1, 'FULLPAGENAME', 'PUNOIMESTRANE' ),
	'fullpagenamee'          => array( 1, 'FULLPAGENAMEE', 'PUNOIMESTRANEE' ),
	'msg'                    => array( 0, 'MSG:', 'POR:' ),
	'subst'                  => array( 0, 'SUBST:', 'ZAMJENI:' ),
	'msgnw'                  => array( 0, 'MSGNW:', 'NVPOR:' ),
	'img_thumbnail'          => array( 1, 'thumbnail', 'thumb', 'mini' ),
	'img_manualthumb'        => array( 1, 'thumbnail=$1', 'thumb=$1', 'mini=$1' ),
	'img_right'              => array( 1, 'right', 'desno', 'd' ),
	'img_left'               => array( 1, 'left', 'lijevo', 'l' ),
	'img_none'               => array( 1, 'none', 'n', 'bez' ),
	'img_width'              => array( 1, '$1px', '$1piksel' , '$1p' ),
	'img_center'             => array( 1, 'center', 'centre', 'centar', 'c' ),
	'img_framed'             => array( 1, 'framed', 'enframed', 'frame', 'okvir', 'ram' ),
	'sitename'               => array( 1, 'SITENAME', 'IMESAJTA' ),
	'ns'                     => array( 0, 'NS:', 'IP:' ),
	'localurl'               => array( 0, 'LOCALURL:', 'LOKALNAADRESA:' ),
	'localurle'              => array( 0, 'LOCALURLE:', 'LOKALNEADRESE:' ),
	'servername'             => array( 0, 'SERVERNAME', 'IMESERVERA' ),
	'scriptpath'             => array( 0, 'SCRIPTPATH', 'SKRIPTA' ),
	'grammar'                => array( 0, 'GRAMMAR:', 'GRAMATIKA:' ),
	'notitleconvert'         => array( 0, '__NOTITLECONVERT__', '__NOTC__', '__BEZTC__' ),
	'nocontentconvert'       => array( 0, '__NOCONTENTCONVERT__', '__NOCC__', '__BEZCC__' ),
	'currentweek'            => array( 1, 'CURRENTWEEK', 'TRENUTNASEDMICA' ),
	'currentdow'             => array( 1, 'CURRENTDOW', 'TRENUTNIDOV' ),
	'revisionid'             => array( 1, 'REVISIONID', 'IDREVIZIJE' ),
	'plural'                 => array( 0, 'PLURAL:', 'MNOŽINA:' ),
	'fullurl'                => array( 0, 'FULLURL:', 'PUNURL:' ),
	'fullurle'               => array( 0, 'FULLURLE:', 'PUNURLE:' ),
	'lcfirst'                => array( 0, 'LCFIRST:', 'LCPRVI:' ),
	'ucfirst'                => array( 0, 'UCFIRST:', 'UCPRVI:' ),
);

$fallback8bitEncoding = "iso-8859-2";
$separatorTransformTable = array(',' => '.', '.' => ',' );
$linkTrail = '/^([a-zćčžšđž]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Podvuci veze:',
'tog-highlightbroken'         => 'Formatiraj pokvarene veze <a href="" class="new">ovako</a> (alternativa: ovako<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Uravnjaj pasuse',
'tog-hideminor'               => 'Sakrij male izmjene u spisku nedavnih izmjena',
'tog-extendwatchlist'         => 'Proširi spisak praćenja za pogled svih izmjena',
'tog-usenewrc'                => 'Poboljšan spisak nedavnih izmjena (JavaScript)',
'tog-numberheadings'          => 'Automatski numeriši podnaslove',
'tog-showtoolbar'             => 'Prikaži dugmiće za izmjene (JavaScript)',
'tog-editondblclick'          => 'Izmijeni stranice dvostrukim klikom (JavaScript)',
'tog-editsection'             => 'Omogući da mijenjam pojedinačne odjeljke putem [uredi] linka',
'tog-editsectiononrightclick' => 'Uključite uređivanje odjeljka sa pritiskom na desno dugme miša u naslovu odjeljka (JavaScript)',
'tog-showtoc'                 => 'Prikaži sadržaj<br />(u svim stranicama sa više od tri podnaslova)',
'tog-rememberpassword'        => 'Zapamti šifru za iduće posjete',
'tog-editwidth'               => 'Kutija za uređivanje je dostigla najveću moguću širinu',
'tog-watchcreations'          => 'Dodaj stranice koje ja napravim u moj spisak praćenih članaka',
'tog-watchdefault'            => 'Dodaj stranice koje uređujem u moj spisak praćenih članaka',
'tog-watchmoves'              => 'Stranice koje premjestim dodaj na spisak praćenja',
'tog-watchdeletion'           => 'Stranice koje obrišem dodaj na spisak praćenja',
'tog-minordefault'            => 'Označi sve izmjene malim isprva',
'tog-previewontop'            => 'Prikaži pretpregled prije polja za izmjenu a ne posle',
'tog-previewonfirst'          => 'Prikaži izgled pri prvoj izmjeni',
'tog-nocache'                 => 'Onemogući keširanje stranica',
'tog-enotifwatchlistpages'    => 'Pošalji mi e-poštu kad se promijene stranice',
'tog-enotifusertalkpages'     => 'Pošalji mi e-poštu kad se promijeni moja korisnička stranica za razgovor',
'tog-enotifminoredits'        => 'Pošalji mi e-poštu takođe za male izmjene stranica',
'tog-enotifrevealaddr'        => 'Otkrij adresu moje e-pošte u porukama obaviještenja',
'tog-shownumberswatching'     => 'Prikaži broj korisnika koji prate',
'tog-fancysig'                => 'Jednostavan potpis (bez automatskog linka)',
'tog-externaleditor'          => 'Po potrebi koristite vanjski program za uređivanje (samo za naprednije korisnike, potrebne su promjene na računaru)',
'tog-externaldiff'            => 'Koristi vanjski (diff) program za prikaz razlika',
'tog-showjumplinks'           => 'Omogući "skoči na" poveznice',
'tog-uselivepreview'          => 'Koristite pregled uživo (JavaScript) (Eksperimentalno)',
'tog-forceeditsummary'        => 'Opomeni me pri unosu praznog sažetka',
'tog-watchlisthideown'        => 'Sakrij moje izmjene sa spiska praćenih članaka',
'tog-watchlisthidebots'       => 'Sakrij izmjene botova sa spiska praćenih članaka',
'tog-watchlisthideminor'      => 'Sakrij zanemarljive izmjene sa spiska mojih praćenja',
'tog-ccmeonemails'            => 'Pošalji mi kopije emailova koje pošaljem drugim korisnicima',
'tog-diffonly'                => 'Ne prikazuj sadržaj stranice ispod prikaza razlika',
'tog-showhiddencats'          => 'Prikaži skrivene kategorije',

'underline-always'  => 'Uvijek',
'underline-never'   => 'Nikad',
'underline-default' => 'Po podešavanjima brauzera',

'skinpreview' => '(Pregled)',

# Dates
'sunday'        => 'nedelja',
'monday'        => 'ponedeljak',
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
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'mart',
'april'         => 'april',
'may_long'      => 'maj',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'august',
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
'august-gen'    => 'augusta',
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
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorija|Kategorije}}',
'category_header'                => 'Članaka u kategoriji "$1"',
'subcategories'                  => 'Potkategorije',
'category-media-header'          => 'Mediji u kategoriji "$1"',
'category-empty'                 => "''Ova kategorija trenutno ne sadrži članke ni medije.''",
'hidden-categories'              => '{{PLURAL:$1|Sakrivena kategorija|Sakrivene kategorije}}',
'hidden-category-category'       => 'Skrivene kategorije', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Ova kategorija ima samo sljedeću podkategoriju.|Ova kategorija ima sljedeću {{PLURAL:$1|podkategoriju|$1 podkategorije}}, od $2 ukupno.}}',
'category-subcat-count-limited'  => 'Ova kategorija sadrži {{PLURAL:$1|podkategoriju|$1 podkategorije|$1 podkategorija}}.',
'category-article-count'         => '{{PLURAL:$2|U ovoj kategoriji se nalazi ovaj članak.|Prikazano je {{PLURAL:$1|članak|$1 članka|$1 članaka}} od ukupno $2 u ovoj kategoriji.}}',
'category-article-count-limited' => '{{PLURAL:$1|Slijedeća stranica|Slijedećih $1 stranica}} je u ovoj kategoriji.',
'category-file-count'            => '{{PLURAL:$2|Ova kategorija ima samo slijedeću datoteku.|Prikazano je {{PLURAL:$1|$1 datoteka|$1 datoteke|$1 datoteka}} u ovoj kategoriji, od ukupno $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Slijedeća datoteka je|Slijedeće $1 datoteke su|Slijedećih $1 datoteka je}} u ovoj kategoriji.',
'listingcontinuesabbrev'         => 'nast.',

'mainpagetext'      => 'Viki softver is uspješno instaliran.',
'mainpagedocfooter' => 'Kontaktirajte [http://meta.wikimedia.org/wiki/Help:Contents uputstva za korisnike] za informacije o upotrebi wiki programa.

== Početak ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lista postavki]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki najčešće postavljana pitanja]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lista E-Mail adresa MediaWiki]',

'about'          => 'O...',
'article'        => 'Članak',
'newwindow'      => '(otvara se u novom prozoru)',
'cancel'         => 'Poništite',
'qbfind'         => 'Pronađite',
'qbbrowse'       => 'Prelistajte',
'qbedit'         => 'Izmjenite',
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
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Greška',
'returnto'          => 'Povratak na $1.',
'tagline'           => 'Izvor: {{SITENAME}}',
'help'              => 'Pomoć',
'search'            => 'Pretraga',
'searchbutton'      => 'Traži',
'go'                => 'Idi',
'searcharticle'     => 'Idi',
'history'           => 'Historija stranice',
'history_short'     => 'Historija',
'updatedmarker'     => 'promjene od moje zadnje posjete',
'info_short'        => 'Informacija',
'printableversion'  => 'Prilagođeno štampanju',
'permalink'         => 'Trajni link',
'print'             => 'Štampa',
'edit'              => 'Uredi',
'create'            => 'Napravi',
'editthispage'      => 'Uredite ovu stranicu',
'create-this-page'  => 'Napravi ovu stranicu',
'delete'            => 'Obriši',
'deletethispage'    => 'Obriši ovu stranicu',
'undelete_short'    => 'Vrati obrisanih {{PLURAL:$1|jednu izmjenu|$1 izmjena}}',
'protect'           => 'Zaštitite',
'protect_change'    => 'promijeni zaštitu',
'protectthispage'   => 'Zaštitite ovu stranicu',
'unprotect'         => 'odštiti',
'unprotectthispage' => 'Odštiti ovu stranicu',
'newpage'           => 'Nova stranica',
'talkpage'          => 'Razgovor o stranici',
'talkpagelinktext'  => 'Razgovor',
'specialpage'       => 'Posebna Stranica',
'personaltools'     => 'Lični alati',
'postcomment'       => 'Pošaljite komentar',
'articlepage'       => 'Pogledaj članak',
'talk'              => 'Razgovor',
'views'             => 'Pregledi',
'toolbox'           => 'Traka sa alatima',
'userpage'          => 'Pogledaj korisničku stranicu',
'projectpage'       => 'Pogledaj stranu o ovoj strani',
'imagepage'         => 'Pogledajte stranicu slike',
'mediawikipage'     => 'Pogledaj stranicu sa porukama',
'templatepage'      => 'Pogledajte stranicu za šablone',
'viewhelppage'      => 'Pogledajte stranicu za pomoć',
'categorypage'      => 'Pogledaj stranicu kategorije',
'viewtalkpage'      => 'Pogledaj raspravu',
'otherlanguages'    => 'Ostali jezici',
'redirectedfrom'    => '(Preusmjereno sa $1)',
'redirectpagesub'   => 'Preusmjeri stranicu',
'lastmodifiedat'    => 'Ova stranica je posljednji put izmijenjena $2, $1', # $1 date, $2 time
'viewcount'         => 'Ovoj stranici je pristupljeno {{PLURAL:$1|jednom|$1 puta}}.',
'protectedpage'     => 'Zaštićena stranica',
'jumpto'            => 'Idi na:',
'jumptonavigation'  => 'navigacija',
'jumptosearch'      => 'traži',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'O projektu {{SITENAME}}',
'aboutpage'            => 'Project:O_projektu_{{SITENAME}}',
'bugreports'           => 'Prijavite grešku',
'bugreportspage'       => 'Project:Prijave_grešaka',
'copyright'            => 'Svi sadržaji podliježu "$1" licenci.',
'copyrightpagename'    => '{{SITENAME}} autorska prava',
'copyrightpage'        => '{{ns:project}}:Autorska_prava',
'currentevents'        => 'Trenutni događaji',
'currentevents-url'    => 'Project:Novosti',
'disclaimers'          => 'Odricanje odgovornosti',
'disclaimerpage'       => 'Project:Uslovi korišćenja, pravne napomene i odricanje odgovornosti',
'edithelp'             => 'Pomoć pri uređivanju stranice',
'edithelppage'         => 'Help:Uređivanje',
'faq'                  => 'ČPP',
'faqpage'              => 'Project:NPP',
'helppage'             => 'Help:Sadržaj',
'mainpage'             => 'Početna strana',
'mainpage-description' => 'Početna strana',
'policy-url'           => 'Projekt:Pravila',
'portal'               => 'Portal zajednice',
'portal-url'           => 'Project:Portal_zajednice',
'privacy'              => 'Pravila o anonimnosti',
'privacypage'          => 'Project:Pravila o anonimnosti',

'badaccess'        => 'Greška pri odobrenju',
'badaccess-group0' => 'Nije vam dozvoljeno izvršiti akciju koju ste zahtjevali.',
'badaccess-group1' => 'Akcija koju ste htjeli napraviti je ograničena za korisnike grupe $1.',
'badaccess-group2' => 'Akcija koju ste htjeli napraviti je ograničena za korisnike iz jedne od grupa $1.',

'versionrequired'     => 'Potrebna je verzija $1 MediaWikija',
'versionrequiredtext' => 'Potrebna je verzija $1 MediaWikija da bi se koristila ova strana. Pogledaj [[Special:Version|verziju]].',

'ok'                      => 'da',
'retrievedfrom'           => 'Dobavljeno iz "$1"',
'youhavenewmessages'      => 'Imate $1 ($2).',
'newmessageslink'         => 'novih poruka',
'newmessagesdifflink'     => 'posljednja promjena',
'youhavenewmessagesmulti' => 'Imate nove poruke na $1',
'editsection'             => 'uredi',
'editold'                 => 'uredi',
'viewsourceold'           => 'pogledaj izvor',
'editsectionhint'         => 'Uredi sekciju: $1',
'toc'                     => 'Sadržaj',
'showtoc'                 => 'prikaži',
'hidetoc'                 => 'sakrij',
'thisisdeleted'           => 'Pogledaj ili vrati $1?',
'viewdeleted'             => 'Pogledaj $1?',
'restorelink'             => '{{PLURAL:$1|jedna izbrisana izmjena|$1 izbrisanih izmjena}}',
'feedlinks'               => 'Fid:',
'feed-invalid'            => 'Nedozvoljen tip potpisa',
'feed-unavailable'        => 'RSS izvori se ne nalaze na {{SITENAME}}',
'site-rss-feed'           => '$1 RSS izvor',
'site-atom-feed'          => '$1 Atom izvor',
'page-rss-feed'           => '"$1" RSS izvor',
'page-atom-feed'          => '"$1" Atom izvor',
'red-link-title'          => '$1 (nije još napisan)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Članak',
'nstab-user'      => 'Korisnička stranica',
'nstab-media'     => 'Mediji',
'nstab-special'   => 'Posebna',
'nstab-project'   => 'Članak',
'nstab-image'     => 'Slika',
'nstab-mediawiki' => 'Poruka',
'nstab-template'  => 'Šablon',
'nstab-help'      => 'Pomoć',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Nema takve akcije',
'nosuchactiontext'  => 'Akcija navedena u URL-u nije
prepoznata od strane {{SITENAME}} softvera.',
'nosuchspecialpage' => 'Nema takve posebne stranice',
'nospecialpagetext' => 'Tražili ste posebnu stranicu, koju {{SITENAME}} softver nije prepoznao.',

# General errors
'error'                => 'Greška',
'databaseerror'        => 'Greška u bazi',
'dberrortext'          => 'Desila se sintaksna greška upita baze.
Ovo je moguće zbog ilegalnog upita, ili moguće greške u softveru.
Poslednji pokušani upit je bio: <blockquote><tt>$1</tt></blockquote>
iz funkcije "<tt>$2</tt>".
MySQL je vratio grešku "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Desila se sintaksna greška upita baze.
Poslednji pokušani upit je bio:
"$1"
iz funkcije "$2".
MySQL je vratio grešku "$3: $4".',
'noconnect'            => 'Žao nam je! Viki ima neke tehničke poteškoće, i ne može da se poveže sa serverom baze.<br />
$1',
'nodb'                 => 'Ne mogu da izaberem bazu $1',
'cachederror'          => 'Ovo je keširana kopija zahtjevane stranice, i možda nije najnovija.',
'laggedslavemode'      => "'''Upozorenje''': Stranica, možda, nije ažurirana.",
'readonly'             => 'Baza je zaključana',
'enterlockreason'      => 'Unesite razlog za zaključavanje, uključujući procijenu
vremena otključavanja',
'readonlytext'         => 'Baza je trenutno zaključana za nove unose i ostale izmjene, vjerovatno zbog rutinskog održavanja, posle čega će biti vraćena u uobičajeno stanje.

Administrator koji ju je zaključao je ponudio ovo objašnjenje: $1',
'missing-article'      => 'U bazi podataka nije pronađen tekst stranice tražen pod nazivom "$1" $2.

Do ovoga dolazi kada se prati premještaj ili historija linka za stranicu koja je pobrisana.


U slučaju da se ne radi o gore navedenom, moguće je da ste pronašli grešku u programu.
Molimo Vas da ovo prijavite [[Special:ListUsers/sysop|administratoru]] sa navođenjem tačne adrese stranice',
'missingarticle-rev'   => '(revizija#: $1)',
'missingarticle-diff'  => '(Razlika: $1, $2)',
'readonly_lag'         => 'Baza podataka je zaključana dok se sekundarne baze podataka na serveru ne sastave sa glavnom.',
'internalerror'        => 'Interna greška',
'internalerror_info'   => 'Interna greška: $1',
'filecopyerror'        => 'Ne može se kopirati "$1" na "$2".',
'filerenameerror'      => 'Ne može se promjeniti ime fajla "$1" to "$2".',
'filedeleteerror'      => 'Ne može se izbrisati fajl "$1".',
'directorycreateerror' => 'Nije moguće napraviti direkciju "$1".',
'filenotfound'         => 'Ne može se naći fajl "$1".',
'fileexistserror'      => 'Nemoguće je napisati fajl "$1": fajl već postoji',
'unexpected'           => 'Neočekivana vrijednost: "$1"="$2".',
'formerror'            => 'Greška:  ne može se poslati upitnik',
'badarticleerror'      => 'Ova akcija ne može biti izvršena na ovoj stranici.',
'cannotdelete'         => 'Ne može se obrisati navedena stranica ili slika.  (Moguće je da ju je neko drugi već obrisao.)',
'badtitle'             => 'Loš naslov',
'badtitletext'         => 'Zahtjevani naslov stranice je bio neispravan, prazan ili neispravno povezan međujezički ili interviki naslov.',
'perfdisabled'         => 'Žao nam je!  Ova mogućnost je privremeno onemogućena jer usporava bazu do te mjere da više niko ne može da koristi viki.',
'perfcached'           => 'Sledeći podaci su keširani i možda neće biti u potpunosti ažurirani:',
'perfcachedts'         => 'Sljedeći podaci se nalaze u memoriji i zadnji put su ažurirani $1.',
'querypage-no-updates' => 'Ažuriranje ove stranice je isključeno.
Podaci koji se ovdje nalaze ne moraju biti aktualni.',
'wrong_wfQuery_params' => 'Netačni parametri za wfQuery()<br />
Funkcija: $1<br />
Pretraga: $2',
'viewsource'           => 'pogledaj kod',
'viewsourcefor'        => 'za $1',
'actionthrottled'      => 'Akcija je usporena',
'actionthrottledtext'  => 'Kao anti-spam mjera, ograničene su vam izmjene u određenom vremenu, i trenutačno ste dostigli to ograničenje. Pokušajte ponovo poslije nekoliko minuta.',
'protectedpagetext'    => 'Ova stranica je zaključana da bi se spriječile izmjene.',
'viewsourcetext'       => 'Možete vidjeti i kopirati izvorni tekst ove stranice:',
'protectedinterface'   => 'Ova stranica je zaštićena jer sadrži tekst MediaWiki programa.',
'editinginterface'     => "'''Upozorenje:''' Vi meijenjate stranicu koja sadrzi aktivan tekst programa.
Promjene na ovoj stranici dovode i do promjena za druge korisnike.
Za prevode, molimo Vas koristite [http://translatewiki.net/wiki/Main_Page?setlang=en Betawiki], projekt prijevoda za MediaWiki.",
'sqlhidden'            => '(SQL pretraga sakrivena)',
'cascadeprotected'     => 'Uređivanje ove sranice je zabranjeno jer sadrži {{PLURAL:$1|stranicu zaštićeu|stranice zaštićene}} od uređivanja iz razloga:
$2',
'namespaceprotected'   => "Vi nemate dozvulu da mijenjate stranicu '''$1'''.",
'customcssjsprotected' => 'Nemate dozvolu za mijenjanje ove stranice jer sadrži osobne postavke nekog drugog korisnika.',
'ns-specialprotected'  => 'Specijalne stranice se ne mogu uređivati.',
'titleprotected'       => "Naslov stranice je zaštićen od postavljanja od [[User:$1|$1]].
Iz razloga ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Loša konfiguracija: nepoznati anti-virus program: <i>$1</i>',
'virus-scanfailed'     => 'kontrolisani fajlovi (code $1)',
'virus-unknownscanner' => 'nepoznati anti-virus program:',

# Login and logout pages
'logouttitle'             => 'Odjavite se',
'logouttext'              => '<strong>Sad ste odjavljeni.</strong><br />
Možete nastaviti da koristite {{SITENAME}} anonimno, ili se ponovo prijaviti
kao isti ili kao drugi korisnik.  Obratite pažnju da neke stranice mogu nastaviti da se prikazuju kao da ste još uvijek prijavljeni, dok ne očistite keš svog brauzera.',
'welcomecreation'         => '<h2>Dobro došli, $1!</h2><p>Vaš nalog je napravljen.
Ne zaboravite da prilagodite sebi svoja podešavanja.',
'loginpagetitle'          => 'Prijavljivanje',
'yourname'                => 'Korisničko ime',
'yourpassword'            => 'Lozinka',
'yourpasswordagain'       => 'Ponovite lozinku',
'remembermypassword'      => 'Zapamti šifru za iduće posjete',
'yourdomainname'          => 'Vaš domen',
'loginproblem'            => '<b>Bilo je problema sa vašim prijavljivanjem.</b><br />Probajte ponovo!',
'login'                   => 'Prijavi se',
'nav-login-createaccount' => 'Prijavi se / Registruj se',
'loginprompt'             => "Morate imati kolačiće ('''cookies''') omogućene da biste se prijavili na {{SITENAME}}.",
'userlogin'               => 'Prijavi se / Registruj se',
'logout'                  => 'Odjavi me',
'userlogout'              => 'Odjavi me',
'notloggedin'             => 'Niste prijavljeni',
'nologinlink'             => 'Napravite nalog',
'createaccount'           => 'Napravi nalog',
'gotaccount'              => 'Imate nalog? $1.',
'gotaccountlink'          => 'Prijavi se',
'createaccountmail'       => 'e-poštom',
'badretype'               => 'Lozinke koje ste unijeli se ne poklapaju.',
'userexists'              => 'Korisničko ime koje ste unijeli je već u upotrebi.  Molimo Vas da izaberete drugo ime.',
'youremail'               => 'E-pošta *',
'username'                => 'Korisničko ime:',
'uid'                     => 'Korisnički ID:',
'yourrealname'            => 'Vaše pravo ime *',
'yourlanguage'            => 'Jezik:',
'yournick'                => 'Nadimak (za potpise):',
'email'                   => 'E-mail',
'loginerror'              => 'Greška pri prijavljivanju',
'prefs-help-email'        => '* E-mail (optional): Enables others to contact you through your user or user_talk page without the need of revealing your identity.',
'nocookiesnew'            => "Korisnički nalog je napravljen, ali niste prijavljeni.  {{SITENAME}} koristi kolačiće (''cookies'') da bi se korisnici prijavili.  Vi ste onemogućili kolačiće na Vašem kompjuteru.  molimo Vas da ih omogućite, a onda se prijavite sa svojim novim korisničkim imenom i lozinkom.",
'nocookieslogin'          => "{{SITENAME}} koristi kolačiće (''cookies'') da bi se korisnici prijavili.  Vi ste onemogućili kolačiće na Vašem kompjuteru.  Molimo Vas da ih omogućite i da pokušate ponovo sa prijavom.",
'noname'                  => 'Niste izabrali ispravno korisničko ime.',
'loginsuccesstitle'       => 'Prijavljivanje uspješno',
'loginsuccess'            => "'''Sad ste prijavljeni na {{SITENAME}} kao \"\$1\".'''",
'nosuchuser'              => 'Ne postoji korisnik sa imenom "$1". Provjerite Vaše kucanje, ili upotrebite donji upitnik da napravite novi korisnički nalog.',
'wrongpassword'           => 'Unijeli ste neispravnu lozinku.  Molimo Vas da pokušate ponovo.',
'wrongpasswordempty'      => 'Lozinka je bila prazna.  Molimo Vas da pokušate ponovo.',
'passwordtooshort'        => 'Vaša šifra je prekratka.
Šifra mora imati najmanje {{PLURAL:$1|1 znak|$1 znakova}} i mora se razlikovati od Vašeg korisničkog imena.',
'mailmypassword'          => 'Pošalji mi moju lozinku',
'passwordremindertitle'   => '{{SITENAME}} podsjetnik za lozinku',
'passwordremindertext'    => 'Neko (vjerovatno Vi, sa IP adrese $1)
je zahtjevao da vam pošaljemo novu {{SITENAME}} lozinku za prijavljivanje na {{SERVERNAME}}.
Lozinka za korisnika "$2" je sad "$3".
Sad treba da se prijavite i promjenite lozinku.

Ako je neko drugi napravio ovaj zahtjev ili ako ste se sjetili vaše lozinke i
ne želite više da je promjenite, možete da ignorišete ovu poruku i da nastavite koristeći
vašu staru lozinku.',
'noemail'                 => 'Ne postoji adresa e-pošte za korisnika "$1".',
'passwordsent'            => 'Nova lozinka je poslata na adresu e-pošte
korisnika "$1".
Molimo Vas da se prijavite pošto je primite.',
'blocked-mailpassword'    => 'Da bi se spriječila nedozvoljena akcija, vašoj IP adresi je onemogućeno uređivanje stranica kao i mogućnost zahtijevanje nove lozinke.',
'mailerror'               => 'Greška pri slanju e-pošte: $1',
'emailconfirmlink'        => 'Potvrdite Vašu e-mail adresu',
'accountcreated'          => 'Korisnički račun je napravljen',
'accountcreatedtext'      => 'Korisnički račun za $1 je napravljen.',
'loginlanguagelabel'      => 'Jezik: $1',

# Password reset dialog
'resetpass' => 'Resetuj korisničku lozinku',

# Edit page toolbar
'bold_sample'     => 'Podebljan tekst',
'bold_tip'        => 'Podebljan tekst',
'italic_sample'   => 'Kurzivan tekst',
'italic_tip'      => 'Kurzivan tekst',
'link_sample'     => 'Naslov poveznice',
'link_tip'        => 'Unutrašnja poveznica',
'extlink_sample'  => 'http://www.example.com opis adrese',
'extlink_tip'     => 'Spoljašnja poveznica (zapamti prefiks http://)',
'headline_sample' => 'Naslov',
'headline_tip'    => 'Podnaslov',
'math_sample'     => 'Unesite formulu ovdje',
'math_tip'        => 'Matematička formula (LaTeX)',
'nowiki_sample'   => 'Dodaj neformatirani tekst ovdje',
'nowiki_tip'      => 'Ignoriši viki formatiranje teksta',
'image_sample'    => 'ime_slike.jpg',
'image_tip'       => 'Uklopljena slika',
'media_sample'    => 'ime_medija_fajla.ogg',
'media_tip'       => 'Putanja ka multimedijalnom fajlu',
'sig_tip'         => 'Vaš potpis sa trenutnim vremenom',
'hr_tip'          => 'Horizontalna linija (koristite oskudno)',

# Edit pages
'summary'                  => 'Sažetak',
'subject'                  => 'Tema/naslov',
'minoredit'                => 'Ovo je mala izmjena',
'watchthis'                => 'Prati ovaj članak',
'savearticle'              => 'Sačuvaj',
'preview'                  => 'Pregled stranice',
'showpreview'              => 'Prikaži izgled',
'showdiff'                 => 'Prikaži izmjene',
'anoneditwarning'          => 'Niste prijavljeni. Vaša IP adresa će biti zapisana.',
'blockedtitle'             => 'Korisnik je blokiran',
'blockedtext'              => "<big>'''Vaše korisničko ime ili IP adresa je blokirana.'''</big>

Blokada izvršena od strane $1.
Dati razlog je slijedeći: ''$2''.

*Početak blokade: $8
*Kraj perioda blokade: $6
*Ime blokiranog korisnika: $7

Možete kontaktirati $1 ili nekog drugog [[{{MediaWiki:Grouppage-sysop}}|administratora]] da biste razgovarali o blokadi.

Ne možete koristiti opciju ''Pošalji e-mail korisniku'' osim ako niste unijeli e-mail adresu u [[Special:Preferences|Vaše postavke]].
Vaša trenutna IP adresa je $3, a oznaka blokade je #$5.
Molimo Vas da navedete gornje podatke pri zahtjevu za deblokadu.",
'blockednoreason'          => 'razlog nije naveden',
'whitelistedittitle'       => 'Obavezno je prijavljivanje za uređivanje',
'whitelistedittext'        => 'Morate da se [[Special:UserLogin|prijavite]] da bi ste uređivali stranice.',
'loginreqtitle'            => 'Potrebno je prijavljivanje',
'accmailtitle'             => 'Lozinka poslata.',
'accmailtext'              => "Lozinka za nalog '$1' je poslata na adresu $2.",
'newarticle'               => '(Novi)',
'newarticletext'           => "'''Došli ste na stranicu koja još nema sadržaja.'''<br />
*Ako želite unijeti sadržaj, počnite tipkati u prozor ispod ovog teksta.
*Ako vam treba pomoć, idite na [[{{MediaWiki:Helppage}}|stranicu za pomoć]].
*Ako ste ovamo dospjeli slučajno, kliknite dugme \"Nazad\" (''Back'') u svom internet pregledaču.",
'anontalkpagetext'         => "----''Ovo je stranica za razgovor za anonimnog korisnika koji još nije napravio nalog ili ga ne koristi.  Zbog toga moramo da koristimo brojčanu IP adresu kako bismo odentifikovali njega ili nju.  Takvu adresu može dijeliti više korisnika.  Ako ste anonimni korisnik i mislite da su vam upućene nebitne primjedbe, molimo Vas da [[Special:UserLogin|napravite nalog ili se prijavite]] da biste izbjegli buduću zabunu sa ostalim anonimnim korisnicima.''",
'noarticletext'            => "<div style=\"border: 1px solid #ccc; padding: 7px;\">'''{{SITENAME}} još nema ovaj članak.'''
* Da započnete članak, kliknite '''[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} uredite ovu stranicu]'''.
* [[Special:Search/{{PAGENAME}}|Pretraži {{PAGENAME}}]] u ostalim člancima
* [[Special:WhatLinksHere/{{NAMESPACE}}{{PAGENAME}}|Stranice koje su povezane za]] {{PAGENAME}} članak
----
* '''Ukoliko ste napravili ovaj članak u poslednjih nekoliko minuta i još se nije pojavio, postoji mogućnost da je server u zastoju zbog osvježavanja baze podataka.''' Molimo Vas da probate sa <span class=\"plainlinks\">[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=purge}} osvježavanjem]<span> ili sačekajte i provjerite kasnije ponovo prije ponovnog pravljenja članka.
* Ako ste napravili članak pod ovim imenom ranije, moguće je da je bio izbrisan.  Potražite '''{{FULLPAGENAME}}''' [{{fullurl:Special:Log|type=delete&page={{FULLPAGENAMEE}}}} u spisku brisanja].",
'usercssjsyoucanpreview'   => "<strong>Pažnja:</strong> Koristite 'Prikaži izgled' dugme da testirate svoj novi CSS/JS prije nego što sačuvate.",
'usercsspreview'           => "'''Zapamtite ovo je samo izgled vašeg CSS-a, još uvijek nije sačuvan!'''",
'userjspreview'            => "'''Zapamtite ovo je samo izgled vaše JavaScript-e, još uvijek nije sačuvan!'''",
'updated'                  => '(Osvježeno)',
'note'                     => '<strong>Pažnja:</strong>',
'previewnote'              => '<strong>Ovo je samo pregled; izmjene stranice nisu još sačuvane!</strong>',
'previewconflict'          => 'Ovaj pregled reflektuje tekst u gornjem polju
kako će izgledati ako pritisnete "Sačuvaj članak".',
'editing'                  => 'Uređujete $1',
'editingsection'           => 'Uređujete $1 (dio)',
'editconflict'             => 'Sukobljenje izmjene: $1',
'explainconflict'          => 'Neko drugi je promjenio ovu stranicu otkad ste Vi počeli da je mjenjate.
Gornje tekstualno polje sadrži tekst stranice koji trenutno postoji.
Vaše izmjene su prikazane u donjem tekstu.
Moraćete da unesete svoje promjene u postojeći tekst.
<b>Samo</b> tekst u gornjem tekstualnom polju će biti snimljen kad
pritisnete "Sačuvaj".<br />',
'yourtext'                 => 'Vaš tekst',
'storedversion'            => 'Uskladištena verzija',
'editingold'               => '<strong>PAŽNJA:  Vi mijenjate stariju
reviziju ove stranice.
Ako je snimite, sve promjene učinjene od ove revizije će biti izgubljene.</strong>',
'yourdiff'                 => 'Razlike',
'copyrightwarning'         => 'Za sve priloge poslate na projekat {{SITENAME}} smatramo da su objavljeni pod $2 (konsultujte $1 za detalje).
Ukoliko ne želite da vaši članci budu podložni izmjenama i slobodnom rasturanju i objavljivanju,
nemojte ih slati ovdje. Takođe, slanje članka podrazumijeva i vašu izjavu da ste ga napisali sami, ili da ste ga kopirali iz izvora u javnom domenu ili sličnog slobodnog izvora.

<strong>NEMOJTE SLATI RAD ZAŠTIĆEN AUTORSKIM PRAVIMA BEZ DOZVOLE AUTORA!</strong>',
'longpagewarning'          => '<strong>PAŽNJA: Ova stranica ima $1 kilobajta; neki
preglednici mogu imati problema kad uređujete stranice skoro ili veće od 32 kilobajta.
Molimo Vas da razmotrite razbijanje stranice na manje dijelove.</strong>',
'readonlywarning'          => '<strong>PAŽNJA:  Baza je zaključana zbog održavanja,
tako da nećete moći da sačuvate svoje izmjene za sada.  Možda želite da kopirate
i nalijepite tekst u tekst editor i sačuvate ga za kasnije.</strong>',
'protectedpagewarning'     => '<strong>PAŽNJA: Ova stranica je zaključana tako da samo korisnici sa administratorskim privilegijama mogu da je mijenjaju.</strong>',
'semiprotectedpagewarning' => "'''Pažnja:''' Ova stranica je zaključana tako da je samo registrovani korisnici mogu uređivati.",
'templatesused'            => 'Šabloni koji su upotrebljeni na ovoj stranici:',
'template-protected'       => '(zaštićeno)',
'template-semiprotected'   => '(polu-zaštićeno)',
'nocreatetext'             => 'Na {{SITENAME}} je zabranjeno postavljanje novih stranica. 
Možete se vratiti i uređivati već postojeće stranice ili se [[Special:UserLogin|prijaviti ili otvoriti korisnički račun]].',
'recreate-deleted-warn'    => "'''Upozorenje: Postavljate stranicu koja je prethodno brisana.'''

Razmotrite je li nastavljanje uređivanja ove stranice u skladu s pravilima. Za vašu informaciju slijedi evidencija brisanja s obrazloženjem za prethodno brisanje:",

# History pages
'viewpagelogs'     => 'Pogledaj protokol ove stranice',
'nohistory'        => 'Ne postoji istorija izmjena za ovu stranicu.',
'revnotfound'      => 'Revizija nije pronađena',
'revnotfoundtext'  => 'Starija revizija ove stranice koju ste zatražili nije nađena.
Molimo Vas da provjerite URL pomoću kojeg ste pristupili ovoj stranici.',
'currentrev'       => 'Trenutna revizija',
'revisionasof'     => 'Revizija od $1',
'previousrevision' => '←Starije izmjene',
'cur'              => 'tren',
'next'             => 'sled',
'last'             => 'posl',
'histlegend'       => 'Objašnjenje: (tren) = razlika sa trenutnom verziom,
(posl) = razlika sa prethodnom verziom, M = mala izmjena',
'histfirst'        => 'Najstarije',
'histlast'         => 'Najnovije',
'historyempty'     => '(prazno)',

# Diffs
'history-title'           => 'Historija izmjena stranice "$1"',
'difference'              => '(Razlika između revizija)',
'lineno'                  => 'Linija $1:',
'compareselectedversions' => 'Uporedite označene verzije',
'editundo'                => 'ukloni ovu izmjenu',

# Search results
'searchresults'         => 'Rezultati pretrage',
'searchresulttext'      => 'Za više informacija o pretraživanju {{SITENAME}}, pogledajte [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "Tražili ste '''[[:$1]]'''",
'searchsubtitleinvalid' => 'Tražili ste $1',
'noexactmatch'          => "Nema stranice sa imenom \"\$1\".

Možete '''[[:\$1|da napravite članak sa tim naslovom]]''' ili [[{{MediaWiki:Helppage}}|da stavite zahtjev za ovaj članak]] ili [[Special:Allpages/\$1|potražite na drugim stranicama]].

::*'''''<u>Opomena: Nemojte da kopirate materijale za koje nemate dozvolu!</u>'''''",
'titlematches'          => 'Naslov članka odgovara',
'notitlematches'        => 'Naslov članka ne odgovara.',
'textmatches'           => 'Tekst stranice odgovara',
'notextmatches'         => 'Tekst članka ne odgovara',
'prevn'                 => 'prethodnih $1',
'nextn'                 => 'sledećih $1',
'viewprevnext'          => 'Pogledaj ($1) ($2) ($3).',
'showingresults'        => 'Prikazani su <b>$1</b> rezultata počev od <b>$2</b>.',
'showingresultsnum'     => 'Prikazani su <b>$3</b> rezultati počev od <b>$2</b>.',
'nonefound'             => "'''Pažnja''': neuspješne pretrage su
često izazvane traženjem čestih riječi kao \"je\" ili \"od\",
koje nisu indeksirane, ili navođenjem više od jednog izraza za traženje (samo stranice
koje sadrže sve izraze koji se traže će se pojaviti u rezultatima).",
'powersearch'           => 'Traži',
'searchdisabled'        => '<p>Izvinjavamo se!  Puno pretraga teksta je privremeno onemogućena.  U međuvremenu, možete koristiti Google za pretragu.  Indeks može biti stariji.',

# Preferences page
'preferences'             => 'Podešavanja',
'mypreferences'           => 'Moje postavke',
'prefsnologin'            => 'Niste prijavljeni',
'prefsnologintext'        => 'Morate biti [[Special:UserLogin|prijavljeni]] da biste podešavali korisnička podešavanja.',
'prefsreset'              => 'Podešavanja su vraćena na prvotne vrijednosti.',
'qbsettings'              => 'Podešavanja brze palete',
'qbsettings-none'         => 'Nikakva',
'qbsettings-fixedleft'    => 'Pričvršćena lijevo',
'qbsettings-fixedright'   => 'Pričvršćena desno',
'qbsettings-floatingleft' => 'Plutajuća lijevo',
'changepassword'          => 'Promjeni lozinku',
'skin'                    => 'Koža',
'math'                    => 'Prikazivanje matematike',
'dateformat'              => 'Format datuma',
'datedefault'             => 'Nije bitno',
'math_failure'            => 'Neuspjeh pri parsiranju',
'math_unknown_error'      => 'nepoznata greška',
'math_unknown_function'   => 'nepoznata funkcija',
'math_lexing_error'       => 'riječnička greška',
'math_syntax_error'       => 'sintaksna greška',
'math_image_error'        => 'PNG konverzija neuspješna; provjerite tačnu instalaciju latex-a, dvips-a, gs-a i convert-a',
'math_bad_tmpdir'         => 'Ne može se napisati ili napraviti privremeni matematični direktorijum',
'math_bad_output'         => 'Ne može se napisati ili napraviti direktorijum za matematični izvještaj.',
'math_notexvc'            => 'Nedostaje izvršno texvc; molimo Vas da pogledate math/README da podesite.',
'prefs-personal'          => 'Korisnički podaci',
'prefs-rc'                => 'Podešavanja nedavnih izmjena',
'prefs-watchlist'         => 'Praćeni članci',
'prefs-misc'              => 'Ostala podešavanja',
'saveprefs'               => 'Sačuvajte podešavanja',
'resetprefs'              => 'Vrati podešavanja',
'oldpassword'             => 'Stara lozinka:',
'newpassword'             => 'Nova lozinka:',
'retypenew'               => 'Ukucajte ponovo novu lozinku:',
'textboxsize'             => 'Veličine tekstualnog polja',
'rows'                    => 'Redova',
'columns'                 => 'Kolona',
'searchresultshead'       => 'Podešavanja rezultata pretrage',
'resultsperpage'          => 'Pogodaka po stranici:',
'contextlines'            => 'Linija po pogotku:',
'contextchars'            => 'Karaktera konteksta po liniji:',
'recentchangesdays'       => 'Broj dana za prikaz u nedavnim izmjenama:',
'recentchangescount'      => 'Broj naslova u nedavnim izmjenama:',
'savedprefs'              => 'Vaša podešavanja su sačuvana.',
'timezonelegend'          => 'Vremenska zona',
'timezonetext'            => 'Unesite broj sati za koji se Vaše lokalno vrijeme razlikuje od serverskog vremena (UTC).',
'localtime'               => 'Lokalno vrijeme',
'timezoneoffset'          => 'Odstupanje',
'servertime'              => 'Vrijeme na serveru',
'guesstimezone'           => 'Popuni iz brauzera',
'defaultns'               => 'Uobičajeno tražite u ovim imenskim prostorima:',

# User rights
'editinguser' => "Uređujete '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",

# Recent changes
'recentchanges'                  => 'Nedavne izmjene',
'recentchangestext'              => 'Na ovoj stranici možete pratiti nedavne izmjene.',
'recentchanges-feed-description' => 'Na ovoj stranici možete pratiti nedavne izmjene.',
'rcnote'                         => "Ispod {{PLURAL:$1|je '''1''' promjena|su '''$1''' zadnje promjene|su '''$1''' zadnjih promjena}} u {{PLURAL:$2|posljednjem '''$2''' danu|posljednja '''$2''' dana|posljednjih '''$2''' dana}}, od $4, $5.",
'rcnotefrom'                     => 'Ispod su izmjene od <b>$2</b> (do <b>$1</b> prikazano).',
'rclistfrom'                     => 'Prikaži nove izmjene počev od $1',
'rcshowhideminor'                => '$1 male izmjene',
'rcshowhidebots'                 => '$1 botove',
'rcshowhideliu'                  => '$1 prijavljene korisnike',
'rcshowhideanons'                => '$1 anonimne korisnike',
'rcshowhidepatr'                 => '$1 patrolirane izmjene',
'rcshowhidemine'                 => '$1 moje izmjene',
'rclinks'                        => 'Prikaži najskorijih $1 izmjena u poslednjih $2 dana; $3',
'diff'                           => 'razl',
'hist'                           => 'ist',
'hide'                           => 'sakrij',
'show'                           => 'pokaži',
'minoreditletter'                => 'm',
'newpageletter'                  => 'N',
'boteditletter'                  => 'b',

# Recent changes linked
'recentchangeslinked'          => 'Srodne izmjene',
'recentchangeslinked-title'    => 'Srodne promjene sa "$1"',
'recentchangeslinked-noresult' => 'Nema izmjena na povezanim stranicama u zadanom periodu.',
'recentchangeslinked-summary'  => "Ova posebna stranica prikazuje promjene na povezanim stranicama. 
Stranice koje su na vašem [[Special:Watchlist|spisku praćenja]] su '''podebljane'''.",

# Upload
'upload'                      => 'Postavi datoteku',
'uploadbtn'                   => 'Postavi datoteku',
'reupload'                    => 'Ponovo pošaljite',
'reuploaddesc'                => 'Vratite se na upitnik za slanje.',
'uploadnologin'               => 'Niste prijavljeni',
'uploadnologintext'           => 'Morate biti [[Special:UserLogin|prijavljeni]]
da bi ste slali fajlove.',
'uploaderror'                 => 'Greška pri slanju',
'uploadlog'                   => 'log slanja',
'uploadlogpage'               => 'Protokol postavljanja',
'uploadlogpagetext'           => 'Ispod je spisak najskorijih slanja.',
'filename'                    => 'Ime fajla',
'filedesc'                    => 'Opis',
'filestatus'                  => 'Status autorskih prava:',
'filesource'                  => 'Izvor:',
'uploadedfiles'               => 'Poslati fajlovi',
'badfilename'                 => 'Ime slike je promjenjeno u "$1".',
'emptyfile'                   => 'Fajl koji ste poslali je prazan. Ovo je moguće zbog greške u imenu fajla. Molimo Vas da provjerite da li stvarno želite da pošaljete ovaj fajl.',
'fileexists'                  => 'Fajl sa ovim imenom već postoji.  Molimo Vas da provjerite <strong><tt>$1</tt></strong> ako niste sigurni da li želite da ga promjenite.',
'fileexists-forbidden'        => 'Fajl sa ovim imenom već postoji; molimo Vas da se vratite i pošaljete ovaj fajl pod novim imenom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Fajl sa ovim imenom već postoji u zajedničkoj ostavi; molimo Vas da se vratite i pošaljete ovaj fajl pod novim imenom. [[Image:$1|thumb|center|$1]]',
'successfulupload'            => 'Uspješno slanje',
'uploadwarning'               => 'Upozorenje pri slanju',
'savefile'                    => 'Sačuvaj fajl',
'uploadedimage'               => 'poslato "[[$1]]"',
'uploaddisabled'              => 'Slanje fajlova je isključeno',
'uploadvirus'                 => 'Fajl sadrži virus!  Detalji:  $1',

# Special:ImageList
'imagelist' => 'Spisak slika',

# Image description page
'filehist'            => 'Historija datoteke',
'filehist-help'       => 'Kliknite na datum/vrijeme da vidite verziju datoteke iz tog vremena.',
'filehist-current'    => 'trenutno',
'filehist-datetime'   => 'Datum/Vrijeme',
'filehist-user'       => 'Korisnik',
'filehist-dimensions' => 'Dimenzije',
'filehist-filesize'   => 'Veličina datoteke',
'filehist-comment'    => 'Komentar',
'imagelinks'          => 'Upotreba slike',
'linkstoimage'        => '{{PLURAL:$1|Slijedeća stranica koristi|Slijedećih $1 stranica koriste}} ovu sliku:',
'nolinkstoimage'      => 'Nema stranica koje koriste ovu sliku.',
'sharedupload'        => 'Ova datoteka se nalazi na [[Commons:Početna strana|Wikimedia Commons]] i može se koristiti i na drugim projektima.',

# MIME search
'mimesearch' => 'MIME pretraga',
'mimetype'   => 'MIME tip:',

# Random page
'randompage' => 'Slučajna stranica',

# Statistics
'statistics'    => 'Statistike',
'sitestats'     => 'Statistika sajta',
'userstats'     => 'Statistike korisnika',
'sitestatstext' => "{{SITENAME}} trenutno ima '''$2''' članaka.

Ovaj broj isključuje preusmjerenja, stranice za razgovor, stranice sa opisom slike, korisničke stranice, šablone, stranice za pomoć, članke bez poveznica, i stranice o projektu {{SITENAME}}.</p>

Totalni broj stranica u bazi:  '''$1'''.

'''$8''' files have been uploaded.

Bilo je '''$3''' pogleda stranica, i '''$4''' izmjena otkad je viki bio instaliran.
To izađe u prosjeku oko '''$5''' izmjena po stranici, i '''$6''' pogleda po izmjeni.

The [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] length is '''$7'''.",
'userstatstext' => "Postoji {{PLURAL:$1| '''1''' rigistrovan [[Special:ListUsers|korisnik]]| '''$1''' registriranih [[Special:ListUsers|korisnika]]}}, od kojih '''$2''' (ili '''$4%''') {{PLURAL:$2|ima|imaju}} $5 prava.",

'disambiguations'     => 'Stranice za višeznačne odrednice',
'disambiguationspage' => '{{ns:template}}:Višeznačna odrednica',

'doubleredirects'     => 'Dvostruka preusmjerenja',
'doubleredirectstext' => 'Svaki red sadrži veze na prvo i drugo preusmjerenje, kao i na prvu liniju teksta drugog preusmjerenja, što obično daje "pravi" ciljni članak, na koji bi prvo preusmjerenje i trebalo da pokazuje.',

'brokenredirects'     => 'Pokvarena preusmjerenja',
'brokenredirectstext' => 'Sledeća preusmjerenja su povezana na nepostojeći članak:',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajtova}}',
'ncategories'             => '$1 {{PLURAL:$1|kategorija|kategorije}}',
'nlinks'                  => '$1 {{PLURAL:$1|veza|veze}}',
'nmembers'                => '$1 {{PLURAL:$1|član|članova}}',
'nrevisions'              => '$1 {{PLURAL:$1|revizija|revizije}}',
'nviews'                  => '$1 {{PLURAL:$1|pregled|pregleda}}',
'specialpage-empty'       => 'Nepostoje rezultati za ovaj izvještaj.',
'lonelypages'             => 'Siročići',
'uncategorizedpages'      => 'Nekategorisane stranice',
'uncategorizedcategories' => 'Nekategorisane kategorije',
'unusedcategories'        => 'Nekorišćene kategorije',
'unusedimages'            => 'Neupotrebljene slike',
'popularpages'            => 'Popularne stranice',
'wantedcategories'        => 'Tražene kategorije',
'wantedpages'             => 'Tražene stranice',
'shortpages'              => 'Kratke stranice',
'longpages'               => 'Dugačke stranice',
'deadendpages'            => 'Stranice bez internih veza',
'listusers'               => 'Spisak korisnika',
'newpages'                => 'Nove stranice',
'ancientpages'            => 'Najstarije stranice',
'move'                    => 'Preusmjeri',
'movethispage'            => 'Premjesti ovu stranicu',
'unusedimagestext'        => '<p>Obratite pažnju da se drugi veb sajtovi, kao što su drugi
međunarodni Vikiji, mogu povezati na sliku direktnom
URL-om, i tako mogu još uvijek biti prikazani ovdje uprkos
aktivnoj upotrebi.</p>',
'unusedcategoriestext'    => 'Sledeće strane kategorija postoje iako ih ni jedan drugi članak ili kategorija ne koriste.',
'notargettitle'           => 'Nema cilja',
'notargettext'            => 'Niste naveli ciljnu stranicu ili korisnika
na kome bi se izvela ova funkcija.',

# Book sources
'booksources' => 'Štampani izvori',

# Special:Log
'specialloguserlabel'  => 'Korisnik:',
'speciallogtitlelabel' => 'Naslov:',
'log'                  => 'Protokoli',

# Special:AllPages
'allpages'       => 'Sve stranice',
'alphaindexline' => '$1 do $2',
'nextpage'       => 'Sljedeća strana ($1)',
'prevpage'       => 'Prethodna stranica ($1)',
'allpagesfrom'   => 'Prikaži stranice počev od:',
'allarticles'    => 'Svi članci',
'allpagessubmit' => 'Idi',

# Special:Categories
'categories'                  => 'Kategorije',
'categoriespagetext'          => 'Sledeće kategorije već postoje u {{SITENAME}}',
'special-categories-sort-abc' => 'sortiraj po abecedi',

# E-mail user
'mailnologin'     => 'Nema adrese za slanje',
'mailnologintext' => 'Morate biti [[Special:UserLogin|prijavljeni]]
i imati ispravnu adresu e-pošte u vašim [[Special:Preferences|podešavanjima]]
da biste slali e-poštu drugim korisnicima.',
'emailuser'       => 'Pošalji e-poštu ovom korisniku',
'emailpage'       => 'Pošalji e-pismo korisniku',
'emailpagetext'   => 'Ako je ovaj korisnik unio ispravnu adresu e-pošte u
cvoja korisnička podešavanja, upitnik ispod će poslati jednu poruku.
Adresa e-pošte koju ste vi uneli u svoja korisnička podešavanja će se pojaviti
kao "Od" adresa poruke, tako da će primalac moći da odgovori.',
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
'emailsent'       => 'Poruka poslata',
'emailsenttext'   => 'Vaša poruka je poslata e-poštom.',

# Watchlist
'watchlist'            => 'Praćeni članci',
'mywatchlist'          => 'Praćeni članci',
'watchlistfor'         => "(korisnika '''$1''')",
'nowatchlist'          => 'Nemate ništa na svom spisku praćenih članaka.',
'watchnologin'         => 'Niste prijavljeni',
'watchnologintext'     => 'Morate biti [[Special:UserLogin|prijavljeni]] da bi ste mijenjali spisak praćenih članaka.',
'addedwatch'           => 'Dodato u spisak praćenih članaka',
'addedwatchtext'       => 'Stranica "[[:$1]]" je dodata vašem [[Special:Watchlist|spisku praćenih članaka]]. Buduće promjene ove stranice i njoj pridružene stranice za razgovor će biti navedene ovde, i stranica će biti <b>podebljana</b> u [[Special:RecentChanges|spisku]] nedavnih izmjena da bi se lakše uočila.

Ako kasnije želite da uklonite stranicu sa vašeg spiska praćenih članaka, kliknite na "prekini praćenje" na paleti.',
'removedwatch'         => 'Uklonjeno iz spiska praćenih članaka',
'removedwatchtext'     => 'Stranica "<nowiki>$1</nowiki>" je uklonjena iz vašeg spiska praćenih članaka.',
'watch'                => 'Prati članak',
'watchthispage'        => 'Prati ovu stranicu',
'unwatch'              => 'Ukinite praćenje',
'unwatchthispage'      => 'Ukinite praćenje',
'notanarticle'         => 'Nije članak',
'watchnochange'        => 'Ništa što pratite nije promjenjeno u prikazanom vremenu.',
'watchlist-details'    => '$1 stranica praćeno ne računajući stranice za razgovor',
'wlheader-enotif'      => '* Obavještavanje e-poštom je omogućeno.',
'wlheader-showupdated' => "* Stranice koje su izmjenjene od kad ste ih poslednji put posjetili su prikazane '''podebljanim slovima'''",
'watchmethod-recent'   => 'provjerava se da li ima praćenih stranica u nedavnim izmjenama',
'watchmethod-list'     => 'provjerava se da li ima nedavnih izmjena u praćenim stranicama',
'watchlistcontains'    => 'Vaš spisak praćenih članaka sadrži $1 stranica.',
'iteminvalidname'      => "Problem sa '$1', neispravno ime...",
'wlnote'               => 'Ispod je najskorijih $1 izmjena, načinjenih u posljednjih <b>$2</b> sati.',
'wlshowlast'           => 'Prikaži poslednjih $1 sati $2 dana $3',
'watchlist-hide-bots'  => 'Sakrij botove',
'watchlist-hide-own'   => 'Sakrij moje izmjene',
'watchlist-hide-minor' => 'Sakrij male izmjene',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Pratim...',
'unwatching' => 'Ne pratim...',

'enotif_mailer'      => '{{SITENAME}} obaviještenje o pošti',
'enotif_reset'       => 'Označi sve strane kao posjećene',
'enotif_newpagetext' => 'Ovo je novi članak.',
'enotif_subject'     => '{{SITENAME}} strana $PAGETITLE je bila $CHANGEDORCREATED od strane $PAGEEDITOR',
'enotif_lastvisited' => 'Pogledajte $1 za sve izmjene od vaše poslednje posjete.',
'enotif_body'        => 'Dragi $WATCHINGUSERNAME,

{{SITENAME}} strana $PAGETITLE je bila $CHANGEDORCREATED $PAGEEDITDATE od strane $PAGEEDITOR,
pogledajte {{fullurl:$PAGETITLE}} za trenutnu verziju.

$NEWPAGE

Rezime editora: $PAGESUMMARY $PAGEMINOREDIT

Kontaktirajte editora:
pošta {{fullurl:Special:Emailuser|target=$PAGEEDITOR}}
viki {{fullurl:User:$PAGEEDITOR}}

Neće biti drugih obaviještenja u slučaju daljih izmjena ukoliko ne posjetite ovu stranu.
Takođe možete da resetujete zastavice za obaviještenja za sve Vaše praćene stranice na vašem spisku praćenenih članaka.

             Vaš prijateljski {{SITENAME}} sistem obaviještavanja

--
Da promjenite podešavanja vezana za spisak praćenenih članaka posjetite
{{fullurl:Special:Watchlist|edit=yes}}

Fidbek i dalja pomoć:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Obrišite stranicu',
'confirm'                     => 'Potvrdite',
'excontent'                   => "sadržaj je bio: '$1'",
'exbeforeblank'               => "sadržaj prije brisanja je bio: '$1'",
'exblank'                     => 'stranica je bila prazna',
'historywarning'              => 'Upozorenje:  Stranica koju želite da obrišete ima historiju:',
'confirmdeletetext'           => 'Brisanjem ćete obrisati stranicu ili sliku zajedno sa historijom iz baze podataka, ali će se iste moći vratiti kasnije. 
Molim potvrdite svoju namjeru, da razumijete posljedice i da ovo radite u skladu sa [[{{MediaWiki:Policy-url}}|pravilima]].',
'actioncomplete'              => 'Akcija završena',
'deletedtext'                 => 'Članak "<nowiki>$1</nowiki>" je obrisan.
Pogledajte $2 za zapis o skorašnjim brisanjima.',
'deletedarticle'              => 'obrisan "[[$1]]"',
'dellogpage'                  => 'Protokol brisanja',
'dellogpagetext'              => 'Ispod je spisak najskorijih brisanja.',
'deletionlog'                 => 'istorija brisanja',
'reverted'                    => 'Vraćeno na prijašnju reviziju',
'deletecomment'               => 'Razlog za brisanje',
'rollback'                    => 'Vrati izmjene',
'rollback_short'              => 'Vrati',
'rollbacklink'                => 'vrati',
'rollbackfailed'              => 'Vraćanje nije uspjelo',
'cantrollback'                => 'Ne može se vratiti izmjena; poslednji autor je ujedno i jedini.',
'alreadyrolled'               => 'Ne može se vratiti poslednja izmjena [[:$1]] od korisnika [[User:$2|$2]] ([[User talk:$2|razgovor]]); neko drugi je već izmjenio ili vratio članak.  Poslednja izmjena od korisnika [[User:$3|$3]] ([[User talk:$3|razgovor]]).',
'editcomment'                 => 'Komentar izmjene je: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Vraćene izmjene $2 na poslednju izmjenu korisnika $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Protokol zaključavanja',
'protectlogtext'              => 'Ispod je spisak zaštićenja stranice.',
'protectedarticle'            => 'stranica "[[$1]]" je zaštićena',
'unprotectedarticle'          => 'odštićena "$1"',
'protect-title'               => 'Zaštićuje se "$1"',
'protect-legend'              => 'Potvrdite zaštitu',
'protectcomment'              => 'Razlog za zaštitu',
'protect-unchain'             => 'Deblokirajte dozvole premještanja',
'protect-text'                => 'Ovdje možete gledati i izmjeniti level zaštite za stranicu <strong><nowiki>$1</nowiki></strong>.',
'protect-default'             => '(standardno)',
'protect-level-autoconfirmed' => 'Blokiraj neregistrovane korisnike',
'protect-level-sysop'         => 'Samo administratori',

# Undelete
'undelete'               => 'Pogledaj izbrisane stranice',
'undeletepage'           => 'Pogledaj i vrati izbrisane stranice',
'viewdeletedpage'        => 'Pogledaj izbrisane stranice',
'undeletepagetext'       => 'Sledeće stranice su izbrisane ali su još uvijek u arhivi i
mogu biti vraćene.  Arhiva moše biti periodično čišćena.',
'undeleterevisions'      => '$1 revizija arhivirano',
'undeletehistory'        => 'Ako vratite stranicu, sve revizije će biti vraćene njenoj istoriji.
Ako je nova stranica istog imena napravljena od brisanja, vraćene
revizije će se pojaviti u ranijoj istoriji, a trenutna revizija sadašnje stranice
neće biti automatski zamijenjena.',
'undeletehistorynoadmin' => 'Ova stranica je izbrisana.  Ispod se nalazi dio istorije brisanja i istorija revizija izbrisane stranice.  Tekst izbrisane stranice je vidljiv samo korisnicima koji su administratori.',
'undeletebtn'            => 'Vrati!',
'undeletedarticle'       => 'vraćeno "$1"',
'undeletedrevisions'     => '$1 revizija vraćeno',

# Namespace form on various pages
'namespace'      => 'Vrsta članka:',
'invert'         => 'Sve osim odabranog',
'blanknamespace' => '(Glavno)',

# Contributions
'contributions' => 'Doprinos korisnika',
'mycontris'     => 'Moj doprinos',
'contribsub2'   => 'Za $1 ($2)',
'nocontribs'    => 'Nisu nađene promjene koje zadovoljavaju ove uslove.',
'uctop'         => ' (vrh)',
'month'         => 'Od mjeseca (i ranije):',
'year'          => 'Od godine (i ranije):',

'sp-contributions-blocklog' => 'Evidencija blokiranja',

# What links here
'whatlinkshere'       => 'Šta je povezano ovdje',
'whatlinkshere-title' => 'Stranice koje vode na "$1"',
'linklistsub'         => '(Spisak veza)',
'linkshere'           => "Sljedeći članci vode na '''[[:$1]]''':",
'nolinkshere'         => "Nema linkova na '''[[:$1]]'''.",
'isredirect'          => 'preusmjerivač',
'istemplate'          => 'kao šablon',
'whatlinkshere-prev'  => '{{PLURAL:$1|prethodni|prethodna|prethodnih}} $1',
'whatlinkshere-next'  => '{{PLURAL:$1|sljedeći|sljedeća|sljedećih}} $1',
'whatlinkshere-links' => '← linkovi',

# Block/unblock
'blockip'              => 'Blokiraj korisnika',
'blockiptext'          => 'Upotrebite donji upitnik da biste uklonili prava pisanja sa određene IP adrese ili korisničkog imena.  Ovo bi trebalo da bude urađeno samo da bi se spriječio vandalizam, i u skladu sa [[{{MediaWiki:Policy-url}}|smjernicama]]. Unesite konkretan razlog ispod (na primjer, navodeći koje stranice su vandalizovane).',
'ipaddress'            => 'IP adresa/korisničko ime',
'ipbexpiry'            => 'Trajanje',
'ipbreason'            => 'Razlog',
'ipbsubmit'            => 'Blokirajte ovog korisnika',
'ipboptions'           => '15 minuta:15 min,1 sat:1 hour,2 sata:2 hours,6 sati:6 hours,12 sati:12 hours,1 dan:1 day,3 dana:3 days,1 sedmica:1 week,2 sedmice:2 weeks,1 mjesec:1 month,3 mjeseca:3 months,6 mjeseci:6 months,1 godine:1 year,zauvijek:infinite', # display1:time1,display2:time2,...
'badipaddress'         => 'Pogrešna IP adresa',
'blockipsuccesssub'    => 'Blokiranje je uspjelo',
'blockipsuccesstext'   => '[[Special:Contributions/$1|$1]] je blokiran.
<br />Pogledajte [[Special:IPBlockList|IP spisak blokiranih korisnika]] za pregled blokiranja.',
'unblockip'            => 'Odblokiraj korisnika',
'unblockiptext'        => 'Upotrebite donji upitnik da bi ste vratili
pravo pisanja ranije blokiranoj IP adresi
ili korisničkom imenu.',
'ipusubmit'            => 'Deblokirajte ovog korisnika',
'ipblocklist'          => 'Spisak blokiranih IP adresa i korisničkih imena',
'blocklistline'        => '$1, $2 blokirao korisnika $3 ($4)',
'blocklink'            => 'blokirajte',
'unblocklink'          => 'deblokiraj',
'contribslink'         => 'doprinosi',
'autoblocker'          => 'Automatski ste blokirani jer dijelite IP adresu sa "$1".  Razlog za blokiranje je: "\'\'\'$2\'\'\'"',
'blocklogpage'         => 'Evidencija blokiranja',
'blocklogentry'        => 'je blokirao [[$1]] sa vremenom isticanja blokade od $2 $3',
'blocklogtext'         => 'Ovo je istorija blokiranja i deblokiranja korisnika.  Automatsko blokirane IP adrese nisu uspisane ovde.  Pogledajte [[Special:IPBlockList|blokirane IP adrese]] za spisak trenutnih zabrana i blokiranja.',
'unblocklogentry'      => 'deblokiran $1',
'range_block_disabled' => 'Administratorska mogućnost da blokira grupe je isključena.',
'ipb_expiry_invalid'   => 'Pogrešno vrijeme trajanja.',
'ip_range_invalid'     => 'Netačan raspon IP adresa.',
'proxyblocker'         => 'Bloker proksija',
'proxyblockreason'     => 'Vaša IP adresa je blokirana jer je ona otvoreni proksi.  Molimo vas da kontaktirate vašeg davatelja internetskih usluga (Internet Service Provider-a) ili tehničku podršku i obavijestite ih o ovom ozbiljnom sigurnosnom problemu.',
'proxyblocksuccess'    => 'Proksi uspješno blokiran.',

# Developer tools
'lockdb'              => 'Zaključajte bazu',
'unlockdb'            => 'Otključaj bazu',
'lockdbtext'          => 'Zaključavanje baze će svim korisnicima ukinuti mogućnost izmjene stranica,
promjene korisničkih podešavanja, izmjene praćenih članaka, i svega ostalog
što zahtjeva promjene u bazi.
Molimo Vas da potvrdite da je ovo zaista ono što namjeravate da uradite, i da ćete
otkučati bazu kad završite posao oko njenog održavanja.',
'unlockdbtext'        => 'Otključavanje baze će svim korisnicima vratiti mogućnost
izmjene stranica, promjene korisničkih stranica, izmjene spiska praćenih članaka,
i svega ostalog što zahtjeva promjene u bazi.
Molimo Vas da potvrdite da je ovo zaista ono što namijeravate da uradite.',
'lockconfirm'         => 'Da, zaista želim da zaključam bazu.',
'unlockconfirm'       => 'Da, zaista želim da otključam bazu.',
'lockbtn'             => 'Zaključajte bazu',
'unlockbtn'           => 'Otključaj bazu',
'locknoconfirm'       => 'Niste potvrdili svoju namjeru.',
'lockdbsuccesssub'    => 'Baza je zaključana',
'unlockdbsuccesssub'  => 'Baza je otključana',
'lockdbsuccesstext'   => '{{SITENAME}} baza podataka je zaključana. <br /> Sjetite se da je otključate kad završite sa održavanjem.',
'unlockdbsuccesstext' => '{{SITENAME}} baza podataka je otključana.',

# Move page
'move-page-legend' => 'Premjestite stranicu',
'movepagetext'     => "Korištenjem ovog formulara možete preusmjeriti članak 
zajedno sa stranicom za diskusiju tog članka.

Članak pod starim imenom će postati stranica koja preusmjerava 
na članak pod novim imenom. Linkovi koji vode na članak sa 
starim imenom neće biti preusmjereni. Vaša je dužnost da se 
pobrinete da svi linkovi koji vode na članak sa starim imenom 
budu adekvatno preusmjereni (stranica posebne namjene za 
održavanje je korisna za obavještenje o [[Special:BrokenRedirects|mrtvim]] i [[Special:DoubleRedirects|duplim]] preusmjerenjima).

Imajte na umu da članak '''neće''' biti preusmjeren ukoliko 
već postoji članak pod imenom na koje namjeravate da 
preusmjerite.

'''Pažnja!'''
Imajte na umu da preusmjeravanje popularnog članka može biti 
drastična i neočekivana promjena za korisnike.",
'movepagetalktext' => "Odgovarajuća stranica za razgovor, ako postoji, će automatski biti premještena istovremeno '''osim:'''
*Ako premještate stranicu preko imenskih prostora,
*Neprazna stranica za razgovor već postoji pod novim imenom, ili
*Odčekirajte donju kutiju.

U tim slučajevima, moraćete ručno da premjestite stranicu ukoliko to želite.",
'movearticle'      => 'Premjestite stranicu',
'newtitle'         => 'Novi naslov',
'movepagebtn'      => 'premjestite stranicu',
'pagemovedsub'     => 'Premještanje uspjelo',
'articleexists'    => 'Stranica pod tim imenom već postoji, ili je ime koje ste izabrali neispravno.  Molimo Vas da izaberete drugo ime.',
'talkexists'       => 'Sama stranica je uspješno premještena, ali
stranica za razgovor nije mogla biti premještena jer takva već postoji na novom naslovu.  Molimo Vas da ih spojite ručno.',
'movedto'          => 'premještena na',
'movetalk'         => 'Premjestite "stranicu za razgovor" takođe, ako je moguće.',
'1movedto2'        => 'članak [[$1]] premješten na [[$2]]',
'1movedto2_redir'  => 'stranica [[$1]] premještena u stranicu [[$2]] putem preusmjerenja',
'movelogpage'      => 'Protokol premještanja',
'revertmove'       => 'vrati',
'selfmove'         => 'Izvorni i ciljani naziv su isti; strana ne može da se premjesti preko same sebe.',

# Export
'export'        => 'Izvezite stranice',
'exporttext'    => 'Možete izvesti tekst i historiju jedne ili više stranica uklopljene u XML kod. U budućim verzijama MediaWiki programa bit će moguće uvesti ovakvu stranicu u neki drugi wiki. Trenutna verzija to još ne podržava.

Za izvoz stranica unesite njihove naslove u polje ispod, jedan naslov po retku, i označite želite li trenutačnu verziju zajedno sa svim prijašnjima, ili samo trenutnu verziju sa informacijom o zadnjoj promjeni.

U drugom slučaju možete koristiti i vezu, npr. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] za članak [[{{MediaWiki:Mainpage}}]].',
'exportcuronly' => 'Uključite samo trenutnu reviziju, ne cijelu istoriju',

# Namespace 8 related
'allmessages'               => 'Sve sistemske poruke',
'allmessagestext'           => 'Ovo je spisak svih sistemskih poruka u {{ns:mediawiki}} imenskom prostoru.',
'allmessagesnotsupportedDB' => 'Ova stranica ne može biti korištena jer je <i>wgUseDatabaseMessages</i> isključen.',

# Thumbnails
'thumbnail-more'  => 'uvećajte',
'filemissing'     => 'Nedostaje fajl',
'thumbnail_error' => 'Greška pri pravljenju umanjene slike: $1',

# Special:Import
'import'                => 'Ivoz stranica',
'importtext'            => 'Molimo Vas da izvezete fajl iz izvornog vikija koristeći [[Special:Export|izvoz]], sačuvajte ga kod sebe i pošaljite ovde.',
'importfailed'          => 'Uvoz nije uspjeo: $1',
'importnotext'          => 'Stranica je prazna, ili bez teksta',
'importsuccess'         => 'Uspješno ste uvezli stranicu!',
'importhistoryconflict' => 'Postoji konfliktna istorija revizija',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Moja korisnička stranica',
'tooltip-pt-anonuserpage'         => 'Korisnička stranica za ip koju Vi uređujete kao',
'tooltip-pt-mytalk'               => 'Moja stranica za razgovor',
'tooltip-pt-anontalk'             => 'Razgovor o doprinosu sa ove IP adrese',
'tooltip-pt-preferences'          => 'Moja podešavanja',
'tooltip-pt-watchlist'            => 'Spisak članaka koje pratite.',
'tooltip-pt-mycontris'            => 'Spisak mog doprinosa',
'tooltip-pt-login'                => 'Predlažemo da se prijavite, ali nije obvezno.',
'tooltip-pt-anonlogin'            => 'Prijava nije obavezna, ali donosi mnogo koristi.',
'tooltip-pt-logout'               => 'Odjava sa projekta {{SITENAME}}',
'tooltip-ca-talk'                 => 'Razgovor o sadržaju',
'tooltip-ca-edit'                 => 'Možete da uređujete ovaj članak. Molimo Vas, koristite dugme "Prikaži izgled',
'tooltip-ca-addsection'           => 'Dodajte svoj komentar.',
'tooltip-ca-viewsource'           => 'Ovaj članak je zaključan. Možete ga samo vidjeti ili kopirati kod.',
'tooltip-ca-history'              => 'Prethodne verzije ove stranice.',
'tooltip-ca-protect'              => 'Zaštitite stranicu od budućih izmjena',
'tooltip-ca-delete'               => 'Izbrišite ovu stranicu',
'tooltip-ca-undelete'             => 'Vratite izmjene koje su načinjene prije brisanja stranice',
'tooltip-ca-move'                 => 'Pomjerite stranicu',
'tooltip-ca-watch'                => 'Dodajte stranicu u listu praćnih članaka',
'tooltip-ca-unwatch'              => 'Izbrišite stranicu sa liste praćnih članaka',
'tooltip-search'                  => 'Pretraži projekat {{SITENAME}}',
'tooltip-p-logo'                  => 'Glavna stranica',
'tooltip-n-mainpage'              => 'Posjetite početnu stranicu',
'tooltip-n-portal'                => 'O projektu, šta možete da uradite, gdje se šta nalazi',
'tooltip-n-currentevents'         => 'Podaci o onome na čemu se trenutno radi',
'tooltip-n-recentchanges'         => 'Spisak nedavnih izmjena na wiki.',
'tooltip-n-randompage'            => 'Otvorite slučajan članak',
'tooltip-n-help'                  => 'Mjesto gdje možete nešto da naučite.',
'tooltip-t-whatlinkshere'         => 'Spisak svih članaka koji su povezani sa ovim',
'tooltip-t-recentchangeslinked'   => 'Nedavne izmjene na stranicama koje su povezane sa ovom',
'tooltip-feed-rss'                => 'RSS za ovu stranicu',
'tooltip-feed-atom'               => 'Atom za ovu stranicu',
'tooltip-t-contributions'         => 'Pogledajte spisak doprinosa ovog korisnika',
'tooltip-t-emailuser'             => 'Pošaljite pismo ovom korisniku',
'tooltip-t-upload'                => 'Postavi slike i druge medije',
'tooltip-t-specialpages'          => 'Spisak svih posebnih stranica',
'tooltip-ca-nstab-main'           => 'Pogledajte sadržaj članka',
'tooltip-ca-nstab-user'           => 'Pogledajte korisničku stranicu',
'tooltip-ca-nstab-media'          => 'Pogledajte medija fajl',
'tooltip-ca-nstab-special'        => 'Ovo je specijalna stranica i zato je ne možete uređivati',
'tooltip-ca-nstab-project'        => 'Pogledajte projekat stranicu',
'tooltip-ca-nstab-image'          => 'Pogledajte stranicu slike',
'tooltip-ca-nstab-mediawiki'      => 'Pogledajte sistemsku poruku',
'tooltip-ca-nstab-template'       => 'Pogledajte šablon',
'tooltip-ca-nstab-help'           => 'Pogledajte stranicu za pomoć',
'tooltip-ca-nstab-category'       => 'Pogledajte stranicu kategorije',
'tooltip-minoredit'               => 'Naznačite da se radi o maloj izmjeni',
'tooltip-save'                    => 'Sačuvajte Vaše izmjene',
'tooltip-preview'                 => 'Pregledajte Vaše izmjene; molimo Vas da koristite ovo prije nego što sačuvate stranicu!',
'tooltip-diff'                    => 'Prikaži moje izmjene u tekstu.',
'tooltip-compareselectedversions' => 'Pogledajte pazlike između dvije selektovane verzije ove stranice.',
'tooltip-watch'                   => 'Dodajte ovu stranicu na Vaš spisak praćenih članaka',

# Metadata
'nodublincore'      => 'Dublin Core RDF metapodaci onemogućeni za ovaj server.',
'nocreativecommons' => 'Creative Commons RDF metapodaci onemogućeni za ovaj server.',
'notacceptable'     => 'Viki server ne može da pruži podatke u onom formatu koji Vaš klijent može da pročita.',

# Attribution
'anonymous'        => 'Anonimni korisnik od {{SITENAME}}',
'siteuser'         => '{{SITENAME}} korisnik $1',
'lastmodifiedatby' => 'Ovu stranicu je posljednji put promjenio $3, u $2, $1', # $1 date, $2 time, $3 user
'othercontribs'    => 'Bazirano na radu od strane korisnika $1.',
'siteusers'        => '{{SITENAME}} korisnik (korisnici) $1',

# Spam protection
'spamprotectiontitle' => 'Filter za zaštitu od neželjenih poruka',
'spamprotectiontext'  => 'Strana koju želite da sačuvate je blokirana od strane filtera za neželjene poruke.  Ovo je vjerovatno izazvao vezom ka spoljašnjem sajtu.',
'spamprotectionmatch' => 'Sledeći tekst je izazvao naš filter za neželjene poruke: $1',

# Patrolling
'markaspatrolleddiff'        => 'Označi kao patrolirano',
'markaspatrolledtext'        => 'Označi ovaj članak kao patroliran',
'markedaspatrolled'          => 'Označeno kao patrolirano',
'markedaspatrolledtext'      => 'Izabrana revizija je označena kao patrolirana.',
'markedaspatrollederror'     => 'Ne može se označiti kao patrolirano',
'markedaspatrollederrortext' => 'Morate naglasiti reviziju koju treba označiti kao patroliranu.',

# Browsing diffs
'previousdiff' => '← Starija izmjena',
'nextdiff'     => 'Novija izmjena →',

# Media information
'mediawarning'         => "'''Upozorenje''': Ovaj fajl sadrži loš kod, njegovim izvršavanjem možete da ugrozite Vaš sistem.
<hr />",
'thumbsize'            => 'Veličina umanjenog prikaza:',
'file-info-size'       => '($1 × $2 piksela, veličina datoteke: $3, MIME tip: $4)',
'file-nohires'         => '<small>Veća rezolucija nije dostupna.</small>',
'svg-long-desc'        => '(SVG fajl, dozvoljeno $1 × $2 piksela, veličina fajla: $3)',
'show-big-image'       => 'Vidi sliku u punoj veličini (rezoluciji)',
'show-big-image-thumb' => '<small>Veličina ovoga prikaza: $1 × $2 piksela</small>',

# Special:NewImages
'imagelisttext' => 'Ispod je spisak $1 slika poređanih $2.',
'showhidebots'  => '($1 botove)',
'ilsubmit'      => 'Traži',
'bydate'        => 'po datumu',

# Bad image list
'bad_image_list' => "Koristi se sljedeći format:

Razmatraju se samo stavke u spisku (linije koje počinju sa *). 
Prvi link u liniji mora biti povezan sa lošom slikom.
Svi drugi linkovi u istoj liniji se smatraju izuzecima, npr. kod stranica gdje se slike pojavljuju ''inline''.",

# Metadata
'metadata'          => 'Metapodaci',
'metadata-help'     => 'Ova datoteka sadržava dodatne podatke koje je vjerojatno dodala digitalna kamera ili skener u procesu snimanja odnosno digitalizacije. Ako je datoteka mijenjana, podatci možda nisu u skladu sa stvarnim stanjem.',
'metadata-expand'   => 'Pokaži sve detalje',
'metadata-collapse' => 'Sakrij dodatne podatke',
'metadata-fields'   => "Slijedeći EXIF metapodaci će biti prikazani ispod slike u tablici s metapodacima. Ostali će biti sakriveni (možete ih vidjeti ako kliknete na link ''Pokaži sve detalje'').
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# External editor support
'edit-externally'      => 'Izmjeni ovu sliku koristeći vanjski program',
'edit-externally-help' => 'Pogledajte [http://www.mediawiki.org/wiki/Manual:External_editors instrukcije za podešavanje] za više informacija.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'sve',
'watchlistall2'    => 'sve',
'namespacesall'    => 'sve',
'monthsall'        => 'sve',

# E-mail address confirmation
'confirmemail'            => 'Potvrdite adresu e-pošte',
'confirmemail_text'       => 'Ova viki zahtjeva da potvrdite adresu Vaše e-pošte prije nego što koristite mogućnosti e-pošte. Aktivirajte dugme ispod kako bi ste poslali poštu za potvrdu na Vašu adresu. Pošta uključuje poveznicu koja sadrži kod; učitajte poveznicu u Vaš brauzer da bi ste potvrdili da je adresa Vaše e-pošte validna.',
'confirmemail_send'       => 'Pošaljite kod za potvrdu',
'confirmemail_sent'       => 'E-pošta za potvrđivanje poslata.',
'confirmemail_sendfailed' => 'Pošta za potvrđivanje nije poslata. Provjerite adresu zbog nepravilnih karaktera.

Povratna pošta: $1',
'confirmemail_invalid'    => 'Netačan kod za potvrdu. Moguće je da je kod istekao.',
'confirmemail_success'    => 'Adresa vaše e-pošte je potvrđena. Možete sad da se prijavite i uživate u viki.',
'confirmemail_loggedin'   => 'Adresa Vaše e-pošte je potvrđena.',
'confirmemail_error'      => 'Nešto je pošlo po zlu prilikom sačuvavanja vaše potvrde.',
'confirmemail_subject'    => 'Vikiriječnik adresa e-pošte za potvrđivanje',
'confirmemail_body'       => 'Neko, vjerovatno Vi, je sa IP adrese $1 registrovao nalog "$2" sa ovom adresom e-pošte na {{SITENAME}}.

Da potvrdite da ovaj nalog stvarno pripada vama i da aktivirate mogućnost e-pošte na {{SITENAME}}, otvorite ovu poveznicu u vašem pretraživaču:

$3

Ako ovo niste vi, pratite ovaj link da prekinete prijavu:
$5

Ovaj kod za potvrdu će isteći u $4.',

# Delete conflict
'confirmrecreate' => "Korisnik [[User:$1|$1]] ([[User talk:$1|razgovor]]) je obrisao ovaj članak pošto ste počeli uređivanje sa razlogom:
: ''$2''

Molimo Vas da potvrdite da stvarno želite da ponovo napravite ovaj članak.",

# Multipage image navigation
'imgmultipageprev' => '← prethodna stranica',

# Table pager
'table_pager_prev' => 'Prethodna stranica',

# Watchlist editing tools
'watchlisttools-edit' => 'Pogledaj i uredi listu praćenih članaka.',
'watchlisttools-raw'  => 'Uređivanje praćenih stranica u okviru praćenja.',

# Iranian month names
'iranian-calendar-m1' => 'Farvardin (Iranski kalendar)',

# Special:Version
'version' => 'Verzija', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages'                   => 'Posebne stranice',
'specialpages-group-maintenance' => 'Izvještaji za održavanje',
'specialpages-group-other'       => 'Ostale posebne stranice',
'specialpages-group-login'       => 'Prijava / Otvaranje računa',
'specialpages-group-changes'     => 'Nedavne izmjene i evidencije',
'specialpages-group-media'       => 'Mediji i postavljanje datoteka',
'specialpages-group-users'       => 'Korisnici i korisnička prava',
'specialpages-group-highuse'     => 'Najčešće korištene stranice',

);
