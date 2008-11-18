<?php
/** Welsh (Cymraeg)
 *
 * @ingroup Language
 * @file
 *
 * @author Lloffiwr
 * @author Thaf
 * @author לערי ריינהארט
 */

$namespaceNames = array(
	NS_MEDIA          => "Media",
	NS_SPECIAL        => "Arbennig",
	NS_MAIN           => "",
	NS_TALK           => "Sgwrs",
	NS_USER           => "Defnyddiwr",
	NS_USER_TALK      => "Sgwrs_Defnyddiwr",
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK   => "Sgwrs_$1",
	NS_IMAGE          => "Delwedd",
	NS_IMAGE_TALK     => "Sgwrs_Delwedd",
	NS_MEDIAWIKI      => "MediaWici",
	NS_MEDIAWIKI_TALK => "Sgwrs_MediaWici",
	NS_TEMPLATE       => "Nodyn",
	NS_TEMPLATE_TALK  => "Sgwrs_Nodyn",
	NS_CATEGORY		  => "Categori",
	NS_CATEGORY_TALK  => "Sgwrs_Categori",
	NS_HELP			  => "Cymorth",
	NS_HELP_TALK	  => "Sgwrs Cymorth"
);

$skinNames = array(
	'standard'    => 'Safonol',
	'nostalgia'   => 'Hiraeth',
	'cologneblue' => 'Glas Cwlen',
);

$defaultDateFormat = 'dmy';

$bookstoreList = array(
	"AddALL" => "http://www.addall.com/New/Partner.cgi?query=$1&type=ISBN",
	"PriceSCAN" => "http://www.pricescan.com/books/bookDetail.asp?isbn=$1",
	"Barnes & Noble" => "http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1",
	"Amazon.com" => "http://www.amazon.com/exec/obidos/ISBN=$1",
	"Amazon.co.uk" => "http://www.amazon.co.uk/exec/obidos/ISBN=$1"
);

$magicWords = array(
	'redirect'            => array( '0', '#redirect', '#ail-cyfeirio', '#ailgyfeirio' ),
	'notoc'               => array( '0', '__NOTOC__', '__DIMTAFLENCYNNWYS__', '__DIMRHESTRGYNNWYS__', '__DIMRHG__' ),
	'noeditsection'       => array( '0', '__NOEDITSECTION__', '__DIMADRANGOLYGU__', '__DIMGOLYGUADRAN__' ),
	'currentmonth'        => array( '1', 'CURRENTMONTH', 'MISCYFOES', 'MISCYFREDOL' ),
	'currentmonthname'    => array( '1', 'CURRENTMONTHNAME', 'ENWMISCYFOES', 'ENWMISCYFREDOL' ),
	'currentmonthnamegen' => array( '1', 'CURRENTMONTHNAMEGEN', 'GENENWMISCYFOES' ),
	'currentday'          => array( '1', 'CURRENTDAY', 'DYDDIADCYFOES', 'DYDDCYFREDOL' ),
	'currentdayname'      => array( '1', 'CURRENTDAYNAME', 'ENWDYDDCYFOES', 'ENWDYDDCYFREDOL' ),
	'currentyear'         => array( '1', 'CURRENTYEAR', 'FLWYDDYNCYFOES', 'BLWYDDYNGYFREDOL' ),
	'currenttime'         => array( '1', 'CURRENTTIME', 'AMSERCYFOES', 'AMSERCYFREDOL' ),
	'currenthour'         => array( '1', 'CURRENTHOUR', 'AWRGYFREDOL' ),
	'numberofarticles'    => array( '1', 'NUMBEROFARTICLES', 'NIFEROERTHYGLAU', 'NIFERYRERTHYGLAU' ),
	'numberoffiles'       => array( '1', 'NUMBEROFFILES', 'NIFERYFFEILIAU' ),
	'numberofusers'       => array( '1', 'NUMBEROFUSERS', 'NIFERYDEFNYDDWYR' ),
	'numberofedits'       => array( '1', 'NUMBEROFEDITS', 'NIFERYGOLYGIADAU' ),
	'pagename'            => array( '1', 'PAGENAME', 'ENWTUDALEN' ),
	'pagenamee'           => array( '1', 'PAGENAMEE', 'ENWTUDALENE' ),
	'namespace'           => array( '1', 'NAMESPACE', 'PARTH' ),
	'namespacee'          => array( '1', 'NAMESPACE', 'PARTHE' ),
	'fullpagename'        => array( '1', 'FULLPAGENAME', 'ENWLLAWNTUDALEN' ),
	'fullpagenamee'       => array( '1', 'FULLPAGENAMEE', 'ENWLLAWNTUDALENE' ),
	'subpagename'         => array( '1', 'SUBPAGENAME', 'ENWISDUDALEN' ),
	'subpagenamee'        => array( '1', 'SUBPAGENAMEE', 'ENWISDUDALENE' ),
	'talkpagename'        => array( '1', 'TALKPAGENAME', 'ENWTUDALENSGWRS' ),
	'talkpagenamee'       => array( '1', 'TALKPAGENAMEE', 'ENWTUDALENSGWRSE' ),
	'img_thumbnail'       => array( '1', 'ewin bawd', 'bawd', 'thumb', 'thumbnail', 'mân-lun' ),
	'img_manualthumb'     => array( '1', 'thumbnail=$1', 'thumb=$1', 'mân-lun=$1', 'bawd=$1' ),
	'img_right'           => array( '1', 'de', 'right' ),
	'img_left'            => array( '1', 'chwith', 'left' ),
	'img_none'            => array( '1', 'dim', 'none' ),
	'img_center'          => array( '1', 'canol', 'centre', 'center' ),
	'img_page'            => array( '1', 'page=$1', 'page $1', 'tudalen=$1', 'tudalen $1' ),
	'img_upright'         => array( '1', 'upright', 'upright=$1', 'upright $1', 'unionsyth', 'unionsyth=$1', 'unionsyth $1' ),
	'img_sub'             => array( '1', 'sub', 'is' ),
	'img_super'           => array( '1', 'super', 'sup', 'uwch' ),
	'img_top'             => array( '1', 'top', 'brig' ),
	'img_middle'          => array( '1', 'middle', 'canol' ),
	'img_bottom'          => array( '1', 'bottom', 'gwaelod', 'godre' ),
	'server'              => array( '0', 'SERVER', 'GWEINYDD' ),
	'servername'          => array( '0', 'SERVERNAME', 'ENW\'RGWEINYDD' ),
	'grammar'             => array( '0', 'GRAMMAR', 'GRAMADEG' ),
	'currentweek'         => array( '1', 'CURRENTWEEK', 'WYTHNOSGYFREDOL' ),
	'revisionid'          => array( '1', 'REVISIONID', 'IDYGOLYGIAD' ),
	'revisionday'         => array( '1', 'REVISIONDAY', 'DIWRNODYGOLYGIAD' ),
	'revisionday2'        => array( '1', 'REVISIONDAY2', 'DIWRNODYGOLYGIAD2' ),
	'revisionmonth'       => array( '1', 'REVISIONMONTH', 'MISYGOLYGIAD' ),
	'revisionyear'        => array( '1', 'REVISIONYEAR', 'BLWYDDYNYGOLYGIAD' ),
	'revisiontimestamp'   => array( '1', 'REVISIONTIMESTAMP', 'STAMPAMSERYGOLYGIAD' ),
	'plural'              => array( '0', 'PLURAL:', 'LLUOSOG:' ),
	'fullurl'             => array( '0', 'FULLURL:', 'URLLLAWN:' ),
	'fullurle'            => array( '0', 'FULLURLE:', 'URLLLAWNE:' ),
	'newsectionlink'      => array( '1', '_NEWSECTIONLINK_', '_CYSWLLTADRANNEWYDD_' ),
	'currentversion'      => array( '1', 'CURRENTVERSION', 'GOLYGIADCYFREDOL' ),
	'currenttimestamp'    => array( '1', 'CURRENTTIMESTAMP', 'STAMPAMSERCYFREDOL' ),
	'localtimestamp'      => array( '1', 'LOCALTIMESTAMP', 'STAMPAMSERLLEOL' ),
	'language'            => array( '0', '#LANGUAGE:', '#IAITH:' ),
	'contentlanguage'     => array( '1', 'CONTENTLANGUAGE', 'CONTENTLANG', 'IAITHYCYNNWYS' ),
	'pagesinnamespace'    => array( '1', 'PAGESINNAMESPACE:', 'PAGESINNS:', 'TUDALENNAUYNYPARTH:' ),
	'numberofadmins'      => array( '1', 'NUMBEROFADMINS', 'NIFERYGWEINYDDWYR' ),
	'formatnum'           => array( '0', 'FORMATNUM', 'FFORMATIORHIF' ),
	'special'             => array( '0', 'special', 'arbennig' ),
	'hiddencat'           => array( '1', '_HIDDENCAT_', '_CATCUDD_' ),
	'pagesincategory'     => array( '1', 'PAGESINCATEGORY', 'PAGESINCAT', 'TUDALENNAUYNYCAT' ),
	'pagesize'            => array( '1', 'PAGESIZE', 'MAINTTUD' ),
);

$linkTrail = "/^([àáâèéêìíîïòóôûŵŷa-z]+)(.*)\$/sDu";

$messages = array(
# User preference toggles
'tog-underline'               => 'Tanlinellu cysylltiadau:',
'tog-highlightbroken'         => 'Fformatio cysylltiadau wedi\'u torri <a href="" class="new">fel hyn</a> (dewis arall: fel hyn<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Unioni paragraffau',
'tog-hideminor'               => 'Cuddiwch golygiadau bach mewn newidiadau diweddar',
'tog-extendwatchlist'         => 'Ehangu manylion y rhestr gwylio i ddangos pob golygiad i dudalen, nid dim ond y diweddaraf',
'tog-usenewrc'                => "Fersiwn well o 'Newidiadau diweddar' (JavaScript)",
'tog-numberheadings'          => "Rhifwch benawdau'n awtomatig",
'tog-showtoolbar'             => 'Dangos y bar offer golygu (JavaScript)',
'tog-editondblclick'          => 'Golygu tudalennau gyda chlic dwbwl (JavaScript)',
'tog-editsection'             => 'Galluogi golygu adran drwy gyswllt [golygu] wrth ymyl pennawd yr adran',
'tog-editsectiononrightclick' => 'Galluogi golygu adran drwy dde-glicio ar bennawd yr adran (JavaScript)',
'tog-showtoc'                 => 'Dangos y daflen gynnwys (ar gyfer tudalennau sydd â mwy na 3 pennawd)',
'tog-rememberpassword'        => "Y cyfrifiadur hwn i gofio'r cyfrinair",
'tog-editwidth'               => "Gosod lled llawn i'r blwch golygu",
'tog-watchcreations'          => 'Ychwanegu tudalennau at fy rhestr gwylio wrth i mi eu creu',
'tog-watchdefault'            => 'Ychwanegu tudalen at fy rhestr gwylio wrth i mi ei golygu',
'tog-watchmoves'              => 'Ychwanegu tudalen at fy rhestr gwylio wrth i mi ei symud.',
'tog-watchdeletion'           => 'Ychwanegu tudalennau at fy rhestr gwylio wrth i mi eu dileu',
'tog-minordefault'            => 'Marciwch pob golygiad fel un bach',
'tog-previewontop'            => 'Dangos y rhagolwg uwchben yn hytrach nag o dan y bocs golygu.',
'tog-previewonfirst'          => 'Dangos rhagolwg ar y golygiad cyntaf',
'tog-nocache'                 => 'Analluogi storio tudalennau mewn celc',
'tog-enotifwatchlistpages'    => 'Gyrru e-bost ataf fy hunan pan fo newid i dudalen ar fy rhestr gwylio',
'tog-enotifusertalkpages'     => "Gyrru e-bost ataf fy hunan pan fo newid i'm tudalen sgwrs",
'tog-enotifminoredits'        => 'Gyrru e-bost ataf fy hunan ar gyfer golygiadau bychain i dudalennau, hefyd',
'tog-enotifrevealaddr'        => 'Datguddio fy nghyfeiriad e-bost mewn e-byst hysbysu',
'tog-shownumberswatching'     => "Dangos y nifer o ddefnyddwyr sy'n gwylio",
'tog-fancysig'                => 'Llofnod crai (heb gyswllt wici ynghlwm wrtho)',
'tog-externaleditor'          => 'Defnyddio golygydd allanol trwy ragosodiad (ar gyfer arbenigwyr yn unig; mae angen gosodiadau arbennig ar eich cyfrifiadur)',
'tog-externaldiff'            => 'Defnyddio "external diff" trwy ragosodiad (ar gyfer arbenigwyr yn unig; mae angen gosodiadau arbennig ar eich cyfrifiadur)',
'tog-showjumplinks'           => 'Galluogi cysylltiadau hygyrchedd, e.e. [alt-z]',
'tog-uselivepreview'          => 'Defnyddio rhagolwg byw (JavaScript) (Arbrofol)',
'tog-forceeditsummary'        => 'Tynnu fy sylw pan adawaf flwch crynodeb golygu yn wag',
'tog-watchlisthideown'        => 'Cuddio fy ngolygiadau fy hunan yn fy rhestr gwylio',
'tog-watchlisthidebots'       => 'Cuddio golygiadau bot yn fy rhestr gwylio',
'tog-watchlisthideminor'      => 'Cuddio golygiadau bychain rhag y rhestr gwylio',
'tog-ccmeonemails'            => 'Anfoner copi ataf pan anfonaf e-bost at ddefnyddiwr arall',
'tog-diffonly'                => "Peidio â dangos cynnwys y dudalen islaw'r gymhariaeth ar dudalennau cymharu",
'tog-showhiddencats'          => 'Dangos categorïau cuddiedig',

'underline-always'  => 'Bob amser',
'underline-never'   => 'Byth',
'underline-default' => 'Rhagosodyn y porwr',

'skinpreview' => '(Rhagolwg)',

# Dates
'sunday'        => 'Dydd Sul',
'monday'        => 'Dydd Llun',
'tuesday'       => 'Dydd Mawrth',
'wednesday'     => 'Dydd Mercher',
'thursday'      => 'Dydd Iau',
'friday'        => 'Dydd Gwener',
'saturday'      => 'Dydd Sadwrn',
'sun'           => 'Sul',
'mon'           => 'Llun',
'tue'           => 'Maw',
'wed'           => 'Mer',
'thu'           => 'Iau',
'fri'           => 'Gwe',
'sat'           => 'Sad',
'january'       => 'Ionawr',
'february'      => 'Chwefror',
'march'         => 'Mawrth',
'april'         => 'Ebrill',
'may_long'      => 'Mai',
'june'          => 'Mehefin',
'july'          => 'Gorffennaf',
'august'        => 'Awst',
'september'     => 'Medi',
'october'       => 'Hydref',
'november'      => 'Tachwedd',
'december'      => 'Rhagfyr',
'january-gen'   => 'Ionawr',
'february-gen'  => 'Chwefror',
'march-gen'     => 'Mawrth',
'april-gen'     => 'Ebrill',
'may-gen'       => 'Mai',
'june-gen'      => 'Mehefin',
'july-gen'      => 'Gorffennaf',
'august-gen'    => 'Awst',
'september-gen' => 'Medi',
'october-gen'   => 'Hydref',
'november-gen'  => 'Tachwedd',
'december-gen'  => 'Rhagfyr',
'jan'           => 'Ion',
'feb'           => 'Chwe',
'mar'           => 'Maw',
'apr'           => 'Ebr',
'may'           => 'Mai',
'jun'           => 'Meh',
'jul'           => 'Gor',
'aug'           => 'Awst',
'sep'           => 'Medi',
'oct'           => 'Hyd',
'nov'           => 'Tach',
'dec'           => 'Rhag',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Categori|Categori|Categorïau|Categorïau|Categorïau|Categorïau}}',
'category_header'                => 'Erthyglau yn y categori "$1"',
'subcategories'                  => 'Is-gategorïau',
'category-media-header'          => "Cyfryngau yn y categori '$1'",
'category-empty'                 => "''Ar hyn o bryd nid oes unrhyw erthyglau na ffeiliau yn y categori hwn.''",
'hidden-categories'              => '{{PLURAL:$1|Categori cuddiedig|Categori cuddiedig|Categorïau cuddiedig|Categorïau cuddiedig|Categorïau cuddiedig|Categorïau cuddiedig}}',
'hidden-category-category'       => 'Categorïau cuddiedig', # Name of the category where hidden categories will be listed
'category-subcat-count'          => "{{PLURAL:$1|Nid oes dim is-gategorïau|Dim ond yr is-gategori sy'n dilyn sydd|Mae'r $1 is-gategori sy'n dilyn ymhlith cyfanswm o $2|Mae'r $1 is-gategori sy'n dilyn ymhlith cyfanswm o $2|Mae'r $1 is-gategori sy'n dilyn ymhlith cyfanswm o $2|Mae'r $1 is-gategori sy'n dilyn ymhlith cyfanswm o $2}} yn y categori hwn.",
'category-subcat-count-limited'  => 'Mae gan y categori hwn $1 {{PLURAL:$1|is-gategori|is-gategori|is-gategori|is-gategori|is-gategori|is-gategori|}}.',
'category-article-count'         => "{{PLURAL:$2|Nid oes dim tudalennau|Dim ond y dudalen sy'n dilyn sydd|Dangosir isod y $1 dudalen sydd|Dangosir isod y $1 tudalen sydd|Dangosir isod y $1 thudalen sydd|Dangosir isod $1 {{PLURAL:$1|Dim|dudalen|dudalen|tudalen|thudalen|tudalen}} ymhlith cyfanswm o $2 sydd}} yn y categori hwn.",
'category-article-count-limited' => "Mae'r {{PLURAL:$1|tudalen|dudalen|$1 dudalen|$1 tudalen|$1 thudalen|$1 tudalen}} sy'n dilyn yn y categori hwn.",
'category-file-count'            => "{{PLURAL:$2|Nid oes dim ffeiliau|Dim ond y ffeil sy'n dilyn sydd|Mae'r $1 ffeil sy'n dilyn ymlith cyfanswm o $2|Mae'r $1 ffeil sy'n dilyn ymlith cyfanswm o $2|Mae'r $1 ffeil sy'n dilyn ymlith cyfanswm o $2|Mae'r $1 ffeil sy'n dilyn ymlith cyfanswm o $2}} yn y categori hwn.",
'category-file-count-limited'    => "Mae'r {{PLURAL:$1|dim ffeil|un ffeil|$1 ffeil|$1 ffeil|$1 ffeil|$1 ffeil}} canlynol yn y categori hwn.",
'listingcontinuesabbrev'         => ' parh.',

'mainpagetext'      => "<big>'''Wedi llwyddo gosod meddalwedd Mediawiki yma'''</big>",
'mainpagedocfooter' => 'Ceir cymorth (yn Saesneg) ar ddefnyddio meddalwedd wici yn y [http://meta.wikimedia.org/wiki/Help:Contents Canllaw Defnyddwyr] ar wefan Wikimedia.

