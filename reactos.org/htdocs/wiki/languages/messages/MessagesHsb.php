<?php
/** Upper Sorbian (Hornjoserbsce)
 *
 * @ingroup Language
 * @file
 *
 * @author Dundak
 * @author Michawiki
 * @author Tlustulimu
 * @author לערי ריינהארט
 */

$fallback = 'de';

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Specialnje',
	NS_MAIN             => '',
	NS_TALK             => 'Diskusija',
	NS_USER             => 'Wužiwar',
	NS_USER_TALK        => 'Diskusija_z_wužiwarjom',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_diskusija',
	NS_IMAGE            => 'Wobraz',
	NS_IMAGE_TALK       => 'Diskusija_k_wobrazej',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_diskusija',
	NS_TEMPLATE         => 'Předłoha',
	NS_TEMPLATE_TALK    => 'Diskusija_k_předłoze',
	NS_HELP             => 'Pomoc',
	NS_HELP_TALK        => 'Pomoc_diskusija',
	NS_CATEGORY         => 'Kategorija',
	NS_CATEGORY_TALK    => 'Diskusija_ke_kategoriji'
);

$specialPageAliases = array(
	'DoubleRedirects'           => array( "Dwójne_daleposrědkowanja" ),
	'BrokenRedirects'           => array( "Skóncowane_daleposrědkowanja" ),
	'Disambiguations'           => array( "Rozjasnjenja_wjazmyslnosće" ),
	'Userlogin'                 => array( "Přizwjewić" ),
	'Userlogout'                => array( "Wotzjewić" ),
	'Preferences'               => array( "Nastajenja" ),
	'Watchlist'                 => array( "Wobkedźbowanki" ),
	'Recentchanges'             => array( "Aktualne_změny" ),
	'Upload'                    => array( "Nahraće" ),
	'Imagelist'                 => array( "Dataje" ),
	'Newimages'                 => array( "Nowe_dataje" ),
	'Listusers'                 => array( "Wužiwarjo" ),
	'Statistics'                => array( "Statistika" ),
	'Randompage'                => array( "Připadna_strona" ),
	'Lonelypages'               => array( "Wosyroćene_strony" ),
	'Uncategorizedpages'        => array( "Njekategorizowane_strony" ),
	'Uncategorizedcategories'   => array( "Njekategorizowane_kategorije" ),
	'Uncategorizedimages'       => array( "Njekategorizowane_dataje" ),
	'Uncategorizedtemplates'    => array( "Njekategorizowane_předłohi" ),
	'Unusedcategories'          => array( "Njewužiwane_kategorije" ),
	'Unusedimages'              => array( "Njewužiwane_dataje" ),
	'Wantedpages'               => array( "Požadane_strony" ),
	'Wantedcategories'          => array( "Požadane_kategorije" ),
	'Mostlinked'                => array( "Z_najwjace_stronami_zwjazane_strony" ),
	'Mostlinkedcategories'      => array( "Najhusćišo_wužiwane_kategorije" ),
	'Mostlinkedtemplates'       => array( "Najhusćišo_wužiwane_předłohi" ),
	'Mostcategories'            => array( "Strony_z_najwjace_kategorijemi" ),
	'Mostimages'                => array( "Z_najwjace_stronami_zwjazane_dataje" ),
	'Mostrevisions'             => array( "Strony_z_najwjace_wersijemi" ),
	'Fewestrevisions'           => array( "Strony_z_najmjenje_wersijemi" ),
	'Shortpages'                => array( "Najkrótše_strony" ),
	'Longpages'                 => array( "Najdlěše_strony" ),
	'Newpages'                  => array( "Nowe_strony" ),
	'Ancientpages'              => array( "Najstarše_strony" ),
	'Deadendpages'              => array( "Strony_bjez_wotkazow" ),
	'Protectedpages'            => array( "Škitane_strony" ),
	'Allpages'                  => array( "Wšě_strony" ),
	'Prefixindex'               => array( "Prefiksindeks" ),
	'Ipblocklist'               => array( "Blokowane_IP-adresy" ),
	'Specialpages'              => array( "Specialne_strony" ),
	'Contributions'             => array( "Přinoški" ),
	'Emailuser'                 => array( "E-Mejl" ),
	'Whatlinkshere'             => array( "Lisćina_wotkazow" ),
	'Recentchangeslinked'       => array( "Změny_zwjazanych_stronow" ),
	'Movepage'                  => array( "Přesunyć" ),
	'Blockme'                   => array( "Blokowanje_proksijow" ),
	'Booksources'               => array( "Pytanje_po_ISBN" ),
	'Categories'                => array( "Kategorije" ),
	'Export'                    => array( "Eksport" ),
	'Version'                   => array( "Wersija" ),
	'Allmessages'               => array( "MediaWiki-zdźělenki" ),
	'Log'                       => array( "Protokol" ),
	'Blockip'                   => array( "Blokować" ),
	'Undelete'                  => array( "Wobnowić" ),
	'Import'                    => array( "Import" ),
	'Lockdb'                    => array( "Datowu_banku_zamknyć" ),
	'Unlockdb'                  => array( "Datowu_banku_wotamknyć" ),
	'Userrights'                => array( "Prawa" ),
	'MIMEsearch'                => array( "Pytanje_po_MIME" ),
	'Unwatchedpages'            => array( "Njewobkedźbowane_strony" ),
	'Listredirects'             => array( "Daleposrědkowanja" ),
	'Revisiondelete'            => array( "Wušmórnjenje_wersijow" ),
	'Unusedtemplates'           => array( "Njewužiwane_předłohi" ),
	'Randomredirect'            => array( "Připadne_daleposrědkowanje" ),
	'Mypage'                    => array( "Moja_wužiwarska_strona" ),
	'Mytalk'                    => array( "Moja_diskusijna_strona" ),
	'Mycontributions'           => array( "Moje_přinoški" ),
	'Listadmins'                => array( "Administratorojo" ),
	'Search'                    => array( "Pytać" ),
	'Withoutinterwiki'          => array( "Falowace_mjezyrěčne_wotkazy" ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Wotkazy podšmórnić:',
'tog-highlightbroken'         => 'Wotkazy na prózdne strony wuzběhnyć',
'tog-justify'                 => 'Tekst w blokowej sadźbje',
'tog-hideminor'               => 'Snadne změny w aktualnych změnach schować',
'tog-extendwatchlist'         => 'Rozšěrjena lisćina wobkedźbowankow',
'tog-usenewrc'                => 'Rozšěrjena lisćina aktualnych změnow (trjeba JavaScript)',
'tog-numberheadings'          => 'Nadpisma awtomatisce čisłować',
'tog-showtoolbar'             => 'Gratowu lajstu pokazać (JavaScript)',
'tog-editondblclick'          => 'Strony z dwójnym kliknjenjom wobdźěłować (JavaScript)',
'tog-editsection'             => 'Wobdźěłowanje jednotliwych wotrězkow přez wotkazy [wobdźěłać] zmóžnić',
'tog-editsectiononrightclick' => 'Wobdźěłowanje jednotliwych wotrězkow přez kliknjenje z prawej tastu<br />na nadpisma wotrězkow zmóžnić (JavaScript)',
'tog-showtoc'                 => 'Zapis wobsaha pokazać (za strony z wjace hač 3 nadpismami)',
'tog-rememberpassword'        => 'Hesło na tutym ličaku składować',
'tog-editwidth'               => 'Wobdźěłanske polo ma połnu šěrokosć',
'tog-watchcreations'          => 'Strony, kotrež wutworjam, swojim wobkedźbowankam přidać',
'tog-watchdefault'            => 'Strony, kotrež wobdźěłuju, swojim wobkedźbowankam přidać',
'tog-watchmoves'              => 'Sam přesunjene strony wobkedźbowankam přidać',
'tog-watchdeletion'           => 'Sam wušmórnjene strony wobkedźbowankam přidać',
'tog-minordefault'            => 'Wšě změny zwoprědka jako snadne woznamjenić',
'tog-previewontop'            => 'Přehlad nad wobdźěłanskim polom pokazać',
'tog-previewonfirst'          => 'Do składowanja přeco přehlad pokazać',
'tog-nocache'                 => 'Pufrowanje stronow znjemóžnić',
'tog-enotifwatchlistpages'    => 'E-mejlku pósłać, hdyž so strona z wobkedźbowankow změni',
'tog-enotifusertalkpages'     => 'Mejlku pósłać, hdyž so moja wužiwarska diskusijna strona změni',
'tog-enotifminoredits'        => 'Tež dla snadnych změnow mejlki pósłać',
'tog-enotifrevealaddr'        => 'Moju e-mejlowu adresu w e-mejlowych zdźělenkach wotkryć',
'tog-shownumberswatching'     => 'Ličbu wobkedźbowacych wužiwarjow pokazać',
'tog-fancysig'                => 'Hrube signatury (bjez awtomatiskeho wotkaza)',
'tog-externaleditor'          => 'Eksterny editor jako standard wužiwać (jenož za ekspertow, žada sej specialne nastajenja na wašim ličaku)',
'tog-externaldiff'            => 'Eksterny diff-program jako standard wužiwać (jenož za ekspertow, žada sej specialne nastajenja na wašim ličaku)',
'tog-showjumplinks'           => 'Wotkazy typa „dźi do” zmóžnić',
'tog-uselivepreview'          => 'Live-přehlad wužiwać (JavaScript) (eksperimentalnje)',
'tog-forceeditsummary'        => 'Mje skedźbnić, jeli zabudu zjeće',
'tog-watchlisthideown'        => 'Moje změny we wobkedźbowankach schować',
'tog-watchlisthidebots'       => 'Změny awtomatiskich programow (botow) we wobkedźbowankach schować',
'tog-watchlisthideminor'      => 'Snadne změny we wobkedźbowankach schować',
'tog-nolangconversion'        => 'Konwertowanje rěčnych wariantow znjemóžnić',
'tog-ccmeonemails'            => 'Kopije mejlkow dóstać, kiž druhim wužiwarjam pósćelu',
'tog-diffonly'                => 'Jenož rozdźěle pokazać (nic pak zbytny wobsah)',
'tog-showhiddencats'          => 'Schowane kategorije pokazać',

'underline-always'  => 'přeco',
'underline-never'   => 'ženje',
'underline-default' => 'po standardźe wobhladowaka',

'skinpreview' => '(Přehlad)',

# Dates
'sunday'        => 'Njedźela',
'monday'        => 'Póndźela',
'tuesday'       => 'Wutora',
'wednesday'     => 'Srjeda',
'thursday'      => 'Štwórtk',
'friday'        => 'Pjatk',
'saturday'      => 'Sobota',
'sun'           => 'Nje',
'mon'           => 'Pón',
'tue'           => 'Wut',
'wed'           => 'Srj',
'thu'           => 'Štw',
'fri'           => 'Pja',
'sat'           => 'Sob',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'měrc',
'april'         => 'apryla',
'may_long'      => 'meja',
'june'          => 'junij',
'july'          => 'julij',
'august'        => 'awgust',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'nowember',
'december'      => 'december',
'january-gen'   => 'januara',
'february-gen'  => 'februara',
'march-gen'     => 'měrca',
'april-gen'     => 'apryla',
'may-gen'       => 'meje',
'june-gen'      => 'junija',
'july-gen'      => 'julija',
'august-gen'    => 'awgusta',
'september-gen' => 'septembra',
'october-gen'   => 'oktobra',
'november-gen'  => 'nowembra',
'december-gen'  => 'decembra',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'měr',
'apr'           => 'apr',
'may'           => 'meje',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'awg',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'now',
'dec'           => 'dec',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorija|Kategoriji|Kategorije|Kategorije}}',
'category_header'                => 'Nastawki w kategoriji „$1”',
'subcategories'                  => 'Podkategorije',
'category-media-header'          => 'Dataje w kategoriji „$1”',
'category-empty'                 => "''Tuta kategorija tuchwilu žane nastawki abo medije njewobsahuje.''",
'hidden-categories'              => '{{PLURAL:$1|Schowana kategorija|Schowanej kategoriji|Schowane kategorije|Schowanych kategorijow}}',
'hidden-category-category'       => 'Schowane kategorije', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Tuta kategorija ma jenož slědowacu podkategoriju.|Tuta kategorija ma {{PLURAL:$1|slědowacu podkategoriju|$1 slědowacej podkategoriji|$1 slědowace podkategorije|$1 slědowacych podkategorijow}} z dohromady $2.}}',
'category-subcat-count-limited'  => 'Tuta kategorija ma {{PLURAL:$1|slědowacu podkategoriju|slědowacej $1 podkategoriji|slědowace $1 podkategorije|slědowacych $1 podkategorijow}}:',
'category-article-count'         => '{{PLURAL:$2|Tuta kategorija wobsahuje jenož slědowacu stronu.|{{PLURAL:$1|Slědowaca strona je|Slědowacej $1 stronje stej|Slědowace $1 strony su|Slědowacych $1 stronow je}} w tutej kategoriji z dohromady $2.}}',
'category-article-count-limited' => '{{PLURAL:$1|Slědowaca strona je|Slědowacej $1 stronje stej|Slědowace $1 strony su|Slědowacych $1 stronow je}} w tutej kategoriji:',
'category-file-count'            => '{{PLURAL:$2|Tuta kategorija wobsahuje jenož slědowacu stronu.|{{PLURAL:$1|Slědowaca dataja je|Slědowacej $1 dataji stej|Slědowace $1 dataje|Slědowacych $1 datajow je}} w tutej kategoriji z dohromady $2.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Slědowaca dataj je|Slědowacej $1 dataji stej|Slědowace $1 dataje su|Slědowacych $1 je}} w tutej kategoriji:',
'listingcontinuesabbrev'         => ' (pokročowane)',

'mainpagetext'      => '<big><b>MediaWiki bu wuspěšnje instalowany.</b></big>',
'mainpagedocfooter' => 'Prošu hlej [http://meta.wikimedia.org/wiki/Help:Contents dokumentaciju] za informacije wo wužiwanju softwary.

== Za nowačkow ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Wo nastajenjach]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]',

'about'          => 'Wo',
'article'        => 'Nastawk',
'newwindow'      => '(wočinja so w nowym woknje)',
'cancel'         => 'Přetorhnyć',
'qbfind'         => 'Namakać',
'qbbrowse'       => 'Přepytować',
'qbedit'         => 'wobdźěłać',
'qbpageoptions'  => 'stronu',
'qbpageinfo'     => 'Kontekst',
'qbmyoptions'    => 'Moje strony',
'qbspecialpages' => 'Specialne strony',
'moredotdotdot'  => 'Wjace…',
'mypage'         => 'Moja strona',
'mytalk'         => 'Moja diskusija',
'anontalk'       => 'Z tutej IP diskutować',
'navigation'     => 'Nawigacija',
'and'            => 'a',

# Metadata in edit box
'metadata_help' => 'Metadaty:',

'errorpagetitle'    => 'Zmylk',
'returnto'          => 'Wróćo k stronje $1.',
'tagline'           => 'z {{GRAMMAR:genitiw|{{SITENAME}}}}',
'help'              => 'Pomoc',
'search'            => 'Pytać',
'searchbutton'      => 'Pytać',
'go'                => 'Nastawk',
'searcharticle'     => 'Nastawk',
'history'           => 'stawizny',
'history_short'     => 'stawizny',
'updatedmarker'     => 'Změny z mojeho poslednjeho wopyta',
'info_short'        => 'Informacija',
'printableversion'  => 'Ćišćomna wersija',
'permalink'         => 'Trajny wotkaz',
'print'             => 'Ćišćeć',
'edit'              => 'wobdźěłać',
'create'            => 'Wutworić',
'editthispage'      => 'Stronu wobdźěłać',
'create-this-page'  => 'Stronu wudźěłać',
'delete'            => 'Wušmórnyć',
'deletethispage'    => 'Stronu wušmórnyć',
'undelete_short'    => '{{PLURAL:$1|jednu wersiju|$1 wersiji|$1 wersije|$1 wersijow}} wobnowić',
'protect'           => 'Škitać',
'protect_change'    => 'změnić',
'protectthispage'   => 'Stronu škitać',
'unprotect'         => 'Škit zběhnyć',
'unprotectthispage' => 'Škit strony zběhnyć',
'newpage'           => 'Nowa strona',
'talkpage'          => 'diskusija',
'talkpagelinktext'  => 'diskusija',
'specialpage'       => 'Specialna strona',
'personaltools'     => 'Wosobinske nastroje',
'postcomment'       => 'Komentar dodać',
'articlepage'       => 'Nastawk',
'talk'              => 'diskusija',
'views'             => 'Zwobraznjenja',
'toolbox'           => 'Nastroje',
'userpage'          => 'Wužiwarsku stronu pokazać',
'projectpage'       => 'Projektowu stronu pokazać',
'imagepage'         => 'Wobrazowu stronu pokazać',
'mediawikipage'     => 'Zdźělenku pokazać',
'templatepage'      => 'Předłohu pokazać',
'viewhelppage'      => 'Pomocnu stronu pokazać',
'categorypage'      => 'Kategoriju pokazać',
'viewtalkpage'      => 'Diskusiju pokazać',
'otherlanguages'    => 'W druhich rěčach',
'redirectedfrom'    => '(ze strony „$1” sposrědkowane)',
'redirectpagesub'   => 'Daleposrědkowanje',
'lastmodifiedat'    => 'Strona bu posledni raz dnja $1 w $2 hodź. změnjena.', # $1 date, $2 time
'viewcount'         => 'Strona bu {{PLURAL:$1|jónu|dwójce|$1 razy|$1 razow}} wopytana.',
'protectedpage'     => 'Škitana strona',
'jumpto'            => 'Dźi do:',
'jumptonavigation'  => 'Nawigacija',
'jumptosearch'      => 'Pytać',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Wo {{GRAMMAR:lokatiw|{{SITENAME}}}}',
'aboutpage'            => 'Project:Wo',
'bugreports'           => 'Zmylkowe wopisanja',
'bugreportspage'       => 'Project:Zmylkowe wopisanja',
'copyright'            => 'Wobsah steji pod $1.',
'copyrightpagename'    => '{{SITENAME}} awtorske prawa',
'copyrightpage'        => '{{ns:project}}:Awtorske prawa',
'currentevents'        => 'Aktualne podawki',
'currentevents-url'    => 'Project:Aktualne podawki',
'disclaimers'          => 'Licencne postajenja',
'disclaimerpage'       => 'Project:Impresum',
'edithelp'             => 'Pomoc za wobdźěłowanje',
'edithelppage'         => 'Help:Wobdźěłowanje',
'faq'                  => 'Husto stajene prašenja (FAQ)',
'faqpage'              => 'Project:Husto stajene prašenja (FAQ)',
'helppage'             => 'Help:Wobsah',
'mainpage'             => 'Hłowna strona',
'mainpage-description' => 'Hłowna strona',
'policy-url'           => 'Project:Směrnicy',
'portal'               => 'Portal {{GRAMMAR:genitiw|{{SITENAME}}}}',
'portal-url'           => 'Project:Portal',
'privacy'              => 'Škit datow',
'privacypage'          => 'Project:Škit datow',

'badaccess'        => 'Nimaš wotpowědne dowolnosće',
'badaccess-group0' => 'Nimaš wotpowědne dowolnosće za tutu akciju.',
'badaccess-group1' => 'Tuta akcija da so jenož wot wužiwarjow skupiny $1 wuwjesć.',
'badaccess-group2' => 'Tuta akcija da so jenož wot wužiwarjow skupin $1 wuwjesć.',
'badaccess-groups' => 'Tuta akcija da so jenož wot wužiwarjow skupin $1 wuwjesć.',

'versionrequired'     => 'Wersija $1 softwary MediaWiki trěbna',
'versionrequiredtext' => 'Wersija $1 MediaWiki je trěbna, zo by so tuta strona wužiwać móhła. Hlej [[Special:Version|wersijowu stronu]]',

'ok'                      => 'W porjadku',
'retrievedfrom'           => 'Z {{GRAMMAR:genitiw|$1}}',
'youhavenewmessages'      => 'Maš $1 ($2).',
'newmessageslink'         => 'nowe powěsće',
'newmessagesdifflink'     => 'poslednja změna',
'youhavenewmessagesmulti' => 'Maš nowe powěsće: $1',
'editsection'             => 'wobdźěłać',
'editold'                 => 'wobdźěłać',
'viewsourceold'           => 'Žórło wobhladać',
'editsectionhint'         => 'Wotrězk wobdźěłać: $1',
'toc'                     => 'Wobsah',
'showtoc'                 => 'pokazać',
'hidetoc'                 => 'schować',
'thisisdeleted'           => '$1 pokazać abo wobnowić?',
'viewdeleted'             => '$1 pokazać?',
'restorelink'             => '{{PLURAL:$1|1 wušmórnjenu wersiju|$1 wušmórnjenej wersiji|$1 wušmórnjene wersije|$1 wušmórnjenych wersijow}}',
'feedlinks'               => 'Kanal:',
'feed-invalid'            => 'Njepłaćiwy typ abonementa.',
'feed-unavailable'        => 'Syndikaciske kanale na {{GRAMMAR:lokatiw|{{SITENAME}}}} k dispoziciji njesteja',
'site-rss-feed'           => '$1 RSS kanal',
'site-atom-feed'          => 'Atom-kanal za $1',
'page-rss-feed'           => 'RSS-kanal za „$1“',
'page-atom-feed'          => 'Atom-Kanal za „$1“',
'red-link-title'          => '$1 (strona hišće njepisana)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Nastawk',
'nstab-user'      => 'Wužiwarska strona',
'nstab-media'     => 'Medije',
'nstab-special'   => 'Specialna strona',
'nstab-project'   => 'Projektowa strona',
'nstab-image'     => 'Dataja',
'nstab-mediawiki' => 'Zdźělenka',
'nstab-template'  => 'Předłoha',
'nstab-help'      => 'Pomoc',
'nstab-category'  => 'Kategorija',

# Main script and global functions
'nosuchaction'      => 'Žana tajka akcija',
'nosuchactiontext'  => 'Akcija podata z URL njebu wot wikija připóznata.',
'nosuchspecialpage' => 'Tuta specialna strona njeeksistuje.',
'nospecialpagetext' => "<big>'''Tuta specialna strona wikiju njeeksistuje.'''</big>

Lisćina płaćiwych specialnych stronow hodźi so pod [[Special:SpecialPages|Lis´cina specialnych stronow]] namakać.",

# General errors
'error'                => 'Zmylk',
'databaseerror'        => 'Zmylk w datowej bance',
'dberrortext'          => 'Syntaktiski zmylk při wotprašowanju datoweje banki.
To móhło bug w programje być. Poslednje spytane wotprašenje w datowej bance běše:
<blockquote><tt>$1</tt></blockquote>
z funkcije „<tt>$2</tt>”.
MySQL wróći zmylk „<tt>$3: $4</tt>”.',
'dberrortextcl'        => 'Syntaktiski zmylk je we wotprašowanju datoweje banki wustupił.
Poslednje wotprašenje w datowej bance běše:
„$1”
z funkcije „$2”.
MySQL wróći zmylk „$3: $4”.',
'noconnect'            => 'Wiki ma techniske problemy a njemóže ze serwerom datoweje banki zwjazać.<br />
$1',
'nodb'                 => 'Datowa banka $1 njeda so wubrać',
'cachederror'          => 'Naslědne je kopija z pufrowaka a njemóhło hižo aktualne być.',
'laggedslavemode'      => 'Kedźbu: Je móžno, zo strona žane zaktualizowanja njewobsahuje.',
'readonly'             => 'Datowa banka je zawrjena',
'enterlockreason'      => 'Zapodaj přičinu za zawrjenje a přibližny čas, hdy budźe zawrjenje zběhnjene',
'readonlytext'         => 'Datowa banka je tuchwilu za nowe zapiski a druhe změny zawrjena, najskerje wothladowanskich dźěłow dla; po jich zakónčenju budźe wšitko zaso normalne.

Administrator, kiž je datowu banku zawrěł, je jako přičinu podał: $1',
'missing-article'      => 'Datowa banka njenamaka tekst strony z mjenom "$1" $2, kotryž dyrbjał so namakać.

To so zwjetša zawinuje, hdyž so njepłaćiwa změna abo zapisk stawiznow na stronu wotkazuje, kotraž bu wušmórnjena.

