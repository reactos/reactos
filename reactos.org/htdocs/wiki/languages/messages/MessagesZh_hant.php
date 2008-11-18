<?php
/** Traditional Chinese (‪中文(繁體)‬)
 *
 * @ingroup Language
 * @file
 *
 * @author Alexsh
 * @author Shinjiman
 */

$fallback = 'zh-hans';

$namespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Special',
	NS_MAIN             => '',
	NS_TALK             => 'Talk',
	NS_USER             => 'User',
	NS_USER_TALK        => 'User_talk',
	# NS_PROJECT set by $wgMetaNamespace
	NS_PROJECT_TALK     => '$1_talk',
	NS_IMAGE            => 'Image',
	NS_IMAGE_TALK       => 'Image_talk',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_talk',
	NS_TEMPLATE         => 'Template',
	NS_TEMPLATE_TALK    => 'Template_talk',
	NS_HELP             => 'Help',
	NS_HELP_TALK        => 'Help_talk',
	NS_CATEGORY         => 'Category',
	NS_CATEGORY_TALK    => 'Category_talk'
);

$namespaceAliases = array(
	"媒體" => NS_MEDIA,
	"特殊" => NS_SPECIAL,
	"對話" => NS_TALK,
	"用戶" => NS_USER,
	"用戶對話" => NS_USER_TALK,
	# This has never worked so it's unlikely to annoy anyone if I disable it -- TS
	#"{{SITENAME}}_對話" => NS_PROJECT_TALK
	"圖像" => NS_IMAGE,
	"圖像對話" => NS_IMAGE_TALK,
	"樣板" => NS_TEMPLATE,
	"樣板討論" => NS_TEMPLATE_TALK,
	"幫助" => NS_HELP,
	"幫助討論" => NS_HELP_TALK,
	"分類" => NS_CATEGORY,
	"分類討論" => NS_CATEGORY_TALK,
);

$skinNames = array(
	'standard' => '標準',
	'nostalgia' => '懷舊',
	'cologneblue' => '科隆香水藍',
	'modern' => '現代',
);

$bookstoreList = array(
	'博客來書店' => 'http://www.books.com.tw/exep/prod/booksfile.php?item=$1',
	'三民書店' => 'http://www.sanmin.com.tw/page-qsearch.asp?ct=search_isbn&qu=$1',
	'天下書店' => 'http://www.cwbook.com.tw/search/result1.jsp?field=2&keyWord=$1',
	'新絲路書店' => 'http://www.silkbook.com/function/Search_list_book_data.asp?item=5&text=$1'
);

$messages = array(
# User preference toggles
'tog-underline'               => '下劃鏈結',
'tog-highlightbroken'         => '毀壞的鏈結格式<a href="" class="new">像這樣</a> (或者像這個<a href="" class="internal">?</a>)',
'tog-justify'                 => '段落對齊',
'tog-hideminor'               => '最近更改中隱藏小修改',
'tog-extendwatchlist'         => '增強監視清單以顯示所有可用更改',
'tog-usenewrc'                => '增強最近更改 (JavaScript)',
'tog-numberheadings'          => '標題自動編號',
'tog-showtoolbar'             => '顯示編輯工具欄',
'tog-editondblclick'          => '雙擊編輯頁面 (JavaScript)',
'tog-editsection'             => '允許通過點擊[編輯]連結編輯段落',
'tog-editsectiononrightclick' => '允許右擊標題編輯段落 (JavaScript)',
'tog-showtoc'                 => '顯示目錄 (針對一頁超過3個標題的頁面)',
'tog-rememberpassword'        => '在這部電腦上記住我的密碼',
'tog-editwidth'               => '編輯欄位已有最大寬度',
'tog-watchcreations'          => '將我創建的頁面添加到我的監視列表中',
'tog-watchdefault'            => '將我更改的頁面添加到我的監視列表中',
'tog-watchmoves'              => '將我移動的頁面加入我的監視列表',
'tog-watchdeletion'           => '將我刪除的頁面加入我的監視列表',
'tog-minordefault'            => '預設將編輯設定為小編輯',
'tog-previewontop'            => '在編輯框上方顯示預覽',
'tog-previewonfirst'          => '第一次編輯時顯示原文內容的預覽',
'tog-nocache'                 => '停用頁面快取',
'tog-enotifwatchlistpages'    => '當在我的監視列表中的頁面改變時發電子郵件給我',
'tog-enotifusertalkpages'     => '當我的對話頁發生改變時發電子郵件給我',
'tog-enotifminoredits'        => '即使是頁面的小修改也向我發電子郵件',
'tog-enotifrevealaddr'        => '在通知電子郵件中顯示我的電子郵件位址',
'tog-shownumberswatching'     => '顯示監視用戶的數目',
'tog-fancysig'                => '使用原始簽名 (不產生自動連結)',
'tog-externaleditor'          => '預設使用外部編輯器 (進階者專用，需要在您的電腦上作出一些特別設定)',
'tog-externaldiff'            => '預設使用外部差異分析 (進階者專用，需要在您的電腦上作出一些特別設定)',
'tog-showjumplinks'           => '啟用「跳轉到」訪問連結',
'tog-uselivepreview'          => '使用實時預覽 (JavaScript) (試驗中)',
'tog-forceeditsummary'        => '當沒有輸入摘要時提醒我',
'tog-watchlisthideown'        => '監視列表中隱藏我的編輯',
'tog-watchlisthidebots'       => '監視列表中隱藏機器人的編輯',
'tog-watchlisthideminor'      => '監視列表中隱藏小修改',
'tog-nolangconversion'        => '不進行用字轉換',
'tog-ccmeonemails'            => '當我寄電子郵件給其他用戶時，也寄一份複本到我的信箱。',
'tog-diffonly'                => '在比較兩個修訂版本差異時不顯示頁面內容',
'tog-showhiddencats'          => '顯示隱藏分類',

'underline-always'  => '總是使用',
'underline-never'   => '從不使用',
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
'pagecategories'                 => '$1個分類',
'category_header'                => '類別「$1」中的頁面',
'subcategories'                  => '附分類',
'category-media-header'          => '"$1"分類中的媒體',
'category-empty'                 => "''這個分類中尚未包含任何頁面或媒體。''",
'hidden-categories'              => '$1個隱藏分類',
'hidden-category-category'       => '隱藏分類', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|這個分類中只有以下的附分類。|這個分類中有以下的$1個附分類，共有$2個附分類。}}',
'category-subcat-count-limited'  => '這個分類下有$1個子分類。',
'category-article-count'         => '{{PLURAL:$2|這個分類中只有以下的頁面。|這個分類中有以下的$1個頁面，共有$2個頁面。}}',
'category-article-count-limited' => '這個分類下有$1個頁面。',
'category-file-count'            => '{{PLURAL:$2|這個分類中只有以下的檔案。|這個分類中有以下的$1個檔案，共有$2個檔案。}}',
'category-file-count-limited'    => '這個分類下有$1個檔案。',
'listingcontinuesabbrev'         => '續',

'mainpagetext'      => "<big>'''已成功安裝 MediaWiki!'''</big>",
'mainpagedocfooter' => '請參閱 [http://meta.wikimedia.org/wiki/Help:Contents 用戶手冊] 以獲得使用此 wiki 軟體的訊息！

== 入門 ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings MediaWiki 配置設定清單]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki 常見問題解答]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki 發佈郵件清單]',

'about'          => '關於',
'article'        => '頁面',
'newwindow'      => '(在新視窗中打開)',
'cancel'         => '取消',
'qbfind'         => '尋找',
'qbbrowse'       => '瀏覽',
'qbedit'         => '編輯',
'qbpageoptions'  => '頁面選項',
'qbpageinfo'     => '頁面訊息',
'qbmyoptions'    => '我的選項',
'qbspecialpages' => '特殊頁面',
'moredotdotdot'  => '更多...',
'mypage'         => '我的頁面',
'mytalk'         => '我的對話頁',
'anontalk'       => '該IP的對話頁',
'navigation'     => '導航',
'and'            => '和',

# Metadata in edit box
'metadata_help' => '元數據:',

'errorpagetitle'    => '錯誤',
'returnto'          => '返回到$1。',
'tagline'           => '出自{{SITENAME}}',
'help'              => '幫助',
'search'            => '搜索',
'searchbutton'      => '搜索',
'go'                => '進入',
'searcharticle'     => '進入',
'history'           => '頁面歷史',
'history_short'     => '歷史',
'updatedmarker'     => '我上次訪問以來的修改',
'info_short'        => '資訊',
'printableversion'  => '可列印版',
'permalink'         => '永久連結',
'print'             => '列印',
'edit'              => '編輯',
'create'            => '建立',
'editthispage'      => '編輯本頁',
'create-this-page'  => '建立本頁',
'delete'            => '刪除',
'deletethispage'    => '刪除本頁',
'undelete_short'    => '反刪除$1項修訂',
'protect'           => '保護',
'protect_change'    => '更改',
'protectthispage'   => '保護本頁',
'unprotect'         => '解除保護',
'unprotectthispage' => '解除此頁保護',
'newpage'           => '新頁面',
'talkpage'          => '討論本頁',
'talkpagelinktext'  => '對話',
'specialpage'       => '特殊頁面',
'personaltools'     => '個人工具',
'postcomment'       => '發表評論',
'articlepage'       => '查看頁面',
'talk'              => '討論',
'views'             => '檢視',
'toolbox'           => '工具箱',
'userpage'          => '查看用戶頁面',
'projectpage'       => '查看計劃頁面',
'imagepage'         => '查看媒體頁面',
'mediawikipage'     => '檢視使用者介面訊息',
'templatepage'      => '檢視模板頁面',
'viewhelppage'      => '檢視說明頁面',
'categorypage'      => '檢視分類頁面',
'viewtalkpage'      => '檢視討論頁面',
'otherlanguages'    => '其它語言',
'redirectedfrom'    => '(重定向自$1)',
'redirectpagesub'   => '重定向頁面',
'lastmodifiedat'    => '這頁的最後修訂在 $1 $2。', # $1 date, $2 time
'viewcount'         => '本頁面已經被瀏覽$1次。',
'protectedpage'     => '被保護頁',
'jumpto'            => '跳轉到:',
'jumptonavigation'  => '導航',
'jumptosearch'      => '搜索',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '關於{{SITENAME}}',
'aboutpage'            => 'Project:關於',
'bugreports'           => '錯誤報告',
'bugreportspage'       => 'Project:錯誤報告',
'copyright'            => '本站的全部文本內容在$1之條款下提供。',
'copyrightpagename'    => '{{SITENAME}}版權',
'copyrightpage'        => '{{ns:project}}:版權訊息',
'currentevents'        => '現時事件',
'currentevents-url'    => 'Project:現時事件',
'disclaimers'          => '免責聲明',
'disclaimerpage'       => 'Project:一般免責聲明',
'edithelp'             => '編輯幫助',
'edithelppage'         => 'Help:如何編輯頁面',
'faq'                  => '常見問題解答',
'faqpage'              => 'Project:常見問題解答',
'helppage'             => 'Help:目錄',
'mainpage'             => '首頁',
'mainpage-description' => '首頁',
'policy-url'           => 'Project:方針',
'portal'               => '社區',
'portal-url'           => 'Project:社區',
'privacy'              => '隱私政策',
'privacypage'          => 'Project:隱私政策',

'badaccess'        => '權限錯誤',
'badaccess-group0' => '您剛才的請求不允許執行。',
'badaccess-group1' => '您剛才的請求只有$1用戶組的用戶才能使用。',
'badaccess-group2' => '您剛才的請求只有$1用戶組的用戶才能使用。',
'badaccess-groups' => '您剛才的請求只有$1用戶組的用戶才能使用。',

'versionrequired'     => '需要MediaWiki $1 版',
'versionrequiredtext' => '需要版本$1的 MediaWiki 才能使用此頁。參見[[Special:Version|版本頁]]。',

'ok'                      => '確定',
'retrievedfrom'           => '取自"$1"',
'youhavenewmessages'      => '您有$1（$2）。',
'newmessageslink'         => '新訊息',
'newmessagesdifflink'     => '上次更改',
'youhavenewmessagesmulti' => '您在 $1 有一條新訊息',
'editsection'             => '編輯',
'editold'                 => '編輯',
'viewsourceold'           => '檢視原始碼',
'editsectionhint'         => '編輯段落: $1',
'toc'                     => '目錄',
'showtoc'                 => '顯示',
'hidetoc'                 => '隱藏',
'thisisdeleted'           => '查看或恢復$1?',
'viewdeleted'             => '檢視$1',
'restorelink'             => '$1個被刪除的版本',
'feedlinks'               => '訂閱:',
'feed-invalid'            => '無效的訂閱類型。',
'feed-unavailable'        => '聯合訂閱並無於{{SITENAME}}上提供',
'site-rss-feed'           => '$1的RSS訂閱',
'site-atom-feed'          => '$1的Atom訂閱',
'page-rss-feed'           => '「$1」的RSS訂閱',
'page-atom-feed'          => '「$1」的Atom訂閱',
'red-link-title'          => '$1 (尚未撰寫)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '頁面',
'nstab-user'      => '用戶頁面',
'nstab-media'     => '媒體頁面',
'nstab-special'   => '特殊頁面',
'nstab-project'   => '計劃頁面',
'nstab-image'     => '檔案',
'nstab-mediawiki' => '介面',
'nstab-template'  => '模板',
'nstab-help'      => '幫助頁面',
'nstab-category'  => '分類',

# Main script and global functions
'nosuchaction'      => '這個命令不存在',
'nosuchactiontext'  => '這個wiki無法識別URL請求的命令',
'nosuchspecialpage' => '此特殊頁面不存在',
'nospecialpagetext' => "<big>'''您請求的特殊頁面無效。'''</big>

[[Special:SpecialPages|{{int:specialpages}}]]中載有所有有效特殊頁面的列表。",

# General errors
'error'                => '錯誤',
'databaseerror'        => '資料庫錯誤',
'dberrortext'          => '發生資料庫查詢語法錯誤。
可能是由於軟體自身的錯誤所引起。
最後一次資料庫查詢指令是:
<blockquote><tt>$1</tt></blockquote>
來自於函數 "<tt>$2</tt>"。
MySQL返回錯誤 "<tt>$3: $4</tt>"。',
'dberrortextcl'        => '發生了一個資料庫查詢語法錯誤。
最後一次的資料庫查詢是:
「$1」
來自於函數「$2」。
MySQL返回錯誤「$3: $4」。',
'noconnect'            => '抱歉！網站遇到一些技術問題，無法連接數據庫伺服器。<br />
$1',
'nodb'                 => '無法選擇資料庫 $1',
'cachederror'          => '以下頁面是快取中的副本，未必是最新版本。',
'laggedslavemode'      => '警告: 頁面可能不包含最近的更新。',
'readonly'             => '資料庫禁止訪問',
'enterlockreason'      => '請輸入禁止訪問原因, 包括估計重新開放的時間',
'readonlytext'         => '資料庫目前禁止輸入新內容及更改，
這很可能是由於資料庫正在維修，之後即可恢復。
管理員有如下解釋: $1',
'missing-article'      => '資料庫找不到文字"$1" $2。

<p>通常這是由於修訂歷史頁上過時的連結到已經被刪除的頁面所導致的。</p>

<p>如果情況不是這樣，您可能找到了軟體內的一個臭蟲。
請記錄下URL地址，並向[[Special:ListUsers/sysop|管理員]]報告。</p>',
'missingarticle-rev'   => '(修訂#: $1)',
'missingarticle-diff'  => '(差異: $1, $2)',
'readonly_lag'         => '附屬資料庫伺服器正在將快取更新到主伺服器，資料庫已被自動鎖定',
'internalerror'        => '內部錯誤',
'internalerror_info'   => '內部錯誤: $1',
'filecopyerror'        => '無法複製檔案"$1"到"$2"。',
'filerenameerror'      => '無法重新命名檔案"$1"到"$2"。',
'filedeleteerror'      => '無法刪除檔案"$1"。',
'directorycreateerror' => '無法建立目錄"$1"。',
'filenotfound'         => '找不到檔案"$1"。',
'fileexistserror'      => '無法寫入檔案"$1": 檔案已存在',
'unexpected'           => '不正常值："$1"="$2"。',
'formerror'            => '錯誤：無法提交表單',
'badarticleerror'      => '無法在本頁上進行此項操作。',
'cannotdelete'         => '無法刪除選定的頁面或圖像（它可能已經被其他人刪除了）。',
'badtitle'             => '錯誤的標題',
'badtitletext'         => '所請求頁面的標題是無效的、不存在，跨語言或跨wiki連結的標題錯誤。',
'perfdisabled'         => '抱歉！由於此項操作有可能造成資料庫癱瘓，目前暫時無法使用。',
'perfcached'           => '下列是快取資料，因此可能不是最新的:',
'perfcachedts'         => '下列是快取資料，其最後更新時間是$1。',
'querypage-no-updates' => '當前禁止對此頁面進行更新。此處的資料將不能被立即重新整理。',
'wrong_wfQuery_params' => '錯誤的參數導致wfQuery()<br />函數：$1<br />查詢：$2',
'viewsource'           => '原始碼',
'viewsourcefor'        => '$1的原始碼',
'actionthrottled'      => '動作已壓制',
'actionthrottledtext'  => '基於反垃圾的考量，您現在於這段短時間之中限制去作這一個動作，而您已經超過這個上限。請在數分鐘後再嘗試。',
'protectedpagetext'    => '該頁面已被鎖定以防止編輯。',
'viewsourcetext'       => '你可以檢視並複製本頁面的原始碼。',
'protectedinterface'   => '該頁提供了軟體的介面文字，它已被鎖定以防止隨意的修改。',
'editinginterface'     => "'''警告:''' 您正在編輯的頁面是用於提供軟體的介面文字。改變此頁將影響其他用戶的介面外觀。如要翻譯，請考慮使用[http://translatewiki.net/wiki/Main_Page?setlang=zh-hant Betawiki]，一個用來為MediaWiki軟件本地化的計劃。",
'sqlhidden'            => '(隱藏SQL查詢)',
'cascadeprotected'     => '這個頁面已經被保護，因為這個頁面被以下已標註"聯鎖保護"的{{PLURAL:$1|一個|多個}}被保護頁面包含:
$2',
'namespaceprotected'   => "您並沒有權限編輯'''$1'''名字空間的頁面。",
'customcssjsprotected' => '您並無許可權去編輯這個頁面，因為它包含了另一位用戶的個人設定。',
'ns-specialprotected'  => '特殊頁面是不可以編輯的。',
'titleprotected'       => '這個標題已經被[[User:$1|$1]]保護以防止建立。理由是<i>$2</i>。',

