<?php
/**
*
* acp_users [English]
*
* @package language
* @version $Id: users.php 8479 2008-03-29 00:22:48Z naderman $
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
	'ADMIN_SIG_PREVIEW'		=> 'Signature preview',
	'AT_LEAST_ONE_FOUNDER'	=> 'You are not able to change this founder to a normal user. There needs to be at least one founder enabled for this board. If you want to change this users founder status, promote another user to be a founder first.',

	'BAN_ALREADY_ENTERED'	=> 'The ban had been previously entered successfully. The ban list has not been updated.',
	'BAN_SUCCESSFUL'		=> 'Ban entered successfully.',

	'CANNOT_BAN_FOUNDER'			=> 'You are not allowed to ban founder accounts.',
	'CANNOT_BAN_YOURSELF'			=> 'You are not allowed to ban yourself.',
	'CANNOT_DEACTIVATE_BOT'			=> 'You are not allowed to deactivate bot accounts. Please deactivate the bot within the bots page instead.',
	'CANNOT_DEACTIVATE_FOUNDER'		=> 'You are not allowed to deactivate founder accounts.',
	'CANNOT_DEACTIVATE_YOURSELF'	=> 'You are not allowed to deactivate your own account.',
	'CANNOT_FORCE_REACT_BOT'		=> 'You are not allowed to force reactivation on bot accounts. Please reactivate the bot within the bots page instead.',
	'CANNOT_FORCE_REACT_FOUNDER'	=> 'You are not allowed to force reactivation on founder accounts.',
	'CANNOT_FORCE_REACT_YOURSELF'	=> 'You are not allowed to force reactivation of your own account.',
	'CANNOT_REMOVE_ANONYMOUS'		=> 'You are not able to remove the guest user account.',
	'CANNOT_REMOVE_YOURSELF'		=> 'You are not allowed to remove your own user account.',
	'CANNOT_SET_FOUNDER_IGNORED'	=> 'You are not able to promote ignored users to be founders.',
	'CANNOT_SET_FOUNDER_INACTIVE'	=> 'You need to activate users before you promote them to founders, only activated users are able to be promoted.',
	'CONFIRM_EMAIL_EXPLAIN'			=> 'You only need to specify this if you are changing the users e-mail address.',

	'DELETE_POSTS'			=> 'Delete posts',
	'DELETE_USER'			=> 'Delete user',
	'DELETE_USER_EXPLAIN'	=> 'Please note that deleting a user is final, they cannot be recovered.',

	'FORCE_REACTIVATION_SUCCESS'	=> 'Successfully forced reactivation.',
	'FOUNDER'						=> 'Founder',
	'FOUNDER_EXPLAIN'				=> 'Founders have all administrative permissions and can never be banned, deleted or altered by non-founder members.',

	'GROUP_APPROVE'					=> 'Approve member',
	'GROUP_DEFAULT'					=> 'Make group default for member',
	'GROUP_DELETE'					=> 'Remove member from group',
	'GROUP_DEMOTE'					=> 'Demote group leader',
	'GROUP_PROMOTE'					=> 'Promote to group leader',

	'IP_WHOIS_FOR'			=> 'IP whois for %s',

	'LAST_ACTIVE'			=> 'Last active',

	'MOVE_POSTS_EXPLAIN'	=> 'Please select the forum to which you wish to move all the posts this user has made.',

	'NO_SPECIAL_RANK'		=> 'No special rank assigned',
	'NOT_MANAGE_FOUNDER'	=> 'You tried to manage a user with founder status. Only founders are allowed to manage other founders.',

	'QUICK_TOOLS'			=> 'Quick tools',

	'REGISTERED'			=> 'Registered',
	'REGISTERED_IP'			=> 'Registered from IP',
	'RETAIN_POSTS'			=> 'Retain posts',

	'SELECT_FORM'			=> 'Select form',
	'SELECT_USER'			=> 'Select user',

	'USER_ADMIN'					=> 'User administration',
	'USER_ADMIN_ACTIVATE'			=> 'Activate account',
	'USER_ADMIN_ACTIVATED'			=> 'User activated successfully.',
	'USER_ADMIN_AVATAR_REMOVED'		=> 'Successfully removed avatar from user account.',
	'USER_ADMIN_BAN_EMAIL'			=> 'Ban by e-mail',
	'USER_ADMIN_BAN_EMAIL_REASON'	=> 'E-mail address banned via user management',
	'USER_ADMIN_BAN_IP'				=> 'Ban by IP',
	'USER_ADMIN_BAN_IP_REASON'		=> 'IP banned via user management',
	'USER_ADMIN_BAN_NAME_REASON'	=> 'Username banned via user management',
	'USER_ADMIN_BAN_USER'			=> 'Ban by username',
	'USER_ADMIN_DEACTIVATE'			=> 'Deactivate account',
	'USER_ADMIN_DEACTIVED'			=> 'User deactivated successfully.',
	'USER_ADMIN_DEL_ATTACH'			=> 'Delete all attachments',
	'USER_ADMIN_DEL_AVATAR'			=> 'Delete avatar',
	'USER_ADMIN_DEL_POSTS'			=> 'Delete all posts',
	'USER_ADMIN_DEL_SIG'			=> 'Delete signature',
	'USER_ADMIN_EXPLAIN'			=> 'Here you can change your users information and certain specific options.',
	'USER_ADMIN_FORCE'				=> 'Force reactivation',
	'USER_ADMIN_MOVE_POSTS'			=> 'Move all posts',
	'USER_ADMIN_SIG_REMOVED'		=> 'Successfully removed signature from user account.',
	'USER_ATTACHMENTS_REMOVED'		=> 'Successfully removed all attachments made by this user.',
	'USER_AVATAR_UPDATED'			=> 'Successfully updated user avatars details.',
	'USER_CUSTOM_PROFILE_FIELDS'	=> 'Custom profile fields',
	'USER_DELETED'					=> 'User deleted successfully.',
	'USER_GROUP_ADD'				=> 'Add user to group',
	'USER_GROUP_NORMAL'				=> 'User defined groups user is a member of',
	'USER_GROUP_PENDING'			=> 'Groups user is in pending mode',
	'USER_GROUP_SPECIAL'			=> 'Pre-defined groups user is a member of',
	'USER_NO_ATTACHMENTS'			=> 'There are no attached files to display.',
	'USER_OVERVIEW_UPDATED'			=> 'User details updated.',
	'USER_POSTS_DELETED'			=> 'Successfully removed all posts made by this user.',
	'USER_POSTS_MOVED'				=> 'Successfully moved users posts to target forum.',
	'USER_PREFS_UPDATED'			=> 'User preferences updated.',
	'USER_PROFILE'					=> 'User profile',
	'USER_PROFILE_UPDATED'			=> 'User profile updated.',
	'USER_RANK'						=> 'User rank',
	'USER_RANK_UPDATED'				=> 'User rank updated.',
	'USER_SIG_UPDATED'				=> 'User signature successfully updated.',
	'USER_TOOLS'					=> 'Basic tools',
));

?>