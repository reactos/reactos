-- phpMyAdmin SQL Dump
-- version 2.11.8.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Sep 26, 2008 at 03:26 PM
-- Server version: 4.1.20
-- PHP Version: 5.0.4

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Database: `roscms`
--

-- --------------------------------------------------------

--
-- Table structure for table `data_`
--

CREATE TABLE `data_` (
  `data_id` int(11) NOT NULL auto_increment,
  `data_name` varchar(100) collate utf8_unicode_ci NOT NULL default '' COMMENT 'together with page_type unique entries',
  `data_type` varchar(10) collate utf8_unicode_ci NOT NULL default '' COMMENT '"page", "layout", "content", "script"',
  `data_acl` varchar(50) collate utf8_unicode_ci NOT NULL default 'default',
  PRIMARY KEY  (`data_id`),
  KEY `data_name` (`data_name`,`data_type`),
  KEY `data_type` (`data_type`),
  KEY `data_acl` (`data_acl`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='all RosCMS v1 "pages", "content", "dyncontent" and "inctext"';

INSERT INTO `data_` VALUES (1, 'index', 'page', 'defaultpage');
INSERT INTO `data_` VALUES (2, 'default', 'template', 'layout');
INSERT INTO `data_` VALUES (3, 'index', 'content', 'default');
INSERT INTO `data_` VALUES (4, 'news', 'script', 'script');
INSERT INTO `data_` VALUES (5, 'news_page', 'content', 'default');

-- --------------------------------------------------------

--
-- Table structure for table `data_a`
--

CREATE TABLE `data_a` (
  `data_id` int(11) NOT NULL auto_increment,
  `data_name` varchar(100) collate utf8_unicode_ci NOT NULL default '' COMMENT 'together with page_type unique entries',
  `data_type` varchar(10) collate utf8_unicode_ci NOT NULL default '' COMMENT '"page", "layout", "content", "script"',
  `data_acl` varchar(50) collate utf8_unicode_ci NOT NULL default 'default',
  PRIMARY KEY  (`data_id`),
  KEY `data_name` (`data_name`,`data_type`),
  KEY `data_type` (`data_type`),
  KEY `data_acl` (`data_acl`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='all RosCMS v1 "pages", "content", "dyncontent" and "inctext"';

-- --------------------------------------------------------

--
-- Table structure for table `data_revision`
--

CREATE TABLE `data_revision` (
  `rev_id` int(11) NOT NULL auto_increment COMMENT 'unique revisions id',
  `data_id` int(11) NOT NULL default '0',
  `rev_version` int(11) NOT NULL default '0' COMMENT 'version number increase with every entry change',
  `rev_language` varchar(10) collate utf8_unicode_ci NOT NULL default '' COMMENT 'language code ("en", "de", etc.)',
  `rev_usrid` int(11) NOT NULL default '0' COMMENT 'user id who saved the entry',
  `rev_datetime` datetime NOT NULL default '0000-00-00 00:00:00',
  `rev_date` date NOT NULL default '0000-00-00',
  `rev_time` time NOT NULL default '00:00:00',
  PRIMARY KEY  (`rev_id`),
  KEY `data_id` (`data_id`),
  KEY `rev_language` (`rev_language`),
  KEY `rev_version` (`rev_version`),
  KEY `rev_datetime` (`rev_datetime`),
  KEY `rev_usrid` (`rev_usrid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_revision` VALUES (1, 1, 1, 'en', 1, '2007-08-13 17:03:44', '2007-08-13', '17:03:44');
INSERT INTO `data_revision` VALUES (2, 2, 1, 'en', 1, '2007-08-13 17:05:03', '2007-08-13', '17:05:03');
INSERT INTO `data_revision` VALUES (6, 3, 2, 'en', 1, '2007-08-13 17:15:15', '2007-08-13', '17:15:15');
INSERT INTO `data_revision` VALUES (5, 4, 2, 'en', 1, '2007-08-13 17:11:36', '2007-08-13', '17:11:36');
INSERT INTO `data_revision` VALUES (7, 5, 1, 'en', 1, '2007-08-13 17:15:20', '2007-08-13', '17:15:20');
INSERT INTO `data_revision` VALUES (8, 5, 1, 'en', 1, '2007-08-13 17:15:45', '2007-08-13', '17:15:45');

-- --------------------------------------------------------

--
-- Table structure for table `data_revision_a`
--

CREATE TABLE `data_revision_a` (
  `rev_id` int(11) NOT NULL auto_increment COMMENT 'unique revisions id',
  `data_id` int(11) NOT NULL default '0',
  `rev_version` int(11) NOT NULL default '0' COMMENT 'version number increase with every entry change',
  `rev_language` varchar(10) collate utf8_unicode_ci NOT NULL default '' COMMENT 'language code ("en", "de", etc.)',
  `rev_usrid` int(11) NOT NULL default '0' COMMENT 'user id who saved the entry',
  `rev_datetime` datetime NOT NULL default '0000-00-00 00:00:00',
  `rev_date` date NOT NULL default '0000-00-00',
  `rev_time` time NOT NULL default '00:00:00',
  PRIMARY KEY  (`rev_id`),
  KEY `data_id` (`data_id`),
  KEY `rev_language` (`rev_language`),
  KEY `rev_version` (`rev_version`),
  KEY `rev_datetime` (`rev_datetime`),
  KEY `rev_usrid` (`rev_usrid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_security`
--

CREATE TABLE `data_security` (
  `sec_name` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `sec_fullname` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `sec_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `sec_branch` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `sec_lev1_read` tinyint(4) NOT NULL default '0',
  `sec_lev1_write` tinyint(4) NOT NULL default '0',
  `sec_lev1_add` tinyint(4) NOT NULL default '0',
  `sec_lev1_pub` tinyint(4) NOT NULL default '0',
  `sec_lev1_trans` tinyint(4) NOT NULL default '0',
  `sec_lev2_read` tinyint(4) NOT NULL default '0',
  `sec_lev2_write` tinyint(4) NOT NULL default '0',
  `sec_lev2_add` tinyint(4) NOT NULL default '0',
  `sec_lev2_pub` tinyint(4) NOT NULL default '0',
  `sec_lev2_trans` tinyint(4) NOT NULL default '0',
  `sec_lev3_read` tinyint(4) NOT NULL default '0',
  `sec_lev3_write` tinyint(4) NOT NULL default '0',
  `sec_lev3_add` tinyint(4) NOT NULL default '0',
  `sec_lev3_pub` tinyint(4) NOT NULL default '0',
  `sec_lev3_trans` tinyint(4) NOT NULL default '0',
  `sec_allow` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `sec_deny` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`sec_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RosCMS content security';

INSERT INTO `data_security` VALUES ('default', 'Content (default)', 'Default content security ACL; normal translator cannot publish nor add new content', 'website', 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, '', '');
INSERT INTO `data_security` VALUES ('script', 'Script or Variable', 'Entry contain scripts, variables or similar content which can only be accessed and altered by admins.', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, '', '');
INSERT INTO `data_security` VALUES ('layout', 'Layout', 'Website layout related content, changing may destroy the website layout, so be careful!', 'website', 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, '', '|transmaint|');
INSERT INTO `data_security` VALUES ('announcement', 'Official announcement', 'Official announcement, which shouldn''t be altered by anyone except admins and devs', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, '|developer|', '');
INSERT INTO `data_security` VALUES ('notrans', 'No translation needed', 'Content don''t need a translation.', '', 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, '', '');
INSERT INTO `data_security` VALUES ('rosversion', 'ReactOS versions number', 'Currrent ReactOS version number, change the related contents on every release day!', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, '|developer|', '');
INSERT INTO `data_security` VALUES ('defaultpage', 'Page (default)', 'Default page security ACL', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, '|developer|', '');
INSERT INTO `data_security` VALUES ('defaultmenu', 'Menu (default)', 'Menu content which can be translated by language maintainers', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, '|transmaint|', '');
INSERT INTO `data_security` VALUES ('system', 'System', 'RosCMS system', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '|ros_sadmin|', '');
INSERT INTO `data_security` VALUES ('readonly', 'Read-Only', 'Read only, noone can alter this', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, '', '');
INSERT INTO `data_security` VALUES ('langmaint', 'Language Maintainer', 'Language-Group related content', 'website', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, '|transmaint|', '');

-- --------------------------------------------------------

--
-- Table structure for table `data_stext`
--

CREATE TABLE `data_stext` (
  `stext_id` int(11) NOT NULL auto_increment,
  `data_rev_id` int(11) NOT NULL default '0',
  `stext_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `stext_content` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`stext_id`),
  KEY `data_rev_id` (`data_rev_id`),
  FULLTEXT KEY `stext_content` (`stext_content`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_stext` VALUES (14, 1, 'description', 'test page');
INSERT INTO `data_stext` VALUES (13, 1, 'comment', '');
INSERT INTO `data_stext` VALUES (12, 2, 'description', 'test template');
INSERT INTO `data_stext` VALUES (15, 1, 'title', 'Index');
INSERT INTO `data_stext` VALUES (19, 6, 'description', 'test content');
INSERT INTO `data_stext` VALUES (18, 5, 'description', 'news php script');
INSERT INTO `data_stext` VALUES (22, 7, 'description', 'news issue 1');
INSERT INTO `data_stext` VALUES (23, 7, 'title', 'News_page');
INSERT INTO `data_stext` VALUES (26, 8, 'description', 'news issue 2');
INSERT INTO `data_stext` VALUES (27, 8, 'title', 'News_page');

-- --------------------------------------------------------

--
-- Table structure for table `data_stext_a`
--

CREATE TABLE `data_stext_a` (
  `stext_id` int(11) NOT NULL auto_increment,
  `data_rev_id` int(11) NOT NULL default '0',
  `stext_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `stext_content` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`stext_id`),
  KEY `data_rev_id` (`data_rev_id`),
  FULLTEXT KEY `stext_content` (`stext_content`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_tag`
--

CREATE TABLE `data_tag` (
  `tag_id` int(11) NOT NULL auto_increment,
  `data_id` int(11) NOT NULL default '0',
  `data_rev_id` int(11) NOT NULL default '0',
  `tag_name_id` int(11) NOT NULL default '0',
  `tag_value_id` int(11) NOT NULL default '0',
  `tag_usrid` int(11) NOT NULL default '0',
  PRIMARY KEY  (`tag_id`),
  KEY `data_id` (`data_id`),
  KEY `data_rev_id` (`data_rev_id`),
  KEY `tag_name_id` (`tag_name_id`),
  KEY `tag_value_id` (`tag_value_id`),
  KEY `tag_usrid` (`tag_usrid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_tag` VALUES (11, 1, 1, 1, 5, -1);
INSERT INTO `data_tag` VALUES (2, 1, 1, 2, 2, -1);
INSERT INTO `data_tag` VALUES (10, 2, 2, 1, 5, -1);
INSERT INTO `data_tag` VALUES (15, 4, 5, 5, 7, -1);
INSERT INTO `data_tag` VALUES (14, 4, 5, 1, 5, -1);
INSERT INTO `data_tag` VALUES (24, 3, 6, 1, 5, -1);
INSERT INTO `data_tag` VALUES (22, 5, 7, 1, 5, -1);
INSERT INTO `data_tag` VALUES (18, 5, 7, 6, 9, -1);
INSERT INTO `data_tag` VALUES (21, 5, 8, 1, 5, -1);
INSERT INTO `data_tag` VALUES (20, 5, 8, 6, 10, -1);

-- --------------------------------------------------------

--
-- Table structure for table `data_tag_a`
--

CREATE TABLE `data_tag_a` (
  `tag_id` int(11) NOT NULL auto_increment,
  `data_id` int(11) NOT NULL default '0',
  `data_rev_id` int(11) NOT NULL default '0',
  `tag_name_id` int(11) NOT NULL default '0',
  `tag_value_id` int(11) NOT NULL default '0',
  `tag_usrid` int(11) NOT NULL default '0',
  PRIMARY KEY  (`tag_id`),
  KEY `data_id` (`data_id`),
  KEY `data_rev_id` (`data_rev_id`),
  KEY `tag_name_id` (`tag_name_id`),
  KEY `tag_value_id` (`tag_value_id`),
  KEY `tag_usrid` (`tag_usrid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_tag_name`
--

CREATE TABLE `data_tag_name` (
  `tn_id` int(11) NOT NULL auto_increment,
  `tn_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`tn_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_tag_name` VALUES (1, 'status');
INSERT INTO `data_tag_name` VALUES (2, 'extension');
INSERT INTO `data_tag_name` VALUES (4, 'star');
INSERT INTO `data_tag_name` VALUES (5, 'kind');
INSERT INTO `data_tag_name` VALUES (6, 'number');

-- --------------------------------------------------------

--
-- Table structure for table `data_tag_name_a`
--

CREATE TABLE `data_tag_name_a` (
  `tn_id` int(11) NOT NULL auto_increment,
  `tn_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`tn_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_tag_value`
--

CREATE TABLE `data_tag_value` (
  `tv_id` int(11) NOT NULL auto_increment,
  `tv_value` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`tv_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_tag_value` VALUES (7, 'php');
INSERT INTO `data_tag_value` VALUES (2, 'html');
INSERT INTO `data_tag_value` VALUES (4, 'off');
INSERT INTO `data_tag_value` VALUES (5, 'stable');
INSERT INTO `data_tag_value` VALUES (9, '1');
INSERT INTO `data_tag_value` VALUES (10, '2');

-- --------------------------------------------------------

--
-- Table structure for table `data_tag_value_a`
--

CREATE TABLE `data_tag_value_a` (
  `tv_id` int(11) NOT NULL auto_increment,
  `tv_value` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`tv_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_text`
--

CREATE TABLE `data_text` (
  `text_id` int(11) NOT NULL auto_increment,
  `data_rev_id` int(11) NOT NULL default '0',
  `text_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `text_content` text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`text_id`),
  KEY `data_rev_id` (`data_rev_id`),
  FULLTEXT KEY `text_content` (`text_content`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `data_text` VALUES (9, 1, 'content', '[#templ_default]');
INSERT INTO `data_text` VALUES (8, 2, 'content', '[#cont_head][#cont_menu_top][#cont_menu_side][#cont_contmenu_[#%NAME%]][#cont_menu_search][#cont_menu_language][#cont_menu_misc][#cont_%NAME%][#cont_bottom]');
INSERT INTO `data_text` VALUES (13, 6, 'content', '<h1>Welcome to RosCMS v3</h1><p>RosCMS test page</p>[#inc_news]');
INSERT INTO `data_text` VALUES (15, 7, 'content', 'news #1 ...');
INSERT INTO `data_text` VALUES (17, 8, 'content', 'news #2');
INSERT INTO `data_text` VALUES (12, 5, 'content', '<h3>News</h3> <?php $tmp_year = "2085"; $query_content = mysql_query("SELECT d.data_name, r.rev_usrid, s1.stext_content AS ''title'', s2.stext_content AS ''description'', v.tv_value as ''numb'', r.rev_date  FROM data_ d, data_revision r, data_stext s1, data_stext s2, data_tag a, data_tag_name n, data_tag_value v  WHERE d.data_name = ''news_page''  AND d.data_type = ''content''  AND r.data_id = d.data_id  AND (r.rev_language = ''".mysql_real_escape_string($roscms_template_var_lang)."'' OR r.rev_language = ''en'')   AND r.rev_version > 0  AND s1.data_rev_id = r.rev_id  AND s1.stext_name = ''title''  AND s2.data_rev_id = r.rev_id  AND s2.stext_name = ''description''  AND r.data_id = a.data_id  AND r.rev_id = a.data_rev_id  AND a.tag_usrid = ''-1''  AND a.tag_name_id = n.tn_id  AND n.tn_name = ''number''  AND a.tag_value_id = v.tv_id  ORDER BY r.rev_datetime DESC  LIMIT 3;"); while($result_content = mysql_fetch_array($query_content)) { 
if ($tmp_year > substr($result_content[''rev_date''], 0, 4)) { $tmp_year = substr($result_content[''rev_date''], 0, 4); echo "<h3>".$tmp_year."</h3>"; } $query_usraccount= mysql_query("SELECT user_name, user_fullname   FROM users   WHERE user_id = ''".mysql_real_escape_string($result_content["rev_usrid"])."''   LIMIT 1;"); $result_usraccount=mysql_fetch_array($query_usraccount); ?>  <p align="left"><b><font color="#666666"><?php echo $result_content[''rev_date'']; ?>,  <?php  if ($result_usraccount[''user_fullname'']) { echo $result_usraccount[''user_fullname''];  }  else { echo $result_usraccount[''user_name''];  }  ?>  </font></b><br />  <b><a href="<?php echo "[#link_".$result_content["data_name"]."_".$result_content["numb"]."]"; ?>"><?php echo $result_content["title"]; ?></a></b><br />  <?php echo $result_content["description"]; ?></p> <?php } // end while ?> ');

-- --------------------------------------------------------

--
-- Table structure for table `data_text_a`
--

CREATE TABLE `data_text_a` (
  `text_id` int(11) NOT NULL auto_increment,
  `data_rev_id` int(11) NOT NULL default '0',
  `text_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `text_content` text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`text_id`),
  KEY `data_rev_id` (`data_rev_id`),
  FULLTEXT KEY `text_content` (`text_content`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `data_user_filter`
--

CREATE TABLE `data_user_filter` (
  `filt_id` int(11) NOT NULL auto_increment,
  `filt_usrid` int(11) NOT NULL default '0',
  `filt_title` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `filt_type` smallint(6) NOT NULL default '1',
  `filt_string` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `filt_datetime` datetime NOT NULL default '0000-00-00 00:00:00',
  `filt_usage` int(11) NOT NULL default '0',
  `filt_usagedate` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`filt_id`),
  KEY `filt_usrid` (`filt_usrid`),
  KEY `filt_name` (`filt_title`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `languages`
--

CREATE TABLE `languages` (
  `lang_id` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `lang_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_name_org` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_level` int(11) NOT NULL default '0',
  PRIMARY KEY  (`lang_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='available languages';

-- --------------------------------------------------------

--
-- Table structure for table `subsys_mappings`
--

CREATE TABLE `subsys_mappings` (
  `map_roscms_userid` bigint(20) NOT NULL default '0',
  `map_subsys_name` varchar(10) character set utf8 NOT NULL default '',
  `map_subsys_userid` int(7) NOT NULL default '0',
  PRIMARY KEY  (`map_roscms_userid`,`map_subsys_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `usergroups`
--

CREATE TABLE `usergroups` (
  `usrgroup_name_id` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_intern_id` varchar(25) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_seclev` tinyint(4) NOT NULL default '0',
  `usrgroup_securitylevel` tinyint(4) NOT NULL default '0',
  `usrgroup_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `usrgroup_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  PRIMARY KEY  (`usrgroup_name_id`),
  UNIQUE KEY `usrgroup_name` (`usrgroup_name`),
  KEY `usrgroup_visible` (`usrgroup_visible`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `usergroups` VALUES('ros_admin', 'Administrator', 'roscms_usrgrp_admin', 3, 70, 'Administrator-Group: manage the ReactOS homepage (content, database)', '0');
INSERT INTO `usergroups` VALUES('user', 'User', 'roscms_usrgrp_user', 0, 0, 'Normal Visitors', '1');
INSERT INTO `usergroups` VALUES('developer', 'Developer', 'roscms_usrgrp_dev', 2, 50, 'ReactOS Developer: persons with svn commit access', '1');
INSERT INTO `usergroups` VALUES('moderator', 'Moderator', 'roscms_usrgrp_team', 2, 20, 'Homepage Moderators: for compatibility & package manager database, etc.', '1');
INSERT INTO `usergroups` VALUES('translator', 'Translator', 'roscms_usrgrp_trans', 1, 10, 'Homepage Translators: translate the homepage content', '1');
INSERT INTO `usergroups` VALUES('ros_sadmin', 'Super-Administrator', 'roscms_usrgrp_sadmin', 3, 100, 'Super-Administrator-Group: manage the ReactOS homepage (content, database, etc.); only persons who know what they are doing ...', '0');
INSERT INTO `usergroups` VALUES('mediateam', 'Media-Team', 'roscms_usrgrp_team', 1, 20, 'Media-Team members: UI-Team, etc.', '0');
INSERT INTO `usergroups` VALUES('test', 'Test-User', 'roscms_usrgrp_team', 1, 10, 'RosCMS tester group', '0');
INSERT INTO `usergroups` VALUES('transmaint', 'Language Maintainer', 'roscms_usrgrp_trans', 2, 15, 'Language Maintainer', '1');
INSERT INTO `usergroups` VALUES('newsletter', 'Newsletter Author', 'roscms_usrgrp_dev', 2, 50, 'ReactOS Newsletter Author', '1');

-- --------------------------------------------------------

--
-- Table structure for table `usergroup_members`
--

CREATE TABLE `usergroup_members` (
  `usergroupmember_userid` bigint(20) NOT NULL default '0',
  `usergroupmember_usergroupid` varchar(10) collate utf8_unicode_ci NOT NULL default 'user'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `users`
--

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
  `user_register_activation` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `user_fullname` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `user_email` varchar(150) collate utf8_unicode_ci NOT NULL default '',
  `user_email_activation` varchar(200) collate utf8_unicode_ci NOT NULL default '',
  `user_website` varchar(150) collate utf8_unicode_ci NOT NULL default '',
  `user_language` varchar(5) collate utf8_unicode_ci NOT NULL default '',
  `user_country` varchar(2) collate utf8_unicode_ci NOT NULL default '',
  `user_timezone` varchar(5) collate utf8_unicode_ci NOT NULL default '',
  `user_occupation` varchar(50) collate utf8_unicode_ci NOT NULL default '',
  `user_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `user_setting_multisession` varchar(5) collate utf8_unicode_ci NOT NULL default 'true',
  `user_setting_browseragent` varchar(5) collate utf8_unicode_ci NOT NULL default 'false',
  `user_setting_ipaddress` varchar(5) collate utf8_unicode_ci NOT NULL default 'false',
  `user_setting_timeout` varchar(5) collate utf8_unicode_ci NOT NULL default 'true',
  PRIMARY KEY  (`user_id`),
  UNIQUE KEY `user_name` (`user_name`),
  UNIQUE KEY `user_email` (`user_email`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RosCMS User Table';

INSERT INTO `users` VALUES (1, 'roscms_user', '', '', '', CURRENT_TIMESTAMP, 1, 'no', 'no', NULL, '', '', '', '', '', '', '', '', '', '', 'false', 'false', 'false', 'true');

-- --------------------------------------------------------

--
-- Table structure for table `user_countries`
--

CREATE TABLE `user_countries` (
  `coun_id` varchar(2) NOT NULL default '',
  `coun_name` varchar(100) NOT NULL default '',
  PRIMARY KEY  (`coun_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO `user_countries` VALUES('af', 'Afghanistan');
INSERT INTO `user_countries` VALUES('ax', 'Aland Islands');
INSERT INTO `user_countries` VALUES('al', 'Albania');
INSERT INTO `user_countries` VALUES('dz', 'Algeria');
INSERT INTO `user_countries` VALUES('as', 'American Samoa');
INSERT INTO `user_countries` VALUES('ad', 'Andorra');
INSERT INTO `user_countries` VALUES('ao', 'Angola');
INSERT INTO `user_countries` VALUES('ai', 'Anguilla');
INSERT INTO `user_countries` VALUES('aq', 'Antarctica');
INSERT INTO `user_countries` VALUES('ag', 'Antigua and Barbuda');
INSERT INTO `user_countries` VALUES('ar', 'Argentina');
INSERT INTO `user_countries` VALUES('am', 'Armenia');
INSERT INTO `user_countries` VALUES('aw', 'Aruba');
INSERT INTO `user_countries` VALUES('au', 'Australia');
INSERT INTO `user_countries` VALUES('at', 'Austria');
INSERT INTO `user_countries` VALUES('az', 'Azerbaijan');
INSERT INTO `user_countries` VALUES('bs', 'Bahamas');
INSERT INTO `user_countries` VALUES('bh', 'Bahrain');
INSERT INTO `user_countries` VALUES('bd', 'Bangladesh');
INSERT INTO `user_countries` VALUES('bb', 'Barbados');
INSERT INTO `user_countries` VALUES('by', 'Belarus');
INSERT INTO `user_countries` VALUES('be', 'Belgium');
INSERT INTO `user_countries` VALUES('bz', 'Belize');
INSERT INTO `user_countries` VALUES('bj', 'Benin');
INSERT INTO `user_countries` VALUES('bm', 'Bermuda');
INSERT INTO `user_countries` VALUES('bt', 'Bhutan');
INSERT INTO `user_countries` VALUES('bo', 'Bolivia');
INSERT INTO `user_countries` VALUES('ba', 'Bosnia and Herzegovina');
INSERT INTO `user_countries` VALUES('bw', 'Botswana');
INSERT INTO `user_countries` VALUES('bv', 'Bouvet Island');
INSERT INTO `user_countries` VALUES('br', 'Brazil');
INSERT INTO `user_countries` VALUES('io', 'British Indian Ocean Territory');
INSERT INTO `user_countries` VALUES('vg', 'British Virgin Islands');
INSERT INTO `user_countries` VALUES('bn', 'Brunei');
INSERT INTO `user_countries` VALUES('bg', 'Bulgaria');
INSERT INTO `user_countries` VALUES('bf', 'Burkina Faso');
INSERT INTO `user_countries` VALUES('bi', 'Burundi');
INSERT INTO `user_countries` VALUES('kh', 'Cambodia');
INSERT INTO `user_countries` VALUES('cm', 'Cameroon');
INSERT INTO `user_countries` VALUES('ca', 'Canada');
INSERT INTO `user_countries` VALUES('cv', 'Cape Verde');
INSERT INTO `user_countries` VALUES('ky', 'Cayman Islands');
INSERT INTO `user_countries` VALUES('cf', 'Central African Republic');
INSERT INTO `user_countries` VALUES('td', 'Chad');
INSERT INTO `user_countries` VALUES('cl', 'Chile');
INSERT INTO `user_countries` VALUES('cn', 'China');
INSERT INTO `user_countries` VALUES('cx', 'Christmas Island');
INSERT INTO `user_countries` VALUES('cc', 'Cocos (Keeling) Islands');
INSERT INTO `user_countries` VALUES('co', 'Colombia');
INSERT INTO `user_countries` VALUES('km', 'Comoros');
INSERT INTO `user_countries` VALUES('cg', 'Congo');
INSERT INTO `user_countries` VALUES('ck', 'Cook Islands');
INSERT INTO `user_countries` VALUES('cr', 'Costa Rica');
INSERT INTO `user_countries` VALUES('hr', 'Croatia');
INSERT INTO `user_countries` VALUES('cy', 'Cyprus');
INSERT INTO `user_countries` VALUES('cz', 'Czech Republic');
INSERT INTO `user_countries` VALUES('cd', 'Democratic Republic of Congo');
INSERT INTO `user_countries` VALUES('dk', 'Denmark');
INSERT INTO `user_countries` VALUES('xx', 'Disputed Territory');
INSERT INTO `user_countries` VALUES('dj', 'Djibouti');
INSERT INTO `user_countries` VALUES('dm', 'Dominica');
INSERT INTO `user_countries` VALUES('do', 'Dominican Republic');
INSERT INTO `user_countries` VALUES('tl', 'East Timor');
INSERT INTO `user_countries` VALUES('ec', 'Ecuador');
INSERT INTO `user_countries` VALUES('eg', 'Egypt');
INSERT INTO `user_countries` VALUES('sv', 'El Salvador');
INSERT INTO `user_countries` VALUES('gq', 'Equatorial Guinea');
INSERT INTO `user_countries` VALUES('er', 'Eritrea');
INSERT INTO `user_countries` VALUES('ee', 'Estonia');
INSERT INTO `user_countries` VALUES('et', 'Ethiopia');
INSERT INTO `user_countries` VALUES('fk', 'Falkland Islands');
INSERT INTO `user_countries` VALUES('fo', 'Faroe Islands');
INSERT INTO `user_countries` VALUES('fm', 'Federated States of Micronesia');
INSERT INTO `user_countries` VALUES('fj', 'Fiji');
INSERT INTO `user_countries` VALUES('fi', 'Finland');
INSERT INTO `user_countries` VALUES('fr', 'France');
INSERT INTO `user_countries` VALUES('gf', 'French Guyana');
INSERT INTO `user_countries` VALUES('pf', 'French Polynesia');
INSERT INTO `user_countries` VALUES('tf', 'French Southern Territories');
INSERT INTO `user_countries` VALUES('ga', 'Gabon');
INSERT INTO `user_countries` VALUES('gm', 'Gambia');
INSERT INTO `user_countries` VALUES('ge', 'Georgia');
INSERT INTO `user_countries` VALUES('de', 'Germany');
INSERT INTO `user_countries` VALUES('gh', 'Ghana');
INSERT INTO `user_countries` VALUES('gi', 'Gibraltar');
INSERT INTO `user_countries` VALUES('gr', 'Greece');
INSERT INTO `user_countries` VALUES('gl', 'Greenland');
INSERT INTO `user_countries` VALUES('gd', 'Grenada');
INSERT INTO `user_countries` VALUES('gp', 'Guadeloupe');
INSERT INTO `user_countries` VALUES('gu', 'Guam');
INSERT INTO `user_countries` VALUES('gt', 'Guatemala');
INSERT INTO `user_countries` VALUES('gn', 'Guinea');
INSERT INTO `user_countries` VALUES('gw', 'Guinea-Bissau');
INSERT INTO `user_countries` VALUES('gy', 'Guyana');
INSERT INTO `user_countries` VALUES('ht', 'Haiti');
INSERT INTO `user_countries` VALUES('hm', 'Heard Island and Mcdonald Islands');
INSERT INTO `user_countries` VALUES('hn', 'Honduras');
INSERT INTO `user_countries` VALUES('hk', 'Hong Kong');
INSERT INTO `user_countries` VALUES('hu', 'Hungary');
INSERT INTO `user_countries` VALUES('is', 'Iceland');
INSERT INTO `user_countries` VALUES('in', 'India');
INSERT INTO `user_countries` VALUES('id', 'Indonesia');
INSERT INTO `user_countries` VALUES('iq', 'Iraq');
INSERT INTO `user_countries` VALUES('xe', 'Iraq-Saudi Arabia Neutral Zone');
INSERT INTO `user_countries` VALUES('ie', 'Ireland');
INSERT INTO `user_countries` VALUES('il', 'Israel');
INSERT INTO `user_countries` VALUES('it', 'Italy');
INSERT INTO `user_countries` VALUES('ci', 'Ivory Coast');
INSERT INTO `user_countries` VALUES('jm', 'Jamaica');
INSERT INTO `user_countries` VALUES('jp', 'Japan');
INSERT INTO `user_countries` VALUES('jo', 'Jordan');
INSERT INTO `user_countries` VALUES('kz', 'Kazakhstan');
INSERT INTO `user_countries` VALUES('ke', 'Kenya');
INSERT INTO `user_countries` VALUES('ki', 'Kiribati');
INSERT INTO `user_countries` VALUES('kw', 'Kuwait');
INSERT INTO `user_countries` VALUES('kg', 'Kyrgyzstan');
INSERT INTO `user_countries` VALUES('la', 'Laos');
INSERT INTO `user_countries` VALUES('lv', 'Latvia');
INSERT INTO `user_countries` VALUES('lb', 'Lebanon');
INSERT INTO `user_countries` VALUES('ls', 'Lesotho');
INSERT INTO `user_countries` VALUES('lr', 'Liberia');
INSERT INTO `user_countries` VALUES('ly', 'Libya');
INSERT INTO `user_countries` VALUES('li', 'Liechtenstein');
INSERT INTO `user_countries` VALUES('lt', 'Lithuania');
INSERT INTO `user_countries` VALUES('lu', 'Luxembourg');
INSERT INTO `user_countries` VALUES('mo', 'Macau');
INSERT INTO `user_countries` VALUES('mk', 'Macedonia');
INSERT INTO `user_countries` VALUES('mg', 'Madagascar');
INSERT INTO `user_countries` VALUES('mw', 'Malawi');
INSERT INTO `user_countries` VALUES('my', 'Malaysia');
INSERT INTO `user_countries` VALUES('mv', 'Maldives');
INSERT INTO `user_countries` VALUES('ml', 'Mali');
INSERT INTO `user_countries` VALUES('mt', 'Malta');
INSERT INTO `user_countries` VALUES('mh', 'Marshall Islands');
INSERT INTO `user_countries` VALUES('mq', 'Martinique');
INSERT INTO `user_countries` VALUES('mr', 'Mauritania');
INSERT INTO `user_countries` VALUES('mu', 'Mauritius');
INSERT INTO `user_countries` VALUES('yt', 'Mayotte');
INSERT INTO `user_countries` VALUES('mx', 'Mexico');
INSERT INTO `user_countries` VALUES('md', 'Moldova');
INSERT INTO `user_countries` VALUES('mc', 'Monaco');
INSERT INTO `user_countries` VALUES('mn', 'Mongolia');
INSERT INTO `user_countries` VALUES('ms', 'Montserrat');
INSERT INTO `user_countries` VALUES('ma', 'Morocco');
INSERT INTO `user_countries` VALUES('mz', 'Mozambique');
INSERT INTO `user_countries` VALUES('mm', 'Myanmar');
INSERT INTO `user_countries` VALUES('na', 'Namibia');
INSERT INTO `user_countries` VALUES('nr', 'Nauru');
INSERT INTO `user_countries` VALUES('np', 'Nepal');
INSERT INTO `user_countries` VALUES('nl', 'Netherlands');
INSERT INTO `user_countries` VALUES('an', 'Netherlands Antilles');
INSERT INTO `user_countries` VALUES('nc', 'New Caledonia');
INSERT INTO `user_countries` VALUES('nz', 'New Zealand');
INSERT INTO `user_countries` VALUES('ni', 'Nicaragua');
INSERT INTO `user_countries` VALUES('ne', 'Niger');
INSERT INTO `user_countries` VALUES('ng', 'Nigeria');
INSERT INTO `user_countries` VALUES('nu', 'Niue');
INSERT INTO `user_countries` VALUES('nf', 'Norfolk Island');
INSERT INTO `user_countries` VALUES('kp', 'North Korea');
INSERT INTO `user_countries` VALUES('mp', 'Northern Mariana Islands');
INSERT INTO `user_countries` VALUES('no', 'Norway');
INSERT INTO `user_countries` VALUES('om', 'Oman');
INSERT INTO `user_countries` VALUES('pk', 'Pakistan');
INSERT INTO `user_countries` VALUES('pw', 'Palau');
INSERT INTO `user_countries` VALUES('ps', 'Palestinian Occupied Territories');
INSERT INTO `user_countries` VALUES('pa', 'Panama');
INSERT INTO `user_countries` VALUES('pg', 'Papua New Guinea');
INSERT INTO `user_countries` VALUES('py', 'Paraguay');
INSERT INTO `user_countries` VALUES('pe', 'Peru');
INSERT INTO `user_countries` VALUES('ph', 'Philippines');
INSERT INTO `user_countries` VALUES('pn', 'Pitcairn Islands');
INSERT INTO `user_countries` VALUES('pl', 'Poland');
INSERT INTO `user_countries` VALUES('pt', 'Portugal');
INSERT INTO `user_countries` VALUES('pr', 'Puerto Rico');
INSERT INTO `user_countries` VALUES('qa', 'Qatar');
INSERT INTO `user_countries` VALUES('re', 'Reunion');
INSERT INTO `user_countries` VALUES('ro', 'Romania');
INSERT INTO `user_countries` VALUES('ru', 'Russia');
INSERT INTO `user_countries` VALUES('rw', 'Rwanda');
INSERT INTO `user_countries` VALUES('sh', 'Saint Helena and Dependencies');
INSERT INTO `user_countries` VALUES('kn', 'Saint Kitts and Nevis');
INSERT INTO `user_countries` VALUES('lc', 'Saint Lucia');
INSERT INTO `user_countries` VALUES('pm', 'Saint Pierre and Miquelon');
INSERT INTO `user_countries` VALUES('vc', 'Saint Vincent and the Grenadines');
INSERT INTO `user_countries` VALUES('ws', 'Samoa');
INSERT INTO `user_countries` VALUES('sm', 'San Marino');
INSERT INTO `user_countries` VALUES('st', 'Sao Tome and Principe');
INSERT INTO `user_countries` VALUES('sa', 'Saudi Arabia');
INSERT INTO `user_countries` VALUES('sn', 'Senegal');
INSERT INTO `user_countries` VALUES('cs', 'Serbia and Montenegro');
INSERT INTO `user_countries` VALUES('sc', 'Seychelles');
INSERT INTO `user_countries` VALUES('sl', 'Sierra Leone');
INSERT INTO `user_countries` VALUES('sg', 'Singapore');
INSERT INTO `user_countries` VALUES('sk', 'Slovakia');
INSERT INTO `user_countries` VALUES('si', 'Slovenia');
INSERT INTO `user_countries` VALUES('sb', 'Solomon Islands');
INSERT INTO `user_countries` VALUES('so', 'Somalia');
INSERT INTO `user_countries` VALUES('za', 'South Africa');
INSERT INTO `user_countries` VALUES('gs', 'South Georgia and South Sandwich Islands');
INSERT INTO `user_countries` VALUES('kr', 'South Korea');
INSERT INTO `user_countries` VALUES('es', 'Spain');
INSERT INTO `user_countries` VALUES('pi', 'Spratly Islands');
INSERT INTO `user_countries` VALUES('lk', 'Sri Lanka');
INSERT INTO `user_countries` VALUES('sr', 'Suriname');
INSERT INTO `user_countries` VALUES('sj', 'Svalbard and Jan Mayen');
INSERT INTO `user_countries` VALUES('sz', 'Swaziland');
INSERT INTO `user_countries` VALUES('se', 'Sweden');
INSERT INTO `user_countries` VALUES('ch', 'Switzerland');
INSERT INTO `user_countries` VALUES('sy', 'Syria');
INSERT INTO `user_countries` VALUES('tw', 'Taiwan');
INSERT INTO `user_countries` VALUES('tj', 'Tajikistan');
INSERT INTO `user_countries` VALUES('tz', 'Tanzania');
INSERT INTO `user_countries` VALUES('th', 'Thailand');
INSERT INTO `user_countries` VALUES('tg', 'Togo');
INSERT INTO `user_countries` VALUES('tk', 'Tokelau');
INSERT INTO `user_countries` VALUES('to', 'Tonga');
INSERT INTO `user_countries` VALUES('tt', 'Trinidad and Tobago');
INSERT INTO `user_countries` VALUES('tn', 'Tunisia');
INSERT INTO `user_countries` VALUES('tr', 'Turkey');
INSERT INTO `user_countries` VALUES('tm', 'Turkmenistan');
INSERT INTO `user_countries` VALUES('tc', 'Turks And Caicos Islands');
INSERT INTO `user_countries` VALUES('tv', 'Tuvalu');
INSERT INTO `user_countries` VALUES('ug', 'Uganda');
INSERT INTO `user_countries` VALUES('ua', 'Ukraine');
INSERT INTO `user_countries` VALUES('ae', 'United Arab Emirates');
INSERT INTO `user_countries` VALUES('uk', 'United Kingdom');
INSERT INTO `user_countries` VALUES('xd', 'United Nations Neutral Zone');
INSERT INTO `user_countries` VALUES('us', 'United States');
INSERT INTO `user_countries` VALUES('um', 'United States Minor Outlying Islands');
INSERT INTO `user_countries` VALUES('uy', 'Uruguay');
INSERT INTO `user_countries` VALUES('vi', 'US Virgin Islands');
INSERT INTO `user_countries` VALUES('uz', 'Uzbekistan');
INSERT INTO `user_countries` VALUES('vu', 'Vanuatu');
INSERT INTO `user_countries` VALUES('va', 'Vatican City');
INSERT INTO `user_countries` VALUES('ve', 'Venezuela');
INSERT INTO `user_countries` VALUES('vn', 'Vietnam');
INSERT INTO `user_countries` VALUES('wf', 'Wallis and Futuna');
INSERT INTO `user_countries` VALUES('eh', 'Western Sahara');
INSERT INTO `user_countries` VALUES('ye', 'Yemen');
INSERT INTO `user_countries` VALUES('zm', 'Zambia');
INSERT INTO `user_countries` VALUES('zw', 'Zimbabwe');

-- --------------------------------------------------------

--
-- Table structure for table `user_language`
--

CREATE TABLE `user_language` (
  `lang_id` varchar(5) NOT NULL default '',
  `lang_name` varchar(100) NOT NULL default '',
  PRIMARY KEY  (`lang_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `user_sessions`
--

CREATE TABLE `user_sessions` (
  `usersession_id` varchar(32) character set utf8 NOT NULL default '' COMMENT 'Unique ID of this session',
  `usersession_user_id` bigint(20) NOT NULL default '0' COMMENT 'User this session belongs to',
  `usersession_expires` datetime default NULL COMMENT 'Expiry date/time (NULL if does not expire)',
  `usersession_browseragent` varchar(255) character set utf8 NOT NULL default '' COMMENT 'HTTP_USER_AGENT when this session was created',
  `usersession_ipaddress` varchar(15) character set utf8 NOT NULL default '' COMMENT 'IP address from which this session was created',
  PRIMARY KEY  (`usersession_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci PACK_KEYS=0 COMMENT='Sessions';

-- --------------------------------------------------------

--
-- Table structure for table `user_timezone`
--

CREATE TABLE `user_timezone` (
  `tz_code` varchar(10) NOT NULL default '',
  `tz_name` varchar(50) NOT NULL default '',
  `tz_value` double NOT NULL default '0',
  `tz_value2` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`tz_code`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO `user_timezone` VALUES('IDLW', 'International Date Line West', -12, '-1200');
INSERT INTO `user_timezone` VALUES('NT', 'Samoa', -11, '-1100');
INSERT INTO `user_timezone` VALUES('HAST', 'Hawaii-Aleutian', -10, '-1000');
INSERT INTO `user_timezone` VALUES('AKST', 'Alaska', -9, '-0900');
INSERT INTO `user_timezone` VALUES('PST', 'Pacific (America)', -8, '-0800');
INSERT INTO `user_timezone` VALUES('MST', 'Mountain (America)', -7, '-0700');
INSERT INTO `user_timezone` VALUES('CST', 'Central (America)', -6, '-0600');
INSERT INTO `user_timezone` VALUES('EST', 'Eastern (America)', -5, '-0500');
INSERT INTO `user_timezone` VALUES('AST', 'Atlantic (America)', -4, '-0400');
INSERT INTO `user_timezone` VALUES('NST', 'Newfoundland', -3.5, '-0330');
INSERT INTO `user_timezone` VALUES('GST', 'Greenland', -3, '-0300');
INSERT INTO `user_timezone` VALUES('AT', 'Azores', -2, '-0200');
INSERT INTO `user_timezone` VALUES('WAT', 'West Africa', -1, '-0100');
INSERT INTO `user_timezone` VALUES('UTC', 'Universal Coordinated', 0, '+0000');
INSERT INTO `user_timezone` VALUES('GMT', 'Greenwich Mean', 0, '+0000');
INSERT INTO `user_timezone` VALUES('WET', 'Western European', 0, '+0000');
INSERT INTO `user_timezone` VALUES('CET', 'Central European', 1, '+0100');
INSERT INTO `user_timezone` VALUES('EET', 'Eastern European', 2, '+0200');
INSERT INTO `user_timezone` VALUES('MSK', 'Moscow', 3, '+0300');
INSERT INTO `user_timezone` VALUES('IRT', 'Iran', 3.5, '+0330');
INSERT INTO `user_timezone` VALUES('ZP4', 'Russia, United Arab Emirates', 4, '+0400');
INSERT INTO `user_timezone` VALUES('ZP5', 'Russia', 5, '+0500');
INSERT INTO `user_timezone` VALUES('IST', 'Indian', 5.5, '+0530');
INSERT INTO `user_timezone` VALUES('ZP6', 'Russia', 6, '+0600');
INSERT INTO `user_timezone` VALUES('ICT', 'Indochina', 7, '+0700');
INSERT INTO `user_timezone` VALUES('CCT', 'China', 8, '+0800');
INSERT INTO `user_timezone` VALUES('JST', 'Japan, Korea', 9, '+0900');
INSERT INTO `user_timezone` VALUES('ACST', 'Australian Central', 9.5, '+0930');
INSERT INTO `user_timezone` VALUES('AEST', 'Australian Eastern', 10, '+1000');
INSERT INTO `user_timezone` VALUES('MAGS', 'Magadan', 11, '+1100');
INSERT INTO `user_timezone` VALUES('NZST', 'New Zealand', 12, '+1200');
INSERT INTO `user_timezone` VALUES('IDLE', 'International Date Line East', 12, '+1200');
INSERT INTO `user_timezone` VALUES('AWST', 'Western Australian', 8, '+0800');
INSERT INTO `user_timezone` VALUES('BT', 'Eastern Africa', 3, '+0300');

-- --------------------------------------------------------

--
-- Table structure for table `user_unsafenames`
--

CREATE TABLE `user_unsafenames` (
  `unsafe_name` varchar(20) NOT NULL default '',
  `unsafe_part` char(1) NOT NULL default '1',
  PRIMARY KEY  (`unsafe_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `user_unsafepwds`
--

CREATE TABLE `user_unsafepwds` (
  `pwd_name` varchar(100) NOT NULL default '',
  PRIMARY KEY  (`pwd_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
