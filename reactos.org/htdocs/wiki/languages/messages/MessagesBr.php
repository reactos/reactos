<?php
/** Breton (Brezhoneg)
 *
 * @ingroup Language
 * @file
 *
 * @author Fulup
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Dibar',
	NS_MAIN           => '',
	NS_TALK           => 'Kaozeal',
	NS_USER           => 'Implijer',
	NS_USER_TALK      => 'Kaozeadenn_Implijer',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => 'Kaozeadenn_$1',
	NS_IMAGE          => 'Skeudenn',
	NS_IMAGE_TALK     => 'Kaozeadenn_Skeudenn',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Kaozeadenn_MediaWiki',
	NS_TEMPLATE       => 'Patrom',
	NS_TEMPLATE_TALK  => 'Kaozeadenn_Patrom',
	NS_HELP           => 'Skoazell',
	NS_HELP_TALK      => 'Kaozeadenn_Skoazell',
	NS_CATEGORY       => 'Rummad',
	NS_CATEGORY_TALK  => 'Kaozeadenn_Rummad'
);

$skinNames = array(
	'standard'    => 'Standard',
	'nostalgia'   => 'Melkoni',
	'cologneblue' => 'Glaz Kologn',
	'monobook'    => 'MonoBook',
	'myskin'      => 'MySkin',
	'chick'       => 'Plogig',
	'simple'      => 'Eeun',
);

$bookstoreList = array(
	'Amazon.fr'    => 'http://www.amazon.fr/exec/obidos/ISBN=$1',
	'alapage.fr'   => 'http://www.alapage.com/mx/?tp=F&type=101&l_isbn=$1&donnee_appel=ALASQ&devise=&',
	'fnac.com'     => 'http://www3.fnac.com/advanced/book.do?isbn=$1',
	'chapitre.com' => 'http://www.chapitre.com/frame_rec.asp?isbn=$1',
);

$datePreferences = false;
$defaultDateFormat = 'dmy';
$dateFormats = array(
	'dmy time' => 'H:i',
	'dmy date' => 'j M Y',
	'dmy both' => 'j M Y "da" H:i',
);

$separatorTransformTable = array(',' => "\xc2\xa0", '.' => ',' );
$linkTrail = "/^([a-zàâçéèêîôûäëïöüùÇÉÂÊÎÔÛÄËÏÖÜÀÈÙ]+)(.*)$/sDu";

$messages = array(
# User preference toggles
'tog-underline'               => 'Liammoù islinennet',
'tog-highlightbroken'         => 'Furmad al liammoù torr <a href="" class="new">evel-mañ</a> (pe : evel-se<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Rannbennadoù marzekaet',
'tog-hideminor'               => "Kuzhat ar <i>C'hemmoù nevez</i> dister",
'tog-extendwatchlist'         => 'Implijout ar roll evezhiañ gwellaet evit heuliañ an holl gemmoù',
'tog-usenewrc'                => 'Kemmoù nevez gwellaet<br /> (gant merdeerioù zo hepken)',
'tog-numberheadings'          => 'Niverenniñ emgefre an titloù',
'tog-showtoolbar'             => 'Diskouez ar varrenn gant ar meuzioù skridaozañ',
'tog-editondblclick'          => 'Daouglikañ evit kemmañ ur bajenn (JavaScript)',
'tog-editsection'             => 'Kemmañ ur rann dre al liammoù [kemmañ]',
'tog-editsectiononrightclick' => 'Kemmañ ur rann dre glikañ a-zehou<br /> war titl ar rann',
'tog-showtoc'                 => 'Diskouez an daolenn<br /> (evit ar pennadoù zo ouzhpenn 3 rann enno)',
'tog-rememberpassword'        => "Derc'hel soñj eus ma ger-tremen (toupin)",
'tog-editwidth'               => 'Digeriñ ar prenestr skridaozañ en e led brasañ',
'tog-watchcreations'          => 'Evezhiañ ar pajennoù krouet ganin',
'tog-watchdefault'            => 'Evezhiañ ar pennadoù savet pe kemmet ganin',
'tog-watchmoves'              => "Ouzhpennañ da'm roll evezhiañ ar pajennoù adanvet ganin",
'tog-watchdeletion'           => "Ouzhpennañ da'm roll evezhiañ ar pajennoù diverket ganin",
'tog-minordefault'            => "Sellet ouzh ar c'hemmoù degaset ganin<br /> evel kemmoù dister dre ziouer",
'tog-previewontop'            => 'Rakwelet tres ar bajenn a-us ar prenestr skridaozañ',
'tog-previewonfirst'          => 'Rakwelet tres ar bajenn kerkent hag an aozadenn gentañ',
'tog-nocache'                 => 'Diweredekaat krubuilh ar pajennoù',
'tog-enotifwatchlistpages'    => 'Kas ur postel din pa vez degaset kemmoù war ur bajenn evezhiet ganin',
'tog-enotifusertalkpages'     => 'Kas ur postel din pa vez degaset kemmoù war ma fajenn gaozeal',
'tog-enotifminoredits'        => 'Kas ur postel din, ha pa vije evit kemenn kemmoù dister',
'tog-enotifrevealaddr'        => "Lakaat ma chomlec'h postel war wel er posteloù kemenn-diwall",
'tog-shownumberswatching'     => 'Diskouez an niver a lennerien',
'tog-fancysig'                => 'Sinadurioù diliamm (hep liamm emgefre)',
'tog-externaleditor'          => "Ober gant ur skridaozer diavaez dre ziouer (evit arbennigourien hepken rak ezhomm zo arventenniñ hoc'h urzhiataer evit se)",
'tog-externaldiff'            => "Ober gant ur c'heñverier diavaez dre ziouer (evit arbennigourien hepken rak ezhomm zo arventenniñ hoc'h urzhiataer evit se)",
'tog-showjumplinks'           => 'Gweredekaat al liammoù moned "lammat da"',
'tog-uselivepreview'          => 'Implijout Rakwelet prim (JavaScript) (taol-arnod)',
'tog-forceeditsummary'        => 'Kemenn din pa ne skrivan netra er stern diverrañ',
'tog-watchlisthideown'        => "Kuzhat ma c'hemmoù er rollad evezhiañ",
'tog-watchlisthidebots'       => 'Kuzhat kemmoù ar botoù er rollad evezhiañ',
'tog-watchlisthideminor'      => "Kuzhat ar c'hemmoù dister er rollad evezhiañ",
'tog-nolangconversion'        => "Diweredekaat an treiñ diforc'hioù yezh",
'tog-ccmeonemails'            => 'Kas din un eilenn eus ar posteloù a gasan da implijerien all',
'tog-diffonly'                => "Arabat diskouez danvez ar pennadoù dindan an diforc'hioù",
'tog-showhiddencats'          => 'Diskouez ar rummadoù kuzhet',

'underline-always'  => 'Atav',
'underline-never'   => 'Morse',
'underline-default' => 'Diouzh ar merdeer',

'skinpreview' => '(Rakwelet)',

# Dates
'sunday'        => 'Sul',
'monday'        => 'Lun',
'tuesday'       => 'Meurzh',
'wednesday'     => "Merc'her",
'thursday'      => 'Yaou',
'friday'        => 'Gwener',
'saturday'      => 'Sadorn',
'sun'           => 'Sul',
'mon'           => 'Lun',
'tue'           => 'Meu',
'wed'           => 'Mer',
'thu'           => 'Meu',
'fri'           => 'Gwe',
'sat'           => 'Sad',
'january'       => 'Genver',
'february'      => "C'hwevrer",
'march'         => 'Meurzh',
'april'         => 'Ebrel',
'may_long'      => 'Mae',
'june'          => 'Mezheven',
'july'          => 'Gouere',
'august'        => 'Eost',
'september'     => 'Gwengolo',
'october'       => 'Here',
'november'      => 'Du',
'december'      => 'Kerzu',
'january-gen'   => 'Genver',
'february-gen'  => "C'hwevrer",
'march-gen'     => 'Meurzh',
'april-gen'     => 'Ebrel',
'may-gen'       => 'Mae',
'june-gen'      => 'Mezheven',
'july-gen'      => 'Gouere',
'august-gen'    => 'Eost',
'september-gen' => 'Gwengolo',
'october-gen'   => 'Here',
'november-gen'  => 'Du',
'december-gen'  => 'Kerzu',
'jan'           => 'Gen',
'feb'           => "C'hwe",
'mar'           => 'Meu',
'apr'           => 'Ebr',
'may'           => 'Mae',
'jun'           => 'Mez',
'jul'           => 'Gou',
'aug'           => 'Eos',
'sep'           => 'Gwe',
'oct'           => 'Her',
'nov'           => 'Du',
'dec'           => 'Kzu',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Rummad |Rummad }}',
'category_header'                => 'Niver a bennadoù er rummad "$1"',
'subcategories'                  => 'Isrummad',
'category-media-header'          => 'Restroù liesvedia er rummad "$1"',
'category-empty'                 => "''N'eus na pajenn na media ebet er rummad-mañ evit ar mare.''",
'hidden-categories'              => '{{PLURAL:$1|Rummad kuzhet|Rummad kuzhet}}',
'hidden-category-category'       => 'Rummadoù kuzhet', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$2|N'eus er rummad-mañ nemet an isrummad da-heul.|{{PLURAL:$1|isrummad|$1 isrummad}} zo d'ar rummad-mañ diwar un hollad a $2.}}",
'category-subcat-count-limited'  => 'Er rummad-mañ e kaver an {{PLURAL:$1|isrummad-se|$1 isrummadoù-se}}.',
'category-article-count'         => "{{PLURAL:$2|N'eus er rummad-mañ nemet ar bajenn da-heul.|Emañ ar {{PLURAL:$1|bajenn da-heul|$1 pajenn da-heul}} er rummad-mañ, war un hollad a $2.}}",
'category-article-count-limited' => '{{PLURAL:$1|Emañ ar bajenn|Emañ an $1 pajenn}} da-heul er rummad-mañ.',
'category-file-count'            => "{{PLURAL:$2|N'eus er rummad-mañ nemet ar restr da-heul.|Emañ ar {{PLURAL:$1|restr|$1 restr}} da-heul er rummad-mañ, war un hollad a $2.}}",
'category-file-count-limited'    => '{{PLURAL:$1|Emañ ar restr|Emañ an $1 restr}} da-heul er rummad-mañ.',
'listingcontinuesabbrev'         => "(war-lerc'h)",

'mainpagetext'      => 'Meziant {{SITENAME}} staliet.',
'mainpagedocfooter' => "Sellit ouzh [http://meta.wikimedia.org/wiki/Help:Contents Sturlevr an implijerien] evit gouzout hiroc'h war an doare da implijout ar meziant wiki.

== Kregiñ ganti ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Configuration settings list]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki release mailing list]",

'about'          => 'Diwar-benn',
'article'        => 'Pennad',
'newwindow'      => '(digeriñ en ur prenestr nevez)',
'cancel'         => 'Nullañ',
'qbfind'         => 'Klask',
'qbbrowse'       => 'Furchal',
'qbedit'         => 'Kemmañ',
'qbpageoptions'  => 'Pajenn an dibaboù',
'qbpageinfo'     => 'Pajenn gelaouiñ',
'qbmyoptions'    => 'Ma dibaboù',
'qbspecialpages' => 'Pajennoù dibar',
'moredotdotdot'  => "Ha muioc'h c'hoazh...",
'mypage'         => 'Ma zammig pajenn',
'mytalk'         => "Ma c'haozeadennoù",
'anontalk'       => "Kaozeal gant ar chomlec'h IP-mañ",
'navigation'     => 'Merdeiñ',
'and'            => 'ha(g)',

# Metadata in edit box
'metadata_help' => 'Metaroadennoù :',

'errorpagetitle'    => 'Fazi',
'returnto'          => "Distreiñ d'ar bajenn $1.",
'tagline'           => 'Eus {{SITENAME}}',
'help'              => 'Skoazell',
'search'            => 'Klask',
'searchbutton'      => 'Klask',
'go'                => 'Kas',
'searcharticle'     => 'Mont',
'history'           => 'Istor ar bajenn',
'history_short'     => 'Istor',
'updatedmarker'     => 'kemmet abaoe ma zaol-sell diwezhañ',
'info_short'        => 'Titouroù',
'printableversion'  => 'Stumm da voullañ',
'permalink'         => "Chomlec'h ar stumm-mañ",
'print'             => 'Moullañ',
'edit'              => 'Kemmañ',
'create'            => 'Krouiñ',
'editthispage'      => 'Kemmañ ar bajenn-mañ',
'create-this-page'  => 'Krouiñ ar bajenn-mañ',
'delete'            => 'Diverkañ',
'deletethispage'    => 'Diverkañ ar bajenn-mañ',
'undelete_short'    => "Diziverkañ {{PLURAL:$1|ur c'hemm|$1 kemm}}",
'protect'           => 'Gwareziñ',
'protect_change'    => 'kemm gwarez',
'protectthispage'   => 'Gwareziñ ar bajenn-mañ',
'unprotect'         => 'Diwareziñ',
'unprotectthispage' => 'Diwareziñ ar bajenn-mañ',
'newpage'           => 'Pajenn nevez',
'talkpage'          => 'Pajenn gaozeal',
'talkpagelinktext'  => 'Kaozeal',
'specialpage'       => 'Pajenn zibar',
'personaltools'     => 'Ostilhoù personel',
'postcomment'       => 'Ouzhpennañ e soñj',
'articlepage'       => 'Sellet ouzh ar pennad',
'talk'              => 'Kaozeal',
'views'             => 'Gweladennoù',
'toolbox'           => 'Boest ostilhoù',
'userpage'          => 'Pajenn implijer',
'projectpage'       => 'Pajenn meta',
'imagepage'         => 'Pajenn skeudenn',
'mediawikipage'     => "Sellet ouzh pajenn ar c'hemennadennoù",
'templatepage'      => 'Gwelet patrom ar bajenn',
'viewhelppage'      => 'Gwelet ar bajenn skoazell',
'categorypage'      => 'Gwelet pajenn ar rummadoù',
'viewtalkpage'      => 'Pajenn gaozeal',
'otherlanguages'    => 'Yezhoù all',
'redirectedfrom'    => '(Adkaset eus $1)',
'redirectpagesub'   => 'Pajenn adkas',
'lastmodifiedat'    => "Kemmoù diwezhañ degaset d'ar bajenn-mañ : $2, $1.", # $1 date, $2 time
'viewcount'         => 'Sellet euz eus bet {{PLURAL:$1|$1 wech|$1 (g)wech}} ouzh ar bajenn-mañ.',
'protectedpage'     => 'Pajenn warezet',
'jumpto'            => 'Mont da :',
'jumptonavigation'  => 'merdeiñ',
'jumptosearch'      => 'klask',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Diwar-benn {{SITENAME}}',
'aboutpage'            => 'Project:Diwar-benn',
'bugreports'           => 'Teul an drein',
'bugreportspage'       => 'Project:Teul an drein',
'copyright'            => "Danvez a c'haller implijout dindan $1.",
'copyrightpagename'    => 'aotre {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Gwirioù aozer (Copyright)',
'currentevents'        => 'Keleier',
'currentevents-url'    => 'Project:Keleier',
'disclaimers'          => 'Kemennoù',
'disclaimerpage'       => 'Project:Kemenn hollek',
'edithelp'             => 'Skoazell',
'edithelppage'         => 'Help:Penaos degas kemmoù en ur bajenn',
'faq'                  => 'FAG',
'faqpage'              => 'Project:FAG',
'helppage'             => 'Help:Skoazell',
'mainpage'             => 'Degemer',
'mainpage-description' => 'Degemer',
'policy-url'           => 'Project:Reolennoù',
'portal'               => 'Porched ar gumuniezh',
'portal-url'           => 'Project:Degemer',
'privacy'              => 'Reolennoù prevezded',
'privacypage'          => 'Project:Reolennoù prevezded',

'badaccess'        => 'Fazi aotre',
'badaccess-group0' => "N'oc'h ket aotreet da seveniñ ar pezh hoc'h eus goulennet.",
'badaccess-group1' => 'Miret eo an ober-mañ evit an dud er strollad $1 hepken.',
'badaccess-group2' => 'Miret eo an ober-mañ evit an dud en unan eus ar strolladoù $1 hepken.',
'badaccess-groups' => 'Miret eo an ober-mañ evit an dud en unan eus ar strolladoù $1 hepken.',

'versionrequired'     => 'Rekis eo Stumm $1 MediaWiki',
'versionrequiredtext' => 'Rekis eo stumm $1 MediaWiki evit implijout ar bajenn-mañ. Sellit ouzh [[Special:Version]]',

'ok'                      => 'Mat eo',
'retrievedfrom'           => 'Adtapet diwar « $1 »',
'youhavenewmessages'      => "$1 zo ganeoc'h ($2).",
'newmessageslink'         => 'Kemennoù nevez',
'newmessagesdifflink'     => "Diforc'hioù e-keñver ar stumm kent",
'youhavenewmessagesmulti' => "Kemennoù nevez zo ganeoc'h war $1",
'editsection'             => 'kemmañ',
'editold'                 => 'kemmañ',
'viewsourceold'           => 'gwelet ar vammenn',
'editsectionhint'         => 'Kemmañ ar rann : $1',
'toc'                     => 'Taolenn',
'showtoc'                 => 'diskouez',
'hidetoc'                 => 'kuzhat',
'thisisdeleted'           => 'Diskouez pe diziverkañ $1 ?',
'viewdeleted'             => 'Gwelet $1?',
'restorelink'             => "{{PLURAL:$1|ur c'hemm diverket|$1 kemm diverket}}",
'feedlinks'               => 'Lanv :',
'feed-invalid'            => 'Seurt lanv direizh.',
'feed-unavailable'        => "N'haller ket implijout lanvadoù koumanantiñ war {{SITENAME}}",
'site-rss-feed'           => 'Lanv RSS evit $1',
'site-atom-feed'          => 'Lanv Atom evit $1',
'page-rss-feed'           => 'Lanv RSS evit "$1"',
'page-atom-feed'          => 'Lanv Atom evit "$1"',
'red-link-title'          => "$1 (n'eo ket bet savet c'hoazh)",

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Pennad',
'nstab-user'      => 'Pajenn implijer',
'nstab-media'     => 'Media',
'nstab-special'   => 'Dibar',
'nstab-project'   => 'Diwar-benn',
'nstab-image'     => 'Skeudenn',
'nstab-mediawiki' => 'Kemennadenn',
'nstab-template'  => 'Patrom',
'nstab-help'      => 'Skoazell',
'nstab-category'  => 'Rummad',

# Main script and global functions
'nosuchaction'      => 'Ober dianav',
'nosuchactiontext'  => "N'eo ket anavezet gant ar wiki an ober spisaet en Url.",
'nosuchspecialpage' => "N'eus ket eus ar bajenn zibar-mañ",
'nospecialpagetext' => "Goulennet hoc'h eus ur bajenn zibar n'eo ket anavezet gant ar wiki.",

# General errors
'error'                => 'Fazi',
'databaseerror'        => 'Fazi bank roadennoù',
'dberrortext'          => 'Fazi ereadur er bank roadennoù. Setu ar goulenn bet pledet gantañ da ziwezhañ :
<blockquote><tt>$1</tt></blockquote>
adal an arc\'hwel "<tt>$2</tt>".
Adkaset eo bet ar fazi "<tt>$3: $4</tt>" gant MySQL.',
'dberrortextcl'        => 'Ur fazi ereadur zo en ur goulenn graet ouzh ar bank roadennoù. Setu ar goulenn bet pledet gantañ da ziwezhañ :
"$1"
graet gant an arc\'hwel "$2"
adkaset eo bet ar fazi "$3 : $4" gant MySQL.',
'noconnect'            => "Ho tigarez! Da-heul kudennoù teknikel, n'haller ket kevreañ ouzh ar bank roadennoù evit poent.
<br />
$1",
'nodb'                 => 'Dibosupl dibab ar bank roadennoù $1',
'cachederror'          => "Un eilenn eus ar bajenn goulennet eo homañ; marteze n'eo ket bet hizivaet",
'laggedslavemode'      => "Diwallit : marteze a-walc'h n'emañ ket ar c'hemmoù diwezhañ war ar bajenn-mañ",
'readonly'             => 'Hizivadurioù stanket war ar bank roadennoù',
'enterlockreason'      => 'Merkit perak eo stanket hag istimit pegeit e chomo evel-henn',
'readonlytext'         => "Evit poent n'haller ket ouzhpennañ pe gemmañ netra er bank roadennoù mui. Un tamm kempenn boutin d'ar bank moarvat. goude-se e vo plaen an traoù en-dro.

Setu displegadenn ar merour bet prennet ar bank gantañ : $1",
'missingarticle-rev'   => '(adweladenn # : $1)',
'missingarticle-diff'  => '(Dif : $1, $2)',
'readonly_lag'         => "Stanket eo bet ar bank roadennoù ent emgefre p'emañ an eilservijerioù oc'h adpakañ o dale e-keñver ar pennservijer",
'internalerror'        => 'Fazi diabarzh',
'internalerror_info'   => 'Fazi diabarzh : $1',
'filecopyerror'        => 'Dibosupl eilañ « $1 » war-du « $2 ».',
'filerenameerror'      => 'Dibosupl da adenvel « $1 » e « $2 ».',
'filedeleteerror'      => 'Dibosupl da ziverkañ « $1 ».',
'directorycreateerror' => 'N\'eus ket bet gallet krouiñ kavlec\'h "$1".',
'filenotfound'         => 'N\'haller ket kavout ar restr "$1".',
'fileexistserror'      => 'Dibosupl skrivañ war ar restr "$1": bez\'ez eus eus ar restr-se dija',
'unexpected'           => 'Talvoudenn dic\'hortoz : "$1"="$2".',
'formerror'            => 'Fazi: Dibosupl eo kinnig ar furmskrid',
'badarticleerror'      => "N'haller ket seveniñ an ober-mañ war ar bajenn-mañ.",
'cannotdelete'         => 'Dibosupl da ziverkañ ar bajenn pe ar skeudenn spisaet.',
'badtitle'             => 'Titl fall',
'badtitletext'         => "Faziek pe c'houllo eo titl ar bajenn goulennet; pe neuze eo faziek al liamm etreyezhel",
'perfdisabled'         => "Ho tigarez! Diweredekaet eo bet an arc'hwel-mañ evit poent rak gorrekaat a ra ar bank roadennoù kement ha ma n'hall ket mui den implijout ar wiki.",
'perfcached'           => "Krubuilhet eo ar roadennoù da-heul ha marteze n'int ket bet hizivaet.",
'perfcachedts'         => "Krubuilhet eo ar roadennoù-mañ; hizivaet int bet da ziwezhañ d'an $1.",
'querypage-no-updates' => 'Diweredekaet eo an hizivaat evit ar bajenn-mañ. Evit poent ne vo ket nevesaet ar roadennoù amañ.',
'wrong_wfQuery_params' => "Arventennoù faziek war an urzhiad wfQuery()<br />
Arc'hwel : $1<br />
Goulenn : $2",
'viewsource'           => 'Sellet ouzh tarzh an destenn',
'viewsourcefor'        => 'evit $1',
'actionthrottled'      => 'Ober daleet',
'actionthrottledtext'  => "A-benn dizarbenn ar spam, n'haller ket implijout an ober-mañ re alies en ur frapad amzer lakaet, hag aet oc'h dreist ar muzul. Klaskit en-dro a-benn un nebeut munutennoù.",
'protectedpagetext'    => "Prennet eo bet ar bajenn-mañ. N'haller ket degas kemmoù enni.",
'viewsourcetext'       => 'Gallout a rit gwelet hag eilañ danvez ar bajenn-mañ',
'protectedinterface'   => 'Testenn ar bajenn-mañ a dalvez evit etrefas ar meziant. Setu perak eo bet gwarezet ar bajenn.',
'editinginterface'     => "'''Diwallit :''' Emaoc'h oc'h adaozañ ur bajenn a dalvez da sevel skridoù evit etrefas ar meziant. Ar c'hemmoù graet d'ar bajenn-mañ a zegaso kemmoù war etrefas an holl implijerien. Mar fell deoc'h skoazellañ evit treiñ traoù, soñjit kentoc'h implijout [http://translatewiki.net/wiki/Main_Page?setlang=br Betawiki], ar raktres evit lec'helaat MediaWiki.",
'sqlhidden'            => '(Reked SQL kuzhet)',
'cascadeprotected'     => 'Gwarezet eo ar bajenn-mañ; n\'haller ket degas kemmoù enni peogwir he c\'haver er {{PLURAL:$1|bajenn|pajennoù}} da-heul zo bet gwarezet en ur zibab an dibarzh "skalierad" :
$2',
'namespaceprotected'   => "N'oc'h ket aotreet da zegas kemmoù e pajennoù an esaouenn anv '''$1'''.",
'customcssjsprotected' => "N'oc'h ket aotreet da zegas kemmoù war ar bajenn-mañ rak kavout a reer enni arventennoù personel un implijer all.",
'ns-specialprotected'  => "N'haller ket kemmañ ar pajennoù en esaouenn anv {{ns:special}}.",
'titleprotected'       => "Gwarezet eo bet an titl-mañ p'eo bet krouet gant [[User:$1|$1]].
Setu amañ perak ''$2''.",

# Login and logout pages
'logouttitle'                => 'Dilugañ',
'logouttext'                 => "Diluget oc'h bremañ.
Gallout a rit kenderc'hel da implijout {{SITENAME}} en un doare dizanv, pe en em lugañ en-dro gant un anv all mar fell deoc'h.",
'welcomecreation'            => '== Degemer mat, $1! ==

Krouet eo bet ho kont implijer.
Na zisoñjit ket personelaat ho {{SITENAME}} en ur sellet ouzh pajenn ar Penndibaboù.',
'loginpagetitle'             => 'Ho tisklêriadenn',
'yourname'                   => "Hoc'h anv implijer",
'yourpassword'               => 'Ho ker-tremen',
'yourpasswordagain'          => 'Skrivit ho ker-tremen en-dro',
'remembermypassword'         => "Derc'hel soñj eus ma ger-tremen (toupin)",
'yourdomainname'             => 'Ho tomani',
'externaldberror'            => "Pe ez eus bet ur fazi gwiriekaat diavaez er bank titouroù pe n'oc'h ket aotreet da nevesaat ho kont diavaez.",
'loginproblem'               => '<b>Kudenn zisklêriañ.</b><br />Klaskit en-dro !',
'login'                      => 'Disklêriañ',
'nav-login-createaccount'    => 'Krouiñ ur gont pe en em lugañ',
'loginprompt'                => "Ret eo deoc'h bezañ gweredekaet an toupinoù evit bezañ luget ouzh {{SITENAME}}.",
'userlogin'                  => 'Krouiñ ur gont pe en em lugañ',
'logout'                     => 'Dilugañ',
'userlogout'                 => 'Dilugañ',
'notloggedin'                => 'Diluget',
'nologin'                    => "N'oc'h ket luget ? $1.",
'nologinlink'                => 'Krouiñ ur gont',
'createaccount'              => 'Krouiñ ur gont nevez',
'gotaccount'                 => "Ur gont zo ganeoc'h dija ? $1.",
'gotaccountlink'             => 'En em lugañ',
'createaccountmail'          => 'dre bostel',
'badretype'                  => "N'eo ket peurheñvel an eil ouzh egile an daou c'her-tremen bet lakaet ganeoc'h.",
'userexists'                 => "Implijet eo dija an anv implijer lakaet ganeoc'h. Dibabit unan all mar plij.",
'youremail'                  => 'Postel *:',
'username'                   => 'Anv implijer :',
'uid'                        => 'Niv. identelezh an implijer :',
'prefs-memberingroups'       => 'Ezel eus {{PLURAL:$1|ar strollad|ar strolladoù}}:',
'yourrealname'               => 'Anv gwir*',
'yourlanguage'               => 'Yezh an etrefas&nbsp;',
'yourvariant'                => 'Adstumm:',
'yournick'                   => "Sinadur evit ar c'haozeadennoù",
'badsig'                     => 'Direizh eo ho sinadur kriz; gwiriit ho palizennoù HTML.',
'badsiglength'               => "Sinadur re hir.
Dre ret e rank bezañ nebeutoc'h eget {{PLURAL:$1|arouezenn|arouezenn}} ennañ.",
'email'                      => 'Postel',
'prefs-help-realname'        => "* Hoc'h anv (diret) : ma vez spisaet ganeoc'h e vo implijet evit sinañ ho tegasadennoù.",
'loginerror'                 => 'Kudenn zisklêriañ',
'prefs-help-email'           => "* Postel (diret) : gantañ e vo aes mont e darempred ganeoc'h adal al lec'hienn o terc'hel kuzh ho chomlec'h, hag adkas ur ger-tremen deoc'h ma tichañsfe deoc'h koll ho hini.",
'prefs-help-email-required'  => "Ezhomm zo eus ur chomlec'h postel.",
'nocookiesnew'               => "krouet eo bet ar gont implijer met n'hoc'h ket luget. {{SITENAME}} a implij toupinoù evit al lugañ met diweredekaet eo an toupinoù ganeoc'h. Trugarez da weredekaat anezho ha d'en em lugañ en-dro.",
'nocookieslogin'             => "{{SITENAME}} a implij toupinoù evit al lugañ met diweredekaet eo an toupinoù ganeoc'h. Trugarez da weredekaat anezho ha d'en em lugañ en-dro.",
'noname'                     => "N'hoc'h eus lakaet anv implijer ebet.",
'loginsuccesstitle'          => "Disklêriet oc'h.",
'loginsuccess'               => 'Luget oc\'h bremañ war {{SITENAME}} evel "$1".',
'nosuchuser'                 => 'N\'eus ket eus an implijer "$1".
Gwiriit eo bet skrivet mat an anv ganeoc\'h pe implijit ar furmskrid a-is a-benn krouiñ ur gont implijer nevez.',
'nosuchusershort'            => "N'eus perzhiad ebet gantañ an anv « <nowiki>$1</nowiki> ». Gwiriit ar reizhskrivadur.",
'nouserspecified'            => "Ret eo deoc'h spisaat un anv implijer.",
'wrongpassword'              => 'Ger-tremen kamm. Klaskit en-dro.',
'wrongpasswordempty'         => 'Ger-tremen ebet. Lakait unan mar plij.',
'passwordtooshort'           => 'Fall eo her-tremen pe re verr eo. 
{{PLURAL:$1|1 arouezenn|$1 arouezenn}} a rank bezañ ennañ da nebeutañ ha disheñvel eus an anv implijer e rank bezañ ivez.',
'mailmypassword'             => 'Kasit din ur ger-tremen nevez',
'passwordremindertitle'      => 'Ho ker-tremen berrbad nevez evit {{SITENAME}}',
'passwordremindertext'       => "Unan bennak (c'hwi moarvat gant ar chomlec'h IP \$1)
en deus goulennet ma vo kaset deoc'h ur ger-tremen nevez evit mont war {{SITENAME}} (\$4).
Ger-tremen an implijer \"\$2\" zo bremañ \"\$3\".
Erbediñ a reomp deoc'h en em lugañ ha kemmañ ar ger-tremen-mañ an abretañ ar gwellañ.

Mard eo bet graet ar goulenn gant unan bennak all, pe m'hoc'h eus soñj eus ho ker-tremen bremañ ha
ne fell ket deoc'h cheñch anezhañ ken, e c'hallit leuskel ar postel-mañ a-gostez ha kenderc'hel d'ober gant ho ker-tremen kozh.",
'noemail'                    => 'N\'eus bet enrollet chomlec\'h elektronek ebet evit an implijer "$1".',
'passwordsent'               => 'Kaset ez eus bet ur ger-tremen nevez da chomlec\'h elektronek an implijer "$1".
Trugarez deoc\'h evit en em zisklêriañ kerkent ha ma vo bet resevet ganeoc\'h.',
'blocked-mailpassword'       => "N'haller ket degas kemmoù adal ar chomlec'h IP-mañ ken, gant se n'hallit ket implijout an arc'hwel adtapout gerioù-tremen, kuit m'en em ledfe kammvoazioù.",
'eauthentsent'               => "Kaset ez eus bet ur postel kadarnaat war-du ar chomlec'h postel spisaet.
A-raok na vije kaset postel ebet d'ar gont-se e vo ret deoc'h heuliañ ar c'huzulioù merket er postel resevet evit kadarnaat ez eo mat ho kont deoc'h.",
'throttled-mailpassword'     => "Kaset ez eus bet deoc'h ur postel degas soñj e-kerzh an
{{PLURAL:$1|eurvezh|$1 eurvezh}} ziwezhañ. Evit mirout ouzh nep gaou ne gaser nemet ur postel a-seurt-se dre {{PLURAL:$1|eurvezh|$1 eurvezh}}.",
'mailerror'                  => 'Fazi en ur gas ar postel : $1',
'acct_creation_throttle_hit' => "Ho tigarez, krouet ez eus bet $1 (c'h)gont ganeoc'h dija. N'hallit ket krouiñ unan nevez.",
'emailauthenticated'         => "Gwiriet eo bet ho chomlec'h postel d'an $1.",
'emailnotauthenticated'      => "N'eo ket bet gwiriekaet ho chomlec'h postel evit c'hoazh. Ne vo ket tu da gas postel ebet deoc'h evit hini ebet eus an dezverkoù dindan.",
'noemailprefs'               => "Merkit ur chomlec'h postel mar fell deoc'h ez afe an arc'hwelioù-mañ en-dro.",
'emailconfirmlink'           => "Kadarnait ho chomlec'h postel",
'invalidemailaddress'        => "N'haller ket degemer ar chomlec'h postel-mañ rak faziek eo e furmad evit doare. Merkit ur chomlec'h reizh pe goullonderit ar vaezienn-mañ.",
'accountcreated'             => 'Kont krouet',
'accountcreatedtext'         => 'Krouet eo bet kont implijer $1.',
'createaccount-title'        => 'Krouiñ ur gont war {{SITENAME}}',
'createaccount-text'         => 'Unan bennak en deus krouet ur gont gant ho chomlec\'h postel war {{SITENAME}} ($4) zo e anv "$2" hag a ra gant ar ger-tremen "$3".
Mat e vefe deoc\'h en em lugañ ha cheñch ho ker-tremen bremañ.

Na daolit ket evezh ouzh ar c\'hemenn-mañ m\'eo bet krouet ar gont dre fazi.',
'loginlanguagelabel'         => 'Yezh : $1',

# Password reset dialog
'resetpass'               => 'Adsevel ar ger-tremen',
'resetpass_announce'      => "En em enrollet oc’h dre ur ger-tremen da c'hortoz kaset deoc'h dre bostel. A-benn bezañ enrollet da vat e rankit spisaat ur ger-tremen nevez amañ :",
'resetpass_text'          => '<!-- Ouzhpennañ testenn amañ -->',
'resetpass_header'        => 'Adsevel ar ger-tremen',
'resetpass_submit'        => 'Kemmañ ar ger-tremen hag en em lugañ',
'resetpass_success'       => "Kemmet mat eo bet ho ker-temen ! Emaoc'h oc'h en em lugañ e-barzh...",
'resetpass_bad_temporary' => "Ger-tremen da c'hortoz faziek. Marteze hoc'h eus cheñchet ho ker-tremen dija pe hoc'h eus goulennet ur ger-tremen da c'hortoz all.",
'resetpass_forbidden'     => "N'haller ket cheñch ar gerioù-termen war {{SITENAME}}",
'resetpass_missing'       => "N'eus bet lakaet titour ebet.",

# Edit page toolbar
'bold_sample'     => 'Testenn dev',
'bold_tip'        => 'Testenn dev',
'italic_sample'   => 'Testenn italek',
'italic_tip'      => 'Testenn italek',
'link_sample'     => 'Liamm titl',
'link_tip'        => 'Liamm diabarzh',
'extlink_sample'  => 'http://www.example.com liamm titl',
'extlink_tip'     => 'Liamm diavaez (na zisoñjit ket http://)',
'headline_sample' => 'Testenn istitl',
'headline_tip'    => 'Istitl live 2',
'math_sample'     => 'Lakait ho formulenn amañ',
'math_tip'        => 'Formulenn jedoniel (LaTeX)',
'nowiki_sample'   => 'Lakait an destenn anfurmadet amañ',
'nowiki_tip'      => 'Na ober van da ereadur ar wiki',
'image_sample'    => 'Skouer.jpg',
'image_tip'       => 'Skeudenn enframmet',
'media_sample'    => 'Skouer.ogg',
'media_tip'       => 'Liamm restr media',
'sig_tip'         => 'Ho sinadur gant an deiziad',
'hr_tip'          => 'Liamm a-led (arabat implijout re)',

# Edit pages
'summary'                          => 'Diverrañ&nbsp;',
'subject'                          => 'Danvez/titl',
'minoredit'                        => 'Kemm dister',
'watchthis'                        => 'Evezhiañ ar pennad-mañ',
'savearticle'                      => 'Enrollañ',
'preview'                          => 'Rakwelet',
'showpreview'                      => 'Rakwelet',
'showlivepreview'                  => 'Rakwelet prim',
'showdiff'                         => "Diskouez ar c'hemmoù",
'anoneditwarning'                  => "'''Diwallit :''' N'oc'h ket luget. Ho chomlec'h IP eo a vo enrollet war istor kemmoù ar bajenn-mañ.",
'missingsummary'                   => "'''Taolit evezh:''' N'hoc'h eus ket lakaet tamm testenn diverrañ ebet evit ho kemmoù. Mar klikit war enrollañ en-dro, e vo enrollet ho testenn evel m'emañ hepmuiken.",
'missingcommenttext'               => "Skrivit hoc'h evezhiadenn a-is.",
'missingcommentheader'             => "'''Taolit evezh:''' N'hoc'h eus ket lakaet tamm danvez/titl ebet evit hoc'h evezhiadenn. Mar klikit war enrollañ en-dro, e vo enrollet ho testenn evel m'emañ hepmuiken.",
'summary-preview'                  => 'Rakwelet an diverrañ',
'subject-preview'                  => 'Rakwelet danvez/titl',
'blockedtitle'                     => 'Implijer stanket',
'blockedtext'                      => "<big>'''Stanket eo bet ho kont implijer pe ho chomlec'h IP'''</big>

Gant $1 eo bet graet. Setu an abeg : ''$2''.

* Stanket adalek : $8
* Stanket betek : $6
* Pad ar stankadenn : $7

Gallout a rit mont e darempred gant $1 pe gant unan eus ar [[{{MediaWiki:Grouppage-sysop}}|verourien]] all evit eskemm ganto war se. N'hallit implijout an arc'hwel 'kas ur postel d'an implijer-mañ' nemet ma'z eus bet spisaet ganeoc'h ur chomlec'h postel reizh en ho [[Special:Preferences|penndibaboù kont]] ha ma n'oc'h ket bet stanket. $3 eo ho chomlec'h IP, ha #$5 eo niverenn an identelezh stanket. Merkit anezho en ho koulennoù bep tro.",
'autoblockedtext'                  => "Stanket eo bet ho chomlec'h IP ent emgefreek rak implijet e veze gant un implijer all bet stanket gant \$1.
Setu aze an abeg :

:''\$2''

* Deroù ar stankadenn : \$8
* Termen ar stankadenn : \$6

Gallout a rit mont e darempred gant \$1 pe gant unan eus ar
[[{{MediaWiki:Grouppage-sysop}}|verourien]] all ma kavit abeg er stankadenn.

Notennit mat ne c'hallot implijout an dibarzh \"kas ur postel d'an implijer\" nemet ma'z eus bet merket ganeoc'h ur chomlec'h postel reizh en ho [[Special:Preferences|penndibaboù implijer]] ha ma n'eo ket bet stanket ivez.

\$5 eo ho niverenn stankadenn. Merkit mat an niverenn-se pa rit goulennoù.",
'blockednoreason'                  => "n'eus bet roet abeg ebet",
'blockedoriginalsource'            => "Kavout a reot mammenn '''$1''' a-is:",
'blockededitsource'                => "Kavout a reot testenn ho '''kemmoù''' war '''$1''' a-is :",
'whitelistedittitle'               => 'Ret eo bezañ luget evit skridaozañ',
'whitelistedittext'                => "Ret eo deoc'h en em $1 evit gallout skridaozañ.",
'confirmedittitle'                 => "Ret eo kadarnaat e chomlec'h postel a-raok kemmañ pajennoù",
'confirmedittext'                  => "Rankout a ri bezañ kadarnaet ho chomlec'h postel a-raok gellout degas kemmoù er pajennoù. Skrivit ha kadarnait ho chomlec'h postel en ho [[Special:Preferences|penndibaboù implijer]] mar plij.",
'nosuchsectiontitle'               => "N'eus ket eus ar rann-mañ",
'nosuchsectiontext'                => "Klasket hoc'h eus degas kemmoù en ur rann n'eus ket anezhi. Dre ma n'eus rann $1 ebet, n'eus lec'h ebet da enrollañ ho kemmoù.",
'loginreqtitle'                    => 'Anv implijer rekis',
'loginreqlink'                     => 'en em lugañ',
'loginreqpagetext'                 => "Ret eo deoc'h $1 evit gwelet pajennoù all.",
'accmailtitle'                     => 'Ger-tremen kaset.',
'accmailtext'                      => 'Kaset eo bet ger-tremen « $1 » da $2.',
'newarticle'                       => '(Nevez)',
'newarticletext'                   => 'Skrivit amañ testenn ho pennad.',
'anontalkpagetext'                 => "---- ''Homañ eo ar bajenn gaozeal evit un implijer(ez) dianv n'eus ket c'hoazh krouet kont ebet pe na implij ket anezhi. Setu perak e rankomp ober gant ar chomlec'h IP niverel evit disklêriañ anezhañ/i. Gallout a ra ur chomlec'h a seurt-se bezañ rannet etre meur a implijer(ez). Ma'z oc'h un implijer(ez) dianv ha ma stadit ez eus bet kaset deoc'h kemennadennoù na sellont ket ouzhoc'h, gallout a rit [[Special:UserLogin|krouiñ ur gont pe en em lugañ]] kuit a vagañ muioc'h a gemmesk.",
'noarticletext'                    => "N'eus evit poent tamm skrid ebet war ar bajenn-mañ, gallout a rit [[Special:Search/{{PAGENAME}}|klask an titl anezhañ]] war pajennoù all pe [{{fullurl:{{FULLPAGENAME}}|action=edit}} kemmañ ar bajenn diouzhtu].",
'userpage-userdoesnotexist'        => 'N\'eo ket enrollet ar gont "$1". Merkit ma fell deoc\'h krouiñ/kemmañ ar bajenn-mañ.',
'clearyourcache'                   => "'''Notenn :''' Goude bezañ enrollet ho pajenn e rankot freskaat krubuilh ho merdeer a-bennn gwelet ar c'hemmoù : '''Mozilla / Firefox / Safari : ''' dalc'hit ''Pennlizherenn'' en ur glikañ war ''Adkargañ'', pe pouezañ war ''Ctrl-F5'' pe ''Ctrl-R'' (''Command-R'' war ur Macintosh); '''Konqueror: '''klikañ war ''Adkargañ'' pe pouezañ war ''F5;'' '''Opera:''' riñsañ ar grubuilh e ''Ostilhoù → Penndibaboù;'' '''Internet Explorer:''' derc'hel ''Ctrl'' en ur glikañ war ''Freskaat,'' pe pouezañ war ''Ctrl-F5.''",
'usercssjsyoucanpreview'           => "<strong>Tun :</strong> Grit gant ar bouton '''Rakwelet''' evit testiñ ho follenn CSS/JS nevez a-raok enrollañ anezhi.",
'usercsspreview'                   => "'''Dalc'hit soñj n'emaoc'h nemet o rakwelet ho follenn CSS deoc'h ha n'eus ket bet enrollet netra c'hoazh!'''",
'userjspreview'                    => "'''Dalc'hit soñj emaoc'h o rakwelet pe o testiñ ho kod javascript deoc'h ha n'eo ket bet enrollet c'hoazh!'''",
'userinvalidcssjstitle'            => "'''Diwallit:''' N'eus tamm gwiskadur \"\$1\" ebet. Ho pez soñj e vez implijet lizherennoù bihan goude an anv implijer hag ar veskell / gant ar pajennoù personel dezho un astenn .css ha .js; da skouer eo mat ar follenn stil {{ns:user}}:Foo/monobook.css ha faziek an hini {{ns:user}}:Foo/Monobook.css.",
'updated'                          => '(Hizivaet)',
'note'                             => '<strong>Notenn :</strong>',
'previewnote'                      => "<strong>Diwallit mat, n'eo homañ nemet ur rakweladenn, n'eo ket enrollet an destenn c'hoazh!</strong>",
'previewconflict'                  => 'Gant ar rakweladenn e teu testenn ar bajenn war wel evel ma vo pa vo bet enrollet.',
'session_fail_preview'             => "<strong>Ho tigarez! N'eus ket bet tu da enrollañ ho kemmoù rak kollet eo bet roadennoù an dalc'h. Klaskit en-dro mar plij. Ma ne'z a ket en-dro c'hoazh, klaskit dilugañ ho kont ha lugañ en-dro.</strong>",
'session_fail_preview_html'        => "<strong>Ho tigarez! N'omp ket bet gouest da enrollañ ho kemmoù rak kollet ez eus bet roadennoù e-kerzh an dalc'h.</strong>

''Gweredekaet eo al linenoù HTML e {{SITENAME}}. Rak-se eo kuzh ar rakweladennoù a-benn en em zifenn diouzh an tagadennoù JavaScript.''

<strong>Mard e oa onest ar c'hemmoù oc'h eus klasket degas, klaskit en-dro. Mar ned a ket en-dro, klaskit dilugañ ha lugañ en-dro (pe digevreañ/kevreañ).</strong>",
'token_suffix_mismatch'            => "<strong>Distaolet eo bet ar c'hemmoù degaset ganeoc'h abalamour ma oa bet kemmesket an arouezennoù poentadur gant ho merdeer en daveer kemmañ. Distaolet eo bet ar c'hemmoù kuit na vije breinet ar bajennad skrid.
C'hoarvezout a ra a-wechoù pa implijit ur servijer proksi dreinek dizanav.</strong>",
'editing'                          => "oc'h aozañ $1",
'editingsection'                   => "oc'h aozañ $1 (rann)",
'editingcomment'                   => "oc'h aozañ $1 (soñj)",
'editconflict'                     => 'tabut kemmañ : $1',
'explainconflict'                  => "<b>Enrollet eo bet ar bajenn-mañ war-lerc'h m'ho pefe kroget d'he c'hemmañ.
E-krec'h an takad aozañ emañ an destenn evel m'emañ enrollet bremañ er bank roadennoù. Ho kemmoù deoc'h a zeu war wel en takad aozañ traoñ. Ret e vo deoc'h degas ho kemmoù d'an destenn zo evit poent. N'eus nemet an destenn zo en takad krec'h a vo saveteet.</b><br />",
'yourtext'                         => 'Ho testenn',
'storedversion'                    => 'Stumm enrollet',
'nonunicodebrowser'                => "<strong>DIWALLIT: N'eo ket skoret an Unicode gant ho merdeer. Un diskoulm da c'hortoz zo bet kavet evit ma c'hallfec'h degas kemmoù er pennadoù : dont a raio war wel an arouezennoù an-ASCII er prenestr skridaozañ evel kodoù eizhdekvedennel.</strong>",
'editingold'                       => "<strong>Diwallit : o kemm ur stumm kozh eus ar bajenn-mañ emaoc'h. Mard enrollit bremañ e vo kollet an holl gemmoù bet graet abaoe ar stumm-se.</strong>",
'yourdiff'                         => "Diforc'hioù",
'copyrightwarning'                 => "Sellet e vez ouzh an holl degasadennoù graet war {{SITENAME}} evel ouzh degasadennoù a zouj da dermenoù ar $2 (Sellet ouzh $1 evit gouzout hiroc'h). Mar ne fell ket deoc'h e vefe embannet ha skignet ho skridoù, arabat kas anezho.<br />
Heñveldra, prometiñ a rit kemer perzh dre zegas skridoù savet ganeoc'h hepken pe tennet eus ur vammenn frank a wirioù.
<strong>NA IMPLIJIT KET LABOURIOÙ GANT GWIRIOÙ AOZER (COPYRIGHT) HEP AOTRE D'OBER KEMENT-SE!</strong>",
'copyrightwarning2'                => "Notit mat e c'hall kement degasadenn graet ganeoc'h war {{SITENAME}} bezañ kemmet, adaozet pe lamet kuit gant an implijerien all. Mar ne fell ket deoc'h e vije kemmet-digemmet ar pezh hoc'h eus skrivet na gemerit ket perzh er raktres-mañ.<br /> Gouestlañ a rit ivez eo bet savet ar boued spered ganeoc'h pe eilet diwar ur vammenn frank a wirioù pe en domani foran (gwelet $1 evit gouzout hiroc'h). <strong>NA IMPLIJIT KET LABOURIOÙ GANT GWIRIOÙ AOZER HEP AOTRE D'OBER KEMENT-SE!</strong>",
'longpagewarning'                  => "<strong>KEMENN DIWALL: $1 ko eo hed ar bajenn-mañ;
merdeerioù zo o deus poan da verañ ar pajennoù tro-dro pe en tu all da 32 ko pa vezont savet.
Marteze e c'hallfec'h rannañ ar bajenn e rannoù bihanoc'h.</strong>",
'longpageerror'                    => "<strong>FAZI: $1 kilobit hir eo an destenn lakaet ganeoc'h, ar pezh zo hiroc'h eget $2 kilobit, ar vent vrasañ aotreet. N'haller ket enrollañ.</strong>",
'readonlywarning'                  => "<strong>KEMENN DIWALL: stanket eo bet ar bajenn-mañ evit bezañ trezalc'het,
n'oc'h ket evit enrollañ ho kemmoù diouzhtu eta. Gallout a rit eilañ an destenn en ur restr hag enrollañ anezhi diwezhatoc'hik.</strong>",
'protectedpagewarning'             => "<strong>KEMENN DIWALL: Stanket eo bet ar bajenn-mañ.
N'eus nemet an implijerien ganto ar statud merour a c'hall degas kemmoù enni.</strong>",
'semiprotectedpagewarning'         => "''Notenn :''' Gwarezet eo ar bajenn-mañ; n'eus nemet an implijerien bet krouet ur gont ganto a c'hall degas kemmoù enni.",
'cascadeprotectedwarning'          => "'''Diwallit :''' Prennet eo ar bajenn-mañ. N'eus nemet ar verourien a c'hall degas kemmoù enni peogwir he c'haver e-touez ar {{PLURAL:\$1|bajenn|pajennoù}} da-heul zo bet gwarezet en ur zibab an dibarzh \"skalierad\" :",
'titleprotectedwarning'            => "<strong>WARNING:  Gwarezet eo bet ar bajenn-mañ e doare ma n'hallo bezañ krouet nemet gant implijerien zo.</strong>",
'templatesused'                    => 'Patromoù implijet war ar bajenn-mañ :',
'templatesusedpreview'             => 'Patromoù implijet er rakweladenn-mañ :',
'templatesusedsection'             => 'Patromoù implijet er rann-mañ :',
'template-protected'               => '(gwarezet)',
'template-semiprotected'           => '(damwarezet)',
'hiddencategories'                 => "{{PLURAL:$1|1 rummad kuzhet|$1 rummad kuzhet}} m'emañ rollet ar bajenn-mañ :",
'edittools'                        => '<!-- Diskouezet e vo an destenn kinniget amañ dindan ar sternioù kemmañ ha kargañ. -->',
'nocreatetitle'                    => "Strishaet eo bet ar c'hrouiñ pajennoù",
'nocreatetext'                     => 'Strishaet eo bet an tu da grouiñ pajennoù nevez war {{SITENAME}}.
Gallout a rit mont war-gil ha degas kemmoù en ur bajenn zo anezhi dija, pe [[Special:UserLogin|en em enrollañ ha krouiñ ur gont]].',
'nocreate-loggedin'                => "N'oc'h ket aotreet da grouiñ pajennoù nevez war {{SITENAME}}.",
'permissionserrors'                => 'Fazioù Aotre',
'permissionserrorstext'            => "N'oc'h ket aotreet d'ober kement-mañ evit {{PLURAL:$1|an abeg-mañ|an abegoù-mañ}} :",
'permissionserrorstext-withaction' => "N'oc'h ket aotreet da $2, evit an {{PLURAL:$1|abeg-mañ|abeg-mañ}} :",
'recreate-deleted-warn'            => "'''Diwallit : Emaoc'h o krouiñ ur bajenn zo bet diverket c'hoazh.'''

En em soñjit ervat ha talvoudus eo kenderc'hel krouiñ ar bajenn.
Deoc'h da c'houzout, aze emañ marilh an diverkadenn :",

# Parser/template warnings
'expensive-parserfunction-warning'        => "Diwallit : Re a c'halvoù koustus e-keñver an arc'hwelioù parser zo gant ar bajenn-mañ.

Dleout a rafe bezañ nebeutoc'h eget $2, ha $1 zo.",
'expensive-parserfunction-category'       => "Pagjennoù enno re a c'halvoù koustus e-keñver an arc'hwelioù parser.",
'post-expand-template-inclusion-warning'  => 'Diwallit : re a batromoù zo war ar bajenn-mañ. 
Lod anezho a vo lakaet a-gostez.',
'post-expand-template-inclusion-category' => 'Pajennoù enno re a batromoù',
'post-expand-template-argument-warning'   => 'Diwallit : war ar bajenn-mañ ez eus eus da nebeutañ un arventenn eus ur patrom zo re vras.
A-gostez eo bet lezet an arventenn-se.',
'post-expand-template-argument-category'  => 'Pajennoù enno arventennoù patrom bet lezet a-gostez',

# "Undo" feature
'undo-success' => "Gallout a reer disteurel ar c'hemmoù-mañ. Gwiriit, mar plij, gant ar geñveriadenn a-is evit bezañ sur eo an dra-se a fell deoc'h ober; goude-se enrollit ar c'hemmoù a-is a-benn echuiñ disteurel ar c'hemmoù.",
'undo-failure' => "N'eus ket bet tu da zisteuler ar c'hemm-mañ abalamour d'un tabut gant kemmoù degaset e-keit-se.",
'undo-norev'   => "N'eus ket bet gallet degas ar c'hemmoù-mañ rak pe n'eus ket anezho pe int bet diverket.",
'undo-summary' => 'Disteurel kemmoù $1 a-berzh [[Special:Contributions/$2]] ([[User talk:$2]])',

# Account creation failure
'cantcreateaccounttitle' => 'Dibosupl krouiñ ar gont',
'cantcreateaccount-text' => "Stanket eo bet ar c'hrouiñ kontoù adal ar chomlec'h IP ('''$1''') gant [[User:$3|$3]].

An abeg roet gant $3 zo ''$2''",

# History pages
'viewpagelogs'        => 'Gwelet ar marilhoù evit ar bajenn-mañ',
'nohistory'           => "Ar bajenn-mañ n'he deus tamm istor ebet.",
'revnotfound'         => "N'eo ket bet kavet ar stumm-mañ",
'revnotfoundtext'     => "N'eo ket bet kavet stumm kent ar bajenn-mañ. Gwiriit an URL lakaet ganeoc'h evit mont d'ar bajenn-mañ.",
'currentrev'          => 'Stumm a-vremañ pe stumm red',
'revisionasof'        => 'Stumm eus an $1',
'revision-info'       => 'Stumm eus an $1 gant $2',
'previousrevision'    => '← Stumm kent',
'nextrevision'        => "Stumm war-lerc'h →",
'currentrevisionlink' => 'Gwelet ar stumm red',
'cur'                 => 'red',
'next'                => 'goude',
'last'                => 'diwez',
'page_first'          => 'kentañ',
'page_last'           => 'diwezhañ',
'histlegend'          => "Sellet ouzh an diforc'hioù : lakait un ask adal d'ar stummoù a fell deoc'h keñveriañ ha pouezit war kadarnaat pe war ar bouton en traoñ.<br />
Alc'hwez : (red) = diforc'hioù gant ar stumm a-vremañ,
(diwez) = diforc'hioù gant ar stumm kent, D = kemm dister",
'deletedrev'          => '[diverket]',
'histfirst'           => 'Kentañ',
'histlast'            => 'Diwezhañ',
'historysize'         => '({{PLURAL:$1|$1 okted|$1 okted}})',
'historyempty'        => '(goullo)',

# Revision feed
'history-feed-title'          => "Istor ar c'hemmoù",
'history-feed-description'    => "Istor ar c'hemmoù degaset war ar bajenn-mañ eus ar wiki",
'history-feed-item-nocomment' => "$1 d'an $2", # user at time
'history-feed-empty'          => "Ar bajenn goulennet n'eus ket anezhi.
Marteze eo bet diverket eus ar wiki, pe adanvet.
Implijit [[Special:Search|klaskit er wiki]] evit kavout pajennoù all a c'hallfe klotañ.",

# Revision deletion
'rev-deleted-comment'         => '(evezhiadenn diverket)',
'rev-deleted-user'            => '(anv implijer diverket)',
'rev-deleted-event'           => '(elfenn dilamet)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Lamet eo bet kuit ar stumm-mañ eus ar bajenn eus an dielloù foran.
Marteze e kavot munudoù war [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} roll ar pajennoù diverket].
</div>',
'rev-deleted-text-view'       => "<div class=\"mw-warning plainlinks\">
Lamet eo bet kuit ar stumm-mañ eus ar bajenn diouzh an dielloù foran.
Dre ma'z oc'h merour war al lec'hienn-mañ e c'hallit sellet outañ;
Marteze e kavot munudoù all war [{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} roll ar pajennoù diverket].
</div>",
'rev-delundel'                => 'diskouez/kuzhat',
'revisiondelete'              => 'Diverkañ/diziverkañ stummoù',
'revdelete-nooldid-title'     => "N'eus stumm pal ebet evit an degasadennoù",
'revdelete-nooldid-text'      => "Pe n'eo ket bet spisaet ganeoc'h ar stumm(où) pal da implijout an arc'hwel-mañ evito. pe n'eus ket eus ar stummoù spisaet, pe emaoc'h o klask kuzhat ar stumm red.",
'revdelete-selected'          => '{{PLURAL:$2|Stumm dibabet|Stummoù dibabet}} eus [[:$1]] :',
'logdelete-selected'          => '{{PLURAL:$1|Darvoud eus ar marilh diuzet|Darvoud eus ar marilh diuzet}} :',
'revdelete-text'              => "Derc'hel a raio ar stummoù diverket da zont war wel war istor ar bajenn,
met n'hallo ket an dud sellet outo.

Gouest e vo merourien all {{SITENAME}} da dapout krog en testennoù kuzhet ha da ziziverkañ anezho en-dro dre an hevelep etrefas, nemet e vije bet lakaet e plas strishadurioù ouzhpenn gant renerien al lec'hienn.",
'revdelete-legend'            => 'Lakaat strishadurioù gwelet',
'revdelete-hide-text'         => 'Kuzhat testenn ar stumm',
'revdelete-hide-name'         => 'Kuzhat an ober hag ar vukadenn',
'revdelete-hide-comment'      => "Kuzhat notenn ar c'hemm",
'revdelete-hide-user'         => "Kuzhat anv implijer pe chomlec'h IP an aozer",
'revdelete-hide-restricted'   => 'Lakaat ar strishadurioù-mañ da dalvezout evit ar verourien ha prennañ an etrefas',
'revdelete-suppress'          => 'Diverkañ roadennoù ar verourien hag ar re all',
'revdelete-hide-image'        => 'Kuzhat danvez ar restr',
'revdelete-unsuppress'        => 'Lemel ar strishadurioù war ar stummoù assavet',
'revdelete-log'               => 'Notenn evit ar marilh :',
'revdelete-submit'            => 'Lakaat da dalvezout evit ar stumm diuzet',
'revdelete-logentry'          => 'Cheñchet eo bet hewelusted ar stumm evit [[$1]]',
'logdelete-logentry'          => 'kemmet eo bet gwelusted an darvoud evit [[$1]]',
'revdelete-success'           => "'''Gwelusted ar stummoù kaset da benn vat.'''",
'logdelete-success'           => "'''Gwelusted an darvoud kaset da benn vat.'''",
'revdel-restore'              => 'Cheñch ar gweluster',
'pagehist'                    => 'Istor ar bajenn',
'deletedhist'                 => 'Diverkañ an istor',
'revdelete-content'           => 'danvez',
'revdelete-summary'           => 'kemmañ an diverrañ',
'revdelete-uname'             => 'anv implijer',
'revdelete-restricted'        => 'Lakaat ar strishadurioù da dalvezout evit ar verourien',
'revdelete-unrestricted'      => 'dilemel ar strishadurioù evit ar verourien',
'revdelete-hid'               => 'kuzhat $1',
'revdelete-unhid'             => 'diguzhat $1',
'revdelete-log-message'       => '$1 evit $2 {{PLURAL:$2|adweladenn|adweladenn}}',
'logdelete-log-message'       => '$1 evit $2 {{PLURAL:$2|darvoud|darvoud}}',

# Suppression log
'suppressionlog'     => 'Marilh diverkañ',
'suppressionlogtext' => 'A-is emañ roll an diverkadennoù hag ar stankadennoù diwezhañ enno an adweladennoù kuzhet ouzh ar verourien. Gwelet [[Special:IPBlockList|roll an IPoù stanket]] evit kaout roll ar stankadennoù ha forbannadennoù e talvoud evit poent.',

# History merging
'mergehistory'                     => 'Kendeuziñ istor ur bajenn',
'mergehistory-header'              => "Gant ar bajenn-mañ e c'hallit kendeuziñ an adweladennoù c'hoarvezet da istor ur bajenn war-du unan nevez.
Gwiriit ne vo ket torret red istor ar bajenn gant ar c'hemm-mañ.",
'mergehistory-box'                 => 'Kendeuziñ istor div bajenn :',
'mergehistory-from'                => 'Pajenn orin :',
'mergehistory-into'                => 'Pajenn dal :',
'mergehistory-list'                => 'Aozañ an istorioù da gendeuziñ',
'mergehistory-merge'               => 'Gallout a reer kendeuziñ ar stummoù da-heul eus [[:$1]] e [[:$2]]. Na implijit bouton radio ar bann nemet evit kendeuziñ ar stummoù bet krouet en deroù betek an deiziad merket. Notit mat e vo nevesaet ar bann mard implijit al liammoù merdeiñ.',
'mergehistory-go'                  => "Diskouez ar stummoù a c'haller kendeuziñ",
'mergehistory-submit'              => 'Kendeuziñ ar stummoù',
'mergehistory-empty'               => "N'haller ket kendeuziñ stumm ebet.",
'mergehistory-success'             => 'Kendeuzet ez eus bet $3 {{PLURAL:$3|stumm|stumm}} eus [[:$1]] e [[:$2]].',
'mergehistory-fail'                => 'Dibosupl kendeuziñ an istorioù. Gwiriit ar bajenn hag arventennoù an deiziadoù.',
'mergehistory-no-source'           => "N'eus ket eus ar bajenn orin $1.",
'mergehistory-no-destination'      => "N'eus ket eus ar bajenn dal $1.",
'mergehistory-invalid-source'      => 'Ret eo da anv ar bajenn orin bezañ reizh.',
'mergehistory-invalid-destination' => 'Ret eo da anv ar bajenn dal bezañ reizh.',
'mergehistory-autocomment'         => 'Kendeuzet [[:$1]] gant [[:$2]]',
'mergehistory-comment'             => 'Kendeuzet [[:$1]] gant [[:$2]] : $3',

# Merge log
'mergelog'           => "Marilh ar c'hendeuzadennoù.",
'pagemerge-logentry' => 'kendeuzet [[$1]] gant [[$2]] (stummoù betek an $3)',
'revertmerge'        => "Nullañ ar c'hendeuziñ",
'mergelogpagetext'   => 'Setu aze roll kendeuzadennoù diwezhañ un eil pajenn istor gant eben.',

# Diffs
'history-title'           => 'Istor stummoù disheñvel "$1"',
'difference'              => "(Diforc'hioù etre ar stummoù)",
'lineno'                  => 'Linenn $1:',
'compareselectedversions' => 'Keñveriañ ar stummoù diuzet',
'editundo'                => 'disteuler',
'diff-multi'              => "({{PLURAL:$1|Ur reizhadenn da c'hortoz|$1 reizhadenn da c'hortoz}} kuzhet.)",

# Search results
'searchresults'             => "Disoc'h ar c'hlask",
'searchresulttext'          => "Evit kaout muioc'h a ditouroù diwar-benn ar c'hlask e {{SITENAME}}, sellet ouzh [[{{MediaWiki:Helppage}}|{{int:help}}]].",
'searchsubtitle'            => 'Evit ar goulenn "[[:$1]]"',
'searchsubtitleinvalid'     => 'Evit ar goulenn "$1"',
'noexactmatch'              => "'''N'eus pajenn ebet anvet \"\$1\".''' Gallout a rit [[:\$1|krouiñ ar bajenn]].",
'noexactmatch-nocreate'     => "'''N'eus pajenn ebet anvet \"\$1\".'''",
'toomanymatches'            => 'Re a respontoù a glot gant ar goulenn, klaskit gant ur goulenn all',
'titlematches'              => 'Klotadurioù gant an titloù',
'notitlematches'            => "N'emañ ar ger(ioù) goulennet e titl pennad ebet",
'textmatches'               => 'Klotadurioù en testennoù',
'notextmatches'             => "N'emañ ar ger(ioù) goulennet e testenn pennad ebet",
'prevn'                     => '$1 kent',
'nextn'                     => "$1 war-lerc'h",
'viewprevnext'              => 'Gwelet ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|1 ger|$2 ger}})',
'search-result-score'       => 'Klotadusted : $1%',
'search-redirect'           => '(adkas $1)',
'search-section'            => '(rann $1)',
'search-suggest'            => "N'hoc'h eus ket soñjet kentoc'h e : $1",
'search-interwiki-caption'  => 'Raktresoù kar',
'search-interwiki-default'  => "$1 disoc'h :",
'search-interwiki-more'     => "(muioc'h)",
'search-mwsuggest-enabled'  => 'gant kinnigoù',
'search-mwsuggest-disabled' => 'hep kinnigoù',
'search-relatedarticle'     => "Disoc'hoù kar",
'mwsuggest-disable'         => 'Diweredekaat kinnigoù AJAX',
'searchrelated'             => "disoc'hoù kar",
'searchall'                 => 'An holl',
'showingresults'            => "Diskouez betek {{PLURAL:$1|'''1''' disoc'h|'''$1''' disoc'h}} o kregiñ gant #'''$2'''.",
'showingresultsnum'         => "Diskouez {{PLURAL:$3|'''1''' disoc'h|'''$3''' disoc'h}} o kregiñ gant #'''$2'''.",
'showingresultstotal'       => "O tiskouez a-is {{PLURAL:$3|disoc'h '''$1''' diwar '''$3'''|disoc'hoù '''$1 - $2''' diwar ''$3'''}}",
'nonefound'                 => "'''Notenn''' : dre ziouer ne glasker nemet en esaouennoù anv zo. Klaskit spisaat '''holl :''' evit klask e pep lec'h (e-barzh ar pajennoù-kaozeal, ar patromoù, hag all), pe dibabit an esaouenn anv a zere.",
'powersearch'               => 'Klask',
'powersearch-legend'        => 'Klask araokaet',
'powersearch-ns'            => 'Klask en esaouennoù anv :',
'powersearch-redir'         => 'Roll an adkasoù',
'powersearch-field'         => 'Klask',
'search-external'           => 'Klask diavaez',
'searchdisabled'            => "<p>Diweredekaet eo bet an arc'hwel klask war an destenn a-bezh evit ur frapad rak ur samm re vras e oa evit ar servijer. Emichañs e vo tu d'e adlakaat pa vo ur servijer galloudusoc'h ganeomp. Da c'hortoz e c'hallit klask gant Google:</p>",

# Preferences page
'preferences'              => 'Penndibaboù',
'mypreferences'            => 'penndibaboù',
'prefs-edits'              => 'Niver a gemmoù :',
'prefsnologin'             => 'Diluget',
'prefsnologintext'         => "ret eo deoc'h bezañ [[Special:UserLogin|luget]] evit kemm ho tibaboù implijer.",
'prefsreset'               => 'Adlakaet eo bet ar penndibaboù diouzh ar stumm bet enrollet.',
'qbsettings'               => 'Personelaat ar varrenn ostilhoù',
'qbsettings-none'          => 'Hini ebet',
'qbsettings-fixedleft'     => 'Kleiz',
'qbsettings-fixedright'    => 'Dehou',
'qbsettings-floatingleft'  => 'War-neuñv a-gleiz',
'qbsettings-floatingright' => 'War-neuñv a-zehou',
'changepassword'           => 'Kemmañ ar ger-tremen',
'skin'                     => 'Gwiskadur',
'math'                     => 'Tres jedoniel',
'dateformat'               => 'Stumm an deiziad',
'datedefault'              => 'Dre ziouer',
'datetime'                 => 'Deiziad hag eur',
'math_failure'             => 'Fazi jedoniezh',
'math_unknown_error'       => 'fazi dianav',
'math_unknown_function'    => 'kevreizhenn jedoniel dianav',
'math_lexing_error'        => 'fazi ger',
'math_syntax_error'        => 'fazi ereadur',
'math_image_error'         => "C'hwitet eo bet ar gaozeadenn e PNG, gwiriit staliadur Latex, dvips, gs ha convert",
'math_bad_tmpdir'          => "N'hall ket krouiñ pe skrivañ er c'havlec'h da c'hortoz",
'math_bad_output'          => "N'hall ket krouiñ pe skrivañ er c'havlec'h ermaeziañ",
'math_notexvc'             => "N'hall ket an erounezeg 'texvc' bezañ kavet. Lennit math/README evit he c'hefluniañ.",
'prefs-personal'           => 'Titouroù personel',
'prefs-rc'                 => 'Kemmoù diwezhañ',
'prefs-watchlist'          => 'Roll evezhiañ',
'prefs-watchlist-days'     => 'Niver a zevezhioù da ziskouez er rollad evezhiañ :',
'prefs-watchlist-edits'    => 'Niver a gemmoù da ziskouez er roll evezhiañ astennet :',
'prefs-misc'               => 'Penndibaboù liesseurt',
'saveprefs'                => 'Enrollañ ar penndibaboù',
'resetprefs'               => 'Adlakaat ar penndibaboù kent',
'oldpassword'              => 'Ger-tremen kozh',
'newpassword'              => 'Ger-tremen nevez :',
'retypenew'                => 'Kadarnaat ar ger-tremen nevez',
'textboxsize'              => 'Prenestr skridaozañ',
'rows'                     => 'Linennoù :',
'columns'                  => 'Bannoù',
'searchresultshead'        => 'Enklaskoù',
'resultsperpage'           => 'Niver a respontoù dre bajenn :',
'contextlines'             => 'Niver a linennoù dre respont',
'contextchars'             => 'Niver a arouezennoù kendestenn dre linenn',
'stub-threshold'           => 'Bevenn uhelañ evit al <a href="#" class="stub">liammoù war-du an danvez pennadoù</a> (okted) :',
'recentchangesdays'        => "Niver a zevezhioù da ziskouez er c'hemmoù diwezhañ :",
'recentchangescount'       => "Niver a ditloù er c'hemmoù diwezhañ",
'savedprefs'               => 'Enrollet eo bet ar penndibaboù.',
'timezonelegend'           => 'Takad eur',
'timezonetext'             => "Mar ne resisait ket al linkadur eur e vo graet gant eur Europa ar C'hornôg dre ziouer.",
'localtime'                => "Eur lec'hel",
'timezoneoffset'           => 'Linkadur eur',
'servertime'               => 'Eur ar servijer',
'guesstimezone'            => 'Ober gant talvoudenn ar merdeer',
'allowemail'               => 'Aotren ar posteloù a-berzh implijerien all',
'prefs-searchoptions'      => 'Dibarzhioù klask',
'prefs-namespaces'         => 'Esaouennoù',
'defaultns'                => 'Klask en esaouennoù-mañ dre ziouer :',
'default'                  => 'dre ziouer',
'files'                    => 'Restroù',

# User rights
'userrights'                  => 'Merañ statud an implijerien', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Merañ strolladoù an implijer',
'userrights-user-editname'    => 'Lakait un anv implijer :',
'editusergroup'               => 'Kemmañ ar strolladoù implijerien',
'editinguser'                 => "Kemmañ gwirioù an implijer '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Kemmañ strolladoù an implijer',
'saveusergroups'              => 'Enrollañ ar strolladoù implijer',
'userrights-groupsmember'     => 'Ezel eus :',
'userrights-groups-help'      => "Cheñch strollad an implijer a c'hallit ober.
* Ul log asket a verk emañ an implijer er strollad.
* Ul log diask a verk n'emañ ket an implijer er strollad.
* Ur * a verk n'hallit ket dilemel ar strollad ur wech bet ouzhpennet, pe ar c'hontrol.",
'userrights-reason'           => 'Abeg kemmañ :',
'userrights-no-interwiki'     => "N'oc'h ket aotreet da gemmañ ar gwirioù implijer war wikioù all.",
'userrights-nodatabase'       => "N'eus ket eus an diaz titouroù $1 pe n'eo ket lec'hel.",
'userrights-nologin'          => "Ret eo deoc'h [[Special:UserLogin|bezañ enrollet]] gant ur gont merour a-benn reiñ gwirioù implijer.",
'userrights-notallowed'       => "N'eo ket aotreet ho kont da reiñ gwirioù implijer.",
'userrights-changeable-col'   => "Ar strolladoù a c'hallit cheñch",
'userrights-unchangeable-col' => "Ar strolladoù n'hallit ket cheñch",

# Groups
'group'               => 'Strollad :',
'group-user'          => 'Implijerien',
'group-autoconfirmed' => 'Implijerien bet kadarnaet en emgefre',
'group-bot'           => 'Botoù',
'group-sysop'         => 'Merourien',
'group-bureaucrat'    => 'Pennoù-bras',
'group-suppress'      => 'Dindan evezh',
'group-all'           => '(pep tra)',

'group-user-member'          => 'Implijer',
'group-autoconfirmed-member' => 'Implijer bet kadarnaet ent emgefre',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Merour',
'group-bureaucrat-member'    => 'Penn-bras',
'group-suppress-member'      => 'Dindan evezh',

'grouppage-user'          => '{{ns:project}}:Implijerien',
'grouppage-autoconfirmed' => '{{ns:project}}: Implijerien bet kadarnaet ent emgefre',
'grouppage-bot'           => '{{ns:project}}:Botoù',
'grouppage-sysop'         => '{{ns:project}}:Merourien',
'grouppage-bureaucrat'    => '{{ns:project}}: Pennoù-bras',
'grouppage-suppress'      => '{{ns:project}}: Dindan evezh',

# Rights
'right-read'             => 'Lenn ar pajennoù',
'right-edit'             => 'Kemmañ ar pajennoù',
'right-createpage'       => 'Krouiñ pajennoù (estreget pajennoù kaozeal)',
'right-createtalk'       => 'Krouiñ pajennoù kaozeal',
'right-createaccount'    => 'Krouiñ kontoù implijer nevez',
'right-minoredit'        => "Merkañ ar c'hemmoù evel kemmoù dister",
'right-move'             => 'Adenvel pajennoù',
'right-suppressredirect' => 'Chom hep sevel un adkas adalek ar bajenn gozh en ur adenvel ar bajenn',
'right-upload'           => 'Enporzhiañ restroù',
'right-reupload'         => 'Frikañ ur restr zo anezhi dija',
'right-reupload-own'     => 'Frikañ ur restr bet pellgarget gant an-unan',
'right-autoconfirmed'    => 'Kemmañ ar pajennoù damwarezet',
'right-delete'           => 'Diverkañ pajennoù',
'right-browsearchive'    => 'Klask pajennoù bet diverket',
'right-undelete'         => 'Assevel ur bajenn',
'right-editinterface'    => 'Degas kemmoù war an etrefas implijer',
'right-siteadmin'        => 'Prennañ ha dibrennañ ar bank-titouroù',

# User rights log
'rightslog'      => 'Marilh statud an implijerien',
'rightslogtext'  => "Setu marilh ar c'hemmoù statud bet c'hoarvezet d'an implijerien.",
'rightslogentry' => 'en/he deus cheñchet gwirioù implijer $1 a oa $2 hag a zo bet lakaet da $3',
'rightsnone'     => '(netra)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|kemm|kemm}}',
'recentchanges'                     => 'Kemmoù diwezhañ',
'recentchangestext'                 => "Dre ar bajenn-mañ e c'hallit heuliañ ar c'hemmoù diwezhañ bet degaset d'ar wiki.",
'recentchanges-feed-description'    => "Heuilhit ar c'hemmoù diwezhañ er wiki el lusk-mañ.",
'rcnote'                            => "Setu aze an {{PLURAL:$1|'''1''' change|'''$1''' kemm diwezhañ}} bet c'hoarvezet e-pad an {{PLURAL:$2|deiz|'''$2''' deiz}} diwezhañ, savet d'an $4 da $5.",
'rcnotefrom'                        => "Setu aze roll ar c'hemmoù c'hoarvezet abaoe an '''$2''' ('''$1''' d'ar muiañ).",
'rclistfrom'                        => "Diskouez ar c'hemmoù diwezhañ abaoe an $1.",
'rcshowhideminor'                   => "$1 ar c'hemmoù dister",
'rcshowhidebots'                    => '$1 ar robotoù',
'rcshowhideliu'                     => '$1 an implijerien enrollet',
'rcshowhideanons'                   => '$1 an implijerien dizanv',
'rcshowhidepatr'                    => "$1 ar c'hemmoù gwiriet",
'rcshowhidemine'                    => "$1 ma c'hemmoù",
'rclinks'                           => "Diskouez an $1 kemm diwezhañ c'hoarvezet e-pad an $2 devezh diwezhañ<br />$3",
'diff'                              => "diforc'h",
'hist'                              => 'ist',
'hide'                              => 'kuzhat',
'show'                              => 'diskouez',
'minoreditletter'                   => 'D',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|implijer o heuliañ|implijer}} o heuliañ]',
'rc_categories'                     => 'Bevenn ar rummadoù (dispartiañ gant "|")',
'rc_categories_any'                 => 'An holl',
'newsectionsummary'                 => '/* $1 */ rann nevez',

