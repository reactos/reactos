<?php
/**
*
* mcp [English]
*
* @package language
* @version $Id: mcp.php 8479 2008-03-29 00:22:48Z naderman $
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
	'ACTION'				=> 'Action',
	'ACTION_NOTE'			=> 'Action/Note',
	'ADD_FEEDBACK'			=> 'Add feedback',
	'ADD_FEEDBACK_EXPLAIN'	=> 'If you would like to add a report on this please fill out the following form. Only use plain text; HTML, BBCode, etc. are not permitted.',
	'ADD_WARNING'			=> 'Add warning',
	'ADD_WARNING_EXPLAIN'	=> 'To send a warning to this user please fill out the following form. Only use plain text; HTML, BBCode, etc. are not permitted.',
	'ALL_ENTRIES'			=> 'All entries',
	'ALL_NOTES_DELETED'		=> 'Successfully removed all user notes.',
	'ALL_REPORTS'			=> 'All reports',
	'ALREADY_REPORTED'		=> 'This post has already been reported.',
	'ALREADY_WARNED'		=> 'A warning has already been issued for this post.',
	'APPROVE'				=> 'Approve',
	'APPROVE_POST'			=> 'Approve post',
	'APPROVE_POST_CONFIRM'	=> 'Are you sure you want to approve this post?',
	'APPROVE_POSTS'			=> 'Approve posts',
	'APPROVE_POSTS_CONFIRM'	=> 'Are you sure you want to approve the selected posts?',

	'CANNOT_MOVE_SAME_FORUM'=> 'You cannot move a topic to the forum it’s already in.',
	'CANNOT_WARN_ANONYMOUS'	=> 'You cannot warn unregistered guest users.',
	'CANNOT_WARN_SELF'		=> 'You cannot warn yourself.',
	'CAN_LEAVE_BLANK'		=> 'This can be left blank.',
	'CHANGE_POSTER'			=> 'Change poster',
	'CLOSE_REPORT'			=> 'Close report',
	'CLOSE_REPORT_CONFIRM'	=> 'Are you sure you want to close the selected report?',
	'CLOSE_REPORTS'			=> 'Close reports',
	'CLOSE_REPORTS_CONFIRM'	=> 'Are you sure you want to close the selected reports?',

	'DELETE_POSTS'				=> 'Delete posts',
	'DELETE_POSTS_CONFIRM'		=> 'Are you sure you want to delete these posts?',
	'DELETE_POST_CONFIRM'		=> 'Are you sure you want to delete this post?',
	'DELETE_REPORT'				=> 'Delete report',
	'DELETE_REPORT_CONFIRM'		=> 'Are you sure you want to delete the selected report?',
	'DELETE_REPORTS'			=> 'Delete reports',
	'DELETE_REPORTS_CONFIRM'	=> 'Are you sure you want to delete the selected reports?',
	'DELETE_SHADOW_TOPIC'		=> 'Delete shadow topic',
	'DELETE_TOPICS'				=> 'Delete selected topics',
	'DELETE_TOPICS_CONFIRM'		=> 'Are you sure you want to delete these topics?',
	'DELETE_TOPIC_CONFIRM'		=> 'Are you sure you want to delete this topic?',
	'DISAPPROVE'				=> 'Disapprove',
	'DISAPPROVE_REASON'			=> 'Reason for disapproval',
	'DISAPPROVE_POST'			=> 'Disapprove post',
	'DISAPPROVE_POST_CONFIRM'	=> 'Are you sure you want to disapprove this post?',
	'DISAPPROVE_POSTS'			=> 'Disapprove posts',
	'DISAPPROVE_POSTS_CONFIRM'	=> 'Are you sure you want to disapprove the selected posts?',
	'DISPLAY_LOG'				=> 'Display entries from previous',
	'DISPLAY_OPTIONS'			=> 'Display options',

	'EMPTY_REPORT'					=> 'You must enter a description when selecting this reason.',
	'EMPTY_TOPICS_REMOVED_WARNING'	=> 'Please note that one or several topics have been removed from the database because they were or become empty.',

	'FEEDBACK'				=> 'Feedback',
	'FORK'					=> 'Copy',
	'FORK_TOPIC'			=> 'Copy topic',
	'FORK_TOPIC_CONFIRM'	=> 'Are you sure you want to copy this topic?',
	'FORK_TOPICS'			=> 'Copy selected topics',
	'FORK_TOPICS_CONFIRM'	=> 'Are you sure you want to copy the selected topics?',
	'FORUM_DESC'			=> 'Description',
	'FORUM_NAME'			=> 'Forum name',
	'FORUM_NOT_EXIST'		=> 'The forum you selected does not exist.',
	'FORUM_NOT_POSTABLE'	=> 'The forum you selected cannot be posted to.',
	'FORUM_STATUS'			=> 'Forum status',
	'FORUM_STYLE'			=> 'Forum style',

	'GLOBAL_ANNOUNCEMENT'	=> 'Global announcement',

	'IP_INFO'				=> 'IP address information',
	'IPS_POSTED_FROM'		=> 'IP addresses this user has posted from',

	'LATEST_LOGS'				=> 'Latest 5 logged actions',
	'LATEST_REPORTED'			=> 'Latest 5 reports',
	'LATEST_UNAPPROVED'			=> 'Latest 5 posts awaiting approval',
	'LATEST_WARNING_TIME'		=> 'Latest warning issued',
	'LATEST_WARNINGS'			=> 'Latest 5 warnings',
	'LEAVE_SHADOW'				=> 'Leave shadow topic in place',
	'LIST_REPORT'				=> '1 report',
	'LIST_REPORTS'				=> '%d reports',
	'LOCK'						=> 'Lock',
	'LOCK_POST_POST'			=> 'Lock post',
	'LOCK_POST_POST_CONFIRM'	=> 'Are you sure you want to prevent editing this post?',
	'LOCK_POST_POSTS'			=> 'Lock selected posts',
	'LOCK_POST_POSTS_CONFIRM'	=> 'Are you sure you want to prevent editing the selected posts?',
	'LOCK_TOPIC_CONFIRM'		=> 'Are you sure you want to lock this topic?',
	'LOCK_TOPICS'				=> 'Lock selected topics',
	'LOCK_TOPICS_CONFIRM'		=> 'Are you sure you want to lock all selected topics?',
	'LOGS_CURRENT_TOPIC'		=> 'Currently viewing logs of:',
	'LOGIN_EXPLAIN_MCP'			=> 'To moderate this forum you must login.',
	'LOGVIEW_VIEWTOPIC'			=> 'View topic',
	'LOGVIEW_VIEWLOGS'			=> 'View topic log',
	'LOGVIEW_VIEWFORUM'			=> 'View forum',
	'LOOKUP_ALL'				=> 'Look up all IPs',
	'LOOKUP_IP'					=> 'Look up IP',

	'MARKED_NOTES_DELETED'		=> 'Successfully removed all marked user notes.',

	'MCP_ADD'						=> 'Add a warning',

	'MCP_BAN'					=> 'Banning',
	'MCP_BAN_EMAILS'			=> 'Ban e-mails',
	'MCP_BAN_IPS'				=> 'Ban IPs',
	'MCP_BAN_USERNAMES'			=> 'Ban Usernames',

	'MCP_LOGS'						=> 'Moderator logs',
	'MCP_LOGS_FRONT'				=> 'Front page',
	'MCP_LOGS_FORUM_VIEW'			=> 'Forum logs',
	'MCP_LOGS_TOPIC_VIEW'			=> 'Topic logs',

	'MCP_MAIN'						=> 'Main',
	'MCP_MAIN_FORUM_VIEW'			=> 'View forum',
	'MCP_MAIN_FRONT'				=> 'Front page',
	'MCP_MAIN_POST_DETAILS'			=> 'Post details',
	'MCP_MAIN_TOPIC_VIEW'			=> 'View topic',
	'MCP_MAKE_ANNOUNCEMENT'			=> 'Modify to “Announcement”',
	'MCP_MAKE_ANNOUNCEMENT_CONFIRM'	=> 'Are you sure you want to change this topic to an “Announcement”?',
	'MCP_MAKE_ANNOUNCEMENTS'		=> 'Modify to “Announcements”',
	'MCP_MAKE_ANNOUNCEMENTS_CONFIRM'=> 'Are you sure you want to change the selected topics to “Announcements”?',
	'MCP_MAKE_GLOBAL'				=> 'Modify to “Global announcement”',
	'MCP_MAKE_GLOBAL_CONFIRM'		=> 'Are you sure you want to change this topic to a “Global announcement”?',
	'MCP_MAKE_GLOBALS'				=> 'Modify to “Global announcements”',
	'MCP_MAKE_GLOBALS_CONFIRM'		=> 'Are you sure you want to change the selected topics to “Global announcements”?',
	'MCP_MAKE_STICKY'				=> 'Modify to “Sticky”',
	'MCP_MAKE_STICKY_CONFIRM'		=> 'Are you sure you want to change this topic to a “Sticky”?',
	'MCP_MAKE_STICKIES'				=> 'Modify to “Stickies”',
	'MCP_MAKE_STICKIES_CONFIRM'		=> 'Are you sure you want to change the selected topics to “Stickies”?',
	'MCP_MAKE_NORMAL'				=> 'Modify to “Standard Topic”',
	'MCP_MAKE_NORMAL_CONFIRM'		=> 'Are you sure you want to change this topic to a “Standard Topic”?',
	'MCP_MAKE_NORMALS'				=> 'Modify to “Standard Topics”',
	'MCP_MAKE_NORMALS_CONFIRM'		=> 'Are you sure you want to change the selected topics to “Standard Topics”?',

	'MCP_NOTES'						=> 'User notes',
	'MCP_NOTES_FRONT'				=> 'Front page',
	'MCP_NOTES_USER'				=> 'User details',

	'MCP_POST_REPORTS'				=> 'Reports issued on this post',

	'MCP_REPORTS'					=> 'Reported posts',
	'MCP_REPORT_DETAILS'			=> 'Report details',
	'MCP_REPORTS_CLOSED'			=> 'Closed reports',
	'MCP_REPORTS_CLOSED_EXPLAIN'	=> 'This is a list of all reports about posts which have previously been resolved.',
	'MCP_REPORTS_OPEN'				=> 'Open reports',
	'MCP_REPORTS_OPEN_EXPLAIN'		=> 'This is a list of all reported posts which are still to be handled.',

	'MCP_QUEUE'								=> 'Moderation queue',
	'MCP_QUEUE_APPROVE_DETAILS'				=> 'Approve details',
	'MCP_QUEUE_UNAPPROVED_POSTS'			=> 'Posts awaiting approval',
	'MCP_QUEUE_UNAPPROVED_POSTS_EXPLAIN'	=> 'This is a list of all posts which require approving before they will be visible to users.',
	'MCP_QUEUE_UNAPPROVED_TOPICS'			=> 'Topics awaiting approval',
	'MCP_QUEUE_UNAPPROVED_TOPICS_EXPLAIN'	=> 'This is a list of all topics which require approving before they will be visible to users.',

	'MCP_VIEW_USER'			=> 'View warnings for a specific user',

	'MCP_WARN'				=> 'Warnings',
	'MCP_WARN_FRONT'		=> 'Front page',
	'MCP_WARN_LIST'			=> 'List warnings',
	'MCP_WARN_POST'			=> 'Warn for specific post',
	'MCP_WARN_USER'			=> 'Warn user',

	'MERGE_POSTS'			=> 'Merge posts',
	'MERGE_POSTS_CONFIRM'	=> 'Are you sure you want to merge the selected posts?',
	'MERGE_TOPIC_EXPLAIN'	=> 'Using the form below you can merge selected posts into another topic. These posts will not be reordered and will appear as if the users posted them to the new topic.<br />Please enter the destination topic id or click on “Select topic” to search for one.',
	'MERGE_TOPIC_ID'		=> 'Destination topic identification number',
	'MERGE_TOPICS'			=> 'Merge topics',
	'MERGE_TOPICS_CONFIRM'	=> 'Are you sure you want to merge the selected topics?',
	'MODERATE_FORUM'		=> 'Moderate forum',
	'MODERATE_TOPIC'		=> 'Moderate topic',
	'MODERATE_POST'			=> 'Moderate post',
	'MOD_OPTIONS'			=> 'Moderator options',
	'MORE_INFO'				=> 'Further information',
	'MOST_WARNINGS'			=> 'Users with most warnings',
	'MOVE_TOPIC_CONFIRM'	=> 'Are you sure you want to move the topic into a new forum?',
	'MOVE_TOPICS'			=> 'Move selected topics',
	'MOVE_TOPICS_CONFIRM'	=> 'Are you sure you want to move the selected topics into a new forum?',

	'NOTIFY_POSTER_APPROVAL'		=> 'Notify poster about approval?',
	'NOTIFY_POSTER_DISAPPROVAL'		=> 'Notify poster about disapproval?',
	'NOTIFY_USER_WARN'				=> 'Notify user about warning?',
	'NOT_MODERATOR'					=> 'You are not a moderator of this forum.',
	'NO_DESTINATION_FORUM'			=> 'Please select a forum for destination.',
	'NO_DESTINATION_FORUM_FOUND'	=> 'There is no destination forum available.',
	'NO_ENTRIES'					=> 'No log entries for this period.',
	'NO_FEEDBACK'					=> 'No feedback exists for this user.',
	'NO_FINAL_TOPIC_SELECTED'		=> 'You have to select a destination topic for merging posts.',
	'NO_MATCHES_FOUND'				=> 'No matches found.',
	'NO_POST'						=> 'You have to select a post in order to warn the user for a post.',
	'NO_POST_REPORT'				=> 'This post was not reported.',
	'NO_POST_SELECTED'				=> 'You must select at least one post to perform this action.',
	'NO_REASON_DISAPPROVAL'			=> 'Please give an appropriate reason for disapproval.',
	'NO_REPORT'						=> 'No report found',
	'NO_REPORTS'					=> 'No reports found',
	'NO_REPORT_SELECTED'			=> 'You must select at least one report to perform this action.',
	'NO_TOPIC_ICON'					=> 'None',
	'NO_TOPIC_SELECTED'				=> 'You must select at least one topic to perform this action.',
	'NO_TOPICS_QUEUE'				=> 'There are no topics waiting for approval.',

	'ONLY_TOPIC'			=> 'Only topic “%s”',
	'OTHER_USERS'			=> 'Other users posting from this IP',

	'POSTER'					=> 'Poster',
	'POSTS_APPROVED_SUCCESS'	=> 'The selected posts have been approved.',
	'POSTS_DELETED_SUCCESS'		=> 'The selected posts have been successfully removed from the database.',
	'POSTS_DISAPPROVED_SUCCESS'	=> 'The selected posts have been disapproved.',
	'POSTS_LOCKED_SUCCESS'		=> 'The selected posts have been locked successfully.',
	'POSTS_MERGED_SUCCESS'		=> 'The selected posts have been merged.',
	'POSTS_UNLOCKED_SUCCESS'	=> 'The selected posts have been unlocked successfully.',
	'POSTS_PER_PAGE'			=> 'Posts per page',
	'POSTS_PER_PAGE_EXPLAIN'	=> '(Set to 0 to view all posts.)',
	'POST_APPROVED_SUCCESS'		=> 'The selected post has been approved.',
	'POST_DELETED_SUCCESS'		=> 'The selected post has been successfully removed from the database.',
	'POST_DISAPPROVED_SUCCESS'	=> 'The selected post has been disapproved.',
	'POST_LOCKED_SUCCESS'		=> 'Post locked successfully.',
	'POST_NOT_EXIST'			=> 'The post you requested does not exist.',
	'POST_REPORTED_SUCCESS'		=> 'This post has been successfully reported.',
	'POST_UNLOCKED_SUCCESS'		=> 'Post unlocked successfully.',

	'READ_USERNOTES'			=> 'User notes',
	'READ_WARNINGS'				=> 'User warnings',
	'REPORTER'					=> 'Reporter',
	'REPORTED'					=> 'Reported',
	'REPORTED_BY'				=> 'Reported by',
	'REPORTED_ON_DATE'			=> 'on',
	'REPORTS_CLOSED_SUCCESS'	=> 'The selected reports have been closed successfully.',
	'REPORTS_DELETED_SUCCESS'	=> 'The selected reports have been deleted successfully.',
	'REPORTS_TOTAL'				=> 'In total there are <strong>%d</strong> reports to review.',
	'REPORTS_ZERO_TOTAL'		=> 'There are no reports to review.',
	'REPORT_CLOSED'				=> 'This report has already been closed.',
	'REPORT_CLOSED_SUCCESS'		=> 'The selected report has been closed successfully.',
	'REPORT_DELETED_SUCCESS'	=> 'The selected report has been deleted successfully.',
	'REPORT_DETAILS'			=> 'Report details',
	'REPORT_MESSAGE'			=> 'Report this message',
	'REPORT_MESSAGE_EXPLAIN'	=> 'Use this form to report the selected message. Reporting should generally be used only if the message breaks forum rules.',
	'REPORT_NOTIFY'				=> 'Notify me',
	'REPORT_NOTIFY_EXPLAIN'		=> 'Informs you when your report is dealt with.',
	'REPORT_POST_EXPLAIN'		=> 'Use this form to report the selected post to the forum moderators and board administrators. Reporting should generally be used only if the post breaks forum rules.',
	'REPORT_REASON'				=> 'Report reason',
	'REPORT_TIME'				=> 'Report time',
	'REPORT_TOTAL'				=> 'In total there is <strong>1</strong> report to review.',
	'RESYNC'					=> 'Resync',
	'RETURN_MESSAGE'			=> '%sReturn to the message%s',
	'RETURN_NEW_FORUM'			=> '%sGo to the new forum%s',
	'RETURN_NEW_TOPIC'			=> '%sGo to the new topic%s',
	'RETURN_POST'				=> '%sReturn to the post%s',
	'RETURN_QUEUE'				=> '%sReturn to the queue%s',
	'RETURN_REPORTS'			=> '%sReturn to the reports%s',
	'RETURN_TOPIC_SIMPLE'		=> '%sReturn to the topic%s',

	'SEARCH_POSTS_BY_USER'				=> 'Search posts by',
	'SELECT_ACTION'						=> 'Select desired action',
	'SELECT_FORUM_GLOBAL_ANNOUNCEMENT'	=> 'Please select the forum you wish this global announcement to be displayed.',
	'SELECT_FORUM_GLOBAL_ANNOUNCEMENTS'	=> 'One or more of the selected topics are global announcements. Please select the forum you wish these to be displayed.',
	'SELECT_MERGE'						=> 'Select for merge',
	'SELECT_TOPICS_FROM'				=> 'Select topics from',
	'SELECT_TOPIC'						=> 'Select topic',
	'SELECT_USER'						=> 'Select user',
	'SORT_ACTION'						=> 'Log action',
	'SORT_DATE'							=> 'Date',
	'SORT_IP'							=> 'IP address',
	'SORT_WARNINGS'						=> 'Warnings',
	'SPLIT_AFTER'						=> 'Split from selected post',
	'SPLIT_FORUM'						=> 'Forum for new topic',
	'SPLIT_POSTS'						=> 'Split selected posts',
	'SPLIT_SUBJECT'						=> 'New topic title',
	'SPLIT_TOPIC_ALL'					=> 'Split topic from selected posts',
	'SPLIT_TOPIC_ALL_CONFIRM'			=> 'Are you sure you want to split this topic?',
	'SPLIT_TOPIC_BEYOND'				=> 'Split topic at selected post',
	'SPLIT_TOPIC_BEYOND_CONFIRM'		=> 'Are you sure you want to split this topic at the selected post?',
	'SPLIT_TOPIC_EXPLAIN'				=> 'Using the form below you can split a topic in two, either by selecting the posts individually or by splitting at a selected post.',

	'THIS_POST_IP'				=> 'IP for this post',
	'TOPICS_APPROVED_SUCCESS'	=> 'The selected topics have been approved.',
	'TOPICS_DELETED_SUCCESS'	=> 'The selected topics have been successfully removed from the database.',
	'TOPICS_DISAPPROVED_SUCCESS'=> 'The selected topics have been disapproved.',
	'TOPICS_FORKED_SUCCESS'		=> 'The selected topics have been copied successfully.',
	'TOPICS_LOCKED_SUCCESS'		=> 'The selected topics have been locked.',
	'TOPICS_MOVED_SUCCESS'		=> 'The selected topics have been moved successfully.',
	'TOPICS_RESYNC_SUCCESS'		=> 'The selected topics have been resynchronised.',
	'TOPICS_TYPE_CHANGED'		=> 'Topic types changed successfully.',
	'TOPICS_UNLOCKED_SUCCESS'	=> 'The selected topics have been unlocked.',
	'TOPIC_APPROVED_SUCCESS'	=> 'The selected topic has been approved.',
	'TOPIC_DELETED_SUCCESS'		=> 'The selected topic has been successfully removed from the database.',
	'TOPIC_DISAPPROVED_SUCCESS'	=> 'The selected topic has been disapproved.',
	'TOPIC_FORKED_SUCCESS'		=> 'The selected topic has been copied successfully.',
	'TOPIC_LOCKED_SUCCESS'		=> 'The selected topic has been locked.',
	'TOPIC_MOVED_SUCCESS'		=> 'The selected topic has been moved successfully.',
	'TOPIC_NOT_EXIST'			=> 'The topic you selected does not exist.',
	'TOPIC_RESYNC_SUCCESS'		=> 'The selected topic has been resynchronised.',
	'TOPIC_SPLIT_SUCCESS'		=> 'The selected topic has been split successfully.',
	'TOPIC_TIME'				=> 'Topic time',
	'TOPIC_TYPE_CHANGED'		=> 'Topic type changed successfully.',
	'TOPIC_UNLOCKED_SUCCESS'	=> 'The selected topic has been unlocked.',
	'TOTAL_WARNINGS'			=> 'Total Warnings',

	'UNAPPROVED_POSTS_TOTAL'		=> 'In total there are <strong>%d</strong> posts waiting for approval.',
	'UNAPPROVED_POSTS_ZERO_TOTAL'	=> 'There are no posts waiting for approval.',
	'UNAPPROVED_POST_TOTAL'			=> 'In total there is <strong>1</strong> post waiting for approval.',
	'UNLOCK'						=> 'Unlock',
	'UNLOCK_POST'					=> 'Unlock post',
	'UNLOCK_POST_EXPLAIN'			=> 'Allow editing',
	'UNLOCK_POST_POST'				=> 'Unlock post',
	'UNLOCK_POST_POST_CONFIRM'		=> 'Are you sure you want to allow editing this post?',
	'UNLOCK_POST_POSTS'				=> 'Unlock selected posts',
	'UNLOCK_POST_POSTS_CONFIRM'		=> 'Are you sure you want to allow editing the selected posts?',
	'UNLOCK_TOPIC'					=> 'Unlock topic',
	'UNLOCK_TOPIC_CONFIRM'			=> 'Are you sure you want to unlock this topic?',
	'UNLOCK_TOPICS'					=> 'Unlock selected topics',
	'UNLOCK_TOPICS_CONFIRM'			=> 'Are you sure you want to unlock all selected topics?',
	'USER_CANNOT_POST'				=> 'You cannot post in this forum.',
	'USER_CANNOT_REPORT'			=> 'You cannot report posts in this forum.',
	'USER_FEEDBACK_ADDED'			=> 'User feedback added successfully.',
	'USER_WARNING_ADDED'			=> 'User warned successfully.',

	'VIEW_DETAILS'			=> 'View details',

	'WARNED_USERS'			=> 'Warned users',
	'WARNED_USERS_EXPLAIN'	=> 'This is a list of users with unexpired warnings issued to them.',
	'WARNING_PM_BODY'		=> 'The following is a warning which has been issued to you by an administrator or moderator of this site.[quote]%s[/quote]',
	'WARNING_PM_SUBJECT'	=> 'Board warning issued',
	'WARNING_POST_DEFAULT'	=> 'This is a warning regarding the following post made by you: %s .',
	'WARNINGS_ZERO_TOTAL'	=> 'No warnings exist.',

	'YOU_SELECTED_TOPIC'	=> 'You selected topic number %d: %s.',

	'report_reasons'		=> array(
		'TITLE'	=> array(
			'WAREZ'		=> 'Warez',
			'SPAM'		=> 'Spam',
			'OFF_TOPIC'	=> 'Off-topic',
			'OTHER'		=> 'Other',
		),
		'DESCRIPTION' => array(
			'WAREZ'		=> 'The post contains links to illegal or pirated software.',
			'SPAM'		=> 'The reported post has the only purpose to advertise for a website or another product.',
			'OFF_TOPIC'	=> 'The reported post is off topic.',
			'OTHER'		=> 'The reported post does not fit into any other category, please use the further information field.',
		)
	),
));

?>