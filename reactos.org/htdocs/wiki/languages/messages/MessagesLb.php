<?php
/** Luxembourgish (Lëtzebuergesch)
 *
 * @ingroup Language
 * @file
 *
 * @author Kaffi
 * @author Robby
 * @author לערי ריינהארט
 */

$fallback = 'de';

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Spezial',
	NS_TALK           => 'Diskussioun',
	NS_USER           => 'Benotzer',
	NS_USER_TALK      => 'Benotzer_Diskussioun',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => '$1_Diskussioun',
	NS_IMAGE          => 'Bild',
	NS_IMAGE_TALK     => 'Bild_Diskussioun',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki_Diskussioun',
	NS_TEMPLATE       => 'Schabloun',
	NS_TEMPLATE_TALK  => 'Schabloun_Diskussioun',
	NS_HELP           => 'Hëllef',
	NS_HELP_TALK      => 'Hëllef_Diskussioun',
	NS_CATEGORY       => 'Kategorie',
	NS_CATEGORY_TALK  => 'Kategorie_Diskussioun',
);

$skinNames = array(
	'standard'    => 'Klassesch',
	'nostalgia'   => 'Nostalgie',
	'cologneblue' => 'Köln Blo',
	'monobook'    => 'MonoBook',
	'myskin'      => 'MySkin',
	'chick'       => 'Chick',
	'simple'      => 'Einfach',
	'modern'      => 'Modern',
);

$specialPageAliases = array(
	'DoubleRedirects'         => array( 'Duebel Viruleedungen' ),
	'BrokenRedirects'         => array( 'Futtis Viruleedungen' ),
	'Disambiguations'         => array( 'Homonymie' ),
	'Userlogin'               => array( 'Umellen' ),
	'Userlogout'              => array( 'Ofmellen' ),
	'CreateAccount'           => array( 'Benotzerkont opmaachen' ),
	'Preferences'             => array( 'Astellungen' ),
	'Watchlist'               => array( 'Iwwerwaachungslëscht' ),
	'Recentchanges'           => array( 'Rezent Ännerungen' ),
	'Upload'                  => array( 'Eroplueden' ),
	'Imagelist'               => array( 'Billerlëscht' ),
	'Newimages'               => array( 'Nei Biller' ),
	'Listusers'               => array( 'Benotzer' ),
	'Listgrouprights'         => array( 'Grupperechter' ),
	'Statistics'              => array( 'Statistik' ),
	'Randompage'              => array( 'Zoufälleg Säit' ),
	'Lonelypages'             => array( 'Weesesäiten' ),
	'Uncategorizedpages'      => array( 'Säiten ouni Kategorie' ),
	'Uncategorizedcategories' => array( 'Kategorien ouni Kategorie' ),
	'Uncategorizedimages'     => array( 'Biller ouni Kategorie' ),
	'Uncategorizedtemplates'  => array( 'Schablounen ouni Kategorie' ),
	'Unusedcategories'        => array( 'Onbenotze Kategorien' ),
	'Unusedimages'            => array( 'Onbenotzte Biller' ),
	'Wantedpages'             => array( 'Gewënschte Säiten' ),
	'Wantedcategories'        => array( 'Gewënschte Kategorien' ),
	'Missingfiles'            => array( 'Fichieren déi feelen' ),
	'Mostlinked'              => array( 'Dacks verlinkte Säiten' ),
	'Mostlinkedcategories'    => array( 'Dacks benotzte Kategorien' ),
	'Mostlinkedtemplates'     => array( 'Dacks benotzte Schablounen' ),
	'Mostcategories'          => array( 'Säite mat de meeschte Kategorien' ),
	'Mostimages'              => array( 'Dacks benotzte Biller' ),
	'Mostrevisions'           => array( 'Säite mat de meeschten Ännerungen' ),
	'Fewestrevisions'         => array( 'Säite mat de mannsten Ännerungen' ),
	'Shortpages'              => array( 'Kuerz Säiten' ),
	'Longpages'               => array( 'Laang Säiten' ),
	'Newpages'                => array( 'Nei Säiten' ),
	'Ancientpages'            => array( 'Al Säiten' ),
	'Deadendpages'            => array( 'Saackgaassesäiten' ),
	'Protectedpages'          => array( 'Protegéiert Säiten' ),
	'Protectedtitles'         => array( 'Gespaarte Säiten' ),
	'Allpages'                => array( 'All Säiten' ),
	'Prefixindex'             => array( 'Indexsich' ),
	'Ipblocklist'             => array( 'Lëscht vu gespaarten IPen a Benotzer' ),
	'Specialpages'            => array( 'Spezialsäiten' ),
	'Contributions'           => array( 'Kontributiounen' ),
	'Emailuser'               => array( 'Dësem Benotzer eng E-Mail schécken' ),
	'Confirmemail'            => array( 'E-Mail confirméieren' ),
	'Whatlinkshere'           => array( 'Linken op dës Säit' ),
	'Recentchangeslinked'     => array( 'Ännerungen op verlinkte Säiten' ),
	'Movepage'                => array( 'Säit réckelen' ),
	'Blockme'                 => array( 'Mech spären' ),
	'Booksources'             => array( 'Bicher mat hirer ISBN sichen' ),
	'Categories'              => array( 'Kategorien' ),
	'Export'                  => array( 'Exportéieren' ),
	'Version'                 => array( 'Versioun' ),
	'Allmessages'             => array( 'All Systemmessagen' ),
	'Log'                     => array( 'Logbicher' ),
	'Blockip'                 => array( 'Spären' ),
	'Undelete'                => array( 'Restauréieren' ),
	'Import'                  => array( 'Importéieren' ),
	'Lockdb'                  => array( 'Datebank spären' ),
	'Unlockdb'                => array( 'Spär vun der Datebank ophiewen' ),
	'Userrights'              => array( 'Benotzerrechter' ),
	'MIMEsearch'              => array( 'Sich no MIME-Zorten' ),
	'FileDuplicateSearch'     => array( 'Sich no duebele Fichieren' ),
	'Unwatchedpages'          => array( 'Säiten déi net iwwerwaacht ginn' ),
	'Listredirects'           => array( 'Viruleedungen' ),
	'Revisiondelete'          => array( 'Versioun läschen' ),
	'Unusedtemplates'         => array( 'Onbenotzte Schablounen' ),
	'Randomredirect'          => array( 'Zoufälleg Viruleedung' ),
	'Mypage'                  => array( 'Meng Benotzersäit' ),
	'Mytalk'                  => array( 'Meng Diskussiounssäit' ),
	'Mycontributions'         => array( 'Meng Kontributiounen' ),
	'Listadmins'              => array( 'Lëscht vun den Administrateuren' ),
	'Listbots'                => array( 'Botten' ),
	'Popularpages'            => array( 'Beléiwste Säiten' ),
	'Search'                  => array( 'Sichen' ),
	'Resetpass'               => array( 'Passwuert zrécksetzen' ),
	'Withoutinterwiki'        => array( 'Säiten ouni Interwiki-Linken' ),
	'MergeHistory'            => array( 'Versiounen zesummeleeën' ),
	'Filepath'                => array( 'Pad bäi de Fichier' ),
	'Invalidateemail'         => array( 'E-Mailadress net confirméieren' ),
	'Blankpage'               => array( 'Eidel Säit' ),
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Linken ënnersträichen:',
'tog-highlightbroken'         => 'Format vu futtise Linken <a href="" class="new">esou</a> (alternativ: <a href="" class="internal">?</a>).',
'tog-justify'                 => "Ränner vum Text riten (''justify'')",
'tog-hideminor'               => 'Verstopp kleng Ännerungen an de rezenten Ännerungen',
'tog-extendwatchlist'         => 'Iwwerwaachungslëscht op all Ännerungen ausbreeden',
'tog-usenewrc'                => 'Mat JavaScript erweidert rezent Ännerungen (klappt net mat all Browser)',
'tog-numberheadings'          => 'Iwwerschrëften automatesch numeréieren',
'tog-showtoolbar'             => 'Ännerungstoolbar weisen (JavaScript)',
'tog-editondblclick'          => 'Säite mat Duebelklick veränneren (JavaScript)',
'tog-editsection'             => "Linke fir d'Ännere vun eenzelnen Abschnitte weisen",
'tog-editsectiononrightclick' => 'Eenzel Abschnitte per Rietsklick änneren (JavaScript)',
'tog-showtoc'                 => 'Inhaltsverzeechniss weisen bäi Säite mat méi wéi dräi Iwwerschrëften',
'tog-rememberpassword'        => 'Mäi Passwuert op dësem Computer verhalen',
'tog-editwidth'               => 'Verännerungskëscht iwwert déi ganz Breed vum Ecran',
'tog-watchcreations'          => 'Säiten déi ech nei uleeën automatesch op meng Iwwerwaachungslëscht setzen',
'tog-watchdefault'            => 'Säiten déi ech änneren op meng Iwwerwaachungslëscht setzen',
'tog-watchmoves'              => 'Säiten déi ech réckelen automatesch op meng Iwwerwaachungslëscht setzen',
'tog-watchdeletion'           => 'Säiten déi ech läschen op meng Iwwerwaachungslëscht setzen',
'tog-minordefault'            => "Alles wat ech änneren automatesch als 'Kleng Ännerungen' weisen",
'tog-previewontop'            => "De ''Preview'' uewen un der Ännerungsfënster weisen",
'tog-previewonfirst'          => "Beim éischten Änneren de ''Preview'' weisen.",
'tog-nocache'                 => 'Säitecache deaktivéieren',
'tog-enotifwatchlistpages'    => 'Schéckt mir eng E-Mail wann eng vun de Säiten op menger Iwwerwaachungslëscht geännert gëtt',
'tog-enotifusertalkpages'     => 'Schéckt mir E-Maile wa meng Diskussiounssäit geännert gëtt.',
'tog-enotifminoredits'        => 'Schéckt mir och bäi kléngen Ännerungen op vu mir iwwerwaachte Säiten eng E-Mail.',
'tog-enotifrevealaddr'        => 'Meng E-Mailadress an de Benoriichtigungsmaile weisen.',
'tog-shownumberswatching'     => "D'Zuel vun de Benotzer déi dës Säit iwwerwaache weisen",
'tog-fancysig'                => 'Ënnerschrëft ouni automatesche Link op déi eege Benotzersäit',
'tog-externaleditor'          => 'Externen Editor als Standard benotzen (Nëmme fir Experten, et musse seziell Astellungen op ärem Computer gemaach ginn)',
'tog-externaldiff'            => 'En Externen Diff-Programm als Standard benotzen (nëmme fir Experten, et musse speziell Astellungen op ärem Computer gemaach ginn)',
'tog-showjumplinks'           => 'Aktivéiere vun de "Sprang op"-Linken',
'tog-uselivepreview'          => 'Live-Preview notzen (JavaScript) (experimentell)',
'tog-forceeditsummary'        => 'Warnen, wa beim Späicheren de Resumé feelt',
'tog-watchlisthideown'        => 'Meng Ännerungen op menger Iwwerwaachungslëscht verstoppen',
'tog-watchlisthidebots'       => 'Ännerunge vu Botten op menger Iwwerwaachungslëscht verstoppen',
'tog-watchlisthideminor'      => 'Kleng Ännerungen op menger Iwwerwaachungslëscht verstoppen',
'tog-nolangconversion'        => 'Ëmwandlung vu Sproochvarianten ausschalten',
'tog-ccmeonemails'            => 'Schéck mir eng Kopie vun de Mailen, déi ech anere Benotzer schécken.',
'tog-diffonly'                => "Weis bei Versiounevergläicher just d'Ënnerscheeder an net déi ganz Säit",
'tog-showhiddencats'          => 'Verstoppte Kategorie weisen',

'underline-always'  => 'ëmmer',
'underline-never'   => 'Ni',
'underline-default' => 'vun der Browserastellung ofhängeg',

'skinpreview' => '(Kucken)',

# Dates
'sunday'        => 'Sonndeg',
'monday'        => 'Méindeg',
'tuesday'       => 'Dënschdeg',
'wednesday'     => 'Mëttwoch',
'thursday'      => 'Donneschdeg',
'friday'        => 'Freideg',
'saturday'      => 'Samsdeg',
'sun'           => 'Son',
'mon'           => 'Méi',
'tue'           => 'Dën',
'wed'           => 'Mët',
'thu'           => 'Don',
'fri'           => 'Fre',
'sat'           => 'Sam',
'january'       => 'Januar',
'february'      => 'Februar',
'march'         => 'Mäerz',
'april'         => 'Abrëll',
'may_long'      => 'Mee',
'june'          => 'Juni',
'july'          => 'Juli',
'august'        => 'August',
'september'     => 'September',
'october'       => 'Oktober',
'november'      => 'November',
'december'      => 'Dezember',
'january-gen'   => 'Januar',
'february-gen'  => 'Februar',
'march-gen'     => 'Mäerz',
'april-gen'     => 'Abrëll',
'may-gen'       => 'Mee',
'june-gen'      => 'Juni',
'july-gen'      => 'Juli',
'august-gen'    => 'August',
'september-gen' => 'September',
'october-gen'   => 'Oktober',
'november-gen'  => 'November',
'december-gen'  => 'Dezember',
'jan'           => 'Jan.',
'feb'           => 'Feb.',
'mar'           => 'Mäe.',
'apr'           => 'Abr.',
'may'           => 'Mee',
'jun'           => 'Jun.',
'jul'           => 'Jul.',
'aug'           => 'Aug.',
'sep'           => 'Sep.',
'oct'           => 'Okt.',
'nov'           => 'Nov.',
'dec'           => 'Dez.',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorie|Kategorien}}',
'category_header'                => 'Säiten an der Kategorie "$1"',
'subcategories'                  => 'Ënnerkategorien',
'category-media-header'          => 'Medien an der Kategorie "$1"',
'category-empty'                 => "''Dës Kategorie ass fir den Ament eidel''",
'hidden-categories'              => '{{PLURAL:$1|Verstoppte Kategorie|Verstoppte Kategorien}}',
'hidden-category-category'       => 'Verstoppte Kategorien', # Name of the category where hidden categories will be listed
'category-subcat-count'          => 'Dës Kategorie huet {{PLURAL:$2|nëmmen dës Ënnerkategorie.|dës {{PLURAL:$1|Ënnerkategorie|$1 Ënnerkategorien}}, vun $2 am Ganzen.}}',
'category-subcat-count-limited'  => 'Dës Kategorie huet dës {{PLURAL:$1|Ënnerkategorie|$1 Ënnerkategorien}}.',
'category-article-count'         => 'An dëser Kategorie {{PLURAL:$2|ass just dës Säit.|{{PLURAL:$1|ass just dës Säit|si(nn) $1 Säiten}}, vu(n) $2 am Ganzen.}}',
'category-article-count-limited' => 'Dës {{PLURAL:$1|Säit ass|$1 Säite sinn}} an dëser Kategorie.',
'category-file-count'            => 'An dëser Kategorie {{PLURAL:$2|ass just dëse Fichier.|{{PLURAL:$1|ass just dëse Fichier|si(nn) $1 Fichieren}}, vun $2 am Ganzen.}}',
'category-file-count-limited'    => '{{PLURAL:$1|Dëse Fichier ass|Dës $1 Fichieren sinn}} an dëser Kategorie.',
'listingcontinuesabbrev'         => '(Fortsetzung)',

'mainpagetext'      => "<big>'''MediaWiki gouf installéiert.'''</big>",
'mainpagedocfooter' => "Kuckt w.e.g. [http://meta.wikimedia.org/wiki/Help:Contents d'Benotzerhandbuch] fir den Interface ze personnaliséieren.

== Starthëllefen ==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Hëllef bei der Konfiguratioun]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki-FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Mailinglëscht vun neie MediaWiki-Versiounen]",

'about'          => 'A propos',
'article'        => 'Säit',
'newwindow'      => '(geet an enger neier Fënster op)',
'cancel'         => 'Zréck',
'qbfind'         => 'Fannen',
'qbbrowse'       => 'Duerchsichen',
'qbedit'         => 'Änneren',
'qbpageoptions'  => 'Säitenoptiounen',
'qbpageinfo'     => 'Kontext',
'qbmyoptions'    => 'Meng Säiten',
'qbspecialpages' => 'Spezialsäiten',
'moredotdotdot'  => 'Méi …',
'mypage'         => 'meng Säit',
'mytalk'         => 'meng Diskussioun',
'anontalk'       => 'Diskussioun fir dës IP Adress',
'navigation'     => 'Navigatioun',
'and'            => 'an',

# Metadata in edit box
'metadata_help' => 'Metadaten:',

'errorpagetitle'    => 'Feeler',
'returnto'          => 'Zréck op $1.',
'tagline'           => 'Vu {{SITENAME}}',
'help'              => 'Hëllef',
'search'            => 'Sichen',
'searchbutton'      => 'Volltext-Sich',
'go'                => 'Lass',
'searcharticle'     => 'Säit',
'history'           => 'Historique vun der Säit',
'history_short'     => 'Versiounen',
'updatedmarker'     => "geännert zënter dat ech d'Säit fir d'lescht gekuckt hunn",
'info_short'        => 'Informatioun',
'printableversion'  => 'Printversioun',
'permalink'         => 'Zitéierfäege Link',
'print'             => 'Drécken',
'edit'              => 'Änneren',
'create'            => 'Uleeën',
'editthispage'      => 'Dës Säit änneren',
'create-this-page'  => 'Dës Säit uleeën',
'delete'            => 'Läschen',
'deletethispage'    => 'Dës Säit läschen',
'undelete_short'    => '$1 {{PLURAL:$1|Versioun|Versiounen}} restauréieren',
'protect'           => 'Protegéieren',
'protect_change'    => 'änneren',
'protectthispage'   => 'Dës Säit schützen',
'unprotect'         => 'Deprotegéieren',
'unprotectthispage' => 'Protectioun vun dëser Säit annulléieren',
'newpage'           => 'Nei Säit',
'talkpage'          => 'Diskussioun',
'talkpagelinktext'  => 'Diskussioun',
'specialpage'       => 'Spezialsäit',
'personaltools'     => 'Perséinlech Tools',
'postcomment'       => 'Bemierkung derbäisetzen',
'articlepage'       => 'Säit',
'talk'              => 'Diskussioun',
'views'             => 'Offroen',
'toolbox'           => 'Geschirkëscht',
'userpage'          => 'Benotzersäit',
'projectpage'       => 'Meta-Text',
'imagepage'         => 'Billersäit kucken',
'mediawikipage'     => 'Säit mat de Message weisen',
'templatepage'      => 'Schabloune(säit) weisen',
'viewhelppage'      => 'Hëllefssäit weisen',
'categorypage'      => 'Kategoriesäit weisen',
'viewtalkpage'      => 'Diskussioun weisen',
'otherlanguages'    => 'Aner Sproochen',
'redirectedfrom'    => '(Virugeleet vun $1)',
'redirectpagesub'   => 'Viruleedungssäit',
'lastmodifiedat'    => "Dës Säit gouf den $1 ëm $2 Auer fir d'lescht geännert.", # $1 date, $2 time
'viewcount'         => 'Dës Säit gouf bis elo {{PLURAL:$1|emol|$1-mol}} ofgefrot.',
'protectedpage'     => 'Protegéiert Säit',
'jumpto'            => 'Wiesselen op:',
'jumptonavigation'  => 'Navigatioun',
'jumptosearch'      => 'Sich',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Iwwer {{SITENAME}}',
'aboutpage'            => 'Project: Iwwer {{SITENAME}}',
'bugreports'           => 'Feelermeldungen',
'bugreportspage'       => 'Project:Feelermeldungen',
'copyright'            => 'Inhalt ass zur Verfügung gestallt ënnert der $1.<br />',
'copyrightpagename'    => '{{SITENAME}} Copyright',
'copyrightpage'        => '{{ns:project}}:Copyright',
'currentevents'        => 'Aktualitéit',
'currentevents-url'    => 'Project:Aktualitéit',
'disclaimers'          => 'Impressum',
'disclaimerpage'       => 'Project:Impressum',
'edithelp'             => 'Hëllef beim Änneren',
'edithelppage'         => 'Help:Wéi änneren ech eng Säit',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Hëllef',
'mainpage'             => 'Haaptsäit',
'mainpage-description' => 'Haaptsäit',
'policy-url'           => 'Project:Richtlinnen',
'portal'               => '{{SITENAME}}-Portal',
'portal-url'           => 'Project:Kommunautéit',
'privacy'              => 'Dateschutz',
'privacypage'          => 'Project:Dateschutz',

'badaccess'        => 'Net genuch Rechter',
'badaccess-group0' => 'Dir hutt net déi néideg Rechter fir dës Aktioun duerchzeféieren.',
'badaccess-group1' => "D'Aktioun déi dir gewielt hutt, kann nëmme vu Benotzer aus de Gruppen $1 duerchgefouert ginn.",
'badaccess-group2' => "D'Aktioun déi dir gewielt hutt, kann nëmme vu Benotzer aus enger vun den $1 Gruppen duerchgefouert ginn.",
'badaccess-groups' => "D'Aktioun déi dir gewielt hutt, kann nëmme vu Benotzer aus de Gruppen $1 duerchgefouert ginn.",

'versionrequired'     => 'Versioun $1 vu MediaWiki gëtt gebraucht',
'versionrequiredtext' => "D'Versioun $1 vu MediaWiki ass néideg, fir dës Säit ze notzen. Kuckt d'[[Special:Version|Versiounssäit]]",

'ok'                      => 'OK',
'retrievedfrom'           => 'Vun „$1“',
'youhavenewmessages'      => 'Dir hutt $2 op ärer $1.',
'newmessageslink'         => 'nei Messagen',
'newmessagesdifflink'     => 'Nei Messagen',
'youhavenewmessagesmulti' => 'Dir huet nei Messagen op $1',
'editsection'             => 'änneren',
'editold'                 => 'änneren',
'viewsourceold'           => 'Quellcode kucken',
'editsectionhint'         => 'Abschnitt veränneren: $1',
'toc'                     => 'Inhaltsverzeechnis',
'showtoc'                 => 'weisen',
'hidetoc'                 => 'verstoppen',
'thisisdeleted'           => '$1 kucken oder zrécksetzen?',
'viewdeleted'             => 'Weis $1?',
'restorelink'             => '$1 geläschte {{PLURAL:$1|Versioun|Versiounen}}',
'feedlinks'               => 'Feed:',
'feed-unavailable'        => "''Syndication Feeds'' stinn net zur Verfügung.",
'site-rss-feed'           => 'RSS-Feed fir $1',
'site-atom-feed'          => 'Atom-Feed fir $1',
'page-rss-feed'           => 'RSS-Feed fir "$1"',
'page-atom-feed'          => 'Atom-Feed fir "$1"',
'red-link-title'          => '$1 (Säit gëtt et (nach) net)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Säit',
'nstab-user'      => 'Benotzersäit',
'nstab-media'     => 'Media Säit',
'nstab-special'   => 'Spezialsäit',
'nstab-project'   => 'Portalsäit',
'nstab-image'     => 'Fichier',
'nstab-mediawiki' => 'Systemmessage',
'nstab-template'  => 'Schabloun',
'nstab-help'      => 'Hëllef-Säit',
'nstab-category'  => 'Kategorie',

# Main script and global functions
'nosuchaction'      => 'Dës Aktioun gëtt et net',
'nosuchactiontext'  => 'Déi Aktioun, déi an der URL ugi war, gëtt vun dëser Wiki net ënnerstetzt.',
'nosuchspecialpage' => 'Spezialsäit gëtt et net',
'nospecialpagetext' => "<big>'''Dir hutt eng Spezialsäit ofgefrot déi et net gëtt.'''</big>

All Spezialsäiten déi et gëtt sinn op der [[Special:SpecialPages|Lescht vun de Spezialsäiten]] ze fannen.",