# Recent changes linked
'recentchangeslinked'          => 'Heuliañ al liammoù',
'recentchangeslinked-title'    => 'Kemmoù a denn da "$1"',
'recentchangeslinked-noresult' => 'Kemm ebet war ar pajennoù liammet e-pad an amzer spisaet.',
'recentchangeslinked-summary'  => "Rollet eo war ar bajenn zibar-mañ ar c'hemmoù diwezhañ bet degaset war ar pajennoù liammet ouzh ur bajenn lakaet (pe ouzh izili ur rummad lakaet). 
E '''tev''' emañ ar pajennoù zo war ho [[Special:Watchlist|roll evezhiañ]].",
'recentchangeslinked-page'     => 'Anv ar bajenn:',
'recentchangeslinked-to'       => "Diskouez ar c'hemmoù war-du ar pajennoù liammet kentoc'h eget re ar bajenn lakaet",

# Upload
'upload'                      => 'Kargañ war ar servijer',
'uploadbtn'                   => 'Kargañ ur restr',
'reupload'                    => 'Eilañ adarre',
'reuploaddesc'                => "Distreiñ d'ar furmskrid.",
'uploadnologin'               => 'diluget',
'uploadnologintext'           => "ret eo deoc'h bezañ [[Special:UserLogin|luget]]
evit eilañ restroù war ar servijer.",
'upload_directory_read_only'  => "N'hall ket ar servijer skrivañ e renkell ar c'hargadennoù ($1).",
'uploaderror'                 => 'Fazi',
'uploadtext'                  => "Grit gant ar furmskrid a-is evit enporzhiañ skeudennoù.
Evit sellet pe klask skeudennoù bet enporzhiet a-raok sellit ouzh [[Special:ImageList|roll ar skeudennoù eilet war ar servijer]]. Kavet e vo ar skeudennoù enporzhiet pe diverket war [[Special:Log/upload|marilh ar pajennoù enporzhiet]] ivez.