# Virus scanner
'virus-badscanner'     => "損壞設定: 未知的病毒掃瞄器: ''$1''",
'virus-scanfailed'     => '掃瞄失敗 (代碼 $1)',
'virus-unknownscanner' => '未知的防病毒:',

# Login and logout pages
'logouttitle'                => '用戶退出',
'logouttext'                 => '您現在已經退出。
您可以繼續以匿名方式使用{{SITENAME}}，或再次以相同或不同用戶身份[[Special:UserLogin|登入]]。',
'welcomecreation'            => '<h2>歡迎，$1!</h2><p>您的帳號已經建立，不要忘記設置[[Special:Preferences|{{SITENAME}}個人參數]]。</p>',
'loginpagetitle'             => '用戶登入',
'yourname'                   => '您的用戶名:',
'yourpassword'               => '您的密碼:',
'yourpasswordagain'          => '再次輸入密碼:',
'remembermypassword'         => '下次登入記住密碼。',
'yourdomainname'             => '您的網域:',
'externaldberror'            => '這可能是由於外部驗證資料庫錯誤或您被禁止更新您的外部帳號。',
'loginproblem'               => '<b>登入有問題。</b><br />再試一次！',
'login'                      => '登入',
'nav-login-createaccount'    => '登入／建立新帳號',
'loginprompt'                => '您必須允許瀏覽器紀錄Cookie才能成功登入 {{SITENAME}} 並順利進行操作',
'userlogin'                  => '登入／建立新帳號',
'logout'                     => '登出',
'userlogout'                 => '登出',
'notloggedin'                => '未登入',
'nologin'                    => '您還沒有帳號嗎？$1。',
'nologinlink'                => '建立新帳號',
'createaccount'              => '建立新帳號',
'gotaccount'                 => '已經擁有帳號？$1。',
'gotaccountlink'             => '登入',
'createaccountmail'          => '通過e-Mail',
'badretype'                  => '您所輸入的密碼並不相同。',
'userexists'                 => '您所輸入的用戶名稱已經存在，請另選一個名稱。',
'youremail'                  => '電子郵件:',
'username'                   => '用戶名:',
'uid'                        => '用戶ID:',
'prefs-memberingroups'       => '{{PLURAL:$1|一|多}}組的成員:',
'yourrealname'               => '真實姓名:',
'yourlanguage'               => '介面語言:',
'yourvariant'                => '字體變換:',
'yournick'                   => '簽名:',
'badsig'                     => '錯誤的原始簽名。檢查一下HTML標籤。',
'badsiglength'               => '簽名過長。
它的長度必須在$1個字元以下。',
'email'                      => '電子郵件',
'prefs-help-realname'        => '真實姓名是可選的。
如果您選擇提供它，那它便用以對您的貢獻署名。',
'loginerror'                 => '登入錯誤',
'prefs-help-email'           => '電子郵件是可選的，但當您忘記您的個密碼時可以將新密碼寄回給您。您亦可以在您沒有公開自己的用戶身分時透過您的用戶頁或用戶討論頁與您聯繫。',
'prefs-help-email-required'  => '需要電子郵件地址。',
'nocookiesnew'               => '已成功創建新賬戶！偵測到您已關閉 Cookies，請開啟它並登入。',
'nocookieslogin'             => '本站利用 Cookies 進行用戶登入，偵測到您已關閉 Cookies，請開啟它並重新登入。',
'noname'                     => '你沒有輸入一個有效的用戶名。',
'loginsuccesstitle'          => '登入成功',
'loginsuccess'               => '你現在以 "$1"的身份登入{{SITENAME}}。',
'nosuchuser'                 => '找不到用戶 "$1"。
檢查您的拼寫，或者用下面的表格[[Special:Userlogin/signup|建立一個新帳號]]。',
'nosuchusershort'            => '沒有一個名為「<nowiki>$1</nowiki>」的用戶。請檢查您輸入的文字是否有錯誤。',
'nouserspecified'            => '你需要指定一個用戶名。',
'wrongpassword'              => '您輸入的密碼錯誤，請再試一次。',
'wrongpasswordempty'         => '沒有輸入密碼！請重試。',
'passwordtooshort'           => '您的密碼不正確或太短，不能少於$1個字元，而且必須跟用戶名不同。',
'mailmypassword'             => '將新密碼寄給我',
'passwordremindertitle'      => '{{SITENAME}}的新臨時密碼',
'passwordremindertext'       => '有人(可能是您，來自IP位址$1)已請求{{SITENAME}}的新密碼 ($4)。
用戶"$2"的一個新臨時密碼現在已被設定好為"$3"。
如果這個動作是您所指示的，您便需要立即登入並選擇一個新的密碼。

如果是其他人發出了該請求，或者您已經記起了您的密碼並不準備改變它，
您可以忽略此消息並繼續使用您的舊密碼。',
'noemail'                    => '用戶"$1"沒有登記電子郵件地址。',
'passwordsent'               => '用戶"$1"的新密碼已經寄往所登記的電子郵件地址。
請在收到後再登入。',
'blocked-mailpassword'       => '由於這個用戶被封禁，我們暫時禁止您請求申請新密碼。造成不便敬請見諒',
'eauthentsent'               => '一封確認信已經發送到所示的地址。在發送其它郵件到此帳戶前，您必須首先依照這封信中的指導確認這個電子郵件信箱真實有效。',
'throttled-mailpassword'     => '密碼提醒已經在前$1小時內發送。為防止濫用，限定在$1小時內僅發送一次密碼提醒。',
'mailerror'                  => '發送郵件錯誤: $1',
'acct_creation_throttle_hit' => '對不起，您已經註冊了$1帳號。你不能再註冊了。',
'emailauthenticated'         => '您的電子郵件地址已經於$1確認有效。',
'emailnotauthenticated'      => '您的郵箱位址<strong>還沒被認証</strong>。以下功能將不會發送任何郵件。',
'noemailprefs'               => '<strong>指定一個電子郵件地址以使用此功能</strong>',
'emailconfirmlink'           => '確認您的郵箱地址',
'invalidemailaddress'        => '郵箱地址格式不正確，請輸入正確的郵箱位址或清空該輸入框。',
'accountcreated'             => '已建立帳戶',
'accountcreatedtext'         => '$1的帳戶已經被創建。',
'createaccount-title'        => '在{{SITENAME}}中創建新帳戶',
'createaccount-text'         => '有人在{{SITENAME}}中利用您的電郵創建了一個名為 "$2" 的新帳戶($4)，密碼是 "$3" 。您應該立即登入並更改密碼。

如果該帳戶創建錯誤的話，您可以忽略此信息。',
'loginlanguagelabel'         => '語言: $1',

# Password reset dialog
'resetpass'               => '重設帳戶密碼',
'resetpass_announce'      => '您是透過一個臨時的發送到郵件中的代碼登入的。要完成登入，您必須在這裡設定一個新密碼:',
'resetpass_text'          => '<!-- 在此處加入文字 -->',
'resetpass_header'        => '重設密碼',
'resetpass_submit'        => '設定密碼並登入',
'resetpass_success'       => '您的密碼已經被成功更改﹗現下正為您登入...',
'resetpass_bad_temporary' => '無效的臨時密碼。您可能已成功地更改了您的密碼，或者需要請求一個新的臨時密碼。',
'resetpass_forbidden'     => '無法在{{SITENAME}}上更改密碼',
'resetpass_missing'       => '無表單資料。',

# Edit page toolbar
'bold_sample'     => '粗體文字',
'bold_tip'        => '粗體文字',
'italic_sample'   => '斜體文字',
'italic_tip'      => '斜體文字',
'link_sample'     => '連結標題',
'link_tip'        => '內部連結',
'extlink_sample'  => 'http://www.example.com 連結標題',
'extlink_tip'     => '外部連結(加前綴 http://)',
'headline_sample' => '大標題文字',
'headline_tip'    => '2級標題文字',
'math_sample'     => '在此插入數學公式',
'math_tip'        => '插入數學公式 (LaTeX)',
'nowiki_sample'   => '在此插入非格式文字',
'nowiki_tip'      => '插入非格式文字',
'image_tip'       => '嵌入檔案',
'media_tip'       => '檔案連結',
'sig_tip'         => '帶有時間的簽名',
'hr_tip'          => '水平線 (小心使用)',

# Edit pages
'summary'                          => '摘要',
'subject'                          => '主題',
'minoredit'                        => '這是一個小修改',
'watchthis'                        => '監視本頁',
'savearticle'                      => '保存本頁',
'preview'                          => '預覽',
'showpreview'                      => '顯示預覽',
'showlivepreview'                  => '即時預覽',
'showdiff'                         => '顯示差異',
'anoneditwarning'                  => "'''警告:'''您沒有登錄，您的IP位址將記錄在此頁的編輯歷史中。",
'missingsummary'                   => "'''提示:''' 您沒有提供一個編輯摘要。如果您再次單擊儲存，您的編輯將不帶編輯摘要儲存。",
'missingcommenttext'               => '請在下面輸入評論。',
'missingcommentheader'             => "'''提示:''' 您沒有為此評論提供一個標題。如果您再次單擊儲存，您的編輯將不帶標題儲存。",
'summary-preview'                  => '摘要預覽',
'subject-preview'                  => '主題/標題預覽',
'blockedtitle'                     => '用戶被查封',
'blockedtext'                      => "<big>你的用戶名或IP地址已經被$1查封。</big>

這次查封是由$1所封的。當中的原因是''$2''。

* 這次查封開始的時間是：$8
* 這次查封到期的時間是：$6
* 對於被查封者：$7

你可以聯絡$1或者其他的[[{{MediaWiki:Grouppage-sysop}}|管理員]]，討論這次查封。
除非你已經在你的[[Special:Preferences|帳號參數設置]]中設定了一個有效的電子郵件地址，否則你是不能使用「電郵這位用戶」的功能。當設定了一個有效的電子郵件地址後，這個功能是不會封鎖的。

你目前的IP地址是$3，而該查封ID是 #$5。 請在你的查詢中註明以上所有的資料。",
'autoblockedtext'                  => "你的IP地址已經被自動查封，由於先前的另一位用戶被$1所查封。
而查封的原因是：

:''$2''

* 這次查封的開始時間是：$8
* 這次查封的到期時間是：$6
* 對於被查封者：$7

你可以聯絡$1或者其他的[[{{MediaWiki:Grouppage-sysop}}|管理員]]，討論這次查封。
除非你已經在你的[[Special:Preferences|帳號參數設置]]中設定了一個有效的電子郵件地址，否則你是不能使用「電郵這位用戶」的功能。當設定了一個有效的電子郵件地址後，這個功能是不會封鎖的。

您現時正在使用的 IP 地址是 $3，查封ID是 #$5。 請在你的查詢中註明以上所有的資料。",
'blockednoreason'                  => '無給出原因',
'blockedoriginalsource'            => "以下是'''$1'''的原始碼:",
'blockededitsource'                => "你對'''$1'''進行'''編輯'''的文字如下:",
'whitelistedittitle'               => '登入後才可編輯',
'whitelistedittext'                => '您必須先$1才可編輯頁面。',
'confirmedittitle'                 => '郵件確認後才可編輯',
'confirmedittext'                  => '在編輯此頁之前您必須確認您的郵箱位址。請透過[[Special:Preferences|參數設定]]設定並驗証您的郵箱位址。',
'nosuchsectiontitle'               => '沒有這個段落',
'nosuchsectiontext'                => '您嘗試編輯的段落並不存在。在這裡是無第$1個段落，所以是沒有一個地方去儲存你的編輯。',
'loginreqtitle'                    => '需要登入',
'loginreqlink'                     => '登入',
'loginreqpagetext'                 => '您必須$1才能檢視其它頁面。',
'accmailtitle'                     => '密碼已寄出',
'accmailtext'                      => "'$1'的密碼已經寄到$2。",
'newarticle'                       => '(新)',
'newarticletext'                   => '您進入了一個尚未創建的頁面。
要創建該頁面，請在下面的編輯框中輸入內容(詳情參見[[Help:幫助|幫助]])。
如果您是不小心來到此頁面，直接點擊您瀏覽器中的"返回"按鈕返回。',
'anontalkpagetext'                 => "---- ''這是一個還未建立帳號的匿名用戶的對話頁。我們因此只能用IP地址來與他／她聯絡。該IP地址可能由幾名用戶共享。如果您是一名匿名用戶並認為本頁上的評語與您無關，請[[Special:UserLogin/signup|創建新帳號]]或[[Special:UserLogin|登入]]以避免在未來於其他匿名用戶混淆。''",
'noarticletext'                    => '此頁目前沒有內容，您可以在其它頁[[Special:Search/{{PAGENAME}}|搜索此頁標題]]或[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} 編輯此頁]。',
'userpage-userdoesnotexist'        => '用戶帳號「$1」未曾創建。請在創建／編輯這個頁面前先檢查一下。',
'clearyourcache'                   => "'''注意 - 在儲存以後, 您必須清除瀏覽器的快取才能看到所作出的改變。'''
'''Mozilla / Firefox / Safari:''' 按著 ''Shift'' 再點擊''重新整理''，或按下''Ctrl-F5''或''Ctrl-R''，(在Macintosh上按下''Command-R'')；
'''Konqueror:''' 只需點擊 ''重新整理''或按下''F5''；
'''Opera:''' 在 ''工具→設定'' 中完整地清除它們的快取；
'''Internet Explorer:''' 按著 ''Ctrl'' 再點擊 ''重新整理''，或按下 ''Ctrl-F5''。",
'usercssjsyoucanpreview'           => "<strong>提示:</strong> 在保存前請用'顯示預覧'按鈕來測試您新的 CSS/JS 。",
'usercsspreview'                   => "'''注意您只是在預覽您的個人 CSS。'''
'''還沒有儲存﹗'''",
'userjspreview'                    => "'''注意您只是在測試／預覽您的個人 JavaScript。'''
'''還沒有儲存﹗'''",
'userinvalidcssjstitle'            => "'''警告:''' 不存在面板\"\$1\"。注意自訂的 .css 和 .js 頁要使用小寫標題，例如，{{ns:user}}:Foo/monobook.css 不同於 {{ns:user}}:Foo/Monobook.css。",
'updated'                          => '(已更新)',
'note'                             => '<strong>注意:</strong>',
'previewnote'                      => '<strong>請記住這只是預覽，內容還未保存！</strong>',
'previewconflict'                  => '這個預覽顯示了上面文字編輯區中的內容。它將在你選擇保存後出現。',
'session_fail_preview'             => '<strong>抱歉！由於部份資料遺失，我們無法處理您的編輯。請再試一次，如果仍然失敗，請[[Special:UserLogout|登出]]後重新登入。</strong>',
'session_fail_preview_html'        => "<strong>抱歉！部份資料已遺失，我們無法處理您的編輯。</strong>

''由於{{SITENAME}}已經開放原始 HTML 碼，預覽已經隱藏以預防 JavaScript 的攻擊。''

<strong>如果這個編輯過程沒有問題，請再試一次。如果仍然有問題，請[[Special:UserLogout|登出]]後再重新登入一次。</strong>",
'token_suffix_mismatch'            => '<strong>由於您用戶端中的編輯信符毀損了一些標點符號字元，為防止編輯的文字損壞，您的編輯已經被拒絕。
這種情況通常出現於使用含有很多臭蟲、以網絡為主的匿名代理服務的時候。</strong>',
'editing'                          => '正在編輯$1',
'editingsection'                   => '正在編輯$1 (段落)',
'editingcomment'                   => '正在編輯$1 (評論)',
'editconflict'                     => '編輯衝突：$1',
'explainconflict'                  => '有人在你開始編輯後更改了頁面。
上面的文字框內顯示的是目前本頁的內容。
你所做的修改顯示在下面的文字框中。
你應當將你所做的修改加入現有的內容中。
<b>只有</b>在上面文字框中的內容會在你點擊"保存頁面"後被保存。<br />',
'yourtext'                         => '您的文字',
'storedversion'                    => '已保存版本',
'nonunicodebrowser'                => '<strong>警告: 您的瀏覽器不兼容Unicode編碼。這裡有一個工作區將使您能安全地編輯頁面: 非ASCII字元將以十六進製編碼模式出現在編輯框中。</strong>',
'editingold'                       => '<strong>警告：你正在編輯的是本頁的舊版本。
如果你保存它的話，在本版本之後的任何修改都會丟失。</strong>',
'yourdiff'                         => '差異',
'copyrightwarning'                 => '請注意您對{{SITENAME}}的所有貢獻都被認為是在$2下發佈，請查看在$1的細節。
如果您不希望您的文字被任意修改和再散佈，請不要提交。<br />
您同時也要向我們保證您所提交的內容是您自己所作，或得自一個不受版權保護或相似自由的來源。
<strong>不要在未獲授權的情況下發表！</strong><br />',
'copyrightwarning2'                => '請注意您對{{SITENAME}}的所有貢獻
都可能被其他貢獻者編輯，修改或刪除。
如果您不希望您的文字被任意修改和再散佈，請不要提交。<br />
您同時也要向我們保證您所提交的內容是您自己所作，或得自一個不受版權保護或相似自由的來源（參閱$1的細節）。
<strong>不要在未獲授權的情況下發表！</strong>',
'longpagewarning'                  => '<strong>警告: 本頁長度達$1KB；一些瀏覽器將無法編輯長過32KB頁面。請考慮將本文切割成幾個小段落。</strong>',
'longpageerror'                    => '<strong>錯誤: 您所提交的文字長度有$1KB，這大於$2KB的最大值。該文本不能被儲存。</strong>',
'readonlywarning'                  => '<strong>警告: 資料庫被鎖以進行維護，所以您目前將無法保存您的修改。您或許希望先將本段文字複製並保存到文字文件，然後等一會兒再修改。</strong>',
'protectedpagewarning'             => '<strong>警告: 本頁已經被保護，只有擁有管理員許可權的用戶才可修改。</strong>',
'semiprotectedpagewarning'         => "'''注意:''' 本頁面被鎖定，僅限註冊用戶編輯。",
'cascadeprotectedwarning'          => '警告: 本頁已經被保護，只有擁有管理員權限的用戶才可修改，因為本頁已被以下連鎖保護的{{PLURAL:$1|一個|多個}}頁面所包含:',
'titleprotectedwarning'            => '<strong>警告: 本頁面已被鎖上，只有一些用戶方可創建。</strong>',
'templatesused'                    => '在這個頁面上使用的模板有:',
'templatesusedpreview'             => '此次預覽中使用的模板有:',
'templatesusedsection'             => '在這個段落上使用的模板有:',
'template-protected'               => '(保護)',
'template-semiprotected'           => '(半保護)',
'hiddencategories'                 => '這個頁面是屬於$1個隱藏分類的成員:',
'edittools'                        => '<!-- 此處的文字將被顯示在以下編輯和上傳表單中。 -->',
'nocreatetitle'                    => '創建頁面受限',
'nocreatetext'                     => '{{SITENAME}}限制了創建新頁面的功能。你可以返回並編輯已有的頁面，或者[[Special:UserLogin|登錄或創建新賬戶]]。',
'nocreate-loggedin'                => '您在{{SITENAME}}中並無許可權去創建新頁面。',
'permissionserrors'                => '權限錯誤',
'permissionserrorstext'            => '根據以下的{{PLURAL:$1|原因|原因}}，您並無權限去做以下的動作:',
'permissionserrorstext-withaction' => '根據以下的{{PLURAL:$1|原因|原因}}，您並無權限去做$2:',
'recreate-deleted-warn'            => "'''警告: 你現在重新創建一個先前曾經刪除過的頁面。'''