# General errors
'error'                => 'Feeler',
'databaseerror'        => 'Datebank Feeler',
'dberrortext'          => 'En Datebank Syntax Feeler ass opgetrueden. De läschten Datebank Query war: "$1" vun der Funktioun "$2". MySQL Feeler "$3: $4".',
'dberrortextcl'        => 'En Datebank Syntax Feeler ass opgetrueden. De läschten Datebank Query war: "$1" vun der Funktioun "$2". De MySQL Feeler war "$3: $4".',
'noconnect'            => 'Et gëtt zur Zäit technesch Problemer op dëser Wiki, an et konnt keng Verbindung mat dem Datebankserver opgemaach ginn.<br />
$1',
'nodb'                 => "D'Datebank $1 konnt net gewielt ginn",
'cachederror'          => 'Folgend Säit ass eng Kopie aus dem Cache an net onbedéngt aktuell.',
'laggedslavemode'      => 'Opgepasst: Dës Säit ass net onbedingt um neiste Stand.',
'readonly'             => "D'Datebank ass gespaart",
'enterlockreason'      => "Gitt w.e.g. e Grond u firwat d'Datebak gespaart ass, a wéi laang dës Spär ongeféier bestoe soll.",
'readonlytext'         => 'Datebank ass elo fir all Ännerunge gespaart, wahrscheinlech wéinst Maintenance vun der Datebank, duerno ass erëm alles beim alen.

Den Administrateur huet dës Erklärung uginn: $1',
'missingarticle-rev'   => '(Versiounsnummer: $1)',
'missingarticle-diff'  => '(Ënnerscheed tësche Versiounen: $1, $2)',
'readonly_lag'         => "D'Datebank gouf automatesch gespaart fir datt d'Zweetserveren (slaves) nees mat dem Haaptserver (master) synchron geschalt kënne ginn.",
'internalerror'        => 'Interne Feeler',
'internalerror_info'   => 'Interne Feeler: $1',
'filecopyerror'        => 'De Fichier "$1" konnt net op "$2" kopéiert ginn.',
'filerenameerror'      => 'De Fichier "$1" konnt net op "$2" ëmbenannt ginn.',
'filedeleteerror'      => 'De Fichier "$1" konnt net geläscht ginn.',
'directorycreateerror' => 'De Repertoire "$1" konnt net geschafe ginn.',
'filenotfound'         => 'De Fichier "$1" gouf net fonnt.',
'fileexistserror'      => 'De Fichier "$1" konnt net geschriwwe ginn, wëll et dee Fichier scho gëtt.',
'unexpected'           => 'Onerwarte Wert: "$1"="$2".',
'formerror'            => 'Feeler: Dat wat Dir aginn hutt konnt net verschafft ginn.',
'badarticleerror'      => 'Dës Aktioun kann net op dëser Säit duerchgefouert ginn.',
'cannotdelete'         => "D'Bild oder d'Säit kann net geläscht ginn (ass waarscheinlech schonns vun engem Anere geläscht ginn).",
'badtitle'             => 'Schlechten Titel',
'badtitletext'         => 'De gewënschten Titel ass invalid, eidel, oder een net korrekten Interwiki Link.',
'perfdisabled'         => "'''Pardon!''' Dës Fonktioun gouf wéint Iwwerlaaschtung vum Server temporaire ausgeschalt.",
'perfcached'           => 'Dës Date kommen aus dem Cache a si méiglecherweis net aktuell:',
'perfcachedts'         => 'Dës Donneeë kommen aus dem Cache, leschten Update: $1',
'querypage-no-updates' => "D'Aktualiséierung vun dëser Säit ass zur Zäit ausgeschalt. D'Date gi bis op weideres net aktualiséiert.'''",
'wrong_wfQuery_params' => 'Falsche Parameter fir wfQuery()<br />
Funktioun: $1<br />
Ufro: $2',
'viewsource'           => 'Quelltext kucken',
'viewsourcefor'        => 'fir $1',
'actionthrottled'      => 'Dës Aktioun gouf gebremst',
'actionthrottledtext'  => 'Fir géint de Spam virzegoen, ass dës Aktioun esou programméiert datt dir se an enger kuerzer Zäit nëmmen limitéiert dacks maache kënnt. Dir hutt dës Limit iwwerschratt. Versicht et w.e.g. an e puer MInutten nach eng Kéier.',
'protectedpagetext'    => 'Dës Säit ass fir Ännerunge gespaart.',
'viewsourcetext'       => 'Dir kënnt de Quelltext vun dëser Säit kucken a kopéieren:',
'protectedinterface'   => 'Op dëser Säit fannt Dir Text fir de Sprooch-Interface vun der Software an dofir ass si gespaart fir Mëssbrauch ze verhenneren.',
'editinginterface'     => "'''Opgepasst:''' Dir sidd am Gaang, eng Säit z'änneren, déi do ass, fir Interface-Text fir d'Software ze liwweren. Ännerungen op dëser Säit änneren den Interface-Text, je no Kontext, op allen oder verschiddene Säiten, déi vun alle Benotzer gesi ginn. Fir d'Iwwersetzungen z'änneren onvitéiere mir iech de  [http://translatewiki.net/wiki/Main_Page?setlang=lb Projet Betawiki] vun den internationale Messagen ze benotzen.",
'sqlhidden'            => '(SQL-Offro verstoppt)',
'cascadeprotected'     => 'Dës Säit gouf fir Ännerunge gespaart, well se duerch Cascadeprotectioun vun {{PLURAL:$1|dëser Säit|dëse Säite}} protegéiert ass mat der Cascadenoptioun:
$2',
'namespaceprotected'   => "Dir hutt net déi néideg Rechter fir d'Säiten am Nummraum '''$1''' ze änneren.",
'customcssjsprotected' => 'Dir hutt net déi néideg Rechter fir dës Säit ze änneren, wëll si zu de perséinlechen Astellungen vun engem anere Benotzer gehéiert.',
'ns-specialprotected'  => 'Spezialsäite kënnen net verännert ginn.',
'titleprotected'       => "Eng Säit mat dësem Numm kann net ugeluecht ginn. Dës Spär gouf vum [[User:$1|$1]] gemaach deen als Grond ''$2'' uginn huet.",

# Virus scanner
'virus-badscanner'     => 'Schlecht Configuratioun: onbekannte  Virescanner: <i>$1</i>',
'virus-scanfailed'     => 'De Scan huet net fonctionnéiert (Code $1)',
'virus-unknownscanner' => 'onbekannten Antivirus:',

# Login and logout pages
'logouttitle'                => 'Benotzer-Ofmeldung',
'logouttext'                 => '<strong>Dir sidd elo ofgemeld.</strong>

Dir kënnt {{SITENAME}} elo anonym benotzen, oder Iech [[Special:UserLogin|nacheemol umellen]] als deeselwechten oder en anere Benotzer.
Opgepasst: Op verschiddene Säite gesäit et nach esou aus, wéi wann Dir nach ugemeld wiert, bis Dir ärem Browser seng Cache eidelmaacht.',
'welcomecreation'            => '== Wëllkomm, $1! ==
Äre Kont gouf kreéiert. 
Denkt drun, Är [[Special:Preferences|{{SITENAME}}-Astellungen]] unzepassen.',
'loginpagetitle'             => 'Benotzer-Umeldung',
'yourname'                   => 'Benotzernumm:',
'yourpassword'               => 'Passwuert:',
'yourpasswordagain'          => 'Passwuert widderhuelen:',
'remembermypassword'         => 'Meng Umeldung op dësem Computer verhalen',
'yourdomainname'             => 'Ären Domain',
'externaldberror'            => 'Entweder ass e Feeler bäi der externer Authentifizéierung geschitt, oder Dir däerft ären externe Benotzerkont net aktualiséieren.',
'loginproblem'               => "'''Et gouf e Problem bäi ärer Umeldung.'''<br />

Probéiert et w.e.g. nach eng Kéier!",
'login'                      => 'Umellen',
'nav-login-createaccount'    => 'Aloggen',
'loginprompt'                => "Fir sech op {{SITENAME}} umellen ze kënnen, mussen d'Cookien aktivéiert sinn.",
'userlogin'                  => 'Aloggen',
'logout'                     => 'Ofmellen',
'userlogout'                 => 'Ausloggen',
'notloggedin'                => 'Net ageloggt',
'nologin'                    => 'Hutt Dir kee Benotzerkont? $1.',
'nologinlink'                => 'Neie Benotzerkonto maachen',
'createaccount'              => 'Neie Kont opmaachen',
'gotaccount'                 => 'Dier hutt schonn e Kont? $1.',
'gotaccountlink'             => 'Umellen',
'createaccountmail'          => 'Via E-Mail',
'badretype'                  => 'Är Passwierder stëmmen net iwwerdeneen.',
'userexists'                 => 'Dëse Benotzernumm gëtt scho benotzt.
Sicht iech een anere Benotzernumm.',
'youremail'                  => 'E-Mailadress:',
'username'                   => 'Benotzernumm:',
'uid'                        => 'Benotzer ID:',
'prefs-memberingroups'       => 'Member vun {{PLURAL:$1|der Benotzergrupp|de Benotzergruppen}}:',
'yourrealname'               => 'Richtege Numm:',
'yourlanguage'               => 'Sprooch:',
'yournick'                   => 'Ënnerschrëft:',
'badsig'                     => "D'Syntax vun ärer Ënnerschëft ass net korrekt; iwwerpréift w.e.g. ären HTML Code.",
'badsiglength'               => "D'Ënnerschrëft ass ze laang.
Si muss manner wéi $1 {{PLURAL:$1|Zeechen|Zeechen}} hunn.",
'email'                      => 'E-Mail',
'prefs-help-realname'        => 'Äre richtege Numm ass fakultativ. Wann Dir en ugitt gëtt e benotzt fir iech är Kontributiounen zouzeuerdnen.',
'loginerror'                 => 'Feeler bäi der Umeldung',
'prefs-help-email'           => "D'E-Mailadress ass fakultativ, awer si erméiglecht et Iech ärt Passwuert ze mailen wann Dir et vergiess hutt.
Dir kënnt et och zouloossen datt aner Benotzer iech - iwwert e Link op ärer Benotzersäit oder ärer diskussiounssäit - kontaktéiere kënnen ouni datt Dir är Identitéit präisgitt.",
'prefs-help-email-required'  => 'Eng gülteg E-Mailadress gëtt heifir gebraucht.',
'nocookiesnew'               => "De Benotzerkont gouf ugeluecht, Awer Dir sidd net ageloggt. 
{{SITENAME}} brauch fir dës Funktioun Cookien.
Dir hutt d'Cookien desaktivéiert.
Aktivéiert déi w.e.g. a loggt Iech da matt ärem neie Benotzernomm an dem respektive Passwort ein.",
'nocookieslogin'             => "{{SITENAME}} benotzt Cookiën beim Umelle vun de Benotzer. Dir hutt Cookiën ausgeschalt, w.e.g aktivéiert d'Cookiën a versicht et nach eng Kéier.",
'noname'                     => 'Dir hutt kee gültege Benotzernumm uginn.',
'loginsuccesstitle'          => 'Umeldung huet geklappt',
'loginsuccess'               => "'''Dir sidd elo als \"\$1\" op {{SITENAME}} ugemellt.'''",
'nosuchuser'                 => 'De Benotzernumm "$1" gëtt et net.
Kuckt w.e.g. op d\'Schreifweis richteg ass, oder [[Special:Userlogin/signup|meld iech als neie Benotzer un]].',
'nosuchusershort'            => 'De Benotzernumm "<nowiki>$1</nowiki>" gëtt et net. Kuckt w.e.g. op d\'Schreifweis richteg ass.',
'nouserspecified'            => 'Gitt w.e.g. e Benotzernumm un.',
'wrongpassword'              => 'Dir hutt e falscht (oder kee) Passwuert aginn. Probéiert w.e.g. nach eng Kéier.',
'wrongpasswordempty'         => "D'Passwuert dat Dir aginn huet war eidel. Probéiert w.e.g. nach eng Kéier.",
'passwordtooshort'           => 'Ärt Passwuert ass ongülteg oder ze kuerz: Et muss mindestens {{PLURAL:$1|1 Zeeche|$1 Zeeche}} laang sinn an et däerf net matt dem Benotzernumm identesch sinn.',
'mailmypassword'             => 'Neit Passwuert per E-Mail kréien',
'passwordremindertitle'      => 'Neit Passwuert fir ee {{SITENAME}}-Benotzerkonto',
'passwordremindertext'       => 'Iergend een (waarscheinlech Dir selwer, matt der IP-Adress $1) huet een neit Passwuert fir {{SITENAME}} ($4) gefrot. Een temporärt Passwuert fir de Benotzer $2 gouf ugeluecht an et ass: $3. Wann et dëst ass wat Dir wollt, da sollt Dir iech elo aloggen an en neit Passwuert eraussichen.

Wann een aneren dës Ufro sollt gemaach hunn oder wann Dir iech an der Zwëschenzäit nees un ärt Passwuert erënnere kënnt an Dir ärt Passwuert net ännere wëllt da kënnt dir weider ärt aalt Passwuert benotzen.',
'noemail'                    => 'De Benotzer "$1" huet keng E-Mailadress uginn.',
'passwordsent'               => 'Een neit Passwuert gouf un déi fir de Benotzer "$1" gespäichert E-Mailadress geschéckt.
Melt iech w.e.g. domatt un, soubal Dir et kritt hutt.',
'blocked-mailpassword'       => "Déi vun iech benotzten IP-Adress ass fir d'Ännere vu Säite gespaart. Fir Mëssbrauch ze verhënneren, gouf d'Méiglechkeet fir een neit Passwuert unzefroen och gespaart.",
'eauthentsent'               => "Eng Confirmatiouns-E-Mail gouf un déi uginnen Adress geschéckt.<br/ >
Ier iergend eng E-Mail vun anere Benotzer op dee Kont geschéckt ka ginn, muss der als éischt d'Instructiounen an der Confirmatiouns-E-Mail befollegen, fir ze bestätegen datt de Kont wierklech ären eegenen ass.",
'throttled-mailpassword'     => "An {{PLURAL:$1|der läschter Stonn|de läschte(n) $1 Stonnen}} gouf eng Erënenrung un d'Passwuert verschéckt.
Fir de Mëssbrauch vun dëser Funktioun ze verhënneren kann nëmmen all {{PLURAL:$1|Stonn|$1 Stonnen}} esou eng Erënnerung verschéckt ginn.",
'mailerror'                  => 'Feeler beim Schécke vun der E-Mail: $1',
'acct_creation_throttle_hit' => 'Dir hutt scho(nn) $1 Konten. Dir kënnt keen Neie méi derbäikréien.',
'emailauthenticated'         => 'Är E-Mailadress gouf bestätegt: $1..',
'emailnotauthenticated'      => 'Är E-Mail Adress gouf <strong>nach net confirméiert</strong>.<br/ >
Dowéinst ass et bis ewell net méiglech, fir déi folgend Funktiounen E-Mailen ze schécken oder ze kréien.',
'noemailprefs'               => 'Gitt eng E-Mailadress un, fir datt déie folgend Funktiounen fonctionéieren.',
'emailconfirmlink'           => 'Confirméiert är E-Mailadress w.e.g..',
'invalidemailaddress'        => 'Dës E-Mailadress gëtt net akzeptéiert well se en ongëltegt Format (z.B. ongëlteg Zeechen) ze hu schéngt.
Gitt eng valabel E-Mailadress an oder loosst dëst Feld eidel.',
'accountcreated'             => 'De Kont gouf geschaf',
'accountcreatedtext'         => 'De Benotzerkont fir $1 gouf geschaf.',
'createaccount-title'        => 'Opmaache vun engem Benotzerkont op {{SITENAME}}',
'createaccount-text'         => 'Et gouf e Benotzerkont "$2" fir iech op {{SITENAME}} ($4) ugeluecht mat dem Passwuert "$3".
Dir sollt iech aloggen an ärt Passwuert elo änneren.

Falls dëse Benotzerkonto ongewollt ugeluecht ginn ass kënnt Dir dës Noriicht einfach ignoréieren.',
'loginlanguagelabel'         => 'Sprooch: $1',

# Password reset dialog
'resetpass'               => 'Passwuert fir Benotzerkont zrécksetzen',
'resetpass_announce'      => 'Dir sidd mat engem temporären , per E-Mail geschéckte Code ageloggt.
Fir är Umeldung ofzeschléissen, musst Dir elo hei een neit Passwuert uginn:',
'resetpass_text'          => '<!-- Schreiwt ären Text heihin-->',
'resetpass_header'        => 'Passwuert zrécksetzen',
'resetpass_submit'        => 'Passwuert aginn an umellen',
'resetpass_success'       => 'Ärt Passwuert gouf geännert. Logged iech elo an ...',
'resetpass_bad_temporary' => 'Ongültegt temporairt Passwuert. 
Dir hutt ärt Passwuert scho geännert oder een  neit temporairt Passwuert ugefrot.',
'resetpass_forbidden'     => 'Passwierder kënnen net geännert ginn.',
'resetpass_missing'       => 'Eidelt Formular',

# Edit page toolbar
'bold_sample'     => 'Fettgedréckten Text',
'bold_tip'        => 'Fettgedréckten Text',
'italic_sample'   => 'Kursiven Text',
'italic_tip'      => 'Kursiven Text',
'link_sample'     => 'Link-Text',
'link_tip'        => 'Interne Link',
'extlink_sample'  => 'http://www.example.com Link Titel',
'extlink_tip'     => 'Externe Link (Vergiesst net den http:// Prefix)',
'headline_sample' => 'Titel Text',
'headline_tip'    => 'Iwwerschrëft vum Niveau 2',
'math_sample'     => 'Formel hei asetzen',
'math_tip'        => 'Mathematesch Formel (LaTeX)',
'nowiki_sample'   => 'Unformatéierten Text hei afügen',
'nowiki_tip'      => 'Unformatéierten Text',
'image_sample'    => 'Beispill.jpg',
'image_tip'       => 'Bildlink',
'media_sample'    => 'Beispill.ogg',
'media_tip'       => 'Link op e Medieichier',
'sig_tip'         => 'Är Ënnerschrëft matt Zäitstempel',
'hr_tip'          => 'Horizontal Linn (mat Moosse gebrauchen)',

# Edit pages
'summary'                          => 'Resumé',
'subject'                          => 'Sujet/Iwwerschrëft',
'minoredit'                        => 'Kleng Ännerung',
'watchthis'                        => 'Dës Säit iwwerwaachen',
'savearticle'                      => 'Säit späicheren',
'preview'                          => 'Kucken ouni ofzespäicheren',
'showpreview'                      => 'Kucken ouni ofzespäicheren',
'showlivepreview'                  => 'Live-Kucken ouni ofzespäicheren',
'showdiff'                         => 'Weis Ännerungen',
'anoneditwarning'                  => 'Dir sidd net ageloggt. Dowéinst gëtt amplaz vun engem Benotzernumm är IP Adress am Historique vun dëser Säit gespäichert.',
'missingsummary'                   => "'''Erënnerung:''' Dir hutt kee Resumé aginn. Wann Dir nachemol op \"Säit ofspäicheren\" klickt, gëtt är Ännerung ouni Resumé ofgespäichert.",
'missingcommenttext'               => 'Gitt w.e.g. eng Bemierkung an.',
'missingcommentheader'             => "'''OPGEPASST:''' Dir hutt keen Titel/Sujet fir dës Bemierkung aginn. Wann Dir nach en Kéier op \"Späicheren\" klickt da gëtt àr Ännerung ouni Titel ofgespäichert.",
'summary-preview'                  => 'Resumé kucken ouni ofzespäicheren',
'subject-preview'                  => 'Sujet/Iwwerschrëft kucken',
'blockedtitle'                     => 'Benotzer ass gespaart',
'blockedtext'                      => "<big>Äre Benotzernumm oder är IP Adress gouf gespaart.</big>

Dëse Spär gouf vum \$1 gemaach. Als Grond gouf ''\$2'' uginn.

* Ufank vun der Spär: \$8
* Ënn vun der Spär: \$6
* Spär betrefft: \$7

Dir kënnt den/d' \$1 kontaktéieren oder ee vun deenen aneren [[{{MediaWiki:Grouppage-sysop}}|Administrateuren]] fir d'Spär ze beschwetzen.

Dëst sollt Der besonnesch maachen, wann Der d'Gefill hutt, dass de Grond fir d'Spären net bei Iech läit.
D'Ursaach dofir ass an deem Fall, datt der eng dynamesch IP hutt, iwwert en Access-Provider, iwwert deen och aner Leit fueren.
Aus deem Grond ass et recommandéiert, sech e Benotzernumm zouzeleeën, fir all Mëssverständnes z'évitéieren. 

Dir kënnt d'Funktioun \"Dësem Benotzer eng E-Mail schécken\" nëmme benotzen, wann Dir eng gülteg E-Mail Adress bei äre [[Special:Preferences|Astellungen]] aginn hutt.
Är aktuell-IP Adress ass \$3 an d'Nummer vun der Spär ass #\$5. 
Schreift all dës Informatioune w.e.g. bei all Ufro derbäi.",
'autoblockedtext'                  => 'Är IP-Adress gouf automatesch gespaart, well se vun engem anere Benotzer gebraucht gouf, an dëse vum $1 gespaart ginn ass.
De Grond dofir war:

:\'\'$2\'\'

* Ufank vun der Spär: $8
* Dauer vun der Spär: $6
* D\'Spär leeft of: $7

Dir kënnt de(n) $1 oder soss een [[{{MediaWiki:Grouppage-sysop}}|Administrateur]] kontaktéieren, fir iwwer dës Spär ze diskutéieren.

Bedenkt datt Dir d\'Fonctioun "Dësem Benotzer eng E-Mail schécken" benotze kënnt wann Dir eng gülteg E-Mailadress an ären [[Special:Preferences|Astellungen]] uginn hutt a wann dat net fir iech gespaart gouf.

