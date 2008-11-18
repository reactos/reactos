<?php
/** Estonian (Eesti)
 *
 * @ingroup Language
 * @file
 *
 * @author Avjoska
 * @author Jaan513
 * @author Võrok
 * @author WikedKentaur
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA            => 'Meedia',
	NS_SPECIAL          => 'Eri',
	NS_MAIN             => '',
	NS_TALK             => 'Arutelu',
	NS_USER             => 'Kasutaja',
	NS_USER_TALK        => 'Kasutaja_arutelu',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_arutelu',
	NS_IMAGE            => 'Pilt',
	NS_IMAGE_TALK       => 'Pildi_arutelu',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_arutelu',
	NS_TEMPLATE         => 'Mall',
	NS_TEMPLATE_TALK    => 'Malli_arutelu',
	NS_HELP             => 'Juhend',
	NS_HELP_TALK        => 'Juhendi_arutelu',
	NS_CATEGORY         => 'Kategooria',
	NS_CATEGORY_TALK    => 'Kategooria_arutelu'
);

$skinNames = array(
	'standard' => 'Standard',
	'nostalgia' => 'Nostalgia',
	'cologneblue' => 'Kölni sinine',
	'monobook' => 'MonoBook',
	'myskin' => 'Mu oma nahk'
);

#Lisasin eestimaised poed, aga võõramaiseid ei julenud kustutada.

$bookstoreList = array(
	'Apollo' => 'http://www.apollo.ee/search.php?keyword=$1&search=OTSI',
	'minu Raamat' => 'http://www.raamat.ee/advanced_search_result.php?keywords=$1',
	'Raamatukoi' => 'http://www.raamatukoi.ee/cgi-bin/index?valik=otsing&paring=$1',
	'AddALL' => 'http://www.addall.com/New/Partner.cgi?query=$1&type=ISBN',
	'PriceSCAN' => 'http://www.pricescan.com/books/bookDetail.asp?isbn=$1',
	'Barnes & Noble' => 'http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);


$magicWords = array(
	#   ID                                 CASE  SYNONYMS
	'redirect'               => array( 0,    '#redirect', "#suuna"    ),
);

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );
$linkTrail = "/^([a-z]+)(.*)\$/sD";

$datePreferences = array(
	'default',
	'et numeric',
	'dmy',
	'et roman',
	'ISO 8601'
);

$datePreferenceMigrationMap = array(
	'default',
	'et numeric',
	'dmy',
	'et roman',
);

$defaultDateFormat = 'dmy';

$dateFormats = array(
	'et numeric time' => 'H:i',
	'et numeric date' => 'd.m.Y',
	'et numeric both' => 'd.m.Y, "kell" H:i',

	'dmy time' => 'H:i',
	'dmy date' => 'j. F Y',
	'dmy both' => 'j. F Y, "kell" H:i',

	'et roman time' => 'H:i',
	'et roman date' => 'j. xrm Y',
	'et roman both' => 'j. xrm Y, "kell" H:i',
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Lingid alla kriipsutada',
'tog-highlightbroken'         => 'Vorminda lingirikked <a href="" class="new">nii</a> (alternatiiv: nii<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Lõikude rööpjoondus',
'tog-hideminor'               => 'Peida pisiparandused viimastes muudatustes',
'tog-extendwatchlist'         => 'Laienda jälgimisloendit, et näha kõiki muudatusi',
'tog-usenewrc'                => 'Laiendatud viimased muudatused (mitte kõikide brauserite puhul)',
'tog-numberheadings'          => 'Pealkirjade automaatnummerdus',
'tog-showtoolbar'             => 'Redigeerimise tööriistariba näitamine',
'tog-editondblclick'          => 'Artiklite redigeerimine topeltklõpsu peale (JavaScript)',
'tog-editsection'             => '[redigeeri] lingid peatükkide muutmiseks',
'tog-editsectiononrightclick' => 'Peatükkide redigeerimine paremklõpsuga alampealkirjadel (JavaScript)',
'tog-showtoc'                 => 'Näita sisukorda (lehtedel, millel on rohkem kui 3 pealkirja)',
'tog-rememberpassword'        => 'Parooli meeldejätmine tulevasteks seanssideks',
'tog-editwidth'               => 'Redaktoriaknal on täislaius',
'tog-watchcreations'          => 'Lisa minu loodud lehed jälgimisloendisse',
'tog-watchdefault'            => 'Jälgi uusi ja muudetud artikleid',
'tog-watchmoves'              => 'Lisa minu teisaldatud artiklid jälgimisloendisse',
'tog-watchdeletion'           => 'Lisa minu kustutatud leheküljed jälgimisloendisse',
'tog-minordefault'            => 'Märgi kõik parandused vaikimisi pisiparandusteks',
'tog-previewontop'            => 'Näita eelvaadet redaktoriakna ees, mitte järel',
'tog-previewonfirst'          => 'Näita eelvaadet esimesel redigeerimisel',
'tog-nocache'                 => 'Keela lehekülgede puhverdamine',
'tog-enotifwatchlistpages'    => 'Teata meili teel, kui minu jälgitavat artiklit muudetakse',
'tog-enotifusertalkpages'     => 'Teata meili teel, kui minu arutelu lehte muudetakse',
'tog-enotifminoredits'        => 'Teata meili teel ka pisiparandustest',
'tog-fancysig'                => 'Kasuta lihtsaid allkirju (ilma linkideta kasutajalehele)',
'tog-externaleditor'          => 'Kasuta vaikimisi välist redaktorit',
'tog-externaldiff'            => 'Kasuta vaikimisi välist võrdlusvahendit (diff)',
'tog-forceeditsummary'        => 'Nõua redigeerimisel resümee välja täitmist',
'tog-watchlisthideown'        => 'Peida minu redaktsioonid jälgimisloendist',
'tog-watchlisthidebots'       => 'Peida robotid jälgimisloendist',
'tog-watchlisthideminor'      => 'Peida pisiparandused jälgimisloendist',
'tog-ccmeonemails'            => 'Saada mulle koopiad e-mailidest, mida ma teistele kasutajatele saadan',
'tog-showhiddencats'          => 'Näita peidetud kategooriaid',

'underline-always'  => 'Alati',
'underline-never'   => 'Mitte kunagi',
'underline-default' => 'Brauseri vaikeväärtus',

'skinpreview' => '(Eelvaade)',

# Dates
'sunday'        => 'pühapäev',
'monday'        => 'esmaspäev',
'tuesday'       => 'teisipäev',
'wednesday'     => 'kolmapäev',
'thursday'      => 'neljapäev',
'friday'        => 'reede',
'saturday'      => 'laupäev',
'sun'           => 'P',
'mon'           => 'E',
'tue'           => 'T',
'wed'           => 'K',
'thu'           => 'N',
'fri'           => 'R',
'sat'           => 'L',
'january'       => 'jaanuar',
'february'      => 'veebruar',
'march'         => 'märts',
'april'         => 'aprill',
'may_long'      => 'mai',
'june'          => 'juuni',
'july'          => 'juuli',
'august'        => 'august',
'september'     => 'september',
'october'       => 'oktoober',
'november'      => 'november',
'december'      => 'detsember',
'january-gen'   => 'jaanuari',
'february-gen'  => 'veebruari',
'march-gen'     => 'märtsi',
'april-gen'     => 'aprilli',
'may-gen'       => 'mai',
'june-gen'      => 'juuni',
'july-gen'      => 'juuli',
'august-gen'    => 'augusti',
'september-gen' => 'septembri',
'october-gen'   => 'oktoobri',
'november-gen'  => 'novembri',
'december-gen'  => 'detsembri',
'jan'           => 'jaan',
'feb'           => 'veebr',
'mar'           => 'märts',
'apr'           => 'apr',
'may'           => 'mai',
'jun'           => 'juuni',
'jul'           => 'juuli',
'aug'           => 'aug',
'sep'           => 'sept',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dets',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategooria|Kategooriad}}',
'category_header'                => 'Artiklid kategooriast "$1"',
'subcategories'                  => 'Allkategooriad',
'category-media-header'          => 'Meediafailid kategooriast "$1"',
'category-empty'                 => "''Selles kategoorias pole ühtegi artiklit ega meediafaili.''",
'hidden-categories'              => '{{PLURAL:$1|Peidetud kategooria|Peidetud kategooriad}}',
'hidden-category-category'       => 'Peidetud kategooriad', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Sellel kategoorial on ainult järgmine allkategooria.|Sellel kategoorial on {{PLURAL:$1|järgmine allkategooria|järgmised $1 allkategooriat}}, (kokku $2).}}',
'category-subcat-count-limited'  => 'Sellel kategoorial on {{PLURAL:$1|järgmine allkategooria|järgmised $1 allkategooriat}}.',
'category-article-count'         => '{{PLURAL:$2|Antud kategoorias on ainult järgmine lehekülg.|Antud kategoorias on {{PLURAL:$1|järgmine lehekülg|järgmised $1 lehekülge}} (kokku $2).}}',
'category-article-count-limited' => 'Antud kategoorias on {{PLURAL:$1|järgmine lehekülg|järgmised $1 lehekülge}}.',
'category-file-count'            => '{{PLURAL:$2|Selles kategoorias on ainult järgmine fail.|{{PLURAL:$1|Järgmine fail |Järgmised $1 faili}} on selles kategoorias (kokku $2).}}',
'category-file-count-limited'    => '{{PLURAL:$1|Järgmine fail|Järgmised $1 faili}} on selles kategoorias.',
'listingcontinuesabbrev'         => 'jätk',

'mainpagetext'      => "<big>'''Wiki tarkvara installeeritud.'''</big>",
'mainpagedocfooter' => 'Juhiste saamiseks kasutamise ning konfigureerimise kohta vaata palun inglisekeelset [http://meta.wikimedia.org/wiki/MediaWiki_localisation dokumentatsiooni liidese kohaldamisest]
ning [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide kasutusjuhendit].',

'about'          => 'Tiitelandmed',
'article'        => 'artikkel',
'newwindow'      => '(avaneb uues aknas)',
'cancel'         => 'Tühista',
'qbfind'         => 'Otsi',
'qbbrowse'       => 'Sirvi',
'qbedit'         => 'Redigeeri',
'qbpageoptions'  => 'Lehekülje suvandid',
'qbpageinfo'     => 'Lehekülje andmed',
'qbmyoptions'    => 'Minu suvandid',
'qbspecialpages' => 'Erileheküljed',
'moredotdotdot'  => 'Veel...',
'mypage'         => 'Minu lehekülg',
'mytalk'         => 'Arutelu',
'anontalk'       => 'Arutelu selle IP jaoks',
'navigation'     => 'Navigeerimine',
'and'            => 'ja',

'errorpagetitle'    => 'Viga',
'returnto'          => 'Naase $1 juurde',
'tagline'           => 'Allikas: {{SITENAME}}',
'help'              => 'Juhend',
'search'            => 'Otsi',
'searchbutton'      => 'Otsi',
'go'                => 'Mine',
'searcharticle'     => 'Mine',
'history'           => 'Artikli ajalugu',
'history_short'     => 'Ajalugu',
'info_short'        => 'Info',
'printableversion'  => 'Prinditav versioon',
'permalink'         => 'Püsilink',
'print'             => 'Prindi',
'edit'              => 'redigeeri',
'create'            => 'Loo',
'editthispage'      => 'Redigeeri seda artiklit',
'create-this-page'  => 'Loo see lehekülg',
'delete'            => 'kustuta',
'deletethispage'    => 'Kustuta see artikkel',
'undelete_short'    => 'Taasta {{PLURAL:$1|üks muudatus|$1 muudatust}}',
'protect'           => 'Kaitse',
'protectthispage'   => 'Kaitse seda artiklit',
'unprotect'         => 'Ära kaitse',
'unprotectthispage' => 'Ära kaitse seda artiklit',
'newpage'           => 'Uus artikkel',
'talkpage'          => 'Selle artikli arutelu',
'talkpagelinktext'  => 'Arutelu',
'specialpage'       => 'Erilehekülg',
'personaltools'     => 'Personaalsed tööriistad',
'postcomment'       => 'Postita kommentaar',
'articlepage'       => 'Artiklilehekülg',
'talk'              => 'Arutelu',
'views'             => 'vaatamisi',
'toolbox'           => 'Tööriistakast',
'userpage'          => 'Kasutajalehekülg',
'projectpage'       => 'Metalehekülg',
'imagepage'         => 'Pildilehekülg',
'templatepage'      => 'Mallilehekülg',
'categorypage'      => 'Kategoorialehekülg',
'viewtalkpage'      => 'Arutelulehekülg',
'otherlanguages'    => 'Teised keeled',
'redirectedfrom'    => '(Ümber suunatud artiklist $1)',
'redirectpagesub'   => 'Ümbersuunamisleht',
'lastmodifiedat'    => 'Viimane muutmine: $2, $1', # $1 date, $2 time
'viewcount'         => 'Seda lehekülge on külastatud {{PLURAL:$1|üks kord|$1 korda}}.',
'protectedpage'     => 'Kaitstud lehekülg',
'jumpto'            => 'Mine:',
'jumptonavigation'  => 'navigeerimiskast',
'jumptosearch'      => 'otsi',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '{{SITENAME}} tiitelandmed',
'aboutpage'            => 'Project:Tiitelandmed',
'bugreports'           => 'Teated programmivigadest',
'bugreportspage'       => 'Project:Teated_programmivigadest',
'copyright'            => 'Kogu tekst on kasutatav litsentsi $1 tingimustel.',
'copyrightpagename'    => '{{SITENAME}} ja autoriõigused',
'copyrightpage'        => '{{ns:project}}:Autoriõigused',
'currentevents'        => 'Sündmused maailmas',
'currentevents-url'    => 'Project:Sündmused maailmas',
'disclaimers'          => 'Hoiatused',
'disclaimerpage'       => 'Project:Hoiatused',
'edithelp'             => 'Redigeerimisjuhend',
'edithelppage'         => 'Help:Kuidas_lehte_redigeerida',
'faq'                  => 'KKK',
'faqpage'              => 'Project:KKK',
'helppage'             => 'Help:Juhend',
'mainpage'             => 'Esileht',
'mainpage-description' => 'Esileht',
'policy-url'           => 'Project:policy',
'portal'               => 'Kogukonnavärav',
'portal-url'           => 'Project:Kogukonnavärav',
'privacy'              => 'Privaatsus',
'privacypage'          => 'Project:Privaatsus',

'badaccess'        => 'Õigus puudub',
'badaccess-group0' => 'Sul ei ole õigust läbi viia toimingut, mida üritasid.',

'retrievedfrom'       => 'Välja otsitud andmebaasist "$1"',
'youhavenewmessages'  => 'Teile on $1 ($2).',
'newmessageslink'     => 'uusi sõnumeid',
'newmessagesdifflink' => 'erinevus eelviimasest redaktsioonist',
'editsection'         => 'redigeeri',
'editold'             => 'redigeeri',
'editsectionhint'     => 'Redigeeri alaosa $1',
'toc'                 => 'Sisukord',
'showtoc'             => 'näita',
'hidetoc'             => 'peida',
'thisisdeleted'       => 'Vaata või taasta $1?',
'viewdeleted'         => 'Vaata lehekülge $1?',
'restorelink'         => '{{PLURAL:$1|üks kustutatud versioon|$1 kustutatud versiooni}}',
'feedlinks'           => 'Sööde:',
'red-link-title'      => '$1 (pole veel kirjutatud)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikkel',
'nstab-user'      => 'Kasutaja leht',
'nstab-media'     => 'Meedia',
'nstab-special'   => 'Eri',
'nstab-project'   => 'Abileht',
'nstab-image'     => 'Pilt',
'nstab-mediawiki' => 'Sõnum',
'nstab-template'  => 'Mall',
'nstab-help'      => 'Juhend',
'nstab-category'  => 'Kategooria',

# Main script and global functions
'nosuchaction'      => 'Sellist toimingut pole.',
'nosuchactiontext'  => 'Wiki ei tunne sellele aadressile vastavat toimingut.',
'nosuchspecialpage' => 'Sellist erilehekülge pole.',
'nospecialpagetext' => 'Viki ei tunne sellist erilehekülge.',

# General errors
'error'                => 'Viga',
'databaseerror'        => 'Andmebaasi viga',
'dberrortext'          => 'Andmebaasipäringus oli süntaksiviga.
Otsingupäring oli ebakorrektne või on tarkvaras viga.
Viimane andmebaasipäring oli:
<blockquote><tt>$1</tt></blockquote>
ja see kutsuti funktsioonist "<tt>$2</tt>".
MySQL andis vea "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Andmebaasipäringus oli süntaksiviga.
Viimane andmebaasipäring oli:
"$1"
ja see kutsuti funktsioonist "$2".
MySQL andis vea "$3: $4".',
'noconnect'            => 'Vabandame! Vikil on tehnilisi probleeme ning ta ei saa andmebaasiserveriga $1 ühendust.',
'nodb'                 => 'Andmebaasi $1 ei õnnestunud kätte saada',
'cachederror'          => 'Järgnev tekst pärineb serveri vahemälust ega pruugi olla lehekülje viimane versioon.',
'readonly'             => 'Andmebaas on hetkel kirjutuskaitse all',
'enterlockreason'      => 'Sisesta lukustamise põhjus ning juurdepääsu taastamise ligikaudne aeg',
'readonlytext'         => 'Andmebaas on praegu kirjutuskaitse all, tõenäoliselt andmebaasi rutiinseks hoolduseks, mille lõppedes normaalne olukord taastub.
Administraator, kes selle kaitse alla võttis, andis järgmise selgituse:
<p>$1',
'internalerror'        => 'Sisemine viga',
'filecopyerror'        => 'Ei saanud faili "$1" kopeerida nimega "$2".',
'filerenameerror'      => 'Ei saanud faili "$1" failiks "$2" ümber nimetada.',
'filedeleteerror'      => 'Faili nimega "$1" ei ole võimalik kustutada.',
'filenotfound'         => 'Faili nimega "$1" ei leitud.',
'unexpected'           => 'Ootamatu väärtus: "$1"="$2".',
'formerror'            => 'Viga: vormi ei saanud salvestada',
'badarticleerror'      => 'Seda toimingut ei saa sellel leheküljel sooritada.',
'cannotdelete'         => 'Seda lehekülge või pilti ei ole võimalik kustutada. (Võib-olla keegi teine juba kustutas selle.)',
'badtitle'             => 'Vigane pealkiri',
'badtitletext'         => 'Küsitud artiklipealkiri oli kas vigane, tühi või siis
valesti viidatud keelte- või wikidevaheline pealkiri.',
'perfdisabled'         => 'Vabandage! See funktsioon ajutiselt ei tööta, sest ta aeglustab andmebaasi kasutamist võimatuseni. Sellepärast täiustatakse vastavat programmi lähitulevikus. Võib-olla teete seda Teie!',
'perfcached'           => 'Järgnevad andmed on puhverdatud ja ei pruugi olla kõige värskemad:',
'perfcachedts'         => 'Järgmised andmed on vahemälus. Viimase uuendamise daatum on $1.',
'wrong_wfQuery_params' => 'Valed parameeterid funktsioonile wfQuery()<br />
Funktsioon: $1<br />
Päring: $2',
'viewsource'           => 'Vaata lähteteksti',
'viewsourcefor'        => '$1',
'viewsourcetext'       => 'Võite vaadata ja kopeerida lehekülje algteksti:',
'protectedinterface'   => 'Sellel leheküljel on tarkvara kasutajaliidese tekst. Kuritahtliku muutmise vältimiseks on lehekülg lukustatud.',
'editinginterface'     => "'''Hoiatus:''' Te redigeerite tarkvara kasutajaliidese tekstiga lehekülge. Muudatused siin mõjutavad kõikide kasutajate kasutajaliidest. Tõlkijad, palun kaaluge MediaWiki tõlkimisprojekti – [http://translatewiki.net/wiki/Main_Page?setlang=et Betawiki] kasutamist.",
'sqlhidden'            => '(SQL päring peidetud)',

# Login and logout pages
'logouttitle'                => 'Väljalogimine',
'logouttext'                 => 'Te olete välja loginud.
Võite kasutada süsteemi anonüümselt, aga ka sama või mõne teise kasutajana uuesti sisse logida.',
'welcomecreation'            => '<h2>Tere tulemast, $1!</h2><p>Teie konto on loodud. Ärge unustage seada oma eelistusi.',
'loginpagetitle'             => 'Sisselogimine',
'yourname'                   => 'Teie kasutajanimi',
'yourpassword'               => 'Teie parool',
'yourpasswordagain'          => 'Sisestage parool uuesti',
'remembermypassword'         => 'Parooli meeldejätmine tulevasteks seanssideks.',
'yourdomainname'             => 'Teie domeen:',
'loginproblem'               => '<b>Sisselogimine ei õnnestunud.</b><br />Proovige uuesti!',
'login'                      => 'Logi sisse',
'nav-login-createaccount'    => 'Logi sisse / registreeru kasutajaks',
'loginprompt'                => 'Teie brauser peab nõustuma küpsistega, et saaksite {{SITENAME}} lehele sisse logida.',
'userlogin'                  => 'Logi sisse / registreeru kasutajaks',
'logout'                     => 'Logi välja',
'userlogout'                 => 'Logi välja',
'notloggedin'                => 'Te pole sisse loginud',
'nologin'                    => 'Sul pole kontot? $1.',
'nologinlink'                => 'Registreeru siin',
'createaccount'              => 'Loo uus konto',
'gotaccount'                 => 'Kui sul on juba konto olemas, siis $1.',
'gotaccountlink'             => 'logi sisse',
'createaccountmail'          => 'meili teel',
'badretype'                  => 'Sisestatud paroolid ei lange kokku.',
'userexists'                 => 'Sisestatud kasutajanimi on juba kasutusel. Valige uus nimi.',
'youremail'                  => 'Teie e-posti aadress*',
'username'                   => 'Kasutajanimi:',
'uid'                        => 'Kasutaja ID:',
'prefs-memberingroups'       => 'Kuulub {{PLURAL:$1|gruppi|gruppidesse}}:',
'yourrealname'               => 'Teie tegelik nimi*',
'yourlanguage'               => 'Keel:',
'yournick'                   => 'Teie hüüdnimi (allakirjutamiseks)',
'email'                      => 'E-post',
'prefs-help-realname'        => '* <strong>Tegelik nimi</strong> (pole kohustuslik): kui otsustate selle avaldada, kasutatakse seda Teie kaastöö seostamiseks Teiega.<br />',
'loginerror'                 => 'Viga sisselogimisel',
'prefs-help-email'           => '* <strong>E-post</strong> (pole kohustuslik): Võimaldab inimestel Teiega veebisaidi kaudu ühendust võtta, ilma et Te peaksite neile oma meiliaadressi avaldama, samuti on sellest kasu, kui unustate parooli.',
'nocookiesnew'               => 'Kasutajakonto loodi, aga sa ei ole sisse logitud, sest {{SITENAME}} kasutab kasutajate tuvastamisel küpsiseid. Sinu brauseris on küpsised keelatud. Palun sea küpsised lubatuks ja logi siis oma vastse kasutajanime ning parooliga sisse.',
'nocookieslogin'             => '{{SITENAME}} kasutab kasutajate tuvastamisel küpsiseid. Sinu brauseris on küpsised keelatud. Palun sea küpsised lubatuks ja proovi siis uuesti.',
'noname'                     => 'Sa ei sisestanud kasutajanime lubataval kujul.',
'loginsuccesstitle'          => 'Sisselogimine õnnestus',
'loginsuccess'               => 'Te olete sisse loginud. Teie kasutajanimi on "$1".',
'nosuchuser'                 => 'Kasutajat nimega "$1" ei ole olemas. Kontrollige kirjapilti või kasutage alljärgnevat vormi uue kasutajakonto loomiseks.',
'nosuchusershort'            => 'Kasutajat nimega "<nowiki>$1</nowiki>" ei ole olemas. Kontrollige kirjapilti.',
'nouserspecified'            => 'Kasutajanimi puudub.',
'wrongpassword'              => 'Vale parool. Proovige uuesti.',
'wrongpasswordempty'         => 'Parool jäi sisestamata. Palun proovi uuesti.',
'passwordtooshort'           => 'Sisestatud parool on vigane või liiga lühike. See peab koosnema vähemalt {{PLURAL:$1|ühest|$1}} tähemärgist ning peab erinema kasutajanimest.',
'mailmypassword'             => 'Saada mulle meili teel uus parool',
'passwordremindertitle'      => '{{SITENAME}} - unustatud salasõna',
'passwordremindertext'       => 'Keegi (tõenäoliselt Teie, IP-aadressilt $1),
palus, et me saadaksime Teile uue parooli süsteemi sisselogimiseks ($4).
Kasutaja "$2" parool on nüüd "$3".
Võiksid sisse logida ja selle ajutise parooli ära muuta.

Sinu {{SITENAME}}.',
'noemail'                    => 'Kasutaja "$1" meiliaadressi meil kahjuks pole.',
'passwordsent'               => 'Uus parool on saadetud kasutaja "$1" registreeritud meiliaadressil.
Pärast parooli saamist logige palun sisse.',
'mailerror'                  => 'Viga kirja saatmisel: $1',
'acct_creation_throttle_hit' => 'Vabandame, aga te olete loonud juba $1 kontot. Rohkem te ei saa.',
'emailauthenticated'         => 'Sinu e-posti aadress kinnitati $1.',
'emailnotauthenticated'      => 'Sinu e-posti aadress <strong>pole veel kinnitatud</strong>. E-posti kinnitamata aadressile ei saadeta.',
'noemailprefs'               => 'Järgnevate võimaluste toimimiseks on vaja sisestada e-posti aadress.',
'emailconfirmlink'           => 'Kinnita oma e-posti aadress',
'loginlanguagelabel'         => 'Keel: $1',

# Edit page toolbar
'bold_sample'     => 'Rasvane kiri',
'bold_tip'        => 'Rasvane kiri',
'italic_sample'   => 'Kaldkiri',
'italic_tip'      => 'Kaldkiri',
'link_sample'     => 'Lingitav pealkiri',
'link_tip'        => 'Siselink',
'extlink_sample'  => 'http://www.example.com Lingi nimi',
'extlink_tip'     => 'Välislink (ärge unustage kasutada http:// eesliidet)',
'headline_sample' => 'Pealkiri',
'headline_tip'    => '2. taseme pealkiri',
'math_sample'     => 'Sisesta valem siia',
'math_tip'        => 'Matemaatiline valem (LaTeX)',
'nowiki_sample'   => 'Sisesta formaatimata tekst',
'nowiki_tip'      => 'Ignoreeri viki vormindust',
'image_sample'    => 'Näidis.jpg',
'image_tip'       => 'Pilt',
'media_sample'    => 'Näidis.mp3',
'media_tip'       => 'Link failile',
'sig_tip'         => 'Sinu signatuur kuupäeva ja kellaajaga',
'hr_tip'          => 'Horisontaalkriips (kasuta säästlikult)',

# Edit pages
'summary'                  => 'Resümee',
'subject'                  => 'Kommentaari pealkiri',
'minoredit'                => 'See on pisiparandus',
'watchthis'                => 'Jälgi seda artiklit',
'savearticle'              => 'Salvesta',
'preview'                  => 'Eelvaade',
'showpreview'              => 'Näita eelvaadet',
'showlivepreview'          => 'Näita eelvaadet',
'showdiff'                 => 'Näita muudatusi',
'anoneditwarning'          => 'Te ei ole sisse logitud. Selle lehe redigeerimislogisse salvestatakse Teie IP-aadress.',
'summary-preview'          => 'Resümee eelvaade',
'blockedtitle'             => 'Kasutaja on blokeeritud',
'blockedtext'              => "<big>'''Teie kasutajanime või IP-aadressi blokeeris $1.'''</big>

Tema põhjendus on järgmine: ''$2''.

* Blokeeringu algus: $8
* Blokeeringu lõpp: $6
* Sooviti blokeerida: $7

Küsimuse arutamiseks võite pöörduda $1 või mõne teise [[{{MediaWiki:Grouppage-sysop}}|administraatori]] poole.

Pange tähele, et Te ei saa sellele kasutajale teadet saata, kui Te pole registreerinud oma [[Special:Preferences|eelistuste lehel]] kehtivat e-posti aadressi.

Teie praegune IP on $3 ning blokeeringu number on #$5. Lisage need andmed kõigile järelpärimistele, mida kavatsete teha.",
'blockednoreason'          => 'põhjendust ei ole kirja pandud',
'whitelistedittitle'       => 'Redigeerimiseks tuleb sisse logida',
'whitelistedittext'        => 'Lehekülgede toimetamiseks peate $1.',
'loginreqtitle'            => 'Vajalik on sisselogimine',
'loginreqlink'             => 'sisse logima',
'loginreqpagetext'         => 'Lehekülgede vaatamiseks peate $1.',
'accmailtitle'             => 'Parool saadetud.',
'accmailtext'              => "Kasutaja '$1' parool saadeti aadressile $2.",
'newarticle'               => '(Uus)',
'newarticletext'           => "Seda lehekülge veel ei ole.
Lehekülje loomiseks hakake kirjutama all olevasse tekstikasti
(lisainfo saamiseks vaadake [[{{MediaWiki:Helppage}}|juhendit]]).
Kui sattusite siia kogemata, klõpsake lihtsalt brauseri ''back''-nupule või lingile ''tühista''.",
'anontalkpagetext'         => "---- ''See on arutelulehekülg anonüümse kasutaja kohta, kes ei ole loonud kontot või ei kasuta seda. Sellepärast tuleb meil kasutaja identifitseerimiseks kasutada tema IP-aadressi. See IP-aadress võib olla mitmele kasutajale ühine. Kui olete anonüümne kasutaja ning leiate, et kommentaarid sellel leheküljel ei ole mõeldud Teile, siis palun [[Special:UserLogin|looge konto või logige sisse]], et edaspidi arusaamatusi vältida.''",
'noarticletext'            => "<div style=\"border: 1px solid #ccc; padding: 7px; background-color: #fff; color: #000\">
'''Sellise pealkirjaga lehekülge ei ole.'''
* <span class=\"plainlinks\">'''[{{fullurl:{{FULLPAGENAME}}|action=edit}} Alusta seda lehekülge]''' või</span>
* <span class=\"plainlinks\">[[{{ns:special}}:Search/{{PAGENAMEE}}|Otsi väljendit \"{{PAGENAME}}]]\" teistest artiklitest või</span>
* [[Special:WhatLinksHere/{{NAMESPACE}}:{{PAGENAMEE}}|Vaata lehekülgi, mis siia viitavad]].
</div>",
'clearyourcache'           => "'''Märkus:''' Pärast salvestamist pead sa muudatuste nägemiseks oma brauseri puhvri tühjendama: '''Mozilla:''' ''ctrl-shift-r'', '''IE:''' ''ctrl-f5'', '''Safari:''' ''cmd-shift-r'', '''Konqueror''' ''f5''.",
'usercssjsyoucanpreview'   => "<strong>Vihje:</strong> Kasuta nuppu 'Näita eelvaadet' oma uue css/js testimiseks enne salvestamist.",
'usercsspreview'           => "'''Ärge unustage, et seda versiooni teie isiklikust stiililehest pole veel salvestatud!'''",
'userjspreview'            => "'''Ärge unustage, et see versioon teie isiklikust javascriptist on alles salvestamata!'''",
'updated'                  => '(Värskendatud)',
'note'                     => '<strong>Meeldetuletus:</strong>',
'previewnote'              => '<strong>Ärge unustage, et see versioon ei ole veel salvestatud!</strong>',
'previewconflict'          => 'See eelvaade näitab, kuidas ülemises toimetuskastis olev tekst hakkab välja nägema, kui otsustate salvestada.',
'editing'                  => 'Redigeerimisel on $1',
'editingsection'           => 'Redigeerimisel on osa leheküljest $1',
'editingcomment'           => 'Lisamisel on $1 kommentaar',
'editconflict'             => 'Redigeerimiskonflikt: $1',
'explainconflict'          => 'Keegi teine on muutnud seda lehekülge pärast seda, kui Teie seda redigeerima hakkasite.
Ülemine toimetuskast sisaldab teksti viimast versiooni.
Teie muudatused on alumises kastis.
Teil tuleb need viimasesse versiooni üle viia.
Kui Te klõpsate nupule
 "Salvesta", siis salvestub <b>ainult</b> ülemises toimetuskastis olev tekst.<br />',
'yourtext'                 => 'Teie tekst',
'storedversion'            => 'Salvestatud redaktsioon',
'editingold'               => '<strong>ETTEVAATUST! Te redigeerite praegu selle lehekülje vana redaktsiooni.
Kui Te selle salvestate, siis lähevad kõik vahepealsed muudatused kaduma.</strong>',
'yourdiff'                 => 'Erinevused',
'copyrightwarning'         => "Pidage silmas, et kõik {{SITENAME}}'le tehtud kaastööd loetakse avaldatuks vastavalt $2 (vaata ka $1). Kui Te ei soovi, et Teie poolt kirjutatut halastamatult redigeeritakse ja omal äranägemisel kasutatakse, siis ärge seda siia salvestage.<br />
Te kinnitate ka, et kirjutasite selle ise või võtsite selle kopeerimiskitsenduseta allikast.<br />
<strong>ÄRGE SAATKE AUTORIÕIGUSEGA KAITSTUD MATERJALI ILMA LOATA!</strong>",
'copyrightwarning2'        => "Pidage silmas, et kõiki {{SITENAME}}'le tehtud kaastöid võidakse muuta või kustutada teiste kaastööliste poolt. Kui Te ei soovi, et Teie poolt kirjutatut halastamatult redigeeritakse, siis ärge seda siia salvestage.<br />
Te kinnitate ka, et kirjutasite selle ise või võtsite selle kopeerimiskitsenduseta allikast (vaata ka $1).<br />
<strong>ÄRGE SAATKE AUTORIÕIGUSEGA KAITSTUD MATERJALI ILMA LOATA!</strong>",
'longpagewarning'          => '<strong>HOIATUS: Selle lehekülje pikkus ületab $1 kilobaiti. Mõne brauseri puhul valmistab raskusi juba 32-le kilobaidile läheneva pikkusega lehekülgede redigeerimine. Palun kaaluge selle lehekülje sisu jaotamist lühemate lehekülgede vahel.</strong>',
'readonlywarning'          => '<strong>HOIATUS: Andmebaas on lukustatud hooldustöödeks, nii et praegu ei saa parandusi salvestada. Võite teksti alal hoida tekstifailina ning salvestada hiljem.</strong>',
'protectedpagewarning'     => '<strong>HOIATUS: See lehekülg on lukustatud, nii et seda saavad redigeerida ainult administraatori õigustega kasutajad.</strong>',
'semiprotectedpagewarning' => "'''Märkus:''' See lehekülg on lukustatud nii, et üksnes registreeritud kasutajad saavad seda muuta.",
'templatesused'            => 'Sellel lehel on kasutusel järgnevad mallid:',
'templatesusedpreview'     => 'Selles eelvaates kasutatakse järgmisi malle:',
'template-protected'       => '(kaitstud)',
'template-semiprotected'   => '(osaliselt kaitstud)',
'hiddencategories'         => 'See lehekülg kuulub {{PLURAL:$1|1 peidetud kategooriasse|$1 peidetud kategooriasse}}:',
'recreate-deleted-warn'    => "'''Hoiatus: Te loote uuesti lehte, mis on varem kustutatud.'''

Kaaluge, kas lehe uuesti loomine on kohane.
Lehe eelnevad kustutamised:",

# "Undo" feature
'undo-success' => 'Selle redaktsiooni käigus tehtud muudatusi saab eemaldada. Palun kontrolli allolevat võrdlust veendumaks, et tahad need muudatused tõepoolest eemaldada. Seejärel saad lehekülje salvestada.',
'undo-summary' => 'Tühistati muudatus $1, mille tegi [[Special:Contributions/$2|$2]] ([[User talk:$2|Arutelu]])',

# History pages
'viewpagelogs'        => 'Vaata selle lehe logisid',
'nohistory'           => 'Sellel leheküljel ei ole eelmisi redaktsioone.',
'revnotfound'         => 'Redaktsiooni ei leitud',
'revnotfoundtext'     => 'Teie poolt päritud vana redaktsiooni ei leitud.
Palun kontrollige aadressi, millel Te seda lehekülge leida püüdsite.',
'currentrev'          => 'Viimane redaktsioon',
'revisionasof'        => 'Redaktsioon: $1',
'previousrevision'    => '←Vanem redaktsioon',
'nextrevision'        => 'Uuem redaktsioon→',
'currentrevisionlink' => 'vaata viimast redaktsiooni',
'cur'                 => 'viim',
'next'                => 'järg',
'last'                => 'eel',
'page_first'          => 'esimene',
'page_last'           => 'viimane',
'histlegend'          => 'Märgi versioonid, mida tahad võrrelda ja vajuta võrdlemisnupule.
Legend: (viim) = erinevused võrreldes viimase redaktsiooniga,
(eel) = erinevused võrreldes eelmise redaktsiooniga, P = pisimuudatus',
'deletedrev'          => '[kustutatud]',
'histfirst'           => 'Esimesed',
'histlast'            => 'Viimased',
'historysize'         => '({{PLURAL:$1|1 bait|$1 baiti}})',
'historyempty'        => '(tühi)',

# Diffs
'history-title'           => '"$1" muudatuste ajalugu',
'difference'              => '(Erinevused redaktsioonide vahel)',
'lineno'                  => 'Rida $1:',
'compareselectedversions' => 'Võrdle valitud redaktsioone',
'editundo'                => 'eemalda',
'diff-multi'              => '({{PLURAL:$1|Ühte vahepealset muudatust|$1 vahepealset muudatust}} ei näidata.)',

# Search results
'searchresults'         => 'Otsingu tulemused',
'searchresulttext'      => 'Lisainfot otsimise kohta vaata [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => 'Päring "[[:$1]]"',
'searchsubtitleinvalid' => 'Päring "$1"',
'noexactmatch'          => "'''Artiklit pealkirjaga \"\$1\" ei leitud.''' Võite [[:\$1|selle artikli luua]].",
'titlematches'          => 'Vasted artikli pealkirjades',
'notitlematches'        => 'Artikli pealkirjades otsitavat ei leitud',
'textmatches'           => 'Vasted artikli tekstides',
'notextmatches'         => 'Artikli tekstides otsitavat ei leitud',
'prevn'                 => 'eelmised $1',
'nextn'                 => 'järgmised $1',
'viewprevnext'          => 'Näita ($1) ($2) ($3).',
'showingresults'        => "Allpool näitame {{PLURAL:$1|'''ühte''' tulemit|'''$1''' tulemit}} alates tulemist #'''$2'''.",
'nonefound'             => '<strong>Märkus</strong>: otsingute ebaõnnestumise sagedaseks põhjuseks on asjaolu,
et väga sageli esinevaid sõnu ei võta süsteem otsimisel arvesse. Teine põhjus võib olla
mitme otsingusõna kasutamine (tulemusena ilmuvad ainult leheküljed, mis sisaldavad kõiki otsingusõnu).',
'powersearch'           => 'Otsi',
'searchdisabled'        => "<p>Vabandage! Otsing vikist on ajutiselt peatatud, et säilitada muude teenuste normaalne töökiirus. Otsimiseks võite kasutada allpool olevat Google'i otsinguvormi, kuid sellelt saadavad tulemused võivad olla vananenud.</p>",

# Preferences page
'preferences'             => 'Eelistused',
'mypreferences'           => 'eelistused',
'prefs-edits'             => 'Redigeerimiste arv:',
'prefsnologin'            => 'Te ei ole sisse loginud',
'prefsnologintext'        => 'Et oma eelistusi seada, [[Special:UserLogin|tuleb Teil]]
sisse logida.',
'prefsreset'              => 'Teie eelistused on arvutimälu järgi taastatud.',
'qbsettings'              => 'Kiirriba sätted',
'qbsettings-none'         => 'Ei_ole',
'qbsettings-fixedleft'    => 'Püsivalt_vasakul',
'qbsettings-fixedright'   => 'Püsivalt paremal',
'qbsettings-floatingleft' => 'Ujuvalt vasakul',
'changepassword'          => 'Muuda parool',
'skin'                    => 'Kujundus',
'math'                    => 'Valemite näitamine',
'dateformat'              => 'Kuupäeva formaat',
'datedefault'             => 'Eelistus puudub',
'datetime'                => 'Kuupäev ja kellaaeg',
'math_failure'            => 'Arusaamatu süntaks',
'math_unknown_error'      => 'Tundmatu viga',
'math_unknown_function'   => 'Tundmatu funktsioon',
'math_lexing_error'       => 'Väljalugemisviga',
'math_syntax_error'       => 'Süntaksiviga',
'prefs-personal'          => 'Kasutaja andmed',
'prefs-rc'                => 'Viimaste muudatuste kuvamine',
'prefs-watchlist'         => 'Jälgimisloend',
'prefs-watchlist-days'    => 'Mitme päeva muudatusi näidata loendis:',
'prefs-watchlist-edits'   => 'Mitu muudatust näidatakse laiendatud jälgimisloendis:',
'prefs-misc'              => 'Muud seaded',
'saveprefs'               => 'Salvesta eelistused',
'resetprefs'              => 'Lähtesta eelistused',
'oldpassword'             => 'Vana parool',
'newpassword'             => 'Uus parool',
'retypenew'               => 'Sisestage uus parool uuesti',
'textboxsize'             => 'Redigeerimisseaded',
'rows'                    => 'Redaktoriakna ridade arv:',
'columns'                 => 'Veergude arv',
'searchresultshead'       => 'Otsingutulemite sätted',
'resultsperpage'          => 'Tulemeid leheküljel',
'contextlines'            => 'Ridu tulemis',
'contextchars'            => 'Konteksti pikkus real',
'recentchangesdays'       => 'Mitu päeva näidata viimastes muudatustes:',
'recentchangescount'      => 'Pealkirjade arv viimastes muudatustes',
'savedprefs'              => 'Teie eelistused on salvestatud.',
'timezonelegend'          => 'Ajavöönd',
'timezonetext'            => 'Kohaliku aja ja serveri aja (maailmaaja) vahe tundides.',
'localtime'               => 'Kohalik aeg',
'timezoneoffset'          => 'Ajavahe',
'servertime'              => 'Serveri aeg',
'guesstimezone'           => 'Loe aeg brauserist',
'allowemail'              => 'Luba teistel kasutajatel mulle e-posti saata',
'defaultns'               => 'Vaikimisi otsi järgmistest nimeruumidest:',
'default'                 => 'vaikeväärtus',
'files'                   => 'Failid',

# User rights
'userrights'               => 'Kasutaja õiguste muutmine', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Muuda kasutajagruppi',
'userrights-user-editname' => 'Sisesta kasutajatunnus:',
'editusergroup'            => 'Muuda kasutajagruppi',
'editinguser'              => "Redigeerimisel on '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Kasutajagrupi valik',
'saveusergroups'           => 'Salvesta grupi muudatused',
'userrights-groupsmember'  => 'Kuulub gruppi:',
'userrights-no-interwiki'  => 'Sul ei ole luba muuta kasutajaõigusi teistes vikides.',

# Groups
'group'            => 'Grupp:',
'group-bot'        => 'Botid',
'group-sysop'      => 'Administraatorid',
'group-bureaucrat' => 'Bürokraadid',
'group-all'        => '(kõik)',

'group-sysop-member'      => 'Administraator',
'group-bureaucrat-member' => 'Bürokraat',

'grouppage-sysop' => '{{ns:project}}:administraatorid',

# User rights log
'rightslogtext' => 'See on logi kasutajate õiguste muutuste kohta.',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|muudatus|muudatust}}',
'recentchanges'                     => 'Viimased muudatused',
'recentchangestext'                 => 'Jälgige sellel leheküljel viimaseid muudatusi.',
'rcnote'                            => "Allpool on esitatud {{PLURAL:$1|'''1''' muudatus|viimased '''$1''' muudatust}} viimase {{PLURAL:$2|päeva|'''$2''' päeva}} jooksul, seisuga $4, kell $5.",
'rcnotefrom'                        => 'Allpool on esitatud muudatused alates <b>$2</b> (näidatakse kuni <b>$1</b> muudatust).',
'rclistfrom'                        => 'Näita muudatusi alates $1',
'rcshowhideminor'                   => '$1 pisiparandused',
'rcshowhidebots'                    => '$1 robotid',
'rcshowhideliu'                     => '$1 sisseloginud kasutajad',
'rcshowhideanons'                   => '$1 anonüümsed kasutajad',
'rcshowhidemine'                    => '$1 minu parandused',
'rclinks'                           => 'Näita viimast $1 muudatust viimase $2 päeva jooksul<br />$3',
'diff'                              => 'erin',
'hist'                              => 'ajal',
'hide'                              => 'peida',
'show'                              => 'näita',
'minoreditletter'                   => 'P',
'newpageletter'                     => 'U',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|jälgiv kasutaja|jälgivat kasutajat}}]',
'newsectionsummary'                 => '/* $1 */ uus alajaotus',

