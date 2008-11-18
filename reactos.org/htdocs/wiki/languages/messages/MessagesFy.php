<?php
/** Western Frisian (Frysk)
 *
 * @ingroup Language
 * @file
 *
 * @author Maartenvdbent
 * @author Pyt
 * @author Snakesteuben
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard' => 'Standert',
	'nostalgia' => 'Nostalgy',
);

$datePreferences = array(
	'default',
	'fy normal',
	'ISO 8601',
);

$defaultDateFormat = 'fy normal';

$dateFormats = array(
	'fy normal time' => 'H.i',
	'fy normal date' => 'j M Y',
	'fy normal both' => 'j M Y, H.i',
);

$datePreferenceMigrationMap = array(
	'default',
	'fy normal',
	'fy normal',
	'fy normal',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Wiki',
	NS_MAIN           => '',
	NS_TALK           => 'Oerlis',
	NS_USER           => 'Meidogger',
	NS_USER_TALK      => 'Meidogger_oerlis',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => '$1_oerlis',
	NS_IMAGE          => 'Ofbyld',
	NS_IMAGE_TALK     => 'Ofbyld_oerlis',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'MediaWiki_oerlis',
	NS_TEMPLATE       => 'Berjocht',
	NS_TEMPLATE_TALK  => 'Berjocht_oerlis',
	NS_HELP           => 'Hulp',
	NS_HELP_TALK      => 'Hulp_oerlis',
	NS_CATEGORY       => 'Kategory',
	NS_CATEGORY_TALK  => 'Kategory_oerlis'
);

$namespaceAliases = array(
	'Brûker' => NS_USER,
	'Brûker_oerlis' => NS_USER_TALK,
);

$separatorTransformTable = array(',' => '.', '.' => ',' );
$linkTrail = '/^([a-zàáèéìíòóùúâêîôûäëïöü]+)(.*)$/sDu';

$messages = array(
# User preference toggles
'tog-underline'               => 'Keppelings ûnderstreekje:',
'tog-highlightbroken'         => 'Keppelings nei lege siden ta <a href="" class="new">read</a> (oars mei in fraachteken<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Paragrafen útfolje',
'tog-hideminor'               => "Tekstwizigings wei litte út 'Koartlyn feroare'",
'tog-extendwatchlist'         => 'Wreidzje folchlist út om alle wizigings sjen te litten',
'tog-usenewrc'                => "Utwreide ferzje fan 'Koartlyn feroare' brûke (net mei alle blêdzjers mooglik)",
'tog-numberheadings'          => 'Koppen fansels nûmerje',
'tog-showtoolbar'             => 'Brûk arkbalke by bewurkjen',
'tog-editondblclick'          => 'Dûbelklik jout bewurkingsside (freget JavaScript)',
'tog-editsection'             => 'Jou [bewurk]-keppelings foar seksjebewurking',
'tog-editsectiononrightclick' => 'Rjochtsklik op sekjsetitels jout seksjebewurking (freget JavaScript)',
'tog-showtoc'                 => 'Ynhâldsopjefte, foar siden mei mear as twa koppen',
'tog-rememberpassword'        => 'Oare kear fansels oanmelde',
'tog-editwidth'               => 'Bewurkingsfjild sa breed as de side',
'tog-watchcreations'          => "Set siden dy't jo begjinne yn jo folchlist",
'tog-watchdefault'            => "Sides dy't jo feroare hawwe folgje",
'tog-watchmoves'              => "Siden dy't jo werneamd hawwe folgje",
'tog-watchdeletion'           => "Siden dy't jo wiske hawwe folgje",
'tog-minordefault'            => 'Feroarings yn it earst oanjaan as tekstwizigings.',
'tog-previewontop'            => 'By it neisjen, bewurkingsfjild ûnderoan sette',
'tog-previewonfirst'          => 'Lit foarbyld sjen by earste wiziging',
'tog-nocache'                 => 'Gjin oerslach brûke',
'tog-enotifwatchlistpages'    => 'E-mail my as in side op myn folchlist feroare is.',
'tog-enotifusertalkpages'     => 'E-mail my as myn oerlisside feroare wurdt',
'tog-enotifminoredits'        => 'E-mail my ek by lytse feroarings fan siden op myn folchlist',
'tog-enotifrevealaddr'        => 'Myn e-mailadres sjen litte yn e-mailberjochten',
'tog-shownumberswatching'     => 'It tal brûkers sjen litte dat dizze side folget',
'tog-fancysig'                => 'Sinjatuer sûnder automatyske ferwizing nei dyn Meidoggerside',
'tog-externaleditor'          => 'Standert in eksterne tekstbewurker brûke (allinne foar experts - foar dizze funksje binne spesjale ynstellings nedich)',
'tog-externaldiff'            => 'Standert in ekstern ferlikingsprogramma brûke (allinne foar experts - foar dizze funksje binne spesjale ynstellings nedich)',
'tog-showjumplinks'           => '"gean nei"-tapaslikens-links ynskeakelje',
'tog-uselivepreview'          => '"live proefbyld" brûke (JavaScript nedich - eksperimenteel)',
'tog-forceeditsummary'        => 'Warskôgje at ik de gearfetting leech lit.',
'tog-watchlisthideown'        => 'Eigen bewurkings op myn folchlist ferbergje',
'tog-watchlisthidebots'       => 'Lit gjin bot wizigings sjen yn de folchlist',
'tog-watchlisthideminor'      => 'Lit gjin tekstwizigings sjen yn de folchlist',
'tog-nolangconversion'        => 'Fariantomsetting útskeakelje',
'tog-ccmeonemails'            => "Stjoer my in kopy fan e-mails dy't ik nei oare brûkers stjoer",
'tog-diffonly'                => "Side-ynhâld dy't feroare wurdt net sjen litte",
'tog-showhiddencats'          => 'Ferburgen kategoryen werjaan',

'underline-always'  => 'Altyd',
'underline-never'   => 'Nea',
'underline-default' => 'Webblêder-standert',

'skinpreview' => '(Proefbyld)',

# Dates
'sunday'        => 'snein',
'monday'        => 'moandei',
'tuesday'       => 'tiisdei',
'wednesday'     => 'woansdei',
'thursday'      => 'tongersdei',
'friday'        => 'freed',
'saturday'      => 'sneon',
'sun'           => 'si',
'mon'           => 'mo',
'tue'           => 'ti',
'wed'           => 'wo',
'thu'           => 'to',
'fri'           => 'fr',
'sat'           => 'so',
'january'       => 'jannewaris',
'february'      => 'febrewaris',
'march'         => 'maart',
'april'         => 'april',
'may_long'      => 'maaie',
'june'          => 'juny',
'july'          => 'july',
'august'        => 'augustus',
'september'     => 'septimber',
'october'       => 'oktober',
'november'      => 'novimber',
'december'      => 'desimber',
'january-gen'   => 'jannewaris',
'february-gen'  => 'febrewaris',
'march-gen'     => 'maart',
'april-gen'     => 'april',
'may-gen'       => 'maaie',
'june-gen'      => 'juny',
'july-gen'      => 'july',
'august-gen'    => 'augustus',
'september-gen' => 'septimber',
'october-gen'   => 'oktober',
'november-gen'  => 'novimber',
'december-gen'  => 'desimber',
'jan'           => 'jan',
'feb'           => 'feb',
'mar'           => 'mrt',
'apr'           => 'apr',
'may'           => 'maaie',
'jun'           => 'jun',
'jul'           => 'jul',
'aug'           => 'aug',
'sep'           => 'sep',
'oct'           => 'okt',
'nov'           => 'nov',
'dec'           => 'des',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategory|Kategoryen}}',
'category_header'                => 'Siden yn de kategory "$1"',
'subcategories'                  => 'Subkategoryen',
'category-media-header'          => 'Media yn de kategory "$1"',
'category-empty'                 => "''Yn dizze kategory binne gjin siden of triemmen opnaam.''",
'hidden-categories'              => 'Ferburgen {{PLURAL:$1|kategory|kategoryen}}',
'hidden-category-category'       => 'Ferburgen kategoryen', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Dizze kategory hat allinne de folgjende ûnderkategory.|Dizze kategory hat de folgjende {{PLURAL:$1|ûnderkategory|$1 ûnderkategoryen}}, fan in totaal fan $2.}}',
'category-subcat-count-limited'  => 'Dizze kategory hat de folgjende {{PLURAL:$1|ûnderkategory|$1 ûnderkategoryen}}.',
'category-article-count'         => '{{PLURAL:$2|Dizze kategory befettet allinne de folgjende side.|De folgjende {{PLURAL:$1|side is|$1 siden binne}} yn dizze kategory, fan yn totaal $2.}}',
'category-article-count-limited' => 'De folgjende {{PLURAL:$1|side is|$1 siden binne}} yn dizze kategory.',
'category-file-count'            => '{{PLURAL:$2|Dizze kategory befettet de folgjende triem.|Dizze kategory befettet {{PLURAL:$1|de folgjende triem|$1 de folgjende triemmen}}, fan yn totaal $2.}}',
'category-file-count-limited'    => 'Dizze kategory befettet {{PLURAL:$1|de folgjende triem|de folgjende $1 triemmen}}.',
'listingcontinuesabbrev'         => '(ferfolch)',

'mainpagetext'      => 'Wiki-program goed ynstallearre.',
'mainpagedocfooter' => "Rieplachtsje de [http://meta.wikimedia.org/wiki/Help:Ynhâldsopjefte hantlieding] foar ynformaasje oer it gebrûk fan 'e wikisoftware.

== Mear help oer Mediawiki ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings List mei ynstellings]
* [http://www.mediawiki.org/wiki/Manual:FAQ Faak stelde fragen (FAQ)]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Mailinglist foar oankundigings fan nije ferzjes]",

'about'          => 'Ynfo',
'article'        => 'Ynhâld side',
'newwindow'      => '(nij finster)',
'cancel'         => 'Ferlitte',
'qbfind'         => 'Sykje',
'qbbrowse'       => 'Blêdzje',
'qbedit'         => 'Bewurkje',
'qbpageoptions'  => 'Side-opsjes',
'qbpageinfo'     => 'Side-ynfo',
'qbmyoptions'    => 'Myn Opsjes',
'qbspecialpages' => 'Bysûndere siden',
'moredotdotdot'  => 'Mear...',
'mypage'         => 'Myn side',
'mytalk'         => 'Myn oerlis',
'anontalk'       => 'Oerlisside foar dit IP-adres',
'navigation'     => 'Navigaasje',
'and'            => 'en',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fout',
'returnto'          => 'Werom nei "$1".',
'tagline'           => 'Ut {{SITENAME}}',
'help'              => 'Help',
'search'            => 'Sykje',
'searchbutton'      => 'Sykje',
'go'                => 'Side',
'searcharticle'     => 'Side',
'history'           => 'Sideskiednis',
'history_short'     => 'Skiednis',
'updatedmarker'     => 'bewurke sûnt myn lêste besite',
'info_short'        => 'Ynformaasje',
'printableversion'  => 'Ofdruk-ferzje',
'permalink'         => 'Fêste keppeling',
'print'             => 'Ofdrukke',
'edit'              => 'Wizigje',
'create'            => 'Oanmeitsje',
'editthispage'      => 'Side bewurkje',
'create-this-page'  => 'Dizze side oanmeitsje',
'delete'            => 'Wiskje',
'deletethispage'    => 'Side wiskje',
'undelete_short'    => '$1 {{PLURAL:$1|ferzje|ferzjes}} weromsette',
'protect'           => 'Skoattel',
'protect_change'    => 'feroarje nivo fan skoatteljen',
'protectthispage'   => 'Side skoattelje',
'unprotect'         => 'Jou frij',
'unprotectthispage' => 'Side frij jaan',
'newpage'           => 'Nije side',
'talkpage'          => 'Sideoerlis',
'talkpagelinktext'  => 'Oerlis',
'specialpage'       => 'Bysûndere side',
'personaltools'     => 'Persoanlike ynstellings',
'postcomment'       => 'Skriuw in opmerking',
'articlepage'       => 'Side lêze',
'talk'              => 'Oerlis',
'views'             => 'Aspekten/aksjes',
'toolbox'           => 'Arkkiste',
'userpage'          => 'Meidoggerside',
'projectpage'       => 'Metaside',
'imagepage'         => 'Ofbyldside',
'mediawikipage'     => 'Berjochtside sjen litte',
'templatepage'      => 'Berjochtside lêze',
'viewhelppage'      => 'Helpside sjen litte',
'categorypage'      => 'Besjoch kategoryside',
'viewtalkpage'      => 'Oerlisside',
'otherlanguages'    => 'Oare talen',
'redirectedfrom'    => '(Trochwiisd fan "$1")',
'redirectpagesub'   => 'Trochferwiis-side',
'lastmodifiedat'    => 'Lêste kear bewurke op $2, $1.', # $1 date, $2 time
'viewcount'         => 'Disse side is {{PLURAL:$1|ienris|$1 kear}} iepenslein.',
'protectedpage'     => 'Skoattele side',
'jumpto'            => 'Gean nei:',
'jumptonavigation'  => 'navigaasje',
'jumptosearch'      => 'sykje',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Oer de {{SITENAME}}',
'aboutpage'            => 'Project:Ynfo',
'bugreports'           => 'Brekmelding',
'bugreportspage'       => 'Project:Brekmelding',
'copyright'            => 'Ynhâld is beskikber ûnder de $1.',
'copyrightpagename'    => '{{SITENAME}} auteursrjocht',
'copyrightpage'        => '{{ns:project}}:Auteursrjocht',
'currentevents'        => 'Hjoeddeis',
'currentevents-url'    => 'Project:Rinnende saken',
'disclaimers'          => 'Foarbehâld',
'disclaimerpage'       => 'Project:Algemien foarbehâld',
'edithelp'             => 'Bewurk-help',
'edithelppage'         => 'Help:Bewurk-rie',
'faq'                  => 'FAQ (faak stelde fragen)',
'faqpage'              => 'Project:Faak stelde fragen',
'helppage'             => 'Help:Help',
'mainpage'             => 'Haadside',
'mainpage-description' => 'Haadside',
'policy-url'           => 'Project:Belied',
'portal'               => 'Brûkersportaal',
'portal-url'           => 'Project:Brûkersportaal',
'privacy'              => 'Privacybelied',
'privacypage'          => 'Project:Privacybelied',

'badaccess'        => 'Gjin tastimming',
'badaccess-group0' => 'Jo hawwe gjin rjochten om de frege hanneling út te fieren.',
'badaccess-group1' => "De frege hanneling is foarbehâlden oan brûkers yn'e groep $1.",
'badaccess-group2' => "De frege hanneling is foarbehâlden oan brûkers yn ien fan'e groepen $1.",
'badaccess-groups' => "De frege hanneling is foarbehâlden oan brûkers yn ien fan 'e groepen $1.",

'versionrequired'     => 'Ferzje $1 fan MediaWiki is eask',
'versionrequiredtext' => "Ferzje $1 fan MediaWiki is eask om dizze side te brûken. Mear ynfo is beskikber op 'e side [[Special:Version|softwareferzje]].",

'ok'                      => 'Goed',
'retrievedfrom'           => 'Untfongen fan "$1"',
'youhavenewmessages'      => 'Jo hawwe $1 ($2).',
'newmessageslink'         => 'nije berjochten',
'newmessagesdifflink'     => 'ferskil mei foarlêste ferzje',
'youhavenewmessagesmulti' => 'Jo hawwe nije berjochten op $1',
'editsection'             => 'bewurkje',
'editold'                 => 'bewurkje',
'viewsourceold'           => 'boarnetekst besjen',
'editsectionhint'         => 'Dielside bewurkje: $1',
'toc'                     => 'Ynhâld',
'showtoc'                 => 'sjen litte',
'hidetoc'                 => 'net sjen litte',
'thisisdeleted'           => '"$1" lêze of werombringe?',
'viewdeleted'             => '$1 sjen litte?',
'restorelink'             => '$1 wiske {{PLURAL:$1|ferzje|ferzjes}}',
'feedlinks'               => 'Feed:',
'feed-invalid'            => 'Feedtype wurdt net stipe.',
'feed-unavailable'        => 'Syndikaasjefeeds binne net beskikber',
'site-rss-feed'           => '$1 RSS Feed',
'site-atom-feed'          => '$1 Atom-Feed',
'page-rss-feed'           => '"$1" RSS Feed',
'page-atom-feed'          => '"$1" Atom Feed',
'red-link-title'          => '$1 (noch net skreaun)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Side',
'nstab-user'      => 'Meidogger',
'nstab-media'     => 'Mediaside',
'nstab-special'   => 'Spesjaal',
'nstab-project'   => 'Projektside',
'nstab-image'     => 'Triem',
'nstab-mediawiki' => 'Berjocht',
'nstab-template'  => 'Berjocht',
'nstab-help'      => 'Helpside',
'nstab-category'  => 'Kategory',

# Main script and global functions
'nosuchaction'      => 'Unbekende aksje.',
'nosuchactiontext'  => "De aksje dy't jo oanjoegen fia de URL is net bekind by it Wiki-program",
'nosuchspecialpage' => 'Unbekende side',
'nospecialpagetext' => "Jo hawwe in Wiki-side opfrege dy't net bekind is by it Wiki-programma.",

# General errors
'error'                => 'Fout',
'databaseerror'        => 'Databankfout',
'dberrortext'          => 'Sinboufout in databankfraach.
De lêst besochte databankfraach wie:
<blockquote><tt>$1</tt></blockquote>
fan funksje "<tt>$2</tt>" út.
MySQL joech fout "<tt>$3: $4</tt>" werom.',
'dberrortextcl'        => 'Sinboufout in databankfraach.
De lêst besochte databankfraach wie:
"$1"
fan funksje "$2" út.
MySQL joech fout "<tt>$3: $4</tt>" werom.',
'noconnect'            => 'Troch in fout yn de technyk, kin de Wiki gjin ferbining meitsje mei de databanktsjinner. <br />
$1',
'nodb'                 => 'Kin databank "$1" net berikke.',
'cachederror'          => "Dit is in bewarre ferzje dy't mooglik ferâldere is.",
'laggedslavemode'      => 'Warskôging: Mûglik binne resinte bewurkings noch net trochfierd.',
'readonly'             => "Databank is 'Net-skriuwe'.",
'enterlockreason'      => "Skriuw wêrom de databank 'net-skriuwe' makke is, en hoenear't men wêr nei alle gedachten wer skriuwe kin.",
'readonlytext'         => 'De {{SITENAME}} databank is ôfsletten foar nije siden en oare wizigings,
nei alle gedachten is it foar ûnderhâld, en kinne jo der letter gewoan wer brûk fan meitsje.
De behearder hat dizze útlis jûn:
<p>$1</p>',
'missing-article'      => 'Yn de database is gjin ynhâld oantroffen foar de side "$1" dy\'t der wol wêze moatte soe ($2). 

Dat kin foarkomme as Jo in ferâldere ferwizing nei it ferskil tusken twa ferzjes fan in side folgje of in ferzje opfreegje dy\'t wiske is.

As dat net sa is, hawwe Jo faaks in fout yn \'e software fûn.
Meitsje dêr melding fan by in [[Special:ListUsers/sysop|systeembehearder]] fan {{SITENAME}} en neam dêrby de URL fan dizze side.',
'missingarticle-rev'   => '(ferzjenûmer: $1)',
'missingarticle-diff'  => '(Feroaring: $1, $2)',
'readonly_lag'         => 'De database is automatysk beskoattele wylst de ûndergeskikte databaseservers syngronisearje mei de haadserver.',
'internalerror'        => 'Ynterne fout',
'internalerror_info'   => 'Ynterne fout: $1',
'filecopyerror'        => 'Koe triem "$1" net kopiearje as "$2".',
'filerenameerror'      => 'Koe triem "$1" net werneame as "$2".',
'filedeleteerror'      => 'Koe triem "$1" net wiskje.',
'directorycreateerror' => 'Map "$1" koe net oanmakke wurde.',
'filenotfound'         => 'Koe triem "$1" net fine.',
'fileexistserror'      => 'Skriuwen nei triem "$1" ûnmûglik: de triem bestiet al',
'unexpected'           => 'Hommelse wearde: "$1"="$2".',
'formerror'            => 'Fout: koe formulier net oerlizze',
'badarticleerror'      => 'Dat kin op dizze side net dien wurden.',
'cannotdelete'         => 'Koe de oantsjutte side of it oantsjutte ôfbyld net wiskje. (Faaks hat in oar dat al dien.)',
'badtitle'             => 'Misse titel',
'badtitletext'         => 'De opfrege sidetitel wie ûnjildich, leech, of in miskeppele yntertaal of ynterwiki titel.',
'perfdisabled'         => "Sorry! Dit ûnderdiel is tydlik út set om't it de databank sa starich makket
dat gjinien de wiki brûke kin.",
'perfcached'           => "Dit is bewarre ynformaasje dy't mooglik ferâldere is.",
'perfcachedts'         => 'De neikommende gegevens komme út de bewarre ynformaasje, dizze is it lêst fernijd op $1.',
'querypage-no-updates' => 'Dizze side kin net bywurke wurde. Dizze gegevens wurde net ferfarske.',
'wrong_wfQuery_params' => 'Ferkearde parameters foar wfQuery()<br />
Funksje: $1<br />
Query: $2',
'viewsource'           => 'Besjoch de boarne',
'viewsourcefor'        => 'fan $1',
'actionthrottled'      => 'Hanneling opkeard',
'actionthrottledtext'  => 'As maatregel tsjin spam is it tal kearen per tiidsienheid beheind dat jo dizze hanneling ferrjochtsje kinne. Jo binne oer de limyt. Besykje it in tal minuten letter wer.',
'protectedpagetext'    => 'Dizze side is befeilige. Bewurkjen is net mûglik.',
'viewsourcetext'       => 'Jo kinne de boarnetekst fan dizze side besjen en kopiearje:',
'protectedinterface'   => "Dizze side befettet tekst foar berjochten fan 'e software en is befeilige om misbrûk tefoaren te kommen.",
'editinginterface'     => "'''Warskôging;''' Jo bewurkje in side dy't brûkt wurdt troch software. Bewurkings op dizze side beynfloedzje de gebrûksynterface fan elkenien. Oerweagje foar oersettings [http://translatewiki.net/wiki/Main_Page?setlang=fy Betawiki] te brûken, it oersetprojekt foar MediaWiki.",
'sqlhidden'            => '(SQL query ferburgen)',
'cascadeprotected'     => 'Dizze side is skoattele tsjin wizigjen, om\'t der in ûnderdiel útmakket fan de neikommende {{PLURAL:$1|side|siden}}, dy\'t skoattele {{PLURAL:$1|is|binne}} mei de "ûnderlizzende siden" opsje ynskeakele: $2',
'namespaceprotected'   => "Jo hawwe gjin rjochten om siden yn'e nammerûmte '''$1''' te bewurkjen.",
'customcssjsprotected' => "Jo kinne dizze side net bewurkje, om't er persoanlike ynstellings fan in oare brûker befettet.",
'ns-specialprotected'  => "Siden yn'e nammerûmte {{ns:special}} kinne net bewurke wurde.",
'titleprotected'       => "It oanmeitsjen fan dizze side is befeilige troch [[User:$1|$1]].
De oanfierde reden is ''$2''.",

# Virus scanner
'virus-badscanner'     => 'Minne konfiguraasje: ûnbekende virusscanner: <i>$1</i>',
'virus-scanfailed'     => 'scannen is mislearre (koade $1)',
'virus-unknownscanner' => 'ûnbekend antivirus:',

# Login and logout pages
'logouttitle'                => 'Ofmelde',
'logouttext'                 => "<strong>Jo binne no ôfmeld.</strong>

Jo kinne de {{SITENAME}} fierders anonym brûke,
of jo op 'e [[Special:UserLogin|nij oanmelde]] ûnder deselde of in oare namme.
Mûglik wurdt noch in tal siden werjûn as wiene Jo oanmeld, oant Jo de cache fan Jo browser leegje.",
'welcomecreation'            => '<h2>Wolkom, $1!</h2><p>Jo ynstellings binne oanmakke.
Ferjit net se oan jo foarkar oan te passen.',
'loginpagetitle'             => 'Oanmelde',
'yourname'                   => 'Jo meidochnamme:',
'yourpassword'               => 'Jo wachtwurd',
'yourpasswordagain'          => 'Jo wachtwurd (nochris)',
'remembermypassword'         => 'Oare kear fansels oanmelde.',
'yourdomainname'             => 'Jo domein:',
'externaldberror'            => 'Der is in fout by it oanmelden by de database of jo hawwe gjin tastimming om jo ekstern account by te wurkjen.',
'loginproblem'               => '<b>Der wie wat mis mei jo oanmelden.</b><br />Besykje it nochris, a.j.w.',
'login'                      => 'Oanmelde',
'nav-login-createaccount'    => 'Oanmelde',
'loginprompt'                => "Jo moatte 'cookies' oanstean hawwe om yn jo oan te melden by {{SITENAME}}.",
'userlogin'                  => 'Oanmelde',
'logout'                     => 'Ofmelde',
'userlogout'                 => 'Ofmelde',
'notloggedin'                => 'Net oanmelde',
'nologin'                    => 'Noch net oanmelden as meidogger? $1.',
'nologinlink'                => 'Meitsje in brûker oan',
'createaccount'              => 'Nije ynstellings oanmeitsje',
'gotaccount'                 => 'Hawwe jo jo al as meidogger oanmelde? $1.',
'gotaccountlink'             => 'Oanmelde',
'createaccountmail'          => 'troch e-mail',
'badretype'                  => 'De ynfierde wachtwurden binne net lyk.',
'userexists'                 => 'Dy meidochnamme wurdt al brûkt. Besykje in oarenien.',
'youremail'                  => 'Jo e-postadres (*).',
'username'                   => 'Meidochnamme:',
'uid'                        => 'Wikinûmer:',
'prefs-memberingroups'       => 'Lid fan {{PLURAL:$1|groep|groepen}}:',
'yourrealname'               => 'Jo wiere namme:',
'yourlanguage'               => 'Taal:',
'yournick'                   => 'Jo alias (foar sinjaturen)',
'badsig'                     => 'Unjildige ûndertekening; kontrolearje de HTML-tags.',
'badsiglength'               => 'Bynamme is te lang; dy moat koarter as $1 {{PLURAL:$1|teken|tekens}} wêze.',
'email'                      => 'E-post',
'prefs-help-realname'        => 'Echte namme is opsjoneel; as jo dy opjouwe kin dy namme brûkt wurde om jo erkenning te jaan foar jo wurk.',
'loginerror'                 => 'Oanmeldflater',
'prefs-help-email'           => 'E-post is opsjoneel, mar makket it mûglik jo wachtwurd te stjoeren as jo it fergetten hawwe.
Jo kinne ek oaren de mûglikheid jaan kontakt mei jo op te nimmen troch in ferwizing op jo brûkers- en oerlisside, sûnder dat jo jo identiteit oer hoege te jaan.',
'prefs-help-email-required'  => 'Hjir is in e-mailadres foar nedich.',
'nocookiesnew'               => 'De brûker is oanmakke mar net oanmeld. {{SITENAME}} brûkt cookies foar it oanmelden fan brûkers. Skeakelje dy yn en meld jo dan oan mei jo nije brûkersnamme en wachtwurd.',
'nocookieslogin'             => '{{SITENAME}} brûkt cookies foar it oanmelden fan brûkers. Jo hawwe cookies útskeakele. Skeakelje dy opsje oan en besykje it nochris.',
'noname'                     => 'Jo moatte in meidognamme opjaan.',
'loginsuccesstitle'          => 'Oanmelden slagge.',
'loginsuccess'               => "'''Jo binne no oanmelden op de {{SITENAME}} as: \"\$1.\"'''",
'nosuchuser'                 => 'Der is gjin meidogger "$1".
Kontrolearje de stavering, of [[Special:Userlogin/signup|meitsje in nije meidogger oan]].',
'nosuchusershort'            => 'Der is gjin meidogger mei de namme "$1". It is goed skreaun?',
'nouserspecified'            => 'Jo moatte in brûkersnamme opjaan.',
'wrongpassword'              => "Meidochnamme en wachtwurd hearre net by elkoar. Besykje op 'e nij, of fier it wachtwurd twa kear yn en meitsje nije meidoggersynstellings.",
'wrongpasswordempty'         => 'It opjûne wachtwurd wie leech. Besykje it nochris.',
'passwordtooshort'           => 'It wachtwurd is te koart. It moat op syn minst {{PLURAL:$1|1 teken|$1 tekens}} wêze.',
'mailmypassword'             => 'Stjoer my in nij wachtwurd.',
'passwordremindertitle'      => 'Nij wachtwurd foar de {{SITENAME}}',
'passwordremindertext'       => 'Immen (nei alle gedachten jo, fan Ynternet-adres $1)
hat frege en stjoer jo in nij wachtwurd foar {{SITENAME}} ($4).
It tydlike wachtwurd foar brûker "$2" is no "$3".
Meld jo no oan, en feroarje jo wachtwurd, as jo dat wolle.

As immen oars as Jo dit fersyk dien hat of as it wachtwurd Jo yntuskentiid wer yn\'t sin kommen is en Jo it net langer feroarje wolle, lûk Jo dan neat fan dit berjocht oan en gean troch mei it brûken fan Jo besteand wachtwurd.',
'noemail'                    => 'Der is gjin e-postadres foar meidogger "$1".',
'passwordsent'               => 'In nij wachtwurd is tastjoerd oan it e-postadres foar "$1". Jo kinne jo wer oanmelde as jo it wachtwurd ûntfongen hawwe.',
'blocked-mailpassword'       => 'Jo IP-adres is blokkearre foar it meitsjen fan feroarings. Om misbrûk tefoaren te kommen is it net mûglik in oar wachtwurd oan te freegjen.',
'eauthentsent'               => "Foar befêstiging is jo in netpostberjocht tastjoerd op it adres dat jo ynsteld hawwe. Der wurdt gjin oare netpost stjoerd, oant jo it adres befêstigje sa't it yn it netpostberjocht stiet.",
'throttled-mailpassword'     => "Yn {{PLURAL:$1|de lêste oere|de lêste $1 oeren}} is der al in wachtwurdwink ferstjoerd.
Om misbrûk tefoaren te kommen wurdt der mar ien wachtwurdwink yn 'e {{PLURAL:$1|oere|$1 oeren}} ferstjoerd.",
'mailerror'                  => 'Fout by it ferstjoeren fan e-mail: $1',
'acct_creation_throttle_hit' => 'Sorry, jo hawwe al $1 meidochnammen oanmakke; jo kinne net mear oanmeitsje.',
'emailauthenticated'         => 'Jo netpostadres waard befêstige op $1.',
'emailnotauthenticated'      => 'Jo netpostadres is <strong>noch net befêstige</strong>. Jo kinne oare brûkers gjin post stjoere, en foar de neikommende opsjes wurdt jo gjin post stjoerd.',
'noemailprefs'               => 'Jou in e-mailadres op om dizze funksjes te brûken.',
'emailconfirmlink'           => 'Befêstigje jo netpostadres.',
'invalidemailaddress'        => "It e-mailadres is net akseptearre om't it in ûnjildige opmaak hat.
Jou beleaven in jildich e-mailadres op of lit it fjild leech.",
'accountcreated'             => 'Brûker oanmakke',
'accountcreatedtext'         => 'De brûker $1 is oanmakke.',
'createaccount-title'        => 'Brûkers registrearje foar {{SITENAME}}',
'createaccount-text'         => 'Immen hat in brûker op {{SITENAME}} ($4) oanmakke mei de namme "$2" en jo e-mailadres. It wachtwurd foar "$2" is "$3". Meld jo oan en feroarje jo wachtwurd.

Negearje it berjocht as dizze brûker sûnder jo meiwitten oanmakke is.',
'loginlanguagelabel'         => 'Taal: $1',

# Password reset dialog
'resetpass'               => 'Wachtwurd opnij ynstelle',
'resetpass_announce'      => "Jo binne oanmeld mei in tydlike koade dy't jo per e-mail tastjoerd is. Fier in nij wachtwurd yn om it oanmelden ôf te meitsjen.",
'resetpass_header'        => "Wachtwurd op 'e nij ynstelle",
'resetpass_submit'        => 'Wachtwurd ynstelle en oanmelde',
'resetpass_success'       => 'Jo wachtwurd is feroare. Dwaande mei oanmelden ...',
'resetpass_bad_temporary' => 'Unjildich tydlik wachtwurd. Jo hawwe jo wachtwurd al feroare of in nij tydlik wachtwurd oanfrege.',
'resetpass_forbidden'     => 'Wachtwurden kinne net feroare wurde',
'resetpass_missing'       => 'Jo hawwe gjin wachtwurd ynjûn.',

# Edit page toolbar
'bold_sample'     => 'Fette tekst',
'bold_tip'        => 'Fette tekst',
'italic_sample'   => 'Skeane tekst',
'italic_tip'      => 'Skeane tekst',
'link_sample'     => 'Link titel',
'link_tip'        => 'Ynterne ferwizing',
'extlink_sample'  => 'http://www.example.com linktekst',
'extlink_tip'     => 'Eksterne link (ferjit http:// net)',
'headline_sample' => 'Koptekst',
'headline_tip'    => 'Underkopke',
'math_sample'     => 'Foechje hjir in formule yn',
'math_tip'        => 'Wiskundige formule (LaTeX)',
'nowiki_sample'   => 'Foechje hjir platte tekst yn',
'nowiki_tip'      => 'Negearje it wiki formaat',
'image_tip'       => 'Mediatriem',
'media_tip'       => 'Link nei triem',
'sig_tip'         => 'Jo hântekening mei dei en oere',
'hr_tip'          => 'Horizontale line (mei ferdrach brûke)',

# Edit pages
'summary'                          => 'Gearfetting',
'subject'                          => 'Mêd',
'minoredit'                        => 'Dit is in tekstwiziging',
'watchthis'                        => 'Folgje dizze side',
'savearticle'                      => 'Fêstlizze',
'preview'                          => 'Oerlêze',
'showpreview'                      => 'Earst oerlêze',
'showlivepreview'                  => 'Bewurking foar kontrôle besjen',
'showdiff'                         => 'Wizigings',
'anoneditwarning'                  => "'''Warskôging:''' Jo binne net oanmeld. By it fêstlizzen wurdt jo ynternetadres opnaam yn de sideskiednis.",
'missingsummary'                   => "'''Wink:''' jo hawwe gjin gearfetting jûn foar jo bewurking. As jo nochris op ''Side opslaan'' klikke wurdt de bewurking sûnder gearfetting opslein.",
'missingcommenttext'               => 'Set jo opmerking beleaven hjir ûnder.',
'missingcommentheader'             => "'''Tink derom:''' Jo hawwe gjin ûnderwerp/kop foar dizze opmerking opjûn. As jo op 'e nij op \"opslaan\" klikke, wurdt jo feroaring sûnder in ûnderwerp/kop opslein.",
'summary-preview'                  => 'Gearfetting sa at dy brûkt wurdt',
'subject-preview'                  => 'Neisjen ûnderwerp/kop',
'blockedtitle'                     => 'Meidogger is útsletten troch',
'blockedtext'                      => "<big>'''Jo meidoggernamme of Ynternet-adres is útsletten.'''</big>

De útsluting is útfierd troch $1.
De opjûne reden is ''$2''.

* Begjin útsluting : $8
* Ein útsluting : $6
* Bedoeld út te sluten: $7

Jo kinne kontakt opnimme mei $1 of in oare [[{{MediaWiki:Grouppage-sysop}}|behearder]] om de útsluting te besprekken.
Jo kinne gjin gebrûk meitsje fan 'e funksje 'Skriuw meidogger', of jo moatte in jildich e-postadres opjûn hawwe yn jo [[Special:Preferences|foarkarren]] en it gebrûk fan dy funksje moat net útsletten wêze.
Jo tsjintwurdich e-postadres is $3 en it útsletnûmer is #$5. Neam beide gegevens as jo earne op dizze útsluting reagearje.",
'autoblockedtext'                  => "Jo IP-adres is automatysk útsletten om't brûkt is troch in oare brûker, dy't útsletten is troch $1.
De opjûne reden is:

:''$2''

* Begjin útsluting : $8
* Ein útsluting : $6
* Bedoeld út te sluten: $7

Jo kinne kontakt opnimme mei $1 of in oare [[{{MediaWiki:Grouppage-sysop}}|behearder]] om de útsluting te besprekken.
Jo kinne gjin gebrûk meitsje fan 'e funksje 'Skriuw meidogger', of jo moatte in jildich e-postadres opjûn hawwe yn jo [[Special:Preferences|foarkarren]] en it gebrûk fan dy funksje moat net útsletten wêze.
Jo tsjintwurdich e-postadres is $3 en it útsletnûmer is #$5. Neam beide gegevens as jo earne op dizze útsluting reagearje.",
'blockednoreason'                  => 'gjin reden opjûn',
'blockedoriginalsource'            => "Hjir ûnder stiet de boarnetekst fan '''$1''':",
'blockededitsource'                => "Hjir ûnder stiet de tekst fan '''jo bewurkings''' oan '''$1''':",
'whitelistedittitle'               => 'Foar bewurkjen is oanmelden ferplichte',
'whitelistedittext'                => 'Jo moatte $1 om siden te bewurkjen.',
'confirmedittitle'                 => 'Netpostbefêstiging frege foar bewurkjen',
'confirmedittext'                  => "Jo moatte jo e-mailadres befêstichje foar't jo siden feroarje kinne. Fier in e-mailedres yn by jo [[Special:Preferences|ynstellings]] en befêstichje it.",
'nosuchsectiontitle'               => 'Dizze subkop bestiet net',
'nosuchsectiontext'                => "Jo besochten in subkop te bewurkjen dy't net bestiet. Om't subkop $1 net bestiet, kin jo bewurking ek net opslein wurde.",
'loginreqtitle'                    => 'Oanmelding frege',
'loginreqlink'                     => 'Oanmelde',
'loginreqpagetext'                 => 'Jo moatte jo $1 om oare siden besjen te kinnen.',
'accmailtitle'                     => 'Wachtwurd ferstjoerd.',
'accmailtext'                      => 'It wachtwurd foar "$1" is ferstjoerd nei $2.',
'newarticle'                       => '(Nij)',
'newarticletext'                   => "Jo hawwe in keppeling folge nei in side dêr't noch gjin tekst op stiet.
Om sels tekst te meistjsen kinne jo dy gewoan yntype in dit bewurkingsfjild
([[{{MediaWiki:Helppage}}|Mear ynformaasje oer bewurkjen]].)
Oars kinne jo tebek mei de tebek-knop fan jo blêder.",
'anontalkpagetext'                 => "----''Dit is de oerlisside fan in ûnbekende meidogger; in meidogger dy't him/har net oanmeld hat. Om't der gjin namme bekend is, wurdt it ynternet-adres brûkt om oan te jaan wa. Mar faak is it sa dat sa'n adres net altyd troch deselde persoan brûkt wurdt. As jo it idee hawwe dat jo as ûnbekende meidogger opmerkings foar in oar krije, dan kinne jo jo [[Special:UserLogin/signup|registrearje]], of jo [[Special:UserLogin|oanmelde]]. Fan in oanmelde meidogger is it ynternet-adres net sichtber, en as oanmelde meidogger krije jo allinnich opmerkings dy't foar josels bedoeld binne.''",
'noarticletext'                    => 'Der stjit noch gjin tekst op dizze side. Jo kinne
[[Special:Search/{{PAGENAME}}|hjirboppe nei dy tekst sykje]], of [{{fullurl:{{FULLPAGENAME}}|action=edit}} de side skriuwe].',
'userpage-userdoesnotexist'        => 'Jo bewurkje in brûkersside fan in brûker dy\'t net bestiet (brûker "$1").
Kontrolearje oft jo dizze side wol oanmeitsje/bewurkje wolle.',
'clearyourcache'                   => "'''Opmerking:''' Nei it fêstlizzen kin it nedich wêze de oerslach fan dyn blêder te leegjen foardat de wizigings te sjen binne.

'''Mozilla / Firefox / Safari:''' hâld ''Shift'' yntreaun wylst jo op ''Dizze side fernije'' klikke, of typ ''Ctrl-F5'' of ''Ctrl-R'' (''Command-R'' op in Macintosh); '''Konqueror: '''klik ''Reload'' of typ ''F5;'' '''Opera:''' leegje jo cache yn ''Extra → Voorkeuren;'' '''Internet Explorer:''' hâld ''Ctrl'' yntreaun wylst jo ''Vernieuwen'' klikke of typ ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Tip:</strong> Brûk de knop 'Earst oerlêze' om jo nije CSS/JS te testen foar it fêstlizzen.",
'usercsspreview'                   => "'''Dit is allinne mar it oerlêzen fan jo persoanlike CSS. Hy is noch net fêstlein!'''",
'userjspreview'                    => "'''Tink derom: jo besjogge no jo persoanlike JavaScript. De side is net fêstlein!'''",
'userinvalidcssjstitle'            => "'''Warskôging:''' der is gjin skin \"\$1\". Tink derom: jo eigen .css- en .js-siden begjinne mei in lytse letter, bygelyks {{ns:user}}:Namme/monobook.css ynsté fan {{ns:user}}:Namme/Monobook.css.",
'updated'                          => '(Bewurke)',
'note'                             => '<strong>Opmerking:</strong>',
'previewnote'                      => '<strong>Tink der om dat dizze side noch net fêstlein is!</strong>',
'previewconflict'                  => 'Dizze side belanget allinich it earste bewurkingsfjild oan.',
'session_fail_preview'             => "<strong>Jo bewurking is net ferwurke, om't de sessygegevens ferlern gien binne.
Besykje it nochris. As it dan noch net slagget, [[Special:UserLogout|meld jo dan ôf]] en wer oan.</strong>",
'session_fail_preview_html'        => "<strong>Jo bewurking is net ferwurke, om't sesjegegevens ferlern gien binne.</strong>

''Om't yn {{SITENAME}} rûge HTML ynskeakele is, is in foarfertoaning net mûglik as beskerming tsjin oanfallen mei JavaScript.''

<strong>As dit in legitime bewurking is, besykje it dan fannijs.
As it dan  noch net slagget, [[Special:UserLogout|meld jo dan ôf]] en wer oan.</strong>",
'token_suffix_mismatch'            => "<strong>Jo bewurking is wegere om't jo blêder de lêstekens yn it bewurkingstoken ûnkrekt behannele hat.
De bewurking is wegere om skeinen fan 'e sidetekst tefoaren te kommen.
Dat bart soms as der in webbasearre proxytsjinst brûkt wurdt dy't flaters befettet.</strong>",
'editing'                          => 'Bewurkje "$1"',
'editingsection'                   => 'Bewurkje $1 (seksje)',
'editingcomment'                   => 'Dwaande mei bewurkjen fan $1 (opmerking)',
'editconflict'                     => 'Tagelyk bewurke: "$1"',
'explainconflict'                  => "In oar hat de side feroare sûnt jo begûn binne mei it bewurkjen. It earste bewurkingsfjild is hoe't de tekst wilens wurden is. Jo feroarings stean yn it twadde fjild. Dy wurde allinnich tapast safier as jo se yn it earste fjild ynpasse. <b>Allinnich</b> de tekst út it earste fjild kin fêstlein wurde. <p>",
'yourtext'                         => 'Jo tekst',
'storedversion'                    => 'Fêstleine ferzje',
'nonunicodebrowser'                => '<strong>WARSKOGING: Jo browser kin net goed oer de wei mei unicode.
Dêr wurdt troch de MediaWiki software rekken mei holden, dat Jo kinne dan dochs sûnder problemen siden bewurkje: net-ASCII tekens wurden yn it bewurkingsfjild werjûn as heksadesimale koades.</strong>',
'editingold'                       => '<strong>Warskôging: Jo binne dwaande mei in âldere ferzje fan dizze side.
Soene jo dy fêstlizze, dan is alles wei wat sûnt dy tiid feroare is.</strong>',
'yourdiff'                         => 'Feroarings',
'copyrightwarning'                 => "Tink derom dat alle bydragen oan {{SITENAME}} beskôge wurde frijjûn te wêzen ûnder de $2 (sjoch $1 foar bysûnderheden). As jo net wolle dat jo tekst troch oaren neffens eigen goedfinen bewurke en ferspraat wurde kin, kies dan net foar 'Side Bewarje'.</br>
Hjirby sizze jo tagelyk ta, dat jo dizze tekst sels skreaun hawwe, of oernommen hawwe út in frije, iepenbiere boarne.</br/>
<strong>BRûK GJIN MATERIAAL DAT BESKERME WURDT TROCH AUTERURSRJOCHT, OF JO MOATTE DêR TASTIMMING TA HAWWE!</STRONG>",
'copyrightwarning2'                => 'Al jo bydragen oan {{SITENAME}} kinne bewurke, feroare of fuorthelle wurde troch oare brûkers.
As jo net wolle dat jo teksten yngeand oanpast wurde troch oaren, set se hjir dan net.<br />
Jo sizze ek ta dat jo de oarspronklike auteur binne fan dit materiaal, of dat jo it kopiearre hawwe út in boarne yn it publike domein, of in soartgelikense frije boarne (sjuch $1 foar details).
<strong>BRUK GJIN MATERIAAL DAT BESKERME WURDT TROCH AUTEURSRJOCHT, OF JO MOATTE DER TASTIMMING FOAR HAWWE!</strong>',
'longpagewarning'                  => "<strong>Warskôging: Dizze side is $1 kilobyte lang;
der binne blêders dy't problemen hawwe mei siden fan tsjin de 32kb. of langer.
Besykje de side yn lytsere stikken te brekken.</strong>",
'longpageerror'                    => "<strong>FOUT: de tekst dy't jo tafoege hawwe is $1 kilobyte grut, wat grutter is as it maksimum fan $2 kilobytes.
Bewarjen is net mûglik.</strong>",
'readonlywarning'                  => '<strong>Waarskôging: De databank is ôfsletten foar
ûnderhâld, dus jo kinne jo bewurkings no net fêstlizze.
It wie baas en nim de tekst foar letter oer yn in tekstbestân.</strong>',
'protectedpagewarning'             => '<strong>Waarskôging: Dizze side is beskerme, dat gewoane brûkers dy net bewurkje kinne.</strong>',
'semiprotectedpagewarning'         => "'''Tink derom:''' dizze side is befeilige en kin allinne troch registrearre brûkers bewurke wurde.",
'cascadeprotectedwarning'          => "'''Warskôging:''' Dizze side is skoattele sadat allinnich behearders de side wizigje kinne, om't der in ûnderdiel útmakket fan de neikommende {{PLURAL:\$1|side|siden}}, dy't skoattele binne mei de \"ûnderlizzende siden\" opsje ynskeakele:",
'titleprotectedwarning'            => '<strong>WARSKÔGING: Dizze side is befeilige, dat allinne inkelde brûkers kinne him oanmeitsje.</strong>',
'templatesused'                    => 'Berjochten brûkt op dizze side:',
'templatesusedpreview'             => 'Yn dit proefbyld sjabloanen:',
'templatesusedsection'             => "Sjabloanen dy't brûkt wurde yn dizze subkop:",
'template-protected'               => '(befeilige)',
'template-semiprotected'           => '(semi-befeilige)',
'hiddencategories'                 => 'Dizze side falt yn de folgjende ferburgen
{{PLURAL:$1|kategory|kategoryen}}:',
'edittools'                        => '<!-- Tekst hjir stiet ûnder bewurkingsfjilden en oanbringfjilden.  -->',
'nocreatetitle'                    => 'It oanmeitsjen fan siden is beheind',
'nocreatetext'                     => '{{SITENAME}} hat de mûglikheid beheind om nije siden te meitsjen.
Jo kinne al besteande siden feroarje of jo kinne [[Special:UserLogin|jo oanmelde of in brûker oanmeitsje]].',
'nocreate-loggedin'                => 'Jo meie gjin nije siden meitsje',
'permissionserrors'                => 'Flaters yn rjochten',
'permissionserrorstext'            => 'Jo hawwe gjin rjochtem dit te dwaan om de folgjende {{PLURAL:$1|reden|redenen}}:',
'permissionserrorstext-withaction' => 'Jo hawwe gjin rjocht ta $2 om de folgjende {{PLURAL:$1|reden|redenen}}:',
'recreate-deleted-warn'            => "'''Warskôging: Jo binne dwaande in side oan te meitsjen dy't earder weidien is.'''

Betink oft it gaadlik is dat jo dizze side fierder bewurkje. Foar jo geriif stiet hjirûnder it lochboek oer it weidwaan fan dizze side:",

# Parser/template warnings
'expensive-parserfunction-warning'        => 'Warskôging: Dizze side brûkt tefolle kostbere parserfunksjes.

No binne it $1, wylst it minder as $2 wêze moatte.',
'expensive-parserfunction-category'       => "Siden dy't tefolle kostbere parserfuksjes brûke",
'post-expand-template-inclusion-warning'  => 'Warskôging: jo geane oer de maksimale opnamegrutte foar sjabloanen.
Guon sjabloanen wurden net opnommen.',
'post-expand-template-inclusion-category' => "Side wêrfoar't de maksimale trânsklúzjegrutte teboppe gien is",
'post-expand-template-argument-warning'   => 'Warskôging: Dizze side befettet minstens ien sjabloanparameter mei in te grutte opnamegrutte.
Dy parameters binne weilitten.',
'post-expand-template-argument-category'  => "Siden dy't missende sjabloaneleminten befetsje",

# "Undo" feature
'undo-success' => 'De feroaring kin werom set wurde. Kontrolearje de ferliking hjirûnder om wis te wêzen dat jo dit feroarje wolle en druk dan op fêstlizze om it werom setten troch te fieren.',
'undo-failure' => 'De feroarings kinne net werom set wurde troch in konflikt mei oare feroarings tuskentroch.',
'undo-norev'   => 'De feroaring kin werom set wurde, omdat it net bestiet of is wiske.',
'undo-summary' => 'Werom sette fan ferzje $1 fan [[Special:Contributions/$2|$2]] ([[User talk:$2|Oerlis]])',

# Account creation failure
'cantcreateaccounttitle' => 'Registrearjen is mislearre.',
'cantcreateaccount-text' => "Registraasje fan in brûker fia dit IP-adres ('''$1''') is blokkearre troch [[User:$3|$3]].

De fan $3 opjûne reden is ''$2''",

# History pages
'viewpagelogs'        => 'Lochboek foar dizze side sjen litte',
'nohistory'           => 'Dit is de earste ferzje fan de side.',
'revnotfound'         => 'Ferzje net fûn',
'revnotfoundtext'     => "De âlde ferzje fan dizze side dêr't jo om frege hawwe, is der net.
Gean nei of de keppeling dy jo brûkt hawwe wol goed is.",
'currentrev'          => 'Aktuele ferzje',
'revisionasof'        => 'Ferzje op $1',
'revision-info'       => 'Ferzje op $1 fan $2',
'previousrevision'    => '←Eardere ferskillen',
'nextrevision'        => 'Nijere ferzje→',
'currentrevisionlink' => 'Rinnende ferzje',
'cur'                 => 'no',
'next'                => 'dan',
'last'                => 'doe',
'page_first'          => 'earste',
'page_last'           => 'lêste',
'histlegend'          => "Utlis: (no) = ferskil mei de side sa't dy no is,
(doe) = ferskill mei de side sa't er doe wie, foar de feroaring, T = Tekstwiziging",
'deletedrev'          => '[fuorthelle]',
'histfirst'           => 'Ierst',
'histlast'            => 'Lêst',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(leech)',

# Revision feed
'history-feed-title'          => 'Sideskiednis',
'history-feed-description'    => 'Sideskiednis foar dizze side op de wiki',
'history-feed-item-nocomment' => '$1 op $2', # user at time
'history-feed-empty'          => 'De frege side bestiet net.
Faaks is dy fuorthelle of omneamd.
[[Special:Search|Sykje de wiki troch]] foar relevante nije siden.',

# Revision deletion
'rev-deleted-comment'         => '(opmerking wiske)',
'rev-deleted-user'            => '(meidoggernamme wiske)',
'rev-deleted-event'           => '(logboekrigel fuorthelle)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Dy bewurking fan de side is fuorthelle út de publike argiven.
Der kinne details oanwêzich wêze yn it [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} fuorthel-logboek].
</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">Dizze bewurking fan de side is fuorthelle út de publike argiven.
As behearder fan {{SITENAME}} kinne jo him besjen;
der kinne details wêze yn it [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} fuorthel-logboek].</div>',
'rev-delundel'                => 'sjen litte/ferbergje',
'revisiondelete'              => 'Wiskje/weromsette ferzjes',
'revdelete-nooldid-title'     => 'Gjin doelferzje',
'revdelete-nooldid-text'      => 'Jo hawwe gjin doelferzje(s) foar dizze hanneling opjûn, de oanjûne ferzje bestiet net, of jo besykje de lêste ferzje te ferskûljen.',
'revdelete-selected'          => 'Spesifisearre {{PLURAL:$2|ferzje|ferzjes}} fan [[:$1]]:',
'logdelete-selected'          => '{{PLURAL:$1|keazen lochboekregel|keazen lochboekregels}}:',
'revdelete-text'              => "Fuorthelle bewurkings binne sichtber yn 'e skiednis, mar de ynhâld is net langer publyk tagonklik.
Oare behearders fan {{SITENAME}} kinne de ferburgen ynhâld benaderje en it fuortheljen ûngedien meitsje mei help fan dit skerm, of der moatte oanfoljende beheinings jilde dy't ynsteld binne troch de systeembehearder.",
'revdelete-legend'            => 'Sichtberensbeheinings ynstelle.',
'revdelete-hide-text'         => 'De bewurke tekst ferskûlje',
'revdelete-hide-name'         => 'Aksje en doel ferskûlje',
'revdelete-hide-comment'      => 'De bewurkingsgearfetting ferskûlje',
'revdelete-hide-user'         => 'Meidoggernamme/IP fan de meidogger ferskûlje',
'revdelete-hide-restricted'   => 'Dizze beheinings tapasse op behearders en dizze ynterface ôfslute',
'revdelete-suppress'          => 'Gegevens foar behearders likegoed as oaren ûnderdrukke',
'revdelete-hide-image'        => 'Triem ynhâld ferskûlje',
'revdelete-unsuppress'        => 'Beheinings op tebeksette feroarings fuorthelje',
'revdelete-log'               => 'Opmerking yn logboek:',
'revdelete-submit'            => 'Tapasse op selektearre bewurking',
'revdelete-logentry'          => 'sichtberens fan bewurkings is feroare foar [[$1]]',
'logdelete-logentry'          => 'feroare sichtberens fan barren [[$1]]',
'revdelete-success'           => "'''Sichtberens fan'e feroaring mei sukses ynsteld.'''",
'logdelete-success'           => "'''Sichtberens fan it barren mei sukses ynsteld.'''",
'revdel-restore'              => 'Sichtberens feroarje',
'pagehist'                    => 'Sideskiednis',
'deletedhist'                 => 'Wiske skiednis',
'revdelete-content'           => 'ynhâld',
'revdelete-summary'           => 'gearfetting bewurkje',
'revdelete-uname'             => 'meidoggernamme',
'revdelete-restricted'        => 'hat beheinings oplein oan behearders',
'revdelete-unrestricted'      => 'hat beheinings foar behearders goedmakke',
'revdelete-hid'               => 'hat $1 ferburgen',
'revdelete-unhid'             => '$1 net mear ferburgen',
'revdelete-log-message'       => '$1 foar $2 {{PLURAL:$2|ferzje|ferzjes}}',
'logdelete-log-message'       => '$1 foar $2 {{PLURAL:$2|lochboekregel|lochboekregels}}',

# Suppression log
'suppressionlog'     => 'Ferskûl-logboek',
'suppressionlogtext' => "Hjir ûnder stiet in list fan fuorthellings en blokkades dy't foar behearders ferskûle binne. Sjoch de [[Special:IPBlockList|IP block list]] foar de blokkades op dit stuit.",

# History merging
'mergehistory'                     => 'Skiednis fan kombinearjen',
'mergehistory-header'              => "Troch dizze side kinne jo ferzjes fan 'e skiednis fan in boarneside nei in nijere side gearfoegje.
Soargje dat jo mei dizze feroaring it skiednisferrin fan de side net oantaaste.",
'mergehistory-box'                 => 'Ferzjes fan twa siden kombinearje:',
'mergehistory-from'                => 'Triemnamme boarne:',
'mergehistory-into'                => 'Bestimmingside:',
'mergehistory-list'                => 'Gearfoegbere bewurkingsskiednis',
'mergehistory-merge'               => "De folgjende ferzjes fan [[:$1]] kinne gearfoege wurde nei [[:$2]].
Brûk de kolom mei de karrûntsjes om allinne de ferzjes makke op en foar de oanjûne tiid gear te foegjen.
Tink derom it brûken fan de navigaasjeferwizings dy kolom op'e nij ynstelt.",
'mergehistory-go'                  => "Besjen bewurkings dy't kombinearre wurde kinne",
'mergehistory-submit'              => 'Kombinearje ferzjes',
'mergehistory-empty'               => 'Gjin ferzjes kinne kombinearren wurde.',
'mergehistory-success'             => 'Kombinearjen slagge fan $3 {{PLURAL:$3|ferzje|ferzjes}} fan [[:$1]] no [[:$2]].',
'mergehistory-fail'                => 'It is net mooglik de skiednis te kombinearje; kontrolearje nochris de side en tiidparameters.',
'mergehistory-no-source'           => 'Boarneside $1 bestiet net.',
'mergehistory-no-destination'      => 'Doelside $1 bestiet net.',
'mergehistory-invalid-source'      => 'De titel fan de boarneside moet jildich wêze.',
'mergehistory-invalid-destination' => 'De titel fan de doelside moet jildich wêze.',
'mergehistory-autocomment'         => '[[:$1]] kombinearre mei [[:$2]]',
'mergehistory-comment'             => '[[:$1]] kombinearre mei [[:$2]]: $3',

# Merge log
'mergelog'           => 'Gearfoegingslogboek',
'pagemerge-logentry' => '[[$1]] kombinearre mei [[$2]] (maksimaal $3 ferzjes)',
'revertmerge'        => 'Gearfoeging ûngedien meitsje',
'mergelogpagetext'   => 'Hjirûnder stiet in list fan resinte gearfoegings fan ien side-skiednis nei in oaren.',

# Diffs
'history-title'           => 'Sideskiednis fan "$1"',
'difference'              => '(Ferskil tusken ferzjes)',
'lineno'                  => 'Rigel $1:',
'compareselectedversions' => 'Ferlykje selektearre ferzjes',
'editundo'                => 'werom sette',
'diff-multi'              => '({{PLURAL:$1|Ien tuskenlizzende ferzje wurdt|$1 tuskenlizzende ferzjes wurde}} net sjen litten.)',

# Search results
'searchresults'             => 'Sykresultaat',
'searchresulttext'          => 'Lês foar mear ynformaasje oer it sykjen yn de {{SITENAME}} de [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Foar fraach "[[:$1]]"',
'searchsubtitleinvalid'     => 'Foar fraach "$1"',
'noexactmatch'              => "'''Der is gjin side mei krekt de titel \"\$1\".'''
Jo kinne de [[:\$1|side oanmeitsje]].",
'noexactmatch-nocreate'     => "'''Der is gjin side mei krekt de titel \"\$1\".'''",
'toomanymatches'            => 'Der wiene tefolle resultaten.
Prebearje in oare sykopdracht.',
'titlematches'              => 'Titels',
'notitlematches'            => 'Gjin titels',
'textmatches'               => 'Siden',
'notextmatches'             => 'Gjin siden',
'prevn'                     => 'foarige $1',
'nextn'                     => 'folgende $1',
'viewprevnext'              => '($1) ($2) ($3) besjen.',
'search-result-size'        => '$1 ({{PLURAL:$2|1 wurd|$2 wurden}})',
'search-result-score'       => 'Relevante: $1%',
'search-redirect'           => '(trochferwizing $1)',
'search-section'            => '(seksje $1)',
'search-suggest'            => 'Bedoele jo: $1',
'search-interwiki-caption'  => 'Susterprojekten',
'search-interwiki-default'  => '$1 resultaten:',
'search-interwiki-more'     => '(mear)',
'search-mwsuggest-enabled'  => 'mei suggestjes',
'search-mwsuggest-disabled' => 'gjin suggestjes',
'search-relatedarticle'     => 'Besibbe',
'mwsuggest-disable'         => 'Suggestjes fia AJAX útskeakelje',
'searchrelated'             => 'besibbe',
'searchall'                 => 'alle',
'showingresults'            => "{{PLURAL:$1|'''1''' resultaat|'''$1''' resultaten}} fan #'''$2''' ôf.",
'showingresultsnum'         => "{{PLURAL:$3|'''1''' resultaat|'''$3''' resultaten}} fan #'''$2''' ôf.",
'showingresultstotal'       => "It binne hjirûnder  {{PLURAL:$3|resultaat '''$1''' fan '''$3'''|resultaten '''$1 - $2''' fan '''$3'''}}",
'nonefound'                 => 'As der gjin resultaten binne, tink der dan om dat der <b>net</b> socht
wurde kin om wurden as "it" en "in", om\'t dy net byhâlden wurde, en dat as der mear
wurden syke wurde, allinnich siden fûn wurde wêr\'t <b>alle</b> worden op fûn wurde.',
'powersearch'               => 'Sykje',
'powersearch-legend'        => 'Sykje',
'powersearch-ns'            => 'Sykje op nammeromten:',
'powersearch-redir'         => 'Trochferwizings werjaan',
'powersearch-field'         => 'Sykje op',
'search-external'           => 'Ekstern sykjen',
'searchdisabled'            => '<p>Op it stuit stiet it trochsykjen fan tekst út omdat dizze funksje tefolle kompjûterkapasiteit ferget. As we nije apparatuer krije, en dy is ûnderweis, dan wurdt dizze funksje wer aktyf. Oant salang kinne jo sykje fia Google:</p>',

# Preferences page
'preferences'              => 'Ynstellings',
'mypreferences'            => 'Myn foarkarynstellings',
'prefs-edits'              => 'Tal bewurkings:',
'prefsnologin'             => 'Net oanmeld',
'prefsnologintext'         => 'Jo moatte <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} oanmeld]</span> wêze om jo foarkar-ynstellings te feroarje te kinnen.',
'prefsreset'               => "De ynstellings binne werom set sa't se fêstlein wienen.",
'qbsettings'               => 'Menu',
'qbsettings-none'          => 'Ut',
'qbsettings-fixedleft'     => 'Lofts fêst',
'qbsettings-fixedright'    => 'Rjochts fêst',
'qbsettings-floatingleft'  => 'Lofts sweevjend',
'qbsettings-floatingright' => 'Rjochts sweevjend',
'changepassword'           => 'Wachtwurd feroarje',
'skin'                     => 'Side-oansjen',
'math'                     => 'Formules',
'dateformat'               => 'Datum',
'datedefault'              => 'Gjin foarkar',
'datetime'                 => 'Datum en tiid',
'math_failure'             => 'Untsjutbere formule',
'math_unknown_error'       => 'Unbekinde fout',
'math_unknown_function'    => 'Unbekinde funksje',
'math_lexing_error'        => 'Unbekind wurd',
'math_syntax_error'        => 'Sinboufout',
'math_image_error'         => 'PNG-omsetting is mislearre. 
Gean nei oft latex, dvips, en gs goed ynstallearre binne en set om',
'math_bad_tmpdir'          => 'De tydlike formulepad kin net skreaun of makke wêze.',
'math_bad_output'          => 'De formulepad kin net skreaun of makke wêze.',
'math_notexvc'             => 'It programma texvc net fûn; sjoch math/README te ynstallearjen.',
'prefs-personal'           => 'Persoanlike gegevens',
'prefs-rc'                 => 'Koartlyn feroare',
'prefs-watchlist'          => 'Folchlist',
'prefs-watchlist-days'     => 'Oantal dagen yn folchlist sjen litte:',
'prefs-watchlist-edits'    => 'Tal wizigings om sjen te litten yn de útwreide folchlist:',
'prefs-misc'               => 'Ferskaat',
'saveprefs'                => 'Ynstellings fêstlizze',
'resetprefs'               => 'Ynstellings werom sette',
'oldpassword'              => 'Ald wachtwurd',
'newpassword'              => 'Nij wachtwurd',
'retypenew'                => 'Nij wachtwurd (nochris)',
'textboxsize'              => 'Siden bewurkje',
'rows'                     => 'Rigen',
'columns'                  => 'Kolommen',
'searchresultshead'        => 'Sykje',
'resultsperpage'           => 'Treffers de side',
'contextlines'             => 'Rigels ynhâld de treffer:',
'contextchars'             => 'Tekens fan de ynhâld de rigel:',
'stub-threshold'           => 'Drompel foar markearring <a href="#" class="stub">stobbe</a> (bytes):',
'recentchangesdays'        => 'Dagen om sjen te litten yn Koartlyn feroare:',
'recentchangescount'       => "Tal titels op 'Koartlyn feroare'",
'savedprefs'               => 'Jo ynstellings binne fêstlein.',
'timezonelegend'           => 'Tiidsône',
'timezonetext'             => 'Jou it tal oeren dat jo tiidsône ferskilt fan UTC (Greenwich).',
'localtime'                => 'Jo tiidsône',
'timezoneoffset'           => 'Ferskil',
'servertime'               => 'UTC',
'guesstimezone'            => 'Freegje de blêder',
'allowemail'               => 'Lêt my ek e-mail fan oare meidoggers ûntfange',
'prefs-searchoptions'      => 'Sykje-ynstellings',
'prefs-namespaces'         => 'Nammeromten',
'defaultns'                => "Nammeromten dy't normaal trochsocht wurde:",
'default'                  => 'standert',
'files'                    => 'Triemen',

# User rights
'userrights'                  => 'Behear fan meidoggerrjochten', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Behear fan meidoggerrjochten',
'userrights-user-editname'    => 'Meidoggernamme:',
'editusergroup'               => 'Wizigje meidoggerrjochten',
'editinguser'                 => "Bewurkje meidoggerrjochten fan '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Wizigje meidoggerrjochten',
'saveusergroups'              => 'Meidoggerrjochten fêstlizze',
'userrights-groupsmember'     => 'Sit yn group:',
'userrights-groups-help'      => "Jo kinne de groepen feroarje dêr't dizze brûker lid fan is. 
* In oankrúst fekje betsjut dat de brûker lid is fan 'e groep.
* In net oankrúst fekje betsjut dat de brûker gjin lid is fan 'e groep.
* In \"*\" betsjut dat jo in brûker net út in groep weihelje kinne nei't jo dy tafoege hawwe, of oarsom.",
'userrights-reason'           => 'Reden foar wizigjen:',
'userrights-no-interwiki'     => 'Jo hawwe gjin foech om rjochten fan meidoggers op oare wikis te wizigjen.',
'userrights-nodatabase'       => 'Databank $1 bestiet net of is net lokaal.',
'userrights-nologin'          => 'Jo moatte jo [[Special:UserLogin|oanmelde]] as rjochtenútfurder om rjochten fan meidoggers wizigje te kinnen.',
'userrights-notallowed'       => 'Jo hawwe gjin rjochten om rjochten fan meidoggers te wizigjen.',
'userrights-changeable-col'   => "Groepen dy't jo beheare kinne",
'userrights-unchangeable-col' => "Groepen dy't jo net beheare kinne",

# Groups
'group'               => 'Groep:',
'group-user'          => 'Meidoggers',
'group-autoconfirmed' => 'befêstige brûkers',
'group-bot'           => 'Bots',
'group-sysop'         => 'Behearders',
'group-bureaucrat'    => 'Rjochtenútfurders',
'group-suppress'      => 'tasichthâlders',
'group-all'           => '(eltsenien)',

'group-user-member'          => 'Meidogger',
'group-autoconfirmed-member' => 'Registrearre brûker',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Behearder',
'group-bureaucrat-member'    => 'Rjochtenútfurder',
'group-suppress-member'      => 'Tasichthâlder',

'grouppage-user'          => '{{ns:project}}:Meidoggers',
'grouppage-autoconfirmed' => '{{ns:project}}:Registrearre brûkers',
'grouppage-bot'           => '{{ns:project}}:Bots',
'grouppage-sysop'         => '{{ns:project}}:Behearders',
'grouppage-bureaucrat'    => '{{ns:project}}:Rjochtenútfurders',
'grouppage-suppress'      => '{{ns:project}}:Tafersjoch',

# Rights
'right-read'                 => 'Siden sjen',
'right-edit'                 => 'Siden bewurkjen',
'right-createpage'           => 'Siden oanmeitsjen (net oerlissiden)',
'right-createtalk'           => 'Oerlissiden oanmeitsjen',
'right-createaccount'        => 'Nije brûkers oanmeitsje',
'right-minoredit'            => 'Bydragen markearje as tekstwiziging',
'right-move'                 => 'Siden werneamen',
'right-move-subpages'        => 'Siden ynklusyf har subsiden ferpleatse.',
'right-suppressredirect'     => "In trochferwizing op 'e doelside fuorthelje by werneamen fan in side",
'right-upload'               => 'Triemmen oanbieden',
'right-reupload'             => 'In besteande triem oerskriuwen',
'right-reupload-own'         => 'Sels heechladene triemmen oerskriuwe',
'right-reupload-shared'      => "Media út 'e dielde mediadatabank lokaal oerskriuwe",
'right-upload_by_url'        => 'Triemen oanbieden fia in URL',
'right-purge'                => 'De cache foar in side leegje sûnder befêstiging',
'right-autoconfirmed'        => 'Behannele wurde as in registrearre brûker',
'right-bot'                  => 'Behannele wurde as in automatisearre proses',
'right-nominornewtalk'       => "Lytse bewurkings oan in oerlisside liede net ta in melding 'nije berjochten'",
'right-apihighlimits'        => 'Hegere limiten yn API-sykopdrachten brûke',
'right-writeapi'             => 'Bewurkje fia API',
'right-delete'               => 'Siden wiskje',
'right-bigdelete'            => 'Wiskje siden mei grutte skiednis',
'right-deleterevision'       => 'Spesifisearre ferzjes fan siden wiskje',
'right-deletedhistory'       => 'Wiske ferzjes besjen, sûnder sjen te kinnen wat wiske is.',
'right-browsearchive'        => 'Wiske siden besjen',
'right-undelete'             => 'Wiske siden tebeksette',
'right-suppressrevision'     => 'Ferskûle ferzjes besjen en tebeksette',
'right-suppressionlog'       => 'Net-publike logboeken besjen',
'right-block'                => 'Oare brûkers de mûglikheid ta bewurkjen ôfnimme',
'right-blockemail'           => 'In brûker it rjocht ta it ferstjoeren fan e-mail ôfnimme',
'right-hideuser'             => 'In brûkersnamme foar oare brûkers ferskûlje',
'right-ipblock-exempt'       => "IP-blokkades út 'e wei gean",
'right-proxyunbannable'      => "Blokkades foar proxy's jilde net",
'right-protect'              => "Befeiligingsnivo's feroarje en beskerme siden bewurkje",
'right-editprotected'        => 'Befeilige siden bewurkje (sûnder cascading-befeiliging)',
'right-editinterface'        => 'Brûkersinterface bewurkje',
'right-editusercssjs'        => 'De CSS- en JS-triemmen fan oare brûkers bewurkje',
'right-rollback'             => 'Gau de lêste bewurking(s) fan in brûker fan in side tebekdraaie',
'right-markbotedits'         => 'Tebekdraaide bewurkings markearje as botbewurkings',
'right-noratelimit'          => 'Hat gjin tiidsôfhinklike beheinings',
'right-import'               => "Siden út oare wiki's ymportearje",
'right-importupload'         => 'Ymportearjen siden fan in triemoanbied',
'right-patrol'               => 'Bewurkings as kontrolearre markearje',
'right-autopatrol'           => 'Bewurkings wurde automatysk as kontrolearre markearre',
'right-patrolmarks'          => 'Kontroletekens yn resinte feroarings besjen',
'right-unwatchedpages'       => "In list mei siden besjen dy't net op in folchlist steane",
'right-trackback'            => 'In trackback opjaan',
'right-mergehistory'         => 'De skiednis fan siden gearfoegje',
'right-userrights'           => 'Alle meidoggerrjochten bywurkje',
'right-userrights-interwiki' => "Wizigje meidoggerrjochten fan meidoggers yn oare wiki's",
'right-siteadmin'            => 'De database blokkearje en wer frij jaan',

# User rights log
'rightslog'      => 'Rjochten-loch',
'rightslogtext'  => 'Dit is in loch fan feroarings fan meidoggerrjochten.',
'rightslogentry' => 'groep foar $1 feroare fan $2 yn $3',
'rightsnone'     => '(gjin)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|bewurking|bewurkings}}',
'recentchanges'                     => 'Koartlyn feroare',
'recentchangestext'                 => 'De lêste feroarings fan de {{SITENAME}}.',
'recentchanges-feed-description'    => 'Mei dizze feed kinne jo de nijste feroarings yn dizze wiki besjen.',
'rcnote'                            => "Dit {{PLURAL:$1|is de lêste feroaring|binne de lêste '''$1''' feroarings}} yn de lêste {{PLURAL:$2|dei|'''$2''' dagen}}, fan $4 $5.",
'rcnotefrom'                        => 'Dit binne de feroarings sûnt <b>$2</b> (maksimaal <b>$1</b>).',
'rclistfrom'                        => 'Jou nije feroarings, begjinnende mei $1',
'rcshowhideminor'                   => '$1 tekstwizigings',
'rcshowhidebots'                    => 'bots $1',
'rcshowhideliu'                     => '$1 meidoggers',
'rcshowhideanons'                   => '$1 anonimen',
'rcshowhidepatr'                    => 'kontrolearre bewurkings $1',
'rcshowhidemine'                    => '$1 eigen bewurkings',
'rclinks'                           => 'Jou $1 nije feroarings yn de lêste $2 dagen<br /> $3',
'diff'                              => 'ferskil',
'hist'                              => 'skiednis',
'hide'                              => 'gjin',
'show'                              => 'al',
'minoreditletter'                   => 'T',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 folgjende {{PLURAL:$1|meidogger|meidoggers}}]',
'rc_categories'                     => 'Alline kategoryen (skiede mei in "|")',
'rc_categories_any'                 => 'Elk',
'newsectionsummary'                 => '/* $1 */ nije seksje',