Är aktuell IP-Adress ass $3 an d\'Nummer vun ärer Spär ass $5. 
Gitt dës Donnéeë w.e.g bei allen Ufroen zu dëser Spär un.',
'blockednoreason'                  => 'Kee Grond uginn',
'blockedoriginalsource'            => "De Quelltext vun '''$1''' steet hei ënnendrënner:",
'blockededitsource'                => "Den Text vun '''ären Ännerungen''' op '''$1''' steet hei ënnendrënner:",
'whitelistedittitle'               => "Login noutwännesch fir z'änneren",
'whitelistedittext'                => 'Dir musst iech $1, fir Säiten änneren ze kënnen.',
'confirmedittitle'                 => "Konfirmatioun vun ärer E-Mailadress ass erfuederlech fir z'änneren.",
'confirmedittext'                  => 'Dir musst är E-Mail-Adress conirméieren, ier Dir ännerunge maache kënnt.
Gitt w.e.g. Eng E-Mailadrss a validéiert se op äre [[Special:Preferences|Benotzerastellungen]].',
'nosuchsectiontitle'               => 'Et gëtt keen Abschnitt mat dem Numm',
'nosuchsectiontext'                => "Dir hutt versicht een Abschnitt z'änneren den et net gëtt. Well et den Abschnitt $1 net gëtt, gëtt et keng Plaz fir är Ännerung ze späicheren.",
'loginreqtitle'                    => 'Umeldung néideg',
'loginreqlink'                     => 'umellen',
'loginreqpagetext'                 => 'Dir musst iech $1, fir aner Säite liesen zu kënnen.',
'accmailtitle'                     => 'Passwuert gouf geschéckt.',
'accmailtext'                      => "D'Passwuert fir „$1“ gouf op $2 geschéckt.",
'newarticle'                       => '(Nei)',
'newarticletext'                   => "Dir hutt op e Link vun enger Säit geklickt, déi et nach net gëtt. Fir dës Säit unzeleeën, gitt w.e.g. ären Text an déi Këscht hei ënnendrënner an (kuckt d'[[{{MediaWiki:Helppage}}|Hëllef Säit]] fir méi Informatiounen). Wann Dir duerch een Iertum heihi komm sidd, da klickt einfach op de Knäppchen '''Zréck''' vun ärem Browser.",
'anontalkpagetext'                 => "---- ''Dëst ass d'Diskussiounssäit fir en anonyme Benotzer deen nach kee Kont opgemaach huet oder en net benotzt.
Dowéinster musse mir d'IP Adress benotzen fir hien/hatt z'identifizéieren. 
Sou eng IP Adress ka vun e puer Benotzer gedeelt ginn. 
Wann Dir en anonyme Benotzer sidd an dir irrelevant Kommentäre krut,  [[Special:UserLogin|maacht w.e.g. e Kont op]] oder [[Special:UserLogin|loggt Iech an]] fir weider Verwiesselungen mat anonyme Benotzer ze verhënneren.''",
'noarticletext'                    => 'Dës Säit huet momentan nach keen Text, Dir kënnt op anere Säiten no [[Special:Search/{{PAGENAME}}|dësem Säitentitel sichen]] oder [{{fullurl:{{FULLPAGENAME}}|action=edit}} esou eng Säit uleeën].',
'userpage-userdoesnotexist'        => 'De Benotzerkont "$1" gëtt et net. Iwwerpréift w.e.g. op Dir dës Säit erschafe/ännere wëllt.',
'clearyourcache'                   => "'''Opgepasst - Nom Späichere muss der Ärem Browser seng Cache eidel maachen, fir d'Ännerungen ze gesinn.''' '''Mozilla / Firefox / Safari: ''' dréckt op ''Shift'' während Dir ''reload'' klickt oder dréckt ''Ctrl-F5'' oder ''Ctrl-R''(''Command-R'' op engem Macintosh);'''Konqueror: ''' klickt  ''Reload'' oder dréckt ''F5'' '''Opera:''' maacht de Cache eidel an ''Tools → Preferences;'' '''Internet Explorer:''' dréckt ''Ctrl'' während Dir op ''Refresh'' klickt oder dréckt ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Tipp:</strong> Benotzt de ''Kucken ouni ze späichere''-Button, fir äre neien CSS/JS virum Späicheren ze testen.",
'usercsspreview'                   => "'''Bedenkt: Dir kuckt just är Benotzer CSS.
Si gouf nach net gepäichert!'''",
'userjspreview'                    => "'''Denkt drun datt Dir äre Javascript nëmmen test, nach ass näischt gespäichert!'''",
'updated'                          => '(Geännert)',
'note'                             => '<strong>Notiz:</strong>',
'previewnote'                      => "<strong>Dëst ass nëmmen e Preview; D'Ännerunge sinn nach net gespäichert!</strong>",
'previewconflict'                  => 'Dir gesitt an dem ieweschten Textfeld wéi den Text ausgesi wäert, wann Dir späichert.',
'session_fail_preview'             => "<strong>Är Ännerung konnt net gespäichert gi well d'Date vun ärer Sessioun verluergaange sinn.
Versicht et w.e.g. nach eng Kéier.
Wann de Problem dann ëmmer nach bestoe sollt, da versicht iech [[Special:UserLogout|auszeloggen]] an dann erëm anzeloggen.</strong>",
'session_fail_preview_html'        => "<strong>Är Ännerung konnt net gespäichert gi well d'Date vun ärer Sessioun verluergaange sinn.</strong>

''Well op {{SITENAME}} ''raw HTML'' aktivéiert ass, gouf de Preview ausgeblendt fir JavaScript-Attacken ze vermeiden.''

<strong>Wann dir eng berechtigt Ännerung maache wëllt, da versicht et w.e.g. nach eng Kéier. 
Wann de Problem dann ëmmer nach bestoe sollt, versicht iech [[Special:UserLogout|auszeloggen]] an dann erëm anzeloggen.</strong>",
'editing'                          => 'Ännere vun $1',
'editingsection'                   => 'Ännere vun $1 (Abschnitt)',
'editingcomment'                   => 'Ännere vun $1 (Bemierkung)',
'editconflict'                     => 'Ännerungskonflikt: $1',
'explainconflict'                  => "Een anere Benotzer huet un dëser Säit geschafft, während Dir amgaange waart, se ze änneren.
Dat iewegt Textfeld weist Iech den aktuellen Text.
Är Ännerunge gesitt Dir am ënneschten Textfeld.
Dir musst Är Ännerungen an dat iewegt Textfeld androen.
'''Nëmmen''' den Text aus dem iewegten Textfeld gëtt gehale wann Dir op \"Säit späicheren\" klickt.",
'yourtext'                         => 'Ären Text',
'storedversion'                    => 'Gespäichert Versioun',
'nonunicodebrowser'                => '<strong>OPGEPASST:</strong> Äre Browser ass net Unicode kompatibel. Ännert dat w.e.g. éier Dir eng Säit ännert.',
'editingold'                       => '<strong>OPGEPASST: Dir ännert eng al Versioun vun dëser Säit. Wann Dir späichert, sinn all rezent Versioune vun dëser Säit verluer.</strong>',
'yourdiff'                         => 'Ënnerscheeder',
'copyrightwarning'                 => 'W.e.g. notéiert datt all Kontributiounen op {{SITENAME}} automatesch ënner der $2 (kuckt $1 fir méi Informatiounen) verëffentlecht sinn.
Wann Dir net wëllt datt är Texter vun anere Mataarbechter verännert, geläscht a weiderverdeelt kënne ginn, da setzt näischt heihinner.<br />
Dir verspriecht ausserdeem datt dir dësen Text selwer verfaasst hutt, oder aus dem Domaine public oder ähnleche Ressource kopéiert hutt.
<strong>DROT KEE COPYRECHTLECH GESCHÜTZTE CONTENU OUNI ERLAABNISS AN!</strong>',
'copyrightwarning2'                => 'W.e.g. notéiert datt all Kontributiounen op {{SITENAME}} vun anere Benotzer verännert oder geläscht kënne ginn. Wann dir dat net wëllt, da setzt näischt heihinner.<br />
Dir verspriecht ausserdeem datt dir dësen Text selwer verfaasst hutt, oder aus dem Domaine public oder anere fräie Quelle kopéiert hutt. (cf. $1 fir méi Detailler). <strong>DROT KEE COPYRECHTLECH GESCHÜTZTE CONTENU AN!</strong>',
'longpagewarning'                  => '<strong>WARNUNG: Dës Säit ass $1 kB grouss; verschidde Browser kéinte Problemer hunn, Säiten ze verschaffen, déi méi grouss wéi 32 KB sinn.

Iwwerleet w.e.g., ob eng Opdeelung vun der Säit a méi kleng Abschnitter méiglich ass.</strong>',
'longpageerror'                    => '<strong>FEELER: Den Text, den Dir Versicht ze späicheren, huet $1 KB. Dëst ass méi wéi den erlaabte Maximum vun $2 KB – dofir kann den Text net gespäichert ginn.</strong>',
'readonlywarning'                  => "<strong>OPGEPASST: D'Datebank gouf wéinst Maintenanceaarbechte fir Säitenànnerunge gespaart, dofir kënnt Dir déi Säit den Ament net ofspäicheren. Versuergt den Text a versicht d'Ännerunge méi spéit nach emol ze maachen.</strong>",
'protectedpagewarning'             => '<strong>OPGEPASST: Dës Säit gouf gespaart a kann nëmme vun engem Administrateur geännert ginn.</strong>',
'semiprotectedpagewarning'         => "'''Bemierkung:''' Dës Säit gouf esou gespaart, datt nëmme ugemellte Benotzer s'ännere kënnen.",
'cascadeprotectedwarning'          => "'''Passt op:''' Dës Säit gouf gespaart a kann nëmme vu Benotzer mat Administreursrechter geännert ginn. Si ass an dës {{PLURAL:$1|Säit|Säiten}} agebonnen, déi duerch Kaskadespäroptioun protegéiert {{PLURAL:$1|ass|sinn}}:'''",
'titleprotectedwarning'            => '<strong>OPGEPASST: Dës Säit gouf gespaart sou datt nëmme verschidde Benotzer se uleeë kënnen.</strong>',
'templatesused'                    => 'Schablounen déi op dëser Säit am Gebrauch sinn:',
'templatesusedpreview'             => 'Schablounen déi an dësem Preview am Gebrauch sinn:',
'templatesusedsection'             => 'Schablounen déi an dësem Abschnitt am Gebrauch sinn:',
'template-protected'               => '(protegéiert)',
'template-semiprotected'           => '(gespaart fir net-ugemellten an nei Benotzer)',
'hiddencategories'                 => 'Dës Säit gehéiert zu {{PLURAL:$1|1 verstoppter Kategorie|$1 verstoppte Kategorien}}:',
'edittools'                        => '<!-- Dësen Text gëtt ënnert dem "Ännere"-Formulair esouwéi dem "Eropluede"-Formulair ugewisen. -->',
'nocreatetitle'                    => "D'Uleeë vun neie Säiten ass limitéiert.",
'nocreatetext'                     => "Op {{SITENAME}} gouf d'Schafe vun neie Säite limitéiert. Dir kënnt Säiten déi scho bestinn änneren oder Iech [[Special:UserLogin|umellen]].",
'nocreate-loggedin'                => 'Dir hutt keng Berechtigung fir nei Säiten unzeleeën.',
'permissionserrors'                => 'Berechtigungs-Feeler',
'permissionserrorstext'            => 'Dir hutt net genuch Rechter fir déi Aktioun auszeféieren. {{PLURAL:$1|Grond|Grënn}}:',
'permissionserrorstext-withaction' => "Dir sidd net berechtegt, d'Aktioun $2 auszeféieren, wéinst {{PLURAL:$1|dësem Grond|dëse Grënn}}:",
'recreate-deleted-warn'            => "'''Opgepasst: Dës Säit gouf schonns eng Kéier geläscht.'''

Frot iech ob et wierklech sënnvoll ass dës Säit nees nei ze schafen.
Fir iech z'informéieren fannt Dir hei d'Läschlescht mat dem Grond:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Opgepasst: Dës Säit huet zevill Ufroe vu komplexe Parserfunktiounen.
 
Et däerfen net méi wéi $2 ufroe sinn, aktuell sinn et $1 Ufroen.',
'expensive-parserfunction-category'       => 'Säiten, déi komplex Parserfunktiounen ze dacks opruffen',
'post-expand-template-inclusion-warning'  => "Opgepasst: D'Gréisst vun den agebonnene Schablounen ass ze grouss, e puer Schabloune kënnen net agebonne ginn.",
'post-expand-template-inclusion-category' => "Säiten, op denen d'maximal Gréist vun agebonnene Schablounen iwwerschratt ass",
'post-expand-template-argument-category'  => 'Säiten, op dene mindestens e Parameter vun enger Schabloun vergiess ginn ass',

# "Undo" feature
'undo-success' => "D'Ännerung gëtt réckgängeg gemaach. Iwwerpréift w.e.g. de Verglach ënnedrënner fir nozekuckeen ob et esou richteg ass, duerno späichert w.e.g d'Ännerungen of fir dës Aktioun ofzeschléissen.",
'undo-failure' => '<span class="error">D\'Ännerung konnt net réckgängeg gemaach ginn, wëll de betraffenen Abschnitt an der Tëschenzäit geännert gouf.</span>',
'undo-norev'   => "D'Ännerung kann net zréckgesat ginn, well et se net gëtt oder well se scho geläscht ass.",
'undo-summary' => 'Ännerung $1 vu(n) [[Special:Contributions/$2|$2]] ([[User talk:$2|Diskussioun]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]]) annulléieren.',

# Account creation failure
'cantcreateaccounttitle' => 'Benotzerkont konnt net opgemaach ginn',
'cantcreateaccount-text' => 'Dës IP Adress (\'\'\'$1\'\'\') gouf vum [[User:$3|$3]] blokéiert fir Benotzer-Konten op der lëtzebuergescher Wikipedia opzemaachen. De Benotzer $3 huet "$2" als Ursaach uginn.',

# History pages
'viewpagelogs'        => 'Logbicher fir dës Säit weisen',
'nohistory'           => 'Et gëtt keng al Versioune vun dëser Säit.',
'revnotfound'         => 'Dës Versioun gouf net fonnt.',
'revnotfoundtext'     => "Déi Versioun vun der Säit déi Dir gefrot hutt konnt net fonnt ginn. Kuckt d'URL no, déi Dir benotzt hutt fir op dës Säit ze kommen.",
'currentrev'          => 'Aktuell Versioun',
'revisionasof'        => 'Versioun vum $1',
'revision-info'       => 'Versioun vum $1 vum $2.',
'previousrevision'    => '← Méi al Versioun',
'nextrevision'        => 'Méi rezent Ännerung→',
'currentrevisionlink' => 'Aktuell Versioun',
'cur'                 => 'aktuell',
'next'                => 'nächst',
'last'                => 'lescht',
'page_first'          => 'éischt',
'page_last'           => 'Enn',
'histlegend'          => "Fir d'Ännerungen unzeweisen: Klickt déi zwou Versiounen un, déi solle verglach ginn.<br />
*(aktuell) = Ënnerscheed mat der aktueller Versioun,
*(lescht) = Ënnerscheed mat der aler Versioun,
*k = Kleng Ännerung.",
'deletedrev'          => '[geläscht]',
'histfirst'           => 'Eelsten',
'histlast'            => 'Neitsten',
'historysize'         => '({{PLURAL:$1|1 Byte|$1 Byten}})',
'historyempty'        => '(eidel)',

# Revision feed
'history-feed-title'          => 'Historique vun de Versiounen',
'history-feed-description'    => 'Versiounshistorique fir dës Säit op {{SITENAME}}',
'history-feed-item-nocomment' => '$1 ëm $2', # user at time
'history-feed-empty'          => 'Déi ugefrote Säit gëtt et net.
Vläicht gouf se geläscht oder geréckelt.
[[Special:Search|Sich op]] {{SITENAME}} no passenden neie Säiten.',

# Revision deletion
'rev-deleted-comment'         => '(Bemierkung geläscht)',
'rev-deleted-user'            => '(Benotzernumm ewechgeholl)',
'rev-deleted-event'           => '(Aktioun aus dem Logbuch erausgeholl)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks"> Dës Versioun gouf aus den ëffentlechen Archiven erausgeholl.
Dir fannt eventuell méi Informatiounen an der [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} Läschlëscht].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Dës Versioun gouf geläscht a kann net méi ëffentlech gewise ginn.
Als Administrateur op {{SITENAME}} kënnt Dir se weiderhi gesinn.
Prezisiounen iwwert d\'Läschen esou wéi de Grond fannt Dir am [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} Läsch-Logbuch].</div>',
'rev-delundel'                => 'weisen/verstoppen',
'revisiondelete'              => 'Versioune läschen/restauréieren',
'revdelete-nooldid-title'     => 'Ongülteg Zilversioun',
'revdelete-nooldid-text'      => 'Dir hutt entweder keng Versioun uginn fir dës Funktioun ze benotzen, déi Versioun déi Diruginn huet gëtt et net, oder dir versicht déi aktuell Versioun ze verstoppen.',
'revdelete-selected'          => "{{PLURAL:$2|Gewielte Versioun|Gewielte Versioune}} vu(n) '''$1''' :",
'logdelete-selected'          => 'Ausgewielten {{PLURAL:$1|Evenement|Evenementer}} aus dem Logbuch:',
'revdelete-text'              => 'Geläschte Versiounen oder aner geäschte Bestanddeeler sinn net méi ëffentlech zougänglech, si stinn awer weiderhinn an der Versiounsgeschicht vun der Säit.

{{SITENAME}}-Administrateure kënnen de geläschten Inhalt oder aner geläschte Bestanddeeler weiderhi gesinn a restauréieren, et sief, et gouf festgeluecht, datt déi Limitatioune vum Accès och fir Administrateure gëllen.',
'revdelete-legend'            => "Limitatioune fir d'Sichtbarkeet festleeën",
'revdelete-hide-text'         => 'Text vun der Versioun verstoppen',
'revdelete-hide-name'         => 'Logbuch-Aktioun verstoppen',
'revdelete-hide-comment'      => 'Bemierkung verstoppen',
'revdelete-hide-user'         => 'Dem Auteur säi Benotzernumm/IP verstoppen',
'revdelete-hide-restricted'   => 'Dës Limitatioune och fir Administrateuren uwenden an dëse Formulair spären',
'revdelete-suppress'          => 'Grond vum Läschen och fir Administrateure verstoppt',
'revdelete-hide-image'        => 'Bildinhalt verstoppen',
'revdelete-unsuppress'        => 'Limitatiounen fir restauréiert Versiounen ophiewen',
'revdelete-log'               => "Bemierkung (fir d'Logbicher/Lëschten):",
'revdelete-submit'            => 'Op déi gewielte Versioun uwenden',
'revdelete-logentry'          => 'Sichtbarkeet vun der Versioun gouf geännert fir [[$1]]',
'revdelete-success'           => "'''Sichtbarkeet vun de Versioune geännert.''''",
'revdel-restore'              => 'Sichtbarkeet änneren',
'pagehist'                    => 'Versioune vun dëser Säit',
'deletedhist'                 => 'Geläschte Versiounen',
'revdelete-content'           => 'Inhalt',
'revdelete-summary'           => 'Résumé änneren',
'revdelete-uname'             => 'Benotzernumm',
'revdelete-restricted'        => 'Limitatioune fir Administrateuren ageschalt',
'revdelete-unrestricted'      => 'Limitatioune fir Administrateuren opgehuewen',
'revdelete-hid'               => '$1 verstoppen',
'revdelete-unhid'             => '$1 weisen',
'revdelete-log-message'       => '$1 fir $2 {{PLURAL:$2|Versioun|Versiounen}}',

# Suppression log
'suppressionlog'     => 'Lëscht vun de verstoppten a geläschte Säiten',
'suppressionlogtext' => "Ënnendrënner ass eng Lëscht vun de geläschte Säiten a Spären déi fir d'Administrateuren net sichtbar sinn.
Kuckt [[Special:IPBlockList|Lëscht vun de gespaarten IPen]] fir déi aktuell Spären.",

# History merging
'mergehistory'                     => 'Historiquë fusionéieren',
'mergehistory-header'              => "Mat dëser Spezialsäit kënnt Dir d'Versiounsgeschicht vun enger Ursprungssäit mat der Versiounsgeschicht vun enger Zilsäit zesummeleeën.
Passt op, datt d'Versiounsgeschicht der Säit historesch korrekt ass.

'''Als Minimum muss déi aktuell Versioun vun der Ursprungssäit bestoe bleiwen.'''",
'mergehistory-box'                 => 'Historiquë vun zwou Säite fusionéieren',
'mergehistory-from'                => 'Originalsäit:',
'mergehistory-into'                => 'Zilsäit:',
'mergehistory-list'                => 'Versiounen, déi zesummegeluecht kënne ginn',
'mergehistory-go'                  => 'Weis déi Versiounen, déi zesummegeluecht kënne ginn',
'mergehistory-submit'              => 'Versioune verschmelzen',
'mergehistory-empty'               => 'Et kënne keng Versioune zesummegeluecht ginn.',
'mergehistory-success'             => '{{PLURAL:$3|1 Versioun gouf|$3 Versioune goufe}} vun [[:$1]] op [[:$2]] zesummegeluecht.',
'mergehistory-fail'                => "Versiounszesummeleeung war net méiglech, kuckt w.e.g. d'Säiten an d'Zäit-Parameter no.",
'mergehistory-no-source'           => 'Originalsäit "$1" gëtt et net.',
'mergehistory-no-destination'      => 'Zilsäit "$1" gëtt et net.',
'mergehistory-invalid-source'      => "D'Originalsäit muss ee gültege Säitennumm hunn.",
'mergehistory-invalid-destination' => 'Zilsäit muss e gültege Säitennumm sinn.',
'mergehistory-autocomment'         => '[[:$1]] zesummegeluecht an [[:$2]]',
'mergehistory-comment'             => '[[:$1]] zesummegeluecht an [[:$2]]: $3',

# Merge log
'mergelog'           => 'Fusiouns-Logbuch',
'pagemerge-logentry' => '[[$1]] zesummegeluecht an [[$2]] (Versioune bis $3)',
'revertmerge'        => 'Zesummeféieren ophiewen',
'mergelogpagetext'   => 'Lëscht vun de rezenten Zesummeféierungen vu Versiounsgeschichten.',

# Diffs
'history-title'           => 'Versiounshistorique vun „$1“',
'difference'              => '(Ennerscheed tëscht Versiounen)',
'lineno'                  => 'Linn $1:',
'compareselectedversions' => 'Ausgewielte Versioune vergläichen',
'editundo'                => 'zréck',
'diff-multi'              => '({{PLURAL:$1|Eng Tëscheversioun gëtt net|$1 Tëscheversioune ginn net}} gewisen)',

# Search results
'searchresults'             => 'Resultat vun der Sich',
'searchresulttext'          => "Fir méi Informatiounen iwwert d'Sichfunktiounen op {{SITENAME}}, kuckt w.e.g op [[{{MediaWiki:Helppage}}|{{int:help}}]].",
'searchsubtitle'            => 'Dir hutt no "[[:$1]]" gesicht ([[Special:Prefixindex/$1|all Säiten déi mat "$1" ufänken]] | [[Special:WhatLinksHere/$1|all Sàiten déi op "$1" linken]])',
'searchsubtitleinvalid'     => 'Dir hutt no "$1" gesicht.',
'noexactmatch'              => "'''Et gëtt keng Säite mam Titel \"\$1\".'''

Dir kënnt [[:\$1|déi Säit uleeën]].",
'noexactmatch-nocreate'     => "'''Et gëtt keng Säit mam Titel \"\$1\".'''",
'toomanymatches'            => 'Zevill Resultater goufe fonnt, versicht w.e.g. eng aner Ufro',
'titlematches'              => 'Säitentitel Iwwerdeneestëmmungen',
'notitlematches'            => 'Keng Iwwereneestëmmungen mat Säitentitelen',
'textmatches'               => 'Säitentext Iwwerdeneestëmmungen',
'notextmatches'             => 'Keng Iwwereneestëmmungen',
'prevn'                     => 'vireg $1',
'nextn'                     => 'nächst $1',
'viewprevnext'              => 'Weis ($1) ($2) ($3)',
'search-result-size'        => '$1 ({{PLURAL:$2|1 Wuert|$2 Wierder}})',
'search-result-score'       => 'Relevanz: $1 %',
'search-redirect'           => '(Viruleedung $1)',
'search-section'            => '(Abschnitt $1)',
'search-suggest'            => 'Méngt Dir: $1',
'search-interwiki-caption'  => 'Schwesterprojeten',
'search-interwiki-default'  => '$1 Resultater:',
'search-interwiki-more'     => '(méi)',
'search-mwsuggest-enabled'  => 'matt Virschléi',
'search-mwsuggest-disabled' => 'keng Virschléi',
'search-relatedarticle'     => 'A Verbindung',
'mwsuggest-disable'         => 'Ajax-Virschléi ausschalten',
'searchrelated'             => 'a Verbindng',
'searchall'                 => 'all',
'showingresults'            => "Hei gesitt der  {{PLURAL:$1| '''1''' Resultat|'''$1''' Resultater}}, ugefaang mat #'''$2'''.",
'showingresultsnum'         => "Hei gesitt der  {{PLURAL:$3|'''1''' Resultat|'''$3''' Resultater}}, ugefaange mat #'''$2'''.",
'showingresultstotal'       => "Weis ënnendrënner d'{{PLURAL:$3|Resultat|Resultater}} '''$1 - $2''' vu(n) '''$3'''",
'nonefound'                 => "'''Opgepasst''': Nëmmen e puer Nimmraim gi ''par default'' duerchsicht. Versicht an ärer Ufro ''all:'' anzestellen fir de dsamte contenu (inklusiv Diskussiounssäiten, Schablonen, ...), oder benotzed déi gwënscht Nimmräim als Virastellung.",
'powersearch'               => 'Erweidert Sich',
'powersearch-legend'        => 'Erweidert Sich',
'powersearch-ns'            => 'Sich an den Nimmraim:',
'powersearch-redir'         => 'Viruleedungen weisen',
'powersearch-field'         => 'Sich no:',
'search-external'           => 'Extern Sich',
'searchdisabled'            => "D'Sichfunktioun op {{SITENAME}} ass ausgeschalt. Dir kënnt iwwerdeems mat H!ellef vu Google sichen. Bedenkt awer, datt deenen hire  Sichindex fir {{SITENAME}} eventuell net dem aktuellste Stand entsprecht.",