Jeli to njetrjechi, sy najskerje programowu zmólku w softwarje namakał.
Zdźěl to prošu [[Special:ListUsers/sysop|admininistratorej]] podawajo wotpowědny URL.',
'missingarticle-rev'   => '(Wersijowe čisło: $1)',
'missingarticle-diff'  => '(Rozdźěl: $1, $2)',
'readonly_lag'         => 'Datowa banka bu awtomatisce zawrjena, mjeztym zo pospytuja wotwisne serwery datowych bankow  hłowny serwer docpěć',
'internalerror'        => 'Znutřkowny zmylk',
'internalerror_info'   => 'Znutřkowny zmylk: $1',
'filecopyerror'        => 'Njebě móžno dataju „$1” k „$2” kopěrować.',
'filerenameerror'      => 'Njebě móžno dataju „$1” na „$2” přemjenować.',
'filedeleteerror'      => 'Njebě móžno dataju „$1” wušmórnyć.',
'directorycreateerror' => 'Zapis „$1“ njeda so wutworić.',
'filenotfound'         => 'Njebě móžno dataju „$1” namakać.',
'fileexistserror'      => 'Njebě móžno do dataje „$1“ pisać, dokelž tuta dataja hižo eksistuje.',
'unexpected'           => 'Njewočakowana hódnota: "$1"="$2".',
'formerror'            => 'Zmylk: njeje móžno formular wotesłać',
'badarticleerror'      => 'Tuta akcija njeda so na tutej stronje wuwjesć.',
'cannotdelete'         => 'Njeje móžno podatu stronu abo dataju wušmórnyć. (Po zdaću je to hižo něchtó druhi činił.)',
'badtitle'             => 'Wopačny titul',
'badtitletext'         => 'Požadane mjeno strony běše njepłaćiwy, prózdny abo njekorektny titul z mjezyrěcneho abo interwikijoweho wotkaza. Snano wobsahuje jedne znamješko abo wjacore znamješka, kotrež w titulach dowolene njejsu.',
'perfdisabled'         => 'Wodaj! Tuta funkcija bu nachwilnje znjemóžnjena, dokelž datowu banku tak spomaluje, zo hižo nichtó wiki wužiwać njemóže.',
'perfcached'           => 'Sćěhowace daty z pufrowaka pochadźeja a snano cyle aktualne njejsu.',
'perfcachedts'         => 'Sćěhowace daty su z pufrowaka a buchu $1 posledni raz zaktualizowane.',
'querypage-no-updates' => "'''Aktualizacija za tutu stronu je tuchwilu znjemóžnjena. Daty so hač na dalše njewobnowjeja.'''",
'wrong_wfQuery_params' => 'Njeprawe parametry za wfQuery()

Funkcija: $1

Wotprašenje: $2',
'viewsource'           => 'Žórło wobhladać',
'viewsourcefor'        => 'za $1',
'actionthrottled'      => 'Akcije wobmjezowane',
'actionthrottledtext'  => 'Jako připrawa přećiwo spamej, je častosć wuwjedźenja tuteje akcije w krótkej dobje wobmjezowana a ty sy tutón limit překročił. Prošu spytaj za něšto mjeńšiny hišće raz.',
'protectedpagetext'    => 'Strona je přećiwo wobdźěłowanju škitana.',
'viewsourcetext'       => 'Móžeš pak jeje žórło wobhladać a jo kopěrować:',
'protectedinterface'   => 'Tuta strona skići tekst za rěčny zwjerch a je škitana zo by so znjewužiwanju zadźěwało.',
'editinginterface'     => '<b>Kedźbu:</b> Wobdźěłuješ stronu, kotraž wobsahuje tekst za rěčny zwjerch. Změny wuskutkuja so bjezposrědnje za wšěch druhich wužiwarjow tutoho rěčneho zwjercha.',
'sqlhidden'            => '(SQL wotprašenje schowane)',
'cascadeprotected'     => 'Strona je za wobdźěłowanje zawrjena, dokelž je w {{PLURAL:$1|slědowacej stronje|slědowacymaj stronomaj|slědowacych stronach}} zapřijata, {{PLURAL:$1|kotraž je|kotrejž stej|kotrež su}} přez kaskadowu opciju {{PLURAL:$1|škitana|škitanej|škitane}}:
$2',
'namespaceprotected'   => "Nimaš dowolnosć, zo by stronu w mjenowym rumje '''$1''' wobdźěłał.",
'customcssjsprotected' => 'Nimaš prawo, zo by tutu stronu wobdźěłał, dokelž wosobinske nastajenja druheho wužiwarja wobsahuje.',
'ns-specialprotected'  => 'Specialne strony njedadźa so wobdźěłać.',
'titleprotected'       => "Tutón titul bu přećiwo wutworjenju přez [[User:$1|$1]] škitany.
Podata přičina je ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Špatna konfiguracija: njeznaty wirusowy skener: <i>$1</i>',
'virus-scanfailed'     => 'Skenowanje njeporadźiło (kode $1)',
'virus-unknownscanner' => 'njeznaty antiwirus:',

# Login and logout pages
'logouttitle'                => 'Wotzjewjenje',
'logouttext'                 => '<strong>Sy nětko wotzjewjeny.</strong><br />
Móžeš {{GRAMMAR:akuzatiw|{{SITENAME}}}} nětko anonymnje dale wužiwać abo so ze samsnym abo druhim wužiwarskim mjenom zaso přizjewić. Wobkedźbuj zo so někotre strony dale jewja kaž by hišće přizjewjeny był doniž pufrowak swojeho wobhladowaka njewuprózdnješ.',
'welcomecreation'            => '== Witaj, $1! ==

Twoje konto bu wutworjene. Njezabudź swoje nastajenja za [[Special:Preferences|{{GRAMMAR:akuzatiw|{{SITENAME}}}}]] změnić.',
'loginpagetitle'             => 'Přizjewjenje',
'yourname'                   => 'Wužiwarske mjeno',
'yourpassword'               => 'Hesło',
'yourpasswordagain'          => 'Hesło znowa zapodać',
'remembermypassword'         => 'Hesło na tutym ličaku sej spomjatkować',
'yourdomainname'             => 'Twoja domejna',
'externaldberror'            => 'Běše pak eksterny zmylk awtentifikacije datoweje banki, pak njesměš swoje eksterne konto aktualizować.',
'loginproblem'               => '<b>Běše problem z přizjewjenjom.</b><br />

Prošu spytaj hišće raz!',
'login'                      => 'Přizjewić',
'nav-login-createaccount'    => 'Konto wutworić abo so přizjewić',
'loginprompt'                => 'Zo by so pola {{GRAMMAR:genitiw|{{SITENAME}}}} přizjewić móhł, dyrbja so placki (cookies) zmóžnić.',
'userlogin'                  => 'Konto wutworić abo so přizjewić',
'logout'                     => 'Wotzjewić',
'userlogout'                 => 'Wotzjewić',
'notloggedin'                => 'Njepřizjewjeny',
'nologin'                    => 'Nimaš žane konto? $1.',
'nologinlink'                => 'Tu móžeš wužiwarske konto wutworić',
'createaccount'              => 'Wužiwarske konto wutworić',
'gotaccount'                 => 'Maš hižo wužiwarske konto? $1.',
'gotaccountlink'             => 'Přizjewić',
'createaccountmail'          => 'z mejlku',
'badretype'                  => 'Hesle, kotrejž sy zapodał, so njekryjetej.',
'userexists'                 => 'Wužiwarske mjeno, kotrež sy zapodał, so hižo wužiwa. Wubjer druhe mjeno.',
'youremail'                  => 'E-mejl *:',
'username'                   => 'Wužiwarske mjeno:',
'uid'                        => 'ID wužiwarja:',
'prefs-memberingroups'       => 'Čłon {{PLURAL:$1|wužiwarskeje skupiny|wužiwarskeju skupinow|wužiwarskich skupinow|wužiwarskich skupinow}}:',
'yourrealname'               => 'Woprawdźite mjeno *',
'yourlanguage'               => 'Rěč:',
'yourvariant'                => 'Warianta:',
'yournick'                   => 'Podpis:',
'badsig'                     => 'Njepłaćiwa signatura, prošu HTML přepruwować.',
'badsiglength'               => 'Podpis smě maksimalnje $1 {{PLURAL:$1|znamješko|znamješce|znamješka|znamješkow}} dołhi być.',
'email'                      => 'E-mejl',
'prefs-help-realname'        => '* Woprawdźite mjeno (opcionalne): jeli so rozsudźiš to zapodać, budźe to so wužiwać, zo by tebi woprawnjenje za twoje dźěło dało.',
'loginerror'                 => 'Zmylk při přizjewjenju',
'prefs-help-email'           => 'E-mejlowa adresa je opcionalna, ale zmóžnja ći nowe hesło emejlować, jeli sy swoje hesło zabył. Móžeš tež druhim dowolić, će přez swoju wužiwarsku abo diskusijnu stronu skontaktować, bjeztoho zo by dyrbjał swoju identitu wotkrył.',
'prefs-help-email-required'  => 'Je płaćiwa emejlowa adresa trjeba.',
'nocookiesnew'               => 'Wužiwarske konto bu wutworjene, njejsy pak přizjewjeny. {{SITENAME}} wužiwa placki (cookies), zo bychu so wužiwarjo přizjewili. Sy placki znjemóžnił. Prošu zmóžń je a přizjew so potom ze swojim nowym wužiwarskim mjenom a hesłom.',
'nocookieslogin'             => '{{SITENAME}} wužiwa placki (cookies) za přizjewjenje wužiwarjow wužiwa. Sy placki znjemóžnił. Prošu zmóžń je a spytaj hišće raz.',
'noname'                     => 'Njejsy płaćiwe wužiwarske mjeno podał.',
'loginsuccesstitle'          => 'Přizjewjenje wuspěšne',
'loginsuccess'               => '<b>Sy nětko jako „$1” w {{GRAMMAR:lokatiw|{{SITENAME}}}} přizjewjeny.</b>',
'nosuchuser'                 => 'Njeje wužiwar z mjenom "$1". Přepruwuj swój prawopis abo [[Special:Userlogin/signup|wutwor nowe konto]].',
'nosuchusershort'            => 'Wužiwarske mjeno „<nowiki>$1</nowiki>” njeeksistuje. Prošu přepruwuj prawopis.',
'nouserspecified'            => 'Dyrbiš wužiwarske mjeno podać',
'wrongpassword'              => 'Hesło, kotrež sy zapodał, je wopačne. Prošu spytaj hišće raz.',
'wrongpasswordempty'         => 'Hesło, kotrež sy zapodał, běše prózdne. Prošu spytaj hišće raz.',
'passwordtooshort'           => 'Hesło je překrótke. Dyrbi znajmjeńša $1 {{PLURAL:$1|znamješko|znamješce|znamješka|znamješkow}} měć.',
'mailmypassword'             => 'Nowe hesło e-mejlować',
'passwordremindertitle'      => 'Skedźbnjenje na hesło z {{GRAMMAR:genitiw|{{SITENAME}}}}',
'passwordremindertext'       => 'Něchtó z IP-adresu $1 (najskerje ty) je wo nowe hesło za přizjewjenje za {{GRAMMAR:Akuzatiw|{{SITENAME}}}} ($4) prosył. Nachwilne hesło za wužiwarja "$2" je so wutworiło a je nětko "$3". Jeli je to twój wotpohlad było dyrbiš so nětko přizjewić a now hesło wubrać.

Jeli něchtó druhi wo nowe hesło prosył abo ty sy so zaso na swoje hesło dopomnił a hižo nochceš je změnić, móžeš tutu powěsć ignorować a swoje stare hesło dale wužiwać.',
'noemail'                    => 'Za wužiwarja $1 žana e-mejlowa adresa podata njeje.',
'passwordsent'               => 'Nowe hesło bu na e-mejlowu adresu zregistrowanu za wužiwarja „$1” pósłane.
Prošu přizjew so znowa, po tym zo sy je přijał.',
'blocked-mailpassword'       => 'Twoja IP-adresa bu blokowana; tohodla njeje dowolene, nowe hesło požadać, zo by so znjewužiwanju zadźěwało.',
'eauthentsent'               => 'Wobkrućenska mejlka bu na naspomnjenu e-mejlowu adresu pósłana.
Prjedy hač so druha mejlka ke kontu pósćele, dyrbiš so po instrukcijach w mejlce měć, zo by wobkrućił, zo konto je woprawdźe twoje.',
'throttled-mailpassword'     => 'Bu hižo nowe hesło za {{PLURAL:$1|poslednju hodźinu|poslednjej $1 hodźinje|poslednje $1 hodźiny|poslednich $1 hodźin}} pósłane. Zo by znjewužiwanju zadźěwało, so jenož jedne hesło na {{PLURAL:$1|hodźinu|$1 hodźinje|$1 hodźiny|$1 hodźinow}} pósćele.',
'mailerror'                  => 'Zmylk při słanju mejlki: $1',
'acct_creation_throttle_hit' => 'Wodaj, sy hižo $1 {{PLURAL:$1|konto|kontaj|konty|kontow}} wutworił. Njemóžeš dalše wutworić.',
'emailauthenticated'         => 'Twoja e-mejlowa adresa bu $1 wobkrućena.',
'emailnotauthenticated'      => 'Twoja e-mejlowa adresa hišće wobkrućena <strong>njeje</strong>. Žane mejlki za jednu z sćěhowacych funkcijow pósłane njebudu.',
'noemailprefs'               => 'Podaj e-mejlowu adresu za tute funkcije, zo bychu fungowali.',
'emailconfirmlink'           => 'Wobkruć swoju e-mejlowu adresu',
'invalidemailaddress'        => 'E-mejlowa adresa so njeakceptuje, dokelž ma po zdaću njepłaćiwy format. Prošu zapodaj płaćiwu adresu abo wuprózdń te polo.',
'accountcreated'             => 'Wužiwarske konto wutworjene',
'accountcreatedtext'         => 'Wužiwarske konto za $1 bu wutworjene.',
'createaccount-title'        => 'Wutworjenje wužiwarskeho konta za {{SITENAME}}',
'createaccount-text'         => 'Něchtó je wužiwarske konto za twoju e-mejlowu adresu na {{SITENAME}} ($4) z mjenom "$2" z hesłom "$3" wutworił. Ty měł so nětko přizjewić a swoje hesło změnić.

Móžeš tutu zdźělenku ignorować, jeli so wužiwarske konto zmylnje wutworiło.',
'loginlanguagelabel'         => 'Rěč: $1',

# Password reset dialog
'resetpass'               => 'Hesło za wužiwarske konto wróćo stajić',
'resetpass_announce'      => 'Sy so z nachwilnym e-mejlowanym hesłom přizjewił. Zo by přizjewjenje zakónčił, dyrbiš nětko nowe hesło postajić.',
'resetpass_text'          => '<!-- Tu tekst zasunyć -->',
'resetpass_header'        => 'Hesło wróćo stajić',
'resetpass_submit'        => 'Hesło posrědkować a so přizjewić',
'resetpass_success'       => 'Twoje hesło bu wuspěšnje změnjene! Nětko přizjewjenje běži...',
'resetpass_bad_temporary' => 'Njepłaćiwe nachwilne hesło. Snano sy swoje hesło hižo wuspěšnje změnił abo nowe nachwilne hesło požadał.',
'resetpass_forbidden'     => 'Hesła njehodźa so we {{SITENAME}} změnić.',
'resetpass_missing'       => 'Prózdny formular.',

# Edit page toolbar
'bold_sample'     => 'Tučny tekst',
'bold_tip'        => 'Tučny tekst',
'italic_sample'   => 'Kursiwny tekst',
'italic_tip'      => 'Kursiwny tekst',
'link_sample'     => 'Mjeno wotkaza',
'link_tip'        => 'Znutřkowny wotkaz',
'extlink_sample'  => 'http://www.example.com Mjeno wotkaza',
'extlink_tip'     => 'Zwonkowny wotkaz (pomysli sej na prefiks http://)',
'headline_sample' => 'Nadpismo',
'headline_tip'    => 'Nadpismo runiny 2',
'math_sample'     => 'Zasuń tu formulu',
'math_tip'        => 'Matematiska formula (LaTeX)',
'nowiki_sample'   => 'Zasuń tu njeformatowany tekst',
'nowiki_tip'      => 'Wiki-syntaksu ignorować',
'image_sample'    => 'Přikład.jpg',
'image_tip'       => 'Zasadźeny wobraz',
'media_sample'    => 'Přikład.ogg',
'media_tip'       => 'Wotkaz k medijowej dataji',
'sig_tip'         => 'Twoja signatura z časowym kołkom',
'hr_tip'          => 'Wodoruna linija (zrědka wužiwać!)',

# Edit pages
'summary'                          => 'Zjeće',
'subject'                          => 'Tema/Nadpismo',
'minoredit'                        => 'Snadna změna',
'watchthis'                        => 'Stronu wobkedźbować',
'savearticle'                      => 'Składować',
'preview'                          => 'Přehlad',
'showpreview'                      => 'Přehlad pokazać',
'showlivepreview'                  => 'Hnydomny přehlad',
'showdiff'                         => 'Změny pokazać',
'anoneditwarning'                  => '<b>Kedźbu:</b> Njejsy přizjewjeny. Změny so z twojej IP-adresu składuja.',
'missingsummary'                   => '<b>Kedźbu:</b> Njejsy žane zjeće zapodał. Jeli hišće raz na „Składować” kliknješ so twoje změny bjez komentara składuja.',
'missingcommenttext'               => 'Prošu zapodaj zjeće.',
'missingcommentheader'             => '<b>Kedźbu:</b> Njejsy nadpis za tutón komentar podał. Jeli na „Składować” kliknješ, składuje so twoja změna bjez nadpisa.',
'summary-preview'                  => 'Přehlad zjeća',
'subject-preview'                  => 'Přehlad temy',
'blockedtitle'                     => 'Wužiwar je zablokowany',
'blockedtext'                      => "<big>'''Twoje wužiwarske mjeno abo twoja IP-adresa bu zablokowane.'''</big>

Blokowar je $1.
Podata přičina je ''$2''.

* Spočatk blokowanja: $8
* Kónc blokowanja: $6
* Zablokowany wužiwar: $7

Móžeš $1 abo druheho [[{{MediaWiki:Grouppage-sysop}}|administratora]] kontaktować, zo by wo blokowanju diskutował.
Njemóžeš 'e-mejlowu funkciju' wužiwać, chibazo sy płaćiwu e-mejlowu adresu w swojich [[Special:Preferences|kontowych nastajenjach]] podał a njebu přećiwo jeje wužiwanju zablokowany.
Twoja tuchwilna IP-adresa je $3 a blokowanski ID je #$5. Prošu podaj wšě horjeka naspomnjene podrobnosće w swojich naprašowanjach.",
'autoblockedtext'                  => 'Twoja IP-adresa bu awtomatisce blokowana, dokelž ju druhi wužiwar wužiwaše, kiž bu wot $1 zablokowany.
Přičina blokowanja bě:

:\'\'$2\'\'

* Započatk blokowanja: $8
* Kónc blokowanja: $6
* Zablokowany wužiwar: $7

Móžeš $1 abo jednoho z druhich [[{{MediaWiki:Grouppage-sysop}}|administratorow]] kontaktować, zo by blokowanje diskutował.

Wobkedźbuj, zo njemóžeš funkciju "Wužiwarjej mejlku pósłać" wužiwać, jeli nimaš płaćiwu e-mejlowu adresu, kotraž je w twojich [[Special:Preferences|wužiwarskich nastajenjach]] zregistrowana a njebi blokowany ju wužiwać.

Twój aktualna adresa IP je $3 a ID blokowanja je #$5.
Prošu podaj wšě horjeka naspomnjene podrobnosće w naprašowanjach, kotrež činiš.',
'blockednoreason'                  => 'žana přičina podata',
'blockedoriginalsource'            => 'To je žórłowy tekst strony <b>$1</b>:',
'blockededitsource'                => 'Tekst <b>twojich změnow</b> strony <b>$1</b> so tu pokazuje:',
'whitelistedittitle'               => 'Za wobdźěłowanje je přizjewjenje trěbne.',
'whitelistedittext'                => 'Dyrbiš so $1, zo by strony wobdźěłować móhł.',
'confirmedittitle'                 => 'Twoja e-mejlowa adresa dyrbi so wobkrućić, prjedy hač móžeš strony wobdźěłować.',
'confirmedittext'                  => 'Dyrbiš swoju e-mejlowu adresa wobkrućić, prjedy hač móžeš strony wobdźěłować. Prošu zapodaj a wobkruć swoju e-mejlowu adresu we [[Special:Preferences|wužiwarskich nastajenjach]].',
'nosuchsectiontitle'               => 'Wotrězk njeeksistuje',
'nosuchsectiontext'                => 'Sy spytał, njewobstejacy wotrězk $1 wobdźěłać. Móžeš pak jenož wobstejace wotrězki wobdźěłać.',
'loginreqtitle'                    => 'Přizjewjenje trěbne',
'loginreqlink'                     => 'přizjewić',
'loginreqpagetext'                 => 'Dyrbiš so $1, zo by strony čitać móhł.',
'accmailtitle'                     => 'Hesło bu pósłane.',
'accmailtext'                      => 'Hesło za wužiwarja "$1" bu na adresu $2 pósłane.',
'newarticle'                       => '(Nowy nastawk)',
'newarticletext'                   => 'Sy wotkaz k stronje slědował, kotraž hišće njeeksistuje. Zo by stronu wutworił, wupjelń slědowace tekstowe polo (hlej [[{{MediaWiki:Helppage}}|stronu pomocy]] za dalše informacije). Jeli sy zmylnje tu, klikń prosće na tłóčatko <b>Wróćo</b> we swojim wobhladowaku.',
'anontalkpagetext'                 => "---- ''To je diskusijna strona za anonymneho wužiwarja, kiž hišće konto wutworił njeje abo je njewužiwa. Dyrbimy tohodla numerisku IP-adresu wužiwać, zo bychmy jeho/ju identifikowali. Tajka IP-adresa hodźi so wot wjacorych wužiwarjow zhromadnje wužiwać. Jeli sy anonymny wužiwar a měniš, zo buchu irelewantne komentary k tebi pósłane, [[Special:UserLogin/signup|wutwor prošu konto]] abo [[Special:UserLogin|přizjew so]], zo by přichodnu šmjatańcu z anonymnymi wužiwarjemi wobešoł.''",
'noarticletext'                    => 'Tuchwilu tuta strona žadyn tekst njewobsahuje, móžeš jeje titul w druhich stronach [[Special:Search/{{PAGENAME}}|pytać]] abo [{{fullurl:{{FULLPAGENAME}}|action=edit}} stronu wobdźěłać].',
'userpage-userdoesnotexist'        => 'Wužiwarske konto „$1“ njeje zregistrowane. Prošu pruwuj, hač chceš tutu stronu woprawdźe wutworić/wobdźěłać.',
'clearyourcache'                   => '<b>Kedźbu:</b> Po składowanju dyrbiš snano pufrowak swojeho wobhladowaka wuprózdnić, <b>Mozilla/Firefox/Safari:</b> tłóč na <i>Umsch</i> kliknjo na <i>Znowa</i> abo tłóč <i>Strg-Umsch-R</i> (<i>Cmd-Shift-R</i> na Apple Mac); <b>IE:</b> tłóč <i>Strg</i> kliknjo na symbol <i>Aktualisieren</i> abo tłóč <i>Strg-F5</i>; <b>Konqueror:</b>: Klikń jenož na tłóčatko <i>Erneut laden</i> abo tłoč  <i>F5</i>; Wužiwarjo <b>Opery</b> móža swój pufrowak dospołnje  w <i>Tools→Preferences</i> wuprózdnić.',
'usercssjsyoucanpreview'           => '<strong>Pokiw:</strong> Wužij tłóčku „Přehlad”, zo by swój nowy css/js do składowanja testował.',
'usercsspreview'                   => "'''Wobkedźbujće, zo sej jenož přehlad swojeho wužiwarskeho CSS wobhladuješ. Hišće njeje składowany!'''",
'userjspreview'                    => "== Přehlad twojeho wosobinskeho JavaScript ==

'''Kedźbu:''' Po składowanju dyrbiš pufrowak swojeho wobhladowaka wuprózdnić '''Mozilla/Firefox:''' ''Strg-Shift-R'', '''Internet Explorer:''' ''Strg-F5'', '''Opera:''' ''F5'', '''Safari:''' ''Cmd-Shift-R'', '''Konqueror:''' ''F5''.",
'userinvalidcssjstitle'            => "'''Warnowanje:''' Skin z mjenom „$1” njeeksistuje. Prošu mysli na to, zo wosobinske strony .css a .js titul z małym pismikom wuwziwaja, na př. {{ns:user}}:Foo/monobook.css město {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Zaktualizowany)',
'note'                             => '<strong>Kedźbu:</strong>',
'previewnote'                      => '<strong>Kedźbu, to je jenož přehlad, změny hišće składowane njejsu!</strong>',
'previewconflict'                  => 'Tutón přehlad tekst w hornim tekstowym polu zwobrazni kaž so zjewi, jeli jón składuješ.',
'session_fail_preview'             => '<strong>Njemóžachmy twoju změnu předźěłać, dokelž su so posedźenske daty zhubili. Spytaj prošu hišće raz.
Jeli to hišće njefunguje, [[Special:UserLogout|wotzjew so]] a přizjew so zaso.</strong>',
'session_fail_preview_html'        => "<strong>Njemóžachmy twoje změnu předźěłać, dokelž su so posedźenske daty zhubili.</strong>

''Dokelž we {{GRAMMAR:lokatiw|{{SITENAME}}}} je luty HTML zmóžnił, je přehlad jako wěstotna naprawa přećiwo atakam přez JavaScript schowany.''

