<?php
/** Gan (贛語)
 *
 * @ingroup Language
 * @file
 *
 * @author Symane
 * @author Vipuser
 */

$fallback = 'zh-hant';

$messages = array(
# User preference toggles
'tog-underline'               => '下劃連結',
'tog-highlightbroken'         => '非法連結格式<a href="" class="new">像咁樣</a> (或者像咁樣<a href="" class="internal">?</a>).',
'tog-justify'                 => '對到段落',
'tog-hideminor'               => '該朝子嗰改動弆到嗰細修改',
'tog-extendwatchlist'         => '增加監視清單來顯示全部用得正嗰改動',
'tog-usenewrc'                => '加強該朝子嗰改動(JavaScript)',
'tog-numberheadings'          => '標題自動編號',
'tog-showtoolbar'             => '顯示編輯工具欄(JavaScript)',
'tog-editondblclick'          => '按兩下改吖(JavaScript)',
'tog-editsection'             => '可以用[編寫]連結來編寫個別段落',
'tog-editsectiononrightclick' => '可以按右鍵來編寫隻把子段落(JavaScript)',
'tog-showtoc'                 => '超過三隻標題就顯到目錄',
'tog-rememberpassword'        => '到箇隻電腦記到偶嗰密碼',
'tog-editwidth'               => '編輯欄嗰最闊度',
'tog-watchcreations'          => '拿偶開嘞嗰頁面加到偶嗰監視列表',
'tog-watchdefault'            => '拿偶改嘞嗰頁面加到偶嗰監視列表',
'tog-watchmoves'              => '拿偶動嘞嗰頁面加到偶嗰監視列表',
'tog-watchdeletion'           => '拿偶刪撇嗰頁面加到偶嗰監視列表',
'tog-minordefault'            => '全部嗰編輯設成細修改',
'tog-previewontop'            => '到編輯框嗰上首顯示預覽',
'tog-previewonfirst'          => '頭道修改時顯示預覽',
'tog-nocache'                 => '停用頁面嗰緩存',
'tog-enotifwatchlistpages'    => '偶監視框嗰頁面一有改動發電子郵件到偶',
'tog-enotifusertalkpages'     => '偶對話框嗰頁面一有改動發email到偶',
'tog-enotifminoredits'        => '有細嗰改動都要發email到偶',
'tog-enotifrevealaddr'        => '通知郵件可話到人聽偶嗰email地址',
'tog-shownumberswatching'     => '顯示有幾多人監視',
'tog-fancysig'                => '原始簽名（冇有自動連接）',
'tog-externaleditor'          => '默認用外部編輯器（專家用嗰功能，要在您嗰電腦上特別設置一下）',
'tog-externaldiff'            => '默認用外部差異比較器（專家用嗰功能，要在您嗰電腦上特別設置一下）',
'tog-showjumplinks'           => '啟用“跳到”訪問連結',
'tog-uselivepreview'          => '使用即時預覽(JavaScript)（實驗中）',
'tog-forceeditsummary'        => '冇改動注解時要同偶話',
'tog-watchlisthideown'        => '監視列表弆到偶嗰編輯',
'tog-watchlisthidebots'       => '監視列表弆到機器人嗰編輯',
'tog-watchlisthideminor'      => '監視列表弆到細修改',
'tog-nolangconversion'        => '嫑字轉換',
'tog-ccmeonemails'            => '偶發email到人家時也發封副本到偶',
'tog-diffonly'                => '比較兩隻版本差異時伓顯示文章內容',
'tog-showhiddencats'          => '顯示弆到嗰分類',

'underline-always'  => '總歸要用',
'underline-never'   => '絕伓使用',
'underline-default' => '瀏覽器預設',

'skinpreview' => '（預覽）',

# Dates
'sunday'        => '禮拜天',
'monday'        => '禮拜一',
'tuesday'       => '禮拜二',
'wednesday'     => '禮拜三',
'thursday'      => '禮拜四',
'friday'        => '禮拜五',
'saturday'      => '禮拜六',
'sun'           => '禮拜天',
'mon'           => '禮拜一',
'tue'           => '禮拜二',
'wed'           => '禮拜三',
'thu'           => '禮拜四',
'fri'           => '禮拜五',
'sat'           => '禮拜六',
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
'pagecategories'           => '$1隻分類',
'category_header'          => '“$1”分類中嗰文章',
'subcategories'            => '亞分類',
'category-media-header'    => '“$1”分類中嗰媒體',
'category-empty'           => '“箇隻分類有包到任何文章或媒體”',
'hidden-categories'        => '{{PLURAL:$1|隻隱藏分類|隻隱藏分類}}',
'hidden-category-category' => '弆到嗰分類', # Name of the category where hidden categories will be listed
'listingcontinuesabbrev'   => '續',

'mainpagetext'      => "<big>'''安裝正MediaWiki嘍!'''</big>",
'mainpagedocfooter' => '參看[http://meta.wikimedia.org/wiki/Help:Contents 用戶指南]裡頭會話到啷用wiki軟件

== 開始使用 ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings MediaWiki 配置設定列表]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki 平常問題解答]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki 發佈email清單]',

'about'          => '關於',
'article'        => '文章',
'newwindow'      => '（開隻新窗口）',
'cancel'         => '取消',
'qbfind'         => '尋',
'qbbrowse'       => '查看',
'qbedit'         => '編寫',
'qbpageoptions'  => '箇頁',
'qbpageinfo'     => '箇頁信息',
'qbmyoptions'    => '偶嗰選項',
'qbspecialpages' => '特殊頁',
'moredotdotdot'  => '別嗰...',
'mypage'         => '偶嗰頁面',
'mytalk'         => '偶嗰對話框',
'anontalk'       => '箇隻IP嗰對話框',
'navigation'     => '導航',
'and'            => '同到',

# Metadata in edit box
'metadata_help' => '元數據:',

'errorpagetitle'    => '錯誤',
'returnto'          => '回到$1。',
'tagline'           => '出自{{SITENAME}}',
'help'              => '幫助',
'search'            => '尋',
'searchbutton'      => '尋',
'go'                => '去',
'searcharticle'     => '去',
'history'           => '文章歷史',
'history_short'     => '歷史',
'updatedmarker'     => '最末道瀏覽後嗰改動',
'info_short'        => '消息',
'printableversion'  => '可打印版本',
'permalink'         => '永久連結',
'print'             => '打印',
'edit'              => '編寫',
'editthispage'      => '編寫箇頁',
'delete'            => '刪吥去',
'deletethispage'    => '刪吥箇頁',
'undelete_short'    => '反刪吥$1嗰修改',
'protect'           => '保護',
'protect_change'    => '更改保護',
'protectthispage'   => '保護箇頁',
'unprotect'         => '解除保護',
'unprotectthispage' => '解除保護箇頁',
'newpage'           => '新文章',
'talkpage'          => '談吖箇頁',
'talkpagelinktext'  => '談詑',
'specialpage'       => '特殊頁',
'personaltools'     => '個人工具',
'postcomment'       => '話滴想法',
'articlepage'       => '看吖文章',
'talk'              => '談詑',
'views'             => '眵',
'toolbox'           => '工具盒',
'userpage'          => '眵吖用戶頁',
'projectpage'       => '眵吖計劃頁',
'imagepage'         => '眵吖媒體頁',
'mediawikipage'     => '眵吖消息頁',
'templatepage'      => '眵吖模板頁',
'viewhelppage'      => '眵吖幫助頁',
'categorypage'      => '眵吖分類頁',
'viewtalkpage'      => '眵吖討論頁',
'otherlanguages'    => '別嗰話',
'redirectedfrom'    => '(從$1跳過來)',
'redirectpagesub'   => '跳轉頁',
'lastmodifiedat'    => '箇頁最晏嗰改動係：$1 $2。', # $1 date, $2 time
'viewcount'         => '箇頁拕人眵嘞$1回。',
'protectedpage'     => '拕保護頁',
'jumpto'            => '跳到:',
'jumptonavigation'  => '導航',
'jumptosearch'      => '尋',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '關於 {{SITENAME}}',
'aboutpage'            => 'Project:關於',
'bugreports'           => '報告錯誤',
'bugreportspage'       => 'Project:錯報報告',
'copyright'            => '箇版嗰內容係根據$1嗰條款發佈。',
'copyrightpagename'    => '{{SITENAME}} 版權',
'copyrightpage'        => '{{ns:project}}:版權資訊',
'currentevents'        => '新出嗰事',
'currentevents-url'    => 'Project:新出嗰事',
'disclaimers'          => '免責聲明',
'disclaimerpage'       => 'Project:免責聲明',
'edithelp'             => '編寫幫助',
'edithelppage'         => 'Help:啷編寫文章',
'faq'                  => 'FAQ',
'faqpage'              => 'Project:問得蠻多嗰問題',
'helppage'             => 'Help:説明',
'mainpage'             => '封面',
'mainpage-description' => '封面',
'policy-url'           => 'Project:政策',
'portal'               => '社區',
'portal-url'           => 'Project:社區',
'privacy'              => '隱私政策',
'privacypage'          => 'Project:隱私政策',

'badaccess'        => '許可權錯誤',
'badaccess-group0' => '倷嗰要求冇拕批准。',
'badaccess-group1' => '倷嗰要求單$1嗰用戶才扤得正。',
'badaccess-group2' => '倷嗰要求單$1嗰用戶才扤得正。',
'badaccess-groups' => '倷嗰要求單$1嗰用戶才扤得正。',

'versionrequired'     => '需要$1版嗰mediawiki',
'versionrequiredtext' => '$1版嗰mediawiki才用得正箇頁。參看[[Special:Version|版本頁]]。',

'ok'                      => '做得',
'retrievedfrom'           => '版本頁 "$1"',
'youhavenewmessages'      => '倷有 $1 ($2).',
'newmessageslink'         => '新消息',
'newmessagesdifflink'     => '最晏嗰改動',
'youhavenewmessagesmulti' => '$1 上有倷嗰新消息',
'editsection'             => '編寫',
'editold'                 => '編寫',
'viewsourceold'           => '眵吖原始碼',
'editsectionhint'         => '編寫段落: $1',
'toc'                     => '目錄',
'showtoc'                 => '敨開',
'hidetoc'                 => '收到',
'thisisdeleted'           => '眵吖或還原$1？',
'viewdeleted'             => '眵吖$1?',
'restorelink'             => '$1隻拕刪吥嗰版本',
'feedlinks'               => '鎖定:',
'feed-invalid'            => '冇用嗰鎖定類型。',
'feed-unavailable'        => '同步訂閱源到{{SITENAME}}用伓正',
'site-rss-feed'           => '$1嗰RSS訊息',
'site-atom-feed'          => '$1嗰Atom訊息',
'page-rss-feed'           => '"$1"嗰RSS訊息',
'page-atom-feed'          => '"$1" Atom Feed',
'red-link-title'          => '$1 （哈冇開始寫）',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '文章',
'nstab-user'      => '用戶頁',
'nstab-media'     => '媒體頁',
'nstab-special'   => '特殊頁',
'nstab-project'   => '計劃頁',
'nstab-image'     => '檔案',
'nstab-mediawiki' => '消息',
'nstab-template'  => '模版',
'nstab-help'      => '説明頁',
'nstab-category'  => '分類',

# Main script and global functions
'nosuchaction'      => '冇有箇隻命令',
'nosuchactiontext'  => 'Wiki識別伓到箇隻URL命令',
'nosuchspecialpage' => '冇有箇隻特殊頁',
'nospecialpagetext' => "<big>'''倷要求嗰特殊頁冇有用。'''</big>

[[Special:SpecialPages]]上尋得到用得上嗰特殊頁。",

# General errors
'error'                => '錯誤',
'databaseerror'        => '資料庫錯誤',
'dberrortext'          => '資料庫查詢語法有錯。
可能係軟件有錯。
最晏嗰資料庫指令係:
<blockquote><tt>$1</tt></blockquote>
來自函數 "<tt>$2</tt>"。
MySQL回到錯誤 "<tt>$3: $4</tt>"。',
'dberrortextcl'        => '資料庫查詢語法有錯。
最晏嗰資料庫指令係:
“$1”
來自函數“$2”。
MySQL回到錯誤“$3: $4”。',
'noconnect'            => '對伓住！wiki碰到嘞技術問題，連伓到資料庫服務器。<br />
$1',
'nodb'                 => '選伓正服務器 $1',
'cachederror'          => '底下係請求頁嗰緩存副本，可能伓係最新嗰。',
'laggedslavemode'      => '警告：頁面可能冇有新近內容。',
'readonly'             => '資料庫上正鎖囉',
'enterlockreason'      => '請輸入鎖到資料庫嗰理由，包括預計幾時間解鎖',
'readonlytext'         => '資料庫上嘞鎖改伓正，可能佢正維修中，搞正嘞仰上會還原。管理員嗰解釋： $1',
'readonly_lag'         => '附屬資料庫服務器拿緩存更新到主服務器，資料庫自動鎖到嘞',
'internalerror'        => '內部錯誤',
'internalerror_info'   => '內部錯誤: $1',
'filecopyerror'        => '複製伓正檔案 "$1" 到 "$2"。',
'filerenameerror'      => '重命名伓正檔案 "$1" 到 "$2"。',
'filedeleteerror'      => '刪伓正檔案 "$1"。',
'directorycreateerror' => '創建伓正目錄 "$1"。',
'filenotfound'         => '尋伓到檔案 "$1"。',
'fileexistserror'      => '文件 "$1" 寫伓正進去：佢已存在',
'unexpected'           => '伓正常值： "$1"="$2"。',
'formerror'            => '錯誤：交伓正表格',
'badarticleerror'      => '箇隻操作到箇頁用伓正。',
'cannotdelete'         => '揀正嗰頁面或圖像刪伓正。（佢可能拕人家刪吥嘞。）',
'badtitle'             => '錯誤嗰標題',
'badtitletext'         => '所要求嗰頁面標題伓正確，伓存在，跨語言或跨wiki連結。標題錯誤，佢可能有隻或好幾隻伓合嗰標題字符。',
'perfdisabled'         => '對伓住！箇隻操作可能會癱瘓資料庫，所以要暫停使用。',
'perfcached'           => '底下係緩存資料，可能伓係最新嗰。',
'perfcachedts'         => '底下係緩存資料，佢最晏更新嗰時間係 $1。',
'querypage-no-updates' => '箇頁目前改伓正，佢嗰資料伓能仰上更新。',
'wrong_wfQuery_params' => '參數錯誤斢到嘞 wfQuery()<br />
函數： $1<br />
查詢： $2',
'viewsource'           => '原始碼',
'viewsourcefor'        => '$1 嗰原始碼',
'protectedpagetext'    => '箇頁鎖到嘞，改伓正。',
'viewsourcetext'       => '倷可以眵吖或複製箇頁嗰原始碼：',
'protectedinterface'   => '箇頁給正嘞軟件嗰界面文本，佢拕鎖到怕人亂扤。',
'editinginterface'     => "!!糊糊涂涂!!'''警告'''：倷編寫嗰頁面係用來提供軟件嗰界面文本，改動箇頁會礙到別嗰用戶嗰界面外觀。",
'sqlhidden'            => '(SQL 弆到嗰查詢)',
'cascadeprotected'     => '箇頁已拕保護，因為佢拕「聯鎖保護」嗰{{PLURAL:$1|一隻|幾隻}}拕保護頁包到：
$2',
'namespaceprotected'   => "倷冇權編寫'''$1'''空間裡度嗰頁面。",
'customcssjsprotected' => '倷冇權編寫箇頁，佢含到別嗰用戶嗰個人設定。',
'ns-specialprotected'  => '編寫伓正{{ns:special}}空間嗰頁面。',

# Login and logout pages
'logouttitle'                => '用戶退出',
'logouttext'                 => '<strong>倷退出正嘞。</strong>

倷可以接到匿名使用{{SITENAME}}，或重登入過，隻把子頁面可能會接到話倷係登入狀態，除非係倷刪吥瀏覽器緩存。',
'welcomecreation'            => '== 歡迎, $1! ==

