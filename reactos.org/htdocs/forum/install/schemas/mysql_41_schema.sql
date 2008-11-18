#
# $Id: mysql_41_schema.sql 8456 2008-03-22 12:31:17Z Kellanved $
#

# Table: 'phpbb_attachments'
CREATE TABLE phpbb_attachments (
	attach_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	post_msg_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	in_message tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	poster_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	is_orphan tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	physical_filename varchar(255) DEFAULT '' NOT NULL,
	real_filename varchar(255) DEFAULT '' NOT NULL,
	download_count mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	attach_comment text NOT NULL,
	extension varchar(100) DEFAULT '' NOT NULL,
	mimetype varchar(100) DEFAULT '' NOT NULL,
	filesize int(20) UNSIGNED DEFAULT '0' NOT NULL,
	filetime int(11) UNSIGNED DEFAULT '0' NOT NULL,
	thumbnail tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (attach_id),
	KEY filetime (filetime),
	KEY post_msg_id (post_msg_id),
	KEY topic_id (topic_id),
	KEY poster_id (poster_id),
	KEY is_orphan (is_orphan)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_acl_groups'
CREATE TABLE phpbb_acl_groups (
	group_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_option_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_role_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_setting tinyint(2) DEFAULT '0' NOT NULL,
	KEY group_id (group_id),
	KEY auth_opt_id (auth_option_id),
	KEY auth_role_id (auth_role_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_acl_options'
CREATE TABLE phpbb_acl_options (
	auth_option_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	auth_option varchar(50) DEFAULT '' NOT NULL,
	is_global tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	is_local tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	founder_only tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (auth_option_id),
	KEY auth_option (auth_option)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_acl_roles'
CREATE TABLE phpbb_acl_roles (
	role_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	role_name varchar(255) DEFAULT '' NOT NULL,
	role_description text NOT NULL,
	role_type varchar(10) DEFAULT '' NOT NULL,
	role_order smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (role_id),
	KEY role_type (role_type),
	KEY role_order (role_order)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_acl_roles_data'
CREATE TABLE phpbb_acl_roles_data (
	role_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_option_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_setting tinyint(2) DEFAULT '0' NOT NULL,
	PRIMARY KEY (role_id, auth_option_id),
	KEY ath_op_id (auth_option_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_acl_users'
CREATE TABLE phpbb_acl_users (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_option_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_role_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	auth_setting tinyint(2) DEFAULT '0' NOT NULL,
	KEY user_id (user_id),
	KEY auth_option_id (auth_option_id),
	KEY auth_role_id (auth_role_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_banlist'
CREATE TABLE phpbb_banlist (
	ban_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	ban_userid mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	ban_ip varchar(40) DEFAULT '' NOT NULL,
	ban_email varchar(100) DEFAULT '' NOT NULL,
	ban_start int(11) UNSIGNED DEFAULT '0' NOT NULL,
	ban_end int(11) UNSIGNED DEFAULT '0' NOT NULL,
	ban_exclude tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	ban_reason varchar(255) DEFAULT '' NOT NULL,
	ban_give_reason varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (ban_id),
	KEY ban_end (ban_end),
	KEY ban_user (ban_userid, ban_exclude),
	KEY ban_email (ban_email, ban_exclude),
	KEY ban_ip (ban_ip, ban_exclude)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_bbcodes'
CREATE TABLE phpbb_bbcodes (
	bbcode_id tinyint(3) DEFAULT '0' NOT NULL,
	bbcode_tag varchar(16) DEFAULT '' NOT NULL,
	bbcode_helpline varchar(255) DEFAULT '' NOT NULL,
	display_on_posting tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	bbcode_match text NOT NULL,
	bbcode_tpl mediumtext NOT NULL,
	first_pass_match mediumtext NOT NULL,
	first_pass_replace mediumtext NOT NULL,
	second_pass_match mediumtext NOT NULL,
	second_pass_replace mediumtext NOT NULL,
	PRIMARY KEY (bbcode_id),
	KEY display_on_post (display_on_posting)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_bookmarks'
CREATE TABLE phpbb_bookmarks (
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (topic_id, user_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_bots'
CREATE TABLE phpbb_bots (
	bot_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	bot_active tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	bot_name varchar(255) DEFAULT '' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	bot_agent varchar(255) DEFAULT '' NOT NULL,
	bot_ip varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (bot_id),
	KEY bot_active (bot_active)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_config'
CREATE TABLE phpbb_config (
	config_name varchar(255) DEFAULT '' NOT NULL,
	config_value varchar(255) DEFAULT '' NOT NULL,
	is_dynamic tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (config_name),
	KEY is_dynamic (is_dynamic)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_confirm'
CREATE TABLE phpbb_confirm (
	confirm_id char(32) DEFAULT '' NOT NULL,
	session_id char(32) DEFAULT '' NOT NULL,
	confirm_type tinyint(3) DEFAULT '0' NOT NULL,
	code varchar(8) DEFAULT '' NOT NULL,
	seed int(10) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (session_id, confirm_id),
	KEY confirm_type (confirm_type)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_disallow'
CREATE TABLE phpbb_disallow (
	disallow_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	disallow_username varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (disallow_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_drafts'
CREATE TABLE phpbb_drafts (
	draft_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	save_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	draft_subject varchar(100) DEFAULT '' NOT NULL,
	draft_message mediumtext NOT NULL,
	PRIMARY KEY (draft_id),
	KEY save_time (save_time)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_extensions'
CREATE TABLE phpbb_extensions (
	extension_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	group_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	extension varchar(100) DEFAULT '' NOT NULL,
	PRIMARY KEY (extension_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_extension_groups'
CREATE TABLE phpbb_extension_groups (
	group_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	group_name varchar(255) DEFAULT '' NOT NULL,
	cat_id tinyint(2) DEFAULT '0' NOT NULL,
	allow_group tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	download_mode tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	upload_icon varchar(255) DEFAULT '' NOT NULL,
	max_filesize int(20) UNSIGNED DEFAULT '0' NOT NULL,
	allowed_forums text NOT NULL,
	allow_in_pm tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (group_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_forums'
CREATE TABLE phpbb_forums (
	forum_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	parent_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	left_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	right_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_parents mediumtext NOT NULL,
	forum_name varchar(255) DEFAULT '' NOT NULL,
	forum_desc text NOT NULL,
	forum_desc_bitfield varchar(255) DEFAULT '' NOT NULL,
	forum_desc_options int(11) UNSIGNED DEFAULT '7' NOT NULL,
	forum_desc_uid varchar(8) DEFAULT '' NOT NULL,
	forum_link varchar(255) DEFAULT '' NOT NULL,
	forum_password varchar(40) DEFAULT '' NOT NULL,
	forum_style smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	forum_image varchar(255) DEFAULT '' NOT NULL,
	forum_rules text NOT NULL,
	forum_rules_link varchar(255) DEFAULT '' NOT NULL,
	forum_rules_bitfield varchar(255) DEFAULT '' NOT NULL,
	forum_rules_options int(11) UNSIGNED DEFAULT '7' NOT NULL,
	forum_rules_uid varchar(8) DEFAULT '' NOT NULL,
	forum_topics_per_page tinyint(4) DEFAULT '0' NOT NULL,
	forum_type tinyint(4) DEFAULT '0' NOT NULL,
	forum_status tinyint(4) DEFAULT '0' NOT NULL,
	forum_posts mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_topics mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_topics_real mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_last_post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_last_poster_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_last_post_subject varchar(100) DEFAULT '' NOT NULL,
	forum_last_post_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	forum_last_poster_name varchar(255) DEFAULT '' NOT NULL,
	forum_last_poster_colour varchar(6) DEFAULT '' NOT NULL,
	forum_flags tinyint(4) DEFAULT '32' NOT NULL,
	display_subforum_list tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	display_on_index tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_indexing tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_icons tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_prune tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	prune_next int(11) UNSIGNED DEFAULT '0' NOT NULL,
	prune_days mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	prune_viewed mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	prune_freq mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (forum_id),
	KEY left_right_id (left_id, right_id),
	KEY forum_lastpost_id (forum_last_post_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_forums_access'
CREATE TABLE phpbb_forums_access (
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	session_id char(32) DEFAULT '' NOT NULL,
	PRIMARY KEY (forum_id, user_id, session_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_forums_track'
CREATE TABLE phpbb_forums_track (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	mark_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (user_id, forum_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_forums_watch'
CREATE TABLE phpbb_forums_watch (
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	notify_status tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	KEY forum_id (forum_id),
	KEY user_id (user_id),
	KEY notify_stat (notify_status)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_groups'
CREATE TABLE phpbb_groups (
	group_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	group_type tinyint(4) DEFAULT '1' NOT NULL,
	group_founder_manage tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	group_name varchar(255) DEFAULT '' NOT NULL,
	group_desc text NOT NULL,
	group_desc_bitfield varchar(255) DEFAULT '' NOT NULL,
	group_desc_options int(11) UNSIGNED DEFAULT '7' NOT NULL,
	group_desc_uid varchar(8) DEFAULT '' NOT NULL,
	group_display tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	group_avatar varchar(255) DEFAULT '' NOT NULL,
	group_avatar_type tinyint(2) DEFAULT '0' NOT NULL,
	group_avatar_width smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	group_avatar_height smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	group_rank mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	group_colour varchar(6) DEFAULT '' NOT NULL,
	group_sig_chars mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	group_receive_pm tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	group_message_limit mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	group_legend tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	PRIMARY KEY (group_id),
	KEY group_legend_name (group_legend, group_name)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_icons'
CREATE TABLE phpbb_icons (
	icons_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	icons_url varchar(255) DEFAULT '' NOT NULL,
	icons_width tinyint(4) DEFAULT '0' NOT NULL,
	icons_height tinyint(4) DEFAULT '0' NOT NULL,
	icons_order mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	display_on_posting tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	PRIMARY KEY (icons_id),
	KEY display_on_posting (display_on_posting)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_lang'
CREATE TABLE phpbb_lang (
	lang_id tinyint(4) NOT NULL auto_increment,
	lang_iso varchar(30) DEFAULT '' NOT NULL,
	lang_dir varchar(30) DEFAULT '' NOT NULL,
	lang_english_name varchar(100) DEFAULT '' NOT NULL,
	lang_local_name varchar(255) DEFAULT '' NOT NULL,
	lang_author varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (lang_id),
	KEY lang_iso (lang_iso)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_log'
CREATE TABLE phpbb_log (
	log_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	log_type tinyint(4) DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	reportee_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	log_ip varchar(40) DEFAULT '' NOT NULL,
	log_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	log_operation text NOT NULL,
	log_data mediumtext NOT NULL,
	PRIMARY KEY (log_id),
	KEY log_type (log_type),
	KEY forum_id (forum_id),
	KEY topic_id (topic_id),
	KEY reportee_id (reportee_id),
	KEY user_id (user_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_moderator_cache'
CREATE TABLE phpbb_moderator_cache (
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	username varchar(255) DEFAULT '' NOT NULL,
	group_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	group_name varchar(255) DEFAULT '' NOT NULL,
	display_on_index tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	KEY disp_idx (display_on_index),
	KEY forum_id (forum_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_modules'
CREATE TABLE phpbb_modules (
	module_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	module_enabled tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	module_display tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	module_basename varchar(255) DEFAULT '' NOT NULL,
	module_class varchar(10) DEFAULT '' NOT NULL,
	parent_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	left_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	right_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	module_langname varchar(255) DEFAULT '' NOT NULL,
	module_mode varchar(255) DEFAULT '' NOT NULL,
	module_auth varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (module_id),
	KEY left_right_id (left_id, right_id),
	KEY module_enabled (module_enabled),
	KEY class_left_id (module_class, left_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_poll_options'
CREATE TABLE phpbb_poll_options (
	poll_option_id tinyint(4) DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	poll_option_text text NOT NULL,
	poll_option_total mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	KEY poll_opt_id (poll_option_id),
	KEY topic_id (topic_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_poll_votes'
CREATE TABLE phpbb_poll_votes (
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	poll_option_id tinyint(4) DEFAULT '0' NOT NULL,
	vote_user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	vote_user_ip varchar(40) DEFAULT '' NOT NULL,
	KEY topic_id (topic_id),
	KEY vote_user_id (vote_user_id),
	KEY vote_user_ip (vote_user_ip)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_posts'
CREATE TABLE phpbb_posts (
	post_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	poster_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	icon_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	poster_ip varchar(40) DEFAULT '' NOT NULL,
	post_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	post_approved tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	post_reported tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	enable_bbcode tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_smilies tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_magic_url tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_sig tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	post_username varchar(255) DEFAULT '' NOT NULL,
	post_subject varchar(100) DEFAULT '' NOT NULL COLLATE utf8_unicode_ci,
	post_text mediumtext NOT NULL,
	post_checksum varchar(32) DEFAULT '' NOT NULL,
	post_attachment tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	post_postcount tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	post_edit_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	post_edit_reason varchar(255) DEFAULT '' NOT NULL,
	post_edit_user mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	post_edit_count smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	post_edit_locked tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (post_id),
	KEY forum_id (forum_id),
	KEY topic_id (topic_id),
	KEY poster_ip (poster_ip),
	KEY poster_id (poster_id),
	KEY post_approved (post_approved),
	KEY tid_post_time (topic_id, post_time)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_privmsgs'
CREATE TABLE phpbb_privmsgs (
	msg_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	root_level mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	author_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	icon_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	author_ip varchar(40) DEFAULT '' NOT NULL,
	message_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	enable_bbcode tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_smilies tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_magic_url tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	enable_sig tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	message_subject varchar(100) DEFAULT '' NOT NULL,
	message_text mediumtext NOT NULL,
	message_edit_reason varchar(255) DEFAULT '' NOT NULL,
	message_edit_user mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	message_attachment tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	message_edit_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	message_edit_count smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	to_address text NOT NULL,
	bcc_address text NOT NULL,
	PRIMARY KEY (msg_id),
	KEY author_ip (author_ip),
	KEY message_time (message_time),
	KEY author_id (author_id),
	KEY root_level (root_level)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_privmsgs_folder'
CREATE TABLE phpbb_privmsgs_folder (
	folder_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	folder_name varchar(255) DEFAULT '' NOT NULL,
	pm_count mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (folder_id),
	KEY user_id (user_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_privmsgs_rules'
CREATE TABLE phpbb_privmsgs_rules (
	rule_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_check mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_connection mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_string varchar(255) DEFAULT '' NOT NULL,
	rule_user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_group_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_action mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rule_folder_id int(11) DEFAULT '0' NOT NULL,
	PRIMARY KEY (rule_id),
	KEY user_id (user_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_privmsgs_to'
CREATE TABLE phpbb_privmsgs_to (
	msg_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	author_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	pm_deleted tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	pm_new tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	pm_unread tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	pm_replied tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	pm_marked tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	pm_forwarded tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	folder_id int(11) DEFAULT '0' NOT NULL,
	KEY msg_id (msg_id),
	KEY author_id (author_id),
	KEY usr_flder_id (user_id, folder_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_profile_fields'
CREATE TABLE phpbb_profile_fields (
	field_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	field_name varchar(255) DEFAULT '' NOT NULL,
	field_type tinyint(4) DEFAULT '0' NOT NULL,
	field_ident varchar(20) DEFAULT '' NOT NULL,
	field_length varchar(20) DEFAULT '' NOT NULL,
	field_minlen varchar(255) DEFAULT '' NOT NULL,
	field_maxlen varchar(255) DEFAULT '' NOT NULL,
	field_novalue varchar(255) DEFAULT '' NOT NULL,
	field_default_value varchar(255) DEFAULT '' NOT NULL,
	field_validation varchar(20) DEFAULT '' NOT NULL,
	field_required tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	field_show_on_reg tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	field_hide tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	field_no_view tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	field_active tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	field_order mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (field_id),
	KEY fld_type (field_type),
	KEY fld_ordr (field_order)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_profile_fields_data'
CREATE TABLE phpbb_profile_fields_data (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (user_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_profile_fields_lang'
CREATE TABLE phpbb_profile_fields_lang (
	field_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	lang_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	option_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	field_type tinyint(4) DEFAULT '0' NOT NULL,
	lang_value varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (field_id, lang_id, option_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_profile_lang'
CREATE TABLE phpbb_profile_lang (
	field_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	lang_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	lang_name varchar(255) DEFAULT '' NOT NULL,
	lang_explain text NOT NULL,
	lang_default_value varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (field_id, lang_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_ranks'
CREATE TABLE phpbb_ranks (
	rank_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	rank_title varchar(255) DEFAULT '' NOT NULL,
	rank_min mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	rank_special tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	rank_image varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (rank_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_reports'
CREATE TABLE phpbb_reports (
	report_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	reason_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_notify tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	report_closed tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	report_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	report_text mediumtext NOT NULL,
	PRIMARY KEY (report_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_reports_reasons'
CREATE TABLE phpbb_reports_reasons (
	reason_id smallint(4) UNSIGNED NOT NULL auto_increment,
	reason_title varchar(255) DEFAULT '' NOT NULL,
	reason_description mediumtext NOT NULL,
	reason_order smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (reason_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_search_results'
CREATE TABLE phpbb_search_results (
	search_key varchar(32) DEFAULT '' NOT NULL,
	search_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	search_keywords mediumtext NOT NULL,
	search_authors mediumtext NOT NULL,
	PRIMARY KEY (search_key)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_search_wordlist'
CREATE TABLE phpbb_search_wordlist (
	word_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	word_text varchar(255) DEFAULT '' NOT NULL,
	word_common tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	word_count mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (word_id),
	UNIQUE wrd_txt (word_text),
	KEY wrd_cnt (word_count)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_search_wordmatch'
CREATE TABLE phpbb_search_wordmatch (
	post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	word_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	title_match tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	UNIQUE unq_mtch (word_id, post_id, title_match),
	KEY word_id (word_id),
	KEY post_id (post_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_sessions'
CREATE TABLE phpbb_sessions (
	session_id char(32) DEFAULT '' NOT NULL,
	session_user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	session_forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	session_last_visit int(11) UNSIGNED DEFAULT '0' NOT NULL,
	session_start int(11) UNSIGNED DEFAULT '0' NOT NULL,
	session_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	session_ip varchar(40) DEFAULT '' NOT NULL,
	session_browser varchar(150) DEFAULT '' NOT NULL,
	session_forwarded_for varchar(255) DEFAULT '' NOT NULL,
	session_page varchar(255) DEFAULT '' NOT NULL,
	session_viewonline tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	session_autologin tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	session_admin tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (session_id),
	KEY session_time (session_time),
	KEY session_user_id (session_user_id),
	KEY session_forum_id (session_forum_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_sessions_keys'
CREATE TABLE phpbb_sessions_keys (
	key_id char(32) DEFAULT '' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	last_ip varchar(40) DEFAULT '' NOT NULL,
	last_login int(11) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (key_id, user_id),
	KEY last_login (last_login)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_sitelist'
CREATE TABLE phpbb_sitelist (
	site_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	site_ip varchar(40) DEFAULT '' NOT NULL,
	site_hostname varchar(255) DEFAULT '' NOT NULL,
	ip_exclude tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (site_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_smilies'
CREATE TABLE phpbb_smilies (
	smiley_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	code varchar(50) DEFAULT '' NOT NULL,
	emotion varchar(50) DEFAULT '' NOT NULL,
	smiley_url varchar(50) DEFAULT '' NOT NULL,
	smiley_width smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	smiley_height smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	smiley_order mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	display_on_posting tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	PRIMARY KEY (smiley_id),
	KEY display_on_post (display_on_posting)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles'
CREATE TABLE phpbb_styles (
	style_id smallint(4) UNSIGNED NOT NULL auto_increment,
	style_name varchar(255) DEFAULT '' NOT NULL,
	style_copyright varchar(255) DEFAULT '' NOT NULL,
	style_active tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	template_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	theme_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	imageset_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (style_id),
	UNIQUE style_name (style_name),
	KEY template_id (template_id),
	KEY theme_id (theme_id),
	KEY imageset_id (imageset_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles_template'
CREATE TABLE phpbb_styles_template (
	template_id smallint(4) UNSIGNED NOT NULL auto_increment,
	template_name varchar(255) DEFAULT '' NOT NULL,
	template_copyright varchar(255) DEFAULT '' NOT NULL,
	template_path varchar(100) DEFAULT '' NOT NULL,
	bbcode_bitfield varchar(255) DEFAULT 'kNg=' NOT NULL,
	template_storedb tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (template_id),
	UNIQUE tmplte_nm (template_name)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles_template_data'
CREATE TABLE phpbb_styles_template_data (
	template_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	template_filename varchar(100) DEFAULT '' NOT NULL,
	template_included text NOT NULL,
	template_mtime int(11) UNSIGNED DEFAULT '0' NOT NULL,
	template_data mediumtext NOT NULL,
	KEY tid (template_id),
	KEY tfn (template_filename)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles_theme'
CREATE TABLE phpbb_styles_theme (
	theme_id smallint(4) UNSIGNED NOT NULL auto_increment,
	theme_name varchar(255) DEFAULT '' NOT NULL,
	theme_copyright varchar(255) DEFAULT '' NOT NULL,
	theme_path varchar(100) DEFAULT '' NOT NULL,
	theme_storedb tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	theme_mtime int(11) UNSIGNED DEFAULT '0' NOT NULL,
	theme_data mediumtext NOT NULL,
	PRIMARY KEY (theme_id),
	UNIQUE theme_name (theme_name)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles_imageset'
CREATE TABLE phpbb_styles_imageset (
	imageset_id smallint(4) UNSIGNED NOT NULL auto_increment,
	imageset_name varchar(255) DEFAULT '' NOT NULL,
	imageset_copyright varchar(255) DEFAULT '' NOT NULL,
	imageset_path varchar(100) DEFAULT '' NOT NULL,
	PRIMARY KEY (imageset_id),
	UNIQUE imgset_nm (imageset_name)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_styles_imageset_data'
CREATE TABLE phpbb_styles_imageset_data (
	image_id smallint(4) UNSIGNED NOT NULL auto_increment,
	image_name varchar(200) DEFAULT '' NOT NULL,
	image_filename varchar(200) DEFAULT '' NOT NULL,
	image_lang varchar(30) DEFAULT '' NOT NULL,
	image_height smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	image_width smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	imageset_id smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (image_id),
	KEY i_d (imageset_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_topics'
CREATE TABLE phpbb_topics (
	topic_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	icon_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_attachment tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	topic_approved tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	topic_reported tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	topic_title varchar(100) DEFAULT '' NOT NULL COLLATE utf8_unicode_ci,
	topic_poster mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	topic_time_limit int(11) UNSIGNED DEFAULT '0' NOT NULL,
	topic_views mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_replies mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_replies_real mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_status tinyint(3) DEFAULT '0' NOT NULL,
	topic_type tinyint(3) DEFAULT '0' NOT NULL,
	topic_first_post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_first_poster_name varchar(255) DEFAULT '' NOT NULL,
	topic_first_poster_colour varchar(6) DEFAULT '' NOT NULL,
	topic_last_post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_last_poster_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_last_poster_name varchar(255) DEFAULT '' NOT NULL,
	topic_last_poster_colour varchar(6) DEFAULT '' NOT NULL,
	topic_last_post_subject varchar(100) DEFAULT '' NOT NULL,
	topic_last_post_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	topic_last_view_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	topic_moved_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_bumped tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	topic_bumper mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	poll_title varchar(255) DEFAULT '' NOT NULL,
	poll_start int(11) UNSIGNED DEFAULT '0' NOT NULL,
	poll_length int(11) UNSIGNED DEFAULT '0' NOT NULL,
	poll_max_options tinyint(4) DEFAULT '1' NOT NULL,
	poll_last_vote int(11) UNSIGNED DEFAULT '0' NOT NULL,
	poll_vote_change tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (topic_id),
	KEY forum_id (forum_id),
	KEY forum_id_type (forum_id, topic_type),
	KEY last_post_time (topic_last_post_time),
	KEY topic_approved (topic_approved),
	KEY forum_appr_last (forum_id, topic_approved, topic_last_post_id),
	KEY fid_time_moved (forum_id, topic_last_post_time, topic_moved_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_topics_track'
CREATE TABLE phpbb_topics_track (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	forum_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	mark_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (user_id, topic_id),
	KEY forum_id (forum_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_topics_posted'
CREATE TABLE phpbb_topics_posted (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	topic_posted tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (user_id, topic_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_topics_watch'
CREATE TABLE phpbb_topics_watch (
	topic_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	notify_status tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	KEY topic_id (topic_id),
	KEY user_id (user_id),
	KEY notify_stat (notify_status)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_user_group'
CREATE TABLE phpbb_user_group (
	group_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	group_leader tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	user_pending tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	KEY group_id (group_id),
	KEY user_id (user_id),
	KEY group_leader (group_leader)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_users'
CREATE TABLE phpbb_users (
	user_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	user_type tinyint(2) DEFAULT '0' NOT NULL,
	group_id mediumint(8) UNSIGNED DEFAULT '3' NOT NULL,
	user_permissions mediumtext NOT NULL,
	user_perm_from mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_ip varchar(40) DEFAULT '' NOT NULL,
	user_regdate int(11) UNSIGNED DEFAULT '0' NOT NULL,
	username varchar(255) DEFAULT '' NOT NULL,
	username_clean varchar(255) DEFAULT '' NOT NULL,
	user_password varchar(40) DEFAULT '' NOT NULL,
	user_passchg int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_pass_convert tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	user_email varchar(100) DEFAULT '' NOT NULL,
	user_email_hash bigint(20) DEFAULT '0' NOT NULL,
	user_birthday varchar(10) DEFAULT '' NOT NULL,
	user_lastvisit int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_lastmark int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_lastpost_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_lastpage varchar(200) DEFAULT '' NOT NULL,
	user_last_confirm_key varchar(10) DEFAULT '' NOT NULL,
	user_last_search int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_warnings tinyint(4) DEFAULT '0' NOT NULL,
	user_last_warning int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_login_attempts tinyint(4) DEFAULT '0' NOT NULL,
	user_inactive_reason tinyint(2) DEFAULT '0' NOT NULL,
	user_inactive_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_posts mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_lang varchar(30) DEFAULT '' NOT NULL,
	user_timezone decimal(5,2) DEFAULT '0' NOT NULL,
	user_dst tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	user_dateformat varchar(30) DEFAULT 'd M Y H:i' NOT NULL,
	user_style smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	user_rank mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	user_colour varchar(6) DEFAULT '' NOT NULL,
	user_new_privmsg int(4) DEFAULT '0' NOT NULL,
	user_unread_privmsg int(4) DEFAULT '0' NOT NULL,
	user_last_privmsg int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_message_rules tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	user_full_folder int(11) DEFAULT '-3' NOT NULL,
	user_emailtime int(11) UNSIGNED DEFAULT '0' NOT NULL,
	user_topic_show_days smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	user_topic_sortby_type varchar(1) DEFAULT 't' NOT NULL,
	user_topic_sortby_dir varchar(1) DEFAULT 'd' NOT NULL,
	user_post_show_days smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	user_post_sortby_type varchar(1) DEFAULT 't' NOT NULL,
	user_post_sortby_dir varchar(1) DEFAULT 'a' NOT NULL,
	user_notify tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	user_notify_pm tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	user_notify_type tinyint(4) DEFAULT '0' NOT NULL,
	user_allow_pm tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	user_allow_viewonline tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	user_allow_viewemail tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	user_allow_massemail tinyint(1) UNSIGNED DEFAULT '1' NOT NULL,
	user_options int(11) UNSIGNED DEFAULT '895' NOT NULL,
	user_avatar varchar(255) DEFAULT '' NOT NULL,
	user_avatar_type tinyint(2) DEFAULT '0' NOT NULL,
	user_avatar_width smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	user_avatar_height smallint(4) UNSIGNED DEFAULT '0' NOT NULL,
	user_sig mediumtext NOT NULL,
	user_sig_bbcode_uid varchar(8) DEFAULT '' NOT NULL,
	user_sig_bbcode_bitfield varchar(255) DEFAULT '' NOT NULL,
	user_from varchar(100) DEFAULT '' NOT NULL,
	user_icq varchar(15) DEFAULT '' NOT NULL,
	user_aim varchar(255) DEFAULT '' NOT NULL,
	user_yim varchar(255) DEFAULT '' NOT NULL,
	user_msnm varchar(255) DEFAULT '' NOT NULL,
	user_jabber varchar(255) DEFAULT '' NOT NULL,
	user_website varchar(200) DEFAULT '' NOT NULL,
	user_occ text NOT NULL,
	user_interests text NOT NULL,
	user_actkey varchar(32) DEFAULT '' NOT NULL,
	user_newpasswd varchar(40) DEFAULT '' NOT NULL,
	user_form_salt varchar(32) DEFAULT '' NOT NULL,
	PRIMARY KEY (user_id),
	KEY user_birthday (user_birthday),
	KEY user_email_hash (user_email_hash),
	KEY user_type (user_type),
	UNIQUE username_clean (username_clean)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_warnings'
CREATE TABLE phpbb_warnings (
	warning_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	post_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	log_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	warning_time int(11) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (warning_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_words'
CREATE TABLE phpbb_words (
	word_id mediumint(8) UNSIGNED NOT NULL auto_increment,
	word varchar(255) DEFAULT '' NOT NULL,
	replacement varchar(255) DEFAULT '' NOT NULL,
	PRIMARY KEY (word_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


# Table: 'phpbb_zebra'
CREATE TABLE phpbb_zebra (
	user_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	zebra_id mediumint(8) UNSIGNED DEFAULT '0' NOT NULL,
	friend tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	foe tinyint(1) UNSIGNED DEFAULT '0' NOT NULL,
	PRIMARY KEY (user_id, zebra_id)
) CHARACTER SET `utf8` COLLATE `utf8_bin`;