<strong>Jeli to je legitimny wobdźěłowanski pospyt, spytaj prošu hišće raz. Jeli to hišće njefunguje, [[Special:UserLogout|wotzjew so]] a přizjew so znowa.</strong>",
'token_suffix_mismatch'            => '<strong>Twoja změna je so wotpokazała, dokelž twój wobhladowak je znamješka skepsał.
Składowanje móže wobsah strony zničić. Móže so to na přikład přez wopačnje dźěłowacy proksy stać.</strong>',
'editing'                          => 'Wobdźěłanje strony $1',
'editingsection'                   => 'Wobdźěłanje strony $1 (wotrězk)',
'editingcomment'                   => 'Wobdźěłanje strony $1 (komentar)',
'editconflict'                     => 'Wobdźěłowanski konflikt: $1',
'explainconflict'                  => 'Něchtó druhi je stronu změnił w samsnym času, hdyž sy spytał ju wobdźěłować. Hornje tekstowe polo wobsahuje tekst strony kaž tuchwilu eksistuje. Twoje změny so w delnim tekstowym polu pokazuja. Dyrbiš swoje změny do eksistowaceho teksta zadźěłać. <b>Jenož</b> tekst w hornim tekstowym polu so składuje hdyž znowa na „Składować” kliknješ.<br />',
'yourtext'                         => 'Twój tekst',
'storedversion'                    => 'Składowana wersija',
'nonunicodebrowser'                => '<strong>KEDŹBU: Twój wobhladowak z Unikodu kompatibelny njeje. Prošu wužiwaj hinaši wobhladowak.</strong>',
'editingold'                       => '<strong>KEDŹBU: Wobdźěłuješ staršu wersiju strony. Jeli ju składuješ, zjewi so jako najnowša wersija!</strong>',
'yourdiff'                         => 'Rozdźěle',
'copyrightwarning'                 => 'Prošu wobkedźbuj, zo wšě přinoški k {{GRAMMAR:datiw|{{SITENAME}}}} $2 podleže (hlej $1 za podrobnosće). Jeli nochceš, zo so twój přinošk po dobrozdaću wobdźěłuje a znowa rozšěrja, njeskładuj jón.<br />
Lubiš tež, zo sy to sam napisał abo ze zjawneje domejny abo z podobneho žórła kopěrował.
Kopěrowanje tekstow, kiž su přez awtorske prawa škitane, je zakazane! <strong>NJESKŁADUJ PŘINOŠKI Z COPYRIGHTOM BJEZ DOWOLNOSĆE!</strong>',
'copyrightwarning2'                => 'Prošu wobkedźbuj, zo wšě přinoški k {{GRAMMAR:datiw|{{SITENAME}}}} hodźa so wot druhich wužiwarjow wobdźěłować, změnić abo wotstronić. Jeli nochceš, zo so twój přinošk po dobrozdaću wobdźěłuje, njeskładuj jón.<br />

Lubiš nam tež, zo sy jón sam napisał abo ze zjawneje domejny abo z podobneho swobodneho žórła kopěrował (hlej $1 za podrobnosće).

<strong>NJESKŁADUJ PŘINOŠKI Z COPYRIGHTOM BJEZ DOWOLNOSĆE!</strong>',
'longpagewarning'                  => '<strong>KEDŹBU: Strona wobsahuje $1 kB; někotre wobhladowaki maja problemy, strony wobdźěłać, kotrež wobsahuja 32 kB abo wjace. Prošu přemysli sej stronu do mjeńšich wotrězkow rozrjadować.</strong>',
'longpageerror'                    => '<strong>ZMYLK: Tekst, kotryž sy spytał składować wobsahuje $1 kB, maksimalna wulkosć pak je $2 kB. Njehodźi so składować.</strong>',
'readonlywarning'                  => '<strong>KEDŹBU: Datowa banka bu wothladanja dla zawrjena, tohodla njemóžeš swoje wobdźěłowanja nětko składować. Móžeš tekst do tekstoweje dataje přesunyć a jón za pozdźišo składować.</strong>',
'protectedpagewarning'             => '<strong>KEDŹBU: Strona bu škitana, tak zo jenož wužiwarjo z prawami administratora móža ju wobdźěłać.</strong>',
'semiprotectedpagewarning'         => '<b>Kedźbu:</b> Strona bu škitana, tak zo jenož přizjewjeni wužiwarjo móža ju wobdźěłać.',
'cascadeprotectedwarning'          => "'''KEDŹBU:''' Tuta strona je škitana, tak zo móža ju jenož wužiwarjo z prawami administratora wobdźělać, dokelž je w {{PLURAL:$1|slědowacej stronje|slědowacych stronach}} zapřijata, {{PLURAL:$1|kotraž je|kotrež su}} přez kaskadowu opciju {{PLURAL:$1|škitana|škitane}}:",
'titleprotectedwarning'            => '<strong>WARNOWANJE: Tuta strona bu zawrjena, jenož wěsći wužiwarjo móža ju wutworić.</strong>',
'templatesused'                    => 'Na tutej stronje wužiwane předłohi:',
'templatesusedpreview'             => 'W tutym přehledźe wužiwane předłohi:',
'templatesusedsection'             => 'W tutym wotrězku wužiwane předłohi:',
'template-protected'               => '(škitana)',
'template-semiprotected'           => '(škitana za njepřizjewjenych wužiwarjow a nowačkow)',
'hiddencategories'                 => 'Tuta strona je čłon w {{PLURAL:$1|1 schowanej kategoriji|$1 schowanymaj kategorijomaj|$1 schowanych kategorijach|$1 schowanych kategorijach}}:',
'edittools'                        => '<!-- Tutón tekst so spody wobdźěłowanskich a nahrawanskich formularow pokazuje. -->',
'nocreatetitle'                    => 'Wutworjenje stron je wobmjezowane.',
'nocreatetext'                     => 'Na {{GRAMMAR:Lokatiw|{{SITENAME}}}} bu wutworjenje nowych stronow wobmjezowane. Móžeš wobstejace strony wobdźěłać abo [[Special:UserLogin|so přizjewić abo wužiwarske konto wutworić]].',
'nocreate-loggedin'                => 'Nimaš prawo, zo by nowe strony w tutym wikiju wutworił.',
'permissionserrors'                => 'Woprawnjenske zmylki',
'permissionserrorstext'            => 'Nimaš prawo, zo by tutu akciju wuwjedł. {{PLURAL:$1|Přičina|Přičiny}}:',
'permissionserrorstext-withaction' => 'Nimaš dowolnosć za $2 ze {{PLURAL:$1|slědowaceje přičiny|slědowaceju přičinow|slědowacych přičinow|slědowacych přičinow}}:',
'recreate-deleted-warn'            => "'''Kedźbu: Wutworiš stronu, kiž bu prjedy wušmórnjena.'''

Prošu přepruwuj, hač je znowawutworjenje woprawnjena a wotpowěduje prawidłam projekta.
Tu slěduje wujimk z protokola wušmórnjenjow z přičinu za předawše wušmórnjenje:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Warnowanje: Tuta strona wobsahuje přewjele parserowych wołanjow, kotrež serwer poćežuja.
 
Jich dyrbi jenož $2 być, je nětko $1.',
'expensive-parserfunction-category'       => 'Strony, kotrež tajke parserowe funkcije přehusto wołaja, kotrež serwer poćežuja.',
'post-expand-template-inclusion-warning'  => 'Warnowanje: Wulkosć zapřijatych předłohow je přewulka. Někotre předłohi so njezapřijmu.',
'post-expand-template-inclusion-category' => 'Strony, hdźež maksimalna wulkosć zapřijatych předłohow je překročena',
'post-expand-template-argument-warning'   => 'Warnowanje: Tuta strona wobsahuje znajmjeńša jedyn předłohowy argument, kotryž ma přewulku espansisku wulkosć. Tute argumenty bu wuwostajene.',
'post-expand-template-argument-category'  => 'Strony, kotrež wuwostajene předłohowe argumenty wobsahuja',

# "Undo" feature
'undo-success' => 'Wersija je so wuspěšnje wotstroniła. Prošu přepruwuj deleka w přirunanskim napohledźe, hač twoja změna bu přewzata a klikń potom na „Składować”, zo by změnu składował.',
'undo-failure' => '<span class="error">Wobdźěłanje njehodźeše so wotstronić, dokelž wotpowědny wotrězk bu mjeztym změnjeny.</span>',
'undo-norev'   => 'Změna njeda so cofnyć, dokelž njeeksistuje abo bu wušmórnjena.',
'undo-summary' => 'Změna $1 [[Special:Contributions/$2|$2]] ([[User talk:$2|diskusija]]) bu cofnjena.',

# Account creation failure
'cantcreateaccounttitle' => 'Wužiwarske konto njeda so wutworić.',
'cantcreateaccount-text' => "Wutworjenje wužiwarskeho konta z IP-adresy '''$1''' bu wot [[User:$3|$3]] zablokowane.

Přičina za blokowanje, podata wot $3, je: ''$2''",

# History pages
'viewpagelogs'        => 'protokole tuteje strony pokazać',
'nohistory'           => 'Njeje žanych staršich wersijow strony.',
'revnotfound'         => 'Njebě móžno, požadanu wersiju namakać',
'revnotfoundtext'     => 'Stara wersija strony, kotruž sy žadał, njeda so namakać. Prošu pruwuj URL, kiž sy wužiwał.',
'currentrev'          => 'Aktualna wersija',
'revisionasof'        => 'Wersija z $1',
'revision-info'       => 'Wersija z $1 wužiwarja $2',
'previousrevision'    => '←starša wersija',
'nextrevision'        => 'nowša wersija→',
'currentrevisionlink' => 'Aktualnu wersiju pokazać',
'cur'                 => 'akt',
'next'                => 'přich',
'last'                => 'posl',
'page_first'          => 'spočatk',
'page_last'           => 'kónc',
'histlegend'          => 'Diff wubrać: Wubjer opciske pola za přirunanje a tłóč na enter abo tłóčku deleka.

Legenda: (akt) = rozdźěl k tuchwilnej wersiji, (posl) = rozdźěl k předchadnej wersiji, S = snadna změna.',
'deletedrev'          => '[wušmórnjena]',
'histfirst'           => 'tuchwilnu',
'histlast'            => 'najstaršu',
'historysize'         => '({{PLURAL:$1|1 bajt|$1 bajtaj|$1 bajty|$1 bajtow}})',
'historyempty'        => '(prózdna)',

# Revision feed
'history-feed-title'          => 'Stawizny wersijow',
'history-feed-description'    => 'Stawizny wersijow za tutu stronu w {{GRAMMAR:lokatiw|{{SITENAME}}}}',
'history-feed-item-nocomment' => '$1 w $2 hodź.', # user at time
'history-feed-empty'          => 'Strona, kotruž sy požadał, njeeksistuje. Bu snano z wikija wotstronjena abo přesunjena. Móžeš tu [[Special:Search|w {{SITENAME}}]] za stronami z podobnym titulom pytać.',

# Revision deletion
'rev-deleted-comment'         => '(komentar wotstronjeny)',
'rev-deleted-user'            => '(wužiwarske mjeno wotstronjene)',
'rev-deleted-event'           => '(Protokolowa akcija bu wotstronjena)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">Tuta wersija bu wušmórnjena a njeda so wjace čitać. Přićinu móžeš w [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} protokolu wušmórnjenjow] zhonić.</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Tuta wersija bu wušmórnjena a njeda so wjace čitać. Jako administrator móžeš ju pak dale čitać. Přićinu móžeš w [{{fullurl:{{ns:special}}:Log/delete|page={{PAGENAMEE}}}} protokolu wušmórnjenjow] zhonić.</div>',
'rev-delundel'                => 'pokazać/schować',
'revisiondelete'              => 'Wersije wušmórnyć/wobnowić',
'revdelete-nooldid-title'     => 'Njepłaćiwa cilowa wersija',
'revdelete-nooldid-text'      => 'Pak njejsy cilowu wersiju podał, zo by tutu funkciju wuwjedł, podata wersija njeeksistuje pak pospytuješ aktualnu wersiju schować.',
'revdelete-selected'          => '{{PLURAL:$2|Wubrana wersija|Wubranej wersiji|Wubrane wersije|Wubranych wersijow}} wot [[:$1]]:',
'logdelete-selected'          => "{{PLURAL:$1|Wubrany zapisk z protokola|Wubranej zapiskaj z protokola|Wubrane zapiski z protokola|Wubrane zapiski z protokola}} za '''$1:'''",
'revdelete-text'              => 'Wušmórnjene wersije so w stawiznach dale jewja, jich wobsah pak za wužiwarjow čitajomne njeje.

Druzy administratorojo we {{SITENAME}} móža schowany tekst dale čitać a jón wobnowić, chibazo su tež jich prawa wobmjezowane.',
'revdelete-legend'            => 'Wobmjezowanja za widźomnosć nastajić',
'revdelete-hide-text'         => 'Tekst tuteje wersije schować',
'revdelete-hide-name'         => 'Akciju w protokolach schować',
'revdelete-hide-comment'      => 'Zjeće schować',
'revdelete-hide-user'         => 'Wužiwarske mjeno/IP-adresu schować',
'revdelete-hide-restricted'   => 'Tute wobmjezowanja na administratorow nałožić a tutón interfejs zawrěć',
'revdelete-suppress'          => 'Přičinu wušmórnjenja tež za administratorow schować',
'revdelete-hide-image'        => 'Wobsah wobraza schować',
'revdelete-unsuppress'        => 'Wobmjezowanja za wobnowjene wersije zběhnyć',
'revdelete-log'               => 'Komentar w protokolu:',
'revdelete-submit'            => 'Na wubranu wersiju nałožować',
'revdelete-logentry'          => 'Widźomnosć wersije změnjena za [[$1]]',
'logdelete-logentry'          => 'je widźomnosć za [[$1]] změnił',
'revdelete-success'           => "'''Widźomnosć wersije bu wuspěšnje změnjena.'''",
'logdelete-success'           => 'Widźomnosć zapiska bu wuspěšnje změnjena.',
'revdel-restore'              => 'Widźomnosć změnić',
'pagehist'                    => 'Stawizny strony',
'deletedhist'                 => 'Wušmórnjene stawizny',
'revdelete-content'           => 'wobsah',
'revdelete-summary'           => 'zjeće wobdźěłać',
'revdelete-uname'             => 'wužiwarske mjeno',
'revdelete-restricted'        => 'na administratorow nałožene wobmjezowanja',
'revdelete-unrestricted'      => 'Wobmjezowanja za administratorow wotstronjene',
'revdelete-hid'               => '$1 schowany',
'revdelete-unhid'             => '$1 pokazany',
'revdelete-log-message'       => '$1 za $2 {{PLURAL:$2|wersija|wersiji|wersije|wersijow}}',
'logdelete-log-message'       => '$1 za $2 {{PLURAL:$2|podawk|podawkaj|podawki|podawkow}}',

# Suppression log
'suppressionlog'     => 'Protokol potłóčenjow',
'suppressionlogtext' => 'Deleka je lisćina wušmórnjenjow a zablokowanjow, inkluziwnje wobsaha schowaneho wot administratorow. Hlej [[Special:IPBlockList|Lisćina zablokowanjow IP]] za lisćinu tuchwilnych zablokowanjow.',

# History merging
'mergehistory'                     => 'Stawizny stronow zjednoćić',
'mergehistory-header'              => 'Tuta strona ći dowola wersije stawiznow žórłoweje strony na nowej stronje zjednoćić.
Zawěsć, zo tuta změna stawiznisku kontinuitu strony wobchowuje.',
'mergehistory-box'                 => 'Wersije dweju stronow zjednoćić:',
'mergehistory-from'                => 'Žórłowa strona:',
'mergehistory-into'                => 'Cilowa strona:',
'mergehistory-list'                => 'Zjednoćujomne wersijowe stawizny',
'mergehistory-merge'               => 'Slědowace wersije wot [[:$1|$1]] hodźa so z [[:$2|$2]] zjednoćić. Wužij špaltu z opciskimi tłóčatkami, zo by jenož te wersije zjednoćił, kotrež su so w podatym času a bo před nim wutworili. Wobkedźbuj, zo wužiwanje nawigaciskich wotkazow budźe tutu špaltu wróćo stajeć.',
'mergehistory-go'                  => 'Zjednoćujomne změny pokazać',
'mergehistory-submit'              => 'Wersije zjednoćić',
'mergehistory-empty'               => 'Njehodźa so žane wersije zjednoćeć.',
'mergehistory-success'             => '$3 {{PLURAL:$3|wersija|wersiji|wersije|wersijow}} wot [[:$1]] wuspěšnje z [[:$2]] {{PLURAL:$3|zjednoćena|zjednoćenej|zjednoćene|zjednoćene}}.',
'mergehistory-fail'                => 'Njeje móžno zjednócenje stawiznow přewjesć, prošu přepruwuj stronu a časowe parametry.',
'mergehistory-no-source'           => 'Žórłowa strona $1 njeeksistuje.',
'mergehistory-no-destination'      => 'Cilowa strona $1 njeeksistuje.',
'mergehistory-invalid-source'      => 'Žórłowa strona dyrbi płaćiwy titul być.',
'mergehistory-invalid-destination' => 'Cilowa strona dyrbi płaćiwy titul być.',
'mergehistory-autocomment'         => '[[:$1]] z [[:$2]] zjednoćeny',
'mergehistory-comment'             => '[[:$1]] z [[:$2]] zjednoćeny: $3',

# Merge log
'mergelog'           => 'Protokol zjednoćenja',
'pagemerge-logentry' => '[[$1]] z [[$2]] zjednoćeny (do $3 {{PLURAL:$3|wersije|wersijow|wersijow|wersijow}})',
'revertmerge'        => 'Zjednoćenje cofnyć',
'mergelogpagetext'   => 'Deleka je lisćina najaktualnišich zjednoćenjow stawiznow dweju stronow.',

# Diffs
'history-title'           => 'Stawizny wersijow strony „$1“',
'difference'              => '(rozdźěl mjez wersijomaj)',
'lineno'                  => 'Rjadka $1:',
'compareselectedversions' => 'Wubranej wersiji přirunać',
'editundo'                => 'cofnyć',
'diff-multi'              => '<small>(Přirunanje wersijow zapřija {{PLURAL:$1|jednu mjez nimaj ležacu wersiju|dwě mjez nimaj ležacej wersiji|$1 mjez nimaj ležace wersije|$1 mjez nimaj ležacych wersijow}}.)</small>',

# Search results
'searchresults'             => 'Pytanske wuslědki',
'searchresulttext'          => 'Za dalše informacije wo pytanju {{GRAMMAR:genitiw|{{SITENAME}}}}, hlej [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Sy za \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|wšěmi stronami, kotrež započinaja so z "$1"]] | [[Special:WhatLinksHere/$1|wšěmi stronami, kotrež na "$1" wotkazuja]]) pytal.',
'searchsubtitleinvalid'     => 'Sy naprašowanje za „$1“ stajił.',
'noexactmatch'              => "'''Strona z titulom \"\$1\" njeeksistuje.'''
Móžeš [[:\$1|tutu stronu wutworić]].",
'noexactmatch-nocreate'     => "'''Njeje strona z titulom \"\$1\".'''",
'toomanymatches'            => 'Přewjele pytanskich wuslědkow, prošu spytaj druhe wotprašenje.',
'titlematches'              => 'Strony z wotpowědowacym titulom',
'notitlematches'            => 'Žane strony z wotpowědowacym titulom',
'textmatches'               => 'Strony z wotpowědowacym tekstom',
'notextmatches'             => 'Žane strony z wotpowědowacym tekstom',
'prevn'                     => 'předchadne $1',
'nextn'                     => 'přichodne $1',
'viewprevnext'              => '($1) ($2) ($3) pokazać',
'search-result-size'        => '$1 ({{PLURAL:$2|1 słowo|$2 słowje|$2 słowa|$2 słowow}})',
'search-result-score'       => 'Relewanca: $1 %',
'search-redirect'           => '(Daleposrědkowanje $1)',
'search-section'            => '(wotrězk $1)',
'search-suggest'            => 'Měnješe ty $1?',
'search-interwiki-caption'  => 'Sotrowske projekty',
'search-interwiki-default'  => '$1 wuslědki:',
'search-interwiki-more'     => '(dalše)',
'search-mwsuggest-enabled'  => 'z namjetami',
'search-mwsuggest-disabled' => 'žane namjety',
'search-relatedarticle'     => 'Přiwuzne',
'mwsuggest-disable'         => 'Namjety AJAX znjemóžnić',
'searchrelated'             => 'přiwuzny',
'searchall'                 => 'wšě',
'showingresults'            => "Deleka so hač {{PLURAL:$1|'''1''' wuslědk pokazuje|'''$1''' wuslědkaj pokazujetej|'''$1''' wuslědki pokazuja|'''$1''' wuslědkow pokazuje}}, započinajo z #'''$2'''.",
'showingresultsnum'         => "Deleka so {{PLURAL:$3|'''1''' wuslědk pokazuje|'''$3''' wuslědkaj pokazujetej|'''$3''' wuslědki pokazuja|'''$3''' wuslědkow pokazuje}}, započinajo z #'''$2'''.",
'showingresultstotal'       => "{{PLURAL:3|Slěduje wuslědk '''$1''' z '''$3'''|Slědujetej wuslědkaj '''$1 - $2''' z '''$3'''|Slěduja wuslědki '''$1 - $2''' z '''$3'''|Slěduje wuslědkow '''$1 - $2''' z '''$3'''}}",
'nonefound'                 => '<b>Kedźbu:</b> Pytanja bjez wuspěcha so často z pytanjom za powšitkownymi słowami zawinuja, kotrež so njeindicěruja abo přez podaće wjace hač jednoho pytanskeho wuraza. Jenož strony, kotrež wšě pytanske wurazy wobsahuja, so w lisćinje wuslědkow zjewja. W tym padźe spytaj ličbu pytanskich wurazow pomjeńšić.',
'powersearch'               => 'Pytać',
'powersearch-legend'        => 'Rozšěrjene pytanje',
'powersearch-ns'            => 'W mjenowych rumach pytać:',
'powersearch-redir'         => 'Daleposrědkowanja nalistować',
'powersearch-field'         => 'Pytać za:',
'search-external'           => 'Eksterne pytanje',
'searchdisabled'            => 'Pytanje w {{GRAMMAR:lokatiw|{{SITENAME}}}} tuchwilu móžne njeje. Móžeš mjeztym z Google pytać. Wobkedźbuj, zo móža wuslědki z wobsaha {{GRAMMAR:genitiw|{{SITENAME}}}} zestarjene być.',

# Preferences page
'preferences'              => 'Nastajenja',
'mypreferences'            => 'moje nastajenja',
'prefs-edits'              => 'Ličba změnow:',
'prefsnologin'             => 'Njepřizjewjeny',
'prefsnologintext'         => 'Dyrbiš <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} přizjewjeny]</span>  być, zo by móhł nastajenja postajić.',
'prefsreset'               => 'Nastajenja su so ze składa wróćo stajili. Twoje změnjenja njejsu so składowali.',
'qbsettings'               => 'Pobóčna lajsta',
'qbsettings-none'          => 'Žane',
'qbsettings-fixedleft'     => 'Leži nalěwo',
'qbsettings-fixedright'    => 'Leži naprawo',
'qbsettings-floatingleft'  => 'Wisa nalěwo',
'qbsettings-floatingright' => 'Wisa naprawo',
'changepassword'           => 'Hesło změnić',
'skin'                     => 'Šat',
'dateformat'               => 'Format datuma',
'datedefault'              => 'Žane nastajenje',
'datetime'                 => 'Datum a čas',
'math_failure'             => 'Analyza njeje so poradźiła',
'math_unknown_error'       => 'njeznaty zmylk',
'math_unknown_function'    => 'njeznata funkcija',
'math_lexing_error'        => 'leksikalny zmylk',
'math_syntax_error'        => 'syntaktiski zmylk',
'math_image_error'         => 'Konwertowanje do PNG zwrěšćiło; kontroluj prawu instalaciju latex, dvips, gs a konwertuj',
'math_bad_tmpdir'          => 'Njemóžno do nachwilneho matematiskeho zapisa pisać abo jón wutworić',
'math_bad_output'          => 'Njemóžno do matematiskeho zapisa za wudaće pisać abo jón wutworić',
'math_notexvc'             => 'Wuwjedźomny texvc pobrachuje; prošu hlej math/README za konfiguraciju.',
'prefs-personal'           => 'Wužiwarske daty',
'prefs-rc'                 => 'Aktualne změny',
'prefs-watchlist'          => 'Wobkedźbowanki',
'prefs-watchlist-days'     => 'Ličba dnjow, kotrež maja so we wobkedźbowankach pokazać:',
'prefs-watchlist-edits'    => 'Ličba změnow, kotrež maja so we wobkedźbowankach pokazać:',
'prefs-misc'               => 'Wšelake nastajenja',
'saveprefs'                => 'Składować',
'resetprefs'               => 'Njeskładowane změny zaćisnyć',
'oldpassword'              => 'Stare hesło:',
'newpassword'              => 'Nowe hesło:',
'retypenew'                => 'Nowe hesło wospjetować:',
'textboxsize'              => 'Wobdźěłowanje',
'rows'                     => 'Rjadki:',
'columns'                  => 'Stołpiki:',
'searchresultshead'        => 'Pytać',
'resultsperpage'           => 'Wuslědki za stronu:',
'contextlines'             => 'Rjadki na wuslědk:',
'contextchars'             => 'Kontekst na rjadku:',
'stub-threshold'           => 'Wotkazowe formatowanje <a href="#" class="stub">małych stronow</a> (w bajtach):',
'recentchangesdays'        => 'Ličba dnjow w lisćinje aktualnych změnow:',
'recentchangescount'       => 'Ličba stron w lisćinje aktualnych změnow, w stawiznach a w protokolach:',
'savedprefs'               => 'Nastajenja buchu składowane.',
'timezonelegend'           => 'Časowe pasmo',
'timezonetext'             => '¹Zapisaj ličbu hodźin, wo kotrež so twój lokalny čas wot časa serwera (UTC) wotchila.',
'localtime'                => 'Lokalny čas',
'timezoneoffset'           => 'Rozdźěl¹',
'servertime'               => 'Čas serwera',
'guesstimezone'            => 'Z wobhladowaka přewzać',
'allowemail'               => 'Mejlki wot druhich wužiwarjow přijimować',
'prefs-searchoptions'      => 'Pytanske opcije',
'prefs-namespaces'         => 'Mjenowe rumy',
'defaultns'                => 'W tutych mjenowych rumach awtomatisce pytać:',
'default'                  => 'standard',
'files'                    => 'Dataje',

