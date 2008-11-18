<?php
/** Slovak (Slovenčina)
 *
 * @ingroup Language
 * @file
 *
 * @author Helix84
 * @author Liso
 * @author Maros
 * @author Michawiki
 * @author Palica
 * @author Sp5uhe
 * @author Valasek
 * @author לערי ריינהארט
 */

$datePreferences = array(
	'default',
	'dmyt',
	'short dmyt',
	'tdmy',
	'short dmyt',
	'ISO 8601',
);

$datePreferenceMigrationMap = array(
	'default',
	'dmyt',
	'short dmyt',
	'tdmy',
	'short tdmy',
);

$dateFormats = array(
	/*
	'Default',
	'15. január 2001 16:12',
	'15. jan. 2001 16:12',
	'16:12, 15. január 2001',
	'16:12, 15. jan. 2001',
	'ISO 8601' => '2001-01-15 16:12:34'*/

	'dmy time' => 'H:i',
	'dmy date' => 'j. F Y',
	'dmy both' => 'H:i, j. F Y',

	'dmyt time' => 'H:i',
	'dmyt date' => 'j. F Y',
	'dmyt both' => 'j. F Y H:i',

	'short dmyt time' => 'H:i',
	'short dmyt date' => 'j. M. Y',
	'short dmyt both' => 'j. M. Y H:i',

	'tdmy time' => 'H:i',
	'tdmy date' => 'j. F Y',
	'tdmy both' => 'H:i, j. F Y',

	'short tdmy time' => 'H:i',
	'short tdmy date' => 'j. M. Y',
	'short tdmy both' => 'H:i, j. M. Y',

);

$bookstoreList = array(
	'Bibsys' => 'http://ask.bibsys.no/ask/action/result?cmd=&kilde=biblio&fid=isbn&term=$1',
	'BokBerit' => 'http://www.bokberit.no/annet_sted/bocker/$1.html',
	'Bokkilden' => 'http://www.bokkilden.no/ProductDetails.aspx?ProductId=$1',
	'Haugenbok' => 'http://www.haugenbok.no/searchresults.cfm?searchtype=simple&isbn=$1',
	'Akademika' => 'http://www.akademika.no/sok.php?isbn=$1',
	'Gnist' => 'http://www.gnist.no/sok.php?isbn=$1',
	'Amazon.co.uk' => 'http://www.amazon.co.uk/exec/obidos/ISBN=$1',
	'Amazon.de' => 'http://www.amazon.de/exec/obidos/ISBN=$1',
	'Amazon.com' => 'http://www.amazon.com/exec/obidos/ISBN=$1'
);

# Note to translators:
# Please include the English words as synonyms. This allows people
# from other wikis to contribute more easily.
#
$magicWords = array(
	'redirect'            => array( '0', '#redirect', '#presmeruj' ),
	'notoc'               => array( '0', '__NOTOC__', '__BEZOBSAHU__' ),
	'forcetoc'            => array( '0', '__FORCETOC__', '__VYNÚŤOBSAH__' ),
	'toc'                 => array( '0', '__TOC__', '__OBSAH__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__NEUPRAVUJSEKCIE__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'MESIAC' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'MENOMESIACA' ),
	'currentmonthnamegen' => array( '1', 'CURRENTMONTHNAMEGEN', 'MENOAKTUÁLNEHOMESIACAGEN' ),
	'currentmonthabbrev'  => array( '1', 'CURRENTMONTHABBREV', 'MENOAKTUÁLNEHOMESIACASKRATKA' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'AKTUÁLNYDEŇ' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'MENOAKTUÁLNEHODŇA' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'AKTUÁLNYROK' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'AKTUÁLNYČAS' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'POČETČLÁNKOV' ),
	'pagename'            => array( '1', 'PAGENAME', 'MENOSTRÁNKY' ),
	'namespace'           => array( '1', 'NAMESPACE', 'MENNÝPRIESTOR' ),
	'msg'                 => array( '0', 'MSG:', 'SPRÁVA:' ),
	'img_thumbnail'       => array( '1', 'thumbnail', 'thumb', 'náhľad', 'náhľadobrázka' ),
	'img_right'           => array( '1', 'right', 'vpravo' ),
	'img_left'            => array( '1', 'left', 'vľavo' ),
	'img_none'            => array( '1', 'none', 'žiadny' ),
	'img_width'           => array( '1', '$1px', '$1bod' ),
	'img_center'          => array( '1', 'center', 'centre', 'stred' ),
	'img_framed'          => array( '1', 'framed', 'enframed', 'frame', 'rám' ),
	'sitename'            => array( '1', 'SITENAME', 'MENOLOKALITY' ),
	'ns'                  => array( '0', 'NS:', 'MP:' ),
	'grammar'             => array( '0', 'GRAMMAR:', 'GRAMATIKA:' ),
	'notitleconvert'      => array( '0', '__NOTITLECONVERT__', '__NOTC__' ),
	'nocontentconvert'    => array( '0', '__NOCONTENTCONVERT__', '__NOCC__' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'AKTUÁLNYTÝŽDEŇ' ),
);

$namespaceNames = array(
	NS_MEDIA          => 'Médiá',
	NS_SPECIAL        => 'Špeciálne',
	NS_MAIN           => '',
	NS_TALK           => 'Diskusia',
	NS_USER           => 'Redaktor',
	NS_USER_TALK      => 'Diskusia_s_redaktorom',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'Diskusia_k_{{grammar:datív|$1}}',
	NS_IMAGE          => 'Obrázok',
	NS_IMAGE_TALK     => 'Diskusia_k_obrázku',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Diskusia_k_MediaWiki',
	NS_TEMPLATE       => 'Šablóna',
	NS_TEMPLATE_TALK  => 'Diskusia_k_šablóne',
	NS_HELP           => 'Pomoc',
	NS_HELP_TALK      => 'Diskusia_k_pomoci',
	NS_CATEGORY       => 'Kategória',
	NS_CATEGORY_TALK  => 'Diskusia_ku_kategórii',
);

# Compatbility with old names
$namespaceAliases = array(
	"Komentár"               => NS_TALK,
	"Komentár_k_redaktorovi" => NS_USER_TALK,
	"Komentár_k_Wikipédii"   => NS_PROJECT_TALK,
	"Komentár_k_obrázku"     => NS_IMAGE_TALK,
	"Komentár_k_MediaWiki"   => NS_MEDIAWIKI_TALK,
);

$separatorTransformTable = array(
	',' => "\xc2\xa0",
	'.' => ','
);

$linkTrail = '/^([a-záäčďéíľĺňóôŕšťúýž]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Podčiarkovať odkazy:',
'tog-highlightbroken'         => 'Neexistujúce odkazy formátovať <a href="" class="new">takto</a> (alternatívne: takto<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Zarovnávať okraje odstavcov',
'tog-hideminor'               => 'V posledných úpravách nezobrazovať drobné úpravy',
'tog-extendwatchlist'         => 'Rozšíriť zoznam sledovaných, aby ukazoval všetky súvisiace zmeny',
'tog-usenewrc'                => 'Rozšírené zobrazenie posledných úprav (vyžaduje JavaScript)',
'tog-numberheadings'          => 'Automaticky číslovať nadpisy',
'tog-showtoolbar'             => 'Zobraziť panel nástrojov úprav',
'tog-editondblclick'          => 'Upravovať stránky po dvojitom kliknutí (JavaScript)',
'tog-editsection'             => 'Umožniť úpravu sekcie pomocu odkazov [upraviť]',
'tog-editsectiononrightclick' => 'Umožniť upravovať sekcie po kliknutí pravým tlačidlom na nadpisy sekcií (JavaScript)',
'tog-showtoc'                 => 'Zobrazovať obsah (pre stránky s viac ako 3 nadpismi)',
'tog-rememberpassword'        => 'Zapamätať si heslo na tomto počítači',
'tog-editwidth'               => 'Maximálna šírka textového poľa úprav',
'tog-watchcreations'          => 'Pridávať stránky, ktoré vytvorím, automaticky medzi sledované',
'tog-watchdefault'            => 'Pridávať stránky, ktoré upravujem, automaticky medzi sledované',
'tog-watchmoves'              => 'Pridávať stránky, ktoré presuniem, do môjho zoznamu sledovaných',
'tog-watchdeletion'           => 'Pridávať stránky, ktoré zmažem, do môjho zoznamu sledovaných',
'tog-minordefault'            => 'Označovať všetky zmeny štandardne ako drobné',
'tog-previewontop'            => 'Zobrazovať náhľad pred textovým poľom úprav, nie až za ním',
'tog-previewonfirst'          => 'Zobraziť náhľad pred prvou úpravou',
'tog-nocache'                 => 'Zakázať priebežné ukladanie stránok do vyrovnávacej pamäte',
'tog-enotifwatchlistpages'    => 'Upozorniť ma emailom, keď sa zmení stránka z môjho zoznamu sledovaných',
'tog-enotifusertalkpages'     => 'Upozorniť ma emailom po zmene mojej používateľskej diskusnej stránky',
'tog-enotifminoredits'        => 'Upozorniť ma emailom aj na drobné úpravy stránok',
'tog-enotifrevealaddr'        => 'Zobraziť moju emailovú adresu v emailoch s upozorneniami',
'tog-shownumberswatching'     => 'Zobraziť počet používateľov sledujúcich stránku',
'tog-fancysig'                => 'Nespracovávať podpisy (bez automatických odkazov)',
'tog-externaleditor'          => 'Používať štandardne externý editor (iba pre expertov, vyžaduje špeciálne nastavenie vášho počítača)',
'tog-externaldiff'            => 'Používať štandardne externý diff (iba pre expertov, vyžaduje špeciálne nastavenie vášho počítača)',
'tog-showjumplinks'           => 'Používať odkazy „skočiť na“ pre lepšiu dostupnosť',
'tog-uselivepreview'          => 'Používať živý náhľad (JavaScript) (experimentálna funkcia)',
'tog-forceeditsummary'        => 'Upozoriť ma, keď nevyplním zhrnutie úprav',
'tog-watchlisthideown'        => 'Skryť moje úpravy zo zoznamu sledovaných',
'tog-watchlisthidebots'       => 'Skryť úpravy botov zo zoznamu sledovaných',
'tog-watchlisthideminor'      => 'Skryť drobné úpravy zo zoznamu sledovaných',
'tog-nolangconversion'        => 'Vypnúť konverziu variantov',
'tog-ccmeonemails'            => 'Posielať mi kópie mojich emailov, ktoré pošlem ostatným používateľom',
'tog-diffonly'                => 'Nezobrazovať obsah stránky pod rozdielmi',
'tog-showhiddencats'          => 'Zobraziť skryté kategórie',

'underline-always'  => 'Vždy',
'underline-never'   => 'Nikdy',
'underline-default' => 'Štandardné nastavenie prehliadača',

'skinpreview' => '(Náhľad)',

# Dates
'sunday'        => 'nedeľa',
'monday'        => 'pondelok',
'tuesday'       => 'utorok',
'wednesday'     => 'streda',
'thursday'      => 'štvrtok',
'friday'        => 'piatok',
'saturday'      => 'sobota',
'sun'           => 'Ned',
'mon'           => 'Pon',
'tue'           => 'Uto',
'wed'           => 'Str',
'thu'           => 'Štv',
'fri'           => 'Pia',
'sat'           => 'Sob',
'january'       => 'január',
'february'      => 'február',
'march'         => 'marec',
'april'         => 'apríl',
'may_long'      => 'máj',
'june'          => 'jún',
'july'          => 'júl',
'august'        => 'august',
'september'     => 'september',
'october'       => 'október',
'november'      => 'november',
'december'      => 'december',
'january-gen'   => 'januára',
'february-gen'  => 'februára',
'march-gen'     => 'marca',
'april-gen'     => 'apríla',
'may-gen'       => 'mája',
'june-gen'      => 'júna',
'july-gen'      => 'júla',
'august-gen'    => 'augusta',
'september-gen' => 'septembra',
'october-gen'   => 'októbra',
'november-gen'  => 'novembra',
'december-gen'  => 'decembra',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mar',
'apr'           => 'apr',
'may'           => 'máj',
'jun'           => 'jún',
'jul'           => 'júl',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategória|Kategórie|Kategórie}}',
'category_header'                => 'Stránky v kategórii „$1“',
'subcategories'                  => 'Podkategórie',
'category-media-header'          => 'Multimediálne súbory v kategórii „$1“',
'category-empty'                 => "''Táto kategória momentálne neobsahuje články ani multimediálne súbory.''",
'hidden-categories'              => '{{PLURAL:$1|Skrytá kategória|Skryté kategórie}}',
'hidden-category-category'       => 'Skryté kategórie', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Táto kategória má nasledovnú podkategóriu.|Táto kategória má {{PLURAL:$1|nasledovnú podkategóriu|nasledovné $1 podkategórie|nasledovných $1 podkategórií}} z $2 celkom.}}',
'category-subcat-count-limited'  => 'Táto kategória má {{PLURAL:$1|nasledovnú podkategóriu|nasledovné $1 podkategórie|nasledovných $1 podkategórií}}.',
'category-article-count'         => '{{PLURAL:$2|Táto kategória obsahuje iba nasledovnú stránku.|V tejto kategórii sa {{PLURAL:$1|nachádza jedna stránka|nachádzajú $1 stránky|nachádza $1 stránok}} z $2 celkom.}}',
'category-article-count-limited' => 'V tejto kategórii sa {{PLURAL:$1|nachádza jedna stránka|nachádzajú $1 stránky|nachádza $1 stránok}}.',
'category-file-count'            => '{{PLURAL:$2|Táto kategória obsahuje iba nasledovný súbor.|V tejto kategórii sa {{PLURAL:$1|nachádza jeden súbor|nachádzajú $1 súbory|nachádza $1 súborov}} z $2 celkom.}}',
'category-file-count-limited'    => 'V tejto kategórii sa {{PLURAL:$1|nachádza jeden súbor|nachádzajú $1 súbory|nachádza $1 súborov}}',
'listingcontinuesabbrev'         => 'pokrač.',

'mainpagetext'      => "<big>'''Softvér MediaWiki bol úspešne nainštalovaný.'''</big>",
'mainpagedocfooter' => 'Informácie ako používať wiki softvér nájdete v [http://meta.wikimedia.org/wiki/Help:Contents Používateľskej príručke].

== Začíname ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Zoznam konfiguračných nastavení]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce mailing list nových verzií MediaWiki]',

'about'          => 'Projekt',
'article'        => 'Stránka s obsahom',
'newwindow'      => '(otvorí v novom okne)',
'cancel'         => 'Zrušiť',
'qbfind'         => 'Hľadať',
'qbbrowse'       => 'Prehliadať',
'qbedit'         => 'Upraviť',
'qbpageoptions'  => 'Táto stránka',
'qbpageinfo'     => 'Kontext',
'qbmyoptions'    => 'Moje stránky',
'qbspecialpages' => 'Špeciálne stránky',
'moredotdotdot'  => 'Viac...',
'mypage'         => 'Moja stránka',
'mytalk'         => 'Moja diskusia',
'anontalk'       => 'Diskusia k tejto IP adrese',
'navigation'     => 'Navigácia',
'and'            => 'a',

# Metadata in edit box
'metadata_help' => 'Metadáta:',

'errorpagetitle'    => 'Chyba',
'returnto'          => 'Späť na $1.',
'tagline'           => 'Z {{GRAMMAR:genitív|{{SITENAME}}}}',
'help'              => 'Pomoc',
'search'            => 'Hľadať',
'searchbutton'      => 'Hľadať',
'go'                => 'Vykonať',
'searcharticle'     => 'Ísť na',
'history'           => 'História stránky',
'history_short'     => 'História',
'updatedmarker'     => 'aktualizované od mojej poslednej návštevy',
'info_short'        => 'Informácie',
'printableversion'  => 'Verzia na tlač',
'permalink'         => 'Trvalý odkaz',
'print'             => 'Tlač',
'edit'              => 'upraviť',
'create'            => 'Vytvoriť',
'editthispage'      => 'Upraviť túto stránku',
'create-this-page'  => 'Vytvoriť túto stránku',
'delete'            => 'Vymazať',
'deletethispage'    => 'Vymazať túto stránku',
'undelete_short'    => 'Obnoviť {{PLURAL:$1|jednu úpravu|$1 úpravy|$1 úprav}}',
'protect'           => 'Zamknúť',
'protect_change'    => 'Zmeniť',
'protectthispage'   => 'Zamknúť túto stránku',
'unprotect'         => 'Odomknúť',
'unprotectthispage' => 'Odomknúť túto stránku',
'newpage'           => 'Nová stránka',
'talkpage'          => 'Diskusia k stránke',
'talkpagelinktext'  => 'Diskusia',
'specialpage'       => 'Špeciálna stránka',
'personaltools'     => 'Osobné nástroje',
'postcomment'       => 'Pridať komentár',
'articlepage'       => 'Zobraziť stránku',
'talk'              => 'Diskusia',
'views'             => 'Zobrazení',
'toolbox'           => 'Nástroje',
'userpage'          => 'Zobraziť stránku používateľa',
'projectpage'       => 'Zobraziť projektovú stránku',
'imagepage'         => 'Zobraziť popisnú stránku súboru',
'mediawikipage'     => 'Zobraziť stránku so správou',
'templatepage'      => 'Zobraziť stránku šablóny',
'viewhelppage'      => 'Zobraziť stránku Pomocníka',
'categorypage'      => 'Zobraziť stránku kategórie',
'viewtalkpage'      => 'Zobraziť diskusiu k stránke',
'otherlanguages'    => 'V iných jazykoch',
'redirectedfrom'    => '(Presmerované z $1)',
'redirectpagesub'   => 'Presmerovacia stránka',
'lastmodifiedat'    => 'Čas poslednej úpravy tejto stránky je $2, $1.', # $1 date, $2 time
'viewcount'         => 'Táto stránka bola navštívená {{PLURAL:$1|raz|$1-krát|$1-krát}}.',
'protectedpage'     => 'Zamknutá stránka',
'jumpto'            => 'Prejsť na:',
'jumptonavigation'  => 'navigácia',
'jumptosearch'      => 'hľadanie',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'O {{GRAMMAR:lokál|{{SITENAME}}}}',
'aboutpage'            => 'Project:Úvod',
'bugreports'           => 'Oznámenia o chybách',
'bugreportspage'       => 'Project:Oznámenia o chybách',
'copyright'            => 'Obsah je k dispozícii za licenčných podmienok $1.',
'copyrightpagename'    => 'autorské práva {{GRAMMAR:genitív|{{SITENAME}}}}',
'copyrightpage'        => '{{ns:project}}:Autorské práva',
'currentevents'        => 'Aktuality',
'currentevents-url'    => 'Project:Aktuality',
'disclaimers'          => 'Vylúčenie zodpovednosti',
'disclaimerpage'       => 'Project:Vylúčenie zodpovednosti',
'edithelp'             => 'Ako upravovať stránku',
'edithelppage'         => 'Help:Ako upravovať stránku',
'faq'                  => 'Často kladené otázky',
'faqpage'              => 'Project:Často_kladené_otázky',
'helppage'             => 'Help:Obsah',
'mainpage'             => 'Hlavná stránka',
'mainpage-description' => 'Hlavná stránka',
'policy-url'           => 'Project:Zásady a smernice',
'portal'               => 'Portál komunity',
'portal-url'           => 'Project:Portál komunity',
'privacy'              => 'Ochrana osobných údajov',
'privacypage'          => 'Project:Ochrana osobných údajov',

'badaccess'        => 'Chyba povolenia',
'badaccess-group0' => 'Nemáte povolenie na vykonanie požadovanej činnosti.',
'badaccess-group1' => 'Činnosť, ktorú požadujete môže vykonať iba člen skupiny $1.',
'badaccess-group2' => 'Činnosť, ktorú požadujete môže vykonať iba člen jednej zo skupín $1.',
'badaccess-groups' => 'Činnosť, ktorú požadujete môže vykonať iba člen jednej zo skupín $1.',

'versionrequired'     => 'Požadovaná verzia MediaWiki $1',
'versionrequiredtext' => 'Aby ste mohli používať túto stránku, požaduje sa verzia MediaWiki $1. Pozri [[Special:Version]].',

'ok'                      => 'OK',
'retrievedfrom'           => 'Zdroj: „$1“',
'youhavenewmessages'      => 'Máte $1 ($2).',
'newmessageslink'         => 'nové správy',
'newmessagesdifflink'     => 'posledná zmena',
'youhavenewmessagesmulti' => 'Máte nové správy na $1',
'editsection'             => 'upraviť',
'editold'                 => 'upraviť',
'viewsourceold'           => 'zobraziť zdroj',
'editsectionhint'         => 'Upraviť sekciu: $1',
'toc'                     => 'Obsah',
'showtoc'                 => 'zobraziť',
'hidetoc'                 => 'skryť',
'thisisdeleted'           => 'Zobraziť alebo obnoviť $1?',
'viewdeleted'             => 'Zobraziť $1?',
'restorelink'             => '{{PLURAL:$1|jednu zmazanú úpravu|$1 zmazané úpravy|$1 zmazaných úprav}}',
'feedlinks'               => 'Kanál:',
'feed-invalid'            => 'Neplatný typ kanála.',
'feed-unavailable'        => 'Kanály nie sú dostupné',
'site-rss-feed'           => 'RSS kanál $1',
'site-atom-feed'          => 'Atom kanál $1',
'page-rss-feed'           => 'RSS kanál „$1“',
'page-atom-feed'          => 'Atom kanál „$1“',
'red-link-title'          => '$1 (zatiaľ nevytvorená)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Stránka',
'nstab-user'      => 'Stránka používateľa',
'nstab-media'     => 'Multimédiá',
'nstab-special'   => 'Špeciálne',
'nstab-project'   => 'Projektová stránka',
'nstab-image'     => 'Súbor',
'nstab-mediawiki' => 'Správa',
'nstab-template'  => 'Šablóna',
'nstab-help'      => 'Pomoc',
'nstab-category'  => 'Kategória',

# Main script and global functions
'nosuchaction'      => 'Takáto činnosť neexistuje',
'nosuchactiontext'  => 'Softvér MediaWiki nepozná akciu,
ktorú vyžadujete pomocou URL.',
'nosuchspecialpage' => 'Takáto špeciálna stránka neexistuje',
'nospecialpagetext' => "<big>'''Vyžiadali ste si neplatnú špeciálnu stránku.'''</big>

