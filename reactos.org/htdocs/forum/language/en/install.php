<?php
/**
*
* install [English]
*
* @package language
* @version $Id: install.php 8479 2008-03-29 00:22:48Z naderman $
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
	'ADMIN_CONFIG'				=> 'Administrator configuration',
	'ADMIN_PASSWORD'			=> 'Administrator password',
	'ADMIN_PASSWORD_CONFIRM'	=> 'Confirm administrator password',
	'ADMIN_PASSWORD_EXPLAIN'	=> 'Please enter a password between 6 and 30 characters in length.',
	'ADMIN_TEST'				=> 'Check administrator settings',
	'ADMIN_USERNAME'			=> 'Administrator username',
	'ADMIN_USERNAME_EXPLAIN'	=> 'Please enter a username between 3 and 20 characters in length.',
	'APP_MAGICK'				=> 'Imagemagick support [ Attachments ]',
	'AUTHOR_NOTES'				=> 'Author notes<br />» %s',
	'AVAILABLE'					=> 'Available',
	'AVAILABLE_CONVERTORS'		=> 'Available convertors',

	'BEGIN_CONVERT'					=> 'Begin conversion',
	'BLANK_PREFIX_FOUND'			=> 'A scan of your tables has shown a valid installation using no table prefix.',
	'BOARD_NOT_INSTALLED'			=> 'No installation found',
	'BOARD_NOT_INSTALLED_EXPLAIN'	=> 'The phpBB Unified Convertor Framework requires a default installation of phpBB3 to function, please <a href="%s">proceed by first installing phpBB3</a>.',

	'CATEGORY'					=> 'Category',
	'CACHE_STORE'				=> 'Cache type',
	'CACHE_STORE_EXPLAIN'		=> 'The physical location where data is cached, filesystem is preferred.',
	'CAT_CONVERT'				=> 'Convert',
	'CAT_INSTALL'				=> 'Install',
	'CAT_OVERVIEW'				=> 'Overview',
	'CAT_UPDATE'				=> 'Update',
	'CHANGE'					=> 'Change',
	'CHECK_TABLE_PREFIX'		=> 'Please check your table prefix and try again.',
	'CLEAN_VERIFY'				=> 'Cleaning up and verifying the final structure',
	'CLEANING_USERNAMES'		=> 'Cleaning usernames',
	'COLLIDING_CLEAN_USERNAME'	=> '<strong>%s</strong> is the clean username for:',
	'COLLIDING_USERNAMES_FOUND'	=> 'Colliding usernames were found on your old board. In order to complete the conversion please delete or rename these users so that there is only one user on your old board for each clean username.',
	'COLLIDING_USER'			=> '» user id: <strong>%d</strong> username: <strong>%s</strong> (%d posts)',
	'CONFIG_CONVERT'			=> 'Converting the configuration',
	'CONFIG_FILE_UNABLE_WRITE'	=> 'It was not possible to write the configuration file. Alternative methods for this file to be created are presented below.',
	'CONFIG_FILE_WRITTEN'		=> 'The configuration file has been written. You may now proceed to the next step of the installation.',
	'CONFIG_PHPBB_EMPTY'		=> 'The phpBB3 config variable for “%s” is empty.',
	'CONFIG_RETRY'				=> 'Retry',
	'CONTACT_EMAIL_CONFIRM'		=> 'Confirm contact e-mail',
	'CONTINUE_CONVERT'			=> 'Continue conversion',
	'CONTINUE_CONVERT_BODY'		=> 'A previous conversion attempt has been determined. You are now able to choose between starting a new conversion or continuing the conversion.',
	'CONTINUE_LAST'				=> 'Continue last statements',
	'CONTINUE_OLD_CONVERSION'	=> 'Continue previously started conversion',
	'CONVERT'					=> 'Convert',
	'CONVERT_COMPLETE'			=> 'Conversion completed',
	'CONVERT_COMPLETE_EXPLAIN'	=> 'You have now successfully converted your board to phpBB 3.0. You can now login and <a href="../">access your board</a>. Please ensure that the settings were transferred correctly before enabling your board by deleting the install directory. Remember that help on using phpBB is available online via the <a href="http://www.phpbb.com/support/documentation/3.0/">Documentation</a> and the <a href="http://www.phpbb.com/community/viewforum.php?f=46">support forums</a>.',
	'CONVERT_INTRO'				=> 'Welcome to the phpBB Unified Convertor Framework',
	'CONVERT_INTRO_BODY'		=> 'From here, you are able to import data from other (installed) board systems. The list below shows all the conversion modules currently available. If there is no convertor shown in this list for the board software you wish to convert from, please check our website where further conversion modules may be available for download.',
	'CONVERT_NEW_CONVERSION'	=> 'New conversion',
	'CONVERT_NOT_EXIST'			=> 'The specified convertor does not exist.',
	'CONVERT_OPTIONS'			=> 'Options',
	'CONVERT_SETTINGS_VERIFIED'	=> 'The information you entered has been verified. To start the conversion process, please push the button below.',
	'CONV_ERR_FATAL'			=> 'Fatal conversion error',

	'CONV_ERROR_ATTACH_FTP_DIR'			=> 'FTP upload for attachments is enabled at the old board. Please disable the FTP upload option and make sure a valid upload directory is specified, then copy all attachment files to this new web accessible directory. Once you have done this, restart the convertor.',
	'CONV_ERROR_CONFIG_EMPTY'			=> 'There is no configuration information available for the conversion.',
	'CONV_ERROR_FORUM_ACCESS'			=> 'Unable to get forum access information.',
	'CONV_ERROR_GET_CATEGORIES'			=> 'Unable to get categories.',
	'CONV_ERROR_GET_CONFIG'				=> 'Could not retrieve your board configuration.',
	'CONV_ERROR_COULD_NOT_READ'			=> 'Unable to access/read “%s”.',
	'CONV_ERROR_GROUP_ACCESS'			=> 'Unable to get group authentication information.',
	'CONV_ERROR_INCONSISTENT_GROUPS'	=> 'Inconsistency in groups table detected in add_bots() - you need to add all special groups if you do it manually.',
	'CONV_ERROR_INSERT_BOT'				=> 'Unable to insert bot into users table.',
	'CONV_ERROR_INSERT_BOTGROUP'		=> 'Unable to insert bot into bots table.',
	'CONV_ERROR_INSERT_USER_GROUP'		=> 'Unable to insert user into user_group table.',
	'CONV_ERROR_MESSAGE_PARSER'			=> 'Message parser error',
	'CONV_ERROR_NO_AVATAR_PATH'			=> 'Note to developer: you must specify $convertor[\'avatar_path\'] to use %s.',
	'CONV_ERROR_NO_FORUM_PATH'			=> 'The relative path to the source board has not been specified.',
	'CONV_ERROR_NO_GALLERY_PATH'		=> 'Note to developer: you must specify $convertor[\'avatar_gallery_path\'] to use %s.',
	'CONV_ERROR_NO_GROUP'				=> 'Group “%1$s” could not be found in %2$s.',
	'CONV_ERROR_NO_RANKS_PATH'			=> 'Note to developer: you must specify $convertor[\'ranks_path\'] to use %s.',
	'CONV_ERROR_NO_SMILIES_PATH'		=> 'Note to developer: you must specify $convertor[\'smilies_path\'] to use %s.',
	'CONV_ERROR_NO_UPLOAD_DIR'			=> 'Note to developer: you must specify $convertor[\'upload_path\'] to use %s.',
	'CONV_ERROR_PERM_SETTING'			=> 'Unable to insert/update permission setting.',
	'CONV_ERROR_PM_COUNT'				=> 'Unable to select folder pm count.',
	'CONV_ERROR_REPLACE_CATEGORY'		=> 'Unable to insert new forum replacing old category.',
	'CONV_ERROR_REPLACE_FORUM'			=> 'Unable to insert new forum replacing old forum.',
	'CONV_ERROR_USER_ACCESS'			=> 'Unable to get user authentication information.',
	'CONV_ERROR_WRONG_GROUP'			=> 'Wrong group “%1$s” defined in %2$s.',
	'CONV_OPTIONS_BODY'					=> 'This page collects the data required to access the source board. Enter the database details of your former board; the converter will not change anything in the database given below. The source board should be disabled to allow a consistent conversion.',
	'CONV_SAVED_MESSAGES'				=> 'Saved messages',

	'COULD_NOT_COPY'			=> 'Could not copy file <strong>%1$s</strong> to <strong>%2$s</strong><br /><br />Please check that the target directory exists and is writable by the webserver.',
	'COULD_NOT_FIND_PATH'		=> 'Could not find path to your former board. Please check your settings and try again.<br />» %s was specified as the source path.',

	'DBMS'						=> 'Database type',
	'DB_CONFIG'					=> 'Database configuration',
	'DB_CONNECTION'				=> 'Database connection',
	'DB_ERR_INSERT'				=> 'Error while processing <code>INSERT</code> query.',
	'DB_ERR_LAST'				=> 'Error while processing <var>query_last</var>.',
	'DB_ERR_QUERY_FIRST'		=> 'Error while executing <var>query_first</var>.',
	'DB_ERR_QUERY_FIRST_TABLE'	=> 'Error while executing <var>query_first</var>, %s (“%s”).',
	'DB_ERR_SELECT'				=> 'Error while running <code>SELECT</code> query.',
	'DB_HOST'					=> 'Database server hostname or DSN',
	'DB_HOST_EXPLAIN'			=> 'DSN stands for Data Source Name and is relevant only for ODBC installs.',
	'DB_NAME'					=> 'Database name',
	'DB_PASSWORD'				=> 'Database password',
	'DB_PORT'					=> 'Database server port',
	'DB_PORT_EXPLAIN'			=> 'Leave this blank unless you know the server operates on a non-standard port.',
	'DB_USERNAME'				=> 'Database username',
	'DB_TEST'					=> 'Test connection',
	'DEFAULT_LANG'				=> 'Default board language',
	'DEFAULT_PREFIX_IS'			=> 'The convertor was not able to find tables with the specified prefix. Please make sure you have entered the correct details for the board you are converting from. The default table prefix for %1$s is <strong>%2$s</strong>.',
	'DEV_NO_TEST_FILE'			=> 'No value has been specified for the test_file variable in the convertor. If you are a user of this convertor, you should not be seeing this error, please report this message to the convertor author. If you are a convertor author, you must specify the name of a file which exists in the source board to allow the path to it to be verified.',
	'DIRECTORIES_AND_FILES'		=> 'Directory and file setup',
	'DISABLE_KEYS'				=> 'Disabling keys',
	'DLL_FIREBIRD'				=> 'Firebird',
	'DLL_FTP'					=> 'Remote FTP support [ Installation ]',
	'DLL_GD'					=> 'GD graphics support [ Visual Confirmation ]',
	'DLL_MBSTRING'				=> 'Multi-byte character support',
	'DLL_MSSQL'					=> 'MSSQL Server 2000+',
	'DLL_MSSQL_ODBC'			=> 'MSSQL Server 2000+ via ODBC',
	'DLL_MYSQL'					=> 'MySQL',
	'DLL_MYSQLI'				=> 'MySQL with MySQLi Extension',
	'DLL_ORACLE'				=> 'Oracle',
	'DLL_POSTGRES'				=> 'PostgreSQL 7.x/8.x',
	'DLL_SQLITE'				=> 'SQLite',
	'DLL_XML'					=> 'XML support [ Jabber ]',
	'DLL_ZLIB'					=> 'zlib compression support [ gz, .tar.gz, .zip ]',
	'DL_CONFIG'					=> 'Download config',
	'DL_CONFIG_EXPLAIN'			=> 'You may download the complete config.php to your own PC. You will then need to upload the file manually, replacing any existing config.php in your phpBB 3.0 root directory. Please remember to upload the file in ASCII format (see your FTP application documentation if you are unsure how to achieve this). When you have uploaded the config.php please click “Done” to move to the next stage.',
	'DL_DOWNLOAD'				=> 'Download',
	'DONE'						=> 'Done',

	'ENABLE_KEYS'				=> 'Re-enabling keys. This can take a while.',

	'FILES_OPTIONAL'			=> 'Optional files and directories',
	'FILES_OPTIONAL_EXPLAIN'	=> '<strong>Optional</strong> - These files, directories or permission settings are not required. The installation system will attempt to use various techniques to create them if they do not exist or cannot be written to. However, the presence of these will speed installation.',
	'FILES_REQUIRED'			=> 'Files and Directories',
	'FILES_REQUIRED_EXPLAIN'	=> '<strong>Required</strong> - In order to function correctly phpBB needs to be able to access or write to certain files or directories. If you see “Not Found” you need to create the relevant file or directory. If you see “Unwritable” you need to change the permissions on the file or directory to allow phpBB to write to it.',
	'FILLING_TABLE'				=> 'Filling table <strong>%s</strong>',
	'FILLING_TABLES'			=> 'Filling tables',
	'FINAL_STEP'				=> 'Process final step',
	'FORUM_ADDRESS'				=> 'Board address',
	'FORUM_ADDRESS_EXPLAIN'		=> 'This is the URL of your former board, for example <samp>http://www.example.com/phpBB2/</samp>. If an address is entered here and not left empty every instance of this address will be replaced by your new board address within messages, private messages and signatures.',
	'FORUM_PATH'				=> 'Board path',
	'FORUM_PATH_EXPLAIN'		=> 'This is the <strong>relative</strong> path on disk to your former board from the <strong>root of this phpBB3 installation</strong>.',
	'FOUND'						=> 'Found',
	'FTP_CONFIG'				=> 'Transfer config by FTP',
	'FTP_CONFIG_EXPLAIN'		=> 'phpBB has detected the presence of the FTP module on this server. You may attempt to install your config.php via this if you wish. You will need to supply the information listed below. Remember your username and password are those to your server! (ask your hosting provider for details if you are unsure what these are).',
	'FTP_PATH'					=> 'FTP path',
	'FTP_PATH_EXPLAIN'			=> 'This is the path from your root directory to that of phpBB, e.g. <samp>htdocs/phpBB3/</samp>.',
	'FTP_UPLOAD'				=> 'Upload',

	'GPL'						=> 'General Public License',

	'INITIAL_CONFIG'			=> 'Basic configuration',
	'INITIAL_CONFIG_EXPLAIN'	=> 'Now that install has determined your server can run phpBB you need to supply some specific information. If you do not know how to connect to your database please contact your hosting provider (in the first instance) or use the phpBB support forums. When entering data please ensure you check it thoroughly before continuing.',
	'INSTALL_CONGRATS'			=> 'Congratulations!',
	'INSTALL_CONGRATS_EXPLAIN'	=> '
		<p>You have now successfully installed phpBB %1$s. From here, you have two options as to what to do with your newly installed phpBB3:</p>
		<h2>Convert an existing board to phpBB3</h2>
		<p>The phpBB Unified Convertor Framework supports the conversion of phpBB 2.0.x and other board systems to phpBB3. If you have an existing board that you wish to convert, please <a href="%2$s">proceed on to the convertor</a>.</p>
		<h2>Go live with your phpBB3!</h2>
		<p>Clicking the button below will take you to your Administration Control Panel (ACP). Take some time to examine the options available to you. Remember that help is available online via the <a href="http://www.phpbb.com/support/documentation/3.0/">Documentation</a> and the <a href="http://www.phpbb.com/community/viewforum.php?f=46">support forums</a>, see the <a href="%3$s">README</a> for further information.</p><p><strong>Please now delete, move or rename the install directory before you use your board. If this directory is still present, only the Administration Control Panel (ACP) will be accessible.</strong></p>',
	'INSTALL_INTRO'				=> 'Welcome to Installation',

	'INSTALL_INTRO_BODY'		=> 'With this option, it is possible to install phpBB3 onto your server.</p><p>In order to proceed, you will need your database settings. If you do not know your database settings, please contact your host and ask for them. You will not be able to continue without them. You need:</p>

	<ul>
		<li>The Database Type - the database you will be using.</li>
		<li>The Database server hostname or DSN - the address of the database server.</li>
		<li>The Database server port - the port of the database server (most of the time this is not needed).</li>
		<li>The Database name - the name of the database on the server.</li>
		<li>The Database username and Database password - the login data to access the database.</li>
	</ul>

	<p><strong>Note:</strong> if you are installing using SQLite, you should enter the full path to your database file in the DSN field and leave the username and password fields blank. For security reasons, you should make sure that the database file is not stored in a location accessible from the web.</p>

	<p>phpBB3 supports the following databases:</p>
	<ul>
		<li>MySQL 3.23 or above (MySQLi supported)</li>
		<li>PostgreSQL 7.3+</li>
		<li>SQLite 2.8.2+</li>
		<li>Firebird 2.0+</li>
		<li>MS SQL Server 2000 or above (directly or via ODBC)</li>
		<li>Oracle</li>
	</ul>
	
	<p>Only those databases supported on your server will be displayed.',
	'INSTALL_INTRO_NEXT'		=> 'To commence the installation, please press the button below.',
	'INSTALL_LOGIN'				=> 'Login',
	'INSTALL_NEXT'				=> 'Next stage',
	'INSTALL_NEXT_FAIL'			=> 'Some tests failed and you should correct these problems before proceeding to the next stage. Failure to do so may result in an incomplete installation.',
	'INSTALL_NEXT_PASS'			=> 'All the basic tests have been passed and you may proceed to the next stage of installation. If you have changed any permissions, modules, etc. and wish to re-test you can do so if you wish.',
	'INSTALL_PANEL'				=> 'Installation Panel',
	'INSTALL_SEND_CONFIG'		=> 'Unfortunately phpBB could not write the configuration information directly to your config.php. This may be because the file does not exist or is not writable. A number of options will be listed below enabling you to complete installation of config.php.',
	'INSTALL_START'				=> 'Start install',
	'INSTALL_TEST'				=> 'Test again',
	'INST_ERR'					=> 'Installation error',
	'INST_ERR_DB_CONNECT'		=> 'Could not connect to the database, see error message below.',
	'INST_ERR_DB_FORUM_PATH'	=> 'The database file specified is within your board directory tree. You should put this file in a non web-accessible location.',
	'INST_ERR_DB_NO_ERROR'		=> 'No error message given.',
	'INST_ERR_DB_NO_MYSQLI'		=> 'The version of MySQL installed on this machine is incompatible with the “MySQL with MySQLi Extension” option you have selected. Please try the “MySQL” option instead.',
	'INST_ERR_DB_NO_SQLITE'		=> 'The version of the SQLite extension you have installed is too old, it must be upgraded to at least 2.8.2.',
	'INST_ERR_DB_NO_ORACLE'		=> 'The version of Oracle installed on this machine requires you to set the <var>NLS_CHARACTERSET</var> parameter to <var>UTF8</var>. Either upgrade your installation to 9.2+ or change the parameter.',
	'INST_ERR_DB_NO_FIREBIRD'	=> 'The version of Firebird installed on this machine is older than 2.0, please upgrade to a newer version.',
	'INST_ERR_DB_NO_FIREBIRD_PS'=> 'The database you selected for Firebird has a page size less than 8192, it must be at least 8192.',
	'INST_ERR_DB_NO_POSTGRES'	=> 'The database you have selected was not created in <var>UNICODE</var> or <var>UTF8</var> encoding. Try installing with a database in <var>UNICODE</var> or <var>UTF8</var> encoding.',
	'INST_ERR_DB_NO_NAME'		=> 'No database name specified.',
	'INST_ERR_EMAIL_INVALID'	=> 'The e-mail address you entered is invalid.',
	'INST_ERR_EMAIL_MISMATCH'	=> 'The e-mails you entered did not match.',
	'INST_ERR_FATAL'			=> 'Fatal installation error',
	'INST_ERR_FATAL_DB'			=> 'A fatal and unrecoverable database error has occurred. This may be because the specified user does not have appropriate permissions to <code>CREATE TABLES</code> or <code>INSERT</code> data, etc. Further information may be given below. Please contact your hosting provider in the first instance or the support forums of phpBB for further assistance.',
	'INST_ERR_FTP_PATH'			=> 'Could not change to the given directory, please check the path.',
	'INST_ERR_FTP_LOGIN'		=> 'Could not login to FTP server, check your username and password.',
	'INST_ERR_MISSING_DATA'		=> 'You must fill out all fields in this block.',
	'INST_ERR_NO_DB'			=> 'Cannot load the PHP module for the selected database type.',
	'INST_ERR_PASSWORD_MISMATCH'	=> 'The passwords you entered did not match.',
	'INST_ERR_PASSWORD_TOO_LONG'	=> 'The password you entered is too long. The maximum length is 30 characters.',
	'INST_ERR_PASSWORD_TOO_SHORT'	=> 'The password you entered is too short. The minimum length is 6 characters.',
	'INST_ERR_PREFIX'			=> 'Tables with the specified prefix already exist, please choose an alternative.',
	'INST_ERR_PREFIX_INVALID'	=> 'The table prefix you have specified is invalid for your database. Please try another, removing characters such as the hyphen.',
	'INST_ERR_PREFIX_TOO_LONG'	=> 'The table prefix you have specified is too long. The maximum length is %d characters.',
	'INST_ERR_USER_TOO_LONG'	=> 'The username you entered is too long. The maximum length is 20 characters.',
	'INST_ERR_USER_TOO_SHORT'	=> 'The username you entered is too short. The minimum length is 3 characters.',
	'INVALID_PRIMARY_KEY'		=> 'Invalid primary key : %s',

	'LONG_SCRIPT_EXECUTION'		=> 'Please note that this can take a while... Please do not stop the script.',

	// mbstring
	'MBSTRING_CHECK'						=> '<samp>mbstring</samp> extension check',
	'MBSTRING_CHECK_EXPLAIN'				=> '<strong>Required</strong> - <samp>mbstring</samp> is a PHP extension that provides multibyte string functions. Certain features of mbstring are not compatible with phpBB and must be disabled.',
	'MBSTRING_FUNC_OVERLOAD'				=> 'Function overloading',
	'MBSTRING_FUNC_OVERLOAD_EXPLAIN'		=> '<var>mbstring.func_overload</var> must be set to either 0 or 4.',
	'MBSTRING_ENCODING_TRANSLATION'			=> 'Transparent character encoding',
	'MBSTRING_ENCODING_TRANSLATION_EXPLAIN'	=> '<var>mbstring.encoding_translation</var> must be set to 0.',
	'MBSTRING_HTTP_INPUT'					=> 'HTTP input character conversion',
	'MBSTRING_HTTP_INPUT_EXPLAIN'			=> '<var>mbstring.http_input</var> must be set to <samp>pass</samp>.',
	'MBSTRING_HTTP_OUTPUT'					=> 'HTTP output character conversion',
	'MBSTRING_HTTP_OUTPUT_EXPLAIN'			=> '<var>mbstring.http_output</var> must be set to <samp>pass</samp>.',

	'MAKE_FOLDER_WRITABLE'		=> 'Please make sure that this folder exists and is writable by the webserver then try again:<br />»<strong>%s</strong>.',
	'MAKE_FOLDERS_WRITABLE'		=> 'Please make sure that these folders exist and are writable by the webserver then try again:<br />»<strong>%s</strong>.',

	'NAMING_CONFLICT'			=> 'Naming conflict: %s and %s are both aliases<br /><br />%s',
	'NEXT_STEP'					=> 'Proceed to next step',
	'NOT_FOUND'					=> 'Cannot find',
	'NOT_UNDERSTAND'			=> 'Could not understand %s #%d, table %s (“%s”)',
	'NO_CONVERTORS'				=> 'No convertors are available for use.',
	'NO_CONVERT_SPECIFIED'		=> 'No convertor specified.',
	'NO_LOCATION'				=> 'Cannot determine location. If you know Imagemagick is installed, you may specify the location later within your administration control panel',
	'NO_TABLES_FOUND'			=> 'No tables found.',

	'OVERVIEW_BODY'				=> 'Welcome to phpBB3!<br /><br />phpBB™ is the most widely used open source bulletin board solution in the world. phpBB3 is the latest installment in a seven year long package lineup. Like its predecessors, phpBB3 is feature-rich, user-friendly, and fully supported by the phpBB Team. phpBB3 greatly improves on what made phpBB2 popular, and adds commonly requested features that were not present in previous versions. We hope it exceeds your expectations.<br /><br />This installation system will guide you through installing phpBB3, updating to the latest version of phpBB3 from past releases, as well as converting to phpBB3 from a different discussion board system (including phpBB2). For more information, we encourage you to read <a href="../docs/INSTALL.html">the installation guide</a>.<br /><br />To read the phpBB3 license or learn about obtaining support and our stance on it, please select the respective options from the side menu. To continue, please select the appropriate tab above.',

	'PCRE_UTF_SUPPORT'				=> 'PCRE UTF-8 support',
	'PCRE_UTF_SUPPORT_EXPLAIN'		=> 'phpBB will <strong>not</strong> run if your PHP installation is not compiled with UTF-8 support in the PCRE extension.',
	'PHP_GETIMAGESIZE_SUPPORT'			=> 'PHP function getimagesize() is available',
	'PHP_GETIMAGESIZE_SUPPORT_EXPLAIN'	=> '<strong>Required</strong> - In order for phpBB to function correctly, the getimagesize function needs to be available.',
	'PHP_OPTIONAL_MODULE'			=> 'Optional modules',
	'PHP_OPTIONAL_MODULE_EXPLAIN'	=> '<strong>Optional</strong> - These modules or applications are optional. However, if they are available they will enable extra features.',
	'PHP_SUPPORTED_DB'				=> 'Supported databases',
	'PHP_SUPPORTED_DB_EXPLAIN'		=> '<strong>Required</strong> - You must have support for at least one compatible database within PHP. If no database modules are shown as available you should contact your hosting provider or review the relevant PHP installation documentation for advice.',
	'PHP_REGISTER_GLOBALS'			=> 'PHP setting <var>register_globals</var> is disabled',
	'PHP_REGISTER_GLOBALS_EXPLAIN'	=> 'phpBB will still run if this setting is enabled, but if possible, it is recommended that register_globals is disabled on your PHP install for security reasons.',
	'PHP_SAFE_MODE'					=> 'Safe mode',
	'PHP_SETTINGS'					=> 'PHP version and settings',
	'PHP_SETTINGS_EXPLAIN'			=> '<strong>Required</strong> - You must be running at least version 4.3.3 of PHP in order to install phpBB. If <var>safe mode</var> is displayed below your PHP installation is running in that mode. This will impose limitations on remote administration and similar features.',
	'PHP_URL_FOPEN_SUPPORT'			=> 'PHP setting <var>allow_url_fopen</var> is enabled',
	'PHP_URL_FOPEN_SUPPORT_EXPLAIN'	=> '<strong>Optional</strong> - This setting is optional, however certain phpBB functions like off-site avatars will not work properly without it. ',
	'PHP_VERSION_REQD'				=> 'PHP version >= 4.3.3',
	'POST_ID'						=> 'Post ID',
	'PREFIX_FOUND'					=> 'A scan of your tables has shown a valid installation using <strong>%s</strong> as table prefix.',
	'PREPROCESS_STEP'				=> 'Executing pre-processing functions/queries',
	'PRE_CONVERT_COMPLETE'			=> 'All pre-conversion steps have successfully been completed. You may now begin the actual conversion process. Please note that you may have to manually do and adjust several things. After conversion, especially check the permissions assigned, rebuild your search index which is not converted and also make sure files got copied correctly, for example avatars and smilies.',
	'PROCESS_LAST'					=> 'Processing last statements',

	'REFRESH_PAGE'				=> 'Refresh page to continue conversion',
	'REFRESH_PAGE_EXPLAIN'		=> 'If set to yes, the convertor will refresh the page to continue the conversion after having finished a step. If this is your first conversion for testing purposes and to determine any errors in advance, we suggest to set this to No.',
	'REQUIREMENTS_TITLE'		=> 'Installation compatibility',
	'REQUIREMENTS_EXPLAIN'		=> 'Before proceeding with the full installation phpBB will carry out some tests on your server configuration and files to ensure that you are able to install and run phpBB. Please ensure you read through the results thoroughly and do not proceed until all the required tests are passed. If you wish to use any of the features depending on the optional tests, you should ensure that these tests are passed also.',
	'RETRY_WRITE'				=> 'Retry writing config',
	'RETRY_WRITE_EXPLAIN'		=> 'If you wish you can change the permissions on config.php to allow phpBB to write to it. Should you wish to do that you can click Retry below to try again. Remember to return the permissions on config.php after phpBB has finished installation.',

	'SCRIPT_PATH'				=> 'Script path',
	'SCRIPT_PATH_EXPLAIN'		=> 'The path where phpBB is located relative to the domain name, e.g. <samp>/phpBB3</samp>.',
	'SELECT_LANG'				=> 'Select language',
	'SERVER_CONFIG'				=> 'Server configuration',
	'SEARCH_INDEX_UNCONVERTED'	=> 'Search index was not converted',
	'SEARCH_INDEX_UNCONVERTED_EXPLAIN'	=> 'Your old search index was not converted. Searching will always yield an empty result. To create a new search index go to the Administration Control Panel, select Maintenance and then choose Search index from the submenu.',
	'SOFTWARE'					=> 'Board software',
	'SPECIFY_OPTIONS'			=> 'Specify conversion options',
	'STAGE_ADMINISTRATOR'		=> 'Administrator details',
	'STAGE_ADVANCED'			=> 'Advanced settings',
	'STAGE_ADVANCED_EXPLAIN'	=> 'The settings on this page are only necessary to set if you know that you require something different from the default. If you are unsure, just proceed to the next page, as these settings can be altered from the Administration Control Panel later.',
	'STAGE_CONFIG_FILE'			=> 'Configuration file',
	'STAGE_CREATE_TABLE'		=> 'Create database tables',
	'STAGE_CREATE_TABLE_EXPLAIN'	=> 'The database tables used by phpBB 3.0 have been created and populated with some initial data. Proceed to the next screen to finish installing phpBB.',
	'STAGE_DATABASE'			=> 'Database settings',
	'STAGE_FINAL'				=> 'Final stage',
	'STAGE_INTRO'				=> 'Introduction',
	'STAGE_IN_PROGRESS'			=> 'Conversion in progress',
	'STAGE_REQUIREMENTS'		=> 'Requirements',
	'STAGE_SETTINGS'			=> 'Settings',
	'STARTING_CONVERT'			=> 'Starting conversion process',
	'STEP_PERCENT_COMPLETED'	=> 'Step <strong>%d</strong> of <strong>%d</strong>',
	'SUB_INTRO'					=> 'Introduction',
	'SUB_LICENSE'				=> 'License',
	'SUB_SUPPORT'				=> 'Support',
	'SUCCESSFUL_CONNECT'		=> 'Successful connection',
	'SUPPORT_BODY'				=> 'Full support will be provided for the current stable release of phpBB3, free of charge. This includes:</p><ul><li>installation</li><li>configuration</li><li>technical questions</li><li>problems relating to potential bugs in the software</li><li>updating from Release Candidate (RC) versions to the latest stable version</li><li>converting from phpBB 2.0.x to phpBB3</li><li>converting from other discussion board software to phpBB3 (please see the <a href="http://www.phpbb.com/community/viewforum.php?f=65">Convertors Forum</a>)</li></ul><p>We encourage users still running beta versions of phpBB3 to replace their installation with a fresh copy of the latest version.</p><h2>MODs / Styles</h2><p>For issues relating to MODs, please post in the appropriate <a href="http://www.phpbb.com/community/viewforum.php?f=81">Modifications Forum</a>.<br />For issues relating to styles, templates and imagesets, please post in the appropriate <a href="http://www.phpbb.com/community/viewforum.php?f=80">Styles Forum</a>.<br /><br />If your question relates to a specific package, please post directly in the topic dedicated to the package.</p><h2>Obtaining Support</h2><p><a href="http://www.phpbb.com/community/viewtopic.php?f=14&amp;t=571070">The phpBB Welcome Package</a><br /><a href="http://www.phpbb.com/support/">Support Section</a><br /><a href="http://www.phpbb.com/support/documentation/3.0/quickstart/">Quick Start Guide</a><br /><br />To ensure you stay up to date with the latest news and releases, why not <a href="http://www.phpbb.com/support/">subscribe to our mailing list</a>?<br /><br />',
	'SYNC_FORUMS'				=> 'Starting to synchronise forums',
	'SYNC_POST_COUNT'			=> 'Synchronising post_counts',
	'SYNC_POST_COUNT_ID'		=> 'Synchronising post_counts from <var>entry</var> %1$s to %2$s.',
	'SYNC_TOPICS'				=> 'Starting to synchronise topics',
	'SYNC_TOPIC_ID'				=> 'Synchronising topics from <var>topic_id</var> %1$s to %2$s.',

	'TABLES_MISSING'			=> 'Could not find these tables<br />» <strong>%s</strong>.',
	'TABLE_PREFIX'				=> 'Prefix for tables in database',
	'TABLE_PREFIX_SAME'			=> 'The table prefix needs to be the one used by the software you are converting from.<br />» Specified table prefix was %s.',
	'TESTS_PASSED'				=> 'Tests passed',
	'TESTS_FAILED'				=> 'Tests failed',

	'UNABLE_WRITE_LOCK'			=> 'Unable to write lock file.',
	'UNAVAILABLE'				=> 'Unavailable',
	'UNWRITABLE'				=> 'Unwritable',
	'UPDATE_TOPICS_POSTED'		=> 'Generating topics posted information',
	'UPDATE_TOPICS_POSTED_ERR'	=> 'An error occured while generating topics posted information. You can retry this step in the ACP after the conversion process is completed.',
	'VERIFY_OPTIONS'			=> 'Verifying conversion options',
	'VERSION'					=> 'Version',

	'WELCOME_INSTALL'			=> 'Welcome to phpBB3 Installation',
	'WRITABLE'					=> 'Writable',
));

// Updater
$lang = array_merge($lang, array(
	'ALL_FILES_UP_TO_DATE'		=> 'All files are up to date with the latest phpBB version. You should now <a href="../ucp.php?mode=login">login to your board</a> and check if everything is working fine. Do not forget to delete, rename or move your install directory!',
	'ARCHIVE_FILE'				=> 'Source file within archive',

	'BACK'				=> 'Back',
	'BINARY_FILE'		=> 'Binary file',
	'BOT'				=> 'Spider/Robot',

	'CHANGE_CLEAN_NAMES'			=> 'The method used to make sure a username is not used by multiple users has been changed. There are some users which have the same name when compared with the new method. You have to delete or rename these users to make sure that each name is only used by one user before you can proceed.',
	'CHECK_FILES'					=> 'Check files',
	'CHECK_FILES_AGAIN'				=> 'Check files again',
	'CHECK_FILES_EXPLAIN'			=> 'Within the next step all files will be checked against the update files - this can take a while if this is the first file check.',
	'CHECK_FILES_UP_TO_DATE'		=> 'According to your database your version is up to date. You may want to proceed with the file check to make sure all files are really up to date with the latest phpBB version.',
	'CHECK_UPDATE_DATABASE'			=> 'Continue update process',
	'COLLECTED_INFORMATION'			=> 'File information',
	'COLLECTED_INFORMATION_EXPLAIN'	=> 'The list below shows information about the files needing an update. Please read the information in front of every status block to see what they mean and what you may need to do to perform a successful update.',
	'COLLECTING_FILE_DIFFS'			=> 'Collecting file differences',
	'COMPLETE_LOGIN_TO_BOARD'		=> 'You should now <a href="../ucp.php?mode=login">login to your board</a> and check if everything is working fine. Do not forget to delete, rename or move your install directory!',
	'CONTINUE_UPDATE_NOW'			=> 'Continue the update process now',
	'CURRENT_FILE'					=> 'Beginning of current original file',
	'CURRENT_VERSION'				=> 'Current version',

	'DATABASE_TYPE'						=> 'Database type',
	'DATABASE_UPDATE_INFO_OLD'			=> 'The database update file within the install directory is outdated. Please make sure you uploaded the correct version of the file.',
	'DELETE_USER_REMOVE'				=> 'Delete user and remove posts',
	'DELETE_USER_RETAIN'				=> 'Delete user but keep posts',
	'DESTINATION'						=> 'Destination file',
	'DIFF_INLINE'						=> 'Inline',
	'DIFF_RAW'							=> 'Raw unified diff',
	'DIFF_SEP_EXPLAIN'					=> 'End of current original file / Beginning of new updated file',
	'DIFF_SIDE_BY_SIDE'					=> 'Side by Side',
	'DIFF_UNIFIED'						=> 'Unified diff',
	'DO_NOT_UPDATE'						=> 'Do not update this file',
	'DONE'								=> 'Done',
	'DOWNLOAD'							=> 'Download',
	'DOWNLOAD_AS'						=> 'Download as',
	'DOWNLOAD_UPDATE_METHOD'			=> 'Download modified files archive',
	'DOWNLOAD_UPDATE_METHOD_EXPLAIN'	=> 'Once downloaded you should unpack the archive. You will find the modified files you need to upload to your phpBB root directory within it. Please upload the files to their respective locations then. After you have uploaded all files, please check the files again with the other button below.',

	'ERROR'			=> 'Error',
	'EDIT_USERNAME'	=> 'Edit username',

	'FILE_ALREADY_UP_TO_DATE'		=> 'File is already up to date.',
	'FILE_DIFF_NOT_ALLOWED'			=> 'File not allowed to be diffed.',
	'FILE_USED'						=> 'Information used from',			// Single file
	'FILES_CONFLICT'				=> 'Conflict files',
	'FILES_CONFLICT_EXPLAIN'		=> 'The following files are modified and do not represent the original files from the old version. phpBB determined that these files create conflicts if they are tried to be merged. Please investigate the conflicts and try to manually resolve them or continue the update choosing the preferred merging method. If you resolve the conflicts manually check the files again after you modified them. You are also able to choose between the preferred merge method for every file. The first one will result in a file where the conflicting lines from your old file will be lost, the other one will result in loosing the changes from the newer file.',
	'FILES_MODIFIED'				=> 'Modified files',
	'FILES_MODIFIED_EXPLAIN'		=> 'The following files are modified and do not represent the original files from the old version. The updated file will be a merge between your modifications and the new file.',
	'FILES_NEW'						=> 'New files',
	'FILES_NEW_EXPLAIN'				=> 'The following files currently do not exist within your installation. These files will be added to your installation.',
	'FILES_NEW_CONFLICT'			=> 'New conflicting files',
	'FILES_NEW_CONFLICT_EXPLAIN'	=> 'The following files are new within the latest version but it has been determined that there is already a file with the same name within the same position. This file will be overwritten by the new file.',
	'FILES_NOT_MODIFIED'			=> 'Not modified files',
	'FILES_NOT_MODIFIED_EXPLAIN'	=> 'The following files are not modified and represent the original phpBB files from the version you want to update from.',
	'FILES_UP_TO_DATE'				=> 'Already updated files',
	'FILES_UP_TO_DATE_EXPLAIN'		=> 'The following files are already up to date and do not need to be updated.',
	'FTP_SETTINGS'					=> 'FTP settings',
	'FTP_UPDATE_METHOD'				=> 'FTP upload',

	'INCOMPATIBLE_UPDATE_FILES'		=> 'The update files found are incompatible with your installed version. Your installed version is %1$s and the update file is for updating phpBB %2$s to %3$s.',
	'INCOMPLETE_UPDATE_FILES'		=> 'The update files are incomplete.',
	'INLINE_UPDATE_SUCCESSFUL'		=> 'The database update was successful. Now you need to continue the update process.',

	'KEEP_OLD_NAME'		=> 'Keep username',

	'LATEST_VERSION'		=> 'Latest version',
	'LINE'					=> 'Line',
	'LINE_ADDED'			=> 'Added',
	'LINE_MODIFIED'			=> 'Modified',
	'LINE_REMOVED'			=> 'Removed',
	'LINE_UNMODIFIED'		=> 'Unmodified',
	'LOGIN_UPDATE_EXPLAIN'	=> 'In order to update your installation you need to login first.',

	'MAPPING_FILE_STRUCTURE'	=> 'To ease the upload here are the file locations which map your phpBB installation.',

	'MERGE_MODIFICATIONS_OPTION'	=> 'Merge modifications',

	'MERGE_NO_MERGE_NEW_OPTION'	=> 'Do not merge - use new file',
	'MERGE_NO_MERGE_MOD_OPTION'	=> 'Do not merge - use currently installed file',
	'MERGE_MOD_FILE_OPTION'		=> 'Merge differences and use modified code within conflicting block',
	'MERGE_NEW_FILE_OPTION'		=> 'Merge differences and use new file code within conflicting block',
	'MERGE_SELECT_ERROR'		=> 'Conflicting file merge modes are not correctly selected.',
	'MERGING_FILES'				=> 'Merging differences',
	'MERGING_FILES_EXPLAIN'		=> 'Currently collecting final file changes.<br /><br />Please wait until phpBB has completed all operations on changed files.',

	'NEW_FILE'						=> 'End of new updated file',
	'NEW_USERNAME'					=> 'New username',
	'NO_AUTH_UPDATE'				=> 'Not authorised to update',
	'NO_ERRORS'						=> 'No errors',
	'NO_UPDATE_FILES'				=> 'Not updating the following files',
	'NO_UPDATE_FILES_EXPLAIN'		=> 'The following files are new or modified but the directory they normally reside in could not be found on your installation. If this list contains files to other directories than language/ or styles/ than you may have modified your directory structure and the update may be incomplete.',
	'NO_UPDATE_FILES_OUTDATED'		=> 'No valid update directory was found, please make sure you uploaded the relevant files.<br /><br />Your installation does <strong>not</strong> seem to be up to date. Updates are available for your version of phpBB %1$s, please visit <a href="http://www.phpbb.com/downloads/" rel="external">http://www.phpbb.com/downloads/</a> to obtain the correct package to update from Version %2$s to Version %3$s.',
	'NO_UPDATE_FILES_UP_TO_DATE'	=> 'Your version is up to date. There is no need to run the update tool. If you want to make an integrity check on your files make sure you uploaded the correct update files.',
	'NO_UPDATE_INFO'				=> 'Update file information could not be found.',
	'NO_UPDATES_REQUIRED'			=> 'No updates required',
	'NO_VISIBLE_CHANGES'			=> 'No visible changes',
	'NOTICE'						=> 'Notice',
	'NUM_CONFLICTS'					=> 'Number of conflicts',
	'NUMBER_OF_FILES_COLLECTED'		=> 'Currently differences of %1$d of %2$d files have been checked.<br />Please wait until all files are checked.',

	'OLD_UPDATE_FILES'		=> 'Update files are out of date. The update files found are for updating from phpBB %1$s to phpBB %2$s but the latest version of phpBB is %3$s.',

	'PACKAGE_UPDATES_TO'				=> 'Current package updates to version',
	'PERFORM_DATABASE_UPDATE'			=> 'Perform database update',
	'PERFORM_DATABASE_UPDATE_EXPLAIN'	=> 'Below you will find a button to the database update script. The database update can take a while, so please do not stop the execution if it seems to hang. After the database update has been performed just follow the instructions to continue the update process.',
	'PREVIOUS_VERSION'					=> 'Previous version',
	'PROGRESS'							=> 'Progress',

	'RESULT'					=> 'Result',
	'RUN_DATABASE_SCRIPT'		=> 'Update my database now',

	'SELECT_DIFF_MODE'			=> 'Select diff mode',
	'SELECT_DOWNLOAD_FORMAT'	=> 'Select download archive format',
	'SELECT_FTP_SETTINGS'		=> 'Select FTP settings',
	'SHOW_DIFF_CONFLICT'		=> 'Show differences/conflicts',
	'SHOW_DIFF_FINAL'			=> 'Show resulting file',
	'SHOW_DIFF_MODIFIED'		=> 'Show merged differences',
	'SHOW_DIFF_NEW'				=> 'Show file contents',
	'SHOW_DIFF_NEW_CONFLICT'	=> 'Show differences',
	'SHOW_DIFF_NOT_MODIFIED'	=> 'Show differences',
	'SOME_QUERIES_FAILED'		=> 'Some queries failed, the statements and errors are listing below.',
	'SQL'						=> 'SQL',
	'SQL_FAILURE_EXPLAIN'		=> 'This is probably nothing to worry about, update will continue. Should this fail to complete you may need to seek help at our support forums. See <a href="../docs/README.html">README</a> for details on how to obtain advice.',
	'STAGE_FILE_CHECK'			=> 'Check files',
	'STAGE_UPDATE_DB'			=> 'Update database',
	'STAGE_UPDATE_FILES'		=> 'Update files',
	'STAGE_VERSION_CHECK'		=> 'Version check',
	'STATUS_CONFLICT'			=> 'Modified file producing conflicts',
	'STATUS_MODIFIED'			=> 'Modified file',
	'STATUS_NEW'				=> 'New file',
	'STATUS_NEW_CONFLICT'		=> 'Conflicting new file',
	'STATUS_NOT_MODIFIED'		=> 'Not modified file',
	'STATUS_UP_TO_DATE'			=> 'Already updated file',

	'TOGGLE_DISPLAY'			=> 'View/Hide file list',

	'UPDATE_COMPLETED'				=> 'Update completed',
	'UPDATE_DATABASE'				=> 'Update database',
	'UPDATE_DATABASE_EXPLAIN'		=> 'Within the next step the database will be updated.',
	'UPDATE_DATABASE_SCHEMA'		=> 'Updating database schema',
	'UPDATE_FILES'					=> 'Update files',
	'UPDATE_FILES_NOTICE'			=> 'Please make sure you have updated your board files too, this file is only updating your database.',
	'UPDATE_INSTALLATION'			=> 'Update phpBB installation',
	'UPDATE_INSTALLATION_EXPLAIN'	=> 'With this option, it is possible to update your phpBB installation to the latest version.<br />During the process all of your files will be checked for their integrity. You are able to review all differences and files before the update.<br /><br />The file update itself can be done in two different ways.</p><h2>Manual Update</h2><p>With this update you only download your personal set of changed files to make sure you do not lose your file modifications you may have done. After you downloaded this package you need to manually upload the files to their correct position under your phpBB root directory. Once done, you are able to do the file check stage again to see if you moved the files to their correct location.</p><h2>Automatic Update with FTP</h2><p>This method is similar to the first one but without the need to download the changed files and uploading them on your own. This will be done for you. In order to use this method you need to know your FTP login details since you will be asked for them. Once finished you will be redirected to the file check again to make sure everything got updated correctly.<br /><br />',
	'UPDATE_INSTRUCTIONS'			=> '

		<h1>Release announcement</h1>

		<p>Please read <a href="%1$s" title="%1$s"><strong>the release announcement for the latest version</strong></a> before you continue your update process, it may contain useful information. It also contains full download links as well as the change log.</p>

		<br />

		<h1>How to update your installation with the Automatic Update Package</h1>

		<p>The recommended way of updating your installation listed here is only valid for the automatic update package. You are also able to update your installation using the methods listed within the INSTALL.html document. The steps for updating phpBB3 automatically are:</p>

		<ul style="margin-left: 20px; font-size: 1.1em;">
			<li>Go to the <a href="http://www.phpbb.com/downloads/" title="http://www.phpbb.com/downloads/">phpBB.com downloads page</a> and download the "Automatic Update Package" archive.<br /><br /></li>
			<li>Unpack the archive.<br /><br /></li>
			<li>Upload the complete uncompressed install folder to your phpBB root directory (where your config.php file is).<br /><br /></li>
		</ul>

		<p>Once uploaded your board will be offline for normal users due to the install directory you uploaded now present.<br /><br />
		<strong><a href="%2$s" title="%2$s">Now start the update process by pointing your browser to the install folder</a>.</strong><br />
		<br />
		You will then be guided through the update process. You will be notified once the update is complete.
		</p>
	',
	'UPDATE_INSTRUCTIONS_INCOMPLETE'	=> '

		<h1>Incomplete update detected</h1>

		<p>phpBB detected an incomplete automatic update. Please make sure you followed every step within the automatic update tool. Below you will find the link again, or go directly to your install directory.</p>
	',
	'UPDATE_METHOD'					=> 'Update method',
	'UPDATE_METHOD_EXPLAIN'			=> 'You are now able to choose your preferred update method. Using the FTP upload will present you with a form you need to enter your FTP account details into. With this method the files will be automatically moved to the new location and backups of the old files being created by appending .bak to the filename. If you choose to download the modified files you are able to unpack and upload them to their correct location manually later.',
	'UPDATE_REQUIRES_FILE'			=> 'The updater requires that the following file is present: %s',
	'UPDATE_SUCCESS'				=> 'Update was successful',
	'UPDATE_SUCCESS_EXPLAIN'		=> 'Successfully updated all files. The next step involves checking all files again to make sure the files got updated correctly.',
	'UPDATE_VERSION_OPTIMIZE'		=> 'Updating version and optimising tables',
	'UPDATING_DATA'					=> 'Updating data',
	'UPDATING_TO_LATEST_STABLE'		=> 'Updating database to latest stable release',
	'UPDATED_VERSION'				=> 'Updated version',
	'UPLOAD_METHOD'					=> 'Upload method',

	'UPDATE_DB_SUCCESS'				=> 'Database update was successful.',
	'USER_ACTIVE'					=> 'Active user',
	'USER_INACTIVE'					=> 'Inactive user',

	'VERSION_CHECK'				=> 'Version check',
	'VERSION_CHECK_EXPLAIN'		=> 'Checks to see if the version of phpBB you are currently running is up to date.',
	'VERSION_NOT_UP_TO_DATE'	=> 'Your version of phpBB is not up to date. Please continue the update process.',
	'VERSION_NOT_UP_TO_DATE_ACP'=> 'Your version of phpBB is not up to date.<br />Below you will find a link to the release announcement for the latest version as well as instructions on how to perform the update.',
	'VERSION_UP_TO_DATE'		=> 'Your installation is up to date, no updates are available for your version of phpBB. You may want to continue anyway to perform a file validity check.',
	'VERSION_UP_TO_DATE_ACP'	=> 'Your installation is up to date, no updates are available for your version of phpBB. You do not need to update your installation.',
	'VIEWING_FILE_CONTENTS'		=> 'Viewing file contents',
	'VIEWING_FILE_DIFF'			=> 'Viewing file differences',

	'WRONG_INFO_FILE_FORMAT'	=> 'Wrong info file format',
));

// Default database schema entries...
$lang = array_merge($lang, array(
	'CONFIG_BOARD_EMAIL_SIG'		=> 'Thanks, The Management',
	'CONFIG_SITE_DESC'				=> 'A short text to describe your forum',
	'CONFIG_SITENAME'				=> 'yourdomain.com',

	'DEFAULT_INSTALL_POST'			=> 'This is an example post in your phpBB3 installation. Everything seems to be working. You may delete this post if you like and continue to set up your board. During the installation process your first category and your first forum are assigned an appropriate set of permissions for the predefined usergroups administrators, bots, global moderators, guests, registered users and registered COPPA users. If you also choose to delete your first category and your first forum, do not forget to assign permissions for all these usergroups for all new categories and forums you create. It is recommended to rename your first category and your first forum and copy permissions from these while creating new categories and forums. Have fun!',

	'EXT_GROUP_ARCHIVES'			=> 'Archives',
	'EXT_GROUP_DOCUMENTS'			=> 'Documents',
	'EXT_GROUP_DOWNLOADABLE_FILES'	=> 'Downloadable Files',
	'EXT_GROUP_FLASH_FILES'			=> 'Flash Files',
	'EXT_GROUP_IMAGES'				=> 'Images',
	'EXT_GROUP_PLAIN_TEXT'			=> 'Plain Text',
	'EXT_GROUP_QUICKTIME_MEDIA'		=> 'Quicktime Media',
	'EXT_GROUP_REAL_MEDIA'			=> 'Real Media',
	'EXT_GROUP_WINDOWS_MEDIA'		=> 'Windows Media',

	'FORUMS_FIRST_CATEGORY'			=> 'Your first category',
	'FORUMS_TEST_FORUM_DESC'		=> 'Description of your first forum.',
	'FORUMS_TEST_FORUM_TITLE'		=> 'Your first forum',

	'RANKS_SITE_ADMIN_TITLE'		=> 'Site Admin',
	'REPORT_WAREZ'					=> 'The post contains links to illegal or pirated software.',
	'REPORT_SPAM'					=> 'The reported post has the only purpose to advertise for a website or another product.',
	'REPORT_OFF_TOPIC'				=> 'The reported post is off topic.',
	'REPORT_OTHER'					=> 'The reported post does not fit into any other category, please use the further information field.',

	'SMILIES_ARROW'					=> 'Arrow',
	'SMILIES_CONFUSED'				=> 'Confused',
	'SMILIES_COOL'					=> 'Cool',
	'SMILIES_CRYING'				=> 'Crying or Very Sad',
	'SMILIES_EMARRASSED'			=> 'Embarrassed',
	'SMILIES_EVIL'					=> 'Evil or Very Mad',
	'SMILIES_EXCLAMATION'			=> 'Exclamation',
	'SMILIES_GEEK'					=> 'Geek',
	'SMILIES_IDEA'					=> 'Idea',
	'SMILIES_LAUGHING'				=> 'Laughing',
	'SMILIES_MAD'					=> 'Mad',
	'SMILIES_MR_GREEN'				=> 'Mr. Green',
	'SMILIES_NEUTRAL'				=> 'Neutral',
	'SMILIES_QUESTION'				=> 'Question',
	'SMILIES_RAZZ'					=> 'Razz',
	'SMILIES_ROLLING_EYES'			=> 'Rolling Eyes',
	'SMILIES_SAD'					=> 'Sad',
	'SMILIES_SHOCKED'				=> 'Shocked',
	'SMILIES_SMILE'					=> 'Smile',
	'SMILIES_SURPRISED'				=> 'Surprised',
	'SMILIES_TWISTED_EVIL'			=> 'Twisted Evil',
	'SMILIES_UBER_GEEK'				=> 'Uber Geek',
	'SMILIES_VERY_HAPPY'			=> 'Very Happy',
	'SMILIES_WINK'					=> 'Wink',

	'TOPICS_TOPIC_TITLE'			=> 'Welcome to phpBB3',
));

?>