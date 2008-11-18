<?php
/** Simplified Chinese (‪中文(简体)‬)
 *
 * @ingroup Language
 * @file
 *
 * @author O
 * @author Shinjiman
 */

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
	'媒体'	=> NS_MEDIA,
	'特殊'  => NS_SPECIAL,
	'对话'  => NS_TALK,
	'讨论'	=> NS_TALK,
	'用户'  => NS_USER,
	'用户对话' => NS_USER_TALK,
	'用户讨论' => NS_USER_TALK,
	# This has never worked so it's unlikely to annoy anyone if I disable it -- TS
	#'{{SITENAME}}_对话' => NS_PROJECT_TALK
	'图像' => NS_IMAGE,
	'图像对话' => NS_IMAGE_TALK,
	'模板'	=> NS_TEMPLATE,
	'模板讨论'=> NS_TEMPLATE_TALK,
	'帮助'	=> NS_HELP,
	'帮助讨论'=> NS_HELP_TALK,
	'分类'	=> NS_CATEGORY,
	'分类讨论'=> NS_CATEGORY_TALK,
);

$skinNames = array(
	'standard' => '标准',
	'nostalgia' => '怀旧',
	'cologneblue' => '科隆香水蓝',
	'modern' => '现代',
);

$extraUserToggles = array(
	'nolangconversion',
);
$datePreferences = array(
	'default',
	'ISO 8601',
);
$defaultDateFormat = 'zh';
$dateFormats = array(
	'zh time' => 'H:i',
	'zh date' => 'Y年n月j日 (l)',
	'zh both' => 'Y年n月j日 (D) H:i',
);

$bookstoreList = array(
	'AddALL' => 'http://www.addall.com/New/Partner.cgi?query=$1&type=ISBN',
	'PriceSCAN' => 'http://www.pricescan.com/books/bookDetail.asp?isbn=$1',
	'Barnes & Noble' => 'http://search.barnesandnoble.com/bookSearch/isbnInquiry.asp?isbn=$1',
	'亚马逊' => 'http://www.amazon.com/exec/obidos/ISBN=$1',
	'博客来书店' => 'http://www.books.com.tw/exep/prod/booksfile.php?item=$1',
	'三民书店' => 'http://www.sanmin.com.tw/page-qsearch.asp?ct=search_isbn&qu=$1',
	'天下书店' => 'http://www.cwbook.com.tw/search/result1.jsp?field=2&keyWord=$1',
	'新丝路书店' => 'http://www.silkbook.com/function/Search_list_book_data.asp?item=5&text=$1'
);

