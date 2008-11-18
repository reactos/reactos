<?php
/** Kurdish (Latin) (Kurdî / كوردی (Latin))
 *
 * @ingroup Language
 * @file
 *
 * @author Bangin
 */

$namespaceNames = array(
	NS_MEDIA            => 'Medya',
	NS_SPECIAL          => 'Taybet',
	NS_MAIN             => '',
	NS_TALK             => 'Nîqaş',
	NS_USER             => 'Bikarhêner',
	NS_USER_TALK        => 'Bikarhêner_nîqaş',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_nîqaş',
	NS_IMAGE            => 'Wêne',
	NS_IMAGE_TALK       => 'Wêne_nîqaş',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_nîqaş',
	NS_TEMPLATE         => 'Şablon',
	NS_TEMPLATE_TALK    => 'Şablon_nîqaş',
	NS_HELP             => 'Alîkarî',
	NS_HELP_TALK        => 'Alîkarî_nîqaş',
	NS_CATEGORY         => 'Kategorî',
	NS_CATEGORY_TALK    => 'Kategorî_nîqaş'
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Xetekê di bin lînka çêke:',
'tog-highlightbroken'         => 'Lînkan berve gotarên vala biguherîne',
'tog-justify'                 => 'Gotar bi forma "block"',
'tog-hideminor'               => 'Guherandinên biçûk ji listêya guherandinên dawî veşêre',
'tog-extendwatchlist'         => 'Lîsteya şopandinê veke ji bo dîtinê her xeyrandinên meqbûl',
'tog-usenewrc'                => 'Wêşandinê zêdetir (JavaScript gireke)',
'tog-numberheadings'          => 'Sernavan otomatîk bihejmêre',
'tog-showtoolbar'             => 'Tiştên guherandinê bibîne (JavaScript bibîne)',
'tog-editondblclick'          => 'Rûpelan bi du klîkan biguherîne (Java Script gireke)',
'tog-editsection'             => 'Lînkan ji bo guherandinê beşan biwêşîne',
'tog-editsectiononrightclick' => 'Beşekê bi rast-klîkekê biguherîne (JavaScript gireke)',
'tog-showtoc'                 => 'Tabloya naverokê nîşanbide (ji rûpelan bi zêdetirî sê sernavan)',
'tog-rememberpassword'        => 'Qeydkirinê min di vê komputerê da wîne bîrê',
'tog-editwidth'               => 'Cihê guherandinê yê tewrî mezin',
'tog-watchcreations'          => 'Rûpelan, yê min çêkir, têke lîsteya min ya şopandinê',
'tog-watchdefault'            => 'Rûpelan, yê min guhart, têke lîsteya min ya şopandinê',
'tog-watchmoves'              => 'Rûpelan, yê min navî wan guhart, têke lîsteya min ya şopandinê',
'tog-watchdeletion'           => 'Rûpelan, yê min jêbir, têke lîsteya min ya şopandinê',
'tog-minordefault'            => 'Her guherandinekê min bike wek guherandinekî biçûk be',
'tog-previewontop'            => 'Pêşdîtinê gotarê li jorî cihê guherandinê nîşan bide',
'tog-previewonfirst'          => 'Li cem guherandinê yekemîn hercaran pêşdîtinê nîşan bide',
'tog-nocache'                 => "Cache'ê rûpelan biskînîne",
'tog-enotifwatchlistpages'    => 'E-nameyekê ji min ra bişîne eger rûpelek yê ez dişopînim hate guhartin',
'tog-enotifusertalkpages'     => 'E-nameyekê ji min ra bişîne eger guftûgoyê min hate guhartin',
'tog-enotifminoredits'        => 'E-nameyekê ji min ra bişîne eger bes guherandinekî biçûk be jî',
'tog-enotifrevealaddr'        => 'Adrêsa e-nameya min di e-nameyan înformasyonan bêlibike',
'tog-shownumberswatching'     => 'Nîşan bide, çiqas bikarhêner dişopînin',
'tog-fancysig'                => 'Îmze vê lînkkirinê otomatik berve rûpelê bikarhêner',
'tog-externaleditor'          => 'Edîtorekî derva bike "standard" (bes ji zanan ra, tercihên taybetî li komputerê gerekin)',
'tog-externaldiff'            => 'Birnemijekî derva yê diff bike "standard" (bes ji zanan ra, tercihên taybetî li komputerê gerekin)',
'tog-showjumplinks'           => 'Lînkên "Here-berve" qebûlbike',
'tog-uselivepreview'          => 'Pêşdîtinê "live" bikarwîne (JavaScript gireke) (ceribandin)',
'tog-forceeditsummary'        => 'Bibêje, eger kurteyekê vala kê were tomarkirin',
'tog-watchlisthideown'        => 'Guherandinên min ji lîsteya şopandinê veşêre',
'tog-watchlisthidebots'       => "Guherandinên bot'an ji lîsteya şopandinê veşêre",
'tog-watchlisthideminor'      => 'Xeyrandinên biçûk pêşneke',
'tog-nolangconversion'        => 'Konvertkirinê varîyantên zimên biskînîne',
'tog-ccmeonemails'            => 'Kopîyan ji e-nameyan ji min ra bişîne yê min şande bikarhênerên din',
'tog-diffonly'                => 'Li cem nîşandinê versyonan bes ferqê nîşanbide, ne rûpel tevda',
'tog-showhiddencats'          => 'Kategorîyên veşartî bibîne',

'underline-always'  => 'Tim',
'underline-never'   => 'Ne carekê',
'underline-default' => "Tercihên browser'ê da",

'skinpreview' => '(Pêşdîtin)',

# Dates
'sunday'        => 'yekşem',
'monday'        => 'duşem',
'tuesday'       => 'Sêşem',
'wednesday'     => 'Çarşem',
'thursday'      => 'Pêncşem',
'friday'        => 'În',
'saturday'      => 'şemî',
'sun'           => 'Ykş',
'mon'           => 'Duş',
'tue'           => 'Sêş',
'wed'           => 'Çarş',
'thu'           => 'Sêş',
'fri'           => 'Înê',
'sat'           => 'Şem',
'january'       => 'Rêbendan',
'february'      => 'reşemî',
'march'         => 'adar',
'april'         => 'avrêl',
'may_long'      => 'gulan',
'june'          => 'pûşper',
'july'          => 'Tîrmeh',
'august'        => 'tebax',
'september'     => 'rezber',
'october'       => 'kewçêr',
'november'      => 'sermawez',
'december'      => 'Berfanbar',
'january-gen'   => 'Rêbendan',
'february-gen'  => 'Reşemî',
'march-gen'     => 'Adar',
'april-gen'     => 'Avrêl',
'may-gen'       => 'Gulan',
'june-gen'      => 'Pûşper',
'july-gen'      => 'Tîrmeh',
'august-gen'    => 'Gelawêj',
'september-gen' => 'Rezber',
'october-gen'   => 'Kewçêr',
'november-gen'  => 'Sermawez',
'december-gen'  => 'Berfanbar',
'jan'           => 'rêb',
'feb'           => 'reş',
'mar'           => 'adr',
'apr'           => 'avr',
'may'           => 'gul',
'jun'           => 'pşr',
'jul'           => 'tîr',
'aug'           => 'teb',
'sep'           => 'rez',
'oct'           => 'kew',
'nov'           => 'ser',
'dec'           => 'ber',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Kategorî|Kategorîyan}}',
'category_header'                => 'Gotarên di kategoriya "$1" de',
'subcategories'                  => 'Binekategorî',
'category-media-header'          => 'Medya di kategorîya "$1" da',
'category-empty'                 => "''Di vê kategorîyê da niha gotar ya medya tune ne.''",
'hidden-categories'              => '{{PLURAL:$1|Kategorîya veşartî|Kategorîyên veşartî}}',
'hidden-category-category'       => 'Kategorîyên veşartî', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|Di vê kategorîyê da bes ev binkategorîya heye:|Di vê kategorîyê da {{PLURAL:$2|binkategorîyek heye|$2 binkategorî hene}}. Jêr {{PLURAL:$1|binkategorîyek tê|$1 binkategorî tên}} nîşandan.}}',
'category-subcat-count-limited'  => 'Di vê kategorîyê da ev {{PLURAL:$1|binkategorîya heye|$1 binkategorî hene}}.',
'category-article-count'         => '{{PLURAL:$2|Di vê kategorîyê da bes ev rûpela heye:|Di vê kategorîyê da {{PLURAL:$2|rûpelek heye|$2 rûpel hene}}. Jêr {{PLURAL:$1|rûpelek tê|$1 rûpel tên}} nîşandan.}}',
'category-article-count-limited' => 'Ev {{PLURAL:$1|rûpelê|$1 rûpelên}} jêr di vê kategorîyê da {{PLURAL:$1|ye|ne}}.',
'category-file-count'            => "{{PLURAL:$2|Di vê kategorîyê da bes ev data'ya heye:|Di vê kategorîyê da {{PLURAL:$2|data'yek heye|$2 data hene}}. Jêr {{PLURAL:$1|data'yek tê|$1 data tên}} nîşandan.}}",
'category-file-count-limited'    => "Ev {{PLURAL:$1|data'yê|$1 datayên}} jêr di vê kategorîyê da ne.",
'listingcontinuesabbrev'         => 'dewam',

'mainpagetext'      => "<big>'''MediaWiki serketî hate çêkirin.'''</big>",
'mainpagedocfooter' => 'Alîkarî ji bo bikaranîn û guherandin yê datayê Wîkî tu di bin [http://meta.wikimedia.org/wiki/Help:Contents pirtûka alîkarîyê ji bikarhêneran] da dikarê bibînê.

== Alîkarî ji bo destpêkê ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Lîsteya varîyablên konfîgûrasîyonê]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki FAQ]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Lîsteya e-nameyên versyonên nuh yê MediaWiki]',

'about'          => 'Der barê',
'article'        => 'Gotar',
'newwindow'      => '(di rûpelekî din da yê were nîşandan)',
'cancel'         => 'Betal',
'qbfind'         => 'Bibîne',
'qbbrowse'       => 'Bigere',
'qbedit'         => 'Biguherîne',
'qbpageoptions'  => 'Ev rûpel',
'qbpageinfo'     => "Data'yên rûpelê",
'qbmyoptions'    => 'Rûpelên min',
'qbspecialpages' => 'Rûpelên taybet',
'moredotdotdot'  => 'Zêde...',
'mypage'         => 'Rûpela min',
'mytalk'         => 'Rûpela guftûgo ya min',
'anontalk'       => 'Guftûgo ji bo vê IPê',
'navigation'     => 'Navîgasyon',
'and'            => 'û',

# Metadata in edit box
'metadata_help' => "Data'yên meta:",

'errorpagetitle'    => 'Çewtî (Error)',
'returnto'          => 'Bizivire $1.',
'tagline'           => 'Ji {{SITENAME}}',
'help'              => 'Alîkarî',
'search'            => 'Lêbigere',
'searchbutton'      => 'Lêbigere',
'go'                => 'Gotar',
'searcharticle'     => 'Gotar',
'history'           => 'Dîroka rûpelê',
'history_short'     => 'Dîrok / Nivîskar',
'updatedmarker'     => 'hate guherandin ji serlêdana dawî yê min da',
'info_short'        => 'Zanyarî',
'printableversion'  => 'Versiyon ji bo çapkirinê',
'permalink'         => 'Lînkê tim',
'print'             => 'Çap',
'edit'              => 'Biguherîne',
'create'            => 'Çêke',
'editthispage'      => 'Vê rûpelê biguherîne',
'create-this-page'  => 'Vê rûpelê çêke',
'delete'            => 'Jê bibe',
'deletethispage'    => 'Vê rûpelê jê bibe',
'undelete_short'    => 'Dîsa {{PLURAL:$1|guherandinekî|$1 guherandinan}} çêke',
'protect'           => 'Biparêze',
'protect_change'    => 'parastinê biguherîne',
'protectthispage'   => 'Vê rûpelê biparêze',
'unprotect'         => 'Parastinê rake',
'unprotectthispage' => 'Parastina vê rûpelê rake',
'newpage'           => 'Rûpela nû',
'talkpage'          => 'Vê rûpelê guftûgo bike',
'talkpagelinktext'  => 'Nîqaş',
'specialpage'       => 'Rûpela taybet',
'personaltools'     => 'Amûrên şexsî',
'postcomment'       => 'Şîroveyekê bişîne',
'articlepage'       => 'Li naveroka rûpelê binêre',
'talk'              => 'Guftûgo (nîqaş)',
'views'             => 'Dîtin',
'toolbox'           => 'Qutiya amûran',
'userpage'          => 'Rûpelê vê/vî bikarhênerê/î temaşe bike',
'projectpage'       => 'Li rûpelê projektê seke',
'imagepage'         => 'Li rûpelê medyayê seke',
'mediawikipage'     => 'Li rûpelê mêsajê seke',
'templatepage'      => 'Rûpelê şablonê seke',
'viewhelppage'      => 'Rûpelê alîkarîyê seke',
'categorypage'      => 'Li rûpelê kategorîyê seke',
'viewtalkpage'      => 'Guftûgoyê temaşe bike',
'otherlanguages'    => 'Zimanên din',
'redirectedfrom'    => '(Hat ragihandin ji $1)',
'redirectpagesub'   => 'Rûpelê redirect',
'lastmodifiedat'    => 'Ev rûpel carî dawî di $2, $1 de hat guherandin.', # $1 date, $2 time
'viewcount'         => 'Ev rûpel {{PLURAL:$1|carekê|caran}} tê xwestin.',
'protectedpage'     => 'Rûpela parastî',
'jumpto'            => 'Here cem:',
'jumptonavigation'  => 'navîgasyon',
'jumptosearch'      => 'lêbigere',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Der barê {{SITENAME}}',
'aboutpage'            => 'Project:Der barê',
'bugreports'           => 'Raporên çewtiyan',
'bugreportspage'       => 'Project:Raporên çewtiyan',
'copyright'            => 'Ji bo naverokê $1 derbas dibe.',
'copyrightpagename'    => 'Mafên nivîsanê',
'copyrightpage'        => '{{ns:project}}:Mafên nivîsanê',
'currentevents'        => 'Bûyerên rojane',
'currentevents-url'    => 'Project:Bûyerên rojane',
'disclaimers'          => 'Ferexetname',
'disclaimerpage'       => 'Project:Ferexetname',
'edithelp'             => 'Alîkarî ji bo guherandin',
'edithelppage'         => 'Help:Rûpeleke çawa biguherînim',
'faq'                  => 'Pirs û Bersîv (FAQ)',
'faqpage'              => 'Project:Pirs û Bersîv',
'helppage'             => 'Help:Alîkarî',
'mainpage'             => 'Destpêk',
'mainpage-description' => 'Destpêk',
'policy-url'           => 'Project:Qebûlkirin',
'portal'               => 'Portala komê',
'portal-url'           => 'Project:Portala komê',
'privacy'              => "Parastinê data'yan",
'privacypage'          => "Project:Parastinê data'yan",

