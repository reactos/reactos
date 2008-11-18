/*

 $Id: mssql_schema.sql 8456 2008-03-22 12:31:17Z Kellanved $

*/

BEGIN TRANSACTION
GO

/*
	Table: 'phpbb_attachments'
*/
CREATE TABLE [phpbb_attachments] (
	[attach_id] [int] IDENTITY (1, 1) NOT NULL ,
	[post_msg_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[in_message] [int] DEFAULT (0) NOT NULL ,
	[poster_id] [int] DEFAULT (0) NOT NULL ,
	[is_orphan] [int] DEFAULT (1) NOT NULL ,
	[physical_filename] [varchar] (255) DEFAULT ('') NOT NULL ,
	[real_filename] [varchar] (255) DEFAULT ('') NOT NULL ,
	[download_count] [int] DEFAULT (0) NOT NULL ,
	[attach_comment] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[extension] [varchar] (100) DEFAULT ('') NOT NULL ,
	[mimetype] [varchar] (100) DEFAULT ('') NOT NULL ,
	[filesize] [int] DEFAULT (0) NOT NULL ,
	[filetime] [int] DEFAULT (0) NOT NULL ,
	[thumbnail] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_attachments] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_attachments] PRIMARY KEY  CLUSTERED 
	(
		[attach_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [filetime] ON [phpbb_attachments]([filetime]) ON [PRIMARY]
GO

CREATE  INDEX [post_msg_id] ON [phpbb_attachments]([post_msg_id]) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_attachments]([topic_id]) ON [PRIMARY]
GO

CREATE  INDEX [poster_id] ON [phpbb_attachments]([poster_id]) ON [PRIMARY]
GO

CREATE  INDEX [is_orphan] ON [phpbb_attachments]([is_orphan]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_acl_groups'
*/
CREATE TABLE [phpbb_acl_groups] (
	[group_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[auth_option_id] [int] DEFAULT (0) NOT NULL ,
	[auth_role_id] [int] DEFAULT (0) NOT NULL ,
	[auth_setting] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [group_id] ON [phpbb_acl_groups]([group_id]) ON [PRIMARY]
GO

CREATE  INDEX [auth_opt_id] ON [phpbb_acl_groups]([auth_option_id]) ON [PRIMARY]
GO

CREATE  INDEX [auth_role_id] ON [phpbb_acl_groups]([auth_role_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_acl_options'
*/
CREATE TABLE [phpbb_acl_options] (
	[auth_option_id] [int] IDENTITY (1, 1) NOT NULL ,
	[auth_option] [varchar] (50) DEFAULT ('') NOT NULL ,
	[is_global] [int] DEFAULT (0) NOT NULL ,
	[is_local] [int] DEFAULT (0) NOT NULL ,
	[founder_only] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_acl_options] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_acl_options] PRIMARY KEY  CLUSTERED 
	(
		[auth_option_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [auth_option] ON [phpbb_acl_options]([auth_option]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_acl_roles'
*/
CREATE TABLE [phpbb_acl_roles] (
	[role_id] [int] IDENTITY (1, 1) NOT NULL ,
	[role_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[role_description] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[role_type] [varchar] (10) DEFAULT ('') NOT NULL ,
	[role_order] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_acl_roles] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_acl_roles] PRIMARY KEY  CLUSTERED 
	(
		[role_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [role_type] ON [phpbb_acl_roles]([role_type]) ON [PRIMARY]
GO

CREATE  INDEX [role_order] ON [phpbb_acl_roles]([role_order]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_acl_roles_data'
*/
CREATE TABLE [phpbb_acl_roles_data] (
	[role_id] [int] DEFAULT (0) NOT NULL ,
	[auth_option_id] [int] DEFAULT (0) NOT NULL ,
	[auth_setting] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_acl_roles_data] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_acl_roles_data] PRIMARY KEY  CLUSTERED 
	(
		[role_id],
		[auth_option_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [ath_op_id] ON [phpbb_acl_roles_data]([auth_option_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_acl_users'
*/
CREATE TABLE [phpbb_acl_users] (
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[auth_option_id] [int] DEFAULT (0) NOT NULL ,
	[auth_role_id] [int] DEFAULT (0) NOT NULL ,
	[auth_setting] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [user_id] ON [phpbb_acl_users]([user_id]) ON [PRIMARY]
GO

CREATE  INDEX [auth_option_id] ON [phpbb_acl_users]([auth_option_id]) ON [PRIMARY]
GO

CREATE  INDEX [auth_role_id] ON [phpbb_acl_users]([auth_role_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_banlist'
*/
CREATE TABLE [phpbb_banlist] (
	[ban_id] [int] IDENTITY (1, 1) NOT NULL ,
	[ban_userid] [int] DEFAULT (0) NOT NULL ,
	[ban_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[ban_email] [varchar] (100) DEFAULT ('') NOT NULL ,
	[ban_start] [int] DEFAULT (0) NOT NULL ,
	[ban_end] [int] DEFAULT (0) NOT NULL ,
	[ban_exclude] [int] DEFAULT (0) NOT NULL ,
	[ban_reason] [varchar] (255) DEFAULT ('') NOT NULL ,
	[ban_give_reason] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_banlist] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_banlist] PRIMARY KEY  CLUSTERED 
	(
		[ban_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [ban_end] ON [phpbb_banlist]([ban_end]) ON [PRIMARY]
GO

CREATE  INDEX [ban_user] ON [phpbb_banlist]([ban_userid], [ban_exclude]) ON [PRIMARY]
GO

CREATE  INDEX [ban_email] ON [phpbb_banlist]([ban_email], [ban_exclude]) ON [PRIMARY]
GO

CREATE  INDEX [ban_ip] ON [phpbb_banlist]([ban_ip], [ban_exclude]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_bbcodes'
*/
CREATE TABLE [phpbb_bbcodes] (
	[bbcode_id] [int] DEFAULT (0) NOT NULL ,
	[bbcode_tag] [varchar] (16) DEFAULT ('') NOT NULL ,
	[bbcode_helpline] [varchar] (255) DEFAULT ('') NOT NULL ,
	[display_on_posting] [int] DEFAULT (0) NOT NULL ,
	[bbcode_match] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[bbcode_tpl] [text] DEFAULT ('') NOT NULL ,
	[first_pass_match] [text] DEFAULT ('') NOT NULL ,
	[first_pass_replace] [text] DEFAULT ('') NOT NULL ,
	[second_pass_match] [text] DEFAULT ('') NOT NULL ,
	[second_pass_replace] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_bbcodes] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_bbcodes] PRIMARY KEY  CLUSTERED 
	(
		[bbcode_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [display_on_post] ON [phpbb_bbcodes]([display_on_posting]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_bookmarks'
*/
CREATE TABLE [phpbb_bookmarks] (
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_bookmarks] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_bookmarks] PRIMARY KEY  CLUSTERED 
	(
		[topic_id],
		[user_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_bots'
*/
CREATE TABLE [phpbb_bots] (
	[bot_id] [int] IDENTITY (1, 1) NOT NULL ,
	[bot_active] [int] DEFAULT (1) NOT NULL ,
	[bot_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[bot_agent] [varchar] (255) DEFAULT ('') NOT NULL ,
	[bot_ip] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_bots] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_bots] PRIMARY KEY  CLUSTERED 
	(
		[bot_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [bot_active] ON [phpbb_bots]([bot_active]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_config'
*/
CREATE TABLE [phpbb_config] (
	[config_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[config_value] [varchar] (255) DEFAULT ('') NOT NULL ,
	[is_dynamic] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_config] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_config] PRIMARY KEY  CLUSTERED 
	(
		[config_name]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [is_dynamic] ON [phpbb_config]([is_dynamic]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_confirm'
*/
CREATE TABLE [phpbb_confirm] (
	[confirm_id] [char] (32) DEFAULT ('') NOT NULL ,
	[session_id] [char] (32) DEFAULT ('') NOT NULL ,
	[confirm_type] [int] DEFAULT (0) NOT NULL ,
	[code] [varchar] (8) DEFAULT ('') NOT NULL ,
	[seed] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_confirm] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_confirm] PRIMARY KEY  CLUSTERED 
	(
		[session_id],
		[confirm_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [confirm_type] ON [phpbb_confirm]([confirm_type]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_disallow'
*/
CREATE TABLE [phpbb_disallow] (
	[disallow_id] [int] IDENTITY (1, 1) NOT NULL ,
	[disallow_username] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_disallow] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_disallow] PRIMARY KEY  CLUSTERED 
	(
		[disallow_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_drafts'
*/
CREATE TABLE [phpbb_drafts] (
	[draft_id] [int] IDENTITY (1, 1) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[save_time] [int] DEFAULT (0) NOT NULL ,
	[draft_subject] [varchar] (100) DEFAULT ('') NOT NULL ,
	[draft_message] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_drafts] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_drafts] PRIMARY KEY  CLUSTERED 
	(
		[draft_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [save_time] ON [phpbb_drafts]([save_time]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_extensions'
*/
CREATE TABLE [phpbb_extensions] (
	[extension_id] [int] IDENTITY (1, 1) NOT NULL ,
	[group_id] [int] DEFAULT (0) NOT NULL ,
	[extension] [varchar] (100) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_extensions] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_extensions] PRIMARY KEY  CLUSTERED 
	(
		[extension_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_extension_groups'
*/
CREATE TABLE [phpbb_extension_groups] (
	[group_id] [int] IDENTITY (1, 1) NOT NULL ,
	[group_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[cat_id] [int] DEFAULT (0) NOT NULL ,
	[allow_group] [int] DEFAULT (0) NOT NULL ,
	[download_mode] [int] DEFAULT (1) NOT NULL ,
	[upload_icon] [varchar] (255) DEFAULT ('') NOT NULL ,
	[max_filesize] [int] DEFAULT (0) NOT NULL ,
	[allowed_forums] [varchar] (8000) DEFAULT ('') NOT NULL ,
	[allow_in_pm] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_extension_groups] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_extension_groups] PRIMARY KEY  CLUSTERED 
	(
		[group_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_forums'
*/
CREATE TABLE [phpbb_forums] (
	[forum_id] [int] IDENTITY (1, 1) NOT NULL ,
	[parent_id] [int] DEFAULT (0) NOT NULL ,
	[left_id] [int] DEFAULT (0) NOT NULL ,
	[right_id] [int] DEFAULT (0) NOT NULL ,
	[forum_parents] [text] DEFAULT ('') NOT NULL ,
	[forum_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_desc] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[forum_desc_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_desc_options] [int] DEFAULT (7) NOT NULL ,
	[forum_desc_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[forum_link] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_password] [varchar] (40) DEFAULT ('') NOT NULL ,
	[forum_style] [int] DEFAULT (0) NOT NULL ,
	[forum_image] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_rules] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[forum_rules_link] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_rules_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_rules_options] [int] DEFAULT (7) NOT NULL ,
	[forum_rules_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[forum_topics_per_page] [int] DEFAULT (0) NOT NULL ,
	[forum_type] [int] DEFAULT (0) NOT NULL ,
	[forum_status] [int] DEFAULT (0) NOT NULL ,
	[forum_posts] [int] DEFAULT (0) NOT NULL ,
	[forum_topics] [int] DEFAULT (0) NOT NULL ,
	[forum_topics_real] [int] DEFAULT (0) NOT NULL ,
	[forum_last_post_id] [int] DEFAULT (0) NOT NULL ,
	[forum_last_poster_id] [int] DEFAULT (0) NOT NULL ,
	[forum_last_post_subject] [varchar] (100) DEFAULT ('') NOT NULL ,
	[forum_last_post_time] [int] DEFAULT (0) NOT NULL ,
	[forum_last_poster_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[forum_last_poster_colour] [varchar] (6) DEFAULT ('') NOT NULL ,
	[forum_flags] [int] DEFAULT (32) NOT NULL ,
	[display_subforum_list] [int] DEFAULT (1) NOT NULL ,
	[display_on_index] [int] DEFAULT (1) NOT NULL ,
	[enable_indexing] [int] DEFAULT (1) NOT NULL ,
	[enable_icons] [int] DEFAULT (1) NOT NULL ,
	[enable_prune] [int] DEFAULT (0) NOT NULL ,
	[prune_next] [int] DEFAULT (0) NOT NULL ,
	[prune_days] [int] DEFAULT (0) NOT NULL ,
	[prune_viewed] [int] DEFAULT (0) NOT NULL ,
	[prune_freq] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_forums] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_forums] PRIMARY KEY  CLUSTERED 
	(
		[forum_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [left_right_id] ON [phpbb_forums]([left_id], [right_id]) ON [PRIMARY]
GO

CREATE  INDEX [forum_lastpost_id] ON [phpbb_forums]([forum_last_post_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_forums_access'
*/
CREATE TABLE [phpbb_forums_access] (
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[session_id] [char] (32) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_forums_access] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_forums_access] PRIMARY KEY  CLUSTERED 
	(
		[forum_id],
		[user_id],
		[session_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_forums_track'
*/
CREATE TABLE [phpbb_forums_track] (
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[mark_time] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_forums_track] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_forums_track] PRIMARY KEY  CLUSTERED 
	(
		[user_id],
		[forum_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_forums_watch'
*/
CREATE TABLE [phpbb_forums_watch] (
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[notify_status] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [forum_id] ON [phpbb_forums_watch]([forum_id]) ON [PRIMARY]
GO

CREATE  INDEX [user_id] ON [phpbb_forums_watch]([user_id]) ON [PRIMARY]
GO

CREATE  INDEX [notify_stat] ON [phpbb_forums_watch]([notify_status]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_groups'
*/
CREATE TABLE [phpbb_groups] (
	[group_id] [int] IDENTITY (1, 1) NOT NULL ,
	[group_type] [int] DEFAULT (1) NOT NULL ,
	[group_founder_manage] [int] DEFAULT (0) NOT NULL ,
	[group_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[group_desc] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[group_desc_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[group_desc_options] [int] DEFAULT (7) NOT NULL ,
	[group_desc_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[group_display] [int] DEFAULT (0) NOT NULL ,
	[group_avatar] [varchar] (255) DEFAULT ('') NOT NULL ,
	[group_avatar_type] [int] DEFAULT (0) NOT NULL ,
	[group_avatar_width] [int] DEFAULT (0) NOT NULL ,
	[group_avatar_height] [int] DEFAULT (0) NOT NULL ,
	[group_rank] [int] DEFAULT (0) NOT NULL ,
	[group_colour] [varchar] (6) DEFAULT ('') NOT NULL ,
	[group_sig_chars] [int] DEFAULT (0) NOT NULL ,
	[group_receive_pm] [int] DEFAULT (0) NOT NULL ,
	[group_message_limit] [int] DEFAULT (0) NOT NULL ,
	[group_legend] [int] DEFAULT (1) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_groups] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_groups] PRIMARY KEY  CLUSTERED 
	(
		[group_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [group_legend_name] ON [phpbb_groups]([group_legend], [group_name]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_icons'
*/
CREATE TABLE [phpbb_icons] (
	[icons_id] [int] IDENTITY (1, 1) NOT NULL ,
	[icons_url] [varchar] (255) DEFAULT ('') NOT NULL ,
	[icons_width] [int] DEFAULT (0) NOT NULL ,
	[icons_height] [int] DEFAULT (0) NOT NULL ,
	[icons_order] [int] DEFAULT (0) NOT NULL ,
	[display_on_posting] [int] DEFAULT (1) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_icons] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_icons] PRIMARY KEY  CLUSTERED 
	(
		[icons_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [display_on_posting] ON [phpbb_icons]([display_on_posting]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_lang'
*/
CREATE TABLE [phpbb_lang] (
	[lang_id] [int] IDENTITY (1, 1) NOT NULL ,
	[lang_iso] [varchar] (30) DEFAULT ('') NOT NULL ,
	[lang_dir] [varchar] (30) DEFAULT ('') NOT NULL ,
	[lang_english_name] [varchar] (100) DEFAULT ('') NOT NULL ,
	[lang_local_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[lang_author] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_lang] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_lang] PRIMARY KEY  CLUSTERED 
	(
		[lang_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [lang_iso] ON [phpbb_lang]([lang_iso]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_log'
*/
CREATE TABLE [phpbb_log] (
	[log_id] [int] IDENTITY (1, 1) NOT NULL ,
	[log_type] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[reportee_id] [int] DEFAULT (0) NOT NULL ,
	[log_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[log_time] [int] DEFAULT (0) NOT NULL ,
	[log_operation] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[log_data] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_log] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_log] PRIMARY KEY  CLUSTERED 
	(
		[log_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [log_type] ON [phpbb_log]([log_type]) ON [PRIMARY]
GO

CREATE  INDEX [forum_id] ON [phpbb_log]([forum_id]) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_log]([topic_id]) ON [PRIMARY]
GO

CREATE  INDEX [reportee_id] ON [phpbb_log]([reportee_id]) ON [PRIMARY]
GO

CREATE  INDEX [user_id] ON [phpbb_log]([user_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_moderator_cache'
*/
CREATE TABLE [phpbb_moderator_cache] (
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[username] [varchar] (255) DEFAULT ('') NOT NULL ,
	[group_id] [int] DEFAULT (0) NOT NULL ,
	[group_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[display_on_index] [int] DEFAULT (1) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [disp_idx] ON [phpbb_moderator_cache]([display_on_index]) ON [PRIMARY]
GO

CREATE  INDEX [forum_id] ON [phpbb_moderator_cache]([forum_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_modules'
*/
CREATE TABLE [phpbb_modules] (
	[module_id] [int] IDENTITY (1, 1) NOT NULL ,
	[module_enabled] [int] DEFAULT (1) NOT NULL ,
	[module_display] [int] DEFAULT (1) NOT NULL ,
	[module_basename] [varchar] (255) DEFAULT ('') NOT NULL ,
	[module_class] [varchar] (10) DEFAULT ('') NOT NULL ,
	[parent_id] [int] DEFAULT (0) NOT NULL ,
	[left_id] [int] DEFAULT (0) NOT NULL ,
	[right_id] [int] DEFAULT (0) NOT NULL ,
	[module_langname] [varchar] (255) DEFAULT ('') NOT NULL ,
	[module_mode] [varchar] (255) DEFAULT ('') NOT NULL ,
	[module_auth] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_modules] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_modules] PRIMARY KEY  CLUSTERED 
	(
		[module_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [left_right_id] ON [phpbb_modules]([left_id], [right_id]) ON [PRIMARY]
GO

CREATE  INDEX [module_enabled] ON [phpbb_modules]([module_enabled]) ON [PRIMARY]
GO

CREATE  INDEX [class_left_id] ON [phpbb_modules]([module_class], [left_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_poll_options'
*/
CREATE TABLE [phpbb_poll_options] (
	[poll_option_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[poll_option_text] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[poll_option_total] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [poll_opt_id] ON [phpbb_poll_options]([poll_option_id]) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_poll_options]([topic_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_poll_votes'
*/
CREATE TABLE [phpbb_poll_votes] (
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[poll_option_id] [int] DEFAULT (0) NOT NULL ,
	[vote_user_id] [int] DEFAULT (0) NOT NULL ,
	[vote_user_ip] [varchar] (40) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_poll_votes]([topic_id]) ON [PRIMARY]
GO

CREATE  INDEX [vote_user_id] ON [phpbb_poll_votes]([vote_user_id]) ON [PRIMARY]
GO

CREATE  INDEX [vote_user_ip] ON [phpbb_poll_votes]([vote_user_ip]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_posts'
*/
CREATE TABLE [phpbb_posts] (
	[post_id] [int] IDENTITY (1, 1) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[poster_id] [int] DEFAULT (0) NOT NULL ,
	[icon_id] [int] DEFAULT (0) NOT NULL ,
	[poster_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[post_time] [int] DEFAULT (0) NOT NULL ,
	[post_approved] [int] DEFAULT (1) NOT NULL ,
	[post_reported] [int] DEFAULT (0) NOT NULL ,
	[enable_bbcode] [int] DEFAULT (1) NOT NULL ,
	[enable_smilies] [int] DEFAULT (1) NOT NULL ,
	[enable_magic_url] [int] DEFAULT (1) NOT NULL ,
	[enable_sig] [int] DEFAULT (1) NOT NULL ,
	[post_username] [varchar] (255) DEFAULT ('') NOT NULL ,
	[post_subject] [varchar] (100) DEFAULT ('') NOT NULL ,
	[post_text] [text] DEFAULT ('') NOT NULL ,
	[post_checksum] [varchar] (32) DEFAULT ('') NOT NULL ,
	[post_attachment] [int] DEFAULT (0) NOT NULL ,
	[bbcode_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[bbcode_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[post_postcount] [int] DEFAULT (1) NOT NULL ,
	[post_edit_time] [int] DEFAULT (0) NOT NULL ,
	[post_edit_reason] [varchar] (255) DEFAULT ('') NOT NULL ,
	[post_edit_user] [int] DEFAULT (0) NOT NULL ,
	[post_edit_count] [int] DEFAULT (0) NOT NULL ,
	[post_edit_locked] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_posts] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_posts] PRIMARY KEY  CLUSTERED 
	(
		[post_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [forum_id] ON [phpbb_posts]([forum_id]) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_posts]([topic_id]) ON [PRIMARY]
GO

CREATE  INDEX [poster_ip] ON [phpbb_posts]([poster_ip]) ON [PRIMARY]
GO

CREATE  INDEX [poster_id] ON [phpbb_posts]([poster_id]) ON [PRIMARY]
GO

CREATE  INDEX [post_approved] ON [phpbb_posts]([post_approved]) ON [PRIMARY]
GO

CREATE  INDEX [tid_post_time] ON [phpbb_posts]([topic_id], [post_time]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_privmsgs'
*/
CREATE TABLE [phpbb_privmsgs] (
	[msg_id] [int] IDENTITY (1, 1) NOT NULL ,
	[root_level] [int] DEFAULT (0) NOT NULL ,
	[author_id] [int] DEFAULT (0) NOT NULL ,
	[icon_id] [int] DEFAULT (0) NOT NULL ,
	[author_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[message_time] [int] DEFAULT (0) NOT NULL ,
	[enable_bbcode] [int] DEFAULT (1) NOT NULL ,
	[enable_smilies] [int] DEFAULT (1) NOT NULL ,
	[enable_magic_url] [int] DEFAULT (1) NOT NULL ,
	[enable_sig] [int] DEFAULT (1) NOT NULL ,
	[message_subject] [varchar] (100) DEFAULT ('') NOT NULL ,
	[message_text] [text] DEFAULT ('') NOT NULL ,
	[message_edit_reason] [varchar] (255) DEFAULT ('') NOT NULL ,
	[message_edit_user] [int] DEFAULT (0) NOT NULL ,
	[message_attachment] [int] DEFAULT (0) NOT NULL ,
	[bbcode_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[bbcode_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[message_edit_time] [int] DEFAULT (0) NOT NULL ,
	[message_edit_count] [int] DEFAULT (0) NOT NULL ,
	[to_address] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[bcc_address] [varchar] (4000) DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_privmsgs] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_privmsgs] PRIMARY KEY  CLUSTERED 
	(
		[msg_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [author_ip] ON [phpbb_privmsgs]([author_ip]) ON [PRIMARY]
GO

CREATE  INDEX [message_time] ON [phpbb_privmsgs]([message_time]) ON [PRIMARY]
GO

CREATE  INDEX [author_id] ON [phpbb_privmsgs]([author_id]) ON [PRIMARY]
GO

CREATE  INDEX [root_level] ON [phpbb_privmsgs]([root_level]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_privmsgs_folder'
*/
CREATE TABLE [phpbb_privmsgs_folder] (
	[folder_id] [int] IDENTITY (1, 1) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[folder_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[pm_count] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_privmsgs_folder] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_privmsgs_folder] PRIMARY KEY  CLUSTERED 
	(
		[folder_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [user_id] ON [phpbb_privmsgs_folder]([user_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_privmsgs_rules'
*/
CREATE TABLE [phpbb_privmsgs_rules] (
	[rule_id] [int] IDENTITY (1, 1) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[rule_check] [int] DEFAULT (0) NOT NULL ,
	[rule_connection] [int] DEFAULT (0) NOT NULL ,
	[rule_string] [varchar] (255) DEFAULT ('') NOT NULL ,
	[rule_user_id] [int] DEFAULT (0) NOT NULL ,
	[rule_group_id] [int] DEFAULT (0) NOT NULL ,
	[rule_action] [int] DEFAULT (0) NOT NULL ,
	[rule_folder_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_privmsgs_rules] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_privmsgs_rules] PRIMARY KEY  CLUSTERED 
	(
		[rule_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [user_id] ON [phpbb_privmsgs_rules]([user_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_privmsgs_to'
*/
CREATE TABLE [phpbb_privmsgs_to] (
	[msg_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[author_id] [int] DEFAULT (0) NOT NULL ,
	[pm_deleted] [int] DEFAULT (0) NOT NULL ,
	[pm_new] [int] DEFAULT (1) NOT NULL ,
	[pm_unread] [int] DEFAULT (1) NOT NULL ,
	[pm_replied] [int] DEFAULT (0) NOT NULL ,
	[pm_marked] [int] DEFAULT (0) NOT NULL ,
	[pm_forwarded] [int] DEFAULT (0) NOT NULL ,
	[folder_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [msg_id] ON [phpbb_privmsgs_to]([msg_id]) ON [PRIMARY]
GO

CREATE  INDEX [author_id] ON [phpbb_privmsgs_to]([author_id]) ON [PRIMARY]
GO

CREATE  INDEX [usr_flder_id] ON [phpbb_privmsgs_to]([user_id], [folder_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_profile_fields'
*/
CREATE TABLE [phpbb_profile_fields] (
	[field_id] [int] IDENTITY (1, 1) NOT NULL ,
	[field_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[field_type] [int] DEFAULT (0) NOT NULL ,
	[field_ident] [varchar] (20) DEFAULT ('') NOT NULL ,
	[field_length] [varchar] (20) DEFAULT ('') NOT NULL ,
	[field_minlen] [varchar] (255) DEFAULT ('') NOT NULL ,
	[field_maxlen] [varchar] (255) DEFAULT ('') NOT NULL ,
	[field_novalue] [varchar] (255) DEFAULT ('') NOT NULL ,
	[field_default_value] [varchar] (255) DEFAULT ('') NOT NULL ,
	[field_validation] [varchar] (20) DEFAULT ('') NOT NULL ,
	[field_required] [int] DEFAULT (0) NOT NULL ,
	[field_show_on_reg] [int] DEFAULT (0) NOT NULL ,
	[field_hide] [int] DEFAULT (0) NOT NULL ,
	[field_no_view] [int] DEFAULT (0) NOT NULL ,
	[field_active] [int] DEFAULT (0) NOT NULL ,
	[field_order] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_profile_fields] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_profile_fields] PRIMARY KEY  CLUSTERED 
	(
		[field_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [fld_type] ON [phpbb_profile_fields]([field_type]) ON [PRIMARY]
GO

CREATE  INDEX [fld_ordr] ON [phpbb_profile_fields]([field_order]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_profile_fields_data'
*/
CREATE TABLE [phpbb_profile_fields_data] (
	[user_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_profile_fields_data] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_profile_fields_data] PRIMARY KEY  CLUSTERED 
	(
		[user_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_profile_fields_lang'
*/
CREATE TABLE [phpbb_profile_fields_lang] (
	[field_id] [int] DEFAULT (0) NOT NULL ,
	[lang_id] [int] DEFAULT (0) NOT NULL ,
	[option_id] [int] DEFAULT (0) NOT NULL ,
	[field_type] [int] DEFAULT (0) NOT NULL ,
	[lang_value] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_profile_fields_lang] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_profile_fields_lang] PRIMARY KEY  CLUSTERED 
	(
		[field_id],
		[lang_id],
		[option_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_profile_lang'
*/
CREATE TABLE [phpbb_profile_lang] (
	[field_id] [int] DEFAULT (0) NOT NULL ,
	[lang_id] [int] DEFAULT (0) NOT NULL ,
	[lang_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[lang_explain] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[lang_default_value] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_profile_lang] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_profile_lang] PRIMARY KEY  CLUSTERED 
	(
		[field_id],
		[lang_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_ranks'
*/
CREATE TABLE [phpbb_ranks] (
	[rank_id] [int] IDENTITY (1, 1) NOT NULL ,
	[rank_title] [varchar] (255) DEFAULT ('') NOT NULL ,
	[rank_min] [int] DEFAULT (0) NOT NULL ,
	[rank_special] [int] DEFAULT (0) NOT NULL ,
	[rank_image] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_ranks] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_ranks] PRIMARY KEY  CLUSTERED 
	(
		[rank_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_reports'
*/
CREATE TABLE [phpbb_reports] (
	[report_id] [int] IDENTITY (1, 1) NOT NULL ,
	[reason_id] [int] DEFAULT (0) NOT NULL ,
	[post_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[user_notify] [int] DEFAULT (0) NOT NULL ,
	[report_closed] [int] DEFAULT (0) NOT NULL ,
	[report_time] [int] DEFAULT (0) NOT NULL ,
	[report_text] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_reports] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_reports] PRIMARY KEY  CLUSTERED 
	(
		[report_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_reports_reasons'
*/
CREATE TABLE [phpbb_reports_reasons] (
	[reason_id] [int] IDENTITY (1, 1) NOT NULL ,
	[reason_title] [varchar] (255) DEFAULT ('') NOT NULL ,
	[reason_description] [text] DEFAULT ('') NOT NULL ,
	[reason_order] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_reports_reasons] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_reports_reasons] PRIMARY KEY  CLUSTERED 
	(
		[reason_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_search_results'
*/
CREATE TABLE [phpbb_search_results] (
	[search_key] [varchar] (32) DEFAULT ('') NOT NULL ,
	[search_time] [int] DEFAULT (0) NOT NULL ,
	[search_keywords] [text] DEFAULT ('') NOT NULL ,
	[search_authors] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_search_results] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_search_results] PRIMARY KEY  CLUSTERED 
	(
		[search_key]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_search_wordlist'
*/
CREATE TABLE [phpbb_search_wordlist] (
	[word_id] [int] IDENTITY (1, 1) NOT NULL ,
	[word_text] [varchar] (255) DEFAULT ('') NOT NULL ,
	[word_common] [int] DEFAULT (0) NOT NULL ,
	[word_count] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_search_wordlist] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_search_wordlist] PRIMARY KEY  CLUSTERED 
	(
		[word_id]
	)  ON [PRIMARY] 
GO

CREATE  UNIQUE  INDEX [wrd_txt] ON [phpbb_search_wordlist]([word_text]) ON [PRIMARY]
GO

CREATE  INDEX [wrd_cnt] ON [phpbb_search_wordlist]([word_count]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_search_wordmatch'
*/
CREATE TABLE [phpbb_search_wordmatch] (
	[post_id] [int] DEFAULT (0) NOT NULL ,
	[word_id] [int] DEFAULT (0) NOT NULL ,
	[title_match] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  UNIQUE  INDEX [unq_mtch] ON [phpbb_search_wordmatch]([word_id], [post_id], [title_match]) ON [PRIMARY]
GO

CREATE  INDEX [word_id] ON [phpbb_search_wordmatch]([word_id]) ON [PRIMARY]
GO

CREATE  INDEX [post_id] ON [phpbb_search_wordmatch]([post_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_sessions'
*/
CREATE TABLE [phpbb_sessions] (
	[session_id] [char] (32) DEFAULT ('') NOT NULL ,
	[session_user_id] [int] DEFAULT (0) NOT NULL ,
	[session_forum_id] [int] DEFAULT (0) NOT NULL ,
	[session_last_visit] [int] DEFAULT (0) NOT NULL ,
	[session_start] [int] DEFAULT (0) NOT NULL ,
	[session_time] [int] DEFAULT (0) NOT NULL ,
	[session_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[session_browser] [varchar] (150) DEFAULT ('') NOT NULL ,
	[session_forwarded_for] [varchar] (255) DEFAULT ('') NOT NULL ,
	[session_page] [varchar] (255) DEFAULT ('') NOT NULL ,
	[session_viewonline] [int] DEFAULT (1) NOT NULL ,
	[session_autologin] [int] DEFAULT (0) NOT NULL ,
	[session_admin] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_sessions] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_sessions] PRIMARY KEY  CLUSTERED 
	(
		[session_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [session_time] ON [phpbb_sessions]([session_time]) ON [PRIMARY]
GO

CREATE  INDEX [session_user_id] ON [phpbb_sessions]([session_user_id]) ON [PRIMARY]
GO

CREATE  INDEX [session_forum_id] ON [phpbb_sessions]([session_forum_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_sessions_keys'
*/
CREATE TABLE [phpbb_sessions_keys] (
	[key_id] [char] (32) DEFAULT ('') NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[last_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[last_login] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_sessions_keys] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_sessions_keys] PRIMARY KEY  CLUSTERED 
	(
		[key_id],
		[user_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [last_login] ON [phpbb_sessions_keys]([last_login]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_sitelist'
*/
CREATE TABLE [phpbb_sitelist] (
	[site_id] [int] IDENTITY (1, 1) NOT NULL ,
	[site_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[site_hostname] [varchar] (255) DEFAULT ('') NOT NULL ,
	[ip_exclude] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_sitelist] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_sitelist] PRIMARY KEY  CLUSTERED 
	(
		[site_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_smilies'
*/
CREATE TABLE [phpbb_smilies] (
	[smiley_id] [int] IDENTITY (1, 1) NOT NULL ,
	[code] [varchar] (50) DEFAULT ('') NOT NULL ,
	[emotion] [varchar] (50) DEFAULT ('') NOT NULL ,
	[smiley_url] [varchar] (50) DEFAULT ('') NOT NULL ,
	[smiley_width] [int] DEFAULT (0) NOT NULL ,
	[smiley_height] [int] DEFAULT (0) NOT NULL ,
	[smiley_order] [int] DEFAULT (0) NOT NULL ,
	[display_on_posting] [int] DEFAULT (1) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_smilies] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_smilies] PRIMARY KEY  CLUSTERED 
	(
		[smiley_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [display_on_post] ON [phpbb_smilies]([display_on_posting]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles'
*/
CREATE TABLE [phpbb_styles] (
	[style_id] [int] IDENTITY (1, 1) NOT NULL ,
	[style_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[style_copyright] [varchar] (255) DEFAULT ('') NOT NULL ,
	[style_active] [int] DEFAULT (1) NOT NULL ,
	[template_id] [int] DEFAULT (0) NOT NULL ,
	[theme_id] [int] DEFAULT (0) NOT NULL ,
	[imageset_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_styles] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_styles] PRIMARY KEY  CLUSTERED 
	(
		[style_id]
	)  ON [PRIMARY] 
GO

CREATE  UNIQUE  INDEX [style_name] ON [phpbb_styles]([style_name]) ON [PRIMARY]
GO

CREATE  INDEX [template_id] ON [phpbb_styles]([template_id]) ON [PRIMARY]
GO

CREATE  INDEX [theme_id] ON [phpbb_styles]([theme_id]) ON [PRIMARY]
GO

CREATE  INDEX [imageset_id] ON [phpbb_styles]([imageset_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles_template'
*/
CREATE TABLE [phpbb_styles_template] (
	[template_id] [int] IDENTITY (1, 1) NOT NULL ,
	[template_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[template_copyright] [varchar] (255) DEFAULT ('') NOT NULL ,
	[template_path] [varchar] (100) DEFAULT ('') NOT NULL ,
	[bbcode_bitfield] [varchar] (255) DEFAULT ('kNg=') NOT NULL ,
	[template_storedb] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_styles_template] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_styles_template] PRIMARY KEY  CLUSTERED 
	(
		[template_id]
	)  ON [PRIMARY] 
GO

CREATE  UNIQUE  INDEX [tmplte_nm] ON [phpbb_styles_template]([template_name]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles_template_data'
*/
CREATE TABLE [phpbb_styles_template_data] (
	[template_id] [int] DEFAULT (0) NOT NULL ,
	[template_filename] [varchar] (100) DEFAULT ('') NOT NULL ,
	[template_included] [varchar] (8000) DEFAULT ('') NOT NULL ,
	[template_mtime] [int] DEFAULT (0) NOT NULL ,
	[template_data] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

CREATE  INDEX [tid] ON [phpbb_styles_template_data]([template_id]) ON [PRIMARY]
GO

CREATE  INDEX [tfn] ON [phpbb_styles_template_data]([template_filename]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles_theme'
*/
CREATE TABLE [phpbb_styles_theme] (
	[theme_id] [int] IDENTITY (1, 1) NOT NULL ,
	[theme_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[theme_copyright] [varchar] (255) DEFAULT ('') NOT NULL ,
	[theme_path] [varchar] (100) DEFAULT ('') NOT NULL ,
	[theme_storedb] [int] DEFAULT (0) NOT NULL ,
	[theme_mtime] [int] DEFAULT (0) NOT NULL ,
	[theme_data] [text] DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_styles_theme] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_styles_theme] PRIMARY KEY  CLUSTERED 
	(
		[theme_id]
	)  ON [PRIMARY] 
GO

CREATE  UNIQUE  INDEX [theme_name] ON [phpbb_styles_theme]([theme_name]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles_imageset'
*/
CREATE TABLE [phpbb_styles_imageset] (
	[imageset_id] [int] IDENTITY (1, 1) NOT NULL ,
	[imageset_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[imageset_copyright] [varchar] (255) DEFAULT ('') NOT NULL ,
	[imageset_path] [varchar] (100) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_styles_imageset] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_styles_imageset] PRIMARY KEY  CLUSTERED 
	(
		[imageset_id]
	)  ON [PRIMARY] 
GO

CREATE  UNIQUE  INDEX [imgset_nm] ON [phpbb_styles_imageset]([imageset_name]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_styles_imageset_data'
*/
CREATE TABLE [phpbb_styles_imageset_data] (
	[image_id] [int] IDENTITY (1, 1) NOT NULL ,
	[image_name] [varchar] (200) DEFAULT ('') NOT NULL ,
	[image_filename] [varchar] (200) DEFAULT ('') NOT NULL ,
	[image_lang] [varchar] (30) DEFAULT ('') NOT NULL ,
	[image_height] [int] DEFAULT (0) NOT NULL ,
	[image_width] [int] DEFAULT (0) NOT NULL ,
	[imageset_id] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_styles_imageset_data] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_styles_imageset_data] PRIMARY KEY  CLUSTERED 
	(
		[image_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [i_d] ON [phpbb_styles_imageset_data]([imageset_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_topics'
*/
CREATE TABLE [phpbb_topics] (
	[topic_id] [int] IDENTITY (1, 1) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[icon_id] [int] DEFAULT (0) NOT NULL ,
	[topic_attachment] [int] DEFAULT (0) NOT NULL ,
	[topic_approved] [int] DEFAULT (1) NOT NULL ,
	[topic_reported] [int] DEFAULT (0) NOT NULL ,
	[topic_title] [varchar] (100) DEFAULT ('') NOT NULL ,
	[topic_poster] [int] DEFAULT (0) NOT NULL ,
	[topic_time] [int] DEFAULT (0) NOT NULL ,
	[topic_time_limit] [int] DEFAULT (0) NOT NULL ,
	[topic_views] [int] DEFAULT (0) NOT NULL ,
	[topic_replies] [int] DEFAULT (0) NOT NULL ,
	[topic_replies_real] [int] DEFAULT (0) NOT NULL ,
	[topic_status] [int] DEFAULT (0) NOT NULL ,
	[topic_type] [int] DEFAULT (0) NOT NULL ,
	[topic_first_post_id] [int] DEFAULT (0) NOT NULL ,
	[topic_first_poster_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[topic_first_poster_colour] [varchar] (6) DEFAULT ('') NOT NULL ,
	[topic_last_post_id] [int] DEFAULT (0) NOT NULL ,
	[topic_last_poster_id] [int] DEFAULT (0) NOT NULL ,
	[topic_last_poster_name] [varchar] (255) DEFAULT ('') NOT NULL ,
	[topic_last_poster_colour] [varchar] (6) DEFAULT ('') NOT NULL ,
	[topic_last_post_subject] [varchar] (100) DEFAULT ('') NOT NULL ,
	[topic_last_post_time] [int] DEFAULT (0) NOT NULL ,
	[topic_last_view_time] [int] DEFAULT (0) NOT NULL ,
	[topic_moved_id] [int] DEFAULT (0) NOT NULL ,
	[topic_bumped] [int] DEFAULT (0) NOT NULL ,
	[topic_bumper] [int] DEFAULT (0) NOT NULL ,
	[poll_title] [varchar] (255) DEFAULT ('') NOT NULL ,
	[poll_start] [int] DEFAULT (0) NOT NULL ,
	[poll_length] [int] DEFAULT (0) NOT NULL ,
	[poll_max_options] [int] DEFAULT (1) NOT NULL ,
	[poll_last_vote] [int] DEFAULT (0) NOT NULL ,
	[poll_vote_change] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_topics] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_topics] PRIMARY KEY  CLUSTERED 
	(
		[topic_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [forum_id] ON [phpbb_topics]([forum_id]) ON [PRIMARY]
GO

CREATE  INDEX [forum_id_type] ON [phpbb_topics]([forum_id], [topic_type]) ON [PRIMARY]
GO

CREATE  INDEX [last_post_time] ON [phpbb_topics]([topic_last_post_time]) ON [PRIMARY]
GO

CREATE  INDEX [topic_approved] ON [phpbb_topics]([topic_approved]) ON [PRIMARY]
GO

CREATE  INDEX [forum_appr_last] ON [phpbb_topics]([forum_id], [topic_approved], [topic_last_post_id]) ON [PRIMARY]
GO

CREATE  INDEX [fid_time_moved] ON [phpbb_topics]([forum_id], [topic_last_post_time], [topic_moved_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_topics_track'
*/
CREATE TABLE [phpbb_topics_track] (
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[forum_id] [int] DEFAULT (0) NOT NULL ,
	[mark_time] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_topics_track] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_topics_track] PRIMARY KEY  CLUSTERED 
	(
		[user_id],
		[topic_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [forum_id] ON [phpbb_topics_track]([forum_id]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_topics_posted'
*/
CREATE TABLE [phpbb_topics_posted] (
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[topic_posted] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_topics_posted] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_topics_posted] PRIMARY KEY  CLUSTERED 
	(
		[user_id],
		[topic_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_topics_watch'
*/
CREATE TABLE [phpbb_topics_watch] (
	[topic_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[notify_status] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [topic_id] ON [phpbb_topics_watch]([topic_id]) ON [PRIMARY]
GO

CREATE  INDEX [user_id] ON [phpbb_topics_watch]([user_id]) ON [PRIMARY]
GO

CREATE  INDEX [notify_stat] ON [phpbb_topics_watch]([notify_status]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_user_group'
*/
CREATE TABLE [phpbb_user_group] (
	[group_id] [int] DEFAULT (0) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[group_leader] [int] DEFAULT (0) NOT NULL ,
	[user_pending] [int] DEFAULT (1) NOT NULL 
) ON [PRIMARY]
GO

CREATE  INDEX [group_id] ON [phpbb_user_group]([group_id]) ON [PRIMARY]
GO

CREATE  INDEX [user_id] ON [phpbb_user_group]([user_id]) ON [PRIMARY]
GO

CREATE  INDEX [group_leader] ON [phpbb_user_group]([group_leader]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_users'
*/
CREATE TABLE [phpbb_users] (
	[user_id] [int] IDENTITY (1, 1) NOT NULL ,
	[user_type] [int] DEFAULT (0) NOT NULL ,
	[group_id] [int] DEFAULT (3) NOT NULL ,
	[user_permissions] [text] DEFAULT ('') NOT NULL ,
	[user_perm_from] [int] DEFAULT (0) NOT NULL ,
	[user_ip] [varchar] (40) DEFAULT ('') NOT NULL ,
	[user_regdate] [int] DEFAULT (0) NOT NULL ,
	[username] [varchar] (255) DEFAULT ('') NOT NULL ,
	[username_clean] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_password] [varchar] (40) DEFAULT ('') NOT NULL ,
	[user_passchg] [int] DEFAULT (0) NOT NULL ,
	[user_pass_convert] [int] DEFAULT (0) NOT NULL ,
	[user_email] [varchar] (100) DEFAULT ('') NOT NULL ,
	[user_email_hash] [float] DEFAULT (0) NOT NULL ,
	[user_birthday] [varchar] (10) DEFAULT ('') NOT NULL ,
	[user_lastvisit] [int] DEFAULT (0) NOT NULL ,
	[user_lastmark] [int] DEFAULT (0) NOT NULL ,
	[user_lastpost_time] [int] DEFAULT (0) NOT NULL ,
	[user_lastpage] [varchar] (200) DEFAULT ('') NOT NULL ,
	[user_last_confirm_key] [varchar] (10) DEFAULT ('') NOT NULL ,
	[user_last_search] [int] DEFAULT (0) NOT NULL ,
	[user_warnings] [int] DEFAULT (0) NOT NULL ,
	[user_last_warning] [int] DEFAULT (0) NOT NULL ,
	[user_login_attempts] [int] DEFAULT (0) NOT NULL ,
	[user_inactive_reason] [int] DEFAULT (0) NOT NULL ,
	[user_inactive_time] [int] DEFAULT (0) NOT NULL ,
	[user_posts] [int] DEFAULT (0) NOT NULL ,
	[user_lang] [varchar] (30) DEFAULT ('') NOT NULL ,
	[user_timezone] [float] DEFAULT (0) NOT NULL ,
	[user_dst] [int] DEFAULT (0) NOT NULL ,
	[user_dateformat] [varchar] (30) DEFAULT ('d M Y H:i') NOT NULL ,
	[user_style] [int] DEFAULT (0) NOT NULL ,
	[user_rank] [int] DEFAULT (0) NOT NULL ,
	[user_colour] [varchar] (6) DEFAULT ('') NOT NULL ,
	[user_new_privmsg] [int] DEFAULT (0) NOT NULL ,
	[user_unread_privmsg] [int] DEFAULT (0) NOT NULL ,
	[user_last_privmsg] [int] DEFAULT (0) NOT NULL ,
	[user_message_rules] [int] DEFAULT (0) NOT NULL ,
	[user_full_folder] [int] DEFAULT (-3) NOT NULL ,
	[user_emailtime] [int] DEFAULT (0) NOT NULL ,
	[user_topic_show_days] [int] DEFAULT (0) NOT NULL ,
	[user_topic_sortby_type] [varchar] (1) DEFAULT ('t') NOT NULL ,
	[user_topic_sortby_dir] [varchar] (1) DEFAULT ('d') NOT NULL ,
	[user_post_show_days] [int] DEFAULT (0) NOT NULL ,
	[user_post_sortby_type] [varchar] (1) DEFAULT ('t') NOT NULL ,
	[user_post_sortby_dir] [varchar] (1) DEFAULT ('a') NOT NULL ,
	[user_notify] [int] DEFAULT (0) NOT NULL ,
	[user_notify_pm] [int] DEFAULT (1) NOT NULL ,
	[user_notify_type] [int] DEFAULT (0) NOT NULL ,
	[user_allow_pm] [int] DEFAULT (1) NOT NULL ,
	[user_allow_viewonline] [int] DEFAULT (1) NOT NULL ,
	[user_allow_viewemail] [int] DEFAULT (1) NOT NULL ,
	[user_allow_massemail] [int] DEFAULT (1) NOT NULL ,
	[user_options] [int] DEFAULT (895) NOT NULL ,
	[user_avatar] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_avatar_type] [int] DEFAULT (0) NOT NULL ,
	[user_avatar_width] [int] DEFAULT (0) NOT NULL ,
	[user_avatar_height] [int] DEFAULT (0) NOT NULL ,
	[user_sig] [text] DEFAULT ('') NOT NULL ,
	[user_sig_bbcode_uid] [varchar] (8) DEFAULT ('') NOT NULL ,
	[user_sig_bbcode_bitfield] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_from] [varchar] (100) DEFAULT ('') NOT NULL ,
	[user_icq] [varchar] (15) DEFAULT ('') NOT NULL ,
	[user_aim] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_yim] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_msnm] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_jabber] [varchar] (255) DEFAULT ('') NOT NULL ,
	[user_website] [varchar] (200) DEFAULT ('') NOT NULL ,
	[user_occ] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[user_interests] [varchar] (4000) DEFAULT ('') NOT NULL ,
	[user_actkey] [varchar] (32) DEFAULT ('') NOT NULL ,
	[user_newpasswd] [varchar] (40) DEFAULT ('') NOT NULL ,
	[user_form_salt] [varchar] (32) DEFAULT ('') NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]
GO

ALTER TABLE [phpbb_users] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_users] PRIMARY KEY  CLUSTERED 
	(
		[user_id]
	)  ON [PRIMARY] 
GO

CREATE  INDEX [user_birthday] ON [phpbb_users]([user_birthday]) ON [PRIMARY]
GO

CREATE  INDEX [user_email_hash] ON [phpbb_users]([user_email_hash]) ON [PRIMARY]
GO

CREATE  INDEX [user_type] ON [phpbb_users]([user_type]) ON [PRIMARY]
GO

CREATE  UNIQUE  INDEX [username_clean] ON [phpbb_users]([username_clean]) ON [PRIMARY]
GO


/*
	Table: 'phpbb_warnings'
*/
CREATE TABLE [phpbb_warnings] (
	[warning_id] [int] IDENTITY (1, 1) NOT NULL ,
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[post_id] [int] DEFAULT (0) NOT NULL ,
	[log_id] [int] DEFAULT (0) NOT NULL ,
	[warning_time] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_warnings] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_warnings] PRIMARY KEY  CLUSTERED 
	(
		[warning_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_words'
*/
CREATE TABLE [phpbb_words] (
	[word_id] [int] IDENTITY (1, 1) NOT NULL ,
	[word] [varchar] (255) DEFAULT ('') NOT NULL ,
	[replacement] [varchar] (255) DEFAULT ('') NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_words] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_words] PRIMARY KEY  CLUSTERED 
	(
		[word_id]
	)  ON [PRIMARY] 
GO


/*
	Table: 'phpbb_zebra'
*/
CREATE TABLE [phpbb_zebra] (
	[user_id] [int] DEFAULT (0) NOT NULL ,
	[zebra_id] [int] DEFAULT (0) NOT NULL ,
	[friend] [int] DEFAULT (0) NOT NULL ,
	[foe] [int] DEFAULT (0) NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [phpbb_zebra] WITH NOCHECK ADD 
	CONSTRAINT [PK_phpbb_zebra] PRIMARY KEY  CLUSTERED 
	(
		[user_id],
		[zebra_id]
	)  ON [PRIMARY] 
GO



COMMIT
GO

