<?php
/**
*
* acp_groups [English]
*
* @package language
* @version $Id: groups.php 8479 2008-03-29 00:22:48Z naderman $
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
	'ACP_GROUPS_MANAGE_EXPLAIN'		=> 'From this panel you can administrate all your usergroups. You can delete, create and edit existing groups. Furthermore, you may choose group leaders, toggle open/hidden/closed group status and set the group name and description.',
	'ADD_USERS'						=> 'Add users',
	'ADD_USERS_EXPLAIN'				=> 'Here you can add new users to the group. You may select whether this group becomes the new default for the selected users. Additionally you can define them as group leaders. Please enter each username on a separate line.',

	'COPY_PERMISSIONS'				=> 'Copy permissions from',
	'COPY_PERMISSIONS_EXPLAIN'		=> 'Once created, the group will have the same permissions as the one you select here.',
	'CREATE_GROUP'					=> 'Create new group',

	'GROUPS_NO_MEMBERS'				=> 'This group has no members',
	'GROUPS_NO_MODS'				=> 'No group leaders defined',

	'GROUP_APPROVE'					=> 'Approve member',
	'GROUP_APPROVED'				=> 'Approved members',
	'GROUP_AVATAR'					=> 'Group avatar',
	'GROUP_AVATAR_EXPLAIN'			=> 'This image will be displayed in the Group Control Panel.',
	'GROUP_CLOSED'					=> 'Closed',
	'GROUP_COLOR'					=> 'Group colour',
	'GROUP_COLOR_EXPLAIN'			=> 'Defines the colour members’ usernames will appear in, leave blank for user default.',
	'GROUP_CONFIRM_ADD_USER'		=> 'Are you sure that you want to add the user %1$s to the group?',
	'GROUP_CONFIRM_ADD_USERS'		=> 'Are you sure that you want to add the users %1$s to the group?',
	'GROUP_CREATED'					=> 'Group has been created successfully.',
	'GROUP_DEFAULT'					=> 'Make group default for member',
	'GROUP_DEFS_UPDATED'			=> 'Default group set for all selected members.',
	'GROUP_DELETE'					=> 'Remove member from group',
	'GROUP_DELETED'					=> 'Group deleted and user default groups set successfully.',
	'GROUP_DEMOTE'					=> 'Demote group leader',
	'GROUP_DESC'					=> 'Group description',
	'GROUP_DETAILS'					=> 'Group details',
	'GROUP_EDIT_EXPLAIN'			=> 'Here you can edit an existing group. You can change its name, description and type (open, closed, etc.). You can also set certain group wide options such as colouration, rank, etc. Changes made here override users’ current settings. Please note that group members can override group-avatar settings, unless you set appropriate user permissions.',
	'GROUP_ERR_USERS_EXIST'			=> 'The specified users are already members of this group.',
	'GROUP_FOUNDER_MANAGE'			=> 'Founder manage only',
	'GROUP_FOUNDER_MANAGE_EXPLAIN'	=> 'Restrict management of this group to founders only. Users having group permissions are still able to see this group as well as this group’s members.',
	'GROUP_HIDDEN'					=> 'Hidden',
	'GROUP_LANG'					=> 'Group language',
	'GROUP_LEAD'					=> 'Group leaders',
	'GROUP_LEADERS_ADDED'			=> 'New leaders added to group successfully.',
	'GROUP_LEGEND'					=> 'Display group in legend',
	'GROUP_LIST'					=> 'Current members',
	'GROUP_LIST_EXPLAIN'			=> 'This is a complete list of all the current users with membership of this group. You can delete members (except in certain special groups) or add new ones as you see fit.',
	'GROUP_MEMBERS'					=> 'Group members',
	'GROUP_MEMBERS_EXPLAIN'			=> 'This is a complete listing of all the members of this usergroup. It includes separate sections for leaders, pending and existing members. From here you can manage all aspects of who has membership of this group and what their role is. To remove a leader but keep them in the group use Demote rather than delete. Similarly use Promote to make an existing member a leader.',
	'GROUP_MESSAGE_LIMIT'			=> 'Group private message limit per folder',
	'GROUP_MESSAGE_LIMIT_EXPLAIN'	=> 'This setting overrides the per-user folder message limit. A value of 0 means the user default limit will be used.',
	'GROUP_MODS_ADDED'				=> 'New group leaders added successfully.',
	'GROUP_MODS_DEMOTED'			=> 'Group leaders demoted successfully.',
	'GROUP_MODS_PROMOTED'			=> 'Group members promoted successfully.',
	'GROUP_NAME'					=> 'Group name',
	'GROUP_NAME_TAKEN'				=> 'The group name you entered is already in use, please select an alternative.',
	'GROUP_OPEN'					=> 'Open',
	'GROUP_PENDING'					=> 'Pending members',
	'GROUP_PROMOTE'					=> 'Promote to group leader',
	'GROUP_RANK'					=> 'Group rank',
	'GROUP_RECEIVE_PM'				=> 'Group able to receive private messages',
	'GROUP_RECEIVE_PM_EXPLAIN'		=> 'Please note that hidden groups are not able to be messaged, regardless of this setting.',
	'GROUP_REQUEST'					=> 'Request',
	'GROUP_SETTINGS_SAVE'			=> 'Group wide settings',
	'GROUP_TYPE'					=> 'Group type',
	'GROUP_TYPE_EXPLAIN'			=> 'This determines which users can join or view this group.',
	'GROUP_UPDATED'					=> 'Group preferences updated successfully.',
	
	'GROUP_USERS_ADDED'				=> 'New users added to group successfully.',
	'GROUP_USERS_EXIST'				=> 'The selected users are already members.',
	'GROUP_USERS_REMOVE'			=> 'Users removed from group and new defaults set successfully.',

	'MAKE_DEFAULT_FOR_ALL'	=> 'Make default group for every member',
	'MEMBERS'				=> 'Members',

	'NO_GROUP'					=> 'No group specified.',
	'NO_GROUPS_CREATED'			=> 'No groups created yet.',
	'NO_PERMISSIONS'			=> 'Do not copy permissions',
	'NO_USERS'					=> 'You haven’t entered any users.',
	'NO_USERS_ADDED'			=> 'No users were added to the group.',

	'SPECIAL_GROUPS'			=> 'Pre-defined groups',
	'SPECIAL_GROUPS_EXPLAIN'	=> 'Pre-defined groups are special groups, they cannot be deleted or directly modified. However you can still add users and alter basic settings.',

	'TOTAL_MEMBERS'				=> 'Members',

	'USERS_APPROVED'				=> 'Users approved successfully.',
	'USER_DEFAULT'					=> 'User default',
	'USER_DEF_GROUPS'				=> 'User defined groups',
	'USER_DEF_GROUPS_EXPLAIN'		=> 'These are groups created by you or another admin on this board. You can manage memberships as well as edit group properties or even delete the group.',
	'USER_GROUP_DEFAULT'			=> 'Set as default group',
	'USER_GROUP_DEFAULT_EXPLAIN'	=> 'Saying yes here will set this group as the default group for the added users.',
	'USER_GROUP_LEADER'				=> 'Set as group leader',
));

?>