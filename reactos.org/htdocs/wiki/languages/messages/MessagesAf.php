<?php
/** Afrikaans (Afrikaans)
 *
 * @ingroup Language
 * @file
 *
 * @author Adriaan
 * @author Anrie
 * @author Arnobarnard
 * @author Manie
 * @author Naudefj
 * @author SPQRobin
 * @author Spacebirdy
 */

$skinNames = array(
	'standard' => 'Standaard',
	'nostalgia' => 'Nostalgie',
	'cologneblue' => 'Keulen blou',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Spesiaal',
	NS_MAIN           => '',
	NS_TALK           => 'Bespreking',
	NS_USER           => 'Gebruiker',
	NS_USER_TALK      => 'Gebruikerbespreking',
	# NS_PROJECT set by $wgMetaNamespace,
	NS_PROJECT_TALK   => '$1bespreking',
	NS_IMAGE          => 'Beeld',
	NS_IMAGE_TALK     => 'Beeldbespreking',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWikibespreking',
	NS_TEMPLATE       => 'Sjabloon',
	NS_TEMPLATE_TALK  => 'Sjabloonbespreking',
	NS_HELP           => 'Hulp',
	NS_HELP_TALK      => 'Hulpbespreking',
	NS_CATEGORY       => 'Kategorie',
	NS_CATEGORY_TALK  => 'Kategoriebespreking'
);

# South Africa uses space for thousands and comma for decimal
# Reference: AWS Reël 7.4 p. 52, 2002 edition
# glibc is wrong in this respect in some versions
$separatorTransformTable = array( ',' => "\xc2\xa0", '.' => ',' );
$linkTrail = "/^([a-z]+)(.*)\$/sD";

