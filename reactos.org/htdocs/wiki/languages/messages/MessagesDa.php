<?php
/** Danish (Dansk)
 *
 * @ingroup Language
 * @file
 *
 * @author Anders Wegge Jakobsen <awegge@gmail.com>
 * @author Boivie
 * @author EPO
 * @author H92
 * @author Jan Friberg
 * @author Jon Harald Søby
 * @author Lars J. Helbo <lars.helbo@gmail.com>
 * @author MinuteElectron
 * @author Peter Andersen
 * @author Ranveig
 * @author S.Örvarr.S
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Speciel',
	NS_MAIN           => '',
	NS_TALK           => 'Diskussion',
	NS_USER           => 'Bruger',
	NS_USER_TALK      => 'Brugerdiskussion',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1-diskussion',
	NS_IMAGE          => 'Billede',
	NS_IMAGE_TALK     => 'Billeddiskussion',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki-diskussion',
	NS_TEMPLATE       => 'Skabelon',
	NS_TEMPLATE_TALK  => 'Skabelondiskussion',
	NS_HELP           => 'Hjælp',
	NS_HELP_TALK      => 'Hjælp-diskussion',
	NS_CATEGORY       => 'Kategori',
	NS_CATEGORY_TALK  => 'Kategoridiskussion'
);

$skinNames = array(
	'standard'      => 'Klassik',
	'nostalgia'     => 'Nostalgi',
	'cologneblue'   => 'Kølnerblå',
	'monobook'      => 'MonoBook',
	'chick'         => 'chick'
);

$bookstoreList = array(
	"Bibliotek.dk" => "http://bibliotek.dk/vis.php?base=dfa&origin=kommando&field1=ccl&term1=is=$1&element=L&start=1&step=10",
	"Bogguide.dk" => "http://www.bogguide.dk/find_boeger_bog.asp?ISBN=$1",
	'inherit' => true,
);

$separatorTransformTable = array(',' => '.', '.' => ',' );
$linkTrail = '/^([a-zæøå]+)(.*)$/sDu';

$specialPageAliases = array(
	'DoubleRedirects'           => array( 'Dobbelte_omdirigeringer' ),
	'BrokenRedirects'           => array( 'Defekte_omdirigeringer' ),
	'Disambiguations'           => array( 'Flertydige_artikler' ),
	'Userlogin'                 => array( 'Log_på', 'Brugerlogin' ),
	'Userlogout'                => array( 'Brugerlogout' ),
	'CreateAccount'             => array( 'Opret_konto' ),
	'Preferences'               => array( 'Indstillinger' ),
	'Watchlist'                 => array( 'Overvågningsliste' ),
	'Recentchanges'             => array( 'Seneste_ændringer' ),
	'Upload'                    => array( 'Upload' ),
	'Imagelist'                 => array( 'Filer', 'Filliste' ),
	'Newimages'                 => array( 'Nye_filer' ),
	'Listusers'                 => array( 'Brugerliste', 'Bruger' ),
	'Statistics'                => array( 'Statistik' ),
	'Randompage'                => array( 'Tilfældig_side' ),
	'Lonelypages'               => array( 'Forældreløse_sider' ),
	'Uncategorizedpages'        => array( 'Ukategoriserede_sider' ),
	'Uncategorizedcategories'   => array( 'Ukategoriserede_kategorier' ),
	'Uncategorizedimages'       => array( 'Ukategoriserede_filer' ),
	'Uncategorizedtemplates'    => array( 'Ukategoriserede_skabeloner' ),
	'Unusedcategories'          => array( 'Ubrugte_kategorier' ),
	'Unusedimages'              => array( 'Ubrugte_filer' ),
	'Wantedpages'               => array( 'Ønskede_sider' ),
	'Wantedcategories'          => array( 'Ønskede_kategorier' ),
	'Mostlinked'                => array( 'Sider_med_flest_henvisninger' ),
	'Mostlinkedcategories'      => array( 'Kategorier_med_flest_sider' ),
	'Mostlinkedtemplates'       => array( 'Hyppigst_brugte_skabeloner' ),
	'Mostcategories'            => array( 'Sider_med_flest_kategorier' ),
	'Mostimages'                => array( 'Mest_brugte_filer' ),
	'Mostrevisions'             => array( 'Artikler_med_flest_redigeringer' ),
	'Fewestrevisions'           => array( 'Artikler_med_færrest_redigeringer' ),
	'Shortpages'                => array( 'Korteste_sider' ),
	'Longpages'                 => array( 'Længste_sider' ),
	'Newpages'                  => array( 'Nye_sider' ),
	'Ancientpages'              => array( 'Ældste_sider' ),
	'Deadendpages'              => array( 'Blindgydesider' ),
	'Protectedpages'            => array( 'Beskyttede_sider' ),
	'Protectedtitles'           => array( 'Beskyttede_titler' ),
	'Allpages'                  => array( 'Alle_sider' ),
	'Prefixindex'               => array( 'Præfiksindeks' ),
	'Ipblocklist'               => array( 'Blokerede_adresser' ),
	'Specialpages'              => array( 'Specialsider' ),
	'Contributions'             => array( 'Bidrag' ),
	'Emailuser'                 => array( 'E-Mail' ),
	'Confirmemail'              => array( 'Bekræft_e-mail' ),
	'Whatlinkshere'             => array( 'Hvad_linker_hertil' ),
	'Recentchangeslinked'       => array( 'Relaterede_ændringer' ),
	'Movepage'                  => array( 'Flyt_side' ),
	'Blockme'                   => array( 'Proxyspærring' ),
	'Booksources'               => array( 'ISBN-søgning' ),
	'Categories'                => array( 'Kategorier' ),
	'Export'                    => array( 'Eksporter' ),
	'Version'                   => array( 'Version' ),
	'Allmessages'               => array( 'MediaWiki-systemmeddelelser' ),
	'Log'                       => array( 'Loglister' ),
	'Blockip'                   => array( 'Bloker_adresse' ),
	'Undelete'                  => array( 'Gendannelse' ),
	'Import'                    => array( 'Import' ),
	'Lockdb'                    => array( 'Databasespærring' ),
	'Unlockdb'                  => array( 'Databaseåbning' ),
	'Userrights'                => array( 'Brugerrettigheder' ),
	'MIMEsearch'                => array( 'MIME-type-søgning' ),
	'FileDuplicateSearch'       => array( 'Filduplikatsøgning' ),
	'Unwatchedpages'            => array( 'Uovervågede_sider' ),
	'Listredirects'             => array( 'Henvisninger' ),
	'Revisiondelete'            => array( 'Versionssletning' ),
	'Unusedtemplates'           => array( 'Ubrugte_skabeloner' ),
	'Randomredirect'            => array( 'Tilfældig_henvisning' ),
	'Mypage'                    => array( 'Min_brugerside' ),
	'Mytalk'                    => array( 'Min_diskussionsside' ),
	'Mycontributions'           => array( 'Mine_bidrag' ),
	'Listadmins'                => array( 'Administratorer' ),
	'Listbots'                  => array( 'Botter' ),
	'Popularpages'              => array( 'Populære_sider' ),
	'Search'                    => array( 'Søgning' ),
	'Resetpass'                 => array( 'Nulstil_kodeord' ),
	'Withoutinterwiki'          => array( 'Manglende_interwikilinks' ),
	'MergeHistory'              => array( 'Sammenfletning_af_historikker' ),
	'Filepath'                  => array( 'Filsti' ),
	'Invalidateemail'           => array( 'Ugyldiggør_e-mail' ),
);

$dateFormats = array(
	'mdy time' => 'H:i',
	'mdy date' => 'M j. Y',
	'mdy both' => 'M j. Y, H:i',

	'dmy time' => 'H:i',
	'dmy date' => 'j. F Y',
	'dmy both' => 'j. M Y, H:i',

	'ymd time' => 'H:i',
	'ymd date' => 'Y M j',
	'ymd both' => 'Y M j, H:i'
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Understreg henvisninger',
'tog-highlightbroken'         => 'Brug røde henvisninger til tomme sider',
'tog-justify'                 => 'Vis artikler med lige marginer',
'tog-hideminor'               => 'Skjul mindre ændringer i listen over seneste ændringer',
'tog-extendwatchlist'         => 'Udvidet liste med seneste ændringer',
'tog-usenewrc'                => 'Forbedret liste over seneste ændringer (JavaScript)',
'tog-numberheadings'          => 'Automatisk nummerering af overskrifter',
'tog-showtoolbar'             => 'Vis værktøjslinje til redigering (JavaScript)',
'tog-editondblclick'          => 'Redigér sider med dobbeltklik (JavaScript)',
'tog-editsection'             => 'Redigér afsnit ved hjælp af [redigér]-henvisninger',
'tog-editsectiononrightclick' => 'Redigér afsnit ved at klikke på deres titler (JavaScript)',
'tog-showtoc'                 => 'Vis indholdsfortegnelse (i artikler med mere end tre afsnit)',
'tog-rememberpassword'        => 'Husk adgangskode til næste besøg fra denne computer',
'tog-editwidth'               => 'Redigeringsboksen har fuld bredde',
'tog-watchcreations'          => 'Tilføj sider jeg opretter til min overvågningsliste',
'tog-watchdefault'            => 'Tilføj sider jeg redigerer til min overvågningsliste',
'tog-watchmoves'              => 'Tilføj sider jeg flytter til min overvågningsliste',
'tog-watchdeletion'           => 'Tilføj sider jeg sletter til min overvågningsliste',
'tog-minordefault'            => 'Markér som standard alle redigering som mindre',
'tog-previewontop'            => 'Vis forhåndsvisning over redigeringsboksen',
'tog-previewonfirst'          => 'Vis forhåndsvisning når du starter med at redigere',
'tog-nocache'                 => 'Slå caching af sider fra',
'tog-enotifwatchlistpages'    => 'Send mig en e-mail ved sideændringer',
'tog-enotifusertalkpages'     => 'Send mig en e-mail når min brugerdiskussionsside ændres',
'tog-enotifminoredits'        => 'Send mig også en e-mail ved mindre ændringer af overvågede sider',
'tog-enotifrevealaddr'        => 'Vis min e-mail-adresse i mails med besked om ændringer',
'tog-shownumberswatching'     => 'Vis antal brugere, der overvåger',
'tog-fancysig'                => 'Signaturer uden automatisk henvisning',
'tog-externaleditor'          => 'Brug ekstern editor automatisk',
'tog-externaldiff'            => 'Brug ekstern forskelsvisning automatisk',
'tog-showjumplinks'           => 'Vis tilgængeligheds-henvisninger',
'tog-uselivepreview'          => 'Brug automatisk forhåndsvisning (JavaScript) (eksperimentel)',
'tog-forceeditsummary'        => 'Advar, hvis sammenfatning mangler ved gemning',
'tog-watchlisthideown'        => 'Skjul egne ændringer i overvågningslisten',
'tog-watchlisthidebots'       => 'Skjul ændringer fra bots i overvågningslisten',
'tog-watchlisthideminor'      => 'Skjul mindre ændringer i overvågningslisten',
'tog-nolangconversion'        => 'Deaktiver konverteringer af sprogvarianter',
'tog-ccmeonemails'            => 'Send mig kopier af e-mails, som jeg sender til andre brugere.',
'tog-diffonly'                => 'Vis ved versionssammenligninger kun forskelle, ikke hele siden',
'tog-showhiddencats'          => 'Show hidden categories',

'underline-always'  => 'altid',
'underline-never'   => 'aldrig',
'underline-default' => 'efter browserindstilling',

'skinpreview' => '(Forhåndsvisning)',

# Dates
'sunday'        => 'søndag',
'monday'        => 'mandag',
'tuesday'       => 'tirsdag',
'wednesday'     => 'onsdag',
'thursday'      => 'torsdag',
'friday'        => 'fredag',
'saturday'      => 'lørdag',
'sun'           => 'søn',
'mon'           => 'man',
'tue'           => 'tir',
'wed'           => 'ons',
'thu'           => 'tor',
'fri'           => 'fre',
'sat'           => 'lør',
'january'       => 'januar',
'february'      => 'februar',
'march'         => 'marts',
'april'         => 'april',
'may_long'      => 'maj',
'june'          => 'juni',
'july'          => 'juli',
'august'        => 'august',
'september'     => 'september',
'october'       => 'oktober',
'november'      => 'november',
'december'      => 'december',
'january-gen'   => 'januars',
'february-gen'  => 'februars',
'march-gen'     => 'marts',
'april-gen'     => 'aprils',
'may-gen'       => 'majs',
'june-gen'      => 'junis',
'july-gen'      => 'julis',
'august-gen'    => 'augusts',
'september-gen' => 'septembers',
'october-gen'   => 'oktobers',
'november-gen'  => 'novembers',
'december-gen'  => 'decembers',
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
'pagecategories'                 => '{{PLURAL:$1|Kategori|Kategorier}}',
'category_header'                => 'Artikler i kategorien "$1"',
'subcategories'                  => 'Underkategorier',
'category-media-header'          => 'Medier i kategorien „$1“',
'category-empty'                 => "''Denne kategori indeholder for øjeblikket hverken sider eller medie-filer.''",
'hidden-categories'              => '{{PLURAL:$1|Skjult kategori|Skjulte kategorier}}',
'hidden-category-category'       => 'Skjulte kategorier', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Denne kategori har en underkategori.|Denne kategori indeholder nedenstående {{PLURAL:$1|underkategori|$1 underkategorier}}, af ialt $2.}}',
'category-subcat-count-limited'  => 'Denne kategori indeholder {{PLURAL:$1|underkategori|$1 underkategorier}}.',
'category-article-count'         => 'Denne kategori indeholder {{PLURAL:$2|kun den nedenstående side|{{PLURAL:$1|den nedenstående side|de nedenstående $1 sider}} af ialt $2.}}',
'category-article-count-limited' => 'Kategorien indeholder {{PLURAL:$1|den nedenstående side|de nedenstående $1 sider}}.',
'category-file-count'            => 'Denne kategori indeholder {{PLURAL:$2|kun den nedenstående fil|{{PLURAL:$1|den nedenstående fil|de nedenstående $1 filer}} af ialt $2.}}',
'category-file-count-limited'    => 'Kategorien indeholder {{PLURAL:$1|den nedenstående fil|de nedenstående $1 filer}}.',
'listingcontinuesabbrev'         => ' forts.',

'mainpagetext'      => 'MediaWiki er nu installeret.',
'mainpagedocfooter' => 'Se vores engelsksprogede [http://meta.wikimedia.org/wiki/MediaWiki_localisation dokumentation om tilpasning af brugergrænsefladen] og [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide brugervejledningen] for oplysninger om opsætning og anvendelse.',

'about'          => 'Om',
'article'        => 'Artikel',
'newwindow'      => '(åbner i et nyt vindue)',
'cancel'         => 'Afbryd',
'qbfind'         => 'Søg',
'qbbrowse'       => 'Gennemse',
'qbedit'         => 'Redigér',
'qbpageoptions'  => 'Indstillinger for side',
'qbpageinfo'     => 'Information om side',
'qbmyoptions'    => 'Mine indstillinger',
'qbspecialpages' => 'Specielle sider',
'moredotdotdot'  => 'Mere...',
'mypage'         => 'Min side',
'mytalk'         => 'Min diskussion',
'anontalk'       => 'Diskussionsside for denne IP-adresse',
'navigation'     => 'Navigation',
'and'            => 'og',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fejl',
'returnto'          => 'Tilbage til $1.',
'tagline'           => 'Fra {{SITENAME}}',
'help'              => 'Hjælp',
'search'            => 'Søg',
'searchbutton'      => 'Søg',
'go'                => 'Gå til',
'searcharticle'     => 'Gå til',
'history'           => 'Historik',
'history_short'     => 'Historik',
'updatedmarker'     => '(ændret)',
'info_short'        => 'Information',
'printableversion'  => 'Udskriftsvenlig udgave',
'permalink'         => 'Permanent henvisning',
'print'             => 'Udskriv',
'edit'              => 'Redigér',
'create'            => 'opret',
'editthispage'      => 'Redigér side',
'create-this-page'  => 'opret ny side',
'delete'            => 'Slet',
'deletethispage'    => 'Slet side',
'undelete_short'    => 'Fortryd sletning af {{PLURAL:$1|$1 version|$1 versioner}}',
'protect'           => 'Beskyt',
'protect_change'    => 'Ændret beskyttelse',
'protectthispage'   => 'Beskyt side',
'unprotect'         => 'Fjern beskyttelse',
'unprotectthispage' => 'Frigiv side',
'newpage'           => 'Ny side',
'talkpage'          => 'Diskussion',
'talkpagelinktext'  => 'diskussion',
'specialpage'       => 'Speciel side',
'personaltools'     => 'Personlige værktøjer',
'postcomment'       => 'Tilføj en kommentar',
'articlepage'       => 'Se artiklen',
'talk'              => 'Diskussion',
'views'             => 'Visninger',
'toolbox'           => 'Værktøjer',
'userpage'          => 'Se brugersiden',
'projectpage'       => 'Se projektsiden',
'imagepage'         => 'Se billedsiden',
'mediawikipage'     => 'Vise indholdsside',
'templatepage'      => 'Vise skabelonside',
'viewhelppage'      => 'Vise hjælpeside',
'categorypage'      => 'Vise kategoriside',
'viewtalkpage'      => 'Se diskussion',
'otherlanguages'    => 'Andre sprog',
'redirectedfrom'    => '(Omdirigeret fra $1)',
'redirectpagesub'   => 'Omdirigering',
'lastmodifiedat'    => 'Denne side blev senest ændret den $2, $1.', # $1 date, $2 time
'viewcount'         => 'Siden er vist i alt $1 {{PLURAL:$1|gang|gange}}.',
'protectedpage'     => 'Beskyttet side',
'jumpto'            => 'Skift til:',
'jumptonavigation'  => 'Navigation',
'jumptosearch'      => 'Søgning',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Om {{SITENAME}}',
'aboutpage'            => 'Project:Om',
'bugreports'           => 'Fejlrapporter',
'bugreportspage'       => 'Project:Fejlrapporter',
'copyright'            => 'Indholdet er udgivet under $1.',
'copyrightpagename'    => '{{SITENAME}} ophavsret',
'copyrightpage'        => '{{ns:project}}:Ophavsret',
'currentevents'        => 'Aktuelle begivenheder',
'currentevents-url'    => 'Project:Aktuelle begivenheder',
'disclaimers'          => 'Forbehold',
'disclaimerpage'       => 'Project:Generelle forbehold',
'edithelp'             => 'Hjælp til redigering',
'edithelppage'         => 'Help:Hvordan redigerer jeg en side',
'faq'                  => 'OSS',
'faqpage'              => 'Project:OSS',
'helppage'             => 'Help:Hjælp',
'mainpage'             => 'Forside',
'mainpage-description' => 'Forside',
'policy-url'           => 'Project:Politik',
'portal'               => 'Forside for skribenter',
'portal-url'           => 'Project:Forside',
'privacy'              => 'Behandling af personlige oplysninger',
'privacypage'          => 'Project:Behandling_af_personlige_oplysninger',

'badaccess'        => 'Manglende rettigheder',
'badaccess-group0' => 'Du har ikke de nødvendige rettigheder til denne handling.',
'badaccess-group1' => 'Denne handling kan kun udføres af brugere, som tilhører gruppen „$1“.',
'badaccess-group2' => 'Denne handling kan kun udføres af brugere, som tilhører en af grupperne „$1“.',
'badaccess-groups' => 'Denne handling kan kun udføres af brugere, som tilhører en af grupperne „$1“.',

'versionrequired'     => 'Kræver version $1 af MediaWiki',
'versionrequiredtext' => 'Version $1 af MediaWiki er påkrævet, for at bruge denne side. Se [[Special:Version|Versionssiden]]',

'ok'                      => 'OK',
'retrievedfrom'           => 'Hentet fra "$1"',
'youhavenewmessages'      => 'Du har $1 ($2).',
'newmessageslink'         => 'nye beskeder',
'newmessagesdifflink'     => 'ændringer siden sidste visning',
'youhavenewmessagesmulti' => 'Der er nye meddelelser til dig: $1',
'editsection'             => 'redigér',
'editold'                 => 'redigér',
'viewsourceold'           => 'vis kildekode',
'editsectionhint'         => 'Rediger afsnit: $1',
'toc'                     => 'Indholdsfortegnelse',
'showtoc'                 => 'vis',
'hidetoc'                 => 'skjul',
'thisisdeleted'           => 'Se eller gendan $1?',
'viewdeleted'             => 'Vise $1?',
'restorelink'             => '{{PLURAL:$1|en slettet ændring|$1 slettede ændringer}}',
'feedlinks'               => 'Feed:',
'feed-invalid'            => 'Ugyldig abonnementstype.',
'feed-unavailable'        => '{{SITENAME}} har ingen syndikerings-feeds',
'site-rss-feed'           => '$1 RSS-feed',
'site-atom-feed'          => '$1 Atom-feed',
'page-rss-feed'           => '"$1" RSS-feed',
'page-atom-feed'          => '"$1" Atom-feed',
'red-link-title'          => '$1 (uoprettet)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'artikel',
'nstab-user'      => 'brugerside',
'nstab-media'     => 'medie',
'nstab-special'   => 'speciel',
'nstab-project'   => 'om',
'nstab-image'     => 'billede',
'nstab-mediawiki' => 'besked',
'nstab-template'  => 'skabelon',
'nstab-help'      => 'hjælp',
'nstab-category'  => 'kategori',

# Main script and global functions
'nosuchaction'      => 'Funktionen findes ikke',
'nosuchactiontext'  => "Funktion angivet i URL'en kan ikke genkendes af MediaWiki-softwaren",
'nosuchspecialpage' => 'En sådan specialside findes ikke',
'nospecialpagetext' => 'Du har bedt om en specialside, der ikke kan genkendes af MediaWiki-softwaren.',

# General errors
'error'                => 'Fejl',
'databaseerror'        => 'Databasefejl',
'dberrortext'          => 'Der er opstået en syntaksfejl i en databaseforespørgsel. Dette kan være på grund af en ugyldig forespørgsel (se $5), eller det kan betyde en fejl i softwaren. 
Den seneste forsøgte databaseforespørgsel var: <blockquote><tt>$1</tt></blockquote> fra funktionen "<tt>$2</tt>". 
MySQL returnerede fejlen "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Der er opstået en syntaksfejl i en databaseforespørgsel. 
Den seneste forsøgte databaseforespørgsel var: "$1" fra funktionen "$2". 
MySQL returnerede fejlen "$3: $4".',
'noconnect'            => 'Der er problemer med {{SITENAME}}s database, vi kan desværre ikke komme i kontakt med den for øjeblikket. Prøv igen senere. <br />$1',
'nodb'                 => 'Kunne ikke vælge databasen $1',
'cachederror'          => 'Det følgende er en gemt kopi af den ønskede side, og er måske ikke helt opdateret.',
'laggedslavemode'      => 'Bemærk: Den viste side indeholder muligvis ikke de nyeste ændringer.',
'readonly'             => 'Databasen er skrivebeskyttet',
'enterlockreason'      => 'Skriv en begrundelse for skrivebeskyttelsen, med samt en vurdering af, hvornår skrivebeskyttelsen ophæves igen',
'readonlytext'         => 'Databasen er midlertidigt skrivebeskyttet. Forsøg venligst senere.

Årsag til spærringen: $1',
'missing-article'      => 'Databasen indeholder ikke teksten til en side der burde eksistere med navnet "$1" $2.

Den sandsynlige årsag er at du har fulgt et forældet link til en
forskel eller en gammel version af en side der er blevet slettet. Hvis
det ikke er tilfældet, har du muligvis fundet en software-fejl. Gør
venligst en [[Special:ListUsers/sysop|administrator]] opmærksom på
det, og husk at fortælle hvilken URL du har fulgt.',
'missingarticle-rev'   => '(versionsnummer: $1)',
'missingarticle-diff'  => '(Forskel: $1, $2)',
'readonly_lag'         => 'Databasen er automatisk blevet låst mens slave database serverne synkronisere med master databasen',
'internalerror'        => 'Intern fejl',
'internalerror_info'   => 'Internal fejl: $1',
'filecopyerror'        => 'Kunne ikke kopiere filen "$1" til "$2".',
'filerenameerror'      => 'Kunne ikke omdøbe filen "$1" til "$2".',
'filedeleteerror'      => 'Kunne ikke slette filen "$1".',
'directorycreateerror' => 'Kunne ikke oprette kataloget "$1".',
'filenotfound'         => 'Kunne ikke finde filen "$1".',
'fileexistserror'      => 'Kunne ikke oprette "$1": filen findes allerede',
'unexpected'           => 'Uventet værdi: "$1"="$2".',
'formerror'            => 'Fejl: Kunne ikke afsende formular',
'badarticleerror'      => 'Denne funktion kan ikke udføres på denne side.',
'cannotdelete'         => 'Kunne ikke slette siden eller filen der blev angivet.',
'badtitle'             => 'Forkert titel',
'badtitletext'         => 'Den ønskede sides titel var ikke tilladt, tom eller siden er forkert henvist fra en {{SITENAME}} på et andet sprog.',
'perfdisabled'         => 'Denne funktion er desværre midlertidigt afbrudt, fordi den belaster databasen meget hårdt og i en sådan grad, at siden bliver meget langsom. Funktionen bliver forhåbentlig omskrevet i den nærmeste fremtid (måske af dig, det er jo open source!).',
'perfcached'           => 'Følgende data er gemt i cachen, det er muligvis ikke helt opdateret:',
'perfcachedts'         => 'Disse data stammer fra cachen, sidste update: $1',
'querypage-no-updates' => "'''Aktualiseringsfunktionen for denne side er pt. deaktiveret. Dataene bliver indtil videre ikke fornyet.'''",
'wrong_wfQuery_params' => 'Ugyldig parameter til wfQuery()<br />
Funktion: $1<br />
Forespørgsel: $2',
'viewsource'           => 'Vis kilden',
'viewsourcefor'        => 'for $1',
'actionthrottled'      => 'Begrænsning af handling',
'actionthrottledtext'  => 'For at modvirke spam, er det ikke muligt at udføre denne handling mange gange på kort tid. Du har overskredet grænsen, hvorfor handlingen er blevet afbrudt. Vær venlig at forsøge igen om et par minutter.',
'protectedpagetext'    => 'Denne side er skrivebeskyttet.',
'viewsourcetext'       => 'Du kan dog se og kopiere kildekoden til siden:',
'protectedinterface'   => 'Denne side indeholder tekst til softwarens sprog-interface og er skrivebeskyttet for at forhindre misbrug.',
'editinginterface'     => "'''Advarsel:''' Denne side indeholder tekst, som bruges af MediaWiki-softwaren. Ændringer har virkning på brugergrænsefladen.",
'sqlhidden'            => '(SQL forespørgsel gemt)',
'cascadeprotected'     => 'Denne side er skrivebeskyttet, da den er indeholdt i nedenstående {{PLURAL:$1|side|sider}}, som er skrivebeskyttet med tilvalg af "nedarvende sidebeskyttelse":
$2',
'namespaceprotected'   => 'Du har ikke rettigheder til t redigere sider i $1-navnerummet.',
'customcssjsprotected' => 'Du har ikke rettigheder til at redigere denne side, da den indeholder en anden brugers personlige indstillinger.',
'ns-specialprotected'  => 'Sider i navnerummet {{ns:special}} kan ikke redigeres.',
'titleprotected'       => 'Dette sidenavn er beskyttet mod oprettelse af [[User:$1|$1]]. Begrundelsen for beskyttelsen er <i>$2</i>.',

# Virus scanner
'virus-badscanner'     => 'Konfigurationsfejl: ukendt virus-scanner: <i>$1</i>',
'virus-scanfailed'     => 'virus-scan fejlede med fejlkode $1',
'virus-unknownscanner' => 'ukendt virus-scanner:',

# Login and logout pages
'logouttitle'                => 'Bruger-log-af',
'logouttext'                 => 'Du er nu logget af.
Du kan fortsætte med at bruge {{SITENAME}} anonymt, eller du kan logge på igen som den samme eller en anden bruger.',
'welcomecreation'            => '== Velkommen, $1! ==

Din konto er blevet oprettet. Glem ikke at personliggøre dine {{SITENAME}}-indstillinger.',
'loginpagetitle'             => 'Bruger log på',
'yourname'                   => 'Dit brugernavn',
'yourpassword'               => 'Din adgangskode',
'yourpasswordagain'          => 'Gentag adgangskode',
'remembermypassword'         => 'Husk min adgangskode til næste gang.',
'yourdomainname'             => 'Dit domænenavn',
'externaldberror'            => 'Der er opstået en fejl i en ekstern adgangsdatabase, eller du har ikke rettigheder til at opdatere denne.',
'loginproblem'               => '<b>Der har været et problem med at få dig logget på.</b><br />Prøv igen!',
'login'                      => 'Log på',
'nav-login-createaccount'    => 'Opret en konto eller log på',
'loginprompt'                => 'Du skal have cookies slået til for at kunne logge på {{SITENAME}}.',
'userlogin'                  => 'Opret en konto eller log på',
'logout'                     => 'Log af',
'userlogout'                 => 'Log af',
'notloggedin'                => 'Ikke logget på',
'nologin'                    => 'Du har ingen brugerkonto? $1.',
'nologinlink'                => 'Opret ny brugerkonto',
'createaccount'              => 'Opret en ny konto',
'gotaccount'                 => 'Du har allerede en brugerkonto? $1.',
'gotaccountlink'             => 'Log på',
'createaccountmail'          => 'via e-mail',
'badretype'                  => 'De indtastede adgangskoder er ikke ens.',
'userexists'                 => 'Det brugernavn du har valgt er allerede i brug. Vælg venligst et andet brugernavn.',
'youremail'                  => 'Din e-mail-adresse *',
'username'                   => 'Brugernavn:',
'uid'                        => 'Bruger-ID:',
'prefs-memberingroups'       => 'Medlem af {{PLURAL:$1|gruppen|grupperne}}:',
'yourrealname'               => 'Dit rigtige navn*',
'yourlanguage'               => 'Ønsket sprog',
'yourvariant'                => 'Sprogvariant',
'yournick'                   => 'Dit kaldenavn (til signaturer)',
'badsig'                     => 'Syntaksen i underskriften er ugyldig; kontroller venligst den brugte HTML.',
'badsiglength'               => 'Underskriften er for lang. Den må højst indeholde {{PLURAL:$1|}}$1 tegn.',
'email'                      => 'E-mail',
'prefs-help-realname'        => '* <strong>Dit rigtige navn</strong> (valgfrit): Hvis du vælger at oplyse dit navn vil dette blive brugt til at tilskrive dig dit arbejde.',
'loginerror'                 => 'Logon mislykket',
'prefs-help-email'           => '** <strong>E-mail-adresse</strong> (valgfrit): Giver andre mulighed for at kontakte dig, 
uden du behøver at afsløre din e-mail-adresse. 
Det kan også bruges til at fremsende en ny adgangskode til dig, hvis du glemmer den du har.',
'prefs-help-email-required'  => 'E-mail-adresse er krævet.',
'nocookiesnew'               => 'Din brugerkonto er nu oprettet, men du er ikke logget på. {{SITENAME}} bruger cookies til at logge brugere på. Du har slået cookies fra. Vær venlig at slå cookies til, og derefter kan du logge på med dit nye brugernavn og kodeord.',
'nocookieslogin'             => '{{SITENAME}} bruger cookies til at logge brugere på. Du har slået cookies fra. Slå dem venligst til og prøv igen.',
'noname'                     => 'Du har ikke angivet et gyldigt brugernavn.',
'loginsuccesstitle'          => 'Du er nu logget på',
'loginsuccess'               => 'Du er nu logget på {{SITENAME}} som "$1".',
'nosuchuser'                 => 'Der er ingen bruger med navnet "$1". Kontrollér stavemåden igen, eller brug formularen herunder til at oprette en ny brugerkonto.',
'nosuchusershort'            => 'Der er ingen bruger ved navn "$1". Tjek din stavning.',
'nouserspecified'            => 'Angiv venligst et brugernavn.',
'wrongpassword'              => 'Den indtastede adgangskode var forkert. Prøv igen.',
'wrongpasswordempty'         => 'Du glemte at indtaste password. Prøv igen.',
'passwordtooshort'           => 'Dit kodeord er for kort. Det skal være mindst {{PLURAL:$1|}}$1 tegn langt.',
'mailmypassword'             => 'Send et nyt password til min e-mail-adresse',
'passwordremindertitle'      => 'Nyt password til {{SITENAME}}',
'passwordremindertext'       => 'Nogen (sandsynligvis dig, fra IP-addressen $1)
har bedt om at vi sender dig en ny adgangskode til at logge på {{SITENAME}} ($4).
Adgangskoden for bruger "$2" er nu "$3".
Du bør logge på nu og ændre din adgangskode.,

Hvis en anden har bestilt den nye adgangskode eller hvis du er kommet i tanke om dit gamle password og ikke mere vil ændre det, 
kan du bare ignorere denne mail og fortsætte med at bruge dit gamle password.',
'noemail'                    => 'Der er ikke oplyst en e-mail-adresse for bruger "$1".',
'passwordsent'               => 'En ny adgangskode er sendt til e-mail-adressen,
som er registreret for "$1".
Du bør logge på og ændre din adgangskode straks efter du har modtaget e-mail\'en.',
'blocked-mailpassword'       => 'Din IP-adresse er spærret for ændring af sider. For at forhindre misbrug, er det heller ikke muligt, at bestille et nyt password.',
'eauthentsent'               => 'En bekrftelsesmail er sendt til den angivne E-mail-adresse.

Før en E-mail kan modtages af andre brugere af {{SITENAME}}-mailfunktionen, skal adressen og dens tilhørsforhold til denne bruger bekræftes. Følg venligst anvisningerne i denne mail.',
'throttled-mailpassword'     => 'Indenfor {{PLURAL:$1|den sidste time|de sidste $1 timer}} er der allerede sendt et nyt password. For at forhindre misbrug af funktionen, kan der kun bestilles et nyt password en gang for hver {{PLURAL:$1|time|$1 timer}}.',
'mailerror'                  => 'Fejl ved afsendelse af e-mail: $1',
'acct_creation_throttle_hit' => 'Du har allerede oprettet $1 kontoer. Du kan ikke oprette flere.',
'emailauthenticated'         => 'Din e-mail-adresse blev bekræftet på $1.',
'emailnotauthenticated'      => 'Din e-mail-adresse er endnu ikke bekræftet og de avancerede e-mail-funktioner er slået fra indtil bekræftelse har fundet sted (d.u.a.). Log ind med den midlertidige adgangskode, der er blevet sendt til dig, for at bekræfte, eller bestil et nyt på loginsiden.',
'noemailprefs'               => 'Angiv en E-mail-adresse, så følgende funktioner er til rådighed.',
'emailconfirmlink'           => 'Bekræft E-mail-adressen (autentificering).',
'invalidemailaddress'        => 'E-mail-adressen kan ikke accepteres da den tilsyneladende har et ugyldigt format. Skriv venligst en e-mail-adresse med et korrekt format eller tøm feltet.',
'accountcreated'             => 'Brugerkonto oprettet',
'accountcreatedtext'         => 'Brugerkontoen $1 er oprettet.',
'createaccount-title'        => 'Opret brugerkonto på {{SITENAME}}',
'createaccount-text'         => 'En bruger ($1) har oprettet en konto for $2 på {{SITENAME}}
($4). Password for "$2" er "$3". Du opfordres til at logge ind, og ændre kodeordet omgående.

Denne besked kan ignorewres, hvis denne konto er oprettet som følge af en fejl.',
'loginlanguagelabel'         => 'Sprog: $1',

# Password reset dialog
'resetpass'               => 'Nulstille password for brugerkonto',
'resetpass_announce'      => 'Log på med den via e-mail tilsendte password. For at afslutte tilmeldingen, skal du nu vælge et nyt password.',
'resetpass_header'        => 'Nulstille password',
'resetpass_submit'        => 'Send password og log på',
'resetpass_success'       => 'Dit password er nu ændret. Nu følger tilmelding …',
'resetpass_bad_temporary' => 'Ugyldigt foreløbigt password. Du har allerede ændret dit password eller bestilt et nyt foreløbigt password.',
'resetpass_forbidden'     => 'Dette password kan ikke ændres på {{SITENAME}}.',
'resetpass_missing'       => 'Tom formular.',

# Edit page toolbar
'bold_sample'     => 'Fed tekst',
'bold_tip'        => 'Fed tekst',
'italic_sample'   => 'Kursiv tekst',
'italic_tip'      => 'Kursiv tekst',
'link_sample'     => 'Henvisning',
'link_tip'        => 'Intern henvisning',
'extlink_sample'  => 'http://www.eksempel.dk Titel på henvisning',
'extlink_tip'     => 'Ekstern henvisning (husk http:// præfiks)',
'headline_sample' => 'Tekst til overskrift',
'headline_tip'    => 'Type 2 overskrift',
'math_sample'     => 'Indsæt formel her (LaTeX)',
'math_tip'        => 'Matematisk formel (LaTeX)',
'nowiki_sample'   => 'Indsæt tekst her som ikke skal wikiformateres',
'nowiki_tip'      => 'Ignorer wikiformatering',
'image_sample'    => 'Eksempel.jpg',
'image_tip'       => 'Indlejret billede',
'media_sample'    => 'Eksempel.mp3',
'media_tip'       => 'Henvisning til multimediefil',
'sig_tip'         => 'Din signatur med tidsstempel',
'hr_tip'          => 'Horisontal linje (brug den sparsomt)',

# Edit pages
'summary'                          => 'Beskrivelse',
'subject'                          => 'Emne/overskrift',
'minoredit'                        => 'Dette er en mindre ændring.',
'watchthis'                        => 'Overvåg denne artikel',
'savearticle'                      => 'Gem side',
'preview'                          => 'Forhåndsvisning',
'showpreview'                      => 'Forhåndsvisning',
'showlivepreview'                  => 'Live-forhåndsvisning',
'showdiff'                         => 'Vis ændringer',
'anoneditwarning'                  => 'Du arbejder uden at være logget på. Istedet for brugernavn vises din IP-adresse i versionshistorikken.',
'missingsummary'                   => "'''Bemærk:''' du har ikke angivet en redigeringsbeskrivelse. Hvis du atter trykker på „Gem“, gemmes ændringerne uden resume.",
'missingcommenttext'               => 'Indtast venligst et resume.',
'missingcommentheader'             => "'''BEMÆRK:''' du har ikke angivet en overskrift i feltet „Emne:“. Hvis du igen trykker på „Gem side“, gemmes bearbejdningen uden overskrift.",
'summary-preview'                  => 'Forhåndsvisning af resumelinien',
'subject-preview'                  => 'Forhåndsvisning af emnet',
'blockedtitle'                     => 'Brugeren er blokeret',
'blockedtext'                      => "<big>'''Dit brugernavn eller din IP-adresse er blevet blokeret.'''</big>

Blokeringen er foretaget af $1. Begrundelsen er ''$2''.

Blokeringen starter: $8
Blokeringen udløber: $6
Blokeringen er rettet mod: $7

Du kan kontakte $1 eller en af de andre [[{{MediaWiki:Grouppage-sysop}}|administratorer]] for at diskutere blokeringen.
Du kan ikke bruge funktionen 'e-mail til denne bruger' medmindre der er angivet en gyldig email-addresse i dine
[[Special:Preferences|kontoindstillinger]]. Din nuværende IP-addresse er $3, og blokerings-ID er #$5. Angiv venligst en eller begge i alle henvendelser.",
'autoblockedtext'                  => 'Den IP-adresse er blevet blokeret automatisk, fordi den blev brugt af en anden bruger, som blev blokeret af $1.
Begrundelsen for det er:

:\'\'$2\'\'

Blokeringsperiodens start: $8
Blokeringen udløber: $6

Du kan kontakte $1 eller en af de andre [[{{MediaWiki:Grouppage-sysop}}|administratorer]] for at diskutere blokeringen.

Bemærk, at du ikke kan bruge funktionen "e-mail til denne bruger" medmindre du har en gyldig e-mail addresse registreret i din [[Special:Preferences|brugerindstilling]].

Din blokerings-ID er $5. Angiv venligst denne ID ved alle henvendelser.',
'blockednoreason'                  => 'ingen begrundelse givet',
'blockedoriginalsource'            => "Kildekoden fra '''$1''' vises her:",
'blockededitsource'                => "Kildekoden fra '''Dine ændringer''' til '''$1''':",
'whitelistedittitle'               => 'Log på for at redigere',
'whitelistedittext'                => 'Du skal $1 for at kunne ændre artikler.',
'confirmedittitle'                 => 'For at kunne bearbejde er bekræftelsen af E-mail-adressen nødvendig.',
'confirmedittext'                  => 'Du skal først bekræfte E-mail-adressen, før du kan lave ændringer. Udfyld og bekræft din E-mail-adresse i dine [[Special:Preferences|Indstillinger]].',
'nosuchsectiontitle'               => 'Afsnit findes ikke',
'nosuchsectiontext'                => 'Du forsøgte at ændre det ikke eksisterende afsnit $1. Det er dog kun muligt at ændre eksisterende afsnit.',
'loginreqtitle'                    => 'Log på nødvendigt',
'loginreqlink'                     => 'logge på',
'loginreqpagetext'                 => 'Du skal $1 for at se andre sider.',
'accmailtitle'                     => 'Adgangskode sendt.',
'accmailtext'                      => "Adgangskoden for '$1' er sendt til $2.",
'newarticle'                       => '(Ny)',
'newarticletext'                   => "'''{{SITENAME}} har endnu ikke nogen {{NAMESPACE}}-side ved navn {{PAGENAME}}.'''<br /> Du kan begynde en side ved at skrive i boksen herunder. (se [[{{MediaWiki:Helppage}}|hjælpen]] for yderligere oplysninger).<br /> Eller du kan [[Special:Search/{{PAGENAME}}|søge efter {{PAGENAME}} i {{SITENAME}}]].<br /> Hvis det ikke var din mening, så tryk på '''Tilbage'''- eller '''Back'''-knappen.",
'anontalkpagetext'                 => "---- ''Dette er en diskussionsside for en anonym bruger, der ikke har oprettet en konto endnu eller ikke bruger den. Vi er derfor nødt til at bruge den nummeriske IP-adresse til at identificere ham eller hende. En IP-adresse kan være delt mellem flere brugere. Hvis du er en anonym bruger og synes, at du har fået irrelevante kommentarer på sådan en side, så vær venlig at oprette en brugerkonto og [[Special:UserLogin|logge på]], så vi undgår fremtidige forvekslinger med andre anonyme brugere.''",
'noarticletext'                    => "'''{{SITENAME}} har ikke nogen side med præcis dette navn.''' * Du kan '''[{{fullurl:{{FULLPAGENAME}}|action=edit}} starte siden {{PAGENAME}}]''' * Eller [[Special:Search/{{PAGENAME}}|søge efter {{PAGENAME}}]] i andre artikler ---- * Hvis du har oprettet denne artikel indenfor de sidste få minutter, så kan de skyldes at der er lidt forsinkelse i opdateringen af {{SITENAME}}s cache. Vent venligst og tjek igen senere om artiklen dukker op, inden du forsøger at oprette artiklen igen.",
'userpage-userdoesnotexist'        => 'Brugerkontoen "$1" findes ikke. Overvej om du ønsker at oprette eller redigere denne side.',
'clearyourcache'                   => "'''Bemærk''', efter at have gemt, er du nødt til at tømme din browsers cache for at kunne se ændringerne. '''Mozilla / Firefox / Safari''': hold ''shifttasten'' nede og klik på ''reload'' eller tryk på ''control-shift-r'' (Mac: ''cmd-shift-r''); '''Internet Explorer''': hold ''controltasten'' nede og klik på ''refresh'' eller tryk på ''control-F5''; '''Konqueror''': klik på ''reload'' eller tryk på ''F5''",
'usercssjsyoucanpreview'           => "<strong>Tip:</strong> Brug knappen 'forhåndsvisning' til at teste dit nye css/js før du gemmer.",
'usercsspreview'                   => "'''Husk at du kun tester/forhåndsviser dit eget css, den er ikke gemt endnu!'''",
'userjspreview'                    => "'''Husk at du kun tester/forhåndsviser dit eget javascript, det er ikke gemt endnu!'''",
'userinvalidcssjstitle'            => "'''Advarsel:''' Der findes intet skin „$1“. Tænk på, at brugerspecifikke .css- og .js-sider begynder med små bogstaver, altså f.eks. ''{{ns:user}}:Hansen/monobook.css'' og ikke ''{{ns:user}}:Hansen/Monobook.css''.",
'updated'                          => '(Opdateret)',
'note'                             => '<strong>Bemærk:</strong>',
'previewnote'                      => 'Husk at dette er kun en forhåndsvisning, siden er ikke gemt endnu!',
'previewconflict'                  => 'Denne forhåndsvisning er resultatet af den redigérbare tekst ovenfor, sådan vil det komme til at se ud hvis du vælger at gemme teksten.',
'session_fail_preview'             => '<strong>Din ændring kunne ikke gemmes, da dine sessionsdata er gået tabt.
Prøv venligst igen. Hvis problemet fortsætter, log af og log på igen.</strong>',
'session_fail_preview_html'        => "<strong>Din ændring kunne ikke gemmes, da dine sessionsdata er gået tabt.</strong>

''Da ren HTM er aktiveret i denne Wiki, er forhåndsvisningen blændet ud for at forebygge JavaScript-angreb.''

<strong>Forsøg venligst igen. Hvis problemet fortsætter, log af og log på igen.</strong>",
'token_suffix_mismatch'            => '<strong>Din redigering er afvist, da din browser har forvansket tegnsætningen i redigeringskontrolfilen. Afvisningen sker for at forhindre utilsigtede ændringer i artiklen. Denne fejl opstår nogle gange, når du redigerer gennem en fejlprogrammeret webbaseret anonymiseringstjeneste.</strong>',
'editing'                          => 'Redigerer $1',
'editingsection'                   => 'Redigerer $1 (afsnit)',
'editingcomment'                   => 'Redigerer $1 (kommentar)',
'editconflict'                     => 'Redigeringskonflikt: $1',
'explainconflict'                  => 'Nogen har ændret denne side, efter du startede på at redigere den.
Den øverste tekstboks indeholder den nuværende tekst.
Dine ændringer er vist i den nederste tekstboks.
Du er nødt til at sammenflette dine ændringer med den eksisterende tekst.
<b>Kun</b> teksten i den øverste tekstboks vil blive gemt når du trykker "Gem side".<br />',
'yourtext'                         => 'Din tekst',
'storedversion'                    => 'Den gemte version',
'nonunicodebrowser'                => '<strong>Advarsel: Din browser er ikke unicode-kompatibel, skift eller opdater din browser før du redigerer en artikel.</strong>',
'editingold'                       => '<strong>ADVARSEL: Du redigerer en gammel version af denne side.
Hvis du gemmer den, vil alle ændringer foretaget siden denne revision blive overskrevet.</strong>',
'yourdiff'                         => 'Forskelle',
'copyrightwarning'                 => '<strong>Husk: <big>kopier ingen websider</big>, som ikke tilhører dig selv, brug <big>ingen ophavsretsligt beskyttede værker</big> uden tilladelse fra ejeren!</strong><br />
Du lover os hermed, at du selv <strong>har skrevet teksten</strong>, at teksten tilhører almenheden, er (<strong>public domain</strong>), eller at <strong>ophavsrets-indehaveren</strong> har givet sin <strong>tilladelse</strong>. Hvis denne tekst allerede er offentliggkort andre steder, skriv det venligst på diskussionssiden.
<i>Bemærk venligst, at alle {{SITENAME}}-artikler automatisk står under „$2“ (se $1 for detaljer). Hvis du ikke vil, at dit arbejde her ændres og udbredes af andre, så tryk ikke på „Gem“.</i>',
'copyrightwarning2'                => 'Bemærk venligst, at alle artikler på {{SITENAME}} kan bearbejdes, ændres eller slettes af andre brugere.
Læg ingen tekster ind, hvis du ikke kan acceptere at disse kan ændres.

Du bekræfter hermed også, at du selv har skrevet denne tekst eller kopieret den fra en offentlig kilde
(se $1 for detaljer). <strong>OVERFØR IKKE OPHAVSRETSLIGT BESKYTTET INDHOLD!</strong>',
'longpagewarning'                  => '<strong>ADVARSEL: Denne side er $1 kilobyte stor; nogle browsere kan have problemer med at redigere sider der nærmer sig eller er større end 32 Kb. 
Overvej om siden kan opdeles i mindre dele.</strong>',
'longpageerror'                    => '<strong>FEJL: Teksten, som du ville gemme, er $1 kB stor. Det er større end det tilladet maksimum på $2 kB. Det er ikke muligt at gemme.</strong>',
'readonlywarning'                  => '<strong>ADVARSEL: Databasen er låst på grund af vedligeholdelse,
så du kan ikke gemme dine ændringer lige nu. Det kan godt være en god ide at kopiere din tekst til en tekstfil, så du kan gemme den til senere.</strong>',
'protectedpagewarning'             => '<strong>ADVARSEL: Denne side er skrivebeskyttet, så kun administratorer kan redigere den.</strong>',
'semiprotectedpagewarning'         => "'''Halv spærring:''' Siden er spærret, så kun registrerede brugere kan ændre den.",
'cascadeprotectedwarning'          => "'''BEMÆRK: Denne side er skrivebeskyttet, så den kun kan ændres af brugere med Administratorrettigheder. Den er indeholdt i nedenstående {{PLURAL:$1|side|sider}}, som er skrivebeskyttet med tilvalg af nedarvende sidebeskyttelse:'''",
'titleprotectedwarning'            => '<strong>ADVARSEL:  Den side er låst så kun nogle brugere kan oprette den.</strong>',
'templatesused'                    => 'Skabeloner der er brugt på denne side:',
'templatesusedpreview'             => 'Følgende skabeloner bruges af denne artikelforhåndsvisning:',
'templatesusedsection'             => 'Følgende skabeloner bruges af dette afsnit:',
'template-protected'               => '(skrivebeskyttet)',
'template-semiprotected'           => '(skrivebeskyttet for ikke anmeldte og nye brugere)',
'hiddencategories'                 => 'Denne side er i {{PLURAL:$1|en skjult kategori|$1 skjulte kategorier}}:',
'edittools'                        => '<!-- Denne tekst vises under formularen „Ændre“ samt "Upload". -->',
'nocreatetitle'                    => 'Oprettelse af nye sider er begrænset.',
'nocreatetext'                     => 'Serveren har begrænset oprettelse af nye sider. Bestående sider kan ændres eller [[Special:UserLogin|logge på]].',
'nocreate-loggedin'                => 'Du har ikke rettigheder til at oprette nye sider.',
'permissionserrors'                => 'Rettighedskonflikt',
'permissionserrorstext'            => 'Du har ikke rettigheder til at gennemføre denne handling, {{PLURAL:$1|årsagen|årsagerne}} er:',
'permissionserrorstext-withaction' => 'Du har ikke rettigheder til $2, med nedenstående {{PLURAL:$1|grund|grunde}}:',
'recreate-deleted-warn'            => "'''Advarsel: Du er ved at genskabe en tidligere slettet side.'''
 
Overvej om det er passende at genoprette siden. De slettede versioner for 
denne side er vist nedenfor:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Advarsel: Denne side indeholder ligeledes mange bekostelig analysere funktion opringninger.

Det burde være mindre end $2, der er nu $1.',
'expensive-parserfunction-category'       => 'Sider med for mange beregningstunge oversætter-funktioner',
'post-expand-template-inclusion-warning'  => 'Advarsel: Der er tilføjet for mange skabeloner til denne side, så nogle af dem bliver ikke vist..',
'post-expand-template-inclusion-category' => 'Sider der indeholder for mange skabeloner',
'post-expand-template-argument-warning'   => 'Advarsel: Mindst en af skabelonparametrene på denne side fylder mere end det tilladte. Denne parameter er derfor udeladt.',
'post-expand-template-argument-category'  => 'Sider med udeladte skabelonparametre',

# "Undo" feature
'undo-success' => 'Ændringen er nu annulleret. Kontroller venligst bearbejdningen i sammenligningen og klik så på „Gem side“, for at gemme den.',
'undo-failure' => '<span class="error">Ændringen kunne ikke annulleres, da det pågældende afsnit i mellemtiden er ændret.</span>',
'undo-norev'   => 'Ændringen kunne ikke annuleres fordi den ikke eksisterer eller er blevet slettet.',
'undo-summary' => 'Fjerner version $1 af [[Special:Contributions/$2|$2]] ([[User talk:$2|diskussion]])',

# Account creation failure
'cantcreateaccounttitle' => 'Brugerkontoen kan ikke oprettes.',
'cantcreateaccount-text' => "Oprettelsen af en brugerkonto fra IP-adressen <b>$1</b> er spærret af [[User:$3|$3]]. Årsagen til blokeringen er angivet som ''$2''",

# History pages
'viewpagelogs'        => 'Vis loglister for denne side',
'nohistory'           => 'Der er ingen versionshistorik for denne side.',
'revnotfound'         => 'Versionen er ikke fundet',
'revnotfoundtext'     => 'Den gamle version af den side du spurgte efter kan
ikke findes. Kontrollér den URL du brugte til at få adgang til denne side.',
'currentrev'          => 'Nuværende version',
'revisionasof'        => 'Versionen fra $1',
'revision-info'       => 'Version fra $1 til $2',
'previousrevision'    => '←Ældre version',
'nextrevision'        => 'Nyere version→',
'currentrevisionlink' => 'se nuværende version',
'cur'                 => 'nuværende',
'next'                => 'næste',
'last'                => 'forrige',
'page_first'          => 'Startem',
'page_last'           => 'Enden',
'histlegend'          => 'Forklaring: (nuværende) = forskel til den nuværende
version, (forrige) = forskel til den forrige version, M = mindre ændring',
'deletedrev'          => '[slettet]',
'histfirst'           => 'Ældste',
'histlast'            => 'Nyeste',
'historysize'         => '($1 {{PLURAL:$1|Byte|Bytes}})',
'historyempty'        => '(tom)',

# Revision feed
'history-feed-title'          => 'Versionshistorie',
'history-feed-description'    => 'Versionshistorie for denne side i {{SITENAME}}',
'history-feed-item-nocomment' => '$1 med $2', # user at time
'history-feed-empty'          => 'Den ønskede side findes ikke. Måske er den slettet eller flyttet. [[Special:Search|Gennesøg]] {{SITENAME}} efter passende nye sider.',

# Revision deletion
'rev-deleted-comment'         => '(Fjerne bearbejdsningskommentar)',
'rev-deleted-user'            => '(Fjerne brugernavn)',
'rev-deleted-event'           => '(Fjerne aktion)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks"> Denne version er slettet og er ikke mere offentligt tilgængelig.
Nærmere oplysninger om sletningen samt en begrundelse for den findes i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} slette-loggen].</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Denne version er slettet og er ikke længere offentligt tilgængelig.
Som administrator kan du stadig se den.
Nærmere oplysninger om sletningen samt en begrundelse for den findes i [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} slette-loggen].</div>',
'rev-delundel'                => 'vise/skjule',
'revisiondelete'              => 'Slette/gendanne versioner',
'revdelete-nooldid-title'     => 'Ingen version angivet',
'revdelete-nooldid-text'      => 'Du har ikke angivet en version, som denne handling kan udføres på.',
'revdelete-selected'          => "{{PLURAL:$2|Valgte version|Valgte versioner}} af '''$1:'''",
'logdelete-selected'          => '{{PLURAL:$1|Valgte logbogsindførsel|Valgte logbogsindførsler}}:',
'revdelete-text'              => 'Indholdet eller andre bestanddele er ikke mere offentligt tilgængelige, vises dog fortsat i versionshistorikken. 

Administratorer kan dog fortsat se og gendanne det fjernede indhold, medmindre det er bestemt, at adgangsbegrænsningen også skal gælde for administratorer.',
'revdelete-legend'            => 'Fastlægge begrænsninger for versionerne:',
'revdelete-hide-text'         => 'Skjul versionens tekst',
'revdelete-hide-name'         => 'Skjul logbogsaktionen',
'revdelete-hide-comment'      => 'Skjul ændringskommentar',
'revdelete-hide-user'         => 'Skjul brugerens grugernavn/IP',
'revdelete-hide-restricted'   => 'Disse begrænsninger gælder også for administratorer',
'revdelete-suppress'          => 'Skjul også årsagen til sletningen for administratorer',
'revdelete-hide-image'        => 'Skjul billedindhold',
'revdelete-unsuppress'        => 'Ophæve begrænsninger for gendannede versioner',
'revdelete-log'               => 'Kommentar/begrundelse (vises også i logbogen):',
'revdelete-submit'            => 'Bruge på udvalgte versioner',
'revdelete-logentry'          => 'Versionsvisning ændret for [[$1]]',
'logdelete-logentry'          => "ændrede [[$1]]'s synlighed",
'revdelete-success'           => 'Versionsvisning er ændret.',
'logdelete-success'           => "'''Synlighed ændret med success.'''",
'revdel-restore'              => 'Ændre synlighed',
'pagehist'                    => 'Sidehistorik',
'deletedhist'                 => 'Slettet historik',
'revdelete-content'           => 'indhold',
'revdelete-summary'           => 'redigeringsbeskrivelse',
'revdelete-uname'             => 'bruger',
'revdelete-restricted'        => 'tilføjede begrænsninger for administratorer',
'revdelete-unrestricted'      => 'fjernede begrænsninger for administratorer',
'revdelete-hid'               => 'usynliggjorde $1',
'revdelete-unhid'             => 'synliggjorde $1',
'revdelete-log-message'       => '$1 for $2 {{PLURAL:$2|version|versioner}}',
'logdelete-log-message'       => '$1 for $2 {{PLURAL:$2|hændelse|hændelser}}',

# Suppression log
'suppressionlog'     => 'Skjulningslog',
'suppressionlogtext' => 'Nedenfor listes de sletninger og blokeringer der er skjult for almindelige systemadministratorer.Below is a list of deletions and blocks involving content hidden from sysops.
Se [[Special:IPBlockList|IP blokeringslisten]] for alle blokeringer.',

# History merging
'mergehistory'                     => 'Sammenflet sidehistorikker',
'mergehistory-header'              => "Denne sider giver mulighed for at flette historikken fra en kildeside ind i en nyere side. 
Vær opmæksom på at bevare kontinuiteten i sidehistorikken.

'''Bevar som minimum den nuværende udgave af kildesiden.'''",
'mergehistory-box'                 => 'Sammenflet versioner af to sider:',
'mergehistory-from'                => 'Kildeside:',
'mergehistory-into'                => 'Destinationsside:',
'mergehistory-list'                => 'Sammenflettelig revisioner',
'mergehistory-merge'               => 'Nedenstående udgave af [[:$1]] kan sammenflettes med [[:$2]]. Vælg én version nedfor for at sammenflette denne og alle tidligere versioner. Bemærk at navigation i historikken nulstiller valget.',
'mergehistory-go'                  => 'Vis sammenflettelige versioner',
'mergehistory-submit'              => 'Sammenflet versioner',
'mergehistory-empty'               => 'Der findes ingen sammenflettelige udgaver',
'mergehistory-success'             => '$3 {{PLURAL:$3|version|versioner}} af [[:$1]] blev flettet sammen med [[:$2]].',
'mergehistory-fail'                => 'Sammenfletningen kunne ikke gennemføres. Vær venlig at kontrollere sidenavne og tidsafgrænsning.',
'mergehistory-no-source'           => 'Kildesiden $1 findes ikke.',
'mergehistory-no-destination'      => 'Destinationssiden $1 findes ikke.',
'mergehistory-invalid-source'      => 'Angiv et gyldigt sidenavn som kildeside.',
'mergehistory-invalid-destination' => 'Angiv et gyldigt sidenavn som destinationsside.',
'mergehistory-autocomment'         => 'Flettede [[:$1]] ind i [[:$2]]',
'mergehistory-comment'             => 'Flettede [[:$1]] ind i [[:$2]]: $3',

# Merge log
'mergelog'           => 'Sammenfletningslog',
'pagemerge-logentry' => 'flettede [[$1]] ind i [[$2]] (revisioner indtil $3)',
'revertmerge'        => 'Gendan sammenfletning',
'mergelogpagetext'   => 'Nedenfor vises en liste med de nyeste sammenfletninger af en sides historik i en anden.',

# Diffs
'history-title'           => 'Revisionshistorik for "$1"',
'difference'              => '(Forskelle mellem versioner)',
'lineno'                  => 'Linje $1:',
'compareselectedversions' => 'Sammenlign valgte versioner',
'editundo'                => 'annuller',
'diff-multi'              => "<span style='font-size: smaller'>(Versionssammenligningen medtager {{plural:$1|en mellemliggende version|$1 mellemliggende versioner}}.)</span>",

# Search results
'searchresults'             => 'Søgeresultater',
'searchresulttext'          => 'For mere information om søgning på {{SITENAME}}, se [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Til din søgning „[[:$1]]“.',
'searchsubtitleinvalid'     => 'Til din søgning „$1“.',
'noexactmatch'              => '{{SITENAME}} har ingen artikel med dette navn. Du kan [[:$1|oprette en artikel med dette navn]].',
'noexactmatch-nocreate'     => "'''Der er ingen side med navnet \"\$1\".'''",
'toomanymatches'            => 'Søgningen fandt for mange sider. Prøv venligst med en anden søgning.',
'titlematches'              => 'Artikeltitler der opfyldte forespørgslen',
'notitlematches'            => 'Ingen artikeltitler opfyldte forespørgslen',
'textmatches'               => 'Artikeltekster der opfyldte forespørgslen',
'notextmatches'             => 'Ingen artikeltekster opfyldte forespørgslen',
'prevn'                     => 'forrige $1',
'nextn'                     => 'næste $1',
'viewprevnext'              => 'Vis ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|et ord|$2 ord}})',
'search-result-score'       => 'Relevans: $1%',
'search-redirect'           => '(omdirriger $1)',
'search-section'            => '(sektion $1)',
'search-suggest'            => 'Mente du: $1',
'search-interwiki-caption'  => 'Søsterprojekter',
'search-interwiki-default'  => '{{PLURAL:$1|et resultat|$1 resultater}}:',
'search-interwiki-more'     => '(mere)',
'search-mwsuggest-enabled'  => 'med forslag',
'search-mwsuggest-disabled' => 'ingen forslag',
'search-relatedarticle'     => 'Relateret',
'mwsuggest-disable'         => 'Slå AJAX-forslag fra',
'searchrelated'             => 'relateret',
'searchall'                 => 'alle',
'showingresults'            => 'Nedenfor vises <b>$1</b> {{PLURAL:$1|resultat|resultater}} startende med nummer <b>$2</b>.',
'showingresultsnum'         => 'Herunder vises <b>$3</b> {{PLURAL:$3|resultat|resultater}} startende med nummer <b>$2</b>.',
'showingresultstotal'       => "Viser {{PLURAL:$3|resultat '''$1''' af '''$3'''|resultaterne '''$1 - $2''' af '''$3'''}}",
'nonefound'                 => '<strong>Bemærk</strong>: Søgning uden resultat skyldes at man søger efter almindelige ord som "har" og "fra", der ikke er indekseret, eller at man har angivet mere end ét søgeord (da kun sider indeholdende alle søgeordene vil blive fundet).',
'powersearch'               => 'Søg',
'powersearch-legend'        => 'Avanceret søgning',
'powersearch-ns'            => 'Søg i navnerummene:',
'powersearch-redir'         => 'Vis omdirrigeringer',
'powersearch-field'         => 'Søg efter',
'search-external'           => 'Brug anden søgemaskine',
'searchdisabled'            => '<p>Beklager! Fuldtekstsøgningen er midlertidigt afbrudt på grund af for stor belastning på serverne. I mellemtidem kan du anvende Google- eller Yahoo!-søgefelterne herunder. Bemærk at deres kopier af {{SITENAME}}s indhold kan være forældet.</p>',

# Preferences page
'preferences'              => 'Indstillinger',
'mypreferences'            => 'Indstillinger',
'prefs-edits'              => 'Antal redigeringer:',
'prefsnologin'             => 'Ikke logget på',
'prefsnologintext'         => 'Du skal være [[Special:UserLogin|logget på]] for at ændre brugerindstillinger.',
'prefsreset'               => 'Indstillingerne er blevet gendannet fra lageret.',
'qbsettings'               => 'Hurtigmenu',
'qbsettings-none'          => 'Ingen',
'qbsettings-fixedleft'     => 'Fast venstre',
'qbsettings-fixedright'    => 'Fast højre',
'qbsettings-floatingleft'  => 'Flydende venstre',
'qbsettings-floatingright' => 'Flydende højre',
'changepassword'           => 'Skift adgangskode',
'skin'                     => 'Udseende',
'math'                     => 'Matematiske formler',
'dateformat'               => 'Datoformat',
'datedefault'              => 'Standard',
'datetime'                 => 'Dato og klokkeslet',
'math_failure'             => 'Fejl i matematikken',
'math_unknown_error'       => 'ukendt fejl',
'math_unknown_function'    => 'ukendt funktion',
'math_lexing_error'        => 'lexerfejl',
'math_syntax_error'        => 'syntaksfejl',
'math_image_error'         => 'PNG-konvertering mislykkedes; undersøg om latex, dvips, gs og convert er installeret korrekt',
'math_bad_tmpdir'          => 'Kan ikke skrive til eller oprette temp-mappe til math',
'math_bad_output'          => 'Kan ikke skrive til eller oprette uddata-mappe til math',
'math_notexvc'             => 'Manglende eksekvérbar texvc; se math/README for opsætningsoplysninger.',
'prefs-personal'           => 'Brugerdata',
'prefs-rc'                 => 'Seneste ændringer og artikelstumper',
'prefs-watchlist'          => 'Overvågningsliste',
'prefs-watchlist-days'     => 'Antal dage, som overvågningslisten standardmæssigt skal omfatte:',
'prefs-watchlist-edits'    => 'Antal redigeringer der vises i udvidet overvågningsliste:',
'prefs-misc'               => 'Forskelligt',
'saveprefs'                => 'Gem indstillinger',
'resetprefs'               => 'Gendan indstillinger',
'oldpassword'              => 'Gammel adgangskode',
'newpassword'              => 'Ny adgangskode',
'retypenew'                => 'Gentag ny adgangskode',
'textboxsize'              => 'Redigering',
'rows'                     => 'Rækker',
'columns'                  => 'Kolonner',
'searchresultshead'        => 'Søgeresultater',
'resultsperpage'           => 'Resultater pr. side',
'contextlines'             => 'Linjer pr. resultat',
'contextchars'             => 'Tegn pr. linje i resultatet',
'stub-threshold'           => 'Grænse for visning af henvisning som <a href="#" class="stub">artikelstump</a>:',
'recentchangesdays'        => 'Antal dage, som listen „Sidste ændringer“ standardmæssigt skal omfatte:',
'recentchangescount'       => 'Antallet af titler på siden "seneste ændringer"',
'savedprefs'               => 'Dine indstillinger er blevet gemt.',
'timezonelegend'           => 'Tidszone',
'timezonetext'             => 'Indtast antal timer din lokale tid er forskellig fra serverens tid (UTC). Der bliver automatisk tilpasset til dansk tid, ellers skulle man for eksempel for dansk vintertid, indtaste "1" (og "2" når vi er på sommertid).',
'localtime'                => 'Lokaltid',
'timezoneoffset'           => 'Forskel',
'servertime'               => 'Serverens tid er nu',
'guesstimezone'            => 'Hent tidszone fra browseren',
'allowemail'               => 'Tillade E-mails fra andre brugere.',
'prefs-searchoptions'      => 'Søgeindstillinger',
'prefs-namespaces'         => 'Navnerum',
'defaultns'                => 'Søg som standard i disse navnerum:',
'default'                  => 'standard',
'files'                    => 'Filer',

# User rights
'userrights'                  => 'Forvaltning af brugerrettigheder', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Administrér brugergrupper',
'userrights-user-editname'    => 'Skriv et brugernavn:',
'editusergroup'               => 'Redigér brugergrupper',
'editinguser'                 => 'Redigerer $1',
'userrights-editusergroup'    => 'Redigér brugergrupper',
'saveusergroups'              => 'Gem brugergrupper',
'userrights-groupsmember'     => 'Medlem af:',
'userrights-groups-help'      => 'Du kan ændre denne brugers gruppermedlemsskaber::
* Et markeret afkrydsningsfelt betyder at brugeren er medlen af den pågældende gruppe..
* Et umarkeret felt betyder at brugeren ikke er medlem af gruppen.
* En * betyder at du ikke kan fravælge gruppen, når den først er tilføjet og omvendt.',
'userrights-reason'           => 'Årsag:',
'userrights-no-interwiki'     => 'Du kan ikke ændre brugerrettigheder på andre wikier.',
'userrights-nodatabase'       => 'Databasen $1 eksisterer ikke lokalt.',
'userrights-nologin'          => 'Du skal [[Special:UserLogin|logge på]] med en administrativ konto, før du kan ændre brugerettigheder.',
'userrights-notallowed'       => 'Din konto har ikke andgang til at ændre brugerrettigheder.',
'userrights-changeable-col'   => 'Redigerbare grupper',
'userrights-unchangeable-col' => 'Uredigerbare grupper',

# Groups
'group'               => 'Gruppe:',
'group-user'          => 'Brugere',
'group-autoconfirmed' => 'Registrerede brugere',
'group-bot'           => 'Robotter',
'group-sysop'         => 'Administratorer',
'group-bureaucrat'    => 'Bureaukrater',
'group-suppress'      => 'Logskjulere',
'group-all'           => '(alle)',

'group-user-member'          => 'bruger',
'group-autoconfirmed-member' => 'Registreret bruger',
'group-bot-member'           => 'Robot',
'group-sysop-member'         => 'Administrator',
'group-bureaucrat-member'    => 'Bureaukrat',
'group-suppress-member'      => 'logskjuler',

'grouppage-user'          => '{{ns:project}}:Brugere',
'grouppage-autoconfirmed' => '{{ns:project}}:Registrerede brugere',
'grouppage-bot'           => '{{ns:project}}:Robotter',
'grouppage-sysop'         => '{{ns:project}}:Administratorer',
'grouppage-bureaucrat'    => '{{ns:project}}:Bureaukrater',
'grouppage-suppress'      => '{{ns:project}}:Logskjulere',

# Rights
'right-read'                 => 'Se sider',
'right-edit'                 => 'Ændre sider',
'right-createpage'           => 'Oprette andre sider end diskussionssider',
'right-createtalk'           => 'Oprette diskussionssider',
'right-createaccount'        => 'Oprette nye brugere',
'right-minoredit'            => 'Markere redigeringer som mindre',
'right-move'                 => 'Flytte sider',
'right-move-subpages'        => 'Flytte sider og undersider',
'right-suppressredirect'     => 'Flytte sider uden at oprette en omdirigering fra det gamle navn',
'right-upload'               => 'Lægge filer op',
'right-reupload'             => 'Overskrive en allerede eksisterende fil',
'right-reupload-own'         => 'Overskrive en eksisterende fil brugeren selv har lagt op',
'right-reupload-shared'      => 'Lægge en lokal fil op, selvom den allerede findes centralt',
'right-upload_by_url'        => 'Lægge en fil op fra en URL',
'right-purge'                => 'Nulstille sidens cache uden bekræftelse',
'right-autoconfirmed'        => 'Ændre delvist beskyttede sider',
'right-bot'                  => 'Redigeringer markeres som robot',
'right-nominornewtalk'       => 'Mindre ændringer på diskussionssider markerer ikke disse med nyt indhold',
'right-apihighlimits'        => 'Bruge højere grænser i API',
'right-writeapi'             => 'Bruge redigeringsdelen af API',
'right-delete'               => 'Slette sider',
'right-bigdelete'            => 'Slette sider med mange versioner',
'right-deleterevision'       => 'Slette og gendanne enkelte versioner af sider',
'right-deletedhistory'       => 'Se slettede verioner, uden at vise versionens indhold.',
'right-browsearchive'        => 'Søge i slettede sider',
'right-undelete'             => 'Gendanne en side',
'right-suppressrevision'     => 'Skjule og synliggøre sletninger for administratorer',
'right-suppressionlog'       => 'Se skjulte loglister',
'right-block'                => 'Blokere brugere',
'right-blockemail'           => 'Blokere en brugers mulighed for at sende mail',
'right-hideuser'             => 'Blokere et brugernavn og skjule navnet',
'right-ipblock-exempt'       => 'Redigere fra blokerede IP-adresser',
'right-proxyunbannable'      => 'Redigere gennem automatisk blokeret proxy',
'right-protect'              => 'Ændre beskyttelse og redigere beskyttede sider',
'right-editprotected'        => 'Ændre beskyttede sider (uden nedarvet sidebeskyttelse)',
'right-editinterface'        => 'Ændre brugergrænsefladens tekster',
'right-editusercssjs'        => 'Ændre andre brugeres JS og CSS filer',
'right-rollback'             => 'Hurtig gendannelse af alle redigeringer foretaget af den seneste bruger',
'right-markbotedits'         => 'Markere gendannelser som ændringer foretaget af en robot',
'right-noratelimit'          => 'Upåvirket af hastighedsgrænser',
'right-import'               => 'Importere sider fra andre wikier',
'right-importupload'         => 'Importere sider fra en uploadet fil',
'right-patrol'               => 'Markere andres ændringer som patruljeret',
'right-autopatrol'           => 'Egne redigeringer vises automatisk som patruljerede',
'right-patrolmarks'          => 'Se de seneste patruljeringer',
'right-unwatchedpages'       => 'Se hvilke sider der ikke er på en brugers overvågningsliste',
'right-trackback'            => 'Tilføje trackback',
'right-mergehistory'         => 'Sammenflette sidehistorik',
'right-userrights'           => 'Ændre alle brugerrettigheder',
'right-userrights-interwiki' => 'Ændre brugerrettigheder på andre wikier',
'right-siteadmin'            => 'Låse og frigive databasen',

# User rights log
'rightslog'      => 'Rettigheds-logbog',
'rightslogtext'  => 'Dette er en log over ændringer i brugeres rettigheder.',
'rightslogentry' => 'ændrede grupperettigheder for „[[$1]]“ fra „$2“ til „$3“.',
'rightsnone'     => '(-)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|ændring|ændringer}}',
'recentchanges'                     => 'Seneste ændringer',
'recentchangestext'                 => "På denne side kan du følge de seneste ændringer på '''{{SITENAME}}'''.",
'recentchanges-feed-description'    => 'Med dette feed kan du følge de seneste ændringer på {{SITENAME}}.',
'rcnote'                            => "Herunder ses {{PLURAL:$1|'''1''' ændring|de sidste '''$1''' ændringer}} fra {{PLURAL:$2|i dag|de sidste '''$2''' dage}} fra den $4, kl. $5.",
'rcnotefrom'                        => 'Nedenfor ses ændringerne fra <b>$2</b> til <b>$1</b> vist.',
'rclistfrom'                        => 'Vis nye ændringer startende fra $1',
'rcshowhideminor'                   => '$1 mindre ændringer',
'rcshowhidebots'                    => '$1 robotter',
'rcshowhideliu'                     => '$1 registrerede brugere',
'rcshowhideanons'                   => '$1 anonyme brugere',
'rcshowhidepatr'                    => '$1 kontrollerede ændringer',
'rcshowhidemine'                    => '$1 egne bidrag',
'rclinks'                           => 'Vis seneste $1 ændringer i de sidste $2 dage<br />$3',
'diff'                              => 'forskel',
'hist'                              => 'historik',
'hide'                              => 'skjul',
'show'                              => 'vis',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|overvågende bruger|overvågende brugere}}]',
'rc_categories'                     => 'Kun sider fra kategorierne (adskilt med „|“):',
'rc_categories_any'                 => 'Alle',
'rc-change-size'                    => '$1 {{PLURAL:$1|Byte|Bytes}}',
'newsectionsummary'                 => '/* $1 */ nyt afsnit',

