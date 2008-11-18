<?php
/** Waray (Winaray)
 *
 * @ingroup Language
 * @file
 *
 * @author Harvzsf
 * @author לערי ריינהארט
 */

$messages = array(
# User preference toggles
'tog-underline'           => 'Bagisa ha ilarom an mga sumpay:',
'tog-hideminor'           => 'Tago-a an mga gagmay nga pagliwat ha mga bag-o pa la nga mga kabag-ohan',
'tog-extendwatchlist'     => 'Igpadako an angay timan-an nga makita an ngatanan nga mga nahanunungod nga mga kabag-ohan',
'tog-watchcreations'      => 'Igdugang in mga pakli nga akon ginhimo ngadto han akon angay timan-an',
'tog-watchdefault'        => 'Igdugang in mga pakli nga akon ginliwat ngadto han akon angay timan-an',
'tog-watchmoves'          => 'Igdugang in mga pakli nga akon ginpamalhin ngadto han akon angay timan-an',
'tog-watchdeletion'       => 'Igdugang in mga pakli nga akon ginpamara ngadto han akon angay timan-an',
'tog-shownumberswatching' => 'Igpakita an ihap han mga nangingita nga mga nagamit',
'tog-watchlisthideown'    => 'Tago-a an akon mga ginliwat tikang han angay timan-an',
'tog-watchlisthidebots'   => 'Tago-a an ginliwat hin bot tikang han angay timan-an',
'tog-watchlisthideminor'  => 'Tago-a an mga gagmay nga pagliwat tikang han angay timan-an',
'tog-ccmeonemails'        => 'Padad-i ak hin mga kopya hin mga email nga akon ginpapadara ha iba nga mga nágámit',
'tog-showhiddencats'      => 'Igpakita an mga tinago nga mga kategorya',

# Dates
'sunday'        => 'Dominggo',
'monday'        => 'Lunes',
'tuesday'       => 'Martes',
'wednesday'     => 'Miyerkoles',
'thursday'      => 'Huwebes',
'friday'        => 'Biyernes',
'saturday'      => 'Sabado',
'sun'           => 'Dom',
'mon'           => 'Lun',
'tue'           => 'Mar',
'wed'           => 'Mi',
'thu'           => 'Hu',
'fri'           => 'Bi',
'sat'           => 'Sab',
'january'       => 'Enero',
'february'      => 'Pebrero',
'march'         => 'Marso',
'april'         => 'Abril',
'may_long'      => 'Mayo',
'june'          => 'Hunyo',
'july'          => 'Hulyo',
'august'        => 'Agosto',
'september'     => 'Septyembre',
'october'       => 'Oktubre',
'november'      => 'Nobyembre',
'december'      => 'Disyembre',
'january-gen'   => 'han Enero',
'february-gen'  => 'han Pebrero',
'march-gen'     => 'han Marso',
'april-gen'     => 'han Abril',
'may-gen'       => 'han Mayo',
'june-gen'      => 'han Hunyo',
'july-gen'      => 'han Hulyo',
'august-gen'    => 'han Agosto',
'september-gen' => 'han Septyembre',
'october-gen'   => 'han Oktubre',
'november-gen'  => 'han Nobyembre',
'december-gen'  => 'han Disyembre',
'jan'           => 'Ene',
'feb'           => 'Peb',
'mar'           => 'Mar',
'apr'           => 'Abr',
'may'           => 'May',
'jun'           => 'Hun',
'jul'           => 'Hul',
'aug'           => 'Ago',
'sep'           => 'Sep',
'oct'           => 'Okt',
'nov'           => 'Nob',
'dec'           => 'Dis',

# Categories related messages
'pagecategories'           => '{{PLURAL:$1|Kategorya|Mga Kategorya}}',
'category_header'          => 'Mga pakli ha kategorya "$1"',
'subcategories'            => 'Mga ilarom nga kategorya',
'category-media-header'    => 'Media ha kategorya "$1"',
'category-empty'           => "''Ini nga kategorya ha yana waray mga pakli o media.''",
'hidden-categories'        => '{{PLURAL:$1|Tinago nga kategorya|Tinago nga mga kategorya}}',
'hidden-category-category' => 'Tinago nga mga kategorya', # Name of the category where hidden categories will be listed

'qbfind'         => 'Bilnga',
'qbbrowse'       => 'Igdalikyat',
'qbedit'         => 'Igliwat',
'qbpageoptions'  => 'Ini nga pakli',
'qbmyoptions'    => 'Akon mga pakli',
'qbspecialpages' => 'Mga ispisyal nga pakli',
'mypage'         => 'Akon pakli',
'mytalk'         => 'Akon paghingay',
'anontalk'       => 'Paghingay para hini nga IP',
'navigation'     => 'Paglayag',
'and'            => 'ngan',

'errorpagetitle'   => 'Sayop',
'returnto'         => 'Balik ngadto ha $1.',
'tagline'          => 'Tikang ha {{SITENAME}}',
'help'             => 'Bulig',
'search'           => 'Bilnga',
'searchbutton'     => 'Bilnga',
'go'               => 'Kadto-a',
'searcharticle'    => 'Kadto-a',
'history'          => 'Kaagi han pakli',
'history_short'    => 'Kaagi',
'info_short'       => 'Impormasyon',
'printableversion' => 'Maipapatik nga bersyon',
'permalink'        => 'Sumpay nga unob',
'edit'             => 'Igliwat',
'editthispage'     => 'Igliwat ini nga pakli',
'delete'           => 'Para-a',
'deletethispage'   => 'Para-a ini nga pakli',
'newpage'          => 'Bag-o nga pakli',
'talkpagelinktext' => 'Hiruhimangraw',
'specialpage'      => 'Ispisyal nga Pakli',
'personaltools'    => 'Kalugaringon nga mga garamiton',
'talk'             => 'Hiruhimangraw',
'views'            => 'Mga paglantaw',
'toolbox'          => 'Garamiton',
'otherlanguages'   => 'Ha iba nga mga yinaknan',
'jumpto'           => 'Laktaw ngadto ha:',
'jumptonavigation' => 'paglayag',
'jumptosearch'     => 'bilnga',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Mahitungod han {{SITENAME}}',
'currentevents'        => 'Mga panhitabo',
'currentevents-url'    => 'Project:Mga panhitabo',
'disclaimers'          => 'Mga Disclaimer',
'helppage'             => 'Help:Sulod',
'mainpage'             => 'Syahan nga Pakli',
'mainpage-description' => 'Syahan nga Pakli',
'policy-url'           => 'Project:Polisiya',
'portal'               => 'Ganghaan han Komunidad',
'portal-url'           => 'Project:Ganghaan han Komunidad',

'badaccess-group0' => 'Diri ka gintutugutan pagbuhat han buruhaton nga imo ginhangyo.',

'versionrequired'     => 'Kinahanglan an Bersion $1 han MediaWiki',
'versionrequiredtext' => 'Kinahanglan an Bersyon $1 han MediaWiki ha paggamit hini nga pakli.  Kitaa an [[Special:Version|bersyon nga pakli]].',

'youhavenewmessages'      => 'Mayda ka $1 ($2).',
'newmessageslink'         => 'bag-o nga mga mensahe',
'youhavenewmessagesmulti' => 'Mayda ka mga bag-o nga mensahe ha $1',
'editsection'             => 'igliwat',
'hidetoc'                 => 'tago-a',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Artikulo',
'nstab-special'   => 'Ispisyal',
'nstab-project'   => 'Pakli han proyekto',
'nstab-mediawiki' => 'Mensahe',
'nstab-template'  => 'Plantilya',
'nstab-help'      => 'Pakli hin bulig',
'nstab-category'  => 'Kategorya',

# Login and logout pages
'login'              => 'Sakob',
'userlogin'          => 'Sakob/Pagrehistro',
'userlogout'         => 'Gawas',
'yourlanguage'       => 'Yinaknan:',
'loginlanguagelabel' => 'Yinaknan: $1',

# History pages
'viewpagelogs' => 'Kitaa an mga log para hini nga pakli',
'next'         => 'sunod',
'last'         => 'kataposan',
'page_first'   => 'syahan',
'page_last'    => 'kataposan',

# Search results
'prevn'        => 'naha-una nga $1',
'nextn'        => 'sunod nga $1',
'viewprevnext' => 'Kitaa an ($1) ($2) ($3)',
'powersearch'  => 'Bilnga',

# Preferences page
'preferences'       => 'Mga karuyag',
'mypreferences'     => 'Akon mga karuyag',
'datetime'          => 'Pitsa ngan oras',
'searchresultshead' => 'Bilnga',
'timezonelegend'    => 'Zona hin oras',
'localtime'         => 'Oras nga lokal',

# Recent changes
'recentchanges'   => 'Mga kabag-ohan',
'hide'            => 'Tago-a',
'minoreditletter' => 'g',
'newpageletter'   => 'B',

# Recent changes linked
'recentchangeslinked' => 'Mga may kalabotan nga binag-o',

# Upload
'upload'    => 'Pagkarga hin file',
'uploadbtn' => 'Igkarga an file',

# Special:ImageList
'imagelist_date' => 'Pitsa',
'imagelist_name' => 'Ngaran',

# Image description page
'filehist-datetime' => 'Pitsa/Oras',
'imagelinks'        => 'Mga sumpay',
'linkstoimage'      => 'Nasumpay hini nga fayl an mga nasunod nga mga pakli:',
'nolinkstoimage'    => 'Waray mga pakli nga nasumpay hini nga fayl.',
'sharedupload'      => 'Ini nga fayl ginsaro nga pagkarga ngan puyde gamiton hin iba nga mga proyekto.',
'shareduploadwiki'  => 'Alayon pagkita han $1 para hin dugang nga impormasyon.',

# Unused templates
'unusedtemplateswlh' => 'iba nga mga sumpay',

# Random page
'randompage' => 'Bisan ano nga pakli',

# Statistics
'statistics' => 'Mga estadistika',
'sitestats'  => '{{SITENAME}} nga mga estadistika',

# Miscellaneous special pages
'longpages' => 'Haglaba nga mga pakli',
'move'      => 'Balhina',

# Book sources
'booksources-go' => 'Kadto-a',

# Special:Log
'log-search-submit' => 'Kadto-a',

# Special:AllPages
'allpages'       => 'Ngatanan nga mga pakli',
'allarticles'    => 'Ngatanan nga mga artikulo',
'allpagesprev'   => 'Naha-una',
'allpagesnext'   => 'Sunod',
'allpagessubmit' => 'Kadto-a',

# Special:Categories
'categories' => 'Mga Kategorya',

# Watchlist
'watchlist'     => 'Akon barantayan',
'mywatchlist'   => 'Akon angay timan-an',
'watch'         => 'Bantayi',
'watchthispage' => 'Bantayi ini nga pakli',

# Delete/protect/revert
'deletedtext' => 'Ginpara an "<nowiki>$1</nowiki>".
Kitaa an $2 para hin talaan han mga gibag-ohi nga mga ginpamara.',

# Contributions
'mycontris' => 'Akon mga ámot',

# What links here
'whatlinkshere' => 'Mga nasumpay dinhi',

# Block/unblock
'ipblocklist-submit' => 'Bilnga',

# Special:NewImages
'ilsubmit' => 'Bilnga',

# Multipage image navigation
'imgmultipageprev' => '← naha-una nga pakli',
'imgmultipagenext' => 'sunod nga pakli →',

# Table pager
'table_pager_next'         => 'Sunod nga pakli',
'table_pager_prev'         => 'Naha-una nga pakli',
'table_pager_first'        => 'Una nga pakli',
'table_pager_last'         => 'Kataposan nga pakli',
'table_pager_limit'        => 'Igpakita in $1 nga mga item ha tagsa pakli',
'table_pager_limit_submit' => 'Kadto-a',

# Size units
'size-bytes'     => '$1 nga B',
'size-kilobytes' => '$1 nga KB',
'size-megabytes' => '$1 nga MB',
'size-gigabytes' => '$1 nga GB',

# Special:SpecialPages
'specialpages' => 'Mga Ispisyal nga Pakli',

);