$messages = array(
# User preference toggles
'tog-underline'               => 'Onderstreep skakels.',
'tog-highlightbroken'         => 'Wys gebroke skakels <a href="" class="new">so</a> (andersins: so<a href="" class="internal">?</a>)',
'tog-justify'                 => 'Justeer paragrawe.',
'tog-hideminor'               => 'Moenie klein wysigings in die onlangse wysigingslys wys nie.',
'tog-extendwatchlist'         => 'Brei dophoulys uit om alle toepaslike wysigings te wys',
'tog-usenewrc'                => 'Verbeterde onlangse wysigingslys (vir moderne blaaiers).',
'tog-numberheadings'          => 'Nommer opskrifte outomaties',
'tog-showtoolbar'             => 'Wys redigeergereedskap (vereis JavaScript)',
'tog-editondblclick'          => 'Dubbelkliek om blaaie te wysig (benodig JavaScript).',
'tog-editsection'             => 'Wys [wysig]-skakels vir elke afdeling',
'tog-editsectiononrightclick' => 'Wysig afdeling met regskliek op afdeling se titel (JavaScript)',
'tog-showtoc'                 => 'Wys inhoudsopgawe (by bladsye met meer as drie opskrifte)',
'tog-rememberpassword'        => 'Onthou wagwoord oor sessies.',
'tog-editwidth'               => 'Wysigingsboks met volle wydte.',
'tog-watchcreations'          => 'Voeg bladsye wat ek skep by my dophoulys',
'tog-watchdefault'            => 'Lys nuwe en gewysigde bladsye.',
'tog-watchmoves'              => 'Voeg die bladsye wat ek skuif by my dophoulys',
'tog-watchdeletion'           => 'Voeg bladsye wat ek verwyder by my dophoulys',
'tog-minordefault'            => 'Merk alle wysigings automaties as klein by verstek.',
'tog-previewontop'            => 'Wys voorskou bo wysigingsboks.',
'tog-previewonfirst'          => 'Wys voorskou met eerste wysiging',
'tog-nocache'                 => 'Deaktiveer bladsykasstelsel (Engels: caching)',
'tog-enotifwatchlistpages'    => 'Stuur vir my e-pos met bladsyveranderings',
'tog-enotifusertalkpages'     => 'Stuur vir my e-pos as my eie besprekingsblad verander word',
'tog-enotifminoredits'        => 'Stuur ook e-pos vir klein bladsywysigings',
'tog-enotifrevealaddr'        => 'Stel my e-posadres bloot in kennisgewingspos',
'tog-shownumberswatching'     => 'Wys die aantal gebruikers wat dophou',
'tog-fancysig'                => 'Doodgewone handtekening (sonder outomatiese skakel)',
'tog-externaleditor'          => "Gebruik outomaties 'n eksterne redigeringsprogram",
'tog-externaldiff'            => "Gebruik 'n eksterne vergelykingsprogram (net vir eksperts - benodig spesiale verstellings op jou rekenaar)",
'tog-showjumplinks'           => 'Wys "spring na"-skakels vir toeganklikheid',
'tog-uselivepreview'          => 'Gebruik lewendige voorskou (JavaScript) (eksperimenteel)',
'tog-forceeditsummary'        => "Let my daarop as ek nie 'n opsomming van my wysiging gee nie",
'tog-watchlisthideown'        => 'Versteek my wysigings in dophoulys',
'tog-watchlisthidebots'       => 'Versteek robotwysigings in dophoulys',
'tog-watchlisthideminor'      => 'Versteek klein wysigings van my dophoulys',
'tog-ccmeonemails'            => "Stuur my 'n kopie van die e-pos wat ek aan ander stuur",
'tog-diffonly'                => "Moenie 'n bladsy se inhoud onder die wysigingsverskil wys nie",
'tog-showhiddencats'          => 'Wys versteekte kategorië',

'underline-always'  => 'Altyd',
'underline-never'   => 'Nooit',
'underline-default' => 'Blaaierverstek',

'skinpreview' => '(Voorskou)',

# Dates
'sunday'        => 'Sondag',
'monday'        => 'Maandag',
'tuesday'       => 'Dinsdag',
'wednesday'     => 'Woensdag',
'thursday'      => 'Donderdag',
'friday'        => 'Vrydag',
'saturday'      => 'Saterdag',
'sun'           => 'So',
'mon'           => 'Ma',
'tue'           => 'Di',
'wed'           => 'Wo',
'thu'           => 'Do',
'fri'           => 'Vr',
'sat'           => 'Sa',
'january'       => 'Januarie',
'february'      => 'Februarie',
'march'         => 'Maart',
'april'         => 'April',
'may_long'      => 'Mei',
'june'          => 'Junie',
'july'          => 'Julie',
'august'        => 'Augustus',
'september'     => 'September',
'october'       => 'Oktober',
'november'      => 'November',
'december'      => 'Desember',
'january-gen'   => 'Januarie',
'february-gen'  => 'Februarie',
'march-gen'     => 'Maart',
'april-gen'     => 'April',
'may-gen'       => 'Mei',
'june-gen'      => 'Junie',
'july-gen'      => 'Julie',
'august-gen'    => 'Augustus',
'september-gen' => 'September',
'october-gen'   => 'Oktober',
'november-gen'  => 'November',
'december-gen'  => 'Desember',
'jan'           => 'Jan',
'feb'           => 'Feb',
'mar'           => 'Mrt',
'apr'           => 'Apr',
'may'           => 'Mei',
'jun'           => 'Jun',
'jul'           => 'Jul',
'aug'           => 'Aug',
'sep'           => 'Sep',
'oct'           => 'Okt',
'nov'           => 'Nov',
'dec'           => 'Des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorie|Kategorieë}}',
'category_header'                => 'Bladsye in kategorie "$1"',
'subcategories'                  => 'Subkategorieë',
'category-media-header'          => 'Media in kategorie "$1"',
'category-empty'                 => "''Hierdie kategorie bevat geen bladsye of media nie.''",
'hidden-categories'              => '{{PLURAL:$1|Versteekte kategorie|Versteekte kategorië}}',
'hidden-category-category'       => 'Versteekte kategorië', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|Hierdie kategorie het slegs die volgende subkategorie.|Hierdie kategorie het die volgende {{PLURAL:$1|subkategorie|$1 subkategorië}}, uit 'n totaal van $2.}}",
'category-subcat-count-limited'  => 'Hierdie kategorie het die volgende {{PLURAL:$1|subkategorie|$1 subkategorië}}.',
'category-article-count'         => "{{PLURAL:$2|Hierdie kategorie bevat slegs die volgende bladsy.|Die volgende {{PLURAL:$1|bladsy|$1 bladsye}} is in hierdie kategorie, uit 'n totaal van $2.}}",
'category-article-count-limited' => 'Die volgende {{PLURAL:$1|bladsy|$1 bladsye}} is in die huidige kategorie.',
'category-file-count'            => "{{PLURAL:$2|Hierdie kategorie bevat net die volgende lêer.|Die volgende {{PLURAL:$1|lêer|$1 lêers}} is in hierdie kategorie, uit 'n totaal van $2.}}",
'category-file-count-limited'    => 'Die volgende {{PLURAL:$1|lêer|$1 lêers}} is in die huidige kategorie.',
'listingcontinuesabbrev'         => 'vervolg',

'mainpagetext'      => "<big>'''MediaWiki is suksesvol geïnstalleer.'''</big>",
'mainpagedocfooter' => "Konsulteer '''[http://meta.wikimedia.org/wiki/Help:Contents User's Guide]''' vir inligting oor hoe om die wikisagteware te gebruik.

== Hoe om te Begin ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'Omtrent',
'article'        => 'Inhoudbladsy',
'newwindow'      => '(verskyn in nuwe venster)',
'cancel'         => 'Kanselleer',
'qbfind'         => 'Vind',
'qbbrowse'       => 'Snuffel',
'qbedit'         => 'Wysig',
'qbpageoptions'  => 'Bladsyopsies',
'qbpageinfo'     => 'Bladsyinligting',
'qbmyoptions'    => 'My opsies',
'qbspecialpages' => 'Spesiale bladsye',
'moredotdotdot'  => 'Meer...',
'mypage'         => 'My bladsy',
'mytalk'         => 'My besprekings',
'anontalk'       => 'Besprekingsblad vir hierdie IP',
'navigation'     => 'Navigasie',
'and'            => 'en',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fout',
'returnto'          => 'Keer terug na $1.',
'tagline'           => 'Vanuit {{SITENAME}}',
'help'              => 'Hulp',
'search'            => 'Soek',
'searchbutton'      => 'Soek',
'go'                => 'Wys',
'searcharticle'     => 'Wys',
'history'           => 'Ouer weergawes',
'history_short'     => 'Geskiedenis',
'updatedmarker'     => 'opgedateer sedert my laaste besoek',
'info_short'        => 'Inligting',
'printableversion'  => 'Drukbare weergawe',
'permalink'         => 'Permanente skakel',
'print'             => 'Druk',
'edit'              => 'Wysig',
'create'            => 'Skep',
'editthispage'      => 'Wysig hierdie bladsy',
'create-this-page'  => 'Skep hierdie bladsy',
'delete'            => 'Skrap',
'deletethispage'    => 'Skrap bladsy',
'undelete_short'    => 'Herstel {{PLURAL:$1|een wysiging|$1 wysigings}}',
'protect'           => 'Beskerm',
'protect_change'    => 'wysig',
'protectthispage'   => 'Beskerm hierdie bladsy',
'unprotect'         => 'Verwyder beskerming',
'unprotectthispage' => 'Verwyder beskerming',
'newpage'           => 'Nuwe bladsy',
'talkpage'          => 'Bespreek hierdie bladsy',
'talkpagelinktext'  => 'Besprekings',
'specialpage'       => 'Spesiale bladsy',
'personaltools'     => 'Persoonlike gereedskap',
'postcomment'       => 'Lewer kommentaar',
'articlepage'       => 'Lees artikel',
'talk'              => 'Bespreking',
'views'             => 'Aansigte',
'toolbox'           => 'Gereedskap',
'userpage'          => 'Lees gebruikersbladsy',
'projectpage'       => 'Lees metabladsy',
'imagepage'         => 'Lees bladsy oor prent',
'mediawikipage'     => 'Bekyk boodskapsbladsy',
'templatepage'      => 'Bekyk sjabloonsbladsy',
'viewhelppage'      => 'Bekyk hulpbladsy',
'categorypage'      => 'Bekyk kategorieblad',
'viewtalkpage'      => 'Lees bespreking',
'otherlanguages'    => 'Ander tale',
'redirectedfrom'    => '(Aangestuur vanaf $1)',
'redirectpagesub'   => 'Aanstuurblad',
'lastmodifiedat'    => 'Laaste wysiging op $2, $1.', # $1 date, $2 time
'viewcount'         => 'Hierdie bladsy is al {{PLURAL:$1|keer|$1 kere}} aangevra.',
'protectedpage'     => 'Beskermde bladsy',
'jumpto'            => 'Spring na:',
'jumptonavigation'  => 'navigasie',
'jumptosearch'      => 'soek',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Inligting oor {{SITENAME}}',
'aboutpage'            => 'Project:Omtrent',
'bugreports'           => 'Foutverslae',
'bugreportspage'       => 'Project:Foutverslae',
'copyright'            => 'Teks is beskikbaar onderhewig aan $1.',
'copyrightpagename'    => '{{SITENAME}} kopiereg',
'copyrightpage'        => '{{ns:project}}:kopiereg',
'currentevents'        => 'Huidige gebeure',
'currentevents-url'    => 'Project:Huidige gebeure',
'disclaimers'          => 'Voorbehoud',
'disclaimerpage'       => 'Project:Voorwaardes',
'edithelp'             => 'Wysighulp',
'edithelppage'         => 'Help:Wysig',
'faq'                  => 'Gewilde vrae',
'faqpage'              => 'Project:GewildeVrae',
'helppage'             => 'Help:Hulp',
'mainpage'             => 'Tuisblad',
'mainpage-description' => 'Tuisblad',
'policy-url'           => 'Project:Beleid',
'portal'               => 'Gebruikersportaal',
'portal-url'           => 'Project:Gebruikersportaal',
'privacy'              => 'Privaatheidsbeleid',
'privacypage'          => 'Project:Privaatheidsbeleid',

'badaccess'        => 'Toestemmingsfout',
'badaccess-group0' => 'U is nie toegelaat om die aksie uit te voer wat U aangevra het nie.',
'badaccess-group1' => 'Die gevraagde aksie is beperk tot gebruikers in die $1 groep.',
'badaccess-group2' => 'Die aksie wat U aangevra het is beperk tot gebruikers in een van die groepe $1.',
'badaccess-groups' => 'Die aksie wat U aangevra het is beperk tot gebruikers in een van die groepe $1.',

'versionrequired'     => 'Weergawe $1 van MediaWiki benodig',
'versionrequiredtext' => 'Weergawe $1 van MediaWiki word benodig om hierdie bladsy te gebruik. Sien [[Special:Version|version page]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Ontsluit van "$1"',
'youhavenewmessages'      => 'U het $1 (sien $2).',
'newmessageslink'         => 'nuwe boodskappe',
'newmessagesdifflink'     => 'die laaste wysiging',
'youhavenewmessagesmulti' => 'U het nuwe boodskappe op $1',
'editsection'             => 'wysig',
'editold'                 => 'wysig',
'viewsourceold'           => 'bekyk bronteks',
'editsectionhint'         => 'Wysig afdeling: $1',
'toc'                     => 'Inhoud',
'showtoc'                 => 'wys',
'hidetoc'                 => 'versteek',
'thisisdeleted'           => 'Bekyk of herstel $1?',
'viewdeleted'             => 'Bekyk $1?',
'restorelink'             => '{{PLURAL:$1|die geskrapte wysiging|$1 geskrapte wysigings}}',
'feedlinks'               => 'Voer:',
'feed-invalid'            => 'Voertipe word nie ondersteun nie.',
'feed-unavailable'        => 'Sindikasievoer is nie beskikbaar op {{SITENAME}}',
'site-rss-feed'           => '$1 RSS-voer',
'site-atom-feed'          => '$1 Atom-voer',
'page-rss-feed'           => '"$1" RSS-voer',
'page-atom-feed'          => '"$1" Atom-voer',
'red-link-title'          => '$1 (nog nie geskryf nie)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Bladsy',
'nstab-user'      => 'Gebruikerblad',
'nstab-media'     => 'Mediablad',
'nstab-special'   => 'Spesiaal',
'nstab-project'   => 'Projekblad',
'nstab-image'     => 'Beeld',
'nstab-mediawiki' => 'Boodskap',
'nstab-template'  => 'Sjabloon',
'nstab-help'      => 'Hulpblad',
'nstab-category'  => 'Kategorie',

# Main script and global functions
'nosuchaction'      => 'Ongeldige aksie',
'nosuchactiontext'  => 'Onbekende aksie deur die adres gespesifeer',
'nosuchspecialpage' => 'Ongeldige spesiale bladsy',
'nospecialpagetext' => 'Ongeldige spesiale bladsy gespesifeer.',

# General errors
'error'                => 'Fout',
'databaseerror'        => 'Databasisfout',
'dberrortext'          => 'Sintaksisfout in databasisnavraag.
Die laaste navraag was:
<blockquote><tt>$1</tt></blockquote>
van funksie "<tt>$2</tt>".
MySQL foutboodskap "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Sintaksisfout in databasisnavraag.
Die laaste navraag was:
<blockquote><tt>$1</tt></blockquote>
van funksie "<tt>$2</tt>".
MySQL foutboodskap "<tt>$3: $4</tt>".',
'noconnect'            => 'Die wiki ondervind tegniese probleme en kon nie na die databasis konnekteer nie.<br />
$1',
'nodb'                 => 'Kon nie databasis $1 selekteer nie',
'cachederror'          => "Die volgende is 'n gekaste kopie van die aangevraagde blad, en is dalk nie op datum nie.",
'laggedslavemode'      => 'Waarskuwing: Onlangse wysigings dalk nie in bladsy vervat nie.',
'readonly'             => 'Databasis gesluit',
'enterlockreason'      => 'Rede vir die sluiting,
en beraming van wanneer ontsluiting sal plaas vind',
'readonlytext'         => 'Die {{SITENAME}} databasis is tans gesluit vir nuwe
artikelwysigings, waarskynlik vir roetine databasisonderhoud,
waarna dit terug sal wees na normaal.
Die administreerder wat dit gesluit het se verduideliking:

$1',
'missing-article'      => "Die databasis kon nie soos verwag die teks vir die bladsy genaamd \"\$1\" \$2 kry nie.

Dit gebeur gewoonlik as mens 'n verouderde verskil- of geskiedenis-skakel volg na 'n bladsy wat reeds verwyder is.

Indien dit nie die geval is nie, het u moontlik 'n fout in die sagteware ontdek. Rapporteer asseblief die probleem aan 'n [[Special:ListUsers/sysop|administrateur]], en maak 'n nota van die URL.",
'missingarticle-rev'   => '(weergawe#: $1)',
'missingarticle-diff'  => '(Wysiging: $1, $2)',
'readonly_lag'         => 'Die databasis is outomaties gesluit terwyl die slaafdatabasisse sinchroniseer met die meester',
'internalerror'        => 'Interne fout',
'internalerror_info'   => 'Interne fout: $1',
'filecopyerror'        => 'Kon nie lêer van "$1" na "$2" kopieer nie.',
'filerenameerror'      => 'Kon nie lêernaam van "$1" na "$2" wysig nie.',
'filedeleteerror'      => 'Kon nie lêer "$1" skrap nie.',
'directorycreateerror' => 'Kon nie gids "$1" skep nie.',
'filenotfound'         => 'Kon nie lêer "$1" vind nie.',
'fileexistserror'      => 'Nie moontlik om na lêer "$1" te skryf: lêer bestaan reeds',
'unexpected'           => 'Onverwagte waarde: "$1"="$2".',
'formerror'            => 'Fout: kon vorm nie stuur nie',
'badarticleerror'      => 'Die aksie kon nie op hierdie bladsy uitgevoer word nie.',
'cannotdelete'         => 'Kon nie die bladsy of prent skrap nie, iemand anders het dit miskien reeds geskrap.',
'badtitle'             => 'Ongeldige titel',
'badtitletext'         => "Die bladsytitel waarvoor gevra is, is ongeldig, leeg, of
'n verkeerd geskakelde tussen-taal of tussen-wiki titel.",
'perfdisabled'         => 'Jammer, hierdie funksie is tydelik afgeskakel omdat dit die databasis soveel verstadig dat dit onbruikbaar vir andere raak.',
'perfcached'           => "Die volgende inligting is 'n gekaste kopie en mag dalk nie volledig op datum wees nie.",
'perfcachedts'         => 'Die volgende data is gekas. Laaste opdatering: $1',
'querypage-no-updates' => 'Opdatering van hierdie bladsy is huidiglik afgeskakel. Inligting hier sal nie tans verfris word nie.',
'wrong_wfQuery_params' => 'Foutiewe parameters na wfQuery()<br />
Funksie: $1<br />
Navraag: $2',
'viewsource'           => 'Bekyk bronteks',
'viewsourcefor'        => 'vir $1',
'actionthrottled'      => 'Outo-rem op aksie uitgevoer',
'actionthrottledtext'  => "As 'n teen-strooi aksie, word U beperk om hierdie aksie te veel keer in 'n kort tyd uit te voer, en U het hierdie limiet oorskry.
Probeer asseblief weer oor 'n paar minute.",
'protectedpagetext'    => 'Hierdie bladsy is beskerm om redigering te verhoed.',
'viewsourcetext'       => 'U kan die bronteks van hierdie bladsy bekyk en wysig:',
'protectedinterface'   => 'Hierdie bladsy verskaf teks vir die koppelvlak van die sagteware, en is beskerm om misbruik te voorkom.',
'editinginterface'     => "'''Waarskuwing:''' U is besig om 'n bladsy te redigeer wat koppelvlakinligting aan die programmatuur voorsien. Wysigings aan hierdie bladsy sal die voorkoms van die gebruikerskoppelvlak vir ander gebruikers beïnvloed. Vir vertalings, oorweeg om eerder [http://translatewiki.net/wiki/Main_Page?setlang=af Betawiki] (die vertalingsprojek vir MediaWiki) te gebruik.",
'sqlhidden'            => '(SQL navraag versteek)',
'cascadeprotected'     => 'Hierdie bladsy is beskerm teen redigering omdat dit ingesluit is in die volgende {{PLURAL:$1|bladsy|bladsye}} wat beskerm is met die "kaskade" opsie aangeskakel: $2',
'namespaceprotected'   => "U het nie toestemming om bladsye in die '''$1'''-naamruimte te wysig nie.",
'customcssjsprotected' => "U het nie toestemming om hierdie bladsy te redigeer nie, want dit bevat 'n ander gebruiker se persoonlike verstellings.",
'ns-specialprotected'  => 'Spesiale bladsye kan nie geredigeer word nie.',
'titleprotected'       => "Hierdie titel is beskerm teen skepping deur [[User:$1|$1]].
Die rede gegee is ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Slegte konfigurasie: onbekende virusskandeerder: <i>$1</i>',
'virus-scanfailed'     => 'skandering het misluk (kode $1)',
'virus-unknownscanner' => 'onbekende antivirus:',

# Login and logout pages
'logouttitle'                => 'Teken uit',
'logouttext'                 => "<strong>U is nou uitgeteken</strong>

U kan aanhou om {{SITENAME}} anoniem te gebruik; of U kan [[Special:Userlogin|inteken]] as dieselfde of 'n ander gebruiker.",
'welcomecreation'            => '<h2>Welkom, $1.</h2>
Jou rekening is geskep;
moenie vergeet om jou persoonlike voorkeure te stel nie.',
'loginpagetitle'             => 'Teken in',
'yourname'                   => 'Gebruikersnaam:',
'yourpassword'               => 'Wagwoord:',
'yourpasswordagain'          => 'Herhaal wagwoord',
'remembermypassword'         => 'Onthou my wagwoord oor sessies.',
'yourdomainname'             => 'U domein:',
'externaldberror'            => "'n Databasis fout het voorgekom tydens aanmelding of jy word nie toegelaat om jou eksterne rekening op te dateer nie.",
'loginproblem'               => '<b>Daar was probleme met jou intekening.</b><br />Probeer weer.',
'login'                      => 'Teken in',
'nav-login-createaccount'    => 'Teken in',
'loginprompt'                => 'U blaaier moet koekies toelaat om by {{SITENAME}} te kan aanteken.',
'userlogin'                  => 'Teken in',
'logout'                     => 'Teken uit',
'userlogout'                 => 'Teken uit',
'notloggedin'                => 'Nie ingeteken nie',
'nologin'                    => 'Nog nie geregistreer nie? $1.',
'nologinlink'                => "Skep gerus 'n rekening",
'createaccount'              => 'Skep nuwe rekening',
'gotaccount'                 => "Het u reeds 'n rekening? $1.",
'gotaccountlink'             => 'Teken in',
'createaccountmail'          => 'deur e-pos',
'badretype'                  => 'Die ingetikte wagwoorde is nie dieselfde nie.',
'userexists'                 => "Die gebruikersnaam wat jy gekies het is reeds geneem.
Kies asseblief 'n ander naam.",
'youremail'                  => 'E-pos',
'username'                   => 'Gebruikersnaam:',
'uid'                        => 'Gebruiker-ID:',
'prefs-memberingroups'       => 'Lid van {{PLURAL:$1|groep|groepe}}:',
'yourrealname'               => 'Regte naam:',
'yourlanguage'               => 'Taal:',
'yournick'                   => 'Bynaam (vir handtekening)',
'badsig'                     => 'Ongeldige handtekening; gaan HTML na.',
'badsiglength'               => 'Die handtekening is te lank. 
Dit moet minder as $1 {{PLURAL:$1|karakter|karakters}} wees.',
'email'                      => 'E-pos',
'prefs-help-realname'        => 'Regte naam (opsioneel): as u hierdie verskaf, kan dit gebruik word om erkenning vir u werk te gee.',
'loginerror'                 => 'Intekenfout',
'prefs-help-email'           => 'E-posadres is opsioneel, maar maak dit moontlik om u wagwoord aan u te pos sou u dit vergeet. 
U kan ook besluit om e-pos te ontvang as ander gebruikers u gebruikers- of besprekingsblad wysig sonder om u identiteit te verraai.',
'prefs-help-email-required'  => 'E-pos adres word benodig.',
'nocookiesnew'               => 'Die gebruikersrekening is geskep, maar u is nie ingeteken nie.
{{SITENAME}} gebruik koekies om gebruikers in te teken.
U rekenaar laat tans nie koekies toe nie.
Stel u rekenaar om dit te aanvaar, dan kan u met u nuwe naam en wagwoord inteken.',
'nocookieslogin'             => '{{SITENAME}} gebruik koekies vir die aanteken van gebruikers, maar u blaaier laat dit nie toe nie. Skakel dit asseblief aan en probeer weer.',
'noname'                     => 'Ongeldige gebruikersnaam.',
'loginsuccesstitle'          => 'Suksesvolle intekening',
'loginsuccess'               => 'U is ingeteken by {{SITENAME}} as "$1".',
'nosuchuser'                 => 'Die gebruiker "$1" bestaan nie. 
Maak seker dit is reg gespel of [[Special:Userlogin/signup|skep \'n nuwe rekening]].',
'nosuchusershort'            => 'Daar is geen gebruikersnaam "<nowiki>$1</nowiki>" nie. Maak seker dit is reg gespel.',
'nouserspecified'            => "U moet 'n gebruikersnaam spesifiseer.",
'wrongpassword'              => 'Ongeldige wagwoord, probeer weer.',
'wrongpasswordempty'         => 'Die wagwoord was leeg. Probeer asseblief weer.',
'passwordtooshort'           => 'U wagwoord is te kort.
Dit moet ten minste {{PLURAL:$1|1 karakter|$1 karakters}} hê en kan nie jou gebruikersnaam insluit nie.',
'mailmypassword'             => "E-pos my 'n nuwe wagwoord",
'passwordremindertitle'      => 'Wagwoordwenk van {{SITENAME}}',
'passwordremindertext'       => 'Iemand (waarskynlik u vanaf IP-adres $1) het \'n nuwe wagwoord vir {{SITENAME}} ($4) gevra. \'n Tydelike wagwoord is vir gebruiker "$2" geskep. Die nuwe wagwoord is "$3". U kan met die tydelike wagwoord aanteken en \'n nuwe wagwoord stel.

Indien iemand anders hierdie navraag gerig het, of u het die wagwoord intussen onthou en wil nie meer die wagwoord wysig nie, kan u die boodskap ignoreer en voortgaan om die ou wagwoord te gebruik.',
'noemail'                    => 'Daar is geen e-posadres vir gebruiker "$1" nie.',
'passwordsent'               => 'Nuwe wagwoord gestuur na e-posadres vir "$1".
Teken asseblief in na jy dit ontvang het.',
'blocked-mailpassword'       => 'U IP-adres is tans teen wysigings geblokkeer. Om verdere misbruik te voorkom is dit dus nie moontlik om die wagwoordherwinningfunksie te gebruik nie.',
'eauthentsent'               => "'n Bevestigingpos is gestuur na die gekose e-posadres.
Voordat ander pos na die adres gestuur word,
moet die instruksies in bogenoemde pos gevolg word om te bevestig dat die adres werklik u adres is.",
'throttled-mailpassword'     => "Daar is reeds 'n wagwoordwenk in die laaste {{PLURAL:$1|uur|$1 ure}} gestuur.
Om misbruik te voorkom, word slegs een wagwoordwenk per {{PLURAL:$1|uur|$1 ure}} gestuur.",
'mailerror'                  => 'Fout tydens e-pos versending: $1',
'acct_creation_throttle_hit' => 'Jammer. U het reeds $1 rekeninge geskep. U kan nie nog skep nie.',
'emailauthenticated'         => 'U e-posadres is bevestig op $1.',
'emailnotauthenticated'      => 'U e-poasadres is <strong>nog nie bevestig nie</strong>. Geen e-pos sal gestuur word vir die volgende funksies nie.',
'noemailprefs'               => "Spesifiseer 'n eposadres vir hierdie funksies om te werk.",
'emailconfirmlink'           => 'Bevestig u e-posadres',
'invalidemailaddress'        => "Die e-posadres is nie aanvaar nie, aangesien dit 'n ongeldige formaat blyk te hê.
Voer asseblief 'n geldige e-posadres in, of laat die veld leeg.",
'accountcreated'             => 'Rekening geskep',
'accountcreatedtext'         => 'Die rekening vir gebruiker $1 is geskep.',
'createaccount-title'        => 'Rekeningskepping vir {{SITENAME}}',
'createaccount-text'         => 'Iemand het \'n rekening vir u e-posadres geskep by {{SITENAME}} ($4), met die naam "$2" en "$3". as die wagwoord.
U word aangeraai om in te teken so gou as moontlik u wagwoord te verander.

Indien hierdie rekening foutief geskep is, kan u hierdie boodskap ignoreer.',
'loginlanguagelabel'         => 'Taal: $1',

# Password reset dialog
'resetpass'               => 'Herstel rekening wagwoord',
'resetpass_announce'      => "U het aangeteken met 'n tydelike e-poskode.
Om voort te gaan moet u 'n nuwe wagwoord hier kies:",
'resetpass_header'        => 'Herstel wagwoord',
'resetpass_submit'        => 'Stel wagwoord en teken in',
'resetpass_success'       => 'U wagwoord is suksesvol gewysig! Besig om U in te teken ...',
'resetpass_bad_temporary' => "Ongeldige tydelike wagwoord. 
U het u wagwoord al gewysig of 'n nuwe tydelike wagwoord aangevra.",
'resetpass_forbidden'     => 'Wagwoorde kannie op {{SITENAME}} gewysig word nie.',
'resetpass_missing'       => "U het nie 'n wagwoord verskaf nie.",

# Edit page toolbar
'bold_sample'     => 'Vet teks',
'bold_tip'        => 'Vetdruk',
'italic_sample'   => 'Skuins teks',
'italic_tip'      => 'Skuinsdruk',
'link_sample'     => 'Skakelnaam',
'link_tip'        => 'Interne skakel',
'extlink_sample'  => 'http://www.example.com skakel se titel',
'extlink_tip'     => 'Eksterne skakel (onthou http:// vooraan)',
'headline_sample' => 'Opskrif',
'headline_tip'    => 'Vlak 2-opskrif',
'math_sample'     => 'Plaas formule hier',
'math_tip'        => 'Wiskundige formule (LaTeX)',
'nowiki_sample'   => 'Plaas ongeformatteerde teks hier',
'nowiki_tip'      => 'Ignoreer wiki-formattering',
'image_sample'    => 'Voorbeeld.jpg',
'image_tip'       => 'Beeld/prentjie/diagram',
'media_sample'    => 'Voorbeeld.ogg',
'media_tip'       => 'Skakel na ander tipe medialêer',
'sig_tip'         => 'Handtekening met datum',
'hr_tip'          => 'Horisontale streep (selde nodig)',

# Edit pages
'summary'                          => 'Opsomming',
'subject'                          => 'Onderwerp/opskrif',
'minoredit'                        => 'Klein wysiging',
'watchthis'                        => 'Hou bladsy dop',
'savearticle'                      => 'Stoor bladsy',
'preview'                          => 'Voorskou',
'showpreview'                      => 'Wys voorskou',
'showlivepreview'                  => 'Lewendige voorskou',
'showdiff'                         => 'Wys veranderings',
'anoneditwarning'                  => "'''Waarskuwing:''' Aangesien u nie aangeteken is nie, sal u IP-adres in dié blad se wysigingsgeskiedenis gestoor word.",
'missingsummary'                   => "'''Onthou:''' Geen opsomming van die wysiging is verskaf nie. As \"Stoor\" weer geklik word, word die wysiging sonder opsomming gestoor.",
'missingcommenttext'               => 'Tik die opsomming onder.',
'missingcommentheader'             => "'''Let op:''' U het geen onderwerp/opskrif vir die opmerking verskaf nie. As u weer op \"Stoor\" klik, sal u wysiging sonder die onderwerp/opskrif gestoor word.",
'summary-preview'                  => 'Opsomming nakijken',
'subject-preview'                  => 'Onderwerp/ opskrif voorskou',
'blockedtitle'                     => 'Gebruiker is geblokkeer',
'blockedtext'                      => "<big>'''U gebruikersnaam of IP-adres is geblokkeer.'''</big>

Die blokkering is deur $1 gedoen.
Die rede gegee is ''$2''.

* Begin van blokkade: $8
* Blokkade eindig: $6
* Blokkering gemik op: $7

U mag $1 of een van die ander [[{{MediaWiki:Grouppage-sysop}}|administreerders]] kontak om dit te bespreek.
U kan nie die 'e-pos hierdie gebruiker' opsie gebruik tensy 'n geldige e-pos adres gespesifiseer is in U [[Special:Preferences|rekening voorkeure]] en U is nie geblokkeer om dit te gebruik nie. 
U huidige IP-adres is $3, en die blokkering ID is #$5. 
Sluit asseblief een of albei hierdie verwysings in by enige navrae.",
'autoblockedtext'                  => "U IP-adres is outomaties geblok omdat dit deur 'n gebruiker gebruik was, wat deur $1 geblokkeer is. 
Die rede verskaf is:

:''$2''

* Aanvang van blok: $8
* Einde van blok: $6
* Bedoelde blokkeerder: $7

U kan die blok met $1 of enige van die [[{{MediaWiki:Grouppage-sysop}}|administrateurs]] bespreek.

Neem kennis dat u slegs die 'e-pos die gebruiker' funksionaliteit kan gebruik as u 'n geldige e-posadres het in u [[Special:Preferences|voorkeure]] het, en die gebruik daarvan is nie ook geblokkeer is nie.

U huidige IP-adres is $3 en die blokkadenommer is #$5.
Vermeld asseblief die bovermelde bloknommer as u die saak rapporteer,",
'blockednoreason'                  => 'geen rede gegeef nie',
'blockedoriginalsource'            => "Die bronteks van '''$1''' word onder gewys:",
'blockededitsource'                => "Die teks van '''jou wysigings''' aan '''$1''' word hieronder vertoon:",
'whitelistedittitle'               => 'U moet aanteken wees om te kan redigeer.',
'whitelistedittext'                => 'U moet $1 om bladsye te wysig.',
'confirmedittitle'                 => 'E-pos-bevestiging nodig om te redigeer',
'confirmedittext'                  => 'U moet u e-posadres bevestig voor u bladsye wysig. Verstel en bevestig asseblief u e-posadres by u [[Special:Preferences|voorkeure]].',
'nosuchsectiontitle'               => 'Afdeling bestaan nie',
'nosuchsectiontext'                => "U probeer 'n afdeling wysig wat nie bestaan nie.
Omdat die afdeling $1 nie bestaan nie, kan u wysigings nie gestoor word nie.",
'loginreqtitle'                    => 'Inteken Benodig',
'loginreqlink'                     => 'teken in',
'loginreqpagetext'                 => 'U moet $1 om ander bladsye te bekyk.',
'accmailtitle'                     => 'Wagwoord gestuur.',
'accmailtext'                      => "Die wagwoord van '$1' is gestuur aan $2.",
'newarticle'                       => '(Nuut)',
'newarticletext'                   => "Die bladsy waarna geskakel is, bestaan nie.
Om 'n nuwe bladsy te skep, tik in die invoerboks hier onder. Lees die [[{{MediaWiki:Helppage}}|hulpbladsy]]
vir meer inligting.
Indien jy per ongeluk hier is, gebruik jou blaaier se '''terug''' knoppie.",
'anontalkpagetext'                 => "----''Hierdie is die besprekingsblad vir 'n anonieme gebruiker wat nog nie 'n rekening geskep het nie of wat dit nie gebruik nie. Daarom moet ons sy/haar numeriese IP-adres gebruik vir identifikasie. Só 'n adres kan deur verskeie gebruikers gedeel word. Indien jy 'n anonieme gebruiker is wat voel dat ontoepaslike kommentaar teen jou gerig is, [[Special:UserLogin|skep 'n rekening of teken in]] om verwarring met ander anonieme gebruikers te voorkom.''",
'noarticletext'                    => 'Daar is tans geen inligting vir hierdie artikel nie. Jy kan [[Special:Search/{{PAGENAME}}|soek vir hierdie bladsytitel]] in ander bladsye of [{{fullurl:{{FULLPAGENAME}}|action=edit}} wysig hierdie bladsy].',
'userpage-userdoesnotexist'        => 'U is besig om \'n gebruikersblad wat nie bestaan nie te wysig (gebruiker "$1"). Maak asseblief seker of u die bladsy wil skep/ wysig.',
'clearyourcache'                   => "'''Let wel''': Na die voorkeure gestoor is, moet u blaaier se kasgeheue verfris word om die veranderinge te sien: '''Mozilla:''' klik ''Reload'' (of ''Ctrl-R''), '''IE / Opera:''' ''Ctrl-F5'', '''Safari:''' ''Cmd-R'', '''Konqueror''' ''Ctrl-R''.",
'usercssjsyoucanpreview'           => '<strong>Wenk:</strong> Gebruik die "Wys voorskou"-knoppie om u nuwe CSS/JS te toets voor u stoor.',
'usercsspreview'                   => "'''Onthou hierdie is slegs 'n voorskou van u persoonlike CSS.'''
'''Dit is nog nie gestoor nie!'''",
'userjspreview'                    => "'''Onthou hierdie is slegs 'n toets/voorskou van u gebruiker-JavaScript, dit is nog nie gestoor nie.'''",
'updated'                          => '(Gewysig)',
'note'                             => '<strong>Nota:</strong>',
'previewnote'                      => "<strong>Onthou dat hierdie slegs 'n voorskou is en nog nie gestoor is nie!</strong>",
'previewconflict'                  => 'Hierdie voorskou vertoon die teks in die boonste teksarea soos dit sou lyk indien jy die bladsy stoor.',
'session_fail_preview'             => '<strong>Jammer! Weens verlies aan sessie-inligting is die wysiging nie verwerk nie.
Probeer asseblief weer. As dit steeds nie werk nie, probeer om [[Special:UserLogout|af te teken]] en dan weer aan te teken.</strong>',
'session_fail_preview_html'        => "<strong>Jammer! U wysigings is nie verwerk nie omdat sessie-data verlore gegaan het.</strong>

''Omrede rou HTML hier by {{SITENAME}} ingevoer kan word, kan die voorskou nie gesien word nie ter beskerming teen aanvalle met JavaScript.''

<strong>As dit 'n regmatige wysiging is, probeer asseblief weer. As dit daarna nog nie werk nie, [[Special:UserLogout|teken dan af]] en weer aan.</strong>",
'editing'                          => 'Besig om $1 te wysig',
'editingsection'                   => 'Besig om $1 (onderafdeling) te wysig',
'editingcomment'                   => 'Besig om $1 (kommentaar) te wysig',
'editconflict'                     => 'Wysigingskonflik: $1',
'explainconflict'                  => 'Iemand anders het hierdie bladsy gewysig sedert jy dit begin verander het.
Die boonste invoerboks het die teks wat tans bestaan.
Jou wysigings word in die onderste invoerboks gewys.
Jy sal jou wysigings moet saamsmelt met die huidige teks.
<strong>Slegs</strong> die teks in die boonste invoerboks sal gestoor word wanneer jy "Stoor bladsy" druk.<br />',
'yourtext'                         => 'Jou teks',
'storedversion'                    => 'Gestoorde weergawe',
'editingold'                       => "<strong>WAARSKUWING: Jy is besig om 'n ouer weergawe van hierdie bladsy te wysig.
As jy dit stoor, sal enige wysigings sedert hierdie een weer uitgewis word.</strong>",
'yourdiff'                         => 'Wysigings',
'copyrightwarning'                 => 'Alle bydraes aan {{SITENAME}} word beskou as beskikbaar gestel onder die $2 (lees $1 vir meer inligting).
As u nie wil toelaat dat u teks deur ander persone gewysig of versprei word nie, moet dit asseblief nie hier invoer nie.<br />
Hierdeur beloof u ons dat u die byvoegings self geskryf het, of gekopieer het van publieke domein of soortgelyke vrye bronne.
<strong>MOENIE WERK WAT DEUR KOPIEREG BESKERM WORD HIER PLAAS SONDER TOESTEMMING NIE!</strong>',
'copyrightwarning2'                => 'Enige bydraes op {{SITENAME}} mag genadeloos gewysig of selfs verwyder word; indien u dit nie met u bydrae wil toelaat nie, moenie dit hier bylas nie.<br />
Deur enigiets hier te plaas, beloof u dat u dit self geskryf het, of dat dit gekopieer is vanuit "publieke domein" of soortgelyke vrye bronne (sien $1 vir details).
<strong>MOENIE WERK WAT DEUR KOPIEREG BESKERM WORD HIER PLAAS SONDER TOESTEMMING NIE!</strong>',
'longpagewarning'                  => 'WAARSKUWING: Hierdie bladsy is $1 kG groot.
Probeer asseblief die bladsy verkort en die detail na subartikels skuif sodat dit nie 32 kG oorskry nie.',
'longpageerror'                    => '<strong>FOUT: die teks wat u bygevoeg het is $1 kilogrepe groot, wat groter is as die maximum van $2 kilogrepe.
Die bladsy kan nie gestoor word nie.</strong>',
'readonlywarning'                  => "<strong>WAARSKUWING: Die databasis is gesluit vir onderhoud. Dus sal u nie nou u wysigings kan stoor nie. Dalk wil u die teks plak in 'n lêer en stoor vir later. </strong>",
'protectedpagewarning'             => '<strong>WAARSKUWING: Hierdie blad is beskerm, en slegs administrateurs kan die inhoud verander.</strong>',
'semiprotectedpagewarning'         => "'''Let wel:''' Hierdie artikel is beskerm sodat slegs ingetekende gebruikers dit kan wysig.",
'cascadeprotectedwarning'          => "'''Waarskuwing:''' Die bladsy was beveilig sodat dit slegs deur administrateurs gewysig kan word, omrede dit ingesluit is in die volgende {{PLURAL:$1|bladsy|bladsye}} wat kaskade-beskerming geniet:",
'titleprotectedwarning'            => '<strong>WAARSKUWING:  Die bladsy is gesluit sodat net sekere gebruikers dit kan skep.</strong>',
'templatesused'                    => 'Sjablone in gebruik op hierdie blad:',
'templatesusedpreview'             => 'Sjablone in hierdie voorskou gebruik:',
'templatesusedsection'             => 'Sjablone gebruik in hierdie afdeling:',
'template-protected'               => '(beskermd)',
'template-semiprotected'           => '(half-beskerm)',
'hiddencategories'                 => "Hierdie bladsy is 'n lid van {{PLURAL:$1|1 versteekte kategorie|$1 versteekte kategorië}}:",
'nocreatetitle'                    => 'Bladsy skepping beperk',
'nocreatetext'                     => '{{SITENAME}} het die skep van nuwe bladsye beperk.
U kan slegs bestaande bladsye wysig, of u kan [[Special:UserLogin|aanteken of registreer]].',
'nocreate-loggedin'                => 'U het nie regte om nuwe blaaie op {{SITENAME}} te skep nie.',
'permissionserrors'                => 'Toestemmings Foute',
'permissionserrorstext'            => 'U het nie toestemming om hierdie te doen nie, om die volgende {{PLURAL:$1|rede|redes}}:',
'permissionserrorstext-withaction' => 'U het geen regte om $2, vir die volgende {{PLURAL:$1|rede|redes}}:',
'recreate-deleted-warn'            => "'''Waarskuwing: U skep 'n bladsy wat vantevore verwyder was.'''

U moet besluit of dit wys is om voort te gaan en aan die bladsy te werk. 
Die verwyderingslogboek vir die blad word hieronder vertoon vir u gerief:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Waarskuwing: Die bladsy gebruik te veel duur ontlederfunksies.

Daar is $1 funksies, terwyl die bladsy minder as $2 moet hê.',
'expensive-parserfunction-category'       => 'Bladsye wat te veel duur ontlederfunkies gebruik',
'post-expand-template-inclusion-category' => 'Bladsye waar die maksimum sjabloon insluit grootte oorskry is',
'post-expand-template-argument-category'  => 'Bladsye met weggelate sjabloonargumente',

# "Undo" feature
'undo-failure' => 'Die wysiging kan nie ongedaan gemaak word nie omdat dit met intermediêre wysigings bots.',
'undo-norev'   => 'Die wysiging kon nie ongedaan gemaak word nie omdat dit nie bestaan nie of reeds verwyder is.',
'undo-summary' => 'Rol weergawe $1 deur [[Special:Contributions/$2|$2]] ([[User talk:$2|bespreek]]) terug.',

# Account creation failure
'cantcreateaccounttitle' => 'Kan nie rekening skep nie',
'cantcreateaccount-text' => "Die registrasie van nuwe rekeninge vanaf die IP-adres ('''$1''') is geblok deur [[User:$3|$3]].

Die rede verskaf deur $3 is ''$2''",

# History pages
'viewpagelogs'        => 'Bekyk logboeke vir hierdie bladsy',
'nohistory'           => 'Daar is geen wysigingsgeskiedenis vir hierdie bladsy nie.',
'revnotfound'         => 'Weergawe nie gevind nie',
'revnotfoundtext'     => 'Die ou weergawe wat jy aangevra het kon nie gevind word nie. Gaan asseblief die URL na wat jy gebruik het.',
'currentrev'          => 'Huidige wysiging',
'revisionasof'        => 'Wysiging soos op $1',
'revision-info'       => 'Weergawe soos op $1 deur $2',
'previousrevision'    => '← Ouer weergawe',
'nextrevision'        => 'Nuwer weergawe →',
'currentrevisionlink' => 'bekyk huidige weergawe',
'cur'                 => 'huidige',
'next'                => 'volgende',
'last'                => 'vorige',
'page_first'          => 'eerste',
'page_last'           => 'laaste',
'histlegend'          => 'Byskrif: (huidige) = verskil van huidige weergawe,
(vorige) = verskil van vorige weergawe, M = klein wysiging',
'deletedrev'          => '[geskrap]',
'histfirst'           => 'Oudste',
'histlast'            => 'Nuutste',
'historysize'         => '({{PLURAL:$1|1 greep|$1 grepe}})',
'historyempty'        => '(leeg)',

# Revision feed
'history-feed-title'          => 'Weergawegeskiedenis',
'history-feed-description'    => 'Wysigingsgeskiedenis vir die bladsy op die wiki',
'history-feed-item-nocomment' => '$1 by $2', # user at time

# Revision deletion
'rev-deleted-comment'       => '(opsomming geskrap)',
'rev-deleted-user'          => '(gebruikersnaam geskrap)',
'rev-deleted-event'         => '(stawingsaksie verwyder)',
'rev-delundel'              => 'wys/versteek',
'revisiondelete'            => 'Verwyder/herstel weergawes',
'revdelete-nooldid-title'   => 'Ongeldige teiken weergawe',
'revdelete-selected'        => 'Geselekteerde {{PLURAL:$2|wysiging|wysigings}} vir [[:$1]]:',
'logdelete-selected'        => 'Geselekteerde {{PLURAL:$1|logboek aksie|logboek aksies}}:',
'revdelete-legend'          => 'Stel sigbaarheid beperkinge',
'revdelete-hide-text'       => 'Steek hersiening teks weg',
'revdelete-hide-name'       => 'Steek aksie en teiken weg',
'revdelete-hide-comment'    => 'Versteek wysigopsomming',
'revdelete-hide-user'       => 'Steek redigeerder se gebruikersnaam/IP weg',
'revdelete-hide-restricted' => 'Pas die beperkings op administrateurs toe en sluit die koppelvlak.',
'revdelete-suppress'        => 'Onderdruk data van administrateurs en ander.',
'revdelete-hide-image'      => 'Steek lêer inhoud weg',
'revdelete-unsuppress'      => 'Verwyder beperkinge op herstelde weergawes',
'revdelete-log'             => 'Boekstaaf opmerking:',
'revdelete-submit'          => 'Pas op gekose weergawe toe',
'revdel-restore'            => 'Verander sigbaarheid',
'pagehist'                  => 'Bladsy geskiedenis',
'deletedhist'               => 'Verwyderde geskiedenis',
'revdelete-content'         => 'inhoud',
'revdelete-summary'         => 'redigeringsopsomming',
'revdelete-uname'           => 'gebruikersnaam',
'revdelete-hid'             => '$1 verskuil',
'revdelete-unhid'           => '$1 onverskuil',
'revdelete-log-message'     => '$1 vir $2 {{PLURAL:$2|weergawe|weergawes}}',
'logdelete-log-message'     => '$1 vir $2 {{PLURAL:$2|gebeurtenis|gebeurtenisse}}',

# Suppression log
'suppressionlog' => 'Verbergingslogboek',

# History merging
'mergehistory'                     => 'Geskiedenis van bladsy samesmeltings',
'mergehistory-box'                 => 'Versmelt weergawes van twee bladsye:',
'mergehistory-from'                => 'Bronbladsy:',
'mergehistory-into'                => 'Bestemmingsbladsy:',
'mergehistory-list'                => 'Versmeltbare wysigingsgeskiedenis',
'mergehistory-go'                  => 'Wys versmeltbare wysigings',
'mergehistory-submit'              => 'Versmelt weergawes',
'mergehistory-empty'               => 'Geen weergawes kan versmelt word nie.',
'mergehistory-success'             => '$3 {{PLURAL:$3|weergawe|weergawes}} van [[:$1]] is suksesvol versmelt met [[:$2]].',
'mergehistory-no-source'           => 'Bronbladsy $1 bestaan nie.',
'mergehistory-no-destination'      => 'Bestemmingsbladsy $1 bestaan nie.',
'mergehistory-invalid-source'      => "Bronbladsy moet 'n geldige titel wees.",
'mergehistory-invalid-destination' => "Bestemmingsbladsy moet 'n geldige titel wees.",
'mergehistory-autocomment'         => '[[:$1]] saamgevoeg by [[:$2]]',
'mergehistory-comment'             => '[[:$1]] saamgevoeg by [[:$2]]: $3',

# Merge log
'mergelog'           => 'Versmeltingslogboek',
'pagemerge-logentry' => 'versmelt [[$1]] met [[$2]] (weergawes tot en met $3)',

# Diffs
'history-title'           => 'Weergawegeskiedenis van "$1"',
'difference'              => '(Verskil tussen weergawes)',
'lineno'                  => 'Lyn $1:',
'compareselectedversions' => 'Vergelyk gekose weergawes',
'editundo'                => 'maak ongedaan',
'diff-multi'              => '({{PLURAL:$1|Een tussenin wysiging|$1 tussenin wysigings}} word nie gewys nie.)',

# Search results
'searchresults'             => 'soekresultate',
'searchresulttext'          => 'Vir meer inligting oor {{SITENAME}} soekresultate, lees [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Vir navraag "[[:$1]]"',
'searchsubtitleinvalid'     => 'Vir navraag "$1"',
'noexactmatch'              => "'''Geen bladsy met die titel \"\$1\" bestaan nie.''' Probeer 'n volteksnavraag of [[:\$1|skep die bladsy]].",
'noexactmatch-nocreate'     => "'''Daar bestaan geen bladsy met titel \"\$1\" nie.'''",
'toomanymatches'            => "Te veel resultate. Probeer asseblief 'n ander soektog.",
'titlematches'              => 'Artikeltitel resultate',
'notitlematches'            => 'Geen artikeltitel resultate nie',
'textmatches'               => 'Artikelteks resultate',
'notextmatches'             => 'Geen artikelteks resultate nie',
'prevn'                     => 'vorige $1',
'nextn'                     => 'volgende $1',
'viewprevnext'              => 'Kyk na ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 woord|$2 woorde}})',
'search-result-score'       => 'Relevansie: $1%',
'search-redirect'           => '(aanstuur $1)',
'search-section'            => '(afdeling $1)',
'search-suggest'            => 'Het U bedoel: $1',
'search-interwiki-caption'  => 'Suster projekte',
'search-interwiki-default'  => '$1 resultate:',
'search-interwiki-more'     => '(meer)',
'search-mwsuggest-enabled'  => 'met voorstelle',
'search-mwsuggest-disabled' => 'geen voorstelle',
'search-relatedarticle'     => 'Verwante',
'mwsuggest-disable'         => 'Deaktiveer AJAX voorstelle',
'searchrelated'             => 'verwante',
'searchall'                 => 'alle',
'showingresults'            => "Hier volg {{PLURAL:$1|'''1''' resultaat|'''$1''' resultate}} wat met #'''$2''' begin.",
'showingresultsnum'         => "Hieronder {{PLURAL:$3|is '''1''' resultaat|is '''$3''' resultate}} vanaf #'''$2'''.",
'showingresultstotal'       => "Hieronder is {{PLURAL:$3|resultaat '''$1''' van '''$3'''|resultate '''$1 - $2''' van '''$3'''}}",
'nonefound'                 => "<strong>Nota</strong>: onsuksesvolle navrae word gewoonlik veroorsaak deur 'n soektog met algemene
woorde wat nie geindekseer word nie, of spesifisering van meer as een woord (slegs blaaie wat alle navraagwoorde
bevat, word gewys).",
'powersearch'               => 'Soek',
'powersearch-legend'        => 'Gevorderde soektog',
'powersearch-ns'            => 'Soek in naamruimtes:',
'powersearch-redir'         => 'Wys aanstuurbladsye',
'powersearch-field'         => 'Soek vir',
'search-external'           => 'Eksterne soektog',
'searchdisabled'            => '{{SITENAME}} se soekfunksie is tans afgeskakel ter wille van werkverrigting. Gebruik gerus intussen Google of Yahoo! Let daarop dat hulle indekse van die {{SITENAME}}-inhoud verouderd mag wees.',

# Preferences page
'preferences'              => 'Voorkeure',
'mypreferences'            => 'My voorkeure',
'prefs-edits'              => 'Aantal wysigings:',
'prefsnologin'             => 'Nie ingeteken nie',
'prefsnologintext'         => 'Jy moet <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} aanteken] om voorkeure te kan verander.',
'prefsreset'               => 'Voorkeure is herstel.',
'qbsettings'               => 'Snelbalkvoorkeure',
'qbsettings-none'          => 'Geen',
'qbsettings-fixedleft'     => 'Links vas.',
'qbsettings-fixedright'    => 'Regs vas.',
'qbsettings-floatingleft'  => 'Dryf links.',
'qbsettings-floatingright' => 'Dryf regs.',
'changepassword'           => 'Verander wagwoord',
'skin'                     => 'Omslag',
'math'                     => 'Wiskunde',
'dateformat'               => 'Datumformaat',
'datedefault'              => 'Geen voorkeur',
'datetime'                 => 'Datum en tyd',
'math_failure'             => 'Kon nie verbeeld nie',
'math_unknown_error'       => 'onbekende fout',
'math_unknown_function'    => 'onbekende funksie',
'math_lexing_error'        => 'leksikale fout',
'math_syntax_error'        => 'sintaksfout',
'prefs-personal'           => 'Gebruikersdata',
'prefs-rc'                 => 'Onlangse wysigings',
'prefs-watchlist'          => 'Dophoulys',
'prefs-watchlist-days'     => 'Aantal dae om in dophoulys te wys:',
'prefs-watchlist-edits'    => 'Aantal wysigings om in uitgebreide dophoulys te wys:',
'prefs-misc'               => 'Allerlei',
'saveprefs'                => 'Stoor voorkeure',
'resetprefs'               => 'Herstel voorkeure',
'oldpassword'              => 'Ou wagwoord',
'newpassword'              => 'Nuwe wagwoord',
'retypenew'                => 'Tik nuwe wagwoord weer in',
'textboxsize'              => 'Wysiging',
'rows'                     => 'Rye',
'columns'                  => 'Kolomme',
'searchresultshead'        => 'Soekresultate',
'resultsperpage'           => 'Aantal resultate om te wys',
'contextlines'             => 'Aantal lyne per resultaat',
'contextchars'             => 'Karakters konteks per lyn',
'recentchangesdays'        => 'Aantal dae wat in onlangse wysigings vertoon word:',
'recentchangescount'       => 'Aantal titels in onlangse wysigings',
'savedprefs'               => 'Jou voorkeure is gestoor.',
'timezonelegend'           => 'Tydsone',
'timezonetext'             => 'Aantal ure waarmee plaaslike tyd van UTC verskil.',
'localtime'                => 'Plaaslike tyd',
'timezoneoffset'           => 'Verplasing¹',
'servertime'               => 'Tyd op die bediener is nou',
'guesstimezone'            => 'Vul in vanaf webblaaier',
'allowemail'               => 'Laat e-pos van ander toe',
'prefs-searchoptions'      => 'Soekopsies',
'prefs-namespaces'         => 'Naamruimtes',
'defaultns'                => 'Verstek naamruimtes vir soektog:',
'default'                  => 'verstek',
'files'                    => 'Lêers',

# User rights
'userrights'                  => 'Bestuur gebruikersregte', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Beheer gebruikersgroepe',
'userrights-user-editname'    => 'Voer gebruikersnaam in:',
'editusergroup'               => 'Wysig gebruikersgroepe',
'editinguser'                 => "Besig om gebruikersregte van gebruiker '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) te wysig",
'userrights-editusergroup'    => 'wysig gebruikersgroepe',
'saveusergroups'              => 'Stoor gebruikersgroepe',
'userrights-groupsmember'     => 'Lid van:',
'userrights-groups-help'      => "U kan die groepe waarvan die gebruiker 'n lid is verander. 
* 'n Geselekteerde boks beteken dat die gebruiker lid is van die groep.
* 'n Ongeselekteerde boks beteken dat die gebruiker nie 'n lid van die groep is nie.
* 'n Ster (*) beteken dat u nie die gebruiker uit 'n groep kan verwyder as hy eers daaraan behoort nie, of vice versa.",
'userrights-reason'           => 'Rede vir wysiging:',
'userrights-no-interwiki'     => 'U het nie toestemming om gebruikersregte op ander wikis te verander nie.',
'userrights-nodatabase'       => 'Databasis $1 bestaan nie of is nie hier beskikbaar nie.',
'userrights-notallowed'       => 'U het nie die toestemming om gebruikersregte toe te ken nie.',
'userrights-changeable-col'   => 'Groepe wat u kan verander',
'userrights-unchangeable-col' => 'Groepe wat u nie kan verander nie',

# Groups
'group'               => 'Groep:',
'group-user'          => 'Gebruikers',
'group-autoconfirmed' => 'Bevestigde gebruikers',
'group-bot'           => 'Robotte',
'group-sysop'         => 'Administrateurs',
'group-bureaucrat'    => 'Burokrate',
'group-suppress'      => 'Toesighouers',
'group-all'           => '(alle)',

'group-user-member'          => 'Gebruiker',
'group-autoconfirmed-member' => 'Geregistreerde gebruiker',
'group-bot-member'           => 'Robot',
'group-sysop-member'         => 'Administrateur',
'group-bureaucrat-member'    => 'Burokraat',
'group-suppress-member'      => 'Toesighouer',

'grouppage-user'          => '{{ns:project}}:Gebruikers',
'grouppage-autoconfirmed' => '{{ns:project}}:Geregistreerde gebruikers',
'grouppage-bot'           => '{{ns:project}}:Robotte',
'grouppage-sysop'         => '{{ns:project}}:Administrateurs',
'grouppage-bureaucrat'    => '{{ns:project}}:Burokrate',

# Rights
'right-read'                 => 'Lees bladsye',
'right-edit'                 => 'Wysig bladsye',
'right-createpage'           => 'Skep bladsye (nie besprekingsblaaie nie)',
'right-createtalk'           => 'Skep besprekingsbladsye',
'right-createaccount'        => 'Skep nuwe gebruikersrekeninge',
'right-minoredit'            => "Merk as 'n klein verandering",
'right-move'                 => 'Skuif bladsye',
'right-upload'               => 'Laai lêers op',
'right-reupload'             => "Oorskryf 'n bestaande lêer",
'right-reupload-own'         => "Oorskryf 'n lêer wat u self opgelaai het",
'right-delete'               => 'Vee bladsye uit',
'right-bigdelete'            => 'Skrap bladsye met groot geskiedenisse',
'right-deleterevision'       => 'Skrap en ontskrap spesifieke hersienings van bladsye',
'right-browsearchive'        => 'Soek uigeveede bladsye',
'right-undelete'             => "Ontskrap 'n bladsy",
'right-suppressionlog'       => 'Besigtig privaat logboeke',
'right-editinterface'        => 'Wysig die gebruikerskoppelvlak',
'right-import'               => "Importeer bladsye vanaf ander wiki's",
'right-importupload'         => "Importeer bladsye vanaf 'n lêer",
'right-patrol'               => 'Merk ander se wysigings as gepatrolleer',
'right-mergehistory'         => 'Versmelt die geskiedenis van bladsye',
'right-userrights'           => 'Wysig alle gebruiker regte',
'right-userrights-interwiki' => 'Wysig gebruikersregte van gebruikers op ander wikis',
'right-siteadmin'            => 'Sluit en ontsluit die datbasis',

# User rights log
'rightslog'      => 'Gebruikersregtelogboek',
'rightslogtext'  => 'Hieronder is die logboek van gebruikersregte wat verander is.',
'rightslogentry' => 'groep lidmaatskap verander vir $1 van $2 na $3',
'rightsnone'     => '(geen)',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|wysiging|wysigings}}',
'recentchanges'                  => 'Onlangse wysigings',
'recentchangestext'              => 'Volg die mees onlangse wysigings aan die wiki op die bladsy.',
'recentchanges-feed-description' => 'Spoor die mees onlangse wysigings op die wiki na in die voer.',
'rcnote'                         => "Hier volg die laaste {{PLURAL:$1|'''$1''' wysiging|'''$1''' wysigings}} vir die afgelope {{PLURAL:$2|dag|'''$2''' dae}}, soos vanaf $4, $5.",
'rcnotefrom'                     => 'Hier onder is die wysigings sedert <b>$2</b> (tot by <b>$1</b> word gewys).',
'rclistfrom'                     => 'Vertoon wysigings vanaf $1',
'rcshowhideminor'                => '$1 klein wysigings',
'rcshowhidebots'                 => '$1 robotte',
'rcshowhideliu'                  => '$1 aangetekende gebruikers',
'rcshowhideanons'                => '$1 anonieme gebruikers',
'rcshowhidepatr'                 => '$1 gepatrolleerde wysigings',
'rcshowhidemine'                 => '$1 my wysigings',
'rclinks'                        => 'Vertoon die laaste $1 wysigings in die afgelope $2 dae<br />$3',
'diff'                           => 'verskil',
'hist'                           => 'geskiedenis',
'hide'                           => 'versteek',
'show'                           => 'Wys',
'minoreditletter'                => 'k',
'newpageletter'                  => 'N',
'boteditletter'                  => 'b',
'rc_categories'                  => 'Beperk tot kategorië (skei met "|")',
'rc_categories_any'              => 'Enige',
'newsectionsummary'              => '/* $1 */ nuwe afdeling',

# Recent changes linked
'recentchangeslinked'          => 'Verwante veranderings',
'recentchangeslinked-title'    => 'Wysigings verwant aan "$1"',
'recentchangeslinked-noresult' => 'Geen veranderinge op geskakelde bladsye gedurende die periode nie.',
'recentchangeslinked-summary'  => "Hier volg 'n lys van wysigings wat onlangs gemaak is aan bladsye wat van die gespesifiseerde bladsy geskakel word (of van bladsye van die gespesifiseerde kategorie).
Bladsye op [[Special:Watchlist|jou dophoulys]] word in '''vetdruk''' uitgewys.",
'recentchangeslinked-page'     => 'Bladsy naam:',
'recentchangeslinked-to'       => 'Besigtig wysigings aan bladsye met skakels na die bladsy',

# Upload
'upload'                     => 'Laai lêer',
'uploadbtn'                  => 'Laai lêer',
'reupload'                   => 'Herlaai',
'reuploaddesc'               => 'Keer terug na die laaivorm.',
'uploadnologin'              => 'Nie ingeteken nie',
'uploadnologintext'          => 'Teken eers in [[Special:UserLogin|logged in]]
om lêers te laai.',
'upload_directory_missing'   => 'Die oplaaigids ($1) bestaan nie en kon nie deur die webbediener geskep word nie.',
'upload_directory_read_only' => 'Die webbediener kan nie na die oplaai gids ($1) skryf nie.',
'uploaderror'                => 'Laaifout',
'uploadtext'                 => "'''STOP!''' Voor jy hier laai, lees en volg {{SITENAME}} se
[[{{MediaWiki:Copyrightpage}}|beleid oor prentgebruik]].

Om prente wat voorheen gelaai is te sien of te soek, gaan na die
[[Special:ImageList|lys van gelaaide prente]].
Laai van lêers en skrappings word aangeteken in die
[[Special:Log/upload|laailog]].

Gebruik die vorm hier onder om nuwe prente te laai wat jy ter illustrasie in jou artikels wil gebruik.
In die meeste webblaaiers sal jy 'n \"Browse...\" knop sien, wat jou bedryfstelsel se standaard lêeroopmaak dialoogblokkie sal oopmaak.
Deur 'n lêer in hierdie dialoogkassie te kies, vul jy die teksboks naas die knop met die naam van die lêer.
Jy moet ook die blokkie merk om te bevestig dat jy geen kopieregte skend deur die lêer op te laai nie.
Kliek die \"Laai\" knop om die laai af te handel.
Dit mag dalk 'n rukkie neem as jy 'n stadige internetverbinding het.

Die voorkeurformate is JPEG vir fotografiese prente, PNG vir tekeninge en ander ikoniese prente, en OGG vir klanklêers.
Gebruik asseblief beskrywende lêername om verwarring te voorkom.
Om die prent in 'n artikel te gebruik, gebruik 'n skakel met die formaat '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.jpg]]</nowiki>''' of
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:file.png|alt text]]</nowiki>''' of
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:file.ogg]]</nowiki>''' vir klanklêers.