'badaccess'        => 'Eror li bi dest Hînan',
'badaccess-group0' => 'Tu nikanî vê tiştî bikê.',
'badaccess-group1' => 'Ev tişta yê tu dixazê bikê bes ji bikarhênerên yê grupê $1 tê qebûlkirin.',
'badaccess-group2' => 'Ev tişta yê tu dixazê bikê bes ji bikarhênerên ra ye, yê bi kêmani di grupê $1 da ne.',
'badaccess-groups' => 'Ev tişta yê tu dixazê bikê bes ji bikarhênerên ra ye, yê bi kêmani di grupê $1 da ne.',

'versionrequired'     => 'Verzîyonê $1 ji MediaWiki pêwîste',
'versionrequiredtext' => 'Verzîyonê $1 ji MediaWiki pêwîste ji bo bikaranîna vê rûpelê. Li [[Special:Version|versyon]] seke.',

'ok'                      => 'Temam',
'retrievedfrom'           => 'Ji "$1" hatiye standin.',
'youhavenewmessages'      => '$1 yên te hene ($2).',
'newmessageslink'         => 'Nameyên nû',
'newmessagesdifflink'     => 'Ciyawazî ji revîzyona berê re',
'youhavenewmessagesmulti' => 'Nameyên nih li $1 ji te ra hene.',
'editsection'             => 'biguherîne',
'editold'                 => 'biguherîne',
'viewsourceold'           => 'çavkanî bibîne',
'editsectionhint'         => 'Beşê biguherîne: $1',
'toc'                     => 'Tabloya Naverokê',
'showtoc'                 => 'nîşan bide',
'hidetoc'                 => 'veşêre',
'thisisdeleted'           => '$1 lêsekê ya dîsa çêkê?',
'viewdeleted'             => 'Li $1 seke?',
'restorelink'             => '{{PLURAL:$1|guherandinekî|$1 guherandinên}} jêbirî',
'feedlinks'               => 'Feed:',
'feed-invalid'            => "Feed'ekî neserrast.",
'feed-unavailable'        => 'Feed ji {{SITENAME}} ra tune ne.',
'site-rss-feed'           => '$1 RSS Feed',
'site-atom-feed'          => '$1 Atom Feed',
'page-rss-feed'           => '"$1" RSS Feed',
'page-atom-feed'          => '"$1" Atom Feed',
'red-link-title'          => '$1 (hên nehatîye nivîsandin)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Gotar',
'nstab-user'      => 'Bikarhêner',
'nstab-media'     => 'Medya',
'nstab-special'   => 'Taybet',
'nstab-project'   => 'Rûpela projeyê',
'nstab-image'     => 'Wêne',
'nstab-mediawiki' => 'Peyam',
'nstab-template'  => 'Şablon',
'nstab-help'      => 'Alîkarî',
'nstab-category'  => 'Kategorî',

# Main script and global functions
'nosuchaction'      => 'Çalakiyek bi vê rengê tune',
'nosuchactiontext'  => "Ew tişta yê di wê URL'ê da tê gotin ji MediaWiki netê çêkirin.",
'nosuchspecialpage' => 'Rûpeleke taybet bi vê rengê tune',
'nospecialpagetext' => "<big>'''Rûpelê taybetî yê te xwastîyê tune ye.'''</big>

Hemû rûpelên taybetî di [[Special:SpecialPages|lîsteya rûpelên taybetî]] da werin dîtin.",

# General errors
'error'                => 'Çewtî (Error)',
'databaseerror'        => "Şaşbûnek di database'ê da",
'dberrortext'          => 'Li cem sekirina database <blockquote><tt>$1</tt></blockquote>
ji fonksyonê "<tt>$2</tt>" yê
MySQL ev şaşbûna hate dîtin: "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Li cem sekirina database "$1 ji fonksyonê "<tt>$2</tt>" yê
MySQL ev şaşbûna hate dîtin: "<tt>$3: $4</tt>".',
'noconnect'            => 'Bibexşîne! Çend pirsgrêkên teknîkî heye, girêdan ji pêşkêşvanê (suxrekirê, server) re niha ne gengaz e. <br />
$1',
'nodb'                 => 'Database $1 nikanî hatiba sekirin. Xêra xwe derengtir dîsa bicerbîne.',
'cachederror'          => "Evê jêr bes kopîyek ji cache'ê ye û belkî ne yê niha ye.",
'laggedslavemode'      => 'Zanibe: Ev rûpela belkî guherandinên yê ne niha nîşandide.',
'readonly'             => 'Database hatîye girtin',
'enterlockreason'      => 'Hoyek ji bo bestin binav bike, herweha zemaneke mezende kirî ji bo helgirtina bestinê!',
'readonlytext'         => "Database'ê {{SITENAME}} ji bo guherandinan û gotarên nuh hatîye girtin.

Sedemê girtinê ev e: $1",
'readonly_lag'         => "Database otomatîk hate girtin, ji bo server'î database'î slave kanibe xwe wekî server'î database'î master'ê bike.",
'internalerror'        => 'Şaşbûnekî înternal',
'internalerror_info'   => 'Şaşbûnê înternal: $1',
'filecopyerror'        => 'Datayê „$1“ nikanî çûba berve „$2“ kopîkirin.',
'filerenameerror'      => 'Navê faylê "$1" nebû "$2".',
'filedeleteerror'      => '"$1" nikanî hatiba jêbirin.',
'directorycreateerror' => '"$1" nikanî hatiba çêkirin.',
'filenotfound'         => 'Dosya bi navê "$1" nehat dîtin.',
'fileexistserror'      => 'Di data\'yê "$1" nikanî hatiba nivîsandin, ji ber ku ew data\'ya berê heye.',
'unexpected'           => 'Tiştek yê nehatibû zanîn: "$1"="$2".',
'formerror'            => 'Şaşbûn: Ew nivîs nikanîn hatibana bikaranîn.',
'badarticleerror'      => 'Ev çalakî di vê rûpelê de nabe.',
'cannotdelete'         => 'Ev rûpela nikanî hatiba jêbirin. Meqûle ku kesekî din vê rûpelê jêbir.',
'badtitle'             => 'Sernivîsa nebaş',
'badtitletext'         => "Sernavê rûpelê xastî qedexe ye, vala ye ya lînkekî zimanekî wîkî'yekî din e.",
'perfdisabled'         => "Bibexşîne! Ev fonksîyona ji bo westîyanê server'ê niha hatîye sikinandin.",
'perfcached'           => "Ev data'yan ji cache'ê ne û belkî ne zindî bin.",
'perfcachedts'         => "Ev data'ya hatîye cache'kirin û carê paşîn $1 hate zindîkirin.",
'querypage-no-updates' => "Fonksîyonê zindîkirinê yê vê rûpelê hatîye sikinandin. Data'yên vir netên zindîkirin.",
'wrong_wfQuery_params' => "Parameter'ên şaş ji bo wfQuery()<br />
Fonksîyon: $1<br />
Jêpirskirin: $2",
'viewsource'           => 'Çavkanî',
'viewsourcefor'        => 'ji $1 ra',
'actionthrottled'      => 'Hejmarê guherandinan hatîye hesibandin',
'actionthrottledtext'  => 'Te vê tiştî zêde pir di demekê biçûk da kir. Xêra xwe çend deqa biskine û carekî din bicerbîne.',
'protectedpagetext'    => 'Ev rûpela hatîye parastin ji bo nenivîsandinê.',
'viewsourcetext'       => 'Tu dikarê li çavkanîyê vê rûpelê sekê û wê kopîbikê:',
'protectedinterface'   => "Di vê rûpelê da nivîsandin ji bo interface'î zimanan yê vê software'ê ye. Ew tê parstin ji bo vandalîzm li vê derê çênebe.",
'editinginterface'     => "'''Hîşyar:''' Tu rûpelekî diguherînê yê ji wêşandinê înformasyonan di sistêmê da girîn in. Guherandin di vê rûpelê da ji her bikarhêneran ra yê were dîtin. Ji bo tercemekirinan, xêra xwe di [http://translatewiki.net/wiki/Main_Page?setlang=ku Betawiki] da bixebite, projeyê MediaWiki.",
'sqlhidden'            => '(Jêpirskirina SQL hatîye veşartin)',
'cascadeprotected'     => '<strong>Ev rûpela hatîye parastin ji ber guherandinê, ji ber ku ev rûpela di {{PLURAL:$1|vê rûpelê|van rûpelan da}} tê bikaranîn:
$2

</strong>',
'namespaceprotected'   => "Qebûlkirinê te tune, ku tu vê rûpelê di namespace'a $1 da biguherînê.",
'customcssjsprotected' => 'Qebûlkirinên te tune ne, tu nikanê vê rûpelê biguherînê, ji ber ku di vir da tercihên bikarhênerekî din hene.',
'ns-specialprotected'  => "Rûpel di namespace'a {{ns:special}} nikanin werin guherandin.",
'titleprotected'       => "Rûpelek bi vî navî nikane were çêkirin. Ev astengkirina ji [[User:$1|$1]] bi sedemê ''$2'' hate çêkirin.",

# Login and logout pages
'logouttitle'                => 'Derketina bikarhêner',
'logouttext'                 => '<strong>Tu niha derketî (logged out).</strong><br />
Tu dikarî {{SITENAME}} niha weke bikarhênerekî nediyarkirî bikarbînî, yan jî tu dikarî dîsa bi vî navê xwe yan navekî din wek bikarhêner têkevî. Bila di bîra te de be ku gengaz e hin rûpel mîna ku tu hîn bi navê xwe qeyd kiriyî werin nîşandan, heta ku tu nîşanên çavlêgerandina (browser) xwe jênebî.',
'welcomecreation'            => '== Bi xêr hatî, $1! ==

Hesaba te hat afirandin. Tu dikarî niha tercîhên xwe eyar bikî.',
'loginpagetitle'             => 'Qeyda bikarhêner (User login)',
'yourname'                   => 'Navê te wek bikarhêner (user name)',
'yourpassword'               => 'Şîfreya te (password)',
'yourpasswordagain'          => 'Şîfreya xwe careke din binîvîse',
'remembermypassword'         => 'Şifreya min di her rûniştdemê de bîne bîra xwe.',
'yourdomainname'             => 'Domaînê te',
'externaldberror'            => "Ya şaşbûnek di naskirinê derva heye, ya tu nikarî account'î xwe yê derva bikarwînê.",
'loginproblem'               => '<b>Di qeyda te (login) de pirsgirêkek derket.</b><br />Careke din biceribîne!',
'login'                      => 'Têkeve (login)',
'nav-login-createaccount'    => 'Têkeve an hesabeke nû çêke',
'loginprompt'                => "<b>Eger tu xwe nû qeyd bikî, nav û şîfreya xwe hilbijêre.</b> Ji bo xwe qeyd kirinê di {{SITENAME}} de divê ku ''cookies'' gengaz be.",
'userlogin'                  => 'Têkeve an hesabeke nû çêke',
'logout'                     => 'Derkeve (log out)',
'userlogout'                 => 'Derkeve',
'notloggedin'                => 'Xwe qeyd nekir (not logged in)',
'nologin'                    => 'Tu hêj ne endamî? $1.',
'nologinlink'                => 'Bibe endam',
'createaccount'              => 'Hesabê nû çêke',
'gotaccount'                 => 'Hesabê te heye? $1.',
'gotaccountlink'             => 'Têkeve (login)',
'createaccountmail'          => 'bi e-name',
'badretype'                  => 'Herdu şîfreyên ku te nivîsîn hevûdin nagirin.',
'userexists'                 => 'Ev navî bikarhênerî berê tê bikaranîn. Xêra xwe navekî din bibe.',
'youremail'                  => 'E-maila te*',
'username'                   => 'Navê bikarhêner:',
'uid'                        => "ID'ya bikarhêner:",
'prefs-memberingroups'       => 'Endamê {{PLURAL:$1|grûpê|grûpan}}:',
'yourrealname'               => 'Navê te yê rastî*',
'yourlanguage'               => 'Ziman',
'yourvariant'                => 'Varîyant:',
'yournick'                   => 'Leqeba te (ji bo îmza)',
'badsig'                     => 'Nivîsandinê îmzê ne baş e; xêra xwe nivîsandina HTML seke, ku şaşbûn hene ya na.',
'badsiglength'               => 'Navî te zêde dirêj e; ew gireke di bin {{PLURAL:$1|nîşanekê|nîşanan}} da be.',
'email'                      => 'E-name',
'prefs-help-realname'        => 'Ne gereke. Tu dikarî navî xwe binivisînê, ew ê bi karkirên te were nivîsandin.',
'loginerror'                 => 'Çewtî (Login error)',
'prefs-help-email'           => 'Adrêsa te yê e-nameyan ne gereke were nivîsandin, lê ew qebûldike, ku bikarhênerên din vê naskirinê te kanibin e-nameyan ji te ra bişînin.',
'prefs-help-email-required'  => 'Adrêsa e-nameyan gereke.',
'nocookiesnew'               => "Account'î bikarhêner hatibû çêkirin, lê te xwe qeyd nekirîye. {{SITENAME}} cookie'yan bikartîne ji bo qeydkirinê bikarhêneran. Te cookie'yan girtîye. Xêra xwe cookie'yan qebûlbike, manê tu kanibê bi navî bikarhêner û şîfreya xwe qeydbikê.",
'nocookieslogin'             => 'Ji bo qeydkirina bikarhêneran {{SITENAME}} "cookies" bikartîne. Te fonksîyona "cookies" girtîye. Xêra xwe kerema xwe "cookies" gengaz bike û careke din biceribîne.',
'noname'                     => 'Navê ku te nivîsand derbas nabe.',
'loginsuccesstitle'          => 'Têketin serkeftî!',
'loginsuccess'               => 'Tu niha di {{SITENAME}} de qeydkirî yî wek "$1".',
'nosuchuser'                 => 'Bikarhênera/ê bi navê "$1" tune. Navê rast binivîse an bi vê formê <b>hesabeke nû çêke</b>. (Ji bo hevalên nû "Têkeve" çênabe!)',
'nosuchusershort'            => 'Li vê derê ne bikarhênerek bi navî "<nowiki>$1</nowiki>" heye. Li nivîsandinê xwe seke.',
'nouserspecified'            => 'Navî xwe wek bikarhêner têkê.',
'wrongpassword'              => 'Şifreya ku te nivîsand şaşe. Ji kerema xwe careke din biceribîne.',
'wrongpasswordempty'         => 'Cîhê şîfreya te vala ye. Carekê din binivisîne.',
'passwordtooshort'           => 'Şîfreya te netê qebûlkirin: Şîfreya te gereke bi kêmani {{PLURAL:$1|nîşaneka|$1 nîşanên}} xwe hebe û ne wek navî tê wek bikarhêner be.',
'mailmypassword'             => 'Şîfreyeke nû bi e-mail ji min re bişîne',
'passwordremindertitle'      => 'Şîfreyakekî nuh ji hesabekî {{SITENAME}} ra',
'passwordremindertext'       => 'Kesek (têbê tu, bi IP\'ya $1) xwast ku şîfreyekî nuh ji {{SITENAME}} ($4) ji te ra were şandin. Şîfreya nuh ji bikarhêner "$2" niha "$3" e. Tu dikarî niha têkevê û şîfreya xwe biguherînê.