==Cychwyn arni==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Rhestr gosodiadau wrth gyflunio]
* [http://www.mediawiki.org/wiki/Manual:FAQ Cwestiynau poblogaidd ar MediaWiki]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Rhestr postio datganiadau MediaWiki]',

'about'          => 'Ynglŷn â',
'article'        => 'Erthygl',
'newwindow'      => '(yn agor mewn ffenest newydd)',
'cancel'         => 'Diddymu',
'qbfind'         => 'Canfod',
'qbbrowse'       => 'Pori',
'qbedit'         => 'Golygu',
'qbpageoptions'  => 'Y dudalen hon',
'qbpageinfo'     => 'Cyd-destun',
'qbmyoptions'    => 'Fy nhudalennau',
'qbspecialpages' => 'Tudalennau arbennig',
'moredotdotdot'  => 'Rhagor...',
'mypage'         => 'Fy nhudalen',
'mytalk'         => 'Fy sgwrs',
'anontalk'       => 'Sgwrs ar gyfer y cyfeiriad IP hwn',
'navigation'     => 'Panel llywio',
'and'            => 'a/ac',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Gwall',
'returnto'          => 'Dychwelyd at $1.',
'tagline'           => 'Oddi ar {{SITENAME}}',
'help'              => 'Cymorth',
'search'            => 'Chwilio',
'searchbutton'      => 'Chwilio',
'go'                => 'Eler',
'searcharticle'     => 'Mynd',
'history'           => 'Hanes y dudalen',
'history_short'     => 'Hanes',
'updatedmarker'     => 'diwygiwyd ers i mi ymweld ddiwethaf',
'info_short'        => 'Gwybodaeth',
'printableversion'  => 'Fersiwn argraffu',
'permalink'         => 'Dolen barhaol',
'print'             => 'Argraffu',
'edit'              => 'Golygu',
'create'            => 'Creu',
'editthispage'      => 'Golygwch y dudalen hon',
'create-this-page'  => "Creu'r dudalen",
'delete'            => 'Dileu',
'deletethispage'    => 'Dilëer y dudalen hon',
'undelete_short'    => 'Adfer $1 {{PLURAL:$1|golygiad|golygiad|olygiad|golygiad|golygiad|golygiad}}',
'protect'           => 'Diogelu',
'protect_change'    => 'newid',
'protectthispage'   => "Diogelu'r dudalen hon",
'unprotect'         => 'Dad-ddiogelu',
'unprotectthispage' => "Dad-ddiogelu'r dudalen hon",
'newpage'           => 'Tudalen newydd',
'talkpage'          => 'Sgwrsiwch am y dudalen hon',
'talkpagelinktext'  => 'Sgwrs',
'specialpage'       => 'Tudalen Arbennig',
'personaltools'     => 'Offer personol',
'postcomment'       => 'Postiwch sylw',
'articlepage'       => 'Dangos tudalen yn y prif barth',
'talk'              => 'Sgwrs',
'views'             => 'Golygon',
'toolbox'           => 'Blwch offer',
'userpage'          => 'Gwyliwch dudalen y defnyddiwr',
'projectpage'       => 'Gweld tudalen y wici',
'imagepage'         => 'Gweld tudalen y ffeil clyweled',
'mediawikipage'     => 'Gweld tudalen y neges',
'templatepage'      => 'Dangos y dudalen templed',
'viewhelppage'      => 'Dangos y dudalen gymorth',
'categorypage'      => 'Dangos tudalen gategori',
'viewtalkpage'      => 'Gweld y sgwrs',
'otherlanguages'    => 'Ieithoedd eraill',
'redirectedfrom'    => '(Ailgyfeiriad oddi wrth $1)',
'redirectpagesub'   => 'Tudalen ailgyfeirio',
'lastmodifiedat'    => 'Newidiwyd y dudalen hon ddiwethaf $2, $1.', # $1 date, $2 time
'viewcount'         => "{{PLURAL:$1|Ni chafwyd dim|Cafwyd $1|Cafwyd $1|Cafwyd $1|Cafwyd $1|Cafwyd $1}} ymweliad â'r dudalen hon.",
'protectedpage'     => 'Tudalen a ddiogelwyd',
'jumpto'            => 'Neidio i:',
'jumptonavigation'  => 'llywio',
'jumptosearch'      => 'chwilio',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Ynglŷn â {{SITENAME}}',
'aboutpage'            => 'Project:Ynglŷn â {{SITENAME}}',
'bugreports'           => 'Adroddiadau diffygion',
'bugreportspage'       => 'Project:Adroddiadau diffygion',
'copyright'            => "Mae'r cynnwys ar gael o dan $1.",
'copyrightpagename'    => 'Hawlfraint {{SITENAME}}',
'copyrightpage'        => '{{ns:project}}:Hawlfraint',
'currentevents'        => 'Materion cyfoes',
'currentevents-url'    => 'Project:Materion cyfoes',
'disclaimers'          => 'Gwadiadau',
'disclaimerpage'       => 'Project:Gwadiad Cyffredinol',
'edithelp'             => 'Help gyda golygu',
'edithelppage'         => 'Help:Golygu',
'faq'                  => 'Cwestiynau cyffredin',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:Cymorth',
'mainpage'             => 'Hafan',
'mainpage-description' => 'Hafan',
'policy-url'           => 'Project:Policy',
'portal'               => 'Porth y Gymuned',
'portal-url'           => 'Project:Porth y Gymuned',
'privacy'              => 'Polisi preifatrwydd',
'privacypage'          => 'Project:Polisi preifatrwydd',

'badaccess'        => 'Gwall caniatâd',
'badaccess-group0' => 'Ni chaniateir i chi wneud y weithred y ceisiasoch amdani.',
'badaccess-group1' => "Dim ond defnyddwyr yng ngrŵp $1 sy'n cael gwneud y weithred y ceisiasoch amdani.",
'badaccess-group2' => "Dim ond defnyddwyr o blith y grwpiau $1 sy'n cael gwneud y weithred y ceisiasoch amdani.",
'badaccess-groups' => "Dim ond defnyddwyr o blith y grwpiau $1 sy'n cael gwneud y weithred y ceisiasoch amdani.",

'versionrequired'     => 'Mae angen fersiwn $1 y meddalwedd MediaWiki',
'versionrequiredtext' => "Mae angen fersiwn $1 y meddalwedd Mediawiki er mwyn gwneud defnydd o'r dudalen hon. Gweler y dudalen am y [[Special:Version|fersiwn]].",

'ok'                      => 'Iawn',
'retrievedfrom'           => 'Wedi dod o "$1"',
'youhavenewmessages'      => 'Mae gennych chi $1 ($2).',
'newmessageslink'         => 'Neges(eueon) newydd',
'newmessagesdifflink'     => 'y newid diweddaraf',
'youhavenewmessagesmulti' => 'Mae negeseuon newydd gennych ar $1',
'editsection'             => 'golygu',
'editold'                 => 'golygu',
'viewsourceold'           => 'dangos y tarddiad',
'editsectionhint'         => "Golygu'r adran: $1",
'toc'                     => 'Taflen Cynnwys',
'showtoc'                 => 'dangos',
'hidetoc'                 => 'cuddio',
'thisisdeleted'           => 'Ydych chi am ddangos, neu ddad-ddileu $1?',
'viewdeleted'             => 'Gweld $1?',
'restorelink'             => "$1 {{PLURAL:$1|golygiad sydd wedi'i ddileu|golygiad sydd wedi'i ddileu|olygiad sydd wedi'u dileu|golygiad sydd wedi'u dileu|golygiad sydd wedi'u dileu|golygiad sydd wedi'u dileu}}",
'feedlinks'               => 'Porthiant:',
'feed-invalid'            => 'Math annilys o borthiant ar danysgrifiad.',
'site-rss-feed'           => 'Porthiant RSS $1',
'site-atom-feed'          => 'Porthiant Atom $1',
'page-rss-feed'           => "Porthiant RSS '$1'",
'page-atom-feed'          => "Porthiant Atom '$1'",
'red-link-title'          => '$1 (heb ei greu eto)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Tudalen',
'nstab-user'      => 'Tudalen defnyddiwr',
'nstab-media'     => 'Tudalen cyfrwng',
'nstab-special'   => 'Arbennig',
'nstab-project'   => 'Tudalen y wici',
'nstab-image'     => 'Ffeil',
'nstab-mediawiki' => 'Neges',
'nstab-template'  => 'Nodyn',
'nstab-help'      => 'Cymorth',
'nstab-category'  => 'Categori',

# Main script and global functions
'nosuchaction'      => "Dim gweithred o'r fath",
'nosuchactiontext'  => "Dydi'r wici ddim yn adnabod y weithred yn y cyfeiriad URL.",
'nosuchspecialpage' => 'Y dudalen arbennig ddim yn bod',
'nospecialpagetext' => "<big>'''Dyw'r wici ddim yn adnabod y dudalen arbennig y gofynnwyd amdani.'''</big>

Mae rhestr o'r tudalennau arbennig dilys i'w gael [[Special:SpecialPages|yma]].",

# General errors
'error'                => 'Gwall',
'databaseerror'        => 'Gwall databas',
'dberrortext'          => 'Mae gwall cystrawen wedi taro\'r databas.
Efallai fod gwall yn y meddalwedd.
Y gofyniad olaf y trïodd y databas oedd:
<blockquote><tt>$1</tt></blockquote>
o\'r ffwythiant "<tt>$2</tt>".
Rhoddwyd y côd gwall "<tt>$3: $4</tt>" gan MySQL.',
'dberrortextcl'        => 'Mae gwall cystrawen wedi taro\'r databas.
Y gofyniad olaf y trïodd y databas oedd:
"$1"
o\'r ffwythiant "$2".
Rhoddwyd y côd gwall "$3: $4<" gan MySQL.',
'noconnect'            => "Mae'n ddrwg gennym ni! Oherwydd anawsterau technegol, nid yw'r wici yn gallu cysylltu â gweinydd y databas. <br />
$1",
'nodb'                 => 'Ddim yn gallu dewis y databas $1',
'cachederror'          => "Codwyd y copi hwn o'r dudalen y gofynasoch amdani o gelc; efallai nad yw hi'n gyfamserol.",
'laggedslavemode'      => "Rhybudd: hwyrach nad yw'r dudalen yn cynnwys diwygiadau diweddar.",
'readonly'             => 'Databas ar glo',
'enterlockreason'      => "Rhowch eglurhad dros gloi'r databas, ac amcangyfrif hyd at pa bryd y bydd y databas dan glo",
'readonlytext'         => "Mae databas Wicipedia ar glo; nid yw'n bosib cadw erthyglau newydd na gwneud unrhyw newid arall. Mae'n debygol fod hyn er mwyn cynnal a chadw'r databas -- fe fydd ar gael eto cyn bo hir.

Rhoddwyd y rheswm canlynol gan y gweinyddwr a'i glodd: $1",
'missingarticle-diff'  => '(Gwahaniaeth: $1, $2)',
'readonly_lag'         => "Mae'r databas wedi'i gloi'n awtomatig tra bod y gwas-weinyddion yn asio gyda'r prif weinydd",
'internalerror'        => 'Gwall mewnol',
'internalerror_info'   => 'Gwall mewnol: $1',
'filecopyerror'        => 'Wedi methu copïo\'r ffeil "$1" i "$2".',
'filerenameerror'      => "Wedi methu ail-enwi'r ffeil '$1' yn '$2'.",
'filedeleteerror'      => 'Wedi methu dileu\'r ffeil "$1".',
'directorycreateerror' => 'Wedi methu creu\'r cyfeiriadur "$1".',
'filenotfound'         => "Heb gael hyd i'r ffeil '$1'.",
'fileexistserror'      => 'Nid oes modd ysgrifennu i\'r ffeil "$1": ffeil eisoes ar glawr',
'unexpected'           => 'Gwerth annisgwyl: "$1"="$2".',
'formerror'            => 'Gwall: Wedi methu danfon y ffurflen',
'badarticleerror'      => "Mae'n amhosib cyflawni'r weithred hon ar y dudalen hon.",
'cannotdelete'         => "Mae'n amhosib dileu'r dudalen neu'r ddelwedd hon. (Efallai fod rhywun arall eisoes wedi'i dileu).",
'badtitle'             => 'Teitl gwael',
'badtitletext'         => "Mae'r teitl a ofynnwyd amdano yn annilys, yn wag, neu cysylltu'n anghywir rhwng ieithoedd neu wicïau. Gall fod ynddo un nod neu ragor na ellir eu defnyddio mewn teitlau.",
'perfdisabled'         => "Ymddiheurwn! Mae'r nodwedd hon wedi'i analluogi dros dro gan ei bod yn ormod o dreth ar y databas.",
'perfcached'           => "Mae'r wybodaeth ganlynol yn gopi cadw; mae'n bosib nad y fersiwn diweddaraf ydyw.",
'perfcachedts'         => 'Rhoddwyd y data canlynol ar gadw mewn celc a ddiweddarwyd ddiwethaf am $1.',
'querypage-no-updates' => "Ar hyn o bryd, nid yw'r meddalwedd wedi ei osod i ddiweddaru data'r dudalen hon.",
'wrong_wfQuery_params' => 'Paramedrau anghywir i wfQuery()<br />
Ffwythiant: $1<br />
Gofyniad: $2',
'viewsource'           => 'Dangos côd y dudalen',
'viewsourcefor'        => 'ar gyfer $1',
'actionthrottled'      => 'Tagwyd y weithred',
'actionthrottledtext'  => "Mae camau gwrth-sbam y wici yn cyfyngu ar ba mor aml y gall defnyddwyr ailwneud y weithred hon mewn byr amser, ac rydych chi wedi croesi'r terfyn.
Ceisiwch eto ymhen rhai munudau.",
'protectedpagetext'    => "Mae'r dudalen hon wedi'i diogelu rhag cael ei golygu.",
'viewsourcetext'       => 'Cewch weld a chopïo côd y dudalen:',
'protectedinterface'   => 'Testun ar gyfer rhyngwyneb y wici yw cynnwys y dudalen hon. Clowyd y dudalen er mwyn ei diogeli.',
'editinginterface'     => "'''Dalier sylw:''' Rydych yn golygu tudalen sy'n rhan o destun rhyngwyneb y meddalwedd. Bydd newidiadau i'r dudalen hon yn effeithio ar y rhyngwyneb a ddefnyddir gan eraill. Os am gyfieithu'r neges, ystyriwch ddefnyddio [http://translatewiki.net/wiki/Main_Page?setlang=cy Betawiki], sef y prosiect MediaWiki sy'n hyrwyddo creu wicïau amlieithog.",
'sqlhidden'            => '(cuddiwyd chwiliad SQL)',
'cascadeprotected'     => "Gwarchodwyd y dudalen hon rhag ei newid, oherwydd ei bod wedi ei chynnwys yn y {{PLURAL:$1|dudalen ganlynol|dudalen ganlynol|tudalennau canlynol|tudalennau canlynol|tudalennau canlynol|tudalennau canlynol}}, a {{PLURAL:$1|honno yn ei thro wedi ei|honno yn ei thro wedi ei|rheiny yn eu tro wedi eu|rheiny yn eu tro wedi eu|rheiny yn eu tro wedi eu|rheiny yn eu tro wedi eu}} gwarchod, a'r dewisiad 'sgydol' ynghynn:
$2",
'namespaceprotected'   => "Nid oes caniatâd gennych i olygu tudalennau yn y parth '''$1'''.",
'customcssjsprotected' => "Nid oes caniatad ganddoch i olygu'r dudalen hon oherwydd bod gosodiadau personol defnyddiwr arall arno.",
'ns-specialprotected'  => 'Ni ellir golygu tudalennau arbennig.',
'titleprotected'       => "Diogelwyd y teitl hwn rhag ei greu gan [[User:$1|$1]].
Rhoddwyd y rheswm hwn - ''$2''.",

# Login and logout pages
'logouttitle'                => "Allgofnodi'r defnyddiwr",
'logouttext'                 => '<strong>Rydych wedi allgofnodi.</strong>

Gallwch ddefnyddio {{SITENAME}} yn anhysbys, neu fe allwch [[Special:UserLogin|fewngofnodi eto]] wrth yr un un enw neu wrth enw arall. 
Sylwer y bydd rhai tudalennau yn parhau i ymddangos fel ag yr oeddent pan oeddech wedi mewngofnodi hyd nes i chi glirio celc eich porwr.',
'welcomecreation'            => "==Croeso, $1!==
Mae eich cyfrif wedi'i greu. 
Cofiwch osod y [[Special:Preferences|dewisiadau]] sydd fwyaf hwylus i chi ar {{SITENAME}}.",
'loginpagetitle'             => 'Mewngofnodi',
'yourname'                   => 'Eich enw defnyddiwr:',
'yourpassword'               => 'Eich cyfrinair:',
'yourpasswordagain'          => 'Ail-deipiwch y cyfrinair:',
'remembermypassword'         => "Y cyfrifiadur hwn i gofio'r cyfrinair",
'yourdomainname'             => 'Eich parth',
'externaldberror'            => "Naill ai: cafwyd gwall dilysu allanol ar databas neu: ar y llaw arall efallai nad oes hawl gennych chi i ddiwygio'ch cyfrif allanol.",
'loginproblem'               => '<b>Ni lwyddodd y mewngofnodi.</b><br />Ceisiwch eto!',
'login'                      => 'Mewngofnodi',
'nav-login-createaccount'    => 'Mewngofnodi',
'loginprompt'                => "Mae'n rhaid galluogi cwcis er mwyn mewngofnodi i {{SITENAME}}.",
'userlogin'                  => 'Mewngofnodi',
'logout'                     => 'Allgofnodi',
'userlogout'                 => 'Allgofnodi',
'notloggedin'                => 'Nid ydych wedi mewngofnodi',
'nologin'                    => 'Dim cyfrif gennych? $1.',
'nologinlink'                => 'Crëwch gyfrif',
'createaccount'              => 'Creu cyfrif newydd',
'gotaccount'                 => 'Oes cyfrif gennych eisoes? $1.',
'gotaccountlink'             => 'Mewngofnodwch',
'createaccountmail'          => 'trwy e-bost',
'badretype'                  => "Nid yw'r cyfrineiriau'n union yr un fath.",
'userexists'                 => 'Mae rhywun arall wedi dewis yr enw defnyddiwr hwn. Dewiswch un arall os gwelwch yn dda.',
'youremail'                  => 'Eich cyfeiriad e-bost',
'username'                   => 'Enw defnyddiwr:',
'uid'                        => 'ID Defnyddiwr:',
'prefs-memberingroups'       => "Yn aelod o'r {{PLURAL:$1|grŵp|grŵp|grwpiau|grwpiau|grwpiau|grwpiau}} canlynol:",
'yourrealname'               => 'Eich enw cywir*',
'yourlanguage'               => 'Iaith rhyngwyneb',
'yourvariant'                => 'Amrywiad',
'yournick'                   => 'Eich llysenw (fel llofnod):',
'badsig'                     => 'Llofnod crai annilys; gwiriwch y tagiau HTML.',
'badsiglength'               => "Mae'r llysenw'n rhy hir. 
Rhaid iddo fod yn llai na $1 {{PLURAL:$1|llythyren|lythyren|lythyren|lythyren|llythyren|llythyren}} o hyd.",
'email'                      => 'E-bost',
'prefs-help-realname'        => '* Enw iawn (dewisol): Os ydych yn dewis ei roi, fe fydd yn cael ei ddefnyddio er mwyn rhoi cydnabyddiaeth i chi am eich gwaith.',
'loginerror'                 => 'Problem mewngofnodi',
'prefs-help-email'           => "* E-bost (dewisol): Mae'n galluogi eraill i gysylltu â chi trwy eich tudalen defnyddiwr neu dudalen sgwrs, heb ddatguddio eich manylion personol.",
'prefs-help-email-required'  => 'Cyfeiriad e-bost yn angenrheidiol.',
'nocookiesnew'               => "Mae'r cyfrif defnyddiwr wedi cael ei greu, ond nid ydych wedi mewngofnodi. Mae {{SITENAME}} yn defnyddio cwcis wrth i ddefnyddwyr fewngofnodi. Rydych chi wedi analluogi cwcis. Mewngofnodwch eto gyda'ch enw defnyddiwr a'ch cyfrinair newydd os gwelwch yn dda, ar ôl galluogi cwcis.",
'nocookieslogin'             => 'Mae {{SITENAME}} yn defnyddio cwcis wrth i ddefnyddwyr fewngofnodi. Rydych chi wedi analluogi cwcis. Trïwch eto os gwelwch yn dda, ar ôl galluogi cwcis.',
'noname'                     => 'Dydych chi ddim wedi cynnig enw defnyddiwr dilys.',
'loginsuccesstitle'          => 'Llwyddodd y mewngofnodi',
'loginsuccess'               => "'''Yr ydych wedi mewngofnodi i {{SITENAME}} wrth yr enw \"\$1\".'''",
'nosuchuser'                 => "Does yna'r un defnyddiwr â'r enw '$1'. Sicrhewch eich bod chi wedi'i sillafu'n iawn, neu crëwch gyfrif newydd.",
'nosuchusershort'            => 'Does dim defnyddiwr o\'r enw "<nowiki>$1</nowiki>". Gwiriwch eich sillafu.',
'nouserspecified'            => "Mae'n rhaid nodi enw defnyddiwr.",
'wrongpassword'              => "Nid yw'r cyfrinair a deipiwyd yn gywir. Rhowch gynnig arall arni, os gwelwch yn dda.",
'wrongpasswordempty'         => 'Roedd y cyfrinair yn wag. Rhowch gynnig arall arni.',
'passwordtooshort'           => "Mae eich cyfrinair yn rhy fyr neu'n annilys. Mae'n rhaid iddo gynnwys o leia $1 {{PLURAL:$1|nod|nod|nod|nod|nod|nod}} a bod yn wahanol i'ch enw defnyddiwr.",
'mailmypassword'             => 'Anfoner cyfrinair newydd ataf trwy e-bost',
'passwordremindertitle'      => 'Hysbysu cyfrinair dros dro newydd ar gyfer {{SITENAME}}',
'passwordremindertext'       => "Mae rhywun (chi mwy na thebyg, o'r cyfeiriad IP $1) wedi gofyn i ni anfon cyfrinair newydd ar gyfer {{SITENAME}} atoch ($4).
Mae cyfrinair y defnyddiwr '$2' wedi'i newid i '$3'. Dylid mewngofnodi a'i newid cyn gynted â phosib.

Os mai rhywun arall a holodd am y cyfrinair, ynteu eich bod wedi cofio'r hen gyfrinair, ac nac ydych am newid y cyfrinair, rhydd i chi anwybyddu'r neges hon a pharhau i ddefnyddio'r hen un.",
'noemail'                    => "Does dim cyfeiriad e-bost yng nghofnodion y defnyddiwr '$1'.",
'passwordsent'               => 'Mae cyfrinair newydd wedi\'i ddanfon at gyfeiriad e-bost cofrestredig "$1". Mewngofnodwch eto ar ôl i chi dderbyn y cyfrinair, os gwelwch yn dda.',
'eauthentsent'               => 'Anfonwyd e-bost o gadarnhâd at y cyfeiriad a benwyd.
Cyn y gellir anfon unrhywbeth arall at y cyfeiriad hwnnw rhaid i chi ddilyn y cyfarwyddiadau yn yr e-bost hwnnw er mwyn cadarnhau bod y cyfeiriad yn un dilys.',
'throttled-mailpassword'     => "Anfonwyd e-bost atoch i'ch atgoffa o'ch cyfrinair eisoes, yn ystod y $1 {{PLURAL:$1|awr|awr|awr|awr|awr|awr}} diwethaf.
Er mwyn rhwystro camddefnydd, dim ond un e-bost i'ch atgoffa o'ch cyfrinair gaiff ei anfon bob yn $1 {{PLURAL:$1|awr|awr|awr|awr|awr|awr}}.",
'mailerror'                  => 'Gwall wrth ddanfon e-bost: $1',
'acct_creation_throttle_hit' => 'Rydych chi wedi creu $1 cyfrif yn barod. Ni chewch greu rhagor.',
'emailauthenticated'         => 'Cadarnhawyd eich cyfeiriad e-bost ar $1.',
'emailnotauthenticated'      => "Nid yw eich cyfeiriad e-bost wedi'i ddilysu eto. Ni fydd unrhyw negeseuon e-bost yn cael eu hanfon atoch ar gyfer y nodweddion canlynol.",
'noemailprefs'               => "<strong>Mae'n rhaid i chi gynnig cyfeiriad e-bost er mwyn i'r nodweddion hyn weithio.</strong>",
'emailconfirmlink'           => 'Cadarnhewch eich cyfeiriad e-bost',
'invalidemailaddress'        => 'Ni allwn dderbyn y cyfeiriad e-bost gan fod ganddo fformat annilys. Mewnbynnwch cyfeiriad dilys neu gwagiwch y maes hwnnw, os gwelwch yn dda.',
'accountcreated'             => 'Crëwyd y cyfrif',
'accountcreatedtext'         => 'Crëwyd cyfrif defnyddiwr ar gyfer $1.',
'createaccount-title'        => 'Creu cyfrif ar {{SITENAME}}',
'createaccount-text'         => 'Creodd rhywun gyfrif o\'r enw $2 ar {{SITENAME}} ($4) ar gyfer y cyfeiriad e-bost hwn. "$3" yw\'r cyfrinair ar gyfer "$2". Dylech fewngofnodi a newid eich cyfrinair yn syth.

Rhydd ichi anwybyddu\'r neges hon os mai camgymeriad oedd creu\'r cyfrif.',
'loginlanguagelabel'         => 'Iaith: $1',

# Password reset dialog
'resetpass'               => 'Ailosod cyfrinair y cyfrif',
'resetpass_announce'      => "Fe wnaethoch fewngofnodi gyda chôd dros dro oddi ar e-bost.
Er mwyn cwblhau'r mewngofnodi, rhaid i chi osod cyfrinair newydd fel hyn:",
'resetpass_header'        => 'Ailosod y cyfrinair',
'resetpass_submit'        => 'Gosod y cyfrinair a mewngofnodi',
'resetpass_success'       => "Llwyddodd y newid i'ch cyfrinair! Wrthi'n mewngofnodi...",
'resetpass_bad_temporary' => 'Cyfrinair dros dro annilys.
Efallai eich bod eisoes wedi llwyddo newid eich cyfrinair neu eich bod wedi gwneud cais am gyfrinair dros dro newydd.',
'resetpass_forbidden'     => 'Ni ellir newid cyfrineiriau ar {{SITENAME}}',
'resetpass_missing'       => 'Dim data ar y ffurflen.',

# Edit page toolbar
'bold_sample'     => 'Testun cryf',
'bold_tip'        => 'Testun cryf',
'italic_sample'   => 'Testun italig',
'italic_tip'      => 'Testun italig',
'link_sample'     => 'Teitl y cyswllt',
'link_tip'        => 'Cyswllt mewnol',
'extlink_sample'  => 'http://www.example.com teitl y cyswllt',
'extlink_tip'     => 'Cyswllt allanol (cofiwch y rhagddodiad http:// )',
'headline_sample' => 'Testun pennawd',
'headline_tip'    => 'Pennawd lefel 2',
'math_sample'     => 'Gosodwch fformwla yma',
'math_tip'        => 'Fformwla mathemategol (LaTeX)',
'nowiki_sample'   => 'Rhowch destun di-fformatedig yma',
'nowiki_tip'      => "Anwybyddu'r gystrawen wici",
'image_sample'    => 'Enghraifft.jpg',
'image_tip'       => 'Ffeil mewnosodol',
'media_sample'    => 'Example.mp3',
'media_tip'       => 'Cyswllt ffeil media',
'sig_tip'         => 'Eich llofnod gyda stamp amser',
'hr_tip'          => "Llinell lorweddol (peidiwch â'i gor-ddefnyddio)",

# Edit pages
'summary'                          => 'Crynodeb',
'subject'                          => 'Pwnc/pennawd',
'minoredit'                        => 'Golygiad bychan yw hwn',
'watchthis'                        => 'Gwylier y dudalen hon',
'savearticle'                      => "Cadw'r dudalen",
'preview'                          => 'Rhagolwg',
'showpreview'                      => 'Dangos rhagolwg',
'showlivepreview'                  => 'Rhagolwg byw',
'showdiff'                         => 'Dangos newidiadau',
'anoneditwarning'                  => "'''Dalier sylw''': Nid ydych wedi mewngofnodi. Fe fydd eich cyfeiriad IP yn ymddangos ar hanes golygu'r dudalen hon. Gallwch ddewis cuddio'ch cyfeiriad IP drwy greu cyfrif (a mewngofnodi) cyn golygu.",
'missingsummary'                   => "'''Sylwer:''' Nid ydych wedi gosod nodyn yn y blwch 'Crynodeb'.
Os y pwyswch eto ar 'Cadw'r dudalen' caiff y golygiad ei gadw heb nodyn.",
'missingcommenttext'               => 'Rhowch eich sylwadau isod.',
'missingcommentheader'             => "'''Nodyn:''' Nid ydych wedi cynnig unrhywbeth yn y blwch 'Pwnc/Pennawd:'. Os y cliciwch 'Cadw'r dudalen' eto fe gedwir y golygiad heb bennawd.",
'summary-preview'                  => "Rhagolwg o'r crynodeb",
'subject-preview'                  => 'Rhagolwg pwnc/pennawd',
'blockedtitle'                     => "Mae'r defnyddiwr hwn wedi cael ei flocio",
'blockedtext'                      => "<big>'''Mae eich enw defnyddiwr neu gyfeiriad IP wedi cael ei flocio.'''</big>

$1 a osododd y bloc.
Y rheswm a roddwyd dros y blocio yw: ''$2''.

*Dechreuodd y bloc am: $8
*Bydd y bloc yn dod i ben am: $6
*Bwriadwyd blocio: $7

Gallwch gysylltu â $1 neu un arall o'r [[{{MediaWiki:Grouppage-sysop}}|gweinyddwyr]] i drafod y bloc.
Sylwch mai dim ond y rhai sydd wedi gosod cyfeiriad e-bost yn eu [[Special:Preferences|dewisiadau defnyddiwr]], a hwnnw heb ei flocio, sydd yn gallu 'anfon e-bost at ddefnyddiwr' trwy'r wici.
$3 yw eich cyfeiriad IP presennol. Cyfeirnod y bloc yw #$5. 
Pan yn ysgrifennu at weinyddwr, cofiwch gynnwys yr holl fanylion uchod, os gwelwch yn dda.",
'autoblockedtext'                  => "Rhoddwyd bloc yn awtomatig ar eich cyfeiriad IP oherwydd iddo gael ei ddefnyddio gan ddefnyddiwr arall, a bod bloc wedi ei roi ar hwnnw gan $1.
Y rheswm a roddwyd dros y bloc oedd:

:''$2''

*Dechreuodd y bloc am: $8
*Daw'r bloc i ben am: $6
*Bwriadwyd blocio: $7

Gallwch gysylltu â $1 neu un arall o'r [[{{MediaWiki:Grouppage-sysop}}|gweinyddwyr]] i drafod y bloc.

Sylwch mai dim ond y rhai sydd wedi gosod cyfeiriad e-bost yn eu [[Special:Preferences|dewisiadau defnyddiwr]], a hwnnw heb ei flocio, sydd yn gallu 'anfon e-bost at ddefnyddiwr' trwy'r wici.

Eich cyfeiriad IP presennol yw $3. Cyfeirnod y bloc yw $5. Nodwch y manylion hyn wrth drafod y bloc.",
'blockednoreason'                  => 'dim rheswm wedi ei roi',
'blockedoriginalsource'            => "Dangosir côd '''$1''' isod:",
'whitelistedittitle'               => 'Rhaid mewngofnodi i golygu',
'whitelistedittext'                => 'Rhaid $1 i olygu tudalennau.',
'confirmedittitle'                 => 'Cadarnhad trwy e-bost cyn dechrau golygu.',
'confirmedittext'                  => "Mae'n rhaid i chi gadarnhau eich cyfeiriad e-bost cyn y gallwch ddechrau golygu tudalennau.
Gosodwch eich cyfeiriad e-bost drwy eich [[Special:Preferences|dewisiadau defnyddiwr]] ac yna'i gadarnhau, os gwelwch yn dda.",
'nosuchsectiontitle'               => 'Yr adran ddim yn bod',
'nosuchsectiontext'                => "Rydych wedi ceisio golygu adran nad ydy'n bod. Gan nad oes adran o'r enw $1, ni ellir rhoi eich golygiad ar gadw.",
'loginreqtitle'                    => 'Mae angen mewngofnodi',
'loginreqlink'                     => 'mewngofnodi',
'loginreqpagetext'                 => "Mae'n rhaid $1 er mwyn gweld tudalennau eraill.",
'accmailtitle'                     => 'Wedi danfon cyfrinair.',
'accmailtext'                      => 'Anfonwyd cyfrinair "$1" at $2.',
'newarticle'                       => '(Newydd)',
'newarticletext'                   => "Rydych chi wedi dilyn cysylltiad i dudalen sydd heb gael ei chreu eto.
I greu'r dudalen, dechreuwch deipio yn y blwch isod (gweler y [[{{MediaWiki:Helppage}}|dudalen gymorth]] am fwy o wybodaeth).
Os daethoch yma ar ddamwain, cliciwch botwm '''n&ocirc;l''' y porwr.",
'anontalkpagetext'                 => "---- ''Dyma dudalen sgwrs defnyddiwr sydd heb greu cyfrif, neu nad yw'n defnyddio'i gyfrif. Mae'n rhaid i ni ddefnyddio'r cyfeiriad IP i'w (h)adnabod. Mae'n bosib fod sawl defnyddiwr yn rhannu'r un cyfeiriad IP. Os ydych chi'n ddefnyddiwr anhysbys ac yn teimlo'ch bod wedi derbyn sylwadau amherthnasol, [[Special:UserLogin/signup|crëwch gyfrif]] neu [[Special:UserLogin|mewngofnodwch]] i osgoi dryswch gyda defnyddwyr anhysbys o hyn ymlaen.''",
'noarticletext'                    => "Mae'r dudalen hon yn wag. Gallwch [[Special:Search/{{PAGENAME}}|chwilio am y teitl hwn]] ar dudalennau eraill neu [{{fullurl:{{FULLPAGENAME}}|action=edit}} golygu'r dudalen].",
'userpage-userdoesnotexist'        => 'Nid oes defnyddiwr a\'r enw "$1" yn bod. Gwnewch yn siwr eich bod am greu/golygu\'r dudalen hon.',
'clearyourcache'                   => "'''Sylwer - Wedi i chi roi'r dudalen ar gadw, efallai y bydd angen mynd heibio celc eich porwr er mwyn gweld y newidiadau.''' 
'''Mozilla / Firefox / Safari:''' pwyswch ar ''Shift'' tra'n clicio ''Ail-lwytho/Reload'', neu gwasgwch ''Ctrl-F5'' neu ''Ctrl-R'' (''Command-R'' ar Macintosh); '''Konqueror:''' cliciwch y botwm ''Ail-lwytho/Reload'', neu gwasgwch ''F5''; '''Opera:''' gwacewch y celc yn llwyr trwy ''Offer → Dewisiadau / Tools→Preferences''; '''Internet Explorer:''' pwyswch ar ''Ctrl'' tra'n clicio ''Adnewyddu/Refresh'', neu gwasgwch ''Ctrl-F5''.",
'usercssjsyoucanpreview'           => "<strong>Tip:</strong> Defnyddiwch y botwm 'Dangos rhagolwg' er mwyn profi eich CSS/JS newydd cyn ei gadw.",
'usercsspreview'                   => "'''Cofiwch -- dim ond rhagolwg o'ch CSS defnyddiwr yw hwn; nid yw wedi'i gadw eto!'''",
'userjspreview'                    => "'''Cofiwch -- dim ond rhagolwg o'ch JavaScript yw hwn; nid yw wedi'i gadw eto!'''",
'updated'                          => '(Diweddariad)',
'note'                             => '<strong>Dalier sylw:</strong>',
'previewnote'                      => "<strong>Cofiwch taw rhagolwg yw hwn; nid yw'r dudalen wedi ei chadw eto.</strong>",
'previewconflict'                  => "Mae'r rhagolwg hwn yn dangos y testun yn yr ardal golygu uchaf, fel ag y byddai'n ymddangos petaech yn rhoi'r dudalen ar gadw.",
'session_fail_preview'             => "<strong>Ymddiheurwn! Methwyd prosesu eich golygiad gan fod rhan o ddata'r sesiwn wedi'i golli. Ceisiwch eto. 
Os digwydd yr un peth eto, ceisiwch [[Special:UserLogout|allgofnodi]] ac yna mewngofnodi eto.</strong>",
'session_fail_preview_html'        => "<strong>Ymddiheurwn! Methwyd prosesu eich golygiad gan fod rhan o ddata'r sesiwn wedi'i golli.</strong>

''Oherwydd bod HTML amrwd ar waith ar {{SITENAME}}, cuddir y rhagolwg er mwyn gochel rhag ymosodiad JavaScript.''

<strong>Os ydych am wneud golygiad dilys, ceisiwch eto. 
Os methwch unwaith eto, ceisiwch [[Special:UserLogout|allgofnodi]] ac yna mewngofnodi unwaith eto.</strong>",
'token_suffix_mismatch'            => "<strong>Gwrthodwyd eich golygiad oherwydd bod eich gweinydd cleient wedi gwneud cawl o'r atalnodau yn y tocyn golygu.
Gwrthodwyd y golygiad rhag i destun y dudalen gael ei lygru. 
Weithiau fe ddigwydd hyn wrth ddefnyddio dirprwy-wasanaeth anhysbys gwallus yn seiliedig ar y we.</strong>",
'editing'                          => 'Yn golygu $1',
'editingsection'                   => 'Yn golygu $1 (adran)',
'editingcomment'                   => 'Yn golygu $1 (esboniad)',
'editconflict'                     => 'Gwrthdaro golygyddol: $1',
'explainconflict'                  => "Mae rhywun arall wedi newid y dudalen hon ers i chi ddechrau ei golygu hi.
Mae'r ardal testun uchaf yn cynnwys testun y dudalen fel y mae hi rwan.
Mae eich newidiadau chi yn ymddangos yn yr ardal testun isaf.
Bydd yn rhaid i chi gyfuno eich newidiadau chi a'r testun sydd yn bodoli eisioes.
'''Dim ond''' y testun yn yr ardal testun <b>uchaf</b> fydd yn cael ei roi ar gadw pan wasgwch y botwm \"Cadw'r dudalen\".",
'yourtext'                         => 'Eich testun',
'storedversion'                    => "Fersiwn o'r storfa",
'nonunicodebrowser'                => '<strong>RHYBUDD: Nid yw eich porwr yn cydymffurfio ag Unicode. Serch hyn, mae modd i chi olygu tudalennau: bydd nodau sydd ddim yn rhan o ASCII yn ymddangos yn y blwch golygu fel codau hecsadegol.</strong>',
'editingold'                       => "<strong>RHYBUDD: Rydych chi'n golygu hen ddiwygiad o'r dudalen hon. Os caiff ei chadw, bydd unrhyw newidiadau diweddarach yn cael eu colli.</strong>",
'yourdiff'                         => 'Gwahaniaethau',
'copyrightwarning'                 => "Mae pob cyfraniad i {{SITENAME}} yn cael ei ryddhau o dan termau'r Drwydded Ddogfen Rhydd ($2) (gwelwch $1 am fanylion). Os nad ydych chi'n fodlon i'ch gwaith gael ei olygu heb drugaredd, neu i gopïau ymddangos ar draws y we, peidiwch a'i gyfrannu yma.<br />
Rydych chi'n cadarnhau mai chi yw awdur y cyfraniad, neu eich bod chi wedi'i gopïo o'r parth cyhoeddus (''public domain'') neu rywle rhydd tebyg. '''Nid''' yw'r mwyafrif o wefannau yn y parth cyhoeddus.

<strong>PEIDIWCH Â CHYFRANNU GWAITH O DAN HAWLFRAINT HEB GANIATÂD!</strong>",
'copyrightwarning2'                => "Sylwch fod pob cyfraniad i {{SITENAME}} yn cael ei ryddhau o dan termau'r Drwydded Ddogfen Rhydd (gwelwch $1 am fanylion).
Os nad ydych chi'n fodlon i'ch gwaith gael ei olygu heb drugaredd, neu i gopïau ymddangos ar draws y we, peidiwch a'i gyfrannu yma.<br />
Rydych chi'n cadarnhau mai chi yw awdur y cyfraniad, neu eich bod chi wedi'i gopïo o'r parth cyhoeddus (''public domain'') neu rywle rhydd tebyg.<br />
<strong>PEIDIWCH Â CHYFRANNU GWAITH O DAN HAWLFRAINT HEB GANIATÂD!</strong>",
'longpagewarning'                  => "<strong>RHYBUDD: Mae'r dudalen hon yn $1 cilobeit o hyd; mae rhai porwyr yn cael trafferth wrth lwytho tudalennau sy'n hirach na 32kb.
Byddai'n dda o beth llunio sawl tudalen llai o hyd o ddeunydd y dudalen hon.</strong>",
'longpageerror'                    => "<strong>GWALL: Mae'r testun yr ydych wedi ei osod yma yn $1 cilobeit o hyd, ac yn hwy na'r hyd eithaf o $2 cilobeit.
Ni ellir ei roi ar gadw.</strong>",
'readonlywarning'                  => "<strong>RHYBUDD: Mae'r databas wedi'i gloi am gyfnod er mwyn cynnal a chadw, felly fyddwch chi ddim yn gallu cadw'ch golygiadau ar hyn o bryd. Rydyn ni'n argymell eich bod chi'n copïo a gludo'r testun i ffeil a'i gadw ar eich disg tan bod y sustem yn weithredol eto.</strong>",
'protectedpagewarning'             => "<strong>RHYBUDD: Mae'r dudalen hon wedi'i diogelu. Dim ond gweinyddwyr sydd yn gallu ei golygu.</strong>",
'semiprotectedpagewarning'         => "'''Sylwer:''' Mae'r dudalen hon wedi ei chloi; dim ond defnyddwyr cofrestredig a allant ei golygu.",
'cascadeprotectedwarning'          => "'''Dalier sylw:''' Mae'r dudalen hon wedi ei gwarchod fel nad ond defnyddwyr â galluoedd gweinyddwyr sy'n gallu ei newid, oherwydd ei bod yn rhan o'r {{PLURAL:$1|dudalen ganlynol|dudalen ganlynol|tudalennau canlynol|tudalennau canlynol|tudalennau canlynol|tudalennau canlynol}} sydd wedi {{PLURAL:$1|ei|ei|eu|eu|eu|eu}} sgydol-gwarchod.",
'titleprotectedwarning'            => "<strong>RHYBUDD:  Mae'r dudalen hon wedi ei chloi; dim ond rhai defnyddwyr a allant ei chreu.</strong>",
'templatesused'                    => 'Nodiadau a ddefnyddir yn y dudalen hon:',
'templatesusedpreview'             => 'Nodiadau a ddefnyddir yn y rhagolwg hwn:',
'templatesusedsection'             => 'Nodiadau a ddefnyddir yn yr adran hon:',
'template-protected'               => '(wedi ei diogelu)',
'template-semiprotected'           => '(lled-diogelwyd)',
'hiddencategories'                 => "Mae'r dudalen hon yn aelod o $1 {{PLURAL:$1|categori|categori|gategori|chategori|chategori|categori}} cuddiedig:",
'nocreatetitle'                    => 'Cyfyngwyd ar greu tudalennau',
'nocreatetext'                     => "Mae'r safle hwn wedi cyfyngu'r gallu i greu tudalennau newydd. Gallwch olygu tudalen sydd eisoes yn bodoli, neu [[Special:UserLogin|fewngofnodi, neu greu cyfrif]].",
'nocreate-loggedin'                => "Nid yw'r gallu gennych i greu tudalennau ar {{SITENAME}}.",
'permissionserrors'                => 'Gwallau Caniatâd',
'permissionserrorstext'            => "Nid yw'r gallu ganddoch i weithredu yn yr achos yma, am y {{PLURAL:$1|rheswm|rheswm|rhesymau|rhesymau|rhesymau|rhesymau}} canlynol:",
'permissionserrorstext-withaction' => "Nid yw'r gallu hwn ($2) ganddoch, am y {{PLURAL:$1|rheswm|rheswm|rhesymau|rhesymau|rhesymau|rhesymau}} canlynol:",
'recreate-deleted-warn'            => "'''Dalier sylw: Rydych yn ail-greu tudalen a ddilewyd rhywdro.'''

Ystyriwch a fyddai'n dda o beth i barhau i olygu'r dudalen hon.
Dyma lòg dileu'r dudalen, er gwybodaeth:",

# Parser/template warnings
'post-expand-template-inclusion-category' => "Tudalennau a phatrymlun ynddynt sy'n fwy na chyfyngiad y meddalwedd",

# "Undo" feature
'undo-success' => "Gellir dadwneud y golygiad. Byddwch gystal â gwirio'r gymhariaeth isod i sicrhau mai dyma sydd arnoch eisiau gwneud, ac yna rhowch y newidiadau ar gadw i gwblhau'r gwaith o ddadwneud y golygiad.",
'undo-failure' => 'Methwyd a dadwneud y golygiad oherwydd gwrthdaro â golygiadau cyfamserol.',
'undo-norev'   => "Ni ellid dadwneud y golygiad oherwydd nad yw'n bod neu iddo gael ei ddileu.",
'undo-summary' => 'Dadwneud y golygiad $1 gan [[Special:Contributions/$2|$2]] ([[User talk:$2|Sgwrs]] | [[Special:Contributions/$2|{{MediaWiki:Contribslink}}]])',

# Account creation failure
'cantcreateaccounttitle' => 'Yn methu creu cyfrif',
'cantcreateaccount-text' => "Rhwystrwyd y gallu i greu cyfrif ar gyfer y cyfeiriad IP hwn, ('''$1'''), gan [[User:$3|$3]].

Y rheswm a roddwyd dros y bloc gan $3 yw ''$2''.",

# History pages
'viewpagelogs'        => "Dangos logiau'r dudalen hon",
'nohistory'           => "Does dim hanes golygu i'r dudalen hon.",
'revnotfound'         => "Ni ddaethpwyd o hyd i'r diwygiad",
'revnotfoundtext'     => "Ni ddaethpwyd o hyd i'r hen ddiwygiad o'r dudalen y gofynnwyd amdano. Gwnewch yn siwr fod yr URL yn gywir os gwelwch yn dda.",
'currentrev'          => 'Diwygiad cyfoes',
'revisionasof'        => 'Diwygiad $1',
'revision-info'       => 'Y fersiwn a roddwyd ar gadw am $1 gan $2',
'previousrevision'    => '← at y diwygiad blaenorol',
'nextrevision'        => 'At y diwygiad dilynol →',
'currentrevisionlink' => 'Y diwygiad cyfoes',
'cur'                 => 'cyf',
'next'                => 'nesaf',
'last'                => 'cynt',
'page_first'          => 'cyntaf',
'page_last'           => 'olaf',
'histlegend'          => "Cymharu dau fersiwn: marciwch y cylchoedd ar y ddau fersiwn i'w cymharu, yna pwyswch ar 'return' neu'r botwm 'Cymharer y fersiynau dewisedig'.<br />
Eglurhad: (cyf.) = gwahaniaethau rhyngddo a'r fersiwn cyfredol,
(cynt) = gwahaniaethau rhyngddo a'r fersiwn cynt, B = golygiad bychan",
'deletedrev'          => '[dilëwyd]',
'histfirst'           => 'Cynharaf',
'histlast'            => 'Diweddaraf',
'historysize'         => '({{PLURAL:$1|$1 beit|$1 beit|$1 feit|$1 beit|$1 beit|$1 beit}})',
'historyempty'        => '(gwag)',

# Revision feed
'history-feed-title'          => 'Hanes diwygio',
'history-feed-description'    => "Hanes diwygio'r dudalen hon ar y wici",
'history-feed-item-nocomment' => '$1 am $2', # user at time

# Revision deletion
'rev-deleted-comment'         => '(sylwad wedi ei ddiddymu)',
'rev-deleted-user'            => '(enw defnyddiwr wedi ei ddiddymu)',
'rev-deleted-event'           => '(tynnwyd gweithred y lòg)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
Tynnwyd y dudalen hon o\'r archif cyhoeddus.
Hwyrach bod manylion pellach ar y [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} lòg dileu].</div>',
'rev-deleted-text-view'       => "<div class=\"mw-warning plainlinks\">
Mae'r diwygiad hwn o'r dudalen wedi cael ei ddiddymu o'r archifau cyhoeddus.
Fel gweinyddwr ar {{SITENAME}} gallwch ei weld;
gall fod manylion yn y [{{fullurl:Special:Log/delete|page={{FULLPAGENAMEE}}}} lòg dileu].</div>",
'rev-delundel'                => 'dangos/cuddio',
'revisiondelete'              => 'Dileu/dad-ddileu diwygiadau',
'revdelete-selected'          => 'Y {{PLURAL:$2|golygiad|golygiad|golygiadau|golygiadau|golygiadau|golygiadau}} dewisedig o [[:$1]]:',
'logdelete-selected'          => "{{PLURAL:$1|Digwyddiad|Digwyddiad|Digwyddiadau|Digwyddiadau|Digwyddiadau|Digwyddiadau}} a ddewiswyd o'r lòg:",
'revdelete-hide-text'         => 'Cuddio testun y diwygiad',
'revdelete-hide-name'         => "Cuddio'r weithred a'r targed",
'revdelete-hide-comment'      => 'Cuddio sylwad golygu',
'revdelete-hide-user'         => 'Cuddio enw defnyddiwr/IP y golygydd',
'revdelete-suppress'          => 'Atal data oddi wrth Weinyddwyr yn ogystal ag eraill',
'revdelete-hide-image'        => 'Cuddio cynnwys y ffeil',
'revdelete-unsuppress'        => "Tynnu'r cyfyngiadau ar y golygiadau a adferwyd",
'revdelete-log'               => 'Sylw ar gyfer y lòg:',
'revdelete-logentry'          => 'newidiwyd y gallu i weld golygiadau ar [[$1]]',
'logdelete-logentry'          => 'newidiwyd y gallu i weld y digwyddiad [[$1]]',
'revdelete-success'           => "'''Llwyddodd y newid i'r gallu i weld golygiadau.'''",
'logdelete-success'           => "'''Llwyddwyd i guddio'r digwyddiad.'''",
'revdel-restore'              => 'Newid gwelededd',
'pagehist'                    => 'Hanes y dudalen',
'deletedhist'                 => 'Hanes dilëedig',
'revdelete-content'           => 'cynnwys',
'revdelete-summary'           => 'crynodeb golygu',
'revdelete-uname'             => 'yr enw defnyddiwr ar gyfer',
'revdelete-restricted'        => 'cyfyngwyd ar allu gweinyddwyr i weld',
'revdelete-unrestricted'      => 'tynnwyd y cyfyngiadau ar allu gweinyddwyr i weld',
'revdelete-hid'               => 'cuddiwyd $1',
'revdelete-unhid'             => 'dangoswyd $1',
'revdelete-log-message'       => '$1 $2 {{PLURAL:$2|golygiad|golygiad|olygiad|golygiad|golygiad|golygiad|}}',

# Suppression log
'suppressionlog'     => 'Lòg cuddio',
'suppressionlogtext' => "Dyma restr y dileuon a'r blociau lle y cuddiwyd cynnwys rhag y gweinyddwyr.
Gallwch weld rhestr y gwaharddiadau a'r blociau gweithredol ar y [[Special:IPBlockList|rhestr blociau IP]].",

# History merging
'mergehistory'                     => 'Cyfuno hanesion y tudalennau',
'mergehistory-header'              => 'Pwrpas y dudalen hon yw cyfuno diwygiadau o hanes un dudalen gwreiddiol ar dudalen newydd.
Pan yn gwneud hyn dylid sicrhau nad yw dilyniant hanes tudalennau yn cael ei ddifetha.',
'mergehistory-box'                 => "Cyfuno'r diwygiadau o ddwy dudalen:",
'mergehistory-from'                => 'Y dudalen wreiddiol:',
'mergehistory-into'                => 'Y dudalen cyrchfan:',
'mergehistory-list'                => 'Hanes diwygiadau y gellir eu cyfuno',
'mergehistory-merge'               => "Gellir cyfuno'r diwygiadau canlynol o [[:$1]] i'r dudalen [[:$2]]. Defnyddiwch y botymau radio i gyfuno dim ond y diwygiadau a grewyd hyd at yr amser penodedig. Sylwch y bydd y golofn botwm radio yn cael ei hail-osod pan ddefnyddir y cysylltau llywio.",
'mergehistory-go'                  => 'Dangos y golygiadau y gellir eu cyfuno',
'mergehistory-submit'              => 'Cyfuner y diwygiadau',
'mergehistory-success'             => "Cyfunwyd $3 {{PLURAL:$3|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}} o [[:$1]] yn llwyddiannus i'r dudalen [[:$2]].",
'mergehistory-fail'                => "Methodd y cyfuno hanes; a wnewch wirio paramedrau'r dudalen a'r amser unwaith eto.",
'mergehistory-no-source'           => "Nid yw'r dudalen gwreiddiol $1 yn bod.",
'mergehistory-no-destination'      => "Nid yw'r dudalen cyrchfan $1 yn bod.",
'mergehistory-invalid-source'      => 'Rhaid bod teitl dilys gan y dudalen gwreiddiol.',
'mergehistory-invalid-destination' => 'Rhaid bod teitl dilys gan y dudalen cyrchfan.',

# Merge log
'mergelog'           => 'Lòg cyfuno',
'pagemerge-logentry' => 'llyncwyd [[$1]] gan [[$2]] (golygiadau hyd at $3)',
'revertmerge'        => 'Daduno',
'mergelogpagetext'   => "Fe ddilyn rhestr o'r achosion diweddaraf o hanes tudalen yn cael ei gyfuno a hanes tudalen arall.",

# Diffs
'history-title'           => "Hanes golygu '$1'",
'difference'              => '(Gwahaniaethau rhwng diwygiadau)',
'lineno'                  => 'Llinell $1:',
'compareselectedversions' => 'Cymharer y fersiynau dewisedig',
'editundo'                => 'dadwneud',
'diff-multi'              => '(Ni ddangosir {{PLURAL:$1|yr $1 diwygiad|yr $1 diwygiad|y $1 ddiwygiad|y $1 diwygiad|y $1 diwygiad|y $1 diwygiad}} rhyngol.)',

# Search results
'searchresults'             => "Canlyniadau'r chwiliad",
'searchresulttext'          => 'Am fwy o wybodaeth am chwilio {{SITENAME}}, gwelwch [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'            => 'Chwiliwyd am \'\'\'[[:$1]]\'\'\' ([[Special:Prefixindex/$1|pob tudalen yn dechrau gyda "$1"]] | [[Special:WhatLinksHere/$1|pob tudalen sy\'n cysylltu â "$1"]])',
'searchsubtitleinvalid'     => "Chwiliwyd am '''$1'''",
'noexactmatch'              => "'''Nid oes tudalen a'r enw '$1' yn bod.''' Gallwch [[:$1|greu'r dudalen]].",
'noexactmatch-nocreate'     => "'''Does dim tudalen a'r enw '$1' yn bod.'''",
'toomanymatches'            => "Cafwyd hyd i ormod o enghreifftiau o'r term chwilio; ceisiwch chwilio am derm arall",
'titlematches'              => 'Teitlau erthygl yn cyfateb',
'notitlematches'            => 'Does dim teitl yn cyfateb',
'textmatches'               => 'Testun erthygl yn cyfateb',
'notextmatches'             => 'Does dim testun yn cyfateb',
'prevn'                     => 'y $1 cynt',
'nextn'                     => 'y $1 nesaf',
'viewprevnext'              => 'Dangos ($1) ($2) ($3).',
'search-result-size'        => '$1 ({{PLURAL:$2|dim geiriau|$2 gair|$2 air|$2 gair|$2 gair|$2 gair|}})',
'search-result-score'       => 'Perthnasedd: $1%',
'search-redirect'           => '(ailgyfeiriad $1)',
'search-section'            => '(adran $1)',
'search-suggest'            => 'Ai am hyn y chwiliwch: $1',
'search-interwiki-caption'  => 'Chwaer-brosiectau',
'search-interwiki-default'  => 'Y canlyniadau o $1:',
'search-interwiki-more'     => '(rhagor)',
'search-mwsuggest-enabled'  => 'gydag awgrymiadau',
'search-mwsuggest-disabled' => 'dim awgrymiadau',
'mwsuggest-disable'         => 'Analluogi awgrymiadau AJAX',
'searchall'                 => 'oll',
'showingresults'            => "Yn dangos $1 {{PLURAL:$1|canlyniad|canlyniad|ganlyniad|chanlyniad|chanlyniad|canlyniad}} isod gan ddechrau gyda rhif '''$2'''.",
'showingresultsnum'         => "Yn dangos $3 {{PLURAL:$3|canlyniad|canlyniad|ganlyniad|chanlyniad|chanlyniad|canlyniad}} isod gan ddechrau gyda rhif '''$2'''.",
'showingresultstotal'       => "Yn dangos {{PLURAL:$3|canlyniad '''$1'''|canlyniad '''$1'''|canlyniadau '''$1 - $2'''|canlyniadau '''$1 - $2'''|canlyniadau '''$1 - $2'''|canlyniadau '''$1 - $2'''}} o'r cyfanswm '''$3'''",
'nonefound'                 => "'''Sylwer''': Dim ond rhai parthau sy'n cael eu chwilio'n ddiofyn. Os ydych am chwilio'r holl barthau (gan gynnwys tudalennau sgwrs, nodiadau, ayb) teipiwch ''all:'' o flaen yr enw. Os am chwilio parth arbennig teipiwch ''enw'r parth:'' o flaen yr enw.",
'powersearch'               => 'Chwilio',
'powersearch-legend'        => 'Chwiliad uwch',
'powersearch-ns'            => 'Chwilio yn y parthau:',
'powersearch-redir'         => 'Rhestru ailgyfeiriadau',
'powersearch-field'         => 'Chwilier am',
'search-external'           => 'Chwiliad allanol',
'searchdisabled'            => "Mae'r teclyn chwilio ar {{SITENAME}} wedi'i analluogi dros dro.
Yn y cyfamser gallwch chwilio drwy Google.
Cofiwch y gall mynegeion Google o gynnwys {{SITENAME}} fod ar ei hôl hi.",

# Preferences page
'preferences'              => 'Dewisiadau',
'mypreferences'            => 'fy newisiadau',
'prefs-edits'              => 'Nifer y golygiadau:',
'prefsnologin'             => 'Nid ydych wedi mewngofnodi',
'prefsnologintext'         => 'Rhaid i chi <span class="plainlinks">[{{fullurl:Special:UserLogin|returnto=$1}} fewngofnodi]</span> er mwyn gosod eich dewisiadau defnyddiwr.',
'prefsreset'               => "Mae'r dewisiadau wedi cael eu hail-osod o'r storfa.",
'qbsettings'               => 'Panel llywio',
'qbsettings-none'          => 'Dim',
'qbsettings-fixedleft'     => 'Sefydlog ar y chwith',
'qbsettings-fixedright'    => 'Sefydlog ar y dde',
'qbsettings-floatingleft'  => 'Yn arnofio ar y chwith',
'qbsettings-floatingright' => 'Yn arnofio ar y dde',
'changepassword'           => 'Newid y cyfrinair',
'skin'                     => 'Croen',
'math'                     => 'Mathemateg',
'dateformat'               => 'Fformat dyddiad',
'datedefault'              => 'Dim dewisiad',
'datetime'                 => 'Dyddiad ac amser',
'math_failure'             => 'Wedi methu dosrannu',
'math_unknown_error'       => 'gwall anhysbys',
'math_unknown_function'    => 'ffwythiant anhysbys',
'math_lexing_error'        => 'gwall lecsio',
'math_syntax_error'        => 'gwall cystrawen',
'math_image_error'         => "Trosiad PNG wedi methu; gwiriwch fod latex, dvips, a gs wedi'u sefydlu'n gywir cyn trosi.",
'math_bad_tmpdir'          => 'Yn methu creu cyfeiriadur mathemateg dros dro, nac ysgrifennu iddo',
'math_bad_output'          => 'Yn methu creu cyfeiriadur allbwn mathemateg nac ysgrifennu iddo',
'math_notexvc'             => 'Rhaglen texvc yn eisiau; gwelwch math/README er mwyn ei chyflunio.',
'prefs-personal'           => 'Data defnyddiwr',
'prefs-rc'                 => 'Newidiadau diweddar',
'prefs-watchlist'          => 'Rhestr gwylio',
'prefs-watchlist-days'     => "Nifer y diwrnodau i'w dangos yn y rhestr gwylio:",
'prefs-watchlist-edits'    => "Nifer y golygiadau i'w dangos wrth ehangu'r rhestr gwylio:",
'prefs-misc'               => 'Amrywiol',
'saveprefs'                => "Cadw'r dewisiadau",
'resetprefs'               => "Clirio'r darpar newidiadau",
'oldpassword'              => 'Hen gyfrinair:',
'newpassword'              => 'Cyfrinair newydd:',
'retypenew'                => 'Ail-deipiwch y cyfrinair newydd:',
'textboxsize'              => 'Golygu',
'rows'                     => 'Rhesi:',
'columns'                  => 'Colofnau:',
'searchresultshead'        => 'Chwilio',
'resultsperpage'           => 'Cyfradd taro fesul tudalen:',
'contextlines'             => "Nifer y llinellau i'w dangos ar gyfer pob hit:",
'contextchars'             => 'Nifer y llythrennau a nodau eraill i bob llinell:',
'stub-threshold'           => 'Trothwy ar gyfer fformatio <a href="#" class="stub">cyswllt eginyn</a> (beitiau):',
'recentchangesdays'        => "Nifer y diwrnodau i'w dangos yn 'newidiadau diweddar':",
'recentchangescount'       => "Nifer y golygiadau i'w dangos ar dudalennau newidiadau diweddar, hanes, a logiau:",
'savedprefs'               => 'Mae eich dewisiadau wedi cael eu cadw.',
'timezonelegend'           => 'Ardal amser',
'timezonetext'             => '¹Nifer yr oriau o wahaniaeth rhwng eich amser lleol ac amser y gweinydd (UTC).',
'localtime'                => 'Amser lleol',
'timezoneoffset'           => 'Atred¹',
'servertime'               => 'Amser y gweinydd yw',
'guesstimezone'            => 'Llenwi oddi wrth y porwr',
'allowemail'               => 'Galluogi e-bost oddi wrth ddefnyddwyr eraill',
'prefs-searchoptions'      => 'Dewisiadau chwilio',
'prefs-namespaces'         => 'Parthau',
'defaultns'                => 'Chwiliwch y parthau rhagosodedig isod:',
'default'                  => 'rhagosodyn',
'files'                    => 'Ffeiliau',

# User rights
'userrights'                  => 'Rheoli galluoedd defnyddwyr', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => 'Rheoli grwpiau defnyddiwr',
'userrights-user-editname'    => 'Rhowch enw defnyddiwr:',
'editusergroup'               => 'Golygu Grwpiau Defnyddwyr',
'editinguser'                 => "Newid galluoedd y defnyddiwr '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup'    => 'Golygu grwpiau defnyddwyr',
'saveusergroups'              => "Cadw'r Grwpiau Defnyddwyr",
'userrights-groupsmember'     => 'Yn aelod o:',
'userrights-groups-help'      => 'Gallwch newid y grwpiau y perthyn y defnyddiwr hwn iddynt:
* Mae defnyddiwr yn perthyn i grŵp pan mae tic yn y bocs.
* Nid yw defnyddiwr yn perthyn i grŵp pan nad oes tic yn y bocs.
* Mae * yn golygu na fyddwch yn gallu dad-wneud unrhyw newid yn y grŵp hwnnw.',
'userrights-reason'           => 'Y rheswm dros y newid:',
'userrights-no-interwiki'     => "Nid yw'r gallu ganddoch i newid galluoedd defnyddwyr ar wicïau eraill.",
'userrights-nodatabase'       => "Nid yw'r bas data $1 yn bod neu nid yw'n un lleol.",
'userrights-nologin'          => 'Rhaid i chi [[Special:UserLogin|fewngofnodi]] ar gyfrif gweinyddwr er mwyn pennu galluoedd defnyddwyr.',
'userrights-notallowed'       => "Nid yw'r gallu i bennu galluoedd defnyddwyr ynghlwm wrth eich cyfrif defnyddiwr.",
'userrights-changeable-col'   => 'Grwpiau y gallwch eu newid',
'userrights-unchangeable-col' => 'Grwpiau na allwch eu newid',

# Groups
'group'               => 'Grŵp:',
'group-user'          => 'Defnyddwyr',
'group-autoconfirmed' => "Defnyddwyr wedi eu cadarnhau'n awtomatig",
'group-bot'           => 'Botiau',
'group-sysop'         => 'Gweinyddwyr',
'group-bureaucrat'    => 'Biwrocratiaid',
'group-all'           => '(oll)',

'group-user-member'          => 'Defnyddiwr',
'group-autoconfirmed-member' => "Defnyddiwr wedi ei gadarnhau'n awtomatig",
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Gweinyddwr',
'group-bureaucrat-member'    => 'Biwrocrat',

'grouppage-user'          => '{{ns:project}}:Defnyddwyr',
'grouppage-autoconfirmed' => "{{ns:project}}:Defnyddwyr wedi eu cadarnhau'n awtomatig",
'grouppage-bot'           => '{{ns:project}}:Botiau',
'grouppage-sysop'         => '{{ns:project}}:Gweinyddwyr',
'grouppage-bureaucrat'    => '{{ns:project}}:Biwrocratiaid',

# Rights
'right-read'             => 'Darllen tudalennau',
'right-edit'             => 'Golygu tudalennau',
'right-createpage'       => 'Creu tudalennau (nad ydynt yn dudalennau sgwrs)',
'right-createtalk'       => 'Creu tudalennau sgwrs',
'right-createaccount'    => 'Creu cyfrifon defnyddwyr newydd',
'right-minoredit'        => "Marcio golygiadau'n rhai bychain",
'right-move'             => 'Symud tudalennau',
'right-move-subpages'    => "Symud tudalennau gyda'u his-dudalennau",
'right-suppressredirect' => "Peidio â chreu ailgyfeiriad o'r hen enw wrth symud tudalen",
'right-upload'           => 'Uwchlwytho ffeiliau',
'right-reupload'         => 'Trosysgrifo ffeil sydd eisoes yn bod',
'right-reupload-own'     => "Trosysgrifo ffeil sydd eisoes yn bod ac wedi ei uwchlwytho gennych chi'ch hunan",
'right-autoconfirmed'    => 'Golygu tudalennau sydd wedi eu lled-ddiogelu',
'right-delete'           => 'Dileu tudalennau',
'right-bigdelete'        => 'Dileu tudalennau a hanes llwythog iddynt',
'right-undelete'         => 'Adfer tudalen dilëedig',
'right-editinterface'    => "Golygu'r rhyngwyneb",
'right-import'           => 'Mewnforio tudalennau o wicïau eraill',
'right-mergehistory'     => 'Cyfuno hanes y tudalennau',
'right-userrights'       => 'Golygu holl alluoedd defnyddwyr',
'right-siteadmin'        => "Cloi a datgloi'r databas",

# User rights log
'rightslog'     => 'Lòg galluoedd defnyddiwr',
'rightslogtext' => 'Lòg y newidiadau i alluoedd defnyddwyr yw hwn.',
'rightsnone'    => '(dim)',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|newid|newid|newid|newid|newid|o newidiadau}}',
'recentchanges'                     => 'Newidiadau diweddar',
'recentchangestext'                 => "Dilynwch y newidiadau diweddaraf i'r wici ar y dudalen hon.",
'recentchanges-feed-description'    => "Dilynwch y newidiadau diweddaraf i'r wici gyda'r porthiant hwn.",
'rcnote'                            => "Isod mae'r '''$1''' newid diweddaraf yn ystod y '''$2''' {{PLURAL:$2|diwrnod|diwrnod|ddiwrnod|diwrnod|diwrnod|diwrnod}} diwethaf, hyd at $5, $4.",
'rcnotefrom'                        => "Isod mae pob newidiad ers '''$2''' (hyd at '''$1''' ohonynt).",
'rclistfrom'                        => 'Dangos newidiadau newydd gan ddechrau o $1',
'rcshowhideminor'                   => '$1 golygiadau bychain',
'rcshowhidebots'                    => '$1 botiau',
'rcshowhideliu'                     => '$1 defnyddwyr mewngofnodedig',
'rcshowhideanons'                   => '$1 defnyddwyr anhysbys',
'rcshowhidepatr'                    => '$1 golygiadau wedi derbyn ymweliad patrôl',
'rcshowhidemine'                    => '$1 fy ngolygiadau',
'rclinks'                           => 'Dangos y $1 newidiad diweddaraf yn ystod y $2 diwrnod diwethaf<br />$3',
'diff'                              => 'gwahan',
'hist'                              => 'hanes',
'hide'                              => 'Cuddio',
'show'                              => 'Dangos',
'minoreditletter'                   => 'B',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1 {{PLURAL:$1|defnyddwyr|defnyddiwr|ddefnyddiwr|defnyddiwr|defnyddiwr|o ddefnyddwyr}} yn gwylio]',
'rc_categories'                     => 'Cyfyngu i gategorïau (gwahanwch gyda "|")',
'rc_categories_any'                 => 'Unrhyw un',
'newsectionsummary'                 => '/* $1 */ adran newydd',