# User rights
'userrights'                  => 'Zrjadowanje wužiwarskich prawow', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Wužiwarske skupiny zrjadować',
'userrights-user-editname'    => 'Wužiwarske mjeno:',
'editusergroup'               => 'Wužiwarske skupiny wobdźěłać',
'editinguser'                 => "Měnja so wužiwarske prawa wot wužiwarja '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Wužiwarske skupiny wobdźěłać',
'saveusergroups'              => 'Wužiwarske skupiny składować',
'userrights-groupsmember'     => 'Čłon skupiny:',
'userrights-groups-help'      => 'Móžeš skupiny změnić, w kotrychž wužiwar je.
* Markěrowany kašćik woznamjenja, zo wužiwar je w tej skupinje.
* Njemarkěrowany kašćik woznamjenja, zo wužiwar w tej skupinje njeje.
* "*" podawa, zo njemóžeš skupinu wotstronić, tak ruče kaž sy ju přidał abo nawopak.',
'userrights-reason'           => 'Přičina:',
'userrights-no-interwiki'     => 'Nimaš prawo wužiwarske prawa w druhich wikijach změnić.',
'userrights-nodatabase'       => 'Datowa banka $1 njeeksistuje abo lokalna njeje.',
'userrights-nologin'          => 'Dyrbiš so z admininstratorowym kontom [[Special:UserLogin|přizjewić]], zo by wužiwarske prawa změnił.',
'userrights-notallowed'       => 'Twoje konto nima trěbne prawa, zo by wužiwarske prawa přidźělił.',
'userrights-changeable-col'   => 'Skupiny, kotrež móžeš změnić',
'userrights-unchangeable-col' => 'Skupiny, kotrež njemóžeš změnić',

# Groups
'group'               => 'Skupina:',
'group-user'          => 'Wužiwarjo',
'group-autoconfirmed' => 'awtomatisce potwjerdźeny',
'group-bot'           => 'Boty',
'group-sysop'         => 'administratorojo',
'group-bureaucrat'    => 'Běrokraća',
'group-suppress'      => 'Dohladowarjo',
'group-all'           => '(wšě)',

'group-user-member'          => 'Wužiwar',
'group-autoconfirmed-member' => 'Potwjerdźeny wužiwar',
'group-bot-member'           => 'bot',
'group-sysop-member'         => 'administrator',
'group-bureaucrat-member'    => 'běrokrat',
'group-suppress-member'      => 'Dohladowar',

'grouppage-user'          => '{{ns:project}}:Wužiwarjo',
'grouppage-autoconfirmed' => '{{ns:project}}:Awtomatisce potwjerdźeni wužiwarjo',
'grouppage-bot'           => '{{ns:project}}:Boćiki',
'grouppage-sysop'         => '{{ns:project}}:Administratorojo',
'grouppage-bureaucrat'    => '{{ns:project}}:Běrokraća',
'grouppage-suppress'      => '{{ns:project}}:Dohladowanje',

# Rights
'right-read'                 => 'Strony čitać',
'right-edit'                 => 'Strony wobdźěłać',
'right-createpage'           => 'Strony wutworić (kotrež diskusijne strony njejsu)',
'right-createtalk'           => 'Diskusijne strony wutworić',
'right-createaccount'        => 'Nowe wužiwarske konta wutworić',
'right-minoredit'            => 'Změny jako snadne markěrować',
'right-move'                 => 'Strony přesunyć',
'right-move-subpages'        => 'Strony z jich podstronami přesunyć',
'right-suppressredirect'     => 'Při přesunjenju strony ze stareho mjena žane daleposrědkowanje wutworić',
'right-upload'               => 'Dataje nahrać',
'right-reupload'             => 'Eksistowacu dataju přepisać',
'right-reupload-own'         => 'Eksistowacu dataju, kotraž bu wot samsneho wužiwarja nahrata, přepisać',
'right-reupload-shared'      => 'Dataje w hromadźe wužiwanej repozitoriju lokalnje přepisać',
'right-upload_by_url'        => 'Dataju z URL-adresy nahrać',
'right-purge'                => 'Pufrowak sydła za stronu bjez wobkrućenskeje strony wuprózdnić',
'right-autoconfirmed'        => 'Połzaškitane strony wobdźěłać',
'right-bot'                  => 'Ma so jako awtomatiski proces wobjednać',
'right-nominornewtalk'       => 'Snadne změny k diskusijnym stronam zwobraznjenje nowych powěsćow wuwołać njedać',
'right-apihighlimits'        => 'Wyše limity wi API-naprašowanjach wužiwać',
'right-writeapi'             => 'writeAPI wužiwać',
'right-delete'               => 'Strony zničić',
'right-bigdelete'            => 'Strony z dołhimi stawiznami zničić',
'right-deleterevision'       => 'Jednotliwe wersije wušmórnyć a wobnowić',
'right-deletedhistory'       => 'Wušmórnjene zapiski stawiznow bjez přisłušneho teksta wobhladać',
'right-browsearchive'        => 'Zničene strony pytać',
'right-undelete'             => 'Strony wobnowić',
'right-suppressrevision'     => 'Wersije, kotrež su před administratorami schowane, přepruwować a wobnowić',
'right-suppressionlog'       => 'Priwatne protokole wobhladać',
'right-block'                => 'Druhich wužiwarjow při wobdźěłowanju haćić',
'right-blockemail'           => 'Wužiwarja při słanju e-mejlow haćić',
'right-hideuser'             => 'Wužiwarske mjeno blokować a schować',
'right-ipblock-exempt'       => 'Blokowanja IP, awtomatiske blokowanje a blokowanja wobwodow wobeńć',
'right-proxyunbannable'      => 'Automatiske blokowanja proksyjow wobeńć',
'right-protect'              => 'Škitowe schodźenki změnić a škitanu stronu wobdźěłać',
'right-editprotected'        => 'Škitane strony wobdźěłać (bjez kaskadoweho škita)',
'right-editinterface'        => 'Wužiwarski powjerch wobdźěłać',
'right-editusercssjs'        => 'Dataje CSS a JS druhich wužiwarjow wobdźěłać',
'right-rollback'             => 'Poslednjeho wužiwarja, kotryž wěstu stronu wobdźěła, spěšnje rewertować',
'right-markbotedits'         => 'Rewertowane změny jako botowe změny markěrować',
'right-noratelimit'          => 'Přez žane limity wobmjezowane',
'right-import'               => 'Strony z druhich wikijow importować',
'right-importupload'         => 'Strony přez nahraće datajow importować',
'right-patrol'               => 'Změny jako dohladowane markěrować',
'right-autopatrol'           => 'Změny awtomatisce jako dohladowane markěrować dać',
'right-patrolmarks'          => 'Kontrolowe marki w najnowšich změnach wobhladać',
'right-unwatchedpages'       => 'Lisćinu njewobkedźbowanych stronow wobhladać',
'right-trackback'            => 'Trackback pósłać',
'right-mergehistory'         => 'Stawizny stronow zjednoćić',
'right-userrights'           => 'Wužiwarske prawa wobdźěłać',
'right-userrights-interwiki' => 'Wužiwarske prawa wužiwarjow druhich wikijow wobdźěłać',
'right-siteadmin'            => 'Datowu banku zawrěć a wotewrěć',

# User rights log
'rightslog'      => 'Protokol zrjadowanja wužiwarskich prawow',
'rightslogtext'  => 'To je protokol změnow wužiwarskich prawow.',
'rightslogentry' => 'skupinowe čłonstwo za $1 z $2 na $3 změnjene',
'rightsnone'     => '(ničo)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|změna|změnje|změny|změnow}}',
'recentchanges'                     => 'Aktualne změny',
'recentchangestext'                 => 'Na tutej stronje móžeš najaktualniše změny w {{GRAMMAR:lokatiw|{{SITENAME}}}} wobkedźbować.',
'recentchanges-feed-description'    => 'Slěduj najaktualniše změny {{GRAMMAR:genitiw|{{SITENAME}}}} w tutym kanalu.',
'rcnote'                            => "Deleka {{PLURAL:\$1|je '''1''' změna|stej poslednjej '''\$1''' změnje|su poslednje '''\$1''' změny|je poslednich '''\$1''' změnow}} w {{PLURAL:\$2|poslednim dnju|poslednimaj '''\$2''' dnjomaj|poslednich '''\$2''' dnjach|poslednich '''\$2''' dnjach}}, staw wot \$4, \$5. <div id=\"rc-legend\" style=\"float:right;font-size:84%;margin-left:5px;\"> <b>Legenda</b><br />   <b><tt>N</tt></b>&nbsp;– Nowy přinošk<br /> <b><tt>S</tt></b>&nbsp;– Snadna změna<br /> <b><tt>B</tt></b>&nbsp;– Změny awtomatiskich programow (bot)<br />  ''(± ličba)''&nbsp;– Změna wulkosće w bajtach </div>",
'rcnotefrom'                        => "Deleka so změny wot '''$2''' pokazuja (hač k '''$1''').",
'rclistfrom'                        => 'Nowe změny pokazać, započinajo z $1',
'rcshowhideminor'                   => 'snadne změny $1',
'rcshowhidebots'                    => 'změny awtomatiskich programow (bots) $1',
'rcshowhideliu'                     => 'změny přizjewjenych wužiwarjow $1',
'rcshowhideanons'                   => 'změny anonymnych wužiwarjow $1',
'rcshowhidepatr'                    => 'dohladowane změny $1',
'rcshowhidemine'                    => 'moje změny $1',
'rclinks'                           => 'Poslednje $1 změnow poslednich $2 dnjow pokazać<br />$3',
'diff'                              => 'rozdźěl',
'hist'                              => 'wersije',
'hide'                              => 'schować',
'show'                              => 'pokazać',
'minoreditletter'                   => 'S',
'newpageletter'                     => 'N',
'boteditletter'                     => 'B',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|wobkedźbowacy wužiwar|wobkedźbowacaj wužiwarjej|wobkedźbowacy wužiwarjo|wobkedźbowacych wužiwarjow}}]',
'rc_categories'                     => 'Jenož kategorije (dźělene z "|")',
'rc_categories_any'                 => 'wšě',
'rc-change-size'                    => '$1 {{PLURAL:$1|bajt|bajtaj|bajty|bajtow}}',
'newsectionsummary'                 => 'nowy wotrězk: /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Změny zwjazanych stron',
'recentchangeslinked-title'    => 'Změny na stronach, kotrež su z „$1“ wotkazane',
'recentchangeslinked-noresult' => 'Njejsu změny zwajzanych stron we wubranej dobje.',
'recentchangeslinked-summary'  => "Tuta strona nalistuje poslednje změny na wotkazanych stronach (resp. pola kategorijow na čłonach kategorije). 
Strony na [[Special:Watchlist|wobkedźbowankach]] su '''tučne'''.",
'recentchangeslinked-page'     => 'Mjeno strony:',
'recentchangeslinked-to'       => 'Změny na stronach pokazać, kotrež na datu stronu wotkazuja',

# Upload
'upload'                      => 'Dataju nahrać',
'uploadbtn'                   => 'Dataju nahrać',
'reupload'                    => 'Znowa nahrać',
'reuploaddesc'                => 'Nahraće přetorhnyć a so k nahrawanskemu formularej wróćić.',
'uploadnologin'               => 'Njepřizjewjeny',
'uploadnologintext'           => 'Dyrbiš [[Special:UserLogin|přizjewjeny]] być, zo by dataje nahrawać móhł.',
'upload_directory_missing'    => 'Zapis nahraćow ($1) faluje a njeda so přez webserwer wutworić.',
'upload_directory_read_only'  => 'Nahrawanski zapis ($1) njehodźi so přez webserwer popisować.',
'uploaderror'                 => 'Zmylk při nahrawanju',
'uploadtext'                  => "Wužij slědowacy formular, zo by nowe dataje nahrał.
Zo by prjedy nahrate dataje wobhladał abo pytał dźi k [[Special:ImageList|lisćinje nahratych datajow]], nahraća so tež w [[Special:Log/upload|protokolu nahraćow]], wušmórnjenja  [[Special:Log/delete|protokolu wušmornjenjow]] protokoluja.

