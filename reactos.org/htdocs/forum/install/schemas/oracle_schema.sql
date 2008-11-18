/*

 $Id: oracle_schema.sql 8456 2008-03-22 12:31:17Z Kellanved $

*/

/*
  This first section is optional, however its probably the best method
  of running phpBB on Oracle. If you already have a tablespace and user created
  for phpBB you can leave this section commented out!

  The first set of statements create a phpBB tablespace and a phpBB user,
  make sure you change the password of the phpBB user before you run this script!!
*/

/*
CREATE TABLESPACE "PHPBB"
	LOGGING
	DATAFILE 'E:\ORACLE\ORADATA\LOCAL\PHPBB.ora'
	SIZE 10M
	AUTOEXTEND ON NEXT 10M
	MAXSIZE 100M;

CREATE USER "PHPBB"
	PROFILE "DEFAULT"
	IDENTIFIED BY "phpbb_password"
	DEFAULT TABLESPACE "PHPBB"
	QUOTA UNLIMITED ON "PHPBB"
	ACCOUNT UNLOCK;

GRANT ANALYZE ANY TO "PHPBB";
GRANT CREATE SEQUENCE TO "PHPBB";
GRANT CREATE SESSION TO "PHPBB";
GRANT CREATE TABLE TO "PHPBB";
GRANT CREATE TRIGGER TO "PHPBB";
GRANT CREATE VIEW TO "PHPBB";
GRANT "CONNECT" TO "PHPBB";

COMMIT;
DISCONNECT;

CONNECT phpbb/phpbb_password;
*/
/*
	Table: 'phpbb_attachments'
*/
CREATE TABLE phpbb_attachments (
	attach_id number(8) NOT NULL,
	post_msg_id number(8) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	in_message number(1) DEFAULT '0' NOT NULL,
	poster_id number(8) DEFAULT '0' NOT NULL,
	is_orphan number(1) DEFAULT '1' NOT NULL,
	physical_filename varchar2(255) DEFAULT '' ,
	real_filename varchar2(255) DEFAULT '' ,
	download_count number(8) DEFAULT '0' NOT NULL,
	attach_comment clob DEFAULT '' ,
	extension varchar2(100) DEFAULT '' ,
	mimetype varchar2(100) DEFAULT '' ,
	filesize number(20) DEFAULT '0' NOT NULL,
	filetime number(11) DEFAULT '0' NOT NULL,
	thumbnail number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_attachments PRIMARY KEY (attach_id)
)
/

CREATE INDEX phpbb_attachments_filetime ON phpbb_attachments (filetime)
/
CREATE INDEX phpbb_attachments_post_msg_id ON phpbb_attachments (post_msg_id)
/
CREATE INDEX phpbb_attachments_topic_id ON phpbb_attachments (topic_id)
/
CREATE INDEX phpbb_attachments_poster_id ON phpbb_attachments (poster_id)
/
CREATE INDEX phpbb_attachments_is_orphan ON phpbb_attachments (is_orphan)
/

CREATE SEQUENCE phpbb_attachments_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_attachments
BEFORE INSERT ON phpbb_attachments
FOR EACH ROW WHEN (
	new.attach_id IS NULL OR new.attach_id = 0
)
BEGIN
	SELECT phpbb_attachments_seq.nextval
	INTO :new.attach_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_acl_groups'