Evit enklozañ ar skeudenn en ur pennad, lakait er pennad-se ul liamm skrivet evel-henn :
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:anv_ar_restr.jpg]]</nowiki>''' pe
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:anv_ar_restr.png|testenn all]]</nowiki>''' pe
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:anv_ar_restr.ogg]]</nowiki>''' evit sevel ul liamm war-eeun war-du ar restr.",
'upload-permitted'            => 'Seurtoù restroù aotreet : $1.',
'upload-preferred'            => 'Seurtoù restroù gwellañ : $1.',
'upload-prohibited'           => 'Seurtoù restroù berzet : $1.',
'uploadlog'                   => 'marilh ar pajennoù enporzhiet',
'uploadlogpage'               => 'Marilh ar pajennoù enporzhiet',
'uploadlogpagetext'           => 'Setu marilh ar restroù diwezhañ bet karget war ar servijer.',
'filename'                    => 'Anv&nbsp;',
'filedesc'                    => 'Deskrivadur&nbsp;',
'fileuploadsummary'           => 'Diverrañ :',
'filestatus'                  => 'Statud ar gwirioù aozer:',
'filesource'                  => 'Mammenn:',
'uploadedfiles'               => 'Restroù karget',
'ignorewarning'               => "Na ober van eus ar c'hemennoù diwall ha saveteiñ ar restr forzh penaos",
'ignorewarnings'              => "Na ober van ouzh ar c'hemennoù diwall",
'minlength1'                  => 'Anv ar restroù a rank bezañ keit hag ul lizherenn da nebeutañ.',
'illegalfilename'             => "Lakaet ez eus bet er restr « $1 » arouezennoù n'int ket aotreet evit titl ur bajenn. Mar plij, adanvit ar restr hag adkasit anezhi.",
'badfilename'                 => 'Anvet eo bet ar skeudenn « $1 ».',
'filetype-badmime'            => 'N\'eo ket aotreet pellgargañ ar restroù a seurt MIME "$1".',
'filetype-unwanted-type'      => "'''Eus ar seurt restroù n'int ket c'hoantaet eo \".\$1\"'''.  Ar re a zere ar gwellañ zo eus {{PLURAL:\$3|ar seurt|ar seurt}} \$2.",
'filetype-banned-type'        => "'''N'eo ket \".\$1\"''' eus ar seurt restroù aotreet. 
\$2 eo {{PLURAL:\$3|ar seurt restroù|ar seurt restroù}} degemeret.",
'filetype-missing'            => 'N\'eus astenn ebet stag ouzh ar restr (evel ".jpg").',
'large-file'                  => "Erbediñ a reer ne vefe ket brasoc'h ar restroù eget $1; $2 eo ment ar restr-mañ.",
'largefileserver'             => "Brasoc'h eo ar restr-mañ eget ar pezh a c'hall ar servijer aotren.",
'emptyfile'                   => "Evit doare eo goullo ar restr bet karget ganeoc'h. Moarvat eo abalamour d'an tipo en anv ar restr. Gwiriit mat e fell deoc'h pellgargañ ar restr-mañ.",
'fileexists'                  => "Ur restr all gant an anv-se zo c'hoazh. Trugarez da wiriañ <strong><tt>$1</tt></strong>. Ha sur oc'h da gaout c'hoant da gemmañ ar restr-mañ ?",
'filepageexists'              => "Amañ <strong><tt>$1</tt></strong> eo bet krouet ar bajenn zeskrivañ evit ar restr-mañ, padal n'eus ket restr ebet dezhi an anv-se evit c'hoazh. An diverradenn skrivet ganeoc'h ne erlec'hio ket an destenn a oa a-raok ; ret e vije deoc'h kemmañ ar bajenn gant an dorn.",
'fileexists-extension'        => "Bez' ez eus dija ur restr gant an anv-se war-bouez nebeut :<br />
Anv ar restr pellgarget : <strong><tt>$1</tt></strong><br />
Anv ar restr zo anezhi dija : <strong><tt>$2</tt></strong><br />
An diforc'h nemetañ zo war pennlizherenn an astenn. Gwiriit eo disheñvel ar restroù ha klaskit en-dro.",
'fileexists-thumb'            => "<center>'''Skeudenn zo anezhi'''</center>",
'fileexists-thumbnail-yes'    => "Evit doare ez eus ur skeudenn krennet he ment eus ar restr <i>(thumbnail)</i>. Gwiriit ar restr <strong><tt>$1</tt></strong>.<br />
Mard eo an hevelep skeudenn ha hini ar restr orin zo gwelloc'h he ferzhded n'eo ket dav pellgargañ ur stumm krennet ouzhpenn.",
'file-thumbnail-no'           => "Kregiñ a ra anv ar restr gant <strong><tt>$1</tt></strong>. Evit doare eo ur skeudenn krennet he ment <i>(thumbnail)</i>.
Ma'z eus ganeoc'h ur skeudenn uhel he fizhder, pellgargit anezhi; a-hend-all cheñchit anv ar restr.",
'fileexists-forbidden'        => "Ur restr all gant an anv-se zo c'hoazh; kit war ho kiz hag enporzhiit ar restr dindan un anv all [[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Ur restr all gant an hevelep anv zo c'hoazh er c'havlec'h eskemm restroù; kit war ho kiz hag enpozhiit ar restr adarre dindan un anv all. [[Image:$1|thumb|center|$1]]",
'successfulupload'            => 'Eiladenn kaset da benn vat',
'uploadwarning'               => 'Diwallit!',
'savefile'                    => 'Enrollañ ar restr',
'uploadedimage'               => '"[[$1]]" enporzhiet',
'overwroteimage'              => 'enporzhiet ur stumm nevez eus "[[$1]]"',
'uploaddisabled'              => 'Ho tigarez, diweredekaet eo bet kas ar restr-mañ.',
'uploaddisabledtext'          => "N'haller ket kargañ restroù war {{SITENAME}}.",
'uploadscripted'              => "Er restr-mañ ez eus kodoù HTML pe skriptoù a c'hallfe bezañ kammgomprenet gant ur merdeer Kenrouedad.",
'uploadcorrupt'               => 'Brein eo ar restr-mañ, par eo he ment da netra pe fall eo an astenn anezhi.
Gwiriit anezhi mar plij.',
'uploadvirus'                 => 'Viruzet eo ar restr! Titouroù : $1',
'sourcefilename'              => 'Anv ar restr tarzh :',
'destfilename'                => 'Anv ma vo enrollet ar restr :',
'upload-maxfilesize'          => 'Ment vrasañ ar restr : $1',
'watchthisupload'             => 'Evezhiañ ar bajenn-mañ',
'filewasdeleted'              => "Ur restr gant an anv-mañ zo bet enporzhiet dija ha diverket goude-se. Mat e vefe deoc'h gwiriañ an $1 a-raok hec'h enporzhiañ en-dro.",
'upload-wasdeleted'           => "'''Diwallit : Oc'h enporzhiañ ur bajenn bet diverket c'hoazh emaoc'h.'''