# Preferences page
'preferences'              => 'Astellungen',
'mypreferences'            => 'Meng Astellungen',
'prefs-edits'              => 'Zuel vun den Ännerungen:',
'prefsnologin'             => 'Net ageloggt',
'prefsnologintext'         => 'Dir musst <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}}agelogged]</span> sinn, fir är Astellungen änneren ze kënnen.',
'prefsreset'               => "D'Astellungen goufen zréckgesat esou wéi se ofgespäichert waren.",
'qbsettings'               => 'Geschirläischt',
'qbsettings-none'          => 'Keen',
'qbsettings-fixedleft'     => 'Lénks, fest',
'qbsettings-fixedright'    => 'Riets, fest',
'qbsettings-floatingleft'  => 'schwiewt lenks',
'qbsettings-floatingright' => 'Schwiewt riets',
'changepassword'           => 'Passwuert änneren',
'skin'                     => 'Skin',
'dateformat'               => 'Datumsformat',
'datedefault'              => 'Egal (Standard)',
'datetime'                 => 'Datum an Auerzäit',
'math_failure'             => 'Parser-Feeler',
'math_unknown_error'       => 'Onbekannte Feeler',
'math_unknown_function'    => 'Onbekannte Funktioun',
'math_lexing_error'        => "'Lexing'-Feeler",
'math_syntax_error'        => 'Syntaxfeeler',
'math_image_error'         => "d'PNG-Konvertéierung huet net fonctionnéiert;
iwwerpréift déi korrekt Installatioun vu LaTeX, dvips, gs a convert",
'prefs-personal'           => 'Benotzerprofil',
'prefs-rc'                 => 'Rezent Ännerungen',
'prefs-watchlist'          => 'Iwwerwaachungslëscht',
'prefs-watchlist-days'     => 'Zuel vun den Deeg, déi an der Iwwerwaachungslëscht ugewise solle ginn:',
'prefs-watchlist-edits'    => 'Maximal Zuel vun den Ännerungen déi an der erweiderter Iwwerwaachungslëscht ugewise solle ginn:',
'prefs-misc'               => 'Verschiddenes',
'saveprefs'                => 'Späicheren',
'resetprefs'               => 'Net gespäichert Ännerungen zrécksetzen',
'oldpassword'              => 'Aalt Passwuert:',
'newpassword'              => 'Neit Passwuert:',
'retypenew'                => 'Neit Passwuert (nachemol):',
'textboxsize'              => 'Änneren',
'rows'                     => 'Zeilen',
'columns'                  => 'Kolonnen',
'searchresultshead'        => 'Sich',
'resultsperpage'           => 'Zuel vun de Resultater pro Säit:',
'contextlines'             => 'Zuel vun de Linnen:',
'contextchars'             => 'Kontextcharactère pro Linn:',
'stub-threshold'           => 'Maximum (a Byte) bei deem e Link nach ëmmer am <a href="#" class="stub">Skizze-Format</a> gewise gëtt:',
'recentchangesdays'        => 'Deeg déi an de Rezenten Ännerungen ugewise ginn:',
'recentchangescount'       => 'Zuel vun Den Ännerungen déi bei de rezenten Ännerungen de Versiounen an den Log-Säite gewise ginn:',
'savedprefs'               => 'Är Astellunge goufe gespäichert.',
'timezonelegend'           => 'Zäitzon',
'timezonetext'             => "¹Gitt d'Zuel vun de Stonnen an, déi tëscht ärer Zäitzon an der Serverzäit (UTC) leien .",
'localtime'                => 'Lokalzäit:',
'timezoneoffset'           => 'Ënnerscheed¹:',
'servertime'               => 'Serverzäit:',
'guesstimezone'            => 'Vum Browser iwwerhuelen',
'allowemail'               => 'E-Maile vun anere Benotzer kréien.',
'prefs-searchoptions'      => 'Sichoptiounen',
'prefs-namespaces'         => 'Nummraim',
'defaultns'                => 'Dës Nimmraim duerchsichen:',
'default'                  => 'Standard',
'files'                    => 'Fichieren',

# User rights
'userrights'                  => 'Benotzerrechterverwaltung', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Benotzergrupp verwalten',
'userrights-user-editname'    => 'Benotzernumm uginn:',
'editusergroup'               => 'Benotzergruppen änneren',
'editinguser'                 => "Ännere vun de Rechter vum Benotzer '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Benotzergruppen änneren',
'saveusergroups'              => 'Benotzergruppe späicheren',
'userrights-groupsmember'     => 'Member vun:',
'userrights-groups-help'      => "Dir kënnt d'Gruppen zu denen dëse Benutzer gehéiert änneren. 
* Een ugekräizten Haischen bedeit, datt de Benotzer Member vun dësem Grupp ass.
* Een ugekräizten Haischen bedeit, datt de Benotzer net Member vun dësem Grupp ass.
* E * bedeit datt Dir de Grupp net méi ewech huele kënnt wann e bis eemol derbäigesat ass oder gouf.",
'userrights-reason'           => 'Grond:',
'userrights-no-interwiki'     => "Dir hutt net déi néideg Rechter, fir d'Rechter vu Benoutzer op anere Wikien z'änneren.",
'userrights-nodatabase'       => "D'Datebank $1 gëtt et net oder se ass net lokal.",
'userrights-nologin'          => 'Dir musst mat engem Administrateurs-Benutzerkont [[Special:UserLogin|agelogged sinn]], fir Benotzerrechter änneren ze kënnen.',
'userrights-notallowed'       => "Dir hutt net déi néideg Rechter fir d'Rechter vun anere Benotzer z'änneren.",
'userrights-changeable-col'   => 'Gruppen déi Dir ännere kënnt',
'userrights-unchangeable-col' => 'Gruppen déi Dir net ännere kënnt',

# Groups
'group'               => 'Grupp:',
'group-user'          => 'Benotzer',
'group-autoconfirmed' => 'Registréiert Benotzer',
'group-bot'           => 'Botten',
'group-sysop'         => 'Administrateuren',
'group-bureaucrat'    => 'Bürokraten',
'group-suppress'      => 'Iwwersicht',
'group-all'           => '(all)',

'group-user-member'          => 'Benotzer',
'group-autoconfirmed-member' => 'Registréierte Benotzer',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Administrateur',
'group-bureaucrat-member'    => 'Bürokrat',
'group-suppress-member'      => 'Iwwersiicht',

'grouppage-user'          => '{{ns:project}}:Benotzer',
'grouppage-autoconfirmed' => '{{ns:project}}:Registréiert Benotzer',
'grouppage-bot'           => '{{ns:project}}:Botten',
'grouppage-sysop'         => '{{ns:project}}:Administrateuren',
'grouppage-bureaucrat'    => '{{ns:project}}:Bürokraten',
'grouppage-suppress'      => '{{ns:project}}:Iwwersiicht',

# Rights
'right-read'                 => 'Säite liesen',
'right-edit'                 => 'Säiten änneren',
'right-createpage'           => 'Säiten uleeën (déi keng Diskussiounssäite sinn)',
'right-createtalk'           => 'Diskussiounssäiten uleeën',
'right-createaccount'        => 'Nei Benotzerkonten uleeën',
'right-minoredit'            => 'Ännerungen als kleng markéieren',
'right-move'                 => 'Säite réckelen',
'right-move-subpages'        => 'Säiten zesumme mat hiren Ënnersäite réckelen',
'right-suppressredirect'     => 'Kee Redirect vum ale Numm aus uleeë wann eng Säit eréckelt gëtt',
'right-upload'               => 'Fichieren eroplueden',
'right-reupload'             => 'E Fichier iwwerschreiwen',
'right-reupload-own'         => 'E Fichier iwwerschreiwen dee vum selweschte Benotzer eropgeluede gouf',
'right-upload_by_url'        => 'E Fichier vun enger URL-Adress eroplueden',
'right-purge'                => 'De Säitecache eidelmaachen ouni nozefroen',
'right-autoconfirmed'        => 'Hallef-protegéiert Säiten änneren',
'right-bot'                  => 'Als automatesche Prozess behandelen (Bot)',
'right-nominornewtalk'       => 'Kleng Ännerungen op Diskussiounssäite léisen de Banner vun de neie Messagen net aus',
'right-apihighlimits'        => 'Benotzt méi héich Limite bei den API Ufroen',
'right-writeapi'             => "API benotzen fir d'Wiki z'änneren",
'right-delete'               => 'Säite läschen',
'right-bigdelete'            => 'Säite mat engem groussen Historique läschen',
'right-deleterevision'       => 'Spezifesch Versioune vu Säite läschen a restauréieren',
'right-deletedhistory'       => 'Weis geläschte Versiounen am Historique, ouni den assoziéierten Text',
'right-browsearchive'        => 'Geläschte Säite sichen',
'right-undelete'             => 'Eng Säit restauréieren',
'right-suppressrevision'     => 'Virun den Administrateure verstoppte Versiounen nokucken a restauréieren',
'right-suppressionlog'       => 'Privat Lëschte kucken',
'right-block'                => 'Aner Benotzer fir Ännerunge spären',
'right-blockemail'           => 'E Benotzer spären esou datt hie keng Maile verschécke kann',
'right-hideuser'             => 'E Benotzernumm spären, an deem e virun der Ëffentlechkeet verstoppt gëtt',
'right-ipblock-exempt'       => 'Ausname vun IP-Spären, automatesche Spären a vu Späre vu Plage vun IPen',
'right-proxyunbannable'      => 'Automatesche Proxyspären ëmgoen',
'right-protect'              => 'Protectiounsniveauen änneren a protegéiert Säiten änneren',
'right-editprotected'        => 'Protegéiert Säiten (ouni Kaskadeprotectioun) änneren',
'right-editinterface'        => 'De Benotzerinterface änneren',
'right-editusercssjs'        => 'Anere Benotzer hir CSS a JS Fichieren änneren',
'right-rollback'             => "Ännerunge vum läschte Benotzer vun enger spezieller Säit séier z'récksetzen ''(rollback)''",
'right-markbotedits'         => 'Annuléiert Ännerungen als Botännerungen uweisen',
'right-import'               => 'Säite vun anere Wikien importéieren',
'right-importupload'         => 'Säite vun engem eropgeluedene Ficher importéieren',
'right-patrol'               => 'Aneren hir Ännerungen als kontrolléiert markéieren',
'right-unwatchedpages'       => 'Lëscht vun den net iwwerwaachte Säite weisen',
'right-mergehistory'         => 'Zesummeféierung vum Historique vun de Versioune vu Säiten',
'right-userrights'           => 'All Benotzerrechter änneren',
'right-userrights-interwiki' => 'Benotzerrechter vu Benotzer op anere Wiki-Siten änneren',
'right-siteadmin'            => "Datebank spären an d'Spär ophiewen",

# User rights log
'rightslog'      => 'Logbuch vun de Benotzerrechter',
'rightslogtext'  => "Dëst ass d'Lëscht vun den Ännerunge vu Benotzerrechter.",
'rightslogentry' => "huet d'Benotzerrechter vum $1 vun $2 op $3 geännert.",
'rightsnone'     => '(keen)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|Ännerung|Ännerungen}}',
'recentchanges'                     => 'Rezent Ännerungen',
'recentchangestext'                 => "Op dëser Säit kënnt Dir déi rezent Ännerungen op '''{{SITENAME}}''' gesinn.",
'recentchanges-feed-description'    => 'Verfollegt mat dësem Feed déi rezent Ännerungen op {{SITENAME}}.',
'rcnote'                            => "Hei {{PLURAL:$1|ass déi lescht Ännerung|sinn déi lescht '''$1''' Ännerungen}} {{PLURAL:$2|vum leschten Dag|vun de leschten '''$2''' Deeg}}, Stand: $4 ëm $5 Auer.",
'rcnotefrom'                        => "Ugewise ginn d'Ännerunge vum '''$2''' un (maximal '''$1''' Ännerunge gi gewisen).",
'rclistfrom'                        => 'Weis Ännerunge vun $1 un',
'rcshowhideminor'                   => 'Kleng Ännerunge $1',
'rcshowhidebots'                    => 'Botte $1',
'rcshowhideliu'                     => 'Ugemellte Benotzer $1',
'rcshowhideanons'                   => 'Anonym Benotzer $1',
'rcshowhidepatr'                    => '$1 iwwerwaacht Ännerungen',
'rcshowhidemine'                    => 'Meng Ännerungen $1',
'rclinks'                           => 'Weis déi lescht $1 Ännerungen vun de leschten $2 Deeg.<br />$3',
'diff'                              => 'Ënnerscheed',
'hist'                              => 'Versiounen',
'hide'                              => 'verstoppen',
'show'                              => 'weisen',
'minoreditletter'                   => 'k',
'newpageletter'                     => 'N',
'boteditletter'                     => 'B',
'number_of_watching_users_pageview' => '[$1 Benotzer {{PLURAL:$1|iwwerwaacht|iwwerwaachen}}]',
'rc_categories'                     => 'Nëmme Säiten aus de Kategorien (getrennt mat "|"):',
'rc_categories_any'                 => 'All',
'rc-change-size'                    => '$1 {{PLURAL:$1|Byte|Bytes}}',
'newsectionsummary'                 => 'Neien Abschnitt /* $1 */',

# Recent changes linked
'recentchangeslinked'          => 'Ännerungen op verlinkte Säiten',
'recentchangeslinked-title'    => 'Ännerungen a Verbindung matt "$1"',
'recentchangeslinked-noresult' => 'Am ausgewielten Zäitraum goufen op de verlinkte Säite keng Ännerunge gemaach.',
'recentchangeslinked-summary'  => "Dëst ass eng Lëscht matt Ännerunge vu verlinkte Säiten op eng bestëmmte Säit (oder vu Membersäite vun der spetifizéierter Kategorie).
Säite vun [[Special:Watchlist|ärer Iwwerwaachungslëscht]] si '''fett''' geschriwwen.",
'recentchangeslinked-page'     => 'Säitennumm:',
'recentchangeslinked-to'       => 'Weis Ännerungen zu de verlinkte Säiten aplaz vun der gefroter Säit',

# Upload
'upload'                      => 'Eroplueden',
'uploadbtn'                   => 'Fichier eroplueden',
'reupload'                    => 'Nacheemol eroplueden',
'reuploaddesc'                => 'Eroplueden ofbriechen an zréck op de Formulaire fir Eropzelueden',
'uploadnologin'               => 'Net ageloggt',
'uploadnologintext'           => 'Dir musst [[Special:UserLogin|agelogged sinn]], fir Fichieren eroplueden zu kënnen.',
'upload_directory_missing'    => 'De Repertoire an deen Dir eropluede wollt ($1) feelt a konnt net vum Webserver ugeluecht ginn.',
'upload_directory_read_only'  => 'De Webserver kann net an den Upload-Repertoire ($1) schreiwen.',
'uploaderror'                 => 'Feeler bäim Eroplueden',
'uploadtext'                  => "Benotzt dëse Formulair, fir nei Fichieren eropzelueden.
Gitt op d'[[Special:ImageList|Lëscht vun den eropgeluedene Fichieren]], fir no Fichieren ze sichen déi virdrun eropgeluede goufen, Eropgeluedungen fannt dir an der [[Special:Log/upload|Lëscht vun den eropgeluedene Fichieren]], geläschte Fichieren am [[Special:Log/delete|Läschlog]].

Fir e '''Bild''' op enger Säit zu benotzen, schreiwt aplaz vum Bild eng vun dëse Formelen:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fichier.jpg]]</nowiki></tt>''' fir déi ganz Versioun vum Fichier ze benotzen
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Fichier.png|200px|thumb|left|alt text]]</nowiki></tt>''' fir eng 200 Pixel breed Versioun an enger Këscht am lenke Rand mat 'alt text' als Beschreiwung
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Fichier.ogg]]</nowiki></tt>''' fir e Fichier direk ze verlinken ouni de Fichier ze weisen",
'upload-permitted'            => 'Erlaabte Formater vun de Fichieren: $1.',
'upload-preferred'            => 'Fichierszorten déi am beschte funktionéieren: $1.',
'upload-prohibited'           => 'Verbuede Fichiers Formater: $1.',
'uploadlog'                   => 'Lëscht vun den eropgeluedene Fichieren',
'uploadlogpage'               => 'Logbuch vum Eroplueden',
'uploadlogpagetext'           => "Dëst ass d'Lëscht vun de rezente Fichieren déi eropgeluede goufen.
Kuckt [[Special:NewImages|d'Gallerie vun de neie Fichieren]] wann Dir méi e visuellen Iwwerbléck wëllt",
'filename'                    => 'Numm vum Fichier',
'filedesc'                    => 'Resumé',
'fileuploadsummary'           => 'Resumé/Quell:',
'filestatus'                  => 'Copyright Status:',
'filesource'                  => 'Quell:',
'uploadedfiles'               => 'Eropgeluede Fichierën',
'ignorewarning'               => 'Warnung ignoréieren an de Fichier nawell späicheren',
'ignorewarnings'              => 'Ignoréier all Iwwerschreiwungswarnungen',
'minlength1'                  => "D'Nimm vu Fichiere musse mindestens e Buschtaf am Numm hunn.",
'illegalfilename'             => 'Am Fichiernumm "$1" sti Schrëftzeechen, déi net am Numm vun enger Säit erlaabt sinn. W.e.g. nennt de Fichier anescht, a probéiert dann nach eng Kéier.',
'badfilename'                 => 'Den Numm vum Fichier gouf an "$1" ëmgeännert.',
'filetype-badmime'            => 'Fichieren vum MIME-Typ "$1" kënnen net eropgeluede ginn.',
'filetype-unwanted-type'      => "'''\".\$1\"''' ass een onerwënschte Fichiersformat. 
Erwënschte {{PLURAL:\$3|Format ass|Formater sinn}}: \$2.",
'filetype-banned-type'        => "'''.$1''' ass ee Fichersformat deen net erlaabt ass. 
Erlaabt {{PLURAL:$3|ass|sinn}}: $2.",
'filetype-missing'            => 'De Fichier huet keng Erweiderung (wéi z. B. ".jpg").',
'large-file'                  => "D'Fichieren sollte no Méiglechkeet net méi grouss wéi $1 sinn. Dëse Fhihier huet $2.",
'largefileserver'             => 'Dëse Fichier ass méi grouss wéi déi um Server agestallte Maximalgréisst.',
'emptyfile'                   => 'De Fichier deen Dir eropgelueden hutt, schéngt eidel ze sinn. Dëst kann duerch en Tippfeeler am Numm vum Fichier kommen. Préift w.e.g. no, op Dir dëse Fichier wierklech eropluede wëllt.',
'fileexists'                  => 'Et gëtt schonn e Fichier mat dësem Numm, kuckt w.e.g. <strong><tt>$1</tt></strong> wann Dir net sécher sidd, op Dir den Numm ännere wëllt.',
'filepageexists'              => "Eng Beschreiwungssäit gouf schonns als <strong><tt>$1</tt></strong> geschriwwen, et gëtt awer kee Fichier mat deem Numm.

Dir kënnt also äre Fichier eroplueden, mee déi Beschreiwung déi dir aginn hutt gëtt net op d'Beschreiwungssäit iwwerholl. D'Beschreiwungssäit musst der nom Eropluede vum Fichier nach manuell änneren.",
'fileexists-extension'        => 'E Fichier mat engem ähnlechen Namen gëtt et schonn:<br />
Numm vum Fichier den Dir versicht eropzelueden: <strong><tt>$1</tt></strong><br />
Numm vum Fichier den et scho gëtt: <strong><tt>$2</tt></strong><br />
Wielt w.e.g. en anere Numm.',
'fileexists-thumb'            => "<center>'''Dëse Fichier gëtt et'''</center>",
'file-thumbnail-no'           => 'Den Numm vum Fichier fänkt mat <strong><tt>$1</tt></strong> unn.
Da däit drop hin dat et e Bild vu reduzéierter Gréisst <i>(thumbnail)</i> ass.
Wann Dir dat Bild a méi enger grousser Opléisung hutt, da lued dëst erop, soss ännert den Numm vum Fichier w.e.g.',
'fileexists-forbidden'        => "Et gëtt schonn e Fichier mat ësem Nummm. Gitt w.e.g. z'réck a lued dëse Fichier ënntert engem aner Numm erop. [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => 'E Fichier mat dësem Numm gëtt et schonn an dem gedeelte Repertoire.
Wann Dir dëse Fichier trotzdem eroplued wellt da gitt w.e.g. zréck a lued dëse Fichier ënner engem anere Numm erop. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Dëse Fichier schéngt een Doublon vun {{PLURAL:$1|dësem Fichier|dëse Fichieren}} ze sinn:',
'successfulupload'            => 'Eroplueden erfollegräich',
'uploadwarning'               => 'Opgepasst',
'savefile'                    => 'Fichier späicheren',
'uploadedimage'               => 'huet "[[$1]]" eropgelueden',
'overwroteimage'              => 'huet eng nei Versioun vun "[[$1]]" eropgelueden',
'uploaddisabled'              => "Pardon, d'Eroplueden vu Fichieren ass ausgeschalt.",
'uploaddisabledtext'          => "D'Eroplueden vu Fichieren ass ausgeschalt.",
'uploadscripted'              => 'An dësem Fichier ass HTML- oder Scriptcode, de vun engem Webbrowser falsch interpretéiert kéint ginn.',
'uploadcorrupt'               => 'De Fichier ass futti oder en huet eng falsch Fichiers-Erweiderung. Kuckt de Fichier weg no a lued de Fichier nach eng Kéier erop.',
'uploadvirus'                 => 'An dësem Fichier ass ee Virus! Detailer: $1',
'sourcefilename'              => 'Numm vum Originalfichier:',
'destfilename'                => 'Numm vum Fichier',
'upload-maxfilesize'          => 'Maximal Fichiersgréisst: $1',
'watchthisupload'             => 'Dës Säit iwwerwaachen',
'filewasdeleted'              => 'E Fichier matt dësem Numm gouf schonn eemol eropgelueden an duerno nees geläscht. kuckt w.e.g op $1 no, ier Dir dee Fichier nach eng Kéier eroplued.',
'upload-wasdeleted'           => "'''Opgepasst: Dir lued e Fichier erop, dee schonn eng Kéier geläscht ginn ass.'''

Kuckt w.e.g. genee no, ob d'dat erneit Eroplueden de Richtlinnen entsprecht.
Zu ärer Informatioun steett hei Läsch-Lëscht mat dem Grond vum viregte Läschen:",
'filename-bad-prefix'         => 'Den Numm vum Fichier fänkt mat <strong>„$1“</strong> un. Dësen Numm ass automatesch vun der Kamera gi ginn a seet näischt iwwert dat aus, wat drop ass. Gitt dem Fichier w.e.gl. en Numm, deen den Inhalt besser beschreift, an deen net verwiesselt ka ginn.',

'upload-proto-error'      => 'Falsche Protokoll',
'upload-proto-error-text' => "D'URL muss matt <code>http://</code> oder <code>ftp://</code> ufänken.",
'upload-file-error'       => 'Interne Feeler',
'upload-file-error-text'  => 'Beim Erstelle vun engem temporäre Fichier um Server ass een interne Feeler geschitt.
Informéiert w.e.g. e vun den [[Special:ListUsers/sysop|Administrateuren]].',
'upload-misc-error'       => 'Onbekannte Feeler beim Eroplueden',
'upload-misc-error-text'  => "Beim Eroplueden ass en onbekannte Feeler geschitt.
Kuckt d'URL w.e.g. no, a vergewëssert iech datt d'Säit online ass a probéiert et dann nach eng Kéier.
Wann de Problem weider besteet, dann un de [[Special:ListUsers/sysop|Administrateuren]].",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => "URL ass net z'erreechen",
'upload-curl-error6-text'  => 'Déi URL déi Dir uginn hutt kann net erreecht ginn.
Kuckt w.e.g. no op kee Feeler an der URL ass an op de Site och online ass.',
'upload-curl-error28'      => "D'Eroplueden huet ze laang gedauert (timeout)",
'upload-curl-error28-text' => "Dëse Site huet ze laang gebraucht fir z'äntwerten. Kuckt w. e. g. no, ob dëse Site online ass, waart een Ament a probéiert et dann nach eng Kéier. Et ka sënnvoll sinn, et nach eng Kéier méi spéit ze versichen.",

'license'            => 'Lizenzéiert:',
'nolicense'          => 'Keng Lizenz ausgewielt',
'license-nopreview'  => '(Kucken ouni ofzespäichere geet net)',
'upload_source_url'  => ' (gülteg, ëffentlech zougänglech URL)',
'upload_source_file' => ' (e Fichier op Ärem Computer)',

# Special:ImageList
'imagelist-summary'     => "Op dëser Spezialsäit stinn all déi eropgeluede Fichieren. Déi als läscht eropgeluede Fichieren ginn als éischt ugewisen. Duerch e Klick op d'Iwwerschrëfte vun de Kolonnen kënnt Dir d'Sortéierung ëmdréinen an Dir kënnt esou och no enger anerer Kolonn sortéieren.",
'imagelist_search_for'  => 'Sicht nom Fichier:',
'imgfile'               => 'Fichier',
'imagelist'             => 'Lëscht vun de Fichieren',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Numm',
'imagelist_user'        => 'Benotzer',
'imagelist_size'        => 'Gréisst',
'imagelist_description' => 'Beschreiwung',