Zo by dataju do strony zapřijał, wužij wotkaz w jednej ze slědowacych formow:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Dataja.jpg]]</nowiki></tt>''', zo by połnu wersiju dataje wužiwał
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Dataja.png|200px|thumb|left|alternatiwny tekst]]</nowiki></tt>''', zo by wobraz ze šěrokosću 200 pikselow do kašćika na lěwej kromje z alternatiwnym tekstom jako wopisanje wužiwał
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Dataja.ogg]]</nowiki></tt>''' zo by direktnje k dataji wotkazał, bjeztoho zo by so dataja zwobrazniła",
'upload-permitted'            => 'Dowolene datajowe typy: $1.',
'upload-preferred'            => 'Preferowane datajowe typy: $1.',
'upload-prohibited'           => 'Zakazane datajowe typy: $1.',
'uploadlog'                   => 'Protokol nahraćow',
'uploadlogpage'               => 'Protokol nahraćow',
'uploadlogpagetext'           => 'Deleka je lisćina najnowšich nahratych datajow.
Hlej [[Special:NewImages|galeriju nowych datajow]] za wizuelny přehlad.',
'filename'                    => 'Mjeno dataje',
'filedesc'                    => 'Zjeće',
'fileuploadsummary'           => 'Zjeće:',
'filestatus'                  => 'Licenca:',
'filesource'                  => 'Žórło:',
'uploadedfiles'               => 'Nahrate dataje',
'ignorewarning'               => 'Warnowanje ignorować a dataju najebać toho składować.',
'ignorewarnings'              => 'Wšě warnowanja ignorować',
'minlength1'                  => 'Datajowe mjena dyrbja znajmjeńša jedyn pismik dołhe być.',
'illegalfilename'             => 'Mjeno dataje „$1” wobsahuje znamješka, kotrež w titlach stronow dowolene njejsu. Prošu přemjenuj dataju a spytaj ju znowa nahrać.',
'badfilename'                 => 'Mjeno dataje bu do „$1” změnjene.',
'filetype-badmime'            => 'Dataje družiny MIME „$1” njesmědźa so składować.',
'filetype-unwanted-type'      => "'''\".\$1\"''' je njepožadany datajowy typ. 
{{PLURAL:\$3|Preferowany datajowy typ je|Preferowanej datajowej typaj stej|Preferowane datajowe typy su|Preferowane datajowe typy su}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' njeje dowoleny datajowy typ.
{{PLURAL:\$3|Dowoleny datajowy typ je|Dowolenej datajowej typaj stej|Dowolene datajowe typy su|Dowolene datajowe typy su}} \$2.",
'filetype-missing'            => 'Dataja nima kóncowku (na přikład „.jpg“).',
'large-file'                  => 'Doporuča so, zo dataje wjetše hač $1 njejsu; tuta dataja ma $2.',
'largefileserver'             => 'Dataja je wjetša hač serwer dowoluje.',
'emptyfile'                   => 'Dataja, kotruž sy nahrał, zda so prózdna być. Z přičinu móhł pisanski zmylk w mjenje dataje być. Prošu pruwuj hač chceš ju woprawdźe nahrać.',
'fileexists'                  => 'Dataja z tutym mjenom hižo eksistuje. Jeli kliknješ na „Składować”, so wona přepisuje. Prošu pruwuj <strong><tt>$1</tt></strong> jeli njejsy wěsty hač chceš ju změnić.',
'filepageexists'              => 'Wopisanska strona za tutu dataju bu hižo pola <strong><tt>$1</tt></strong> wutworjena,
ale tuchwilu dataja z tutym mjeno njeeksistuje. Zjeće, kotrež zapodaš, njezjewi so na wopisanskej stronje. Zo by so twoje zjeće tam jewiło, dyrbiš ju manuelnje wobdźěłać.',
'fileexists-extension'        => 'Dataja z podobnym mjenom hižo eksistuje:<br />
Mjeno dataje, kotruž chceš nahrać: <strong><tt>$1</tt></strong><br />
Mjeno eksistowaceje dataje: <strong><tt>$2</tt></strong><br />
Jenož kóncowce rozeznawatej so we wulko- a małopisanju. Prošu wuzwol hinaše mjeno.',
'fileexists-thumb'            => "<center>'''Eksistowacy wobraz'''</center>",
'fileexists-thumbnail-yes'    => 'Dataja zda so minaturka <i>(thumbnail)</i> być. Prošu přepruwuj dataju <strong><tt>$1</tt></strong>.<br />
Jeli je to wobraz w originalnej wulkosći, njetrjebaš minaturku nahrać.',
'file-thumbnail-no'           => 'Mjeno dataje započina so z <strong><tt>$1</tt></strong>. Zda so, zo to je wobraz z redukowanej wulkosću <i>(thumbnail)</i> pokazać.
Jeli maš tutón wobraz z połnym rozeznaćom, nahraj tutón, hewak změń prošu datajowe mjeno.',
'fileexists-forbidden'        => 'Dataja z tutym mjenom hižo eksistuje; prošu dźi wróćo a nahraj tutu dataju z druhim mjenom. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Dataja z tutym mjenom hižo eksistuje w zhromadnej chowarni. Jeli hišće chceš swoju dataju nahrać,  dźi prošu wróćo a wužij nowe mjeno. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Tuta dataja je duplikat {{PLURAL:$1|slědowaceje dataje|slědowaceju datajow|slědowacych datajow|slědowacych datajow}}:',
'successfulupload'            => 'Dataja bu wuspěšnje nahrata',
'uploadwarning'               => 'Warnowanje',
'savefile'                    => 'Dataju składować',
'uploadedimage'               => 'je dataju „[[$1]]” nahrał',
'overwroteimage'              => 'je nowu wersiju dataje „[[$1]]“ nahrał',
'uploaddisabled'              => 'Wodaj, nahraće je znjemóžnjene.',
'uploaddisabledtext'          => 'Nahraće datajow je we {{SITENAME}} znjemóžnjene.',
'uploadscripted'              => 'Dataja wobsahuje HTML- abo skriptowy kod, kotryž móhł so mylnje přez wobhladowak wuwjesć.',
'uploadcorrupt'               => 'Dataja je wobškodźena abo ma wopačnu kóncowku. Prošu přepruwuj dataju a nahraj ju hišće raz.',
'uploadvirus'                 => 'Dataja wirus wobsahuje! Podrobnosće: $1',
'sourcefilename'              => 'Mjeno žórłoweje dataje:',
'destfilename'                => 'Mjeno ciloweje dataje:',
'upload-maxfilesize'          => 'Maksimalna datajowa wulkosć: $1',
'watchthisupload'             => 'Stronu wobkedźbować',
'filewasdeleted'              => 'Dataja z tutym mjenom bu prjedy nahrata a pozdźišo wušmórnjena. Prošu přepruwuj $1 prjedy hač ju znowa składuješ.',
'upload-wasdeleted'           => "'''Kedźbu: Nahrawaš dataju, kotraž bu prjedy wušmórnjena.'''

Prošu přepruwuj dokładnje, hač wospjetowane nahraće směrnicam wotpowěduje.
Za twoju informaciju slěduje protokol wušmórnjenjow z wopodstatnjenjom za předchadne wušmórnjenje:",
'filename-bad-prefix'         => 'Datajowe mjeno započina so z <strong>„$1“</strong>. To je powšitkownje datajowe mjeno, kotrež digitalna kamera zwjetša dawa a kotrež tohodla jara wuprajiwe njeje. Prošu wubjer bóle wuprajiwe mjeno za twoju dataju.',
'filename-prefix-blacklist'   => ' #<!-- Njezměń tutu linku! --> <pre>
# Syntaksa:
#   * Wšo wot znamješka "#" hač ke kóncej linki je komentar
#   * Kóžda njeprózdna linka je prefiks za typiske datajowe mjena,
# kotrež so awtomatisce přez digitalne kamery připokazuja
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # někptre mobilne telefony
IMG # generic
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- Njezměń tutu linku! -->',

'upload-proto-error'      => 'Wopačny protokol',
'upload-proto-error-text' => 'URL dyrbi so z <code>http://</code> abo <code>ftp://</code> započeć.',
'upload-file-error'       => 'Nutřkowny zmylk',
'upload-file-error-text'  => 'Nutřkowny zmylk wustupi při pospytu, nachwilnu dataju na serwerje wutworić. Prošu skontaktuj [[Special:ListUsers/sysop|administratora]].',
'upload-misc-error'       => 'Njeznaty zmylk při nahraću',
'upload-misc-error-text'  => 'Njeznaty zmylk wustupi při nahrawanju. Prošu přepruwuj, hač URL je płaćiwy a přistupny a spytaj hišće raz. Jeli problem dale eksistuje, skontaktuj [[Special:ListUsers/sysop|administratora]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL docpějomny njeje.',
'upload-curl-error6-text'  => 'Podaty URL njehodźeše so docpěć. Prošu přepruwuj, hač URL je korektny a sydło docpějomne.',
'upload-curl-error28'      => 'Překročenje časa při nahrawanju',
'upload-curl-error28-text' => 'Sydło za wotmołwu předołho trjebaše. Prošu pruwuj, hač sydło je docpějomne, čakaj wokomik a spytaj hišće raz. Spytaj hewak w druhim času hišće raz.',

'license'            => 'Licenca:',
'nolicense'          => 'žadyn wuběr',
'license-nopreview'  => '(žadyn přehlad k dispoziciji)',
'upload_source_url'  => ' (płaćiwy, zjawnje docpějomny URL)',
'upload_source_file' => ' (dataja na twojim ličaku)',

# Special:ImageList
'imagelist-summary'     => 'Tuta specialna strona naliči wšě nahrate dataje. Standardnje so naposlědk nahrate dateje cyle horjeka pokazuja. Kliknjo na nadpisma stołpikow móžeš sortěrowanje wobroćić abo po druhich kriterijach rjadować.',
'imagelist_search_for'  => 'Za mjenom wobraza pytać:',
'imgfile'               => 'dataja',
'imagelist'             => 'Lisćina datajow',
'imagelist_date'        => 'datum',
'imagelist_name'        => 'mjeno dataje',
'imagelist_user'        => 'wužiwar',
'imagelist_size'        => 'wulkosć (byte)',
'imagelist_description' => 'wopisanje',

# Image description page
'filehist'                       => 'Wersije dataje',
'filehist-help'                  => 'Klikń na wěsty čas, zo by wersiju dataje w tutym času zwobraznił.',
'filehist-deleteall'             => 'wšě wersije wušmórnyć',
'filehist-deleteone'             => 'tutu wersiju wušmórnyć',
'filehist-revert'                => 'cofnyć',
'filehist-current'               => 'aktualnje',
'filehist-datetime'              => 'Čas',
'filehist-user'                  => 'Wužiwar',
'filehist-dimensions'            => 'Rozeznaće',
'filehist-filesize'              => 'Wulkosć dataje',
'filehist-comment'               => 'Komentar',
'imagelinks'                     => 'Wotkazy',
'linkstoimage'                   => '{{PLURAL:$1|Slědowaca strona wotkazuje|Slědowacej $1 stronje wotkazujetej|Slědowace $1 strony wotkazuja|Slědowacych $1 stronow wotkazuje}} na tutu dataju:',
'nolinkstoimage'                 => 'Njejsu strony, kotrež na tutu dataju wotkazuja.',
'morelinkstoimage'               => '[[Special:WhatLinksHere/$1|Dalše wotkazy]] k tutej dataji wobhladać.',
'redirectstofile'                => '{{PLURAL:$1|Slědowaca dataja pósrednja|Slědowacej $1 pósrědnjatej|Slědowace $1 posrědnjaju|Slěddowacych $1 pósrědnja}} k toś tej dataji dalej:',
'duplicatesoffile'               => '{{PLURAL:$1|Slědowaca dataja je duplikat|Slědowacej $1 dataji stej duplikata|Slědowace $1 dataje su duplikaty|Slědowacych $1 duplikatow je duplikaty}} tuteje dataje:',
'sharedupload'                   => 'Tuta dataja je zhromadne nahraće a móže so přez druhe projekty wužiwać.',
'shareduploadwiki'               => 'Za dalše informacije hlej $1.',
'shareduploadwiki-desc'          => 'Wopisanje na $1 so deleka w zhromadnym składźišću pokazuje.',
'shareduploadwiki-linktext'      => 'stronu datajoweho wopisanja',
'shareduploadduplicate'          => 'Tuta dataja je duplikat $1 z hromadźe wužiwaneho repozitorija.',
'shareduploadduplicate-linktext' => 'druha dataja',
'shareduploadconflict'           => 'Tuta dataja ma samsne mjeno kaž $1 z hromadźe wužiwaneho repozitorija.',
'shareduploadconflict-linktext'  => 'druha dataja',
'noimage'                        => 'Dataja z tutym mjenom njeeksistuje, ale móžeš $1.',
'noimage-linktext'               => 'nahrać',
'uploadnewversion-linktext'      => 'nowu wersiju tuteje dataje nahrać',
'imagepage-searchdupe'           => 'Dwójne dataje pytać',

# File reversion
'filerevert'                => 'Wersiju $1 cofnyć',
'filerevert-legend'         => 'Dataju wróćo stajeć',
'filerevert-intro'          => "Stajiš dataju '''[[Media:$1|$1]]''' na [$4 wersiju wot $2, $3 hodź.] wróćo.",
'filerevert-comment'        => 'Přičina:',
'filerevert-defaultcomment' => 'wróćo stajene na wersiju wot $1, $2 hodź.',
'filerevert-submit'         => 'Cofnyć',
'filerevert-success'        => "'''[[Media:$1|$1]]''' bu na [$4 wersiju wot $2, $3 hodź.] wróćo stajeny.",
'filerevert-badversion'     => 'W zapodatym času žana wersija dataje njeje.',

# File deletion
'filedelete'                  => '„$1“ wušmórnyć',
'filedelete-legend'           => 'Wušmórnju dataju',
'filedelete-intro'            => "Wušmórnješ '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Wušmórnješ wersiju '''[[Media:$1|$1]]''' wot [$4 wot $2, $3 hodź].",
'filedelete-comment'          => 'Přičina:',
'filedelete-submit'           => 'Wušmórnyć',
'filedelete-success'          => "Strona '''„$1“''' bu wušmórnjena.",
'filedelete-success-old'      => "Wersija '''[[Media:$1|$1]]''' wot $2, $3 hodź. bu zničena.",
'filedelete-nofile'           => "'''„$1“''' njeeksistuje na tutym webowym sydle.",
'filedelete-nofile-old'       => "Njeje žana archiwowana wersija '''$1''' z podatymi atributami.",
'filedelete-iscurrent'        => 'Spytaš najnowšu wersiju dataje wušmórnyć. Prošu cofń do toho na staršu wersiju.',
'filedelete-otherreason'      => 'Druha/přidatna přičina:',
'filedelete-reason-otherlist' => 'Druha přičina',
'filedelete-reason-dropdown'  => '*Powšitkowne přičina za wušmórnjenja
** Zranjenje awtorksich prawow
** Dwójna dataja',
'filedelete-edit-reasonlist'  => 'Přičiny za wušmórnjenje wobdźěłać',

# MIME search
'mimesearch'         => 'Pytanje po družinje MIME',
'mimesearch-summary' => 'Na tutej specialnej stronje hodźa so dataje po družinje MIME filtrować. Dyrbiš přeco družinu MIME a podrjadowanu družinu zapodać: <tt>image/jpeg</tt> (hlej stronu wopisanja dataje).',
'mimetype'           => 'Družina MIME:',
'download'           => 'Sćahnyć',

# Unwatched pages
'unwatchedpages' => 'Njewobkedźbowane strony',

# List redirects
'listredirects' => 'Lisćina daleposrědkowanjow',

# Unused templates
'unusedtemplates'     => 'Njewužiwane předłohi',
'unusedtemplatestext' => 'Tuta specialna strona naliči wšě předłohi, kiž so w druhich stronach njewužiwaja. Prošu přepruwuj tež druhe móžne wotkazy na předłohi, prjedy hač je wušmórnješ.',
'unusedtemplateswlh'  => 'Druhe wotkazy',

# Random page
'randompage'         => 'Připadny nastawk',
'randompage-nopages' => 'W tutym mjenowym rumje strony njejsu.',

# Random redirect
'randomredirect'         => 'Připadne daleposrědkowanje',
'randomredirect-nopages' => 'Žane daleposrědkowanja w tutym mjenowym rumje.',

# Statistics
'statistics'             => 'Statistika',
'sitestats'              => 'Statistika {{GRAMMAR:genitiw|{{SITENAME}}}}',
'userstats'              => 'Statistika wužiwarjow',
'sitestatstext'          => "{{PLURAL:$1|Je|Stej|Su|Je}} dohromady {{PLURAL:$1|'''1''' strona|'''$1''' stronje|'''$1''' strony|'''$1''' stronow}} w datowej bance. To zapřija tež diskusijne strony, strony wo {{GRAMMAR:lokatiw|{{SITENAME}}}}, krótke nastawki, daleposrědkowanja a druhe, kotrež najskerje nastawki njejsu.

{{PLURAL:$2|Zwostanje|Zwostanjetej|Zwostanu|Zwostanje}} {{PLURAL:$2|'''1''' strona|'''$2''' stronje|'''$2''' strony|'''$2''' stronow}}, {{PLURAL:$2|kotraž najskerje je|kotrejž najskerje stej|kotrež najskerje su|kotrež najskerje je}} {{PLURAL:$2|woprawdźity nastawk|woprawdźitej nastawkaj|woprawdźite nastawki|woprawdźitych nastawkow}}.

{{PLURAL:$8|Je so 1 dataja nahrała|Stej so '''$8''' dataji nahrałoj|Su so '''$8''' dataje nahrali|Je so '''$8''' datajow nahrało}}.

Běše dohromady '''$3''' {{PLURAL:$3|wobhladanje|wobhladani|wobhladanja|wobhladanjow}} stronow a '''$4''' {{PLURAL:$4|změna|změnje|změny|změnow}} stronow, wot toho zo bu {{SITENAME}} připrawjeny. Bě to přerěznje '''$5''' {{PLURAL:$5|změna|změnje|změny|změnow}} na stronu a '''$6''' {{PLURAL:$6|wobhladanje|wobhladani|wobhladanja|wobhladanjow}} na změnu.

Dołhosć [http://www.mediawiki.org/wiki/Manual:Job_queue rynka nadawkow] je '''$7'''.",
'userstatstext'          => "{{PLURAL:$1|Je '''1''' [[Special:ListUsers|wužiwar]] zregistrowany|Staj '''$1''' [[Special:ListUsers|wužiwarjej]] zregistrowanej|Su '''$1''' [[Special:ListUsers|wužiwarjo]] zregistrowani|Je '''$1''' [[Special:ListUsers|wužiwarjow]] zregistrowanych}}, '''$2''' (abo '''$4%''') z nich {{PLURAL:$2|je|staj|su|je}} $5.",
'statistics-mostpopular' => 'Najhusćišo wopytowane strony',

'disambiguations'      => 'Rozjasnjenja wjacezmyslnosće',
'disambiguationspage'  => 'Template:Wjacezmyslnosć',
'disambiguations-text' => "Slědowace strony na '''rozjasnjenje wjacezmyslnosće''' wotkazuja. Měli město toho na poprawnu stronu wotkazać.<br />Strona so jako rozjasnjenje wjacezmyslnosće zarjaduje, jeli předłohu wužiwa, na kotruž so wot [[MediaWiki:Disambiguationspage]] wotkazuje.",

'doubleredirects'            => 'Dwójne daleposrědkowanja',
'doubleredirectstext'        => 'Kóžda rjadka wobsahuje wotkazy k prěnjemu a druhemu daleposrědkowanju kaž tež k prěnjej lince druheho daleposrědkowanja, kotraž zwjetša woprawdźity cil strony podawa, na kotryž prěnje daleposrědkowanje měło pokazać.',
'double-redirect-fixed-move' => '[[$1]] bu přesunjeny, je nětko daleposrědkowanje do [[$2]]',
'double-redirect-fixer'      => 'Porjedźer daleposrědkowanjow',

'brokenredirects'        => 'Skóncowane daleposrědkowanja',
'brokenredirectstext'    => 'Slědowace daleposrědkowanja wotkazuja na njeeksistowace strony:',
'brokenredirects-edit'   => '(wobdźěłać)',
'brokenredirects-delete' => '(wušmórnyć)',

'withoutinterwiki'         => 'Strony bjez mjezyrěčnych wotkazow',
'withoutinterwiki-summary' => 'Sćěhowace strony njewotkazuja na druhe rěčne wersije:',
'withoutinterwiki-legend'  => 'Prefiks',
'withoutinterwiki-submit'  => 'Pokazać',

'fewestrevisions' => 'Strony z najmjenje wersijemi',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bajt|bajtaj|bajty|bajtow}}',
'ncategories'             => '$1 {{PLURAL:$1|jedna kategorija|kategoriji|kategorije|kategorijow}}',
'nlinks'                  => '$1 {{PLURAL:$1|wotkaz|wotkazaj|wotkazy|wotkazow}}',
'nmembers'                => '{{PLURAL:$1|$1 čłon|$1 čłonaj|$1 čłony|$1 čłonow}}',
'nrevisions'              => '$1 {{PLURAL:$1|wobdźěłanje|wobdźěłani|wobdźěłanja|wobdźěłanjow}}',
'nviews'                  => '$1 {{PLURAL:$1|jedyn wopyt|wopytaj|wopyty|wopytow}}',
'specialpage-empty'       => 'Tuchwilu žane zapiski.',
'lonelypages'             => 'Wosyroćene strony',
'lonelypagestext'         => 'Na slědowace strony druhe strony w tutym wikiju njewotkazuja:',
'uncategorizedpages'      => 'Njekategorizowane strony',
'uncategorizedcategories' => 'Njekategorizowane kategorije',
'uncategorizedimages'     => 'Njekategorizowane dataje',
'uncategorizedtemplates'  => 'Njekategorizowane předłohi',
'unusedcategories'        => 'Njewužiwane kategorije',
'unusedimages'            => 'Njewužiwane dataje',
'popularpages'            => 'Často wopytowane strony',
'wantedcategories'        => 'Požadane kategorije',
'wantedpages'             => 'Požadane strony',
'missingfiles'            => 'Falowace dataje',
'mostlinked'              => 'Z najwjace stronami zwjazane strony',
'mostlinkedcategories'    => 'Z najwjace stronami zwjazane kategorije',
'mostlinkedtemplates'     => 'Najhusćišo wužiwane předłohi',
'mostcategories'          => 'Strony z najwjace kategorijemi',
'mostimages'              => 'Z najwjace stronami zwjazane dataje',
'mostrevisions'           => 'Nastawki z najwjace wersijemi',
'prefixindex'             => 'Wšě nastawki (z prefiksom)',
'shortpages'              => 'Krótke nastawki',
'longpages'               => 'Dołhe nastawki',
'deadendpages'            => 'Nastawki bjez wotkazow',
'deadendpagestext'        => 'Slědowace strony njejsu z druhimi stronami w tutym wikiju zwjazane.',
'protectedpages'          => 'Škitane strony',
'protectedpages-indef'    => 'Jenož strony z njewobmjezowanym škitom',
'protectedpagestext'      => 'Tuta specialna strona naliči wšě strony, kotrež su přećiwo přesunjenju abo wobdźěłowanju škitane.',
'protectedpagesempty'     => 'Tuchwilu žane.',
'protectedtitles'         => 'Škitane titule',
'protectedtitlestext'     => 'Slědowace titule su přećiwo wutworjenju škitane',
'protectedtitlesempty'    => 'Žane titule njejsu tuchwilu z tutymi parametrami škitane.',
'listusers'               => 'Lisćina wužiwarjow',
'newpages'                => 'Nowe strony',
'newpages-username'       => 'Wužiwarske mjeno:',
'ancientpages'            => 'Najstarše nastawki',
'move'                    => 'Přesunyć',
'movethispage'            => 'Stronu přesunyć',
'unusedimagestext'        => 'Prošu wobkedźbuj, zo druhe websydła móža k dataji z direktnym URL wotkazować a su hišće tu naspomnjene, hačrunjež so hižo aktiwnje wužiwaja.',
'unusedcategoriestext'    => 'Slědowace kategorije eksistuja, hačrunjež žana druha strona abo kategorija je njewužiwa.',
'notargettitle'           => 'Žadyn cil',
'notargettext'            => 'Njejsy cilowu stronu abo wužiwarja podał, zo by funkciju wuwjesć móhł.',
'nopagetitle'             => 'Žana tajka cilowa strona',
'nopagetext'              => 'Cilowa strona, kotruž sće podał, njeeksistuje.',
'pager-newer-n'           => '{{PLURAL:$1|nowši 1|nowšej $1|nowše $1|nowšich $1}}',
'pager-older-n'           => '{{PLURAL:$1|starši 1|staršej $1|starše $1|staršich $1}}',
'suppress'                => 'Dohladowanje',

# Book sources
'booksources'               => 'Pytanje po ISBN',
'booksources-search-legend' => 'Žórła za knihi pytać',
'booksources-go'            => 'Pytać',
'booksources-text'          => 'To je lisćina wotkazow k druhim sydłam, kotrež nowe a trjebane knihi předawaja. Tam móžeš tež dalše informacije wo knihach dóstać, kotrež pytaš:',

# Special:Log
'specialloguserlabel'  => 'Wužiwar:',
'speciallogtitlelabel' => 'Strona:',
'log'                  => 'Protokole',
'all-logs-page'        => 'Wšě protokole',
'log-search-legend'    => 'Protokole přepytować',
'log-search-submit'    => 'OK',
'alllogstext'          => 'Kombinowane zwobraznjenje wšěch k dispozicij stejacych protokolow w {{GRAMMAR:lokatiw|{{SITENAME}}}}. Móžeš napohlad wobmjezować, wuběrajo typ protokola, wužiwarske mjeno (dźiwajo na wulkopisanje) abo potrjechu stronu (tež dźiwajo na wulkopisanje).',
'logempty'             => 'Žane wotpowědowace zapiski w protokolu.',
'log-title-wildcard'   => 'Titul započina so z …',

# Special:AllPages
'allpages'          => 'Wšě nastawki',
'alphaindexline'    => '$1 do $2',
'nextpage'          => 'přichodna strona ($1)',
'prevpage'          => 'předchadna strona ($1)',
'allpagesfrom'      => 'Strony pokazać, započinajo z:',
'allarticles'       => 'Wšě nastawki',
'allinnamespace'    => 'Wšě strony (mjenowy rum $1)',
'allnotinnamespace' => 'Wšě strony (nic w mjenowym rumje $1)',
'allpagesprev'      => 'Předchadne',
'allpagesnext'      => 'Přichodne',
'allpagessubmit'    => 'Pokazać',
'allpagesprefix'    => 'Strony pokazać z prefiksom:',
'allpagesbadtitle'  => 'Mjeno strony, kotrež sy zapodał, njebě płaćiwe. Měješe pak mjezyrěčny, pak mjezywikijowy prefiks abo wobsahowaše jedne abo wjace znamješkow, kotrež w titlach dowolene njejsu.',
'allpages-bad-ns'   => 'Mjenowy rum „$1" w {{grammar:lokatiw|{{SITENAME}}}} njeeksistuje.',

# Special:Categories
'categories'                    => 'Kategorije',
'categoriespagetext'            => 'Slědowace kategorije wobsahuja strony abo medije.
[[Special:UnusedCategories|Njewužiwane kategorije]] so tu njepokazuja.
Hlej tež [[Special:WantedCategories|požadane kategorije]].',
'categoriesfrom'                => 'Kategorije pokazać, započinajo z:',
'special-categories-sort-count' => 'Po ličbje sortěrować',
'special-categories-sort-abc'   => 'Alfabetisce sortěrować',

# Special:ListUsers
'listusersfrom'      => 'Započinajo z:',
'listusers-submit'   => 'Pokazać',
'listusers-noresult' => 'Njemóžno wužiwarjow namakać. Prošu wobkedźbuj, zo so mało- abo wulkopisanje na wotprašowanje wuskutkuje.',

# Special:ListGroupRights
'listgrouprights'          => 'Prawa wužiwarskeje skupiny',
'listgrouprights-summary'  => 'Slěduje lisćina wužiwarskich skupinow na tutej wikiju z jich wotpowědnymi přistupnymi prawami. Tu móžeš [[{{MediaWiki:Listgrouprights-helppage}}|dalše informacije]] wo jednotliwych prawach namakać.',
'listgrouprights-group'    => 'Skupina',
'listgrouprights-rights'   => 'Prawa',
'listgrouprights-helppage' => 'Help:Skupinske prawa',
'listgrouprights-members'  => '(lisćina čłonow)',

# E-mail user
'mailnologin'     => 'Njejsy přizjewjeny.',
'mailnologintext' => 'Dyrbiš [[Special:UserLogin|přizjewjeny]] być a płaćiwu e-mejlowu adresu w swojich [[Special:Preferences|nastajenjach]] měć, zo by druhim wužiwarjam mejlki pósłać móhł.',
'emailuser'       => 'Wužiwarjej mejlku pósłać',
'emailpage'       => 'Wužiwarjej mejlku pósłać',
'emailpagetext'   => 'Jeli wužiwar je płaćiwu e-mejlowu adresu do swojich nastajenjow zapodał, budźe formular jednotliwu powěsć słać.
E-mejlowa adresa, kotruž sy w [[Special:Preferences|swojich wužiwarskich nastajenjach]] zapodał zjewi so jako adresa "Wot" e-mejlki, tak zo přijimowar móže ći direktnje wotmołwić.',
'usermailererror' => 'E-mejlowy objekt je zmylk wróćił:',
'defemailsubject' => 'Powěsć z {{grammar:genitiw|{{SITENAME}}}}',
'noemailtitle'    => 'Žana e-mejlowa adresa podata',
'noemailtext'     => 'Tutón wužiwar njeje płaćiwu e-mejlowu adresu podał abo je so rozsudźił, zo nochce mejlki druhich wužiwarjow dóstać.',
'emailfrom'       => 'Wot:',
'emailto'         => 'Komu:',
'emailsubject'    => 'Tema:',
'emailmessage'    => 'Powěsć:',
'emailsend'       => 'Wotesłać',
'emailccme'       => 'E-mejluj mi kopiju mojeje powěsće.',
'emailccsubject'  => 'Kopija twojeje powěsće wužiwarjej $1: $2',
'emailsent'       => 'Mejlka wotesłana',
'emailsenttext'   => 'Twoja mejlka bu wotesłana.',
'emailuserfooter' => 'Tuta e-mejlka bu z pomocu funkcije "Wužiwarjej mejlku pósłać" na {{SITENAME}} wot $1 do $2 pósłana.',

# Watchlist
'watchlist'            => 'Wobkedźbowanki',
'mywatchlist'          => 'Wobkedźbowanki',
'watchlistfor'         => '(za wužiwarja <b>$1</b>)',
'nowatchlist'          => 'Nimaš žane strony w swojich wobkedźbowankach.',
'watchlistanontext'    => 'Dyrbiš so $1, zo by swoje wobkedźbowanki wobhladać abo wobdźěłać móhł.',
'watchnologin'         => 'Njejsy přizjewjeny.',
'watchnologintext'     => 'Dyrbiš [[Special:UserLogin|přizjewjeny]] być, zo by swoje wobkedźbowanki změnić móhł.',
'addedwatch'           => 'Strona bu wobkedźbowankam přidata.',
'addedwatchtext'       => "Strona [[:$1]] bu k twojim [[Special:Watchlist|wobkedźbowankam]] přidata.
Přichodne změny tuteje strony a přisłušneje diskusijneje strony budu so tam nalistować a strona so '''w tučnym pismje''' w [[Special:RecentChanges|lisćinje aktualnych změnach]] zjewi, zo by so wosnadniło ju wubrać.

Jeli chceš stronu pozdźišo ze swojich wobkedźbowankow wotstronić, klikń na rajtark „njewobkedźbować” horjeka na tutej stronje.",
'removedwatch'         => 'Strona bu z wobkedźbowankow wotstronjena',
'removedwatchtext'     => 'Strona [[:$1]] bu z wobkedźbowankow wotstronjena.',
'watch'                => 'wobkedźbować',
'watchthispage'        => 'stronu wobkedźbować',
'unwatch'              => 'njewobkedźbować',
'unwatchthispage'      => 'wobkedźbowanje skónčić',
'notanarticle'         => 'njeje nastawk',
'notvisiblerev'        => 'Wersija bu wušmórnjena',
'watchnochange'        => 'Žana z twojich wobkedźbowanych stron njebu w podatej dobje wobdźěłana.',
'watchlist-details'    => '{{PLURAL:$1|$1 wobkedźbowana strona|$1 wobkedźbowanej stronje|$1 wobkedźbowane strony|$1 wobkedźbowanych stronow}}, diskusijne strony wuwzate.',
'wlheader-enotif'      => '* E-mejlowe zdźělenje je zmóžnjene.',
'wlheader-showupdated' => '* Strony, kotrež buchu po twojim poslednim wopyće změnjene so <b>tučne</b> pokazuja.',
'watchmethod-recent'   => 'Aktualne změny za wobkedźbowane strony přepruwować',
'watchmethod-list'     => 'Wobkedźbowanki za aktualnymi změnami přepruwować',
'watchlistcontains'    => 'Maš $1 {{PLURAL:$1|stronu|stronje|strony|stronow}} w swojich wobkedźbowankach.',
'iteminvalidname'      => 'Problem ze zapiskom „$1“, njepłaćiwe mjeno.',
'wlnote'               => 'Deleka {{PLURAL:$1|je poslednja|stej poslednjej|su poslednje|su poslednje}} $1 {{PLURAL:$1|změna|změnje|změny|změnow}} za poslednje <b>$2</b> hodź.',
'wlshowlast'           => 'Poslednje $1 hodź. - $2 dnjow - $3 pokazać',
'watchlist-show-bots'  => 'změny botow pokazać',
'watchlist-hide-bots'  => 'změny botow schować',
'watchlist-show-own'   => 'moje změny pokazać',
'watchlist-hide-own'   => 'moje změny schować',
'watchlist-show-minor' => 'snadne změny pokazać',
'watchlist-hide-minor' => 'snadne změny schować',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Wobkedźbuju…',
'unwatching' => 'Njewobkedźbuju…',

'enotif_mailer'                => '{{SITENAME}} E-mejlowe zdźělenje',
'enotif_reset'                 => 'Wšě strony jako wopytane woznamjenić',
'enotif_newpagetext'           => 'To je nowa strona.',
'enotif_impersonal_salutation' => 'wužiwar {{GRAMMAR:genitiw|{{SITENAME}}}}',
'changed'                      => 'změnjena',
'created'                      => 'wutworjena',
'enotif_subject'               => '[{{SITENAME}}] Strona „$PAGETITLE” bu přez wužiwarja $PAGEEDITOR $CHANGEDORCREATED.',
'enotif_lastvisited'           => 'Hlej $1 za wšě změny po twojim poslednim wopyće.',
'enotif_lastdiff'              => 'Hlej $1 za tutu změnu.',
'enotif_anon_editor'           => 'anonymny wužiwar $1',
'enotif_body'                  => 'Luby $WATCHINGUSERNAME,<br />

Strona we {{GRAMMAR:lokatiw|{{SITENAME}}}} z mjenom $PAGETITLE bu dnja $PAGEEDITDATE wot $PAGEEDITOR $CHANGEDORCREATED,
hlej $PAGETITLE_URL za aktualnu wersiju.

$NEWPAGE

Zjeće wobdźěłaćerja běše: $PAGESUMMARY $PAGEMINOREDIT

Skontaktuj wobdźěłarja:
e-mejl: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Njebudu žane druhe zdźělenki w padźe dalšich změnow, chibazo wopytaš tutu stronu.
Móžeš tež zdźělenske marki za wšě swoje wobkedźbowane strony we swojich wobkedźbowankach wróćo stajić.

               Twój přećelny zdźělenski system {{GRAMMAR:genitiw|{{SITENAME}}}}

--
Zo by nastajenja twojich wobkedźbowankow změnił, wopytaj
{{fullurl:{{ns:special}}:Watchlist/edit}}