En em soñjit ervat ha mat eo kenderc'hel da enporzhiañ ar restr-mañ.
Kavit amañ ar marilh diverkañ evit ar restr-mañ :",
'filename-bad-prefix'         => "Anv ar restr emaoc'h oc'h enporzhiañ a grog gant <strong>\"\$1\"</strong>, da lavaret eo un anv dizeskrivus roet alies ent emgefre gant luc'hskeudennerezioù niverel. Dibabit un anv splannoc'h evit deskrivañ ar restr.",
'filename-prefix-blacklist'   => " #<!-- leave this line exactly as it is --> <pre>
# Setu penaos emañ an ereadur :
#   * Pep tra adal un arouezenn \"#\" betek dibenn al linenn a ya d'ober un notenn
#   * Pep linenn n'eo ket goullo zo ur rakger evit anvioù restroù heverk roet ent emgefre gant luc'hskeudennerezioù niverel
CIMG # Casio
DSC_ # Nikon
DSCF # Fuji
DSCN # Nikon
DUW # pellgomzerioù hezoug zo
IMG # jenerik
JD # Jenoptik
MGP # Pentax
PICT # misc.
 #</pre> <!-- leave this line exactly as it is -->",

'upload-proto-error'      => 'Protokol direizh',
'upload-proto-error-text' => 'Rekis eo an URLoù a grog gant <code>http://</code> pe <code>ftp://</code> evit enporzhiañ.',
'upload-file-error'       => 'Fazi diabarzh',
'upload-file-error-text'  => "Ur fazi diabarzh zo c'hoarvezet en ur grouiñ ur restr da c'hortoz war ar servijer. Kit e darempred gant unan eus merourien ar reizhiad.",
'upload-misc-error'       => 'Fazi kargañ dianav',
'upload-misc-error-text'  => "Ur fazi dianav zo bet e-ser kargañ. Gwiriit eo reizh an URL hag e c'hall bezañ tizhet ha klaskit en-dro. Ma talc'h ar gudenn, kit e darempred gant merourien ar reizhiad.",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => "N'eus ket bet gallet tizhout an URL",
'upload-curl-error6-text'  => "N'eus ket bet gallet tizhout an URL. Gwiriit mat eo reizh an URL hag emañ al lec'hienn enlinenn.",
'upload-curl-error28'      => "Aet dreist d'an termen",
'upload-curl-error28-text' => "Re bell eo bet al lec'hienn o respont. Gwiriit mat emañ al lec'hienn enlinenn, gortozit ur pennadig ha klaskit en-dro. Mat e vo deoc'h adklask d'ur mare dibresoc'h marteze ivez.",

'license'            => 'Aotre implijout :',
'nolicense'          => 'Hini ebet diuzet',
'license-nopreview'  => '(Dibosupl rakwelet)',
'upload_source_url'  => " (Un URL reizh a c'hall bezañ tizhet gant an holl)",
'upload_source_file' => " (ur restr war hoc'h urzhiataer)",

# Special:ImageList
'imagelist-summary'     => 'Diskouez a ra ar bajenn zibar-mañ an holl restroù bet enporzhiet.
Dre ziouer e teu ar restroù enporzhiet da ziwezhañ e laez ar roll.
Klikañ war penn ar bann a cheñch an urzh kinnig.',
'imagelist_search_for'  => 'Klask anv ar skeudenn :',
'imgfile'               => 'restr',
'imagelist'             => 'Roll ar skeudennoù',
'imagelist_date'        => 'Deiziad',
'imagelist_name'        => 'Anv',
'imagelist_user'        => 'Implijer',
'imagelist_size'        => 'Ment (e bitoù)',
'imagelist_description' => 'Deskrivadur',

# Image description page
'filehist'                       => 'Istor ar restr',
'filehist-help'                  => 'Klikañ war un deiziad/eur da welet ar restr evel ma oa da neuze.',
'filehist-deleteall'             => 'diverkañ pep tra',
'filehist-deleteone'             => 'diverkañ',
'filehist-revert'                => 'disteuler',
'filehist-current'               => 'red',
'filehist-datetime'              => 'Deiziad/Eur',
'filehist-user'                  => 'Implijer',
'filehist-dimensions'            => 'Mentoù',
'filehist-filesize'              => 'Ment ar restr',
'filehist-comment'               => 'Notenn',
'imagelinks'                     => 'Liammoù war-du ar skeudenn',
'linkstoimage'                   => "Liammet eo ar {{PLURAL:$1|bajenn-mañ|$1 ar pajennoù-mañ}} d'ar restr-mañ :",
'nolinkstoimage'                 => "N'eus liamm ebet war-du ar skeudenn-mañ war pajenn ebet.",
'sharedupload'                   => 'Rannet eo ar restr-mañ. Gallout a ra bezañ implijet evit raktresoù all.',
'shareduploadwiki'               => "Gwelit an $1 mar plij evit gouzout hiroc'h.",
'shareduploadwiki-desc'          => "Diskouezet eo a-is an deskrivadur a-zivout $1 er c'havlec'h rannet.",
'shareduploadwiki-linktext'      => 'pajenn deskrivañ ar restr',
'shareduploadduplicate-linktext' => 'ur restr all',
'shareduploadconflict-linktext'  => 'ur restr all',
'noimage'                        => "N'eus pajenn ebet anvet evel-se c'hoazh, gallout a rit $1.",
'noimage-linktext'               => 'enporzhiañ unan',
'uploadnewversion-linktext'      => 'Kargañ ur stumm nevez eus ar restr-mañ',
'imagepage-searchdupe'           => 'Klask ar restroù e doubl',

# File reversion
'filerevert'                => 'Disteuler $1',
'filerevert-legend'         => 'Disteuler ar restr',
'filerevert-intro'          => "Emaoc'h o tistreiñ '''[[Media:$1|$1]]''' d'ar [stumm $4 eus $3, $2].",
'filerevert-comment'        => 'Notenn :',
'filerevert-defaultcomment' => 'Distroet da stumm $2, $1',
'filerevert-submit'         => 'Disteuler',
'filerevert-success'        => "'''Distroet eo bet [[Media:$1|$1]]''' da [stumm $4 an $3, $2].",
'filerevert-badversion'     => "N'eus stumm lec'hel kent ebet eus ar restr-mañ d'ar mare spisaet.",

# File deletion
'filedelete'                  => 'Diverkañ $1',
'filedelete-legend'           => 'Diverkañ ar restr',
'filedelete-intro'            => "O tiverkañ '''[[Media:$1|$1]]''' emaoc'h.",
'filedelete-intro-old'        => "Emaoc'h o tiverkañ stumm '''[[Media:$1|$1]]''' eus [$4 $3, $2].",
'filedelete-comment'          => 'Notenn :',
'filedelete-submit'           => 'Diverkañ',
'filedelete-success'          => "Diverket eo bet '''$1'''.",
'filedelete-success-old'      => '<span class="plainlinks">Diverket eo bet stumm \'\'\'[[Media:$1|$1]]\'\'\' eus $3, $2.</span>',
'filedelete-nofile'           => "N'eus ket eus '''$1''' war {{SITENAME}}.",
'filedelete-nofile-old'       => "N'eus stumm diellaouet ebet eus '''$1''' gant an dezverkoù lakaet.",
'filedelete-iscurrent'        => "Emaoc'h o klask diverkañ doare diwezhañ ar restr-mañ. Distroit d'ur stumm koshoc'h da gentañ.",
'filedelete-otherreason'      => 'Abeg all/ouzhpenn :',
'filedelete-reason-otherlist' => 'Abeg all',
'filedelete-reason-dropdown'  => "*Abegoù diverkañ boas
** Gaou ouzh ar gwirioù perc'hennañ
** Restr zo anezhi dija",
'filedelete-edit-reasonlist'  => 'Kemmañ a ra an abegoù diverkañ',

