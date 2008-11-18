<?php
/** Irish (Gaeilge)
 *
 * @ingroup Language
 * @file
 *
 * @author Alison
 * @author Kwekubo
 * @author Moilleadóir
 * @author Spacebirdy
 * @author לערי ריינהארט
 */

$skinNames = array(
	'standard' => 'Gnáth',
	'nostalgia' => 'Sean-nós',
	'cologneblue' => 'Gorm na Colóna',
	'monobook' => 'MonoBook',
	'myskin' => 'MySkin',
	'chick' => 'Chick'
);

$magicWords = array(
	#   ID	                         CASE  SYNONYMS
	'redirect'               => array( 0,    '#redirect', '#athsheoladh' ),
	'notoc'                  => array( 0,    '__NOTOC__', '__GANCÁ__'              ),
	'forcetoc'               => array( 0,    '__FORCETOC__',         '__CÁGACHUAIR__'  ),
	'toc'                    => array( 0,    '__TOC__', '__CÁ__'                ),
	'noeditsection'          => array( 0,    '__NOEDITSECTION__',    '__GANMHÍRATHRÚ__'  ),
	'currentmonth'           => array( 1,    'CURRENTMONTH',  'MÍLÁITHREACH'  ),
	'currentmonthname'       => array( 1,    'CURRENTMONTHNAME',     'AINMNAMÍOSALÁITHREAÍ'  ),
	'currentmonthnamegen'    => array( 1,    'CURRENTMONTHNAMEGEN',  'GINAINMNAMÍOSALÁITHREAÍ'  ),
	'currentmonthabbrev'     => array( 1,    'CURRENTMONTHABBREV',   'GIORRÚNAMÍOSALÁITHREAÍ'  ),
	'currentday'             => array( 1,    'CURRENTDAY',           'LÁLÁITHREACH'  ),
	'currentdayname'         => array( 1,    'CURRENTDAYNAME',       'AINMANLAELÁITHRIGH'  ),
	'currentyear'            => array( 1,    'CURRENTYEAR',          'BLIAINLÁITHREACH'  ),
	'currenttime'            => array( 1,    'CURRENTTIME',          'AMLÁITHREACH'  ),
	'numberofarticles'       => array( 1,    'NUMBEROFARTICLES',     'LÍONNANALT'  ),
	'numberoffiles'          => array( 1,    'NUMBEROFFILES',        'LÍONNAGCOMHAD'  ),
	'pagename'               => array( 1,    'PAGENAME',             'AINMANLGH'  ),
	'pagenamee'              => array( 1,    'PAGENAMEE',            'AINMANLGHB'  ),
	'namespace'              => array( 1,    'NAMESPACE',            'AINMSPÁS'  ),
	'msg'                    => array( 0,    'MSG:',                 'TCHT:'  ),
	'subst'                  => array( 0,    'SUBST:',               'IONAD:'  ),
	'msgnw'                  => array( 0,    'MSGNW:',               'TCHTFS:'  ),
	'img_thumbnail'          => array( 1,    'thumbnail', 'thumb',   'mionsamhail', 'mion'  ),
	'img_right'              => array( 1,    'right',                'deas'  ),
	'img_left'               => array( 1,    'left',                 'clé'  ),
	'img_none'               => array( 1,    'none',                 'faic'  ),
	'img_center'             => array( 1,    'center', 'centre',     'lár'  ),
	'img_framed'             => array( 1,    'framed', 'enframed', 'frame', 'fráma', 'frámaithe' ),
	'int'                    => array( 0,    'INT:', 'INMH:'                   ),
	'sitename'               => array( 1,    'SITENAME',             'AINMANTSUÍMH'  ),
	'ns'                     => array( 0,    'NS:', 'AS:'                    ),
	'localurl'               => array( 0,    'LOCALURL:',            'URLÁITIÚIL'  ),
	'localurle'              => array( 0,    'LOCALURLE:',           'URLÁITIÚILB'  ),
	'server'                 => array( 0,    'SERVER',               'FREASTALAÍ'  ),
	'servername'             => array( 0,    'SERVERNAME',            'AINMANFHREASTALAÍ' ),
	'scriptpath'             => array( 0,    'SCRIPTPATH',           'SCRIPTCHOSÁN'  ),
	'grammar'                => array( 0,    'GRAMMAR:',             'GRAMADACH:'  ),
	'notitleconvert'         => array( 0,    '__NOTITLECONVERT__', '__NOTC__', '__GANTIONTÚNADTEIDEAL__', '__GANTT__'),
	'nocontentconvert'       => array( 0,    '__NOCONTENTCONVERT__', '__NOCC__', '__GANTIONTÚNANÁBHAIR__', '__GANTA__' ),
	'currentweek'            => array( 1,    'CURRENTWEEK',          'SEACHTAINLÁITHREACH'  ),
	'currentdow'             => array( 1,    'CURRENTDOW',           'LÁLÁITHREACHNAS'  ),
	'revisionid'             => array( 1,    'REVISIONID',           'IDANLEASAITHE'  ),
);

$namespaceNames = array(
	NS_MEDIA	          => 'Meán',
	NS_SPECIAL          => 'Speisialta',
	NS_MAIN             => '',
	NS_TALK             => 'Plé',
	NS_USER             => 'Úsáideoir',
	NS_USER_TALK        => 'Plé_úsáideora',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => 'Plé_{{grammar:genitive|$1}}',
	NS_IMAGE            => 'Íomhá',
	NS_IMAGE_TALK       => 'Plé_íomhá',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'Plé_MediaWiki',
	NS_TEMPLATE         => 'Teimpléad',
	NS_TEMPLATE_TALK    => 'Plé_teimpléid',
	NS_HELP             => 'Cabhair',
	NS_HELP_TALK        => 'Plé_cabhrach',
	NS_CATEGORY         => 'Catagóir',
	NS_CATEGORY_TALK    => 'Plé_catagóire'
);

$namespaceAliases = array(
	'Plé_í­omhá' => NS_IMAGE_TALK,
	'Múnla' => NS_TEMPLATE,
	'Plé_múnla' => NS_TEMPLATE_TALK,
	'Rang' => NS_CATEGORY
);