# Recent changes linked
'recentchangeslinked' => 'Seotud muudatused',

# Upload
'upload'               => 'Faili üleslaadimine',
'uploadbtn'            => 'Lae fail',
'reupload'             => 'Uuesti üleslaadimine',
'reuploaddesc'         => 'Tagasi üleslaadimise vormi juurde.',
'uploadnologin'        => 'sisse logimata',
'uploadnologintext'    => 'Kui Te soovite faile üles laadida, peate [[Special:UserLogin|sisse logima]].',
'uploaderror'          => 'Faili laadimine ebaõnnestus',
'uploadtext'           => '<strong>STOPP!</strong> Enne kui sooritad üleslaadimise,
peaksid tagama, et see järgib siinset [[{{MediaWiki:Policy-url}}|piltide kasutamise korda]].

Et näha või leida eelnevalt üleslaetud pilte,
mine vaata [[Special:ImageList|piltide nimekirja]].
Üleslaadimised ning kustutamised logitakse [[Special:Log/upload|üleslaadimise logis]].

Järgneva vormi abil saad laadida üles uusi pilte
oma artiklite illustreerimiseks.
Enamikul brauseritest, näed nuppu "Browse...", mis viib sind
sinu operatsioonisüsteemi standardsesse failiavamisaknasse.
Faili valimisel sisestatakse selle faili nimi tekstiväljale
nupu kõrval.
Samuti pead märgistama kastikese, kinnitades sellega,
et sa ei riku seda faili üleslaadides kellegi autoriõigusi.
Üleslaadimise lõpuleviimiseks vajuta nupule "Üleslaadimine".
See võib võtta pisut aega, eriti kui teil on aeglane internetiühendus.