# MIME search
'mimesearch'         => 'Klask MIME',
'mimesearch-summary' => 'Aotren a ra ar bajenn-mañ ar silañ restroù evit ar seurt restroù MIME. Enmont : seurt/isseurt, evel <tt>skeudenn/jpeg</tt>.',
'mimetype'           => 'Seurt MIME :',
'download'           => 'pellgargañ',

# Unwatched pages
'unwatchedpages' => "Pajennoù n'int ket evezhiet",

# List redirects
'listredirects' => 'Roll an adkasoù',

# Unused templates
'unusedtemplates'     => 'Patromoù dizimplij',
'unusedtemplatestext' => 'Rollet eo amañ an holl bajennoù dezho an esaouenn anv "patrom" ha n\'int ket implijet war pajenn ebet. Ho pet soñj da wiriañ mat hag-eñ n\'eus ket liammoù all war-du ar patromoù-se a-raok diverkañ anezho.',
'unusedtemplateswlh'  => 'liammoù all',

# Random page
'randompage'         => 'Ur bajenn dre zegouezh',
'randompage-nopages' => "N'eus pajenn ebet en esaouenn anv-mañ.",

# Random redirect
'randomredirect'         => 'Ur bajenn adkas dre zegouezh',
'randomredirect-nopages' => "N'eus pajenn adkas ebet en esaouenn anv-mañ.",

# Statistics
'statistics'             => 'Stadegoù',
'sitestats'              => "Stadegoù al lec'hienn",
'userstats'              => 'Stadegoù implijer',
'sitestatstext'          => "En holl ez eus {{PLURAL:\$1|'''1''' pajenn|'''\$1''' pajenn}} er bank roadennoù.
Er sifr-mañ emañ ar pajennoù \"kaozeal\", ar pajennoù a denn da {{SITENAME}}, an danvez-pennadoù, ar pajennoù adkas ha meur a seurt pajenn all n'haller ket sellet outo evel pennadoù da vat.
Mar lakaer ar pajennoù-se er-maez e chom {{PLURAL:\$2|'''1''' pajenn|'''\$2''' pajenn}} zo moarvat gwir pennadoù.

Abaoe m'eo bet savet {{SITENAME}} ez eus bet sellet ouzh '''\$3''' {{PLURAL:\$3|pajenn|pajenn}} en holl, ha degaset kemmoù war '''\$4''' {{PLURAL:\$4|pajenn|pajenn}}.

'''\$8''' {{PLURAL:\$8|restr|restr}} zo bet karget.

Sed a ra e-tro '''\$5''' kemm dre bajenn ha '''\$6''' selladenn evit pep kemm.

Hirder al [http://www.mediawiki.org/wiki/Manual:Job_queue lostennad labour] zo par da '''\$7'''.",
'userstatstext'          => "{{PLURAL:$1|'''1''' [[Special:ListUsers|implijer]] enrollet|'''$1''' [[Special:ListUsers|implijer]] enrollet}} zo; en o zouez ez eus '''$2''' (pe '''$4%''') {{PLURAL:$2|gant|gant}} gwirioù $5.",
'statistics-mostpopular' => 'Pajennoù muiañ sellet',

'disambiguations'      => 'Pajennoù digejañ',
'disambiguationspage'  => 'Project:Liammoù_ouzh_ar_pajennoù_disheñvelaat',
'disambiguations-text' => "Liammet eo ar pajennoù da-heul ouzh ur '''bajenn digejañ'''.
Padal e tlefent kas war-eeun d'an danvez anezho.<br />
Sellet e vez ouzh ur bajenn evel ouzh ur bajenn digejañ ma ra gant ur patrom liammet ouzh [[MediaWiki:Disambiguationspage]]",

'doubleredirects'     => 'Adkasoù doubl',
'doubleredirectstext' => '<b>Diwallit:</b> Gallout a ra bezañ "pozitivoù faos er roll-mañ. D\'ar mare-se eo moarvat peogwir ez eus testenn war bajenn an #REDIRECT kentañ ivez.<br />War bep linenn emañ al liammoù war-du pajenn an adkas kentañ hag en eil hag ivez linenn gentañ pajenn an eil adkas zo sañset reiñ ar pal "gwirion". War-du ar pal-se e tlefe liammañ an #REDIRECT kentañ.',

'brokenredirects'        => 'Adkasoù torret',
'brokenredirectstext'    => "Kas a ra an adkasoù-mañ d'ur bajenn n'eus ket anezhi.",
'brokenredirects-edit'   => '(kemmañ)',
'brokenredirects-delete' => '(diverkañ)',

'withoutinterwiki'         => 'Pajennoù hep liammoù yezh',
'withoutinterwiki-summary' => "Ar pajennoù da-heul n'int ket liammet ouzh yezh all ebet :",
'withoutinterwiki-legend'  => 'Rakger',
'withoutinterwiki-submit'  => 'Diskouez',

'fewestrevisions' => 'Pennadoù nebeutañ reizhet',

# Miscellaneous special pages
'nbytes'                  => '$1 eizhbit',
'ncategories'             => '$1 rummad',
'nlinks'                  => '$1 daveenn',
'nmembers'                => '$1 pennad',
'nrevisions'              => '$1 stumm',
'nviews'                  => '$1 selladenn',
'specialpage-empty'       => 'Goullo eo ar bajenn-mañ.',
'lonelypages'             => 'Pajennoù hep liamm daveto',
'lonelypagestext'         => "N'eo ket liammet ar pajennoù da-heul ouzh pajenn all ebet eus {{SITENAME}}.",
'uncategorizedpages'      => 'Pajennoù hep rummad ebet',
'uncategorizedcategories' => 'Rummadoù hep rummadoù',
'uncategorizedimages'     => 'Restroù hep rummad',
'uncategorizedtemplates'  => 'Patromoù hep rummad',
'unusedcategories'        => 'Rummadoù dizimplij',
'unusedimages'            => 'Skeudennoù en o-unan',
'popularpages'            => 'Pajennoù sellet ar muiañ',
'wantedcategories'        => 'Rummadoù a vank',
'wantedpages'             => 'Pajennoù goulennet ar muiañ',
'mostlinked'              => 'Pajennoù dezho al liammoù niverusañ',
'mostlinkedcategories'    => 'Rummadoù dezho al liammoù niverusañ',
'mostlinkedtemplates'     => 'Patromoù implijet ar muiañ',
'mostcategories'          => 'Pennadoù rummatet ar muiañ',
'mostimages'              => 'Skeudennoù implijet ar muiañ',
'mostrevisions'           => 'Pennadoù bet kemmet ar muiañ',
'prefixindex'             => 'An holl bajennoù dre o lizherenn gentañ',
'shortpages'              => 'Pennadoù berr',
'longpages'               => 'Pennadoù hir',
'deadendpages'            => 'Pajennoù dall (hep liamm diabarzh)',
'deadendpagestext'        => "Ar pajennoù da-heul n'int ket liammet ouzh pajenn ebet all eus {{SITENAME}}.",
'protectedpages'          => 'Pajennoù gwarezet',
'protectedpages-indef'    => 'Gwarezoù da badout hepken',
'protectedpagestext'      => "Gwarezet eo ar pajennoù da-heul; n'haller na degas kemmoù enno nag o dilec'hiañ",
'protectedpagesempty'     => "N'eus pajenn gwarezet ebet gant an arventennoù-mañ evit poent.",
'protectedtitles'         => 'Titloù gwarezet',
'protectedtitlestext'     => "An titloù da-heul zo bet gwarezet p'int bet krouet",
'protectedtitlesempty'    => "N'eus bet gwarezet titl ebet dezhañ an arventennoù-se evit poent.",
'listusers'               => 'Roll an implijerien',
'newpages'                => 'Pajennoù nevez',
'newpages-username'       => 'Anv implijer :',
'ancientpages'            => 'Pennadoù koshañ',
'move'                    => 'adenvel',
'movethispage'            => 'Adenvel ar bajenn',
'unusedimagestext'        => "Na zisoñjit ket e c'hall lec'hiennoù all kaout ul liamm eeun war-du ar skeudenn-mañ hag e c'hall neuze ar skeudenn-mañ bezañ rollet amañ c'hoazh daoust dezhi bezañ implijet e lec'h all.",
'unusedcategoriestext'    => "Krouet eo bet ar rummadoù-mañ met n'int ket bet implijet e pennad pe rummad ebet.",
'notargettitle'           => 'netra da gavout',
'notargettext'            => 'Merkit anv ur bajenn da gavout pe hini un implijer.',
'pager-newer-n'           => "{{PLURAL:$1|1 nevesoc'h|$1 nevesoc'h}}",
'pager-older-n'           => "{{PLURAL:$1|1 koshoc'h|$1 koshoc'h}}",
'suppress'                => 'Dindan evezh',

# Book sources
'booksources'               => 'Oberennoù dave',
'booksources-search-legend' => 'Klask en oberennoù dave',
'booksources-isbn'          => 'ISBN :',
'booksources-go'            => 'Kadarnaat',
'booksources-text'          => "Ur roll liammoù a gas da lec'hiennoù all ma werzher levrioù kozh ha nevez a gavot a-is; marteze e kavot eno titouroù pelloc'h war al levrioù a glaskit :",

# Special:Log
'specialloguserlabel'  => 'Implijer :',
'speciallogtitlelabel' => 'Titl :',
'log'                  => 'Marilhoù',
'all-logs-page'        => 'An holl varilhoù',
'log-search-legend'    => 'Klask er rolloù',
'log-search-submit'    => 'Mont',
'alllogstext'          => "Diskwel ar marilhoù pellgargañ, diverkañ, gwareziñ, stankañ ha merañ. Gallout a rit personelaat ar mod diskwel en ur zibab ar marilh, an anv implijer pe ar bajenn a fell deoc'h.",
'logempty'             => 'Goullo eo istor ar bajenn-mañ.',
'log-title-wildcard'   => 'Klask an titloù a grog gant an destenn-mañ',

# Special:AllPages
'allpages'          => 'An holl bajennoù',
'alphaindexline'    => '$1 da $2',
'nextpage'          => "Pajenn war-lerc'h ($1)",
'prevpage'          => 'Pajenn gent ($1)',
'allpagesfrom'      => 'Diskouez ar pajennoù adal :',
'allarticles'       => 'An holl bennadoù',
'allinnamespace'    => 'An holl bajennoù (esaouenn $1)',
'allnotinnamespace' => "An holl bajennoù (ar re n'emaint ket en esaouenn anv $1)",
'allpagesprev'      => 'Kent',
'allpagesnext'      => "War-lerc'h",
'allpagessubmit'    => 'Kadarnaat',
'allpagesprefix'    => 'Diskouez ar pajennoù a grog gant :',
'allpagesbadtitle'  => "Fall e oa anv ar bajenn lakaet pe neuze ez eus ennañ ur rakger etrewiki pe etreyezhoù. Evit doare ez arouezennoù n'haller ket implijout en titloù.",
'allpages-bad-ns'   => 'N\'eus ket a esaouenn anv anvet "$1" war {{SITENAME}}.',

# Special:Categories
'categories'                    => 'Roll ar rummadoù',
'categoriespagetext'            => 'War ar wiki emañ ar rummadoù da-heul :',
'special-categories-sort-count' => 'Urzhiañ dre gont',
'special-categories-sort-abc'   => 'urzh al lizherenneg',

# Special:ListUsers
'listusersfrom'      => 'Diskouez anv an implijerien adal :',
'listusers-submit'   => 'Diskouez',
'listusers-noresult' => "N'eus bet kavet implijer ebet.",

# Special:ListGroupRights
'listgrouprights'          => 'Gwirioù ar strolladoù implijer',
'listgrouprights-summary'  => 'War ar bajenn-mañ ez eus ur roll eus ar strolladoù implijer termenet war ar wiki-mañ, gant ar gwirioù moned stag outo.',
'listgrouprights-group'    => 'Strollad',
'listgrouprights-rights'   => 'Gwirioù',
'listgrouprights-helppage' => 'Help:Gwirioù ar strolladoù',
'listgrouprights-members'  => '(roll an izili)',

# E-mail user
'mailnologin'     => "Chomlec'h ebet",
'mailnologintext' => "Ret eo deoc'h bezañ [[Special:UserLogin|luget]]
ha bezañ merket ur chomlec'h postel reizh en ho [[Special:Preferences|penndibaboù]]
evit gallout kas ur postel d'un implijer all.",
'emailuser'       => "Kas ur postel d'an implijer-mañ",
'emailpage'       => 'Postel implijer',
'emailpagetext'   => "M'en deus an implijer-se merket ur chomlec'h postel reizh en e benndibaboù e vo kaset ur postel dezhañ dre ar furmskrid a-is.
E maezienn \"Kaser\" ho postel e vo merket ar chomlec'h postel resisaet ganeoc'h-c'hwi, d'ar resever da c'halloud respont deoc'h ma kar.",
'usermailererror' => 'Fazi postel :',
'defemailsubject' => 'postel kaset eus {{SITENAME}}',
'noemailtitle'    => "Chomlec'h elektronek ebet",
'noemailtext'     => "N'en deus ket an implijer-mañ resisaet chomlec'h postel reizh ebet pe dibabet en deus chom hep resev posteloù a-berzh an implijerien all.",
'emailfrom'       => 'Kaser',
'emailto'         => 'Resever',
'emailsubject'    => 'Danvez',
'emailmessage'    => 'Postel',
'emailsend'       => 'Kas',
'emailccme'       => "Kasit din un eilenn eus ma c'hemennadenn dre bostel.",
'emailccsubject'  => 'Eilenn eus ho kemennadenn da $1: $2',
'emailsent'       => 'Postel kaset',
'emailsenttext'   => 'Kaset eo bet ho postel.',

# Watchlist
'watchlist'            => 'Rollad evezhiañ',
'mywatchlist'          => 'ma roll evezhiañ',
'watchlistfor'         => "(evit '''$1''')",
'nowatchlist'          => "N'eus pennad ebet en ho rollad evezhiañ.",
'watchlistanontext'    => "Ret eo deoc'h $1 evit gwelet pe kemmañ an elfennoù zo en ho rollad evezhiañ.",
'watchnologin'         => 'Diluget',
'watchnologintext'     => "Ret eo deoc'h bezañ [[Special:UserLogin|luget]]
evit kemmañ ho roll.",
'addedwatch'           => "Ouzhpennet d'ar roll",
'addedwatchtext'       => 'Ouzh ho [[Special:Watchlist|rollad evezhiañ]] eo bet ouzhpennet ar bajenn "[[:$1]]".
Kemmoù da zont ar bajenn-mañ ha re ar bajenn gaozeal stag outi a vo rollet amañ hag e teuio ar bajenn <b>e tev</b> er [[Special:RecentChanges|roll kemmoù diwezhañ]] evit bezañ gwelet aesoc\'h ganeoc\'h.

Evit tennañ ar bajenn-mañ a-ziwar ho rollad evezhiañ. klikit war "Paouez da evezhiañ" er framm merdeiñ.',
'removedwatch'         => 'Lamet a-ziwar ar rollad evezhiañ',
'removedwatchtext'     => 'Lamet eo bet ar bajenn « <nowiki>$1</nowiki> » a-ziwar ho rollad evezhiañ.',
'watch'                => 'Evezhiañ',
'watchthispage'        => 'Evezhiañ ar bajenn-mañ',
'unwatch'              => 'paouez da evezhiañ',
'unwatchthispage'      => 'Paouez da evezhiañ',
'notanarticle'         => 'Pennad ebet',
'notvisiblerev'        => 'Stumm diverket',
'watchnochange'        => "Pajenn ebet eus ar re evezhiet ganeoc'h n'eo bet kemmet e-pad ar prantad spisaet",
'watchlist-details'    => "Lakaet hoc'h eus $1 pajenn dindan evezh, anez kontañ ar pajennoù kaozeal.",
'wlheader-enotif'      => "* War enaou emañ ar c'has posteloù.",
'wlheader-showupdated' => "* E '''tev''' emañ merket ar pajennoù bet degaset kemmoù enno abaoe ar wech ziwezhañ hoc'h eus sellet outo",
'watchmethod-recent'   => "Gwiriañ ar c'hemmoù diwezhañ er pajennoù dindan evezh",
'watchmethod-list'     => "Gwiriañ ar c'hemmoù diwezhañ evit ar pajennoù evezhiet",
'watchlistcontains'    => '$1 pajenn zo en ho rollad evezhiañ',
'iteminvalidname'      => "Ur gudenn zo gant ar pennad « $1 » : n'eo ket mat e anv...",
'wlnote'               => 'A-is emañ an $1 kemm diwezhañ abaoe an <b>$2</b> eurvezh diwezhañ.',
'wlshowlast'           => 'Diskouez an $1 eurvezh $2 devezh diwezhañ $3',
'watchlist-show-bots'  => 'Diskouez kemmoù ar robotoù',
'watchlist-hide-bots'  => 'Kuzhat kemmoù ar botoù',
'watchlist-show-own'   => "Diskouez ma c'hemmoù",
'watchlist-hide-own'   => "Kuzhat ma c'hemmoù",
'watchlist-show-minor' => "Diskouez ar c'hemmoù dister",
'watchlist-hide-minor' => "Kuzhat ar c'hemmoù dister",

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Heuliet...',
'unwatching' => 'Paouez da evezhiañ...',

'enotif_mailer'                => 'Posteler Kemenn {{SITENAME}}',
'enotif_reset'                 => 'Merkañ an holl bajennoù evel gwelet',
'enotif_newpagetext'           => 'Ur bajenn nevez eo homañ.',
'enotif_impersonal_salutation' => 'implijer {{SITENAME}}',
'changed'                      => 'kemmet',
'created'                      => 'Krouet',
'enotif_subject'               => '$CHANGEDORCREATED eo bet pajenn $PAGETITLE {{SITENAME}} gant $PAGEEDITOR',
'enotif_lastvisited'           => 'Sellet ouzh $1 evit gwelet an holl gemmoù abaoe ho selladenn ziwezhañ.',
'enotif_lastdiff'              => "Gwelet $1 evit sellet ouzh ar c'hemm-mañ.",
'enotif_anon_editor'           => 'implijer dizanv $1',
'enotif_body'                  => '$WATCHINGUSERNAME ker,

$CHANGEDORCREATED Kemmet eo bet ar bajenn {{SITENAME}} evit $PAGETITLE gant $PAGEEDITOR d\'an $PAGEEDITDATE, gwelet $PAGETITLE_URL evit gwelet ar stumm red.

$NEWPAGE

Diverrañ an Implijer : $PAGESUMMARY $PAGEMINOREDIT

Mont e darempred gant an implijer:
postel: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

Nemet e yafec\'h da welet ar bajenn end-eeun, ne vo kemenn all ebet ma vez degaset kemmoù enni pelloc\'h. Gallout a rit nevesaat doare ar pennadoù evezhiet ganeoc\'h en ho rollad evezhiañ ivez.

            Ho reizhiad kemenn {{SITENAME}} muiañ karet

--
A-benn kemmañ doare ho rollad evezhiañ, sellit ouzh {{fullurl:{{ns:special}}:Watchlist/edit}}

Evezhiadennoù ha skoazell pelloc\'h :
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Diverkañ ur bajenn',
'confirm'                     => 'Kadarnaat',
'excontent'                   => "endalc'had '$1'",
'excontentauthor'             => "an danvez a oa : '$1' (ha '[[Special:Contributions/$2|$2]]' a oa bet an implijer nemetañ)",
'exbeforeblank'               => "A-raok diverkañ e oa an endalc'had : '$1'",
'exblank'                     => "pajenn c'houllo",
'delete-confirm'              => 'Diverkañ "$1"',
'delete-legend'               => 'Diverkañ',
'historywarning'              => "Diwallit: War-nes diverkañ ur bajenn ganti un istor emaoc'h :",
'confirmdeletetext'           => "War-nes diverkañ da viken ur bajenn pe ur skeudenn eus ar bank roadennoù emaoc'h. Diverket e vo ivez an holl stummoù kozh stag outi.
Kadarnait, mar plij, eo mat an dra-se hoc'h eus c'hoant da ober, e komprenit mat an heuliadoù, hag e rit se diouzh ar [[{{MediaWiki:Policy-url}}]].",
'actioncomplete'              => 'Diverkadenn kaset da benn',
'deletedtext'                 => '"Diverket eo bet <nowiki>$1</nowiki>".
Sellet ouzh $2 evit roll an diverkadennoù diwezhañ.',
'deletedarticle'              => 'diverket "$1"',
'suppressedarticle'           => 'diverket "[[$1]]"',
'dellogpage'                  => 'Roll ar pajennoù diverket',
'dellogpagetext'              => 'Setu roll ar pajennnoù diwezhañ bet diverket.',
'deletionlog'                 => 'roll an diverkadennoù',
'reverted'                    => 'Adlakaat ar stumm kent',
'deletecomment'               => 'Abeg an diverkadenn',
'deleteotherreason'           => 'Abegoù/traoù all :',
'deletereasonotherlist'       => 'Abeg all',
'deletereason-dropdown'       => "*Abegoù diverkañ boazetañ
** Goulenn gant saver ar pennad
** Gaou ouzh ar gwirioù perc'hennañ
** Vandalerezh",
'delete-edit-reasonlist'      => 'Kemmañ a ra an abegoù diverkañ',
'delete-toobig'               => 'Bras eo istor ar bajenn-mañ, ouzhpenn $1 {{PLURAL:$1|stumm|stumm}} zo. Bevennet eo bet an diverkañ pajennoù a-seurt-se kuit da zegas reuz war {{SITENAME}} dre fazi .',
'delete-warning-toobig'       => "Bras eo istor ar bajenn-mañ, ouzhpenn {{PLURAL:$1|stumm|stumm}} zo. 
Diverkañ anezhi a c'hallo degas reuz war mont en-dro diaz titouroù {{SITENAME}}; 
taolit evezh bras.",
'rollback'                    => "disteuler ar c'hemmoù",
'rollback_short'              => 'Disteuler',
'rollbacklink'                => 'disteuler',
'rollbackfailed'              => "C'hwitet eo bet an distaoladenn",
'cantrollback'                => 'Dibosupl da zisteuler: an aozer diwezhañ eo an hini nemetañ da vezañ kemmet ar pennad-mañ',
'alreadyrolled'               => "Dibosupl eo disteuler ar c'hemm diwezhañ graet e [[:$1]]
gant [[User:$2|$2]] ([[User talk:$2|Talk]]); kemmet pe distaolet eo bet c'hoazh gant unan bennak all.

Ar c'hemm diwezhañ a oa gant [[User:$3|$3]] ([[User talk:$3|Talk]]).",
'editcomment'                 => 'Diverradenn ar c\'hemm a oa: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "Kemmoù distaolet gant [[Special:Contributions/$2|$2]] ([[User talk:$2|Kaozeal]]); adlakaet d'ar stumm diwezhañ a-gent gant [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Disteuler kemmoù $1; distreiñ da stumm diwezhañ $2.',
'sessionfailure'              => 'Evit doare ez eus ur gudenn gant ho talc\'h;
Nullet eo bet an ober-mañ a-benn en em wareziñ diouzh an tagadennoù preizhañ.
Klikit war "kent" hag adkargit ar bajenn oc\'h deuet drezi; goude klaskit en-dro.',
'protectlogpage'              => 'Log_gwareziñ',
'protectlogtext'              => 'Kavit a-is ur roll pajennoù gwarezet ha diwarezet. Sellet ouzh ar [[Special:ProtectedPages|roll ar pajennoù gwarezet]] evit kaout roll ar pajennoù gwarezet bremañ.',
'protectedarticle'            => 'en/he deus gwarezet [[$1]]',
'modifiedarticleprotection'   => 'Kemmañ live gwareziñ "[[$1]]"',
'unprotectedarticle'          => 'en/he deus diwarezet [[$1]]',
'protect-title'               => 'Stankañ "$1"',
'protect-legend'              => 'Kadarnaat ar stankañ',
'protectcomment'              => 'Abeg ar stankañ',
'protectexpiry'               => 'Termen',
'protect_expiry_invalid'      => 'Direizh eo termen ar prantad.',
'protect_expiry_old'          => 'Echuet eo ar prantad termen.',
'protect-unchain'             => 'Dibrennañ an aotreoù adenvel',
'protect-text'                => "Amañ e c'hallit gwelet ha cheñch live gwareziñ ar bajenn <strong><nowiki>$1</nowiki></strong>.",
'protect-locked-blocked'      => 'E-keit ha ma viot stanket ne viot ket evit cheñch al live gwareziñ. Setu aze arventennoù a-vremañ ar bajenn <strong>$1</strong>:',
'protect-locked-dblock'       => "N'haller ket cheñch al liveoù gwareziñ rk prennet eo an diaz titouroù.
Setu doare a-vremañ ar bajenn <strong>$1</strong>:",
'protect-locked-access'       => "N'eo ket aotreet ho kont da zegas kemmoù e live gwareziñ ur bajenn.
Setu an doare a-vremañ evit ar bajenn-mañ <strong>$1</strong>:",
'protect-cascadeon'           => "Gwarezet eo ar bajenn-mañ peogwir he c'haver er {{PLURAL:$1|bajenn|pajennoù}} da-heul zo gweredekaet enno ar gwareziñ dre skalierad. Gallout a rit kemmañ al live gwareziñ met ne cheñcho ket ar gwareziñ dre skalierad.",
'protect-default'             => '(dre ziouer)',
'protect-fallback'            => 'Ezhomm zo aotre "$1"',
'protect-level-autoconfirmed' => "Stankañ an implijerien n'int ket enrollet",
'protect-level-sysop'         => 'Merourien hepken',
'protect-summary-cascade'     => 'Gwareziñ dre skalierad',
'protect-expiring'            => "a zeu d'e dermen d'an $1",
'protect-cascade'             => 'Gwareziñ dre skalierad - gwareziñ a ra an holl bajennoù zo er bajenn-mañ. ARABAT IMPLIJOUT.',
'protect-cantedit'            => "N'oc'h ket evit cheñch live gwareziñ ar bajenn-mañ rak n'oc'h ket aotreet da zegas kemmoù enni.",
'restriction-type'            => 'Aotre',
'restriction-level'           => 'Live strishaat :',
'minimum-size'                => 'Ment vihanañ',
'maximum-size'                => 'Ment vrasañ:',
'pagesize'                    => '(okted)',

# Restrictions (nouns)
'restriction-edit'   => 'Kemmañ',
'restriction-move'   => 'Adenvel',
'restriction-create' => 'Krouiñ',
'restriction-upload' => 'Enporzhiañ',

# Restriction levels
'restriction-level-sysop'         => 'Gwarez klok',
'restriction-level-autoconfirmed' => 'Gwarez darnel',
'restriction-level-all'           => 'ne vern pe live',

# Undelete
'undelete'                     => 'Diziverkañ ar bajenn ziverket',
'undeletepage'                 => 'Gwelet ha diziverkañ ar bajenn ziverket',
'undeletepagetitle'            => "'''Mont a ra stummoù diverket eus [[:$1]] d'ober ar roll da-heul'''.",
'viewdeletedpage'              => 'Gwelet ar pajennoù diverket',
'undeletepagetext'             => "Diverket eo bet ar pajennoù-mañ, er pod-lastez emaint met er bank roadennoù emaint c'hoazh ha gallout a reont bezañ diziverket eta.
Ingal e c'hall ar pod-lastez bezañ goullonderet.",
'undeleteextrahelp'            => "Evit diziverkañ ar bajenn en he fezh, laoskit goullo an holl logoù bihan ha klikit war '''''Diziverkañ!'''''. Evit diziverkañ stummoù zo hepken, askit ar logoù bihan a glot gant ar stummoù da vezañ adsavet, ha klikit war '''''Diziverkañ!'''''. Mar klikit war '''''Adderaouiñ''''' e vo naetaet ar stern diverrañ hag al logoù asket.",
'undeleterevisions'            => "$1 (g/c'h)kemm diellaouet",
'undeletehistory'              => "Ma tiziverkit ar bajenn e vo diziverket an holl gemmoù bet degaset en hec'h istor.
Ma'z eus bet krouet ur bajenn nevez dezhi an hevelep anv abaoe an diverkadenn, e teuio war wel ar c'hemmoù diziverket er rann istor a-raok, ha ne vo ket erlec'hiet ar stumm red ent emgefre.",
'undeleterevdel'               => 'Ne vo ket adsavet ar stumm-se eus ar bajenn ma talvez kement ha diverkañ evit darn an doare diwezhañ anezhi. En degouezh-mañ e rankit diaskañ pe diguzhat ar stummoù diverket da ziwezhañ.',
'undeletehistorynoadmin'       => "Diverket eo bet ar pennad-mañ. Displeget eo perak en diverradenn a-is, war un dro gant munudoù an implijerien o deus bet degaset kemmoù er bajenn a-raok na vije diverket. N'eus nemet ar verourien a c'hall tapout krog war an destenn bet diverket.",
'undelete-revision'            => 'Stumm diverket eus $1 (gwiriadenn eus an $2) gant $3 :',
'undeleterevision-missing'     => "Stumm fall pe diank. Pe hoc'h eus heuliet ul liamm fall, pe eo bet diziverket ar stumm pe c'hoazh eo bet lamet diouzh an dielloù.",
'undelete-nodiff'              => "N'eus bet kavet stumm kent ebet.",
'undeletebtn'                  => 'Diziverkañ!',
'undeletelink'                 => 'assevel',
'undeletereset'                => 'Adderaouiñ',
'undeletecomment'              => 'Notenn :',
'undeletedarticle'             => 'Diziverket"$1"',
'undeletedrevisions'           => '$1 stumm bet diziverket',
'undeletedrevisions-files'     => 'Diziverket ez eus bet $1 stumm ha $2 restr',
'undeletedfiles'               => '$1 restr adsavet',
'cannotundelete'               => "Dibosupl eo diziverkañ; moarvat eo bet diziverket gant unan bennak all araozoc'h.",
'undeletedpage'                => "<big>'''Diziverket eo bet $1'''</big>