# Recent changes linked
'recentchangeslinked'          => 'Relaterede ændringer',
'recentchangeslinked-title'    => 'Ændringer der relaterer til $1',
'recentchangeslinked-noresult' => 'I det udvalgte tidsrum blev der ikke foretaget ændringer på siderne der henvises til.',
'recentchangeslinked-summary'  => "Denne specialside viser de seneste ændringer på de sider der henvises til. Sider på din overvågningsliste er vist med '''fed''' skrift.",
'recentchangeslinked-page'     => 'Side:',
'recentchangeslinked-to'       => 'Vis ændringer i sider der henviser til den angivne side i stedet',

# Upload
'upload'                      => 'Læg en fil op',
'uploadbtn'                   => 'Læg en fil op',
'reupload'                    => 'Læg en fil op igen',
'reuploaddesc'                => 'Tilbage til formularen til at lægge filer op.',
'uploadnologin'               => 'Ikke logget på',
'uploadnologintext'           => 'Du skal være [[Special:UserLogin|logget på]] for at kunne lægge filer op.',
'upload_directory_missing'    => 'upload-kataloget ($1) findes ikke. Webserveren har ikke mulighed for at oprette kataloget.',
'upload_directory_read_only'  => 'Webserveren har ingen skriverettigheder for upload-kataloget ($1).',
'uploaderror'                 => 'Fejl under oplægning af fil',
'uploadtext'                  => "<strong>STOP!</strong> Før du lægger filer op her, så vær sikker på du har læst og følger {{SITENAME}}s [[{{MediaWiki:Policy-url}}|politik om brug af billeder]]. Følg venligst disse retningslinjer: * Angiv tydeligt hvor filen stammer fra * Brug et beskrivende filnavn, så det er til at se hvad filen indeholder * Tjek i [[Special:ImageList|listen over filer]] om filen allerede er lagt op