# Image description page
'filehist'                       => 'Versiounen',
'filehist-help'                  => 'Klickt op e bestëmmten Zäitpunkt fir déi respektiv Versioun vum Fichier ze kucken.',
'filehist-deleteall'             => 'All Versioune läschen',
'filehist-deleteone'             => 'Läschen',
'filehist-revert'                => 'zrécksetzen',
'filehist-current'               => 'aktuell',
'filehist-datetime'              => 'Versioun vum',
'filehist-user'                  => 'Benotzer',
'filehist-dimensions'            => 'Dimensiounen',
'filehist-filesize'              => 'Gréisst vum Fichier',
'filehist-comment'               => 'Bemierkung',
'imagelinks'                     => 'Biller Linken',
'linkstoimage'                   => 'Dës {{PLURAL:$1|Säit benotzt|Säite benotzen}} dëse Fichier:',
'nolinkstoimage'                 => 'Keng Säit benotzt dëse Fichier.',
'morelinkstoimage'               => 'Weis [[Special:WhatLinksHere/$1|méi Linken]] op dëse Fichier.',
'redirectstofile'                => '{{PLURAL:$1|De Fichier leed|Dës Fichiere leede}} virun op de Fichier:',
'duplicatesoffile'               => '{{PLURAL:$1|De Fichier ass een Doublon|Dës Fichiere sinn Doublonen}} vum Fichier:',
'sharedupload'                   => 'Dës Fichier ass ee gemeinsam genotzten Upload a ka vun anere Projeten benotzt ginn.',
'shareduploadwiki'               => 'Kuckt w.e.g. $1 fir méi Informatiounen.',
'shareduploadwiki-desc'          => "D'Beschreiwung op sénger $1 op dem geeelte Repertoire steet ënnendrënner.",
'shareduploadwiki-linktext'      => 'Datei-Beschreiwungssäit',
'shareduploadduplicate'          => 'Dëse Fichier ass een Doublon vun $1 aus dem gemeinsam genotzte Repertoire.',
'shareduploadduplicate-linktext' => 'een anere Fichier',
'shareduploadconflict'           => 'Dëse Fichier huet deeselweschte Numm wéi $1 aus engem gemeinschaftlech genotzte Repertoire.',
'shareduploadconflict-linktext'  => 'een anere Fichier',
'noimage'                        => 'Ee Fichier mat dësem Numm gëtt et net, Dir kënnt awer $1.',
'noimage-linktext'               => 'eent eroplueden',
'uploadnewversion-linktext'      => 'Eng nei Versioun vun dësem Fichier eroplueden',
'imagepage-searchdupe'           => 'No duebele Fichiere sichen',

# File reversion
'filerevert'                => '"$1" zrécksetzen',
'filerevert-legend'         => 'De Fichier zrécksetzen.',
'filerevert-intro'          => "Du setz de Fichier '''[[Media:$1|$1]]''' op d'[$4 Versioun vum $2, $3 Auer] zréck.",
'filerevert-comment'        => 'Grond:',
'filerevert-defaultcomment' => "zréckgesat op d'Versioun vum $1, $2 Auer",
'filerevert-submit'         => 'Zrécksetzen',
'filerevert-success'        => "'''[[Media:$1|$1]]''' gouf op d'[$4 Versioun vum $2, $3 Auer] zréckgesat.",
'filerevert-badversion'     => 'Et gët keng Versioun vun deem Fichier mat der Zäitinformatoun déi Dir uginn hutt.',

# File deletion
'filedelete'                  => 'Läsch "$1"',
'filedelete-legend'           => 'Fichier läschen',
'filedelete-intro'            => "Dir läscht de Fichier '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Dir läscht  d'Versioun $4  vum $2, $3 Auer vum Fichier '''„[[Media:$1|$1]]“'''.",
'filedelete-comment'          => 'Grond:',
'filedelete-submit'           => 'Läschen',
'filedelete-success'          => "'''$1''' gouf geläscht.",
'filedelete-success-old'      => "D'Versioun vu(n) '''[[Media:$1|$1]]''' vum $2, $3 Auer gouf geläscht.",
'filedelete-nofile'           => "'''$1''' gëtt et net.",
'filedelete-nofile-old'       => "Et gëtt vun '''$1''' keng archivéiert Versioun mat den Attributer déi dir uginn hutt.",
'filedelete-iscurrent'        => "Dir versicht déi aktuell Versioun vun dësem Fichier ze läschen.
Setzt dëse w.e.g. vir d'éischt op méi eng al Versioun zréck.",
'filedelete-otherreason'      => 'Aneren/zousätzleche Grond:',
'filedelete-reason-otherlist' => 'Anere Grond',
'filedelete-reason-dropdown'  => "* Allgemeng Läschgrënn
** Verletzung vun den Droits d'auteur
** De Fichier gëtt et nach eng Kéier an der Datebank",
'filedelete-edit-reasonlist'  => 'Läschgrënn änneren',

# MIME search
'mimesearch'         => 'Sich no MIME-Zort',
'mimesearch-summary' => "Op dëser Spezialsäit kënnen d'Fichieren no hirem MIME-Typ gefiltert ginn.
Dir musst ëmmer de Medien- a Subtyp aginn: z. Bsp. <tt>image/jpeg</tt>.",
'mimetype'           => 'MIME-Typ:',
'download'           => 'eroflueden',

# Unwatched pages
'unwatchedpages' => 'Nët iwwerwaachte Säiten',

# List redirects
'listredirects' => 'Lëscht vun de Viruleedungen',

# Unused templates
'unusedtemplates'     => 'Onbenotzte Schablounen',
'unusedtemplatestext' => "Op dëser Säit stinn all d'Schablounen, déi a kenger anerer Säit benotzt ginn. Vergiesst net nozekucken, ob et keng aner Linken op dës Schabloune gëtt.",
'unusedtemplateswlh'  => 'Aner Linken',

# Random page
'randompage'         => 'Zoufallssäit',
'randompage-nopages' => 'Et gëtt keng Säiten an dësem Nummraum.',

# Random redirect
'randomredirect'         => 'Zoufälleg Viruleedung',
'randomredirect-nopages' => 'An dësem Nummraum gëtt et keng Viruleedungen.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => '{{SITENAME}}-Statistik',
'userstats'              => 'Benotzerstatistik',
'sitestatstext'          => "Et sinn am Ganzen '''\$1''' {{PLURAL:\$1|Säit|Säiten}} an der Datebank.
Dozou zielen d'\"Diskussiounssäiten\", Säiten iwwert {{SITENAME}}, kuerz Säiten, Viruleedungen an anerer déi eventuell net als Säite gezielt kënne ginn.

Déi ausgeschloss ginn et {{PLURAL:\$2|Säit|Säiten}} déi als Säite betruecht {{PLURAL:\$2|ka|kënne}} ginn.

Am ganzen {{PLURAL:\$8|gouf '''1''' Fichier|goufen '''\$8''' Fichieren}} eropgelueden.

Am ganze gouf '''\$3''' {{PLURAL:\$3|Säitenoffro|Säitenoffroen}} ann '''\$4''' {{PLURAL:\$4|Ännerung|Ännerunge}} zënter datt {{SITENAME}} ageriicht gouf.

Doraus ergi sech '''\$5''' Ännerunge pro Säit an '''\$6''' Säitenoffroen pro Ännerung.

Längt vun der [http://www.mediawiki.org/wiki/Manual:Job_queue „Job queue“]: '''\$7'''",
'userstatstext'          => "'''$1''' [[Special:ListUsers|Benotzer]] {{PLURAL:$1|ass|sinn}} ageschriwwen.  '''$2''' (oder '''$4%''') vun dëse {{PLURAL:$2|ass|sinn}} $5.",
'statistics-mostpopular' => 'Am meeschte gekuckte Säiten',

'disambiguations'      => 'Homonymie Säiten',
'disambiguationspage'  => 'Schabloun:Homonymie',
'disambiguations-text' => 'Dës Säite si mat enger Homonymie-Säit verlinkt.
Sie sollten am beschten op déi eigentlech gemengte Säit verlinkt sinn.<br />
Eng Säite gëtt als Homonymiesäit behandelt, wa si eng Schabloun benotzt déi vu [[MediaWiki:Disambiguationspage]] verlinkt ass.',

'doubleredirects'            => 'Duebel Viruleedungen',
'doubleredirectstext'        => '<b>Opgepasst:</b> An dëser Lëscht kënne falsch Positiver stoen. Dat heescht meeschtens datt et nach Text zu de Linke vun der éischter Viruleedung gëtt.<br /> 
An all Rei sti Linken zur éischter an zweeter Viruleedung, souwéi déi éischt Zeil vum Text vun der zweeter Viruleedung, wou normalerweis déi "richteg" Zilsäit drasteet, op déi déi éischt Viruleedung hilinke soll.',
'double-redirect-fixed-move' => '[[$1]] gouf geréckelt, et ass elo en Viruleedung op [[$2]]',
'double-redirect-fixer'      => 'Verbesserung vu Viruleedungen',

'brokenredirects'        => 'Futtis Viruleedungen',
'brokenredirectstext'    => 'Viruleedungen op Säiten déi et net gëtt.',
'brokenredirects-edit'   => '(änneren)',
'brokenredirects-delete' => '(läschen)',

'withoutinterwiki'         => 'Säiten ouni Interwiki-Linken',
'withoutinterwiki-summary' => 'Op dëser Spezialsäit stinn all déi Säiten déi keng Interwikilinken hunn.',
'withoutinterwiki-legend'  => 'Prefix',
'withoutinterwiki-submit'  => 'Weisen',

'fewestrevisions' => 'Säite mat de mannsten Ännerungen',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|Byte|Byten}}',
'ncategories'             => '$1 {{PLURAL:$1|Kategorie|Kategorien}}',
'nlinks'                  => '$1 {{PLURAL:$1|Link|Linken}}',
'nmembers'                => '$1 {{PLURAL:$1|Member|Memberen}}',
'nrevisions'              => '$1 {{PLURAL:$1|Versioun|Versiounen}}',
'nviews'                  => '$1 {{PLURAL:$1|Offro|Offroen}}',
'specialpage-empty'       => 'Dës Säit ass eidel.',
'lonelypages'             => 'Weesesäiten',
'lonelypagestext'         => 'Dës Säite sinn net vun anere Säite vun {{SITENAME}} verlinkt.',
'uncategorizedpages'      => 'Säiten ouni Kategorie',
'uncategorizedcategories' => 'Kategorien déi selwer nach keng Kategorie hunn',
'uncategorizedimages'     => 'Biller ouni Kategorie',
'uncategorizedtemplates'  => 'Schablounen ouni Kategorie',
'unusedcategories'        => 'Onbenotzt Kategorien',
'unusedimages'            => 'Onbenotzte Biller',
'popularpages'            => 'Populär Säiten',
'wantedcategories'        => 'Gewënschte Kategorien',
'wantedpages'             => 'Gewënschte Säiten',
'missingfiles'            => 'Fichieren déi feelen',
'mostlinked'              => 'Dacks verlinkte Säiten',
'mostlinkedcategories'    => 'Dacks benotzte Kategorien',
'mostlinkedtemplates'     => 'Dacks benotzte Schablounen',
'mostcategories'          => 'Säite mat de meeschte Kategorien',
'mostimages'              => 'Dacks benotzte Biller',
'mostrevisions'           => 'Säite mat de meeschten Versiounen',
'prefixindex'             => 'All Säiten (no hiren Ufanksbuchstawen)',
'shortpages'              => 'Kuerz Säiten',
'longpages'               => 'Laang Säiten',
'deadendpages'            => 'Sakgaasse-Säiten',
'deadendpagestext'        => 'Dës Säite si mat kenger anerer Säit op {{SITENAME}} verlinkt.',
'protectedpages'          => 'Protegéiert Säiten',
'protectedpages-indef'    => 'Nëmme onbegrenzt-gespaarte Säite weisen',
'protectedpagestext'      => 'Dës Säite si gespaart esou datt si weder geännert nach geréckelt kënne ginn',
'protectedpagesempty'     => 'Elo si keng Säite mat dëse Parameteren protegéiert.',
'protectedtitles'         => 'Gespaarten Titel',
'protectedtitlestext'     => 'Dës Titele si protegéiert an e ka keng Säit mat deenen Titelen ugeluecht ginn',
'protectedtitlesempty'    => 'Zur Zäit si mat de Parameteren déi Dir uginn huet keng Säite gespaart esou datt si net ugeluecht kënne ginn.',
'listusers'               => 'Benotzerlëscht',
'newpages'                => 'Nei Säiten',
'newpages-username'       => 'Benotzernumm:',
'ancientpages'            => 'Al Säiten',
'move'                    => 'Réckelen',
'movethispage'            => 'Dës Säit réckelen',
'unusedimagestext'        => 'Denkt w.e.g. drunn datt aner Internetsäiten dëse Fichier matt enger direkter URL verlinke kënnen. An dem Fall gëtt de Fichier hei opgelëscht obwuel en aktiv gebraucht gëtt.',
'unusedcategoriestext'    => 'Dës Kategoriesäiten existéieren, mee weder en Artikel nach eng Kategorie maachen dovunner Gebrauch.',
'notargettitle'           => 'Dir hutt keng Säit uginn.',
'notargettext'            => 'Dir hutt keng Zilsäit oder keen Zilbenotzer uginn fir déi dës Funktioun ausgheféiert soll ginn.',
'nopagetitle'             => 'Zilsäit gëtt et net',
'nopagetext'              => 'Déi Zilsäit déi dir uginn hutt gëtt et net.',
'pager-newer-n'           => '{{PLURAL:$1|nächsten|nächst $1}}',
'pager-older-n'           => '{{PLURAL:$1|vireg|vireg $1}}',
'suppress'                => 'Iwwersiicht',

# Book sources
'booksources'               => 'Bicherreferenzen',
'booksources-search-legend' => 'No Bicherreferenze sichen',
'booksources-go'            => 'Sichen',
'booksources-text'          => 'Hei ass eng Lescht mat Linken op Internetsäiten, déi nei a gebraucht Bicher verkafen. Do kann et sinn datt Dir méi Informatiounen iwwer déi Bicher fannt déi Dir sicht.',

# Special:Log
'specialloguserlabel'  => 'Benotzer:',
'speciallogtitlelabel' => 'Titel:',
'log'                  => 'Logbicher',
'all-logs-page'        => "All d'Logbicher",
'log-search-legend'    => 'Logbicher duerchsichen',
'log-search-submit'    => 'Sichen',
'alllogstext'          => "Dëst ass eng kombinéiert Lëscht vu Logbicher op {{SITENAME}}.
Dir kënnt d'Sich limitéieren wann dir e Log-Typ, e Benotzernumm (case-senisitive) oder déi gefrote Säit  (och case-senisitive) agitt.",
'logempty'             => 'Näischt fonnt.',
'log-title-wildcard'   => 'Titel fänkt u matt …',

# Special:AllPages
'allpages'          => 'All Säiten',
'alphaindexline'    => '$1 bis $2',
'nextpage'          => 'Nächst Säit ($1)',
'prevpage'          => 'Säit viru(n) ($1)',
'allpagesfrom'      => 'Säite weisen, ugefaange mat:',
'allarticles'       => "All d'Säiten",
'allinnamespace'    => "All d'Säiten ($1 Nummraum)",
'allnotinnamespace' => "All d'Säiten (net am $1 Nummraum)",
'allpagesprev'      => 'Vireg',
'allpagesnext'      => 'Nächst',
'allpagessubmit'    => 'Lass',
'allpagesprefix'    => 'Säite mat Prefix weisen:',
'allpagesbadtitle'  => 'Den Titel vun dëser Säit ass net valabel oder hat en Interwiki-Prefix. Et ka sinn datt een oder méi Zeechen drasinn, déi net an Titele benotzt kënne ginn.',
'allpages-bad-ns'   => 'De Nummraum „$1“ gëtt et net op {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Kategorien',
'categoriespagetext'            => 'Dës Kategorie huet Säiten oder Medien.
[[Special:UnusedCategories|Onbenotze Kategorien]] ginn hei net gewisen.
Kuckt och [[Special:WantedCategories|Gewënschte Kategorien]].',
'categoriesfrom'                => 'Weis Kategorien ugefaang bäi:',
'special-categories-sort-count' => 'No der Zuel sortéieren',
'special-categories-sort-abc'   => 'alphabetesch sortéieren',

# Special:ListUsers
'listusersfrom'      => "D'Benotzer uweisen, ugefaange bei:",
'listusers-submit'   => 'Weis',
'listusers-noresult' => 'Kee Benotzer fonnt.',

# Special:ListGroupRights
'listgrouprights'          => 'Rechter vun de Benotzergruppen',
'listgrouprights-summary'  => 'Dëst ass eng Lëscht vun den op dëser Wiki definéierte Benotzergruppen an den domatt verbonnene Rechter.
Et ginn [[{{MediaWiki:Listgrouprights-helppage}}|zousätzlech Informatiounen]] iwwer individuell Benotzerrechter.',
'listgrouprights-group'    => 'Grupp',
'listgrouprights-rights'   => 'Rechter',
'listgrouprights-helppage' => 'Help:Grupperechter',
'listgrouprights-members'  => '(Lëscht vun de Memberen)',

# E-mail user
'mailnologin'     => 'Keng E-Mailadress',
'mailnologintext' => 'Dir musst [[Special:UserLogin|ugemellt]] sinn an eng gülteg E-Mail Adress an äre [[Special:Preferences|Asteelunge]] aginn hunn, fir engem anere Benotzer eng E-Mail ze schécken.',
'emailuser'       => 'Dësem Benotzer eng E-Mail schécken',
'emailpage'       => 'Dem Benotzer eng E-Mail schécken',
'emailpagetext'   => 'Wann dëse Benotzer eng gëlteg E-Mailadress a sengen Astellungen uginn huet, kënnt Dir mat dësem Formulaire e Message schécken. Déi E-Mailadress, déi dir an [[Special:Preferences|Ären Astellungen]] aginn hutt, steet an der "From" Adress vun der Mail, sou datt den Destinataire Iech direkt äntwerte kann.',
'usermailererror' => 'E-Mail-Objet mellt deen heite Feeler:',
'defemailsubject' => 'E-Mail vu(n) {{SITENAME}}',
'noemailtitle'    => 'Keng E-Mailadress',
'noemailtext'     => 'Dëse Benotzer huet keng gülteg E-Mailadress uginn, oder well keng E-Mail vun anere Wikipedianer kréien.',
'emailfrom'       => 'Vum:',
'emailto'         => 'Fir:',
'emailsubject'    => 'Sujet:',
'emailmessage'    => 'Message:',
'emailsend'       => 'Schécken',
'emailccme'       => 'Eng E-Mailkopie vun der Noriicht fir mech',
'emailccsubject'  => 'Kopie vun denger Noriicht un $1: $2',
'emailsent'       => 'E-Mail geschéckt',
'emailsenttext'   => 'Är E-Mail gouf fortgeschéckt.',
'emailuserfooter' => 'Dës E-Mail gouf vum $1 dem $2 geschéckt dobäi gouf d\'Funktioun "Benotzer E-Mail" op {{SITENAME}} benotzt.',

# Watchlist
'watchlist'            => 'Meng Iwwerwaachungslëscht',
'mywatchlist'          => 'Meng Iwwerwaachungslëscht',
'watchlistfor'         => "(fir '''$1''')",
'nowatchlist'          => 'Är Iwwerwaachungslëscht ass eidel.',
'watchlistanontext'    => "Dir musst $1 fir Säiten op ärer Iwwerwaachungslëscht ze gesinn oder z'änneren.",
'watchnologin'         => 'Net ageloggt',
'watchnologintext'     => "Dir musst [[Special:UserLogin|ugemellt]] sinn, fir Är Iwwerwaachungslëscht z'änneren.",
'addedwatch'           => "Op d'Iwwerwaachungslëscht gesat",
'addedwatchtext'       => "D'Säit \"<nowiki>\$1</nowiki>\" gouf bei an är [[Special:Watchlist|Iwwerwaachtungslëscht]] gesat. All weider Ännerungen op dëser Säit an/oder der Diskussiounssäit ginn hei opgelëscht, an d'Säit gesäit '''fettgedréckt''' bei de [[Special:RecentChanges|rezenten Ännerungen]] aus fir, se méi séier erëmzefannen.

Wann dir dës Säit net iwwerwaache wëllt, klickt op \"Net méi iwwerwaachen\" uewen op der Säit.",
'removedwatch'         => 'Vun der Iwwerwaachungslëscht erofgeholl',
'removedwatchtext'     => 'D\'Säit "[[:$1]]" gouf vun ärer Iwwerwaachungslëscht erofgeholl.',
'watch'                => 'Iwwerwaachen',
'watchthispage'        => 'Dës Säit iwwerwaachen',
'unwatch'              => 'Net méi iwwerwaachen',
'unwatchthispage'      => 'Net méi iwwerwaachen',
'notanarticle'         => 'Keng Säit',
'notvisiblerev'        => 'Versioun gouf geläscht',
'watchnochange'        => 'Keng vun Äre verfollegte Säite gouf während der ugewisener Zäitperiod verännert.',
'watchlist-details'    => "{{PLURAL:$1|1 Säit|$1 Säiten}} sinn op ärer Iwwerwaachungsklëscht (d'Diskussiounssäite net matgezielt).",
'wlheader-enotif'      => '* E-Mail-Bescheed ass aktivéiert.',
'wlheader-showupdated' => "* Säiten déi zënter ärer leschter Visite geännert goufen, si '''fett''' geschriwwen",
'watchmethod-recent'   => 'Rezent Ännerunge ginn op iwwerwaacht Säiten iwwerpréift',
'watchmethod-list'     => 'Verfollegt Säite ginn op rezent Ännerungen iwwerpréift',
'watchlistcontains'    => 'Op ärer Iwwerwaachungslëscht $1 {{PLURAL:$1|steet $1 Säit|stinn $1 Säiten}}.',
'iteminvalidname'      => "Problem mat dem Objet '$1', ongültege Numm ...",
'wlnote'               => "Hei {{PLURAL:$1|ass déi lescht Ännerung|sinn dé lescht '''$1''' Ännerunge}} vun {{PLURAL:$2|der leschter Stonn|de leschte(n) '''$2''' Stonnen}}.",
'wlshowlast'           => "Weis d'Ännerunge vun de leschte(n) $1 Stonnen, $2 Deeg oder $3 (an de leschten 30 Deeg).",
'watchlist-show-bots'  => 'Bot-Ännerunge weisen',
'watchlist-hide-bots'  => 'Bot-Ännerunge verstoppen',
'watchlist-show-own'   => 'Meng Ännerunge weisen',
'watchlist-hide-own'   => 'Meng Ännerunge verstoppen',
'watchlist-show-minor' => 'Kleng Ännerunge weisen',
'watchlist-hide-minor' => 'kleng Ännerunge verstoppen',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Iwwerwaachen …',
'unwatching' => 'Net méi iwwerwaachen …',

'enotif_mailer'                => '{{SITENAME}} E-Mail-Informatiounssystem',
'enotif_reset'                 => 'All Säiten als besicht markéieren',
'enotif_newpagetext'           => 'Dëst ass eng nei Säit.',
'enotif_impersonal_salutation' => '{{SITENAME}}-Benotzer',
'changed'                      => 'geännert',
'created'                      => 'gemaach',
'enotif_subject'               => '[{{SITENAME}}] D\'Säit "$PAGETITLE" gouf vum $PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited'           => 'All Ännerungen op ee Bléck: $1',
'enotif_lastdiff'              => 'Kuckt $1 fir dës Ännerung.',
'enotif_anon_editor'           => 'Anonyme Benotzer $1',
'enotif_body'                  => 'Léiwe $WATCHINGUSERNAME,

d\'{{SITENAME}}-Säit "$PAGETITLE" gouf vum $PAGEEDITOR den $PAGEEDITDATE $CHANGEDORCREATED.

Aktuell Versioun: $PAGETITLE_URL

$NEWPAGE

Zusammefaassung vuun dem Mataarbechter: $PAGESUMMARY $PAGEMINOREDIT