# Recent changes linked
'recentchangeslinked'          => 'Newidiadau perthnasol',
'recentchangeslinked-title'    => 'Newidiadau cysylltiedig â "$1"',
'recentchangeslinked-noresult' => 'Ni chafwyd unrhyw newidiadau i dudalennau cysylltiedig yn ystod cyfnod yr ymholiad.',
'recentchangeslinked-summary'  => "Mae'r dudalen arbennig hon yn dangos y newidiadau diweddaraf i'r tudalennau hynny y mae cyswllt yn arwain atynt ar y dudalen a enwir (neu newidiadau i dudalennau sy'n aelodau o'r categori a enwir). Dangosir tudalennau sydd ar [[Special:Watchlist|eich rhestr gwylio]] mewn print '''trwm'''.",
'recentchangeslinked-page'     => "Enw'r dudalen:",
'recentchangeslinked-to'       => "Dangos newidiadau i'r tudalennau â chyswllt arnynt sy'n arwain at y dudalen a enwir",

# Upload
'upload'                      => 'Uwchlwytho ffeil',
'uploadbtn'                   => 'Uwchlwytho ffeil',
'reupload'                    => 'Ail-uwchlwytho',
'reuploaddesc'                => "Dileu'r uwchlwytho a dychwelyd i'r ffurflen uwchlwytho",
'uploadnologin'               => 'Nid ydych wedi mewngofnodi',
'uploadnologintext'           => "Mae'n rhaid i chi [[Special:UserLogin|fewngofnodi]] er mwyn uwchlwytho ffeiliau.",
'upload_directory_read_only'  => "Ni all y gweinydd ysgrifennu i'r cyfeiriadur uwchlwytho ($1).",
'uploaderror'                 => "Gwall tra'n uwchlwytho ffeil",
'uploadtext'                  => "Defnyddiwch y ffurflen isod i uwchlwytho ffeiliau.
I weld a chwilio am ffeiliau sydd eisoes wedi eu huwchlwytho ewch at y [[Special:ImageList|rhestr o'r ffeiliau sydd wedi eu huwchlwytho]]. I weld cofnodion uwchlwytho a dileu ffeiliau ewch at y [[Special:Log/upload|lòg uwchlwytho]] neu'r [[Special:Log/delete|lòg dileu]].

I osod ffeil mewn tudalen defnyddiwch gyswllt wici, ar un o'r ffurfiau canlynol:
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Ffeil.jpg]]</nowiki><tt>''', er mwyn defnyddio fersiwn llawn y ffeil
*'''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:Ffeil.png|200px|bawd|chwith|testun amgen]]</nowiki><tt>''' a wnaiff dangos llun 200 picsel o led mewn bocs ar yr ochr chwith, a'r testun 'testun amgen' wrth ei odre
*'''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:Ffeil.ogg]]</nowiki><tt>''' a fydd yn arwain yn syth at y ffeil heb arddangos y ffeil.",
'upload-permitted'            => 'Mathau o ffeiliau a ganiateir: $1',
'upload-preferred'            => 'Mathau ffeil dewisol: $1.',
'upload-prohibited'           => 'Mathau o ffeiliau a waherddir: $1.',
'uploadlog'                   => 'lòg uwchlwytho',
'uploadlogpage'               => 'Lòg uwchlwytho',
'uploadlogpagetext'           => "Isod mae rhestr o'r uwchlwythiadau ffeiliau <nowiki>diweddaraf</nowiki>.
Gweler y [[Special:NewImages|galeri o ffeiliau newydd]] i fwrw golwg drostynt.",
'filename'                    => "Enw'r ffeil",
'filedesc'                    => 'Crynodeb',
'fileuploadsummary'           => 'Crynodeb:',
'filestatus'                  => 'Statws hawlfraint:',
'filesource'                  => 'Ffynhonnell:',
'uploadedfiles'               => 'Ffeiliau a uwchlwythwyd',
'ignorewarning'               => "Anwybydder y rhybudd, a rhoi'r dudalen ar gadw beth bynnag",
'ignorewarnings'              => 'Anwybydder pob rhybudd',
'minlength1'                  => 'Rhaid i enwau ffeiliau gynnwys un llythyren neu ragor.',
'illegalfilename'             => 'Mae\'r enw ffeil "$1" yn cynnwys nodau sydd wedi\'u gwahardd mewn teitlau tudalennau. Ail-enwch y ffeil ac uwchlwythwch hi eto os gwelwch yn dda.',
'badfilename'                 => 'Mae enw\'r ffeil wedi\'i newid i "$1".',
'filetype-badmime'            => "Ni chaniateir uwchlwytho ffeiliau o'r math MIME '$1'.",
'filetype-unwanted-type'      => "Mae'r math '''\".\$1\"''' o ffeil yn anghymeradwy.  Mae'n well defnyddio ffeil {{PLURAL:\$3|o'r math|o'r math|o'r mathau|o'r mathau|o'r mathau|o'r mathau}} \$2.",
'filetype-banned-type'        => "Ni chaniateir ffeiliau o'r math '''\".\$1\"'''.  \$2 yw'r {{PLURAL:\$3|math|math|mathau|mathau|mathau|mathau}} o ffeil a ganiateir.",
'filetype-missing'            => "Nid oes gan y ffeil hon estyniad (megis '.jpg').",
'large-file'                  => "Argymhellir na ddylai ffeil fod yn fwy na $1. Mae'r ffeil hwn yn $2 o faint.",
'largefileserver'             => "Mae'r ffeil yn fwy na'r hyn mae'r gweinydd yn ei ganiatau.",
'emptyfile'                   => "Ymddengys fod y ffeil a uwchlwythwyd yn wag. Efallai bod gwall teipio yn enw'r ffeil. Sicrhewch eich bod wir am uwchlwytho'r ffeil.",
'fileexists'                  => "Mae ffeil gyda'r enw hwn eisoes yn bodoli; gwiriwch <strong><tt>$1</tt></strong> os nad ydych yn sicr bod angen ei newid.",
'fileexists-extension'        => "Mae ffeil ag enw tebyg eisoes yn bod:<br />
Enw'r ffeil ar fin ei uwchlwytho: <strong><tt>$1</tt></strong><br />
Enw'r ffeil sydd eisoes yn bod: <strong><tt>$2</tt></strong><br />
Dewiswch enw arall os gwelwch yn dda.",
'fileexists-thumb'            => "<center>'''Y ddelwedd eisoes ar glawr'''</center>",
'fileexists-thumbnail-yes'    => "Ymddengys bod delwedd wedi ei leihau <i>(bawd)</i> ar y ffeil. Cymharwch gyda'r ffeil <strong><tt>$1</tt></strong>.<br />
Os mai'r un un llun ar ei lawn faint sydd ar yr ail ffeil yna does dim angen uwchlwytho llun ychwanegol o faint bawd.",
'file-thumbnail-no'           => "Mae <strong><tt>$1</tt></strong> ar ddechrau enw'r ffeil. Mae'n ymddangos bod y ddelwedd wedi ei leihau <i>(maint bawd)</i>.
Os yw'r ddelwedd ar ei lawn faint gallwch barhau i'w uwchlwytho. Os na, newidiwch enw'r ffeil, os gwelwch yn dda.",
'fileexists-forbidden'        => "Mae ffeil gyda'r enw hwn eisoes yn bodoli; ewch nôl ac uwchlwythwch y ffeil o dan enw newydd.
[[Image:$1|thumb|center|$1]]",
'fileexists-shared-forbidden' => "Mae ffeil gyda'r enw hwn eisoes yn bodoli yn y storfa ffeiliau cyfrannol; ewch nôl ac uwchlwythwch y ffeil o dan enw newydd. [[Image:$1|thumb|center|$1]]",
'successfulupload'            => 'Wedi llwyddo uwchlwytho',
'uploadwarning'               => 'Rhybudd uwchlwytho',
'savefile'                    => "Cadw'r ffeil",
'uploadedimage'               => '"[[$1]]" wedi\'i llwytho',
'overwroteimage'              => "uwchlwythwyd fersiwn newydd o '[[$1]]'",
'uploaddisabled'              => "Ymddiheurwn; mae uwchlwytho wedi'i analluogi.",
'uploaddisabledtext'          => 'Analluogir uwchlwytho ffeiliau ar y wici yma.',
'uploadscripted'              => "Mae'r ffeil hon yn cynnwys HTML neu sgript a all achosi problemau i borwyr gwe.",
'uploadcorrupt'               => 'Mae nam ar y ffeil neu mae ganddi estyniad anghywir. Gwiriwch y ffeil ac uwchlwythwch eto.',
'uploadvirus'                 => 'Mae firws gan y ffeil hon! Manylion: $1',
'sourcefilename'              => "Enw'r ffeil wreiddiol:",
'destfilename'                => 'Enw ffeil y cyrchfan:',
'upload-maxfilesize'          => 'Maint mwyaf ffeil: $1',
'watchthisupload'             => 'Gwylier y dudalen hon',
'upload-wasdeleted'           => "'''Rhybudd: Rydych yn uwchlwytho ffeil sydd eisoes wedi ei dileu.'''