Brug formularen herunder til at lægge nye filer op, som kan bruges i dine artikler.
På de fleste browsere vil du se en \"Browse...\" knap eller en \"Gennemse...\" knap, som vil bringe dig til dit styresystems standard-dialog til åbning af filer.
Når du vælger en fil, vil navnet på filen dukke op i tekstfeltet ved siden af knappen.
Du skal også bekræfte, at du ikke er ved at bryde nogens ophavsret. Det gør du ved at sætte et mærke i tjekboksen. Vælg \"Læg en fil op\"-knappen for at lægge filen op. Dette kan godt tage lidt tid hvis du har en langsom internetforbindelse. De foretrukne formater er JPEG til fotografiske billeder, PNG til tegninger og andre små billeder, og OGG til lyd.

For at bruge et billede i en artikel, så brug en henvisning af denne type
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:fil.jpg]]</nowiki>''' eller
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:fil.png|alternativ tekst]]</nowiki>''' eller

For at indlejre '''mediefiler''' ind, bruges f.eks.:
* '''<tt><nowiki>[[</nowiki>{{ns:media}}:fil.ogg<nowiki>]]</nowiki></tt>'''
* '''<tt><nowiki>[[</nowiki>{{ns:media}}:fil.ogg|Henvisningstekst<nowiki>]]</nowiki></tt>'''

