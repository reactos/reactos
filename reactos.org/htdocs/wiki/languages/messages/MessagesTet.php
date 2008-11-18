<?php
/** Tetum (Tetun)
 *
 * @ingroup Language
 * @file
 *
 * @author MF-Warburg
 */

$skinNames = array(
	'standard'    => 'Klásiku',
	'cologneblue' => 'Kolónia azúl',
	'myskin'      => 'MySkin',
	'chick'       => 'Manu',
);

$namespaceNames = array(
	NS_MEDIA          => 'Media',
	NS_SPECIAL        => 'Espesiál',
	NS_MAIN           => '',
	NS_TALK           => 'Diskusaun',
	NS_USER           => 'Uza-na\'in',
	NS_USER_TALK      => 'Diskusaun_Uza-na\'in',
	# NS_PROJECT set by \$wgMetaNamespace
	NS_PROJECT_TALK   => 'Diskusaun_$1',
	NS_IMAGE          => 'Imajen',
	NS_IMAGE_TALK     => 'Diskusaun_Imajen',
	NS_MEDIAWIKI      => 'MediaWiki',
	NS_MEDIAWIKI_TALK => 'Diskusaun_MediaWiki',
	NS_TEMPLATE       => 'Template',
	NS_TEMPLATE_TALK  => 'Diskusaun_Template',
	NS_HELP           => 'Ajuda',
	NS_HELP_TALK      => 'Diskusaun_Ajuda',
	NS_CATEGORY       => 'Kategoria',
	NS_CATEGORY_TALK  => 'Diskusaun_Kategoria',
);

$namespaceAliases = array(
	"Kategoría"           => NS_CATEGORY,
	"Diskusaun_Kategoría" => NS_CATEGORY_TALK,
);

$specialPageAliases = array(
	'CreateAccount'             => array( 'Rejista' ),
	'Preferences'               => array( 'Preferénsia' ),
	'Watchlist'                 => array( 'Lista_hateke' ),
	'Recentchanges'             => array( 'Mudansa_foufoun_sira' ),
	'Upload'                    => array( 'Tau_iha_arkivu_laran' ),
	'Imagelist'                 => array( 'Lista_imajen' ),
	'Newimages'                 => array( 'Imajen_foun' ),
	'Listusers'                 => array( 'Lista_uza-na\'in' ),
	'Statistics'                => array( 'Estatístika' ),
	'Randompage'                => array( 'Pájina_ruma' ),
	'Shortpages'                => array( 'Pájina_badak' ),
	'Longpages'                 => array( 'Pájina_naruk' ),
	'Protectedpages'            => array( 'Pájina_sira-ne\'ebé_proteje_tiha' ),
	'Allpages'                  => array( 'Pájina_hotu' ),
	'Ipblocklist'               => array( 'Lista_ema_sira-ne\'ebé_blokeiu_tiha' ),
	'Specialpages'              => array( 'Pájina_espesiál_sira' ),
	'Contributions'             => array( 'Kontribuisaun' ),
	'Emailuser'                 => array( 'Haruka_korreiu_eletróniku' ),
	'Whatlinkshere'             => array( 'Pájina_sira_ne\'ebé_bá_iha_ne\'e' ),
	'Movepage'                  => array( 'Book' ),
	'Categories'                => array( 'Kategoria' ),
	'Export'                    => array( 'Esporta' ),
	'Version'                   => array( 'Versaun' ),
	'Allmessages'               => array( 'Mensajen_hotu' ),
	'Blockip'                   => array( 'Blokeiu' ),
	'Undelete'                  => array( 'Restaurar' ),
	'Userrights'                => array( 'Kuana', 'Priviléjiu' ),
	'Mypage'                    => array( 'Ha\'u-nia_pájina' ),
	'Listadmins'                => array( 'Lista_administradór' ),
	'Search'                    => array( 'Buka' ),
	'Withoutinterwiki'          => array( 'Laiha_interwiki' ),
);