建正嘞倷嗰帳戶，莫忘吥設置{{SITENAME}}嗰個人參數。',
'loginpagetitle'             => '用戶登入',
'yourname'                   => '用戶名：',
'yourpassword'               => '密碼：',
'yourpasswordagain'          => '輸過道密碼：',
'remembermypassword'         => '讓電腦記到密碼',
'yourdomainname'             => '倷嗰域名：',
'externaldberror'            => '外部驗證資料庫出錯，或倷更新伓正倷嗰外部帳戶。',
'loginproblem'               => '<b>登入出錯。</b><br />重試吖看！',
'login'                      => '登入',
'nav-login-createaccount'    => '登入/新開隻帳戶',
'loginprompt'                => '要開到cookies才登入得正{{SITENAME}}。',
'userlogin'                  => '登入/新開隻帳戶',
'logout'                     => '退出',
'userlogout'                 => '退出',
'notloggedin'                => '冇登入',
'nologin'                    => '倷冇有帳戶？ $1.',
'nologinlink'                => '開隻新帳戶',
'createaccount'              => '開隻新帳戶',
'gotaccount'                 => '有嘍帳戶？ $1.',
'gotaccountlink'             => '登入',
'createaccountmail'          => '通過email',
'badretype'                  => '倷輸嗰密碼伓合。',
'userexists'                 => '倷輸嗰用戶名係人家嗰，選過隻嘍！',
'youremail'                  => '電子郵件：',
'username'                   => '用戶名：',
'uid'                        => '用戶ID：',
'yourrealname'               => '真名：',
'yourlanguage'               => '語言：',
'yourvariant'                => '轉換字體',
'yournick'                   => '簽名：',
'badsig'                     => '原始簽名錯誤，請檢查HTML。',
'badsiglength'               => '花名咁長？佢嗰長度要在$1隻字符以內。',
'email'                      => '電子郵件',
'prefs-help-realname'        => '真名係選填嗰，要係倷填嘞，倷嗰作品就會標到倷嗰名字。',
'loginerror'                 => '登入錯誤',
'prefs-help-email'           => 'email係選填嗰，佢可以讓伓認得倷嗰人通過email聯繫正倷。',
'prefs-help-email-required'  => '需要電子郵件地址。',
'nocookiesnew'               => '帳戶扤正嘞！測到倷關吥嘞Cookies，麻煩倷開到佢登入過。',
'nocookieslogin'             => '箇首要用 Cookies 登入，測到倷關吥嘞Cookies，麻煩倷開到佢登入過。',
'noname'                     => '倷冇輸正有效嗰用戶名。',
'loginsuccesstitle'          => '登入正嘞',
'loginsuccess'               => '倷搦到"$1"嗰身份登到{{SITENAME}}。',
'nosuchuser'                 => '箇首冇叫"$1"嗰用戶。望吖倷嗰拼寫，要伓建過隻新帳戶。',
'nosuchusershort'            => '箇首冇叫"<nowiki>$1</nowiki>"嗰用戶。請望吖倷嗰拼寫。',
'nouserspecified'            => '倷要指正一隻用戶名。',
'wrongpassword'              => '倷輸嗰密碼錯誤伓對，請試過吖囉。',
'wrongpasswordempty'         => '倷冇輸入密碼，請試過吖囉。',
'passwordtooshort'           => '倷嗰密碼伓對或太短嘞，佢最少要有$1隻字符，哈要同用戶名伓一樣。',
'mailmypassword'             => '拿新密碼寄到偶',
'passwordremindertitle'      => '{{SITENAME}}嗰密碼提醒',
'passwordremindertext'       => '有人(可能係倷，IP位址$1)要偶俚拿新嗰{{SITENAME}} ($4) 嗰登入密碼寄到倷。眼下用戶"$2"嗰密碼係"$3"。請仰上就登入同到換吥密碼。要係別嗰人發嗰請求，或者倷尋回嘞倷嗰密碼，伓想改佢，倷可以嫑搭箇隻消息，繼續用舊密碼。',
'noemail'                    => '冇有用戶"$1"嗰email地址。',
'passwordsent'               => '新嗰密碼已經寄到用戶"$1"嗰email去嘍。收到後請再登入過。',
'blocked-mailpassword'       => '倷嗰IP地址拕封到嘞。用伓正密碼復原功能以防亂用。',
'eauthentsent'               => '確認email寄到話正嗰地址去嘍。別嗰email發到箇隻帳戶之前，倷起先要按箇封email話嗰佢係否倷嗰。',
'throttled-mailpassword'     => '$1嗰鐘頭前發出嘞密碼提醒。怕別嗰人亂扤，$1嗰鐘頭之內就只會發一隻密碼提醒。',
'mailerror'                  => '發送email錯誤: $1',
'acct_creation_throttle_hit' => '對伓住，倷建嘞$1隻帳號。倷再建伓正囉。',
'emailauthenticated'         => '倷嗰email已經拕$1確認有效嘍。',
'emailnotauthenticated'      => '倷嗰email<strong>哈冇拕認證</strong>。底下嗰功能都伓會發任何郵件。',
'noemailprefs'               => '<strong>話正隻email來用箇隻功能</strong>',
'emailconfirmlink'           => '確認倷嗰email',
'invalidemailaddress'        => 'email嗰格式伓合，請輸隻正確嗰email位址或清吥箇隻空格。',
'accountcreated'             => '帳戶扤正嘍',
'accountcreatedtext'         => '扤正嘍$1嗰帳戶。',
'createaccount-title'        => '到{{SITENAME}}創建嗰帳戶',
'loginlanguagelabel'         => '語言: $1',

# Password reset dialog
'resetpass'               => '設過帳戶密碼',
'resetpass_announce'      => '倷係用到臨時email嗰代碼登入嗰。要登正入，倷要到箇首設定隻新密碼:',
'resetpass_header'        => '設過密碼',
'resetpass_submit'        => '設定密碼同到登入',
'resetpass_success'       => '倷嗰密碼改正嘍！正幫倷登入...',
'resetpass_bad_temporary' => '冇用嗰臨時密碼。可能倷改正嘞倷嗰密碼，或者重新要過隻新嗰臨時密碼。',
'resetpass_forbidden'     => '到{{SITENAME}}上改伓正密碼',
'resetpass_missing'       => '尋伓到資料。',

# Edit page toolbar
'bold_sample'     => '粗體字',
'bold_tip'        => '粗體字',
'italic_sample'   => '斜體字',
'italic_tip'      => '斜體字',
'link_sample'     => '連結標題',
'link_tip'        => '內部連結',
'extlink_sample'  => 'http://www.example.com 連結標題',
'extlink_tip'     => '外部連結(頭上加 http://)',
'headline_sample' => '標題文字',
'headline_tip'    => '二級標題',
'math_sample'     => '到箇首扻入數學公式',
'math_tip'        => '數學公式 (LaTeX)',
'nowiki_sample'   => '到箇首扻入非格式文本',
'nowiki_tip'      => '扻入非格式文本',
'image_tip'       => '扻進文件',
'media_tip'       => '檔案連結',
'sig_tip'         => '倷帶時間嗰簽名',
'hr_tip'          => '橫線 (小聲使用)',

# Edit pages
'summary'                   => '摘要',
'subject'                   => '主題/頭條',
'minoredit'                 => '箇係隻細修改',
'watchthis'                 => '眏到箇頁',
'savearticle'               => '存到箇頁',
'preview'                   => '預覽',
'showpreview'               => '望吖起',
'showlivepreview'           => '即時預覽',
'showdiff'                  => '顯示差異',
'anoneditwarning'           => "'''警告:'''倷哈冇登入，箇頁嗰編寫歷史會記到倷嗰IP。",
'missingsummary'            => "'''提示:''' 倷冇提供編寫摘要。要係倷再按係保存嗰話，倷保存嗰編輯就會冇編輯摘要。",
'missingcommenttext'        => '請到底下評論。',
'missingcommentheader'      => "'''提示:''' 倷嗰評論冇提供標題。要係倷再按係保存嗰話，倷保存嗰編輯就會冇標題。",
'summary-preview'           => '摘要預覽',
'subject-preview'           => '主題/頭條預覽',
'blockedtitle'              => '用戶封到嘞',
'blockedtext'               => "<big>倷嗰用戶名或IP地址拕$1封到嘞。</big>

箇道封鎖係$1封嗰。個中原因係''$2''。

* 箇回封鎖嗰開始時間係：$8
* 箇回封鎖嗰到期時間係：$6
* 對於拕查封嗰人：$7

倷聯繫得正$1或別嗰[[{{MediaWiki:Grouppage-sysop}}|管理員]]，討論箇回封鎖。除非倷到倷嗰[[Special:Preferences|帳號參數設置]]裡度設正嘞有效嗰email，伓然嗰話倷係用伓正「email到箇隻用戶」嗰功能。設正嘞有效嗰email後，箇隻功能係伓會拕封到嗰。倷嗰IP地址係$3，許拕封到嗰ID係 #$5。請倷到全部嗰查詢裡度注明箇隻地址同／或查封ID。",
'autoblockedtext'           => '別嗰人用過倷嗰IP地址，故係佢拕自動鎖到嘞。封佢嗰人係$1.
下首係封鎖嗰理由:

:\'\'$2\'\'

* 封鎖開始: $8
* 封鎖過期: $6

倷聯繫得正$1或別嗰[[{{MediaWiki:Grouppage-sysop}}|管理員]]去談下箇道封鎖。

注意嗰係話伓定倷冇"e-mail箇隻用戶"嗰功能，除非倷到[[Special:Preferences|用戶設置]]有隻註冊email地址，再就係倷冇因為用佢拕封過。

倷嗰封鎖ID係$5。請到查詢嗰時間都要緊標到佢。',
'blockednoreason'           => '冇話理由',
'blockedoriginalsource'     => "底下係'''$1'''嗰原始碼:",
'blockededitsource'         => "底下係倷對'''$1'''嗰'''編輯'''內容:",
'whitelistedittitle'        => '登入後才編得正',
'whitelistedittext'         => '起先倷要$1才編得正箇頁。',
'confirmedittitle'          => 'email確認後才編得正',
'confirmedittext'           => '確認嘞email才能編寫箇頁。麻煩用[[Special:Preferences|參數設置]]設置同確認倷嗰email。',
'nosuchsectiontitle'        => '冇箇隻段落',
'nosuchsectiontext'         => '倷嘗試編寫嗰段落伓存在。到箇首係冇第$1隻段落，所以係冇場窪去存到倷嗰編輯。',
'loginreqtitle'             => '需要登入',
'loginreqlink'              => '登入',
'loginreqpagetext'          => '倷要$1才能眵別嗰頁面。',
'accmailtitle'              => '密碼寄出嘞',
'accmailtext'               => "'$1'嗰密碼發到$2嘞。",
'newarticle'                => '(新)',
'newarticletext'            => '倷連到嗰頁面伓存在。要新開箇隻頁面，倷能到下底嗰方框編寫內容（細節請望[[Help:説明|説明]]）。要係倷伓係特試來到箇首，按吖瀏覽器嗰“返回”即可。',
'anontalkpagetext'          => "---- ''箇係匿名用戶嗰討論頁，話伓定佢哈冇開隻帳戶。別人單用得正IP地址同佢聯繫。箇隻IP地址可能有好幾隻用戶共用。如果倷係匿名用戶，覺得箇頁嗰內容同倷冇關，歡迎去[[Special:UserLogin|開隻新帳戶或登入]]，省得同別嗰匿名用戶扤混來。''",
'noarticletext'             => '眼下箇頁哈冇內容，倷可以到別嗰頁面[[Special:Search/{{PAGENAME}}|尋吖箇頁嗰標題]]或[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} 編寫箇頁]。',
'clearyourcache'            => "'''注意:''' 保存之後, 倷要清吥瀏覽器嗰緩存才眵得正改嗰內容。 '''Mozilla / Firefox / Safari:''' 按到 ''Shift'' 接到按''刷新''(或按吖''Ctrl-Shift-R''，到蘋果Mac上按''Cmd-Shift-R'')；'''IE:''' 按到 ''Ctrl''接到按''刷新''，或按吖''Ctrl-F5''；'''Konqueror:''' 單只要按 ''刷新''；'''Opera:''' 用戶要到 ''工具-設置'' 完全嗰清除緩存。",
'usercssjsyoucanpreview'    => "<strong>提示:</strong> 存到前請用'望吖起'來測吖倷嗰新CSS/JS 。",
'usercsspreview'            => "'''注意倷單係到預覽倷個人嗰 CSS，內容哈冇保存！'''",
'userjspreview'             => "'''注意倷單係到測試／預覽倷個人嗰 JavaScript，內容哈冇保存！'''",
'userinvalidcssjstitle'     => "'''警告:''' 冇\"\$1\"嗰皮膚。請記到自定義嗰 .css 同 .js 頁要用小寫。就話，{{ns:user}}:Foo/monobook.css 伓等同 {{ns:user}}:Foo/Monobook.css。",
'updated'                   => '(更新正嘍)',
'note'                      => '<strong>注意:</strong>',
'previewnote'               => '<strong>請記到箇光係預覽，內容哈冇保存！</strong>',
'previewconflict'           => '箇隻預覽係上首文字編輯區嗰內容。倷選擇保存嗰話佢才會保存到。',
'session_fail_preview'      => '<strong>對伓住！箇隻段落嗰資料跌吥嘞，偶個俚處理伓正倷嗰編輯。請試過吖。哈係扤伓正嗰話，試吖退出後登入過。</strong>',
'session_fail_preview_html' => "<strong>對伓住！相關嗰程式資料跌吥嘞，偶個俚處理伓正倷嗰編輯。</strong> 

''箇隻wiki開放正嘞原HTML碼，預覽弆到嘞以防止JavaScript嗰攻擊。'' 

