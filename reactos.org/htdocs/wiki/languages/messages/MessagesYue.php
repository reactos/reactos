<?php
/** Yue (粵語)
 *
 * @ingroup Language
 * @file
 *
 */

$skinNames = array(
	'standard'    => '傳統', /* "Classic, Standard" */
	'nostalgia'   => '懷舊', /* "Nostalgia" */
	'cologneblue' => '科隆藍', /* "Cologne Blue" */
	'monobook'    => 'MonoBook',
	'myskin'      => '我嘅畫面', /* "MySkin" */
	'chick'       => '俏', /* "Chick" */
	'simple'      => '簡單', /* "Simple" */
	'modern'      => '摩登', /* "Modern" */
);

$bookstoreList = array(
	'AddALL' => 'http://www.addall.com/New/Partner.cgi?query=$1&type=ISBN',
	'PriceSCAN' => 'http://www.pricescan.com/books/bookDetail.asp?isbn=$1',
	'Barnes & Noble' => 'http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1',
	'亞馬遜' => 'http://www.amazon.com/exec/obidos/ISBN=$1',
	'博客來書店' => 'http://www.books.com.tw/exep/prod/booksfile.php?item=$1',
	'三民書店' => 'http://www.sanmin.com.tw/page-qsearch.asp?ct=search_isbn&qu=$1',
	'天下書店' => 'http://www.cwbook.com.tw/search/result1.jsp?field=2&keyWord=$1',
	'新絲路書店' => 'http://www.silkbook.com/function/Search_list_book_data.asp?item=5&text=$1'
);

$datePreferences = array(
	'default',
	'yue dmy',
	'yue mdy',
	'yue ymd',
	'ISO 8601',
);

$defaultDateFormat = 'yue';

$dateFormats = array(
	'yue time' => 'H:i',
	'yue date' => 'Y年n月j號 (l)',
	'yue both' => 'Y年n月j號 (D) H:i',

	'yue dmy time' => 'H:i',
	'yue dmy date' => 'j-n-Y',
	'yue dmy both' => 'j-n-Y H:i',

	'yue mdy time' => 'H:i',
	'yue mdy date' => 'n-j-Y',
	'yue mdy both' => 'n-j-Y H:i',

	'yue ymd time' => 'H:i',
	'yue ymd date' => 'Y-n-j',
	'yue ymd both' => 'Y-n-j H:i',
);

$namespaceNames = array(
	NS_MEDIA 		=> 'Media',
	NS_SPECIAL 		=> 'Special',
	NS_MAIN			=> '',
	NS_TALK			=> 'Talk',
	NS_USER 		=> 'User',
	NS_USER_TALK 		=> 'User_talk',
	# NS_PROJECT 		=> $wgMetaNamespace,
	NS_PROJECT_TALK 	=> '$1_talk',
	NS_IMAGE 		=> 'Image',
	NS_IMAGE_TALK 		=> 'Image_talk',
	NS_MEDIAWIKI 		=> 'MediaWiki',
	NS_MEDIAWIKI_TALK 	=> 'MediaWiki_talk',
	NS_TEMPLATE 		=> 'Template',
	NS_TEMPLATE_TALK 	=> 'Template_talk',
	NS_HELP 		=> 'Help',
	NS_HELP_TALK 		=> 'Help_talk',
	NS_CATEGORY 		=> 'Category',
	NS_CATEGORY_TALK 	=> 'Category_talk',
);

$namespaceAliases = array(
	"媒體" 			=> NS_MEDIA,
	"媒体" 			=> NS_MEDIA,
	"特別" 			=> NS_SPECIAL,
	"特殊" 			=> NS_SPECIAL,
	"對話" 			=> NS_TALK,
	"对话" 			=> NS_TALK,
	"討論" 			=> NS_TALK,
	"讨论" 			=> NS_TALK,
	"用戶" 			=> NS_USER,
	"用户" 			=> NS_USER,
	"用戶 對話" 		=> NS_USER_TALK,
	"用户 对话" 		=> NS_USER_TALK,
	"用戶 討論" 		=> NS_USER_TALK,
	"用户 讨论" 		=> NS_USER_TALK,
	# This has never worked so it's unlikely to annoy anyone if I disable it -- TS
	#"{$wgMetaNamespace} 討論" => NS_PROJECT_TALK,
	#"{$wgMetaNamespace} 讨论" => NS_PROJECT_TALK,
	"圖" 			=> NS_IMAGE,
	"圖像" 			=> NS_IMAGE,
	"图" 			=> NS_IMAGE,
	"图像" 			=> NS_IMAGE,
	"圖 討論" 		=> NS_IMAGE_TALK,
	"图 讨论" 		=> NS_IMAGE_TALK,
	"圖像 討論" 		=> NS_IMAGE_TALK,
	"图像 讨论" 		=> NS_IMAGE_TALK,
	"模" 			=> NS_TEMPLATE,
	"模 討論" 		=> NS_TEMPLATE_TALK,
	"模 讨论" 		=> NS_TEMPLATE_TALK,
	"幫手" 			=> NS_HELP,
	"幫助" 			=> NS_HELP,
	"說明" 			=> NS_HELP,
	"帮手" 			=> NS_HELP,
	"帮助" 			=> NS_HELP,
	"说明" 			=> NS_HELP,
	"幫手 討論" 		=> NS_HELP_TALK,
	"幫助 討論" 		=> NS_HELP_TALK,
	"說明 討論" 		=> NS_HELP_TALK,
	"帮手 讨论" 		=> NS_HELP_TALK,
	"帮助 讨论" 		=> NS_HELP_TALK,
	"说明 讨论" 		=> NS_HELP_TALK,
	"類" 			=> NS_CATEGORY,
	"分類" 			=> NS_CATEGORY,
	"类" 			=> NS_CATEGORY,
	"分类" 			=> NS_CATEGORY,
	"類 討論" 		=> NS_CATEGORY_TALK,
	"分類 討論" 		=> NS_CATEGORY_TALK,
	"类 讨论" 		=> NS_CATEGORY_TALK,
	"分类 讨论" 		=> NS_CATEGORY_TALK,
);

$linkTrail = '/^([a-z]+)(.*)$/sD';

#-------------------------------------------------------------------
# Default messages
#-------------------------------------------------------------------
# Allowed characters in keys are: A-Z, a-z, 0-9, underscore (_) and
# hyphen (-). If you need more characters, you may be able to change
# the regex in MagicWord::initRegex

$messages = array(
# User preference toggles
'tog-underline'               => '連結加底線：',
'tog-highlightbroken'         => '格式化連結 <a href="" class="new">好似咁</a>（又或者: 好似咁<a href="" class="internal">?</a>）.',
'tog-justify'                 => '拍齊段落',
'tog-hideminor'               => '最新更改唔顯示小修改',
'tog-extendwatchlist'         => '展開監視清單去顯示合適嘅更改',
'tog-usenewrc'                => '強化版最近更改（JavaScript）',
'tog-numberheadings'          => '標題自動編號',
'tog-showtoolbar'             => '顯示修改工具列（JavaScript）',
'tog-editondblclick'          => '撳兩下改嘢（JavaScript）',
'tog-editsection'             => '可以用 [修改] 掣更改個別段落',
'tog-editsectiononrightclick' => '可以撳右掣更改個別段落（JavaScript）',
'tog-showtoc'                 => '多過三段時顯示目錄',
'tog-rememberpassword'        => '響呢部電腦度記住我嘅密碼',
'tog-editwidth'               => '全螢幕咁闊嘅修改欄',
'tog-watchcreations'          => '將我開嘅頁加入監視清單',
'tog-watchdefault'            => '將我修改嘅頁加入監視清單',
'tog-watchmoves'              => '將我移動嘅頁加入監視清單',
'tog-watchdeletion'           => '將我刪除嘅頁加入監視清單',
'tog-minordefault'            => '預設全部編輯做小修改',
'tog-previewontop'            => '喺修改欄上方顯示預覽',
'tog-previewonfirst'          => '第一次修改時顯示預覽',
'tog-nocache'                 => '停用頁面快取',
'tog-enotifwatchlistpages'    => '當響我張監視清單度嘅頁面有修改時電郵通知我',
'tog-enotifusertalkpages'     => '個人留言版有修改時電郵通知我',
'tog-enotifminoredits'        => '小修改都要電郵通知我',
'tog-enotifrevealaddr'        => '喺電郵通知信上面話畀人聽我嘅電郵地址',
'tog-shownumberswatching'     => '顯示有幾多人監視',
'tog-fancysig'                => '程式碼簽名（冇自動連結）',
'tog-externaleditor'          => '預設用外掛編輯器（高階者專用，需要響你部電腦度做一啲特別設定）',
'tog-externaldiff'            => '預設用外掛比較器（高階者專用，需要響你部電腦度做一啲特別設定）',
'tog-showjumplinks'           => '啟用 "跳至" 協助連結',
'tog-uselivepreview'          => '用即時預覽（JavaScript）（實驗緊）',
'tog-forceeditsummary'        => '我冇入修改註解時通知我',
'tog-watchlisthideown'        => '響監視清單度隱藏我嘅編輯',
'tog-watchlisthidebots'       => '響監視清單度隱藏機械人嘅編輯',
'tog-watchlisthideminor'      => '響監視清單度隱藏小修改',
'tog-nolangconversion'        => '唔要用字轉換',
'tog-ccmeonemails'            => '當我寄電郵畀其他人嗰陣寄返封副本畀我',
'tog-diffonly'                => '響差異下面唔顯示頁面內容',
'tog-showhiddencats'          => '顯示隱藏類',

'underline-always'  => '全部',
'underline-never'   => '永不',
'underline-default' => '瀏覽器預設',

'skinpreview' => '(預覽)',

# Dates
'sunday'        => '星期日',
'monday'        => '星期一',
'tuesday'       => '星期二',
'wednesday'     => '星期三',
'thursday'      => '星期四',
'friday'        => '星期五',
'saturday'      => '星期六',
'sun'           => '日',
'mon'           => '一',
'tue'           => '二',
'wed'           => '三',
'thu'           => '四',
'fri'           => '五',
'sat'           => '六',
'january'       => '1月',
'february'      => '2月',
'march'         => '3月',
'april'         => '4月',
'may_long'      => '5月',
'june'          => '6月',
'july'          => '7月',
'august'        => '8月',
'september'     => '9月',
'october'       => '10月',
'november'      => '11月',
'december'      => '12月',
'january-gen'   => '一月',
'february-gen'  => '二月',
'march-gen'     => '三月',
'april-gen'     => '四月',
'may-gen'       => '五月',
'june-gen'      => '六月',
'july-gen'      => '七月',
'august-gen'    => '八月',
'september-gen' => '九月',
'october-gen'   => '十月',
'november-gen'  => '十一月',
'december-gen'  => '十二月',
'jan'           => '1月',
'feb'           => '2月',
'mar'           => '3月',
'apr'           => '4月',
'may'           => '5月',
'jun'           => '6月',
'jul'           => '7月',
'aug'           => '8月',
'sep'           => '9月',
'oct'           => '10月',
'nov'           => '11月',
'dec'           => '12月',

# Categories related messages
'pagecategories'                 => '屬於$1類',
'category_header'                => '"$1" 類中嘅版',
'subcategories'                  => '分類',
'category-media-header'          => ' "$1" 類嘅媒體',
'category-empty'                 => "''呢類無任何版或媒體檔。''",
'hidden-categories'              => '屬於$1隱類',
'hidden-category-category'       => '隱藏類', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|呢類淨係有下面嘅細類。|呢類有下面嘅$1個細類，總共有$2類。}}',
'category-subcat-count-limited'  => '呢個類別入邊有$1個細類別。',
'category-article-count'         => '{{PLURAL:$2|呢類淨係有下面嘅版。|呢類有下面嘅$1版，總共有$2版。}}',
'category-article-count-limited' => '呢個類別入邊有$1版。',
'category-file-count'            => '{{PLURAL:$2|呢類淨係有下面嘅檔案。|呢類有下面嘅$1個檔案，總共有$2個檔案。}}',
'category-file-count-limited'    => '呢個類別入邊有$1個檔案。',
'listingcontinuesabbrev'         => '續',

'mainpagetext'      => "<big>'''MediaWiki已經裝好。'''</big>",
'mainpagedocfooter' => '參閱[http://meta.wikimedia.org/wiki/Help:Contents 用戶指引]（英），裏面有資料講點用wiki軟件。

==開始使用==
* [http://www.mediawiki.org/wiki/Manual:Configuration_settings 配置設定清單]（英）
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki 常見問題]（英）
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki 發佈郵件名單]（英）',

'about'          => '關於',
'article'        => '內容頁',
'newwindow'      => '（響新視窗度打開）',
'cancel'         => '取消',
'qbfind'         => '搵嘢',
'qbbrowse'       => '瀏覽',
'qbedit'         => '編輯',
'qbpageoptions'  => '呢一頁',
'qbpageinfo'     => '附近文字',
'qbmyoptions'    => '我嘅選項',
'qbspecialpages' => '特別頁',
'moredotdotdot'  => '更多...',
'mypage'         => '我嘅頁',
'mytalk'         => '我嘅討論',
'anontalk'       => '同呢個 IP 傾偈',
'navigation'     => '導航',
'and'            => '同埋',

# Metadata in edit box
'metadata_help' => 'Metadata',

'errorpagetitle'    => '錯誤',
'returnto'          => '返去$1 。',
'tagline'           => '出自{{SITENAME}}',
'help'              => '幫助',
'search'            => '搵嘢',
'searchbutton'      => '搵嘢',
'go'                => '去',
'searcharticle'     => '去',
'history'           => '版史',
'history_short'     => '史',
'updatedmarker'     => '我上次來之後嘅修改',
'info_short'        => '資訊',
'printableversion'  => '可打印版本',
'permalink'         => '永久連結',
'print'             => '印',
'edit'              => '編輯',
'create'            => '建立',
'editthispage'      => '編輯呢頁',
'create-this-page'  => '建立呢頁',
'delete'            => '刪除',
'deletethispage'    => '刪除呢頁',
'undelete_short'    => '反刪除$1次修改',
'protect'           => '保護',
'protect_change'    => '改',
'protectthispage'   => '保護呢頁',
'unprotect'         => '解除保護',
'unprotectthispage' => '解呢頁嘅保護',
'newpage'           => '開新頁',
'talkpage'          => '討論呢版',
'talkpagelinktext'  => '傾偈',
'specialpage'       => '特別頁',
'personaltools'     => '個人工具',
'postcomment'       => '寫句意見',
'articlepage'       => '睇目錄',
'talk'              => '討論',
'views'             => '去睇',
'toolbox'           => '工具箱',
'userpage'          => '去睇用戶頁',
'projectpage'       => '去睇專題頁',
'imagepage'         => '去睇媒體頁',
'mediawikipage'     => '去睇信息頁',
'templatepage'      => '去睇模頁',
'viewhelppage'      => '去睇幫手頁',
'categorypage'      => '去睇分類頁',
'viewtalkpage'      => '睇討論',
'otherlanguages'    => '第啲語言',
'redirectedfrom'    => '(由$1跳轉過來)',
'redirectpagesub'   => '跳轉頁',
'lastmodifiedat'    => '呢一頁嘅最後修改係響$1 $2。', # $1 date, $2 time
'viewcount'         => '呢一頁已經有$1人次睇過。',
'protectedpage'     => '受保護頁',
'jumpto'            => '跳去:',
'jumptonavigation'  => '定向',
'jumptosearch'      => '搵嘢',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '關於{{SITENAME}}',
'aboutpage'            => 'Project:關於',
'bugreports'           => '臭蟲回報',
'bugreportspage'       => 'Project:臭蟲回報',
'copyright'            => '響版度嘅內容係根據$1嘅條款發佈。',
'copyrightpagename'    => '{{SITENAME}}版權',
'copyrightpage'        => '{{ns:project}}:版權',
'currentevents'        => '最近發生嘅事',
'currentevents-url'    => 'Project:最近發生嘅事',
'disclaimers'          => '免責聲明',
'disclaimerpage'       => 'Project:一般免責聲明',
'edithelp'             => '編輯協助',
'edithelppage'         => 'Help:編輯',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:FAQ',
'helppage'             => 'Help:目錄',
'mainpage'             => '頭版',
'mainpage-description' => '頭版',
'policy-url'           => 'Project:政策',
'portal'               => '社區大堂',
'portal-url'           => 'Project:社區大堂',
'privacy'              => '私隱政策',
'privacypage'          => 'Project:私隱政策',

'badaccess'        => '權限錯誤',
'badaccess-group0' => '你係唔准執行你要求嘅動作。',
'badaccess-group1' => '你所要求嘅動作只係限制畀$1組嘅用戶。',
'badaccess-group2' => '你所要求嘅動作只係限制畀$1組嘅其中一位用戶。',
'badaccess-groups' => '你所要求嘅動作只係限制畀$1組嘅其中一位用戶。',

'versionrequired'     => '係需要用 $1 版嘅 MediaWiki',
'versionrequiredtext' => '要用呢一頁，要用MediaWiki版本 $1 。睇睇[[Special:Version|版本頁]]。',

'ok'                      => 'OK',
'retrievedfrom'           => '由 "$1" 收',
'youhavenewmessages'      => '你有$1（$2）。',
'newmessageslink'         => '新信息',
'newmessagesdifflink'     => '上次更改',
'youhavenewmessagesmulti' => '你響 $1 有新信',
'editsection'             => '編輯',
'editold'                 => '編輯',
'viewsourceold'           => '睇吓原始碼',
'editsectionhint'         => '編輯小節: $1',
'toc'                     => '目錄',
'showtoc'                 => '展開',
'hidetoc'                 => '收埋',
'thisisdeleted'           => '睇下定係還原 $1 ？',
'viewdeleted'             => '去睇$1？',
'restorelink'             => '$1 次已刪除嘅編輯',
'feedlinks'               => 'Feed:',
'feed-invalid'            => '唔啱嘅 feed 類型。',
'feed-unavailable'        => '聯合 feeds 並唔係響{{SITENAME}}度提供',
'site-rss-feed'           => '$1嘅RSS Feed',
'site-atom-feed'          => '$1嘅Atom Feed',
'page-rss-feed'           => '"$1"嘅RSS Feed',
'page-atom-feed'          => '"$1"嘅Atom Feed',
'red-link-title'          => '$1 (未寫)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '版',
'nstab-user'      => '用戶頁',
'nstab-media'     => '媒體頁',
'nstab-special'   => '特別頁',
'nstab-project'   => '計劃頁',
'nstab-image'     => '檔案',
'nstab-mediawiki' => '信息',
'nstab-template'  => '模',
'nstab-help'      => '幫助頁',
'nstab-category'  => '分類',

# Main script and global functions
'nosuchaction'      => '冇呢個動作',
'nosuchactiontext'  => '呢個 URL 嘅指定動作 wiki 識別唔到',
'nosuchspecialpage' => '冇呢頁特別頁',
'nospecialpagetext' => "<big>'''無你所要求嘅特別頁。'''</big>

喺[[Special:SpecialPages|{{int:specialpages}}]]有全部用得嘅特別頁。",

# General errors
'error'                => '錯誤',
'databaseerror'        => '資料庫錯誤',
'dberrortext'          => '資料庫查詢語法錯咗。
咁係可能指出軟件中可能有臭蟲。
最後一次資料庫嘅嘗試係：
<blockquote><tt>$1</tt></blockquote>
於 "<tt>$2</tt>" 功能中。
MySQL 嘅錯誤回應 "<tt>$3: $4</tt>"。',
'dberrortextcl'        => '資料庫查詢語法錯咗。
最後一次資料庫嘅嘗試係：
"$1"
於 "$2"功能中。
MySQL 嘅錯誤回應 "$3: $4"',
'noconnect'            => '對唔住！Wiki 而家有啲技術問題，連唔上資料庫伺服器。<br />
$1',
'nodb'                 => '伺服器 $1 唔選得',
'cachederror'          => '以下係已請求頁嘅快取複本，內容可能唔係最新嘅。',
'laggedslavemode'      => '警告：呢頁可能未包括最新嘅更新。',
'readonly'             => '資料庫鎖咗',
'enterlockreason'      => '輸入鎖資料庫嘅原因，同埋預計幾耐後會解鎖',
'readonlytext'         => '{{SITENAME}}資料庫而家鎖住咗，唔改得；可能因為維修緊。搞掂就會正常返。