$messages = array(
# User preference toggles
'tog-hideminor'          => "Lá'os hatudu osan-rahun sira iha mudansa foufoun sira",
'tog-watchlisthidebots'  => 'Hamsumik bot iha lista hateke',
'tog-watchlisthideminor' => 'Hamsumik osan-rahun iha lista hateke',

'underline-always' => 'Sempre',
'underline-never'  => 'Nunka',

# Dates
'sunday'        => 'Loron-domingu',
'monday'        => 'Loron-segunda',
'tuesday'       => 'Loron-tersa',
'wednesday'     => 'Loron-kuarta',
'thursday'      => 'Loron-kinta',
'friday'        => 'Loron-sesta',
'saturday'      => 'Loron-sábadu',
'sun'           => 'Dom',
'mon'           => 'Seg',
'tue'           => 'Ter',
'wed'           => 'Kua',
'thu'           => 'Kin',
'fri'           => 'Ses',
'sat'           => 'Sáb',
'january'       => 'Janeiru',
'february'      => 'Fevereiru',
'march'         => 'Marsu',
'april'         => 'Abríl',
'may_long'      => 'Maiu',
'june'          => 'Juñu',
'july'          => 'Jullu',
'august'        => 'Agostu',
'september'     => 'Setembru',
'october'       => 'Outubru',
'november'      => 'Novembru',
'december'      => 'Dezembru',
'january-gen'   => 'Janeiru nian',
'february-gen'  => 'Fevereiru nian',
'march-gen'     => 'Marsu nian',
'april-gen'     => 'Abríl nian',
'may-gen'       => 'Maiu nian',
'june-gen'      => 'Juñu nian',
'july-gen'      => 'Jullu nian',
'august-gen'    => 'Agostu nian',
'september-gen' => 'Setembru nian',
'october-gen'   => 'Outubru nian',
'november-gen'  => 'Novembru nian',
'december-gen'  => 'Dezembru nian',
'jan'           => 'Jan',
'feb'           => 'Fev',
'mar'           => 'Mar',
'apr'           => 'Abr',
'may'           => 'Mai',
'jun'           => 'Jun',
'jul'           => 'Jul',
'aug'           => 'Ago',
'sep'           => 'Set',
'oct'           => 'Out',
'nov'           => 'Nov',
'dec'           => 'Dez',

# Categories related messages
'pagecategories'         => '{{PLURAL:$1|Kategoría|Kategoría}}',
'category_header'        => 'Artigu iha kategoría "$1"',
'category-empty'         => "''Kategoria ne'e agora la iha pájina sira.''",
'listingcontinuesabbrev' => 'kont.',

'about'          => 'Kona-ba',
'article'        => 'Pájina',
'qbfind'         => 'Hetan',
'qbedit'         => 'Edita',
'qbpageoptions'  => "Pájina ne'e",
'qbmyoptions'    => "Ha'u-nia pájina sira",
'qbspecialpages' => 'Pájina espesiál sira',
'moredotdotdot'  => 'Barak liu...',
'mypage'         => "Ha'u-nia pájina",
'mytalk'         => "Ha'u-nia diskusaun",
'anontalk'       => "Diskusaun ba IP ne'e",
'navigation'     => 'Hatudu-dalan',
'and'            => 'ho',

'errorpagetitle'    => 'Sala',
'tagline'           => 'Husi {{SITENAME}}',
'help'              => 'Ajuda',
'search'            => 'Buka',
'searchbutton'      => 'Buka',
'go'                => 'Bá',
'searcharticle'     => 'Pájina',
'history'           => 'Istória pájina',
'history_short'     => 'Istória',
'info_short'        => 'Informasaun',
'printableversion'  => 'Versaun ba impresaun',
'permalink'         => 'Ligasaun mahelak',
'print'             => 'Imprime',
'edit'              => 'Edita',
'create'            => 'Kria',
'editthispage'      => "Edita pájina ne'e",
'delete'            => 'Halakon',
'deletethispage'    => "Halakon pájina ne'e",
'undelete_short'    => 'Restaurar {{PLURAL:$1|versaun ida|$1 versaun}}',
'protect'           => 'Proteje',
'protectthispage'   => "Proteje pájina ne'e",
'unprotect'         => 'La proteje',
'unprotectthispage' => "La proteje pájina ne'e",
'newpage'           => 'Pájina foun',
'talkpage'          => "Diskusaun kona-ba pájina ne'e",
'talkpagelinktext'  => 'Diskusaun',
'specialpage'       => 'Pájina espesiál',
'postcomment'       => 'Tau tan komentáriu ida',
'talk'              => 'Diskusaun',
'toolbox'           => 'Kaixa besi nian',
'userpage'          => "Haree pájina uza-na'in",
'projectpage'       => 'Haree pájina projetu nian',
'imagepage'         => 'Haree pájina imajen nian',
'mediawikipage'     => 'Haree pájina mensajen nian',
'viewhelppage'      => 'Haree pájina ajuda',
'categorypage'      => 'Haree pájina kategoría nian',
'viewtalkpage'      => 'Haree diskusaun',
'otherlanguages'    => 'Iha lian seluk',
'protectedpage'     => "Pájina ne'ebé naproteje",
'jumpto'            => 'Bá:',
'jumptonavigation'  => 'hatudu-dalan',
'jumptosearch'      => 'buka',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Kona-ba {{SITENAME}}',
'aboutpage'            => 'Project:Kona-ba',
'copyright'            => 'Testu pájina nian iha $1 okos.',
'currentevents'        => 'Mamosuk atuál sira',
'currentevents-url'    => 'Project:Mamosuk atuál sira',
'disclaimers'          => 'Avisu legál',
'disclaimerpage'       => 'Project:Avisu legál',
'edithelp'             => 'Ajuda kona-ba edita',
'edithelppage'         => 'Help:Edita',
'mainpage'             => 'Pájina Mahuluk',
'mainpage-description' => 'Pájina Mahuluk',
'portal'               => 'Portál komunidade nian',
'portal-url'           => 'Project:Portál komunidade nian',
'privacy'              => 'Polítika privasidade nian',
'privacypage'          => 'Project:Polítika privasidade nian',

'badaccess-group0' => "Ó la bele halo ne'e.",
'badaccess-group1' => "Ba halo ne'e tenke iha lubu $1.",
'badaccess-group2' => "Ba halo ne'e tenke iha lubu ida husi $1.",
'badaccess-groups' => "Ba halo ne'e tenke iha lubu ida husi $1.",

'versionrequired'     => 'Presiza MediaWiki versaun $1',
'versionrequiredtext' => "Presiza MediaWiki versaun $1 ba uza pájina ne'e. Haree [[Special:Version|pájina versaun]].",

'ok'                      => 'OK',
'retrievedfrom'           => 'Husi "$1"',
'youhavenewmessages'      => 'Ó iha $1 ($2).',
'newmessageslink'         => 'mensajen foun',
'youhavenewmessagesmulti' => 'Ó iha mensajen foun sira iha $1',
'editsection'             => 'edita',
'editold'                 => 'edita',
'editsectionhint'         => 'Edita parte $1 pájina nian',
'showtoc'                 => 'hatudu',
'hidetoc'                 => 'subar',
'thisisdeleted'           => 'Haree ka restaurar $1?',
'viewdeleted'             => 'Haree $1?',
'site-rss-feed'           => 'Feed RSS $1',
'site-atom-feed'          => 'Feed Atom $1',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Pájina',
'nstab-user'      => "Pájina uza-na'in",
'nstab-special'   => 'Espesiál',
'nstab-project'   => 'Pájina projetu nian',
'nstab-mediawiki' => 'Mensajen',
'nstab-help'      => 'Pájina ajuda',
'nstab-category'  => 'Kategoria',

# Main script and global functions
'nosuchspecialpage' => "Pájina espesiál ne'e la iha",

# General errors
'error'         => 'Sala',
'viewsourcefor' => 'ba $1',

# Login and logout pages
'yourname'                  => "Naran uza-n'in:",
'nav-login-createaccount'   => 'Log in / kriar konta ida',
'userlogin'                 => 'Log in / kriar konta ida',
'nologinlink'               => 'Registrar',
'createaccount'             => "Registrar uza-na'in",
'userexists'                => "Uza-na'in ne'e ona iha wiki. Favór ida lori naran seluk.",
'youremail'                 => 'Korreiu eletróniku:',
'username'                  => "Naran uza-na'in:",
'uid'                       => "Uza-na'in ID:",
'yourlanguage'              => 'Lian:',
'email'                     => 'Korreiu eletróniku',
'prefs-help-email-required' => 'Haruka diresaun korreiu eletróniku.',
'accountcreated'            => "Registrar tiha uza-na'in",
'loginlanguagelabel'        => 'Lian: $1',

# Password reset dialog
'resetpass_text' => "<!-- Hakerek testu iha ne'e -->",

# Edit page toolbar
'link_tip'     => 'Ligasaun ba laran',
'extlink_tip'  => "Ligasaun ba li'ur (tau tan http://)",
'image_sample' => 'Ezemplu.jpg',
'media_sample' => 'Ezemplu.ogg',

# Edit pages
'minoredit'         => "Ne'e osan-rahun",
'watchthis'         => "Hateke pájina ne'e",
'anoneditwarning'   => 'Ó lá\'os "log-in" iha momentu.',
'blockedtitle'      => "Uza-na'in nablokeiu",
'newarticle'        => '(Foun)',
'noarticletext'     => "Iha momentu lá'os testu iha pájina ne'e, bele [[Special:Search/{{PAGENAME}}|buka naran pájina nian]] iha pájina seluk ka [{{fullurl:{{FULLPAGENAME}}|action=edit}} edita pájina ne'e].",
'editing'           => 'Edita $1',
'editingcomment'    => 'Edita $1 (komentáriu)',
'yourtext'          => 'Ó-nia testu',
'yourdiff'          => 'Diferensa sira',
'nocreate-loggedin' => 'Ó la iha kuana kria pájina foun iha {{SITENAME}}.',

# Account creation failure
'cantcreateaccounttitle' => "La bele registrar uza-na'in",

# History pages
'currentrev'          => 'Versaun atuál',
'revisionasof'        => 'Versaun $1 nian',
'previousrevision'    => '←Versaun tuan liu',
'nextrevision'        => 'Versaun foun liu→',
'currentrevisionlink' => 'Versaun atuál',
'cur'                 => 'atuál',
'next'                => 'oinmai',
'last'                => 'ikus',
'page_first'          => 'uluk',
'histfirst'           => 'sedu liu hotu',
'historyempty'        => '(mamuk)',

# Revision feed
'history-feed-item-nocomment' => '$1 iha $2', # user at time

# Revision deletion
'rev-delundel'        => 'hatudu/subar',
'revisiondelete'      => 'Halakon/restaurar versaun',
'revdelete-hide-user' => "Subar naran edita-na'in/IP",

# Diffs
'lineno' => 'Liña $1:',

# Search results
'noexactmatch' => "'''Pájina ''$1'' la iha.''' Ó bele [[:$1|kria pájina ne'e]].",
'prevn'        => 'molok $1',
'nextn'        => 'oinmai $1',
'viewprevnext' => 'Haree ($1) ($2) ($3)',
'powersearch'  => 'Buka',

# Preferences page
'mypreferences'     => "Ha'u-nia preferénsia",
'prefs-rc'          => 'Mudansa foufoun sira',
'prefs-watchlist'   => 'Lista hateke',
'textboxsize'       => 'Edita',
'searchresultshead' => 'Buka',

# User rights
'userrights'               => "Filak kuana uza-na'in", # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => "Filak lubu uza-na'in",
'userrights-user-editname' => "Hakerek naran uza-na'in ida-nian:",
'editusergroup'            => "Filak lubu uza-na'in",
'userrights-editusergroup' => "Filak lubu uza-na'in",
'userrights-groupsmember'  => 'Membru iha:',
'userrights-no-interwiki'  => "Ó la bele filak kuana uza-na'in iha wiki seluk.",

# Groups
'group'            => 'Lubu:',
'group-bot'        => 'Bot sira',
'group-sysop'      => 'Administradór sira',
'group-bureaucrat' => 'Burokrata sira',
'group-all'        => '(hotu)',

'group-bot-member'        => 'Bot',
'group-sysop-member'      => 'Administradór',
'group-bureaucrat-member' => 'Burokrata',

'grouppage-bot'        => '{{ns:project}}:Bot sira',
'grouppage-sysop'      => '{{ns:project}}:Administradór sira',
'grouppage-bureaucrat' => '{{ns:project}}:Burokrata sira',

# User rights log
'rightslog'      => "Lista filak kuana uza-na'in",
'rightslogtext'  => "Ne'e lista ba filak lubu uza-na'in.",
'rightslogentry' => 'filak lubu $1 nian husi $2 ba $3',
'rightsnone'     => '(mamuk)',

# Recent changes
'recentchanges'   => 'Mudansa foufoun sira',
'rcshowhideminor' => '$1 osan-rahun sira',
'rcshowhidebots'  => '$1 bot sira',
'rcshowhideliu'   => '$1 ema rejista',
'rcshowhideanons' => '$1 ema anónimu',
'rcshowhidemine'  => "$1 ha'u-nia edita",
'diff'            => 'diferensa',
'hist'            => 'istória',
'hide'            => 'Hamsumik',
'show'            => 'Hatudu',
'minoreditletter' => 'o',
'newpageletter'   => 'F',
'boteditletter'   => 'b',

# Recent changes linked
'recentchangeslinked' => 'Muda sira',

# Upload
'upload'          => 'Tau iha arkivu laran',
'uploadbtn'       => 'Tau iha arkivu laran',
'watchthisupload' => "Hateke pájina ne'e",

# Special:ImageList
'imagelist_name' => 'Naran',
'imagelist_user' => "Uza-na'in",

# Image description page
'filehist-deleteall' => 'halakon hotu',
'filehist-deleteone' => 'halakon',
'filehist-current'   => 'atuál',
'filehist-datetime'  => 'Loron/Tempu',
'filehist-user'      => "Uza-na'in",
'filehist-comment'   => 'Komentáriu',
'imagelinks'         => 'Ligasaun',

# File reversion
'filerevert-comment' => 'Komentáriu:',

# File deletion
'filedelete'         => 'Halakon $1',
'filedelete-comment' => 'Komentáriu:',
'filedelete-submit'  => 'Halakon',

# Random page
'randompage' => 'Pájina ruma',

# Statistics
'statistics'    => 'Estátistika',
'sitestats'     => '{{SITENAME}} estátistika',
'userstatstext' => "Iha ne'e registrar tiha ona '''$1''' {{PLURAL:$1|[[Special:ListUsers|uza-na'in]]|[[Special:ListUsers|uza-na'in]]}}, '''$2''' (ka '''$4%''') $5.",

'brokenredirects-edit'   => '(edita)',
'brokenredirects-delete' => '(halakon)',

# Miscellaneous special pages
'nlinks'            => '$1 ligasaun',
'nmembers'          => '$1 {{PLURAL:$1|membru|membru}}',
'nrevisions'        => '$1 {{PLURAL:$1|versaun|versaun}}',
'shortpages'        => 'Pájina badak',
'longpages'         => 'Pájina naruk',
'listusers'         => "Lista uza-na'in",
'newpages'          => 'Pájina foun',
'newpages-username' => "Naran uza-na'in:",
'ancientpages'      => 'Pájina tuan liu hotu sira',
'move'              => 'Book',
'movethispage'      => "Book pájina ne'e",

# Book sources
'booksources-go' => 'Bá',

# Special:Log
'specialloguserlabel' => "Uza-na'in:",
'log-search-submit'   => 'Bá',

# Special:AllPages
'allpages'       => 'Pájina hotu',
'alphaindexline' => "$1 to'o $2",
'nextpage'       => 'Pájina oinmai ($1)',
'allarticles'    => 'Pájina hotu',
'allpagesnext'   => 'Oinmai',
'allpagessubmit' => 'Bá',

# Special:Categories
'categories' => 'Kategoría',

# Special:ListUsers
'listusers-submit' => 'Hatudu',

# Special:ListGroupRights
'listgrouprights-group'  => 'Lubu',
'listgrouprights-rights' => 'Priviléjiu',

# E-mail user
'emailuser'       => "Haruka korreiu eletróniku ba uza-na'in ne'e",
'defemailsubject' => '{{SITENAME}} korreiu eletróniku',
'noemailtitle'    => "Lá'os diresaun korreiu eletróniku",
'emailsend'       => 'Haruka',

# Watchlist
'watchlist'            => "Ha'u-nia lista hateke",
'mywatchlist'          => "Ha'u-nia lista hateke",
'watchlistfor'         => "('''$1''' nian)",
'addedwatch'           => 'tau tan tiha ba lista hateke',
'watch'                => 'Hateke',
'watchthispage'        => "Hateke pájina ne'e",
'unwatch'              => 'La hateke',
'watchlist-hide-bots'  => 'Hamsumik edita "bot" sira',
'watchlist-hide-own'   => "Hamsumik edita ha'u-nia",
'watchlist-show-minor' => 'Hatudu osan-rahun',
'watchlist-hide-minor' => 'Subar osan-rahun',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'Hateke...',
'unwatching' => 'La hateke...',

'enotif_newpagetext'           => "Ne'e pájina foun.",
'enotif_impersonal_salutation' => "Uza-na'in {{SITENAME}} nian",
'changed'                      => 'filak',
'created'                      => 'kria',

# Delete/protect/revert
'deletepage'                  => 'Halakon pájina',
'excontent'                   => "testu iha pájina: '$1'",
'excontentauthor'             => "testu iha pájina: '$1' (no ema ida de'it ne'ebé kontribui '[[Special:Contributions/$2|$2]]')",
'exblank'                     => 'pájina mamuk',
'delete-legend'               => 'Halakon',
'deletedarticle'              => 'halakon "[[$1]]"',
'dellogpage'                  => 'Lista halakon',
'deletionlog'                 => 'lista halakon',
'deletecomment'               => 'Tansá ó halakon:',
'protectedarticle'            => 'proteje "[[$1]]"',
'protectcomment'              => 'Komentáriu:',
'protectexpiry'               => "to'o:",
'protect-fallback'            => 'Presiza kuana "$1"',
'protect-level-autoconfirmed' => 'Blokeiu ema anónimu',
'protect-level-sysop'         => "de'it administradór",
'protect-expiring'            => "to'o $1 (UTC)",
'protect-cantedit'            => "Ó la bele filak proteje pájina ne'e nian, tan ba ó la bele edita pájina ne'e.",
'restriction-type'            => 'Kuana:',

# Restrictions (nouns)
'restriction-edit'   => 'Edita',
'restriction-move'   => 'Book',
'restriction-create' => 'Kria',

# Undelete
'undelete'               => 'Haree pájina halakon tiha',
'undeletebtn'            => 'Restaurar',
'undeletelink'           => 'restaurar',
'undeletecomment'        => 'Komentáriu:',
'undeletedarticle'       => 'restaurar "[[$1]]"',
'undeletedrevisions'     => 'restaurar $1 {{PLURAL:$1|versaun|versaun}}',
'undelete-search-submit' => 'Buka',

# Namespace form on various pages
'blanknamespace' => '(Prinsipál)',

# Contributions
'contributions' => "Kontribuisaun uza-na'in",
'mycontris'     => "Ha'u-nia kontribuisaun",

'sp-contributions-newbies'  => "Hatudu de'it kontribuisaun uza-na'in foun sira-nia",
'sp-contributions-search'   => 'Buka kontribuisaun',
'sp-contributions-username' => "Diresaun IP ka naran uza-na'in:",
'sp-contributions-submit'   => 'Buka',

# What links here
'whatlinkshere'           => "Artigu sira ne'ebé bá iha ne'e",
'whatlinkshere-title'     => 'Pájina sira ne\'ebé bá "$1".',
'whatlinkshere-page'      => 'Pájina:',
'linklistsub'             => '(Lista ligasaun nian)',
'linkshere'               => "Pájina sira ne'e link ba '''[[:$1]]''':",
'whatlinkshere-prev'      => '{{PLURAL:$1|oinmai|oinmai $1}}',
'whatlinkshere-next'      => '{{PLURAL:$1|molok|molok $1}}',
'whatlinkshere-links'     => '← ligasaun',
'whatlinkshere-hidelinks' => '$1 ligasaun',

# Block/unblock
'blockip'                 => "Blokeiu uza-na'in",
'blockip-legend'          => "Blokeiu uza-na'in",
'ipboptions'              => '2 hours:2 hours,1 loron:1 day,3 Loron:3 days,1 semana:1 week,2 semana:2 weeks,1 fulan:1 month,3 fulan:3 months,6 fulan:6 months,1 tinan:1 year,infinite:infinite', # display1:time1,display2:time2,...
'ipbotheroption'          => 'seluk',
'ipblocklist'             => "Ema anónimu no rejista ne'ebé nablokeiu",
'ipblocklist-username'    => "Naran uza-na'in ka IP:",
'ipblocklist-submit'      => 'Buka',
'blocklink'               => 'blokeiu',
'unblocklink'             => 'la blokeiu',
'contribslink'            => 'kontribuisaun',
'block-log-flags-noemail' => 'korreiu eletróniku blokeiu',
'ipb_already_blocked'     => '"$1" nablokeiu tiha ona',

# Move page
'move-page'               => 'Book $1',
'move-page-legend'        => 'Book pájina',
'movearticle'             => 'Book pájina:',
'movenotallowed'          => 'Ó la bele book pájina iha {{SITENAME}}.',
'newtitle'                => 'Naran foun:',
'move-watch'              => "Hateke pájina ne'e",
'movepagebtn'             => 'Book pájina',
'movedto'                 => 'book tiha ba',
'movetalk'                => 'Book pájina diskusaun mós',
'1movedto2'               => 'book tiha [[$1]] ba [[$2]]',
'movelogpage'             => 'Lista book',
'delete_and_move'         => 'Halakon ho book',
'delete_and_move_confirm' => 'Sin, halakon pájina',

# Export
'export' => 'Esporta pájina sira',

# Namespace 8 related
'allmessagesname'    => 'Naran',
'allmessagescurrent' => 'Testu atuál',

# Import log
'import-logentry-interwiki-detail' => '$1 {{PLURAL:$1|versaun|versaun}} husi $2',

# Tooltip help for the actions
'tooltip-pt-userpage'      => "Ha'u-nia pájina uza-na'in",
'tooltip-pt-mytalk'        => "Ha'u-nia pájina diskusaun",
'tooltip-pt-preferences'   => "Ha'u-nia preferénsia",
'tooltip-pt-mycontris'     => "Lista ha'u-nia kontribuisaun",
'tooltip-ca-addsection'    => "Tau tan komentáriu ida ba diskusaun ne'e.",
'tooltip-ca-protect'       => "Proteje pájina ne'e",
'tooltip-ca-delete'        => "Halakon pájina ne'e",
'tooltip-ca-move'          => "Book pájina ne'e",
'tooltip-ca-watch'         => "Tau tan pájina ne'e ba ó-nia lista hateke",
'tooltip-search'           => 'Buka iha {{SITENAME}}',
'tooltip-n-mainpage'       => 'Vizita Pájina Mahuluk',
'tooltip-n-portal'         => "Kona-ba projetu, ne'ebé ó bele halo, iha ne'ebé ó hetan saida",
'tooltip-n-recentchanges'  => "Lista mudansa foufoun sira iha wiki ne'e.",
'tooltip-n-randompage'     => 'Hola pájina ruma',
'tooltip-n-help'           => 'Hatudu pájina ajuda.',
'tooltip-t-whatlinkshere'  => "Lista pájina nian ne'ebé bá iha ne'e",
'tooltip-t-contributions'  => "Haree lista kontribuisaun uza-na'in ne'e nian",
'tooltip-t-upload'         => 'Tau iha arkivu laran',
'tooltip-t-specialpages'   => 'Lista pájina espesiál hotu nian',
'tooltip-ca-nstab-user'    => "Haree pájina uza-na'in",
'tooltip-ca-nstab-project' => 'Haree pájina projetu nian',
'tooltip-minoredit'        => "Halo ne'e osan-rahun",
'tooltip-watch'            => "Tau tan pájina ne'e ba ó-nia lista hateke",

# Browsing diffs
'previousdiff' => '←Versaun molok',
'nextdiff'     => 'Versaun oinmai→',

# Special:NewImages
'showhidebots' => '($1 bot sira)',
'ilsubmit'     => 'Buka',

# EXIF tags
'exif-artist' => 'Autór',

'exif-meteringmode-255' => 'Seluk',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'hotu',
'imagelistall'     => 'hotu',
'watchlistall2'    => 'hotu',
'namespacesall'    => 'hotu',
'monthsall'        => 'hotu',

# action=purge
'confirm_purge_button' => 'OK',

# Multipage image navigation
'imgmultipageprev' => '← pájina molok',
'imgmultipagenext' => 'pájina oinmai →',
'imgmultigo'       => 'Bá!',

# Table pager
'table_pager_next'         => 'Pájina oinmai',
'table_pager_prev'         => 'Pájina molok',
'table_pager_first'        => 'Pájina uluk',
'table_pager_limit_submit' => 'Bá',

# Auto-summaries
'autosumm-new' => 'Pájina foun: $1',

# Watchlist editor
'watchlistedit-normal-title' => 'Filak lista hateke',

# Watchlist editing tools
'watchlisttools-edit' => 'Haree no edita lista hateke',

# Special:Version
'version'                  => 'Versaun', # Not used as normal message but as header for the special page itself
'version-specialpages'     => 'Pájina espesiál',
'version-other'            => 'Seluk',
'version-version'          => 'Versaun',
'version-software-version' => 'Versaun',

# Special:FileDuplicateSearch
'fileduplicatesearch-submit' => 'Buka',

# Special:SpecialPages
'specialpages' => 'Pájina espesiál sira',

);
