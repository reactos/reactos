#
# $Id: schema_data.sql 8492 2008-04-07 13:08:42Z acydburn $
#

# POSTGRES BEGIN #

# -- Config
INSERT INTO phpbb_config (config_name, config_value) VALUES ('active_sessions', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_attachments', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_autologin', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_avatar_local', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_avatar_remote', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_avatar_upload', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_bbcode', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_birthdays', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_bookmarks', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_emailreuse', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_forum_notify', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_mass_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_name_chars', 'USERNAME_CHARS_ANY');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_namechange', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_nocensors', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_pm_attach', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_post_flash', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_post_links', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_privmsg', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_bbcode', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_flash', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_img', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_links', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_sig_smilies', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_smilies', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('allow_topic_notify', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('attachment_quota', '52428800');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('auth_bbcode_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('auth_flash_pm', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('auth_img_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('auth_method', 'roscms');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('auth_smilies_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_filesize', '6144');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_gallery_path', 'images/avatars/gallery');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_max_height', '90');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_max_width', '90');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_min_height', '20');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_min_width', '20');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_path', 'images/avatars/upload');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('avatar_salt', 'phpbb_avatar');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_contact', 'contact@yourdomain.tld');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_disable', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_disable_msg', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_dst', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_email', 'address@yourdomain.tld');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_email_form', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_email_sig', '{L_CONFIG_BOARD_EMAIL_SIG}');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_hide_emails', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('board_timezone', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('browser_check', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('bump_interval', '10');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('bump_type', 'd');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('cache_gc', '7200');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('captcha_gd', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('captcha_gd_foreground_noise', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('captcha_gd_x_grid', '25');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('captcha_gd_y_grid', '25');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('check_dnsbl', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('chg_passforce', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('cookie_domain', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('cookie_name', 'phpbb3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('cookie_path', '/');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('cookie_secure', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('coppa_enable', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('coppa_fax', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('coppa_mail', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('database_gc', '604800');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('default_dateformat', 'D M d, Y g:i a');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('default_style', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('display_last_edited', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('display_order', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('edit_time', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('email_check_mx', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('email_enable', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('email_function_name', 'mail');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('email_package_size', '50');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('enable_confirm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('enable_pm_icons', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('enable_post_confirm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('flood_interval', '15');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('force_server_vars', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('form_token_lifetime', '7200');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('form_token_mintime', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('form_token_sid_guests', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('forward_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('forwarded_for_check', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('full_folder_action', '2');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_mysql_max_word_len', '254');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_mysql_min_word_len', '4');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_native_common_thres', '5');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_native_load_upd', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_native_max_chars', '14');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('fulltext_native_min_chars', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('gzip_compress', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('hot_threshold', '25');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('icons_path', 'images/icons');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_create_thumbnail', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_display_inlined', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_imagick', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_link_height', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_link_width', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_max_height', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_max_thumb_width', '400');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_max_width', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('img_min_thumb_filesize', '12000');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ip_check', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_enable', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_host', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_password', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_package_size', '20');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_port', '5222');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_use_ssl', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('jab_username', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_base_dn', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_email', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_password', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_port', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_server', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_uid', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_user', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ldap_user_filter', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('limit_load', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('limit_search_load', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_anon_lastread', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_birthdays', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_cpf_memberlist', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_cpf_viewprofile', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_cpf_viewtopic', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_db_lastread', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_db_track', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_jumpbox', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_moderators', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_online', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_online_guests', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_online_time', '5');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_onlinetrack', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_search', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_tplcompile', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('load_user_activity', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_attachments', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_attachments_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_autologin_time', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_filesize', '262144');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_filesize_pm', '262144');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_login_attempts', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_name_chars', '20');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_pass_chars', '30');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_poll_options', '10');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_chars', '60000');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_font_size', '200');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_img_height', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_img_width', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_smilies', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_post_urls', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_quote_depth', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_reg_attempts', '5');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_chars', '255');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_font_size', '200');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_img_height', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_img_width', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_smilies', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('max_sig_urls', '5');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('min_name_chars', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('min_pass_chars', '6');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('min_search_author_chars', '3');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('min_time_reg', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('min_time_terms', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('override_user_style', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('pass_complex', 'PASS_TYPE_ANY');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('pm_edit_time', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('pm_max_boxes', '4');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('pm_max_msgs', '50');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('posts_per_page', '10');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('print_pm', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('queue_interval', '600');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('ranks_path', 'images/ranks');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('require_activation', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('script_path', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_block_size', '250');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_gc', '7200');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_indexing_state', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_interval', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_anonymous_interval', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_type', 'fulltext_native');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('search_store_results', '1800');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('secure_allow_deny', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('secure_allow_empty_referer', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('secure_downloads', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('server_name', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('server_port', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('server_protocol', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('session_gc', '3600');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('session_length', '3600');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('site_desc', '{L_CONFIG_SITE_DESC}');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('sitename', '{L_CONFIG_SITENAME}');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smilies_path', 'images/smilies');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_auth_method', 'PLAIN');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_delivery', '0');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_host', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_password', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_port', '25');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('smtp_username', '');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('topics_per_page', '25');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('tpl_allow_php', '1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('upload_icons_path', 'images/upload_icons');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('upload_path', 'files');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('version', '3.0.1');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('warnings_expire_days', '90');
INSERT INTO phpbb_config (config_name, config_value) VALUES ('warnings_gc', '14400');

INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('cache_last_gc', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('cron_lock', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('database_last_gc', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('last_queue_run', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('newest_user_colour', 'AA0000', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('newest_user_id', '2', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('newest_username', '', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('num_files', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('num_posts', '1', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('num_topics', '1', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('num_users', '1', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('rand_seed', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('rand_seed_last_update', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('record_online_date', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('record_online_users', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('search_last_gc', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('session_last_gc', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('upload_dir_size', '0', 1);
INSERT INTO phpbb_config (config_name, config_value, is_dynamic) VALUES ('warnings_last_gc', '0', 1);

# -- Forum related auth options
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_announce', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_attach', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_bbcode', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_bump', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_delete', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_download', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_edit', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_email', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_flash', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_icons', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_ignoreflood', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_img', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_list', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_noapprove', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_poll', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_post', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_postcount', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_print', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_read', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_reply', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_report', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_search', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_sigs', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_smilies', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_sticky', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_subscribe', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_user_lock', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_vote', 1);
INSERT INTO phpbb_acl_options (auth_option, is_local) VALUES ('f_votechg', 1);

# -- Moderator related auth options
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_approve', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_chgposter', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_delete', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_edit', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_info', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_lock', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_merge', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_move', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_report', 1, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_split', 1, 1);

# -- Global moderator auth option (not a local option)
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_ban', 0, 1);
INSERT INTO phpbb_acl_options (auth_option, is_local, is_global) VALUES ('m_warn', 0, 1);

# -- Admin related auth options
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_aauth', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_attach', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_authgroups', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_authusers', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_backup', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_ban', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_bbcode', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_board', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_bots', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_clearlogs', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_email', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_fauth', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_forum', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_forumadd', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_forumdel', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_group', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_groupadd', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_groupdel', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_icons', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_jabber', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_language', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_mauth', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_modules', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_names', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_phpinfo', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_profile', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_prune', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_ranks', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_reasons', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_roles', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_search', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_server', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_styles', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_switchperm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_uauth', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_user', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_userdel', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_viewauth', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_viewlogs', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('a_words', 1);

# -- User related auth options
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_attach', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chgavatar', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chgcensors', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chgemail', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chggrp', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chgname', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_chgpasswd', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_download', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_hideonline', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_ignoreflood', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_masspm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_attach', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_bbcode', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_delete', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_download', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_edit', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_emailpm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_flash', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_forward', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_img', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_printpm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_pm_smilies', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_readpm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_savedrafts', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_search', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_sendemail', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_sendim', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_sendpm', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_sig', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_viewonline', 1);
INSERT INTO phpbb_acl_options (auth_option, is_global) VALUES ('u_viewprofile', 1);


# -- standard auth roles
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_ADMIN_STANDARD', 'ROLE_DESCRIPTION_ADMIN_STANDARD', 'a_', 1);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_ADMIN_FORUM', 'ROLE_DESCRIPTION_ADMIN_FORUM', 'a_', 3);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_ADMIN_USERGROUP', 'ROLE_DESCRIPTION_ADMIN_USERGROUP', 'a_', 4);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_ADMIN_FULL', 'ROLE_DESCRIPTION_ADMIN_FULL', 'a_', 2);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_USER_FULL', 'ROLE_DESCRIPTION_USER_FULL', 'u_', 3);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_USER_STANDARD', 'ROLE_DESCRIPTION_USER_STANDARD', 'u_', 1);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_USER_LIMITED', 'ROLE_DESCRIPTION_USER_LIMITED', 'u_', 2);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_USER_NOPM', 'ROLE_DESCRIPTION_USER_NOPM', 'u_', 4);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_USER_NOAVATAR', 'ROLE_DESCRIPTION_USER_NOAVATAR', 'u_', 5);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_MOD_FULL', 'ROLE_DESCRIPTION_MOD_FULL', 'm_', 3);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_MOD_STANDARD', 'ROLE_DESCRIPTION_MOD_STANDARD', 'm_', 1);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_MOD_SIMPLE', 'ROLE_DESCRIPTION_MOD_SIMPLE', 'm_', 2);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_MOD_QUEUE', 'ROLE_DESCRIPTION_MOD_QUEUE', 'm_', 4);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_FULL', 'ROLE_DESCRIPTION_FORUM_FULL', 'f_', 7);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_STANDARD', 'ROLE_DESCRIPTION_FORUM_STANDARD', 'f_', 5);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_NOACCESS', 'ROLE_DESCRIPTION_FORUM_NOACCESS', 'f_', 1);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_READONLY', 'ROLE_DESCRIPTION_FORUM_READONLY', 'f_', 2);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_LIMITED', 'ROLE_DESCRIPTION_FORUM_LIMITED', 'f_', 3);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_BOT', 'ROLE_DESCRIPTION_FORUM_BOT', 'f_', 9);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_ONQUEUE', 'ROLE_DESCRIPTION_FORUM_ONQUEUE', 'f_', 8);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_POLLS', 'ROLE_DESCRIPTION_FORUM_POLLS', 'f_', 6);
INSERT INTO phpbb_acl_roles (role_name, role_description, role_type, role_order) VALUES ('ROLE_FORUM_LIMITED_POLLS', 'ROLE_DESCRIPTION_FORUM_LIMITED_POLLS', 'f_', 4);


# -- phpbb_styles
INSERT INTO phpbb_styles (style_name, style_copyright, style_active, template_id, theme_id, imageset_id) VALUES ('roscms', '&copy; phpBB Group', 1, 1, 1, 1);

# -- phpbb_styles_imageset
INSERT INTO phpbb_styles_imageset (imageset_name, imageset_copyright, imageset_path) VALUES ('roscms', '&copy; phpBB Group', 'roscms');

# -- phpbb_styles_imageset_data
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('site_logo', 'site_logo.gif', '', 52, 139, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_link', 'forum_link.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_read', 'forum_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_read_locked', 'forum_read_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_read_subforum', 'forum_read_subforum.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_unread', 'forum_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_unread_locked', 'forum_unread_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('forum_unread_subforum', 'forum_unread_subforum.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_moved', 'topic_moved.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read', 'topic_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read_mine', 'topic_read_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read_hot', 'topic_read_hot.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read_hot_mine', 'topic_read_hot_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read_locked', 'topic_read_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_read_locked_mine', 'topic_read_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread', 'topic_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread_mine', 'topic_unread_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread_hot', 'topic_unread_hot.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread_hot_mine', 'topic_unread_hot_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread_locked', 'topic_unread_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('topic_unread_locked_mine', 'topic_unread_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_read', 'sticky_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_read_mine', 'sticky_read_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_read_locked', 'sticky_read_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_read_locked_mine', 'sticky_read_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_unread', 'sticky_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_unread_mine', 'sticky_unread_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_unread_locked', 'sticky_unread_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('sticky_unread_locked_mine', 'sticky_unread_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_read', 'announce_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_read_mine', 'announce_read_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_read_locked', 'announce_read_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_read_locked_mine', 'announce_read_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_unread', 'announce_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_unread_mine', 'announce_unread_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_unread_locked', 'announce_unread_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('announce_unread_locked_mine', 'announce_unread_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_read', 'announce_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_read_mine', 'announce_read_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_read_locked', 'announce_read_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_read_locked_mine', 'announce_read_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_unread', 'announce_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_unread_mine', 'announce_unread_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_unread_locked', 'announce_unread_locked.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('global_unread_locked_mine', 'announce_unread_locked_mine.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('pm_read', 'topic_read.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('pm_unread', 'topic_unread.gif', '', 27, 27, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_back_top', 'icon_back_top.gif', '', 11, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_aim', 'icon_contact_aim.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_email', 'icon_contact_email.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_icq', 'icon_contact_icq.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_jabber', 'icon_contact_jabber.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_msnm', 'icon_contact_msnm.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_www', 'icon_contact_www.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_contact_yahoo', 'icon_contact_yahoo.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_post_delete', 'icon_post_delete.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_post_info', 'icon_post_info.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_post_report', 'icon_post_report.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_post_target', 'icon_post_target.gif', '', 9, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_post_target_unread', 'icon_post_target_unread.gif', '', 9, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_topic_attach', 'icon_topic_attach.gif', '', 10, 7, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_topic_latest', 'icon_topic_latest.gif', '', 9, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_topic_newest', 'icon_topic_newest.gif', '', 9, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_topic_reported', 'icon_topic_reported.gif', '', 14, 16, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_topic_unapproved', 'icon_topic_unapproved.gif', '', 14, 16, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('icon_user_warn', 'icon_user_warn.gif', '', 20, 20, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('subforum_read', 'subforum_read.gif', '', 9, 11, 1);
INSERT INTO phpbb_styles_imageset_data (image_name, image_filename, image_lang, image_height, image_width, imageset_id) VALUES ('subforum_unread', 'subforum_unread.gif', '', 9, 11, 1);

# -- phpbb_styles_template
INSERT INTO phpbb_styles_template (template_name, template_copyright, template_path, bbcode_bitfield, template_storedb) VALUES ('roscms', '&copy; phpBB Group', 'roscms', 'lNg=', 0);

# -- phpbb_styles_theme
INSERT INTO phpbb_styles_theme (theme_name, theme_copyright, theme_path, theme_storedb, theme_data) VALUES ('roscms', '&copy; phpBB Group', 'roscms', 1, '');

# -- Forums
INSERT INTO phpbb_forums (forum_name, forum_desc, left_id, right_id, parent_id, forum_type, forum_posts, forum_topics, forum_topics_real, forum_last_post_id, forum_last_poster_id, forum_last_poster_name, forum_last_poster_colour, forum_last_post_time, forum_link, forum_password, forum_image, forum_rules, forum_rules_link, forum_rules_uid, forum_desc_uid, prune_days, prune_viewed, forum_parents) VALUES ('{L_FORUMS_FIRST_CATEGORY}', '', 1, 4, 0, 0, 1, 1, 1, 1, 2, 'Admin', 'AA0000', 972086460, '', '', '', '', '', '', '', 0, 0, '');

INSERT INTO phpbb_forums (forum_name, forum_desc, left_id, right_id, parent_id, forum_type, forum_posts, forum_topics, forum_topics_real, forum_last_post_id, forum_last_poster_id, forum_last_poster_name, forum_last_poster_colour, forum_last_post_subject, forum_last_post_time, forum_link, forum_password, forum_image, forum_rules, forum_rules_link, forum_rules_uid, forum_desc_uid, prune_days, prune_viewed, forum_parents) VALUES ('{L_FORUMS_TEST_FORUM_TITLE}', '{L_FORUMS_TEST_FORUM_DESC}', 2, 3, 1, 1, 1, 1, 1, 1, 2, 'Admin', 'AA0000', '{L_TOPICS_TOPIC_TITLE}', 972086460, '', '', '', '', '', '', '', 0, 0, '');

# -- Users / Anonymous user
INSERT INTO phpbb_users (user_type, group_id, username, username_clean, user_regdate, user_password, user_email, user_lang, user_style, user_rank, user_colour, user_posts, user_permissions, user_ip, user_birthday, user_lastpage, user_last_confirm_key, user_post_sortby_type, user_post_sortby_dir, user_topic_sortby_type, user_topic_sortby_dir, user_avatar, user_sig, user_sig_bbcode_uid, user_from, user_icq, user_aim, user_yim, user_msnm, user_jabber, user_website, user_occ, user_interests, user_actkey, user_newpasswd, user_allow_massemail) VALUES (2, 1, 'Anonymous', 'anonymous', 0, '', '', 'en', 1, 0, '', 0, '', '', '', '', '', 't', 'a', 't', 'd', '', '', '', '', '', '', '', '', '', '', '', '', '', '', 0);

# -- username: Admin    password: admin (change this or remove it once everything is working!)
INSERT INTO phpbb_users (user_type, group_id, username, username_clean, user_regdate, user_password, user_email, user_lang, user_style, user_rank, user_colour, user_posts, user_permissions, user_ip, user_birthday, user_lastpage, user_last_confirm_key, user_post_sortby_type, user_post_sortby_dir, user_topic_sortby_type, user_topic_sortby_dir, user_avatar, user_sig, user_sig_bbcode_uid, user_from, user_icq, user_aim, user_yim, user_msnm, user_jabber, user_website, user_occ, user_interests, user_actkey, user_newpasswd) VALUES (3, 5, 'Admin', 'admin', 0, '21232f297a57a5a743894a0e4a801fc3', 'admin@yourdomain.com', 'en', 1, 1, 'AA0000', 1, '', '', '', '', '', 't', 'a', 't', 'd', '', '', '', '', '', '', '', '', '', '', '', '', '', '');

# -- Groups
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('GUESTS', 3, 0, '', 0, '', '', '');
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('REGISTERED', 3, 0, '', 0, '', '', '');
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('REGISTERED_COPPA', 3, 0, '', 0, '', '', '');
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('GLOBAL_MODERATORS', 3, 0, '00AA00', 1, '', '', '');
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('ADMINISTRATORS', 3, 1, 'AA0000', 1, '', '', '');
INSERT INTO phpbb_groups (group_name, group_type, group_founder_manage, group_colour, group_legend, group_avatar, group_desc, group_desc_uid) VALUES ('BOTS', 3, 0, '9E8DA7', 0, '', '', '');

# -- User -> Group
INSERT INTO phpbb_user_group (group_id, user_id, user_pending, group_leader) VALUES (1, 1, 0, 0);
INSERT INTO phpbb_user_group (group_id, user_id, user_pending, group_leader) VALUES (2, 2, 0, 0);
INSERT INTO phpbb_user_group (group_id, user_id, user_pending, group_leader) VALUES (4, 2, 0, 0);
INSERT INTO phpbb_user_group (group_id, user_id, user_pending, group_leader) VALUES (5, 2, 0, 1);

# -- Ranks
INSERT INTO phpbb_ranks (rank_title, rank_min, rank_special, rank_image) VALUES ('{L_RANKS_SITE_ADMIN_TITLE}', 0, 1, '');

# -- Roles data

# Standard Admin (a_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 1, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'a_%' AND auth_option NOT IN ('a_switchperm', 'a_jabber', 'a_phpinfo', 'a_server', 'a_backup', 'a_styles', 'a_clearlogs', 'a_modules', 'a_language', 'a_email', 'a_bots', 'a_search', 'a_aauth', 'a_roles');

# Forum admin (a_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 2, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'a_%' AND auth_option IN ('a_', 'a_authgroups', 'a_authusers', 'a_fauth', 'a_forum', 'a_forumadd', 'a_forumdel', 'a_mauth', 'a_prune', 'a_uauth', 'a_viewauth', 'a_viewlogs');

# User and Groups Admin (a_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 3, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'a_%' AND auth_option IN ('a_', 'a_authgroups', 'a_authusers', 'a_ban', 'a_group', 'a_groupadd', 'a_groupdel', 'a_ranks', 'a_uauth', 'a_user', 'a_viewauth', 'a_viewlogs');

# Full Admin (a_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 4, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'a_%';

# All Features (u_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 5, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%';

# Standard Features (u_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 6, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option NOT IN ('u_viewonline', 'u_chggrp', 'u_chgname', 'u_ignoreflood', 'u_pm_flash', 'u_pm_forward');

# Limited Features (u_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 7, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option NOT IN ('u_attach', 'u_viewonline', 'u_chggrp', 'u_chgname', 'u_ignoreflood', 'u_pm_attach', 'u_pm_emailpm', 'u_pm_flash', 'u_savedrafts', 'u_search', 'u_sendemail', 'u_sendim');

# No Private Messages (u_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 8, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option IN ('u_', 'u_chgavatar', 'u_chgcensors', 'u_chgemail', 'u_chgpasswd', 'u_download', 'u_hideonline', 'u_sig', 'u_viewprofile');
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 8, auth_option_id, 0 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option IN ('u_readpm', 'u_sendpm', 'u_masspm');

# No Avatar (u_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 9, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option NOT IN ('u_attach', 'u_chgavatar', 'u_viewonline', 'u_chggrp', 'u_chgname', 'u_ignoreflood', 'u_pm_attach', 'u_pm_emailpm', 'u_pm_flash', 'u_savedrafts', 'u_search', 'u_sendemail', 'u_sendim');
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 9, auth_option_id, 0 FROM phpbb_acl_options WHERE auth_option LIKE 'u_%' AND auth_option IN ('u_chgavatar');

# Full Moderator (m_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 10, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'm_%';

# Standard Moderator (m_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 11, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'm_%' AND auth_option NOT IN ('m_ban', 'm_chgposter');

# Simple Moderator (m_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 12, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'm_%' AND auth_option IN ('m_', 'm_delete', 'm_edit', 'm_info', 'm_report');

# Queue Moderator (m_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 13, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'm_%' AND auth_option IN ('m_', 'm_approve', 'm_edit');

# Full Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 14, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%';

# Standard Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 15, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option NOT IN ('f_announce', 'f_flash', 'f_ignoreflood', 'f_poll', 'f_sticky', 'f_user_lock');

# No Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 16, auth_option_id, 0 FROM phpbb_acl_options WHERE auth_option = 'f_';

# Read Only Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 17, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option IN ('f_', 'f_download', 'f_list', 'f_read', 'f_search', 'f_subscribe', 'f_print');

# Limited Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 18, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option NOT IN ('f_announce', 'f_attach', 'f_bump', 'f_delete', 'f_flash', 'f_icons', 'f_ignoreflood', 'f_poll', 'f_sticky', 'f_user_lock', 'f_votechg');

# Bot Access (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 19, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option IN ('f_', 'f_download', 'f_list', 'f_read', 'f_print');

# On Moderation Queue (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 20, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option NOT IN ('f_announce', 'f_bump', 'f_delete', 'f_flash', 'f_icons', 'f_ignoreflood', 'f_poll', 'f_sticky', 'f_user_lock', 'f_votechg', 'f_noapprove');
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 20, auth_option_id, 0 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option IN ('f_noapprove');

# Standard Access + Polls (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 21, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option NOT IN ('f_announce', 'f_flash', 'f_ignoreflood', 'f_sticky', 'f_user_lock');

# Limited Access + Polls (f_)
INSERT INTO phpbb_acl_roles_data (role_id, auth_option_id, auth_setting) SELECT 22, auth_option_id, 1 FROM phpbb_acl_options WHERE auth_option LIKE 'f_%' AND auth_option NOT IN ('f_announce', 'f_attach', 'f_bump', 'f_delete', 'f_flash', 'f_icons', 'f_ignoreflood', 'f_sticky', 'f_user_lock', 'f_votechg');

# Permissions

# GUESTS - u_download and u_search ability
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) SELECT 1, 0, auth_option_id, 0, 1 FROM phpbb_acl_options WHERE auth_option IN ('u_', 'u_download', 'u_search');

# Admin user - full user features
INSERT INTO phpbb_acl_users (user_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (2, 0, 0, 5, 0);

# ADMINISTRATOR Group - full user features
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (5, 0, 0, 5, 0);

# ADMINISTRATOR Group - standard admin
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (5, 0, 0, 1, 0);

# REGISTERED and REGISTERED_COPPA having standard user features
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (2, 0, 0, 6, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (3, 0, 0, 6, 0);

# GLOBAL_MODERATORS having full user features
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (4, 0, 0, 5, 0);

# GLOBAL_MODERATORS having full global moderator access
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (4, 0, 0, 10, 0);

# Giving all groups read only access to the first category
# since administrators and moderators are already within the registered users group we do not need to set them here
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (1, 1, 0, 17, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (2, 1, 0, 17, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (3, 1, 0, 17, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (6, 1, 0, 17, 0);

# Giving access to the first forum

# guests having read only access
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (1, 2, 0, 17, 0);

# registered and registered_coppa having standard access
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (2, 2, 0, 15, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (3, 2, 0, 15, 0);

# global moderators having standard access + polls
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (4, 2, 0, 21, 0);

# administrators having full forum and full moderator access
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (5, 2, 0, 14, 0);
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (5, 2, 0, 10, 0);

# Bots having bot access
INSERT INTO phpbb_acl_groups (group_id, forum_id, auth_option_id, auth_role_id, auth_setting) VALUES (6, 2, 0, 19, 0);


# -- Demo Topic
INSERT INTO phpbb_topics (topic_title, topic_poster, topic_time, topic_views, topic_replies, topic_replies_real, forum_id, topic_status, topic_type, topic_first_post_id, topic_first_poster_name, topic_first_poster_colour, topic_last_post_id, topic_last_poster_id, topic_last_poster_name, topic_last_poster_colour, topic_last_post_subject, topic_last_post_time, topic_last_view_time, poll_title) VALUES ('{L_TOPICS_TOPIC_TITLE}', 2, 972086460, 0, 0, 0, 2, 0, 0, 1, 'Admin', 'AA0000', 1, 2, 'Admin', 'AA0000', '{L_TOPICS_TOPIC_TITLE}', 972086460, 972086460, '');

# -- Demo Post
INSERT INTO phpbb_posts (topic_id, forum_id, poster_id, icon_id, post_time, post_username, poster_ip, post_subject, post_text, post_checksum, bbcode_uid) VALUES (1, 2, 2, 0, 972086460, '', '127.0.0.1', '{L_TOPICS_TOPIC_TITLE}', '{L_DEFAULT_INSTALL_POST}', '5dd683b17f641daf84c040bfefc58ce9', '');

# -- Admin posted to the demo topic
INSERT INTO phpbb_topics_posted (user_id, topic_id, topic_posted) VALUES (2, 1, 1);

# -- Smilies
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':D', 'icon_e_biggrin.gif', '{L_SMILIES_VERY_HAPPY}', 15, 17, 1);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-D', 'icon_e_biggrin.gif', '{L_SMILIES_VERY_HAPPY}', 15, 17, 2);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':grin:', 'icon_e_biggrin.gif', '{L_SMILIES_VERY_HAPPY}', 15, 17, 3);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':)', 'icon_e_smile.gif', '{L_SMILIES_SMILE}', 15, 17, 4);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-)', 'icon_e_smile.gif', '{L_SMILIES_SMILE}', 15, 17, 5);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':smile:', 'icon_e_smile.gif', '{L_SMILIES_SMILE}', 15, 17, 6);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (';)', 'icon_e_wink.gif', '{L_SMILIES_WINK}', 15, 17, 7);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (';-)', 'icon_e_wink.gif', '{L_SMILIES_WINK}', 15, 17, 8);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':wink:', 'icon_e_wink.gif', '{L_SMILIES_WINK}', 15, 17, 9);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':(', 'icon_e_sad.gif', '{L_SMILIES_SAD}', 15, 17, 10);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-(', 'icon_e_sad.gif', '{L_SMILIES_SAD}', 15, 17, 11);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':sad:', 'icon_e_sad.gif', '{L_SMILIES_SAD}', 15, 17, 12);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':o', 'icon_e_surprised.gif', '{L_SMILIES_SURPRISED}', 15, 17, 13);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-o', 'icon_e_surprised.gif', '{L_SMILIES_SURPRISED}', 15, 17, 14);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':eek:', 'icon_e_surprised.gif', '{L_SMILIES_SURPRISED}', 15, 17, 15);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':shock:', 'icon_eek.gif', '{L_SMILIES_SHOCKED}', 15, 17, 16);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':?', 'icon_e_confused.gif', '{L_SMILIES_CONFUSED}', 15, 17, 17);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-?', 'icon_e_confused.gif', '{L_SMILIES_CONFUSED}', 15, 17, 18);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':???:', 'icon_e_confused.gif', '{L_SMILIES_CONFUSED}', 15, 17, 19);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES ('8-)', 'icon_cool.gif', '{L_SMILIES_COOL}', 15, 17, 20);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':cool:', 'icon_cool.gif', '{L_SMILIES_COOL}', 15, 17, 21);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':lol:', 'icon_lol.gif', '{L_SMILIES_LAUGHING}', 15, 17, 22);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':x', 'icon_mad.gif', '{L_SMILIES_MAD}', 15, 17, 23);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-x', 'icon_mad.gif', '{L_SMILIES_MAD}', 15, 17, 24);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':mad:', 'icon_mad.gif', '{L_SMILIES_MAD}', 15, 17, 25);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':P', 'icon_razz.gif', '{L_SMILIES_RAZZ}', 15, 17, 26);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-P', 'icon_razz.gif', '{L_SMILIES_RAZZ}', 15, 17, 27);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':razz:', 'icon_razz.gif', '{L_SMILIES_RAZZ}', 15, 17, 28);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':oops:', 'icon_redface.gif', '{L_SMILIES_EMARRASSED}', 15, 17, 29);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':cry:', 'icon_cry.gif', '{L_SMILIES_CRYING}', 15, 17, 30);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':evil:', 'icon_evil.gif', '{L_SMILIES_EVIL}', 15, 17, 31);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':twisted:', 'icon_twisted.gif', '{L_SMILIES_TWISTED_EVIL}', 15, 17, 32);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':roll:', 'icon_rolleyes.gif', '{L_SMILIES_ROLLING_EYES}', 15, 17, 33);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':!:', 'icon_exclaim.gif', '{L_SMILIES_EXCLAMATION}', 15, 17, 34);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':?:', 'icon_question.gif', '{L_SMILIES_QUESTION}', 15, 17, 35);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':idea:', 'icon_idea.gif', '{L_SMILIES_IDEA}', 15, 17, 36);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':arrow:', 'icon_arrow.gif', '{L_SMILIES_ARROW}', 15, 17, 37);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':|', 'icon_neutral.gif', '{L_SMILIES_NEUTRAL}', 15, 17, 38);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':-|', 'icon_neutral.gif', '{L_SMILIES_NEUTRAL}', 15, 17, 39);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':mrgreen:', 'icon_mrgreen.gif', '{L_SMILIES_MR_GREEN}', 15, 17, 40);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':geek:', 'icon_e_geek.gif', '{L_SMILIES_GEEK}', 17, 17, 41);
INSERT INTO phpbb_smilies (code, smiley_url, emotion, smiley_width, smiley_height, smiley_order) VALUES (':ugeek:', 'icon_e_ugeek.gif', '{L_SMILIES_UBER_GEEK}', 17, 18, 42);

# -- icons
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('misc/fire.gif', 16, 16, 1, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('smile/redface.gif', 16, 16, 9, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('smile/mrgreen.gif', 16, 16, 10, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('misc/heart.gif', 16, 16, 4, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('misc/star.gif', 16, 16, 2, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('misc/radioactive.gif', 16, 16, 3, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('misc/thinking.gif', 16, 16, 5, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('smile/info.gif', 16, 16, 8, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('smile/question.gif', 16, 16, 6, 1);
INSERT INTO phpbb_icons (icons_url, icons_width, icons_height, icons_order, display_on_posting) VALUES ('smile/alert.gif', 16, 16, 7, 1);

# -- reasons
INSERT INTO phpbb_reports_reasons (reason_title, reason_description, reason_order) VALUES ('warez', '{L_REPORT_WAREZ}', 1);
INSERT INTO phpbb_reports_reasons (reason_title, reason_description, reason_order) VALUES ('spam', '{L_REPORT_SPAM}', 2);
INSERT INTO phpbb_reports_reasons (reason_title, reason_description, reason_order) VALUES ('off_topic', '{L_REPORT_OFF_TOPIC}', 3);
INSERT INTO phpbb_reports_reasons (reason_title, reason_description, reason_order) VALUES ('other', '{L_REPORT_OTHER}', 4);

# -- extension_groups
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_IMAGES}', 1, 1, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_ARCHIVES}', 0, 1, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_PLAIN_TEXT}', 0, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_DOCUMENTS}', 0, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_REAL_MEDIA}', 3, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_WINDOWS_MEDIA}', 2, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_FLASH_FILES}', 5, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_QUICKTIME_MEDIA}', 6, 0, 1, '', 0, '');
INSERT INTO phpbb_extension_groups (group_name, cat_id, allow_group, download_mode, upload_icon, max_filesize, allowed_forums) VALUES ('{L_EXT_GROUP_DOWNLOADABLE_FILES}', 0, 0, 1, '', 0, '');

# -- extensions
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'gif');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'png');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'jpeg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'jpg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'tif');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'tiff');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (1, 'tga');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'gtar');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'gz');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'tar');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'zip');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'rar');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'ace');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'torrent');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'tgz');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, 'bz2');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (2, '7z');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'txt');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'c');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'h');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'cpp');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'hpp');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'diz');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'csv');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'ini');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'log');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'js');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (3, 'xml');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'xls');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'xlsx');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'xlsm');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'xlsb');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'doc');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'docx');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'docm');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'dot');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'dotx');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'dotm');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'pdf');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'ai');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'ps');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'ppt');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'pptx');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'pptm');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'odg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'odp');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'ods');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'odt');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (4, 'rtf');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (5, 'rm');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (5, 'ram');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (6, 'wma');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (6, 'wmv');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (7, 'swf');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, 'mov');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, 'm4v');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, 'm4a');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, 'mp4');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, '3gp');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, '3g2');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (8, 'qt');

INSERT INTO phpbb_extensions (group_id, extension) VALUES (9, 'mpeg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (9, 'mpg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (9, 'mp3');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (9, 'ogg');
INSERT INTO phpbb_extensions (group_id, extension) VALUES (9, 'ogm');

# POSTGRES COMMIT #