Eger kesekî din vê xastinê ji te ra xast ya şîfreya kevin dîsa hate bîrê te, tu dikarê guh nedê vê peyamê û tu dikarê bi şîfreya xwe yê kevin hên karbikê.',
'noemail'                    => 'Navnîşana bikarhênerê/î "$1" nehat tomar kirine.',
'passwordsent'               => 'Ji navnîşana e-mail ku ji bo "$1" hat tomarkirin şîfreyekê nû hat şandin. Vê bistîne û dîsa têkeve.',
'blocked-mailpassword'       => "IP'ya te yê ji te niha tê bikaranin ji bo guherandinê ra hatîye astengkirin. Ji bo tiştên şaş çênebin, xastinê te ji bo şifreyeka nuh jî hatîye qedexekirin.",
'eauthentsent'               => 'E-nameyeka naskirinê ji adresa nivîsî ra hate şandin. Berî e-name ji bikarhênerên din bi vê rêkê dikaribim bi te gên, ew adresa û rastbûna xwe gireke werin naskirin. Xêra xwe e-nameyê naskirinê bixûne!',
'throttled-mailpassword'     => 'Berî {{PLURAL:$1|saetekê|$1 saetan}} şîfreyekî nuh hate xastin. Ji bo şaşbûn bi vê fonksyonê çênebin, bes her {{PLURAL:$1|saetekê|$1 saetan}} şîfreyekî nuh dikare were xastin.',
'mailerror'                  => 'Şaşbûnek li cem şandina e-nameyekê: $1',
'acct_creation_throttle_hit' => 'Biborîne! Te hesab $1 vekirine. Tu êdî nikarî hesabên din vekî.',
'emailauthenticated'         => 'Adresa e-nameya hate naskirin: $1.',
'emailnotauthenticated'      => 'Adresa e-nameyan yê te hên nehatîye naskirin. Fonksyonên e-nameyan piştî naskirina te dikarin ji te werin kirin.',
'noemailprefs'               => "'''Te hên adresa e-nameyan nenivîsandîye''', fonksyonên e-nameyan hên ji te ra ne tên qebûlkirin.",
'emailconfirmlink'           => 'E-Mail adresê xwe nasbike',
'invalidemailaddress'        => 'Adresa e-nameyan yê te ne tê qebûlkirin, ji ber ku formata xwe qedexe ye (belkî nîşanên qedexe). Xêra xwe adreseka serrast binivisîne ya vê derê vala bêle.',
'accountcreated'             => 'Account hate çêkirin',
'accountcreatedtext'         => 'Hesabê bikarhêneran ji $1 ra hate çêkirin.',
'createaccount-title'        => 'Çêkirina hesabekî ji {{SITENAME}}',
'createaccount-text'         => 'Kesek ji te ra account\'ekî bikarhêneran "$2" li {{SITENAME}} ($4) çêkir. Şîfreya otomatîk ji "$2" ra "$3" ye.
Niha ê baş be eger tu xwe qeyd bikê û tu şîfreya xwe biguherînê.

Eger account\'a bikarhêneran şaşî hate çêkirin, guhdare vê peyamê meke.',
'loginlanguagelabel'         => 'Ziman: $1',

# Password reset dialog
'resetpass'               => "Şîfreya account'î bikarhêneran şondabibe",
'resetpass_announce'      => 'Te xwe bi şîfreyekê qeydkir, yê bi e-nameyekê ji te ra hate şandin. Ji bo xelaskirinê qeydkirinê, tu niha gireke şîfreyeka nuh binivisînê.',
'resetpass_text'          => '<!-- Nivîsê xwe li vir binivisîne -->',
'resetpass_header'        => 'Şîfreya xwe betalbike',
'resetpass_submit'        => 'Şîfrê bişîne û xwe qedybike',
'resetpass_success'       => 'Şîfreya te hate guherandin! Niha tu tê qeydkirin...',
'resetpass_bad_temporary' => 'Şîfreya te niha netê qebûlkirin. Te berê şîfreyekî nuh tomarkir ya şîfreyekî nuh xast.',
'resetpass_forbidden'     => 'Şîfre nikanin werin guhartin di {{SITENAME}} da',
'resetpass_missing'       => 'Tablo vala ye.',

# Edit page toolbar
'bold_sample'     => 'Nivîsa estûr',
'bold_tip'        => 'Nivîsa estûr',
'italic_sample'   => 'Nivîsa xwar (îtalîk)',
'italic_tip'      => 'Nivîsa xwar (îtalîk)',
'link_sample'     => 'Navê lînkê',
'link_tip'        => 'Lînka hundir',
'extlink_sample'  => 'http://www.example.com navê lînkê',
'extlink_tip'     => 'Lînka derve (http:// di destpêkê de ji bîr neke)',
'headline_sample' => 'Nivîsara sernameyê',
'headline_tip'    => 'Sername asta 2',
'math_sample'     => 'Kurteristê matêmatîk li vir binivisîne',
'math_tip'        => 'Kurteristê matêmatîk (LaTeX)',
'nowiki_sample'   => 'Nivîs ku nebe formatkirin',
'nowiki_tip'      => 'Guh nede formatkirina wiki',
'image_sample'    => 'Mînak.jpg',
'image_tip'       => 'Wêne li hundirê gotarê',
'media_sample'    => 'Mînak.ogg',
'media_tip'       => "Lînka data'yê",
'sig_tip'         => 'Îmze û demxeya wext ya te',
'hr_tip'          => 'Rastexêza berwarî (kêm bi kar bîne)',

# Edit pages
'summary'                   => 'Kurte û çavkanî (Te çi kir?)',
'subject'                   => 'Mijar/sernivîs',
'minoredit'                 => 'Ev guheraniyekê biçûk e',
'watchthis'                 => 'Vê gotarê bişopîne',
'savearticle'               => 'Rûpelê tomar bike',
'preview'                   => 'Pêşdîtin',
'showpreview'               => 'Pêşdîtin',
'showlivepreview'           => 'Pêşdîtinê zindî',
'showdiff'                  => 'Guherandinê nîşan bide',
'anoneditwarning'           => "'''Zanibe:''' Tu neketîyê! Navnîşana IP'ya te wê di dîroka guherandina vê rûpelê da bê tomar kirin.",
'missingsummary'            => "<span style=\"color:#990000;\">'''Zanibe:'''</span> Te nivîsekî kurt ji bo guherandinê ra nenivîsand. Eger tu niha carekî din li Tomar xê, guherandinê te vê nivîsekî kurt yê were tomarkirin.",
'missingcommenttext'        => 'Xêra xwe kurtehîya naverokê li jêr binivisîne.',
'missingcommentheader'      => "<span style=\"color:#990000;\">'''Zanibe:'''</span> Te sernavekî nenivîsandîye. Eger tu niha carekî din li Tomar xê, ev guherandina vê sernavekê yê were tomarkirin.",
'summary-preview'           => 'Pêşdîtinê kurtenivîsê',
'subject-preview'           => 'Pêşdîtinê sernivîsê',
'blockedtitle'              => 'Bikarhêner hat asteng kirin',
'blockedtext'               => "<big>'''Navî te ya IP'ya te hate astengkirin.'''</big>

Astengkirinê te ji $1 hate çêkirin. Sedemê astengkirinê te ev e: ''$2''.

* Destpêkê astengkirinê: $8
* Xelasbûnê astengkirinê: $6
* Astengkirinê ji van ra: $7

Tu dikarî bi $1 ya [[{{MediaWiki:Grouppage-sysop}}|koordînatorekî]] din ra ji astengkirinê te ra dengkê. Tu nikanê 'Ji vê/î bikarhênerê/î re e-name bişîne' bikarwîne eger te di [[Special:Preferences|tercihên xwe]] da adrêsê e-nameyekê nenivîsandîye ya tu ji vê fonksîyonê ra jî hatîyê astengkirin.

IP'yê te yê niha $3 ye, û ID'ya astengkirinê te #$5 e. Xêra xwe yek ji van nimran têke peyamê xwe.",
'autoblockedtext'           => "Adrêsa IP ya te otomatîk hate astenkirin, ji ber ku bikarhênerekî din bi wê kardikir, yê niha ji $1 hate astengkirin.
Sedemê astengkirinê ev e:

: ''$2''

*Destpêka astengkirinê: $8
*Dawîya astengkirinê: $6

Eger tu difikirê ku ev astengkirina ne serrast e, xêra xwe bi $1 ya yekî din ji [[{{MediaWiki:Grouppage-sysop}}|koordînatoran]] ra dengke.

Zanibe ku tu nikanê e-nameya bişînê heta tu di [[Special:Preferences|tercihên xwe]] da adrêsa e-nameyan binivîsînê. Tu nikanê e-nameya bişînê eger ew tişta jî hatîye qedexekirin ji te ra.

'''Eger tu dixazê nivîsarekê bişînê, xêra xwe van tiştan têke nameya xwe:'''

*Koordînator, yê te astengkir: $1
*Sedema astengkirinê: $2
*ID'ya astengkirinê: #$5",
'blockednoreason'           => 'sedem nehatîye gotin',
'blockedoriginalsource'     => "Çavkanîya '''$1''' tê wêşandan:",
'blockededitsource'         => "Nivîsarên '''guherandinên te''' di '''$1''' da tê wêşandan:",
'whitelistedittitle'        => 'Ji bo guherandinê vê gotarê tu gireke xwe qeydbikê.',
'whitelistedittext'         => 'Ji bo guherandina rûpelan, $1 pêwîst e.',
'confirmedittitle'          => 'Ji bo guherandinê, naskirina e-nameya te tê xastin.',
'confirmedittext'           => 'Tu gireke adrêsa e-nameya xwe nasbikê berî tu rûpelan diguherînê. Xêra xwe adrêsa e-nameya ya xwe di [[Special:Preferences|tercihên xwe]] da binivisîne û nasbike.',
'nosuchsectiontitle'        => 'Beşekî wisa tune ye',
'nosuchsectiontext'         => 'Te dixast beşekê biguherînê yê tune ye. Ji ber ku beşa $1 tune ye, guherandinên te jî nikanin werin tomarkirin.',
'loginreqtitle'             => 'Têketin pêwîst e',
'loginreqlink'              => 'têkevê',
'loginreqpagetext'          => 'Tu gireke $1 ji bo dîtina rûpelên din.',
'accmailtitle'              => 'Şîfre hat şandin.',
'accmailtext'               => "Şîfreya '$1' hat şandin ji $2 re.",
'newarticle'                => '(Nû)',
'newarticletext'            => "Ev rûpel hîn tune. Eger tu bixwazî vê rûpelê çêkî, dest bi nivîsandinê bike û piştre qeyd bike. '''Wêrek be''', biceribîne!<br />
Ji bo alîkarî binêre: [[{{MediaWiki:Helppage}}|Alîkarî]].<br />
Eger tu bi şaştî hatî, bizivire rûpela berê.",
'anontalkpagetext'          => "----
''Ev rûpela guftûgo ye ji bo bikarhênerên nediyarkirî ku hîn hesabekî xwe çênekirine an jî bikarnaînin. Ji ber vê yekê divê em wan bi navnîşana IP ya hejmarî nîşan bikin. Navnîşaneke IP dikare ji aliyê gelek kesan ve were bikaranîn. Heger tu bikarhênerekî nediyarkirî bî û bawerdikî ku nirxandinên bê peywend di der barê te de hatine kirin ji kerema xwe re [[Special:UserLogin|hesabekî xwe veke an jî têkeve]] da ku tu xwe ji tevlîheviyên bi bikarhênerên din re biparêzî.''",
'noarticletext'             => 'Ev rûpel niha vala ye, tu dikarî [[Special:Search/{{PAGENAME}}|Di nav gotarên din de li "{{PAGENAME}}" bigere]] an [{{fullurl:{{FULLPAGENAME}}|action=edit}} vê rûpelê biguherînî].',
'userpage-userdoesnotexist' => 'Account\'î bikarhêneran "$1" nehatîye qeydkirin. Xêra xwe seke ku tu dixazê vê rûpelê çêkê/biguherînê.',
'clearyourcache'            => "'''Zanibe:''' Piştî tomarkirinê, tu gireke cache'a browser'î xwe dîsa wînê ji bo dîtina guherandinan. '''Mozilla / Firefor /Safari:''' Kepsa ''Shift'' bigre û li ''Reload'' xe, ya ''Ctrl-Shift-R'' bikepsîne (''Cmd-Shift-R'' li cem Apple Mac); '''IE:''' Kepsa ''Ctrl'' bigre û li ''Reload'' xe, ya li ''Ctrl-F5''; '''Konqueror:''' bes li ''Reload'' xe ya li kepsa ''F5'' xe; bikarhênerên '''Opera''' girekin belkî cache'a xwe tevda di bin ''Tools → Preferences'' da valabikin.",
'usercssjsyoucanpreview'    => "<strong>Tîp:</strong> 'Pêşdîtin' bikarwîne ji bo tu bibînê çawa CSS/JS'ê te yê nuh e berî tomarkirinê.",
'usercsspreview'            => "'''Zanibe ku tu bes CSS'ê xwe pêşdibînê, ew ne hatîye tomarkirin!'''",
'userjspreview'             => "'''Zanibe ku tu bes JavaScript'a xwe dicerbînê, ew hên nehatîye tomarkirin!'''",
'updated'                   => '(Hat taze kirin)',
'note'                      => '<strong>Not:</strong>',
'previewnote'               => '<strong>Ji bîr neke ku ev bi tenê çavdêriyek e, ev rûpel hîn nehat qeyd kirin!</strong>',
'editing'                   => 'Biguherîne: "$1"',
'editingsection'            => 'Tê guherandin: $1 (beş)',
'editingcomment'            => '$1 (şîrove) tê guherandin.',
'editconflict'              => 'Têkçûna guherandinan: $1',
'explainconflict'           => "Ji dema te dest bi guherandinê kir heta niha kesekê/î din ev rûpel guherand.
Jor guhartoya heyî tê dîtîn.
Guherandinên te jêr tên nîşan dan.
Divê tû wan bikî yek.
Heke niha tomar bikî, '''bi tene''' nivîsara qutiya jor wê bê tomarkirin.",
'yourtext'                  => 'Nivîsara te',
'storedversion'             => 'Versiyona qeydkirî',
'editingold'                => '<strong>HÎŞYAR: Tu ser revîsyoneke kevn a vê rûpelê dixebitî.
Eger tu qeyd bikî, hemû guhertinên ji vê revîzyonê piştre winda dibin.
</strong>',
'yourdiff'                  => 'Ciyawazî',
'copyrightwarning'          => "Dîqat bike: Hemû tevkariyên {{SITENAME}} di bin $2 de tên belav kirin (ji bo hûragahiyan li $1 binêre). Eger tu nexwazî ku nivîsên te bê dilrehmî bên guherandin û li gora keyfa herkesî bên belavkirin, li vir neweşîne.<br />
Tu soz didî ku te ev bi xwe nivîsand an jî ji çavkaniyekê azad an geliyane ''(public domain)'' girt.
<strong>BERHEMÊN MAFÊN WAN PARASTÎ (©) BÊ DESTÛR NEWEŞÎNE!</strong>",
'longpagewarning'           => "HIŞYAR: Drêjahiya vê rûpelê $1 kB (kilobyte) e, ev pir e. Dibe ku çend ''browser''
baş nikarin rûpelên ku ji 32 kB drêjtir in biguherînin. Eger tu vê rûpelê beş beş bikî gelo ne çêtir e?",
'protectedpagewarning'      => '<strong>ŞIYARÎ:  Ev rûpel tê parastin. Bi tenê bikarhênerên ku xwediyên mafên "sysop" ne, dikarin vê rûpelê biguherînin.</strong>',
'templatesused'             => 'Şablon di van rûpelan da tê bikaranîn',
'templatesusedpreview'      => 'Şablon yê di vê pêşdîtinê da tên bikaranîn:',
'templatesusedsection'      => 'Şablon yê di vê perçê da tên bikaranîn:',
'template-protected'        => '(tê parastin)',
'template-semiprotected'    => '(nîv-parastî)',
'permissionserrorstext'     => 'Tu nikanê vê tiştî bikê, ji bo {{PLURAL:$1|vê sedemê|van sedeman}}:',
'recreate-deleted-warn'     => "'''Zanibe: Tu kê rûpelekê çêkê yê niha hate jêbirin!'''