Læg mærke til at præcis som med alle andre sider, så kan og må andre gerne redigere eller slette de filer, du har lagt op, hvis de mener det hjælper {{SITENAME}}, og du kan blive blokeret fra at lægge op hvis du misbruger systemet.",
'upload-permitted'            => 'Tilladte filtyper: $1.',
'upload-preferred'            => 'Foretrukne filtyper: $1.',
'upload-prohibited'           => 'Uønskede filtyper: $1.',
'uploadlog'                   => 'oplægningslog',
'uploadlogpage'               => 'Oplægningslog',
'uploadlogpagetext'           => 'Herunder vises de senest oplagte filer. Alle de viste tider er serverens tid.',
'filename'                    => 'Filnavn',
'filedesc'                    => 'Beskrivelse',
'fileuploadsummary'           => 'Beskrivelse/kilde:',
'filestatus'                  => 'Status på ophavsret',
'filesource'                  => 'Kilde',
'uploadedfiles'               => 'Filer som er lagt op',
'ignorewarning'               => 'Ignorerer advarsler og gemme fil.',
'ignorewarnings'              => 'Ignorerer advarsler',
'minlength1'                  => 'Navnet på filen skal være på mindst et bogstav.',
'illegalfilename'             => 'Filnavnet "$1" indeholder tegn, der ikke er tilladte i sidetitler. Omdøb filen og prøv at lægge den op igen.',
'badfilename'                 => 'Navnet på filen er blevet ændret til "$1".',
'filetype-badmime'            => 'Filer med MIME-typen „$1“ må ikke uploades.',
'filetype-unwanted-type'      => "'''\".\$1\"''' er ikke en foretrukken filtype. {{PLURAL:\$3|Den foretrukne filtype|De foretrukne filtyper}} er \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' er en uønsket filtype. {{PLURAL:\$3|Den tilladte filtype|De tillatdte filtyper}} er \$2.",
'filetype-missing'            => 'Filen der skal uploades har ingen endelse (f.eks. „.jpg“).',
'large-file'                  => 'Filstørrelsen skal så vidt muligt ikke overstige $1. Denne fil er $2 stor.',
'largefileserver'             => 'Filen er større end den på serveren indstillede maksimale størrelse.',
'emptyfile'                   => 'Filen du lagde op lader til at være tom. Det kan skyldes en slåfejl i filnavnet. Kontroller om du virkelig ønsker at lægge denne fil op.',
'fileexists'                  => 'En fil med det navn findes allerede, tjek venligst $1 om du er sikker på du vil ændre den.',
'filepageexists'              => 'Siden med beskrivelse af denne fil er allerede oprettet på <strong><tt>$1</tt></strong>, men der eksisterer ikke en fil med dette navn. Den beskrivelse du kan angive nedenfor vil derfor ikke blive brugt. For at få din beskrivelse vist, skal du selv redigere beskrivelsessiden.',
'fileexists-extension'        => 'En fil med lignende navn findes allerede:<br />
Navnet på den valgte fil: <strong><tt>$1</tt></strong><br />
Navnet på den eksisterende fil: <strong><tt>$2</tt></strong><br />
Kun filendelsen adskiller sig med store og små bogstaver. Kontroller venligst om filerne har samme indhold.',
'fileexists-thumb'            => "<center>'''Eksisterende billede'''</center>",
'fileexists-thumbnail-yes'    => 'Det ser ud som om filen indeholder et billede i reduceret størrelse <i>(thumbnail)</i>. Kontroller filen <strong><tt>$1</tt></strong>.<br />
Hvis det er billedet i original størrelse, er det ikke nødvendigt at uploade et separat forhåndsvisningsbillede.',
'file-thumbnail-no'           => 'Filnavnet begynder med <strong><tt>$1</tt></strong>. Det tyder på et billede i reduceret format <i>(thumbnail)</i>.
Kontroller om du har billedet i fuld størrelse og upload det under det originale navn.',
'fileexists-forbidden'        => 'Der findes allerede en fil med dette navn. Gå tilbage og upload filen under et andet navn. [[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Der findes allerede en fil med dette navn. Gå tilbage og upload filen under et andet navn. [[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Denne fil er en bublet af {{PLURAL:$1|den nedenstående fil|de nedenstående $1 filer}}:',
'successfulupload'            => 'Oplægningen er gennemført',
'uploadwarning'               => 'Advarsel',
'savefile'                    => 'Gem fil',
'uploadedimage'               => 'Lagde "[[$1]]" op',
'overwroteimage'              => 'Lagde en ny version af "[[$1]]" op',
'uploaddisabled'              => 'Desværre er funktionen til at lægge billeder op afbrudt på denne server.',
'uploaddisabledtext'          => 'Upload af filer er deaktiveret på {{SITENAME}}.',
'uploadscripted'              => 'Denne fil indeholder HTML eller script-kode, der i visse tilfælde can fejlfortolkes af en browser.',
'uploadcorrupt'               => 'Denne fil er beskadiget eller forsynet med en forkert endelse. Kontroller venligst filen og prøv at lægge den op igen.',
'uploadvirus'                 => 'Denne fil indeholder en virus! Virusnavn: $1',
'sourcefilename'              => 'Kildefil',
'destfilename'                => 'Målnavn',
'upload-maxfilesize'          => 'Maksimal filstørrelse: $1',
'watchthisupload'             => 'Overvåge denne side',
'filewasdeleted'              => 'En fil med dette navn er tidligere uploadet og i mellemtiden slettet igen. Kontroller først indførslen i $1, før du gemmer filen.',
'upload-wasdeleted'           => "'''Advarsel: Du er ved at uploade en fil der tidligere er blevet slettet.'''

Overvej om det er passende at fortsætte med uploadet.
Sletningsloggen for denne fil er gengivet herunder.",
'filename-bad-prefix'         => 'Navnet på filen du er ved at lægge op begynder med <strong>"$1"</strong>. Dette er et ikkebeskrivende navn, der typisk er skabt automatisk af et digitalkamera. Vær venlig at vælge et mere beskrivende navn på dit billede.',

'upload-proto-error'      => 'Forkert protokol',
'upload-proto-error-text' => 'Adressen skal begynde med <code>http://</code> eller <code>ftp://</code>.',
'upload-file-error'       => 'Intern fejl',
'upload-file-error-text'  => 'Ved oprettelse af en midlertidig fil på serveren, er der sket en fejl. Informer venligst en system-administrator.',
'upload-misc-error'       => 'Ukendt fejl ved upload',
'upload-misc-error-text'  => 'Ved upload er der sket en ukendt fejl. Kontroller adressen for fejl, sidens onlinestatus og forsøg igen. Hvis problemet fortsætter, informeres en system-administrator.',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'URL er utilgængelig',
'upload-curl-error6-text'  => 'Den angivne URL er ikke tilgængelig. Kontroller adressen for fejl samt sidens onlinestatus .',
'upload-curl-error28'      => 'Tidsoverskridelse ved upload',
'upload-curl-error28-text' => 'Siden er for længe om at svare. Kontroller om siden er online, vent et øjeblik og prøv igen. Det kan måske hjælpe at prøve på et senere tidspunkt.',

'license'            => 'Licens',
'nolicense'          => 'intet forvalg',
'license-nopreview'  => '(forhåndsvisning ikke mulig)',
'upload_source_url'  => ' (gyldig, offentligt tillgængelig URL)',
'upload_source_file' => ' (en fil på din computer)',

# Special:ImageList
'imagelist-summary'     => 'Denne specialside viser alle uploadede filer. Standardmæssigt vises de sidst uploadede filer først. Med et klik på spalteoverskriften kan sorteringen vendes om eller der kan sorteres efter en anden spalte.',
'imagelist_search_for'  => 'Søge efter fil:',
'imgfile'               => 'Fil',
'imagelist'             => 'Billedliste',
'imagelist_date'        => 'Dato',
'imagelist_name'        => 'Navn',
'imagelist_user'        => 'Bruger',
'imagelist_size'        => 'Størrelse (Byte)',
'imagelist_description' => 'Beskrivelse',

