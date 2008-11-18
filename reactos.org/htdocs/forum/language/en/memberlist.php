<?php
/**
*
* memberlist [English]
*
* @package language
* @version $Id: memberlist.php 8479 2008-03-29 00:22:48Z naderman $
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

$lang = array_merge($lang, array(
	'ABOUT_USER'			=> 'Profile',
	'ACTIVE_IN_FORUM'		=> 'Most active forum',
	'ACTIVE_IN_TOPIC'		=> 'Most active topic',
	'ADD_FOE'				=> 'Add foe',
	'ADD_FRIEND'			=> 'Add friend',
	'AFTER'					=> 'After',

	'ALL'					=> 'All',

	'BEFORE'				=> 'Before',

	'CC_EMAIL'				=> 'Send a copy of this e-mail to yourself.',
	'CONTACT_USER'			=> 'Contact',

	'DEST_LANG'				=> 'Language',
	'DEST_LANG_EXPLAIN'		=> 'Select an appropriate language (if available) for the recipient of this message.',

	'EMAIL_BODY_EXPLAIN'	=> 'This message will be sent as plain text, do not include any HTML or BBCode. The return address for this message will be set to your e-mail address.',
	'EMAIL_DISABLED'		=> 'Sorry but all e-mail related functions have been disabled.',
	'EMAIL_SENT'			=> 'The e-mail has been sent.',
	'EMAIL_TOPIC_EXPLAIN'	=> 'This message will be sent as plain text, do not include any HTML or BBCode. Please note that the topic information is already included in the message. The return address for this message will be set to your e-mail address.',
	'EMPTY_ADDRESS_EMAIL'	=> 'You must provide a valid e-mail address for the recipient.',
	'EMPTY_MESSAGE_EMAIL'	=> 'You must enter a message to be emailed.',
	'EMPTY_MESSAGE_IM'		=> 'You must enter a message to be send.',
	'EMPTY_NAME_EMAIL'		=> 'You must enter the real name of the recipient.',
	'EMPTY_SUBJECT_EMAIL'	=> 'You must specify a subject for the e-mail.',
	'EQUAL_TO'				=> 'Equal to',

	'FIND_USERNAME_EXPLAIN'	=> 'Use this form to search for specific members. You do not need to fill out all fields. To match partial data use * as a wildcard. When entering dates use the format <kbd>YYYY-MM-DD</kbd>, e.g. <samp>2004-02-29</samp>. Use the mark checkboxes to select one or more usernames (several usernames may be accepted depending on the form itself) and click the Select Marked button to return to the previous form.',
	'FLOOD_EMAIL_LIMIT'		=> 'You cannot send another e-mail at this time. Please try again later.',

	'GROUP_LEADER'			=> 'Group leader',

	'HIDE_MEMBER_SEARCH'	=> 'Hide member search',

	'IM_ADD_CONTACT'		=> 'Add Contact',
	'IM_AIM'				=> 'Please note that you need AOL Instant Messenger installed to use this.',
	'IM_AIM_EXPRESS'		=> 'AIM Express',
	'IM_DOWNLOAD_APP'		=> 'Download application',
	'IM_ICQ'				=> 'Please note that users may have selected to not receive unsolicited instant messages.',
	'IM_JABBER'				=> 'Please note that users may have selected to not receive unsolicited instant messages.',
	'IM_JABBER_SUBJECT'		=> 'This is an automated message please do not reply! Message from user %1$s at %2$s.',
	'IM_MESSAGE'			=> 'Your message',
	'IM_MSNM'				=> 'Please note that you need Windows Messenger installed to use this.',
	'IM_MSNM_BROWSER'		=> 'Your browser does not support this.',
	'IM_MSNM_CONNECT'		=> 'MSNM is not connected.\nYou have to connect to MSNM to continue.',		
	'IM_NAME'				=> 'Your Name',
	'IM_NO_DATA'			=> 'There is no suitable contact information for this user.',
	'IM_NO_JABBER'			=> 'Sorry, direct messaging of Jabber users is not supported on this board. You will need a Jabber client installed on your system to contact the recipient above.',
	'IM_RECIPIENT'			=> 'Recipient',
	'IM_SEND'				=> 'Send message',
	'IM_SEND_MESSAGE'		=> 'Send message',
	'IM_SENT_JABBER'		=> 'Your message to %1$s has been sent successfully.',
	'IM_USER'				=> 'Send an instant message',
	
	'LAST_ACTIVE'				=> 'Last active',
	'LESS_THAN'					=> 'Less than',
	'LIST_USER'					=> '1 user',
	'LIST_USERS'				=> '%d users',
	'LOGIN_EXPLAIN_LEADERS'		=> 'The board requires you to be registered and logged in to view the team listing.',
	'LOGIN_EXPLAIN_MEMBERLIST'	=> 'The board requires you to be registered and logged in to access the memberlist.',
	'LOGIN_EXPLAIN_SEARCHUSER'	=> 'The board requires you to be registered and logged in to search users.',
	'LOGIN_EXPLAIN_VIEWPROFILE'	=> 'The board requires you to be registered and logged in to view profiles.',

	'MORE_THAN'				=> 'More than',

	'NO_EMAIL'				=> 'You are not permitted to send e-mail to this user.',
	'NO_VIEW_USERS'			=> 'You are not authorised to view the member list or profiles.',

	'ORDER'					=> 'Order',
	'OTHER'					=> 'Other',

	'POST_IP'				=> 'Posted from IP/domain',

	'RANK'					=> 'Rank',
	'REAL_NAME'				=> 'Recipient name',
	'RECIPIENT'				=> 'Recipient',
	'REMOVE_FOE'			=> 'Remove foe',
	'REMOVE_FRIEND'			=> 'Remove friend',

	'SEARCH_USER_POSTS'		=> 'Search user’s posts',
	'SELECT_MARKED'			=> 'Select marked',
	'SELECT_SORT_METHOD'	=> 'Select sort method',
	'SEND_AIM_MESSAGE'		=> 'Send AIM message',
	'SEND_ICQ_MESSAGE'		=> 'Send ICQ message',
	'SEND_IM'				=> 'Instant messaging',
	'SEND_JABBER_MESSAGE'	=> 'Send Jabber message',
	'SEND_MESSAGE'			=> 'Message',
	'SEND_MSNM_MESSAGE'		=> 'Send MSNM/WLM message',
	'SEND_YIM_MESSAGE'		=> 'Send YIM message',
	'SORT_EMAIL'			=> 'E-mail',
	'SORT_LAST_ACTIVE'		=> 'Last active',
	'SORT_POST_COUNT'		=> 'Post count',

	'USERNAME_BEGINS_WITH'	=> 'Username begins with',
	'USER_ADMIN'			=> 'Administrate user',
	'USER_FORUM'			=> 'User statistics',
	'USER_ONLINE'			=> 'Online',
	'USER_PRESENCE'			=> 'Board presence',

	'VIEWING_PROFILE'		=> 'Viewing profile - %s',
	'VISITED'				=> 'Last visited',

	'WWW'					=> 'Website',
));

?>