<?php # $Id: serendipity_lang_zh.inc.php 716 2005-11-21 08:58:33Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
/* vim: set sts=4 ts=4 expandtab : */

@define('LANG_CHARSET', 'gb2312');
@define('DATE_LOCALES', 'cn, zh, zh_CN.GB2312, zh_GB, zh_CN');
@define('DATE_FORMAT_ENTRY', '%A, %B %e. %Y');
@define('DATE_FORMAT_SHORT', '%Y-%m-%d %H:%M');
@define('WYSIWYG_LANG', 'en');
@define('NUMBER_FORMAT_DECIMALS', '2');
@define('NUMBER_FORMAT_DECPOINT', '.');
@define('NUMBER_FORMAT_THOUSANDS', ',');
@define('LANG_DIRECTION', 'ltr');

@define('SERENDIPITY_ADMIN_SUITE', 'Serendipity 管理套件');
@define('HAVE_TO_BE_LOGGED_ON', '请先登入');
@define('WRONG_USERNAME_OR_PASSWORD', '您输入了错误的帐号或密码');
@define('APPEARANCE', '外观配置');
@define('MANAGE_STYLES', '布景管理');
@define('CONFIGURE_PLUGINS', '设定外挂');
@define('CONFIGURATION', '管理设定');
@define('BACK_TO_BLOG', '回到日记首页');
@define('LOGIN', '登入');
@define('LOGOUT', '登出');
@define('LOGGEDOUT', '登出.');
@define('CREATE', '建立');
@define('SAVE', '储存');
@define('NAME', '名称');
@define('CREATE_NEW_CAT', '新增类别');
@define('I_WANT_THUMB', '我要在文章内使用缩图.');
@define('I_WANT_BIG_IMAGE', '我要在文章内使用大型图片.');
@define('I_WANT_NO_LINK', ' 我要它以图片显示');
@define('I_WANT_IT_TO_LINK', '我要它以连结显示这个 URL:');
@define('BACK', '返回');
@define('FORWARD', '前进');
@define('ANONYMOUS', '匿名');
@define('NEW_TRACKBACK_TO', '新的引用到');
@define('NEW_COMMENT_TO', '新的回响到');
@define('RECENT', '新文库...');
@define('OLDER', '旧文库...');
@define('DONE', '完成');
@define('WELCOME_BACK', '欢迎回来,');
@define('TITLE', '标题');
@define('DESCRIPTION', '简介');
@define('PLACEMENT', '位址');
@define('DELETE', '删除');
@define('SAVE', '储存');
@define('UP', '上');
@define('DOWN', '下');
@define('ENTRIES', '文章');
@define('NEW_ENTRY', '新增文章');
@define('EDIT_ENTRIES', '编辑文章');
@define('CATEGORIES', '类别');
@define('WARNING_THIS_BLAHBLAH', "警告:\\n这个可能需要长时间如果有很多不是缩图的图片.");
@define('CREATE_THUMBS', '重建缩图');
@define('MANAGE_IMAGES', '管理图片');
@define('NAME', '名称');
@define('EMAIL', 'Email');
@define('HOMEPAGE', '网址');
@define('COMMENT', '回响');
@define('REMEMBER_INFO', '记住资料? ');
@define('SUBMIT_COMMENT', '传送回响');
@define('NO_ENTRIES_TO_PRINT', '没有文章可以显示');
@define('COMMENTS', '回响');
@define('ADD_COMMENT', '新增回响');
@define('NO_COMMENTS', '没有回响');
@define('POSTED_BY', '作者');
@define('ON', '在');
@define('A_NEW_COMMENT_BLAHBLAH', '新回响已发表到您的日记 "%s", 在这个文章里面 "%s".');
@define('A_NEW_TRACKBACK_BLAHBLAH', '您的日记文章 "%s" 已有了新的引用.');
@define('NO_CATEGORY', '没有类别');
@define('ENTRY_BODY', '文章主内容');
@define('EXTENDED_BODY', '文章副内容');
@define('CATEGORY', '类别');
@define('EDIT', '编辑');
@define('NO_ENTRIES_BLAHBLAH', '找不到此查询 %s 的文章' . "\n");
@define('YOUR_SEARCH_RETURNED_BLAHBLAH', '您搜寻的 %s 显示了 %s 结果:');
@define('IMAGE', '图片');
@define('ERROR_FILE_NOT_EXISTS', '错误: 档案不存在!');
@define('ERROR_FILE_EXISTS', '错误: 档案名称已被使用, 请重新输入!');
@define('ERROR_SOMETHING', '错误: 有错误.');
@define('ADDING_IMAGE', '新增图片...');
@define('THUMB_CREATED_DONE', '缩图建立.<br>完成.');
@define('ERROR_FILE_EXISTS_ALREADY', '错误: 档案已存在!');
@define('ERROR_UNKNOWN_NOUPLOAD', '未知的错误发生, 档案还没上传. 也许你的档案大於限制的大小. 请询问您的 ISP 或修改您的 php.ini 档.');
@define('GO', '继续!');
@define('NEWSIZE', '新大小: ');
@define('RESIZE_BLAHBLAH', '<b>重设大小 %s</b><p>');
@define('ORIGINAL_SIZE', '原有的大小: <i>%sx%s</i> 像素');
@define('HERE_YOU_CAN_ENTER_BLAHBLAH', '<p>在这里您可以修改图片大小. 如果您要修改成相同的图片比例, 您只需要输入一个数值然后按 TAB -- 系统会自动帮您计算比例以免出错</p>');
@define('QUICKJUMP_CALENDAR', '日历快速跳跃');
@define('QUICKSEARCH', '快速搜寻');
@define('SEARCH_FOR_ENTRY', '搜寻文章');
@define('ARCHIVES', '保存文库');
@define('BROWSE_ARCHIVES', '以月份浏览保存文库');
@define('TOP_REFERRER', '主要来源');
@define('SHOWS_TOP_SITES', '显示连结到您的日记的网站');
@define('TOP_EXITS', '主要出源');
@define('SHOWS_TOP_EXIT', '显示您的日记的主要出源');
@define('SYNDICATION', '文章同步');
@define('SHOWS_RSS_BLAHBLAH', '显示 RSS 同步连结');
@define('ADVERTISES_BLAHBLAH', '宣传您的网路日记');
@define('HTML_NUGGET', 'HTML 讯息');
@define('HOLDS_A_BLAHBLAH', '显示 HTML 讯息到侧列');
@define('TITLE_FOR_NUGGET', '讯息的标题');
@define('THE_NUGGET', 'HTML 讯息!');
@define('SYNDICATE_THIS_BLOG', '同步这个日记');
@define('YOU_CHOSE', '您选择 %s');
@define('IMAGE_SIZE', '图片大小');
@define('IMAGE_AS_A_LINK', '输入图片');
@define('POWERED_BY', 'Powered by');
@define('TRACKBACKS', '引用');
@define('TRACKBACK', '引用');
@define('NO_TRACKBACKS', '没有引用');
@define('TOPICS_OF', '主题');
@define('VIEW_FULL', '浏览全部');
@define('VIEW_TOPICS', '浏览主题');
@define('AT', '在');
@define('SET_AS_TEMPLATE', '使用布景');
@define('IN', '在');
@define('EXCERPT', '摘要');
@define('TRACKED', '引用');
@define('LINK_TO_ENTRY', '连结到文章');
@define('LINK_TO_REMOTE_ENTRY', '连结到远端文章');
@define('IP_ADDRESS', 'IP 位址');
@define('USER', '作者');
@define('THUMBNAIL_USING_OWN', '使用 %s 当它的缩图尺寸因为图片已经很小了.');
@define('THUMBNAIL_FAILED_COPY', '使用 %s 当它的缩图, 但是无法复制!');
@define('AUTHOR', '发表者');
@define('LAST_UPDATED', '最后更新');
@define('TRACKBACK_SPECIFIC', '引用此文章特定的 URI (网址)');
@define('DIRECT_LINK', '直接的文章连结');
@define('COMMENT_ADDED', '您的回响已成功增入. ');
@define('COMMENT_ADDED_CLICK', '点 %s这里返回%s 到回响, 和点 %s这里关闭%s 这个视窗.');
@define('COMMENT_NOT_ADDED_CLICK', '点 %s这里返回%s 到回响, 和点 %s这里关闭%s 这个视窗.');
@define('COMMENTS_DISABLE', '不允许回响到这篇文章');
@define('COMMENTS_ENABLE', '允许回响到这篇文章');
@define('COMMENTS_CLOSED', '作者不允许回响到这篇文章');
@define('EMPTY_COMMENT', '您的回响没有任何讯息, 请 %s返回%s 重试');
@define('ENTRIES_FOR', '文章给 %s');
@define('DOCUMENT_NOT_FOUND', '找不到此篇文件 %s.');
@define('USERNAME', '帐号');
@define('PASSWORD', '密码');
@define('AUTOMATIC_LOGIN', '自动登入');
@define('SERENDIPITY_INSTALLATION', 'Serendipity 安装程式');
@define('LEFT', '左');
@define('RIGHT', '右');
@define('HIDDEN', '隐藏');
@define('REMOVE_TICKED_PLUGINS', '移除勾选的外挂');
@define('SAVE_CHANGES_TO_LAYOUT', '储存外观配置');
@define('COMMENTS_FROM', '回响来源');
@define('ERROR', '错误');
@define('ENTRY_SAVED', '您的文章已储存');
@define('DELETE_SURE', '确定要删除 #%s 吗?');
@define('NOT_REALLY', '算了...');
@define('DUMP_IT', '删除吧!');
@define('RIP_ENTRY', 'R.I.P. 文章 #%s');
@define('CATEGORY_DELETED_ARTICLES_MOVED', '类别 #%s 已删除. 旧文章已被移动到类别 #%s');
@define('CATEGORY_DELETED', '类别 #%s 已删除.');
@define('INVALID_CATEGORY', '没有提供删除的类别');
@define('CATEGORY_SAVED', '类别已储存');
@define('SELECT_TEMPLATE', '请选择网路日记的布景');
@define('ENTRIES_NOT_SUCCESSFULLY_INSERTED', '没有完成增入文章!');
@define('MT_DATA_FILE', 'Movable Type 资料档');
@define('FORCE', '强制');
@define('CREATE_AUTHOR', '新增作者 \'%s\'.');
@define('CREATE_CATEGORY', '新增类别 \'%s\'.');
@define('MYSQL_REQUIRED', '您必须要有 MySQL 的扩充功能才能执行这个动作.');
@define('COULDNT_CONNECT', '不能联结到 MySQL 资料库: %s.');
@define('COULDNT_SELECT_DB', '不能选择资料库: %s.');
@define('COULDNT_SELECT_USER_INFO', '不能选择使用者的资料: %s.');
@define('COULDNT_SELECT_CATEGORY_INFO', '不能选择类别的资料: %s.');
@define('COULDNT_SELECT_ENTRY_INFO', '不能选择文章的资料: %s.');
@define('COULDNT_SELECT_COMMENT_INFO', '不能选择回响的资料: %s.');
@define('YES', '是');
@define('NO', '否');
@define('USE_DEFAULT', '预设');
@define('CHECK_N_SAVE', '储存');
@define('DIRECTORY_WRITE_ERROR', '不能读写档案夹 %s. 请检查权限.');
@define('DIRECTORY_CREATE_ERROR', '档案夹 %s 不存在也无法建立. 请自己建立这个档案夹');
@define('DIRECTORY_RUN_CMD', '&nbsp;-&gt; run <i>%s %s</i>');
@define('CANT_EXECUTE_BINARY', '无法执行 %s 资源档案');
@define('FILE_WRITE_ERROR', '无法读写档案 %s.');
@define('FILE_CREATE_YOURSELF', '请自己建立这个档案或检查权限');
@define('COPY_CODE_BELOW', '<br />* 请复制下面的代码然后放入 %s 到您的 %s 档案夹:<b><pre>%s</pre></b>' . "\n");
@define('WWW_USER', '请改变 www 到使用者的 Apache (i.e. nobody).');
@define('BROWSER_RELOAD', '完成之后, 重新刷新您的浏览器.');
@define('DIAGNOSTIC_ERROR', '系统侦测到一些错误:');
@define('SERENDIPITY_NOT_INSTALLED', 'Serendipity 还没安装完成. 请按 <a href="%s">安装</a>.');
@define('INCLUDE_ERROR', 'serendipity 错误: 无法包括 %s - 退出.');
@define('DATABASE_ERROR', 'serendipity 错误: 无法连结到资料库 - 退出.');
@define('CHECK_DATABASE_EXISTS', '检查资料库是否存在. 如果您看到资料库查询错误, 请不用管它...');
@define('CREATE_DATABASE', '建立预设资料库设定...');
@define('ATTEMPT_WRITE_FILE', '读写 %s 档案...');
@define('SERENDIPITY_INSTALLED', '%sSerendipity 已安装完成.%s 请记得您的密码: "%s", 您的帐号是 "%s".%s您现在可以到新建立的 <a href="%s">网路日记</a>');
@define('WRITTEN_N_SAVED', '储存完毕');
@define('IMAGE_ALIGNMENT', '图片对齐');
@define('ENTER_NEW_NAME', '输入新名称给: ');
@define('RESIZING', '重设大小');
@define('RESIZE_DONE', '完成 (重设 %s 个图片).');
@define('SYNCING', '进行资料库和图片档案夹同步化');
@define('SYNC_DONE', '完成 (同步了 %s 个图片).');
@define('FILE_NOT_FOUND', '找不到档案名称 <b>%s</b>, 也许已经被删除了?');
@define('ABORT_NOW', '放弃');
@define('REMOTE_FILE_NOT_FOUND', '档案不在远端主机内, 您确定这个 URL: <b>%s</b> 是正确的吗?');
@define('FILE_FETCHED', '%s 取回为 %s');
@define('FILE_UPLOADED', '档案 %s 上传为 %s');
@define('WORD_OR', '或');
@define('SCALING_IMAGE', '缩放 %s 到 %s x %s px');
@define('KEEP_PROPORTIONS', '维持比例');
@define('REALLY_SCALE_IMAGE', '确定要缩放图片吗? 这个动作不能复原!');
@define('TOGGLE_ALL', '切换展开');
@define('TOGGLE_OPTION', '切换选项');
@define('SUBSCRIBE_TO_THIS_ENTRY', '订阅这篇文章');
@define('UNSUBSCRIBE_OK', "%s 已取消订阅这篇文章");
@define('NEW_COMMENT_TO_SUBSCRIBED_ENTRY', '新回响到订阅的文章 "%s"');
@define('SUBSCRIPTION_MAIL', "您好 %s,\n\n您订阅的文章有了新的回响在 \"%s\", 标题是 \"%s\"\n回响的发表者是: %s\n\n您可以在这找到此文章: %s\n\n您可以点这个连结取消订阅: %s\n");
@define('SUBSCRIPTION_TRACKBACK_MAIL', "您好 %s,\n\n您订阅的文章有了新的引用在 \"%s\", 标题是 \"%s\"\n引用的作者是: %s\n\n您可以在这找到此文章: %s\n\n您可以点这个连结取消订阅: %s\n");
@define('SIGNATURE', "\n-- \n%s is powered by Serendipity.\nThe best blog around, you can use it too.\nCheck out <http://s9y.org> to find out how.");
@define('SYNDICATION_PLUGIN_091', 'RSS 0.91 feed');
@define('SYNDICATION_PLUGIN_10', 'RSS 1.0 feed');
@define('SYNDICATION_PLUGIN_20', 'RSS 2.0 feed');
@define('SYNDICATION_PLUGIN_20c', 'RSS 2.0 comments');
@define('SYNDICATION_PLUGIN_ATOM03', 'ATOM 0.3 feed');
@define('SYNDICATION_PLUGIN_MANAGINGEDITOR', '栏位 "managingEditor"');
@define('SYNDICATION_PLUGIN_WEBMASTER',  '栏位 "webMaster"');
@define('SYNDICATION_PLUGIN_BANNERURL', 'RSS feed 的图片');
@define('SYNDICATION_PLUGIN_BANNERWIDTH', '图片宽度');
@define('SYNDICATION_PLUGIN_BANNERHEIGHT', '图片高度');
@define('SYNDICATION_PLUGIN_WEBMASTER_DESC',  '管理员的电子邮件, 如果有. (空白: 隐藏) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_MANAGINGEDITOR_DESC', '作者的电子邮件, 如果有. (空白: 隐藏) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_BANNERURL_DESC', '图片的位址 URL, 以 GIF/JPEG/PNG 格式, 如果有. (空白: serendipity-logo)');
@define('SYNDICATION_PLUGIN_BANNERWIDTH_DESC', '像素, 最大. 144');
@define('SYNDICATION_PLUGIN_BANNERHEIGHT_DESC', '像素, 最大. 400');
@define('SYNDICATION_PLUGIN_TTL', '栏位 "ttl" (time-to-live)');
@define('SYNDICATION_PLUGIN_TTL_DESC', '过几分钟后您的文章不会被外来的网站或程式储存到快取记忆里 (空白: 隐藏) [RSS 2.0]');
@define('SYNDICATION_PLUGIN_PUBDATE', '栏位 "pubDate"');
@define('SYNDICATION_PLUGIN_PUBDATE_DESC', '"pubDate"-栏位需要内嵌到RSS-频道, 以显示最后文章的日期吗?');
@define('CONTENT', '内容');
@define('TYPE', '类型');
@define('DRAFT', '草稿');
@define('PUBLISH', '公开');
@define('PREVIEW', '预览');
@define('DATE', '日期');
@define('DATE_FORMAT_2', 'Y-m-d H:i'); // Needs to be ISO 8601 compliant for date conversion!
@define('DATE_INVALID', '警告: 您提供的日期不正确. 它必须是 YYYY-MM-DD HH:MM 格式.');
@define('CATEGORY_PLUGIN_DESC', '显示类别清单.');
@define('ALL_AUTHORS', '全部作者');
@define('CATEGORIES_TO_FETCH', '显示类别');
@define('CATEGORIES_TO_FETCH_DESC', '显示哪位作者的类别?');
@define('PAGE_BROWSE_ENTRIES', '页数 %s 共 %s, 总共 %s 篇文章');
@define('PREVIOUS_PAGE', '上一页');
@define('NEXT_PAGE', '下一页');
@define('ALL_CATEGORIES', '全部类别');
@define('DO_MARKUP', '执行标记转换');
@define('GENERAL_PLUGIN_DATEFORMAT', '日期格式化');
@define('GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH', '文章的日期格式, 使用 PHP 的 strftime() 变数. (预设: "%s")');
@define('ERROR_TEMPLATE_FILE', '无法开启布景档案, 请更新 serendipity!');
@define('ADVANCED_OPTIONS', '进阶选项');
@define('EDIT_ENTRY', '编辑文章');
@define('HTACCESS_ERROR', '要检查您的主机安装设定, serendipity 需要读写档案 ".htaccess". 但是因为权限错误, 没有办法为您检查. 请改变档案权限像这样: <br />&nbsp;&nbsp;%s<br />然后重新刷新这个网页.');
@define('SIDEBAR_PLUGINS', '侧列外挂');
@define('EVENT_PLUGINS', '事件外挂');
@define('SORT_ORDER', '排序');
@define('SORT_ORDER_NAME', '档案名称');
@define('SORT_ORDER_EXTENSION', '副档名');
@define('SORT_ORDER_SIZE', '档案大小');
@define('SORT_ORDER_WIDTH', '图片宽度');
@define('SORT_ORDER_HEIGHT', '图片长度');
@define('SORT_ORDER_DATE', '上传日期');
@define('SORT_ORDER_ASC', '递增排序');
@define('SORT_ORDER_DESC', '递减排序');
@define('THUMBNAIL_SHORT', '缩图');
@define('ORIGINAL_SHORT', '原始');
@define('APPLY_MARKUP_TO', '套用标记到 %s');
@define('CALENDAR_BEGINNING_OF_WEEK', '一周的第一天');
@define('SERENDIPITY_NEEDS_UPGRADE', 'Serendipity 侦测到您的配置版本是 %s, 但是 Serendipity 本身的安装版本是 %s, 请更新您的程式版本! <a href="%s">更新</a>');
@define('SERENDIPITY_UPGRADER_WELCOME', '您好, 欢迎来到 Serendipity 的更新系统.');
@define('SERENDIPITY_UPGRADER_PURPOSE', '更新系统会帮您更新到 Serendipity 版本 %s.');
@define('SERENDIPITY_UPGRADER_WHY', '您看到这个讯息是因为您安装了 Serendipity 版本 %s, 但是您没有更新资料库');
@define('SERENDIPITY_UPGRADER_DATABASE_UPDATES', '资料库更新 (%s)');
@define('SERENDIPITY_UPGRADER_FOUND_SQL_FILES', '系统找到以下的 .sql 档, 那些档案必须先执行才能继续安装 Serendipity');
@define('SERENDIPITY_UPGRADER_VERSION_SPECIFIC',  '特定的版本任务');
@define('SERENDIPITY_UPGRADER_NO_VERSION_SPECIFIC', '没有特定的版本任务');
@define('SERENDIPITY_UPGRADER_PROCEED_QUESTION', '确定要执行上面的任务吗?');
@define('SERENDIPITY_UPGRADER_PROCEED_ABORT', '我自己执行');
@define('SERENDIPITY_UPGRADER_PROCEED_DOIT', '请帮我执行');
@define('SERENDIPITY_UPGRADER_NO_UPGRADES', '您不需要进行任何更新');
@define('SERENDIPITY_UPGRADER_CONSIDER_DONE', '假装 Serendipity 更新完成吧');
@define('SERENDIPITY_UPGRADER_YOU_HAVE_IGNORED', '您略过了更新任务, 请确定资料库已安装完成, 和其他的任务安装无误');
@define('SERENDIPITY_UPGRADER_NOW_UPGRADED', '您的 Serendipity 以更新至版本 %s');
@define('SERENDIPITY_UPGRADER_RETURN_HERE', '您可以点 %s这里%s 返回日记首页');
@define('MANAGE_USERS', '管理作者');
@define('CREATE_NEW_USER', '新增作者');
@define('CREATE_NOT_AUTHORIZED', '您不能修改跟您相同权限的作者');
@define('CREATE_NOT_AUTHORIZED_USERLEVEL', '您不能新增比您高权限的作者');
@define('CREATED_USER', '新作者 %s 已经新增');
@define('MODIFIED_USER', '作者 %s 的资料已经更改');
@define('USER_LEVEL', '作者权限');
@define('DELETE_USER', '您要删除这个作者 #%d %s. 确定吗? 这会在主页隐藏他所写的任何文章.');
@define('DELETED_USER', '作者 #%d %s 已被删除.');
@define('LIMIT_TO_NUMBER', '要显示多少项目?');
@define('ENTRIES_PER_PAGE', '每页显示的文章');
@define('XML_IMAGE_TO_DISPLAY', 'XML 按钮');
@define('XML_IMAGE_TO_DISPLAY_DESC','连结到 XML Feeds 的都会用这个图片表示. 不填写将会使用预设的图片, 或输入 \'none\' 关闭这个功能.');

