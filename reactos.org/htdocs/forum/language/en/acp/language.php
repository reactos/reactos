<?php
/**
*
* acp_language [English]
*
* @package language
* @version $Id: language.php 8479 2008-03-29 00:22:48Z naderman $
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
	'ACP_FILES'						=> 'Admin language files',
	'ACP_LANGUAGE_PACKS_EXPLAIN'	=> 'Here you are able to install/remove language packs.',

	'EMAIL_FILES'			=> 'E-mail templates',

	'FILE_CONTENTS'				=> 'File contents',
	'FILE_FROM_STORAGE'			=> 'File from storage folder',

	'HELP_FILES'				=> 'Help files',

	'INSTALLED_LANGUAGE_PACKS'	=> 'Installed language packs',
	'INVALID_LANGUAGE_PACK'		=> 'The selected language pack seems to be not valid. Please verify the language pack and upload it again if necessary.',
	'INVALID_UPLOAD_METHOD'		=> 'The selected upload method is not valid, please choose a different method.',

	'LANGUAGE_DETAILS_UPDATED'			=> 'Language details successfully updated.',
	'LANGUAGE_ENTRIES'					=> 'Language entries',
	'LANGUAGE_ENTRIES_EXPLAIN'			=> 'Here you are able to change existing language pack entries or not already translated ones.<br /><strong>Note:</strong> Once you changed a language file, the changes will be stored within a separate folder for you to download. The changes will not be seen by your users until you replace the original language files at your webspace (by uploading them).',
	'LANGUAGE_FILES'					=> 'Language files',
	'LANGUAGE_KEY'						=> 'Language key',
	'LANGUAGE_PACK_ALREADY_INSTALLED'	=> 'This language pack is already installed.',
	'LANGUAGE_PACK_DELETED'				=> 'The language pack <strong>%s</strong> has been removed successfully. All users using this language have been reset to the boards default language.',
	'LANGUAGE_PACK_DETAILS'				=> 'Language pack details',
	'LANGUAGE_PACK_INSTALLED'			=> 'The language pack <strong>%s</strong> has been successfully installed.',
	'LANGUAGE_PACK_ISO'					=> 'ISO',
	'LANGUAGE_PACK_LOCALNAME'			=> 'Local name',
	'LANGUAGE_PACK_NAME'				=> 'Name',
	'LANGUAGE_PACK_NOT_EXIST'			=> 'The selected language pack does not exist.',
	'LANGUAGE_PACK_USED_BY'				=> 'Used by (including robots)',
	'LANGUAGE_VARIABLE'					=> 'Language variable',
	'LANG_AUTHOR'						=> 'Language pack author',
	'LANG_ENGLISH_NAME'					=> 'English name',
	'LANG_ISO_CODE'						=> 'ISO code',
	'LANG_LOCAL_NAME'					=> 'Local name',

	'MISSING_LANGUAGE_FILE'		=> 'Missing language file: <strong style="color:red">%s</strong>',
	'MISSING_LANG_VARIABLES'	=> 'Missing language variables',
	'MODS_FILES'				=> 'MODs language files',

	'NO_FILE_SELECTED'				=> 'You haven’t specified a language file.',
	'NO_LANG_ID'					=> 'You haven’t specified a language pack.',
	'NO_REMOVE_DEFAULT_LANG'		=> 'You are not able to remove the default language pack.<br />If you want to remove this language pack, change your boards default language first.',
	'NO_UNINSTALLED_LANGUAGE_PACKS'	=> 'No uninstalled language packs',

	'REMOVE_FROM_STORAGE_FOLDER'		=> 'Remove from storage folder',

	'SELECT_DOWNLOAD_FORMAT'	=> 'Select download format',
	'SUBMIT_AND_DOWNLOAD'		=> 'Submit and download file',
	'SUBMIT_AND_UPLOAD'			=> 'Submit and upload file',

	'THOSE_MISSING_LANG_FILES'			=> 'The following language files are missing from the %s language folder',
	'THOSE_MISSING_LANG_VARIABLES'		=> 'The following language variables are missing from the <strong>%s</strong> language pack',

	'UNINSTALLED_LANGUAGE_PACKS'	=> 'Uninstalled language packs',

	'UNABLE_TO_WRITE_FILE'		=> 'The file could not be written to %s.',
	'UPLOAD_COMPLETED'			=> 'The upload was completed successfully.',
	'UPLOAD_FAILED'				=> 'The upload failed for unknown reasons. You may need to replace the relevant file manually.',
	'UPLOAD_METHOD'				=> 'Upload method',
	'UPLOAD_SETTINGS'			=> 'Upload settings',

	'WRONG_LANGUAGE_FILE'		=> 'Selected language file is invalid.',
));

?>