你應該要考慮一下繼續編輯這一個頁面是否合適。
為方便起見，這一個頁面的刪除記錄已經在下面提供:",

# Parser/template warnings
'expensive-parserfunction-warning'        => '警告: 這個頁面有太多耗費的語法功能呼叫。

它應該少過$2，現在有$1。',
'expensive-parserfunction-category'       => '頁面中有太多耗費的語法功能呼叫',
'post-expand-template-inclusion-warning'  => '警告: 包含模板大小過大。
一些模板將不會包含。',
'post-expand-template-inclusion-category' => '模板包含上限已經超過的頁面',
'post-expand-template-argument-warning'   => '警告: 這個頁面有最少一個模參數有過大擴展大小。
這些參數會被略過。',
'post-expand-template-argument-category'  => '包含着略過模板參數的頁面',

# "Undo" feature
'undo-success' => '該編輯可以被撤銷。請檢查以下對比以核實這正是您想做的，然後儲存以下更改以完成撤銷編輯。',
'undo-failure' => '由於中途的編輯不一致，此編輯不能撤銷。',
'undo-norev'   => '由於其修訂版本不存在或已刪除，此編輯不能撤銷。',
'undo-summary' => '取消由[[Special:Contributions/$2|$2]] ([[User talk:$2|對話]])所作出的修訂 $1',

# Account creation failure
'cantcreateaccounttitle' => '無法創建帳戶',
'cantcreateaccount-text' => "從這個IP地址 (<b>$1</b>) 創建帳戶已經被[[User:$3|$3]]禁止。

當中被$3封禁的原因是''$2''",

# History pages
'viewpagelogs'        => '查詢這個頁面的日誌',
'nohistory'           => '沒有本頁的修訂記錄。',
'revnotfound'         => '沒有找到修訂記錄',
'revnotfoundtext'     => '您請求的更早版本的修訂記錄沒有找到。
請檢查您請求本頁面用的URL是否正確。',
'currentrev'          => '當前修訂版本',
'revisionasof'        => '在$1所做的修訂版本',
'revision-info'       => '在$1由$2所做的修訂版本',
'previousrevision'    => '←上一修訂',
'nextrevision'        => '下一修訂→',
'currentrevisionlink' => '當前修訂',
'cur'                 => '當前',
'next'                => '後繼',
'last'                => '先前',
'page_first'          => '最前',
'page_last'           => '最後',
'histlegend'          => '差異選擇: 標記要比較版本的單選按鈕並點擊底部的按鈕進行比較。<br />
說明: (當前) 指與當前版本比較，(先前) 指與前一個修訂版本比較，小 = 小修改。',
'deletedrev'          => '[已刪除]',
'histfirst'           => '最早版本',
'histlast'            => '最新版本',
'historysize'         => '($1 位元組)',
'historyempty'        => '(空)',

# Revision feed
'history-feed-title'          => '修訂歷史',
'history-feed-description'    => '本站上此頁的修訂歷史',
'history-feed-item-nocomment' => '$1在$2', # user at time
'history-feed-empty'          => '所請求的頁面不存在。它可能已被刪除或重新命名。
嘗試[[Special:Search|搜索本站]]獲得相關的新建頁面。',

# Revision deletion
'rev-deleted-comment'         => '(註釋已移除)',
'rev-deleted-user'            => '(用戶名已移除)',
'rev-deleted-event'           => '(日誌動作已移除)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">該頁面修訂已經被從公共文檔中移除。
在[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌]中您可能會檢視到詳細的訊息。</div>',
'rev-deleted-text-view'       => "<div class='mw-warning plainlinks'>
該頁面修訂已經被從公共文檔中移除。作為{{SITENAME}}的管理員，您可以檢視它；
在[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌]中您可能會檢視到詳細的訊息。
</div>",
'rev-delundel'                => '顯示/隱藏',
'revisiondelete'              => '刪除/恢復刪除修訂',
'revdelete-nooldid-title'     => '無效的目標修訂',
'revdelete-nooldid-text'      => '您尚未指定一個目標修訂去進行這個功能、
所指定的修訂不存在，或者您嘗試去隱藏現時的修訂。',
'revdelete-selected'          => "選取'''$1'''的$2次修訂:",
'logdelete-selected'          => "選取'''$1'''的日誌項目:",
'revdelete-text'              => '刪除的修訂仍將顯示在頁面歷史中, 但它們的文字內容已不能被公眾訪問。

在{{SITENAME}}的其他管理員將仍能訪問隱藏的內容並透過與此相同的介面恢復刪除，除非網站工作者進行了一些附加的限制。',
'revdelete-legend'            => '設定可見性之限制',
'revdelete-hide-text'         => '隱藏修訂文字',
'revdelete-hide-name'         => '隱藏動作和目標',
'revdelete-hide-comment'      => '隱藏編輯說明',
'revdelete-hide-user'         => '隱藏編輯者的用戶名/IP',
'revdelete-hide-restricted'   => '將此限制同樣應用於管理員同時鎖定這個界面',
'revdelete-suppress'          => '同時廢止由操作員以及其他用戶的資料',
'revdelete-hide-image'        => '隱藏檔案內容',
'revdelete-unsuppress'        => '在已恢復的修訂中移除限制',
'revdelete-log'               => '日誌註釋:',
'revdelete-submit'            => '應用於選取的修訂',
'revdelete-logentry'          => '[[$1]]的修訂可見性已更改',
'logdelete-logentry'          => '[[$1]]的事件可見性已更改',
'revdelete-success'           => "'''修訂的可見性已經成功設定。'''",
'logdelete-success'           => "'''事件的可見性已經成功設定。'''",
'revdel-restore'              => '更改可見性',
'pagehist'                    => '頁面歷史',
'deletedhist'                 => '已刪除之歷史',
'revdelete-content'           => '內容',
'revdelete-summary'           => '編輯摘要',
'revdelete-uname'             => '用戶名',
'revdelete-restricted'        => '已應用限制至操作員',
'revdelete-unrestricted'      => '已移除對於操作員的限制',
'revdelete-hid'               => '隱藏 $1',
'revdelete-unhid'             => '不隱藏 $1',
'revdelete-log-message'       => '$1的$2次修訂',
'logdelete-log-message'       => '$1的$2項事件',

# Suppression log
'suppressionlog'     => '廢止日誌',
'suppressionlogtext' => '以下是刪除以及由操作員牽涉到內容封鎖的清單。
參看[[Special:IPBlockList|IP封鎖名單]]去參看現時進行中的禁止以及封鎖之名單。',

# History merging
'mergehistory'                     => '合併頁面歷史',
'mergehistory-header'              => '這一頁可以講您合併一個來源頁面的歷史到另一個新頁面中。
請確認這次更改會繼續保留該頁面先前的歷史版本。',
'mergehistory-box'                 => '合併兩個頁面的修訂:',
'mergehistory-from'                => '來源頁面:',
'mergehistory-into'                => '目的頁面:',
'mergehistory-list'                => '可以合併的編輯歷史',
'mergehistory-merge'               => '以下[[:$1]]的修訂可以合併到[[:$2]]。用該選項按鈕欄去合併只有在指定時間以前所創建的修訂。要留意的是使用導航連結便會重設這一欄。',
'mergehistory-go'                  => '顯示可以合併的編輯',
'mergehistory-submit'              => '合併修訂',
'mergehistory-empty'               => '沒有修訂可以合併',
'mergehistory-success'             => '[[:$1]]的$3次修訂已經成功地合併到[[:$2]]。',
'mergehistory-fail'                => '不可以進行歷史合併，請重新檢查該頁面以及時間參數。',
'mergehistory-no-source'           => '來源頁面$1不存在。',
'mergehistory-no-destination'      => '目的頁面$1不存在。',
'mergehistory-invalid-source'      => '來源頁面必須是一個有效的標題。',
'mergehistory-invalid-destination' => '目的頁面必須是一個有效的標題。',
'mergehistory-autocomment'         => '已經合併[[:$1]]去到[[:$2]]',
'mergehistory-comment'             => '已經合併[[:$1]]去到[[:$2]]: $3',

# Merge log
'mergelog'           => '合併日誌',
'pagemerge-logentry' => '已合併[[$1]]到[[$2]] (修訂截至$3)',
'revertmerge'        => '解除合併',
'mergelogpagetext'   => '以下是一個最近由一個頁面的修訂歷史合併到另一個頁面的列表。',

# Diffs
'history-title'           => '「$1」的修訂歷史',
'difference'              => '(修訂版本間差異)',
'lineno'                  => '第$1行：',
'compareselectedversions' => '比較選定的版本',
'editundo'                => '撤銷',
'diff-multi'              => '($1個中途的修訂版本沒有顯示。)',

# Search results
'searchresults'             => '搜索結果',
'searchresulttext'          => '有關搜索{{SITENAME}}的更多詳情,參見[[{{MediaWiki:Helppage}}|{{int:help}}]]。',
'searchsubtitle'            => '查詢\'\'\'[[:$1]]\'\'\'([[Special:Prefixindex/$1|所有以 "$1" 開頭的頁面]] | [[Special:WhatLinksHere/$1|所有鏈接到 "$1" 的頁面]])',
'searchsubtitleinvalid'     => '查詢"$1"',
'noexactmatch'              => "'''沒找到標題為\"\$1\"的頁面。''' 您可以[[:\$1|創建此頁面]]。",
'noexactmatch-nocreate'     => "'''沒找到標題為\"\$1\"的頁面。'''",
'toomanymatches'            => '過多的匹配已回應，請嘗試一個不同的查詢',
'titlematches'              => '頁面題目相符',
'notitlematches'            => '沒有找到匹配頁面題目',
'textmatches'               => '頁面內容相符',
'notextmatches'             => '沒有頁面內容匹配',
'prevn'                     => '前$1個',
'nextn'                     => '後$1個',
'viewprevnext'              => '檢視 ($1) ($2) ($3)',
'search-result-size'        => '$1 ($2個字)',
'search-result-score'       => '相關度: $1%',
'search-redirect'           => '(重定向 $1)',
'search-section'            => '(段落 $1)',
'search-suggest'            => '你是否解: $1',
'search-interwiki-caption'  => '姊妹計劃',
'search-interwiki-default'  => '$1項結果:',
'search-interwiki-more'     => '(更多)',
'search-mwsuggest-enabled'  => '有建議',
'search-mwsuggest-disabled' => '無建議',
'search-relatedarticle'     => '相關',
'mwsuggest-disable'         => '停用AJAX建議',
'searchrelated'             => '相關',
'searchall'                 => '所有',
'showingresults'            => '下面顯示從第<b>$2</b>條開始的<b>$1</b>條結果:',
'showingresultsnum'         => '下面顯示從第<b>$2</b>條開始的<b>$3</b>條結果:',
'showingresultstotal'       => "下面顯示從'''$1 - $2'''，總共'''$3'''項之結果",
'nonefound'                 => "'''注意''': 只有一些名字空間是會作為預設搜尋。嘗試''all:''去搜尋全部的頁面（包埋討論頁面、模板等），或可用需要的名字空間作為前綴。",
'powersearch'               => '進階搜索',
'powersearch-legend'        => '進階搜索',
'powersearch-ns'            => '在以下的名字空間中搜尋:',
'powersearch-redir'         => '重新定向清單',
'powersearch-field'         => '搜尋',
'search-external'           => '外部搜索',
'searchdisabled'            => '{{SITENAME}}由於性能方面的原因，全文搜索已被暫時停用。您可以暫時透過Google搜索。請留意他們的索引可能會過時。',

# Preferences page
'preferences'              => '參數設置',
'mypreferences'            => '我的參數設置',
'prefs-edits'              => '編輯數量:',
'prefsnologin'             => '還未登入',
'prefsnologintext'         => '您必須先<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} 登入]</span>才能設置個人參數。',
'prefsreset'               => '參數已重新設置。',
'qbsettings'               => '快速導航條',
'qbsettings-none'          => '無',
'qbsettings-fixedleft'     => '左側固定',
'qbsettings-fixedright'    => '右側固定',
'qbsettings-floatingleft'  => '左側漂移',
'qbsettings-floatingright' => '右側漂移',
'changepassword'           => '更改密碼',
'skin'                     => '面板',
'math'                     => '數學公式',
'dateformat'               => '日期格式',
'datedefault'              => '預設值',
'datetime'                 => '日期和時間',
'math_failure'             => '解析失敗',
'math_unknown_error'       => '未知錯誤',
'math_unknown_function'    => '未知函數',
'math_lexing_error'        => '句法錯誤',
'math_syntax_error'        => '語法錯誤',
'math_image_error'         => 'PNG 轉換失敗；請檢查是否正確安裝了 latex, dvips, gs 和 convert',
'math_bad_tmpdir'          => '無法寫入或建立數學公式臨時目錄',
'math_bad_output'          => '無法寫入或建立數學公式輸出目錄',
'math_notexvc'             => '無法執行"texvc"；請參照 math/README 進行配置。',
'prefs-personal'           => '用戶資料',
'prefs-rc'                 => '最近更改',
'prefs-watchlist'          => '監視列表',
'prefs-watchlist-days'     => '監視列表中顯示記錄的天數:',
'prefs-watchlist-edits'    => '在增強的監視列表中顯示的最多更改次數:',
'prefs-misc'               => '雜項',
'saveprefs'                => '保存參數設置',
'resetprefs'               => '重設參數',
'oldpassword'              => '舊密碼',
'newpassword'              => '新密碼',
'retypenew'                => '確認密碼:',
'textboxsize'              => '編輯',
'rows'                     => '列:',
'columns'                  => '欄:',
'searchresultshead'        => '搜索結果設定',
'resultsperpage'           => '每頁顯示連結數',
'contextlines'             => '每連結行數:',
'contextchars'             => '每行字數:',
'stub-threshold'           => '<a href="#" class="stub">短頁面連結</a>格式門檻值 (位元組):',
'recentchangesdays'        => '最近更改中的顯示日數:',
'recentchangescount'       => '最近更改、歷史及日誌頁面中的編輯數:',
'savedprefs'               => '您的個人參數設置已經保存。',
'timezonelegend'           => '時區',
'timezonetext'             => '¹輸入當地時間與伺服器時間(UTC)的時差。',
'localtime'                => '當地時間',
'timezoneoffset'           => '時差¹',
'servertime'               => '伺服器時間',
'guesstimezone'            => '從瀏覽器填寫',
'allowemail'               => '接受來自其他用戶的郵件',
'prefs-searchoptions'      => '搜尋選項',
'prefs-namespaces'         => '頁面名稱空間',
'defaultns'                => '預設搜索的名字空間',
'default'                  => '預設',
'files'                    => '檔案',

# User rights
'userrights'                  => '用戶權限管理', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => '管理用戶群組',
'userrights-user-editname'    => '輸入用戶名:',
'editusergroup'               => '編輯用戶群組',
'editinguser'                 => "正在更改用戶'''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) 的用戶權限",
'userrights-editusergroup'    => '編輯用戶群組',
'saveusergroups'              => '保存用戶群組',
'userrights-groupsmember'     => '屬於:',
'userrights-groups-help'      => '您可以改動這位用戶所屬的群組:
* 已剔選的核取方塊代表該用戶屬於該群組。
* 未剔選的核取方塊代表該用戶不是屬於該群組。
* 一個 * 表示當您加入群組之後更不能移除它，反之亦然。',
'userrights-reason'           => '更改原因:',
'userrights-no-interwiki'     => '您並沒有權限去編輯在其它wiki上的用戶權限。',
'userrights-nodatabase'       => '資料庫$1不存在或並非為本地的。',
'userrights-nologin'          => '您必須要以操作員帳戶[[Special:UserLogin|登入]]之後才可以指定用戶權限。',
'userrights-notallowed'       => '您的帳戶無權限去指定用戶權限。',
'userrights-changeable-col'   => '您可以更改的群組',
'userrights-unchangeable-col' => '您不可以更改的群組',

# Groups
'group'               => '群組:',
'group-user'          => '用戶',
'group-autoconfirmed' => '自動確認用戶',
'group-bot'           => '機器人',
'group-sysop'         => '操作員',
'group-bureaucrat'    => '行政員',
'group-suppress'      => '監督',
'group-all'           => '(全部)',

'group-user-member'          => '用戶',
'group-autoconfirmed-member' => '自動確認用戶',
'group-bot-member'           => '機器人',
'group-sysop-member'         => '操作員',
'group-bureaucrat-member'    => '行政員',
'group-suppress-member'      => '監督',

'grouppage-user'          => '{{ns:project}}:用戶',
'grouppage-autoconfirmed' => '{{ns:project}}:自動確認用戶',
'grouppage-bot'           => '{{ns:project}}:機器人',
'grouppage-sysop'         => '{{ns:project}}:操作員',
'grouppage-bureaucrat'    => '{{ns:project}}:行政員',
'grouppage-suppress'      => '{{ns:project}}:監督',