<strong>要係佢係合法編輯嗰，請試過吖。哈係扤伓正嗰話，試吖退出後登入過。</strong>",
'token_suffix_mismatch'     => '<strong>倷嗰用戶端嗰編輯信毀吥嘞嚸標點符號字符，噉嗰話倷嗰編輯就拕拒絕嘞。
箇種情況通常係含到好多臭蟲、以網絡為主嗰匿名代理服務扤得。</strong>',
'editing'                   => '眼下在編輯$1',
'editingsection'            => '眼下在編輯$1 (段落)',
'editingcomment'            => '眼下在編輯$1 (評論)',
'editconflict'              => '編輯仗: $1',
'explainconflict'           => "倷開始編輯之後有人改動嘞箇頁。
上首嗰方框顯示嗰係眼下本頁嗰內容。
倷嗰修改到下底嗰方框顯示。
倷需要拿倷嗰修改並到眼下嗰內容。
'''單只係'''上首方框嗰內容會等倷按\"存到箇頁\"之後拕保存。",
'yourtext'                  => '倷編嗰內容',
'storedversion'             => '存到嗰版本',
'nonunicodebrowser'         => '<strong>警告：倷嗰瀏覽器伓兼容Unicode。箇度有隻辦法方便倷安全嗰編寫得正文章：伓係ASCII嗰字符會到編輯框裡度用十六進位編碼顯到。</strong>',
'editingold'                => '<strong>警告：倷於今正編寫箇頁嗰舊版本。
要係倷存到佢嗰話，箇隻版本嗰全部改動會都跌吥去。</strong>',
'yourdiff'                  => '差異',
'copyrightwarning'          => '請記得到{{SITENAME}}嗰全部貢獻會拕認為係$2之下發出嗰（望吖$1有別嗰資料）。要係倷伓想自家嗰編輯好嚟嚟拕亂扤吥，唉就莫遞交。<br />
倷都要話正倷嗰文字係倷自家寫嗰，或者係公有領域或別嗰自由資源複製到嗰。<br />
<strong>冇任何許可嗰情況下請莫遞交有版權嗰作品！</strong>',
'copyrightwarning2'         => '請記得別嗰人編得正、改得正或者刪得正倷到{{SITENAME}}嗰全部貢獻。要係倷伓想自家嗰編輯好嚟嚟拕改吥，唉就莫遞交。<br />
倷都要話正倷嗰文字係倷自家寫嗰，或者係公有領域或別嗰自由資源複製到嗰（望吖$1有別嗰資料）。
<strong>冇任何許可嗰情況下請莫遞交有版權嗰作品！</strong>',
'longpagewarning'           => '<strong>警告：箇頁有$1 kilobytes咁長；有嗰瀏覽器會編伓正超吥32kb嗰頁面。
請考慮吖拿佢分到細嚸嗰小段落。</strong>',
'longpageerror'             => '<strong>錯誤：倷遞交嗰文字有$1 kilobytes咁長，佢長過最大嗰$2 kilobytes。存伓正倷遞交嗰文字。</strong>',
'readonlywarning'           => '<strong>警告: 資料庫鎖到嘞進行定期修護，眼下倷存伓正倷嗰改動。倷可以拿佢存到文檔再著。</strong>',
'protectedpagewarning'      => '<strong>警告: 箇頁已經受保護，單只管理員許可權嗰用戶才改得正。</strong>',
'semiprotectedpagewarning'  => "'''注意：'''箇頁拕鎖到嘞，單只註冊用戶編得正。",
'cascadeprotectedwarning'   => '警告: 箇頁已經受保護，單只管理員許可權嗰用戶才改得正，因為箇頁同底下嗰連鎖保護嗰{{PLURAL:$1|一隻|多隻}}頁面包到嘞:',
'titleprotectedwarning'     => '<strong>警告：箇隻頁鎖到嘍，只有一滴子人才建得正。</strong>',
'templatesused'             => '箇隻頁面使用嗰模板有:',
'templatesusedpreview'      => '箇隻預覽使用嗰模板有:',
'templatesusedsection'      => '箇隻段落使用嗰模板有:',
'template-protected'        => '(保護)',
'template-semiprotected'    => '(半保護)',
'edittools'                 => '<!--箇首嗰文本會到下底嗰編輯同上傳列表裡坨顯示。 -->',
'nocreatetitle'             => '新建頁面拕限制',
'nocreatetext'              => '箇隻網站限制新建頁面嗰功能。倷可以回頭去編輯有嘞嗰頁面，或者[[Special:UserLogin|登入或新開帳戶]]。',
'nocreate-loggedin'         => '倷到 {{SITENAME}} 冇權新開頁面。',
'permissionserrors'         => '許可權錯誤',
'permissionserrorstext'     => '根據底下嗰{{PLURAL:$1|原因|原因}}，倷冇許可權去扤:',
'recreate-deleted-warn'     => "'''警告: 倷正重建一隻之前拕刪吥嗰頁面。'''

倷應該要考慮吖繼續編輯箇頁面係否有必要。
為到方便，箇頁嗰刪除記錄已經到下底提供:",

# "Undo" feature
'undo-success' => '箇隻編輯可以拕取銷。請檢查吖以確定箇係倷想扤嗰，接到保存修改去完成撤銷編輯。',
'undo-failure' => '半中嗰編輯有人挭仗，箇隻編輯伓可以拕取銷。',
'undo-summary' => '取消由[[Special:Contributions/$2|$2]] ([[User talk:$2|對話]])所修訂嗰 $1',

# Account creation failure
'cantcreateaccounttitle' => '新開伓正帳戶',
'cantcreateaccount-text' => "IP 位址伓能 ('''$1''') 新開帳戶。箇可能係因為經常有來自倷嗰學堂或網絡供應商 (ISP)故意嗰破壞扤得。",

# History pages
'viewpagelogs'        => '眵吖箇頁嗰日誌',
'nohistory'           => '箇頁冇修改歷史。',
'revnotfound'         => '冇尋到修改歷史',
'revnotfoundtext'     => '倷請求嗰更早版本嗰修改歷史冇尋到。請檢查倷嗰URL係否正確。',
'currentrev'          => '眼前嗰修改版本',
'revisionasof'        => '$1嗰修改版本',
'revision-info'       => '$2到$1扤嗰修訂版本',
'previousrevision'    => '←之前嗰修改',
'nextrevision'        => '接到嗰修改→',
'currentrevisionlink' => '眼前嗰修改',
'cur'                 => '眼前',
'next'                => '之後',
'last'                => '之前',
'page_first'          => '最早',
'page_last'           => '最晏',
'histlegend'          => '差異選擇: 標到伓同版本嗰單選鍵，接到按吖督上嗰鍵比較下。<br />
說明: (眼下) 指同目前版本嗰比較，(之前) 指同之前修改版本嗰比較，細 = 細修改。',
'deletedrev'          => '[拕刪除]',
'histfirst'           => '最早版本',
'histlast'            => '最晏版本',
'historysize'         => '({{PLURAL:$1|1 字節|$1 字節}})',
'historyempty'        => '(空)',

# Revision feed
'history-feed-title'          => '修改歷史',
'history-feed-description'    => '本站箇頁嗰修改歷史',
'history-feed-item-nocomment' => '$1到$2', # user at time
'history-feed-empty'          => '要求嗰頁面伓存在。佢可能拕刪吥嘞或改嘞名。試吖[[Special:Search|到本站尋]]有關嗰新頁面內容。',

# Revision deletion
'rev-deleted-comment'         => '(注釋挪吥嘞)',
'rev-deleted-user'            => '(用戶名挪吥嘞)',
'rev-deleted-event'           => '(項目挪吥嘞)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">箇頁嗰改動從共用文檔挪吥嘞。到[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌] 裡度倷話伓定有詳細嗰資料。</div>',
'rev-deleted-text-view'       => "<div class='mw-warning plainlinks'>箇頁嗰改動從共用文檔挪吥嘞。作為本站嗰管理員，倷查看得正；到[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 刪除日誌] 裡度有詳細嗰資料。 </div>",
'rev-delundel'                => '顯示/弆到',
'revisiondelete'              => '刪除/反刪除修改',
'revdelete-nooldid-title'     => '冇目標修訂',
'revdelete-nooldid-text'      => '倷冇话箇隻操作嗰目标修改。',
'revdelete-selected'          => "揀'''$1'''嗰$2回修訂:",
'logdelete-selected'          => "揀'''$1'''嗰$2隻日誌事件:",
'revdelete-text'              => '刪吥嗰改動哈會到頁面歷史裡頭顯示, 但公眾瀏覽伓正佢嗰內容。 

箇站別嗰管理員哈係能眵吖弆到嗰內容，同到通過同佢一樣嗰界面恢復刪除，除非設正嘞附加嗰限制。',
'revdelete-legend'            => '設置修改限制',
'revdelete-hide-text'         => '弆到修改內容',
'revdelete-hide-name'         => '弆到動作同目標',
'revdelete-hide-comment'      => '弆到編輯說明',
'revdelete-hide-user'         => '弆到編者嗰用戶名/IP',
'revdelete-hide-restricted'   => '同樣嗰限制應用到管理員',
'revdelete-suppress'          => '同時壓到由操作員同別嗰用戶嗰資料',
'revdelete-hide-image'        => '弆到檔內容',
'revdelete-unsuppress'        => '移吥恢復正嗰改動嗰限制',
'revdelete-log'               => '日誌說明:',
'revdelete-submit'            => '應用到選正嗰修改',
'revdelete-logentry'          => '已更改[[$1]]嗰修改可見性',
'logdelete-logentry'          => '已更改[[$1]]嗰事件可見性',
'revdelete-success'           => "'''修訂嗰可見性設置正嘍。'''",
'logdelete-success'           => "'''事件嗰可見性設置正嘍。'''",

# History merging
'mergehistory'                     => '合併頁面嗰歷史',
'mergehistory-box'                 => '合併兩隻頁面嗰版本：',
'mergehistory-from'                => '來嗰頁面：',
'mergehistory-into'                => '要去嗰頁面：',
'mergehistory-list'                => '合併得正嗰修改歷史',
'mergehistory-go'                  => '顯示合併得正嗰修改',
'mergehistory-submit'              => '合併版本',
'mergehistory-empty'               => '冇版本合併得正.',
'mergehistory-no-source'           => '冇箇隻 $1 來嗰頁面。',
'mergehistory-no-destination'      => '冇箇隻 $1 要去嗰頁面。',
'mergehistory-invalid-source'      => '來嗰頁面題目要寫正。',
'mergehistory-invalid-destination' => '要去嗰頁面題目要寫正。',

# Merge log
'mergelog'    => '合併記錄',
'revertmerge' => '伓合併',

# Diffs
'history-title'           => '歷史版本嗰 "$1"',
'difference'              => '(修改之間差異)',
'lineno'                  => '第$1行:',
'compareselectedversions' => '比較揀正嗰版本',
'editundo'                => '還原',
'diff-multi'              => '($1回半中間嗰改動伓會顯示。)',

# Search results
'searchresults'         => '尋到嗰結果',
'searchresulttext'      => '有關嗰{{SITENAME}}嗰更多資料,請參看[[{{MediaWiki:Helppage}}|{{int:help}}]]。',
'searchsubtitle'        => "用'''[[:$1]]'''",
'searchsubtitleinvalid' => "用'''$1'''尋",
'noexactmatch'          => "'''冇尋到標題係\"\$1\"嗰頁面。''' 倷可以[[:\$1|新開箇隻頁面]]。",
'noexactmatch-nocreate' => "'''冇題目係 \"\$1\" 嗰頁面'''",
'toomanymatches'        => '返回多傷嘍嗰結果，請試吖用別嗰詞語尋過',
'titlematches'          => '文章標題符合',
'notitlematches'        => '冇頁面同文章標題符合',
'textmatches'           => '頁面內容符合',
'notextmatches'         => '冇頁面內容符合',
'prevn'                 => '前$1隻',
'nextn'                 => '後$1隻',
'viewprevnext'          => '眵吖($1) ($2) ($3)',
'showingresults'        => '底下從第<b>$2</b>條顯示起先嗰<b>$1</b>條結果:',
'showingresultsnum'     => '底下從第<b>$2</b>條顯示起先嗰<b>$3</b>條結果:',
'nonefound'             => '<strong>注意：</strong>尋伓到往往係因為搜索夾到像“嗰”或“同”之類嗰常用字扤得。',
'powersearch'           => '高級尋',
'searchdisabled'        => '{{SITENAME}}嗰搜索功能已經關閉。倷可以用Google尋吖。但係佢嗰索引可能係早先嗰。',

# Preferences page
'preferences'              => '參數設置',
'mypreferences'            => '偶嗰參數設置',
'prefs-edits'              => '編輯數:',
'prefsnologin'             => '哈冇登入',
'prefsnologintext'         => '倷要[[Special:UserLogin|登入]]後才設得正個人參數。',
'prefsreset'               => '參數已經拕重新設過。',
'qbsettings'               => '快捷導航條',
'qbsettings-none'          => '冇',
'qbsettings-fixedleft'     => '左首固定',
'qbsettings-fixedright'    => '右首固定',
'qbsettings-floatingleft'  => '左首漂移',
'qbsettings-floatingright' => '左首漂移',
'changepassword'           => '改過密碼',
'skin'                     => '皮',
'math'                     => '數學公式',
'dateformat'               => '日期格式',
'datedefault'              => '默認項目',
'datetime'                 => '日期同到時間',
'math_failure'             => '分析失敗',
'math_unknown_error'       => '未知錯誤',
'math_unknown_function'    => '未知函數',
'math_lexing_error'        => '句法錯誤',
'math_syntax_error'        => '文法錯誤',
'math_image_error'         => 'PNG轉換失敗；請檢查係否裝正嘞latex, dvips, gs同到convert',
'math_bad_tmpdir'          => '寫伓正或建伓正數學公式臨時目錄',
'math_bad_output'          => '寫伓正或建伓正數學公式輸出目錄',
'math_notexvc'             => '執行伓正"texvc"；請參看 math/README 再配置過。',
'prefs-personal'           => '用戶介紹',
'prefs-rc'                 => '最近更改',
'prefs-watchlist'          => '監視列表',
'prefs-watchlist-days'     => '監視列表顯示最久嗰日數:',
'prefs-watchlist-edits'    => '加強版嗰監視列表顯示最多更改數目:',
'prefs-misc'               => '雜項',
'saveprefs'                => '存到參數',
'resetprefs'               => '設過參數',
'oldpassword'              => '老密碼：',
'newpassword'              => '新密碼：',
'retypenew'                => '確認密碼:',
'textboxsize'              => '編寫',
'rows'                     => '橫:',
'columns'                  => '豎:',
'searchresultshead'        => '設置尋到嗰結果',
'resultsperpage'           => '設置尋到嗰連結數',
'contextlines'             => '設置尋到嗰行數:',
'contextchars'             => '設置尋到嗰字數:',
'stub-threshold'           => '<a href="#" class="stub">細文連結</a>格式門檻:',
'recentchangesdays'        => '最近更改中嗰顯示日數:',
'recentchangescount'       => '最近更改中嗰編輯數:',
'savedprefs'               => '倷嗰個人參數設置保存正嘞。',
'timezonelegend'           => '時區',
'timezonetext'             => '¹倷嗰當地時區同服務器時間(UTC)嗰時差。',
'localtime'                => '當地時區',
'timezoneoffset'           => '時差¹',
'servertime'               => '服務器時間',
'guesstimezone'            => '到瀏覽器上填',
'allowemail'               => '接受別嗰用戶嗰郵件',
'defaultns'                => '默認搜索嗰名字空間:',
'default'                  => '默認',
'files'                    => '檔案',

# User rights
'userrights'               => '用戶許可權管理', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'   => '管理用戶群',
'userrights-user-editname' => '輸入用戶名:',
'editusergroup'            => '編輯用戶群',
'editinguser'              => "眼下在編輯用戶嗰權限 '''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]])",
'userrights-editusergroup' => '編輯用戶群',
'saveusergroups'           => '存儲用戶群',
'userrights-groupsmember'  => '歸到:',
'userrights-reason'        => '改嗰原因:',
'userrights-no-interwiki'  => '倷冇得權改吥別嗰wiki網站上箇隻用戶嗰權利。',
'userrights-nodatabase'    => '冇得箇隻數據庫 $1 或係冇在本地。',

# Groups
'group'               => '群:',
'group-autoconfirmed' => '自動確認用戶',
'group-bot'           => '機器人',
'group-sysop'         => '操作員',
'group-bureaucrat'    => '行政員',
'group-all'           => '(全部)',

'group-autoconfirmed-member' => '自動確認用戶',
'group-bot-member'           => '機器人',
'group-sysop-member'         => '操作員',
'group-bureaucrat-member'    => '行政員',

'grouppage-autoconfirmed' => '{{ns:project}}:自動確認用戶',
'grouppage-bot'           => '{{ns:project}}:機器人',
'grouppage-sysop'         => '{{ns:project}}:操作員',
'grouppage-bureaucrat'    => '{{ns:project}}:行政員',

# User rights log
'rightslog'      => '用戶許可權日誌',
'rightslogtext'  => '底下記到用戶許可權嗰更改記錄。',
'rightslogentry' => '拿 $1 嗰許可權從 $2 改到 $3',
'rightsnone'     => '(冇)',