管理員嘅解釋： $1',
'missing-article'      => '資料庫搵唔到你要嘅版，「$1」 $2。

通常係因為修訂歷史頁上面，由過時嘅連結去到刪除咗嘅版所引起嘅。

如果唔係，你可能係搵到軟件裏面嘅臭蟲。
請記低 URL 地址，向[[Special:ListUsers/sysop|管理員]]報告。',
'missingarticle-rev'   => '(修訂#: $1)',
'missingarticle-diff'  => '(差異: $1, $2)',
'readonly_lag'         => '當從伺服器追緊主伺服器時，資料庫會自動被鎖',
'internalerror'        => '內部錯誤',
'internalerror_info'   => '內部錯誤: $1',
'filecopyerror'        => '檔案 "$1" 抄唔到去 "$2"。',
'filerenameerror'      => '檔案 "$1" 唔改得做 "$2"。',
'filedeleteerror'      => '檔案 "$1" 唔刪得。',
'directorycreateerror' => '目錄 "$1" 開唔到。',
'filenotfound'         => '檔案 "$1" 搵唔到。',
'fileexistserror'      => '檔案 "$1" 寫唔到: 檔案已經存在',
'unexpected'           => '意外數值。 "$1"="$2"。',
'formerror'            => '錯誤：表格交唔到',
'badarticleerror'      => '喺呢頁唔可以做呢個動作。',
'cannotdelete'         => '指定嗰頁或檔案唔刪得。（可能已經刪咗。）',
'badtitle'             => '錯嘅標題',
'badtitletext'         => '要求嘅標題唔啱、空白，跨語言或者跨維基連結標題錯誤。亦可能係標題包括咗一個或多過一個字元。',
'perfdisabled'         => '對唔住！呢個功能有可能造成資料庫癱瘓，所以要暫時停用。',
'perfcached'           => '以下嘅資料係嚟自快取，可能唔係最新嘅。',
'perfcachedts'         => '以下嘅資料係嚟自快取，上一次嘅更新喺$1。',
'querypage-no-updates' => '響呢一頁嘅更新現時停用。啲資料將唔會即時更新。',
'wrong_wfQuery_params' => 'wfQuery() 嘅參數錯誤<br />
函數： $1<br />
查詢： $2',
'viewsource'           => '睇吓原始碼',
'viewsourcefor'        => '$1嘅原始碼',
'actionthrottled'      => '動作已壓制',
'actionthrottledtext'  => '基於反垃圾嘢嘅考量，你而家響呢段短時間之內限制咗去做呢一個動作，而你已經超過咗個上限。請響幾分鐘之後再試過。',
'protectedpagetext'    => '呢一頁已經鎖咗唔畀改。',
'viewsourcetext'       => '你可以睇吓或者複製呢一頁嘅原始碼：',
'protectedinterface'   => '呢一頁提供軟件嘅介面文字，呢一頁已經鎖上以預防濫用。',
'editinginterface'     => "'''警告：'''你而家編輯緊嘅呢一個用嚟提供介面文字嘅頁面。響呢一頁嘅更改會影響到其他用戶使用中嘅介面外觀。要翻譯，請考慮利用[http://translatewiki.net/wiki/Main_Page?setlang=yue Betawiki]，一個用來為MediaWiki軟件本地化嘅計劃。",
'sqlhidden'            => '(SQL 查詢隱藏)',
'cascadeprotected'     => '呢一版已經保護咗唔能夠編輯，因為佢係響以下嘅{{PLURAL:$1|一|幾}}頁度包含咗，當中啟用咗"連串"保護選項來保護嗰一版: $2',
'namespaceprotected'   => "你無權編輯響'''$1'''空間名裏面嘅呢一版。",
'customcssjsprotected' => '你無權編輯呢一版，因為佢包含咗另一位用戶嘅個人設定。',
'ns-specialprotected'  => '特別頁係唔可以編輯嘅。',
'titleprotected'       => "呢個標題已經俾[[User:$1|$1]]保護咗防止去開。原因係''$2''。",

# Virus scanner
'virus-badscanner'     => "壞設定: 未知嘅病毒掃瞄器: ''$1''",
'virus-scanfailed'     => '掃瞄失敗 (代碼 $1)',
'virus-unknownscanner' => '未知嘅防病毒:',

# Login and logout pages
'logouttitle'                => '用戶登出',
'logouttext'                 => "'''你而家已經登出咗。'''

你仍然可以用匿名身份用{{SITENAME}}，又或者[[Special:UserLogin|重新登入]]。但係留意某啲頁面可能會繼續話你登入咗，除非等你清除瀏覽器嘅快取儲存。",
'welcomecreation'            => '== 歡迎， $1！ ==

你個戶口已經起好。唔好唔記得去改改你嘅[[Special:Preferences|{{SITENAME}}喜好設定]]喎。',
'loginpagetitle'             => '用戶登入',
'yourname'                   => '用戶名:',
'yourpassword'               => '密碼:',
'yourpasswordagain'          => '再輸入密碼:',
'remembermypassword'         => '響呢部電腦度記住我嘅密碼',
'yourdomainname'             => '你嘅網域:',
'externaldberror'            => '外部驗證資料庫出錯，或者唔允許你更新你嘅外部帳戶。',
'loginproblem'               => "'''你嘅登入手續出咗問題。'''

唔該再試吓登入。",
'login'                      => '登入',
'nav-login-createaccount'    => '登入／開新戶口',
'loginprompt'                => '你一定要開咗 cookies 先登入到{{SITENAME}}。',
'userlogin'                  => '登入／開新戶口',
'logout'                     => '登出',
'userlogout'                 => '登出',
'notloggedin'                => '未登入',
'nologin'                    => '重未有戶口？ $1。',
'nologinlink'                => '開一個新嘅戶口',
'createaccount'              => '建立戶口',
'gotaccount'                 => '已經有戶口？ $1 。',
'gotaccountlink'             => '登入',
'createaccountmail'          => '用電郵',
'badretype'                  => '你所入嘅密碼唔一致。',
'userexists'                 => '你入嘅用戶名已經有人用緊，請揀過另外一個名。',
'youremail'                  => '電郵:',
'username'                   => '用戶名:',
'uid'                        => '用戶 ID:',
'prefs-memberingroups'       => '{{PLURAL:$1|一|多}}組嘅成員:',
'yourrealname'               => '真實姓名:',
'yourlanguage'               => '語言:',
'yourvariant'                => '字體變化:',
'yournick'                   => '簽名:',
'badsig'                     => '無效嘅程式碼簽名。檢查吓 HTML 有無錯。',
'badsiglength'               => '簽名太長喇。
佢嘅長度一定要響$1個字元以下。',
'email'                      => '電郵',
'prefs-help-realname'        => '真名係可以選填嘅。
如果你選擇提供佢，佢會用來喺有需要嘅時候標示你嘅作品。',
'loginerror'                 => '登入錯誤',
'prefs-help-email'           => '電郵地址係可以選填嘅，但當你唔記得咗你個密碼嗰陣可以將新密碼寄番畀你。你亦可以等人可以響唔知你電郵地址嘅情況之下都可以聯絡你。',
'prefs-help-email-required'  => '需要電郵地址。',
'nocookiesnew'               => '已經建立咗戶口，但你未登入。 {{SITENAME}} 要用 cookies 嚟登入。你已經停咗用 cookies。麻煩啟用返先，然後再用你新嘅用戶名同密碼。',
'nocookieslogin'             => '{{SITENAME}} 要用 cookies 嚟登入。你已經停用 cookies。請先啟用後再度試過喇。',
'noname'                     => '你未指定一個有效嘅用戶名。',
'loginsuccesstitle'          => '登入成功',
'loginsuccess'               => "'''你已經成功咁喺{{SITENAME}}登入做「$1」。'''",
'nosuchuser'                 => '呢度冇叫做 "$1"嘅用戶。 請檢查你個名嘅輸入方法，或者[[Special:Userlogin/signup|建立一個新嘅戶口]]。',
'nosuchusershort'            => '呢度冇叫做 "<nowiki>$1</nowiki>"嘅用戶。 請檢查你個名嘅輸入方法。',
'nouserspecified'            => '你需要指定一個用戶名。',
'wrongpassword'              => '密碼唔啱，麻煩你再試多次。',
'wrongpasswordempty'         => '你都未入密碼，唔該再試多次啦。',
'passwordtooshort'           => '你嘅密碼唔啱或者太短。佢最少要有$1個半形字元，同埋要唔同你嘅用戶名。',
'mailmypassword'             => '寄返個密碼畀我',
'passwordremindertitle'      => '{{SITENAME}}嘅新臨時密碼',
'passwordremindertext'       => '有人（可能係你，IP 位置 $1）
請求 {{SITENAME}} 嘅新密碼 ($4)。
而家用戶 "$2" 嘅新臨時密碼設定咗做 "$3"。
如果呢個係你所要求嘅，你就需要即刻登入，揀一個新嘅密碼。

如果係其他人作出呢個請求，
又或者你記得返你嘅密碼而又唔想再轉，
你可以唔使理呢個信息，繼續用舊密碼。',
'noemail'                    => '呢度冇用戶 "$1" 嘅電郵地址。',
'passwordsent'               => '新嘅密碼已經寄咗畀呢位用戶 "$1" 嘅電郵地址。收到之後請重新登入。',
'blocked-mailpassword'       => '你嘅IP地址被鎖住，唔可以用密碼復原功能以防止濫用。',
'eauthentsent'               => '確認電郵已經傳送到指定嘅電郵地址。喺其它嘅郵件傳送到呢個戶口之前，你需要按電郵嘅指示，嚟確認呢個戶口真係屬於你嘅。',
'throttled-mailpassword'     => '一個密碼提醒已經響$1個鐘頭之前發送咗。為咗防止濫用，響$1個鐘頭之內只可以發送一個密碼提醒。',
'mailerror'                  => '傳送電郵錯誤： $1',
'acct_creation_throttle_hit' => '對唔住，你已經開咗 $1 個戶口，唔可以再開多個戶口。',
'emailauthenticated'         => '你嘅電郵地址已經喺 $1 確認。',
'emailnotauthenticated'      => '你嘅電郵地址重未確認。 任何傳送電郵嘅功能都唔會運作。',
'noemailprefs'               => '設置一個電郵地址令到呢啲功能開始運作。',
'emailconfirmlink'           => '確認你嘅電郵地址',
'invalidemailaddress'        => '呢個電郵地址嘅格式唔啱，所以接受唔到。請輸入一個啱格式嘅地址，或清咗嗰個空格。',
'accountcreated'             => '戶口已經建立咗',
'accountcreatedtext'         => '$1嘅戶口起好咗。',
'createaccount-title'        => '響{{SITENAME}}度開個新戶口',
'createaccount-text'         => '有人響{{SITENAME}}度用咗你個電郵開咗個名叫 "$2" 嘅新戶口 ($4)，密碼係 "$3" 。你應該而家登入，改埋個密碼。

如果個戶口係開錯咗嘅話，你可以唔埋呢篇信。',
'loginlanguagelabel'         => '語言：$1',

# Password reset dialog
'resetpass'               => '重設戶口密碼',
'resetpass_announce'      => '你已經用咗一個臨時電郵碼登入。要完成登入，你一定要響呢度定一個新嘅密碼：',
'resetpass_text'          => '<!-- 響呢度加入文字 -->',
'resetpass_header'        => '重設密碼',
'resetpass_submit'        => '設定密碼同登入',
'resetpass_success'       => '你嘅密碼已經成功咁更改！而家幫你登入緊...',
'resetpass_bad_temporary' => '唔啱嘅臨時密碼。你可能已經成功咁更改你嘅密碼，又或者重新請求過一個新嘅臨時密碼。',
'resetpass_forbidden'     => '響{{SITENAME}}度唔可以更改密碼',
'resetpass_missing'       => '響資料度搵唔到嘢。',

# Edit page toolbar
'bold_sample'     => '粗體字',
'bold_tip'        => '粗體字',
'italic_sample'   => '斜體字',
'italic_tip'      => '斜體字',
'link_sample'     => '連結標題',
'link_tip'        => '內部連結',
'extlink_sample'  => 'http://www.example.com 連結標題',
'extlink_tip'     => '連出去（記住加 http:// 開頭）',
'headline_sample' => '標題文字',
'headline_tip'    => '二級標題',
'math_sample'     => '喺呢度插入方程式',
'math_tip'        => '數學方程（LaTeX）',
'nowiki_sample'   => '喺呢度插入非格式代文字',
'nowiki_tip'      => '唔理 wiki 格式',
'image_tip'       => '嵌入檔案',
'media_tip'       => '檔案連結',
'sig_tip'         => '你嘅簽名同埋時間戳',
'hr_tip'          => '橫線（請小心用）',

# Edit pages
'summary'                          => '摘要',
'subject'                          => '主題／標題',
'minoredit'                        => '呢個係小修改',
'watchthis'                        => '睇實呢一頁',
'savearticle'                      => '儲存呢頁',
'preview'                          => '預覽',
'showpreview'                      => '顯示預覽',
'showlivepreview'                  => '實時預覽',
'showdiff'                         => '顯示差異',
'anoneditwarning'                  => "'''警告：'''你重未登入。你嘅 IP 位址會喺呢個頁面嘅修訂歷史中記錄落嚟。",
'missingsummary'                   => "'''提醒：''' 你未提供編輯摘要。如果你再撳多一下儲存嘅話，咁你儲存嘅編輯就會無摘要。",
'missingcommenttext'               => '請輸入一個註解。',
'missingcommentheader'             => "'''提醒：'''你響呢個註解度並無提供一個主題／標題。如果你再撳一次儲存，你嘅編輯就會無題。",
'summary-preview'                  => '摘要預覽',
'subject-preview'                  => '標題／頭條預覽',
'blockedtitle'                     => '用戶已經封鎖',
'blockedtext'                      => "<big>你嘅用戶名或者 IP 位址已經被 $1 封咗。</big>

呢次封鎖係由$1所封嘅。當中嘅原因係''$2''。

* 呢次封鎖嘅開始時間係：$8
* 呢次封鎖嘅到期時間係：$6
* 對於被封鎖者：$7

你可以聯絡 $1 或者其他嘅[[{{MediaWiki:Grouppage-sysop}}|管理員]]，討論呢次封鎖。
除非你已經響你嘅[[Special:Preferences|戶口喜好設定]]入面設定咗有效嘅電郵地址，否則你係唔可以用「電郵呢個用戶」嘅功能。當設定咗一個有效嘅電郵地址之後，呢個功能係唔會封鎖嘅。

你現時嘅 IP 位址係 $3 ，而個封鎖 ID 係 #$5。 請你喺你嘅查詢都註明以上封鎖嘅資料。",
'autoblockedtext'                  => "你嘅IP地址已經被自動封鎖，由於之前嘅另一位用戶係畀$1封咗。
而封鎖嘅原因係：

:''$2''

* 呢次封鎖嘅開始時間係：$8
* 呢次封鎖嘅到期時間係：$6
* 對於被封鎖者：$7

你可以聯絡 $1 或者其他嘅[[{{MediaWiki:Grouppage-sysop}}|管理員]]，討論呢次封鎖。

除非你已經響你嘅[[Special:Preferences|戶口喜好設定]]入面設定咗有效嘅電郵地址，否則你係唔可以用「電郵呢個用戶」嘅功能。當設定咗一個有效嘅電郵地址之後，呢個功能係唔會封鎖嘅。