# Rights
'right-read'                 => '閱讀頁面',
'right-edit'                 => '編輯頁面',
'right-createpage'           => '建立頁面（不含討論頁面）',
'right-createtalk'           => '建立討論頁面',
'right-createaccount'        => '創建新用戶賬戶',
'right-minoredit'            => '標示作小編輯',
'right-move'                 => '移動頁面',
'right-move-subpages'        => '移動頁面跟它的字頁面',
'right-suppressredirect'     => '移動頁面時不建立重定向',
'right-upload'               => '上載檔案',
'right-reupload'             => '覆蓋現有的檔案',
'right-reupload-own'         => '覆蓋由同一位上載的檔案',
'right-reupload-shared'      => '於本地無視共用媒體檔案庫上的檔案',
'right-upload_by_url'        => '由URL地址上載一個檔案',
'right-purge'                => '不需要確認之下清除網站快取',
'right-autoconfirmed'        => '編輯半保護頁面',
'right-bot'                  => '視為一個自動程序',
'right-nominornewtalk'       => '小編輯不引發新信息提示',
'right-apihighlimits'        => '在API查詢中使用更高的上限',
'right-writeapi'             => '使用編寫的API',
'right-delete'               => '刪除頁面',
'right-bigdelete'            => '刪除大量歷史之頁面',
'right-deleterevision'       => '刪除及同反刪除頁面中的指定修訂',
'right-deletedhistory'       => '查看已刪除之項目，不含有關的字',
'right-browsearchive'        => '搜尋已刪除之頁面',
'right-undelete'             => '反刪除頁面',
'right-suppressrevision'     => '查看及恢復由操作員隱藏之修訂',
'right-suppressionlog'       => '查看私人的日誌',
'right-block'                => '封鎖其他用戶防止編輯',
'right-blockemail'           => '封鎖用戶不可發電郵',
'right-hideuser'             => '封鎖用戶名，對公眾隱藏',
'right-ipblock-exempt'       => '繞過IP封鎖、自動封鎖以及範圍封鎖',
'right-proxyunbannable'      => '繞過Proxy的自動封鎖',
'right-protect'              => '更改保護等級以及埋編輯保護頁面',
'right-editprotected'        => '編輯保護頁面（無連鎖保護）',
'right-editinterface'        => '編輯用戶界面',
'right-editusercssjs'        => '編輯其他用戶的CSS和JS檔案',
'right-rollback'             => '快速復原上位用戶對某一頁面之編輯',
'right-markbotedits'         => '標示復原編輯作機械人編輯',
'right-noratelimit'          => '沒有使用頻率限制',
'right-import'               => '由其它wiki中匯入頁面',
'right-importupload'         => '由檔案上載中匯入頁面',
'right-patrol'               => '標示其它的編輯作已巡查的',
'right-autopatrol'           => '將自己的編輯自動標示為已巡查的',
'right-patrolmarks'          => '檢視最近巡查標記更改',
'right-unwatchedpages'       => '查看未監視之頁面',
'right-trackback'            => '遞交一個trackback',
'right-mergehistory'         => '合併頁面歷史',
'right-userrights'           => '編輯所有用戶的權限',
'right-userrights-interwiki' => '編輯在其它wiki上的用戶權限',
'right-siteadmin'            => '鎖定和解除鎖定資料庫',

# User rights log
'rightslog'      => '用戶權限日誌',
'rightslogtext'  => '以下記錄了用戶權限的更改記錄。',
'rightslogentry' => '將 $1 的權限從 $2 改為 $3',
'rightsnone'     => '(無)',

# Recent changes
'nchanges'                          => '$1次更改',
'recentchanges'                     => '最近更改',
'recentchangestext'                 => '跟蹤這個wiki上的最新更改。',
'recentchanges-feed-description'    => '跟蹤此訂閱在 wiki 上的最近更改。',
'rcnote'                            => "以下是在$4 $5，最近'''$2'''天內的'''$1'''次最近更改記錄:",
'rcnotefrom'                        => '下面是自<b>$2</b>(最多顯示<b>$1</b>):',
'rclistfrom'                        => '顯示自$1以來的新更改',
'rcshowhideminor'                   => '$1小編輯',
'rcshowhidebots'                    => '$1機器人的編輯',
'rcshowhideliu'                     => '$1已登入用戶的編輯',
'rcshowhideanons'                   => '$1匿名用戶的編輯',
'rcshowhidepatr'                    => ' $1檢查過的編輯',
'rcshowhidemine'                    => '$1我的編輯',
'rclinks'                           => '顯示最近$2天內最新的$1次改動。<br />$3',
'diff'                              => '差異',
'hist'                              => '歷史',
'hide'                              => '隱藏',
'show'                              => '顯示',
'minoreditletter'                   => '小',
'newpageletter'                     => '新',
'boteditletter'                     => '機',
'number_of_watching_users_pageview' => '[$1個關注用戶]',
'rc_categories'                     => '分類界限(以"|"分割)',
'rc_categories_any'                 => '任意',
'newsectionsummary'                 => '/* $1 */ 新段落',

# Recent changes linked
'recentchangeslinked'          => '鏈出更改',
'recentchangeslinked-title'    => '對於「$1」有關的鏈出更改',
'recentchangeslinked-noresult' => '在這一段時間中連結的頁面並無更改。',
'recentchangeslinked-summary'  => "這一個特殊頁面列示''由''所給出的一個頁面之連結到頁面的最近更改（或者是對於指定分類的成員）。
在[[Special:Watchlist|您的監視列表]]中的頁面會以'''粗體'''顯示。",
'recentchangeslinked-page'     => '頁面名稱:',
'recentchangeslinked-to'       => '顯示連到所給出的頁面',

# Upload
'upload'                      => '上傳檔案',
'uploadbtn'                   => '上傳檔案',
'reupload'                    => '重新上載',
'reuploaddesc'                => '取消上載並返回上載表單',
'uploadnologin'               => '未登入',
'uploadnologintext'           => '您必須先[[Special:UserLogin|登入]]
才能上載檔案。',
'upload_directory_missing'    => '上傳目錄($1)遺失，不能由網頁伺服器建立。',
'upload_directory_read_only'  => '上傳目錄($1)不存在或無寫權限。',
'uploaderror'                 => '上載錯誤',
'uploadtext'                  => "使用下面的表單來上傳檔案。
要檢視或搜索以前上傳的檔案
可以進入[[Special:ImageList|檔案上傳清單]]，
（重新）上傳將在[[Special:Log/upload|上傳日誌]]中記錄，
而刪除將在[[Special:Log/delete|刪除日誌]]中記錄。