# Recent changes
'nchanges'                          => '$1道改動',
'recentchanges'                     => '最晏嗰改動',
'recentchangestext'                 => '跟到箇隻wiki上嗰最新改動。',
'recentchanges-feed-description'    => '跟到箇隻 wiki 上集合嗰最後改動。',
'rcnote'                            => "下底係到$3，最近'''$2'''日之內嗰最後'''$1'''回改動:",
'rcnotefrom'                        => "底下係自'''$2'''嗰更改(頂多顯示'''$1'''):",
'rclistfrom'                        => '顯示自$1後嗰新改動',
'rcshowhideminor'                   => '$1細編輯',
'rcshowhidebots'                    => '$1機器人嗰編輯',
'rcshowhideliu'                     => '$1登入用戶嗰編輯',
'rcshowhideanons'                   => '$1匿名用戶嗰編輯',
'rcshowhidepatr'                    => '$1檢查過嗰編輯',
'rcshowhidemine'                    => '$1偶嗰編輯',
'rclinks'                           => '顯示最晏$2日之內最新嗰$1回改動。<br />$3',
'diff'                              => '差異',
'hist'                              => '歷史',
'hide'                              => '弆到',
'show'                              => '顯示',
'minoreditletter'                   => '細',
'newpageletter'                     => '新',
'boteditletter'                     => '機',
'number_of_watching_users_pageview' => '[$1隻監視用戶]',
'rc_categories'                     => '分類界定(用"|"隔開)',
'rc_categories_any'                 => '任何',
'newsectionsummary'                 => '/* $1 */ 新段落',

# Recent changes linked
'recentchangeslinked'          => '連結頁嗰更改',
'recentchangeslinked-title'    => '連結頁嗰改動到 "$1"',
'recentchangeslinked-noresult' => '箇段時間嗰連結頁冇更改。',
'recentchangeslinked-summary'  => "箇隻特殊頁列出箇頁連出去頁面嗰最晏改動。倷監視列表嗰頁面會用'''粗體'''顯到。",

# Upload
'upload'                      => '上傳檔案',
'uploadbtn'                   => '上傳檔案',
'reupload'                    => '上傳過',
'reuploaddesc'                => '返回上傳列表。',
'uploadnologin'               => '冇登入',
'uploadnologintext'           => '倷要[[Special:UserLogin|登入]]再上傳得正檔案。',
'upload_directory_read_only'  => '上傳目錄($1)伓存在或冇寫入許可權。',
'uploaderror'                 => '上傳出錯',
'uploadtext'                  => "用下底嗰表格上傳檔案。
要眵或要尋先前上傳嗰圖像請去[[Special:ImageList|圖像列表]]，上傳同刪除會記到[[Special:Log/upload|上傳日誌]]裡度。

