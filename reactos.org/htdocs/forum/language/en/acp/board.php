<?php
/**
*
* acp_board [English]
*
* @package language
* @version $Id: board.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* DO NOT CHANGE
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

if (empty($lang) || !is_array($lang))
{
	$lang = array();
}

// DEVELOPERS PLEASE NOTE
//
// All language files should use UTF-8 as their encoding and the files must not contain a BOM.
//
// Placeholders can now contain order information, e.g. instead of
// 'Page %s of %s' you can (and should) write 'Page %1$s of %2$s', this allows
// translators to re-order the output of data while ensuring it remains correct
//
// You do not need this where single placeholders are used, e.g. 'Message %d' is fine
// equally where a string contains only two placeholders which are used to wrap text
// in a url you again do not need to specify an order e.g., 'Click %sHERE%s' is fine

// Board Settings
$lang = array_merge($lang, array(
	'ACP_BOARD_SETTINGS_EXPLAIN'	=> 'Here you can determine the basic operation of your board, give it a fitting name and description, and among other settings adjust the default values for timezone and language.',
	'CUSTOM_DATEFORMAT'				=> 'Custom…',
	'DEFAULT_DATE_FORMAT'			=> 'Date format',
	'DEFAULT_DATE_FORMAT_EXPLAIN'	=> 'The date format is the same as the PHP <code>date</code> function.',
	'DEFAULT_LANGUAGE'				=> 'Default language',
	'DEFAULT_STYLE'					=> 'Default style',
	'DISABLE_BOARD'					=> 'Disable board',
	'DISABLE_BOARD_EXPLAIN'			=> 'This will make the board unavailable to users. You can also enter a short (255 character) message to display if you wish.',
	'OVERRIDE_STYLE'				=> 'Override user style',
	'OVERRIDE_STYLE_EXPLAIN'		=> 'Replaces user’s style with the default.',
	'SITE_DESC'						=> 'Site description',
	'SITE_NAME'						=> 'Site name',
	'SYSTEM_DST'					=> 'Enable Summer Time/<abbr title="Daylight Saving Time">DST</abbr>',
	'SYSTEM_TIMEZONE'				=> 'System timezone',
	'WARNINGS_EXPIRE'				=> 'Warning duration',
	'WARNINGS_EXPIRE_EXPLAIN'		=> 'Number of days that will elapse before the warning will automatically expire from a user’s record.',
));

// Board Features
$lang = array_merge($lang, array(
	'ACP_BOARD_FEATURES_EXPLAIN'	=> 'Here you can enable/disable several board features.',

	'ALLOW_ATTACHMENTS'			=> 'Allow attachments',
	'ALLOW_BIRTHDAYS'			=> 'Allow birthdays',
	'ALLOW_BIRTHDAYS_EXPLAIN'	=> 'Allow birthdays to be entered and age being displayed in profiles. Please note the birthday list within the board index is controlled by a separate load setting.',
	'ALLOW_BOOKMARKS'			=> 'Allow bookmarking topics',
	'ALLOW_BOOKMARKS_EXPLAIN'	=> 'User is able to store personal bookmarks.',
	'ALLOW_BBCODE'				=> 'Allow BBCode',
	'ALLOW_FORUM_NOTIFY'		=> 'Allow subscribing to forums',
	'ALLOW_NAME_CHANGE'			=> 'Allow username changes',
	'ALLOW_NO_CENSORS'			=> 'Allow disabling of word censoring',
	'ALLOW_NO_CENSORS_EXPLAIN'	=> 'Users can choose to disable the automatic word censoring of posts and private messages.',
	'ALLOW_PM_ATTACHMENTS'		=> 'Allow attachments in private messages',
	'ALLOW_SIG'					=> 'Allow signatures',
	'ALLOW_SIG_BBCODE'			=> 'Allow BBCode in user signatures',
	'ALLOW_SIG_FLASH'			=> 'Allow use of <code>[FLASH]</code> BBCode tag in user signatures',
	'ALLOW_SIG_IMG'				=> 'Allow use of <code>[IMG]</code> BBCode tag in user signatures',
	'ALLOW_SIG_LINKS'			=> 'Allow use of links in user signatures',
	'ALLOW_SIG_LINKS_EXPLAIN'	=> 'If disallowed the <code>[URL]</code> BBCode tag and automatic/magic URLs are disabled.',
	'ALLOW_SIG_SMILIES'			=> 'Allow use of smilies in user signatures',
	'ALLOW_SMILIES'				=> 'Allow smilies',
	'ALLOW_TOPIC_NOTIFY'		=> 'Allow subscribing to topics',
	'BOARD_PM'					=> 'Private messaging',
	'BOARD_PM_EXPLAIN'			=> 'Enable or disable private messaging for all users.',
));

// Avatar Settings
$lang = array_merge($lang, array(
	'ACP_AVATAR_SETTINGS_EXPLAIN'	=> 'Avatars are generally small, unique images a user can associate with themselves. Depending on the style they are usually displayed below the username when viewing topics. Here you can determine how users can define their avatars. Please note that in order to upload avatars you need to have created the directory you name below and ensure it can be written to by the web server. Please also note that file size limits are only imposed on uploaded avatars, they do not apply to remotely linked images.',

	'ALLOW_LOCAL'					=> 'Enable gallery avatars',
	'ALLOW_REMOTE'					=> 'Enable remote avatars',
	'ALLOW_REMOTE_EXPLAIN'			=> 'Avatars linked to from another website.',
	'ALLOW_UPLOAD'					=> 'Enable avatar uploading',
	'AVATAR_GALLERY_PATH'			=> 'Avatar gallery path',
	'AVATAR_GALLERY_PATH_EXPLAIN'	=> 'Path under your phpBB root directory for pre-loaded images, e.g. <samp>images/avatars/gallery</samp>.',
	'AVATAR_STORAGE_PATH'			=> 'Avatar storage path',
	'AVATAR_STORAGE_PATH_EXPLAIN'	=> 'Path under your phpBB root directory, e.g. <samp>images/avatars/upload</samp>.',
	'MAX_AVATAR_SIZE'				=> 'Maximum avatar dimensions',
	'MAX_AVATAR_SIZE_EXPLAIN'		=> 'Width x Height in pixels.',
	'MAX_FILESIZE'					=> 'Maximum avatar file size',
	'MAX_FILESIZE_EXPLAIN'			=> 'For uploaded avatar files.',
	'MIN_AVATAR_SIZE'				=> 'Minimum avatar dimensions',
	'MIN_AVATAR_SIZE_EXPLAIN'		=> 'Width x Height in pixels.',
));

// Message Settings
$lang = array_merge($lang, array(
	'ACP_MESSAGE_SETTINGS_EXPLAIN'		=> 'Here you can set all default settings for private messaging.',

	'ALLOW_BBCODE_PM'			=> 'Allow BBCode in private messages',
	'ALLOW_FLASH_PM'			=> 'Allow use of <code>[FLASH]</code> BBCode tag',
	'ALLOW_FLASH_PM_EXPLAIN'	=> 'Note that the ability to use flash in private messages, if enabled here, also depends on the permissions.',
	'ALLOW_FORWARD_PM'			=> 'Allow forwarding of private messages',
	'ALLOW_IMG_PM'				=> 'Allow use of <code>[IMG]</code> BBCode tag',
	'ALLOW_MASS_PM'				=> 'Allow sending of private messages to multiple users and groups',
	'ALLOW_PRINT_PM'			=> 'Allow print view in private messaging',
	'ALLOW_QUOTE_PM'			=> 'Allow quotes in private messages',
	'ALLOW_SIG_PM'				=> 'Allow signature in private messages',
	'ALLOW_SMILIES_PM'			=> 'Allow smilies in private messages',
	'BOXES_LIMIT'				=> 'Maximum private messages per box',
	'BOXES_LIMIT_EXPLAIN'		=> 'Users may receive no more than this many messages in each of their private message boxes. Set this value to 0 to allow unlimited messages.',
	'BOXES_MAX'					=> 'Maximum private message folders',
	'BOXES_MAX_EXPLAIN'			=> 'By default users may create this many personal folders for private messages.',
	'ENABLE_PM_ICONS'			=> 'Enable use of topic icons in private messages',
	'FULL_FOLDER_ACTION'		=> 'Full folder default action',
	'FULL_FOLDER_ACTION_EXPLAIN'=> 'Default action to take if a user’s folder is full assuming the user’s folder action, if set at all, is not applicable. The only exception is for the “Sent messages” folder where the default action is always to delete old messages.',
	'HOLD_NEW_MESSAGES'			=> 'Hold new messages',
	'PM_EDIT_TIME'				=> 'Limit editing time',
	'PM_EDIT_TIME_EXPLAIN'		=> 'Limits the time available to edit a private message not already delivered. Setting the value to 0 disables this behaviour.',
));

// Post Settings
$lang = array_merge($lang, array(
	'ACP_POST_SETTINGS_EXPLAIN'			=> 'Here you can set all default settings for posting.',
	'ALLOW_POST_LINKS'					=> 'Allow links in posts/private messages',
	'ALLOW_POST_LINKS_EXPLAIN'			=> 'If disallowed the <code>[URL]</code> BBCode tag and automatic/magic URLs are disabled.',
	'ALLOW_POST_FLASH'					=> 'Allow use of <code>[FLASH]</code> BBCode tag in posts',
	'ALLOW_POST_FLASH_EXPLAIN'			=> 'If disallowed the <code>[FLASH]</code> BBCode tag is disabled in posts. Otherwise the permission system controls which users can use the <code>[FLASH]</code> BBCode tag.',

	'BUMP_INTERVAL'					=> 'Bump interval',
	'BUMP_INTERVAL_EXPLAIN'			=> 'Number of minutes, hours or days between the last post to a topic and the ability to bump this topic.',
	'CHAR_LIMIT'					=> 'Maximum characters per post',
	'CHAR_LIMIT_EXPLAIN'			=> 'The number of characters allowed within a post. Set to 0 for unlimited characters.',
	'DISPLAY_LAST_EDITED'			=> 'Display last edited time information',
	'DISPLAY_LAST_EDITED_EXPLAIN'	=> 'Choose if the last edited by information to be displayed on posts.',
	'EDIT_TIME'						=> 'Limit editing time',
	'EDIT_TIME_EXPLAIN'				=> 'Limits the time available to edit a new post. Setting the value to 0 disables this behaviour.',
	'FLOOD_INTERVAL'				=> 'Flood interval',
	'FLOOD_INTERVAL_EXPLAIN'		=> 'Number of seconds a user must wait between posting new messages. To enable users to ignore this alter their permissions.',
	'HOT_THRESHOLD'					=> 'Popular topic threshold',
	'HOT_THRESHOLD_EXPLAIN'			=> 'Posts per topic threshold required for the popular topic annotation. Set to 0 to disable popular topics.',
	'MAX_POLL_OPTIONS'				=> 'Maximum number of poll options',
	'MAX_POST_FONT_SIZE'			=> 'Maximum font size per post',
	'MAX_POST_FONT_SIZE_EXPLAIN'	=> 'Maximum font size allowed in a post. Set to 0 for unlimited font size.',
	'MAX_POST_IMG_HEIGHT'			=> 'Maximum image height per post',
	'MAX_POST_IMG_HEIGHT_EXPLAIN'	=> 'Maximum height of an image/flash file in postings. Set to 0 for unlimited size.',
	'MAX_POST_IMG_WIDTH'			=> 'Maximum image width per post',
	'MAX_POST_IMG_WIDTH_EXPLAIN'	=> 'Maximum width of an image/flash file in postings. Set to 0 for unlimited size.',
	'MAX_POST_URLS'					=> 'Maximum links per post',
	'MAX_POST_URLS_EXPLAIN'			=> 'Maximum number of URLs in a post. Set to 0 for unlimited links.',
	'POSTING'						=> 'Posting',
	'POSTS_PER_PAGE'				=> 'Posts per page',
	'QUOTE_DEPTH_LIMIT'				=> 'Maximum nested quotes per post',
	'QUOTE_DEPTH_LIMIT_EXPLAIN'		=> 'Maximum number of nested quotes in a post. Set to 0 for unlimited depth.',
	'SMILIES_LIMIT'					=> 'Maximum smilies per post',
	'SMILIES_LIMIT_EXPLAIN'			=> 'Maximum number of smilies in a post. Set to 0 for unlimited smilies.',
	'TOPICS_PER_PAGE'				=> 'Topics per page',
));

// Signature Settings
$lang = array_merge($lang, array(
	'ACP_SIGNATURE_SETTINGS_EXPLAIN'	=> 'Here you can set all default settings for signatures.',

	'MAX_SIG_FONT_SIZE'				=> 'Maximum signature font size',
	'MAX_SIG_FONT_SIZE_EXPLAIN'		=> 'Maximum font size allowed in user signatures. Set to 0 for unlimited size.',
	'MAX_SIG_IMG_HEIGHT'			=> 'Maximum signature image height',
	'MAX_SIG_IMG_HEIGHT_EXPLAIN'	=> 'Maximum height of an image/flash file in user signatures. Set to 0 for unlimited height.',
	'MAX_SIG_IMG_WIDTH'				=> 'Maximum signature image width',
	'MAX_SIG_IMG_WIDTH_EXPLAIN'		=> 'Maximum width of an image/flash file in user signatures. Set to 0 for unlimited width.',
	'MAX_SIG_LENGTH'				=> 'Maximum signature length',
	'MAX_SIG_LENGTH_EXPLAIN'		=> 'Maximum number of characters in user signatures.',
	'MAX_SIG_SMILIES'				=> 'Maximum smilies per signature',
	'MAX_SIG_SMILIES_EXPLAIN'		=> 'Maximum smilies allowed in user signatures. Set to 0 for unlimited smilies.',
	'MAX_SIG_URLS'					=> 'Maximum signature links',
	'MAX_SIG_URLS_EXPLAIN'			=> 'Maximum number of links in user signatures. Set to 0 for unlimited links.',
));

// Registration Settings
$lang = array_merge($lang, array(
	'ACP_REGISTER_SETTINGS_EXPLAIN'		=> 'Here you are able to define registration and profile related settings.',

	'ACC_ACTIVATION'			=> 'Account activation',
	'ACC_ACTIVATION_EXPLAIN'	=> 'This determines whether users have immediate access to the board or if confirmation is required. You can also completely disable new registrations.',
	'ACC_ADMIN'					=> 'By Admin',
	'ACC_DISABLE'				=> 'Disable',
	'ACC_NONE'					=> 'None',
	'ACC_USER'					=> 'By User',
//	'ACC_USER_ADMIN'			=> 'User + Admin',
	'ALLOW_EMAIL_REUSE'			=> 'Allow e-mail address re-use',
	'ALLOW_EMAIL_REUSE_EXPLAIN'	=> 'Different users can register with the same e-mail address.',
	'COPPA'						=> 'COPPA',
	'COPPA_FAX'					=> 'COPPA fax number',
	'COPPA_MAIL'				=> 'COPPA mailing address',
	'COPPA_MAIL_EXPLAIN'		=> 'This is the mailing address where parents will send COPPA registration forms.',
	'ENABLE_COPPA'				=> 'Enable COPPA',
	'ENABLE_COPPA_EXPLAIN'		=> 'This requires users to declare whether they are 13 or over for compliance with the U.S. COPPA. If this is disabled the COPPA specific groups will no longer be displayed.',
	'MAX_CHARS'					=> 'Max',
	'MIN_CHARS'					=> 'Min',
	'MIN_TIME_REG'				=> 'Minimum time for registration',
	'MIN_TIME_REG_EXPLAIN'		=> 'The registration form cannot be submitted before this time has passed.',
	'MIN_TIME_TERMS'			=> 'Minimum time for accepting terms',
	'MIN_TIME_TERMS_EXPLAIN'	=> 'The terms page cannot be skipped before this time has passed.',
	'NO_AUTH_PLUGIN'			=> 'No suitable auth plugin found.',
	'PASSWORD_LENGTH'			=> 'Password length',
	'PASSWORD_LENGTH_EXPLAIN'	=> 'Minimum and maximum number of characters in passwords.',
	'REG_LIMIT'					=> 'Registration attempts',
	'REG_LIMIT_EXPLAIN'			=> 'Number of attempts users can make at the confirmation code before being locked out that session.',
	'USERNAME_ALPHA_ONLY'		=> 'Alphanumeric only',
	'USERNAME_ALPHA_SPACERS'	=> 'Alphanumeric and spacers',
	'USERNAME_ASCII'			=> 'ASCII (no international unicode)',
	'USERNAME_LETTER_NUM'		=> 'Any letter and number',
	'USERNAME_LETTER_NUM_SPACERS'	=> 'Any letter, number, and spacer',
	'USERNAME_CHARS'			=> 'Limit username chars',
	'USERNAME_CHARS_ANY'		=> 'Any character',
	'USERNAME_CHARS_EXPLAIN'	=> 'Restrict type of characters that may be used in usernames, spacers are: space, -, +, _, [ and ].',
	'USERNAME_LENGTH'			=> 'Username length',
	'USERNAME_LENGTH_EXPLAIN'	=> 'Minimum and maximum number of characters in usernames.',
));

// Visual Confirmation Settings
$lang = array_merge($lang, array(
	'ACP_VC_SETTINGS_EXPLAIN'		=> 'Here you are able to define visual confirmation defaults and CAPTCHA settings.',

	'CAPTCHA_GD'							=> 'GD CAPTCHA',
	'CAPTCHA_GD_FOREGROUND_NOISE'			=> 'GD CAPTCHA foreground noise',
	'CAPTCHA_GD_EXPLAIN'					=> 'Use GD to make a more advanced CAPTCHA.',
	'CAPTCHA_GD_FOREGROUND_NOISE_EXPLAIN'	=> 'Use foreground noise to make the GD based CAPTCHA harder.',
	'CAPTCHA_GD_X_GRID'						=> 'GD CAPTCHA background noise x-axis',
	'CAPTCHA_GD_X_GRID_EXPLAIN'				=> 'Use lower settings of this to make the GD based CAPTCHA harder. 0 will disable x-axis background noise.',
	'CAPTCHA_GD_Y_GRID'						=> 'GD CAPTCHA background noise y-axis',
	'CAPTCHA_GD_Y_GRID_EXPLAIN'				=> 'Use lower settings of this to make the GD based CAPTCHA harder. 0 will disable y-axis background noise.',

	'CAPTCHA_PREVIEW_MSG'					=> 'Your changes to the visual confirmation setting were not saved. This is just a preview.',
	'CAPTCHA_PREVIEW_EXPLAIN'				=> 'The CAPTCHA as it will look like using the current settings. Use the preview button to refresh. Note that captchas are randomized and will differ from one view to the next.',
	'VISUAL_CONFIRM_POST'					=> 'Enable visual confirmation for guest postings',
	'VISUAL_CONFIRM_POST_EXPLAIN'			=> 'Requires anonymous users to enter a random code matching an image to help prevent mass postings.',
	'VISUAL_CONFIRM_REG'					=> 'Enable visual confirmation for registrations',
	'VISUAL_CONFIRM_REG_EXPLAIN'			=> 'Requires new users to enter a random code matching an image to help prevent mass registrations.',
));

// Cookie Settings
$lang = array_merge($lang, array(
	'ACP_COOKIE_SETTINGS_EXPLAIN'		=> 'These details define the data used to send cookies to your users browsers. In most cases the default values for the cookie settings should be sufficient. If you do need to change any do so with care, incorrect settings can prevent users logging in.',

	'COOKIE_DOMAIN'				=> 'Cookie domain',
	'COOKIE_NAME'				=> 'Cookie name',
	'COOKIE_PATH'				=> 'Cookie path',
	'COOKIE_SECURE'				=> 'Cookie secure',
	'COOKIE_SECURE_EXPLAIN'		=> 'If your server is running via SSL set this to enabled else leave as disabled. Having this enabled and not running via SSL will result in server errors during redirects.',
	'ONLINE_LENGTH'				=> 'View online time span',
	'ONLINE_LENGTH_EXPLAIN'		=> 'Number of minutes after which inactive users will not appear in “Who is online” listings. The higher this value the greater is the processing required to generate the listing.',
	'SESSION_LENGTH'			=> 'Session length',
	'SESSION_LENGTH_EXPLAIN'	=> 'Sessions will expire after this time, in seconds.',
));

// Load Settings
$lang = array_merge($lang, array(
	'ACP_LOAD_SETTINGS_EXPLAIN'	=> 'Here you can enable and disable certain board functions to reduce the amount of processing required. On most servers there is no need to disable any functions. However on certain systems or in shared hosting environments it may be beneficial to disable capabilities you do not really need. You can also specify limits for system load and active sessions beyond which the board will go offline.',

	'CUSTOM_PROFILE_FIELDS'			=> 'Custom profile fields',
	'LIMIT_LOAD'					=> 'Limit system load',
	'LIMIT_LOAD_EXPLAIN'			=> 'If the system’s 1-minute load average exceeds this value the board will automatically go offline. A value of 1.0 equals ~100% utilisation of one processor. This only functions on UNIX based servers and where this information is accessible. The value here resets itself to 0 if phpBB was unable to get the load limit.',
	'LIMIT_SESSIONS'				=> 'Limit sessions',
	'LIMIT_SESSIONS_EXPLAIN'		=> 'If the number of sessions exceeds this value within a one minute period the board will go offline. Set to 0 for unlimited sessions.',
	'LOAD_CPF_MEMBERLIST'			=> 'Allow styles to display custom profile fields in memberlist',
	'LOAD_CPF_VIEWPROFILE'			=> 'Display custom profile fields in user profiles',
	'LOAD_CPF_VIEWTOPIC'			=> 'Display custom profile fields on topic pages',
	'LOAD_USER_ACTIVITY'			=> 'Show user’s activity',
	'LOAD_USER_ACTIVITY_EXPLAIN'	=> 'Displays active topic/forum in user profiles and user control panel. It is recommended to disable this on boards with more than one million posts.',
	'RECOMPILE_STYLES'				=> 'Recompile stale style components',
	'RECOMPILE_STYLES_EXPLAIN'		=> 'Check for updated style components on filesystem and recompile.',
	'YES_ANON_READ_MARKING'			=> 'Enable topic marking for guests',
	'YES_ANON_READ_MARKING_EXPLAIN'	=> 'Stores read/unread status information for guests. If disabled posts are always read for guests.',
	'YES_BIRTHDAYS'					=> 'Enable birthday listing',
	'YES_BIRTHDAYS_EXPLAIN'			=> 'If disabled the birthday listing is no longer displayed. To let this setting take effect the birthday feature need to be enabled too.',
	'YES_JUMPBOX'					=> 'Enable display of jumpbox',
	'YES_MODERATORS'				=> 'Enable display of moderators',
	'YES_ONLINE'					=> 'Enable online user listings',
	'YES_ONLINE_EXPLAIN'			=> 'Display online user information on index, forum and topic pages.',
	'YES_ONLINE_GUESTS'				=> 'Enable online guest listings in viewonline',
	'YES_ONLINE_GUESTS_EXPLAIN'		=> 'Allow display of guest user information in viewonline.',
	'YES_ONLINE_TRACK'				=> 'Enable display of user online/offline information',
	'YES_ONLINE_TRACK_EXPLAIN'		=> 'Display online information for user in profiles and topic pages.',
	'YES_POST_MARKING'				=> 'Enable dotted topics',
	'YES_POST_MARKING_EXPLAIN'		=> 'Indicates whether user has posted to a topic.',
	'YES_READ_MARKING'				=> 'Enable server-side topic marking',
	'YES_READ_MARKING_EXPLAIN'		=> 'Stores read/unread status information in the database rather than a cookie.',
));

// Auth settings
$lang = array_merge($lang, array(
	'ACP_AUTH_SETTINGS_EXPLAIN'	=> 'phpBB supports authentication plug-ins, or modules. These allow you determine how users are authenticated when they log into the board. By default three plug-ins are provided; DB, LDAP and Apache. Not all methods require additional information so only fill out fields if they are relevant to the selected method.',

	'AUTH_METHOD'				=> 'Select an authentication method',

	'APACHE_SETUP_BEFORE_USE'	=> 'You have to setup apache authentication before you switch phpBB to this authentication method. Keep in mind that the username you use for apache authentication has to be the same as your phpBB username. Apache authentication can only be used with mod_php (not with a CGI version) and safe_mode disabled.',

	'LDAP_DN'						=> 'LDAP base <var>dn</var>',
	'LDAP_DN_EXPLAIN'				=> 'This is the Distinguished Name, locating the user information, e.g. <samp>o=My Company,c=US</samp>.',
	'LDAP_EMAIL'					=> 'LDAP e-mail attribute',
	'LDAP_EMAIL_EXPLAIN'			=> 'Set this to the name of your user entry e-mail attribute (if one exists) in order to automatically set the e-mail address for new users. Leaving this empty results in empty e-mail address for users who log in for the first time.',
	'LDAP_INCORRECT_USER_PASSWORD'	=> 'Binding to LDAP server failed with specified user/password.',
	'LDAP_NO_EMAIL'					=> 'The specified e-mail attribute does not exist.',
	'LDAP_NO_IDENTITY'				=> 'Could not find a login identity for %s.',
	'LDAP_PASSWORD'					=> 'LDAP password',
	'LDAP_PASSWORD_EXPLAIN'			=> 'Leave blank to use anonymous binding. Else fill in the password for the above user.  Required for Active Directory Servers. <strong>WARNING:</strong> This password will be stored as plain text in the database visible to everybody who can access your database or who can view this configuration page.',
	'LDAP_PORT'						=> 'LDAP server port',
	'LDAP_PORT_EXPLAIN'				=> 'Optionally you can specify a port which should be used to connect to the LDAP server instead of the default port 389.',
	'LDAP_SERVER'					=> 'LDAP server name',
	'LDAP_SERVER_EXPLAIN'			=> 'If using LDAP this is the hostname or IP address of the LDAP server. Alternatively you can specify an URL like ldap://hostname:port/',
	'LDAP_UID'						=> 'LDAP <var>uid</var>',
	'LDAP_UID_EXPLAIN'				=> 'This is the key under which to search for a given login identity, e.g. <var>uid</var>, <var>sn</var>, etc.',
	'LDAP_USER'						=> 'LDAP user <var>dn</var>',
	'LDAP_USER_EXPLAIN'				=> 'Leave blank to use anonymous binding. If filled in phpBB uses the specified distinguished name on login attempts to find the correct user, e.g. <samp>uid=Username,ou=MyUnit,o=MyCompany,c=US</samp>. Required for Active Directory Servers.',
	'LDAP_USER_FILTER'				=> 'LDAP user filter',
	'LDAP_USER_FILTER_EXPLAIN'		=> 'Optionally you can further limit the searched objects with additional filters. For example <samp>objectClass=posixGroup</samp> would result in the use of <samp>(&amp;(uid=$username)(objectClass=posixGroup))</samp>',
));

// Server Settings
$lang = array_merge($lang, array(
	'ACP_SERVER_SETTINGS_EXPLAIN'	=> 'Here you define server and domain dependant settings. Please ensure the data you enter is accurate, errors will result in e-mails containing incorrect information. When entering the domain name remember it does include http:// or other protocol term. Only alter the port number if you know your server uses a different value, port 80 is correct in most cases.',

	'ENABLE_GZIP'				=> 'Enable GZip compression',
	'ENABLE_GZIP_EXPLAIN'		=> 'Generated content will be compressed prior to sending it to the user. This can reduce network traffic but will also increase CPU usage on both server and client side.',
	'FORCE_SERVER_VARS'			=> 'Force server URL settings',
	'FORCE_SERVER_VARS_EXPLAIN'	=> 'If set to yes the server settings defined here will be used in favour of the automatically determined values.',
	'ICONS_PATH'				=> 'Post icons storage path',
	'ICONS_PATH_EXPLAIN'		=> 'Path under your phpBB root directory, e.g. <samp>images/icons</samp>.',
	'PATH_SETTINGS'				=> 'Path settings',
	'RANKS_PATH'				=> 'Rank image storage path',
	'RANKS_PATH_EXPLAIN'		=> 'Path under your phpBB root directory, e.g. <samp>images/ranks</samp>.',
	'SCRIPT_PATH'				=> 'Script path',
	'SCRIPT_PATH_EXPLAIN'		=> 'The path where phpBB is located relative to the domain name, e.g. <samp>/phpBB3</samp>.',
	'SERVER_NAME'				=> 'Domain name',
	'SERVER_NAME_EXPLAIN'		=> 'The domain name this board runs from (for example: <samp>www.example.com</samp>).',
	'SERVER_PORT'				=> 'Server port',
	'SERVER_PORT_EXPLAIN'		=> 'The port your server is running on, usually 80, only change if different.',
	'SERVER_PROTOCOL'			=> 'Server protocol',
	'SERVER_PROTOCOL_EXPLAIN'	=> 'This is used as the server protocol if these settings are forced. If empty or not forced the protocol is determined by the cookie secure settings (<samp>http://</samp> or <samp>https://</samp>).',
	'SERVER_URL_SETTINGS'		=> 'Server URL settings',
	'SMILIES_PATH'				=> 'Smilies storage path',
	'SMILIES_PATH_EXPLAIN'		=> 'Path under your phpBB root directory, e.g. <samp>images/smilies</samp>.',
	'UPLOAD_ICONS_PATH'			=> 'Extension group icons storage path',
	'UPLOAD_ICONS_PATH_EXPLAIN'	=> 'Path under your phpBB root directory, e.g. <samp>images/upload_icons</samp>.',
));

// Security Settings
$lang = array_merge($lang, array(
	'ACP_SECURITY_SETTINGS_EXPLAIN'		=> 'Here you are able to define session and login related settings.',

	'ALL'							=> 'All',
	'ALLOW_AUTOLOGIN'				=> 'Allow persistent logins',
	'ALLOW_AUTOLOGIN_EXPLAIN'		=> 'Determines whether users can autologin when they visit the board.',
	'AUTOLOGIN_LENGTH'				=> 'Persistent login key expiration length (in days)',
	'AUTOLOGIN_LENGTH_EXPLAIN'		=> 'Number of days after which persistent login keys are removed or zero to disable.',
	'BROWSER_VALID'					=> 'Validate browser',
	'BROWSER_VALID_EXPLAIN'			=> 'Enables browser validation for each session improving security.',
	'CHECK_DNSBL'					=> 'Check IP against DNS Blackhole List',
	'CHECK_DNSBL_EXPLAIN'			=> 'If enabled the user’s IP address is checked against the following DNSBL services on registration and posting: <a href="http://spamcop.net">spamcop.net</a>, <a href="http://dsbl.org">dsbl.org</a> and <a href="http://www.spamhaus.org">www.spamhaus.org</a>. This lookup may take a while, depending on the server’s configuration. If slowdowns are experienced or too many false positives reported it is recommended to disable this check.',
	'CLASS_B'						=> 'A.B',
	'CLASS_C'						=> 'A.B.C',
	'EMAIL_CHECK_MX'				=> 'Check e-mail domain for valid MX record',
	'EMAIL_CHECK_MX_EXPLAIN'		=> 'If enabled, the e-mail domain provided on registration and profile changes is checked for a valid MX record.',
	'FORCE_PASS_CHANGE'				=> 'Force password change',
	'FORCE_PASS_CHANGE_EXPLAIN'		=> 'Require user to change their password after a set number of days. Setting this value to 0 disables this behaviour.',
	'FORM_TIME_MAX'					=> 'Maximum time to submit forms',
	'FORM_TIME_MAX_EXPLAIN'			=> 'The time a user has to submit a form. Use -1 to disable. Note that a form might become invalid if the session expires, regardless of this setting.',
	'FORM_TIME_MIN'					=> 'Minimum time to submit forms',
	'FORM_TIME_MIN_EXPLAIN'			=> 'Submissions faster than this time are ignored by the board. Use 0 to disable.',
	'FORM_SID_GUESTS'				=> 'Tie forms to guest sessions',
	'FORM_SID_GUESTS_EXPLAIN'		=> 'If enabled, the form token issued to guests will be session-exclusive. This can cause problems with some ISPs.',
	'FORWARDED_FOR_VALID'			=> 'Validated <var>X_FORWARDED_FOR</var> header',
	'FORWARDED_FOR_VALID_EXPLAIN'	=> 'Sessions will only be continued if the sent <var>X_FORWARDED_FOR</var> header equals the one sent with the previous request. Bans will be checked against IPs in <var>X_FORWARDED_FOR</var> too.',
	'IP_VALID'						=> 'Session IP validation',
	'IP_VALID_EXPLAIN'				=> 'Determines how much of the users IP is used to validate a session; <samp>All</samp> compares the complete address, <samp>A.B.C</samp> the first x.x.x, <samp>A.B</samp> the first x.x, <samp>None</samp> disables checking. On IPv6 addresses <samp>A.B.C</samp> compares the first 4 blocks and <samp>A.B</samp> the first 3 blocks.',
	'MAX_LOGIN_ATTEMPTS'			=> 'Maximum number of login attempts',
	'MAX_LOGIN_ATTEMPTS_EXPLAIN'	=> 'After this number of failed logins the user needs to additionally confirm his login visually (visual confirmation).',
	'NO_IP_VALIDATION'				=> 'None',
	'PASSWORD_TYPE'					=> 'Password complexity',
	'PASSWORD_TYPE_EXPLAIN'			=> 'Determines how complex a password needs to be when set or altered, subsequent options include the previous ones.',
	'PASS_TYPE_ALPHA'				=> 'Must contain letters and numbers',
	'PASS_TYPE_ANY'					=> 'No requirements',
	'PASS_TYPE_CASE'				=> 'Must be mixed case',
	'PASS_TYPE_SYMBOL'				=> 'Must contain symbols',
	'TPL_ALLOW_PHP'					=> 'Allow php in templates',
	'TPL_ALLOW_PHP_EXPLAIN'			=> 'If this option is enabled, <code>PHP</code> and <code>INCLUDEPHP</code> statements will be recognised and parsed in templates.',
));

// Email Settings
$lang = array_merge($lang, array(
	'ACP_EMAIL_SETTINGS_EXPLAIN'	=> 'This information is used when the board sends e-mails to your users. Please ensure the e-mail address you specify is valid, any bounced or undeliverable messages will likely be sent to that address. If your host does not provide a native (PHP based) e-mail service you can instead send messages directly using SMTP. This requires the address of an appropriate server (ask your provider if necessary). If the server requires authentication (and only if it does) enter the necessary username, password and authentication method.',

	'ADMIN_EMAIL'					=> 'Return e-mail address',
	'ADMIN_EMAIL_EXPLAIN'			=> 'This will be used as the return address on all e-mails, the technical contact e-mail address. It will always be used as the <samp>Return-Path</samp> and <samp>Sender</samp> address in e-mails.',
	'BOARD_EMAIL_FORM'				=> 'Users send e-mail via board',
	'BOARD_EMAIL_FORM_EXPLAIN'		=> 'Instead of showing the users e-mail address users are able to send e-mails via the board.',
	'BOARD_HIDE_EMAILS'				=> 'Hide e-mail addresses',
	'BOARD_HIDE_EMAILS_EXPLAIN'		=> 'This function keeps e-mail addresses completely private.',
	'CONTACT_EMAIL'					=> 'Contact e-mail address',
	'CONTACT_EMAIL_EXPLAIN'			=> 'This address will be used whenever a specific contact point is needed, e.g. spam, error output, etc. It will always be used as the <samp>From</samp> and <samp>Reply-To</samp> address in e-mails.',
	'EMAIL_FUNCTION_NAME'			=> 'E-mail function name',
	'EMAIL_FUNCTION_NAME_EXPLAIN'	=> 'The e-mail function used to send mails through PHP.',
	'EMAIL_PACKAGE_SIZE'			=> 'E-mail package size',
	'EMAIL_PACKAGE_SIZE_EXPLAIN'	=> 'This is the number of maximum e-mails sent out in one package. This setting is applied to the internal message queue; set this value to 0 if you have problems with non-delivered notification emails.',
	'EMAIL_SIG'						=> 'E-mail signature',
	'EMAIL_SIG_EXPLAIN'				=> 'This text will be attached to all e-mails the board sends.',
	'ENABLE_EMAIL'					=> 'Enable board-wide e-mails',
	'ENABLE_EMAIL_EXPLAIN'			=> 'If this is set to disabled no e-mails will be sent by the board at all.',
	'SMTP_AUTH_METHOD'				=> 'Authentication method for SMTP',
	'SMTP_AUTH_METHOD_EXPLAIN'		=> 'Only used if a username/password is set, ask your provider if you are unsure which method to use.',
	'SMTP_CRAM_MD5'					=> 'CRAM-MD5',
	'SMTP_DIGEST_MD5'				=> 'DIGEST-MD5',
	'SMTP_LOGIN'					=> 'LOGIN',
	'SMTP_PASSWORD'					=> 'SMTP password',
	'SMTP_PASSWORD_EXPLAIN'			=> 'Only enter a password if your SMTP server requires it.',
	'SMTP_PLAIN'					=> 'PLAIN',
	'SMTP_POP_BEFORE_SMTP'			=> 'POP-BEFORE-SMTP',
	'SMTP_PORT'						=> 'SMTP server port',
	'SMTP_PORT_EXPLAIN'				=> 'Only change this if you know your SMTP server is on a different port.',
	'SMTP_SERVER'					=> 'SMTP server address',
	'SMTP_SETTINGS'					=> 'SMTP settings',
	'SMTP_USERNAME'					=> 'SMTP username',
	'SMTP_USERNAME_EXPLAIN'			=> 'Only enter a username if your SMTP server requires it.',
	'USE_SMTP'						=> 'Use SMTP server for e-mail',
	'USE_SMTP_EXPLAIN'				=> 'Select “Yes” if you want or have to send e-mail via a named server instead of the local mail function.',
));

// Jabber settings
$lang = array_merge($lang, array(
	'ACP_JABBER_SETTINGS_EXPLAIN'	=> 'Here you can enable and control the use of Jabber for instant messaging and board notifications. Jabber is an open source protocol and therefore available for use by anyone. Some Jabber servers include gateways or transports which allow you to contact users on other networks. Not all servers offer all transports and changes in protocols can prevent transports from operating. Please be sure to enter already registered account details - phpBB will use the details you enter here as is.',

	'JAB_ENABLE'				=> 'Enable Jabber',
	'JAB_ENABLE_EXPLAIN'		=> 'Enables use of Jabber messaging and notifications.',
	'JAB_GTALK_NOTE'			=> 'Please note that GTalk will not work because the <samp>dns_get_record</samp> function could not be found. This function is not available in PHP4, and is not implemented on Windows platforms. It currently does not work on BSD-based systems, including Mac OS.',
	'JAB_PACKAGE_SIZE'			=> 'Jabber package size',
	'JAB_PACKAGE_SIZE_EXPLAIN'	=> 'This is the number of messages sent in one package. If set to 0 the message is sent immediately and will not be queued for later sending.',
	'JAB_PASSWORD'				=> 'Jabber password',
	'JAB_PORT'					=> 'Jabber port',
	'JAB_PORT_EXPLAIN'			=> 'Leave blank unless you know it is not port 5222.',
	'JAB_SERVER'				=> 'Jabber server',
	'JAB_SERVER_EXPLAIN'		=> 'See %sjabber.org%s for a list of servers.',
	'JAB_SETTINGS_CHANGED'		=> 'Jabber settings changed successfully.',
	'JAB_USE_SSL'				=> 'Use SSL to connect',
	'JAB_USE_SSL_EXPLAIN'		=> 'If enabled a secure connection is tried to be established. The Jabber port will be modified to 5223 if port 5222 is specified.',
	'JAB_USERNAME'				=> 'Jabber username',
	'JAB_USERNAME_EXPLAIN'		=> 'Specify a registered username. The username will not be checked for validity.',
));

?>