要在頁面中加入檔案，使用以下其中一種形式的連接:
* '''<tt><nowiki>[[{{ns:image}}:file.jpg]]</nowiki></tt>'''去用檔案的完整版本
* '''<tt><nowiki>[[{{ns:image}}:file.png|200px|thumb|left|替換文字]]</nowiki></tt>'''去用一個200像素寬，左面盒上有'替換文字'的描述
* '''<tt><nowiki>[[{{ns:media}}:file.ogg]]</nowiki></tt>'''去直接連接到檔案而不顯示檔案",
'upload-permitted'            => '准許的檔案類型: $1。',
'upload-preferred'            => '建議的檔案類型: $1。',
'upload-prohibited'           => '禁止的檔案類型: $1。',
'uploadlog'                   => '上載紀錄',
'uploadlogpage'               => '上載紀錄',
'uploadlogpagetext'           => '以下是最近上載的檔案的一覽表。
查看[[Special:NewImages|新檔案畫廊]]去看更富圖像的總覽。',
'filename'                    => '檔案名',
'filedesc'                    => '檔案描述',
'fileuploadsummary'           => '檔案描述:',
'filestatus'                  => '版權狀態:',
'filesource'                  => '來源:',
'uploadedfiles'               => '已上載檔案',
'ignorewarning'               => '忽略警告並儲存檔案',
'ignorewarnings'              => '忽略所有警告',
'minlength1'                  => '檔案名字必須至少有一個字母。',
'illegalfilename'             => '檔案名“$1”包含有頁面標題所禁止的字符。請改名後重新上傳。',
'badfilename'                 => '檔案名已被改為“$1”。',
'filetype-badmime'            => 'MIME類別"$1"不是容許的檔案格式。',
'filetype-unwanted-type'      => "'''\".\$1\"'''是一種不需要的檔案類型。
建議的{{PLURAL:\$3|一種|多種}}檔案類型有\$2。",
'filetype-banned-type'        => "'''\".\$1\"'''是一種不准許的檔案類型。
容許的{{PLURAL:\$3|一種|多種}}檔案類型有\$2。",
'filetype-missing'            => '該檔案名稱並沒有副檔名 (像 ".jpg")。',
'large-file'                  => '建議檔案大小不能超過 $1；本檔案大小為 $2。',
'largefileserver'             => '這個檔案的大小比伺服器配置允許的大小還要大。',
'emptyfile'                   => '您所上傳的檔案不存在。這可能是由於檔案名鍵入錯誤。請檢查您是否真的要上傳此檔案。',
'fileexists'                  => '已存在相同名稱的檔案，如果您無法確定您是否要改變它，請檢查<strong><tt>$1</tt></strong>。',
'filepageexists'              => '這個檔案的描述頁已經在<strong><tt>$1</tt></strong>創建，但是這個名稱的檔案尚未存在。您輸入了的摘要是不會顯示在該描述頁中。要令該摘要在該處中出現，您便要手動地去編輯它。',
'fileexists-extension'        => '一個相似檔名的檔案已經存在:<br />
上載檔案的檔名: <strong><tt>$1</tt></strong><br />
現有檔案的檔名: <strong><tt>$2</tt></strong><br />
請選擇一個不同的名字。',
'fileexists-thumb'            => "<center>'''已經存在的檔案'''</center>",
'fileexists-thumbnail-yes'    => '這個檔案好像是一幅圖像的縮圖版本<i>(縮圖)</i>。請檢查清楚該檔案<strong><tt>$1</tt></strong>。<br />
如果檢查後的檔案是同原本圖像的大小是一樣的話，就不用再上載多一幅縮圖。',
'file-thumbnail-no'           => '該檔名是以<strong><tt>$1</tt></strong>開始。它好像一幅圖像的縮圖版本<i>(縮圖)</i>。
如果你有該圖像的完整大小，如不是請再修改檔名。',
'fileexists-forbidden'        => '已存在相同名稱的檔案；請返回並用一個新的名稱來上傳此檔案。[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => '在共享檔案庫中已存在此名稱的檔案；請返回並用一個新的名稱來上傳此檔案。[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => '這個檔案是跟以下的{{PLURAL:$1|一|多}}個檔案重覆:',
'successfulupload'            => '上傳成功',
'uploadwarning'               => '上載警告',
'savefile'                    => '保存檔案',
'uploadedimage'               => '已上載"[[$1]]"',
'overwroteimage'              => '已經上載"[[$1]]"的新版本',
'uploaddisabled'              => '無法上傳',
'uploaddisabledtext'          => '檔案上傳在{{SITENAME}}不可用。',
'uploadscripted'              => '該檔案包含可能被網路瀏覽器錯誤解釋的 HTML 或腳本代碼。',
'uploadcorrupt'               => '該檔案包含或具有一個不正確的擴展名。請檢查此檔案並重新上傳。',
'uploadvirus'                 => '該檔案包含有病毒！詳情: $1',
'sourcefilename'              => '來源檔案名:',
'destfilename'                => '目標檔案名:',
'upload-maxfilesize'          => '檔案最大限制大小: $1',
'watchthisupload'             => '監視此頁',
'filewasdeleted'              => '之前已經有一個同名檔案被上傳後又被刪除了。在上傳此檔案之前您需要檢查$1。',
'upload-wasdeleted'           => "'''警告: 您現在重新上傳一個先前曾經刪除過的檔案。'''

您應該要考慮一下繼續上傳一個檔案頁面是否合適。
為方便起見，這一個檔案的刪除記錄已經在下面提供:",
'filename-bad-prefix'         => '您上傳的檔案名稱是以<strong>「$1」</strong>作為開頭，通常這種沒有含意的檔案名稱是由數碼相機中自動編排。請在您的檔案中重新選擇一個更加有意義的檔案名稱。',

'upload-proto-error'      => '協議錯誤',
'upload-proto-error-text' => '遠程上傳要求 URL 以 <code>http://</code> 或 <code>ftp://</code> 開頭。',
'upload-file-error'       => '內部錯誤',
'upload-file-error-text'  => '當試圖在伺服器上創建臨時檔案時發生內部錯誤。請與[[Special:ListUsers/sysop|管理員]]聯繫。',
'upload-misc-error'       => '未知的上傳錯誤',
'upload-misc-error-text'  => '在上傳時發生未知的錯誤. 請驗証使用了正確並可訪問的 URL，然後進行重試。如果問題仍然存在，請與[[Special:ListUsers/sysop|管理員]]聯繫。',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => '無法訪問 URL',
'upload-curl-error6-text'  => '無法訪問所提供的 URL。請再次檢查該 URL 是否正確，並且網站的訪問是否正常。',
'upload-curl-error28'      => '上傳超時',
'upload-curl-error28-text' => '網站回應時間過長。請檢查此網站的訪問是否正常，過一會再進行嘗試。您可能需要在網路訪問空閒時間再次進行嘗試。',

'license'            => '授權:',
'nolicense'          => '未選定',
'license-nopreview'  => '(無預覽可用)',
'upload_source_url'  => ' (一個有效的，可公開訪問的 URL)',
'upload_source_file' => ' (在您電腦上的一個檔案)',

# Special:ImageList
'imagelist-summary'     => '這個特殊頁面顯示所有上傳過的檔案。
預設中最後上傳的檔案會顯示在這個列表中的最頂處。
點擊一欄的標題去改變這個排列。',
'imagelist_search_for'  => '按檔案名稱搜索:',
'imgfile'               => '檔案',
'imagelist'             => '檔案列表',
'imagelist_date'        => '日期',
'imagelist_name'        => '名稱',
'imagelist_user'        => '用戶',
'imagelist_size'        => '大小',
'imagelist_description' => '描述',

# Image description page
'filehist'                       => '檔案歷史',
'filehist-help'                  => '點擊日期／時間以檢視當時出現過的檔案。',
'filehist-deleteall'             => '刪除全部',
'filehist-deleteone'             => '刪除',
'filehist-revert'                => '恢復',
'filehist-current'               => '當前',
'filehist-datetime'              => '日期／時間',
'filehist-user'                  => '用戶',
'filehist-dimensions'            => '維度',
'filehist-filesize'              => '檔案大小',
'filehist-comment'               => '註解',
'imagelinks'                     => '連結',
'linkstoimage'                   => '以下的$1個頁面連接到本檔案:',
'nolinkstoimage'                 => '沒有頁面連接到本檔案。',
'morelinkstoimage'               => '查看連接到這個檔案的[[Special:WhatLinksHere/$1|更多連結]]。',
'redirectstofile'                => '以下的$1個檔案重新定向到這個檔案:',
'duplicatesoffile'               => '以下的$1個檔案是跟這個檔案重覆:',
'sharedupload'                   => '該檔案是一個共享上傳，它可能在其它項目中被應用。',
'shareduploadwiki'               => '請參閱在共享檔案庫上的$1以了解其相關資訊。',
'shareduploadwiki-desc'          => '它在$1上的描述於下面顯示。',
'shareduploadwiki-linktext'      => '檔案描述頁面',
'shareduploadduplicate'          => '這個檔案是跟共用檔案庫的$1重覆。',
'shareduploadduplicate-linktext' => '另一個檔案',
'shareduploadconflict'           => '這個檔案在共用檔案庫中的$1有同樣的檔名。',
'shareduploadconflict-linktext'  => '另一個檔案',
'noimage'                        => '不存在此名稱的檔案，您可以$1。',
'noimage-linktext'               => '上傳它',
'uploadnewversion-linktext'      => '上傳該檔案的新版本',
'imagepage-searchdupe'           => '搜尋重覆的檔案',

# File reversion
'filerevert'                => '恢復$1',
'filerevert-legend'         => '恢復檔案',
'filerevert-intro'          => "您現正在恢復'''[[Media:$1|$1]]'''到[$4 於$2 $3的版本]。",
'filerevert-comment'        => '註解:',
'filerevert-defaultcomment' => '已經恢復到於$1 $2的版本',
'filerevert-submit'         => '恢復',
'filerevert-success'        => "'''[[Media:$1|$1]]'''已經恢復到[$4 於$2 $3的版本]。",
'filerevert-badversion'     => '這個檔案所提供的時間截記並無先前的本地版本。',

# File deletion
'filedelete'                  => '刪除$1',
'filedelete-legend'           => '刪除檔案',
'filedelete-intro'            => "您現正刪除'''[[Media:$1|$1]]'''。",
'filedelete-intro-old'        => "你現正刪除'''[[Media:$1|$1]]'''於[$4 $2 $3]的版本。",
'filedelete-comment'          => '刪除理由:',
'filedelete-submit'           => '刪除',
'filedelete-success'          => "'''$1'''已經刪除。",
'filedelete-success-old'      => "'''[[Media:$1|$1]]'''於 $2 $3 的版本已經刪除。",
'filedelete-nofile'           => "'''$1'''在{{SITENAME}}中不存在。",
'filedelete-nofile-old'       => "在已指定屬性的情況下，這裡沒有'''$1'''於 $2 $3 的版本。",
'filedelete-iscurrent'        => '您現正嘗試刪除這個檔案的最近版本。請先將個該檔案回退成較舊的版本。',
'filedelete-otherreason'      => '其它／附加的理由:',
'filedelete-reason-otherlist' => '其它理由',
'filedelete-reason-dropdown'  => '
*常用刪除理由
** 侵犯版權
** 重覆檔案',
'filedelete-edit-reasonlist'  => '編輯刪除埋由',

# MIME search
'mimesearch'         => 'MIME 搜索',
'mimesearch-summary' => '本頁面啟用檔案MIME類型過濾器。輸入︰內容類型/子類型，如 <tt>image/jpeg</tt>。',
'mimetype'           => 'MIME 類型:',
'download'           => '下載',

# Unwatched pages
'unwatchedpages' => '未被監視的頁面',

# List redirects
'listredirects' => '重定向頁面清單',

# Unused templates
'unusedtemplates'     => '未使用的模板',
'unusedtemplatestext' => '本頁面列出模板名字空間下所有未被其他頁面使用的頁面。請在刪除這些模板前檢查其他鏈入該模板的頁面。',
'unusedtemplateswlh'  => '其他連結',

# Random page
'randompage'         => '隨機頁面',
'randompage-nopages' => '在這個名字空間中沒有頁面。',

# Random redirect
'randomredirect'         => '隨機重定向頁面',
'randomredirect-nopages' => '在這個名字空間中沒有重定向頁面。',

# Statistics
'statistics'             => '統計',
'sitestats'              => '{{SITENAME}}統計數據',
'userstats'              => '用戶統計',
'sitestatstext'          => "資料庫中共有'''\$1'''頁頁面。
其中包括對話頁、關於{{SITENAME}}的頁面、最少量的\"stub\"頁、重定向的頁面，
以及未達到頁面質量的頁面。除此之外還有'''\$2'''頁可能是合乎標準的頁面。

'''\$8'''個檔案已被上傳。

從{{SITENAME}}設定以來，全網站共有頁面瀏覽'''\$3'''次，頁面編輯'''\$4'''次。
即每頁平均編輯'''\$5'''次，各次編輯後頁面的每個版本平均瀏覽'''\$6'''次。

[http://www.mediawiki.org/wiki/Manual:Job_queue 工作隊列]的長度是'''\$7'''。",
'userstatstext'          => "網站有'''$1'''位註冊[[Special:ListUsers|用戶]]，其中
'''$2''' (或 '''$4%''') 有$5許可權。",
'statistics-mostpopular' => '被查閱次數最多的頁面',

'disambiguations'      => '消含糊頁',
'disambiguationspage'  => 'Template:disambig
Template:消含糊
Template:消除含糊
Template:消歧义
Template:消除歧义
Template:消歧義
Template:消除歧義',
'disambiguations-text' => '以下的頁面都有到<b>消含糊頁</b>的鏈接, 但它們應該是連到適當的標題。<br />一個頁面會被視為消含糊頁如果它是連自[[MediaWiki:Disambiguationspage]]。',

'doubleredirects'            => '雙重重定向頁面',
'doubleredirectstext'        => '這一頁列出所有重定向頁面重定向到另一個重定向頁的頁面。每一行都包含到第一和第二個重定向頁面的鏈接，以及第二個重定向頁面的目標，通常顯示的都會是"真正"的目標頁面，也就是第一個重定向頁面應該指向的頁面。',
'double-redirect-fixed-move' => '[[$1]]已經完成移動，它現在重新定向到[[$2]]',
'double-redirect-fixer'      => '重新定向修正器',

'brokenredirects'        => '損壞的重定向頁',
'brokenredirectstext'    => '以下的重定向頁指向的是不存在的頁面。',
'brokenredirects-edit'   => '(編輯)',
'brokenredirects-delete' => '(刪除)',

'withoutinterwiki'         => '未有語言鏈接的頁面',
'withoutinterwiki-summary' => '以下的頁面是未有語言鏈接到其它語言版本。',
'withoutinterwiki-legend'  => '前綴',
'withoutinterwiki-submit'  => '顯示',

'fewestrevisions' => '最少修訂的頁面',

# Miscellaneous special pages
'nbytes'                  => '$1位元組',
'ncategories'             => '$1個分類',
'nlinks'                  => '$1個連結',
'nmembers'                => '$1個成員',
'nrevisions'              => '$1個修訂',
'nviews'                  => '$1次瀏覽',
'specialpage-empty'       => '這個報告的結果為空。',
'lonelypages'             => '孤立頁面',
'lonelypagestext'         => '以下頁面尚未被{{SITENAME}}中的其它頁面連結。',
'uncategorizedpages'      => '待分類頁面',
'uncategorizedcategories' => '待分類類別',
'uncategorizedimages'     => '待分類檔案',
'uncategorizedtemplates'  => '待分類模版',
'unusedcategories'        => '未使用的分類',
'unusedimages'            => '未使用圖像',
'popularpages'            => '熱點頁面',
'wantedcategories'        => '需要的分類',
'wantedpages'             => '待撰頁面',
'missingfiles'            => '遺失檔案',
'mostlinked'              => '最多連結頁面',
'mostlinkedcategories'    => '最多連結分類',
'mostlinkedtemplates'     => '最多連結模版',
'mostcategories'          => '最多分類頁面',
'mostimages'              => '最多連結檔案',
'mostrevisions'           => '最多修訂頁面',
'prefixindex'             => '前綴索引',
'shortpages'              => '短頁面',
'longpages'               => '長頁面',
'deadendpages'            => '斷鏈頁面',
'deadendpagestext'        => '以下頁面沒有連結到{{SITENAME}}中的其它頁面。',
'protectedpages'          => '已保護頁面',
'protectedpages-indef'    => '只有無期之保護頁面',
'protectedpagestext'      => '以下頁面已經被保護以防止移動或編輯',
'protectedpagesempty'     => '在這些參數下沒有頁面正在保護。',
'protectedtitles'         => '已保護的標題',
'protectedtitlestext'     => '以下的頁面已經被保護以防止建立',
'protectedtitlesempty'    => '在這些參數之下並無標題正在保護。',
'listusers'               => '用戶列表',
'newpages'                => '最新頁面',
'newpages-username'       => '用戶名:',
'ancientpages'            => '最舊頁面',
'move'                    => '移動',
'movethispage'            => '移動本頁',
'unusedimagestext'        => '<p>請注意其它網站可能直接透過 URL 鏈接此檔案，所以這裡列出的圖像有可能依然被使用。</p>',
'unusedcategoriestext'    => '雖然沒有被其它頁面或者分類所採用，但列表中的分類頁依然存在。',
'notargettitle'           => '無目標',
'notargettext'            => '您還沒有指定一個目標頁面或用戶以進行此項操作。',
'nopagetitle'             => '無目標頁面',
'nopagetext'              => '您所指定的目標頁面並不存在。',
'pager-newer-n'           => '新$1次',
'pager-older-n'           => '舊$1次',
'suppress'                => '監督',

# Book sources
'booksources'               => '網路書源',
'booksources-search-legend' => '尋找網路書源',
'booksources-go'            => '送出',
'booksources-text'          => '以下是一份銷售新書或二手書的列表，並可能有你正尋找的書的進一步訊息：',

# Special:Log
'specialloguserlabel'  => '用戶:',
'speciallogtitlelabel' => '標題:',
'log'                  => '日誌',
'all-logs-page'        => '所有日誌',
'log-search-legend'    => '搜尋日誌',
'log-search-submit'    => '去',
'alllogstext'          => '綜合顯示上傳、刪除、保護、查封以及站務日誌。',
'logempty'             => '在日誌中不存在匹配項。',
'log-title-wildcard'   => '搜尋以這個文字開始的標題',

# Special:AllPages
'allpages'          => '所有頁面',
'alphaindexline'    => '$1 到 $2',
'nextpage'          => '下一頁($1)',
'prevpage'          => '上一頁($1)',
'allpagesfrom'      => '顯示從此處開始的頁面:',
'allarticles'       => '所有頁面',
'allinnamespace'    => '所有頁面(屬於$1名字空間)',
'allnotinnamespace' => '所有頁面(不屬於$1名字空間)',
'allpagesprev'      => '前',
'allpagesnext'      => '後',
'allpagessubmit'    => '提交',
'allpagesprefix'    => '顯示具有此前綴(名字空間)的頁面:',
'allpagesbadtitle'  => '給定的頁面標題是非法的，或者具有一個內部語言或內部 wiki 的前綴。它可能包含一個或更多的不能用於標題的字元。',
'allpages-bad-ns'   => '在{{SITENAME}}中沒有一個叫做"$1"的名字空間。',

# Special:Categories
'categories'                    => '頁面分類',
'categoriespagetext'            => '以下的分類中包含了頁面或媒體。
[[Special:UnusedCategories|未用分類]]不會在這裏列示。
請同時參閱[[Special:WantedCategories|需要的分類]]。',
'categoriesfrom'                => '顯示由此項起之分類:',
'special-categories-sort-count' => '按數量排列',
'special-categories-sort-abc'   => '按字母排列',

# Special:ListUsers
'listusersfrom'      => '給定顯示用戶條件:',
'listusers-submit'   => '顯示',
'listusers-noresult' => '找不到用戶。',

# Special:ListGroupRights
'listgrouprights'          => '用戶群組權限',
'listgrouprights-summary'  => '以下面是一個在這個wiki中定義出來的用戶權限清單，以及它們的存取權。
更多有關個別權限的細節可以在[[{{MediaWiki:Listgrouprights-helppage}}|這裏]]找到。',
'listgrouprights-group'    => '群組',
'listgrouprights-rights'   => '權限',
'listgrouprights-helppage' => 'Help:群組權限',
'listgrouprights-members'  => '(成員清單)',

# E-mail user
'mailnologin'     => '無電郵地址',
'mailnologintext' => '您必須先[[Special:UserLogin|登入]]
並在[[Special:Preferences|參數設置]]
中有一個有效的e-mail地址才可以電郵其他用戶。',
'emailuser'       => 'E-mail該用戶',
'emailpage'       => 'E-mail用戶',
'emailpagetext'   => '如果該用戶已經在他或她的參數設置頁中輸入了有效的e-mail地址，以下的表格將寄一個訊息給該用戶。您在[[Special:Preferences|您參數設置]]中所輸入的e-mail地址將出現在郵件「發件人」一欄中，這樣該用戶就可以回覆您。',
'usermailererror' => '目標郵件地址返回錯誤：',
'defemailsubject' => '{{SITENAME}}電子郵件',
'noemailtitle'    => '無e-mail地址',
'noemailtext'     => '該用戶還沒有指定一個有效的e-mail地址，
或者選擇不接受來自其他用戶的e-mail。',
'emailfrom'       => '發件人:',
'emailto'         => '收件人:',
'emailsubject'    => '主題:',
'emailmessage'    => '訊息:',
'emailsend'       => '發送',
'emailccme'       => '將我的消息的副本發送一份到我的電郵信箱。',
'emailccsubject'  => '將您的訊息複製到 $1: $2',
'emailsent'       => '電子郵件已發送',
'emailsenttext'   => '您的電子郵件已經發出。',
'emailuserfooter' => '這封電郵是由$1寄給$2經{{SITENAME}}的「電郵用戶」功能發出的。',

# Watchlist
'watchlist'            => '監視列表',
'mywatchlist'          => '我的監視列表',
'watchlistfor'         => "('''$1'''的監視列表)",
'nowatchlist'          => '您的監視列表為空。',
'watchlistanontext'    => '請$1以檢視或編輯您的監視列表。',
'watchnologin'         => '未登入',
'watchnologintext'     => '您必須先[[Special:UserLogin|登入]]
才能更改您的監視列表',
'addedwatch'           => '加入到監視列表',
'addedwatchtext'       => "頁面\"[[:\$1]]\"已經被加入到您的[[Special:Watchlist|監視清單]]中。
將來有關此頁面及其討論頁的任何修改將會在那裡列出，
而且還會在[[Special:RecentChanges|最近更改]]中
以'''粗體'''形式列出以使起更容易識別。",
'removedwatch'         => '已停止監視',
'removedwatchtext'     => '頁面「<nowiki>$1</nowiki>」已經從[[Special:Watchlist|您的監視頁面]]中移除。',
'watch'                => '監視',
'watchthispage'        => '監視本頁',
'unwatch'              => '取消監視',
'unwatchthispage'      => '停止監視',
'notanarticle'         => '不是頁面',
'notvisiblerev'        => '修訂版本已經刪除',
'watchnochange'        => '在顯示的時間段內您所監視的頁面沒有更改。',
'watchlist-details'    => '不包含討論頁，有 $1 頁在您的監視列表上。',
'wlheader-enotif'      => '* 已經啟動電子郵件通知功能。',
'wlheader-showupdated' => "* 在你上次檢視後有被修改過的頁面會顯示為'''粗體'''",
'watchmethod-recent'   => '檢查被監視頁面的最近編輯',
'watchmethod-list'     => '檢查最近編輯的被監視頁面',
'watchlistcontains'    => '您的監視列表包含$1個頁面。',
'iteminvalidname'      => "頁面'$1'錯誤，無效命名...",
'wlnote'               => '以下是最近<b>$2</b>小時內的最後$1次修改。',
'wlshowlast'           => '顯示最近$1小時；$2天；$3的修改。',
'watchlist-show-bots'  => '顯示機器人的編輯',
'watchlist-hide-bots'  => '隱藏機器人的編輯',
'watchlist-show-own'   => '顯示我的修改',
'watchlist-hide-own'   => '隱藏我的修改',
'watchlist-show-minor' => '顯示小修改',
'watchlist-hide-minor' => '隱藏小修改',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => '正在監視...',
'unwatching' => '正在停止監視...',

'enotif_mailer'                => '{{SITENAME}}郵件通知器',
'enotif_reset'                 => '將所有頁面標為已閱讀',
'enotif_newpagetext'           => '這是新建頁面。',
'enotif_impersonal_salutation' => '{{SITENAME}}用戶',
'changed'                      => '修改了',
'created'                      => '建立了',
'enotif_subject'               => '{{SITENAME}}有頁面 $PAGETITLE 被 $PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited'           => '檢視您上次訪問後的所有更改請參閱$1。',
'enotif_lastdiff'              => '檢視更改請參閱$1。',
'enotif_anon_editor'           => '匿名用戶$1',
'enotif_body'                  => '親愛的 $WATCHINGUSERNAME,

$PAGEEDITOR 已經在 $PAGEEDITDATE $CHANGEDORCREATED{{SITENAME}}的 $PAGETITLE 頁面，請到 $PAGETITLE_URL 檢視當前版本。

$NEWPAGE

編輯摘要: $PAGESUMMARY $PAGEMINOREDIT

聯繫此編輯者:

郵件: $PAGEEDITOR_EMAIL

本站: $PAGEEDITOR_WIKI

在您訪問此頁之前，將來的更改將不會向您發通知。您也可以重設您所有監視頁面的通知標記。

                {{SITENAME}}通知系統

--
要改變您的監視列表設定，請參閱
{{fullurl:{{ns:special}}:Watchlist/edit}}

回饋和進一步的幫助:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => '刪除頁面',
'confirm'                     => '確認',
'excontent'                   => '內容為: "$1"',
'excontentauthor'             => '內容為: "$1" (而且唯一貢獻者為"$2")',
'exbeforeblank'               => '被清空前的內容為: "$1"',
'exblank'                     => '頁面為空',
'delete-confirm'              => '刪除「$1」',
'delete-legend'               => '刪除',
'historywarning'              => '警告: 您將要刪除的頁內含有歷史',
'confirmdeletetext'           => '您即將刪除一個頁面或圖像以及其歷史。
請確定您要進行此項操作，並且了解其後果，同時您的行為符合[[{{MediaWiki:Policy-url}}]]。',
'actioncomplete'              => '操作完成',
'deletedtext'                 => '「<nowiki>$1</nowiki>」已經被刪除。最近刪除的紀錄請參見$2。',
'deletedarticle'              => '已刪除「[[$1]]」',
'suppressedarticle'           => '已廢止「[[$1]]」',
'dellogpage'                  => '刪除紀錄',
'dellogpagetext'              => '以下是最近刪除的紀錄列表。',
'deletionlog'                 => '刪除紀錄',
'reverted'                    => '恢復到早期版本',
'deletecomment'               => '刪除理由:',
'deleteotherreason'           => '其它／附加的理由:',
'deletereasonotherlist'       => '其它理由',
'deletereason-dropdown'       => '
*常用刪除理由
** 作者請求
** 侵犯版權
** 破壞',
'delete-edit-reasonlist'      => '編輯刪除理由',
'delete-toobig'               => '這個頁面有一個十分大量的編輯歷史，超過$1次修訂。刪除此類頁面的動作已經被限制，以防止在{{SITENAME}}上的意外擾亂。',
'delete-warning-toobig'       => '這個頁面有一個十分大量的編輯歷史，超過$1次修訂。刪除它可能會擾亂{{SITENAME}}的資料庫操作；在繼續此動作前請小心。',
'rollback'                    => '恢復編輯',
'rollback_short'              => '恢復',
'rollbacklink'                => '恢復',
'rollbackfailed'              => '無法恢復',
'cantrollback'                => '無法恢復編輯；最後的貢獻者是本文的唯一作者。',
'alreadyrolled'               => '無法恢復由[[User:$2|$2]] ([[User talk:$2|討論]]  | [[Special:Contributions/$2|{{int:contribslink}}]])進行的[[$1]]的最後編輯；
其他人已經編輯或是恢復了該頁。

最後對頁面編輯的編輯者: [[User:$3|$3]] ([[User talk:$3|討論]] | [[Special:Contributions/$3|{{int:contribslink}}]])。',
'editcomment'                 => '編輯說明: "<i>$1</i>"。', # only shown if there is an edit comment
'revertpage'                  => '恢復由[[Special:Contributions/$2|$2]] ([[User talk:$2|對話]])的編輯至[[User:$1|$1]]的最後一個版本', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '恢復由$1的編輯；更改回$2的最後一個版本。',
'sessionfailure'              => '您的登入資訊似乎有問題，為防止此該訊息被攔截，本次操作已經取消，請按「上一頁」重新載入。',
'protectlogpage'              => '保護日誌',
'protectlogtext'              => '下面是頁面鎖定和取消鎖定的列表。請參考[[Special:ProtectedPages|保護頁面清單]]以檢視當前進行的頁面保護。',
'protectedarticle'            => '已保護"[[$1]]"',
'modifiedarticleprotection'   => '已經更改 "[[$1]]" 的保護等級',
'unprotectedarticle'          => '已解除保護"[[$1]]"',
'protect-title'               => '正在更改"$1"的保護等級',
'protect-legend'              => '確認保護',
'protectcomment'              => '註解:',
'protectexpiry'               => '到期:',
'protect_expiry_invalid'      => '輸入的終止時間無效。',
'protect_expiry_old'          => '終止時間已過去。',
'protect-unchain'             => '移動權限解鎖',
'protect-text'                => '你可以在這裡瀏覽和修改對頁面<strong><nowiki>$1</nowiki></strong>的保護級別。',
'protect-locked-blocked'      => '您不能在被查封時更改保護級別。
以下是<strong>$1</strong>現時的保護級別:',
'protect-locked-dblock'       => '在資料庫鎖定時無法更改保護級別。
以下是<strong>$1</strong>現時的保護級別:',
'protect-locked-access'       => '您的帳戶權限不能修改保護級別。
以下是<strong>$1</strong>現時的保護級別:',
'protect-cascadeon'           => '以下的{{PLURAL:$1|一個|多個}}頁面包含着本頁面的同時，啟動了連鎖保護，因此本頁面目前也被保護，未能編輯。您可以設定本頁面的保護級別，但這並不會對連鎖保護有所影響。',
'protect-default'             => '(預設)',
'protect-fallback'            => '需要"$1"的許可',
'protect-level-autoconfirmed' => '禁止未註冊用戶',
'protect-level-sysop'         => '僅操作員',
'protect-summary-cascade'     => '連鎖',
'protect-expiring'            => '終止於 $1 (UTC)',
'protect-cascade'             => '保護本頁中包含的頁面 (連鎖保護)',
'protect-cantedit'            => '您無法更改這個頁面的保護等級，因為您沒有權限去編輯它。',
'restriction-type'            => '權限:',
'restriction-level'           => '限制級別:',
'minimum-size'                => '最小大小',
'maximum-size'                => '最大大小:',
'pagesize'                    => '(位元組)',

# Restrictions (nouns)
'restriction-edit'   => '編輯',
'restriction-move'   => '移動',
'restriction-create' => '建立',
'restriction-upload' => '上傳',

# Restriction levels
'restriction-level-sysop'         => '全保護',
'restriction-level-autoconfirmed' => '半保護',
'restriction-level-all'           => '任何級別',

# Undelete
'undelete'                     => '恢復被刪頁面',
'undeletepage'                 => '瀏覽及恢復被刪頁面',
'undeletepagetitle'            => "'''以下包含[[:$1]]的已刪除之修訂版本'''。",
'viewdeletedpage'              => '檢視被刪除的頁面',
'undeletepagetext'             => '以下頁面已經被刪除，但依然在檔案中並可以被恢復。
檔案庫可能被定時清理。',
'undelete-fieldset-title'      => '恢復修訂',
'undeleteextrahelp'            => "恢復整個頁面時，請清除所有複選框後按 '''''恢復''''' 。 恢復特定版本時，請選擇相應版本前的複選框後按'''''恢復''''' 。按 '''''重設''''' 將清除評論內容及所有複選框。",
'undeleterevisions'            => '$1版本存檔',
'undeletehistory'              => '如果您恢復了該頁面，所有版本都會被恢復到修訂歷史中。
如果本頁刪除後有一個同名的新頁面建立，被恢復的版本將會稱為較新的歷史。',
'undeleterevdel'               => '如果把最新修訂部份刪除，反刪除便無法進行。如果遇到這種情況，您必須反選或反隱藏最新已刪除的修訂。',
'undeletehistorynoadmin'       => '這個頁面已經被刪除，刪除原因顯示在下方編輯摘要中。被刪除前的所有修訂版本，連同刪除前貢獻用戶等等細節只有管理員可以看見。',
'undelete-revision'            => '刪除$1時由$3（在$2）所編寫的修訂版本:',
'undeleterevision-missing'     => '此版本的內容不正確或已經遺失。可能連結錯誤、被移除或已經被恢復。',
'undelete-nodiff'              => '找不到先前的修訂版本。',
'undeletebtn'                  => '恢復',
'undeletelink'                 => '恢復',
'undeletereset'                => '重設',
'undeletecomment'              => '原因',
'undeletedarticle'             => '已經恢復「$1」',
'undeletedrevisions'           => '$1個修訂版本已經恢復',
'undeletedrevisions-files'     => '$1 個版本和 $2 個檔案被恢復',
'undeletedfiles'               => '$1 個檔案被恢復',
'cannotundelete'               => '恢復失敗；可能之前已經被其他人恢復。',
'undeletedpage'                => "<big>'''$1已經被恢復'''</big> 請參考[[Special:Log/delete|刪除日誌]]來查詢刪除及恢復記錄。",
'undelete-header'              => '如要查詢最近的記錄請參閱[[Special:Log/delete|刪除日誌]]。',
'undelete-search-box'          => '搜尋已刪除頁面',
'undelete-search-prefix'       => '顯示頁面自:',
'undelete-search-submit'       => '搜尋',
'undelete-no-results'          => '刪除記錄裡沒有符合的結果。',
'undelete-filename-mismatch'   => '不能刪除帶有時間截記的檔案修訂 $1: 檔案不匹配',
'undelete-bad-store-key'       => '不能刪除帶有時間截記的檔案修訂 $1: 檔案於刪除前遺失。',
'undelete-cleanup-error'       => '刪除無用的存檔檔案 "$1" 時發生錯誤。',
'undelete-missing-filearchive' => '由於檔案存檔 ID $1 不在資料庫中，不能在檔案存檔中恢復。它可能已經反刪除了。',
'undelete-error-short'         => '反刪除檔案時發生錯誤: $1',
'undelete-error-long'          => '當進行反刪除檔案時遇到錯誤:

$1',

# Namespace form on various pages
'namespace'      => '名字空間:',
'invert'         => '反向選擇',
'blanknamespace' => '(主)',

# Contributions
'contributions' => '用戶貢獻',
'mycontris'     => '我的貢獻',
'contribsub2'   => '$1的貢獻 ($2)',
'nocontribs'    => '沒有找到符合特徵的更改。',
'uctop'         => '(最新修改)',
'month'         => '從該月份 (或更早):',
'year'          => '從該年份 (或更早):',

'sp-contributions-newbies'     => '只顯示新建立之用戶的貢獻',
'sp-contributions-newbies-sub' => '新手',
'sp-contributions-blocklog'    => '封禁記錄',
'sp-contributions-search'      => '搜尋貢獻記錄',
'sp-contributions-username'    => 'IP位址或用戶名稱：',
'sp-contributions-submit'      => '搜尋',

# What links here
'whatlinkshere'            => '鏈入頁面',
'whatlinkshere-title'      => '鏈接到「$1」的頁面',
'whatlinkshere-page'       => '頁面:',
'linklistsub'              => '(連結列表)',
'linkshere'                => '以下頁面連結到[[:$1]]：',
'nolinkshere'              => '沒有頁面連結到[[:$1]]。',
'nolinkshere-ns'           => '在所選的名字空間內沒有頁面鏈接到[[:$1]]。',
'isredirect'               => '重定向頁',
'istemplate'               => '包含',
'isimage'                  => '檔案連結',
'whatlinkshere-prev'       => '前$1個',
'whatlinkshere-next'       => '後$1個',
'whatlinkshere-links'      => '← 鏈入',
'whatlinkshere-hideredirs' => '$1重定向',
'whatlinkshere-hidetrans'  => '$1包含',
'whatlinkshere-hidelinks'  => '$1連結',
'whatlinkshere-hideimages' => '$1檔案連結',
'whatlinkshere-filters'    => '過濾器',

# Block/unblock
'blockip'                         => '查封用戶',
'blockip-legend'                  => '查封用戶',
'blockiptext'                     => '用下面的表單來禁止來自某一特定IP地址的修改許可權。
只有在為防止破壞，及符合[[{{MediaWiki:Policy-url}}|守則]]的情況下才可採取此行動。
請在下面輸入一個具體的理由（例如引述一個被破壞的頁面）。',
'ipaddress'                       => 'IP地址:',
'ipadressorusername'              => 'IP地址或用戶名:',
'ipbexpiry'                       => '期限:',
'ipbreason'                       => '原因:',
'ipbreasonotherlist'              => '其它原因',
'ipbreason-dropdown'              => '
*一般的封禁理由
** 屢次增加不實資料
** 刪除頁面內容
** 外部連結廣告
** 在頁面中增加無意義文字
** 無禮的行為、攻擊／騷擾別人
** 濫用多個帳號
** 不能接受的用戶名',
'ipbanononly'                     => '僅阻止匿名用戶',
'ipbcreateaccount'                => '阻止創建新帳號',
'ipbemailban'                     => '阻止用戶傳送電郵',
'ipbenableautoblock'              => '自動查封此用戶最後所用的IP位址，以及後來試圖編輯所用的所有位址',
'ipbsubmit'                       => '查封該地址',
'ipbother'                        => '其它時間:',
'ipboptions'                      => '2小時:2 hours,1天:1 day,3天:3 days,1周:1 week,2周:2 weeks,1個月:1 month,3個月:3 months,6個月:6 months,1年:1 year,永久:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => '其它',
'ipbotherreason'                  => '其它／附帶原因:',
'ipbhidename'                     => '在查封日誌、活躍查封列表以及用戶列表中隱藏用戶名',
'ipbwatchuser'                    => '監視這位用戶的用戶頁面以及其對話頁面',
'badipaddress'                    => 'IP地址不正確。',
'blockipsuccesssub'               => '查封成功',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]]已經被查封。
<br />參看[[Special:IPBlockList|被封IP地址列表]]以覆審查封。',
'ipb-edit-dropdown'               => '編輯查封原因',
'ipb-unblock-addr'                => '解除封禁$1',
'ipb-unblock'                     => '解除禁封用戶名或IP地址',
'ipb-blocklist-addr'              => '檢視$1的現有封禁',
'ipb-blocklist'                   => '檢視現有的封禁',
'unblockip'                       => '解封用戶',
'unblockiptext'                   => '用下面的表單來恢復先前被查封的IP位址或用戶的寫權限。',
'ipusubmit'                       => '解封此地址',
'unblocked'                       => '[[User:$1|$1]] 的封禁已經解除。',
'unblocked-id'                    => '封禁 $1 已經被移除',
'ipblocklist'                     => '被封IP地址列表',
'ipblocklist-legend'              => '搜尋一位已經被查封的用戶',
'ipblocklist-username'            => '用戶名稱或IP地址:',
'ipblocklist-submit'              => '搜尋',
'blocklistline'                   => '$1，$2禁封$3 ($4)',
'infiniteblock'                   => '永久',
'expiringblock'                   => '$1 到期',
'anononlyblock'                   => '僅限匿名用戶',
'noautoblockblock'                => '禁用自動查封',
'createaccountblock'              => '禁止創建賬戶',
'emailblock'                      => '禁止電子郵件',
'ipblocklist-empty'               => '查封列表為空。',
'ipblocklist-no-results'          => '所要求的IP地址/用戶名沒有被查封。',
'blocklink'                       => '查封',
'unblocklink'                     => '解除禁封',
'contribslink'                    => '貢獻',
'autoblocker'                     => '你的IP和被封了的 "$1" 是一樣的。封鎖原因： "$2".',
'blocklogpage'                    => '封鎖記錄',
'blocklogentry'                   => '[[$1]]已被查封 $3 ，終止時間為$2',
'blocklogtext'                    => '這是關於用戶封禁和解除封禁操作的記錄。被自動封禁的IP地址沒有被列出。請參閱[[Special:IPBlockList|被查封的IP地址和用戶列表]]。',
'unblocklogentry'                 => '[[$1]]已被解封',
'block-log-flags-anononly'        => '僅限匿名用戶',
'block-log-flags-nocreate'        => '禁止此IP/用戶建立新帳戶',
'block-log-flags-noautoblock'     => '停用自動封禁',
'block-log-flags-noemail'         => '禁止電子郵件',
'block-log-flags-angry-autoblock' => '加強自動封鎖已啟用',
'range_block_disabled'            => '只有管理員才能創建禁止查封的範圍。',
'ipb_expiry_invalid'              => '無效的終止時間。',
'ipb_expiry_temp'                 => '隱藏用戶名封鎖必須是永久性的。',
'ipb_already_blocked'             => '已經封鎖"$1"',
'ipb_cant_unblock'                => '錯誤: 找不到查封ID$1。可能已經解除封禁。',
'ipb_blocked_as_range'            => '錯誤: 該IP $1 無直接查封，不可以解除封禁。但是它是在 $2 的查封範圍之內，該段範圍是可以解除封禁的。',
'ip_range_invalid'                => '無效的IP範圍。\\n',
'blockme'                         => '查封我',
'proxyblocker'                    => '代理封鎖器',
'proxyblocker-disabled'           => '這個功能已經停用。',
'proxyblockreason'                => '您的IP位址是一個開放的代理，它已經被封鎖。請聯繫您的網際網路服務提供商或技術支援者並告知告知他們該嚴重的安全問題。',
'proxyblocksuccess'               => '完成。\\n',
'sorbsreason'                     => '您的IP位址在{{SITENAME}}中被 DNSBL列為屬於開放代理服務器。',
'sorbs_create_account_reason'     => '由於您的IP位址在{{SITENAME}}中被 DNSBL列為屬於開放代理服務器，所以您無法建立帳號。',