Ail-feddyliwch a ddylech barhau i uwchlwytho'r ffel hon.
Dyma'r lòg dileu ar gyfer y ffeil i chi gael gweld:",
'filename-bad-prefix'         => "Mae'r enw ar y ffeil yr ydych yn ei uwchlwytho yn dechrau gyda <strong>\"\$1\"</strong>. Mae'r math hwn o enw diystyr fel arfer yn cael ei osod yn awtomatig gan gamerâu digidol. Mae'n well gosod enw sy'n disgrifio'r ffeil arno.",

'upload-proto-error'      => 'Protocol gwallus',
'upload-proto-error-text' => "Rhaid cael URLs yn dechrau gyda <code>http://</code> neu <code>ftp://</code> wrth uwchlwytho'n bell.",
'upload-file-error'       => 'Gwall mewnol',
'upload-file-error-text'  => 'Cafwyd gwall mewnol wrth geisio creu ffeil dros dro ar y gweinydd.
Byddwch gystal â chysylltu â [[Special:ListUsers/sysop|gweinyddwr]].',
'upload-misc-error'       => 'Gwall uwchlwytho anhysbys',
'upload-misc-error-text'  => "Cafwyd gwall anghyfarwydd yn ystod yr uwchlwytho.
Sicrhewch bod yr URL yn ddilys ac yn hygyrch a cheisiwch eto.
Os yw'r broblem yn parhau, cysylltwch â [[Special:ListUsers/sysop|gweinyddwr]].",

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => 'Wedi methu cyrraedd yr URL',
'upload-curl-error6-text'  => 'Ni chyrhaeddwyd yr URL a roddwyd.
Gwiriwch yr URL a sicrhau bod y wefan ar waith.',
'upload-curl-error28-text' => 'Oedodd y wefan yn rhy hir cyn ymateb.
Sicrhewch bod y wefan ar waith, arhoswch ennyd, yna ceisiwch eto.
Efallai yr hoffech rhoi cynnig arni ar adeg llai prysur.',