Wospjetne prašenja a dalša pomoc:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Stronu wušmórnyć',
'confirm'                     => 'Wobkrućić',
'excontent'                   => "wobsah běše: '$1'",
'excontentauthor'             => "wobsah bě: '$1' (a jenički wobdźěłowar bě '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "wobsah do wuprózdnjenja běše: '$1'",
'exblank'                     => 'strona běše prózdna',
'delete-confirm'              => '„$1“ wušmórnyć',
'delete-legend'               => 'Wušmórnyć',
'historywarning'              => 'KEDŹBU: Strona, kotruž chceš wušmórnyć, ma stawizny:',
'confirmdeletetext'           => 'Sy so rozsudźił stronu ze jeje stawiznami wušmórnić.
Prošu potwjerdź, zo maš wotpohlad to činić, zo rozumiš sćěwki a zo to wotpowědujo [[{{MediaWiki:Policy-url}}|zasadam tutoho wikija]] činiš.',
'actioncomplete'              => 'Dokónčene',
'deletedtext'                 => 'Strona „<nowiki>$1</nowiki>” bu wušmórnjena. Hlej $2 za lisćinu aktualnych wušmórnjenjow.',
'deletedarticle'              => 'je stronu [[$1]] wušmórnył.',
'suppressedarticle'           => '"[[$1]]" potłóčeny',
'dellogpage'                  => 'Protokol wušmórnjenjow',
'dellogpagetext'              => 'Deleka je lisćina najaktualnišich wušmórnjenjow.',
'deletionlog'                 => 'Protokol wušmórnjenjow',
'reverted'                    => 'Na staršu wersiju cofnjene',
'deletecomment'               => 'Přičina wušmórnjenja:',
'deleteotherreason'           => 'Druha/přidatna přičina:',
'deletereasonotherlist'       => 'Druha přičina',
'deletereason-dropdown'       => '*Zwučene přičiny za wušmórnjenje
** Požadanje awtora
** Zranjenje copyrighta
** Wandalizm',
'delete-edit-reasonlist'      => 'Přičiny za wušmórnjenje wobdźěłać',
'delete-toobig'               => 'Tuta strona ma z wjace hač $1 {{PLURAL:$1|wersiju|wersijomaj|wersijemi|wersijemi}} wulke wobdźěłanske stawizny. Wušmórnjenje tajkich stronow bu wobmjezowane, zo by připadne přetorhnjenje {{SITENAME}} wobešło.',
'delete-warning-toobig'       => 'Tuta strona ma z wjace hač $1 {{PLURAL:$1|wersiju|wersijomaj|wersijemi|wersijemi}} wulke wobdźěłanske stawizny. Wušmórnjenje móže operacije datoweje banki {{SITENAME}} přetorhnyć; pokročuj z kedźbliwosću.',
'rollback'                    => 'Změny cofnyć',
'rollback_short'              => 'Cofnyć',
'rollbacklink'                => 'Cofnyć',
'rollbackfailed'              => 'Cofnjenje njeporadźiło',
'cantrollback'                => 'Njemóžno změnu cofnyć; strona nima druhich awtorow.',
'alreadyrolled'               => 'Njemóžno poslednu změnu [[:$1]] přez wužiwarja [[User:$2|$2]] ([[User talk:$2|Diskusija]] | [[Special:Contributions/$2|{{int:contribslink}}]]) cofnyć; něchtó druhi je stronu wobdźěłał abo změnu hižo cofnył.

Poslednja změna bě wot wužiwarja [[User:$3|$3]] ([[User talk:$3|Diskusija]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Komentar wobdźěłanja běše: „<i>$1</i>”.', # only shown if there is an edit comment
'revertpage'                  => 'Změny [[Special:Contributions/$2|$2]] ([[User talk:$2|Diskusija]]) cofnjene a nawróćene k poslednjej wersiji wužiwarja [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Změny wužiwarja $1 cofnjene; wróćo na wersiju wužiwarja $2.',
'sessionfailure'              => 'Zda so, zo je problem z twojim přizjewjenjom; tuta akcija bu wěstosće dla přećiwo zadobywanju do posedźenja znjemóžniła. Prošu klikń na "Wróćo" a začitaj stronu, z kotrejež přińdźeš, znowa; potom spytaj hišće raz.',
'protectlogpage'              => 'Protokol škita',
'protectlogtext'              => 'To je protokol škitanych stronow a zběhnjenja škita.
Hlej [[Special:ProtectedPages|tutu specialnu stronu]] za lisćinu škitanych stron.',
'protectedarticle'            => 'je stronu [[$1]] škitał',
'modifiedarticleprotection'   => 'je škit strony [[$1]] změnił',
'unprotectedarticle'          => 'je škit strony [[$1]] zběhnył',
'protect-title'               => 'Stronu „$1” škitać',
'protect-legend'              => 'Škit wobkrućić',
'protectcomment'              => 'Přičina za škitanje:',
'protectexpiry'               => 'Čas škita:',
'protect_expiry_invalid'      => 'Njepłaćiwy čas spadnjenja.',
'protect_expiry_old'          => 'Čas škita leži w zańdźenosći.',
'protect-unchain'             => 'Škit přećiwo přesunjenju změnić',
'protect-text'                => 'Tu móžeš status škita strony <b><nowiki>$1</nowiki></b> wobhladać a změnić.',
'protect-locked-blocked'      => 'Njemóžeš škit strony změnič, dokelž twoje konto je zablokowane. Tu widźiš aktualne škitne nastajenja za stronu<strong>„$1“:</strong>',
'protect-locked-dblock'       => 'Datowa banka je zawrjena, tohodla njemóžeš škit strony změnić. Tu widźiš aktualne škitne nastajenja za stronu<strong>„$1“:</strong>',
'protect-locked-access'       => 'Nimaš trěbne prawa, zo by škit strony změnił. Tu widźiš aktualne škitne nastajenja za stronu<strong>„$1“:</strong>',
'protect-cascadeon'           => 'Tuta strona je tuchwilu škitana, dokelž je w {{PLURAL:$1|slědowacej stronje|slědowacych stronach}} zapřijata, {{PLURAL:$1|kotraž je|kotrež su}} přez kaskadowu opciju {{PLURAL:$1|škitana|škitane}}. Móžeš škitowy status strony změnić, to wšak wliw na kaskadowy škit nima.',
'protect-default'             => '(standard)',
'protect-fallback'            => 'Prawo "$1" trěbne.',
'protect-level-autoconfirmed' => 'jenož přizjewjeni wužiwarjo',
'protect-level-sysop'         => 'jenož administratorojo',
'protect-summary-cascade'     => 'kaskadowacy',
'protect-expiring'            => 'spadnje $1 (UTC)',
'protect-cascade'             => 'Kaskadowacy škit – wšě w tutej stronje zapřijate strony so škituja.',
'protect-cantedit'            => 'Njemóžeš škitowe runiny tuteje strony změnić, dokelž nimaš dowolnosć, zo by ju wobdźěłał.',
'restriction-type'            => 'Škitowy status',
'restriction-level'           => 'Runina škita:',
'minimum-size'                => 'Minimalna wulkosć:',
'maximum-size'                => 'Maksimalna wulkosć:',
'pagesize'                    => '(bajtow)',

# Restrictions (nouns)
'restriction-edit'   => 'wobdźěłać',
'restriction-move'   => 'přesunyć',
'restriction-create' => 'Wutworić',
'restriction-upload' => 'Nahrać',

# Restriction levels
'restriction-level-sysop'         => 'dospołnje škitany',
'restriction-level-autoconfirmed' => 'połškitany (móže so jenož přez přizjewjenych wužiwarjow wobdźěłać, kiž nowačcy njejsu)',
'restriction-level-all'           => 'wšě',

# Undelete
'undelete'                     => 'Wušmórnjenu stronu wobnowić',
'undeletepage'                 => 'Wušmórnjene strony wobnowić',
'undeletepagetitle'            => "'''Slědowace wudaće pokazuje wušmórnjene wersije wot [[:$1]]'''.",
'viewdeletedpage'              => 'Wušmórnjene strony wobhladać',
'undeletepagetext'             => 'Tute strony buchu wušmórnjene, su pak hišće w datowej bance składowane a móža so wobnowić.',
'undelete-fieldset-title'      => 'Wersije wobnowić',
'undeleteextrahelp'            => "Zo by wšě stawizny strony wobnowił, wostaj prošu wšě kontrolowe kašćiki njewubrane a klikń na '''''Wobnowić'''''. Zo by selektiwne wobnowjenje přewjedł, wubjer kašćiki, kotrež wersijam wotpowěduja, kotrež maja so wobnowić a klikń na '''''Wobnowić'''''.
Kliknjenje na '''''Wróćo stajić''''' komentarne polo a wšě kontrolowe kašćiki wuprózdni.",
'undeleterevisions'            => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}} {{PLURAL:$1|archiwowana|archiwowanej|archiwowane|archiwowane}}',
'undeletehistory'              => 'Jeli tutu stronu wobnowiš, so wšě (tež prjedy wušmórnjene) wersije zaso do stawiznow wobnowja. Jeli bu po wušmórnjenju nowa strona ze samsnym mjenom wutworjena, budu so wobnowjene wersije w prjedawšich stawiznach jewić.',
'undeleterevdel'               => 'Wobnowjenje so njepřewjedźe, jeli je najwyša strona docpěta abo datajowa wersija budźe so zdźěla wušmórnje. 
W tutym padźe dyrbiš najnowšu wušmórnjenu wersiju znjemóžnić abo pokazać.',
'undeletehistorynoadmin'       => 'Strona bu wušmórnjena. Přičina za wušmórnjenje so deleka w zjeću pokazuje, zhromadnje z podrobnosćemi wužiwarjow, kotřiž běchu tutu stronu do zničenja wobdźěłali. Tuchwilny wobsah strony je jenož administratoram přistupny.',
'undelete-revision'            => 'Wušmórnjena wersija strony $1 - $2, $3:',
'undeleterevision-missing'     => 'Njepłaćiwa abo pobrachowaca wersija. Pak je wotkaz wopačny, pak bu wotpowědna wersija z archiwa wobnowjena abo wotstronjena.',
'undelete-nodiff'              => 'Předchadna wersija njeeksistuje.',
'undeletebtn'                  => 'Wobnowić',
'undeletelink'                 => 'wobnowić',
'undeletereset'                => 'Cofnyć',
'undeletecomment'              => 'Přičina:',
'undeletedarticle'             => 'Strona „$1” bu wuspěšnje wobnowjena.',
'undeletedrevisions'           => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}} {{PLURAL:$1|wobnowjena|wobnowjenej|wobnowjene|wobnowjene}}',
'undeletedrevisions-files'     => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}} a $2 {{PLURAL:$2|dataja|dataji|dataje|datajow}} {{PLURAL:$2|wobnowjena|wobnowjenej|wobnowjene|wobnowjene}}',
'undeletedfiles'               => '$1 {{PLURAL:$1|dataja|dataji|dataje|datajow}} {{PLURAL:$1|wobnowjena|wobnowjenej|wobnowjene|wobnowjene}}.',
'cannotundelete'               => 'Wobnowjenje zwrěšćiło; něchtó druhi je stronu prjedy wobnowił.',
'undeletedpage'                => "<big>'''Strona $1 bu z wuspěchom wobnowjena.'''</big>

Hlej [[Special:Log/delete|protokol]] za lisćinu aktualnych wušmórnjenjow a wobnowjenjow.",
'undelete-header'              => 'Hlej [[Special:Log/delete|protokol wušmórnjenjow]] za njedawno wušmórnjene strony.',
'undelete-search-box'          => 'Wušmórnjene strony pytać',
'undelete-search-prefix'       => 'Strony pokazać, kotrež započinaja so z:',
'undelete-search-submit'       => 'Pytać',
'undelete-no-results'          => 'Žane přihódne strony w archiwje namakane.',
'undelete-filename-mismatch'   => 'Datajowa wersija z časowym kołkom $1 njeda so wobnowić: Datajowej mjenje njehodźitej so jedne k druhemu.',
'undelete-bad-store-key'       => 'Datajowa wersija z časowym kołkom $1 njeda so wobnowić: dataja před zničenjom hižo njeeksistowaše.',
'undelete-cleanup-error'       => 'Zmylk při wušmórnjenju njewužita wersija $1 z archiwa.',
'undelete-missing-filearchive' => 'Dataja z archiwowym ID $1 njeda so wobnowić, dokelž w datowej bance njeje. Snano bu wona hižo wobnowjena.',
'undelete-error-short'         => 'Zmylk při wobnowjenju dataje $1',
'undelete-error-long'          => 'Buchu zmylki při wobnowjenju dataje zwěsćene:

$1',

# Namespace form on various pages
'namespace'      => 'Mjenowy rum:',
'invert'         => 'Wuběr wobroćić',
'blanknamespace' => '(Nastawki)',

# Contributions
'contributions' => 'Přinoški wužiwarja',
'mycontris'     => 'Moje přinoški',
'contribsub2'   => 'za wužiwarja $1 ($2)',
'nocontribs'    => 'Žane změny, kotrež podatym kriterijam wotpowěduja.',
'uctop'         => '(aktualnje)',
'month'         => 'wot měsaca (a do toho):',
'year'          => 'wot lěta (a do toho):',

'sp-contributions-newbies'     => 'jenož přinoški nowačkow pokazać',
'sp-contributions-newbies-sub' => 'Za nowačkow',
'sp-contributions-blocklog'    => 'protokol zablokowanjow',
'sp-contributions-search'      => 'Přinoški pytać',
'sp-contributions-username'    => 'IP-adresa abo wužiwarske mjeno:',
'sp-contributions-submit'      => 'OK',

# What links here
'whatlinkshere'            => 'Što wotkazuje sem',
'whatlinkshere-title'      => 'Strony, kotrež na „$1“ wotkazuja',
'whatlinkshere-page'       => 'Strona:',
'linklistsub'              => '(Lisćina wotkazow)',
'linkshere'                => "Sćěhowace strony na stronu '''[[:$1]]''' wotkazuja:",
'nolinkshere'              => "Žane strony na '''[[:$1]]''' njewotkazuja.",
'nolinkshere-ns'           => "Žane strony njewotkazuja na '''[[:$1]]''' we wubranym mjenowym rumje.",
'isredirect'               => 'daleposrědkowanje',
'istemplate'               => 'zapřijeće předłohi',
'isimage'                  => 'wobrazowy wotkaz',
'whatlinkshere-prev'       => '{{PLURAL:$1|předchadny|předchadnej|předchadne|předchadne $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|přichodny|přichodnej|přichodne|přichodne $1}}',
'whatlinkshere-links'      => '← wotkazy',
'whatlinkshere-hideredirs' => 'Daleposrědkowanja $1',
'whatlinkshere-hidetrans'  => 'Zapřijeća $1',
'whatlinkshere-hidelinks'  => 'Wotkazy $1',
'whatlinkshere-hideimages' => 'wobrazowe wotkazy $1',
'whatlinkshere-filters'    => 'Filtry',

# Block/unblock
'blockip'                         => 'Wužiwarja zablokować',
'blockip-legend'                  => 'Wužiwarja blokować',
'blockiptext'                     => 'Wužij slědowacy formular deleka, zo by pisanski přistup za podatu IP-adresu abo wužiwarske mjeno blokował. To měło so jenož stać, zo by wandalizmej zadźěwało a woptpowědujo [[{{MediaWiki:Policy-url}}|zasadam]]. Zapodaj deleka přičinu (na př. citujo wosebite strony, kotrež běchu z woporom wandalizma).',
'ipaddress'                       => 'IP-adresa',
'ipadressorusername'              => 'IP-adresa abo wužiwarske mjeno',
'ipbexpiry'                       => 'Spadnjenje',
'ipbreason'                       => 'Přičina',
'ipbreasonotherlist'              => 'Druha přičina',
'ipbreason-dropdown'              => '*powšitkowne přičiny
** wandalizm
** wutworjenje njezmyslnych stronow
** linkspam
** wobobinske nadběhi
*specifiske přičiny
** njepřihódne wužiwarske mjeno
** znowapřizjewjenje na přeco zablokowaneho wužiwarja
** proksy, wandalizma jednotliwych wužiwarjow dla dołhodobnje zablokowany',
'ipbanononly'                     => 'Jenož anonymnych wužiwarjow zablokować',
'ipbcreateaccount'                => 'Wutworjenju nowych kontow zadźěwać',
'ipbemailban'                     => 'Wotpósłanje mejlkow znjemóžnić',
'ipbenableautoblock'              => 'IP-adresy blokować kiž buchu přez tutoho wužiwarja hižo wužiwane kaž tež naslědne adresy, z kotrychž so wobdźěłanje pospytuje',
'ipbsubmit'                       => 'Wužiwarja zablokować',
'ipbother'                        => 'Druha doba',
'ipboptions'                      => '1 hodźinu:1 hour,2 hodźinje:2 hours, 6 hodźiny:6 hours,1 dźeń:1 day,3 dny:3 days,1 tydźeń:1 week,2 njedźeli:2 weeks,1 měsać:1 month,3 měsacy:3 months,6 měsacow:6 months,1 lěto:1 year,na přeco:indefinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'druha doba (jendźelsce)',
'ipbotherreason'                  => 'Druha/přidatna přičina:',
'ipbhidename'                     => 'Wužiwarske mjeno/IP-adresu w protokolu zablokowanjow, w lisćinje aktiwnych zablokowanjow a w zapisu wužiwarjow schować.',
'ipbwatchuser'                    => 'Wužiwarsku a diskusijnu stronu tutoho wužiwarja wobkedźbować',
'badipaddress'                    => 'Njepłaćiwa IP-adresa',
'blockipsuccesssub'               => 'Zablokowanje wuspěšne',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]] bu zablokowany.
<br />Hlej [[Special:IPBlockList|lisćinu blokowanjow IP]], zo by zablokowanjow pruwował.',
'ipb-edit-dropdown'               => 'přičiny zablokowanjow wobdźěłać',
'ipb-unblock-addr'                => 'zablokowanje wužiwarja „$1“ zběhnyć',
'ipb-unblock'                     => 'zablokowanje wužiwarja abo IP-adresy zběhnyć',
'ipb-blocklist-addr'              => 'aktualne zablokowanja za wužiwarja „$1“ zwobraznić',
'ipb-blocklist'                   => 'tuchwilne blokowanja zwobraznić',
'unblockip'                       => 'Zablokowanje zběhnyć',
'unblockiptext'                   => 'Wužij formular deleka, zo by blokowanje IP-adresy abo wužiwarskeho mjena zběhnył.',
'ipusubmit'                       => 'Zablokowanje zběhnyć',
'unblocked'                       => 'Blokowanje wužiwarja [[User:$1|$1]] zběhnjene',
'unblocked-id'                    => 'Blokowanje ID $1 bu zběhnjene.',
'ipblocklist'                     => 'Zablokowane IP-adresy a wužiwarske mjena',
'ipblocklist-legend'              => 'Pytanje za zablokowanym wužiwarjom',
'ipblocklist-username'            => 'Wužiwarske mjeno abo IP-adresa:',
'ipblocklist-submit'              => 'Pytać',
'blocklistline'                   => '$1, $2 je wužiwarja $3 zablokował ($4)',
'infiniteblock'                   => 'na přeco',
'expiringblock'                   => 'hač do $1',
'anononlyblock'                   => 'jenož anonymnych blokować',
'noautoblockblock'                => 'awtoblokowanje znjemóžnjene',
'createaccountblock'              => 'wutworjenje wužiwarskich kontow znjemóžnjene',
'emailblock'                      => 'Wotpósłanje mejlkow bu znjemóžnjene',
'ipblocklist-empty'               => 'Liścina blokowanjow je prózdna.',
'ipblocklist-no-results'          => 'Požadana IP-adresa/požadane wužiwarske mjeno njeje zablokowane.',
'blocklink'                       => 'zablokować',
'unblocklink'                     => 'blokowanje zběhnyć',
'contribslink'                    => 'přinoški',
'autoblocker'                     => 'Awtomatiske blokowanje, dokelž twoja IP-adresa bu njedawno wot wužiwarja „[[User:$1|$1]]” wužita. Přičina, podata za blokowanje $1, je: "$2"',
'blocklogpage'                    => 'Protokol zablokowanjow',
'blocklogentry'                   => 'je wužiwarja [[$1]] zablokował z časom spadnjenja $2 $3',
'blocklogtext'                    => 'To je protokol blokowanja a wotblokowanja wužiwarjow. Awtomatisce blokowane IP-adresy so njenalistuja. Hlej [[Special:IPBlockList|lisćinu zablokowanych IP-adresow]] za lisćinu tuchwilnych wuhnaćow a zablokowanjow.',
'unblocklogentry'                 => 'zablokowanje wužiwarja $1 bu zběhnjene',
'block-log-flags-anononly'        => 'jenož anonymnych',
'block-log-flags-nocreate'        => 'wutworjenje wužiwarskich kontow znjemóžnjene',
'block-log-flags-noautoblock'     => 'awtomatiske zablokowanje znjemóžnjene',
'block-log-flags-noemail'         => 'wotpósłanje mejlkow bu znjemóžnjene',
'block-log-flags-angry-autoblock' => 'polěpšene awtomatiske blokowanje zmóžnjene',
'range_block_disabled'            => 'Kmanosć administratorow, cyłe wobłuki IP-adresow blokować, je znjemóžnjena.',
'ipb_expiry_invalid'              => 'Čas spadnjenja je njepłaćiwy.',
'ipb_expiry_temp'                 => 'Blokowanja schowanych wužiwarskich mjenow maja permanentne być.',
'ipb_already_blocked'             => 'Wužiwar „$1” je hižo zablokowany.',
'ipb_cant_unblock'                => 'Zmylk: Njemóžno ID zablokowanja $1 namakać. Zablokowanje je so najskerje mjeztym zběhnyło.',
'ipb_blocked_as_range'            => 'Zmylk: IP $1 njeje direktnje zablokowana a njeda so wublokować. Blokuje so wšak jako dźěl wobwoda $2, kotryž da so wublokować.',
'ip_range_invalid'                => 'Njepłaciwy wobłuk IP-adresow.',
'blockme'                         => 'Blokować',
'proxyblocker'                    => 'Awtomatiske blokowanje wotewrjenych proksy-serwerow',
'proxyblocker-disabled'           => 'Tuta funkcija je deaktiwizowana.',
'proxyblockreason'                => 'Twoja IP-adresa bu zablokowana, dokelž je wotewrjeny proksy. Prošu skontaktuj swojeho prowidera abo syćoweho administratora a informuj jeho wo tutym chutnym wěstotnym problemje.',
'proxyblocksuccess'               => 'Dokónčene.',
'sorbs'                           => 'SORBS DNSbl',
'sorbsreason'                     => 'Twoja IP-adresa je jako wotewrjeny proksy na DNSBL {{GRAMMAR:genitiw|{{SITENAME}}}} zapisana.',
'sorbs_create_account_reason'     => 'Twoja IP-adresa je jako wotewrjeny proksy na DNSBL {{GRAMMAR:genitiw|{{SITENAME}}}} zapisana. Njemóžeš konto wutworić.',

# Developer tools
'lockdb'              => 'Datowu banku zamknyć',
'unlockdb'            => 'Datowu banku wotamknyć',
'lockdbtext'          => 'Zamknjenje datoweje banki znjemóžni wšěm wužiwarjam strony wobdźěłać, jich nastajenja změnić, jich wobkedźbowanki wobdźěłać a wšě druhe dźěła činić, kotrež sej změny w datowej bance žadaja. Prošu wobkruć, zo chceš datowu banku woprawdźe zamknyć a zo chceš ju zaso wotamknyć, hdyž wothladowanje je sčinjene.',
'unlockdbtext'        => 'Wotamknjenje datoweje banki zaso wšěm wužiwarjam zmóžni strony wobdźěłać, jich nastajenja změnić, jich wobkedźbowanki wobdźěłać a wšě druhe dźěła činić, kotrež sej změny w datowej bance žadaja. Prošu wobkruć, zo chceš datowu banku woprawdźe wotamknyć.',
'lockconfirm'         => 'Haj, chcu datowu banku woprawdźe zamknyć.',
'unlockconfirm'       => 'Haj, chcu datowu banku woprawdźe wotamknyć.',
'lockbtn'             => 'Datowu banku zamknyć',
'unlockbtn'           => 'Datowu banku wotamknyć',
'locknoconfirm'       => 'Njejsy kontrolowy kašćik nakřižował.',
'lockdbsuccesssub'    => 'Datowa banka bu wuspěšnje zamknjena.',
'unlockdbsuccesssub'  => 'Datowa banka bu wuspěšnje wotamknjena.',
'lockdbsuccesstext'   => 'Datowa banka bu zamknjena.
<br />Njezabudź [[Special:UnlockDB|wotzamknyć]], po tym zo wothladowanje je sčinjene.',
'unlockdbsuccesstext' => 'Datowa banka bu wotamknjena.',
'lockfilenotwritable' => 'Do dataje zamknjenja datoweje banki njeda so zapisować. Za zamknjenje abo wotamknjenje datoweje banki dyrbi webowy serwer pisanske prawo měć.',
'databasenotlocked'   => 'Datajowa banka zamknjena njeje.',

# Move page
'move-page'               => '$1 přesunyć',
'move-page-legend'        => 'Stronu přesunyć',
'movepagetext'            => 'Wužiwanje formulara deleka budźe stronu přemjenować, suwajo jeje cyłe stawizny pod nowe mjeno. Stary titl budźe daleposrědkowanje na nowy titl. Wotkazy na stary titl so njezměnja. Pruwuj za dwójnymi abo skóncowanymi daleposrědkowanjemi. Dyrbiš zaručić, zo wotkazy na stronu pokazuja, na kotruž dyrbja dowjesć.

Wobkedźbuj, zo strona so <b>nje</b> přesunje, jeli strona z nowym titlom hizo eksistuje, chibazo wona je prózdna abo dalesposrědkowanje a nima zašłe stawizny. To woznamjenja, zo móžeš stronu tam wróćo přemjenować, hdźež bu runje přemjenowana, jeli zmylk činiš a njemóžeš wobstejacu stronu přepisować.

