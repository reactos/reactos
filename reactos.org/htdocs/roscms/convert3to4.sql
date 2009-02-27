
SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";



-- --------------------------------------------------------
-- create and convert countries
-- --------------------------------------------------------
CREATE TABLE roscms_countries (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(64) collate utf8_unicode_ci NOT NULL,
  name_short varchar(2) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_countries
SELECT
  NULL,
  coun_name,
  coun_id
FROM user_countries
ORDER BY coun_name ASC;



-- --------------------------------------------------------
-- create and convert languages
-- --------------------------------------------------------
CREATE TABLE roscms_languages (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(64) collate utf8_unicode_ci NOT NULL,
  name_short varchar(8) collate utf8_unicode_ci NOT NULL,
  name_original varchar(64) collate utf8_unicode_ci NOT NULL,
  level int(10) unsigned NOT NULL,
  PRIMARY KEY  (id),
  UNIQUE KEY `name` (`name`),
  UNIQUE KEY name_short (name_short)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_languages
SELECT
  NULL,
  lang_name,
  lang_id,
  lang_name_org,
  lang_level
FROM languages
ORDER BY lang_name ASC;



-- --------------------------------------------------------
-- create and convert filters
-- --------------------------------------------------------
CREATE TABLE roscms_filter (
  id bigint(20) unsigned NOT NULL auto_increment,
  user_id bigint(20) NOT NULL COMMENT '->accounts(id); -1=system',
  type ENUM( 'entry', 'user' ) NOT NULL,
  name varchar(32) collate utf8_unicode_ci NOT NULL,
  setting tinytext collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id),
  UNIQUE KEY user_id_2 (user_id, name),
  KEY user_id (user_id),
  KEY name (name)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- only include smart filters (labels aren't used by now and will be implemented by tags)
INSERT INTO roscms_filter
SELECT
  NULL,
  filt_usrid,
  'entry',
  filt_title,
  filt_string
FROM data_user_filter WHERE filt_type = 1
ORDER BY filt_usrid ASC, filt_title ASC;



-- --------------------------------------------------------
-- create and convert groups
-- --------------------------------------------------------
CREATE TABLE roscms_groups (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(32) collate utf8_unicode_ci NOT NULL,
  name_short varchar(10) collate utf8_unicode_ci NOT NULL,
  security_level tinyint(3) unsigned NOT NULL,
  description varchar(255) collate utf8_unicode_ci NOT NULL,
  visible tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY name (name),
  UNIQUE KEY name_short (name_short),
  KEY security_level (security_level)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_groups
SELECT
  NULL,
  usrgroup_name,
  usrgroup_name_id,
  usrgroup_seclev,
  usrgroup_description,
  usrgroup_visible
FROM usergroups
ORDER BY usrgroup_name;



-- --------------------------------------------------------
-- create dependency table (nothing to convert, as it's new and will be filled later)
-- --------------------------------------------------------
CREATE TABLE roscms_rel_revisions_dependencies (
  id bigint(20) NOT NULL auto_increment,
  rev_id bigint(20) unsigned NOT NULL COMMENT '->revisions(id)',
  child_id bigint(20) unsigned default NULL COMMENT '->entries(id)',
  child_name varchar(80) collate utf8_unicode_ci default NULL,
  include tinyint(1) NOT NULL default '0',
  user_defined tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (id),
  KEY rev_id (rev_id),
  KEY child_id (child_id),
  KEY child_name (child_name)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='if dependency_id is NULL-> set name & type';



-- --------------------------------------------------------
-- create and convert access lists (allowed groups are stored in rel_groups_access)
-- --------------------------------------------------------
CREATE TABLE roscms_entries_access (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(100) collate utf8_unicode_ci NOT NULL,
  name_short varchar(50) collate utf8_unicode_ci NOT NULL,
  description varchar(255) collate utf8_unicode_ci NOT NULL,
  standard tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY name (name),
  UNIQUE KEY name_short (name_short)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_entries_access
SELECT
  NULL,
  sec_fullname,
  sec_name,
  sec_description,
  FALSE
FROM data_security
ORDER BY sec_name;

UPDATE roscms_entries_access SET standard=TRUE WHERE name_short='default';


-- --------------------------------------------------------
-- table for entry areas
-- --------------------------------------------------------
CREATE TABLE roscms_entries_areas (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(30) NOT NULL,
  name_short varchar(15) NOT NULL,
  description varchar(255) NOT NULL,
  PRIMARY KEY  (id),
  UNIQUE KEY name_short (name_short),
  UNIQUE KEY `name` (name)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO roscms_entries_areas VALUES
(1, 'Translate', 'translate', 'user can translate this entry to the lang he has set in his profile'),
(2, 'Edit', 'edit', 'modify content of this entry'),
(3, 'View History', 'history', 'view History tab'),
(4, 'View Fields', 'fields', 'view fields tab'),
(5, 'View Revision Tab', 'revision', 'view revision tab'),
(6, 'View Dependencies', 'dependencies', 'view dependencies tab'),
(7, 'System metadata', 'system_meta', 'modify System metadata'),
(8, 'Add Fields', 'add_fields', 'add new text fields'),
(9, 'Read', 'read', 'can view this entry');



-- --------------------------------------------------------
-- table for acl
-- --------------------------------------------------------
CREATE TABLE roscms_rel_acl (
  id bigint(20) unsigned NOT NULL auto_increment,
  right_id bigint(20) unsigned NOT NULL COMMENT '->entries_areas(id)',
  access_id bigint(20) unsigned NOT NULL COMMENT '->entries_access(id)',
  group_id bigint(20) unsigned NOT NULL COMMENT '->groups(id)',
  PRIMARY KEY  (id),
  UNIQUE KEY right_id (right_id,access_id,group_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- convert table
INSERT INTO roscms_rel_acl
SELECT DISTINCT
  NULL,
  r.id,
  a.id,
  g.id
FROM roscms_entries_access a JOIN data_security s ON a.name_short=s.sec_name JOIN roscms_groups g JOIN roscms_entries_areas r WHERE (((
   (g.security_level = 1 AND s.sec_lev1_read = 1 AND r.name_short='read')
OR (g.security_level = 1 AND s.sec_lev1_write = 1 AND r.name_short='edit')
OR (g.security_level = 1 AND s.sec_lev1_add = 1 AND r.name_short='add_fields')
OR (g.security_level = 1 AND s.sec_lev1_trans = 1 AND r.name_short='translate')

OR (g.security_level = 2 AND s.sec_lev2_read = 1 AND r.name_short='read')
OR (g.security_level = 2 AND s.sec_lev2_write = 1 AND r.name_short='edit')
OR (g.security_level = 2 AND s.sec_lev2_add = 1 AND r.name_short='add_fields')
OR (g.security_level = 2 AND s.sec_lev2_trans = 1 AND r.name_short='translate')

OR (g.security_level = 3 AND s.sec_lev3_read = 1 AND r.name_short='read')
OR (g.security_level = 3 AND s.sec_lev3_write = 1 AND r.name_short='edit')
OR (g.security_level = 3 AND s.sec_lev3_add = 1 AND (r.name_short='add_fields' OR r.name_short='fields' OR r.name_short='revision'))
OR (g.security_level = 3 AND s.sec_lev3_trans = 1 AND r.name_short='translate')

OR (s.sec_allow LIKE CONCAT('%',s.sec_allow,'%') AND r.name_short='read')
OR (s.sec_allow LIKE CONCAT('%',s.sec_allow,'%') AND r.name_short='edit')
OR (s.sec_allow LIKE CONCAT('%',s.sec_allow,'%') AND r.name_short='add_fields')
OR (s.sec_allow LIKE CONCAT('%',s.sec_allow,'%') AND r.name_short='translate')

OR (g.security_level = 3 AND r.name_short='system_meta')
OR (g.security_level > 1 AND r.name_short='dependencies')

OR r.name_short = 'metadata'
OR r.name_short = 'history')
AND NOT s.sec_deny LIKE CONCAT('%',g.name_short,'%'))
OR s.sec_allow LIKE CONCAT('%',g.name_short,'%'));



-- --------------------------------------------------------
-- create areas
-- --------------------------------------------------------
CREATE TABLE roscms_area (
  id bigint(20) NOT NULL auto_increment,
  `name` varchar(30) NOT NULL,
  name_short varchar(18) NOT NULL,
  description varchar(255) NOT NULL,
  PRIMARY KEY  (id),
  UNIQUE KEY `name` (`name`),
  UNIQUE KEY name_short (name_short)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;

INSERT INTO roscms_area VALUES
(1, 'System Tags', 'system_tags', 'Can the user modify/see system tags'),
(2, 'Content Management System', 'CMS', 'RosCMS Interface itself'),
(3, 'Entry Details', 'entry_details', 'Shows Entry Details such as Rev-ID'),
(4, 'New Entries', 'new_entry', 'Be able to create new entries'),
(5, 'Admin Branch', 'admin', 'Can Access Admin Branch'),
(6, 'Delete Files', 'delete_file', 'Able to delete files from generated content'),
(7, 'User Branch', 'user', 'Access user branch'),
(8, 'Add Translator', 'addtransl', 'add someone to translator group'),
(9, 'Add new membership', 'addmembership', 'add someone to new group'),
(10, 'Delete Membership', 'delmembership', 'removes someones membership to a group'),
(11, 'Disable Account', 'disableaccount', 'disable/enable user accounts'),
(12, 'User Details', 'user_details', 'Access to user details, such as user groups, user-id and contact data'),
(13, 'Foreign Drafts', 'other_drafts', 'beein able to view drafts of other people'),
(14, 'Maintain Branch', 'maintain', 'Access to Maintain branch'),
(15, 'Statistics Branch', 'stats', 'Access to Statistics branch'),
(16, 'Website Branch', 'website', 'Access to Website branch'),
(17, 'Pages', 'pages', 'View Pages'),
(18, 'Dynamic Pages', 'dynamic_pages', 'View Dynamic Pages'),
(19, 'Scripts', 'scripts', 'View Scripts'),
(20, 'Delete Tags', 'deltag', 'Delete System Tags from entries'),
(21, 'Update Tags', 'updatetag', 'Update Tag value'),
(22, 'More Languages', 'more_lang', 'Can change things in more languages than the user has set in his profile'),
(23, 'Logs', 'logs', 'Can view Logs'),
(24, 'Delete Entries', 'del_entry', 'Delete Entries'),
(25, 'Delete without archiv', 'del_wo_archiv', 'delete entries without moving them to archiv'),
(26, 'add level 0 group', 'addlvl0group', 'Add memberships with group security level 0'),
(27, 'add level 1 groups', 'addlvl1group', 'Add memberships with group security level 1'),
(28, 'add level 2 groups', 'addlvl2group', 'Add memberships with group security level 2'),
(29, 'add level 3 groups', 'addlvl3group', 'Add memberships with group security level 3'),
(30, 'Mix private & public entries', 'mix_priv_pub', 'show private and public type entries together'),
(31, 'Entry Details Security', 'entry_security', 'change security settings & name + type of entry'),
(32, 'show more filter', 'more_filter', 'show more than standard filter'),
(33, 'show admin filter', 'admin_filter', 'special admin filters'),
(34, 'Show all filter', 'dont_hide_filter', 'don''t hide filter from users'),
(35, 'Make Entries Stable', 'make_stable', 'Make Entries Stable'),
(36, 'show system entries', 'show_sys_entry', 'show entries of type ''system'''),
(37, 'Add manuel dependencies', 'add_dependencies', 'add new manuell dependencies to entries');



-- --------------------------------------------------------
-- create area protection list
-- --------------------------------------------------------
CREATE TABLE roscms_rel_groups_area (
  group_id bigint(20) NOT NULL,
  area_id bigint(20) NOT NULL,
  PRIMARY KEY  (group_id,area_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO roscms_rel_groups_area
SELECT DISTINCT g.id, a.id
FROM roscms_area a JOIN roscms_groups g
WHERE ((a.name_short = 'system_tags' OR a.name_short = 'entry_details' OR a.name_short = 'new_entry' OR a.name_short = 'deltag' OR a.name_short = 'del_entry' OR a.name_short = 'mix_priv_pub' OR a.name_short = 'show_sys_entry' OR a.name_short = 'addlvl1group' OR a.name_short = 'more_filter' OR a.name_short = 'make_stable' OR a.name_short = 'dont_hide_filter')
AND g.security_level > 1)

OR ((a.name_short = 'delete_file' OR a.name_short = 'delmembership' OR a.name_short = 'disableaccount' OR a.name_short = 'user_details' OR a.name_short = 'other_drafts' OR a.name_short = 'stats' OR a.name_short = 'dynamic_pages' OR a.name_short = 'updatetag' OR a.name_short = 'del_wo_archiv' OR a.name_short = 'addlvl2group' OR a.name_short = 'user' OR a.name_short = 'addmembership' OR a.name_short = 'maintain' OR a.name_short = 'admin_filter' OR a.name_short = 'add_dependencies')
AND g.security_level = 3)

OR ((a.name_short = 'admin' OR a.name_short = 'logs' OR a.name_short = 'addlvl3group')
AND g.name_short = 'ros_sadmin')

OR ((a.name_short='pages' OR a.name_short = 'scripts')
AND g.security_level > 1 AND g.name_short != 'transmaint')

OR ((a.name_short = 'CMS' OR a.name_short = 'website' OR a.name_short = 'addlvl0group')
AND g.security_level > 0)

OR ((a.name_short = 'maintain' OR a.name_short = 'user' OR a.name_short = 'addmembership' OR a.name_short = 'addtransl' OR a.name_short = 'addlvl0group')
AND g.name_short = 'transmaint')

OR ((a.name_short = 'more_lang')
AND g.name_short != 'translator' AND g.name_short != 'transmaint' AND g.security_level > 0)

OR ((a.name_short = 'entry_security')
AND (g.name_short = 'ros_sadmin' OR g.name_short = 'ros_admin'));


-- --------------------------------------------------------
-- create and convert entries
-- --------------------------------------------------------
CREATE TABLE roscms_entries (
  id bigint(20) unsigned NOT NULL auto_increment,
  type enum('page','content','dynamic','script','system') collate utf8_unicode_ci NOT NULL,
  name varchar(64) collate utf8_unicode_ci NOT NULL,
  access_id bigint(20) unsigned COMMENT '->access(id)',
  old_id int(11) NOT NULL,
  old_archive tinyint(1) NOT NULL,
  PRIMARY KEY  (id),
  KEY access_id (access_id),
  KEY type (type),
  KEY name (name)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- double entries are removed after converting revisions
INSERT INTO roscms_entries
SELECT
  NULL,
  d.data_type,
  d.data_name,
  s.id,
  d.data_id,
  TRUE
FROM data_a d JOIN roscms_entries_access s ON d.data_acl=s.name_short WHERE data_type != 'template'
UNION
SELECT
  NULL,
  'content',
  d.data_name,
  s.id,
  d.data_id,
  TRUE
FROM data_a d JOIN roscms_entries_access s ON d.data_acl=s.name_short WHERE data_type = 'template'
UNION
SELECT
  NULL,
  d.data_type,
  d.data_name,
  s.id,
  d.data_id,
  FALSE
FROM data_ d JOIN roscms_entries_access s ON d.data_acl=s.name_short WHERE data_type != 'template'
UNION
SELECT
  NULL,
  'template',
  d.data_name,
  s.id,
  d.data_id,
  FALSE
FROM data_ d JOIN roscms_entries_access s ON d.data_acl=s.name_short WHERE data_type = 'template';



-- --------------------------------------------------------
-- create and convert entry revisions
-- --------------------------------------------------------
CREATE TABLE roscms_entries_revisions (
  id bigint(20) unsigned NOT NULL auto_increment,
  data_id bigint(20) unsigned NOT NULL COMMENT '->entries(id)',
  lang_id bigint(20) unsigned NOT NULL COMMENT '->languages(id)',
  user_id bigint(20) unsigned NOT NULL COMMENT '->accounts(id)',
  version int(10) unsigned NOT NULL,
  `datetime` datetime NOT NULL,
  status varchar(10) NOT NULL default 'unknown',
  archive tinyint(1) NOT NULL default '0',
  old_id int(11) NOT NULL,
  PRIMARY KEY  (id),
  KEY data_id (data_id),
  KEY `language` (lang_id),
  KEY user_id (user_id),
  KEY version (version)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- old_id will be dropped after tag convert
INSERT INTO roscms_entries_revisions
SELECT
  NULL,
  d.id,
  l.id,
  r.rev_usrid,
  r.rev_version,
  r.rev_datetime,
  'unknown',
  TRUE,
  r.rev_id
FROM data_revision_a r JOIN roscms_languages l ON r.rev_language=l.name_short JOIN roscms_entries d ON (d.old_id=r.data_id AND d.old_archive IS TRUE)
UNION
SELECT
  NULL,
  d.id,
  l.id,
  r.rev_usrid,
  r.rev_version,
  r.rev_datetime,
  'unknown',
  FALSE,
  r.rev_id
FROM data_revision r JOIN roscms_languages l ON r.rev_language=l.name_short JOIN roscms_entries d ON (d.old_id=r.data_id AND d.old_archive IS FALSE);



-- --------------------------------------------------------
-- remove double entries from database
-- --------------------------------------------------------
CREATE TABLE roscms_converter_temp (
  old_data_id bigint(20) unsigned NOT NULL,
  new_data_id bigint(20) unsigned NOT NULL,
  KEY old_data_id (old_data_id),
  KEY new_data_id (new_data_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_converter_temp
SELECT DISTINCT
  b.id,
  a.id
FROM roscms_entries a JOIN roscms_entries b ON (a.name=b.name AND a.type=b.type AND b.old_archive IS TRUE AND a.old_archive IS FALSE);

UPDATE roscms_entries_revisions r JOIN roscms_converter_temp c ON r.data_id=c.old_data_id SET r.data_id=c.new_data_id;
DELETE FROM roscms_entries WHERE id IN (SELECT old_data_id FROM roscms_converter_temp);



-- --------------------------------------------------------
-- create and convert stexts
-- --------------------------------------------------------
CREATE TABLE roscms_entries_stext (
  id bigint(20) unsigned NOT NULL auto_increment,
  rev_id bigint(20) unsigned NOT NULL COMMENT '->entries_revisions(id)',
  name varchar(32) collate utf8_unicode_ci NOT NULL,
  content varchar(255) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id),
  UNIQUE KEY rev_id_2 (rev_id, name),
  KEY rev_id (rev_id),
  KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_entries_stext
SELECT
  NULL,
  r.id,
  t.stext_name,
  t.stext_content
FROM data_stext_a t JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS TRUE)
UNION
SELECT
  NULL,
  r.id,
  t.stext_name,
  t.stext_content
FROM data_stext t JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS FALSE);



-- --------------------------------------------------------
-- create and convert texts
-- --------------------------------------------------------
CREATE TABLE roscms_entries_text (
  id bigint(20) unsigned NOT NULL auto_increment,
  rev_id bigint(20) unsigned NOT NULL COMMENT '->entries_revisions(id)',
  name varchar(32) collate utf8_unicode_ci NOT NULL,
  content text collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id),
  KEY rev_id (rev_id),
  KEY `name` (`name`),
  FULLTEXT KEY content (content)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_entries_text
SELECT
  NULL,
  r.id,
  t.text_name,
  t.text_content
FROM data_text_a t JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS TRUE)
UNION
SELECT
  NULL,
  r.id,
  t.text_name,
  t.text_content
FROM data_text t JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS FALSE);



-- --------------------------------------------------------
-- create and convert tags
-- --------------------------------------------------------
CREATE TABLE roscms_entries_tags (
  id bigint(20) unsigned NOT NULL auto_increment,
  rev_id bigint(20) unsigned NOT NULL COMMENT '->entries_revisions(id)',
  user_id bigint(21) NOT NULL COMMENT '->accounts(id); -1=>system',
  name varchar(32) collate utf8_unicode_ci NOT NULL,
  value varchar(128) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id),
  KEY rev_id (rev_id),
  KEY user_id (user_id),
  KEY name (name)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_entries_tags
SELECT
  NULL,
  r.id,
  t.tag_usrid,
  n.tn_name,
  v.tv_value
FROM data_tag_a t JOIN data_tag_name_a n ON t.tag_name_id=n.tn_id JOIN data_tag_value_a v ON t.tag_value_id=v.tv_id JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS TRUE)
UNION
SELECT
  NULL,
  r.id,
  t.tag_usrid,
  n.tn_name,
  v.tv_value
FROM data_tag t JOIN data_tag_name n ON t.tag_name_id=n.tn_id JOIN data_tag_value v ON t.tag_value_id=v.tv_id JOIN roscms_entries_revisions r ON (t.data_rev_id=r.old_id AND r.archive IS FALSE);



-- --------------------------------------------------------
-- port status tags to revisions
-- --------------------------------------------------------

UPDATE roscms_entries_revisions r
SET status = (SELECT value FROM roscms_entries_tags WHERE rev_id=r.id AND name='status' ORDER BY value ASC LIMIT 1);

DELETE FROM roscms_entries_tags WHERE name='status';




-- --------------------------------------------------------
-- clean up unneeded tags
-- --------------------------------------------------------
DELETE FROM roscms_entries_tags WHERE name='visible';
DELETE FROM roscms_entries_tags WHERE name='kind' AND value='default';
DELETE FROM roscms_entries_tags WHERE name='number_sort';


-- --------------------------------------------------------
-- create and convert timezones
-- --------------------------------------------------------
CREATE TABLE roscms_timezones (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(50) collate utf8_unicode_ci NOT NULL,
  name_short varchar(5) collate utf8_unicode_ci NOT NULL,
  difference char(5) collate utf8_unicode_ci NOT NULL default '+0000',
  decimal_difference decimal(10,2) NOT NULL default '0',
  PRIMARY KEY  (id),
  KEY difference (decimal_difference)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_timezones
SELECT
  NULL,
  tz_name,
  tz_code,
  tz_value2,
  tz_value
FROM user_timezone
ORDER BY tz_value;



-- --------------------------------------------------------
-- create and convert user accounts
-- --------------------------------------------------------
CREATE TABLE roscms_accounts (
  id bigint(20) unsigned NOT NULL auto_increment,
  name varchar(20) collate utf8_unicode_ci NOT NULL,
  password varchar(32) collate utf8_unicode_ci NOT NULL COMMENT 'md5 encoded',
  email varchar(150) collate utf8_unicode_ci NOT NULL,
  lang_id bigint(20) unsigned COMMENT '->languages(id)',
  country_id bigint(20) unsigned COMMENT '->countries(id)',
  timezone_id bigint(20) unsigned COMMENT '->timezones(id)',
  logins int(10) unsigned NOT NULL default '0',
  fullname varchar(100) collate utf8_unicode_ci NOT NULL,
  homepage varchar(150) collate utf8_unicode_ci NOT NULL,
  occupation varchar(50) collate utf8_unicode_ci NOT NULL,
  description varchar(255) collate utf8_unicode_ci NOT NULL,
  match_session tinyint(1) NOT NULL default '1',
  match_browseragent tinyint(1) NOT NULL default '0',
  match_ip tinyint(1) NOT NULL default '0',
  match_session_expire tinyint(1) NOT NULL default '1',
  activation varchar(200) collate utf8_unicode_ci NOT NULL COMMENT 'account / email',
  activation_password varchar(50) collate utf8_unicode_ci NOT NULL COMMENT 'code to activate the new password',
  created datetime NOT NULL,
  modified datetime NOT NULL,
  visible tinyint(1) NOT NULL default '0',
  disabled tinyint(1) NOT NULL default '1',
  PRIMARY KEY  (id),
  UNIQUE KEY `name` (`name`),
  UNIQUE KEY email (email),
  KEY `language` (lang_id),
  KEY country (country_id),
  KEY timezone (timezone_id),
  KEY activation_password (activation_password)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_accounts
SELECT
  u.user_id,
  u.user_name,
  u.user_roscms_password,
  u.user_email,
  l.id,
  c.id,
  t.id,
  u.user_login_counter,
  u.user_fullname,
  u.user_website,
  u.user_occupation,
  u.user_description,
  u.user_setting_multisession,
  u.user_setting_browseragent,
  u.user_setting_ipaddress,
  u.user_setting_timeout,
  CONCAT(u.user_register_activation,u.user_email_activation) AS activation,
  u.user_roscms_getpwd_id,
  u.user_register,
  u.user_timestamp_touch2,
  (u.user_account_hidden = 'no') AS visible,
  (u.user_account_enabled = 'no') AS disabled
FROM users u
LEFT JOIN roscms_languages l ON u.user_language=l.name_short
LEFT JOIN roscms_timezones t ON u.user_timezone=t.name_short
LEFT JOIN roscms_countries c ON u.user_country=c.name_short;



-- --------------------------------------------------------
-- create and convert group memberships
-- --------------------------------------------------------
CREATE TABLE roscms_rel_groups_accounts (
  group_id bigint(20) unsigned NOT NULL COMMENT '->groups(id)',
  user_id bigint(20) unsigned NOT NULL COMMENT '->accounts(id)',
  PRIMARY KEY  (group_id,user_id),
  KEY group_id (group_id),
  KEY user_id (user_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_rel_groups_accounts
SELECT DISTINCT
  g.id,
  m.usergroupmember_userid
FROM usergroup_members m JOIN roscms_groups g ON m.usergroupmember_usergroupid=g.name_short JOIN roscms_accounts u ON u.id=m.usergroupmember_userid;



-- --------------------------------------------------------
-- create and convert subsys mappings
-- --------------------------------------------------------
CREATE TABLE roscms_rel_accounts_subsys (
  user_id bigint(20) unsigned NOT NULL COMMENT '->accounts(id)',
  subsys varchar(10) collate utf8_unicode_ci NOT NULL,
  subsys_user_id bigint(20) unsigned NOT NULL COMMENT '->$subsys.$user(id)',
  PRIMARY KEY  (user_id,subsys),
  UNIQUE KEY subsys (subsys,subsys_user_id),
  KEY subsys_user_id (subsys_user_id),
  KEY user_id (user_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_rel_accounts_subsys
SELECT DISTINCT
  map_roscms_userid,
  map_subsys_name,
  map_subsys_userid
FROM subsys_mappings s JOIN roscms_accounts u ON s.map_roscms_userid=u.id;



-- --------------------------------------------------------
-- create and convert forbidden account names (or parts)
-- --------------------------------------------------------
CREATE TABLE roscms_accounts_forbidden (
  `name` varchar(20) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`name`),
  FULLTEXT KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='consists of a collection of forbidden names or parts';

INSERT INTO roscms_accounts_forbidden
SELECT
  unsafe_name
FROM user_unsafenames;



-- --------------------------------------------------------
-- create and convert account sessions
-- --------------------------------------------------------
CREATE TABLE roscms_accounts_sessions (
  id varchar(32) collate utf8_unicode_ci NOT NULL,
  user_id bigint(20) unsigned NOT NULL COMMENT '->accounts(id)',
  expires datetime default NULL,
  browseragent varchar(255) collate utf8_unicode_ci NOT NULL,
  ip varchar(15) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (id),
  KEY user_id (user_id)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO roscms_accounts_sessions SELECT * FROM user_sessions;



-- --------------------------------------------------------
-- remove converter specific fields
-- --------------------------------------------------------
ALTER TABLE roscms_entries
  DROP old_id,
  DROP old_archive,
  ADD UNIQUE KEY type_name ( type , name );
ALTER TABLE roscms_entries_revisions DROP old_id;
ALTER TABLE roscms_entries_access DROP name_short;



-- --------------------------------------------------------
-- convert to dynamic entries
-- --------------------------------------------------------
UPDATE roscms_entries SET type = 'dynamic' WHERE type='page' AND (name='news_page' OR name='newsletter' OR name='interview');

INSERT INTO roscms_entries (type, name, access_id)
SELECT DISTINCT
  'content',
  CONCAT(d.name,'_',t.value),
  d.access_id
FROM roscms_entries d JOIN roscms_entries e ON e.name=d.name JOIN roscms_entries_revisions r ON r.data_id=d.id JOIN roscms_entries_tags t ON t.rev_id=r.id
WHERE t.name='number' AND d.type='content' AND e.type = 'dynamic';

UPDATE roscms_entries_revisions r JOIN roscms_entries o ON r.data_id=o.id JOIN roscms_entries_tags t ON t.rev_id=r.id JOIN roscms_entries d ON d.name=CONCAT(o.name,'_',t.value) SET r.data_id=d.id WHERE t.name='number' AND o.type='content';

INSERT INTO roscms_entries_tags (rev_id, name, value, user_id)
SELECT
  r2.id,
  'next_index',
  MAX(t.value*1)+1 AS val,
  -1
FROM roscms_entries d
JOIN roscms_entries_revisions r2 ON d.id=r2.data_id
JOIN roscms_entries e ON e.name LIKE CONCAT(d.name,'_%')
JOIN roscms_entries_revisions r ON r.data_id=e.id
JOIN roscms_entries_tags t ON t.rev_id=r.id
WHERE d.type = 'dynamic'
AND e.type = 'content'
AND t.name='number'
GROUP BY d.name;

DELETE FROM roscms_entries WHERE type='content' AND (name='news_page' OR name='newsletter' OR name='interview');



-- --------------------------------------------------------
-- drop old tables
-- --------------------------------------------------------
DROP TABLE data_text;
DROP TABLE data_text_a;
DROP TABLE data_stext;
DROP TABLE data_stext_a;
DROP TABLE roscms_converter_temp;
DROP TABLE data_revision;
DROP TABLE data_revision_a;
DROP TABLE data_;
DROP TABLE data_a;
DROP TABLE usergroup_members;
DROP TABLE data_security;
DROP TABLE usergroups;
DROP TABLE data_user_filter;
DROP TABLE user_countries;
DROP TABLE languages;
DROP TABLE user_language;
DROP TABLE data_tag;
DROP TABLE data_tag_a;
DROP TABLE data_tag_name;
DROP TABLE data_tag_name_a;
DROP TABLE data_tag_value;
DROP TABLE data_tag_value_a;
DROP TABLE subsys_mappings;
DROP TABLE user_timezone;
DROP TABLE users;
DROP TABLE user_unsafenames;
DROP TABLE user_unsafepwds;
DROP TABLE user_sessions;