Eelistatud formaatideks on fotode puhul JPEG , joonistuste
ja ikoonilaadsete piltide puhul PNG, helide jaoks aga OGG.
Nimeta oma failid palun nõnda, et nad kirjeldaksid arusaadaval moel faili sisu, see aitab segadusi vältida.
Pildi lisamiseks artiklile, kasuta linki kujul:
<b><nowiki>[[</nowiki>{{ns:image}}<nowiki>:pilt.jpg]]</nowiki></b> või <b><nowiki>[[</nowiki>{{ns:image}}<nowiki>:pilt.png|alt. tekst]]</nowiki></b>.
Helifaili puhul: <b><nowiki>[[</nowiki>{{ns:media}}<nowiki>:fail.ogg]]</nowiki></b>.

Pane tähele, et nagu ka ülejäänud siinsete lehekülgede puhul,
võivad teised sinu poolt laetud faile saidi huvides
muuta või kustutada ning juhul kui sa süsteemi kuritarvitad
võidakse sinu ligipääs sulgeda.',
'upload-permitted'     => 'Lubatud failitüübid: $1.',
'upload-preferred'     => 'Eelistatud failitüübid: $1.',
'upload-prohibited'    => 'Keelatud failitüübid: $1.',
'uploadlog'            => 'üleslaadimise logi',
'uploadlogpage'        => 'Üleslaadimise logi',
'uploadlogpagetext'    => 'Allpool on loend viimastest failide üleslaadimistest. Kõik ajad näidatakse serveri aja järgi.',
'filename'             => 'Faili nimi',
'filedesc'             => 'Lühikirjeldus',
'fileuploadsummary'    => 'Info faili kohta:',
'uploadedfiles'        => 'Üleslaaditud failid',
'ignorewarning'        => 'Ignoreeri hoiatust ja salvesta fail hoiatusest hoolimata',
'ignorewarnings'       => 'Ignoreeri hoiatusi',
'illegalfilename'      => 'Faili "$1" nimi sisaldab sümboleid, mis pole pealkirjades lubatud. Palun nimetage fail ümber ja proovige uuesti.',
'badfilename'          => 'Pildi nimi on muudetud. Uus nimi on "$1".',
'filetype-banned-type' => "'''\".\$1\"''' ei ole lubatud failitüüp.  Lubatud failitüübid on \$2.",
'large-file'           => 'On soovitatav, et üleslaetavad failid ei oleks suuremad kui $1; selle faili suurus on $2.',
'largefileserver'      => 'Antud fail on suurem serverikonfiguratsiooni poolt lubatavast failisuurusest.',
'fileexists'           => 'Sellise nimega fail on juba olemas. Palun kontrollige <strong><tt>$1</tt></strong>, kui te ei ole kindel, kas tahate seda muuta.',
'fileexists-forbidden' => 'Sellise nimega fail on juba olemas, palun pöörduge tagasi ja laadige fail üles mõne teise nime all. [[Image:$1|thumb|center|$1]]',
'successfulupload'     => 'Üleslaadimine õnnestus',
'uploadwarning'        => 'Üleslaadimise hoiatus',
'savefile'             => 'Salvesta fail',
'uploadedimage'        => 'Fail "[[$1]]" on üles laaditud',
'overwroteimage'       => 'üles laaditud uus variant "[[$1]]"',
'uploaddisabled'       => 'Üleslaadimine hetkel keelatud',
'uploaddisabledtext'   => 'Vabandage, faili laadimine pole hetkel võimalik.',
'uploadcorrupt'        => 'Fail on vigane või vale laiendiga. Palun kontrolli faili ja proovi seda uuesti üles laadida.',
'uploadvirus'          => 'Fail sisaldab viirust! Täpsemalt: $1',
'sourcefilename'       => 'Lähtefail:',
'destfilename'         => 'Failinimi vikis:',
'upload-maxfilesize'   => 'Maksimaalne failisuurus: $1',
'watchthisupload'      => 'Jälgi seda lehekülge',