'license'            => 'Trwyddedu:',
'nolicense'          => "Dim un wedi'i ddewis",
'license-nopreview'  => '(Dim rhagolwg ar gael)',
'upload_source_url'  => " (URL dilys, ar gael i'r cyhoedd)",
'upload_source_file' => ' (ffeil ar eich cyfrifiadur)',

# Special:ImageList
'imagelist_search_for'  => "Chwilio am enw'r ddelwedd:",
'imgfile'               => 'ffeil',
'imagelist'             => "Rhestr o'r holl ffeiliau",
'imagelist_date'        => 'Dyddiad',
'imagelist_name'        => 'Enw',
'imagelist_user'        => 'Defnyddiwr',
'imagelist_size'        => 'Maint',
'imagelist_description' => 'Disgrifiad',

# Image description page
'filehist'                       => 'Hanes y ffeil',
'filehist-help'                  => 'Cliciwch ar ddyddiad/amser i weld y ffeil fel ag yr oedd bryd hynny.',
'filehist-deleteall'             => 'eu dileu i gyd',
'filehist-deleteone'             => 'dileu',
'filehist-revert'                => 'gwrthdroi',
'filehist-current'               => 'cyfredol',
'filehist-datetime'              => 'Dyddiad/Amser',
'filehist-user'                  => 'Defnyddiwr',
'filehist-dimensions'            => 'Hyd a lled',
'filehist-filesize'              => 'Maint y ffeil',
'filehist-comment'               => 'Sylw',
'imagelinks'                     => "Cysylltiadau'r ffeil",
'linkstoimage'                   => "Mae'r {{PLURAL:$1|tudalen|dudalen|tudalennau|tudalennau|tudalennau|tudalennau}} isod yn cysylltu i'r ddelwedd hon:",
'nolinkstoimage'                 => 'Nid oes cyswllt ar unrhyw dudalen yn arwain at y ffeil hon.',
'sharedupload'                   => "Mae'r ffeil hon ar gael i'w rannu, felly gall fod ar waith ar brosiectau eraill.",
'shareduploadwiki'               => 'Gwelwch $1 am fwy o fanylion.',
'shareduploadwiki-desc'          => 'Dangosir isod y disgrifiad sydd ar $1 yn y gronfa ar y cyd.',
'shareduploadwiki-linktext'      => 'dudalen disgrifiad y ffeil',
'shareduploadduplicate-linktext' => 'ffeil arall',
'shareduploadconflict-linktext'  => 'ffeil arall',
'noimage'                        => "Does dim ffeil a'r enw hwn i gael, ond gallwch $1.",
'noimage-linktext'               => 'uwchlwytho un',
'uploadnewversion-linktext'      => "Uwchlwytho fersiwn newydd o'r ffeil hon",
'imagepage-searchdupe'           => 'Chwilio am ffeiliau wedi eu dyblygu',

# File reversion
'filerevert'                => 'Gwrthdroi $1',
'filerevert-legend'         => "Gwrthdroi'r ffeil",
'filerevert-intro'          => "Rydych yn gwrthdroi '''[[Media:$1|$1]]''' i'r [fersiwn $4 fel ag yr oedd ar $3, $2].",
'filerevert-comment'        => 'Sylw:',
'filerevert-defaultcomment' => 'Wedi adfer fersiwn $2, $1',
'filerevert-submit'         => 'Gwrthdroi',
'filerevert-success'        => "Mae '''[[Media:$1|$1]]''' wedi cael ei wrthdroi i'r [fersiwn $4 fel ag yr oedd ar $3, $2].",
'filerevert-badversion'     => "Nid oes fersiwn lleol cynt o'r ffeil hwn gyda'r amsernod a nodwyd.",

# File deletion
'filedelete'                  => 'Dileu $1',
'filedelete-legend'           => "Dileu'r ffeil",
'filedelete-intro'            => "Rydych ar fin dileu '''[[Media:$1|$1]]'''.",
'filedelete-intro-old'        => "You are deleting the version of '''[[Media:$1|$1]]''' as of [$4 $3, $2].",
'filedelete-comment'          => 'Sylw:',
'filedelete-submit'           => 'Dileer',
'filedelete-success'          => "Mae '''$1''' wedi cael ei dileu.",
'filedelete-success-old'      => "The version of '''[[Media:$1|$1]]''' as of $3, $2 has been deleted.",
'filedelete-nofile'           => "Nid oes '''$1''' ar y wefan {{SITENAME}}.",
'filedelete-nofile-old'       => "Nid oes fersiwn o '''$1''' gyda'r priodoleddau a enwir yn yr archif.",
'filedelete-iscurrent'        => "Rydych yn ceisio dileu'r fersiwn diweddaraf o'r ffeil hwn. Rhaid gwrthdroi i fersiwn gynt yn gyntaf.",
'filedelete-otherreason'      => 'Rheswm arall/ychwanegol:',
'filedelete-reason-otherlist' => 'Rheswm arall',
'filedelete-reason-dropdown'  => '*Rhesymau cyffredin dros ddileu
** Yn torri hawlfraint
** Dwy ffeil yn union debyg',
'filedelete-edit-reasonlist'  => 'Rhowch reswm dros y dileu',

# MIME search
'mimesearch' => 'Chwiliad MIME',
'mimetype'   => 'Ffurf MIME:',
'download'   => 'islwytho',

# Unwatched pages
'unwatchedpages' => 'Tudalennau sydd â neb yn eu gwylio',

# List redirects
'listredirects' => "Rhestru'r ail-gyfeiriadau",

# Unused templates
'unusedtemplates'    => 'Nodiadau heb eu defnyddio',
'unusedtemplateswlh' => 'cysylltiadau eraill',

# Random page
'randompage'         => 'Tudalen ar hap',
'randompage-nopages' => 'Does dim tudalennau yn y parth hwn.',

# Random redirect
'randomredirect'         => 'Tudalen ailgyfeirio ar hap',
'randomredirect-nopages' => 'Does dim tudalennau ailgyfeirio yn y parth hwn.',

# Statistics
'statistics'             => 'Ystadegau',
'sitestats'              => 'Ystadegau {{SITENAME}}',
'userstats'              => 'Ystadegau defnyddwyr',
'sitestatstext'          => "Mae '''\$1''' {{PLURAL:\$1|tudalen i gyd|tudalen|dudalen i gyd|tudalen i gyd|thudalen i gyd|o dudalennau i gyd}} ar y databas.
Mae hyn yn cynnwys tudalennau \"sgwrs\", tudalennau ynglŷn â {{SITENAME}}, egin erthyglau cwta, ailgyfeiriadau, a thudalennau eraill nad ydynt yn erthyglau go iawn. Ag eithrio'r rhain, mae'n debyg bod yna '''\$2''' {{PLURAL:\$2|erthygl|erthygl|erthygl|erthygl|erthygl|erthygl}} yn y wici.

{{PLURAL:\$1|Ni chafodd unrhyw ffeil ei|Cafodd '''\$8''' ffeil ei|Cafodd '''\$8''' ffeil eu|Cafodd '''\$8''' ffeil eu|Cafodd '''\$8''' ffeil eu|Cafodd '''\$8''' ffeil eu}} huwchlwytho.

Ers sefydlu'r meddalwedd {{PLURAL:\$3|ni chafwyd unrhyw|cafwyd '''\$3'''|cafwyd '''\$3'''|cafwyd '''\$3'''|cafwyd '''\$3'''|cafwyd '''\$3'''}} ymweliad â'r wefan o wefannau eraill a{{PLURAL:\$4|c ni chafwyd unrhyw olygiad|c '''\$4''' golygiad| '''\$4''' olygiad| '''\$4''' golygiad| '''\$4''' golygiad| '''\$4''' golygiad}} i dudalennau.
Ar gyfartaledd felly, bu '''\$5''' golygiad i bob tudalen, a '''\$6''' ymweliad â thudalen ar gyfer pob golygiad.

Hyd y [http://www.mediawiki.org/wiki/Manual:Job_queue rhes dasgau] yw '''\$7'''.",
'userstatstext'          => "Mae '''$1''' {{PLURAL:$1|[[Special:ListUsers|defnyddiwr]]|[[Special:ListUsers|defnyddiwr]]|[[Special:ListUsers|ddefnyddiwr]]|[[Special:ListUsers|defnyddiwr]]|[[Special:ListUsers|defnyddiwr]]|[[Special:ListUsers|defnyddiwr]]}} ar y cofrestr defnyddwyr.
Mae gan '''$2''' (neu '''$4%''') ohonynt alluoedd $5.",
'statistics-mostpopular' => "Tudalennau sy'n derbyn ymweliad amlaf",

'disambiguations'      => 'Tudalennau gwahaniaethu',
'disambiguations-text' => "Mae'r tudalennau canlynol yn cysylltu â thudalennau gwahaniaethu. Yn hytrach dylent gysylltu'n syth â'r erthygl briodol.<br />Diffinir tudalen yn dudalen gwahaniaethu pan mae'n cynnwys un o'r nodiadau '[[MediaWiki:Disambiguationspage|tudalen gwahaniaethu]]'.",

'doubleredirects'     => 'Ailgyfeiriadau dwbl',
'doubleredirectstext' => "Mae pob rhes yn cynnwys cysylltiad i'r ddau ail-gyfeiriad cyntaf, ynghyd â chyrchfan yr ail ailgyfeiriad. Fel arfer bydd hyn yn rhoi'r gwir dudalen y dylai'r tudalennau cynt gyfeirio ati.",

'brokenredirects'        => "Ailgyfeiriadau wedi'u torri",
'brokenredirectstext'    => "Mae'r ailgyfeiriadau isod yn cysylltu â thudalennau sydd heb eu creu eto.",
'brokenredirects-edit'   => '(golygu)',
'brokenredirects-delete' => '(dileu)',

'withoutinterwiki'         => 'Tudalennau heb gysylltiadau ag ieithoedd eraill',
'withoutinterwiki-summary' => 'Nid oes gysylltiad rhwng y tudalennau canlynol a thudalennau mewn ieithoedd eraill:',
'withoutinterwiki-legend'  => 'Rhagddodiad',
'withoutinterwiki-submit'  => 'Dangos',

'fewestrevisions' => "Erthyglau â'r nifer lleiaf o olygiadau iddynt",

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|beit|beit|feit|beit|beit|beit}}',
'ncategories'             => '$1 {{PLURAL:$1|categori|categori|gategori|chategori|chategori|categori}}',
'nlinks'                  => '$1 {{PLURAL:$1|cyswllt|cyswllt|gyswllt|chyswllt|chyswllt|cyswllt}}',
'nmembers'                => '$1 {{PLURAL:$1|aelod|aelod|aelod|aelod|aelod|aelod}}',
'nrevisions'              => '$1 {{PLURAL:$1|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}}',
'nviews'                  => '$1 {{PLURAL:$1|ymweliad|ymweliad|ymweliad|ymweliad|ymweliad|ymweliad}}',
'specialpage-empty'       => "Ni chafwyd canlyniadau i'w hadrodd.",
'lonelypages'             => 'Erthyglau heb gysylltiadau iddynt',
'lonelypagestext'         => 'Nid oes cysylltiad yn arwain at y tudalennau canlynol oddi wrth unrhyw dudalen arall yn {{SITENAME}}.',
'uncategorizedpages'      => 'Tudalennau heb gategori',
'uncategorizedcategories' => 'Categorïau sydd heb gategori',
'uncategorizedimages'     => 'Ffeiliau heb eu categoreiddio',
'uncategorizedtemplates'  => 'Nodiadau heb eu categoreiddio',
'unusedcategories'        => 'Categorïau gwag',
'unusedimages'            => 'Lluniau na ddefnyddiwyd eto',
'popularpages'            => 'Erthyglau poblogaidd',
'wantedcategories'        => 'Categorïau sydd eu hangen',
'wantedpages'             => 'Erthyglau sydd eu hangen',
'missingfiles'            => 'Ffeiliau yn eisiau',
'mostlinked'              => 'Tudalennau yn nhrefn nifer y cysylltiadau iddynt',
'mostlinkedcategories'    => 'Categorïau yn nhrefn nifer eu haelodau',
'mostlinkedtemplates'     => 'Nodiadau yn nhrefn nifer y cysylltiadau iddynt',
'mostcategories'          => 'Erthyglau yn nhrefn nifer eu categorïau',
'mostimages'              => 'Ffeiliau yn nhrefn nifer y cysylltiadau iddynt',
'mostrevisions'           => 'Tudalennau yn nhrefn nifer golygiadau',
'prefixindex'             => 'Mynegai rhagddodiaid',
'shortpages'              => 'Erthyglau byr',
'longpages'               => 'Tudalennau hirion',
'deadendpages'            => 'Tudalennau heb gysylltiadau ynddynt',
'deadendpagestext'        => "Nid oes cysylltiad yn arwain at dudalen arall oddi wrth yr un o'r tudalennau isod.",
'protectedpages'          => 'Tudalennau wedi eu diogelu',
'protectedpagestext'      => "Mae'r tudalennau hyn wedi eu diogelu rhag cael eu symud na'u golygu",
'protectedpagesempty'     => "Does dim tudalennau wedi eu diogelu gyda'r paramedrau hyn.",
'protectedtitles'         => 'Diogelwyd',
'protectedtitlestext'     => "Diogelwyd rhag creu tudalennau gyda'r teitlau hyn",
'protectedtitlesempty'    => "Ar hyn o bryd nid oes unrhyw deitlau wedi eu diogelu a'r paramedrau hyn.",
'listusers'               => 'Rhestr defnyddwyr',
'newpages'                => 'Erthyglau newydd',
'newpages-username'       => 'Enw defnyddiwr:',
'ancientpages'            => 'Erthyglau hynaf',
'move'                    => 'Symud',
'movethispage'            => 'Symud y dudalen hon',
'unusedimagestext'        => "Sylwch y gall gwefannau eraill gysylltu a ffeil drwy URL uniongyrchol. Gan hynny mae'n bosibl fod ffeil wedi ei rhestru yma serch ei bod yn cael defnydd.",
'unusedcategoriestext'    => "Mae'r tudalennau categori isod yn bodoli er nad oes unrhyw dudalen arall yn eu defnyddio.",
'notargettitle'           => 'Dim targed',
'notargettext'            => 'Dydych chi ddim wedi dewis defnyddiwr neu dudalen i weithredu arno.',
'pager-newer-n'           => '{{PLURAL:$1|y $1 mwy diweddar|yr 1 mwy diweddar|y $1 mwy diweddar|y $1 mwy diweddar|y $1 mwy diweddar|y $1 mwy diweddar}}.',
'pager-older-n'           => '{{PLURAL:$1|y $1 cynharach|yr $1 cynharach|y $1 cynharach|y $1 cynharach|y $1 cynharach|y $1 cynharach}}',
'suppress'                => 'Goruchwylio',

# Book sources
'booksources'               => 'Ffynonellau llyfrau',
'booksources-search-legend' => 'Chwilier am lyfrau',
'booksources-go'            => 'Mynd',
'booksources-text'          => "Mae'r rhestr isod yn cynnwys cysylltiadau i wefannau sy'n gwerthu llyfrau newydd a rhai ail-law. Mae rhai o'r gwefannau hefyd yn cynnig gwybodaeth pellach am y llyfrau hyn:",

# Special:Log
'specialloguserlabel'  => 'Defnyddiwr:',
'speciallogtitlelabel' => 'Teitl:',
'log'                  => 'Logiau',
'all-logs-page'        => 'Pob lòg',
'log-search-legend'    => 'Chwilio am logiau',
'log-search-submit'    => 'Eler',
'alllogstext'          => "Mae pob cofnod yn holl logiau {{SITENAME}} wedi cael eu rhestru yma.
Gallwch weld chwiliad mwy penodol trwy ddewis y math o lòg, enw'r defnyddiwr, neu'r dudalen benodedig.
Sylwer bod llythrennau mawr neu fach o bwys i'r chwiliad.",
'logempty'             => 'Does dim eitemau yn cyfateb yn y lòg.',
'log-title-wildcard'   => "Chwilio am deitlau'n dechrau gyda'r geiriau hyn",

# Special:AllPages
'allpages'          => 'Pob tudalen',
'alphaindexline'    => '$1 i $2',
'nextpage'          => 'Y bloc nesaf gan ddechrau gyda ($1)',
'prevpage'          => 'Y bloc cynt gan ddechrau gyda ($1)',
'allpagesfrom'      => 'Dangos pob tudalen gan ddechrau o:',
'allarticles'       => 'Pob erthygl',
'allinnamespace'    => 'Pob tudalen (parth $1)',
'allnotinnamespace' => 'Pob tudalen (heblaw am y parth $1)',
'allpagesprev'      => 'Gynt',
'allpagesnext'      => 'Nesaf',
'allpagessubmit'    => 'Eler',
'allpagesprefix'    => "Dangos pob tudalen gyda'r rhagddodiad:",
'allpagesbadtitle'  => 'Roedd y darpar deitl yn annilys oherwydd bod ynddo naill ai:<p> - rhagddodiad rhyngwici neu ryngieithol, neu </p>- nod neu nodau na ellir eu defnyddio mewn teitlau.',
'allpages-bad-ns'   => 'Nid oes gan {{SITENAME}} barth o\'r enw "$1".',

# Special:Categories
'categories'                    => 'Categorïau',
'categoriespagetext'            => "Mae'r categorïau isod yn cynnwys tudalennau neu ffeiliau.
Ni ddangosir [[Special:UnusedCategories|categorïau gwag]] yma.
Gweler hefyd [[Special:WantedCategories|categorïau sydd eu hangen]].",
'categoriesfrom'                => 'Dangos categorïau gan ddechrau gyda:',
'special-categories-sort-count' => 'trefnu yn ôl nifer',
'special-categories-sort-abc'   => 'trefnu yn ôl yr wyddor',

# Special:ListUsers
'listusersfrom'      => 'Dangos y defnyddwyr gan ddechrau â:',
'listusers-submit'   => 'Dangos',
'listusers-noresult' => "Dim defnyddiwr i'w gael.",

# Special:ListGroupRights
'listgrouprights'         => 'Galluoedd grwpiau defnyddwyr',
'listgrouprights-summary' => "Dyma restr o'r grwpiau defnyddwyr sydd i'w cael ar y wici hon, ynghyd â galluoedd aelodau'r gwahanol grwpiau. Cewch wybodaeth pellach am y gwahanol alluoedd ar y [[{{MediaWiki:Listgrouprights-helppage}}|dudalen gymorth]].",
'listgrouprights-group'   => 'Grŵp',
'listgrouprights-rights'  => 'Galluoedd',
'listgrouprights-members' => '(rhestr aelodau)',

# E-mail user
'mailnologin'     => "Does dim cyfeiriad i'w anfon iddo",
'mailnologintext' => 'Rhaid eich bod wedi [[Special:UserLogin|mewngofnodi]]
a bod cyfeiriad e-bost dilys yn eich [[Special:Preferences|dewisiadau]]
er mwyn medru anfon e-bost at ddefnyddwyr eraill.',
'emailuser'       => 'Anfon e-bost at y defnyddiwr hwn',
'emailpage'       => 'Anfon e-bost at ddefnyddiwr',
'emailpagetext'   => 'Os yw\'r defnyddiwr hwn wedi gosod cyfeiriad e-bost dilys yn ei ddewisiadau, gellir anfon neges ato o\'i ysgrifennu ar y ffurflen isod. 
Bydd y cyfeiriad e-bost a osodoch yn eich [[Special:Preferences|dewisiadau chithau]] yn ymddangos ym maes "Oddi wrth" yr e-bost, fel bod y defnyddiwr arall yn gallu anfon ateb atoch.',
'usermailererror' => 'Dychwelwyd gwall gan y rhaglen e-bost:',
'defemailsubject' => 'E-bost {{SITENAME}}',
'noemailtitle'    => 'Dim cyfeiriad e-bost',
'noemailtext'     => "Mae'r defnyddiwr hwn naill ai heb roi cyfeiriad e-bost dilys, neu mae wedi dewis peidio â derbyn e-bost oddi wrth ddefnyddwyr eraill.",
'emailfrom'       => 'Oddi wrth:',
'emailto'         => 'At:',
'emailsubject'    => 'Pwnc:',
'emailmessage'    => 'Neges:',
'emailsend'       => 'Anfon',
'emailccme'       => "Anfoner gopi o'r neges e-bost ataf.",
'emailccsubject'  => "Copi o'ch neges at $1: $2",
'emailsent'       => "Neges e-bost wedi'i hanfon",
'emailsenttext'   => 'Mae eich neges e-bost wedi cael ei hanfon.',
'emailuserfooter' => 'Anfonwyd yr e-bost hwn oddi wrth $1 at $2 trwy ddefnyddio\'r teclyn "Anfon e-bost at ddefnyddiwr" ar {{SITENAME}}.',

# Watchlist
'watchlist'            => 'Fy rhestr gwylio',
'mywatchlist'          => 'Fy rhestr gwylio',
'watchlistfor'         => "(ar gyfer '''$1''')",
'nowatchlist'          => "Mae eich rhestr gwylio'n wag.",
'watchlistanontext'    => "Rhaid $1 er mwyn gweld neu ddiwygio'ch rhestr gwylio.",
'watchnologin'         => 'Nid ydych wedi mewngofnodi',
'watchnologintext'     => "Mae'n rhaid i chi [[Special:UserLogin|fewngofnodi]] er mwyn newid eich rhestr gwylio.",
'addedwatch'           => 'Rhoddwyd ar eich rhestr gwylio',
'addedwatchtext'       => "Mae'r dudalen \"[[:\$1|\$1]]\" wedi cael ei hychwanegu at eich [[Special:Watchlist|rhestr gwylio]].
Pan fydd y dudalen hon, neu ei thudalen sgwrs, yn newid, fe fyddant yn ymddangos ar eich rhestr gwylio ac hefyd '''yn gryf''' ar restr y [[Special:RecentChanges|newidiadau diweddar]], fel ei bod yn haws eu gweld.

Os ydych am ddiddymu'r dudalen o'r rhestr gwylio, cliciwch ar \"Stopio gwylio\" yn y bar ar frig y dudalen.",
'removedwatch'         => 'Tynnwyd oddi ar eich rhestr gwylio',
'removedwatchtext'     => 'Mae\'r dudalen "[[:$1]]" wedi\'i thynnu oddi ar [[Special:Watchlist|eich rhestr gwylio]].',
'watch'                => 'Gwylio',
'watchthispage'        => 'Gwylier y dudalen hon',
'unwatch'              => 'Stopio gwylio',
'unwatchthispage'      => 'Stopio gwylio',
'notanarticle'         => 'Ddim yn erthygl/ffeil',
'notvisiblerev'        => 'Y diwygiad wedi cael ei ddileu',
'watchnochange'        => "Ni olygwyd dim o'r erthyglau yr ydych yn cadw golwg arnynt yn ystod y cyfnod uchod.",
'watchlist-details'    => 'Mae {{PLURAL:$1|$1 tudalen|$1 dudalen|$1 dudalen|$1 tudalen|$1 thudalen|$1 o dudalennau}} ar eich rhestr gwylio, heb gynnwys tudalennau sgwrs.',
'wlheader-enotif'      => '* Galluogwyd hysbysiadau trwy e-bost.',
'wlheader-showupdated' => "* Mae tudalennau sydd wedi newid ers i chi ymweld ddiwethaf wedi'u '''hamlygu'''.",
'watchmethod-recent'   => "yn chwilio'r diwygiadau diweddar am dudalennau ar y rhestr gwylio",
'watchmethod-list'     => "yn chwilio'r rhestr gwylio am ddiwygiadau diweddar",
'watchlistcontains'    => '{{PLURAL:$1|Nid oes $1 tudalen|Mae $1 dudalen|Mae $1 dudalen|Mae $1 tudalen|Mae $1 thudalen|Mae $1 o dudalennau}} ar eich rhestr gwylio.',
'iteminvalidname'      => "Problem gyda'r eitem '$1', enw annilys...",
'wlnote'               => "Gweler isod y '''$1''' {{PLURAL:$1|newidiad|newidiad|newidiad|newidiad|newidiad|newidiad}} diweddaraf yn ystod y <b>$2</b> {{PLURAL:$1|awr|awr|awr|awr|awr|awr}} ddiwethaf.",
'wlshowlast'           => "Dangoser newidiadau'r $1 awr ddiwethaf neu'r $2 {{PLURAL:$2|diwrnod|diwrnod|ddiwrnod|diwrnod|diwrnod|diwrnod}} diwethaf neu'r $3 newidiadau.",
'watchlist-show-bots'  => 'Dangos golygiadau bot',
'watchlist-hide-bots'  => 'Cuddio golygiadau bot',
'watchlist-show-own'   => 'Dangos fy ngolygiadau',
'watchlist-hide-own'   => 'Cuddio fy ngolygiadau',
'watchlist-show-minor' => 'Dangos golygiadau bychain',
'watchlist-hide-minor' => 'Cuddio golygiadau bychain',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => "Wrthi'n ychwanegu...",
'unwatching' => "Wrthi'n tynnu...",

'enotif_mailer'                => 'Sustem hysbysu {{SITENAME}}',
'enotif_reset'                 => 'Ystyried bod pob tudalen wedi cael ymweliad',
'enotif_newpagetext'           => 'Mae hon yn dudalen newydd.',
'enotif_impersonal_salutation' => 'at ddefnyddiwr {{SITENAME}}',
'changed'                      => 'Newidiwyd',
'created'                      => 'Crewyd',
'enotif_subject'               => '$CHANGEDORCREATED y dudalen \'$PAGETITLE\' ar {{SITENAME}} gan $PAGEEDITOR',
'enotif_lastvisited'           => 'Gwelwch $1 am bob newid ers eich ymweliad blaenorol.',
'enotif_lastdiff'              => 'Gallwch weld y newid ar $1.',
'enotif_anon_editor'           => 'defnyddiwr anhysbys $1',
'enotif_body'                  => 'Annwyl $WATCHINGUSERNAME,

$CHANGEDORCREATED y dudalen \'$PAGETITLE\' ar {{SITENAME}} ar $PAGEEDITDATE gan $PAGEEDITOR; gweler $PAGETITLE_URL am y diwygiad presennol.

$NEWPAGE

Crynodeb y golygydd: $PAGESUMMARY $PAGEMINOREDIT

Cysylltu â\'r golygydd:
e-bost: $PAGEEDITOR_EMAIL
wici: $PAGEEDITOR_WIKI

Os digwydd mwy o olygiadau i\'r dudalen cyn i chi ymweld â hi, ni chewch ragor o negeseuon hysbysu. Nodwn bod modd i chi ailosod y fflagiau hysbysu ar eich rhestr gwylio, ar gyfer y tudalennau rydych yn eu gwylio.

             Sustem hysbysu {{SITENAME}}

--
I newid eich gosodiadau gwylio, ymwelwch â
{{fullurl:{{ns:special}}:Watchlist/edit}}

Am fwy o gymorth ac adborth:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Dilëer y dudalen',
'confirm'                     => 'Cadarnhau',
'excontent'                   => "y cynnwys oedd: '$1'",
'excontentauthor'             => "y cynnwys oedd: '$1' (a'r unig gyfrannwr oedd '[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "y cynnwys cyn blancio oedd: '$1'",
'exblank'                     => 'roedd y dudalen yn wag',
'delete-confirm'              => 'Dileu "$1"',
'delete-legend'               => 'Dileu',
'historywarning'              => "Rhybudd: mae hanes i'r dudalen rydych ar fin ei dileu.",
'confirmdeletetext'           => "Rydych chi ar fin dileu tudalen neu ddelwedd, ynghŷd â'i hanes, o'r data-bas, a hynny'n barhaol.
Os gwelwch yn dda, cadarnhewch eich bod chi wir yn bwriadu gwneud hyn, eich bod yn deall y canlyniadau, ac yn ei wneud yn ôl [[{{MediaWiki:Policy-url}}|polisïau {{SITENAME}}]].",
'actioncomplete'              => "Wedi cwblhau'r weithred",
'deletedtext'                 => 'Mae "<nowiki>$1</nowiki>" wedi\'i ddileu.
Gwelwch y $2 am gofnod o\'r dileuon diweddar.',
'deletedarticle'              => 'wedi dileu "[[$1]]"',
'suppressedarticle'           => 'cuddiwyd "[[$1]]"',
'dellogpage'                  => 'Log dileuon',
'dellogpagetext'              => "Ceir rhestr isod o'r dileadau diweddaraf.",
'deletionlog'                 => 'log dileuon',
'reverted'                    => "Wedi gwrthdroi i'r golygiad cynt",
'deletecomment'               => 'Esboniad am y dileu:',
'deleteotherreason'           => 'Rheswm arall:',
'deletereasonotherlist'       => 'Rheswm arall',
'deletereason-dropdown'       => "*Rhesymau arferol dros ddileu
** Ar gais yr awdur
** Torri'r hawlfraint
** Fandaliaeth",
'delete-edit-reasonlist'      => 'Golygu rhesymau dileu',
'rollback'                    => 'Gwrthdroi golygiadau',
'rollback_short'              => 'Gwrthdroi',
'rollbacklink'                => 'gwrthdroi',
'rollbackfailed'              => 'Methodd y gwrthdroi',
'cantrollback'                => "Wedi methu gwrthdroi'r golygiad; y cyfrannwr diwethaf oedd unig awdur y dudalen hon.",
'alreadyrolled'               => "Nid yw'n bosib dadwneud y golygiad diwethaf i'r dudalen [[:$1|$1]] gan [[User:$2|$2]] ([[User talk:$2|Sgwrs]] | [[Special:Contributions/$2|{{int:contribslink}}]]);
mae rhywun arall eisoes wedi dadwneud y golygiad neu wedi golygu'r dudalen.

[[User:$3|$3]] ([[User talk:$3|Sgwrs]] | [[Special:Contributions/$3|{{int:contribslink}}]]) a wnaeth y golygiad diwethaf.",
'editcomment'                 => 'Crynodeb y golygiad oedd: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => 'Wedi gwrthdroi golygiadau gan [[Special:Contributions/$2|$2]] ([[User talk:$2|Sgwrs]]); wedi adfer y golygiad diweddaraf gan [[User:$1|$1]]', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => "Gwrthdrowyd y golygiadau gan $1; 
wedi gwrthdroi i'r golygiad olaf gan $2.",
'sessionfailure'              => "Mae'n debyg fod yna broblem gyda'ch sesiwn mewngofnodi; diddymwyd y weithred er mwyn diogelu'r sustem rhag ddefnyddwyr maleisus. Gwasgwch botwm 'nôl' eich porwr ac ail-lwythwch y dudalen honno, yna ceisiwch eto.",
'protectlogpage'              => 'Lòg diogelu',
'protectlogtext'              => 'Isod mae rhestr o bob gweithred diogelu (a dad-ddiogelu) tudalen.',
'protectedarticle'            => "wedi diogelu '[[$1]]'",
'modifiedarticleprotection'   => 'newidiwyd y lefel diogelu ar gyfer "[[$1]]"',
'unprotectedarticle'          => 'wedi dad-ddiogelu "[[$1]]"',
'protect-title'               => "Newid y lefel diogelu ar gyfer '$1'",
'protect-legend'              => "Cadarnháu'r diogelu",
'protectcomment'              => 'Sylw:',
'protectexpiry'               => 'Yn dod i ben:',
'protect_expiry_invalid'      => 'Amser terfynu annilys.',
'protect_expiry_old'          => "Mae'r amser darfod yn y gorffennol.",
'protect-unchain'             => "Datgloi'r cyfyngiadau ar symud tudalen",
'protect-text'                => 'Yma, gallwch weld a newid y lefel diogelu ar gyfer y dudalen <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-blocked'      => "Ni allwch newid y lefel diogelu tra eich bod wedi eich blocio.
Dyma'r gosodiadau diogelu cyfredol ar gyfer y dudalen <strong>$1</strong>:",
'protect-locked-dblock'       => "Ni ellir newid y lefel diogelu gan fod y databas dan glo.
Dyma'r gosodiadau diogelu cyfredol ar gyfer y dudalen <strong>$1</strong>:",
'protect-locked-access'       => "Nid yw'r gallu i newid lefel diogelu ar dudalen ynghlwm wrth eich cyfrif defnyddiwr.
Dyma'r gosodiadau diogelu cyfredol ar gyfer y dudalen <strong>$1</strong>:",
'protect-cascadeon'           => "Mae'r dudalen hon wedi ei diogelu ar hyn o bryd oherwydd ei bod wedi ei chynnwys yn y {{PLURAL:$1|dudalen|dudalen|tudalennau|tudalennau|tudalennau|tudalennau}} canlynol sydd wedi {{PLURAL:$1|ei|ei|eu|eu|eu|eu}} diogelu'n rhaeadrol.  Gallwch newid lefel diogelu'r dudalen hon, ond ni fydd hynny'n effeithio ar y diogelu rhaeadrol.",
'protect-default'             => '(rhagosodedig)',
'protect-fallback'            => 'Mynnu\'r gallu "$1"',
'protect-level-autoconfirmed' => 'Blocio defnyddwyr heb gyfrif',
'protect-level-sysop'         => 'Gweinyddwyr yn unig',
'protect-summary-cascade'     => 'sgydol',
'protect-expiring'            => 'yn dod i ben am $1 (UTC)',
'protect-cascade'             => 'Diogelwch dudalennau sydd wedi eu cynnwys yn y dudalen hon (diogelu sgydol)',
'protect-cantedit'            => "Ni allwch newid lefel diogelu'r dudalen hon, am nad yw'r gallu i olygu'r dudalen ganddoch.",
'restriction-type'            => 'Cyfyngiad:',
'restriction-level'           => 'Lefel cyfyngu:',
'minimum-size'                => 'Maint lleiaf',
'maximum-size'                => 'Maint mwyaf:',
'pagesize'                    => '(beit)',

# Restrictions (nouns)
'restriction-edit'   => 'Golygu',
'restriction-move'   => 'Symud',
'restriction-create' => 'Gosod',

# Restriction levels
'restriction-level-sysop'         => 'llwyr diogelwyd',
'restriction-level-autoconfirmed' => 'lled-ddiogelwyd',
'restriction-level-all'           => 'pob lefel',

# Undelete
'undelete'                     => 'Gweld tudalennau dilëedig',
'undeletepage'                 => 'Gweld ac adfer tudalennau dilëedig',
'undeletepagetitle'            => "'''Dyma'r diwygiadau dilëedig o [[:$1|$1]]'''.",
'viewdeletedpage'              => "Gweld tudalennau sydd wedi'u dileu",
'undeletepagetext'             => "Mae'r tudalennau isod wedi cael eu dileu ond mae cofnod ohonynt o hyd yn yr archif, felly mae'n bosibl eu hadfer. 
Gall yr archif gael ei glanhau o dro i dro.",
'undelete-fieldset-title'      => 'Dewis ac adfer diwygiadau',
'undeleteextrahelp'            => "I adfer y dudalen gyfan, gadewch pob blwch ticio'n wag a phwyswch y botwm '''''Adfer'''''. I adfer rhai diwygiadau'n unig, ticiwch y blychau ar gyfer y diwygiadau dewisedig, a phwyswch ar '''''Adfer'''''. Os y pwyswch ar '''''Ailosod''''' bydd y blwch sylwadau a phob blwch ticio yn gwacáu.",
'undeleterevisions'            => 'Gosodwyd $1 {{PLURAL:$1|fersiwn|fersiwn|fersiwn|fersiwn|fersiwn|fersiwn}} yn yr archif',
'undeletehistory'              => "Os adferwch y dudalen, fe fydd yr hanes gyfan yn cael ei atgyfodi hefyd. 
Os oes tudalen newydd o'r un enw wedi cael ei chreu ers y dilëad, fe ddangosir y fersiynau cynt yn yr hanes, heb ddisodli'r dudalen bresennol.
Sylwer hefyd fod cyfyngiadau ar ddiwygiadau o'r ffeil yn cael eu colli wrth eu hadfer",
'undeleterevdel'               => "Ni fydd yr adfer yn cael ei chyflawni pe byddai peth o'r diwygiad blaen i'r dudalen neu'r ffeil yn cael ei dileu oherwydd yr adfer.
Os hynny, rhaid i chi dad-ticio neu datguddio'r diwygiad dilëedig diweddaraf.",
'undeletehistorynoadmin'       => "Mae'r dudalen hon wedi'i dileu. Dangosir y rheswm am y dileu isod, gyda manylion o'r holl ddefnyddwyr sydd wedi golygu'r dudalen cyn y dileu. Dim ond gweinyddwyr sydd yn gallu gweld testun y diwygiadau i'r dudalen.",
'undelete-revision'            => 'Testun y golygiad gan $3 o $1 (fel ag yr oedd am $2), a ddilëwyd:',
'undeleterevision-missing'     => "Y diwygiad yn annilys neu yn eisiau.
Mae'n bosib bod nam ar y cyswllt, neu fod y diwygiad eisoes wedi ei adfer neu wedi ei ddileu o'r archif.",
'undelete-nodiff'              => 'Ni chafwyd hyd i olygiad cynharach.',
'undeletebtn'                  => 'Adfer!',
'undeletelink'                 => 'adfer',
'undeletereset'                => 'Ailosod',
'undeletecomment'              => 'Sylwadau:',
'undeletedarticle'             => 'wedi adfer "[[$1]]"',
'undeletedrevisions'           => 'wedi adfer $1 {{PLURAL:$1|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}}',
'undeletedrevisions-files'     => 'Adferwyd $1 {{PLURAL:$1|fersiwn|fersiwn|fersiwn|fersiwn|fersiwn|fersiwn}} a $2 {{PLURAL:$2|ffeil|ffeil|ffeil|ffeil|ffeil|ffeil}}',
'undeletedfiles'               => 'Adferwyd $1 {{PLURAL:$1|ffeil|ffeil|ffeil|ffeil|ffeil|ffeil}}',
'cannotundelete'               => "Wedi methu dad-ddileu;
gall rhywyn arall fod wedi dad-ddileu'r dudalen yn barod.",
'undeletedpage'                => "<big>'''Adferwyd $1'''</big>