Let asseblief op dat, soos met {{SITENAME}} bladsye, mag ander jou gelaaide lêers redigeer as hulle dink dit dien die ensiklopedie, en jy kan verhoed word om lêers te laai as jy die stelsel misbruik.",
'upload-permitted'           => 'Toegelate lêertipes: $1.',
'upload-preferred'           => 'Aanbevole lêertipes: $1.',
'upload-prohibited'          => 'Verbode lêertipes: $1.',
'uploadlog'                  => 'laailog',
'uploadlogpage'              => 'laai_log',
'uploadlogpagetext'          => "Hier volg 'n lys van die mees onlangse lêers wat gelaai is.",
'filename'                   => 'Lêernaam',
'filedesc'                   => 'Opsomming',
'fileuploadsummary'          => 'Opsomming:',
'filestatus'                 => 'Outeursregsituasie:',
'filesource'                 => 'Bron:',
'uploadedfiles'              => 'Gelaaide lêers',
'ignorewarning'              => 'Ignoreer waarskuwings en stoor die lêer',
'ignorewarnings'             => 'Ignoreer enige waarskuwings',
'minlength1'                 => 'Prentname moet ten minste een letter lank wees.',
'illegalfilename'            => 'Die lêernaam "$1" bevat karakters wat nie toegelaat word in bladsytitels nie. Verander asseblief die naam en probeer die lêer weer laai.',
'badfilename'                => 'Prentnaam is verander na "$1".',
'filetype-badmime'           => 'Lêers met MIME-tipe "$1" word nie toegelaat nie.',
'filetype-unwanted-type'     => "'''\".\$1\"''' is 'n ongewenste lêertipe. 
Aanbevole {{PLURAL:\$3|lêertipe|lêertipes}} is \$2.",
'filetype-banned-type'       => "'''\".\$1\"''' is nie 'n toegelate lêertipe nie.
Toelaatbare {{PLURAL:\$3|lêertipes|lêertipes}} is \$2.",
'filetype-missing'           => 'Die lêer het geen uitbreiding (soos ".jpg").',
'large-file'                 => 'Aanbeveling: maak lêer kleiner as $1;
die lêer is $2.',
'largefileserver'            => 'Hierdie lêer is groter as wat die bediener se opstelling toelaat.',
'emptyfile'                  => "Die lêer wat jy probeer oplaai het blyk leeg te wees. Dit mag wees omdat jy 'n tikfout in die lêernaam gemaak het. Gaan asseblief na en probeer weer.",
'fileexists'                 => "'n Lêer met die naam bestaan reeds, kyk na <strong><tt>$1</tt></strong> as u nie seker is dat u dit wil wysig nie.",
'fileexists-thumb'           => "<center>'''Bestaande lêer'''</center>",
'file-exists-duplicate'      => "Die lêer is 'n duplikaat van die volgende {{PLURAL:$1|lêer|lêers}}:",
'successfulupload'           => 'Laai suksesvol',
'uploadwarning'              => 'Laaiwaarskuwing',
'savefile'                   => 'Stoor lêer',
'uploadedimage'              => 'het "[[$1]]" gelaai',
'overwroteimage'             => 'het een nuwe weergawe van "[[$1]]" gelaai',
'uploaddisabled'             => 'Laai is uitgeskakel',
'uploaddisabledtext'         => 'Die oplaai van lêers is afgeskakel op {{SITENAME}}.',
'uploadcorrupt'              => "Die lêer is foutief of is van 'n verkeerde tipe. Gaan asseblief die lêer na en laai weer op.",
'uploadvirus'                => "Hierdie lêer bevat 'n virus! Inligting: $1",
'sourcefilename'             => 'Bronlêernaam:',
'destfilename'               => 'Teikenlêernaam:',
'upload-maxfilesize'         => 'Maksimum lêer grootte: $1',
'watchthisupload'            => 'Hou hierdie bladsy dop',