Sellit ouzh [[Special:Log/delete|marilh an diverkadennoù]] evit teuler ur sell ouzh an diverkadennoù diwezhañ.",
'undelete-header'              => 'Gwelet [[Special:Log/delete|al log diverkañ]] evit ar pajennoù diverket nevez zo.',
'undelete-search-box'          => 'Klask pajennoù diverket',
'undelete-search-prefix'       => 'Diskouez ar pajennoù a grog gant :',
'undelete-search-submit'       => 'Klask',
'undelete-no-results'          => "N'eus bet kavet pajenn ebet a glotje e dielloù an diverkadennoù.",
'undelete-filename-mismatch'   => "Dibosupl diziverkañ stumm ar restr d'ar mare $1: ne glot ket anv ar restr",
'undelete-bad-store-key'       => "Dibosupl diziverkañ stumm ar restr d'ar mare $1: ezvezant e oa ar restr a-raok an diverkadenn.",
'undelete-cleanup-error'       => 'Fazi en ur ziverkañ ar restr diellaouet dizimplij "$1".',
'undelete-missing-filearchive' => "Dibosupl adsevel ID diell ar restr $1 rak n'emañ ket er bank ditouroù. Diziverket eo bet c'hoazh, marteze a-walc'h.",
'undelete-error-short'         => 'Fazi e-ser diziverkañ ar restr : $1',
'undelete-error-long'          => 'Fazioù zo bet kavet e-ser diziverkañ ar restr :

$1',

# Namespace form on various pages
'namespace'      => 'Esaouenn anv :',
'invert'         => 'Eilpennañ an dibab',
'blanknamespace' => '(Pennañ)',

# Contributions
'contributions' => 'Degasadennoù an implijer',
'mycontris'     => 'Ma degasadennnoù',
'contribsub2'   => 'Evit $1 ($2)',
'nocontribs'    => "N'eus bet kavet kemm ebet o klotañ gant an dezverkoù-se.",
'uctop'         => ' (diwezhañ)',
'month'         => 'Abaoe miz (hag a-raok) :',
'year'          => 'Abaoe bloaz (hag a-raok) :',

'sp-contributions-newbies'     => "Diskouez hepken degasadennoù ar c'hontoù nevez",
'sp-contributions-newbies-sub' => 'Evit an implijerien nevez',
'sp-contributions-blocklog'    => 'Roll ar stankadennoù',
'sp-contributions-search'      => 'Klask degasadennoù',
'sp-contributions-username'    => "Anv implijer pe chomlec'h IP :",
'sp-contributions-submit'      => 'Klask',

# What links here
'whatlinkshere'            => 'Pajennoù liammet',
'whatlinkshere-title'      => 'Pajennoù liammet ouzh $1',
'whatlinkshere-page'       => 'Pajenn :',
'linklistsub'              => '(Roll al liammoù)',
'linkshere'                => "Ar pajennoù a-is zo enno ul liamm a gas war-du '''[[:$1]]''':",
'nolinkshere'              => "N'eus pajenn ebet enni ul liamm war-du '''[[:$1]]'''.",
'nolinkshere-ns'           => "Pajenn ebet n'eo liammet ouzh '''[[:$1]]''' en esaouenn anv dibabet.",
'isredirect'               => 'pajenn adkas',
'istemplate'               => 'enframmet',
'isimage'                  => 'liamm ar restr',
'whatlinkshere-prev'       => '{{PLURAL:$1|kent|kent $1}}',
'whatlinkshere-next'       => "{{PLURAL:$1|war-lerc'h|war-lerc'h $1}}",
'whatlinkshere-links'      => '← liammoù',
'whatlinkshere-hideredirs' => '$1 adkas',
'whatlinkshere-hidetrans'  => '$1 treuzkluzadur',
'whatlinkshere-hidelinks'  => '$1 liamm',
'whatlinkshere-filters'    => 'Siloù',

# Block/unblock
'blockip'                     => "Stankañ ur chomlec'h IP",
'blockip-legend'              => 'Stankañ an implijer',
'blockiptext'                 => "Grit gant ar furmskrid a-is evit stankañ ar moned skrivañ ouzh ur chomlec'h IP pe un implijer bennak.
Seurt diarbennoù n'hallont bezañ kemeret nemet evit mirout ouzh ar vandalerezh hag a-du gant an [[{{MediaWiki:Policy-url}}|erbedadennoù ha reolennoù da heuliañ]].
Roit a-is an abeg resis (o verkañ, da skouer, roll ar pajennoù bet graet gaou outo).",
'ipaddress'                   => "Chomlec'h IP",
'ipadressorusername'          => "Chomlec'h IP pe anv implijer",
'ipbexpiry'                   => 'Pad ar stankadenn',
'ipbreason'                   => 'Abeg ar stankañ',
'ipbreasonotherlist'          => 'Abeg all',
'ipbreason-dropdown'          => "*Abegoù stankañ boutinañ
** Degas titouroù faos
** Tennañ danvez eus ar pajennoù
** Degas liammoù Spam war-du lec'hiennoù diavaez
** Degas danvez diboell/dizoare er pajennoù
** Emzalc'h hegazus/handeus betek re
** Mont re bell gant implij meur a gont
** Anv implijer n'eo ket aotreet",
'ipbanononly'                 => 'Stankañ an implijerien dianav hepken',
'ipbcreateaccount'            => 'Mirout a grouiñ kontoù',
'ipbemailban'                 => 'Mirout ouzh an implijer da gas posteloù',
'ipbenableautoblock'          => "Stankañ war-eeun ar chomlec'h IP diwezhañ implijet gant an den-mañ hag an holl chomlec'hioù en deus klasket degas kemmoù drezo war-lerc'h",
'ipbsubmit'                   => "Stankañ ouzh ar chomlec'h-mañ",
'ipbother'                    => 'Prantad all',
'ipboptions'                  => '2 eurvezh:2 hours,1 devezh:1 day,3 devezh:3 days,1 sizhunvezh:1 week,2 sizhunvezh:2 weeks,1 mizvezh:1 month,3 mizvezh:3 months,6 mizvezh:6 months,1 bloaz:1 year,da viken:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'prantad all',
'ipbotherreason'              => 'Abeg all/ouzhpenn',
'ipbhidename'                 => 'Kuzhat an anv implijer ar marilh stankañ, diouzh roll ar stankadennoù red hag ar roll implijerien',
'badipaddress'                => "Kamm eo ar chomlec'h IP.",
'blockipsuccesssub'           => 'Stankadenn deuet da benn vat',
'blockipsuccesstext'          => 'Stanket eo bet chomlec\'h IP "$1".
<br />Gallout a rit sellet ouzh ar [[Special:IPBlockList|bajenn-mañ]] evit gwelet roll ar chomlec\'hioù IP stanket.',
'ipb-edit-dropdown'           => 'Kemmañ an abegoù stankañ',
'ipb-unblock-addr'            => 'Distankañ $1',
'ipb-unblock'                 => "Distankañ un implijer pe ur chomlec'h IP",
'ipb-blocklist-addr'          => 'Gwelet ar stankadennoù zo evit $1',
'ipb-blocklist'               => 'Teuler ur sell ouzh roll an dud stanket evit poent',
'unblockip'                   => "Distankañ ur chomlec'h IP",
'unblockiptext'               => "Grit gant ar furmskrid a-is evit adsevel ar moned skrivañ ouzh ur chomlec'h IP bet stanket a-gent.",
'ipusubmit'                   => "Distankañ ar chomlec'h-mañ",
'unblocked'                   => 'Distanket eo bet [[User:$1|$1]]',
'unblocked-id'                => 'Distanket eo bet $1',
'ipblocklist'                 => "Roll ar chomlec'hioù IP stanket",
'ipblocklist-legend'          => 'Kavout un implijer stanket',
'ipblocklist-username'        => "Anv implijer pe chomlec'h IP :",
'ipblocklist-submit'          => 'Klask',
'blocklistline'               => '$1, $2 en/he deus stanket $3 ($4)',
'infiniteblock'               => 'da viken',
'expiringblock'               => "a zeu d'e dermen d'an $1",
'anononlyblock'               => 'implijerien dizanv hepken',
'noautoblockblock'            => 'emstankañ diweredekaet',
'createaccountblock'          => "Harzet eo ar c'hrouiñ kontoù",
'emailblock'                  => 'Postel stanket',
'ipblocklist-empty'           => 'Goullo eo roll ar stankadennoù.',
'ipblocklist-no-results'      => "An anv implier pe ar chomlec'h IP goulennet n'eo ket stanket anezhañ.",
'blocklink'                   => 'stankañ',
'unblocklink'                 => 'distankañ',
'contribslink'                => 'degasadennoù',
'autoblocker'                 => 'Emstanket rak rannañ a rit ur chomlec\'h IP gant "$1". Abeg : "$2".',
'blocklogpage'                => 'Log stankañ',
'blocklogentry'               => 'stanket "[[$1]]" e-pad $2 $3',
'blocklogtext'                => "Setu roud stankadennoù ha distankadennoù an implijerien. N'eo ket bet rollet ar chomlec'hioù IP bet stanket outo ent emgefre. Sellet ouzh [[Special:IPBlockList|roll an implijerien stanket]] evit gwelet piv zo stanket e gwirionez.",
'unblocklogentry'             => 'distanket "$1"',
'block-log-flags-anononly'    => 'implijerien dizanv hepken',
'block-log-flags-nocreate'    => 'berzet eo krouiñ kontoù',
'block-log-flags-noautoblock' => 'Emstankañ diweredekaet',
'block-log-flags-noemail'     => 'postel stanket',
'range_block_disabled'        => "Diweredekaet eo bet ar stankañ stuc'hadoù IP.",
'ipb_expiry_invalid'          => 'amzer termen direizh.',
'ipb_already_blocked'         => 'Stanket eo "$1" dija',
'ipb_cant_unblock'            => "Fazi: N'eo ket stanket an ID $1. Moarvat eo bet distanket c'hoazh.",
'ipb_blocked_as_range'        => "Fazi : N'eo ket bet stanket ar chomlec'h IP $1 war-eeun, setu n'hall ket bezañ distanket. Stanket eo bet dre al live $2 avat, hag a c'hall bezañ distanket.",
'ip_range_invalid'            => 'Stankañ IP direizh.',
'blockme'                     => "Stankit ac'hanon",
'proxyblocker'                => 'Stanker proksi',
'proxyblocker-disabled'       => "Diweredekaet eo an arc'hwel-mañ.",
'proxyblockreason'            => "Stanket eo bet hoc'h IP rak ur proksi digor eo. Trugarez da gelaouiñ ho pourvezer moned ouzh ar Genrouedad pe ho skoazell deknikel eus ar gudenn surentez-mañ.",
'proxyblocksuccess'           => 'Echu.',
'sorbsreason'                 => "Rollet eo ho chomlec'h IP evel ur proksi digor en DNSBL implijet gant {{SITENAME}}.",
'sorbs_create_account_reason' => "Rollet eo ho chomlec'h IP evel ur proksi digor war an DNSBL implijet gant {{SITENAME}}. N'hallit ket krouiñ ur gont",

# Developer tools
'lockdb'              => 'Prennañ ar bank',
'unlockdb'            => 'Dibrennañ ar bank',
'lockdbtext'          => "Ma vez prennet ar bank roadennoù n'hallo ket mui implijer ebet kemmañ pajennoù, enrollañ e benndibaboù, kemmañ e rollad evezhiañ na seveniñ oberiadenn ebet a c'houlenn degas kemm pe gemm er bank roadennoù.
Kadarnait, mar plij, eo se hoc'h eus c'hoant da ober hag e vo dibrennet ar bank ganeoc'h kerkent ha ma vo bet kaset da benn hoc'h oberiadenn drezalc'h.",
'unlockdbtext'        => "Dibrennañ ar bank a lakay adarre an holl implijerien e-tailh da gemmañ pajennoù, hizivaat o fenndibaboù hag o rollad evezhiañ ha seveniñ an holl oberiadennoù a c'houlenn ma vefe kemmet ar bank roadennoù.
Kadarnait, mar plij, eo se hoc'h eus c'hoant da ober.",
'lockconfirm'         => 'Ya, kadarnaat a ran e fell din prennañ ar bank roadennoù.',
'unlockconfirm'       => 'Ya, kadarnaat a ran e fell din dibrennañ ar bank roadennoù.',
'lockbtn'             => 'Prennañ ar bank',
'unlockbtn'           => 'Dibrennañ ar bank',
'locknoconfirm'       => "N'eo ket bet asket al log kadarnaat ganeoc'h.",
'lockdbsuccesssub'    => 'Bank prennet.',
'unlockdbsuccesssub'  => 'Bank dibrennet.',
'lockdbsuccesstext'   => "Prennet eo bank roadennnoù {{SITENAME}}.

<br />Na zisoñjit ket e zibrennañ pa vo bet kaset da benn vat hoc'h oberiadenn drezalc'h.",
'unlockdbsuccesstext' => 'Dibrennet eo bank roadennoù {{SITENAME}}.',
'lockfilenotwritable' => "N'haller ket skrivañ war restr prennañ ar bank roadennoù. A-benn prennañ-dibrennañ ar bank e rankit bezañ aotreet da skrivañ war ar servijer Kenrouedad.",
'databasenotlocked'   => "N'eo ket prennet ar bank titouroù.",

# Move page
'move-page'               => "Dilec'hiañ $1",
'move-page-legend'        => 'Adenvel ur pennad',
'movepagetext'            => "Grit gant ar furmskrid a-is evit adenvel ur pennad hag adkas an holl stummoù kent anezhañ war-du an anv nevez.
Dont a raio an titl kentañ da vezañ ur bajenn adkas war-du an titl nevez.
Ne vo ket kemmet liammoù an titl kozh ha ne vo ket dilec'hiet ar bajenn gaozeal, ma'z eus anezhi.

'''DIWALLIT!'''
Gallout a ra kement-se bezañ ur c'hemm bras ha dic'hortoz evit ur pennad a vez sellet outi alies;
bezit sur e komprenit mat an heuliadoù a-raok kenderc'hel ganti.",
'movepagetalktext'        => "Gant se e vo adanvet ent emgefre ar bajenn gaozeal stag, ma'z eus anezhi '''nemet ma:'''
*ec'h adanvit ur bajenn war-du ul lec'h all,
*ez eus ur bajenn gaozeal c'hoazh gant an anv nevez, pe
*diweredekaet hoc'h eus ar bouton a-is.

En degouezh-se e rankot adenvel pe gendeuziñ ar bajenn c'hwi hoc'h-unan ma karit.",
'movearticle'             => "Dilec'hiañ ar pennad",
'movenotallowed'          => "N'oc'h ket aotreet da zilec'hiañ pajennoù.",
'newtitle'                => 'anv nevez',
'move-watch'              => 'Evezhiañ ar bajenn-mañ',
'movepagebtn'             => 'Adenvel ar pennad',
'pagemovedsub'            => "Dilec'hiadenn kaset da benn vat",
'movepage-moved'          => '<big>\'\'\'Adkaset eo bet "$1" war-du "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => "Ur pennad gantañ an anv-se zo dija pe n'eo ket reizh an titl hoc'h eus dibabet.
Dibabit unan all mar plij.",
'cantmove-titleprotected' => "N'hallit ket dilec'hiañ ur bajenn d'al lec'h-mañ rak gwarezet eo bet an titl nevez p'eo bet krouet.",
'talkexists'              => "Dilec'hiet mat eo bet ar bajenn hec'h-unan met chomet eo ar bajenn gaozeal rak unan all a oa dija gant an anv nevez-se. Kendeuzit anezho c'hwi hoc'h-unan mar plij.",
'movedto'                 => 'adanvet e',
'movetalk'                => 'Adenvel ivez ar bajenn "gaozeal", mar bez ret.',
'1movedto2'               => '[[$1]] adkaset war-du [[$2]]',
'1movedto2_redir'         => '[[$1]] adkaset war-du [[$2]] (adkas)',
'movelogpage'             => 'Roll an adkasoù',
'movelogpagetext'         => 'Setu roll ar pajennoù bet savet un adkas evito.',
'movereason'              => 'Abeg an adkas',
'revertmove'              => 'nullañ',
'delete_and_move'         => 'Diverkañ ha sevel adkas',
'delete_and_move_text'    => "==Ezhomm diverkañ==

Savet eo ar pennad tal \"[[:\$1]]\" c'hoazh.
Diverkañ anezhañ a fell deoc'h ober evit reiñ lec'h d'an adkas ?",
'delete_and_move_confirm' => 'Ya, diverkañ ar bajenn',
'delete_and_move_reason'  => "Diverket evit ober lec'h d'an adkas",
'selfmove'                => "Heñvel eo titl ar poent loc'hañ ha hini ar pal; n'haller ket adkas ur bajenn war-du he lec'h orin.",
'immobile_namespace'      => "Dibarek eo titl ar vammenn pe ar pal; n'haller ket adenvel pajennoù war-du an esaouenn anv-mañ.",
'imagetypemismatch'       => 'Ne glot ket astenn nevez ar restr gant ar furmad-mañ.',