Ceir cofnod o'r tudalennau a ddilëwyd neu a adferwyd yn ddiweddar ar y [[Special:Log/delete|lòg dileuon]].",
'undelete-header'              => "Ewch i'r [[Special:Log/delete|lòg dileuon]] i weld tudalennau a ddilëwyd yn ddiweddar.",
'undelete-search-box'          => "Chwilio'r tudalennau a ddilëwyd",
'undelete-search-prefix'       => 'Dangos tudalennau gan ddechrau gyda:',
'undelete-search-submit'       => 'Chwilio',
'undelete-no-results'          => 'Ni chafwyd hyd i dudalennau cyfatebol yn archif y dileuon.',
'undelete-filename-mismatch'   => "Ni ellir adfer y golgiad ffeil â'r stamp amser $1: nid oedd enw'r ffeil yn cysefeillio",
'undelete-bad-store-key'       => "Ni ellir adfer y golgiad ffeil â'r stamp amser $1: roedd y ffeil yn eisiau cyn y dileuad.",
'undelete-cleanup-error'       => 'Cafwyd gwall wrth ddileu\'r ffeil archif na ddefnyddiwyd "$1".',
'undelete-missing-filearchive' => "Ni ellid adfer archif y ffeil â'r ID $1 oherwydd nad yw yn y databas.
Efallai ei bod eisoes wedi ei hadfer.",
'undelete-error-short'         => 'Gwall wrth adfer y ffeil: $1',
'undelete-error-long'          => 'Cafwyd gwallau wrth adfer y ffeil:

$1',

# Namespace form on various pages
'namespace'      => 'Parth:',
'invert'         => 'Dewiswch pob parth heblaw am hwn',
'blanknamespace' => '(Prif)',

# Contributions
'contributions' => "Cyfraniadau'r defnyddiwr",
'mycontris'     => 'Fy nghyfraniadau',
'contribsub2'   => 'Dros $1 ($2)',
'nocontribs'    => "Heb ddod o hyd i newidiadau gyda'r maen prawf hwn.",
'uctop'         => '(cyfredol)',
'month'         => 'Cyfraniadau hyd at fis:',
'year'          => 'Cyfraniadau hyd at y flwyddyn:',

'sp-contributions-newbies'     => 'Dangos cyfraniadau gan gyfrifon newydd yn unig',
'sp-contributions-newbies-sub' => 'Ar gyfer cyfrifon newydd',
'sp-contributions-blocklog'    => 'Lòg blocio',
'sp-contributions-search'      => 'Chwilio am gyfraniadau',
'sp-contributions-username'    => 'Cyfeiriad IP neu enw defnyddiwr:',
'sp-contributions-submit'      => 'Chwilier',

# What links here
'whatlinkshere'            => "Beth sy'n cysylltu yma",
'whatlinkshere-title'      => 'Tudalennau sy\'n cysylltu â "$1"',
'whatlinkshere-page'       => 'Tudalen:',
'linklistsub'              => '(Rhestr cysylltiadau)',
'linkshere'                => "Mae'r tudalennau isod yn cysylltu â '''[[:$1]]''':",
'nolinkshere'              => "Nid oes cyswllt ar unrhyw dudalen arall yn arwain at '''[[:$1]]'''.",
'nolinkshere-ns'           => "Nid oes cyswllt ar unrhyw dudalen yn y parth dewisedig yn arwain at '''[[:$1]]'''.",
'isredirect'               => 'tudalen ail-gyfeirio',
'istemplate'               => 'cynhwysiad',
'whatlinkshere-prev'       => '{{PLURAL:$1|cynt|cynt|$1 cynt|$1 cynt|$1 cynt|$1 cynt}}',
'whatlinkshere-next'       => '{{PLURAL:$1|nesaf|nesaf|$1 nesaf|$1 nesaf|$1 nesaf|$1 nesaf}}',
'whatlinkshere-links'      => '← cysylltiadau',
'whatlinkshere-hideredirs' => '$1 ail-gyfeiriadau',
'whatlinkshere-hidelinks'  => '$1 cysylltau',
'whatlinkshere-hideimages' => '$1 cysylltau delweddau',
'whatlinkshere-filters'    => 'Hidlau',

# Block/unblock
'blockip'                     => "Blocio'r defnyddiwr",
'blockiptext'                 => "Defnyddiwch y ffurflen isod i flocio cyfeiriad IP neu ddefnyddiwr rhag ysgrifennu i'r databas. Dylech chi ddim ond gwneud hyn er mwyn rhwystro fandaliaeth a chan ddilyn [[{{MediaWiki:Policy-url}}|polisi'r wici]]. Llenwch y rheswm am y bloc yn y blwch isod -- dywedwch pa dudalen sydd wedi cael ei fandaleiddio.",
'ipaddress'                   => 'Cyfeiriad IP:',
'ipadressorusername'          => 'Cyfeiriad IP neu enw defnyddiwr:',
'ipbexpiry'                   => 'Am gyfnod o:',
'ipbreason'                   => 'Rheswm:',
'ipbreasonotherlist'          => 'Rheswm arall',
'ipbreason-dropdown'          => '*Rhesymau cyffredin dros flocio
** Gosod gwybodaeth anghywir
** Dileu cynnwys tudalennau
** Gosod cysylltiadau spam i wefannau eraill
** Gosod dwli ar dudalennau
** Ymddwyn yn fygythiol/tarfu
** Camddefnyddio cyfrifon niferus
** Enw defnyddiwr annerbyniol',
'ipbanononly'                 => 'Blocio defnyddwyr anhysbys yn unig',
'ipbcreateaccount'            => 'Atal y gallu i greu cyfrif',
'ipbemailban'                 => 'Atal y defnyddiwr rhag anfon e-bost',
'ipbenableautoblock'          => "Blocio'n awtomatig y cyfeiriad IP diwethaf y defnyddiodd y defnyddiwr hwn, ac unrhyw gyfeiriad IP arall y bydd yn ceisio defnyddio i olygu ohono.",
'ipbsubmit'                   => 'Blociwch y defnyddiwr hwn',
'ipbother'                    => 'Cyfnod arall:',
'ipboptions'                  => '2 awr:2 hours,ddiwrnod:1 day,3 niwrnod:3 days,wythnos:1 week,bythefnos:2 weeks,fis:1 month,3 mis:3 months,6 mis:6 months,flwyddyn:1 year,5 mlynedd:5 years,amhenodol:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => 'arall',
'ipbotherreason'              => 'Rheswm arall:',
'ipbhidename'                 => "Cuddio'r enw defnyddiwr o'r lòg blocio, rhestr y blociau cyfredol a'r rhestr defnyddwyr",
'badipaddress'                => 'Cyfeiriad IP annilys.',
'blockipsuccesssub'           => 'Y blocio wedi llwyddo',
'blockipsuccesstext'          => 'Mae cyfeiriad IP [[Special:Contributions/$1|$1]] wedi cael ei flocio.
<br />Gwelwch [[Special:IPBlockList|restr y blociau IP]] er mwyn arolygu blociau.',
'ipb-edit-dropdown'           => "Golygu'r rhesymau dros flocio",
'ipb-unblock-addr'            => 'Datflocio $1',
'ipb-unblock'                 => 'Datflocio enw defnyddiwr neu cyfeiriad IP',
'ipb-blocklist-addr'          => 'Gweld y blociau cyfredol ar gyfer $1',
'ipb-blocklist'               => 'Dangos y blociau cyfredol',
'unblockip'                   => 'Dadflocio defnyddiwr',
'unblockiptext'               => "Defnyddiwch y ffurflen isod i ail-alluogi golygiadau gan ddefnyddiwr neu o gyfeiriad IP a fu gynt wedi'i flocio.",
'ipusubmit'                   => 'Datflociwch y cyfeiriad hwn',
'unblocked'                   => 'Mae [[User:$1|$1]] wedi cael ei ddad-flocio',
'unblocked-id'                => 'Tynnwyd y bloc $1',
'ipblocklist'                 => "Cyfeiriadau IP ac enwau defnyddwyr sydd wedi'u blocio",
'ipblocklist-legend'          => 'Dod o hyd i ddefnyddiwr sydd wedi ei blocio',
'ipblocklist-username'        => "Enw'r defnyddiwr neu ei gyfeiriad IP:",
'ipblocklist-submit'          => 'Chwilier',
'blocklistline'               => '$1, $2 wedi blocio $3 ($4)',
'infiniteblock'               => 'bloc parhaus',
'expiringblock'               => 'yn dod i ben $1',
'anononlyblock'               => 'ataliwyd dim ond pan nad yw wedi mewngofnodi',
'noautoblockblock'            => 'analluogwyd blocio awtomatig',
'createaccountblock'          => 'ataliwyd y gallu i greu cyfrif',
'emailblock'                  => 'rhwystrwyd e-bostio',
'ipblocklist-empty'           => "Mae'r rhestr blociau'n wag.",
'ipblocklist-no-results'      => 'Nid yw cyfeiriad IP neu enw defnyddiwr yr ymholiad wedi ei flocio.',
'blocklink'                   => 'bloc',
'unblocklink'                 => 'dadflocio',
'contribslink'                => 'cyfraniadau',
'autoblocker'                 => 'Rydych chi wedi cael eich blocio yn awtomatig gan eich bod chi\'n rhannu cyfeiriad IP gyda "[[User:$1|$1]]". Dyma\'r rheswm a roddwyd dros flocio $1: "$2".',
'blocklogpage'                => 'Lòg blociau',
'blocklogentry'               => 'wedi blocio "[[$1]]" am gyfnod o $2 $3',
'blocklogtext'                => "Dyma lòg o'r holl weithredoedd blocio a datflocio. Nid yw'r cyfeiriadau IP sydd wedi cael eu blocio'n awtomatig ar y rhestr. Gweler [[Special:IPBlockList|rhestr y blociau IP]] am restr y blociau a'r gwaharddiadau sydd yn weithredol ar hyn o bryd.",
'unblocklogentry'             => 'wedi dadflocio $1',
'block-log-flags-anononly'    => 'defnyddwyr anhysbys yn unig',
'block-log-flags-nocreate'    => 'analluogwyd creu cyfrif',
'block-log-flags-noautoblock' => 'analluogwyd blocio awtomatig',
'block-log-flags-noemail'     => 'analluogwyd e-bostio',
'range_block_disabled'        => 'Ar hyn o bryd nid yw gweinyddwyr yn gallu blocio ystod o gyfeiriadau IP.',
'ipb_expiry_invalid'          => 'Amser terfynu yn annilys.',
'ipb_already_blocked'         => 'Mae "$1" eisoes wedi ei flocio',
'ipb_cant_unblock'            => "Gwall: Ni chafwyd hyd i'r bloc a'r ID $1.
Hwyrach ei fod wedi ei ddad-flocio'n barod.",
'ipb_blocked_as_range'        => "Gwall: Nid yw'r IP $1 wedi ei blocio'n uniongyrchol ac felly ni ellir ei datflocio. Wedi dweud hynny, y mae'n rhan o'r amrediad $2 sydd wedi ei blocio; gellir datflocio'r amrediad.",
'ip_range_invalid'            => 'Dewis IP annilys.',
'proxyblocker'                => 'Dirprwy-flociwr',
'proxyblocker-disabled'       => 'Analluogwyd y swyddogaeth hon.',
'proxyblockreason'            => "Mae eich cyfeiriad IP wedi'i flocio gan ei fod yn ddirprwy agored (open proxy). Cysylltwch â'ch gweinyddwr rhyngrwyd neu gymorth technegol er mwyn eu hysbysu am y broblem ddifrifol yma.",
'proxyblocksuccess'           => 'Wedi llwyddo.',
'sorbsreason'                 => 'Mae eich cyfeiriad IP wedi cael ei osod ymhlith y dirprwyon agored ar y Rhestr DNS Gwaharddedig a ddefnyddir gan {{SITENAME}}.',
'sorbs_create_account_reason' => 'Mae eich cyfeiriad IP wedi cael ei osod ymhlith y dirprwyon agored ar y Rhestr DNS Gwaharddedig a ddefnyddir gan {{SITENAME}}.
Ni allwch greu cyfrif.',