'upload-misc-error' => 'Tundmatu viga üleslaadimisel',

'license'   => 'Litsents:',
'nolicense' => 'pole valitud',

# Special:ImageList
'imagelist' => 'Piltide loend',

# Image description page
'filehist'                  => 'Faili ajalugu',
'filehist-deleteall'        => 'kustuta kõik',
'filehist-deleteone'        => 'kustuta see',
'filehist-current'          => 'viimane',
'filehist-datetime'         => 'Kuupäev/kellaaeg',
'filehist-user'             => 'Kasutaja',
'filehist-dimensions'       => 'Mõõtmed',
'filehist-filesize'         => 'Faili suurus',
'filehist-comment'          => 'Kommentaar',
'imagelinks'                => 'Viited pildile',
'linkstoimage'              => 'Sellele pildile {{PLURAL:$1|viitab järgmine lehekülg|viitavad järgmised leheküljed}}:',
'nolinkstoimage'            => 'Sellele pildile ei viita ükski lehekülg.',
'noimage'                   => 'Sellise nimega faili pole, võite selle $1.',
'noimage-linktext'          => 'üles laadida',
'uploadnewversion-linktext' => 'Lae üles selle faili uus versioon',
'imagepage-searchdupe'      => 'Otsi faili duplikaate',

# File deletion
'filedelete'                  => 'Kustuta $1',
'filedelete-legend'           => 'Kustuta fail',
'filedelete-comment'          => 'Kustutamise põhjus:',
'filedelete-submit'           => 'Kustuta',
'filedelete-success'          => "'''$1''' on kustutatud.",
'filedelete-otherreason'      => 'Muu/täiendav põhjus',
'filedelete-reason-otherlist' => 'Muu põhjus',
'filedelete-reason-dropdown'  => '*Harilikud kustutamise põhjused
** Autoriõiguste rikkumine
** Duplikaat',
'filedelete-edit-reasonlist'  => 'Redigeeri kustutamise põhjuseid',

