<?php
/** Sranan Tongo (Sranantongo)
 *
 * @ingroup Language
 * @file
 *
 * @author Adfokati
 * @author Jordi
 * @author Ooswesthoesbes
 * @author Stretsh
 */

$fallback = 'nl';

$messages = array(
# User preference toggles
'tog-underline'               => 'Poti strepi ondro den miti:',
'tog-highlightbroken'         => 'Skrifi miti go na leygi papira <a href="" class="new">so</a> (tra fasi: leki disi<a href="" class="internal">?</a>).',
'tog-justify'                 => 'Fiti na ala tu sey',
'tog-hideminor'               => 'Kibri pikin kenki ini laste kenki',
'tog-extendwatchlist'         => 'Moro langa "Tan luku" réy',
'tog-usenewrc'                => 'Moro betre Laste Kenki (JavaScript)',
'tog-numberheadings'          => 'Gi den ede wan nomru sondro fu yepi',
'tog-showtoolbar'             => 'Sori Kenki-Wrokosani-barki (JavaScript)',
'tog-editondblclick'          => 'Naki tu tron fu kenki (JavaScript)',
'tog-editsection'             => 'Gi primisi fu kenki pisi-papira nanga [kenki]-miti',
'tog-editsectiononrightclick' => "Gi primisi fu kenki pisi-papira nanga wan naki n'a leti-anu sey na tapu wan pisi-ede (JavaScript)",
'tog-showtoc'                 => 'Sori san de (gi papira nanga moro leki 3 pisi-ede)',
'tog-rememberpassword'        => 'Memre mi psawortu',
'tog-editwidth'               => 'A kenki boksu span na marki na ala tu sei',
'tog-watchcreations'          => 'Tan luku den papira di mi meki',
'tog-watchdefault'            => 'Tan luku den papira di mi kenki',
'tog-watchmoves'              => 'Tan luku den papira di mi froysi',
'tog-watchdeletion'           => 'Tan luku den papira di mi puru',
'tog-minordefault'            => "Marki ala mi kenki leki 'pikin'",
'tog-previewontop'            => 'Fusi hey opo mi kenkibox libi si',
'tog-previewonfirst'          => 'Sori wan Si-na-fesi na a fosi kenki',
'tog-nocache'                 => 'No kebroiki cache',
'tog-enotifwatchlistpages'    => 'Seni mi wan E-mail te papira ini mi "Tan luku" rey kenki',
'tog-enotifusertalkpages'     => 'Seni mi wan E-mail te mi Taki papira kenki',
'tog-enotifminoredits'        => 'E-mail mi fu pikin kenki fu peprewoysi opo mi sirey',
'tog-enotifrevealaddr'        => 'Sori mi e-mail nen ini den e-mail boskopu',
'tog-shownumberswatching'     => 'Sori omeni kebroikiman e tan luku a papira disi',
'tog-externaleditor'          => 'Tan kebroiki wan dorosey kenki-wrokosani (soso gi sabiman - spesrutu seti de fanowdu gi disi)',
'tog-externaldiff'            => 'Tan kebroiki wan dorosey agersi-wrokosani (soso gi sabiman - spesrutu set de fanowdu gi disi)',
'tog-showjumplinks'           => 'Sori den "go na" miti',
'tog-uselivepreview'          => 'Kebroiki "wanten sori-na-fesi" (JavaScript – ondrosuku fasi)',
'tog-forceeditsummary'        => 'Gi wan boskopu efu a "Syatu" boksu leygi',
'tog-watchlisthideown'        => 'Kibri mi eygi kenki ini mi Tan luku réy',
'tog-watchlisthidebots'       => 'Kibri den kenki fu den Bot ini mi Tan luku réy',
'tog-watchlisthideminor'      => 'Kibri pikin kenki ini mi Tan luku réy',
'tog-ccmeonemails'            => 'Seni mi wan kopi fu den e-mail mi e seni go na tra kebroikiman',
'tog-diffonly'                => 'No sori san a parira abi ondro den kenki',
'tog-showhiddencats'          => "Sori grupu d'e kibri",

'underline-always'  => 'Ala ten',
'underline-never'   => 'Noiti',
'underline-default' => 'Di fu a browser',

'skinpreview' => '(Si-na-fesi)',

# Dates
'sunday'        => 'sonde',
'monday'        => 'munde',
'tuesday'       => 'tudewroko',
'wednesday'     => 'dridewroko',
'thursday'      => 'fodewroko',
'friday'        => 'freyda',
'saturday'      => 'satra',
'sun'           => 'son',
'mon'           => 'mun',
'tue'           => 'tud',
'wed'           => 'dri',
'thu'           => 'fod',
'fri'           => 'fre',
'sat'           => 'sat',
'january'       => 'foswan mun',
'february'      => 'fostu mun',
'march'         => 'fosdri mun',
'april'         => 'fosfo mun',
'may_long'      => 'fosfeyfi mun',
'june'          => 'fossiksi mun',
'july'          => 'fosseybi mun',
'august'        => 'fosayti mun',
'september'     => 'fosneygi mun',
'october'       => 'fostin mun',
'november'      => 'foserfu mun',
'december'      => 'fostwarfu mun',
'january-gen'   => 'foswan mun',
'february-gen'  => 'fostu mun',
'march-gen'     => 'fosdri mun',
'april-gen'     => 'fosfo mun',
'may-gen'       => 'fosfeyfi mun',
'june-gen'      => 'fossiksi mun',
'july-gen'      => 'fosseybi mun',
'august-gen'    => 'fosayti mun',
'september-gen' => 'fosneygi mun',
'october-gen'   => 'fostin mun',
'november-gen'  => 'foserfu mun',
'december-gen'  => 'fostwarfu mun',
'jan'           => 'wan',
'feb'           => 'tu',
'mar'           => 'dri',
'apr'           => 'fo',
'may'           => 'fosfeyfi mun',
'jun'           => 'sik',
'jul'           => 'sey',
'aug'           => 'ayt',
'sep'           => 'ney',
'oct'           => 'tin',
'nov'           => 'erf',
'dec'           => 'twa',

# Categories related messages
'pagecategories'                 => '{{PLURAL:$1|Grupu|Grupu}}',
'category_header'                => 'Den papira ini grupu “$1”',
'subcategories'                  => 'Ondrogrupu',
'category-media-header'          => 'Media ini grupu “$1”',
'category-empty'                 => "''A grupu disi no abi papira noso media nownowde.''",
'hidden-categories'              => "{{PLURAL:$1|a grupu|den grupu}} d'e kibri",
'hidden-category-category'       => "Grupu d'e kibri", # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|A grupu disi abi den ondro-grupu disi.|A grupu disi abi {{PLURAL:$1|a ondro-grupu|$1 den ondro-grupu}} disi, fu $2 teri na makandra.}}',
'category-subcat-count-limited'  => 'A grupu disi abi {{PLURAL:$1|a ondro-grupu|$1 den ondro-grupu}} disi.',
'category-article-count'         => '{{PLURAL:$2|A grupu disi abi a papira disi.|A grupu disi abi {{PLURAL:$1|a papira|$1 den papira}} disi, fu $2 teri na makandra.}}',
'category-article-count-limited' => 'Den {{PLURAL:$1|papira|$1 papira}} disi de ini a grupu disi.',
'category-file-count'            => '{{PLURAL:$2|A grupu disi abi soso a file disi.|A grupu disi abi {{PLURAL:$1|a file disi|de $1 file disi}}, fu $2, teri na makandra.}}',
'category-file-count-limited'    => '{{PLURAL:$1|A file disi|Den $1 file disi}} de ini a grupu disi.',
'listingcontinuesabbrev'         => 'moro',

'mainpagetext'      => "<big>'''MediaWiki seti kon bun.'''</big>",
'mainpagedocfooter' => 'Luku na ini a [http://meta.wikimedia.org/wiki/Help:Yepi yepibuku] fu si fa fu kebrouki a wikisoftware.

== Moro yepi ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings Den seti]
* [http://www.mediawiki.org/wiki/Manual:FAQ Sani di ben aksi furu (FAQ)]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce Boskopu grupu gi nyun meki]',

'about'          => 'Abra',
'article'        => 'Papira',
'newwindow'      => '(o opo ini wan nyun fensre)',
'cancel'         => 'No kenki',
'qbfind'         => 'Suku',
'qbbrowse'       => 'Browse',
'qbedit'         => 'Kenki',
'qbpageoptions'  => 'A papira disi',
'qbpageinfo'     => 'Abra a papira',
'qbmyoptions'    => 'Mi papira',
'qbspecialpages' => 'Spesrutu papira',
'moredotdotdot'  => 'Moro...',
'mypage'         => 'Mi kebroikiman papira',
'mytalk'         => 'Mi kruderi',
'anontalk'       => 'Taki fu disi IP',
'navigation'     => 'Fenipresi',
'and'            => 'nanga',

# Metadata in edit box
'metadata_help' => 'Metadata:',

'errorpagetitle'    => 'Fowtu',
'returnto'          => 'Drai baka go na $1.',
'tagline'           => 'Fu {{SITENAME}}',
'help'              => 'Yepi',
'search'            => 'Suku',
'searchbutton'      => 'Suku',
'go'                => 'Go',
'searcharticle'     => 'Go',
'history'           => 'Historia fu a papira',
'history_short'     => 'Historia',
'updatedmarker'     => 'kenki sensi mi laste fisiti',
'info_short'        => 'Infrumasie',
'printableversion'  => 'Print',
'permalink'         => 'Permalink',
'print'             => 'Kwinsi',
'edit'              => 'Kenki',
'create'            => 'Meki',
'editthispage'      => 'Kenki a papira disi',
'create-this-page'  => 'Meki a papira disi',
'delete'            => 'Puru',
'deletethispage'    => 'Puru a papira disi',
'undelete_short'    => 'Poti $1 {{PLURAL:$1|kenki|kenki}} baka',
'protect'           => 'Sroto',
'protect_change'    => 'Kenki a fasi fu sroto',
'protectthispage'   => 'Sroto a papira disi',
'unprotect'         => 'Opo',
'unprotectthispage' => 'Opo a papira disi',
'newpage'           => 'Nyun papira',
'talkpage'          => 'Kruderi abra a papira disi',
'talkpagelinktext'  => 'Taki',
'specialpage'       => 'Spesrutu papira',
'personaltools'     => 'Mi eigi wrokosani',
'postcomment'       => 'Poti wan boskopu',
'articlepage'       => 'Luku a papira',
'talk'              => 'Taki',
'views'             => 'Views',
'toolbox'           => 'Wrokosani baki',
'userpage'          => 'Luku a papira fu a kebroikiman',
'projectpage'       => 'Luku a project papira',
'imagepage'         => 'Luku a media papira',
'mediawikipage'     => 'Luku a boskopu papira',
'templatepage'      => 'Luku a template papira',
'viewhelppage'      => 'Luku a yepi papira',
'categorypage'      => 'Luku a grupu papira',
'viewtalkpage'      => 'Luku a taki papira',
'otherlanguages'    => 'Ini tra tongo',
'redirectedfrom'    => '(Seni komopo fu $1)',
'redirectpagesub'   => 'Seni doro papira',
'lastmodifiedat'    => 'A papira disi ben kenki leki laste na tapu $1 na $2.', # $1 date, $2 time
'viewcount'         => 'A papira disi opo {{PLURAL:$1|wan leisi|$1 leisi}}.',
'protectedpage'     => 'A papira disi sroto',
'jumpto'            => 'Go na:',
'jumptonavigation'  => 'fenipresi',
'jumptosearch'      => 'suku',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => 'Abra {{SITENAME}}',
'aboutpage'            => 'Project:Abra',
'bugreports'           => 'Buku fu fowtu',
'bugreportspage'       => 'Project:Buku fu fowtu',
'copyright'            => 'Ala san skrifi dyaso de ondro $1.',
'copyrightpagename'    => '{{SITENAME}} kopi leti',
'copyrightpage'        => '{{ns:project}}:Kopi leti',
'currentevents'        => 'Ini a nyunsu',
'currentevents-url'    => 'Project:Ini a nyunsu',
'disclaimers'          => 'Disclaimers',
'disclaimerpage'       => 'Project:Disclaimer gi ala',
'edithelp'             => 'Yepi nanga kenki',
'edithelppage'         => 'Help:Kenki',
'faq'                  => 'FAQ (Sani di ben aksi furu)',
'faqpage'              => 'Project:Sani di ben aksi furu',
'helppage'             => 'Help:San de',
'mainpage'             => 'Fesipapira',
'mainpage-description' => 'Fesipapira',
'policy-url'           => 'Project:Polisi',
'portal'               => 'Kebroikiman konmakandra',
'portal-url'           => 'Project:Kebroikiman konmakandra',
'privacy'              => 'Privacybeleid',
'privacypage'          => 'Project:Privacy',

'badaccess'        => 'Primisi fowtu',
'badaccess-group0' => 'Yu no abi primisi fu du a sani san yu wani',
'badaccess-group1' => 'Soso kebroikiman fu a grupu $1 kan du a sani disi.',
'badaccess-group2' => 'Soso kebroikiman fu wan fu den grupu $1 kan du a sani disi.',
'badaccess-groups' => 'Soso kebroikiman fu wan fu den grupu $1 kan du a sani disi.',

'versionrequired'     => 'Versie $1 fu MediaWiki de fanowdu',
'versionrequiredtext' => 'Versie $1 fu MediaWiki de fanowdu fu man kebroiki a papira disi. Luku ini a papira [[Special:Version|softwareversie]].',

'ok'                      => 'Abun',
'retrievedfrom'           => 'Teki baka fu "$1"',
'youhavenewmessages'      => 'Yu abi $1 ($2).',
'newmessageslink'         => 'nyun boskopu',
'newmessagesdifflink'     => 'laste kenki',
'youhavenewmessagesmulti' => 'Yu abi nyun boskopu na tapu $1',
'editsection'             => 'kenki',
'editold'                 => 'kenki',
'viewsourceold'           => 'Luku a source',
'editsectionhint'         => 'Kenki a pisi: $1',
'toc'                     => 'San de',
'showtoc'                 => 'sori',
'hidetoc'                 => 'kibri',
'thisisdeleted'           => 'Luku noso poti baka $1?',
'viewdeleted'             => 'Luku $1?',
'restorelink'             => '$1 taki de ben puru',
'feedlinks'               => 'Feed:',
'feed-invalid'            => 'A sortu feed disi no bun dyaso',
'feed-unavailable'        => 'Den Syndication feed no de na tapu {{SITENAME}}',
'site-rss-feed'           => '$1 RSS-feed',
'site-atom-feed'          => '$1 Atom-feed',
'page-rss-feed'           => '“$1” RSS-feed',
'page-atom-feed'          => '“$1” Atom-feed',
'red-link-title'          => '$1 (a no meki ete)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => 'Papira',
'nstab-user'      => 'Kebroikiman papira',
'nstab-media'     => 'Media papira',
'nstab-special'   => 'Spesrutu',
'nstab-project'   => 'Project papira',
'nstab-image'     => 'File',
'nstab-mediawiki' => 'Boskopu',
'nstab-template'  => 'Template',
'nstab-help'      => 'Yepi papira',
'nstab-category'  => 'Grupu',

# Main script and global functions
'nosuchaction'      => 'A sani disi no man',
'nosuchactiontext'  => 'A wiki no sabi a komanderi ini a URL',
'nosuchspecialpage' => 'A spesrutu papira disi no de',
'nospecialpagetext' => "<big>'''Yu aksi fu si wan spesrutu papira san no de.'''</big>

Wan réy fu spesrutu papira de fu feni na [[Special:SpecialPages|{{int:specialpages}}]].",

# General errors
'error'                => 'Fowtu',
'databaseerror'        => 'Database fowtu',
'missingarticle-rev'   => '(versie nomru: $1)',
'missingarticle-diff'  => '(Kenki: $1, $2)',
'internalerror'        => 'Fowtu na inisey',
'internalerror_info'   => 'Fowtu na inisey: $1',
'filecopyerror'        => 'No ben man kopi a file “$1” go na “$2”.',
'filerenameerror'      => 'No ben man kenki a nen fu file “$1” go na “$2”.',
'filedeleteerror'      => 'No ben man puru a file “$1”.',
'directorycreateerror' => 'No ben man meki a map “$1”.',
'filenotfound'         => 'Ne ben man feni a file “$1”.',
'fileexistserror'      => 'No man skrifi go na file “$1”: a file de kaba',
'unexpected'           => 'No ben ferwakti a warti disi: "$1"="$2".',
'formerror'            => 'Fowtu: no ben man seni a formulier',
'badarticleerror'      => 'No man du a sani disi na tapu a papira disi.',
'cannotdelete'         => 'No man puru a papira noso a file. A kan dati wan tra suma puru en kaba.',
'badtitle'             => 'A papira nen no bun',
'badtitletext'         => 'A nen fu a papira san ben aksi no bun, leygi, noso abi wan miti go na inter-tongo noso inter-wiki nen san no bun.
A kan taki a abi wan noso moro karakter san no bun fu kebroiki gi nen.',
'viewsource'           => 'Luku a source',
'viewsourcefor'        => 'fu $1',
'protectedpagetext'    => 'A papira disi sroto gi kenki.',
'viewsourcetext'       => 'Yu kan luku nanga kopi a source fu a papira disi:',
'customcssjsprotected' => 'Yu no kan kenki a papira disi, bika a abi seti fu wan tra kebroikiman.',
'ns-specialprotected'  => 'Spesrutu papira no kan kenki.',
'titleprotected'       => "[[User:$1|$1]] sroto a meki fu wan papira nanga a nen disi
Fu san ede: ''$2''.",

# Login and logout pages
'logouttitle'               => 'Kenroikiman psa gwe',
'logouttext'                => '<strong>Yu psa gwe now.</strong><br />
Yu kan tan kebroiki {{SITENAME}} sondro nen, noso yu kan psa kon baka leki a srefi noso wan tra kebroikiman.
Sabi taki a kan gersi leki yu psa kon ete, te leki yu leygi a cache fu yu browser.',
'welcomecreation'           => '== Welkom, $1! ==
Yu account meki now.
No fergiti fu kenki den seti fu yu gi {{SITENAME}}.',
'loginpagetitle'            => 'Kebroikiman nen',
'yourname'                  => 'Kebroikiman nen:',
'yourpassword'              => 'Psa wortu:',
'yourpasswordagain'         => 'Psa wortu ete wan leysi:',
'remembermypassword'        => 'Tan memre mi kebroikiman nen nanga psa wortu.',
'yourdomainname'            => 'Yu domein:',
'loginproblem'              => '<b>Wan problema ben de di yu e psa kon.</b><br />
Pruberi baka!',
'login'                     => 'Psa kon',
'nav-login-createaccount'   => 'Psa kon / meki wan account',
'loginprompt'               => 'Yu musu man kisi cookies fu man psa kon na {{SITENAME}}.',
'userlogin'                 => 'Psa kon / meki wan account',
'logout'                    => 'Psa gwe',
'userlogout'                => 'Psa gwe',
'notloggedin'               => 'No psa kon',
'nologin'                   => 'No abi wan kebroikiman nen ete? $1.',
'nologinlink'               => 'Meki wan account',
'createaccount'             => 'Meki wan account',
'gotaccount'                => 'Abi wan kebroikiman nen kba? $1.',
'gotaccountlink'            => 'Psa kon',
'createaccountmail'         => 'via e-mail',
'badretype'                 => 'Den tu psa wortu no de srefi.',
'userexists'                => 'A kebroikiman nen disi de kaba.
Teki wan tra nen.',
'youremail'                 => 'E-mail:',
'username'                  => 'Kebroikiman nen:',
'uid'                       => 'Kebroikiman ID:',
'prefs-memberingroups'      => 'Memre fu {{PLURAL:$1|grupu|grupu}}:',
'yourrealname'              => 'Yu tru nen:',
'yourlanguage'              => 'Tongo:',
'yournick'                  => 'Ondroskrifi:',
'email'                     => 'E-mail',
'prefs-help-realname'       => 'Tru nen no de ferplekti. Efu yu poti en, a de fu gi yu grani gi yu wroko.',
'loginerror'                => 'Fowtu na a psa kon',
'prefs-help-email'          => 'E-mail nen no de ferplekti, ma a e gi trawan a okasi fu kontakti yu na tapu yu kebroikiman papira noso na tapu yu taki papira, sondro fu sori suma na yu fu tru.',
'prefs-help-email-required' => 'Wan e-mail nen de fanowdu gi disi.',
'nocookiesnew'              => 'A account ben meki, ma yu no psa kon.<br />
{{SITENAME}} e kebroiki cookies fu meki kebroikiman psa kon.<br />
Kenki den seti fu yu browser so dati a kan kisi den cookies disi, dan psa kon baka nanga yu nyun kebroikiman nen nanga psa wortu.',
'nocookieslogin'            => '{{SITENAME}} e kebroiki cookies fu meki kebroikiman psa kon.<br />
Yu browser no man kisi cookies.<br />
Kenki den seti fu yu browser so dati a kan kisi den cookies disi, én pruberi baka.',
'noname'                    => 'Yu no gi wan bun kebroikiman nen.',
'loginsuccesstitle'         => 'Yu psa kon now.',
'loginsuccess'              => "'''Now yu de na tapu {{SITENAME}} leki \"\$1\".'''",
'nosuchuser'                => 'No wan kebroikiman de san nen "$1".<br />Luku efu yu skrifi a nen bun, noso meki an nyun account.',
'nosuchusershort'           => 'No wan kebroikiman de di nen "<nowiki>$1</nowiki>".<br />Luku efu yu skrifi a nen bun.',
'nouserspecified'           => 'Yu musu gi wan kebroikiman nen.',
'wrongpassword'             => 'Psa wortu no bun.<br />
Pruberi baka.',
'wrongpasswordempty'        => 'No wan psa wortu ben gi.<br />
Pruberi baka.',
'passwordtooshort'          => 'A psa wortu no bun noso a syatu tumsi.<br />
A musu abi {{PLURAL:$1|karakter|$1 karakter}} noso moro, én a no kan de srefi leki yu kebroikiman nen.',
'mailmypassword'            => 'E-mail psa wortu',
'passwordremindertitle'     => 'Nyun sranga psa wortu gi {{SITENAME}}',
'passwordremindertext'      => 'Wan suma (kande yu,  fu IP $1) ben aksi fu wi seni yu wan nyun psa wortu gi {{SITENAME}} ($4).<br />
A psa wortu fu kebroikiman "$2" na "$3" now.<br />
Psa kon nownowde én kenki yu psa wortu.

Efu na wan tra suma ben aksi disi, noso efu yu memre yu psa wortu én yu no wani kenki en moro, yu kan skotu a boskopu disi én tan kebroiki yu owru psa wortu.',
'noemail'                   => 'No wan e-mail nen de gi kebroikiman "$1".',
'passwordsent'              => 'Wan nyun psa wortu seni go na a e-mail fu "$1".
Psa kon baka te yu kisi en.',
'eauthentsent'              => 'Wan e-mail seni go na a e-mail nen di yu gi.
Bifo tra e-mail kan seni go na a account, yu musu du san skrifi ini a e-mail fu sori taki a account na fu yu fu tru.',
'accountcreated'            => 'Masyin ben skopu',
'accountcreatedtext'        => 'A masyin $1 ben skopu.',
'createaccount-title'       => 'Masyin skopu fu {{SITENAME}}',
'loginlanguagelabel'        => 'Tongo: $1',

# Password reset dialog
'resetpass'        => 'Kenki yu waktiwortu',
'resetpass_header' => 'Kenki yu waktiwortu',
'resetpass_submit' => 'Kenki yu waktiwortu nanga kon',

# Edit page toolbar
'bold_sample'     => 'Fatu skrifi',
'bold_tip'        => 'Fatu',
'italic_sample'   => 'Skoinsi skrifi',
'italic_tip'      => 'Skoinsi',
'link_sample'     => 'Miti nen',
'link_tip'        => 'Miti go na insey',
'extlink_sample'  => 'http://www.example.com miti nen',
'extlink_tip'     => 'Miti go na dorosey (no fergiti fu poti http:// fosi)',
'headline_sample' => 'Pisi ede nen',
'headline_tip'    => 'Pisi ede nen',
'math_sample'     => 'Poti formula dyaso',
'math_tip'        => 'Formula fu teri (LaTeX)',
'nowiki_sample'   => 'Skrifi sondro wiki skrifi-fasi dyaso',
'nowiki_tip'      => 'Skotu a wiki skrifi-fasi',
'image_tip'       => 'Media file',
'media_tip'       => 'Miti go na file',
'sig_tip'         => 'Yu ondroskrifi nanga a dei nanga a yuru',
'hr_tip'          => 'Didon lini (no kebroiki furu)',

# Edit pages
'summary'                => "In' syatu",
'subject'                => 'Abra san/ede',
'minoredit'              => 'Disi na wan pikin kenki',
'watchthis'              => 'Tan luku a papira disi',
'savearticle'            => 'Kibri a papira disi',
'preview'                => 'Luku-na-fesi',
'showpreview'            => 'Sori na fesi',
'showlivepreview'        => 'Fusi libi si pre kenki (LIVE)',
'showdiff'               => 'Sori den kenki',
'anoneditwarning'        => "'''Warskow:''' Yu no psa kon ete. Yu IP o kibri poti ini a kenki historia fu a papira disi.",
'missingcommenttext'     => 'Presi yu oponaki dyaso-ondro.',
'summary-preview'        => "Luku In'syatu na fesi",
'subject-preview'        => 'Ondroinfru/edelen fusi',
'blockedtitle'           => 'Masyin ben spikri',
'blockedtext'            => "<big>'''Pasi tapu gi yu kebroikiman-nen noso IP.'''</big>

$1 tapu pasi gi yu. Disi na fu sanede ''$2''.

* Bigin fu tapu pasi: $8
* Kaba fu tapu pasi: $6
* Tapu pasi gi: $7

Yu kan skrifi $1 noso wan tra [[{{MediaWiki:Grouppage-sysop}}|beheerder]] fu taki abra a tapu pasi disi.
Yu n'o man kebroiki 'e-mail a kebroikiman disi', efu yu no abi wan bun email-nen ini yu [[Special:Preferences|seti]] én pasi tapu fu yu kebroiki en.
Yu IP now na $3 en a tapu pasi ID na #$5. Gi wan, noso ala tu, ini yu brifi te yu o skrifi fu aksi san psa.",
'whitelistedittitle'     => 'Yu mu kon fu a kenki',
'whitelistedittext'      => 'Yu mu $1 fu a kenki fu peprewoysi.',
'loginreqtitle'          => 'Yu mu kon',
'loginreqlink'           => 'kon',
'loginreqpagetext'       => 'Yu mu $1 tu a libi si fu trawan peprewoysi.',
'accmailtitle'           => 'Waktiwortu ben stir.',
'accmailtext'            => 'A waktiwortu fu "$1" ben stir na $2.',
'newarticle'             => '(Nyun)',
'newarticletext'         => "Yu e pruberi fu opo wan papira di no de ete. Fu meki a papira, bigin skrifi ini a boksu dyaso na ondrosey (luku ini [[{{MediaWiki:Helppage}}|yepipapira]] gi yepi).
Kebroiki a '''back''' knopo ini yu browser, efu yu no ben wan opo a papira disi.",
'noarticletext'          => 'A papira disi leigi.
Yu kan [[Special:Search/{{PAGENAME}}|suku a papira nen disi]] ini tra papira noso <span class="plainlinks">[{{fullurl:{{FULLPAGENAME}}|action=edit}} kenki a papira disi]</span>.',
'note'                   => '<strong>Opotaki:</strong>',
'previewnote'            => '<strong>Disi na soso fu luku na fesi;
yu kenki no kibri ete!</strong>',
'editing'                => 'E Kenki $1',
'editingsection'         => 'E kenki $1 (pisi papira)',
'editingcomment'         => 'Kenki fu $1 (opotaki)',
'yourtext'               => 'Yu litiwrok',
'yourdiff'               => 'Kenki',
'copyrightwarning'       => "Ala sani di yu e poti na tapu {{SITENAME}} de leki efu den ben gi fri ondro a $2 (luku $1 gi a fin'fini).
Efu yu no wani dati trawan e kenki noso panya san yu skrifi, no skrifi noti dyaso.<br />
Yu e pramisi unu dati na yu skrifi disi yusrefi, noso yu teki en puru fu wan fri, opo presi.<br />
<strong>NO KEBROIKI SANI DI KIBRI BAKA SKRIFIMAN-LETI, SONDRO FU ABI PRIMISI FU DU SO!</strong>",
'longpagewarning'        => '<strong>WARSKOW: A papira disi de $1 kilobyte bigi;
Son browser abi problema fu kenki papira di bigi moro leki 32kb.
Kande yu kan prati a papira disi ini moro pikin pisi.</strong>',
'templatesused'          => 'Template di ben kebroiki tapu a papira disi:',
'templatesusedpreview'   => 'Template di ben kebroiki ini a Luku-na-fesi disi:',
'templatesusedsection'   => 'Ankra teki opo disi seksi:',
'template-protected'     => '(a sroto)',
'template-semiprotected' => '(sroto wan pisi)',
'nocreatetext'           => '{{SITENAME}} puru den primisi fu meki nyun papira.
Yu kan go baka fu kenki papira di de kba, noso yu kan [[Special:UserLogin|psa kon noso meki wan account]].',
'recreate-deleted-warn'  => "'''Warskow: yu e meki wan papira, di ben puru fu dyaso kaba, baka.'''

Denki fosi efu na wan bun sani fu meki a papira disi baka. A log buku fu puru sori dyaso gi yepi:",

# Account creation failure
'cantcreateaccounttitle' => 'Kan masyin ni skopu.',

# History pages
'viewpagelogs'        => 'Luku a log buku fu a papira disi',
'currentrev'          => 'A versi disi',
'revisionasof'        => 'Versi tapu $1',
'revision-info'       => 'Versi na $1 fu $2',
'previousrevision'    => '←Moro owru versi',
'nextrevision'        => 'Moro nyun versi→',
'currentrevisionlink' => 'A versi disi',
'cur'                 => 'disi',
'next'                => 'trawan',
'last'                => "a wan n'en fesi",
'page_first'          => 'foswan',
'page_last'           => 'laste',
'histlegend'          => "Teki den difrenti: marki den versi fu san yu wan si den difrenti, dan naki ENTER noso a knopo na ondrosey.<br />
Den syatu: (disi) = a difrenti nanga a versi disi, (a wan n'en fesi) = a difrenti nanga a versi na en fesi, p = pikin kenki",
'deletedrev'          => '[ben e trowe]',
'histfirst'           => 'A moro owru wan',
'histlast'            => 'A moro nyun wan',
'historysize'         => '({{PLURAL:$1|wan byte|$1 byte}})',
'historyempty'        => '(leygi)',

# Revision feed
'history-feed-title'          => 'Kenkistori',
'history-feed-description'    => 'Kenkistori du disi papira opo {{SITENAME}}',
'history-feed-item-nocomment' => '$1 tapu $2', # user at time
'history-feed-empty'          => 'A aksa papira ben no da.
A kan ben trowe efu dribi.
[[Special:Search|Suku {{SITENAME}}]] fu rilivante peprewoysi.',

# Revision deletion
'rev-deleted-comment' => '(opetaki ben trowe)',
'rev-deleted-user'    => '(masyin ben trowe)',
'rev-deleted-event'   => '(aksi ben trowe)',
'rev-delundel'        => 'libi si/no libi si',
'revisiondelete'      => 'Versie trowe/otrowe',

# Diffs
'history-title'           => 'Historia fu "$1"',
'difference'              => '(A difrenti fu den kenki)',
'lineno'                  => 'Lini $1:',
'compareselectedversions' => 'Luku den difrenti fu den versi di teki',
'editundo'                => "drai pot' baka",
'diff-multi'              => '(No e sori {{PLURAL:$1|wan versi|$1 versi}} na mindrisey.)',

# Search results
'searchresults'         => 'Sukuleysi',
'searchresulttext'      => 'Fu pasa infrumasi abra suku opo {{SITENAME}}, leysi [[{{MediaWiki:Helppage}}|{{int:help}}]].',
'searchsubtitle'        => "Y ben o suku na '''[[:$1]]'''",
'searchsubtitleinvalid' => "Yu ben o suku na '''$1'''",
'noexactmatch'          => "'''No wan papira de nanga a nen \"\$1\".'''
Yu kan [[:\$1|meki a papira disi]].",
'noexactmatch-nocreate' => "'''Da ben no papira nanga a nen \"\$1\".'''",
'prevn'                 => '$1 di psa',
'nextn'                 => '$1 trawan',
'viewprevnext'          => 'Luku ($1) ($2) ($3).',
'powersearch'           => 'Suku moro dipi',

# Preferences page
'preferences'              => 'Seti',
'mypreferences'            => 'Mi seti',
'prefs-edits'              => 'Nomru fu kenki:',
'prefsnologin'             => 'No kon',
'qbsettings'               => 'Kwikbak',
'qbsettings-none'          => 'Nowan',
'qbsettings-fixedleft'     => 'Set na ku',
'qbsettings-fixedright'    => 'Set na pe',
'qbsettings-floatingleft'  => 'Han na ku',
'qbsettings-floatingright' => 'Han na pe',
'changepassword'           => 'Kenki yu waktiwortu',
'skin'                     => 'Buba',
'math'                     => 'Fomula',
'dateformat'               => 'Datumopomeki',
'datedefault'              => 'No wana',
'datetime'                 => 'Datum nanga ten',
'math_lexing_error'        => 'leksikografi fowtu',
'math_syntax_error'        => 'sintaki fowtu',
'prefs-personal'           => 'Masyinmasi',
'prefs-rc'                 => 'Bakaseywan kenki',
'prefs-watchlist'          => 'Sirey',
'prefs-watchlist-days'     => 'Maximum teki fu dey ini mi sirey:',
'prefs-watchlist-edits'    => 'Maximum teki fu dey ini mi granmeki sirey:',
'prefs-misc'               => 'Diversi',
'saveprefs'                => 'Oponaki',
'resetprefs'               => 'Bakadray na owru si',
'oldpassword'              => 'Owru waktiwortu:',
'newpassword'              => 'Nyun waktiwortu:',
'retypenew'                => 'Nyun psa wortu ete wan tron:',
'textboxsize'              => 'Kenki',
'rows'                     => 'Rei:',
'columns'                  => 'Kolum:',
'searchresultshead'        => 'Suku',
'resultsperpage'           => 'Ris ies papira:',
'contextlines'             => 'Lina ies ris:',
'contextchars'             => 'Kontekst ies lina:',
'stub-threshold'           => 'Dupolo fu seti <a href="#" class="stub">stub</a>:',
'recentchangesdays'        => 'Teki fu dey tu libi si ini bakaseywan kenki:',
'recentchangescount'       => 'Teki fu peprewoysi ini bakaseywan kenki:',
'savedprefs'               => 'Yu masi ben oponaki.',
'timezonelegend'           => 'Gron fu ten',
'timezonetext'             => '¹A teki fu yuru taki yu presiten kenki fu a serverten (UTC).',
'localtime'                => 'Lokali ten',
'timezoneoffset'           => 'Ski ini ten¹',
'servertime'               => 'Serverten',
'guesstimezone'            => 'Fu a browser presi',
'allowemail'               => 'E-mail fu trawan masyin adu',
'defaultns'                => 'Soma ini disi nenpreki suku:',
'default'                  => 'soma',
'files'                    => 'Gefre',

# User rights
'userrights-lookup-user'   => 'Masyinguru kenki',
'userrights-user-editname' => 'Wan masyinnen gi:',
'editusergroup'            => 'Masyinguru kenki',
'editinguser'              => "Kenki fu lesi fu '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => 'Masyinguru kenki',
'saveusergroups'           => 'Masyinguru oponaki',
'userrights-groupsmember'  => 'Masyin fu:',
'userrights-no-interwiki'  => 'Yu abi no lesi tu kenki fu masyinlesi opo trawan wiki.',

# Groups
'group'               => 'Guru:',
'group-autoconfirmed' => 'Registiri masyin',
'group-bot'           => 'Bot',
'group-sysop'         => 'Sesopu',
'group-bureaucrat'    => 'Burokrati',
'group-all'           => '(ala)',

'group-autoconfirmed-member' => 'Registiri masyin',
'group-bot-member'           => 'Bot',
'group-sysop-member'         => 'Sesopu',
'group-bureaucrat-member'    => 'Burokrati',

'grouppage-autoconfirmed' => '{{ns:project}}:Registiri masyin',
'grouppage-bot'           => '{{ns:project}}:Bot',
'grouppage-sysop'         => '{{ns:project}}:Admin',
'grouppage-bureaucrat'    => '{{ns:project}}:Burokrati',

# User rights log
'rightslog'  => 'Log buku fu kebroikiman leti',
'rightsnone' => '(no)',

# Recent changes
'nchanges'                       => '$1 {{PLURAL:$1|kenki|kenki}}',
'recentchanges'                  => 'Laste kenki',
'recentchanges-feed-description' => 'Nanga a feed disi yu kan luku den moro nyun kenki fu a wiki disi.',
'rcnote'                         => "Dya na ondrosey {{PLURAL:$1|'''1''' kenki|den '''$1''' laste kenki}} ini {{PLURAL:$2|a dei|den '''$2''' dei}} na fesi de fu si, tapu $4 na $5.",
'rcnotefrom'                     => "Kenki fu '''$2''' (e sori te go miti '''$1''' kenki).",
'rclistfrom'                     => 'Sori nyun kenki, bigin fu $1',
'rcshowhideminor'                => '$1 den pikin kenki',
'rcshowhidebots'                 => '$1 den bot',
'rcshowhideliu'                  => '$1 kebroikiman di psa kon',
'rcshowhideanons'                => '$1 den kebroikiman sondro nen',
'rcshowhidepatr'                 => '$1 den kenki di kisi luku',
'rcshowhidemine'                 => '$1 mi kenki',
'rclinks'                        => 'Sori den laste $1 kenki ini den $2 laste dei<br />$3',
'diff'                           => 'kenki',
'hist'                           => 'hist',
'hide'                           => 'kibri',
'show'                           => 'Sori',
'minoreditletter'                => 'p',
'newpageletter'                  => 'N',
'boteditletter'                  => 'b',
'newsectionsummary'              => '/* $1 */ nyon gron',

# Recent changes linked
'recentchangeslinked'          => 'Kenki di abi wan sani fu du nanga disi',
'recentchangeslinked-title'    => 'Kenki di abi wan sani fu du nanga "$1"',
'recentchangeslinked-noresult' => 'Noti ben kenki ini den miti papira ini a pisi di gi.',
'recentchangeslinked-summary'  => "A spesrutu papira disi e sori den laste kenki di ben meki tapu papira di miti tapu wan papira di sori (noso go na memre fu wan grupu di sori).
Papira ini [[Special:Watchlist|yu Tan Luku réy]] '''fatu'''.",

# Upload
'upload'            => 'Lai wan file poti',
'uploadbtn'         => 'Lai file poti',
'reupload'          => 'Ri-uploti',
'uploadnologin'     => 'No kon',
'uploaderror'       => 'Uplotifowtu',
'uploadlog'         => 'uplotibuku',
'uploadlogpage'     => 'Log buku fu den lai-poti',
'filename'          => 'Gefrenen',
'filedesc'          => 'Infrumasi-box',
'fileuploadsummary' => 'Infrumasi:',
'uploadwarning'     => 'Atessi fu uploti',
'savefile'          => 'Gefre oponaki',
'uploadedimage'     => 'lai "[[$1]]" poti',
'overwroteimage'    => 'abi wan nyun si fu "[[$1]]" e uploti',
'watchthisupload'   => 'Disi papira si',

'license-nopreview' => '(No fusi)',

# Special:ImageList
'imagelist_search_for'  => 'Suku na gefre:',
'imgfile'               => 'gefre',
'imagelist'             => 'Réy fu file',
'imagelist_date'        => 'Datum',
'imagelist_name'        => 'Nen',
'imagelist_user'        => 'Masyin',
'imagelist_size'        => 'Gran (byte)',
'imagelist_description' => 'Infrumasi',

# Image description page
'filehist'                  => 'File historia',
'filehist-help'             => 'Naki na tapu a dei/ten fu a file fu si fa a ben de na a ten dati.',
'filehist-deleteall'        => 'trowe ala',
'filehist-deleteone'        => 'trowe disi',
'filehist-revert'           => 'bakadray',
'filehist-current'          => 'disi',
'filehist-datetime'         => 'Dei/ten',
'filehist-user'             => 'Kebroikiman',
'filehist-dimensions'       => 'Den marki',
'filehist-filesize'         => 'File marki',
'filehist-comment'          => 'Boskopu',
'imagelinks'                => 'File nen miti',
'linkstoimage'              => '{{PLURAL:$1|A papira|$1 Den papira}} disi e kebroike a file disi:',
'nolinkstoimage'            => 'Nowan papira e miti kon na a file disi.',
'sharedupload'              => 'A file disi lai poti fu prati én tra project kan kebroiki en.',
'shareduploadwiki'          => 'Si $1 fu pasa infrumasi.',
'shareduploadwiki-linktext' => 'gefreinfrumasi',
'noimage'                   => 'No wan file de nanga a nen disi. Yu kan $1.',
'noimage-linktext'          => 'lai en poti',
'uploadnewversion-linktext' => 'Lai wan moro nyun versi fu a file disi poti',

# File reversion
'filerevert'                => '$1 bakadray',
'filerevert-legend'         => 'Gefre bakadray',
'filerevert-intro'          => "Yu ben '''[[Media:$1|$1]]''' bakadrayn tu a [$4 si opo $2, $3]",
'filerevert-comment'        => 'Opotaki:',
'filerevert-defaultcomment' => 'E bakadray tu a si opo $1, $2',
'filerevert-submit'         => 'Bakadray',
'filerevert-success'        => "'''[[Media:$1|$1]]''' ben bakadray tu a [$4 si opo $2, $3]",

# File deletion
'filedelete'             => '"$1" trowe',
'filedelete-legend'      => 'Gefre trowe',
'filedelete-intro'       => "Yu ben '''[[Media:$1|$1]]''' trowen.",
'filedelete-intro-old'   => "Yu ben a si fu '''[[Media:$1|$1]]''' fu [$4 $3, $2] trowen.",
'filedelete-comment'     => 'Opotaki:',
'filedelete-submit'      => 'Trowe',
'filedelete-success'     => "'''$1''' ben e trowe.",
'filedelete-success-old' => '<span class="plainlinks">A si fu \'\'\'[[Media:$1|$1]]\'\'\' fu $3, $2 ben e trowen.</span>',
'filedelete-nofile'      => "'''$1''' ben no da.",

# MIME search
'mimesearch' => 'Suku MIME-type',
'mimetype'   => 'MIME-type:',
'download'   => 'Dawnloti',

# List redirects
'listredirects' => 'Réy fu seni-doro',

# Unused templates
'unusedtemplates'    => 'Template di no ben kebroiki',
'unusedtemplateswlh' => 'trawan skaki',

# Random page
'randompage'         => 'Iniwan papira',
'randompage-nopages' => 'Da ben no peprewoysi ini disi nenpreki.',

# Random redirect
'randomredirect'         => 'Iniwan seni-doro',
'randomredirect-nopages' => 'Da ben no stirpeprewoysi ini disi nenpreki.',

# Statistics
'statistics'    => 'Den statistiek',
'sitestats'     => '{{SITENAME}}-Infrumasi',
'userstats'     => 'Masyininfrumasi',
'sitestatstext' => "Ini a databesi {{PLURAL:$1|ben wan papira|ben '''$1''' peprewoysi}}, nanga takipeprewoysi, {{SITENAME}}-peprewoysi, den stub dy si syartu ben, stirpeprewoysi, boskopu nanga trawan peprewoysi dy hosa no infrumasi abi.
Da {{PLURAL:$2|ben hosa wan peprewoysi|ben hosa '''$2''' peprewoysi}} nanga tru infrumasi.

Da {{PLURAL:$8|ben '''wan''' gefre|ben '''$8''' gefre}} uploti.

Da {{PLURAL:$3|ben '''wan''' papira|ben '''$3''' peprewoysi}} libi si nanga {{PLURAL:$4|wan kenki|'''$4''' kenki}} e meki sins {{SITENAME}} ben e skopu.
Taki gi wan midi fu '''$5''' kenki ies papira nanga '''$6''' sko peprewoysi ies kenki.

A gran fu a [http://www.mediawiki.org/wiki/Manual:Job_queue job queue] ben '''$7'''.",
'userstatstext' => "Da {{PLURAL:$1|ben '''wan''' register masyin|ben '''$1''' register masyin}}, pefu da
'''$2''' (efu '''$4%''') $5-lesi {{PLURAL:$2|abi|abi}}.",

'disambiguations' => 'Seni doro papira',

'doubleredirects'     => 'Seni doro tu leisi',
'doubleredirectstext' => 'Disi rei abi peprewoysi dy stir na trawan stir. Ies rei abi skaki na a foswan nanga a fostu stirpapira nanga wan skaki na a duli fu a fosty stirpapira. Pasa den ten ben a bakaseywan papira a tru duli.',

'brokenredirects'        => 'Seni-doro di no bun',
'brokenredirectstext'    => 'Dyaso ben stirpeprewoysi dy wan stir ben na wan papira dy no da ben.',
'brokenredirects-edit'   => '(kenki)',
'brokenredirects-delete' => '(trowe)',

'withoutinterwiki'         => 'Papira sondro miti go na tra tongo',
'withoutinterwiki-summary' => 'Disi peprewoysi skaki no na si ini wan trawan tongo:',
'withoutinterwiki-submit'  => 'Libi si',

'fewestrevisions' => 'Papira nanga den moro pikin kenki',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|byte}}',
'ncategories'             => '$1 {{PLURAL:$1|guru|guru}}',
'nlinks'                  => '$1 {{PLURAL:$1|miti|miti}}',
'nmembers'                => '$1 {{PLURAL:$1|memre|memre}}',
'nrevisions'              => '$1 {{PLURAL:$1|si|si}}',
'nviews'                  => '$1 {{PLURAL:$1|mansi|mansi}}',
'lonelypages'             => 'Weisi papira',
'lonelypagestext'         => 'Na den ondroben peprewoysi sey fu u {{SITENAME}} no skaki.',
'uncategorizedpages'      => 'Papira sondro grupu',
'uncategorizedcategories' => 'Grupu sondro grupu',
'uncategorizedimages'     => 'File sondro grupu',
'uncategorizedtemplates'  => 'Template sondro grupu',
'unusedcategories'        => 'Grupu di no ben kebroiki',
'unusedimages'            => 'File di no ben kebroiki',
'wantedcategories'        => 'Grupu di de fanowdu, ma no de',
'wantedpages'             => 'Papira di de fanowdu, ma di no de ete',
'mostlinked'              => 'Den papira di abi den moro furu miti kon na inisey',
'mostlinkedcategories'    => 'Den grupu di abi den moro furu miti kon na inisey',
'mostlinkedtemplates'     => 'Den template di abi den moro furu miti kon na inisey',
'mostcategories'          => 'Den papira nanga den moro furu grupu',
'mostimages'              => 'Den file di abi den moro furu miti kon na inisey',
'mostrevisions'           => 'Den papira nanga den moro furu kenki',
'prefixindex'             => "Ala papira tapu fes'poti",
'shortpages'              => 'Syatu papira',
'longpages'               => 'Langa papira',
'deadendpages'            => 'Papira sondro miti',
'deadendpagestext'        => 'Den ondroben peprewoysi abi no skaki na trawan peprewoysi ini {{SITENAME}}.',
'protectedpages'          => 'Papira di sroto',
'protectedpagestext'      => 'Da ondroben peprewoysi ben tapu nanga kan no kenki abi efru e dribi ben',
'listusers'               => 'Réy fu kebroikiman',
'newpages'                => 'Nyun papira',
'newpages-username'       => 'Masyinnen:',
'ancientpages'            => 'Den moro owru papira',
'move'                    => 'Froisi',
'movethispage'            => 'Froisi a papira disi',
'pager-newer-n'           => '{{PLURAL:$1|nyunr wan|nyunr $1}}',
'pager-older-n'           => '{{PLURAL:$1|owrur wan|owrur $1}}',

# Book sources
'booksources'    => 'Buku source',
'booksources-go' => 'Suku',

# Special:Log
'specialloguserlabel'  => 'Kebroikiman:',
'speciallogtitlelabel' => 'Papira nen:',
'log'                  => 'Log buku',
'all-logs-page'        => 'Ala log buku',
'log-search-submit'    => 'Go',
'log-title-wildcard'   => 'Peprewoysi suku dy nanga disi nen bigin',

# Special:AllPages
'allpages'          => 'Ala papira',
'alphaindexline'    => '$1 te go miti $2',
'nextpage'          => "A papira d'e kon ($1)",
'prevpage'          => 'A papira di psa ($1)',
'allpagesfrom'      => 'Sori papira, bigin na:',
'allarticles'       => 'Ala papira',
'allinnamespace'    => 'Ala peprewoysi (nenpreki $1)',
'allnotinnamespace' => 'Ala peprewoysi (no ini nenpreki $1)',
'allpagessubmit'    => 'Go',
'allpagesprefix'    => 'Sori papira di e bigin nanga:',
'allpages-bad-ns'   => '{{SITENAME}} abi no nenpreki nanga a nen "$1".',

# Special:Categories
'categories' => 'Den grupu',

# Special:ListUsers
'listusersfrom'      => 'Masyin libi si fu:',
'listusers-submit'   => 'Libi si',
'listusers-noresult' => 'No masyin dyaso.',

# E-mail user
'mailnologin'     => 'No stiradresi',
'emailuser'       => 'E-mail a kebroikiman disi',
'emailpage'       => 'Mayin e-mail',
'defemailsubject' => 'E-mail fu {{SITENAME}}',
'noemailtitle'    => 'Disi masyin abi no e-mailadresi',
'emailfrom'       => 'Fu',
'emailto'         => 'A',
'emailsubject'    => 'Infrumasi',
'emailmessage'    => 'Boskopu',
'emailsend'       => 'Stir',

# Watchlist
'watchlist'            => 'Mi Tan Luku réy',
'mywatchlist'          => 'Mi Tan luku réy',
'watchlistfor'         => "(fu '''$1''')",
'addedwatch'           => 'Presi a yu sirey',
'addedwatchtext'       => "A papira \"[[:\$1]]\" ben presi a yu [[Special:Watchlist|sirey]]. Folo kenki fu disi papira nanga a taki sey opo [[Special:Watchlist|yu sirey]] nanga sey '''deku''' ini a [[Special:RecentChanges|rey fu bakseywan kenki]].

Iksi yu wan papira no langar wana si, go na a papira nanga du opo \"No si\" ini a menu.",
'removedwatch'         => 'Trowe fu yu sirey',
'removedwatchtext'     => 'A papira "[[:$1]]" ben trowe fu yu sirey.',
'watch'                => 'Tan luku',
'watchthispage'        => 'Tan luku a papira disi',
'unwatch'              => 'No tan luku',
'watchlist-details'    => '{{PLURAL:$1|Wan papira|$1 papira}} de ini yu Tan Luku réy, sondro fu teri den kruderi papira.',
'wlshowlast'           => 'Sori laste $1 yuru, $2 dey ($3)',
'watchlist-hide-bots'  => 'Kibri kenki fu den bot',
'watchlist-hide-own'   => 'Kibri mi kenki',
'watchlist-hide-minor' => 'Kibri pikin kenki',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => 'A wiki e poti a papira disi ini yu Tan Luku...',
'unwatching' => 'A wiki e puru a papira disi fu yu Tan Luku...',

# Delete/protect/revert
'deletepage'                  => 'Disi papira trowe',
'historywarning'              => 'Warskow: a papira di yu wani puru abi wan historia:',
'confirmdeletetext'           => 'Yu wanaefru ben trowen wan papira, nanga si stori. Gi klari a dyaso-ondro ini a box taki disi we fuli yu miki ben, taki yu den folo luku nanga taki a trowe gu ben nanga a [[{{MediaWiki:Policy-url}}|polisi]].',
'actioncomplete'              => 'Aksi e du',
'deletedtext'                 => '"<nowiki>$1</nowiki>" ben e trowe. Si a $2 fu wan sibuku fu bakaseywan trowe.',
'deletedarticle'              => 'puru "[[$1]]"',
'dellogpage'                  => 'Log buku fu puru',
'deletecomment'               => 'Yesikrari fu trowe:',
'deleteotherreason'           => 'Trawan/okwan yesikrari:',
'deletereasonotherlist'       => 'Trawan yesikrari',
'rollbacklink'                => 'drai baka',
'protectlogpage'              => 'Log buku fu den sroto',
'protect-legend'              => 'Gi tapu klari',
'protectcomment'              => 'Opotaki:',
'protectexpiry'               => 'Lasi:',
'protect_expiry_invalid'      => 'A lasi ben fowtu.',
'protect_expiry_old'          => 'A lasi ben ini iksini.',
'protect-unchain'             => 'Gi dribi u',
'protect-text'                => 'Dyaso ben yu kan tu kenki nanga aluku a tapunivo fu a papira <strong><nowiki>$1</nowiki></strong>.',
'protect-locked-access'       => "'''Yu masyin abi no lesi tu kenki a tapunivo.'''
Disi ben a tapunivo fu a papira <strong>[[$1]]</strong> now:",
'protect-cascadeon'           => 'Disi papira ben tapu sins a ini den folo {{PLURAL:$1|papira|peprewoysi}} ben e presi, dy tapu ben nanga a kaskade-opsi. A tapunivo kenki abi no efekti.',
'protect-default'             => '(soma saki)',
'protect-fallback'            => 'Dyaso ben a lesi "$1" fanowdu',
'protect-level-autoconfirmed' => 'Wawan rigistir masyin',
'protect-level-sysop'         => 'Wawan sesopu',
'protect-summary-cascade'     => 'kaskada',
'protect-expiring'            => 'lasi opo $1',
'protect-cascade'             => 'Kaskadetapu - tapu ala peprewoysi nanga ankra dy ini disi papira teki ben (atessi; disi kan gran folo abi).',
'protect-cantedit'            => 'Yu kan a tapunivo fu disi papira no kenki, sins yu no lesi abi tu kenki a.',
'restriction-type'            => 'Den leti:',
'restriction-level'           => 'Pelkinivo:',

# Undelete
'undeletebtn'            => 'Poti baka',
'undelete-search-submit' => 'Suku',

# Namespace form on various pages
'namespace'      => 'Nen-presi',
'invert'         => 'Drai a teki',
'blanknamespace' => '(Fesi nen-presi)',

# Contributions
'contributions' => 'Sani di a kebroikiman du',
'mycontris'     => 'Sani di mi du',
'contribsub2'   => 'Fu $1 ($2)',
'uctop'         => '(a moro nyun kenki)',
'month'         => 'Fu a mun (nanga moro owru):',
'year'          => 'Fu a yari (nanga moro owru):',

'sp-contributions-newbies-sub' => 'Gi nyun account',
'sp-contributions-blocklog'    => 'Log buku fu den tapu pasi',
'sp-contributions-submit'      => 'Suku',

# What links here
'whatlinkshere'       => 'San e miti kon dyaso',
'whatlinkshere-title' => 'Papira di e sori go na $1',
'linklistsub'         => '(Réy fu miti)',
'linkshere'           => "Den papira disi e miti go na '''[[:$1]]''':",
'nolinkshere'         => "No wan papira e miti kon na '''[[:$1]]'''.",
'isredirect'          => 'papira fu drai go',
'istemplate'          => 'poti leki wan template',
'whatlinkshere-prev'  => '{{PLURAL:$1|a wan|den $1}} di psa',
'whatlinkshere-next'  => "{{PLURAL:$1|a wan|den $1}} d'e kon",
'whatlinkshere-links' => '← miti kon dyaso',

# Block/unblock
'blockip'            => 'Tapu pasi gi kebroikiman',
'ipboptions'         => '15 min:15 min,1 yuru:1 hour,2 yuru:2 hours,6 yuru:6 hours,12 yuru:12 hours,1 dey:1 day,3 dey:3 days,1 wiki:1 week,2 wiki:2 weeks,1 mun:1 month,3 mun:3 months,6 mun:6 months,1 yari:1 year,fu têgo:infinite', # display1:time1,display2:time2,...
'ipblocklist'        => 'Réy fu tapu pasi gi kebroikiman nen nanga IP',
'ipblocklist-submit' => 'Suku',
'blocklink'          => 'tapu pasi',
'unblocklink'        => 'opo pasi gi',
'contribslink'       => 'kenki',
'blocklogpage'       => 'Log buku fu den tapu pasi',
'blocklogentry'      => 'patu pasi gi "[[$1]]" te go miti $2 $3',

# Move page
'move-page-legend' => 'Dribi papira',
'movepagetext'     => "Nanaga a ondroben box kan yu wan papira dribi.
A stori go na a nyun papira.
A owru nen sey wan stir na a nyun papira sey.
Stir na a owru papira ben no kenki.
Luku na a dribi efu da no tustir efu broko stir ben e kon.
Yu ben risponsibu fu den stir.

A papira kan '''wawan''' dribi sey leki a nyun papiranen:
* no da ben, efu
* a stirpapira nanga no stori ben.

'''ATESSI!'''
Fu poppelari peprewoysi kan a drabi drastiki nanga ofusi folo abi.
Ben suri taki den folo abrasi ben pre yu disi aksi du.",
'movepagetalktext' => "A takipapira sey a trawan nen, '''iksi''':
* A takipapira ondro a nyun nen da ala ben;
* Yu a ondroben box odu.",
'movearticle'      => 'Dribi papira:',
'newtitle'         => 'Na nyun papiranen:',
'move-watch'       => 'Disi papira si',
'movepagebtn'      => 'Dribi papira',
'pagemovedsub'     => 'Dribi fu a papira ben gu',
'articleexists'    => 'A papira ben ala da efu a papira nen ben fowtu.
Gi wan trawan papiranen.',
'talkexists'       => "'''A papira ben dribi, ma a taki papira kan no dribi sey sins da ala wan papira nanga a nyun nen ben. Presi den takipeprewoysi yuse.'''",
'movedto'          => 'ben dribi na',
'movetalk'         => 'Taki papira nangadribi',
'1movedto2'        => '[[$1]] froisi go na [[$2]]',
'movelogpage'      => 'Log buku fu froisi',
'movereason'       => 'Yesikrari:',
'revertmove'       => 'drai baka',

# Export
'export' => 'Export',

# Namespace 8 related
'allmessages'     => 'Boskopu fu a systeem',
'allmessagesname' => 'Nen',

# Thumbnails
'thumbnail-more'  => 'Moro bigi',
'thumbnail_error' => 'Fowtu na a meki fu a thumbnail: $1',

# Import log
'importlogpage' => "Log buku fu den sen'teki",

# Tooltip help for the actions
'tooltip-pt-userpage'             => 'Mi kebroikiman papira',
'tooltip-pt-mytalk'               => 'Mi taki',
'tooltip-pt-preferences'          => 'Seti na mi fa',
'tooltip-pt-watchlist'            => "Den papira di m'e tan luku fu si efu den e kenki",
'tooltip-pt-mycontris'            => 'Ala sani di mi du dyaso',
'tooltip-pt-login'                => "A b'o bun efu yu psa kon, ma a no de ferplekti.",
'tooltip-pt-logout'               => 'Gwe',
'tooltip-ca-talk'                 => 'Taki abra a papira disi',
'tooltip-ca-edit'                 => 'Yu kan kenki a papira disi.<br />
Luku-na-fesi bifo yu kibri.',
'tooltip-ca-addsection'           => 'Poti wan boskopu ini a kruderi disi.',
'tooltip-ca-viewsource'           => 'A papira disi sroto.
Yu ka luku a source.',
'tooltip-ca-protect'              => 'Sroto a papira disi',
'tooltip-ca-delete'               => 'Puru a papira disi',
'tooltip-ca-move'                 => 'Froisi a papira disi',
'tooltip-ca-watch'                => 'Poti a papira disi ini yu Tan Luku réy',
'tooltip-ca-unwatch'              => 'Puru a papira disi fu mi Tan Luku réy',
'tooltip-search'                  => 'Suku ini {{SITENAME}}',
'tooltip-p-logo'                  => 'Fruwondruwiwiri',
'tooltip-n-mainpage'              => 'Go na a Fesipapira',
'tooltip-n-portal'                => 'Abra a project, san yu kan du, pe fu feni sani',
'tooltip-n-currentevents'         => 'Kon sabi moro fu de sani di e psa nownowde',
'tooltip-n-recentchanges'         => 'Den laste kenki ini a wiki.',
'tooltip-n-randompage'            => 'Luku iniwan papira',
'tooltip-n-help'                  => 'A presi fu feni yepi.',
'tooltip-t-whatlinkshere'         => 'Ala wiki papira di e sori kon na a papira disi',
'tooltip-t-contributions'         => 'Sori san a kebroikiman disi du dyaso',
'tooltip-t-emailuser'             => 'Seni wan e-mail gi a kebroikiman disi',
'tooltip-t-upload'                => 'Lai file poti',
'tooltip-t-specialpages'          => 'Ala spesrutu papira',
'tooltip-ca-nstab-user'           => 'Luku a kebroikiman papira',
'tooltip-ca-nstab-media'          => 'Papira fu media libi si',
'tooltip-ca-nstab-project'        => 'Luku a project papira',
'tooltip-ca-nstab-image'          => 'Luku file papira',
'tooltip-ca-nstab-mediawiki'      => 'Boskopu libi si',
'tooltip-ca-nstab-template'       => 'Luku a template',
'tooltip-ca-nstab-help'           => 'Luku a yepi papira',
'tooltip-ca-nstab-category'       => 'Luku a grupu papira',
'tooltip-minoredit'               => 'Marki disi leki wan pikin kenki',
'tooltip-save'                    => 'Kibri yu kenki',
'tooltip-preview'                 => 'Luku yu kenki na fesi. Du disi bifo yu kibri yu kenki!',
'tooltip-diff'                    => 'Sori sortu kenki yu meki ini a papira',
'tooltip-compareselectedversions' => 'Luku den difrenti fu de tu versi fu a papira di teki.',
'tooltip-watch'                   => 'Poti a papira disi ini yu Tan Luku réy',

# Attribution
'siteuser'  => '{{SITENAME}}-masyin $1',
'siteusers' => '{{SITENAME}}-masyin $1',

# Browsing diffs
'previousdiff' => '← A psa kenki',
'nextdiff'     => "A kenki d'e kon →",

# Media information
'widthheightpage'      => '$1×$2, $3 peprewoysi',
'file-info-size'       => '($1 × $2 pixel, file marki: $3, MIME-type: $4)',
'file-nohires'         => '<small>Moro srapu no de.</small>',
'svg-long-desc'        => '(SVG file, marki $1 × $2 pixel, bigi: $3)',
'show-big-image'       => 'Moro srapu',
'show-big-image-thumb' => '<small>Bigi fu a luku-na-fesi disi: $1 × $2 pixel</small>',

# Special:NewImages
'newimages'    => 'Nyun file',
'showhidebots' => '(Bot $1)',
'noimages'     => 'Noti a si.',
'ilsubmit'     => 'Suku',
'bydate'       => 'opo datum',

# Bad image list
'bad_image_list' => 'A opomeki ben leki ondro:

Wawan lina ini a rei (lina dy bigin nanga *) ben awroko. A foswan skaki opo a lina mu a skaki ben na a owinsi gefre.
Ala trawan skaki dy opo se lina ben, ben awroko leki spesyal, leki fru eksempre peprewoysi pe opo a gefre ini a si ben opoteki.',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => "A file disi abi moro informatie, kande fu wan fotocamera noso wan scanner di ben kebroiki fu meki en.
Efu a file ben kenki, a kan taki son fin'fini no sa de srefi leki a fosi file.",
'metadata-expand'   => "Sori moro fin'fini",
'metadata-collapse' => "Kibri a fin'fini",
'metadata-fields'   => 'Den EXIF-metadata boksu ini a boskopu disi o sori owktu tapu wan prenki papira, efu a metadata tabel tapu.
Trawan o kibri.
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# External editor support
'edit-externally'      => 'Kenki a file disi ini wan dorosey wrokosani.',
'edit-externally-help' => 'Luku ini a [http://www.mediawiki.org/wiki/Manual:External_editors skorobuku fu den seti] gi moro yepi.',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => 'ala',
'imagelistall'     => 'ala',
'watchlistall2'    => 'ala',
'namespacesall'    => 'ala',
'monthsall'        => 'ala',

# action=purge
'confirm_purge_button' => 'oki',

# Multipage image navigation
'imgmultigo' => 'Go!',

# Table pager
'ascending_abbrev'         => 'opo.',
'descending_abbrev'        => 'afo.',
'table_pager_limit_submit' => 'Go',

# Auto-summaries
'autosumm-new' => 'Nyon papira: $1',

# Watchlist editing tools
'watchlisttools-view' => 'Sori Tan Luku réy',
'watchlisttools-edit' => 'Luku nanga kenki my Tan Luku réy',
'watchlisttools-raw'  => 'Kenki a lala Tan Luku réy',

# Special:Version
'version' => 'Versi', # Not used as normal message but as header for the special page itself

# Special:SpecialPages
'specialpages' => 'Spesrutu papira',

);