# Developer tools
'lockdb'              => '禁止更改資料庫',
'unlockdb'            => '開放更改資料庫',
'lockdbtext'          => '鎖住資料庫將禁止所有用戶進行編輯頁面、更改參數、編輯監視列表以及其他需要更改資料庫的操作。
請確認您的決定，並且保證您在維護工作結束後會重新開放資料庫。',
'unlockdbtext'        => '開放資料庫將會恢復所有用戶進行編輯頁面、修改參數、編輯監視列表以及其他需要更改資料庫的操作。
請確認您的決定。',
'lockconfirm'         => '是的，我確實想要封鎖資料庫。',
'unlockconfirm'       => '是的，我確實想要開放資料庫。',
'lockbtn'             => '資料庫上鎖',
'unlockbtn'           => '開放資料庫',
'locknoconfirm'       => '您並沒有勾選確認按鈕。',
'lockdbsuccesssub'    => '資料庫成功上鎖',
'unlockdbsuccesssub'  => '資料庫開放',
'lockdbsuccesstext'   => '{{SITENAME}}資料庫已經上鎖。
<br />請記住在維護完成後重新開放資料庫。',
'unlockdbsuccesstext' => '{{SITENAME}}資料庫重新開放。',
'lockfilenotwritable' => '資料庫鎖定檔案不可寫入。要鎖定和解鎖資料庫，該檔案必須對網路伺服器可寫入。',
'databasenotlocked'   => '資料庫沒有鎖定。',

# Move page
'move-page'               => '移動$1',
'move-page-legend'        => '移動頁面',
'movepagetext'            => "用下面的表單來重新命名一個頁面，並將其修訂歷史同時移動到新頁面。
老的頁面將成為新頁面的重定向頁。
您可以自動地更新指到原標題的重定向。
如果您選擇不去做的話，請檢查[[Special:DoubleRedirects|雙重]]或[[Special:BrokenRedirects|損壞重定向]]連結。
您應當負責確定所有連結依然會連到指定的頁面。

注意如果新頁面已經有內容的話，頁面將'''不會'''被移動，
除非新頁面無內容或是重定向頁，而且沒有修訂歷史。
這意味著您再必要時可以在移動到新頁面後再移回老的頁面，
同時您也無法覆蓋現有頁面。

<b>警告！</b>
對一個經常被訪問的頁面而言這可能是一個重大與唐突的更改；
請在行動前先了結其所可能帶來的後果。",
'movepagetalktext'        => "有關的對話頁（如果有的話）將被自動與該頁面一起移動，'''除非'''：
*您將頁面移動到不同的名字空間；
*新頁面已經有一個包含內容的對話頁，或者
*您不勾選下面的覆選框。

在這些情況下，您在必要時必須手工移動或合併頁面。",
'movearticle'             => '移動頁面:',
'movenotallowed'          => '您在{{SITENAME}}中度並沒有許可權去移動頁面。',
'newtitle'                => '新標題:',
'move-watch'              => '監視此頁',
'movepagebtn'             => '移動頁面',
'pagemovedsub'            => '移動成功',
'movepage-moved'          => "<big>'''「$1」已經移動到「$2」'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => '該名字的頁面已經存在，或者您選擇的名字無效。請再選一個名字。',
'cantmove-titleprotected' => '您不可以移動這個頁面到這個位置，因為該新標題已經被保護以防止建立。',
'talkexists'              => '頁面本身移動成功，
但是由於新標題下已經有對話頁存在，所以對話頁無法移動。請手工合併兩個頁面。',
'movedto'                 => '移動到',
'movetalk'                => '如果可能的話，同時移動對話頁。',
'move-subpages'           => '如果可能的話，同時移動所有的子頁面',
'move-talk-subpages'      => '如果可能的話，同時移動所有的子對話頁面',
'movepage-page-exists'    => '頁面$1已經存在，不可以自動地覆寫。',
'movepage-page-moved'     => '頁面$1已經移動到$2。',
'movepage-page-unmoved'   => '頁面$1不可以移動到$2。',
'movepage-max-pages'      => '最多有$1個頁面已經移動同時不可以自動地再移動更多。',
'1movedto2'               => '[[$1]]移動到[[$2]]',
'1movedto2_redir'         => '[[$1]]透過重定向移動到[[$2]]',
'movelogpage'             => '移動日誌',
'movelogpagetext'         => '以下是已經移動的頁面清單:',
'movereason'              => '原因',
'revertmove'              => '恢復該移動',
'delete_and_move'         => '刪除並移動',
'delete_and_move_text'    => '==需要刪除==

目標頁面"[[:$1]]"已經存在。你確認需要刪除原頁面並以進行移動嗎？',
'delete_and_move_confirm' => '是的，刪除此頁面',
'delete_and_move_reason'  => '刪除以便移動',
'selfmove'                => '原始標題與目標標題相同，您不能移動一頁覆蓋本身。',
'immobile_namespace'      => '目標標題屬於特別類型；不能將頁面移入這個名字空間。',
'imagenocrossnamespace'   => '不可以移動檔案到非檔案名字空間',
'imagetypemismatch'       => '該新副檔名不匹配它的類型',
'imageinvalidfilename'    => '目標檔案名稱是無效的',
'fix-double-redirects'    => '更新指到原先標題的任何重新定向',

# Export
'export'            => '匯出頁面',
'exporttext'        => '您可以將特定頁面或一組頁面的文本以及編輯歷史以 XML 格式匯出；這樣可以將有關頁面透過"[[Special:Import|匯入頁面]]"頁面匯入到另一個運行 MediaWiki 的網站。

要匯出頁面，請在下面的文字框中輸入頁面標題，每行一個標題，
並選擇你是否需要匯出帶有頁面歷史的以前的版本，
或是只選擇匯出帶有最後一次編輯訊息的當前版本。

此外你還可以利用連結匯出檔案，例如你可以使用[[{{MediaWiki:Mainpage}}]]匯出"[[{{MediaWiki:Mainpage}}]]"頁面。',
'exportcuronly'     => '僅包含當前的修訂，而不是全部的歷史。',
'exportnohistory'   => "----
'''注意:''' 由於性能原因，從此表單匯出頁面的全部歷史已被停用。",
'export-submit'     => '匯出',
'export-addcattext' => '由分類中加入頁面:',
'export-addcat'     => '加入',
'export-download'   => '另存為檔案',
'export-templates'  => '包含模版',