Zanibe ku nuhçêkirinê vê rûpelê hêja ye ya na.
Înformasyon li ser jêbirinê vê rûpelê li vir e:",

# "Undo" feature
'undo-success' => 'Ev guherandina kane were şondakirin. Xêra xwe ferqê piştî tomarkirinê bibîne û seke, ku tu ew versîyona dixwazê û tomarbike. Eger te şaşbûnekî kir, xêra xwe derkeve.',
'undo-failure' => 'Ev guherandina nikane were şondakirin ji ber ku guherandinên piştî wê.',
'undo-summary' => 'Rêvîsyona $1 yê [[Special:Contributions/$2|$2]] ([[User talk:$2|guftûgo]]) şondakir',

# Account creation failure
'cantcreateaccounttitle' => 'Account nikanî hatiba çêkirin',
'cantcreateaccount-text' => "Çêkirinê account'an ji vê IP'yê ('''$1''') hatîye qedexekirin ji [[User:$3|$3]].

Sedemê qedexekirinê ji $3 ev e: ''$2''",

# History pages
'viewpagelogs'        => 'Reşahîyên vê rûpelê bibîne',
'nohistory'           => 'Ew rûpel dîroka guherandinê tune.',
'revnotfound'         => 'Revîzyon nehat dîtin',
'currentrev'          => 'Revîzyona niha',
'revisionasof'        => 'Revîzyon a $1',
'previousrevision'    => '←Rêvîzyona kevintir',
'nextrevision'        => 'Revîzyona nûtir→',
'currentrevisionlink' => 'Revîzyona niha nîşan bide',
'cur'                 => 'ferq',
'next'                => 'pêş',
'last'                => 'berê',
'page_first'          => 'yekemîn',
'page_last'           => 'paşîn',
'histlegend'          => 'Legend: (ferq) = cudayî nav vê û versiyon a niha,
(berê) = cudayî nav vê û yê berê vê, B = guhêrka biçûk',
'deletedrev'          => '[jêbir]',
'histfirst'           => 'Kevintirîn',
'histlast'            => 'Nûtirîn',
'historysize'         => '({{PLURAL:$1|1 byte|$1 bytes}})',
'historyempty'        => '(vala)',

# Revision feed
'history-feed-title'          => 'Dîroka versyona',
'history-feed-item-nocomment' => '$1 li $2', # user at time
'history-feed-empty'          => 'Rûpelê xastî tune ye. Belkî ew rûpela hatîye jêbirin ya sernava xwe hatîye guherandin. [[Special:Search|Di wîkîyê da li rûpelên nêzîkî wê bigere]].',

# Revision deletion
'rev-deleted-comment'         => '(nivîs hate jêbirin)',
'rev-deleted-user'            => '(navî bikarhêner hate jêbirin)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Ev verzyona vê rûpelê hatîye jêbirin. Belkî înformasyon di [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} reşahîya jêbirinê] da hebin.
</div>',
'rev-delundel'                => 'nîşan bide/veşêre',
'revisiondelete'              => 'Rêvîsyona jêbibe/dîsa çêke',
'revdelete-legend'            => 'Guherandina qebûlkirina dîtinê',
'revdelete-hide-comment'      => 'Nivîsandinê kurte yê guherandinê veşêre',
'revdelete-hide-user'         => "Navî bikarhêner/IP'yê veşêre",
'revdelete-hide-restricted'   => 'Ev qebûlkirinan ji koordînatoran ra ye jî û ev rûpela tê girtin',
'revdelete-suppress'          => 'Sedemê jêbirinê ji koordînatoran ra jî veşêre',

# History merging
'mergehistory-from' => 'Çavkanîya rûpelê:',

# Diffs
'history-title'           => 'Dîroka versyonên "$1"',
'difference'              => '(Ciyawaziya nav revîzyonan)',
'lineno'                  => 'Dêrra $1:',
'compareselectedversions' => 'Guhartoyan Helsengêne',
'editundo'                => 'Betalbike',
'diff-multi'              => '({{PLURAL:$1|Verzyonekî navberê netê|$1 verzyonên navberê netên}} dîtin.)',

# Search results
'searchresults'         => 'Encamên lêgerînê',
'searchresulttext'      => 'Ji bo zêdetir agahî der barê lêgerînê di {{SITENAME}} de, binêre [[{{MediaWiki:Helppage}}|Searching {{SITENAME}}]].',
'searchsubtitle'        => 'Ji bo query "[[:$1]]"',
'searchsubtitleinvalid' => 'Ji bo query "$1"',
'noexactmatch'          => "'''Rûpeleke bi navê \"\$1\" tune.''' Tu dikarî [[:\$1|vê rûpelê biafirînî]]",
'noexactmatch-nocreate' => "'''Rûpelek bi nava \"\$1\" tune ye.'''",
'titlematches'          => 'Dîtinên di sernivîsên gotaran de',
'notitlematches'        => 'Di nav sernivîsan de nehat dîtin.',
'textmatches'           => 'Dîtinên di nivîsara rûpelan de',
'notextmatches'         => 'Di nivîsarê de nehat dîtin.',
'prevn'                 => '$1 paş',
'nextn'                 => '$1 pêş',
'viewprevnext'          => '($1) ($2) ($3).',
'showingresults'        => "{{PLURAL:$1|Encamek|'''$1''' encam}}, bi #'''$2''' dest pê dike.",
'showingresultsnum'     => '<b>$3</b> encam, bi #<b>$2</b> dest pê dike.',
'powersearch'           => 'Lê bigere',
'searchdisabled'        => '<p>Tu dikarî li {{SITENAME}} bi Google an Yahoo! bigere. Têbînî: Dibe ku encamen lêgerîne ne yên herî nû ne.
</p>',

# Preferences page
'preferences'        => 'Tercîhên min',
'mypreferences'      => 'Tercihên min',
'prefs-edits'        => 'Hejmarê guherandinan:',
'prefsnologin'       => 'Xwe qeyd nekir',
'prefsnologintext'   => 'Tu gireke xwe [[Special:UserLogin|qeydbikê]] ji bo guherandina tercihên bikarhêneran.',
'prefsreset'         => 'Tercih hatin şondakirin.',
'qbsettings-none'    => 'Tune',
'changepassword'     => 'Şîfre biguherîne',
'skin'               => 'Pêste',
'math'               => 'TeX',
'dateformat'         => 'Formata rojê',
'datedefault'        => 'Tercih tune ne',
'datetime'           => 'Dem û rêkewt',
'math_unknown_error' => 'şaşbûnekî nezanîn',
'math_image_error'   => 'Wêşandana PNG nemeşî',
'prefs-personal'     => 'Agahiyên bikarhênerê/î',
'prefs-rc'           => 'Guherandinên dawî',
'prefs-watchlist'    => 'Lîsteya şopandinê',
'prefs-misc'         => 'Eyaren cuda',
'saveprefs'          => 'Tercîhan qeyd bike',
'resetprefs'         => 'Nivîsarên netomarkirî şondabike',
'oldpassword'        => 'Şîfreya kevn',
'newpassword'        => 'Şîfreya nû',
'retypenew'          => 'Şîfreya nû careke din binîvîse',
'textboxsize'        => 'Guheranin',
'rows'               => 'Rêz',
'columns'            => 'sitûn',
'searchresultshead'  => 'Eyarên encamên lêgerinê',
'savedprefs'         => 'Tercîhên te qeyd kirî ne.',
'timezonelegend'     => 'Navçeya demê',
'timezonetext'       => '¹Hejmara saetan têkê, yê navbera navçeya demê te û UTC da ne.',
'localtime'          => 'Demê vê cihê',
'timezoneoffset'     => 'Cudahî:¹',
'servertime'         => "Dema server'ê",
'guesstimezone'      => "Ji browser'î xwe têkê",
'allowemail'         => 'Qebûlbike ku bikarhênerên di e-nameyan ji te ra bişînin',
'default'            => 'asayî',
'files'              => 'Dosya',

# User rights
'userrights'               => 'Îdarekirina mafên bikarhêneran', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => 'Îdarekirina grûpan',
'userrights-user-editname' => 'Navî bikarhênerê têke:',
'editusergroup'            => 'Grûpên bikarhêneran biguherîne',
'editinguser'              => "Mafên bikarhêner '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) tên xeyrandin",
'userrights-editusergroup' => 'Grûpên bikarhêneran biguherîne',
'saveusergroups'           => 'Grûpên bikarhêneran tomarbike',
'userrights-groupsmember'  => 'Endamê:',
'userrights-reason'        => 'Sedemê guherandinê:',
'userrights-no-interwiki'  => 'Mafên te ji bo guherandina mafên bikarhêneran di Wîkîyên din da tune ne.',
'userrights-nodatabase'    => "Database'a $1 tune ye ya ne yê vir e.",
'userrights-nologin'       => "Ji bo guherandina mafên bikarhêneran, tu gereke xwe bi account'eka koordînatorekî [[Special:UserLogin|qeydbikê]].",
'userrights-notallowed'    => "Account'a te mafê xwe tune ye ji bo guherandina mafên bikarhêneran.",

# Groups
'group'            => 'Kom:',
'group-bot'        => 'Bot',
'group-sysop'      => 'Koordînatoran',
'group-bureaucrat' => 'Bûrokrat',
'group-all'        => '(hemû)',

'group-bot-member'        => 'Bot',
'group-sysop-member'      => 'Koordînator',
'group-bureaucrat-member' => 'Burokrat',

'grouppage-sysop' => '{{ns:project}}:Admînistrator',

# User rights log
'rightslog'  => 'Reşahîya mafên bikarhêneran',
'rightsnone' => '(tune)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|guherandinek|guherandin}}',
'recentchanges'                     => 'Guherandinên dawî',
'rcnote'                            => "Jêr {{PLURAL:$1|guherandinek|'''$1''' guherandinên dawî}} di {{PLURAL:$2|rojê|'''$2''' rojên dawî}} de ji $3 şûnde tên nîşan dan.",
'rclistfrom'                        => 'an jî guherandinên ji $1 şûnda nîşan bide.',
'rcshowhideminor'                   => '$1 guherandinên biçûk',
'rcshowhidebots'                    => '$1 bot',
'rcshowhideliu'                     => '$1 bikarhênerê qeydkirî',
'rcshowhideanons'                   => '$1 bikarhênerên neqeydkirî (IP)',
'rcshowhidepatr'                    => '$1 guherandinên kontrolkirî',
'rcshowhidemine'                    => '$1 guherandinên min',
'rclinks'                           => '$1 guherandinên di $2 rojên dawî de nîşan bide<br />$3',
'diff'                              => 'ciyawazî',
'hist'                              => 'dîrok',
'hide'                              => 'veşêre',
'show'                              => 'nîşan bide',
'minoreditletter'                   => 'B',
'newpageletter'                     => 'Nû',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|bikarhênerek|$1 bikarhêner}} vê rûpelê {{PLURAL:$1|dişopîne|dişopînin}}.]',
'newsectionsummary'                 => '/* $1 */ beşeka nuh',

# Recent changes linked
'recentchangeslinked'         => 'Guherandinên peywend',
'recentchangeslinked-summary' => "Ev rûpela taybetî guherandinên dawî ji rûpelên lînkkirî nîşandide. Ew rûpel yê di lîsteya te ya şopandinê da ne bi nivîsa '''estûr''' tên nîşandan.",