Zoznam platných špeciálnych stránok nájdete na [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Chyba',
'databaseerror'        => 'Chyba v databáze',
'dberrortext'          => 'Nastala syntaktická chyba v príkaze na prehľadávanie databázy.
Posledný pokus o prehľadávanie bol:
<blockquote><tt>$1</tt></blockquote>
z funkcie „<tt>$2</tt>“.
MySQL vrátil chybu „<tt>$3: $4</tt>“.',
'dberrortextcl'        => 'Nastala syntaktická chyba pri požiadavke do databázy.
Posledný pokus o požiadavku do databázy znel:
„$1“
z funkcie „$2“.
MySQL vrátil chybu „$3: $4“.',
'noconnect'            => 'Prepáčte! Wiki má technické problémy a nemôže kontaktovať databázový server.<br />
$1',
'nodb'                 => 'Nebolo možné vybrať databázu $1',
'cachederror'          => 'Nasledujúca stránka je kópia vyžiadanej stránky z vyrovnávacej pamäte a nemusí byť aktuálna.',
'laggedslavemode'      => 'Upozornenie: Je možné, že stránka neobsahuje posledné aktualizácie.',
'readonly'             => 'Databáza je zamknutá',
'enterlockreason'      => 'Zadajte dôvod požadovaného zamknutia vrátane odhadu, kedy očakávate odomknutie',
'readonlytext'         => 'Databáza je momentálne zamknutá, nové stránky a úpravy sú zablokované, pravdepodobne z dôvodu údržby databázy. Po skončení tejto údržby bude {{SITENAME}} opäť fungovať normálne.

Správca, ktorý nariadil uzamknutie, uvádza tento dôvod: $1',
'missing-article'      => 'Text stránky s názvom „$1” $2, ktorú ste požadovali, nebol nájdený v databáze. 

To sa zvyčajne stane, keď kliknete na zastaralý odkaz na rozdiel alebo do histórie stránky, ktorá bola zmazaná.

Ak to tak nie je, je možné, že ste našli chybu v softvéri.
Oznámte to prosím [[Special:ListUsers/sysop|správcovi]] a uveďte URL.',
'missingarticle-rev'   => '(č. revízie: $1)',
'missingarticle-diff'  => '(rozdiel: $1, $2)',
'readonly_lag'         => 'Databáza bola automaticky zamknutá pokým záložné databázové servery nedoženú hlavný server',
'internalerror'        => 'Vnútorná chyba',
'internalerror_info'   => 'Vnútorná chyba: $1',
'filecopyerror'        => 'Nebolo možné skopírovať súbor „$1“ na „$2“.',
'filerenameerror'      => 'Nebolo možné premenovať súbor „$1“ na „$2“.',
'filedeleteerror'      => 'Nebolo možné vymazať súbor „$1“.',
'directorycreateerror' => 'Nebolo možné vytvoriť adresár „$1“.',
'filenotfound'         => 'Nebolo možné nájsť súbor „$1“.',
'fileexistserror'      => 'Nebolo možné zapisovať do súboru „$1“: súbor existuje',
'unexpected'           => 'Neočakávaná hodnota: „$1“=„$2“.',
'formerror'            => 'Chyba: nepodarilo sa odoslať formulár',
'badarticleerror'      => 'Na tejto stránke túto činnosť nemožno vykonať.',
'cannotdelete'         => 'Nebolo možné zmazať danú stránku alebo súbor. (Možno už bol zmazaný niekým iným.)',
'badtitle'             => 'Neplatný nadpis',
'badtitletext'         => 'Požadovaný nadpis bol neplatný, nezadaný, alebo nesprávne odkazovaný z inej jazykovej verzie {{GRAMMAR:genitív|{{SITENAME}}}}. Mohol tiež obsahovať jeden alebo viac znakov, ktoré nie je možné použiť v nadpisoch.',
'perfdisabled'         => 'Prepáčte! Táto funkcia bola dočasne vypnutá,
pretože tak spomaľuje databázu, že nikto nemôže používať
wiki.',
'perfcached'           => 'Nasledujúce údaje pochádzajú z vyrovnávacej pamäte a nemusia byť úplne aktuálne.',
'perfcachedts'         => 'Nasledujúce údaje pochádzajú z vyrovnávacej pamäte a naposledy boli aktualizované $1.',
'querypage-no-updates' => 'Aktualizácie tejto stránky sú momentálne vypnuté. Tieto dáta sa v súčasnosti nebudú obnovovať.',
'wrong_wfQuery_params' => 'Nesprávny parameter pre wfQuery()<br />
Funkcia: $1<br />
Požiadavka: $2',
'viewsource'           => 'Zobraziť zdroj',
'viewsourcefor'        => '$1',
'actionthrottled'      => 'Činnosť bola spomalená',
'actionthrottledtext'  => 'Ako opatrenie proti spamu je počet vykonaní tejto činnosti za určitý čas obmedzený. Tento limit ste prekročili. Prosím, skúste to znova o niekoľko minút.',
'protectedpagetext'    => 'Táto stránka bola zamknutá aby sa zamedzilo úpravám.',
'viewsourcetext'       => 'Môžete si zobraziť a kopírovať zdroj tejto stránky:',
'protectedinterface'   => 'Táto stránka poskytuje text používateľského rozhrania a je zamknutá, aby sa predišlo jej zneužitiu.',
'editinginterface'     => "'''Upozornenie:''' Upravujete stránku, ktorá poskytuje text používateľského rozhrania. Zmeny tejto stránky ovplyvnia vzhľad používateľského rozhrania ostatných používateľov. Zmeny prosím vykonávajte prostredníctvom [http://translatewiki.net/wiki/Main_Page?setlang=sk Betawiki], projektu pre lokalizáciu MediaWiki.",
'sqlhidden'            => '(SQL príkaz je skrytý)',
'cascadeprotected'     => 'Táto stránka bola zamknutá proti úpravám, pretože je použitá na {{PLURAL:$1|nasledovnej stránke, ktorá je zamknutá|nasledovných stránkach, ktoré sú zamknuté}} voľbou „kaskádového zamknutia“:
$2',
'namespaceprotected'   => "Nemáte povolenie upravovať stránky v mennom priestore '''$1'''.",
'customcssjsprotected' => 'Nemáte povolenie na úpravu tejto stránky, pretože obsahuje osobné nastavenia iného používateľa.',
'ns-specialprotected'  => 'Stránky v mennom pristore {{ns:special}} nie je možné upravovať.',
'titleprotected'       => "Používateľ [[User:$1|$1]] zabránil vytváraniu stránky s týmto názvom.
Udaný dôvod: ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Chybná konfigurácia: neznámy antivírus: <i>$1</i>',
'virus-scanfailed'     => 'kontrola zlyhala (kód $1)',
'virus-unknownscanner' => 'neznámy antivírus:',

# Login and logout pages
'logouttitle'                => 'Odhlásiť používateľa',
'logouttext'                 => '<strong>Práve ste sa odhlásili.</strong>

Odteraz môžete používať {{GRAMMAR:akuzatív|{{SITENAME}}}} ako anonymný používateľ alebo sa môžete
opäť [[Special:UserLogin|prihlásiť]] pod rovnakým alebo odlišným používateľským menom.
Uvedomte si, že niektoré stránky sa môžu naďalej zobrazovať ako keby ste boli prihlásený, až kým nevymažete
vyrovnávaciu pamäť vášho prehliadača.',
'welcomecreation'            => '== Vitaj, $1! ==

Vaše konto je vytvorené.
Nezabudnite si nastaviť svoje [[Special:Preferences|používateľské nastavenia]].',
'loginpagetitle'             => 'Prihlásenie používateľa',
'yourname'                   => 'Používateľské meno:',
'yourpassword'               => 'Heslo:',
'yourpasswordagain'          => 'Zopakujte heslo:',
'remembermypassword'         => 'Pamätať si prihlásenie na tomto počítači',
'yourdomainname'             => 'Vaša doména:',
'externaldberror'            => 'Buď nastala chyba externej autentifikačnej databázy alebo vám nie je povolené aktualizovať váš externý účet.',
'loginproblem'               => '<b>Nastal problém pri vašom prihlasovaní.</b><br />Skúste znova!',
'login'                      => 'Prihlásiť',
'nav-login-createaccount'    => 'Vytvorenie konta / prihlásenie',
'loginprompt'                => 'Na prihlásenie do {{GRAMMAR:genitív|{{SITENAME}}}} musíte mať zapnuté koláčiky (cookies).',
'userlogin'                  => 'Vytvorenie konta / prihlásenie',
'logout'                     => 'Odhlásiť',
'userlogout'                 => 'Odhlásiť',
'notloggedin'                => 'Neprihlásený/á',
'nologin'                    => 'Nemáte ešte účet? $1.',
'nologinlink'                => 'Vytvoriť nový účet',
'createaccount'              => 'Vytvoriť nový účet',
'gotaccount'                 => 'Máte už vytvorený účet? $1.',
'gotaccountlink'             => 'Prihlásiť',
'createaccountmail'          => 'e-mailom',
'badretype'                  => 'Zadané heslá nie sú rovnaké.',
'userexists'                 => 'Zadané používateľské meno už používa niekto iný.
Zadajte iné meno.',
'youremail'                  => 'Váš e-mail²',
'username'                   => 'Používateľské meno:',
'uid'                        => 'ID používateľa:',
'prefs-memberingroups'       => 'Člen {{PLURAL:$1|skupiny|skupín}}:',
'yourrealname'               => 'Skutočné meno *:',
'yourlanguage'               => 'Jazyk:',
'yourvariant'                => 'Variant jazyka:',
'yournick'                   => 'Podpis:',
'badsig'                     => 'Neplatný podpis v pôvodnom tvare; skontrolujte HTML značky.',
'badsiglength'               => 'Podpis je príliš dlhý.
Musí obsahovať menej ako $1 {{PLURAL:$1|znak|znaky|znakov}}.',
'email'                      => 'E-mail',
'prefs-help-realname'        => '¹ Skutočné meno (nepovinné): ak sa rozhodnete ho poskytnúť, bude použité na označenie vašej práce.',
'loginerror'                 => 'Chyba pri prihlasovaní',
'prefs-help-email'           => '² E-mail (nepovinné): Umožní iným ľuďom kontaktovať vás pomocou odkazu z vašej používateľskej a diskusnej stránky (bez potreby uverejňovania vašej e-mailovej adresy) a môže naň byť poslané nové heslo ak zabudnete pôvodné.',
'prefs-help-email-required'  => 'Vyžaduje sa e-mailová adresa.',
'nocookiesnew'               => 'Používateľské konto bolo vytvorené, ale nie ste prihlásený. {{SITENAME}} používa cookies na prihlásenie. Máte cookies vypnuté. Zapnite ich a potom sa prihláste pomocou vášho nového používateľského mena a hesla.',
'nocookieslogin'             => '{{SITENAME}} používa cookies na prihlásenie. Vy máte cookies vypnuté. Prosíme, zapnite ich a skúste znovu.',
'noname'                     => 'Nezadali ste platné používateľské meno.',
'loginsuccesstitle'          => 'Prihlásenie úspešné',
'loginsuccess'               => "'''Teraz ste prihlásený do {{GRAMMAR:genitív|{{SITENAME}}}} ako „$1“.'''",
'nosuchuser'                 => 'Používateľské meno „$1“ neexistuje.
Skontrolujte preklepy alebo sa [[Special:Userlogin/signup|zaregistrujte ako nový používateľ]].',
'nosuchusershort'            => 'V súčasnosti neexistuje používateľ s menom „<nowiki>$1</nowiki>“. Skontrolujte preklepy.',
'nouserspecified'            => 'Musíte uviesť meno používateľa.',
'wrongpassword'              => 'Zadané heslo je nesprávne. Skúste  znovu.',
'wrongpasswordempty'         => 'Zadané heslo bolo prázdne. Skúste prosím znova.',
'passwordtooshort'           => 'Vaše heslo je neplatné alebo príliš krátke.
Musí mať dĺžku aspoň {{PLURAL:$1|$1 znak|$1 znaky|$1 znakov}} a líšiť sa od vášho používateľského mena.',
'mailmypassword'             => 'Pošlite mi e-mailom dočasné heslo',
'passwordremindertitle'      => 'Nové dočasné heslo pre {{GRAMMAR:akuzatív|{{SITENAME}}}}',
'passwordremindertext'       => 'Niekto (pravdepodobne vy, z IP adresy $1)
požiadal, aby sme vám zaslali nové prihlasovacie heslo do {{GRAMMAR:genitív|{{SITENAME}}}} ($4).
Pre používateľa „$2“ bolo vytvorené nové heslo „$3“.
Ak to bolo vaším zámerom, teraz by ste sa mali prihlásiť a zmeniť svoje heslo.

Ak túto požiadavku poslal niekto iný alebo ste si spomenuli svoje heslo a neželáte
si ho zmeniť, môžete túto správu ignorovať a naďalej používať svoje staré heslo.',
'noemail'                    => 'Používateľ „$1“ nezadal e-mailovú adresu.',
'passwordsent'               => 'Nové heslo bolo zaslané na e-mailovú adresu
používateľa „$1“.
Prosím, prihláste sa znovu, keď ho dostanete.',
'blocked-mailpassword'       => 'Boli zablokované úpravy z vašej IP adresy, a tak nie je dovolené použiť funkciu znovuvyžiadania hesla, aby sa zabránilo zneužitiu.',
'eauthentsent'               => 'Email s potvrdením bol zaslaný na uvedenú emailovú adresu.
Predtým ako sa na účet pošle akákoľvek ďalšia pošta, musíte splniť inštrukcie v emaili, aby sa potvrdilo, že účet je skutočne váš.',
'throttled-mailpassword'     => 'V priebehu {{PLURAL:$1|poslednej $1 hodiny|posledných $1 hodín}} už došlo k vyžiadaniu hesla.
Aby sa zabránilo zneužitiu, vyžiadanie hesla je možné vykonať iba raz za {{PLURAL:$1|$1 hodinu|$1 hodiny|$1 hodín}}.',
'mailerror'                  => 'Chyba pri posielaní e-mailu: $1',
'acct_creation_throttle_hit' => 'Prepáčte, už máte vytvorených $1 účtov. Nemôžete ich z tejto IP adresy vytvoriť za 24 hodín viac. Toto je opatrenie proti vandalizmu.',
'emailauthenticated'         => 'Vaša e-mailová adresa bola overená na $1.',
'emailnotauthenticated'      => 'Vaša e-mailová adresa ešte nebola overená. Preto nemôžete prijať emaily pre žiadnu z nasledovných funkcií.',
'noemailprefs'               => '<strong>Nezadali ste žiadnu e-mailovú adresu</strong>, nasledujúce
nástroje nebudú prístupné.',
'emailconfirmlink'           => 'Potvrďte vašu e-mailovú adresu',
'invalidemailaddress'        => 'Emailovú adresu nemožno akceptovať, pretože sa zdá, že má neplatný formát. Zadajte adresu v správnom tvare alebo nechajte príslušné políčko prázdne.',
'accountcreated'             => 'Účet vytvorený',
'accountcreatedtext'         => 'Používateľský účet $1 bol vytvorený.',
'createaccount-title'        => 'Vytvorenie účtu na {{GRAMMAR:lokál|{{SITENAME}}}}',
'createaccount-text'         => 'Niekto vytvoril účet pre vašu emailovú adresu na {{GRAMMAR:lokál|{{SITENAME}}}}
($4) s názvom „$2“, s heslom „$3“. Mali by ste sa prihlásiť a svoje heslo teraz zmeniť.

Ak bol účet vytvorený omylom, túto správu môžete ignorovať.',
'loginlanguagelabel'         => 'Jazyk: $1',

# Password reset dialog
'resetpass'               => 'Zmeniť heslo k účtu',
'resetpass_announce'      => 'Prishlásili ste sa pomocou dočasného emailom zaslaného kódu. Pre dokončenie prihlásenia je potrebné tu nastaviť nové heslo:',
'resetpass_text'          => '<!-- Pridajte text sem -->',
'resetpass_header'        => 'Zmeniť heslo',
'resetpass_submit'        => 'Nastaviť heslo a prihlásiť sa',
'resetpass_success'       => 'Vaše heslo bolo úspešne zmenené! Prebieha prihlasovanie...',
'resetpass_bad_temporary' => 'Neplatné dočasné heslo. Možno ste už úspešne zmenili svoje heslo alebo vyžiadali nové dočasné heslo.',
'resetpass_forbidden'     => 'Heslá nie je možné zmeniť',
'resetpass_missing'       => 'Chýbajú údaje formulára.',

# Edit page toolbar
'bold_sample'     => 'Tučný text',
'bold_tip'        => 'Tučný text',
'italic_sample'   => 'Kurzíva',
'italic_tip'      => 'Kurzíva',
'link_sample'     => 'Názov odkazu',
'link_tip'        => 'Interný odkaz',
'extlink_sample'  => 'http://www.example.com názov odkazu',
'extlink_tip'     => 'Externý odkaz (nezabudnite na predponu http://)',
'headline_sample' => 'Text nadpisu',
'headline_tip'    => 'Text nadpisu úrovne 2',
'math_sample'     => 'Sem vložte vzorec',
'math_tip'        => 'Matematický vzorec (LaTeX)',
'nowiki_sample'   => 'Sem vložte neformátovaný text',
'nowiki_tip'      => 'Ignoruj wiki formátovanie',
'image_sample'    => 'Príklad.jpg',
'image_tip'       => 'Vložený obrázok',
'media_sample'    => 'Príklad.ogg',
'media_tip'       => 'Odkaz na media súbor',
'sig_tip'         => 'Váš podpis s dátumom a časom',
'hr_tip'          => 'Vodorovná čiara (radšej ju nepoužívajte)',

# Edit pages
'summary'                          => 'Zhrnutie úprav',
'subject'                          => 'Téma/nadpis',
'minoredit'                        => 'Toto je drobná úprava',
'watchthis'                        => 'Sledovať úpravy tejto stránky',
'savearticle'                      => 'Uložiť stránku',
'preview'                          => 'Náhľad',
'showpreview'                      => 'Zobraziť náhľad',
'showlivepreview'                  => 'Živý náhľad',
'showdiff'                         => 'Zobraziť rozdiely',
'anoneditwarning'                  => "'''Upozornenie:''' Nie ste prihlásený.
Vaša IP adresa bude zaznamenaná v histórii úprav tejto stránky.",
'missingsummary'                   => "'''Pripomienka:''' Neposkytli ste zhrnutie úprav. Ak kliknete znova na Uložiť, vaše úpravy sa uložia bez zhrnutia úprav.",
'missingcommenttext'               => 'Prosím, dolu napíšte komentár.',
'missingcommentheader'             => "'''Pripomienka:''' Neposkytli ste predmet/hlavičku tohto komentára. Ak znova kliknete na tlačidlo Uložiť, vaša úprava sa uloží bez nej.",
'summary-preview'                  => 'Náhľad zhrnutia',
'subject-preview'                  => 'Náhľad predmetu/hlavičky',
'blockedtitle'                     => 'Používateľ je zablokovaný',
'blockedtext'                      => "<big>'''Vaše používateľské meno alebo IP adresa bola zablokovaná.'''</big>

Zablokoval vás správca $1. Udáva tento dôvod:<br />''$2''

* Blokovanie začalo: $8
* Blokovanie vyprší: $6
* Kto mal byť zablokovaný: $7

Môžete kontaktovať $1 alebo s jedného z ďalších [[{{MediaWiki:Grouppage-sysop}}|správcov]] a prediskutovať blokovanie.
Uvedomte si, že nemôžete použiť funkciu „{{MediaWiki:Emailuser}}“, pokiaľ nemáte registrovanú platnú e-mailovú adresu vo svojich [[Special:Preferences|nastaveniach]].
Vaša IP adresa je $3 a ID blokovania je #$5.
Prosím, uveďte oba tieto údaje do každej správy, ktorú posielate.",
'autoblockedtext'                  => "Vaša IP adresa bola automaticky zablokovaná, pretože ju používa iný používateľ, ktorého zablokoval $1.
Udaný dôvod zablokovania:

:''$2''

* Blokovanie začalo: $8
* Blokovanie vyprší: $6
* Blokovanie sa týka: $7

Ak potrebujete informácie o blokovaní, môžete kontaktovať $1 alebo niektorého iného
[[{{MediaWiki:Grouppage-sysop}}|správcu]].

Pozn.: Nemôžete použiť funkciu „{{MediaWiki:emailuser}}“, ak ste si vo svojich
[[Special:Preferences|používateľských nastaveniach]] nezaregistrovali platnú emailovú adresu.

Vaša aktuálna IP adresa je $3. ID vášho blokovania je $5.
Prosím, uveďte tieto podrobnosti v akýchkoľvek otázkach, ktoré sa opýtate.",
'blockednoreason'                  => 'nebol uvedený dôvod',
'blockedoriginalsource'            => "Zdroj '''$1''' je zobrazený nižšie:",
'blockededitsource'                => "Text '''vašich úprav''' stránky '''$1''' je zobrazený nižšie:",
'whitelistedittitle'               => 'Aby ste mohli upravovať stránky, musíte sa prihlásiť',
'whitelistedittext'                => 'Aby ste mohli upravovať stránky, musíte sa $1',
'confirmedittitle'                 => 'Aby ste mohli upravovať je potrebné potvrdenie e-mailu',
'confirmedittext'                  => 'Pred úpravami stránok musíte potvrdiť vašu emailovú adresu. Prosím, nastavte a overte svoju emailovú adresu v [[Special:Preferences|používateľských nastaveniach]].',
'nosuchsectiontitle'               => 'Sekcia neexistuje',
'nosuchsectiontext'                => 'Pokúšali ste sa upravovať sekciu, ktorá neexistuje. Keďže sekcia $1 neexistuje, nie je kam uložiť vašu úpravu.',
'loginreqtitle'                    => 'Je potrebné prihlásiť sa',
'loginreqlink'                     => 'prihlásiť',
'loginreqpagetext'                 => 'Aby ste mohli prezerať ďalšie stránky, musíte sa $1.',
'accmailtitle'                     => 'Heslo bolo odoslané.',
'accmailtext'                      => 'Heslo pre „$1“ bolo poslané na $2.',
'newarticle'                       => '(Nový)',
'newarticletext'                   => "Sledovali ste odkaz na stránku, ktorá zatiaľ neexistuje.
Stránku vytvoríte tak, že začnete písať do dolného poľa a potom stlačíte tlačidlo „Uložiť stránku“.
(Viac informácií nájdete na stránkach [[{{MediaWiki:Helppage}}|Pomocníka]]).
Ak ste sa sem dostali nechtiac, iba kliknite na tlačidlo '''späť''' vo svojom prehliadači.",
'anontalkpagetext'                 => "----''Toto je diskusná stránka anonymného používateľa, ktorý nemá vytvorené svoje konto alebo ho nepoužíva. 
Preto musíme na jeho identifikáciu použiť numerickú IP adresu. Je možné, že takúto IP adresu používajú viacerí používatelia.
Ak ste anonymný používateľ a máte pocit, že vám boli adresované irelevantné diskusné príspevky, [[Special:UserLogin/signup|vytvorte si konto]] alebo sa [[Special:UserLogin|prihláste]], aby sa zamedzilo budúcim zámenám s inými anonymnými používateľmi.''",
'noarticletext'                    => 'Na tejto stránke sa momentálne nenachádza žiadny text. Môžete [[Special:Search/{{PAGENAME}}|vyhľadávať názov tejto stránky]] v obsahu iných stránok alebo [{{fullurl:{{FULLPAGENAME}}|action=edit}} upravovať túto stránku].',
'userpage-userdoesnotexist'        => 'Používateľský účet „$1“ nie je registrovaný. Prosím, zaškrtnite ak chcete vytvoriť/upravovať túto stránku.',
'clearyourcache'                   => "'''Poznámka: Aby sa zmeny prejavili, po uložení musíte vymazať vyrovnávaciu pamäť vášho prehliadača.'''
'''Mozilla / Firefox / Safari:''' držte stlačený ''Shift'' a kliknite na ''Reload'' alebo stlačte buď ''Ctrl-F5'' alebo ''Ctrl-R'' (''Comand-R'' na Macintosh); '''Konqueror:''': kliknite na tlačidlo ''Reload'' alebo stlačte ''F5''; '''Opera''' vymazať vyrovnávaciu pamäť prehliadača v ponuke ''Tools→Preferences''; '''Internet Explorer:''' držte ''Ctrl'' a kliknite na ''Refresh'' alebo stlačte ''Ctrl-F5'';",
'usercssjsyoucanpreview'           => '<strong>Tip:</strong> Váš nový CSS/JS pred uložením otestujete stlačením tlačidla „Zobraziť náhľad“.',
'usercsspreview'                   => "'''Nezabudnite, že toto je iba náhľad vášho používateľského CSS, ešte nebolo uložené!'''",
'userjspreview'                    => "'''Nezabudnite, že iba testujete/náhľad vášho používateľského JavaScriptu, ešte nebol uložený!'''",
'userinvalidcssjstitle'            => "'''Upozornenie:''' Neexistuje vzhľad „$1“. Pamätajte, že vlastné .css a .js stránky používajú názov s malými písmenami, napr. {{ns:user}}:Foo/monobook.css a nie {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Aktualizovaný)',
'note'                             => '<strong>Poznámka: </strong>',
'previewnote'                      => '<strong>Nezabudnite, toto je iba náhľad stránky, ktorú upravujete.
Zmeny ešte nie sú uložené!</strong>',
'previewconflict'                  => 'Tento náhľad upravenej stránky zobrazuje text z horného poľa s textom tak, ako sa zobrazí potom, keď ju uložíte.',
'session_fail_preview'             => '<strong>Prepáčte, nemohli sme spracovať váš príspevok kvôli strate údajov relácie.
Skúste to prosím ešte raz.
Ak to nebude fungovať, skúste sa [[Special:UserLogout|odhlásiť]] a znovu prihlásiť.</strong>',
'session_fail_preview_html'        => "<strong>Prepáčte! Nemohli sme spracovať vašu úpravu kvôli strate údajov relácie.</strong>

''Pretože {{SITENAME}} má použitie HTML umožnené, náhľad sa nezobrazí (prevencia pred JavaScript útokmi).''

<strong>Ak je toto legitímny pokus o úpravu, skúste to prosím znova. Ak to stále nefunguje, skúste sa [[Special:UserLogout|odhlásiť]] a znovu prihlásiť.</strong>",
'token_suffix_mismatch'            => '<strong>Vaša úprava bola zamietnutá, pretože váš klient pokazil znaky s diakritikou v editačnom symbole (token). Úprava bola zamietnutá, aby sa zabránilo poškodeniu textu stránky. Toto sa občas stáva, keď používate chybnú anonymnú proxy službu cez webové rozhranie.</strong>',
'editing'                          => 'Úprava stránky $1',
'editingsection'                   => 'Úprava stránky $1 (sekcia)',
'editingcomment'                   => 'Úprava stránky $1 (komentár)',
'editconflict'                     => 'Konflikt pri úprave: $1',
'explainconflict'                  => "Niekto iný zmenil túto stránku, zatiaľ čo ste ju upravovali vy.
Horné okno na úpravy obsahuje text stránky tak, ako je momentálne platný.
Vaše úpravy sú uvedené v dolnom okne na úpravy.
Budete musieť zlúčiť vaše zmeny s existujúcim textom.
'''Iba''' obsah horného okna sa uloží, keď stlačíte „{{MediaWiki:savearticle}}“.",
'yourtext'                         => 'Váš text',
'storedversion'                    => 'Uložená verzia',
'nonunicodebrowser'                => '<strong>UPOZORNENIE: Váš prehliadač nepodporuje unicode. Dočasným riešením ako bezpečne upravovať stránky je, že ne-ASCII znaky sa v upravovacom textovom poli zobrazia ako zodpovedajúce hexadecimálne hodnoty.</strong>',
'editingold'                       => '<strong>UPOZORNENIE: Upravujete starú
verziu tejto stránky. Ak vašu úpravu uložíte, prepíšete tým všetky úpravy, ktoré nasledovali po tejto starej verzii.</strong>',
'yourdiff'                         => 'Rozdiely',
'copyrightwarning'                 => 'Nezabudnite, že všetky príspevky do {{GRAMMAR:genitív|{{SITENAME}}}} sa považujú za príspevky pod licenciou $2 (podrobnosti pozri pod $1). Ak nechcete, aby bolo to, čo ste napísali, neúprosne upravované a ďalej ľubovoľne rozširované, tak sem váš text neumiestňujte.<br />

Týmto sa právne zaväzujete, že ste tento text buď napísali sám, alebo že je skopírovaný
z voľného diela (public domain) alebo podobného zdroja neobmedzeného autorskými právami.
<strong>NEUMIESTŇUJTE TU BEZ POVOLENIA DIELA CHRÁNENÉ AUTORSKÝM PRÁVOM!</strong>',
'copyrightwarning2'                => 'Prosím uvedomte si, že všetky príspevky do {{GRAMMAR:genitív|{{SITENAME}}}} môžu byť upravované, skracované alebo odstránené inými prispievateľmi. Ak nechcete, aby Vaše texty boli menené, tak ich tu neuverejňujte.<br />

Týmto sa právne zaväzujete, že ste tento text buď napísali sám, alebo že je skopírovaný
z voľného diela (public domain) alebo podobného zdroja neobmedzeného autorskými právami (podrobnosti: $1).
<strong>NEUMIESTŇUJTE SEM BEZ POVOLENIA DIELA CHRÁNENÉ AUTORSKÝM PRÁVOM!</strong>',
'longpagewarning'                  => '<strong>UPOZORNENIE: Táto stránka má $1 kilobajtov; niektoré
prehliadače by mohli mať problémy s úpravou stránok, ktorých veľkosť sa blíži k alebo presahuje 32kb.
Zvážte, či by nebolo možné rozdeliť stránku na menšie sekcie.</strong>',
'longpageerror'                    => '<strong>CHYBA: Text, ktorý ste poslali má $1 kilobajtov, čo je viac ako maximum $2 kilobajtov. Nie je možné ho uložiť.</strong>',
'readonlywarning'                  => '<strong>UPOZORNENIE: Databáza bola počas upravovania stránky zamknutá z dôvodu údržby,
takže stránku momentálne nemôžete uložiť. Môžete skopírovať a vložiť
text do textového súboru a uložiť si ho na neskôr.</strong>',
'protectedpagewarning'             => '<strong>UPOZORNENIE: Táto stránka bola zamknutá, takže ju môžu upravovať iba používatelia s oprávnením správcu.</strong>',
'semiprotectedpagewarning'         => "'''Poznámka:''' Táto stránka bola zamknutá tak, aby ju mohli upravovať iba registrovaní používatelia.",
'cascadeprotectedwarning'          => "'''Upozornenie:''' Táto stránka bola zamknutá (takže ju môžu upravovať iba používatelia s privilégiami správcu), pretože je použitá na {{PLURAL:$1|nasledovnej stránke|nasledovných stránkach}} s kaskádovým zamknutím:",
'titleprotectedwarning'            => '<strong>UPOZORNENIE: Táto stránka bola zamknutá, takže ju môžu vytvoriť iba niektorí používatelia.</strong>',
'templatesused'                    => 'Šablóny použité na tejto stránke:',
'templatesusedpreview'             => 'Šablóny použité v tomto náhľade:',
'templatesusedsection'             => 'Šablóny použité v tejto sekcii:',
'template-protected'               => '(zamknutá)',
'template-semiprotected'           => '(čiastočne zamknutá)',
'hiddencategories'                 => 'Táto stránka patrí do {{PLURAL:$1|1 skrytej kategórie|$1 skrytých kategórií}}:',
'edittools'                        => '<!-- Tento text sa zobrazí pod upravovacím a nahrávacím formulárom. -->',
'nocreatetitle'                    => 'Tvorba nových stránok bola obmedzená',
'nocreatetext'                     => 'Na {{GRAMMAR:lokál|{{SITENAME}}}} je tvorba nových stránok obmedzená.
Teraz sa môžete vrátiť späť a upravovať existujúcu stránku alebo [[Special:UserLogin|sa prihlásiť alebo vytvoriť účet]].',
'nocreate-loggedin'                => 'Nemáte povolenie vytvárať nové stránky.',
'permissionserrors'                => 'Chyba povolení',
'permissionserrorstext'            => 'Na to nemáte povolenie z {{PLURAL:$1|nasledujúceho dôvodu|nasledujúcich dôvodov}}:',
'permissionserrorstext-withaction' => 'Nemáte oprávnenie $2 z {{PLURAL:$1|nasledovného dôvodu|nasledovných dôvodov}}:',
'recreate-deleted-warn'            => "'''Upozornenie: Opätovne vytvárate stránku, ktorá bola predtým zmazaná.'''

Mali by ste zvážiť, či je vhodné pokračovať v úpravách tejto stránky.
Odkaz na záznam zmazaní:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Upozornenie: Táto stránka obsahuje príliš mnoho volaní funkcií syntaktického analyzátora, ktoré nadmerne zaťažujú server.

Je ich $1. Malo by ich byť menej ako $2.',
'expensive-parserfunction-category'       => 'Stránky s príliš veľkým počtom volaní funkcií syntaktického analyzátora',
'post-expand-template-inclusion-warning'  => 'Upozornenie: Vkladaná šablóna je príliš veľká.
Niektoré zo šablón nebudú vložené.',
'post-expand-template-inclusion-category' => 'Stránky, kde je prekročená povolená veľkosť šablón',
'post-expand-template-argument-warning'   => 'Upozornenie: Táto stránka obsahuje aspoň jeden argument šalóny, ktorý je príliš veľký.
Tieto argumenty boli vynechané.',
'post-expand-template-argument-category'  => 'Stránky obsahujúce vynechané argumenty šablón',

# "Undo" feature
'undo-success' => 'Úpravu nie je možné vrátiť. Prosím skontrolujte tento rozdiel, čím overíte, že táto úprava je tá, ktorú chcete, a následne uložte zmeny, čím ukončíte vrátenie.',
'undo-failure' => 'Úpravu nie je možné vrátiť kvôli konfliktným medziľahlým úpravám.',
'undo-norev'   => 'Túto úpravu nie je možné vrátiť, pretože neexistuje alebo bola zmazaná.',
'undo-summary' => 'Revízia $1 používateľa [[Special:Contributions/$2|$2]] ([[User talk:$2|diskusia]]) bola vrátená',

# Account creation failure
'cantcreateaccounttitle' => 'Nie je možné vytvoriť účet',
'cantcreateaccount-text' => "Tvorbu účtov z tejto IP adresy ('''$1''') zablokoval [[User:$3|$3]].

Dôvod, ktorý $3 uviedol, je ''$2''",

# History pages
'viewpagelogs'        => 'Zobraziť záznamy pre túto stránku',
'nohistory'           => 'Pre túto stránku neexistuje história.',
'revnotfound'         => 'Predošlá verzia nebola nájdená',
'revnotfoundtext'     => 'Požadovaná staršia verzia stránky nebola nájdená.
Prosím skontrolujte URL adresu, ktorú ste použili na prístup k tejto stránke.',
'currentrev'          => 'Aktuálna verzia',
'revisionasof'        => 'Verzia zo dňa a času $1',
'revision-info'       => 'Revízia z $1; $2',
'previousrevision'    => '← Staršia verzia',
'nextrevision'        => 'Novšia verzia →',
'currentrevisionlink' => 'Aktuálna úprava',
'cur'                 => 'aktuálna',
'next'                => 'ďalšia',
'last'                => 'posledná',
'page_first'          => 'prvá',
'page_last'           => 'posledná',
'histlegend'          => 'Porovnanie zmien: označte výberové políčka revízií, ktoré sa majú porovnať a kliknite na tlačidlo dolu.<br />
Legenda: (aktuálna) = rozdiel oproti aktuálnej verzii,
(posledná) = rozdiel oproti predchádzajúcej verzii, D = drobná úprava',
'deletedrev'          => '[zmazané]',
'histfirst'           => 'najskoršie',
'histlast'            => 'posledné',
'historysize'         => '(({{PLURAL:$1|jeden bajt|$1 bajty|$1 bajtov}}))',
'historyempty'        => '(prázdne)',

# Revision feed
'history-feed-title'          => 'História úprav',
'history-feed-description'    => 'História úprav pre túto stránku na wiki',
'history-feed-item-nocomment' => '$1 na $2', # user at time
'history-feed-empty'          => 'Požadovaná stránka neexistuje.
Možno bola zmazaná z wiki alebo premenovaná.
Skúste [[Special:Search|vyhľadávať na wiki]] relevantné nové stránky.',

# Revision deletion
'rev-deleted-comment'         => '(komentár odstránený)',
'rev-deleted-user'            => '(používateľské meno odstránené)',
'rev-deleted-event'           => '(činnosť odstránená zo záznamu)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Táto revízia stránky bola odstránená z verejných archívov.
Podrobnosti nájdete v [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} zázname mazaní].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
Táto revízia stránky bola odstránená z verejných archívov.
Ako správca tohto projektu si ju môžete prezrieť;
podrobnosti môžu byť v [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} zázname mazaní].
</div>',
'rev-delundel'                => 'zobraziť/skryť',
'revisiondelete'              => 'Zmazať/obnoviť revízie',
'revdelete-nooldid-title'     => 'Neplatná cieľová revízia',
'revdelete-nooldid-text'      => 'Neuviedli ste cieľovú revíziu alebo revízie, na ktorých sa má táto funkcia vykonať, uvedená revízia neexistuje alebo sa pokúšate skryť aktuálnu revíziu.',
'revdelete-selected'          => '{{PLURAL:$2|Vybraná jedna revízia|Vybrané $2 revízie|Vybraných $2 revízií}} z [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|Vybraná udalosť záznamu|Vybrané udalosti záznamu}}:',
'revdelete-text'              => 'Zmazané revízie sú stále viditeľné v histórii úprav stránky,
ale ich obsah nebude prístupný verejnosti.