# Namespace 8 related
'allmessages'               => '系統介面',
'allmessagesname'           => '名稱',
'allmessagesdefault'        => '預設的文字',
'allmessagescurrent'        => '當前的文字',
'allmessagestext'           => '這裡列出所有可定製的系統介面。
如果想貢獻正宗的MediaWiki本地化的話，請參閱[http://www.mediawiki.org/wiki/Localisation MediaWiki本地化]以及[http://translatewiki.net Betawiki]。',
'allmessagesnotsupportedDB' => "這個頁面無法使用，因為'''\$wgUseDatabaseMessages'''已被設定關閉。",
'allmessagesfilter'         => '正則表達式過濾條件：',
'allmessagesmodified'       => '僅顯示修改過的',

# Thumbnails
'thumbnail-more'           => '放大',
'filemissing'              => '無法找到檔案',
'thumbnail_error'          => '創建縮圖錯誤: $1',
'djvu_page_error'          => 'DjVu頁面超出範圍',
'djvu_no_xml'              => '無法在DjVu檔案中擷取XML',
'thumbnail_invalid_params' => '不正確的縮圖參數',
'thumbnail_dest_directory' => '無法建立目標目錄',

# Special:Import
'import'                     => '匯入頁面',
'importinterwiki'            => '跨 wiki 匯入',
'import-interwiki-text'      => '選擇一個 wiki 和頁面標題以進行匯入。
修訂日期和編輯者名字將同時被儲存。
所有的跨 wiki 匯入操作被記錄在[[Special:Log/import|匯入日誌]]。',
'import-interwiki-history'   => '複製此頁的所有歷史版本',
'import-interwiki-submit'    => '匯入',
'import-interwiki-namespace' => '將頁面轉移到名字空間:',
'importtext'                 => '請使用[[Special:Export|匯出功能]]從源 wiki 匯出檔案，
儲存到您的磁片並上傳到這裡。',
'importstart'                => '正在匯入頁面...',
'import-revision-count'      => '$1個修訂',
'importnopages'              => '沒有匯入的頁面。',
'importfailed'               => '匯入失敗: <nowiki>$1</nowiki>',
'importunknownsource'        => '未知的源匯入類型',
'importcantopen'             => '無法打開匯入檔案',
'importbadinterwiki'         => '損壞的內部 wiki 連結',
'importnotext'               => '空或沒有文字',
'importsuccess'              => '匯入完成！',
'importhistoryconflict'      => '存在衝突的修訂歷史(可能在之前已經匯入過此頁面)',
'importnosources'            => '跨Wiki匯入源沒有定義，同時不允許直接的歷史上傳。',
'importnofile'               => '沒有上傳匯入檔案。',
'importuploaderrorsize'      => '上載匯入檔案失敗。檔案大於可以允許的上傳大小。',
'importuploaderrorpartial'   => '上載匯入檔案失敗。檔案只有部份已經上傳。',
'importuploaderrortemp'      => '上載匯入檔案失敗。臨時資料夾已遺失。',
'import-parse-failure'       => 'XML匯入語法失敗',
'import-noarticle'           => '沒有頁面作匯入！',
'import-nonewrevisions'      => '所有的修訂已經在先前匯入。',
'xml-error-string'           => '$1 於行$2，欄$3 ($4位元組): $5',
'import-upload'              => '上傳XML資料',

# Import log
'importlogpage'                    => '匯入日誌',
'importlogpagetext'                => '來自其它 wiki 的行政性的帶編輯歷史匯入頁面。',
'import-logentry-upload'           => '透過檔案上傳匯入的$1',
'import-logentry-upload-detail'    => '$1個修訂',
'import-logentry-interwiki'        => '跨 wiki $1',
'import-logentry-interwiki-detail' => '來自$2的$1個修訂',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '我的用戶頁',
'tooltip-pt-anonuserpage'         => '您編輯本站所用IP的對應用戶頁',
'tooltip-pt-mytalk'               => '我的對話頁',
'tooltip-pt-anontalk'             => '對於來自此IP地址編輯的對話',
'tooltip-pt-preferences'          => '我的參數設置',
'tooltip-pt-watchlist'            => '我的監視列表',
'tooltip-pt-mycontris'            => '我的貢獻列表',
'tooltip-pt-login'                => '建議您登入，儘管並非必須。',
'tooltip-pt-anonlogin'            => '建議您登入，儘管並非必須。',
'tooltip-pt-logout'               => '登出',
'tooltip-ca-talk'                 => '關於頁面正文的討論',
'tooltip-ca-edit'                 => '您可以編輯此頁，請在保存之前先預覽一下。',
'tooltip-ca-addsection'           => '於本討論頁增加新的討論主題',
'tooltip-ca-viewsource'           => '該頁面已被保護。你可以查看該頁原始碼。',
'tooltip-ca-history'              => '本頁面的早前版本。',
'tooltip-ca-protect'              => '保護該頁面',
'tooltip-ca-delete'               => '刪除本頁',
'tooltip-ca-undelete'             => '將這個頁面恢復到被刪除以前的狀態',
'tooltip-ca-move'                 => '移動本頁',
'tooltip-ca-watch'                => '將此頁面加入監視列表',
'tooltip-ca-unwatch'              => '將此頁面從監視列表中移除',
'tooltip-search'                  => '搜尋該網站',
'tooltip-search-go'               => '如果相同的標題存在的話便直接前往該頁面',
'tooltip-search-fulltext'         => '搜尋該文字的頁面',
'tooltip-p-logo'                  => '首頁',
'tooltip-n-mainpage'              => '訪問首頁',
'tooltip-n-portal'                => '關於本計劃、你可以做什麼、應該如何做',
'tooltip-n-currentevents'         => '提供當前新聞事件的背景資料',
'tooltip-n-recentchanges'         => '列出該網站中的最近修改',
'tooltip-n-randompage'            => '隨機載入一個頁面',
'tooltip-n-help'                  => '尋求幫助',
'tooltip-t-whatlinkshere'         => '列出所有與本頁相鏈的頁面',
'tooltip-t-recentchangeslinked'   => '頁面鏈出所有頁面的更改',
'tooltip-feed-rss'                => '訂閱本頁面歷史的RSS資訊',
'tooltip-feed-atom'               => '訂閱本頁面歷史的Atom訊息',
'tooltip-t-contributions'         => '查看該用戶的貢獻列表',
'tooltip-t-emailuser'             => '向該用戶發送電子郵件',
'tooltip-t-upload'                => '上傳檔案',
'tooltip-t-specialpages'          => '全部特殊頁面的列表',
'tooltip-t-print'                 => '這個頁面的可列印版本',
'tooltip-t-permalink'             => '這個頁面版本的永久連結',
'tooltip-ca-nstab-main'           => '查看頁面內容',
'tooltip-ca-nstab-user'           => '查看用戶頁',
'tooltip-ca-nstab-media'          => '查看多媒體檔案資訊頁面',
'tooltip-ca-nstab-special'        => '本頁面會隨著資料庫的數據即時更新，任何人均不能直接編輯',
'tooltip-ca-nstab-project'        => '查看項目頁面',
'tooltip-ca-nstab-image'          => '查看檔案頁面',
'tooltip-ca-nstab-mediawiki'      => '查看系統資訊',
'tooltip-ca-nstab-template'       => '查看模板',
'tooltip-ca-nstab-help'           => '查看幫助頁面',
'tooltip-ca-nstab-category'       => '查看分類頁面',
'tooltip-minoredit'               => '標記為小修改',
'tooltip-save'                    => '保存您的修改',
'tooltip-preview'                 => '預覽您的編輯，請先使用本功能再保存！',
'tooltip-diff'                    => '顯示您對頁面的貢獻',
'tooltip-compareselectedversions' => '查看本頁被點選的兩個版本間的差異',
'tooltip-watch'                   => '將此頁加入您的監視列表',
'tooltip-recreate'                => '重建該頁面，無論是否被刪除。',
'tooltip-upload'                  => '開始上傳',

# Stylesheets
'common.css'      => '/* 此處的 CSS 將應用於所有的面板 */',
'standard.css'    => '/* 此處的 CSS 將影響使用標準面板的用戶 */',
'nostalgia.css'   => '/* 此處的 CSS 將影響使用懷舊面板的用戶 */',
'cologneblue.css' => '/* 此處的 CSS 將影響使用科隆香水藍面板的用戶 */',
'monobook.css'    => '/* 此處的 CSS 將影響使用 Monobook 面板的用戶 */',
'myskin.css'      => '/* 此處的 CSS 將影響使用 Myskin 面板的用戶 */',
'chick.css'       => '/* 此處的 CSS 將影響使用 Chick 面板的用戶 */',
'simple.css'      => '/* 此處的 CSS 將影響使用 Simple 面板的用戶 */',
'modern.css'      => '/* 此處的 CSS 將影響使用 Modern 面板的用戶 */',

# Scripts
'common.js'      => '/* 此處的JavaScript將載入於所有用戶每一個頁面。 */',
'standard.js'    => '/* 此處的JavaScript將載入於使用標準面板的用戶 */',
'nostalgia.js'   => '/* 此處的JavaScript將載入於使用懷舊面板的用戶 */',
'cologneblue.js' => '/* 此處的JavaScript將載入於使用科隆香水藍面板的用戶 */',
'monobook.js'    => '/* 此處的JavaScript將載入於使用Monobook面板的用戶 */',
'myskin.js'      => '/* 此處的JavaScript將載入於使用Myskin面板的用戶 */',
'chick.js'       => '/* 此處的JavaScript將載入於使用Chick面板的用戶 */',
'simple.js'      => '/* 此處的JavaScript將載入於使用Simple面板的用戶 */',
'modern.js'      => '/* 此處的JavaScript將載入於使用Modern面板的用戶 */',

# Metadata
'nodublincore'      => 'Dublin Core RDF 元數據在該伺服器不可使用。',
'nocreativecommons' => 'Creative Commons RDF 元數據在該伺服器不可使用。',
'notacceptable'     => '該網站伺服器不能提供您的客戶端能識別的數據格式。',

# Attribution
'anonymous'        => '{{SITENAME}}的匿名用戶',
'siteuser'         => '{{SITENAME}}用戶$1',
'lastmodifiedatby' => '此頁由$3於$1 $2的最後更改。', # $1 date, $2 time, $3 user
'othercontribs'    => '在$1的工作基礎上。',
'others'           => '其他',
'siteusers'        => '{{SITENAME}}用戶$1',
'creditspage'      => '頁面致謝',
'nocredits'        => '該頁沒有致謝名單訊息。',

# Spam protection
'spamprotectiontitle' => '垃圾過濾器',
'spamprotectiontext'  => '垃圾過濾器禁止保存您剛才提交的頁面，這可能是由於您所加入的外部網站鏈接所產生的問題。',
'spamprotectionmatch' => '觸發了我們的垃圾過濾器的文本如下：$1',
'spambot_username'    => 'MediaWiki 廣告清除',
'spam_reverting'      => '恢復到不包含連結至$1的最近版本',
'spam_blanking'       => '所有包含連結至$1的修訂，清空',

# Info page
'infosubtitle'   => '頁面訊息',
'numedits'       => '編輯數 (頁面): $1',
'numtalkedits'   => '編輯數 (討論頁): $1',
'numwatchers'    => '監視者數目: $1',
'numauthors'     => '作者數量 (頁面): $1',
'numtalkauthors' => '作者數量 (討論頁): $1',

# Math options
'mw_math_png'    => '永遠使用PNG圖像',
'mw_math_simple' => '如果是簡單的公式使用HTML，否則使用PNG圖像',
'mw_math_html'   => '如果可以用HTML，否則用PNG圖像',
'mw_math_source' => '顯示為TeX代碼 (使用文字瀏覽器時)',
'mw_math_modern' => '推薦為新版瀏覽器使用',
'mw_math_mathml' => '儘可能使用MathML (試驗中)',

# Patrolling
'markaspatrolleddiff'                 => '標記為已檢查',
'markaspatrolledtext'                 => '標記此頁面為已檢查',
'markedaspatrolled'                   => '標記為已檢查',
'markedaspatrolledtext'               => '選定的版本已被標記為已檢查.',
'rcpatroldisabled'                    => '最新更改檢查被關閉',
'rcpatroldisabledtext'                => '最新更改檢查的功能目前已關閉。',
'markedaspatrollederror'              => '不能標誌為已檢查',
'markedaspatrollederrortext'          => '你需要指定某個版本才能標誌為已檢查。',
'markedaspatrollederror-noautopatrol' => '您無法將你自己所作的更改標記為已檢查。',

# Patrol log
'patrol-log-page'   => '巡查日誌',
'patrol-log-header' => '這個是已經巡查過的日誌。',
'patrol-log-line'   => '$2的版本$1已被標記為已檢查的$3',
'patrol-log-auto'   => '(自動)',

# Image deletion
'deletedrevision'                 => '已刪除舊版本$1',
'filedeleteerror-short'           => '刪除檔案發生錯誤: $1',
'filedeleteerror-long'            => '當刪除檔案時遇到錯誤:

$1',
'filedelete-missing'              => '因為檔案 "$1" 不存在，所以它不可以刪除。',
'filedelete-old-unregistered'     => '所指定的檔案修訂 "$1" 在資料庫中不存在。',
'filedelete-current-unregistered' => '所指定的檔案 "$1" 在資料庫中不存在。',
'filedelete-archive-read-only'    => '存檔目錄 "$1" 在網頁伺服器中不可寫。',

# Browsing diffs
'previousdiff' => '←上一個',
'nextdiff'     => '下一個→',

# Media information
'mediawarning'         => "'''警告''': 該檔案可能包含惡意代碼，執行它可能對您的系統帶來危險。<hr />",
'imagemaxsize'         => '在圖像描述頁對檔案大小限制為:',
'thumbsize'            => '略圖大小:',
'widthheightpage'      => '$1×$2, $3頁',
'file-info'            => '(檔案大小: $1, MIME 類型: $2)',
'file-info-size'       => '($1 × $2 像素，檔案大小：$3 ，MIME類型：$4)',
'file-nohires'         => '<small>無更高解像度可提供。</small>',
'svg-long-desc'        => '(SVG檔案，表面大小： $1 × $2 像素，檔案大小：$3)',
'show-big-image'       => '完整解像度',
'show-big-image-thumb' => '<small>這幅縮圖的解像度: $1 × $2 像素</small>',

# Special:NewImages
'newimages'             => '新建圖像畫廊',
'imagelisttext'         => "以下是按$2排列的'''$1'''個檔案列表。",
'newimages-summary'     => '這個特殊頁面中顯示最後已上傳的檔案。',
'showhidebots'          => '($1機器人)',
'noimages'              => '無可檢視圖像。',
'ilsubmit'              => '搜尋',
'bydate'                => '按日期',
'sp-newimages-showfrom' => '從$1 $2開始顯示新檔案',

# Bad image list
'bad_image_list' => '請根據以下的格式去編寫:

只有列示項目（以 * 開頭的項目）會被考慮。第一個連結一定要連接去壞檔案中。
然後在同一行的連結會考慮作例外，即是該檔案可以在哪一個頁面中同時顯示。',

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => '簡體',
'variantname-zh-hant' => '繁體',
'variantname-zh-cn'   => '大陸簡體',
'variantname-zh-tw'   => '台灣正體',
'variantname-zh-hk'   => '香港繁體',
'variantname-zh-sg'   => '新加坡簡體',
'variantname-zh'      => '不轉換',

# Metadata
'metadata'          => '元數據',
'metadata-help'     => '此檔案中包含有擴展的訊息。這些訊息可能是由數位相機或掃描儀在創建或數字化過程中所添加的。

如果此檔案的源檔案已經被修改，一些訊息在修改後的檔案中將不能完全反映出來。',
'metadata-expand'   => '顯示詳細資料',
'metadata-collapse' => '隱藏詳細資料',
'metadata-fields'   => '在本訊息中所列出的 EXIF 元數據域將包含在圖片顯示頁面,
當元數據表損壞時只顯示以下訊息，其他的元數據預設為隱藏。
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => '寬度',
'exif-imagelength'                 => '高度',
'exif-bitspersample'               => '每象素比特數',
'exif-compression'                 => '壓縮方案',
'exif-photometricinterpretation'   => '象素合成',
'exif-orientation'                 => '方位',
'exif-samplesperpixel'             => '象素數',
'exif-planarconfiguration'         => '數據排列',
'exif-ycbcrsubsampling'            => '黃色對洋紅二次抽樣比率',
'exif-ycbcrpositioning'            => '黃色和洋紅配置',
'exif-xresolution'                 => '水準分辨率',
'exif-yresolution'                 => '垂直分辨率',
'exif-resolutionunit'              => 'X 軸與 Y 軸分辨率單位',
'exif-stripoffsets'                => '圖像數據區',
'exif-rowsperstrip'                => '每帶行數',
'exif-stripbytecounts'             => '每壓縮帶位元組數',
'exif-jpeginterchangeformat'       => 'JPEG SOI 偏移',
'exif-jpeginterchangeformatlength' => 'JPEG 數據位元組',
'exif-transferfunction'            => '轉移功能',
'exif-whitepoint'                  => '白點色度',
'exif-primarychromaticities'       => '主要色度',
'exif-ycbcrcoefficients'           => '顏色空間轉換矩陣系數',
'exif-referenceblackwhite'         => '黑白參照值對',
'exif-datetime'                    => '檔案更改日期和時間',
'exif-imagedescription'            => '圖像標題',
'exif-make'                        => '照相機製造商',
'exif-model'                       => '照相機型號',
'exif-software'                    => '所用軟體',
'exif-artist'                      => '作者',
'exif-copyright'                   => '版權所有者',
'exif-exifversion'                 => 'Exif 版本',
'exif-flashpixversion'             => '支援的 Flashpix 版本',
'exif-colorspace'                  => '顏色空間',
'exif-componentsconfiguration'     => '每分量含義',
'exif-compressedbitsperpixel'      => '圖像壓縮模式',
'exif-pixelydimension'             => '有效圖像寬度',
'exif-pixelxdimension'             => '有效圖像高度',
'exif-makernote'                   => '製造商註釋',
'exif-usercomment'                 => '用戶註釋',
'exif-relatedsoundfile'            => '相關的音頻檔案',
'exif-datetimeoriginal'            => '數據產生時間',
'exif-datetimedigitized'           => '數字化處理時間',
'exif-subsectime'                  => '日期時間秒',
'exif-subsectimeoriginal'          => '原始日期時間秒',
'exif-subsectimedigitized'         => '數字化日期時間秒',
'exif-exposuretime'                => '曝光時間',
'exif-exposuretime-format'         => '$1 秒 ($2)',
'exif-fnumber'                     => '光圈(F值)',
'exif-exposureprogram'             => '曝光模式',
'exif-spectralsensitivity'         => '感光',
'exif-isospeedratings'             => 'ISO 速率',
'exif-oecf'                        => '光電轉換因子',
'exif-shutterspeedvalue'           => '快門速度',
'exif-aperturevalue'               => '光圈',
'exif-brightnessvalue'             => '亮度',
'exif-exposurebiasvalue'           => '曝光補償',
'exif-maxaperturevalue'            => '最大陸地光圈',
'exif-subjectdistance'             => '物距',
'exif-meteringmode'                => '測量模式',
'exif-lightsource'                 => '光源',
'exif-flash'                       => '閃光燈',
'exif-focallength'                 => '焦距',
'exif-subjectarea'                 => '主體區域',
'exif-flashenergy'                 => '閃光燈強度',
'exif-spatialfrequencyresponse'    => '空間頻率附應',
'exif-focalplanexresolution'       => 'X軸焦平面分辨率',
'exif-focalplaneyresolution'       => 'Y軸焦平面分辨率',
'exif-focalplaneresolutionunit'    => '焦平面分辨率單位',
'exif-subjectlocation'             => '主題位置',
'exif-exposureindex'               => '曝光指數',
'exif-sensingmethod'               => '感光模式',
'exif-filesource'                  => '檔案源',
'exif-scenetype'                   => '場景類型',
'exif-cfapattern'                  => 'CFA 模式',
'exif-customrendered'              => '自定義圖像處理',
'exif-exposuremode'                => '曝光模式',
'exif-whitebalance'                => '白平衡',
'exif-digitalzoomratio'            => '數字變焦比率',
'exif-focallengthin35mmfilm'       => '35毫米膠片焦距',
'exif-scenecapturetype'            => '情景拍攝類型',
'exif-gaincontrol'                 => '場景控制',
'exif-contrast'                    => '對比度',
'exif-saturation'                  => '飽和度',
'exif-sharpness'                   => '銳化',
'exif-devicesettingdescription'    => '設備設定描述',
'exif-subjectdistancerange'        => '主體距離範圍',
'exif-imageuniqueid'               => '唯一圖像ID',
'exif-gpsversionid'                => 'GPS 標籤(tag)版本',
'exif-gpslatituderef'              => '北緯或南緯',
'exif-gpslatitude'                 => '緯度',
'exif-gpslongituderef'             => '東經或西經',
'exif-gpslongitude'                => '經度',
'exif-gpsaltituderef'              => '海拔正負參照',
'exif-gpsaltitude'                 => '海拔',
'exif-gpstimestamp'                => 'GPS 時間(原子時鐘)',
'exif-gpssatellites'               => '測量使用的衛星',
'exif-gpsstatus'                   => '接收器狀態',
'exif-gpsmeasuremode'              => '測量模式',
'exif-gpsdop'                      => '測量精度',
'exif-gpsspeedref'                 => '速度單位',
'exif-gpsspeed'                    => 'GPS 接收器速度',
'exif-gpstrackref'                 => '運動方位參照',
'exif-gpstrack'                    => '運動方位',
'exif-gpsimgdirectionref'          => '圖像方位參照',
'exif-gpsimgdirection'             => '圖像方位',
'exif-gpsmapdatum'                 => '使用地理測繪數據',
'exif-gpsdestlatituderef'          => '目標緯度參照',
'exif-gpsdestlatitude'             => '目標緯度',
'exif-gpsdestlongituderef'         => '目標經度的參照',
'exif-gpsdestlongitude'            => '目標經度',
'exif-gpsdestbearingref'           => '目標方位參照',
'exif-gpsdestbearing'              => '目標方位',
'exif-gpsdestdistanceref'          => '目標距離參照',
'exif-gpsdestdistance'             => '目標距離',
'exif-gpsprocessingmethod'         => 'GPS 處理方法名稱',
'exif-gpsareainformation'          => 'GPS 區域名稱',
'exif-gpsdatestamp'                => 'GPS 日期',
'exif-gpsdifferential'             => 'GPS 差動修正',

# EXIF attributes
'exif-compression-1' => '未壓縮',

'exif-unknowndate' => '未知的日期',

'exif-orientation-1' => '標準', # 0th row: top; 0th column: left
'exif-orientation-2' => '水準翻轉', # 0th row: top; 0th column: right
'exif-orientation-3' => '旋轉180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => '垂直翻轉', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '旋轉90° 逆時針並垂直翻轉', # 0th row: left; 0th column: top
'exif-orientation-6' => '旋轉90° 順時針', # 0th row: right; 0th column: top
'exif-orientation-7' => '旋轉90° 順時針並垂直翻轉', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '旋轉90° 逆時針', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => '矮胖格式',
'exif-planarconfiguration-2' => '平面格式',

'exif-componentsconfiguration-0' => '不存在',

'exif-exposureprogram-0' => '未定義',
'exif-exposureprogram-1' => '手動',
'exif-exposureprogram-2' => '標準程式',
'exif-exposureprogram-3' => '光圈優先模式',
'exif-exposureprogram-4' => '快門優先模式',
'exif-exposureprogram-5' => '藝術程式(景深優先)',
'exif-exposureprogram-6' => '運動程式(快速快門速度優先)',
'exif-exposureprogram-7' => '肖像模式(適用於背景在焦距以外的近距攝影)',
'exif-exposureprogram-8' => '風景模式(適用於背景在焦距上的風景照片)',

'exif-subjectdistance-value' => '$1米',

'exif-meteringmode-0'   => '未知',
'exif-meteringmode-1'   => '平均水準',
'exif-meteringmode-2'   => '中心加權平均測量',
'exif-meteringmode-3'   => '點測',
'exif-meteringmode-4'   => '多點測',
'exif-meteringmode-5'   => '模式測量',
'exif-meteringmode-6'   => '局部測量',
'exif-meteringmode-255' => '其它',

'exif-lightsource-0'   => '未知',
'exif-lightsource-1'   => '日光燈',
'exif-lightsource-2'   => '熒光燈',
'exif-lightsource-3'   => '鎢絲燈(白熾燈)',
'exif-lightsource-4'   => '閃光燈',
'exif-lightsource-9'   => '晴天',
'exif-lightsource-10'  => '多雲',
'exif-lightsource-11'  => '深色調陰影',
'exif-lightsource-12'  => '日光熒光燈(色溫 D 5700    7100K)',
'exif-lightsource-13'  => '日溫白色熒光燈(N 4600    5400K)',
'exif-lightsource-14'  => '冷白色熒光燈(W 3900    4500K)',
'exif-lightsource-15'  => '白色熒光 (WW 3200    3700K)',
'exif-lightsource-17'  => '標準燈光A',
'exif-lightsource-18'  => '標準燈光B',
'exif-lightsource-19'  => '標準燈光C',
'exif-lightsource-24'  => 'ISO攝影棚鎢燈',
'exif-lightsource-255' => '其他光源',

'exif-focalplaneresolutionunit-2' => '英寸',

'exif-sensingmethod-1' => '未定義',
'exif-sensingmethod-2' => '一塊彩色區域傳感器',
'exif-sensingmethod-3' => '兩塊彩色區域傳感器',
'exif-sensingmethod-4' => '三塊彩色區域傳感器',
'exif-sensingmethod-5' => '連續彩色區域傳感器',
'exif-sensingmethod-7' => '三線傳感器',
'exif-sensingmethod-8' => '連續彩色線性傳感器',

'exif-scenetype-1' => '直接照像圖片',

'exif-customrendered-0' => '標準處理',
'exif-customrendered-1' => '自定義處理',

'exif-exposuremode-0' => '自動曝光',
'exif-exposuremode-1' => '手動曝光',
'exif-exposuremode-2' => '自動曝光感知調節',

'exif-whitebalance-0' => '自動白平衡',
'exif-whitebalance-1' => '手動白平衡',

'exif-scenecapturetype-0' => '標準',
'exif-scenecapturetype-1' => '風景',
'exif-scenecapturetype-2' => '肖像',
'exif-scenecapturetype-3' => '夜景',

'exif-gaincontrol-0' => '無',
'exif-gaincontrol-1' => '低增益',
'exif-gaincontrol-2' => '高增益',
'exif-gaincontrol-3' => '低減益',
'exif-gaincontrol-4' => '高減益',

'exif-contrast-0' => '標準',
'exif-contrast-1' => '低',
'exif-contrast-2' => '高',

'exif-saturation-0' => '標準',
'exif-saturation-1' => '低飽和度',
'exif-saturation-2' => '高飽和度',

'exif-sharpness-0' => '標準',
'exif-sharpness-1' => '低',
'exif-sharpness-2' => '高',

'exif-subjectdistancerange-0' => '未知',
'exif-subjectdistancerange-1' => '自動處理程式(宏)',
'exif-subjectdistancerange-2' => '近景',
'exif-subjectdistancerange-3' => '遠景',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => '北緯',
'exif-gpslatitude-s' => '南緯',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => '東經',
'exif-gpslongitude-w' => '西經',

'exif-gpsstatus-a' => '測量過程',
'exif-gpsstatus-v' => '互動測量',

'exif-gpsmeasuremode-2' => '二維測量',
'exif-gpsmeasuremode-3' => '三維測量',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => '公里每小時',
'exif-gpsspeed-m' => '英里每小時',
'exif-gpsspeed-n' => '海里每小時(節)',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => '真方位',
'exif-gpsdirection-m' => '地磁方位',

# External editor support
'edit-externally'      => '用外部程式編輯此檔案',
'edit-externally-help' => '請參見[http://www.mediawiki.org/wiki/Manual:External_editors 設定步驟]了解詳細資訊。',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => '全部',
'imagelistall'     => '全部',
'watchlistall2'    => '全部',
'namespacesall'    => '全部',
'monthsall'        => '全部',

# E-mail address confirmation
'confirmemail'             => '確認郵箱位址',
'confirmemail_noemail'     => '您沒有在您的[[Special:Preferences|用戶設定]]裡面輸入一個有效的 email 位址。',
'confirmemail_text'        => '{{SITENAME}}要求您在使用郵件功能之前驗證您的郵箱位址。
點擊以下按鈕可向您的郵箱發送一封確認郵件。該郵件包含有一行代碼連結；
請在您的瀏覽器中加載此連結以確認您的郵箱位址是有效的。',
'confirmemail_pending'     => '<div class="error">
一個確認代碼已經被發送到您的郵箱，您可能需要等幾分鐘才能收到。如果無法收到，請在申請一個新的確認碼！
</div>',
'confirmemail_send'        => '郵發確認代碼',
'confirmemail_sent'        => '確認郵件已發送。',
'confirmemail_oncreate'    => '一個確認代碼已經被發送到您的郵箱。該代碼並不要求您進行登錄，
但若您要啟用在此 wiki 上的任何基於電子郵件的功能，您必須先提交此代碼。',
'confirmemail_sendfailed'  => '不能發送確認郵件，請檢查郵箱位址是否包含非法字元。

郵件傳送員回應: $1',
'confirmemail_invalid'     => '無效的確認碼，該代碼可能已經過期。',
'confirmemail_needlogin'   => '您需要$1以確認您的郵箱位址。',
'confirmemail_success'     => '您的郵箱已經被確認。您現在可以[[Special:UserLogin|登錄]]並使用此網站了。',
'confirmemail_loggedin'    => '您的郵箱位址現下已被確認。',
'confirmemail_error'       => '你的確認過程發生錯誤。',
'confirmemail_subject'     => '{{SITENAME}}郵箱位址確認',
'confirmemail_body'        => '擁有IP位址$1的用戶(可能是您)在{{SITENAME}}創建了帳戶"$2"，並提交了您的電子郵箱位址。

請確認這個帳戶是屬於您的，並同時啟用在{{SITENAME}}上的
電子郵件功能。請在瀏覽器中打開下面的連結:

$3

如果您*未*註冊賬戶，
請打開下面的連結去取消電子郵件確認:

$5

確認碼會在$4過期。',
'confirmemail_invalidated' => '電郵地址確認已取消',
'invalidateemail'          => '取消電郵確認',

# Scary transclusion
'scarytranscludedisabled' => '[跨wiki轉換代碼不可用]',
'scarytranscludefailed'   => '[模板$1讀取失敗]',
'scarytranscludetoolong'  => '[URL 地址太長]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">此頁面的引用:
$1
</div>',
'trackbackremove'   => '([$1删除])',
'trackbacklink'     => '迴響',
'trackbackdeleteok' => 'Trackback 刪除成功。',

# Delete conflict
'deletedwhileediting' => '警告: 此頁在您開始編輯之後已經被刪除﹗',
'confirmrecreate'     => '在您編輯這個頁面後，用戶[[User:$1|$1]]([[User talk:$1|對話]])以下列原因刪除了這個頁面: $2。
請確認在您重新創建頁面前三思。',
'recreate'            => '重建',

# HTML dump
'redirectingto' => '重定向到[[:$1]]...',

# action=purge
'confirm_purge'        => '要清除此頁面的快取嗎?\\n\\n$1',
'confirm_purge_button' => '確定',

# AJAX search
'searchcontaining' => "搜索包含''$1''的頁面。",
'searchnamed'      => "搜索名稱為''$1''的頁面。",
'articletitles'    => '頁面以"$1"開頭',
'hideresults'      => '隱藏結果',
'useajaxsearch'    => '使用AJAX搜索',

# Multipage image navigation
'imgmultipageprev' => '← 上一頁',
'imgmultipagenext' => '下一頁 →',
'imgmultigo'       => '確定！',
'imgmultigoto'     => '到第$1頁',

# Table pager
'ascending_abbrev'         => '遞增',
'descending_abbrev'        => '遞減',
'table_pager_next'         => '下一頁',
'table_pager_prev'         => '上一頁',
'table_pager_first'        => '第一頁',
'table_pager_last'         => '最末頁',
'table_pager_limit'        => '每頁顯示 $1 筆記錄',
'table_pager_limit_submit' => '送出',
'table_pager_empty'        => '沒有結果',

# Auto-summaries
'autosumm-blank'   => '移除所有頁面內容',
'autosumm-replace' => "正在將頁面替換為 '$1'",
'autoredircomment' => '正在重定向到 [[$1]]',
'autosumm-new'     => '新頁面: $1',

# Size units
'size-bytes' => '$1 位元組',

# Live preview
'livepreview-loading' => '正在載入…',
'livepreview-ready'   => '正在載入… 完成!',
'livepreview-failed'  => '實時預覽失敗! 嘗試標準預覽。',
'livepreview-error'   => '連接失敗: $1 "$2" 嘗試標準預覽。',

# Friendlier slave lag warnings
'lag-warn-normal' => '過去$1秒內的更改未必會在這個清單中顯示。',
'lag-warn-high'   => '由於資料庫的過度延遲，過去$1秒內的更改未必會在這個清單中顯示。',

# Watchlist editor
'watchlistedit-numitems'       => '您的監視列表中共有$1個標題，當中不包括對話頁面。',
'watchlistedit-noitems'        => '您的監視列表並無標題。',
'watchlistedit-normal-title'   => '編輯監視列表',
'watchlistedit-normal-legend'  => '從監視列表中移除標題',
'watchlistedit-normal-explain' => '在您的監視列表中的標題在下面顯示。要移除一個標題，在它前面剔一下，接著點擊移除標題。您亦都可以[[Special:Watchlist/raw|編輯原始監視列表]]。',
'watchlistedit-normal-submit'  => '移除標題',
'watchlistedit-normal-done'    => '$1個標題已經從您的監視列表中移除:',
'watchlistedit-raw-title'      => '編輯原始監視列表',
'watchlistedit-raw-legend'     => '編輯原始監視列表',
'watchlistedit-raw-explain'    => '您的監視列表中的標題在下面顯示，同時亦都可以透過編輯這個表去加入以及移除標題；一行一個標題。當完成以後，點擊更新監視列表。你亦都可以去用[[Special:Watchlist/edit|標準編輯器]]。',
'watchlistedit-raw-titles'     => '標題:',
'watchlistedit-raw-submit'     => '更新監視列表',
'watchlistedit-raw-done'       => '您的監視列表已經更新。',
'watchlistedit-raw-added'      => '已經加入了$1個標題:',
'watchlistedit-raw-removed'    => '已經移除了$1個標題:',

# Watchlist editing tools
'watchlisttools-view' => '檢視有關更改',
'watchlisttools-edit' => '檢視並編輯監視列表',
'watchlisttools-raw'  => '編輯原始監視列表',

# Core parser functions
'unknown_extension_tag' => '不明的擴展標籤 "$1"',

# Special:Version
'version'                          => '版本', # Not used as normal message but as header for the special page itself
'version-extensions'               => '已經安裝的擴展',
'version-specialpages'             => '特殊頁面',
'version-parserhooks'              => '語法鈎',
'version-variables'                => '變數',
'version-other'                    => '其它',
'version-mediahandlers'            => '媒體處理器',
'version-hooks'                    => '鈎',
'version-extension-functions'      => '擴展函數',
'version-parser-extensiontags'     => '語法擴展標籤',
'version-parser-function-hooks'    => '語法函數鈎',
'version-skin-extension-functions' => '面版擴展函數',
'version-hook-name'                => '鈎名',
'version-hook-subscribedby'        => '利用於',
'version-version'                  => '版本',
'version-license'                  => '授權',
'version-software'                 => '已經安裝的軟件',
'version-software-product'         => '產品',
'version-software-version'         => '版本',

# Special:FilePath
'filepath'         => '檔案路徑',
'filepath-page'    => '檔案名:',
'filepath-submit'  => '搜尋路徑',
'filepath-summary' => '這個特殊頁面擷取一個檔案的完整路徑。圖像會以完整的解像度顯示，其它的檔案類型會以同它們已關聯程式啟動。

請輸入檔名，不要包含"{{ns:image}}:"開頭。',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => '選擇重覆檔案',
'fileduplicatesearch-summary'  => '用重覆檔案的切細值去找出檔案是否重覆。

輸入檔名時不需要輸入 "{{ns:image}}:" 開頭。',
'fileduplicatesearch-legend'   => '找重覆',
'fileduplicatesearch-filename' => '檔案名稱:',
'fileduplicatesearch-submit'   => '找',
'fileduplicatesearch-info'     => '$1 × $2 像素<br />檔案大小: $3<br />MIME類型: $4',
'fileduplicatesearch-result-1' => '檔案 "$1" 無完全相同的重覆。',
'fileduplicatesearch-result-n' => '檔案 "$1" 有$2項完全相同的重覆。',

# Special:SpecialPages
'specialpages'                   => '特殊頁面',
'specialpages-note'              => '----
* 標準特殊頁面。
* <span class="mw-specialpagerestricted">有限制的特殊頁面。</span>',
'specialpages-group-maintenance' => '維護報告',
'specialpages-group-other'       => '其它特殊頁面',
'specialpages-group-login'       => '登入／創建',
'specialpages-group-changes'     => '最近更改和日誌',
'specialpages-group-media'       => '媒體報告和上傳',
'specialpages-group-users'       => '用戶和權限',
'specialpages-group-highuse'     => '高度使用頁面',
'specialpages-group-pages'       => '頁面清單',
'specialpages-group-pagetools'   => '頁面工具',
'specialpages-group-wiki'        => 'Wiki資料和工具',
'specialpages-group-redirects'   => '重新定向特殊頁面',
'specialpages-group-spam'        => '反垃圾工具',

# Special:BlankPage
'blankpage'              => '空白頁面',
'intentionallyblankpage' => '這個頁面是為空白，以用來作測試速度等用途',

);
