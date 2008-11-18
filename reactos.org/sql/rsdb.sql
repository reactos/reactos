-- 
-- Database: `rsdb`
-- 

-- --------------------------------------------------------

-- 
--  `rsdb_categories`
-- 

CREATE TABLE `rsdb_categories` (
  `cat_id` bigint(20) NOT NULL auto_increment COMMENT 'Category ID',
  `cat_name` varchar(100) collate utf8_unicode_ci NOT NULL default '' COMMENT 'Category Name',
  `cat_visible` char(1) collate utf8_unicode_ci NOT NULL default '1' COMMENT 'Category Visibility (0/1)',
  `cat_description` varchar(255) collate utf8_unicode_ci NOT NULL default '' COMMENT 'Category Description',
  `cat_path` bigint(20) NOT NULL default '0' COMMENT 'Category Path-Parent (tree)',
  `cat_order` int(11) NOT NULL default '0' COMMENT 'Category Order (for Dev Network)',
  `cat_viewcounter` bigint(20) NOT NULL default '0' COMMENT 'Category User Click Counter',
  `cat_icon` varchar(100) collate utf8_unicode_ci NOT NULL default '' COMMENT 'Category Icon (optional)',
  `cat_comp` char(1) collate utf8_unicode_ci NOT NULL default '1' COMMENT 'Category Compatibility View (0/1)',
  `cat_pack` char(1) collate utf8_unicode_ci NOT NULL default '1' COMMENT 'Category Packages View (0/1)',
  `cat_devnet` char(1) collate utf8_unicode_ci NOT NULL default '0' COMMENT 'Category Developer Network View (0/1)',
  `cat_media` char(1) collate utf8_unicode_ci NOT NULL default '0' COMMENT 'Category Media View (0/1)',
  PRIMARY KEY  (`cat_id`),
  KEY `cat_order` (`cat_order`),
  KEY `cat_name` (`cat_name`),
  KEY `cat_name_2` (`cat_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci PACK_KEYS=0 COMMENT='Categories';

-- --------------------------------------------------------

-- 
--  `rsdb_group_bundles`
-- 

CREATE TABLE `rsdb_group_bundles` (
  `bundle_id` bigint(20) NOT NULL default '0',
  `bundle_groupid` bigint(20) NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Group Bundles';

-- --------------------------------------------------------

-- 
--  `rsdb_groups`
-- 

CREATE TABLE `rsdb_groups` (
  `grpentr_id` bigint(20) NOT NULL auto_increment,
  `grpentr_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `grpentr_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `grpentr_category` bigint(20) NOT NULL default '0',
  `grpentr_type` varchar(10) collate utf8_unicode_ci NOT NULL default 'default',
  `grpentr_vendor` bigint(20) NOT NULL default '0',
  `grpentr_pic` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `grpentr_viewcounter` bigint(20) NOT NULL default '0',
  `grpentr_order` int(11) NOT NULL default '0',
  `grpentr_icon` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `grpentr_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `grpentr_comp` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `grpentr_pack` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `grpentr_devnet` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `grpentr_media` char(1) collate utf8_unicode_ci NOT NULL default '0',
  `grpentr_usrid` bigint(20) NOT NULL default '0',
  `grpentr_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `grpentr_checked` varchar(10) collate utf8_unicode_ci NOT NULL default 'no',
  PRIMARY KEY  (`grpentr_id`),
  KEY `grpentr_name` (`grpentr_name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Group Entries';

-- --------------------------------------------------------

-- 
--  `rsdb_item_comp`
-- 

CREATE TABLE `rsdb_item_comp` (
  `comp_id` bigint(20) NOT NULL auto_increment,
  `comp_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `comp_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `comp_groupid` bigint(20) NOT NULL default '0',
  `comp_viewcounter` bigint(20) NOT NULL default '0',
  `comp_appversion` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `comp_osversion` varchar(6) collate utf8_unicode_ci NOT NULL default '000000',
  `comp_status` tinyint(4) NOT NULL default '0',
  `comp_rosversion` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `comp_rosrevision` bigint(20) NOT NULL default '0',
  `comp_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `comp_media` bigint(20) NOT NULL default '0' COMMENT 'Screenshots',
  `comp_infotext` text collate utf8_unicode_ci NOT NULL,
  `comp_award` smallint(6) NOT NULL default '0',
  `comp_usrid` bigint(20) NOT NULL default '0',
  `comp_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `comp_checked` varchar(10) collate utf8_unicode_ci NOT NULL default 'no',
  PRIMARY KEY  (`comp_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Compatibility';

-- --------------------------------------------------------

-- 
--  `rsdb_item_comp_forum`
-- 

CREATE TABLE `rsdb_item_comp_forum` (
  `fmsg_id` bigint(20) NOT NULL auto_increment,
  `fmsg_comp_id` bigint(20) NOT NULL default '0',
  `fmsg_parent` bigint(20) NOT NULL default '0',
  `fmsg_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `fmsg_subject` varchar(250) collate utf8_unicode_ci NOT NULL default '',
  `fmsg_body` text collate utf8_unicode_ci NOT NULL,
  `fmsg_user_id` bigint(20) NOT NULL default '0',
  `fmsg_user_ip` varchar(250) collate utf8_unicode_ci NOT NULL default '',
  `fmsg_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `fmsg_useful_vote_value` int(11) NOT NULL default '0',
  `fmsg_useful_vote_user` int(11) NOT NULL default '0',
  `fmsg_useful_vote_user_history` text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`fmsg_id`),
  KEY `item_id` (`fmsg_comp_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Compatibility Forum';

-- --------------------------------------------------------

-- 
--  `rsdb_item_comp_testresults`
-- 

CREATE TABLE `rsdb_item_comp_testresults` (
  `test_id` bigint(20) NOT NULL auto_increment,
  `test_comp_id` bigint(20) NOT NULL default '0',
  `test_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `test_com_version` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `test_comp_revision` int(11) NOT NULL default '0',
  `test_whatworks` text collate utf8_unicode_ci NOT NULL,
  `test_whatdoesntwork` text collate utf8_unicode_ci NOT NULL,
  `test_whatnottested` text collate utf8_unicode_ci NOT NULL,
  `test_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `test_result_install` tinyint(4) NOT NULL default '0',
  `test_result_function` tinyint(4) NOT NULL default '0',
  `test_user_comment` text collate utf8_unicode_ci NOT NULL,
  `test_conclusion` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `test_user_id` bigint(20) NOT NULL default '0',
  `test_user_submit_timestamp` timestamp NULL default CURRENT_TIMESTAMP,
  `test_useful_vote_value` int(11) NOT NULL default '0',
  `test_useful_vote_user` int(11) NOT NULL default '0',
  `test_useful_vote_user_history` text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`test_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Compatibility Test Results';

-- --------------------------------------------------------

-- 
--  `rsdb_item_devnet`
-- 

CREATE TABLE `rsdb_item_devnet` (
  `devnet_id` bigint(20) NOT NULL auto_increment,
  `devnet_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `devnet_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `devnet_groupid` bigint(20) NOT NULL default '0',
  `devnet_viewcounter` bigint(20) NOT NULL default '0',
  `devnet_order` int(11) NOT NULL default '0',
  `devnet_version` int(11) NOT NULL default '0',
  `devnet_description` text collate utf8_unicode_ci NOT NULL,
  `devnet_text` longtext collate utf8_unicode_ci NOT NULL,
  `devnet_osversions` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`devnet_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Developer Network Entries';

-- --------------------------------------------------------

-- 
--  `rsdb_item_pack`
-- 

CREATE TABLE `rsdb_item_pack` (
  `pack_id` bigint(20) NOT NULL auto_increment,
  `pack_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `pack_shortname` varchar(15) collate utf8_unicode_ci NOT NULL default '',
  `pack_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `pack_groupid` bigint(20) NOT NULL default '0',
  `pack_viewcounter` bigint(20) NOT NULL default '0',
  `pack_appversion` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `pack_packmgrversion` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `pack_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`pack_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Packages';

-- --------------------------------------------------------

-- 
--  `rsdb_item_vendor`
-- 

CREATE TABLE `rsdb_item_vendor` (
  `vendor_id` bigint(20) NOT NULL auto_increment,
  `vendor_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `vendor_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `vendor_fullname` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `vendor_url` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `vendor_email` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `vendor_infotext` text collate utf8_unicode_ci NOT NULL,
  `vendor_problem` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `vendor_usrid` bigint(20) NOT NULL default '0',
  `vendor_usrip` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `vendor_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `vendor_checked` varchar(10) collate utf8_unicode_ci NOT NULL default 'no',
  PRIMARY KEY  (`vendor_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Vendor Information';

-- --------------------------------------------------------

-- 
--  `rsdb_languages`
-- 

CREATE TABLE `rsdb_languages` (
  `lang_id` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `lang_name` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `lang_level` int(11) NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='available languages';

-- --------------------------------------------------------

-- 
--  `rsdb_logs`
-- 

CREATE TABLE `rsdb_logs` (
  `log_id` bigint(20) NOT NULL auto_increment,
  `log_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `log_usrid` bigint(20) NOT NULL default '0',
  `log_usrip` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `log_level` varchar(10) collate utf8_unicode_ci NOT NULL default 'high',
  `log_action` varchar(20) collate utf8_unicode_ci NOT NULL default '',
  `log_title` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `log_description` text collate utf8_unicode_ci NOT NULL,
  `log_category` varchar(100) collate utf8_unicode_ci NOT NULL default '',
  `log_badusr` bigint(20) NOT NULL default '0',
  `log_referrer` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `log_browseragent` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `log_read` text collate utf8_unicode_ci NOT NULL,
  `log_taskdone_usr` bigint(20) NOT NULL default '0',
  PRIMARY KEY  (`log_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RSDB Logs';

-- --------------------------------------------------------

-- 
--  `rsdb_object_description`
-- 

CREATE TABLE `rsdb_object_description` (
  `desc_id` bigint(20) NOT NULL default '0',
  `desc_visible` char(1) collate utf8_unicode_ci NOT NULL default '',
  `desc_text` text collate utf8_unicode_ci NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RSDB Object "Description"';

-- --------------------------------------------------------

-- 
--  `rsdb_object_media`
-- 

CREATE TABLE `rsdb_object_media` (
  `media_id` bigint(20) NOT NULL auto_increment,
  `media_groupid` bigint(20) NOT NULL default '0',
  `media_visible` char(1) collate utf8_unicode_ci NOT NULL default '1',
  `media_order` int(11) NOT NULL default '0',
  `media_file` varchar(250) collate utf8_unicode_ci NOT NULL default '',
  `media_filetype` varchar(50) collate utf8_unicode_ci NOT NULL default 'picture',
  `media_thumbnail` varchar(250) collate utf8_unicode_ci NOT NULL default '',
  `media_description` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `media_exif` text collate utf8_unicode_ci NOT NULL,
  `media_date` datetime NOT NULL default '0000-00-00 00:00:00',
  `media_user_id` bigint(20) NOT NULL default '0',
  `media_user_ip` varchar(250) collate utf8_unicode_ci NOT NULL default '',
  `media_useful_vote_value` int(11) NOT NULL default '0',
  `media_useful_vote_user` int(11) NOT NULL default '0',
  `media_useful_vote_user_history` text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`media_id`),
  KEY `media_groupid` (`media_groupid`),
  KEY `media_filetype` (`media_filetype`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Media Files';

-- --------------------------------------------------------

-- 
--  `rsdb_object_osversions`
-- 

CREATE TABLE `rsdb_object_osversions` (
  `ver_id` int(11) NOT NULL auto_increment,
  `ver_name` varchar(25) collate utf8_unicode_ci NOT NULL default '',
  `ver_value` varchar(6) collate utf8_unicode_ci NOT NULL default '',
  `ver_visible` char(1) collate utf8_unicode_ci NOT NULL default '0',
  PRIMARY KEY  (`ver_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='Operating System Version Numbers';

-- --------------------------------------------------------

-- 
--  `rsdb_stats`
-- 

CREATE TABLE `rsdb_stats` (
  `stat_date` date NOT NULL default '0000-00-00' COMMENT 'Date',
  `stat_pviews` int(11) NOT NULL default '0' COMMENT 'Page Views',
  `stat_visitors` int(11) NOT NULL default '0' COMMENT 'Visitors',
  `stat_users` int(11) NOT NULL default '0' COMMENT 'Registered Users',
  `stat_s_cat` int(11) NOT NULL default '0' COMMENT 'Submit Categories',
  `stat_s_grp` int(11) NOT NULL default '0' COMMENT 'Submit Groups',
  `stat_s_icomp` int(11) NOT NULL default '0' COMMENT 'Submit Comp-Items',
  `stat_s_ictest` int(11) NOT NULL default '0' COMMENT 'Submit Comp-Tests',
  `stat_s_icbb` int(11) NOT NULL default '0' COMMENT 'Submit Comp-Forum-entries',
  `stat_s_icvotes` int(11) NOT NULL default '0' COMMENT 'Submit Comp votes',
  `stat_s_media` int(11) NOT NULL default '0' COMMENT 'Submit Media',
  `stat_s_votes` int(11) NOT NULL default '0' COMMENT 'Submit Votes Total',
  `stat_vislst` text collate utf8_unicode_ci NOT NULL COMMENT 'Visitors List',
  `stat_usrlst` text collate utf8_unicode_ci NOT NULL COMMENT 'User ID List',
  `stat_brow_IE` int(11) NOT NULL default '0' COMMENT 'IE counter',
  `stat_brow_MOZ` int(11) NOT NULL default '0' COMMENT 'Mozilla counter',
  `stat_brow_OPERA` int(11) NOT NULL default '0' COMMENT 'Opera counter',
  `stat_brow_KHTML` int(11) NOT NULL default '0',
  `stat_brow_text` int(11) NOT NULL default '0',
  `stat_brow_other` int(11) NOT NULL default '0',
  `stat_os_winnt` int(11) NOT NULL default '0',
  `stat_os_ros` int(11) NOT NULL default '0',
  `stat_os_unix` int(11) NOT NULL default '0',
  `stat_os_bsd` int(11) NOT NULL default '0',
  `stat_os_linux` int(11) NOT NULL default '0',
  `stat_os_mac` int(11) NOT NULL default '0',
  `stat_os_other` int(11) NOT NULL default '0',
  `stat_reflst` text collate utf8_unicode_ci NOT NULL COMMENT 'Referrers list',
  PRIMARY KEY  (`stat_date`),
  UNIQUE KEY `stat_date` (`stat_date`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='RSDB Stats';

-- --------------------------------------------------------

-- 
--  `rsdb_urls`
-- 

CREATE TABLE `rsdb_urls` (
  `url_id` bigint(20) NOT NULL auto_increment,
  `url_t` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `url_u` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `url_a` bigint(20) NOT NULL default '0',
  `url_i` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `url_s` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  `url_lang` varchar(10) collate utf8_unicode_ci NOT NULL default '',
  `url_browser` varchar(255) collate utf8_unicode_ci NOT NULL default '',
  PRIMARY KEY  (`url_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='URL logs';