*/
CREATE TABLE phpbb_acl_groups (
	group_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	auth_option_id number(8) DEFAULT '0' NOT NULL,
	auth_role_id number(8) DEFAULT '0' NOT NULL,
	auth_setting number(2) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_acl_groups_group_id ON phpbb_acl_groups (group_id)
/
CREATE INDEX phpbb_acl_groups_auth_opt_id ON phpbb_acl_groups (auth_option_id)
/
CREATE INDEX phpbb_acl_groups_auth_role_id ON phpbb_acl_groups (auth_role_id)
/

/*
	Table: 'phpbb_acl_options'
*/
CREATE TABLE phpbb_acl_options (
	auth_option_id number(8) NOT NULL,
	auth_option varchar2(50) DEFAULT '' ,
	is_global number(1) DEFAULT '0' NOT NULL,
	is_local number(1) DEFAULT '0' NOT NULL,
	founder_only number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_acl_options PRIMARY KEY (auth_option_id)
)
/

CREATE INDEX phpbb_acl_options_auth_option ON phpbb_acl_options (auth_option)
/

CREATE SEQUENCE phpbb_acl_options_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_acl_options
BEFORE INSERT ON phpbb_acl_options
FOR EACH ROW WHEN (
	new.auth_option_id IS NULL OR new.auth_option_id = 0
)
BEGIN
	SELECT phpbb_acl_options_seq.nextval
	INTO :new.auth_option_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_acl_roles'
*/
CREATE TABLE phpbb_acl_roles (
	role_id number(8) NOT NULL,
	role_name varchar2(765) DEFAULT '' ,
	role_description clob DEFAULT '' ,
	role_type varchar2(10) DEFAULT '' ,
	role_order number(4) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_acl_roles PRIMARY KEY (role_id)
)
/

CREATE INDEX phpbb_acl_roles_role_type ON phpbb_acl_roles (role_type)
/
CREATE INDEX phpbb_acl_roles_role_order ON phpbb_acl_roles (role_order)
/

CREATE SEQUENCE phpbb_acl_roles_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_acl_roles
BEFORE INSERT ON phpbb_acl_roles
FOR EACH ROW WHEN (
	new.role_id IS NULL OR new.role_id = 0
)
BEGIN
	SELECT phpbb_acl_roles_seq.nextval
	INTO :new.role_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_acl_roles_data'
*/
CREATE TABLE phpbb_acl_roles_data (
	role_id number(8) DEFAULT '0' NOT NULL,
	auth_option_id number(8) DEFAULT '0' NOT NULL,
	auth_setting number(2) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_acl_roles_data PRIMARY KEY (role_id, auth_option_id)
)
/

CREATE INDEX phpbb_acl_roles_data_ath_op_id ON phpbb_acl_roles_data (auth_option_id)
/

/*
	Table: 'phpbb_acl_users'
*/
CREATE TABLE phpbb_acl_users (
	user_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	auth_option_id number(8) DEFAULT '0' NOT NULL,
	auth_role_id number(8) DEFAULT '0' NOT NULL,
	auth_setting number(2) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_acl_users_user_id ON phpbb_acl_users (user_id)
/
CREATE INDEX phpbb_acl_users_auth_option_id ON phpbb_acl_users (auth_option_id)
/
CREATE INDEX phpbb_acl_users_auth_role_id ON phpbb_acl_users (auth_role_id)
/

/*
	Table: 'phpbb_banlist'
*/
CREATE TABLE phpbb_banlist (
	ban_id number(8) NOT NULL,
	ban_userid number(8) DEFAULT '0' NOT NULL,
	ban_ip varchar2(40) DEFAULT '' ,
	ban_email varchar2(300) DEFAULT '' ,
	ban_start number(11) DEFAULT '0' NOT NULL,
	ban_end number(11) DEFAULT '0' NOT NULL,
	ban_exclude number(1) DEFAULT '0' NOT NULL,
	ban_reason varchar2(765) DEFAULT '' ,
	ban_give_reason varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_banlist PRIMARY KEY (ban_id)
)
/

CREATE INDEX phpbb_banlist_ban_end ON phpbb_banlist (ban_end)
/
CREATE INDEX phpbb_banlist_ban_user ON phpbb_banlist (ban_userid, ban_exclude)
/
CREATE INDEX phpbb_banlist_ban_email ON phpbb_banlist (ban_email, ban_exclude)
/
CREATE INDEX phpbb_banlist_ban_ip ON phpbb_banlist (ban_ip, ban_exclude)
/

CREATE SEQUENCE phpbb_banlist_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_banlist
BEFORE INSERT ON phpbb_banlist
FOR EACH ROW WHEN (
	new.ban_id IS NULL OR new.ban_id = 0
)
BEGIN
	SELECT phpbb_banlist_seq.nextval
	INTO :new.ban_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_bbcodes'
*/
CREATE TABLE phpbb_bbcodes (
	bbcode_id number(3) DEFAULT '0' NOT NULL,
	bbcode_tag varchar2(16) DEFAULT '' ,
	bbcode_helpline varchar2(765) DEFAULT '' ,
	display_on_posting number(1) DEFAULT '0' NOT NULL,
	bbcode_match clob DEFAULT '' ,
	bbcode_tpl clob DEFAULT '' ,
	first_pass_match clob DEFAULT '' ,
	first_pass_replace clob DEFAULT '' ,
	second_pass_match clob DEFAULT '' ,
	second_pass_replace clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_bbcodes PRIMARY KEY (bbcode_id)
)
/

CREATE INDEX phpbb_bbcodes_display_on_post ON phpbb_bbcodes (display_on_posting)
/

/*
	Table: 'phpbb_bookmarks'
*/
CREATE TABLE phpbb_bookmarks (
	topic_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_bookmarks PRIMARY KEY (topic_id, user_id)
)
/


/*
	Table: 'phpbb_bots'
*/
CREATE TABLE phpbb_bots (
	bot_id number(8) NOT NULL,
	bot_active number(1) DEFAULT '1' NOT NULL,
	bot_name varchar2(765) DEFAULT '' ,
	user_id number(8) DEFAULT '0' NOT NULL,
	bot_agent varchar2(255) DEFAULT '' ,
	bot_ip varchar2(255) DEFAULT '' ,
	CONSTRAINT pk_phpbb_bots PRIMARY KEY (bot_id)
)
/

CREATE INDEX phpbb_bots_bot_active ON phpbb_bots (bot_active)
/

CREATE SEQUENCE phpbb_bots_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_bots
BEFORE INSERT ON phpbb_bots
FOR EACH ROW WHEN (
	new.bot_id IS NULL OR new.bot_id = 0
)
BEGIN
	SELECT phpbb_bots_seq.nextval
	INTO :new.bot_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_config'
*/
CREATE TABLE phpbb_config (
	config_name varchar2(255) DEFAULT '' ,
	config_value varchar2(765) DEFAULT '' ,
	is_dynamic number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_config PRIMARY KEY (config_name)
)
/

CREATE INDEX phpbb_config_is_dynamic ON phpbb_config (is_dynamic)
/

/*
	Table: 'phpbb_confirm'
*/
CREATE TABLE phpbb_confirm (
	confirm_id char(32) DEFAULT '' ,
	session_id char(32) DEFAULT '' ,
	confirm_type number(3) DEFAULT '0' NOT NULL,
	code varchar2(8) DEFAULT '' ,
	seed number(10) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_confirm PRIMARY KEY (session_id, confirm_id)
)
/

CREATE INDEX phpbb_confirm_confirm_type ON phpbb_confirm (confirm_type)
/

/*
	Table: 'phpbb_disallow'
*/
CREATE TABLE phpbb_disallow (
	disallow_id number(8) NOT NULL,
	disallow_username varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_disallow PRIMARY KEY (disallow_id)
)
/


CREATE SEQUENCE phpbb_disallow_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_disallow
BEFORE INSERT ON phpbb_disallow
FOR EACH ROW WHEN (
	new.disallow_id IS NULL OR new.disallow_id = 0
)
BEGIN
	SELECT phpbb_disallow_seq.nextval
	INTO :new.disallow_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_drafts'
*/
CREATE TABLE phpbb_drafts (
	draft_id number(8) NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	save_time number(11) DEFAULT '0' NOT NULL,
	draft_subject varchar2(300) DEFAULT '' ,
	draft_message clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_drafts PRIMARY KEY (draft_id)
)
/

CREATE INDEX phpbb_drafts_save_time ON phpbb_drafts (save_time)
/

CREATE SEQUENCE phpbb_drafts_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_drafts
BEFORE INSERT ON phpbb_drafts
FOR EACH ROW WHEN (
	new.draft_id IS NULL OR new.draft_id = 0
)
BEGIN
	SELECT phpbb_drafts_seq.nextval
	INTO :new.draft_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_extensions'
*/
CREATE TABLE phpbb_extensions (
	extension_id number(8) NOT NULL,
	group_id number(8) DEFAULT '0' NOT NULL,
	extension varchar2(100) DEFAULT '' ,
	CONSTRAINT pk_phpbb_extensions PRIMARY KEY (extension_id)
)
/


CREATE SEQUENCE phpbb_extensions_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_extensions
BEFORE INSERT ON phpbb_extensions
FOR EACH ROW WHEN (
	new.extension_id IS NULL OR new.extension_id = 0
)
BEGIN
	SELECT phpbb_extensions_seq.nextval
	INTO :new.extension_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_extension_groups'
*/
CREATE TABLE phpbb_extension_groups (
	group_id number(8) NOT NULL,
	group_name varchar2(765) DEFAULT '' ,
	cat_id number(2) DEFAULT '0' NOT NULL,
	allow_group number(1) DEFAULT '0' NOT NULL,
	download_mode number(1) DEFAULT '1' NOT NULL,
	upload_icon varchar2(255) DEFAULT '' ,
	max_filesize number(20) DEFAULT '0' NOT NULL,
	allowed_forums clob DEFAULT '' ,
	allow_in_pm number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_extension_groups PRIMARY KEY (group_id)
)
/


CREATE SEQUENCE phpbb_extension_groups_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_extension_groups
BEFORE INSERT ON phpbb_extension_groups
FOR EACH ROW WHEN (
	new.group_id IS NULL OR new.group_id = 0
)
BEGIN
	SELECT phpbb_extension_groups_seq.nextval
	INTO :new.group_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_forums'
*/
CREATE TABLE phpbb_forums (
	forum_id number(8) NOT NULL,
	parent_id number(8) DEFAULT '0' NOT NULL,
	left_id number(8) DEFAULT '0' NOT NULL,
	right_id number(8) DEFAULT '0' NOT NULL,
	forum_parents clob DEFAULT '' ,
	forum_name varchar2(765) DEFAULT '' ,
	forum_desc clob DEFAULT '' ,
	forum_desc_bitfield varchar2(255) DEFAULT '' ,
	forum_desc_options number(11) DEFAULT '7' NOT NULL,
	forum_desc_uid varchar2(8) DEFAULT '' ,
	forum_link varchar2(765) DEFAULT '' ,
	forum_password varchar2(120) DEFAULT '' ,
	forum_style number(4) DEFAULT '0' NOT NULL,
	forum_image varchar2(255) DEFAULT '' ,
	forum_rules clob DEFAULT '' ,
	forum_rules_link varchar2(765) DEFAULT '' ,
	forum_rules_bitfield varchar2(255) DEFAULT '' ,
	forum_rules_options number(11) DEFAULT '7' NOT NULL,
	forum_rules_uid varchar2(8) DEFAULT '' ,
	forum_topics_per_page number(4) DEFAULT '0' NOT NULL,
	forum_type number(4) DEFAULT '0' NOT NULL,
	forum_status number(4) DEFAULT '0' NOT NULL,
	forum_posts number(8) DEFAULT '0' NOT NULL,
	forum_topics number(8) DEFAULT '0' NOT NULL,
	forum_topics_real number(8) DEFAULT '0' NOT NULL,
	forum_last_post_id number(8) DEFAULT '0' NOT NULL,
	forum_last_poster_id number(8) DEFAULT '0' NOT NULL,
	forum_last_post_subject varchar2(300) DEFAULT '' ,
	forum_last_post_time number(11) DEFAULT '0' NOT NULL,
	forum_last_poster_name varchar2(765) DEFAULT '' ,
	forum_last_poster_colour varchar2(6) DEFAULT '' ,
	forum_flags number(4) DEFAULT '32' NOT NULL,
	display_subforum_list number(1) DEFAULT '1' NOT NULL,
	display_on_index number(1) DEFAULT '1' NOT NULL,
	enable_indexing number(1) DEFAULT '1' NOT NULL,
	enable_icons number(1) DEFAULT '1' NOT NULL,
	enable_prune number(1) DEFAULT '0' NOT NULL,
	prune_next number(11) DEFAULT '0' NOT NULL,
	prune_days number(8) DEFAULT '0' NOT NULL,
	prune_viewed number(8) DEFAULT '0' NOT NULL,
	prune_freq number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_forums PRIMARY KEY (forum_id)
)
/

CREATE INDEX phpbb_forums_left_right_id ON phpbb_forums (left_id, right_id)
/
CREATE INDEX phpbb_forums_forum_lastpost_id ON phpbb_forums (forum_last_post_id)
/

CREATE SEQUENCE phpbb_forums_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_forums
BEFORE INSERT ON phpbb_forums
FOR EACH ROW WHEN (
	new.forum_id IS NULL OR new.forum_id = 0
)
BEGIN
	SELECT phpbb_forums_seq.nextval
	INTO :new.forum_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_forums_access'
*/
CREATE TABLE phpbb_forums_access (
	forum_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	session_id char(32) DEFAULT '' ,
	CONSTRAINT pk_phpbb_forums_access PRIMARY KEY (forum_id, user_id, session_id)
)
/


/*
	Table: 'phpbb_forums_track'
*/
CREATE TABLE phpbb_forums_track (
	user_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	mark_time number(11) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_forums_track PRIMARY KEY (user_id, forum_id)
)
/


/*
	Table: 'phpbb_forums_watch'
*/
CREATE TABLE phpbb_forums_watch (
	forum_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	notify_status number(1) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_forums_watch_forum_id ON phpbb_forums_watch (forum_id)
/
CREATE INDEX phpbb_forums_watch_user_id ON phpbb_forums_watch (user_id)
/
CREATE INDEX phpbb_forums_watch_notify_stat ON phpbb_forums_watch (notify_status)
/

/*
	Table: 'phpbb_groups'
*/
CREATE TABLE phpbb_groups (
	group_id number(8) NOT NULL,
	group_type number(4) DEFAULT '1' NOT NULL,
	group_founder_manage number(1) DEFAULT '0' NOT NULL,
	group_name varchar2(255) DEFAULT '' ,
	group_desc clob DEFAULT '' ,
	group_desc_bitfield varchar2(255) DEFAULT '' ,
	group_desc_options number(11) DEFAULT '7' NOT NULL,
	group_desc_uid varchar2(8) DEFAULT '' ,
	group_display number(1) DEFAULT '0' NOT NULL,
	group_avatar varchar2(255) DEFAULT '' ,
	group_avatar_type number(2) DEFAULT '0' NOT NULL,
	group_avatar_width number(4) DEFAULT '0' NOT NULL,
	group_avatar_height number(4) DEFAULT '0' NOT NULL,
	group_rank number(8) DEFAULT '0' NOT NULL,
	group_colour varchar2(6) DEFAULT '' ,
	group_sig_chars number(8) DEFAULT '0' NOT NULL,
	group_receive_pm number(1) DEFAULT '0' NOT NULL,
	group_message_limit number(8) DEFAULT '0' NOT NULL,
	group_legend number(1) DEFAULT '1' NOT NULL,
	CONSTRAINT pk_phpbb_groups PRIMARY KEY (group_id)
)
/

CREATE INDEX phpbb_groups_group_legend_name ON phpbb_groups (group_legend, group_name)
/

CREATE SEQUENCE phpbb_groups_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_groups
BEFORE INSERT ON phpbb_groups
FOR EACH ROW WHEN (
	new.group_id IS NULL OR new.group_id = 0
)
BEGIN
	SELECT phpbb_groups_seq.nextval
	INTO :new.group_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_icons'
*/
CREATE TABLE phpbb_icons (
	icons_id number(8) NOT NULL,
	icons_url varchar2(255) DEFAULT '' ,
	icons_width number(4) DEFAULT '0' NOT NULL,
	icons_height number(4) DEFAULT '0' NOT NULL,
	icons_order number(8) DEFAULT '0' NOT NULL,
	display_on_posting number(1) DEFAULT '1' NOT NULL,
	CONSTRAINT pk_phpbb_icons PRIMARY KEY (icons_id)
)
/

CREATE INDEX phpbb_icons_display_on_posting ON phpbb_icons (display_on_posting)
/

CREATE SEQUENCE phpbb_icons_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_icons
BEFORE INSERT ON phpbb_icons
FOR EACH ROW WHEN (
	new.icons_id IS NULL OR new.icons_id = 0
)
BEGIN
	SELECT phpbb_icons_seq.nextval
	INTO :new.icons_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_lang'
*/
CREATE TABLE phpbb_lang (
	lang_id number(4) NOT NULL,
	lang_iso varchar2(30) DEFAULT '' ,
	lang_dir varchar2(30) DEFAULT '' ,
	lang_english_name varchar2(300) DEFAULT '' ,
	lang_local_name varchar2(765) DEFAULT '' ,
	lang_author varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_lang PRIMARY KEY (lang_id)
)
/

CREATE INDEX phpbb_lang_lang_iso ON phpbb_lang (lang_iso)
/

CREATE SEQUENCE phpbb_lang_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_lang
BEFORE INSERT ON phpbb_lang
FOR EACH ROW WHEN (
	new.lang_id IS NULL OR new.lang_id = 0
)
BEGIN
	SELECT phpbb_lang_seq.nextval
	INTO :new.lang_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_log'
*/
CREATE TABLE phpbb_log (
	log_id number(8) NOT NULL,
	log_type number(4) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	reportee_id number(8) DEFAULT '0' NOT NULL,
	log_ip varchar2(40) DEFAULT '' ,
	log_time number(11) DEFAULT '0' NOT NULL,
	log_operation clob DEFAULT '' ,
	log_data clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_log PRIMARY KEY (log_id)
)
/

CREATE INDEX phpbb_log_log_type ON phpbb_log (log_type)
/
CREATE INDEX phpbb_log_forum_id ON phpbb_log (forum_id)
/
CREATE INDEX phpbb_log_topic_id ON phpbb_log (topic_id)
/
CREATE INDEX phpbb_log_reportee_id ON phpbb_log (reportee_id)
/
CREATE INDEX phpbb_log_user_id ON phpbb_log (user_id)
/

CREATE SEQUENCE phpbb_log_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_log
BEFORE INSERT ON phpbb_log
FOR EACH ROW WHEN (
	new.log_id IS NULL OR new.log_id = 0
)
BEGIN
	SELECT phpbb_log_seq.nextval
	INTO :new.log_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_moderator_cache'
*/
CREATE TABLE phpbb_moderator_cache (
	forum_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	username varchar2(765) DEFAULT '' ,
	group_id number(8) DEFAULT '0' NOT NULL,
	group_name varchar2(765) DEFAULT '' ,
	display_on_index number(1) DEFAULT '1' NOT NULL
)
/

CREATE INDEX phpbb_moderator_cache_disp_idx ON phpbb_moderator_cache (display_on_index)
/
CREATE INDEX phpbb_moderator_cache_forum_id ON phpbb_moderator_cache (forum_id)
/

/*
	Table: 'phpbb_modules'
*/
CREATE TABLE phpbb_modules (
	module_id number(8) NOT NULL,
	module_enabled number(1) DEFAULT '1' NOT NULL,
	module_display number(1) DEFAULT '1' NOT NULL,
	module_basename varchar2(255) DEFAULT '' ,
	module_class varchar2(10) DEFAULT '' ,
	parent_id number(8) DEFAULT '0' NOT NULL,
	left_id number(8) DEFAULT '0' NOT NULL,
	right_id number(8) DEFAULT '0' NOT NULL,
	module_langname varchar2(255) DEFAULT '' ,
	module_mode varchar2(255) DEFAULT '' ,
	module_auth varchar2(255) DEFAULT '' ,
	CONSTRAINT pk_phpbb_modules PRIMARY KEY (module_id)
)
/

CREATE INDEX phpbb_modules_left_right_id ON phpbb_modules (left_id, right_id)
/
CREATE INDEX phpbb_modules_module_enabled ON phpbb_modules (module_enabled)
/
CREATE INDEX phpbb_modules_class_left_id ON phpbb_modules (module_class, left_id)
/

CREATE SEQUENCE phpbb_modules_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_modules
BEFORE INSERT ON phpbb_modules
FOR EACH ROW WHEN (
	new.module_id IS NULL OR new.module_id = 0
)
BEGIN
	SELECT phpbb_modules_seq.nextval
	INTO :new.module_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_poll_options'
*/
CREATE TABLE phpbb_poll_options (
	poll_option_id number(4) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	poll_option_text clob DEFAULT '' ,
	poll_option_total number(8) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_poll_options_poll_opt_id ON phpbb_poll_options (poll_option_id)
/
CREATE INDEX phpbb_poll_options_topic_id ON phpbb_poll_options (topic_id)
/

/*
	Table: 'phpbb_poll_votes'
*/
CREATE TABLE phpbb_poll_votes (
	topic_id number(8) DEFAULT '0' NOT NULL,
	poll_option_id number(4) DEFAULT '0' NOT NULL,
	vote_user_id number(8) DEFAULT '0' NOT NULL,
	vote_user_ip varchar2(40) DEFAULT '' 
)
/

CREATE INDEX phpbb_poll_votes_topic_id ON phpbb_poll_votes (topic_id)
/
CREATE INDEX phpbb_poll_votes_vote_user_id ON phpbb_poll_votes (vote_user_id)
/
CREATE INDEX phpbb_poll_votes_vote_user_ip ON phpbb_poll_votes (vote_user_ip)
/

/*
	Table: 'phpbb_posts'
*/
CREATE TABLE phpbb_posts (
	post_id number(8) NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	poster_id number(8) DEFAULT '0' NOT NULL,
	icon_id number(8) DEFAULT '0' NOT NULL,
	poster_ip varchar2(40) DEFAULT '' ,
	post_time number(11) DEFAULT '0' NOT NULL,
	post_approved number(1) DEFAULT '1' NOT NULL,
	post_reported number(1) DEFAULT '0' NOT NULL,
	enable_bbcode number(1) DEFAULT '1' NOT NULL,
	enable_smilies number(1) DEFAULT '1' NOT NULL,
	enable_magic_url number(1) DEFAULT '1' NOT NULL,
	enable_sig number(1) DEFAULT '1' NOT NULL,
	post_username varchar2(765) DEFAULT '' ,
	post_subject varchar2(300) DEFAULT '' ,
	post_text clob DEFAULT '' ,
	post_checksum varchar2(32) DEFAULT '' ,
	post_attachment number(1) DEFAULT '0' NOT NULL,
	bbcode_bitfield varchar2(255) DEFAULT '' ,
	bbcode_uid varchar2(8) DEFAULT '' ,
	post_postcount number(1) DEFAULT '1' NOT NULL,
	post_edit_time number(11) DEFAULT '0' NOT NULL,
	post_edit_reason varchar2(765) DEFAULT '' ,
	post_edit_user number(8) DEFAULT '0' NOT NULL,
	post_edit_count number(4) DEFAULT '0' NOT NULL,
	post_edit_locked number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_posts PRIMARY KEY (post_id)
)
/

CREATE INDEX phpbb_posts_forum_id ON phpbb_posts (forum_id)
/
CREATE INDEX phpbb_posts_topic_id ON phpbb_posts (topic_id)
/
CREATE INDEX phpbb_posts_poster_ip ON phpbb_posts (poster_ip)
/
CREATE INDEX phpbb_posts_poster_id ON phpbb_posts (poster_id)
/
CREATE INDEX phpbb_posts_post_approved ON phpbb_posts (post_approved)
/
CREATE INDEX phpbb_posts_tid_post_time ON phpbb_posts (topic_id, post_time)
/

CREATE SEQUENCE phpbb_posts_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_posts
BEFORE INSERT ON phpbb_posts
FOR EACH ROW WHEN (
	new.post_id IS NULL OR new.post_id = 0
)
BEGIN
	SELECT phpbb_posts_seq.nextval
	INTO :new.post_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_privmsgs'
*/
CREATE TABLE phpbb_privmsgs (
	msg_id number(8) NOT NULL,
	root_level number(8) DEFAULT '0' NOT NULL,
	author_id number(8) DEFAULT '0' NOT NULL,
	icon_id number(8) DEFAULT '0' NOT NULL,
	author_ip varchar2(40) DEFAULT '' ,
	message_time number(11) DEFAULT '0' NOT NULL,
	enable_bbcode number(1) DEFAULT '1' NOT NULL,
	enable_smilies number(1) DEFAULT '1' NOT NULL,
	enable_magic_url number(1) DEFAULT '1' NOT NULL,
	enable_sig number(1) DEFAULT '1' NOT NULL,
	message_subject varchar2(300) DEFAULT '' ,
	message_text clob DEFAULT '' ,
	message_edit_reason varchar2(765) DEFAULT '' ,
	message_edit_user number(8) DEFAULT '0' NOT NULL,
	message_attachment number(1) DEFAULT '0' NOT NULL,
	bbcode_bitfield varchar2(255) DEFAULT '' ,
	bbcode_uid varchar2(8) DEFAULT '' ,
	message_edit_time number(11) DEFAULT '0' NOT NULL,
	message_edit_count number(4) DEFAULT '0' NOT NULL,
	to_address clob DEFAULT '' ,
	bcc_address clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_privmsgs PRIMARY KEY (msg_id)
)
/

CREATE INDEX phpbb_privmsgs_author_ip ON phpbb_privmsgs (author_ip)
/
CREATE INDEX phpbb_privmsgs_message_time ON phpbb_privmsgs (message_time)
/
CREATE INDEX phpbb_privmsgs_author_id ON phpbb_privmsgs (author_id)
/
CREATE INDEX phpbb_privmsgs_root_level ON phpbb_privmsgs (root_level)
/

CREATE SEQUENCE phpbb_privmsgs_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_privmsgs
BEFORE INSERT ON phpbb_privmsgs
FOR EACH ROW WHEN (
	new.msg_id IS NULL OR new.msg_id = 0
)
BEGIN
	SELECT phpbb_privmsgs_seq.nextval
	INTO :new.msg_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_privmsgs_folder'
*/
CREATE TABLE phpbb_privmsgs_folder (
	folder_id number(8) NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	folder_name varchar2(765) DEFAULT '' ,
	pm_count number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_privmsgs_folder PRIMARY KEY (folder_id)
)
/

CREATE INDEX phpbb_privmsgs_folder_user_id ON phpbb_privmsgs_folder (user_id)
/

CREATE SEQUENCE phpbb_privmsgs_folder_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_privmsgs_folder
BEFORE INSERT ON phpbb_privmsgs_folder
FOR EACH ROW WHEN (
	new.folder_id IS NULL OR new.folder_id = 0
)
BEGIN
	SELECT phpbb_privmsgs_folder_seq.nextval
	INTO :new.folder_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_privmsgs_rules'
*/
CREATE TABLE phpbb_privmsgs_rules (
	rule_id number(8) NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	rule_check number(8) DEFAULT '0' NOT NULL,
	rule_connection number(8) DEFAULT '0' NOT NULL,
	rule_string varchar2(765) DEFAULT '' ,
	rule_user_id number(8) DEFAULT '0' NOT NULL,
	rule_group_id number(8) DEFAULT '0' NOT NULL,
	rule_action number(8) DEFAULT '0' NOT NULL,
	rule_folder_id number(11) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_privmsgs_rules PRIMARY KEY (rule_id)
)
/

CREATE INDEX phpbb_privmsgs_rules_user_id ON phpbb_privmsgs_rules (user_id)
/

CREATE SEQUENCE phpbb_privmsgs_rules_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_privmsgs_rules
BEFORE INSERT ON phpbb_privmsgs_rules
FOR EACH ROW WHEN (
	new.rule_id IS NULL OR new.rule_id = 0
)
BEGIN
	SELECT phpbb_privmsgs_rules_seq.nextval
	INTO :new.rule_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_privmsgs_to'
*/
CREATE TABLE phpbb_privmsgs_to (
	msg_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	author_id number(8) DEFAULT '0' NOT NULL,
	pm_deleted number(1) DEFAULT '0' NOT NULL,
	pm_new number(1) DEFAULT '1' NOT NULL,
	pm_unread number(1) DEFAULT '1' NOT NULL,
	pm_replied number(1) DEFAULT '0' NOT NULL,
	pm_marked number(1) DEFAULT '0' NOT NULL,
	pm_forwarded number(1) DEFAULT '0' NOT NULL,
	folder_id number(11) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_privmsgs_to_msg_id ON phpbb_privmsgs_to (msg_id)
/
CREATE INDEX phpbb_privmsgs_to_author_id ON phpbb_privmsgs_to (author_id)
/
CREATE INDEX phpbb_privmsgs_to_usr_flder_id ON phpbb_privmsgs_to (user_id, folder_id)
/

/*
	Table: 'phpbb_profile_fields'
*/
CREATE TABLE phpbb_profile_fields (
	field_id number(8) NOT NULL,
	field_name varchar2(765) DEFAULT '' ,
	field_type number(4) DEFAULT '0' NOT NULL,
	field_ident varchar2(20) DEFAULT '' ,
	field_length varchar2(20) DEFAULT '' ,
	field_minlen varchar2(255) DEFAULT '' ,
	field_maxlen varchar2(255) DEFAULT '' ,
	field_novalue varchar2(765) DEFAULT '' ,
	field_default_value varchar2(765) DEFAULT '' ,
	field_validation varchar2(60) DEFAULT '' ,
	field_required number(1) DEFAULT '0' NOT NULL,
	field_show_on_reg number(1) DEFAULT '0' NOT NULL,
	field_hide number(1) DEFAULT '0' NOT NULL,
	field_no_view number(1) DEFAULT '0' NOT NULL,
	field_active number(1) DEFAULT '0' NOT NULL,
	field_order number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_profile_fields PRIMARY KEY (field_id)
)
/

CREATE INDEX phpbb_profile_fields_fld_type ON phpbb_profile_fields (field_type)
/
CREATE INDEX phpbb_profile_fields_fld_ordr ON phpbb_profile_fields (field_order)
/

CREATE SEQUENCE phpbb_profile_fields_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_profile_fields
BEFORE INSERT ON phpbb_profile_fields
FOR EACH ROW WHEN (
	new.field_id IS NULL OR new.field_id = 0
)
BEGIN
	SELECT phpbb_profile_fields_seq.nextval
	INTO :new.field_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_profile_fields_data'
*/
CREATE TABLE phpbb_profile_fields_data (
	user_id number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_profile_fields_data PRIMARY KEY (user_id)
)
/


/*
	Table: 'phpbb_profile_fields_lang'
*/
CREATE TABLE phpbb_profile_fields_lang (
	field_id number(8) DEFAULT '0' NOT NULL,
	lang_id number(8) DEFAULT '0' NOT NULL,
	option_id number(8) DEFAULT '0' NOT NULL,
	field_type number(4) DEFAULT '0' NOT NULL,
	lang_value varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_profile_fields_lang PRIMARY KEY (field_id, lang_id, option_id)
)
/


/*
	Table: 'phpbb_profile_lang'
*/
CREATE TABLE phpbb_profile_lang (
	field_id number(8) DEFAULT '0' NOT NULL,
	lang_id number(8) DEFAULT '0' NOT NULL,
	lang_name varchar2(765) DEFAULT '' ,
	lang_explain clob DEFAULT '' ,
	lang_default_value varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_profile_lang PRIMARY KEY (field_id, lang_id)
)
/


/*
	Table: 'phpbb_ranks'
*/
CREATE TABLE phpbb_ranks (
	rank_id number(8) NOT NULL,
	rank_title varchar2(765) DEFAULT '' ,
	rank_min number(8) DEFAULT '0' NOT NULL,
	rank_special number(1) DEFAULT '0' NOT NULL,
	rank_image varchar2(255) DEFAULT '' ,
	CONSTRAINT pk_phpbb_ranks PRIMARY KEY (rank_id)
)
/


CREATE SEQUENCE phpbb_ranks_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_ranks
BEFORE INSERT ON phpbb_ranks
FOR EACH ROW WHEN (
	new.rank_id IS NULL OR new.rank_id = 0
)
BEGIN
	SELECT phpbb_ranks_seq.nextval
	INTO :new.rank_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_reports'
*/
CREATE TABLE phpbb_reports (
	report_id number(8) NOT NULL,
	reason_id number(4) DEFAULT '0' NOT NULL,
	post_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	user_notify number(1) DEFAULT '0' NOT NULL,
	report_closed number(1) DEFAULT '0' NOT NULL,
	report_time number(11) DEFAULT '0' NOT NULL,
	report_text clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_reports PRIMARY KEY (report_id)
)
/


CREATE SEQUENCE phpbb_reports_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_reports
BEFORE INSERT ON phpbb_reports
FOR EACH ROW WHEN (
	new.report_id IS NULL OR new.report_id = 0
)
BEGIN
	SELECT phpbb_reports_seq.nextval
	INTO :new.report_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_reports_reasons'
*/
CREATE TABLE phpbb_reports_reasons (
	reason_id number(4) NOT NULL,
	reason_title varchar2(765) DEFAULT '' ,
	reason_description clob DEFAULT '' ,
	reason_order number(4) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_reports_reasons PRIMARY KEY (reason_id)
)
/


CREATE SEQUENCE phpbb_reports_reasons_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_reports_reasons
BEFORE INSERT ON phpbb_reports_reasons
FOR EACH ROW WHEN (
	new.reason_id IS NULL OR new.reason_id = 0
)
BEGIN
	SELECT phpbb_reports_reasons_seq.nextval
	INTO :new.reason_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_search_results'
*/
CREATE TABLE phpbb_search_results (
	search_key varchar2(32) DEFAULT '' ,
	search_time number(11) DEFAULT '0' NOT NULL,
	search_keywords clob DEFAULT '' ,
	search_authors clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_search_results PRIMARY KEY (search_key)
)
/


/*
	Table: 'phpbb_search_wordlist'
*/
CREATE TABLE phpbb_search_wordlist (
	word_id number(8) NOT NULL,
	word_text varchar2(765) DEFAULT '' ,
	word_common number(1) DEFAULT '0' NOT NULL,
	word_count number(8) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_search_wordlist PRIMARY KEY (word_id),
	CONSTRAINT u_phpbb_wrd_txt UNIQUE (word_text)
)
/

CREATE INDEX phpbb_search_wordlist_wrd_cnt ON phpbb_search_wordlist (word_count)
/

CREATE SEQUENCE phpbb_search_wordlist_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_search_wordlist
BEFORE INSERT ON phpbb_search_wordlist
FOR EACH ROW WHEN (
	new.word_id IS NULL OR new.word_id = 0
)
BEGIN
	SELECT phpbb_search_wordlist_seq.nextval
	INTO :new.word_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_search_wordmatch'
*/
CREATE TABLE phpbb_search_wordmatch (
	post_id number(8) DEFAULT '0' NOT NULL,
	word_id number(8) DEFAULT '0' NOT NULL,
	title_match number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT u_phpbb_unq_mtch UNIQUE (word_id, post_id, title_match)
)
/

CREATE INDEX phpbb_search_wordmatch_word_id ON phpbb_search_wordmatch (word_id)
/
CREATE INDEX phpbb_search_wordmatch_post_id ON phpbb_search_wordmatch (post_id)
/

/*
	Table: 'phpbb_sessions'
*/
CREATE TABLE phpbb_sessions (
	session_id char(32) DEFAULT '' ,
	session_user_id number(8) DEFAULT '0' NOT NULL,
	session_forum_id number(8) DEFAULT '0' NOT NULL,
	session_last_visit number(11) DEFAULT '0' NOT NULL,
	session_start number(11) DEFAULT '0' NOT NULL,
	session_time number(11) DEFAULT '0' NOT NULL,
	session_ip varchar2(40) DEFAULT '' ,
	session_browser varchar2(150) DEFAULT '' ,
	session_forwarded_for varchar2(255) DEFAULT '' ,
	session_page varchar2(765) DEFAULT '' ,
	session_viewonline number(1) DEFAULT '1' NOT NULL,
	session_autologin number(1) DEFAULT '0' NOT NULL,
	session_admin number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_sessions PRIMARY KEY (session_id)
)
/

CREATE INDEX phpbb_sessions_session_time ON phpbb_sessions (session_time)
/
CREATE INDEX phpbb_sessions_session_user_id ON phpbb_sessions (session_user_id)
/
CREATE INDEX phpbb_sessions_session_forum_id ON phpbb_sessions (session_forum_id)
/

/*
	Table: 'phpbb_sessions_keys'
*/
CREATE TABLE phpbb_sessions_keys (
	key_id char(32) DEFAULT '' ,
	user_id number(8) DEFAULT '0' NOT NULL,
	last_ip varchar2(40) DEFAULT '' ,
	last_login number(11) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_sessions_keys PRIMARY KEY (key_id, user_id)
)
/

CREATE INDEX phpbb_sessions_keys_last_login ON phpbb_sessions_keys (last_login)
/

/*
	Table: 'phpbb_sitelist'
*/
CREATE TABLE phpbb_sitelist (
	site_id number(8) NOT NULL,
	site_ip varchar2(40) DEFAULT '' ,
	site_hostname varchar2(255) DEFAULT '' ,
	ip_exclude number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_sitelist PRIMARY KEY (site_id)
)
/


CREATE SEQUENCE phpbb_sitelist_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_sitelist
BEFORE INSERT ON phpbb_sitelist
FOR EACH ROW WHEN (
	new.site_id IS NULL OR new.site_id = 0
)
BEGIN
	SELECT phpbb_sitelist_seq.nextval
	INTO :new.site_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_smilies'
*/
CREATE TABLE phpbb_smilies (
	smiley_id number(8) NOT NULL,
	code varchar2(150) DEFAULT '' ,
	emotion varchar2(150) DEFAULT '' ,
	smiley_url varchar2(50) DEFAULT '' ,
	smiley_width number(4) DEFAULT '0' NOT NULL,
	smiley_height number(4) DEFAULT '0' NOT NULL,
	smiley_order number(8) DEFAULT '0' NOT NULL,
	display_on_posting number(1) DEFAULT '1' NOT NULL,
	CONSTRAINT pk_phpbb_smilies PRIMARY KEY (smiley_id)
)
/

CREATE INDEX phpbb_smilies_display_on_post ON phpbb_smilies (display_on_posting)
/

CREATE SEQUENCE phpbb_smilies_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_smilies
BEFORE INSERT ON phpbb_smilies
FOR EACH ROW WHEN (
	new.smiley_id IS NULL OR new.smiley_id = 0
)
BEGIN
	SELECT phpbb_smilies_seq.nextval
	INTO :new.smiley_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_styles'
*/
CREATE TABLE phpbb_styles (
	style_id number(4) NOT NULL,
	style_name varchar2(765) DEFAULT '' ,
	style_copyright varchar2(765) DEFAULT '' ,
	style_active number(1) DEFAULT '1' NOT NULL,
	template_id number(4) DEFAULT '0' NOT NULL,
	theme_id number(4) DEFAULT '0' NOT NULL,
	imageset_id number(4) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_styles PRIMARY KEY (style_id),
	CONSTRAINT u_phpbb_style_name UNIQUE (style_name)
)
/

CREATE INDEX phpbb_styles_template_id ON phpbb_styles (template_id)
/
CREATE INDEX phpbb_styles_theme_id ON phpbb_styles (theme_id)
/
CREATE INDEX phpbb_styles_imageset_id ON phpbb_styles (imageset_id)
/

CREATE SEQUENCE phpbb_styles_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_styles
BEFORE INSERT ON phpbb_styles
FOR EACH ROW WHEN (
	new.style_id IS NULL OR new.style_id = 0
)
BEGIN
	SELECT phpbb_styles_seq.nextval
	INTO :new.style_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_styles_template'
*/
CREATE TABLE phpbb_styles_template (
	template_id number(4) NOT NULL,
	template_name varchar2(765) DEFAULT '' ,
	template_copyright varchar2(765) DEFAULT '' ,
	template_path varchar2(100) DEFAULT '' ,
	bbcode_bitfield varchar2(255) DEFAULT 'kNg=' NOT NULL,
	template_storedb number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_styles_template PRIMARY KEY (template_id),
	CONSTRAINT u_phpbb_tmplte_nm UNIQUE (template_name)
)
/


CREATE SEQUENCE phpbb_styles_template_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_styles_template
BEFORE INSERT ON phpbb_styles_template
FOR EACH ROW WHEN (
	new.template_id IS NULL OR new.template_id = 0
)
BEGIN
	SELECT phpbb_styles_template_seq.nextval
	INTO :new.template_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_styles_template_data'
*/
CREATE TABLE phpbb_styles_template_data (
	template_id number(4) DEFAULT '0' NOT NULL,
	template_filename varchar2(100) DEFAULT '' ,
	template_included clob DEFAULT '' ,
	template_mtime number(11) DEFAULT '0' NOT NULL,
	template_data clob DEFAULT '' 
)
/

CREATE INDEX phpbb_styles_template_data_tid ON phpbb_styles_template_data (template_id)
/
CREATE INDEX phpbb_styles_template_data_tfn ON phpbb_styles_template_data (template_filename)
/

/*
	Table: 'phpbb_styles_theme'
*/
CREATE TABLE phpbb_styles_theme (
	theme_id number(4) NOT NULL,
	theme_name varchar2(765) DEFAULT '' ,
	theme_copyright varchar2(765) DEFAULT '' ,
	theme_path varchar2(100) DEFAULT '' ,
	theme_storedb number(1) DEFAULT '0' NOT NULL,
	theme_mtime number(11) DEFAULT '0' NOT NULL,
	theme_data clob DEFAULT '' ,
	CONSTRAINT pk_phpbb_styles_theme PRIMARY KEY (theme_id),
	CONSTRAINT u_phpbb_theme_name UNIQUE (theme_name)
)
/


CREATE SEQUENCE phpbb_styles_theme_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_styles_theme
BEFORE INSERT ON phpbb_styles_theme
FOR EACH ROW WHEN (
	new.theme_id IS NULL OR new.theme_id = 0
)
BEGIN
	SELECT phpbb_styles_theme_seq.nextval
	INTO :new.theme_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_styles_imageset'
*/
CREATE TABLE phpbb_styles_imageset (
	imageset_id number(4) NOT NULL,
	imageset_name varchar2(765) DEFAULT '' ,
	imageset_copyright varchar2(765) DEFAULT '' ,
	imageset_path varchar2(100) DEFAULT '' ,
	CONSTRAINT pk_phpbb_styles_imageset PRIMARY KEY (imageset_id),
	CONSTRAINT u_phpbb_imgset_nm UNIQUE (imageset_name)
)
/


CREATE SEQUENCE phpbb_styles_imageset_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_styles_imageset
BEFORE INSERT ON phpbb_styles_imageset
FOR EACH ROW WHEN (
	new.imageset_id IS NULL OR new.imageset_id = 0
)
BEGIN
	SELECT phpbb_styles_imageset_seq.nextval
	INTO :new.imageset_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_styles_imageset_data'
*/
CREATE TABLE phpbb_styles_imageset_data (
	image_id number(4) NOT NULL,
	image_name varchar2(200) DEFAULT '' ,
	image_filename varchar2(200) DEFAULT '' ,
	image_lang varchar2(30) DEFAULT '' ,
	image_height number(4) DEFAULT '0' NOT NULL,
	image_width number(4) DEFAULT '0' NOT NULL,
	imageset_id number(4) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_styles_imageset_data PRIMARY KEY (image_id)
)
/

CREATE INDEX phpbb_styles_imageset_data_i_d ON phpbb_styles_imageset_data (imageset_id)
/

CREATE SEQUENCE phpbb_styles_imageset_data_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_styles_imageset_data
BEFORE INSERT ON phpbb_styles_imageset_data
FOR EACH ROW WHEN (
	new.image_id IS NULL OR new.image_id = 0
)
BEGIN
	SELECT phpbb_styles_imageset_data_seq.nextval
	INTO :new.image_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_topics'
*/
CREATE TABLE phpbb_topics (
	topic_id number(8) NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	icon_id number(8) DEFAULT '0' NOT NULL,
	topic_attachment number(1) DEFAULT '0' NOT NULL,
	topic_approved number(1) DEFAULT '1' NOT NULL,
	topic_reported number(1) DEFAULT '0' NOT NULL,
	topic_title varchar2(300) DEFAULT '' ,
	topic_poster number(8) DEFAULT '0' NOT NULL,
	topic_time number(11) DEFAULT '0' NOT NULL,
	topic_time_limit number(11) DEFAULT '0' NOT NULL,
	topic_views number(8) DEFAULT '0' NOT NULL,
	topic_replies number(8) DEFAULT '0' NOT NULL,
	topic_replies_real number(8) DEFAULT '0' NOT NULL,
	topic_status number(3) DEFAULT '0' NOT NULL,
	topic_type number(3) DEFAULT '0' NOT NULL,
	topic_first_post_id number(8) DEFAULT '0' NOT NULL,
	topic_first_poster_name varchar2(765) DEFAULT '' ,
	topic_first_poster_colour varchar2(6) DEFAULT '' ,
	topic_last_post_id number(8) DEFAULT '0' NOT NULL,
	topic_last_poster_id number(8) DEFAULT '0' NOT NULL,
	topic_last_poster_name varchar2(765) DEFAULT '' ,
	topic_last_poster_colour varchar2(6) DEFAULT '' ,
	topic_last_post_subject varchar2(300) DEFAULT '' ,
	topic_last_post_time number(11) DEFAULT '0' NOT NULL,
	topic_last_view_time number(11) DEFAULT '0' NOT NULL,
	topic_moved_id number(8) DEFAULT '0' NOT NULL,
	topic_bumped number(1) DEFAULT '0' NOT NULL,
	topic_bumper number(8) DEFAULT '0' NOT NULL,
	poll_title varchar2(765) DEFAULT '' ,
	poll_start number(11) DEFAULT '0' NOT NULL,
	poll_length number(11) DEFAULT '0' NOT NULL,
	poll_max_options number(4) DEFAULT '1' NOT NULL,
	poll_last_vote number(11) DEFAULT '0' NOT NULL,
	poll_vote_change number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_topics PRIMARY KEY (topic_id)
)
/

CREATE INDEX phpbb_topics_forum_id ON phpbb_topics (forum_id)
/
CREATE INDEX phpbb_topics_forum_id_type ON phpbb_topics (forum_id, topic_type)
/
CREATE INDEX phpbb_topics_last_post_time ON phpbb_topics (topic_last_post_time)
/
CREATE INDEX phpbb_topics_topic_approved ON phpbb_topics (topic_approved)
/
CREATE INDEX phpbb_topics_forum_appr_last ON phpbb_topics (forum_id, topic_approved, topic_last_post_id)
/
CREATE INDEX phpbb_topics_fid_time_moved ON phpbb_topics (forum_id, topic_last_post_time, topic_moved_id)
/

CREATE SEQUENCE phpbb_topics_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_topics
BEFORE INSERT ON phpbb_topics
FOR EACH ROW WHEN (
	new.topic_id IS NULL OR new.topic_id = 0
)
BEGIN
	SELECT phpbb_topics_seq.nextval
	INTO :new.topic_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_topics_track'
*/
CREATE TABLE phpbb_topics_track (
	user_id number(8) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	forum_id number(8) DEFAULT '0' NOT NULL,
	mark_time number(11) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_topics_track PRIMARY KEY (user_id, topic_id)
)
/

CREATE INDEX phpbb_topics_track_forum_id ON phpbb_topics_track (forum_id)
/

/*
	Table: 'phpbb_topics_posted'
*/
CREATE TABLE phpbb_topics_posted (
	user_id number(8) DEFAULT '0' NOT NULL,
	topic_id number(8) DEFAULT '0' NOT NULL,
	topic_posted number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_topics_posted PRIMARY KEY (user_id, topic_id)
)
/


/*
	Table: 'phpbb_topics_watch'
*/
CREATE TABLE phpbb_topics_watch (
	topic_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	notify_status number(1) DEFAULT '0' NOT NULL
)
/

CREATE INDEX phpbb_topics_watch_topic_id ON phpbb_topics_watch (topic_id)
/
CREATE INDEX phpbb_topics_watch_user_id ON phpbb_topics_watch (user_id)
/
CREATE INDEX phpbb_topics_watch_notify_stat ON phpbb_topics_watch (notify_status)
/

/*
	Table: 'phpbb_user_group'
*/
CREATE TABLE phpbb_user_group (
	group_id number(8) DEFAULT '0' NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	group_leader number(1) DEFAULT '0' NOT NULL,
	user_pending number(1) DEFAULT '1' NOT NULL
)
/

CREATE INDEX phpbb_user_group_group_id ON phpbb_user_group (group_id)
/
CREATE INDEX phpbb_user_group_user_id ON phpbb_user_group (user_id)
/
CREATE INDEX phpbb_user_group_group_leader ON phpbb_user_group (group_leader)
/

/*
	Table: 'phpbb_users'
*/
CREATE TABLE phpbb_users (
	user_id number(8) NOT NULL,
	user_type number(2) DEFAULT '0' NOT NULL,
	group_id number(8) DEFAULT '3' NOT NULL,
	user_permissions clob DEFAULT '' ,
	user_perm_from number(8) DEFAULT '0' NOT NULL,
	user_ip varchar2(40) DEFAULT '' ,
	user_regdate number(11) DEFAULT '0' NOT NULL,
	username varchar2(255) DEFAULT '' ,
	username_clean varchar2(255) DEFAULT '' ,
	user_password varchar2(120) DEFAULT '' ,
	user_passchg number(11) DEFAULT '0' NOT NULL,
	user_pass_convert number(1) DEFAULT '0' NOT NULL,
	user_email varchar2(300) DEFAULT '' ,
	user_email_hash number(20) DEFAULT '0' NOT NULL,
	user_birthday varchar2(10) DEFAULT '' ,
	user_lastvisit number(11) DEFAULT '0' NOT NULL,
	user_lastmark number(11) DEFAULT '0' NOT NULL,
	user_lastpost_time number(11) DEFAULT '0' NOT NULL,
	user_lastpage varchar2(600) DEFAULT '' ,
	user_last_confirm_key varchar2(10) DEFAULT '' ,
	user_last_search number(11) DEFAULT '0' NOT NULL,
	user_warnings number(4) DEFAULT '0' NOT NULL,
	user_last_warning number(11) DEFAULT '0' NOT NULL,
	user_login_attempts number(4) DEFAULT '0' NOT NULL,
	user_inactive_reason number(2) DEFAULT '0' NOT NULL,
	user_inactive_time number(11) DEFAULT '0' NOT NULL,
	user_posts number(8) DEFAULT '0' NOT NULL,
	user_lang varchar2(30) DEFAULT '' ,
	user_timezone number(5, 2) DEFAULT '0' NOT NULL,
	user_dst number(1) DEFAULT '0' NOT NULL,
	user_dateformat varchar2(90) DEFAULT 'd M Y H:i' NOT NULL,
	user_style number(4) DEFAULT '0' NOT NULL,
	user_rank number(8) DEFAULT '0' NOT NULL,
	user_colour varchar2(6) DEFAULT '' ,
	user_new_privmsg number(4) DEFAULT '0' NOT NULL,
	user_unread_privmsg number(4) DEFAULT '0' NOT NULL,
	user_last_privmsg number(11) DEFAULT '0' NOT NULL,
	user_message_rules number(1) DEFAULT '0' NOT NULL,
	user_full_folder number(11) DEFAULT '-3' NOT NULL,
	user_emailtime number(11) DEFAULT '0' NOT NULL,
	user_topic_show_days number(4) DEFAULT '0' NOT NULL,
	user_topic_sortby_type varchar2(1) DEFAULT 't' NOT NULL,
	user_topic_sortby_dir varchar2(1) DEFAULT 'd' NOT NULL,
	user_post_show_days number(4) DEFAULT '0' NOT NULL,
	user_post_sortby_type varchar2(1) DEFAULT 't' NOT NULL,
	user_post_sortby_dir varchar2(1) DEFAULT 'a' NOT NULL,
	user_notify number(1) DEFAULT '0' NOT NULL,
	user_notify_pm number(1) DEFAULT '1' NOT NULL,
	user_notify_type number(4) DEFAULT '0' NOT NULL,
	user_allow_pm number(1) DEFAULT '1' NOT NULL,
	user_allow_viewonline number(1) DEFAULT '1' NOT NULL,
	user_allow_viewemail number(1) DEFAULT '1' NOT NULL,
	user_allow_massemail number(1) DEFAULT '1' NOT NULL,
	user_options number(11) DEFAULT '895' NOT NULL,
	user_avatar varchar2(255) DEFAULT '' ,
	user_avatar_type number(2) DEFAULT '0' NOT NULL,
	user_avatar_width number(4) DEFAULT '0' NOT NULL,
	user_avatar_height number(4) DEFAULT '0' NOT NULL,
	user_sig clob DEFAULT '' ,
	user_sig_bbcode_uid varchar2(8) DEFAULT '' ,
	user_sig_bbcode_bitfield varchar2(255) DEFAULT '' ,
	user_from varchar2(300) DEFAULT '' ,
	user_icq varchar2(15) DEFAULT '' ,
	user_aim varchar2(765) DEFAULT '' ,
	user_yim varchar2(765) DEFAULT '' ,
	user_msnm varchar2(765) DEFAULT '' ,
	user_jabber varchar2(765) DEFAULT '' ,
	user_website varchar2(600) DEFAULT '' ,
	user_occ clob DEFAULT '' ,
	user_interests clob DEFAULT '' ,
	user_actkey varchar2(32) DEFAULT '' ,
	user_newpasswd varchar2(120) DEFAULT '' ,
	user_form_salt varchar2(96) DEFAULT '' ,
	CONSTRAINT pk_phpbb_users PRIMARY KEY (user_id),
	CONSTRAINT u_phpbb_username_clean UNIQUE (username_clean)
)
/

CREATE INDEX phpbb_users_user_birthday ON phpbb_users (user_birthday)
/
CREATE INDEX phpbb_users_user_email_hash ON phpbb_users (user_email_hash)
/
CREATE INDEX phpbb_users_user_type ON phpbb_users (user_type)
/

CREATE SEQUENCE phpbb_users_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_users
BEFORE INSERT ON phpbb_users
FOR EACH ROW WHEN (
	new.user_id IS NULL OR new.user_id = 0
)
BEGIN
	SELECT phpbb_users_seq.nextval
	INTO :new.user_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_warnings'
*/
CREATE TABLE phpbb_warnings (
	warning_id number(8) NOT NULL,
	user_id number(8) DEFAULT '0' NOT NULL,
	post_id number(8) DEFAULT '0' NOT NULL,
	log_id number(8) DEFAULT '0' NOT NULL,
	warning_time number(11) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_warnings PRIMARY KEY (warning_id)
)
/


CREATE SEQUENCE phpbb_warnings_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_warnings
BEFORE INSERT ON phpbb_warnings
FOR EACH ROW WHEN (
	new.warning_id IS NULL OR new.warning_id = 0
)
BEGIN
	SELECT phpbb_warnings_seq.nextval
	INTO :new.warning_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_words'
*/
CREATE TABLE phpbb_words (
	word_id number(8) NOT NULL,
	word varchar2(765) DEFAULT '' ,
	replacement varchar2(765) DEFAULT '' ,
	CONSTRAINT pk_phpbb_words PRIMARY KEY (word_id)
)
/


CREATE SEQUENCE phpbb_words_seq
/

CREATE OR REPLACE TRIGGER t_phpbb_words
BEFORE INSERT ON phpbb_words
FOR EACH ROW WHEN (
	new.word_id IS NULL OR new.word_id = 0
)
BEGIN
	SELECT phpbb_words_seq.nextval
	INTO :new.word_id
	FROM dual;
END;
/


/*
	Table: 'phpbb_zebra'
*/
CREATE TABLE phpbb_zebra (
	user_id number(8) DEFAULT '0' NOT NULL,
	zebra_id number(8) DEFAULT '0' NOT NULL,
	friend number(1) DEFAULT '0' NOT NULL,
	foe number(1) DEFAULT '0' NOT NULL,
	CONSTRAINT pk_phpbb_zebra PRIMARY KEY (user_id, zebra_id)
)
/