# Image description page
'filehist'                       => 'Filhistorik',
'filehist-help'                  => 'Klik på en dato/tid for at se den version af filen.',
'filehist-deleteall'             => 'slet alle',
'filehist-deleteone'             => 'slet denne',
'filehist-revert'                => 'gendan',
'filehist-current'               => 'nuværende',
'filehist-datetime'              => 'Dato/tid',
'filehist-user'                  => 'Bruger',
'filehist-dimensions'            => 'Dimensioner',
'filehist-filesize'              => 'Filstørrelse',
'filehist-comment'               => 'Kommentar',
'imagelinks'                     => 'Billedehenvisninger',
'linkstoimage'                   => '{{Plural:$1|Den følgende side|De følgende $1 sider}} henviser til dette billede:',
'nolinkstoimage'                 => 'Der er ingen sider der henviser til dette billede.',
'morelinkstoimage'               => 'Se [[Special:WhatLinksHere/$1|flere henvisninger]] til denne fil.',
'redirectstofile'                => '{{PLURAL:$1|Nedenstående fil|De nedenstående $1 filer}} er en omdirigering til denne fil:',
'duplicatesoffile'               => '{{PLURAL:$1|Nedenstående fil|De nedenstående $1 filer}} er dubletter af denne fil:',
'sharedupload'                   => 'Denne fil er en fælles upload og kan bruges af andre projekter.',
'shareduploadwiki'               => 'Se venligst $1 for yderligere information.',
'shareduploadwiki-desc'          => 'Dens beskrivelse på $1 vises nedenfor.',
'shareduploadwiki-linktext'      => 'siden med billedbeskrivelsen',
'shareduploadduplicate'          => 'Denne fil er en dublet af $1 fra den delte server.',
'shareduploadduplicate-linktext' => 'anden fil',
'shareduploadconflict'           => 'Denne fil har samme navn som $1 fra den delte server.',
'shareduploadconflict-linktext'  => 'anden fil',
'noimage'                        => 'Der eksisterer ingen fil med dette navn, du kan $1',
'noimage-linktext'               => 'lægge den op',
'uploadnewversion-linktext'      => 'Læg en ny version af denne fil op',
'imagepage-searchdupe'           => 'Søg efter dubletfiler',

# File reversion
'filerevert'                => 'Gendan $1',
'filerevert-legend'         => 'Gendan fil',
'filerevert-intro'          => '<span class="plainlinks">Du gendanner \'\'\'[[Media:$1|$1]]\'\'\' til [$4 version fra $2, $3].</span>',
'filerevert-comment'        => 'Kommentar:',
'filerevert-defaultcomment' => 'Gendannet til version fra $1, $2',
'filerevert-submit'         => 'Gendan',
'filerevert-success'        => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\' er gendannet til [$4 version fra $2, $3].</span>',
'filerevert-badversion'     => 'Der findes ingen lokal udgave af denne fil med det opgivne tidsstempel.',

# File deletion
'filedelete'                  => 'Slet $1',
'filedelete-legend'           => 'Slet fil',
'filedelete-intro'            => "Du er ved at slette '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => '<span class="plainlinks">Du er ved at slette en tidligere version af \'\'\'[[Media:$1|$1]]\'\'\' fra [$4 $2, $3].</span>',
'filedelete-comment'          => 'Kommentar:',
'filedelete-submit'           => 'Slet',
'filedelete-success'          => "'''$1''' er blevet slettet.",
'filedelete-success-old'      => '<span class="plainlinks">En gamllem version af \'\'\'[[Media:$1|$1]]\'\'\' fra $2, $3 er blevet slettet.</span>',
'filedelete-nofile'           => "'''$1''' findes ikke på dette websted.",
'filedelete-nofile-old'       => "Der findes ikke en version af '''$1''' fra $2, $3.",
'filedelete-iscurrent'        => 'Du har forsøgt at slette den nyeste version. Gendan en tidligere udgave først.',
'filedelete-otherreason'      => 'Anden/uddybende begrundelse:',
'filedelete-reason-otherlist' => 'Anden begrundelse',
'filedelete-reason-dropdown'  => '*Hyppige sletningsbegrundelser
** Ophavsretskrænkelse
** Dubletfil
** Filen er ubrugt',
'filedelete-edit-reasonlist'  => 'Rediger sletningsårsager',

# MIME search
'mimesearch'         => 'Søge efter MIME-type',
'mimesearch-summary' => 'På denne specialside kan filerne filtreres efter MIME-typen. Indtastningen skal altid indeholde medie- og undertypen: <tt>image/jpeg</tt> (se billedbeskrivelsessiden).',
'mimetype'           => 'MIME-type:',
'download'           => 'DownloadHerunterladen',

# Unwatched pages
'unwatchedpages' => 'Ikke overvågede sider',

# List redirects
'listredirects' => 'Henvisningsliste',

# Unused templates
'unusedtemplates'     => 'Ikke brugte skabeloner',
'unusedtemplatestext' => 'Her opremses alle sider i skabelonnavnerummet, der ikke er indeholdt i andre sider. Husk at kontrollere for andre henvisninger til skabelonerne, før de slettes.',
'unusedtemplateswlh'  => 'andre henvisninger',

# Random page
'randompage'         => 'Tilfældig artikel',
'randompage-nopages' => 'I dette navnerum findes ingen sider.',

# Random redirect
'randomredirect'         => 'Tilfældige henvisninger',
'randomredirect-nopages' => 'I dette navnerum findes ingen henvisninger.',

# Statistics
'statistics'             => 'Statistik',
'sitestats'              => 'Statistiske oplysninger om {{SITENAME}}',
'userstats'              => 'Statistik om brugere på {{SITENAME}}',
'sitestatstext'          => "Der er {{PLURAL:\$1|'''1''' side|'''\$1''' sider ialt}} i databasen.
Dette tal indeholder \"diskussion\"-sider, sider om {{SITENAME}}, minimale \"stub\"
sider, omdirigeringssider og andre sider der sikkert ikke kan kaldes artikler.
Hvis man udelader disse, så er der {{PLURAL:\$2|'''1''' side|'''\$2''' sider}} som sandsynligvis er virkelige
indholds{{PLURAL:\$2|side|sider}}. 

'''\$8''' {{PLURAL:\$8|fil|filer}} er blevet uploadet.

Der har været ialt '''\$3''' {{PLURAL:\$3|sidevisning|sidevisninger}}, og '''\$4''' {{PLURAL:\$4|sideændring|sideændringer}}
siden {{SITENAME}} blev oprettet.
Det bliver til '''\$5''' gennemsnitlige ændringer pr. side, og '''\$6''' visninger pr. ændring.

[http://meta.wikimedia.org/wiki/Help:Job_queue job queue] længden er '''\$7'''.",
'userstatstext'          => "Der findes '''$1''' {{PLURAL:$1|registreret|registrerede}} [[Special:ListUsers|brugere]].
deraf har '''$2''' (=$4%) $5-rettigheder.",
'statistics-mostpopular' => 'Mest besøgte sider',

'disambiguations'      => 'Artikler med flertydige titler',
'disambiguationspage'  => 'Template:Flertydig',
'disambiguations-text' => 'De følgende sider henviser til en flertydig titel. De bør henvise direkte til det passende emne i stedet. En side behandles som en side med en flertydig titel hvis den bruger en skabelon som er henvist til fra [[MediaWiki:Disambiguationspage]].',

'doubleredirects'            => 'Dobbelte omdirigeringer',
'doubleredirectstext'        => '<b>Bemærk:</b> Denne liste kan indeholde forkerte resultater. Det er som regel, fordi siden indeholder ekstra tekst under den første #REDIRECT.<br /> Hver linje indeholder henvisninger til den første og den anden omdirigering, og den første linje fra den anden omdirigeringstekst, det giver som regel den "rigtige" målartikel, som den første omdirigering skulle have peget på.',
'double-redirect-fixed-move' => '[[$1]] blev flyttet og er nu en omdirigering til [[$2]]',
'double-redirect-fixer'      => 'Omdirrigerings-retter',

'brokenredirects'        => 'Defekte omdirigeringer',
'brokenredirectstext'    => 'De følgende omdirigeringer peger på en side der ikke eksisterer.',
'brokenredirects-edit'   => '(rediger)',
'brokenredirects-delete' => '(slet)',

'withoutinterwiki'         => 'Sider uden henvisninger til andre sprog',
'withoutinterwiki-summary' => 'Følgende sider henviser ikke til andre sprog.',
'withoutinterwiki-legend'  => 'Præfiks',
'withoutinterwiki-submit'  => 'Vis',

'fewestrevisions' => 'Sider med de færreste versioner',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|kategori|kategorier}}',
'nlinks'                  => '{{PLURAL:$1|1 henvisning|$1 henvisninger}}',
'nmembers'                => '– {{PLURAL:$1|1 artikel|$1 artikler}}',
'nrevisions'              => '{{PLURAL:$1|1 ændring|$1 ændringer}}',
'nviews'                  => '{{PLURAL:$1|1 visning|$1 visninger}}',
'specialpage-empty'       => 'Siden indeholder i øjeblikket ingen artikler.',
'lonelypages'             => 'Forældreløse artikler',
'lonelypagestext'         => 'De følgende sider har ikke henvisninger fra andre sider i denne wiki.',
'uncategorizedpages'      => 'Ukategoriserede sider',
'uncategorizedcategories' => 'Ukategoriserede kategorier',
'uncategorizedimages'     => 'Ikke kategoriserede filer',
'uncategorizedtemplates'  => 'Ikke kategoriserede skabeloner',
'unusedcategories'        => 'Ubrugte kategorier',
'unusedimages'            => 'Ubrugte billeder',
'popularpages'            => 'Populære artikler',
'wantedcategories'        => 'Brugte men ikke anlagte kategorier',
'wantedpages'             => 'Ønskede artikler',
'missingfiles'            => 'Manglende filer',
'mostlinked'              => 'Sider med flest henvisninger',
'mostlinkedcategories'    => 'Mest brugte kategorier',
'mostlinkedtemplates'     => 'Hyppigst brugte skabeloner',
'mostcategories'          => 'Mest brugte sider',
'mostimages'              => 'Mest brugte filer',
'mostrevisions'           => 'Sider med de fleste ændringer',
'prefixindex'             => 'Alle sider (med præfiks)',
'shortpages'              => 'Korte artikler',
'longpages'               => 'Lange artikler',
'deadendpages'            => 'Blindgydesider',
'deadendpagestext'        => 'De følgende sider henviser ikke til andre sider i denne wiki.',
'protectedpages'          => 'Skrivebeskyttede sider',
'protectedpages-indef'    => 'Kun beskyttelser uden udløbadato',
'protectedpagestext'      => 'De følgende sider er beskyttede mod redigering og flytning.',
'protectedpagesempty'     => 'I øjeblikket er ingen sider beskyttet på denne måde.',
'protectedtitles'         => 'Beskyttede sidenavne',
'protectedtitlestext'     => 'Disse sidenavne er beskyttet mod at blive oprettet',
'protectedtitlesempty'    => 'Der er ingen sidetitler der er beskyttet med disse arametre.',
'listusers'               => 'Brugerliste',
'newpages'                => 'Nyeste artikler',
'newpages-username'       => 'Brugernavn:',
'ancientpages'            => 'Ældste artikler',
'move'                    => 'Flyt',
'movethispage'            => 'Flyt side',
'unusedimagestext'        => '<p>Læg mærke til, at andre websider såsom de andre internationale {{SITENAME}}er måske henviser til et billede med en direkte URL, så det kan stadig være listet her, selvom det er i aktivt brug.',
'unusedcategoriestext'    => 'Denne specialside viser alle kategorier, som ikke selv er henført til en kategori.',
'notargettitle'           => 'Sideangivelse mangler',
'notargettext'            => 'Du har ikke angivet en side eller bruger at udføre denne funktion på.',
'nopagetitle'             => 'Siden findes ikke',
'nopagetext'              => 'Den angivne side findes ikke.',
'pager-newer-n'           => '{{PLURAL:$1|1 nyere|$1 nyere}}',
'pager-older-n'           => '{{PLURAL:$1|1 ældre|$1 ældre}}',
'suppress'                => 'Skjul logs',

# Book sources
'booksources'               => 'Bogkilder',
'booksources-search-legend' => 'Søgning efter bøger',
'booksources-go'            => 'Søg',
'booksources-text'          => 'Dette er en liste med henvisninger til Internetsider, som sælger nye og brugte bøger. Der kan der også findes yderligere informationer om bøgerne. {{SITENAME}} er ikke forbundet med nogen af dem.',

# Special:Log
'specialloguserlabel'  => 'Bruger:',
'speciallogtitlelabel' => 'Titel:',
'log'                  => 'Loglister',
'all-logs-page'        => 'Alle loglister',
'log-search-legend'    => 'Gennemsøg logs',
'log-search-submit'    => 'Søg',
'alllogstext'          => 'Samlet visning af oplægningslog, sletningslog, blokeringslog, bureaukratlog og listen over beskyttede sider. Du kan sortere i visningen ved at vælge type, brugernavn og/eller en udvalgt side.',
'logempty'             => 'Intet passende fundet.',
'log-title-wildcard'   => 'Titel begynder med …',

# Special:AllPages
'allpages'          => 'Alle artikler',
'alphaindexline'    => '$1 til $2',
'nextpage'          => 'Næste side ($1)',
'prevpage'          => 'Forrige side ($1)',
'allpagesfrom'      => 'Vis sider startende fra:',
'allarticles'       => 'Alle artikler',
'allinnamespace'    => 'Alle sider (i $1 navnerummet)',
'allnotinnamespace' => 'Alle sider (ikke i $1 navnerummet)',
'allpagesprev'      => 'Forrige',
'allpagesnext'      => 'Næste',
'allpagessubmit'    => 'Vis',
'allpagesprefix'    => 'Vis sider med præfiks:',
'allpagesbadtitle'  => 'Det indtastede sidenavn er ugyldigt: Det har enten et foranstillet sprog-, en Interwiki-forkortelse eller indeholder et eller flere tegn, som ikke må anvendes i sidenavne.',
'allpages-bad-ns'   => 'Navnerummet $1 findes ikke på {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Kategorier',
'categoriespagetext'            => '{{SITENAME}} har følgende kategorier.',
'categoriesfrom'                => 'Vis kategorier startende med:',
'special-categories-sort-count' => 'sorter efter antal',
'special-categories-sort-abc'   => 'sorter alfabetisk',

# Special:ListUsers
'listusersfrom'      => 'Vis brugere fra:',
'listusers-submit'   => 'Vis',
'listusers-noresult' => 'Ingen bruger fundet.',

# Special:ListGroupRights
'listgrouprights'          => 'Brugergrupperettigheder',
'listgrouprights-summary'  => 'Denne side vider de brugergrupper der er defineret på denne wiki og de enkelte gruppers rettigheder.

Der findes muligvis [[{{MediaWiki:Listgrouprights-helppage}}|yderligere information]] om de enkelte rettigheder.',
'listgrouprights-group'    => 'Gruppe',
'listgrouprights-rights'   => 'Rettigheder',
'listgrouprights-helppage' => 'Help:Grupperettigheder',
'listgrouprights-members'  => '(vis medlemmer)',

# E-mail user
'mailnologin'     => 'Du er ikke logget på',
'mailnologintext' => 'Du skal være [[Special:UserLogin|logget på]] og have en gyldig e-mailadresse sat i dine [[Special:Preferences|indstillinger]] for at sende e-mail til andre brugere.',
'emailuser'       => 'E-mail til denne bruger',
'emailpage'       => 'E-mail bruger',
'emailpagetext'   => 'Hvis denne bruger har sat en gyldig e-mail-adresse i sine brugerindstillinger, så vil formularen herunder sende en enkelt besked. Den e-mailadresse, du har sat i dine brugerindstillinger, vil dukke op i "Fra" feltet på denne mail, så modtageren er i stand til at svare.',
'usermailererror' => 'E-mail-modulet returnerede en fejl:',
'defemailsubject' => 'W-mail fra {{SITENAME}}',
'noemailtitle'    => 'Ingen e-mail-adresse',
'noemailtext'     => 'Denne bruger har ikke angivet en gyldig e-mail-adresse, eller har valgt ikke at modtage e-mail fra andre brugere.',
'emailfrom'       => 'Fra:',
'emailto'         => 'Til:',
'emailsubject'    => 'Emne:',
'emailmessage'    => 'Besked:',
'emailsend'       => 'Send',
'emailccme'       => 'Send en kopi af denne E-mail til mig',
'emailccsubject'  => 'Kopi sendes til $1: $2',
'emailsent'       => 'E-mail sendt',
'emailsenttext'   => 'Din e-mail er blevet sendt.',
'emailuserfooter' => 'Denne e-mail er sendt af $1 til $2 ved hjælp af funktionen "E-mail til denne bruger" på {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Overvågningsliste',
'mywatchlist'          => 'Overvågningsliste',
'watchlistfor'         => "(for '''$1''')",
'nowatchlist'          => 'Du har ingenting i din overvågningsliste.',
'watchlistanontext'    => 'Du skal $1, for at se din overvågningsliste eller ændre indholdet af den.',
'watchnologin'         => 'Ikke logget på',
'watchnologintext'     => 'Du skal være [[Special:UserLogin|logget på]] for at kunne ændre din overvågningsliste.',
'addedwatch'           => 'Tilføjet til din overvågningsliste',
'addedwatchtext'       => "Siden \"[[:\$1]]\" er blevet tilføjet til din [[Special:Watchlist|overvågningsliste]]. Fremtidige ændringer til denne side og den tilhørende diskussionsside vil blive listet der, og siden vil fremstå '''fremhævet''' i [[Special:RecentChanges|listen med de seneste ændringer]] for at gøre det lettere at finde den. Hvis du senere vil fjerne siden fra din overvågningsliste, så klik \"Fjern overvågning\".",
'removedwatch'         => 'Fjernet fra overvågningsliste',
'removedwatchtext'     => 'Siden "$1" er blevet fjernet fra din overvågningsliste.',
'watch'                => 'Overvåg',
'watchthispage'        => 'Overvåg side',
'unwatch'              => 'Fjern overvågning',
'unwatchthispage'      => 'Fjern overvågning',
'notanarticle'         => 'Ikke en artikel',
'notvisiblerev'        => 'Versionen er blevet slettet',
'watchnochange'        => 'Ingen af siderne i din overvågningsliste er ændret i den valgte periode.',
'watchlist-details'    => 'Du har $1 {{PLURAL:$1|side|sider}} på din overvågningsliste (ekskl. diskussionssider).',
'wlheader-enotif'      => '* E-mail underretning er slået til.',
'wlheader-showupdated' => "* Sider der er ændret siden dit sidste besøg er '''fremhævet'''",
'watchmethod-recent'   => 'Tjekker seneste ændringer for sider i din overvågningsliste',
'watchmethod-list'     => 'Tjekker seneste ændringer for sider i din overvågningsliste',
'watchlistcontains'    => 'Din overvågningsliste indeholder $1 {{PLURAL:$1|side|sider}}.',
'iteminvalidname'      => "Problem med '$1', ugyldigt navn...",
'wlnote'               => "Nedenfor ses de seneste $1 {{PLURAL:$1|ændring|ændringer}} i {{PLURAL:$2|den sidste time|'''de sidste $2 timer}}'''.",
'wlshowlast'           => 'Vis de seneste $1 timer $2 dage $3',
'watchlist-show-bots'  => 'Vise bot-ændringer',
'watchlist-hide-bots'  => 'Skjule bot-ændringer',
'watchlist-show-own'   => 'vise egne ændringer',
'watchlist-hide-own'   => 'skjule egne ændringer',
'watchlist-show-minor' => 'vise små ændringer',
'watchlist-hide-minor' => 'skjule små ændringer',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Overvåge …',
'unwatching' => 'Ikke overvåge …',

'enotif_mailer'                => '{{SITENAME}} informationsmail',
'enotif_reset'                 => 'Marker alle sider som besøgt',
'enotif_newpagetext'           => 'Dette er en ny side.',
'enotif_impersonal_salutation' => '{{SITENAME}} bruger',
'changed'                      => 'ændret',
'created'                      => 'oprettet',
'enotif_subject'               => '{{SITENAME}}-siden $PAGETITLE er blevet ændret af $PAGEEDITOR',
'enotif_lastvisited'           => 'Se $1 for alle ændringer siden dit sidste besøg.',
'enotif_lastdiff'              => 'Se $1 for at vise denne ændring.',
'enotif_anon_editor'           => 'anonym bruger $1',
'enotif_body'                  => 'Kære $WATCHINGUSERNAME,

{{SITENAME}}-siden $PAGETITLE er blevet ændret den $PAGEEDITDATE af $PAGEEDITOR, se $PAGETITLE_URL for den nyeste version.


$NEWPAGE

Bidragyderens beskrivelse: $PAGESUMMARY $PAGEMINOREDIT
Kontakt bidragyderen:
mail $PAGEEDITOR_EMAIL
wiki $PAGEEDITOR_WIKI

Du vil ikke modtage flere beskeder om yderligere ændringer af denne side med mindre du besøger den. På din overvågningsliste kan du også nulstille alle markeringer på de sider, du overvåger.

             Med venlig hilsen {{SITENAME}}s informationssystem
--
Besøg {{fullurl:Special:Watchlist/edit}} for at ændre indstillingerne for din overvågningsliste