Dëse Mataarbechter kontaktéieren:
E-Mail: $PAGEEDITOR_EMAIL
Wiki: $PAGEEDITOR_WIKI

Et gi soulaang keng weider Maile geschéckt, bis Dir d\'Säit nees emol besicht hutt. Op ärer Iwwerwaachungslëscht kënnt Dir all Benoorichtigungsmarkeren zesummen zrécksetzen. 


             Äre frëndleche {{SITENAME}} Benoriichtigungssystem

--
Fir d\'Astellungen op ärer Iwwerwaachungslëscht unzupassen, besicht w.e.g.: {{fullurl:Special:Watchlist/edit}}',

# Delete/protect/revert
'deletepage'                  => 'Säit läschen',
'confirm'                     => 'Konfirméieren',
'excontent'                   => "Inhalt war: '$1'",
'excontentauthor'             => "Op der Säit stong: '$1' (An als eenzegen dru geschriwwen hat de '[[Special:Contributions/$2|$2]]').",
'exbeforeblank'               => "Den Inhalt virum Läsche war: '$1'",
'exblank'                     => "D'Säit war eidel",
'delete-confirm'              => 'Läsche vu(n) "$1"',
'delete-legend'               => 'Läschen',
'historywarning'              => 'Opgepasst: Déi Säit déi dir läsche wëllt huet en Historique.',
'confirmdeletetext'           => "Dir sidd am Gaang, eng Säit mat hirem kompletten Historique vollstänneg aus der Datebank ze läschen.
W.e.g. konfirméiert, datt Dir dëst wierklech wëllt, datt Dir d'Konsequenze verstitt, an datt dat Ganzt en accordance mat de [[{{MediaWiki:Policy-url}}|Richtlinien]] geschitt.",
'actioncomplete'              => 'Aktioun ofgeschloss',
'deletedtext'                 => '"<nowiki>$1</nowiki>" gouf geläscht. Kuckt $2 fir eng Lëscht vun de Säiten déi viru Kuerzem geläscht goufen.',
'deletedarticle'              => '"$1" gouf geläscht',
'suppressedarticle'           => 'geläscht "$1"',
'dellogpage'                  => 'Läschungslog',
'dellogpagetext'              => 'Hei fannt dir eng Lëscht mat rezent geläschte Säiten. All Auerzäiten sinn déi vum Server.',
'deletionlog'                 => 'Läschungslog',
'reverted'                    => 'Op déi Versioun virdrun zréckgesat',
'deletecomment'               => "Grond fir d'Läschen:",
'deleteotherreason'           => 'Aneren/ergänzende Grond:',
'deletereasonotherlist'       => 'Anere Grond',
'deletereason-dropdown'       => '* Heefegst Grënn fir eng Säit ze läschen
** Wonsch vum Auteur
** Verletzung vun engem Copyright
** Vandalismus',
'delete-edit-reasonlist'      => 'Läschgrënn änneren',
'delete-toobig'               => "Dës Säit huet e laangen Historique, méi wéi $1 {{PLURAL:$1|Versioun|Versiounen}}.
D'Läsche vun esou Säite gouf limitéiert fir ongewollte Stéierungen op {{SITENAME}} ze verhënneren.",
'delete-warning-toobig'       => "Dës Säit huet eng laang Versiounsgeschicht, méi wéi $1 {{PLURAL:$1|Versioun|Versiounen}}.
D'Läschen dovun kann zu Stéierungen am Funktionnement vun {{SITENAME}} féieren;
dës Aktioun soll mat Vierssiicht gemaach ginn.",
'rollback'                    => 'Ännerungen zrécksetzen',
'rollback_short'              => 'Zrécksetzen',
'rollbacklink'                => 'Zrécksetzen',
'rollbackfailed'              => 'Zrécksetzen huet net geklappt',
'cantrollback'                => 'Lescht Ännerung kann net zréckgesat ginn. De leschten Auteur ass deen eenzegen Auteur vun dëser Säit.',
'alreadyrolled'               => 'Déi lescht Ännerung vun der Säit [[$1]] vum [[User:$2|$2]] ([[User talk:$2|Diskussioun]] | [[Special:Contributions/$2|{{int:contribslink}}]]); kann net zréckgesat ginn; 
een Aneren huet dëst entweder scho gemaach oder nei Ännerungen agedroen.

Déi lescht Ännerung vun der Säit ass vum [[User:$3|$3]] ([[User talk:$3|Diskussioun]] | [[Special:Contributions/$3|{{int:contribslink}}]]).',
'editcomment'                 => 'Ännerungskommentar: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Ännerunge vum [[Special:Contributions/$2|Kontributioune]] ([[User talk:$2|Diskussioun]]) zréckgesat op déi lescht Versioun vum [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "D'Ännerunge vum $1 goufen zréckgesat op déi lescht Versioun vum $2.",
'sessionfailure'              => 'Et schéngt e Problem mat ärer Loginséance ze ginn;
Dës Aktioun gouf aus Sécherheetsgrënn afgebrach, fir ze verhënneren datt är Séance eine piratéiert ka ginn.
KLickt w.e.g. op "Zréck" a lued déi Sàit vun däer Dir komm sidd nei, a versicht et dann nach eng Kéier.',
'protectlogpage'              => 'Protectiouns-Logbuch',
'protectlogtext'              => "Dëst ass d'Lëscht vun de Säitespären.
Kuckt d'[[Special:ProtectedPages|Lëscht vun de protegéierte Säite]] fir eng L!escht vun den aktuelle Säite-Protectiounen.",
'protectedarticle'            => 'huet [[$1]] protegéiert',
'modifiedarticleprotection'   => 'huet d\'Protectioun vun "[[$1]]" geännert',
'unprotectedarticle'          => "huet d'Spär vu(n) [[$1]] opgehuewen",
'protect-title'               => 'Ännerung vun der Protectioun vu(n) „$1“',
'protect-legend'              => "Konfirméiert d'Protectioun",
'protectcomment'              => 'Grond:',
'protectexpiry'               => 'Dauer vun der Spär:',
'protect_expiry_invalid'      => "D'Dauer déi Dir uginn hutt ass ongültig.",
'protect_expiry_old'          => "D'Spärzäit läit an der Vergaangenheet.",
'protect-unchain'             => 'Réckel-Protectioun änneren',
'protect-text'                => "Hei kënnt Dir de Protectiounsstatus fir d'Säit <strong>$1</strong> kucken an änneren.",
'protect-locked-dblock'       => "Den Niveau vun der Proectioun vun der Säit kann net geänenert ginn, well d'Datebank gespaart ass.
Hei sinn déi aktuell Astellungen fir d'Säit <strong>$1</strong>:",
'protect-locked-access'       => "Dir hutt net déi néideg Rechter fir de Protectiouns-Niveau vun dëser Säit z'änneren.
Hei sinn déi aktuell Astellunge fir d'Säit <strong>$1</strong>:",
'protect-cascadeon'           => "Dës Säit ass elo gespaart well si an déi folgend {{PLURAL:$1|Säit|Säiten}} agebonn ass déi duerch eng Kaskadespär gespaart {{PLURAL:$1|ass|sinn}}. De Protectiounsniveau ka fir dës Seite geännert ginn, dëst huet awer keen Afloss op d'Kaskadespär.",
'protect-default'             => 'Alleguer (Standard)',
'protect-fallback'            => 'Eng "$1"-Autorisatioun gëtt gebraucht.',
'protect-level-autoconfirmed' => 'Spär fir net ugemellte Benotzer',
'protect-level-sysop'         => 'Nëmmen Administrateuren',
'protect-summary-cascade'     => 'Protectioun a Kaskaden',
'protect-expiring'            => 'bis $1 (UTC)',
'protect-cascade'             => "Kaskade-Spär – alleguerten d'Schablounen déi an dës Säit agebonne si ginn och gespaart.",
'protect-cantedit'            => "Dir kënnt d'Spär vun dëser Seite net änneren, well Dir net déi néideg Rechter hutt fir déi Säit z'änneren.",
'restriction-type'            => 'Berechtigung:',
'restriction-level'           => 'NIveau vun de Limitatiounen:',
'minimum-size'                => 'Mindestgréisst',
'maximum-size'                => 'Maximalgréisst:',
'pagesize'                    => '(Byten)',

# Restrictions (nouns)
'restriction-edit'   => 'Änneren',
'restriction-move'   => 'réckelen',
'restriction-create' => 'Uleeën',
'restriction-upload' => 'Eroplueden',

# Restriction levels
'restriction-level-sysop'         => 'ganz protegéiert',
'restriction-level-autoconfirmed' => 'hallef-protegéiert (nëmmen ugemellte Benotzer déi net nei sinn)',
'restriction-level-all'           => 'alleguerten',

# Undelete
'undelete'                     => 'Geläschte Säite restauréieren',
'undeletepage'                 => 'Geläschte Säite kucken a restauréieren',
'undeletepagetitle'            => "'''Op dëser Lëscht sti geläschte Versioune vun [[:$1]]'''.",
'viewdeletedpage'              => 'Geläschte Säite weisen',
'undeletepagetext'             => "Dës Säite goufe geläscht mee sinn nach ëmmer am Archiv a kënne vun Administrateure restauréiert ginn. D'Archiv gëtt periodesch eidel gemaach.",
'undelete-fieldset-title'      => 'Versioune restauréieren',
'undeleteextrahelp'            => "Fir d'Säit komplett mat alle Versiounen ze retabléieren, markéiert keng vun den eenzelne Casë mat engem Krop a klickt op '''''Restauréieren'''''. 
Fir nëmmen eng bestëmmte Versioun vun der Säit ze restauréieren, markéiert d'Case vun der gewënschter Versioun mat engem Krop, a klickt duerno op '''''Restauréieren'''''. 
Klickt op '''''Reset''''' fir d'Kommentarfeld eidel ze maachen an d'Kreep aus all de Casen ewechzehuelen.",
'undeleterevisions'            => '{{PLURAL:$1|1 Versioun|$1 Versiounen}} archivéiert',
'undeletehistory'              => 'Wann Dir dës Säit restauréiert, ginn och all déi al Versioune restauréiert.
Wann zënter dem Läschen eng nei Säit mat dem selweschte Numm ugeluecht gouf, ginn déi restauréiert Versioune chronologesch an den Historique agedro.',
'undeletehistorynoadmin'       => "Dës Säit gouf geläscht. De Grond fir d'Läsche gesitt der ënnen, zesumme mat der Iwwersiicht vun den eenzele Versioune vun der Säit an hiren Auteuren. Déi verschidden Textversioune kënnen awer just vun Administrateure gekuckt a restauréiert ginn.",
'undelete-revision'            => 'Geläschte Versioun vun $1 (Versioun  vum $2) vum $3:',
'undeleterevision-missing'     => "Ongëlteg oder Versioun déi feelt. Entweder ass de Link falsch oder d'Versioun gouf aus dem Archiv restauréiert oder geläscht.",
'undelete-nodiff'              => 'Et si keng méi al Versiounen do.',
'undeletebtn'                  => 'Restauréieren',
'undeletelink'                 => 'restauréieren',
'undeletereset'                => 'Ofbriechen',
'undeletecomment'              => 'Grond:',
'undeletedarticle'             => 'huet "[[$1]]" restauréiert',
'undeletedrevisions'           => '$1 {{PLURAL:$1|Versioun gouf|$1 Versioune goufe}} restauréiert',
'undeletedrevisions-files'     => '{{PLURAL:$1|1 Versioun|$1 Versiounen}} an {{PLURAL:$2|1 Fichier|$2 Fichieren}} goufe restauréiert',
'undeletedfiles'               => '$1 {{PLURAL:$1|Fichier gouf|Fichiere goufe}} restauréiert',
'cannotundelete'               => "D'Restauratioun huet net fonktionéiert. Een anere Benotzer huet déi Säit warscheinlech scho virun iech restauréiert.",
'undeletedpage'                => "'''$1''' gouf restauréiert.

Am [[Special:Log/delete|Läsch-Logbuch]] fannt Dir déi geläscht a restauréiert Säiten.",
'undelete-header'              => 'Kuckt [[Special:Log/delete|Läschlescht]] fir rezent geläschte Säiten.',
'undelete-search-box'          => 'Sich no geläschte Säiten',
'undelete-search-prefix'       => 'Weis Säiten déi esou ufänken:',
'undelete-search-submit'       => 'Sichen',
'undelete-no-results'          => 'Et goufen am Archiv keng Säite fonnt déi op är Sich passen.',
'undelete-filename-mismatch'   => "D'Dateiversioun vum $1 konnt net restauréiert ginn: De Fichier gouf net fonnt.",
'undelete-bad-store-key'       => "D'Versioun vum Fichier mat dem Zäitstempel $1 konnt net restauréiert ginn: De Fichier war scho virum Läschen net méi do.",
'undelete-cleanup-error'       => 'Feeler beim Läsche vun der onbenotzter Archiv-Versioun $1.',
'undelete-missing-filearchive' => 'De Fichier mat der Archiv-ID $1 kann net restauréiert ginn, well e net an der Datebank ass. Méiglecherweis gouf e scho restauréiert.',
'undelete-error-short'         => 'Feeler bäim Restauréieren vum Fichier: $1',
'undelete-error-long'          => 'Beim Restauréiere vun engem Fichier goufe Feeler fonnt:

$1',

# Namespace form on various pages
'namespace'      => 'Nummraum:',
'invert'         => 'Auswiel ëmdréinen',
'blanknamespace' => '(Haapt)',

# Contributions
'contributions' => 'Kontributiounen vum Benotzer',
'mycontris'     => 'Meng Kontributiounen',
'contribsub2'   => 'Fir $1 ($2)',
'nocontribs'    => 'Et goufe keng Ännerunge fonnt, déi dëse Kritèren entspriechen.',
'uctop'         => '(aktuell)',
'month'         => 'Vum Mount (a virdrun):',
'year'          => 'Vum Joer (a virdrun):',

'sp-contributions-newbies'     => 'Nëmme Kontributioune vun neie Mataarbechter weisen',
'sp-contributions-newbies-sub' => 'Fir déi Nei',
'sp-contributions-blocklog'    => 'Spärlescht',
'sp-contributions-search'      => 'No Kontributioune sichen',
'sp-contributions-username'    => 'IP-Adress oder Benotzernumm:',
'sp-contributions-submit'      => 'Sichen',

# What links here
'whatlinkshere'            => 'Linken op dës Säit',
'whatlinkshere-title'      => 'Säiten, déi mat "$1" verlinkt sinn',
'whatlinkshere-page'       => 'Säit:',
'linklistsub'              => '(Lëscht vun de Linken)',
'linkshere'                => "Déi folgend Säite linken op '''[[:$1]]''':",
'nolinkshere'              => "Keng Säit ass mat '''[[:$1]]''' verlinkt.",
'nolinkshere-ns'           => "Keng Säite linken op '''[[:$1]]''' am gewielten Nummraum.",
'isredirect'               => 'Viruleedung',
'istemplate'               => 'an dëser Säit dran',
'isimage'                  => 'Link op de Fichier',
'whatlinkshere-prev'       => '{{PLURAL:$1|vireg|vireg $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|nächsten|nächst $1}}',
'whatlinkshere-links'      => '← Linken',
'whatlinkshere-hideredirs' => '$1 Viruleedungen',
'whatlinkshere-hidetrans'  => 'Agebonne Schabloune $1',
'whatlinkshere-hidelinks'  => '$1 Linken',
'whatlinkshere-hideimages' => '$1 Linken op de Fichier',
'whatlinkshere-filters'    => 'Filteren',

# Block/unblock
'blockip'                         => 'Benotzer spären',
'blockip-legend'                  => 'Benotzer spären',
'blockiptext'                     => 'Benotzt dës Form fir eng spezifesch IP Adress oder e Benotzernumm ze spären. Dëst soll nëmmen am Fall vu Vandalismus gemaach ginn, en accordance mat den [[{{MediaWiki:Policy-url}}|interne Richlinen]]. Gitt e spezifesche Grond un (zum Beispill Säite wou Vandalismus virgefall ass).',
'ipaddress'                       => 'IP-Adress oder Benotzernamm:',
'ipadressorusername'              => 'IP-Adress oder Benotzernumm:',
'ipbexpiry'                       => 'Gültegkeet:',
'ipbreason'                       => 'Grond:',
'ipbreasonotherlist'              => 'Anere Grond',
'ipbreason-dropdown'              => "*Heefeg Ursaache fir Benotzer ze spären:
**Bewosst falsch Informatiounen an een oder méi Säite gesat
**Ouni Grond Inhalt vun Säite geläscht
**Spam-Verknëppunge mat externe Säiten
**Topereien an d'Säite gesat
**Beleidegt oder bedréit aner Mataarbechter
**Mëssbrauch vu verschiddene Benotzernimm
**Net akzeptabele Benotzernumm",
'ipbanononly'                     => 'Nëmmen anonym Benotzer spären',
'ipbcreateaccount'                => 'Opmaache vun engem Benotzerkont verhënneren',
'ipbemailban'                     => 'Verhënneren datt de Benotzer E-Maile verschéckt',
'ipbenableautoblock'              => 'Automatesch déi lescht IP-Adress spären déi vun dësem Benotzer benotzt gouf, an all IP-Adressen vun denen dëse Benotzer versicht Ännerunge virzehuelen',
'ipbsubmit'                       => 'Dës IP-Adress resp dëse Benotzer spären',
'ipbother'                        => 'Aner Dauer:',
'ipboptions'                      => '1 Stonn:1 hour,2 Stonen:2 hours,6 Stonnen:6 hours,1 Dag:1 day,3 Deeg:3 days,1 Woch:1 week,2 Wochen:2 weeks,1 Mount:1 month,3 Méint:3 months,1 Joer:1 year,Onbegrenzt:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'Aner Dauer',
'ipbotherreason'                  => 'Aneren oder zousätzleche Grond:',
'ipbhidename'                     => 'Benotzernumm an der Spärlëscht, der Lëscht vun den aktive Spären an der Lëscht vun de Benotzer verstoppen',
'ipbwatchuser'                    => 'Dësem Benotzer seng Benotzer- an Diskussiouns-Säit iwwerwaachen',
'badipaddress'                    => "D'IP-Adress huet dat falscht Format.",
'blockipsuccesssub'               => 'Gouf gespaart',
'blockipsuccesstext'              => "[[Special:Contributions/$1|$1]] gouf gespaart. <br />

Kuckt d'[[Special:IPBlockList|IP Spär-Lëscht]] fir all Spären ze gesin.",
'ipb-edit-dropdown'               => 'Spärgrënn änneren',
'ipb-unblock-addr'                => 'Spär vum $1 annuléieren',
'ipb-unblock'                     => 'Spär vun enger IP-Adress oder engem Benotzer annuléieren',
'ipb-blocklist-addr'              => 'Kuckt aktuell Späre vum $1',
'ipb-blocklist'                   => 'Kuckt aktuell Spären',
'unblockip'                       => 'Spär annuléieren',
'unblockiptext'                   => 'Matt dësem Formulaire kënnt Dir enger IP-Adress oder engen Benotzer seng Spär ohiewen.',
'ipusubmit'                       => "D'Spär vun dëser Adress ophiewen",
'unblocked'                       => "D'Spär fir de(n) [[User:$1|$1]] gouf annulléiert",
'unblocked-id'                    => "D'Spär $1 gouf annulléiert",
'ipblocklist'                     => 'Lëscht vu gespaarten IP-Adressen a Benotzernimm',
'ipblocklist-legend'              => 'No engem gespaarte Benotzer sichen',
'ipblocklist-username'            => 'Benotzernumm oder IP-Adress:',
'ipblocklist-submit'              => 'Sichen',
'blocklistline'                   => '$1, $2 gespaart $3 (gülteg bis $4)',
'infiniteblock'                   => 'onbegrenzt',
'expiringblock'                   => 'bis $1',
'anononlyblock'                   => 'nëmmen anonym Benotzer',
'noautoblockblock'                => 'déi automatesch Spär ass deaktivéiert',
'createaccountblock'              => 'Opmaache vu Benotzerkonte gespaart',
'emailblock'                      => 'E-Maile schécke gespaart',
'ipblocklist-empty'               => "D'Spärlëscht ass eidel.",
'ipblocklist-no-results'          => 'Déi gesichten IP-Adress respektiv de gesichte Benotzer ass net gespaart.',
'blocklink'                       => 'spären',
'unblocklink'                     => 'Spär annulléieren',
'contribslink'                    => 'Kontributiounen',
'autoblocker'                     => 'Dir sidd automatesch gespaart well dir eng IP Adress mam "$1" deelt. Grond "$2".',
'blocklogpage'                    => 'Spärlëscht',
'blocklogentry'                   => '"[[$1]]" gespaart, gülteg bis $2 $3',
'blocklogtext'                    => "Dëst ass eng Lëscht vu Spären an den Annulatioune vun de Spären. Automatesch gespaarten IP Adresse sinn hei net opgelëscht. Kuckt d'[[Special:IPBlockList|IP Spärlëschtt]] fir déi aktuell Spären.",
'unblocklogentry'                 => "huet d'Spär vum [[$1]] annulléiert",
'block-log-flags-anononly'        => 'Nëmmen anonym Benotzer',
'block-log-flags-nocreate'        => 'Schafe vu Benotzerkonte gespaart',
'block-log-flags-noautoblock'     => 'Autoblock deaktivéiert',
'block-log-flags-noemail'         => 'E-Mail gespaart',
'block-log-flags-angry-autoblock' => 'erweidert automatesch Spär aktivéiert',
'range_block_disabled'            => 'Dem Administrateur seng Fähegkeet fir ganz Adressberäicher ze spären ass ausser Kraaft.',
'ipb_expiry_invalid'              => "D'Dauer déi Dir uginn hutt ass ongülteg.",
'ipb_expiry_temp'                 => 'Verstoppte Späre vu Benotzernimm solle permanent sinn.',
'ipb_already_blocked'             => '"$1" ass scho gespaart.',
'ipb_cant_unblock'                => "Feeler: D'Nummer vun der Spär $1 gouf net fonnt. D'Spär gouf waarscheinlech schonn opgehuewen.",
'ipb_blocked_as_range'            => "Feeler: D'IP-Adress $1 gouf net direkt gespaart an déi Spär kann dofir och net opghuee ginn.
Si ass awer als Deel vun der Rei $2 gespaart, an dës Spär kann opgehuewe ginn.",
'ip_range_invalid'                => 'Ongëltegen IP Block.',
'blockme'                         => 'Spär mech',
'proxyblocker'                    => 'Proxy blocker',
'proxyblocker-disabled'           => 'Dës Funktioun ass ausgeschalt.',
'proxyblockreason'                => 'Är IP-Adress gouf gespaart, well si een oppene Proxy ass. Kontaktéiert w.e.g. ären Internet-Provider oder ärs Systemadministrateuren und informéiert si iiwwer dëses méigleche Sécherheetsprobleem.',
'proxyblocksuccess'               => 'Gemaach.',
'sorbsreason'                     => 'Är IP Adress steet als oppene Proxy an der schwaarzer Lëscht (DNSBL) déi vu {{SITENAME}} benotzt gëtt.',
'sorbs_create_account_reason'     => 'Är IP-Adress steet als oppene Proxy an der schwaarzer Lëscht déi op {{SITENAME}} benotzt gëtt. DIr kënnt keen neie Benotzerkont opmaachen.',

# Developer tools
'lockdb'              => 'Datebank spären',
'unlockdb'            => 'Spär vun der Datebank ophiewen',
'lockdbtext'          => "Wann d'Datebank gespaart ass, ka kee Benotzer Säiten änneren, seng Astellungen änneren, seng Iwwerwaachungslëscht änneren, an all aner Aarbecht, déi op d'Datebank zréckgräift.
W.e.g. konfirméiert, datt dir dëst wierklech maache wëllt, an datt dir d'Spär ewechhuelt soubal d'Maintenance-Aarbechten eriwwer sinn.",
'unlockdbtext'        => "D'Ophiewe vun der Spär vun der Datebank léisst et erëm zou datt all Benotzer Säiten änneren, hir Astellungen an hir Iwwerwaachungslëscht veränneren an all aner Operatiounen déi Ännerungen an der Datebank erfuederen.

