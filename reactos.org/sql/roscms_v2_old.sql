-- 
-- Database: RosCMS
-- 

-- --------------------------------------------------------

-- 
-- Table structure for table `content`
-- 

DROP TABLE IF EXISTS `content`;
CREATE TABLE `content` (
  `content_id` bigint(20) NOT NULL auto_increment,
  `content_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `content_lang` varchar(10) collate utf8_unicode_ci NOT NULL default 'all',
  `content_text` text collate utf8_unicode_ci NOT NULL,
  `content_version` int(11) NOT NULL default '0',
  `content_active` tinyint(4) NOT NULL default '0',
  `content_visible` tinyint(4) NOT NULL default '0',
  `content_type` varchar(10) collate utf8_unicode_ci NOT NULL default 'default',
  `content_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `content_editor` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `content_usrname_id` bigint(20) NOT NULL default '0',
  `content_date` date NOT NULL default '0000-00-00',
  `content_time` time NOT NULL default '00:00:00',
  PRIMARY KEY  (`content_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Contents';

-- 
-- Data for table`content`
-- 

INSERT INTO `content` (`content_id`, `content_name`, `content_lang`, `content_text`, `content_version`, `content_active`, `content_visible`, `content_type`, `content_description`, `content_editor`, `content_usrname_id`, `content_date`, `content_time`) VALUES (31, 'index', 'all', '<h1>ReactOS Content Management System - Test Page</h1>\r\n<p>English start page</p>', 1, 1, 1, 'default', '', 'richtext', 2, '2005-12-05', '11:48:43'),
(2, 'bottom', 'all', '</div></td>\r\n </tr>\r\n</table>\r\n\r\n<!--\r\n     links/lynx/etc.. dont handle css (atleast not external\r\n     files by default) so dont overly depend on it.\r\n -->\r\n<hr style="height: 1px;"/>\r\n\r\n<address style="text-align:center;">\r\n  ReactOS is a registered trademark or a trademark of ReactOS Foundation in the United States and other countries.</address>\r\n</body>\r\n</html>									', 1, 1, 1, 'layout', '', '', 2, '2005-09-19', '23:15:40'),
(21, 'menu_top', 'all', '<div id="top">\r\n  <div id="topMenu"> \r\n    <!-- \r\n       Use <p> to align things for links/lynx, then in the css make it\r\n	   margin: 0; and use text-align: left/right/etc;.\r\n   -->\r\n	<p style="text-align:left;"> \r\n		<a href="[#link_index]">Home</a> <span style="color: #ffffff">|</span> \r\n		<a href="[#link_news]">News</a> <span style="color: #ffffff">|</span> \r\n		<a href="[#link_sitemap]">Sitemap</a></p>\r\n	 </div>\r\n	</div>', 1, 1, 1, 'default', '2005-12-21 20:01:24 [2] |', '', 2, '2005-12-05', '11:49:02'),
(24, 'menu_side', 'all', '<table style="border:0" width="100%" cellpadding="0" cellspacing="0">\r\n  <tr valign="top">\r\n  <td style="width:147px" id="leftNav"> \r\n  <div class="navTitle">Navigation</div>\r\n    <ol>\r\n      <li><a href="[#link_index]">Home</a></li>\r\n      <li><a href="[#link_news]">News</a></li>\r\n      <li><a href="[#link_sitemap]">Sitemap</a></li>\r\n    </ol>\r\n  <p></p>\r\n			', 1, 1, 1, 'default', '2005-12-21 20:01:51 [2] |', '', 2, '2005-12-05', '11:48:53'),
(96, 'head', 'all', '<?xml version="1.0" encoding="[#inc_charset]"?>\r\n<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"\r\n    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">\r\n<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="[#roscms_language_short]" >\r\n\r\n<head>\r\n	<title>ReactOS Homepage - [#roscms_pagetitle]</title>\r\n	\r\n	<meta http-equiv="Content-Type" content="application/xml" />\r\n	<meta http-equiv="pics-label" content=''(pics-1.1 "http://www.icra.org/ratingsv02.html" l gen true for "http://www.reactos.com/" r (cz 1 lz 1 nz 1 oz 1 vz 1))'' />\r\n	<meta http-equiv="Pragma" content="no-cache" />\r\n	<meta name="Publisher" content="ReactOS Web Team" />\r\n	<meta name="Copyright" content="ReactOS Foundation" />\r\n	<meta name="generator" content="RosCMS" />\r\n	<meta name="Keywords" content="reactos, ros, ros32, win32 ros64, roscms, operating system, wiki, forum, download, information, support database, compatibility database, compatibility, package database, reactos package manager, reactos content management system, reactos 0.1, reactos 0.2, reactos 0.3, ros 0.1, ros 0.2, ros 0.3" />\r\n	<meta name="Description" content="ReactOS Homepage - ..." />\r\n	<meta name="Page-topic" content="Betriebssystem, Computer, Software, downloads" />\r\n	<meta name="Audience" content="all" />\r\n	<meta name="Content-language" content="DE" />\r\n	<meta name="Page-type" content="Operating System/Information/News" />\r\n	<meta name="Robots" content="index,follow" />\r\n	\r\n	<link rel="SHORTCUT ICON" href="[#roscms_path_homepage]favicon.ico" />\r\n	<link rel="alternate" type="application/rss+xml" title="ReactOS News Feed (RSS)" href="[#roscms_path_homepage]reactos.rdf" />\r\n	<link href="[#roscms_path_homepage]style.css" type="text/css" rel="stylesheet" />\r\n</head>\r\n<body>', 2, 1, 1, 'layout', '2005-12-21 19:17:16 [2] |', '', 2, '2005-09-19', '23:38:29'),
(228, 'menu_misc', 'all', '<div class="navTitle">Latest Update</div>   \r\n      <ol>\r\n        <li><div style="text-align:center;">[#roscms_date] [#roscms_time]</div></li>\r\n      </ol>      \r\n      <p> </p>\r\n      </td>\r\n\r\n    <td id="content"><div class="contentSmall">	', 1, 1, 1, 'default', '', '', 2, '2005-12-21', '19:16:47'),
(229, 'sidebar_right', 'all', '</div></td>\r\n\r\n  <td id="rightNav">\r\n<h1>Latest Release</h1>\r\n<p><strong>Version [#inc_reactos_version]</strong>\r\n</p>\r\n', 8, 1, 1, 'default', '2005-12-21 19:50:38 [2] |', '', 2, '2005-12-21', '19:18:36'),
(230, 'contmenu_home', 'all', '<div class="navTitle">Home</div>   \r\n<ol> \r\n  <li><a href="[#link_index]">Front Page</a></li>\r\n  <li><a href="[#link_news]">News</a></li>\r\n  <li><a href="[#link_sitemap]">Sitemap</a></li>\r\n</ol>\r\n<p></p>', 4, 1, 1, 'default', '2005-12-21 20:02:26 [2] |', '', 2, '2005-12-21', '19:20:34'),
(120, 'menu_search', 'all', ' <div class="navTitle">Search</div>   \r\n <div class="navBox"><form method="get" action="http://www.google.com/search" style="padding:0;margin:0">\r\n  <div style="text-align:center;">\r\n\r\n   <input name="q" value=""  size="12" maxlength="80" class="searchInput" type="text" /><input name="domains" value="http://www.reactos.org" type="hidden" /><input name="sitesearch" value="http://www.reactos.org" type="hidden" />\r\n   <input name="btnG" value="Go" type="submit" class="button" />\r\n\r\n  </div></form>\r\n </div>\r\n<p></p>', 2, 1, 1, 'default', '', '', 2, '2005-11-09', '23:04:31'),
(128, 'index', 'de', '<h1>ReactOS Content Management System - Test Page</h1>\r\n<p>German start page</p>', 1, 1, 1, 'default', '', 'richtext', 1, '2005-11-29', '20:14:46'),
(152, 'menu_language', 'all', '<div class="navTitle">Language</div>  \r\n	<div class="navBox"> \r\n		<form method="post" action="[#roscms_path_homepage]?page=[#roscms_pagename]&amp;forma=[#roscms_format]&amp;lang=" style="padding:0;margin:0">\r\n			<div style="text-align:center;"> \r\n				<select id="lang" size="1" name="lang" class="selectbox" style="width:110px" onchange="window.open(''[#roscms_path_homepage]?page=[#roscms_pagename]&amp;forma=[#roscms_format]&amp;lang='' + this.options[this.selectedIndex].value,''_main'')">\r\n					<optgroup label="current language"> \r\n						<option value="#" selected="selected">[#roscms_language]</option>\r\n					</optgroup>\r\n					<optgroup label="most popular"> \r\n						<option value="en">English</option>\r\n						<option value="de">Deutsch (German)</option>\r\n					</optgroup>\r\n				</select>\r\n				<input name="langsubmit" type="submit" id="langsubmit" value="&gt;" />\r\n			</div>\r\n		</form>\r\n	</div>\r\n<p></p>', 3, 1, 1, 'layout', '2005-12-21 20:00:23 [2] |', '', 2, '2005-11-10', '21:48:20'),
(220, 'contmenu_home', 'de', '<div class="navTitle">Startseite</div>   \r\n<ol> \r\n  <li><a href="[#link_index]">Startseite</a></li>\r\n  <li><a href="[#link_news]">Neuigkeiten</a></li>\r\n  <li><a href="[#link_sitemap]">Sitemap</a></li> \r\n</ol>\r\n<p></p>', 3, 1, 1, 'default', '2005-12-21 20:02:35 [2] |', '', 15, '2005-12-07', '21:24:40'),
(222, 'sidebar_right', 'all', '</div></td>\r\n\r\n  <td id="rightNav">\r\n[#inc_javascript_screenshot]\r\n <a href="[#link_screenshots]">More Screenshots</a><br />\r\n <a href="[#link_tour]">Take a look at the ReactOS Tour</a>\r\n</div><br />\r\n<h1>Latest Release</h1>\r\n<p><strong>Version [#inc_reactos_version]</strong><br />\r\n    <a href="[#link_download]">Download Now!</a><br />\r\n    <a href="[#link_dev_changelogs]">Changelog</a>\r\n</p>\r\n[#inc_template_news_latest]\r\n[#inc_template_newsletter_latest]\r\n<div class="contentSmall">\r\n    <span class="contentSmallTitle">Developer Quotes</span>\r\n\r\n[#inc_javascript_quote]', 7, 0, 1, 'default', '', '', 2, '2005-12-07', '21:30:53');

-- --------------------------------------------------------

-- 
-- Table structure for table `dyn_content`
-- 

DROP TABLE IF EXISTS `dyn_content`;
CREATE TABLE `dyn_content` (
  `dyn_id` bigint(20) NOT NULL auto_increment,
  `dyn_content_id` bigint(20) NOT NULL default '0',
  `dyn_content_nr` int(11) NOT NULL default '0',
  `dyn_content_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `dyn_content_lang` varchar(10) collate utf8_unicode_ci NOT NULL default 'all',
  `dyn_content_text1` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `dyn_content_text2` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `dyn_content_text3` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `dyn_content_text4` longtext collate utf8_unicode_ci NOT NULL,
  `dyn_content_version` int(11) NOT NULL default '0',
  `dyn_content_active` tinyint(4) NOT NULL default '0',
  `dyn_content_visible` tinyint(4) NOT NULL default '0',
  `dyn_content_editor` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `dyn_content_usrname_id` bigint(20) NOT NULL default '0',
  `dyn_content_date` date NOT NULL default '0000-00-00',
  `dyn_content_time` time NOT NULL default '00:00:00',
  PRIMARY KEY  (`dyn_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Contents';

-- 
-- Data for table`dyn_content`
-- 

INSERT INTO `dyn_content` (`dyn_id`, `dyn_content_id`, `dyn_content_nr`, `dyn_content_name`, `dyn_content_lang`, `dyn_content_text1`, `dyn_content_text2`, `dyn_content_text3`, `dyn_content_text4`, `dyn_content_version`, `dyn_content_active`, `dyn_content_visible`, `dyn_content_editor`, `dyn_content_usrname_id`, `dyn_content_date`, `dyn_content_time`) VALUES (28, 1, 1, 'news_page', 'all', 'RosCMS test news', '', 'RosCMS test news description', '<p><b><u>RosCMS</u></b> test news content</p> ...', 0, 1, 1, '', 2, '2005-12-21', '19:53:14');

-- --------------------------------------------------------

-- 
-- Table structure for table `include_text`
-- 

DROP TABLE IF EXISTS `include_text`;
CREATE TABLE `include_text` (
  `inc_id` bigint(20) NOT NULL auto_increment,
  `inc_level` tinyint(4) NOT NULL default '0',
  `inc_word` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `inc_text` text collate utf8_unicode_ci NOT NULL,
  `inc_lang` varchar(10) collate utf8_unicode_ci NOT NULL default 'all',
  `inc_extra` varchar(20) collate utf8_unicode_ci NOT NULL default '',
  `inc_vis` tinyint(4) NOT NULL default '0',
  `inc_seclevel` tinyint(4) NOT NULL default '50',
  `inc_usrname_id` bigint(20) NOT NULL default '0',
  `inc_date` date NOT NULL default '0000-00-00',
  `inc_time` time NOT NULL default '00:00:00',
  PRIMARY KEY  (`inc_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Replace words with text phrases';

-- 
-- Data for table`include_text`
-- 

INSERT INTO `include_text` (`inc_id`, `inc_level`, `inc_word`, `inc_text`, `inc_lang`, `inc_extra`, `inc_vis`, `inc_seclevel`, `inc_usrname_id`, `inc_date`, `inc_time`) VALUES (48, 15, 'template_news_detail', '<?php\r\n  	if ( !defined("ROSCMS_SYSTEM") ) {\r\n		define ("ROSCMS_SYSTEM", "Version 0.1"); // to prevent hacking activity\r\n	}\r\n\r\n	include("db/connect_db.inc.php"); // database connection script\r\n\r\n	$query_content = mysql_query("SELECT * \r\n									FROM `dyn_content` \r\n									WHERE 1 AND `dyn_content_name` = ''news_page'' AND `dyn_content_id` = ''$roscms_template_var_pageid''\r\n									ORDER BY `dyn_content_nr` ASC Limit 1") ;\r\n	\r\n	$result_content = mysql_fetch_array($query_content);\r\n	$roscms_usrnameid = $result_content["dyn_content_usrname_id"];\r\n	$query_usraccount= mysql_query("SELECT * \r\n										FROM `users` \r\n										WHERE `user_id` = ".$roscms_usrnameid." LIMIT 1");\r\n	$result_usraccount=mysql_fetch_array($query_usraccount);\r\n?>\r\n<h1><a href="[#link_index]">Home</a> &gt; <a href="[#link_news]">ReactOS News</a> &gt; News: <?php echo $result_content["dyn_content_text1"]; ?> #<?php echo $roscms_template_var_pageid; ?></h1> \r\n  <p><strong><?php echo $result_content["dyn_content_text3"]; ?></strong></p>\r\n  <p><i>by <?php echo $result_usraccount[''user_name'']; ?> on <?php echo $result_content[''dyn_content_date'']; ?></i></p>\r\n<?php echo $result_content[''dyn_content_text4'']; ?>\r\n<p><a href="[#link_news]">News Archive</a></p>', 'all', 'template_php', 1, 50, 1, '2005-12-21', '19:55:14'),
(19, 10, 'path_homepage_media', '[#roscms_path_homepage]media/', 'all', '', 1, 50, 0, '2005-07-13', '18:00:35'),
(20, 15, 'path_homepage_media_pictures', '[#roscms_path_homepage]media/pictures/', 'all', '', 1, 50, 0, '2005-07-13', '18:00:35'),
(23, 15, 'template_news', '<h1><a href="[#link_index]">Home</a> &gt; ReactOS News</h1>\r\n<?php\r\n  	if ( !defined("ROSCMS_SYSTEM") ) {\r\n		define ("ROSCMS_SYSTEM", "Version 0.1"); // to prevent hacking activity\r\n	}\r\n\r\n	include("db/connect_db.inc.php"); // database connection script\r\n\r\n\r\n	$query_content = mysql_query("SELECT * \r\n									FROM `dyn_content` \r\n									WHERE 1 AND `dyn_content_name` = ''news_page'' AND `dyn_content_nr` = ''1''\r\n									ORDER BY `dyn_content_id` DESC ") ;\r\n	\r\n	while($result_content = mysql_fetch_array($query_content)) { // content\r\n		$roscms_usrnameid = $result_content["dyn_content_usrname_id"];\r\n		$query_usraccount= mysql_query("SELECT * \r\n											FROM `users` \r\n											WHERE `user_id` = ".$roscms_usrnameid." LIMIT 0 , 1");\r\n		$result_usraccount=mysql_fetch_array($query_usraccount);\r\n?>\r\n\r\n<p><b><a href="<?php echo "[#link_".$result_content["dyn_content_name"]."_".$result_content["dyn_content_id"]."]"; ?>"><?php echo $result_content["dyn_content_text1"]; ?></a></b>\r\n<br><?php echo $result_content[''dyn_content_text3'']; ?>\r\n<br><i>by <?php echo $result_usraccount[''user_name'']; ?> on <?php echo $result_content[''dyn_content_date'']; ?></i></p>\r\n  <?php	\r\n	}	// end while\r\n?>\r\n', 'all', 'template_php', 1, 50, 0, '2005-07-30', '11:50:18'),
(26, 0, 'reactos_version', '0.2.8', 'all', '', 1, 50, 1, '2005-08-13', '16:18:56'),
(27, 15, 'template_sitemap', '<h1><a href="[#link_index]">Home</a> &gt; Sitemap</h1>\r\n<?php\r\n  	if ( !defined("ROSCMS_SYSTEM") ) {\r\n		define ("ROSCMS_SYSTEM", "Version 0.1"); // to prevent hacking activity\r\n	}\r\n\r\n	include("db/connect_db.inc.php"); // database connection script\r\n\r\n\r\n	$query_sitemap = mysql_query("SELECT * \r\n									FROM pages\r\n									WHERE page_active = ''1'' AND page_visible = ''1'' AND (page_language = ''all'' OR page_language = ''en'')\r\n									ORDER BY ''page_name'' ASC") ;\r\n	\r\n	while($result_sitemap = mysql_fetch_array($query_sitemap)) { // content\r\n	if ($result_sitemap[''page_description''] != "") {\r\n?>\r\n\r\n<p><b><a href="<?php echo "[#link_".$result_sitemap[''page_name'']."]"; ?>"><?php echo ucfirst($result_sitemap[''page_title'']); ?></a></b>\r\n<br><i><?php echo $result_sitemap[''page_description'']; ?></i></p>\r\n  <?php\r\n  		}	\r\n	}	// end while\r\n?>\r\n', 'all', 'template_php', 1, 50, 0, '2005-08-13', '20:58:04'),
(34, 0, 'charset', 'utf-8', 'all', '', 1, 50, 1, '2005-08-27', '15:54:16'),
(39, 15, 'path_homepage_media_screenshots', '[#roscms_path_homepage]media/screenshots/', 'all', '', 1, 50, 0, '2005-07-13', '18:00:35');

-- --------------------------------------------------------

-- 
-- Table structure for table `languages`
-- 

DROP TABLE IF EXISTS `languages`;
CREATE TABLE `languages` (
  `lang_id` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `lang_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_level` int(11) NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='available languages';

-- 
-- Data for table`languages`
-- 

INSERT INTO `languages` (`lang_id`, `lang_name`, `lang_level`)
VALUES ('en', 'English', 10),
('de', 'German', 8);

-- --------------------------------------------------------

-- 
-- Table structure for table `pages`
-- 

DROP TABLE IF EXISTS `pages`;
CREATE TABLE `pages` (
  `page_id` bigint(20) NOT NULL auto_increment,
  `page_name` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `page_language` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `pages_extra` varchar(20) collate utf8_unicode_ci NOT NULL default '',
  `pages_extention` varchar(10) collate utf8_unicode_ci NOT NULL default 'default',
  `page_text` text collate utf8_unicode_ci NOT NULL,
  `page_version` int(11) NOT NULL default '0',
  `page_active` tinyint(4) NOT NULL default '0',
  `page_visible` tinyint(4) NOT NULL default '0',
  `page_usrname_id` bigint(20) NOT NULL default '0',
  `page_date` date NOT NULL default '0000-00-00',
  `page_time` time NOT NULL default '00:00:00',
  `page_generate_usrid` bigint(20) NOT NULL default '0',
  `page_generate_timestamp` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `page_title` varchar(150) collate utf8_unicode_ci NOT NULL default '',
  `page_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`page_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Pages';

-- 
-- Data for table`pages`
-- 

INSERT INTO `pages` (`page_id`, `page_name`, `page_language`, `pages_extra`, `pages_extention`, `page_text`, `page_version`, `page_active`, `page_visible`, `page_usrname_id`, `page_date`, `page_time`, `page_generate_usrid`, `page_generate_timestamp`, `page_title`, `page_description`) VALUES (6, 'news_page', 'all', 'dynamic', 'default', '[#cont_head]\r\n[#cont_menu_top]\r\n[#cont_menu_side]\r\n\r\n[#cont_contmenu_home]\r\n\r\n[#cont_menu_search]\r\n[#cont_menu_language]\r\n[#cont_menu_misc]\r\n\r\n[#inc_template_news_detail]\r\n\r\n[#cont_bottom]', 1, 1, 1, 1, '2005-09-12', '18:50:20', 1, '1135191757', 'News', ''),
(7, 'news', 'all', '', 'default', '[#cont_head]\r\n[#cont_menu_top]\r\n[#cont_menu_side]\r\n\r\n[#cont_contmenu_home]\r\n\r\n[#cont_menu_search]\r\n[#cont_menu_language]\r\n[#cont_menu_misc]\r\n\r\n[#inc_template_news]\r\n\r\n[#cont_bottom]', 1, 1, 1, 1, '2005-09-12', '18:50:28', 1, '1135191757', 'News', 'News page: latest ReactOS news, rss news feed'),
(9, 'index', 'all', '', 'default', '[#cont_head]\r\n[#cont_menu_top]\r\n[#cont_menu_side]\r\n\r\n[#cont_contmenu_home]\r\n\r\n[#cont_menu_search]\r\n[#cont_menu_language]\r\n[#cont_menu_misc]\r\n\r\n[#cont_index]\r\n\r\n[#cont_sidebar_right]\r\n[#cont_bottom]', 1, 1, 1, 1, '2005-12-21', '19:21:43', 1, '1135191757', 'Frontpage', 'Frontpage'),
(91, 'sitemap', 'all', '', 'default', '[#cont_head]\r\n[#cont_menu_top]\r\n[#cont_menu_side]\r\n\r\n[#cont_contmenu_home]\r\n\r\n[#cont_menu_search]\r\n[#cont_menu_language]\r\n[#cont_menu_misc]\r\n\r\n[#inc_template_sitemap]\r\n\r\n[#cont_bottom]\r\n', 1, 1, 1, 1, '2005-12-21', '19:58:07', 1, '1135191757', 'Sitemap', 'Sitemap page');

-- --------------------------------------------------------

-- 
-- Table structure for table `roscms_security_log`
-- 

DROP TABLE IF EXISTS `roscms_security_log`;
CREATE TABLE `roscms_security_log` (
  `roscms_sec_log_id` bigint(20) NOT NULL auto_increment,
  `roscms_sec_log_section` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `roscms_sec_log_priority` int(11) NOT NULL default '100',
  `roscms_sec_log_reason` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `roscms_sec_log_user` varchar(100) collate utf8_unicode_ci NOT NULL default 'roscms_system',
  `roscms_sec_log_useraccount` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `roscms_sec_log_userip` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `roscms_sec_log_referrer` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `roscms_sec_log_text` text collate utf8_unicode_ci NOT NULL,
  `roscms_sec_log_date` date NOT NULL default '0000-00-00',
  `roscms_sec_log_time` time NOT NULL default '00:00:00',
  `roscms_sec_log_visible` int(11) NOT NULL default '1',
  PRIMARY KEY  (`roscms_sec_log_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RosCMS Security Log';

-- 
-- Data for table`roscms_security_log`
-- 


-- --------------------------------------------------------

-- 
-- Table structure for table `subsys_mappings`
-- 

DROP TABLE IF EXISTS `subsys_mappings`;
CREATE TABLE `subsys_mappings` (
  `map_roscms_userid` bigint(20) NOT NULL default '0',
  `map_subsys_name` varchar(10) character set utf8 NOT NULL default '',
  `map_subsys_userid` int(7) NOT NULL default '0',
  PRIMARY KEY  (`map_roscms_userid`,`map_subsys_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;


-- --------------------------------------------------------

-- 
-- Table structure for table `user_sessions`
-- 

DROP TABLE IF EXISTS `user_sessions`;
CREATE TABLE `user_sessions` (
  `usersession_id` varchar(32) character set utf8 NOT NULL default '' COMMENT 'Unique ID of this session',
  `usersession_user_id` bigint(20) NOT NULL default '0' COMMENT 'User this session belongs to',
  `usersession_expires` datetime default NULL COMMENT 'Expiry date/time (NULL if does not expire)',
  `usersession_browseragent` varchar(255) character set utf8 NOT NULL default '' COMMENT 'HTTP_USER_AGENT when this session was created',
  `usersession_ipaddress` varchar(15) character set utf8 NOT NULL default '' COMMENT 'IP address from which this session was created',
  `usersession_created` datetime NOT NULL default '0000-00-00 00:00:00' COMMENT 'session created (date/time) - all session will get deleted ... by date',
  PRIMARY KEY  (`usersession_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci PACK_KEYS=0 COMMENT='Sessions';


-- --------------------------------------------------------

-- 
-- Table structure for table `usergroup_members`
-- 

DROP TABLE IF EXISTS `usergroup_members`;
CREATE TABLE `usergroup_members` (
  `usergroupmember_userid` bigint(20) NOT NULL default '0',
  `usergroupmember_usergroupid` varchar(10) collate utf8_unicode_ci NOT NULL default 'user'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- 
-- Data for table`usergroup_members`
-- 

INSERT INTO `usergroup_members` (`usergroupmember_userid`, `usergroupmember_usergroupid`)
VALUES (1, 'ros_sadmin'),
(1, 'developer'),
(1, 'translator'),
(1, 'moderator'),
(1, 'user');

-- --------------------------------------------------------

-- 
-- Table structure for table `usergroups`
-- 

DROP TABLE IF EXISTS `usergroups`;
CREATE TABLE `usergroups` (
  `usrgroup_name_id` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_intern_id` varchar(25) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_securitylevel` tinyint(4) NOT NULL default '0',
  `usrgroup_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`usrgroup_name_id`),
  UNIQUE KEY `usrgroup_name` (`usrgroup_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- 
-- Data for table`usergroups`
-- 

INSERT INTO `usergroups` (`usrgroup_name_id`, `usrgroup_name`, `usrgroup_intern_id`, `usrgroup_securitylevel`, `usrgroup_description`) VALUES ('ros_admin', 'Administrator', 'roscms_usrgrp_admin', 70, 'Administrator-Group: manage the ReactOS homepage (content, database)'),
('user', 'User', 'roscms_usrgrp_user', 0, 'Normal Visitors'),
('developer', 'Developer', 'roscms_usrgrp_dev', 50, 'ReactOS Developer: persons with svn commit access'),
('moderator', 'Moderator', 'roscms_usrgrp_team', 20, 'Homepage Moderators: for compatibility & package manager database, etc.'),
('translator', 'Translator', 'roscms_usrgrp_trans', 10, 'Homepage Translators: translate the homepage content'),
('ros_sadmin', 'Super-Administrator', 'roscms_usrgrp_sadmin', 100, 'Super-Administrator-Group: manage the ReactOS homepage (content, database, etc.); only persons who know what they are doing ...'),
('mediateam', 'Media-Team', 'roscms_usrgrp_team', 20, 'Media-Team members: UI-Team, etc.'),
('test', 'Test-User', 'roscms_usrgrp_team', 10, 'RosCMS tester group');

-- --------------------------------------------------------

-- 
-- Table structure for table `users`
-- 

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
  `user_id` bigint(20) NOT NULL auto_increment,
  `user_name` varchar(20) collate utf8_unicode_ci NOT NULL default '',
  `user_roscms_password` varchar(32) collate utf8_unicode_ci NOT NULL default '',
  `user_roscms_getpwd_id` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `user_timestamp_touch` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `user_timestamp_touch2` timestamp NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `user_login_counter` bigint(20) NOT NULL default '0',
  `user_account_enabled` varchar(10) collate utf8_unicode_ci NOT NULL default 'no',
  `user_account_hidden` varchar(5) collate utf8_unicode_ci NOT NULL default 'no',
  `user_register` timestamp NULL default NULL,
  `user_fullname` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `user_email` varchar(150) collate utf8_unicode_ci NOT NULL default '',
  `user_website` varchar(150) collate utf8_unicode_ci NOT NULL default '',
  `user_language` varchar(2) collate utf8_unicode_ci NOT NULL default '',
  `user_country` varchar(2) collate utf8_unicode_ci NOT NULL default '',
  `user_timezone` varchar(3) collate utf8_unicode_ci NOT NULL default '',
  `user_occupation` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `user_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `user_setting_multisession` varchar(5) collate utf8_unicode_ci NOT NULL default 'false',
  `user_setting_browseragent` varchar(5) collate utf8_unicode_ci NOT NULL default 'true',
  `user_setting_ipaddress` varchar(5) collate utf8_unicode_ci NOT NULL default 'true',
  `user_setting_timeout` varchar(5) collate utf8_unicode_ci NOT NULL default 'true',
  PRIMARY KEY  (`user_id`),
  UNIQUE KEY `user_name` (`user_name`),
  UNIQUE KEY `user_email` (`user_email`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RosCMS User Table';

-- 
-- Data for table`users`
-- 

INSERT INTO `users` (`user_id`, `user_name`, `user_roscms_password`, `user_roscms_getpwd_id`, `user_timestamp_touch`, `user_timestamp_touch2`, `user_login_counter`, `user_account_enabled`, `user_account_hidden`, `user_register`, `user_fullname`, `user_email`, `user_website`, `user_language`, `user_country`, `user_timezone`, `user_occupation`, `user_description`, `user_setting_multisession`, `user_setting_browseragent`, `user_setting_ipaddress`, `user_setting_timeout`) 
VALUES (1, 'roscms_user', '', '', '', '2005-12-08 20:02:50', 1, 'yes', 'yes', '2005-08-20 20:18:06', 'ReactOS Content Management System Default User', 'ros-dev@reactos.org', 'http://www.reactos.org', 'en', '', '', '', 'RosCMS Default User', 'true', 'false', 'false', 'true');