# MIME search
'mimesearch' => 'MIME otsing',
'mimetype'   => 'MIME tüüp:',

# Unwatched pages
'unwatchedpages' => 'Jälgimata lehed',

# List redirects
'listredirects' => 'Ümbersuunamised',

# Unused templates
'unusedtemplates'     => 'Kasutamata mallid',
'unusedtemplatestext' => 'See lehekülg loetleb kõik mallinimeruumi leheküljed, millele teistelt lehekülgedelt ei viidata. Enne kustutamist palun kontrollige, kas siia pole muid linke.',
'unusedtemplateswlh'  => 'teised lingid',

# Random page
'randompage' => 'Juhuslik artikkel',

# Random redirect
'randomredirect' => 'Juhuslik ümbersuunamine',

# Statistics
'statistics'    => 'Statistika',
'sitestats'     => 'Saidi statistika',
'userstats'     => 'Kasutaja statistika',
'sitestatstext' => "Andmebaas sisaldab kokku {{PLURAL:$1|'''1''' lehekülje|'''$1''' lehekülge}}.
See arv hõlmab ka arutelulehekülgi, abilehekülgi, väga lühikesi lehekülgi (nuppe), ümbersuunamislehekülgi ning muid lehekülgi. Ilma neid arvestamata on vikis praegu {{PLURAL:$2|'''1''' lehekülg|'''$2''' lehekülge}}, mida võib pidada artikliteks.

Üles on laetud '''$8''' {{PLURAL:$8|fail|faili}}.

Alates {{SITENAME}} töösse seadmisest on lehekülgede vaatamisi kokku '''$3''' ja redigeerimisi '''$4'''.
Seega keskmiselt '''$5''' redigeerimist lehekülje kohta ja '''$6''' lehekülje vaatamist ühe redigeerimise kohta.

[http://www.mediawiki.org/wiki/Manual:Job_queue Töö järjekorra] pikkus on '''$7'''.",
'userstatstext' => "Registreeritud [[Special:ListUsers|kasutajate]] arv: '''$1''', kelledest '''$2''' (ehk '''$4%''') on $5 õigused.",

'disambiguations' => 'Täpsustusleheküljed',

'doubleredirects'     => 'Kahekordsed ümbersuunamised',
'doubleredirectstext' => 'Igal real on ära toodud esimene ja teine ümbersuunamisleht ning samuti teise ümbersuunamislehe viide, mis tavaliselt on viiteks, kuhu esimene ümbersuunamisleht peaks otse suunama.',

'brokenredirects'        => 'Vigased ümbersuunamised',
'brokenredirectstext'    => 'Järgmised leheküljed on ümber suunatud olematutele lehekülgedele.',
'brokenredirects-edit'   => '(redigeeri)',
'brokenredirects-delete' => '(kustuta)',

'withoutinterwiki' => 'Keelelinkideta leheküljed',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bait|baiti}}',
'ncategories'             => '$1 {{PLURAL:$1|kategooria|kategooriat}}',
'nlinks'                  => '$1 {{PLURAL:$1|link|linki}}',
'nmembers'                => '$1 {{PLURAL:$1|liige|liiget}}',
'nrevisions'              => '$1 {{PLURAL:$1|redaktsioon|redaktsiooni}}',
'nviews'                  => 'Külastuste arv: $1',
'lonelypages'             => 'Viitamata artiklid',
'lonelypagestext'         => 'Järgmistele lehekülgedele ei ole linki ühelgi Viki leheküljel.',
'uncategorizedpages'      => 'Kategoriseerimata leheküljed',
'uncategorizedcategories' => 'Kategoriseerimata kategooriad',
'uncategorizedimages'     => 'Kategoriseerimata failid',
'uncategorizedtemplates'  => 'Kategoriseerimata mallid',
'unusedcategories'        => 'Kasutamata kategooriad',
'unusedimages'            => 'Kasutamata pildid',
'popularpages'            => 'Loetumad artiklid',
'wantedcategories'        => 'Kõige oodatumad kategooriad',
'wantedpages'             => 'Kõige oodatumad artiklid',
'mostlinked'              => 'Kõige viidatumad leheküljed',
'mostlinkedcategories'    => 'Kõige viidatumad kategooriad',
'mostlinkedtemplates'     => 'Kõige viidatumad mallid',
'mostcategories'          => 'Enim kategoriseeritud artiklid',
'mostimages'              => 'Kõige kasutatumad failid',
'mostrevisions'           => 'Kõige pikema redigeerimislooga artiklid',
'shortpages'              => 'Lühikesed artiklid',
'longpages'               => 'Pikad artiklid',
'deadendpages'            => 'Edasipääsuta artiklid',
'deadendpagestext'        => 'Järgmised leheküljed ei viita ühelegi teisele Viki leheküljele.',
'protectedpages'          => 'Kaitstud leheküljed',
'listusers'               => 'Kasutajad',
'newpages'                => 'Uued leheküljed',
'ancientpages'            => 'Kõige vanemad artiklid',
'move'                    => 'Teisalda',
'movethispage'            => 'Muuda pealkirja',
'unusedimagestext'        => 'Pange palun tähele, et teised veebisaidid võivad linkida failile otselingiga ja seega võivad siin toodud failid olla ikkagi aktiivses kasutuses.',
'unusedcategoriestext'    => 'Need kategooriad pole ühesgi artiklis või teises kategoorias kasutuses.',
'notargettitle'           => 'Puudub sihtlehekülg',
'notargettext'            => 'Sa ei ole esitanud sihtlehekülge ega kasutajat, kelle kallal seda operatsiooni toime panna.',
'pager-newer-n'           => '{{PLURAL:$1|uuem 1|uuemad $1}}',
'pager-older-n'           => '{{PLURAL:$1|vanem 1|vanemad $1}}',

# Book sources
'booksources' => 'Otsi raamatut',