# Export
'export'            => 'Ezporzhiañ pajennoù',
'exporttext'        => "Gallout a rit ezporzhiañ en XML an destenn ha pennad istor ur bajenn pe ur strollad pajennoù; a-benn neuze e c'hall an disoc'h bezañ enporzhiet en ur wiki all a ya en-dro gant ar meziant MediaWiki, treuzfurmet pe enrollet da vezañ implijet diouzh ma karot.",
'exportcuronly'     => 'Ezporzhiañ hepken ar stumm red hep an istor anezhañ',
'exportnohistory'   => "----
'''Notenn :''' Dilezet eo bet an ezporzhiañ istor klok ar pajennoù evit poent peogwir e veze gorrekaet ar reizhiad diwar se.",
'export-submit'     => 'Ezporzhiañ',
'export-addcattext' => 'Ouzhpennañ pajennoù ar rummad :',
'export-addcat'     => 'Ouzhpennañ',
'export-download'   => 'Aotren enrollañ evel ur restr',
'export-templates'  => 'Lakaat ar patromoù e-barzh ivez',

# Namespace 8 related
'allmessages'               => 'Roll kemennoù ar reizhiad',
'allmessagesname'           => 'Anv',
'allmessagesdefault'        => 'Testenn dre ziouer',
'allmessagescurrent'        => 'Testenn zo bremañ',
'allmessagestext'           => "Setu roll an holl gemennadennoù a c'haller kaout e bed MediaWiki",
'allmessagesnotsupportedDB' => "N'haller ket kaout {{ns:special}}:AllMessages rak diweredekaet eo bet wgUseDatabaseMessages.",
'allmessagesfilter'         => 'Sil anv kemennadenn :',
'allmessagesmodified'       => 'Diskouez ar re bet kemmet hepken',

# Thumbnails
'thumbnail-more'           => 'Brasaat',
'filemissing'              => 'Restr ezvezant',
'thumbnail_error'          => 'Fazi e-ser krouiñ an alberz : $1',
'djvu_page_error'          => 'Pajenn DjVu er-maez ar bevennoù',
'djvu_no_xml'              => 'Dibosupl da dapout an XML evit ar restr DjVu',
'thumbnail_invalid_params' => 'Arventennoù direizh evit ar munud',
'thumbnail_dest_directory' => "Dibosupl krouiñ ar c'havlec'h pal",

# Special:Import
'import'                     => 'Enporzhiañ pajennoù',
'importinterwiki'            => 'enporzhiadenn etrewiki',
'import-interwiki-text'      => 'Diuzit ur wiki hag ur bajenn da enporzhiañ.
Miret e vo deiziadoù ar stummmoù hag anvioù an aozerien.
Miret eo an holl enporzhiadennoù etrewiki e-barzh [[Special:Log/import|log an enporzhiadennoù]].',
'import-interwiki-history'   => 'Eilañ holl stummoù istor ar bajenn-mañ',
'import-interwiki-submit'    => 'Enporzhiañ',
'import-interwiki-namespace' => 'Treuzkas ar pajennoù en esaouenn anv :',
'importtext'                 => "Ezporzhiit ur restr adal ar wiki orin en ur implij an arc'hwel Special:Export, enrollit ar bajenn war ho pladenn ha degasit anezhi amañ.",
'importstart'                => "Oc'h enporzhiañ pajennoù...",
'import-revision-count'      => '$1 stumm',
'importnopages'              => 'Pajenn ebet da enporzhiañ.',
'importfailed'               => "C'hwitet eo an enporzhiadenn: $1",
'importunknownsource'        => 'Dianav eo seurt ar vammenn enporzhiañ',
'importcantopen'             => "N'eus ket bet gallet digeriñ ar restr enporzhiet",
'importbadinterwiki'         => 'Liamm etrewiki fall',
'importnotext'               => 'Goullo pe hep tamm testenn ebet',
'importsuccess'              => 'Deuet eo an enporzhiadenn da benn vat!',
'importhistoryconflict'      => "Divankadennoù zo er pennad istor ha tabut zo gant se (marteze eo bet enporzhiet ar bajenn araozoc'h)",
'importnosources'            => "N'eus bet spisaet tamm mammenn etrewiki ebet ha diweredekaet eo enporzhiañ an Istor war-eeun.",
'importnofile'               => "N'eus bet enporzhiet restr ebet.",
'importuploaderrorsize'      => "C'hwitet eo bet enporzhiañ ar restr. Brasoc'h eo ar restr eget ar vent aotreet.",
'importuploaderrorpartial'   => "C'hwitet eo vet enporzhiañ ar restr. Enporzhiet evit darn eo bet hepken.",
'importuploaderrortemp'      => "C'hwitet eo bet enporzhiañ ar restr. Mankout a ra ur restr badennek.",
'import-parse-failure'       => "Troc'h e dielfennadenn an enporzh XML",
'import-noarticle'           => 'Pajenn ebet da enporzhiañ !',
'import-nonewrevisions'      => "Enporzhiet eo bet an holl degasadennoù c'hoazh.",
'xml-error-string'           => '$1 war al linenn $2, bann $3 (okted $4) : $5',

# Import log
'importlogpage'                    => 'Log an enporzhiadennoù',
'importlogpagetext'                => "Enporzhiadennoù melestradurel eus pajennoù adal wikioù all gant istor ar c'hemmadennoù degaset enno.",
'import-logentry-upload'           => 'en/he deus enporzhiet (pellgarget) [[$1]]',
'import-logentry-upload-detail'    => '$1 stumm',
'import-logentry-interwiki'        => 'treuzwikiet $1',
'import-logentry-interwiki-detail' => '$1 kemm adal $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Ma fajenn implijer',
'tooltip-pt-anonuserpage'         => "Ar bajenn implijer evit ar c'homlec'h IP implijet ganeoc'h",
'tooltip-pt-mytalk'               => 'Ma fajenn gaozeal',
'tooltip-pt-anontalk'             => "Kaozeadennoù diwar-benn ar c'hemmoù graet adal ar chomlec'h-mañ",
'tooltip-pt-preferences'          => 'Ma fenndibaboù',
'tooltip-pt-watchlist'            => "Roll ar pajennoù evezhiet ganeoc'h.",
'tooltip-pt-mycontris'            => 'Roll ma degasadennoù',
'tooltip-pt-login'                => "Daoust ma n'eo ket ret, ec'h aliomp deoc'h d'en em lugañ.",
'tooltip-pt-anonlogin'            => "Daoust ma n'eo ket ret, ec'h aliomp deoc'h d'en em lugañ.",
'tooltip-pt-logout'               => 'Dilugañ',
'tooltip-ca-talk'                 => 'Kaozeadennoù diwar-benn ar pennad',
'tooltip-ca-edit'                 => 'Gallout a rit degas kemmoù er bajenn-mañ. Implijit ar stokell Rakwelet a-raok enrollañ, mar plij.',
'tooltip-ca-addsection'           => 'Kemerit perzh er gaoz-mañ.',
'tooltip-ca-viewsource'           => 'Gwarezet eo ar bajenn-mañ. Gallout a rit gwelet an danvez anezhañ memes tra.',
'tooltip-ca-history'              => 'Stummoù kozh ar bajenn-mañ gant an aozerien anezhi.',
'tooltip-ca-protect'              => 'Gwareziñ ar bajenn-mañ',
'tooltip-ca-delete'               => 'Diverkañ ar bajenn-mañ',
'tooltip-ca-undelete'             => 'Adsevel ar bajenn-mañ',
'tooltip-ca-move'                 => 'Adenvel ar bajenn-mañ',
'tooltip-ca-watch'                => "Ouzhpennañ ar bajenn-mañ d'ho roll evezhiañ",
'tooltip-ca-unwatch'              => 'Paouez da evezhiañ ar bajenn-mañ',
'tooltip-search'                  => 'Klaskit er wiki-mañ',
'tooltip-search-go'               => "Mont d'ar bajenn dezhi an anv-mañ rik, ma'z eus anezhi",
'tooltip-search-fulltext'         => 'Klask an destenn-mañ er pajennoù',
'tooltip-p-logo'                  => 'Pajenn bennañ',
'tooltip-n-mainpage'              => 'Diskouez ar Bajenn bennañ',
'tooltip-n-portal'                => "Diwar-benn ar raktres, ar pezh a c'hallit ober, pelec'h kavout an traoù",
'tooltip-n-currentevents'         => 'Tapout keleier diwar-benn an darvoudoù diwezhañ',
'tooltip-n-recentchanges'         => "Roll ar c'hemmoù diwezhañ c'hoarvezet war ar wiki.",
'tooltip-n-randompage'            => 'Diskwel ur bajenn dre zegouezh',
'tooltip-n-help'                  => 'Skoazell.',
'tooltip-t-whatlinkshere'         => 'Roll ar pajennoù liammet ouzh ar bajenn-mañ',
'tooltip-t-recentchangeslinked'   => "Roll ar c'hemmoù diwezhañ war ar pajennoù liammet ouzh ar bajenn-mañ",
'tooltip-feed-rss'                => 'Magañ ar red RSS evit ar bajenn-mañ',
'tooltip-feed-atom'               => 'Magañ ar red Atom evit ar bajenn-mañ',
'tooltip-t-contributions'         => 'Gwelet roll degasadennoù an implijer-mañ',
'tooltip-t-emailuser'             => "Kas ur postel d'an implijer-mañ",
'tooltip-t-upload'                => 'Enporzhiañ ur skeudenn pe ur restr media war ar servijer',
'tooltip-t-specialpages'          => 'Roll an holl bajennoù dibar',
'tooltip-t-print'                 => 'Stumm moulladus ar bajenn-mañ',
'tooltip-t-permalink'             => 'Liamm padus war-du ar stumm-mañ eus ar bajenn',
'tooltip-ca-nstab-main'           => 'Gwelet ar pennad',
'tooltip-ca-nstab-user'           => 'Gwelet ar bajenn implijer',
'tooltip-ca-nstab-media'          => 'Gwelet pajenn ar media',
'tooltip-ca-nstab-special'        => "Ur bajenn dibar eo homañ, n'oc'h ket evit degas kemmoù enni.",
'tooltip-ca-nstab-project'        => 'Gwelet pajenn ar raktres',
'tooltip-ca-nstab-image'          => 'Gwelet pajenn deskrivañ ar bajenn-mañ',
'tooltip-ca-nstab-mediawiki'      => 'Gwelet kemenn ar reizhiad',
'tooltip-ca-nstab-template'       => 'Gwelet ar patrom',
'tooltip-ca-nstab-help'           => 'Gwelet ar bajenn soazell',
'tooltip-ca-nstab-category'       => 'Gwelet pajenn ar rummad',
'tooltip-minoredit'               => "Merkañ ar c'hemm-mañ evel dister",
'tooltip-save'                    => 'Enrollañ ho kemmoù',
'tooltip-preview'                 => "Rakwelet ar c'hemmoù; trugarez d'ober gantañ a-raok enrollañ!",
'tooltip-diff'                    => "Diskouez ar c'hemmoù degaset ganeoc'h en destenn.",
'tooltip-compareselectedversions' => "Sellet ouzh an diforc'hioù zo etre daou stumm diuzet ar bajenn-mañ.",
'tooltip-watch'                   => 'Ouzhpennañ ar bajenn-mañ ouzh ho rollad evezhiañ',
'tooltip-recreate'                => 'Adkrouiñ ar bajenn ha pa vije bet diverket a-raok',
'tooltip-upload'                  => 'Kregiñ da enporzhiañ',

# Stylesheets
'common.css'   => '/** Talvezout a raio ar CSS lakaet amañ evit an holl gwiskadurioù */',
'monobook.css' => '/* Ar CSS lakaet amañ a dalvezo evit implijerien ar gwiskadur Monobook */',

# Scripts
'common.js'   => '* Forzh pe JavaScript amañ a vo karget evit an holl implijerien war kement pajenn lennet ganto. */',
'monobook.js' => '/* Deprecated; use [[MediaWiki:common.js]] */
/*Lagadennoù titouriñ ha stokelloù prim*/',

# Metadata
'nodublincore'      => "Diweredekaet eo ar metaroadennoù 'Dublin Core RDF' war ar servijer-mañ.",
'nocreativecommons' => "N'eo ket gweredekaet ar metaroadennoù 'Creative Commons RDF' war ar servijer-mañ.",
'notacceptable'     => "N'eo ket ar servijer wiki-mañ evit pourchas stlennoù en ur furmad lennus evit ho arval.",

# Attribution
'anonymous'        => 'Implijer(ez) dianv eus {{SITENAME}}',
'siteuser'         => 'Implijer(ez) $1 eus {{SITENAME}}',
'lastmodifiedatby' => "Kemmet eo bet ar bajenn-mañ da ziwezhañ d'an/ar $2, $1 gant $3", # $1 date, $2 time, $3 user
'othercontribs'    => 'Diazezet war labour $1.',
'others'           => 're all',
'siteusers'        => 'Implijer(ez) $1 eus {{SITENAME}}',
'creditspage'      => 'Pajennoù kredoù',
'nocredits'        => "N'eus tamm titour kred hegerz ebet evit ar bajenn-mañ.",

# Spam protection
'spamprotectiontitle' => "Pajenn warezet ent emgefre abalamour d'ar Spam",
'spamprotectiontext'  => "Pajenn warezet ent emgefre abalamour d'ar Spam",
'spamprotectionmatch' => 'Dihunet eo bet an detektour Spam: $1 gant an destenn-mañ',
'spambot_username'    => 'Naetaat spam MediaWiki',
'spam_reverting'      => "Distreiñ d'ar stumm diwezhañ hep liamm davet $1",
'spam_blanking'       => 'Diverkañ an holl stummoù enno liammoù davet $1',

# Info page
'infosubtitle'   => 'Titouroù evit ar bajenn',
'numedits'       => 'Niver a gemmoù (pennad) : $1',
'numtalkedits'   => 'Niver a gemmoù (pajenn gaozeal) : $1',
'numwatchers'    => 'Niver a dud o lenn : $1',
'numauthors'     => 'Niver a aozerien zisheñvel (pennad) : $1',
'numtalkauthors' => 'Niver a aozerien zisheñvel (pajenn gaozeal) : $1',

# Math options
'mw_math_png'    => 'Produiñ atav ur skeudenn PNG',
'mw_math_simple' => "HTML m'eo eeun-kenañ, a-hend-all ober gant PNG",
'mw_math_html'   => 'HTML mar bez tu, a-hend-all PNG',
'mw_math_source' => "Leuskel ar c'hod TeX orin",
'mw_math_modern' => 'Evit ar merdeerioù arnevez',
'mw_math_mathml' => 'MathML',

# Patrolling
'markaspatrolleddiff'                 => 'Merkañ evel gwiriet',
'markaspatrolledtext'                 => 'Merkañ ar pennad-mañ evel gwiriet',
'markedaspatrolled'                   => 'Merkañ evel gwiriet',
'markedaspatrolledtext'               => 'Merket eo bet ar stumm diuzet evel gwiriet.',
'rcpatroldisabled'                    => "Diweredekaet ar gwiriañ ar C'hemmoù diwezhañ",
'rcpatroldisabledtext'                => "Diweredekaet eo bet an arc'hwel evezhiañ ar c'hemmoù diwezhañ.",
'markedaspatrollederror'              => "N'hall ket bezañ merket evel gwiriet",
'markedaspatrollederrortext'          => "Ret eo deoc'h spisaat ur stumm a-benn e verkañ evel gwiriet.",
'markedaspatrollederror-noautopatrol' => "N'oc'h ket aotreet da verkañ evel gwiriet ar c'hemmoù degaset ganeoc'h.",

# Patrol log
'patrol-log-page' => 'Log gwiriañ',
'patrol-log-line' => 'en/he deus merket ar stumm $1 eus $2 evel gwiriet $3',
'patrol-log-auto' => '(emgefre)',

# Image deletion
'deletedrevision'                 => 'Diverket stumm kozh $1.',
'filedeleteerror-short'           => 'Fazi e-ser diverkañ ar restr : $1',
'filedeleteerror-long'            => 'Fazioù zo bet kavet e-ser diverkañ ar restr :

$1',
'filedelete-missing'              => 'N\'haller ket diverkañ ar restr "$1" peogwir n\'eus ket anezhi.',
'filedelete-old-unregistered'     => 'Stumm spisaet ar restr "$1" n\'emañ ket er bank titouroù.',
'filedelete-current-unregistered' => 'Ar restr spisaet "$1" n\'emañ ket er bank titouroù.',
'filedelete-archive-read-only'    => 'N\'hall ket ar servijer web skrivañ war ar c\'havlec\'h dielloù "$1".',

# Browsing diffs
'previousdiff' => "← Diforc'h kent",
'nextdiff'     => "Diforc'h war-lerc'h →",

# Media information
'mediawarning'         => "'''Diwallit''': Kodoù siek a c'hall bezañ er restr-mañ; ma'z erounezit anezhi e c'hallje tagañ ho reizhiad.<hr />",
'imagemaxsize'         => 'Ment vrasañ ar skeudennoù er pajennoù deskrivañ :',
'thumbsize'            => 'Ment an alberz :',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|pajenn|pajenn}}',
'file-info'            => '(ment ar restr : $1, seurt MIME : $2)',
'file-info-size'       => '($1 × $2 piksel, ment ar restr : $3, seurt MIME : $4)',
'file-nohires'         => "<small>N'haller ket gwellaat ar pizhder.</small>",
'svg-long-desc'        => '(restr SVG file, pizhder $1 × $2 piksel, ment ar restr : $3)',
'show-big-image'       => 'Pizhder leun',
'show-big-image-thumb' => '<small>Ment ar rakweladenn-mañ : $1 × $2 piksel</small>',

# Special:NewImages
'newimages'             => 'Roll ar skeudennoù nevez',
'imagelisttext'         => 'Setu ur roll $1 skeudenn rummet $2.',
'newimages-summary'     => 'Diskouez a ra ar bajenn zibar-mañ roll ar restroù diwezhañ bet enporzhiet.',
'showhidebots'          => '($1 bot)',
'noimages'              => 'Netra da welet.',
'ilsubmit'              => 'Klask',
'bydate'                => 'dre an deiziad anezho',
'sp-newimages-showfrom' => 'Diskouez ar restroù nevez adal $1, $2',

# Bad image list
'bad_image_list' => "Setu doare ar furmad :

Ne seller nemet ouzh roll an elfennoù (linennoù a grog gant *). Ret eo d'al liamm kentañ war ul linenn bezañ ul liamm war-du ur skeudenn fall.
Kement liamm all war an hevelep linenn a seller outañ evel un nemedenn, da skouer pennadoù ma c'hall ar skeudenn dont war wel.",

# Metadata
'metadata'          => 'Metaroadennoù',
'metadata-help'     => "Titouroù ouzhpen zo er restr-mañ; bet lakaet moarvat gant ar c'hamera niverel pe ar skanner implijet evit he niverelaat. Mard eo bet cheñchet ar skeudenn e-keñver he stad orin marteze ne vo ket kenkoulz munudoù zo e-keñver ar skeudenn kemmet.",
'metadata-expand'   => 'Dispakañ ar munudoù',
'metadata-collapse' => 'Krennañ ar munudoù',
'metadata-fields'   => "Ensoc'het e vo ar maeziennoù metastlennoù EXIF rollet er gemennadenn-mañ e pajenn deskrivañ ar skeudenn pa vo punet taolenn ar metaroadennoù. Kuzhet e vo ar re all dre ziouer.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Led',
'exif-imagelength'                 => 'Hed',
'exif-bitspersample'               => 'Niv. a vitoù dre barzhioù',
'exif-compression'                 => 'Seurt gwaskadur',
'exif-photometricinterpretation'   => 'Kenaozadur piksel',
'exif-orientation'                 => 'Tuadur',
'exif-samplesperpixel'             => 'Niver a standilhonoù',
'exif-planarconfiguration'         => 'Kempenn ar roadennoù',
'exif-ycbcrsubsampling'            => 'Feur standilhoniñ Y da C',
'exif-ycbcrpositioning'            => "Lec'hiadur Y ha C",
'exif-xresolution'                 => 'Pizhder led ar skeudenn',
'exif-yresolution'                 => 'Pizhder hed ar skeudenn',
'exif-resolutionunit'              => 'Unanennoù pizhder X ha Y',
'exif-stripoffsets'                => "Lec'hiadur roadennoù ar skeudenn",
'exif-rowsperstrip'                => 'Niver a linennoù dre vandenn',
'exif-stripbytecounts'             => 'Ment e oktedoù dre vandenn',
'exif-jpeginterchangeformat'       => "Lec'hiadur ar SOI JPEG",
'exif-jpeginterchangeformatlength' => 'Ment ar roadennoù JPEG en eizhbitoù',
'exif-transferfunction'            => "Arc'hwel treuzkas",
'exif-whitepoint'                  => 'Kromategezh ar poent gwenn',
'exif-primarychromaticities'       => 'Kromategezh al livioù orin',
'exif-ycbcrcoefficients'           => 'Kenefederioù moull treuzfurmiñ an egorenn liv',
'exif-referenceblackwhite'         => 'Talvoudenn dave gwenn ha du',
'exif-datetime'                    => 'Deiziad hag eur kemm restr',
'exif-imagedescription'            => 'Titl ar skeudenn',
'exif-make'                        => 'Oberier ar benveg',
'exif-model'                       => 'Doare ar benveg',
'exif-software'                    => 'Meziant bet implijet',
'exif-artist'                      => 'Aozer',
'exif-copyright'                   => "Perc'henn ar gwirioù aozer (copyright)",
'exif-exifversion'                 => 'Stumm exif',
'exif-flashpixversion'             => 'Skoret ganti stumm Flashpix',
'exif-colorspace'                  => "Lec'h al livioù",
'exif-componentsconfiguration'     => 'Talvoudegezh pep parzh',
'exif-compressedbitsperpixel'      => 'Doare gwaskañ ar skeudenn',
'exif-pixelydimension'             => 'Ledander skeudenn gwiriek',
'exif-pixelxdimension'             => 'Uhelder skeudenn gwiriek',
'exif-makernote'                   => 'Notennoù an oberier',
'exif-usercomment'                 => 'Evezhiadennoù',
'exif-relatedsoundfile'            => 'Restr son stag',
'exif-datetimeoriginal'            => 'Deiziad hag eur ar sevel roadoù',
'exif-datetimedigitized'           => 'Deiziad hag eur an niverelaat',
'exif-subsectime'                  => 'Deiziad kemmañ diwezhañ',
'exif-subsectimeoriginal'          => 'Deiziad an dennadenn orin',
'exif-subsectimedigitized'         => 'Deiziad niverelaat',
'exif-exposuretime'                => "Amzer louc'hañ",
'exif-exposuretime-format'         => '$1 eilenn ($2)',
'exif-fnumber'                     => 'Hed etre sti',
'exif-exposureprogram'             => "Programm louc'hañ",
'exif-spectralsensitivity'         => 'Kizidigezh spektrel',
'exif-isospeedratings'             => 'Kizidigezh ISO',
'exif-oecf'                        => 'Faktor amdreiñ elektronek',
'exif-shutterspeedvalue'           => 'Tizh klozañ',
'exif-aperturevalue'               => 'Digorder',
'exif-brightnessvalue'             => 'Sklêrder',
'exif-exposurebiasvalue'           => "Reizhadenn louc'hañ",
'exif-maxaperturevalue'            => 'Maezienn digeriñ vrasañ',
'exif-subjectdistance'             => 'Hed ar sujed',
'exif-meteringmode'                => 'Doare muzuliañ',
'exif-lightsource'                 => "Mammenn c'houloù",
'exif-flash'                       => "Luc'h",
'exif-focallength'                 => 'Hirder ar fokalenn',
'exif-subjectarea'                 => 'Gorread ar sujed',
'exif-flashenergy'                 => "Nerzh al luc'h",
'exif-spatialfrequencyresponse'    => 'Frekañs egorel',
'exif-focalplanexresolution'       => 'Muzuliadur a-led ur fokalenn blaen',
'exif-focalplaneyresolution'       => 'Muzuliadur a-serzh ur fokalenn blaen',
'exif-focalplaneresolutionunit'    => 'Unanenn spisder evit ur fokalenn blaen',
'exif-subjectlocation'             => "Lec'hiadur ar sujed",
'exif-exposureindex'               => "Meneger louc'hañ",
'exif-sensingmethod'               => 'Hentenn detektiñ',
'exif-filesource'                  => 'Tarzh ar restr',
'exif-scenetype'                   => 'Seurt arvest',
'exif-cfapattern'                  => 'Framm silañ al livioù',
'exif-customrendered'              => 'Plediñ gant ar skeudennoù personelaet',
'exif-exposuremode'                => "Mod louc'hañ",
'exif-whitebalance'                => 'Mentel ar gwennoù',
'exif-digitalzoomratio'            => 'Feur brasaat niverel (zoum)',
'exif-focallengthin35mmfilm'       => 'Hirder ar fokalenn e filmoù 35 mm',
'exif-scenecapturetype'            => 'Doare pakañ an arvest',
'exif-gaincontrol'                 => 'Reizhañ ar sklêrder',
'exif-contrast'                    => 'Dargemm',
'exif-saturation'                  => 'Saturadur',
'exif-sharpness'                   => 'Spisder',
'exif-devicesettingdescription'    => 'Deskrivadur doare ar wikefre',
'exif-subjectdistancerange'        => 'Hed ar sujed',
'exif-imageuniqueid'               => 'Anavezer nemetañ ar skeudenn',
'exif-gpsversionid'                => 'Stumm an neudennad GPS',
'exif-gpslatituderef'              => 'Ledred Norzh pe su',
'exif-gpslatitude'                 => 'Ledred',
'exif-gpslongituderef'             => 'Hedred kornôg pe reter',
'exif-gpslongitude'                => 'Hedred',
'exif-gpsaltituderef'              => 'Daveenn uhelder',
'exif-gpsaltitude'                 => 'Uhelder',
'exif-gpstimestamp'                => 'Eur GPS (eurier atomek)',
'exif-gpssatellites'               => 'Loarelloù implijet evit ar muzuliañ',
'exif-gpsstatus'                   => 'Statud ar resever',
'exif-gpsmeasuremode'              => 'Doare muzuliañ',
'exif-gpsdop'                      => 'Resisder ar muzul',
'exif-gpsspeedref'                 => 'Unanenn dizh',
'exif-gpsspeed'                    => 'Tizh ar resever GPS',
'exif-gpstrackref'                 => "Daveenn evit durc'hadur ar fiñv",
'exif-gpstrack'                    => "Durc'hadur ar fiñv",
'exif-gpsimgdirectionref'          => "Daveenn evit durc'hadur ar skeudenn",
'exif-gpsimgdirection'             => "Durc'hadur ar skeudenn",
'exif-gpsmapdatum'                 => 'Reizhiad geodetek implijet',
'exif-gpsdestlatituderef'          => 'Daveenn evit ledred ar pal',
'exif-gpsdestlatitude'             => 'Ledred ar pal',
'exif-gpsdestlongituderef'         => 'Daveenn evit hedred ar pal',
'exif-gpsdestlongitude'            => 'Hedred ar pal',
'exif-gpsdestbearingref'           => 'Daveenn evit notenniñ ar pal',
'exif-gpsdestbearing'              => 'Notenniñ ar pal',
'exif-gpsdestdistanceref'          => 'Daveenn evit an hed betek ar pal',
'exif-gpsdestdistance'             => 'Hed betek ar pal',
'exif-gpsprocessingmethod'         => 'Anv hentenn blediñ ar GPS',
'exif-gpsareainformation'          => 'Anv an takad GPS',
'exif-gpsdatestamp'                => 'Deiziad GPS',
'exif-gpsdifferential'             => "Reizhadenn diforc'hadus GPS",