'upload-proto-error' => 'Verkeerde protokol',
'upload-file-error'  => 'Interne fout',
'upload-misc-error'  => 'Onbekende laai fout',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'  => 'Kon nie die URL bereik nie',
'upload-curl-error28' => 'Oplaai neem te lank',

'license'            => 'Lisensiëring:',
'nolicense'          => 'Niks gekies',
'license-nopreview'  => '(Voorskou nie beskikbaar)',
'upload_source_url'  => " ('n geldige, publiek toeganklike URL)",
'upload_source_file' => " ('n lêer op U rekenaar)",

# Special:ImageList
'imagelist-summary'     => 'Die spesiale bladsy wys al die opgelaaide lêers.
Die nuutste lêer word eerste vertoon.
Klik op die opskrifte om die tabel anders te sorteer.',
'imagelist_search_for'  => 'Soek vir medianaam:',
'imgfile'               => 'lêer',
'imagelist'             => 'Prentelys',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Naam',
'imagelist_user'        => 'Gebruiker',
'imagelist_size'        => 'Grootte',
'imagelist_description' => 'Beskryving',

# Image description page
'filehist'                       => 'Lêergeskiedenis',
'filehist-help'                  => 'Klik op die datum/tyd om te sien hoe die lêer destyds gelyk het.',
'filehist-deleteall'             => 'verwyder alles',
'filehist-deleteone'             => 'skrap',
'filehist-revert'                => 'rol terug',
'filehist-current'               => 'huidig',
'filehist-datetime'              => 'Datum/Tyd',
'filehist-user'                  => 'Gebruiker',
'filehist-dimensions'            => 'Dimensies',
'filehist-filesize'              => 'Lêergrootte',
'filehist-comment'               => 'Opmerking',
'imagelinks'                     => 'Prentskakels',
'linkstoimage'                   => 'Die volgende {{PLURAL:$1|bladsy|$1 bladsye}} gebruik hierdie prent:',
'nolinkstoimage'                 => 'Daar is geen bladsye wat hierdie prent gebruik nie.',
'morelinkstoimage'               => 'Wys [[Special:WhatLinksHere/$1|meer skakels]] na die lêer.',
'redirectstofile'                => "Die volgende {{PLURAL:$1|lêer is 'n aanstuur|$1 lêers is aansture}} na die lêer:",
'duplicatesoffile'               => "Die volgende {{PLURAL:$1|lêer is 'n duplikaat|$1 lêers is duplikate}} van die lêer:",
'sharedupload'                   => 'Die lêer word gedeel en mag moontlik op ander projekte gebruik word.',
'shareduploadwiki'               => 'Sien $1 vir verdere inligting.',
'shareduploadwiki-desc'          => 'Die $1 uit die gedeelde lêerbank word hieronder weergegee.',
'shareduploadwiki-linktext'      => 'lêer beskrywingsbladsy',
'shareduploadduplicate'          => "Die lêer is 'n duplikaat van $1 uit die gedeelde mediabank.",
'shareduploadduplicate-linktext' => "'n ander lêer",
'shareduploadconflict'           => 'Die lêer het dieselfde naam as $1 in die gedeelde mediabank.',
'shareduploadconflict-linktext'  => "'n ander lêer",
'noimage'                        => "Daar bestaan nie 'n lêer met so 'n naam nie, maar u kan $1.",
'noimage-linktext'               => 'een oplaai',
'uploadnewversion-linktext'      => 'Laai een nuwe weergawe van hierdie lêer',
'imagepage-searchdupe'           => 'Soek vir duplikaat lêers',