@define('DIRECTORIES_AVAILABLE', '您可以在子目录内点任何目录来建立新的目录.');
@define('ALL_DIRECTORIES', '全部目绿');
@define('MANAGE_DIRECTORIES', '管理目录');
@define('DIRECTORY_CREATED', '目录 <strong>%s</strong> 已经新增.');
@define('PARENT_DIRECTORY', '母目录');
@define('CONFIRM_DELETE_DIRECTORY', '确定要删除这个目录内的全部内容吗 %s?');
@define('ERROR_NO_DIRECTORY', '错误: 目录 %s 不存在');
@define('CHECKING_DIRECTORY', '检查此目录的档案 %s');
@define('DELETING_FILE', '删除档案 %s...');
@define('ERROR_DIRECTORY_NOT_EMPTY', '不能移除未清空的目录. 勾选 "强制删除" 核取方块如果您要移除这些档案, 然后在继续. 存在的档案是:');
@define('DIRECTORY_DELETE_FAILED', '不能删除目录 %s. 请检查权限或看上面的讯息.');
@define('DIRECTORY_DELETE_SUCCESS', '目录 %s 成功删除.');
@define('SKIPPING_FILE_EXTENSION', '略过档案: 没有 %s 的副档名.');
@define('SKIPPING_FILE_UNREADABLE', '略过档案: %s 不能读取.');
@define('FOUND_FILE', '找到 新/修改 过的档案: %s.');
@define('ALREADY_SUBCATEGORY', '%s 已经是此类别的子类别 %s.');
@define('PARENT_CATEGORY', '母类别');
@define('IN_REPLY_TO', '回覆到');
@define('TOP_LEVEL', '最高层');
@define('SYNDICATION_PLUGIN_GENERIC_FEED', '%s feed');
@define('PERMISSIONS', '权限');
@define('SETTINGS_SAVED_AT', '新设定已经被储存到 %s');