Tilbagemelding og yderligere hjælp:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Slet side',
'confirm'                     => 'Bekræft',
'excontent'                   => "indholdet var: '$1'",
'excontentauthor'             => "indholdet var: '$1' (og den eneste forfatter var '$2')",
'exbeforeblank'               => "indholdet før siden blev tømt var: '$1'",
'exblank'                     => 'siden var tom',
'delete-confirm'              => 'Slet "$1"',
'delete-legend'               => 'Slet',
'historywarning'              => 'Advarsel: Siden du er ved at slette har en historie:',
'confirmdeletetext'           => 'Du er ved permanent at slette en side
eller et billede sammen med hele den tilhørende historie fra databasen. Bekræft venligst at du virkelig vil gøre dette, at du forstår konsekvenserne, og at du gør dette i overensstemmelse med
[[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Gennemført',
'deletedtext'                 => '"$1" er slettet. Se $2 for en fortegnelse over de nyeste sletninger.',
'deletedarticle'              => 'slettede "$1"',
'suppressedarticle'           => 'skjulte "[[$1]]"',
'dellogpage'                  => 'Sletningslog',
'dellogpagetext'              => 'Herunder vises de nyeste sletninger. Alle tider er serverens tid.',
'deletionlog'                 => 'sletningslog',
'reverted'                    => 'Gendannet en tidligere version',
'deletecomment'               => 'Begrundelse for sletning',
'deleteotherreason'           => 'Anden/uddybende begrundelse:',
'deletereasonotherlist'       => 'Anden begrundelse',
'deletereason-dropdown'       => '
*Hyppige sletningsårsager
** Efter forfatters ønske
** Overtrædelse af ophavsret
** Hærværk',
'delete-edit-reasonlist'      => 'Ret sletningsbegrundelser',
'delete-toobig'               => 'Denne side har en stor historik, over {{PLURAL:$1|en version|$1 versioner}}. Sletning af sådanne sider er begrænset blevet for at forhindre utilsigtet forstyrrelse af {{SITENAME}}.',
'delete-warning-toobig'       => 'Denne side har en stor historik, over {{PLURAL:$1|en version|$1 versioner}} versioner, slettes den kan det forstyrre driften af {{SITENAME}}, gå forsigtigt frem.',
'rollback'                    => 'Fjern redigeringer',
'rollback_short'              => 'Fjern redigering',
'rollbacklink'                => 'fjern redigering',
'rollbackfailed'              => 'Kunne ikke fjerne redigeringen',
'cantrollback'                => 'Kan ikke fjerne redigering; den sidste bruger er den eneste forfatter.',
'alreadyrolled'               => 'Kan ikke fjerne den seneste redigering af [[:$1]] foretaget af [[User:$2|$2]] ([[User talk:$2|diskussion]]); en anden har allerede redigeret siden eller fjernet redigeringen. Den seneste redigering er foretaget af [[User:$3|$3]] ([[User talk:$3|diskussion]]).',
'editcomment'                 => 'Kommentaren til redigeringen var: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Gendannelse til seneste version ved $1, fjerner ændringer fra $2', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "$1's ændringer er fjernet, gendannet til den seneste version af $2.",
'sessionfailure'              => 'Der lader til at være et problem med din loginsession; denne handling blev annulleret som en sikkerhedsforanstaltning mod kapring af sessionen. Tryk på "tilbage"-knappen og genindlæs den side du kom fra, og prøv dernæst igen.',
'protectlogpage'              => 'Liste_over_beskyttede_sider',
'protectlogtext'              => 'Herunder er en liste med sider der er blevet beskyttet/har fået fjernet beskyttelsen.',
'protectedarticle'            => '[[$1]] beskyttet',
'modifiedarticleprotection'   => 'ændrede beskyttelsen af "[[$1]]"',
'unprotectedarticle'          => 'fjernet beskyttelse af [[$1]]',
'protect-title'               => 'Ændre beskyttelse af "$1"',
'protect-legend'              => 'Bekræft beskyttelse',
'protectcomment'              => 'Begrundelse for beskyttelse',
'protectexpiry'               => 'Udløb',
'protect_expiry_invalid'      => 'Udløbstiden er ugyldig.',
'protect_expiry_old'          => 'Udløbstiden ligger i fortiden.',
'protect-unchain'             => 'Ændre flytningsbeskyttelse',
'protect-text'                => "Her kan beskyttelsesstatus for siden '''$1''' ses og ændres.",
'protect-locked-blocked'      => 'Du kan ikke ændre sidens status, da din brugerkonto er spærret. Her er de aktuelle beskyttelsesindstillinger for siden <strong>„$1“:</strong>',
'protect-locked-dblock'       => 'Databasen er spærret, sidebeskyttelsen kan derfor ikke ændres. her er de aktuelle beskyttelsesindstillinger for siden <strong>„$1“:</strong>',
'protect-locked-access'       => 'Den brugerkonto har ikke de nødvendige rettigheder til at ændre sidebeskyttelsen. Her er de aktuelle beskyttelsesindstillinger for siden <strong>„$1“:</strong>',
'protect-cascadeon'           => 'Denne side er del af en nedarvet skrivebeskyttelse. Wen er indeholdt i nedenstående {{PLURAL:$1|side|sider}}, som er skrivebeskyttet med tilvalg af "nedarvende sidebeskyttelse" Sidebeskyttelsen kan ændres for denne side, det påvirker dog ikke kaskadespærringen:',
'protect-default'             => 'Alle (Standard)',
'protect-fallback'            => 'Kræv "$1"-tilladelse',
'protect-level-autoconfirmed' => 'Spærring for ikke registrerede brugere',
'protect-level-sysop'         => 'Kun administratorer',
'protect-summary-cascade'     => 'nedarvende',
'protect-expiring'            => 'til $1 (UTC)',
'protect-cascade'             => 'Nedarvende spærring – alle skabeloner, som er indbundet i denne side spærres også.',
'protect-cantedit'            => 'Du kan ikke ændre beskyttelsesniveau for denne side, da du ikke kan redigere fden.',
'restriction-type'            => 'Beskyttelsesstatus',
'restriction-level'           => 'Beskyttelseshøjde',
'minimum-size'                => 'Mindste størrelse',
'maximum-size'                => 'Største størrelse',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'ændre',
'restriction-move'   => 'flytte',
'restriction-create' => 'oprette',
'restriction-upload' => 'oplægge',

# Restriction levels
'restriction-level-sysop'         => 'beskyttet (kun administratorer)',
'restriction-level-autoconfirmed' => 'beskyttet (kun anmeldte og ikke nye brugere)',
'restriction-level-all'           => 'alle',

# Undelete
'undelete'                     => 'Gendan en slettet side',
'undeletepage'                 => 'Se og gendan slettede sider',
'undeletepagetitle'            => "'''Slettede versioner af [[:$1|$1]]'''.",
'viewdeletedpage'              => 'Vise slettede sider',
'undeletepagetext'             => 'De følgende sider er slettede, men de findes stadig i arkivet og kan gendannes. Arkivet blivet periodevis slettet.',
'undelete-fieldset-title'      => 'Gendan versioner',
'undeleteextrahelp'            => '* For at gendanne siden komplet med alle versioner, angives en begrundelse før der trykkes på „Gendan“.
* Hvis der kun skal gendannes bestemte versioner, vælges disse enkeltvis, angiv en begrundelse og klik på „Gendan“.
* „Afbryde“ tømmer kommentarfeltet og fjerner alle markeringer ved versionerne.',
'undeleterevisions'            => '$1 {{PLURAL:$1|revision|revisioner}} arkiveret',
'undeletehistory'              => 'Hvis du gendanner siden, vil alle de historiske
revisioner også blive gendannet. Hvis en ny side med det samme navn
er oprettet siden denne blev slettet, så vil de gendannede revisioner
dukke op i den tidligere historie, og den nyeste revision vil forblive
på siden.',
'undeleterevdel'               => 'Gendannelsen gennemføres ikke, når den mest aktuelle version er skjult eller indeholder skjulte dele.
I dette tilfælde må den nyeste version ikke markeres eller dens status skal ændres til en normal version.
Versioner af filer, som du ikke har adgang til, gendannes ikke.',
'undeletehistorynoadmin'       => 'Denne side blev ikke slettet. Årsagen til sletningen er angivet i resumeet,
sammen med oplysninger om den sidste bruger, der ændrede denne side før sletningen.
Den aktuelle tekst for den slettede side er kun tilgængelig for administratorer.',
'undelete-revision'            => 'Slettet version fra $1 af $2 slettet af $3:',
'undeleterevision-missing'     => 'Ugyldig eller manglende version. Enten er henvisningen forkert eller versionen blev fjernet eller gendannet fra arkivet.',
'undelete-nodiff'              => 'Der findes ingen tidligere version.',
'undeletebtn'                  => 'Gendan!',
'undeletelink'                 => 'gendan',
'undeletereset'                => 'Afbryde',
'undeletecomment'              => 'Begrundelse:',
'undeletedarticle'             => 'gendannede "$1"',
'undeletedrevisions'           => '$1 {{PLURAL:$1|version|versioner}} gendannet',
'undeletedrevisions-files'     => '$1 {{plural:$1|version|versioner}} og $2 {{plural:$2|fil|filer}} gendannet',
'undeletedfiles'               => '$1 {{plural:$1|fil|filer}} gendannet',
'cannotundelete'               => 'Gendannelse mislykkedes; en anden har allerede gendannet siden.',
'undeletedpage'                => "'''$1''' blev gendannet.

I [[Special:Log/delete|slette-loggen]] findes en oversigt over de nyligt slettede og gendannede sider.",
'undelete-header'              => 'Se [[Special:Log/delete|slette-loggen]] for nyligt slettede og gendannede sider.',
'undelete-search-box'          => 'Søge efter slettede sider',
'undelete-search-prefix'       => 'Søgebegreb (odets start uden wildcards):',
'undelete-search-submit'       => 'Søg',
'undelete-no-results'          => 'Der blev ikke fundet en passende side i arkivet.',
'undelete-filename-mismatch'   => 'Kan ikke gendanne filen med tidsstempel $1: forkert filnavn',
'undelete-bad-store-key'       => 'Kan ikke gendanne filen med tidsstempel $1: file fandtes ikke da den blev slettet',
'undelete-cleanup-error'       => 'Fejl under sletning af ubrugt arkiveret version "$1".',
'undelete-missing-filearchive' => 'Kunne ikke genskabe arkiveret fil med ID $1 fordi den ikke findes i databasen. Måske er den allerede gendannet.',
'undelete-error-short'         => 'Fejl under gendannelsen af fil: $1',
'undelete-error-long'          => 'Der opstod en fejl under gendannelsen af filen:

$1',

# Namespace form on various pages
'namespace'      => 'Navnerum:',
'invert'         => 'Inverter udvalg',
'blanknamespace' => '(Artikler)',

# Contributions
'contributions' => 'Brugerbidrag',
'mycontris'     => 'Mine bidrag',
'contribsub2'   => 'For $1 ($2)',
'nocontribs'    => 'Ingen ændringer er fundet som opfylder disse kriterier.',
'uctop'         => ' (seneste)',
'month'         => 'Måned:',
'year'          => 'År:',

'sp-contributions-newbies'     => 'Vis kun bidrag fra nye brugere',
'sp-contributions-newbies-sub' => 'For nybegyndere',
'sp-contributions-blocklog'    => 'Blokeringslog',
'sp-contributions-search'      => 'Søge efter brugerbidrag',
'sp-contributions-username'    => 'IP-adresse eller brugernavn:',
'sp-contributions-submit'      => 'Søg',

# What links here
'whatlinkshere'            => 'Hvad henviser hertil',
'whatlinkshere-title'      => 'Sider der henviser til $1',
'whatlinkshere-page'       => 'Side:',
'linklistsub'              => '(Henvisningsliste)',
'linkshere'                => "De følgende sider henviser til '''„[[:$1]]“''':",
'nolinkshere'              => "Ingen sider henviser til '''„[[:$1]]“'''.",
'nolinkshere-ns'           => "Ingen side henviser til '''„[[:$1]]“''' i det valgte navnerum.",
'isredirect'               => 'omdirigeringsside',
'istemplate'               => 'Skabelonmedtagning',
'isimage'                  => 'fillink',
'whatlinkshere-prev'       => '{{PLURAL:$1|forrige|forrige $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|næste|næste $1}}',
'whatlinkshere-links'      => '← henvisninger',
'whatlinkshere-hideredirs' => '$1 omdirrigeringer',
'whatlinkshere-hidetrans'  => '$1 inkluderinger',
'whatlinkshere-hidelinks'  => '$1 henvisninger',
'whatlinkshere-hideimages' => '$1 fillinks',
'whatlinkshere-filters'    => 'Filtre',

# Block/unblock
'blockip'                         => 'Bloker bruger',
'blockip-legend'                  => 'Bloker bruger',
'blockiptext'                     => 'Brug formularen herunder til at blokere for skriveadgangen fra en specifik IP-adresse eller et brugernavn. Dette må kun gøres for at forhindre vandalisme og skal være i overensstemmelse med [[{{MediaWiki:Policy-url}}|{{SITENAME}}s politik]]. Angiv en specifik begrundelse herunder (for eksempel med angivelse af sider der har været udsat for vandalisme). Udløbet (expiry) angives i GNUs standardformat, som er beskrevet i [http://www.gnu.org/software/tar/manual/html_chapter/tar_7.html vejledningen til tar] (på engelsk), fx "1 hour", "2 days", "next Wednesday", "1 January 2017". Alternativt kan en blokering gøres uendelig (skriv "indefinite" eller "infinite"). For oplysninger om blokering af IP-adresseblokke, se [[meta:Range blocks|IP-adresseblokke]] (på engelsk). For at ophæve en blokering, se [[Special:IPBlockList|listen over blokerede IP-adresser og brugernavne]].',
'ipaddress'                       => 'IP-adresse/brugernavn',
'ipadressorusername'              => 'IP-adresse eller brugernavn',
'ipbexpiry'                       => 'varighed',
'ipbreason'                       => 'Begrundelse',
'ipbreasonotherlist'              => 'Anden begrundelse',
'ipbreason-dropdown'              => '
* Generelle begrundelser
** Sletning af sider
** Oprettelse af tåbelige sider
** Vedvarende overtrædelse af reglerne for eksterne henvisninger
** Overtrædelse af reglen „Ingen personangreb“
* Brugerspecifikke grunde
** Uegnet brugernavn
** Ny tilmelding fra en ubegrænset spærret bruger
* IP-specifikke grunde
** Proxy, pga. vandalisme fra enkelte, brugere spærret i længere tid',
'ipbanononly'                     => 'Kun anonyme brugere spærres',
'ipbcreateaccount'                => 'Forhindre oprettelse af brugerkonti',
'ipbemailban'                     => 'Spærre brugerens adgang til at sende mail',
'ipbenableautoblock'              => 'Spærre den IP-adresse, der bruges af denne bruger samt automatisk alle følgende, hvorfra han foretager ændringer eller forsøger at anlægge brugerkonti',
'ipbsubmit'                       => 'Bloker denne bruger',
'ipbother'                        => 'Anden varighed (engelsk)',
'ipboptions'                      => '1 time:1 hour,2 timer:2 hours,6 timer:6 hours,1 dag:1 day,3 dage:3 days,1 uge:1 week,2 uger:2 weeks,1 måned:1 month,3 måneder:3 months,1 år:1 year,ubegrænset:indefinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => 'Anden varighed',
'ipbotherreason'                  => 'Anden/uddybende begrundelse',
'ipbhidename'                     => 'Skjul brugernavn/IP-adresse i spærrelog, listen med aktive spærringer og brugerfortegnelsen.',
'ipbwatchuser'                    => 'Overvåg denne brugers brugerside og diskussionsside.',
'badipaddress'                    => 'IP-adressen/brugernavnet er udformet forkert eller eksistere ikke.',
'blockipsuccesssub'               => 'Blokeringen er gennemført.',
'blockipsuccesstext'              => '"$1" er blevet blokeret.
<br />Se [[Special:IPBlockList|IP blokeringslisten]] for alle blokeringer.',
'ipb-edit-dropdown'               => 'Ændre spærreårsager',
'ipb-unblock-addr'                => 'frigive „$1“',
'ipb-unblock'                     => 'Frigive IP-adresse/bruger',
'ipb-blocklist-addr'              => 'Vise aktuelle spærringer for „$1“',
'ipb-blocklist'                   => 'Vise alle aktuelle spærringer',
'unblockip'                       => 'Ophæv blokering af bruger',
'unblockiptext'                   => 'Brug formularen herunder for at gendanne skriveadgangen for en tidligere blokeret IP-adresse eller bruger.',
'ipusubmit'                       => 'Ophæv blokeringen af denne adresse',
'unblocked'                       => '[[User:$1|$1]] blev frigivet',
'unblocked-id'                    => 'Blokering $1 er blevet fjernet',
'ipblocklist'                     => 'Blokerede IP-adresser og brugernavne',
'ipblocklist-legend'              => 'Find en blokeret bruger',
'ipblocklist-username'            => 'Brugernavn eller IP-addresse:',
'ipblocklist-submit'              => 'Søg',
'blocklistline'                   => '$1, $2 blokerede $3 ($4)',
'infiniteblock'                   => 'udløber infinite',
'expiringblock'                   => 'udløber $1',
'anononlyblock'                   => 'kun anonyme',
'noautoblockblock'                => 'Autoblok deaktiveret',
'createaccountblock'              => 'Oprettelse af brugerkonti spærret',
'emailblock'                      => 'e-mail blokeret',
'ipblocklist-empty'               => 'Blokeringslisten er tom.',
'ipblocklist-no-results'          => 'Den angivene IP-addresse eller brugernavn er ikke blokeret.',
'blocklink'                       => 'bloker',
'unblocklink'                     => 'ophæv blokering',
'contribslink'                    => 'bidrag',
'autoblocker'                     => 'Automatisk blokeret fordi du deler IP-adresse med "$1". Begrundelse "$2".',
'blocklogpage'                    => 'Blokeringslog',
'blocklogentry'                   => 'blokerede "[[$1]]" med en udløbstid på $2 $3',
'blocklogtext'                    => 'Dette er en liste med blokerede brugere og ophævede blokeringer af brugere. Automatisk blokerede IP-adresser er ikke anført her. Se [[Special:IPBlockList|blokeringslisten]] for den nuværende liste med blokerede brugere.',
'unblocklogentry'                 => 'ophævede blokering af "$1"',
'block-log-flags-anononly'        => 'kun anonyme',
'block-log-flags-nocreate'        => 'Oprettelse af brugerkonti blokeret',
'block-log-flags-noautoblock'     => 'Autoblok deaktiveret',
'block-log-flags-noemail'         => 'e-mail blokeret',
'block-log-flags-angry-autoblock' => 'udvidet automatisk blokering slået tilenhanced autoblock enabled',
'range_block_disabled'            => 'Sysop-muligheden for at oprette blokeringsklasser er slået fra.',
'ipb_expiry_invalid'              => 'Udløbstiden er ugyldig.',
'ipb_expiry_temp'                 => 'Brugernavnet kan kun skjules ved permanente blokeringer.',
'ipb_already_blocked'             => '„$1“ er allerede blokeret',
'ipb_cant_unblock'                => 'Fejl: Spærre-ID $1 ikke fundet. Spærringen er allerede ophævet.',
'ipb_blocked_as_range'            => 'Fejl: IP-adressen $1 er ikke dirkete blokeret. Derfor kan en blokering ikke ophæves. Adressen er blokeret som en del af intervallet $2. Denne blokering kan ophæves.',
'ip_range_invalid'                => 'Ugyldigt IP-interval.',
'blockme'                         => 'Bloker mig',
'proxyblocker'                    => 'Proxy-blokering',
'proxyblocker-disabled'           => 'Denne funktion er ikke i brug.',
'proxyblockreason'                => "Din IP-adresse er blevet blokeret fordi den er en såkaldt ''åben proxy''. Kontakt din Internet-udbyder eller tekniske hotline og oplyse dem om dette alvorlige sikkerhedsproblem.",
'proxyblocksuccess'               => 'Færdig.',
'sorbsreason'                     => 'IP-adressen er opført i DNSBL på {{SITENAME}} som åben PROXY.',
'sorbs_create_account_reason'     => 'IP-adressen er opført i DNSBL på {{SITENAME}} som åben PROXY. Oprettelse af nye brugere er ikke mulig.',

# Developer tools
'lockdb'              => 'Lås database',
'unlockdb'            => 'Lås database op',
'lockdbtext'          => 'At låse databasen vil forhindre alle brugere i at kunne redigere sider, ændre indstillinger, redigere overvågningslister og andre ting der kræver ændringer i databasen. Bekræft venligst at du har til hensigt at gøre dette, og at du vil låse databasen op, når din vedligeholdelse er overstået.',
'unlockdbtext'        => 'At låse databasen op vil gøre, at alle brugere igen kan redigere sider, ændre deres indstillinger, redigere deres overvågningsliste, og andre ting der kræver ændringer i databasen. Bekræft venligst at du har til hensigt at gøre dette.',
'lockconfirm'         => 'Ja, jeg vil virkelig låse databasen.',
'unlockconfirm'       => 'Ja, jeg vil virkelig låse databasen op.',
'lockbtn'             => 'Lås databasen',
'unlockbtn'           => 'Lås databasen op',
'locknoconfirm'       => 'Du har ikke bekræftet handlingen.',
'lockdbsuccesssub'    => 'Databasen er nu låst',
'unlockdbsuccesssub'  => 'Databasen er nu låst op',
'lockdbsuccesstext'   => 'Mediawikidatabasen er låst. <br />Husk at fjerne låsen når du er færdig med din vedligeholdelse.',
'unlockdbsuccesstext' => 'Mediawikidatabasen er låst op.',
'lockfilenotwritable' => 'Database-spærrefilen kan ikke ændres. Hvis databasen skal spærres eller frigives, skal webserveren kunne skrive i denne fil.',
'databasenotlocked'   => 'Databasen er ikke spærret.',

# Move page
'move-page'               => 'Flyt $1',
'move-page-legend'        => 'Flyt side',
'movepagetext'            => "Når du bruger formularen herunder vil du få omdøbt en side og flyttet hele sidens historie til det nye navn. Den gamle titel vil blive en omdirigeringsside til den nye titel. Henvisninger til den gamle titel vil ikke blive ændret. Sørg for at tjekke for dobbelte eller dårlige omdirigeringer. Du er ansvarlig for, at alle henvisninger stadig peger derhen, hvor det er meningen de skal pege. Bemærk at siden '''ikke''' kan flyttes hvis der allerede er en side med den nye titel, medmindre den side er tom eller er en omdirigering uden nogen historie. Det betyder at du kan flytte en side tilbage hvor den kom fra, hvis du kommer til at lave en fejl. <b>ADVARSEL!</b> Dette kan være en drastisk og uventet ændring for en populær side; vær sikker på, at du forstår konsekvenserne af dette før du fortsætter.",
'movepagetalktext'        => "Den tilhørende diskussionsside, hvis der er en, vil automatisk blive flyttet med siden '''medmindre:''' *Du flytter siden til et andet navnerum,
*En ikke-tom diskussionsside allerede eksisterer under det nye navn, eller
*Du fjerner markeringen i boksen nedenunder.

I disse tilfælde er du nødt til at flytte eller sammenflette siden manuelt.",
'movearticle'             => 'Flyt side',
'movenotallowed'          => 'Du har ikke rettigheder til at flytte sider.',
'newtitle'                => 'Til ny titel',
'move-watch'              => 'Denne side overvåges',
'movepagebtn'             => 'Flyt side',
'pagemovedsub'            => 'Flytning gennemført',
'movepage-moved'          => '<big>Siden \'\'\'"$1" er flyttet til "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'En side med det navn eksisterer allerede, eller det
navn du har valgt er ikke gyldigt. Vælg et andet navn.',
'cantmove-titleprotected' => 'Du kan ikke omdøbe en side til dette navn. Det nye navn er beskyttet mod oprettelse.',
'talkexists'              => 'Siden blev flyttet korrekt, men den tilhørende diskussionsside kunne ikke flyttes, fordi der allerede eksisterer en med den nye titel. Du er nødt til at flette dem sammen manuelt.',
'movedto'                 => 'flyttet til',
'movetalk'                => 'Flyt også "diskussionssiden", hvis den eksisterer.',
'move-subpages'           => 'Flyt også undersider hvis de findes',
'move-talk-subpages'      => 'Flyt diskussionssidens undersider hvis de findes',
'movepage-page-exists'    => 'Siden $1 findes allerede og kan ikke overskrives automatisk.',
'movepage-page-moved'     => 'Siden $1 er flyttet til $2.',
'movepage-page-unmoved'   => 'Siden $1 kan ikke flyttes til $2.',
'movepage-max-pages'      => 'Grænsen på $1 {{PLURAL:$1|sideflytning|sideflytninger}} er nået. De resterende sider vil ikke blive flyttet automatisk.',
'1movedto2'               => '$1 flyttet til $2',
'1movedto2_redir'         => '$1 flyttet til $2 over en omdirigering',
'movelogpage'             => 'Flyttelog',
'movelogpagetext'         => 'Nedenfor vises flyttede sider.',
'movereason'              => 'Begrundelse',
'revertmove'              => 'gendan',
'delete_and_move'         => 'Slet og flyt',
'delete_and_move_text'    => '==Sletning nødvendig==

Artiklen "[[:$1]]" eksisterer allerede. Vil du slette den for at lave plads til flytningen?',
'delete_and_move_confirm' => 'Slette eksisterende side før flytning',
'delete_and_move_reason'  => 'Slet for at lave plads til flyningen',
'selfmove'                => 'Begge sider har samme navn. Man kan ikke flytte en side oven i sig selv.',
'immobile_namespace'      => 'Måltitlen er en speciel type; man kan ikke flytte sider ind i det navnerum.',
'imagenocrossnamespace'   => 'Filer kan ikke flyttes til et navnerum der ikke indeholder filer',
'imagetypemismatch'       => 'Den nye filendelse passer ikke til filtypen',
'imageinvalidfilename'    => 'Destinationsnavnet er ugyldigt',
'fix-double-redirects'    => 'Opdater henvisninger til det oprindelige navn',

# Export
'export'            => 'Eksportér sider',
'exporttext'        => 'Du kan eksportere teksten og historikken fra en eller flere sider i et simpelt XML format. Dette kan bruges til at indsætte siderne i en anden wiki der bruger MediaWiki softwaren, eller du kan beholde den for din egen fornøjelses skyld',
'exportcuronly'     => 'Eksportér kun den nuværende version, ikke hele historikken',
'exportnohistory'   => "---- '''Bemærk:''' Eksporten af en komplet versionshistorik er pga. ydelsesårsager pt. ikke mulig.",
'export-submit'     => 'Eksportere sider',
'export-addcattext' => 'Tilføje sider fra kategori:',
'export-addcat'     => 'Tilføje',
'export-download'   => 'Tilbyd at gemme som en fil',
'export-templates'  => 'Medtag skabeloner',

# Namespace 8 related
'allmessages'               => 'Alle beskeder',
'allmessagesname'           => 'Navn',
'allmessagesdefault'        => 'Standard tekst',
'allmessagescurrent'        => 'Nuværende tekst',
'allmessagestext'           => 'Dette er en liste med alle beskeder i MediaWiki: navnerummet.',
'allmessagesnotsupportedDB' => '{{ns:special}}:AllMessages ikke understøttet fordi wgUseDatabaseMessages er slået fra.',
'allmessagesfilter'         => 'Meddelelsesnavnefilter:',
'allmessagesmodified'       => 'Vis kun ændrede',

# Thumbnails
'thumbnail-more'           => 'Forstør',
'filemissing'              => 'Filen mangler',
'thumbnail_error'          => 'Fejl ved oprettelse af thumbnail: $1',
'djvu_page_error'          => 'DjVu-side udenfor sideområdet',
'djvu_no_xml'              => 'XML-data kan ikke hentes til DjVu-filen',
'thumbnail_invalid_params' => 'Ugyldige thumbnail-parametre',
'thumbnail_dest_directory' => 'Kataloget kan ikke oprettes.',

# Special:Import
'import'                     => 'Importer sider',
'importinterwiki'            => 'Importer sider fra en anden wiki',
'import-interwiki-text'      => 'Vælg en Wiki og en side til importen.
Datoen i den pågældende version og forfatterne ændres ikke.
Alle Transwiki import-aktioner protokolleres i [[Special:Log/import|import-loggen]].',
'import-interwiki-history'   => 'Importer alle versioner af denne side',
'import-interwiki-submit'    => 'Importer',
'import-interwiki-namespace' => 'Importer  siderne i navnerummet:',
'importtext'                 => "Eksportér filen fra kilde-wiki'en ved hjælp af værktøjet Special:Export, gem den på din harddisk og læg den op her.",
'importstart'                => 'Importere sider …',
'import-revision-count'      => '– {{PLURAL:$1|1 version|$1 versioner}}',
'importnopages'              => 'Ingen sider fundet til import.',
'importfailed'               => 'Importering fejlede: $1',
'importunknownsource'        => 'Ukendt fejlkilde',
'importcantopen'             => 'Importfil kunne ikke åbnes',
'importbadinterwiki'         => 'Forkert Interwiki-henvisning',
'importnotext'               => 'Tom eller ingen tekst',
'importsuccess'              => 'Importen lykkedes!',
'importhistoryconflict'      => 'Der er en konflikt i versionhistorikken (siden kan have været importeret før)',
'importnosources'            => 'Ingen transwiki importkilde defineret og direkte historikuploads er deaktiveret.',
'importnofile'               => 'Ingen importfil valgt!',
'importuploaderrorsize'      => 'Upload af importfil mislykkedes da filen er større en den tilladte maksimale uploadstørrelse.',
'importuploaderrorpartial'   => 'Upload af importfil mislykkedes da filen kun blev delvist uploadet.',
'importuploaderrortemp'      => 'Upload af importfil mislykkedes da en midlertidig mappe mangler.',
'import-parse-failure'       => 'XML fortolkningsfejl under importering',
'import-noarticle'           => 'Der er ingen sider at importere!',
'import-nonewrevisions'      => 'Alle versioner er allerede importeret.',
'xml-error-string'           => '$1 på linie $2, kolonne $3 (byte $4): $5',
'import-upload'              => 'Upload XML-data',

# Import log
'importlogpage'                    => 'Importlog',
'importlogpagetext'                => 'Administrativ import af sider med versionshistorik fra andre Wikis.',
'import-logentry-upload'           => '[[$1]] blev importeret',
'import-logentry-upload-detail'    => '{{PLURAL:$1|1 version|$1 versioner}}',
'import-logentry-interwiki'        => '[[$1]] blev importeret (Transwiki)',
'import-logentry-interwiki-detail' => '{{PLURAL:$1|1 version|$1 versioner}} af $2 importeret',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Min brugerside',
'tooltip-pt-anonuserpage'         => 'Brugersiden for den ip-adresse du redigerer som',
'tooltip-pt-mytalk'               => 'Min diskussionsside',
'tooltip-pt-anontalk'             => 'Diskussion om redigeringer fra denne ip-adresse',
'tooltip-pt-preferences'          => 'Mine indstillinger',
'tooltip-pt-watchlist'            => 'Listen over sider du overvåger for ændringer.',
'tooltip-pt-mycontris'            => 'Listen over dine bidrag',
'tooltip-pt-login'                => 'Du opfordres til at logge på, men det er ikke obligatorisk.',
'tooltip-pt-anonlogin'            => 'Du opfordres til at logge på, men det er ikke obligatorisk',
'tooltip-pt-logout'               => 'Log af',
'tooltip-ca-talk'                 => 'Diskussion om indholdet på siden',
'tooltip-ca-edit'                 => 'Du kan redigere denne side. Brug venligst forhåndsvisning før du gemmer.',
'tooltip-ca-addsection'           => 'Tilføj en kommentar til denne diskussion.',
'tooltip-ca-viewsource'           => 'Denne side er beskyttet. Du kan kigge på kildekoden.',
'tooltip-ca-history'              => 'Tidligere versioner af denne side.',
'tooltip-ca-protect'              => 'Beskyt denne side',
'tooltip-ca-delete'               => 'Slet denne side',
'tooltip-ca-undelete'             => 'Gendan de redigeringer der blev lavet på denne side før den blev slettet',
'tooltip-ca-move'                 => 'Flyt denne side',
'tooltip-ca-watch'                => 'Sæt denne side på din overvågningsliste',
'tooltip-ca-unwatch'              => 'Fjern denne side fra din overvågningsliste',
'tooltip-search'                  => 'Søg på denne wiki',
'tooltip-search-go'               => 'Vid en side med præcis dette navn, hvis den findes',
'tooltip-search-fulltext'         => 'Søg efter sider der indeholder denne tekst',
'tooltip-p-logo'                  => 'Forsiden',
'tooltip-n-mainpage'              => 'Besøg forsiden',
'tooltip-n-portal'                => 'Om projektet, hvad du kan gøre, hvor tingene findes',
'tooltip-n-currentevents'         => 'Find baggrundsinformation om aktuelle begivenheder',
'tooltip-n-recentchanges'         => 'Listen over de seneste ændringer i wikien.',
'tooltip-n-randompage'            => 'Gå til en tilfældig artikel',
'tooltip-n-help'                  => 'Hvordan gør jeg ...',
'tooltip-t-whatlinkshere'         => 'Liste med alle sider som henviser hertil',
'tooltip-t-recentchangeslinked'   => 'Seneste ændringer i sider som denne side henviser til',
'tooltip-feed-rss'                => 'RSS-feed for denne side',
'tooltip-feed-atom'               => 'Atom-feed for denne side',
'tooltip-t-contributions'         => 'Se denne brugers bidrag',
'tooltip-t-emailuser'             => 'Send en e-mail til denne bruger',
'tooltip-t-upload'                => 'Upload et billede eller anden mediafil',
'tooltip-t-specialpages'          => 'Liste med alle specielle sider',
'tooltip-t-print'                 => 'Printervenlig udgave af denne side',
'tooltip-t-permalink'             => 'Permanent henvisning til denne version af denne side',
'tooltip-ca-nstab-main'           => 'Se indholdet',
'tooltip-ca-nstab-user'           => 'Se brugersiden',
'tooltip-ca-nstab-media'          => 'Se mediasiden',
'tooltip-ca-nstab-special'        => 'Dette er en speciel side; man kan ikke redigere sådanne sider.',
'tooltip-ca-nstab-project'        => 'Vise portalsiden',
'tooltip-ca-nstab-image'          => 'Se billedsiden',
'tooltip-ca-nstab-mediawiki'      => 'Se systembeskeden',
'tooltip-ca-nstab-template'       => 'Se skabelonen',
'tooltip-ca-nstab-help'           => 'Se hjælpesiden',
'tooltip-ca-nstab-category'       => 'Se kategorisiden',
'tooltip-minoredit'               => 'Marker dette som en mindre ændring',
'tooltip-save'                    => 'Gem dine ændringer',
'tooltip-preview'                 => 'Forhåndsvis dine ændringer, brug venligst denne funktion inden du gemmer!',
'tooltip-diff'                    => 'Vis hvilke ændringer du har lavet i teksten.',
'tooltip-compareselectedversions' => 'Se forskellene imellem de to valgte versioner af denne side.',
'tooltip-watch'                   => 'Tilføj denne side til din overvågningsliste',
'tooltip-recreate'                => 'Opret side, selv om den blev slettet.',
'tooltip-upload'                  => 'Upload fil',

# Stylesheets
'common.css'   => '/** CSS inkluderet her vil være aktivt for alle brugere. */',
'monobook.css' => '/** CSS inkluderet her vil være aktivt for brugere af Monobook-temaet . */',

# Scripts
'common.js'   => '/* Javascript inkluderet her vil være aktivt for alle brugere. */',
'monobook.js' => '/* Udgået; brug [[MediaWiki:common.js]] */',

# Metadata
'nodublincore'      => 'Dublin Core RDF-metadata er slået fra på denne server.',
'nocreativecommons' => 'Creative Commons RDF-metadata er slået fra på denne server.',
'notacceptable'     => 'Wiki-serveren kan ikke levere data i et format, som din klient understøtter.',

# Attribution
'anonymous'        => 'Anonym(e) bruger(e) af {{SITENAME}}',
'siteuser'         => '{{SITENAME}} bruger $1',
'lastmodifiedatby' => 'Denne side blev senest ændret $2, $1 af $3.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Baseret på arbejde af $1.',
'others'           => 'andre',
'siteusers'        => '{{SITENAME}} bruger(e) $1',
'creditspage'      => 'Sidens forfattere',
'nocredits'        => 'Der er ingen forfatteroplysninger om denne side.',

# Spam protection
'spamprotectiontitle' => 'Spambeskyttelsesfilter',
'spamprotectiontext'  => 'Siden du prøver at få adgang til er blokeret af spamfilteret. Dette skyldes sandsynligvis en henvisning til et eksternt websted. Se [[m:spam blacklist]] for en komplet liste af blokerede websteder. Hvis du mener at spamfilteret blokerede redigeringen ved en fejl, så kontakt en [[m:Special:Listadmins|m:administrator]]. Det følgende er et udtræk af siden der bevirkede blokeringen:',
'spamprotectionmatch' => 'Følgende tekst udløste vores spamfilter: $1',
'spambot_username'    => 'MediaWiki spam-rensning',
'spam_reverting'      => 'Sidste version uden henvisning til $1 gendannet.',
'spam_blanking'       => 'Alle versioner, som indeholdt henvisninger til $1, er renset.',

# Info page
'infosubtitle'   => 'Information om siden',
'numedits'       => 'Antal redigeringer (artikel): $1',
'numtalkedits'   => 'Antal redigeringer (diskussionsside): $1',
'numwatchers'    => 'Antal overvågere: $1',
'numauthors'     => 'Antal forskellige forfattere (artikel): $1',
'numtalkauthors' => 'Antal forskellige forfattere (diskussionsside): $1',

# Math options
'mw_math_png'    => 'Vis altid som PNG',
'mw_math_simple' => 'HTML hvis meget simpel ellers PNG',
'mw_math_html'   => 'HTML hvis muligt ellers PNG',
'mw_math_source' => 'Lad være som TeX (for tekstbrowsere)',
'mw_math_modern' => 'Anbefalet til moderne browsere',
'mw_math_mathml' => 'MathML hvis muligt',

# Patrolling
'markaspatrolleddiff'                 => 'Markér som patruljeret',
'markaspatrolledtext'                 => 'Markér denne artikel som patruljeret',
'markedaspatrolled'                   => 'Markeret som patruljeret',
'markedaspatrolledtext'               => 'Den valgte revision er nu markeret som patruljeret.',
'rcpatroldisabled'                    => 'Seneste ændringer-patruljeringen er slået fra',
'rcpatroldisabledtext'                => 'Funktionen til seneste ændringer-patruljeringen er pt. slået fra.',
'markedaspatrollederror'              => 'Markering som „kontrolleret“ ikke mulig.',
'markedaspatrollederrortext'          => 'Du skal vælge en sideændring.',
'markedaspatrollederror-noautopatrol' => 'Du må ikke markere dine egne ændringer som kontrolleret.',

# Patrol log
'patrol-log-page'   => 'Kontrollog',
'patrol-log-header' => 'Patruljerede versioner.',
'patrol-log-line'   => 'har markeret $1 af $2 som kontrolleret $3.',
'patrol-log-auto'   => '(automatisk)',
'patrol-log-diff'   => 'Version $1',

# Image deletion
'deletedrevision'                 => 'Slettede gammel version $1',
'filedeleteerror-short'           => 'Fejl under sletning af fil: $1',
'filedeleteerror-long'            => 'Der opstod en fejl under sletningen af filen:

$1',
'filedelete-missing'              => 'Filen "$1" kan ikke slettes fordi den ikke findes.',
'filedelete-old-unregistered'     => 'Den angivne version "$1" findes ikke i databasen.',
'filedelete-current-unregistered' => 'Den angiovne fil "$1" findes ikke i databasen.',
'filedelete-archive-read-only'    => 'Webserveren har ikke skriveadgang til arkiv-kataloget "$1".',

# Browsing diffs
'previousdiff' => '← Gå til foregående forskel',
'nextdiff'     => 'Gå til næste forskel →',

# Media information
'mediawarning'         => "'''Advarsel''', denne filtype kan muligvis indeholde skadelig kode, du kan beskadige dit system hvis du udfører den.<hr />",
'imagemaxsize'         => 'Begræns størrelsen af billeder på billedsiderne til:',
'thumbsize'            => 'Thumbnail størrelse :',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|side|sider}}',
'file-info'            => '(Filstørrelse: $1, MIME-Type: $2)',
'file-info-size'       => '($1 × $2 punkter, filstørrelse: $3, MIME-Type: $4)',
'file-nohires'         => '<small>Ingen højere opløsning fundet.</small>',
'svg-long-desc'        => '(SVG fil, basisstørrelse $1 × $2 punkters, størrelse: $3)',
'show-big-image'       => 'Version i større opløsning',
'show-big-image-thumb' => '<small>Størrelse af forhåndsvisning: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'             => 'Galleri med de nyeste billeder',
'imagelisttext'         => 'Herunder er en liste med $1 {{PLURAL:$1|billede|billeder}} sorteret $2.',
'newimages-summary'     => 'Denne specialside viser de nyeste uploadede billeder og filer.',
'showhidebots'          => '(Bots $1)',
'noimages'              => 'Ingen filer fundet.',
'ilsubmit'              => 'Søg',
'bydate'                => 'efter dato',
'sp-newimages-showfrom' => 'Vis nye filer startende med $2, $1',