你現時用緊嘅 IP 地址係 $3，個封鎖 ID 係 #$5。 請喺你嘅查詢都註明呢個封鎖上面嘅資料。",
'blockednoreason'                  => '無原因畀低',
'blockedoriginalsource'            => "有關'''$1'''嘅原始碼響下面列示：",
'blockededitsource'                => "有關'''你'''對'''$1'''嘅'''編輯'''文字響下面列示：",
'whitelistedittitle'               => '需要登入之後先至可以編輯',
'whitelistedittext'                => '你需要$1去編輯呢頁。',
'confirmedittitle'                 => '要用電郵確定咗先可以改',
'confirmedittext'                  => '你個電郵地址要確定咗先可以編輯。唔該先去[[Special:Preferences|喜好設定]]填咗電郵地址，並做埋確認手續。',
'nosuchsectiontitle'               => '無呢個小節',
'nosuchsectiontext'                => '你嘗試編輯嘅小節並唔存在。之不過呢度係無第$1小節，所以係無一個地方去儲存你嘅編輯。',
'loginreqtitle'                    => '需要登入',
'loginreqlink'                     => '登入',
'loginreqpagetext'                 => '你一定$1去睇其它嘅頁面。',
'accmailtitle'                     => '密碼寄咗喇。',
'accmailtext'                      => '「$1」嘅密碼已經寄咗去 $2。',
'newarticle'                       => '(新)',
'newarticletext'                   => "你連連過嚟嘅頁面重未存在。
要起版新嘅，請你喺下面嗰格度輸入。(睇睇[[{{MediaWiki:Helppage}}|自助版]]拎多啲資料。)
如果你係唔覺意嚟到呢度，撳一次你個瀏覽器'''返轉頭'''個掣。",
'anontalkpagetext'                 => "----''呢度係匿名用戶嘅討論頁，佢可能係重未開戶口，或者佢重唔識開戶口。我哋會用數字表示嘅IP地址嚟代表佢。一個IP地址係可以由幾個用戶夾來用。如果你係匿名用戶，同覺得呢啲留言係同你冇關係嘅話，唔該去[[Special:UserLogin/signup|開一個新戶口]]或[[Special:UserLogin|登入]]，避免喺以後嘅留言會同埋其它用戶混淆。''",
'noarticletext'                    => '喺呢一頁而家並冇任何嘅文字，你可以喺其它嘅頁面中[[Special:Search/{{PAGENAME}}|搵呢一頁嘅標題]]或者[{{fullurl:{{FULLPAGENAME}}|action=edit}} 編輯呢一頁]。',
'userpage-userdoesnotexist'        => '用戶戶口"$1"重未開。請響䦒／編輯呢版之前先檢查一下。',
'clearyourcache'                   => "'''注意 - 喺儲存之後，你可能要先略過你嘅瀏覽器快取去睇到更改。'''
'''Mozilla / Firefox / Safari:''' 㩒住''Shift''掣再撳''重新載入''，又或者㩒''Ctrl-F5''或者''Ctrl-R''（喺Macintosh㩒''Command-R''掣）；
'''Konqueror:''' 就咁以撳個''重載''掣，又或者㩒''F5''；
'''Opera:'''喺''工具→喜好設定''之中清佢哋嘅快取；
'''Internet Explorer:''' 㩒住''Ctrl''掣再撳''重新整理''，又或者㩒''Ctrl-F5''掣。",
'usercssjsyoucanpreview'           => "'''提示：'''響儲存前，用「顯示預覽」個掣嚟測試你嘅新CSS/JS。",
'usercsspreview'                   => "'''請注意你而家只係預覽緊你嘅用戶CSS樣式表。'''
'''內容仍未儲存！'''",
'userjspreview'                    => "'''請注意你而家只係測試／預覽緊你定義嘅JavaScript。'''
'''佢嘅內容重未儲存！'''",
'userinvalidcssjstitle'            => "'''警告：''' 無叫做 \"\$1\" 嘅畫面。請記住自訂介面的 .css 和 .js 頁面時應使用細楷，例如：{{ns:user}}:Foo/monobook.css 而唔係 {{ns:user}}:Foo/Monobook.css 。",
'updated'                          => '(己更新)',
'note'                             => '<strong>留意:</strong>',
'previewnote'                      => '<strong>請記住呢個只係預覽；更改嘅内容重未儲存！</strong>',
'previewconflict'                  => '呢個預覽係反映如果你選擇儲存嘅話，嘅上面嘅文字編輯區裏面嘅字會儲存落嚟。',
'session_fail_preview'             => "'''對唔住！由於小節嘅資料唔見咗，我哋唔能夠處理你嘅編輯。
請再試過喇。如果仍然唔得嘅話，試下[[Special:UserLogout|登出]]，然後重新登入。'''",
'session_fail_preview_html'        => "'''對唔住！有關嘅程序資料已經遺失，我哋唔能夠處理你嘅編輯。'''

''由於{{SITENAME}}已經開放咗原 HTML 碼，預覽已經隱藏落嚟以預防 JavaScript 嘅攻擊。''

'''如果呢個係正當嘅編輯嘗試，請再試過。如果重係唔得嘅話，請先[[Special:UserLogout|登出]]然後再登入。'''",
'token_suffix_mismatch'            => "'''因為你嘅用戶端度嘅編輯幣整壞咗一啲標點符號字元，你嘅編輯已經拒絕咗。個編輯已經拒絕，以防止嗰版嘅文字損毀。
當你響度用緊一啲好多臭蟲，以網絡為主嘅匿名代理服務。'''",
'editing'                          => '而家喺度編輯$1',
'editingsection'                   => '而家喺度編輯$1 （小節）',
'editingcomment'                   => '而家喺度編輯$1 （評論）',
'editconflict'                     => '編輯衝突：$1',
'explainconflict'                  => "有其他人喺你開始編輯之後已經更改呢一頁。
喺上面嗰個空間而家現存嘅頁面文字。
你嘅更改會喺下面嘅文字空間顯示。
你需要合併你嘅更改到原有嘅文字。
喺你撳「儲存頁面」之後，'''只有'''喺上面嘅文字區會被儲存。",
'yourtext'                         => '你嘅文字',
'storedversion'                    => '已經儲存咗嘅版本',
'nonunicodebrowser'                => "'''警告：你嘅瀏覽器係唔係用緊 Unicode 。而家暫時有個解決方法，方便你可以安全咁編輯呢版：唔係 ASCII 嘅字元會喺編輯框裏面用十六進位編碼顯示。'''",
'editingold'                       => "'''警告：你而家係編輯緊喺呢一頁嘅過時版本。如果你儲存佢，喺呢個版本嘅任何更改都會被遺失。'''",
'yourdiff'                         => '差異',
'copyrightwarning'                 => "請留意喺{{SITENAME}}度，所有喺呢度嘅貢獻會被考慮到喺$2之下發出（睇$1有更詳細嘅資訊）。如果你係唔想你編輯嘅文字無喇喇咁被分發，咁就唔好喺呢度遞交。

你亦都要同我哋保證啲文字係你自己寫嘅，或者係由公有領域或相似嘅自由資源複製落嚟。
'''喺未有任何許可嘅情況之下千祈唔好遞交有版權嘅作品！'''",
'copyrightwarning2'                => "請留意喺{{SITENAME}}度，所有嘅貢獻可能會被其他嘅貢獻者編輯、修改，或者刪除。如果你係唔想你編輯嘅文字無喇喇咁被編輯，咁就唔好喺呢度遞交。

你亦都要同我哋保證啲文字係你自己寫嘅，或者係由公有領域或相似嘅自由資源複製落嚟（睇$1有更詳細嘅資訊）。
'''喺未有任何許可嘅情況之下千祈唔好遞交有版權嘅作品！'''",
'longpagewarning'                  => "'''警告：呢一頁有 $1 kilobytes 咁長；有啲瀏覽器可能會喺就離或者超過 32kb 編輯頁面會出現一啲問題。
請考慮分割呢個頁面到細啲嘅小節。'''",
'longpageerror'                    => "'''錯誤：你所遞交嘅文字係有 $1 kilobytes 咁長，係長過最大嘅 $2 kilobytes。儲唔到你遞交嘅文字。'''",
'readonlywarning'                  => "'''錯誤：料庫已經鎖住咗，以便定期保養。而家你唔可以儲起你嘅編輯。你可以儲啲文字落一份文字檔先。'''",
'protectedpagewarning'             => "'''警告：呢版已經受到保護，只有管理員權限嘅用戶先至可以改。'''",
'semiprotectedpagewarning'         => "'''注意：'''呢一頁已經鎖咗，只有已經註冊嘅用戶先至可以改。",
'cascadeprotectedwarning'          => "'''警告：'''呢一頁已經鎖咗，只有管理員權限嘅用戶先至可以改，因為佢係響以下連串保護嘅{{PLURAL:$1|一|幾}}頁度包含咗：",
'titleprotectedwarning'            => '<strong>警告：呢一版已經鎖咗，只有一啲用戶先至可以開到。</strong>',
'templatesused'                    => '喺呢一頁所用嘅模：',
'templatesusedpreview'             => '喺呢一次預覽所用嘅模：',
'templatesusedsection'             => '喺呢一小節所用嘅模：',
'template-protected'               => '(保護)',
'template-semiprotected'           => '(半保護)',
'hiddencategories'                 => '呢一版係屬於$1個隱藏類嘅成員:',
'edittools'                        => '<!-- 喺呢度嘅文字會喺編輯框下面同埋上載表格中顯示。 -->',
'nocreatetitle'                    => '頁面建立被限制',
'nocreatetext'                     => '{{SITENAME}}已經限制咗起新版嘅能力。
你可以番轉頭去編輯一啲已經存在嘅頁面，或者[[Special:UserLogin|登入或開個新戶口]]。',
'nocreate-loggedin'                => '你響{{SITENAME}}度並無許可權去開新版。',
'permissionserrors'                => '權限錯誤',
'permissionserrorstext'            => '根據下面嘅{{PLURAL:$1|原因|原因}}，你並無權限去做呢樣嘢:',
'permissionserrorstext-withaction' => '根據下面嘅{{PLURAL:$1|原因|原因}}，你並無權限去做$2:',
'recreate-deleted-warn'            => "'''警告: 你而家重開一版係先前曾經刪除過嘅。'''

你應該要考慮吓繼續編輯呢一版係唔係適合嘅。
為咗方便起見，呢一版嘅刪除記錄已經響下面提供:",

# Parser/template warnings
'expensive-parserfunction-warning'        => '警告: 呢一版有太多耗費嘅語法功能呼叫。

佢應該少過$2，但而家係$1。',
'expensive-parserfunction-category'       => '響版度有太多嘅耗費嘅語法功能呼叫',
'post-expand-template-inclusion-warning'  => '警告: 包含模大細太大。
有啲模將唔會包含。',
'post-expand-template-inclusion-category' => '模包含上限已超過嘅版',
'post-expand-template-argument-warning'   => '警告: 呢一版有最少一個模參數有太大嘅擴展大細。
呢啲參數會被略過。',
'post-expand-template-argument-category'  => '包含住略過模參數嘅版',

# "Undo" feature
'undo-success' => '呢個編輯可以取消。請檢查一下個差異去確認呢個係你要去做嘅，跟住儲存下面嘅更改去完成編輯。',
'undo-failure' => '呢個編輯唔能夠取消，由於同途中嘅編輯有衝突。',
'undo-norev'   => '呢個編輯唔能夠取消，由於佢唔存在或者刪除咗。',
'undo-summary' => '取消由[[Special:Contributions/$2|$2]] ([[User talk:$2|對話]])所做嘅修訂 $1',

# Account creation failure
'cantcreateaccounttitle' => '唔可以開新戶口',
'cantcreateaccount-text' => "由呢個IP地址 ('''$1''') 開嘅新戶口已經被[[User:$3|$3]]封鎖。

當中俾$3封鎖嘅原因係''$2''",

# History pages
'viewpagelogs'        => '睇呢頁嘅日誌',
'nohistory'           => '呢版冇歷史。',
'revnotfound'         => '搵唔到歷史',
'revnotfoundtext'     => '呢版無你要搵嗰個版本喎。
唔該睇下條網址啱唔啱。',
'currentrev'          => '家下嘅修訂',
'revisionasof'        => '喺$1嘅修訂',
'revision-info'       => '喺$1嘅修訂；修訂自$2',
'previousrevision'    => '←之前嘅修訂',
'nextrevision'        => '新啲嘅修訂→',
'currentrevisionlink' => '家下嘅修訂版本',
'cur'                 => '現時',
'next'                => '之後',
'last'                => '之前',
'page_first'          => '最頭',
'page_last'           => '最尾',
'histlegend'          => '選擇唔同版本：響兩個唔同版本嘅圓框分別撳一下，再撳最底的「比較被選版本」掣以作比較。<br />
說明：（現時）= 同現時修訂版本嘅差別，（先前）= 與前一個修訂版本嘅差別，M = 小修改。',
'deletedrev'          => '[刪除咗]',
'histfirst'           => '最早',
'histlast'            => '最近',
'historysize'         => '($1 {{PLURAL:$1|byte|bytes}})',
'historyempty'        => '(空)',

# Revision feed
'history-feed-title'          => '修訂歷史',
'history-feed-description'    => '響哩個wiki嘅哩一頁嘅修訂歷史',
'history-feed-item-nocomment' => '$1 響 $2', # user at time
'history-feed-empty'          => '要求嘅頁面並唔存在。
佢可能響哩個 wiki 度刪除咗或者改咗名。
試吓[[Special:Search|響哩個wiki度搵]]有關新頁面嘅資料。',

# Revision deletion
'rev-deleted-comment'         => '(評論已經移除咗)',
'rev-deleted-user'            => '(用戶名已經移除咗)',
'rev-deleted-event'           => '(日誌動作已經移除咗)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">
呢頁嘅修訂喺公共檔案庫中已經被洗咗。
喺[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌]裏面可能會有更詳細嘅資料。</div>',
'rev-deleted-text-view'       => '<div class="mw-warning plainlinks">
呢頁嘅修訂喺公共檔案庫中已經洗咗。
作為一個喺{{SITENAME}}嘅管理員，你可以去睇吓佢；
喺[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌]裏面可能會有更詳細嘅資料。</div>',
'rev-delundel'                => '顯示／隱藏',
'revisiondelete'              => '刪除／反刪除修訂',
'revdelete-nooldid-title'     => '無效嘅目標修訂',
'revdelete-nooldid-text'      => '你一係重未指定一個目標修訂去進行呢個功能、
所指定嘅修訂唔存在，或者你試緊去隱藏現時嘅修訂。',
'revdelete-selected'          => '揀[[:$1]]嘅$2次修訂：',
'logdelete-selected'          => '揀[[:$1]]嘅日誌事件：',
'revdelete-text'              => '刪除咗嘅修訂係會仍然出現喺個頁面歷史以及日誌度，但係佢哋嘅文字內容係唔可以供公眾瀏覽。

其他喺{{SITENAME}}嘅管理員仍然可以睇已經隱藏咗嘅內容，同埋可以透過同一個介面去反刪除佢，除非已經設定咗附加嘅限制。',
'revdelete-legend'            => '設定可見性嘅限制',
'revdelete-hide-text'         => '隱藏修訂嘅文字',
'revdelete-hide-name'         => '隱藏動作同目標',
'revdelete-hide-comment'      => '隱藏編輯註解',
'revdelete-hide-user'         => '隱藏編輯者嘅用戶名／IP',
'revdelete-hide-restricted'   => '應用呢嘅限制至操作員去鎖住呢個界面',
'revdelete-suppress'          => '同時壓制由操作員以及其他用戶的資料',
'revdelete-hide-image'        => '隱藏資料內容',
'revdelete-unsuppress'        => '響已經恢復咗嘅修訂度移除限制',
'revdelete-log'               => '記錄註解：',
'revdelete-submit'            => '應用到已經選取嘅修訂',
'revdelete-logentry'          => '已經更改[[$1]]嘅修訂可見性',
'logdelete-logentry'          => '已經更改[[$1]]嘅事件可見性',
'revdelete-success'           => "'''修訂可見性已經成功噉設定。'''",
'logdelete-success'           => "'''事件可見性已經成功噉設定。'''",
'revdel-restore'              => '改可見性',
'pagehist'                    => '頁面歷史',
'deletedhist'                 => '刪除咗嘅歷史',
'revdelete-content'           => '內容',
'revdelete-summary'           => '編輯摘要',
'revdelete-uname'             => '用戶名',
'revdelete-restricted'        => '已經應用限制到操作員',
'revdelete-unrestricted'      => '已經拎走對於操作員嘅限制',
'revdelete-hid'               => '隱藏 $1',
'revdelete-unhid'             => '唔隱藏 $1',
'revdelete-log-message'       => '$1嘅$2次修訂',
'logdelete-log-message'       => '$1嘅$2個事件',

# Suppression log
'suppressionlog'     => '廢止日誌',
'suppressionlogtext' => '下面係刪除同埋由操作員牽涉到內容封鎖嘅一覽。
睇吓[[Special:IPBlockList|IP封鎖一覽]]去睇現時進行緊嘅禁止同埋封鎖表。',

# History merging
'mergehistory'                     => '合併頁歷史',
'mergehistory-header'              => '呢一版可以畀你去合併一個來源頁嘅修訂記錄到另一個新頁。
請確認呢次更改會繼續保留嗰版之前嘅歷史版。',
'mergehistory-box'                 => '合併兩版嘅修訂:',
'mergehistory-from'                => '來源頁:',
'mergehistory-into'                => '目的頁:',
'mergehistory-list'                => '可以合併嘅編輯記錄',
'mergehistory-merge'               => '下面[[:$1]]嘅修訂可以合併到[[:$2]]。用個選項掣欄去合併只有響指定時間之前所開嘅修訂。要留意嘅係用個導航連結就會重設呢一欄。',
'mergehistory-go'                  => '顯示可以合併嘅編輯',
'mergehistory-submit'              => '合併修訂',
'mergehistory-empty'               => '無修訂可以合併',
'mergehistory-success'             => '[[:$1]]嘅$3次修訂已經成功噉合併到[[:$2]]。',
'mergehistory-fail'                => '唔可以進行歷史合併，請重新檢查嗰一版同埋時間參數。',
'mergehistory-no-source'           => '來源頁$1唔存在。',
'mergehistory-no-destination'      => '目的頁$1唔存在。',
'mergehistory-invalid-source'      => '來源頁一定要係一個有效嘅標題。',
'mergehistory-invalid-destination' => '目的頁一定要係一個有效嘅標題。',
'mergehistory-autocomment'         => '已經合併咗[[:$1]]去到[[:$2]]',
'mergehistory-comment'             => '已經合併咗[[:$1]]去到[[:$2]]: $3',

# Merge log
'mergelog'           => '合併日誌',
'pagemerge-logentry' => '合併咗[[$1]]去到[[$2]] (修訂截到$3)',
'revertmerge'        => '反合併',
'mergelogpagetext'   => '下面係一個最近由一版嘅修訂記錄合併到另一個嘅一覽。',

# Diffs
'history-title'           => '"$1"嘅修訂歷史',
'difference'              => '（修訂之間嘅差異）',
'lineno'                  => '第$1行：',
'compareselectedversions' => '比較被選嘅版本',
'editundo'                => '復原',
'diff-multi'              => '(當中有$1次嘅修訂唔會顯示。)',

# Search results
'searchresults'             => '搵嘢結果',
'searchresulttext'          => '有關搵{{SITENAME}}嘅更多資料請參考[[{{MediaWiki:Helppage}}|{{int:help}}]]。',
'searchsubtitle'            => '你利用\'\'\'[[:$1]]\'\'\'搵  ([[Special:Prefixindex/$1|全部由 "$1" 開始嘅頁]] | [[Special:WhatLinksHere/$1|全部連去 "$1" 嘅頁]])',
'searchsubtitleinvalid'     => "你利用'''$1'''搵",
'noexactmatch'              => "'''搵唔到到標題為\"\$1\"嘅頁面。''' 你可以而家[[:\$1|開呢個新頁]]。",
'noexactmatch-nocreate'     => "'''搵唔到到標題為\"\$1\"嘅頁面。'''",
'toomanymatches'            => '太多嘅配合搵到，請試吓一個唔同嘅查詢',
'titlematches'              => '頁面標題符合',
'notitlematches'            => '冇頁面嘅標題符合',
'textmatches'               => '頁面文字符合',
'notextmatches'             => '冇頁面文字符合',
'prevn'                     => '前$1',
'nextn'                     => '後$1',
'viewprevnext'              => '去睇 ($1) ($2) ($3)',
'search-result-size'        => '$1 ($2個字)',
'search-result-score'       => '相關度: $1%',
'search-redirect'           => '(跳轉 $1)',
'search-section'            => '(小節 $1)',
'search-suggest'            => '你係唔係搵: $1',
'search-interwiki-caption'  => '姊妹計劃',
'search-interwiki-default'  => '$1項結果:',
'search-interwiki-more'     => '(更多)',
'search-mwsuggest-enabled'  => '有建議',
'search-mwsuggest-disabled' => '無建議',
'search-relatedarticle'     => '有關',
'mwsuggest-disable'         => '停用AJAX建議',
'searchrelated'             => '有關',
'searchall'                 => '全部',
'showingresults'            => "'自#'''$2'''起顯示最多'''$1'''個結果。",
'showingresultsnum'         => "自#'''$2'''起顯示'''$3'''個結果。",
'showingresultstotal'       => "下面顯示緊由'''$1 - $2'''，總共'''$3'''項嘅結果",
'nonefound'                 => "'''注意''': 只有一啲空間名係會作預設搵嘢。試吓''all:''去搵全部嘅嘢（包埋討論版、模等），或用需要嘅空間名做前綴。",
'powersearch'               => '進階搵嘢',
'powersearch-legend'        => '進階搵嘢',
'powersearch-ns'            => '喺以下嘅空間名度搵:',
'powersearch-redir'         => '彈去清單',
'powersearch-field'         => '搵',
'search-external'           => '出面搵嘢',
'searchdisabled'            => '{{SITENAME}}嘅搜尋功能已經關閉。你可以利用Google嚟搵。不過佢哋對{{SITENAME}}嘅索引可能唔係最新嘅。',

# Preferences page
'preferences'              => '喜好設定',
'mypreferences'            => '我嘅喜好設定',
'prefs-edits'              => '編輯數:',
'prefsnologin'             => '重未登入',
'prefsnologintext'         => '你一定要去<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} 登入]</span>設定好用戶喜好值先。',
'prefsreset'               => '喜好設定已經從儲存空間中重設。',
'qbsettings'               => '快捷列',
'qbsettings-none'          => '無',
'qbsettings-fixedleft'     => '左邊固定',
'qbsettings-fixedright'    => '右邊固定',
'qbsettings-floatingleft'  => '左邊浮動',
'qbsettings-floatingright' => '右邊浮動',
'changepassword'           => '改密碼',
'skin'                     => '畫面',
'math'                     => '數',
'dateformat'               => '日期格式',
'datedefault'              => '冇喜好',
'datetime'                 => '日期同埋時間',
'math_failure'             => '語法拼砌失敗',
'math_unknown_error'       => '唔知錯乜',
'math_unknown_function'    => '唔知乜函數',
'math_lexing_error'        => 'lexing錯誤',
'math_syntax_error'        => '語法錯誤',
'math_image_error'         => 'PNG 轉換失敗；檢查latex、dvips、gs同埋convert係唔係已經正確咁樣安裝',
'math_bad_tmpdir'          => '唔能夠寫入或建立臨時數目錄',
'math_bad_output'          => '唔能夠寫入或建立輸出數目錄',
'math_notexvc'             => 'texvc 執行檔已經遺失；請睇睇 math/README 去較吓。',
'prefs-personal'           => '用戶簡介',
'prefs-rc'                 => '最近更改',
'prefs-watchlist'          => '監視清單',
'prefs-watchlist-days'     => '監視清單嘅顯示日數：',
'prefs-watchlist-edits'    => '喺加強版監視清單度嘅最多顯示更改數：',
'prefs-misc'               => '雜項',
'saveprefs'                => '儲存',
'resetprefs'               => '重設',
'oldpassword'              => '舊密碼：',
'newpassword'              => '新密碼：',
'retypenew'                => '打多次新密碼：',
'textboxsize'              => '編輯中',
'rows'                     => '列：',
'columns'                  => '行：',
'searchresultshead'        => '搵嘢',
'resultsperpage'           => '每頁顯示嘅擊中數：',
'contextlines'             => '每一擊顯示嘅行數：',
'contextchars'             => '每一行嘅字數：',
'stub-threshold'           => '<a href="#" class="stub">楔位連結</a>格式門檻 (bytes):',
'recentchangesdays'        => '最近更改中嘅顯示日數：',
'recentchangescount'       => '最近更改、編輯紀錄同日誌頁中嘅編輯數：',
'savedprefs'               => '你嘅喜好設定已經儲存。',
'timezonelegend'           => '時區',
'timezonetext'             => '¹你嘅本地時間同伺服器時間 (UTC) 之間嘅差，以鐘頭為單位。',
'localtime'                => '本地時間',
'timezoneoffset'           => '時間偏移¹',
'servertime'               => '伺機器時間',
'guesstimezone'            => '由瀏覽器填上',
'allowemail'               => '由其它用戶啟用電子郵件',
'prefs-searchoptions'      => '搵嘢選項',
'prefs-namespaces'         => '空間名',
'defaultns'                => '預設喺呢啲空間名搵嘢：',
'default'                  => '預設',
'files'                    => '檔案',