$messages = array(
# User preference toggles
'tog-underline'               => '链接下划线',
'tog-highlightbroken'         => '无效链接格式<a href="" class="new">像这样</a> (或者像这个<a href="" class="internal">?</a>)',
'tog-justify'                 => '段落对齐',
'tog-hideminor'               => '最近更改中隐藏小修改',
'tog-extendwatchlist'         => '增强监视列表以显示所有可用更改',
'tog-usenewrc'                => '增强最近更改 (JavaScript)',
'tog-numberheadings'          => '标题自动编号',
'tog-showtoolbar'             => '显示编辑工具条 (JavaScript)',
'tog-editondblclick'          => '双击时编辑页面 (JavaScript)',
'tog-editsection'             => '允许通过点击[编辑]链接编辑段落',
'tog-editsectiononrightclick' => '允许右击标题编辑段落 (JavaScript)',
'tog-showtoc'                 => '显示目录 (针对一页超过3个标题的页面)',
'tog-rememberpassword'        => '在这部电脑上记住我的密码',
'tog-editwidth'               => '编辑框具有最大宽度',
'tog-watchcreations'          => '将我创建的页面添加到我的监视列表',
'tog-watchdefault'            => '将我编辑的页面添加到我的监视列表',
'tog-watchmoves'              => '将我移动的页面添加到我的监视列表',
'tog-watchdeletion'           => '将我删除的页面添加到我的监视列表',
'tog-minordefault'            => '默认将编辑设置为小编辑',
'tog-previewontop'            => '在编辑框上方显示预览',
'tog-previewonfirst'          => '在首次编辑时显示预览',
'tog-nocache'                 => '禁用页面缓存',
'tog-enotifwatchlistpages'    => '在我的监视列表中的页面改变时发电子邮件通知我',
'tog-enotifusertalkpages'     => '在我的讨论页更改时发邮件通知我',
'tog-enotifminoredits'        => '在页面有微小编辑时也发邮件通知我',
'tog-enotifrevealaddr'        => '在通知电子邮件列表中显示我的电子邮件地址',
'tog-shownumberswatching'     => '显示监视此页的用户数',
'tog-fancysig'                => '原始签名 (没有自动链接)',
'tog-externaleditor'          => '默认使用外部编辑器  (高级者专用，需要在您的电脑上作出一些特别设置)',
'tog-externaldiff'            => '默认使用外部差异分析  (高级者专用，需要在您的电脑上作出一些特别设置)',
'tog-showjumplinks'           => '启用"转到"访问链接',
'tog-uselivepreview'          => '使用实时预览 (Javascript) (试验中)',
'tog-forceeditsummary'        => '当没有输入摘要时提醒我',
'tog-watchlisthideown'        => '在监视列表中隐藏我的编辑',
'tog-watchlisthidebots'       => '在监视列表中隐藏机器人的编辑',
'tog-watchlisthideminor'      => '在监视列表中隐藏微小更改',
'tog-nolangconversion'        => '不进行用字转换',
'tog-ccmeonemails'            => '把我发送给其他用户的邮件同时发送副本给我自己',
'tog-diffonly'                => '在比较两个修订版本差异时不显示页面内容',
'tog-showhiddencats'          => '显示隐藏分类',

'underline-always'  => '总是使用',
'underline-never'   => '从不使用',
'underline-default' => '浏览器默认',

'skinpreview' => '(预览)',

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
'pagecategories'                 => '$1个分类',
'category_header'                => '"$1"分类中的页面',
'subcategories'                  => '亚类',
'category-media-header'          => '"$1"分类中的媒体',
'category-empty'                 => "''这个分类中尚未包含任何页面或媒体。''",
'hidden-categories'              => '$1个隐藏分类',
'hidden-category-category'       => '隐藏分类', # Name of the category where hidden categories will be listed
'category-subcat-count'          => '{{PLURAL:$2|这个分类中只有以下的亚类。|这个分类中有以下的$1个亚类，共有$2个附分类。}}',
'category-subcat-count-limited'  => '这个分类中有$1个亚类。',
'category-article-count'         => '{{PLURAL:$2|这个分类中只有以下的页面。|这个分类中有以下的$1个页面，共有$2个页面。}}',
'category-article-count-limited' => '这个分类中有$1个页面。',
'category-file-count'            => '{{PLURAL:$2|这个分类中只有以下的文件。|这个分类中有以下的$1个文件，共有$2个文件。}}',
'category-file-count-limited'    => '这个分类中有$1个文件。',
'listingcontinuesabbrev'         => '续',

'mainpagetext'      => "<big>'''已成功安装 MediaWiki!'''</big>",
'mainpagedocfooter' => '请访问 [http://meta.wikimedia.org/wiki/Help:Contents 用户手册] 以获得使用此 wiki 软件的信息！

== 入门 ==

* [http://www.mediawiki.org/wiki/Manual:Configuration_settings MediaWiki 配置设置列表]
* [http://www.mediawiki.org/wiki/Manual:FAQ MediaWiki 常见问题解答]
* [http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce MediaWiki 发布邮件列表]',

'about'          => '关于',
'article'        => '页面',
'newwindow'      => '(在新窗口中打开)',
'cancel'         => '取消',
'qbfind'         => '查找',
'qbbrowse'       => '浏览',
'qbedit'         => '编辑',
'qbpageoptions'  => '页面选项',
'qbpageinfo'     => '页面信息',
'qbmyoptions'    => '我的选项',
'qbspecialpages' => '特殊页面',
'moredotdotdot'  => '更多...',
'mypage'         => '我的页面',
'mytalk'         => '我的对话页',
'anontalk'       => '该IP的对话页',
'navigation'     => '导航',
'and'            => '和',

# Metadata in edit box
'metadata_help' => '元数据:',

'errorpagetitle'    => '错误',
'returnto'          => '返回到$1。',
'tagline'           => '出自{{SITENAME}}',
'help'              => '帮助',
'search'            => '搜索',
'searchbutton'      => '搜索',
'go'                => '进入',
'searcharticle'     => '进入',
'history'           => '页面历史',
'history_short'     => '历史',
'updatedmarker'     => '我上次访问以来的修改',
'info_short'        => '资讯',
'printableversion'  => '可打印版',
'permalink'         => '永久链接',
'print'             => '打印',
'edit'              => '编辑',
'create'            => '创建',
'editthispage'      => '编辑此页',
'create-this-page'  => '创建此页',
'delete'            => '删除',
'deletethispage'    => '删除此页',
'undelete_short'    => '反删除$1项修订',
'protect'           => '保护',
'protect_change'    => '更改',
'protectthispage'   => '保护此页',
'unprotect'         => '解除保护',
'unprotectthispage' => '解除此页保护',
'newpage'           => '新建页面',
'talkpage'          => '讨论此页',
'talkpagelinktext'  => '对话',
'specialpage'       => '特殊页面',
'personaltools'     => '个人工具',
'postcomment'       => '发表评论',
'articlepage'       => '查看页面',
'talk'              => '讨论',
'views'             => '查看',
'toolbox'           => '工具箱',
'userpage'          => '查看用户页面',
'projectpage'       => '查看计划页面',
'imagepage'         => '查看媒体页面',
'mediawikipage'     => '查看信息页面',
'templatepage'      => '查看模板页面',
'viewhelppage'      => '查看帮助页面',
'categorypage'      => '查看分类页面',
'viewtalkpage'      => '查看讨论页面',
'otherlanguages'    => '其它语言',
'redirectedfrom'    => '(重定向自$1)',
'redirectpagesub'   => '重定向页面',
'lastmodifiedat'    => '这页的最后修订在 $1 $2。', # $1 date, $2 time
'viewcount'         => '本页面已经被浏览$1次。',
'protectedpage'     => '被保护页',
'jumpto'            => '跳转到:',
'jumptonavigation'  => '导航',
'jumptosearch'      => '搜索',

# All link text and link target definitions of links into project namespace that get used by other message strings, with the exception of user group pages (see grouppage) and the disambiguation template definition (see disambiguations).
'aboutsite'            => '关于{{SITENAME}}',
'aboutpage'            => 'Project:关于',
'bugreports'           => '错误报告',
'bugreportspage'       => 'Project:错误报告',
'copyright'            => '本站的全部文本内容在$1之条款下提供。',
'copyrightpagename'    => '{{SITENAME}}版权',
'copyrightpage'        => '{{ns:project}}:版权信息',
'currentevents'        => '当前事件',
'currentevents-url'    => 'Project:当前事件',
'disclaimers'          => '免责声明',
'disclaimerpage'       => 'Project:免责声明',
'edithelp'             => '编辑帮助',
'edithelppage'         => 'Help:如何编辑页面',
'faq'                  => '常见问题解答',
'faqpage'              => 'Project:常见问题解答',
'helppage'             => 'Help:目录',
'mainpage'             => '首页',
'mainpage-description' => '首页',
'policy-url'           => 'Project:方针',
'portal'               => '社区',
'portal-url'           => 'Project:社区',
'privacy'              => '隐私政策',
'privacypage'          => 'Project:隐私政策',

'badaccess'        => '权限错误',
'badaccess-group0' => '您刚才的请求不允许执行。',
'badaccess-group1' => '您刚才的请求只有$1用户组的用户才能使用。',
'badaccess-group2' => '您刚才的请求只有$1用户组的用户才能使用。',
'badaccess-groups' => '您刚才的请求只有$1用户组的用户才能使用。',

'versionrequired'     => '需要MediaWiki $1 版',
'versionrequiredtext' => '需要版本$1的 MediaWiki 才能使用此页。参见[[Special:Version|版本頁]]。',

'ok'                      => '确定',
'retrievedfrom'           => '取自"$1"',
'youhavenewmessages'      => '您有$1（$2）。',
'newmessageslink'         => '新信息',
'newmessagesdifflink'     => '上次更改',
'youhavenewmessagesmulti' => '您在 $1 有一条新信息',
'editsection'             => '编辑',
'editold'                 => '编辑',
'viewsourceold'           => '查看源码',
'editsectionhint'         => '编辑段落: $1',
'toc'                     => '目录',
'showtoc'                 => '显示',
'hidetoc'                 => '隐藏',
'thisisdeleted'           => '查看或恢复$1?',
'viewdeleted'             => '查看$1?',
'restorelink'             => '$1个被删除的版本',
'feedlinks'               => '订阅:',
'feed-invalid'            => '无效的订阅类型。',
'feed-unavailable'        => '联合订阅并无于{{SITENAME}}上提供',
'site-rss-feed'           => '$1的RSS订阅',
'site-atom-feed'          => '$1的Atom订阅',
'page-rss-feed'           => '“$1”的RSS订阅',
'page-atom-feed'          => '“$1”的Atom订阅',
'red-link-title'          => '$1 (尚未撰写)',

# Short words for each namespace, by default used in the namespace tab in monobook
'nstab-main'      => '页面',
'nstab-user'      => '用户页面',
'nstab-media'     => '媒体页面',
'nstab-special'   => '特殊页面',
'nstab-project'   => '计划页面',
'nstab-image'     => '文件',
'nstab-mediawiki' => '信息',
'nstab-template'  => '模板',
'nstab-help'      => '帮助页面',
'nstab-category'  => '分类',

# Main script and global functions
'nosuchaction'      => '这个命令不存在',
'nosuchactiontext'  => '这个wiki无法识别URL请求的命令',
'nosuchspecialpage' => '此特殊页面不存在',
'nospecialpagetext' => "<big>'''您请求的特殊页面无效。'''</big>

[[Special:SpecialPages|{{int:specialpages}}]]中载有所有有效特殊页面的列表。",

# General errors
'error'                => '错误',
'databaseerror'        => '数据库错误',
'dberrortext'          => '发生数据库查询语法错误。
可能是由于软件自身的错误所引起。
最后一次数据库查询指令是:
<blockquote><tt>$1</tt></blockquote>
来自于函数 "<tt>$2</tt>"。
MySQL返回错误 "<tt>$3: $4</tt>"。',
'dberrortextcl'        => '发生了一个数据库查询语法错误。
最后一次的数据库查询是:
“$1”
来自于函数“$2”。
MySQL返回错误“$3: $4”。',
'noconnect'            => '抱歉！网站遇到一些技术问题，无法连接数据库服务器。<br />
$1',
'nodb'                 => '无法选择数据库$1',
'cachederror'          => '以下页面是缓存中的副本，未必是最新版本。',
'laggedslavemode'      => '警告: 页面可能不包含最近的更新。',
'readonly'             => '数据库被锁定',
'enterlockreason'      => '请输入锁定的原因，包括预计重新开放的时间',
'readonlytext'         => '数据库目前禁止输入新内容及更改，
这很可能是由于数据库正在维修，完成后即可恢复。

管理员有如下解释: $1',
'missing-article'      => '数据库找不到页面文字"$1" $2。

通常这是由于修订历史页上过时的链接到已经被删除的页面所导致的。

如果情况不是这样，您可能找到了软件内的一个错误。
请记录下 URL 地址，并向[[Special:ListUsers/sysop|管理员]]报告。',
'missingarticle-rev'   => '(修订#: $1)',
'missingarticle-diff'  => '(差异: $1, $2)',
'readonly_lag'         => '附属数据库服务器正在将缓存更新到主服务器，数据库已被自动锁定',
'internalerror'        => '内部错误',
'internalerror_info'   => '内部错误: $1',
'filecopyerror'        => '无法复制文件"$1"到"$2"。',
'filerenameerror'      => '无法重命名文件"$1" 到"$2"。',
'filedeleteerror'      => '无法删除文件 "$1"。',
'directorycreateerror' => '无法创建目录"$1"。',
'filenotfound'         => '找不到文件 "$1"。',
'fileexistserror'      => '无法写入文件"$1": 文件已存在',
'unexpected'           => '非正常值: "$1"="$2"。',
'formerror'            => '错误: 无法提交表单',
'badarticleerror'      => '无法在此页进行此项操作。',
'cannotdelete'         => '无法删除选定的页面或图像（它可能已经被其他人删除了）。',
'badtitle'             => '错误的标题',
'badtitletext'         => '所请求页面的标题是无效的、不存在，跨语言或跨wiki链接的标题错误。它可能包含一个或更多的不能用于标题的字符。',
'perfdisabled'         => '抱歉！由于此项操作有可能造成数据库瘫痪，目前暂时无法使用。',
'perfcached'           => '下列是缓存数据，因此可能不是最新的:',
'perfcachedts'         => '下列是缓存数据，其最后更新时间是$1。',
'querypage-no-updates' => '当前禁止对此页面进行更新。此处的数据将不能被立即刷新。',
'wrong_wfQuery_params' => '错误参数被传递到 wfQuery()<br />
函数: $1<br />
查询: $2',
'viewsource'           => '源码',
'viewsourcefor'        => '对$1的源码',
'actionthrottled'      => '动作已压制',
'actionthrottledtext'  => '基于反垃圾的考量，您现在于这段短时间之中限制去作这一个动作，而您已经超过这个上限。请在数分钟后再尝试。',
'protectedpagetext'    => '该页面已被锁定以防止编辑。',
'viewsourcetext'       => '您可以查看并复制此页面的源码:',
'protectedinterface'   => '该页提供了软件的界面文本，它已被锁定以防止随意的修改。',
'editinginterface'     => "'''警告:''' 您正在编辑的页面是用于提供软件的界面文本。改变此页将影响其他用户的界面外观。如要翻译，请考虑使用[http://translatewiki.net/wiki/Main_Page?setlang=zh-hans Betawiki]，一个用来为MediaWiki软件本地化的计划。",
'sqlhidden'            => '(SQL查询已隐藏)',
'cascadeprotected'     => '这个页面已经被保护，因为这个页面被以下已标注"联锁保护"的{{PLURAL:$1|一个|多个}}被保护页面包含:
$2',
'namespaceprotected'   => "您并没有权限编辑'''$1'''名字空间内的页面。",
'customcssjsprotected' => '您并无权限去编辑这个页面，因为它包含了另一位用户的个人设定。',
'ns-specialprotected'  => '特殊页面是不可以编辑的。',
'titleprotected'       => '这个标题已经被[[User:$1|$1]]保护以防止创建。理由是<i>$2</i>。',

# Virus scanner
'virus-badscanner'     => "损坏设置: 未知的病毒扫瞄器: ''$1''",
'virus-scanfailed'     => '扫瞄失败 (代码 $1)',
'virus-unknownscanner' => '未知的反病毒:',

# Login and logout pages
'logouttitle'                => '退出',
'logouttext'                 => '<strong>您现在已经退出。</strong><br />
您可以继续以匿名方式使用{{SITENAME}}，或再次以相同或不同用户身份[[Special:UserLogin|登录]]。
请注意一些页面可能仍然显示您为登录状态，直到您清空您的浏览器缓存为止。',
'welcomecreation'            => '== 欢迎, $1! ==

 您的账户已经建立，不要忘记设置[[Special:Preferences|{{SITENAME}}的个人参数]]。',
'loginpagetitle'             => '用户登录',
'yourname'                   => '用户名:',
'yourpassword'               => '密码:',
'yourpasswordagain'          => '再次输入密码:',
'remembermypassword'         => '下次登录记住密码',
'yourdomainname'             => '您的域名:',
'externaldberror'            => '这可能是由于外部验证数据库错误或您被禁止更新您的外部账号。',
'loginproblem'               => '<b>登录有问题。</b><br />请再试一次！',
'login'                      => '登录',
'nav-login-createaccount'    => '登录／创建账户',
'loginprompt'                => '您必须启用 Cookies 才能登录{{SITENAME}}。',
'userlogin'                  => '登录／创建账户',
'logout'                     => '退出',
'userlogout'                 => '退出',
'notloggedin'                => '未登录',
'nologin'                    => '您还没有账户吗？$1。',
'nologinlink'                => '创建新账户',
'createaccount'              => '创建新账户',
'gotaccount'                 => '已经拥有账户？$1。',
'gotaccountlink'             => '登录',
'createaccountmail'          => '通过电子邮件',
'badretype'                  => '您所输入的密码并不相同。',
'userexists'                 => '您所输入的用户名已有人使用。请另选一个名。',
'youremail'                  => '电子邮件:',
'username'                   => '用户名:',
'uid'                        => '用户ID:',
'prefs-memberingroups'       => '{{PLURAL:$1|一|多}}组的成员:',
'yourrealname'               => '真实姓名:',
'yourlanguage'               => '界面语言:',
'yourvariant'                => '字体变换:',
'yournick'                   => '签名:',
'badsig'                     => '错误的原始签名。检查一下HTML标签。',
'badsiglength'               => '签名过长。
它的长度必须在$1个字符以下。',
'email'                      => '电子邮箱',
'prefs-help-realname'        => '真实姓名是可选的。
如果您选择提供它，那它便用以对您的贡献署名。',
'loginerror'                 => '登录错误',
'prefs-help-email'           => '电子邮件是可选的，但当您忘记您的个密码时可以将新密码寄回给您。您亦可以在您没有公开自己的用户身份时通过您的用户页或用户讨论页与您联系。',
'prefs-help-email-required'  => '需要电子邮件地址。',
'nocookiesnew'               => '已成功创建新账户！侦测到您已关闭 Cookies，请开启它并登录。',
'nocookieslogin'             => '本站利用 Cookies 进行用户登录，侦测到您已关闭 Cookies，请开启它并重新登录。',
'noname'                     => '你没有输入有效的用户名。',
'loginsuccesstitle'          => '登录成功',
'loginsuccess'               => '你现在以"$1"的身份登录{{SITENAME}}。',
'nosuchuser'                 => '找不到用户"$1"。检查您的拼写，或者[[Special:Userlogin/signup|建立一个新账户]]。',
'nosuchusershort'            => '没有一个名为“<nowiki>$1</nowiki>”的用户。请检查您输入的文字是否有错误。',
'nouserspecified'            => '你需要指定一个用户名。',
'wrongpassword'              => '您输入的密码错误，请再试一次。',
'wrongpasswordempty'         => '您没有输入密码，请重试！',
'passwordtooshort'           => '您的密码不正确或太短，不能少于$1个字元，而且必须跟用户名不同。',
'mailmypassword'             => '将新密码寄给我',
'passwordremindertitle'      => '{{SITENAME}}的新临时密码',
'passwordremindertext'       => '有人(可能是您，来自IP地址$1)已请求{{SITENAME}}的新密码 ($4)。
用户"$2"的一个新临时密码现在已被设置好为"$3"。
如果这个动作是您所指示的，您便需要立即登入并选择一个新的密码。

如果是其他人发出了该请求，或者您已经记起了您的密码并不准备改变它，
您可以忽略此消息并继续使用您的旧密码。',
'noemail'                    => '用户"$1"没有登记电子邮件地址。',
'passwordsent'               => '用户"$1"的新密码已经寄往所登记的电子邮件地址。
请在收到后再登录。',
'blocked-mailpassword'       => '您的IP地址处于查封状态而不允许编辑，为了安全起见，密码恢复功能已被禁用。',
'eauthentsent'               => '一封确认信已经发送到推荐的地址。在发送其它邮件到此账户前，您必须首先依照这封信中的指导确认这个电子邮箱真实有效。',
'throttled-mailpassword'     => '密码提醒已在最近$1小时内发送。为了安全起见，在每$1小时内只能发送一个密码提醒。',
'mailerror'                  => '发送邮件错误: $1',
'acct_creation_throttle_hit' => '对不起，您已经创建了$1个账号。你不能再创建了。',
'emailauthenticated'         => '您的电子邮箱地址已经于$1确认有效。',
'emailnotauthenticated'      => '您的邮箱地址<strong>还没被认证</strong>。以下功能将不会发送任何邮件。',
'noemailprefs'               => '<strong>指定一个电子邮箱地址以使用此功能</strong>',
'emailconfirmlink'           => '确认您的邮箱地址',
'invalidemailaddress'        => '邮箱地址格式不正确，请输入正确的邮箱地址或清空该输入框。',
'accountcreated'             => '已建立账户',
'accountcreatedtext'         => '$1的账户已经被创建。',
'createaccount-title'        => '在{{SITENAME}}中创建新账户',
'createaccount-text'         => '有人在{{SITENAME}}中利用您的邮箱创建了一个名为 "$2" 的新帐户($4)，密码是 "$3" 。您应该立即登入并更改密码。

如果该账户创建错误的话，您可以忽略此信息。',
'loginlanguagelabel'         => '语言: $1',

# Password reset dialog
'resetpass'               => '重设账户密码',
'resetpass_announce'      => '您是通过一个临时的发送到邮件中的代码登录的。要完成登录，您必须在这里设定一个新密码:',
'resetpass_text'          => '<!-- 在此处添加文本 -->',
'resetpass_header'        => '重设密码',
'resetpass_submit'        => '设定密码并登录',
'resetpass_success'       => '您的密码已经被成功更改！现在正为您登录...',
'resetpass_bad_temporary' => '无效的临时密码。您可能已成功地更改了您的密码，或者需要请求一个新的临时密码。',
'resetpass_forbidden'     => '无法在{{SITENAME}}上更改密码',
'resetpass_missing'       => '无表单数据。',

# Edit page toolbar
'bold_sample'     => '粗体文字',
'bold_tip'        => '粗体文字',
'italic_sample'   => '斜体文字',
'italic_tip'      => '斜体文字',
'link_sample'     => '链接标题',
'link_tip'        => '内部链接',
'extlink_sample'  => 'http://www.example.com 链接标题',
'extlink_tip'     => '外部链接(加前缀 http://)',
'headline_sample' => '大标题文字',
'headline_tip'    => '2级标题文字',
'math_sample'     => '在此插入数学公式',
'math_tip'        => '插入数学公式 (LaTeX)',
'nowiki_sample'   => '在此插入非格式文本',
'nowiki_tip'      => '插入非格式文本',
'image_tip'       => '嵌入文件',
'media_tip'       => '文件链接',
'sig_tip'         => '带有时间的签名',
'hr_tip'          => '水平线 (小心使用)',

# Edit pages
'summary'                          => '摘要',
'subject'                          => '标题',
'minoredit'                        => '这是一个小修改',
'watchthis'                        => '监视本页',
'savearticle'                      => '保存本页',
'preview'                          => '预览',
'showpreview'                      => '显示预览',
'showlivepreview'                  => '实时预览',
'showdiff'                         => '显示差异',
'anoneditwarning'                  => "'''警告:'''您没有登录，您的IP地址将记录在此页的编辑历史中。",
'missingsummary'                   => "'''提示:''' 您没有提供一个编辑摘要。如果您再次单击保存，您的编辑将不带编辑摘要保存。",
'missingcommenttext'               => '请在下面输入评论。',
'missingcommentheader'             => "'''提示:''' 您没有为此评论提供一个标题。如果您再次单击保存，您的编辑将不带标题保存。",
'summary-preview'                  => '摘要预览',
'subject-preview'                  => '标题预览',
'blockedtitle'                     => '用户被查封',
'blockedtext'                      => "<big>你的用户名或IP地址已经被$1查封。</big>

这次查封是由$1所封的。当中的原因是''$2''。

* 这次查封开始的时间是：$8
* 这次查封到期的时间是：$6
* 对于被查封者：$7

你可以联络$1或者其他的[[{{MediaWiki:Grouppage-sysop}}|管理员]]，讨论这次查封。
除非你已经在你的[[Special:Preferences|帐号参数设置]]中设置了一个有效的电子邮件地址，否则你是不能使用「电邮这位用户」的功能。当设置定了一个有效的电子邮件地址后，这个功能是不会封锁的。

你的IP地址是$3，而该查封ID是 #$5。 请在你的查询中注明以上所有的资料。",
'autoblockedtext'                  => "你的IP地址已经被自动查封，由于先前的另一位用户被$1所查封。
而查封的原因是：

:''$2''

* 这次查封的开始时间是：$8
* 这次查封的到期时间是：$6
* 对于被查封者：$7

你可以联络$1或者其他的[[{{MediaWiki:Grouppage-sysop}}|管理员]]，讨论这次查封。

除非你已经在你的[[Special:Preferences|帐号参数设置]]中设置了一个有效的电子邮件地址，否则你是不能使用「电邮这位用户」的功能。当设置定了一个有效的电子邮件地址后，这个功能是不会封锁的。

您现时正在使用的 IP 地址是 $3，查封ID是 #$5。 請在你的查詢中註明以上所有的資料。",
'blockednoreason'                  => '无给出原因',
'blockedoriginalsource'            => "以下是'''$1'''的源码:",
'blockededitsource'                => "你对'''$1'''进行'''编辑'''的文字如下:",
'whitelistedittitle'               => '登录后才可编辑',
'whitelistedittext'                => '您必须先$1才可编辑页面。',
'confirmedittitle'                 => '邮件确认后才可编辑',
'confirmedittext'                  => '在编辑此页之前您必须确认您的邮箱地址。请通过[[Special:Preferences|参数设置]]设置并验证您的邮箱地址。',
'nosuchsectiontitle'               => '没有这个段落',
'nosuchsectiontext'                => '您尝试编辑的段落并不存在。在这里是无第$1个段落，所以是没有一个地方去存贮你的编辑。',
'loginreqtitle'                    => '需要登录',
'loginreqlink'                     => '登录',
'loginreqpagetext'                 => '您必须$1才能查看其它页面。',
'accmailtitle'                     => '密码已寄出',
'accmailtext'                      => "'$1'的密码已经被发送到$2。",
'newarticle'                       => '(新)',
'newarticletext'                   => '您进入了一个尚未创建的页面。
要创建该页面，请在下面的编辑框中输入内容(详情参见[[Help:帮助|帮助]])。
如果您是不小心来到此页面，直接点击您浏览器中的"返回"按钮返回。',
'anontalkpagetext'                 => "---- ''这是一个还未建立账户的匿名用户的讨论页, 因此我们只能用IP地址来与他或她联络。该IP地址可能由几名用户共享。如果您是一名匿名用户并认为此页上的评语与您无关，请[[Special:UserLogin/signup|创建新账户]]或[[Special:UserLogin|登录]]以避免在未来与其他匿名用户混淆。''",
'noarticletext'                    => '此页目前没有内容，您可以在其它页[[Special:Search/{{PAGENAME}}|搜索此页标题]]或[{{fullurl:{{NAMESPACE}}:{{PAGENAME}}|action=edit}} 编辑此页]。',
'userpage-userdoesnotexist'        => '用户账户“$1”未曾创建。请在创建／编辑这个页面前先检查一下。',
'clearyourcache'                   => "'''注意 - 在保存以後, 您必須清除瀏覽器的緩存才能看到所作出的改變。'''
'''Mozilla / Firefox / Safari:''' 按著 ''Shift'' 再點擊''刷新''，或按下''Ctrl-F5''或''Ctrl-R''，(在Macintosh上按下''Command-R'')；
'''Konqueror:''' 只需點擊 ''刷新''或按下''F5''；
'''Opera:''' 在 ''工具→設定'' 中完整地清除它們的緩存；
'''Internet Explorer:''' 按著 ''Ctrl'' 再點擊 ''刷新''，或按下 ''Ctrl-F5''。",
'usercssjsyoucanpreview'           => "<strong>提示:</strong> 在保存前请用'显示预  '按钮来测试您新的 CSS/JS 。",
'usercsspreview'                   => "'''注意您只是在预览您的个人 CSS。'''
'''还没有保存！'''",
'userjspreview'                    => "'''注意您只是在测试／预览您的个人 JavaScript。'''
'''还没有保存！'''",
'userinvalidcssjstitle'            => "'''警告:''' 不存在皮肤\"\$1\"。注意自定义的 .css 和 .js 页要使用小写标题，例如，{{ns:user}}:Foo/monobook.css 不同于 {{ns:user}}:Foo/Monobook.css。",
'updated'                          => '(已更新)',
'note'                             => '<strong>注意:</strong>',
'previewnote'                      => '<strong>请记住这只是预览，内容还未保存！</strong>',
'previewconflict'                  => '这个预览显示了上面文字编辑区中的内容。它将在你选择保存后出现。',
'session_fail_preview'             => '<strong>抱歉！我们不能处理你在进程数据丢失时的编辑。请重试！如果再次失败，请[[Special:UserLogout|登出]]后重新登陆。</strong>',
'session_fail_preview_html'        => "<strong>抱歉！我们不能处理你在进程数据丢失时的编辑。</strong>

''由于{{SITENAME}}允许使用原始的 HTML，为了防范 JavaScript 攻击，预览已被隐藏。''

<strong>如果这是一次合法的编辑，请重新进行尝试。如果还不行，请[[Special:UserLogout|退出]]并重新登录。</strong>",
'token_suffix_mismatch'            => '<strong>由于您用户端中的编辑令牌毁损了一些标点符号字元，为防止编辑的文字损坏，您的编辑已经被拒绝。
这种情况通常出现于使用含有很多臭虫、以网络为主的匿名代理服务的时候。</strong>',
'editing'                          => '正在编辑$1',
'editingsection'                   => '正在编辑$1 (段落)',
'editingcomment'                   => '正在编辑$1 (评论)',
'editconflict'                     => '编辑冲突: $1',
'explainconflict'                  => '有人在你开始编辑后更改了页面。
上面的文字框内显示的是目前本页的内容。
你所做的修改显示在下面的文字框中。
你应当将你所做的修改加入现有的内容中。
<b>只有</b>在上面文字框中的内容会在你点击"保存页面"后被保存。<br />',
'yourtext'                         => '您的文字',
'storedversion'                    => '已保存版本',
'nonunicodebrowser'                => '<strong>警告: 您的浏览器不兼容Unicode编码。这里有一个工作区将使您能安全地编辑页面: 非ASCII字符将以十六进制编码方式出现在编辑框中。</strong>',
'editingold'                       => '<strong>警告：你正在编辑的是本页的旧版本。
如果你保存它的话，在本版本之后的任何修改都会丢失。</strong>',
'yourdiff'                         => '差异',
'copyrightwarning'                 => '请注意您对{{SITENAME}}的所有贡献都被认为是在$2下发布，请查看在$1的细节。
如果您不希望您的文字被任意修改和再散布，请不要提交。<br />
您同时也要向我们保证您所提交的内容是您自己所作，或得自一个不受版权保护或相似自由的来源。
<strong>不要在未获授权的情况下发表！</strong><br />',
'copyrightwarning2'                => '请注意您对{{SITENAME}}的所有贡献
都可能被其他贡献者编辑，修改或删除。
如果您不希望您的文字被任意修改和再散布，请不要提交。<br />
您同时也要向我们保证您所提交的内容是您自己所作，或得自一个不受版权保护或相似自由的来源（参阅$1的细节）。
<strong>不要在未获授权的情况下发表！</strong>',
'longpagewarning'                  => '<strong>警告: 该页面的长度是$1KB；一些浏览器在编辑长度接近或大于32KB的页面可能存在问题。
您应该考虑将此页面分成更小的章节。</strong>',
'longpageerror'                    => '<strong>错误: 您所提交的文本长度有$1KB，这大于$2KB的最大值。该文本不能被保存。</strong>',
'readonlywarning'                  => '<strong>警告: 数据库被锁以进行维护，所以您目前将无法保存您的修改。您或许希望先将本段文字复制并保存到文本文件，然后等一会儿再修改。</strong>',
'protectedpagewarning'             => '<strong>警告: 此页已经被保护，只有拥有管理员权限的用户才可修改。</strong>',
'semiprotectedpagewarning'         => "'''注意：''' 本页面被锁定，仅限注册用户编辑。",
'cascadeprotectedwarning'          => '警告: 本页已经被保护，只有拥有管理员权限的用户才可修改，因为本页已被以下连锁保护的{{PLURAL:$1|一个|多个}}页面所包含:',
'titleprotectedwarning'            => '<strong>警告: 本页面已被锁上，只有一些用户方可创建。</strong>',
'templatesused'                    => '在这个页面上使用的模板有:',
'templatesusedpreview'             => '此次预览中使用的模板有:',
'templatesusedsection'             => '在这个段落上使用的模板有:',
'template-protected'               => '(保护)',
'template-semiprotected'           => '(半保护)',
'hiddencategories'                 => '这个页面是属于$1个隐藏分类的成员:',
'edittools'                        => '<!-- 此处的文本将被显示在以下编辑和上传表单中。 -->',
'nocreatetitle'                    => '创建页面受限',
'nocreatetext'                     => '{{SITENAME}}限制了创建新页面的功能。你可以返回并编辑已有的页面，或者[[Special:UserLogin|登录或创建新账户]]。',
'nocreate-loggedin'                => '您在{{SITENAME}}中并无权限去创建新页面。',
'permissionserrors'                => '权限错误',
'permissionserrorstext'            => '根据以下的{{PLURAL:$1|原因|原因}}，您并无权限去做以下的动作:',
'permissionserrorstext-withaction' => '根据以下的{{PLURAL:$1|原因|原因}}，您并无权限去做$2:',
'recreate-deleted-warn'            => "'''警告: 你现在重新创建一个先前曾经删除过的页面。'''

你应该要考虑一下继续编辑这一个页面是否合适。
为方便起见，这一个页面的删除记录已经在下面提供:",

# Parser/template warnings
'expensive-parserfunction-warning'        => '警告: 这个页面有太多耗费的语法功能呼叫。

它应该少过$2，现在有$1。',
'expensive-parserfunction-category'       => '页面中有太多耗费的语法功能呼叫',
'post-expand-template-inclusion-warning'  => '警告: 包含模板大小过大。
一些模板将不会包含。',
'post-expand-template-inclusion-category' => '模板包含上限已经超过的页面',
'post-expand-template-argument-warning'   => '警告: 这个页面有最少一个模参数有过大扩展大小。
这些参数会被略过。',
'post-expand-template-argument-category'  => '包含着略过模板参数的页面',

# "Undo" feature
'undo-success' => '此编辑可以被撤销。请检查以下对比以核实这正是您想做的，然后保存以下更改以完成撤销编辑。',
'undo-failure' => '由于中途不一致的编辑，此编辑不能撤销。',
'undo-norev'   => '由于其修订版本不存在或已删除，此编辑不能撤销。',
'undo-summary' => '取消由[[Special:Contributions/$2|$2]] ([[User talk:$2|对话]])所作出的修订 $1',

# Account creation failure
'cantcreateaccounttitle' => '无法创建账户',
'cantcreateaccount-text' => "从这个IP地址 (<b>$1</b>) 创建账户已经被[[User:$3|$3]]禁止。

当中被$3封禁的原因是''$2''",

# History pages
'viewpagelogs'        => '查看此页面的日志',
'nohistory'           => '此页没有修订记录。',
'revnotfound'         => '没有找到修订记录',
'revnotfoundtext'     => '您请求的更早版本的修订记录没有找到。
请检查您请求本页面用的 URL 是否正确。',
'currentrev'          => '当前修订版本',
'revisionasof'        => '在$1所做的修订版本',
'revision-info'       => '在$1由$2所做的修订版本',
'previousrevision'    => '←上一修订',
'nextrevision'        => '下一修订→',
'currentrevisionlink' => '当前修订',
'cur'                 => '当前',
'next'                => '后继',
'last'                => '先前',
'page_first'          => '最前',
'page_last'           => '最后',
'histlegend'          => '差异选择: 标记要比较版本的单选按钮并点击底部的按钮进行比较。<br />
说明: (当前) 指与当前版本比较，(先前) 指与前一个修订版本比较，小 = 小修改。',
'deletedrev'          => '[已删除]',
'histfirst'           => '最早版本',
'histlast'            => '最新版本',
'historysize'         => '($1 字节)',
'historyempty'        => '(空)',

# Revision feed
'history-feed-title'          => '修订历史',
'history-feed-description'    => '本站上此页的修订历史',
'history-feed-item-nocomment' => '$1在$2', # user at time
'history-feed-empty'          => '所请求的页面不存在。它可能已被删除或重命名。
尝试[[Special:Search|搜索本站]]获得相关的新建页面。',

# Revision deletion
'rev-deleted-comment'         => '(注释已移除)',
'rev-deleted-user'            => '(用户名已移除)',
'rev-deleted-event'           => '(日志动作已移除)',
'rev-deleted-text-permission' => '<div class="mw-warning plainlinks">该页面修订已经被从公共文档中移除。
在[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 删除日志]中您可能会查看到详细的信息。</div>',
'rev-deleted-text-view'       => "<div class='mw-warning plainlinks'>
该页面修订已经被从公共文档中移除。作为{{SITENAME}}的管理员，您可以查看它；
在[{{fullurl:Special:Log/delete|page={{PAGENAMEE}}}} 删除日志]中您可能会查看到详细的信息。
</div>",
'rev-delundel'                => '显示/隐藏',
'revisiondelete'              => '删除/恢复删除修订',
'revdelete-nooldid-title'     => '无效的目标修订',
'revdelete-nooldid-text'      => '您尚未指定一个目标修订去进行这个功能、
所指定的修订不存在，或者您尝试去隐藏现时的修订。',
'revdelete-selected'          => "选取'''$1'''的$2次修订:",
'logdelete-selected'          => "选取'''$1'''的日志项目:",
'revdelete-text'              => '删除的修订仍将显示在页面历史中, 但它们的文本内容已不能被公众访问。

在{{SITENAME}}的其他管理员将仍能访问隐藏的内容并通过与此相同的界面恢复删除，除非站点工作者进行了一些附加的限制。',
'revdelete-legend'            => '设置可见性之限制',
'revdelete-hide-text'         => '隐藏修订文本',
'revdelete-hide-name'         => '隐藏动作和目标',
'revdelete-hide-comment'      => '隐藏编辑说明',
'revdelete-hide-user'         => '隐藏编辑者的用户名/IP',
'revdelete-hide-restricted'   => '将此限制同样应用于管理员同时锁定这个界面',
'revdelete-suppress'          => '同时废止由操作员以及其他用户的资料',
'revdelete-hide-image'        => '隐藏文件内容',
'revdelete-unsuppress'        => '在已恢复的修订中移除限制',
'revdelete-log'               => '日志注释:',
'revdelete-submit'            => '应用于选中的修订',
'revdelete-logentry'          => '[[$1]]的修订可见性已更改',
'logdelete-logentry'          => '[[$1]]的事件可见性已更改',
'revdelete-success'           => "'''修订的可见性已经成功设置。'''",
'logdelete-success'           => "'''事件的可见性已经成功设置。'''",
'revdel-restore'              => '更改可见性',
'pagehist'                    => '页面历史',
'deletedhist'                 => '已删除之历史',
'revdelete-content'           => '内容',
'revdelete-summary'           => '编辑摘要',
'revdelete-uname'             => '用户名',
'revdelete-restricted'        => '已应用限制至操作员',
'revdelete-unrestricted'      => '已移除对于操作员的限制',
'revdelete-hid'               => '隐藏 $1',
'revdelete-unhid'             => '不隐藏 $1',
'revdelete-log-message'       => '$1的$2次修订',
'logdelete-log-message'       => '$1的$2项事件',

# Suppression log
'suppressionlog'     => '废止日志',
'suppressionlogtext' => '以下是删除以及由操作员牵涉到内容封锁的列表。
参看[[Special:IPBlockList|IP封锁名单]]去参看现时进行中的禁止以及封锁之名单。',

# History merging
'mergehistory'                     => '合并页面历史',
'mergehistory-header'              => '这一页可以讲您合并一个来源页面的历史到另一个新页面中。
请确认这次更改会继续保留该页面先前的历史版本。',
'mergehistory-box'                 => '合并两个页面的修订:',
'mergehistory-from'                => '来源页面:',
'mergehistory-into'                => '目的页面:',
'mergehistory-list'                => '可以合并的编辑历史',
'mergehistory-merge'               => '以下[[:$1]]的修订可以合并到[[:$2]]。用该选项按钮列去合并只有在指定时间以前所创建的修订。要留意的是使用导航链接便会重设这一栏。',
'mergehistory-go'                  => '显示可以合并的编辑',
'mergehistory-submit'              => '合并修订',
'mergehistory-empty'               => '没有修订可以合并',
'mergehistory-success'             => '[[:$1]]的$3次修订已经成功地合并到[[:$2]]。',
'mergehistory-fail'                => '不可以进行历史合并，请重新检查该页面以及时间参数。',
'mergehistory-no-source'           => '来源页面$1不存在。',
'mergehistory-no-destination'      => '目的页面$1不存在。',
'mergehistory-invalid-source'      => '来源页面必须是一个有效的标题。',
'mergehistory-invalid-destination' => '目的页面必须是一个有效的标题。',
'mergehistory-autocomment'         => '已经合并[[:$1]]去到[[:$2]]',
'mergehistory-comment'             => '已经合并[[:$1]]去到[[:$2]]: $3',

# Merge log
'mergelog'           => '合并日志',
'pagemerge-logentry' => '已合并[[$1]]到[[$2]] (修订截至$3)',
'revertmerge'        => '解除合并',
'mergelogpagetext'   => '以下是一个最近由一个页面的修订历史合并到另一个页面的列表。',

# Diffs
'history-title'           => '“$1”的修订历史',
'difference'              => '(修订版本间差异)',
'lineno'                  => '第$1行:',
'compareselectedversions' => '比较选定的版本',
'editundo'                => '撤销',
'diff-multi'              => '($1个中途的修订版本没有显示。)',

# Search results
'searchresults'             => '搜索结果',
'searchresulttext'          => '有关搜索{{SITENAME}}的更多详情,参见[[{{MediaWiki:Helppage}}|{{int:help}}]]。',
'searchsubtitle'            => '搜索\'\'\'[[:$1]]\'\'\'([[Special:Prefixindex/$1|所有以 "$1" 开头的页面]] | [[Special:WhatLinksHere/$1|所有链接到 "$1" 的页面]])',
'searchsubtitleinvalid'     => "搜索'''$1'''",
'noexactmatch'              => "'''没找到标题为\"\$1\"的页面。''' 您可以[[:\$1|创建此页面]]。",
'noexactmatch-nocreate'     => "'''没找到标题为\"\$1\"的页面。'''",
'toomanymatches'            => '过多的匹配已反应，请尝试一个不同的查询',
'titlematches'              => '页面题目相符',
'notitlematches'            => '没有找到匹配页面题目',
'textmatches'               => '页面内容相符',
'notextmatches'             => '没有页面内容匹配',
'prevn'                     => '前$1个',
'nextn'                     => '后$1个',
'viewprevnext'              => '查看 ($1) ($2) ($3)',
'search-result-size'        => '$1 ($2个字)',
'search-result-score'       => '相关度: $1%',
'search-redirect'           => '(重定向 $1)',
'search-section'            => '(段落 $1)',
'search-suggest'            => '你是否解: $1',
'search-interwiki-caption'  => '姊妹计划',
'search-interwiki-default'  => '$1项结果:',
'search-interwiki-more'     => '(更多)',
'search-mwsuggest-enabled'  => '有建议',
'search-mwsuggest-disabled' => '无建议',
'search-relatedarticle'     => '相关',
'mwsuggest-disable'         => '禁用AJAX建议',
'searchrelated'             => '相关',
'searchall'                 => '所有',
'showingresults'            => '下面显示从第<b>$2</b>条开始的<b>$1</b>条结果:',
'showingresultsnum'         => '下面显示从第<b>$2</b>条开始的<b>$3</b>条结果:',
'showingresultstotal'       => "下面显示从'''$1 - $2'''，总共'''$3'''项之结果",
'nonefound'                 => "'''注意''': 只有一些名字空间是会作为预设搜寻。尝试''all:''去搜寻全部的页面（包埋讨论页面、模板等），或可用需要的名字空间作为前缀。",
'powersearch'               => '高级搜索',
'powersearch-legend'        => '高级搜索',
'powersearch-ns'            => '在以下的名字空间中搜索:',
'powersearch-redir'         => '重定向列表',
'powersearch-field'         => '搜索',
'search-external'           => '外部搜索',
'searchdisabled'            => '{{SITENAME}}由于性能方面的原因，全文搜索已被暂时禁用。您可以暂时通过Google搜索。请留意他们的索引可能会过时。',

# Preferences page
'preferences'              => '参数设置',
'mypreferences'            => '我的参数设置',
'prefs-edits'              => '编辑数量:',
'prefsnologin'             => '尚未登录',
'prefsnologintext'         => '您必须先<span class="plainlinks">[{{fullurl:Special:Userlogin|returnto=$1}} 登录]</span>才能设置个人参数。',
'prefsreset'               => '参数已被重新设置。',
'qbsettings'               => '快速导航条',
'qbsettings-none'          => '无',
'qbsettings-fixedleft'     => '左侧固定',
'qbsettings-fixedright'    => '右侧固定',
'qbsettings-floatingleft'  => '左侧漂移',
'qbsettings-floatingright' => '右侧漂移',
'changepassword'           => '更改密码',
'skin'                     => '皮肤',
'math'                     => '数学公式',
'dateformat'               => '日期格式',
'datedefault'              => '默认值',
'datetime'                 => '日期和时间',
'math_failure'             => '解析失败',
'math_unknown_error'       => '未知错误',
'math_unknown_function'    => '未知函数',
'math_lexing_error'        => '句法错误',
'math_syntax_error'        => '语法错误',
'math_image_error'         => 'PNG 转换失败；请检查是否正确安装了 latex, dvips, gs 和 convert',
'math_bad_tmpdir'          => '无法写入或建立数学公式临时目录',
'math_bad_output'          => '无法写入或建立数学公式输出目录',
'math_notexvc'             => '无法执行"texvc"；请参照 math/README 进行配置。',
'prefs-personal'           => '用户资料',
'prefs-rc'                 => '最近更改',
'prefs-watchlist'          => '监视列表',
'prefs-watchlist-days'     => '监视列表中显示记录的天数:',
'prefs-watchlist-edits'    => '在增强的监视列表中显示最多更改次数:',
'prefs-misc'               => '杂项',
'saveprefs'                => '保存参数设置',
'resetprefs'               => '重设参数',
'oldpassword'              => '旧密码',
'newpassword'              => '新密码',
'retypenew'                => '确认密码:',
'textboxsize'              => '编辑',
'rows'                     => '行:',
'columns'                  => '列:',
'searchresultshead'        => '搜索结果设定',
'resultsperpage'           => '每页显示链接数',
'contextlines'             => '每链显示行数:',
'contextchars'             => '每行显示字数:',
'stub-threshold'           => '<a href="#" class="stub">短页面链接</a>格式门槛值 (字节):',
'recentchangesdays'        => '最近更改中的显示日数:',
'recentchangescount'       => '最近更改、历史及日志页面中的编辑数:',
'savedprefs'               => '您的个人参数设置已经保存。',
'timezonelegend'           => '时区',
'timezonetext'             => '¹输入当地时间与服务器时间(UTC)的时差。',
'localtime'                => '当地时间',
'timezoneoffset'           => '时差¹',
'servertime'               => '服务器时间',
'guesstimezone'            => '从浏览器填写',
'allowemail'               => '接受来自其他用户的邮件',
'prefs-searchoptions'      => '搜索选项',
'prefs-namespaces'         => '名字空间',
'defaultns'                => '默认搜索的名字空间',
'default'                  => '默认',
'files'                    => '文件',

# User rights
'userrights'                  => '用户权限管理', # Not used as normal message but as header for the special page itself
'userrights-lookup-user'      => '管理用户群组',
'userrights-user-editname'    => '输入用户名:',
'editusergroup'               => '编辑用户群组',
'editinguser'                 => "正在更改用户'''[[User:$1|$1]]''' ([[User talk:$1|{{int:talkpagelinktext}}]] | [[Special:Contributions/$1|{{int:contribslink}}]]) 的用户权限",
'userrights-editusergroup'    => '编辑用户群组',
'saveusergroups'              => '存储用户群组',
'userrights-groupsmember'     => '隶属于:',
'userrights-groups-help'      => '您可以改动这位用户所属的组群:
* 已剔选的核取方块代表该用户属于该组群。
* 未剔选的核取方块代表该用户不是属于该组群。
* 一个 * 表示当您加入群组之后更不能移除它，反之亦然。',
'userrights-reason'           => '更改原因:',
'userrights-no-interwiki'     => '您并没有权限去编辑在其它wiki上的用户权限。',
'userrights-nodatabase'       => '数据库$1不存在或并非为本地的。',
'userrights-nologin'          => '您必须要以操作员帐户[[Special:UserLogin|登入]]之后才可以指定用户权限。',
'userrights-notallowed'       => '您的帐户无权限去指定用户权限。',
'userrights-changeable-col'   => '您可以更改的组群',
'userrights-unchangeable-col' => '您不可以更改的组群',

# Groups
'group'               => '群组:',
'group-user'          => '用户',
'group-autoconfirmed' => '自动确认用户',
'group-bot'           => '机器人',
'group-sysop'         => '操作员',
'group-bureaucrat'    => '行政员',
'group-suppress'      => '监督',
'group-all'           => '(全部)',

'group-user-member'          => '用户',
'group-autoconfirmed-member' => '自动确认用户',
'group-bot-member'           => '机器人',
'group-sysop-member'         => '操作员',
'group-bureaucrat-member'    => '行政员',
'group-suppress-member'      => '监督',

'grouppage-user'          => '{{ns:project}}:用户',
'grouppage-autoconfirmed' => '{{ns:project}}:自动确认用户',
'grouppage-bot'           => '{{ns:project}}:机器人',
'grouppage-sysop'         => '{{ns:project}}:操作员',
'grouppage-bureaucrat'    => '{{ns:project}}:行政员',
'grouppage-suppress'      => '{{ns:project}}:監督',

# Rights
'right-read'                 => '阅读页面',
'right-edit'                 => '编辑页面',
'right-createpage'           => '建立页面（不含讨论页面）',
'right-createtalk'           => '建立讨论页面',
'right-createaccount'        => '创建新用户账户',
'right-minoredit'            => '标示作小编辑',
'right-move'                 => '移动页面',
'right-move-subpages'        => '移动页面跟它的字页面',
'right-suppressredirect'     => '移动页面时不建立重定向',
'right-upload'               => '上载文件',
'right-reupload'             => '覆盖现有的文件',
'right-reupload-own'         => '覆盖由同一位上载的文件',
'right-reupload-shared'      => '于本地无视共用媒体文件库上的文件',
'right-upload_by_url'        => '由URL地址上载一个文件',
'right-purge'                => '不需要确认之下清除网站快取',
'right-autoconfirmed'        => '编辑半保护页面',
'right-bot'                  => '视为一个自动程序',
'right-nominornewtalk'       => '小编辑不引发新信息提示',
'right-apihighlimits'        => '在API查询中使用更高的上限',
'right-writeapi'             => '使用编写的API',
'right-delete'               => '删除页面',
'right-bigdelete'            => '删除大量历史之页面',
'right-deleterevision'       => '删除及同反删除页面中的指定修订',
'right-deletedhistory'       => '查看已删除之项目，不含有关的字',
'right-browsearchive'        => '搜寻已删除之页面',
'right-undelete'             => '反删除页面',
'right-suppressrevision'     => '查看及恢复由操作员隐藏之修订',
'right-suppressionlog'       => '查看私人的日志',
'right-block'                => '封锁其他用户防止编辑',
'right-blockemail'           => '封锁用户不可发电邮',
'right-hideuser'             => '封锁用户名，对公众隐藏',
'right-ipblock-exempt'       => '绕过IP封锁、自动封锁以及范围封锁',
'right-proxyunbannable'      => '绕过Proxy的自动封锁',
'right-protect'              => '更改保护等级以及埋编辑保护页面',
'right-editprotected'        => '编辑保护页面（无连锁保护）',
'right-editinterface'        => '编辑用户接口',
'right-editusercssjs'        => '编辑其他用户的CSS和JS文件',
'right-rollback'             => '快速复原上位用户对某一页面之编辑',
'right-markbotedits'         => '标示复原编辑作机械人编辑',
'right-noratelimit'          => '没有使用频率限制',
'right-import'               => '由其它wiki中导入页面',
'right-importupload'         => '由文件上载中导入页面',
'right-patrol'               => '标示其它编辑作已巡查的',
'right-autopatrol'           => '将自己的编辑自动标示为已巡查的',
'right-patrolmarks'          => '查看最近巡查标记更改',
'right-unwatchedpages'       => '查看未监视之页面',
'right-trackback'            => '递交一个trackback',
'right-mergehistory'         => '合并页面历史',
'right-userrights'           => '编辑所有用户的权限',
'right-userrights-interwiki' => '编辑在其它wiki上的用户权限',
'right-siteadmin'            => '锁定和解除锁定数据库',

# User rights log
'rightslog'      => '用户权限日志',
'rightslogtext'  => '以下记录了用户权限的更改记录。',
'rightslogentry' => '将 $1 的权限从 $2 改为 $3',
'rightsnone'     => '(无)',

# Recent changes
'nchanges'                          => '$1次更改',
'recentchanges'                     => '最近更改',
'recentchangestext'                 => '跟踪这个wiki上的最新更改。',
'recentchanges-feed-description'    => '跟踪此订阅在 wiki 上的最近更改。',
'rcnote'                            => "以下是在$4 $5，最近'''$2'''天内的'''$1'''次最近更改记录:",
'rcnotefrom'                        => '以下是自<b>$2</b>的更改(最多显示<b>$1</b>):',
'rclistfrom'                        => '显示自$1以来的新更改',
'rcshowhideminor'                   => '$1小编辑',
'rcshowhidebots'                    => '$1机器人的编辑',
'rcshowhideliu'                     => '$1登录用户的编辑',
'rcshowhideanons'                   => '$1匿名用户的编辑',
'rcshowhidepatr'                    => ' $1检查过的编辑',
'rcshowhidemine'                    => '$1我的编辑',
'rclinks'                           => '显示最近$2天内最新的$1次改动。<br />$3',
'diff'                              => '差异',
'hist'                              => '历史',
'hide'                              => '隐藏',
'show'                              => '显示',
'minoreditletter'                   => '小',
'newpageletter'                     => '新',
'boteditletter'                     => '机',
'number_of_watching_users_pageview' => '[$1个关注用户]',
'rc_categories'                     => '分类界限(以"|"分割)',
'rc_categories_any'                 => '任意',
'newsectionsummary'                 => '/* $1 */ 新段落',

# Recent changes linked
'recentchangeslinked'          => '链出更改',
'recentchangeslinked-title'    => '对于“$1”有关的链出更改',
'recentchangeslinked-noresult' => '在这一段时间中连结的页面并无更改。',
'recentchangeslinked-summary'  => "这一个特殊页面列示''由''所给出的一个页面之链接到页面的最近更改（或者是对于指定分类的成员）。
在[[Special:Watchlist|您的监视列表]]中的页面会以'''粗体'''显示。",
'recentchangeslinked-page'     => '页面名称:',
'recentchangeslinked-to'       => '显示链到所给出的页面',

# Upload
'upload'                      => '上传文件',
'uploadbtn'                   => '上传文件',
'reupload'                    => '重新上传',
'reuploaddesc'                => '取消上传并返回上传表单',
'uploadnologin'               => '未登录',
'uploadnologintext'           => '您必须先[[Special:UserLogin|登录]]才能上传文件。',
'upload_directory_missing'    => '上传目录($1)遗失，不能由网页服务器建立。',
'upload_directory_read_only'  => '上传目录($1)不存在或无写权限。',
'uploaderror'                 => '上载错误',
'uploadtext'                  => "使用下面的表单来上传用在页面内新的文件。
要查看或搜索以前上传的文件
可以进入[[Special:ImageList|文件上传列表]]，
（重新）上传将在[[Special:Log/upload|上传日志]]中记录，
而删除将在[[Special:Log/delete|删除日志]]中记录。

要在页面中加入文件，使用以下形式的连接:
'''<nowiki>[[</nowiki>{{ns:image}}</nowiki>:file.jpg]]</nowiki>'''，
'''<nowiki>[[</nowiki>{{ns:image}}</nowiki>:file.png|替换文字]]</nowiki>''' 或
'''<nowiki>[[</nowiki>{{ns:media}}</nowiki>:file.ogg]]</nowiki>'''。",
'upload-permitted'            => '准许的文件类型: $1。',
'upload-preferred'            => '建议的文件类型: $1。',
'upload-prohibited'           => '禁止的文件类型: $1。',
'uploadlog'                   => '上传日志',
'uploadlogpage'               => '上传日志',
'uploadlogpagetext'           => '以下是一个最近上传文件的列表。
查看[[Special:NewImages|新文件画廊]]去看更富图像的总览。',
'filename'                    => '文件名',
'filedesc'                    => '文件描述',
'fileuploadsummary'           => '文件描述:',
'filestatus'                  => '版权状态:',
'filesource'                  => '来源:',
'uploadedfiles'               => '已上传文件',
'ignorewarning'               => '忽略警告并保存文件',
'ignorewarnings'              => '忽略所有警告',
'minlength1'                  => '文件名字必须至少有一个字母。',
'illegalfilename'             => '文件名「$1」包含有页面标题所禁止的字符。请改名后重新上传。',
'badfilename'                 => '文件名已被改为「$1」。',
'filetype-badmime'            => 'MIME类别"$1"不是容许的文件格式。',
'filetype-unwanted-type'      => "'''\".\$1\"'''是一种不需要的文件类型。
建议的{{PLURAL:\$3|一种|多种}}文件类型有\$2。",
'filetype-banned-type'        => "'''\".\$1\"'''是一种不准许的文件类型。
容许的{{PLURAL:\$3|一种|多种}}文件类型有\$2。",
'filetype-missing'            => '该文件名称并没有副档名 (像 ".jpg")。',
'large-file'                  => '建议文件大小不能超过 $1；本文件大小为 $2。',
'largefileserver'             => '这个文件的大小比服务器配置允许的大小还要大。',
'emptyfile'                   => '您所上传的文件不存在。这可能是由于文件名键入错误。请检查您是否真的要上传此文件。',
'fileexists'                  => '已存在相同名称的文件，如果您无法确定您是否要改变它，请检查<strong><tt>$1</tt></strong>。',
'filepageexists'              => '这个文件的描述页已经在<strong><tt>$1</tt></strong>创建，但是这个名称的文件尚未存在。您输入了的摘要是不会显示在该描述页中。要令该摘要在该处中出现，您便要手动地去编辑它。',
'fileexists-extension'        => '一个相似名称的文件已经存在:<br />
上载文件的档名: <strong><tt>$1</tt></strong><br />
现有文件的档名: <strong><tt>$2</tt></strong><br />
请选择一个不同的名字。',
'fileexists-thumb'            => "<center>'''已经存在的文件'''</center>",
'fileexists-thumbnail-yes'    => '这个文件好像是一幅图像的缩图版本<i>(缩图)</i>。请检查清楚该文件<strong><tt>$1</tt></strong>。<br />
如果检查后的文件是同原本图像的大小是一样的话，就不用再上载多一幅缩图。',
'file-thumbnail-no'           => '该档名是以<strong><tt>$1</tt></strong>开始。它好像一幅图像的缩图版本<i>(缩图)</i>。
如果你有该图像的完整大小，如不是请再修改文件名。',
'fileexists-forbidden'        => '已存在相同名称的文件；请返回并用一个新的名称来上传此文件。[[Image:$1|thumb|center|$1]]',
'fileexists-shared-forbidden' => '在共享文件库中已存在此名称的文件；请返回并用一个新的名称来上传此文件。[[Image:$1|thumb|center|$1]]',
'file-exists-duplicate'       => '这个文件是跟以下的{{PLURAL:$1|一|多}}个文件重覆:',
'successfulupload'            => '上传成功',
'uploadwarning'               => '上载警告',
'savefile'                    => '保存文件',
'uploadedimage'               => '已上载"[[$1]]"',
'overwroteimage'              => '已经上载"[[$1]]"的新版本',
'uploaddisabled'              => '无法上传',
'uploaddisabledtext'          => '文件上传在{{SITENAME}}不可用。',
'uploadscripted'              => '该文件包含可能被网络浏览器错误解释的 HTML 或脚本代码。',
'uploadcorrupt'               => '该文件包含或具有一个不正确的扩展名。请检查此文件并重新上传。',
'uploadvirus'                 => '该文件包含有病毒！详情: $1',
'sourcefilename'              => '源文件名:',
'destfilename'                => '目标文件名:',
'upload-maxfilesize'          => '文件最大限制大小: $1',
'watchthisupload'             => '监视此页',
'filewasdeleted'              => '之前已经有一个同名文件被上传后又被删除了。在上传此文件之前您需要检查$1。',
'upload-wasdeleted'           => "'''警告: 您现在重新上传一个先前曾经删除过的文件。'''

您应该要考虑一下继续上传一个文件页面是否合适。
为方便起见，这一个文件的删除记录已经在下面提供:",
'filename-bad-prefix'         => '您上传的文件名称是以<strong>“$1”</strong>作为开头，通常这种没有含意的文件名称是由数码相机中自动编排。请在您的文件中重新选择一个更加有意义的文件名称。',

'upload-proto-error'      => '协议错误',
'upload-proto-error-text' => '远程上传要求 URL 以 <code>http://</code> 或 <code>ftp://</code> 开头。',
'upload-file-error'       => '内部错误',
'upload-file-error-text'  => '当试图在服务器上创建临时文件时发生内部错误。请与[[Special:ListUsers/sysop|管理员]]联系。',
'upload-misc-error'       => '未知的上传错误',
'upload-misc-error-text'  => '在上传时发生未知的错误. 请验证使用了正确并可访问的 URL，然后进行重试。如果问题仍然存在，请与[[Special:ListUsers/sysop|管理员]]联系。',

# Some likely curl errors. More could be added from <http://curl.haxx.se/libcurl/c/libcurl-errors.html>
'upload-curl-error6'       => '无法访问 URL',
'upload-curl-error6-text'  => '无法访问所提供的 URL。请再次检查该 URL 是否正确，并且网站的访问是否正常。',
'upload-curl-error28'      => '上传超时',
'upload-curl-error28-text' => '站点响应时间过长。请检查此网站的访问是否正常，过一会再进行尝试。您可能需要在网络访问空闲时间再次进行尝试。',

'license'            => '授权:',
'nolicense'          => '未选定',
'license-nopreview'  => '(无预览可用)',
'upload_source_url'  => ' (一个有效的，可公开访问的 URL)',
'upload_source_file' => ' (在您计算机上的一个文件)',

# Special:ImageList
'imagelist-summary'     => '这个特殊页面显示所有上传过的文件。
预设中最后上传的文件会显示在这个列表中的最顶处。
点击一栏的标题去改变这个排列。',
'imagelist_search_for'  => '按媒体名称搜索:',
'imgfile'               => '文件',
'imagelist'             => '文件列表',
'imagelist_date'        => '日期',
'imagelist_name'        => '名称',
'imagelist_user'        => '用户',
'imagelist_size'        => '大小',
'imagelist_description' => '描述',

# Image description page
'filehist'                       => '文件历史',
'filehist-help'                  => '点击日期／时间以查看当时出现过的文件。',
'filehist-deleteall'             => '删除全部',
'filehist-deleteone'             => '删除',
'filehist-revert'                => '恢复',
'filehist-current'               => '当前',
'filehist-datetime'              => '日期／时间',
'filehist-user'                  => '用户',
'filehist-dimensions'            => '维度',
'filehist-filesize'              => '文件大小',
'filehist-comment'               => '注解',
'imagelinks'                     => '链接',
'linkstoimage'                   => '以下的$1个页面链接到本文件:',
'nolinkstoimage'                 => '没有页面链接到本文件。',
'morelinkstoimage'               => '查看连接到这个文件的[[Special:WhatLinksHere/$1|更多连结]]。',
'redirectstofile'                => '以下的$1个文件重新定向到这个文件:',
'duplicatesoffile'               => '以下的$1个文件是跟这个文件重覆:',
'sharedupload'                   => '该文件是一个共享上传，它可能在其它项目中被应用。',
'shareduploadwiki'               => '请参阅在共享文件库上的$1以了解其相关信息。',
'shareduploadwiki-desc'          => '它在$1上的描述于下面显示。',
'shareduploadwiki-linktext'      => '文件描述页面',
'shareduploadduplicate'          => '这个文件是跟共用文件库的$1重覆。',
'shareduploadduplicate-linktext' => '另一个文件',
'shareduploadconflict'           => '这个文件在共用文件库中的$1有同样的文件名。',
'shareduploadconflict-linktext'  => '另一个文件',
'noimage'                        => '不存在此名称的文件，您可以$1。',
'noimage-linktext'               => '上传它',
'uploadnewversion-linktext'      => '上传该文件的新版本',
'imagepage-searchdupe'           => '搜索重覆的文件',

# File reversion
'filerevert'                => '恢复$1',
'filerevert-legend'         => '恢复文件',
'filerevert-intro'          => "您现正在恢复'''[[Media:$1|$1]]'''到[$4 于$2 $3的版本]。",
'filerevert-comment'        => '注解:',
'filerevert-defaultcomment' => '已经恢复到于$1 $2的版本',
'filerevert-submit'         => '恢复',
'filerevert-success'        => "'''[[Media:$1|$1]]'''已经恢复到[$4 于$2 $3的版本]。",
'filerevert-badversion'     => '这个文件所提供的时间截记并无先前的本地版本。',

# File deletion
'filedelete'                  => '删除$1',
'filedelete-legend'           => '删除文件',
'filedelete-intro'            => "您现正删除'''[[Media:$1|$1]]'''。",
'filedelete-intro-old'        => "你现正删除'''[[Media:$1|$1]]'''于[$4 $2 $3]的版本。",
'filedelete-comment'          => '删除理由:',
'filedelete-submit'           => '删除',
'filedelete-success'          => "'''$1'''已经删除。",
'filedelete-success-old'      => "'''[[Media:$1|$1]]'''于 $2 $3 的版本已经删除。",
'filedelete-nofile'           => "'''$1'''在{{SITENAME}}中不存在。",
'filedelete-nofile-old'       => "在已指定属性的情况下，这里没有'''$1'''于 $2 $3 的版本。",
'filedelete-iscurrent'        => '您现正尝试删除这个文件的最近版本。请先将个该文件回退成较旧的版本。',
'filedelete-otherreason'      => '其它／附加的理由:',
'filedelete-reason-otherlist' => '其它理由',
'filedelete-reason-dropdown'  => '
*常用删除理由
** 侵犯版权
** 重覆文件',
'filedelete-edit-reasonlist'  => '编辑删除埋由',

# MIME search
'mimesearch'         => 'MIME 搜索',
'mimesearch-summary' => '本页面启用文件MIME类型过滤器。输入：内容类型/子类型，如 <tt>image/jpeg</tt>。',
'mimetype'           => 'MIME 类型:',
'download'           => '下载',

# Unwatched pages
'unwatchedpages' => '未被监视的页面',

# List redirects
'listredirects' => '重定向页面列表',

# Unused templates
'unusedtemplates'     => '未使用的模板',
'unusedtemplatestext' => '此页面列出模板名字空间下所有未被其它页面使用的页面。请在删除这些模板前检查其它链入该模板的页面。',
'unusedtemplateswlh'  => '其它链接',

# Random page
'randompage'         => '随机页面',
'randompage-nopages' => '在这个名字空间中没有页面。',

# Random redirect
'randomredirect'         => '随机重定向页面',
'randomredirect-nopages' => '在这个名字空间中没有重定向页面。',

# Statistics
'statistics'             => '统计',
'sitestats'              => '{{SITENAME}}统计数据',
'userstats'              => '用户统计',
'sitestatstext'          => "数据库中共有'''\$1'''页页面。
其中包括对话页、关于{{SITENAME}}的页面、最少量的\"小作品\"页、重定向的页面，
以及未达到页面质量的页面。除此之外还有'''\$2'''页可能是合乎标准的页面。

'''\$8'''个文件已被上传。

从{{SITENAME}}设置以来，全站点共有页面浏览'''\$3'''次，页面编辑'''\$4'''次。
即每页平均编辑'''\$5'''次，各次编辑后页面的每个版本平均浏览'''\$6'''次。

[http://www.mediawiki.org/wiki/Manual:Job_queue 工作排队]的长度是'''\$7'''。",
'userstatstext'          => "网站有'''$1'''位注册[[Special:ListUsers|用户]]，其中
'''$2''' (或 '''$4%''') 有$5权限。",
'statistics-mostpopular' => '浏览最多的页面',

'disambiguations'      => '消含糊页',
'disambiguationspage'  => 'Template:disambig
Template:消含糊
Template:消除含糊
Template:消歧义
Template:消除歧义
Template:消歧義
Template:消除歧義',
'disambiguations-text' => '以下的页面都有到<b>消含糊页</b>的链接, 但它们应该是链到适当的标题。<br />一个页面会被视为消含糊页如果它是链自[[MediaWiki:Disambiguationspage]]。',

'doubleredirects'            => '双重重定向页面',
'doubleredirectstext'        => '这一页列出所有重定向页面重定向到另一个重定向页的页面。每一行都包含到第一和第二个重定向页面的链接，以及第二个重定向页面的目标，通常显示的都会是"真正"的目标页面，也就是第一个重定向页面应该指向的页面。',
'double-redirect-fixed-move' => '[[$1]]已经完成移动，它现在重定向到[[$2]]',
'double-redirect-fixer'      => '重定向修正器',

'brokenredirects'        => '损坏的重定向页',
'brokenredirectstext'    => '以下的重定向页面指向的是不存在的页面。',
'brokenredirects-edit'   => '(编辑)',
'brokenredirects-delete' => '(删除)',

'withoutinterwiki'         => '未有语言链接的页面',
'withoutinterwiki-summary' => '以下的页面是未有语言链接到其它语言版本。',
'withoutinterwiki-legend'  => '前缀',
'withoutinterwiki-submit'  => '显示',

'fewestrevisions' => '最少修订的页面',

# Miscellaneous special pages
'nbytes'                  => '$1字节',
'ncategories'             => '$1个分类',
'nlinks'                  => '$1个链接',
'nmembers'                => '$1个成员',
'nrevisions'              => '$1个修订',
'nviews'                  => '$1次浏览',
'specialpage-empty'       => '这个报告的结果为空。',
'lonelypages'             => '孤立页面',
'lonelypagestext'         => '以下页面尚未被{{SITENAME}}中的其它页面链接。',
'uncategorizedpages'      => '未归类页面',
'uncategorizedcategories' => '未归类分类',
'uncategorizedimages'     => '未归类文件',
'uncategorizedtemplates'  => '未归类模版',
'unusedcategories'        => '未使用分类',
'unusedimages'            => '未使用图像',
'popularpages'            => '热点页面',
'wantedcategories'        => '待撰分类',
'wantedpages'             => '待撰页面',
'missingfiles'            => '遗失文件',
'mostlinked'              => '最多链接页面',
'mostlinkedcategories'    => '最多链接分类',
'mostlinkedtemplates'     => '最多链接模版',
'mostcategories'          => '最多分类页面',
'mostimages'              => '最多链接文件',
'mostrevisions'           => '最多修订页面',
'prefixindex'             => '前缀索引',
'shortpages'              => '短页面',
'longpages'               => '长页面',
'deadendpages'            => '断链页面',
'deadendpagestext'        => '以下页面没有链接到{{SITENAME}}中的其它页面。',
'protectedpages'          => '已保护页面',
'protectedpages-indef'    => '只有无期之保护页面',
'protectedpagestext'      => '以下页面已经被保护以防止移移或编辑',
'protectedpagesempty'     => '在这些参数下没有页面正在保护。',
'protectedtitles'         => '已保护的标题',
'protectedtitlestext'     => '以下的页面已经被保护以防止创建',
'protectedtitlesempty'    => '在这些参数之下并无标题正在保护。',
'listusers'               => '用户列表',
'newpages'                => '最新页面',
'newpages-username'       => '用户名:',
'ancientpages'            => '最早页面',
'move'                    => '移动',
'movethispage'            => '移动此页',
'unusedimagestext'        => '<p>请注意其它网站可能直接通过 URL 链接此文件，所以这里列出的图像有可能依然被使用。</p>',
'unusedcategoriestext'    => '虽然没有被其它页面或者分类所采用，但列表中的分类页依然存在。',
'notargettitle'           => '无目标',
'notargettext'            => '您还没有指定一个目标页面或用户以进行此项操作。',
'nopagetitle'             => '无目标页面',
'nopagetext'              => '您所指定的目标页面并不存在。',
'pager-newer-n'           => '新$1次',
'pager-older-n'           => '旧$1次',
'suppress'                => '监督',

# Book sources
'booksources'               => '网络书源',
'booksources-search-legend' => '搜索网络书源',
'booksources-go'            => '转到',
'booksources-text'          => '以下是一些网络书店的链接列表，其中可能有您要找的书籍的更多信息:',

# Special:Log
'specialloguserlabel'  => '用户:',
'speciallogtitlelabel' => '标题:',
'log'                  => '日志',
'all-logs-page'        => '所有日志',
'log-search-legend'    => '搜寻日志',
'log-search-submit'    => '去',
'alllogstext'          => '综合显示上传、删除、保护、查封以及管理日志。
您可以选择日志类型，用户名或者相关页面来缩小查询范围。',
'logempty'             => '在日志中不存在匹配项。',
'log-title-wildcard'   => '搜寻以这个文字开始的标题',

# Special:AllPages
'allpages'          => '所有页面',
'alphaindexline'    => '$1到$2',
'nextpage'          => '下一页($1)',
'prevpage'          => '上一页($1)',
'allpagesfrom'      => '显示从此处开始的页面:',
'allarticles'       => '所有页面',
'allinnamespace'    => '所有页面(属于$1名字空间)',
'allnotinnamespace' => '所有页面(不属于$1名字空间)',
'allpagesprev'      => '前',
'allpagesnext'      => '后',
'allpagessubmit'    => '提交',
'allpagesprefix'    => '显示具有此前缀(名字空间)的页面:',
'allpagesbadtitle'  => '给定的页面标题是非法的，或者具有一个内部语言或内部 wiki 的前缀。它可能包含一个或更多的不能用于标题的字符。',
'allpages-bad-ns'   => '在{{SITENAME}}中没有一个叫做"$1"的名字空间。',

# Special:Categories
'categories'                    => '页面分类',
'categoriespagetext'            => '以下的分类中包含了页面或媒体。
[[Special:UnusedCategories|未用分类]]不会在这里列示。
请同时参阅[[Special:WantedCategories|需要的分类]]。',
'categoriesfrom'                => '显示由此项起之分类:',
'special-categories-sort-count' => '按数量排列',
'special-categories-sort-abc'   => '按字母排列',

# Special:ListUsers
'listusersfrom'      => '给定显示用户条件:',
'listusers-submit'   => '显示',
'listusers-noresult' => '找不到用户。',

# Special:ListGroupRights
'listgrouprights'          => '用户群组权限',
'listgrouprights-summary'  => '以下面是一个在这个wiki中定义出来的用户权限列表，以及它们的访问权。
更多有关个别权限的细节可以在[[{{MediaWiki:Listgrouprights-helppage}}|这里]]找到。',
'listgrouprights-group'    => '群组',
'listgrouprights-rights'   => '权限',
'listgrouprights-helppage' => 'Help:群组权限',
'listgrouprights-members'  => '(成员列表)',

# E-mail user
'mailnologin'     => '无电邮地址',
'mailnologintext' => '您必须先[[Special:UserLogin|登录]]
并在[[Special:Preferences|参数设置]]
中有一个有效的电子邮箱地址才可以向其他用户发邮件。',
'emailuser'       => '向该用户发邮件',
'emailpage'       => '向用户发邮件',
'emailpagetext'   => '如果该用户已经在他或她的参数设置页中输入了有效的电子邮箱地址，以下的表单将寄一个信息给该用户。您在[[Special:Preferences|您参数设置]]中所输入的电子邮箱地址将出现在邮件"发件人"一栏中，这样该用户就可以回复您。',
'usermailererror' => 'Mail 对象返回错误:',
'defemailsubject' => '{{SITENAME}}电子邮件',
'noemailtitle'    => '无电子邮件地址',
'noemailtext'     => '该用户还没有指定一个有效的电子邮件地址，
或者选择不接受来自其他用户的电子邮件。',
'emailfrom'       => '发件人:',
'emailto'         => '收件人:',
'emailsubject'    => '主题:',
'emailmessage'    => '信息:',
'emailsend'       => '发送',
'emailccme'       => '将我的消息的副本发送一份到我的邮箱。',
'emailccsubject'  => '将您的消息复制到 $1: $2',
'emailsent'       => '电子邮件已发送',
'emailsenttext'   => '您的电子邮件已经发出。',
'emailuserfooter' => '这封电邮是由$1寄给$2经{{SITENAME}}的“电邮用户”功能发出的。',

# Watchlist
'watchlist'            => '监视列表',
'mywatchlist'          => '我的监视列表',
'watchlistfor'         => "('''$1'''的监视列表')",
'nowatchlist'          => '您的监视列表为空。',
'watchlistanontext'    => '请$1以查看或编辑您的监视列表。',
'watchnologin'         => '未登录',
'watchnologintext'     => '您必须先[[Special:UserLogin|登录]]才能更改您的监视列表。',
'addedwatch'           => '已添加至监视列表',
'addedwatchtext'       => "页面\"[[:\$1]]\"已经被加入到您的[[Special:Watchlist|监视列表]]中。
将来有关此页面及其讨论页的任何修改将会在那里列出，
而且还会在[[Special:RecentChanges|最近更改]]中
以'''粗体'''形式列出以使起更容易识别。",
'removedwatch'         => '已停止监视',
'removedwatchtext'     => '页面"<nowiki>$1</nowiki>"已经从[[Special:Watchlist|您的监视页面]]中移除。',
'watch'                => '监视',
'watchthispage'        => '监视此页',
'unwatch'              => '取消监视',
'unwatchthispage'      => '停止监视',
'notanarticle'         => '不是页面',
'notvisiblerev'        => '修订版本已经删除',
'watchnochange'        => '在显示的时间段内您所监视的页面没有更改。',
'watchlist-details'    => '不包含讨论页，有 $1 页在您的监视列表上。',
'wlheader-enotif'      => '* 已经启动电子邮件通知功能。',
'wlheader-showupdated' => "* 在你上次查看后有被修改过的页面会显示为'''粗体'''",
'watchmethod-recent'   => '检查被监视页面的最近编辑',
'watchmethod-list'     => '查看监视页中的最新修改',
'watchlistcontains'    => '您的监视列表包含$1个页面。',
'iteminvalidname'      => "页面'$1'错误，无效命名...",
'wlnote'               => "以下是最近'''$2'''小时内的最后'''$1'''次修改:",
'wlshowlast'           => '显示最近$1小时 $2天 $3的修改',
'watchlist-show-bots'  => '显示机器人的编辑',
'watchlist-hide-bots'  => '隐藏机器人的编辑',
'watchlist-show-own'   => '显示我的编辑',
'watchlist-hide-own'   => '隐藏我的编辑',
'watchlist-show-minor' => '显示小编辑',
'watchlist-hide-minor' => '隐藏小编辑',

# Displayed when you click the "watch" button and it is in the process of watching
'watching'   => '监视……',
'unwatching' => '解除监视……',

'enotif_mailer'                => '{{SITENAME}}邮件通知器',
'enotif_reset'                 => '将所有页面标为已读',
'enotif_newpagetext'           => '这是新建页面。',
'enotif_impersonal_salutation' => '{{SITENAME}}用户',
'changed'                      => '修改了',
'created'                      => '建立了',
'enotif_subject'               => '{{SITENAME}}有页面 $PAGETITLE 被 $PAGEEDITOR $CHANGEDORCREATED',
'enotif_lastvisited'           => '查看您上次访问后的所有更改请访问$1。',
'enotif_lastdiff'              => '检视更改请访问$1。',
'enotif_anon_editor'           => '匿名用户$1',
'enotif_body'                  => '亲爱的 $WATCHINGUSERNAME,

$PAGEEDITOR 已经在 $PAGEEDITDATE $CHANGEDORCREATED{{SITENAME}}的 $PAGETITLE 页面，请到 $PAGETITLE_URL 查看当前版本。

$NEWPAGE

编辑摘要: $PAGESUMMARY $PAGEMINOREDIT

联系此编辑者:

邮件: $PAGEEDITOR_EMAIL

本站: $PAGEEDITOR_WIKI

在您访问此页之前，将来的更改将不会向您发通知。您也可以重设您所有监视页面的通知标记。

                {{SITENAME}}通知系统

--
要改变您的监视列表设置，请访问
{{fullurl:{{ns:special}}:Watchlist/edit}}

反馈和进一步的帮助:
{{fullurl:{{MediaWiki:Helppage}}}}',

# Delete/protect/revert
'deletepage'                  => '删除页面',
'confirm'                     => '确认',
'excontent'                   => '内容为: "$1"',
'excontentauthor'             => '内容为: "$1" (而且唯一贡献者为"$2")',
'exbeforeblank'               => '被清空前的内容为: "$1"',
'exblank'                     => '页面为空',
'delete-confirm'              => '删除“$1”',
'delete-legend'               => '删除',
'historywarning'              => '警告: 您将要删除的页内含有历史',
'confirmdeletetext'           => '您即将删除一个页面或图像以及其历史。
请确定您要进行此项操作，并且了解其后果，同时您的行为符合[[{{MediaWiki:Policy-url}}]]。',
'actioncomplete'              => '操作完成',
'deletedtext'                 => '"<nowiki>$1</nowiki>"已经被删除。最近删除的纪录请参见$2。',
'deletedarticle'              => '已删除"[[$1]]"',
'suppressedarticle'           => '已废止"[[$1]]"',
'dellogpage'                  => '删除日志',
'dellogpagetext'              => '以下是最近删除的纪录列列表:',
'deletionlog'                 => '删除日志',
'reverted'                    => '恢复到早期版本',
'deletecomment'               => '删除原因:',
'deleteotherreason'           => '其它／附加的理由:',
'deletereasonotherlist'       => '其它理由',
'deletereason-dropdown'       => '
*常用删除理由
** 作者请求
** 侵犯版权
** 破坏',
'delete-edit-reasonlist'      => '编辑删除理由',
'delete-toobig'               => '这个页面有一个十分大量的编辑历史，超过$1次修订。删除此类页面的动作已经被限制，以防止在{{SITENAME}}上的意外扰乱。',
'delete-warning-toobig'       => '这个页面有一个十分大量的编辑历史，超过$1次修订。删除它可能会扰乱{{SITENAME}}的数据库操作；在继续此动作前请小心。',
'rollback'                    => '恢复编辑',
'rollback_short'              => '恢复',
'rollbacklink'                => '恢复',
'rollbackfailed'              => '恢复失败',
'cantrollback'                => '无法恢复编辑；最后的贡献者是本文的唯一作者。',
'alreadyrolled'               => '无法恢复由[[User:$2|$2]] ([[User talk:$2|讨论]]  | [[Special:Contributions/$2|{{int:contribslink}}]])进行的[[$1]]的最后编辑；
其他人已经编辑或是恢复了该页。

最后对页面编辑的编辑者: [[User:$3|$3]] ([[User talk:$3|讨论]] | [[Special:Contributions/$3|{{int:contribslink}}]])。',
'editcomment'                 => '编辑说明: "<i>$1</i>"。', # only shown if there is an edit comment
'revertpage'                  => '恢复由[[Special:Contributions/$2|$2]] ([[User talk:$2|对话]])的编辑至[[User:$1|$1]]的最后一个版本', # Additional available: $3: revid of the revision reverted to, $4: timestamp of the revision reverted to, $5: revid of the revision reverted from, $6: timestamp of the revision reverted from
'rollback-success'            => '恢复由$1的编辑；更改回$2的最后一个版本。',
'sessionfailure'              => '似乎在您登录时发生问题，作为一项防范性措施，该动作已经被取消。请单击"后退"再次尝试！',
'protectlogpage'              => '保护日志',
'protectlogtext'              => '下面是页面锁定和取消锁定的列表。请参考[[Special:ProtectedPages|保护页面列表]]以查看当前进行的页面保护。',
'protectedarticle'            => '已保护"[[$1]]"',
'modifiedarticleprotection'   => '已经更改 "[[$1]]" 的保护等级',
'unprotectedarticle'          => '已取消保护"[[$1]]"',
'protect-title'               => '正在更改"$1"的保护等级',
'protect-legend'              => '确认保护',
'protectcomment'              => '注解:',
'protectexpiry'               => '到期:',
'protect_expiry_invalid'      => '输入的终止时间无效。',
'protect_expiry_old'          => '终止时间已过去。',
'protect-unchain'             => '移动权限解锁',
'protect-text'                => '你可以在这里浏览和修改对页面<strong><nowiki>$1</nowiki></strong>的保护级别。',
'protect-locked-blocked'      => '您不能在被查封时更改保护级别。
以下是<strong>$1</strong>现时的保护级别:',
'protect-locked-dblock'       => '在资料库锁定时无法更改保护级别。
以下是<strong>$1</strong>现时的保护级别:',
'protect-locked-access'       => '您的帐户权限不能修改保护级别。
以下是<strong>$1</strong>现时的保护级别:',
'protect-cascadeon'           => '以下的{{PLURAL:$1|一个|多个}}页面包含  本页面的同时，启动了连锁保护，因此本页面目前也被保护，未能编辑。您可以设置本页面的保护级别，但这并不会对连锁保护有所影响。',
'protect-default'             => '(默认)',
'protect-fallback'            => '需要"$1"的许可',
'protect-level-autoconfirmed' => '禁止未注册用户',
'protect-level-sysop'         => '仅操作员',
'protect-summary-cascade'     => '联锁',
'protect-expiring'            => '终止于 $1 (UTC)',
'protect-cascade'             => '保护本页中包含的页面 (连锁保护)',
'protect-cantedit'            => '您无法更改这个页面的保护等级，因为您没有权限去编辑它。',
'restriction-type'            => '权限:',
'restriction-level'           => '限制级别:',
'minimum-size'                => '最小大小',
'maximum-size'                => '最大大小:',
'pagesize'                    => '(字节)',

# Restrictions (nouns)
'restriction-edit'   => '编辑',
'restriction-move'   => '移动',
'restriction-create' => '创建',
'restriction-upload' => '上传',

# Restriction levels
'restriction-level-sysop'         => '全保护',
'restriction-level-autoconfirmed' => '半保护',
'restriction-level-all'           => '任何级别',

# Undelete
'undelete'                     => '恢复被删页面',
'undeletepage'                 => '浏览及恢复被删页面',
'undeletepagetitle'            => "'''以下包含[[:$1]]的已删除之修订版本'''。",
'viewdeletedpage'              => '查看被删页面',
'undeletepagetext'             => '以下页面已经被删除，但依然在档案中并可以被恢复。
档案库可能被定时清理。',
'undelete-fieldset-title'      => '恢复修订',
'undeleteextrahelp'            => "恢复整个页面时，请清除所有复选框后点击 '''''恢复'''''。恢复特定版本时，请选择相应版本前的复选框后点击 '''''恢复'''''。点击 '''''重设''''' 将清除评论内容及所有复选框。",
'undeleterevisions'            => '$1版本存档',
'undeletehistory'              => '如果您恢复了该页面，所有版本都会被恢复到修订历史中。
如果本页删除后有一个同名的新页面建立，被恢复的版本将会称为较新的历史。',
'undeleterevdel'               => '如果把最新修订部份删除，反删除便无法进行。如果遇到这种情况，您必须反选或反隐藏最新已删除的修订。',
'undeletehistorynoadmin'       => '这个页面已被删除。删除原因显示在下方编辑摘要中，被删除前的所有修订文本连同删除前贡献用户的细节信息只对管理员可见。',
'undelete-revision'            => '删除$1时由$3（在$2）所编写的修订版本:',
'undeleterevision-missing'     => '无效或丢失的修订版本。您可能使用了错误的链接，或者此修订版本已经被从存档中恢复或移除。',
'undelete-nodiff'              => '找不到先前的修订版本。',
'undeletebtn'                  => '恢复',
'undeletelink'                 => '恢复',
'undeletereset'                => '重设',
'undeletecomment'              => '评论:',
'undeletedarticle'             => '已恢复的"[[$1]]"',
'undeletedrevisions'           => '$1个修订版本已恢复',
'undeletedrevisions-files'     => '$1个修订版本和$2个文件已经被恢复',
'undeletedfiles'               => '$1个文件已经被恢复',
'cannotundelete'               => '恢复删除失败；可能已有其他人先行恢复了此页面。',
'undeletedpage'                => "<big>'''$1已经被恢复'''</big>

参考[[Special:Log/delete|删除日志]]查看删除及恢复记录。",
'undelete-header'              => '如要查询最近的记录请参阅[[Special:Log/delete|删除日志]]。',
'undelete-search-box'          => '搜索已删除页面',
'undelete-search-prefix'       => '显示页面自:',
'undelete-search-submit'       => '搜索',
'undelete-no-results'          => '删除记录里没有符合的结果。',
'undelete-filename-mismatch'   => '不能删除带有时间截记的文件修订 $1: 文件不匹配',
'undelete-bad-store-key'       => '不能删除带有时间截记的文件修订 $1: 文件于删除前遗失。',
'undelete-cleanup-error'       => '删除无用的存档文件 "$1" 时发生错误。',
'undelete-missing-filearchive' => '由于文件存档 ID $1 不在数据库中，不能在文件存档中恢复。它可能已经反删除了。',
'undelete-error-short'         => '反删除文件时发生错误: $1',
'undelete-error-long'          => '当进行反删除文件时遇到错误:

$1',

# Namespace form on various pages
'namespace'      => '名字空间:',
'invert'         => '反向选定',
'blanknamespace' => '(主)',

# Contributions
'contributions' => '用户贡献',
'mycontris'     => '我的贡献',
'contribsub2'   => '$1的贡献 ($2)',
'nocontribs'    => '没有找到符合特征的更改。',
'uctop'         => '(最新修改)',
'month'         => '从该月份 (或更早):',
'year'          => '从该年份 (或更早):',

'sp-contributions-newbies'     => '只显示新创建之用户的贡献',
'sp-contributions-newbies-sub' => '新手',
'sp-contributions-blocklog'    => '查封记录',
'sp-contributions-search'      => '搜寻贡献记录',
'sp-contributions-username'    => 'IP地址或用户名称：',
'sp-contributions-submit'      => '搜索',

# What links here
'whatlinkshere'            => '链入页面',
'whatlinkshere-title'      => '链接到“$1”的页面',
'whatlinkshere-page'       => '页面:',
'linklistsub'              => '(链接列表)',
'linkshere'                => '以下页面链接到[[:$1]]：',
'nolinkshere'              => '没有页面链接到[[:$1]]。',
'nolinkshere-ns'           => '在所选的名字空间内没有页面链接到[[:$1]]。',
'isredirect'               => '重定向页',
'istemplate'               => '包含',
'isimage'                  => '文件链接',
'whatlinkshere-prev'       => '前$1个',
'whatlinkshere-next'       => '后$1个',
'whatlinkshere-links'      => '←链入',
'whatlinkshere-hideredirs' => '$1重定向',
'whatlinkshere-hidetrans'  => '$1包含',
'whatlinkshere-hidelinks'  => '$1链接',
'whatlinkshere-hideimages' => '$1文件链接',
'whatlinkshere-filters'    => '过滤器',

# Block/unblock
'blockip'                         => '查封用户',
'blockip-legend'                  => '查封用户',
'blockiptext'                     => '用下面的表单来禁止来自某一特定IP地址的修改权限。
只有在为防止破坏，及符合[[{{MediaWiki:Policy-url}}|守则]]的情况下才可采取此行动。
请在下面输入一个具体的理由（例如引述一个被破坏的页面）。',
'ipaddress'                       => 'IP地址:',
'ipadressorusername'              => 'IP地址或用户名:',
'ipbexpiry'                       => '期限:',
'ipbreason'                       => '原因:',
'ipbreasonotherlist'              => '其它原因',
'ipbreason-dropdown'              => '
*一般的封禁理由
** 屡次增加不实资料
** 删除页面内容
** 外部连结广告
** 在页面中增加无意义文字
** 无礼的行为、攻击／骚扰别人
** 滥用多个帐号
** 不能接受的用户名',
'ipbanononly'                     => '仅阻止匿名用户',
'ipbcreateaccount'                => '阻止创建新账号',
'ipbemailban'                     => '阻止用户发送电邮',
'ipbenableautoblock'              => '自动查封此用户最后所用的IP地址，以及后来试图编辑所用的所有地址',
'ipbsubmit'                       => '查封该地址',
'ipbother'                        => '其它时间:',
'ipboptions'                      => '2小时:2 hours,1天:1 day,3天:3 days,1周:1 week,2周:2 weeks,1个月:1 month,3个月:3 months,6个月:6 months,1年:1 year,永久:infinite', # display1:time1,display2:time2,...
'ipbotheroption'                  => '其它',
'ipbotherreason'                  => '其它／附带原因:',
'ipbhidename'                     => '在查封日志、活跃查封列表以及用户列表中隐藏用户名',
'ipbwatchuser'                    => '監視這位用戶的用戶頁面以及其對話頁面',
'badipaddress'                    => 'IP地址不正确。',
'blockipsuccesssub'               => '查封成功',
'blockipsuccesstext'              => '[[Special:Contributions/$1|$1]]已经被查封。
<br />参看[[Special:IPBlockList|被封IP地址列表]]以复审查封。',
'ipb-edit-dropdown'               => '编辑查封原因',
'ipb-unblock-addr'                => '解除封禁$1',
'ipb-unblock'                     => '解除禁封用户名或IP地址',
'ipb-blocklist-addr'              => '查看$1的现有封禁',
'ipb-blocklist'                   => '查看现有的封禁',
'unblockip'                       => '解封用户',
'unblockiptext'                   => '用下面的表单来恢复先前被查封的IP地址或用户的写权限。',
'ipusubmit'                       => '解封此地址',
'unblocked'                       => '[[User:$1|$1]]已经被解封',
'unblocked-id'                    => '封禁 $1 已经被删除',
'ipblocklist'                     => '被封IP地址列表',
'ipblocklist-legend'              => '检索一位已经被查封的用户',
'ipblocklist-username'            => '用户名称或IP地址:',
'ipblocklist-submit'              => '搜索',
'blocklistline'                   => '$1，$2禁封$3 ($4)',
'infiniteblock'                   => '永久',
'expiringblock'                   => '$1 到期',
'anononlyblock'                   => '仅限匿名用户',
'noautoblockblock'                => '禁用自动查封',
'createaccountblock'              => '禁止创建账户',
'emailblock'                      => '禁止电子邮件',
'ipblocklist-empty'               => '查封列表为空。',
'ipblocklist-no-results'          => '所要求的IP地址/用户名没有被查封。',
'blocklink'                       => '查封',
'unblocklink'                     => '解除禁封',
'contribslink'                    => '贡献',
'autoblocker'                     => '因为您与"[[$1]]"共享一个IP地址而被自动查封。$1被封的理由是"$2"。',
'blocklogpage'                    => '查封日志',
'blocklogentry'                   => '[[$1]]已被查封 $3 ，终止时间为$2',
'blocklogtext'                    => '这是关于用户查封和解封操作的日志。
被自动查封的IP地址没有被列出。请参看[[Special:IPBlockList|被封IP地址列表]]。',
'unblocklogentry'                 => '[[$1]]已被解封',
'block-log-flags-anononly'        => '仅限匿名用户',
'block-log-flags-nocreate'        => '禁止此IP/用户建立新帐户',
'block-log-flags-noautoblock'     => '禁用自动封禁',
'block-log-flags-noemail'         => '禁止电子邮件',
'block-log-flags-angry-autoblock' => '加强自动封锁已启用',
'range_block_disabled'            => '只有管理员才能创建禁止查封的范围。',
'ipb_expiry_invalid'              => '无效的终止时间。',
'ipb_expiry_temp'                 => '隐藏用户名封锁必须是永久性的。',
'ipb_already_blocked'             => '已经封锁"$1"',
'ipb_cant_unblock'                => '错误: 找不到查封ID$1。可能已经解除封禁。',
'ipb_blocked_as_range'            => '错误: 该IP $1 无直接查封，不可以解除封禁。但是它是在 $2 的查封范围之内，该段范围是可以解除封禁的。',
'ip_range_invalid'                => '无效的IP范围。\\n',
'blockme'                         => '禁封我',
'proxyblocker'                    => '代理封锁器',
'proxyblocker-disabled'           => '这个功能已经禁用。',
'proxyblockreason'                => '您的IP地址是一个开放的代理，它已经被封锁。请联系您的因特网服务提供商或技术支持者并告知告知他们该严重的安全问题。',
'proxyblocksuccess'               => '完成。\\n',
'sorbsreason'                     => '您的IP地址在{{SITENAME}}中被 DNSBL 列为属于开放代理服务器。',
'sorbs_create_account_reason'     => '由于您的IP地址在{{SITENAME}}中被 DNSBL 列为属于开放代理服务器，所以您不能创建新账户。',

# Developer tools
'lockdb'              => '锁定数据库',
'unlockdb'            => '解锁数据库',
'lockdbtext'          => '锁住数据库将禁止所有用户进行编辑页面、更改参数、编辑监视列表以及其他需要更改数据库的操作。
请确认您的决定，并且保证您在维护工作结束后会重新开放数据库。',
'unlockdbtext'        => '开放数据库将会恢复所有用户进行编辑页面、修改参数、编辑监视列表以及其他需要更改数据库的操作。
请确认您的决定。',
'lockconfirm'         => '是的，我确实想要锁定数据库。',
'unlockconfirm'       => '是的，我确实想要解锁数据库。',
'lockbtn'             => '数据库锁定',
'unlockbtn'           => '解锁数据库',
'locknoconfirm'       => '您并没有勾选确认按钮。',
'lockdbsuccesssub'    => '数据库锁定成功',
'unlockdbsuccesssub'  => '数据库解锁',
'lockdbsuccesstext'   => '{{SITENAME}}数据库已经锁定。
<br />请记住在维护完成后重新开放数据库。',
'unlockdbsuccesstext' => '{{SITENAME}}数据库重新开放。',
'lockfilenotwritable' => '数据库锁定文件不可写。要锁定和解锁数据库，该文件必须对网络服务器可写。',
'databasenotlocked'   => '数据库没有锁定。',

# Move page
'move-page'               => '移动$1',
'move-page-legend'        => '移动页面',
'movepagetext'            => "用下面的表单来重命名一个页面，并将其修订历史同时移动到新页面。
老的页面将成为新页面的重定向页。
您可以自动地更新指到原标题的重定向。
如果您选择不去做的话，请检查[[Special:DoubleRedirects|双重]]或[[Special:BrokenRedirects|损坏重定向]]连结。
您应当负责确定所有链接依然会链到指定的页面。

注意如果新页面已经有内容的话，页面将'''不会'''被移动，
除非新页面无内容或是重定向页，而且没有修订历史。
这意味着您再必要时可以在移动到新页面后再移回老的页面，
同时您也无法覆盖现有页面。

<b>警告！</b>
对一个经常被访问的页面而言这可能是一个重大与唐突的更改；
请在行动前先了结其所可能带来的后果。",
'movepagetalktext'        => "有关的讨论页将被自动与该页面一起移动，'''除非''':
*新页面已经有一个包含内容的讨论页，或者
*您不勾选下面的复选框。

在这些情况下，您在必要时必须手工移动或合并页面。",
'movearticle'             => '移动页面:',
'movenotallowed'          => '您在{{SITENAME}}中度并没有权限去移动页面。',
'newtitle'                => '新标题:',
'move-watch'              => '监视此页',
'movepagebtn'             => '移动页面',
'pagemovedsub'            => '移动成功',
'movepage-moved'          => "<big>'''“$1”已经移动到“$2”'''</big>", # The two titles are passed in plain text as $3 and $4 to allow additional goodies in the message.
'articleexists'           => '该名字的页面已经存在，或者您选择的名字无效。请再选一个名字。',
'cantmove-titleprotected' => '您不可以移动这个页面到这个位置，因为该新标题已经被保护以防止创建。',
'talkexists'              => '页面本身移动成功，
但是由于新标题下已经有对话页存在，所以对话页无法移动。请手工合并两个页面。',
'movedto'                 => '移动到',
'movetalk'                => '移动关联的讨论页',
'move-subpages'           => '如果可能的话，同时移动所有的子页面',
'move-talk-subpages'      => '如果可能的话，同时移动所有的子对话页面',
'movepage-page-exists'    => '页面$1已经存在，不可以自动地覆写。',
'movepage-page-moved'     => '页面$1已经移动到$2。',
'movepage-page-unmoved'   => '页面$1不可以移动到$2。',
'movepage-max-pages'      => '最多有$1个页面已经移动同时不可以自动地再移动更多。',
'1movedto2'               => '[[$1]]移动到[[$2]]',
'1movedto2_redir'         => '[[$1]]通过重定向移动到[[$2]]',
'movelogpage'             => '移动日志',
'movelogpagetext'         => '以下是已经移动的页面列表:',
'movereason'              => '原因',
'revertmove'              => '恢复该移动',
'delete_and_move'         => '删除并移动',
'delete_and_move_text'    => '==需要删除==

目标页面"[[:$1]]"已经存在。你确认需要删除原页面并以进行移动吗？',
'delete_and_move_confirm' => '是的，删除此页面',
'delete_and_move_reason'  => '删除以便移动',
'selfmove'                => '原始标题和目标标题相同，不能移动一个页面到它自身。',
'immobile_namespace'      => '目标标题属于特别类型；不能将页面移入这个名字空间。',
'imagenocrossnamespace'   => '不可以移动文件到非文件名字空间',
'imagetypemismatch'       => '该新副档名不匹配它的类型',
'imageinvalidfilename'    => '目标文件名称是无效的',
'fix-double-redirects'    => '更新指到原先标题的任何重定向',

# Export
'export'            => '导出页面',
'exporttext'        => '您可以将特定页面或一组页面的文本以及编辑历史以 XML 格式导出；这样可以将有关页面通过"[[Special:Import|导入页面]]"页面导入到另一个运行 MediaWiki 的网站。

要导出页面，请在下面的文本框中输入页面标题，每行一个标题，
并选择你是否需要导出带有页面历史的以前的版本，
或是只选择导出带有最后一次编辑信息的当前版本。

此外你还可以利用链接导出文件，例如你可以使用[[{{ns:special}}:Export/{{MediaWiki:Mainpage}}]]导出"[[{{MediaWiki:Mainpage}}]]"页面。',
'exportcuronly'     => '仅包含当前的修订，而不是全部的历史。',
'exportnohistory'   => "----
'''注意:''' 由于性能原因，从此表单导出页面的全部历史已被禁用。",
'export-submit'     => '导出',
'export-addcattext' => '由分类中添加页面:',
'export-addcat'     => '添加',
'export-download'   => '另存为文件',
'export-templates'  => '包含模版',

# Namespace 8 related
'allmessages'               => '系统界面',
'allmessagesname'           => '名称',
'allmessagesdefault'        => '默认的文字',
'allmessagescurrent'        => '当前的文字',
'allmessagestext'           => '这里列出所有可定制的系统界面。
如果想贡献正宗的MediaWiki本地化的话，请参阅[http://www.mediawiki.org/wiki/Localisation MediaWiki本地化]以及[http://translatewiki.net Betawiki]。',
'allmessagesnotsupportedDB' => "这个页面无法使用，因为'''\$wgUseDatabaseMessages'''已被设置关闭。",
'allmessagesfilter'         => '按消息名称筛选:',
'allmessagesmodified'       => '仅显示已修改的',

# Thumbnails
'thumbnail-more'           => '放大',
'filemissing'              => '无法找到文件',
'thumbnail_error'          => '创建缩略图错误: $1',
'djvu_page_error'          => 'DjVu页面超出范围',
'djvu_no_xml'              => '无法在DjVu文件中撷取XML',
'thumbnail_invalid_params' => '不正确的缩略图参数',
'thumbnail_dest_directory' => '无法建立目标目录',

# Special:Import
'import'                     => '导入页面',
'importinterwiki'            => '跨 wiki 导入',
'import-interwiki-text'      => '选择一个 wiki 和页面标题以进行导入。
修订日期和编辑者名字将同时被保存。
所有的跨 wiki 导入操作被记录在[[Special:Log/import|导入日志]]。',
'import-interwiki-history'   => '复制此页的所有历史版本',
'import-interwiki-submit'    => '导入',
'import-interwiki-namespace' => '将页面转移到名字空间:',
'importtext'                 => '请使用[[Special:Export|导出功能]]从源 wiki 导出文件，
保存到您的磁盘并上传到这里。',
'importstart'                => '正在导入页面...',
'import-revision-count'      => '$1个修订',
'importnopages'              => '没有导入的页面。',
'importfailed'               => '导入失败: <nowiki>$1</nowiki>',
'importunknownsource'        => '未知的源导入类型',
'importcantopen'             => '无法打开导入文件',
'importbadinterwiki'         => '损坏的内部 wiki 链接',
'importnotext'               => '空或没有文本',
'importsuccess'              => '导入完成！',
'importhistoryconflict'      => '存在冲突的修订历史(可能在之前已经导入过此页面)',
'importnosources'            => '跨Wiki导入源没有定义，同时不允许直接的历史上传。',
'importnofile'               => '没有上传导入文件。',
'importuploaderrorsize'      => '上载导入文件失败。文件大于可以允许的上传大小。',
'importuploaderrorpartial'   => '上载导入文件失败。文件只有部份已经上传。',
'importuploaderrortemp'      => '上载导入文件失败。临时文件夹已遗失。',
'import-parse-failure'       => 'XML导入语法失败',
'import-noarticle'           => '没有页面作导入！',
'import-nonewrevisions'      => '所有的修订已经在先前导入。',
'xml-error-string'           => '$1 於行$2，列$3 ($4字节): $5',
'import-upload'              => '上传XML资料',

# Import log
'importlogpage'                    => '导入日志',
'importlogpagetext'                => '来自其它 wiki 的行政性的带编辑历史导入页面。',
'import-logentry-upload'           => '通过文件上传导入的$1',
'import-logentry-upload-detail'    => '$1个修订',
'import-logentry-interwiki'        => '跨 wiki $1',
'import-logentry-interwiki-detail' => '来自$2的$1个修订',

# Tooltip help for the actions
'tooltip-pt-userpage'             => '我的用户页',
'tooltip-pt-anonuserpage'         => '您编辑本站所用IP的对应用户页',
'tooltip-pt-mytalk'               => '我的对话页',
'tooltip-pt-anontalk'             => '对于来自此IP地址的编辑的对话',
'tooltip-pt-preferences'          => '我的参数设置',
'tooltip-pt-watchlist'            => '我的监视列表',
'tooltip-pt-mycontris'            => '我的贡献列表',
'tooltip-pt-login'                => '我们鼓励您登录，但这并不是强制性的',
'tooltip-pt-anonlogin'            => '我们鼓励您登录，但这并不是强制性的',
'tooltip-pt-logout'               => '退出',
'tooltip-ca-talk'                 => '关于页面正文的讨论',
'tooltip-ca-edit'                 => '你可编辑此页，请在保存前先预览一下。',
'tooltip-ca-addsection'           => '在该讨论页增加新的评论主题',
'tooltip-ca-viewsource'           => '该页面已被保护。你可以查看该页源码。',
'tooltip-ca-history'              => '此页面的早前版本',
'tooltip-ca-protect'              => '保护此页',
'tooltip-ca-delete'               => '删除此页',
'tooltip-ca-undelete'             => '将这个页面恢复到被删除以前的状态',
'tooltip-ca-move'                 => '移动此页',
'tooltip-ca-watch'                => '将此页面加入监视列表',
'tooltip-ca-unwatch'              => '将此页面从监视列表中移去',
'tooltip-search'                  => '搜索该网站',
'tooltip-search-go'               => '如果相同的标题存在的话便直接前往该页面',
'tooltip-search-fulltext'         => '搜索该文字的页面',
'tooltip-p-logo'                  => '首页',
'tooltip-n-mainpage'              => '访问首页',
'tooltip-n-portal'                => '关于本计划, 您可以做什么, 应该如何做',
'tooltip-n-currentevents'         => '提供当前事件的背景资料',
'tooltip-n-recentchanges'         => '列出该网站的最近修改',
'tooltip-n-randompage'            => '随机载入一个页面',
'tooltip-n-help'                  => '寻求帮助',
'tooltip-t-whatlinkshere'         => '列出所有与此页相链的页面',
'tooltip-t-recentchangeslinked'   => '从此页链出的所有页面的更改',
'tooltip-feed-rss'                => '此页的 RSS 订阅',
'tooltip-feed-atom'               => '此页的 Atom 订阅',
'tooltip-t-contributions'         => '查看该用户的贡献列表',
'tooltip-t-emailuser'             => '向该用户发送一封电子邮件',
'tooltip-t-upload'                => '上传文件',
'tooltip-t-specialpages'          => '所有特殊页面列表',
'tooltip-t-print'                 => '这个页面的可打印版本',
'tooltip-t-permalink'             => '这个页面版本的永久链接',
'tooltip-ca-nstab-main'           => '查看页面内容',
'tooltip-ca-nstab-user'           => '查看用户页面',
'tooltip-ca-nstab-media'          => '查看媒体页面',
'tooltip-ca-nstab-special'        => '这是一个特殊页面，您不能对它进行编辑',
'tooltip-ca-nstab-project'        => '查看计划页面',
'tooltip-ca-nstab-image'          => '查看文件页面',
'tooltip-ca-nstab-mediawiki'      => '查看系统界面消息',
'tooltip-ca-nstab-template'       => '查看模板',
'tooltip-ca-nstab-help'           => '查看帮助页面',
'tooltip-ca-nstab-category'       => '查看分类页面',
'tooltip-minoredit'               => '将此标记为小更改',
'tooltip-save'                    => '保存您的更改',
'tooltip-preview'                 => '预览您的更改，请在保存前使用此功能！',
'tooltip-diff'                    => '显示您对该文字所做的更改。',
'tooltip-compareselectedversions' => '查看此页面两个选定的版本间的差异。',
'tooltip-watch'                   => '将该页面加到您的监视列表',
'tooltip-recreate'                => '重建该页面，无论是否被删除。',
'tooltip-upload'                  => '开始上传',

# Stylesheets
'common.css'      => '/* 此处的 CSS 将应用于所有的皮肤 */',
'standard.css'    => '/* 此处的 CSS 将影响使用标准皮肤的用户 */',
'nostalgia.css'   => '/* 此处的 CSS 将影响使用怀旧皮肤的用户 */',
'cologneblue.css' => '/* 此处的 CSS 将影响使用科隆香水蓝皮肤的用户 */',
'monobook.css'    => '/* 此处的 CSS 将影响使用 Monobook 皮肤的用户 */',
'myskin.css'      => '/* 此处的 CSS 将影响使用 Myskin 皮肤的用户 */',
'chick.css'       => '/* 此处的 CSS 将影响使用 Chick 皮肤的用户 */',
'simple.css'      => '/* 此处的 CSS 将影响使用 Simple 皮肤的用户 */',
'modern.css'      => '/* 此处的 CSS 将影响使用 Modern 皮肤的用户 */',

# Scripts
'common.js'      => '/* 此处的JavaScript将加载于所有用户每一个页面。 */',
'standard.js'    => '/* 此处的JavaScript将加载于使用标准皮肤的用户 */',
'nostalgia.js'   => '/* 此处的JavaScript将加载于使用怀旧皮肤的用户 */',
'cologneblue.js' => '/* 此处的JavaScript将加载于使用科隆香水蓝皮肤的用户 */',
'monobook.js'    => '/* 此处的JavaScript将加载于使用Monobook皮肤的用户 */',
'myskin.js'      => '/* 此处的JavaScript将加载于使用Myskin皮肤的用户 */',
'chick.js'       => '/* 此处的JavaScript将加载于使用Chick皮肤的用户 */',
'simple.js'      => '/* 此处的JavaScript将加载于使用Simple皮肤的用户 */',
'modern.js'      => '/* 此处的JavaScript将加载于使用Modern皮肤的用户 */',

# Metadata
'nodublincore'      => 'Dublin Core RDF 元数据在该服务器不可用。',
'nocreativecommons' => 'Creative Commons RDF 元数据在该服务器不可用。',
'notacceptable'     => '该网站服务器不能提供您的客户端能识别的数据格式。',

# Attribution
'anonymous'        => '{{SITENAME}}的匿名用户',
'siteuser'         => '{{SITENAME}}用户$1',
'lastmodifiedatby' => '此页由$3于$1 $2的最后更改。', # $1 date, $2 time, $3 user
'othercontribs'    => '在$1的工作基础上。',
'others'           => '其他',
'siteusers'        => '{{SITENAME}}用户$1',
'creditspage'      => '页面致谢',
'nocredits'        => '该页没有致谢名单信息。',

# Spam protection
'spamprotectiontitle' => '广告保护过滤器',
'spamprotectiontext'  => '您要保存的页面被广告过滤器阻止。这可能是由于一个到外部站点的链接引起的。',
'spamprotectionmatch' => '以下是触发广告过滤器的文本: $1',
'spambot_username'    => 'MediaWiki 广告清除',
'spam_reverting'      => '恢复到不包含链接至$1的最近版本',
'spam_blanking'       => '所有包含链接至$1的修订，消隐',

# Info page
'infosubtitle'   => '页面信息',
'numedits'       => '编辑数 (页面): $1',
'numtalkedits'   => '编辑数 (讨论页): $1',
'numwatchers'    => '监视者数目: $1',
'numauthors'     => '作者数量 (页面): $1',
'numtalkauthors' => '作者数量 (讨论页): $1',

# Math options
'mw_math_png'    => '永远使用PNG图像',
'mw_math_simple' => '如果是简单的公式使用HTML，否则使用PNG图像',
'mw_math_html'   => '如果可以用HTML，否则用PNG图像',
'mw_math_source' => '显示为TeX代码 (使用文字浏览器时)',
'mw_math_modern' => '推荐为新版浏览器使用',
'mw_math_mathml' => '尽可能使用MathML (试验中)',

# Patrolling
'markaspatrolleddiff'                 => '标记为已检查',
'markaspatrolledtext'                 => '标记此页面为已检查',
'markedaspatrolled'                   => '标记为已检查',
'markedaspatrolledtext'               => '选定的版本已被标记为已检查.',
'rcpatroldisabled'                    => '最新更改检查被关闭',
'rcpatroldisabledtext'                => '最新更改检查的功能目前已关闭。',
'markedaspatrollederror'              => '不能标志为已检查',
'markedaspatrollederrortext'          => '你需要指定某个版本才能标志为已检查。',
'markedaspatrollederror-noautopatrol' => '您无法将你自己所作的更改标记为已检查。',

# Patrol log
'patrol-log-page'   => '巡查日志',
'patrol-log-header' => '这个是已经巡查过的日志。',
'patrol-log-line'   => '$2的版本$1已被标记为已检查的$3',
'patrol-log-auto'   => '(自动)',

# Image deletion
'deletedrevision'                 => '已删除旧版本$1',
'filedeleteerror-short'           => '删除文件发生错误: $1',
'filedeleteerror-long'            => '当删除文件时遇到错误:

$1',
'filedelete-missing'              => '因为文件 "$1" 不存在，所以它不可以删除。',
'filedelete-old-unregistered'     => '所指定的文件修订 "$1" 在数据库中不存在。',
'filedelete-current-unregistered' => '所指定的文件 "$1" 在数据库中不存在。',
'filedelete-archive-read-only'    => '存档目录 "$1" 在网页服务器中不可写。',

# Browsing diffs
'previousdiff' => '←上一个',
'nextdiff'     => '下一个→',

# Media information
'mediawarning'         => "'''警告''': 该文件可能包含恶意代码，运行它可能对您的系统带来危险。<hr />",
'imagemaxsize'         => '在文件描述页对图像大小限制为:',
'thumbsize'            => '缩略图大小:',
'widthheightpage'      => '$1×$2, $3页',
'file-info'            => '(文件大小: $1, MIME 类型: $2)',
'file-info-size'       => '($1 × $2 像素，文件大小：$3 ，MIME类型：$4)',
'file-nohires'         => '<small>无更高解像度可提供。</small>',
'svg-long-desc'        => '(SVG文件，表面大小： $1 × $2 像素，文件大小：$3)',
'show-big-image'       => '完整分辨率',
'show-big-image-thumb' => '<small>这幅略缩图的分辨率: $1 × $2 像素</small>',

# Special:NewImages
'newimages'             => '新建图像画廊',
'imagelisttext'         => "以下是按$2排列的'''$1'''个文件列表。",
'newimages-summary'     => '这个特殊页面中显示最后已上传的文件。',
'showhidebots'          => '($1机器人)',
'noimages'              => '无可查看图像。',
'ilsubmit'              => '搜索',
'bydate'                => '按日期',
'sp-newimages-showfrom' => '从$1 $2开始显示新文件',

# Bad image list
'bad_image_list' => '请根据以下的格式去编写:

只有列示项目（以 * 开头的项目）会被考虑。第一个连结一定要连接去坏文件中。
然后在同一行的连结会考虑作例外，即是該文件可以在哪一个页面中同时显示。',

/*
Short names for language variants used for language conversion links.
To disable showing a particular link, set it to 'disable', e.g.
'variantname-zh-sg' => 'disable',
Variants for Chinese language
*/
'variantname-zh-hans' => '简体',
'variantname-zh-hant' => '繁体',
'variantname-zh-cn'   => '大陆简体',
'variantname-zh-tw'   => '台湾正体',
'variantname-zh-hk'   => '香港繁体',
'variantname-zh-sg'   => '新加坡简体',
'variantname-zh'      => '不转换',

# Metadata
'metadata'          => '元数据',
'metadata-help'     => '此文件中包含有扩展的信息。这些信息可能是由数码相机或扫描仪在创建或数字化过程中所添加的。

如果此文件的源文件已经被修改，一些信息在修改后的文件中将不能完全反映出来。',
'metadata-expand'   => '显示详细资料',
'metadata-collapse' => '隐藏详细资料',
'metadata-fields'   => '在本信息中所列出的 EXIF 元数据域将包含在图片显示页面,
当元数据表损坏时只显示以下信息，其他的元数据默认为隐藏。
* make
* model
* datetimeoriginal
* exposuretime
* fnumber
* focallength', # Do not translate list items

# EXIF tags
'exif-imagewidth'                  => '宽度',
'exif-imagelength'                 => '高度',
'exif-bitspersample'               => '每象素比特数',
'exif-compression'                 => '压缩方案',
'exif-photometricinterpretation'   => '象素合成',
'exif-orientation'                 => '方位',
'exif-samplesperpixel'             => '象素数',
'exif-planarconfiguration'         => '数据排列',
'exif-ycbcrsubsampling'            => '黄色对洋红二次抽样比率',
'exif-ycbcrpositioning'            => '黄色和洋红配置',
'exif-xresolution'                 => '水平分辨率',
'exif-yresolution'                 => '垂直分辨率',
'exif-resolutionunit'              => 'X 轴与 Y 轴分辨率单位',
'exif-stripoffsets'                => '图像数据区',
'exif-rowsperstrip'                => '每带行数',
'exif-stripbytecounts'             => '每压缩带字节数',
'exif-jpeginterchangeformat'       => 'JPEG SOI 偏移',
'exif-jpeginterchangeformatlength' => 'JPEG 数据字节',
'exif-transferfunction'            => '转移功能',
'exif-whitepoint'                  => '白点色度',
'exif-primarychromaticities'       => '主要色度',
'exif-ycbcrcoefficients'           => '颜色空间转换矩阵系数',
'exif-referenceblackwhite'         => '黑白参照值对',
'exif-datetime'                    => '文件更改日期和时间',
'exif-imagedescription'            => '图像标题',
'exif-make'                        => '照相机制造商',
'exif-model'                       => '照相机型号',
'exif-software'                    => '所用软件',
'exif-artist'                      => '作者',
'exif-copyright'                   => '版权所有者',
'exif-exifversion'                 => 'Exif 版本',
'exif-flashpixversion'             => '支持的 Flashpix 版本',
'exif-colorspace'                  => '颜色空间',
'exif-componentsconfiguration'     => '每分量含义',
'exif-compressedbitsperpixel'      => '图像压缩模式',
'exif-pixelydimension'             => '有效图像宽度',
'exif-pixelxdimension'             => '有效图像高度',
'exif-makernote'                   => '制造商注释',
'exif-usercomment'                 => '用户注释',
'exif-relatedsoundfile'            => '相关的音频文件',
'exif-datetimeoriginal'            => '数据产生时间',
'exif-datetimedigitized'           => '数字化处理时间',
'exif-subsectime'                  => '日期时间秒',
'exif-subsectimeoriginal'          => '原始日期时间秒',
'exif-subsectimedigitized'         => '数字化日期时间秒',
'exif-exposuretime'                => '曝光时间',
'exif-exposuretime-format'         => '$1 秒 ($2)',
'exif-fnumber'                     => '光圈(F值)',
'exif-exposureprogram'             => '曝光模式',
'exif-spectralsensitivity'         => '感光',
'exif-isospeedratings'             => 'ISO 速率',
'exif-oecf'                        => '光电转换因子',
'exif-shutterspeedvalue'           => '快门速度',
'exif-aperturevalue'               => '光圈',
'exif-brightnessvalue'             => '亮度',
'exif-exposurebiasvalue'           => '曝光补偿',
'exif-maxaperturevalue'            => '最大陆地光圈',
'exif-subjectdistance'             => '物距',
'exif-meteringmode'                => '测量模式',
'exif-lightsource'                 => '光源',
'exif-flash'                       => '闪光灯',
'exif-focallength'                 => '焦距',
'exif-subjectarea'                 => '主体区域',
'exif-flashenergy'                 => '闪光灯强度',
'exif-spatialfrequencyresponse'    => '空间频率响应',
'exif-focalplanexresolution'       => 'X轴焦平面分辨率',
'exif-focalplaneyresolution'       => 'Y轴焦平面分辨率',
'exif-focalplaneresolutionunit'    => '焦平面分辨率单位',
'exif-subjectlocation'             => '主题位置',
'exif-exposureindex'               => '曝光指数',
'exif-sensingmethod'               => '感光模式',
'exif-filesource'                  => '文件源',
'exif-scenetype'                   => '场景类型',
'exif-cfapattern'                  => 'CFA 模式',
'exif-customrendered'              => '自定义图像处理',
'exif-exposuremode'                => '曝光模式',
'exif-whitebalance'                => '白平衡',
'exif-digitalzoomratio'            => '数字变焦比率',
'exif-focallengthin35mmfilm'       => '35毫米胶片焦距',
'exif-scenecapturetype'            => '情景拍摄类型',
'exif-gaincontrol'                 => '场景控制',
'exif-contrast'                    => '对比度',
'exif-saturation'                  => '饱和度',
'exif-sharpness'                   => '锐化',
'exif-devicesettingdescription'    => '设备设定描述',
'exif-subjectdistancerange'        => '主体距离范围',
'exif-imageuniqueid'               => '唯一图像ID',
'exif-gpsversionid'                => 'GPS 标签(tag)版本',
'exif-gpslatituderef'              => '北纬或南纬',
'exif-gpslatitude'                 => '纬度',
'exif-gpslongituderef'             => '东经或西经',
'exif-gpslongitude'                => '经度',
'exif-gpsaltituderef'              => '海拔正负参照',
'exif-gpsaltitude'                 => '海拔',
'exif-gpstimestamp'                => 'GPS 时间(原子时钟)',
'exif-gpssatellites'               => '测量使用的卫星',
'exif-gpsstatus'                   => '接收器状态',
'exif-gpsmeasuremode'              => '测量模式',
'exif-gpsdop'                      => '测量精度',
'exif-gpsspeedref'                 => '速度单位',
'exif-gpsspeed'                    => 'GPS 接收器速度',
'exif-gpstrackref'                 => '运动方位参照',
'exif-gpstrack'                    => '运动方位',
'exif-gpsimgdirectionref'          => '图像方位参照',
'exif-gpsimgdirection'             => '图像方位',
'exif-gpsmapdatum'                 => '使用地理测绘数据',
'exif-gpsdestlatituderef'          => '目标纬度参照',
'exif-gpsdestlatitude'             => '目标纬度',
'exif-gpsdestlongituderef'         => '目标经度的参照',
'exif-gpsdestlongitude'            => '目标经度',
'exif-gpsdestbearingref'           => '目标方位参照',
'exif-gpsdestbearing'              => '目标方位',
'exif-gpsdestdistanceref'          => '目标距离参照',
'exif-gpsdestdistance'             => '目标距离',
'exif-gpsprocessingmethod'         => 'GPS 处理方法名称',
'exif-gpsareainformation'          => 'GPS 区域名称',
'exif-gpsdatestamp'                => 'GPS 日期',
'exif-gpsdifferential'             => 'GPS 差动修正',

# EXIF attributes
'exif-compression-1' => '未压缩',

'exif-unknowndate' => '未知的日期',

'exif-orientation-1' => '标准', # 0th row: top; 0th column: left
'exif-orientation-2' => '水平翻转', # 0th row: top; 0th column: right
'exif-orientation-3' => '旋转180°', # 0th row: bottom; 0th column: right
'exif-orientation-4' => '垂直翻转', # 0th row: bottom; 0th column: left
'exif-orientation-5' => '旋转90° 逆时针并垂直翻转', # 0th row: left; 0th column: top
'exif-orientation-6' => '旋转90° 顺时针', # 0th row: right; 0th column: top
'exif-orientation-7' => '旋转90° 顺时针并垂直翻转', # 0th row: right; 0th column: bottom
'exif-orientation-8' => '旋转90° 逆时针', # 0th row: left; 0th column: bottom

'exif-planarconfiguration-1' => '矮胖格式',
'exif-planarconfiguration-2' => '平面格式',

'exif-componentsconfiguration-0' => '不存在',

'exif-exposureprogram-0' => '未定义',
'exif-exposureprogram-1' => '手动',
'exif-exposureprogram-2' => '标准程序',
'exif-exposureprogram-3' => '光圈优先模式',
'exif-exposureprogram-4' => '快门优先模式',
'exif-exposureprogram-5' => '艺术程序(景深优先)',
'exif-exposureprogram-6' => '运动程序(快速快门速度优先)',
'exif-exposureprogram-7' => '肖像模式(适用于背景在焦距以外的近距摄影)',
'exif-exposureprogram-8' => '风景模式(适用于背景在焦距上的风景照片)',

'exif-subjectdistance-value' => '$1米',

'exif-meteringmode-0'   => '未知',
'exif-meteringmode-1'   => '平均水平',
'exif-meteringmode-2'   => '中心加权平均测量',
'exif-meteringmode-3'   => '点测',
'exif-meteringmode-4'   => '多点测',
'exif-meteringmode-5'   => '模式测量',
'exif-meteringmode-6'   => '局部测量',
'exif-meteringmode-255' => '其它',

'exif-lightsource-0'   => '未知',
'exif-lightsource-1'   => '日光灯',
'exif-lightsource-2'   => '荧光灯',
'exif-lightsource-3'   => '钨丝灯(白炽灯)',
'exif-lightsource-4'   => '闪光灯',
'exif-lightsource-9'   => '晴天',
'exif-lightsource-10'  => '多云',
'exif-lightsource-11'  => '深色调阴影',
'exif-lightsource-12'  => '日光荧光灯(色温 D 5700 – 7100K)',
'exif-lightsource-13'  => '日温白色荧光灯(N 4600 – 5400K)',
'exif-lightsource-14'  => '冷白色荧光灯(W 3900 – 4500K)',
'exif-lightsource-15'  => '白色荧光 (WW 3200 – 3700K)',
'exif-lightsource-17'  => '标准灯光A',
'exif-lightsource-18'  => '标准灯光B',
'exif-lightsource-19'  => '标准灯光C',
'exif-lightsource-24'  => 'ISO摄影棚钨灯',
'exif-lightsource-255' => '其他光源',

'exif-focalplaneresolutionunit-2' => '英寸',

'exif-sensingmethod-1' => '未定义',
'exif-sensingmethod-2' => '一块彩色区域传感器',
'exif-sensingmethod-3' => '两块彩色区域传感器',
'exif-sensingmethod-4' => '三块彩色区域传感器',
'exif-sensingmethod-5' => '连续彩色区域传感器',
'exif-sensingmethod-7' => '三线传感器',
'exif-sensingmethod-8' => '连续彩色线性传感器',

'exif-scenetype-1' => '直接照像图片',

'exif-customrendered-0' => '标准处理',
'exif-customrendered-1' => '自定义处理',

'exif-exposuremode-0' => '自动曝光',
'exif-exposuremode-1' => '手动曝光',
'exif-exposuremode-2' => '自动曝光感知调节',

'exif-whitebalance-0' => '自动白平衡',
'exif-whitebalance-1' => '手动白平衡',

'exif-scenecapturetype-0' => '标准',
'exif-scenecapturetype-1' => '风景',
'exif-scenecapturetype-2' => '肖像',
'exif-scenecapturetype-3' => '夜景',

'exif-gaincontrol-0' => '无',
'exif-gaincontrol-1' => '低增益',
'exif-gaincontrol-2' => '高增益',
'exif-gaincontrol-3' => '低减益',
'exif-gaincontrol-4' => '高减益',

'exif-contrast-0' => '标准',
'exif-contrast-1' => '低',
'exif-contrast-2' => '高',

'exif-saturation-0' => '标准',
'exif-saturation-1' => '低饱和度',
'exif-saturation-2' => '高饱和度',

'exif-sharpness-0' => '标准',
'exif-sharpness-1' => '低',
'exif-sharpness-2' => '高',

'exif-subjectdistancerange-0' => '未知',
'exif-subjectdistancerange-1' => '自动处理程序(宏)',
'exif-subjectdistancerange-2' => '近景',
'exif-subjectdistancerange-3' => '远景',

# Pseudotags used for GPSLatitudeRef and GPSDestLatitudeRef
'exif-gpslatitude-n' => '北纬',
'exif-gpslatitude-s' => '南纬',

# Pseudotags used for GPSLongitudeRef and GPSDestLongitudeRef
'exif-gpslongitude-e' => '东经',
'exif-gpslongitude-w' => '西经',

'exif-gpsstatus-a' => '测量过程',
'exif-gpsstatus-v' => '互动测量',

'exif-gpsmeasuremode-2' => '二维测量',
'exif-gpsmeasuremode-3' => '三维测量',

# Pseudotags used for GPSSpeedRef and GPSDestDistanceRef
'exif-gpsspeed-k' => '公里每小时',
'exif-gpsspeed-m' => '英里每小时',
'exif-gpsspeed-n' => '海里每小时(节)',

# Pseudotags used for GPSTrackRef, GPSImgDirectionRef and GPSDestBearingRef
'exif-gpsdirection-t' => '真方位',
'exif-gpsdirection-m' => '地磁方位',

# External editor support
'edit-externally'      => '用外部程序编辑此文件',
'edit-externally-help' => '请参见[http://www.mediawiki.org/wiki/Manual:External_editors 设置步骤]了解详细信息。',

# 'all' in various places, this might be different for inflected languages
'recentchangesall' => '全部',
'imagelistall'     => '全部',
'watchlistall2'    => '全部',
'namespacesall'    => '全部',
'monthsall'        => '全部',

# E-mail address confirmation
'confirmemail'             => '确认邮箱地址',
'confirmemail_noemail'     => '您没有在您的[[Special:Preferences|用户设置]]里面输入一个有效的 email 地址。',
'confirmemail_text'        => '{{SITENAME}}要求您在使用邮件功能之前验证您的邮箱地址。
点击以下按钮可向您的邮箱发送一封确认邮件。该邮件包含有一行代码链接；
请在您的浏览器中加载此链接以确认您的邮箱地址是有效的。',
'confirmemail_pending'     => '<div class="error">
一个确认代码已经被发送到您的邮箱，您可能需要等几分钟才能收到。如果无法收到，请在申请一个新的确认码！
</div>',
'confirmemail_send'        => '邮发确认代码',
'confirmemail_sent'        => '确认邮件已发送。',
'confirmemail_oncreate'    => '一个确认代码已经被发送到您的邮箱。该代码并不要求您进行登录，
但若您要启用在此 wiki 上的任何基于电子邮件的功能，您必须先提交此代码。',
'confirmemail_sendfailed'  => '不能发送确认邮件，请检查邮箱地址是否包含非法字符。

邮件传送员回应: $1',
'confirmemail_invalid'     => '无效的确认码，该代码可能已经过期。',
'confirmemail_needlogin'   => '您需要$1以确认您的邮箱地址。',
'confirmemail_success'     => '您的邮箱已经被确认。您现在可以[[Special:UserLogin|登录]]并使用此网站了。',
'confirmemail_loggedin'    => '您的邮箱地址现在已被确认。',
'confirmemail_error'       => '你的确认过程发生错误。',
'confirmemail_subject'     => '{{SITENAME}}邮箱地址确认',
'confirmemail_body'        => '拥有IP地址$1的用户(可能是您)在{{SITENAME}}创建了账户"$2"，并提交了您的电子邮箱地址。

请确认这个账户是属于您的，并同时激活在{{SITENAME}}上的
电子邮件功能。请在浏览器中打开下面的链接:

$3

如果您*未*注册账户，
请打开下面的连结去取消电子邮件确认:

$5

确认码会在$4过期。',
'confirmemail_invalidated' => '电邮地址确认已取消',
'invalidateemail'          => '取消电邮确认',

# Scary transclusion
'scarytranscludedisabled' => '[跨网站的编码转换不可用]',
'scarytranscludefailed'   => '[提取$1失败]',
'scarytranscludetoolong'  => '[URL 过长]',

# Trackbacks
'trackbackbox'      => '<div id="mw_trackbacks">此页面的引用:
$1
</div>',
'trackbackremove'   => '([$1删除])',
'trackbacklink'     => '引用',
'trackbackdeleteok' => '该引用已被成功删除。',

# Delete conflict
'deletedwhileediting' => '警告: 此页在您开始编辑之后已经被删除！',
'confirmrecreate'     => '在您编辑这个页面后，用户[[User:$1|$1]]([[User talk:$1|对话]])以下列原因删除了这个页面: $2。
请确认在您重新创建页面前三思。',
'recreate'            => '重建',

# HTML dump
'redirectingto' => '重定向到[[:$1]]...',

# action=purge
'confirm_purge'        => '要清除此页面的缓存吗?\\n\\n$1',
'confirm_purge_button' => '确定',

# AJAX search
'searchcontaining' => "搜索包含''$1''的页面。",
'searchnamed'      => "搜索名为''$1''的页面。",
'articletitles'    => '页面以"$1"开头',
'hideresults'      => '隐藏结果',
'useajaxsearch'    => '使用AJAX搜索',

# Multipage image navigation
'imgmultipageprev' => '← 上一页',
'imgmultipagenext' => '下一页 →',
'imgmultigo'       => '确定！',
'imgmultigoto'     => '到第$1页',

# Table pager
'ascending_abbrev'         => '升',
'descending_abbrev'        => '降',
'table_pager_next'         => '下一页',
'table_pager_prev'         => '上一页',
'table_pager_first'        => '第一页',
'table_pager_last'         => '末一页',
'table_pager_limit'        => '每页显示$1项',
'table_pager_limit_submit' => '到',
'table_pager_empty'        => '没有结果',

# Auto-summaries
'autosumm-blank'   => '移除所有页面内容',
'autosumm-replace' => "正在将页面替换为 '$1'",
'autoredircomment' => '正在重定向到 [[$1]]',
'autosumm-new'     => '新页面: $1',

# Size units
'size-bytes' => '$1 字节',

# Live preview
'livepreview-loading' => '正在加载…',
'livepreview-ready'   => '正在加载… 完成!',
'livepreview-failed'  => '实时预览失败! 尝试标准预览。',
'livepreview-error'   => '连接失败: $1 "$2" 尝试标准预览。',

# Friendlier slave lag warnings
'lag-warn-normal' => '过去$1秒内的更改未必会在这个列表中显示。',
'lag-warn-high'   => '由于数据库的过度延迟，过去$1秒的更改未必会在这个列表中显示。',

# Watchlist editor
'watchlistedit-numitems'       => '您的监视列表中共有$1个标题，当中不包括对话页面。',
'watchlistedit-noitems'        => '您的监视列表并无标题。',
'watchlistedit-normal-title'   => '编辑监视列表',
'watchlistedit-normal-legend'  => '从监视列表中移除标题',
'watchlistedit-normal-explain' => '在您的监视列表中的标题在下面显示。要移除一个标题，在它前面剔一下，接着点击移除标题。您亦都可以[[Special:Watchlist/raw|编辑原始监视列表]]。',
'watchlistedit-normal-submit'  => '移除标题',
'watchlistedit-normal-done'    => '$1个标题已经从您的监视列表中移除:',
'watchlistedit-raw-title'      => '编辑原始监视列表',
'watchlistedit-raw-legend'     => '编辑原始监视列表',
'watchlistedit-raw-explain'    => '您的监视列表中的标题在下面显示，同时亦都可以通过编辑这个表去加入以及移除标题；一行一个标题。当完成以后，点击更新监视列表。你亦都可以去用[[Special:Watchlist/edit|标准编辑器]]。',
'watchlistedit-raw-titles'     => '标题:',
'watchlistedit-raw-submit'     => '更新监视列表',
'watchlistedit-raw-done'       => '您的监视列表已经更新。',
'watchlistedit-raw-added'      => '已经加入了$1个标题:',
'watchlistedit-raw-removed'    => '已经移除了$1个标题:',

# Watchlist editing tools
'watchlisttools-view' => '查看有关更改',
'watchlisttools-edit' => '查看并编辑监视列表',
'watchlisttools-raw'  => '编辑源监视列表',

# Core parser functions
'unknown_extension_tag' => '不明的扩展标签 "$1"',

# Special:Version
'version'                          => '版本', # Not used as normal message but as header for the special page itself
'version-extensions'               => '已经安装的扩展',
'version-specialpages'             => '特殊页面',
'version-parserhooks'              => '语法钩',
'version-variables'                => '变数',
'version-other'                    => '其它',
'version-mediahandlers'            => '媒体处理器',
'version-hooks'                    => '钩',
'version-extension-functions'      => '扩展函数',
'version-parser-extensiontags'     => '语法扩展标签',
'version-parser-function-hooks'    => '语法函数钩',
'version-skin-extension-functions' => '皮肤扩展函数',
'version-hook-name'                => '钩名',
'version-hook-subscribedby'        => '利用於',
'version-version'                  => '版本',
'version-license'                  => '授权',
'version-software'                 => '已经安装的软件',
'version-software-product'         => '产品',
'version-software-version'         => '版本',

# Special:FilePath
'filepath'         => '文件路径',
'filepath-page'    => '文件名:',
'filepath-submit'  => '查找路径',
'filepath-summary' => '这个特殊页面撷取一个文件的完整路径。图像会以完整的解像度显示，其它的文件类型会以同它们已关联程序启动。

请输入档名，不要包含"{{ns:image}}:"开头。',

# Special:FileDuplicateSearch
'fileduplicatesearch'          => '选择重覆文件',
'fileduplicatesearch-summary'  => '用重覆文件的切细值去找出文件是否重覆。

输入档名时不需要输入 "{{ns:image}}:" 开头。',
'fileduplicatesearch-legend'   => '找重覆',
'fileduplicatesearch-filename' => '文件名称:',
'fileduplicatesearch-submit'   => '找',
'fileduplicatesearch-info'     => '$1 × $2 像素<br />文件大小: $3<br />MIME类型: $4',
'fileduplicatesearch-result-1' => '文件 "$1" 无完全相同的重覆。',
'fileduplicatesearch-result-n' => '文件 "$1" 有$2项完全相同的重覆。',

# Special:SpecialPages
'specialpages'                   => '特殊页面',
'specialpages-note'              => '----
* 标准特殊页面。
* <span class="mw-specialpagerestricted">有限制的特殊页面。</span>',
'specialpages-group-maintenance' => '维护报告',
'specialpages-group-other'       => '其它特殊页面',
'specialpages-group-login'       => '登入／创建',
'specialpages-group-changes'     => '最近更改和日志',
'specialpages-group-media'       => '媒体报告和上传',
'specialpages-group-users'       => '用户和权限',
'specialpages-group-highuse'     => '高度使用页面',
'specialpages-group-pages'       => '页面列表',
'specialpages-group-pagetools'   => '页面工具',
'specialpages-group-wiki'        => 'Wiki资料和工具',
'specialpages-group-redirects'   => '重定向特殊页面',
'specialpages-group-spam'        => '反垃圾工具',

# Special:BlankPage
'blankpage'              => '空白页面',
'intentionallyblankpage' => '这个页面是为空白，以用来作测试速度等用途',

);