$messages = array(
# User preference toggles
'tog-underline'               => 'Cuir línte faoi naisc:',
'tog-highlightbroken'         => 'Formáidigh na naisc briste, <a href="" class="new">mar seo</a>
(rogha malartach: mar seo<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Comhfhadaigh na paragraif',
'tog-hideminor'               => 'Ná taispeáin fo-athruithe i measc na n-athruithe is déanaí',
'tog-extendwatchlist'         => 'Leathnaigh an liosta faire chun gach athrú cuí a thaispeáint',
'tog-usenewrc'                => 'Stíl nua do na hathruithe is déanaí (le JavaScript)',
'tog-numberheadings'          => 'Uimhrigh ceannteidil go huathoibríoch',
'tog-showtoolbar'             => 'Taispeáin an barra uirlisí eagair (JavaScript)',
'tog-editondblclick'          => 'Cuir leathanaigh in eagar le déchliceáil (JavaScript)',
'tog-editsection'             => 'Cumasaigh mír-eagarthóireacht le naisc mar seo: [athrú]',
'tog-editsectiononrightclick' => 'Cumasaigh mír-eagarthóireacht le deaschliceáil<br /> ar ceannteidil (JavaScript)',
'tog-showtoc'                 => "Taispeáin an clár ábhair (d'ailt le níos mó ná 3 ceannteidil)",
'tog-rememberpassword'        => "Cuimhnigh m'fhocal faire",
'tog-editwidth'               => 'Cuir uasmhéid ar an mbosca eagair',
'tog-watchcreations'          => 'Déan faire ar leathanaigh a chruthaím',
'tog-watchdefault'            => 'Déan faire ar leathanaigh a athraím',
'tog-watchmoves'              => 'Déan faire ar leathanaigh a athainmnaím',
'tog-watchdeletion'           => 'Déan faire ar leathanaigh a scriosaim',
'tog-minordefault'            => 'Déan mionathrú de gach aon athrú, mar réamhshocrú',
'tog-previewontop'            => 'Cuir an réamhamharc os cionn an bhosca eagair, <br />agus ná cuir é taobh thíos de',
'tog-previewonfirst'          => 'Taispeáin réamhamharc don chéad athrú',
'tog-nocache'                 => 'Ciorraigh taisce na leathanach',
'tog-enotifwatchlistpages'    => 'Cuir ríomhphost chugam nuair a athraítear leathanaigh',
'tog-enotifusertalkpages'     => 'Cuir ríomhphost chugam nuair a athraítear mo leathanach phlé úsáideora',
'tog-enotifminoredits'        => 'Cuir ríomhphost chugam nuair a dhéantar mionathruithe chomh maith',
'tog-enotifrevealaddr'        => 'Taispeáin mo sheoladh ríomhphoist i dteachtaireachtaí fógra',
'tog-shownumberswatching'     => 'Taispeán an méid úsáideoirí atá ag faire',
'tog-fancysig'                => 'Síniuithe bunúsacha (gan nasc uathoibríoch)',
'tog-externaleditor'          => 'Bain úsáid as eagarthóir seachtrach, mar réamhshocrú',
'tog-externaldiff'            => 'Bain úsáid as difríocht sheachtrach, mar réamhshocrú',
'tog-showjumplinks'           => 'Cumasaigh naisc insroichteachta "léim go dtí"',
'tog-uselivepreview'          => 'Bain úsáid as réamhamharc beo (JavaScript) (Turgnamhach)',
'tog-watchlisthideown'        => 'Folaigh mo chuid athruithe ón liosta faire',
'tog-watchlisthidebots'       => 'Folaigh athruithe de chuid róbait ón liosta faire',
'tog-watchlisthideminor'      => 'Folaigh mionathruithe ón liosta faire',

'underline-always'  => 'Déan i gcónaí é',
'underline-never'   => 'Ná déan é riamh',
'underline-default' => 'Réamhshocrú an bhrabhsálaí',

'skinpreview' => '(Réamhamharc)',

# Dates
'sunday'        => 'an Domhnach',
'monday'        => 'an Luan',
'tuesday'       => 'an Mháirt',
'wednesday'     => 'an Chéadaoin',
'thursday'      => 'an Déardaoin',
'friday'        => 'an Aoine',
'saturday'      => 'an Satharn',
'sun'           => 'Domh',
'mon'           => 'Luan',
'tue'           => 'Máirt',
'wed'           => 'Céad',
'thu'           => 'Déar',
'fri'           => 'Aoine',
'sat'           => 'Sath',
'january'       => 'Eanáir',
'february'      => 'Feabhra',
'march'         => 'Márta',
'april'         => 'Aibreán',
'may_long'      => 'Bealtaine',
'june'          => 'Meitheamh',
'july'          => 'Iúil',
'august'        => 'Lúnasa',
'september'     => 'Meán Fómhair',
'october'       => 'Deireadh Fómhair',
'november'      => 'Mí na Samhna',
'december'      => 'Mí na Nollag',
'january-gen'   => 'Eanáir',
'february-gen'  => 'Feabhra',
'march-gen'     => 'an Mhárta',
'april-gen'     => 'an Aibreáin',
'may-gen'       => 'na Bealtaine',
'june-gen'      => 'an Mheithimh',
'july-gen'      => 'Iúil',
'august-gen'    => 'Lúnasa',
'september-gen' => 'Mheán Fómhair',
'october-gen'   => 'Dheireadh Fómhair',
'november-gen'  => 'na Samhna',
'december-gen'  => 'na Nollag',
'jan'           => 'Ean',
'feb'           => 'Feabh',
'mar'           => 'Márta',
'apr'           => 'Aib',
'may'           => 'Beal',
'jun'           => 'Meith',
'jul'           => 'Iúil',
'aug'           => 'Lún',
'sep'           => 'MFómh',
'oct'           => 'DFómh',
'nov'           => 'Samh',
'dec'           => 'Noll',

# Categories related messages
'pagecategories'              => '{{PLURAL:$1|Catagóir|Catagóirí}}',
'category_header'             => 'Ailt sa chatagóir "$1"',
'subcategories'               => 'Fo-chatagóirí',
'category-media-header'       => 'Meáin sa chatagóir "$1"',
'category-empty'              => "''Níl aon leathanaigh ná méid sa chatagóir ar an am seo.''",
'category-subcat-count'       => '{{PLURAL:$2| Níl ach an fo-chatagóir seo a leanas ag an gcatagóir seo.|Tá {{PLURAL:$1|fo-chatagóir|fo-chatagóirí}} ag an gcatagóir seo, as $2 sam iomlán.}}',
'category-article-count'      => '{{PLURAL:$2|Níl sa chatagóir seo ach an leathanach seo a leanas.|Tá {{PLURAL:$1|$1 leathanach|$1 leathanaigh}} sa chatagóir seo, as iomlán de $2.}}',
'category-file-count'         => '{{PLURAL:$2|Tá ach an comhad a leanas sa chatagóir seo|Tá {{PLURAL:$1|an comhad seo|$1 na comhaid seo}} a leanas sa chatagóir seo, as $2 san iomlán.}}',
'category-file-count-limited' => 'Tá {{PLURAL:$1|an comhad seo|$1 na comhaid seo}} a leanas sa chatagóir reatha.',
'listingcontinuesabbrev'      => 'ar lean.',

'mainpagetext'      => "<big>'''D'éirigh le suiteáil MediaWiki.'''</big>",
'mainpagedocfooter' => 'Féach ar [http://meta.wikimedia.org/wiki/MediaWiki_localisation doiciméid um conas an chomhéadán a athrú]
agus an [http://meta.wikimedia.org/wiki/MediaWiki_User%27s_Guide Lámhleabhar úsáideora] chun cabhair úsáide agus fíoraíochta a fháil.',

'about'          => 'Maidir leis',
'article'        => 'Leathanach ábhair',
'newwindow'      => '(a osclófar i bhfuinneog nua)',
'cancel'         => 'Cealaigh',
'qbfind'         => 'Aimsigh',
'qbbrowse'       => 'Brabhsáil',
'qbedit'         => 'Cuir in eagar',
'qbpageoptions'  => 'An leathanach seo',
'qbpageinfo'     => 'Comhthéacs',
'qbmyoptions'    => 'Mo chuid leathanaigh',
'qbspecialpages' => 'Leathanaigh speisialta',
'moredotdotdot'  => 'Tuilleadh...',
'mypage'         => 'Mo leathanach',
'mytalk'         => 'Mo chuid phlé',
'anontalk'       => 'Plé don seoladh IP seo',
'navigation'     => 'Nascleanúint',
'and'            => 'agus',

# Metadata in edit box
'metadata_help' => 'Meiteasonraí:',

'errorpagetitle'    => 'Earráid',
'returnto'          => 'Dul ar ais go $1.',
'tagline'           => 'Ó {{SITENAME}}.',
'help'              => 'Cabhair',
'search'            => 'Cuardaigh',
'searchbutton'      => 'Cuardaigh',
'go'                => 'Téir',
'searcharticle'     => 'Téir',
'history'           => 'Stair an lgh seo',
'history_short'     => 'Stair',
'updatedmarker'     => 'leasaithe (ó shin mo chuairt dheireanach)',
'info_short'        => 'Eolas',
'printableversion'  => 'Eagrán inphriontáilte',
'permalink'         => 'Nasc seasmhach',
'print'             => 'Priontáil',
'edit'              => 'Athraigh an lch seo',
'create'            => 'Cruthaigh',
'editthispage'      => 'Athraigh an lch seo',
'create-this-page'  => 'Cruthaigh an lch seo',
'delete'            => 'Scrios',
'deletethispage'    => 'Scrios an lch seo',
'undelete_short'    => 'Díscrios {{PLURAL:$1|athrú amháin|$1 athruithe}}',
'protect'           => 'Glasáil',
'protect_change'    => 'athraigh',
'protectthispage'   => 'Glasáil an lch seo',
'unprotect'         => 'Díghlasáil',
'unprotectthispage' => 'Díghlasáil an lch seo',
'newpage'           => 'Leathanach nua',
'talkpage'          => 'Pléigh an lch seo',
'talkpagelinktext'  => 'Plé',
'specialpage'       => 'Leathanach Speisialta',
'personaltools'     => 'Do chuid uirlisí',
'postcomment'       => 'Caint ar an lch',
'articlepage'       => 'Féach ar an alt',
'talk'              => 'Plé',
'views'             => 'Radhairc',
'toolbox'           => 'Bosca uirlisí',
'userpage'          => 'Féach ar lch úsáideora',
'projectpage'       => 'Féach ar lch thionscadail',
'imagepage'         => 'Féach ar lch íomhá',
'mediawikipage'     => 'Féach ar lch teachtaireacht',
'templatepage'      => 'Féach ar leathanach an teimpléad',
'viewhelppage'      => 'Féach ar lch chabhair',
'categorypage'      => 'Féach ar lch chatagóir',
'viewtalkpage'      => 'Féach ar phlé',
'otherlanguages'    => 'I dteangacha eile',
'redirectedfrom'    => '(Athsheolta ó $1)',
'redirectpagesub'   => 'Lch athdhírithe',
'lastmodifiedat'    => 'Athraíodh an leathanach seo ag $2, $1.', # $1 date, $2 time
'viewcount'         => 'Rochtainíodh an leathanach seo {{PLURAL:$1|uair amháin|$1 uaire}}.',
'protectedpage'     => 'Leathanach glasáilte',
'jumpto'            => 'Léim go:',
'jumptonavigation'  => 'nascleanúint',
'jumptosearch'      => 'cuardaigh',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Maidir leis an {{SITENAME}}',
'aboutpage'            => 'Project:Maidir leis',
'bugreports'           => 'Fabht-thuairiscí',
'bugreportspage'       => 'Project:Fabht-thuairiscí',
'copyright'            => 'Tá an t-ábhar le fáil faoin $1.',
'copyrightpagename'    => 'Cóipcheart {{GRAMMAR:genitive|{{SITENAME}}}}',
'copyrightpage'        => '{{ns:project}}:Cóipchearta',
'currentevents'        => 'Cursaí reatha',
'currentevents-url'    => 'Project:Cursaí reatha',
'disclaimers'          => 'Séanadh',
'disclaimerpage'       => 'Project:Séanadh_ginearálta',
'edithelp'             => 'Cabhair eagarthóireachta',
'edithelppage'         => 'Help:Eagarthóireacht',
'faq'                  => 'Ceisteanna Coiteanta',
'faqpage'              => 'Project:Ceisteanna_Coiteanta',
'helppage'             => 'Help:Clár_ábhair',
'mainpage'             => 'Príomhleathanach',
'mainpage-description' => 'Príomhleathanach',
'policy-url'           => 'Project:Polasaí',
'portal'               => 'Ionad pobail',
'portal-url'           => 'Project:Ionad pobail',
'privacy'              => 'Polasaí príobháideachta',
'privacypage'          => 'Project:Polasaí príobháideachta',

'badaccess' => 'Earráid ceada',

'versionrequired'     => 'Tá leagan $1 de MediaWiki de dhíth',
'versionrequiredtext' => 'Tá an leagan $1 de MediaWiki riachtanach chun an leathanach seo a úsáid. Féach ar [[Special:Version]]',

'ok'                      => 'Déan',
'retrievedfrom'           => 'Aisghabháil ó "$1"',
'youhavenewmessages'      => 'Tá $1 agat ($2).',
'newmessageslink'         => 'teachtaireachtaí nua',
'newmessagesdifflink'     => 'difear ón leasú leathdhéanach',
'youhavenewmessagesmulti' => 'Tá teachtaireachtaí nua agat ar $1',
'editsection'             => 'athraigh',
'editold'                 => 'athraigh',
'viewsourceold'           => 'féach ar foinse',
'editsectionhint'         => 'Athraigh mír: $1',
'toc'                     => 'Clár ábhair',
'showtoc'                 => 'taispeáin',
'hidetoc'                 => 'folaigh',
'thisisdeleted'           => 'Breathnaigh nó cuir ar ais $1?',
'viewdeleted'             => 'Féach ar $1?',
'restorelink'             => '{{PLURAL:$1|athrú scriosta amháin|$1 athruithe scriosta}}',
'feedlinks'               => 'Fotha:',
'feed-invalid'            => 'Cineál liostáil fotha neamhbhailí.',
'feed-unavailable'        => 'Níl fotha sindeacáitiú ar fáil ar {{SITENAME}}.',
'site-rss-feed'           => '$1 Fotha RSS',
'site-atom-feed'          => '$1 Fotha Atom',
'page-rss-feed'           => '"$1" Fotha RSS',
'page-atom-feed'          => '"$1" Fotha Atom',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Alt',
'nstab-user'      => 'Lch úsáideora',
'nstab-media'     => 'Lch meáin',
'nstab-special'   => 'Speisialta',
'nstab-project'   => 'Tionscadal',
'nstab-image'     => 'Comhad',
'nstab-mediawiki' => 'Teachtaireacht',
'nstab-template'  => 'Teimpléad',
'nstab-help'      => 'Cabhair',
'nstab-category'  => 'Catagóir',

# Main script and global functions
'nosuchaction'      => 'Níl a leithéid de ghníomh ann',
'nosuchactiontext'  => 'Níl aithníonn an vicí an gníomh atá ann sa líonsheoladh.',
'nosuchspecialpage' => 'Níl a leithéid de leathanach speisialta ann',
'nospecialpagetext' => "Níl aithníonn an vicí an leathanach speisialta a d'iarr tú ar.",

# General errors
'error'                => 'Earráid',
'databaseerror'        => 'Earráid sa bhunachar sonraí',
'dberrortext'          => 'Tharla earráid chomhréire in iarratas chuig an mbunachar sonraí.
B\'fhéidir gur fabht sa bhogearraí é seo.
Seo é an t-iarratas deireanach chuig an mbunachar sonrai:
<blockquote><tt>$1</tt></blockquote>
ón bhfeidhm "<tt>$2</tt>".
Thug MySQL an earráid seo: "<tt>$3: $4</tt>".',
'dberrortextcl'        => 'Tharlaigh earráid chomhréire in iarratas chuig an bhunachar

sonraí.
"$1", ón suim "$2",
ab ea an iarratas fiosraithe deireanach chuig an bhunachar sonrai,
Chuir MySQL an earráid seo ar ais: "$3: $4".',
'noconnect'            => 'Tá brón orainn! Tá roinnt deacrachtaí teicniúla ag an vicí faoi láthair, agus ní féidir leis teagmháil a dhéanamh leis an mbunachar sonraí. <br />
$1',
'nodb'                 => 'Theip rogha an bhunachair sonraí $1',
'cachederror'          => 'Seo í cóip athscríofa den leathanach a raibh tú ag lorg (is dócha nach bhfuil sí bord ar bhord leis an leagan reatha).',
'laggedslavemode'      => "Rabhadh: B'fhéidir nach bhfuil na nuashonrúcháin is déanaí le feiceáil ar an leathanach seo.",
'readonly'             => 'Bunachar sonraí glasáilte',
'enterlockreason'      => 'Iontráil cúis don glasáil, agus meastachán
den uair a díghlasálfar an bunachar sonraí.',
'readonlytext'         => 'Tá an bunachar sonraí {{GRAMMAR:genitive|{{SITENAME}}}} glasáilte anois do iontráilí agus athruithe nua
(is dócha go bhfuil sé do gnáthchothabháil).
Tar éis seo, díghlasálfar an bunachar sonraí arís.
Tugadh an míniú seo ag an riarthóir a ghlasáil é:
$1',
'missingarticle-rev'   => '(leagan#: $1)',
'missingarticle-diff'  => '(Diof: $1, $2)',
'readonly_lag'         => 'Glasáladh an bunachar sonraí go huathoibríoch, go dtiocfaidh na sclábhfhreastalaithe suas leis an máistirfhreastalaí.',
'internalerror'        => 'Earráid inmhéanach',
'internalerror_info'   => 'Earráid inmhéanach: $1',
'filecopyerror'        => 'Ní féidir an comhad "$1" a chóipeáil go "$2".',
'filerenameerror'      => 'Ní féidir an comhad "$1" a athainmnigh mar "$2".',
'filedeleteerror'      => 'Ní féidir an comhad "$1" a scriosaigh amach.',
'directorycreateerror' => 'Ní féidir an chomhadlann "$1" a chruth.',
'filenotfound'         => 'Ní bhfuarthas an comhad "$1".',
'unexpected'           => 'Luach gan súil leis: "$1"="$2".',
'formerror'            => 'Earráid: ní féidir an foirm a tabhair isteach',
'badarticleerror'      => 'Ní féidir an gníomh seo a dhéanamh ar an leathanach seo.',
'cannotdelete'         => "Ní féidir an leathanach nó íomhá sonraithe a scriosaigh. (B'fhéidir gur scrios duine eile é cheana féin.)",
'badtitle'             => 'Teideal neamhbhailí',
'badtitletext'         => "Bhí teideal an leathanaigh a d'iarr tú ar neamhbhailí, folamh, nó
teideal idirtheangach nó idirvicí nasctha go mícheart.",
'perfdisabled'         => 'Tá brón orainn! Díchumasaíodh an gné seo ar feadh tamaill chun luas an bhunachair sonraí a chosaint.',
'perfcached'           => 'Fuarthas na sonraí a leanas as taisce, agus is dócha go bhfuil siad as dáta.',
'wrong_wfQuery_params' => 'Paraiméadair míchearta don wfQuery()<br />
Feidhm: $1<br />
Iarratas: $2',
'viewsource'           => 'Féach ar fhoinse',
'viewsourcefor'        => 'le haghaidh $1',
'protectedpagetext'    => 'Tá an leathanach seo glasáilte chun coisc ar eagarthóireacht.',
'viewsourcetext'       => 'Is féidir foinse an leathanach seo a fheiceáil ná a cóipeáil:',
'editinginterface'     => "'''Rabhadh:''' Tá tú ag athrú leathanaigh a bhfuil téacs comhéadain do na bogearraí air. Cuirfear athruithe ar an leathanach seo i bhfeidhm ar an gcomhéadan úsáideora.
Más maith leat MediaWiki a aistriú, cuimhnigh ar [http://translatewiki.net/wiki/Main_Page?setlang=ga Betawiki] (tionscadal logánaithe MediaWiki) a úsáid.",
'sqlhidden'            => '(Iarratas SQL folaithe)',
'titleprotected'       => "Tá an teideal seo cosanta ar chruthú le [[User:$1|$1]].
An fáth ná ''$2''.",

# Login and logout pages
'logouttitle'                => 'Logáil amach',
'logouttext'                 => 'Tá tú logáilte amach anois.
Is féidir leat an {{SITENAME}} a úsáid fós gan ainm, nó is féidir leat logáil isteach
arís mar an úsáideoir céanna, nó mar úsáideoir eile. Tabhair faoi deara go taispeáinfear roinnt
leathanaigh mar atá tú logtha ann fós, go dtí go ghlanfá amach do thaisce brabhsálaí',
'welcomecreation'            => '== Tá fáilte romhat, $1! ==

Cruthaíodh do chuntas. Ná déan dearmad ar do sainroghanna phearsanta {{GRAMMAR:genitive|{{SITENAME}}}} a hathrú.',
'loginpagetitle'             => 'Logáil isteach',
'yourname'                   => "D'ainm úsáideora",
'yourpassword'               => "D'fhocal faire",
'yourpasswordagain'          => "Athiontráil d'fhocal faire",
'remembermypassword'         => 'Cuimhnigh orm',
'yourdomainname'             => "D'fhearann",
'externaldberror'            => 'Bhí earráid bhunachair sonraí ann maidir le fíordheimhniú seachtrach, nóThere was either an external authentication database error or you are not allowed to update your external account.',
'loginproblem'               => '<b>Tharla earráid agus tú ag logáil isteach.</b><br />Bain triail eile as!',
'login'                      => 'Logáil isteach',
'nav-login-createaccount'    => 'Logáil isteach',
'loginprompt'                => 'Tá fianáin de dhíth chun logáil isteach a dhéanamh ag {{SITENAME}}.',
'userlogin'                  => 'Logáil isteach',
'logout'                     => 'Logáil amach',
'userlogout'                 => 'Logáil amach',
'notloggedin'                => 'Níl tú logáilte isteach',
'nologin'                    => 'Nach bhfuil logáil isteach agat? $1.',
'nologinlink'                => 'Cruthaigh cuntas',
'createaccount'              => 'Cruthaigh cuntas nua',
'gotaccount'                 => 'An bhfuil cuntas agat cheana féin? $1.',
'gotaccountlink'             => 'Logáil isteach',
'createaccountmail'          => 'le ríomhphost',
'badretype'                  => "D'iontráil tú dhá fhocal faire difriúla.",
'userexists'                 => "Tá an ainm úsáideora a d'iontráil tú á úsáid cheana féin.
Déan rogha d'ainm eile, más é do thoil é.",
'youremail'                  => 'Do ríomhphost *',
'username'                   => "D'ainm úsáideora:",
'uid'                        => 'D’uimhir úsáideora:',
'prefs-memberingroups'       => 'Comhalta {{PLURAL:$1|an ghrúpa|na ghrúpaí}}:',
'yourrealname'               => "D'fhíorainm **",
'yourlanguage'               => 'Teanga',
'yourvariant'                => 'Malairt',
'yournick'                   => 'Do leasainm (i síniuithe)',
'badsig'                     => 'Amhsíniú neamhbhailí; scrúdaigh na clibeanna HTML.',
'email'                      => 'Ríomhphost',
'prefs-help-realname'        => '* <strong>Fíorainm</strong> (roghnach): má toghaíonn tú é sin a chur ar fáil, úsáidfear é chun
do chuid dreachtaí a chur i leith tusa.',
'loginerror'                 => 'Earráid leis an logáil isteach',
'prefs-help-email'           => '<strong>Ríomhphost</strong> (roghnach): Leis an tréith seo is féidir teagmháil a dhéanamh leat tríd do leathanach úsáideora nó leathanach phlé gan do sheoladh ríomhphost a thaispeáint.',
'prefs-help-email-required'  => 'Ní foláir seoladh ríomhpoist a thabhairt.',
'nocookiesnew'               => "Cruthaíodh an cuntas úsáideora, ach níl tú logáilte isteach.

Úsáideann {{SITENAME}} fianáin chun úsáideoirí a logáil isteach. 
Tá fianáin díchumasaithe agat. 
Cumasaigh iad le do thoil, agus ansin logáil isteach le d'ainm úsáideora agus d'fhocal faire úrnua.",
'nocookieslogin'             => 'Úsáideann {{SITENAME}} fianáin chun úsáideoirí a logáil isteach. 
Tá fianáin díchumasaithe agat. 
Cumasaigh iad agus bain triail eile as, le do thoil.',
'noname'                     => 'Níor thug tú ainm úsáideora bailí.',
'loginsuccesstitle'          => 'Logáladh isteach thú',
'loginsuccess'               => "'''Tá tú logáilte isteach anois sa {{SITENAME}} mar \"<nowiki>\$1</nowiki>\".'''",
'nosuchuser'                 => 'Níl aon úsáideoir ann leis an ainm "$1".
Cinntigh do litriú, nó [[Special:Userlogin/signup|bain úsáid as an foirm thíos]] chun cuntas úsáideora nua a chruthú.',
'nosuchusershort'            => 'Níl aon úsáideoir ann leis an ainm "<nowiki>$1</nowiki>". Cinntigh do litriú.',
'nouserspecified'            => 'Caithfidh ainm úsáideoir a shonrú.',
'wrongpassword'              => "D'iontráil tú focal faire mícheart (nó ar iarraidh). Déan iarracht eile le do thoil.",
'wrongpasswordempty'         => 'Níor iontráil tú focal faire. Bain triail eile as.',
'passwordtooshort'           => "Tá d'fhocal faire ró-ghearr.
Caithfidh go bhfuil {{PLURAL:$1|1 carachtar|$1 carachtair}} carachtar ann ar a laghad.",
'mailmypassword'             => "Seol m'fhocal faire chugam.",
'passwordremindertitle'      => 'Cuimneachán an fhocail faire ó {{SITENAME}}',
'passwordremindertext'       => 'D\'iarr duine éigin (tusa de réir cosúlachta, ón seoladh IP $1)
go sheolfaimis focal faire {{GRAMMAR:genitive|{{SITENAME}}}} nua  ($4).
"$3" an focal faire don úsáideoir "$2" anois.
Ba chóir duit lógail isteach anois agus d\'fhocal faire a athrú.',
'noemail'                    => 'Níl aon seoladh ríomhphoist i gcuntas don úsáideoir "$1".',
'passwordsent'               => 'Cuireadh focal faire nua chuig an seoladh ríomhphoist atá cláraithe do "$1".
Nuair a gheobhaidh tú é, logáil isteach arís le do thoil.',
'eauthentsent'               => 'Cuireadh teachtaireacht ríomhphoist chuig an seoladh
chun fíordheimhniú a dhéanamh. Chun fíordheimhniú a dhéanamh gur leatsa an cuntas, caithfidh tú glac leis an teachtaireacht sin nó ní sheolfar aon rud eile chuig do chuntas.',
'mailerror'                  => 'Tharlaigh earráid leis an seoladh: $1',
'acct_creation_throttle_hit' => 'Gabh ár leithscéal, chruthaigh tú $1 cuntais cheana féin.
Ní féidir leat níos mó díobh a chruthú.',
'emailauthenticated'         => "D'fhíordheimhníodh do sheoladh ríomhphoist ar $1.",
'emailnotauthenticated'      => 'Ní dhearna fíordheimhniú ar do sheoladh ríomhphoist fós, agu díchumasaítear na hardtréithe ríomhphoist go dtí go fíordheimhneofaí é (d.c.f.).
Chun fíordheimhniú a dhéanamh, logáil isteach leis an focal faire neamhbhuan atá seolta chugat, nó iarr ar ceann nua ar an leathanach logála istigh.',
'emailconfirmlink'           => 'Deimhnigh do ríomhsheoladh',
'invalidemailaddress'        => 'Ní féidir an seoladh ríomhphoist a ghlacadh leis mar is dócha go bhfuil formáid neamhbhailí aige.
Iontráil seoladh dea-fhormáidte le do thoil, nó glan an réimse sin.',
'accountcreated'             => 'Cúntas cruthaithe',
'accountcreatedtext'         => 'Cruthaíodh cúntas úsáideora le haghaidh $1.',
'createaccount-title'        => 'Cuntas cruthú le {{SITENAME}}',
'createaccount-text'         => 'Chruthaigh duine éigin cuntas do do sheoladh ríomhphoist ar {{SITENAME}} ($4) leis an ainm "$2" agus pasfhocal "$3". Ba cheart duit logáil isteach agus do phasfhocal a athrú anois. Is féidir leat neamhaird a thabhairt don teachtaireacht seo má cruthaíodh trí earráid í.',
'loginlanguagelabel'         => 'Teanga: $1',

# Password reset dialog
'resetpass_text'   => '<!-- Cur téacs anseo -->',
'resetpass_header' => 'Athshocraigh an pasfhocail',

# Edit page toolbar
'bold_sample'     => 'Cló trom',
'bold_tip'        => 'Cló trom',
'italic_sample'   => 'Cló Iodáileach',
'italic_tip'      => 'Cló Iodáileach',
'link_sample'     => 'Ainm naisc',
'link_tip'        => 'Nasc inmhéanach',
'extlink_sample'  => 'http://www.example.com ainm naisc',
'extlink_tip'     => 'Nasc seachtrach (cuimhnigh an réimír http://)',
'headline_sample' => 'Cló ceannlíne',
'headline_tip'    => 'Ceannlíne Leibhéil 2',
'math_sample'     => 'Cuir foirmle isteach anseo',
'math_tip'        => 'Foirmle matamataice (LaTeX)',
'nowiki_sample'   => 'Cuir téacs neamh-fhormáide anseo',
'nowiki_tip'      => 'Scaoil thar ar fhormáid mearshuímh',
'image_sample'    => 'Sámpla.jpg',
'image_tip'       => 'Íomhá leabaithe',
'media_sample'    => 'Sámpla.mp3',
'media_tip'       => 'Nasc chuig comhad meáin',
'sig_tip'         => 'Do shíniú le stampa ama',
'hr_tip'          => 'Líne cothrománach (inúsáidte go coigilteach)',

# Edit pages
'summary'                => 'Achoimriú',
'subject'                => 'Ábhar/ceannlíne',
'minoredit'              => 'Is mionathrú é seo',
'watchthis'              => 'Déan faire ar an lch seo',
'savearticle'            => 'Sábháil an lch',
'preview'                => 'Réamhamharc',
'showpreview'            => 'Taispeáin réamhamharc',
'showlivepreview'        => 'Réamhamharc beo',
'showdiff'               => 'Taispeáin athruithe',
'anoneditwarning'        => "'''Rabhadh:''' Níl tú logáilte isteach. Cuirfear do sheoladh IP i stair eagarthóireachta an leathanaigh seo.",
'missingsummary'         => "'''Cuimhneachán:''' Níor thug tú achoimriú don athrú. Má chliceáileann tú Sábháil arís, sábhálfar an t-athrú gan é a hachoimriú.",
'summary-preview'        => 'Réamhamharc an achoimre',
'blockedtitle'           => 'Tá an úsáideoir seo faoi chosc',
'blockedtext'            => "<big>'''Chuir \$1 cosc ar d’ainm úsáideora nó ar do sheoladh IP.'''</big>

Is í seo an chúis a thugadh:<br />''\$2''.<p>Is féidir leat teagmháil a dhéanamh le \$1 nó le duine eile de na [[{{MediaWiki:Grouppage-sysop}}|riarthóirí]] chun an cosc a phléigh.

* Tús an chosc: \$8
* Dul as feidhm: \$6
* Sprioc an chosc: \$7
<br />
Tabhair faoi deara nach féidir leat an gné \"cuir ríomhphost chuig an úsáideoir seo\" a úsáid mura bhfuil seoladh ríomhphoist bailí cláraithe i do [[Special:Preferences|shocruithe úsáideora]]. 

Is é \$3 do sheoladh IP agus #\$5 do ID coisc. Déan tagairt don seoladh seo le gach ceist a chuirfeá.

==Nóta do úsáideoirí AOL==
De bhrí ghníomhartha leanúnacha creachadóireachta a dhéanann aon úsáideoir AOL áirithe, is minic a coisceann {{SITENAME}} ar friothálaithe AOL. Faraor, áfach, is féidir le 
go leor úsáídeoirí AOL an friothálaí céanna a úsáid, agus mar sin is minic a coiscaítear úsáideoirí AOL neamhchiontacha. Gabh ár leithscéal d'aon trioblóid. 

Dá dtarlódh an scéal seo duit, cuir ríomhphost chuig riarthóir le seoladh ríomhphoist AOL. Bheith cinnte tagairt a dhéanamh leis an seoladh IP seo thuas.",
'whitelistedittitle'     => 'Logáil isteach chun athrú a dhéanamh',
'whitelistedittext'      => 'Ní mór duit $1 chun ailt a athrú.',
'loginreqtitle'          => 'Tá logáil isteach de dhíth ort',
'loginreqlink'           => 'logáil isteach',
'loginreqpagetext'       => 'Caithfidh tú $1 chun leathanaigh a amharc.',
'accmailtitle'           => 'Seoladh an focal faire.',
'accmailtext'            => "Seoladh focal faire don úsáideoir '$1' go dtí '$2'.",
'newarticle'             => '(Nua)',
'newarticletext'         => "Lean tú nasc chuig leathanach nach bhfuil ann fós.
Chun an leathanach a chruthú, tosaigh ag clóscríobh sa bhosca thíos
(féach ar an [[{{MediaWiki:Helppage}}|leathanach cabhrach]] chun a thuilleadh eolais a fháil).
Má tháinig tú anseo as dearmad, brúigh an cnaipe '''ar ais''' ar do bhrabhsálaí.",
'anontalkpagetext'       => "---- ''Leathanach plé é seo a bhaineann le húsáideoir gan ainm nár chruthaigh cuntas fós, nó nach bhfuil ag úsáid an cuntas aige. Dá bhrí sin, caithfimid an seoladh IP a úsáid chun é/í a hionannú. Is féidir le níos mó ná úsáideoir amháin an seoladh IP céanna a úsáid. Má tá tú i d'úsáideoir gan ainm agus má tá sé do thuairim go rinneadh léiriuithe neamhfheidhmeacha fút, [[Special:Userlogin|cruthaigh cuntas]] nó [[Special:UserLogin|logáil isteach]] chun mearbhall le húsáideoirí eile gan ainmneacha a héalú amach anseo.''",
'noarticletext'          => 'Níl aon téacs ar an leathanach seo faoi láthair.  Is féidir [[Special:Search/{{PAGENAME}}|cuardach a dhéanamh le haghaidh an teidil seo]] i leathanaigh eile nó [{{fullurl:{{FULLPAGENAME}}|action=edit}} an leathanach seo a athrú].',
'clearyourcache'         => "'''Tugtar faoi deara:''' Tar éis duit an t-inneachar a shábháil, caithfear gabháil thar thaisce an bhrabhsálaí chun na hathruithe a fheiceáil.
'''Mozilla/Safari/Konqueror:''' cliceáil ar ''Athlódáil'', agus ''Iomlaoid'' á bhrú agat (nó brúigh ''Ctrl-Iomlaoid-R''), '''IE:''' brúigh ''Ctrl-F5'', '''Opera:''' brúigh ''F5''.",
'usercssjsyoucanpreview' => "<strong>Leid:</strong> Sula sábhálaím tú, úsáid an cnaipe
'Réamhamharc' chun do CSS/JS nua a tástáil.",
'usercsspreview'         => "'''Cuimhnigh nach bhfuil seo ach réamhamharc do CSS úsáideora -
níor sábháladh é go fóill!'''",
'userjspreview'          => "'''Cuimhnigh nach bhfuil seo ach réamhamharc do JavaScript úsáideora
- níor sábháladh é go fóill!'''",
'updated'                => '(Leasaithe)',
'note'                   => '<strong>Tabhair faoi deara:</strong>',
'previewnote'            => '<strong>Níl ann seo ach réamhamharc; 
níor sábháladh na hathruithe fós!</strong>',
'previewconflict'        => 'San réamhamharc seo, feachann tú an téacs dé réir an eagarbhosca
thuas mar a taispeáinfear é má sábháilfear é.',
'editing'                => 'Ag athrú $1',
'editingsection'         => 'Ag athrú $1 (mir)',
'editingcomment'         => 'Ag athrú $1 (tuairisc)',
'editconflict'           => 'Coimhlint athraithe: $1',
'explainconflict'        => 'D\'athraigh duine eile an leathanach seo ó shin a thosaigh tú ag athrú é.
Sa bhosca seo thuas feiceann tú téacs an leathanaigh mar atá sé faoi láthair.
Tá do chuid athruithe sa bhosca thíos.
Caithfidh tú do chuid athruithe a chumasadh leis an leagan láithreach.
Nuair a brúann tú ar an cnaipe "Sábháil an leathanach", ní shábhálfar aon rud <b>ach
amháin</b> an téacs sa bhosca thuas.',
'yourtext'               => 'Do chuid téacs',
'storedversion'          => 'Eagrán sábháilte',
'editingold'             => '<strong>AIRE: Tá tú ag athrú eagrán an leathanaigh atá as dáta.
Dá shábhálfá é, caillfear aon athrú a rinneadh ó shin an eagrán seo.</strong>',
'yourdiff'               => 'Difríochtaí',
'copyrightwarning'       => 'Tabhair faoi deara go dtuigtear go bhfuil gach dréacht do {{SITENAME}} eisithe faoi $2 (féach ar $1 le haghaidh tuilleadh eolais). 
Murar mian leat go gcuirfí do chuid scríbhinne in eagar go héadrócaireach agus go n-athdálfaí gan teorainn í, ná cuir isteach anseo í.<br /> 
Ina theannta sin, geallann tú gur scríobh tú féin an dréacht seo, nó gur chóipeáil tú é ó fhoinse san fhearann poiblí nó acmhainn eile saor ó chóipcheart (féach ar $1 le haghaidh tuilleadh eolais). 
<strong>NÁ CUIR ISTEACH OBAIR LE CÓIPCHEART GAN CHEAD!</strong>',
'copyrightwarning2'      => 'Tabhair faoi deara gur féidir le heagarthóirí eile gach dréacht do {{SITENAME}} a chur in eagar, a athrú agus a scriosadh. 
Murar mian leat go gcuirfí do chuid scríbhinne in eagar go héadrócaireach, ná cuir isteach anseo í.<br /> 
Ina theannta sin, geallann tú gur scríobh tú féin an dréacht seo, nó gur chóipeáil tú é ó fhoinse san fhearann poiblí nó acmhainn eile saor ó chóipcheart (féach ar $1 le haghaidh tuilleadh eolais). 
<strong>NÁ CUIR ISTEACH OBAIR LE CÓIPCHEART GAN CHEAD!</strong>',
'longpagewarning'        => 'AIRE: Tá an leathanach seo $1 cilibheart i bhfad; ní féidir le roinnt brabhsálaithe
leathanaigh a athrú má tá siad breis agus $1KiB, nó níos fada ná sin.
Más féidir, giotaigh an leathanach i gcodanna níos bige.',
'readonlywarning'        => "AIRE: Glasáladh an bunachar sonraí, agus mar sin
ní féidir leat do chuid athruithe a shábháil díreach anois. B'fhéidir gur mhaith leat an téacs a ghearr is
ghreamú i gcomhad téacs agus é a úsáid níos déanaí.",
'protectedpagewarning'   => '<strong>AIRE: Glasáladh an leathanach seo, agus ní féidir le duine ar bith é a athrú ach amhaín na húsáideoirí le pribhléidí oibreora córais. Bí cinnte go leanann tú na treoirlínte do leathanaigh glasáilte.</strong>',
'templatesused'          => 'Teimpléid in úsáid ar an lch seo:',
'templatesusedpreview'   => 'Teimpléid in úsáid sa réamhamharc alt seo:',
'templatesusedsection'   => 'Teimpléid in úsáid san alt seo:',
'template-protected'     => '(ghlasáil)',
'template-semiprotected' => '(leath-ghlasáil)',
'edittools'              => '<!-- Taispeánfar an téacs seo faoi foirmeacha eagarthóireachta agus uaslódála. -->',
'permissionserrors'      => 'Cead rochtana earráidí',

# Account creation failure
'cantcreateaccounttitle' => 'Ní féidir cuntas a chruthú',

# History pages
'viewpagelogs'        => 'Féach ar logaí faoin leathanach seo',
'nohistory'           => 'Níl aon stáir athraithe ag an leathanach seo.',
'revnotfound'         => 'Ní bhfuarthas an athrú',
'revnotfoundtext'     => "Ní bhfuarthas seaneagrán an leathanaigh a d'iarr tú ar.
Cinntigh an URL a d'úsáid tú chun an leathanach seo a rochtain.",
'currentrev'          => 'Leagan reatha',
'revisionasof'        => 'Leagan ó $1',
'revision-info'       => 'Leagan mar $1 le $2',
'previousrevision'    => '← An leasú roimhe seo',
'nextrevision'        => 'An chéad leasú eile →',
'currentrevisionlink' => 'Leagan reatha',
'cur'                 => 'rth',
'next'                => 'lns',
'last'                => 'rmh',
'page_first'          => 'Céad',
'page_last'           => 'deireanach',
'histlegend'          => 'Chun difríochtaí a roghnú, marcáil na cnaipíní de na heagráin atá tú ag iarraidh comparáid a dhéanamh astu, agus brúigh Iontráil nó an cnaipe ag bun an leathanaigh.<br />
Treoir: (rth) = difríocht ón leagan reatha, (rmh) = difríocht ón leagan roimhe, <b>m</b> = mionathrú.',
'deletedrev'          => '[scriosta]',
'histfirst'           => 'An ceann is luaithe',
'histlast'            => 'An ceann is déanaí',
'historysize'         => '({{PLURAL:$1|1 beart|$1 bheart}})',
'historyempty'        => '(folamh)',

# Revision feed
'history-feed-title'          => 'Stáir leasú',
'history-feed-item-nocomment' => '$1 ag $2', # user at time

# Revision deletion
'rev-deleted-user' => '(ainm úsáideora dealaithe)',
'rev-delundel'     => 'taispeáin/folaigh',
'revdelete-uname'  => 'ainm úsáideora',

# Diffs
'history-title'           => 'Stair leasú "$1"',
'difference'              => '(Difríochtaí idir leaganacha)',
'lineno'                  => 'Líne $1:',
'compareselectedversions' => 'Cuir na leagain roghnaithe i gcomparáid',
'editundo'                => 'cealaigh',
'diff-multi'              => '({{PLURAL:$1|Leasú idirmheánach amháin|$1 leasú idirmheánach}} nach thaispeántar.)',

# Search results
'searchresults'         => 'Torthaí an chuardaigh',
'searchresulttext'      => 'Féach ar [[{{MediaWiki:Helppage}}|{{int:help}}]] chun a thuilleadh eolais a fháil maidir le cuardaigh {{GRAMMAR:genitive|{{SITENAME}}}}.',
'searchsubtitle'        => 'Don iarratas "[[:$1]]"',
'searchsubtitleinvalid' => 'Don iarratas "$1"',
'noexactmatch'          => "'''Níl aon leathanach ann leis an teideal \"\$1\".''' Is féidir leat é a [[:\$1|cruthú]].",
'titlematches'          => 'Tá macasamhla teidil alt ann',
'notitlematches'        => 'Níl macasamhla teidil alt ann',
'textmatches'           => 'Tá macasamhla téacs alt ann',
'notextmatches'         => 'Níl macasamhla téacs alt ann',
'prevn'                 => 'na $1 roimhe',
'nextn'                 => 'an chéad $1 eile',
'viewprevnext'          => 'Taispeáin ($1) ($2) ($3).',
'showingresults'        => "Ag taispeáint thíos {{PLURAL:$1|'''toradh amháin'''|'''$1''' torthaí}}, ag tosú le #'''$2'''.",
'showingresultsnum'     => "Ag taispeáint thíos {{PLURAL:$3|'''toradh amháin'''|'''$3''' torthaí}}, ag tosú le #'''$2'''.",
'nonefound'             => '<strong>Tabhair faoi deara</strong>: go minic, ní éiríonn cuardaigh nuair a cuardaítear focail an-coiteanta, m.sh., "ag" is "an",
a nach bhfuil innéacsaítear, nó nuair a ceisteann tú níos mó ná téarma amháin (ní
taispeáintear sna toraidh ach na leathanaigh ina bhfuil go leoir na téarmaí cuardaigh).',
'powersearch'           => 'Cuardaigh',
'searchdisabled'        => "Tá brón orainn! Mhíchumasaíodh an cuardach téacs iomlán go sealadach chun luas an tsuímh a chosaint. Idir an dá linn, is féidir leat an cuardach Google anseo thíos a úsáid - b'fhéidir go bhfuil sé as dáta.",

# Preferences page
'preferences'              => 'Sainroghanna',
'mypreferences'            => 'Mo shainroghanna',
'prefsnologin'             => 'Níl tú logáilte isteach',
'prefsnologintext'         => 'Ní mór duit <span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} logáil isteach]</span> chun do chuid sainroghanna phearsanta a shocrú.',
'prefsreset'               => "D'athraíodh do chuid sainroghanna ar ais chuig an leagan bunúsach ón stóras.",
'qbsettings'               => 'Sainroghanna an bosca uirlisí',
'qbsettings-none'          => 'Faic',
'qbsettings-fixedleft'     => 'Greamaithe ar chlé',
'qbsettings-fixedright'    => 'Greamaithe ar dheis',
'qbsettings-floatingleft'  => 'Ag faoileáil ar chlé',
'qbsettings-floatingright' => 'Ag faoileáil ar dheis',
'changepassword'           => "Athraigh d'fhocal faire",
'skin'                     => 'Craiceann',
'math'                     => 'Ag aistriú na matamaitice',
'dateformat'               => 'Formáid dáta',
'datedefault'              => 'Is cuma liom',
'datetime'                 => 'Dáta agus am',
'math_failure'             => 'Theip anailís an fhoirmle',
'math_unknown_error'       => 'earráid anaithnid',
'math_unknown_function'    => 'foirmle anaithnid',
'math_lexing_error'        => 'Theipeadh anailís an fhoclóra',
'math_syntax_error'        => 'earráid comhréire',
'math_image_error'         => 'Theipeadh aistriú an PNG; tástáil má tá na ríomhchláir latex, dvips, gs, agus convert
i suite go maith.',
'math_bad_tmpdir'          => 'Ní féidir scríobh chuig an fillteán mata sealadach, nó é a chruthú',
'math_bad_output'          => 'Ní féidir scríobh chuig an fillteán mata aschomhaid, nó é a chruthú',
'math_notexvc'             => 'Níl an ríomhchlár texvc ann; féach ar mata/EOLAIS chun é a sainathrú.',
'prefs-personal'           => 'Sonraí úsáideora',
'prefs-rc'                 => 'Athruithe le déanaí agus taispeántas stumpaí',
'prefs-watchlist'          => 'Liosta faire',
'prefs-watchlist-days'     => 'Líon na laethanta le taispeáint sa liosta faire:',
'prefs-watchlist-edits'    => 'Líon na n-athruithe le taispeáint sa liosta leathnaithe faire:',
'prefs-misc'               => 'Sainroghanna éagsúla',
'saveprefs'                => 'Sábháil sainroghanna',
'resetprefs'               => 'Athshuigh sainroghanna',
'oldpassword'              => 'Focal faire reatha:',
'newpassword'              => 'Focal faire nua:',
'retypenew'                => 'Athscríobh an focal faire nua:',
'textboxsize'              => 'Eagarthóireacht',
'rows'                     => 'Sraitheanna',
'columns'                  => 'Colúin',
'searchresultshead'        => 'Sainroghanna do toraidh cuardaigh',
'resultsperpage'           => 'Cuairt le taispeáint ar gach leathanach',
'contextlines'             => 'Línte le taispeáint do gach cuairt',
'contextchars'             => 'Litreacha chomhthéacs ar gach líne',
'recentchangescount'       => 'Méid teideal sna hathruithe le déanaí',
'savedprefs'               => 'Sábháladh do chuid sainroghanna.',
'timezonelegend'           => 'Crios ama',
'timezonetext'             => 'Iontráil an méid uaireanta a difríonn do am áitiúil
den am an freastalaí (UTC).',
'localtime'                => 'An t-am áitiúil',
'timezoneoffset'           => 'Difear',
'servertime'               => 'Am an freastalaí anois',
'guesstimezone'            => 'Líon ón líonléitheoir',
'allowemail'               => "Tabhair cead d'úsáideoirí eile ríomhphost a sheoladh chugat.",
'defaultns'                => 'Cuardaigh sna ranna seo a los éagmaise:',
'default'                  => 'réamhshocrú',
'files'                    => 'Comhaid',

# User rights
'userrights'               => 'Bainistíocht cearta úsáideora', # Not used as normal message but as header for the special page itself
'userrights-user-editname' => 'Iontráil ainm úsáideora:',
'editusergroup'            => 'Cuir Grúpái Úsáideoirí In Eagar',
'editinguser'              => "Ag athrú ceart don úsáideoir '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Cuir grúpaí na n-úsáideoirí in eagar',
'saveusergroups'           => 'Sábháil Grúpaí na n-Úsáideoirí',
'userrights-groupsmember'  => 'Ball de:',

# Groups
'group'     => 'Grúpa:',
'group-bot' => 'Robónna',
'group-all' => '(an t-iomlán)',

'grouppage-sysop' => '{{ns:project}}:Riarthóirí',

# User rights log
'rightslog' => 'Log cearta úsáideoira',

# Recent changes
'nchanges'                          => '$1 {{PLURAL:$1|athrú amháin|athruithe}}',
'recentchanges'                     => 'Athruithe is déanaí',
'recentchangestext'                 => 'Déan faire ar na hathruithe is déanaí sa vicí ar an leathanach seo.',
'rcnote'                            => "Is {{PLURAL:$1|é seo a leanas <strong>an t-athrú amháin</strong>|iad seo a leanas na <strong>$1</strong> athruithe is déanaí}} {{PLURAL:$2|ar feadh an lae dheireanaigh|ar feadh na '''$2''' lá deireanacha}}, as $5, $4.",
'rcnotefrom'                        => 'Is iad seo a leanas na hathruithe ó <b>$2</b> (go dti <b>$1</b> taispeánaithe).',
'rclistfrom'                        => 'Taispeáin nua-athruithe dom ó $1 anuas',
'rcshowhideminor'                   => '$1 mionathruithe',
'rcshowhidebots'                    => '$1 róbónna',
'rcshowhideliu'                     => '$1 úsáideoirí atá logáilte isteach',
'rcshowhideanons'                   => '$1 úsáideoirí gan ainm',
'rcshowhidepatr'                    => '$1 athruithe faoi phatról',
'rcshowhidemine'                    => '$1 mo chuid athruithe',
'rclinks'                           => 'Taispeáin na $1 athruithe is déanaí sna $2 laethanta seo caite<br />$3 mionathruithe',
'diff'                              => 'difr',
'hist'                              => 'stair',
'hide'                              => 'Folaigh',
'show'                              => 'taispeán',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'r',
'number_of_watching_users_pageview' => '[{{PLURAL:$1|úsáideoir amháin|$1 úsáideoirí}} ag faire]',
'rc_categories_any'                 => 'Aon chatagóir',
'newsectionsummary'                 => '/* $1 */ mír nua',

# Recent changes linked
'recentchangeslinked'       => 'Athruithe gaolmhara',
'recentchangeslinked-title' => 'Athruithe gaolmhar le "$1"',

# Upload
'upload'            => 'Uaslódáil comhad',
'uploadbtn'         => 'Uaslódáil comhad',
'reupload'          => 'Athuaslódáil',
'reuploaddesc'      => 'Dul ar ais chuig an fhoirm uaslódála.',
'uploadnologin'     => 'Nil tú logáilte isteach',
'uploadnologintext' => 'Ní mór duit [[Special:UserLogin|logáil isteach]] chun comhaid a huaslódáil.',
'uploaderror'       => 'Earráid uaslódála',
'uploadtext'        => "Bain úsáid as an bhfoirm thíos chun comhaid a uaslódáil.
Chun comhaid atá ann cheana a fheiceáil nó a chuardach téigh chuig an [[Special:ImageList|liosta comhad uaslódáilte]]. Gheobhaidh tú liosta de chomhaid uaslódáilte sa [[Special:Log/upload|loga uaslódála]] agus liosta de chomhaid scriosta sa [[Special:Log/delete|loga scriosta]] freisin.

Chun comhad a úsáid ar leathanach, cuir isteach nasc mar seo:
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:comhad.jpg]]</nowiki></tt>''' chun leagan iomlán an chomhad a úsáid
* '''<tt><nowiki>[[</nowiki>{{ns:image}}<nowiki>:comhad.png|200px|thumb|left|téacs eile]]</nowiki></tt>''' chun comhad le 200 picteillín ar leithead i mbosca san imeall clé le 'téacs eile' mar tuairisc
* '''<tt><nowiki>[[</nowiki>{{ns:media}}<nowiki>:comhad.ogg]]</nowiki></tt>''' más comhad fuaime atá i gceist",
'uploadlog'         => 'Stair uaslódála',
'uploadlogpage'     => 'Stair_uaslódála',
'uploadlogpagetext' => 'Is liosta é seo a leanas de na uaslódáil comhad is deanaí.
Is am an freastalaí iad na hamanna atá anseo thíos.',
'filename'          => 'Comhadainm',
'filedesc'          => 'Achoimriú',
'fileuploadsummary' => 'Achoimre:',
'filestatus'        => 'Stádas cóipchirt:',
'filesource'        => 'Foinse:',
'uploadedfiles'     => 'Comhaid uaslódáilte',
'illegalfilename'   => 'Tá litreacha san comhadainm  "$1" nach ceadaítear in ainm leathanaigh. Athainmnigh
an comhad agus déan athiarracht, más é do thoil é.',
'badfilename'       => 'D\'athraíodh an ainm íomhá bheith "$1".',
'emptyfile'         => "De réir a chuma, ní aon rud san chomhad a d'uaslódáil tú ach comhad folamh. Is dócha gur
míchruinneas é seo san ainm chomhaid. Seiceáil más é an comhad seo atá le huaslódáil agat.",
'successfulupload'  => "D'éirigh leis an uaslódáil",
'uploadwarning'     => 'Rabhadh suaslódála',
'savefile'          => 'Sábháil comhad',
'uploadedimage'     => 'D\'uaslódáladh "$1"',
'uploaddisabled'    => 'Tá brón orainn, díchumasaítear an córas uaslódála faoi láthair.',
'uploadcorrupt'     => 'Tá an comhad truaillithe nó tá iarmhír comhadainm neamhbhailí aige. Scrúdaigh an comhad agus
uaslódáil é arís, le do thoil.',
'uploadvirus'       => 'Tá víreas ann sa comhad seo! Eolas: $1',
'sourcefilename'    => 'Comhadainm foinse:',
'destfilename'      => 'Comhadainm sprice:',
'watchthisupload'   => 'Déan faire ar an leathanach seo',

'nolicense'          => 'Níl aon cheann roghnaithe',
'upload_source_url'  => ' (URL bailí is féidir a rochtain go poiblí)',
'upload_source_file' => ' (comhad ar do riomhaire)',

# Special:ImageList
'imagelist'      => 'Liosta íomhánna',
'imagelist_user' => 'Úsáideoir',

# Image description page
'filehist'                  => 'Stair comhad',
'filehist-current'          => 'reatha',
'filehist-datetime'         => 'Dáta/Am',
'filehist-user'             => 'Úsáideoir',
'filehist-dimensions'       => 'Toisí',
'filehist-filesize'         => 'Méid an comhad',
'filehist-comment'          => 'Nóta tráchta',
'imagelinks'                => 'Naisc íomhá',
'linkstoimage'              => 'Tá nasc chuig an gcomhad seo ar {{PLURAL:$1|na leathanaigh|$1 an leathanach}} seo a leanas:',
'nolinkstoimage'            => 'Níl leathanach ar bith ann a bhfuil nasc chuig an gcomhad seo air.',
'sharedupload'              => 'Is uaslodáil roinnte atá ann sa comhad seo, agus is féidir le tionscadail eile é a úsáid.',
'shareduploadwiki'          => 'Féach ar an [leathanach cur síos don comhad $1] le tuilleadh eolais.',
'noimage'                   => 'Níl aon chomhad ann leis an ainm seo, ba féidir leat $1',
'noimage-linktext'          => 'uaslódaigh ceann',
'uploadnewversion-linktext' => 'Uaslódáil leagan nua den comhad seo',

# File deletion
'filedelete'        => 'Scrios $1',
'filedelete-submit' => 'Scrios',

# MIME search
'mimesearch' => 'cuardaigh MIME',
'download'   => 'íoslódáil',

# Unwatched pages
'unwatchedpages' => 'Leathanaigh gan faire',

# List redirects
'listredirects' => 'Liostaigh na athsheolaí',

# Unused templates
'unusedtemplates' => 'Teimpléid gan úsáid',

# Random page
'randompage'         => 'Leathanach fánach',
'randompage-nopages' => 'Níl aon leathanaigh san ainmspás seo.',

# Random redirect
'randomredirect' => 'Atreorú randamach',

# Statistics
'statistics'    => 'Staidreamh',
'sitestats'     => 'Staidreamh do {{SITENAME}}',
'userstats'     => 'Staidreamh úsáideora',
'sitestatstext' => "Tá {{PLURAL:$1|'''leathanach amháin''|'''$1''' leathanaigh san iomlán}} sa bhunachar sonraí.
Cuirtear na leathanaigh seo san áireamh: leathanaigh phlé, leathanaigh {{GRAMMAR:genitive|{{SITENAME}}}}, leathanaigh ghearra (síolta), athsheoltaí agus leathaigh eile nach meastar mar leathanaigh inneachair iad i ndáiríre.
Ag fágáil na leathanach sin ar lár, tá {{PLURAL:$2|'''leathanach amháin''' ann atá ina inneachar|'''$2''' leathanaigh ann atá ina n-inneachar}} ceart de réir cosúlachta.

