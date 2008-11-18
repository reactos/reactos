<?php
/** Tachelhit (Tašlḥiyt)
 *
 * @ingroup Language
 * @file
 *
 * @author Zanatos
 */

$fallback = 'ar';

$messages = array(
# User preference toggles
'tog-underline'      => 'sttr f do-izdayn:',
'tog-justify'        => 'skr lɛrd n-stor ɣ togzimin aygiddi',
'tog-numberheadings' => 'nmra n nsmiat wahdot',
'tog-showtoolbar'    => 'sbaynd tizikrt n tbddil(JavaScript)',
'tog-editwidth'      => 'tasnduqt n tbddil arttamz sfha kollot',
'tog-watchcreations' => 'zaydn tiwriqin lli origh i tochwafin-ino',
'tog-watchdefault'   => 'zaydn tiwriqin lli bdlgh i tochwafin-ino',
'tog-watchmoves'     => 'zaydn tiwriqin lli smattigh i tochwafin-ino',
'tog-watchdeletion'  => 'zaydn tiwriqin lli msḥgh i tochwafin-ino',

'underline-always' => 'dima',
'underline-never'  => 'ḥtta manak',

# Dates
'sunday'        => 'assamass',
'monday'        => 'aynass',
'tuesday'       => 'assinas',
'wednesday'     => 'akrass',
'thursday'      => 'akouass',
'friday'        => 'assimuass',
'saturday'      => 'assidias',
'sun'           => 'assamass',
'mon'           => 'aynass',
'tue'           => 'assinas',
'wed'           => 'akrass',
'thu'           => 'akouass',
'fri'           => 'assimuass',
'sat'           => 'assidias',
'january'       => 'yennayer',
'february'      => 'xubrayr',
'march'         => 'Mars',
'april'         => 'ibrir',
'may_long'      => 'mayyuh',
'june'          => 'yunyu',
'july'          => 'yulyu',
'august'        => 'ɣusht',
'september'     => 'shutanbir',
'october'       => 'kṭuber',
'november'      => 'Nuwember',
'december'      => 'Dujanbir',
'january-gen'   => 'yennayer',
'february-gen'  => 'xubrayr',
'march-gen'     => 'Mars',
'april-gen'     => 'ibrir',
'may-gen'       => 'mayyuh',
'june-gen'      => 'yunyu',
'july-gen'      => 'yulyu',
'august-gen'    => 'ɣusht',
'september-gen' => 'shutanbir',
'october-gen'   => 'kṭuber',
'november-gen'  => 'Nuwember',
'december-gen'  => 'Dujanbir',
'jan'           => 'yennayer',
'feb'           => 'xubrayr',
'mar'           => 'Mars',
'apr'           => 'Ibrir',
'may'           => 'Mayyuh',
'jun'           => 'yunyu',
'jul'           => 'yulyu',
'aug'           => 'ɣusht',
'sep'           => 'shutanbir',
'oct'           => 'kṭuber',
'nov'           => 'Nuwember',
'dec'           => 'Dujanbir',

# Categories related messages
'pagecategories'  => '{{PLURAL:$1|amggrd|imggrad}}',
'category_header' => 'tiwriqin ɣ-omggrd "$1"',
'subcategories'   => 'imggrad-mzin',

'cancel'         => 'qn',
'qbfind'         => 'siggl',
'qbedit'         => 'bddl',
'qbpageoptions'  => 'tawriqt ad',
'qbmyoptions'    => 'tiwriqin niw',
'qbspecialpages' => 'tiwriqin tuzliyin',
'moredotdotdot'  => 'uggar...',
'mypage'         => 'tawriqt niw',
'mytalk'         => 'assays ino',
'and'            => 'z',

'tagline'          => 'mn {{SITENAME}}',
'help'             => 'lmɜiwna',
'search'           => 'siggl',
'searchbutton'     => 'siggl',
'go'               => 'ballak',
'searcharticle'    => 'ballak',
'printableversion' => 'igh trit ati tbзat',
'permalink'        => 'azday izawmn',
'edit'             => 'bddl',
'create'           => 'skr',
'delete'           => 'msḥ',
'newpage'          => 'tawriqt tamaynut',
'talkpagelinktext' => 'assays',
'specialpage'      => 'tawriqt tasebtart',
'personaltools'    => 'lmatarial ino',
'talk'             => 'assays',
'views'            => 'chofass',
'toolbox'          => "sndoq l'matarial",
'otherlanguages'   => 's tutlayin yadni',
'jumpto'           => 'ballak s:',
'jumptonavigation' => 'artɛom',
'jumptosearch'     => 'siggl',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'f {{SITENAME}}',
'aboutpage'            => "Project:f'",
'disclaimers'          => 'igh ortlla lmsoliya',
'disclaimerpage'       => 'Project:ortlla lmsoliya iɛomman',
'faq'                  => 'isqsitn li bahra itЗawadn',
'mainpage'             => 'tawriqt tamzwarut',
'mainpage-description' => 'tawriqt tamzwarut',
'privacy'              => "siassa n' lkhossossia",
'privacypage'          => "Project:ssiast n' lkhossossia",

'retrievedfrom'      => 'itsglbd mn "$1"',
'youhavenewmessages' => 'illa dark $1 ($2).',
'editsection'        => 'bddl',
'editold'            => 'bddl',
'editsectionhint'    => 'bdl section: $1',
'showtoc'            => 'sbaynd',
'hidetoc'            => 'ḥbou',
'site-rss-feed'      => "$1 lqm n' RSS",
'site-atom-feed'     => "$1 lqm n' atom",
'page-rss-feed'      => '"$1" tlqim RSS',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-user'      => 'tawriqt o-msxdam',
'nstab-mediawiki' => 'tabrat',
'nstab-category'  => 'amgrd',

# General errors
'nodb' => 'ornzdar annaf database $1',

# Login and logout pages
'welcomecreation'         => '== brrk darnɣ, $1! ==

lcont nk ati styaqyad. ador tot atbadlt {{SITENAME}} lmЗlomat nk.',
'loginpagetitle'          => 'ikchim o-msxdam',
'yourname'                => 'smiyt o-msxdam:',
'yourpassword'            => 'awal iḥdan:',
'yourpasswordagain'       => 'Зawd ara awal iḥdan:',
'nav-login-createaccount' => 'kchem / qiyd amskhdam amaynu',
'userlogin'               => 'kchem / qiyd amskhdam amaynu',
'userlogout'              => 'foɣ',
'yourlanguage'            => 'tutlayt:',
'loginlanguagelabel'      => 'tutlayt: $1',

# Edit pages
'noarticletext'    => 'ɣila orilla walo l-ktba ɣ tawriqt ad, tzdart [[Special:Search/{{PAGENAME}}|atsiglt smiyt n tawriqt ad]] 
ɣ tiwriqin yadni, nɣd [{{fullurl:{{FULLPAGENAME}}|action=edit}} atbdlt tawrikt ad]',
'copyrightwarning' => 'ikhssak atst izd kolchi tikkin noun ɣ {{SITENAME}} llan ɣdo $2 (zr $1 iɣ trit ztsnt uggar).
iɣ ortrit ayg ɣayli torit ḥor artisbadal wnna ka-iran, attid ortgt ɣid.<br />
ikhssak ola kiyi ador tnqilt ɣtamani yadni.
<strong>ador tgat ɣid ɣayli origan ḥor iɣzark orilli lidn nbab-ns!</strong>',

# Search results
'powersearch' => 'amsigl itqdmn',

# Preferences page
'timezonetext' => '¹lfrq nswayЗ gr loqt n ɣilli ɣ tllit d loqt n serveur (UTC).',
'localtime'    => 'loqt n ɣilli ɣtllit',
'servertime'   => 'loqt n serveur',

# Upload
'upload' => 'sΥlid afaylu',

# Special:AllPages
'alphaindexline' => '$1 ar $2',

# Special:Categories
'categories' => 'imggrad',

# Watchlist
'watch'   => 'zaydtin i tochwafin-niw',
'unwatch' => 'ḥiyd-t ɣ tachwafin ino',

# Contributions
'mycontris' => 'tikkin ino',

# Block/unblock
'contribslink' => 'tikkin',

# Tooltip help for the actions
'tooltip-pt-mytalk'       => 'tawriqt o-ssays ino',
'tooltip-pt-login'        => 'yofak atqiyt, mach han origa bziz.',
'tooltip-ca-talk'         => "assays f' mayllan ɣ twriqt ad",
'tooltip-search'          => 'siggl ɣ {{SITENAME}}',
'tooltip-n-mainpage'      => 'zord tawriqt tamzwarut',
'tooltip-n-portal'        => "f' usenfar, matzdart atitskrt, maniɣrattaft ɣayli trit",
'tooltip-n-recentchanges' => 'Umuɣ n yibeddlen imaynuten ɣ l-wiki',
'tooltip-n-help'          => 'tkhassak lmɛiwna ?achkid sɣid',
'tooltip-t-upload'        => 'sɣlid ifaylutn',
'tooltip-t-specialpages'  => 'kolchi tiwriqin tesbtarin',

'exif-gaincontrol-0' => 'walo',

'exif-subjectdistancerange-0' => 'orityawssan',

# Multipage image navigation
'imgmultigo' => 'ballak !',

# Table pager
'table_pager_first'        => 'tawriqt tamzwarut',
'table_pager_last'         => 'tawriqt tamgrut',
'table_pager_limit_submit' => 'ballak',

# Special:SpecialPages
'specialpages' => 'tiwriqin tesbtarin',

);