# File reversion
'filerevert'         => 'Maak $1 ongedaan',
'filerevert-legend'  => 'Maak lêer ongedaan',
'filerevert-comment' => 'Opmerking:',
'filerevert-submit'  => 'Rol terug',

# File deletion
'filedelete'                  => 'Skrap $1',
'filedelete-legend'           => 'Skrap lêer',
'filedelete-intro'            => "U is besig om '''[[Media:$1|$1]]''' te verwyder.",
'filedelete-comment'          => 'Rede vir skrapping:',
'filedelete-submit'           => 'Skrap',
'filedelete-success'          => "'''$1''' is geskrap.",
'filedelete-success-old'      => '<span class="plainlinks">Die weergawe van \'\'\'[[Media:$1|$1]]\'\'\' op $3, $2 is geskrap.</span>',
'filedelete-nofile'           => "'''$1''' bestaan nie op {{SITENAME}} nie.",
'filedelete-otherreason'      => 'Ander/ekstra rede:',
'filedelete-reason-otherlist' => 'Andere rede',
'filedelete-reason-dropdown'  => '*Algemene skrappingsredes:
** Kopieregskending
** Duplikaatlêer',
'filedelete-edit-reasonlist'  => 'Wysig skrap redes',

# MIME search
'mimesearch'         => 'MIME-soek',
'mimesearch-summary' => 'Hierdie bladsy maak dit moontlik om lêers te filtreer volgens hulle MIME-tipe. Invoer: inhoudtipe/subtipe, byvoorbeeld <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-tipe:',
'download'           => 'laai af',

# Unwatched pages
'unwatchedpages' => 'Bladsye wat nie dopgehou word nie',

# List redirects
'listredirects' => 'Lys aansture',

# Unused templates
'unusedtemplates'     => 'Ongebruikte sjablone',
'unusedtemplatestext' => "Hierdie blad lys alle bladsye in die sjabloonnaamruimte wat nêrens in 'n ander blad ingesluit word nie. Onthou om ook ander skakels na die sjablone na te gaan voor verwydering.",
'unusedtemplateswlh'  => 'ander skakels',

# Random page
'randompage'         => 'Lukrake bladsy',
'randompage-nopages' => 'Daar is geen bladye in die naamspasie.',

# Random redirect
'randomredirect'         => 'Lukrake aanstuur',
'randomredirect-nopages' => 'Daar is geen aansture in die naamspasie.',

# Statistics
'statistics'             => 'Statistiek',
'sitestats'              => 'Werfstatistiek',
'userstats'              => 'Gebruikerstatistiek',
'sitestatstext'          => "Daar is {{PLURAL:\$1|'''1''' bladsy|'n totaal van '''\$1''' bladsye}} in die databasis.
Dit sluit \"bespreek\"-bladsye in, bladsye oor {{SITENAME}}, minimale \"verkorte\"
bladsye, wegwysbladsye, en ander wat waarskynlik nie as artikels kwalifiseer nie.
Uitsluitend bogenoemde, is daar {{PLURAL:\$2|'''1''' bladsy|'''\$2''' bladsye}} wat waarskynlik {{PLURAL:\$2|bladsy|bladsye}} met ware inhoud is.

'''\$8''' {{PLURAL:\$8|lêer|lêers}} is gelaai.

{{PLURAL:\$3|Bladsy is al '''1''' keer aangevra|Bladsye is al '''\$3''' kere aangevra}}, en '''\$4''' keer verander sedert hierdie wiki opgezet is.
Dit werk uit op gemiddeld '''\$5''' veranderings per bladsy, en bladsye word '''\$6''' keer per verandering aangevra.

Die ''[http://www.mediawiki.org/wiki/Manual:Job_queue job queue]''-lengte is '''\$7'''.",
'userstatstext'          => "Daar is {{PLURAL:$1|'''1''' geregistreerde [[Special:ListUsers|gebruiker]]|'''$1''' geregistreerde [[Special:ListUsers|gebruikers]]}}, waarvan '''$2''' (of '''$4%''') $5 regte het.",
'statistics-mostpopular' => 'Mees bekykte bladsye',

'disambiguations'      => 'Bladsye wat onduidelikhede opklaar',
'disambiguationspage'  => 'Template:Dubbelsinnig',
'disambiguations-text' => "Die volgende bladsye skakel na '''dubbelsinnigheidsbladsye'''.
Die bladsye moet gewysig word om eerder direk na die regte onderwerpe te skakel.<br />
'n Bladsy word beskou as 'n dubbelsinnigheidsbladsy as dit 'n sjabloon bevat wat geskakel is vanaf [[MediaWiki:Disambiguationspage]]",

'doubleredirects'       => 'Dubbele aansture',
'doubleredirectstext'   => '<b>Let op:</b> Hierdie lys bevat moontlik vals positiewes. Dit beteken gewoonlik dat daar nog teks met skakels onder die eerste #REDIRECT/#AANSTUUR is.<br />
Elke ry bevat skakels na die eerste en die tweede aanstuur, asook die eerste reël van van die tweede aanstuur se teks, wat gewoonlik die "regte" teiken bladsy gee waarna die eerste aanstuur behoort te wys.',
'double-redirect-fixer' => 'Aanstuur hersteller',

'brokenredirects'        => 'Stukkende aansture',
'brokenredirectstext'    => "Die volgende aansture skakel na 'n bladsy wat nie bestaan nie.",
'brokenredirects-edit'   => '(wysig)',
'brokenredirects-delete' => '(skrap)',

'withoutinterwiki'         => 'Bladsye sonder taalskakels',
'withoutinterwiki-summary' => 'Die volgende bladsye het nie skakels na weergawes in ander tale nie:',
'withoutinterwiki-legend'  => 'Voorvoegsel',
'withoutinterwiki-submit'  => 'Wys',

'fewestrevisions' => 'Artikels met die minste wysigings',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|greep|grepe}}',
'ncategories'             => '$1 {{PLURAL:$1|kategorie|kategorieë}}',
'nlinks'                  => '$1 {{PLURAL:$1|skakel|skakels}}',
'nmembers'                => '$1 {{PLURAL:$1|lid|lede}}',
'nrevisions'              => '$1 {{PLURAL:$1|weergawe|weergawes}}',
'nviews'                  => '$1 {{PLURAL:$1|keer|kere}} aangevra',
'specialpage-empty'       => 'Die verslag lewer geen resultate nie.',
'lonelypages'             => 'Weesbladsye',
'lonelypagestext'         => 'Die volgende bladsye is nie geskakel vanaf ander bladsye in {{SITENAME}} nie:',
'uncategorizedpages'      => 'Ongekategoriseerde bladsye',
'uncategorizedcategories' => 'Ongekategoriseerde kategorieë',
'uncategorizedimages'     => 'Ongekategoriseerde lêers',
'uncategorizedtemplates'  => 'Ongekategoriseerde sjablone',
'unusedcategories'        => 'Ongebruikte kategorieë',
'unusedimages'            => 'Ongebruikte lêers',
'popularpages'            => 'Gewilde bladsye',
'wantedcategories'        => 'Gesoekte kategorieë',
'wantedpages'             => 'Gesogte bladsye',
'missingfiles'            => 'Lêers wat nie bestaan nie',
'mostlinked'              => 'Bladsye met meeste skakels daarheen',
'mostlinkedcategories'    => 'Kategorieë met die meeste skakels daarheen',
'mostlinkedtemplates'     => 'Sjablone met die meeste skakels daarheen',
'mostcategories'          => 'Artikels met die meeste kategorieë',
'mostimages'              => 'Beelde met meeste skakels daarheen',
'mostrevisions'           => 'Artikels met meeste wysigings',
'prefixindex'             => 'Alle bladsye (voorvoegselindeks)',
'shortpages'              => 'Kort bladsye',
'longpages'               => 'Lang bladsye',
'deadendpages'            => 'Doodloopbladsye',
'deadendpagestext'        => 'Die volgende bladsye bevat nie skakels na ander bladsye in {{SITENAME}} nie:',
'protectedpages'          => 'Beskermde bladsye',
'protectedpages-indef'    => 'Slegs blokkades sonder vervaldatum',
'protectedpagestext'      => 'Die volgende bladsye is beskerm teen verskuiwing of wysiging:',
'protectedpagesempty'     => 'Geen bladsye is tans met die parameters beveilig nie.',
'protectedtitles'         => 'Beskermde titels',
'protectedtitlestext'     => 'Die volgende titels is beveilig en kan nie geskep word nie',
'protectedtitlesempty'    => 'Geen titels is tans met die parameters beveilig nie.',
'listusers'               => 'Gebruikerslys',
'newpages'                => 'Nuwe bladsye',
'newpages-username'       => 'Gebruikersnaam:',
'ancientpages'            => 'Oudste bladsye',
'move'                    => 'Skuif',
'movethispage'            => 'Skuif hierdie bladsy',
'unusedimagestext'        => "Let asseblief op dat ander webwerwe, soos die internasionale {{SITENAME}}s, dalk met 'n direkte URL na 'n prent skakel, so die prent sal dus hier verskyn al word dit aktief gebruik.",
'unusedcategoriestext'    => 'Die volgende kategoriebladsye bestaan alhoewel geen artikel of kategorie hulle gebruik nie.',
'notargettitle'           => 'Geen teiken',
'notargettext'            => "Jy het nie 'n teikenbladsy of gebruiker waarmee hierdie funksie moet werk, gespesifiseer nie.",
'pager-newer-n'           => '{{PLURAL:$1|nuwer 1|nuwer $1}}',
'pager-older-n'           => '{{PLURAL:$1|ouer 1|ouer $1}}',

# Book sources
'booksources'               => 'Boekbronne',
'booksources-search-legend' => 'Soek vir boekbronne',
'booksources-go'            => 'Soek',

# Special:Log
'specialloguserlabel'  => 'Gebruiker:',
'speciallogtitlelabel' => 'Titel:',
'log'                  => 'Logboeke',
'all-logs-page'        => 'Alle logboeke',
'log-search-legend'    => 'Soek vir logboeke',
'log-search-submit'    => 'Gaan',
'alllogstext'          => "Vertoon 'n samestelling van laai-, skrap-, beskerm-, versper- en administrateurboekstawings van {{SITENAME}}.
U kan die resultate vernou deur 'n boekstaaftipe, gebruikersnaam of spesifieke blad te kies.",
'logempty'             => 'Geen inskrywings in die logboek voldoen aan die kriteria.',
'log-title-wildcard'   => 'Soek bladsye wat met die naam begin',

# Special:AllPages
'allpages'          => 'Alle bladsye',
'alphaindexline'    => '$1 tot $2',
'nextpage'          => 'Volgende blad ($1)',
'prevpage'          => 'Vorige bladsye ($1)',
'allpagesfrom'      => 'Wys bladsye vanaf:',
'allarticles'       => 'Alle artikels',
'allinnamespace'    => 'Alle bladsye (naamruimte $1)',
'allnotinnamespace' => 'Alle bladsye (nie in naamruimte $1 nie)',
'allpagesprev'      => 'Vorige',
'allpagesnext'      => 'Volgende',
'allpagessubmit'    => 'Gaan',
'allpagesprefix'    => 'Wys bladsye wat begin met:',
'allpages-bad-ns'   => '{{SITENAME}} het geen naamspasie "$1" nie.',

# Special:Categories
'categories'                    => 'Kategorieë',
'categoriespagetext'            => 'Die volgende kategorieë bestaan op die wiki.',
'categoriesfrom'                => 'Wys kategorieë vanaf:',
'special-categories-sort-count' => 'sorteer volgens getal',
'special-categories-sort-abc'   => 'sorteer alfabeties',

# Special:ListUsers
'listusersfrom'      => 'Wys gebruikers, beginnende by:',
'listusers-submit'   => 'Wys',
'listusers-noresult' => 'Geen gebruiker gevind.',

# Special:ListGroupRights
'listgrouprights'          => 'Gebruikersgroepregte',
'listgrouprights-summary'  => "Hier volg 'n lys van gebruikersgroepe wat op die wiki gedefinieer is met hulle geassosieerde regte. Vir meer inligting oor individuele regte, sien [[{{MediaWiki:Listgrouprights-helppage}}]].",
'listgrouprights-group'    => 'Groep',
'listgrouprights-rights'   => 'Regte',
'listgrouprights-helppage' => 'Help:Groep regte',
'listgrouprights-members'  => '(lys van lede)',

# E-mail user
'mailnologin'     => 'Geen versendadres beskikbaar',
'mailnologintext' => "U moet [[Special:UserLogin|ingeteken]] wees en 'n geldige e-posadres in die [[Special:Preferences|voorkeure]] hê om e-pos aan ander gebruikers te stuur.",
'emailuser'       => 'Stuur e-pos na hierdie gebruiker',
'emailpage'       => 'Stuur e-pos na gebruiker',
'emailpagetext'   => 'As dié gerbuiker \'n geldige e-posadres in sy/haar gebruikersvoorkeure het, sal hierdie vorm \'n enkele boodskap stuur. Die e-posadres in jou gebruikersvoorkeure sal verkyn as die "Van"-adres van die pos. Dus sal die ontvanger kan terug antwoord.',
'usermailererror' => 'Fout met versending van e-pos:',
'defemailsubject' => '{{SITENAME}}-epos',
'noemailtitle'    => 'Geen e-posadres',
'noemailtext'     => "Hierdie gebruiker het nie 'n geldige e-posadres gespesifiseer nie of het gekies om nie e-pos van ander gebruikers te ontvang nie.",
'emailfrom'       => 'Van:',
'emailto'         => 'Aan:',
'emailsubject'    => 'Onderwerp:',
'emailmessage'    => 'Boodskap:',
'emailsend'       => 'Stuur',
'emailccme'       => "E-pos vir my 'n kopie van my boodskap.",
'emailccsubject'  => 'Kopie van U boodskap aan $1: $2',
'emailsent'       => 'E-pos gestuur',
'emailsenttext'   => 'Jou e-pos is gestuur.',

# Watchlist
'watchlist'            => 'My dophoulys',
'mywatchlist'          => 'My dophoulys',
'watchlistfor'         => "(vir '''$1''')",
'nowatchlist'          => 'Jy het geen items in jou dophoulys nie.',
'watchnologin'         => 'Nie ingeteken nie',
'watchnologintext'     => 'Jy moet [[Special:UserLogin|ingeteken]]
wees om jou dophoulys te verander.',
'addedwatch'           => 'Bygevoeg tot dophoulys',
'addedwatchtext'       => 'Die bladsy "$1" is by u [[Special:Watchlist|dophoulys]] gevoeg.
Die bladsy "$1" is by u [[Special:Watchlist|dophoulys]] gevoeg. Toekomstige veranderinge aan hierdie bladsy en sy verwante besprekingsblad sal daar verskyn en die bladsy sal in \'\'\'vetdruk\'\'\' verskyn in die [[Special:RecentChanges|lys van onlangse wysigings]], sodat u dit makliker kan raaksien.

As u die bladsy later van u dophoulys wil verwyder, kliek "verwyder van dophoulys" in die kieslys bo-aan die bladsy.',
'removedwatch'         => 'Afgehaal van dophoulys',
'removedwatchtext'     => 'Die bladsy "[[:$1]]" is van u dophoulys afgehaal.',
'watch'                => 'Hou dop',
'watchthispage'        => 'Hou hierdie bladsy dop',
'unwatch'              => 'Verwyder van dophoulys',
'unwatchthispage'      => 'Moenie meer dophou',
'notanarticle'         => "Nie 'n artikel",
'notvisiblerev'        => 'Weergawe is verwyder',
'watchnochange'        => 'Geen item op die dophoulys is geredigeer in die gekose periode nie.',
'watchlist-details'    => '{{PLURAL:$1|$1 bladsy|$1 bladsye}} in jou dophoulys, besprekingsbladsye uitgesluit.',
'wlheader-enotif'      => '* E-pos notifikasie is aangeskakel.',
'wlheader-showupdated' => "* Bladsye wat verander is sedert u hulle laas besoek het word in '''vetdruk''' uitgewys",
'watchmethod-recent'   => 'Kontroleer onlangse wysigings aan bladsye op dophoulys',
'watchlistcontains'    => 'Jou dophoulys bevat $1 {{PLURAL:$1|bladsy|bladsye}}.',
'iteminvalidname'      => "Probleem met item '$1', ongeldige naam...",
'wlnote'               => "Hier volg die laaste {{PLURAL:$1|verandering|'''$1''' veranderings}} binne die laaste {{PLURAL:$2|uur|'''$2''' ure}}.",
'wlshowlast'           => 'Wys afgelope $1 ure, $2 dae of $3',
'watchlist-show-bots'  => "Wys 'bot' wysigings",
'watchlist-hide-bots'  => 'Versteek robotte',
'watchlist-show-own'   => 'Wys my wysigings',
'watchlist-hide-own'   => 'Versteek my wysigings',
'watchlist-show-minor' => 'Wys klein wysigings',
'watchlist-hide-minor' => 'Versteek klein wysigings',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Plaas op dophoulys...',
'unwatching' => 'Verwyder van dophoulys...',