# User rights
'userrights'                  => '用戶權限管理', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => '管理用戶組',
'userrights-user-editname'    => '輸入一個用戶名：',
'editusergroup'               => '編輯用戶組',
'editinguser'                 => "改緊用戶'''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) 嘅用戶權限",
'userrights-editusergroup'    => '編輯用戶組',
'saveusergroups'              => '儲存用戶組',
'userrights-groupsmember'     => '屬於：',
'userrights-groups-help'      => '你可以改呢位用戶所屬嘅組:
* 剔咗嘅盒代表個用戶係屬於嗰組。
* 未剔嘅盒代表個用戶唔係屬於嗰組。
* 一個 * 表示你加入咗佢之後唔可以拎走，反之亦然。',
'userrights-reason'           => '更改嘅原因:',
'userrights-no-interwiki'     => '你並無權限去編輯響其它wiki嘅用戶權限。',
'userrights-nodatabase'       => '資料庫$1唔存在或者唔係本地嘅。',
'userrights-nologin'          => '你一定要以操作員戶口[[Special:UserLogin|登入]]咗之後先可以指定用戶權限。',
'userrights-notallowed'       => '你嘅戶口無權限去指定用戶權限。',
'userrights-changeable-col'   => '你可以改嘅組',
'userrights-unchangeable-col' => '你唔可以改嘅組',

# Groups
'group'               => '組：',
'group-user'          => '用戶',
'group-autoconfirmed' => '自動確認用戶',
'group-bot'           => '機械人',
'group-sysop'         => '操作員',
'group-bureaucrat'    => '事務員',
'group-suppress'      => '監督',
'group-all'           => '(全部)',

'group-user-member'          => '用戶',
'group-autoconfirmed-member' => '自動確認用戶',
'group-bot-member'           => '機械人',
'group-sysop-member'         => '操作員',
'group-bureaucrat-member'    => '事務員',
'group-suppress-member'      => '監督',

'grouppage-user'          => '{{ns:project}}:用戶',
'grouppage-autoconfirmed' => '{{ns:project}}:自動確認用戶',
'grouppage-bot'           => '{{ns:project}}:機械人',
'grouppage-sysop'         => '{{ns:project}}:管理員',
'grouppage-bureaucrat'    => '{{ns:project}}:事務員',
'grouppage-suppress'      => '{{ns:project}}:監督',

# Rights
'right-read'                 => '讀版',
'right-edit'                 => '編輯版',
'right-createpage'           => '開版（唔包討論版）',
'right-createtalk'           => '開討論版',
'right-createaccount'        => '開新用戶戶口',
'right-minoredit'            => '標示小編輯',
'right-move'                 => '搬版',
'right-move-subpages'        => '搬版同埋佢哋嘅細版',
'right-suppressredirect'     => '搬版嗰陣唔開跳轉',
'right-upload'               => '上載檔案',
'right-reupload'             => '覆蓋現有嘅檔案',
'right-reupload-own'         => '覆蓋由同一位上載嘅檔案',
'right-reupload-shared'      => '於本地無視共用媒體檔案庫上嘅檔案',
'right-upload_by_url'        => '由URL地址上載一個檔案',
'right-purge'                => '唔需要確認之下清除網站快取',
'right-autoconfirmed'        => '編輯半保護版',
'right-bot'                  => '視為一個自動程序',
'right-nominornewtalk'       => '小編輯唔引發新信息提示',
'right-apihighlimits'        => '響API查詢度用更高嘅上限',
'right-writeapi'             => '使用編寫嘅API',
'right-delete'               => '刪版',
'right-bigdelete'            => '刪大量歷史嘅版',
'right-deleterevision'       => '刪同反刪版嘅指定修訂',
'right-deletedhistory'       => '睇刪咗嘅項目，唔包同埋嘅字',
'right-browsearchive'        => '搵刪咗嘅版',
'right-undelete'             => '反刪版',
'right-suppressrevision'     => '睇同恢復由操作員隱藏嘅修訂',
'right-suppressionlog'       => '去睇私人嘅日誌',
'right-block'                => '封鎖其他用戶唔畀編輯',
'right-blockemail'           => '封鎖用戶唔畀寄電郵',
'right-hideuser'             => '封鎖用戶名，對公眾隱藏',
'right-ipblock-exempt'       => '繞過IP封鎖、自動封鎖同埋範圍封鎖',
'right-proxyunbannable'      => '繞過Proxy嘅自動封鎖',
'right-protect'              => '改保護等級同埋編輯保護版',
'right-editprotected'        => '編輯保護版（無連串保護）',
'right-editinterface'        => '編輯用戶界面',
'right-editusercssjs'        => '編輯其他用戶嘅CSS同埋JS檔',
'right-rollback'             => '快速反轉上位用戶對某一版嘅編輯',
'right-markbotedits'         => '標示反轉編輯做機械人編輯',
'right-noratelimit'          => '唔受利用率限制影響',
'right-import'               => '由其它wiki度倒入版',
'right-importupload'         => '由檔案上載度倒入版',
'right-patrol'               => '標示其它嘅編輯做已巡查嘅',
'right-autopatrol'           => '將自己嘅編輯自動標示做已巡查嘅',
'right-patrolmarks'          => '去睇最近巡查標記更改',
'right-unwatchedpages'       => '去睇未監視嘅版',
'right-trackback'            => '遞交一個trackback',
'right-mergehistory'         => '合併版歷史',
'right-userrights'           => '編輯全部用戶嘅權限',
'right-userrights-interwiki' => '編輯響其它wiki嘅用戶權限',
'right-siteadmin'            => '鎖同解鎖資料庫',

# User rights log
'rightslog'      => '用戶權限日誌',
'rightslogtext'  => '呢個係用戶權力嘅修改日誌。',
'rightslogentry' => '已經將$1嘅組別從$2改到去$3',
'rightsnone'     => '(無)',

# Recent changes
'nchanges'                          => '$1次更改',
'recentchanges'                     => '最近更改',
'recentchangestext'                 => '追蹤對哩一個 wiki 嘅最後更改。',
'recentchanges-feed-description'    => '追蹤對哩一個 wiki 度呢個集合嘅最後更改。',
'rcnote'                            => "以下係響$4 $5，近'''$2'''日嘅最後'''$1'''次修改。",
'rcnotefrom'                        => "以下係自'''$2'''嘅更改（顯示到'''$1'''）。",
'rclistfrom'                        => '顯示由$1嘅新更改',
'rcshowhideminor'                   => '$1小編輯',
'rcshowhidebots'                    => '$1機械人',
'rcshowhideliu'                     => '$1登入咗嘅用戶',
'rcshowhideanons'                   => '$1匿名用戶',
'rcshowhidepatr'                    => '$1巡邏過嘅編輯',
'rcshowhidemine'                    => '$1我嘅編輯',
'rclinks'                           => '顯示最後$1次喺$2日內嘅更改<br />$3',
'diff'                              => '差異',
'hist'                              => '歷史',
'hide'                              => '隱藏',
'show'                              => '顯示',
'minoreditletter'                   => 'm',
'newpageletter'                     => 'N',
'boteditletter'                     => 'b',
'number_of_watching_users_pageview' => '[$1位用戶監視]',
'rc_categories'                     => '限定到分類（以"|"作分隔）',
'rc_categories_any'                 => '任何',
'newsectionsummary'                 => '/* $1 */ 新小節',

# Recent changes linked
'recentchangeslinked'          => '連結頁嘅更改',
'recentchangeslinked-title'    => '對「$1」有關嘅更改',
'recentchangeslinked-noresult' => '響呢一段時間內連結頁並無更改。',
'recentchangeslinked-summary'  => "呢一個特別頁列示咗''由''所畀到嘅一版連結到頁嘅最近更改（或者係指定分類嘅成員）。
響[[Special:Watchlist|你張監視清單]]嘅版會以'''粗體'''顯示。",
'recentchangeslinked-page'     => '頁名:',
'recentchangeslinked-to'       => '顯示連到所畀到嘅版',

# Upload
'upload'                      => '上載檔案',
'uploadbtn'                   => '上載檔案',
'reupload'                    => '再上載',
'reuploaddesc'                => '取消上載再返到去上載表格',
'uploadnologin'               => '重未登入',
'uploadnologintext'           => '你必須先[[Special:UserLogin|登入]]去上載檔案。',
'upload_directory_missing'    => '嗰個上載嘅目錄 ($1) 唔見咗，唔可以由網頁伺服器建立。',
'upload_directory_read_only'  => '嗰個上載嘅目錄 ($1) 而家唔能夠被網頁伺服器寫入。',
'uploaderror'                 => '上載錯誤',
'uploadtext'                  => "用下面嘅表格嚟上載檔案。
要睇或者搵嘢之前上載嘅圖像請去[[Special:ImageList|已上載檔案一覽]]，（再）上載嘅動作會喺[[Special:Log/upload|上載日誌]]裏面記錄落嚟，而刪除嘅動作會喺[[Special:Log/delete|刪除日誌]]裏面記錄落嚟。

如果要喺頁面度引入呢張圖像，可以使用以下其中一種方式嘅連結：
* '''<tt><nowiki>[[</nowiki>{{ns:image}}:file.jpg<nowiki>]]</nowiki></tt>'''去用檔案嘅完整版
* '''<tt><nowiki>[[</nowiki>{{ns:image}}:file.png|200px|thumb|left|替代文字<nowiki>]]</nowiki></tt>'''去用200像素比例闊，靠左邊加盒，響描述度加'替代文字'
* '''<tt><nowiki>[[</nowiki>{{ns:media}}:file.ogg<nowiki>]]</nowiki></tt>''' 直接連結到檔案而唔顯示個檔案。",
'upload-permitted'            => '准許嘅檔案類型: $1。',
'upload-preferred'            => '建議嘅檔案類型: $1。',
'upload-prohibited'           => '禁止嘅檔案類型: $1。',
'uploadlog'                   => '上載日誌',
'uploadlogpage'               => '上載日誌',
'uploadlogpagetext'           => '以下係最近檔案上載嘅一覽表。
睇[[Special:NewImages|新圖像畫廊]]去睇更詳細嘅總覽。',
'filename'                    => '檔名',
'filedesc'                    => '摘要',
'fileuploadsummary'           => '摘要：',
'filestatus'                  => '版權狀態:',
'filesource'                  => '來源:',
'uploadedfiles'               => '上載檔案中',
'ignorewarning'               => '總要忽略警告同埋儲存檔案',
'ignorewarnings'              => '忽略任何警告',
'minlength1'                  => '檔名必須最少要有一個字。',
'illegalfilename'             => '檔名「$1」含有頁面標題所唔允許嘅字。請試下改檔名再上載。',
'badfilename'                 => '檔名已經更改成「$1」。',
'filetype-badmime'            => '「$1」嘅MIME類型檔案係唔容許上載嘅。',
'filetype-unwanted-type'      => "'''\".\$1\"'''係一種唔需要嘅檔案類型。
建議嘅{{PLURAL:\$3|一種|多種}}檔案類型有\$2。",
'filetype-banned-type'        => "'''\".\$1\"'''係一種唔准許嘅檔案類型。
容許嘅{{PLURAL:\$3|一種|多種}}檔案類型有\$2。",
'filetype-missing'            => '個檔名並冇副檔名（好以「.jpg」）。',
'large-file'                  => '建議檔案嘅大細唔好大過$1 bytes，呢個檔案有$2 bytes',
'largefileserver'             => '呢個檔案超過咗伺服器設定允許嘅大細。',
'emptyfile'                   => '你上載嘅檔案似乎係空嘅。噉樣可能係因為你打錯咗個檔名。請檢查吓你係唔係真係要上載呢個檔案。',
'fileexists'                  => '呢個檔名已經存在，如果你唔肯定係唔係要更改<strong><tt>$1</tt></strong>，請先檢查佢。',
'filepageexists'              => "呢個檔嘅描述頁已經響'''<tt>$1</tt></strong>開咗，但係呢個名嘅檔案重未存在。你輸入咗嘅摘要係唔會顯示響個描述頁度。要令到個摘要響嗰度出現，你就要手動噉去編輯佢。'''",
'fileexists-extension'        => "一個相似檔名嘅檔案已經存在:

上載檔案嘅檔名: '''<tt>$1</tt>'''

現有檔案嘅檔名: '''<tt>$2</tt>'''

請揀一個唔同嘅名。",
'fileexists-thumb'            => "<center>'''已經存在嘅檔案'''</center>",
'fileexists-thumbnail-yes'    => "呢個檔案好似係一幅圖像縮細咗嘅版本''（縮圖）''。請檢查清楚個檔案'''<tt>$1</tt>'''。

如果檢查咗嘅檔案係同原本幅圖個大細係一樣嘅話，就唔使再上載多一幅縮圖。",
'file-thumbnail-no'           => "個檔名係以'''<tt>$1</tt>'''開始。佢好似係一幅圖像嘅縮細版本''（縮圖）''。
如果你有呢幅圖像嘅完整大細，唔係嘅話請再改過個檔名。",
'fileexists-forbidden'        => '呢個檔案嘅名已經存在；麻煩返轉去用第二個名嚟上載呢個檔案。[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => '共享檔案庫入面已經有一個同名嘅檔案；麻煩返轉去用第二個名嚟上載呢個檔案。[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => '呢個檔案係同下面嘅{{PLURAL:$1|一|幾}}個檔案重覆:',
'successfulupload'            => '成功嘅上載',
'uploadwarning'               => '上載警告',
'savefile'                    => '儲存檔案',
'uploadedimage'               => '上載咗"[[$1]]"',
'overwroteimage'              => '已經上載咗"[[$1]]"嘅新版本',
'uploaddisabled'              => '上載已停用',
'uploaddisabledtext'          => '{{SITENAME}}嘅檔案上載已經停用。',
'uploadscripted'              => '呢個檔案包含可能會誤被瀏覽器解釋執行嘅 HTML 或 script 代碼。',
'uploadcorrupt'               => '呢個檔案已損壞或係用咗錯誤嘅副檔名。請檢查吓個檔案，然後再試下上載多次。',
'uploadvirus'                 => '呢個檔案有病毒！詳情：$1',
'sourcefilename'              => '來源檔名:',
'destfilename'                => '目標檔名:',
'upload-maxfilesize'          => '檔案最大限制大細: $1',
'watchthisupload'             => '監視呢頁',
'filewasdeleted'              => '呢個檔案所使用嘅名曾經上載後，跟住就刪除咗。你應該響重新上載佢之前檢查吓$1。',
'upload-wasdeleted'           => "'''警告: 你而家上載嘅一個檔案係先前曾經刪除過嘅。'''

你應該要考慮吓繼續上載呢個檔案係唔係適合嘅。
為咗方便起見，呢個檔案嘅刪除記錄已經響下面提供:",
'filename-bad-prefix'         => "你上載嘅檔名係以'''\"\$1\"'''做開頭，通常呢種無含意嘅檔名係響數碼相機度自動編排。請響你個檔案度揀過一個更加有意義嘅檔名。",

'upload-proto-error'      => '唔正確嘅協議',
'upload-proto-error-text' => '遙遠上載需要一個以 <code>http://</code> 或者 <code>ftp://</code> 作為開頭嘅URL。',
'upload-file-error'       => '內部錯誤',
'upload-file-error-text'  => '當響伺服器度建立一個暫存檔時發生咗一個內部錯誤。請聯絡一位[[Special:ListUsers/sysop|管理員]]。',
'upload-misc-error'       => '未知嘅上載錯誤',
'upload-misc-error-text'  => '響上載時發生咗未知嘅錯誤。請確認輸入咗嘅URL係可以訪問嘅，之後再試多一次。如果重有問題嘅話，請聯絡一位[[Special:ListUsers/sysop|管理員]]。',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => '唔可以到嗰個URL',
'upload-curl-error6-text'  => '輸入嘅URL唔能夠去到。請重新檢查個URL係正確嘅同埋個網站係已經上綫。',
'upload-curl-error28'      => '上載遇時',
'upload-curl-error28-text' => '個網站用咗太多時間回應。請檢查個網站已經係上咗綫，等多一陣然後再試過。你可以響冇咁繁忙嘅時間再試。',

'license'            => '協議:',
'nolicense'          => '未揀',
'license-nopreview'  => '（無預覽可以用得到）',
'upload_source_url'  => ' （啱嘅，公開嘅網址）',
'upload_source_file' => ' （你部電腦裏面嘅一個檔案）',

# Special:ImageList
'imagelist-summary'     => '呢個特別版顯示全部上載過嘅檔案。
響預設最後上載嘅檔案會顯示響呢個表嘅最頂。
撳一欄嘅標題去改個排列。',
'imagelist_search_for'  => '搵媒體名:',
'imgfile'               => '檔案',
'imagelist'             => '檔案清單',
'imagelist_date'        => '日期',
'imagelist_name'        => '名',
'imagelist_user'        => '用戶',
'imagelist_size'        => '大細',
'imagelist_description' => '描述',

# Image description page
'filehist'                       => '檔案歷史',
'filehist-help'                  => '撳個日期／時間去睇響嗰個時間出現過嘅檔案。',
'filehist-deleteall'             => '刪除全部',
'filehist-deleteone'             => '刪除',
'filehist-revert'                => '回復',
'filehist-current'               => '現時',
'filehist-datetime'              => '日期／時間',
'filehist-user'                  => '用戶',
'filehist-dimensions'            => '尺寸',
'filehist-filesize'              => '檔案大細',
'filehist-comment'               => '註解',
'imagelinks'                     => '連結',
'linkstoimage'                   => '以下嘅$1個頁面連結到呢個檔案：',
'nolinkstoimage'                 => '冇個頁面連結到呢個檔案。',
'morelinkstoimage'               => '去睇連到呢個檔案嘅[[Special:WhatLinksHere/$1|更多連結]]。',
'redirectstofile'                => '下面嘅$1個檔案跳轉到呢個檔案:',
'duplicatesoffile'               => '下面嘅$1個檔案係同呢個檔案重覆:',
'sharedupload'                   => '呢個檔案係共用嘅上載，可以喺其他計劃中使用。“',
'shareduploadwiki'               => '更多資訊請睇$1。',
'shareduploadwiki-desc'          => '佢響共用檔案庫上面嘅$1描述響下面度顯示。',
'shareduploadwiki-linktext'      => '檔案描述頁面',
'shareduploadduplicate'          => '呢個檔案係同響個共用檔案庫嘅$1重覆。',
'shareduploadduplicate-linktext' => '另一個檔案',
'shareduploadconflict'           => '呢個檔案響共用檔案庫度嘅$1有同樣嘅檔名。',
'shareduploadconflict-linktext'  => '另一個檔案',
'noimage'                        => '冇同名嘅檔案存在，你可以$1。',
'noimage-linktext'               => '上載佢',
'uploadnewversion-linktext'      => '上載呢個檔案嘅一個新版本',
'imagepage-searchdupe'           => '搵重覆檔案',

# File reversion
'filerevert'                => '回復$1',
'filerevert-legend'         => '回復檔案',
'filerevert-intro'          => "你而家回復緊'''[[Media:$1|$1]]'''到[$4 響$2 $3嘅版本]。",
'filerevert-comment'        => '註解:',
'filerevert-defaultcomment' => '已經回復到響$1 $2嘅版本',
'filerevert-submit'         => '回復',
'filerevert-success'        => "'''[[Media:$1|$1]]'''已經回復到[$4 響$2 $3嘅版本]。",
'filerevert-badversion'     => '呢個檔案所提供嘅時間截記並無之前嘅本地版本。',

# File deletion
'filedelete'                  => '刪除$1',
'filedelete-legend'           => '刪除檔案',
'filedelete-intro'            => "你而家刪除緊'''[[Media:$1|$1]]'''。",
'filedelete-intro-old'        => "你而家刪除緊'''[[Media:$1|$1]]'''響[$4 $2 $3]嘅版本。",
'filedelete-comment'          => '刪除原因:',
'filedelete-submit'           => '刪除',
'filedelete-success'          => "'''$1'''已經刪除咗。",
'filedelete-success-old'      => "'''[[Media:$1|$1]]'''響 $2 $3 嘅版本已經刪除咗。",
'filedelete-nofile'           => "'''$1'''響{{SITENAME}}度唔存在。",
'filedelete-nofile-old'       => "用指定嘅屬性，呢度係無'''$1'''響 $2 $3 嘅歸檔版本。",
'filedelete-iscurrent'        => '你而家嘗試刪除呢個檔案嘅最近版本。請先將個檔打回頭做舊啲嘅版本先。',
'filedelete-otherreason'      => '其它／附加嘅原因:',
'filedelete-reason-otherlist' => '其它原因',
'filedelete-reason-dropdown'  => '
*常用刪除原因
** 侵犯版權
** 重覆檔案',
'filedelete-edit-reasonlist'  => '編輯刪除原因',

# MIME search
'mimesearch'         => 'MIME 搜尋',
'mimesearch-summary' => '呢一版可以過濾有關檔案嘅MIME類型。輸入方法：contenttype/subtype，例如 <tt>image/jpeg</tt>。',
'mimetype'           => 'MIME 類型：',
'download'           => '下載',

# Unwatched pages
'unwatchedpages' => '未監視嘅頁面',

# List redirects
'listredirects' => '彈嚟彈去一覽',

# Unused templates
'unusedtemplates'     => '未用嘅模',
'unusedtemplatestext' => '呢一頁列示喺template空間名未包括喺其它頁面嘅全部頁面。請記得喺刪除佢哋之前檢查其它連結到呢個模嘅頁面。',
'unusedtemplateswlh'  => '其它連結',

# Random page
'randompage'         => '隨便一版',
'randompage-nopages' => '響呢個空間名度搵唔到一版。',

# Random redirect
'randomredirect'         => '隨便彈',
'randomredirect-nopages' => '響呢個空間名度冇一個彈去版。',

# Statistics
'statistics'             => '統計',
'sitestats'              => '{{SITENAME}}嘅統計',
'userstats'              => '用戶統計',
'sitestatstext'          => "資料庫中而家有'''$1'''頁。
其中包括咗「討論」頁、關於{{SITENAME}}嘅頁、好短嘅「楔位」版頁、跳轉，以及其他唔計入內容嘅頁。唔計非內容頁在內，則總共有'''$2'''頁可能會計入正規嘅內容。

'''$8''' 個檔案已經上載。

呢個Wiki喺建立以嚟，總共有'''$3'''次瀏覽，同埋'''$4'''次編輯。
平均每個頁面有'''$5'''次瀏覽，同埋'''$6'''次編輯。

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue]嘅長度係'''$7'''。",
'userstatstext'          => "目前有'''$1'''個註冊[[Special:ListUsers|用戶]]，其中有'''$2'''人（即'''$4%'''）有$5權限。",
'statistics-mostpopular' => '最多人睇嘅頁',