要係想扻文件到頁面，用得正下底嗰方式連結:
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.jpg]]</nowiki>''',
'''<nowiki>[[</nowiki>{{ns:image}}<nowiki>:File.png|alt text]]</nowiki>''' 或
'''<nowiki>[[</nowiki>{{ns:media}}<nowiki>:File.ogg]]</nowiki>''' 直接連接到箇隻文件。",
'upload-permitted'            => '容許嗰文件類型：$1。',
'upload-preferred'            => '優先嗰文件類型：$1。',
'upload-prohibited'           => '禁止嗰文件類型：$1。',
'uploadlog'                   => '上傳日誌',
'uploadlogpage'               => '上傳日誌',
'uploadlogpagetext'           => '底下係最近上傳檔嗰通覽表。',
'filename'                    => '檔案名',
'filedesc'                    => '摘要',
'fileuploadsummary'           => '摘要:',
'filestatus'                  => '版權狀態:',
'filesource'                  => '來源:',
'uploadedfiles'               => '上傳檔案中',
'ignorewarning'               => '伓搭警告同存到檔案',
'ignorewarnings'              => '伓搭所有警告',
'minlength1'                  => '檔案名字至少要有一隻字。',
'illegalfilename'             => '檔案名"$1"有頁面標題伓容許嗰字元。請改吖名再上傳過。',
'badfilename'                 => '檔案名已經拕改成"$1"。',
'filetype-badmime'            => 'MIME類別"$1"係伓容許嗰格式。',
'filetype-missing'            => '箇隻檔案名稱並冇副檔名 (就像 ".jpg")。',
'large-file'                  => '建議檔案嗰大小伓要超吥$1；本檔案大小係$2。',
'largefileserver'             => '箇隻檔案要大過服務器配置容允嗰大小。',
'emptyfile'                   => '倷上傳嗰檔案伓存在。箇可能係因為檔案名按錯嘞。請檢查倷係否真嗰要上傳箇隻檔案。',
'fileexists'                  => '箇隻檔案名已存在。如果倷確定伓正倷係否要改佢，請檢查<strong><tt>$1</tt></strong>。',
'fileexists-extension'        => '有嘞隻飛像嗰檔名:<br /> 上載文檔嗰檔名: <strong><tt>$1</tt></strong><br /> 目前檔嗰檔名: <strong><tt>$2</tt></strong><br /> 請揀隻伓同嗰名字。',
'fileexists-thumb'            => "<center>'''早就有嘍嗰文件'''</center>",
'fileexists-thumbnail-yes'    => '箇隻檔案好像係一隻圖像嗰縮小版<i>(縮圖)</i>。請檢查清楚箇隻檔案<strong><tt>$1</tt></strong>。<br /> 如果檢查後嗰檔同原先圖像嗰大小係一樣嗰話，就嫑再上傳多一隻縮圖。',
'file-thumbnail-no'           => '箇隻檔案名係以<strong><tt>$1</tt></strong>開頭。佢好像一隻圖像嗰縮小版<i>(縮圖)</i>。如果倷有箇隻圖像嗰完整版，伓然請再改過隻檔名。',
'fileexists-forbidden'        => '箇隻檔案名已存在；請回頭並換過隻新嗰名稱來上傳箇隻檔案。[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => '到共用檔案庫裡度有嘞同名嗰檔案；請回頭並換過隻新嗰名稱來上傳箇隻檔案。[[Image:$1|thumb|center|$1]]',
'successfulupload'            => '上傳正嘞',
'uploadwarning'               => '上傳警告',
'savefile'                    => '保存檔案',
'uploadedimage'               => '上傳正嘞"[[$1]]"',
'overwroteimage'              => '上傳正嘞"[[$1]]"嗰新版本',
'uploaddisabled'              => '上傳伓正',
'uploaddisabledtext'          => '上傳伓正文件到{{SITENAME}}。',
'uploadscripted'              => '箇隻檔案包到可能會誤導網絡瀏覽器錯誤解釋嗰 HTML 或腳本代碼。',
'uploadcorrupt'               => '箇隻檔案包含或者係一隻伓正確嗰副檔名。請檢查吖接到重新上傳。',
'uploadvirus'                 => '箇隻檔案有病毒！詳情: $1',
'sourcefilename'              => '原始檔案名:',
'destfilename'                => '目標檔案名:',
'watchthisupload'             => '眏到箇頁',
'filewasdeleted'              => '先前有隻同名檔案上傳後又拕刪吥嘞。上傳箇隻檔案之前倷非要檢查$1。',
'upload-wasdeleted'           => "'''警告: 倷於今上傳嗰檔案係先前刪過嗰。'''

倷要想正係真嗰上傳箇隻檔案。
為到方便起見，箇隻檔案嗰刪除記錄到下底提供嘞:",
'filename-bad-prefix'         => '倷上傳嗰檔案名係以<strong>"$1"</strong>做開頭嗰，通常箇種冇意義嗰名字係數碼相機度嗰自動編排。請到倷嗰檔案揀過隻更加有意義嗰名字。',

'upload-proto-error'      => '協定錯誤',
'upload-proto-error-text' => '遠程上傳要求 URL 用 <code>http://</code> 或 <code>ftp://</code> 開頭。',
'upload-file-error'       => '內部錯誤',
'upload-file-error-text'  => '創建臨時檔案時服務器出現內部錯誤。請聯繫系統管理員。',
'upload-misc-error'       => '未知嗰上傳錯誤',
'upload-misc-error-text'  => '上傳嗰時間發生未知嗰錯誤。請確認輸嗰係正確同訪問得正嗰 URL，接到試過吖。要係哈有問題，請聯繫系統管理員。',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => '訪問伓正 URL',
'upload-curl-error6-text'  => '訪問伓正輸入嗰 URL。請檢查過箇隻URL 係否正確，再就係網站嗰訪問係否正常。',
'upload-curl-error28'      => '上傳超時',
'upload-curl-error28-text' => '站點回應時間過長。請檢查箇隻網站嗰訪問係否正常，過吖再試過。倷可能要等網絡伓咁卡嗰時間再試吖。',

'license'            => '授權:',
'nolicense'          => '冇選定',
'license-nopreview'  => '(冇預覽用得正)',
'upload_source_url'  => '(一隻有效嗰，公開嗰 URL)',
'upload_source_file' => '(倷電腦嗰一隻檔案)',

# Special:ImageList
'imagelist_search_for'  => '按媒體名字尋:',
'imgfile'               => '檔案',
'imagelist'             => '檔案列表',
'imagelist_date'        => '日期',
'imagelist_name'        => '名稱',
'imagelist_user'        => '用戶',
'imagelist_size'        => '大細',
'imagelist_description' => '簡話',

# Image description page
'filehist'                  => '檔案歷史',
'filehist-help'             => '按到日期／時間去眵吖許時間有過嗰檔案。',
'filehist-deleteall'        => '全部刪掉',
'filehist-deleteone'        => '刪吥箇隻',
'filehist-revert'           => '恢復',
'filehist-current'          => '眼前',
'filehist-datetime'         => '日期／時間',
'filehist-user'             => '用戶',
'filehist-dimensions'       => '尺寸',
'filehist-filesize'         => '檔案大細',
'filehist-comment'          => '說明',
'imagelinks'                => '連結',
'linkstoimage'              => '底下嗰頁面連結到箇隻檔案:',
'nolinkstoimage'            => '冇頁面連結到箇隻檔案。',
'sharedupload'              => '箇隻檔案係隻共用嗰上傳，佢可以應用到別嗰項目。',
'shareduploadwiki'          => '更多信息請參看$1。',
'shareduploadwiki-linktext' => '檔案描述頁',
'noimage'                   => '同名嗰檔案伓存在，倷可以$1。',
'noimage-linktext'          => '上傳佢',
'uploadnewversion-linktext' => '上傳箇隻檔案嗰新版本',

# File reversion
'filerevert'                => '恢復$1',
'filerevert-legend'         => '恢復檔案',
'filerevert-intro'          => "眼下倷在恢復'''[[Media:$1|$1]]'''到[$4 於$2 $3嗰版本]。",
'filerevert-comment'        => '說明:',
'filerevert-defaultcomment' => '恢復到嘞$1, $2嗰版本',
'filerevert-submit'         => '恢復',
'filerevert-success'        => "'''[[Media:$1|$1]]'''恢復到嘞[$4 於$2 $3嗰版本]。",
'filerevert-badversion'     => '箇隻檔案所提供嗰時間標記並冇早先嗰本地版本。',

# File deletion
'filedelete'                  => '刪吥 $1',
'filedelete-legend'           => '刪吥檔案',
'filedelete-intro'            => "倷正刪吥'''[[Media:$1|$1]]'''。",
'filedelete-intro-old'        => "倷正刪吥'''[[Media:$1|$1]]'''到[$4 $2 $3]嗰版本。",
'filedelete-comment'          => '說明:',
'filedelete-submit'           => '刪吥',
'filedelete-success'          => "'''$1'''刪吥嘞。",
'filedelete-success-old'      => '<span class="plainlinks">\'\'\'[[Media:$1|$1]]\'\'\'於 $2 $3 嗰版本刪吥嘞。</span>',
'filedelete-nofile'           => "{{SITENAME}}箇隻網站伓存在'''$1'''。",
'filedelete-nofile-old'       => "按到指定屬性嗰情況，箇首冇'''$1'''到$2 $3嗰版本。",
'filedelete-iscurrent'        => '眼下倷正試得刪掉箇隻檔案嗰最近版本。請拿箇隻檔案退到稍舊嗰版本再著。',
'filedelete-otherreason'      => '別嗰/附加緣故:',
'filedelete-reason-otherlist' => '別嗰緣故',
'filedelete-reason-dropdown'  => '*常用刪除理由
** 侵犯版權
** 檔案重複',

# MIME search
'mimesearch'         => 'MIME 搜索',
'mimesearch-summary' => '箇隻頁面啟用檔案MIME類型篩檢程式。輸入：內容類型/子類型，像 <tt>image/jpeg</tt>。',
'mimetype'           => 'MIME 類型:',
'download'           => '下載',

# Unwatched pages
'unwatchedpages' => '冇眏到嗰頁面',

# List redirects
'listredirects' => '重定向頁面列表',

# Unused templates
'unusedtemplates'     => '冇使用嗰模板',
'unusedtemplatestext' => '箇隻頁面列出模板空間名下底冇拕別嗰頁面使用嗰頁面。刪掉箇兮模板前請檢查別嗰連到箇隻模板嗰頁面。',
'unusedtemplateswlh'  => '別嗰連結',

# Random page
'randompage'         => '隨機文章',
'randompage-nopages' => '箇隻名字空間冇嗰頁面。',

# Random redirect
'randomredirect'         => '隨機重定向頁面',
'randomredirect-nopages' => '箇隻名字空間冇重定向頁面。',

# Statistics
'statistics'             => '數據',
'sitestats'              => '{{SITENAME}} 數據',
'userstats'              => '用戶數據',
'sitestatstext'          => "資料庫攏共有'''$1'''頁。
佢就含到「討論」頁、關於{{SITENAME}}頁、滴古子大嗰「stub」頁、重定向頁，同到別嗰算伓正內容嗰頁。
冇算非內容頁嗰話，許'''$2'''頁就算得正係正噠正經嗰內容。

'''$8'''隻檔案上傳嘞。

到箇隻{{SITENAME}}建正以來，攏共有'''$3'''道瀏覽，同到'''$4'''回編輯。
平均每頁有'''$5'''道瀏覽，同到'''$6'''回編輯。

[http://www.mediawiki.org/wiki/Manual:Job_queue job queue] 嗰長度係'''$7'''。",
'userstatstext'          => "網站有'''$1'''隻註冊[[Special:ListUsers|用戶]]，其中 '''$2''' (或 '''$4%''') 有$5許可權。",
'statistics-mostpopular' => '眵嗰人最多嗰頁面',

'disambiguations'      => '扤清楚頁',
'disambiguations-text' => "底下嗰頁面都有到'''扤清楚頁'''嗰連結, 但係佢俚應當係連到正當嗰標題。<br />
如果一隻頁面係連結自[[MediaWiki:Disambiguationspage]]，佢會拕當成扤清楚頁。",

'doubleredirects'     => '雙重重定向頁面',
'doubleredirectstext' => '底下嗰重定向連結到別隻重定向頁面:',

'brokenredirects'        => '壞吥嗰重定向頁',
'brokenredirectstext'    => '底下嗰重定向頁面指到嗰係伓存在嗰頁面:',
'brokenredirects-edit'   => '(編寫)',
'brokenredirects-delete' => '(刪吥)',

'withoutinterwiki'         => '冇語言連結嗰頁面',
'withoutinterwiki-summary' => '底下嗰頁面係冇語言連結到別嗰語言版本:',
'withoutinterwiki-submit'  => '顯到',

'fewestrevisions' => '改得最少嗰文章',

# Miscellaneous special pages
'nbytes'                  => '$1字節',
'ncategories'             => '$1隻分類',
'nlinks'                  => '$1隻連結',
'nmembers'                => '$1隻成員',
'nrevisions'              => '$1隻改動',
'nviews'                  => '$1回瀏覽',
'specialpage-empty'       => '箇隻報告嗰結果係空嗰。',
'lonelypages'             => '孤立嗰頁面',
'lonelypagestext'         => '底下頁面冇連結到{{SITENAME}}箇別嗰頁面。',
'uncategorizedpages'      => '冇歸類嗰頁面',
'uncategorizedcategories' => '冇歸類嗰分類',
'uncategorizedimages'     => '冇歸類嗰文件',
'uncategorizedtemplates'  => '冇歸類嗰模版',
'unusedcategories'        => '冇使用嗰分類',
'unusedimages'            => '冇使用嗰圖像',
'popularpages'            => '熱門頁面',
'wantedcategories'        => '等撰嗰分類',
'wantedpages'             => '等撰嗰頁面',
'mostlinked'              => '最多連結嗰頁面',
'mostlinkedcategories'    => '最多連結嗰分類',
'mostlinkedtemplates'     => '最多連結嗰模版',
'mostcategories'          => '最多分類嗰文章',
'mostimages'              => '連結最多嗰圖像',
'mostrevisions'           => '最常改動嗰文章',
'prefixindex'             => '首碼索引',
'shortpages'              => '短文章',
'longpages'               => '長文章',
'deadendpages'            => '脫接頁面',
'deadendpagestext'        => '下底箇頁面冇連到{{SITENAME}}嗰別隻頁面:',
'protectedpages'          => '受保護頁面',
'protectedpagestext'      => '底下頁面已經受保護以防止亂動',
'protectedpagesempty'     => '箇兮參數下冇頁面拕保護到。',
'protectedtitles'         => '保護題目',
'listusers'               => '用戶列表',
'newpages'                => '新頁面',
'newpages-username'       => '用戶名:',
'ancientpages'            => '老早嗰頁面',
'move'                    => '移動',
'movethispage'            => '移動箇頁',
'unusedimagestext'        => '請注意別嗰網站直接用得正URL連結到箇隻圖像，故係箇首列到嗰圖像可能哈會拕使用。',
'unusedcategoriestext'    => '話係話冇拕別嗰文章或分類採用，但列表嗰分類頁哈係存在。',
'notargettitle'           => '冇目標',
'notargettext'            => '倷冇指正隻功能要用到嗰對象係頁面或用戶。',
'pager-newer-n'           => '{{PLURAL:$1|更新嗰 1|更新嗰 $1}}',
'pager-older-n'           => '{{PLURAL:$1|更舊嗰 1|更舊嗰 $1}}',

# Book sources
'booksources'               => '書籍來源',
'booksources-search-legend' => '尋吖書籍來源',
'booksources-go'            => '跳到',
'booksources-text'          => '底下係一部分網絡書店嗰連結列表，可以提供到倷要找嗰書籍嗰更多資料:',

# Special:Log
'specialloguserlabel'  => '用戶:',
'speciallogtitlelabel' => '標題:',
'log'                  => '日誌',
'all-logs-page'        => '所有日誌',
'log-search-legend'    => '尋日誌',
'log-search-submit'    => '去',
'alllogstext'          => '攏共顯到全部嗰日誌。倷能選隻日誌類型、用戶名或關聯頁面縮小顯示嗰範圍。',
'logempty'             => '日誌中冇符合嗰項目。',
'log-title-wildcard'   => '尋吖箇隻字開頭嗰標題',

# Special:AllPages
'allpages'          => '所有嗰頁面',
'alphaindexline'    => '$1到$2',
'nextpage'          => '下頁($1)',
'prevpage'          => '上頁($1)',
'allpagesfrom'      => '顯示以箇底開始嗰頁面:',
'allarticles'       => '全部文章',
'allinnamespace'    => '全部文章(歸$1空間名)',
'allnotinnamespace' => '全部文章(伓歸$1空間名)',
'allpagesprev'      => '前',
'allpagesnext'      => '後',
'allpagessubmit'    => '交',
'allpagesprefix'    => '以箇隻開頭嗰頁面:',
'allpagesbadtitle'  => '提供嗰頁面標題冇用，或有隻跨語言或跨wiki嗰字頭。佢可能含到一隻或幾隻字伓合標題。',
'allpages-bad-ns'   => '{{SITENAME}}冇名字空間叫"$1"嗰。',

# Special:Categories
'categories'         => '頁面分類',
'categoriespagetext' => '下底嗰分類包到頁面或係媒體文件。',

# Special:ListUsers
'listusersfrom'      => '顯示噉樣用戶條件:',
'listusers-submit'   => '顯示',
'listusers-noresult' => '尋伓到用戶。',

# E-mail user
'mailnologin'     => '冇email地址',
'mailnologintext' => '倷要[[Special:UserLogin|登入]] 起同到倷嗰[[Special:Preferences|參數設置]] 有隻有效嗰email才發得正email到別嗰用戶。',
'emailuser'       => '發email到箇隻用戶',
'emailpage'       => '發email到用戶',
'emailpagetext'   => '要係箇隻用戶到佢嗰參數設置頁填哩有效嗰email位置，下底嗰表格會寄隻信息到箇隻用戶。
倷到倷參數設置填嗰email位置會顯到email嗰「發信人」箇欄，咁樣箇隻用戶就回得正倷囉。',
'usermailererror' => 'Mail位置返回錯誤:',
'defemailsubject' => '{{SITENAME}} 電子郵件',
'noemailtitle'    => '冇電子郵件地址',
'noemailtext'     => '箇隻用戶哈冇指定正一隻有效嗰email，或者佢伓願收別嗰用戶嗰電子郵件。',
'emailfrom'       => '發信人',
'emailto'         => '收信人',
'emailsubject'    => '主題',
'emailmessage'    => '消息',
'emailsend'       => '發出',
'emailccme'       => '拿偶嗰消息嗰副本發到偶嗰郵箱。',
'emailccsubject'  => '拿倷嗰消息複製到 $1: $2',
'emailsent'       => 'email發卟嘞',
'emailsenttext'   => '倷嗰email發卟嘞。',

# Watchlist
'watchlist'            => '監視列表',
'mywatchlist'          => '偶嗰監視列表',
'watchlistfor'         => "('''$1'''嗰監視列表')",
'nowatchlist'          => '倷嗰監視列表什哩都冇有。',
'watchlistanontext'    => '請$1眵吖或改吖倷嗰監視列表。',
'watchnologin'         => '冇登入',
'watchnologintext'     => '倷要[[Special:UserLogin|登入]]起才改得正倷嗰監視列表。',
'addedwatch'           => '加到嘞監視列表',
'addedwatchtext'       => "頁面\"[[:\$1]]\" 加到嘞倷嗰[[Special:Watchlist|監視列表]]。箇頁同佢嗰討論頁嗰全部改動以後都會列到許首，佢會用'''粗體''' 列到[[Special:RecentChanges|最近更改]]讓倷更加容易識別。 倷以後要係拿佢到監視列表刪卟佢嗰話，就到導航條點吖「莫眏到」。",
'removedwatch'         => '莫眏到',
'removedwatchtext'     => '頁面"<nowiki>$1</nowiki>"到倷嗰監視列表刪卟嘞。',
'watch'                => '眏到',
'watchthispage'        => '眏到箇頁',
'unwatch'              => '莫眏到',
'unwatchthispage'      => '莫眏到箇頁',
'notanarticle'         => '伓係文章',
'watchnochange'        => '一徑到顯示嗰時間之內，倷眏到嗰頁面冇改動。',
'watchlist-details'    => '$1隻頁面(伓算討論頁) 拕眏到哩',
'wlheader-enotif'      => '* 啟動嘞email通知功能。',
'wlheader-showupdated' => "* 上回倷眵嗰頁面改動嗰部分用'''粗體'''顯到",
'watchmethod-recent'   => '眵吖拕眏到嗰頁面嗰最近編輯',
'watchmethod-list'     => '望吖監視頁中嗰最晏改動',
'watchlistcontains'    => '倷嗰監視列表包含$1隻頁面。',
'iteminvalidname'      => "頁面'$1'出錯，無效命名...",
'wlnote'               => "下底係最近'''$2'''鐘頭內嗰最晏'''$1'''道修改:",
'wlshowlast'           => '顯示近來$1鐘頭$2日$3嗰改動',
'watchlist-show-bots'  => '顯示機器人嗰編輯',
'watchlist-hide-bots'  => '弆到機器人嗰編輯',
'watchlist-show-own'   => '顯示偶嗰編輯',
'watchlist-hide-own'   => '弆到偶嗰編輯',
'watchlist-show-minor' => '顯到細編輯',
'watchlist-hide-minor' => '弆到細編輯',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => '眏到...',
'unwatching' => '莫眏到...',

'enotif_mailer'                => '{{SITENAME}}郵件報告員',
'enotif_reset'                 => '拿全部文章標成已讀',
'enotif_newpagetext'           => '箇係新開嗰頁面。',
'enotif_impersonal_salutation' => '{{SITENAME}}用戶',
'changed'                      => '改卟嘞',
'created'                      => '建正嘞',
'enotif_subject'               => '{{SITENAME}}有頁面 $PAGETITLE拕$PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited'           => '眵倷上回訪問後嗰全部改動請去$1。',
'enotif_lastdiff'              => '想眵改動請去$1。',
'enotif_anon_editor'           => '匿名用戶$1',
'enotif_body'                  => '$WATCHINGUSERNAME先生/小姐倷好，

$CHANGEDORCREATED{{SITENAME}}嗰 $PAGETITLE 頁面已經由$PAGEEDITOR到 $PAGEEDITDATE，請到 $PAGETITLE_URL眵吖目前嗰版本。

$NEWPAGE
編輯摘要: $PAGESUMMARY $PAGEMINOREDIT
聯絡箇隻編輯人: mail: $PAGEEDITOR_EMAIL

本站: $PAGEEDITOR_WIKI 今後伓會通知倷將來嗰改動，除非接到來到箇頁。倷也能設過倷全部監視頁嗰通知標記。

{{SITENAME}}通知系統 – 會改卟倷嗰監視列表設置，請去 {{fullurl:{{ns:special}}:Watchlist/edit}}

回饋同到別嗰説明: {{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => '刪卟頁面',
'confirm'                     => '確認',
'excontent'                   => "內容係: '$1'",
'excontentauthor'             => '內容係: \'$1\' (唯一嗰貢獻者係"$2")',
'exbeforeblank'               => "拕清空之前嗰內容係: '$1'",
'exblank'                     => '頁面冇內容',
'delete-confirm'              => '刪卟"$1"去',
'delete-legend'               => '刪卟去',
'historywarning'              => '警告: 倷要刪卟嗰頁面含到歷史版',
'confirmdeletetext'           => '仰上倷就要永久刪卟資料庫嗰一隻頁面或圖像同佢嗰歷史。請確定倷要噉做，哈要曉得佢嗰後果，更加伓能違反[[{{MediaWiki:Policy-url}}]]。',
'actioncomplete'              => '扤正嘞',
'deletedtext'                 => '"<nowiki>$1</nowiki>"刪卟嘞。最晏嗰刪除記錄請望$2。',
'deletedarticle'              => '"[[$1]]"刪卟嘞',
'dellogpage'                  => '刪除日誌',
'dellogpagetext'              => '下底係最晏刪除嗰記錄列表:',
'deletionlog'                 => '刪除日誌',
'reverted'                    => '恢復到早先嗰版本',
'deletecomment'               => '刪除嗰緣故:',
'deleteotherreason'           => '別嗰/附加理由:',
'deletereasonotherlist'       => '別嗰理由',
'deletereason-dropdown'       => '*常用刪除嗰理由
** 寫嗰人自家嗰要求
** 侵犯版權
** 特試破壞',
'rollback'                    => '還原修改',
'rollback_short'              => '還原',
'rollbacklink'                => '還原',
'rollbackfailed'              => '還原失敗',
'cantrollback'                => '還原伓正；最末嗰貢獻人係文章嗰唯一作者。',
'alreadyrolled'               => '還原伓正由[[User:$2|$2]] ([[User talk:$2|討論]])做嗰[[$1]]嗰最晏編寫；
別嗰人編輯過或係恢復嘞箇頁。

最晏編輯人: [[User:$3|$3]] ([[User talk:$3|討論]])。',
'editcomment'                 => '編輯介紹: "<i>$1</i>"。', # only shown if there is an edit comment
'revertpage'                  => '返回由[[Special:Contributions/$2|$2]] ([[User talk:$2|對話]])嗰編輯；恢復到[[User:$1|$1]]嗰最末一隻版本', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '返回由$1嗰編輯；恢復到$2嗰最末一隻版本。',
'sessionfailure'              => '倷嗰登入好像有嚸問題，為到防範未然，箇隻動作拕取消嘞。

請按吖“後退”再試過囉！',
'protectlogpage'              => '保護日誌',
'protectlogtext'              => '下底係頁面鎖定同到解除鎖定嗰列表。請望下[[Special:ProtectedPages|保護頁面列表]]來監察目前嗰頁面保護情況。',
'protectedarticle'            => '保護正嘞“[[$1]] ”',
'modifiedarticleprotection'   => '改變嘞“[[$1]] ” 嗰保護等級',
'unprotectedarticle'          => '撤銷保護“[[$1]] ”',
'protect-title'               => '保護“$1”中',
'protect-legend'              => '確認保護',
'protectcomment'              => '注解:',
'protectexpiry'               => '期限:',
'protect_expiry_invalid'      => '到期時間無效。',
'protect_expiry_old'          => '到期時間已過。',
'protect-unchain'             => '莫鎖到移動許可權',
'protect-text'                => '倷到箇首能瀏覽或修改頁面<strong><nowiki>$1</nowiki></strong>嗰保護級別。',
'protect-locked-blocked'      => '倷改伓正拕封鎖時嗰保護級別。下底係<strong>$1</strong>現今嗰保護級別:',
'protect-locked-dblock'       => '資料庫鎖到嘞就改伓正保護級別。下底係<strong>$1</strong>現今嗰保護級別:',
'protect-locked-access'       => '倷嗰許可權改伓正保護級別。

下底係<strong>$1</strong>現今嗰保護級別:',
'protect-cascadeon'           => '下底嗰{{PLURAL:$1|一隻|多隻}}頁面含到箇頁，佢哈啟動嘞連鎖保護，故係箇頁也就拕保護到嘞，編伓正。倷能設過箇頁嗰保護級別，但係箇伓會影響到連鎖保護。',
'protect-default'             => '(默認)',
'protect-fallback'            => '非要“$1”嗰許可',
'protect-level-autoconfirmed' => '禁止冇註冊嗰用戶',
'protect-level-sysop'         => '只限操作員',
'protect-summary-cascade'     => '聯鎖',
'protect-expiring'            => '$1 (UTC)到期',
'protect-cascade'             => '保護箇頁含到嗰頁面 (連鎖保護)',
'protect-cantedit'            => '倷改伓正箇頁嗰保護程度，因為倷冇搦到編輯授權。',
'restriction-type'            => '許可權:',
'restriction-level'           => '限制級別:',
'minimum-size'                => '最細碼子',
'maximum-size'                => '最大碼子:',
'pagesize'                    => '（字節）',

# Restrictions (nouns)
'restriction-edit'   => '編寫',
'restriction-move'   => '斢動',
'restriction-create' => '建立',

# Restriction levels
'restriction-level-sysop'         => '全保護',
'restriction-level-autoconfirmed' => '半保護',
'restriction-level-all'           => '任何等級',

# Undelete
'undelete'                     => '望吖刪卟嗰頁面',
'undeletepage'                 => '望吖同恢復刪卟嗰頁面',
'viewdeletedpage'              => '望吖刪卟嗰頁面',
'undeletepagetext'             => '下底嗰頁面拕刪卟嘞，但到檔案許首哈係恢復得正嗰。檔案庫會定時清理。',
'undeleteextrahelp'            => "要恢復艮隻頁面，請清除全部選擇方塊接到撳吖 '''''恢復'''''。要恢復選正嗰版本，就請揀到相應版本前嗰選擇方塊接到撳吖 '''''恢復'''''。撳 '''''重設''''' 就會清卟評論文字同到全部嗰選擇方塊。",
'undeleterevisions'            => '$1版本存正檔',
'undeletehistory'              => '如果倷要恢復箇頁，全部嗰版本都會跟到恢復到修改歷史去。如果箇頁刪卟後又有隻同名嗰新頁面，拕恢復嗰版本會係先前嗰歷史，而新頁面嗰如今修改伓會自動復原。',
'undeleterevdel'               => '如果最晏嗰修改拕刪卟，噉就扤得反刪除進行伓正。要係咁嗰話，倷就要反選到或反弆到最晏刪卟嗰修改。對於倷冇權限望嗰修改係恢復伓正嗰。',
'undeletehistorynoadmin'       => '箇篇文章刪卟嘞。下底嗰摘要會話原因，刪卟之前嗰全部編寫文本同到貢獻人嗰細節資料就管理員望得到。',
'undelete-revision'            => '刪卟$1由$3（到$2）編寫嗰修改版本:',
'undeleterevision-missing'     => '冇用或跌掉嗰修改版本。話伓定倷碰到隻錯誤嗰連結，要卟就係箇隻版本早從存檔恢復或換卟嘞。',
'undelete-nodiff'              => '冇尋到以前嗰版本。',
'undeletebtn'                  => '恢復',
'undeletelink'                 => '還原',
'undeletereset'                => '設過',
'undeletecomment'              => '評論:',
'undeletedarticle'             => '恢復正嗰"[[$1]]"',
'undeletedrevisions'           => '$1隻修改版本恢復正嘞',
'undeletedrevisions-files'     => '$1隻修改版本同$2隻檔案恢復正嘞',
'undeletedfiles'               => '$1隻檔案恢復正嘞',
'cannotundelete'               => '反刪除伓正；話伓定別嗰人先倷恢復嘞箇隻頁面。',
'undeletedpage'                => "<big>'''$1恢復正嘞'''</big>

望吖[[Special:Log/delete|刪除日誌]]嗰刪除同恢復記錄。",
'undelete-header'              => '要查最晏嗰記錄嗰話請望[[Special:Log/delete|刪除日誌]]。',
'undelete-search-box'          => '尋吖刪卟嗰頁面',
'undelete-search-prefix'       => '顯示以下底開頭嗰頁面:',
'undelete-search-submit'       => '尋吖',
'undelete-no-results'          => '刪卟記錄冇合到嗰結果。',
'undelete-filename-mismatch'   => '刪伓正帶到時間標記嗰檔案修訂 $1: 檔案伓匹配',
'undelete-bad-store-key'       => '刪伓正帶到時間標記嗰檔案修訂 $1: 檔案刪卟之前就跌卟嘞。',
'undelete-cleanup-error'       => '刪卟冇用嗰存檔文件 "$1" 時出錯。',
'undelete-missing-filearchive' => '資料庫冇檔案存檔 ID $1 ，故係佢也就到檔案存檔恢復伓正。佢話伓定早反刪除嘞。',
'undelete-error-short'         => '反刪除檔案嗰時間出錯: $1',
'undelete-error-long'          => '反刪除檔案當中出錯: $1',

# Namespace form on various pages
'namespace'      => '空間名:',
'invert'         => '反選',
'blanknamespace' => '(主要)',

# Contributions
'contributions' => '用戶貢獻',
'mycontris'     => '偶嗰貢獻',
'contribsub2'   => '$1嗰貢獻 ($2)',
'nocontribs'    => '冇尋到合到條件嗰改動。',
'uctop'         => '(頭上)',
'month'         => '從箇月 (或更早):',
'year'          => '從箇年 (或更早):',

'sp-contributions-newbies'     => '單顯到新用戶嗰貢獻',
'sp-contributions-newbies-sub' => '新用戶嗰貢獻',
'sp-contributions-blocklog'    => '封鎖記錄',
'sp-contributions-search'      => '尋貢獻',
'sp-contributions-username'    => 'IP地址或用戶名：',
'sp-contributions-submit'      => '尋',

# What links here
'whatlinkshere'       => '有什哩連到箇首',
'whatlinkshere-title' => '連到 $1 嗰頁面',
'whatlinkshere-page'  => '頁面:',
'linklistsub'         => '(連結列表)',
'linkshere'           => '下底嗰頁面連結到[[:$1]]：',
'nolinkshere'         => '冇頁面連結到[[:$1]]。',
'nolinkshere-ns'      => '選正嗰空間名內冇頁面連結到[[:$1]]。',
'isredirect'          => '重定向頁',
'istemplate'          => '含到',
'whatlinkshere-prev'  => '先$1隻',
'whatlinkshere-next'  => '末$1隻',
'whatlinkshere-links' => '←連結',

# Block/unblock
'blockip'                     => '封到IP地址',
'blockiptext'                 => '用下底嗰表格去阻止某一IP嗰修改許可權。除非倷係為到怕佢亂扤，接到非要符合[[{{MediaWiki:Policy-url}}|守則]]嗰條件下才能噉做。請到下底話隻確切原因（比如引用一隻拕破壞嗰頁面）。',
'ipaddress'                   => 'IP地址:',
'ipadressorusername'          => 'IP地址或用戶名:',
'ipbexpiry'                   => '期限:',
'ipbreason'                   => '原因:',
'ipbreasonotherlist'          => '別嗰原因',
'ipbreason-dropdown'          => '*一般嗰封鎖原因
** 緊編寫假嗰內容
** 刪卟文章內容
** 亂加外部連結
** 寫冇油鹽嗰話
** 嚇人／騷擾別嗰
** 濫用帳號
** 亂起用戶名',
'ipbanononly'                 => '光防到匿名用戶',
'ipbcreateaccount'            => '防止開新帳號',
'ipbemailban'                 => '防止用戶發email',
'ipbenableautoblock'          => '自動封鎖箇隻用戶最晏嗰IP，同後來佢編寫用過嗰地址',
'ipbsubmit'                   => '封鎖箇隻地址',
'ipbother'                    => '別嗰時間:',
'ipboptions'                  => '兩個鍾頭:2 hours,一日:1 day,三日:3 days,一個禮拜:1 week,兩個禮拜:2 weeks,一個月:1 month,三個月:3 months,六個月:6 months,一年:1 year,一世:infinite', # display1:time1,display2:time2,...
'ipbotheroption'              => '別嗰',
'ipbotherreason'              => '別嗰／附加原因:',
'ipbhidename'                 => '封鎖日誌、活躍封鎖列表同用戶列表裡頭弆到用戶名',
'badipaddress'                => 'IP位置伓對。',
'blockipsuccesssub'           => '封鎖正嘞',
'blockipsuccesstext'          => '[[Special:Contributions/$1|$1]]封卟嘞。 <br />望吖[[Special:IPBlockList|拕封IP列表]]來審過封鎖。',
'ipb-edit-dropdown'           => '編寫封鎖原因',
'ipb-unblock-addr'            => '解封$1',
'ipb-unblock'                 => '解封用戶名或IP地址',
'ipb-blocklist-addr'          => '望吖$1目前嗰封禁',
'ipb-blocklist'               => '望吖目前嗰封禁',
'unblockip'                   => '解封IP地址',
'unblockiptext'               => '用下底嗰表格去恢復早先拕封嗰IP嗰編寫權。',
'ipusubmit'                   => '解封箇隻地址',
'unblocked'                   => '[[User:$1|$1]]解封嘞',
'unblocked-id'                => '封禁$1拕刪卟嘞',
'ipblocklist'                 => '拕封IP列表',
'ipblocklist-legend'          => '尋吖拕封鎖嗰用戶',
'ipblocklist-username'        => '用戶名或IP地址:',
'ipblocklist-submit'          => '尋',
'blocklistline'               => '$1，$2封到$3 ($4)',
'infiniteblock'               => '伓限期',
'expiringblock'               => '$1到期',
'anononlyblock'               => '單限制匿名用戶',
'noautoblockblock'            => '停用自動封鎖',
'createaccountblock'          => '禁止新開帳戶',
'emailblock'                  => '禁止email',
'ipblocklist-empty'           => '封鎖列表係空嗰。',
'ipblocklist-no-results'      => '請求嗰IP地址/用戶名冇拕封到。',
'blocklink'                   => '封到',
'unblocklink'                 => '解封',
'contribslink'                => '貢獻',
'autoblocker'                 => '倷同"[[$1]]"共用一隻IP，故係倷也拕自動鎖到嘞。$1封鎖嗰緣故係"$2"。',
'blocklogpage'                => '封鎖日誌',
'blocklogentry'               => '[[$1]]拕封到$3 ，結束時間到$2',
'blocklogtext'                => '箇係用戶封鎖同解封操作嗰日誌。拕自動封鎖嗰IP冇列出。請參看[[Special:IPBlockList|拕封IP地址列表]]。',
'unblocklogentry'             => '[[$1]] 拕解封嘞',
'block-log-flags-anononly'    => '單限制匿名用戶',
'block-log-flags-nocreate'    => '禁止箇隻IP/用戶新開帳戶',
'block-log-flags-noautoblock' => '禁用自動封禁',
'block-log-flags-noemail'     => '禁止email',
'range_block_disabled'        => '就管理員建得正禁止封鎖嗰範圍。',
'ipb_expiry_invalid'          => '冇用嗰結束時間。',
'ipb_already_blocked'         => '鎖到嘞"$1"',
'ipb_cant_unblock'            => '錯誤: 冇發現Block ID $1。箇隻IP話伓定拕解封嘍。',
'ip_range_invalid'            => '冇用嗰IP範圍。',
'blockme'                     => '封吥偶去',
'proxyblocker'                => '代理封鎖器',
'proxyblocker-disabled'       => '箇隻功能用伓正嘍。',
'proxyblockreason'            => '倷嗰IP係一隻公開嗰代理，佢拕封到嘞。請聯絡倷嗰Internet服務提供商或技術幫助再告誦佢俚箇隻嚴重嗰安全問題。',
'proxyblocksuccess'           => '扤正囉。',
'sorbsreason'                 => '{{SITENAME}}用嗰 DNSBL 查到倷嗰IP地址係隻公開代理服務器。',
'sorbs_create_account_reason' => '{{SITENAME}}用嗰 DNSBL 檢查到倷嗰IP地址係隻公開代理服務器，倷也就新開伓正帳戶。',

# Developer tools
'lockdb'              => '鎖到資料庫',
'unlockdb'            => '莫鎖到資料庫',
'lockdbtext'          => '鎖住資料庫將讓所有用戶編伓正頁面、更伓正參數、監視列表同到別嗰需要改動資料庫嗰操作。請確定倷要噉做，接到要話正等維護工作結束後倷會重新開到資料庫。',
'unlockdbtext'        => '開到資料庫將讓所有用戶重新編輯得正頁面、修改得正參數、編輯得正監視列表同到別嗰需要改動資料庫嗰操作。請確定倷要噉做。',
'lockconfirm'         => '係嗰，偶係真嗰想鎖定資料庫。',
'unlockconfirm'       => '係嗰，偶係真嗰想解鎖資料庫。',
'lockbtn'             => '鎖到資料庫',
'unlockbtn'           => '莫鎖到資料庫',
'locknoconfirm'       => '倷冇選正確認鍵。',
'lockdbsuccesssub'    => '資料庫鎖正嘞',
'unlockdbsuccesssub'  => '資料庫解鎖',
'lockdbsuccesstext'   => '{{SITENAME}}資料庫鎖正嘞。 <br />請記得維護正後重新開到資料庫。',
'unlockdbsuccesstext' => '{{SITENAME}}資料庫重新開放。',
'lockfilenotwritable' => '資料庫鎖定檔案寫伓正。要鎖定或解鎖資料庫，需要由網絡服務器寫進才行。',
'databasenotlocked'   => '資料庫冇鎖正。',

# Move page
'move-page-legend'        => '換動頁面',
'movepagetext'            => "用下底嗰表格拿一隻頁面改名，跟到拿佢嗰歷史一齊般到新頁面。
舊嗰頁面就係新頁嗰重定向頁。
連到舊頁面嗰連結伓會自動更改；
勞煩檢查吖雙重或壞嗰重定向連結。
倷有責任確保全部連結會連到指正嗰頁面。

注意如果新頁面早就有嗰話，頁面'''伓會'''搬過去，要不新頁面就係冇內容或係重定向頁，也冇修訂歷史。
噉就係話必要時倷能等換到新頁面之後再又回到舊嗰頁面，跟到倷也覆蓋不正目前頁面。

'''警告！'''
對一隻訪問得多嗰頁面噉會係一隻重要同關鍵嗰改動；
請扤之前了解正佢噉可能嗰後果。",
'movepagetalktext'        => "相關嗰討論頁會自動同箇頁一齊搬走，'''除非''':
*新頁面有嘞隻有內容嗰討論頁，或
*倷伓選下底嗰選擇方塊。
噉倷就非要手工移動或合併頁面。",
'movearticle'             => '換動頁面:',
'movenotallowed'          => '倷到{{SITENAME}}冇權移動頁面。',
'newtitle'                => '新標題:',
'move-watch'              => '眏到箇頁',
'movepagebtn'             => '換卟箇頁',
'pagemovedsub'            => '換正嘞',
'articleexists'           => '已經有頁面叫箇隻名字，要伓倷揀嗰名字冇用。請揀過隻名字。',
'cantmove-titleprotected' => '倷移伓正一隻頁面到箇隻位置，箇隻新題目已經拕保護起來嘞，新建伓正。',
'talkexists'              => '頁面本身移動正嘞，但係新標題下底有嘞對話頁，所以對話頁移伓正。請手工合併兩頁。',
'movedto'                 => '移到',
'movetalk'                => '移動相關嗰討論頁',
'1movedto2'               => '[[$1]]移到[[$2]]',
'1movedto2_redir'         => '[[$1]]通過重定向移到[[$2]]',
'movelogpage'             => '移動日誌',
'movelogpagetext'         => '下底係移動嘞嗰頁面列表:',
'movereason'              => '原因',
'revertmove'              => '恢復',
'delete_and_move'         => '刪除跟到移動',
'delete_and_move_text'    => '==需要刪除==

目標文章"[[:$1]]"存在嘞。為到移動佢，倷要刪卟舊頁面？',
'delete_and_move_confirm' => '係嗰，刪卟箇頁',
'delete_and_move_reason'  => '為到移動刪卟佢',
'selfmove'                => '原始標題同目標標題一樣，一隻頁面移伓正到佢自家。',
'immobile_namespace'      => '目標標題係特別類型；面移伓進去箇隻空間名。',

# Export
'export'            => '導出頁面',
'exporttext'        => '通過XML格式倷能搦特定嗰頁面或一組頁面嗰文本同到佢編輯嗰歷史一齊導出；噉通過"[[Special:Import|導入頁面]]"就導入得到別嗰MediaWiki網站。要導出頁面嗰話，請到下底嗰文字框寫正標題，一行一隻標題，再話正倷係否要導出含歷史嗰舊版本，或單就選導出最晏一回編輯嗰相關內容。

再就係通過連結倷哈導出得正檔案，比如倷用得正[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]]導出"[[{{MediaWiki:Mainpage}}]]"頁面',
'exportcuronly'     => '獨今吖嗰改動，伓係全部嗰歷史。',
'exportnohistory'   => "----
'''注意:''' 由於性能嗰原因，箇隻表格導出嗰頁面嗰全部歷史都拕禁用。",
'export-submit'     => '導出',
'export-addcattext' => '從分類裡頭加進頁面:',
'export-addcat'     => '加入',
'export-download'   => '提供一隻檔案去另存',
'export-templates'  => '包括模板',

# Namespace 8 related
'allmessages'               => '系統消息',
'allmessagesname'           => '名字',
'allmessagesdefault'        => '默認文字',
'allmessagescurrent'        => '眼前嗰文字',
'allmessagestext'           => '箇首列到全部制定得正嗰系統界面。
Please visit [http://www.mediawiki.org/wiki/Localisation MediaWiki Localisation] and [http://translatewiki.net Betawiki] if you wish to contribute to the generic MediaWiki localisation.',
'allmessagesnotsupportedDB' => "'''{{ns:special}}:系統界面功能'''關卟嘞('''\$wgUseDatabaseMessages''')。",
'allmessagesfilter'         => '按消息名篩:',
'allmessagesmodified'       => '單顯到改正嗰',

# Thumbnails
'thumbnail-more'           => '放大',
'filemissing'              => '尋伓到檔案',
'thumbnail_error'          => '縮略圖冇扤正: $1',
'djvu_page_error'          => 'DjVu頁超出範圍',
'djvu_no_xml'              => 'DjVu檔案拿伓出XML',
'thumbnail_invalid_params' => '縮略圖參數係錯嗰',
'thumbnail_dest_directory' => '建伓正目標目錄',

# Special:Import
'import'                     => '導入頁面',
'importinterwiki'            => '跨wiki導入',
'import-interwiki-text'      => '揀正隻wiki同頁面標題去導入。修訂日期同編輯人會一齊存到。全部嗰跨 wiki 導入操作會到[[Special:Log/import|導入日誌]]記到。',
'import-interwiki-history'   => '複製箇頁嗰全部歷史',
'import-interwiki-submit'    => '導入',
'import-interwiki-namespace' => '拿頁面移到空間名:',
'importtext'                 => '請用 Special:Export 從源 wiki 導出檔案，再存到倷嗰磁盤然後上傳到箇首。',
'importstart'                => '導入頁面中...',
'import-revision-count'      => '$1隻修改',
'importnopages'              => '冇導入嗰頁面。',
'importfailed'               => '導入伓正: $1',
'importunknownsource'        => '不明嗰源導入類型',
'importcantopen'             => '開伓正導入檔案',
'importbadinterwiki'         => '扤壞嗰內部wiki連結',
'importnotext'               => '空白或冇字',
'importsuccess'              => '導進去嘍！',
'importhistoryconflict'      => '挭過仗嗰修改歷史(之前就話伓定導過箇隻頁面)',
'importnosources'            => '跨Wiki導入源冇定義，哈伓準直接嗰歷史上傳。',
'importnofile'               => '冇上傳導入檔案。',
'importuploaderrorsize'      => '導入文件上傳嗰時間冇扤正。箇隻文件大傷嘍，上傳伓正咁大嗰文件。',
'importuploaderrorpartial'   => '導入文件上傳嗰時間冇扤正。箇隻文件就傳嘍一滴子。',
'importuploaderrortemp'      => '導入文件上傳嗰時間冇扤正。冇尋到臨時文件夾。',
'import-parse-failure'       => 'XML 導進分析失敗',
'import-noarticle'           => '冇頁面導入！',
'import-nonewrevisions'      => '早先嗰改動全部扤進去嘍。',
'xml-error-string'           => '$1 在 $2 行，$3 列 （$4字節）：$5',

# Import log
'importlogpage'                    => '導入日誌',
'importlogpagetext'                => '管理員由別嗰 wiki 導入頁面同到佢俚嗰編輯歷史記錄。',
'import-logentry-upload'           => '通過檔案上傳導入嗰[[$1]]',
'import-logentry-upload-detail'    => '$1隻修改',
'import-logentry-interwiki'        => '跨wiki $1',
'import-logentry-interwiki-detail' => '$2嗰$1隻修改',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '偶嗰用戶頁',
'tooltip-pt-anonuserpage'         => '倷編輯本站用嗰IP對應嗰用戶頁',
'tooltip-pt-mytalk'               => '偶嗰對話頁',
'tooltip-pt-anontalk'             => '對箇隻IP嗰編輯嗰話嗰事',
'tooltip-pt-preferences'          => '偶嗰參數設置',
'tooltip-pt-watchlist'            => '偶嗰監視列表',
'tooltip-pt-mycontris'            => '偶嗰貢獻列表',
'tooltip-pt-login'                => '登入係伓強制嗰，但佢會有蠻多好處',
'tooltip-pt-anonlogin'            => '登入係伓強制嗰，但佢會有蠻多好處',
'tooltip-pt-logout'               => '登出',
'tooltip-ca-talk'                 => '內容頁嗰討論',
'tooltip-ca-edit'                 => '倷編得正箇頁，但勞煩倷望佢一眼起再存到佢。',
'tooltip-ca-addsection'           => '開隻新嗰討論',
'tooltip-ca-viewsource'           => '箇頁已拕保護。但倷能望吖佢嗰原始碼。',
'tooltip-ca-history'              => '箇頁早先嗰版本',
'tooltip-ca-protect'              => '護到箇頁',
'tooltip-ca-delete'               => '刪卟箇頁',
'tooltip-ca-undelete'             => '拿箇頁還原到刪卟之前嗰樣子',
'tooltip-ca-move'                 => '移動箇頁',
'tooltip-ca-watch'                => '拿箇頁加到監視列表',
'tooltip-ca-unwatch'              => '拿箇頁從監視列表移走',
'tooltip-search'                  => '尋吖{{SITENAME}}',
'tooltip-search-go'               => '要係一樣嗰標題存在嗰話就直接去箇一版',
'tooltip-search-fulltext'         => '尋箇隻文字嗰頁面',
'tooltip-p-logo'                  => '封面',
'tooltip-n-mainpage'              => '眵吖封面',
'tooltip-n-portal'                => '對於箇隻計劃，倷能做什哩，又啷做',
'tooltip-n-currentevents'         => '提供目前嗰事嗰背景',
'tooltip-n-recentchanges'         => '列出箇隻網站該朝子嗰改動',
'tooltip-n-randompage'            => '隨機載進一隻頁面',
'tooltip-n-help'                  => '求人幫',
'tooltip-t-whatlinkshere'         => '列出全部同箇頁連到嗰頁面',
'tooltip-t-recentchangeslinked'   => '從箇頁連出嗰全部頁面嗰改動',
'tooltip-feed-rss'                => '箇頁嗰RSS訂閱',
'tooltip-feed-atom'               => '箇頁嗰Atom訂閱',
'tooltip-t-contributions'         => '望吖箇隻用戶嗰貢獻',
'tooltip-t-emailuser'             => '發封郵件到箇隻用戶',
'tooltip-t-upload'                => '上傳圖像或多媒體文件',
'tooltip-t-specialpages'          => '全部特殊頁列表',
'tooltip-t-print'                 => '箇隻頁面嗰打印版',
'tooltip-t-permalink'             => '箇隻頁面嗰永久連結',
'tooltip-ca-nstab-main'           => '望吖內容頁',
'tooltip-ca-nstab-user'           => '望吖用戶頁',
'tooltip-ca-nstab-media'          => '望吖媒體頁',
'tooltip-ca-nstab-special'        => '箇係隻特殊頁，倷編佢伓正',
'tooltip-ca-nstab-project'        => '望吖計劃頁',
'tooltip-ca-nstab-image'          => '望吖圖像頁',
'tooltip-ca-nstab-mediawiki'      => '望吖系統消息',
'tooltip-ca-nstab-template'       => '望吖模板',
'tooltip-ca-nstab-help'           => '望吖幫助頁',
'tooltip-ca-nstab-category'       => '望吖分類頁',
'tooltip-minoredit'               => '拿佢設成細修改',
'tooltip-save'                    => '存到倷嗰修改',
'tooltip-preview'                 => '預覽倷嗰改動，存到佢之前勞煩噉扤吖！',
'tooltip-diff'                    => '顯出倷對文章嗰改動。',
'tooltip-compareselectedversions' => '望吖箇頁兩隻選定版本之間嗰伓同之處。',
'tooltip-watch'                   => '拿箇頁加到倷嗰監視列表',
'tooltip-recreate'                => '管佢係否會拕刪卟都重新扤過箇頁。',
'tooltip-upload'                  => '開始上傳',

# Stylesheets
'common.css'   => '/** 箇首嗰CSS會用到全部嗰皮膚 */',
'monobook.css' => '/* 箇首嗰 CSS 會礙到正用Monobook皮膚嗰用戶 */',

# Scripts
'common.js'   => '/* 箇首嗰JavaScript仰上載進到所有用戶全部頁面。 */',
'monobook.js' => '/* 伓再使用；請用[[MediaWiki:common.js]] */',

# Metadata
'nodublincore'      => 'Dublin Core RDF metadata到箇隻服務器用伓正。',
'nocreativecommons' => 'Creative Commons RDF metadata到箇隻服務器用伓正。',
'notacceptable'     => '箇隻網站服務器提供伓正倷嗰用戶端認得嗰格式。',

# Attribution
'anonymous'        => '{{SITENAME}}嗰匿名用戶',
'siteuser'         => '{{SITENAME}}用戶$1',
'lastmodifiedatby' => '箇頁由$3對$1 $2最晏嗰改動。', # $1 date, $2 time, $3 user
'othercontribs'    => '以$1為基礎。',
'others'           => '別嗰',
'siteusers'        => '{{SITENAME}}用戶$1',
'creditspage'      => '頁面感謝',
'nocredits'        => '箇頁冇致謝名單。',

# Spam protection
'spamprotectiontitle' => '垃圾廣告隔離器',
'spamprotectiontext'  => '倷想存嗰頁面拕垃圾廣告隔離器測到。噉可能係外部連結扤得。',
'spamprotectionmatch' => '下底係觸發垃圾廣告隔離器嗰內容: $1',
'spambot_username'    => 'MediaWiki 廣告清除',
'spam_reverting'      => '返回到伓包連到$1最晏嗰版本',
'spam_blanking'       => '全部包含連到$1嗰改動，留空',

# Info page
'infosubtitle'   => '頁面嗰信息',
'numedits'       => '編輯數 (文章): $1',
'numtalkedits'   => '編輯數 (討論頁): $1',
'numwatchers'    => '監視人數: $1',
'numauthors'     => '作者人數 (文章): $1',
'numtalkauthors' => '作者人數 (討論頁): $1',

# Math options
'mw_math_png'    => '全部使用PNG圖像',
'mw_math_simple' => '要係簡單嗰話都用HTML，不然就用PNG圖像',
'mw_math_html'   => '可以嗰話都用HTML，不然就用PNG圖像',
'mw_math_source' => '顯示係TeX代碼 (文字瀏覽器用)',
'mw_math_modern' => '新版瀏覽器嗰建議',
'mw_math_mathml' => '儘可能用MathML (試驗中)',

# Patrolling
'markaspatrolleddiff'                 => '標到係檢查過嗰',
'markaspatrolledtext'                 => '標到箇篇文章係檢查過嗰',
'markedaspatrolled'                   => '標到係檢查過嗰',
'markedaspatrolledtext'               => '選正嗰版本標到係檢查過嗰。',
'rcpatroldisabled'                    => '近來修改檢查拕關閉',
'rcpatroldisabledtext'                => '該朝子改動檢查嗰功能拕關閉嘞。',
'markedaspatrollederror'              => '標伓正佢係檢查過嗰',
'markedaspatrollederrortext'          => '倷要指正某隻版本才標得正佢係檢查過嗰。',
'markedaspatrollederror-noautopatrol' => '倷標伓正倷自家嗰修改係檢查過嗰。',

# Patrol log
'patrol-log-page' => '巡查記錄',
'patrol-log-line' => '標正嘞$1/$2版係檢查過嗰$3',
'patrol-log-auto' => '(自動)',

# Image deletion
'deletedrevision'                 => '刪卟嘞舊版本$1。',
'filedeleteerror-short'           => '刪卟檔案出錯: $1',
'filedeleteerror-long'            => '刪卟檔案出嘞錯:

$1',
'filedelete-missing'              => '檔案 "$1" 伓存在，所以刪佢伓正。',
'filedelete-old-unregistered'     => '指正嗰檔案修改 "$1" 資料庫裡伓存在。',
'filedelete-current-unregistered' => '指正嗰檔案 "$1" 資料庫裡伓存在。',
'filedelete-archive-read-only'    => '存檔目錄 "$1" 服務器裡寫伓正。',

# Browsing diffs
'previousdiff' => '←上一隻差異',
'nextdiff'     => '下一隻差異→',

# Media information
'mediawarning'         => "'''警告''': 話伓定箇隻檔案含到惡意代碼，執行佢話伓定會損壞倷嗰系統。<hr />",
'imagemaxsize'         => '檔案解釋頁嗰圖像大細限制到:',
'thumbsize'            => '縮略圖大細:',
'widthheightpage'      => '$1×$2,$3頁',
'file-info'            => '(檔案大細: $1, MIME 類型: $2)',
'file-info-size'       => '($1 × $2 像素，檔案大細：$3 ，MIME類型：$4)',
'file-nohires'         => '<small>冇更高解像度嗰圖像。</small>',
'svg-long-desc'        => '(SVG檔案，表面大細： $1 × $2 像素，檔案大細：$3)',
'show-big-image'       => '完整解析度',
'show-big-image-thumb' => '<small>箇幅縮略圖嗰解析度: $1 × $2 像素</small>',

# Special:NewImages
'newimages'             => '新建圖像畫廊',
'imagelisttext'         => '底下係按$2排列嗰$1隻檔案列表。',
'showhidebots'          => '($1機器人)',
'noimages'              => '冇什哩可望。',
'ilsubmit'              => '尋',
'bydate'                => '按日子',
'sp-newimages-showfrom' => '顯示從 $1 起嗰新文件',

# Bad image list
'bad_image_list' => '請根據下底嗰格式去寫:

會考慮單列到嗰項目（以*開頭嗰項目）。
頭隻連結非要連到隻壞圖。
之後同一行嗰連結會考慮係特殊，也就係話係幅圖都能到哪篇文章同時顯示得正。',

# Metadata
'metadata'          => '元數據',
'metadata-help'     => '箇隻檔案含到額外嗰信息。咁可能係數碼相機或掃描儀扤得。 要係改吥箇隻檔嗰源檔案，佢嗰資料伓見得會同改過後一樣。',
'metadata-expand'   => '顯到詳細資料',
'metadata-collapse' => '弆到詳細資料',
'metadata-fields'   => '箇隻信息列到嗰 EXIF 元數據表會含到圖片顯示頁面裡頭, 要係元數據表扤壞嘞就只會顯下底嗰資料，別嗰元數據會自動弆到。
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => '闊',
'exif-imagelength'                 => '高',
'exif-bitspersample'               => '每像素byte數',
'exif-compression'                 => '壓縮方法',
'exif-photometricinterpretation'   => '像素合成',
'exif-orientation'                 => '擺放方向',
'exif-samplesperpixel'             => '像素數',
'exif-planarconfiguration'         => '數據排列',
'exif-ycbcrsubsampling'            => '黃色對洋紅二次抽樣比率',
'exif-ycbcrpositioning'            => '黃色同洋紅配置',
'exif-xresolution'                 => '橫解析度',
'exif-yresolution'                 => '直解析度',
'exif-resolutionunit'              => 'X軸同Y軸嗰解析度單位',
'exif-stripoffsets'                => '圖像資料位置',
'exif-rowsperstrip'                => '每帶行數',
'exif-stripbytecounts'             => '每壓縮帶byte數',
'exif-jpeginterchangeformat'       => 'JPEG SOI嗰偏移量',
'exif-jpeginterchangeformatlength' => 'JPEG嗰byte數',
'exif-transferfunction'            => '轉移功能',
'exif-whitepoint'                  => '白點色度',
'exif-primarychromaticities'       => '主要嗰色度',
'exif-ycbcrcoefficients'           => '顏色空間轉換矩陣系數',
'exif-referenceblackwhite'         => '黑白參照值',
'exif-datetime'                    => '檔案改動日期同時間',
'exif-imagedescription'            => '圖像標題',
'exif-make'                        => '相機廠商',
'exif-model'                       => '相機型號',
'exif-software'                    => '用嗰軟件',
'exif-artist'                      => '作者',
'exif-copyright'                   => '版權人',
'exif-exifversion'                 => 'Exif版本',
'exif-flashpixversion'             => '支持嗰Flashpix版本',
'exif-colorspace'                  => '顏色空間',
'exif-componentsconfiguration'     => '每部分嗰意思',
'exif-compressedbitsperpixel'      => '圖像壓縮模式',
'exif-pixelydimension'             => '有效圖像嗰闊',
'exif-pixelxdimension'             => '有效圖像嗰高',
'exif-makernote'                   => '廠商摘要',
'exif-usercomment'                 => '用戶摘要',
'exif-relatedsoundfile'            => '相關嗰聲氣資料',
'exif-datetimeoriginal'            => '資料創作時間',
'exif-datetimedigitized'           => '數碼化嗰時間',
'exif-subsectime'                  => '日期時間秒',
'exif-subsectimeoriginal'          => '原始日期時間秒',
'exif-subsectimedigitized'         => '數碼化日期時間秒',
'exif-exposuretime'                => '曝光長度',
'exif-exposuretime-format'         => '$1 秒 ($2)',
'exif-fnumber'                     => '光圈(F值)',
'exif-exposureprogram'             => '曝光模式',
'exif-spectralsensitivity'         => '感光度',
'exif-isospeedratings'             => 'ISO速率',
'exif-oecf'                        => '光電轉換因數',
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
'exif-spatialfrequencyresponse'    => '空間頻率反應',
'exif-focalplanexresolution'       => '焦平面X軸嗰解析度',
'exif-focalplaneyresolution'       => '焦平面Y軸嗰解析度',
'exif-focalplaneresolutionunit'    => '焦平面嗰解析度單位',
'exif-subjectlocation'             => '主體位置',
'exif-exposureindex'               => '曝光指數',
'exif-sensingmethod'               => '感光模式',
'exif-filesource'                  => '檔案來源',
'exif-scenetype'                   => '場景類型',
'exif-cfapattern'                  => 'CFA模式',
'exif-customrendered'              => '自定義圖像處理',
'exif-exposuremode'                => '曝光模式',
'exif-whitebalance'                => '白平衡',
'exif-digitalzoomratio'            => '數碼放大比例',
'exif-focallengthin35mmfilm'       => '35毫米膠片焦距',
'exif-scenecapturetype'            => '場景拍攝類型',
'exif-gaincontrol'                 => '場景控制',
'exif-contrast'                    => '對比度',
'exif-saturation'                  => '飽和度',
'exif-sharpness'                   => '清晰度',
'exif-devicesettingdescription'    => '設備設定描述',
'exif-subjectdistancerange'        => '主體距離範圍',
'exif-imageuniqueid'               => '圖像獨有ID',
'exif-gpsversionid'                => 'GPS定位(tag)版本',
'exif-gpslatituderef'              => '南北緯',
'exif-gpslatitude'                 => '緯度',
'exif-gpslongituderef'             => '東西經',
'exif-gpslongitude'                => '經度',
'exif-gpsaltituderef'              => '海拔參照值',
'exif-gpsaltitude'                 => '海拔',
'exif-gpstimestamp'                => 'GPS時間(原子鐘)',
'exif-gpssatellites'               => '測量用嗰衛星',
'exif-gpsstatus'                   => '接收器狀態',
'exif-gpsmeasuremode'              => '測量模式',
'exif-gpsdop'                      => '測量精度',
'exif-gpsspeedref'                 => '速度單位',
'exif-gpsspeed'                    => 'GPS接收器速度',
'exif-gpstrackref'                 => '移動方位參照',
'exif-gpstrack'                    => '移動方位',
'exif-gpsimgdirectionref'          => '圖像方位參照',
'exif-gpsimgdirection'             => '圖像方位',
'exif-gpsmapdatum'                 => '用嗰地理測量資料',
'exif-gpsdestlatituderef'          => '目標緯度參照',
'exif-gpsdestlatitude'             => '目標緯度',
'exif-gpsdestlongituderef'         => '目標經度嗰參照',
'exif-gpsdestlongitude'            => '目標經度',
'exif-gpsdestbearingref'           => '目標方位參照',
'exif-gpsdestbearing'              => '目標方位',
'exif-gpsdestdistanceref'          => '目標距離參照',
'exif-gpsdestdistance'             => '目標距離',
'exif-gpsprocessingmethod'         => 'GPS處理方法名',
'exif-gpsareainformation'          => 'GPS區功能變數名',
'exif-gpsdatestamp'                => 'GPS日期',
'exif-gpsdifferential'             => 'GPS差動修正',

# EXIF attributes
'exif-compression-1' => '冇壓縮',

'exif-unknowndate' => '未知嗰日期',

'exif-orientation-1' => '標準', # 0th row: top; 0th column: left
'exif-orientation-2' => '左右斢轉', # 0th row: top; 0th column: right
'exif-orientation-3' => '轉動180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => '上下翻轉', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '逆時針轉90°接到上下翻轉', # 0th row: left; 0th column: top
'exif-orientation-6' => '順時針轉90°', # 0th row: right; 0th column: top
'exif-orientation-7' => '順時針轉90°接到上下翻轉', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '逆時針轉90°', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => 'chunky格式',
'exif-planarconfiguration-2' => 'planar格式',

'exif-componentsconfiguration-0' => '伓存在',

'exif-exposureprogram-0' => '冇定義',
'exif-exposureprogram-1' => '手動',
'exif-exposureprogram-2' => '標準程式',
'exif-exposureprogram-3' => '光圈優先模式',
'exif-exposureprogram-4' => '快門優先模式',
'exif-exposureprogram-5' => '藝術程式(著重景深)',
'exif-exposureprogram-6' => '運動程式(著重快門速度)',
'exif-exposureprogram-7' => '人像模式(背景朦朧)',
'exif-exposureprogram-8' => '風景模式(聚焦背景)',

'exif-subjectdistance-value' => '$1米',

'exif-meteringmode-0'   => '未知',
'exif-meteringmode-1'   => '平均水準',
'exif-meteringmode-2'   => '中心加權平均測量',
'exif-meteringmode-3'   => '單點測',
'exif-meteringmode-4'   => '多點測',
'exif-meteringmode-5'   => '模式測量',
'exif-meteringmode-6'   => '局部測量',
'exif-meteringmode-255' => '別嗰',

'exif-lightsource-0'   => '未知',
'exif-lightsource-1'   => '日光燈',
'exif-lightsource-2'   => '螢光燈',
'exif-lightsource-3'   => '白熾燈',
'exif-lightsource-4'   => '閃光燈',
'exif-lightsource-9'   => '天晴',
'exif-lightsource-10'  => '多雲',
'exif-lightsource-11'  => '深色調陰影',
'exif-lightsource-12'  => '日光螢光燈(色溫 D 5700 – 7100K)',
'exif-lightsource-13'  => '日溫白色螢光燈(N 4600 – 5400K)',
'exif-lightsource-14'  => '冷白色螢光燈(W 3900 – 4500K)',
'exif-lightsource-15'  => '白色螢光 (WW 3200 – 3700K)',
'exif-lightsource-17'  => '標準光A',
'exif-lightsource-18'  => '標準光B',
'exif-lightsource-19'  => '標準光C',
'exif-lightsource-24'  => 'ISO攝影棚鎢燈',
'exif-lightsource-255' => '別嗰光源',

'exif-focalplaneresolutionunit-2' => '英寸',

'exif-sensingmethod-1' => '冇定義',
'exif-sensingmethod-2' => '一隻彩色區域感應器',
'exif-sensingmethod-3' => '兩隻彩色區域感應器',
'exif-sensingmethod-4' => '三隻彩色區域感應器',
'exif-sensingmethod-5' => '連續彩色區域感應器',
'exif-sensingmethod-7' => '三線感應器',
'exif-sensingmethod-8' => '連續彩色綫性感應器',

'exif-scenetype-1' => '直接照像圖片',

'exif-customrendered-0' => '標準程式',
'exif-customrendered-1' => '自定義程式',

'exif-exposuremode-0' => '自動曝光',
'exif-exposuremode-1' => '手動曝光',
'exif-exposuremode-2' => '自動曝光感知調節',

'exif-whitebalance-0' => '自動白平衡',
'exif-whitebalance-1' => '手動白平衡',

'exif-scenecapturetype-0' => '標準',
'exif-scenecapturetype-1' => '風景',
'exif-scenecapturetype-2' => '人像',
'exif-scenecapturetype-3' => '夜景',

'exif-gaincontrol-0' => '冇',
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
'exif-subjectdistancerange-1' => '宏觀',
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
'exif-gpsspeed-n' => '海浬每小時(節)',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => '真實方位',
'exif-gpsdirection-m' => '地磁方位',

# External editor support
'edit-externally'      => '用外部程式來編輯箇隻檔案',
'edit-externally-help' => '請參看[http://www.mediawiki.org/wiki/Manual:External_editors 設置步驟]瞭解別嗰內容。',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => '全部',
'imagelistall'     => '全部',
'watchlistall2'    => '全部',
'namespacesall'    => '全部',
'monthsall'        => '全部',

# E-mail address confirmation
'confirmemail'            => '確認email地址',
'confirmemail_noemail'    => '倷冇到倷嗰[[Special:Preferences|用戶設置]]設正一隻有效嗰電子郵件地址。',
'confirmemail_text'       => '箇隻網站要求倷用email功能之前確認下倷嗰email地址。按吖下底嗰鍵來發封確認郵件到倷嗰郵箱。佢會附帶一隻代碼連結；請到倷嗰瀏覽器打開箇隻連結來確認倷嗰email地址係有效嗰。',
'confirmemail_pending'    => '<div class="error">
一隻確認代碼發到倷嗰郵箱，噉可能要等幾分鐘。
要係冇收到，請申請過新嗰確認碼！</div>',
'confirmemail_send'       => '寄出確認碼',
'confirmemail_sent'       => '確認郵件發出嘞。',
'confirmemail_oncreate'   => '一隻確認碼發到倷嗰郵箱。箇隻代碼伓係話倷要仰上登入，但要係倷想用 wiki 嗰任何email嗰相關功能，就非要先提交箇隻代碼。',
'confirmemail_sendfailed' => '發送伓正確認郵件，請檢查email地址係否含到伓合字符。

郵件發送人回應: $1',
'confirmemail_invalid'    => '無效嗰確認碼，箇隻代碼過嘞期。',
'confirmemail_needlogin'  => '倷要$1去確認倷嗰email地址。',
'confirmemail_success'    => '倷嗰郵箱已得到嘞確認。嘎倷能登得正入同到使用箇隻網站。',
'confirmemail_loggedin'   => '倷嗰email地址已得到確認。',
'confirmemail_error'      => '確認過程出錯。',
'confirmemail_subject'    => '{{SITENAME}}電子郵件地址確認',
'confirmemail_body'       => 'IP地址$1嗰用戶(可能係倷)到{{SITENAME}}註冊嘞帳戶"$2"，並一同用嘞倷嗰email地址。 

請確認箇隻帳戶係歸倷嗰，接到啟動{{SITENAME}}裡頭嗰email功能。請到瀏覽器開到下底嗰連結: 

$3 

如果箇*伓係*倷，就冇必要打開箇隻連結。確認碼會到$4時間過期。',

# Scary transclusion
'scarytranscludedisabled' => '[跨網站嗰編碼轉換用伓正]',
'scarytranscludefailed'   => '[對伓住，提取$1失敗]',
'scarytranscludetoolong'  => '[對伓住，URL 太長]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">箇篇文章嗰引用:<br />$1</div>',
'trackbackremove'   => '([$1刪除])',
'trackbacklink'     => '引用',
'trackbackdeleteok' => '成功刪卟箇隻引用。',

# Delete conflict
'deletedwhileediting' => '警告: 倷編輯嗰時間有人刪卟嘞箇頁！',
'confirmrecreate'     => "倷編輯嗰時間，用戶[[User:$1|$1]]([[User talk:$1|對話]])因為下底原因刪卟嘞箇頁:
: ''$2''
請想正後再重建頁面。",
'recreate'            => '重建',

# HTML dump
'redirectingto' => '重定向到[[:$1]]...',

# action=purge
'confirm_purge'        => '想清卟箇頁嗰緩存?

$1',
'confirm_purge_button' => '做得',

# AJAX search
'searchcontaining' => "尋含到''$1''嗰文章。",
'searchnamed'      => "尋叫''$1''嗰文章。",
'articletitles'    => '以"$1"開頭嗰文章',
'hideresults'      => '弆到嗰結果',
'useajaxsearch'    => '用 AJAX 尋',

# Multipage image navigation
'imgmultipageprev' => '← 上頁',
'imgmultipagenext' => '下頁 →',
'imgmultigo'       => '確定！',

# Table pager
'ascending_abbrev'         => '增',
'descending_abbrev'        => '減',
'table_pager_next'         => '下頁',
'table_pager_prev'         => '上頁',
'table_pager_first'        => '首頁',
'table_pager_last'         => '末頁',
'table_pager_limit'        => '每頁顯到$1項',
'table_pager_limit_submit' => '去',
'table_pager_empty'        => '冇結果',

# Auto-summaries
'autosumm-blank'   => '移卟頁面嗰全部內容',
'autosumm-replace' => "正拿頁面換成 '$1'",
'autoredircomment' => '正重定向到[[$1]]',
'autosumm-new'     => '新頁: $1',

# Live preview
'livepreview-loading' => '正在載入…',
'livepreview-ready'   => '正在載入… 完成哩!',
'livepreview-failed'  => '即時預覽失敗! 試吖標準預覽。',
'livepreview-error'   => '連接失敗: $1 "$2" 試吖標準預覽。',

# Friendlier slave lag warnings
'lag-warn-normal' => '將將嗰$1秒之內嗰改動話伓正伓會顯到列表裡頭。',
'lag-warn-high'   => '資料庫咁慢，將將嗰$1秒嗰改動話伓正伓會顯到列表裡頭。',

# Watchlist editor
'watchlistedit-numitems'       => '倷嗰監視列表攏共有$1隻標題，佢伓包括對話頁。',
'watchlistedit-noitems'        => '倷嗰監視列表冇標題。',
'watchlistedit-normal-title'   => '編寫監視列表',
'watchlistedit-normal-legend'  => '到監視列表移卟標題',
'watchlistedit-normal-explain' => '倷嗰監視列表嗰標題會到下底顯到。想移卟隻標題，到佢前頭勾吖，跟到按吖移除標題。倷也能[[Special:Watchlist/raw|編輯原始監視列表]]或[[Special:Watchlist/clear|移除所全部標題]]。',
'watchlistedit-normal-submit'  => '移除標題',
'watchlistedit-normal-done'    => '$1隻標題從倷嗰監視列表移卟嘞:',
'watchlistedit-raw-title'      => '編寫原始監視列表',
'watchlistedit-raw-legend'     => '編寫原始監視列表',
'watchlistedit-raw-explain'    => '倷嗰監視列表嗰標題會到下底顯到，哈能利用箇隻表去加進同到移除標題；一行一隻標題。扤完後，按更新監視列表。倷也能[[Special:Watchlist/edit|標準編輯器]]。',
'watchlistedit-raw-titles'     => '標題:',
'watchlistedit-raw-submit'     => '更新監視列表',
'watchlistedit-raw-done'       => '倷嗰監視列表更新正嘞。',
'watchlistedit-raw-added'      => '加嘞$1隻標題:',
'watchlistedit-raw-removed'    => '移嘞$1隻標題:',

# Watchlist editing tools
'watchlisttools-view' => '眵吖相關更改',
'watchlisttools-edit' => '眵吖同到編寫監視列表',
'watchlisttools-raw'  => '編寫原始監視列表',

# Core parser functions
'unknown_extension_tag' => '伓認得嗰擴展標籤 "$1"',

# Special:Version
'version'                          => '版本', # Not used as normal message but as header for the special page itself
'version-extensions'               => '裝正嗰插件',
'version-specialpages'             => '特別嗰頁面',
'version-parserhooks'              => '解析器鉤子',
'version-variables'                => '變量',
'version-other'                    => '別嗰',
'version-mediahandlers'            => '媒體處理程序',
'version-extension-functions'      => '擴展功能',
'version-parser-extensiontags'     => '解析器擴展標籤',
'version-skin-extension-functions' => '封皮插件功能',
'version-hook-name'                => '鉤子名',
'version-hook-subscribedby'        => '訂閱人',
'version-version'                  => '版本',
'version-license'                  => '許可證',
'version-software'                 => '裝正嗰軟件',
'version-software-version'         => '版本',

# Special:FilePath
'filepath'        => '文件路徑',
'filepath-page'   => '文件：',
'filepath-submit' => '路徑',

# Special:FileDuplicateSearch
'fileduplicatesearch-filename' => '文件名:',
'fileduplicatesearch-submit'   => '尋',

# Special:SpecialPages
'specialpages'                 => '特殊頁',
'specialpages-group-redirects' => '重定向特殊頁面',

);