'enotif_mailer'                => '{{SITENAME}} E-pos kennisgewings',
'enotif_reset'                 => 'Merk alle bladsye as besoek',
'enotif_newpagetext'           => "Dis 'n nuwe bladsy.",
'enotif_impersonal_salutation' => '{{SITENAME}} gebruiker',
'changed'                      => 'verander',
'created'                      => 'geskep',
'enotif_subject'               => 'Bladsy $PAGETITLE op {{SITENAME}} is $CHANGEDORCREATED deur $PAGEEDITOR',
'enotif_lastvisited'           => 'Sien $1 vir alle wysigings sedert U laaste besoek.',
'enotif_lastdiff'              => 'Sien $1 om hierdie wysiging te bekyk.',
'enotif_anon_editor'           => 'anonieme gebruiker $1',
'enotif_body'                  => 'Beste $WATCHINGUSERNAME,

Die bladsy $PAGETITLE op {{SITENAME}} is $CHANGEDORCREATED op $PAGEEDITDATE deur $PAGEEDITOR, sien $PAGETITLE_URL vir die nuutste weergawe.

$NEWPAGE

Samevatting van die wysiging: $PAGESUMMARY $PAGEMINOREDIT

Outeur se kontakdetails:
E-pos: $PAGEEDITOR_EMAIL
Wiki: $PAGEEDITOR_WIKI

Tensy u hierdie bladsy besoek, sal u geen verdere kennisgewings ontvang nie. U kan ook die waarskuwingsvlag op u dophoulys verstel.

             Groete van die {{SITENAME}} waarskuwingssisteem.

--
U kan u dophoulys wysig by:
{{fullurl:Special:Watchlist/edit}}