# Special:Log
'specialloguserlabel'  => 'Kasutaja:',
'speciallogtitlelabel' => 'Pealkiri:',
'log'                  => 'Logid',
'all-logs-page'        => 'Kõik logid',
'log-search-legend'    => 'Otsi logisid',
'alllogstext'          => 'See on kombineeritud vaade üleslaadimise, kustutamise, kaitsmise, blokeerimise ja administraatorilogist. Valiku kitsendamiseks vali soovitav logitüüp, sisesta kasutajanimi või huvi pakkuva lehekülge pealkiri.',
'logempty'             => 'Logides vastavad kirjed puuduvad.',

# Special:AllPages
'allpages'          => 'Kõik artiklid',
'alphaindexline'    => '$1 kuni $2',
'nextpage'          => 'Järgmine lehekülg ($1)',
'allpagesfrom'      => 'Näita alates:',
'allarticles'       => 'Kõik artiklid',
'allinnamespace'    => 'Kõik artiklid ($1 nimeruum)',
'allnotinnamespace' => 'Kõik artiklid (mis ei kuulu $1 nimeruumi)',
'allpagesprev'      => 'Eelmised',
'allpagesnext'      => 'Järgmised',
'allpagessubmit'    => 'Näita',

# Special:Categories
'categories'         => 'Kategooriad',
'categoriespagetext' => 'Vikis on järgmised kategooriad.',
'categoriesfrom'     => 'Näita kategooriaid alates:',

# E-mail user
'mailnologintext' => 'Te peate olema [[Special:UserLogin|sisse logitud]] ja teil peab [[Special:Preferences|eelistustes]] olema kehtiv e-posti aadress, et saata teistele kasutajatele e-kirju.',
'emailuser'       => 'Saada sellele kasutajale e-kiri',
'emailpage'       => 'Saada kasutajale e-kiri',
'emailpagetext'   => 'Kui see kasutaja on oma eelistuste lehel sisestanud e-posti aadressi, siis saate alloleva vormi kaudu talle kirja saata. Et kasutaja saaks vastata, täidetakse kirja saatja väli "kellelt" e-posti aadressiga, mille olete sisestanud oma eelistuste lehel.',
'emailfrom'       => 'Kellelt',
'emailto'         => 'Kellele',
'emailsubject'    => 'Pealkiri',
'emailmessage'    => 'Sõnum',
'emailsend'       => 'Saada',
'emailsent'       => 'E-post saadetud',
'emailsenttext'   => 'Teie sõnum on saadetud.',

# Watchlist
'watchlist'            => 'Jälgimisloend',
'mywatchlist'          => 'Jälgimisloend',
'watchlistfor'         => "('''$1''' jaoks)",
'nowatchlist'          => 'Teie jälgimisloend on tühi.',
'watchlistanontext'    => 'Et näha ja muuta oma jälgimisloendit, peate $1.',
'watchnologin'         => 'Ei ole sisse logitud',
'watchnologintext'     => 'Jälgimisloendi muutmiseks peate [[Special:UserLogin|sisse logima]].',
'addedwatch'           => 'Lisatud jälgimisloendile',
'addedwatchtext'       => 'Lehekülg "<nowiki>$1</nowiki>" on lisatud Teie [[Special:Watchlist|jälgimisloendile]].

Edasised muudatused käesoleval lehel ja sellega seotud aruteluküljel reastatakse jälgimisloendis ning [[Special:RecentChanges|viimaste muudatuste lehel]] tuuakse jälgitava lehe pealkiri esile <b>rasvase</b> kirja abil.

Kui tahad seda lehte hiljem jälgimisloendist eemaldada, klõpsa päisenupule "Lõpeta jälgimine".',
'removedwatch'         => 'Jälgimisloendist kustutatud',
'removedwatchtext'     => 'Artikkel "[[:$1]]" on jälgimisloendist kustutatud.',
'watch'                => 'Jälgi',
'watchthispage'        => 'Jälgi seda artiklit',
'unwatch'              => 'Lõpeta jälgimine',
'unwatchthispage'      => 'Ära jälgi',
'notanarticle'         => 'Pole artikkel',
'watchnochange'        => 'Valitud perioodi jooksul ei ole üheski jälgitavas artiklis muudatusi tehtud.',
'watchlist-details'    => '{{PLURAL:$1|$1 lehekülg|$1 lehekülge}} jälgimisloendis (ei arvestata arutelulehekülgi).',
'wlheader-showupdated' => "* Leheküljed, mida on muudetud peale sinu viimast külastust, on '''rasvases kirjas'''",
'watchmethod-list'     => 'jälgitavate lehekülgede viimased muudatused',
'watchlistcontains'    => 'Sinu jälgimisloendis on $1 {{PLURAL:$1|artikkel|artiklit}}.',
'wlnote'               => "Allpool on {{PLURAL:$1|viimane muudatus|viimased '''$1''' muudatust}} viimase {{PLURAL:$2|tunni|'''$2''' tunni}} jooksul.",
'wlshowlast'           => 'Näita viimast $1 tundi $2 päeva. $3',
'watchlist-show-bots'  => 'Näita roboteid',
'watchlist-hide-bots'  => 'Peida robotite parandused',
'watchlist-show-own'   => 'Näita minu redaktsioone',
'watchlist-hide-own'   => 'Peida minu parandused',
'watchlist-show-minor' => 'Näita pisiparandusi',
'watchlist-hide-minor' => 'Peida pisiparandused',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'jälgin...',
'unwatching' => 'Jälgimise lõpetamine...',

'enotif_reset' => 'Märgi kõik lehed loetuks',
'changed'      => 'muudetud',

# Delete/protect/revert
'deletepage'                  => 'Kustuta lehekülg',
'confirm'                     => 'Kinnita',
'excontent'                   => "sisu oli: '$1'",
'excontentauthor'             => "sisu oli: '$1' (ja ainuke kirjutaja oli '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "sisu enne lehekülje tühjendamist: '$1'",
'exblank'                     => 'lehekülg oli tühi',
'delete-confirm'              => 'Kustuta "$1"',
'delete-legend'               => 'Kustuta',
'historywarning'              => 'Hoiatus: leheküljel, mida tahate kustutada, on ajalugu:&nbsp;',
'confirmdeletetext'           => 'Sa oled andmebaasist jäädavalt kustutamas lehte või pilti koos kogu tema ajalooga. Palun kinnita, et sa tahad seda tõepoolest teha, et sa mõistad tagajärgi ja et sinu tegevus on kooskõlas siinse [[{{MediaWiki:Policy-url}}|sisekorraga]].',
'actioncomplete'              => 'Toiming sooritatud',
'deletedtext'                 => '"<nowiki>$1</nowiki>" on kustutatud. $2 lehel on nimekiri viimastest kustutatud lehekülgedest.',
'deletedarticle'              => '"$1" kustutatud',
'dellogpage'                  => 'Kustutatud_leheküljed',
'dellogpagetext'              => 'Allpool on esitatud nimekiri viimastest kustutamistest.
Kõik toodud kellaajad järgivad serveriaega.',
'deletionlog'                 => 'Kustutatud leheküljed',
'reverted'                    => 'Pöörduti tagasi varasemale versioonile',
'deletecomment'               => 'Kustutamise põhjus',
'deleteotherreason'           => 'Muu/täiendav põhjus:',
'deletereasonotherlist'       => 'Muu põhjus',
'deletereason-dropdown'       => '*Harilikud kustutamise põhjused
** Autori palve
** Autoriõiguste rikkumine
** Vandalism',
'delete-edit-reasonlist'      => 'Redigeeri kustutamise põhjuseid',
'rollback'                    => 'Tühista muudatused',
'rollback_short'              => 'Tühista',
'rollbacklink'                => 'tühista',
'rollbackfailed'              => 'Muudatuste tühistamine ebaõnnestus',
'cantrollback'                => 'Ei saa muudatusi tagasi pöörata; viimane kaastööline on artikli ainus autor.',
'editcomment'                 => 'Artikli sisu oli: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Tühistati [[Eri:Contributions/$2|$2]] ([[Kasutaja arutelu:$2|arutelu]]) muudatus ning pöörduti tagasi viimasele muudatusele, mille tegi [[Kasutaja:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Kaitsmise logi',
'protectlogtext'              => 'Allpool on loetletud lehekülgede kaitsmised ja kaitsete eemaldamised. Praegu kaitstud lehekülgi vaata [[Special:ProtectedPages|kaitstud lehtede loetelust]].',
'protectedarticle'            => 'kaitses lehekülje "[[$1]]"',
'unprotectedarticle'          => 'eemaldas lehekülje "[[$1]]" kaitse',
'protect-title'               => '"$1" kaitsmine',
'protect-legend'              => 'Kinnita kaitsmine',
'protectcomment'              => 'Põhjus',
'protect-text'                => 'Siin võite vaadata ja muuta lehekülje <strong><nowiki>$1</nowiki></strong> kaitsesätteid.',
'protect-default'             => '(tavaline)',
'protect-level-autoconfirmed' => 'Ainult registreeritud kasutajad',
'protect-level-sysop'         => 'Ainult administraatorid',
'protect-expiring'            => 'aegub $1 (UTC)',
'restriction-type'            => 'Lubatud:',
'restriction-level'           => 'Kaitsmise tase:',
'minimum-size'                => 'Min suurus',
'maximum-size'                => 'Max suurus:',
'pagesize'                    => '(baiti)',

# Restrictions (nouns)
'restriction-edit' => 'Redigeerimine',
'restriction-move' => 'Teisaldamine',

# Restriction levels
'restriction-level-sysop'         => 'täielikult kaitstud',
'restriction-level-autoconfirmed' => 'poolkaitstud',
'restriction-level-all'           => 'kõik tasemed',

# Undelete
'undelete'               => 'Taasta kustutatud lehekülg',
'undeletepage'           => 'Kuva ja taasta kustutatud lehekülgi',
'viewdeletedpage'        => 'Vaata kustutatud lehekülgi',
'undeletepagetext'       => 'Järgnevad leheküljed on kustutatud, kuis arhiivis
veel olemas, neid saab taastada. Arhiivi sisu vistatakse aegajalt üle parda.',
'undeleteextrahelp'      => "Kogu lehe ja selle ajaloo taastamiseks jätke kõik linnukesed tühjaks ja vajutage '''''Taasta'''''.
Et taastada valikuliselt, tehke linnukesed kastidesse, mida soovite taastada ja vajutage '''''Taasta'''''.
Nupu '''''Tühjenda''''' vajutamine tühjendab põhjusevälja ja eemaldab kõik linnukesed.",
'undeleterevisions'      => 'Arhiveeritud versioone on $1.',
'undeletehistory'        => 'Kui taastate lehekülje, taastuvad kõik versioonid artikli
ajaloona. Kui vahepeal on loodud uus samanimeline lehekülg, ilmuvad taastatud
versioonid varasema ajaloona. Kehtivat versiooni automaatselt välja ei vahetata.',
'undeletehistorynoadmin' => 'See artikkel on kustutatud. Kustutamise põhjus ning selle lehekülje redigeerimislugu enne kustutamist on näha allolevas kokkuvõttes. Artikli kustutamiseelsete redaktsioonide tekst on kättesaadav ainult administraatoritele.',
'undeletebtn'            => 'Taasta',
'undeletereset'          => 'Tühjenda',
'undeletecomment'        => 'Põhjus:',
'undeletedarticle'       => '"$1" taastatud',
'undeletedrevisions'     => '$1 {{PLURAL:$1|redaktsioon|redaktsiooni}} taastatud',
'cannotundelete'         => 'Taastamine ebaõnnestus; keegi teine võis lehe juba taastada.',
'undelete-search-box'    => 'Otsi kustutatud lehekülgi',
'undelete-search-prefix' => 'Näita lehekülgi, mille pealkiri algab nii:',
'undelete-search-submit' => 'Otsi',