/* DATABASE SETTINGS */
@define('INSTALL_CAT_DB', '资料库设定');
@define('INSTALL_CAT_DB_DESC', '您可以在这输入全部的资料库资料. Serendipity 需要这些资料才能正常运作');
@define('INSTALL_DBTYPE', '资料库类型');
@define('INSTALL_DBTYPE_DESC', '资料库类型');
@define('INSTALL_DBHOST', '资料库主机');
@define('INSTALL_DBHOST_DESC', '资料库主机名称');
@define('INSTALL_DBUSER', '资料库帐号');
@define('INSTALL_DBUSER_DESC', '登入资料库的帐号');
@define('INSTALL_DBPASS', '资料库密码');
@define('INSTALL_DBPASS_DESC', '您的资料库密码');
@define('INSTALL_DBNAME', '资料库名称');
@define('INSTALL_DBNAME_DESC', '资料库名称');
@define('INSTALL_DBPREFIX', '资料表前置名称');
@define('INSTALL_DBPREFIX_DESC', '资料表的前置名称, 例如 serendipity_');

/* PATHS */
@define('INSTALL_CAT_PATHS', '路径设定');
@define('INSTALL_CAT_PATHS_DESC', '给档案夹的路径. 不要忘了最后的斜线!');
@define('INSTALL_FULLPATH', '完全路径');
@define('INSTALL_FULLPATH_DESC', '您的 Serendipity 安装的完全路径和绝对路径');
@define('INSTALL_UPLOADPATH', '上传路径');
@define('INSTALL_UPLOADPATH_DESC', '全部的上传档案会存到这里, 以 \'完全路径\' 表示的相对路径 - 例如 \'uploads/\'');
@define('INSTALL_RELPATH', '相对路径');
@define('INSTALL_RELPATH_DESC', '给浏览器的路径, 例如 \'/serendipity/\'');
@define('INSTALL_RELTEMPLPATH', '相对的布景路径');
@define('INSTALL_RELTEMPLPATH_DESC', '您放布景的路径 - 以 \'相对路径\' 表示的相对路径');
@define('INSTALL_RELUPLOADPATH', '相对的上传路径');
@define('INSTALL_RELUPLOADPATH_DESC', '给浏览器上传档案的路径 - 以 \'相对路径\' 表示的相对路径');
@define('INSTALL_URL', '网路日记 URL');
@define('INSTALL_URL_DESC', '您的 Serendipity 安装的基本 URL');
@define('INSTALL_INDEXFILE', 'Index 档案');
@define('INSTALL_INDEXFILE_DESC', 'Serendipity 的 index 档案');