Terugvoer en verdere bystand:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Skrap bladsy',
'confirm'                     => 'Bevestig',
'excontent'                   => "inhoud was: '$1'",
'excontentauthor'             => "Inhoud was: '$1' (en '[[Special:Contributions/$2|$2]]' was die enigste bydraer)",
'exbeforeblank'               => "Inhoud voor uitwissing was: '$1'",
'exblank'                     => 'bladsy was leeg',
'delete-confirm'              => 'Skrap "$1"',
'delete-legend'               => 'Skrap',
'historywarning'              => "Waarskuwing: Die bladsy het 'n geskiedenis:",
'confirmdeletetext'           => "Jy staan op die punt om 'n bladsy of prent asook al hulle geskiedenis uit die databasis te skrap.
Bevestig asseblief dat jy dit wil doen, dat jy die gevolge verstaan en dat jy dit doen in ooreenstemming met die [[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Aksie uitgevoer',
'deletedtext'                 => '"<nowiki>$1</nowiki>" is geskrap.
Kyk na $2 vir \'n rekord van onlangse skrappings.',
'deletedarticle'              => '"$1" geskrap',
'dellogpage'                  => 'Skraplogboek',
'dellogpagetext'              => "Hier onder is 'n lys van die mees onlangse skrappings. Alle tye is bedienertyd (UGT).",
'deletionlog'                 => 'skrappingslogboek',
'reverted'                    => 'Het terug gegaan na vroeëre weergawe',
'deletecomment'               => 'Rede vir skrapping',
'deleteotherreason'           => 'Ander/ekstra rede:',
'deletereasonotherlist'       => 'Andere rede',
'deletereason-dropdown'       => '*Algemene redes vir verwydering
** Op aanvraag van outeur
** Skending van kopieregte
** Vandalisme',
'delete-edit-reasonlist'      => 'Wysig skrap redes',
'delete-toobig'               => "Die bladsy het 'n lang wysigingsgeskiedenis, meer as $1 {{PLURAL:$1|weergawe|weergawes}}.
Verwydering van die soort blaaie is beperk om ontwrigting van {{SITENAME}} te voorkom.",
'rollback'                    => 'Rol veranderinge terug',
'rollback_short'              => 'Rol terug',
'rollbacklink'                => 'Rol terug',
'rollbackfailed'              => 'Terugrol onsuksesvol',
'cantrollback'                => 'Kan nie na verandering terug keer nie; die laaste bydraer is die enigste outer van hierdie bladsy.',
'editcomment'                 => 'Die wysigopsomming was: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Wysigings deur [[Special:Contributions/$2|$2]] teruggerol na laaste weergawe deur $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Wysigings deur $1 teruggerol; terugverander na laaste weergawe deur $2.',
'protectlogpage'              => 'Beskermlogboek',
'protectedarticle'            => 'het [[$1]] beskerm',
'unprotectedarticle'          => 'het beskerming van [[$1]] verwyder',
'protect-title'               => 'Beskerm "$1"',
'protect-legend'              => 'Bevestig beskerming',
'protectcomment'              => 'Rede vir beskerming:',
'protectexpiry'               => 'Verval:',
'protect_expiry_invalid'      => 'Vervaltyd is ongeldig.',
'protect_expiry_old'          => 'Vervaltyd is in die verlede.',
'protect-unchain'             => 'Gee regte om te skuif',
'protect-text'                => 'U kan die veiligheidsvlak vir blad <strong><nowiki>$1</nowiki></strong> hier bekyk of verander.',
'protect-locked-access'       => "Jou rekening het nie regte om 'n bladsy se veiligheidsvlakke te verander nie.
Hier is die huidige verstellings vir bladsy <strong>$1</strong>:",
'protect-cascadeon'           => 'Die bladsy word beskerm want dit is ingesluit by die volgende {{PLURAL:$1|blad|blaaie}} wat kaskade-beskerming geniet. U kan die veiligheidsvlak van die bladsy verander, maar dit sal nie die ander kaskade blaaie beïnvloed nie.',
'protect-default'             => '(normaal)',
'protect-fallback'            => 'Hiervoor is "$1" regte nodig',
'protect-level-autoconfirmed' => 'Beskerm teen anonieme wysigings',
'protect-level-sysop'         => 'Slegs administrateurs',
'protect-summary-cascade'     => 'kaskade',
'protect-expiring'            => 'verval $1 (UTC)',
'protect-cascade'             => 'Beveilig bladsye insluitend die bladsy (kaskade effek)',
'protect-cantedit'            => 'U kan nie die veiligheidsvlak van die blad verander nie, want u het nie regte om dit te wysig nie.',
'restriction-type'            => 'Regte:',
'restriction-level'           => 'Beperkingsvlak:',
'minimum-size'                => 'Minimum grootte',
'maximum-size'                => 'Maksimum grootte:',
'pagesize'                    => '(grepe)',

# Restrictions (nouns)
'restriction-edit'   => 'Wysig',
'restriction-move'   => 'Skuif',
'restriction-create' => 'Skep',
'restriction-upload' => 'Oplaai',

# Restriction levels
'restriction-level-sysop'         => 'volledig beveilig',
'restriction-level-autoconfirmed' => 'semibeveilig',
'restriction-level-all'           => 'enige vlak',

# Undelete
'undelete'                 => 'Herstel geskrapte bladsy',
'undeletepage'             => 'Bekyk en herstel geskrapte bladsye',
'undeletepagetitle'        => "'''Hieronder is die verwyderde bydraes van [[:$1]]'''.",
'viewdeletedpage'          => 'Bekyk geskrapte bladsye',
'undeletepagetext'         => 'Die volgende bladsye is geskrap, maar hulle is nog in die argief en kan herstel word. Die argief kan periodiek skoongemaak word.',
'undeleterevisions'        => '$1 {{PLURAL:$1|weergawe|weergawes}} in argief',
'undeletehistory'          => "As jy die bladsy herstel, sal alle weergawes herstel word.
As 'n nuwe bladsy met dieselfde naam sedert die skrapping geskep is, sal die herstelde weergawes in die nuwe bladsy se voorgeskiedenis verskyn en die huidige weergawe van die lewendige bladsy sal nie outomaties vervang word nie.",
'undeletehistorynoadmin'   => 'Die bladsy is geskrap.
Die rede hiervoor word onder in die opsomming aangedui, saam met besonderhede van die gebruikers wat die bladsy gewysig het voordat dit verwyder is.
Die verwyderde inhoud is slegs vir administrateurs sigbaar.',
'undelete-revision'        => 'Verwyder weergawe van $1 (vanaf $2) deur $3:',
'undelete-nodiff'          => 'Geen vorige wysigings gevind.',
'undeletebtn'              => 'Herstel',
'undeletelink'             => 'herstel',
'undeletereset'            => 'Herstel',
'undeletecomment'          => 'Opmerking:',
'undeletedarticle'         => 'het "$1" herstel',
'undeletedrevisions'       => '{{PLURAL:$1|1 weergawe|$1 weergawes}} herstel',
'undeletedrevisions-files' => '{{PLURAL:$1|1 weergawe|$1 weergawes}} en {{PLURAL:$2|1 lêer|$2 lêers}} herstel',
'undeletedfiles'           => '{{PLURAL:$1|1 lêer|$1 lêers}} herstel',
'cannotundelete'           => 'Skrapping onsuksesvol; miskien het iemand anders dié bladsy al geskrap.',
'undelete-header'          => 'Sien die [[Special:Log/delete|skraplogboek]] vir onlangs verwyderde bladsye.',
'undelete-search-box'      => 'Soek verwyderde bladsye',
'undelete-search-prefix'   => 'Wys bladsye wat begin met:',
'undelete-search-submit'   => 'Soek',
'undelete-no-results'      => 'Geen bladsye gevind in die argief van geskrapte bladsye.',

# Namespace form on various pages
'namespace'      => 'Naamruimte:',
'invert'         => 'Omgekeerde seleksie',
'blanknamespace' => '(Hoof)',

# Contributions
'contributions' => 'Gebruikersbydraes',
'mycontris'     => 'My bydraes',
'contribsub2'   => 'Vir $1 ($2)',
'nocontribs'    => 'Geen veranderinge wat by hierdie kriteria pas, is gevind nie.',
'uctop'         => ' (boontoe)',
'month'         => 'Vanaf maand (en vroeër):',
'year'          => 'Vanaf jaar (en vroeër):',

'sp-contributions-newbies'     => 'Wys slegs bydraes deur nuwe rekenings',
'sp-contributions-newbies-sub' => 'Vir nuwe gebruikers',
'sp-contributions-blocklog'    => 'Blokkeerlogboek',
'sp-contributions-search'      => 'Soek na bydraes',
'sp-contributions-username'    => 'IP-adres of gebruikersnaam:',
'sp-contributions-submit'      => 'Vertoon',

# What links here
'whatlinkshere'            => 'Skakels hierheen',
'whatlinkshere-title'      => 'Bladsye wat verwys na "$1"',
'whatlinkshere-page'       => 'Bladsy:',
'linklistsub'              => '(Lys van skakels)',
'linkshere'                => "Die volgende bladsye skakel na '''[[:$1]]''':",
'nolinkshere'              => "Geen bladsye skakel na '''[[:$1]]'''.",
'nolinkshere-ns'           => "Geen bladsye skakel na '''[[:$1]]''' in die verkose naamruimte nie.",
'isredirect'               => 'Stuur bladsy aan',
'istemplate'               => 'insluiting',
'isimage'                  => 'lêerskakel',
'whatlinkshere-prev'       => '{{PLURAL:$1|vorige|vorige $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|volgende|volgende $1}}',
'whatlinkshere-links'      => '← skakels',
'whatlinkshere-hideredirs' => '$1 aansture',
'whatlinkshere-hidelinks'  => '$1 skakels',
'whatlinkshere-hideimages' => '$1 beeldskakels',
'whatlinkshere-filters'    => 'Filters',

# Block/unblock
'blockip'                  => 'Blok gebruiker',
'blockip-legend'           => 'Blok gebruiker of IP-adres',
'blockiptext'              => "Gebruik die vorm hier onder om skryftoegang van 'n sekere IP-adres te blok.
Dit moet net gedoen word om vandalisme te voorkom en in ooreenstemming met [[{{MediaWiki:Policy-url}}|{{SITENAME}}-beleid]].
Vul 'n spesifieke rede hier onder in (haal byvoorbeeld spesifieke bladsye wat gevandaliseer is, aan).",
'ipaddress'                => 'IP-adres',
'ipadressorusername'       => 'IP-adres of gebruikernaam:',
'ipbexpiry'                => 'Duur:',
'ipbreason'                => 'Rede:',
'ipbreasonotherlist'       => 'Ander rede',
'ipbreason-dropdown'       => '*Algemene redes vir versperring
** Invoeg van valse inligting
** Skrap van bladsyinhoud
** "Spam" van skakels na eksterne webwerwe
** Invoeg van gemors op bladsye
** Intimiderende gedrag (teistering)
** Misbruik van veelvuldige rekeninge
** Onaanvaarbare gebruikersnaam',
'ipbanononly'              => 'Blokkeer slegs anonieme gebruikers',
'ipbcreateaccount'         => 'Blokkeer registrasie van gebruikers',
'ipbemailban'              => 'Verbied gebruiker om e-pos te stuur',
'ipbsubmit'                => 'Versper hierdie adres',
'ipbother'                 => 'Ander tydperk:',
'ipboptions'               => '2 ure:2 hours,1 dag:1 day,3 dae:3 days,1 week:1 week,2 weke:2 weeks,1 maand:1 month,3 maande:3 months,6 maande:6 months,1 jaar:1 year,onbeperk:infinite', # display1:time1,display2:time2,...
'ipbotheroption'           => 'ander',
'ipbotherreason'           => 'Ander/ekstra rede:',
'ipbwatchuser'             => 'Hou die gebruiker se bladsy en besprekingsbladsy dop.',
'badipaddress'             => 'Die IP-adres is nie in die regte formaat nie.',
'blockipsuccesssub'        => 'Blokkering het geslaag',
'blockipsuccesstext'       => 'Die IP-adres "$1" is geblokkeer.
<br />Sien die [[Special:IPBlockList|IP-bloklys]] vir \'n oorsig van blokkerings.',
'ipb-edit-dropdown'        => 'Werk lys van redes by',
'ipb-unblock-addr'         => 'Deblokkeer $1',
'ipb-unblock'              => "Deblokkeer 'n gebruiker of IP-adres",
'ipb-blocklist-addr'       => 'Wys bestaande blokkades vir $1',
'ipb-blocklist'            => 'Wys bestaande blokkades',
'unblockip'                => 'Maak IP-adres oop',
'unblockiptext'            => "Gebruik die vorm hier onder om skryftoegang te herstel vir 'n voorheen geblokkeerde IP-adres.",
'ipusubmit'                => 'Maak hierdie adres oop',
'unblocked'                => 'Blokkade van [[User:$1|$1]] is opgehef',
'unblocked-id'             => 'Blokkade $1 is opgehef',
'ipblocklist'              => 'Geblokkeerde IP-adresse en gebruikers',
'ipblocklist-legend'       => "Soek 'n geblokkeerde gebruiker",
'ipblocklist-username'     => 'Gebruikersnaam of IP adres:',
'ipblocklist-submit'       => 'Soek',
'blocklistline'            => '$1, $2 het $3 geblok ($4)',
'infiniteblock'            => 'oneindig',
'expiringblock'            => 'verval op $1',
'anononlyblock'            => 'anoniem-alleen',
'createaccountblock'       => 'skep van gebruikersrekeninge is geblokkeer',
'emailblock'               => 'e-pos versper',
'ipblocklist-empty'        => 'Die blokkeerlys is leeg.',
'ipblocklist-no-results'   => 'Die IP-adres of gebruikersnaam is nie geblokkeer nie.',
'blocklink'                => 'blok',
'unblocklink'              => 'maak oop',
'contribslink'             => 'bydraes',
'blocklogpage'             => 'Blokkeerlogboek',
'blocklogentry'            => '"[[$1]]" is vir \'n periode van $2 $3 geblok',
'blocklogtext'             => "Hier is 'n lys van onlangse blokkeer en deblokkeer aksies. Outomaties geblokkeerde IP-adresse word nie vertoon nie. 
Sien die [[Special:IPBlockList|IP-bloklys]] vir geblokkeerde adresse.",
'unblocklogentry'          => 'blokkade van $1 is opgehef:',
'block-log-flags-anononly' => 'anonieme gebruikers alleenlik',
'block-log-flags-noemail'  => 'e-pos versper',
'ipb_expiry_invalid'       => 'Ongeldige duur.',
'ipb_already_blocked'      => '"$1" is reeds geblok',
'ip_range_invalid'         => 'Ongeldige IP waardegebied.',
'blockme'                  => 'Versper my',
'proxyblocker'             => 'Proxyblokker',
'proxyblocker-disabled'    => 'Die funksie is gedeaktiveer.',
'proxyblocksuccess'        => 'Voltooi.',

# Developer tools
'lockdb'              => 'Sluit databasis',
'unlockdb'            => 'Ontsluit databasis',
'lockdbtext'          => 'As jy die databasis sluit, kan geen gebruiker meer bladsye redigeer nie, voorkeure verander nie, dophoulyste verander nie, of ander aksies uitvoer wat veranderinge in die databasis verg nie.
Bevestig asseblief dat dit is wat jy wil doen en dat jy die databasis sal ontsluit sodra jy jou instandhouding afgehandel het.',
'unlockdbtext'        => 'As jy die databasis ontsluit, kan gebruikers weer bladsye redigeer, voorkeure verander, dophoulyste verander, of ander aksies uitvoer wat veranderinge in die databasis verg.
Bevestig asseblief dat dit is wat jy wil doen.',
'lockconfirm'         => 'Ja, ek wil regtig die databasis sluit.',
'unlockconfirm'       => 'Ja, ek wil regtig die databasis ontsluit.',
'lockbtn'             => 'Sluit die databasis',
'unlockbtn'           => 'Ontsluit die databasis',
'locknoconfirm'       => "Jy het nie die 'bevestig' blokkie gemerk nie.",
'lockdbsuccesssub'    => 'Databasissluit het geslaag',
'unlockdbsuccesssub'  => 'Databasisslot is verwyder',
'lockdbsuccesstext'   => 'Die {{SITENAME}} databasis is gesluit.
<br />Onthou om dit te ontsluit wanneer jou onderhoud afgehandel is.',
'unlockdbsuccesstext' => 'Die {{SITENAME}}-databasis is ontsluit.',
'databasenotlocked'   => 'Die databasis is nie gesluit nie.',

# Move page
'move-page'               => 'Skuif "$1"',
'move-page-legend'        => 'Skuif bladsy',
'movepagetext'            => "Die vorm hieronder hernoem 'n bladsy en skuif die hele wysigingsgeskiedenis na die nuwe naam.
Die ou bladsy sal vervang word met 'n aanstuurblad na die nuwe titel.
'''Skakels na die ou bladsytitel sal nie outomaties verander word nie; maak seker dat dubbele aanstuurverwysings nie voorkom nie deur die \"wat skakel hierheen\"-funksie na die skuif te gebruik.''' Dit is jou verantwoordelikheid om seker te maak dat skakels steeds wys na waarheen hulle behoort te gaan.

Let daarop dat 'n bladsy '''nie''' geskuif sal word indien daar reeds 'n bladsy met dieselfde titel bestaan nie, tensy dit leeg of 'n aanstuurbladsy is en geen wysigingsgeskiedenis het nie. Dit beteken dat jy 'n bladsy kan terugskuif na sy ou titel indien jy 'n fout gemaak het, maar jy kan nie 'n bestaande bladsy oorskryf nie.

<b>WAARSKUWING!</b>
Hierdie kan 'n drastiese en onverwagte verandering vir 'n populêre bladsy wees;
maak asseblief seker dat jy die gevolge van hierdie aksie verstaan voordat jy voortgaan. Gebruik ook die ooreenstemmende besprekingsbladsy om oorleg te pleeg met ander bydraers.",
'movepagetalktext'        => "Die ooreenstemmende besprekingsblad sal outomaties saam geskuif word, '''tensy:'''
*'n Besprekengsblad met die nuwe naam reeds bestaan, of
*U die keuse hieronder deselekteer.

Indien wel sal u self die blad moet skuif of versmelt (indien nodig).",
'movearticle'             => 'Skuif bladsy',
'movenotallowed'          => 'U het nie regte om bladsye te skuif nie.',
'newtitle'                => 'Na nuwe titel',
'move-watch'              => 'Hou hierdie bladsy dop',
'movepagebtn'             => 'Skuif bladsy',
'pagemovedsub'            => 'Verskuiwing het geslaag',
'movepage-moved'          => '<big>\'\'\'"$1" is geskuif na "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => "'n Bladsy met daardie naam bestaan reeds, of die naam wat jy gekies het, is nie geldig nie.
Kies asseblief 'n ander naam.",
'cantmove-titleprotected' => "U kan nie 'n bladsy na die titel skuif nie, omdat die nuwe titel beskerm is teen die skep daarvan.",
'talkexists'              => "'''Die bladsy self is suksesvol geskuif, maar die besprekingsbladsy is nie geskuif nie omdat een reeds bestaan met die nuwe titel. Smelt hulle asseblief met die hand saam.'''",
'movedto'                 => 'geskuif na',
'movetalk'                => 'Skuif besprekingsblad ook, indien van toepassing.',
'move-subpages'           => 'Skuif al die subbladsye, indien toepaslik',
'move-talk-subpages'      => 'Skuif al die subbladsye van die besprekingsblad',
'movepage-page-exists'    => 'Die bladsy $1 bestaan reeds en kan nie outomaties oorskryf word nie.',
'movepage-page-moved'     => 'Die bladsy $1 was na $2 geskuif.',
'movepage-page-unmoved'   => 'Die bladsy $1 kon nie na $2 geskuif word nie.',
'1movedto2'               => '[[$1]] geskuif na [[$2]]',
'1movedto2_redir'         => '[[$1]] geskuif na [[$2]] oor bestaande aanstuur',
'movelogpage'             => 'Skuiflogboek',
'movelogpagetext'         => "Hieronder is 'n lys van geskuifde bladsye.",
'movereason'              => 'Rede:',
'revertmove'              => 'rol terug',
'delete_and_move'         => 'Skrap en skuif',
'delete_and_move_text'    => '==Skrapping benodig==

Die teikenartikel "[[:$1]]" bestaan reeds. Wil u dit skrap om plek te maak vir die skuif?',
'delete_and_move_confirm' => 'Ja, skrap die bladsy',
'delete_and_move_reason'  => 'Geskrap om plek te maak vir skuif',
'selfmove'                => 'Bron- en teikentitels is dieselfde; kan nie bladsy oor homself skuif nie.',
'imageinvalidfilename'    => 'Die nuwe lêernaam is ongeldig',
'fix-double-redirects'    => 'Opdateer alle aansture wat na die oorspronklike titel wys',

# Export
'export'            => 'Eksporteer bladsye',
'exporttext'        => 'U kan die teks en geskiedenis van \'n bladsy of bladsye na XML-formaat eksporteer.
Die eksportlêer kan daarna geïmporteer word na enige ander MediaWiki webwerf via die [[Special:Import|importeer bladsy]] skakel.

Verskaf die name van die bladsye wat geëksporteer moet word in die onderstaande veld, een bladsy per lyn, en kies of u alle weergawes (met geskiedenis) of slegs die nuutste weergawe soek.

In die laatste geval kan u ook \'n verwysing gebruik, byvoorbeeld [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] vir die bladsy "{{Mediawiki:Mainpage}}".',
'exportcuronly'     => 'Slegs die nuutste weergawes, sonder volledige geskiedenis',
'export-submit'     => 'Eksporteer',
'export-addcattext' => 'Voeg bladsye by van kategorie:',
'export-addcat'     => 'Voeg by',
'export-download'   => 'Stoor as lêer',
'export-templates'  => 'Sluit sjablone in',

# Namespace 8 related
'allmessages'               => 'Stelselboodskappe',
'allmessagesname'           => 'Naam',
'allmessagesdefault'        => 'Verstekteks',
'allmessagescurrent'        => 'Huidige teks',
'allmessagestext'           => "Hierdie is 'n lys boodskappe wat beskikbaar is in die ''MediaWiki''-naamspasie.",
'allmessagesnotsupportedDB' => "Daar is geen ondersteuning vir '''{{ns:special}}:Allmessages''' omdat '''\$wgUseDatabaseMessages''' uitgeskakel is.",
'allmessagesfilter'         => 'Boodskapnaamfilter:',
'allmessagesmodified'       => 'Wys slegs gewysigdes',

# Thumbnails
'thumbnail-more'           => 'Vergroot',
'filemissing'              => 'Lêer is weg',
'thumbnail_error'          => 'Fout met die skep van duimnaelsketse: $1',
'thumbnail_invalid_params' => 'Ongeldige parameters vir duimnaelskets',

# Special:Import
'import'                     => 'Voer bladsye in',
'import-interwiki-submit'    => 'importeer',
'import-interwiki-namespace' => 'Plaas bladsye in naamruimte:',
'importstart'                => 'Importeer bladsye...',
'import-revision-count'      => '$1 {{PLURAL:$1|weergawe|weergawes}}',
'importnopages'              => 'Geen bladsye om te importeer nie.',
'importfailed'               => 'Intrek onsuksesvol: $1',
'importunknownsource'        => 'Onbekende brontipe.',
'importcantopen'             => 'Kon nie lêer oopmaak nie',
'importbadinterwiki'         => 'Verkeerde interwiki skakel',
'importnotext'               => 'Leeg of geen teks',
'importsuccess'              => 'Klaar met importering!',
'import-noarticle'           => 'Geen bladsye om te importeer nie!',
'xml-error-string'           => '$1 op reël $2, kolom $3 (greep $4): $5',
'import-upload'              => 'Laai XML-data op',

# Import log
'importlogpage'                    => 'Invoer logboek',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|weergawe|weergawes}}',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|weergawe|weergawes}} vanaf $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'My gebruikerbladsy',
'tooltip-pt-anonuserpage'         => 'Die gebruikerbladsy vir die IP-adres waaronder u redigeer',
'tooltip-pt-mytalk'               => 'My besprekingsbladsy',
'tooltip-pt-anontalk'             => 'Bespreking oor bydraes van hierdie IP-adres',
'tooltip-pt-preferences'          => 'My voorkeure',
'tooltip-pt-watchlist'            => 'Die lys bladsye wat jy vir veranderinge dophou',
'tooltip-pt-mycontris'            => 'Lys van my bydraes',
'tooltip-pt-login'                => 'Jy word aangemoedig om in te teken; dit is egter nie verpligtend nie.',
'tooltip-pt-anonlogin'            => 'Jy word aangemoedig om in te teken; dit is egter nie verpligtend nie.',
'tooltip-pt-logout'               => 'Teken uit',
'tooltip-ca-talk'                 => 'Bespreking oor die inhoudsbladsy',
'tooltip-ca-edit'                 => 'Jy kan hierdie bladsy redigeer. Gebruik asseblief die voorskouknop vóór jy dit stoor.',
'tooltip-ca-addsection'           => 'Lewer kommentaar by hierdie bespreking.',
'tooltip-ca-viewsource'           => 'Hierdie bladsy is beskerm. Jy kan die bronteks besigtig.',
'tooltip-ca-history'              => 'Ouer weergawes van hierdie bladsy.',
'tooltip-ca-protect'              => 'Beskerm hierdie bladsy',
'tooltip-ca-delete'               => 'Skrap hierdie bladsy',
'tooltip-ca-undelete'             => 'Herstel die bydraes aan hierdie bladsy voordat dit geskrap is',
'tooltip-ca-move'                 => 'Skuif hierdie bladsy',
'tooltip-ca-watch'                => 'Voeg hierdie bladsy tot u dophoulys',
'tooltip-ca-unwatch'              => 'Verwyder hierdie bladsy van u dophoulys',
'tooltip-search'                  => 'Deursoek {{SITENAME}}',
'tooltip-search-fulltext'         => 'Deursoek die bladsye vir die teks',
'tooltip-p-logo'                  => 'Tuisblad',
'tooltip-n-mainpage'              => 'Besoek die Tuisblad',
'tooltip-n-portal'                => 'Meer oor die projek, wat jy kan doen, nuttige skakels',
'tooltip-n-currentevents'         => "'n Plek waar almal gesellig kan verkeer",
'tooltip-n-recentchanges'         => "'n Lys van onlangse wysigings",
'tooltip-n-randompage'            => "Laai 'n lukrake bladsye",
'tooltip-n-help'                  => 'Vind meer uit oor iets.',
'tooltip-t-whatlinkshere'         => "'n Lys bladsye wat hierheen skakel",
'tooltip-t-recentchangeslinked'   => 'Onlangse wysigings aan bladsye wat vanaf hierdie bladsy geskakel is',
'tooltip-feed-rss'                => 'RSS-voed vir hierdie bladsy',
'tooltip-feed-atom'               => 'Atom-voed vir hierdie bladsy',
'tooltip-t-contributions'         => "Bekyk 'n lys van bydraes deur hierdie gebruiker",
'tooltip-t-emailuser'             => "Stuur 'n e-pos aan hierdie gebruiker",
'tooltip-t-upload'                => 'Laai lêers op',
'tooltip-t-specialpages'          => "'n Lys van al die spesiale bladsye",
'tooltip-t-print'                 => 'Drukbare weergawe van hierdie bladsy',
'tooltip-t-permalink'             => "'n Permanente skakel na hierdie weergawe van die bladsy",
'tooltip-ca-nstab-main'           => 'Bekyk die inhoudbladsy',
'tooltip-ca-nstab-user'           => 'Bekyk die gebruikerbladsy',
'tooltip-ca-nstab-media'          => 'Bekyk die mediabladsy',
'tooltip-ca-nstab-special'        => "Hierdie is 'n spesiale bladsy; u kan dit nie wysig nie",
'tooltip-ca-nstab-project'        => 'Bekyk die projekbladsy',
'tooltip-ca-nstab-image'          => 'Bekyk die leêrbladsy',
'tooltip-ca-nstab-mediawiki'      => 'Bekyk die stelselboodskap',
'tooltip-ca-nstab-template'       => 'Bekyk die sjabloon',
'tooltip-ca-nstab-help'           => 'Bekyk die hulpbladsy',
'tooltip-ca-nstab-category'       => 'Bekyk die kategoriebladsy',
'tooltip-minoredit'               => "Dui aan hierdie is 'n klein wysiging",
'tooltip-save'                    => 'Stoor jou wysigings',
'tooltip-preview'                 => "Sien 'n voorskou van jou wysigings, gebruik voor jy die blad stoor!",
'tooltip-diff'                    => 'Wys watter veranderinge u aan die teks gemaak het.',
'tooltip-compareselectedversions' => 'Vergelyk die twee gekose weergawes van hierdie blad.',
'tooltip-watch'                   => 'Voeg hierdie blad by jou dophoulys',
'tooltip-recreate'                => 'Herskep hierdie bladsy al is dit voorheen geskrap',
'tooltip-upload'                  => 'Begin oplaai',

# Stylesheets
'common.css' => '/** Gemeenskaplike CSS vir alle omslae */',

# Attribution
'anonymous'        => 'Anonieme gebruiker(s) van {{SITENAME}}',
'siteuser'         => '{{SITENAME}} gebruiker $1',
'lastmodifiedatby' => 'Hierdie bladsy is laaste gewysig $2, $1 deur $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Gebaseer op werk van $1.',
'others'           => 'ander',
'siteusers'        => '{{SITENAME}} gebruiker(s) $1',

# Info page
'infosubtitle'   => 'Inligting vir bladsy',
'numedits'       => 'Aantal wysigings (bladsy): $1',
'numtalkedits'   => 'Aantal wysigings (besprekingsblad): $1',
'numwatchers'    => 'Aantal dophouers: $1',
'numauthors'     => 'Aantal outeurs (bladsy): $1',
'numtalkauthors' => 'Aantal outeurs (besprekingsblad): $1',

# Math options
'mw_math_png'    => 'Gebruik altyd PNG.',
'mw_math_simple' => 'Gebruik HTML indien dit eenvoudig is, andersins PNG.',
'mw_math_html'   => 'Gebruik HTML wanneer moontlik, andersins PNG.',
'mw_math_source' => 'Los as TeX (vir teksblaaiers).',
'mw_math_modern' => 'Moderne blaaiers.',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Merk as gekontroleerd',
'markaspatrolledtext'                 => 'Merk hierdie bladsy as gekontroleerd',
'markedaspatrolled'                   => 'As gekontroleerd gemerk',
'markedaspatrolledtext'               => 'Die gekose weergawe is as gekontroleerd gemerk.',
'rcpatroldisabled'                    => 'Onlangse Wysigingskontrolering buiten staat gestel',
'rcpatroldisabledtext'                => 'Die Onlangse Wysigingskontroleringsfunksie is tans buiten staat gestel.',
'markedaspatrollederror'              => 'Kan nie as gekontroleerd merk nie',
'markedaspatrollederrortext'          => "U moet 'n weergawe spesifiseer om as gekontroleerd te merk.",
'markedaspatrollederror-noautopatrol' => 'U kan nie u eie veranderinge as gekontroleerd merk nie.',

# Patrol log
'patrol-log-page' => 'Kontroleringslogboek',
'patrol-log-line' => 'merk $1 van $2 as gepatrolleer $3',
'patrol-log-auto' => '(outomaties)',

# Image deletion
'deletedrevision'                 => 'Ou weergawe $1 geskrap',
'filedeleteerror-short'           => 'Fout met verwydering van lêer: $1',
'filedeleteerror-long'            => 'Foute het voorgekom by die skraping van die lêer:

$1',
'filedelete-missing'              => 'Die lêer "$1" kan nie geskrap word nie, want dit bestaan nie.',
'filedelete-current-unregistered' => 'Die gespesifiseerde lêer "$1" is nie in die databasis nie.',

# Browsing diffs
'previousdiff' => '← Ouer wysiging',
'nextdiff'     => 'Nuwer wysiging →',

# Media information
'imagemaxsize'         => 'Beperk beelde op beeldbeskrywingsbladsye tot:',
'thumbsize'            => 'Grootte van duimnaelskets:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|bladsy|bladsye}}',
'file-info'            => '(lêergrootte: $1, MIME-tipe: $2)',
'file-info-size'       => '($1 × $2 pixels, lêergrootte: $3, MIME type: $4)',
'file-nohires'         => '<small>Geen hoëre resolusie beskikbaar nie.</small>',
'svg-long-desc'        => '(SVG-lêer, nominaal $1 × $2 pixels, lêergrootte: $3)',
'show-big-image'       => 'Volle resolusie',
'show-big-image-thumb' => '<small>Grootte van hierdie voorskou: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'             => 'Gallery van nuwe beelde',
'imagelisttext'         => "Hieronder is a lys van '''$1''' {{PLURAL:$1|lêer|lêers}}, $2 gesorteer.",
'newimages-summary'     => 'Die spesiale bladsy wys die nuutste lêers wat na die wiki opgelaai is.',
'showhidebots'          => '($1 robotte)',
'noimages'              => 'Niks te sien nie.',
'ilsubmit'              => 'Soek',
'bydate'                => 'volgens datum',
'sp-newimages-showfrom' => 'Wys nuwe lêers vanaf $2, $1',

# Bad image list
'bad_image_list' => "Die formaat is as volg:

Slegs lys-items (lyne wat met * begin) word verwerk.
Die eerste skakel op 'n lyn moet na 'n ongewenste lêer skakel.
Enige opeenvolgende skakels op dieselfde lyn word as uitsonderings beskou, b.v. blaaie waar die lêer inlyn kan voorkom.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "Die lêer bevat aanvullende inligting wat moontlik deur 'n digitale kamera of skandeerder bygevoeg is. 
As die lêer verander is, mag sekere inligting nie meer ooreenkom met die van die gewysigde lêer nie.",
'metadata-expand'   => 'Wys uitgebreide gegewens',
'metadata-collapse' => 'Steek uitgebreide gegewens weg',
'metadata-fields'   => 'Die EXIF-metadatavelde wat in die boodskap gelys is sal op die beeld se bladsy ingesluit word as die metadatabel ingevou is. 
Ander velde sal versteek wees.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'          => 'Wydte',
'exif-imagelength'         => 'Hoogte',
'exif-orientation'         => 'Oriëntasie',
'exif-samplesperpixel'     => 'Aantal komponente',
'exif-ycbcrpositioning'    => 'Y- en C-posisionering',
'exif-xresolution'         => 'Horisontale resolusie',
'exif-yresolution'         => 'Vertikale resolusie',
'exif-resolutionunit'      => 'Eenheid X en Y resolusie',
'exif-imagedescription'    => 'Beeldtitel',
'exif-make'                => 'Kamera vervaardiger:',
'exif-model'               => 'Kamera model',
'exif-software'            => 'Sagteware gebruik',
'exif-artist'              => 'Outeur',
'exif-copyright'           => 'Kopiereghouer',
'exif-exifversion'         => 'Exif weergawe',
'exif-colorspace'          => 'Kleurruimte',
'exif-makernote'           => 'Notas van vervaardiger',
'exif-usercomment'         => 'Opmerkings',
'exif-datetimeoriginal'    => 'Gegewens opgestel op',
'exif-exposuretime'        => 'Beligtingstyd',
'exif-exposuretime-format' => '$1 sek ($2)',
'exif-fnumber'             => 'F-getal',
'exif-shutterspeedvalue'   => 'Sluitersnelheid',
'exif-aperturevalue'       => 'Diafragma',
'exif-brightnessvalue'     => 'Helderheid',
'exif-lightsource'         => 'Ligbron',
'exif-flash'               => 'Flits',
'exif-focallength'         => 'Brandpuntsafstand',
'exif-flashenergy'         => 'Flitssterkte',
'exif-exposureindex'       => 'Beligtingsindeks',
'exif-filesource'          => 'Lêerbron',
'exif-scenetype'           => 'Soort toneel',
'exif-whitebalance'        => 'Witbalans',
'exif-scenecapturetype'    => 'Soort opname',
'exif-gaincontrol'         => 'Toneelbeheer',
'exif-contrast'            => 'Kontras',
'exif-saturation'          => 'Versadiging',
'exif-sharpness'           => 'Skerpte',
'exif-imageuniqueid'       => 'Unieke beeld ID',
'exif-gpsversionid'        => 'GPS-merkerweergawe',
'exif-gpslatituderef'      => 'Noorder- of suiderbreedte',
'exif-gpslatitude'         => 'Breedtegraad',
'exif-gpslongituderef'     => 'Ooster- of westerlengte',
'exif-gpslongitude'        => 'Lengtegraad',
'exif-gpsaltituderef'      => 'Hoogteverwysing',
'exif-gpsaltitude'         => 'Hoogte',
'exif-gpstimestamp'        => 'GPS-tyd (atoomhorlosie)',
'exif-gpssatellites'       => 'Satelliete gebruik vir meting',
'exif-gpsstatus'           => 'Ontvangerstatus',
'exif-gpsmeasuremode'      => 'Meetmodus',
'exif-gpsdop'              => 'Meetpresisie',
'exif-gpsspeedref'         => 'Snelheid eenheid',
'exif-gpsspeed'            => 'Snelheid van GPS-ontvanger',
'exif-gpstrackref'         => 'Verwysing vir bewegingsrigting',
'exif-gpstrack'            => 'Bewegingsrigting',
'exif-gpsimgdirectionref'  => 'Verwysing vir rigting van beeld',
'exif-gpsimgdirection'     => 'Rigting van beeld',
'exif-gpsdestlatitude'     => 'Breedtegraad bestemming',
'exif-gpsdestlongitude'    => 'Lengtegraad bestemming',
'exif-gpsdestbearing'      => 'Rigting na bestemming',
'exif-gpsdestdistance'     => 'Afstand na bestemming',
'exif-gpsprocessingmethod' => 'GPS-verwerkingsmetode',
'exif-gpsareainformation'  => 'Naam van GPS-gebied',
'exif-gpsdatestamp'        => 'GPS-datum',
'exif-gpsdifferential'     => 'Differensiële GPS-korreksie',

# EXIF attributes
'exif-compression-1' => 'Ongekompakteerd',

'exif-unknowndate' => 'Datum onbekend',

'exif-orientation-1' => 'Normaal', # 0th row: top; 0th column: left
'exif-orientation-3' => '180° gedraai', # 0th row: bottom; 0th column: right
'exif-orientation-6' => '90° regs gedraai', # 0th row: right; 0th column: top
'exif-orientation-8' => '90° links gedraai', # 0th row: left; 0th column: bottom

'exif-componentsconfiguration-0' => 'bestaan nie',

'exif-exposureprogram-0' => 'Nie bepaal',
'exif-exposureprogram-1' => 'Handmatig',
'exif-exposureprogram-4' => 'Sluiterprioriteit',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Onbekend',
'exif-meteringmode-1'   => 'Gemiddeld',
'exif-meteringmode-5'   => 'Patroon',
'exif-meteringmode-6'   => 'Gedeeltelik',
'exif-meteringmode-255' => 'Ander',

'exif-lightsource-0'   => 'Onbekend',
'exif-lightsource-1'   => 'Sonlig',
'exif-lightsource-2'   => 'Fluoresserend',
'exif-lightsource-4'   => 'Flits',
'exif-lightsource-9'   => 'Mooi weer',
'exif-lightsource-10'  => 'Bewolkte weer',
'exif-lightsource-11'  => 'Skaduwee',
'exif-lightsource-17'  => 'Standaard lig A',
'exif-lightsource-18'  => 'Standaard lig B',
'exif-lightsource-19'  => 'Standaard lig C',
'exif-lightsource-255' => 'Ander ligbron',

'exif-focalplaneresolutionunit-2' => 'duim',

'exif-sensingmethod-1' => 'Ongedefineer',

'exif-scenetype-1' => "'n Direk gefotografeerde beeld",

'exif-exposuremode-0' => 'Outomatiese beligting',
'exif-exposuremode-1' => 'Handmatige beligting',

'exif-scenecapturetype-0' => 'Standaard',
'exif-scenecapturetype-1' => 'Landskap',
'exif-scenecapturetype-2' => 'Portret',
'exif-scenecapturetype-3' => 'Nagtoneel',

'exif-gaincontrol-0' => 'Geen',

'exif-contrast-0' => 'Normaal',
'exif-contrast-1' => 'Sag',
'exif-contrast-2' => 'Hard',

'exif-saturation-0' => 'Normaal',
'exif-saturation-1' => 'Laag',
'exif-saturation-2' => 'Hoog',

'exif-sharpness-0' => 'Normaal',
'exif-sharpness-1' => 'Sag',
'exif-sharpness-2' => 'Hard',

'exif-subjectdistancerange-0' => 'Onbekend',
'exif-subjectdistancerange-2' => 'Naby',
'exif-subjectdistancerange-3' => 'Vêr weg',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Noorderbreedte',
'exif-gpslatitude-s' => 'Suiderbreedte',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Oosterlengte',
'exif-gpslongitude-w' => 'Westerlengte',

'exif-gpsstatus-a' => 'Besig met meting',

'exif-gpsmeasuremode-2' => '2-dimensionele meting',
'exif-gpsmeasuremode-3' => '3-dimensionele meting',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer per huur',
'exif-gpsspeed-m' => 'Myl per huur',
'exif-gpsspeed-n' => 'Knope',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Regte rigting',
'exif-gpsdirection-m' => 'Magnetiese rigting',

# External editor support
'edit-externally'      => "Wysig hierdie lêer met 'n eksterne program",
'edit-externally-help' => 'Sien die [http://www.mediawiki.org/wiki/Manual:External_editors instruksies] (in Engels) vir meer inligting.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alles',
'imagelistall'     => 'alle',
'watchlistall2'    => 'alles',
'namespacesall'    => 'alle',
'monthsall'        => 'alle',

# E-mail address confirmation
'confirmemail'             => 'Bevestig e-posadres',
'confirmemail_text'        => "Hierdie wiki vereis dat u e-posadres bevestig word voordat epos-funksies gebruik word. Klik onderstaande knoppie om 'n bevestigingspos na u adres te stuur. Die pos sal 'n skakel met 'n kode insluit; maak hierdie skakel oop in u webblaaier om te bevestig dat die adres geldig is.",
'confirmemail_send'        => "Pos 'n bevestigingkode",
'confirmemail_sent'        => 'Bevestigingpos gestuur.',
'confirmemail_invalid'     => 'Ongeldige bevestigingkode. Die kode het moontlik verval.',
'confirmemail_needlogin'   => 'U moet $1 om u e-posadres te bevestig.',
'confirmemail_success'     => 'U e-posadres is bevestig. U kan nou aanteken en die wiki gebruik.',
'confirmemail_loggedin'    => 'U e-posadres is nou bevestig.',
'confirmemail_error'       => 'Iets het foutgegaan met die stoor van u bevestiging.',
'confirmemail_subject'     => '{{SITENAME}}: E-posadres-bevestiging',
'confirmemail_body'        => 'Iemand, waarskynlik van u IP-adres: $1, het \'n rekening "$2" geregistreer met hierdie e-posadres by {{SITENAME}}.

Om te bevestig dat hierdie adres werklik aan u behoort, en om die posfasiliteite by {{SITENAME}} te aktiveer, besoek hierdie skakel in u blaaier:

$3

Indien dit nié u was nie, ignoreer bloot die skakel (en hierdie pos).
Follow this link to cancel the e-mail address confirmation:

$5

Hierde bevestigingkode verval om $4.',
'confirmemail_invalidated' => 'Die e-pos bevestiging is gekanselleer.',
'invalidateemail'          => 'Kanselleer e-pos bevestiging',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-invoeging van sjablone is afgeskakel]',
'scarytranscludefailed'   => '[Sjabloon $1 kon nie gelaai word nie]',
'scarytranscludetoolong'  => '[Die URL is te lank]',

# Trackbacks
'trackbackremove' => ' ([$1 Skrap])',

# Delete conflict
'deletedwhileediting' => "'''Let op''': die bladsy is verwyder terwyl u besig was om dit te wysig!",
'confirmrecreate'     => "Gebruiker [[User:$1|$1]] ([[User talk:$1|bespreek]]) het hierdie blad uitgevee ná u begin redigeer het met rede: : ''$2''
Bevestig asseblief dat u regtig hierdie blad oor wil skep.",
'recreate'            => 'Herskep',

# HTML dump
'redirectingto' => 'Stuur aan na [[:$1]]...',

# action=purge
'confirm_purge'        => 'Verwyder die kas van hierdie blad?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Soek na bladsye wat ''$1'' bevat.",
'searchnamed'      => "Soek vir bladsye genaamd ''$1''.",
'articletitles'    => "Artikels wat met ''$1'' begin",
'hideresults'      => 'Steek resultate weg',
'useajaxsearch'    => 'Gebruik AJAX-soek',

# Multipage image navigation
'imgmultipageprev' => '← vorige bladsy',
'imgmultipagenext' => 'volgende bladsy →',
'imgmultigo'       => 'Gaan!',
'imgmultigoto'     => 'Gaan na bladsy $1',

# Table pager
'table_pager_next'         => 'Volgende bladsy',
'table_pager_prev'         => 'Vorige bladsy',
'table_pager_first'        => 'Eerste bladsy',
'table_pager_last'         => 'Laaste bladsy',
'table_pager_limit'        => 'Wys $1 resultate per bladsy',
'table_pager_limit_submit' => 'Gaan',
'table_pager_empty'        => 'Geen resultate',

# Auto-summaries
'autosumm-blank'   => 'Alle inhoud uit bladsy verwyder',
'autosumm-replace' => "Vervang bladsyinhoud met '$1'",
'autoredircomment' => 'Stuur aan na [[$1]]',
'autosumm-new'     => 'Nuwe blad: $1',

# Size units
'size-bytes'     => '$1 G',
'size-kilobytes' => '$1 KG',
'size-megabytes' => '$1 MG',
'size-gigabytes' => '$1 GG',

# Live preview
'livepreview-loading' => 'Laai tans…',
'livepreview-ready'   => 'Laai tans… Gereed!',
'livepreview-failed'  => 'Lewendige voorskou het gefaal.
Probeer normale voorskou.',
'livepreview-error'   => 'Verbinding het misluk: $1 "$2".
Probeer normale voorskou.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Veranderinge nuwer as $1 {{PLURAL:$1|sekonde|sekondes}} mag moontlik nie gewys word nie.',

# Watchlist editor
'watchlistedit-numitems'       => 'U dophoulys bevat {{PLURAL:$1|1 bladsy|$1 bladsye}}, besprekingsbladsye uitgesluit.',
'watchlistedit-noitems'        => 'U dophoulys bevat geen bladsye.',
'watchlistedit-normal-title'   => 'Wysig dophoulys',
'watchlistedit-normal-legend'  => 'Verwyder titels van dophoulys',
'watchlistedit-normal-explain' => "Die bladsye in u dophoulys word hieronder vertoon. 
Selekteer die titels wat verwyder moet word en klik op 'Verwyder Titels' onder aan die bladsy.
Alternatiewelik kan u die [[Special:Watchlist/raw|bronkode wysig]].",
'watchlistedit-normal-submit'  => 'Verwyder Titels',
'watchlistedit-normal-done'    => 'Daar is {{PLURAL:$1|1 bladsy|$1 bladsye}} van u dophoulys verwyder:',
'watchlistedit-raw-title'      => 'Wysig u dophoulys se bronkode',
'watchlistedit-raw-legend'     => 'Wysig u dophoulys se bronkode',
'watchlistedit-raw-explain'    => "Die bladsye in u dophoulys word hieronder vertoon.
U kan die lys wysig deur titels by te sit of te verwyder (een bladsy per lyn).
As u klaar is, klik op 'Opdateer Dophoulys' onder aan die bladsy.
U kan ook die [[Special:Watchlist/edit|standaard opdaterigskerm gebruik]].",
'watchlistedit-raw-titles'     => 'Titels:',
'watchlistedit-raw-submit'     => 'Opdateer Dophoulys',
'watchlistedit-raw-done'       => 'U dophoulys is opgedateer.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 titel|$1 titels}} was bygevoeg:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 titel|$1 titels}} verwyder:',

# Watchlist editing tools
'watchlisttools-view' => 'Besigtig veranderinge',
'watchlisttools-edit' => 'Bekyk en wysig dophoulys',
'watchlisttools-raw'  => 'Wysig bronkode',

# Core parser functions
'unknown_extension_tag' => 'Onbekende etiket "$1"',

# Special:Version
'version'                       => 'Weergawe', # Not used as normal message but as header for the special page itself
'version-extensions'            => 'Uitbreidings geïnstalleer',
'version-specialpages'          => 'Spesiale bladsye',
'version-parserhooks'           => 'Ontlederhoeke',
'version-variables'             => 'Veranderlikes',
'version-other'                 => 'Ander',
'version-mediahandlers'         => 'Mediaverwerkers',
'version-hooks'                 => 'Hoeke',
'version-extension-functions'   => 'Uitbreidingsfunksies',
'version-parser-extensiontags'  => 'Ontleder-uitbreidingsetikette',
'version-parser-function-hooks' => 'Ontleder-funksiehoeke',
'version-hook-name'             => 'Hoek naam',
'version-hook-subscribedby'     => 'Gebruik deur',
'version-version'               => 'Weergawe',
'version-license'               => 'Lisensie',
'version-software'              => 'Geïnstalleerde sagteware',
'version-software-product'      => 'Produk',
'version-software-version'      => 'Weergawe',

# Special:FilePath
'filepath'         => 'Lêerpad',
'filepath-page'    => 'Lêer:',
'filepath-submit'  => 'Pad',
'filepath-summary' => 'Die spesiale bladsy wys die volledige pad vir \'n lêer. 
Beelde word in hulle volle resolusie gewys. Ander lêertipes word direk met hulle MIME-geskakelde programme geopen.

Sleutel die lêernaam in sonder die "{{ns:image}}:" voorvoegsel.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Soek duplikaat lêers',
'fileduplicatesearch-legend'   => "Soek vir 'n duplikaat",
'fileduplicatesearch-filename' => 'Lêernaam:',
'fileduplicatesearch-submit'   => 'Soek',
'fileduplicatesearch-info'     => '$1 × $2 pixels<br />Lêergrootte: $3<br />MIME-tipe: $4',
'fileduplicatesearch-result-1' => 'Die lêer "$1" het geen identiese duplikate nie.',
'fileduplicatesearch-result-n' => 'Die lêer "$1" het {{PLURAL:$2|een identiese duplikaat|$2 identiese duplikate}}.',

# Special:SpecialPages
'specialpages'                   => 'Spesiale bladsye',
'specialpages-note'              => '----
* Normale spesiale bladsye.
* <span class="mw-specialpagerestricted">Beperkte spesiale bladsye.</span>',
'specialpages-group-maintenance' => 'Onderhoud verslae',
'specialpages-group-other'       => 'Ander spesiale bladsye',
'specialpages-group-login'       => 'Inteken / aansluit',
'specialpages-group-changes'     => 'Onlangse wysigings en boekstawings',
'specialpages-group-media'       => 'Media verslae en oplaai',
'specialpages-group-users'       => 'Gebruikers en regte',
'specialpages-group-highuse'     => 'Baie gebruikte bladsye',
'specialpages-group-pages'       => 'Lys van bladsye',
'specialpages-group-pagetools'   => 'Bladsyhulpmiddels',
'specialpages-group-wiki'        => 'Wiki data en hulpmiddels',
'specialpages-group-redirects'   => 'Aanstuur gewone bladsye',
'specialpages-group-spam'        => 'Spam-hulpmiddels',

# Special:BlankPage
'blankpage'              => 'Leë bladsy',
'intentionallyblankpage' => 'Die bladsy is bewustelik leeg gelaat',

);