# Recent changes linked
'recentchangeslinked'          => 'Folgje keppelings',
'recentchangeslinked-title'    => 'Feroarings yn ferbân mei "$1"',
'recentchangeslinked-noresult' => "Der hawwe gjin bewurkings yn 'e bedoelde perioade west op'e siden dy't hjirwei linke wurde.",
'recentchangeslinked-summary'  => "Dizze spesjale side lit de lêste bewurkings sjen op siden dy't keppele wurde fan in spesifisearre side ôf (of fan in spesifisearre Kategory ôf). Siden dy't op [[Special:Watchlist|jo folchlist]] steane, wurde '''tsjûk''' werjûn.",
'recentchangeslinked-page'     => 'Sidenamme:',
'recentchangeslinked-to'       => 'Feroarings oan siden mei ferwizings nei dizze side besjen',

# Upload
'upload'                      => 'Bied triem oan',
'uploadbtn'                   => 'Bied triem oan',
'reupload'                    => "Op 'e nij oanbiede",
'reuploaddesc'                => 'Werom nei oanbied-side.',
'uploadnologin'               => 'Net oanmelde',
'uploadnologintext'           => 'Jo moatte [[Special:UserLogin|oanmeld]] wêze om in triem oanbiede te kinnen.',
'upload_directory_missing'    => 'De heechlaadmap ($1) is der net en koe net oanmakke wurde troch de webserver.',
'upload_directory_read_only'  => 'De webserver kin net skriuwe yn de oanbiedpad ($1).',
'uploaderror'                 => 'Oanbiedfout',
'uploadtext'                  => "Om in nije triemmen oan te bieden, brûke jo de ûndersteande formulier. Earder oanbeane triemmen, kinne jo fine op de [[Special:ImageList|list fan oanbeane ôfbylden]].
Wat oanbean en wat wiske wurdt, wurdt delskreaun yn it [[Special:Log/upload|lochboek]].

