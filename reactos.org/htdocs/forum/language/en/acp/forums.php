<?php
/**
*
* acp_forums [English]
*
* @package language
* @version $Id: forums.php 8479 2008-03-29 00:22:48Z naderman $
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

// Forum Admin
$lang = array_merge($lang, array(
	'AUTO_PRUNE_DAYS'			=> 'Auto-prune post age',
	'AUTO_PRUNE_DAYS_EXPLAIN'	=> 'Number of days since last post after which topic is removed.',
	'AUTO_PRUNE_FREQ'			=> 'Auto-prune frequency',
	'AUTO_PRUNE_FREQ_EXPLAIN'	=> 'Time in days between pruning events.',
	'AUTO_PRUNE_VIEWED'			=> 'Auto-prune post viewed age',
	'AUTO_PRUNE_VIEWED_EXPLAIN'	=> 'Number of days since topic was viewed after which topic is removed.',

	'COPY_PERMISSIONS'				=> 'Copy permissions from',
	'COPY_PERMISSIONS_ADD_EXPLAIN'	=> 'Once created, the forum will have the same permissions as the one you select here. If no forum is selected the newly created forum will not be visible until permissions had been set.',
	'COPY_PERMISSIONS_EDIT_EXPLAIN'	=> 'If you select to copy permissions, the forum will have the same permissions as the one you select here. This will overwrite any permissions you have previously set for this forum with the permissions of the forum you select here. If no forum is selected the current permissions will be kept.',
	'CREATE_FORUM'					=> 'Create new forum',

	'DECIDE_MOVE_DELETE_CONTENT'		=> 'Delete content or move to forum',
	'DECIDE_MOVE_DELETE_SUBFORUMS'		=> 'Delete subforums or move to forum',
	'DEFAULT_STYLE'						=> 'Default style',
	'DELETE_ALL_POSTS'					=> 'Delete posts',
	'DELETE_SUBFORUMS'					=> 'Delete subforums and posts',
	'DISPLAY_ACTIVE_TOPICS'				=> 'Enable active topics',
	'DISPLAY_ACTIVE_TOPICS_EXPLAIN'		=> 'If set to yes active topics in selected subforums will be displayed under this category.',

	'EDIT_FORUM'					=> 'Edit forum',
	'ENABLE_INDEXING'				=> 'Enable search indexing',
	'ENABLE_INDEXING_EXPLAIN'		=> 'If set to yes posts made to this forum will be indexed for searching.',
	'ENABLE_POST_REVIEW'			=> 'Enable post review',
	'ENABLE_POST_REVIEW_EXPLAIN'	=> 'If set to yes users are able to review their post if new posts were made to the topic while users wrote theirs. This should be disabled for chat forums.',
	'ENABLE_RECENT'					=> 'Display active topics',
	'ENABLE_RECENT_EXPLAIN'			=> 'If set to yes topics made to this forum will be shown in the active topics list.',
	'ENABLE_TOPIC_ICONS'			=> 'Enable topic icons',

	'FORUM_ADMIN'						=> 'Forum administration',
	'FORUM_ADMIN_EXPLAIN'				=> 'In phpBB3 there are no categories, everything is forum based. Each forum can have an unlimited number of sub-forums and you can determine whether each may be posted to or not (i.e. whether it acts like an old category). Here you can add, edit, delete, lock, unlock individual forums as well as set certain additional controls. If your posts and topics have got out of sync you can also resynchronise a forum. <strong>You need to copy or set appropriate permissions for newly created forums to have them displayed.</strong>',
	'FORUM_AUTO_PRUNE'					=> 'Enable auto-pruning',
	'FORUM_AUTO_PRUNE_EXPLAIN'			=> 'Prunes the forum of topics, set the frequency/age parameters below.',
	'FORUM_CREATED'						=> 'Forum created successfully.',
	'FORUM_DATA_NEGATIVE'				=> 'Pruning parameters cannot be negative.',
	'FORUM_DESC_TOO_LONG'				=> 'The forum description is too long, it must be less than 4000 characters.',
	'FORUM_DELETE'						=> 'Delete forum',
	'FORUM_DELETE_EXPLAIN'				=> 'The form below will allow you to delete a forum. If the forum is postable you are able to decide where you want to put all topics (or forums) it contained.',
	'FORUM_DELETED'						=> 'Forum successfully deleted.',
	'FORUM_DESC'						=> 'Description',
	'FORUM_DESC_EXPLAIN'				=> 'Any HTML markup entered here will be displayed as is.',
	'FORUM_EDIT_EXPLAIN'				=> 'The form below will allow you to customise this forum. Please note that moderation and post count controls are set via forum permissions for each user or usergroup.',
	'FORUM_IMAGE'						=> 'Forum image',
	'FORUM_IMAGE_EXPLAIN'				=> 'Location, relative to the phpBB root directory, of an additional image to associate with this forum.',
	'FORUM_LINK_EXPLAIN'				=> 'Full URL (including the protocol, i.e.: <samp>http://</samp>) to the destination location that clicking this forum will take the user, e.g.: <samp>http://www.phpbb.com/</samp>.',
	'FORUM_LINK_TRACK'					=> 'Track link redirects',
	'FORUM_LINK_TRACK_EXPLAIN'			=> 'Records the number of times a forum link was clicked.',
	'FORUM_NAME'						=> 'Forum name',
	'FORUM_NAME_EMPTY'					=> 'You must enter a name for this forum.',
	'FORUM_PARENT'						=> 'Parent forum',
	'FORUM_PASSWORD'					=> 'Forum password',
	'FORUM_PASSWORD_CONFIRM'			=> 'Confirm forum password',
	'FORUM_PASSWORD_CONFIRM_EXPLAIN'	=> 'Only needs to be set if a forum password is entered.',
	'FORUM_PASSWORD_EXPLAIN'			=> 'Defines a password for this forum, use the permission system in preference.',
	'FORUM_PASSWORD_UNSET'				=> 'Remove forum password',
	'FORUM_PASSWORD_UNSET_EXPLAIN'		=> 'Check here if you want to remove the forum password.',
	'FORUM_PASSWORD_OLD'				=> 'The forum password is using an old encryption and should be changed.',
	'FORUM_PASSWORD_MISMATCH'			=> 'The passwords you entered did not match.',
	'FORUM_PRUNE_SETTINGS'				=> 'Forum prune settings',
	'FORUM_RESYNCED'					=> 'Forum “%s” successfully resynced',
	'FORUM_RULES_EXPLAIN'				=> 'Forum rules are displayed at any page within the given forum.',
	'FORUM_RULES_LINK'					=> 'Link to forum rules',
	'FORUM_RULES_LINK_EXPLAIN'			=> 'You are able to enter the URL of the page/post containing your forum rules here. This setting will override the forum rules text you specified.',
	'FORUM_RULES_PREVIEW'				=> 'Forum rules preview',
	'FORUM_RULES_TOO_LONG'				=> 'The forum rules must be less than 4000 characters.',
	'FORUM_SETTINGS'					=> 'Forum settings',
	'FORUM_STATUS'						=> 'Forum status',
	'FORUM_STYLE'						=> 'Forum style',
	'FORUM_TOPICS_PAGE'					=> 'Topics per page',
	'FORUM_TOPICS_PAGE_EXPLAIN'			=> 'If non-zero this value will override the default topics per page setting.',
	'FORUM_TYPE'						=> 'Forum type',
	'FORUM_UPDATED'						=> 'Forum information updated successfully.',

	'FORUM_WITH_SUBFORUMS_NOT_TO_LINK'		=> 'You want to change a postable forum having subforums to a link. Please move all subforums out of this forum before you proceed, because after changing to a link you are no longer able to see the subforums currently connected to this forum.',

	'GENERAL_FORUM_SETTINGS'	=> 'General forum settings',

	'LINK'						=> 'Link',
	'LIST_INDEX'				=> 'List subforum in parent-forum’s legend',
	'LIST_INDEX_EXPLAIN'		=> 'Displays this forum on the index and elsewhere as a link within the legend of its parent-forum if the parent-forum’s “List subforums in legend” option is enabled.',
	'LIST_SUBFORUMS'			=> 'List subforums in legend',
	'LIST_SUBFORUMS_EXPLAIN'	=> 'Displays this forum’s subforums on the index and elsewhere as a link within the legend if their “List subforum in parent-forum’s legend” option is enabled.',
	'LOCKED'					=> 'Locked',

	'MOVE_POSTS_NO_POSTABLE_FORUM'	=> 'The forum you selected for moving the posts to is not postable. Please select a postable forum.',
	'MOVE_POSTS_TO'					=> 'Move posts to',
	'MOVE_SUBFORUMS_TO'				=> 'Move subforums to',

	'NO_DESTINATION_FORUM'			=> 'You have not specified a forum to move content to.',
	'NO_FORUM_ACTION'				=> 'No action defined for what happens with the forum content.',
	'NO_PARENT'						=> 'No parent',
	'NO_PERMISSIONS'				=> 'Do not copy permissions',
	'NO_PERMISSION_FORUM_ADD'		=> 'You do not have the necessary permissions to add forums.',
	'NO_PERMISSION_FORUM_DELETE'	=> 'You do not have the necessary permissions to delete forums.',

	'PARENT_IS_LINK_FORUM'		=> 'The parent you specified is a forum link. Link forums are not able to hold other forums, please specify a category or forum as the parent forum.',
	'PARENT_NOT_EXIST'			=> 'Parent does not exist.',
	'PRUNE_ANNOUNCEMENTS'		=> 'Prune announcements',
	'PRUNE_STICKY'				=> 'Prune stickies',
	'PRUNE_OLD_POLLS'			=> 'Prune old polls',
	'PRUNE_OLD_POLLS_EXPLAIN'	=> 'Removes topics with polls not voted in for post age days.',

	'REDIRECT_ACL'	=> 'Now you are able to %sset permissions%s for this forum.',

	'SYNC_IN_PROGRESS'			=> 'Synchronizing forum',
	'SYNC_IN_PROGRESS_EXPLAIN'	=> 'Currently resyncing topic range %1$d/%2$d.',

	'TYPE_CAT'			=> 'Category',
	'TYPE_FORUM'		=> 'Forum',
	'TYPE_LINK'			=> 'Link',

	'UNLOCKED'			=> 'Unlocked',
));

?>