# EXIF attributes
'exif-compression-1' => 'Hep gwaskañ',

'exif-unknowndate' => 'Deiziad dianav',

'exif-orientation-1' => 'Boutin', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Eilpennet a-hed', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Troet eus 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Eilpennet a-serzh', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Troet eus 90° a-gleiz hag eilpennet a-serzh', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Troet eus 90° a-zehou', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Troet eus 90° a-zehou hag eilpennet a-serzh', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Troet eus 90° a-gleiz', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Roadennoù kenstok',
'exif-planarconfiguration-2' => 'Roadennoù distag',

'exif-componentsconfiguration-0' => "n'eus ket anezhi",

'exif-exposureprogram-0' => 'Anspisaet',
'exif-exposureprogram-1' => 'Dre zorn',
'exif-exposureprogram-2' => 'Programm boutin',
'exif-exposureprogram-3' => 'Rakgwir digeriñ',
'exif-exposureprogram-4' => 'Rakkwir serriñ',
'exif-exposureprogram-5' => 'Programm krouiñ (tuadur e-keñver donder ar maez)',
'exif-exposureprogram-6' => 'Programm seveniñ (tuadur e-keñver an tizh serriñ)',
'exif-exposureprogram-7' => 'Mod poltred (evit skeudennoù a-dost gant an diadreñv dispis)',
'exif-exposureprogram-8' => 'Mod gweledva (evit skeudennoù gweledva gant an diadreñv spis)',

'exif-subjectdistance-value' => '$1 metr',

'exif-meteringmode-0'   => 'Dianav',
'exif-meteringmode-1'   => 'Keitat',
'exif-meteringmode-2'   => 'Muzul kreiz keitat',
'exif-meteringmode-3'   => 'Spot',
'exif-meteringmode-4'   => 'Liesspot',
'exif-meteringmode-5'   => 'Patrom',
'exif-meteringmode-6'   => 'Darnek',
'exif-meteringmode-255' => 'All',

'exif-lightsource-0'   => 'Dianav',
'exif-lightsource-1'   => 'Gouloù deiz',
'exif-lightsource-2'   => "Treluc'hus",
'exif-lightsource-3'   => 'Tungsten (gouloù kann)',
'exif-lightsource-4'   => "Luc'h",
'exif-lightsource-9'   => 'Amzer digoumoul',
'exif-lightsource-10'  => 'Amzer goumoulek',
'exif-lightsource-11'  => 'Skeud',
'exif-lightsource-12'  => "Gouloù deiz treluc'hus (D 5700 – 7100K)",
'exif-lightsource-13'  => "Gouloù deiz treluc'hus gwenn (N 4600 – 5400K)",
'exif-lightsource-14'  => "Gouloù treluc'hus gwenn yen (W 3900 – 4500K)",
'exif-lightsource-15'  => "Gouloù treluc'hus gwenn (WW 3200 – 3700K)",
'exif-lightsource-17'  => 'Gouloù standard A',
'exif-lightsource-18'  => 'Gouloù standard B',
'exif-lightsource-19'  => 'Gouloù standard C',
'exif-lightsource-24'  => 'Goulaouiñ studio gant tungsten ISO',
'exif-lightsource-255' => "Mammenn c'houloù all",

'exif-focalplaneresolutionunit-2' => 'meudad',

'exif-sensingmethod-1' => 'Hep resisaat',
'exif-sensingmethod-2' => 'Detekter takad liv monokromatek',
'exif-sensingmethod-3' => 'Detekter takad liv bikromatek',
'exif-sensingmethod-4' => 'Detekter takad liv trikromatek',
'exif-sensingmethod-5' => 'Detekter takad liv kemalennek',
'exif-sensingmethod-7' => 'Detekter teirlinennek',
'exif-sensingmethod-8' => 'Detekter liv linennek kemalennek',

'exif-scenetype-1' => "Lun luc'hskeudennet war-eeun",

'exif-customrendered-0' => 'Plediñ boutin',
'exif-customrendered-1' => 'Plediñ personelaet',

'exif-exposuremode-0' => "Emlouc'hañ",
'exif-exposuremode-1' => "Louc'hañ dre zorn",
'exif-exposuremode-2' => 'Emvraketiñ',

'exif-whitebalance-0' => 'Mentel ar gwennoù emgefre',
'exif-whitebalance-1' => 'Mentel ar gwennoù dre zorn',

'exif-scenecapturetype-0' => 'Standard',
'exif-scenecapturetype-1' => 'Gweledva',
'exif-scenecapturetype-2' => 'Poltred',
'exif-scenecapturetype-3' => 'Arvest noz',

'exif-gaincontrol-0' => 'Hini ebet',
'exif-gaincontrol-1' => 'Kresk pakañ izel',
'exif-gaincontrol-2' => 'Kresk pakañ uhel',
'exif-gaincontrol-3' => 'Digresk pakañ izel',
'exif-gaincontrol-4' => 'Digresk pakañ uhel',

'exif-contrast-0' => 'Boutin',
'exif-contrast-1' => 'Dister',
'exif-contrast-2' => 'Kreñv',

'exif-saturation-0' => 'Boutin',
'exif-saturation-1' => 'Saturadur izel',
'exif-saturation-2' => 'Saturadur uhel',

'exif-sharpness-0' => 'Boutin',
'exif-sharpness-1' => 'Dister',
'exif-sharpness-2' => 'Kreñv',

'exif-subjectdistancerange-0' => 'Dianav',
'exif-subjectdistancerange-1' => 'Makro',
'exif-subjectdistancerange-2' => 'Gwelet a-dost',
'exif-subjectdistancerange-3' => 'Gwelet a-bell',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Ledred norzh',
'exif-gpslatitude-s' => 'Ledred su',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Hedred reter',
'exif-gpslongitude-w' => 'Hedred kornôg',

'exif-gpsstatus-a' => 'O vuzuliañ',
'exif-gpsstatus-v' => 'etreoberatadusted ar muzul',

'exif-gpsmeasuremode-2' => 'Muzuliañ divventek',
'exif-gpsmeasuremode-3' => 'Muzuliañ teirventek',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Kilometr dre eur',
'exif-gpsspeed-m' => 'Miltir dre eur',
'exif-gpsspeed-n' => 'Skoulm',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => "Durc'hadur gwir",
'exif-gpsdirection-m' => 'Norzh magnetek',

# External editor support
'edit-externally'      => 'Kemmañ ar restr-mañ dre un arload diavaez',
'edit-externally-help' => "Gwelet skoazell an [http://www.mediawiki.org/wiki/Manual:External_editors arloadoù diavaez] a-benn gouzout hiroc'h.",

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'an holl',
'imagelistall'     => 'an holl',
'watchlistall2'    => 'pep tra',
'namespacesall'    => 'pep tra',
'monthsall'        => 'an holl',

# E-mail address confirmation
'confirmemail'             => "Kadarnaat ar chomlec'h postel",
'confirmemail_noemail'     => "N'hoc'h eus ket spisaet chomlec'h postel mat ebet en ho [[Special:Preferences|penndibaboù implijer]].",
'confirmemail_text'        => "Rankout a ra ar wiki-mañ bezañ gwiriet ho chomlec'h postel a-raok gallout implijout nep arc'hwel postel. Implijit ar bouton a-is evit kas ur postel kadarnaat d'ho chomlec'h. Ul liamm ennañ ur c'hod a vo er postel. Kargit al liamm-se en o merdeer evit kadarnaat ho chomlec'h.",
'confirmemail_pending'     => "<div class=\"error\">
Ur c'hod kadarnaat zo bet kaset deoc'h dre bostel c'hoazh;
a-raok klask goulenn unan nevez, m'emaoc'h o paouez krouiñ ho kont, e vo fur eus ho perzh gortoz un nebeud munutoù ha leuskel amzer dezhañ d'en em gavout betek ennoc'h.</div>",
'confirmemail_send'        => "Kas ur c'hod kadarnaat",
'confirmemail_sent'        => 'Postel kadarnaat kaset',
'confirmemail_oncreate'    => "Kaset ez eus bet ur c'hod kadarnaat d'ho chomlec'h postel.
N'eus ket ezhomm eus ar c'hod-mañ evit en em lugañ met ret e vo deoc'h ober gantañ evit aotren hini pe hini eus arc'hwelioù postel ar wiki.",
'confirmemail_sendfailed'  => "Dibosupl kas ar postel kadarnaat. Gwiriit ho chomlec'h.

Distro ar posteler : $1",
'confirmemail_invalid'     => "Kod kadarnaat kamm. Marteze eo aet ar c'hod d'e dermen",
'confirmemail_needlogin'   => "Ret eo deoc'h $1 evit kadarnaat ho chomlec'h postel.",
'confirmemail_success'     => "Kadarnaet eo ho chomlec'h postel. A-benn bremañ e c'hallit en em lugañ hag ober ho mad eus ar wiki.",
'confirmemail_loggedin'    => "Kadarnaet eo ho chomlec'h bremañ",
'confirmemail_error'       => 'Ur gudenn zo bet e-ser enrollañ ho kadarnadenn',
'confirmemail_subject'     => "Kadarnadenn chomlec'h postel evit {{SITENAME}}",
'confirmemail_body'        => "Unan bennak, c'hwi moarvat, gant ar chomlec'h IP \$1,
en deus enrollet ur gont \"\$2\" gant ar chomlec'h postel-mañ war lec'hienn {{SITENAME}}.

A-benn kadarnaat eo deoc'h ar gont-se ha gweredekaat
an arc'hwelioù postelerezh war {{SITENAME}}, digorit al liamm a-is en ho merdeer :

\$3

Ma n'eo *ket* bet enrollet ganeoc'h heuilhit al liamm-mañ
evit nullañ kadarnaat ar chomlec'h postel :

\$5

Mont a raio ar c'hod-mañ d'e dermen d'ar \$4.",
'confirmemail_invalidated' => "Nullet eo bet kadarnaat ar chomlec'h postel",
'invalidateemail'          => 'Nullañ kadarnaat ar postel',

# Scary transclusion
'scarytranscludedisabled' => '[Diweredekaet eo an treuzkludañ etrewiki]',
'scarytranscludefailed'   => "[C'hwitet eo bet adtapout ar patrom evit $1; hon digarezit]",
'scarytranscludetoolong'  => '[URL re hir; hon digarez]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Liamm war-gil betek al liamm-mañ :<br />
$1
</div>',
'trackbackremove'   => ' ([Diverkañ $1])',
'trackbacklink'     => 'Liamm war-gil',
'trackbackdeleteok' => 'Diverket mat eo bet al liamm war-gil.',

# Delete conflict
'deletedwhileediting' => "Diwallit : Diverket eo bet ar bajenn-mañ bremañ ha krog e oac'h da zegas kemmoù enni!",
'confirmrecreate'     => "Diverket eo bet ar pennad-mañ gant [[User:$1|$1]] ([[User talk:$1|kaozeal]]) goude ma vije bet kroget ganeoc'h kemmañ anezhañ :
: ''$2''
Kadarnait mar plij e fell deoc'h krouiñ ar pennad-mañ da vat.",
'recreate'            => 'Adkrouiñ',

# HTML dump
'redirectingto' => "Oc'h adkas da [[:$1]]...",

# action=purge
'confirm_purge'        => 'Spurjañ krubuilh ar bajenn-mañ?

$1',
'confirm_purge_button' => 'Mat eo',

# AJAX search
'searchcontaining' => "Klask ar pennadoù enno ''$1''.",
'searchnamed'      => "Klask ar pennadoù anvet ''$1''.",
'articletitles'    => "Pennadoù a grog gant ''$1''",
'hideresults'      => "Kuzhat an disoc'hoù",
'useajaxsearch'    => 'Grit gant ar klask AJAX',

# Multipage image navigation
'imgmultipageprev' => '&larr; pajenn gent',
'imgmultipagenext' => "pajenn war-lerc'h &rarr;",
'imgmultigo'       => 'Mont !',
'imgmultigoto'     => "Mont d'ar bajenn $1",

# Table pager
'ascending_abbrev'         => 'pignat',
'descending_abbrev'        => 'diskenn',
'table_pager_next'         => "Pajenn war-lerc'h",
'table_pager_prev'         => 'Pajenn gent',
'table_pager_first'        => 'Pajenn gentañ',
'table_pager_last'         => 'Pajenn ziwezhañ',
'table_pager_limit'        => 'Diskouez $1 elfenn dre bajenn',
'table_pager_limit_submit' => 'Mont',
'table_pager_empty'        => "Disoc'h ebet",

# Auto-summaries
'autosumm-blank'   => 'O tiverkañ kement danvez zo war ar bajenn',
'autosumm-replace' => "Oc'h erlec'hiañ ar bajenn gant '$1'",
'autoredircomment' => 'Adkas war-du [[$1]]',
'autosumm-new'     => 'Pajenn nevez: $1',

# Live preview
'livepreview-loading' => 'O kargañ...',
'livepreview-ready'   => 'O kargañ... Prest !',
'livepreview-failed'  => "C'hwitet eo rakwelet diouzhtu !
Klaskit rakwelet er mod boutin.",
'livepreview-error'   => 'C\'hwitet kevreañ : $1 "$2"
Klaskit rakwelet er mod boutin.',

# Friendlier slave lag warnings
'lag-warn-normal' => "Marteze ne ziskouezo ket ar roll-mañ an degasadennoù c'hoarvezet $1 eilenn zo hepken.",
'lag-warn-high'   => "Dre m'eo soulgarget ar bankoù roadennoù, marteze ne vo ket gwelet er roll-mañ ar c'hemmoù deuet $1 eilenn zo hepken.",

# Watchlist editor
'watchlistedit-numitems'       => '{{PLURAL:$1|1 pajenn|$1 pajenn}} zo war ho roll evezhiañ, hep kontañ ar pajennoù kaozeal.',
'watchlistedit-noitems'        => "N'eus pajenn ebet war ho roll evezhiañ.",
'watchlistedit-normal-title'   => 'Kemmañ ar roll evezhiañ',
'watchlistedit-normal-legend'  => 'Tennañ ar pajennoù a-ziwar ho roll evezhiañ',
'watchlistedit-normal-explain' => 'Dindan emañ diskouezet ar pajennoù zo war ho roll evezhiañ. Evit tennañ unan, sellet ouzh ar voest e-kichen ha klikañ war Tennañ ar pajennoù. Tu zo da [[Special:Watchlist/raw|gemmañ ar roll (mod diginkl) ivez]].',
'watchlistedit-normal-submit'  => 'Tennañ ar pajennoù',
'watchlistedit-normal-done'    => 'Tennet ez eus bet {{PLURAL:$1|1 pajenn|$1 pajenn}} a-ziwar ho roll evezhiañ :',
'watchlistedit-raw-title'      => 'Kemmañ ar roll evezhiañ (mod diginkl)',
'watchlistedit-raw-legend'     => 'Kemmañ ar roll evezhiañ (mod diginkl)',
'watchlistedit-raw-explain'    => 'Dindan emañ roll ar pajennoù zo war ho roll evezhiañ; gallout a rit kemmañ anezhañ en ur
	ouzhpennañ pe dennañ pajennoù a-ziwarnañ; ur bajenn dre linenn. Ur wech graet, klikañ war Nevesaat ar roll evezhiañ.
	Tu zo da [[Special:Watchlist/edit|implijout an aozer boutin ivez]].',
'watchlistedit-raw-titles'     => 'Titloù :',
'watchlistedit-raw-submit'     => 'Nevesaat ar roll evezhiañ',
'watchlistedit-raw-done'       => 'Nevesaet eo bet ho roll evezhiañ.',
'watchlistedit-raw-added'      => 'Ouzhpennet ez eus bet {{PLURAL:$1|1 pajenn|$1 pajenn}} :',
'watchlistedit-raw-removed'    => 'Tennet ez eus bet {{PLURAL:$1|1 pajenn|$1 pajenn}} :',

# Watchlist editing tools
'watchlisttools-view' => "Gwelet ar c'hemmoù degaset",
'watchlisttools-edit' => 'Gwelet ha kemmañ ar roll evezhiañ',
'watchlisttools-raw'  => 'Kemmañ ar roll (mod diginkl)',

# Iranian month names
'iranian-calendar-m1'  => '1añ miz Jalāli',
'iranian-calendar-m2'  => '2l miz Jalāli',
'iranian-calendar-m3'  => '3e miz Jalāli',
'iranian-calendar-m4'  => '4e miz Jalāli',
'iranian-calendar-m5'  => '5vet miz Jalāli',
'iranian-calendar-m6'  => '6vet miz Jalāli',
'iranian-calendar-m7'  => '7vet miz Jalāli',
'iranian-calendar-m8'  => '8vet miz Jalāli',
'iranian-calendar-m9'  => '9vet miz Jalāli',
'iranian-calendar-m10' => '10vet miz Jalāli',
'iranian-calendar-m11' => '11vet miz Jalāli',
'iranian-calendar-m12' => '12vet miz Jalāli',

# Core parser functions
'unknown_extension_tag' => 'Balizenn astenn "$1" dianav',

# Special:Version
'version'                          => 'Stumm', # Not used as normal message but as header for the special page itself
'version-extensions'               => 'Astennoù staliet',
'version-specialpages'             => 'Pajennoù dibar',
'version-parserhooks'              => 'Galvoù dielfennañ',
'version-variables'                => 'Argemmoù',
'version-other'                    => 'Diseurt',
'version-mediahandlers'            => 'Merer danvez liesvedia',
'version-hooks'                    => 'Galvoù',
'version-extension-functions'      => "Arc'hwelioù an astennoù",
'version-parser-extensiontags'     => 'Balizenn dielfennañ o tont eus an astennoù',
'version-parser-function-hooks'    => "Galv an arc'hwelioù dielfennañ",
'version-skin-extension-functions' => "Arc'hwelioù etrefas astennoù",
'version-hook-name'                => 'Anv ar galv',
'version-hook-subscribedby'        => 'Termenet gant',
'version-version'                  => 'Stumm',
'version-license'                  => 'Aotre implijout',
'version-software'                 => 'Meziant staliet',
'version-software-product'         => 'Produ',
'version-software-version'         => 'Stumm',

# Special:FilePath
'filepath'         => 'Hent moned ur restr',
'filepath-page'    => 'Restr :',
'filepath-submit'  => 'Hent moned',
'filepath-summary' => 'Diskouez a ra ar bajenn-mañ hent moned klok ur restr. Diskouezet eo ar skeudennoù gant ur pizhder uhel, erounit a ra ar restroù all war-eeun gant o frogramm stag.

Merkit anv ar restr hep ar rakger "{{ns:image}} :"',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Klask ar restroù e doubl',
'fileduplicatesearch-summary'  => 'Klask restroù e doubl war diazez talvoudennoù darnek.

Merkañ anv ar restr hep ar rakger "{{ns:image}}:"',
'fileduplicatesearch-legend'   => 'Klask un doubl',
'fileduplicatesearch-filename' => 'Anv ar restr :',
'fileduplicatesearch-submit'   => 'Klask',
'fileduplicatesearch-info'     => '$1 × $2 piksel<br />Ment ar restr : $3<br />seurt MIME : $4',
'fileduplicatesearch-result-1' => 'N\'eus ket a zoubloù heñvel-poch gant ar restr "$1".',
'fileduplicatesearch-result-n' => '{{PLURAL:$2|1 doubl heñvel-poch|$2 doubl heñvel-poch}} zo gant ar restr "$1".',

# Special:SpecialPages
'specialpages'                   => 'Pajennoù dibar',
'specialpages-group-maintenance' => "Rentaoù-kont trezalc'h",
'specialpages-group-other'       => 'Pajennoù dibar all',
'specialpages-group-login'       => 'Em lugañ / en em enrollañ',
'specialpages-group-changes'     => 'Kemmoù diwezhañ ha marilhoù',
'specialpages-group-media'       => 'Danevelloù ar restroù media hag an enporzhiadennoù',
'specialpages-group-users'       => 'An implijerien hag o gwirioù',
'specialpages-group-highuse'     => 'Implij stank ar pajennoù',

);