Iní správcovia {{GRAMMAR:genitív|{{SITENAME}}}} budú stále môcť pristupovať k skrytému obsahu a môžu
ho znova obnoviť použitím tohto rozhrania v prípade, že operátormi projektu
nie sú stanovené ďalšie obmedzenia.',
'revdelete-legend'            => 'Nastaviť obmedzenia viditeľnosti',
'revdelete-hide-text'         => 'Skryť text revízie',
'revdelete-hide-name'         => 'Skryť činnosť a cieľ',
'revdelete-hide-comment'      => 'Skryť zhrnutie úprav',
'revdelete-hide-user'         => 'Skryť používateľské meno/IP',
'revdelete-hide-restricted'   => 'Použiť tieto obmedzenia na správcov a zamknúť toto rozhranie',
'revdelete-suppress'          => 'Skryť údaje pred správcami rovnako ako pred ostatnými',
'revdelete-hide-image'        => 'Skryť obsah súboru',
'revdelete-unsuppress'        => 'Odstrániť obmedzenia obnovených revízií',
'revdelete-log'               => 'Komentár záznamu:',
'revdelete-submit'            => 'Použiť na zvolenú revíziu',
'revdelete-logentry'          => 'viditeľnosť revízie bola zmenená pre [[$1]]',
'logdelete-logentry'          => 'viditeľnosť udalosti [[$1]] bola zmenená',
'revdelete-success'           => "'''Viditeľnosť revízie bola úspešne nastavená.'''",
'logdelete-success'           => "'''Viditeľnosť záznamu bola úspešne nastavená.'''",
'revdel-restore'              => 'Zmeniť viditeľnosť',
'pagehist'                    => 'História stránky',
'deletedhist'                 => 'Zmazaná história',
'revdelete-content'           => 'obsah',
'revdelete-summary'           => 'zhrnutie úprav',
'revdelete-uname'             => 'používateľské meno',
'revdelete-restricted'        => 'obmedzenia správcov použité',
'revdelete-unrestricted'      => 'obmedzenia správcov odstránené',
'revdelete-hid'               => 'skryl $1',
'revdelete-unhid'             => 'odkryl $1',
'revdelete-log-message'       => '$1 $2 {{PLURAL:$2|revíziu|revízie|revízií}}',
'logdelete-log-message'       => '$1 $2 {{PLURAL:$2|udalosť|udalosti|udalostí}}',

# Suppression log
'suppressionlog'     => 'Záznam potlačení',
'suppressionlogtext' => 'Toto je zoznam zmazaní a blokovaní vrátane obsahu, ktorý je skrytý pred správcami. Zoznam momentálne účinných blokovaní nájdete na [[Special:IPBlockList|Zoznam blokovaní IP]].',

# History merging
'mergehistory'                     => 'Zlúčiť histórie stránok',
'mergehistory-header'              => 'Táto stránka vám umožňuje zlúčiť histórie revízií zo zdrojovej stránky do novšej stránky.
Uistite sa, že táto zmena zachová historickú kontinuitu zmien stránky.',
'mergehistory-box'                 => 'Zlúčiť revízie dvoch stránok:',
'mergehistory-from'                => 'Zdrojová stránka:',
'mergehistory-into'                => 'Cieľová stránka:',
'mergehistory-list'                => 'Zlúčiteľná história úprav',
'mergehistory-merge'               => 'Nasledovné revízie [[:$1]] je možné zlúčiť do [[:$2]]. Použite stĺpec s výberovými políčkami ak chcete zlúčiť iba revízie vytvorené pred určitým časom (vrátane). Majte na pamäti, že použitím navigačných odkazov zrušíte zmeny vykonané v tomto stĺpci.',
'mergehistory-go'                  => 'Zobraziť zlúčiteľné úpravy',
'mergehistory-submit'              => 'Zlúčiť revízie',
'mergehistory-empty'               => 'Žiadne revízie nie je možné zlúčiť.',
'mergehistory-success'             => '$3 {{PLURAL:$3|revízia|revízie|revízií}} z [[:$1]] {{PLURAL:$3|bola úspešne zlúčená|boli úspešne zlúčené|bolo úspešne zlúčených}} do [[:$2]].',
'mergehistory-fail'                => 'Nepodarilo sa vykonať zlúčenie histórií. Prosím, skontrolujte parametre stránka a časy.',
'mergehistory-no-source'           => 'Zdrojová stránka $1 neexistuje.',
'mergehistory-no-destination'      => 'Cieľová stránka $1 neexistuje.',
'mergehistory-invalid-source'      => 'Zdrojová stránka musí byť platný názov.',
'mergehistory-invalid-destination' => 'Cieľová stránka musí byť platný názov.',
'mergehistory-autocomment'         => '[[:$1]] bola zlúčená do [[:$2]]',
'mergehistory-comment'             => '[[:$1]] bola zlúčená do [[:$2]]: $3',

# Merge log
'mergelog'           => 'Záznam zlúčení',
'pagemerge-logentry' => 'zlúčil [[$1]] do [[$2]] (revízie do $3)',
'revertmerge'        => 'Vrátiť zlúčenie',
'mergelogpagetext'   => 'Dolu je zoznam posledných zlúčení jednej histórie revízií stránky do druhej.',

# Diffs
'history-title'           => 'História revízií „$1“',
'difference'              => '(Rozdiel medzi revíziami)',
'lineno'                  => 'Riadok $1:',
'compareselectedversions' => 'Porovnať označené verzie',
'editundo'                => 'vrátiť',
'diff-multi'              => '{{PLURAL:$1|Jedna medziľahlá revízia nie je zobrazená|$1 medziľahlé revízie nie sú zobrazené|$1 medziľahlých revízií nie je zobrazených}}.',