/* Generel settings */
@define('INSTALL_CAT_SETTINGS', '一般设定');
@define('INSTALL_CAT_SETTINGS_DESC', 'Serendipity 的一般设定');
@define('INSTALL_USERNAME', '管理员帐号');
@define('INSTALL_USERNAME_DESC', '管理员的登入名称');
@define('INSTALL_PASSWORD', '管理员密码');
@define('INSTALL_PASSWORD_DESC', '管理员的登入密码');
@define('INSTALL_EMAIL', '电子邮件');
@define('INSTALL_EMAIL_DESC', '管理员的电子邮件');
@define('INSTALL_SENDMAIL', '寄送电子邮件给管理员?');
@define('INSTALL_SENDMAIL_DESC', '当有人发布回响到您的文章时要收到电子邮件通知吗?');
@define('INSTALL_SUBSCRIBE', '允许使用者订阅文章?');
@define('INSTALL_SUBSCRIBE_DESC', '您可以允许使用者收到电子邮件通知, 当有回响发布时她们会收到通知.');
@define('INSTALL_BLOGNAME', '日记名称');
@define('INSTALL_BLOGNAME_DESC', '您的日记标题');
@define('INSTALL_BLOGDESC', '日记简介');
@define('INSTALL_BLOGDESC_DESC', '介绍您的日记');
@define('INSTALL_LANG', '语系');
@define('INSTALL_LANG_DESC', '您日记使用的语系');

/* Appearance and options */
@define('INSTALL_CAT_DISPLAY', '外观及选项设定');
@define('INSTALL_CAT_DISPLAY_DESC', '让您设定 Serendipity 的外观和其他设定');
@define('INSTALL_WYSIWYG', '使用 WYSIWYG 编辑器');
@define('INSTALL_WYSIWYG_DESC', '您要使用 WYSIWYG 编辑器吗? (可在 IE5+ 使用, 某些部分可使用於 Mozilla 1.3+)');
@define('INSTALL_XHTML11', '强制符合 XHTML 1.1 要求');
@define('INSTALL_XHTML11_DESC', '您可以让您的日记强制符合 XHTML 1.1 的要求 (对旧的浏览器可能会有 后台/前台 的问题)');
@define('INSTALL_POPUP', '使用弹出视窗');
@define('INSTALL_POPUP_DESC', '您要在回响, 引用等地方使用弹出视窗吗?');
@define('INSTALL_EMBED', '使用内嵌功能?');
@define('INSTALL_EMBED_DESC', '如果你要将 Serendipity 以内嵌的方式放到网页内, 选择 是 会让您舍弃任何标题然后只显示日记内容. 您可以用 indexFile 设定来使用包装函式类别以便您放入网页标题. 详情请查询 README 档案!');
@define('INSTALL_TOP_AS_LINKS', '以连结显示 主要出源/主要来源?');
@define('INSTALL_TOP_AS_LINKS_DESC', '"否": 出源和来源将用文字显示以避免 google 的广告. "是": 出源和来源将用连结显示. "预设": 用全区里面的设定 (建议).');
@define('INSTALL_BLOCKREF', '阻挡来源');
@define('INSTALL_BLOCKREF_DESC', '有任何特殊的主机您不想在来源里显示吗? 用 \';\' 来分开主机名称, 注意主机是以子字串方式阻挡!');
@define('INSTALL_REWRITE', 'URL Rewriting');
@define('INSTALL_REWRITE_DESC', '请选择您想用的 URL Rewriting 方式. 开启 rewrite 规则会以比较清楚的方式显示 URL, 以便搜寻网站能正确的登入您的文章. 您的主机必须支援 mod_rewrite 或 "AllowOverride All" 到您的 Serendipity 档案夹. 预设的设定是系统自动帮您侦测的');

/* Imageconversion Settings */
@define('INSTALL_CAT_IMAGECONV', '图片转换设定');
@define('INSTALL_CAT_IMAGECONV_DESC', '请设定 Serendipity 设定图片转换的方式');
@define('INSTALL_IMAGEMAGICK', '使用 Imagemagick');
@define('INSTALL_IMAGEMAGICK_DESC', '如果有安装 image magick, 您要用它来改变图片大小吗?');
@define('INSTALL_IMAGEMAGICKPATH', '转换程式路径');
@define('INSTALL_IMAGEMAGICKPATH_DESC', 'image magick 转换程式的完全路径和名称');
@define('INSTALL_THUMBSUFFIX', '缩图后置字元');
@define('INSTALL_THUMBSUFFIX_DESC', '缩图会以下面的格式重新命名: original.[后置字元].ext');
@define('INSTALL_THUMBWIDTH', '缩图尺度');
@define('INSTALL_THUMBWIDTH_DESC', '自动建立缩图的最大宽度');

/* Personal details */
@define('USERCONF_CAT_PERSONAL', '个人资料设定');
@define('USERCONF_CAT_PERSONAL_DESC', '改变您的个人资料');
@define('USERCONF_USERNAME', '您的帐号');
@define('USERCONF_USERNAME_DESC', '您登入网路日记的名称');
@define('USERCONF_PASSWORD', '您的密码');
@define('USERCONF_PASSWORD_DESC', '您登入网路日记的密码');
@define('USERCONF_EMAIL', '您的电子邮件');
@define('USERCONF_EMAIL_DESC', '您使用的电子邮件');
@define('USERCONF_SENDCOMMENTS', '寄送回响通知?');
@define('USERCONF_SENDCOMMENTS_DESC', '当有新回响到您的文章时要通知您吗?');
@define('USERCONF_SENDTRACKBACKS', '寄送引用通知?');
@define('USERCONF_SENDTRACKBACKS_DESC', '当有新引用到您的文章时要通知您吗?');
@define('USERCONF_ALLOWPUBLISH', '权限: 可发布文章?');
@define('USERCONF_ALLOWPUBLISH_DESC', '允许这位作者发布文章吗?');
@define('SUCCESS', '完成');
@define('POWERED_BY_SHOW_TEXT', '以文字显示 "Serendipity"');
@define('POWERED_BY_SHOW_TEXT_DESC', '将用文字显示 "Serendipity Weblog"');
@define('POWERED_BY_SHOW_IMAGE', '以 logo 显示 "Serendipity"');
@define('POWERED_BY_SHOW_IMAGE_DESC', '显示 Serendipity 的 logo');
@define('PLUGIN_ITEM_DISPLAY', '该项目的显示位址?');
@define('PLUGIN_ITEM_DISPLAY_EXTENDED', '只在副内容显示');
@define('PLUGIN_ITEM_DISPLAY_OVERVIEW', '只在概观内显示');
@define('PLUGIN_ITEM_DISPLAY_BOTH', '永远显示');