# Namespace form on various pages
'namespace'      => 'Nimeruum:',
'invert'         => 'Näita kõiki peale valitud nimeruumi',
'blanknamespace' => '(Artiklid)',

# Contributions
'contributions' => 'Kasutaja kaastööd',
'mycontris'     => 'Kaastöö',
'contribsub2'   => 'Kasutaja "$1 ($2)" jaoks',
'nocontribs'    => 'Antud kriteeriumile vastavaid muudatusi ei leidnud.',
'uctop'         => ' (üles)',
'month'         => 'Alates kuust (ja varasemad):',
'year'          => 'Alates aastast (ja varasemad):',

'sp-contributions-blocklog' => 'Blokeerimise logi',
'sp-contributions-search'   => 'Otsi kaastöid',
'sp-contributions-username' => 'IP aadress või kasutajanimi:',

# What links here
'whatlinkshere'            => 'Lingid siia',
'whatlinkshere-title'      => 'Leheküljed, mis viitavad lehele "$1"',
'linklistsub'              => '(Linkide loend)',
'linkshere'                => "Lehele '''[[:$1]]''' viitavad järgmised leheküljed:",
'nolinkshere'              => "Lehele '''[[:$1]]''' ei viita ükski lehekülg.",
'isredirect'               => 'ümbersuunamislehekülg',
'istemplate'               => 'kasutamine',
'whatlinkshere-prev'       => '{{PLURAL:$1|eelmised|eelmised $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|järgmised|järgmised $1}}',
'whatlinkshere-links'      => '← lingid',
'whatlinkshere-hideredirs' => '$1 ümbersuunamised',

# Block/unblock
'blockip'                 => 'Blokeeri IP-aadress',
'blockip-legend'          => 'Blokeeri kasutaja',
'blockiptext'             => "See vorm on kirjutamisõiguste blokeerimiseks konkreetselt IP-aadressilt.
'''Seda tohib teha ainult vandalismi vältimiseks ning kooskõlas [[{{MediaWiki:Policy-url}}|{{SITENAME}} sisekorraga]]'''.
Kindlasti tuleb täita ka väli \"põhjus\", paigutades sinna näiteks viited konkreetsetele lehekülgedele, mida rikuti.",
'ipaddress'               => 'IP-aadress',
'ipadressorusername'      => 'IP-aadress või kasutajanimi',
'ipbexpiry'               => 'Kehtivus',
'ipbreason'               => 'Põhjus',
'ipbreasonotherlist'      => 'Muul põhjusel',
'ipbanononly'             => 'Blokeeri ainult anonüümsed kasutajad',
'ipbcreateaccount'        => 'Takista konto loomist',
'ipbemailban'             => 'Takista kasutaja poolt e-maili saatmist',
'ipbsubmit'               => 'Blokeeri see aadress',
'ipbother'                => 'Muu tähtaeg',
'ipboptions'              => '2 tundi:2 hours,1 päev:1 day,3 päeva:3 days,1 nädal:1 week,2 nädalat:2 weeks,1 kuu:1 month,3 kuud:3 months,6 kuud:6 months,1 aasta:1 year,igavene:infinite', # display1:time1,display2:time2,...
'ipbotheroption'          => 'muu tähtaeg',
'ipbotherreason'          => 'Muu/täiendav põhjus:',
'ipbwatchuser'            => 'Jälgi selle kasutaja lehekülge ja arutelu',
'badipaddress'            => 'The IP address is badly formed.',
'blockipsuccesssub'       => 'Blokeerimine õnnestus',
'blockipsuccesstext'      => 'IP-aadress "$1" on blokeeritud.
<br />Kehtivaid blokeeringuid vaata [[Special:IPBlockList|blokeeringute nimekirjast]].',
'unblockip'               => 'Lõpeta IP aadressi blokeerimine',
'unblockiptext'           => 'Kasutage allpool olevat vormi redigeerimisõiguste taastamiseks varem blokeeritud IP aadressile.',
'unblocked'               => '[[User:$1|$1]] blokeering võeti maha.',
'unblocked-id'            => 'Blokeerimine $1 on lõpetatud',
'ipblocklist'             => 'Blokeeritud IP-aadresside ja kasutajakontode loend',
'blocklistline'           => '$1, $2 blokeeris $3 ($4)',
'expiringblock'           => 'aegub $1',
'ipblocklist-empty'       => 'Blokeerimiste loend on tühi.',
'blocklink'               => 'blokeeri',
'unblocklink'             => 'lõpeta blokeerimine',
'contribslink'            => 'kaastöö',
'autoblocker'             => 'Autoblokeeritud kuna teie IP aadress on hiljut kasutatud "[[User:$1|$1]]" poolt. $1-le antud bloki põhjus on "\'\'\'$2\'\'\'"',
'blocklogpage'            => 'Blokeerimise logi',
'blocklogentry'           => 'blokeeris "[[$1]]". Blokeeringu aegumistähtaeg on $2 $3',
'blocklogtext'            => 'See on kasutajate blokeerimiste ja blokeeringute eemaldamiste nimekiri. Automaatselt blokeeritud IP aadresse siin ei näidata. Hetkel aktiivsete blokeeringute ja redigeerimiskeeldude nimekirja vaata [[Special:IPBlockList|IP blokeeringute nimekirja]] leheküljelt.',
'unblocklogentry'         => '"$1" blokeerimine lõpetatud',
'block-log-flags-noemail' => 'e-mail blokeeritud',
'proxyblockreason'        => 'Teie IP aadress on blokeeritud, sest see on anonüümne proxy server. Palun kontakteeruga oma internetiteenuse pakkujaga või tehnilise toega ning informeerige neid sellest probleemist.',
'proxyblocksuccess'       => 'Tehtud.',

# Developer tools
'lockdb'              => 'Lukusta andmebaas',
'unlockdb'            => 'Tee andmebaas lukust lahti',
'lockconfirm'         => 'Jah, ma soovin andmebaasi lukustada.',
'unlockconfirm'       => 'Jah, ma tõesti soovin andmebaasi lukust avada.',
'lockbtn'             => 'Võta andmebaas kirjutuskaitse alla',
'unlockbtn'           => 'Taasta andmebaasi kirjutuspääs',
'lockdbsuccesssub'    => 'Andmebaas kirjutuskaitse all',
'unlockdbsuccesssub'  => 'Kirjutuspääs taastatud',
'lockdbsuccesstext'   => 'Andmebaas on nüüd kirjutuskaitse all.
<br />Kui Teie hooldustöö on läbi, ärge unustage kirjutuspääsu taastada!',
'unlockdbsuccesstext' => 'Andmebaasi kirjutuspääs on taastatud.',

# Move page
'move-page-legend'        => 'Teisalda artikkel',
'movepagetext'            => "Allolevat vormi kasutades saate lehekülje ümber nimetada.
Lehekülje ajalugu tõstetakse uue pealkirja alla automaatselt.
Praeguse pealkirjaga leheküljest saab ümbersuunamisleht uuele leheküljele.
Teistes artiklites olevaid linke praeguse nimega leheküljele automaatselt ei muudeta.
Teie kohuseks on hoolitseda, et ei tekiks topeltümbersuunamisi ning et kõik jääks toimima nagu enne ümbernimetamist.

Lehekülge '''ei nimetata ümber''' juhul, kui uue nimega lehekülg on juba olemas. Erandiks on juhud, kui olemasolev lehekülg on tühi või ümbersuunamislehekülg ja sellel pole redigeerimisajalugu.
See tähendab, et te ei saa kogemata üle kirjutada juba olemasolevat lehekülge, kuid saate ebaõnnestunud ümbernimetamise tagasi pöörata.

'''ETTEVAATUST!'''
Võimalik, et kavatsete teha ootamatut ning drastilist muudatust väga loetavasse artiklisse;
enne muudatuse tegemist mõelge palun järele, mis võib olla selle tagajärjeks.",
'movepagetalktext'        => "Koos artiklileheküljega teisaldatakse automaatselt ka arutelulehekülg, '''välja arvatud juhtudel, kui:'''
*liigutate lehekülge ühest nimeruumist teise,
*uue nime all on juba olemas mittetühi arutelulehekülg või
*jätate alumise kastikese märgistamata.

Neil juhtudel teisaldage arutelulehekülg soovi korral eraldi või ühendage ta omal käel uue aruteluleheküljega.",
'movearticle'             => 'Teisalda artiklilehekülg',
'newtitle'                => 'Uue pealkirja alla',
'move-watch'              => 'Jälgi seda lehekülge',
'movepagebtn'             => 'Teisalda artikkel',
'pagemovedsub'            => 'Artikkel on teisaldatud',
'movepage-moved'          => '<big>\'\'\'"$1" teisaldatud pealkirja "$2" alla\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Selle nimega artikkel on juba olemas või pole valitud nimi lubatav. Palun valige uus nimi.',
'cantmove-titleprotected' => 'Lehte ei saa sinna teisaldada, sest uus pealkiri on artikli loomise eest kaitstud',
'talkexists'              => 'Artikkel on teisaldatud, kuid arutelulehekülge ei saanud teisaldada, sest uue nime all on arutelulehekülg juba olemas. Palun ühendage aruteluleheküljed ise.',
'movedto'                 => 'Teisaldatud pealkirja alla:',
'movetalk'                => 'Teisalda ka "arutelu", kui saab.',
'1movedto2'               => 'Lehekülg "[[$1]]" teisaldatud pealkirja "[[$2]]" alla',
'1movedto2_redir'         => 'Lehekülg "[[$1]]" teisaldatud pealkirja "[[$2]]" alla ümbersuunamisega',
'movelogpage'             => 'Teisaldamise logi',
'movelogpagetext'         => 'See logi sisaldab infot lehekülgede teisaldamistest.',
'movereason'              => 'Põhjus',
'revertmove'              => 'taasta',
'delete_and_move'         => 'Kustuta ja teisalda',
'delete_and_move_confirm' => 'Jah, kustuta lehekülg',
'delete_and_move_reason'  => 'Kustutatud, et asemele tõsta teine lehekülg',

# Export
'export' => 'Lehekülgede eksport',