# Video information, used by Language::formatTimePeriod() to format lengths in the above messages
'hours-abbrev' => 't',

# Bad image list
'bad_image_list' => 'Formatet er:

Kun indholdet af lister (linjer startende med *) bliver brugt. Den første henvisning på en linje er til det uønskede billede. Efterfølgende links på samme linjer er undtagelser, dvs. sider hvor billedet må optræde.',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => 'Denne fil indeholder yderligere informationer, der som regel stammer fra digitalkameraet eller den brugte scanner. Ved en efterfølgende bearbejdning kan nogle data være ændret.',
'metadata-expand'   => 'Vis udvidede data',
'metadata-collapse' => 'Skjul udvidede data',
'metadata-fields'   => 'Følgenden felter fra EXIF-metadata i denne MediaWiki-systemtekst vises på billedbeskrivelsessider; yderligere detaljer kan vises.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Bredde',
'exif-imagelength'                 => 'Længde',
'exif-bitspersample'               => 'Bits pr. farvekomponent',
'exif-compression'                 => 'Kompressionstype',
'exif-photometricinterpretation'   => 'Pixelsammensætning',
'exif-orientation'                 => 'Kameraretning',
'exif-samplesperpixel'             => 'Antal komponenter',
'exif-planarconfiguration'         => 'Dataretning',
'exif-ycbcrsubsampling'            => 'Subsamplingrate fra Y til C',
'exif-ycbcrpositioning'            => 'Y og C positionering',
'exif-xresolution'                 => 'Horisontal opløsning',
'exif-yresolution'                 => 'Vertikal opløsning',
'exif-resolutionunit'              => 'Måleenhed for opløsning',
'exif-stripoffsets'                => 'Bileddata-forskydning',
'exif-rowsperstrip'                => 'Antal linier pr. stribe',
'exif-stripbytecounts'             => 'Bytes pr. komprimeret stribe',
'exif-jpeginterchangeformat'       => 'Offset til JPEG SOI',
'exif-jpeginterchangeformatlength' => 'Størrelse af JPEG-fil i bytes',
'exif-transferfunction'            => 'Overførselsfunktion',
'exif-whitepoint'                  => 'Manuel med måling',
'exif-primarychromaticities'       => 'Kromaticitet af primærfarver',
'exif-ycbcrcoefficients'           => 'YCbCr-koefficienter',
'exif-referenceblackwhite'         => 'Sort/hvide-referencepunkter',
'exif-datetime'                    => 'Lagringstidspunkt',
'exif-imagedescription'            => 'Billedtitel',
'exif-make'                        => 'Producent',
'exif-model'                       => 'Model',
'exif-software'                    => 'Software',
'exif-artist'                      => 'Fotograf',
'exif-copyright'                   => 'Ophavsret',
'exif-exifversion'                 => 'Exif-version',
'exif-flashpixversion'             => 'Understøttet Flashpix-version',
'exif-colorspace'                  => 'Farverum',
'exif-componentsconfiguration'     => 'Betydning af enkelte komponenter',
'exif-compressedbitsperpixel'      => 'Komprimerede bits pr. pixel',
'exif-pixelydimension'             => 'Gyldig billedbredde',
'exif-pixelxdimension'             => 'Gyldig billedhøjde',
'exif-makernote'                   => 'Producentnotits',
'exif-usercomment'                 => 'Brugerkommentarer',
'exif-relatedsoundfile'            => 'Tilhørende lydfil',
'exif-datetimeoriginal'            => 'Optagelsestidspunkt',
'exif-datetimedigitized'           => 'Digitaliseringstidspunkt',
'exif-subsectime'                  => 'Lagringstidspunkt (1/100 s)',
'exif-subsectimeoriginal'          => 'Optagelsestidspunkt (1/100 s)',
'exif-subsectimedigitized'         => 'Digitaliseringstidspunkt (1/100 s)',
'exif-exposuretime'                => 'Belysningsvarighed',
'exif-exposuretime-format'         => '$1 sekunder ($2)',
'exif-fnumber'                     => 'Blænde',
'exif-exposureprogram'             => 'Belysningsprogram',
'exif-spectralsensitivity'         => 'Spectral sensitivitet',
'exif-isospeedratings'             => 'Film- eller sensorfølsomhed (ISO)',
'exif-oecf'                        => 'Optoelektronisk omregningsfaktor',
'exif-shutterspeedvalue'           => 'Belysningstidsværdi',
'exif-aperturevalue'               => 'Blændeværdi',
'exif-brightnessvalue'             => 'Lyshedsværdi',
'exif-exposurebiasvalue'           => 'Belysningsindstilling',
'exif-maxaperturevalue'            => 'Største blænde',
'exif-subjectdistance'             => 'Afstand',
'exif-meteringmode'                => 'Målemetode',
'exif-lightsource'                 => 'Lyskilde',
'exif-flash'                       => 'Blitz',
'exif-focallength'                 => 'Brændvidde',
'exif-subjectarea'                 => 'Område',
'exif-flashenergy'                 => 'Blitzstyrke',
'exif-spatialfrequencyresponse'    => 'Rumligt frekvenssvar',
'exif-focalplanexresolution'       => 'Fokuseringspunkt X-opløsning',
'exif-focalplaneyresolution'       => 'Fokuseringspunkt Y-opløsning',
'exif-focalplaneresolutionunit'    => 'Enhed for fokuseringsopløsning',
'exif-subjectlocation'             => 'Motivsted',
'exif-exposureindex'               => 'Belysningsindeks',
'exif-sensingmethod'               => 'Målemetode',
'exif-filesource'                  => 'Filens kilde',
'exif-scenetype'                   => 'Scenetype',
'exif-cfapattern'                  => 'CFA-mønster',
'exif-customrendered'              => 'Brugerdefineret billedbehandling',
'exif-exposuremode'                => 'Belysningsmodus',
'exif-whitebalance'                => 'Hvisafstemning',
'exif-digitalzoomratio'            => 'Digitalzoom',
'exif-focallengthin35mmfilm'       => 'Brændvidde (småbilledækvivalent)',
'exif-scenecapturetype'            => 'Optagelsestype',
'exif-gaincontrol'                 => 'Forstærkning',
'exif-contrast'                    => 'Kontrast',
'exif-saturation'                  => 'Mætning',
'exif-sharpness'                   => 'Skarphed',
'exif-devicesettingdescription'    => 'Apparatindstilling',
'exif-subjectdistancerange'        => 'Motivafstand',
'exif-imageuniqueid'               => 'Billed-ID',
'exif-gpsversionid'                => 'GPS-dag-version',
'exif-gpslatituderef'              => 'Nordlig eller sydlig bredde',
'exif-gpslatitude'                 => 'Geografisk bredde',
'exif-gpslongituderef'             => 'Østlig eller vestlig længde',
'exif-gpslongitude'                => 'Geografisk længde',
'exif-gpsaltituderef'              => 'Udgangshøjde',
'exif-gpsaltitude'                 => 'Højde',
'exif-gpstimestamp'                => 'GPS-tid',
'exif-gpssatellites'               => 'Til målingen brugte satelitter',
'exif-gpsstatus'                   => 'Modtagerstatus',
'exif-gpsmeasuremode'              => 'Målemetode',
'exif-gpsdop'                      => 'Målepræcision',
'exif-gpsspeedref'                 => 'Hastighedsenhed',
'exif-gpsspeed'                    => 'GPS-modtagerens hastighed',
'exif-gpstrackref'                 => 'Reference for bevægelsesretningen',
'exif-gpstrack'                    => 'Bevægelsesretningen',
'exif-gpsimgdirectionref'          => 'Reference for retningen af billedet',
'exif-gpsimgdirection'             => 'Billedretning',
'exif-gpsmapdatum'                 => 'Geodætisk dato benyttet',
'exif-gpsdestlatituderef'          => 'Reference for bredden',
'exif-gpsdestlatitude'             => 'Bredde',
'exif-gpsdestlongituderef'         => 'Reference for længden',
'exif-gpsdestlongitude'            => 'Længde',
'exif-gpsdestbearingref'           => 'Reference for motivretningen',
'exif-gpsdestbearing'              => 'Motivretning',
'exif-gpsdestdistanceref'          => 'Reference for motivafstanden',
'exif-gpsdestdistance'             => 'Motivafstand',
'exif-gpsprocessingmethod'         => 'GPS-metodens navn',
'exif-gpsareainformation'          => 'GPS-områdets navn',
'exif-gpsdatestamp'                => 'GPS-fato',
'exif-gpsdifferential'             => 'GPS-differentialkorrektur',