@define('COMMENTS_WILL_BE_MODERATED', '发布的回响将需要管理员的审核.');
@define('YOU_HAVE_THESE_OPTIONS', '您有以下选择:');
@define('THIS_COMMENT_NEEDS_REVIEW', '警告: 这个回响须审核才会显示');
@define('DELETE_COMMENT', '删除回响');
@define('APPROVE_COMMENT', '认可回响');
@define('REQUIRES_REVIEW', '需要审核');
@define('COMMENT_APPROVED', '回响 #%s 已经通过审核');
@define('COMMENT_DELETED', '回响 #%s 已经成功删除');
@define('COMMENTS_MODERATE', '回响和引用到这个文章需要管理员的审核');
@define('THIS_TRACKBACK_NEEDS_REVIEW', '警告: 这个引用需要管理员的审核才会显示');
@define('DELETE_TRACKBACK', '删除引用');
@define('APPROVE_TRACKBACK', '认可引用');
@define('TRACKBACK_APPROVED', '引用 #%s 已经通过审核');
@define('TRACKBACK_DELETED', '引用 #%s 已经成功删除');
@define('VIEW', '浏览');
@define('COMMENT_ALREADY_APPROVED', '回响 #%s 已经通过审核');
@define('COMMENT_EDITED', '文章已被编辑');
@define('HIDE', '隐藏');
@define('VIEW_EXTENDED_ENTRY', '继续阅读 "%s"');
@define('TRACKBACK_SPECIFIC_ON_CLICK', '这个连结不是用来点的. 它包含了这个文章的引用 URI. 您可以从您的日记内用这个 URI 来传送 ping 和引用到这个文章. 如果要复制这个连结, 在连结上点右键然后选择 "复制连结" (IE) 或 "复制连结位址" (Mozilla).');
@define('PLUGIN_SUPERUSER_HTTPS', '用 https 登入');
@define('PLUGIN_SUPERUSER_HTTPS_DESC', '让登入连结连到 https 网址. 您的主机必须支援这项功能!');
@define('INSTALL_SHOW_EXTERNAL_LINKS', '让外来连结以连结显示?');
@define('INSTALL_SHOW_EXTERNAL_LINKS_DESC', '"否": 外来连结 (主要出源, 主要来源, 回响) 都不会以文字显示以避免 google 广告 (建议使用). "是": 外来连结将以超连结的方式显示. 可以在侧列外挂盖过此设定!');
@define('PAGE_BROWSE_COMMENTS', '页数 %s 共 %s, 总共 %s 个回响');
@define('FILTERS', '过滤');
@define('FIND_ENTRIES', '搜寻文章');
@define('FIND_COMMENTS', '搜寻回响');
@define('FIND_MEDIA', '搜寻媒体');
@define('FILTER_DIRECTORY', '目录');
@define('SORT_BY', '排序');
@define('TRACKBACK_COULD_NOT_CONNECT', '没有送出引用: 无法开启线路到 %s 用连接埠 %d');
@define('MEDIA', '媒体');
@define('MEDIA_LIBRARY', '媒体存库');
@define('ADD_MEDIA', '新增媒体');
@define('ENTER_MEDIA_URL', '请输入档案的 URL:');
@define('ENTER_MEDIA_UPLOAD', '请选择要上传的档案:');
@define('SAVE_FILE_AS', '储存档案:');
@define('STORE_IN_DIRECTORY', '储存到以下目录: ');
@define('ADD_MEDIA_BLAHBLAH', '<b>新增档案到媒体存库:</b><p>您可以在这上传媒体档, 或告诉系统到哪寻找! 如果您没有想要的图片, 您可以到 <a href="http://images.google.com" target="_blank">google寻找图片</a>.<p><b>选择方式:</b><br>');
@define('MEDIA_RENAME', '更改档案名称');
@define('IMAGE_RESIZE', '更改图片尺寸');
@define('MEDIA_DELETE', '删除这个档案');
@define('FILES_PER_PAGE', '每页显示的档案数');
@define('CLICK_FILE_TO_INSERT', '点选您要输入的档案:');
@define('SELECT_FILE', '选择要输入的档案');
@define('MEDIA_FULLSIZE', '完整尺寸');
@define('CALENDAR_BOW_DESC', '一个礼拜的第一天. 预设是星期一');
@define('SUPERUSER', '日记管理');
@define('ALLOWS_YOU_BLAHBLAH', '在侧列提供连结到日记管理');
@define('CALENDAR', '日历');
@define('SUPERUSER_OPEN_ADMIN', '开启管理页面');
@define('SUPERUSER_OPEN_LOGIN', '开启登入页面');
@define('INVERT_SELECTIONS', '颠倒勾选');
@define('COMMENTS_DELETE_CONFIRM', '确定要删除勾选的回响吗?');
@define('COMMENT_DELETE_CONFIRM', '确定要删除回响 #%d, 发布者是 %s?');
@define('DELETE_SELECTED_COMMENTS', '删除勾选的回响');
@define('VIEW_COMMENT', '浏览回响');
@define('VIEW_ENTRY', '浏览文章');
@define('DELETE_FILE_FAIL' , '无法删除档案 <b>%s</b>');
@define('DELETE_THUMBNAIL', '删除了图片缩图 <b>%s</b>');
@define('DELETE_FILE', '删除了档案 <b>%s</b>');
@define('ABOUT_TO_DELETE_FILE', '您将删除档案 <b>%s</b><br />如果您有在其他的文章内使用这个档案, 那个连结或图片将会无效<br />确定要继续吗?<br /><br />');
@define('TRACKBACK_SENDING', '传送引用到 URI %s...');
@define('TRACKBACK_SENT', '引用完成');
@define('TRACKBACK_FAILED', '引用错误: %s');
@define('TRACKBACK_NOT_FOUND', '找不到引用的URI.');
@define('TRACKBACK_URI_MISMATCH', '自动搜寻的引用跟引用目标不相同.');
@define('TRACKBACK_CHECKING', '搜寻 <u>%s</u> 的引用...');
@define('TRACKBACK_NO_DATA', '目标没有任何资料');
@define('TRACKBACK_SIZE', '目标 URI 超出了允许的 %s bytes 档案大小.');
@define('COMMENTS_VIEWMODE_THREADED', '分线程');
@define('COMMENTS_VIEWMODE_LINEAR', '直线程');
@define('DISPLAY_COMMENTS_AS', '回响显示方式');
@define('COMMENTS_FILTER_SHOW', '显示');
@define('COMMENTS_FILTER_ALL', '全部');
@define('COMMENTS_FILTER_APPROVED_ONLY', '显示审核回响');
@define('COMMENTS_FILTER_NEED_APPROVAL', '显示等待审核');
@define('RSS_IMPORT_BODYONLY', '将输入的文字放到主内容, 将不拆开过长的文章到副内容地区.');
@define('SYNDICATION_PLUGIN_FULLFEED', '在 RSS feed 里显示全部的文章');
@define('WEEK', '周');
@define('WEEKS', '周');
@define('MONTHS', '月');
@define('DAYS', '日');
@define('ARCHIVE_FREQUENCY', '保存文库的项目频率');
@define('ARCHIVE_FREQUENCY_DESC', '保存文库使用的项目清单间隔');
@define('ARCHIVE_COUNT', '保存文库的项目数');
@define('ARCHIVE_COUNT_DESC', '显示的月, 周, 或日');
@define('BELOW_IS_A_LIST_OF_INSTALLED_PLUGINS', '下面是安装好的外挂');
@define('SIDEBAR_PLUGIN', '侧列外挂');
@define('EVENT_PLUGIN', '事件外挂');
@define('CLICK_HERE_TO_INSTALL_PLUGIN', '点这里安装新 %s');
@define('VERSION', '版本');
@define('INSTALL', '安装');
@define('ALREADY_INSTALLED', '已经安装');
@define('SELECT_A_PLUGIN_TO_ADD', '请选择要安装的外挂');
@define('RSS_IMPORT_CATEGORY', '用这个类别给不相同的输入文章');

