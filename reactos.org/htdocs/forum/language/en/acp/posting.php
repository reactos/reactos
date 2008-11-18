<?php
/**
*
* acp_posting [English]
*
* @package language
* @version $Id: posting.php 8479 2008-03-29 00:22:48Z naderman $
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

// BBCodes
// Note to translators: you can translate everything but what's between { and }
$lang = array_merge($lang, array(
	'ACP_BBCODES_EXPLAIN'		=> 'BBCode is a special implementation of HTML offering greater control over what and how something is displayed. From this page you can add, remove and edit custom BBCodes.',
	'ADD_BBCODE'				=> 'Add a new BBCode',

	'BBCODE_ADDED'				=> 'BBCode added successfully.',
	'BBCODE_EDITED'				=> 'BBCode edited successfully.',
	'BBCODE_NOT_EXIST'			=> 'The BBCode you selected does not exist.',
	'BBCODE_HELPLINE'			=> 'Help line',
	'BBCODE_HELPLINE_EXPLAIN'	=> 'This field contains the mouse over text of the BBCode.',
	'BBCODE_HELPLINE_TEXT'		=> 'Help line text',
	'BBCODE_INVALID_TAG_NAME'	=> 'The BBCode tag name that you selected already exists.',
	'BBCODE_INVALID'			=> 'Your BBCode is constructed in an invalid form.',
	'BBCODE_OPEN_ENDED_TAG'		=> 'Your custom BBCode must contain both an opening and a closing tag.',
	'BBCODE_TAG'				=> 'Tag',
	'BBCODE_TAG_TOO_LONG'		=> 'The tag name you selected is too long.',
	'BBCODE_TAG_DEF_TOO_LONG'	=> 'The tag definition that you have entered is too long, please shorten your tag definition.',
	'BBCODE_USAGE'				=> 'BBCode usage',
	'BBCODE_USAGE_EXAMPLE'		=> '[highlight={COLOR}]{TEXT}[/highlight]<br /><br />[font={SIMPLETEXT1}]{SIMPLETEXT2}[/font]',
	'BBCODE_USAGE_EXPLAIN'		=> 'Here you define how to use the BBCode. Replace any variable input by the corresponding token (%ssee below%s).',

	'EXAMPLE'						=> 'Example:',
	'EXAMPLES'						=> 'Examples:',

	'HTML_REPLACEMENT'				=> 'HTML replacement',
	'HTML_REPLACEMENT_EXAMPLE'		=> '&lt;span style="background-color: {COLOR};"&gt;{TEXT}&lt;/span&gt;<br /><br />&lt;span style="font-family: {SIMPLETEXT1};"&gt;{SIMPLETEXT2}&lt;/span&gt;',
	'HTML_REPLACEMENT_EXPLAIN'		=> 'Here you define the default HTML replacement. Do not forget to put back tokens you used above!',

	'TOKEN'					=> 'Token',
	'TOKENS'				=> 'Tokens',
	'TOKENS_EXPLAIN'		=> 'Tokens are placeholders for user input. The input will be validated only if it matches the corresponding definition. If needed, you can number them by adding a number as the last character between the braces, e.g. {TEXT1}, {TEXT2}.<br /><br />Within the HTML replacement you can also use any language string present in your language/ directory like this: {L_<em>&lt;STRINGNAME&gt;</em>} where <em>&lt;STRINGNAME&gt;</em> is the name of the translated string you want to add. For example, {L_WROTE} will be displayed as &quot;wrote&quot; or its translation according to user’s locale.<br /><br /><strong>Please note that only tokens listed below are able to be used within custom BBCodes.</strong>',
	'TOKEN_DEFINITION'		=> 'What can it be?',
	'TOO_MANY_BBCODES'		=> 'You cannot create any more BBCodes. Please remove one or more BBCodes then try again.',

	'tokens'	=>	array(
		'TEXT'			=> 'Any text, including foreign characters, numbers, etc… You should not use this token in HTML tags. Instead try to use IDENTIFIER or SIMPLETEXT.',
		'SIMPLETEXT'	=> 'Characters from the latin alphabet (A-Z), numbers, spaces, commas, dots, minus, plus, hyphen and underscore',
		'IDENTIFIER'	=> 'Characters from the latin alphabet (A-Z), numbers, hyphen and underscore',
		'NUMBER'		=> 'Any series of digits',
		'EMAIL'			=> 'A valid e-mail address',
		'URL'			=> 'A valid URL using any protocol (http, ftp, etc… cannot be used for javascript exploits). If none is given, &quot;http://&quot; is prefixed to the string.',
		'LOCAL_URL'		=> 'A local URL. The URL must be relative to the topic page and cannot contain a server name or protocol.',
		'COLOR'			=> 'A HTML colour, can be either in the numeric form <samp>#FF1234</samp> or a <a href="http://www.w3.org/TR/CSS21/syndata.html#value-def-color">CSS colour keyword</a> such as <samp>fuchsia</samp> or <samp>InactiveBorder</samp>'
	)
));

// Smilies and topic icons
$lang = array_merge($lang, array(
	'ACP_ICONS_EXPLAIN'		=> 'From this page you can add, remove and edit the icons users may add to their topics or posts. These icons are generally displayed next to topic titles on the forum listing, or the post subjects in topic listings. You can also install and create new packages of icons.',
	'ACP_SMILIES_EXPLAIN'	=> 'Smilies or emoticons are typically small, sometimes animated images used to convey an emotion or feeling. From this page you can add, remove and edit the emoticons users can use in their posts and private messages. You can also install and create new packages of smilies.',
	'ADD_SMILIES'			=> 'Add multiple smilies',
	'ADD_SMILEY_CODE'		=> 'Add additional smiley code',
	'ADD_ICONS'				=> 'Add multiple icons',
	'AFTER_ICONS'			=> 'After %s',
	'AFTER_SMILIES'			=> 'After %s',

	'CODE'						=> 'Code',
	'CURRENT_ICONS'				=> 'Current icons',
	'CURRENT_ICONS_EXPLAIN'		=> 'Choose what to do with the currently installed icons.',
	'CURRENT_SMILIES'			=> 'Current smilies',
	'CURRENT_SMILIES_EXPLAIN'	=> 'Choose what to do with the currently installed smilies.',

	'DISPLAY_ON_POSTING'		=> 'Display on posting page',
	'DISPLAY_POSTING'			=> 'On posting page',
	'DISPLAY_POSTING_NO'		=> 'Not on posting page',



	'EDIT_ICONS'				=> 'Edit icons',
	'EDIT_SMILIES'				=> 'Edit smilies',
	'EMOTION'					=> 'Emotion',
	'EXPORT_ICONS'				=> 'Export and download icons.pak',
	'EXPORT_ICONS_EXPLAIN'		=> '%sOn clicking this link, the configuration for your installed icons will be packaged into <samp>icons.pak</samp> which once downloaded can be used to create a <samp>.zip</samp> or <samp>.tgz</samp> file containing all of your icons plus this <samp>icons.pak</samp> configuration file%s.',
	'EXPORT_SMILIES'			=> 'Export and download smilies.pak',
	'EXPORT_SMILIES_EXPLAIN'	=> '%sOn clicking this link, the configuration for your installed smilies will be packaged into <samp>smilies.pak</samp> which once downloaded can be used to create a <samp>.zip</samp> or <samp>.tgz</samp> file containing all of your smilies plus this <samp>smilies.pak</samp> configuration file%s.',

	'FIRST'			=> 'First',

	'ICONS_ADD'				=> 'Add a new icon',
	'ICONS_NONE_ADDED'		=> 'No icons were added.',
	'ICONS_ONE_ADDED'		=> 'The icon has been added successfully.',
	'ICONS_ADDED'			=> 'The icons have been added successfully.',
	'ICONS_CONFIG'			=> 'Icon configuration',
	'ICONS_DELETED'			=> 'The icon has been removed successfully.',
	'ICONS_EDIT'			=> 'Edit icon',
	'ICONS_ONE_EDITED'		=> 'The icon has been updated successfully.',
	'ICONS_NONE_EDITED'		=> 'No icons were updated.',
	'ICONS_EDITED'			=> 'The icons have been updated successfully.',
	'ICONS_HEIGHT'			=> 'Icon height',
	'ICONS_IMAGE'			=> 'Icon image',
	'ICONS_IMPORTED'		=> 'The icons pack has been installed successfully.',
	'ICONS_IMPORT_SUCCESS'	=> 'The icons pack was imported successfully.',
	'ICONS_LOCATION'		=> 'Icon location',
	'ICONS_NOT_DISPLAYED'	=> 'The following icons are not displayed on the posting page',
	'ICONS_ORDER'			=> 'Icon order',
	'ICONS_URL'				=> 'Icon image file',
	'ICONS_WIDTH'			=> 'Icon width',
	'IMPORT_ICONS'			=> 'Install icons package',
	'IMPORT_SMILIES'		=> 'Install smilies package',

	'KEEP_ALL'			=> 'Keep all',

	'MASS_ADD_SMILIES'	=> 'Add multiple smilies',

	'NO_ICONS_ADD'		=> 'There are no icons available for adding.',
	'NO_ICONS_EDIT'		=> 'There are no icons available for modifying.',
	'NO_ICONS_EXPORT'	=> 'You have no icons with which to create a package.',
	'NO_ICONS_PAK'		=> 'No icon packages found.',
	'NO_SMILIES_ADD'	=> 'There are no smilies available for adding.',
	'NO_SMILIES_EDIT'	=> 'There are no smilies available for modifying.',
	'NO_SMILIES_EXPORT'	=> 'You have no smilies with which to create a package.',
	'NO_SMILIES_PAK'	=> 'No smiley packages found.',

	'PAK_FILE_NOT_READABLE'		=> 'Could not read <samp>.pak</samp> file.',

	'REPLACE_MATCHES'	=> 'Replace matches',

	'SELECT_PACKAGE'			=> 'Select a package file',
	'SMILIES_ADD'				=> 'Add a new smiley',
	'SMILIES_NONE_ADDED'		=> 'No smilies were added.',
	'SMILIES_ONE_ADDED'			=> 'The smiley has been added successfully.',
	'SMILIES_ADDED'				=> 'The smilies have been added successfully.',
	'SMILIES_CODE'				=> 'Smiley code',
	'SMILIES_CONFIG'			=> 'Smiley configuration',
	'SMILIES_DELETED'			=> 'The smiley has been removed successfully.',
	'SMILIES_EDIT'				=> 'Edit smiley',
	'SMILIE_NO_CODE'			=> 'The smilie “%s”  was ignored, as there was no code entered.',
	'SMILIE_NO_EMOTION'			=> 'The smilie “%s” was ignored, as there was no emotion entered.',
	'SMILIES_NONE_EDITED'		=> 'No smilies were updated.',
	'SMILIES_ONE_EDITED'		=> 'The smiley has been updated successfully.',
	'SMILIES_EDITED'			=> 'The smilies have been updated successfully.',
	'SMILIES_EMOTION'			=> 'Emotion',
	'SMILIES_HEIGHT'			=> 'Smiley height',
	'SMILIES_IMAGE'				=> 'Smiley image',
	'SMILIES_IMPORTED'			=> 'The smilies pack has been installed successfully.',
	'SMILIES_IMPORT_SUCCESS'	=> 'The smilies pack was imported successfully.',
	'SMILIES_LOCATION'			=> 'Smiley location',
	'SMILIES_NOT_DISPLAYED'		=> 'The following smilies are not displayed on the posting page',
	'SMILIES_ORDER'				=> 'Smiley order',
	'SMILIES_URL'				=> 'Smiley image file',
	'SMILIES_WIDTH'				=> 'Smiley width',

	'WRONG_PAK_TYPE'	=> 'The specified package does not contain the appropriate data.',
));

// Word censors
$lang = array_merge($lang, array(
	'ACP_WORDS_EXPLAIN'		=> 'From this control panel you can add, edit, and remove words that will be automatically censored on your forums. In addition people will not be allowed to register with usernames containing these words. Wildcards (*) are accepted in the word field, e.g. *test* will match detestable, test* would match testing, *test would match detest.',
	'ADD_WORD'				=> 'Add new word',

	'EDIT_WORD'		=> 'Edit word censor',
	'ENTER_WORD'	=> 'You must enter a word and its replacement.',

	'NO_WORD'	=> 'No word selected for editing.',

	'REPLACEMENT'	=> 'Replacement',

	'UPDATE_WORD'	=> 'Update word censor',

	'WORD'				=> 'Word',
	'WORD_ADDED'		=> 'The word censor has been successfully added.',
	'WORD_REMOVED'		=> 'The selected word censor has been successfully removed.',
	'WORD_UPDATED'		=> 'The selected word censor has been successfully updated.',
));

// Ranks
$lang = array_merge($lang, array(
	'ACP_RANKS_EXPLAIN'		=> 'Using this form you can add, edit, view and delete ranks. You can also create special ranks which can be applied to a user via the user management facility.',
	'ADD_RANK'				=> 'Add new rank',

	'MUST_SELECT_RANK'		=> 'You must select a rank.',

	'NO_ASSIGNED_RANK'		=> 'No special rank assigned.',
	'NO_RANK_TITLE'			=> 'You haven’t specified a title for the rank.',
	'NO_UPDATE_RANKS'		=> 'The rank was successfully deleted. However user accounts using this rank were not updated. You will need to manually reset the rank on these accounts.',

	'RANK_ADDED'			=> 'The rank was successfully added.',
	'RANK_IMAGE'			=> 'Rank image',
	'RANK_IMAGE_EXPLAIN'	=> 'Use this to define a small image associated with the rank. The path is relative to the root phpBB directory.',
	'RANK_MINIMUM'			=> 'Minimum posts',
	'RANK_REMOVED'			=> 'The rank was successfully deleted.',
	'RANK_SPECIAL'			=> 'Set as special rank',
	'RANK_TITLE'			=> 'Rank title',
	'RANK_UPDATED'			=> 'The rank was successfully updated.',
));

// Disallow Usernames
$lang = array_merge($lang, array(
	'ACP_DISALLOW_EXPLAIN'	=> 'Here you can control usernames which will not be allowed to be used. Disallowed usernames are allowed to contain a wildcard character of *. Please note that you will not be allowed to specify any username that has already been registered, you must first delete that name then disallow it.',
	'ADD_DISALLOW_EXPLAIN'	=> 'You can disallow a username using the wildcard character * to match any character.',
	'ADD_DISALLOW_TITLE'	=> 'Add a disallowed username',

	'DELETE_DISALLOW_EXPLAIN'	=> 'You can remove a disallowed username by selecting the username from this list and clicking submit.',
	'DELETE_DISALLOW_TITLE'		=> 'Remove a disallowed username',
	'DISALLOWED_ALREADY'		=> 'The name you entered could not be disallowed. It either already exists in the list, exists in the word censor list, or a matching username is present.',
	'DISALLOWED_DELETED'		=> 'The disallowed username has been successfully removed.',
	'DISALLOW_SUCCESSFUL'		=> 'The disallowed username has been successfully added.',

	'NO_DISALLOWED'				=> 'No disallowed usernames',
	'NO_USERNAME_SPECIFIED'		=> 'You haven’t selected or entered a username to operate with.',
));

// Reasons
$lang = array_merge($lang, array(
	'ACP_REASONS_EXPLAIN'	=> 'Here you can manage the reasons used in reports and denial messages when disapproving posts. There is one default reason (marked with a *) you are not able to remove, this reason is normally used for custom messages if no reason fits.',
	'ADD_NEW_REASON'		=> 'Add new reason',
	'AVAILABLE_TITLES'		=> 'Available localised reason titles',

	'IS_NOT_TRANSLATED'			=> 'Reason has <strong>not</strong> been localised.',
	'IS_NOT_TRANSLATED_EXPLAIN'	=> 'Reason has <strong>not</strong> been localised. If you want to provide the localised form, specify the correct key from the language files report reasons section.',
	'IS_TRANSLATED'				=> 'Reason has been localised.',
	'IS_TRANSLATED_EXPLAIN'		=> 'Reason has been localised. If the title you enter here is specified within the language files report reasons section, the localised form of the title and description will be used.',

	'NO_REASON'					=> 'Reason could not be found.',
	'NO_REASON_INFO'			=> 'You have to specify a title and a description for this reason.',
	'NO_REMOVE_DEFAULT_REASON'	=> 'You are not able to remove the default reason “Other”.',

	'REASON_ADD'				=> 'Add report/denial reason',
	'REASON_ADDED'				=> 'Report/denial reason successfully added.',
	'REASON_ALREADY_EXIST'		=> 'A reason with this title already exist, please enter another title for this reason.',
	'REASON_DESCRIPTION'		=> 'Reason description',
	'REASON_DESC_TRANSLATED'	=> 'Displayed reason description',
	'REASON_EDIT'				=> 'Edit report/denial reason',
	'REASON_EDIT_EXPLAIN'		=> 'Here you are able to add or edit a reason. If the reason is translated the localised version is used instead of the description entered here.',
	'REASON_REMOVED'			=> 'Report/denial reason successfully removed.',
	'REASON_TITLE'				=> 'Reason title',
	'REASON_TITLE_TRANSLATED'	=> 'Displayed reason title',
	'REASON_UPDATED'			=> 'Report/denial reason successfully updated.',

	'USED_IN_REPORTS'		=> 'Used in reports',
));

?>