# Search results
'searchresults'             => 'Výsledky vyhľadávania',
'searchresulttext'          => 'Viac informácií o vyhľadávaní vo {{GRAMMAR:lokál|{{SITENAME}}}} nájdete na stránke [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => "Hľadali ste „'''[[:$1]]'''“ ([[Special:Prefixindex/$1|všetky stránky začínajúce na „$1”]] | [[Special:WhatLinksHere/$1|všetky stránky odkazujúce na „$1”]])",
'searchsubtitleinvalid'     => 'Na vyhľadávaciu požiadavku „$1“',
'noexactmatch'              => "'''Neexistuje stránka nazvaná „$1“'''. Chcete '''[[:$1|vytvoriť novú stránku]]''' s týmto názvom?",
'noexactmatch-nocreate'     => "'''Stránka s názvom „$1“ neexistuje.'''",
'toomanymatches'            => 'Hľadanie vrátilo príliš veľa výsledkov. Prosím, skúste inú požiadavku.',
'titlematches'              => 'Vyhovujúce názvy stránok',
'notitlematches'            => 'V názvoch stránok nebola nájdená zhoda',
'textmatches'               => 'Zhody v textoch stránok',
'notextmatches'             => 'V textoch stránok nebola nájdená zhoda',
'prevn'                     => 'predošlých $1',
'nextn'                     => 'ďalších $1',
'viewprevnext'              => 'Zobraziť ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 slovo|$2 slová|$2 slov}})',
'search-result-score'       => 'Relevancia: $1%',
'search-redirect'           => '(presmerovanie $1)',
'search-section'            => '(sekcia $1)',
'search-suggest'            => 'Mali ste na mysli „$1“?',
'search-interwiki-caption'  => 'Sesterské projekty',
'search-interwiki-default'  => '$1 výsledkov:',
'search-interwiki-more'     => '(viac)',
'search-mwsuggest-enabled'  => 's návrhmi',
'search-mwsuggest-disabled' => 'bez návrhov',
'search-relatedarticle'     => 'Súvisiace',
'mwsuggest-disable'         => 'Vypnúť AJAX návrhy',
'searchrelated'             => 'súvisiace',
'searchall'                 => 'všetko',
'showingresults'            => "Nižšie {{PLURAL:$1|je zobrazený jeden výsledok|sú zobrazené '''1''' výsledky|je zobrazených '''$1''' výsledkov}}, počnúc od  #<b>$2</b>.",
'showingresultsnum'         => "Nižšie {{PLURAL:$3|je zobrazený najviac '''1''' výsledok|sú zobrazené najviac '''$3''' výsledky|je zobrazených najviac '''$3''' výsledkov}}, počnúc od  #'''$2'''.",
'showingresultstotal'       => "{{PLURAL:$3|Zobrazuje sa výsledok '''$1''' z '''$3'''|Zobrazujú sa výsledky '''$1 - $2''' z '''$3'''}}",
'nonefound'                 => "'''Poznámka''': Štandardne sa vyhľadáva iba v niektorých menných priestoroch. Skúste pred vyhľadávaný reťazec napísať ''all:'', aby sa hľadalo v celom obsahu (vrátane diskusných stránok, šablón atď.) alebo použite ako predponu požadovaný menný priestor.",
'powersearch'               => 'Vyhľadávanie',
'powersearch-legend'        => 'Pokročilé hľadanie',
'powersearch-ns'            => 'Hľadať v menných priestoroch:',
'powersearch-redir'         => 'Vypísať presmerovania',
'powersearch-field'         => 'Hľadať',
'search-external'           => 'Externé vyhľadávanie',
'searchdisabled'            => 'Prepáčte! Fulltextové vyhľadávanie bolo dočasne vypnuté z dôvodu preťaženia. Zatiaľ môžete použiť hľadanie pomocou Google, ktoré však nemusí byť aktuálne.',

# Preferences page
'preferences'              => 'Nastavenia',
'mypreferences'            => 'nastavenia',
'prefs-edits'              => 'Počet úprav:',
'prefsnologin'             => 'Nie ste prihlásený/á',
'prefsnologintext'         => 'Aby ste mohli zmeniť svoje nastavenia, musíte byť <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} prihlásený].',
'prefsreset'               => 'Boli obnovené pôvodné nastavenia.',
'qbsettings'               => 'Navigačný panel',
'qbsettings-none'          => 'Žiadne',
'qbsettings-fixedleft'     => 'Ukotvené vľavo',
'qbsettings-fixedright'    => 'Ukotvené vpravo',
'qbsettings-floatingleft'  => 'Plávajúce vľavo',
'qbsettings-floatingright' => 'Plávajúce vpravo',
'changepassword'           => 'Zmeniť heslo',
'skin'                     => 'Vzhľad',
'math'                     => 'Vykreslenie matematiky',
'dateformat'               => 'Formát dátumu',
'datedefault'              => 'štandardný',
'datetime'                 => 'Dátum a čas',
'math_failure'             => 'Syntaktická analýza (parsing) neúspešná',
'math_unknown_error'       => 'neznáma chyba',
'math_unknown_function'    => 'neznáma funkcia',
'math_lexing_error'        => 'lexikálna chyba',
'math_syntax_error'        => 'syntaktická chyba',
'math_image_error'         => 'PNG konverzia neúspešná; skontrolujte správnosť inštalácie programov: latex, dvips, gs a convert',
'math_bad_tmpdir'          => 'Nemožno zapisovať alebo vytvoriť dočasný matematický adresár',
'math_bad_output'          => 'Nemožno zapisovať alebo vytvoriť výstupný matematický adresár',
'math_notexvc'             => 'Chýbajúci program texvc; konfigurácia je popísaná v math/README.',
'prefs-personal'           => 'Profil',
'prefs-rc'                 => 'Posledné úpravy',
'prefs-watchlist'          => 'Sledované stránky',
'prefs-watchlist-days'     => 'Koľko dní zobrazovať v sledovaných stránkach:',
'prefs-watchlist-edits'    => 'Počet úprav, ktorý sa zobrazí v rozšírenom zozname sledovaných:',
'prefs-misc'               => 'Rôzne',
'saveprefs'                => 'Uložiť nastavenia',
'resetprefs'               => 'Obnoviť pôvodné nastavenia',
'oldpassword'              => 'Staré heslo:',
'newpassword'              => 'Nové heslo:',
'retypenew'                => 'Nové heslo (ešte raz):',
'textboxsize'              => 'Úpravy',
'rows'                     => 'Riadky:',
'columns'                  => 'Stĺpce:',
'searchresultshead'        => 'Vyhľadávanie',
'resultsperpage'           => 'Počet vyhovujúcich výsledkov zobrazených na stránku:',
'contextlines'             => 'Počet zobrazených riadkov z každej nájdenej stránky:',
'contextchars'             => 'Počet kontextových znakov v riadku',
'stub-threshold'           => 'Prah formátovania <a href="#" class="stub">výhonkov</a> (v bajtoch):',
'recentchangesdays'        => 'Koľko dní zobrazovať v posledných úpravách:',
'recentchangescount'       => 'Počet nadpisov uvedených v posledných úpravách',
'savedprefs'               => 'Vaše nastavenia boli uložené.',
'timezonelegend'           => 'Časové pásmo',
'timezonetext'             => '¹Počet hodín, o ktorý sa váš miestny čas odlišuje od času na serveri (UTC).',
'localtime'                => 'Miestny čas',
'timezoneoffset'           => 'Rozdiel¹',
'servertime'               => 'Aktuálny čas na serveri',
'guesstimezone'            => 'Prevziať z prehliadača',
'allowemail'               => 'Povoliť prijímanie e-mailov od iných používateľov',
'prefs-searchoptions'      => 'Voľby hľadania',
'prefs-namespaces'         => 'Menné priestory',
'defaultns'                => 'Štandardne vyhľadávať v týchto menných priestoroch:',
'default'                  => 'predvolený',
'files'                    => 'Súbory',

# User rights
'userrights'                  => 'Spravovanie používateľských práv', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Spravovať skupiny používateľov',
'userrights-user-editname'    => 'Zadajte meno používateľa:',
'editusergroup'               => 'Upraviť skupinu používateľa',
'editinguser'                 => "Zmena oprávnení používateľa '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Upraviť skupiny používateľa',
'saveusergroups'              => 'Uložiť skupiny používateľa',
'userrights-groupsmember'     => 'Člen skupiny:',
'userrights-groups-help'      => 'Môžete zmeniť, v ktorých skupinách sa používateľ nachádza.
* Zaškrtnuté pole znamená, že používateľ je v skupine.
* Nezaškrtnuté pole znamená, že používateľ nie je v skupine.
* „*“ znamená, že nemôžete odstrániť skupinu, keď ste ju už pridali resp. naopak.',
'userrights-reason'           => 'Dôvod zmeny:',
'userrights-no-interwiki'     => 'Nemáte oprávnenie upravovať práva používateľov na iných wiki.',
'userrights-nodatabase'       => 'Databáza $1 neexistuje alebo nie je lokálna.',
'userrights-nologin'          => 'Aby ste mohli prideľovať používateľom oprávnenia, musíte sa [[Special:UserLogin|prihlásiť]] s účtom správcu.',
'userrights-notallowed'       => 'Váš účet nemá oprávnenia prideľovať používateľom oprávnenia.',
'userrights-changeable-col'   => 'Skupiny, ktoré môžete zmeniť',
'userrights-unchangeable-col' => 'Skupiny, ktoré nemôžete zmeniť',

# Groups
'group'               => 'Skupina:',
'group-user'          => 'Používatelia',
'group-autoconfirmed' => 'zaregistrovaní používatelia',
'group-bot'           => 'Boti',
'group-sysop'         => 'Správcovia',
'group-bureaucrat'    => 'Byrokrati',
'group-suppress'      => 'Dozorcovia',
'group-all'           => '(všetci)',

'group-user-member'          => 'Používateľ',
'group-autoconfirmed-member' => 'zaregistrovaný používateľ',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Správca',
'group-bureaucrat-member'    => 'Byrokrat',
'group-suppress-member'      => 'Dozorca',

'grouppage-user'          => '{{ns:project}}:Používatelia',
'grouppage-autoconfirmed' => '{{ns:project}}:Zaregistrovaní používatelia',
'grouppage-bot'           => '{{ns:project}}:Boti',
'grouppage-sysop'         => '{{ns:project}}:Správcovia',
'grouppage-bureaucrat'    => '{{ns:project}}:Byrokrati',
'grouppage-suppress'      => '{{ns:project}}:Dozor',

# Rights
'right-read'                 => 'Čítať stránky',
'right-edit'                 => 'Upravovať stránky (ktoré nie sú diskusné stránky)',
'right-createpage'           => 'Vytvárať stránky (ktoré nie sú diskusné stránky)',
'right-createtalk'           => 'Vytvárať diskusné stránky',
'right-createaccount'        => 'Vytvárať nové používateľské účty',
'right-minoredit'            => 'Označovať úpravy ako drobné',
'right-move'                 => 'Presúvať stránky',
'right-move-subpages'        => 'Presunúť stránky aj s podstránkami',
'right-suppressredirect'     => 'Nevytvoriť presmerovanie zo starého názvu pri presúvaní stránky',
'right-upload'               => 'Nahrávať súbory',
'right-reupload'             => 'Prepísať existujúci súbor',
'right-reupload-own'         => 'Prepísať existujúci súbor, ktorý nahral rovnaký používateľ',
'right-reupload-shared'      => 'Nahrávať lokálne súbory, ktoré majú prednosť pred zdieľaným úložiskom',
'right-upload_by_url'        => 'Nahrávať súbor z URL adresy',
'right-purge'                => 'Čistiť vyrovnávaciu pamäť stránky bez potvrdzovacej stránky',
'right-autoconfirmed'        => 'Upravovať čiastočne zamknuté stránky',
'right-bot'                  => 'Byť považovaný za automatický proces',
'right-nominornewtalk'       => 'Pri drobných úpravách diskusnej stránky nevypisovať hlásenie o nových správach',
'right-apihighlimits'        => 'Používať vyššie limity v požiadavkách API',
'right-writeapi'             => 'Použitie API na zápis',
'right-delete'               => 'Mazať stránky',
'right-bigdelete'            => 'Mazať stránky s veľkou históriou',
'right-deleterevision'       => 'Mazať a obnovovať konkrétne revízie stránok',
'right-deletedhistory'       => 'Zobrazovať zmazané položky histórie bez ich plného textu',
'right-browsearchive'        => 'Hľadať v zmazaných stránkach',
'right-undelete'             => 'Obnoviť zmazanú stránku',
'right-suppressrevision'     => 'Kontrolovať a obnovovať revízie skryté správcom',
'right-suppressionlog'       => 'Zobrazovať súkromné záznamy',
'right-block'                => 'Blokovať ostatných používateľov',
'right-blockemail'           => 'Zablokovať používateľovi posielanie emailu',
'right-hideuser'             => 'Zablokovať používateľské meno tak, že bude verejnosti skryté',
'right-ipblock-exempt'       => 'Obchádzať blokovanie IP adries, rozsahov a automatické blokovanie',
'right-proxyunbannable'      => 'Obchádzať automatické blokovanie proxy serverov',
'right-protect'              => 'Meniť úroveň zamknutia a upravovať zamknuté stránky',
'right-editprotected'        => 'Upravovať zamknuté schránky (bez kaskádovej ochrany)',
'right-editinterface'        => 'Upravovať správy používateľského rozhrania',
'right-editusercssjs'        => 'Upravovať CSS a JS súbory ostatných používateľov',
'right-rollback'             => 'Rýchlo vrátiť úpravy posledného používateľa, ktorý upravoval danú stránku',
'right-markbotedits'         => 'Označiť vrátené úpravy ako úpravy robota',
'right-noratelimit'          => 'Neovplyvnené obmedzeniami',
'right-import'               => 'Importovať stránky z iných wiki',
'right-importupload'         => 'Importovať stránky nahraním súboru',
'right-patrol'               => 'Označiť úpravy ako strážené',
'right-autopatrol'           => 'Nechať úpravy automaticky označiť ako strážené',
'right-patrolmarks'          => 'Používať možnosti stráženia posledných úprav',
'right-unwatchedpages'       => 'Zobraziť zoznam nesledovaných stránok',
'right-trackback'            => 'Poslať trackback',
'right-mergehistory'         => 'Zlučovať histórie stránok',
'right-userrights'           => 'Upravovať oprávnenia ostatných používateľov',
'right-userrights-interwiki' => 'Upravovať oprávnenia používateľov na iných wiki stránkach',
'right-siteadmin'            => 'Zamykať a odomykať databázu',

# User rights log
'rightslog'      => 'Záznam používateľských práv',
'rightslogtext'  => 'Toto je záznam zmien práv používateľa.',
'rightslogentry' => 'členstvo v skupine zmenené pre $1 z $2 na $3',
'rightsnone'     => '(žiadne)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|zmena|zmeny|zmien}}',
'recentchanges'                     => 'Posledné úpravy',
'recentchangestext'                 => 'Pomocou tejto stránky sledujete posledné úpravy stránok {{GRAMMAR:genitív|{{SITENAME}}}}.',
'recentchanges-feed-description'    => 'Sledovať posledné úpravy tejto wiki týmto kanálom.',
'rcnote'                            => "Tu {{PLURAL:$1|je posledná úprava|sú posledné '''$1''' úpravy|je posledných '''$1''' úprav}} počas {{PLURAL:$2|posledného dňa|posledných '''$2''' dní}} z $4, $5.",
'rcnotefrom'                        => "Nižšie sú zobrazené úpravy od '''$2''' (do '''$1''').",
'rclistfrom'                        => 'Zobraziť nové úpravy počnúc od $1',
'rcshowhideminor'                   => '$1 drobné úpravy',
'rcshowhidebots'                    => '$1 botov',
'rcshowhideliu'                     => '$1 prihlásených používateľov',
'rcshowhideanons'                   => '$1 anonymných používateľov',
'rcshowhidepatr'                    => '$1 úpravy strážených stránok',
'rcshowhidemine'                    => '$1 moje úpravy',
'rclinks'                           => 'Zobraziť posledných $1 úprav v posledných $2 dňoch<br />$3',
'diff'                              => 'rozdiel',
'hist'                              => 'história',
'hide'                              => 'skryť',
'show'                              => 'zobraziť',
'minoreditletter'                   => 'D',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|sledujúci používateľ|sledujúci používatelia|sledujúcich používateľov}}]',
'rc_categories'                     => 'Obmedziť na kategórie (oddeľte znakom „|“)',
'rc_categories_any'                 => 'akékoľvek',
'newsectionsummary'                 => '/* $1 */ nová sekcia',

# Recent changes linked
'recentchangeslinked'          => 'Súvisiace úpravy',
'recentchangeslinked-title'    => 'Zmeny týkajúce sa „$1”',
'recentchangeslinked-noresult' => 'V zadanom období neboli odkazované stránky zmenené.',
'recentchangeslinked-summary'  => "Táto špeciálna stránka obsahuje zoznam posledných úprav stránok, ''na ktoré odkazuje'' zadaná stránka (alebo ktoré patria do zadanej kategórie).
Stránky, ktoré sa nachádzajú vo vašom [[Special:Watchlist|zozname sledovaných]] sú vyznačené '''hrubo'''.",
'recentchangeslinked-page'     => 'Názov stránky:',
'recentchangeslinked-to'       => "Zobraziť zmeny na stránkach, ''ktoré odkazujú na'' zadanú stránku",

# Upload
'upload'                      => 'Nahrať súbor',
'uploadbtn'                   => 'Nahrať súbor',
'reupload'                    => 'Zopakovať nahranie',
'reuploaddesc'                => 'Zrušiť nahrávanie a vrátiť sa späť na nahrávací formulár.',
'uploadnologin'               => 'Nie ste prihlásený',
'uploadnologintext'           => 'Musíte byť [[Special:UserLogin|prihlásený/á]], aby ste mohli nahrávať súbory.',
'upload_directory_missing'    => 'Adresár pre nahrávanie ($1) chýba a webový server ho nedokáže vytvoriť.',
'upload_directory_read_only'  => 'Webový server nemôže zapisovať do adresára pre nahrávanie ($1).',
'uploaderror'                 => 'Chyba pri nahrávaní',
'uploadtext'                  => "Tento formulár použite na nahrávanie súborov.
Už nahrané súbory môžete zobraziť alebo hľadať v [[Special:ImageList|zozname nahraných súborov]]. Nahrávania a mazania sa tiež zaznamenávajú v [[Special:Log/upload|zázname nahrávaní]].

Na začlenenie obrázka do stránky použite odkaz v tvare

* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Súbor.jpg]]</nowiki>''' - použije sa plná verzia obrázka
* '''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:Súbor.png|alternatívny text]]</nowiki>''' - vykreslí sa v šírke 200 pixelov s okrajom, na ľavom okraji, s popisom v „alt” texte.
* '''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:Súbor.ogg]]</nowiki>''' - priamy odkaz na súbor (nezobrazí obrázok na stránke)",
'upload-permitted'            => 'Povolené typy súborov: $1.',
'upload-preferred'            => 'Uprednostňované typy súborov: $1.',
'upload-prohibited'           => 'Zakázané typy súborov: $1.',
'uploadlog'                   => 'Záznam nahrávaní',
'uploadlogpage'               => 'Záznam nahrávaní',
'uploadlogpagetext'           => 'Nižšie je zoznam nedávno nahraných súborov.
Vizuálny prehľad nájdete v [[Special:NewImages|galérii novo nahraných súborov]].',
'filename'                    => 'Názov súboru',
'filedesc'                    => 'Opis súboru',
'fileuploadsummary'           => 'Zhrnutie:',
'filestatus'                  => 'Stav autorských práv:',
'filesource'                  => 'Zdroj:',
'uploadedfiles'               => 'Nahrané súbory',
'ignorewarning'               => 'Ignorovať varovanie a súbor napriek tomu uložiť',
'ignorewarnings'              => 'Ignorovať všetky varovania',
'minlength1'                  => 'Názvy súborov musia mať aspoň jedno písmeno.',
'illegalfilename'             => 'Názov súboru „$1“ obsahuje znaky, ktoré nie sú povolené v názvoch stránok. Prosím premenujte súbor a skúste ho nahrať znovu.',
'badfilename'                 => 'Názov obrázka bol zmenený na „$1“.',
'filetype-badmime'            => 'Nie je povolené nahrávať súbory s MIME typom „$1“.',
'filetype-unwanted-type'      => "„.$1“''' je nežiadúci typ súboru.
{{PLURAL:$3|Uprednostňovaný typ súborov je|Uprednostňované typy súborov sú}} $2.",
'filetype-banned-type'        => "„.$1“''' nie je povolený typ súboru.
{{PLURAL:$3|Povolený typ súborov je|Povolené typy súborov sú}} $2.",
'filetype-missing'            => 'Súbor nemá príponu (ako „.jpg“).',
'large-file'                  => 'Odporúča sa aby veľkosť súborov neprekračovala $1; tento súbor má $2.',
'largefileserver'             => 'Tento súbor je väčší ako je možné nahrať na server (z dôvodu obmedzenia veľkosti súboru v konfigurácii servera).',
'emptyfile'                   => 'Zdá sa, že súbor, ktorý ste nahrali je prázdny. Mohlo sa stať, že ste urobili v názve súboru preklep. Prosím, skontrolujte, či skutočne chcete nahrať tento súbor.',
'fileexists'                  => 'Súbor s týmto názvom už existuje, prosím skontrolujte <strong><tt>$1</tt></strong> ak nie ste si istý, či ho chcete zmeniť.',
'filepageexists'              => 'Popisná stránka tohto súboru už bola vytvorená na <strong><tt>$1</tt></strong>, ale žiadny súbor s týmto názvom momentálne neexistuje. Zhrnutie, ktoré zadáte sa neobjaví na popisnej stránke. Aby sa zhrnutie objavilo na popisnej stránke, budete ho musieť ručne upraviť.',
'fileexists-extension'        => 'Súbor s podobným názvom už existuje:<br />
Názov súboru, ktoý nahrávate: <strong><tt>$1</tt></strong><br />
Názov existujúceho súboru: <strong><tt>$2</tt></strong><br />
Jediný rozdiel je vo veľkosti písmen prípony. Prosím, skontrolujte totožnosť týchto súborov.',
'fileexists-thumb'            => "<center>'''Existujúci obrázok'''</center>",
'fileexists-thumbnail-yes'    => 'Zdá sa, že súbor je obrázkom redukovanej veľkosti <i>(náhľadom)</i>. Prosím, skontolujte súbor <strong><tt>$1</tt></strong>.<br />
Ak je kontrolovaný súbor rovnaký obrázok v pôvodnej veľkosti, nie je potrebné nahrávať ďalší náhľad.',
'file-thumbnail-no'           => 'Názov súboru začína <strong><tt>$1</tt></strong>.
Zdá sa, že je to obrázok redukovanej veľkosti <i>(náhľad)</i>.
Ak máte tento obrázok v plnom rozlíšení, nahrajte ho, inak prosím zmeňte názov.',
'fileexists-forbidden'        => 'Súbor s týmto názvom už existuje; choďte prosím späť a nahrajte tento súbor pod iným názvom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Súbor s týmto názvom už existuje v zdieľanom úložisku súborov.
Ak ho chcete aj napriek tomu nahrať, choďte prosím späť a použite iný názov. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Tento súbor je duplikátom {{PLURAL:$1|nasledovného súboru|nasledovných súborov}}:',
'successfulupload'            => 'Nahranie bolo úspešné',
'uploadwarning'               => 'Varovanie pri nahrávaní',
'savefile'                    => 'Uložiť súbor',
'uploadedimage'               => 'nahraný „[[$1]]“',
'overwroteimage'              => 'bola nahraná nová verzia „[[$1]]“',
'uploaddisabled'              => 'Prepáčte, nahrávanie je vypnuté.',
'uploaddisabledtext'          => 'Nahrávanie súborov je vypnuté.',
'uploadscripted'              => 'Tento súbor obsahuje kód HTML alebo skript, ktorý može byť chybne interpretovaný prehliadačom.',
'uploadcorrupt'               => 'Tento súbor je závadný alebo má nesprávnu príponu. Skontrolujte súbor a nahrajte ho znova.',
'uploadvirus'                 => 'Súbor obsahuje vírus! Podrobnosti: $1',
'sourcefilename'              => 'Názov zdrojového súboru:',
'destfilename'                => 'Názov cieľového súboru:',
'upload-maxfilesize'          => 'Maximálna veľkosť súboru: $1',
'watchthisupload'             => 'Sleduj túto stránku',
'filewasdeleted'              => 'Súbor s týmto názvom bol už nahraný a následne zmazaný. Mali by ste skontrolovať $1 predtým, ako budete pokračovať na opätovné nahranie.',
'upload-wasdeleted'           => "'''Upozornenie: Nahrávate súbor, ktorý bol predtým zmazaný.'''

Mali by ste zvážiť, či je vhodné pokračovať v nahrávaní tohto súboru.
Tu je na záznam zmazaní tohto súboru:",
'filename-bad-prefix'         => 'Názov súboru, ktorý nahrávate, začína <strong>„$1“</strong>, čo nie je popisné meno. Takýto názov typicky priraďujú digitálne fotoaparáty automaticky. Prosím, dajte vášmu súboru popisnejší názov.',
'filename-prefix-blacklist'   => ' #<!-- leave this line exactly as it is --> <pre>
# Syntax je nasledovná:
#   * Všetko od znaku „#“ po koniec riadka je komentár
#   * Každý neprázdny riadok je prefix typických názvov súborov, ktoré automaticky priraďuje digitálny fotoapraát
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # niektoré mobilné telefóny
IMG # všeobecné
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- leave this line exactly as it is -->',