Om de triem yn in side op te nimmen, meitsje jo dêr sa'n keppeling:
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:jo_foto.jpg]]</nowiki></tt>''', foar grutte ferzje,
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:jo_logo.png|omskriuwing]]</nowiki></tt>''' foar 200 in piksel ferzje, mei 'alternative tekst' as beskriuwing, of
*'''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:jo_lûd.ogg]]</nowiki></tt>''', foar direkt keppeling nei de triem (sûnder byld).",
'upload-permitted'            => 'Talitten triemtypen: $1.',
'upload-preferred'            => 'Oanwiisde triemtypen: $1.',
'upload-prohibited'           => 'Ferbeane triemtypen: $1.',
'uploadlog'                   => 'oanbiedloch',
'uploadlogpage'               => 'Oanbiedloch',
'uploadlogpagetext'           => 'List fan de lêst oanbeane triemmen. (Tiid oanjûn as UTC).',
'filename'                    => 'Triemnamme',
'filedesc'                    => 'Omskriuwing',
'fileuploadsummary'           => 'Gearfetting:',
'filestatus'                  => 'Auteursrjochtensituaasje:',
'filesource'                  => 'Boarne:',
'uploadedfiles'               => 'Oanbeane triemmen',
'ignorewarning'               => 'Negearje de warskôging en lis triem dochs fêst.',
'ignorewarnings'              => 'Negearje warskôgings',
'minlength1'                  => 'Triemnammen moatte minstens út ien teken bestean.',
'illegalfilename'             => 'De triemnamme "$1" befettet ûnjildige tekens.
Jou de triem in oare namme en besykje him dan op\'e nij heech te laden.',
'badfilename'                 => 'De ôfbyldnamme is feroare nei "$1".',
'filetype-badmime'            => 'Triemmen fan it MIME type "$1" meie net heechladen wurde.',
'filetype-unwanted-type'      => "'''\".\$1\"''' is in net winske triem-type.
{{PLURAL:\$3|Oanwiisd triem-type is|Oanwiisde triem-typen binne}} \$2.",
'filetype-banned-type'        => "'''\".\$1\"''' is gjin talitten triem-type.
{{PLURAL:\$3|Talitten triem-type is|Talittene triem-typen binne}} \$2.",
'filetype-missing'            => 'De triem hat gjin taheaksel (lykas ".jpg").',
'large-file'                  => 'Oanbefelling: meitsje triemmen net grutter as $1; dizze triem is $2.',
'largefileserver'             => 'De triem is grutter as dat de ynstelling fan de server talit.',
'emptyfile'                   => "De triem dy jo heechladen hawwe liket leech te wêzen.
Dat soe komme kinne fan in typflater yn 'e triemnamme.
Gean nei oft jo dizze triem wier bedoelden heech te laden.",
'fileexists'                  => 'Der bestiet al in triem mei dizze namme.
Kontrolearje <strong><tt>$1</tt></strong> as jo net wis binne oft jo de besteande triem oerskriuwe wolle.',
'filepageexists'              => "De beskriuwingsside foar dizze triem bestiet al op <strong><tt>$1</tt></strong>, mar der bestiet gjin triem mei dizze namme.
De gearfetting dy't jo opjûn hawwe sil net op 'e beskriuwingsside ferskine.
Bewurkje de side mei de hân om de beskriuwing dêr wer te jaan.",
'fileexists-extension'        => "In triem mei deselde namme bestiet al:<br />
Namme fan 'e heechladene triem: <strong><tt>$1</tt></strong><br />
Namme fan 'e besteande triem: <strong><tt>$2</tt></strong><br />
Kies in oare namme.",
'fileexists-thumb'            => "<center>'''Besteande ôfbylding'''</center>",
'fileexists-thumbnail-yes'    => 'De triem liket in ferlytse ferzje te wêzen <i>(miniatuerôfbylding)</i>.
Kontrolearje de triem <strong><tt>$1</tt></strong>.<br />
As de kontrolearre triem deselde ôfbylding fan deselde grutte is, dan hoecht net in ekstra miniatuerôfbylding oanbean te wurden.',
'file-thumbnail-no'           => 'De triemnamme begjint mei <strong><tt>$1</tt></strong>.
It liket in ferlytse ôfbylding te wêzen <i>(miniatuerôfbylding)</i>.
As jo dy ôfbylding yn folsleine resolúsje hawwe, bied him dan oan.
Feroarje oars de triemnamme.',
'fileexists-forbidden'        => 'Der bestiet al in triem mei dizze namme.
Bied jo triem ûnder in oare namme oan.
[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => 'Der bestiet al in triem mei dizze namme by de dielde triemmen.
As jo de triem dochs noch oanbiede wolle, gean dan werom en kies in oare namme.
[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => 'Dizze triem is idintyk oan {{PLURAL:$1|de folgjende triem|de folgjende triemmen}}:',
'successfulupload'            => 'Oanbieden slagge.',
'uploadwarning'               => 'Oanbied-warskôging',
'savefile'                    => 'Lis triem fêst',
'uploadedimage'               => ' "[[$1]]" oanbean',
'overwroteimage'              => 'hat in nije ferzje fan "[[$1]]" tafoege',
'uploaddisabled'              => 'Sorry, op dizze tsjinner kin net oanbean wurde.',
'uploaddisabledtext'          => 'It oanbieden fan triemmen is útskeakele.',
'uploadscripted'              => "Dizze triem befettet HTML- of scriptkoade dy't ferkeard troch jo browser werjûn wurde kin.",
'uploadcorrupt'               => 'De triem is net ynoarder of hat de ferkearde útgong.  
Kontrolearje de triem en bied dy wer oan.',
'uploadvirus'                 => 'De triem befettet in firus! Details: $1',
'sourcefilename'              => 'Triemnamme boarne:',
'destfilename'                => 'Triemnamme om op te slaan:',
'upload-maxfilesize'          => 'Maksimale triemgrutte: $1',
'watchthisupload'             => 'Folgje dizze side',
'filewasdeleted'              => "Der is earder in triem mei dizze namme fuorthelle.
Rieplachtsje it $1 foar't jo him op'e nij tafoegje.",
'upload-wasdeleted'           => "'''Warskôging: jo binne in triem oan it oanbieden, dy't earder fuorthelle wie.'''

Kontrolearje oft it wier jo bedoeling is de triem oan te bieden.
It fuorthellogboek fan dizze triem kinne jo hjir sjen:",
'filename-bad-prefix'         => 'De namme fan de triem dy\'t jo oanbiede begjint mei <strong>"$1"</strong>, dit wiist op in namme dy\'t automatysk troch in digitale kamera oanmakke wurdt. Feroarje de namme as jo wolle yn ien dy\'t in omskriuwing jout fan de triem.',
'filename-prefix-blacklist'   => " #<!-- lit dizze line exakt sa't er is --> <pre>
# Syntax is as folget:
#   * Alles fan in \"#\"-teken oan't de ein fan de line is in kommintaar
#   * Elke net blanke line is a foarheaksel foar triemnammen sa't dy automatysk jûn wurde troch digitale kamera's
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # guon mobile tillefoanen
IMG # algemien
JD # Jenoptik
MGP # Pentax
PICT # ferskaat
 #</pre> <!-- lit dizze line exakt sa't er is -->",

'upload-proto-error'      => 'Ferkeard protokol',
'upload-proto-error-text' => "Oanbieden mei dizze metoade freget URL's dy't begjinne mei <code>http://</code> of <code>ftp://</code>.",
'upload-file-error'       => 'Ynterne fout',
'upload-file-error-text'  => "Der wie in ynterne fout doe't in tydlike triem op'e server oanmakke waard.
Nim kontakt op mei in [[Special:ListUsers/sysop|systeembehearder]].",
'upload-misc-error'       => 'Unbekende oanbiedfout',
'upload-misc-error-text'  => 'Der is by it oanbieden in ûnbekende fout optreden.
Kontrolearje of de URL krekt en beskikber is en besykje it nochris.
As it probleem oanhâldt, nim dan kontakt op mei in
[[Special:ListUsers/sysop|systeembehearder]].',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Koe de URL net berikke',
'upload-curl-error6-text'  => 'De opjûne URL is net berikber.
Kontrolearje oft de URL krekt is en oft de webside beskikber is.',
'upload-curl-error28'      => 'Oanbiedtiid foarby',
'upload-curl-error28-text' => "It duorre te lang foar't it webstee andere.
Kontrolearje oft it webstee beskikber is, wachtsje efkes en besykje it dan wer.
Jo kinne it faaks besykje as it wat minder drok is.",

'license'            => 'Lisinsje:',
'nolicense'          => 'Neat keazen',
'license-nopreview'  => '(Foarfertoaning net beskikber)',
'upload_source_url'  => ' (in jildige, publyk tagonklike URL)',
'upload_source_file' => ' (in triem op jo kompjûter)',

# Special:ImageList
'imagelist-summary'     => 'Op dizze spesjale side binne alle tafoege triemmen te besjen.
Standert wurde de lêst tafoege triemmen boppe oan de list werjûn.
Klikken op in kolomkop feroaret de sortearring.',
'imagelist_search_for'  => 'Sykje nei triem:',
'imgfile'               => 'triem',
'imagelist'             => 'Ofbyld list',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Namme',
'imagelist_user'        => 'Meidogger',
'imagelist_size'        => 'Grutte',
'imagelist_description' => 'Beskriuwing',

# Image description page
'filehist'                       => 'Triem skiednis',
'filehist-help'                  => 'Klik op in tiid om de ferzje fan de triem op dat stuit te sjen.',
'filehist-deleteall'             => 'wiskje alles',
'filehist-deleteone'             => 'wiskje dizze',
'filehist-revert'                => 'werom sette',
'filehist-current'               => 'lêste',
'filehist-datetime'              => 'Tiid',
'filehist-user'                  => 'Meidogger',
'filehist-dimensions'            => 'Ofmjittings',
'filehist-filesize'              => 'Triem grutte',
'filehist-comment'               => 'Opmerkings',
'imagelinks'                     => 'Ofbyldkeppelings',
'linkstoimage'                   => 'Dizze {{PLURAL:$1|side is|$1 siden binne}} keppele oan it ôfbyld:',
'nolinkstoimage'                 => 'Der binne gjin siden oan dit ôfbyld keppele.',
'morelinkstoimage'               => '[[Special:WhatLinksHere/$1|Mear ferwizings]] nei dizze triem besjen.',
'redirectstofile'                => '{{PLURAL:$1|De folgjende triem ferwiist|De folgjende $1 triemmen ferwize}} troch nei dizze triem:',
'duplicatesoffile'               => '{{PLURAL:$1|De folgjende triem is|De folgjende $1 triemmen binne}} idintyk oan dizze triem:',
'sharedupload'                   => 'Dizze triem is in dielde oanbieding en kin ek troch oare projekten brûkt wurde.',
'shareduploadwiki'               => 'Sjoch de $1 foar fierdere ynformaasje.',
'shareduploadwiki-desc'          => "De $1 yn 'e dielde triemmebank wurdt hjirûnder werjûn.",
'shareduploadwiki-linktext'      => 'triembeskriuwing',
'shareduploadduplicate'          => "Dizze triem is idintyk oan $1 yn 'e dielde mediabank.",
'shareduploadduplicate-linktext' => 'in oare triem',
'shareduploadconflict'           => "Dizze triem hat deselde namme as $1 yn 'e dielde databank.",
'shareduploadconflict-linktext'  => 'in oare triem',
'noimage'                        => 'Der bestiet gjin triem mei dizze namme. Jo kinne it $1.',
'noimage-linktext'               => 'bied triem oan',
'uploadnewversion-linktext'      => 'Bied in nije ferzje fan dizze triem oan',
'imagepage-searchdupe'           => 'Duplikaattriemmen sykje',

# File reversion
'filerevert'                => '$1 weromsette',
'filerevert-legend'         => 'Triem weromsette',
'filerevert-intro'          => "Jo binne '''[[Media:$1|$1]]''' oan it weromdraaien ta de [$4 ferzje op $2, $3].",
'filerevert-comment'        => 'Oanmerking:',
'filerevert-defaultcomment' => 'Weromdraaid ta de ferzje op $1, $2',
'filerevert-submit'         => 'werom sette',
'filerevert-success'        => "'''[[Media:$1|$1]]''' is weromdraaid ta de [$4 ferzje op $2, $3].",
'filerevert-badversion'     => "Der is gjin foarige lokale ferzje fan dizze triem fan 'e opjûne tiid.",

# File deletion
'filedelete'                  => 'Wiskje $1',
'filedelete-legend'           => 'Wiskje triem',
'filedelete-intro'            => "Jo wiskje '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "Jo wiskje de ferzje fan '''[[Media:$1|$1]]''' fan [$4 $3, $2].",
'filedelete-comment'          => 'Reden foar it wiskjen:',
'filedelete-submit'           => 'Wiskje',
'filedelete-success'          => "'''$1''' is wiske.",
'filedelete-success-old'      => "De ferzje fan '''[[Media:$1|$1]]''' fan $2, $3 is fuorthelle.",
'filedelete-nofile'           => "'''$1''' bestiet net.",
'filedelete-nofile-old'       => "Der is gjin opsleine ferzje fan '''$1''' mei de oanjûne eigenskippen.",
'filedelete-otherreason'      => 'Oare/eventuele reden:',
'filedelete-reason-otherlist' => 'Oare reden',
'filedelete-reason-dropdown'  => '*Faak foarkommende redenen foar fuortheljen
** Skeinen fan auteursrjochten
** Duplikaattriem',
'filedelete-edit-reasonlist'  => 'Redenen foar fuortheljen bewurkje',

# MIME search
'mimesearch'         => 'Sykje op MIME-type',
'mimesearch-summary' => 'Dizze side makket it filterjen mûglik fan triemmen foar it MIME-type.
Ynfier: contenttype/subtype, bygelyks <tt>image/jpeg</tt>.',
'mimetype'           => 'MIME-type:',
'download'           => 'oanbiede',

# Unwatched pages
'unwatchedpages' => "Siden dy't net op in folchlist steane",

# List redirects
'listredirects' => 'List fan trochferwizings',

# Unused templates
'unusedtemplates'     => 'Net brûkte sjabloanen',
'unusedtemplatestext' => 'Dizze side jout alle siden wer yn\'e Berjocht-nammeromte dy\'t op net ien side brûkt wurde.
Ferjit net de "Wat is hjirmei keppele" nei te gean foar it fuortheljen fan dit berjocht.',
'unusedtemplateswlh'  => 'oare keppelings',

# Random page
'randompage'         => 'Samar in side',
'randompage-nopages' => 'Der binne gjin siden yn dizze nammeromte.',

# Random redirect
'randomredirect' => 'Samar in trochferwizing',

# Statistics
'statistics'             => 'Statistyk',
'sitestats'              => 'Side statistyk',
'userstats'              => 'Meidogger-statistyk',
'sitestatstext'          => "It {{PLURAL:$1|is '''1''' side|binne '''$1''' siden}} yn de databank.
Oerlissiden, siden oer de {{SITENAME}}, stobben, trochferwizings, en oare bysûndere siden, binne dêrby meiteld. Sûnder dizze siden, it {{PLURAL:$2|is '''1''' side|binne '''$2''' siden}} mei materiaal en ynhâld. 

'''$8''' {{PLURAL:$8|triem wurdt|triemmen wurde}} al oanbied.

Der {{PLURAL:$3|is '''1''' sidelêzing|binne '''$3''' sidelêzings}}, en '''$4''' {{PLURAL:$4|bewurking|bewurkings}} sûnt {{SITENAME}} begûnen. Dat komt yn trochslach del op '''$5''' bewurkings per side, en '''$6''' lêzings per bewurking.  

De lingte fan de [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] is '''$7'''.",
'userstatstext'          => "It tal fan registrearre meidoggers is '''{{PLURAL:$1|1|$1}}'''.
It tal fan meidoggers dêrfan mei $5rjochten is '''{{PLURAL:$2|1|$2}}''' (of '''{{PLURAL:$4|1|$4}}''').",
'statistics-mostpopular' => 'Meast besjoene siden',

'disambiguations'      => 'Betsjuttingssiden',
'disambiguationspage'  => 'Template:Neibetsjuttings',
'disambiguations-text' => "De ûndersteande siden keppelje mei in '''Betsjuttingssiden'''.
Se soenen mei de side sels keppele wurde moatte.<br /> In side wurdt sjoen as betsjuttingssiden, as de side ien berjocht fan [[MediaWiki:Disambiguationspage]] brûkt.",

'doubleredirects'     => 'Dûbelde synonimen',
'doubleredirectstext' => '<b>Let op!</b> Der kinne missen yn dizze list stean! Dat komt dan ornaris troch oare keppelings ûnder de "#REDIRECT". Eltse rigel jout keppelings nei it earste synonym, it twadde synonym en dan it werklike doel.',

'brokenredirects'        => 'Misse synonimen',
'brokenredirectstext'    => "De siden dêr't dizze titels synonym oan wêze moatte, bestean net.",
'brokenredirects-edit'   => '(bewurkje)',
'brokenredirects-delete' => '(wiskje)',

'withoutinterwiki'        => 'Siden sûnder ferwizings nei oare talen',
'withoutinterwiki-submit' => 'Sjen litte',

'fewestrevisions' => 'Siden mei de minste ferzjes',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 {{PLURAL:$1|kategory|kategoryen}}',
'nlinks'                  => '$1 {{PLURAL:$1|keppeling|keppelings}}',
'nmembers'                => '$1 {{PLURAL:$1|lid|lea}}',
'nrevisions'              => '$1 {{PLURAL:$1|ferzje|ferzjes}}',
'nviews'                  => '{{PLURAL:$1|1 kear|$1 kear}} sjoen',
'specialpage-empty'       => 'Gjin resultaten foar dit rapport.',
'lonelypages'             => 'Lossteande siden',
'uncategorizedpages'      => 'Siden sûnder kategory',
'uncategorizedcategories' => 'Kategoryen sûnder kategory',
'uncategorizedimages'     => 'Ofbylden sûnder kategory',
'uncategorizedtemplates'  => 'Net-kategorisearre sjabloanen',
'unusedcategories'        => 'Lege kategoryen',
'unusedimages'            => 'Lossteande ôfbylden',
'popularpages'            => 'Populêre siden',
'wantedcategories'        => 'Nedige kategoryen',
'wantedpages'             => 'Nedige siden',
'mostlinked'              => 'Siden wêr it meast mei keppele is',
'mostlinkedcategories'    => "Kategoryen dy't it meast brûkt wurde",
'mostlinkedtemplates'     => 'Meast brûkte sjabloanen',
'mostcategories'          => 'Siden mei de measte kategoryen',
'mostimages'              => "Ofbylden dy't it meast brûkt wurde",
'mostrevisions'           => 'Siden mei de measte bewurkings',
'prefixindex'             => 'Alle siden neffens foarheaksel',
'shortpages'              => 'Koarte siden',
'longpages'               => 'Lange siden',
'deadendpages'            => 'Siden sûnder ferwizings',
'protectedpages'          => 'Skoattele siden',
'protectedpagestext'      => 'De neikommende siden binne skoattele foar werneamen of wizigjen',
'protectedpagesempty'     => "Op it stuit binne der gjin siden befeilige, dy't oan dizze betingsten foldogge.",
'protectedtitles'         => 'Skoattele titels',
'protectedtitlesempty'    => "Der binne op it stuit gjin sidenammen befeilige, dy't oan dizze betingsten foldogge.",
'listusers'               => 'Meidoggerlist',
'newpages'                => 'Nije siden',
'newpages-username'       => 'Meidoggernamme:',
'ancientpages'            => 'Alde siden',
'move'                    => 'Werneam',
'movethispage'            => 'Werneam dizze side',
'unusedimagestext'        => '<p>Tink derom dat oare websiden fan oare parten fan it meartalige projekt mooglik in keppeling nei it URL fan it ôfbyld makke hawwe. Sokke ôfbylden wurde wol brûkt, mar steane dochs op dizze list.',
'notargettitle'           => 'Gjin side',
'notargettext'            => 'Jo hawwe net sein oer hokfoar side jo dit witte wolle.',
'nopagetitle'             => 'Side bestiet net',
'pager-older-n'           => '{{PLURAL:$1|1 âlder|$1 âlder}}',

# Book sources
'booksources'    => 'Boekynformaasje',
'booksources-go' => 'Sykje',

# Special:Log
'specialloguserlabel'  => 'Meidogger:',
'speciallogtitlelabel' => 'Sidenamme:',
'log'                  => 'Lochs',
'all-logs-page'        => 'Alle lochboeken',
'log-search-submit'    => 'Sykje',
'alllogstext'          => 'Kombinearre loch de {{SITENAME}}.
Jo kinne it oersjoch beheine troch in loch, in meidoggernamme of in side oan te jaan.',
'logempty'             => 'Gjin treffers yn it loch.',

# Special:AllPages
'allpages'          => 'Alle siden',
'alphaindexline'    => '$1 oant $2',
'nextpage'          => 'Folgjende side ($1)',
'prevpage'          => 'Foargeande side ($1)',
'allpagesfrom'      => 'Begjin list by',
'allarticles'       => 'Alle siden',
'allinnamespace'    => 'Alle siden, yn de ($1-nammeromte)',
'allnotinnamespace' => 'Alle siden, útsein de $1-nammeromte',
'allpagesprev'      => 'Eardere',
'allpagesnext'      => 'Fierder',
'allpagessubmit'    => 'Los!',
'allpagesprefix'    => "Siden sjen litte dy't begjinne mei:",

# Special:Categories
'categories'         => 'Kategoryen',
'categoriespagetext' => 'Dizze wiki hat de neikommende kategoryen:',

# Special:ListUsers
'listusersfrom'    => 'Lit meidoggers sjen fanôf:',
'listusers-submit' => 'Sjen litte',

# Special:ListGroupRights
'listgrouprights-group'  => 'Groep',
'listgrouprights-rights' => 'Rjochten',

# E-mail user
'mailnologin'     => 'Gjin adres beskikber',
'mailnologintext' => 'Jo moatte [[Special:UserLogin|oanmelden]] wêze, en in jildich e-postadres [[Special:Preferences|ynsteld]] hawwe, om oan oare meidoggers e-post stjoere te kinnen.',
'emailuser'       => 'Skriuw meidogger',
'emailpage'       => 'E-post nei meidogger',
'emailpagetext'   => "As dizze meidogger in jildich e-postadres ynsteld hat, dan kinne jo in berjocht ferstjoere. It e-postadres dat jo ynsteld hawwe wurdt brûkt as de ôfstjoerder, sa't de ûntfanger antwurdzje kin.",
'defemailsubject' => 'E-post fan {{SITENAME}}',
'noemailtitle'    => 'Gjin e-postadres',
'noemailtext'     => 'Dizze meidogger hat gjin jildich e-postadres ynsteld, of hat oanjûn gjin post fan oare meidoggers krije te wollen.',
'emailfrom'       => 'Fan:',
'emailto'         => 'Oan:',
'emailsubject'    => 'Oer',
'emailmessage'    => 'Tekst',
'emailsend'       => 'Stjoer',
'emailsent'       => 'Berjocht stjoerd',
'emailsenttext'   => 'Jo berjocht is stjoerd.',

# Watchlist
'watchlist'            => 'Folchlist',
'mywatchlist'          => 'Folchlist',
'watchlistfor'         => "(foar '''$1''')",
'nowatchlist'          => 'Jo hawwe gjin siden op jo folchlist.',
'watchnologin'         => 'Net oanmeld yn',
'watchnologintext'     => 'Jo moatte [[Special:UserLogin|oanmeld]] wêze om jo folchlist te feroarjen.',
'addedwatch'           => 'Oan folchlist tafoege',
'addedwatchtext'       => "De side \"'''[[:\$1]]'''\" is tafoege oan jo [[Special:Watchlist|folchlist]]. Bewurkings fan dizze side en oerlisside wurde yn de takomst op jo folchlist oanjûn. Hja wurde foar jo ek '''fet''' printe op [[Special:RecentChanges|Koartlyn feroare]].

At jo letter in side net mear folgje wolle, dan brûke jo op dy side de keppeling \"Ferjit dizze side.\" Jo [[Special:Watchlist|folchlist]] hat ek in keppeling \"Jo folchlist bewurkje,\" foar at jo mear as ien side \"ferjitte\" wolle.",
'removedwatch'         => 'Net mear folgje',
'removedwatchtext'     => 'De side "<nowiki>$1</nowiki>" stiet net mear op jo folchlist.',
'watch'                => 'Folgje',
'watchthispage'        => 'Folgje dizze side',
'unwatch'              => 'Ferjit',
'unwatchthispage'      => 'Ferjit dizze side',
'notanarticle'         => 'Dit kin net folge wurde.',
'watchnochange'        => "Fan de siden dy't jo folgje is der yn dizze perioade net ien feroare.",
'watchlist-details'    => 'Jo folchlist hat {{PLURAL:$1|$1 side|$1 siden}}, oerlissiden net meiteld.',
'watchmethod-recent'   => 'Koartlyn feroare ...',
'watchmethod-list'     => 'Folge ...',
'watchlistcontains'    => 'Jo folgje op it stuit $1 {{PLURAL:$1|side|siden}}.',
'iteminvalidname'      => 'Misse namme: "$1" ...',
'wlnote'               => "Dit {{PLURAL:$1|is de lêste feroaring|binne de lêste '''$1''' feroarings}} yn de lêste {{PLURAL:$2|oer|'''$2''' oeren}}.",
'wlshowlast'           => 'Lit feroarings sjen fan de lêste $1 oeren $2 dagen $3',
'watchlist-hide-bots'  => 'Botbewurkings ferskûlje',
'watchlist-hide-own'   => 'Myn bewurkings ferskûlje',
'watchlist-hide-minor' => 'Lytse bewurkings ferskûlje',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => "Dwaande mei op'e folchlist te setten ...",
'unwatching' => "Dwaande mei fan'e folchlist ôf te heljen ...",

'enotif_newpagetext'           => 'Dit is in nije side.',
'enotif_impersonal_salutation' => 'meidogger fan {{SITENAME}}',
'changed'                      => 'feroare',
'created'                      => 'oanmakke',
'enotif_body'                  => 'Bêste $WATCHINGUSERNAME,

De {{SITENAME}}side \'$PAGETITLE\' is op $PAGEEDITDATE $CHANGEDORCREATED troch meidogger $PAGEEDITOR; 
sjoch $PAGETITLE_URL foar de aktuele ferzje.

$NEWPAGE

Gearfetting: $PAGESUMMARY $PAGEMINOREDIT

Foar oerlis mei meidogger $PAGEEDITOR:
- netpost: $PAGEEDITOR_EMAIL
- wiki: $PAGEEDITOR_WIKI

Fierdere meldings wurde jo net tastjoerd, oant jo de side lêzen hawwe. Op jo folchlist kinne jo op \'e nij meldings foar al jo folge siden freegje.

                 Jo freonlike {{SITENAME}}-meldingssysteem.

-- 
Gean nei {{fullurl:{{ns:special}}:Watchlist/edit}}
om jo folchlistynstellings te feroarjen.

Reaksjes en fierdere help:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Wisk side',
'confirm'                     => 'Befêstigje',
'excontent'                   => "ynhâld wie: '$1'",
'excontentauthor'             => "ynhâld wie: '$1' (en de ienige bewurker wie: '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "foar de tekst wiske wie, wie dat: '$1'",
'exblank'                     => 'side wie leech',
'delete-confirm'              => '"$1" wiskje',
'delete-legend'               => 'Wiskje',
'historywarning'              => "Warskôging: De side dy't jo wiskje wolle hat skiednis:",
'confirmdeletetext'           => 'Jo binne dwaande mei it foar altyd wiskjen fan in side
of ôfbyld, tegearre mei alle skiednis, út de databank.
Befêstigje dat jo dat wier dwaan wolle. Befêstigje dat dat is wat jo witte wat it gefolch
is en dat jo dit dogge neffens de [[{{MediaWiki:Policy-url}}]].',
'actioncomplete'              => 'Dien',
'deletedtext'                 => '"<nowiki>$1</nowiki>" is wiske.
Sjoch "$2" foar in list fan wat resint wiske is.',
'deletedarticle'              => '"[[$1]]" is wiske',
'dellogpage'                  => 'Wiskloch',
'dellogpagetext'              => 'Dit is wat der resint wiske is.
(Tiden oanjûn as UTC).',
'deletionlog'                 => 'wiskloch',
'reverted'                    => 'Weromset nei eardere ferzje',
'deletecomment'               => 'Reden foar it wiskjen',
'deleteotherreason'           => 'Oare/eventuele reden:',
'deletereasonotherlist'       => 'Oare reden',
'deletereason-dropdown'       => '*Faak-brûkte redenen
** Frege troch de skriuwer
** Skeining fan auteursrjocht
** Fandalisme',
'rollback'                    => 'Feroarings werom sette',
'rollback_short'              => 'Werom sette',
'rollbacklink'                => 'feroaring werom sette',
'rollbackfailed'              => 'Feroaring werom sette net slagge',
'cantrollback'                => "Dizze feroaring kin net werom setten wurde, om't der mar ien skriuwer is.",
'alreadyrolled'               => 'Kin de feroaring fan [[:$1]]
troch [[User:$2|$2]] ([[User talk:$2|Oerlis]]) net werom sette;
in oar hat de feroaring werom set, of oars wat oan de side feroare.

De lêste feroaring wie fan [[User:$3|$3]] ([[User talk:$3|Oerlis]]).',
'editcomment'                 => 'De gearfetting wie: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Bewurkings fan [[Special:Contributions/$2|$2]] ([[User talk:$2|Oerlis]]) werom set ta de ferzje fan [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Feroarings werom set fan $1; werom set nei de lêste ferzje fan $2.',
'protectlogpage'              => 'Skoattelloch',
'protectlogtext'              => 'Hjirûnder wurdt it skoateljen en frijjaan fan siden oanjûn. 
Sjoch [[Special:ProtectedPages|Skoattele side]] foar mear ynformaasje.',
'protectedarticle'            => '"[[$1]]" skoattele',
'unprotectedarticle'          => 'joech "[[$1]]" frij',
'protect-legend'              => 'Befeiliging befêstigje',
'protectcomment'              => 'Reden foar it skoatteljen',
'protectexpiry'               => 'Ferrint nei',
'protect_expiry_invalid'      => 'Tiid fan ferrinnen is net jildich.',
'protect_expiry_old'          => 'Tiid fan ferrinnen leit yn it ferline.',
'protect-unchain'             => 'Keppeling foar werneamen frijjaan.',
'protect-text'                => 'Hjir kin jo it nivo fan skoatteljen sjen en oanpasse foar de side <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-access'       => "'''Jo brûker hat gjin rjochten om it befeiligingsnivo te feroarjen.'''
Dit binne de rinnende ynstellings foar de side <strong>$1</strong>:",
'protect-cascadeon'           => "Dizze side is op 't stuit befeilige, om't er yn 'e folgjende {{PLURAL:$1|side|siden}} opnommen is, dy't befeilige {{PLURAL:$1|is|binne}} mei de kaskade-opsje. It befeiligingsnivo feroarje hat alhiel gjin effekt.",
'protect-default'             => '(standert)',
'protect-fallback'            => 'Hjir is it rjocht "$1" foar nedich',
'protect-level-autoconfirmed' => 'Slút anonymen út',
'protect-level-sysop'         => 'Allinnich behearders',
'protect-summary-cascade'     => 'kaskade',
'protect-expiring'            => 'ferrint $1 (UTC)',
'protect-cascade'             => "Underlizzende siden - skoattelje ek alle siden dy't in ûnderdiel útmeitsje fan dizze side",
'protect-cantedit'            => "Jo kinne it befeiligingsnivo fan dizze side net feroarje, om't jo gjin rjochten hawwe om it te bewurkjen.",
'restriction-type'            => 'Permisje:',
'restriction-level'           => 'Skoattel nivo:',
'minimum-size'                => 'Min. grutte',
'maximum-size'                => 'Max. grutte:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => 'Wizigje',
'restriction-move'   => 'Werneam',
'restriction-create' => 'Oanmeitsje',
'restriction-upload' => 'Oanbiede',

# Restriction levels
'restriction-level-sysop'         => 'folslein skoattele',
'restriction-level-autoconfirmed' => 'skoattele foar anonymen',
'restriction-level-all'           => "alle nivo's",

# Undelete
'undelete'                => 'Side werom set',
'undeletepage'            => 'Side besjen en werom sette',
'viewdeletedpage'         => 'Wiske siden besjen',
'undeletepagetext'        => 'Dizze siden binne wiske, mar sitte noch yn it argyf en kinne weromset wurde. (It argyf kin út en troch leechmakke wurde.)',
'undelete-fieldset-title' => 'Ferzjes werom sette',
'undeleteextrahelp'       => "Om in side hielendal werom te setten, lit alle seleksjefakjes iepen en klik '''''Weromsette'''''. Om in bepaalde ferzje werom te setten, klik de fakjes dy't mei de ferzjes oerienkomme, en klik '''''Weromsette'''''. Klik '''''Leechmeitsje''''' om it kommentaar fjild ensafuorthinne leech te meitsjen.",
'undeleterevisions'       => '$1 {{PLURAL:$1|ferzje|ferzjes}} in it argyf',
'undeletehistory'         => 'Soenen jo dizze side weromsette, dan wurde alle ferzjes weromset as part fan de skiednis. As der in nije side is mei dizze namme, dan wurdt de hjoeddeiske ferzje <b>net</b> troch de lêste ferzje út dy weromsette skiednis ferfongen.',
'undelete-revision'       => 'Wiske ferzje fan $1 (op $2) fan $3:',
'undelete-nodiff'         => 'Gjin eardere ferzje fûn.',
'undeletebtn'             => 'Weromsette',
'undeletelink'            => 'werom sette',
'undeletereset'           => 'Leechmeitsje',
'undeletecomment'         => 'Utlis foar weromsetten:',
'undeletedarticle'        => '"$1" weromset',
'undelete-header'         => 'Sjoch [[Special:Log/delete|de wiskloch]] foar resint wiske siden.',
'undelete-search-box'     => 'Sykje wiske siden',
'undelete-search-prefix'  => "Lit siden sjen dy't begjinne mei:",
'undelete-search-submit'  => 'Sykje',
'undelete-no-results'     => 'Gjin oerienkommende siden fûn yn it wisk argyf.',

# Namespace form on various pages
'namespace'      => 'Nammeromte:',
'invert'         => 'Seleksje útsein',
'blanknamespace' => '(Haadnammerûmte)',

# Contributions
'contributions' => 'Meidogger-bydragen',
'mycontris'     => 'Myn bydragen',
'contribsub2'   => 'Foar "$1 ($2)"',
'nocontribs'    => "Der binne gjin feroarings fûn dyt't hjirmei oerienkomme.",
'uctop'         => ' (boppen)',
'month'         => 'Fan moanne (en earder):',
'year'          => 'Fan jier (en earder):',

'sp-contributions-newbies-sub' => 'Foar nijlingen',
'sp-contributions-blocklog'    => 'Blokkearlochboek',
'sp-contributions-search'      => 'Sykje nei bydragen',
'sp-contributions-username'    => 'IP Adres of meidoggernamme:',
'sp-contributions-submit'      => 'Sykje',

# What links here
'whatlinkshere'            => 'Wat is hjirmei keppele?',
'whatlinkshere-title'      => 'Siden dy\'t keppele binne mei "$1"',
'whatlinkshere-page'       => 'Side:',
'linklistsub'              => '(List fan keppelings)',
'linkshere'                => "Dizze siden binne keppele oan '''[[:$1]]''':",
'nolinkshere'              => "Der binne gjin siden oan '''[[:$1]]''' keppele.",
'nolinkshere-ns'           => "Gjin siden yn de keazen nammeromte keppelje nei '''[[:$1]]'''.",
'isredirect'               => 'synonym',
'istemplate'               => 'opnaam',
'isimage'                  => 'byld keppeling',
'whatlinkshere-prev'       => '{{PLURAL:$1|foargeande|foargeande $1}}',
'whatlinkshere-next'       => '{{PLURAL:$1|folgjende|folgjende $1}}',
'whatlinkshere-links'      => '← keppelings',
'whatlinkshere-hideredirs' => '$1 trochferwizings',
'whatlinkshere-hidetrans'  => '$1 trânsklúzjes',
'whatlinkshere-hidelinks'  => '$1 keppelings',

# Block/unblock
'blockip'              => 'Slút meidogger út',
'blockip-legend'       => 'Slút brûker út',
'blockiptext'          => "Brûk dizze fjilden om in meidogger fan skriuwtagong út te sluten.
Dat soe allinnich fanwege fandalisme dien wurde moatte, sa't de
[[{{MediaWiki:Policy-url}}|útslut-rie]] it oanjout.
Meld de krekte reden! Neam bygelyks de siden dy't oantaaste waarden.",
'ipaddress'            => 'Meidochnamme of ynternet-adres',
'ipadressorusername'   => 'IP Adres of meidoggernamme:',
'ipbexpiry'            => 'Ferrint nei:',
'ipbreason'            => 'Reden',
'ipbreasonotherlist'   => 'Oare reden',
'ipbanononly'          => 'Slút allinich anonyme meidoggers út',
'ipbcreateaccount'     => 'Blokkearje it oanmeitsjen fan in nij profyl',
'ipbenableautoblock'   => "Automatysk de lêste IP adressen útslute dy't troch dizze meidogger brûkt binne.",
'ipbsubmit'            => 'Slút dizze meidogger út',
'ipbother'             => 'In oare tiid:',
'ipboptions'           => '15 minuten:15 min,1 oere:1 hour,2 oeren:2 hours,6 oeren:6 hours,12 oeren:12 hours,1 dei:1 day,3 dagen:3 days,1 wike:1 week,2 wiken:2 weeks,1 moanne:1 month,3 moanne:3 months,6 moanne:6 months,1 jier:1 year,ûnbeheind:infinite', # display1:time1,display2:time2,...
'ipbotheroption'       => 'oare tiid',
'ipbotherreason'       => 'Oare/eventuele reden:',
'badipaddress'         => 'Gjin jildige IP-adres',
'blockipsuccesssub'    => 'Utsluting slagge',
'blockipsuccesstext'   => 'Meidogger [[Special:Contributions/$1|$1]] is útsletten.<br />
(List fan [[Special:IPBlockList|útslette meidoggers]].)',
'ipb-unblock-addr'     => 'Lit $1 yn',
'ipb-unblock'          => 'Lit in meidogger of IP-adres yn',
'ipb-blocklist-addr'   => 'Besteande útsluting foar $1 besjen',
'ipb-blocklist'        => 'Besteande útslutings besjen',
'unblockip'            => 'Lit meidogger wer ta',
'unblockiptext'        => 'Brûk dizze fjilden om in meidogger wer skriuwtagong te jaan.',
'ipusubmit'            => 'Lit dizze meidogger wer ta.',
'ipblocklist'          => 'List fan útsletten ynternet-adressen en meidochnammen',
'ipblocklist-username' => 'Meidoggernamme of IP-adres:',
'ipblocklist-submit'   => 'Sykje',
'blocklistline'        => '"$3", troch "$2" op $1 ($4)',
'infiniteblock'        => 'trochgeand',
'blocklink'            => 'slút út',
'unblocklink'          => 'lit yn',
'contribslink'         => 'bydragen',
'autoblocker'          => 'Jo wiene útsletten om\'t jo ynternet-adres oerienkomt mei dat fan "[[User:$1|$1]]". Foar it útsluten fan dy meidogger waard dizze reden jûn: "$2".',
'blocklogpage'         => 'Utslútloch',
'blocklogentry'        => '"[[$1]]" útsletten foar $2 $3',
'blocklogtext'         => 'Dit is in loch fan it útsluten en talitten fan meidoggers. Fansels útsletten net-adressen binne net opnaam. Sjoch de [[Special:IPBlockList|útsletlist]] foar de no jildende utslettings.',
'ipb_expiry_invalid'   => 'Tiid fan ferrinnen is net goed.',
'ipb_already_blocked'  => '"$1" is al útsluten',
'ipb_cant_unblock'     => 'Flater: It útsluten fan ID $1 kin net fûn wurde. It is miskien al net mear útsluten.',
'proxyblocksuccess'    => 'Dien.',

# Developer tools
'lockdb'              => "Meitsje de database 'Net-skriuwe'",
'unlockdb'            => 'Meitsje de databank skriuwber',
'lockdbtext'          => "Wylst de databank skoattele is, is foar meidoggers it feroarjen fan siden, ynstellings, folchlisten, ensfh. net mooglik. 

Befêstigje dat dit is sa't jo it hawwe wolle, en dat jo de databank wer frijjouwe as jo ûnderhâld ree is.",
'unlockdbtext'        => 'As de databank frijjûn wurdt, is foar meidoggers it feroarjen fan siden, ynstellings, folchlisten ensfh. wer mooglik. Befêstigje dat dit is wat jo wolle.',
'lockconfirm'         => "Ja, ik wol wier de databank 'Net--skriuwe' meitsje.",
'unlockconfirm'       => 'Ja, ik wol wier de databank skriuwber meitsje.',
'lockbtn'             => "Meitsje de database 'Net-skriuwe'",
'unlockbtn'           => 'Meitsje de databank skriuwber',
'locknoconfirm'       => 'Jo hawwe jo hanneling net befêstige.',
'lockdbsuccesssub'    => "Databank is 'Net-skriuwe'",
'unlockdbsuccesssub'  => 'Database is skriuwber',
'lockdbsuccesstext'   => "De {{SITENAME}} databank is 'Net-skriuwe' makke.
<br />Tink derom en meitsje de databank skriuwber as jo ûnderhâld ree is.",
'unlockdbsuccesstext' => 'De {{SITENAME}} databank is skriuwber makke.',

# Move page
'move-page'               => 'Werneam  $1',
'move-page-legend'        => 'Werneam side',
'movepagetext'            => "Dit werneamt in side, mei alle sideskiednis.
De âlde titel wurdt in trochferwizing nei de nije.
Keppelings mei de âlde side wurde net feroare;
gean sels nei of't der dûbele of misse ferwizings binne.
It hinget fan jo ôf of't de siden noch keppelen binne sa't it mient wie.

De side wurdt '''net''' werneamt as der al in side mei dy namme is, útsein as it in side
sûnder skiednis is en de side leech is of in trochferwizing is. Sa kinne jo in side
daalks weromneame as jo in flater meitsje, mar jo kinne in oare side net oerskriuwe.",
'movepagetalktext'        => "As der in oerlisside by heart, dan bliuwt dy oan de side keppele, '''útsein''':
*De nije sidenamme yn in oare nammeromte is,
*Der keppele oan de nije namme al in net-lege oerlisside is, of
*Jo dêr net foar kieze.

In dizze gefallen is it oan jo hoe't jo de oerlisside werneame of ynfoegje wolle.",
'movearticle'             => 'Werneam side',
'newtitle'                => 'As nij titel',
'move-watch'              => 'Folch dizze side',
'movepagebtn'             => 'Werneam side',
'pagemovedsub'            => 'Werneamen slagge',
'movepage-moved'          => '<big>\'\'\'"$1" hjit no "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => "Der is al in side mei dy namme, of oars is de namme dy't jo oanjûn hawwe net tastien. Besykje it op 'e nij.",
'talkexists'              => "It werneamen fan de side is slagge, mar de eardere oerlisside is net mear keppele om't der foar de side mei de nije namme al in oerlisside wie. Kopiearje de oerlisside fan de 'âlde' side mei de nije.",
'movedto'                 => 'werneamd as',
'movetalk'                => 'Titel fan oerlisside ek feroarje, as dy der is.',
'movepage-page-moved'     => 'De side $1 is werneamd nei $2.',
'1movedto2'               => '[[$1]] feroare ta [[$2]]',
'1movedto2_redir'         => '[[$1]] feroare ta [[$2]], wat in synonym wie',
'movelogpage'             => 'Werneam-loch',
'movelogpagetext'         => 'Dit is in list fan feroare titels.',
'movereason'              => 'Reden:',
'revertmove'              => 'werom sette',
'delete_and_move'         => 'Wiskje en werneam',
'delete_and_move_text'    => '== Wiskjen nedich ==
De doelside "[[:$1]]" is der al. Moat dy wiske wurde om plak te meitsjen foar it werneamen?',
'delete_and_move_confirm' => 'Ja, wiskje de side',
'delete_and_move_reason'  => 'Wiske om plak te meitsjen foar in werneamde side',
'immobile_namespace'      => "De nije titel is yn in nammeromte dêr't gjin siden oan tafoege wurde kinne.",

# Export
'export'            => 'Eksportearje',
'export-submit'     => 'Eksportearje',
'export-addcattext' => 'Siden tafoegje fan kategory:',
'export-addcat'     => 'Tafoegje',
'export-download'   => 'Fêstlizze as triem',
'export-templates'  => 'Tafoegje berjochten',

# Namespace 8 related
'allmessages'         => 'Alle wikiberjochten',
'allmessagesname'     => 'Namme',
'allmessagesdefault'  => 'Standerttekst',
'allmessagescurrent'  => 'Tekst yn de nijste ferzje',
'allmessagestext'     => 'Dit is in list fan alle systeemberjochten beskikber yn de MediaWiki-nammeromte.
Sjoch: [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation], [http://translatewiki.net Betawiki].',
'allmessagesfilter'   => 'Berjocht namme filter:',
'allmessagesmodified' => 'Allinne wizige berjochten',

# Thumbnails
'thumbnail-more'           => 'Fergrutsje',
'filemissing'              => 'Triem net fûn',
'thumbnail_error'          => 'Flater by it oanmeitsjen fan thumbnail: $1',
'thumbnail_dest_directory' => 'Kin de doelmap net oanmeitsje',

# Special:Import
'import'                  => 'Importearje siden',
'import-interwiki-submit' => 'Ymportearje',
'importstart'             => 'Siden oan it ymportearjen ...',
'import-revision-count'   => '$1 {{PLURAL:$1|ferzje|ferzjes}}',
'importnopages'           => 'Gjin siden te ymportearjen.',
'importfailed'            => 'Ymport fout: <nowiki>$1</nowiki>',
'importunknownsource'     => 'Unbekende ymportboarnetype',
'importcantopen'          => 'De ymporttriem koe net iepenen wurde.',
'importbadinterwiki'      => 'Ferkearde ynterwikiferwizing',
'importnotext'            => 'Leech of gjin tekst',
'importsuccess'           => 'Ymport slagge!',
'importnofile'            => 'Gjin ymporttriem is oanbeane.',
'import-noarticle'        => 'Gjin side te ymportearjen!',
'import-nonewrevisions'   => 'Alle ferzjes wurde al ymportearre.',
'xml-error-string'        => '$1 op regel $2, kolom $3 (byte $4): $5',
'import-upload'           => 'XML-gegevens oanbiede',

# Import log
'importlogpage'                    => 'Ymportlochboek',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|ferzje|ferzjes}}',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|ferzje|ferzjes}} fan $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Myn brûkersside',
'tooltip-pt-mytalk'               => 'Myn oerlisside',
'tooltip-pt-preferences'          => 'Myn foarkarynstellings',
'tooltip-pt-watchlist'            => "List fan siden dy'sto besjochst op feroarings",
'tooltip-pt-mycontris'            => 'Myn bydragen',
'tooltip-pt-login'                => 'Jo wurde fan herten útnoege jo oan te melden, mar it hoecht net.',
'tooltip-pt-logout'               => 'Ofmelde',
'tooltip-ca-talk'                 => 'Oerlis oer dizze side',
'tooltip-ca-edit'                 => "Jo kinne dizze side bewurkje. Brûk a.j.w. de foarbyldwerjefteknop foar't Jo de boel bewarje.",
'tooltip-ca-addsection'           => 'In opmerking tafoegje oan de oerlis-side.',
'tooltip-ca-viewsource'           => 'Dizze side is befeilige, mar jo kinne de boarne wol besjen.',
'tooltip-ca-history'              => 'Eardere ferzjes fan dizze side.',
'tooltip-ca-protect'              => 'Dizze side befeiligje',
'tooltip-ca-delete'               => 'Dizze side weidwaan',
'tooltip-ca-move'                 => 'Dizze side ferskowe',
'tooltip-ca-watch'                => 'Dizze side oan myn folchside tafoegje',
'tooltip-ca-unwatch'              => 'Dizze side fan myn folchlist ôfhelje',
'tooltip-search'                  => '{{SITENAME}} trochsykje',
'tooltip-p-logo'                  => 'Haadside',
'tooltip-n-mainpage'              => 'Gean nei de haadside',
'tooltip-n-portal'                => "Oer it projekt: wat'st dwaan kinst, wêr'st dingen fine kinst.",
'tooltip-n-currentevents'         => 'Eftergrûnynformaasje oer rinnende saken.',
'tooltip-n-recentchanges'         => 'De list fan koartlyn oanbrochte feroarings yn dizze wiki.',
'tooltip-n-randompage'            => 'Samar in side sjen litte.',
'tooltip-n-help'                  => 'Helpynformaasje oer dizze wiki.',
'tooltip-t-whatlinkshere'         => "List fan alle siden dy't nei dizze side ferwize",
'tooltip-feed-rss'                => 'RSS-feed foar dizze side',
'tooltip-feed-atom'               => 'Atom-feed foar dizze side',
'tooltip-t-contributions'         => 'Bydragen fan dizze brûker',
'tooltip-t-emailuser'             => 'Stjoer in e-mail nei dizze brûker',
'tooltip-t-upload'                => 'Triemmen oplade',
'tooltip-t-specialpages'          => 'List fan alle spesjale siden',
'tooltip-ca-nstab-user'           => 'Brûkersside sjen litte',
'tooltip-ca-nstab-project'        => 'Projektside sjen litte',
'tooltip-ca-nstab-image'          => 'De triemside sjen litte',
'tooltip-ca-nstab-mediawiki'      => 'Systeemberjocht sjen litte',
'tooltip-ca-nstab-template'       => 'Sjabloan sjen litte',
'tooltip-ca-nstab-help'           => 'Helpside sjen litte',
'tooltip-ca-nstab-category'       => 'Kategory-side sjen litte',
'tooltip-minoredit'               => 'Markearje dit as in lytse feroaring',
'tooltip-save'                    => 'Jo feroarings bewarje',
'tooltip-preview'                 => "Oerlêze foar't de side fêstlein is!",
'tooltip-diff'                    => "Sjen litte hokker feroarings jo yn'e tekst makke hawwe.",
'tooltip-compareselectedversions' => 'Sjoch de ferskillen tusken de twa keazen ferzjes fan dizze side.',
'tooltip-watch'                   => 'Foegje dizze side ta oan jo folchlist [alt-w]',

# Scripts
'common.js' => '/* Alles wat hjir oan JavaScript delset wurdt, wurdt foar alle brûkers laden foar eltse side! */',

# Attribution
'anonymous'     => 'Anonime meidogger(s) fan {{SITENAME}}',
'siteuser'      => '{{SITENAME}} meidogger $1',
'othercontribs' => 'Basearre op wurk fan $1.',
'others'        => 'Oaren',
'siteusers'     => '{{SITENAME}} meidogger(s) $1',

# Spam protection
'spamprotectiontext'  => "De side dy't jo fêstlizze woene is blokkearre troch in spam filter. Dit wurdt wierskynlik feroarsake troch in ferwizing nei in ekstern webstee.",
'spamprotectionmatch' => 'De neikommende tekst hat it spam filter aktivearre: $1',

# Info page
'infosubtitle' => 'Ynformaasje foar side',

# Math options
'mw_math_png'    => 'Altiten as PNG ôfbyldzje',
'mw_math_simple' => 'HTML foar ienfâldiche formules, oars PNG',
'mw_math_html'   => 'HTML as mooglik, oars PNG',
'mw_math_source' => 'Lit de TeX ferzje stean (foar tekstblêders)',
'mw_math_modern' => 'Oanbefelle foar resinte blêders',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Markearje as kontroleare',
'markaspatrolledtext'                 => 'Markearje dizze side as kontrolearre',
'markedaspatrolled'                   => 'Markearre as kontrolearre',
'markedaspatrolledtext'               => 'De selektearre ferzje is markearre as kontrolearre.',
'markedaspatrollederror'              => 'Kin net as kontrolearre markearre wurde',
'markedaspatrollederrortext'          => "Jo moatte in ferzje oanjaan dy't jo as kontrolearre markearje.",
'markedaspatrollederror-noautopatrol' => 'Jo meie jo eigen bewurkings net sels markearre.',

# Browsing diffs
'previousdiff' => '← Toan eardere ferskillen',
'nextdiff'     => 'Neikommende ferskillen →',

# Media information
'imagemaxsize'         => 'Behein ôfmjittings fan ôfbyld op beskriuwingsside ta:',
'thumbsize'            => 'Mjitte fan miniatueren:',
'file-info'            => '(triemgrutte: $1, MIME-type: $2)',
'file-info-size'       => '($1 × $2 pixel, triemgrutte: $3, MIME type: $4)',
'file-nohires'         => '<small>Gjin hegere resolúsje beskikber.</small>',
'svg-long-desc'        => '(SVG-triem, nominaal $1 × $2 pixels, triemgrutte: $3)',
'show-big-image'       => 'Hegere resolúsje',
'show-big-image-thumb' => '<small>Grutte fan dizze ôfbylding: $1 × $2 pixels</small>',

# Special:NewImages
'newimages'     => 'Nije ôfbylden',
'imagelisttext' => "Dit is in list fan '''$1''' {{PLURAL:$1|triem|triemen}}, op $2.",
'showhidebots'  => '(Bots $1)',
'noimages'      => 'Neat te sjen.',
'ilsubmit'      => 'Sykje',
'bydate'        => 'datum',

# Bad image list
'bad_image_list' => "De opmaak is as folget:

Allinne rigels fan in list (rigels dy't begjinne mei *) wurde ferwurke. De earste link op in rigel moat in link wêze nei in net winske ôfbylding.
Alle folgjende links dy't op deselde rigel steane, wurde behannele as útsûndering, lykas bygelyks siden dêr't de ôfbylding yn'e tekst opnommen is.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "Dizze triem befettet oanfoljende ynformaasje, dy't troch in fotokamera, scanner of fotobewurkingsprogramma tafoege wêze kin. As de triem oanpast is, komme de details mûglik net folslein oerien mei de feroare ôfbylding.",
'metadata-expand'   => 'Utwreide details sjen litte',
'metadata-collapse' => 'Ferskûlje útwreide details',
'metadata-fields'   => 'De EXIF-metadatafjilden yn dit berjocht steane op in ôfbyldingsside as de metadatatabel ynklapt is. Oare fjilden wurde ferburgen.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-samplesperpixel'        => 'Oantal komponinten',
'exif-xresolution'            => 'Horizontale resolúsje',
'exif-yresolution'            => 'Fertikale resolúsje',
'exif-imagedescription'       => 'Ofbylding titel',
'exif-make'                   => 'Kamera makker',
'exif-artist'                 => 'Auteur',
'exif-colorspace'             => 'Kleurromte',
'exif-compressedbitsperpixel' => 'Ofbylding kompresjemetoade',
'exif-makernote'              => 'Opmerkings fan makker',
'exif-usercomment'            => 'Opmerkings',
'exif-relatedsoundfile'       => 'Besibbe audiotriem',
'exif-exposuretime-format'    => '$1 sek ($2)',
'exif-flash'                  => 'Flits',
'exif-filesource'             => 'Triemboarne',
'exif-gpsdatestamp'           => 'GPS-datum',

'exif-unknowndate' => 'Datum ûnbekend',

'exif-orientation-1' => 'Normaal', # 0th row: top; 0th column: left

'exif-componentsconfiguration-0' => 'bestiet net',

'exif-exposureprogram-0' => 'Net bepaald',
'exif-exposureprogram-2' => 'Normaal programma',

'exif-subjectdistance-value' => '$1 meter',

'exif-meteringmode-0' => 'Unbekend',
'exif-meteringmode-5' => 'Patroan',

'exif-lightsource-0' => 'Unbekend',
'exif-lightsource-1' => 'Deiljocht',
'exif-lightsource-4' => 'Flits',

'exif-customrendered-0' => 'Normale ferwurking',

'exif-scenecapturetype-0' => 'Standert',
'exif-scenecapturetype-1' => 'Lânskip',

'exif-gaincontrol-0' => 'Gjin',

'exif-contrast-0' => 'Normaal',

'exif-saturation-0' => 'Normaal',

'exif-sharpness-0' => 'Normaal',

'exif-subjectdistancerange-0' => 'Unbekend',
'exif-subjectdistancerange-2' => 'Tichtby',

# External editor support
'edit-externally'      => 'Wizigje dizze triem mei in ekstern programma',
'edit-externally-help' => 'Sjoch de [http://www.mediawiki.org/wiki/Manual:External_editors ynstel-hantlieding] foar mear ynformaasje.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'alle',
'imagelistall'     => 'alle',
'watchlistall2'    => 'alles',
'namespacesall'    => 'alles',
'monthsall'        => 'alle',

# E-mail address confirmation
'confirmemail'            => 'Befêstigjen netpostadres',
'confirmemail_text'       => '{{SITENAME}} freget dat jo jo netpostadres befêstigje eart jo hjir netpost brûke. Brûk de knop hjirûnder om josels in befêstigingskoade ta te stjoeren op it adres dat jo opjûn hawwe. Iepenje de koade dan yn jo blêder om te befêstigjen dat jo netpostadres jildich is.',
'confirmemail_send'       => 'Stjoer in befêstigingskoade',
'confirmemail_sent'       => 'Befêstiginskoade tastjoerd.',
'confirmemail_sendfailed' => 'De befêstigingskoade koe net stjoerd wurde. Faaks stean der ferkearde tekens yn it e-postadres.

Berjocht: $1',
'confirmemail_invalid'    => 'Dizze befêstiginskoade jildt net (mear). 
Faaks is de koade ferrûn.',
'confirmemail_success'    => 'Jo netpostadres is befêstige. Jo kinne jo no oanmelde en de wiki brûke.',
'confirmemail_loggedin'   => 'Jo netpostadres is no befêstige.',
'confirmemail_error'      => 'Der is wat skeefgongen by it fêstlizzen fan jo befêstiging.',
'confirmemail_subject'    => 'Netpost-befêstigingskoade foar {{SITENAME}}',
'confirmemail_body'       => 'Immen, nei gedachten jo, hat him by {{SITENAME}} oanmelde as "$2", mei dit netpostadres ($1). 

Hjirtroch komme ek de netpostfunksjes fan {{SITENAME}} foar jo beskikber. Iepenje de neikommende keppeling om te befêstigjen dat jo wier josels by {{SITENAME}} mei dit netpostadres oanmelde hawwe:

$3

At jo dat *net* wienen, brûk dy keppeling dan net, en klik hjir:

$5

Dizze befêstigingskoade ferrint dan op $4.',

# Scary transclusion
'scarytranscludetoolong' => '[URL-adres is te lang]',

# Trackbacks
'trackbackremove' => ' ([$1 Wiskje])',

# Delete conflict
'confirmrecreate' => "Sûnt jo begûn binne dizze side te bewurkjen, hat meidogger [[User:$1|$1]] ([[User talk:$1|oerlis]]) de side wiske. De reden dy't derfoar jûn waard wie:
: ''$2''
Wolle jo de side wier op 'e nij skriuwe?",

# HTML dump
'redirectingto' => 'Synonym foar [[:$1]]...',

# Multipage image navigation
'imgmultipagenext' => 'folgjende side →',
'imgmultigo'       => 'Los!',
'imgmultigoto'     => 'Gean nei side $1',

# Table pager
'table_pager_next'         => 'Folgjende side',
'table_pager_first'        => 'Earste side',
'table_pager_limit_submit' => 'Los!',
'table_pager_empty'        => 'Gjin resultaat',

# Auto-summaries
'autosumm-blank'   => 'Alle ynhâld fan de side weismiten',
'autosumm-replace' => "Side ferfong mei '$1'",
'autoredircomment' => 'Ferwiist troch nei [[$1]]',
'autosumm-new'     => 'Nije Side: $1',

# Live preview
'livepreview-loading' => 'Ynlade...',

# Watchlist editor
'watchlistedit-normal-title'  => 'Folchlist bewurkje',
'watchlistedit-normal-submit' => 'Siden wiskje',
'watchlistedit-raw-titles'    => 'Siden:',
'watchlistedit-raw-submit'    => 'Folchlist bewurkje',
'watchlistedit-raw-added'     => '{{PLURAL:$1|1 side is|$1 siden binne}} tafoege:',
'watchlistedit-raw-removed'   => '{{PLURAL:$1|1 side|$1 siden}} wiske:',

# Watchlist editing tools
'watchlisttools-view' => 'Folchlist besjen',
'watchlisttools-edit' => 'Folchlist besjen en bewurkje',
'watchlisttools-raw'  => 'Rûge folchlist bewurkje',

# Special:Version
'version'                  => 'Ferzje', # Not used as normal message but as header for the special page itself
'version-specialpages'     => 'Bysûndere siden',
'version-variables'        => 'Fariabels',
'version-other'            => 'Oare',
'version-version'          => 'Ferzje',
'version-license'          => 'Lisinsje',
'version-software'         => 'Ynsteld software',
'version-software-product' => 'Produkt',
'version-software-version' => 'Ferzje',

# Special:FilePath
'filepath'        => 'Triempad',
'filepath-page'   => 'Triem:',
'filepath-submit' => 'Pad',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Sykje op duplikaten',
'fileduplicatesearch-legend'   => 'Sykje op duplikaten',
'fileduplicatesearch-filename' => 'Triemnamme:',
'fileduplicatesearch-submit'   => 'Sykje',
'fileduplicatesearch-info'     => '$1 × $2 pixel<br />Triemgrutte: $3<br />MIME-type: $4',
'fileduplicatesearch-result-1' => 'De triem "$1" hat gjin duplikaten.',
'fileduplicatesearch-result-n' => 'De triem "$1" hat {{PLURAL:$2|1 duplikaat|$2 duplikaten}}.',

# Special:SpecialPages
'specialpages'                   => 'Bysûndere siden',
'specialpages-note'              => '----
* Normale bysûndere siden.
* <span class="mw-specialpagerestricted">Beheinde bysûndere siden.</span>',
'specialpages-group-maintenance' => 'Underhâld siden',
'specialpages-group-other'       => 'Oare bysûndere siden',
'specialpages-group-login'       => 'Oanmelde',
'specialpages-group-changes'     => 'Koartlyn feroare en lochs',
'specialpages-group-media'       => 'Oanbieden en oare triemsiden',
'specialpages-group-users'       => 'Meidoggers en rjochten',
'specialpages-group-highuse'     => "Siden dy't in protte brûkt wurde",
'specialpages-group-pages'       => 'List fan siden',
'specialpages-group-pagetools'   => 'Sidehelpmiddels',
'specialpages-group-wiki'        => 'Wikigegevens en -helpmiddels',
'specialpages-group-redirects'   => 'Trochferwizende bysûndere siden',
'specialpages-group-spam'        => 'Spamhelpmiddels',

# Special:BlankPage
'blankpage'              => 'Side is leech',
'intentionallyblankpage' => 'Dizze side is bewust leech lizzen en wurdt brûkt foar benchmarks, ensfh.',

);