<b>KEDŹBU!</b> Móže to drastiska a njewočakowana změna za woblubowanu stronu być; prošu budź sej wěsty, zo sćěwki rozumiš, prjedy hač pokročuješ.',
'movepagetalktext'        => 'Přisłušna diskusijna strona přesunje so awtomatisce hromadźe z njej, <b>chibazo:</b>
*Njeprózdna diskusijna strona pod nowym mjenom hižo eksistuje abo
*wotstronješ hóčku z kašćika deleka.

W tutych padach dyrbiš stronu manuelnje přesunyć abo zaměšeć, jeli sej to přeješ.',
'movearticle'             => 'Stronu přesunyć',
'movenotallowed'          => 'Nimaš w tutym wikiju prawo, zo by strony přesunył.',
'newtitle'                => 'pod nowe hesło',
'move-watch'              => 'Stronu wobkedźbować',
'movepagebtn'             => 'Stronu přesunyć',
'pagemovedsub'            => 'Přesunjenje wuspěšne',
'movepage-moved'          => '<big>\'\'\'Strona "$1" bu do "$2" přesunjena.\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Strona z tutym mjenom hižo eksistuje abo mjeno, kotrež sy wuzwolił, płaćiwe njeje. Prošu wuzwol druhe mjeno.',
'cantmove-titleprotected' => 'Njemóžeš stronu do tutoho městna přesunyć, dokelž nowy titul bu přećiwo wutworjenju škitany',
'talkexists'              => 'Strona sama bu z wuspěchom přesunjena, diskusijna strona pak njeda so přesunyć, dokelž pod nowym titulom hižo eksistuje. Prošu změš jeju manuelnje.',
'movedto'                 => 'přesunjena do hesła',
'movetalk'                => 'Přisłušnu diskusijnu stronu tohorunja přesunyć',
'move-subpages'           => 'Wšě podstrony přesunyć, jeli eksistuja',
'move-talk-subpages'      => 'Wšě podstrony diskusijnych stronow přesunyć, jeli eksistuja',
'movepage-page-exists'    => 'Strona $1 hižo eksistuje a njeda so awtomatisce přepisać.',
'movepage-page-moved'     => 'Strona $1 bu do $2 přesunjena.',
'movepage-page-unmoved'   => 'Strona $1 njeda so do $2 přesunyć.',
'movepage-max-pages'      => 'Maksimalna ličba $1 {{PLURAL:$1|strony|stronow|stronow|stronow}} bu přesunjena, dalše strony so awtomatisce njepřesunu.',
'1movedto2'               => 'je [[$1]] pod hesło [[$2]] přesunył',
'1movedto2_redir'         => 'je [[$1]] pod hesło [[$2]] přesunył a při tym daleposrědkowanje přepisał.',
'movelogpage'             => 'Protokol přesunjenjow',
'movelogpagetext'         => 'Deleka je lisćina wšěch přesunjenych stronow.',
'movereason'              => 'Přičina',
'revertmove'              => 'wróćo přesunyć',
'delete_and_move'         => 'wušmórnyć a přesunyć',
'delete_and_move_text'    => '== Wušmórnjenje trěbne ==

Cilowa strona „[[:$1]]” hižo eksistuje. Chceš ju wušmórnyć, zo by so přesunjenje zmóžniło?',
'delete_and_move_confirm' => 'Haj, stronu wušmórnyć.',
'delete_and_move_reason'  => 'Strona bu wušmórnjena, zo by so přesunjenje zmóžniło.',
'selfmove'                => 'Žórłowy a cilowy titl stej samsnej; strona njehodźi so na sebje samu přesunyć.',
'immobile_namespace'      => 'Cilowy titl je wosebity typ; strony njehodźa so do tutoho mjenoweho ruma abo z njeho přesunyć.',
'imagenocrossnamespace'   => 'Wobraz njeda so do druheho mjenoweho ruma hač wobraz přesunyć',
'imagetypemismatch'       => 'Nowa dataja swojemu typej njewotpowěduje',
'imageinvalidfilename'    => 'Mjeno ciloweje dataje je njepłaćiwe',
'fix-double-redirects'    => 'Daleposrědkowanja aktualizować, kotrež na prěnjotny titul pokazuja',

# Export
'export'            => 'Strony eksportować',
'exporttext'        => 'Móžeš tekst a stawizny wěsteje strony abo skupiny stronow, kotrež su w XML zawite, eksportować. To da so potom do druheho wikija, kotryž ze software MediaWiki dźěła, přez [[Special:Import|importowansku stronu]] importować.

Zo by strony eksportował, zapodaj title deleka do tekstoweho pola, jedyn titul na linku, a wubjer, hač chceš aktualnu wersiju kaž tež stare wersije z linkami stawiznow strony abo jenož aktualnu wersiju z informacijemi wo poslednjej změnje eksportować.

W poslednim padźe móžeš tež wotkaz wužiwać, na př. „[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]]” za stronu „[[{{MediaWiki:Mainpage}}]]”.',
'exportcuronly'     => 'Jenož aktualnu wersiju zapřijeć, nic dospołne stawizny',
'exportnohistory'   => '----
<b>Kedźbu:</b> Eksport cyłych stawiznow přez tutón formular bu z přičin wukonitosće serwera znjemóžnjeny.',
'export-submit'     => 'Eksportować',
'export-addcattext' => 'Strony z kategorije dodawać:',
'export-addcat'     => 'Dodawać',
'export-download'   => 'Jako XML-dataju składować',
'export-templates'  => 'Předłohi zapřijeć',

# Namespace 8 related
'allmessages'               => 'Systemowe zdźělenki',
'allmessagesname'           => 'Mjeno',
'allmessagesdefault'        => 'Standardny tekst',
'allmessagescurrent'        => 'Aktualny tekst',
'allmessagestext'           => 'To je lisćina wšěch systemowych zdźělenkow, kotrež w mjenowym rumje MediaWiki k dispoziciji steja.',
'allmessagesnotsupportedDB' => "Tuta strona njeda so wužiwać, dokelž '''\$wgUseDatabaseMessages''' bu znjemóžnjeny.",
'allmessagesfilter'         => 'Filter za jednotliwe zdźělenki:',
'allmessagesmodified'       => 'Jenož změnjene pokazać',

# Thumbnails
'thumbnail-more'           => 'powjetšić',
'filemissing'              => 'Dataja pobrachuje',
'thumbnail_error'          => 'Zmylk při wutworjenju miniaturki: $1',
'djvu_page_error'          => 'Strona DjVU zwonka wobłuka strony',
'djvu_no_xml'              => 'Daty XML njemóža so za dataju DjVU wotwołać',
'thumbnail_invalid_params' => 'Njepłaćiwe parametry miniaturki',
'thumbnail_dest_directory' => 'Njemóžno cilowy zapis wutworić.',

# Special:Import
'import'                     => 'Strony importować',
'importinterwiki'            => 'Import z druheho wikija',
'import-interwiki-text'      => 'Wuběr wiki a stronu za importowanje. Daty wersijow a mjena awtorow so zachowaja. Wšě akcije za transwiki-importy so w [[Special:Log/import|protokolu importow]] protokoluja.',
'import-interwiki-history'   => 'Wšě wersije ze stawiznow tuteje strony kopěrować',
'import-interwiki-submit'    => 'Importować',
'import-interwiki-namespace' => 'Strony importować do mjenoweho ruma:',
'importtext'                 => 'Prošu eksportuj dataju ze žórłoweho wikija z pomocu [[Special:Export|Strony eksportować]]. Składuj ju na swojim ličaku a nahraj ju sem.',
'importstart'                => 'Importuju…',
'import-revision-count'      => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}}',
'importnopages'              => 'Žane strony za importowanje.',
'importfailed'               => 'Import zwrěšćił: $1',
'importunknownsource'        => 'Njeznate importowe žórło',
'importcantopen'             => 'Importowa dataja njeda so wočinjeć.',
'importbadinterwiki'         => 'Wopačny interwiki-wotkaz',
'importnotext'               => 'Prózdny abo žadyn tekst',
'importsuccess'              => 'Import wuspěšny!',
'importhistoryconflict'      => 'Je konflikt ze stawiznami strony wustupił. Snano bu strona hižo prjedy importowana.',
'importnosources'            => 'Žane importowanske žórła za transwiki wubrane. Direktne nahraće stawiznow je znjemóžnjene.',
'importnofile'               => 'Žana importowanska dataja wubrana.',
'importuploaderrorsize'      => 'Nahraće importoweje dataje je so njeporadźiło. Dataja je wjetša hač dowolena datajowa wulkosć.',
'importuploaderrorpartial'   => 'Nahraće importoweje dataje je so njeporadźiło. Dataja je so jenož zdźěla nahrała.',
'importuploaderrortemp'      => 'Nahraće importoweje dataje je so njeporadźiło. Temporarny zapis faluje.',
'import-parse-failure'       => 'Zmylk za XML-import:',
'import-noarticle'           => 'Žadyn nastawk za import!',
'import-nonewrevisions'      => 'Wšě wersije buchu hižo prjedy importowane.',
'xml-error-string'           => '$1 linka $2, špalta $3, (bajt $4): $5',
'import-upload'              => 'XML-daty nahrać',

# Import log
'importlogpage'                    => 'Protokol importow',
'importlogpagetext'                => 'To je lisćina importowanych stronow ze stawiznami z druhich wikijow.',
'import-logentry-upload'           => 'strona [[$1]] bu přez nahraće importowana',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}}',
'import-logentry-interwiki'        => 'je stronu [[$1]] z druheho wikija přenjesł',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|wersija|wersiji|wersije|wersijow}} z $2 {{PLURAL:$1|importowana|importowanej|importowane|importowane}}',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'moja wužiwarska strona',
'tooltip-pt-anonuserpage'         => 'Wužiwarska strona IP-adresy, z kotrejž tuchwilu dźěłaš',
'tooltip-pt-mytalk'               => 'moja diskusijna strona',
'tooltip-pt-anontalk'             => 'Diskusija wo změnach z tuteje IP-adresy',
'tooltip-pt-preferences'          => 'moje nastajenja',
'tooltip-pt-watchlist'            => 'lisćina stronow, kotrež wobkedźbuješ',
'tooltip-pt-mycontris'            => 'lisćina mojich přinoškow',
'tooltip-pt-login'                => 'Móžeš so woměrje přizjewić, to pak zawjazowace njeje.',
'tooltip-pt-anonlogin'            => 'Móžeš so woměrje přizjewić, to pak zawjazowace njeje.',
'tooltip-pt-logout'               => 'so wotzjewić',
'tooltip-ca-talk'                 => 'diskusija wo stronje',
'tooltip-ca-edit'                 => 'Móžeš stronu wobdźěłać. Prošu wužij tłóčku „Přehlad” do składowanja.',
'tooltip-ca-addsection'           => 'nowy wotrězk k diskusiji dodać',
'tooltip-ca-viewsource'           => 'Strona je škitana. Móžeš pak jeje žórło wobhladać.',
'tooltip-ca-history'              => 'stawizny tuteje strony',
'tooltip-ca-protect'              => 'stronu škitać',
'tooltip-ca-delete'               => 'stronu wušmórnyć',
'tooltip-ca-undelete'             => 'změny wobnowić, kotrež buchu do wušmórnjenja sčinjene',
'tooltip-ca-move'                 => 'stronu přesunyć',
'tooltip-ca-watch'                => 'stronu  wobkedźbowankam přidać',
'tooltip-ca-unwatch'              => 'stronu z wobkedźbowankow wotstronić',
'tooltip-search'                  => '{{GRAMMAR:akuzatiw|{{SITENAME}}}} přepytać',
'tooltip-search-go'               => 'Dźi k stronje z runje tutym mjenom, jeli eksistuje',
'tooltip-search-fulltext'         => 'Strony za tutym tekstom přepytać',
'tooltip-p-logo'                  => 'hłowna strona',
'tooltip-n-mainpage'              => 'hłownu stronu pokazać',
'tooltip-n-portal'                => 'wo projekće, što móžeš činić, hdźe móžeš informacije namakać',
'tooltip-n-currentevents'         => 'pozadkowe informacije wo aktualnych podawkach pytać',
'tooltip-n-recentchanges'         => 'lisćina aktualnych změnow w tutym wikiju',
'tooltip-n-randompage'            => 'připadny nastawk wopytać',
'tooltip-n-help'                  => 'pomocna strona',
'tooltip-t-whatlinkshere'         => 'lisćina wšěch stronow, kotrež sem wotkazuja',
'tooltip-t-recentchangeslinked'   => 'aktualne změny w stronach, na kotrež tuta strona wotkazuje',
'tooltip-feed-rss'                => 'RSS-feed za tutu stronu',
'tooltip-feed-atom'               => 'Atom-feed za tutu stronu',
'tooltip-t-contributions'         => 'přinoški tutoho wužiwarja wobhladać',
'tooltip-t-emailuser'             => 'wužiwarjej mejlku pósłać',
'tooltip-t-upload'                => 'Dataje nahrać',
'tooltip-t-specialpages'          => 'lisćina wšěch specialnych stronow',
'tooltip-t-print'                 => 'ćišćowy napohlad tuteje strony',
'tooltip-t-permalink'             => 'trajny wotkaz k tutej wersiji strony',
'tooltip-ca-nstab-main'           => 'stronu wobhladać',
'tooltip-ca-nstab-user'           => 'wužiwarsku stronu wobhladać',
'tooltip-ca-nstab-media'          => 'datajowu stronu wobhladać',
'tooltip-ca-nstab-special'        => 'To je specialna strona. Njemóžeš ju wobdźěłać.',
'tooltip-ca-nstab-project'        => 'projektowu stronu wobhladać',
'tooltip-ca-nstab-image'          => 'Datajowu stronu pokazać',
'tooltip-ca-nstab-mediawiki'      => 'systemowu zdźělenku wobhladać',
'tooltip-ca-nstab-template'       => 'předłohu wobhladać',
'tooltip-ca-nstab-help'           => 'pomocnu stronu wobhladać',
'tooltip-ca-nstab-category'       => 'kategorijnu stronu wobhladać',
'tooltip-minoredit'               => 'jako snadnu změnu woznamjenić',
'tooltip-save'                    => 'změny składować',
'tooltip-preview'                 => 'twoje změny přehladnyć, prošu čiń to do składowanja!',
'tooltip-diff'                    => 'změny pokazać, kotrež sy w teksće činił',
'tooltip-compareselectedversions' => 'rozdźěle mjez wubranymaj wersijomaj tuteje strony pokazać',
'tooltip-watch'                   => 'tutu stronu wobkedźbowankam přidać',
'tooltip-recreate'                => 'stronu znowa wutworić, hačrunjež bu wumšmórnjena',
'tooltip-upload'                  => 'nahraće startować',

# Stylesheets
'common.css'   => '/* CSS w tutej dataji budźe so na wšěch stronow wuskutkować. */',
'monobook.css' => '/* CSS wobdźěłać, zo by so skin „monobook” za wšěčh wužiwarjow tutoho skina priměrił */',

# Scripts
'common.js'   => '/* Kóždy JavaScript tu so za wšěch wužiwarjow při kóždym zwobraznjenju někajkeje strony začita. */',
'monobook.js' => '/* Slědowacy JavaScript začita so za wužiwarjow, kotřiž šat MonoBook wužiwaja */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadaty su za tutón serwer znjemóžnjene.',
'nocreativecommons' => 'Creative Commons RDF metadaty su za tutón serwer znjemóžnjene.',
'notacceptable'     => 'Serwer wikija njemóže daty we formaće poskićić, kotryž twój wudawanski nastroj móže čitać.',

# Attribution
'anonymous'        => 'Anonymny wužiwar/anonymni wužiwarjo {{GRAMMAR:genitiw|{{SITENAME}}}}',
'siteuser'         => 'wužiwar {{GRAMMAR:genitiw|{{SITENAME}}}} $1',
'lastmodifiedatby' => 'Strona bu dnja $1 w $2 hodź. wot wužiwarja $3 změnjena.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Na zakładźe dźěła wužiwarja $1.',
'others'           => 'druhich',
'siteusers'        => 'wužiwarjow {{GRAMMAR:genitiw|{{SITENAME}}}} $1',
'creditspage'      => 'Dźak awtoram',
'nocredits'        => 'Za tutu stronu žane informacije wo zasłužbach njejsu.',

# Spam protection
'spamprotectiontitle' => 'Spamowy filter',
'spamprotectiontext'  => 'Strona, kotruž sy spytał składować, bu přez spamowy filter zablokowana. To so najskerje přez wotkaz na  eksterne sydło w čornej lisćinje zawinuje.',
'spamprotectionmatch' => 'Sćěhowacy tekst je naš spamowy filter wotpokazał: $1',
'spambot_username'    => 'MediaWiki čisćenje wot spama',
'spam_reverting'      => 'wróćo na poslednju wersiju, kotraž wotkazy na $1 njewobsahuje',
'spam_blanking'       => 'Wšě wersije wobsahowachu wotkazy na $1, wučisćene.',

# Info page
'infosubtitle'   => 'Informacije za stronu',
'numedits'       => 'Ličba změnow (nastawk): $1',
'numtalkedits'   => 'Ličba změnow (diskusijna strona): $1',
'numwatchers'    => 'Ličba wobkedźbowarjow: $1',
'numauthors'     => 'Ličba rozdźělnych awtorow (nastawk): $1',
'numtalkauthors' => 'Ličba rozdźělnych awtorow (diskusijna strona): $1',

# Math options
'mw_math_png'    => 'Přeco jako PNG zwobraznić',
'mw_math_simple' => 'HTML jeli jara jednory, hewak PNG',
'mw_math_html'   => 'HTML jeli móžno, hewak PNG',
'mw_math_source' => 'Jako TeX wostajić (za tekstowe wobhladowaki)',
'mw_math_modern' => 'Za moderne wobhladowaki doporučene',
'mw_math_mathml' => 'MathML jeli móžno (eksperimentalnje)',

# Patrolling
'markaspatrolleddiff'                 => 'Změnu jako přepruwowanu woznamjenić',
'markaspatrolledtext'                 => 'Tutu změnu nastawka jako přepruwowanu woznamjenić',
'markedaspatrolled'                   => 'Změna bu jako přepruwowana woznamjenjena.',
'markedaspatrolledtext'               => 'Wubrana wersija bu jako přepruwowana woznamjenjena.',
'rcpatroldisabled'                    => 'Přepruwowanje aktualnych změnow je znjemóžnjene.',
'rcpatroldisabledtext'                => 'Funkcija přepruwowanja aktualnych změnow je tuchwilu znjemóžnjena.',
'markedaspatrollederror'              => 'Njemóžno jako přepruwowanu woznamjenić.',
'markedaspatrollederrortext'          => 'Dyrbiš wersiju podać, kotraž so ma jako přepruwowana woznamjenić.',
'markedaspatrollederror-noautopatrol' => 'Njesměš swoje změny jako přepruwowane woznamjenjeć.',

# Patrol log
'patrol-log-page'   => 'Protokol přepruwowanjow',
'patrol-log-header' => 'To je protokol dohladowanych wersijow.',
'patrol-log-line'   => 'je $1 strony $2 jako přepruwowanu markěrował $3.',
'patrol-log-auto'   => '(awtomatisce)',
'patrol-log-diff'   => 'wersiju $1',

# Image deletion
'deletedrevision'                 => 'Stara wersija $1 wušmórnjena',
'filedeleteerror-short'           => 'Zmylk při zničenju dataje: $1',
'filedeleteerror-long'            => 'Buchu zmylki při zničenju dataje zwěsćene:

$1',
'filedelete-missing'              => 'Dataja "$1" njeda so zničić, dokelž njeeksistuje.',
'filedelete-old-unregistered'     => 'Podata datajowa wersija "$1" w datowej bance njeje.',
'filedelete-current-unregistered' => 'Podata dataja "$1" w datowej bance njeje.',
'filedelete-archive-read-only'    => 'Do archiwoweho zapisa "$1" njeda so z webowym serwerom pisać.',

# Browsing diffs
'previousdiff' => '← Předchadna změna',
'nextdiff'     => 'Přichodna změna →',

# Media information
'mediawarning'         => '<b>KEDŹBU:</b> Dataja móhła złowólny kod wobsahować, kotrehož wuwjedźenje móhło twój system wobškodźić.<hr />',
'imagemaxsize'         => 'Wobrazy na stronach wobrazoweho wopisanja wobmjezować na:',
'thumbsize'            => 'Wulkosć miniaturkow (thumbnails):',
'widthheight'          => '$1x$2',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|strona|stronje|strony|stronow}}',
'file-info'            => 'Wulkosć dataje: $1, družina MIME: $2',
'file-info-size'       => '($1 × $2 pikselow, wulkosć dataje: $3, družina MIME: $4)',
'file-nohires'         => '<small>Za tutu dataju žane wyše rozeznaće njeje.</small>',
'svg-long-desc'        => '(SVG-dataja, zakładna wulkosć: $1 × $2 pikselow, datajowa wulkosć: $3)',
'show-big-image'       => 'Wersija z wyšim rozeznaćom',
'show-big-image-thumb' => '<small>Wulkosć miniaturki: $1 × $2 pikselow</small>',

# Special:NewImages
'newimages'             => 'Nowe dataje',
'imagelisttext'         => "Deleka je lisćina '''$1''' {{PLURAL:$1|dataje|datajow|datajow|datajow}}, kotraž je po $2 sortěrowana.",
'newimages-summary'     => 'Tuta specialna strona naliči aktualnje nahrate wobrazy a druhe dataje.',
'showhidebots'          => '(bots $1)',
'noimages'              => 'Žane dataje.',
'ilsubmit'              => 'Pytać',
'bydate'                => 'datumje',
'sp-newimages-showfrom' => 'Nowe dataje pokazać, započinajo wot $1, $2',

# Bad image list
'bad_image_list' => 'Format:

Jenož zapiski lisćiny (linki, kotrež so z * započinaja), so wobkedźbuja. Prěni wotkaz na lince dyrbi wotkaz k njewitanemu wobrazej być.
Nasledne wotkazy na samsnej lince definuja wuwzaća, hdźež so wobraz smě najebać toho jewić.',