@define('INSTALL_OFFSET', 'Server time Offset'); // Translate
@define('STICKY_POSTINGS', 'Sticky Postings'); // Translate
@define('INSTALL_FETCHLIMIT', 'Entries to display on frontpage'); // Translate
@define('INSTALL_FETCHLIMIT_DESC', 'Number of entries to display for each page on the frontend'); // Translate
@define('IMPORT_ENTRIES', 'Import data'); // Translate
@define('EXPORT_ENTRIES', 'Export entries'); // Translate
@define('IMPORT_WELCOME', 'Welcome to the Serendipity import utility'); // Translate
@define('IMPORT_WHAT_CAN', 'Here you can import entries from other weblog software applications'); // Translate
@define('IMPORT_SELECT', 'Please select the software you wish to import from'); // Translate
@define('IMPORT_PLEASE_ENTER', 'Please enter the data as requested below'); // Translate
@define('IMPORT_NOW', 'Import now!'); // Translate
@define('IMPORT_STARTING', 'Starting import procedure...'); // Translate
@define('IMPORT_FAILED', 'Import failed'); // Translate
@define('IMPORT_DONE', 'Import successfully completed'); // Translate
@define('IMPORT_WEBLOG_APP', 'Weblog application'); // Translate
@define('EXPORT_FEED', 'Export full RSS feed'); // Translate
@define('STATUS', 'Status after import'); // Translate
@define('IMPORT_GENERIC_RSS', 'Generic RSS import'); // Translate
@define('ACTIVATE_AUTODISCOVERY', 'Send Trackbacks to links found in the entry'); // Translate
@define('WELCOME_TO_ADMIN', 'Welcome to the Serendipity Administration Suite.'); // Translate
@define('PLEASE_ENTER_CREDENTIALS', 'Please enter your credentials below.'); // Translate
@define('ADMIN_FOOTER_POWERED_BY', 'Powered by Serendipity %s and PHP %s'); // Translate
@define('INSTALL_USEGZIP', 'Use gzip compressed pages'); // Translate
@define('INSTALL_USEGZIP_DESC', 'To speed up delivery of pages, we can compress the pages we send to the visitor, given that his browser supports this. This is recommended'); // Translate
@define('INSTALL_SHOWFUTURE', 'Show future entries'); // Translate
@define('INSTALL_SHOWFUTURE_DESC', 'If enabled, this will show all entries in the future on your blog. Default is to hide those entries and only show them if the publish date has arrived.'); // Translate
@define('INSTALL_DBPERSISTENT', 'Use persistent connections'); // Translate
@define('INSTALL_DBPERSISTENT_DESC', 'Enable the usage of persistent database connections, read more <a href="http://php.net/manual/features.persistent-connections.php" target="_blank">here</a>. This is normally not recommended'); // Translate
@define('NO_IMAGES_FOUND', 'No images found'); // Translate
@define('PERSONAL_SETTINGS', 'Personal Settings'); // Translate
@define('REFERER', 'Referer'); // Translate
@define('NOT_FOUND', 'Not found'); // Translate
@define('NOT_WRITABLE', 'Not writable'); // Translate
@define('WRITABLE', 'Writable'); // Translate
@define('PROBLEM_DIAGNOSTIC', 'Due to a problematic diagnostic, you cannot continue with the installation before the above errors are fixed'); // Translate
@define('SELECT_INSTALLATION_TYPE', 'Select which installation type you wish to use'); // Translate
@define('WELCOME_TO_INSTALLATION', 'Welcome to the Serendipity Installation'); // Translate
@define('FIRST_WE_TAKE_A_LOOK', 'First we will take a look at your current setup and attempt to diagnose any compatibility problems'); // Translate
@define('ERRORS_ARE_DISPLAYED_IN', 'Errors are displayed in %s, recommendations in %s and success in %s'); // Translate
@define('RED', 'red'); // Translate
@define('YELLOW', 'yellow'); // Translate
@define('GREEN', 'green'); // Translate
@define('PRE_INSTALLATION_REPORT', 'Serendipity v%s pre-installation report'); // Translate
@define('RECOMMENDED', 'Recommended'); // Translate
@define('ACTUAL', 'Actual'); // Translate
@define('PHPINI_CONFIGURATION', 'php.ini configuration'); // Translate
@define('PHP_INSTALLATION', 'PHP installation'); // Translate
@define('THEY_DO', 'they do'); // Translate
@define('THEY_DONT', 'they don\'t'); // Translate
@define('SIMPLE_INSTALLATION', 'Simple installation'); // Translate
@define('EXPERT_INSTALLATION', 'Expert installation'); // Translate
@define('COMPLETE_INSTALLATION', 'Complete installation'); // Translate
@define('WONT_INSTALL_DB_AGAIN', 'won\'t install the database again'); // Translate
@define('CHECK_DATABASE_EXISTS', 'Checking to see if the database and tables already exists'); // Translate
@define('CREATING_PRIMARY_AUTHOR', 'Creating primary author \'%s\''); // Translate
@define('SETTING_DEFAULT_TEMPLATE', 'Setting default template'); // Translate
@define('INSTALLING_DEFAULT_PLUGINS', 'Installing default plugins'); // Translate
@define('SERENDIPITY_INSTALLED', 'Serendipity has been successfully installed'); // Translate
@define('VISIT_BLOG_HERE', 'Visit your new blog here'); // Translate
@define('THANK_YOU_FOR_CHOOSING', 'Thank you for choosing Serendipity'); // Translate
@define('ERROR_DETECTED_IN_INSTALL', 'An error was detected in the installation'); // Translate
@define('OPERATING_SYSTEM', 'Operating system'); // Translate
@define('WEBSERVER_SAPI', 'Webserver SAPI'); // Translate
@define('TEMPLATE_SET', '\'%s\' has been set as your active template'); // Translate
@define('SEARCH_ERROR', 'The search function did not work as expected. Notice for the administrator of this blog: This may happen because of missing index keys in your database. On MySQL systems your database user account needs to be privileged to execute this query: <pre>CREATE FULLTEXT INDEX entry_idx on %sentries (title,body,extended)</pre> The specific error returned by the database was: <pre>%s</pre>'); // Translate
@define('EDIT_THIS_CAT', 'Editing "%s"'); // Translate
@define('CATEGORY_REMAINING', 'Delete this category and move its entries to this category'); // Translate
@define('CATEGORY_INDEX', 'Below is a list of categories available to your entries'); // Translate
@define('NO_CATEGORIES', 'No categories'); // Translate
@define('RESET_DATE', 'Reset date'); // Translate
@define('RESET_DATE_DESC', 'Click here to reset the date to the current time'); // Translate
@define('PROBLEM_PERMISSIONS_HOWTO', 'Permissions can be set by running shell command: `<em>%s</em>` on the failed directory, or by setting this using an FTP program'); // Translate
@define('WARNING_TEMPLATE_DEPRECATED', 'Warning: Your current template is using a deprecated template method, you are advised to update if possible'); // Translate
@define('ENTRY_PUBLISHED_FUTURE', 'This entry is not yet published.'); // Translate
@define('ENTRIES_BY', 'Entries by %s'); // Translate
@define('PREVIOUS', 'Previous'); // Translate
@define('NEXT', 'Next'); // Translate
@define('APPROVE', 'Approve'); // Translate
@define('DO_MARKUP_DESCRIPTION', 'Apply markup transformations to the text (smilies, shortcut markups via *, /, _, ...). Disabling this will preserve any HTML-code in the text.'); // Translate
@define('CATEGORY_ALREADY_EXIST', 'A category with the name "%s" already exist'); // Translate
@define('IMPORT_NOTES', 'Note:'); // Translate
@define('ERROR_FILE_FORBIDDEN', 'You are not allowed to upload files with active content'); // Translate
@define('ADMIN', 'Administration'); // Re-Translate
@define('ADMIN_FRONTPAGE', 'Frontpage'); // Translate
@define('QUOTE', 'Quote'); // Translate
@define('IFRAME_SAVE', 'Serendipity is now saving your entry, creating trackbacks and performing possible XML-RPC calls. This may take a while..'); // Translate
@define('IFRAME_SAVE_DRAFT', 'A draft of this entry has been saved'); // Translate
@define('IFRAME_PREVIEW', 'Serendipity is now creating the preview of your entry...'); // Translate
@define('IFRAME_WARNING', 'Your browser does not support the concept of iframes. Please open your serendipity_config.inc.php file and set $serendipity[\'use_iframe\'] variable to FALSE.'); // Translate
@define('NONE', 'none');
@define('USERCONF_CAT_DEFAULT_NEW_ENTRY', 'Default settings for new entries'); // Translate
@define('UPGRADE', 'Upgrade'); // Translate
@define('UPGRADE_TO_VERSION', 'Upgrade to version %s'); // Translate
@define('DELETE_DIRECTORY', 'Delete directory'); // Translate
@define('DELETE_DIRECTORY_DESC', 'You are about to delete the contents of a directory that contains media files, possibly files used in some of your entries.'); // Translate
@define('FORCE_DELETE', 'Delete ALL files in this directory, including those not known by Serendipity'); // Translate
@define('CREATE_DIRECTORY', 'Create directory'); // Translate
@define('CREATE_NEW_DIRECTORY', 'Create new directory'); // Translate
@define('CREATE_DIRECTORY_DESC', 'Here you can create a new directory to place media files in. Choose the name for your new directory and select an optional parent directory to place it in.'); // Translate
@define('BASE_DIRECTORY', 'Base directory'); // Translate
@define('USERLEVEL_EDITOR_DESC', 'Standard editor'); // Translate
@define('USERLEVEL_CHIEF_DESC', 'Chief editor'); // Translate
@define('USERLEVEL_ADMIN_DESC', 'Administrator'); // Translate
@define('USERCONF_USERLEVEL', 'Access level'); // Translate
@define('USERCONF_USERLEVEL_DESC', 'This level is used to determine what kind of access this user has to the blog'); // Translate
@define('USER_SELF_INFO', 'Logged in as %s (%s)'); // Translate
@define('ADMIN_ENTRIES', 'Entries'); // Translate
@define('RECHECK_INSTALLATION', 'Recheck installation'); // Translate
@define('IMAGICK_EXEC_ERROR', 'Unable to execute: "%s", error: %s, return var: %d'); // Translate
@define('INSTALL_OFFSET_DESC', 'Enter the amount of hours between the date of your webserver (current: %clock%) and your desired time zone'); // Translate
@define('UNMET_REQUIREMENTS', 'Requirements failed: %s'); // Translate
@define('CHARSET', 'Charset');
@define('AUTOLANG', 'Use visitor\'s browser language as default');
@define('AUTOLANG_DESC', 'If enabled, this will use the visitor\'s browser language setting to determine the default language of your entry and interface language.');
@define('INSTALL_AUTODETECT_URL', 'Autodetect used HTTP-Host'); // Translate
@define('INSTALL_AUTODETECT_URL_DESC', 'If set to "true", Serendipity will ensure that the HTTP Host which was used by your visitor is used as your BaseURL setting. Enabling this will let you be able to use multiple domain names for your Serendipity Blog, and use the domain for all follow-up links which the user used to access your blog.'); // Translate
@define('CONVERT_HTMLENTITIES', 'Try to auto-convert HTML entities?');
@define('EMPTY_SETTING', 'You did not specify a valid value for "%s"!');
@define('USERCONF_REALNAME', 'Real name'); // Translate
@define('USERCONF_REALNAME_DESC', 'The full name of the author. This is the name seen by readers'); // Translate
@define('HOTLINK_DONE', 'File hotlinked.<br />Done.'); // Translate
@define('ENTER_MEDIA_URL_METHOD', 'Fetch method:'); // Translate
@define('ADD_MEDIA_BLAHBLAH_NOTE', 'Note: If you choose to hotlink to server, make sure you have permission to hotlink to the designated website, or the website is yours. Hotlink allows you to use off-site images without storing them locally.'); // Translate
@define('MEDIA_HOTLINKED', 'hotlinked'); // Translate
@define('FETCH_METHOD_IMAGE', 'Download image to your server'); // Translate
@define('FETCH_METHOD_HOTLINK', 'Hotlink to server'); // Translate
@define('DELETE_HOTLINK_FILE', 'Deleted the hotlinked file entitled <b>%s</b>'); // Translate
@define('SYNDICATION_PLUGIN_SHOW_MAIL', 'Show E-Mail addresses?');
@define('IMAGE_MORE_INPUT', 'Add more images'); // Translate
@define('BACKEND_TITLE', 'Additional information in Plugin Configuration screen'); // Translate
@define('BACKEND_TITLE_FOR_NUGGET', 'Here you can define a custom string which is displayed in the Plugin Configuration screen together with the description of the HTML Nugget plugin. If you have multiple HTML nuggets with an empty title, this helps to distinct the plugins from another.'); // Translate
@define('CATEGORIES_ALLOW_SELECT', 'Allow visitors to display multiple categories at once?'); // Translate
@define('CATEGORIES_ALLOW_SELECT_DESC', 'If this option is enabled, a checkbox will be put next to each category in this sidebar plugin. Users can check those boxes and then see entries belonging to their selection.'); // Translate
@define('PAGE_BROWSE_PLUGINS', 'Page %s of %s, totalling %s plugins.');
@define('INSTALL_CAT_PERMALINKS', 'Permalinks');
@define('INSTALL_CAT_PERMALINKS_DESC', 'Defines various URL patterns to define permanent links in your blog. It is suggested that you use the defaults; if not, you should try to use the %id% value where possible to prevent Serendipity from querying the database to lookup the target URL.');
@define('INSTALL_PERMALINK', 'Permalink Entry URL structure');
@define('INSTALL_PERMALINK_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries may become available. You can use the variables %id%, %title%, %day%, %month%, %year% and any other characters.');
@define('INSTALL_PERMALINK_AUTHOR', 'Permalink Author URL structure');
@define('INSTALL_PERMALINK_AUTHOR_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries from certain authors may become available. You can use the variables %id%, %realname%, %username%, %email% and any other characters.');
@define('INSTALL_PERMALINK_CATEGORY', 'Permalink Category URL structure');
@define('INSTALL_PERMALINK_CATEGORY_DESC', 'Here you can define the relative URL structure begining from your base URL to where entries from certain categories may become available. You can use the variables %id%, %name%, %description% and any other characters.');
@define('INSTALL_PERMALINK_FEEDCATEGORY', 'Permalink RSS-Feed Category URL structure');
@define('INSTALL_PERMALINK_FEEDCATEGORY_DESC', 'Here you can define the relative URL structure begining from your base URL to where RSS-feeds frmo certain categories may become available. You can use the variables %id%, %name%, %description% and any other characters.');
@define('INSTALL_PERMALINK_ARCHIVESPATH', 'Path to archives');
@define('INSTALL_PERMALINK_ARCHIVEPATH', 'Path to archive');
@define('INSTALL_PERMALINK_CATEGORIESPATH', 'Path to categories');
@define('INSTALL_PERMALINK_UNSUBSCRIBEPATH', 'Path to unsubscribe comments');
@define('INSTALL_PERMALINK_DELETEPATH', 'Path to delete comments');
@define('INSTALL_PERMALINK_APPROVEPATH', 'Path to approve comments');
@define('INSTALL_PERMALINK_FEEDSPATH', 'Path to RSS Feeds');
@define('INSTALL_PERMALINK_PLUGINPATH', 'Path to single plugin');
@define('INSTALL_PERMALINK_ADMINPATH', 'Path to admin');
@define('INSTALL_PERMALINK_SEARCHPATH', 'Path to search');
@define('USERCONF_CREATE', 'Forbid creating entries?');
@define('USERCONF_CREATE_DESC', 'If selected, the user may not create new entries');
@define('INSTALL_CAL', 'Calendar Type');
@define('INSTALL_CAL_DESC', 'Choose your desired Calendar format');
@define('REPLY', 'Reply');
@define('USERCONF_GROUPS', 'Group Memberships');
@define('USERCONF_GROUPS_DESC', 'This user is a member of the following groups. Multiple memberships are possible.');
@define('MANAGE_GROUPS', 'Manage groups');
@define('DELETED_GROUP', 'Group #%d %s deleted.');
@define('CREATED_GROUP', 'A new group %s has been created');
@define('MODIFIED_GROUP', 'The properties of group %s have been changed');
@define('GROUP', 'Group');
@define('CREATE_NEW_GROUP', 'Create new group');
@define('DELETE_GROUP', 'You are about to delete group #%d %s. Are you serious?');
@define('USERLEVEL_OBSOLETE', 'NOTICE: The userlevel attribute is now only used for backward compatibility to plugins and fallback authorisation. User privileges are now handled by group memberships!');
@define('SYNDICATION_PLUGIN_FEEDBURNERID', 'FeedBurner ID');
@define('SYNDICATION_PLUGIN_FEEDBURNERID_DESC', 'The ID of the feed you wish to publish');
@define('SYNDICATION_PLUGIN_FEEDBURNERIMG', 'FeedBurner image');
@define('SYNDICATION_PLUGIN_FEEDBURNERIMG_DESC', 'Name of image to display (or leave blank for counter), located on feedburner.com, ex: fbapix.gif');
@define('SYNDICATION_PLUGIN_FEEDBURNERTITLE', 'FeedBurner title');
@define('SYNDICATION_PLUGIN_FEEDBURNERTITLE_DESC', 'Title (if any) to display alongside the image');
@define('SYNDICATION_PLUGIN_FEEDBURNERALT', 'FeedBurner image text');
@define('SYNDICATION_PLUGIN_FEEDBURNERALT_DESC', 'Text (if any) to display when hovering the image');
@define('SEARCH_TOO_SHORT', 'Your search-query must be longer than 3 characters. You can try to append * to shorter words, like: s9y* to trick the search into using shorter words.');
@define('INSTALL_DBPORT', 'Database port');
@define('INSTALL_DBPORT_DESC', 'The port used to connect to your database server');
@define('PLUGIN_GROUP_FRONTEND_EXTERNAL_SERVICES', 'Frontend: External Services');
@define('PLUGIN_GROUP_FRONTEND_FEATURES', 'Frontend: Features');
@define('PLUGIN_GROUP_FRONTEND_FULL_MODS', 'Frontend: Full Mods');
@define('PLUGIN_GROUP_FRONTEND_VIEWS', 'Frontend: Views');
@define('PLUGIN_GROUP_FRONTEND_ENTRY_RELATED', 'Frontend: Entry Related');
@define('PLUGIN_GROUP_BACKEND_EDITOR', 'Backend: Editor');
@define('PLUGIN_GROUP_BACKEND_USERMANAGEMENT', 'Backend: Usermanagement');
@define('PLUGIN_GROUP_BACKEND_METAINFORMATION', 'Backend: Meta information');
@define('PLUGIN_GROUP_BACKEND_TEMPLATES', 'Backend: Templates');
@define('PLUGIN_GROUP_BACKEND_FEATURES', 'Backend: Features');
@define('PLUGIN_GROUP_IMAGES', 'Images');
@define('PLUGIN_GROUP_ANTISPAM', 'Antispam');
@define('PLUGIN_GROUP_MARKUP', 'Markup');
@define('PLUGIN_GROUP_STATISTICS', 'Statistics');
@define('PERMISSION_PERSONALCONFIGURATION', 'personalConfiguration: Access personal configuration');
@define('PERMISSION_PERSONALCONFIGURATIONUSERLEVEL', 'personalConfigurationUserlevel: Change userlevels');
@define('PERMISSION_PERSONALCONFIGURATIONNOCREATE', 'personalConfigurationNoCreate: Change "forbid creating entries"');
@define('PERMISSION_PERSONALCONFIGURATIONRIGHTPUBLISH', 'personalConfigurationRightPublish: Change right to publish entries');
@define('PERMISSION_SITECONFIGURATION', 'siteConfiguration: Access system configuration');
@define('PERMISSION_BLOGCONFIGURATION', 'blogConfiguration: Access blog-centric configuration');
@define('PERMISSION_ADMINENTRIES', 'adminEntries: Administrate entries');
@define('PERMISSION_ADMINENTRIESMAINTAINOTHERS', 'adminEntriesMaintainOthers: Administrate other user\'s entries');
@define('PERMISSION_ADMINIMPORT', 'adminImport: Import entries');
@define('PERMISSION_ADMINCATEGORIES', 'adminCategories: Administrate categories');
@define('PERMISSION_ADMINCATEGORIESMAINTAINOTHERS', 'adminCategoriesMaintainOthers: Administrate other user\'s categories');
@define('PERMISSION_ADMINCATEGORIESDELETE', 'adminCategoriesDelete: Delete categories');
@define('PERMISSION_ADMINUSERS', 'adminUsers: Administrate users');
@define('PERMISSION_ADMINUSERSDELETE', 'adminUsersDelete: Delete users');
@define('PERMISSION_ADMINUSERSEDITUSERLEVEL', 'adminUsersEditUserlevel: Change userlevel');
@define('PERMISSION_ADMINUSERSMAINTAINSAME', 'adminUsersMaintainSame: Administrate users that are in your group(s)');
@define('PERMISSION_ADMINUSERSMAINTAINOTHERS', 'adminUsersMaintainOthers: Administrate users that are not in your group(s)');
@define('PERMISSION_ADMINUSERSCREATENEW', 'adminUsersCreateNew: Create new users');
@define('PERMISSION_ADMINUSERSGROUPS', 'adminUsersGroups: Administrate usergroups');
@define('PERMISSION_ADMINPLUGINS', 'adminPlugins: Administrate plugins');
@define('PERMISSION_ADMINPLUGINSMAINTAINOTHERS', 'adminPluginsMaintainOthers: Administrate other user\'s plugins');
@define('PERMISSION_ADMINIMAGES', 'adminImages: Administrate media files');
@define('PERMISSION_ADMINIMAGESDIRECTORIES', 'adminImagesDirectories: Administrate media directories');
@define('PERMISSION_ADMINIMAGESADD', 'adminImagesAdd: Add new media files');
@define('PERMISSION_ADMINIMAGESDELETE', 'adminImagesDelete: Delete media files');
@define('PERMISSION_ADMINIMAGESMAINTAINOTHERS', 'adminImagesMaintainOthers: Administrate other user\'s media files');
@define('PERMISSION_ADMINIMAGESVIEW', 'adminImagesView: View media files');
@define('PERMISSION_ADMINIMAGESSYNC', 'adminImagesSync: Sync thumbnails');
@define('PERMISSION_ADMINCOMMENTS', 'adminComments: Administrate comments');
@define('PERMISSION_ADMINTEMPLATES', 'adminTemplates: Administrate templates');
@define('INSTALL_BLOG_EMAIL', 'Blog\'s E-Mail address');
@define('INSTALL_BLOG_EMAIL_DESC', 'This configures the E-Mail address that is used as the "From"-Part of outgoing mails. Be sure to set this to an address that is recognized by the mailserver used on your host - many mailservers reject messages that have unknown From-addresses.');
@define('CATEGORIES_PARENT_BASE', 'Only show categories below...');
@define('CATEGORIES_PARENT_BASE_DESC', 'You can choose a parent category so that only the child categories are shown.');
@define('CATEGORIES_HIDE_PARALLEL', 'Hide categories that are not part of the category tree');
@define('CATEGORIES_HIDE_PARALLEL_DESC', 'If you want to hide categories that are part of a different category tree, you need to enable this. This feature makes most sense if used in conjunction with a multi-blog using the "Properties/Tempaltes of categories" plugin.');
@define('PERMISSION_ADMINIMAGESVIEWOTHERS', 'adminImagesViewOthers: View other user\'s media files');
@define('CHARSET_NATIVE', 'Native');
@define('INSTALL_CHARSET', 'Charset selection');
@define('INSTALL_CHARSET_DESC', 'Here you can toggle UTF-8 or native (ISO, EUC, ...) charactersets. Some languages only have UTF-8 translations so that setting the charset to "Native" will have no effects. UTF-8 is suggested for new installations. Do not change this setting if you have already made entries with special characters - this may lead to corrupt characters. Be sure to read more on http://www.s9y.org/index.php?node=46 about this issue.');
@define('CALENDAR_ENABLE_EXTERNAL_EVENTS', 'Enable Plugin API hook');
@define('CALENDAR_EXTEVENT_DESC', 'If enabled, plugins can hook into the calendar to display their own events highlighted. Only enable if you have installed plugins that need this, otherwise it just decreases performance.');
@define('XMLRPC_NO_LONGER_BUNDLED', 'The XML-RPC API Interface to Serendipity is no longer bundled because of ongoing security issues with this API and not many people using it. Thus you need to install the XML-RPC Plugin to use the XML-RPC API. The URL to use in your applications will NOT change - as soon as you have installed the plugin, you will again be able to use the API.');
@define('PERM_READ', 'Read permission');
@define('PERM_WRITE', 'Write permission');