'upload-proto-error'      => 'Nesprávny protokol',
'upload-proto-error-text' => 'Vzdialené nahrávanie vyžaduje, aby URL začínali <code>http://</code> alebo <code>ftp://</code>.',
'upload-file-error'       => 'Vnútorná chyba',
'upload-file-error-text'  => 'Vyskytla sa vnútorná chyba pri pokuse vytvoriť dočasný súbor na serveri.
Prosím, kontaktujte [[Special:ListUsers/sysop|správcu systému]].',
'upload-misc-error'       => 'Neznáma chyba pri nahrávaní',
'upload-misc-error-text'  => 'Počas nahrávania sa vyskytla neznáma chyba.
Prosím, overte, že URL je platný a dostupný a skúste znova.
Ak problém pretrváva, kontaktujte [[Special:ListUsers/sysop|správcu systému]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Nedostupný URL',
'upload-curl-error6-text'  => 'Poskytnutý URL nebol dostupný. Prosím, skontrolujte znova, že URL je správny a lokalita je dostupná.',
'upload-curl-error28'      => 'Vypršal čas vyhradený pre nahrávanie',
'upload-curl-error28-text' => 'Lokalite trvala odpoveď príliš dlho. Prosím, skontrolujte, či je lokalita dopstupná, chvíľu počkajte a skúste znova. Možno je potrebné skúsiť nahrávanie v čase, kedy je lokalita menej zaťažená.',

'license'            => 'Licencovanie:',
'nolicense'          => 'Nič nebolo vybrané',
'license-nopreview'  => '(Náhľad nie je dostupný)',
'upload_source_url'  => ' (platný, verejne prístupný URL)',
'upload_source_file' => ' (súbor na vašom počítači)',

# Special:ImageList
'imagelist-summary'     => 'Táto špeciálna stránka zobrazuje všetky nahrané súbory.
Posledné nahrané súbory sa štandardne zobrazujú na vrchu.
Kliknutím na hlavičku stĺpca zmeníte poradie triedenia.',
'imagelist_search_for'  => 'Hľadať názov súboru:',
'imgfile'               => 'súbor',
'imagelist'             => 'Zoznam obrázkov',
'imagelist_date'        => 'Dátum',
'imagelist_name'        => 'Názov',
'imagelist_user'        => 'Používateľ',
'imagelist_size'        => 'Veľkosť (v bajtoch)',
'imagelist_description' => 'Popis',

# Image description page
'filehist'                       => 'História súboru',
'filehist-help'                  => 'Po kliknutí na dátum/čas uvidíte súbor ako vyzeral vtedy.',
'filehist-deleteall'             => 'zmazať všetky',
'filehist-deleteone'             => 'zmazať',
'filehist-revert'                => 'obnoviť',
'filehist-current'               => 'aktuálna',
'filehist-datetime'              => 'dátum/čas',
'filehist-user'                  => 'používateľ',
'filehist-dimensions'            => 'rozmery',
'filehist-filesize'              => 'veľkosť súboru',
'filehist-comment'               => 'komentár',
'imagelinks'                     => 'Odkazy na obrázok',
'linkstoimage'                   => 'Na tento obrázok {{PLURAL:$1|odkazuje nasledujúca stránka|odkazujú nasledujúce $1 stránky|odkazuje nasledujúcich $1 stránok}}:',
'nolinkstoimage'                 => 'Žiadne stránky neobsahujú odkazy na tento obrázok.',
'morelinkstoimage'               => 'Zobraziť [[Special:WhatLinksHere/$1|ďalšie odkazy]] na tento súbor.',
'redirectstofile'                => '{{PLURAL:$1|Nasledujúci súbor presmerúva|Nasledujúce $1 súbory presmerúvajú|Nasledujúcich $1 súborov presmerúva}} na tento súbor:',
'duplicatesoffile'               => '{{PLURAL:$1|Nasledujúci súbor je duplikát|Nasledujúce $1 súbory sú duplikáty||Nasledujúcich $1 súborov sú duplikáty}} tohto súboru:',
'sharedupload'                   => 'Toto je zdieľaný súbor a je možné ho používať na iných projektoch.',
'shareduploadwiki'               => 'Ďalšie informácie pozrite na $1.',
'shareduploadwiki-desc'          => 'Dolu je zobrazený popis, ktorý obsahuje jeho $1 na zdieľanom úložisku.',
'shareduploadwiki-linktext'      => 'popisná stránka súboru',
'shareduploadduplicate'          => 'Tento súbor je duplikát $1 zo zdieľaného úložiska.',
'shareduploadduplicate-linktext' => 'iný súbor',
'shareduploadconflict'           => 'Tento súbor má rovnaký názov ako $1 zo zdieľaného úložiska.',
'shareduploadconflict-linktext'  => 'iný súbor',
'noimage'                        => 'Súbor s takým názvom neexistuje, ale môžete ho $1',
'noimage-linktext'               => 'nahrať',
'uploadnewversion-linktext'      => 'Nahrať novú verziu tohto súboru',
'imagepage-searchdupe'           => 'Hľadať duplicitné súbory',

# File reversion
'filerevert'                => 'Obnoviť $1',
'filerevert-legend'         => 'Obnoviť súbor',
'filerevert-intro'          => "Obnovujete '''[[Media:$1|$1]]''' na [$4 verziu z $2, $3].",
'filerevert-comment'        => 'komentár:',
'filerevert-defaultcomment' => 'Obnovená verzia z $1, $2',
'filerevert-submit'         => 'Obnoviť',
'filerevert-success'        => "'''[[Media:$1|$1]]''' bol obnovený na [$4 verziu z $2, $3].",
'filerevert-badversion'     => 'Neexistuje predchádzajúca lokálna verzia tohto súboru s požadovanopu časovou známkou.',

# File deletion
'filedelete'                  => 'Zmazať $1',
'filedelete-legend'           => 'Zmazať súbor',
'filedelete-intro'            => "Mažete '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Mažete verziu súboru '''[[Media:$1|$1]]''' z [$4 $3, $2].",
'filedelete-comment'          => 'Komentár:',
'filedelete-submit'           => 'Zmazať',
'filedelete-success'          => "'''$1''' bol zmazaný.",
'filedelete-success-old'      => "Verzia súboru '''[[Media:$1|$1]]''' z $3, $2 bola zmazaná.",
'filedelete-nofile'           => "'''$1''' neexistuje.",
'filedelete-nofile-old'       => "Neexistuje archivovaná verzia '''$1''' s uvedenými atribútmi.",
'filedelete-iscurrent'        => 'Pokúšate sa zmazať poslednú verziu tohto súboru. Prosím, najskôr vráťte staršiu verziu.',
'filedelete-otherreason'      => 'Iný/ďalší dôvod:',
'filedelete-reason-otherlist' => 'Iný dôvod',
'filedelete-reason-dropdown'  => '* Bežné dôvody mazania
** Porušenie autorských práv
** Duplicitný súbor',
'filedelete-edit-reasonlist'  => 'Upraviť dôvody zmazania',

# MIME search
'mimesearch'         => 'MIME vyhľadávanie',
'mimesearch-summary' => 'Táto stránka umožňuje filtovanie súborov podľa MIME typu. Vstup: typobsahu/podtyp, napr. <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME typ:',
'download'           => 'stiahnuť',

# Unwatched pages
'unwatchedpages' => 'Nesledované stránky',

# List redirects
'listredirects' => 'Zoznam presmerovaní',

# Unused templates
'unusedtemplates'     => 'Nepoužité šablóny',
'unusedtemplatestext' => 'Táto stránka obsahuje zoznam všetkých stránok v mennom prisetore Šablóna:, ktoré nie sú vložené v žiadnej inej stránke. Pred zmazaním nezabudnite skontrolovať ostatné odkazy!',
'unusedtemplateswlh'  => 'iné odkazy',

# Random page
'randompage'         => 'Náhodná stránka',
'randompage-nopages' => 'V tomto mennom priestore nie sú žiadne stránky.',

# Random redirect
'randomredirect'         => 'Náhodná presmerovacia stránka',
'randomredirect-nopages' => 'V tomto mennom priestore nie sú žiadne presmerovania.',

# Statistics
'statistics'             => 'Štatistiky',
'sitestats'              => 'Štatistika {{GRAMMAR:genitív|{{SITENAME}}}}',
'userstats'              => 'Štatistika používateľov',
'sitestatstext'          => "{{SITENAME}} momentálne má {{PLURAL:$1|jednu stránku|'''$2''' stránky|'''$2''' stránok}}.
Do toho sa nezapočítavajú presmerovania, diskusné stránky, popisné stránky obrázkov, stránky používateľských profilov, šablóny, stránky Pomocníka, portály, stránky bez odkazov na iné stránky a stránky o {{GRAMMAR:lokál|{{SITENAME}}}}.
Vrátane týchto máme spolu {{PLURAL:$1|jednu stránku|'''$2''' stránky|'''$2''' stránok}}, {{PLURAL:$2|ktorá je pravdepodobne platná stránka s obsahom|ktoré sú pravdepodobne platné stránky s obsahom}}.

Celkovo {{PLURAL:$8|bol nahraný jeden súbor|boli nahrané '''$8''' súbory|bolo nahraných '''$8''' súborov}}.

Celkovo boli stránky navštívené '''$3'''-krát a upravené '''$4'''-krát. To znamená, že pripadá priemerne '''$5''' úprav na každú stránku a '''$6''' návštev na každú úpravu.

[http://www.mediawiki.org/wiki/Manual:Job_queue Dĺžka frontu úloh] je momentálne '''$7'''.",
'userstatstext'          => "Celkovo {{PLURAL:$1|je jeden zaregistrovaný používateľ|sú '''$1''' zaregistrovaní používatelia|je '''$1''' zaregistrovaných používateľov}},
z čoho '''$2''' (alebo '''$4 %''') {{PLURAL:$2|je správca|sú správcovia}} (pozri $5).",
'statistics-mostpopular' => 'Najčastejšie prezerané stránky',

'disambiguations'      => 'Stránky na rozlíšenie viacerých významov',
'disambiguationspage'  => 'Template:Rozlišovacia stránka',
'disambiguations-text' => "Nasledovné stránky odkazujú na '''rozlišovaciu stránku'''.
Mali by však odkazovať priamo na príslušnú tému.<br />
Stránka sa považuje za rozlišovaciu, keď používa šablónu, na ktorú odkazuje [[MediaWiki:Disambiguationspage]]",

'doubleredirects'            => 'Dvojité presmerovania',
'doubleredirectstext'        => 'Každý riadok obsahuje odkaz na prvé a druhé presmerovanie a tiež prvý riadok z textu na ktorý odkazuje druhé presmerovanie, ktoré zvyčajne odkazuje na „skutočný“ cieľ, na ktorý má odkazovať prvé presmerovanie.',
'double-redirect-fixed-move' => 'Stránka [[$1]] bola presunutá, teraz je presmerovaním na [[$2]]',
'double-redirect-fixer'      => 'Korektor presmerovaní',

'brokenredirects'        => 'Pokazené presmerovania',
'brokenredirectstext'    => 'Tieto presmerovania odkazujú na neexistujúcu stránku.',
'brokenredirects-edit'   => '(upraviť)',
'brokenredirects-delete' => '(zmazať)',

'withoutinterwiki'         => 'Stránky bez jazykových odkazov',
'withoutinterwiki-summary' => 'Nasledujúce stránky neodkazujú na iné jazykové verzie:',
'withoutinterwiki-legend'  => 'Predpona',
'withoutinterwiki-submit'  => 'Zobraziť',

'fewestrevisions' => 'Stránky s najmenším počtom revízií',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajty|bajtov}}',
'ncategories'             => '$1 {{PLURAL:$1|kategória|kategórie|kategórií}}',
'nlinks'                  => '$1 {{PLURAL:$1|odkaz|odkazy|odkazov}}',
'nmembers'                => '$1 {{PLURAL:$1|člen|členovia|členov}}',
'nrevisions'              => '$1 {{PLURAL:$1|revízia|revízie|revízií}}',
'nviews'                  => '$1 {{PLURAL:$1|návšteva|návštevy|návštev}}',
'specialpage-empty'       => 'Táto správa neobsahuje žiadne položky.',
'lonelypages'             => 'Opustené stránky',
'lonelypagestext'         => 'Na nasledujúce stránky neodkazujú žiadne iné stránky z {{GRAMMAR:genitív|{{SITENAME}}}}.',
'uncategorizedpages'      => 'Stránky nezaradené do kategórií',
'uncategorizedcategories' => 'Kategórie nezaradené do kategórií',
'uncategorizedimages'     => 'Nekategorizované súbory',
'uncategorizedtemplates'  => 'Nekategorizované šablóny',
'unusedcategories'        => 'Nepoužité kategórie',
'unusedimages'            => 'Nepoužité obrázky',
'popularpages'            => 'Populárne stránky',
'wantedcategories'        => 'Žiadané kategórie',
'wantedpages'             => 'Žiadané stránky',
'missingfiles'            => 'Chýbajúce súbory',
'mostlinked'              => 'Najčastejšie odkazované stránky',
'mostlinkedcategories'    => 'Najčastejšie odkazované kategórie',
'mostlinkedtemplates'     => 'Najčastejšie odkazované šablóny',
'mostcategories'          => 'Stránky s najväčším počtom kategórií',
'mostimages'              => 'Najčastejšie odkazované súbory',
'mostrevisions'           => 'Stránky s najväčším počtom úprav',
'prefixindex'             => 'Index prefixu',
'shortpages'              => 'Krátke stránky',
'longpages'               => 'Dlhé stránky',
'deadendpages'            => 'Slepé stránky',
'deadendpagestext'        => 'Nasledujúce stránky neodkazujú na žiadne iné stránky na {{GRAMMAR:lokál|{{SITENAME}}}}.',
'protectedpages'          => 'Zamknuté stránky',
'protectedpages-indef'    => 'Zamknutia iba na neurčito',
'protectedpagestext'      => 'Upravovanie alebo presúvanie nasledovných stránok je zamknuté',
'protectedpagesempty'     => 'Momentálne nie sú žiadne stránky s týmito parametrami zamknuté.',
'protectedtitles'         => 'Zamknuté názvy',
'protectedtitlestext'     => 'Možnosť vytvoriť stránky s nasledovnými názvami bola zamknutá',
'protectedtitlesempty'    => 'Tieto parametre momentálne nezamykajú žiadne názvy stránok.',
'listusers'               => 'Zoznam používateľov',
'newpages'                => 'Nové stránky',
'newpages-username'       => 'Meno používateľa:',
'ancientpages'            => 'Najdávnejšie upravované stránky',
'move'                    => 'Presunúť',
'movethispage'            => 'Presunúť túto stránku',
'unusedimagestext'        => 'Prosím, uvedomte si, že iné web stránky môžu odkazovať na tento súbor priamo URL adresou a tak tu môžu byť uvedené napriek tomu, že ich externé stránky používajú.',
'unusedcategoriestext'    => 'Nasledujúce kategórie existujú napriek tomu, že do nich nie je zaradená žiadna stránka.',
'notargettitle'           => 'Nebol zadaný cieľ',
'notargettext'            => 'Nezadali ste cieľovú stránku alebo používateľa,
na ktorý/-ého chcete aplikovať túto funkciu.',
'nopagetitle'             => 'Taká cieľová stránka neexistuje',
'nopagetext'              => 'Cieľová stránka, ktorú ste uviedli neexistuje.',
'pager-newer-n'           => '{{PLURAL:$1|1 novší|$1 novšie|$1 novších}}',
'pager-older-n'           => '{{PLURAL:$1|1 starší|$1 staršie|$1 starších}}',
'suppress'                => 'Dozor',

# Book sources
'booksources'               => 'Knižné zdroje',
'booksources-search-legend' => 'Vyhľadávať knižné zdroje',
'booksources-go'            => 'Vykonať',
'booksources-text'          => 'Nižšie je zoznam odkazov na iné stránky, ktoré predávajú nové a použité knihy a tiež môžu obsahovať ďalšie informácie o knihách, ktoré hľadáte:',

# Special:Log
'specialloguserlabel'  => 'Používateľ:',
'speciallogtitlelabel' => 'Názov:',
'log'                  => 'Záznamy',
'all-logs-page'        => 'Všetky záznamy',
'log-search-legend'    => 'Hľadať záznamy',
'log-search-submit'    => 'Spustiť',
'alllogstext'          => 'Kombinované zobrazenie všetkých dostupných záznamov {{GRAMMAR:genitív|{{SITENAME}}}}.
Môžete zúžiť rozsah, ak zvolíte typ záznamu, používateľské meno alebo dotyčnú stránku (záleží na veľkosti písmen).',
'logempty'             => 'V zázname neboli nájdené zodpovedajúce položky.',
'log-title-wildcard'   => 'Hľadať názvy začínajúce týmto textom',

# Special:AllPages
'allpages'          => 'Všetky stránky',
'alphaindexline'    => '$1 do $2',
'nextpage'          => 'Ďalšia stránka ($1)',
'prevpage'          => 'Predchádzajúca stránka ($1)',
'allpagesfrom'      => 'Zobraziť stránky od:',
'allarticles'       => 'Všetky stránky',
'allinnamespace'    => 'Všetky stránky (menný priestor $1)',
'allnotinnamespace' => 'Všetky stránky (nie z menného priestoru $1)',
'allpagesprev'      => 'Predchádzajúci',
'allpagesnext'      => 'Ďalší',
'allpagessubmit'    => 'Vykonať',
'allpagesprefix'    => 'Zobraziť stránky s predponou:',
'allpagesbadtitle'  => 'Zadaný názov stránky je neplatný alebo mal medzijazykový alebo interwiki prefix. Môže obsahovať jeden alebo viac znakov, ktoré nie je možné použiť v názve stránky.',
'allpages-bad-ns'   => '{{SITENAME}} nemá menný priestor „$1“.',

# Special:Categories
'categories'                    => 'Kategórie',
'categoriespagetext'            => 'Nasledovné kategórie obsahujú stránky alebo multimediálne súbory.
Nie sú tu zobrazené [[Special:UnusedCategories|nepoužité kategórie]].
Pozri aj [[Special:WantedCategories|žiadané kategórie]].',
'categoriesfrom'                => 'Zobraziť kategórie počnúc:',
'special-categories-sort-count' => 'zoradiť podľa počtu',
'special-categories-sort-abc'   => 'zoradiť podľa abecedy',

# Special:ListUsers
'listusersfrom'      => 'Zobraziť používateľov počnúc:',
'listusers-submit'   => 'Zobraziť',
'listusers-noresult' => 'Neboli nájdení používatelia. Prosím, skontrolujte aj varianty s veľkými/malými písmenami.',

# Special:ListGroupRights
'listgrouprights'          => 'Práva skupiny používateľov',
'listgrouprights-summary'  => 'Toto je zoznam skupín používateľov definovaných na tejto wiki a ich prístupových práv.
Môžete si prečítať [[{{MediaWiki:Listgrouprights-helppage}}|ďalšie informácie]] o jednotlivých právach.',
'listgrouprights-group'    => 'Skupina',
'listgrouprights-rights'   => 'Práva',
'listgrouprights-helppage' => 'Help:Práva skupiny',
'listgrouprights-members'  => '(zoznam členov)',

# E-mail user
'mailnologin'     => 'Žiadna adresa na zaslanie',
'mailnologintext' => 'Musíte byť [[Special:UserLogin|prihlásený]] a mať platnú e-mailovú adresu vo vašich [[Special:Preferences|nastaveniach]], aby ste mohli iným používateľom posielať e-maily.',
'emailuser'       => 'E-mail tomuto používateľovi',
'emailpage'       => 'E-mail používateľovi',
'emailpagetext'   => 'Ak tento používateľ zadal platnú emailovú adresu vo svojich nastaveniach,
môžete mu pomocou nasledovného formulára poslať email.
Emailová adresa, ktorú ste zadali vo svojich [[Special:Preferences|nastaveniach]] sa zobrazí ako adresa odosielateľa emailu, aby vám mohol príjemca priamo odpovedať.',
'usermailererror' => 'Emailový program vrátil chybu:',
'defemailsubject' => 'email {{GRAMMAR:genitív|{{SITENAME}}}}',
'noemailtitle'    => 'Chýba e-mailová adresa',
'noemailtext'     => 'Tento používateľ nešpecifikoval platnú e-mailovú adresu
alebo sa rozhodol, že nebude prijímať e-maily od druhých používateľov.',
'emailfrom'       => 'Od:',
'emailto'         => 'Komu:',
'emailsubject'    => 'Predmet:',
'emailmessage'    => 'Správa:',
'emailsend'       => 'Odoslať',
'emailccme'       => 'Pošli mi emailom kópiu mojej správy.',
'emailccsubject'  => 'Kópia správy pre $1: $2',
'emailsent'       => 'E-mail bol odoslaný',
'emailsenttext'   => 'Vaša e-mailová správa bola odoslaná.',
'emailuserfooter' => 'Tento email poslal $1 pre $2 pomocu funkcie „Email používateľovi” na {{GRAMMAR:lokál|{{SITENAME}}}}.',

# Watchlist
'watchlist'            => 'Sledované stránky',
'mywatchlist'          => 'Sledované stránky',
'watchlistfor'         => "(používateľa '''$1''')",
'nowatchlist'          => 'V zozname sledovaných stránok nemáte žiadne položky.',
'watchlistanontext'    => 'Prosím $1 pre prezeranie alebo úpravu vášho zoznamu sledovaných stránok.',
'watchnologin'         => 'Nie ste prihlásený/á',
'watchnologintext'     => 'Musíte byť [[Special:UserLogin|prihlásený/á]], aby ste mohli modifikovať vaše sledované stránky.',
'addedwatch'           => 'Pridaná do zoznamu sledovaných stránok',
'addedwatchtext'       => "Stránka [[$1]] bola pridaná do [[Special:Watchlist|sledovaných stránok]]. Budú tam uvedené ďalšie úpravy tejto stránky a jej diskusie a stránka bude zobrazená '''tučne''' v [[Special:RecentChanges|zozname posledných úprav]], aby ste ju ľahšie našli.

Ak budete chcieť neskôr stránku odstrániť zo sledovaných stránok, kliknite na „{{MediaWiki:unwatch}}“ v záložkách na vrchu.",
'removedwatch'         => 'Odstránená zo zoznamu sledovaných stránok',
'removedwatchtext'     => 'Stránka „[[:$1]]“ bola odstránená z vášho zoznamu sledovaných stránok.',
'watch'                => 'Sledovať',
'watchthispage'        => 'Sledovať túto stránku',
'unwatch'              => 'Nesledovať',
'unwatchthispage'      => 'Prestať sledovať túto stránku',
'notanarticle'         => 'Toto nie je stránka',
'notvisiblerev'        => 'Revízia bola zmazaná',
'watchnochange'        => 'V rámci zobrazeného času nebola upravená žiadna z vašich sledovaných stránok.',
'watchlist-details'    => '{{PLURAL:$1|Jedna sledovaná stránka|$1 sledované stránky|$1 sledovaných stránok}}, nepočítajúc diskusné stránky.',
'wlheader-enotif'      => '* Upozorňovanie e-mailom je zapnuté.',
'wlheader-showupdated' => "* Stránky, ktoré boli zmené od vašej poslednej návštevy sú zobrazené '''tučne'''.",
'watchmethod-recent'   => 'kontrolujú sa sledované stránky v posledných úpravách',
'watchmethod-list'     => 'kontrolujú posledné úpravy v sledovaných stránkach',
'watchlistcontains'    => 'Váš zoznam sledovaných obsahuje {{PLURAL:$1|jednu stránku|$1 stránky|$1 stránok}}.',
'iteminvalidname'      => 'Problém s položkou „$1“, neplatné meno...',
'wlnote'               => "Nižšie {{PLURAL:$1|je posledná jedna zmena|sú posledné '''$1''' zmeny|je posledných '''$1''' zmien}} za {{PLURAL:$2|poslednú hodinu|posledné '''$2''' hodiny|posledných '''$2''' hodín}}.",
'wlshowlast'           => 'Zobraziť posledných $1 hodín $2 dní $3',
'watchlist-show-bots'  => 'Zobraziť úpravy botov',
'watchlist-hide-bots'  => 'Skryť úpravy botov',
'watchlist-show-own'   => 'Zobraziť moje úpravy',
'watchlist-hide-own'   => 'Skryť moje úpravy',
'watchlist-show-minor' => 'Zobraziť drobné úpravy',
'watchlist-hide-minor' => 'Skryť drobné úpravy',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Pridávam do zoznamu sledovaných...',
'unwatching' => 'Odoberám zo zoznamu sledovaných...',

'enotif_mailer'                => 'Upozorňovač {{GRAMMAR:genitív|{{SITENAME}}}}',
'enotif_reset'                 => 'Označiť všetky stránky ako „navštívené“',
'enotif_newpagetext'           => 'Toto je nová stránka.',
'enotif_impersonal_salutation' => 'používateľ {{GRAMMAR:genitív|{{SITENAME}}}}',
'changed'                      => 'zmene',
'created'                      => 'vytvorení',
'enotif_subject'               => '{{SITENAME}} - stránka $PAGETITLE bola $CHANGEDORCREATED $PAGEEDITOR',
'enotif_lastvisited'           => 'Všetky zmeny od vašej poslednej návštevy uvidíte na $1.',
'enotif_lastdiff'              => 'Zmenu uvidíte v $1.',
'enotif_anon_editor'           => 'anonymný používateľ $1',
'enotif_body'                  => 'Drahý $WATCHINGUSERNAME,

na {{GRAMMAR:lokál|{{SITENAME}}}} došlo $PAGEEDITDATE k $CHANGEDORCREATED stránky $PAGETITLE používateľom $PAGEEDITOR, pozrite si aktuálnu verziu $PAGETITLE_URL .

$NEWPAGE

Zhrnutie: $PAGESUMMARY $PAGEMINOREDIT
Kontaktujte používateľa:
mail $PAGEEDITOR_EMAIL
wiki $PAGEEDITOR_WIKI

Nedostanete ďalšie upozornenia, aj ak bude stránka znovu upravovaná, kým nenavštívíte túto stránku. Možete tiež vynulovať upozornenia pre všetky vaše sledované stránky.

 Váš upozorňovací systém {{GRAMMAR:genitív|{{SITENAME}}}}

--
Zmeniť nastavenia vašich sledovaných stránok môžete na
{{fullurl:Special:Watchlist/edit}}

