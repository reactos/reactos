/*

 $Id: postgres_schema.sql 8456 2008-03-22 12:31:17Z Kellanved $

*/

BEGIN;

/*
	Domain definition
*/
CREATE DOMAIN varchar_ci AS varchar(255) NOT NULL DEFAULT ''::character varying;

/*
	Operation Functions
*/
CREATE FUNCTION _varchar_ci_equal(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) = LOWER($2)' LANGUAGE SQL STRICT;
CREATE FUNCTION _varchar_ci_not_equal(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) != LOWER($2)' LANGUAGE SQL STRICT;
CREATE FUNCTION _varchar_ci_less_than(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) < LOWER($2)' LANGUAGE SQL STRICT;
CREATE FUNCTION _varchar_ci_less_equal(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) <= LOWER($2)' LANGUAGE SQL STRICT;
CREATE FUNCTION _varchar_ci_greater_than(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) > LOWER($2)' LANGUAGE SQL STRICT;
CREATE FUNCTION _varchar_ci_greater_equals(varchar_ci, varchar_ci) RETURNS boolean AS 'SELECT LOWER($1) >= LOWER($2)' LANGUAGE SQL STRICT;

/*
	Operators
*/
CREATE OPERATOR <(
  PROCEDURE = _varchar_ci_less_than,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel);

CREATE OPERATOR <=(
  PROCEDURE = _varchar_ci_less_equal,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel);

CREATE OPERATOR >(
  PROCEDURE = _varchar_ci_greater_than,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel);

CREATE OPERATOR >=(
  PROCEDURE = _varchar_ci_greater_equals,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel);

CREATE OPERATOR <>(
  PROCEDURE = _varchar_ci_not_equal,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel);

CREATE OPERATOR =(
  PROCEDURE = _varchar_ci_equal,
  LEFTARG = varchar_ci,
  RIGHTARG = varchar_ci,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES,
  MERGES,
  SORT1= <);

/*
	Table: 'phpbb_attachments'
*/
CREATE SEQUENCE phpbb_attachments_seq;

CREATE TABLE phpbb_attachments (
	attach_id INT4 DEFAULT nextval('phpbb_attachments_seq'),
	post_msg_id INT4 DEFAULT '0' NOT NULL CHECK (post_msg_id >= 0),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	in_message INT2 DEFAULT '0' NOT NULL CHECK (in_message >= 0),
	poster_id INT4 DEFAULT '0' NOT NULL CHECK (poster_id >= 0),
	is_orphan INT2 DEFAULT '1' NOT NULL CHECK (is_orphan >= 0),
	physical_filename varchar(255) DEFAULT '' NOT NULL,
	real_filename varchar(255) DEFAULT '' NOT NULL,
	download_count INT4 DEFAULT '0' NOT NULL CHECK (download_count >= 0),
	attach_comment varchar(4000) DEFAULT '' NOT NULL,
	extension varchar(100) DEFAULT '' NOT NULL,
	mimetype varchar(100) DEFAULT '' NOT NULL,
	filesize INT4 DEFAULT '0' NOT NULL CHECK (filesize >= 0),
	filetime INT4 DEFAULT '0' NOT NULL CHECK (filetime >= 0),
	thumbnail INT2 DEFAULT '0' NOT NULL CHECK (thumbnail >= 0),
	PRIMARY KEY (attach_id)
);

CREATE INDEX phpbb_attachments_filetime ON phpbb_attachments (filetime);
CREATE INDEX phpbb_attachments_post_msg_id ON phpbb_attachments (post_msg_id);
CREATE INDEX phpbb_attachments_topic_id ON phpbb_attachments (topic_id);
CREATE INDEX phpbb_attachments_poster_id ON phpbb_attachments (poster_id);
CREATE INDEX phpbb_attachments_is_orphan ON phpbb_attachments (is_orphan);