# Developer tools
'lockdb'              => "Cloi'r databas",
'unlockdb'            => "Datgloi'r databas",
'lockdbtext'          => "Bydd cloi'r databas yn golygu na all unrhyw ddefnyddiwr olygu tudalennau, newid eu dewisiadau, golygu eu rhestrau gwylio, na gwneud unrhywbeth arall sydd angen newid yn y databas. Cadarnhewch eich bod chi wir am wneud hyn, ac y byddwch yn ei ddatgloi unwaith mae'r gwaith cynnal a chadw ar ben.",
'unlockdbtext'        => "Bydd datgloi'r databas yn ail-alluogi unrhyw ddefnyddiwr i olygu tudalennau, newid eu dewisiadau, golygu eu rhestrau gwylio, neu i wneud unrhywbeth arall sydd angen newid yn y databas. Cadarnhewch eich bod chi wir am wneud hyn.",
'lockconfirm'         => "Ydw, rydw i wir am gloi'r databas.",
'unlockconfirm'       => "Ydw, rydw i wir am ddatgloi'r databas.",
'lockbtn'             => "Cloi'r databas",
'unlockbtn'           => "Datgloi'r databas",
'locknoconfirm'       => "Nid ydych wedi ticio'r blwch cadarnhad.",
'lockdbsuccesssub'    => "Wedi llwyddo cloi'r databas",
'unlockdbsuccesssub'  => "Databas wedi'i ddatgloi",
'lockdbsuccesstext'   => "Mae'r databas wedi'i gloi.<br />
Cofiwch [[Special:UnlockDB|ddatgloi'r]] databas pan fydd y gwaith cynnal ar ben.",
'unlockdbsuccesstext' => "Mae'r databas wedi'i ddatgloi.",
'databasenotlocked'   => "Nid yw'r databas ar glo.",

# Move page
'move-page'               => 'Symud $1',
'move-page-legend'        => 'Symud tudalen',
'movepagetext'            => "Wrth ddefnyddio'r ffurflen isod byddwch yn ail-enwi tudalen, gan symud ei hanes gyfan i'r enw newydd.
Bydd yr hen deitl yn troi'n dudalen ail-gyfeirio i'r teitl newydd.
Gallwch ddewis bod y meddalwedd yn cywiro tudalennau ailgyfeirio oedd yn arwain at yr hen deitl yn awtomatig.
Os nad ydych yn dewis hyn, yna byddwch gystal â thrwsio [[Special:DoubleRedirects|ail-gyfeiriadau dwbl]] ac [[Special:BrokenRedirects|ail-gyfeiriadau tor]] eich hunan.
Eich cyfrifoldeb chi yw sicrhau bod cysylltiadau wici'n dal i arwain at y man iawn!

Sylwch '''na fydd''' y dudalen yn symud os oes yna dudalen o'r enw newydd yn bodoli'n barod ar y databas (heblaw ei bod hi'n wag neu'n ail-gyfeiriad heb unrhyw hanes golygu).
Felly, os y gwnewch gamgymeriad wrth ail-enwi tudalen dylai fod yn bosibl ei hail-enwi eto ar unwaith wrth yr enw gwreiddiol.
Hefyd, mae'n amhosibl ysgrifennu dros ben tudalen sydd yn bodoli'n barod.

'''DALIER SYLW!'''
Gall hwn fod yn newid sydyn a llym i dudalen boblogaidd;
gnewch yn siwr eich bod chi'n deall y canlyniadau cyn mynd ati.",
'movepagetalktext'        => "Bydd y dudalen sgwrs yn symud gyda'r dudalen hon '''onibai:'''
*bod tudalen sgwrs wrth yr enw newydd yn bodoli'n barod
*bod y blwch isod heb ei farcio.

Os felly, gallwch symud y dudalen sgwrs neu ei gyfuno ar ôl symud y dudalen ei hun.",
'movearticle'             => 'Symud y dudalen:',
'movenotallowed'          => 'Nid oes caniatâd gennych i symud tudalennau.',
'newtitle'                => "I'r teitl newydd:",
'move-watch'              => 'Gwylier y dudalen hon',
'movepagebtn'             => 'Symud tudalen',
'pagemovedsub'            => 'Y symud wedi llwyddo',
'movepage-moved'          => '<big>\'\'\'Symudwyd y dudalen "$1" i "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => "Mae tudalen gyda'r darpar enw yn bodoli'n barod, neu mae eich darpar enw yn annilys.
Dewiswch enw arall os gwelwch yn dda.",
'talkexists'              => "'''Mae'r dudalen wedi'i symud yn llwyddiannus, ond nid oedd hi'n bosibl symud y dudalen sgwrs oherwydd bod yna dudalen sgwrs gyda'r enw newydd yn bodoli'n barod. Cyfunwch y ddwy dudalen, os gwelwch yn dda.'''",
'movedto'                 => 'symud i',
'movetalk'                => 'Symud y dudalen sgwrs hefyd',
'movepage-page-exists'    => "Mae'r dudalen $1 eisoes ar gael ac ni ellir ysgrifennu drosto yn awtomatig.",
'movepage-page-moved'     => 'Symudwyd y dudalen $1 i $2.',
'movepage-page-unmoved'   => 'Ni ellid symud y dudalen $1 i $2.',
'movepage-max-pages'      => 'Symudwyd yr uchafswm o $1 {{PLURAL:$1|tudalen|dudalen|dudalen|tudalen|thudalen|tudalen}} y gellir eu symud yn awtomatig.',
'1movedto2'               => 'Wedi symud [[$1]] i [[$2]]',
'1movedto2_redir'         => 'Wedi symud [[$1]] i [[$2]] trwy ailgyfeiriad.',
'movelogpage'             => 'Lòg symud tudalennau',
'movelogpagetext'         => "Isod mae rhestr y tudalennau sydd wedi'u symud",
'movereason'              => 'Rheswm:',
'revertmove'              => 'symud nôl',
'delete_and_move'         => 'Dileu a symud',
'delete_and_move_text'    => "==Angen dileu==

Mae'r erthygl \"[[:\$1]]\" yn bodoli'n barod. Ydych chi am ddileu'r erthygl er mwyn cwblhau'r symudiad?",
'delete_and_move_confirm' => "Ie, dileu'r dudalen",
'delete_and_move_reason'  => "Wedi'i dileu er mwyn symud tudalen arall yn ei lle.",
'selfmove'                => "Mae'r teitlau hen a newydd yn union yr un peth;
nid yw'n bosib cyflawnu'r symud.",
'immobile_namespace'      => "Mae teitl y dudalen gwreiddiol neu'r cyrchfan yn arbennig; ni ellir symud tudalennau i'r parth hwnnw nag oddi wrtho.",

# Export
'export'            => 'Allforio tudalennau',
'exporttext'        => "Gallwch allforio testun a hanes golygu tudalen penodol neu set o dudalennau wedi'u lapio mewn côd XML. Gall hwn wedyn gael ei fewnforio i wici arall sy'n defnyddio meddalwedd MediaWiki, trwy ddefnyddio'r [[Special:Import|dudalen mewnforio]].

I allforio tudalennau, teipiwch y teitlau yn y bocs testun isod, bobi linell i'r teitlau; a dewis p'un ai ydych chi eisiau'r diwygiad presennol a'r holl fersiynnau blaenorol, gyda hanes y dudalen; ynteu a ydych am y diwygiad presennol a'r wybodaeth am y golygiad diweddaraf yn unig.

Yn achos yr ail ddewis, mae modd defnyddio cyswllt, e.e. [[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]] ar gyfer y dudalen \"[[{{MediaWiki:Mainpage}}]]\".",
'exportcuronly'     => 'Cynnwys y diwygiad diweddaraf yn unig, nid yr hanes llawn',
'exportnohistory'   => "----
'''Sylwer:''' er mwyn peidio â gor-lwytho'r gweinydd, analluogwyd allforio hanes llawn y tudalennau.",
'export-submit'     => 'Allforier',
'export-addcattext' => "Ychwanegu tudalennau i'w hallforio o'r categori:",
'export-addcat'     => 'Ychwaneger',
'export-download'   => 'Cynnig rhoi ar gadw ar ffurf ffeil',
'export-templates'  => 'Cynnwys nodiadau',

# Namespace 8 related
'allmessages'               => 'Pob neges',
'allmessagesname'           => 'Enw',
'allmessagesdefault'        => 'Testun rhagosodedig',
'allmessagescurrent'        => 'Testun cyfredol',
'allmessagestext'           => "Dyma restr o'r holl negeseuon yn y parth MediaWici.
Os ydych am gyfrannu at y gwaith o gyfieithu ar gyfer holl prosiectau Mediawiki ar y cyd, mae croeso i chi ymweld â [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] a [http://translatewiki.net Betawiki].",
'allmessagesnotsupportedDB' => "Nid yw '''{{ns:special}}:PobNeges''' yn cael ei gynnal gan fod '''\$wgUseDatabaseMessages''' wedi ei ddiffodd.",
'allmessagesfilter'         => 'Hidl enw neges:',
'allmessagesmodified'       => 'Dangos y rhai a ddiwygiwyd yn unig',

# Thumbnails
'thumbnail-more'           => 'Chwyddo',
'filemissing'              => 'Ffeil yn eisiau',
'thumbnail_error'          => "Cafwyd gwall wrth greu'r mân-lun: $1",
'djvu_page_error'          => 'Y dudalen DjVu allan o amrediad',
'djvu_no_xml'              => 'Ddim yn gallu mofyn XML ar gyfer ffeil DjVu',
'thumbnail_invalid_params' => 'Paramedrau maint mân-lun annilys',
'thumbnail_dest_directory' => "Methwyd â chreu'r cyfeiriadur cyrchfan",

# Special:Import
'import'                  => 'Mewnforio tudalennau',
'importinterwiki'         => 'Mewnforiad traws-wici',
'import-interwiki-submit' => 'Mewnforio',
'importtext'              => "Os gwelwch yn dda, allforiwch y ffeil o'r wici gwreiddiol gan ddefnyddio'r nodwedd <b>Special:Export</b>, cadwch hi i'ch disg, ac uwchlwythwch hi fan hyn.",
'import-revision-count'   => '$1 {{PLURAL:$1|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}}',
'importfailed'            => 'Mewnforio wedi methu: $1',
'importbadinterwiki'      => 'Cyswllt rhyngwici gwallus',
'importnotext'            => 'Gwag, neu heb destun',
'importsuccess'           => 'Mewnforio wedi llwyddo!',
'importhistoryconflict'   => "Mae gwrthdaro rhwng adolygiadau hanes (efallai eich bod chi wedi mewnforio'r dudalen o'r blaen)",
'importnosources'         => "Ni ddiffiniwyd unrhyw ffynonellau mewnforio traws-wici, ac mae uwchlwytho hanesion yn uniongyrchol wedi'i analluogi.",
'importnofile'            => 'Ni uwchlwythwyd unrhyw ffeil mewnforio.',
'xml-error-string'        => '$1 ar linell $2, col $3 (beit $4): $5',

# Import log
'importlogpage'                    => 'Lòg mewnforio',
'import-logentry-upload-detail'    => '$1 {{PLURAL:$1|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}}',
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|diwygiad|diwygiad|ddiwygiad|diwygiad|diwygiad|diwygiad}} o $2',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Fy nhudalen defnyddiwr',
'tooltip-pt-anonuserpage'         => 'Y tudalen defnyddiwr ar gyfer y cyfeiriad IP yr ydych yn ei ddefnyddio wrth olygu',
'tooltip-pt-mytalk'               => 'Fy nhudalen sgwrs',
'tooltip-pt-anontalk'             => "Sgwrs ynglŷn â golygiadau o'r cyfeiriad IP hwn",
'tooltip-pt-preferences'          => 'Fy newisiadau',
'tooltip-pt-watchlist'            => 'Rydych yn dilyn hynt y tudalennau sydd ar y rhestr hon',
'tooltip-pt-mycontris'            => 'Fy nghyfraniadau yn nhrefn amser',
'tooltip-pt-login'                => "Fe'ch anogir i fewngofnodi, er nad oes rhaid gwneud.",
'tooltip-pt-anonlogin'            => "Fe'ch anogir i fewngofnodi, er nad oes rhaid gwneud.",
'tooltip-pt-logout'               => 'Allgofnodi',
'tooltip-ca-talk'                 => 'Sgwrsio am y dudalen',
'tooltip-ca-edit'                 => "Gallwch olygu'r dudalen hon. Da o beth fyddai defnyddio'r botwm 'Dangos rhagolwg' cyn rhoi ar gadw.",
'tooltip-ca-addsection'           => "Ychwanegu sylw i'r drafodaeth",
'tooltip-ca-viewsource'           => "Mae'r dudalen hon wedi'i diogelu. Gallwch weld y côd yma.",
'tooltip-ca-history'              => "Fersiynau cynt o'r dudalen hon.",
'tooltip-ca-protect'              => "Diogelu'r dudalen hon",
'tooltip-ca-delete'               => "Dileu'r dudalen hon",
'tooltip-ca-undelete'             => "Adfer y golygiadau i'r dudalen hon a wnaethpwyd cyn ei dileu",
'tooltip-ca-move'                 => 'Symud y dudalen hon',
'tooltip-ca-watch'                => "Ychwanegu'r dudalen hon at eich rhestr gwylio",
'tooltip-ca-unwatch'              => "Tynnu'r dudalen oddi ar eich rhestr gwylio",
'tooltip-search'                  => 'Chwilio {{SITENAME}}',
'tooltip-search-go'               => "Mynd i'r dudalen â'r union deitl hwn, os oes un",
'tooltip-search-fulltext'         => 'Chwilio am y testun hwn',
'tooltip-p-logo'                  => 'Yr Hafan',
'tooltip-n-mainpage'              => "Ymweld â'r Hafan",
'tooltip-n-portal'                => "Pethau i'w gwneud, adnoddau a thudalennau'r gymuned",
'tooltip-n-currentevents'         => 'Gwybodaeth yn gysylltiedig â materion cyfoes',
'tooltip-n-recentchanges'         => 'Rhestr y newidiadau diweddar ar y wici.',
'tooltip-n-randompage'            => 'Dewiswch dudalen ar hap',
'tooltip-n-help'                  => 'Tudalennau cymorth',
'tooltip-t-whatlinkshere'         => "Rhestr o bob tudalen sy'n cysylltu â hon",
'tooltip-t-recentchangeslinked'   => 'Newidiadau diweddar i dudalennau sydd yn cysylltu â hon',
'tooltip-feed-rss'                => 'Porthiant RSS ar gyfer y dudalen hon',
'tooltip-feed-atom'               => 'Porthiant atom ar gyfer y dudalen hon',
'tooltip-t-contributions'         => "Gwelwch restr o gyfraniadau'r defnyddiwr hwn",
'tooltip-t-emailuser'             => 'Anfonwch e-bost at y defnyddiwr hwn',
'tooltip-t-upload'                => 'Uwchlwythwch ffeil delwedd, sain, fideo, ayb',
'tooltip-t-specialpages'          => "Rhestr o'r holl dudalennau arbennig",
'tooltip-t-print'                 => "Cynhyrchwch fersiwn o'r dudalen yn barod at ei hargraffu",
'tooltip-t-permalink'             => "Ail-lwytho'r dudalen fel bod modd gweld y cyfeiriad URL llawn a chreu cyswllt parhaol i'r fersiwn hwn o'r dudalen",
'tooltip-ca-nstab-main'           => 'Gweld tudalen yn y prif barth',
'tooltip-ca-nstab-user'           => 'Gweld tudalen y defnyddiwr',
'tooltip-ca-nstab-media'          => 'Gweld y dudalen gyfrwng',
'tooltip-ca-nstab-special'        => "Mae hwn yn dudalen arbennig; ni allwch olygu'r dudalen ei hun",
'tooltip-ca-nstab-project'        => 'Gweld tudalen y wici',
'tooltip-ca-nstab-image'          => 'Gweld tudalen y ffeil',
'tooltip-ca-nstab-mediawiki'      => 'Gweld neges y system',
'tooltip-ca-nstab-template'       => 'Dangos y nodyn',
'tooltip-ca-nstab-help'           => 'Gweld y dudalen gymorth',
'tooltip-ca-nstab-category'       => 'Dangos tudalen y categori',
'tooltip-minoredit'               => 'Marciwch hwn yn olygiad bychan.',
'tooltip-save'                    => 'Cadwch eich newidiadau',
'tooltip-preview'                 => "Dangos rhagolwg o'r newidiadau; defnyddiwch cyn cadw os gwelwch yn dda!",
'tooltip-diff'                    => "Dangos y newidiadau rydych chi wedi gwneud i'r testun.",
'tooltip-compareselectedversions' => 'Cymharwch y fersiynau detholedig.',
'tooltip-watch'                   => "Ychwanegu'r dudalen hon at eich rhestr gwylio",
'tooltip-recreate'                => "Ail-greu'r dudalen serch iddi gael ei dileu",
'tooltip-upload'                  => 'Dechrau uwchlwytho',

# Metadata
'nodublincore'      => "Mae metadata RDF 'Dublin Core' wedi cael ei analluogi ar y gwasanaethwr hwn.",
'nocreativecommons' => "Mae metadata RDF 'Creative Commons' wedi'i analluogi ar y gwasanaethwr hwn.",
'notacceptable'     => "Dydy gweinydd y wici ddim yn medru rhoi'r data mewn fformat darllenadwy i'ch cleient.",

# Attribution
'anonymous'        => 'Defnyddwyr anhysbys {{SITENAME}}',
'siteuser'         => 'Defnyddiwr {{SITENAME}} $1',
'lastmodifiedatby' => 'Newidiwyd y dudalen hon ddiwethaf $2, $1 gan $3', # $1 date, $2 time, $3 user
'othercontribs'    => 'Yn seiliedig ar waith gan $1.',
'others'           => 'eraill',
'siteusers'        => 'Defnyddwyr {{SITENAME}} $1',
'creditspage'      => "Cydnabyddiaethau'r dudalen",
'nocredits'        => "Does dim cydnabyddiaethau i'r dudalen hon.",

# Spam protection
'spamprotectiontitle' => 'Hidlydd amddiffyn rhag sbam',
'spamprotectiontext'  => 'Ataliwyd y dudalen rhag ei rhoi ar gadw gan yr hidlydd sbam, yn fwy na thebyg oherwydd bod cysylltiad allanol ar y dudalen.',
'spamprotectionmatch' => "Dyma'r testun gyneuodd ein hidlydd amddiffyn rhag sbam: $1",
'spambot_username'    => 'Teclyn clirio sbam MediaWiki',
'spam_reverting'      => "Yn troi nôl i'r diwygiad diweddaraf sydd ddim yn cynnwys cysylltiadau i $1",
'spam_blanking'       => 'Roedd cysylltiadau i $1 gan bob golygiad, yn blancio',

# Info page
'infosubtitle'   => 'Gwybodaeth am y dudalen',
'numedits'       => "Nifer y golygiadau (i'r dudalen): $1",
'numtalkedits'   => "Nifer y golygiadau (i'r dudalen sgwrs): $1",
'numwatchers'    => 'Nifer y gwylwyr: $1',
'numauthors'     => "Nifer yr awduron (o'r dudalen): $1",
'numtalkauthors' => "Nifer yr awduron (o'r dudalen sgwrs): $1",

# Math options
'mw_math_png'    => 'Arddangos symbolau mathemateg fel delwedd PNG bob amser',
'mw_math_simple' => 'HTML os yn syml iawn, PNG fel arall',
'mw_math_html'   => 'HTML os yn bosib, PNG fel arall',
'mw_math_source' => 'Gadewch fel côd TeX (ar gyfer porwyr testun)',
'mw_math_modern' => 'Argymelledig ar gyfer porwyr modern',
'mw_math_mathml' => 'MathML os yn bosib (arbrofol)',

# Patrolling
'rcpatroldisabled'                    => "Patrol y Newidiadau Diweddar wedi'i analluogi",
'rcpatroldisabledtext'                => 'Analluogwyd y nodwedd Patrol y Newidiadau Diweddar.',
'markedaspatrollederror-noautopatrol' => "Ni chaniateir i chi farcio'ch newidiadau eich hunan fel rhai derbyniol.",

# Patrol log
'patrol-log-page' => 'Lòg patrolio',
'patrol-log-line' => 'wedi marcio bod fersiwn $1 o $2 wedi derbyn ymweliad patrôl $3',
'patrol-log-auto' => '(awtomatig)',

# Image deletion
'deletedrevision'                 => 'Wedi dileu hen ddiwygiad $1.',
'filedeleteerror-short'           => "Gwall wrth ddileu'r ffeil: $1",
'filedeleteerror-long'            => "Cafwyd gwallau wrth ddileu'r ffeil:

$1",
'filedelete-missing'              => 'Ni ellir dileu\'r ffeil "$1" gan nad yw\'n bodoli.',
'filedelete-old-unregistered'     => 'Nid yw\'r diwygiad "$1" o\'r ffeil yn y databas.',
'filedelete-current-unregistered' => 'Nid yw\'r ffeil "$1" yn y databas.',
'filedelete-archive-read-only'    => 'Nid oes modd i\'r gweweinydd ysgrifennu ar y cyfeiriadur archif "$1".',

# Browsing diffs
'previousdiff' => '← Y fersiwn gynt',
'nextdiff'     => 'Y fersiwn dilynol →',

# Media information
'mediawarning'         => "'''Rhybudd''': Gallasai'r ffeil hon gynnwys côd maleisus; os ydyw mae'n bosib y bydd eich cyfrifiadur yn cael ei danseilio wrth lwytho'r ffeil.
<hr />",
'imagemaxsize'         => 'Tocio maint y delweddau ar y tudalennau disgrifiad i:',
'thumbsize'            => 'Maint mân-lun :',
'widthheightpage'      => '$1×$2, $3 {{PLURAL:$3|tudalen|dudalen|dudalen|tudalen|thudalen|tudalen}}',
'file-info'            => '(maint y ffeil: $1, ffurf MIME: $2)',
'file-info-size'       => '($1 × $2 picsel, maint y ffeil: $3, ffurf MIME: $4)',
'file-nohires'         => '<small>Wedi ei chwyddo hyd yr eithaf.</small>',
'svg-long-desc'        => '(Ffeil SVG, maint mewn enw $1 × $2 picsel, maint y ffeil: $3)',
'show-big-image'       => 'Maint llawn',
'show-big-image-thumb' => '<small>Maint y rhagolwg: $1 × $2 picsel</small>',

# Special:NewImages
'newimages'             => 'Oriel y ffeiliau newydd',
'imagelisttext'         => "Isod mae rhestr {{PLURAL:$1|gwag o ffeiliau|o '''$1''' ffeil|o '''$1''' ffeil wedi'u trefnu $2|o '''$1''' ffeil wedi'u trefnu $2|o '''$1''' o ffeiliau wedi'u trefnu $2|o '''$1''' o ffeiliau wedi'u trefnu $2|}}.",
'newimages-summary'     => "Mae'r dudalen arbennig hon yn dangos y ffeiliau a uwchlwythwyd yn ddiweddar.",
'showhidebots'          => '($1 botiau)',
'noimages'              => "Does dim byd i'w weld.",
'ilsubmit'              => 'Chwilio',
'bydate'                => 'yn ôl dyddiad',
'sp-newimages-showfrom' => "Dangos ffeiliau sy'n newydd ers: $2, $1",

# Bad image list
'bad_image_list' => "Dyma'r fformat:

Dim ond eitemau mewn rhestr (llinellau'n dechrau a *) gaiff eu hystyried.
Rhaid i'r cyswllt cyntaf ar linell fod yn gyswllt at ffeil wallus.
Mae unrhyw gysylltau eraill ar yr un llinell yn cael eu trin fel eithriadau, h.y. tudalennau lle mae'r ffeil yn gallu ymddangos.",

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "Mae'r ffeil hon yn cynnwys gwybodaeth ychwanegol, sydd mwy na thebyg wedi dod o'r camera digidol neu'r sganiwr a ddefnyddiwyd i greu'r ffeil neu ei digido. Os yw'r ffeil wedi ei cael ei newid ers ei chreu efallai nad yw'r manylion hyn yn dal i fod yn gywir.",
'metadata-expand'   => 'Dangos manylion estynedig',
'metadata-collapse' => 'Cuddio manylion estynedig',
'metadata-fields'   => "Pan fod tabl y metadata wedi'i grebachu fe ddangosir y meysydd metadata EXIF a restrir yn y neges hwn ar dudalen wybodaeth y ddelwedd.
Cuddir y meysydd eraill trwy ragosodiad.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength", # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => 'Lled',
'exif-imagelength'                 => 'Uchder',
'exif-bitspersample'               => 'Nifer y didau i bob cydran',
'exif-compression'                 => 'Cynllun cywasgu',
'exif-photometricinterpretation'   => 'Cyfansoddiad picseli',
'exif-orientation'                 => 'Gogwydd',
'exif-samplesperpixel'             => 'Nifer y cydrannau',
'exif-planarconfiguration'         => 'Trefn y data',
'exif-ycbcrsubsampling'            => 'Cymhareb is-samplo Y i C',
'exif-ycbcrpositioning'            => 'Lleoli Y a C',
'exif-xresolution'                 => 'Datrysiad llorweddol',
'exif-yresolution'                 => 'Datrysiad fertigol',
'exif-resolutionunit'              => 'Datrysiad uned X a Y',
'exif-stripoffsets'                => "Lleoliad data'r ddelwedd",
'exif-rowsperstrip'                => 'Nifer y rhesi i bob stribed',
'exif-stripbytecounts'             => 'Nifer y beitiau i bob stribed cywasgedig',
'exif-jpeginterchangeformat'       => 'Yr atred i JPEG SOI',
'exif-jpeginterchangeformatlength' => "Nifer beitiau'r data JPEG",
'exif-transferfunction'            => 'Ffwythiant trosglwyddo',
'exif-whitepoint'                  => 'Cromatigedd y cyfeirbwynt gwyn',
'exif-primarychromaticities'       => 'Cromatigedd y lliwiau cysefin',
'exif-datetime'                    => "Dyddiad ac amser y newid i'r ffeil",
'exif-imagedescription'            => 'Teitl y ddelwedd',
'exif-make'                        => 'Gwneuthurwr y camera',
'exif-model'                       => 'Model y camera',
'exif-software'                    => 'Meddalwedd a ddefnyddir',
'exif-artist'                      => 'Awdur',
'exif-copyright'                   => 'Deiliad yr hawlfraint',
'exif-exifversion'                 => 'Fersiwn Exif',
'exif-colorspace'                  => 'Gofod lliw',
'exif-componentsconfiguration'     => 'Ystyr pob cydran',
'exif-compressedbitsperpixel'      => 'Modd cywasgu delwedd',
'exif-pixelydimension'             => 'Lled delwedd dilys',
'exif-pixelxdimension'             => 'Uchder delwedd dilys',
'exif-makernote'                   => "Nodiadau'r gwneuthurwr",
'exif-usercomment'                 => "Sylwadau'r defnyddiwr",
'exif-relatedsoundfile'            => 'Ffeil sain cysylltiedig',
'exif-datetimeoriginal'            => 'Dyddiad ac amser y cynhyrchwyd y data',
'exif-datetimedigitized'           => 'Dyddiad ac amser y digiteiddiwyd',
'exif-subsectimeoriginal'          => 'Iseiliadau DyddiadAmserGwreiddiol',
'exif-subsectimedigitized'         => 'Iseiliadau DyddiadAmserDigidol',
'exif-exposuretime'                => 'Amser dinoethi',
'exif-exposuretime-format'         => '$1 eiliad ($2)',
'exif-fnumber'                     => 'Cymhareb yr agorfa (rhif F)',
'exif-exposureprogram'             => 'Rhaglen Dinoethi',
'exif-spectralsensitivity'         => 'Sensitifedd sbectrol',
'exif-isospeedratings'             => 'Cyfraddiad cyflymder ISO',
'exif-shutterspeedvalue'           => 'Cyflymder y caead',
'exif-aperturevalue'               => 'Agorfa',
'exif-brightnessvalue'             => 'Disgleirdeb',
'exif-exposurebiasvalue'           => 'Bias dinoethi',
'exif-maxaperturevalue'            => "Maint mwyaf agorfa'r glan",
'exif-subjectdistance'             => 'Pellter y goddrych',
'exif-meteringmode'                => 'Modd mesur goleuni',
'exif-lightsource'                 => 'Ffynhonell goleuni',
'exif-flash'                       => 'Fflach',
'exif-focallength'                 => 'Hyd ffocal y lens',
'exif-subjectarea'                 => 'Maint a lleoliad y goddrych',
'exif-flashenergy'                 => "Ynni'r fflach",
'exif-subjectlocation'             => 'Lleoliad y goddrych',
'exif-exposureindex'               => 'Indecs dinoethiad',
'exif-sensingmethod'               => 'Dull synhwyro',
'exif-filesource'                  => 'Ffynhonnell y ffeil',
'exif-scenetype'                   => 'Math o olygfa',
'exif-cfapattern'                  => 'Patrwm CFA',
'exif-exposuremode'                => 'Modd dinoethi',
'exif-whitebalance'                => 'Cydbwysedd Gwyn',
'exif-digitalzoomratio'            => 'Cymhareb closio digidol',
'exif-contrast'                    => 'Cyferbyniad',
'exif-saturation'                  => 'Dirlawnder',
'exif-sharpness'                   => 'Eglurder',
'exif-imageuniqueid'               => 'ID unigryw y ddelwedd',
'exif-gpslatituderef'              => "Lledred i'r Gogledd neu i'r De",
'exif-gpslatitude'                 => 'Lledred',
'exif-gpslongituderef'             => "Hydred i'r Dwyrain neu i'r Gorllewin",
'exif-gpslongitude'                => 'Hydred',
'exif-gpsaltituderef'              => 'Cyfeirnod uchder',
'exif-gpsaltitude'                 => 'Uchder',
'exif-gpsmeasuremode'              => 'Modd mesur',
'exif-gpsdop'                      => 'Manylder mesur',
'exif-gpsdestdistance'             => 'Pellter i ben y daith',
'exif-gpsdatestamp'                => 'Dyddiad GPS',

'exif-unknowndate' => 'Dyddiad anhysbys',

'exif-orientation-3' => 'Wedi ei droi 180°', # 0th row: bottom; 0th column: right

'exif-componentsconfiguration-0' => "ddim i'w gael",

'exif-exposureprogram-0' => 'Heb ei gosod',

'exif-subjectdistance-value' => '$1 medr',

'exif-meteringmode-0'   => 'Anhysbys',
'exif-meteringmode-1'   => 'Cyfartaleddu',
'exif-meteringmode-2'   => 'Cyfartaleddu canol-bwysedig',
'exif-meteringmode-3'   => 'Smotyn',
'exif-meteringmode-4'   => 'Smotiau',
'exif-meteringmode-5'   => 'Patrymu',
'exif-meteringmode-6'   => 'Rhannol',
'exif-meteringmode-255' => 'Arall',

'exif-lightsource-0'  => 'Anhysbys',
'exif-lightsource-1'  => 'Golau dydd',
'exif-lightsource-2'  => 'Fflworoleuol',
'exif-lightsource-4'  => 'Fflach',
'exif-lightsource-9'  => 'Tywydd braf',
'exif-lightsource-10' => 'Tywydd cymylog',
'exif-lightsource-11' => 'Cysgod',

'exif-focalplaneresolutionunit-2' => 'modfeddi',

'exif-sensingmethod-1' => 'Heb ei ddiffinio',

'exif-exposuremode-0' => 'Dinoethi awtomatig',
'exif-exposuremode-1' => "Dinoethiad wedi'i osod â llaw",

'exif-scenecapturetype-1' => 'Tirlun',
'exif-scenecapturetype-2' => 'Portread',
'exif-scenecapturetype-3' => 'Golygfa nos',

'exif-gaincontrol-0' => 'Dim',

'exif-contrast-1' => 'Meddal',
'exif-contrast-2' => 'Caled',

'exif-sharpness-1' => 'Meddal',
'exif-sharpness-2' => 'Caled',

'exif-subjectdistancerange-0' => 'Anhysbys',
'exif-subjectdistancerange-2' => 'Golygfa agos',
'exif-subjectdistancerange-3' => 'Golygfa pell',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => "Lledred i'r Gogledd",
'exif-gpslatitude-s' => "Lledred i'r De",

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => "Hydred i'r Dwyrain",
'exif-gpslongitude-w' => "Hydred i'r Gorllewin",

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Cilomedr yr awr',
'exif-gpsspeed-m' => 'Milltir yr awr',
'exif-gpsspeed-n' => 'Notiau',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Gwir gyfeiriad',
'exif-gpsdirection-m' => 'Cyfeiriad magnetig',

# External editor support
'edit-externally'      => 'Golygwch y ffeil gyda rhaglen allanol',
'edit-externally-help' => 'Gwelwch y [http://www.mediawiki.org/wiki/Manual:External_editors cyfarwyddiadau gosod] am fwy o wybodaeth.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'holl',
'imagelistall'     => 'holl',
'watchlistall2'    => 'holl',
'namespacesall'    => 'pob un',
'monthsall'        => 'pob mis',

# E-mail address confirmation
'confirmemail'            => "Cadarnhau'r cyfeiriad e-bost",
'confirmemail_noemail'    => 'Does dim cyfeiriad e-bost dilys wedi ei osod yn eich [[Special:Preferences|dewisiadau defnyddiwr]].',
'confirmemail_text'       => "Cyn i chi allu defnyddio'r nodweddion e-bost, mae'n rhaid i {{SITENAME}} ddilysu'ch cyfeiriad e-bost. Pwyswch y botwm isod er mwyn anfon côd cadarnhau i'ch cyfeiriad e-bost. Bydd yr e-bost yn cynnwys cyswllt gyda chôd ynddi; llwythwch y cyswllt ar eich porwr er mwyn cadarnhau dilysrwydd eich cyfeiriad e-bost.",
'confirmemail_pending'    => '<div class="error">
Mae côd cadarnhau eisoes wedi ei anfon atoch; os ydych newydd greu\'ch cyfrif, hwyrach y gallech ddisgwyl rhai munudau amdano cyn gofyn yr eilwaith am gôd newydd.
</div>',
'confirmemail_send'       => 'Postiwch gôd cadarnhau',
'confirmemail_sent'       => "Wedi anfon e-bost er mwyn cadarnhau'r cyfeiriad.",
'confirmemail_oncreate'   => "Anfonwyd côd cadarnhau at eich cyfeiriad e-bost.
Nid oes rhaid wrth y côd wrth fewngofnodi, ond rhaid ei ddefnyddio er mwyn galluogi offer ar y wici sy'n defnyddio e-bost.",
'confirmemail_sendfailed' => "Ni fu'n bosibl danfon yr e-bost cadarnháu oddi wrth {{SITENAME}}. Gwiriwch eich cyfeiriad e-bost am nodau annilys.

Dychwelodd yr ebostydd: $1",
'confirmemail_invalid'    => 'Côd cadarnhau annilys. Efallai fod y côd wedi dod i ben.',
'confirmemail_needlogin'  => 'Rhaid $1 er mwyn cadarnhau eich cyfeiriad e-bost.',
'confirmemail_success'    => "Mae eich cyfeiriad e-bost wedi'i gadarnhau. Cewch fewngofnodi a mwynhau'r Wici.",
'confirmemail_loggedin'   => 'Cadarnhawyd eich cyfeiriad e-bost.',
'confirmemail_error'      => 'Cafwyd gwall wrth ddanfon eich cadarnhad.',
'confirmemail_subject'    => 'Cadarnhâd cyfeiriad e-bost ar {{SITENAME}}',
'confirmemail_body'       => 'Mae rhywun (chi, yn fwy na thebyg, o\'r cyfeiriad IP $1) wedi cofrestru\'r cyfrif "$2" ar {{SITENAME}} gyda\'r cyfeiriad e-bost hwn.

I gadarnhau mai chi yn wir yw perchennog y cyfrif hwn, ac i alluogi nodweddion e-bost ar {{SITENAME}}, agorwch y cyswllt hwn yn eich porwr:

$3

Os *nad* chi sydd berchen y cyfrif hwn, dilynwch y cyswllt hwn er mwyn dileu cadarnhad y cyfeiriad e-bost:

$5 

Bydd y côd cadarnhau yn dod i ben am $4.',

# Scary transclusion
'scarytranscludedisabled' => '[Analluogwyd cynhwysiad rhyng-wici]',
'scarytranscludefailed'   => '[Methwyd â nôl y nodyn ar gyfer $1]',
'scarytranscludetoolong'  => "[Mae'r URL yn rhy hir]",

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">
Cysylltiadau \'Trackback\' ar gyfer yr erthygl hon:<br />
$1
</div>',
'trackbackremove'   => ' ([$1 Dileu])',
'trackbacklink'     => "Cyswllt 'trackback'",
'trackbackdeleteok' => "Dilewyd y cyswllt 'trackback' yn lwyddiannus.",

# Delete conflict
'deletedwhileediting' => "'''Rhybudd''': Dilëwyd y dudalen wedi i chi ddechrau ei golygu!",
'confirmrecreate'     => "Mae'r defnyddiwr [[User:$1|$1]] ([[User talk:$1|Sgwrs]]) wedi dileu'r erthygl hon ers i chi ddechrau golygu. Y rheswm oedd:
: ''$2''
Cadarnhewch eich bod chi wir am ail-greu'r erthygl.",
'recreate'            => 'Ail-greu',

# HTML dump
'redirectingto' => "Wrthi'n ailgyfeirio i [[:$1]]...",

# action=purge
'confirm_purge'        => "Clirio'r dudalen o'r storfa?

$1",
'confirm_purge_button' => 'Iawn',

# AJAX search
'searchcontaining' => "Chwilio am dudalennau yn cynnwys ''$1''.",
'searchnamed'      => "Chwilio am dudalennau a'r enw ''$1''.",
'articletitles'    => "Erthyglau'n dechrau gyda: ''$1''",
'hideresults'      => "Cuddio'r canlyniadau",
'useajaxsearch'    => 'Chwilio gyda AJAX',

# Multipage image navigation
'imgmultipageprev' => "← i'r dudalen gynt",
'imgmultipagenext' => "i'r dudalen nesaf →",
'imgmultigo'       => 'Eler!',
'imgmultigoto'     => "Mynd i'r dudalen $1",

# Table pager
'ascending_abbrev'         => 'esgynnol',
'descending_abbrev'        => 'am lawr',
'table_pager_next'         => 'Tudalen nesaf',
'table_pager_prev'         => 'Tudalen gynt',
'table_pager_first'        => 'Tudalen gyntaf',
'table_pager_last'         => 'Tudalen olaf',
'table_pager_limit'        => 'Dangos $1 eitem y dudalen',
'table_pager_limit_submit' => 'Eler',
'table_pager_empty'        => 'Dim canlyniadau',

# Auto-summaries
'autosumm-blank'   => "Yn gwacau'r dudalen yn llwyr",
'autosumm-replace' => "Gwacawyd y dudalen a gosod y canlynol yn ei le: '$1'",
'autoredircomment' => 'Yn ailgyfeirio at [[$1]]',
'autosumm-new'     => 'Tudalen newydd: $1',

# Live preview
'livepreview-loading' => "Wrthi'n llwytho…",
'livepreview-ready'   => 'Llwytho… Ar ben!',
'livepreview-failed'  => 'Y rhagolwg byw wedi methu! Rhowch gynnig ar y rhagolwg arferol.',
'livepreview-error'   => 'Wedi methu cysylltu: $1 "$2". Rhowch gynnig ar y rhagolwg arferol.',

# Friendlier slave lag warnings
'lag-warn-normal' => 'Hwyrach na ddangosir isod y newidiadau a ddigwyddodd o fewn y $1 {{PLURAL:$1|eiliad|eiliad|eiliad|eiliad|eiliad|eiliad}} ddiwethaf.',
'lag-warn-high'   => 'Mae gweinydd y data-bas ar ei hôl hi: efallai nad ymddengys newidiadau o fewn y $1 {{PLURAL:$1|eiliad|eiliad|eiliad|eiliad|eiliad|eiliad}} ddiwethaf ar y rhestr.',

# Watchlist editor
'watchlistedit-numitems'       => 'Mae {{PLURAL:$1|$1 tudalen|$1 dudalen|$1 dudalen|$1 tudalen|$1 thudalen|$1 o dudalennau}} ar eich rhestr gwylio, heb gynnwys tudalennau sgwrs.',
'watchlistedit-noitems'        => "Mae'ch rhestr gwylio'n wag.",
'watchlistedit-normal-title'   => "Golygu'r rhestr gwylio",
'watchlistedit-normal-legend'  => 'Tynnu tudalennau oddi ar y rhestr gwylio',
'watchlistedit-normal-explain' => "Rhestrir y teitlau ar eich rhestr gwylio isod. I dynnu teitl oddi ar y rhestr, ticiwch y blwch ar ei gyfer, yna cliciwch 'Tynnu'r tudalennau'. Gallwch hefyd ddewis golygu'r rhestr gwylio ar ei [[Special:Watchlist/raw|ffurf syml]].",
'watchlistedit-normal-submit'  => "Tynnu'r tudalennau",
'watchlistedit-normal-done'    => 'Tynnwyd {{PLURAL:$1|$1 tudalen|$1 dudalen|$1 dudalen|$1 tudalen|$1 thudalen|$1 tudalen}} oddi ar eich rhestr gwylio:',
'watchlistedit-raw-title'      => 'Golygu ffeil y rhestr gwylio',
'watchlistedit-raw-legend'     => 'Golygu ffeil y rhestr gwylio',
'watchlistedit-raw-explain'    => "Rhestrir y teitlau ar eich rhestr gwylio isod. Gellir newid y rhestr drwy ychwanegu neu dynnu teitlau; gyda llinell yr un i bob teitl. Pan yn barod, pwyswch ar Diweddaru'r rhestr gwylio.
Gallwch hefyd [[Special:Watchlist/edit|ddefnyddio'r rhestr arferol]].",
'watchlistedit-raw-titles'     => 'Teitlau:',
'watchlistedit-raw-submit'     => "Diweddaru'r rhestr gwylio",
'watchlistedit-raw-done'       => 'Diweddarwyd eich rhestr gwylio.',
'watchlistedit-raw-added'      => 'Ychwanegwyd {{PLURAL:$1|1 teitl|$1 teitl|$1 deitl|$1 theitl|$1 theitl|$1 o deitlau}}:',
'watchlistedit-raw-removed'    => 'Tynnwyd {{PLURAL:$1|1 teitl|$1 teitl|$1 deitl|$1 theitl|$1 theitl|$1 o deitlau}}:',

# Watchlist editing tools
'watchlisttools-view' => 'Gweld newidiadau perthnasol',
'watchlisttools-edit' => "Gweld a golygu'r rhestr gwylio",
'watchlisttools-raw'  => "Golygu'r rhestr gwylio syml",

# Core parser functions
'unknown_extension_tag' => 'Tag estyniad anhysbys "$1"',

# Special:Version
'version'                       => 'Fersiwn', # Not used as normal message but as header for the special page itself
'version-extensions'            => 'Estyniadau gosodedig',
'version-specialpages'          => 'Tudalennau arbennig',
'version-parserhooks'           => 'Bachau dosrannydd',
'version-variables'             => 'Newidynnau',
'version-other'                 => 'Arall',
'version-mediahandlers'         => 'Trinyddion cyfryngau',
'version-hooks'                 => 'Bachau',
'version-extension-functions'   => 'Ffwythiannau estyn',
'version-parser-extensiontags'  => 'Tagiau estyn dosrannydd',
'version-parser-function-hooks' => 'Bachau ffwythiant dosrannu',
'version-hook-name'             => "Enw'r bachyn",
'version-hook-subscribedby'     => 'Tanysgrifwyd gan',
'version-version'               => 'Fersiwn',
'version-license'               => 'Trwydded',
'version-software'              => 'Meddalwedd gosodedig',
'version-software-product'      => 'Cynnyrch',
'version-software-version'      => 'Fersiwn',

# Special:FilePath
'filepath'         => 'Llwybr y ffeil',
'filepath-page'    => 'Ffeil:',
'filepath-submit'  => 'Llwybr',
'filepath-summary' => 'Mae\'r dudalen arbennig hon yn adrodd llwybr ffeil yn gyfan.
Dangosir delweddau ar eu llawn maint, dechreuir ffeiliau o fathau eraill yn uniongyrchol gan y rhaglen cysylltiedig.

Rhowch enw\'r ffeil heb y rhagddodiad "{{ns:image}}:".',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => 'Chwilio am ffeiliau dyblyg',
'fileduplicatesearch-summary'  => 'Chwilier am ffeiliau dyblyg ar sail ei werth stwnsh.

Rhowch enw\'r ffeil heb y rhagddodiad "{{ns:image}}:".',
'fileduplicatesearch-legend'   => 'Chwilio am ddyblygeb',
'fileduplicatesearch-filename' => "Enw'r ffeil:",
'fileduplicatesearch-submit'   => 'Chwilier',
'fileduplicatesearch-info'     => '$1 × $2 picsel<br />Maint y ffeil: $3<br />math MIME: $4',
'fileduplicatesearch-result-1' => 'Nid oes yr un ffeil i gael sydd yn union yr un fath â\'r ffeil "$1".',
'fileduplicatesearch-result-n' => '{{PLURAL:$2|Nid oes yr un ffeil|Mae $2 ffeil|Mae $2 ffeil|Mae $2 ffeil|Mae $2 ffeil|Mae $2 ffeil|}} i gael sydd yn union yr un fath â\'r ffeil "$1".',

# Special:SpecialPages
'specialpages'                   => 'Tudalennau arbennig',
'specialpages-note'              => '----
* Tudalennau arbennig ar gael i bawb.
* <span class="mw-specialpagerestricted">Tudalennau arbennig cyfyngedig.</span>',
'specialpages-group-maintenance' => 'Adroddiadau cynnal a chadw',
'specialpages-group-other'       => 'Eraill',
'specialpages-group-login'       => 'Mewngofnodi / creu cyfrif',
'specialpages-group-changes'     => 'Newidiadau diweddar a logiau',
'specialpages-group-media'       => 'Ffeiliau - adroddiadau ac uwchlwytho',
'specialpages-group-users'       => "Defnyddwyr a'u galluoedd",
'specialpages-group-highuse'     => 'Tudalennau aml eu defnydd',
'specialpages-group-pages'       => 'Rhestr tudalennau',
'specialpages-group-pagetools'   => 'Offer trin tudalennau',
'specialpages-group-wiki'        => 'Data ac offer y wici',
'specialpages-group-redirects'   => 'Tudalennau arbennig ailgyfeirio',
'specialpages-group-spam'        => 'Offer sbam',

# Special:BlankPage
'blankpage'              => 'Tudalen wag',
'intentionallyblankpage' => 'Gadawyd y dudalen hon yn wag o fwriad',

);