Návrhy a ďalšia pomoc:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Zmazať stránku',
'confirm'                     => 'Potvrdiť',
'excontent'                   => 'obsah bol: „$1“',
'excontentauthor'             => 'obsah bol: „$1“ (a jediný autor bol [[Special:Contributions/$2]])',
'exbeforeblank'               => "obsah pred vyčistením stránky bol: '$1'",
'exblank'                     => 'stránka bola prázdna',
'delete-confirm'              => 'Zmazať „$1“',
'delete-legend'               => 'Zmazať',
'historywarning'              => 'UPOZORNENIE: Stránka, ktorú chcete zmazať má históriu:',
'confirmdeletetext'           => 'Chystáte sa trvalo zmazať z databázy stránku alebo obrázok spolu so všetkými jeho/jej predošlými verziami. Potvrďte, že máte v úmysle tak urobiť, že ste si vedomý následkov, a že to robíte v súlade so [[{{MediaWiki:Policy-url}}|zásadami a smernicami {{GRAMMAR:genitív|{{SITENAME}}}}]].',
'actioncomplete'              => 'Úloha bola dokončená',
'deletedtext'                 => '"<nowiki>$1</nowiki>" bol zmazaný.
Na $2 nájdete zoznam posledných zmazaní.',
'deletedarticle'              => '„[[$1]]“ zmazaná',
'suppressedarticle'           => 'potlačil „[[$1]]“',
'dellogpage'                  => 'Záznam zmazaní',
'dellogpagetext'              => 'Tu je zoznam posledných zmazaní.',
'deletionlog'                 => 'záznam zmazaní',
'reverted'                    => 'Obnovené na skoršiu verziu',
'deletecomment'               => 'Dôvod na zmazanie',
'deleteotherreason'           => 'Iný/ďalší dôvod:',
'deletereasonotherlist'       => 'Iný dôvod',
'deletereason-dropdown'       => '*Bežné dôvody zmazania
** Na žiadosť autora
** Porušenie autorských práv
** Vandalizmus',
'delete-edit-reasonlist'      => 'Upraviť dôvody zmazania',
'delete-toobig'               => 'Táto stránka má veľkú históriu úprav, viac ako $1 {{PLURAL:$1|revíziu|revízie|revízií}}. Mazanie takýchto stránok bolo obmedzené, aby sa zabránilo náhodnému poškodeniu {{GRAMMAR:genitív|{{SITENAME}}}}.',
'delete-warning-toobig'       => 'Táto stránka má veľkú históriu úprav, viac ako $1 {{PLURAL:$1|revíziu|revízie|revízií}}. Jej zmazanie by mohlo narušiť databázové operácie {{GRAMMAR:genitív|{{SITENAME}}}}; postupujte opatrne.',
'rollback'                    => 'Rollback úprav',
'rollback_short'              => 'Rollback',
'rollbacklink'                => 'rollback',
'rollbackfailed'              => 'Rollback neúspešný',
'cantrollback'                => 'Nie je možné úpravu vrátiť späť, posledný autor je jediný autor tejto stránky.',
'alreadyrolled'               => 'Nemožno vrátiť späť poslednú úpravu [[:$1]] od [[User:$2|$2]] ([[User talk:$2|Diskusia]] | [[Special:Contributions/$2|{{int:contribslink}}]]); niekto iný buď upravoval stránku alebo už vrátil úpravy späť.