# Upload
'upload'                 => 'Wêneyekî barbike',
'uploadbtn'              => 'Wêneyê (ya tiştekî din ya mêdya) barbike',
'reupload'               => 'Dîsa barbike',
'reuploaddesc'           => 'Barkirinê biskîne û dîsa here rûpela barkirinê.',
'uploadnologin'          => 'Xwe qeyd nekir',
'uploadnologintext'      => 'Ji bo barkirina wêneyan divê ku tu [[Special:UserLogin|têkevî]].',
'uploaderror'            => 'Şaşbûn bo barkirinê',
'uploadtext'             => "Berê tu wêneyên nû bar bikî, ji bo dîtin an vedîtina wêneyên ku ji xwe hene binêre: [[Special:ImageList|lîsteya wêneyên barkirî]]. Herwisa wêneyên ku hatine barkirin an jî jê birin li vir dikarî bibînî: [[Special:Log/upload|reşahîya barkirîyan]].

Yek ji lînkên jêr ji bo bikarhînana wêne an file'ê di gotarê de bikar bihîne:
'''<nowiki>[[</nowiki>{{ns:image}}:File.jpg<nowiki>]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}:File.png|alt text<nowiki>]]</nowiki>''',
anjî ji bo file'ên dengî '''<nowiki>[[</nowiki>{{ns:media}}:File.ogg<nowiki>]]</nowiki>'''",
'upload-permitted'       => "Formatên data'yan, yên tên qebûlkirin: $1.",
'upload-preferred'       => "Formatên data'yan, yên tên xastin: $1.",
'upload-prohibited'      => "Formatên data'yan, yên ne tên qebûlkirin: $1.",
'uploadlog'              => 'Reşahîya barkirinê',
'uploadlogpage'          => 'Reşahîya barkirinê',
'filename'               => 'Navê dosyayê',
'filedesc'               => 'Kurte',
'fileuploadsummary'      => 'Kurte:',
'filesource'             => 'Çavkanî:',
'uploadedfiles'          => 'Dosyayên bar kirî',
'ignorewarning'          => 'Hişyarê qebûl neke û dosyayê qeyd bike.',
'ignorewarnings'         => 'Guh nede hîşyaran',
'minlength1'             => "Navên data'yan bi kêmani gireke tîpek be.",
'illegalfilename'        => 'Navî datayê "$1" ne tê qebûlkirin ji ber ku tişt tê da hatine nivîsandin yê qedexe ne. Xêra xwe navî datayê biguherîne û carekî din barbike.',
'badfilename'            => 'Navê vî wêneyî hat guherandin û bû "$1".',
'filetype-badmime'       => 'Data bi formata MIME yê "$1" nameşin werin barkirin.',
'filetype-unwanted-type' => "'''\".\$1\"''' formatekî nexastî ye. Format yên tên qebûlkirin ev in: \$2.",
'filetype-banned-type'   => "'''\".\$1\"''' formatekî qedexe ye. Format yên tên qebûlkirin ev in: \$2.",
'filetype-missing'       => 'Piştnavî datayê tune (wek ".jpg").',
'large-file'             => "Mezinbûna data'yan bila ne ji $1 mezintir bin; ev data'ya $2 mezin e.",
'emptyfile'              => "Data'ya barkirî vala ye. Sedemê valabûnê belkî şaşnivîsek di navê data'yê da ye. Xêra xwe seke, ku tu rast dixazê vê data'yê barbikê.",
'fileexists'             => 'Datayek bi vê navê berê heye. Eger tu niha li „Tomarbike“ xê, ew wêneyê kevin ê here û wêneyê te ê were barkirin di bin wê navê. Di bin <strong><tt>$1</tt></strong> du dikarî sekê, ku di dixwazê wê wêneyê biguherînê. Eger tu naxazê, xêra xwe li „Betal“ xe.',
'fileexists-extension'   => 'Datayek wek vê navê berê heye:<br />
Navî datayê yê tê barkirin: <strong><tt>$1</tt></strong><br />
Navî datayê yê berê heyê: <strong><tt>$2</tt></strong><br />
Xêra xwe navekî din bibîne.',
'fileexists-thumb'       => "<center>'''Wêne yê berê heye'''</center>",
'file-thumbnail-no'      => 'Navî vê datayê bi <strong><tt>$1</tt></strong> destpêdike. Ev dibêje ku ev wêneyekî çûçik e <i>(thumbnail)</i>. Xêra xwe seke, ku belkî versyonekî mezin yê vê wêneyê li cem te heye û wê wêneyê mezintir di bin navî orîjînal da barbike.',
'fileexists-forbidden'   => 'Medyayek bi vê navî heye; xêra xwe şonda here û vê medyayê bi navekî din barbike.
[[Image:$1|thumb|center|$1]]',
'successfulupload'       => 'Barkirin serkeftî',
'uploadwarning'          => 'Hişyara barkirinê',
'savefile'               => 'Dosyayê tomar bike',
'uploadedimage'          => '"$1" barkirî',
'overwroteimage'         => 'versyonekî nuh ji "[[$1]]" hate barkirin',
'uploaddisabled'         => 'Barkirin hatîye qedexekirin',
'uploaddisabledtext'     => "Barkirinê data'yan di {{SITENAME}} da hatine qedexekirin.",
'uploadvirus'            => "Di vê data'yê da vîrûsek heye! Înformasyon: $1",
'sourcefilename'         => 'Navî wêneyê (ya tiştekî din ya mêdya):',
'destfilename'           => 'Navî wêneyê (ya tiştekî din ya mêdya) yê xastî:',
'upload-maxfilesize'     => "Mezinbûna data'yê ya herî mezin: $1",
'watchthisupload'        => 'Vê rûpelê bişopîne',
'filewasdeleted'         => "Data'yek bi vê navê hatibû barkirin û jêbirin. Xêra xwe li $1 seke ku barkirina te hêja ye ya na.",
'upload-wasdeleted'      => "'''Hîşyar: Tu data'yekê bardikê yê berê hatibû jêbirin.'''

Zanibe, ku ev barkirina kê were qebûlkirin ya na.

Înformasyonan li ser jêbirinê kevin ra:",
'filename-bad-prefix'    => 'Nava wê data\'yê, yê tu niha bardikê, bi <strong>"$1"</strong> destpêdike. Kamêrayên dîjîtal wan navan didin wêneyên xwe. Ji kerema xwe navekî baştir binivisîne ji bo mirov zûtir zanibin ku şayeşê vê wêneyê çî ye.',

'license' => 'Lîsens:',

# Special:ImageList
'imagelist_search_for'  => 'Li navî wêneyê bigere:',
'imagelist'             => 'Listeya wêneyan',
'imagelist_date'        => 'Dem',
'imagelist_name'        => 'Nav',
'imagelist_user'        => 'Bikarhêner',
'imagelist_size'        => 'Mezinbûn',
'imagelist_description' => 'Wesif',

# Image description page
'filehist'                  => 'Dîroka datayê',
'filehist-help'             => 'Li demekê xe ji bo dîtina verzyona wê demê',
'filehist-deleteall'        => 'giştika jêbibe',
'filehist-deleteone'        => 'vî jêbibe',
'filehist-revert'           => 'şonda bibe',
'filehist-current'          => 'niha',
'filehist-datetime'         => 'Roj / Katjimêr',
'filehist-user'             => 'Bikarhêner',
'filehist-dimensions'       => 'Mezinbûn',
'filehist-filesize'         => "Mezinbûna data'yê",
'filehist-comment'          => 'Nivîs',
'imagelinks'                => 'Lînkên vî wêneyî',
'linkstoimage'              => 'Di van rûpelan de lînkek ji vî wêneyî re heye:',
'nolinkstoimage'            => 'Rûpelekî ku ji vî wêneyî re girêdankê çêdike nîne.',
'noimage'                   => 'Medyayek bi vê navî tune, lê tu kanî $1',
'noimage-linktext'          => 'wê barbike',
'uploadnewversion-linktext' => 'Versyonekî nû yê vê datayê barbike',

# File reversion
'filerevert'         => '"$1" şondabike',
'filerevert-comment' => 'Nivîs:',
'filerevert-submit'  => 'Şonda',

# File deletion
'filedelete'                  => '$1 jêbibe',
'filedelete-legend'           => 'Data jêbibe',
'filedelete-intro'            => "Tu kê '''[[Media:$1|$1]]''' jêbibê.",
'filedelete-intro-old'        => "Tu niha verzyona '''[[Media:$1|$1]]''' [$4 verzyon, ji $2, saet $3] jêdibê.",
'filedelete-comment'          => 'Nivîs:',
'filedelete-submit'           => 'Jêbibe',
'filedelete-success'          => "'''$1''' hate jêbirin.",
'filedelete-success-old'      => "<span class=\"plainlinks\">Verzyona \$2 ji data'ya '''[[Media:\$1|\$1]]''' di saet \$3 da hate jêbirin.</span>",
'filedelete-nofile'           => "'''$1''' li vê rûpelê tune.",
'filedelete-otherreason'      => 'Sedemên din:',
'filedelete-reason-otherlist' => 'Sedemên din',
'filedelete-reason-dropdown'  => '*Sedemên jêbirina wêneyan
** wêneyeka pîs e
** kopîyek e',
'filedelete-edit-reasonlist'  => 'Sedemên jêbirinê biguherîne',

# MIME search
'download' => 'dabezandin',

# Unwatched pages
'unwatchedpages' => 'Gotar ê ne tên şopandin',

# List redirects
'listredirects' => "Lîsteya redirect'an",

# Unused templates
'unusedtemplates'    => 'Şablonên netên bikaranîn',
'unusedtemplateswlh' => 'lînkên din',

# Random page
'randompage' => 'Rûpelek bi helkeft',

# Statistics
'statistics'    => 'Statîstîk',
'sitestats'     => 'Statîstîkên rûpelê',
'userstats'     => 'Statistîkên bikarhêneran',
'sitestatstext' => "Di ''database'' de {{PLURAL:$1|rûpelek|'''$1''' rûpel}} hene.
Tê de rûpelên guftûgoyê, rûpelên der barê {{SITENAME}}, rûpelên pir kurt (stub), rûpelên ragihandinê (redirect) û rûpelên din ku qey ne gotar in hene.
Derve wan, {{PLURAL:$2|rûpelek|'''$2''' rûpel}} hene, ku qey {{PLURAL:$2|gotarêkî rewa ye|gotarên rewa ne}}.

{{PLURAL:$8|Dosyayek hatîye|'''$8''' dosya hatine}} barkirin.

Ji afirandina Wîkiyê heta roja îro '''$3''' {{PLURAL:$3|cara rûpelek hate|caran rûpelan hatin}} mezekirin û '''$4''' {{PLURAL:$3|cara rûpelek hate|caran rûpelan hatin}} guherandin ji destpêkê {{SITENAME}} da.
Ji ber wê di nîvî de her rûpel '''$5''' carî hatiye guherandin, û nîspeta dîtun û guherandinan '''$6''' e.