Confirméiert w.e.g datt et dat ass wat Dir maache wëllt.",
'lockconfirm'         => "Jo, ech wëll d'Datebank wierklech spären.",
'unlockconfirm'       => "Jo, ech well d'Spär vun der Datebank wirklech ophiewen.",
'lockbtn'             => 'Datebank spären',
'unlockbtn'           => 'Spär vun der Datebank ophiewen',
'locknoconfirm'       => "Dir hutt d'Konfirmatiounsbox net ugeklickt.",
'lockdbsuccesssub'    => "D'Datebank ass elo gespaart",
'unlockdbsuccesssub'  => "D'Spär vun der Datebank gouf opgehuewen",
'lockdbsuccesstext'   => "D'{{SITENAME}}-Datebank gouf gespaart. <br />
Denkt drun [[Special:UnlockDB|d'Spär erëm ewechzehuele]] soubaal d'Maintenance-Aarbechte fäerdeg sinn.",
'unlockdbsuccesstext' => "D'Spär vun der Datebank ass opgehuewen.",
'databasenotlocked'   => "D'Datebank ass net gespaart.",

# Move page
'move-page'               => 'Réckel $1',
'move-page-legend'        => 'Säit réckelen',
'movepagetext'            => "Wann dir dëse Formulaire benotzt, réckelt dir eng komplett Säit mat hirem Historique op en neien Numm.
Den alen Titel déngt als Viruleedung op déi nei Säit.
Dir kënnt Viruleedungen op déi al Säit ginn automatesch aktualiséieren.
Wann Dir dat net maacht, da vergewëssert iech datt keng [[Special:DoubleRedirects|duebel]] oder [[Special:BrokenRedirects|futtis Viruleedungen]] am Spill sinn.
Dir sidd responsabel datt d'Linke weiderhinn dohinner pointéieren, wou se hi sollen.

Beuecht w.e.g. datt d'Säit '''net''' geréckelt gëtt, wann ët schonns eng Säit mat deem Titel gëtt, ausser dës ass eidel, ass eng Viruleedung oder huet keen Historique.
Dëst bedeit datt dir eng Säit ëmbenenne kënnt an datt dir keng Säit iwwerschreiwe kënnt, déi et schonns gëtt.

'''OPGEPASST!'''
Dëst kann en drastesche Changement fir eng populär Säit bedeiten;
verstitt w.e.g. d'Konsequenze vun ärer Handlung éier Dir d'Säit réckelt.",
'movepagetalktext'        => "D'assoziéiert Diskussiounssäit, falls eng do ass, gëtt automatesch matgeréckelt, '''ausser:'''
*D'Säit gëtt an een anere Nummraum geréckelt.
*Et gëtt schonn eng Diskussiounssäit mat dësem Numm, oder
*Dir klickt d'Këschtchen ënnedrënner net un.

An deene Fäll musst Dir d'Diskussiounssäit manuell réckelen oder fusionéieren.",
'movearticle'             => 'Säit réckelen:',
'movenotallowed'          => 'Dir hutt net déi néideg Rechter fir Säiten ze réckelen.',
'newtitle'                => 'Op den neien Titel:',
'move-watch'              => 'Dës Säit iwwerwaachen',
'movepagebtn'             => 'Säit réckelen',
'pagemovedsub'            => 'Gouf geréckelt',
'movepage-moved'          => "<big>'''D'Säit \"\$1\" gouf op \"\$2\" geréckelt.'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Eng Säit mat dësem Numm gëtt et schonns, oder den Numm deen Dir gewielt hutt gëtt net akzeptéiert. Wielt w.e.g. en aneren Numm.',
'cantmove-titleprotected' => "Dir kënnt keng Säit op dës Plaz réckelen, well dee neien Titel fir d'Uleeë gespaart ass",
'talkexists'              => "D'Säit selwer gouf erfollegräich geréckelt, mee d'Diskussiounssäit konnt net mat eriwwergeholl gi well et schonns eng ënnert deem neien Titel gëtt. W.e.g. setzt dës manuell zesummen.",
'movedto'                 => 'geréckelt op',
'movetalk'                => "D'Diskussiounssäit matréckelen, wa méiglich.",
'move-subpages'           => 'All ënnersäite, wann et der gëtt, mattréckelen',
'move-talk-subpages'      => 'All Ënnersäite vun Diskussiounssäiten, wann et der gëtt, matréckelen',
'movepage-page-exists'    => "D'Säit $1 gëtt et schonn a kann net automatesch iwwerschriwwe ginn.",
'movepage-page-moved'     => "D'Säit $1 gouf schonn op $2 geréckelt.",
'movepage-page-unmoved'   => "D'Säit $1 konnt nett op $2 geréckelt ginn.",
'movepage-max-pages'      => 'Déi Maximalzuel vun $1 {{PLURAL:$1|Säit gouf|Säite goufe}} gouf geréckelt. All déi aner Säite kënnen net automatesch geréckelt ginn.',
'1movedto2'               => '[[$1]] gouf op [[$2]] geréckelt',
'1movedto2_redir'         => '[[$1]] gouf op [[$2]] geréckelt, dobäi gouf eng Viruleedung iwwerschriwwen.',
'movelogpage'             => 'Réckellëscht',
'movelogpagetext'         => 'Dëst ass eng Lëscht vun alle geréckelte Säiten.',
'movereason'              => 'Grond:',
'revertmove'              => 'zréck réckelen',
'delete_and_move'         => 'Läschen a réckelen',
'delete_and_move_text'    => '== Läsche vun der Destinatiounssäit néideg == D\'Säit "[[:$1]]" existéiert schonn. Wëll der se läsche fir d\'Réckelen ze erméiglechen?',
'delete_and_move_confirm' => "Jo, läsch d'Destinatiounssäit",
'delete_and_move_reason'  => 'Geläscht fir Plaz ze maache fir eng Säit heihin ze réckelen',
'selfmove'                => 'Source- an Destinatiounsnumm sinn dselwecht; eng Säit kann net op sech selwer geréckelt ginn.',
'imagenocrossnamespace'   => 'Fichiere kënnen net an aner Nummraim geréckelt ginn',
'imagetypemismatch'       => 'Déi nei Dateierweiderung ass net mat dem Fichier kompatibel',
'imageinvalidfilename'    => 'Den Numm vum Zil-Fichier ass ongëlteg',
'fix-double-redirects'    => 'All Viruleedungen déi op den Originaltitel weisen aktualiséieren',

# Export
'export'            => 'Säiten exportéieren',
'exporttext'        => "Dir kënnt den Text exportéieren an den Historique änneren vun enger bestëmmter Säit, oder engem Set vu Säiten, an XML agepakt.
Dat dann an eng aner Wiki mat MediaWiki Software impotéiert gi mat Hellef vun der [[Special:Import|Import-Säit]].

Fir eng Säit z'exportéieren, gitt den Titel an d'Textkëscht heidrënner an, een Titel pro Linn, a wielt aus op Dir nëmmen déi aktuell Versioun oder all Versioune mam ganzen Historique exportéiere wëllt.

Wann nëmmen déi aktuell Versioun exportéiert soll ginn, kënnt Dir och e Link benotze wéi z.B [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] fir d'\"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => 'Nëmmen déi aktuell Versioun exportéieren an net de ganzen Historique',
'exportnohistory'   => "----
'''Hiwäis:''' Den Export vu komplette Versiounshistoriquen ass aus Performancegrënn bis op weideres net méiglech.",
'export-submit'     => 'Exportéieren',
'export-addcattext' => 'Säiten aus Kategorie derbäisetzen:',
'export-addcat'     => 'Derbäisetzen',
'export-download'   => 'Als XML-Datei späicheren',
'export-templates'  => 'Inklusiv Schablounen',

# Namespace 8 related
'allmessages'               => 'All Systemmessagen',
'allmessagesname'           => 'Numm',
'allmessagesdefault'        => 'Standardtext',
'allmessagescurrent'        => 'Aktuellen Text',
'allmessagestext'           => "Dëst ass eng Lëscht vun alle '''Messagen am MediaWiki:Nummraum, déi vun der MediaWiki-Software benotzt ginn.
Besicht w.e.g. [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisatioun] a [http://translatewiki.net Betawiki] wann Dir wëllt bei de MediaWiki Iwwersetzunge matschaffen.",
'allmessagesnotsupportedDB' => "Dës Säit kann net benotzt gi well '''\$wgUseDatabaseMessages''' ausgeschalt ass.",
'allmessagesfilter'         => 'Noriichtennummfilter:',
'allmessagesmodified'       => 'Nëmme geännerter weisen',

# Thumbnails
'thumbnail-more'           => 'vergréisseren',
'filemissing'              => 'Fichier feelt',
'thumbnail_error'          => 'Feeler beim Erstellen vum Thumbnail vun: $1',
'djvu_page_error'          => 'DjVu-Säit baussent dem Säiteberäich',
'thumbnail_invalid_params' => 'Ongëlteg Thumbnail-Parameter',

# Special:Import
'import'                     => 'Säiten importéieren',
'importinterwiki'            => 'Transwiki-Import',
'import-interwiki-history'   => "Importéier all d'Versioune vun dëser Säit",
'import-interwiki-submit'    => 'Import',
'import-interwiki-namespace' => 'Kopéier Säiten an den Nummraum:',
'importtext'                 => 'Exportéiert de Fichier w.e.g vun der Source-Wiki mat der [[Special:Export|Funktioun Export]].
Späichert en op ärem Computer of a lued en hei nees erop.',
'importstart'                => 'Importéier Säiten …',
'import-revision-count'      => '$1 {{PLURAL:$1|Versioun|Versiounen}}',
'importnopages'              => "Et gëtt keng Säiten fir z'importéieren.",
'importfailed'               => 'Importatioun huet net fonctionnéiert: $1',
'importunknownsource'        => 'Onbekannt Importquell',
'importcantopen'             => 'De Fichier dee sollt importéiert gi konnt net opgemaach ginn',
'importbadinterwiki'         => 'Falschen Interwiki-Link',
'importnotext'               => 'Eidel oder keen Text',
'importsuccess'              => 'Den Import ass fäerdeg!',
'importhistoryconflict'      => 'Et gëtt Konflikter am Historique vun de Versionen, (méiglecherweis gouf dës Säit virdrun importéiert).',
'importnosources'            => 'Fir den Transwiki-Import si keng Quellen definéiert an et ass net méiglech fir Säite mat alle Versiounen aus dem Transwiki-Tëschespäicher eropzelueden.',
'importnofile'               => 'Et gouf keen importéierte Fichier eropgelueden',
'importuploaderrorsize'      => 'DEropluede vum importéierte Fichier huet net fonctionnéiert. De Fichier ass méi grouss wéi maximal erlaabt.',
'importuploaderrorpartial'   => "D'Eropluede vum Fichier huet net geklappt. De Fichier gouf nëmmen deelweis eropgelueden.",
'importuploaderrortemp'      => "D'Eropluede vum Fichier huet net fonctionnéiert. En temporäre Repertoire feelt.",
'import-parse-failure'       => 'Feeler bei engem XML-Import',
'import-noarticle'           => "Keng Säit fir z'importéieren!",
'import-nonewrevisions'      => "All d'Versioune goufe scho virdrunn importéiert.",
'xml-error-string'           => '$1 an der Zeil $2, Spalt $3, (Byte $4): $5',
'import-upload'              => 'XML-Daten importéieren',

# Import log
'importlogpage'                    => 'Lëscht vun den Säitenimporten',
'importlogpagetext'                => 'Administrativen Import vu Säite matt dem Historique vun de Ännerungen aus anere Wikien.',
'import-logentry-upload'           => 'huet [[$1]] vun engem Fichier duerch eroplueden importéiert',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|Versioun|Versiounen}}',
'import-logentry-interwiki'        => 'huet $1 importéiert (Transwiki)',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|Versioun|Versioune}} vum $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Meng Benotzersäit',
'tooltip-pt-anonuserpage'         => 'Benotzersäit vun der IP-Adress vun där aus Dir den Ament Ännerunge maachtt',
'tooltip-pt-mytalk'               => 'Meng Diskussioun',
'tooltip-pt-anontalk'             => "Diskussioun iwwer d'Ännerungen déi vun dëser IP-Adress aus gemaach gi sinn",
'tooltip-pt-preferences'          => 'Meng Astellungen',
'tooltip-pt-watchlist'            => 'Lëscht vu Säiten, bei deenen Der op Ännerungen oppasst',
'tooltip-pt-mycontris'            => 'Lëscht vu menge Kontributiounen',
'tooltip-pt-login'                => 'Sech unzemellen gëtt gäre gesinn, Dir musst et awer net maachen.',
'tooltip-pt-anonlogin'            => 'Et wier gutt, Dir géift Iech aloggen, och wann et keng Musse-Saach ass.',
'tooltip-pt-logout'               => 'Ofmellen',
'tooltip-ca-talk'                 => 'Diskussioun iwwert de Säiteninhalt',
'tooltip-ca-edit'                 => 'Dës Säit ka geännert ginn. Maacht vum Preview Gebrauch a kuckt ob alles an der Rei ass ier der ofspäichert.',
'tooltip-ca-addsection'           => 'Eng Bemierkung bäi dës Diskussioun derbäisetzen.',
'tooltip-ca-viewsource'           => 'Dës Säit ass protegéiert. Nëmmen de Quelltext ka gewise ginn.',
'tooltip-ca-history'              => 'Vireg Versioune vun dëser Säit',
'tooltip-ca-protect'              => 'Dës Säit protegéieren',
'tooltip-ca-delete'               => 'Dës Säit läschen',
'tooltip-ca-undelete'             => 'Dës Säit restauréieren',
'tooltip-ca-move'                 => 'Dës Säit réckelen',
'tooltip-ca-watch'                => 'Dës Säit op är Iwwerwaachungslëscht bäisetzen',
'tooltip-ca-unwatch'              => 'Dës Säit vun der Iwwerwaachungslëscht erofhuelen',
'tooltip-search'                  => 'Op {{SITENAME}} sichen',
'tooltip-search-go'               => 'Direkt op genee déi Säit goen, wann et se gëtt.',
'tooltip-search-fulltext'         => 'No Säite sichen, wou dësen Text drann ass',
'tooltip-p-logo'                  => 'Haaptsäit',
'tooltip-n-mainpage'              => 'Eis Entréesdier',
'tooltip-n-portal'                => 'Iwwer de Portal, wat Dir maache kënnt, wou wat ze fannen ass',
'tooltip-n-currentevents'         => "D'Aktualitéit a wat dohannert ass",
'tooltip-n-recentchanges'         => 'Lëscht vun de rezenten Ännerungen op {{SITENAME}}.',
'tooltip-n-randompage'            => 'Zoufälleg Säit',
'tooltip-n-help'                  => 'Hëllefsäiten weisen.',
'tooltip-t-whatlinkshere'         => 'Lëscht vun alle Säiten, déi heihi linken',
'tooltip-t-recentchangeslinked'   => 'Rezent Ännerungen op Säiten, déi von hei verlinkt sinn',
'tooltip-feed-rss'                => 'RSS-Feed fir dës Säit',
'tooltip-feed-atom'               => 'Atom-Feed fir dës Säit',
'tooltip-t-contributions'         => 'Lëscht vun de Kontributiounen vun dësem Benotzer',
'tooltip-t-emailuser'             => 'Dësem Benotzer eng E-Mail schécken',
'tooltip-t-upload'                => 'Biller oder Mediefichieren eroplueden',
'tooltip-t-specialpages'          => 'Lëscht vun alle Spezialsäiten',
'tooltip-t-print'                 => 'Versioun vun dëser Säit fir auszedrécken',
'tooltip-t-permalink'             => 'Permanente Link op dës Versioun vun dëser Säit',
'tooltip-ca-nstab-main'           => 'Contenu vun der Säit weisen',
'tooltip-ca-nstab-user'           => 'Benotzersäit weisen',
'tooltip-ca-nstab-media'          => 'Mediesäit weisen',
'tooltip-ca-nstab-special'        => 'Dëst ass eng Spezialsäit. Si kann net geännert ginn.',
'tooltip-ca-nstab-project'        => 'Portalsäit weisen',
'tooltip-ca-nstab-image'          => 'Billersäit weisen',
'tooltip-ca-nstab-mediawiki'      => 'Systemmessage weisen',
'tooltip-ca-nstab-template'       => 'Schabloun weisen',
'tooltip-ca-nstab-help'           => 'Hëllefesäite weisen',
'tooltip-ca-nstab-category'       => 'Kategoriesäit weisen',
'tooltip-minoredit'               => 'Dës Ännerung als kleng markéieren.',
'tooltip-save'                    => 'Ännerungen späicheren',
'tooltip-preview'                 => "Klickt op 'Preview' éier Der späichert!",
'tooltip-diff'                    => 'Weis wéi eng Ännerungen der beim Text gemaach hutt.',
'tooltip-compareselectedversions' => "D'Ennerscheeder op dëser Säit tëscht den zwou gewielte Versioune weisen.",
'tooltip-watch'                   => 'Dës Säit op är Iwwerwaachungslëscht bäisetzen',
'tooltip-recreate'                => "D'Säit nees maachen, obwuel se geläscht gi war.",
'tooltip-upload'                  => 'Mam eroplueden ufänken',

# Metadata
'nodublincore'      => 'Dublin Core RDF Metadata ass op dësem Server ausgeschalt.',
'nocreativecommons' => 'Creative Commons RDF Metadata ass op dësem Server ausgeschalt.',
'notacceptable'     => "De Wiki-Server kann d'Donnéeë net an engem Format liwweren déi vun ärem Apparat geliest kënne ginn.",

# Attribution
'anonymous'        => 'Anonym(e) Benotzer op {{SITENAME}}',
'siteuser'         => '{{SITENAME}}-Benotzer $1',
'lastmodifiedatby' => "Dës Säit gouf den $1 ëm $2 Auer voum $3 fir d'lescht geännert.", # $1 date, $2 time, $3 user
'othercontribs'    => 'Op der Basis vun der Aarbecht vum $1',
'others'           => 'anerer',
'siteusers'        => '{{SITENAME}}-Benotzer $1',
'creditspage'      => 'Quellen',
'nocredits'        => "Fir dës Säit si keng Informatiounen iwwert d'Mataarbechter vun der Säit disponibel.",

# Spam protection
'spamprotectiontitle' => 'Spamfilter',
'spamprotectiontext'  => "D'Säit déi dir späichere wollt gouf vum Spamfilter gespaart. 
Dëst warscheinlech duerch en externe Link den op der schwaarzer Lëscht (blacklist) vun den externe Säite steet.",
'spamprotectionmatch' => "'''Dësen Text gouf vum Spamfilter fonnt: ''$1'''''",
'spambot_username'    => 'Botz vum Spam duerch MediaWiki',
'spam_reverting'      => 'Déi lescht Versioun ouni Linken op $1 restauréieren.',
'spam_blanking'       => 'An alle Versioune ware Linken op $1, et ass elo alles gebotzt.',

# Info page
'infosubtitle'   => 'Informatioun zur Säit',
'numedits'       => 'Zuel vun den Ännerunge vun dëser Säit: $1',
'numtalkedits'   => 'Zuel vun den Ännerungen (Diskussiounssäit): $1',
'numwatchers'    => 'Zuel vun de Benotzer déi dës Säit iwwerwaachen: $1',
'numauthors'     => 'Zuel vu verschiddenen Auteuren: $1',
'numtalkauthors' => 'Zuel vun den Auteuren (Diskussiounssäit): $1',

# Math options
'mw_math_png'    => 'Ëmmer als PNG duerstellen',
'mw_math_simple' => 'Einfachen TeX als HTML duerstellen, soss PNG',
'mw_math_html'   => 'Wa méiglech als HTML duerstellen, soss PNG',
'mw_math_source' => 'Als TeX loossen (fir Textbrowser)',
'mw_math_modern' => 'Recommandéiert fir modern Browser',

# Patrolling
'markaspatrolleddiff'        => 'Als kontrolléiert markéieren',
'markaspatrolledtext'        => 'Dës Säit als kontrolléiert markéieren',
'markedaspatrolled'          => 'ass als kontrolléiert markéiert',
'markedaspatrolledtext'      => 'Déi gewielte Versioun gouf als kontrolléiert markéiert.',
'markedaspatrollederror'     => 'Kann net als "kontrolléiert" markéiert ginn.',
'markedaspatrollederrortext' => 'Dir musst eng Säitenännerung auswielen.',

# Patrol log
'patrol-log-page' => 'Kontroll-Logbuch',
'patrol-log-auto' => '(automatesch)',

# Image deletion
'deletedrevision'                 => 'Al, geläschte Versioun $1',
'filedeleteerror-short'           => 'Feeler beim Läsche vum Fichier: $1',
'filedeleteerror-long'            => 'Bäim Läsche vum Fichier si Feeler festgestallt ginn:

$1',
'filedelete-missing'              => 'De Fichier "$1" kann net geläscht ginn, well et e net gëtt.',
'filedelete-old-unregistered'     => 'Déi Versioun vum Fichier déi Dir uginn hutt "$1" gëtt et an der Datebank net.',
'filedelete-current-unregistered' => 'Dee Fichier "$1" ass net an der Datebank.',

# Browsing diffs
'previousdiff' => '← Méi al Ännerung',
'nextdiff'     => 'Méi nei Ännerung →',

# Media information
'imagemaxsize'         => 'Biller op de Billerbeschreiwungssäite limitéieren op:',
'thumbsize'            => 'Gréisst vun de Thumbnails:',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|Säit|Säiten}}',
'file-info'            => '(Dateigréisst: $1, MIME-Typ: $2)',
'file-info-size'       => '($1 × $2 Pixel, Dateigréisst: $3, MIME-Typ: $4)',
'file-nohires'         => '<small>Et gëtt keng méi eng héich Opléisung.</small>',
'svg-long-desc'        => '(SVG-Fichier, Basisgréisst: $1 × $2 Pixel, Gréisst vum Fichier: $3)',
'show-big-image'       => 'Versioun an enger méi héijer Opléisung',
'show-big-image-thumb' => '<small>Gréisst vun dem Thumbnail: $1 × $2 Pixel</small>',

# Special:NewImages
'newimages'             => 'Gallerie vun de neie Biller',
'imagelisttext'         => "Hei ass eng Lëscht vun '''$1''' {{PLURAL:$1|Fichier|Fichieren}}, zortéiert $2.",
'newimages-summary'     => 'Dës Spezialsäit weist eng Lëscht mat de Biller a Fichieren déi als läscht eropgeluede goufen.',
'showhidebots'          => '($1 Botten)',
'noimages'              => 'Keng Biller fonnt.',
'ilsubmit'              => 'Sichen',
'bydate'                => 'no Datum',
'sp-newimages-showfrom' => 'Nei Biller weisen, ugefaang den $1 ëm $2',

# Bad image list
'bad_image_list' => 'Format:

Nëmmen Zeilen, déi mat engem * ufénken, ginn ausgewert. Als éischt no dem * muss ee Link op een net gewëschte Bild stoen.
Duerno sti Linken déi Ausnamen definéieren, a deenen hirem Kontext dat Bild awer opdauchen däerf.',