'disambiguations'      => '搞清楚頁',
'disambiguationspage'  => 'Template:disambig
Template:搞清楚',
'disambiguations-text' => "以下呢啲頁面連結去一個'''搞清楚頁'''。佢哋先至應該指去正確嘅主題。<br />如果一個頁面連結自[[MediaWiki:Disambiguationspage]]，噉就會當佢係搞清楚頁。",

'doubleredirects'            => '雙重跳轉',
'doubleredirectstext'        => '呢一版列出咗全部跳轉頁跳轉到另一版跳轉頁嘅版。每一行包括指去第一個同第二個跳轉嘅連結，以及第二個跳轉嘅目標。呢行文字通常畀出咗第一個跳轉應該指去嘅嗰個「真正」嘅目標頁面。',
'double-redirect-fixed-move' => '[[$1]]已經搬好咗，佢而家跳轉過去[[$2]]',
'double-redirect-fixer'      => '跳轉修正器',

'brokenredirects'        => '破碎嘅跳轉',
'brokenredirectstext'    => '以下嘅跳轉係指向唔存在嘅頁面。',
'brokenredirects-edit'   => '(編輯)',
'brokenredirects-delete' => '(刪除)',

'withoutinterwiki'         => '無語言連結嘅頁',
'withoutinterwiki-summary' => '以下嘅頁面係重未有連結到其它嘅語言版本。',
'withoutinterwiki-legend'  => '前綴',
'withoutinterwiki-submit'  => '顯示',

'fewestrevisions' => '有最少修改嘅版',

# Miscellaneous special pages
'nbytes'                  => '$1 {{PLURAL:$1|byte|bytes}}',
'ncategories'             => '$1 個分類',
'nlinks'                  => '$1 條連結',
'nmembers'                => '$1 位成員',
'nrevisions'              => '$1 次修訂',
'nviews'                  => '$1 次瀏覽',
'specialpage-empty'       => '呢個報告嘅結果係空嘅。',
'lonelypages'             => '孤立咗嘅頁面',
'lonelypagestext'         => '以下嘅面頁係響{{SITENAME}}度未有連結到其它頁面。',
'uncategorizedpages'      => '未有分類嘅頁面',
'uncategorizedcategories' => '未有分類嘅分類',
'uncategorizedimages'     => '未有分類嘅檔案',
'uncategorizedtemplates'  => '未有分類嘅模',
'unusedcategories'        => '未用嘅分類',
'unusedimages'            => '未用嘅檔案',
'popularpages'            => '受歡迎嘅頁面',
'wantedcategories'        => '被徵求嘅分類',
'wantedpages'             => '被徵求嘅頁面',
'missingfiles'            => '遺失咗嘅檔案',
'mostlinked'              => '有最多連結嘅頁面',
'mostlinkedcategories'    => '有最多連結嘅分類',
'mostlinkedtemplates'     => '有最多連結嘅模',
'mostcategories'          => '有最多分類嘅頁面',
'mostimages'              => '有最多連結嘅檔案',
'mostrevisions'           => '有最多修改嘅頁面',
'prefixindex'             => '前綴索引',
'shortpages'              => '短頁',
'longpages'               => '長頁',
'deadendpages'            => '掘頭頁',
'deadendpagestext'        => '呢啲頁無連到{{SITENAME}}內嘅任何一頁。',
'protectedpages'          => '保護頁',
'protectedpages-indef'    => '只有無期保謢頁',
'protectedpagestext'      => '以下嘅頁面係受保頁面，唔能夠移動或編輯',
'protectedpagesempty'     => '響呢啲參數度，現時無頁面響度保護緊。',
'protectedtitles'         => '保護咗嘅標題',
'protectedtitlestext'     => '下面係一個保護咗唔䦒得嘅標題',
'protectedtitlesempty'    => '響呢啲參數之下並無標題保護住。',
'listusers'               => '用戶一覽',
'newpages'                => '新頁',
'newpages-username'       => '用戶名：',
'ancientpages'            => '舊頁面',
'move'                    => '移動',
'movethispage'            => '移動呢一頁',
'unusedimagestext'        => '請注意，第啲網站會用直接用URL連結到一個檔，所以呢度可能有啲用緊嘅檔。',
'unusedcategoriestext'    => '呢啲類存在，但入面冇嘢亦都冇分類。',
'notargettitle'           => '冇目標',
'notargettext'            => '你冇指定到呢個功能要用喺嘅對象頁面或用戶。',
'nopagetitle'             => '冇目標頁',
'nopagetext'              => '你所指定嘅目標頁並唔存在。',
'pager-newer-n'           => '新$1次',
'pager-older-n'           => '舊$1次',
'suppress'                => '監督',

# Book sources
'booksources'               => '書籍來源',
'booksources-search-legend' => '搵書源',
'booksources-go'            => '去',
'booksources-text'          => '以下嘅連結清單列出其它一啲賣新書同二手書嘅網站，可能可以提供到有關你想搵嘅書嘅更多資料：',

# Special:Log
'specialloguserlabel'  => '用戶:',
'speciallogtitlelabel' => '標題:',
'log'                  => '日誌',
'all-logs-page'        => '全部嘅日誌',
'log-search-legend'    => '搵日誌',
'log-search-submit'    => '去',
'alllogstext'          => '響{{SITENAME}}度全部日誌嘅綜合顯示。你可以選擇一個日誌類型、用戶名、或者受影響嘅頁面，嚟縮窄顯示嘅範圍。',
'logempty'             => '日誌中冇符合嘅項目。',
'log-title-wildcard'   => '搵以呢個文字開始嘅標題',

# Special:AllPages
'allpages'          => '所有頁面',
'alphaindexline'    => '$1到$2',
'nextpage'          => '下一頁 ($1)',
'prevpage'          => '上一頁 ($1)',
'allpagesfrom'      => '顯示以下位置開始嘅頁面:',
'allarticles'       => '所有頁面',
'allinnamespace'    => '所有頁面（喺$1空間名入面）',
'allnotinnamespace' => '所有頁面（唔喺$1空間名入面）',
'allpagesprev'      => '上一頁',
'allpagesnext'      => '下一頁',
'allpagessubmit'    => '去搵',
'allpagesprefix'    => '用以下開頭嘅頁面：',
'allpagesbadtitle'  => '提供嘅頁面名無效，又或者有一個跨語言或跨wiki嘅字頭。佢可能包括一個或多個字係唔可以用響標題度嘅。',
'allpages-bad-ns'   => '{{SITENAME}}係無一個空間名叫做"$1"。',

# Special:Categories
'categories'                    => '類',
'categoriespagetext'            => '下面嘅類有版或媒體。
[[Special:UnusedCategories|未用類]]唔會響呢度列示。
請同時參閱[[Special:WantedCategories|需要嘅分類]]。',
'categoriesfrom'                => '顯示由呢項起嘅類:',
'special-categories-sort-count' => '跟數量排',
'special-categories-sort-abc'   => '跟字母排',

# Special:ListUsers
'listusersfrom'      => '顯示由呢個字開始嘅用戶：',
'listusers-submit'   => '顯示',
'listusers-noresult' => '搵唔到用戶。',

# Special:ListGroupRights
'listgrouprights'          => '用戶組權限',
'listgrouprights-summary'  => '下面係一個響呢個wiki定義咗嘅用戶權限一覽，同埋佢哋嘅存取權。
更多有關個別權限嘅[[{{MediaWiki:Listgrouprights-helppage}}|更多細節]]可以響嗰度搵到。',
'listgrouprights-group'    => '組',
'listgrouprights-rights'   => '權',
'listgrouprights-helppage' => 'Help:組權限',
'listgrouprights-members'  => '(成員名單)',

# E-mail user
'mailnologin'     => '冇傳送地址',
'mailnologintext' => '你一定要[[Special:UserLogin|登入咗]]同埋喺你嘅[[Special:Preferences|喜好設定]]度有個有效嘅電郵地址先可以傳送電郵畀其他用戶。',
'emailuser'       => '發電郵畀呢位用戶',
'emailpage'       => '發電郵畀用戶',
'emailpagetext'   => '如果呢位用戶已經喺佢嘅用戶使用偏好入邊填咗個有效嘅電郵地址，以下表格會發送單單一條信息。你喺[[Special:Preferences|你嘅用戶喜好設定]]入面填寫嘅電郵地址會出現喺呢封電郵「由」嘅地址度，以便收件人可以回覆到。',
'usermailererror' => '目標郵件地址返回錯誤：',
'defemailsubject' => '{{SITENAME}} 電郵',
'noemailtitle'    => '無電郵地址',
'noemailtext'     => '呢個用戶重指指定一個有效嘅電郵電址，又或者佢揀咗唔收其他用戶畀佢嘅電郵。',
'emailfrom'       => '由:',
'emailto'         => '到:',
'emailsubject'    => '主題:',
'emailmessage'    => '信息:',
'emailsend'       => '傳送',
'emailccme'       => '傳送一個我嘅信息電郵畀我。',
'emailccsubject'  => '你畀$1: $2封信嘅副本',
'emailsent'       => '電郵已傳送',
'emailsenttext'   => '你嘅電郵信息已傳送。',
'emailuserfooter' => '呢封電郵係由$1寄畀$2經{{SITENAME}}嘅「電郵用戶」功能發出嘅。',

# Watchlist
'watchlist'            => '監視清單',
'mywatchlist'          => '我張監視清單',
'watchlistfor'         => "（用戶「'''$1'''」嘅監視清單）",
'nowatchlist'          => '你嘅監視清單度並冇任何項目。',
'watchlistanontext'    => '請先$1去睇或者改響你監視清單度嘅項目。',
'watchnologin'         => '未登入',
'watchnologintext'     => '你必須先[[Special:UserLogin|登入]]至可以更改你嘅監視清單。',
'addedwatch'           => '加到監視清單度',
'addedwatchtext'       => "頁面「[[:$1]]」已加入到你嘅[[Special:Watchlist|監視清單]]度。
呢個頁面以及佢個討論頁以後嘅修改都會列喺嗰度，佢喺[[Special:RecentChanges|最近更改清單]]度會以'''粗體'''顯示，等你可以容易啲睇到佢。",
'removedwatch'         => '已經由監視清單中刪除',
'removedwatchtext'     => '頁面「[[:$1]]」已經喺[[Special:Watchlist|你嘅監視清單]]度刪除。',
'watch'                => '監視',
'watchthispage'        => '監視呢頁',
'unwatch'              => '唔使監視',
'unwatchthispage'      => '停止監視',
'notanarticle'         => '唔係一個內容頁',
'notvisiblerev'        => '修訂已經刪除咗',
'watchnochange'        => '響顯示嘅時間之內，你所監視嘅頁面並無任何嘅更改。',
'watchlist-details'    => '唔計討論頁，有 $1 版響你個監視清單度。',
'wlheader-enotif'      => '* 電子郵件通知已經啟用。',
'wlheader-showupdated' => "* '''粗體字'''嘅頁響你上次嚟之後被人改過",
'watchmethod-recent'   => '睇緊最近修改中有邊頁監視緊',
'watchmethod-list'     => '睇緊被監視頁有乜新修改',
'watchlistcontains'    => '你嘅監視清單裏面有$1頁。',
'iteminvalidname'      => "項目'$1'出錯，無效嘅名稱...",
'wlnote'               => "以下係最近'''$2'''個鐘之內嘅最新$1次修改。",
'wlshowlast'           => '顯示最近 $1 個鐘 $2 日 $3 嘅修改',
'watchlist-show-bots'  => '顯示機械人嘅編輯',
'watchlist-hide-bots'  => '隱藏機械人嘅編輯',
'watchlist-show-own'   => '顯示我嘅編輯',
'watchlist-hide-own'   => '隱藏我嘅編輯',
'watchlist-show-minor' => '顯示小修改',
'watchlist-hide-minor' => '隱藏小修改',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => '監視緊...',
'unwatching' => '唔再監視緊...',

'enotif_mailer'                => '{{SITENAME}}通知郵遞員',
'enotif_reset'                 => '將所有頁面標成已視察',
'enotif_newpagetext'           => '呢個係一個新頁面。',
'enotif_impersonal_salutation' => '{{SITENAME}}用戶',
'changed'                      => '修改過',
'created'                      => '建立過',
'enotif_subject'               => '{{SITENAME}}嘅頁面$PAGETITLE已由$PAGEEDITOR$CHANGEDORCREATED',
'enotif_lastvisited'           => '你上次視察以嚟嘅修改請睇$1。',
'enotif_lastdiff'              => '睇$1去睇吓呢一次更改。',
'enotif_anon_editor'           => '匿名用戶$1',
'enotif_body'                  => '$WATCHINGUSERNAME先生／小姐你好,

{{SITENAME}}嘅頁面$PAGETITLE已經由$PAGEEDITOR喺$PAGEEDITDATE$CHANGEDORCREATED，現時版本請睇$PAGETITLE_URL。

The {{SITENAME}} page $PAGETITLE has been $CHANGEDORCREATED on $PAGEEDITDATE by $PAGEEDITOR, see $PAGETITLE_URL for the current version.

$NEWPAGE

編輯者留低嘅摘要：$PAGESUMMARY $PAGEMINOREDIT

連絡呢個編輯者:
mail: $PAGEEDITOR_EMAIL
wiki: $PAGEEDITOR_WIKI

今後唔會再有進一步嘅通知，除非你再次訪問呢個頁面。你亦都可以喺你嘅監視清單度復位所有監視頁面嘅通知標誌。

            {{SITENAME}}通知系統敬上

--
要修改你嘅監視清單設定，請睇{{fullurl:{{ns:special}}:Watchlist/edit}}

回饋及更多幫助：
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => '刪除頁面',
'confirm'                     => '確認',
'excontent'                   => "內容係：'$1'",
'excontentauthor'             => "內容係：'$1' (而且唯一嘅貢獻者係'[[Special:Contributions/$2|$2]]')",
'exbeforeblank'               => "喺清空之前嘅內容係：'$1'",
'exblank'                     => '頁面之前係空嘅',
'delete-confirm'              => '刪除"$1"',
'delete-legend'               => '刪除',
'historywarning'              => '警告：你要刪除嘅頁面有歷史版本：',
'confirmdeletetext'           => '你準備刪除一個頁面或者圖像，包括佢嘅所有歷史版本。
請確認你打算噉做，而且你知道後果係點，加上確認你噉做冇違反到[[{{MediaWiki:Policy-url}}]]。',
'actioncomplete'              => '操作完成',
'deletedtext'                 => '"<nowiki>$1</nowiki>"已經刪除。最近嘅刪除記錄請睇$2。',
'deletedarticle'              => '已經刪除"[[$1]]"',
'suppressedarticle'           => '已經廢止"[[$1]]"',
'dellogpage'                  => '刪除日誌',
'dellogpagetext'              => '以下係最近嘅刪除清單。',
'deletionlog'                 => '刪除日誌',
'reverted'                    => '恢復到先前嘅修訂',
'deletecomment'               => '刪除原因:',
'deleteotherreason'           => '其它／附加嘅原因:',
'deletereasonotherlist'       => '其它原因',
'deletereason-dropdown'       => '
*常用刪除原因
** 作者請求
** 侵犯版權
** 破壞',
'delete-edit-reasonlist'      => '編輯刪除原因',
'delete-toobig'               => '呢一版有一個好大量嘅編輯歷史，過咗$1次修訂。刪除呢類版嘅動作已經限制咗，以防止響{{SITENAME}}嘅意外擾亂。',
'delete-warning-toobig'       => '呢一版有一個好大量嘅編輯歷史，過咗$1次修訂。刪除佢可能會擾亂{{SITENAME}}嘅資料庫操作；響繼續嗰陣請小心。',
'rollback'                    => '反轉修改',
'rollback_short'              => '反轉',
'rollbacklink'                => '反轉',
'rollbackfailed'              => '反轉唔到',
'cantrollback'                => '反轉唔到；上一位貢獻者係唯一修改過呢版嘅人。',
'alreadyrolled'               => '無法反轉[[User:$2|$2]]（[[User talk:$2|留言]] | [[Special:Contributions/$2|{{int:contribslink}}]]）對[[:$1]]嘅最後編輯；有人已經修改過或者反轉咗呢個頁面。