Dirêjahîya [http://www.mediawiki.org/wiki/Manual:Job_queue ''job queue''] '''$7''' e.",
'userstatstext' => "Li vir {{PLURAL:$1|[[Special:ListUsers|bikarhênerekî]]|'''$1''' [[Special:ListUsers|bikarhênerên]]}} qeydkirî {{PLURAL:$1|heye|hene}}, ji wan '''$2''' (an '''$4%''') qebûlkirinên $5 {{PLURAL:$2|birîye|birine}}.",

'disambiguations'     => 'Rûpelên cudakirinê',
'disambiguationspage' => 'Template:disambig',

'doubleredirects' => "Redirect'ên ducarî",

'brokenredirects'        => 'Ragihandinên jê bûye',
'brokenredirects-edit'   => '(biguherîne)',
'brokenredirects-delete' => '(jêbibe)',

'withoutinterwiki'        => 'Rûpel vê lînkên berve zimanên din',
'withoutinterwiki-submit' => 'Nîşan bide',

# Miscellaneous special pages
'nbytes'                  => "$1 {{PLURAL:$1|byte|byte'an}}",
'ncategories'             => '$1 {{PLURAL:$1|Kategorî|Kategorîyan}}',
'nlinks'                  => '$1 {{PLURAL:$1|lînk|lînkan}}',
'nmembers'                => '$1 {{PLURAL:$1|endam|endam}}',
'nrevisions'              => '$1 {{PLURAL:$1|guherandin|guherandinan}}',
'nviews'                  => '$1 {{PLURAL:$1|dîtin|dîtin}}',
'lonelypages'             => 'Rûpelên sêwî',
'uncategorizedpages'      => 'Rûpelên bê kategorî',
'uncategorizedcategories' => 'Kategoriyên bê kategorî',
'uncategorizedimages'     => 'Wêneyên vê kategorîyan',
'uncategorizedtemplates'  => 'Şablonên vê kategorîyan',
'unusedcategories'        => 'Kategoriyên ku nayên bi kar anîn',
'unusedimages'            => 'Wêneyên ku nayên bi kar anîn',
'popularpages'            => 'Rûpelên populer',
'wantedcategories'        => 'Kategorîyên tên xwestin',
'wantedpages'             => 'Rûpelên ku tên xwestin',
'mostcategories'          => 'Gotar bi pir kategorîyan',
'shortpages'              => 'Rûpelên kurt',
'longpages'               => 'Rûpelên dirêj',
'deadendpages'            => 'Rûpelên bê dergeh',
'protectedpages'          => 'Rûpelên parastî',
'protectedtitles'         => 'Sernavên parastî',
'listusers'               => 'Lîsteya bikarhêneran',
'newpages'                => 'Rûpelên nû',
'newpages-username'       => 'Navê bikarhêner:',
'ancientpages'            => 'Gotarên kevintirîn',
'move'                    => 'Navê rûpelê biguherîne',
'movethispage'            => 'Vê rûpelê bigerîne',
'notargettitle'           => 'Hedef tune',
'pager-newer-n'           => '{{PLURAL:$1|nuhtirin 1|nuhtirin $1}}',
'pager-older-n'           => '{{PLURAL:$1|kevintirin 1|kevintirin $1}}',

# Book sources
'booksources'               => 'Çavkaniyên pirtûkan',
'booksources-search-legend' => 'Li pirtûkan bigere',
'booksources-go'            => 'Lêbigere',
'booksources-text'          => 'Li vir listek ji lînkên rûpelên, yê pirtûkên nuh ya kevin difiroşin, heye. Hên jî li vir tu dikarî înformasyonan li ser wan pirtûkan tê derxê.',

# Special:Log
'specialloguserlabel'  => 'Bikarhêner:',
'speciallogtitlelabel' => 'Sernav:',
'log'                  => 'Reşahîyan',
'all-logs-page'        => 'Hemû reşahîyan',
'log-search-legend'    => 'Li reşahîyan bigere',
'log-search-submit'    => 'Dê',
'alllogstext'          => 'Ev nîşandana hemû reşahîyên {{SITENAME}} e.

Tu dikarê ji xwe ra reşahîyekê bibê, navî bikarhênerekê ya navî rûpelekê binivisînê û înformasyonan li ser wê bibînê.',
'logempty'             => 'Tişt di vir da tune.',
'log-title-wildcard'   => 'Li sernavan bigere, yê bi vê destpêdikin',

# Special:AllPages
'allpages'          => 'Hemû rûpel',
'alphaindexline'    => '$1 heta $2',
'nextpage'          => 'Rûpela pêşî ($1)',
'prevpage'          => 'Rûpelê berî vê ($1)',
'allpagesfrom'      => 'Pêşdîtina rûpelan bi dest pê kirin ji',
'allarticles'       => 'Hemû gotar',
'allinnamespace'    => 'Hemû rûpelan ($1 boşahî a nav)',
'allnotinnamespace' => "Hemû rûpel (ne di namespace'a $1)",
'allpagesprev'      => 'Pêş',
'allpagesnext'      => 'Paş',
'allpagessubmit'    => 'Biçe',
'allpagesprefix'    => 'Nîşan bide rûpelên bi pêşgira:',
'allpagesbadtitle'  => 'Sernavê rûpelê qedexe bû ya "interwiki"- ya "interlanguage"-pêşnavekî xwe hebû. Meqûle ku zêdertirî tiştekî nikanin werin bikaranîn di sernavê da.',
'allpages-bad-ns'   => 'Namespace\'a "$1" di {{SITENAME}} da tune ye.',

# Special:Categories
'categories'                    => 'Kategorî',
'categoriespagetext'            => 'Di vê wîkiyê de ev kategorî hene:',
'special-categories-sort-count' => 'hatîye rêzkirin li gorî hejmaran',
'special-categories-sort-abc'   => 'hatîye rêzkirin li gorî alfabeyê',

# Special:ListUsers
'listusers-submit'   => 'Pêşêkê',
'listusers-noresult' => 'Ne bikarhênerek hate dîtin.',

# E-mail user
'mailnologin'     => 'Navnîşan neşîne',
'mailnologintext' => 'Te gireke xwe [[Special:UserLogin|qeydbikê]] û adrêsa e-nameyan di [[Special:Preferences|tercihên xwe]] da nivîsandibe ji bo şandina e-nameyan ji bikarhênerên din ra.',
'emailuser'       => 'Ji vê/î bikarhênerê/î re e-name bişîne',
'emailpage'       => 'E-name bikarhêner',
'defemailsubject' => '{{SITENAME}} e-name',
'noemailtitle'    => 'Navnîşana e-name tune',
'emailfrom'       => 'Ji',
'emailto'         => 'Bo',
'emailsubject'    => 'Mijar',
'emailmessage'    => 'Name',
'emailsend'       => 'Bişîne',
'emailccme'       => "Kopîyekê ji min ra ji vê E-Mail'ê ra bişîne.",
'emailsent'       => 'E-name hat şandin',
'emailsenttext'   => 'E-nameya te hat şandin.',

# Watchlist
'watchlist'            => 'Lîsteya min ya şopandinê',
'mywatchlist'          => 'Lîsteya min ya şopandinê',
'watchlistfor'         => "(ji bo '''$1''')",
'nowatchlist'          => 'Tiştek di lîsteya te ya şopandinê da tune ye.',
'watchlistanontext'    => 'Ji bo sekirinê ya xeyrandinê lîsteya te ya şopandinê tu gireke xwe $1.',
'watchnologin'         => 'Te xwe qeyd nekirîye.',
'watchnologintext'     => 'Ji bo xeyrandinê lîsteya te ya şopandinê tu gireke xwe [[Special:UserLogin|qedy kiribe]].',
'addedwatch'           => 'Hat îlawekirinî listeya şopandinê',
'addedwatchtext'       => "Rûpela \"<nowiki>\$1</nowiki>\" çû ser [[Special:Watchlist|lîsteya te ya şopandinê]].
Li dahatû de her guhartoyek li wê rûpelê û rûpela guftûgo ya wê were kirin li vir dêt nîşan dan,

Li rûpela [[Special:RecentChanges|Guherandinên dawî]] jî ji bo hasan dîtina wê, ew rûpel bi '''Nivîsa estûr''' dê nîşan dayîn.


<p>Her dem tu bixwazî ew rûpel li nav lîsteya te ya şopandinê derbikî, li ser wê rûpelê, klîk bike \"êdî neşopîne\".</p>",
'removedwatch'         => 'Ji lîsteya şopandinê hate jêbirin',
'removedwatchtext'     => 'Rûpela "<nowiki>$1</nowiki>" ji lîsteya te ya şopandinê hate jêbirin.',
'watch'                => 'Bişopîne',
'watchthispage'        => 'Vê rûpelê bişopîne',
'unwatch'              => 'Êdî neşopîne',
'unwatchthispage'      => 'Êdî neşopîne',
'notanarticle'         => 'Ne gotar e',
'watchnochange'        => 'Ne rûpelek, yê tu dişopînê, hate xeyrandin di vê wextê da, yê tu dixazê bibînê.',
'watchlist-details'    => '* {{PLURAL:$1|Rûpelek tê|$1 rûpel tên}} şopandin, rûpelên guftûgoyê netên jimartin.',
'wlheader-enotif'      => '* E-mail-şandin çêbû.',
'wlheader-showupdated' => "* Ew rûpel yê hatin xeyrandin jilkî te li wan sekir di '''nivîsa estûr''' tên pêşandin.",
'watchlistcontains'    => 'Di lîsteya şopandina te de {{PLURAL:$1|rûpelek heye|$1 rûpel hene}}.',
'wlnote'               => "Niha {{PLURAL:$1|xeyrandinê|'''$1''' xeyrandinên}} dawî yê {{PLURAL:$2|seetê|'''$2''' seetên}} dawî {{PLURAL:$1|tê|tên}} dîtin.",
'wlshowlast'           => 'Xeyrandînên berî $1 seetan, $2 rojan, ya $3 (di rojên sîyî paşî)',
'watchlist-show-bots'  => "Guherandinên bot'an nîşan bide",
'watchlist-hide-bots'  => "Guherandinên Bot'an veşêre",
'watchlist-show-own'   => 'Guherandinên min pêşke',
'watchlist-hide-own'   => 'Guherandinên min veşêre',
'watchlist-show-minor' => 'Guherandinên biçûk pêşke',
'watchlist-hide-minor' => 'Guherandinên biçûk veşêre',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Bişopîne...',
'unwatching' => 'Neşopîne...',

'enotif_reset'                 => 'Hemû rûpelan wek lêsekirî nîşanbide',
'enotif_newpagetext'           => 'Ev rûpeleke nû ye.',
'enotif_impersonal_salutation' => 'Bikarhênerî {{SITENAME}}',
'changed'                      => 'guhart',
'created'                      => 'afirandî',
'enotif_subject'               => '[{{SITENAME}}] Rûpelê "$PAGETITLE" ji $PAGEEDITOR hate $CANGEDORCREATED',
'enotif_anon_editor'           => 'Bikarhênerê neqeydkirî $1',
'enotif_body'                  => '$WATCHINGUSERNAME,


Rûpelê {{SITENAME}} $PAGETITLE hate $CHANGEDORCREATED di rojê $PAGEEDITDATE da ji $PAGEEDITOR, xêra xwe li $PAGETITLE_URL ji versyonê niha ra seke.

$NEWPAGE

Kurtnivîsê wê bikarhênerî: $PAGESUMMARY $PAGEMINOREDIT

Ji wî bikarhênerî mêsajekî binivisîne:
E-name: $PAGEEDITOR_EMAIL
{{SITENAME}}: $PAGEEDITOR_WIKI

Heta tu vê guherandinê senekê, mêsajên din ji ber ku guherandinê wê rûpelê yê netên.

             Sîstêmê mêsajan yê {{SITENAME}}

--
Eger tu dixazê lîstêya xwe yê şopandinê biguherînê, li
{{fullurl:{{ns:special}}:Watchlist/edit}} seke.

"Feedback" û alîkarîyê din:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Rûpelê jê bibe',
'confirm'                     => 'Pesend bike',
'excontent'                   => "Naveroka berê: '$1'",
'excontentauthor'             => "Nawerokê wê rûpelê ew bû: '$1' (û tenya bikarhêner '$2' bû)",
'exbeforeblank'               => "Nawerok berî betal kirinê ew bû: '$1'",
'exblank'                     => 'rûpel vala bû',
'delete-confirm'              => 'Jêbirina "$1"',
'delete-legend'               => 'Jêbirin',
'historywarning'              => 'Hîşyar: Ew rûpel ku tu dixwazî jê bibî dîrokek heye:',
'confirmdeletetext'           => 'Tu kê niha rûpelekê bi tev dîroka wê jêbibê. Xêra xwe zanibe tu kê niha çi bikê û zanibe, çi di wîkîyê da yê bibe. Hên jî seke, ku ev jêbirina bi [[{{MediaWiki:Policy-url}}|mafên wîkîyê]] ra dimeşin ya na.',
'actioncomplete'              => 'Çalakî temam',
'deletedtext'                 => '"<nowiki>$1</nowiki>" hat jêbirin. Ji bo qeyda rûpelên ku di dema nêzîk de hatin jêbirin binêre $2.',
'deletedarticle'              => '"$1" hat jêbirin',
'dellogpage'                  => 'Reşahîya jêbirin',
'dellogpagetext'              => 'Li jêr lîsteyek ji jêbirinên dawî heye.',
'deletionlog'                 => 'reşahîya jêbirin',
'reverted'                    => 'Hate şondabirin berve verzyonekî berê',
'deletecomment'               => 'Sedema jêbirinê',
'deleteotherreason'           => 'Sedemekî din:',
'deletereasonotherlist'       => 'Sedemekî din',
'deletereason-dropdown'       => "*Sedemên jêbirinê
** vandalîzm
** vala
** ne girek e
** ne gotarek e
** ceribandina IP'yekê",
'delete-edit-reasonlist'      => 'Sedemên jêbirinê biguherîne',
'delete-toobig'               => 'Dîroka vê rûpelê pir mezin e, zêdetirî $1 guherandin. Jêbirina van rûpelan hatîye sînorkirin, ji bo pir şaşbûn (error) di {{SITENAME}} da çênebin.',
'delete-warning-toobig'       => "Dîroka vê rûpelê pir mezin e, zêdetirî $1 guherandin. Jêbirina van rûpelan dikarin şaşbûnan di database'ê {{SITENAME}} da çêkin; zandibe tu çi dikê!",
'rollback_short'              => 'Bizivirîne pêş',
'rollbacklink'                => 'bizivirîne pêş',
'cantrollback'                => "Guharto naye vegerandin; bikarhêrê dawî, '''tenya''' nivîskarê wê rûpelê ye.",
'alreadyrolled'               => 'Guherandina dawiya [[$1]]
bi [[User:$2|$2]] ([[User talk:$2|guftûgo]]) venizivre; keseke din wê rûpelê zivrandiye an guherandiye.

Guhartoya dawî bi [[User:$3|$3]] ([[User talk:$3|guftûgo]]).',
'editcomment'                 => 'Kurtenivîsê guherandinê ev bû: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Guherandina $2 hat betal kirin, vegerand guhartoya dawî ya $1', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => 'Guherandina $1 şondakir; dîsa guharte verzyona $2.',
'protectlogpage'              => 'Reşahîya parastîyan',
'protectedarticle'            => 'parastî [[$1]]',
'modifiedarticleprotection'   => 'parastina "[[$1]]" guherand',
'unprotectedarticle'          => '"[[$1]]" niha vê parastin e',
'protect-title'               => 'parastina "$1" biguherîne',
'protect-legend'              => 'Parastinê teyîd bike',
'protectcomment'              => 'Sedema parastinê',
'protectexpiry'               => 'Heta:',
'protect_expiry_invalid'      => 'Dema nivîsandî şaş e.',
'protect_expiry_old'          => 'Dema girtinê di zemanê berê da ye.',
'protect-default'             => '(standard)',
'protect-level-autoconfirmed' => 'Bikarhênerên neqeydkirî astengbike',
'protect-level-sysop'         => 'Bes koordînatoran (admînan)',
'protect-expiring'            => 'heta rojê $1 (UTC)',
'restriction-type'            => 'Destûr:',

# Restrictions (nouns)
'restriction-edit'   => 'Biguherîne',
'restriction-move'   => 'Nav biguherîne',
'restriction-create' => 'Çêke',

# Restriction levels
'restriction-level-sysop'         => 'tev-parastî',
'restriction-level-autoconfirmed' => 'nîv-parastî',

# Undelete
'undelete'                 => 'Li rûpelên jêbirî seke',
'undeletepage'             => 'Rûpelên jêbirî bibîne û dîsa çêke',
'viewdeletedpage'          => 'Rûpelên vemirandî seke',
'undeletepagetext'         => 'Rûpelên jêr hatine jêbirin, lê ew hên di arşîvê da ne û dikarin dîsa werin çêkirin. Ev arşîva piştî demekê tê pakkirin.',
'undeleteextrahelp'        => "Ji bo dîsaçêkirina vê rûpelê, li checkbox'an nexe û li '''''Dîsa çêke''''' klîk bike. Eger tu naxazî ku hemû verzyon dîsa werin çêkirin, li checkbox'ên wan verzyonan xe, yê tu dixazî dîsa çêkê û paşê li '''''Dîsa çêke'''' klîk bike. Eger tu li '''''Biskine''''' xê, hemû checkbox û cihê sedemê yê werin valakirin.",
'undeleterevisions'        => '$1 {{PLURAL:$1|rêvîzyonek çû|rêvîzyon çûn}} arşîv',
'undeletehistory'          => 'Eger tu vê rûpelê dîsa çêkê, hemû rêvîzyon ê dîsa di dîrokê da werin çêkirin. Eger rûpeleka nuh ji dema jêbirinê da hatîye çêkirin, ew rêvîzyon ê werin pêşî diroka nuh.',
'undelete-revision'        => 'Rêvîzyonên jêbirî yê $1 (di $2) ji $3:',
'undelete-nodiff'          => 'Rêvîzyonên berê nehatin dîtin.',
'undeletebtn'              => 'Dîsa çêke!',
'undeletelink'             => 'dîsa çêke',
'undeletereset'            => 'Biskine',
'undeletecomment'          => 'Sedem:',
'undeletedarticle'         => '"[[$1]]" dîsa çêkir',
'undeletedrevisions'       => '{{PLURAL:$1|Verzyonek dîsa hate|$1 verzyon dîsa hatin}} çêkirin',
'undeletedrevisions-files' => '{{PLURAL:$1|Verzyonek|$1 verzyon}} û {{PLURAL:$2|medyayek hate|$2 medya hatin}} çêkirin',
'undeletedfiles'           => '{{PLURAL:$1|Medyayek hate|$1 medya hatin}} çêkirin',
'undeletedpage'            => "<big>'''$1 dîsa hate çêkirin'''</big>

Ji bo jêbirinan û çêkirinên nuh ra, xêra xwe di [[Special:Log/delete|reşahîya jêbirinê]] da seke.",
'undelete-header'          => '[[Special:Log/delete|Reşahîya jêbirinê]] bibîne ji bo rûpelên jêbirî.',
'undelete-search-box'      => 'Rûpelên jêbirî lêbigere',
'undelete-search-prefix'   => 'Rûpela pêşe min ke ê bi vê destpêdîkin:',
'undelete-search-submit'   => 'Lêbigere',

# Namespace form on various pages
'namespace'      => 'Boşahîya nav:',
'invert'         => 'Hilbijardinê pêçewane bike',
'blanknamespace' => '(Serekî)',

# Contributions
'contributions' => 'Beşdariyên vê bikarhêner',
'mycontris'     => 'Beşdariyên min',
'contribsub2'   => 'Ji bo $1 ($2)',
'uctop'         => ' (ser)',
'month'         => 'Ji mihê (û zûtir):',
'year'          => 'Ji salê (û zûtir):',

'sp-contributions-newbies'     => 'Bes beşdarîyên bikarhênerê nû pêşêkê',
'sp-contributions-newbies-sub' => 'Ji bikarhênerên nû re',
'sp-contributions-blocklog'    => 'Reşahîya astengkirinê',
'sp-contributions-search'      => 'Li beşdarîyan bigere',
'sp-contributions-username'    => 'Adresê IP ya navî bikarhêner:',
'sp-contributions-submit'      => 'Lêbigere',

# What links here
'whatlinkshere'            => 'Lînk yê tên ser vê rûpelê',
'whatlinkshere-title'      => 'Rûpelan, yê berve $1 tên',
'whatlinkshere-page'       => 'Rûpel:',
'linklistsub'              => '(Listeya lînkan)',
'linkshere'                => "Ev rûpel tên ser vê rûpelê '''„[[:$1]]“''':",
'nolinkshere'              => "Ne ji rûpelekê lînk tên ser '''„[[:$1]]“'''.",
'nolinkshere-ns'           => "Ne lînkek berve '''[[:$1]]''' di vê namespace'a da tê.",
'isredirect'               => 'rûpela ragihandinê',
'istemplate'               => 'tê bikaranîn',
'whatlinkshere-prev'       => '{{PLURAL:$1|yê|$1 yên}} berê',
'whatlinkshere-next'       => '{{PLURAL:$1|yê|$1 yên}} din',
'whatlinkshere-links'      => '← lînkan',
'whatlinkshere-hideredirs' => "$1 redirect'an",
'whatlinkshere-hidelinks'  => '$1 lînkan',
'whatlinkshere-hideimages' => '$1 lînkên wêneyan',

# Block/unblock
'blockip'                     => 'Bikarhêner asteng bike',
'blockip-legend'              => 'Bikarhêner asteng bike',
'blockiptext'                 => 'Ji bo astengkirina nivîsandinê ji navnîşaneke IP an bi navekî bikarhêner, vê formê bikarbîne.
Ev bes gireke were bikaranîn ji bo vandalîzmê biskinîne (bi vê [[{{MediaWiki:Policy-url}}|qebûlkirinê]]).

Sedemekê binivîse!',
'ipaddress'                   => "adresê IP'yekê",
'ipadressorusername'          => "adresê IP'yekê ya navekî bikarhênerekî",
'ipbexpiry'                   => 'Dem:',
'ipbreason'                   => 'Sedem',
'ipbreasonotherlist'          => 'Sedemekî din',
'ipbreason-dropdown'          => '*Sedemên astengkirinê (bi tevayî)
** vandalîzm
** înformasyonên şaş kir gotarekê
** rûpelê vala kir
** bes lînkan dikir rûpelan
** kovan dikir gotaran
** heqaretkirin
** pir accounts dikaranîn
** navekî pîs',
'ipbanononly'                 => 'Bes bikarhênerî veşartî astengbike (bikarhênerên qeydkirî bi vê IP-adresê ne tên astengkirin).',
'ipbcreateaccount'            => "Çêkirina account'an qedexebike.",
'ipbemailban'                 => 'Şandinê E-Nameyan qedexe bike.',
'ipbenableautoblock'          => "Otomatîk IP'yên niha û yên nuh yê vê bikarhênerê astengbike.",
'ipbsubmit'                   => 'Vê bikarhêner asteng bike',
'ipbother'                    => 'demekî din',
'ipboptions'                  => '1 seet:1 hour,2 seet:2 hours,6 seet:6 hours,1 roj:1 day,3 roj:3 days,1 hefte:1 week,2 hefte:2 weeks,1 mihe:1 month,3 mihe:3 months,1 sal:1 year,ji her demê ra:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'yên din',
'ipbotherreason'              => 'Sedemekî din',
'ipbhidename'                 => 'Navî bikarhêner / adresê IP ji "pirtûkê" astengkirinê, lîsteya astengkirinên nuh û lîsteya bikarhêneran veşêre',
'ipbwatchuser'                => 'Rûpelên bikarhênerê û guftûgoyê bişopîne',
'badipaddress'                => 'Bikarhêner bi vî navî tune',
'blockipsuccesssub'           => 'Blok serkeftî',
'blockipsuccesstext'          => '"$1" hat asteng kirin.
<br />Bibîne [[Special:IPBlockList|Lîsteya IP\'yan hatî asteng kirin]] ji bo lîsteya blokan.',
'ipb-edit-dropdown'           => 'Sedemên astengkirinê',
'ipb-unblock-addr'            => 'Astengkirinê $1 rake',
'ipb-unblock'                 => "Astengkirina bikarhênerekî ya adrêsa IP'yekê rake",
'ipb-blocklist-addr'          => 'Astengkirinên niha ji $1 ra bibîne',
'ipb-blocklist'               => 'Astengkirinên niha bibîne',
'unblockip'                   => "IP'yekê dîsa veke",
'unblockiptext'               => "Nivîsara jêr bikarwîne ji bo qebûlkirina nivîsandinê bikarhênerekî ya IP'yeka berê astengkirî.",
'ipusubmit'                   => 'Astengkirina vê adrêsê rake',
'unblocked'                   => '[[User:$1|$1]] niha vê astengkirinê ye',
'unblocked-id'                => '$1 dîsa vê astengkirinê ye',
'ipblocklist'                 => "Listek ji adresên IP'yan û bikarhêneran yê hatine astengkirin",
'ipblocklist-legend'          => 'Bikarhênerekî astengkirî bibîne',
'ipblocklist-username'        => "Navî bikarhêner ya adrêsa IP'yê:",
'ipblocklist-submit'          => 'Lêbigere',
'blocklistline'               => '$1, $2 $3 asteng kir ($4)',
'infiniteblock'               => 'ji her demê ra',
'expiringblock'               => 'heta $1',
'anononlyblock'               => 'bes kesên netên zanîn',
'noautoblockblock'            => 'astengkirina otomatîk hatîye temirandin',
'createaccountblock'          => "çêkirina account'an hatîye qedexekirin",
'emailblock'                  => 'E-Mail hate girtin',
'ipblocklist-empty'           => 'Lîsteya astengkirinê vala ye.',
'ipblocklist-no-results'      => "Ew IP'ya ya bikarhênera nehatîye astengkirin.",
'blocklink'                   => 'asteng bike',
'unblocklink'                 => 'betala astengê',
'contribslink'                => 'Beşdarî',
'autoblocker'                 => 'Otomatîk hat bestin jiberku IP-ya we û ya "[[User:$1|$1]]" yek in. Sedem: "\'\'\'$2\'\'\'"',
'blocklogpage'                => 'Reşahîya astengkirinê',
'blocklogentry'               => '"[[$1]]" ji bo dema $2 $3 hatîye asteng kirin',
'blocklogtext'                => "Ev reşahîyek ji astengkirinên û rakirina astengkirinên bikarhêneran ra ye. Adrêsên IP'yan, yê otomatîk hatine astengkirin, nehatine nivîsandin. [[Special:IPBlockList|Lîsteya IP'yên astengkirî]] bibîne ji bo dîtina astengkirinên IP'yan.",
'unblocklogentry'             => 'astenga "$1" betalkir',
'block-log-flags-anononly'    => 'bes bikarhênerên neqeydkirî',
'block-log-flags-nocreate'    => "çêkirina account'an hatîye qedexekirin",
'block-log-flags-noautoblock' => 'astengkirina otomatik tune',
'block-log-flags-noemail'     => 'Şandina e-nameyan hatîye qedexekirin',
'ipb_expiry_invalid'          => 'Dem ne serrast e.',
'ipb_already_blocked'         => '"$1" berê hatîye astengkirin',
'ipb_cant_unblock'            => "Şaşbûn: ID'ya astengkirinê $1 nehate dîtin. Astengkirinê xwe niha belkî hatîye rakirin.",
'blockme'                     => 'Min astengbike',
'proxyblocksuccess'           => 'Çêbû.',
'sorbsreason'                 => "Adrêsa IP ya te ji DNSBL'a {{SITENAME}} wek proxy'eka vekirî tê naskirin.",
'sorbs_create_account_reason' => "Adrêsa IP ya te ji DNSBL'a {{SITENAME}} wek proxy'eka vekirî tê naskirin. Tu nikarê account'ekê ji xwe ra çêkê.",

# Move page
'move-page-legend'        => 'Vê rûpelê bigerîne',
'movepagetalktext'        => "Rûpela '''guftûgoyê''' vê rûpelê wê were, eger hebe, gerandin. Lê ev tişta nameşe, eger

*berê guftûgoyek bi wê navê hebe ya
*tu tiştekî jêr hilbijêrê.

Eger ev mişkla çêbû, tu gireke vê rûpelê bi xwe bigerînê.

Xêra xwe navî nuh û sedemê navgerandinê binivisîne.",
'movearticle'             => 'Rûpelê bigerîne',
'movenotallowed'          => 'Tu nikanê navên gotarên {{SITENAME}} biguherînê.',
'newtitle'                => 'Sernivîsa nû',
'move-watch'              => 'Vê rûpelê bişopîne',
'movepagebtn'             => 'Vê rûpelê bigerîne',
'pagemovedsub'            => 'Gerandin serkeftî',
'articleexists'           => 'Rûpela bi vî navî heye, an navê ku te hilbijart derbas nabe. Navekî din hilbijêre.',
'movedto'                 => 'bû',
'movetalk'                => "Rûpela '''guftûgo''' ya wê jî bigerîne, eger gengaz be.",
'1movedto2'               => '$1 çû cihê $2',
'1movedto2_redir'         => '$1 çû cihê $2 ser redirect',
'movelogpage'             => 'Reşahîya nav guherandin',
'movelogpagetext'         => 'Li jêr lîsteyek ji rûpelan ku navê wan hatiye guherandin heye.',
'movereason'              => 'Sedem',
'revertmove'              => 'şondabike',
'delete_and_move'         => 'Jêbibe û nav biguherîne',
'delete_and_move_text'    => '== Jêbirin gireke ==

Rûpela "[[:$1]]" berê heye. Tu rast dixazê wê jêbibê ji bo navguherandinê ra?',
'delete_and_move_confirm' => 'Erê, wê rûpelê jêbibe',
'delete_and_move_reason'  => 'Jêbir ji bo navguherandinê',

# Namespace 8 related
'allmessages'        => 'Hemû mesajên sîstemê',
'allmessagesname'    => 'Nav',
'allmessagescurrent' => 'Texta niha',
'allmessagestext'    => 'Ev lîsteya hemû mesajên di namespace a MediaWiki: de ye.',

# Thumbnails
'thumbnail-more' => 'Mezin bike',
'filemissing'    => 'Data tune',

# Special:Import
'import'                  => 'Rûpelan wîne (import)',
'import-interwiki-submit' => 'Wîne',
'importtext'              => 'Please export the file from the source wiki using the {{ns:special}}:Export utility, save it to your disk and upload it here.',
'importstart'             => 'Rûpel tên împortkirin...',
'importnopages'           => 'Ne rûpelek ji împortkirinê ra heye.',
'importfailed'            => 'Împort nebû: $1',
'importbadinterwiki'      => 'Interwiki-lînkekî xerab',
'importnotext'            => 'Vala an nivîs tune',
'importsuccess'           => 'Împort çêbû!',

# Import log
'importlogpage' => 'Reşahîya împortê',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Rûpela min a şexsî',
'tooltip-pt-anonuserpage'         => 'The user page for the ip you',
'tooltip-pt-mytalk'               => 'Rûpela guftûgo ya min',
'tooltip-pt-preferences'          => ',Tercîhên min',
'tooltip-pt-watchlist'            => 'The list of pages you',
'tooltip-pt-mycontris'            => 'Lîsteya tevkariyên min',
'tooltip-pt-logout'               => 'Derkeve (Log out)',
'tooltip-ca-talk'                 => 'guftûgo û şîrove ser vê rûpelê',
'tooltip-ca-edit'                 => 'Vê rûpelê biguherîne! Berê qeydkirinê bişkoka "Pêşdîtin',
'tooltip-ca-addsection'           => 'Beşekê zêde bike.',
'tooltip-ca-viewsource'           => 'Ev rûpela tê parastin. Tu dikarê bes li çavkanîyê sekê.',
'tooltip-ca-history'              => 'Versyonên berê yên vê rûpelê.',
'tooltip-ca-protect'              => 'Vê rûplê biparêze',
'tooltip-ca-delete'               => 'Vê rûpelê jê bibe',
'tooltip-ca-move'                 => 'Navekî nû bide vê rûpelê',
'tooltip-ca-watch'                => 'Vê rûpelê têke nav lîsteya te ya şopandinê',
'tooltip-ca-unwatch'              => 'Vê rûpelê ji lîsteya te ya şopandinê rake',
'tooltip-search'                  => 'Li vê wikiyê bigêre',
'tooltip-p-logo'                  => 'Destpêk',
'tooltip-n-mainpage'              => 'Biçe Destpêkê',
'tooltip-n-portal'                => 'Înformasyon li ser {{SITENAME}}, tu çi dikarê bikê, tu çi li ku dikarê bîbînê',
'tooltip-n-recentchanges'         => "Lîsteya guherandinên dawî di vê Wîkî'yê da.",
'tooltip-n-randompage'            => 'Rûpelekî helkeft biwêşîne',
'tooltip-n-help'                  => 'Bersivên ji bo pirsên te.',
'tooltip-t-whatlinkshere'         => 'Lîsteya hemû rûpelên ku ji vê re grêdidin.',
'tooltip-t-recentchangeslinked'   => 'Recent changes in pages linking to this page',
'tooltip-t-emailuser'             => 'Jê re name bişîne',
'tooltip-t-upload'                => "Wêneyan ya data'yan barbike",
'tooltip-t-specialpages'          => 'Lîsteya hemû rûpelên taybetî',
'tooltip-ca-nstab-user'           => 'Rûpela bikarhênerê/î temaşe bike',
'tooltip-ca-nstab-special'        => 'This is a special page, you can',
'tooltip-ca-nstab-template'       => 'Şablonê nîşanbide',
'tooltip-save'                    => 'Guherandinên xwe tomarbike',
'tooltip-compareselectedversions' => 'Cudatiyên guhartoyên hilbijartî yên vê rûpelê bibîne.',
'tooltip-upload'                  => 'Barkirinê destpêke',

# Stylesheets
'monobook.css' => '*.rtl
 {
  dir:rtl;
  text-align:right;
  font-family: "Tahoma", "Unikurd Web", "Arial Unicode MS", "DejaVu Sans", "Lateef", "Scheherazade", "ae_Rasheeq", sans-serif, sans;
 }

 /*Make the site more suitable for Soranî users */
 h1 {font-family: "Tahoma", "Arial Unicode MS", sans-serif, sans, "Unikurd Web", "Scheherazade";}
 h2 {font-family: "Tahoma", "Arial Unicode MS", sans-serif, sans, "Unikurd Web", "Scheherazade";}
 h3 {font-family: "Tahoma", "Arial Unicode MS", sans-serif, sans, "Unikurd Web", "Scheherazade";}
 body {font-family: "Tahoma", "Arial Unicode MS", sans-serif, sans, "Unikurd Web", "Scheherazade";}
 textarea {font-family: Lucida Console, Tahoma;}
 pre {font-family: Lucida Console, Tahoma;}',

# Scripts
'common.js' => '/* JavaScript */

/* Workaround for language variants */

// Set user-defined "lang" attributes for the document element (from zh)
var htmlE=document.documentElement;
if (wgUserLanguage == "ku"){ variant = "ku"; }
if (wgUserLanguage == "ku-latn"){ variant = "ku-Latn"; }
if (wgUserLanguage == "ku-arab"){ variant = "ku-Arab"; htmlE.setAttribute("dir","rtl"); }
htmlE.setAttribute("lang",variant);
htmlE.setAttribute("xml:lang",variant);

// Switch language variants of messages (from zh)
function wgULS(latn,arab){
        //
        ku=latn||arab;
        ku=ku;
        latn=latn;
        arab=arab;
        switch(wgUserLanguage){
                case "ku": return ku;
                case "ku-arab": return arab;
                case "ku-latn": return latn;
                default: return "";
        }
}

// workaround for RTL ([[bugzilla:6756]])  and for [[bugzilla:02020]] & [[bugzilla:04295]]
if (wgUserLanguage == "ku-arab")
{
  document.direction="rtl";
  document.write(\'<link rel="stylesheet" type="text/css" href="\'+stylepath+\'/common/common_rtl.css">\');
  document.write(\'<style type="text/css">html {direction:rtl;} body {direction:rtl; unicode-bidi:embed; lang:ku-Arab; font-family:"Arial Unicode MS",Arial,Tahoma; font-size: 75%; letter-spacing: 0.001em;} html > body div#content ol {clear: left;} ol {margin-left:2.4em; margin-right:2.4em;} ul {margin-left:1.5em; margin-right:1.5em;} h1.firstHeading {background-position: bottom right; background-repeat: no-repeat;} h3 {font-size:110%;} h4 {font-size:100%;} h5 {font-size:90%;} #catlinks {width:100%;} #userloginForm {float: right !important;}</style>\');

  if (skin == "monobook"){
     document.write(\'<link rel="stylesheet" type="text/css" href="\'+stylepath+\'/monobook/rtl.css">\');
  }
}',

# Attribution
'anonymous' => 'Bikarhênera/ê nediyarkirî ya/yê {{SITENAME}}',
'siteuser'  => 'Bikarhênera/ê $1 a/ê {{SITENAME}}',
'others'    => 'ên din',
'siteusers' => 'Bikarhênerên $1 yên {{SITENAME}}',

# Spam protection
'spamprotectiontitle' => 'Parastina spam',
'spamprotectiontext'  => 'Ew rûpela yê tu dixast tomarbikê hate astengkirin ji ber ku parastina spam. Ew çêbû ji ber ku lînkekî derva di vê rûpelê da ye.',
'spamprotectionmatch' => 'Ev nivîsa parastinê spam vêxist: $1',

# Info page
'numedits'     => 'Hejmara guherandinan (rûpel): $1',
'numtalkedits' => 'Hejmara guherandinan (guftûgo): $1',
'numwatchers'  => 'Hejmara kesên dişopînin: $1',

# Math options
'mw_math_png'    => 'Her caran wek PNG nîşanbide',
'mw_math_simple' => 'HTML eger asan be, wekî din PNG',
'mw_math_html'   => 'HTML eger bibe, wekî din PNG',
'mw_math_source' => "Wek TeX bêle (ji browser'ên gotaran ra)",
'mw_math_modern' => "Baştir e ji browser'ên nuhtir",
'mw_math_mathml' => 'MathML eger bibe (ceribandin)',

# Patrolling
'markaspatrolleddiff'   => 'Wek serrastkirî nîşanbide',
'markaspatrolledtext'   => 'Vê rûpelê wek serrastkirî nîşanbide',
'markedaspatrolled'     => 'Wek serrastkirî tê nîşandan',
'markedaspatrolledtext' => 'Guherandina rûpelê wek serrastkirî tê nîşandan.',

# Patrol log
'patrol-log-page' => 'Reşahîya kontrolkirinê',
'patrol-log-line' => '$1 ji $2 hate kontrolkirin $3',
'patrol-log-auto' => '(otomatîk)',

# Image deletion
'deletedrevision'                 => 'Rêvîsîyona berê $1 hate jêbirin.',
'filedelete-missing'              => 'Data\'yê "$1" nikane were jêbirin, ji ber ku ew tune.',
'filedelete-current-unregistered' => 'Datayê "$1" di sistêmê da tune.',

# Browsing diffs
'previousdiff' => '← Ciyawaziya pêştir',
'nextdiff'     => 'Ciyawaziya paştir →',

# Media information
'thumbsize'            => "Mezinbûna thunbnail'ê:",
'widthheight'          => '$1 x $2',
'widthheightpage'      => '$1×$2, $3 rûpel',
'file-info'            => '(mezinbûnê data: $1, MIME-typ: $2)',
'file-info-size'       => '($1 × $2 pixel, mezinbûnê data: $3, MIME-typ: $4)',
'file-nohires'         => '<small>Versyonekî jê mezintir tune.</small>',
'svg-long-desc'        => "(Data'ya SVG, mezinbûna rast: $1 × $2 pixel; mezinbûna data'yê: $3)",
'show-big-image'       => 'Mezînbûn',
'show-big-image-thumb' => '<small>Mezinbûna vê pêşnîşandanê: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'             => 'Pêşangeha wêneyên nû',
'imagelisttext'         => "Jêr lîsteyek ji $1 file'an heye, duxrekirin $2.",
'showhidebots'          => "($1 bot'an)",
'noimages'              => 'Ne tiştek tê dîtin.',
'ilsubmit'              => 'Lêbigere',
'bydate'                => 'li gor dîrokê',
'sp-newimages-showfrom' => "Data'yên nuh ji dema $1, saet $2 da bibîne",

# Variants for Kurdish language
'variantname-ku-arab' => 'tîpên erebî',
'variantname-ku-latn' => 'tîpên latînî',
'variantname-ku'      => 'disable',

# EXIF tags
'exif-imagewidth'                  => 'Panbûn',
'exif-imagelength'                 => 'Dirêjbûn',
'exif-jpeginterchangeformatlength' => "Byte'ên data'ya JPEG",
'exif-imagedescription'            => 'Navî wêneyê',
'exif-artist'                      => 'Nûser',
'exif-exposuretime-format'         => '$1 sanî ($2)',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'hemû',
'imagelistall'     => 'hemû',
'watchlistall2'    => 'hemû',
'namespacesall'    => 'Hemû',
'monthsall'        => 'giştik',

# E-mail address confirmation
'confirmemail'          => 'Adrêsa e-nameyan nasbike',
'confirmemail_noemail'  => 'Te e-mail-adressê xwe di [[Special:Preferences|tercihên xwe da]] nenivîsandîye.',
'confirmemail_success'  => 'E-Mail adrêsa te hate naskirin. Tu niha dikarî xwe qeydbikê û kêfkê.',
'confirmemail_loggedin' => 'Adrêsa te yê E-Mail hate qebûlkirin.',
'confirmemail_body'     => 'Kesek, dibê tu, bi IP adressê $1, xwe li {{SITENAME}} bi vê navnîşana e-name tomar kir ("$2") .

Eger ev rast qeydkirinê te ye û di dixwazî bikaranîna e-nama ji te ra çêbibe li {{SITENAME}}, li vê lînkê bitikîne:

$3

Lê eger ev *ne* tu bû, li lînkê netikîne. Ev e-nameya di rojê $4 da netê qebûlkirin.',

# Scary transclusion
'scarytranscludefailed'  => '[Anîna şablona $1 biserneket; biborîne]',
'scarytranscludetoolong' => '[URL zêde dirêj e; bibore]',

# Trackbacks
'trackbackremove' => '([$1 jêbibe])',

# Delete conflict
'deletedwhileediting' => 'Hîşyar: Piştî te guherandinê xwe dest pê kir ev rûpela hate jêbirin!',
'confirmrecreate'     => "Bikarhêner [[User:$1|$1]] ([[User talk:$1|nîqaş]]) vê rûpelê jêbir, piştî te destpêkir bi guherandinê. Sedemê jêbirinê ev bû:
: ''$2''
Xêra xwe zanibe ku tu bi rastî dixwazê vê rûpelê dîsa çêkê",
'recreate'            => 'Dîsa tomarbike',

# HTML dump
'redirectingto' => 'Redirect berve [[:$1]] tê çêkirin...',

# action=purge
'confirm_purge'        => 'Bîra vê rûpelê jêbîbe ?

$1',
'confirm_purge_button' => 'Temam',

# Multipage image navigation
'imgmultipageprev' => '← rûpela berî vê',
'imgmultipagenext' => 'rûpela din →',
'imgmultigo'       => 'Dê!',

# Table pager
'table_pager_next'         => 'Rûpelê din',
'table_pager_prev'         => 'Rûpelê berî',
'table_pager_first'        => 'Rûpelê yekemîn',
'table_pager_last'         => 'Rûpelê dawî',
'table_pager_limit_submit' => 'Dê',

# Auto-summaries
'autosumm-blank'   => 'Rûpel hate vala kirin',
'autosumm-replace' => "'$1' ket şûna rûpelê.",
'autoredircomment' => 'Redirect berve [[$1]]',
'autosumm-new'     => 'Rûpela nû: $1',

# Live preview
'livepreview-loading' => 'Tê…',
'livepreview-ready'   => 'Tê… Çêbû!',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Xeyrandin yê piştî $1 sanîyan hatine çêkirin belkî netên wêşendan.',
'lag-warn-high'   => 'Ji bo westinê sistêmê ew xeyrandin, yê piştî $1 sanîyan hatine çêkirin netên wêşendan.',

# Watchlist editor
'watchlistedit-numitems'      => 'Di lîsteya te ya şopandinê da {{PLURAL:$1|gotarek heye|$1 gotar hene}} (vê rûpelên guftûgoyan).',
'watchlistedit-noitems'       => 'Di lîsteya te ya şopandinê gotar tune ne.',
'watchlistedit-normal-title'  => 'Lîsteya xwe ya şopandinê biguherîne',
'watchlistedit-normal-legend' => 'Gotaran ji lîsteya min ya şopandinê rake',
'watchlistedit-normal-submit' => 'Gotaran jêbibe',
'watchlistedit-normal-done'   => '{{PLURAL:$1|1 gotar hate|$1 gotaran hatin}} jêbirin ji lîsteya te yê şopandinê:',
'watchlistedit-raw-titles'    => 'Gotar:',
'watchlistedit-raw-removed'   => '{{PLURAL:$1|1 gotar hate|$1 gotar hatin}} jêbirin:',

# Watchlist editing tools
'watchlisttools-edit' => 'Lîsteya şopandinê bibîne û biguherîne',

# Special:Version
'version'       => 'Verzîyon', # Not used as normal message but as header for the special page itself
'version-other' => 'yên din',

# Special:FilePath
'filepath-page' => 'Data:',

# Special:FileDuplicateSearch
'fileduplicatesearch-submit' => 'Lêbigere',

# Special:SpecialPages
'specialpages'               => 'Rûpelên taybet',
'specialpages-note'          => '----
* Rûpelên taybetî ji her kesan ra
* <span class="mw-specialpagerestricted">Rûpelên taybetî ji bikarhêneran bi mafên zêdetir ra</span>',
'specialpages-group-other'   => 'Rûpelên taybetî yên din',
'specialpages-group-login'   => 'Têkevê',
'specialpages-group-changes' => 'Guherandinên dawî û reşahîyan',
'specialpages-group-media'   => 'Nameyên medyayan û barkirinan',
'specialpages-group-users'   => 'Bikarhêner û maf',

);