@define('PERM_DENIED', 'Permission denied.');
@define('INSTALL_ACL', 'Apply read-permissions for categories');
@define('INSTALL_ACL_DESC', 'If enabled, the usergroup permission settings you setup for categories will be applied when logged-in users view your blog. If disabled, the read-permissions of the categories are NOT applied, but the positive effect is a little speedup on your blog. So if you don\'t need multi-user read permissions for your blog, disable this setting.');
@define('PLUGIN_API_VALIDATE_ERROR', 'Configuration syntax wrong for option "%s". Needs content of type "%s".');
@define('USERCONF_CHECK_PASSWORD', 'Old Password');
@define('USERCONF_CHECK_PASSWORD_DESC', 'If you change the password in the field above, you need to enter the current user password into this field.');
@define('USERCONF_CHECK_PASSWORD_ERROR', 'You did not specify the right old password, and are not authorized to change the new password. Your settings were not saved.');
@define('ERROR_XSRF', 'Your browser did not sent a valid HTTP-Referrer string. This may have either been caused by a misconfigured browser/proxy or by a Cross Site Request Forgery (XSRF) aimed at you. The action you requested could not be completed.');
@define('INSTALL_PERMALINK_FEEDAUTHOR_DESC', 'Here you can define the relative URL structure beginning from your base URL to where RSS-feeds from specific users may be viewed. You can use the variables %id%, %realname%, %username%, %email% and any other characters.');
@define('INSTALL_PERMALINK_FEEDAUTHOR', 'Permalink RSS-Feed Author URL structure');
@define('INSTALL_PERMALINK_AUTHORSPATH', 'Path to authors');
@define('AUTHORS', 'Authors');
@define('AUTHORS_ALLOW_SELECT', 'Allow visitors to display multiple authors at once?');
@define('AUTHORS_ALLOW_SELECT_DESC', 'If this option is enabled, a checkbox will be put next to each author in this sidebar plugin.  Users can check those boxes and see entries matching their selection.');
@define('AUTHOR_PLUGIN_DESC', 'Shows a list of authors');
@define('CATEGORY_PLUGIN_TEMPLATE', 'Enable Smarty-Templates?');
@define('CATEGORY_PLUGIN_TEMPLATE_DESC', 'If this option is enabled, the plugin will utilize Smarty-Templating features to output the category listing. If you enable this, you can change the layout via the "plugin_categories.tpl" template file. Enabling this option will impact performance, so if you do not need to make customizations, leave it disabled.');
@define('CATEGORY_PLUGIN_SHOWCOUNT', 'Show number of entries per category?');
@define('AUTHORS_SHOW_ARTICLE_COUNT', 'Show number of articles next to author name?');
@define('AUTHORS_SHOW_ARTICLE_COUNT_DESC', 'If this option is enabled, the number of articles by this author is shown next to the authors name in parentheses.');

@define('COMMENT_NOT_ADDED', '您的回响不能增入因为此篇文章不允许回响. '); // Retranslate: 'Your comment could not be added, because comments for this entry have either been disabled, you entered invalid data, or your comment was caught by anti-spam measurements.'
