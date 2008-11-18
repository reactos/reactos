<?php
/**
*
* acp_prune [English]
*
* @package language
* @version $Id: prune.php 8479 2008-03-29 00:22:48Z naderman $
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

// User pruning
$lang = array_merge($lang, array(
	'ACP_PRUNE_USERS_EXPLAIN'	=> 'Here you can delete (or deactivate) users from your board. This can be done in a variety of ways; by post count, last activity, etc. Each of these criteria can be combined, i.e. you can prune users last active before 2002-01-01 with fewer than 10 posts. Alternatively you can enter a list of users directly into the text box, any criteria entered will be ignored. Take care with this facility! Once a user is deleted there is no way back.',

	'DEACTIVATE_DELETE'			=> 'Deactivate or delete',
	'DEACTIVATE_DELETE_EXPLAIN'	=> 'Choose whether to deactivate users or delete them entirely, note there is no undo!',
	'DELETE_USERS'				=> 'Delete',
	'DELETE_USER_POSTS'			=> 'Delete pruned user posts',
	'DELETE_USER_POSTS_EXPLAIN' => 'Removes posts made by deleted users, has no effect if users are deactivated.',

	'JOINED_EXPLAIN'			=> 'Enter a date in <kbd>YYYY-MM-DD</kbd> format.',

	'LAST_ACTIVE_EXPLAIN'		=> 'Enter a date in <kbd>YYYY-MM-DD</kbd> format.',

	'PRUNE_USERS_LIST'				=> 'Users to be pruned',
	'PRUNE_USERS_LIST_DELETE'		=> 'With the selected critera for pruning users the following accounts will be removed.',
	'PRUNE_USERS_LIST_DEACTIVATE'	=> 'With the selected critera for pruning users the following accounts will be deactivated.',

	'SELECT_USERS_EXPLAIN'		=> 'Enter specific usernames here, they will be used in preference to the criteria above.',

	'USER_DEACTIVATE_SUCCESS'	=> 'The selected users have been deactivated successfully.',
	'USER_DELETE_SUCCESS'		=> 'The selected users have been deleted successfully.',
	'USER_PRUNE_FAILURE'		=> 'No users fit the selected criteria.',

	'WRONG_ACTIVE_JOINED_DATE'	=> 'The date entered is wrong, it is expected in <kbd>YYYY-MM-DD</kbd> format.',
));

// Forum Pruning
$lang = array_merge($lang, array(
	'ACP_PRUNE_FORUMS_EXPLAIN'	=> 'This will delete any topic which has not been posted to or viewed within the number of days you select. If you do not enter a number then all topics will be deleted. By default, it will not remove topics in which polls are still running nor will it remove stickies and announcements.',

	'FORUM_PRUNE'		=> 'Forum prune',

	'NO_PRUNE'			=> 'No forums pruned.',

	'SELECTED_FORUM'	=> 'Selected forum',
	'SELECTED_FORUMS'	=> 'Selected forums',

	'POSTS_PRUNED'					=> 'Posts pruned',
	'PRUNE_ANNOUNCEMENTS'			=> 'Prune announcements',
	'PRUNE_FINISHED_POLLS'			=> 'Prune closed polls',
	'PRUNE_FINISHED_POLLS_EXPLAIN'	=> 'Removes topics with polls which have ended.',
	'PRUNE_FORUM_CONFIRM'			=> 'Are you sure you want to prune the selected forums with the settings specified? Once removed, there is no way to recover the pruned posts and topics.',
	'PRUNE_NOT_POSTED'				=> 'Days since last posted',
	'PRUNE_NOT_VIEWED'				=> 'Days since last viewed',
	'PRUNE_OLD_POLLS'				=> 'Prune old polls',
	'PRUNE_OLD_POLLS_EXPLAIN'		=> 'Removes topics with polls not voted in for post age days.',
	'PRUNE_STICKY'					=> 'Prune stickies',
	'PRUNE_SUCCESS'					=> 'Pruning of forums was successful.',

	'TOPICS_PRUNED'		=> 'Topics pruned',
));

?>