# Namespace 8 related
'allmessages'        => 'Kõik süsteemi sõnumid',
'allmessagesname'    => 'Nimi',
'allmessagesdefault' => 'Vaikimisi tekst',
'allmessagescurrent' => 'Praegune tekst',
'allmessagestext'    => 'See on loend kõikidest kättesaadavatest süsteemi sõnumitest MediaWiki: nimeruumis.
Kui soovid MediaWiki tarkvara tõlkimises osaleda siis vaata lehti [http://www.mediawiki.org/wiki/Localisation MediaWiki Lokaliseerimine] ja [http://translatewiki.net Betawiki].',

# Thumbnails
'thumbnail-more'  => 'Suurenda',
'thumbnail_error' => 'Viga pisipildi loomisel: $1',

# Special:Import
'import'          => 'Lehekülgede import',
'importfailed'    => 'Importimine ebaõnnestus: $1',
'importnosources' => 'Ühtegi transwiki impordiallikat ei ole defineeritud ning ajaloo otseimpordi funktsioon on välja lülitatud.',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Minu kasutaja leht',
'tooltip-pt-anonuserpage'         => 'Selle IP aadressi kasutajaleht',
'tooltip-pt-mytalk'               => 'Minu arutelu leht',
'tooltip-pt-anontalk'             => 'Arutelu sellelt IP aadressilt tehtud muudatuste kohta',
'tooltip-pt-preferences'          => 'Minu eelistused',
'tooltip-pt-watchlist'            => 'Lehekülgede loend, mida jälgid muudatuste osas',
'tooltip-pt-mycontris'            => 'Loend minu kaastöö kohta',
'tooltip-pt-login'                => 'Me julgustame teid sisse logima, kuid see pole kohustuslik.',
'tooltip-pt-anonlogin'            => 'Me julgustame teid sisse logima, kuid see pole kohustuslik.',
'tooltip-pt-logout'               => 'Logi välja',
'tooltip-ca-talk'                 => 'Selle artikli arutelu',
'tooltip-ca-edit'                 => 'Te võite seda lehekülge redigeerida. Palun kasutage enne salvestamist eelvaadet.',
'tooltip-ca-addsection'           => 'Lisa kommentaar arutellu.',
'tooltip-ca-viewsource'           => 'See lehekülg on kaitstud. Te võite kuvada selle koodi.',
'tooltip-ca-history'              => 'Selle lehekülje varasemad versioonid.',
'tooltip-ca-protect'              => 'Kaitse seda lehekülge',
'tooltip-ca-delete'               => 'Kustuta see lehekülg',
'tooltip-ca-undelete'             => 'Taasta tehtud muudatused enne kui see lehekülg kustutati',
'tooltip-ca-move'                 => 'Teisalda see lehekülg teise nime alla.',
'tooltip-ca-watch'                => 'Lisa see lehekülg oma jälgimisloendile',
'tooltip-ca-unwatch'              => 'Eemalda see lehekülg oma jälgimisloendist',
'tooltip-search'                  => 'Otsi vikist',
'tooltip-p-logo'                  => 'Esileht',
'tooltip-n-mainpage'              => 'Mine esilehele',
'tooltip-n-portal'                => 'Projekti kohta, mida te saate teha, kuidas leida informatsiooni jne',
'tooltip-n-currentevents'         => 'Leia informatsiooni sündmuste kohta maailmas',
'tooltip-n-recentchanges'         => 'Vikis tehtud viimaste muudatuste loend.',
'tooltip-n-randompage'            => 'Mine juhuslikule leheküljele',
'tooltip-n-help'                  => 'Kuidas redigeerida.',
'tooltip-t-whatlinkshere'         => 'Kõik Viki leheküljed, mis siia viitavad',
'tooltip-t-recentchangeslinked'   => 'Viimased muudatused lehekülgedel, milledele on siit viidatud',
'tooltip-feed-rss'                => 'Selle lehekülje RSS sööt',
'tooltip-feed-atom'               => 'Selle lehekülje Atom sööt',
'tooltip-t-contributions'         => 'Kuva selle kasutaja kaastööd',
'tooltip-t-emailuser'             => 'Saada sellele kasutajale e-kiri',
'tooltip-t-upload'                => 'Lae üles faile',
'tooltip-t-specialpages'          => 'Erilehekülgede loend',
'tooltip-t-print'                 => 'Selle lehe trükiversioon',
'tooltip-t-permalink'             => 'Püsilink lehe sellele versioonile',
'tooltip-ca-nstab-main'           => 'Näita artiklit',
'tooltip-ca-nstab-user'           => 'Näita kasutaja lehte',
'tooltip-ca-nstab-media'          => 'Näita pildi lehte',
'tooltip-ca-nstab-special'        => 'See on erilehekülg, te ei saa seda redigeerida',
'tooltip-ca-nstab-project'        => 'Näita projekti lehte',
'tooltip-ca-nstab-image'          => 'Näita pildi lehte',
'tooltip-ca-nstab-mediawiki'      => 'Näita süsteemi sõnumit',
'tooltip-ca-nstab-template'       => 'Näita malli',
'tooltip-ca-nstab-help'           => 'Näita abilehte',
'tooltip-ca-nstab-category'       => 'Näita kategooria lehte',
'tooltip-minoredit'               => 'Märgista see pisiparandusena',
'tooltip-save'                    => 'Salvesta muudatused',
'tooltip-preview'                 => 'Näita tehtavaid muudatusi. Palun kasutage seda enne salvestamist!',
'tooltip-diff'                    => 'Näita tehtavaid muudatusi.',
'tooltip-compareselectedversions' => 'Näita erinevusi kahe selle lehe valitud versiooni vahel.',
'tooltip-watch'                   => 'Lisa see lehekülg oma jälgimisloendile',
'tooltip-recreate'                => 'Taasta kustutatud lehekülg',

# Attribution
'anonymous' => '{{SITENAME}} anonüümsed kasutajad',
'siteuser'  => 'Viki kasutaja $1',
'others'    => 'teised',
'siteusers' => 'Viki kasutaja(d) $1',

# Math options
'mw_math_png'    => 'Alati PNG',
'mw_math_simple' => 'Kui väga lihtne, siis HTML, muidu PNG',
'mw_math_html'   => 'Võimaluse korral HTML, muidu PNG',
'mw_math_source' => 'Säilitada TeX (tekstibrauserite puhul)',
'mw_math_modern' => 'Soovitatav moodsate brauserite puhul',
'mw_math_mathml' => 'MathML',

# Browsing diffs
'previousdiff' => '← Eelmised erinevused',
'nextdiff'     => 'Järgmised erinevused →',

# Media information
'mediawarning'         => "'''Hoiatus''': See fail võib sisaldada pahatahtlikku koodi, mille käivitamime võib kahjustada teie arvutisüsteemi.<hr />",
'imagemaxsize'         => 'Maksimaalne faili suurus kirjelduslehekülgedel:',
'thumbsize'            => 'Pisipildi suurus:',
'file-info-size'       => '($1 × $2 pikslit, faili suurus: $3, MIME tüüp: $4)',
'file-nohires'         => '<small>Sellest suuremat pilti pole.</small>',
'svg-long-desc'        => '(SVG fail, algsuurus $1 × $2 pikslit, faili suurus: $3)',
'show-big-image'       => 'Originaalsuurus',
'show-big-image-thumb' => '<small>Selle eelvaate suurus on: $1 × $2 pikselit</small>',

# Special:NewImages
'newimages'             => 'Uute meediafailide galerii',
'imagelisttext'         => 'Failide arv järgnevas loendis: $1. Sorteeritud $2.',
'showhidebots'          => '($1 bottide kaastööd)',
'ilsubmit'              => 'Otsi',
'bydate'                => 'kuupäeva järgi',
'sp-newimages-showfrom' => 'Näita uusi faile alates $2 $1',

# Metadata
'metadata-expand'   => 'Näita täpsemaid detaile',
'metadata-collapse' => 'Peida täpsemad detailid',

# EXIF tags
'exif-software'        => 'Kasutatud tarkvara',
'exif-artist'          => 'Autor',
'exif-exposuretime'    => 'Säriaeg',
'exif-aperturevalue'   => 'Ava',
'exif-brightnessvalue' => 'Heledus',
'exif-focallength'     => 'Fookuskaugus',
'exif-contrast'        => 'Kontrastsus',

'exif-lightsource-10' => 'Pilvine ilm',

# External editor support
'edit-externally'      => 'Töötle faili välise programmiga',
'edit-externally-help' => 'Lisainfot loe leheküljelt [http://www.mediawiki.org/wiki/Manual:External_editors meta:väliste redaktorite kasutamine]',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'kõik',
'imagelistall'     => 'kõik pildid',
'watchlistall2'    => 'Näita kõiki',
'namespacesall'    => 'kõik',
'monthsall'        => 'kõik',

# E-mail address confirmation
'confirmemail'            => 'Kinnita e-posti aadress',
'confirmemail_text'       => 'Enne kui saad e-postiga seotud teenuseid kasutada, pead sa oma e-posti aadressi õigsust kinnitama. Allpool olevale nupule klikkides meilitakse sulle kinnituskood, koodi kinnitamiseks kliki meilis oleval lingil.',
'confirmemail_send'       => 'Meili kinnituskood',
'confirmemail_sent'       => 'Kinnitusmeil saadetud.',
'confirmemail_sendfailed' => 'Kinnitusmeili ei õnnestunud saata. Kontrolli aadressi õigsust.

Mailer returned: $1',
'confirmemail_invalid'    => 'Vigane kinnituskood, kinnituskood võib olla aegunud.',
'confirmemail_needlogin'  => 'Oma e-posti aadressi kinnitamiseks pead sa $1.',
'confirmemail_success'    => 'Sinu e-posti aadress on nüüd kinnitatud. Sa võid sisse logida ning viki imelisest maailma nautida.',
'confirmemail_loggedin'   => 'Sinu e-posti aadress on nüüd kinnitatud.',
'confirmemail_error'      => 'Viga kinnituskoodi salvestamisel.',
'confirmemail_subject'    => '{{SITENAME}}: e-posti aadressi kinnitamine',
'confirmemail_body'       => 'Keegi, ilmselt sa ise, registreeris IP aadressilt $1 saidil {{SITENAME}} kasutajakonto "$2".

Kinnitamaks, et see kasutajakonto tõepoolest kuulub sulle ning aktiveerimaks e-posti teenuseid, ava oma brauseris järgnev link:

$3

Kui see *ei* ole sinu loodud konto, siis ava järgnev link $5 kinnituse tühistamiseks. 

Kinnituskood aegub $4.',

# Delete conflict
'deletedwhileediting' => 'Hoiatus: Sel ajal, kui Te artiklit redigeerisite, on keegi selle kustutanud!',

# HTML dump
'redirectingto' => 'Ümbersuunamine lehele [[:$1]]...',

# Auto-summaries
'autosumm-blank'   => 'Kustutatud kogu lehekülje sisu',
'autosumm-replace' => "Lehekülg asendatud tekstiga '$1'",
'autoredircomment' => 'Ümbersuunamine lehele [[$1]]',
'autosumm-new'     => 'Uus lehekülg: $1',

# Watchlist editor
'watchlistedit-numitems'       => 'Teie jälgimisloendis on {{PLURAL:$1|1 leht|$1 lehte}}, ilma arutelulehtedeta.',
'watchlistedit-noitems'        => 'Teie jälgimisloend ei sisalda ühtegi lehekülge.',
'watchlistedit-normal-title'   => 'Jälgimisloendi redigeerimine',
'watchlistedit-normal-legend'  => 'Jälgimisloendist lehtede eemaldamine',
'watchlistedit-normal-explain' => "Siin on lehed, mis on teie jälgimisloendis.Et lehti eemaldada, tehke vastavatesse kastidesse linnukesed ja vajutage nuppu '''Eemalda valitud lehed'''. Te võite ka [[Special:Watchlist/raw|redigeerida lähtefaili]].",
'watchlistedit-normal-submit'  => 'Eemalda valitud lehed',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 leht|Järgmised $1 lehte}} on Teie jälgimisloendist eemaldatud:',
'watchlistedit-raw-submit'     => 'Uuenda jälgimisloendit',
'watchlistedit-raw-done'       => 'Teie jälgimisloend on uuendatud.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 lehekülg|$1 lehekülge}} lisatud:',

# Watchlist editing tools
'watchlisttools-view' => 'Näita vastavaid muudatusi',
'watchlisttools-edit' => 'Vaata ja redigeeri jälgimisloendit',
'watchlisttools-raw'  => 'Redigeeri lähtefaili',

# Special:Version
'version' => 'Versioon', # Not used as normal message but as header for the special page itself

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Otsi faili duplikaate',
'fileduplicatesearch-legend'   => 'Otsi faili duplikaati',
'fileduplicatesearch-filename' => 'Faili nimi:',
'fileduplicatesearch-submit'   => 'Otsi',

# Special:SpecialPages
'specialpages'                   => 'Erileheküljed',
'specialpages-group-maintenance' => 'Hooldusraportid',
'specialpages-group-other'       => 'Teised erileheküljed',
'specialpages-group-login'       => 'Sisselogimine / registreerumine',
'specialpages-group-changes'     => 'Viimased muudatused ja logid',
'specialpages-group-media'       => 'Failidega seonduv',
'specialpages-group-users'       => 'Kasutajad ja õigused',
'specialpages-group-highuse'     => 'Tihti kasutatud leheküljed',
'specialpages-group-wiki'        => 'Wiki andmed ja tööriistad',

# Special:BlankPage
'blankpage' => 'Tühi leht',

);