/*
	Table: 'phpbb_acl_groups'
*/
CREATE TABLE phpbb_acl_groups (
	group_id INT4 DEFAULT '0' NOT NULL CHECK (group_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	auth_option_id INT4 DEFAULT '0' NOT NULL CHECK (auth_option_id >= 0),
	auth_role_id INT4 DEFAULT '0' NOT NULL CHECK (auth_role_id >= 0),
	auth_setting INT2 DEFAULT '0' NOT NULL
);

CREATE INDEX phpbb_acl_groups_group_id ON phpbb_acl_groups (group_id);
CREATE INDEX phpbb_acl_groups_auth_opt_id ON phpbb_acl_groups (auth_option_id);
CREATE INDEX phpbb_acl_groups_auth_role_id ON phpbb_acl_groups (auth_role_id);

/*
	Table: 'phpbb_acl_options'
*/
CREATE SEQUENCE phpbb_acl_options_seq;

CREATE TABLE phpbb_acl_options (
	auth_option_id INT4 DEFAULT nextval('phpbb_acl_options_seq'),
	auth_option varchar(50) DEFAULT '' NOT NULL,
	is_global INT2 DEFAULT '0' NOT NULL CHECK (is_global >= 0),
	is_local INT2 DEFAULT '0' NOT NULL CHECK (is_local >= 0),
	founder_only INT2 DEFAULT '0' NOT NULL CHECK (founder_only >= 0),
	PRIMARY KEY (auth_option_id)
);

CREATE INDEX phpbb_acl_options_auth_option ON phpbb_acl_options (auth_option);

/*
	Table: 'phpbb_acl_roles'
*/
CREATE SEQUENCE phpbb_acl_roles_seq;

CREATE TABLE phpbb_acl_roles (
	role_id INT4 DEFAULT nextval('phpbb_acl_roles_seq'),
	role_name varchar(255) DEFAULT '' NOT NULL,
	role_description varchar(4000) DEFAULT '' NOT NULL,
	role_type varchar(10) DEFAULT '' NOT NULL,
	role_order INT2 DEFAULT '0' NOT NULL CHECK (role_order >= 0),
	PRIMARY KEY (role_id)
);

CREATE INDEX phpbb_acl_roles_role_type ON phpbb_acl_roles (role_type);
CREATE INDEX phpbb_acl_roles_role_order ON phpbb_acl_roles (role_order);

/*
	Table: 'phpbb_acl_roles_data'
*/
CREATE TABLE phpbb_acl_roles_data (
	role_id INT4 DEFAULT '0' NOT NULL CHECK (role_id >= 0),
	auth_option_id INT4 DEFAULT '0' NOT NULL CHECK (auth_option_id >= 0),
	auth_setting INT2 DEFAULT '0' NOT NULL,
	PRIMARY KEY (role_id, auth_option_id)
);

CREATE INDEX phpbb_acl_roles_data_ath_op_id ON phpbb_acl_roles_data (auth_option_id);

/*
	Table: 'phpbb_acl_users'
*/
CREATE TABLE phpbb_acl_users (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	auth_option_id INT4 DEFAULT '0' NOT NULL CHECK (auth_option_id >= 0),
	auth_role_id INT4 DEFAULT '0' NOT NULL CHECK (auth_role_id >= 0),
	auth_setting INT2 DEFAULT '0' NOT NULL
);

CREATE INDEX phpbb_acl_users_user_id ON phpbb_acl_users (user_id);
CREATE INDEX phpbb_acl_users_auth_option_id ON phpbb_acl_users (auth_option_id);
CREATE INDEX phpbb_acl_users_auth_role_id ON phpbb_acl_users (auth_role_id);

/*
	Table: 'phpbb_banlist'
*/
CREATE SEQUENCE phpbb_banlist_seq;

CREATE TABLE phpbb_banlist (
	ban_id INT4 DEFAULT nextval('phpbb_banlist_seq'),
	ban_userid INT4 DEFAULT '0' NOT NULL CHECK (ban_userid >= 0),
	ban_ip varchar(40) DEFAULT '' NOT NULL,
	ban_email varchar(100) DEFAULT '' NOT NULL,
	ban_start INT4 DEFAULT '0' NOT NULL CHECK (ban_start >= 0),
	ban_end INT4 DEFAULT '0' NOT NULL CHECK (ban_end >= 0),
	ban_exclude INT2 DEFAULT '0' NOT NULL CHECK (ban_exclude >= 0),
	ban_reason varchar(255) DEFAULT '' NOT NULL,
	ban_give_reason varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (ban_id)
);

CREATE INDEX phpbb_banlist_ban_end ON phpbb_banlist (ban_end);
CREATE INDEX phpbb_banlist_ban_user ON phpbb_banlist (ban_userid, ban_exclude);
CREATE INDEX phpbb_banlist_ban_email ON phpbb_banlist (ban_email, ban_exclude);
CREATE INDEX phpbb_banlist_ban_ip ON phpbb_banlist (ban_ip, ban_exclude);

/*
	Table: 'phpbb_bbcodes'
*/
CREATE TABLE phpbb_bbcodes (
	bbcode_id INT2 DEFAULT '0' NOT NULL,
	bbcode_tag varchar(16) DEFAULT '' NOT NULL,
	bbcode_helpline varchar(255) DEFAULT '' NOT NULL,
	display_on_posting INT2 DEFAULT '0' NOT NULL CHECK (display_on_posting >= 0),
	bbcode_match varchar(4000) DEFAULT '' NOT NULL,
	bbcode_tpl TEXT DEFAULT '' NOT NULL,
	first_pass_match TEXT DEFAULT '' NOT NULL,
	first_pass_replace TEXT DEFAULT '' NOT NULL,
	second_pass_match TEXT DEFAULT '' NOT NULL,
	second_pass_replace TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (bbcode_id)
);

CREATE INDEX phpbb_bbcodes_display_on_post ON phpbb_bbcodes (display_on_posting);

/*
	Table: 'phpbb_bookmarks'
*/
CREATE TABLE phpbb_bookmarks (
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	PRIMARY KEY (topic_id, user_id)
);


/*
	Table: 'phpbb_bots'
*/
CREATE SEQUENCE phpbb_bots_seq;

CREATE TABLE phpbb_bots (
	bot_id INT4 DEFAULT nextval('phpbb_bots_seq'),
	bot_active INT2 DEFAULT '1' NOT NULL CHECK (bot_active >= 0),
	bot_name varchar(255) DEFAULT '' NOT NULL,
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	bot_agent varchar(255) DEFAULT '' NOT NULL,
	bot_ip varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (bot_id)
);

CREATE INDEX phpbb_bots_bot_active ON phpbb_bots (bot_active);

/*
	Table: 'phpbb_config'
*/
CREATE TABLE phpbb_config (
	config_name varchar(255) DEFAULT '' NOT NULL,
	config_value varchar(255) DEFAULT '' NOT NULL,
	is_dynamic INT2 DEFAULT '0' NOT NULL CHECK (is_dynamic >= 0),
	PRIMARY KEY (config_name)
);

CREATE INDEX phpbb_config_is_dynamic ON phpbb_config (is_dynamic);

/*
	Table: 'phpbb_confirm'
*/
CREATE TABLE phpbb_confirm (
	confirm_id char(32) DEFAULT '' NOT NULL,
	session_id char(32) DEFAULT '' NOT NULL,
	confirm_type INT2 DEFAULT '0' NOT NULL,
	code varchar(8) DEFAULT '' NOT NULL,
	seed INT4 DEFAULT '0' NOT NULL CHECK (seed >= 0),
	PRIMARY KEY (session_id, confirm_id)
);

CREATE INDEX phpbb_confirm_confirm_type ON phpbb_confirm (confirm_type);

/*
	Table: 'phpbb_disallow'
*/
CREATE SEQUENCE phpbb_disallow_seq;

CREATE TABLE phpbb_disallow (
	disallow_id INT4 DEFAULT nextval('phpbb_disallow_seq'),
	disallow_username varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (disallow_id)
);


/*
	Table: 'phpbb_drafts'
*/
CREATE SEQUENCE phpbb_drafts_seq;

CREATE TABLE phpbb_drafts (
	draft_id INT4 DEFAULT nextval('phpbb_drafts_seq'),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	save_time INT4 DEFAULT '0' NOT NULL CHECK (save_time >= 0),
	draft_subject varchar(100) DEFAULT '' NOT NULL,
	draft_message TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (draft_id)
);

CREATE INDEX phpbb_drafts_save_time ON phpbb_drafts (save_time);

/*
	Table: 'phpbb_extensions'
*/
CREATE SEQUENCE phpbb_extensions_seq;

CREATE TABLE phpbb_extensions (
	extension_id INT4 DEFAULT nextval('phpbb_extensions_seq'),
	group_id INT4 DEFAULT '0' NOT NULL CHECK (group_id >= 0),
	extension varchar(100) DEFAULT '' NOT NULL,
	PRIMARY KEY (extension_id)
);


/*
	Table: 'phpbb_extension_groups'
*/
CREATE SEQUENCE phpbb_extension_groups_seq;

CREATE TABLE phpbb_extension_groups (
	group_id INT4 DEFAULT nextval('phpbb_extension_groups_seq'),
	group_name varchar(255) DEFAULT '' NOT NULL,
	cat_id INT2 DEFAULT '0' NOT NULL,
	allow_group INT2 DEFAULT '0' NOT NULL CHECK (allow_group >= 0),
	download_mode INT2 DEFAULT '1' NOT NULL CHECK (download_mode >= 0),
	upload_icon varchar(255) DEFAULT '' NOT NULL,
	max_filesize INT4 DEFAULT '0' NOT NULL CHECK (max_filesize >= 0),
	allowed_forums varchar(8000) DEFAULT '' NOT NULL,
	allow_in_pm INT2 DEFAULT '0' NOT NULL CHECK (allow_in_pm >= 0),
	PRIMARY KEY (group_id)
);


/*
	Table: 'phpbb_forums'
*/
CREATE SEQUENCE phpbb_forums_seq;

CREATE TABLE phpbb_forums (
	forum_id INT4 DEFAULT nextval('phpbb_forums_seq'),
	parent_id INT4 DEFAULT '0' NOT NULL CHECK (parent_id >= 0),
	left_id INT4 DEFAULT '0' NOT NULL CHECK (left_id >= 0),
	right_id INT4 DEFAULT '0' NOT NULL CHECK (right_id >= 0),
	forum_parents TEXT DEFAULT '' NOT NULL,
	forum_name varchar(255) DEFAULT '' NOT NULL,
	forum_desc varchar(4000) DEFAULT '' NOT NULL,
	forum_desc_bitfield varchar(255) DEFAULT '' NOT NULL,
	forum_desc_options INT4 DEFAULT '7' NOT NULL CHECK (forum_desc_options >= 0),
	forum_desc_uid varchar(8) DEFAULT '' NOT NULL,
	forum_link varchar(255) DEFAULT '' NOT NULL,
	forum_password varchar(40) DEFAULT '' NOT NULL,
	forum_style INT2 DEFAULT '0' NOT NULL CHECK (forum_style >= 0),
	forum_image varchar(255) DEFAULT '' NOT NULL,
	forum_rules varchar(4000) DEFAULT '' NOT NULL,
	forum_rules_link varchar(255) DEFAULT '' NOT NULL,
	forum_rules_bitfield varchar(255) DEFAULT '' NOT NULL,
	forum_rules_options INT4 DEFAULT '7' NOT NULL CHECK (forum_rules_options >= 0),
	forum_rules_uid varchar(8) DEFAULT '' NOT NULL,
	forum_topics_per_page INT2 DEFAULT '0' NOT NULL,
	forum_type INT2 DEFAULT '0' NOT NULL,
	forum_status INT2 DEFAULT '0' NOT NULL,
	forum_posts INT4 DEFAULT '0' NOT NULL CHECK (forum_posts >= 0),
	forum_topics INT4 DEFAULT '0' NOT NULL CHECK (forum_topics >= 0),
	forum_topics_real INT4 DEFAULT '0' NOT NULL CHECK (forum_topics_real >= 0),
	forum_last_post_id INT4 DEFAULT '0' NOT NULL CHECK (forum_last_post_id >= 0),
	forum_last_poster_id INT4 DEFAULT '0' NOT NULL CHECK (forum_last_poster_id >= 0),
	forum_last_post_subject varchar(100) DEFAULT '' NOT NULL,
	forum_last_post_time INT4 DEFAULT '0' NOT NULL CHECK (forum_last_post_time >= 0),
	forum_last_poster_name varchar(255) DEFAULT '' NOT NULL,
	forum_last_poster_colour varchar(6) DEFAULT '' NOT NULL,
	forum_flags INT2 DEFAULT '32' NOT NULL,
	display_subforum_list INT2 DEFAULT '1' NOT NULL CHECK (display_subforum_list >= 0),
	display_on_index INT2 DEFAULT '1' NOT NULL CHECK (display_on_index >= 0),
	enable_indexing INT2 DEFAULT '1' NOT NULL CHECK (enable_indexing >= 0),
	enable_icons INT2 DEFAULT '1' NOT NULL CHECK (enable_icons >= 0),
	enable_prune INT2 DEFAULT '0' NOT NULL CHECK (enable_prune >= 0),
	prune_next INT4 DEFAULT '0' NOT NULL CHECK (prune_next >= 0),
	prune_days INT4 DEFAULT '0' NOT NULL CHECK (prune_days >= 0),
	prune_viewed INT4 DEFAULT '0' NOT NULL CHECK (prune_viewed >= 0),
	prune_freq INT4 DEFAULT '0' NOT NULL CHECK (prune_freq >= 0),
	PRIMARY KEY (forum_id)
);

CREATE INDEX phpbb_forums_left_right_id ON phpbb_forums (left_id, right_id);
CREATE INDEX phpbb_forums_forum_lastpost_id ON phpbb_forums (forum_last_post_id);

/*
	Table: 'phpbb_forums_access'
*/
CREATE TABLE phpbb_forums_access (
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	session_id char(32) DEFAULT '' NOT NULL,
	PRIMARY KEY (forum_id, user_id, session_id)
);


/*
	Table: 'phpbb_forums_track'
*/
CREATE TABLE phpbb_forums_track (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	mark_time INT4 DEFAULT '0' NOT NULL CHECK (mark_time >= 0),
	PRIMARY KEY (user_id, forum_id)
);


/*
	Table: 'phpbb_forums_watch'
*/
CREATE TABLE phpbb_forums_watch (
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	notify_status INT2 DEFAULT '0' NOT NULL CHECK (notify_status >= 0)
);

CREATE INDEX phpbb_forums_watch_forum_id ON phpbb_forums_watch (forum_id);
CREATE INDEX phpbb_forums_watch_user_id ON phpbb_forums_watch (user_id);
CREATE INDEX phpbb_forums_watch_notify_stat ON phpbb_forums_watch (notify_status);

/*
	Table: 'phpbb_groups'
*/
CREATE SEQUENCE phpbb_groups_seq;

CREATE TABLE phpbb_groups (
	group_id INT4 DEFAULT nextval('phpbb_groups_seq'),
	group_type INT2 DEFAULT '1' NOT NULL,
	group_founder_manage INT2 DEFAULT '0' NOT NULL CHECK (group_founder_manage >= 0),
	group_name varchar_ci DEFAULT '' NOT NULL,
	group_desc varchar(4000) DEFAULT '' NOT NULL,
	group_desc_bitfield varchar(255) DEFAULT '' NOT NULL,
	group_desc_options INT4 DEFAULT '7' NOT NULL CHECK (group_desc_options >= 0),
	group_desc_uid varchar(8) DEFAULT '' NOT NULL,
	group_display INT2 DEFAULT '0' NOT NULL CHECK (group_display >= 0),
	group_avatar varchar(255) DEFAULT '' NOT NULL,
	group_avatar_type INT2 DEFAULT '0' NOT NULL,
	group_avatar_width INT2 DEFAULT '0' NOT NULL CHECK (group_avatar_width >= 0),
	group_avatar_height INT2 DEFAULT '0' NOT NULL CHECK (group_avatar_height >= 0),
	group_rank INT4 DEFAULT '0' NOT NULL CHECK (group_rank >= 0),
	group_colour varchar(6) DEFAULT '' NOT NULL,
	group_sig_chars INT4 DEFAULT '0' NOT NULL CHECK (group_sig_chars >= 0),
	group_receive_pm INT2 DEFAULT '0' NOT NULL CHECK (group_receive_pm >= 0),
	group_message_limit INT4 DEFAULT '0' NOT NULL CHECK (group_message_limit >= 0),
	group_legend INT2 DEFAULT '1' NOT NULL CHECK (group_legend >= 0),
	PRIMARY KEY (group_id)
);

CREATE INDEX phpbb_groups_group_legend_name ON phpbb_groups (group_legend, group_name);

/*
	Table: 'phpbb_icons'
*/
CREATE SEQUENCE phpbb_icons_seq;

CREATE TABLE phpbb_icons (
	icons_id INT4 DEFAULT nextval('phpbb_icons_seq'),
	icons_url varchar(255) DEFAULT '' NOT NULL,
	icons_width INT2 DEFAULT '0' NOT NULL,
	icons_height INT2 DEFAULT '0' NOT NULL,
	icons_order INT4 DEFAULT '0' NOT NULL CHECK (icons_order >= 0),
	display_on_posting INT2 DEFAULT '1' NOT NULL CHECK (display_on_posting >= 0),
	PRIMARY KEY (icons_id)
);

CREATE INDEX phpbb_icons_display_on_posting ON phpbb_icons (display_on_posting);

/*
	Table: 'phpbb_lang'
*/
CREATE SEQUENCE phpbb_lang_seq;

CREATE TABLE phpbb_lang (
	lang_id INT2 DEFAULT nextval('phpbb_lang_seq'),
	lang_iso varchar(30) DEFAULT '' NOT NULL,
	lang_dir varchar(30) DEFAULT '' NOT NULL,
	lang_english_name varchar(100) DEFAULT '' NOT NULL,
	lang_local_name varchar(255) DEFAULT '' NOT NULL,
	lang_author varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (lang_id)
);

CREATE INDEX phpbb_lang_lang_iso ON phpbb_lang (lang_iso);

/*
	Table: 'phpbb_log'
*/
CREATE SEQUENCE phpbb_log_seq;

CREATE TABLE phpbb_log (
	log_id INT4 DEFAULT nextval('phpbb_log_seq'),
	log_type INT2 DEFAULT '0' NOT NULL,
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	reportee_id INT4 DEFAULT '0' NOT NULL CHECK (reportee_id >= 0),
	log_ip varchar(40) DEFAULT '' NOT NULL,
	log_time INT4 DEFAULT '0' NOT NULL CHECK (log_time >= 0),
	log_operation varchar(4000) DEFAULT '' NOT NULL,
	log_data TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (log_id)
);

CREATE INDEX phpbb_log_log_type ON phpbb_log (log_type);
CREATE INDEX phpbb_log_forum_id ON phpbb_log (forum_id);
CREATE INDEX phpbb_log_topic_id ON phpbb_log (topic_id);
CREATE INDEX phpbb_log_reportee_id ON phpbb_log (reportee_id);
CREATE INDEX phpbb_log_user_id ON phpbb_log (user_id);

/*
	Table: 'phpbb_moderator_cache'
*/
CREATE TABLE phpbb_moderator_cache (
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	username varchar(255) DEFAULT '' NOT NULL,
	group_id INT4 DEFAULT '0' NOT NULL CHECK (group_id >= 0),
	group_name varchar(255) DEFAULT '' NOT NULL,
	display_on_index INT2 DEFAULT '1' NOT NULL CHECK (display_on_index >= 0)
);

CREATE INDEX phpbb_moderator_cache_disp_idx ON phpbb_moderator_cache (display_on_index);
CREATE INDEX phpbb_moderator_cache_forum_id ON phpbb_moderator_cache (forum_id);

/*
	Table: 'phpbb_modules'
*/
CREATE SEQUENCE phpbb_modules_seq;

CREATE TABLE phpbb_modules (
	module_id INT4 DEFAULT nextval('phpbb_modules_seq'),
	module_enabled INT2 DEFAULT '1' NOT NULL CHECK (module_enabled >= 0),
	module_display INT2 DEFAULT '1' NOT NULL CHECK (module_display >= 0),
	module_basename varchar(255) DEFAULT '' NOT NULL,
	module_class varchar(10) DEFAULT '' NOT NULL,
	parent_id INT4 DEFAULT '0' NOT NULL CHECK (parent_id >= 0),
	left_id INT4 DEFAULT '0' NOT NULL CHECK (left_id >= 0),
	right_id INT4 DEFAULT '0' NOT NULL CHECK (right_id >= 0),
	module_langname varchar(255) DEFAULT '' NOT NULL,
	module_mode varchar(255) DEFAULT '' NOT NULL,
	module_auth varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (module_id)
);

CREATE INDEX phpbb_modules_left_right_id ON phpbb_modules (left_id, right_id);
CREATE INDEX phpbb_modules_module_enabled ON phpbb_modules (module_enabled);
CREATE INDEX phpbb_modules_class_left_id ON phpbb_modules (module_class, left_id);

/*
	Table: 'phpbb_poll_options'
*/
CREATE TABLE phpbb_poll_options (
	poll_option_id INT2 DEFAULT '0' NOT NULL,
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	poll_option_text varchar(4000) DEFAULT '' NOT NULL,
	poll_option_total INT4 DEFAULT '0' NOT NULL CHECK (poll_option_total >= 0)
);

CREATE INDEX phpbb_poll_options_poll_opt_id ON phpbb_poll_options (poll_option_id);
CREATE INDEX phpbb_poll_options_topic_id ON phpbb_poll_options (topic_id);

/*
	Table: 'phpbb_poll_votes'
*/
CREATE TABLE phpbb_poll_votes (
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	poll_option_id INT2 DEFAULT '0' NOT NULL,
	vote_user_id INT4 DEFAULT '0' NOT NULL CHECK (vote_user_id >= 0),
	vote_user_ip varchar(40) DEFAULT '' NOT NULL
);

CREATE INDEX phpbb_poll_votes_topic_id ON phpbb_poll_votes (topic_id);
CREATE INDEX phpbb_poll_votes_vote_user_id ON phpbb_poll_votes (vote_user_id);
CREATE INDEX phpbb_poll_votes_vote_user_ip ON phpbb_poll_votes (vote_user_ip);

/*
	Table: 'phpbb_posts'
*/
CREATE SEQUENCE phpbb_posts_seq;

CREATE TABLE phpbb_posts (
	post_id INT4 DEFAULT nextval('phpbb_posts_seq'),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	poster_id INT4 DEFAULT '0' NOT NULL CHECK (poster_id >= 0),
	icon_id INT4 DEFAULT '0' NOT NULL CHECK (icon_id >= 0),
	poster_ip varchar(40) DEFAULT '' NOT NULL,
	post_time INT4 DEFAULT '0' NOT NULL CHECK (post_time >= 0),
	post_approved INT2 DEFAULT '1' NOT NULL CHECK (post_approved >= 0),
	post_reported INT2 DEFAULT '0' NOT NULL CHECK (post_reported >= 0),
	enable_bbcode INT2 DEFAULT '1' NOT NULL CHECK (enable_bbcode >= 0),
	enable_smilies INT2 DEFAULT '1' NOT NULL CHECK (enable_smilies >= 0),
	enable_magic_url INT2 DEFAULT '1' NOT NULL CHECK (enable_magic_url >= 0),
	enable_sig INT2 DEFAULT '1' NOT NULL CHECK (enable_sig >= 0),
	post_username varchar(255) DEFAULT '' NOT NULL,
	post_subject varchar(100) DEFAULT '' NOT NULL,
	post_text TEXT DEFAULT '' NOT NULL,
	post_checksum varchar(32) DEFAULT '' NOT NULL,
	post_attachment INT2 DEFAULT '0' NOT NULL CHECK (post_attachment >= 0),
	bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	post_postcount INT2 DEFAULT '1' NOT NULL CHECK (post_postcount >= 0),
	post_edit_time INT4 DEFAULT '0' NOT NULL CHECK (post_edit_time >= 0),
	post_edit_reason varchar(255) DEFAULT '' NOT NULL,
	post_edit_user INT4 DEFAULT '0' NOT NULL CHECK (post_edit_user >= 0),
	post_edit_count INT2 DEFAULT '0' NOT NULL CHECK (post_edit_count >= 0),
	post_edit_locked INT2 DEFAULT '0' NOT NULL CHECK (post_edit_locked >= 0),
	PRIMARY KEY (post_id)
);

CREATE INDEX phpbb_posts_forum_id ON phpbb_posts (forum_id);
CREATE INDEX phpbb_posts_topic_id ON phpbb_posts (topic_id);
CREATE INDEX phpbb_posts_poster_ip ON phpbb_posts (poster_ip);
CREATE INDEX phpbb_posts_poster_id ON phpbb_posts (poster_id);
CREATE INDEX phpbb_posts_post_approved ON phpbb_posts (post_approved);
CREATE INDEX phpbb_posts_tid_post_time ON phpbb_posts (topic_id, post_time);

/*
	Table: 'phpbb_privmsgs'
*/
CREATE SEQUENCE phpbb_privmsgs_seq;

CREATE TABLE phpbb_privmsgs (
	msg_id INT4 DEFAULT nextval('phpbb_privmsgs_seq'),
	root_level INT4 DEFAULT '0' NOT NULL CHECK (root_level >= 0),
	author_id INT4 DEFAULT '0' NOT NULL CHECK (author_id >= 0),
	icon_id INT4 DEFAULT '0' NOT NULL CHECK (icon_id >= 0),
	author_ip varchar(40) DEFAULT '' NOT NULL,
	message_time INT4 DEFAULT '0' NOT NULL CHECK (message_time >= 0),
	enable_bbcode INT2 DEFAULT '1' NOT NULL CHECK (enable_bbcode >= 0),
	enable_smilies INT2 DEFAULT '1' NOT NULL CHECK (enable_smilies >= 0),
	enable_magic_url INT2 DEFAULT '1' NOT NULL CHECK (enable_magic_url >= 0),
	enable_sig INT2 DEFAULT '1' NOT NULL CHECK (enable_sig >= 0),
	message_subject varchar(100) DEFAULT '' NOT NULL,
	message_text TEXT DEFAULT '' NOT NULL,
	message_edit_reason varchar(255) DEFAULT '' NOT NULL,
	message_edit_user INT4 DEFAULT '0' NOT NULL CHECK (message_edit_user >= 0),
	message_attachment INT2 DEFAULT '0' NOT NULL CHECK (message_attachment >= 0),
	bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	message_edit_time INT4 DEFAULT '0' NOT NULL CHECK (message_edit_time >= 0),
	message_edit_count INT2 DEFAULT '0' NOT NULL CHECK (message_edit_count >= 0),
	to_address varchar(4000) DEFAULT '' NOT NULL,
	bcc_address varchar(4000) DEFAULT '' NOT NULL,
	PRIMARY KEY (msg_id)
);

CREATE INDEX phpbb_privmsgs_author_ip ON phpbb_privmsgs (author_ip);
CREATE INDEX phpbb_privmsgs_message_time ON phpbb_privmsgs (message_time);
CREATE INDEX phpbb_privmsgs_author_id ON phpbb_privmsgs (author_id);
CREATE INDEX phpbb_privmsgs_root_level ON phpbb_privmsgs (root_level);

/*
	Table: 'phpbb_privmsgs_folder'
*/
CREATE SEQUENCE phpbb_privmsgs_folder_seq;

CREATE TABLE phpbb_privmsgs_folder (
	folder_id INT4 DEFAULT nextval('phpbb_privmsgs_folder_seq'),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	folder_name varchar(255) DEFAULT '' NOT NULL,
	pm_count INT4 DEFAULT '0' NOT NULL CHECK (pm_count >= 0),
	PRIMARY KEY (folder_id)
);

CREATE INDEX phpbb_privmsgs_folder_user_id ON phpbb_privmsgs_folder (user_id);

/*
	Table: 'phpbb_privmsgs_rules'
*/
CREATE SEQUENCE phpbb_privmsgs_rules_seq;

CREATE TABLE phpbb_privmsgs_rules (
	rule_id INT4 DEFAULT nextval('phpbb_privmsgs_rules_seq'),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	rule_check INT4 DEFAULT '0' NOT NULL CHECK (rule_check >= 0),
	rule_connection INT4 DEFAULT '0' NOT NULL CHECK (rule_connection >= 0),
	rule_string varchar(255) DEFAULT '' NOT NULL,
	rule_user_id INT4 DEFAULT '0' NOT NULL CHECK (rule_user_id >= 0),
	rule_group_id INT4 DEFAULT '0' NOT NULL CHECK (rule_group_id >= 0),
	rule_action INT4 DEFAULT '0' NOT NULL CHECK (rule_action >= 0),
	rule_folder_id INT4 DEFAULT '0' NOT NULL,
	PRIMARY KEY (rule_id)
);

CREATE INDEX phpbb_privmsgs_rules_user_id ON phpbb_privmsgs_rules (user_id);

/*
	Table: 'phpbb_privmsgs_to'
*/
CREATE TABLE phpbb_privmsgs_to (
	msg_id INT4 DEFAULT '0' NOT NULL CHECK (msg_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	author_id INT4 DEFAULT '0' NOT NULL CHECK (author_id >= 0),
	pm_deleted INT2 DEFAULT '0' NOT NULL CHECK (pm_deleted >= 0),
	pm_new INT2 DEFAULT '1' NOT NULL CHECK (pm_new >= 0),
	pm_unread INT2 DEFAULT '1' NOT NULL CHECK (pm_unread >= 0),
	pm_replied INT2 DEFAULT '0' NOT NULL CHECK (pm_replied >= 0),
	pm_marked INT2 DEFAULT '0' NOT NULL CHECK (pm_marked >= 0),
	pm_forwarded INT2 DEFAULT '0' NOT NULL CHECK (pm_forwarded >= 0),
	folder_id INT4 DEFAULT '0' NOT NULL
);

CREATE INDEX phpbb_privmsgs_to_msg_id ON phpbb_privmsgs_to (msg_id);
CREATE INDEX phpbb_privmsgs_to_author_id ON phpbb_privmsgs_to (author_id);
CREATE INDEX phpbb_privmsgs_to_usr_flder_id ON phpbb_privmsgs_to (user_id, folder_id);

/*
	Table: 'phpbb_profile_fields'
*/
CREATE SEQUENCE phpbb_profile_fields_seq;

CREATE TABLE phpbb_profile_fields (
	field_id INT4 DEFAULT nextval('phpbb_profile_fields_seq'),
	field_name varchar(255) DEFAULT '' NOT NULL,
	field_type INT2 DEFAULT '0' NOT NULL,
	field_ident varchar(20) DEFAULT '' NOT NULL,
	field_length varchar(20) DEFAULT '' NOT NULL,
	field_minlen varchar(255) DEFAULT '' NOT NULL,
	field_maxlen varchar(255) DEFAULT '' NOT NULL,
	field_novalue varchar(255) DEFAULT '' NOT NULL,
	field_default_value varchar(255) DEFAULT '' NOT NULL,
	field_validation varchar(20) DEFAULT '' NOT NULL,
	field_required INT2 DEFAULT '0' NOT NULL CHECK (field_required >= 0),
	field_show_on_reg INT2 DEFAULT '0' NOT NULL CHECK (field_show_on_reg >= 0),
	field_hide INT2 DEFAULT '0' NOT NULL CHECK (field_hide >= 0),
	field_no_view INT2 DEFAULT '0' NOT NULL CHECK (field_no_view >= 0),
	field_active INT2 DEFAULT '0' NOT NULL CHECK (field_active >= 0),
	field_order INT4 DEFAULT '0' NOT NULL CHECK (field_order >= 0),
	PRIMARY KEY (field_id)
);

CREATE INDEX phpbb_profile_fields_fld_type ON phpbb_profile_fields (field_type);
CREATE INDEX phpbb_profile_fields_fld_ordr ON phpbb_profile_fields (field_order);

/*
	Table: 'phpbb_profile_fields_data'
*/
CREATE TABLE phpbb_profile_fields_data (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	PRIMARY KEY (user_id)
);


/*
	Table: 'phpbb_profile_fields_lang'
*/
CREATE TABLE phpbb_profile_fields_lang (
	field_id INT4 DEFAULT '0' NOT NULL CHECK (field_id >= 0),
	lang_id INT4 DEFAULT '0' NOT NULL CHECK (lang_id >= 0),
	option_id INT4 DEFAULT '0' NOT NULL CHECK (option_id >= 0),
	field_type INT2 DEFAULT '0' NOT NULL,
	lang_value varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (field_id, lang_id, option_id)
);


/*
	Table: 'phpbb_profile_lang'
*/
CREATE TABLE phpbb_profile_lang (
	field_id INT4 DEFAULT '0' NOT NULL CHECK (field_id >= 0),
	lang_id INT4 DEFAULT '0' NOT NULL CHECK (lang_id >= 0),
	lang_name varchar(255) DEFAULT '' NOT NULL,
	lang_explain varchar(4000) DEFAULT '' NOT NULL,
	lang_default_value varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (field_id, lang_id)
);


/*
	Table: 'phpbb_ranks'
*/
CREATE SEQUENCE phpbb_ranks_seq;

CREATE TABLE phpbb_ranks (
	rank_id INT4 DEFAULT nextval('phpbb_ranks_seq'),
	rank_title varchar(255) DEFAULT '' NOT NULL,
	rank_min INT4 DEFAULT '0' NOT NULL CHECK (rank_min >= 0),
	rank_special INT2 DEFAULT '0' NOT NULL CHECK (rank_special >= 0),
	rank_image varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (rank_id)
);


/*
	Table: 'phpbb_reports'
*/
CREATE SEQUENCE phpbb_reports_seq;

CREATE TABLE phpbb_reports (
	report_id INT4 DEFAULT nextval('phpbb_reports_seq'),
	reason_id INT2 DEFAULT '0' NOT NULL CHECK (reason_id >= 0),
	post_id INT4 DEFAULT '0' NOT NULL CHECK (post_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	user_notify INT2 DEFAULT '0' NOT NULL CHECK (user_notify >= 0),
	report_closed INT2 DEFAULT '0' NOT NULL CHECK (report_closed >= 0),
	report_time INT4 DEFAULT '0' NOT NULL CHECK (report_time >= 0),
	report_text TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (report_id)
);


/*
	Table: 'phpbb_reports_reasons'
*/
CREATE SEQUENCE phpbb_reports_reasons_seq;

CREATE TABLE phpbb_reports_reasons (
	reason_id INT2 DEFAULT nextval('phpbb_reports_reasons_seq'),
	reason_title varchar(255) DEFAULT '' NOT NULL,
	reason_description TEXT DEFAULT '' NOT NULL,
	reason_order INT2 DEFAULT '0' NOT NULL CHECK (reason_order >= 0),
	PRIMARY KEY (reason_id)
);


/*
	Table: 'phpbb_search_results'
*/
CREATE TABLE phpbb_search_results (
	search_key varchar(32) DEFAULT '' NOT NULL,
	search_time INT4 DEFAULT '0' NOT NULL CHECK (search_time >= 0),
	search_keywords TEXT DEFAULT '' NOT NULL,
	search_authors TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (search_key)
);


/*
	Table: 'phpbb_search_wordlist'
*/
CREATE SEQUENCE phpbb_search_wordlist_seq;

CREATE TABLE phpbb_search_wordlist (
	word_id INT4 DEFAULT nextval('phpbb_search_wordlist_seq'),
	word_text varchar(255) DEFAULT '' NOT NULL,
	word_common INT2 DEFAULT '0' NOT NULL CHECK (word_common >= 0),
	word_count INT4 DEFAULT '0' NOT NULL CHECK (word_count >= 0),
	PRIMARY KEY (word_id)
);

CREATE UNIQUE INDEX phpbb_search_wordlist_wrd_txt ON phpbb_search_wordlist (word_text);
CREATE INDEX phpbb_search_wordlist_wrd_cnt ON phpbb_search_wordlist (word_count);

/*
	Table: 'phpbb_search_wordmatch'
*/
CREATE TABLE phpbb_search_wordmatch (
	post_id INT4 DEFAULT '0' NOT NULL CHECK (post_id >= 0),
	word_id INT4 DEFAULT '0' NOT NULL CHECK (word_id >= 0),
	title_match INT2 DEFAULT '0' NOT NULL CHECK (title_match >= 0)
);

CREATE UNIQUE INDEX phpbb_search_wordmatch_unq_mtch ON phpbb_search_wordmatch (word_id, post_id, title_match);
CREATE INDEX phpbb_search_wordmatch_word_id ON phpbb_search_wordmatch (word_id);
CREATE INDEX phpbb_search_wordmatch_post_id ON phpbb_search_wordmatch (post_id);

/*
	Table: 'phpbb_sessions'
*/
CREATE TABLE phpbb_sessions (
	session_id char(32) DEFAULT '' NOT NULL,
	session_user_id INT4 DEFAULT '0' NOT NULL CHECK (session_user_id >= 0),
	session_forum_id INT4 DEFAULT '0' NOT NULL CHECK (session_forum_id >= 0),
	session_last_visit INT4 DEFAULT '0' NOT NULL CHECK (session_last_visit >= 0),
	session_start INT4 DEFAULT '0' NOT NULL CHECK (session_start >= 0),
	session_time INT4 DEFAULT '0' NOT NULL CHECK (session_time >= 0),
	session_ip varchar(40) DEFAULT '' NOT NULL,
	session_browser varchar(150) DEFAULT '' NOT NULL,
	session_forwarded_for varchar(255) DEFAULT '' NOT NULL,
	session_page varchar(255) DEFAULT '' NOT NULL,
	session_viewonline INT2 DEFAULT '1' NOT NULL CHECK (session_viewonline >= 0),
	session_autologin INT2 DEFAULT '0' NOT NULL CHECK (session_autologin >= 0),
	session_admin INT2 DEFAULT '0' NOT NULL CHECK (session_admin >= 0),
	PRIMARY KEY (session_id)
);

CREATE INDEX phpbb_sessions_session_time ON phpbb_sessions (session_time);
CREATE INDEX phpbb_sessions_session_user_id ON phpbb_sessions (session_user_id);
CREATE INDEX phpbb_sessions_session_forum_id ON phpbb_sessions (session_forum_id);

/*
	Table: 'phpbb_sessions_keys'
*/
CREATE TABLE phpbb_sessions_keys (
	key_id char(32) DEFAULT '' NOT NULL,
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	last_ip varchar(40) DEFAULT '' NOT NULL,
	last_login INT4 DEFAULT '0' NOT NULL CHECK (last_login >= 0),
	PRIMARY KEY (key_id, user_id)
);

CREATE INDEX phpbb_sessions_keys_last_login ON phpbb_sessions_keys (last_login);

/*
	Table: 'phpbb_sitelist'
*/
CREATE SEQUENCE phpbb_sitelist_seq;

CREATE TABLE phpbb_sitelist (
	site_id INT4 DEFAULT nextval('phpbb_sitelist_seq'),
	site_ip varchar(40) DEFAULT '' NOT NULL,
	site_hostname varchar(255) DEFAULT '' NOT NULL,
	ip_exclude INT2 DEFAULT '0' NOT NULL CHECK (ip_exclude >= 0),
	PRIMARY KEY (site_id)
);


/*
	Table: 'phpbb_smilies'
*/
CREATE SEQUENCE phpbb_smilies_seq;

CREATE TABLE phpbb_smilies (
	smiley_id INT4 DEFAULT nextval('phpbb_smilies_seq'),
	code varchar(50) DEFAULT '' NOT NULL,
	emotion varchar(50) DEFAULT '' NOT NULL,
	smiley_url varchar(50) DEFAULT '' NOT NULL,
	smiley_width INT2 DEFAULT '0' NOT NULL CHECK (smiley_width >= 0),
	smiley_height INT2 DEFAULT '0' NOT NULL CHECK (smiley_height >= 0),
	smiley_order INT4 DEFAULT '0' NOT NULL CHECK (smiley_order >= 0),
	display_on_posting INT2 DEFAULT '1' NOT NULL CHECK (display_on_posting >= 0),
	PRIMARY KEY (smiley_id)
);

CREATE INDEX phpbb_smilies_display_on_post ON phpbb_smilies (display_on_posting);

/*
	Table: 'phpbb_styles'
*/
CREATE SEQUENCE phpbb_styles_seq;

CREATE TABLE phpbb_styles (
	style_id INT2 DEFAULT nextval('phpbb_styles_seq'),
	style_name varchar(255) DEFAULT '' NOT NULL,
	style_copyright varchar(255) DEFAULT '' NOT NULL,
	style_active INT2 DEFAULT '1' NOT NULL CHECK (style_active >= 0),
	template_id INT2 DEFAULT '0' NOT NULL CHECK (template_id >= 0),
	theme_id INT2 DEFAULT '0' NOT NULL CHECK (theme_id >= 0),
	imageset_id INT2 DEFAULT '0' NOT NULL CHECK (imageset_id >= 0),
	PRIMARY KEY (style_id)
);

CREATE UNIQUE INDEX phpbb_styles_style_name ON phpbb_styles (style_name);
CREATE INDEX phpbb_styles_template_id ON phpbb_styles (template_id);
CREATE INDEX phpbb_styles_theme_id ON phpbb_styles (theme_id);
CREATE INDEX phpbb_styles_imageset_id ON phpbb_styles (imageset_id);

/*
	Table: 'phpbb_styles_template'
*/
CREATE SEQUENCE phpbb_styles_template_seq;

CREATE TABLE phpbb_styles_template (
	template_id INT2 DEFAULT nextval('phpbb_styles_template_seq'),
	template_name varchar(255) DEFAULT '' NOT NULL,
	template_copyright varchar(255) DEFAULT '' NOT NULL,
	template_path varchar(100) DEFAULT '' NOT NULL,
	bbcode_bitfield varchar(255) DEFAULT 'kNg=' NOT NULL,
	template_storedb INT2 DEFAULT '0' NOT NULL CHECK (template_storedb >= 0),
	PRIMARY KEY (template_id)
);

CREATE UNIQUE INDEX phpbb_styles_template_tmplte_nm ON phpbb_styles_template (template_name);

/*
	Table: 'phpbb_styles_template_data'
*/
CREATE TABLE phpbb_styles_template_data (
	template_id INT2 DEFAULT '0' NOT NULL CHECK (template_id >= 0),
	template_filename varchar(100) DEFAULT '' NOT NULL,
	template_included varchar(8000) DEFAULT '' NOT NULL,
	template_mtime INT4 DEFAULT '0' NOT NULL CHECK (template_mtime >= 0),
	template_data TEXT DEFAULT '' NOT NULL
);

CREATE INDEX phpbb_styles_template_data_tid ON phpbb_styles_template_data (template_id);
CREATE INDEX phpbb_styles_template_data_tfn ON phpbb_styles_template_data (template_filename);

/*
	Table: 'phpbb_styles_theme'
*/
CREATE SEQUENCE phpbb_styles_theme_seq;

CREATE TABLE phpbb_styles_theme (
	theme_id INT2 DEFAULT nextval('phpbb_styles_theme_seq'),
	theme_name varchar(255) DEFAULT '' NOT NULL,
	theme_copyright varchar(255) DEFAULT '' NOT NULL,
	theme_path varchar(100) DEFAULT '' NOT NULL,
	theme_storedb INT2 DEFAULT '0' NOT NULL CHECK (theme_storedb >= 0),
	theme_mtime INT4 DEFAULT '0' NOT NULL CHECK (theme_mtime >= 0),
	theme_data TEXT DEFAULT '' NOT NULL,
	PRIMARY KEY (theme_id)
);

CREATE UNIQUE INDEX phpbb_styles_theme_theme_name ON phpbb_styles_theme (theme_name);

/*
	Table: 'phpbb_styles_imageset'
*/
CREATE SEQUENCE phpbb_styles_imageset_seq;

CREATE TABLE phpbb_styles_imageset (
	imageset_id INT2 DEFAULT nextval('phpbb_styles_imageset_seq'),
	imageset_name varchar(255) DEFAULT '' NOT NULL,
	imageset_copyright varchar(255) DEFAULT '' NOT NULL,
	imageset_path varchar(100) DEFAULT '' NOT NULL,
	PRIMARY KEY (imageset_id)
);

CREATE UNIQUE INDEX phpbb_styles_imageset_imgset_nm ON phpbb_styles_imageset (imageset_name);

/*
	Table: 'phpbb_styles_imageset_data'
*/
CREATE SEQUENCE phpbb_styles_imageset_data_seq;

CREATE TABLE phpbb_styles_imageset_data (
	image_id INT2 DEFAULT nextval('phpbb_styles_imageset_data_seq'),
	image_name varchar(200) DEFAULT '' NOT NULL,
	image_filename varchar(200) DEFAULT '' NOT NULL,
	image_lang varchar(30) DEFAULT '' NOT NULL,
	image_height INT2 DEFAULT '0' NOT NULL CHECK (image_height >= 0),
	image_width INT2 DEFAULT '0' NOT NULL CHECK (image_width >= 0),
	imageset_id INT2 DEFAULT '0' NOT NULL CHECK (imageset_id >= 0),
	PRIMARY KEY (image_id)
);

CREATE INDEX phpbb_styles_imageset_data_i_d ON phpbb_styles_imageset_data (imageset_id);

/*
	Table: 'phpbb_topics'
*/
CREATE SEQUENCE phpbb_topics_seq;

CREATE TABLE phpbb_topics (
	topic_id INT4 DEFAULT nextval('phpbb_topics_seq'),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	icon_id INT4 DEFAULT '0' NOT NULL CHECK (icon_id >= 0),
	topic_attachment INT2 DEFAULT '0' NOT NULL CHECK (topic_attachment >= 0),
	topic_approved INT2 DEFAULT '1' NOT NULL CHECK (topic_approved >= 0),
	topic_reported INT2 DEFAULT '0' NOT NULL CHECK (topic_reported >= 0),
	topic_title varchar(100) DEFAULT '' NOT NULL,
	topic_poster INT4 DEFAULT '0' NOT NULL CHECK (topic_poster >= 0),
	topic_time INT4 DEFAULT '0' NOT NULL CHECK (topic_time >= 0),
	topic_time_limit INT4 DEFAULT '0' NOT NULL CHECK (topic_time_limit >= 0),
	topic_views INT4 DEFAULT '0' NOT NULL CHECK (topic_views >= 0),
	topic_replies INT4 DEFAULT '0' NOT NULL CHECK (topic_replies >= 0),
	topic_replies_real INT4 DEFAULT '0' NOT NULL CHECK (topic_replies_real >= 0),
	topic_status INT2 DEFAULT '0' NOT NULL,
	topic_type INT2 DEFAULT '0' NOT NULL,
	topic_first_post_id INT4 DEFAULT '0' NOT NULL CHECK (topic_first_post_id >= 0),
	topic_first_poster_name varchar(255) DEFAULT '' NOT NULL,
	topic_first_poster_colour varchar(6) DEFAULT '' NOT NULL,
	topic_last_post_id INT4 DEFAULT '0' NOT NULL CHECK (topic_last_post_id >= 0),
	topic_last_poster_id INT4 DEFAULT '0' NOT NULL CHECK (topic_last_poster_id >= 0),
	topic_last_poster_name varchar(255) DEFAULT '' NOT NULL,
	topic_last_poster_colour varchar(6) DEFAULT '' NOT NULL,
	topic_last_post_subject varchar(100) DEFAULT '' NOT NULL,
	topic_last_post_time INT4 DEFAULT '0' NOT NULL CHECK (topic_last_post_time >= 0),
	topic_last_view_time INT4 DEFAULT '0' NOT NULL CHECK (topic_last_view_time >= 0),
	topic_moved_id INT4 DEFAULT '0' NOT NULL CHECK (topic_moved_id >= 0),
	topic_bumped INT2 DEFAULT '0' NOT NULL CHECK (topic_bumped >= 0),
	topic_bumper INT4 DEFAULT '0' NOT NULL CHECK (topic_bumper >= 0),
	poll_title varchar(255) DEFAULT '' NOT NULL,
	poll_start INT4 DEFAULT '0' NOT NULL CHECK (poll_start >= 0),
	poll_length INT4 DEFAULT '0' NOT NULL CHECK (poll_length >= 0),
	poll_max_options INT2 DEFAULT '1' NOT NULL,
	poll_last_vote INT4 DEFAULT '0' NOT NULL CHECK (poll_last_vote >= 0),
	poll_vote_change INT2 DEFAULT '0' NOT NULL CHECK (poll_vote_change >= 0),
	PRIMARY KEY (topic_id)
);

CREATE INDEX phpbb_topics_forum_id ON phpbb_topics (forum_id);
CREATE INDEX phpbb_topics_forum_id_type ON phpbb_topics (forum_id, topic_type);
CREATE INDEX phpbb_topics_last_post_time ON phpbb_topics (topic_last_post_time);
CREATE INDEX phpbb_topics_topic_approved ON phpbb_topics (topic_approved);
CREATE INDEX phpbb_topics_forum_appr_last ON phpbb_topics (forum_id, topic_approved, topic_last_post_id);
CREATE INDEX phpbb_topics_fid_time_moved ON phpbb_topics (forum_id, topic_last_post_time, topic_moved_id);

/*
	Table: 'phpbb_topics_track'
*/
CREATE TABLE phpbb_topics_track (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	forum_id INT4 DEFAULT '0' NOT NULL CHECK (forum_id >= 0),
	mark_time INT4 DEFAULT '0' NOT NULL CHECK (mark_time >= 0),
	PRIMARY KEY (user_id, topic_id)
);

CREATE INDEX phpbb_topics_track_forum_id ON phpbb_topics_track (forum_id);

/*
	Table: 'phpbb_topics_posted'
*/
CREATE TABLE phpbb_topics_posted (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	topic_posted INT2 DEFAULT '0' NOT NULL CHECK (topic_posted >= 0),
	PRIMARY KEY (user_id, topic_id)
);


/*
	Table: 'phpbb_topics_watch'
*/
CREATE TABLE phpbb_topics_watch (
	topic_id INT4 DEFAULT '0' NOT NULL CHECK (topic_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	notify_status INT2 DEFAULT '0' NOT NULL CHECK (notify_status >= 0)
);

CREATE INDEX phpbb_topics_watch_topic_id ON phpbb_topics_watch (topic_id);
CREATE INDEX phpbb_topics_watch_user_id ON phpbb_topics_watch (user_id);
CREATE INDEX phpbb_topics_watch_notify_stat ON phpbb_topics_watch (notify_status);

/*
	Table: 'phpbb_user_group'
*/
CREATE TABLE phpbb_user_group (
	group_id INT4 DEFAULT '0' NOT NULL CHECK (group_id >= 0),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	group_leader INT2 DEFAULT '0' NOT NULL CHECK (group_leader >= 0),
	user_pending INT2 DEFAULT '1' NOT NULL CHECK (user_pending >= 0)
);

CREATE INDEX phpbb_user_group_group_id ON phpbb_user_group (group_id);
CREATE INDEX phpbb_user_group_user_id ON phpbb_user_group (user_id);
CREATE INDEX phpbb_user_group_group_leader ON phpbb_user_group (group_leader);

/*
	Table: 'phpbb_users'
*/
CREATE SEQUENCE phpbb_users_seq;

CREATE TABLE phpbb_users (
	user_id INT4 DEFAULT nextval('phpbb_users_seq'),
	user_type INT2 DEFAULT '0' NOT NULL,
	group_id INT4 DEFAULT '3' NOT NULL CHECK (group_id >= 0),
	user_permissions TEXT DEFAULT '' NOT NULL,
	user_perm_from INT4 DEFAULT '0' NOT NULL CHECK (user_perm_from >= 0),
	user_ip varchar(40) DEFAULT '' NOT NULL,
	user_regdate INT4 DEFAULT '0' NOT NULL CHECK (user_regdate >= 0),
	username varchar_ci DEFAULT '' NOT NULL,
	username_clean varchar_ci DEFAULT '' NOT NULL,
	user_password varchar(40) DEFAULT '' NOT NULL,
	user_passchg INT4 DEFAULT '0' NOT NULL CHECK (user_passchg >= 0),
	user_pass_convert INT2 DEFAULT '0' NOT NULL CHECK (user_pass_convert >= 0),
	user_email varchar(100) DEFAULT '' NOT NULL,
	user_email_hash INT8 DEFAULT '0' NOT NULL,
	user_birthday varchar(10) DEFAULT '' NOT NULL,
	user_lastvisit INT4 DEFAULT '0' NOT NULL CHECK (user_lastvisit >= 0),
	user_lastmark INT4 DEFAULT '0' NOT NULL CHECK (user_lastmark >= 0),
	user_lastpost_time INT4 DEFAULT '0' NOT NULL CHECK (user_lastpost_time >= 0),
	user_lastpage varchar(200) DEFAULT '' NOT NULL,
	user_last_confirm_key varchar(10) DEFAULT '' NOT NULL,
	user_last_search INT4 DEFAULT '0' NOT NULL CHECK (user_last_search >= 0),
	user_warnings INT2 DEFAULT '0' NOT NULL,
	user_last_warning INT4 DEFAULT '0' NOT NULL CHECK (user_last_warning >= 0),
	user_login_attempts INT2 DEFAULT '0' NOT NULL,
	user_inactive_reason INT2 DEFAULT '0' NOT NULL,
	user_inactive_time INT4 DEFAULT '0' NOT NULL CHECK (user_inactive_time >= 0),
	user_posts INT4 DEFAULT '0' NOT NULL CHECK (user_posts >= 0),
	user_lang varchar(30) DEFAULT '' NOT NULL,
	user_timezone decimal(5,2) DEFAULT '0' NOT NULL,
	user_dst INT2 DEFAULT '0' NOT NULL CHECK (user_dst >= 0),
	user_dateformat varchar(30) DEFAULT 'd M Y H:i' NOT NULL,
	user_style INT2 DEFAULT '0' NOT NULL CHECK (user_style >= 0),
	user_rank INT4 DEFAULT '0' NOT NULL CHECK (user_rank >= 0),
	user_colour varchar(6) DEFAULT '' NOT NULL,
	user_new_privmsg INT4 DEFAULT '0' NOT NULL,
	user_unread_privmsg INT4 DEFAULT '0' NOT NULL,
	user_last_privmsg INT4 DEFAULT '0' NOT NULL CHECK (user_last_privmsg >= 0),
	user_message_rules INT2 DEFAULT '0' NOT NULL CHECK (user_message_rules >= 0),
	user_full_folder INT4 DEFAULT '-3' NOT NULL,
	user_emailtime INT4 DEFAULT '0' NOT NULL CHECK (user_emailtime >= 0),
	user_topic_show_days INT2 DEFAULT '0' NOT NULL CHECK (user_topic_show_days >= 0),
	user_topic_sortby_type varchar(1) DEFAULT 't' NOT NULL,
	user_topic_sortby_dir varchar(1) DEFAULT 'd' NOT NULL,
	user_post_show_days INT2 DEFAULT '0' NOT NULL CHECK (user_post_show_days >= 0),
	user_post_sortby_type varchar(1) DEFAULT 't' NOT NULL,
	user_post_sortby_dir varchar(1) DEFAULT 'a' NOT NULL,
	user_notify INT2 DEFAULT '0' NOT NULL CHECK (user_notify >= 0),
	user_notify_pm INT2 DEFAULT '1' NOT NULL CHECK (user_notify_pm >= 0),
	user_notify_type INT2 DEFAULT '0' NOT NULL,
	user_allow_pm INT2 DEFAULT '1' NOT NULL CHECK (user_allow_pm >= 0),
	user_allow_viewonline INT2 DEFAULT '1' NOT NULL CHECK (user_allow_viewonline >= 0),
	user_allow_viewemail INT2 DEFAULT '1' NOT NULL CHECK (user_allow_viewemail >= 0),
	user_allow_massemail INT2 DEFAULT '1' NOT NULL CHECK (user_allow_massemail >= 0),
	user_options INT4 DEFAULT '895' NOT NULL CHECK (user_options >= 0),
	user_avatar varchar(255) DEFAULT '' NOT NULL,
	user_avatar_type INT2 DEFAULT '0' NOT NULL,
	user_avatar_width INT2 DEFAULT '0' NOT NULL CHECK (user_avatar_width >= 0),
	user_avatar_height INT2 DEFAULT '0' NOT NULL CHECK (user_avatar_height >= 0),
	user_sig TEXT DEFAULT '' NOT NULL,
	user_sig_bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	user_sig_bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	user_from varchar(100) DEFAULT '' NOT NULL,
	user_icq varchar(15) DEFAULT '' NOT NULL,
	user_aim varchar(255) DEFAULT '' NOT NULL,
	user_yim varchar(255) DEFAULT '' NOT NULL,
	user_msnm varchar(255) DEFAULT '' NOT NULL,
	user_jabber varchar(255) DEFAULT '' NOT NULL,
	user_website varchar(200) DEFAULT '' NOT NULL,
	user_occ varchar(4000) DEFAULT '' NOT NULL,
	user_interests varchar(4000) DEFAULT '' NOT NULL,
	user_actkey varchar(32) DEFAULT '' NOT NULL,
	user_newpasswd varchar(40) DEFAULT '' NOT NULL,
	user_form_salt varchar(32) DEFAULT '' NOT NULL,
	PRIMARY KEY (user_id)
);

CREATE INDEX phpbb_users_user_birthday ON phpbb_users (user_birthday);
CREATE INDEX phpbb_users_user_email_hash ON phpbb_users (user_email_hash);
CREATE INDEX phpbb_users_user_type ON phpbb_users (user_type);
CREATE UNIQUE INDEX phpbb_users_username_clean ON phpbb_users (username_clean);

/*
	Table: 'phpbb_warnings'
*/
CREATE SEQUENCE phpbb_warnings_seq;

CREATE TABLE phpbb_warnings (
	warning_id INT4 DEFAULT nextval('phpbb_warnings_seq'),
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	post_id INT4 DEFAULT '0' NOT NULL CHECK (post_id >= 0),
	log_id INT4 DEFAULT '0' NOT NULL CHECK (log_id >= 0),
	warning_time INT4 DEFAULT '0' NOT NULL CHECK (warning_time >= 0),
	PRIMARY KEY (warning_id)
);


/*
	Table: 'phpbb_words'
*/
CREATE SEQUENCE phpbb_words_seq;

CREATE TABLE phpbb_words (
	word_id INT4 DEFAULT nextval('phpbb_words_seq'),
	word varchar(255) DEFAULT '' NOT NULL,
	replacement varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (word_id)
);


/*
	Table: 'phpbb_zebra'
*/
CREATE TABLE phpbb_zebra (
	user_id INT4 DEFAULT '0' NOT NULL CHECK (user_id >= 0),
	zebra_id INT4 DEFAULT '0' NOT NULL CHECK (zebra_id >= 0),
	friend INT2 DEFAULT '0' NOT NULL CHECK (friend >= 0),
	foe INT2 DEFAULT '0' NOT NULL CHECK (foe >= 0),
	PRIMARY KEY (user_id, zebra_id)
);



COMMIT;