# Metadata
'metadata'          => 'Metadaten',
'metadata-help'     => 'An dësem Fichier si weider Informatiounen, déi normalerweis vun der Digitalkamera oder dem benotzte Scanner kommen. Wann de Fichier nodréilech geännert gouf, kann et sinn datt eenzel Detailer net mat dem aktuelle Fichier iwwereestëmmen.',
'metadata-expand'   => 'Weis detailléiert Informatiounen',
'metadata-collapse' => 'Verstopp detailléiert Informatiounen',
'metadata-fields'   => "Dës Felder vun den EXIF-Metadate ginn op Bildbeschreiwungssäite gewisen wann d'Metadatentafel zesummegeklappt ass. Déi aner sinn am Standard verstoppt, kënne awer ugewise ginn.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Breet',
'exif-imagelength'                 => 'Längt',
'exif-bitspersample'               => 'Bite pro Faarfkomponent',
'exif-compression'                 => 'Aart vun der Kompressioun',
'exif-photometricinterpretation'   => 'Pixelzesummesetzung',
'exif-orientation'                 => 'Kameraausriichtung',
'exif-samplesperpixel'             => 'Zuel vun de Komponenten',
'exif-planarconfiguration'         => 'Datenausriichtung',
'exif-ycbcrpositioning'            => 'Y an C Positionéierung',
'exif-xresolution'                 => 'Horizontal Opléisung',
'exif-yresolution'                 => 'Vertikal Opléisung',
'exif-resolutionunit'              => 'Moosseenheet vun der Opléisung',
'exif-stripoffsets'                => 'Plaz wou de Fichier vum Bild gespäichert ass',
'exif-rowsperstrip'                => 'Zuel vun den Zeile pro Stréif',
'exif-stripbytecounts'             => 'Byte pro kompriméiert Strëpp',
'exif-jpeginterchangeformatlength' => 'Gréisst vun de JPEG-Daten a Byten',
'exif-transferfunction'            => 'Transferfunktioun',
'exif-whitepoint'                  => 'Manuell mat Miessung',
'exif-referenceblackwhite'         => 'Schwoarz/Wäiss-Referenzpunkten',
'exif-datetime'                    => 'Späicherzäitpunkt',
'exif-imagedescription'            => 'Numm vum Bild',
'exif-make'                        => 'Fabrikant',
'exif-model'                       => 'Modell',
'exif-software'                    => 'Benotzte Software',
'exif-artist'                      => 'Auteur',
'exif-copyright'                   => "Droits d'auteur",
'exif-exifversion'                 => 'Exif-Versioun',
'exif-flashpixversion'             => 'Ënnerstëtzte Flashpix-Versioun',
'exif-colorspace'                  => 'Faarfraum',
'exif-componentsconfiguration'     => 'Bedeitung vun eenzelne Komponenten',
'exif-compressedbitsperpixel'      => 'Kompriméiert Bite pro Pixel',
'exif-pixelydimension'             => 'Gültëg Bildbreet',
'exif-pixelxdimension'             => 'Gültëg Bildhéicht',
'exif-makernote'                   => 'Notize vum Fabrikant',
'exif-usercomment'                 => 'Bemierkunge vum Benotzer',
'exif-relatedsoundfile'            => 'Tounfichier deen dozou gehéiert',
'exif-datetimeoriginal'            => 'Erfaassungszäitpunkt',
'exif-datetimedigitized'           => 'Digitaliséierungszäitpunkt',
'exif-subsectime'                  => 'Späicherzäitpunkt (1/100 s)',
'exif-subsectimeoriginal'          => 'Erfaassungszäitpunkt (1/100 s)',
'exif-subsectimedigitized'         => 'Digitaliséirungszäitpunkt (1/100 s)',
'exif-exposuretime'                => 'Beliichtungsdauer',
'exif-exposuretime-format'         => '$1 Sekonnen ($2)',
'exif-fnumber'                     => 'Blend',
'exif-exposureprogram'             => 'Beliichtungsprogramm',
'exif-isospeedratings'             => 'Film- oder Sensorempfindlechkeet (ISO)',
'exif-oecf'                        => 'Optoelektroneschen Ëmrechnungsfakteur',
'exif-shutterspeedvalue'           => 'Beliichtungszäitwäert',
'exif-aperturevalue'               => 'Blendewäert',
'exif-brightnessvalue'             => 'Hellegkeetswäert',
'exif-exposurebiasvalue'           => 'Beliichtungsvirgab',
'exif-maxaperturevalue'            => 'Gréisste Blend',
'exif-subjectdistance'             => 'Distanz zum Sujet',
'exif-meteringmode'                => 'Miessmethod',
'exif-lightsource'                 => 'Liichtquell',
'exif-flash'                       => 'Blëtz',
'exif-focallength'                 => 'Brennwäit',
'exif-subjectarea'                 => 'Beräich',
'exif-flashenergy'                 => 'Blëtzstäerkt',
'exif-focalplanexresolution'       => 'Sensoropléisung horizontal',
'exif-focalplaneyresolution'       => 'Sensoropléisung vertikal',
'exif-focalplaneresolutionunit'    => 'Eenheet vun der Sensoropléisung',
'exif-subjectlocation'             => 'Motivstanduert',
'exif-exposureindex'               => 'Beliichtungsindex',
'exif-sensingmethod'               => 'Miessmethod',
'exif-filesource'                  => 'Quell vum Fichier',
'exif-customrendered'              => 'Benotzerdefinéiert Bildveraarbechtung',
'exif-exposuremode'                => 'Beliichtungsmodus',
'exif-whitebalance'                => 'Wäissofgläich',
'exif-digitalzoomratio'            => 'Digitalzoom',
'exif-focallengthin35mmfilm'       => 'Brennwäit (Klengbildäquivalent)',
'exif-scenecapturetype'            => 'Aart vun der Opnam',
'exif-gaincontrol'                 => 'Verstäerkung',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Saturatioun',
'exif-sharpness'                   => 'Schäerft',
'exif-devicesettingdescription'    => 'Beschreiwung vun den Astellunge vum Apparat',
'exif-subjectdistancerange'        => 'Motivdistanz',
'exif-imageuniqueid'               => 'Bild-ID',
'exif-gpsversionid'                => 'Versioun vum GPS-Tag',
'exif-gpslatituderef'              => 'nördlech oder südlech Breet',
'exif-gpslatitude'                 => 'Geografesch Breet',
'exif-gpslongituderef'             => 'östlech oder westlech geografesch Längt',
'exif-gpslongitude'                => 'Geografesch Längt',
'exif-gpsaltituderef'              => 'Referenzhéicht',
'exif-gpsaltitude'                 => 'Héicht',
'exif-gpstimestamp'                => 'GPS-Zäit',
'exif-gpssatellites'               => "Satelitten déi fir d'Moosse benotzt goufen",
'exif-gpsmeasuremode'              => 'Moossmethod',
'exif-gpsdop'                      => 'Prezisioun vun der Miessung',
'exif-gpsspeedref'                 => 'Eenheet vun der Vitesse',
'exif-gpsspeed'                    => 'Vitesse vum GPS-Empfänger',
'exif-gpstrack'                    => 'Bewegungsrichtung',
'exif-gpsimgdirection'             => 'Bildrichtung',
'exif-gpsdestlatituderef'          => "Referenz fir d'Breet",
'exif-gpsdestlatitude'             => 'Breet',
'exif-gpsdestlongituderef'         => "Referenz fir d'Längt",
'exif-gpsdestlongitude'            => 'Längt',
'exif-gpsdestdistanceref'          => "Referenz fir d'Distanz bis bäi den Objet (vun der Foto)",
'exif-gpsdestdistance'             => 'Motivdistanz',
'exif-gpsprocessingmethod'         => 'Numm vun der GPS-Prozedur-Method',
'exif-gpsareainformation'          => 'Numm vun der GPS-Géigend',
'exif-gpsdatestamp'                => 'GPS-Datum',
'exif-gpsdifferential'             => 'GPS-Differentialverbesserung',

# EXIF attributes
'exif-compression-1' => 'Onkompriméiert',

'exif-unknowndate' => 'Onbekannten Datum',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Horizontal gedréit', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Ëm 180° gedréit', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Vertikal gedréit', # 0th row: bottom; 0th column: left
'exif-orientation-6' => "Ëm 90° an d'Richtung vun den Zäre vun der Auer gedréint", # 0th row: right; 0th column: top
'exif-orientation-8' => "Ëm 90° géint d'Richtung vun den Zäre vun der Auer gedréint", # 0th row: left; 0th column: bottom

'exif-componentsconfiguration-0' => 'Gëtt et net',

'exif-exposureprogram-0' => 'Onbekannt',
'exif-exposureprogram-1' => 'Manuell',
'exif-exposureprogram-2' => 'Standardprogramm',
'exif-exposureprogram-3' => 'Zäitautomatik',
'exif-exposureprogram-4' => 'Blendenautomatik',
'exif-exposureprogram-8' => 'Landschaftsopnamen',

'exif-subjectdistance-value' => '$1 Meter',

'exif-meteringmode-0'   => 'Onbekannt',
'exif-meteringmode-1'   => 'Duerchschnëttlech',
'exif-meteringmode-3'   => 'Spotmiessung',
'exif-meteringmode-4'   => 'Méifachspotmiessung',
'exif-meteringmode-5'   => 'Modell',
'exif-meteringmode-6'   => 'Bilddeel',
'exif-meteringmode-255' => 'Onbekannt',

'exif-lightsource-0'   => 'Onbekannt',
'exif-lightsource-1'   => 'Dageslut',
'exif-lightsource-2'   => 'Fluoreszéierend',
'exif-lightsource-4'   => 'Blëtz',
'exif-lightsource-9'   => 'Schéint Wieder',
'exif-lightsource-10'  => 'Wollekeg',
'exif-lightsource-11'  => 'Schiet',
'exif-lightsource-17'  => 'Standardluucht A',
'exif-lightsource-18'  => 'Standardluucht B',
'exif-lightsource-19'  => 'Standardluucht C',
'exif-lightsource-255' => 'Aner Liichtquell',

'exif-focalplaneresolutionunit-2' => 'Zoll/Inchen',

'exif-sensingmethod-1' => 'Ondefinéiert',
'exif-sensingmethod-2' => 'Een-Chip-Farfsensor',
'exif-sensingmethod-3' => 'Zwee-Chip-Farfsensor',
'exif-sensingmethod-4' => 'Dräi-Chip-Farfsensor',
'exif-sensingmethod-7' => 'Trilineare Sensor',

'exif-scenetype-1' => "D'Bild gouf photograféiert",

'exif-customrendered-0' => 'Standard',
'exif-customrendered-1' => 'Benotzerdefinéiert',

'exif-exposuremode-0' => 'Automatesch Beliichtung',
'exif-exposuremode-1' => 'Manuell Beliichtung',
'exif-exposuremode-2' => 'Beliichtungsserie',

'exif-whitebalance-0' => 'Automatesche Wäissofgläich',
'exif-whitebalance-1' => 'Manuelle Wäissofgläich',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Landschaft',
'exif-scenecapturetype-2' => 'Portrait',
'exif-scenecapturetype-3' => 'Nuetszeen',

'exif-gaincontrol-0' => 'Keng',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Schwaach',
'exif-contrast-2' => 'Stark',

'exif-saturation-0' => 'Normal',
'exif-saturation-2' => 'Héich',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Douce',
'exif-sharpness-2' => 'Stark',

'exif-subjectdistancerange-0' => 'Onbekannt',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'No',
'exif-subjectdistancerange-3' => 'wäit ewech',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'nërdlech Breet',
'exif-gpslatitude-s' => 'südlech Breet',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'ëstlech Längt',
'exif-gpslongitude-w' => 'westlech Längt',

'exif-gpsstatus-a' => 'Miessung am gaang',

'exif-gpsmeasuremode-2' => '2-dimensional Miessung',
'exif-gpsmeasuremode-3' => '3-dimensional Miessung',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometer pro Stonn',
'exif-gpsspeed-m' => 'Meile pro Stonn',
'exif-gpsspeed-n' => 'Kniet',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Tatsächlech Richtung',
'exif-gpsdirection-m' => 'Magnéitesch Richtung',

# External editor support
'edit-externally'      => 'Dëse Fichier mat engem externe Programm veränneren',
'edit-externally-help' => "<small>Fir gewuer ze gi wéi dat genee geet liest d'[http://www.mediawiki.org/wiki/Manual:External_editors Installatiounsinstruktiounen].</small>",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'all',
'imagelistall'     => 'alleguerten',
'watchlistall2'    => 'all',
'namespacesall'    => 'all',
'monthsall'        => 'all',

# E-mail address confirmation
'confirmemail'             => 'E-Mailadress confirméieren',
'confirmemail_noemail'     => 'Dir hutt keng gülteg E-Mail-Adress an äre [[Special:Preferences|Benotzerastellungen]] agedro.',
'confirmemail_text'        => "Ier der d'E-Mailfunktioune vun der {{SITENAME}} notze kënnt musst der als éischt är E-Mailadress confirméieren. Dréckt w.e.g. de Knäppchen hei ënnendrënner fir eng Confirmatiouns-E-Mail op déi Adress ze schécken déi der uginn hutt. An däer E-Mail steet e Link mat engem Code, deen der dann an ärem Browser opmaache musst fir esou ze bestätegen, datt är Adress och wierklech existéiert a valabel ass.",
'confirmemail_pending'     => '<div class="error">Dir kruet schon e Confirmatiouns-Code per E-Mail geschéckt. Wenn Dir äre Benotzerkont eréischt elo kuerz opgemaach huet, da gedëllegt iech nach  epuer Minutten bis ären E-Mail ukomm ass ier Dir een neie Code ufrot.</div>',
'confirmemail_send'        => 'Confirmatiouns-E-Mail schécken',
'confirmemail_sent'        => 'Confirmatiouns-E-Mail gouf geschéckt.',
'confirmemail_oncreate'    => "E Confirmatiouns-Code gouf op är E-Mail-Adress geschéckt.
Dëse Code gëtt fir d'Umeldung net gebraucht. Dir braucht en awer bäi der Aktivéierung vun den E-Mail-Funktiounen bannert der Wiki.",
'confirmemail_sendfailed'  => '{{SITENAME}} konnt är Confirmatiouns-E-Mail net schécken.
Iwwerpréift w.e.g. är E-Mailadress op ongëlteg Zeechen.

Feelermeldung vum Mailserver: $1',
'confirmemail_invalid'     => "Ongëltege Confirmatiounscode. Eventuell ass d'Gëltegkeetsdauer vum Code ofgelaf.",
'confirmemail_needlogin'   => 'Dir musst iech $1, fir är E-Mailadress ze confirméieren.',
'confirmemail_success'     => 'Är E-Mailadress gouf confirméiert. Där kënnt iech elo aloggen an a vollem Ëmfang vun der Wiki profitéieren.',
'confirmemail_loggedin'    => 'Är E-Mailadress gouf elo confirméiert.',
'confirmemail_error'       => 'Et ass eppes falsch gelaf bäim Späichere vun ärer Confirmatioun.',
'confirmemail_subject'     => 'Confirmatioun vun der E-Mailadress fir {{SITENAME}}',
'confirmemail_body'        => 'E Benotzer, waarscheinlech dir selwer, hutt mat der IP Adress $1 de Benotzerkont "$2" um Site {{SITENAME}} opgemaach. 

Fir ze bestätegen, datt dee Kont iech wierklech gehéiert a fir d\'E-Mail-Funktiounen um Site {{SITENAME}} z\'aktivéieren, maacht w.e.g. dëse Link an ärem Browser op:
$3

Wann dir dëse Benotzerkont *net* opgemaach huet, maacht w.e.g. dëse Link an ärem Browser op fir d\'E-Mailconfirmation z\'annulléieren:

$5

Sollt et sech net ëm äre Benotzerkont handelen, da maacht de Link *net* op. De Confirmatiounscode ass gëlteg bis de(n) $4.',
'confirmemail_invalidated' => 'Confirmatioun vun der E-Mailadress annulléiert',
'invalidateemail'          => "Annulléier d'E-Mailconfirmation",

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-Abannung ass ausgeschalt]',
'scarytranscludefailed'   => "[D'Siche no der Schabloun fir $1 huet net funktionéiert]",
'scarytranscludetoolong'  => "[D'URL ass ze laang]",

# Trackbacks
'trackbackremove' => '([$1 läschen])',
'trackbacklink'   => 'Zréckverfollegen',

# Delete conflict
'deletedwhileediting' => "'''Opgepasst''': Dës Säit gouf geläscht nodeems datt dir ugefaangen hutt se z'änneren!",
'confirmrecreate'     => "De Benotzer [[User:$1|$1]] ([[User talk:$1|Diskussioun]]) huet dës Säit geläscht, nodeems datt där ugefaangen hutt drun ze schaffen. D'Begrënnung war: ''$2'' Bestätegt w.e.g., datt Dir dës Säit wierklich erëm nei opmaache wëllt.",
'recreate'            => 'Erëm uleeën',

# HTML dump
'redirectingto' => 'Virugeleed op [[:$1]]',

# action=purge
'confirm_purge'        => 'Dës Säit aus dem Server-Cache läschen?

$1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "No Säite siche mat ''$1''.",
'searchnamed'      => "Sich no Säiten, an deenen hirem Numm ''$1'' virkënnt.",
'articletitles'    => "Säiten déi mat ''$1'' ufänken",
'hideresults'      => 'Verstopp',
'useajaxsearch'    => 'AJAX-ënnerstetzt Sich benotzen',

# Multipage image navigation
'imgmultipageprev' => '← Säit virdrun',
'imgmultipagenext' => 'nächst Säit →',
'imgmultigo'       => 'Lass',
'imgmultigoto'     => "Géi op d'Säit $1",

# Table pager
'ascending_abbrev'         => 'erop',
'descending_abbrev'        => 'erof',
'table_pager_next'         => 'Nächst Säit',
'table_pager_prev'         => 'Säit virdrun',
'table_pager_first'        => 'Éischt Säit',
'table_pager_last'         => 'Lescht Säit',
'table_pager_limit'        => '$1 Objete pro Säit weisen',
'table_pager_limit_submit' => 'Lass',
'table_pager_empty'        => 'Keng Resultater',

# Auto-summaries
'autosumm-blank'   => 'All Inhalt vun der Säit gëtt geläscht',
'autosumm-replace' => "Säit gëtt ersat duerch '$1'",
'autoredircomment' => 'Virugeleet op [[$1]]',
'autosumm-new'     => 'Nei Säit: $1',

# Live preview
'livepreview-loading' => 'Lueden …',
'livepreview-ready'   => 'Lueden … Fäerdeg!',
'livepreview-failed'  => "Live-Preview huet net fonctionéiert! Benotzt w.e.g. d'Fonctioun ''Kucken ouni ofzespäicheren''.",
'livepreview-error'   => 'Verbindung net méiglech: $1 „$2“. 
Benotzt w.e.g. de normale Preview (Kucken ouni ofzespäicheren).',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Ännerunge vun {{PLURAL:$1|der leschter Sekonn|de leschte(n) $1 Sekonnen}} kënne an dëser Lëscht net gewise ginn.',
'lag-warn-high'   => 'Duerch eng héich Serverbelaaschtung kënne Verännerungen déi viru manner wéi $1 {{PLURAL:$1|Sekonn|Sekonne}} gemaach goufen, net an dëser Lëscht ugewise ginn.',

# Watchlist editor
'watchlistedit-numitems'       => "Op Ärer Iwwerwaachungslëscht {{PLURAL:$1|steet 1 Säit|stinn $1 Säiten}}, ouni d'Diskussiounssäiten.",
'watchlistedit-noitems'        => 'Är Iwwerwaachungslëscht ass eidel.',
'watchlistedit-normal-title'   => 'Iwwerwaachungslëscht änneren',
'watchlistedit-normal-legend'  => 'Säite vun der Iwwerwaachungslëscht erofhuelen',
'watchlistedit-normal-explain' => 'D\'Säite vun ärer Iwwerwaachungslëscht ginn ënnendrenner gewisen.
Fir eng Säit erofzehuelen, klickt op d\'Haischen niewen drunn a klickt duerno op "Säiten erofhuelen".
Dir kënnt och [[Special:Watchlist/raw|déi net formatéiert Iwwerwaachungslëscht änneren]].',
'watchlistedit-normal-submit'  => 'Säiten erofhuelen',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 Säit gouf|$1 Säite goufe}} vun ärer Iwwerwaachungslëscht erofgeholl:',
'watchlistedit-raw-title'      => 'Iwwerwaachungslëscht onformatéiert änneren',
'watchlistedit-raw-legend'     => 'Iwwerwaachungslëscht onformatéiert änneren',
'watchlistedit-raw-explain'    => "D'Säite vun ärer Iwwerwaachungslëscht ginn ënnendrenner gewisen a kënne geännert ginn andeems der d'Säiten op d'Lëscht derbäisetzt oder erofhuelt; eng Säit pro Linn. Wann Dir fäerdeg sidd, klickt Iwwerwaachungslëscht aktualiséieren. Dir kënnt och [[Special:Watchlist/edit|de Standard Editeur benotzen]].",
'watchlistedit-raw-titles'     => 'Säiten:',
'watchlistedit-raw-submit'     => 'Iwwerwaachungslëscht aktualiséieren',
'watchlistedit-raw-done'       => 'Är Iwwerwaachungslëscht gouf aktualiséiert.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 Säit gouf|$1 Säite goufen}} derbäigesat:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 Säit gouf|$1 Säite goufen}} erausgeholl:',

# Watchlist editing tools
'watchlisttools-view' => 'Iwwerwaachungslëscht: Ännerungen',
'watchlisttools-edit' => 'Iwwerwaachungslëscht weisen an änneren',
'watchlisttools-raw'  => 'Net-formatéiert Iwwerwaachungslëscht änneren',

# Core parser functions
'unknown_extension_tag' => 'Onbekannten Erweiderungs-Tag "$1"',

# Special:Version
'version'                          => 'Versioun', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Installéiert Erweiderungen',
'version-specialpages'             => 'Spezialsäiten',
'version-parserhooks'              => 'Parser-Erweiderungen',
'version-variables'                => 'Variablen',
'version-other'                    => 'Aner',
'version-mediahandlers'            => 'Medien-Ënnerstetzung',
'version-hooks'                    => 'Klameren',
'version-extension-functions'      => 'Funktioune vun den Erweiderungen',
'version-parser-extensiontags'     => "Parser-Erweiderungen ''(Taggen)''",
'version-parser-function-hooks'    => 'Parser-Funktiounen',
'version-skin-extension-functions' => 'Skin-Erweiderungs-Funktiounen',
'version-hook-name'                => 'Numm vun der Klamer',
'version-hook-subscribedby'        => 'Opruff vum',
'version-version'                  => 'Versioun',
'version-license'                  => 'Lizenz',
'version-software'                 => 'Installéiert Software',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Versioun',

# Special:FilePath
'filepath'         => 'Pad bäi de Fichier',
'filepath-page'    => 'Fichier:',
'filepath-submit'  => 'Pad',
'filepath-summary' => 'Matt dëser Spezialsäit kënnt Dir de komplette Pad vun der aktueller Versioun vun engem engem Fichier direkt offroen. Den ugefrote Fichier gëtt direkt gewisen respektiv mat enger verbonner Applikatioun gestart.

D\'Ufro muss ouni den Zousaz "{{ns:image}}": gemaach ginn.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Sich no duebele Fichieren',
'fileduplicatesearch-summary'  => "Sich no Doublonen vu Fichieren op der Basis vun hirem ''Hash-Wert''.

Gitt den Numm vum Fichier ouni de Prefix \"{{ns:image}}:\" an.",
'fileduplicatesearch-legend'   => 'Sich no engem Doublon',
'fileduplicatesearch-filename' => 'Numm vum Fichier:',
'fileduplicatesearch-submit'   => 'Sichen',
'fileduplicatesearch-info'     => '$1 × $2 Pixel<br />Gréisst vum Fichier: $3<br />MIME Typ: $4',
'fileduplicatesearch-result-1' => 'De Fichier "$1" huet keen identeschen Doublon.',
'fileduplicatesearch-result-n' => 'De Fichier "$1" huet {{PLURAL:$2|1 identeschen Doublon|$2 identesch Doublonen}}.',

# Special:SpecialPages
'specialpages'                   => 'Spezialsäiten',
'specialpages-note'              => '----
* Normal Spezialsäiten
* <span class="mw-specialpagerestricted">Fettgeschriwwe</span> Spezialsäite sinn nëmme fir Benotzer mat méi Rechter.',
'specialpages-group-maintenance' => 'Maintenance-Rapporten',
'specialpages-group-other'       => 'Aner Spezialsäiten',
'specialpages-group-login'       => 'Aloggen / Umellen',
'specialpages-group-changes'     => 'Rezent Ännerungen a Lëschten',
'specialpages-group-media'       => 'Medie-Rapporten an Uploaden',
'specialpages-group-users'       => 'Benotzer a Rechter',
'specialpages-group-highuse'     => 'Dacks benotzte Säiten',
'specialpages-group-pages'       => 'Lëscht vu Säiten',
'specialpages-group-pagetools'   => 'Handwierksgeschir fir Säiten',
'specialpages-group-wiki'        => 'Systemdaten an Handwierksgeschir',
'specialpages-group-redirects'   => 'Spezialsäiten déi viruleeden',
'specialpages-group-spam'        => 'Handwierksgeschir géint de Spam',

# Special:BlankPage
'blankpage'              => 'Eidel Säit',
'intentionallyblankpage' => 'Dës Säit ass absichtlech eidel. Si gëtt fir Benchmarking an Ähnleches benotzt.',

);
