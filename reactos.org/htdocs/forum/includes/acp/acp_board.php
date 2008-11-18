<?php
/**
*
* @package acp
* @version $Id: acp_board.php 8493 2008-04-07 16:04:43Z Kellanved $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
* @todo add cron intervals to server settings? (database_gc, queue_interval, session_gc, search_gc, cache_gc, warnings_gc)
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* @package acp
*/
class acp_board
{
	var $u_action;
	var $new_config = array();

	function main($id, $mode)
	{
		global $db, $user, $auth, $template;
		global $config, $phpbb_root_path, $phpbb_admin_path, $phpEx;

		$user->add_lang('acp/board');

		$action	= request_var('action', '');
		$submit = (isset($_POST['submit'])) ? true : false;

		$form_key = 'acp_board';
		add_form_key($form_key);

		/**
		*	Validation types are:
		*		string, int, bool,
		*		script_path (absolute path in url - beginning with / and no trailing slash),
		*		rpath (relative), rwpath (realtive, writable), path (relative path, but able to escape the root), wpath (writable)
		*/
		switch ($mode)
		{
			case 'settings':
				$display_vars = array(
					'title'	=> 'ACP_BOARD_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'ACP_BOARD_SETTINGS',
						'sitename'				=> array('lang' => 'SITE_NAME',				'validate' => 'string',	'type' => 'text:40:255', 'explain' => false),
						'site_desc'				=> array('lang' => 'SITE_DESC',				'validate' => 'string',	'type' => 'text:40:255', 'explain' => false),
						'board_disable'			=> array('lang' => 'DISABLE_BOARD',			'validate' => 'bool',	'type' => 'custom', 'method' => 'board_disable', 'explain' => true),
						'board_disable_msg'		=> false,
						'default_lang'			=> array('lang' => 'DEFAULT_LANGUAGE',		'validate' => 'lang',	'type' => 'select', 'function' => 'language_select', 'params' => array('{CONFIG_VALUE}'), 'explain' => false),
						'default_dateformat'	=> array('lang' => 'DEFAULT_DATE_FORMAT',	'validate' => 'string',	'type' => 'custom', 'method' => 'dateformat_select', 'explain' => true),
						'board_timezone'		=> array('lang' => 'SYSTEM_TIMEZONE',		'validate' => 'string',	'type' => 'select', 'function' => 'tz_select', 'params' => array('{CONFIG_VALUE}', 1), 'explain' => false),
						'board_dst'				=> array('lang' => 'SYSTEM_DST',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'default_style'			=> array('lang' => 'DEFAULT_STYLE',			'validate' => 'int',	'type' => 'select', 'function' => 'style_select', 'params' => array('{CONFIG_VALUE}', false), 'explain' => false),
						'override_user_style'	=> array('lang' => 'OVERRIDE_STYLE',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'WARNINGS',
						'warnings_expire_days'	=> array('lang' => 'WARNINGS_EXPIRE',		'validate' => 'int',	'type' => 'text:3:4', 'explain' => true, 'append' => ' ' . $user->lang['DAYS']),
					)
				);
			break;

			case 'features':
				$display_vars = array(
					'title'	=> 'ACP_BOARD_FEATURES',
					'vars'	=> array(
						'legend1'				=> 'ACP_BOARD_FEATURES',
						'allow_privmsg'			=> array('lang' => 'BOARD_PM',				'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_topic_notify'	=> array('lang' => 'ALLOW_TOPIC_NOTIFY',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_forum_notify'	=> array('lang' => 'ALLOW_FORUM_NOTIFY',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_namechange'		=> array('lang' => 'ALLOW_NAME_CHANGE',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_attachments'		=> array('lang' => 'ALLOW_ATTACHMENTS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_pm_attach'		=> array('lang' => 'ALLOW_PM_ATTACHMENTS',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_bbcode'			=> array('lang' => 'ALLOW_BBCODE',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_smilies'			=> array('lang' => 'ALLOW_SMILIES',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig'				=> array('lang' => 'ALLOW_SIG',				'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_nocensors'		=> array('lang' => 'ALLOW_NO_CENSORS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_bookmarks'		=> array('lang' => 'ALLOW_BOOKMARKS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_birthdays'		=> array('lang' => 'ALLOW_BIRTHDAYS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'ACP_LOAD_SETTINGS',
						'load_birthdays'		=> array('lang' => 'YES_BIRTHDAYS',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_moderators'		=> array('lang' => 'YES_MODERATORS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_jumpbox'			=> array('lang' => 'YES_JUMPBOX',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_cpf_memberlist'	=> array('lang' => 'LOAD_CPF_MEMBERLIST',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_cpf_viewprofile'	=> array('lang' => 'LOAD_CPF_VIEWPROFILE',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_cpf_viewtopic'	=> array('lang' => 'LOAD_CPF_VIEWTOPIC',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
					)
				);
			break;

			case 'avatar':
				$display_vars = array(
					'title'	=> 'ACP_AVATAR_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'ACP_AVATAR_SETTINGS',

						'avatar_min_width'		=> array('lang' => 'MIN_AVATAR_SIZE', 'validate' => 'int:0', 'type' => false, 'method' => false, 'explain' => false,),
						'avatar_min_height'		=> array('lang' => 'MIN_AVATAR_SIZE', 'validate' => 'int:0', 'type' => false, 'method' => false, 'explain' => false,),
						'avatar_max_width'		=> array('lang' => 'MAX_AVATAR_SIZE', 'validate' => 'int:0', 'type' => false, 'method' => false, 'explain' => false,),
						'avatar_max_height'		=> array('lang' => 'MAX_AVATAR_SIZE', 'validate' => 'int:0', 'type' => false, 'method' => false, 'explain' => false,),

						'allow_avatar_local'	=> array('lang' => 'ALLOW_LOCAL',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_avatar_remote'	=> array('lang' => 'ALLOW_REMOTE',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_avatar_upload'	=> array('lang' => 'ALLOW_UPLOAD',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'avatar_filesize'		=> array('lang' => 'MAX_FILESIZE',			'validate' => 'int:0',	'type' => 'text:4:10', 'explain' => true, 'append' => ' ' . $user->lang['BYTES']),
						'avatar_min'			=> array('lang' => 'MIN_AVATAR_SIZE',		'validate' => 'int:0',	'type' => 'dimension:3:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
						'avatar_max'			=> array('lang' => 'MAX_AVATAR_SIZE',		'validate' => 'int:0',	'type' => 'dimension:3:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
						'avatar_path'			=> array('lang' => 'AVATAR_STORAGE_PATH',	'validate' => 'rwpath',	'type' => 'text:20:255', 'explain' => true),
						'avatar_gallery_path'	=> array('lang' => 'AVATAR_GALLERY_PATH',	'validate' => 'rpath',	'type' => 'text:20:255', 'explain' => true)
					)
				);
			break;

			case 'message':
				$display_vars = array(
					'title'	=> 'ACP_MESSAGE_SETTINGS',
					'lang'	=> 'ucp',
					'vars'	=> array(
						'legend1'				=> 'GENERAL_SETTINGS',
						'allow_privmsg'			=> array('lang' => 'BOARD_PM',				'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'pm_max_boxes'			=> array('lang' => 'BOXES_MAX',				'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),
						'pm_max_msgs'			=> array('lang' => 'BOXES_LIMIT',			'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),
						'full_folder_action'	=> array('lang' => 'FULL_FOLDER_ACTION',	'validate' => 'int',	'type' => 'select', 'method' => 'full_folder_select', 'explain' => true),
						'pm_edit_time'			=> array('lang' => 'PM_EDIT_TIME',			'validate' => 'int:0',	'type' => 'text:5:5', 'explain' => true, 'append' => ' ' . $user->lang['MINUTES']),

						'legend2'				=> 'GENERAL_OPTIONS',
						'allow_mass_pm'			=> array('lang' => 'ALLOW_MASS_PM',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'auth_bbcode_pm'		=> array('lang' => 'ALLOW_BBCODE_PM',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'auth_smilies_pm'		=> array('lang' => 'ALLOW_SMILIES_PM',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_pm_attach'		=> array('lang' => 'ALLOW_PM_ATTACHMENTS',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_pm'			=> array('lang' => 'ALLOW_SIG_PM',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'print_pm'				=> array('lang' => 'ALLOW_PRINT_PM',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'forward_pm'			=> array('lang' => 'ALLOW_FORWARD_PM',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'auth_img_pm'			=> array('lang' => 'ALLOW_IMG_PM',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'auth_flash_pm'			=> array('lang' => 'ALLOW_FLASH_PM',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'enable_pm_icons'		=> array('lang' => 'ENABLE_PM_ICONS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false)
					)
				);
			break;

			case 'post':
				$display_vars = array(
					'title'	=> 'ACP_POST_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'GENERAL_OPTIONS',
						'allow_topic_notify'	=> array('lang' => 'ALLOW_TOPIC_NOTIFY',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_forum_notify'	=> array('lang' => 'ALLOW_FORUM_NOTIFY',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_bbcode'			=> array('lang' => 'ALLOW_BBCODE',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_post_flash'		=> array('lang' => 'ALLOW_POST_FLASH',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_smilies'			=> array('lang' => 'ALLOW_SMILIES',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_post_links'		=> array('lang' => 'ALLOW_POST_LINKS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_nocensors'		=> array('lang' => 'ALLOW_NO_CENSORS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'allow_bookmarks'		=> array('lang' => 'ALLOW_BOOKMARKS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'enable_post_confirm'	=> array('lang' => 'VISUAL_CONFIRM_POST',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'POSTING',
						'bump_type'				=> false,
						'edit_time'				=> array('lang' => 'EDIT_TIME',				'validate' => 'int:0',	'type' => 'text:5:5', 'explain' => true, 'append' => ' ' . $user->lang['MINUTES']),
						'display_last_edited'	=> array('lang' => 'DISPLAY_LAST_EDITED',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'flood_interval'		=> array('lang' => 'FLOOD_INTERVAL',		'validate' => 'int:0',	'type' => 'text:3:10', 'explain' => true, 'append' => ' ' . $user->lang['SECONDS']),
						'bump_interval'			=> array('lang' => 'BUMP_INTERVAL',			'validate' => 'int:0',	'type' => 'custom', 'method' => 'bump_interval', 'explain' => true),
						'topics_per_page'		=> array('lang' => 'TOPICS_PER_PAGE',		'validate' => 'int:1',	'type' => 'text:3:4', 'explain' => false),
						'posts_per_page'		=> array('lang' => 'POSTS_PER_PAGE',		'validate' => 'int:1',	'type' => 'text:3:4', 'explain' => false),
						'hot_threshold'			=> array('lang' => 'HOT_THRESHOLD',			'validate' => 'int:0',	'type' => 'text:3:4', 'explain' => true),
						'max_poll_options'		=> array('lang' => 'MAX_POLL_OPTIONS',		'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => false),
						'max_post_chars'		=> array('lang' => 'CHAR_LIMIT',			'validate' => 'int:0',	'type' => 'text:4:6', 'explain' => true),
						'max_post_smilies'		=> array('lang' => 'SMILIES_LIMIT',			'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),
						'max_post_urls'			=> array('lang' => 'MAX_POST_URLS',			'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true),
						'max_post_font_size'	=> array('lang' => 'MAX_POST_FONT_SIZE',	'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' %'),
						'max_quote_depth'		=> array('lang' => 'QUOTE_DEPTH_LIMIT',		'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),
						'max_post_img_width'	=> array('lang' => 'MAX_POST_IMG_WIDTH',	'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
						'max_post_img_height'	=> array('lang' => 'MAX_POST_IMG_HEIGHT',	'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
					)
				);
			break;

			case 'signature':
				$display_vars = array(
					'title'	=> 'ACP_SIGNATURE_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'GENERAL_OPTIONS',
						'allow_sig'				=> array('lang' => 'ALLOW_SIG',				'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_bbcode'		=> array('lang' => 'ALLOW_SIG_BBCODE',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_img'			=> array('lang' => 'ALLOW_SIG_IMG',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_flash'		=> array('lang' => 'ALLOW_SIG_FLASH',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_smilies'		=> array('lang' => 'ALLOW_SIG_SMILIES',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_sig_links'		=> array('lang' => 'ALLOW_SIG_LINKS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'GENERAL_SETTINGS',
						'max_sig_chars'			=> array('lang' => 'MAX_SIG_LENGTH',		'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true),
						'max_sig_urls'			=> array('lang' => 'MAX_SIG_URLS',			'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true),
						'max_sig_font_size'		=> array('lang' => 'MAX_SIG_FONT_SIZE',		'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' %'),
						'max_sig_smilies'		=> array('lang' => 'MAX_SIG_SMILIES',		'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true),
						'max_sig_img_width'		=> array('lang' => 'MAX_SIG_IMG_WIDTH',		'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
						'max_sig_img_height'	=> array('lang' => 'MAX_SIG_IMG_HEIGHT',	'validate' => 'int:0',	'type' => 'text:5:4', 'explain' => true, 'append' => ' ' . $user->lang['PIXEL']),
					)
				);
			break;

			case 'registration':
				$display_vars = array(
					'title'	=> 'ACP_REGISTER_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'GENERAL_SETTINGS',
						'max_name_chars'		=> array('lang' => 'USERNAME_LENGTH', 'validate' => 'int:8:180', 'type' => false, 'method' => false, 'explain' => false,),
						'max_pass_chars'		=> array('lang' => 'PASSWORD_LENGTH', 'validate' => 'int:8:255', 'type' => false, 'method' => false, 'explain' => false,),

						'require_activation'	=> array('lang' => 'ACC_ACTIVATION',	'validate' => 'int',	'type' => 'custom', 'method' => 'select_acc_activation', 'explain' => true),
						'min_name_chars'		=> array('lang' => 'USERNAME_LENGTH',	'validate' => 'int:1',	'type' => 'custom:5:180', 'method' => 'username_length', 'explain' => true),
						'min_pass_chars'		=> array('lang' => 'PASSWORD_LENGTH',	'validate' => 'int:1',	'type' => 'custom', 'method' => 'password_length', 'explain' => true),
						'allow_name_chars'		=> array('lang' => 'USERNAME_CHARS',	'validate' => 'string',	'type' => 'select', 'method' => 'select_username_chars', 'explain' => true),
						'pass_complex'			=> array('lang' => 'PASSWORD_TYPE',		'validate' => 'string',	'type' => 'select', 'method' => 'select_password_chars', 'explain' => true),
						'chg_passforce'			=> array('lang' => 'FORCE_PASS_CHANGE',	'validate' => 'int:0',	'type' => 'text:3:3', 'explain' => true, 'append' => ' ' . $user->lang['DAYS']),

						'legend2'				=> 'GENERAL_OPTIONS',
						'allow_namechange'		=> array('lang' => 'ALLOW_NAME_CHANGE',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'allow_emailreuse'		=> array('lang' => 'ALLOW_EMAIL_REUSE',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'enable_confirm'		=> array('lang' => 'VISUAL_CONFIRM_REG',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'max_login_attempts'	=> array('lang' => 'MAX_LOGIN_ATTEMPTS',	'validate' => 'int:0',	'type' => 'text:3:3', 'explain' => true),
						'max_reg_attempts'		=> array('lang' => 'REG_LIMIT',				'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),

						'legend3'			=> 'COPPA',
						'coppa_enable'		=> array('lang' => 'ENABLE_COPPA',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'coppa_mail'		=> array('lang' => 'COPPA_MAIL',		'validate' => 'string',	'type' => 'textarea:5:40', 'explain' => true),
						'coppa_fax'			=> array('lang' => 'COPPA_FAX',			'validate' => 'string',	'type' => 'text:25:100', 'explain' => false),
					)
				);
			break;

			case 'cookie':
				$display_vars = array(
					'title'	=> 'ACP_COOKIE_SETTINGS',
					'vars'	=> array(
						'legend1'		=> 'ACP_COOKIE_SETTINGS',
						'cookie_domain'	=> array('lang' => 'COOKIE_DOMAIN',	'validate' => 'string',	'type' => 'text::255', 'explain' => false),
						'cookie_name'	=> array('lang' => 'COOKIE_NAME',	'validate' => 'string',	'type' => 'text::16', 'explain' => false),
						'cookie_path'	=> array('lang'	=> 'COOKIE_PATH',	'validate' => 'string',	'type' => 'text::255', 'explain' => false),
						'cookie_secure'	=> array('lang' => 'COOKIE_SECURE',	'validate' => 'bool',	'type' => 'radio:disabled_enabled', 'explain' => true)
					)
				);
			break;

			case 'load':
				$display_vars = array(
					'title'	=> 'ACP_LOAD_SETTINGS',
					'vars'	=> array(
						'legend1'			=> 'GENERAL_SETTINGS',
						'limit_load'		=> array('lang' => 'LIMIT_LOAD',		'validate' => 'string',	'type' => 'text:4:4', 'explain' => true),
						'session_length'	=> array('lang' => 'SESSION_LENGTH',	'validate' => 'int:60',	'type' => 'text:5:10', 'explain' => true, 'append' => ' ' . $user->lang['SECONDS']),
						'active_sessions'	=> array('lang' => 'LIMIT_SESSIONS',	'validate' => 'int:0',	'type' => 'text:4:4', 'explain' => true),
						'load_online_time'	=> array('lang' => 'ONLINE_LENGTH',		'validate' => 'int:0',	'type' => 'text:4:3', 'explain' => true, 'append' => ' ' . $user->lang['MINUTES']),

						'legend2'				=> 'GENERAL_OPTIONS',
						'load_db_track'			=> array('lang' => 'YES_POST_MARKING',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_db_lastread'		=> array('lang' => 'YES_READ_MARKING',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_anon_lastread'	=> array('lang' => 'YES_ANON_READ_MARKING',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_online'			=> array('lang' => 'YES_ONLINE',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_online_guests'	=> array('lang' => 'YES_ONLINE_GUESTS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_onlinetrack'		=> array('lang' => 'YES_ONLINE_TRACK',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_birthdays'		=> array('lang' => 'YES_BIRTHDAYS',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_moderators'		=> array('lang' => 'YES_MODERATORS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_jumpbox'			=> array('lang' => 'YES_JUMPBOX',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_user_activity'	=> array('lang' => 'LOAD_USER_ACTIVITY',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'load_tplcompile'		=> array('lang' => 'RECOMPILE_STYLES',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend3'				=> 'CUSTOM_PROFILE_FIELDS',
						'load_cpf_memberlist'	=> array('lang' => 'LOAD_CPF_MEMBERLIST',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_cpf_viewprofile'	=> array('lang' => 'LOAD_CPF_VIEWPROFILE',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
						'load_cpf_viewtopic'	=> array('lang' => 'LOAD_CPF_VIEWTOPIC',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => false),
					)
				);
			break;

			case 'auth':
				$display_vars = array(
					'title'	=> 'ACP_AUTH_SETTINGS',
					'vars'	=> array(
						'legend1'		=> 'ACP_AUTH_SETTINGS',
						'auth_method'	=> array('lang' => 'AUTH_METHOD',	'validate' => 'string',	'type' => 'select', 'method' => 'select_auth_method', 'explain' => false)
					)
				);
			break;

			case 'server':
				$display_vars = array(
					'title'	=> 'ACP_SERVER_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'ACP_SERVER_SETTINGS',
						'gzip_compress'			=> array('lang' => 'ENABLE_GZIP',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'PATH_SETTINGS',
						'smilies_path'			=> array('lang' => 'SMILIES_PATH',		'validate' => 'rpath',	'type' => 'text:20:255', 'explain' => true),
						'icons_path'			=> array('lang' => 'ICONS_PATH',		'validate' => 'rpath',	'type' => 'text:20:255', 'explain' => true),
						'upload_icons_path'		=> array('lang' => 'UPLOAD_ICONS_PATH',	'validate' => 'rpath',	'type' => 'text:20:255', 'explain' => true),
						'ranks_path'			=> array('lang' => 'RANKS_PATH',		'validate' => 'rpath',	'type' => 'text:20:255', 'explain' => true),

						'legend3'				=> 'SERVER_URL_SETTINGS',
						'force_server_vars'		=> array('lang' => 'FORCE_SERVER_VARS',	'validate' => 'bool',			'type' => 'radio:yes_no', 'explain' => true),
						'server_protocol'		=> array('lang' => 'SERVER_PROTOCOL',	'validate' => 'string',			'type' => 'text:10:10', 'explain' => true),
						'server_name'			=> array('lang' => 'SERVER_NAME',		'validate' => 'string',			'type' => 'text:40:255', 'explain' => true),
						'server_port'			=> array('lang' => 'SERVER_PORT',		'validate' => 'int:0',			'type' => 'text:5:5', 'explain' => true),
						'script_path'			=> array('lang' => 'SCRIPT_PATH',		'validate' => 'script_path',	'type' => 'text::255', 'explain' => true),
					)
				);
			break;

			case 'security':
				$display_vars = array(
					'title'	=> 'ACP_SECURITY_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'ACP_SECURITY_SETTINGS',
						'allow_autologin'		=> array('lang' => 'ALLOW_AUTOLOGIN',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'max_autologin_time'	=> array('lang' => 'AUTOLOGIN_LENGTH',		'validate' => 'int:0',	'type' => 'text:5:5', 'explain' => true, 'append' => ' ' . $user->lang['DAYS']),
						'ip_check'				=> array('lang' => 'IP_VALID',				'validate' => 'int',	'type' => 'custom', 'method' => 'select_ip_check', 'explain' => true),
						'browser_check'			=> array('lang' => 'BROWSER_VALID',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'forwarded_for_check'	=> array('lang' => 'FORWARDED_FOR_VALID',	'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'check_dnsbl'			=> array('lang' => 'CHECK_DNSBL',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'email_check_mx'		=> array('lang' => 'EMAIL_CHECK_MX',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'pass_complex'			=> array('lang' => 'PASSWORD_TYPE',			'validate' => 'string',	'type' => 'select', 'method' => 'select_password_chars', 'explain' => true),
						'chg_passforce'			=> array('lang' => 'FORCE_PASS_CHANGE',		'validate' => 'int:0',	'type' => 'text:3:3', 'explain' => true, 'append' => ' ' . $user->lang['DAYS']),
						'max_login_attempts'	=> array('lang' => 'MAX_LOGIN_ATTEMPTS',	'validate' => 'int:0',	'type' => 'text:3:3', 'explain' => true),
						'tpl_allow_php'			=> array('lang' => 'TPL_ALLOW_PHP',			'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'form_token_lifetime'	=> array('lang' => 'FORM_TIME_MAX',			'validate' => 'int:-1',	'type' => 'text:5:5', 'explain' => true, 'append' => ' ' . $user->lang['SECONDS']),
						'form_token_sid_guests'	=> array('lang' => 'FORM_SID_GUESTS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

					)
				);
			break;

			case 'email':
				$display_vars = array(
					'title'	=> 'ACP_EMAIL_SETTINGS',
					'vars'	=> array(
						'legend1'				=> 'GENERAL_SETTINGS',
						'email_enable'			=> array('lang' => 'ENABLE_EMAIL',			'validate' => 'bool',	'type' => 'radio:enabled_disabled', 'explain' => true),
						'board_email_form'		=> array('lang' => 'BOARD_EMAIL_FORM',		'validate' => 'bool',	'type' => 'radio:enabled_disabled', 'explain' => true),
						'email_function_name'	=> array('lang' => 'EMAIL_FUNCTION_NAME',	'validate' => 'string',	'type' => 'text:20:50', 'explain' => true),
						'email_package_size'	=> array('lang' => 'EMAIL_PACKAGE_SIZE',	'validate' => 'int:0',	'type' => 'text:5:5', 'explain' => true),
						'board_contact'			=> array('lang' => 'CONTACT_EMAIL',			'validate' => 'string',	'type' => 'text:25:100', 'explain' => true),
						'board_email'			=> array('lang' => 'ADMIN_EMAIL',			'validate' => 'string',	'type' => 'text:25:100', 'explain' => true),
						'board_email_sig'		=> array('lang' => 'EMAIL_SIG',				'validate' => 'string',	'type' => 'textarea:5:30', 'explain' => true),
						'board_hide_emails'		=> array('lang' => 'BOARD_HIDE_EMAILS',		'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),

						'legend2'				=> 'SMTP_SETTINGS',
						'smtp_delivery'			=> array('lang' => 'USE_SMTP',				'validate' => 'bool',	'type' => 'radio:yes_no', 'explain' => true),
						'smtp_host'				=> array('lang' => 'SMTP_SERVER',			'validate' => 'string',	'type' => 'text:25:50', 'explain' => false),
						'smtp_port'				=> array('lang' => 'SMTP_PORT',				'validate' => 'int:0',	'type' => 'text:4:5', 'explain' => true),
						'smtp_auth_method'		=> array('lang' => 'SMTP_AUTH_METHOD',		'validate' => 'string',	'type' => 'select', 'method' => 'mail_auth_select', 'explain' => true),
						'smtp_username'			=> array('lang' => 'SMTP_USERNAME',			'validate' => 'string',	'type' => 'text:25:255', 'explain' => true),
						'smtp_password'			=> array('lang' => 'SMTP_PASSWORD',			'validate' => 'string',	'type' => 'password:25:255', 'explain' => true)
					)
				);
			break;

			default:
				trigger_error('NO_MODE', E_USER_ERROR);
			break;
		}

		if (isset($display_vars['lang']))
		{
			$user->add_lang($display_vars['lang']);
		}

		$this->new_config = $config;
		$cfg_array = (isset($_REQUEST['config'])) ? utf8_normalize_nfc(request_var('config', array('' => ''), true)) : $this->new_config;
		$error = array();

		// We validate the complete config if whished
		validate_config_vars($display_vars['vars'], $cfg_array, $error);

		if ($submit && !check_form_key($form_key))
		{
			$error[] = $user->lang['FORM_INVALID'];
		}
		// Do not write values if there is an error
		if (sizeof($error))
		{
			$submit = false;
		}

		// We go through the display_vars to make sure no one is trying to set variables he/she is not allowed to...
		foreach ($display_vars['vars'] as $config_name => $null)
		{
			if (!isset($cfg_array[$config_name]) || strpos($config_name, 'legend') !== false)
			{
				continue;
			}

			if ($config_name == 'auth_method')
			{
				continue;
			}

			$this->new_config[$config_name] = $config_value = $cfg_array[$config_name];

			if ($config_name == 'email_function_name')
			{
				$this->new_config['email_function_name'] = trim(str_replace(array('(', ')'), array('', ''), $this->new_config['email_function_name']));
				$this->new_config['email_function_name'] = (empty($this->new_config['email_function_name']) || !function_exists($this->new_config['email_function_name'])) ? 'mail' : $this->new_config['email_function_name'];
				$config_value = $this->new_config['email_function_name'];
			}

			if ($submit)
			{
				set_config($config_name, $config_value);
			}
		}

		if ($mode == 'auth')
		{
			// Retrieve a list of auth plugins and check their config values
			$auth_plugins = array();

			$dp = @opendir($phpbb_root_path . 'includes/auth');

			if ($dp)
			{
				while (($file = readdir($dp)) !== false)
				{
					if (preg_match('#^auth_(.*?)\.' . $phpEx . '$#', $file))
					{
						$auth_plugins[] = basename(preg_replace('#^auth_(.*?)\.' . $phpEx . '$#', '\1', $file));
					}
				}
				closedir($dp);

				sort($auth_plugins);
			}

			$updated_auth_settings = false;
			$old_auth_config = array();
			foreach ($auth_plugins as $method)
			{
				if ($method && file_exists($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx))
				{
					include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

					$method = 'acp_' . $method;
					if (function_exists($method))
					{
						if ($fields = $method($this->new_config))
						{
							// Check if we need to create config fields for this plugin and save config when submit was pressed
							foreach ($fields['config'] as $field)
							{
								if (!isset($config[$field]))
								{
									set_config($field, '');
								}

								if (!isset($cfg_array[$field]) || strpos($field, 'legend') !== false)
								{
									continue;
								}

								$old_auth_config[$field] = $this->new_config[$field];
								$config_value = $cfg_array[$field];
								$this->new_config[$field] = $config_value;

								if ($submit)
								{
									$updated_auth_settings = true;
									set_config($field, $config_value);
								}
							}
						}
						unset($fields);
					}
				}
			}

			if ($submit && (($cfg_array['auth_method'] != $this->new_config['auth_method']) || $updated_auth_settings))
			{
				$method = basename($cfg_array['auth_method']);
				if ($method && in_array($method, $auth_plugins))
				{
					include_once($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx);

					$method = 'init_' . $method;
					if (function_exists($method))
					{
						if ($error = $method())
						{
							foreach ($old_auth_config as $config_name => $config_value)
							{
								set_config($config_name, $config_value);
							}
							trigger_error($error . adm_back_link($this->u_action), E_USER_WARNING);
						}
					}
					set_config('auth_method', basename($cfg_array['auth_method']));
				}
				else
				{
					trigger_error('NO_AUTH_PLUGIN', E_USER_ERROR);
				}
			}
		}

		if ($submit)
		{
			add_log('admin', 'LOG_CONFIG_' . strtoupper($mode));

			trigger_error($user->lang['CONFIG_UPDATED'] . adm_back_link($this->u_action));
		}

		$this->tpl_name = 'acp_board';
		$this->page_title = $display_vars['title'];

		$template->assign_vars(array(
			'L_TITLE'			=> $user->lang[$display_vars['title']],
			'L_TITLE_EXPLAIN'	=> $user->lang[$display_vars['title'] . '_EXPLAIN'],

			'S_ERROR'			=> (sizeof($error)) ? true : false,
			'ERROR_MSG'			=> implode('<br />', $error),

			'U_ACTION'			=> $this->u_action)
		);

		// Output relevant page
		foreach ($display_vars['vars'] as $config_key => $vars)
		{
			if (!is_array($vars) && strpos($config_key, 'legend') === false)
			{
				continue;
			}

			if (strpos($config_key, 'legend') !== false)
			{
				$template->assign_block_vars('options', array(
					'S_LEGEND'		=> true,
					'LEGEND'		=> (isset($user->lang[$vars])) ? $user->lang[$vars] : $vars)
				);

				continue;
			}

			$type = explode(':', $vars['type']);

			$l_explain = '';
			if ($vars['explain'] && isset($vars['lang_explain']))
			{
				$l_explain = (isset($user->lang[$vars['lang_explain']])) ? $user->lang[$vars['lang_explain']] : $vars['lang_explain'];
			}
			else if ($vars['explain'])
			{
				$l_explain = (isset($user->lang[$vars['lang'] . '_EXPLAIN'])) ? $user->lang[$vars['lang'] . '_EXPLAIN'] : '';
			}
			
			$content = build_cfg_template($type, $config_key, $this->new_config, $config_key, $vars);
			
			if (empty($content))
			{
				continue;
			}
			
			$template->assign_block_vars('options', array(
				'KEY'			=> $config_key,
				'TITLE'			=> (isset($user->lang[$vars['lang']])) ? $user->lang[$vars['lang']] : $vars['lang'],
				'S_EXPLAIN'		=> $vars['explain'],
				'TITLE_EXPLAIN'	=> $l_explain,
				'CONTENT'		=> build_cfg_template($type, $config_key, $this->new_config, $config_key, $vars),
				)
			);

			unset($display_vars['vars'][$config_key]);
		}

		if ($mode == 'auth')
		{
			$template->assign_var('S_AUTH', true);

			foreach ($auth_plugins as $method)
			{
				if ($method && file_exists($phpbb_root_path . 'includes/auth/auth_' . $method . '.' . $phpEx))
				{
					$method = 'acp_' . $method;
					if (function_exists($method))
					{
						$fields = $method($this->new_config);

						if ($fields['tpl'])
						{
							$template->assign_block_vars('auth_tpl', array(
								'TPL'	=> $fields['tpl'])
							);
						}
						unset($fields);
					}
				}
			}
		}
	}

	/**
	* Select auth method
	*/
	function select_auth_method($selected_method, $key = '')
	{
		global $phpbb_root_path, $phpEx;

		$auth_plugins = array();

		$dp = @opendir($phpbb_root_path . 'includes/auth');

		if (!$dp)
		{
			return '';
		}

		while (($file = readdir($dp)) !== false)
		{
			if (preg_match('#^auth_(.*?)\.' . $phpEx . '$#', $file))
			{
				$auth_plugins[] = preg_replace('#^auth_(.*?)\.' . $phpEx . '$#', '\1', $file);
			}
		}
		closedir($dp);

		sort($auth_plugins);

		$auth_select = '';
		foreach ($auth_plugins as $method)
		{
			$selected = ($selected_method == $method) ? ' selected="selected"' : '';
			$auth_select .= '<option value="' . $method . '"' . $selected . '>' . ucfirst($method) . '</option>';
		}

		return $auth_select;
	}

	/**
	* Select mail authentication method
	*/
	function mail_auth_select($selected_method, $key = '')
	{
		global $user;

		$auth_methods = array('PLAIN', 'LOGIN', 'CRAM-MD5', 'DIGEST-MD5', 'POP-BEFORE-SMTP');
		$s_smtp_auth_options = '';

		foreach ($auth_methods as $method)
		{
			$s_smtp_auth_options .= '<option value="' . $method . '"' . (($selected_method == $method) ? ' selected="selected"' : '') . '>' . $user->lang['SMTP_' . str_replace('-', '_', $method)] . '</option>';
		}

		return $s_smtp_auth_options;
	}

	/**
	* Select full folder action
	*/
	function full_folder_select($value, $key = '')
	{
		global $user;

		return '<option value="1"' . (($value == 1) ? ' selected="selected"' : '') . '>' . $user->lang['DELETE_OLDEST_MESSAGES'] . '</option><option value="2"' . (($value == 2) ? ' selected="selected"' : '') . '>' . $user->lang['HOLD_NEW_MESSAGES_SHORT'] . '</option>';
	}

	/**
	* Select ip validation
	*/
	function select_ip_check($value, $key = '')
	{
		$radio_ary = array(4 => 'ALL', 3 => 'CLASS_C', 2 => 'CLASS_B', 0 => 'NO_IP_VALIDATION');

		return h_radio('config[ip_check]', $radio_ary, $value, $key);
	}

	/**
	* Select account activation method
	*/
	function select_acc_activation($value, $key = '')
	{
		global $user, $config;

		$radio_ary = array(USER_ACTIVATION_DISABLE => 'ACC_DISABLE', USER_ACTIVATION_NONE => 'ACC_NONE');
		if ($config['email_enable'])
		{
			$radio_ary += array(USER_ACTIVATION_SELF => 'ACC_USER', USER_ACTIVATION_ADMIN => 'ACC_ADMIN');
		}

		return h_radio('config[require_activation]', $radio_ary, $value, $key);
	}

	/**
	* Maximum/Minimum username length
	*/
	function username_length($value, $key = '')
	{
		global $user;

		return '<input id="' . $key . '" type="text" size="3" maxlength="3" name="config[min_name_chars]" value="' . $value . '" /> ' . $user->lang['MIN_CHARS'] . '&nbsp;&nbsp;<input type="text" size="3" maxlength="3" name="config[max_name_chars]" value="' . $this->new_config['max_name_chars'] . '" /> ' . $user->lang['MAX_CHARS'];
	}

	/**
	* Allowed chars in usernames
	*/
	function select_username_chars($selected_value, $key)
	{
		global $user;

		$user_char_ary = array('USERNAME_CHARS_ANY', 'USERNAME_ALPHA_ONLY', 'USERNAME_ALPHA_SPACERS', 'USERNAME_LETTER_NUM', 'USERNAME_LETTER_NUM_SPACERS', 'USERNAME_ASCII');
		$user_char_options = '';
		foreach ($user_char_ary as $user_type)
		{
			$selected = ($selected_value == $user_type) ? ' selected="selected"' : '';
			$user_char_options .= '<option value="' . $user_type . '"' . $selected . '>' . $user->lang[$user_type] . '</option>';
		}

		return $user_char_options;
	}

	/**
	* Maximum/Minimum password length
	*/
	function password_length($value, $key)
	{
		global $user;

		return '<input id="' . $key . '" type="text" size="3" maxlength="3" name="config[min_pass_chars]" value="' . $value . '" /> ' . $user->lang['MIN_CHARS'] . '&nbsp;&nbsp;<input type="text" size="3" maxlength="3" name="config[max_pass_chars]" value="' . $this->new_config['max_pass_chars'] . '" /> ' . $user->lang['MAX_CHARS'];
	}

	/**
	* Required chars in passwords
	*/
	function select_password_chars($selected_value, $key)
	{
		global $user;

		$pass_type_ary = array('PASS_TYPE_ANY', 'PASS_TYPE_CASE', 'PASS_TYPE_ALPHA', 'PASS_TYPE_SYMBOL');
		$pass_char_options = '';
		foreach ($pass_type_ary as $pass_type)
		{
			$selected = ($selected_value == $pass_type) ? ' selected="selected"' : '';
			$pass_char_options .= '<option value="' . $pass_type . '"' . $selected . '>' . $user->lang[$pass_type] . '</option>';
		}

		return $pass_char_options;
	}

	/**
	* Select bump interval
	*/
	function bump_interval($value, $key)
	{
		global $user;

		$s_bump_type = '';
		$types = array('m' => 'MINUTES', 'h' => 'HOURS', 'd' => 'DAYS');
		foreach ($types as $type => $lang)
		{
			$selected = ($this->new_config['bump_type'] == $type) ? ' selected="selected"' : '';
			$s_bump_type .= '<option value="' . $type . '"' . $selected . '>' . $user->lang[$lang] . '</option>';
		}

		return '<input id="' . $key . '" type="text" size="3" maxlength="4" name="config[bump_interval]" value="' . $value . '" />&nbsp;<select name="config[bump_type]">' . $s_bump_type . '</select>';
	}

	/**
	* Board disable option and message
	*/
	function board_disable($value, $key)
	{
		global $user;

		$radio_ary = array(1 => 'YES', 0 => 'NO');

		return h_radio('config[board_disable]', $radio_ary, $value) . '<br /><input id="' . $key . '" type="text" name="config[board_disable_msg]" maxlength="255" size="40" value="' . $this->new_config['board_disable_msg'] . '" />';
	}

	/**
	* Select default dateformat
	*/
	function dateformat_select($value, $key)
	{
		global $user, $config;

		// Let the format_date function operate with the acp values
		$old_tz = $user->timezone;
		$old_dst = $user->dst;

		$user->timezone = $config['board_timezone'];
		$user->dst = $config['board_dst'];

		$dateformat_options = '';

		foreach ($user->lang['dateformats'] as $format => $null)
		{
			$dateformat_options .= '<option value="' . $format . '"' . (($format == $value) ? ' selected="selected"' : '') . '>';
			$dateformat_options .= $user->format_date(time(), $format, false) . ((strpos($format, '|') !== false) ? $user->lang['VARIANT_DATE_SEPARATOR'] . $user->format_date(time(), $format, true) : '');
			$dateformat_options .= '</option>';
		}

		$dateformat_options .= '<option value="custom"';
		if (!isset($user->lang['dateformats'][$value]))
		{
			$dateformat_options .= ' selected="selected"';
		}
		$dateformat_options .= '>' . $user->lang['CUSTOM_DATEFORMAT'] . '</option>';

		// Reset users date options
		$user->timezone = $old_tz;
		$user->dst = $old_dst;

		return "<select name=\"dateoptions\" id=\"dateoptions\" onchange=\"if (this.value == 'custom') { document.getElementById('" . addslashes($key) . "').value = '" . addslashes($value) . "'; } else { document.getElementById('" . addslashes($key) . "').value = this.value; }\">$dateformat_options</select>
		<input type=\"text\" name=\"config[$key]\" id=\"$key\" value=\"$value\" maxlength=\"30\" />";
	}
}

?>