Autorom poslednej úpravy je [[User:$3|$3]] ([[User talk:$3|Diskusia]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Komentár k úprave bol: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Posledné úpravy používateľa [[Special:Contributions/$2|$2]] ([[User talk:$2|diskusia]]) vrátené; bola obnovená posledná úprava $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Úpravy $1 vrátené; obnovená posledná verzia od $2.',
'sessionfailure'              => 'Zdá sa, že je problém s vašou prihlasovacou reláciou;
táto akcia bola zrušená ako prevencia proti zneužitiu relácie (session).
Prosím, stlačte "naspäť", obnovte stránku, z ktorej ste sa sem dostali, a skúste to znova.',
'protectlogpage'              => 'Záznam zamknutí',
'protectlogtext'              => 'Nižšie je zoznam zamknutí/odomknutí stránok.
Môžete si pozrieť aj [[Special:ProtectedPages|zoznam momentálne platných zamknutí]].',
'protectedarticle'            => 'zamyká „[[$1]]“',
'modifiedarticleprotection'   => 'zmenená úroveň ochrany „[[$1]]“',
'unprotectedarticle'          => 'odomyká „[[$1]]“',
'protect-title'               => 'Zamykám „$1“',
'protect-legend'              => 'Potvrďte zamknutie',
'protectcomment'              => 'Komentár:',
'protectexpiry'               => 'Zamknuté do:',
'protect_expiry_invalid'      => 'Neplatný čas vypršania.',
'protect_expiry_old'          => 'Čas vypršania je v minulosti.',
'protect-unchain'             => 'Odomknúť povolenia pre presun',
'protect-text'                => 'Tu si môžete pozrieť a zmeniť úroveň ochrany stránky <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => 'Nemôžete meniť úroveň ochrany, kým ste zablokovaný.
Tu sú aktuálne nastavenia stránky <strong>$1</strong>:',
'protect-locked-dblock'       => 'Nie je možné zmeniť úroveň ochrany z dôvodu aktívneho zámku databázy.
Tu sú aktuálne nastavenia stránky <strong>$1</strong>:',
'protect-locked-access'       => 'Váš účet nemá oprávnenie meniť úroveň ochrany stránky.
Tu sú aktuálne nastavenia stránky <strong>$1</strong>:',
'protect-cascadeon'           => 'Táto stránka je momentálne zamknutá, lebo je použitá na {{PLURAL:$1|nasledovnej stránke, ktorá má|nasledovných stránkach, ktoré majú}} zapnutú kaskádovú ochranu. Môžete zmeniť úroveň ochrany tejto stránky, ale neovplyvní to kaskádovú ochranu.',
'protect-default'             => '(predvolené)',
'protect-fallback'            => 'Vyžadovať povolenie „$1“',
'protect-level-autoconfirmed' => 'Zablokovať neregistrovaných používateľov',
'protect-level-sysop'         => 'Len pre správcov',
'protect-summary-cascade'     => 'kaskáda',
'protect-expiring'            => 'vyprší o $1 (UTC)',
'protect-cascade'             => 'Kaskádové zamknutie - chrániť všetky stránky použité na tejto stránke.',
'protect-cantedit'            => 'Nemôžete zmeniť úrovne ochrany tejto stránky, pretože nemáte oprávnenie ju upravovať.',
'restriction-type'            => 'Povolenie:',
'restriction-level'           => 'Úroveň obmedzenia:',
'minimum-size'                => 'Minimálna veľkosť',
'maximum-size'                => 'Maximálna veľkosť:',
'pagesize'                    => '(bajtov)',

# Restrictions (nouns)
'restriction-edit'   => 'Úprava',
'restriction-move'   => 'Presun',
'restriction-create' => 'Vytvoriť',
'restriction-upload' => 'Nahrať',

# Restriction levels
'restriction-level-sysop'         => 'úplne zamknutá',
'restriction-level-autoconfirmed' => 'čiastočne zamknutá',
'restriction-level-all'           => 'akákoľvek úroveň',

# Undelete
'undelete'                     => 'Obnoviť zmazanú stránku',
'undeletepage'                 => 'Zobraziť a obnoviť vymazané stránky',
'undeletepagetitle'            => "'''Nasledovné pozostáva zo zmazaných revízií stránky [[:$1]]'''.",
'viewdeletedpage'              => 'Zobraziť zmazané stránky',
'undeletepagetext'             => 'Tieto stránky boli zmazané, ale sú stále v archíve a
môžu byť obnovené. Archív môže byť pravidelne vyprázdnený.',
'undelete-fieldset-title'      => 'Obnoviť revízie',
'undeleteextrahelp'            => "Ak chcete obnoviť celú stránku, nechajte všetky zaškrtávacie polia nezaškrtnuté a kliknite na '''''Obnoviť'''''.
Ak chcete vykonať selektívnu obnovu, zašktrnite polia zodpovedajúce revíziám, ktoré sa majú obnoviť a kliknite na '''''Obnoviť'''''.
Kliknutie na '''''Reset''''' vyčistí pole s komentárom a všetky zaškrtávacie polia.",
'undeleterevisions'            => '$1 {{PLURAL:verzia je archivovaná|verzie sú archivované|verzií je archivovaných}}',
'undeletehistory'              => 'Ak obnovíte túto stránku, obnovia sa aj všetky predchádzajúce verzie do histórie predchádzajúcich verzií.
Ak bola od zmazania vytvorená nová stránka s rovnakým názvom, obnovené revízie sa objavia v histórii stránky.',
'undeleterevdel'               => 'Obnovenie sa nevykoná, ak by malo mať za dôsledok čiastočné zmazanie poslednej revízie. V takých prípadoch musíte odznačiť alebo odkryť najnovšie zmazané revízie.',
'undeletehistorynoadmin'       => 'Táto stránka bola zmazaná. Dôvod zmazania je zobrazený dolu v zhrnutí spolu s podrobnosťami o používateľoch, ktorí túto stránku upravovali pred zmazaním. Samotný text týchto zmazaných revízií je prístupný iba správcom.',
'undelete-revision'            => '$3 zmazal revíziu $1 (z $2):',
'undeleterevision-missing'     => 'Neplatná alebo chýbajúca revízia. Zrejme ste použili zlý odkaz alebo revízia bola obnovená alebo odstránená z histórie.',
'undelete-nodiff'              => 'Nebola nájdená žiadna predošlá revízia.',
'undeletebtn'                  => 'Obnoviť!',
'undeletelink'                 => 'obnoviť',
'undeletereset'                => 'Reset',
'undeletecomment'              => 'Komentár:',
'undeletedarticle'             => 'obnovený „[[$1]]“',
'undeletedrevisions'           => '{{PLURAL:$1|jedna verzia bola obnovená|$1 verzie boli obnovené|$1 verzií bolo obnovených}}',
'undeletedrevisions-files'     => '{{PLURAL:$1|Jedna revízia|$1 revízie|$1 revízií}} a {{PLURAL:$2|jeden súbor bol obnovený|$2 súbory boli obnovené|$2 súborov bolo obnovených}}',
'undeletedfiles'               => '{{PLURAL:$1|Jeden súbor bol obnovený|$1 súbory boli obnovené|$1 súborov bolo obnovených}}',
'cannotundelete'               => 'Obnovenie sa nepodarilo; pravdepodobne niekto iný obnovil stránku skôr ako vy.',
'undeletedpage'                => "<big>'''$1 bol obnovený'''</big>

Zoznam posledných mazaní a obnovení nájdete v [[Special:Log/delete|Zázname mazaní]].",
'undelete-header'              => 'Pozri nedávno zmazané stránky v [[Special:Log/delete|zázname mazaní]].',
'undelete-search-box'          => 'Hľadať zmazané stránky',
'undelete-search-prefix'       => 'Zobraziť stránky od:',
'undelete-search-submit'       => 'Hľadať',
'undelete-no-results'          => 'V archíve mazaní neboli nájdené zodpovedajúce stránky.',
'undelete-filename-mismatch'   => 'Nebolo možné obnoviť revíziu súboru s časovou známkou $1: rozdiel v názvoch súborov',
'undelete-bad-store-key'       => 'Nebolo možné obnoviť revíziu súboru s časovou známkou $1: súbor chýbal predtým, než bol zmazaný',
'undelete-cleanup-error'       => 'Chyba pri mazaní nepoužitého archívneho súboru „$1“.',
'undelete-missing-filearchive' => 'Nebolo možné obnoviť archív s ID $1, pretože sa nenachádza v databáze. Je možné, že už bol obnovený.',
'undelete-error-short'         => 'Chyba pri obnovovaní súboru: $1',
'undelete-error-long'          => 'Vyskytli sa chyby pri obnovovaní súboru:

$1',

# Namespace form on various pages
'namespace'      => 'Menný priestor:',
'invert'         => 'Invertovať výber',
'blanknamespace' => '(Hlavný)',

# Contributions
'contributions' => 'Príspevky používateľa',
'mycontris'     => 'Moje príspevky',
'contribsub2'   => 'Príspevky $1 ($2)',
'nocontribs'    => 'Neboli nájdené úpravy, ktoré by zodpovedali týmto kritériám.',
'uctop'         => '(posledná úprava)',
'month'         => 'Mesiac:',
'year'          => 'Rok:',

'sp-contributions-newbies'     => 'Zobraziť len príspevky nových účtov',
'sp-contributions-newbies-sub' => 'Príspevky nováčikov',
'sp-contributions-blocklog'    => 'Záznam blokovaní',
'sp-contributions-search'      => 'Hľadať príspevky',
'sp-contributions-username'    => 'IP adresa alebo meno používateľa:',
'sp-contributions-submit'      => 'Hľadať',

# What links here
'whatlinkshere'            => 'Odkazy na túto stránku',
'whatlinkshere-title'      => 'Stránky odkazujúce na „$1”',
'whatlinkshere-page'       => 'Page:',
'linklistsub'              => '(Zoznam odkazov)',
'linkshere'                => "Nasledujúce stránky odkazujú na '''[[:$1]]''':",
'nolinkshere'              => "Žiadne stránky neodkazujú na '''[[:$1]]'''.",
'nolinkshere-ns'           => "Žiadne stránky neodkazujú na '''[[:$1]]''' vo zvolenom mennom priestore.",
'isredirect'               => 'presmerovacia stránka',
'istemplate'               => 'použitá',
'isimage'                  => 'odkaz na obrázok',
'whatlinkshere-prev'       => '{{PLURAL:$1|predchádzajúca|predchádzajúce $1|predchádzajúcich $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|nasledujúca|nasledujúce $1|nasledujúcich $1}}',
'whatlinkshere-links'      => '← odkazy',
'whatlinkshere-hideredirs' => '$1 presmerovania',
'whatlinkshere-hidetrans'  => '$1 transklúzie',
'whatlinkshere-hidelinks'  => '$1 odkazy',
'whatlinkshere-hideimages' => '$1 odkazov na obrázok',
'whatlinkshere-filters'    => 'Filtre',

# Block/unblock
'blockip'                         => 'Zablokovať používateľa',
'blockip-legend'                  => 'Zablokovať používateľa',
'blockiptext'                     => 'Použite tento formulár na zablokovanie možnosti zápisov uskutočnených z konkrétnej IP adresy alebo od používateľa.
Mali by ste to urobiť len v prípade bránenia vandalizmu a v súlade so [[{{MediaWiki:Policy-url}}|zásadami a smernicami {{GRAMMAR:genitív|{{SITENAME}}}}]].
Nižšie uveďte konkrétny dôvod (napríklad uveďte konkrétne stránky, ktoré padli za obeť vandalizmu).',
'ipaddress'                       => 'IP adresa:',
'ipadressorusername'              => 'IP adresa/meno používateľa:',
'ipbexpiry'                       => 'Ukončenie:',
'ipbreason'                       => 'Dôvod:',
'ipbreasonotherlist'              => 'Iný dôvod',
'ipbreason-dropdown'              => '* Bežné dôvody blokovania
** Zámerné vkladanie chybných informácií
** Mazanie obsahu stránok
** Spam odkazy na externé stránky
** Vkladanie nezmyslov do stránok
** Zastrašujúce správanie/obťažovanie
** Zneužívanie viacerých účtov
** Neprípustné používateľské meno',
'ipbanononly'                     => 'Blokovať iba anonymných používateľov.',
'ipbcreateaccount'                => 'Zabrániť vytváraniu účtov',
'ipbemailban'                     => 'Zabrániť používateľovi posielať emaily',
'ipbenableautoblock'              => 'Automaticky blokovať poslednú IP adresu, ktorú tento používateľ použil, a všetky ďalšie adresy, z ktorých sa pokúsi upravovať.',
'ipbsubmit'                       => 'Zablokovať tohto používateľa',
'ipbother'                        => 'Iný čas',
'ipboptions'                      => '2 hodiny:2 hours,1 deň:1 day,3 dni:3 days,1 týždeň:1 week,2 týždne:2 weeks,1 mesiac:1 month,3 mesiace:3 months,6 mesiacov:6 months,1 rok:1 year,na neurčito:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'iný čas',
'ipbotherreason'                  => 'Iný/ďalší dôvod',
'ipbhidename'                     => 'Skryť používateľa zo záznamu blokovaní, aktívneho zoznamu blokovaní a zoznamu používateľov',
'ipbwatchuser'                    => 'Sledovať používateľskú a diskusnú stránku tohto používateľa',
'badipaddress'                    => 'IP adresa má nesprávny formát.',
'blockipsuccesssub'               => 'Zablokovanie bolo úspešné',
'blockipsuccesstext'              => '„$1“ bol/a zablokovaný/á.<br />
[[Special:IPBlockList|IP block list]] obsahuje zoznam blokovaní.',
'ipb-edit-dropdown'               => 'Upraviť dôvody pre blokovanie',
'ipb-unblock-addr'                => 'Odblokovať $1',
'ipb-unblock'                     => 'Odblokovať používateľa alebo IP adresu',
'ipb-blocklist-addr'              => 'Zobraziť existujúce blokovania pre $1',
'ipb-blocklist'                   => 'Zobraziť existujúce blokovania',
'unblockip'                       => 'Odblokovať používateľa',
'unblockiptext'                   => 'Použite tento formulár na obnovenie možnosti zápisov
z/od momentálne zablokovanej IP adresy/používateľa.',
'ipusubmit'                       => 'Odblokovať túto adresu',
'unblocked'                       => '[[User:$1|$1]] bol odblokovaný',
'unblocked-id'                    => 'Blokovanie $1 bolo odstránené',
'ipblocklist'                     => 'Zablokovaní používatelia/IP adresy',
'ipblocklist-legend'              => 'Nájsť zablokovaného používateľa',
'ipblocklist-username'            => 'Používateľské meno alebo IP adresa:',
'ipblocklist-submit'              => 'Hľadať',
'blocklistline'                   => '$1, $2 zablokoval $3 (ukončenie $4)',
'infiniteblock'                   => 'na neurčito',
'expiringblock'                   => 'ukončenie $1',
'anononlyblock'                   => 'iba anon.',
'noautoblockblock'                => 'automatické blokovanie vypnuté',
'createaccountblock'              => 'tvorba účtov bola zablokovaná',
'emailblock'                      => 'email blokovaný',
'ipblocklist-empty'               => 'Zoznam blokovaní je prázdny.',
'ipblocklist-no-results'          => 'Požadovaná IP adresa alebo používateľské meno nie je blokovaná.',
'blocklink'                       => 'zablokovať',
'unblocklink'                     => 'odblokuj',
'contribslink'                    => 'príspevky',
'autoblocker'                     => 'Ste zablokovaný, pretože zdieľate IP adresu s „$1“. Dôvod „$2“.',
'blocklogpage'                    => 'Záznam_blokovaní',
'blocklogentry'                   => 'zablokoval/a "[[$1]]" s časom ukončenia $2 $3',
'blocklogtext'                    => 'Toto je zoznam blokovaní a odblokovaní používateľov. Automaticky
blokované IP adresy nie sú zahrnuté. Pozri zoznam
[[Special:IPBlockList|aktuálnych blokovaní]].',
'unblocklogentry'                 => 'odblokoval/a $1',
'block-log-flags-anononly'        => 'iba anonymní používatelia',
'block-log-flags-nocreate'        => 'možnosť vytvoriť si účet bola vypnutá',
'block-log-flags-noautoblock'     => 'autoblokovanie vypnuté',
'block-log-flags-noemail'         => 'email blokovaný',
'block-log-flags-angry-autoblock' => 'rozšírené automatické blokovanie zapnuté',
'range_block_disabled'            => 'Možnosť správcov vytvárať rozsah zablokovaní je vypnutá.',
'ipb_expiry_invalid'              => 'Neplatný čas ukončenia.',
'ipb_expiry_temp'                 => 'Blokovania skrytých používateľských mien by mali byť trvalé.',
'ipb_already_blocked'             => '„$1“ je už zablokovaný',
'ipb_cant_unblock'                => 'Chyba: ID bloku $1 nenájdený. Možno už bol odblokovaný.',
'ipb_blocked_as_range'            => 'Chyba: IP adresa $1 nie je blokovaná priamo a nie je ju teda možné odblokovať. Je však blokovaná v rámci rozsahu $2, ktorý je možné odblokovať.',
'ip_range_invalid'                => 'Neplatný IP rozsah.',
'blockme'                         => 'Zablokuj ma',
'proxyblocker'                    => 'Blokovač proxy',
'proxyblocker-disabled'           => 'Táto funkcia je vypnutá.',
'proxyblockreason'                => 'Vaša IP adresa bola zablokovaná, pretože je otvorená proxy. Prosím kontaktujte vášho internetového poskytovateľa alebo technickú podporu a informujte ich o tomto vážnom bezpečnostnom probléme.',
'proxyblocksuccess'               => 'Hotovo.',
'sorbsreason'                     => 'Vaša IP adresa je vedená ako nezabezpečený proxy server v DNSBL.',
'sorbs_create_account_reason'     => 'Vaša IP adresa je vedená ako nezabezpečený proxy server v databáze DNSBL, ktorú používa {{SITENAME}}. Nemôžete si vytvoriť účet.',

# Developer tools
'lockdb'              => 'Zamknúť databázu',
'unlockdb'            => 'Odomknúť databázu',
'lockdbtext'          => 'Zamknutím databázy sa preruší možnosť všetkých
používateľov upravovať stránky, meniť svoje nastavenia, upravovať sledované stránky a
iné veci vyžadujúce zmeny v databáze.
Potvrďte, že to naozaj chcete urobiť, a že
odomknete databázu po ukončení údržby.',
'unlockdbtext'        => 'Odomknutie databázy obnoví schopnosť všetkých
používateľov upravovať stránky, meniť svoje nastavenia, upravovať svoj zoznam sledovaných stránok a
iné veci vyžadujúce zmeny v databáze.
Potvrďte, že to naozaj chcete urobiť.',
'lockconfirm'         => 'Áno, naozaj chcem zamknúť databázu.',
'unlockconfirm'       => 'Áno, naozaj chcem odomknúť databázu.',
'lockbtn'             => 'Zamknúť databázu',
'unlockbtn'           => 'Odomknúť databázu',
'locknoconfirm'       => 'Neoznačili ste potvrdzovacie pole.',
'lockdbsuccesssub'    => 'Zamknutie databázy úspešné',
'unlockdbsuccesssub'  => 'Databáza bola úspešne odomknutá',
'lockdbsuccesstext'   => 'Databáza bola zamknutá.<br />
Nezabudnite po dokončení údržby [[Special:UnlockDB|odstrániť zámok]].',
'unlockdbsuccesstext' => 'Databáza {{GRAMMAR:genitív|{{SITENAME}}}} bola odomknutá.',
'lockfilenotwritable' => 'Súbor, ktorý zamyká databázu nie je zapisovateľný. Aby bolo možné zamknúť či odomknúť databázu, je potrebné, aby doňho mohol web server zapisovať.',
'databasenotlocked'   => 'Databáza nie je zamknutá.',

# Move page
'move-page'               => 'Presunúť $1',
'move-page-legend'        => 'Presunúť stránku',
'movepagetext'            => "Pomocou tohto formulára premenujete stránku a premiestnite všetky jej predchádzajúce verzie pod zadaný nový názov.
Starý názov sa stane presmerovacou stránkou na nový názov.
Môžete automaticky aktualizovať odkazy odkazujúce na pôvodný názov.
Ak sa rozhodnete túto možnosť nevyužiť, ubezpečte sa, že ste skontrolovali
výskyt [[Special:DoubleRedirects|dvojitých]] a [[Special:BrokenRedirects|pokazených]] presmerovaní.
Vy ste zodpovedný za to, aby odkazy naďalej ukazovali tam, kam majú.

Uvedomte si, že stránka sa '''nepremiestni''', ak pod novým názvom už stránka existuje.
Toto neplatí iba ak je stránka prázdna alebo presmerovacia a nemá žiadne predchádzajúce verzie.
To znamená, že môžete premenovať stránku späť na názov, ktorý mala pred premenovaním, ak ste sa pomýlili, a že nemôžete prepísať
existujúcu stránku.

'''UPOZORNENIE!'''
Toto môže byť drastická a nečakaná zmena pre populárnu stránku;
ubezpečte sa preto, skôr ako budete pokračovať, že chápete dôsledky svojho činu.",
'movepagetalktext'        => "Príslušná diskusná stránka (ak existuje) bude premiestnená spolu so samotnou stránkou; '''nestane sa tak, iba ak:'''
*už existuje Diskusná stránka pod týmto novým menom, alebo
*nezaškrtnete nižšie sa nachádzajúci textový rámček.

V takých prípadoch budete musieť, ak si to želáte, premiestniť alebo zlúčiť stránku ručne.",
'movearticle'             => 'Presunúť stránku',
'movenotallowed'          => 'Nemáte povolenie presúvať stránky.',
'newtitle'                => 'Na nový názov:',
'move-watch'              => 'Sledovať túto stránku',
'movepagebtn'             => 'Presunúť stránku',
'pagemovedsub'            => 'Presun bol úspešný',
'movepage-moved'          => "<big>'''„$1” bolo presunuté na „$2”'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Stránka s týmto názvom už existuje alebo
vami zadaný názov je neplatný.
Prosím vyberte si iný názov.',
'cantmove-titleprotected' => 'Nemôžete sem presunúť stránku, pretože nový názov bol zamknutý proti vytvoreniu.',
'talkexists'              => "'''Samotná stránka bola úspešne premiestnená,
ale diskusná stránka sa nedala premiestniť,
pretože už jedna existuje pod zadaným novým názvom.
Prosím, zlúčte ich ručne.'''",
'movedto'                 => 'presunutá na',
'movetalk'                => 'Presunúť aj príslušnú diskusnú stránku, ak sa dá.',
'move-subpages'           => 'Presunúť všetky podstránky ak existujú',
'move-talk-subpages'      => 'Presunúť všetky podstránky diskusnej stránky ak existujú',
'movepage-page-exists'    => 'Stránka $1 už existuje a nie je možné ju automaticky prepísať.',
'movepage-page-moved'     => 'Stránka $1 bola presunutá na $2.',
'movepage-page-unmoved'   => 'Stránku $1 nebolo možné presunúť na $2.',
'movepage-max-pages'      => 'Maximum $1 {{PLURAL:$1|stránka bola presunutá|stránok bolo presunutých}} a viac nebude presunutých automaticky.',
'1movedto2'               => '[[$1]] premiestnená na [[$2]]',
'1movedto2_redir'         => '[[$1]] premiestnená na [[$2]] výmenou presmerovania',
'movelogpage'             => 'Záznam presunov',
'movelogpagetext'         => 'Tu je zoznam posledných presunutí.',
'movereason'              => 'Dôvod:',
'revertmove'              => 'obnova',
'delete_and_move'         => 'Vymazať a presunúť',
'delete_and_move_text'    => '==Je potrebné zmazať stránku==

Cieľová stránka „[[:$1]]“ už existuje. Chcete ho vymazať a vytvoriť tak priestor pre presun?',
'delete_and_move_confirm' => 'Áno, zmaž stránku',
'delete_and_move_reason'  => 'Vymazať, aby sa umožnil presun',
'selfmove'                => 'Zdrojový a cieľový názov sú rovnaké; nemožno presunúť stránku na seba samú.',
'immobile_namespace'      => 'Cieľový názov je špeciálneho typu; nemôžem presunúť stránku do tohto menného priestoru.',
'imagenocrossnamespace'   => 'Obrázok nemožno presunúť mimo menného priestoru obrázkov',
'imagetypemismatch'       => 'Nová prípona súboru nezodpovedá jeho typu',
'imageinvalidfilename'    => 'Cieľový názov obrázka nie je platný',
'fix-double-redirects'    => 'Aktualizovať všetky presmerovania odkazujúce na pôvodný názov',

# Export
'export'            => 'Export stránok',
'exporttext'        => 'Môžete exportovať text a históriu úprav konkrétnej
stránky alebo množiny stránok do XML; tieto môžu byť potom importované do inej
wiki používajúceho MediaWiki softvér pomocou stránky Special:Import.

Pre export stránok zadajte názvy do tohto poľa, jeden názov na riadok, a zvoľte, či chcete iba súčasnú verziu s informáciou o poslednej úprave alebo aj všetky staršie verzie s históriou úprav.

V druhom prípade môžete tiež použiť odkaz, napr. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] pre stránku [[{{MediaWiki:Mainpage}}]].',
'exportcuronly'     => 'Zahrň iba aktuálnu verziu, nie kompletnú históriu',
'exportnohistory'   => "----
'''Poznámka:''' Exportovanie plnej histórie stránok pomocou tohto formulára bolo vypnuté z dôvodov výkonnosti.",
'export-submit'     => 'Export',
'export-addcattext' => 'Pridať stránky z kategórie:',
'export-addcat'     => 'Pridať',
'export-download'   => 'Ponúknuť uloženie ako súbor',
'export-templates'  => 'Vrátane šablón',

# Namespace 8 related
'allmessages'               => 'Všetky systémové správy',
'allmessagesname'           => 'Názov',
'allmessagesdefault'        => 'štandardný text',
'allmessagescurrent'        => 'aktuálny text',
'allmessagestext'           => 'Toto je zoznam všetkých správ dostupných v mennom priestore MediaWiki.',
'allmessagesnotsupportedDB' => "Túto stránku nemožno použiť, pretože '''\$wgUseDatabaseMessages''' je vypnuté.",
'allmessagesfilter'         => 'Filter názvov správ:',
'allmessagesmodified'       => 'Zobraziť iba zmenené',

# Thumbnails
'thumbnail-more'           => 'Zväčšiť',
'filemissing'              => 'Chýbajúci súbor',
'thumbnail_error'          => 'Chyba pri vytváraní náhľadu: $1',
'djvu_page_error'          => 'DjVu stránka mimo rozsahu',
'djvu_no_xml'              => 'Nebolo možné priniesť XML DjVu súboru',
'thumbnail_invalid_params' => 'Neplatné parametre náhľadu',
'thumbnail_dest_directory' => 'Nebolo možné vytvoriť cieľový adresár',

# Special:Import
'import'                     => 'Import stránok',
'importinterwiki'            => 'Transwiki import',
'import-interwiki-text'      => 'Zvoľte wiki a názov stránky, ktorá sa má importovať.
Dátumy revízií a mená používateľov budú zachované.
Všetky transwiki importy sa zaznamenávajú v [[Special:Log/import|Zázname importov]].',
'import-interwiki-history'   => 'Skopírovať všetky historické revízie tejto stránky',
'import-interwiki-submit'    => 'Importovať',
'import-interwiki-namespace' => 'Presunúť stránky do menného priestoru:',
'importtext'                 => 'Prosím exportujte súbor zo zdrojovej wiki použitím [[Special:Export|nástroja na export]], uložte ho na svoj disk a nahrajte sem.',
'importstart'                => 'Importujú sa stránky...',
'import-revision-count'      => '$1 {{PLURAL:$1|revízia|revízie|revízií}}',
'importnopages'              => 'Žiadne stránky pre import.',
'importfailed'               => 'Chyba pri importe: $1',
'importunknownsource'        => 'Neznámy typ zdroja pre import',
'importcantopen'             => 'Nedal sa otvoriť súbor importu',
'importbadinterwiki'         => 'Zlý interwiki odkaz',
'importnotext'               => 'Prázdny alebo žiadny text',
'importsuccess'              => 'Import dokončený!',
'importhistoryconflict'      => 'Existujú konfliktné histórie revízií (možno už bola táto stránka importovaná)',
'importnosources'            => 'Neboli definované žiadne zdroje pre transwiki import a priame nahranie histórie je vypnuté.',
'importnofile'               => 'Nebol nahraný import súbor.',
'importuploaderrorsize'      => 'Nahranie alebo import súboru zlyhal. Súbor je väčší ako maximálna povolená veľkosť.',
'importuploaderrorpartial'   => 'Nahranie alebo import súboru zlyhal. Súbor bol nahraný iba čiastočne.',
'importuploaderrortemp'      => 'Nahranie alebo import súboru zlyhal. Dočasný adresár chýba.',
'import-parse-failure'       => 'Chyba syntaktickej analýzy pri XML importe',
'import-noarticle'           => 'Žiadny článok na import!',
'import-nonewrevisions'      => 'Všetky revízie boli predtým importované.',
'xml-error-string'           => '$1 na riadku $2, stĺpec $3 (bajt $4): $5',
'import-upload'              => 'Nahrať XML údaje',

# Import log
'importlogpage'                    => 'Záznam importov',
'importlogpagetext'                => 'Administratívny import stránok vrátane histórie úprav z iných wiki.',
'import-logentry-upload'           => 'importovaný $1 pomocou nahrania súboru',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|revízia|revízie|revízií}}',
'import-logentry-interwiki'        => 'Transwiki import $1 úspešný',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|revízia|revízie|revízií}} z $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Moja používateľská stránka',
'tooltip-pt-anonuserpage'         => 'Používateľská stránka pre ip adresu, ktorú upravujete ako',
'tooltip-pt-mytalk'               => 'Moja diskusná stránka',
'tooltip-pt-anontalk'             => 'Diskusia o úpravách z tejto ip adresy',
'tooltip-pt-preferences'          => 'Moje nastavenia',
'tooltip-pt-watchlist'            => 'Zoznam stránok, na ktorých sledujete zmeny.',
'tooltip-pt-mycontris'            => 'Zoznam mojich príspevkov',
'tooltip-pt-login'                => 'Odporúčame vám prihlásiť sa, nie je to však povinné.',
'tooltip-pt-anonlogin'            => 'Odporúčame vám prihlásiť sa, nie je to však povinné.',
'tooltip-pt-logout'               => 'Odhlásiť',
'tooltip-ca-talk'                 => 'Diskusia o obsahu stránky',
'tooltip-ca-edit'                 => 'Môžete upravovať túto stránku. Prosíme, pred uložením použite tlačidlo Zobraziť náhľad.',
'tooltip-ca-addsection'           => 'Pridať komentár k tejto diskusii.',
'tooltip-ca-viewsource'           => 'Táto stránka je zamknutá. Môžete však vidieť jej zdrojový text.',
'tooltip-ca-history'              => 'Minulé verzie tejto stránky.',
'tooltip-ca-protect'              => 'Zamknúť túto stránku',
'tooltip-ca-delete'               => 'Vymazať túto stránku',
'tooltip-ca-undelete'             => 'Obnoviť úpravy tejto stránky až po dobu, kedy bola vymazaná',
'tooltip-ca-move'                 => 'Presunúť túto stránku',
'tooltip-ca-watch'                => 'Pridať túto stránku do zoznamu sledovaných stránok',
'tooltip-ca-unwatch'              => 'Odstrániť túto stránku zo sledovaných stránok',
'tooltip-search'                  => 'Vyhľadávanie na {{GRAMMAR:datív|{{SITENAME}}}}',
'tooltip-search-go'               => 'Prejsť na stránku s presne takýmto názvom, ak existuje',
'tooltip-search-fulltext'         => 'Hľadať tento text na stránkach',
'tooltip-p-logo'                  => 'Hlavná stránka',
'tooltip-n-mainpage'              => 'Navštíviť Hlavnú stránku',
'tooltip-n-portal'                => 'O projekte, ako môžete prispieť, kde čo nájsť',
'tooltip-n-currentevents'         => 'Aktuálne udalosti a ich pozadie',
'tooltip-n-recentchanges'         => 'Zoznam posledných úprav vo wiki.',
'tooltip-n-randompage'            => 'Zobrazenie náhodnej stránky',
'tooltip-n-help'                  => 'Pozrieť si pomoc.',
'tooltip-t-whatlinkshere'         => 'Zoznam všetkých wiki stránok, ktoré sem odkazujú',
'tooltip-t-recentchangeslinked'   => 'Posledné úpravy v stránkach, ktoré odkazujú na túto stránku',
'tooltip-feed-rss'                => 'RSS feed pre túto stránku',
'tooltip-feed-atom'               => 'Atom feed pre túto stránku',
'tooltip-t-contributions'         => 'Pozrieť si zoznam príspevkov od tohto používateľa',
'tooltip-t-emailuser'             => 'Poslať e-mail tomuto používateľovi',
'tooltip-t-upload'                => 'Nahranie obrázkových alebo multimediálnych súborov',
'tooltip-t-specialpages'          => 'Zoznam všetkých špeciálnych stránok',
'tooltip-t-print'                 => 'Verzia tejto stránky pre tlač',
'tooltip-t-permalink'             => 'Trvalý odkaz na túto verziu stránky',
'tooltip-ca-nstab-main'           => 'Pozrieť si obsah stránky',
'tooltip-ca-nstab-user'           => 'Pozrieť si stránku používateľa',
'tooltip-ca-nstab-media'          => 'Pozrieť si stránku médií',
'tooltip-ca-nstab-special'        => 'Toto je špeciálna stránka, nemôžete ju upravovať.',
'tooltip-ca-nstab-project'        => 'Pozrieť si stránku projektu',
'tooltip-ca-nstab-image'          => 'Zobraziť popisnú stránku súboru',
'tooltip-ca-nstab-mediawiki'      => 'Pozrieť si systémovú stránku',
'tooltip-ca-nstab-template'       => 'Pozrieť si šablónu',
'tooltip-ca-nstab-help'           => 'Pozrieť si stránku Pomocníka',
'tooltip-ca-nstab-category'       => 'Pozrieť si stránku s kategóriami',
'tooltip-minoredit'               => 'Označiť túto úpravu ako drobnú',
'tooltip-save'                    => 'Uložiť vaše úpravy',
'tooltip-preview'                 => 'Náhľad úprav, prosím použite pred uložením!',
'tooltip-diff'                    => 'Zobraziť, aké zmeny ste urobili v texte.',
'tooltip-compareselectedversions' => 'Zobraziť rozdiely medzi dvomi zvolenými verziami tejto stránky.',
'tooltip-watch'                   => 'Pridať túto stránku k sledovaným.',
'tooltip-recreate'                => 'Znovu vytvoriť stránku napriek tomu, že bola zmazaná',
'tooltip-upload'                  => 'Začať nahrávanie',

# Stylesheets
'common.css'   => '/** Tu sa nachádzajúce CSS sa použije pri všetkých skinoch */',
'monobook.css' => '/* úpravou tohto súboru si prispôsobíte skin monobook pre celú wiki */',

# Scripts
'common.js'   => '/* Tu sa nachádzajúci JavaScript sa načíta všetkým používateľom pri každom načítaní stránky. */',
'monobook.js' => '/* Zastaralé; použite [[MediaWiki:common.js]] */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadáta sú pre tento server vypnuté.',
'nocreativecommons' => 'Creative Commons RDF metadata sú pre tento server vypnuté.',
'notacceptable'     => 'Wiki server nedokáže poskytovať dáta vo formáte, v akom ich váš klient vie čítať.',

# Attribution
'anonymous'        => 'anonymných používateľov {{GRAMMAR:genitív|{{SITENAME}}}}',
'siteuser'         => 'používateľa {{GRAMMAR:genitív|{{SITENAME}}}} $1',
'lastmodifiedatby' => 'Túto stránku naposledy upravoval používateľ $3 $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Založené na práci $1.',
'others'           => 'iné',
'siteusers'        => 'používateľov {{GRAMMAR:genitív|{{SITENAME}}}} $1',
'creditspage'      => 'Autori stránky',
'nocredits'        => 'Pre túto stránku neexistujú žiadne dostupné ocenenia.',

# Spam protection
'spamprotectiontitle' => 'Filter na ochranu pred spamom',
'spamprotectiontext'  => 'Stránka, ktorú ste chceli uložiť, bola blokovaná spamovým filtrom.
Pravdepodobne to spôsobil odkaz na externú internetovú lokalitu, ktorá sa nachádza na čiernej listine.',
'spamprotectionmatch' => 'Nasledujúci text aktivoval náš spam filter: $1',
'spambot_username'    => 'MediaWiki čistenie spamu',
'spam_reverting'      => 'Vraciam poslednú verziu, ktorá neobsahuje odkazy na $1',
'spam_blanking'       => 'Všetky revízie obsahovali odkaz na $1, odstraňujem obsah',

# Info page
'infosubtitle'   => 'Informácie o stránke',
'numedits'       => 'Počet úprav (stránka): $1',
'numtalkedits'   => 'Počet úprav (diskusná stránka): $1',
'numwatchers'    => 'Počet zobrazení: $1',
'numauthors'     => 'Počet odlišných autorov (stránka): $1',
'numtalkauthors' => 'Počet odlišných autorov (diskusná stránka): $1',

# Math options
'mw_math_png'    => 'Vždy vykresľovať PNG',
'mw_math_simple' => 'Na jednoduché použiť HTML, inak PNG',
'mw_math_html'   => 'Ak sa dá, použiť HTML, inak PNG',
'mw_math_source' => 'Ponechať TeX (pre textové prehliadače)',
'mw_math_modern' => 'Odporúčané pre moderné prehliadače',
'mw_math_mathml' => 'MathML (experimentálne)',

# Patrolling
'markaspatrolleddiff'                 => 'Označiť ako stráženú',
'markaspatrolledtext'                 => 'Označiť túto stránku ako stráženú',
'markedaspatrolled'                   => 'Označené ako strážené',
'markedaspatrolledtext'               => 'Vybraná verzia bola označená na stráženie.',
'rcpatroldisabled'                    => 'Stráženie posledných zmien bolo vypnuté',
'rcpatroldisabledtext'                => 'Funkcia stráženia posledných zmien je momentálne vypnutá.',
'markedaspatrollederror'              => 'Nie je možné označiť ako strážené',
'markedaspatrollederrortext'          => 'Pre označenie ako strážený je potrebné uviesť revíziu, ktorá sa má označiť ako strážená.',
'markedaspatrollederror-noautopatrol' => 'Nie je vám umožnené označiť vlastné zmeny za strážené.',

# Patrol log
'patrol-log-page'   => 'Záznam strážení',
'patrol-log-header' => 'Toto je záznam strážených revízií.',
'patrol-log-line'   => '$1 z $2 označených ako sledované $3',
'patrol-log-auto'   => '(automaticky)',

# Image deletion
'deletedrevision'                 => 'Zmazať staré verzie $1',
'filedeleteerror-short'           => 'Chyba pri mazaní súboru: $1',
'filedeleteerror-long'            => 'Vyskytli sa chyby pri mazaní súboru:

$1',
'filedelete-missing'              => 'Súbor „$1“ nebolo možné zmazať, pretože neexistuje.',
'filedelete-old-unregistered'     => 'Požadovaná revízia súboru „$1“ sa nenachádza v databáze.',
'filedelete-current-unregistered' => 'Požadovaný súbor „$1“ sa nenachádza v databáze.',
'filedelete-archive-read-only'    => 'Webový server nemôže zapisovať do archívneho adresára „$1“.',

# Browsing diffs
'previousdiff' => '← Predchádzajúca úprava',
'nextdiff'     => 'Ďalšia úprava →',

# Media information
'mediawarning'         => "'''Upozornenie''': Tento súbor môže obsahovať nebezpečný programový kód, po spustení ktorého by bol váš systém kompromitovaný.
<hr />",
'imagemaxsize'         => 'Obmedziť obrázky na popisnej stránke súboru na:',
'thumbsize'            => 'Veľkosť náhľadu:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|stránka|stránky|stránok}}',
'file-info'            => '(veľkosť súboru: $1, MIME typ: $2)',
'file-info-size'       => '($1 × $2 pixelov, veľkosť súboru: $3, MIME typ: $4)',
'file-nohires'         => '<small>Nie je dostupné vyššie rozlíšenie.</small>',
'svg-long-desc'        => '(SVG súbor, $1 × $2 pixelov, veľkosť súboru: $3)',
'show-big-image'       => 'Obrázok vo vyššom rozlíšení',
'show-big-image-thumb' => '<small>Veľkosť tohto náhľadu: $1 × $2 pixelov</small>',

# Special:NewImages
'newimages'             => 'Galéria nových obrázkov',
'imagelisttext'         => "Tu je zoznam {{PLURAL:$1|jedného súboru|'''$1''' súborov|'''$1''' súborov}} zoradený $2.",
'newimages-summary'     => 'Táto špeciálna stránka zobrazuje posledné nahrané súbory.',
'showhidebots'          => '($1 botov)',
'noimages'              => 'Niet čo zobraziť.',
'ilsubmit'              => 'Hľadať',
'bydate'                => 'podľa dátumu',
'sp-newimages-showfrom' => 'Zobraziť nové súbory počínajúc $2, $1',

# Bad image list
'bad_image_list' => 'Formát je nasledovný:

Berú sa do úvahy iba položky zoznamu (riadky začínajúce *). Prvý odkaz na riadku musí byť odkaz na zlý súbor.
Každý ďalší odkaz na rovnakom riadku sa považuje za výnimku, t.j. články, v ktorých sa súbor môže vyskytnúť.',

# Metadata
'metadata'          => 'Metadáta',
'metadata-help'     => 'Tento súbor obsahuje ďalšie informácie, pravdepodobne pochádzajúce z digitálneho fotoaparátu či skenera, ktorý ho vytvoril alebo digitalizoval. Ak bol súbor zmenený, niektoré podrobnosti sa nemusia plne zhodovať so zmeneným súborom.',
'metadata-expand'   => 'Zobraziť detaily EXIF',
'metadata-collapse' => 'Skryť detaily EXIF',
'metadata-fields'   => 'Polia EXIF metadát uvedených v tejto správe sa zobrazia na stránke obrázka vtedy, keď je tabuľka metadát zbalená. Ostatné sa štandardne nezobrazia.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Šírka',
'exif-imagelength'                 => 'Výška',
'exif-bitspersample'               => 'Bitov na zložku',
'exif-compression'                 => 'Kompresná schéma',
'exif-photometricinterpretation'   => 'Pixelové zloženie',
'exif-orientation'                 => 'Orientácia',
'exif-samplesperpixel'             => 'Počet zložiek',
'exif-planarconfiguration'         => 'Rozloženie dát',
'exif-ycbcrsubsampling'            => 'Pomer podvzorkovania Y ku C',
'exif-ycbcrpositioning'            => 'Poloha Y a C',
'exif-xresolution'                 => 'Horizontálne rozlíšenie',
'exif-yresolution'                 => 'Vertikálne rozlíšenie',
'exif-resolutionunit'              => 'Jednotky horizontálneho a verikálneho rozlíšenia',
'exif-stripoffsets'                => 'Umiestnenie obrazových dát',
'exif-rowsperstrip'                => 'Počet riadkov na pás',
'exif-stripbytecounts'             => 'Bajtov na komprimovaný prúžok',
'exif-jpeginterchangeformat'       => 'Offset k JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bytov JPEG dát',
'exif-transferfunction'            => 'Prenosová funkcia',
'exif-whitepoint'                  => 'Chromaticita bieleho bodu',
'exif-primarychromaticities'       => 'Chromaticity primárností',
'exif-ycbcrcoefficients'           => 'Koeficienty transformačnej matice farebného priestoru',
'exif-referenceblackwhite'         => 'Dvojica bielych a čiernych referenčných hodnôt',
'exif-datetime'                    => 'Dátum a čas zmeny súboru',
'exif-imagedescription'            => 'Názov obrázka',
'exif-make'                        => 'Výrobca aparátu',
'exif-model'                       => 'Model aparátu',
'exif-software'                    => 'Použitý softvér',
'exif-artist'                      => 'Autor',
'exif-copyright'                   => 'Držiteľ autorských práv',
'exif-exifversion'                 => 'Verzia exif štítka',
'exif-flashpixversion'             => 'Podporovaná verzia Flashpix',
'exif-colorspace'                  => 'Farebný priestor',
'exif-componentsconfiguration'     => 'Význam jednotlivých zložiek',
'exif-compressedbitsperpixel'      => 'Komprimované bity na pixel',
'exif-pixelydimension'             => 'Platná šírka obrázka',
'exif-pixelxdimension'             => 'Platná vyška obrázka',
'exif-makernote'                   => 'Poznámka výrobcu',
'exif-usercomment'                 => 'Komentár používateľa',
'exif-relatedsoundfile'            => 'Súvisiaci zvukový súbor',
'exif-datetimeoriginal'            => 'Dátum a čas vytvorenia dát',
'exif-datetimedigitized'           => 'Dátum a čas digitalizácie',
'exif-subsectime'                  => 'Subsekundy DateTime',
'exif-subsectimeoriginal'          => 'Zlomky sekundy DateTimeOriginal',
'exif-subsectimedigitized'         => 'Zlomky sekundy DateTimeDigitized',
'exif-exposuretime'                => 'Expozičný čas',
'exif-exposuretime-format'         => '$1 sekundy ($2)',
'exif-fnumber'                     => 'Číslo F',
'exif-exposureprogram'             => 'Expozičný program',
'exif-spectralsensitivity'         => 'Spektrálna citlivosť',
'exif-isospeedratings'             => 'Rýchlostné ohodnotenie ISO',
'exif-oecf'                        => 'Optoelektronický konverzný činiteľ',
'exif-shutterspeedvalue'           => 'Rýchlosť uzávierky',
'exif-aperturevalue'               => 'Clona',
'exif-brightnessvalue'             => 'Jas',
'exif-exposurebiasvalue'           => 'Expozičné skreslenie',
'exif-maxaperturevalue'            => 'Maximálna krajinná clona',
'exif-subjectdistance'             => 'Vzdialenosť subjektu',
'exif-meteringmode'                => 'Merací režim',
'exif-lightsource'                 => 'Svetelný zdroj',
'exif-flash'                       => 'Blesk',
'exif-focallength'                 => 'Ohnisková vzdialenosť objektívu',
'exif-subjectarea'                 => 'Oblasť subjektu',
'exif-flashenergy'                 => 'Energia blesku',
'exif-spatialfrequencyresponse'    => 'Priestorová frekvenčná odozva',
'exif-focalplanexresolution'       => 'Horizontálne rozlíšenie ohniskovej roviny',
'exif-focalplaneyresolution'       => 'Vertikálne rozlíšenie ohniskovej roviny',
'exif-focalplaneresolutionunit'    => 'Jednotka rozlíšenia v ohniskovej rovine',
'exif-subjectlocation'             => 'Umiestnenie subjektu',
'exif-exposureindex'               => 'Expozičný index',
'exif-sensingmethod'               => 'Snímacia metóda',
'exif-filesource'                  => 'Zdroj súboru',
'exif-scenetype'                   => 'Typ scény',
'exif-cfapattern'                  => 'Vzor CFA',
'exif-customrendered'              => 'Ručné spracovanie obrazu',
'exif-exposuremode'                => 'Expozičný režim',
'exif-whitebalance'                => 'Vyváženie bielej',
'exif-digitalzoomratio'            => 'Pomer digitálneho priblíženia',
'exif-focallengthin35mmfilm'       => 'Ohnisková vzdialenosť 35 mm filmu',
'exif-scenecapturetype'            => 'Typ zachytenia scény',
'exif-gaincontrol'                 => 'Riadenie zosilnenia',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Sýtosť',
'exif-sharpness'                   => 'Ostrosť',
'exif-devicesettingdescription'    => 'Opis nastavení zariadenia',
'exif-subjectdistancerange'        => 'Rozsah vzdialenosti subjektu',
'exif-imageuniqueid'               => 'Jedinečný ID obrázka',
'exif-gpsversionid'                => 'Verzia GPS štítka',
'exif-gpslatituderef'              => 'Severná alebo južná šírka',
'exif-gpslatitude'                 => 'Zemepisná šírka',
'exif-gpslongituderef'             => 'Východná alebo západná dĺžka',
'exif-gpslongitude'                => 'Zemepisná dĺžka',
'exif-gpsaltituderef'              => 'Referencia nadmorskej výšky',
'exif-gpsaltitude'                 => 'Nadmorská výška',
'exif-gpstimestamp'                => 'Čas GPS (atómové hodiny)',
'exif-gpssatellites'               => 'Satelity použité pri meraní',
'exif-gpsstatus'                   => 'Stav prijímača',
'exif-gpsmeasuremode'              => 'Režim merania',
'exif-gpsdop'                      => 'Presnosť merania',
'exif-gpsspeedref'                 => 'Rýchlostná jednotka',
'exif-gpsspeed'                    => 'Rýchlosť prijímača GPS',
'exif-gpstrackref'                 => 'Referencia pre smer pohybu',
'exif-gpstrack'                    => 'Smer pohybu',
'exif-gpsimgdirectionref'          => 'Referencia pre smer obrázka',
'exif-gpsimgdirection'             => 'Smer obrázka',
'exif-gpsmapdatum'                 => 'Použité údaje geodetického prieskumu',
'exif-gpsdestlatituderef'          => 'Referencia zemepisnej šírky cieľa',
'exif-gpsdestlatitude'             => 'Zemepisná šírka cieľa',
'exif-gpsdestlongituderef'         => 'Referencia zemepisnej dĺžky cieľa',
'exif-gpsdestlongitude'            => 'Zemepisná dĺžka cieľa',
'exif-gpsdestbearingref'           => 'Referencia polohy cieľa',
'exif-gpsdestbearing'              => 'Smer k cieľu',
'exif-gpsdestdistanceref'          => 'Referencia vzdialenosti cieľa',
'exif-gpsdestdistance'             => 'Vzdialenosť k cieľu',
'exif-gpsprocessingmethod'         => 'Názov GPS metódy spracovania',
'exif-gpsareainformation'          => 'Názov GPS oblasti',
'exif-gpsdatestamp'                => 'Dátum GPS',
'exif-gpsdifferential'             => 'Diferenciálna korekcia GPS',

# EXIF attributes
'exif-compression-1' => 'Bez kompresie',

'exif-unknowndate' => 'Neznámy dátum',

'exif-orientation-1' => 'Normálna', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Horizontálne prevrátená', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Otočená o 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Vertikálne prevrátená', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Otočená o 90° proti smeru hodinových ručičiek a vertikálne prevrátená', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Otočená o 90° v smere hodinových ručičiek', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Otočená o 90° v smere hodinových ručičiek a vertikálne prevrátená', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Otočená o 90° proti smeru hodinových ručičiek', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'masívny formát',
'exif-planarconfiguration-2' => 'rovinný formát',

'exif-componentsconfiguration-0' => 'neexistuje',

'exif-exposureprogram-0' => 'Nedefinovaný',
'exif-exposureprogram-1' => 'Ručný',
'exif-exposureprogram-2' => 'Normálny program',
'exif-exposureprogram-3' => 'Priorita clony',
'exif-exposureprogram-4' => 'Priorita uzávierky',
'exif-exposureprogram-5' => 'Tvorivý program (prevažuje smerom k hĺbke poľa)',
'exif-exposureprogram-6' => 'Akčný program (prevažuje smerom k rýchlosti uzávierky)',
'exif-exposureprogram-7' => 'Režim portrét (pre detailné zábery s nezaostreným pozadím)',
'exif-exposureprogram-8' => 'Režim krajinka (pre fotografie krajiny so zaostreným pozadím)',

'exif-subjectdistance-value' => '$1 metrov',

'exif-meteringmode-0'   => 'Neznámy',
'exif-meteringmode-1'   => 'Priemer',
'exif-meteringmode-2'   => 'Vážený priemer',
'exif-meteringmode-3'   => 'Bod',
'exif-meteringmode-4'   => 'Viacero bodov',
'exif-meteringmode-5'   => 'Vzor',
'exif-meteringmode-6'   => 'Čiastočný',
'exif-meteringmode-255' => 'Iný',

'exif-lightsource-0'   => 'Neznámy',
'exif-lightsource-1'   => 'Denné svetlo',
'exif-lightsource-2'   => 'Fluorescenčný',
'exif-lightsource-3'   => 'Volfrám (inkandescentné svetlo)',
'exif-lightsource-4'   => 'Blesk',
'exif-lightsource-9'   => 'Dobré počasie',
'exif-lightsource-10'  => 'Hmlisté počasie',
'exif-lightsource-11'  => 'Tieň',
'exif-lightsource-12'  => 'Fluorescenčné denné svetlo (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Flourescenčná denná biela (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Fuorescenčná chladná biela (W 3900 – 4500K)',
'exif-lightsource-15'  => 'Fluorescenčná biela (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Štandardné svetlo A',
'exif-lightsource-18'  => 'Štandardné svetlo B',
'exif-lightsource-19'  => 'Štandardné svetlo C',
'exif-lightsource-24'  => 'ISO štúdiový volfrám',
'exif-lightsource-255' => 'Iný svetelný zdroj',

'exif-focalplaneresolutionunit-2' => 'palcov',

'exif-sensingmethod-1' => 'Nedefinovaná',
'exif-sensingmethod-2' => 'Jednočipový farebný snímač oblasti',
'exif-sensingmethod-3' => 'Dvojčipový farebný snímač oblasti',
'exif-sensingmethod-4' => 'Trojčipový farebný snímač oblasti',
'exif-sensingmethod-5' => 'Sekvenčný farebný snímač oblasti',
'exif-sensingmethod-7' => 'Trilineárny snímač',
'exif-sensingmethod-8' => 'Sekvenčný farebný lineárny snímač',

'exif-scenetype-1' => 'Priamo odfotený obrázok',

'exif-customrendered-0' => 'Normálne spracovanie',
'exif-customrendered-1' => 'Ručné spracovanie',

'exif-exposuremode-0' => 'Automatická expozícia',
'exif-exposuremode-1' => 'Ručná expozícia',
'exif-exposuremode-2' => 'Automatická kompenzácia expozície',

'exif-whitebalance-0' => 'Automatické vyváženie bielej',
'exif-whitebalance-1' => 'Ručné vyváženie bielej',

'exif-scenecapturetype-0' => 'Štandardný',
'exif-scenecapturetype-1' => 'Krajinka',
'exif-scenecapturetype-2' => 'Portrét',
'exif-scenecapturetype-3' => 'Nočná scéna',

'exif-gaincontrol-0' => 'Žiadne',
'exif-gaincontrol-1' => 'Slabé zosilnenie nahor',
'exif-gaincontrol-2' => 'Silné zosilnenie nahor',
'exif-gaincontrol-3' => 'Slabé zosilnenie nadol',
'exif-gaincontrol-4' => 'Silné zosilnenie nadol',

'exif-contrast-0' => 'Normálny',
'exif-contrast-1' => 'Mäkký',
'exif-contrast-2' => 'Tvrdý',

'exif-saturation-0' => 'Normálna',
'exif-saturation-1' => 'Nízka sýtosť',
'exif-saturation-2' => 'Výsoká sýtosť',

'exif-sharpness-0' => 'Normálna',
'exif-sharpness-1' => 'Mäkká',
'exif-sharpness-2' => 'Tvrdá',

'exif-subjectdistancerange-0' => 'Neznámy',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Blízky pohľad',
'exif-subjectdistancerange-3' => 'Ďaleký pohľad',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Severná šírka',
'exif-gpslatitude-s' => 'Južná šírka',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Východná dĺžka',
'exif-gpslongitude-w' => 'Západná dĺžka',

'exif-gpsstatus-a' => 'Prebieha meranie',
'exif-gpsstatus-v' => 'Interoperabilita merania',

'exif-gpsmeasuremode-2' => '2-rozmerné meranie',
'exif-gpsmeasuremode-3' => '3-rozmerné meranie',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometrov za hodinu',
'exif-gpsspeed-m' => 'Míľ za hodinu',
'exif-gpsspeed-n' => 'Uzlov',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Skutočný smer',
'exif-gpsdirection-m' => 'Magnetický smer',

# External editor support
'edit-externally'      => 'Upraviť tento súbor pomocou externého programu',
'edit-externally-help' => 'Viac informácií poskytnú inštrukcie pre nastavenie [http://www.mediawiki.org/wiki/Manual:External_editors externého editora].',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'všetky',
'imagelistall'     => 'všetky',
'watchlistall2'    => 'všetky',
'namespacesall'    => 'všetky',
'monthsall'        => 'všetky',

# E-mail address confirmation
'confirmemail'             => 'Potvrdiť e-mailovú adresu',
'confirmemail_noemail'     => 'Nenastavili ste platnú emailovú adresu vo svojich [[Special:Preferences|Nastaveniach]].',
'confirmemail_text'        => '{{SITENAME}} vyžaduje, aby ste potvrdili platnosť vašej e-mailovej adresy
pred používaním e-mailových funkcií. Kliknite na tlačidlo dole, aby sa na vašu adresu odoslal potvrdzovací
e-mail. V e-maili bude aj odkaz obsahujúci kód; otvorte odkaz
vo vašom prehliadači, čím potvrdíte, že vaša e-mailová adresa je platná.',
'confirmemail_pending'     => '<div class="error">
Potvrdzovací kód vám už bol zaslaný; ak ste si účet vytvorili len nedávno
mali by ste počkať niekoľko minút, kým vám bude email doručený, predtým
než si vyžiadate nový kód.
</div>',
'confirmemail_send'        => 'Odoslať potvrdzovací kód',
'confirmemail_sent'        => 'Potvrdzovací e-mail odoslaný.',
'confirmemail_oncreate'    => 'Na vašu emailovú adresu bol odoslaný potvrdzovací kód.
Tento kód nie je potrebný na prihlásenie, ale budete ho musieť poskytnúť pred
zapnutím vlastností wiki využívajcich email.',
'confirmemail_sendfailed'  => 'Z {{GRAMMAR:genitív|{{SITENAME}}}} nebolo možné odoslať potvrdzovací e-mail.
Skontrolujte neplatné znaky vo vašej emailovej adrese.

Hlásenie programu, ktorý odosielal poštu: $1',
'confirmemail_invalid'     => 'Neplatný potvrdzovací kód. Kód možno vypršal.',
'confirmemail_needlogin'   => 'Musíte sa $1 na potvrdenie vašej emailovaj adresy.',
'confirmemail_success'     => 'Vaša e-mailová adresa bola potvrdená. Môžete sa prihlásiť a využívať wiki.',
'confirmemail_loggedin'    => 'Vaša e-mailová adresa bola potvrdená.',
'confirmemail_error'       => 'Niečo sa pokazilo pri ukladaní vášho potvrdenia.',
'confirmemail_subject'     => '{{SITENAME}} - potvrdenie e-mailovej adresy',
'confirmemail_body'        => 'Niekto, pravdepodobne vy z IP adresy $1, zaregistroval účet
„$2“ s touto e-mailovou adresou na {{GRAMMAR:lokál|{{SITENAME}}}}.

Pre potvrdenie, že tento účet skutočne patrí vám a pre aktivovanie
e-mailových funkcií na {{GRAMMAR:lokál|{{SITENAME}}}}, otvorte tento odkaz vo vašom prehliadači:

$3

Ak ste to *neboli* vy, otvorte tento odkaz,
čím zrušíte potvrdenie emailovej adresy:

$5

Tento potvrdzovací kód vyprší $4.',
'confirmemail_invalidated' => 'Potvrdenie emailovej adresy bolo zrušené',
'invalidateemail'          => 'Zrušiť potvrdenie emailovej adresy',

# Scary transclusion
'scarytranscludedisabled' => '[Transklúzia interwiki je vypnutá]',
'scarytranscludefailed'   => '[Nepodarilo sa priniesť šablónu pre $1]',
'scarytranscludetoolong'  => '[URL je príliš dlhé]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Trackback pre túto stránku:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Zmazať])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback úspešne zmazaný.',

# Delete conflict
'deletedwhileediting' => "'''Upozornenie''': Táto stránka bola zmazaná potom ako ste začali s jej úpravami!",
'confirmrecreate'     => "Používateľ [[User:$1|$1]] ([[User talk:$1|diskusia]]) zmazal túto stránku potom, ako ste ju začali upravovať, s odôvodnením:
: ''$2''
Prosím, potvrďte, že túto stránku chcete skutočne znovu vytvoriť.",
'recreate'            => 'Znova vytvoriť',

# HTML dump
'redirectingto' => 'Presmerovanie na [[:$1]]...',

# action=purge
'confirm_purge'        => 'Vyčistiť vyrovnávaciu pamäť (cache) tejto stránky?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Hľadať stránky obsahujúce ''$1''.",
'searchnamed'      => "Hľadať stránky s názvom ''$1''.",
'articletitles'    => "Stránky začínajúce na ''$1''",
'hideresults'      => 'Skryť výsledky',
'useajaxsearch'    => 'Použiť AJAX vyhľadávanie',

# Multipage image navigation
'imgmultipageprev' => '&larr; predošlá stránka',
'imgmultipagenext' => 'ďalšia stránka &rarr;',
'imgmultigo'       => 'Vykonať',
'imgmultigoto'     => 'Prejsť na stránku $1',

# Table pager
'ascending_abbrev'         => 'vzostupne',
'descending_abbrev'        => 'zostupne',
'table_pager_next'         => 'Ďalšia stránka',
'table_pager_prev'         => 'Predošlá stránka',
'table_pager_first'        => 'Prvá stránka',
'table_pager_last'         => 'Posledná stránka',
'table_pager_limit'        => 'Zobraziť $1 položiek na stránku',
'table_pager_limit_submit' => 'Spustiť',
'table_pager_empty'        => 'Bez výsledkov',

# Auto-summaries
'autosumm-blank'   => 'Odstraňujem obsah stránky',
'autosumm-replace' => 'Nahrádzam stránku textom „$1“',
'autoredircomment' => 'Presmerovanie na [[$1]]',
'autosumm-new'     => 'Nová stránka: $1',

# Live preview
'livepreview-loading' => 'Načítava sa…',
'livepreview-ready'   => 'Načítavanie dokončené!',
'livepreview-failed'  => 'Živý náhľad sa nepodarilo zrealizovať!
Skúste obyčajný náhľad.',
'livepreview-error'   => 'Nepodarilo sa pripojiť: $1 „$2“
Skúste obyčajný náhľad.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Zmeny novšie ako $1 {{PLURAL:$1|sekunda|sekundy|sekúnd}} nemožno v tomto zozname zobraziť.',
'lag-warn-high'   => 'Z dôvodu dlhej odozvy databázového servera zmeny novšie ako $1 {{PLURAL:$1|sekunda|sekundy|sekúnd}} nemožno v tomto zozname zobraziť.',

# Watchlist editor
'watchlistedit-numitems'       => 'Váš zoznam sledovaných stránok obsahuje {{PLURAL:$1|jednu stránku|$1 stránky|$1 stránok}} nepočítajúc diskusné stránky.',
'watchlistedit-noitems'        => 'Váš zoznam sledovaných stránok obsahuje žiadne stránky.',
'watchlistedit-normal-title'   => 'Upraviť zoznam sledovaných stránok',
'watchlistedit-normal-legend'  => 'Odstrániť všetky stránky zo zoznamu sledovaných stránok',
'watchlistedit-normal-explain' => 'Nižšie sú zobrazené stránky z vášho zoznamu sledovaných stránok. Ak chcete odstrániť položku, začiarknite políčko vedľa nej a kliknite na Odstrániť položky. Tiež môžete [[Special:Watchlist/raw|upravovať nespracovaný zoznam]].',
'watchlistedit-normal-submit'  => 'Odstrániť položky',
'watchlistedit-normal-done'    => '{{PLURAL:$1|jedna položka bola odstránená|$1 položky boli odstránené|$1 položiek bolo odstránených}} z vášho zoznamu sledovaných stránok:',
'watchlistedit-raw-title'      => 'Upravovať nespracovaný zoznam sledovaných stránok',
'watchlistedit-raw-legend'     => 'Upravovať nespracovaný zoznam sledovaných stránok',
'watchlistedit-raw-explain'    => 'Nižšie sú zobrazené stránky z vášho zoznamu sledovaných stránok.
	Ak chcete upravovať položky, pridajte alebo odstráňte ich zo zoznamu;
	jednu stránku na riadok. Po skončení kliknite na Aktualizovať zoznam sledovaných stránok.
	Tiež môžete [[Special:Watchlist/edit|použiť štandardný editor]].',
'watchlistedit-raw-titles'     => 'Stránky:',
'watchlistedit-raw-submit'     => 'Aktualizovať zoznam sledovaných stránok',
'watchlistedit-raw-done'       => 'Váš zoznam sledovaných stránok bol aktualizovaný.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|Jedna položka bola pridaná|$1 položky boli pridané|$1 položiek bolo pridaných}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|Jedna položka bola odstránená|$1 položky boli odstránené|$1 položiek bolo odstránených}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Zobraziť súvisiace zmeny',
'watchlisttools-edit' => 'Zobraziť a upraviť zoznam sledovaných stránok',
'watchlisttools-raw'  => 'Upraviť nespracovaný zoznam sledovaných stránok',

# Core parser functions
'unknown_extension_tag' => 'Neznáma značka rozšírenia „$1“',

# Special:Version
'version'                          => 'Verzia', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Nainštalované rozšírenia',
'version-specialpages'             => 'Špeciálne stránky',
'version-parserhooks'              => 'Prípojné body syntaktického analyzátora',
'version-variables'                => 'Premenné',
'version-other'                    => 'Iné',
'version-mediahandlers'            => 'Obsluha multimédií',
'version-hooks'                    => 'Prípojné body',
'version-extension-functions'      => 'Rozširujúce funkcie',
'version-parser-extensiontags'     => 'Rozširujúce značky syntaxe',
'version-parser-function-hooks'    => 'Prípojné body funkcií syntaktického analyzátora',
'version-skin-extension-functions' => 'Rozširujúce funkcie vzhľadu',
'version-hook-name'                => 'Názov prípojného bodu',
'version-hook-subscribedby'        => 'Pripojené',
'version-version'                  => 'Verzia',
'version-license'                  => 'Licencia',
'version-software'                 => 'Nainštalovaný softvér',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Verzia',

# Special:FilePath
'filepath'         => 'Cesta k súboru',
'filepath-page'    => 'Súbor:',
'filepath-submit'  => 'Cesta',
'filepath-summary' => 'Táto špeciálna stránka vráti úplnú cestu k súboru.
Obrázky sa zobrazia v plnom rozlíšení, ostatné typy súborov sa spustia v príslušnom priradenom programe.

Zadajte názov súboru bez predpony „{{ns:image}}:“.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Hľadať duplicitné súbory',
'fileduplicatesearch-summary'  => 'Hľadanie duplicitných súborov na základe ich haš hodnôt.

Zadajte názov súboru bez predpony „{{ns:image}}:“.',
'fileduplicatesearch-legend'   => 'Hľadať duplicity',
'fileduplicatesearch-filename' => 'Názov súboru:',
'fileduplicatesearch-submit'   => 'Hľadať',
'fileduplicatesearch-info'     => '$1 × $2 pixelov<br />Veľkosť súboru: $3<br />Typ MIME: $4',
'fileduplicatesearch-result-1' => 'Súbor „$1“ nemá žiadny identický duplikát.',
'fileduplicatesearch-result-n' => 'Súbor „$1“ má {{PLURAL:$2|1 identický duplikát|$2 identické duplikáty|$2 identických duplikátov}}.',

# Special:SpecialPages
'specialpages'                   => 'Špeciálne stránky',
'specialpages-note'              => '----
* Bežné špeciálne stránky.
* <span class="mw-specialpagerestricted">Špeciálne stránky s obmedzeným prístupom.</span>',
'specialpages-group-maintenance' => 'Údržbové správy',
'specialpages-group-other'       => 'Iné špeciálne stránky',
'specialpages-group-login'       => 'Prihlásenie / registrácia',
'specialpages-group-changes'     => 'Posledné zmeny a záznamy',
'specialpages-group-media'       => 'Správy o multimédiách a nahrávaniach',
'specialpages-group-users'       => 'Používatelia a skupiny',
'specialpages-group-highuse'     => 'Často používané stránky',
'specialpages-group-pages'       => 'Zoznam stránok',
'specialpages-group-pagetools'   => 'Nástroje stránky',
'specialpages-group-wiki'        => 'Wiki údaje a nástroje',
'specialpages-group-redirects'   => 'Špeciálne stránky, ktoré sú presmerovania',
'specialpages-group-spam'        => 'Nástroje proti spamu',

# Special:BlankPage
'blankpage'              => 'Prázdna stránka',
'intentionallyblankpage' => 'Táto stránka je zámerne prázdna. Používa sa na meranie výkonnosti atď.',

);