# EXIF attributes
'exif-compression-1' => 'Ukomprimeret',

'exif-unknowndate' => 'Ukendt dato',

'exif-orientation-1' => 'Normal', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Horisontalt spejlet', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Drejet 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Vertikalt spejlet', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Drejet 90° mod uret og spejlet vertikalt', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Drejet 90° med uret', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Drejet 90° med uret og spejlet vertikalt', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Drejet 90° mod uret', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Grovformat',
'exif-planarconfiguration-2' => 'Planformat',

'exif-componentsconfiguration-0' => 'Findes ikke',

'exif-exposureprogram-0' => 'Ukendt',
'exif-exposureprogram-1' => 'Manuel',
'exif-exposureprogram-2' => 'Standardprogram',
'exif-exposureprogram-3' => 'Tidsautomatik',
'exif-exposureprogram-4' => 'Blændeautomatik',
'exif-exposureprogram-5' => 'Kreativprogram med tendens til stor skarphedsdybde',
'exif-exposureprogram-6' => 'Aktionprogram med tendens til kort belysningstid',
'exif-exposureprogram-7' => 'Portrætprogram',
'exif-exposureprogram-8' => 'Landskabsoptagelse',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0'   => 'Ukendt',
'exif-meteringmode-1'   => 'Gennemsnitlig',
'exif-meteringmode-2'   => 'Midtcentreret',
'exif-meteringmode-3'   => 'Spotmåling',
'exif-meteringmode-4'   => 'Flerspotmåling',
'exif-meteringmode-5'   => 'Mønster',
'exif-meteringmode-6'   => 'Billeddel',
'exif-meteringmode-255' => 'Ukendt',

'exif-lightsource-0'   => 'Ukendt',
'exif-lightsource-1'   => 'Dagslys',
'exif-lightsource-2'   => 'Fluorescerende',
'exif-lightsource-3'   => 'Glødelampe',
'exif-lightsource-4'   => 'Blitz',
'exif-lightsource-9'   => 'Godt vejr',
'exif-lightsource-10'  => 'Overskyet',
'exif-lightsource-11'  => 'Skyggefuldt',
'exif-lightsource-12'  => 'Dagslys fluorescerende (D 5700–7100 K)',
'exif-lightsource-13'  => 'Dagshvidt fluorescerende (N 4600–5400 K)',
'exif-lightsource-14'  => 'Koldthvidt fluorescerende (W 3900–4500 K)',
'exif-lightsource-15'  => 'Hvist fluorescerende (WW 3200–3700 K)',
'exif-lightsource-17'  => 'Standardlys A',
'exif-lightsource-18'  => 'Standardlys B',
'exif-lightsource-19'  => 'Standardlys C',
'exif-lightsource-24'  => 'ISO studio kunstlys',
'exif-lightsource-255' => 'Andre lyskilder',

'exif-focalplaneresolutionunit-2' => 'Tomme',

'exif-sensingmethod-1' => 'Udefineret',
'exif-sensingmethod-2' => 'En-chip-farvesensor',
'exif-sensingmethod-3' => 'To-chip-farvesensor',
'exif-sensingmethod-4' => 'Tre-chip-farvesensor',
'exif-sensingmethod-5' => 'Farvesekventiel områdesensor',
'exif-sensingmethod-7' => 'Triliniær sensor',
'exif-sensingmethod-8' => 'Farvesekventiel liniarsensor',

'exif-scenetype-1' => 'Normal',

'exif-customrendered-0' => 'Standard',
'exif-customrendered-1' => 'Brugerdefineret',

'exif-exposuremode-0' => 'Automatisk belysning',
'exif-exposuremode-1' => 'Manuel belysning',
'exif-exposuremode-2' => 'Belysningsrække',

'exif-whitebalance-0' => 'Automatisk',
'exif-whitebalance-1' => 'Manuel',

'exif-scenecapturetype-0' => 'Normal',
'exif-scenecapturetype-1' => 'Landskab',
'exif-scenecapturetype-2' => 'Portræt',
'exif-scenecapturetype-3' => 'Natscene',

'exif-gaincontrol-0' => 'Ingen',
'exif-gaincontrol-1' => 'Low gain up',
'exif-gaincontrol-2' => 'High gain up',
'exif-gaincontrol-3' => 'Low gain down',
'exif-gaincontrol-4' => 'High gain down',

'exif-contrast-0' => 'Normal',
'exif-contrast-1' => 'Svag',
'exif-contrast-2' => 'Stærk',

'exif-saturation-0' => 'Normal',
'exif-saturation-1' => 'Ringe',
'exif-saturation-2' => 'Høj',

'exif-sharpness-0' => 'Normal',
'exif-sharpness-1' => 'Ringe',
'exif-sharpness-2' => 'Stærk',

'exif-subjectdistancerange-0' => 'Ukendt',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Nær',
'exif-subjectdistancerange-3' => 'Fjern',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'nordl. bredde',
'exif-gpslatitude-s' => 'sydl. bredde',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'østl. længde',
'exif-gpslongitude-w' => 'vestl. længde',

'exif-gpsstatus-a' => 'Måling kører',
'exif-gpsstatus-v' => 'Målingens interoperabilitet',

'exif-gpsmeasuremode-2' => '2-dimensional måling',
'exif-gpsmeasuremode-3' => '3-dimensional måling',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'km/h',
'exif-gpsspeed-m' => 'mph',
'exif-gpsspeed-n' => 'Knob',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Faktisk retning',
'exif-gpsdirection-m' => 'Magnetisk retning',

# External editor support
'edit-externally'      => 'Rediger denne fil med en ekstern editor',
'edit-externally-help' => 'Se [http://www.mediawiki.org/wiki/Manual:External_editors setup instruktionerne] for mere information.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alle',
'imagelistall'     => 'alle',
'watchlistall2'    => 'alle',
'namespacesall'    => 'alle',
'monthsall'        => 'alle',

# E-mail address confirmation
'confirmemail'             => 'Bekræft e-mail-adressen',
'confirmemail_noemail'     => 'Du har ikke angivet en gyldig E-mail-adresse i din [[Special:Preferences|brugerprofil]].',
'confirmemail_text'        => '{{SITENAME}} kræver, at du bekræfter en E-mail-adresse (autentificering), før du kan bruge de udvidede E-mail-funktioner. Med et klik på kontrolfeltet forneden sendes en E-mail til dig. Denne E-mail indeholder et link med en bekræftelseskode. Med et klik på dette link bekræftes, at E-mail-adressen er gyldig.',
'confirmemail_pending'     => '<div class="error">En bekræftelsesmail er allerede sendt til dig. Hvis du først for nylig har oprettet brugerkontoen, vent da et par minutter på denne e-mail, før du bestiller en ny kode.</div>',
'confirmemail_send'        => 'Send bekræftelseskode',
'confirmemail_sent'        => 'Bekræftelses-e-amil afsendt.',
'confirmemail_oncreate'    => 'En bekræftelseskode er sendt til din E-mail-adresse. Denne kode skal ikke bruges til anmeldelsen, den kræves dog til aktiveringen af E-mail-funktionerne indenfor Wikien.',
'confirmemail_sendfailed'  => 'Bekræftelsesmailen kunne ikke afsendes. Kontroller at E-mail-adressen er korrekt.

Rückmeldung des Mailservers: $1',
'confirmemail_invalid'     => 'Ugyldig bekræftelseskode. Kodens gyldighed er muligvis udløbet.',
'confirmemail_needlogin'   => 'Du skal $1 for at bekræfte E-mail-adressen.',
'confirmemail_success'     => 'E-mail-adressen er nu bekræftet. Du kan nu logge på.',
'confirmemail_loggedin'    => 'E-mail-adressen er nu bekræftet.',
'confirmemail_error'       => 'Der skete en fejl ved bekræftelsen af E-mail-adressen.',
'confirmemail_subject'     => '[{{SITENAME}}] - bekræftelse af E-mail-adressen',
'confirmemail_body'        => 'Hej,

Nogen med IP-adresse $1, sandsynligvis dig, har bestilt en bekræftelse af denne E-mail-adresse til brugerkontoen "$2" på {{SITENAME}}.

For at aktivere E-mail-funktionen for {{SITENAME}} (igen) og for at bekræfte, at denne brugerkonto virkelig hører til din E-mail-adresse og dermed til dig, bedes du åbne det følgende link i din browser: $3

Bekræftelseskoden er gyldig indtil følgende tidspunkt: $4

Hvis denne E-mail-adresse *ikke* hører til den anførte brugerkonto, skal du i stedet åbne dette link i din browser: $5

-- 
{{SITENAME}}: {{fullurl:{{Mediawiki:mainpage}}}}',
'confirmemail_invalidated' => 'E-mail-bekræftelse afvist',
'invalidateemail'          => 'Cancel e-mail confirmation',

# Scary transclusion
'scarytranscludedisabled' => '[Interwiki-tilkobling er deaktiveret]',
'scarytranscludefailed'   => '[Skabelonindbinding for $1 mislykkedes]',
'scarytranscludetoolong'  => '[URL er for lang; beklager]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Trackbacks for denne side:<br />
$1
</div>',
'trackbackremove'   => '([$1 slet])',
'trackbacklink'     => 'Trackback',
'trackbackdeleteok' => 'Trackback blev slettet.',

# Delete conflict
'deletedwhileediting' => '<span class="error">Bemærk: Det blev forsøgt at slette denne side, efter at du var begyndt, at ændre den! 
Kig i [{{fullurl:Special:Log|type=delete&page=}}{{FULLPAGENAMEE}} slette-loggen], 
hvorfor siden blev slettet. Hvis du gemmer siden bliver den oprettet igen.</span>',
'confirmrecreate'     => "Bruger [[User:$1|$1]] ([[User talk:$1|Diskussion]]) har slettet denne side, efter at du begyndte at ændre den. Begrundelsen lyder:
: ''$2''
Bekræft venligst, at du virkelig vil oprette denne side igen.",
'recreate'            => 'Opret igen',

# HTML dump
'redirectingto' => 'Videresendt til [[:$1]]',

# action=purge
'confirm_purge'        => 'Slette denne side fra serverens cache? $1',
'confirm_purge_button' => 'OK',

# AJAX search
'searchcontaining' => "Søger efter sider, hvori ''$1'' forekommer.",
'searchnamed'      => "Søger efter sider, hvis navn indeholder ''$1''.",
'articletitles'    => "Sider, som begynder med ''$1''",
'hideresults'      => 'Skjul',
'useajaxsearch'    => 'Brug AJAX-søgning',

# Multipage image navigation
'imgmultipageprev' => '← forrige side',
'imgmultipagenext' => 'næste side →',
'imgmultigo'       => 'OK',
'imgmultigoto'     => 'Gå til side $1',

# Table pager
'ascending_abbrev'         => 'op',
'descending_abbrev'        => 'ned',
'table_pager_next'         => 'Næste side',
'table_pager_prev'         => 'Forrige side',
'table_pager_first'        => 'Første side',
'table_pager_last'         => 'Sidste side',
'table_pager_limit'        => 'Vis $1 indførsler pr. side',
'table_pager_limit_submit' => 'Start',
'table_pager_empty'        => 'Ingen resultater',

# Auto-summaries
'autosumm-blank'   => 'Siden blev ryddet.',
'autosumm-replace' => "Sidens indhold blev erstattet med: '$1'",
'autoredircomment' => 'Omdirigering til [[$1]] oprettet',
'autosumm-new'     => 'Siden blev oprettet: $1',

# Live preview
'livepreview-loading' => 'Indlæser …',
'livepreview-ready'   => 'Indlæser … færdig!',
'livepreview-failed'  => 'Live-forhåndsvisning ikke mulig! Brug venligst den almindelige forhåndsvisning.',
'livepreview-error'   => 'Forbindelse ikke mulig: $1 "$2". Brug venligst den almindelige forhåndsvisning.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Ændringer som er nyere end {{PLURAL:$1|et sekund|$1 sekunder}}, vises muligvis ikke i denne liste.',
'lag-warn-high'   => 'Grundet stor belastning af databaseserveren, vil ændringer der er nyere end {{PLURAL:$1|et sekund|$1 sekunder}} måske ikke blive vist i denne liste.',

# Watchlist editor
'watchlistedit-numitems'       => 'Din overvågningsliste indeholder {{PLURAL:$1|1 side|$1 sider}}, diskussionssider fraregnet.',
'watchlistedit-noitems'        => 'Din overvågningsliste er tom.',
'watchlistedit-normal-title'   => 'Rediger overvågningsliste',
'watchlistedit-normal-legend'  => 'Slet sider fra overvågningslisten',
'watchlistedit-normal-explain' => 'Din overvågningsliste er vist nedenfor. Du kan fjerne sider fra den ved at markere den og trykke på Fjern valgte. Du har også mulighed for at [[Special:Watchlist/raw|redigere listen direkte]], eller [[Special:Watchlist/clear|rydde listen]].',
'watchlistedit-normal-submit'  => 'Fjern valgte',
'watchlistedit-normal-done'    => '{{PLURAL:$1|1 side|$1 sider}} er fjernet fra din overvågningsliste:',
'watchlistedit-raw-title'      => 'Direkte redigering af overvågningsliste',
'watchlistedit-raw-legend'     => 'Direkte redigering af overvågningsliste',
'watchlistedit-raw-explain'    => 'Siderne i din overvågningsliste er vist nedenfor. Du kan ændre din overvågningsliste ved at tilføje og fjerne sidenavne. Du kan gemme din nye overvågningsliste ved at trykke på Opdater overvågningsliste nedenfor. Du kan også redigere overvågningslisten i [[Special:Watchlist/edit|sorteret form]].',
'watchlistedit-raw-titles'     => 'Sider:',
'watchlistedit-raw-submit'     => 'Opdater overvågningsliste',
'watchlistedit-raw-done'       => 'Din overvågningsliste blev opdateret.',
'watchlistedit-raw-added'      => '{{PLURAL:$1|1 side|$1 sider}} er tilføjet:',
'watchlistedit-raw-removed'    => '{{PLURAL:$1|1 side|$1 sider}} er fjernet:',

# Watchlist editing tools
'watchlisttools-view' => 'Se ændrede sider i overvågningslisten',
'watchlisttools-edit' => 'Rediger overvågningsliste',
'watchlisttools-raw'  => 'Rediger rå overvågningsliste',

# Core parser functions
'unknown_extension_tag' => 'Unknown extension tag "$1"',

# Special:Version
'version'                          => 'Version', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Installerede udvidelser',
'version-specialpages'             => 'Specielle sider',
'version-parserhooks'              => 'Oversætter-funktioner',
'version-variables'                => 'Variabler',
'version-other'                    => 'Andet',
'version-mediahandlers'            => 'Specialhåndtering af mediefiler',
'version-hooks'                    => 'Funktionstilføjelser',
'version-extension-functions'      => 'Udvidelsesfunktioner',
'version-parser-extensiontags'     => 'Tilføjede tags',
'version-parser-function-hooks'    => 'Oversætter-funktioner',
'version-skin-extension-functions' => 'Ekstra funktioner til udseende',
'version-hook-name'                => 'Navn',
'version-hook-subscribedby'        => 'Brugt af',
'version-version'                  => 'Version',
'version-license'                  => 'Licens',
'version-software'                 => 'Installeret software',
'version-software-product'         => 'Produkt',
'version-software-version'         => 'Version',

# Special:FilePath
'filepath'         => 'Filsti',
'filepath-page'    => 'Fil:',
'filepath-submit'  => 'Vis sti',
'filepath-summary' => 'Denne specialside giver et direkte link til en fil. Billder vises i fuld opløsning og andre mediatyper vil blive aktiveret med deres tilhærende program.

Angiv filnavnet uden "{{ns:image}}:"-præfix.',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Find dubletfiler',
'fileduplicatesearch-summary'  => 'Find dublerede filer baseret på deres hash-værdi.

Angiv filnavnet uden "{{ns:image}}:"-præfix.',
'fileduplicatesearch-legend'   => 'Find dubletfiler.',
'fileduplicatesearch-filename' => 'Filnavn:',
'fileduplicatesearch-submit'   => 'Find',
'fileduplicatesearch-info'     => '$1 × $2 punkter<br />Filstørrelse: $3<br />MIME-type: $4',
'fileduplicatesearch-result-1' => 'Filen "$1" har ingen identiske dubletter.',
'fileduplicatesearch-result-n' => 'Filen "$1" har {{PLURAL:$2|en dublet|$2 dubletter}}.',

# Special:SpecialPages
'specialpages'                   => 'Specialsider',
'specialpages-note'              => '----
* Normale specialsider.
* <span class="mw-specialpagerestricted">Specialsider med begrænset adgang.</span>',
'specialpages-group-maintenance' => 'Vedligeholdelsesside',
'specialpages-group-other'       => 'Andre specialsider',
'specialpages-group-login'       => 'Opret bruger / logon',
'specialpages-group-changes'     => 'Seneste ændringer og loglister',
'specialpages-group-media'       => 'Mediafiler og oplægning',
'specialpages-group-users'       => 'Brugere og rettigheder',
'specialpages-group-highuse'     => 'Højt profilerede sider',
'specialpages-group-pages'       => 'Sidelister',
'specialpages-group-pagetools'   => 'Sideværktøjer',
'specialpages-group-wiki'        => 'Wikidata og værktøjer',
'specialpages-group-redirects'   => 'Specialsider der viderestiller',
'specialpages-group-spam'        => 'Spamværktøjer',

# Special:BlankPage
'blankpage'              => 'Blank side',
'intentionallyblankpage' => 'Denne side er bevidst uden indhold.',

);
