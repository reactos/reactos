#
# $Id: sqlite_schema.sql 8456 2008-03-22 12:31:17Z Kellanved $
#

BEGIN TRANSACTION;

# Table: 'phpbb_attachments'
CREATE TABLE phpbb_attachments (
	attach_id INTEGER PRIMARY KEY NOT NULL ,
	post_msg_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	in_message INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poster_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	is_orphan INTEGER UNSIGNED NOT NULL DEFAULT '1',
	physical_filename varchar(255) NOT NULL DEFAULT '',
	real_filename varchar(255) NOT NULL DEFAULT '',
	download_count INTEGER UNSIGNED NOT NULL DEFAULT '0',
	attach_comment text(65535) NOT NULL DEFAULT '',
	extension varchar(100) NOT NULL DEFAULT '',
	mimetype varchar(100) NOT NULL DEFAULT '',
	filesize INTEGER UNSIGNED NOT NULL DEFAULT '0',
	filetime INTEGER UNSIGNED NOT NULL DEFAULT '0',
	thumbnail INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_attachments_filetime ON phpbb_attachments (filetime);
CREATE INDEX phpbb_attachments_post_msg_id ON phpbb_attachments (post_msg_id);
CREATE INDEX phpbb_attachments_topic_id ON phpbb_attachments (topic_id);
CREATE INDEX phpbb_attachments_poster_id ON phpbb_attachments (poster_id);
CREATE INDEX phpbb_attachments_is_orphan ON phpbb_attachments (is_orphan);

# Table: 'phpbb_acl_groups'
CREATE TABLE phpbb_acl_groups (
	group_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_option_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_role_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_setting tinyint(2) NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_acl_groups_group_id ON phpbb_acl_groups (group_id);
CREATE INDEX phpbb_acl_groups_auth_opt_id ON phpbb_acl_groups (auth_option_id);
CREATE INDEX phpbb_acl_groups_auth_role_id ON phpbb_acl_groups (auth_role_id);

# Table: 'phpbb_acl_options'
CREATE TABLE phpbb_acl_options (
	auth_option_id INTEGER PRIMARY KEY NOT NULL ,
	auth_option varchar(50) NOT NULL DEFAULT '',
	is_global INTEGER UNSIGNED NOT NULL DEFAULT '0',
	is_local INTEGER UNSIGNED NOT NULL DEFAULT '0',
	founder_only INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_acl_options_auth_option ON phpbb_acl_options (auth_option);

# Table: 'phpbb_acl_roles'
CREATE TABLE phpbb_acl_roles (
	role_id INTEGER PRIMARY KEY NOT NULL ,
	role_name varchar(255) NOT NULL DEFAULT '',
	role_description text(65535) NOT NULL DEFAULT '',
	role_type varchar(10) NOT NULL DEFAULT '',
	role_order INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_acl_roles_role_type ON phpbb_acl_roles (role_type);
CREATE INDEX phpbb_acl_roles_role_order ON phpbb_acl_roles (role_order);

# Table: 'phpbb_acl_roles_data'
CREATE TABLE phpbb_acl_roles_data (
	role_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_option_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_setting tinyint(2) NOT NULL DEFAULT '0',
	PRIMARY KEY (role_id, auth_option_id)
);

CREATE INDEX phpbb_acl_roles_data_ath_op_id ON phpbb_acl_roles_data (auth_option_id);

# Table: 'phpbb_acl_users'
CREATE TABLE phpbb_acl_users (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_option_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_role_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	auth_setting tinyint(2) NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_acl_users_user_id ON phpbb_acl_users (user_id);
CREATE INDEX phpbb_acl_users_auth_option_id ON phpbb_acl_users (auth_option_id);
CREATE INDEX phpbb_acl_users_auth_role_id ON phpbb_acl_users (auth_role_id);

# Table: 'phpbb_banlist'
CREATE TABLE phpbb_banlist (
	ban_id INTEGER PRIMARY KEY NOT NULL ,
	ban_userid INTEGER UNSIGNED NOT NULL DEFAULT '0',
	ban_ip varchar(40) NOT NULL DEFAULT '',
	ban_email varchar(100) NOT NULL DEFAULT '',
	ban_start INTEGER UNSIGNED NOT NULL DEFAULT '0',
	ban_end INTEGER UNSIGNED NOT NULL DEFAULT '0',
	ban_exclude INTEGER UNSIGNED NOT NULL DEFAULT '0',
	ban_reason varchar(255) NOT NULL DEFAULT '',
	ban_give_reason varchar(255) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_banlist_ban_end ON phpbb_banlist (ban_end);
CREATE INDEX phpbb_banlist_ban_user ON phpbb_banlist (ban_userid, ban_exclude);
CREATE INDEX phpbb_banlist_ban_email ON phpbb_banlist (ban_email, ban_exclude);
CREATE INDEX phpbb_banlist_ban_ip ON phpbb_banlist (ban_ip, ban_exclude);

# Table: 'phpbb_bbcodes'
CREATE TABLE phpbb_bbcodes (
	bbcode_id tinyint(3) NOT NULL DEFAULT '0',
	bbcode_tag varchar(16) NOT NULL DEFAULT '',
	bbcode_helpline varchar(255) NOT NULL DEFAULT '',
	display_on_posting INTEGER UNSIGNED NOT NULL DEFAULT '0',
	bbcode_match text(65535) NOT NULL DEFAULT '',
	bbcode_tpl mediumtext(16777215) NOT NULL DEFAULT '',
	first_pass_match mediumtext(16777215) NOT NULL DEFAULT '',
	first_pass_replace mediumtext(16777215) NOT NULL DEFAULT '',
	second_pass_match mediumtext(16777215) NOT NULL DEFAULT '',
	second_pass_replace mediumtext(16777215) NOT NULL DEFAULT '',
	PRIMARY KEY (bbcode_id)
);

CREATE INDEX phpbb_bbcodes_display_on_post ON phpbb_bbcodes (display_on_posting);

# Table: 'phpbb_bookmarks'
CREATE TABLE phpbb_bookmarks (
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (topic_id, user_id)
);


# Table: 'phpbb_bots'
CREATE TABLE phpbb_bots (
	bot_id INTEGER PRIMARY KEY NOT NULL ,
	bot_active INTEGER UNSIGNED NOT NULL DEFAULT '1',
	bot_name text(65535) NOT NULL DEFAULT '',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	bot_agent varchar(255) NOT NULL DEFAULT '',
	bot_ip varchar(255) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_bots_bot_active ON phpbb_bots (bot_active);

# Table: 'phpbb_config'
CREATE TABLE phpbb_config (
	config_name varchar(255) NOT NULL DEFAULT '',
	config_value varchar(255) NOT NULL DEFAULT '',
	is_dynamic INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (config_name)
);

CREATE INDEX phpbb_config_is_dynamic ON phpbb_config (is_dynamic);

# Table: 'phpbb_confirm'
CREATE TABLE phpbb_confirm (
	confirm_id char(32) NOT NULL DEFAULT '',
	session_id char(32) NOT NULL DEFAULT '',
	confirm_type tinyint(3) NOT NULL DEFAULT '0',
	code varchar(8) NOT NULL DEFAULT '',
	seed INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (session_id, confirm_id)
);

CREATE INDEX phpbb_confirm_confirm_type ON phpbb_confirm (confirm_type);

# Table: 'phpbb_disallow'
CREATE TABLE phpbb_disallow (
	disallow_id INTEGER PRIMARY KEY NOT NULL ,
	disallow_username varchar(255) NOT NULL DEFAULT ''
);


# Table: 'phpbb_drafts'
CREATE TABLE phpbb_drafts (
	draft_id INTEGER PRIMARY KEY NOT NULL ,
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	save_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	draft_subject text(65535) NOT NULL DEFAULT '',
	draft_message mediumtext(16777215) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_drafts_save_time ON phpbb_drafts (save_time);

# Table: 'phpbb_extensions'
CREATE TABLE phpbb_extensions (
	extension_id INTEGER PRIMARY KEY NOT NULL ,
	group_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	extension varchar(100) NOT NULL DEFAULT ''
);


# Table: 'phpbb_extension_groups'
CREATE TABLE phpbb_extension_groups (
	group_id INTEGER PRIMARY KEY NOT NULL ,
	group_name varchar(255) NOT NULL DEFAULT '',
	cat_id tinyint(2) NOT NULL DEFAULT '0',
	allow_group INTEGER UNSIGNED NOT NULL DEFAULT '0',
	download_mode INTEGER UNSIGNED NOT NULL DEFAULT '1',
	upload_icon varchar(255) NOT NULL DEFAULT '',
	max_filesize INTEGER UNSIGNED NOT NULL DEFAULT '0',
	allowed_forums text(65535) NOT NULL DEFAULT '',
	allow_in_pm INTEGER UNSIGNED NOT NULL DEFAULT '0'
);


# Table: 'phpbb_forums'
CREATE TABLE phpbb_forums (
	forum_id INTEGER PRIMARY KEY NOT NULL ,
	parent_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	left_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	right_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_parents mediumtext(16777215) NOT NULL DEFAULT '',
	forum_name text(65535) NOT NULL DEFAULT '',
	forum_desc text(65535) NOT NULL DEFAULT '',
	forum_desc_bitfield varchar(255) NOT NULL DEFAULT '',
	forum_desc_options INTEGER UNSIGNED NOT NULL DEFAULT '7',
	forum_desc_uid varchar(8) NOT NULL DEFAULT '',
	forum_link varchar(255) NOT NULL DEFAULT '',
	forum_password varchar(40) NOT NULL DEFAULT '',
	forum_style INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_image varchar(255) NOT NULL DEFAULT '',
	forum_rules text(65535) NOT NULL DEFAULT '',
	forum_rules_link varchar(255) NOT NULL DEFAULT '',
	forum_rules_bitfield varchar(255) NOT NULL DEFAULT '',
	forum_rules_options INTEGER UNSIGNED NOT NULL DEFAULT '7',
	forum_rules_uid varchar(8) NOT NULL DEFAULT '',
	forum_topics_per_page tinyint(4) NOT NULL DEFAULT '0',
	forum_type tinyint(4) NOT NULL DEFAULT '0',
	forum_status tinyint(4) NOT NULL DEFAULT '0',
	forum_posts INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_topics INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_topics_real INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_last_post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_last_poster_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_last_post_subject text(65535) NOT NULL DEFAULT '',
	forum_last_post_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_last_poster_name varchar(255) NOT NULL DEFAULT '',
	forum_last_poster_colour varchar(6) NOT NULL DEFAULT '',
	forum_flags tinyint(4) NOT NULL DEFAULT '32',
	display_subforum_list INTEGER UNSIGNED NOT NULL DEFAULT '1',
	display_on_index INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_indexing INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_icons INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_prune INTEGER UNSIGNED NOT NULL DEFAULT '0',
	prune_next INTEGER UNSIGNED NOT NULL DEFAULT '0',
	prune_days INTEGER UNSIGNED NOT NULL DEFAULT '0',
	prune_viewed INTEGER UNSIGNED NOT NULL DEFAULT '0',
	prune_freq INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_forums_left_right_id ON phpbb_forums (left_id, right_id);
CREATE INDEX phpbb_forums_forum_lastpost_id ON phpbb_forums (forum_last_post_id);

# Table: 'phpbb_forums_access'
CREATE TABLE phpbb_forums_access (
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_id char(32) NOT NULL DEFAULT '',
	PRIMARY KEY (forum_id, user_id, session_id)
);


# Table: 'phpbb_forums_track'
CREATE TABLE phpbb_forums_track (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	mark_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (user_id, forum_id)
);


# Table: 'phpbb_forums_watch'
CREATE TABLE phpbb_forums_watch (
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	notify_status INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_forums_watch_forum_id ON phpbb_forums_watch (forum_id);
CREATE INDEX phpbb_forums_watch_user_id ON phpbb_forums_watch (user_id);
CREATE INDEX phpbb_forums_watch_notify_stat ON phpbb_forums_watch (notify_status);

# Table: 'phpbb_groups'
CREATE TABLE phpbb_groups (
	group_id INTEGER PRIMARY KEY NOT NULL ,
	group_type tinyint(4) NOT NULL DEFAULT '1',
	group_founder_manage INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_name varchar(255) NOT NULL DEFAULT '',
	group_desc text(65535) NOT NULL DEFAULT '',
	group_desc_bitfield varchar(255) NOT NULL DEFAULT '',
	group_desc_options INTEGER UNSIGNED NOT NULL DEFAULT '7',
	group_desc_uid varchar(8) NOT NULL DEFAULT '',
	group_display INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_avatar varchar(255) NOT NULL DEFAULT '',
	group_avatar_type tinyint(2) NOT NULL DEFAULT '0',
	group_avatar_width INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_avatar_height INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_rank INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_colour varchar(6) NOT NULL DEFAULT '',
	group_sig_chars INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_receive_pm INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_message_limit INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_legend INTEGER UNSIGNED NOT NULL DEFAULT '1'
);

CREATE INDEX phpbb_groups_group_legend_name ON phpbb_groups (group_legend, group_name);

# Table: 'phpbb_icons'
CREATE TABLE phpbb_icons (
	icons_id INTEGER PRIMARY KEY NOT NULL ,
	icons_url varchar(255) NOT NULL DEFAULT '',
	icons_width tinyint(4) NOT NULL DEFAULT '0',
	icons_height tinyint(4) NOT NULL DEFAULT '0',
	icons_order INTEGER UNSIGNED NOT NULL DEFAULT '0',
	display_on_posting INTEGER UNSIGNED NOT NULL DEFAULT '1'
);

CREATE INDEX phpbb_icons_display_on_posting ON phpbb_icons (display_on_posting);

# Table: 'phpbb_lang'
CREATE TABLE phpbb_lang (
	lang_id INTEGER PRIMARY KEY NOT NULL ,
	lang_iso varchar(30) NOT NULL DEFAULT '',
	lang_dir varchar(30) NOT NULL DEFAULT '',
	lang_english_name varchar(100) NOT NULL DEFAULT '',
	lang_local_name varchar(255) NOT NULL DEFAULT '',
	lang_author varchar(255) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_lang_lang_iso ON phpbb_lang (lang_iso);

# Table: 'phpbb_log'
CREATE TABLE phpbb_log (
	log_id INTEGER PRIMARY KEY NOT NULL ,
	log_type tinyint(4) NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	reportee_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	log_ip varchar(40) NOT NULL DEFAULT '',
	log_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	log_operation text(65535) NOT NULL DEFAULT '',
	log_data mediumtext(16777215) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_log_log_type ON phpbb_log (log_type);
CREATE INDEX phpbb_log_forum_id ON phpbb_log (forum_id);
CREATE INDEX phpbb_log_topic_id ON phpbb_log (topic_id);
CREATE INDEX phpbb_log_reportee_id ON phpbb_log (reportee_id);
CREATE INDEX phpbb_log_user_id ON phpbb_log (user_id);

# Table: 'phpbb_moderator_cache'
CREATE TABLE phpbb_moderator_cache (
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	username varchar(255) NOT NULL DEFAULT '',
	group_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_name varchar(255) NOT NULL DEFAULT '',
	display_on_index INTEGER UNSIGNED NOT NULL DEFAULT '1'
);

CREATE INDEX phpbb_moderator_cache_disp_idx ON phpbb_moderator_cache (display_on_index);
CREATE INDEX phpbb_moderator_cache_forum_id ON phpbb_moderator_cache (forum_id);

# Table: 'phpbb_modules'
CREATE TABLE phpbb_modules (
	module_id INTEGER PRIMARY KEY NOT NULL ,
	module_enabled INTEGER UNSIGNED NOT NULL DEFAULT '1',
	module_display INTEGER UNSIGNED NOT NULL DEFAULT '1',
	module_basename varchar(255) NOT NULL DEFAULT '',
	module_class varchar(10) NOT NULL DEFAULT '',
	parent_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	left_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	right_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	module_langname varchar(255) NOT NULL DEFAULT '',
	module_mode varchar(255) NOT NULL DEFAULT '',
	module_auth varchar(255) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_modules_left_right_id ON phpbb_modules (left_id, right_id);
CREATE INDEX phpbb_modules_module_enabled ON phpbb_modules (module_enabled);
CREATE INDEX phpbb_modules_class_left_id ON phpbb_modules (module_class, left_id);

# Table: 'phpbb_poll_options'
CREATE TABLE phpbb_poll_options (
	poll_option_id tinyint(4) NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_option_text text(65535) NOT NULL DEFAULT '',
	poll_option_total INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_poll_options_poll_opt_id ON phpbb_poll_options (poll_option_id);
CREATE INDEX phpbb_poll_options_topic_id ON phpbb_poll_options (topic_id);

# Table: 'phpbb_poll_votes'
CREATE TABLE phpbb_poll_votes (
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_option_id tinyint(4) NOT NULL DEFAULT '0',
	vote_user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	vote_user_ip varchar(40) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_poll_votes_topic_id ON phpbb_poll_votes (topic_id);
CREATE INDEX phpbb_poll_votes_vote_user_id ON phpbb_poll_votes (vote_user_id);
CREATE INDEX phpbb_poll_votes_vote_user_ip ON phpbb_poll_votes (vote_user_ip);

# Table: 'phpbb_posts'
CREATE TABLE phpbb_posts (
	post_id INTEGER PRIMARY KEY NOT NULL ,
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poster_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	icon_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poster_ip varchar(40) NOT NULL DEFAULT '',
	post_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_approved INTEGER UNSIGNED NOT NULL DEFAULT '1',
	post_reported INTEGER UNSIGNED NOT NULL DEFAULT '0',
	enable_bbcode INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_smilies INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_magic_url INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_sig INTEGER UNSIGNED NOT NULL DEFAULT '1',
	post_username varchar(255) NOT NULL DEFAULT '',
	post_subject text(65535) NOT NULL DEFAULT '',
	post_text mediumtext(16777215) NOT NULL DEFAULT '',
	post_checksum varchar(32) NOT NULL DEFAULT '',
	post_attachment INTEGER UNSIGNED NOT NULL DEFAULT '0',
	bbcode_bitfield varchar(255) NOT NULL DEFAULT '',
	bbcode_uid varchar(8) NOT NULL DEFAULT '',
	post_postcount INTEGER UNSIGNED NOT NULL DEFAULT '1',
	post_edit_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_edit_reason text(65535) NOT NULL DEFAULT '',
	post_edit_user INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_edit_count INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_edit_locked INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_posts_forum_id ON phpbb_posts (forum_id);
CREATE INDEX phpbb_posts_topic_id ON phpbb_posts (topic_id);
CREATE INDEX phpbb_posts_poster_ip ON phpbb_posts (poster_ip);
CREATE INDEX phpbb_posts_poster_id ON phpbb_posts (poster_id);
CREATE INDEX phpbb_posts_post_approved ON phpbb_posts (post_approved);
CREATE INDEX phpbb_posts_tid_post_time ON phpbb_posts (topic_id, post_time);

# Table: 'phpbb_privmsgs'
CREATE TABLE phpbb_privmsgs (
	msg_id INTEGER PRIMARY KEY NOT NULL ,
	root_level INTEGER UNSIGNED NOT NULL DEFAULT '0',
	author_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	icon_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	author_ip varchar(40) NOT NULL DEFAULT '',
	message_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	enable_bbcode INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_smilies INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_magic_url INTEGER UNSIGNED NOT NULL DEFAULT '1',
	enable_sig INTEGER UNSIGNED NOT NULL DEFAULT '1',
	message_subject text(65535) NOT NULL DEFAULT '',
	message_text mediumtext(16777215) NOT NULL DEFAULT '',
	message_edit_reason text(65535) NOT NULL DEFAULT '',
	message_edit_user INTEGER UNSIGNED NOT NULL DEFAULT '0',
	message_attachment INTEGER UNSIGNED NOT NULL DEFAULT '0',
	bbcode_bitfield varchar(255) NOT NULL DEFAULT '',
	bbcode_uid varchar(8) NOT NULL DEFAULT '',
	message_edit_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	message_edit_count INTEGER UNSIGNED NOT NULL DEFAULT '0',
	to_address text(65535) NOT NULL DEFAULT '',
	bcc_address text(65535) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_privmsgs_author_ip ON phpbb_privmsgs (author_ip);
CREATE INDEX phpbb_privmsgs_message_time ON phpbb_privmsgs (message_time);
CREATE INDEX phpbb_privmsgs_author_id ON phpbb_privmsgs (author_id);
CREATE INDEX phpbb_privmsgs_root_level ON phpbb_privmsgs (root_level);

# Table: 'phpbb_privmsgs_folder'
CREATE TABLE phpbb_privmsgs_folder (
	folder_id INTEGER PRIMARY KEY NOT NULL ,
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	folder_name varchar(255) NOT NULL DEFAULT '',
	pm_count INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_privmsgs_folder_user_id ON phpbb_privmsgs_folder (user_id);

# Table: 'phpbb_privmsgs_rules'
CREATE TABLE phpbb_privmsgs_rules (
	rule_id INTEGER PRIMARY KEY NOT NULL ,
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_check INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_connection INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_string varchar(255) NOT NULL DEFAULT '',
	rule_user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_group_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_action INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rule_folder_id int(11) NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_privmsgs_rules_user_id ON phpbb_privmsgs_rules (user_id);

# Table: 'phpbb_privmsgs_to'
CREATE TABLE phpbb_privmsgs_to (
	msg_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	author_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	pm_deleted INTEGER UNSIGNED NOT NULL DEFAULT '0',
	pm_new INTEGER UNSIGNED NOT NULL DEFAULT '1',
	pm_unread INTEGER UNSIGNED NOT NULL DEFAULT '1',
	pm_replied INTEGER UNSIGNED NOT NULL DEFAULT '0',
	pm_marked INTEGER UNSIGNED NOT NULL DEFAULT '0',
	pm_forwarded INTEGER UNSIGNED NOT NULL DEFAULT '0',
	folder_id int(11) NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_privmsgs_to_msg_id ON phpbb_privmsgs_to (msg_id);
CREATE INDEX phpbb_privmsgs_to_author_id ON phpbb_privmsgs_to (author_id);
CREATE INDEX phpbb_privmsgs_to_usr_flder_id ON phpbb_privmsgs_to (user_id, folder_id);

# Table: 'phpbb_profile_fields'
CREATE TABLE phpbb_profile_fields (
	field_id INTEGER PRIMARY KEY NOT NULL ,
	field_name varchar(255) NOT NULL DEFAULT '',
	field_type tinyint(4) NOT NULL DEFAULT '0',
	field_ident varchar(20) NOT NULL DEFAULT '',
	field_length varchar(20) NOT NULL DEFAULT '',
	field_minlen varchar(255) NOT NULL DEFAULT '',
	field_maxlen varchar(255) NOT NULL DEFAULT '',
	field_novalue varchar(255) NOT NULL DEFAULT '',
	field_default_value varchar(255) NOT NULL DEFAULT '',
	field_validation varchar(20) NOT NULL DEFAULT '',
	field_required INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_show_on_reg INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_hide INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_no_view INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_active INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_order INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_profile_fields_fld_type ON phpbb_profile_fields (field_type);
CREATE INDEX phpbb_profile_fields_fld_ordr ON phpbb_profile_fields (field_order);

# Table: 'phpbb_profile_fields_data'
CREATE TABLE phpbb_profile_fields_data (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (user_id)
);


# Table: 'phpbb_profile_fields_lang'
CREATE TABLE phpbb_profile_fields_lang (
	field_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	lang_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	option_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	field_type tinyint(4) NOT NULL DEFAULT '0',
	lang_value varchar(255) NOT NULL DEFAULT '',
	PRIMARY KEY (field_id, lang_id, option_id)
);


# Table: 'phpbb_profile_lang'
CREATE TABLE phpbb_profile_lang (
	field_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	lang_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	lang_name varchar(255) NOT NULL DEFAULT '',
	lang_explain text(65535) NOT NULL DEFAULT '',
	lang_default_value varchar(255) NOT NULL DEFAULT '',
	PRIMARY KEY (field_id, lang_id)
);


# Table: 'phpbb_ranks'
CREATE TABLE phpbb_ranks (
	rank_id INTEGER PRIMARY KEY NOT NULL ,
	rank_title varchar(255) NOT NULL DEFAULT '',
	rank_min INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rank_special INTEGER UNSIGNED NOT NULL DEFAULT '0',
	rank_image varchar(255) NOT NULL DEFAULT ''
);


# Table: 'phpbb_reports'
CREATE TABLE phpbb_reports (
	report_id INTEGER PRIMARY KEY NOT NULL ,
	reason_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_notify INTEGER UNSIGNED NOT NULL DEFAULT '0',
	report_closed INTEGER UNSIGNED NOT NULL DEFAULT '0',
	report_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	report_text mediumtext(16777215) NOT NULL DEFAULT ''
);


# Table: 'phpbb_reports_reasons'
CREATE TABLE phpbb_reports_reasons (
	reason_id INTEGER PRIMARY KEY NOT NULL ,
	reason_title varchar(255) NOT NULL DEFAULT '',
	reason_description mediumtext(16777215) NOT NULL DEFAULT '',
	reason_order INTEGER UNSIGNED NOT NULL DEFAULT '0'
);


# Table: 'phpbb_search_results'
CREATE TABLE phpbb_search_results (
	search_key varchar(32) NOT NULL DEFAULT '',
	search_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	search_keywords mediumtext(16777215) NOT NULL DEFAULT '',
	search_authors mediumtext(16777215) NOT NULL DEFAULT '',
	PRIMARY KEY (search_key)
);


# Table: 'phpbb_search_wordlist'
CREATE TABLE phpbb_search_wordlist (
	word_id INTEGER PRIMARY KEY NOT NULL ,
	word_text varchar(255) NOT NULL DEFAULT '',
	word_common INTEGER UNSIGNED NOT NULL DEFAULT '0',
	word_count INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE UNIQUE INDEX phpbb_search_wordlist_wrd_txt ON phpbb_search_wordlist (word_text);
CREATE INDEX phpbb_search_wordlist_wrd_cnt ON phpbb_search_wordlist (word_count);

# Table: 'phpbb_search_wordmatch'
CREATE TABLE phpbb_search_wordmatch (
	post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	word_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	title_match INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE UNIQUE INDEX phpbb_search_wordmatch_unq_mtch ON phpbb_search_wordmatch (word_id, post_id, title_match);
CREATE INDEX phpbb_search_wordmatch_word_id ON phpbb_search_wordmatch (word_id);
CREATE INDEX phpbb_search_wordmatch_post_id ON phpbb_search_wordmatch (post_id);

# Table: 'phpbb_sessions'
CREATE TABLE phpbb_sessions (
	session_id char(32) NOT NULL DEFAULT '',
	session_user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_last_visit INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_start INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_ip varchar(40) NOT NULL DEFAULT '',
	session_browser varchar(150) NOT NULL DEFAULT '',
	session_forwarded_for varchar(255) NOT NULL DEFAULT '',
	session_page varchar(255) NOT NULL DEFAULT '',
	session_viewonline INTEGER UNSIGNED NOT NULL DEFAULT '1',
	session_autologin INTEGER UNSIGNED NOT NULL DEFAULT '0',
	session_admin INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (session_id)
);

CREATE INDEX phpbb_sessions_session_time ON phpbb_sessions (session_time);
CREATE INDEX phpbb_sessions_session_user_id ON phpbb_sessions (session_user_id);
CREATE INDEX phpbb_sessions_session_forum_id ON phpbb_sessions (session_forum_id);

# Table: 'phpbb_sessions_keys'
CREATE TABLE phpbb_sessions_keys (
	key_id char(32) NOT NULL DEFAULT '',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	last_ip varchar(40) NOT NULL DEFAULT '',
	last_login INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (key_id, user_id)
);

CREATE INDEX phpbb_sessions_keys_last_login ON phpbb_sessions_keys (last_login);

# Table: 'phpbb_sitelist'
CREATE TABLE phpbb_sitelist (
	site_id INTEGER PRIMARY KEY NOT NULL ,
	site_ip varchar(40) NOT NULL DEFAULT '',
	site_hostname varchar(255) NOT NULL DEFAULT '',
	ip_exclude INTEGER UNSIGNED NOT NULL DEFAULT '0'
);


# Table: 'phpbb_smilies'
CREATE TABLE phpbb_smilies (
	smiley_id INTEGER PRIMARY KEY NOT NULL ,
	code varchar(50) NOT NULL DEFAULT '',
	emotion varchar(50) NOT NULL DEFAULT '',
	smiley_url varchar(50) NOT NULL DEFAULT '',
	smiley_width INTEGER UNSIGNED NOT NULL DEFAULT '0',
	smiley_height INTEGER UNSIGNED NOT NULL DEFAULT '0',
	smiley_order INTEGER UNSIGNED NOT NULL DEFAULT '0',
	display_on_posting INTEGER UNSIGNED NOT NULL DEFAULT '1'
);

CREATE INDEX phpbb_smilies_display_on_post ON phpbb_smilies (display_on_posting);

# Table: 'phpbb_styles'
CREATE TABLE phpbb_styles (
	style_id INTEGER PRIMARY KEY NOT NULL ,
	style_name varchar(255) NOT NULL DEFAULT '',
	style_copyright varchar(255) NOT NULL DEFAULT '',
	style_active INTEGER UNSIGNED NOT NULL DEFAULT '1',
	template_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	theme_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	imageset_id INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE UNIQUE INDEX phpbb_styles_style_name ON phpbb_styles (style_name);
CREATE INDEX phpbb_styles_template_id ON phpbb_styles (template_id);
CREATE INDEX phpbb_styles_theme_id ON phpbb_styles (theme_id);
CREATE INDEX phpbb_styles_imageset_id ON phpbb_styles (imageset_id);

# Table: 'phpbb_styles_template'
CREATE TABLE phpbb_styles_template (
	template_id INTEGER PRIMARY KEY NOT NULL ,
	template_name varchar(255) NOT NULL DEFAULT '',
	template_copyright varchar(255) NOT NULL DEFAULT '',
	template_path varchar(100) NOT NULL DEFAULT '',
	bbcode_bitfield varchar(255) NOT NULL DEFAULT 'kNg=',
	template_storedb INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE UNIQUE INDEX phpbb_styles_template_tmplte_nm ON phpbb_styles_template (template_name);

# Table: 'phpbb_styles_template_data'
CREATE TABLE phpbb_styles_template_data (
	template_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	template_filename varchar(100) NOT NULL DEFAULT '',
	template_included text(65535) NOT NULL DEFAULT '',
	template_mtime INTEGER UNSIGNED NOT NULL DEFAULT '0',
	template_data mediumtext(16777215) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_styles_template_data_tid ON phpbb_styles_template_data (template_id);
CREATE INDEX phpbb_styles_template_data_tfn ON phpbb_styles_template_data (template_filename);

# Table: 'phpbb_styles_theme'
CREATE TABLE phpbb_styles_theme (
	theme_id INTEGER PRIMARY KEY NOT NULL ,
	theme_name varchar(255) NOT NULL DEFAULT '',
	theme_copyright varchar(255) NOT NULL DEFAULT '',
	theme_path varchar(100) NOT NULL DEFAULT '',
	theme_storedb INTEGER UNSIGNED NOT NULL DEFAULT '0',
	theme_mtime INTEGER UNSIGNED NOT NULL DEFAULT '0',
	theme_data mediumtext(16777215) NOT NULL DEFAULT ''
);

CREATE UNIQUE INDEX phpbb_styles_theme_theme_name ON phpbb_styles_theme (theme_name);

# Table: 'phpbb_styles_imageset'
CREATE TABLE phpbb_styles_imageset (
	imageset_id INTEGER PRIMARY KEY NOT NULL ,
	imageset_name varchar(255) NOT NULL DEFAULT '',
	imageset_copyright varchar(255) NOT NULL DEFAULT '',
	imageset_path varchar(100) NOT NULL DEFAULT ''
);

CREATE UNIQUE INDEX phpbb_styles_imageset_imgset_nm ON phpbb_styles_imageset (imageset_name);

# Table: 'phpbb_styles_imageset_data'
CREATE TABLE phpbb_styles_imageset_data (
	image_id INTEGER PRIMARY KEY NOT NULL ,
	image_name varchar(200) NOT NULL DEFAULT '',
	image_filename varchar(200) NOT NULL DEFAULT '',
	image_lang varchar(30) NOT NULL DEFAULT '',
	image_height INTEGER UNSIGNED NOT NULL DEFAULT '0',
	image_width INTEGER UNSIGNED NOT NULL DEFAULT '0',
	imageset_id INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_styles_imageset_data_i_d ON phpbb_styles_imageset_data (imageset_id);

# Table: 'phpbb_topics'
CREATE TABLE phpbb_topics (
	topic_id INTEGER PRIMARY KEY NOT NULL ,
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	icon_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_attachment INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_approved INTEGER UNSIGNED NOT NULL DEFAULT '1',
	topic_reported INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_title text(65535) NOT NULL DEFAULT '',
	topic_poster INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_time_limit INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_views INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_replies INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_replies_real INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_status tinyint(3) NOT NULL DEFAULT '0',
	topic_type tinyint(3) NOT NULL DEFAULT '0',
	topic_first_post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_first_poster_name varchar(255) NOT NULL DEFAULT '',
	topic_first_poster_colour varchar(6) NOT NULL DEFAULT '',
	topic_last_post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_last_poster_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_last_poster_name varchar(255) NOT NULL DEFAULT '',
	topic_last_poster_colour varchar(6) NOT NULL DEFAULT '',
	topic_last_post_subject text(65535) NOT NULL DEFAULT '',
	topic_last_post_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_last_view_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_moved_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_bumped INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_bumper INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_title text(65535) NOT NULL DEFAULT '',
	poll_start INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_length INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_max_options tinyint(4) NOT NULL DEFAULT '1',
	poll_last_vote INTEGER UNSIGNED NOT NULL DEFAULT '0',
	poll_vote_change INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_topics_forum_id ON phpbb_topics (forum_id);
CREATE INDEX phpbb_topics_forum_id_type ON phpbb_topics (forum_id, topic_type);
CREATE INDEX phpbb_topics_last_post_time ON phpbb_topics (topic_last_post_time);
CREATE INDEX phpbb_topics_topic_approved ON phpbb_topics (topic_approved);
CREATE INDEX phpbb_topics_forum_appr_last ON phpbb_topics (forum_id, topic_approved, topic_last_post_id);
CREATE INDEX phpbb_topics_fid_time_moved ON phpbb_topics (forum_id, topic_last_post_time, topic_moved_id);

# Table: 'phpbb_topics_track'
CREATE TABLE phpbb_topics_track (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	forum_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	mark_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (user_id, topic_id)
);

CREATE INDEX phpbb_topics_track_forum_id ON phpbb_topics_track (forum_id);

# Table: 'phpbb_topics_posted'
CREATE TABLE phpbb_topics_posted (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	topic_posted INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (user_id, topic_id)
);


# Table: 'phpbb_topics_watch'
CREATE TABLE phpbb_topics_watch (
	topic_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	notify_status INTEGER UNSIGNED NOT NULL DEFAULT '0'
);

CREATE INDEX phpbb_topics_watch_topic_id ON phpbb_topics_watch (topic_id);
CREATE INDEX phpbb_topics_watch_user_id ON phpbb_topics_watch (user_id);
CREATE INDEX phpbb_topics_watch_notify_stat ON phpbb_topics_watch (notify_status);

# Table: 'phpbb_user_group'
CREATE TABLE phpbb_user_group (
	group_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	group_leader INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_pending INTEGER UNSIGNED NOT NULL DEFAULT '1'
);

CREATE INDEX phpbb_user_group_group_id ON phpbb_user_group (group_id);
CREATE INDEX phpbb_user_group_user_id ON phpbb_user_group (user_id);
CREATE INDEX phpbb_user_group_group_leader ON phpbb_user_group (group_leader);

# Table: 'phpbb_users'
CREATE TABLE phpbb_users (
	user_id INTEGER PRIMARY KEY NOT NULL ,
	user_type tinyint(2) NOT NULL DEFAULT '0',
	group_id INTEGER UNSIGNED NOT NULL DEFAULT '3',
	user_permissions mediumtext(16777215) NOT NULL DEFAULT '',
	user_perm_from INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_ip varchar(40) NOT NULL DEFAULT '',
	user_regdate INTEGER UNSIGNED NOT NULL DEFAULT '0',
	username varchar(255) NOT NULL DEFAULT '',
	username_clean varchar(255) NOT NULL DEFAULT '',
	user_password varchar(40) NOT NULL DEFAULT '',
	user_passchg INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_pass_convert INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_email varchar(100) NOT NULL DEFAULT '',
	user_email_hash bigint(20) NOT NULL DEFAULT '0',
	user_birthday varchar(10) NOT NULL DEFAULT '',
	user_lastvisit INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_lastmark INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_lastpost_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_lastpage varchar(200) NOT NULL DEFAULT '',
	user_last_confirm_key varchar(10) NOT NULL DEFAULT '',
	user_last_search INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_warnings tinyint(4) NOT NULL DEFAULT '0',
	user_last_warning INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_login_attempts tinyint(4) NOT NULL DEFAULT '0',
	user_inactive_reason tinyint(2) NOT NULL DEFAULT '0',
	user_inactive_time INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_posts INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_lang varchar(30) NOT NULL DEFAULT '',
	user_timezone decimal(5,2) NOT NULL DEFAULT '0',
	user_dst INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_dateformat varchar(30) NOT NULL DEFAULT 'd M Y H:i',
	user_style INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_rank INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_colour varchar(6) NOT NULL DEFAULT '',
	user_new_privmsg int(4) NOT NULL DEFAULT '0',
	user_unread_privmsg int(4) NOT NULL DEFAULT '0',
	user_last_privmsg INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_message_rules INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_full_folder int(11) NOT NULL DEFAULT '-3',
	user_emailtime INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_topic_show_days INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_topic_sortby_type varchar(1) NOT NULL DEFAULT 't',
	user_topic_sortby_dir varchar(1) NOT NULL DEFAULT 'd',
	user_post_show_days INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_post_sortby_type varchar(1) NOT NULL DEFAULT 't',
	user_post_sortby_dir varchar(1) NOT NULL DEFAULT 'a',
	user_notify INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_notify_pm INTEGER UNSIGNED NOT NULL DEFAULT '1',
	user_notify_type tinyint(4) NOT NULL DEFAULT '0',
	user_allow_pm INTEGER UNSIGNED NOT NULL DEFAULT '1',
	user_allow_viewonline INTEGER UNSIGNED NOT NULL DEFAULT '1',
	user_allow_viewemail INTEGER UNSIGNED NOT NULL DEFAULT '1',
	user_allow_massemail INTEGER UNSIGNED NOT NULL DEFAULT '1',
	user_options INTEGER UNSIGNED NOT NULL DEFAULT '895',
	user_avatar varchar(255) NOT NULL DEFAULT '',
	user_avatar_type tinyint(2) NOT NULL DEFAULT '0',
	user_avatar_width INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_avatar_height INTEGER UNSIGNED NOT NULL DEFAULT '0',
	user_sig mediumtext(16777215) NOT NULL DEFAULT '',
	user_sig_bbcode_uid varchar(8) NOT NULL DEFAULT '',
	user_sig_bbcode_bitfield varchar(255) NOT NULL DEFAULT '',
	user_from varchar(100) NOT NULL DEFAULT '',
	user_icq varchar(15) NOT NULL DEFAULT '',
	user_aim varchar(255) NOT NULL DEFAULT '',
	user_yim varchar(255) NOT NULL DEFAULT '',
	user_msnm varchar(255) NOT NULL DEFAULT '',
	user_jabber varchar(255) NOT NULL DEFAULT '',
	user_website varchar(200) NOT NULL DEFAULT '',
	user_occ text(65535) NOT NULL DEFAULT '',
	user_interests text(65535) NOT NULL DEFAULT '',
	user_actkey varchar(32) NOT NULL DEFAULT '',
	user_newpasswd varchar(40) NOT NULL DEFAULT '',
	user_form_salt varchar(32) NOT NULL DEFAULT ''
);

CREATE INDEX phpbb_users_user_birthday ON phpbb_users (user_birthday);
CREATE INDEX phpbb_users_user_email_hash ON phpbb_users (user_email_hash);
CREATE INDEX phpbb_users_user_type ON phpbb_users (user_type);
CREATE UNIQUE INDEX phpbb_users_username_clean ON phpbb_users (username_clean);

# Table: 'phpbb_warnings'
CREATE TABLE phpbb_warnings (
	warning_id INTEGER PRIMARY KEY NOT NULL ,
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	post_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	log_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	warning_time INTEGER UNSIGNED NOT NULL DEFAULT '0'
);


# Table: 'phpbb_words'
CREATE TABLE phpbb_words (
	word_id INTEGER PRIMARY KEY NOT NULL ,
	word varchar(255) NOT NULL DEFAULT '',
	replacement varchar(255) NOT NULL DEFAULT ''
);


# Table: 'phpbb_zebra'
CREATE TABLE phpbb_zebra (
	user_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	zebra_id INTEGER UNSIGNED NOT NULL DEFAULT '0',
	friend INTEGER UNSIGNED NOT NULL DEFAULT '0',
	foe INTEGER UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (user_id, zebra_id)
);



COMMIT;