上次對呢版嘅編輯係由[[User:$3|$3]]（[[User talk:$3|留言]] | [[Special:Contributions/$3|{{int:contribslink}}]]）做嘅。',
'editcomment'                 => "編輯摘要係：\"''\$1''\"。", # only shown if there is an edit comment
'revertpage'                  => '已經反轉由[[Special:Contributions/$2|$2]]（[[User talk:$2|對話]]）所寫嘅編輯到[[User:$1|$1]]嘅最後版本。', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '已經反轉由$1所寫嘅編輯；恢復到$2嘅最後版本。',
'sessionfailure'              => '你嘅登入會話 (session) 好似有啲問題；為咗防止會話劫持，呢個操作已經取消。請撳「返轉頭」然後重新載入你嚟自嘅頁面，然後再試吓啦。',
'protectlogpage'              => '保護日誌',
'protectlogtext'              => '下面係一個保護同埋解除保護頁面嘅一覽表。睇吓[[Special:ProtectedPages|保護頁面一覽]]去拎現時進行緊嘅頁面保護一覽。',
'protectedarticle'            => '已經保護 "[[$1]]"',
'modifiedarticleprotection'   => '已經改咗 "[[$1]]" 嘅保護等級',
'unprotectedarticle'          => '已經唔再保護 "[[$1]]"',
'protect-title'               => '改緊「$1」嘅保護等級',
'protect-legend'              => '確認保護',
'protectcomment'              => '註解:',
'protectexpiry'               => '到期:',
'protect_expiry_invalid'      => '到期時間唔正確。',
'protect_expiry_old'          => '到期時間係響之前過去嘅。',
'protect-unchain'             => '解除移動權限嘅鎖定',
'protect-text'                => "你可以喺呢度睇到同修改頁面'''<nowiki>$1</nowiki>'''嘅保護等級。",
'protect-locked-blocked'      => "當你響被封鎖嗰陣唔可以改呢版嘅保護等級。
呢個係'''$1'''版嘅現時設定：",
'protect-locked-dblock'       => "響資料庫主動鎖住咗嗰陣係唔可以改呢版嘅保護等級。
呢個係'''$1'''版嘅現時設定：",
'protect-locked-access'       => "你嘅戶口係無權限去改呢版嘅保護等級。
呢個係'''$1'''版嘅現時設定：",
'protect-cascadeon'           => '呢一版現時正響度保護緊，因為佢係響以下嘅{{PLURAL:$1|一|幾}}頁度包含咗，而當中又開咗連串保護。你可以更改呢一版嘅保護等級，但係呢個修改係唔會影響到嗰個連串保護。',
'protect-default'             => '（預設）',
'protect-fallback'            => '需要"$1"嘅許可',
'protect-level-autoconfirmed' => '限制未註冊嘅用戶',
'protect-level-sysop'         => '只限操作員',
'protect-summary-cascade'     => '連串保護',
'protect-expiring'            => '響 $1 (UTC) 到期',
'protect-cascade'             => '保護包含響呢一版嘅頁面 (連串保護)',
'protect-cantedit'            => '你唔可以改呢版嘅保護等級，因為你無權限去編輯佢。',
'restriction-type'            => '許可:',
'restriction-level'           => '限制等級:',
'minimum-size'                => '最小大細',
'maximum-size'                => '最大大細:',
'pagesize'                    => '(bytes)',

# Restrictions (nouns)
'restriction-edit'   => '編輯',
'restriction-move'   => '移動',
'restriction-create' => '建立',
'restriction-upload' => '上載',

# Restriction levels
'restriction-level-sysop'         => '全保護',
'restriction-level-autoconfirmed' => '半保護',
'restriction-level-all'           => '任何等級',

# Undelete
'undelete'                     => '去睇刪除咗嘅頁面',
'undeletepage'                 => '去睇同恢復刪除咗嘅頁面',
'undeletepagetitle'            => "'''下面包括咗[[:$1]]嘅已刪除修訂'''。",
'viewdeletedpage'              => '去睇被刪除咗嘅頁面',
'undeletepagetext'             => '以下頁面已經刪除，但係重喺檔庫度可以恢復。檔案庫可能會定時清理。',
'undelete-fieldset-title'      => '恢復修訂',
'undeleteextrahelp'            => "要恢復成個頁面，唔好剔任何嘅核選盒，再撳'''''恢復'''''。要恢復已經選擇咗嘅修訂，將要恢復代表有關修訂嘅核選盒剔上，再撳'''''恢復'''''。撳'''''重設'''''會清除註解文字同埋全部嘅核選盒。",
'undeleterevisions'            => '$1個修訂都已經存檔',
'undeletehistory'              => '如果你恢復呢個頁面，佢嘅所有修改歷史都會恢復返到嗰篇頁面嘅歷史度。如果喺佢刪除之後又新開咗同名嘅頁面，你恢復嘅修改歷史會顯示喺先前歷史度。',
'undeleterevdel'               => '如果響最新修訂度部份刪除，噉反刪除就唔能夠進行。如果遇到呢種情況，你一定要反選或者反隱藏最新刪除咗嘅修訂。',
'undeletehistorynoadmin'       => '呢一版已經刪咗。刪除嘅原因喺下面嘅摘要度，連同重有刪除之前編輯過呢個頁面嘅用戶嘅詳細資料。所刪除嘅版本嘅實際內容得管理員可以睇到。',
'undelete-revision'            => '已經刪除咗$1嗰陣（響$2）由$3所寫嘅修訂:',
'undeleterevision-missing'     => '唔正確或者遺失咗修訂。你可能有一個壞連結，或者嗰個修訂已經響存檔度恢復咗或者刪除咗。',
'undelete-nodiff'              => '搵唔到之前嘅修訂。',
'undeletebtn'                  => '恢復',
'undeletelink'                 => '恢復',
'undeletereset'                => '重設',
'undeletecomment'              => '註解：',
'undeletedarticle'             => '已經恢復咗"[[$1]]"',
'undeletedrevisions'           => '$1個修訂已經恢復',
'undeletedrevisions-files'     => '$1個修訂同$2個檔案已經恢復',
'undeletedfiles'               => '$1個檔案已經恢復',
'cannotundelete'               => '反刪除失敗；可能有其他人已經反刪除嗰一頁。',
'undeletedpage'                => "<big>'''$1已經成功恢復'''</big>

最近嘅刪除同恢復記錄請睇[[Special:Log/delete]]。",
'undelete-header'              => '睇吓[[Special:Log/delete|刪除日誌]]去睇之前刪除嘅頁頁。',
'undelete-search-box'          => '搵刪除咗嘅頁面',
'undelete-search-prefix'       => '顯示由以下開頭嘅頁面：',
'undelete-search-submit'       => '搵嘢',
'undelete-no-results'          => '響刪除存檔度搵唔到符合嘅頁面。',
'undelete-filename-mismatch'   => '唔能夠刪除帶有時間截記嘅檔案修訂 $1: 檔案錯配',
'undelete-bad-store-key'       => '唔能夠刪除帶有時間截記嘅檔案修訂 $1: 檔案響刪除之前唔見咗。',
'undelete-cleanup-error'       => '刪除無用嘅歸檔檔案 "$1" 時出錯。',
'undelete-missing-filearchive' => '由於檔案歸檔 ID $1 唔響個數據庫度，唔能夠響個檔案歸檔恢復。佢可能已經反刪除咗。',
'undelete-error-short'         => '反刪除檔案嗰陣出錯: $1',
'undelete-error-long'          => '當反刪除緊個檔案嗰陣遇到錯誤:

$1',

# Namespace form on various pages
'namespace'      => '空間名：',
'invert'         => '反選',
'blanknamespace' => '（主）',

# Contributions
'contributions' => '用戶貢獻',
'mycontris'     => '我嘅貢獻',
'contribsub2'   => '$1嘅貢獻 ($2)',
'nocontribs'    => '搵唔到符合呢啲條件嘅修改。',
'uctop'         => '(最頂)',
'month'         => '由呢個月 (同更早):',
'year'          => '由呢一年 (同更早):',

'sp-contributions-newbies'     => '只顯示新戶口嘅貢獻',
'sp-contributions-newbies-sub' => '新戶口嘅貢獻',
'sp-contributions-blocklog'    => '封鎖日誌',
'sp-contributions-search'      => '搵貢獻',
'sp-contributions-username'    => 'IP地址或用戶名：',
'sp-contributions-submit'      => '搵',

# What links here
'whatlinkshere'            => '有乜嘢連結來呢度',
'whatlinkshere-title'      => '連到「$1」嘅頁',
'whatlinkshere-page'       => '頁:',
'linklistsub'              => '（連結一覽）',
'linkshere'                => "呢啲頁連結到'''[[:$1]]'''：",
'nolinkshere'              => "無一頁連結到'''[[:$1]]'''。",
'nolinkshere-ns'           => "響已經揀咗嘅空間名無嘢連結到'''[[:$1]]'''。",
'isredirect'               => '跳轉頁',
'istemplate'               => '包含',
'isimage'                  => '檔案連結',
'whatlinkshere-prev'       => '前$1版',
'whatlinkshere-next'       => '後$1版',
'whatlinkshere-links'      => '← 連結',
'whatlinkshere-hideredirs' => '$1跳轉',
'whatlinkshere-hidetrans'  => '$1包含',
'whatlinkshere-hidelinks'  => '$1連結',
'whatlinkshere-hideimages' => '$1檔案連結',
'whatlinkshere-filters'    => '過濾器',

# Block/unblock
'blockip'                         => '封鎖用戶',
'blockip-legend'                  => '封鎖用戶',
'blockiptext'                     => '使用以下嘅表格嚟去阻止指定嘅IP地址或用戶名嘅寫權限。
僅當僅當為咗避免有版畀人惡意破壞嘅時候先可以使用，而且唔可以違反[[{{MediaWiki:Policy-url}}|政策]]。
喺下面填寫阻止嘅確切原因（比如：引用咗某啲已經破壞咗嘅頁面）。',
'ipaddress'                       => 'IP地址:',
'ipadressorusername'              => 'IP地址或用戶名:',
'ipbexpiry'                       => '期限:',
'ipbreason'                       => '原因:',
'ipbreasonotherlist'              => '其它原因',
'ipbreason-dropdown'              => '
*共用封鎖原因
** 插入錯嘅資料
** 響頁面度拎走
** 亂加入外部連結
** 響頁度加入冇意義嘅嘢
** 嚇人／騷擾
** 濫用多個戶口
** 唔能夠接受嘅用戶名',
'ipbanononly'                     => '只係封鎖匿名用戶',
'ipbcreateaccount'                => '防止開新戶口',
'ipbemailban'                     => '防止用戶傳送電郵',
'ipbenableautoblock'              => '自動封鎖呢個用戶上次用過嘅IP地址，同埋佢地做過編輯嘅IP地址',
'ipbsubmit'                       => '封鎖呢位用戶',
'ipbother'                        => '其它時間:',
'ipboptions'                      => '兩個鐘頭:2 hours,一日:1 day,三日:3 days,一個禮拜:1 week,兩個禮拜:2 weeks,一個月:1 month,三個月:3 months,六個月:6 months,一年:1 year,終身:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => '其它',
'ipbotherreason'                  => '其它／附加嘅原因:',
'ipbhidename'                     => '響個封鎖日誌、現時嘅封鎖名單以用戶名單度隱藏用戶名',
'ipbwatchuser'                    => '監視呢位用戶嘅用戶頁同埋佢嘅討論頁',
'badipaddress'                    => '無效嘅IP地址',
'blockipsuccesssub'               => '封鎖成功',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]]已經封鎖。<br />
去[[Special:IPBlockList|IP封鎖清單]]睇返封鎖名單。',
'ipb-edit-dropdown'               => '改封鎖原因',
'ipb-unblock-addr'                => '解封$1',
'ipb-unblock'                     => '解封一個用戶名或IP地址',
'ipb-blocklist-addr'              => '去睇$1嘅現時封鎖',
'ipb-blocklist'                   => '去睇現時嘅封鎖',
'unblockip'                       => '解封用戶',
'unblockiptext'                   => '使用以下表格恢復之前阻止嘅某個IP地址或者某個用戶名嘅寫權限。',
'ipusubmit'                       => '解封呢個地址',
'unblocked'                       => '"[[User:$1|$1]]"已經解封',
'unblocked-id'                    => '$1嘅封鎖已經拎走咗',
'ipblocklist'                     => 'IP地址同用戶名阻止名單',
'ipblocklist-legend'              => '搵一位封咗嘅用戶',
'ipblocklist-username'            => '用戶名或IP地址:',
'ipblocklist-submit'              => '搵',
'blocklistline'                   => '$1，$2已經封鎖咗$3（$4）',
'infiniteblock'                   => '不設期限',
'expiringblock'                   => '$1 期滿',
'anononlyblock'                   => '只限匿名',
'noautoblockblock'                => '自動封鎖已經停用',
'createaccountblock'              => '封咗開新戶口',
'emailblock'                      => '封咗電郵',
'ipblocklist-empty'               => '封鎖名單係空嘅。',
'ipblocklist-no-results'          => '所請求嘅IP地址或用戶名係冇被封鎖嘅。',
'blocklink'                       => '封鎖',
'unblocklink'                     => '解封',
'contribslink'                    => '貢獻',
'autoblocker'                     => '已經自動封鎖，因為你嘅IP地址冇幾耐之前"[[User:$1|$1]]"使用過。$1\\嘅封鎖原因係: 「$2」',
'blocklogpage'                    => '封鎖日誌',
'blocklogentry'                   => '已封鎖[[$1]]，到期時間為$2 $3',
'blocklogtext'                    => '呢個係封鎖同埋解封動作嘅日誌。自動封鎖IP地址嘅動作冇列出嚟。去[[Special:IPBlockList|IP封鎖名單]]睇現時生效嘅封鎖名單',
'unblocklogentry'                 => '已經解封$1',
'block-log-flags-anononly'        => '只限匿名用戶',
'block-log-flags-nocreate'        => '停用開新戶口',
'block-log-flags-noautoblock'     => '停用自動封鎖器',
'block-log-flags-noemail'         => '封咗電郵',
'block-log-flags-angry-autoblock' => '加強自動封鎖已經啟用',
'range_block_disabled'            => '操作員嘅建立範圍封鎖已經停用。',
'ipb_expiry_invalid'              => '無效嘅期限。',
'ipb_expiry_temp'                 => '隱藏用戶名封鎖定一定係要永久性嘅。',
'ipb_already_blocked'             => '"$1"已經封鎖咗',
'ipb_cant_unblock'                => '錯誤：搵唔到封鎖ID$1。可能已經解封咗。',
'ipb_blocked_as_range'            => '錯誤：個IP $1 無直接封鎖，唔可以解封。但係佢係響 $2 嘅封鎖範圍之內，嗰段範圍係可以解封嘅。',
'ip_range_invalid'                => '無效嘅IP範圍',
'blockme'                         => '封鎖我',
'proxyblocker'                    => 'Proxy 封鎖器',
'proxyblocker-disabled'           => '呢個功能已經停用。',
'proxyblockreason'                => '你嘅IP係一個公開（指任何人都可以用，無須身份認證？）嘅代理地址，因此被封鎖。請聯絡你嘅Internet服務提供商或技術支援，向佢哋報告呢個嚴重嘅安全問題。',
'proxyblocksuccess'               => '完成。',
'sorbsreason'                     => '你嘅IP地址已經畀響{{SITENAME}}度用嘅DNSBL列咗做公開代理。',
'sorbs_create_account_reason'     => '你嘅IP地址已經畀響{{SITENAME}}度用嘅DNSBL列咗做公開代理。你唔可以開新戶口。',

# Developer tools
'lockdb'              => '鎖定資料庫',
'unlockdb'            => '解除鎖定資料庫',
'lockdbtext'          => '鎖定資料庫會暫停所有用戶去編輯頁面、更改佢哋嘅喜好設定、編輯佢哋嘅監視清單嘅能力，同埋其它需要喺資料庫中更改嘅動作。
請確認你的確係需要要噉做，喺你嘅維護工作完成之後會解除鎖定資料庫。',
'unlockdbtext'        => '解除資料庫鎖定以便其他用戶可以恢復進行編輯頁面、修改使用偏好、修改監視清單以及其他需要修改資料庫嘅操作。
請確認你的而且確打算噉做。',
'lockconfirm'         => '係，我真係想去鎖定資料庫。',
'unlockconfirm'       => '係，我真係想去解除鎖定資料庫。',
'lockbtn'             => '鎖定資料庫',
'unlockbtn'           => '解除鎖定資料庫',
'locknoconfirm'       => '你未剔個確認框喎。',
'lockdbsuccesssub'    => '資料庫鎖定已經成功',
'unlockdbsuccesssub'  => '資料庫鎖定已成功移除',
'lockdbsuccesstext'   => '資料庫現已鎖定。<br />
請一定要記得喺完成系統維護工作之後[[Special:UnlockDB|解除資料庫嘅鎖定]]。',
'unlockdbsuccesstext' => '資料庫鎖定現已解除。',
'lockfilenotwritable' => '資料庫封鎖檔案係唔可以寫入嘅。要鎖定或解鎖資料庫，係需要由網頁伺服器中寫入。',
'databasenotlocked'   => '資料庫而家冇鎖到。',

# Move page
'move-page'               => '搬$1',
'move-page-legend'        => '搬頁',
'movepagetext'            => "使用以下表格會將頁面改名，兼且連同搬埋佢嘅歷史過去。
舊標題會變成指去新標題嘅跳轉頁。
你可以自動噉更新指到原先標題嘅跳轉。
如果你揀咗唔去做嘅話，請務必要檢查吓有冇[[Special:Doubledirects|雙重跳轉]]或者[[Special:BrokenRedirects|死跳轉]]（嘅情況發生）。
你有責任確保啲連結依然指去佢哋應該指去嘅地方。

注意如果已經有一個同個新名同名嘅頁面，噉呢個頁面係搬'''唔到'''嘅，除非嗰個同名嘅頁面係空嘅或者佢係一個跳轉頁，兼且要之前冇編輯過（冇編輯歷史）先得。噉即係講萬一你搞錯咗，你可以將呢個頁面改返去佢改之前噉，你唔可以覆蓋一個現有嘅頁面。

'''警告！'''
噉樣對於一個好多人經過嘅頁面嚟講可能係一個好大嘅同埋出人意表嘅修改；請你喺行動之前確認你清楚噉做嘅後果。",
'movepagetalktext'        => "相應嘅討論頁會連同佢一齊自動搬過去，'''除非'''：
*新嘅頁面名下面已經有咗一個非空嘅討論頁，又或者
*你唔剔下面個框。

喺呢啲情況下，需要嘅話你唯有手動搬同合併個頁面。",
'movearticle'             => '搬頁:',
'movenotallowed'          => '你響{{SITENAME}}度並無權限去搬版。',
'newtitle'                => '到新標題:',
'move-watch'              => '睇實呢一版',
'movepagebtn'             => '搬頁',
'pagemovedsub'            => '搬頁成功',
'movepage-moved'          => '<big>\'\'\'"$1"已經搬到去"$2"\'\'\'</big>', # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => '已經有頁面叫嗰個名，或者你揀嘅名唔合法。請揀過第二個名。',
'cantmove-titleprotected' => '你唔可以搬呢版去呢個位置，因為個新標題已經保護咗，唔畀開版。',
'talkexists'              => "'''頁面本身已經成功搬咗，但係個討論頁搬唔到，因為已經有一個同名嘅討論頁。請手工合併佢哋。'''",
'movedto'                 => '搬去',
'movetalk'                => '搬相應嘅討論頁',
'move-subpages'           => '如果可以嘅話，搬全部嘅細頁',
'move-talk-subpages'      => '如果可以嘅話，搬全部嘅細討論頁',
'movepage-page-exists'    => '版$1已經存在，唔可以自動噉覆寫。',
'movepage-page-moved'     => '版$1已經搬到去$2。',
'movepage-page-unmoved'   => '版$1唔可以搬到去$2。',
'movepage-max-pages'      => '最多有$1版已經搬咗同時唔可以自動噉再搬更多。',
'1movedto2'               => '[[$1]]搬到去[[$2]]',
'1movedto2_redir'         => '[[$1]]通過跳轉搬到去[[$2]]',
'movelogpage'             => '移動日誌',
'movelogpagetext'         => '以下係搬過嘅頁面清單。',
'movereason'              => '原因',
'revertmove'              => '恢復',
'delete_and_move'         => '刪除並移動',
'delete_and_move_text'    => '==需要刪除==

目標頁「[[:$1]]」已經存在。你要唔要刪咗佢空個位出嚟畀個搬文動作？',
'delete_and_move_confirm' => '好，刪咗嗰個頁面',
'delete_and_move_reason'  => '已經刪咗嚟畀位畀個搬文動作',
'selfmove'                => '原始標題同目的標題一樣；唔可以將個頁面搬返去自己度。',
'immobile_namespace'      => '來源或目的標題屬於特別類型；唔可以將頁面搬自或搬去嗰個空間名。',
'imagenocrossnamespace'   => '唔可以搬檔案到非檔案空間名',
'imagetypemismatch'       => '個新副檔名唔配佢嘅類型',
'imageinvalidfilename'    => '個目標檔名係無效嘅',
'fix-double-redirects'    => '更新指到原先標題嘅任何跳轉',

# Export
'export'            => '倒出/導出/匯出（Export）頁面',
'exporttext'        => '你可以倒出文字、編輯某個頁面、編輯封裝（wrap）喺一啲XML度嘅一組頁面。呢啲嘢可以用MediaWiki透過[[Special:Import|倒入]]頁倒入去其他wiki度。

要倒出頁面嘅話，就喺下面嘅文字框度打標題名，一行一個標題，然後揀你係要現時版本加上所有嘅舊版本同歷史，定係淨係要現時版本同最後編輯嘅相關資訊。