# Metadata
'metadata'          => 'Metadaty',
'metadata-help'     => 'Dataja wobsahuje přidatne informacije, kotrež pochadźa z digitalneje kamery abo skenera. Jeli dataja bu wot toho změnjena je móžno, zo někotre podrobnosće z nětčišeho stawa wotchila.',
'metadata-expand'   => 'Podrobnosće pokazać',
'metadata-collapse' => 'Podrobnosće schować',
'metadata-fields'   => 'Sćěhowace EXIF-metadaty so standardnje pokazuja. Druhe so po standardźe schowaja a móža so z tabele rozfałdować.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Šěrokosć',
'exif-imagelength'                 => 'Wysokosć',
'exif-bitspersample'               => 'Bitow na barbowu komponentu',
'exif-compression'                 => 'Metoda kompresije',
'exif-photometricinterpretation'   => 'Zestajenje pikselow',
'exif-orientation'                 => 'Wusměrjenje kamery',
'exif-samplesperpixel'             => 'Ličba komponentow',
'exif-planarconfiguration'         => 'Porjad datow',
'exif-ycbcrsubsampling'            => 'Poměr podwotmasanja (Subsampling) wot Y do C',
'exif-ycbcrpositioning'            => 'Zaměstnjenje Y a C',
'exif-xresolution'                 => 'Wodorune rozeznaće',
'exif-yresolution'                 => 'Padorune rozeznaće',
'exif-resolutionunit'              => 'Jednotka rozeznaća X a Y',
'exif-stripoffsets'                => 'Městno wobrazowych datow',
'exif-rowsperstrip'                => 'Ličba rjadkow na pas',
'exif-stripbytecounts'             => 'Bajty na komprimowany pas',
'exif-jpeginterchangeformat'       => 'Offset k JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Bajty JPEG datow',
'exif-transferfunction'            => 'Přenošowanska funkcija',
'exif-whitepoint'                  => 'Barbowa kwalita běłeho dypka',
'exif-primarychromaticities'       => 'Barbowa kwalita primarnych barbow',
'exif-ycbcrcoefficients'           => 'Koeficienty matriksy za transformaciju barbneho ruma',
'exif-referenceblackwhite'         => 'Por čorneje a běłeje referencneje hódnoty',
'exif-datetime'                    => 'Datum a čas datajoweje změny',
'exif-imagedescription'            => 'Titl wobraza',
'exif-make'                        => 'Zhotowjer kamery',
'exif-model'                       => 'Model kamery',
'exif-software'                    => 'Wužiwana softwara',
'exif-artist'                      => 'Awtor',
'exif-copyright'                   => 'Mějićel awtorskich prawow',
'exif-exifversion'                 => 'Wersija EXIF',
'exif-flashpixversion'             => 'Podpěrowana wersija Flashpix',
'exif-colorspace'                  => 'Barbny rum',
'exif-componentsconfiguration'     => 'Woznam kóždeje komponenty',
'exif-compressedbitsperpixel'      => 'Modus wobrazoweje kompresije',
'exif-pixelydimension'             => 'Płaćiwa šěrokosć wobraza',
'exif-pixelxdimension'             => 'Płaćiwa wysokosć wobraza',
'exif-makernote'                   => 'Přispomnjenki zhotowjerja',
'exif-usercomment'                 => 'Přispomjenja wužiwarja',
'exif-relatedsoundfile'            => 'Zwjazana zynkowa dataja',
'exif-datetimeoriginal'            => 'Datum a čas wutworjenja datow',
'exif-datetimedigitized'           => 'Datum a čas digitalizowanja',
'exif-subsectime'                  => 'Dźěle sekundy za DateTime',
'exif-subsectimeoriginal'          => 'Dźěle sekundy za DateTimeOriginal',
'exif-subsectimedigitized'         => 'Dźěle sekundy za DateTimeDigitized',
'exif-exposuretime'                => 'Naswětlenski čas',
'exif-exposuretime-format'         => '$1 sek. ($2)',
'exif-fnumber'                     => 'Zasłona',
'exif-exposureprogram'             => 'Naswětlenski program',
'exif-spectralsensitivity'         => 'Spektralna cutliwosć',
'exif-isospeedratings'             => 'Cutliwosć filma abo sensora (ISO)',
'exif-oecf'                        => 'Optoelektroniski přeličenski faktor (OECF)',
'exif-shutterspeedvalue'           => 'Naswětlenski čas',
'exif-aperturevalue'               => 'Zasłona',
'exif-brightnessvalue'             => 'Swětłosć',
'exif-exposurebiasvalue'           => 'Naswětlenska korektura',
'exif-maxaperturevalue'            => 'Najwjetša zasłona',
'exif-subjectdistance'             => 'Zdalenje k předmjetej',
'exif-meteringmode'                => 'Měrjenska metoda',
'exif-lightsource'                 => 'Žórło swěcy',
'exif-flash'                       => 'Błysk',
'exif-focallength'                 => 'Palnišćowa zdalenosć',
'exif-subjectarea'                 => 'Wobwod předmjeta',
'exif-flashenergy'                 => 'Sylnosć błyska',
'exif-spatialfrequencyresponse'    => 'Cutliwosć rumoweje frekwency',
'exif-focalplanexresolution'       => 'Wodorune rozeznaće sensora',
'exif-focalplaneyresolution'       => 'Padorune rozeznaće sensora',
'exif-focalplaneresolutionunit'    => 'Jednotka rozeznaća sensora',
'exif-subjectlocation'             => 'Městno předmjeta',
'exif-exposureindex'               => 'Naswětlenski indeks',
'exif-sensingmethod'               => 'Měrjenska metoda',
'exif-filesource'                  => 'Žórło dataje',
'exif-scenetype'                   => 'Typ sceny',
'exif-cfapattern'                  => 'Muster CFA',
'exif-customrendered'              => 'Wot wužiwarja definowane předźěłanje wobrazow',
'exif-exposuremode'                => 'Naswětlenski modus',
'exif-whitebalance'                => 'Balansa běłeho dypka',
'exif-digitalzoomratio'            => 'Digitalny zoom',
'exif-focallengthin35mmfilm'       => 'Palnišćowa zdalenosć za film 35 mm přeličena',
'exif-scenecapturetype'            => 'Družina sceny',
'exif-gaincontrol'                 => 'Regulowanje sceny',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Nasyćenosć',
'exif-sharpness'                   => 'Wótrosć',
'exif-devicesettingdescription'    => 'Nastajenja nastroja',
'exif-subjectdistancerange'        => 'Zdalenosć k motiwej',
'exif-imageuniqueid'               => 'ID wobraza',
'exif-gpsversionid'                => 'Wersija ID GPS',
'exif-gpslatituderef'              => 'Sewjerna abo južna šěrina',
'exif-gpslatitude'                 => 'Geografiska šěrina',
'exif-gpslongituderef'             => 'Wuchodna abo zapadna dołhosć',
'exif-gpslongitude'                => 'Geografiska dołhosć',
'exif-gpsaltituderef'              => 'Referencna wyšina',
'exif-gpsaltitude'                 => 'Wyšina',
'exif-gpstimestamp'                => 'Čas GPS (atomowy časnik)',
'exif-gpssatellites'               => 'Satelity wužiwane za měrjenje',
'exif-gpsstatus'                   => 'Status přijimaka',
'exif-gpsmeasuremode'              => 'Měrjenska metoda',
'exif-gpsdop'                      => 'Měrjenska dokładnosć',
'exif-gpsspeedref'                 => 'Jednotka spěšnosće',
'exif-gpsspeed'                    => 'Spěšnosć přijimaka GPS',
'exif-gpstrackref'                 => 'Referenca za směr pohiba',
'exif-gpstrack'                    => 'Směr pohiba',
'exif-gpsimgdirectionref'          => 'Referenca za wusměrjenje wobraza',
'exif-gpsimgdirection'             => 'Wobrazowy směr',
'exif-gpsmapdatum'                 => 'Wužiwane geodetiske daty',
'exif-gpsdestlatituderef'          => 'Referenca za šěrinu',
'exif-gpsdestlatitude'             => 'Šěrina',
'exif-gpsdestlongituderef'         => 'Referenca dołhosće',
'exif-gpsdestlongitude'            => 'Dołhosć',
'exif-gpsdestbearingref'           => 'Referenca za wusměrjenje',
'exif-gpsdestbearing'              => 'Wusměrjenje',
'exif-gpsdestdistanceref'          => 'Referenca za zdalenosć k cilej',
'exif-gpsdestdistance'             => 'Zdalenosć k cilej',
'exif-gpsprocessingmethod'         => 'Metoda předźěłanja GPS',
'exif-gpsareainformation'          => 'Mjeno wobwoda GPS',
'exif-gpsdatestamp'                => 'Datum GPS',
'exif-gpsdifferential'             => 'Diferencialna korektura GPS',

# EXIF attributes
'exif-compression-1' => 'Njekomprimowany',

'exif-unknowndate' => 'Njeznaty datum',

'exif-orientation-1' => 'Normalnje', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Wodorunje wobroćeny', # 0th row: top; 0th column: right
'exif-orientation-3' => '180° zwjertnjeny', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Padorunje wobroćeny', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '90° přećiwo směrej časnika zwjertneny a padorunje wobroćeny', # 0th row: left; 0th column: top
'exif-orientation-6' => '90° w směrje časnika zwjertnjeny', # 0th row: right; 0th column: top
'exif-orientation-7' => '90° w směrje časnika zwjertnjeny a padorunje wobroćeny', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '90° přećiwo směrej časnika zwjertnjeny', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Škropawy format',
'exif-planarconfiguration-2' => 'Płony format',

'exif-componentsconfiguration-0' => 'Njeeksistuje',

'exif-exposureprogram-0' => 'Njeznaty',
'exif-exposureprogram-1' => 'Manuelny',
'exif-exposureprogram-2' => 'Normalny program',
'exif-exposureprogram-3' => 'Priorita zasłony',
'exif-exposureprogram-4' => 'Priorita zawěrki',
'exif-exposureprogram-5' => 'Kreatiwny program (za hłubokosć wótrosće)',
'exif-exposureprogram-6' => 'Akciski program (za wyšu spěšnosć zawěrki)',
'exif-exposureprogram-7' => 'Portretowy modus (za fota z blikosće z pozadkom zwonka fokusa)',
'exif-exposureprogram-8' => 'Krajinowy modus (za fota krajinow z pozadkom we fokusu)',

'exif-subjectdistance-value' => '$1 m',

'exif-meteringmode-0'   => 'Njeznata',
'exif-meteringmode-1'   => 'Přerězk',
'exif-meteringmode-2'   => 'Srjedźa wusměrjeny',
'exif-meteringmode-3'   => 'Spotowe měrjenje',
'exif-meteringmode-4'   => 'Multispot',
'exif-meteringmode-6'   => 'Dźělna',
'exif-meteringmode-255' => 'Druha',

'exif-lightsource-0'   => 'Njeznata',
'exif-lightsource-1'   => 'Dnjowe swětło',
'exif-lightsource-2'   => 'Fluorescentne',
'exif-lightsource-3'   => 'Žehlawka',
'exif-lightsource-4'   => 'Błysk',
'exif-lightsource-9'   => 'Rjane wjedro',
'exif-lightsource-10'  => 'Pomróčene',
'exif-lightsource-11'  => 'Sćin',
'exif-lightsource-12'  => 'Dnjowe swětło fluoreskowace (D 5700 – 7100K)',
'exif-lightsource-13'  => 'Dnjowoběły fluoreskowacy (N 4600 – 5400K)',
'exif-lightsource-14'  => 'Zymnoběły fluoreskowacy (W 3900 – 4500K)',
'exif-lightsource-15'  => 'běły fluoroskowacy (WW 3200 – 3700K)',
'exif-lightsource-17'  => 'Standardne swětło A',
'exif-lightsource-18'  => 'Standardne swětło B',
'exif-lightsource-19'  => 'Standardne swětło C',
'exif-lightsource-24'  => 'ISO studijowa wolframowa žehlawka',
'exif-lightsource-255' => 'Druhe žórło swětła',

'exif-focalplaneresolutionunit-2' => 'cól',

'exif-sensingmethod-1' => 'Njedefinowany',
'exif-sensingmethod-2' => 'Jednočipowy barbowy přestrjenjowy sensor',
'exif-sensingmethod-3' => 'Dwučipowy barbowy přestrjenjowy sensor',
'exif-sensingmethod-4' => 'Třičipowy barbowy přestrjenjowy sensor',
'exif-sensingmethod-5' => 'Sekwencielny barbowy přestrjenjowy sensor',
'exif-sensingmethod-7' => 'Třilinearny sensor',
'exif-sensingmethod-8' => 'Barbowy sekwencielny linearny sensor',

'exif-scenetype-1' => 'Direktnje fotografowany wobraz',

'exif-customrendered-0' => 'Normalne wobdźěłanje',
'exif-customrendered-1' => 'Wužiwarske wobdźěłanje',

'exif-exposuremode-0' => 'Awtomatiske naswětlenje',
'exif-exposuremode-1' => 'Manuelne naswětlenje',
'exif-exposuremode-2' => 'Rjad naswětlenjow (Bracketing)',

'exif-whitebalance-0' => 'Automatiske wurunanje běłeho',
'exif-whitebalance-1' => 'Manuelne wurunanje běłeho',

'exif-scenecapturetype-1' => 'Krajina',
'exif-scenecapturetype-2' => 'Portret',
'exif-scenecapturetype-3' => 'Nócna scena',

'exif-gaincontrol-0' => 'Žane',
'exif-gaincontrol-1' => 'Snadne',
'exif-gaincontrol-2' => 'Wysoke zesylnjenje',
'exif-gaincontrol-3' => 'Niske wosłabjenje',
'exif-gaincontrol-4' => 'Wysoke wosłabjenje',

'exif-contrast-0' => 'Normalny',
'exif-contrast-1' => 'Mjechki',
'exif-contrast-2' => 'Sylny',

'exif-saturation-0' => 'Normalna nasyćenosć',
'exif-saturation-1' => 'Niska nasyćenosć',
'exif-saturation-2' => 'Wysoka nasyćenosć',

'exif-sharpness-0' => 'Normalna',
'exif-sharpness-1' => 'Mjechka',
'exif-sharpness-2' => 'Sylna',

'exif-subjectdistancerange-0' => 'Njeznata',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Bliski pohlad',
'exif-subjectdistancerange-3' => 'Zdaleny pohlad',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Sewjerna šěrina',
'exif-gpslatitude-s' => 'Južna šěrina',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Wuchodna dołhosć',
'exif-gpslongitude-w' => 'Zapadna dołhosć',

'exif-gpsstatus-a' => 'Měrjenje běži',
'exif-gpsstatus-v' => 'Interoperabilita měrjenja',

'exif-gpsmeasuremode-2' => 'dwudimensionalne měrjenje',
'exif-gpsmeasuremode-3' => 'třidimensionalne měrjenje',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mila/h',
'exif-gpsspeed-n' => 'Suki',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Woprawdźity směr',
'exif-gpsdirection-m' => 'Magnetiski směr',

# External editor support
'edit-externally'      => 'Dataju z eksternym programom wobdźěłać',
'edit-externally-help' => 'Hlej [http://www.mediawiki.org/wiki/Manual:External_editors pokiwy za instalaciju] za dalše informacije.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'wšě',
'imagelistall'     => 'wšě',
'watchlistall2'    => 'wšě',
'namespacesall'    => 'wšě',
'monthsall'        => 'wšě',

# E-mail address confirmation
'confirmemail'             => 'Emailowu adresu wobkrućić',
'confirmemail_noemail'     => 'Njejsy płaćiwu e-mejlowu adresu w swojich [[Special:Preferences|nastajenjach]] podał.',
'confirmemail_text'        => 'Tutón wiki žada, zo swoju e-mejlowu adresu wobkrućiš, prjedy hač e-mejlowe funkcije wužiješ. Zaktiwuzij tłóčatko deleka, zo by swojej adresy wobkrućensku mejlku pósłał. Mejlka zapřijmje wotkaz, kotryž kod wobsahuje; wočiń wotkaz we swojim wobhladowaku, zo by wobkrućił, zo twoja e-mejlowa adresa je płaćiwa.',
'confirmemail_pending'     => '<div class="error"> Potwjerdźenski kod bu hižo z e-mejlu připósłany. Jeli sy runje swoje konto wutworił, wočakaj prošu někotre mjeńšiny, prjedy hač sej nowy kod žadaš.</div>',
'confirmemail_send'        => 'Wobkrućenski kod pósłać',
'confirmemail_sent'        => 'Wobkrućenska mejlka bu wotesłana.',
'confirmemail_oncreate'    => 'Wobkrućenski kod bu na twoju e-mejlowu adresu pósłany. Tutón kod za přizjewjenje trěbne njeje, trjebaš jón pak, zo by e-mejlowe funkcije we wikiju aktiwizował.',
'confirmemail_sendfailed'  => '{{SITENAME}} njemóžeše twoje potwjerdźensku e-mejlku pósłać. Přepytaj prošu swoju e-mejlowu adresu za njepłaćiwymi znamješkami.

E-mejlowy program je wróćił: $1',
'confirmemail_invalid'     => 'Njepłaćiwy wobkrućacy kod. Kod je snano spadnył.',
'confirmemail_needlogin'   => 'Dyrbiš so $1, zo by e-mejlowu adresu wobkrućić móhł.',
'confirmemail_success'     => 'Twoja e-mejlowa adresa bu wobkrućena. Móžeš so nětko přizjewić.',
'confirmemail_loggedin'    => 'Twoja e-mejlowa adresu bu nětko wobkrućena.',
'confirmemail_error'       => 'Zmylk při wobkrućenju twojeje e-mailoweje adresy.',
'confirmemail_subject'     => '{{SITENAME}} – wobkrućenje e-mejloweje adresy',
'confirmemail_body'        => 'Něchtó, najskerje ty z IP-adresu $1, je wužiwarske konto "$2" z tutej e-mejlowej adresu we {{GRAMMAR:lokatiw|{{SITENAME}}}} zregistrował.

Zo by so wobkrućiło, zo tute konto woprawdźe tebi słuša a zo bychu so e-mejlowe funkcije we {{GRAMMAR:lokatiw|{{SITENAME}}}} zaktiwizowali, wočiń tutón wotkaz w swojim wobhladowaku:

$3

Jeli *njej*sy konto zregistrował, slěduj wotkaz, zo by wobkrućenje e-mejloweje adresy přetorhnył:

$5

Tute wobkrućenski kod spadnje $4.',
'confirmemail_invalidated' => 'E-mejlowe potwjerdźenje přetorhnjene',
'invalidateemail'          => 'E-mejlowe potwjerdźenje přetorhnyć',

# Scary transclusion
'scarytranscludedisabled' => '[Zapřijeće mjezyrěčnych wotkazow je znjemóžnjene]',
'scarytranscludefailed'   => '[Zapřijimanje předłohi za $1 je so njeporadźiło]',
'scarytranscludetoolong'  => '[URL je předołhi]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">Trackbacks za tutón nastawk:<br />
$1</div>',
'trackbackremove'   => '([$1 wušmórnyć])',
'trackbackdeleteok' => 'Trackback bu wuspěšnje wušmórnjeny.',

# Delete conflict
'deletedwhileediting' => "'''Kedźbu''': Tuta strona bu wušmórnjena, po tym zo sy započał ju wobdźěłać!",
'confirmrecreate'     => "Wužiwar [[User:$1|$1]] ([[User talk:$1|diskusija]]) je stronu wušmórnył, po tym zo sy započał ju wobdźěłać. Přičina:
: ''$2''
Prošu potwjerdź, zo chceš tutu stronu woprawdźe znowa wutworić.",
'recreate'            => 'Znowa wutworić',

# HTML dump
'redirectingto' => 'Posrědkuju k stronje [[:$1]]',

# action=purge
'confirm_purge'        => 'Pufrowak strony wuprózdnić? $1',
'confirm_purge_button' => 'W porjadku',

# AJAX search
'searchcontaining' => 'Strony pytać, kotrež <i>$1</i> wobsahuja.',
'searchnamed'      => 'Strony pytać, w kotrychž titlach so <i>$1</i> jewi.',
'articletitles'    => 'Strony pytać, kotrež so z <i>$1</i> započinaja',
'hideresults'      => 'Wuslědki schować',
'useajaxsearch'    => 'Pytanje AJAX wužiwać',

# Multipage image navigation
'imgmultipageprev' => '← předchadna strona',
'imgmultipagenext' => 'přichodna strona →',
'imgmultigo'       => 'Dźi!',
'imgmultigoto'     => 'Dźi k stronje $1',

# Table pager
'ascending_abbrev'         => 'postupowacy',
'descending_abbrev'        => 'zestupowacy',
'table_pager_next'         => 'přichodna strona',
'table_pager_prev'         => 'předchadna strona',
'table_pager_first'        => 'prěnja strona',
'table_pager_last'         => 'poslednja strona',
'table_pager_limit'        => '$1 {{PLURAL:$1|wuslědk|wuslědkaj|wuslědki|wuslědkow}} na stronu pokazać',
'table_pager_limit_submit' => 'OK',
'table_pager_empty'        => 'Žane wuslědki',

# Auto-summaries
'autosumm-blank'   => 'Strona bu wuprózdnjena',
'autosumm-replace' => "Strona bu z hinašim tekstom přepisana: '$1'",
'autoredircomment' => 'posrědkuju k stronje „[[$1]]”',
'autosumm-new'     => 'nowa strona: $1',

# Size units
'size-kilobytes' => '$1 kB',

# Live preview
'livepreview-loading' => 'Čita so…',
'livepreview-ready'   => 'Začitanje… Hotowe!',
'livepreview-failed'  => 'Dynamiski přehlad njemóžno!
Spytaj normalny přehlad.',
'livepreview-error'   => 'Zwisk njemóžno: $1 "$2"
Spytaj normalny přehlad.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Změny {{PLURAL:$1|zašłeje $1 sekundy|zašłeju $1 sekundow|zašłych $1 sekundow|zašłych $1 sekundow}} so w tutej lisćinje hišće njezwobraznjeja.',
'lag-warn-high'   => 'Wućeženja datoweje banki dla so změny {{PLURAL:$1|zašłeje $1 sekundy|zašłeje $1 sekundow|zašłych $1 sekundow|zašłych $1 sekundow}} w tutej lisćinje hišće njepokazuja.',

# Watchlist editor
'watchlistedit-numitems'       => 'Twoje wobkedźbowanki wobsahuja {{PLURAL:$1|1 zapisk|$1 zapiskaj|$1 zapiski|$1 zapiskow}}, diskusijne strony njejsu ličene.',
'watchlistedit-noitems'        => 'Twoje wobkedźbowanki su prózdne.',
'watchlistedit-normal-title'   => 'Wobkedźbowanki wobdźěłać',
'watchlistedit-normal-legend'  => 'Zapiski z wobkedźbowankow wotstronić',
'watchlistedit-normal-explain' => 'Tu su zapiski z twojich wobkedźbowankow. Zo by zapiski wušmórnył, markěruj kašćiki pódla zapiskow a klikń na „Zapiski wušmórnyć“. Móžeš tež swoje wobkedźbowanki [[Special:Watchlist/raw|w lisćinowym formaće wobdźěłać]].',
'watchlistedit-normal-submit'  => 'Zapiski wotstronić',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 zapisk bu|$1 zapiskaj buštej|$1 zapiski buchu|$1 zapiskow  buchu}} z twojich wobkedźbowankow {{PLURAL:$1|wotstronjeny|wotstronjenej|wotstronjene|wotstronjene}}:',
'watchlistedit-raw-title'      => 'Wobkedźbowanki w lisćinowym formaće wobdźěłać',
'watchlistedit-raw-legend'     => 'Wobkedźbowanki w lisćinowym formaće wobdźěłać',
'watchlistedit-raw-explain'    => 'To su twoje wobkedźbowanki w lisćinowym formaće. Zapiski hodźa so po linkach wušmórnyć abo přidać.
Na linku je jedyn zapisk dowoleny.
Hdyž sy hotowy, klikń na „wobkedźbowanki składować“.
Móžeš tež [[Special:Watchlist/edit|standardnu wobdźěłowansku stronu]] wužiwać.',
'watchlistedit-raw-titles'     => 'Zapiski:',
'watchlistedit-raw-submit'     => 'Wobkedźbowanki składować',
'watchlistedit-raw-done'       => 'Twoje wobkedźbowanki buchu składowane.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 zapisk bu dodaty|$1 zapiskaj buštej dodatej|$1 zapiski buchu dodate|$1 zapiskow buchu dodate}}:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 zapisk bu wotstronjeny|$1 zapiskaj buštej wotstronjenej|$1 zapiski buchu wotstronjene|$1 zapiskow buchu wotstronjene}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Wobkedźbowanki: Změny',
'watchlisttools-edit' => 'normalnje wobdźěłać',
'watchlisttools-raw'  => 'Lisćinowy format wobdźěłać (import/eksport)',

# Iranian month names
'iranian-calendar-m2' => 'Ordibehešt',

# Core parser functions
'unknown_extension_tag' => 'Njeznata taflička rozšěrjenja "$1"',

# Special:Version
'version'                          => 'Wersija', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Instalowane rozšěrjenja',
'version-specialpages'             => 'Specialne strony',
'version-parserhooks'              => 'Parserowe hoki',
'version-variables'                => 'Wariable',
'version-other'                    => 'Druhe',
'version-mediahandlers'            => 'Předźěłaki medijow',
'version-hooks'                    => 'Hoki',
'version-extension-functions'      => 'Funkcije rozšěrjenjow',
'version-parser-extensiontags'     => "Parserowe rozšěrjenja ''(taflički)''",
'version-parser-function-hooks'    => 'Parserowe funkcije',
'version-skin-extension-functions' => 'Rozšěrjenske funkcije za šaty',
'version-hook-name'                => 'Mjeno hoki',
'version-hook-subscribedby'        => 'Abonowany wot',
'version-version'                  => 'Wersija',
'version-license'                  => 'Licenca',
'version-software'                 => 'Instalowana software',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Wersija',

# Special:FilePath
'filepath'         => 'Datajowy puć',
'filepath-page'    => 'Dataja:',
'filepath-submit'  => 'Puć',
'filepath-summary' => 'Tuta specialna strona wróća dospołny puć aktualneje datajoweje wersije. Wobrazy so połnym rozeznaću pokazuja, druhe datajowe typy so ze zwjazanym programom startuja.

Zapodaj datajowe mjeno bjez dodawka "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Dwójne dataje pytać',
'fileduplicatesearch-summary'  => "Pytanje za duplikatnymi datajemi na zakładźe jich hašoweje hódnoty.

Zapodaj datajowe mjeno '''bjez''' prefiksa \"{{ns:image}}:\".",
'fileduplicatesearch-legend'   => 'Duplikaty pytać',
'fileduplicatesearch-filename' => 'Datajowe mjeno:',
'fileduplicatesearch-submit'   => 'Pytać',
'fileduplicatesearch-info'     => '$1 × $2 pikselow<br />Datajowa wulkosć: $3<br />Typ MIME: $4',
'fileduplicatesearch-result-1' => 'Dataja "$1" identiske duplikaty nima.',
'fileduplicatesearch-result-n' => 'Dataja "$1" ma {{PLURAL:$2|1 identiski duplikat|$2 identiskej duplikataj|$2 identiske duplikaty|$2 identiskich duplikatow}}.',

# Special:SpecialPages
'specialpages'                   => 'Specialne strony',
'specialpages-note'              => '----
* Normalne specialne strony.
* <span class="mw-specialpagerestricted">Specialne strony z wobmjezowanym přistupom</span>',
'specialpages-group-maintenance' => 'Hladanske lisćiny',
'specialpages-group-other'       => 'Druhe specialne strony',
'specialpages-group-login'       => 'Přizjewjenje',
'specialpages-group-changes'     => 'Poslednje změny a protokole',
'specialpages-group-media'       => 'Medije',
'specialpages-group-users'       => 'Wužiwarjo a prawa',
'specialpages-group-highuse'     => 'Často wužiwane strony',
'specialpages-group-pages'       => 'Lisćina stronow',
'specialpages-group-pagetools'   => 'Nastroje stronow',
'specialpages-group-wiki'        => 'Wikijowe daty a nastroje',
'specialpages-group-redirects'   => 'Daleposrědkowace specialne strony',
'specialpages-group-spam'        => 'Spamowe nastroje',

# Special:BlankPage
'blankpage'              => 'Prózdna strona',
'intentionallyblankpage' => 'Tuta strona je z wotpohladom prózdna.',

);