D'uaslódáladh {{PLURAL:$8|'''comhad amháin'''|'''$8''' comhaid}}.

Ina iomlán, tharla {{PLURAL:$3|'''radhairc leathanaigh amháin'''|'''$3''' radhairc leathanaigh}} agus {{PLURAL:$4|'''athrú leathanaigh amháin'''|'''$4''' athruithe leathanaigh}} ó bunaíodh {{SITENAME}}.
Is é sin '''$5''' athruithe ar meán do gach leathanach, agus '''$6''' radhairc do gach athrú.

Fad an [http://www.mediawiki.org/wiki/Manual:Job_queue scuaine jabanna]: '''$7'''.",
'userstatstext' => "Tá {{PLURAL:$1|'''[[Special:ListUsers|úsáideoir]] cláraithe amháin'''|'''$1''' [[Special:ListUsers|úsáideoirí]] cláraithe}} anseo agus tá {{PLURAL:$2|'''duine amháin'''|'''$2'''}} (nó '''$4%''') díobh leis na cearta $5.",

'disambiguations'     => 'Leathanaigh idirdhealaithe',
'disambiguationspage' => '{{ns:project}}:Naisc_go_leathanaigh_idirdhealaithe',

'doubleredirects'     => 'Athsheolaidh dúbailte',
'doubleredirectstext' => '<b>Tabhair faoi deara:</b> B\'fheidir go bhfuil toraidh bréagacha ar an liosta seo.
De ghnáth cíallaíonn sé sin go bhfuil téacs breise le naisc thíos sa chéad #REDIRECT no #ATHSHEOLADH.<br />
 Sa
gach sraith tá náisc chuig an chéad is an dara athsheoladh, chomh maith le chéad líne an dara téacs athsheolaidh. De
ghnáth tugann sé sin an sprioc-alt "fíor".',

'brokenredirects'        => 'Atreoruithe briste',
'brokenredirectstext'    => 'Is iad na athsheolaidh seo a leanas a nascaíonn go ailt nach bhfuil ann fós.',
'brokenredirects-edit'   => '(athraigh)',
'brokenredirects-delete' => '(scrios)',

'withoutinterwiki' => 'Leathanaigh gan naisc idirvicí',

'fewestrevisions' => 'Leathanaigh leis na leasaithe is lú',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|bheart amháin|bearta}}',
'ncategories'             => '$1 {{PLURAL:$1|chatagóir amháin|catagóirí}}',
'nlinks'                  => '{{PLURAL:$1|nasc amháin|$1 naisc}}',
'nmembers'                => '{{PLURAL:$1|ball amháin|$1 baill}}',
'nviews'                  => '{{PLURAL:$1|radharc amháin|$1 radhairc}}',
'lonelypages'             => 'Leathanaigh aonair',
'uncategorizedpages'      => 'Leathanaigh gan catagóir',
'uncategorizedcategories' => 'Catagóirí gan catagórú',
'uncategorizedimages'     => 'Íomhánna gan chatagóir',
'uncategorizedtemplates'  => 'Teimpléid gan catagóir',
'unusedcategories'        => 'Catagóirí nach úsáidtear',
'unusedimages'            => 'Íomhánna nach úsáidtear',
'popularpages'            => 'Leathanaigh coitianta',
'wantedcategories'        => 'Catagóirí agus iarraidh ag gabháil leis',
'wantedpages'             => 'Leathanaigh de dhíth',
'mostcategories'          => 'Leathanaigh leis na chatagóir is mó',
'mostrevisions'           => 'Leathanaigh leis na leasaithe is mó',
'prefixindex'             => 'Innéacs réimír',
'shortpages'              => 'Leathanaigh gearra',
'longpages'               => 'Leathanaigh fada',
'deadendpages'            => 'Leathanaigh caocha',
'protectedpages'          => 'Leathanaigh cosanta',
'listusers'               => 'Liosta úsáideoirí',
'newpages'                => 'Leathanaigh nua',
'newpages-username'       => 'Ainm úsáideora:',
'ancientpages'            => 'Na leathanaigh is sine',
'move'                    => 'Athainmnigh',
'movethispage'            => 'Athainmnigh an leathanach seo',
'unusedimagestext'        => '<p>Tabhair faoi deara gur féidir le shuímh
eile naisc a dhéanamh leis an íomha le URL díreach,
agus mar sin bheadh siad ar an liosta seo fós cé go bhfuil siad
in úsáid faoi láthair.',
'unusedcategoriestext'    => 'Tá na leathanaigh catagóire seo a leanas ann, cé nach úsáidtear
iad in aon alt eile nó in aon chatagóir eile.',
'notargettitle'           => 'Níl aon cuspóir ann',
'notargettext'            => 'Níor thug tú leathanach nó úsáideoir sprice
chun an gníomh seo a dhéanamh ar.',

# Book sources
'booksources'               => 'Leabharfhoinsí',
'booksources-search-legend' => 'Cuardaigh le foinsí leabhar',

# Special:Log
'specialloguserlabel'  => 'Úsáideoir:',
'speciallogtitlelabel' => 'Teideal:',
'log'                  => 'Logaí',
'all-logs-page'        => 'Gach logaí',
'alllogstext'          => 'Taispeántas comhcheangaltha de logaí as {{SITENAME}} a bhaineann le huaslódáil, scriosadh, glasáil, coisc,
agus oibreoirí córais. Is féidir leat an taispeántas a ghéarú - roghnaigh an saghas loga, an ainm úsáideora, nó an
leathanach atá i gceist agat.',

# Special:AllPages
'allpages'          => 'Gach leathanach',
'alphaindexline'    => '$1 go $2',
'nextpage'          => 'An leathanach a leanas ($1)',
'prevpage'          => 'Leathanach roimhe sin ($1)',
'allpagesfrom'      => 'Taispeáin leathanaigh, le tosú ag:',
'allarticles'       => 'Gach alt',
'allinnamespace'    => 'Gach leathanach (ainmspás $1)',
'allnotinnamespace' => 'Gach leathanach (lasmuigh den ainmspás $1)',
'allpagesprev'      => 'Siar',
'allpagesnext'      => 'Ar aghaidh',
'allpagessubmit'    => 'Gabh',
'allpagesprefix'    => 'Taispeáin leathanaigh leis an réimír:',
'allpages-bad-ns'   => 'Níl an ainmspás "$1" ar {{SITENAME}}',

# Special:Categories
'categories'         => 'Catagóirí',
'categoriespagetext' => 'Tá na catagóiri seo a leanas ann sa vicí.
Níl na [[Special:UnusedCategories|catagóiri gan úsáid]] ar fáil anseo.
Féach freisin ar [[Special:WantedCategories|catagóirí agus iarraidh ag gabháil leis]].',

# E-mail user
'mailnologin'     => 'Níl aon seoladh maith ann',
'mailnologintext' => 'Ní mór duit bheith  [[Special:UserLogin|logáilte isteach]]
agus bheith le seoladh ríomhphoist bhailí i do chuid [[Special:Preferences|sainroghanna]]
más mian leat ríomhphost a sheoladh chuig úsáideoirí eile.',
'emailuser'       => 'Cuir ríomhphost chuig an úsáideoir seo',
'emailpage'       => 'Seol ríomhphost',
'emailpagetext'   => 'Má d\'iontráil an úsáideoir seo seoladh ríomhphoist bhailí
ina chuid sainroghanna úsáideora, cuirfidh an foirm anseo thíos teachtaireacht amháin do.
Beidh do seoladh ríomhphoist a d\'iontráil tú i do chuid sainroghanna úsáideora
sa bhosca "Seoltóir" an riomhphoist, agus mar sin ba féidir léis an faighteoir ríomhphost eile a chur leatsa.',
'usermailererror' => 'Earráid leis an píosa ríomhphoist:',
'defemailsubject' => 'Ríomhphost {{GRAMMAR:genitive|{{SITENAME}}}}',
'noemailtitle'    => 'Níl aon seoladh ríomhphoist ann',
'noemailtext'     => 'Níor thug an úsáideoir seo seoladh ríomhphoist bhailí, nó shocraigh sé nach
mian leis ríomhphost a fháil ón úsáideoirí eile.',
'emailfrom'       => 'Seoltóir:',
'emailto'         => 'Chuig:',
'emailsubject'    => 'Ábhar:',
'emailmessage'    => 'Teachtaireacht:',
'emailsend'       => 'Seol',
'emailsent'       => 'Ríomhphost seolta',
'emailsenttext'   => 'Seoladh do theachtaireacht ríomhphoist go ráthúil.',

# Watchlist
'watchlist'            => 'Mo liosta faire',
'mywatchlist'          => 'Mo liosta faire',
'watchlistfor'         => "(do '''$1''')",
'nowatchlist'          => 'Níl aon rud ar do liosta faire.',
'watchnologin'         => 'Níl tú logáilte isteach',
'addedwatch'           => 'Curtha ar an liosta faire',
'addedwatchtext'       => "Cuireadh an leathanach \"<nowiki>\$1</nowiki>\" le do [[Special:Watchlist|liosta faire]].
Amach anseo liostálfar athruithe don leathanach seo agus dá leathanach plé ansin,
agus beidh '''cló trom''' ar a theideal san [[Special:RecentChanges|liosta de na hathruithe is déanaí]] sa chaoi go bhfeicfeá iad go héasca.",
'removedwatch'         => 'Bainte den liosta faire',
'removedwatchtext'     => 'Baineadh an leathanach "<nowiki>$1</nowiki>" as [[Special:Watchlist|do liosta faire]].',
'watch'                => 'Fair',
'watchthispage'        => 'Déan faire ar an leathanach seo',
'unwatch'              => 'Ná fair',
'unwatchthispage'      => 'Ná fair fós',
'notanarticle'         => 'Níl alt ann',
'watchnochange'        => 'Níor athraíodh ceann ar bith de na leathanaigh atá ar do liosta faire,
taobh istigh den tréimhse atá roghnaithe agat.',
'watchlist-details'    => 'Tá tú ag faire ar {{PLURAL:$1|leathanach amháin|$1 leathanaigh}}, gan leathanaigh phlé a chur san áireamh.',
'wlheader-enotif'      => '* Cumasaíodh fógraí riomhphoist.',
'wlheader-showupdated' => "* Tá '''cló trom''' ar leathanaigh a athraíodh ón uair is deireanaí a d'fhéach tú orthu.",
'watchmethod-recent'   => 'ag seiceáil na athruithe deireanacha ar do chuid leathanaigh faire',
'watchmethod-list'     => 'ag seiceáil na leathanaigh faire ar do chuid athruithe deireanacha',
'watchlistcontains'    => 'Tá {{PLURAL:$1|leathanach amháin|$1 leathanaigh}} ar do liosta faire.',
'iteminvalidname'      => "Fadhb leis an mír '$1', ainm neamhbhailí...",
'wlnote'               => "Is {{PLURAL:$1|é seo thíos an t-athrú deireanach|iad seo thíos na '''$1''' athruithe deireanacha}} {{PLURAL:$2|san uair deireanach|sna '''$2''' uaire deireanacha}}.",
'wlshowlast'           => 'Taispeáin an $1 uair $2 lá seo caite$3',
'watchlist-hide-bots'  => 'Folaigh athruithe róbó',
'watchlist-hide-own'   => 'Folaigh mo chuid athruithe',
'watchlist-hide-minor' => 'Folaigh mionathruithe',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Á chur le do liosta faire...',
'unwatching' => 'Á bhaint de do liosta faire...',

'enotif_mailer'      => 'Fógrasheoltóir as {{SITENAME}}',
'enotif_reset'       => 'Marcáil gach leathanach bheith tadhlaithe',
'enotif_newpagetext' => 'Is leathanach nua é seo.',
'changed'            => "D'athraigh",
'created'            => 'Cruthaigh',
'enotif_subject'     => '  $CHANGEDORCREATED $PAGEEDITOR an leathanach $PAGETITLE ag {{SITENAME}}.',
'enotif_lastvisited' => 'Féach ar $1 le haghaidh gach athrú a rinneadh ó thús na cuairte seo caite a rinne tú.',
'enotif_body'        => 'A $WATCHINGUSERNAME, a chara,

$CHANGEDORCREATED $PAGEEDITOR an leathanach $PAGETITLE  ag {{SITENAME}} ar $PAGEEDITDATE, féach ar $PAGETITLE_URL chun an leagan reatha a fháil.

$NEWPAGE

Athchoimriú an úsáideora a rinne é: $PAGESUMMARY $PAGEMINOREDIT

Sonraí teagmhála an úsáideora:
r-phost: $PAGEEDITOR_EMAIL
vicí: $PAGEEDITOR_WIKI

I gcás athruithe eile, ní bheidh aon fhógra eile muna dtéann tú go dtí an leathanach seo. Is féidir freisin na bratacha fógartha a athrú do gach leathanach ar do liosta faire.

	     Is mise le meas,
	     Fógrachóras cairdiúil {{GRAMMAR:genitive|{{SITENAME}}}}

--
Chun socruithe do liosta faire a athrú, tabhair cuairt ar
{{fullurl:Special:Watchlist/edit}}

Aiseolas agus a thuilleadh cabhrach:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => 'Scrios an leathanach',
'confirm'                     => 'Cinntigh',
'excontent'                   => "téacs an lgh: '$1'",
'excontentauthor'             => "seo a bhí an t-inneachar: '$1' (agus ba é '[[Special:Contributions/$2|$2]]' an t-aon dhréachtóir)",
'exbeforeblank'               => "is é seo a raibh an ábhar roimh an folmhadh: '$1'",
'exblank'                     => 'bhí an leathanach folamh',
'delete-confirm'              => 'Scrios "$1"',
'historywarning'              => 'Aire: Ta stair ag an leathanach a bhfuil tú ar tí é a scriosadh:',
'confirmdeletetext'           => 'Tá tú ar tí leathanach, agus a chuid staire, a scriosadh.
Deimhnigh, le do thoil, gur mhian leat é seo a dhéanamh, go dtuigeann tú torthaí an ghnímh seo agus go bhfuil tú dá dhéanamh de réir [[{{MediaWiki:Policy-url}}|an pholasaí]].',
'actioncomplete'              => 'Gníomh críochnaithe',
'deletedtext'                 => 'scriosadh "<nowiki>$1</nowiki>".
Féach ar $2 chun cuntas na scriosiadh deireanacha a fháil.',
'deletedarticle'              => 'scriosadh "[[$1]]"',
'dellogpage'                  => 'Loga scriosta',
'dellogpagetext'              => 'Seo é liosta de na scriosaidh is déanaí.',
'deletionlog'                 => 'cuntas scriosaidh',
'reverted'                    => 'Tá eagrán níos luaithe in úsáid anois',
'deletecomment'               => 'Cúis don scriosadh',
'deleteotherreason'           => 'Fáth eile/breise:',
'deletereasonotherlist'       => 'Fáth eile',
'deletereason-dropdown'       => '*Fáthanna coitianta scriosta
** Iarratas ón údar
** Sárú cóipchirt
** Loitiméireacht',
'rollback'                    => 'Athúsáid seanathruithe',
'rollback_short'              => 'Roll siar',
'rollbacklink'                => 'athúsáid',
'rollbackfailed'              => 'Theip an athúsáid',
'cantrollback'                => 'Ní féidir an athrú a athúsáid; ba é údar an ailt an t-aon duine a rinne athrú dó.',
'alreadyrolled'               => "Ní féidir eagrán níos luaí an leathanaigh [[:$1]]
le [[User:$2|$2]] ([[User talk:$2|Plé]]) a athúsáid; d'athraigh duine eile é cheana fein, nó
d'athúsáid duine eile eagrán níos luaí cheana féin.

[[User:$3|$3]] ([[User talk:$3|Plé]]) an té a rinne an athrú is déanaí.",
'editcomment'                 => 'Seo a raibh an mínithe athraithe: "<i>$1</i>".', # only shown if there is an edit comment
'revertpage'                  => "Filleadh eagarthóireachtaí le [[Special:Contributions/$2|$2]] ([[User talk:$2|Plé]]); d'athúsáideadh an athrú seo caite le [[User:$1|$1]]", # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'protectlogpage'              => 'Cuntas_cosanta',
'protectlogtext'              => 'Seo é liosta de glais a cuireadh ar / baineadh de leathanaigh.
Féach ar [[Special:ProtectedPages|Leathanach glasáilte]] chun a thuilleadh eolais a fháil.',
'protectedarticle'            => 'glasáladh "[[$1]]"',
'unprotectedarticle'          => 'díghlasáladh "[[$1]]"',
'protect-title'               => 'Ag glasáil "$1"',
'protect-legend'              => 'Cinntigh an glasáil',
'protectcomment'              => 'Cúis don glasáil',
'protectexpiry'               => 'As feidhm:',
'protect_expiry_invalid'      => 'Am éaga neamhbhailí.',
'protect_expiry_old'          => 'Am éaga san am atá thart.',
'protect-unchain'             => 'Díghlasáil an cead athainmithe',
'protect-text'                => 'Is féidir leat an leibhéal glasála a athrú anseo don leathanach <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-access'       => 'Ní chead ag do chuntas chun athraigh leibhéal cosaint an leathanach.
Seo iad na socruithe reatha faoin leathanach <strong>$1</strong>:',
'protect-default'             => '(réamhshocrú)',
'protect-fallback'            => 'Ceadúnas "$1" riachtanach',
'protect-level-autoconfirmed' => 'Bac úsáideoirí neamhchláraithe',
'protect-level-sysop'         => 'Oibreoirí chórais amháin',
'protect-summary-cascade'     => 'cascáidithe',
'protect-expiring'            => 'as feidhm $1 (UTC)',
'protect-cascade'             => 'Coisc leathanaigh san áireamh an leathanach seo (cosanta cascáideach)',
'protect-cantedit'            => 'Ní féidir leat na leibhéil cosanta a athrú faoin leathanach seo, mar níl cead agat é a cur in eagar.',
'restriction-type'            => 'Ceadúnas:',
'restriction-level'           => 'Leibhéal srianadh:',
'pagesize'                    => '(bearta)',

# Restrictions (nouns)
'restriction-create' => 'Cruthaigh',

# Undelete
'undelete'          => 'Díscrios leathanach scriosta',
'undeletepage'      => 'Féach ar leathanaigh scriosta agus díscrios iad',
'viewdeletedpage'   => 'Féach ar leathanaigh scriosta',
'undeletepagetext'  => 'Scriosaíodh na leathanaigh seo a leanas cheana féin, ach
tá síad sa cartlann fós agus is féidir iad a dhíscrios.
Ó am go ham, is féidir an cartlann a fholmhú.',
'undeleterevisions' => 'Cuireadh {{PLURAL:$1|leagan amháin|$1 leagain}} sa chartlann',
'undeletehistory'   => 'Dá díscriosfá an leathanach, díscriosfar gach leasú i stair an leathanaigh.
Dá gcruthaíodh leathanach nua leis an teideal céanna ó shin an scriosadh, taispeáinfear na sean-athruithe san stair roimhe seo, agus ní athshuífear leagan láithreach an leathanaigh go huathoibríoch.',
'undeletebtn'       => 'Díscrios!',
'undeletereset'     => 'Athshocraigh',
'undeletecomment'   => 'Tuairisc:',
'undeletedarticle'  => 'Díscriosadh "$1" ar ais',

# Namespace form on various pages
'namespace'      => 'Ainmspás:',
'invert'         => 'Cuir an roghnú bun os cionn',
'blanknamespace' => '(Gnáth)',

# Contributions
'contributions' => 'Dréachtaí úsáideora',
'mycontris'     => 'Mo chuid dréachtaí',
'contribsub2'   => 'Do $1 ($2)',
'nocontribs'    => 'Níor bhfuarthas aon athrú a raibh cosúil le na crítéir seo.',
'uctop'         => ' (barr)',
'month'         => 'As mí (agus is luaithe):',
'year'          => 'As bliain (agus is luaithe):',

'sp-contributions-newbies-sub' => 'Le cuntais nua',
'sp-contributions-blocklog'    => 'Log coisc',
'sp-contributions-username'    => 'Seoladh IP ná ainm úsáideoir:',

# What links here
'whatlinkshere'       => 'Naisc don lch seo',
'whatlinkshere-title' => 'Naisc chuig $1',
'whatlinkshere-page'  => 'Leathanach:',
'linklistsub'         => '(Liosta nasc)',
'linkshere'           => "Tá nasc chuig '''[[:$1]]''' ar na leathanaigh seo a leanas:",
'nolinkshere'         => "Níl leathanach ar bith ann a bhfuil nasc chuig '''[[:$1]]''' air.",
'nolinkshere-ns'      => "Níl leathanach ar bith ann san ainmspás roghnaithe a bhfuil nasc chuig '''[[:$1]]''' air.",
'isredirect'          => 'Leathanach athsheolaidh',
'istemplate'          => 'iniamh',
'whatlinkshere-prev'  => '{{PLURAL:$1|roimhe|$1 roimhe}}',
'whatlinkshere-next'  => '{{PLURAL:$1|ar aghaidh|$1 ar aghaidh}}',
'whatlinkshere-links' => '← naisc',

# Block/unblock
'blockip'                 => 'Coisc úsáideoir',
'blockip-legend'          => 'Cosc úsáideoir',
'blockiptext'             => 'Úsáid an foirm anseo thíos chun bealach scríofa a chosc ó
seoladh IP nó ainm úsáideora áirithe.
Is féidir leat an rud seo a dhéanamh amháin chun an chreachadóireacht a chosc, de réir
mar a deirtear sa [[{{MediaWiki:Policy-url}}|polasaí {{GRAMMAR:genitive|{{SITENAME}}}}]].
Líonaigh cúis áirithe anseo thíos (mar shampla, is féidir leat a luaigh
leathanaigh áirithe a rinne an duine damáiste ar).',
'ipaddress'               => 'Seoladh IP / ainm úsáideora',
'ipadressorusername'      => 'Seoladh IP nó ainm úsáideora:',
'ipbexpiry'               => 'Am éaga',
'ipbreason'               => 'Cúis',
'ipbreasonotherlist'      => 'Fáth eile',
'ipbreason-dropdown'      => '*Fáthanna coitianta
** Loitiméaracht
** Naisc turscar
** Fadhbanna cóipcheart
** Ag iarraidh ciapadh daoine eile
** Drochúsáid as cuntais iolrach
** Fadhbanna idirvicí
** Feallaire
** Seachfhreastalaí Oscailte',
'ipbsubmit'               => 'Coisc an úsáideoir seo',
'ipbother'                => 'Méid eile ama',
'ipboptions'              => '2 uair:2 hours,1 lá amháin:1 day,3 lá:3 days,1 sheachtain amháin:1 week,2 sheachtain:2 weeks,1 mhí amháin:1 month,3 mhí:3 months,6 mhí:6 months,1 bhliain amháin:1 year,gan teorainn:infinite', # display1:time1,display2:time2,...
'ipbotheroption'          => 'eile',
'badipaddress'            => 'Níl aon úsáideoir ann leis an ainm seo.',
'blockipsuccesssub'       => "D'éirigh leis an cosc",
'blockipsuccesstext'      => 'Choisceadh [[Special:Contributions/$1|$1]].
<br />Féach ar an g[[Special:IPBlockList|liosta coisc IP]] chun coisc a athbhreithniú.',
'ipb-unblock-addr'        => 'Díchoisc $1',
'ipb-unblock'             => 'Díchosc ainm úsáideora ná seoladh IP',
'unblockip'               => 'Díchoisc úsáideoir',
'unblockiptext'           => 'Úsáid an foirm anseo thíos chun bealach scríofa a thabhairt ar ais do seoladh
IP nó ainm úsáideora a raibh faoi chosc roimhe seo.',
'ipusubmit'               => 'Díchoisc an seoladh seo',
'unblocked'               => 'Díchoisceadh [[User:$1|$1]]',
'ipblocklist'             => 'Liosta seoltaí IP agus ainmneacha úsáideoirí coiscthe',
'ipblocklist-legend'      => 'Aimsigh úsáideoir coiscthe',
'ipblocklist-username'    => 'Ainm úsáideora ná seoladh IP:',
'ipblocklist-submit'      => 'Cuardaigh',
'blocklistline'           => '$1, $2 a choisc $3 (am éaga $4)',
'infiniteblock'           => 'gan teora',
'anononlyblock'           => 'úsáideoirí gan ainm agus iad amháin',
'ipblocklist-empty'       => 'Tá an liosta coisc folamh.',
'blocklink'               => 'Cosc',
'unblocklink'             => 'bain an cosc',
'contribslink'            => 'dréachtaí',
'autoblocker'             => 'Coisceadh go huathoibríoch thú dá bharr gur úsáideadh do sheoladh IP ag an úsáideoir "[[User:$1|$1]]". Is é seo an cúis don cosc ar $1: "$2".',
'blocklogpage'            => 'Cuntas_coisc',
'blocklogentry'           => 'coisceadh [[$1]]; is é $2 an am éaga $3',
'blocklogtext'            => 'Seo é cuntas de gníomhartha coisc úsáideoirí agus míchoisc úsáideoirí. Ní cuirtear
seoltaí IP a raibh coiscthe go huathoibríoch ar an liosta seo. Féach ar an
[[Special:IPBlockList|Liosta coisc IP]] chun
liosta a fháil de coisc atá i bhfeidhm faoi láthair.',
'unblocklogentry'         => 'díchoisceadh $1',
'block-log-flags-noemail' => 'cosc ar ríomhphost',
'range_block_disabled'    => 'Faoi láthair, míchumasaítear an cumas riarthóra chun réimsechoisc a dhéanamh.',
'ipb_expiry_invalid'      => 'Am éaga neamhbhailí.',
'ipb_already_blocked'     => 'Tá cosc ar "$1" cheana féin',
'ip_range_invalid'        => 'Réimse IP neamhbhailí.',
'proxyblocker'            => 'Cosc ar seachfhreastalaithe',
'proxyblockreason'        => "Coisceadh do sheoladh IP dá bharr gur seachfhreastalaí
neamhshlándála is ea é. Déan teagmháil le do chomhlacht idirlín nó le do lucht cabhrach teicneolaíochta
go mbeidh 'fhios acu faoin fadhb slándála tábhachtach seo.",
'proxyblocksuccess'       => 'Rinneadh.',
'sorbsreason'             => 'Liostalaítear do sheoladh IP mar sheachfhreastalaí oscailte sa DNSBL.',

# Developer tools
'lockdb'              => 'Glasáil an bunachar sonraí',
'unlockdb'            => 'Díghlasáil bunachar sonraí',
'lockdbtext'          => "Dá nglasálfá an bunachar sonraí, ní beidh cead ar aon úsáideoir
leathanaigh a chur in eagar, a socruithe a athrú, a liostaí faire a athrú, nó rudaí eile a thrachtann le
athruithe san bunachar sonraí.
Cinntigh go bhfuil an scéal seo d'intinn agat, is go díghlasálfaidh tú an bunachar sonraí nuair a bhfuil
do chuid cothabháile críochnaithe.",
'unlockdbtext'        => "Dá díghlasálfá an bunachar sonraí, beidh ceat ag gach úsáideoirí aris
na leathanaigh a chur in eagar, a sainroghanna a athrú, a liostaí faire a athrú, agus rudaí eile
a dhéanamh a thrachtann le athruithe san bunachar sonraí.
Cinntigh go bhfuil an scéal seo d'intinn agat.",
'lockconfirm'         => 'Sea, is mian liom an bunachar sonraí a ghlasáil.',
'unlockconfirm'       => 'Sea, is mian liom an bunachar sonraí a dhíghlasáil.',
'lockbtn'             => 'Glasáil an bunachar sonraí',
'unlockbtn'           => 'Díghlasáil an bunachar sonraí',
'locknoconfirm'       => 'Níor mharcáil tú an bosca daingnithe.',
'lockdbsuccesssub'    => "D'éirigh le glasáil an bhunachair sonraí",
'unlockdbsuccesssub'  => "D'éirigh le díghlasáil an bhunachair sonraí",
'lockdbsuccesstext'   => 'Glasáladh an bunachar sonraí {{GRAMMAR:genitive|{{SITENAME}}}}.
<br />Cuimhnigh nach mór duit é a dhíghlasáil tar éis do chuid cothabháil.',
'unlockdbsuccesstext' => 'Díghlasáladh an bunachar sonraí {{GRAMMAR:genitive|{{SITENAME}}}}.',
'databasenotlocked'   => 'Níl an bunachar sonraí faoi ghlas.',

# Move page
'move-page-legend'        => 'Athainmnigh an leathanach',
'movepagetext'            => "Úsáid an fhoirm seo thíos chun leathanach a athainmniú. Aistreofar a chuid staire go léir chuig an teideal nua.
Déanfar leathanach atreoraithe den sean-teideal chuig an teideal nua.
Ní athrófar naisc chuig sean-teideal an leathanaigh; 
bí cinnte go ndéanfá cuardach ar atreoruithe [[Special:DoubleRedirects|dúbailte]] nó [[Special:BrokenRedirects|briste]].
Tá dualgas ort bheith cinnte go rachaidh na naisc chuig an áit is ceart fós.

Tabhair faoi deara '''nach''' n-athainmneofar an leathanach má tá leathanach ann cheana féin faoin teideal nua, ach amháin más folamh nó atreorú é nó mura bhfuil aon stair athraithe aige cheana.
Mar sin, is féidir leathanach a athainmniú ar ais chuig an teideal a raibh air roimhe má tá botún déanta agat, agus ní féidir leathanach atá ann cheana a fhorscríobh.

<font color=\"red\">'''AIRE!'''</font>
Is féidir gur dianbheart gan choinne é athrú a dhéanamh ar leathanach móréilimh;
cinntigh go dtuigeann tú na hiarmhairtí go léir roimh dul ar aghaigh.",
'movepagetalktext'        => "Aistreofar an leathanach phlé leis, má tá sin ann:
*'''muna''' bhfuil tú ag aistriú an leathanach trasna ainmspásanna,
*'''muna''' bhfuil leathanach phlé neamhfholamh ann leis an teideal nua, nó
*'''muna''' bhaineann tú an marc den bosca anseo thíos.

Sna scéil sin, caithfidh tú an leathanach a aistrigh nó a báigh leis na lámha má tá sin an rud atá uait.",
'movearticle'             => 'Athainmnigh an leathanach',
'movenotallowed'          => 'Níl cead agat leathanaigh a athainmniú.',
'newtitle'                => 'Go teideal nua',
'move-watch'              => 'Déan faire an leathanach seo',
'movepagebtn'             => 'Athainmnigh an leathanach',
'pagemovedsub'            => "D'éirigh leis an athainmniú",
'movepage-moved'          => '<big>\'\'\'Athainmníodh "$1" mar "$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => 'Tá leathanach leis an teideal seo ann cheana féin, nó níl an teideal a roghnaigh tú ina theideal bailí. Roghnaigh teideal eile le do thoil.',
'talkexists'              => "'''D'athainmníodh an leathanach féin, ach níorbh fhéidir an leathanach plé a athainmniú de bharr go bhfuil ceann ann cheana féin ag an teideal nua.
Cumaisc le chéile iad, le do thoil.'''",
'movedto'                 => 'athainmnithe go',
'movetalk'                => 'Athainmnigh an leathanach plé freisin.',
'1movedto2'               => 'Athainmníodh $1 mar $2',
'1movedto2_redir'         => 'Rinneadh athsheoladh de $1 go $2.',
'movelogpage'             => 'Loga athainmnithe',
'movelogpagetext'         => 'Liosta is ea seo thíos de leathanaigh athainmnithe.',
'movereason'              => 'Cúis',
'revertmove'              => 'athúsáid',
'delete_and_move'         => 'Scrios agus athainmnigh',
'delete_and_move_text'    => '==Tá scriosadh riachtanach==
Tá an leathanach sprice ("[[:$1]]") ann cheana féin.
Ar mhaith leat é a scriosadh chun áit a dhéanamh don athainmniú?',
'delete_and_move_confirm' => 'Tá, scrios an leathanach',
'delete_and_move_reason'  => "Scriosta chun áit a dhéanamh d'athainmniú",
'selfmove'                => 'Tá an ainm céanna ag an bhfoinse mar atá ar an ainm sprice; ní féidir leathanach a athainmniú bheith é féin.',
'immobile_namespace'      => 'Saghas speisialta leathanach atá ann san ainm sprice; ní féidir leathanaigh a athainmniú san ainmspás sin.',

# Export
'export'        => 'Easportáil leathanaigh',
'exporttext'    => 'Is féidir leat an téacs agus stair athraithe de leathanach áirithe a heasportáil,
fillte i bpíosa XML; is féidir leat ansin é a iompórtáil isteach vicí eile atá le na bogearraí MediaWiki
air, nó is féidir leat é a coinniú do do chuid shiamsa féin.',
'exportcuronly' => 'Ná cuir san áireamh ach an leagan láithreach; ná cuir an stair iomlán ann',
'export-submit' => 'Easportáil',

# Namespace 8 related
'allmessages'               => 'Teachtaireachtaí córais',
'allmessagesname'           => 'Ainm',
'allmessagesdefault'        => 'Téacs réamhshocraithe',
'allmessagescurrent'        => 'Téacs reatha',
'allmessagestext'           => 'Liosta is ea seo de theachtaireachtaí córais atá le fáil san ainmspás MediaWiki: .',
'allmessagesnotsupportedDB' => "Ní féidir an leathanach seo a úsáid dá bharr gur díchumasaíodh '''\$wgUseDatabaseMessages'''.",
'allmessagesfilter'         => "Scagaire teachtaireacht d'ainm:",

# Thumbnails
'thumbnail-more'  => 'Méadaigh',
'filemissing'     => 'Comhad ar iarraidh',
'thumbnail_error' => 'Earráid mionsamhail a chruthú: $1',

# Special:Import
'import'                  => 'Iompórtáil leathanaigh',
'importinterwiki'         => 'Iompórtáil trasna vicíonna',
'import-interwiki-submit' => 'iompórtáil',
'importtext'              => 'Easportáil an comhad ón bhfoinse-vicí le do thoil (le húsáid na tréithe
Speisialta:Export), sábháil ar do dhíosca é agus uaslódáil anseo é.',
'importnopages'           => 'Níl aon leathanaigh chun iompórtáil',
'importfailed'            => 'Theip ar an iompórtáil: $1',
'importnotext'            => 'Folamh nó gan téacs',
'importsuccess'           => "D'eirigh leis an iompórtáil!",
'importhistoryconflict'   => 'Tá stair athraithe contrártha ann cheana féin (is dócha go
uaslódáladh an leathanach seo roimh ré)',
'importnosources'         => "Níl aon fhoinse curtha i leith d'iompórtáil trasna vicíonna, agus
ní féidir uaslódála staire díreacha a dhéanamh faoi láthair.",

# Import log
'importlogpage'             => 'Log iompórtáil',
'import-logentry-interwiki' => 'traisvicithe $1',

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mo leathanach úsáideora',
'tooltip-pt-anonuserpage'         => 'Leathanach úsáideora don IP ina dhéanann tú do chuid athruithe',
'tooltip-pt-mytalk'               => 'Mo leathanach plé',
'tooltip-pt-anontalk'             => 'Plé maidir le na hathruithe a dhéantar ón seoladh IP seo',
'tooltip-pt-preferences'          => 'Mo chuid sainroghanna',
'tooltip-pt-watchlist'            => 'Liosta de na leathanaigh a bhfuil tú á bhfaire ar athruithe',
'tooltip-pt-mycontris'            => 'Liosta de mo chuid dréachtaí',
'tooltip-pt-login'                => 'Moltar duit logáil isteach, ach níl sé riachtanach.',
'tooltip-pt-anonlogin'            => 'Moltar duit logáil isteach, ach níl sé riachtanach.',
'tooltip-pt-logout'               => 'Logáil amach',
'tooltip-ca-talk'                 => 'Plé maidir leis an leathanach ábhair',
'tooltip-ca-edit'                 => 'Is féidir leat an leathanach seo a athrú. Más é do thoil é, bain úsáid as an cnaipe réamhamhairc roimh sábháil a dhéanamh.',
'tooltip-ca-addsection'           => 'Cuir trácht leis an plé seo..',
'tooltip-ca-viewsource'           => 'Tá an leathanach seo glasáilte. Is féidir leat a fhoinse a fheiceáil.',
'tooltip-ca-history'              => 'Leagain stairiúla den leathanach seo.',
'tooltip-ca-protect'              => 'Glasáil an leathanach seo',
'tooltip-ca-delete'               => 'Scrios an leathanach seo',
'tooltip-ca-undelete'             => 'Díscrios na hathruithe a rinneadh don leathanach seo roimh a scriosadh é',
'tooltip-ca-move'                 => 'Athainmnigh an leathanach',
'tooltip-ca-watch'                => 'Cuir an leathanach seo le do liosta faire',
'tooltip-ca-unwatch'              => 'Bain an leathanach seo de do liosta faire',
'tooltip-search'                  => 'Cuardaigh sa vicí seo',
'tooltip-p-logo'                  => 'Príomhleathanach',
'tooltip-n-mainpage'              => 'Tabhair cuairt ar an bPríomhleathanach',
'tooltip-n-portal'                => 'Maidir leis an tionscadal, cad is féidir leat a dhéanamh, conas achmhainní a fháil',
'tooltip-n-currentevents'         => 'Faigh eolas cúlrach maidir le chursaí reatha',
'tooltip-n-recentchanges'         => 'Liosta de na hathruithe is déanaí sa vicí.',
'tooltip-n-randompage'            => 'Lódáil leathanach fánach',
'tooltip-n-help'                  => 'An áit chun cabhair a fháil.',
'tooltip-t-whatlinkshere'         => 'Liosta de gach leathanach sa vicí ina bhfuil nasc chuig an leathanach seo',
'tooltip-t-recentchangeslinked'   => 'Na hathruithe is déanaí ar leathanaigh a nascaíonn chuig an leathanach seo',
'tooltip-feed-rss'                => 'Fotha RSS don leathanach seo',
'tooltip-feed-atom'               => 'Fotha Atom don leathanach seo',
'tooltip-t-contributions'         => 'Féach ar an liosta dréachtaí a rinne an t-úsáideoir seo',
'tooltip-t-emailuser'             => 'Cuir teachtaireacht chuig an úsáideoir seo',
'tooltip-t-upload'                => 'Comhaid íomhá nó meáin a uaslódáil',
'tooltip-t-specialpages'          => 'Liosta de gach leathanach speisialta',
'tooltip-ca-nstab-main'           => 'Féach ar an leathanach ábhair',
'tooltip-ca-nstab-user'           => 'Féach ar an leathanach úsáideora',
'tooltip-ca-nstab-media'          => 'Féach ar an leathanach meáin',
'tooltip-ca-nstab-special'        => 'Is leathanach speisialta é seo, ní féidir leat an leathanach é fhéin a athrú.',
'tooltip-ca-nstab-project'        => 'Féach ar an leathanach thionscadail',
'tooltip-ca-nstab-image'          => 'Féach ar an leathanach íomhá',
'tooltip-ca-nstab-mediawiki'      => 'Féach ar an teachtaireacht córais',
'tooltip-ca-nstab-template'       => 'Féach ar an teimpléad',
'tooltip-ca-nstab-help'           => 'Féach ar an leathanach cabhrach',
'tooltip-ca-nstab-category'       => 'Féach ar an leathanach catagóire',
'tooltip-minoredit'               => 'Déan mionathrú den athrú seo',
'tooltip-save'                    => 'Sábháil do chuid athruithe',
'tooltip-preview'                 => 'Réamhamharc ar do chuid athruithe; úsáid an gné seo roimh a shábhálaíonn tú!',
'tooltip-diff'                    => 'Taispeáin na difríochtaí áirithe a rinne tú don téacs',
'tooltip-compareselectedversions' => 'Féach na difríochtaí idir an dhá leagain roghnaithe den leathanach seo.',
'tooltip-watch'                   => 'Cuir an leathanach seo le do liosta faire',

# Stylesheets
'monobook.css' => '/* athraigh an comhad seo chun an craiceann MonoBook a athrú don suíomh ar fad */',

# Metadata
'nodublincore'      => 'Míchumasaítear meitea-shonraí Dublin Core RDF ar an freastalaí seo.',
'nocreativecommons' => 'Míchumasaítear meitea-shonraí Creative Commons RDF ar an freastalaí seo.',
'notacceptable'     => 'Ní féidir leis an freastalaí vicí na sonraí a chur ar fáil i bhformáid atá inléite ag do chliant.',

# Attribution
'anonymous'        => 'Úsáideoir(í) gan ainm ar {{SITENAME}}',
'siteuser'         => 'Úsáideoir $1 ag {{SITENAME}}',
'lastmodifiedatby' => 'Leasaigh $3 an leathanach seo go déanaí ag $2, $1.', # $1 date, $2 time, $3 user
'othercontribs'    => 'Bunaithe ar saothair le $1.',
'others'           => 'daoine eile',
'siteusers'        => 'Úsáideoir(í) ag {{SITENAME}} $1',
'creditspage'      => 'Creidiúintí leathanaigh',
'nocredits'        => 'Níl aon eolas creidiúna le fáil don leathanach seo.',

# Spam protection
'spamprotectiontitle' => 'Scagaire in aghaidh ríomhphost dramhála',
'spamprotectiontext'  => 'Chuir an scagaire dramhála bac ar an leathanach a raibh tú ar
iarradh sábháil. Is dócha gur nasc chuig suíomh seachtrach ba chúis leis.',
'spamprotectionmatch' => 'Truicear ár scagaire dramhála ag an téacs seo a leanas: $1',
'spambot_username'    => 'MediaWiki turscar glanadh',

# Info page
'infosubtitle'   => 'Eolas don leathanach',
'numedits'       => 'Méid athruithe (alt): $1',
'numtalkedits'   => 'Méid athruithe (leathanach phlé): $1',
'numwatchers'    => 'Méid féachnóirí: $1',
'numauthors'     => 'Méid údair ar leith (alt): $1',
'numtalkauthors' => 'Méid údair ar leith (leathanach phlé): $1',

# Math options
'mw_math_png'    => 'Déan PNG-íomhá gach uair',
'mw_math_simple' => 'Déan HTML má tá sin an-easca, nó PNG ar mhodh eile',
'mw_math_html'   => 'Déan HTML más féidir, nó PNG ar mhodh eile',
'mw_math_source' => 'Fág mar cló TeX (do teacsleitheoirí)',
'mw_math_modern' => 'Inmholta do bhrabhsálaithe nua-aimseartha',
'mw_math_mathml' => 'MathML más féidir (turgnamhach)',

# Patrolling
'markaspatrolleddiff'   => 'Marcáil bheith patrólaithe',
'markaspatrolledtext'   => 'Comharthaigh an t-alt seo mar patrólta',
'markedaspatrolled'     => 'Marcáil bheith patrólaithe',
'markedaspatrolledtext' => 'Marcáladh an athrú áirithe seo bheith patrólaithe.',
'rcpatroldisabled'      => 'Mhíchumasaíodh Patról na n-Athruithe is Déanaí',
'rcpatroldisabledtext'  => 'Tá an tréith Patról na n-Athruithe is Déanaí míchumasaithe faoi láthair.',

# Patrol log
'patrol-log-auto' => '(uathoibríoch)',

# Image deletion
'deletedrevision' => 'Scriosadh an sean-leagan $1',

# Browsing diffs
'previousdiff' => '← An difríocht roimhe seo',
'nextdiff'     => 'An chéad dhifear eile →',

# Media information
'mediawarning'         => "'''Aire''': Tá seans ann go bhfuil cód mailíseach sa comhad seo - b'fheidir go gcuirfear do chóras i gcontúirt dá rithfeá é.
<hr />",
'imagemaxsize'         => 'Cuir an teorann seo ar na íomhánna atá le fáil ar leathanaigh cuir síos íomhánna:',
'thumbsize'            => 'Méid mionshamhla :',
'file-info-size'       => '($1 × $2 picteilín, méid comhaid: $3, cineál MIME: $4)',
'file-nohires'         => '<small>Níl aon taifeach is mó ar fáil.</small>',
'svg-long-desc'        => '(Comhad SVG, ainmniúil $1 × $2 picteilíni, méid comhaid: $3)',
'show-big-image'       => 'Taispeáin leagan ardtaifigh den íomhá',
'show-big-image-thumb' => '<small>Méid an réamhamhairc seo: $1 × $2 picteilín</small>',

# Special:NewImages
'newimages'     => 'Gailearaí na n-íomhánna nua',
'imagelisttext' => 'Tá liosta thíos de {{PLURAL:$1|comhad amháin|$1 comhaid $2}}.',
'showhidebots'  => '($1 róbónna)',
'noimages'      => 'Tada le feiceáil.',
'ilsubmit'      => 'Cuardaigh',
'bydate'        => 'de réir dáta',

# Metadata
'metadata'          => 'Meiteasonraí',
'metadata-expand'   => 'Taispeáin sonraí síneadh',
'metadata-collapse' => 'Folaigh sonraí síneadh',

# EXIF tags
'exif-imagewidth'                  => 'Leithead',
'exif-imagelength'                 => 'Airde',
'exif-bitspersample'               => 'Gíotáin sa chomhpháirt',
'exif-compression'                 => 'Scéim comhbhrúite',
'exif-photometricinterpretation'   => 'Comhbhrú picteilíní',
'exif-orientation'                 => 'Treoshuíomh',
'exif-samplesperpixel'             => 'Líon na gcomhpháirt',
'exif-planarconfiguration'         => 'Eagar na sonraí',
'exif-ycbcrsubsampling'            => 'Cóimheas foshamplála de Y i gcoinne C',
'exif-ycbcrpositioning'            => 'Suí Y agus C',
'exif-xresolution'                 => 'Taifeach íomhá i dtreo an leithid',
'exif-yresolution'                 => 'Taifeach íomhá i dtreo an airde',
'exif-resolutionunit'              => 'Aonad an taifigh X agus Y',
'exif-stripoffsets'                => 'Suíomh na sonraí íomhá',
'exif-rowsperstrip'                => 'Líon na rónna sa stráice',
'exif-stripbytecounts'             => 'Bearta sa stráice comhbhrúite',
'exif-jpeginterchangeformat'       => 'Aischló don SOI JPEG',
'exif-jpeginterchangeformatlength' => 'Bearta sonraí JPEG',
'exif-transferfunction'            => 'Feidhm aistrithe',
'exif-whitepoint'                  => 'Crómatacht na bpointí bán',
'exif-primarychromaticities'       => 'Crómatachta na bpríomhacht',
'exif-ycbcrcoefficients'           => 'Comhéifeachtaí mhaitrís trasfhoirmithe an dathspáis',
'exif-referenceblackwhite'         => 'Péire luachanna tagartha don dubh is don bán',
'exif-datetime'                    => 'Dáta agus am athrú an chomhaid',
'exif-imagedescription'            => 'Íomhátheideal',
'exif-make'                        => 'Déantóir an ceamara',
'exif-model'                       => 'Déanamh an ceamara',
'exif-software'                    => 'Na bogearraí a úsáideadh',
'exif-artist'                      => 'Údar',
'exif-copyright'                   => 'Úinéir an chóipchirt',
'exif-exifversion'                 => 'Leagan EXIF',
'exif-flashpixversion'             => 'Leagan Flashpix atá á thacú',
'exif-colorspace'                  => 'Dathspás',
'exif-componentsconfiguration'     => 'Ciall le gach giota',
'exif-compressedbitsperpixel'      => 'Modh chomhbhrú na n-íomhánna',
'exif-pixelydimension'             => 'Leithead bailí don íomhá',
'exif-pixelxdimension'             => 'Airde bailí don íomhá',
'exif-makernote'                   => 'Nótaí an déantóra',
'exif-usercomment'                 => 'Nótaí an úsáideora',
'exif-relatedsoundfile'            => 'comhad gaolmhara fuaime',
'exif-datetimeoriginal'            => 'Dáta agus am ghiniúint na sonraí',
'exif-datetimedigitized'           => 'Dáta agus am digitithe',
'exif-subsectime'                  => 'Foshoicindí DateTime',
'exif-subsectimeoriginal'          => 'Foshoicindí DateTimeOriginal',
'exif-subsectimedigitized'         => 'Foshoicindí DateTimeDigitized',
'exif-exposuretime'                => 'Am nochta',
'exif-fnumber'                     => 'Uimhir F',
'exif-exposureprogram'             => 'Clár nochta',
'exif-spectralsensitivity'         => 'Íogaireacht an speictrim',
'exif-isospeedratings'             => 'Grádú ISO luais',
'exif-oecf'                        => 'Fachtóir optaileictreonach tiontaithe',
'exif-shutterspeedvalue'           => 'Luas nochta',
'exif-aperturevalue'               => 'Cró',
'exif-brightnessvalue'             => 'Gile',
'exif-exposurebiasvalue'           => 'Laobh nochta',
'exif-maxaperturevalue'            => 'Cró tíre uasmhéideach',
'exif-subjectdistance'             => 'Fad ón ábhar',
'exif-meteringmode'                => 'Modh meadarachta',
'exif-lightsource'                 => 'Foinse solais',
'exif-flash'                       => 'Splanc',
'exif-focallength'                 => 'Fad fócasach an lionsa',
'exif-subjectarea'                 => 'Achar an ábhair',
'exif-flashenergy'                 => 'Splancfhuinneamh',
'exif-spatialfrequencyresponse'    => 'Freagairt minicíochta spáis',
'exif-focalplanexresolution'       => 'Taifeach an plána fócasaigh X',
'exif-focalplaneyresolution'       => 'Taifeach an plána fócasaigh Y',
'exif-focalplaneresolutionunit'    => 'Aonad taifigh an plána fócasaigh',
'exif-subjectlocation'             => 'Suíomh an ábhair',
'exif-exposureindex'               => 'Innéacs nochta',
'exif-sensingmethod'               => 'Modh braite',
'exif-filesource'                  => 'Foinse comhaid',
'exif-scenetype'                   => 'Cineál radhairc',
'exif-cfapattern'                  => 'Patrún CFA',
'exif-customrendered'              => 'Íomháphróiseáil saincheaptha',
'exif-exposuremode'                => 'Modh nochta',
'exif-whitebalance'                => 'Bánchothromaíocht',
'exif-digitalzoomratio'            => 'Cóimheas zúmála digiteaí',
'exif-focallengthin35mmfilm'       => 'Fad fócasach i scannán 35 mm',
'exif-scenecapturetype'            => 'Cineál gabhála radhairc',
'exif-gaincontrol'                 => 'Rialú radhairc',
'exif-contrast'                    => 'Codarsnacht',
'exif-saturation'                  => 'Sáithiú',
'exif-sharpness'                   => 'Géire',
'exif-devicesettingdescription'    => 'Cur síos ar socruithe gléis',
'exif-subjectdistancerange'        => 'Raon fada ón ábhar',
'exif-imageuniqueid'               => 'Aitheantas uathúil an íomhá',
'exif-gpsversionid'                => 'Leagan clibe GPS',
'exif-gpslatituderef'              => 'Domhan-leithead Thuaidh no Theas',
'exif-gpslatitude'                 => 'Domhan-leithead',
'exif-gpslongituderef'             => 'Domhanfhad Thoir nó Thiar',
'exif-gpslongitude'                => 'Domhanfhad',
'exif-gpsaltituderef'              => 'Tagairt airde',
'exif-gpsaltitude'                 => 'Airde',
'exif-gpstimestamp'                => 'Am GPS (clog adamhach)',
'exif-gpssatellites'               => 'Satailítí úsáidte don tomhas',
'exif-gpsstatus'                   => 'Stádas an ghlacadóra',
'exif-gpsmeasuremode'              => 'Modh tomhais',
'exif-gpsdop'                      => 'Beachtas tomhais',
'exif-gpsspeedref'                 => 'Aonad luais',
'exif-gpsspeed'                    => 'Luas an ghlacadóra GPS',
'exif-gpstrackref'                 => 'Tagairt don treo gluaiseachta',
'exif-gpstrack'                    => 'Treo gluaiseachta',
'exif-gpsimgdirectionref'          => 'Tagairt do treo an íomhá',
'exif-gpsimgdirection'             => 'Treo an íomhá',
'exif-gpsmapdatum'                 => 'Sonraí suirbhéireachta geodasaí a úsáideadh',
'exif-gpsdestlatituderef'          => 'Tagairt don domhan-leithead sprice',
'exif-gpsdestlatitude'             => 'Domhan-leithead sprice',
'exif-gpsdestlongituderef'         => 'Tagairt don domhanfhad sprice',
'exif-gpsdestlongitude'            => 'Domhanfhad sprice',
'exif-gpsdestbearingref'           => 'Tagairt don treo-uillinn sprice',
'exif-gpsdestbearing'              => 'Treo-uillinn sprice',
'exif-gpsdestdistanceref'          => 'Tagairt don fad ón áit sprice',
'exif-gpsdestdistance'             => 'Fad ón áit sprice',
'exif-gpsprocessingmethod'         => 'Ainm an modha próiseála GPS',
'exif-gpsareainformation'          => 'Ainm an cheantair GPS',
'exif-gpsdatestamp'                => 'Dáta GPS',
'exif-gpsdifferential'             => 'Ceartú difreálach GPS',

# EXIF attributes
'exif-compression-1' => 'Neamh-chomhbhrúite',

'exif-orientation-1' => 'Gnáth', # 0th row: top; 0th column: left
'exif-orientation-2' => 'Iompaithe go cothrománach', # 0th row: top; 0th column: right
'exif-orientation-3' => 'Rothlaithe trí 180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => 'Iompaithe go hingearach', # 0th row: bottom; 0th column: left
'exif-orientation-5' => 'Rothlaithe trí 90° CCW agus iompaithe go hingearach', # 0th row: left; 0th column: top
'exif-orientation-6' => 'Rothlaithe trí 90° CW', # 0th row: right; 0th column: top
'exif-orientation-7' => 'Rothlaithe trí 90° CW agus iompaithe go hingearach', # 0th row: right; 0th column: bottom
'exif-orientation-8' => 'Rothlaithe trí 90° CCW', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'Formáid shmutánach',
'exif-planarconfiguration-2' => 'Formáid phlánach',

'exif-componentsconfiguration-0' => 'níl a leithéid ann',

'exif-exposureprogram-0' => 'Gan sainiú',
'exif-exposureprogram-1' => 'Leis na lámha',
'exif-exposureprogram-2' => 'Gnáthchlár',
'exif-exposureprogram-3' => 'Tosaíocht nochta',
'exif-exposureprogram-4' => 'Tosaíocht cró',
'exif-exposureprogram-5' => 'Clár cúise (laofa do doimhneacht réimse)',
'exif-exposureprogram-6' => 'Clár gnímh (laofa do cróluas tapaidh)',
'exif-exposureprogram-7' => 'Modh portráide (do grianghraif i ngar don ábhar,
le cúlra as fócas)',
'exif-exposureprogram-8' => 'Modh tírdhreacha (do grianghraif tírdhreacha le
cúlra i bhfócas)',

'exif-subjectdistance-value' => '$1 méadair',

'exif-meteringmode-0'   => 'Anaithnid',
'exif-meteringmode-1'   => 'Meán',
'exif-meteringmode-2'   => 'MeánUalaitheDonLár',
'exif-meteringmode-3'   => 'Spota',
'exif-meteringmode-4'   => 'Ilspotach',
'exif-meteringmode-5'   => 'Patrún',
'exif-meteringmode-6'   => 'Páirteach',
'exif-meteringmode-255' => 'Eile',

'exif-lightsource-0'   => 'Anaithnid',
'exif-lightsource-1'   => 'Solas lae',
'exif-lightsource-2'   => 'Fluaraiseach',
'exif-lightsource-3'   => 'Tungstan (solas gealbhruthach)',
'exif-lightsource-4'   => 'Splanc',
'exif-lightsource-9'   => 'Aimsir breá',
'exif-lightsource-10'  => 'Aimsir scamallach',
'exif-lightsource-11'  => 'Scáth',
'exif-lightsource-12'  => 'Solas lae fluaraiseach (D 5700 â€“ 7100K)',
'exif-lightsource-13'  => 'Solas bán lae fluaraiseach (N 4600 â€“ 5400K)',
'exif-lightsource-14'  => 'Solas fuar bán fluaraiseach (W 3900 â€“ 4500K)',
'exif-lightsource-15'  => 'Solas bán fluaraiseach (WW 3200 â€“ 3700K)',
'exif-lightsource-17'  => 'Gnáthsholas A',
'exif-lightsource-18'  => 'Gnáthsholas B',
'exif-lightsource-19'  => 'Gnáthsholas C',
'exif-lightsource-24'  => 'Tungstan stiúideó ISO',
'exif-lightsource-255' => 'Foinse eile solais',

'exif-focalplaneresolutionunit-2' => 'orlaigh',

'exif-sensingmethod-1' => 'Gan sainiú',
'exif-sensingmethod-2' => 'Braiteoir aonshliseach ceantair datha',
'exif-sensingmethod-3' => 'Braiteoir dháshliseach ceantair datha',
'exif-sensingmethod-4' => 'Braiteoir tríshliseach ceantair datha',
'exif-sensingmethod-5' => 'Braiteoir dathsheicheamhach ceantair',
'exif-sensingmethod-7' => 'Braiteoir trílíneach',
'exif-sensingmethod-8' => 'Braiteoir dathsheicheamhach línte',

'exif-scenetype-1' => 'Grianghraf a rinneadh go díreach',

'exif-customrendered-0' => 'Gnáthphróiseas',
'exif-customrendered-1' => 'Próiseas saincheaptha',

'exif-exposuremode-0' => 'Nochtadh uathoibríoch',
'exif-exposuremode-1' => 'Nochtadh láimhe',
'exif-exposuremode-2' => 'Brac uathoibríoch',

'exif-whitebalance-0' => 'Bánchothromaíocht uathoibríoch',
'exif-whitebalance-1' => 'Bánchothromaíocht láimhe',

'exif-scenecapturetype-0' => 'Gnáth',
'exif-scenecapturetype-1' => 'Tírdhreach',
'exif-scenecapturetype-2' => 'Portráid',
'exif-scenecapturetype-3' => 'Radharc oíche',

'exif-gaincontrol-0' => 'Dada',
'exif-gaincontrol-1' => 'Íosneartúchán suas',
'exif-gaincontrol-2' => 'Uasneartúchán suas',
'exif-gaincontrol-3' => 'Íosneartúchán síos',
'exif-gaincontrol-4' => 'Uasneartúchán síos',

'exif-contrast-0' => 'Gnáth',
'exif-contrast-1' => 'Bog',
'exif-contrast-2' => 'Crua',

'exif-saturation-0' => 'Gnáth',
'exif-saturation-1' => 'Sáithiúchán íseal',
'exif-saturation-2' => 'Ard-sáithiúchán',

'exif-sharpness-0' => 'Gnáth',
'exif-sharpness-1' => 'Bog',
'exif-sharpness-2' => 'Crua',

'exif-subjectdistancerange-0' => 'Anaithnid',
'exif-subjectdistancerange-1' => 'Macra',
'exif-subjectdistancerange-2' => 'Radharc teann',
'exif-subjectdistancerange-3' => 'Cianradharc',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => 'Domhan-leithead thuaidh',
'exif-gpslatitude-s' => 'Domhan-leithead theas',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => 'Domhanfhad Thoir',
'exif-gpslongitude-w' => 'Domhanfhad Thiar',

'exif-gpsstatus-a' => 'Tomhas ar siúl',
'exif-gpsstatus-v' => 'Tomhas dodhéanta',

'exif-gpsmeasuremode-2' => 'Tomhas déthoiseach',
'exif-gpsmeasuremode-3' => 'Tomhas tríthoiseach',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => 'Ciliméadair san uair',
'exif-gpsspeed-m' => 'Mílte san uair',
'exif-gpsspeed-n' => 'Muirmhílte',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => 'Fíorthreo',
'exif-gpsdirection-m' => 'Treo maighnéadach',

# External editor support
'edit-externally'      => 'Athraigh an comhad seo le feidhmchlár seachtrach',
'edit-externally-help' => 'Féach ar na

[http://www.mediawiki.org/wiki/Manual:External_editors treoracha cumraíochta] (as Béarla)

le tuilleadh eolais.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'gach',
'imagelistall'     => 'gach',
'watchlistall2'    => 'gach',
'namespacesall'    => 'an t-iomlán',
'monthsall'        => 'an t-iomlán',

# E-mail address confirmation
'confirmemail'            => 'Deimhnigh do ríomhsheoladh',
'confirmemail_text'       => 'Tá sé de dhíth an an vicí seo do ríomhsheoladh a bhailíochtú sula n-úsáideann tú na gnéithe ríomhphoist. Brúigh an cnaipe seo thíos chun teachtaireacht deimhnithe a sheoladh chuig do chuntas ríomhphoist. Beidh nasc ann sa chomhad ina mbeidh cód áirithe; lódáil an nasc i do bhrabhsálaí chun deimhniú go bhfuil do ríomhsheoladh bailí.',
'confirmemail_send'       => 'Seol cód deimhnithe',
'confirmemail_sent'       => 'Teachtaireacht deimhnithe seolta chugat.',
'confirmemail_sendfailed' => "Ní féidir {{SITENAME}} do theachtaireacht deimhnithe a sheoladh. 
Féach an bhfuil carachtair neamh-bhailí ann sa seoladh.

D'fhreagair an clár ríomhphoist: $1",
'confirmemail_invalid'    => "Cód deimhnithe neamh-bhailí. B'fhéidir gur chuaidh an cód as feidhm.",
'confirmemail_success'    => 'Deimhníodh do ríomhsheoladh. Is féidir leat logáil isteach anois agus sult a bhaint as an vicí!',
'confirmemail_loggedin'   => 'Deimhníodh do sheoladh ríomhphoist.',
'confirmemail_error'      => 'Tharlaigh botún éigin le sabháil do dheimhniú.',
'confirmemail_subject'    => 'Deimhniú do ríomhsheoladh ar an {{SITENAME}}',
'confirmemail_body'       => 'Chláraigh duine éigin (tusa is dócha) an cuntas "$2" ar {{SITENAME}}
agus rinneadh é seo ón seoladh IP $1, ag úsáid an ríomhsheolta seo.

Chun deimhniú gur leatsa an cuntas seo, agus chun gnéithe ríomhphoist 
a chur i ngníomh ag {{SITENAME}}, oscail an nasc seo i do bhrabhsálaí:

$3

<nowiki>*</nowiki>Mura* tusa a chláraigh an cuntas, lean an nasc seo chun 
deimhniú an ríomhsheolta a chur ar cheal:

$5

Rachaidh an cód deimhnithe seo as feidhm ag $4.',

# Scary transclusion
'scarytranscludedisabled' => '[Díchumasaíodh trasáireamh idir vicíonna]',
'scarytranscludefailed'   => '[Theip leis an iarradh teimpléid do $1]',
'scarytranscludetoolong'  => '[Tá an URL ró-fhada]',

# Trackbacks
'trackbackremove' => ' ([$1 Scrios])',

# Delete conflict
'recreate' => 'Athchruthaigh',

# HTML dump
'redirectingto' => 'Ag athdhíriú go [[:$1]]...',

# action=purge
'confirm_purge'        => 'An bhfuil tú cinnte go dteastaíonn uait taisce an leathanaigh seo a bhánú?

$1',
'confirm_purge_button' => 'Tá',

# AJAX search
'searchnamed'   => "Cuardaigh le leathanaigh ab ainm ''$1''.",
'articletitles' => "Ailt a tosaíonn le ''$1''",
'hideresults'   => 'Folaigh torthaí',

# Multipage image navigation
'imgmultigoto' => 'Téigh go leathanach $1',

# Auto-summaries
'autoredircomment' => 'Ag athdhíriú go [[$1]]',
'autosumm-new'     => 'Leathanach nua: $1',

# Live preview
'livepreview-loading' => 'Ag lódáil…',
'livepreview-ready'   => 'Lódáil… Réidh!',

# Watchlist editor
'watchlistedit-raw-titles' => 'Teideail:',

# Watchlist editing tools
'watchlisttools-view' => 'Féach ar na hathruithe ábhartha',
'watchlisttools-edit' => 'Féach ar do liosta faire ná cuir in eagar é',
'watchlisttools-raw'  => 'Cuir do amhliosta faire in eagar',

# Special:Version
'version'                  => 'Leagan', # Not used as normal message but as header for the special page itself
'version-version'          => 'Leagan',
'version-license'          => 'Ceadúnas',
'version-software-version' => 'Leagan',

# Special:FilePath
'filepath-page' => 'Comhad:',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => 'Ainm comhaid:',
'fileduplicatesearch-submit'   => 'Cuardaigh',

# Special:SpecialPages
'specialpages'               => 'Leathanaigh speisialta',
'specialpages-group-other'   => 'Leathanaigh speisialta eile',
'specialpages-group-login'   => 'Logáil isteach / cruthaigh cuntas',
'specialpages-group-changes' => 'Athruithe is déanaí agus logaí',
'specialpages-group-users'   => 'Úsáideoirí agus cearta',
'specialpages-group-pages'   => 'Liosta leathanaigh',
'specialpages-group-spam'    => 'Uirlisí turscar',

);