喺後面嗰種情況下，你亦都可以用一個連結，例如[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]]對頁面"[[{{MediaWiki:Mainpage}}]]"。',
'exportcuronly'     => '淨係包括而家嘅修訂版本，唔包括完整歷史',
'exportnohistory'   => "----
'''注意：'''因為性能嘅原因，已經停用禁止咗使用呢個表格倒出頁面嘅完整歷史",
'export-submit'     => '倒出/導出/匯出',
'export-addcattext' => '由分類度加入頁面：',
'export-addcat'     => '加入',
'export-download'   => '另存做檔案',
'export-templates'  => '包含模',

# Namespace 8 related
'allmessages'               => '系統信息',
'allmessagesname'           => '名稱',
'allmessagesdefault'        => '預設文字',
'allmessagescurrent'        => '現時文字',
'allmessagestext'           => '以下係 MediaWiki 空間名入邊現有系統信息嘅清單。
如果想貢獻正宗嘅MediaWiki本地化嘅話，請參閱[http://www.mediawiki.org/wiki/Localisation MediaWiki本地化]同埋[http://translatewiki.net Betawiki]。',
'allmessagesnotsupportedDB' => "呢一版唔可以用，因為'''\$wgUseDatabaseMessages'''已經閂咗。",
'allmessagesfilter'         => '信息名過濾（器）：',
'allmessagesmodified'       => '只顯示修改過嘅',

# Thumbnails
'thumbnail-more'           => '放大',
'filemissing'              => '唔見個檔案',
'thumbnail_error'          => '整唔到縮圖: $1',
'djvu_page_error'          => 'DjVu頁超出範圍',
'djvu_no_xml'              => '唔能夠響DjVu檔度攞個XML',
'thumbnail_invalid_params' => '唔正確嘅縮圖參數',
'thumbnail_dest_directory' => '唔能夠開目標目錄',

# Special:Import
'import'                     => '倒入頁面',
'importinterwiki'            => 'Transwiki 倒入',
'import-interwiki-text'      => '揀一個 wiki 同埋一頁去倒入。
修訂日期同編輯者會被保存落嚟。
所有 transwiki 嘅倒入動作會響[[Special:Log/import|倒入日誌]]度記錄落嚟。',
'import-interwiki-history'   => '複製呢一頁所有嘅歷史版本',
'import-interwiki-submit'    => '倒入',
'import-interwiki-namespace' => '轉移頁面到空間名：',
'importtext'                 => '請由原 wiki 嘅[[Special:Export|匯出工具]]匯出成檔案。
儲存喺你個磁碟度，然後再上載到呢度。',
'importstart'                => '倒入緊...',
'import-revision-count'      => '$1次修訂',
'importnopages'              => '冇頁面去倒入。',
'importfailed'               => '倒入失敗：<nowiki>$1</nowiki>',
'importunknownsource'        => '不明嘅倒入來源類型',
'importcantopen'             => '唔能夠開個倒入檔案',
'importbadinterwiki'         => '壞嘅跨 wiki 連結',
'importnotext'               => '空白或者唔係文字',
'importsuccess'              => '已經完成倒入！',
'importhistoryconflict'      => '存在有衝突嘅歷史版本（之前可能曾經倒入過呢頁）',
'importnosources'            => '未定義 transwiki 嘅匯入來源，同埋歷史嘅直接上載已經停用。',
'importnofile'               => '冇上載到任何要倒入嘅檔案。',
'importuploaderrorsize'      => '上載要倒入嘅檔案失敗。個檔案大過可以容許嘅上載大細。',
'importuploaderrorpartial'   => '上載要倒入嘅檔案失敗。個檔案只係部份上載咗。',
'importuploaderrortemp'      => '上載要倒入嘅檔案失敗。個臨時資料夾唔見咗。',
'import-parse-failure'       => 'XML倒入語法失敗',
'import-noarticle'           => '無版去倒入！',
'import-nonewrevisions'      => '全部嘅修訂已經響之前倒入咗。',
'xml-error-string'           => '$1 響行$2，欄$3 ($4 bytes): $5',
'import-upload'              => '上載XML資料',

# Import log
'importlogpage'                    => '倒入日誌',
'importlogpagetext'                => '管理員由其它嘅 wiki 倒入頁面同埋佢哋嘅編輯歷史記錄。',
'import-logentry-upload'           => '由檔案上載倒入咗 [[$1]]',
'import-logentry-upload-detail'    => '$1個修訂',
'import-logentry-interwiki'        => 'transwiki咗 $1',
'import-logentry-interwiki-detail' => '由$2嘅$1個修訂',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '我嘅用戶頁',
'tooltip-pt-anonuserpage'         => '你編輯呢個IP嘅對應用戶頁',
'tooltip-pt-mytalk'               => '我嘅對話頁',
'tooltip-pt-anontalk'             => '對於嚟自呢一個IP地址編輯嘅討論',
'tooltip-pt-preferences'          => '我嘅喜好設定',
'tooltip-pt-watchlist'            => '你所監視嘅頁面更改一覽',
'tooltip-pt-mycontris'            => '我嘅貢獻一覽',
'tooltip-pt-login'                => '登入係唔需要嘅，但會帶嚟好多嘅好處',
'tooltip-pt-anonlogin'            => '登入係唔需要嘅，但會帶嚟好多嘅好處',
'tooltip-pt-logout'               => '登出',
'tooltip-ca-talk'                 => '關於內容頁嘅討論',
'tooltip-ca-edit'                 => '你可以編輯呢一頁。請在儲存之前先預覽一吓。',
'tooltip-ca-addsection'           => '開始新嘅討論',
'tooltip-ca-viewsource'           => '呢一頁已經被保護。你可以睇吓呢一頁呢原始碼。',
'tooltip-ca-history'              => '呢一頁之前嘅版本',
'tooltip-ca-protect'              => '保護呢一頁',
'tooltip-ca-delete'               => '刪除呢一頁',
'tooltip-ca-undelete'             => '將呢個頁面還原到被刪除之前嘅狀態',
'tooltip-ca-move'                 => '移動呢一頁',
'tooltip-ca-watch'                => '將呢一頁加到去你嘅監視清單',
'tooltip-ca-unwatch'              => '將呢一頁喺你嘅監視清單中移去',
'tooltip-search'                  => '搵{{SITENAME}}',
'tooltip-search-go'               => '如果相同嘅標題存在嘅話就直接去嗰一版',
'tooltip-search-fulltext'         => '搵呢個文字嘅版',
'tooltip-p-logo'                  => '頭版',
'tooltip-n-mainpage'              => '睇頭版',
'tooltip-n-portal'                => '關於呢個計劃，你可以做乜，應該要點做',
'tooltip-n-currentevents'         => '提供而家發生嘅事嘅背景資料',
'tooltip-n-recentchanges'         => '列出呢個 wiki 中嘅最近修改',
'tooltip-n-randompage'            => '是但載入一個頁面',
'tooltip-n-help'                  => '搵吓點做嘅地方',
'tooltip-t-whatlinkshere'         => '列出所有連接過嚟呢度嘅頁面',
'tooltip-t-recentchangeslinked'   => '喺呢個頁面連出嘅頁面更改',
'tooltip-feed-rss'                => '呢一頁嘅RSS集合',
'tooltip-feed-atom'               => '呢一頁嘅Atom集合',
'tooltip-t-contributions'         => '睇吓呢個用戶嘅貢獻一覽',
'tooltip-t-emailuser'             => '寄封電子郵件畀呢一位用戶',
'tooltip-t-upload'                => '上載檔案',
'tooltip-t-specialpages'          => '所有特別頁嘅一覽',
'tooltip-t-print'                 => '呢一版嘅可打印版本',
'tooltip-t-permalink'             => '呢一版嘅哩個版本嘅永久連結',
'tooltip-ca-nstab-main'           => '睇吓內容頁',
'tooltip-ca-nstab-user'           => '睇吓用戶頁',
'tooltip-ca-nstab-media'          => '睇吓媒體頁',
'tooltip-ca-nstab-special'        => '呢個係一個特別頁；你唔能夠嗰一頁進行編輯。',
'tooltip-ca-nstab-project'        => '睇吓專案頁',
'tooltip-ca-nstab-image'          => '睇吓檔案頁',
'tooltip-ca-nstab-mediawiki'      => '睇吓系統信息',
'tooltip-ca-nstab-template'       => '睇吓個模',
'tooltip-ca-nstab-help'           => '睇吓幫助頁',
'tooltip-ca-nstab-category'       => '睇吓分類頁',
'tooltip-minoredit'               => '標為細嘅修訂',
'tooltip-save'                    => '保存你嘅更改',
'tooltip-preview'                 => '預覽你嘅修改，請喺保存之前先預覽一次先！',
'tooltip-diff'                    => '顯示你對頁面所作嘅修改',
'tooltip-compareselectedversions' => '顯示該頁面兩個所選版本嘅唔同之處。',
'tooltip-watch'                   => '將呢頁加到去你嘅監視清單度',
'tooltip-recreate'                => '即使已經刪除過都要重新整過呢頁',
'tooltip-upload'                  => '開始上載',

# Stylesheets
'common.css'      => '/* 響呢度放 CSS 碼來改成個網站嘅畫面 */',
'standard.css'    => '/* 響呢度放 CSS 碼去改用戶用嘅傳統畫面 */',
'nostalgia.css'   => '/* 響呢度放 CSS 碼去改用戶用嘅懷舊畫面 */',
'cologneblue.css' => '/* 響呢度放 CSS 碼去改用戶用嘅科隆藍畫面 */',
'monobook.css'    => '/* 響呢度放 CSS 碼去改用戶用嘅 Monobook 畫面 */',
'myskin.css'      => '/* 響呢度放 CSS 碼去改用戶用嘅我嘅畫面 */',
'chick.css'       => '/* 響呢度放 CSS 碼去改用戶用嘅俏畫面 */',
'simple.css'      => '/* 響呢度放 CSS 碼去改用戶用嘅簡單畫面 */',
'modern.css'      => '/* 響呢度放 CSS 碼去改用戶用嘅摩登畫面 */',

# Scripts
'common.js'      => '/* 響每一次個頁面載入時，所有用戶都會載入呢度任何嘅JavaScript。 */',
'standard.js'    => '/* 響每一次個頁面載入時，用標準畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'nostalgia.js'   => '/* 響每一次個頁面載入時，用懷舊畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'cologneblue.js' => '/* 響每一次個頁面載入時，用科隆藍畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'monobook.js'    => '/* 響每一次個頁面載入時，用 Monobook 畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'myskin.js'      => '/* 響每一次個頁面載入時，用我嘅畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'chick.js'       => '/* 響每一次個頁面載入時，用俏畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'simple.js'      => '/* 響每一次個頁面載入時，用簡單畫面嘅用戶都會載入呢度任何嘅JavaScript */',
'modern.js'      => '/* 響每一次個頁面載入時，用摩登畫面嘅用戶都會載入呢度任何嘅JavaScript */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata 已經喺呢一個伺服器上停用。',
'nocreativecommons' => 'Creative Commons RDF metadata 已經喺呢一個伺服器上停用。',
'notacceptable'     => '呢個 wiki 伺服器唔能夠畀一個可以讀嘅資料畀個客。',

# Attribution
'anonymous'        => '{{SITENAME}}嘅匿名用戶',
'siteuser'         => '{{SITENAME}}嘅用戶$1',
'lastmodifiedatby' => '呢一頁最後響 $1 $2 畀 $3 修改。', # $1 date, $2 time, $3 user
'othercontribs'    => '以$1嘅作品為基礎。',
'others'           => '其他',
'siteusers'        => '{{SITENAME}}嘅用戶$1',
'creditspage'      => '頁面信譽',
'nocredits'        => '呢一頁並無任何嘅信譽資料可以提供。',

# Spam protection
'spamprotectiontitle' => '隔垃圾器',
'spamprotectiontext'  => '隔垃圾器已經擋住咗你要儲存嘅頁面。噉可能係由指去外部網站嘅連結引起。',
'spamprotectionmatch' => '以下係觸發我哋嘅反垃圾過濾器嘅文字：$1',
'spambot_username'    => 'MediaWiki垃圾清除',
'spam_reverting'      => '恢復返去最後一個唔包含指去$1嘅連結嘅嗰個版本。',
'spam_blanking'       => '全部版本都含有指去$1嘅連結，留空',

# Info page
'infosubtitle'   => '頁面嘅資訊',
'numedits'       => '編輯次數（版頁）：$1',
'numtalkedits'   => '編輯次數（討論頁）：$1',
'numwatchers'    => '監視者數：$1',
'numauthors'     => '唔同編者嘅數目（版頁）：$1',
'numtalkauthors' => '唔同編者嘅數目（討論頁）：$1',

# Math options
'mw_math_png'    => '全部用PNG表示',
'mw_math_simple' => '如果好簡單嘅就用HTML，否則就用PNG',
'mw_math_html'   => '可以嘅話都用HTML，否則就用PNG',
'mw_math_source' => '保留返用TeX（文字瀏覽器用）',
'mw_math_modern' => '新式瀏覽器嘅建議選項',
'mw_math_mathml' => '可以嘅話用MathML（實驗中）',

# Patrolling
'markaspatrolleddiff'                 => '標示為已巡查嘅',
'markaspatrolledtext'                 => '標示呢版為已巡查嘅',
'markedaspatrolled'                   => '已經標示做已巡查嘅',
'markedaspatrolledtext'               => '已經選擇咗嘅修訂已經標示咗做已巡查嘅。',
'rcpatroldisabled'                    => '最近修改巡查已經停用',
'rcpatroldisabledtext'                => '最近修改嘅巡查功能現時停用中。',
'markedaspatrollederror'              => '唔可以標示做已巡查嘅',
'markedaspatrollederrortext'          => '你需要指定一個修訂用嚟將佢標示做已巡查嘅。',
'markedaspatrollederror-noautopatrol' => '你係唔准去標示你自己嘅更改做已巡查嘅。',

# Patrol log
'patrol-log-page'   => '巡查日誌',
'patrol-log-header' => '呢個係已經巡查過嘅日誌。',
'patrol-log-line'   => '已經標示咗$1/$2版做已經巡查嘅$3',
'patrol-log-auto'   => '(自動)',

# Image deletion
'deletedrevision'                 => '刪除咗$1嘅舊有修訂',
'filedeleteerror-short'           => '刪除檔案出錯: $1',
'filedeleteerror-long'            => '當刪除檔案嗰陣遇到錯誤:

$1',
'filedelete-missing'              => '因為個檔案 "$1" 唔存在，所以佢唔可以刪除。',
'filedelete-old-unregistered'     => '所指定嘅檔案修訂 "$1" 響個數據庫度唔存在。',
'filedelete-current-unregistered' => '所指定嘅檔案 "$1" 響個數據庫度唔存在。',
'filedelete-archive-read-only'    => '個歸檔目錄 "$1" 響網頁伺服器度寫唔到。',

# Browsing diffs
'previousdiff' => '← 上一個差異',
'nextdiff'     => '下一個差異 →',

# Media information
'mediawarning'         => "'''警告'''：呢個檔案可能有一啲惡意嘅程式編碼，如果執行佢嘅話，你嘅系統可能會被波及。<hr />",
'imagemaxsize'         => '限制檔案述頁中嘅圖像細到：',
'thumbsize'            => '縮圖大細：',
'widthheightpage'      => '$1×$2, $3版',
'file-info'            => '(檔案大細：$1 ，MIME類型：$2)',
'file-info-size'       => '($1 × $2 像素，檔案大細：$3 ，MIME類型：$4)',
'file-nohires'         => '<small>冇更高解像度嘅圖像。</small>',
'svg-long-desc'        => '(SVG檔案，表面大細： $1 × $2 像素，檔案大細：$3)',
'show-big-image'       => '完整解像度',
'show-big-image-thumb' => '<small>呢個預覽嘅大細： $1 × $2 像素</small>',

# Special:NewImages
'newimages'             => '新檔案畫廊',
'imagelisttext'         => "以下係'''$1'''個檔案$2排序嘅清單。",
'newimages-summary'     => '呢個特別頁顯示最後上載咗嘅檔案。',
'showhidebots'          => '($1 機械人)',
'noimages'              => '冇嘢去睇。',
'ilsubmit'              => '搵嘢',
'bydate'                => '以時間',
'sp-newimages-showfrom' => '顯示由$1 $2嘅新檔',

# Bad image list
'bad_image_list' => '請根據下面嘅格式去寫:

只有列示項目（以 * 開頭嘅項目）會考慮。第一個連結一定要連去幅壞檔度。
之後響同一行嘅連結會考慮做例外，即係個檔可以響邊版度同時顯示。',

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => '簡體',
'variantname-zh-hant' => '繁體',
'variantname-zh-cn'   => '簡體（中國大陸）',
'variantname-zh-tw'   => '正體（台灣）',
'variantname-zh-hk'   => '繁體（香港）',
'variantname-zh-sg'   => '簡體（新加坡）',
'variantname-zh'      => '無變換',

# Variants for Serbian language
'variantname-sr-ec' => '斯拉夫易卡語',
'variantname-sr-el' => '拉丁易卡語',
'variantname-sr'    => '無變換',

# Variants for Kazakh language
'variantname-kk-cyrl' => '哈薩克西里爾字',
'variantname-kk-latn' => '哈薩克拉丁文',
'variantname-kk-arab' => '哈薩克阿剌伯文',

# Variants for Kurdish language
'variantname-ku-arab' => '庫爾德阿剌伯文',
'variantname-ku-latn' => '庫爾德拉丁文',
'variantname-ku'      => '無變換',

# Metadata
'metadata'          => 'Metadata',
'metadata-help'     => '呢個檔案有額外嘅資料。佢應該係數碼相機或者掃描器整出來嘅。如果佢整咗之後畀人改過，裏面嘅資料未必同改過之後相符。',
'metadata-expand'   => '打開詳細資料',
'metadata-collapse' => '收埋詳細資料',
'metadata-fields'   => '響呢個信息列出嘅 EXIF 元數據項目會喺圖像頁中包含起嚟，而且個元數據表除咗喺下面列出嘅項目之外，其它嘅項目預設會被隱藏。
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => '闊',
'exif-imagelength'                 => '高',
'exif-bitspersample'               => '每部位位元數',
'exif-compression'                 => '壓細方法',
'exif-photometricinterpretation'   => '像素構成',
'exif-orientation'                 => '攞放方向',
'exif-samplesperpixel'             => '部位數',
'exif-planarconfiguration'         => '資料編排',
'exif-ycbcrsubsampling'            => 'Y 到 C 嘅二次抽樣比例',
'exif-ycbcrpositioning'            => 'Y 同 C 位置',
'exif-xresolution'                 => '橫解像度',
'exif-yresolution'                 => '直解像度',
'exif-resolutionunit'              => '橫直解像度單位',
'exif-stripoffsets'                => '圖像資料位置',
'exif-rowsperstrip'                => '每帶行數',
'exif-stripbytecounts'             => '每壓縮帶 bytes 數',
'exif-jpeginterchangeformat'       => 'JPEG SOI 嘅偏移量',
'exif-jpeginterchangeformatlength' => 'JPEG 資料嘅 bytes 數',
'exif-transferfunction'            => '轉移功能',
'exif-whitepoint'                  => '白點色度',
'exif-primarychromaticities'       => '主要嘅色度',
'exif-ycbcrcoefficients'           => '顏色空間轉換矩陣系數',
'exif-referenceblackwhite'         => '黑白對參照值',
'exif-datetime'                    => '檔案更動日期時間',
'exif-imagedescription'            => '圖名',
'exif-make'                        => '相機廠商',
'exif-model'                       => '相機型號',
'exif-software'                    => '用過嘅軟件',
'exif-artist'                      => '作者',
'exif-copyright'                   => '版權人',
'exif-exifversion'                 => 'Exif版本',
'exif-flashpixversion'             => '支援嘅 Flashpix 版本',
'exif-colorspace'                  => '色彩空間',
'exif-componentsconfiguration'     => '每個部份嘅意思',
'exif-compressedbitsperpixel'      => '影像壓縮模式',
'exif-pixelydimension'             => '影像有效闊度',
'exif-pixelxdimension'             => '影像有效高度',
'exif-makernote'                   => '廠商註腳',
'exif-usercomment'                 => '用家註腳',
'exif-relatedsoundfile'            => '相關聲音檔',
'exif-datetimeoriginal'            => '原創日期時間',
'exif-datetimedigitized'           => '制成數碼日期時間',
'exif-subsectime'                  => '日期時間細秒',
'exif-subsectimeoriginal'          => '日期時間原細秒',
'exif-subsectimedigitized'         => '日期時間數碼化細秒',
'exif-exposuretime'                => '曝光長度',
'exif-exposuretime-format'         => '$1 秒 ($2)',
'exif-fnumber'                     => 'F 值',
'exif-exposureprogram'             => '曝光程序',
'exif-spectralsensitivity'         => '光譜敏感度',
'exif-isospeedratings'             => 'ISO 速率',
'exif-oecf'                        => '光電轉換因子',
'exif-shutterspeedvalue'           => '快門速度',
'exif-aperturevalue'               => '光圈',
'exif-brightnessvalue'             => '光度',
'exif-exposurebiasvalue'           => '曝光偏壓',
'exif-maxaperturevalue'            => '最大陸地孔徑',
'exif-subjectdistance'             => '主體距離',
'exif-meteringmode'                => '測距模式',
'exif-lightsource'                 => '光源',
'exif-flash'                       => '閃光燈',
'exif-focallength'                 => '鏡頭焦距',
'exif-focallength-format'          => '$1 毫米',
'exif-subjectarea'                 => '主體面積',
'exif-flashenergy'                 => '閃光燈能量',
'exif-spatialfrequencyresponse'    => '空間頻率響應',
'exif-focalplanexresolution'       => '焦點平面 X 嘅解像度',
'exif-focalplaneyresolution'       => '焦點平面 Y 嘅解像度',
'exif-focalplaneresolutionunit'    => '焦點平面解像度單位',
'exif-subjectlocation'             => '主題位置',
'exif-exposureindex'               => '曝光指數',
'exif-sensingmethod'               => '感知方法',
'exif-filesource'                  => '檔案來源',
'exif-scenetype'                   => '埸景類型',
'exif-cfapattern'                  => 'CFA 形式',
'exif-customrendered'              => '自訂影像處理',
'exif-exposuremode'                => '曝光模式',
'exif-whitebalance'                => '白平衡',
'exif-digitalzoomratio'            => '數碼放大比例',
'exif-focallengthin35mmfilm'       => '以35毫米菲林計嘅焦距',
'exif-scenecapturetype'            => '場景捕捉類型',
'exif-gaincontrol'                 => '場景控制',
'exif-contrast'                    => '對比',
'exif-saturation'                  => '飽和度',
'exif-sharpness'                   => '清晰度',
'exif-devicesettingdescription'    => '裝置設定描述',
'exif-subjectdistancerange'        => '物件距離範圍',
'exif-imageuniqueid'               => '影像獨有編號',
'exif-gpsversionid'                => '全球定位版本',
'exif-gpslatituderef'              => '南北緯',
'exif-gpslatitude'                 => '緯度',
'exif-gpslongituderef'             => '東西經',
'exif-gpslongitude'                => '經度',
'exif-gpsaltituderef'              => '海拔參考點',
'exif-gpsaltitude'                 => '海拔',
'exif-gpstimestamp'                => '全球定位時間（原子鐘）',
'exif-gpssatellites'               => '量度用嘅衞星',
'exif-gpsstatus'                   => '接收器狀態',
'exif-gpsmeasuremode'              => '量度模式',
'exif-gpsdop'                      => '量度準繩度',
'exif-gpsspeedref'                 => '速度單位',
'exif-gpsspeed'                    => '全球定位儀嘅速度',
'exif-gpstrackref'                 => '移動方向參考點',
'exif-gpstrack'                    => '移動方向',
'exif-gpsimgdirectionref'          => '影像方向參考點',
'exif-gpsimgdirection'             => '影像方向',
'exif-gpsmapdatum'                 => '用咗嘅大地測量資料',
'exif-gpsdestlatituderef'          => '目的地緯度參考點',
'exif-gpsdestlatitude'             => '目的地緯度',
'exif-gpsdestlongituderef'         => '目的地經度參考點',
'exif-gpsdestlongitude'            => '目的地經度',
'exif-gpsdestbearingref'           => '目的地坐向參考點',
'exif-gpsdestbearing'              => '目的地坐向',
'exif-gpsdestdistanceref'          => '目的地距離參考點',
'exif-gpsdestdistance'             => '目的地距離',
'exif-gpsprocessingmethod'         => 'GPS 處理方法名',
'exif-gpsareainformation'          => 'GPS 地區名',
'exif-gpsdatestamp'                => 'GPS 日期',
'exif-gpsdifferential'             => 'GPS 差動修正',

# EXIF attributes
'exif-compression-1' => '未壓過',

'exif-unknowndate' => '未知日期',

'exif-orientation-1' => '正常', # 0th row: top; 0th column: left
'exif-orientation-2' => '左右倒轉', # 0th row: top; 0th column: right
'exif-orientation-3' => '轉一百八十度', # 0th row: bottom; 0th column: right
'exif-orientation-4' => '上下倒轉', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '逆時針轉九十度，再上下倒轉', # 0th row: left; 0th column: top
'exif-orientation-6' => '順時針轉九十度', # 0th row: right; 0th column: top
'exif-orientation-7' => '順時針轉九十度，再上下倒轉', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '逆時針轉九十度', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'chunky 格式',
'exif-planarconfiguration-2' => 'planar 格式',

'exif-componentsconfiguration-0' => '根本無',

'exif-exposureprogram-0' => '無定義',
'exif-exposureprogram-1' => '人手',
'exif-exposureprogram-2' => '平常程序',
'exif-exposureprogram-3' => '着重光圈',
'exif-exposureprogram-4' => '着重快門',
'exif-exposureprogram-5' => '創作程序(加重景深)',
'exif-exposureprogram-6' => '動作程序(加大快門速度)',
'exif-exposureprogram-7' => '人像模式(近睇，背景矇)',
'exif-exposureprogram-8' => '風景模式(風景相，聚焦背景)',

'exif-subjectdistance-value' => '$1米',

'exif-meteringmode-0'   => '唔知',
'exif-meteringmode-1'   => '平均',
'exif-meteringmode-2'   => '中間加權平均',
'exif-meteringmode-3'   => '一點',
'exif-meteringmode-4'   => '多點',
'exif-meteringmode-5'   => '圖案',
'exif-meteringmode-6'   => '部分',
'exif-meteringmode-255' => '其他',

'exif-lightsource-0'   => '唔知',
'exif-lightsource-1'   => '日光',
'exif-lightsource-2'   => '光管',
'exif-lightsource-3'   => '燈泡(鎢絲)',
'exif-lightsource-4'   => '閃光燈',
'exif-lightsource-9'   => '晴朗',
'exif-lightsource-10'  => '有雲',
'exif-lightsource-11'  => '陰影',
'exif-lightsource-12'  => '日光螢光燈 (D 5700 – 7100K)',
'exif-lightsource-13'  => '日光白色螢光燈 (N 4600 – 5400K)',
'exif-lightsource-14'  => '冷白螢光燈 (W 3900 – 4500K)',
'exif-lightsource-15'  => '白色螢光燈 (WW 3200 – 3700K)',
'exif-lightsource-17'  => '標準光 A',
'exif-lightsource-18'  => '標準光 B',
'exif-lightsource-19'  => '標準光 C',
'exif-lightsource-24'  => 'ISO 攝影廠鎢燈',
'exif-lightsource-255' => '其它光源',

'exif-focalplaneresolutionunit-2' => '吋',

'exif-sensingmethod-1' => '無定義',
'exif-sensingmethod-2' => '單晶片色彩空間感應器',
'exif-sensingmethod-3' => '雙晶片色彩空間感應器',
'exif-sensingmethod-4' => '三晶片色彩空間感應器',
'exif-sensingmethod-5' => '連續色彩空間感應器',
'exif-sensingmethod-7' => '三綫感應器',
'exif-sensingmethod-8' => '連續色彩綫性感應器',

'exif-scenetype-1' => '一張直接映像',

'exif-customrendered-0' => '一般程序',
'exif-customrendered-1' => '度身程序',

'exif-exposuremode-0' => '自動曝光',
'exif-exposuremode-1' => '手動曝光',
'exif-exposuremode-2' => '自動曝光感知調節',

'exif-whitebalance-0' => '自動白平衡',
'exif-whitebalance-1' => '手動白平衡',

'exif-scenecapturetype-0' => '標準',
'exif-scenecapturetype-1' => '風景',
'exif-scenecapturetype-2' => '人像',
'exif-scenecapturetype-3' => '夜景',

'exif-gaincontrol-0' => '高',
'exif-gaincontrol-1' => '小增',
'exif-gaincontrol-2' => '大增',
'exif-gaincontrol-3' => '小減',
'exif-gaincontrol-4' => '大減',

'exif-contrast-0' => '平常',
'exif-contrast-1' => '軟',
'exif-contrast-2' => '硬',

'exif-saturation-0' => '平常',
'exif-saturation-1' => '低飽和',
'exif-saturation-2' => '高飽和',

'exif-sharpness-0' => '平常',
'exif-sharpness-1' => '軟',
'exif-sharpness-2' => '硬',

'exif-subjectdistancerange-0' => '唔知',
'exif-subjectdistancerange-1' => '微觀',
'exif-subjectdistancerange-2' => '近鏡',
'exif-subjectdistancerange-3' => '遠鏡',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => '北緯',
'exif-gpslatitude-s' => '南緯',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => '東經',
'exif-gpslongitude-w' => '西經',

'exif-gpsstatus-a' => '度緊',
'exif-gpsstatus-v' => '互度',

'exif-gpsmeasuremode-2' => '二維量度',
'exif-gpsmeasuremode-3' => '三維量度',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => '千米/小時',
'exif-gpsspeed-m' => '英里/小時',
'exif-gpsspeed-n' => '浬',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => '真實方向',
'exif-gpsdirection-m' => '地磁方向',

# External editor support
'edit-externally'      => '用外面程式來改呢個檔案',
'edit-externally-help' => '去[http://www.mediawiki.org/wiki/Manual:External_editors setup instructions] 睇多啲資料',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => '全部',
'imagelistall'     => '全部',
'watchlistall2'    => '全部',
'namespacesall'    => '全部',
'monthsall'        => '全部',

# E-mail address confirmation
'confirmemail'             => '確認電郵地址',
'confirmemail_noemail'     => '你唔需要響你嘅[[Special:Preferences|用戶喜好設定]]度設定一個有效嘅電郵地址。',
'confirmemail_text'        => '{{SITENAME}}需要你喺使用電郵功能之前驗證吓你嘅電郵地址。啟用下邊個掣嚟發封確認信去你個地址度。封信入面會附帶一條包含代碼嘅連結；喺你個瀏覽器度打開條連結嚟確認你嘅電郵地址係有效嘅。',
'confirmemail_pending'     => '<div class="error">一個確認碼已經電郵咗畀你；如果你係啱啱開咗個新戶口嘅，你可以響請求一個新嘅確認碼之前等多幾分鐘等佢寄畀你。</div>',
'confirmemail_send'        => '寄出確認碼。',
'confirmemail_sent'        => '確認電郵已經寄出。',
'confirmemail_oncreate'    => '一個確認碼已經寄送咗到嘅嘅電郵地址。
呢個代碼唔係登入嗰陣去用，但係你需要佢去開響呢個wiki度，任何同電郵有關嘅功能。',
'confirmemail_sendfailed'  => '發唔到確認信。請檢查吓個地址有冇無效嘅字。

郵件遞送員回應咗：$1',
'confirmemail_invalid'     => '無效嘅確認碼。個代碼可能已經過咗期。',
'confirmemail_needlogin'   => '你需要先$1去確認你嘅電郵地址。',
'confirmemail_success'     => '你嘅電郵地址已經得到確認。你而家可以[[Special:UserLogin|登入]]同盡情享受wiki啦。',
'confirmemail_loggedin'    => '你嘅電郵地址現已得到確認。',
'confirmemail_error'       => '儲存你嘅確認資料嘅時候有小小嘢發生咗意外。',
'confirmemail_subject'     => '{{SITENAME}}電郵地址確認',
'confirmemail_body'        => '有人（好有可能係嚟自你嘅IP地址 $1）已經用呢個電郵地址喺{{SITENAME}}度註冊咗帳戶"$2"

要確認呢個帳戶的而且確屬於你同埋啟用{{SITENAME}}嘅電郵功能，請喺你嘅瀏覽器度打開呢條連結：

$3

如果你係*未*註冊個戶口嘅，
請跟住呢個連結去取消電郵地址確認：

$5

呢個確認代碼會喺$4到期。',
'confirmemail_invalidated' => '電郵地址確認取消咗',
'invalidateemail'          => '取消電郵確認',

# Scary transclusion
'scarytranscludedisabled' => '[跨 wiki 滲漏正停用]',
'scarytranscludefailed'   => '[$1嘅頡取模動作失敗]',
'scarytranscludetoolong'  => '[URL 太長]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">呢一版嘅過去追蹤：
$1
</div>',
'trackbackremove'   => ' ([$1 刪除])',
'trackbacklink'     => '過去追蹤',
'trackbackdeleteok' => '過去追蹤已經成功噉樣刪除。',

# Delete conflict
'deletedwhileediting' => '警告：你寫緊文嗰陣，有用戶洗咗呢版！',
'confirmrecreate'     => "你寫緊文嗰陣，阿用戶 [[User:$1|$1]] ([[User talk:$1|talk]]) 洗咗呢一頁。以下係佢個理由：
: ''$2''
請確認你係咪真係想重新整過呢版。",
'recreate'            => '重新整過',

# HTML dump
'redirectingto' => '跳轉去[[:$1]]...',

# action=purge
'confirm_purge'        => '肯定要洗咗呢版個快取版本？

$1',
'confirm_purge_button' => '肯定',

# AJAX search
'searchcontaining' => "搵含有''$1''嘅版。",
'searchnamed'      => "搵個名係''$1''嘅版。",
'articletitles'    => "以''$1''開頭嘅版",
'hideresults'      => '收埋結果',
'useajaxsearch'    => '用AJAX來搵嘢',

# Multipage image navigation
'imgmultipageprev' => '← 上一版',
'imgmultipagenext' => '下一版 →',
'imgmultigo'       => '去!',
'imgmultigoto'     => '去第$1版',

# Table pager
'ascending_abbrev'         => '增',
'descending_abbrev'        => '減',
'table_pager_next'         => '下一版',
'table_pager_prev'         => '上一版',
'table_pager_first'        => '第一版',
'table_pager_last'         => '最後一版',
'table_pager_limit'        => '每一版顯示$1個項目',
'table_pager_limit_submit' => '去',
'table_pager_empty'        => '無結果',

# Auto-summaries
'autosumm-blank'   => '移除緊響嗰一版嘅全部內容',
'autosumm-replace' => "用 '$1' 取代緊嗰一版",
'autoredircomment' => '跳緊轉到[[$1]]',
'autosumm-new'     => '新頁： $1',

# Live preview
'livepreview-loading' => '載入緊…',
'livepreview-ready'   => '載入緊… 預備好！',
'livepreview-failed'  => '實時預覽失敗！ 試吓標準預覽。',
'livepreview-error'   => '連接失敗： $1 "$2" 試吓標準預覽。',

# Friendlier slave lag warnings
'lag-warn-normal' => '新過$1秒嘅更改可能唔會響呢個表度顯示。',
'lag-warn-high'   => '由於資料庫嘅過度延遲，新過$1秒嘅更改可能唔會響呢個表度顯示。',

# Watchlist editor
'watchlistedit-numitems'       => '你嘅監視清單總共有$1個標題，當中唔包括對話版。',
'watchlistedit-noitems'        => '你嘅監視清單並無標題。',
'watchlistedit-normal-title'   => '編輯監視清單',
'watchlistedit-normal-legend'  => '響監視清單度拎走',
'watchlistedit-normal-explain' => '響你張監視清單度嘅標題響下面度顯示。要拎走一個標題，響佢前面剔一剔，跟住要撳拎走標題。你亦都可以[[Special:Watchlist/raw|編輯原始清單]]。',
'watchlistedit-normal-submit'  => '拎走標題',
'watchlistedit-normal-done'    => '$1個標題已經響你嘅監視清單度拎走咗:',
'watchlistedit-raw-title'      => '編輯原始監視清單',
'watchlistedit-raw-legend'     => '編輯原始監視清單',
'watchlistedit-raw-explain'    => '你張監視清單嘅標題響下面度顯示，同時亦都可以透過編輯呢個表去加入同埋拎走標題；一行一個標題。當完成咗之後，撳更新監視清單。你亦都可以去用[[Special:Watchlist/edit|標準編輯器]]。',
'watchlistedit-raw-titles'     => '標題:',
'watchlistedit-raw-submit'     => '更新監視清單',
'watchlistedit-raw-done'       => '你嘅監視清單已經更新。',
'watchlistedit-raw-added'      => '已經加入咗$1個標題:',
'watchlistedit-raw-removed'    => '已經拎走咗$1個標題:',

# Watchlist editing tools
'watchlisttools-view' => '睇吓有關嘅更改',
'watchlisttools-edit' => '睇吓同埋編輯監視清單',
'watchlisttools-raw'  => '編輯原始監視清單',

# Core parser functions
'unknown_extension_tag' => '未知嘅擴展標籤 "$1"',

# Special:Version
'version'                          => '版本', # Not used as normal message but as header for the special page itself
'version-extensions'               => '裝咗嘅擴展',
'version-specialpages'             => '特別頁',
'version-parserhooks'              => '語法鈎',
'version-variables'                => '變數',
'version-other'                    => '其它',
'version-mediahandlers'            => '媒體處理器',
'version-hooks'                    => '鈎',
'version-extension-functions'      => '擴展函數',
'version-parser-extensiontags'     => '語法擴展標籤',
'version-parser-function-hooks'    => '語法函數鈎',
'version-skin-extension-functions' => '畫面擴展函數',
'version-hook-name'                => '鈎名',
'version-hook-subscribedby'        => '利用於',
'version-version'                  => '版本',
'version-license'                  => '牌照',
'version-software'                 => '裝咗嘅軟件',
'version-software-product'         => '產品',
'version-software-version'         => '版本',

# Special:FilePath
'filepath'         => '檔案路徑',
'filepath-page'    => '檔名:',
'filepath-submit'  => '搵路徑',
'filepath-summary' => '呢個特別頁拎一個檔案嘅完整路徑。圖像會以完整嘅解像度顯示，其它嘅檔案類型會以同佢哋關聯咗嘅程式啟動。

請輸入檔名，唔好連埋個"{{ns:image}}:"開頭。',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => '㨂重覆檔案',
'fileduplicatesearch-summary'  => '用重覆檔案嘅切細值去搵個檔案係唔係重覆。

輸入檔名嗰陣唔使輸入 "{{ns:image}}:" 開頭。',
'fileduplicatesearch-legend'   => '搵重覆',
'fileduplicatesearch-filename' => '檔名:',
'fileduplicatesearch-submit'   => '搵',
'fileduplicatesearch-info'     => '$1 × $2 像素<br />檔案大細: $3<br />MIME類型: $4',
'fileduplicatesearch-result-1' => '個檔案 "$1" 無完全相同嘅重覆。',
'fileduplicatesearch-result-n' => '個檔案 "$1" 有$2項完全相同嘅重覆。',

# Special:SpecialPages
'specialpages'                   => '特別頁',
'specialpages-note'              => '----
* 標準特別頁。
* <span class="mw-specialpagerestricted">有限制嘅特別頁。</span>',
'specialpages-group-maintenance' => '維護報告',
'specialpages-group-other'       => '其它特別頁',
'specialpages-group-login'       => '登入／開戶口',
'specialpages-group-changes'     => '最近更改同日誌',
'specialpages-group-media'       => '媒體報告同上載',
'specialpages-group-users'       => '用戶同權限',
'specialpages-group-highuse'     => '高度使用頁',
'specialpages-group-pages'       => '頁面一覽',
'specialpages-group-pagetools'   => '版工具',
'specialpages-group-wiki'        => 'Wiki資料同工具',
'specialpages-group-redirects'   => '跳轉特別頁',
'specialpages-group-spam'        => '反垃圾工具',

# Special:BlankPage
'blankpage'              => '空白頁',
'intentionallyblankpage' => '呢一版係留空咗嘅，用來作測速等用嘅',

);
