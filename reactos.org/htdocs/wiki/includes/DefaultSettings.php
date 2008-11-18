<?php
/**
 *
 *                 NEVER EDIT THIS FILE
 *
 *
 * To customize your installation, edit "LocalSettings.php". If you make
 * changes here, they will be lost on next upgrade of MediaWiki!
 *
 * Note that since all these string interpolations are expanded
 * before LocalSettings is included, if you localize something
 * like $wgScriptPath, you must also localize everything that
 * depends on it.
 *
 * Documentation is in the source and on:
 * http://www.mediawiki.org/wiki/Manual:Configuration_settings
 *
 */

# This is not a valid entry point, perform no further processing unless MEDIAWIKI is defined
if( !defined( 'MEDIAWIKI' ) ) {
	echo "This file is part of MediaWiki and is not a valid entry point\n";
	die( 1 );
}

/**
 * Create a site configuration object
 * Not used for much in a default install
 */
require_once( "$IP/includes/SiteConfiguration.php" );
$wgConf = new SiteConfiguration;

/** MediaWiki version number */
$wgVersion			= '1.13.2';

/** Name of the site. It must be changed in LocalSettings.php */
$wgSitename         = 'MediaWiki';

/**
 * Name of the project namespace. If left set to false, $wgSitename will be
 * used instead.
 */
$wgMetaNamespace    = false;

/**
 * Name of the project talk namespace.
 *
 * Normally you can ignore this and it will be something like
 * $wgMetaNamespace . "_talk". In some languages, you may want to set this
 * manually for grammatical reasons. It is currently only respected by those
 * languages where it might be relevant and where no automatic grammar converter
 * exists.
 */
$wgMetaNamespaceTalk = false;


/** URL of the server. It will be automatically built including https mode */
$wgServer = '';

if( isset( $_SERVER['SERVER_NAME'] ) ) {
	$wgServerName = $_SERVER['SERVER_NAME'];
} elseif( isset( $_SERVER['HOSTNAME'] ) ) {
	$wgServerName = $_SERVER['HOSTNAME'];
} elseif( isset( $_SERVER['HTTP_HOST'] ) ) {
	$wgServerName = $_SERVER['HTTP_HOST'];
} elseif( isset( $_SERVER['SERVER_ADDR'] ) ) {
	$wgServerName = $_SERVER['SERVER_ADDR'];
} else {
	$wgServerName = 'localhost';
}

# check if server use https:
$wgProto = (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on') ? 'https' : 'http';

$wgServer = $wgProto.'://' . $wgServerName;
# If the port is a non-standard one, add it to the URL
if(    isset( $_SERVER['SERVER_PORT'] )
	&& !strpos( $wgServerName, ':' )
    && (    ( $wgProto == 'http' && $_SERVER['SERVER_PORT'] != 80 )
	 || ( $wgProto == 'https' && $_SERVER['SERVER_PORT'] != 443 ) ) ) {

	$wgServer .= ":" . $_SERVER['SERVER_PORT'];
}


/**
 * The path we should point to.
 * It might be a virtual path in case with use apache mod_rewrite for example
 *
 * This *needs* to be set correctly.
 *
 * Other paths will be set to defaults based on it unless they are directly
 * set in LocalSettings.php
 */
$wgScriptPath	    = '/wiki';

/**
 * Whether to support URLs like index.php/Page_title These often break when PHP
 * is set up in CGI mode. PATH_INFO *may* be correct if cgi.fix_pathinfo is set,
 * but then again it may not; lighttpd converts incoming path data to lowercase
 * on systems with case-insensitive filesystems, and there have been reports of
 * problems on Apache as well.
 *
 * To be safe we'll continue to keep it off by default.
 *
 * Override this to false if $_SERVER['PATH_INFO'] contains unexpectedly
 * incorrect garbage, or to true if it is really correct.
 *
 * The default $wgArticlePath will be set based on this value at runtime, but if
 * you have customized it, having this incorrectly set to true can cause
 * redirect loops when "pretty URLs" are used.
 */
$wgUsePathInfo =
	( strpos( php_sapi_name(), 'cgi' ) === false ) &&
	( strpos( php_sapi_name(), 'apache2filter' ) === false ) &&
	( strpos( php_sapi_name(), 'isapi' ) === false );


/**@{
 * Script users will request to get articles
 * ATTN: Old installations used wiki.phtml and redirect.phtml - make sure that
 * LocalSettings.php is correctly set!
 *
 * Will be set based on $wgScriptPath in Setup.php if not overridden in
 * LocalSettings.php. Generally you should not need to change this unless you
 * don't like seeing "index.php".
 */
$wgScriptExtension  = '.php'; ///< extension to append to script names by default
$wgScript           = false; ///< defaults to "{$wgScriptPath}/index{$wgScriptExtension}"
$wgRedirectScript   = false; ///< defaults to "{$wgScriptPath}/redirect{$wgScriptExtension}"
/**@}*/


/**@{
 * These various web and file path variables are set to their defaults
 * in Setup.php if they are not explicitly set from LocalSettings.php.
 * If you do override them, be sure to set them all!
 *
 * These will relatively rarely need to be set manually, unless you are
 * splitting style sheets or images outside the main document root.
 */
/**
 * style path as seen by users
 */
$wgStylePath   = false; ///< defaults to "{$wgScriptPath}/skins"
/**
 * filesystem stylesheets directory
 */
$wgStyleDirectory = false; ///< defaults to "{$IP}/skins"
$wgStyleSheetPath = &$wgStylePath;
$wgArticlePath      = false; ///< default to "{$wgScript}/$1" or "{$wgScript}?title=$1", depending on $wgUsePathInfo
$wgVariantArticlePath = false;
$wgUploadPath       = false; ///< defaults to "{$wgScriptPath}/images"
$wgUploadDirectory	= false; ///< defaults to "{$IP}/images"
$wgHashedUploadDirectory	= true;
$wgLogo				= false; ///< defaults to "{$wgStylePath}/common/images/wiki.png"
$wgFavicon			= '/favicon.ico';
$wgAppleTouchIcon   = false; ///< This one'll actually default to off. For iPhone and iPod Touch web app bookmarks
$wgMathPath         = false; ///< defaults to "{$wgUploadPath}/math"
$wgMathDirectory    = false; ///< defaults to "{$wgUploadDirectory}/math"
$wgTmpDirectory     = false; ///< defaults to "{$wgUploadDirectory}/tmp"
$wgUploadBaseUrl    = "";
/**@}*/

/**
 * Default value for chmoding of new directories.
 */
$wgDirectoryMode = 0777;

/**
 * New file storage paths; currently used only for deleted files.
 * Set it like this:
 *
 *   $wgFileStore['deleted']['directory'] = '/var/wiki/private/deleted';
 *
 */
$wgFileStore = array();
$wgFileStore['deleted']['directory'] = false;///< Defaults to $wgUploadDirectory/deleted
$wgFileStore['deleted']['url'] = null;       ///< Private
$wgFileStore['deleted']['hash'] = 3;         ///< 3-level subdirectory split

/**@{
 * File repository structures
 *
 * $wgLocalFileRepo is a single repository structure, and $wgForeignFileRepo is
 * a an array of such structures. Each repository structure is an associative
 * array of properties configuring the repository.
 *
 * Properties required for all repos:
 *    class             The class name for the repository. May come from the core or an extension.
 *                      The core repository classes are LocalRepo, ForeignDBRepo, FSRepo.
 *
 *    name				A unique name for the repository.
 *
 * For all core repos:
 *    url               Base public URL
 *    hashLevels        The number of directory levels for hash-based division of files
 *    thumbScriptUrl    The URL for thumb.php (optional, not recommended)
 *    transformVia404   Whether to skip media file transformation on parse and rely on a 404
 *                      handler instead.
 *    initialCapital    Equivalent to $wgCapitalLinks, determines whether filenames implicitly
 *                      start with a capital letter. The current implementation may give incorrect
 *                      description page links when the local $wgCapitalLinks and initialCapital
 *                      are mismatched.
 *    pathDisclosureProtection
 *                      May be 'paranoid' to remove all parameters from error messages, 'none' to
 *                      leave the paths in unchanged, or 'simple' to replace paths with
 *                      placeholders. Default for LocalRepo is 'simple'.
 *
 * These settings describe a foreign MediaWiki installation. They are optional, and will be ignored
 * for local repositories:
 *    descBaseUrl       URL of image description pages, e.g. http://en.wikipedia.org/wiki/Image:
 *    scriptDirUrl      URL of the MediaWiki installation, equivalent to $wgScriptPath, e.g.
 *                      http://en.wikipedia.org/w
 *
 *    articleUrl        Equivalent to $wgArticlePath, e.g. http://en.wikipedia.org/wiki/$1
 *    fetchDescription  Fetch the text of the remote file description page. Equivalent to
 *                      $wgFetchCommonsDescriptions.
 *
 * ForeignDBRepo:
 *    dbType, dbServer, dbUser, dbPassword, dbName, dbFlags
 *                      equivalent to the corresponding member of $wgDBservers
 *    tablePrefix       Table prefix, the foreign wiki's $wgDBprefix
 *    hasSharedCache    True if the wiki's shared cache is accessible via the local $wgMemc
 *
 * The default is to initialise these arrays from the MW<1.11 backwards compatible settings:
 * $wgUploadPath, $wgThumbnailScriptPath, $wgSharedUploadDirectory, etc.
 */
$wgLocalFileRepo = false;
$wgForeignFileRepos = array();
/**@}*/

/**
 * Allowed title characters -- regex character class
 * Don't change this unless you know what you're doing
 *
 * Problematic punctuation:
 *  []{}|#    Are needed for link syntax, never enable these
 *  <>        Causes problems with HTML escaping, don't use
 *  %         Enabled by default, minor problems with path to query rewrite rules, see below
 *  +         Enabled by default, but doesn't work with path to query rewrite rules, corrupted by apache
 *  ?         Enabled by default, but doesn't work with path to PATH_INFO rewrites
 *
 * All three of these punctuation problems can be avoided by using an alias, instead of a
 * rewrite rule of either variety.
 *
 * The problem with % is that when using a path to query rewrite rule, URLs are
 * double-unescaped: once by Apache's path conversion code, and again by PHP. So
 * %253F, for example, becomes "?". Our code does not double-escape to compensate
 * for this, indeed double escaping would break if the double-escaped title was
 * passed in the query string rather than the path. This is a minor security issue
 * because articles can be created such that they are hard to view or edit.
 *
 * In some rare cases you may wish to remove + for compatibility with old links.
 *
 * Theoretically 0x80-0x9F of ISO 8859-1 should be disallowed, but
 * this breaks interlanguage links
 */
$wgLegalTitleChars = " %!\"$&'()*,\\-.\\/0-9:;=?@A-Z\\\\^_`a-z~\\x80-\\xFF+";


/**
 * The external URL protocols
 */
$wgUrlProtocols = array(
	'http://',
	'https://',
	'ftp://',
	'irc://',
	'gopher://',
	'telnet://', // Well if we're going to support the above.. -ævar
	'nntp://', // @bug 3808 RFC 1738
	'worldwind://',
	'mailto:',
	'news:'
);

/** internal name of virus scanner. This servers as a key to the $wgAntivirusSetup array.
 * Set this to NULL to disable virus scanning. If not null, every file uploaded will be scanned for viruses.
 */
$wgAntivirus= NULL;

/** Configuration for different virus scanners. This an associative array of associative arrays:
 * it contains on setup array per known scanner type. The entry is selected by $wgAntivirus, i.e.
 * valid values for $wgAntivirus are the keys defined in this array.
 *
 * The configuration array for each scanner contains the following keys: "command", "codemap", "messagepattern";
 *
 * "command" is the full command to call the virus scanner - %f will be replaced with the name of the
 * file to scan. If not present, the filename will be appended to the command. Note that this must be
 * overwritten if the scanner is not in the system path; in that case, plase set
 * $wgAntivirusSetup[$wgAntivirus]['command'] to the desired command with full path.
 *
 * "codemap" is a mapping of exit code to return codes of the detectVirus function in SpecialUpload.
 * An exit code mapped to AV_SCAN_FAILED causes the function to consider the scan to be failed. This will pass
 * the file if $wgAntivirusRequired is not set.
 * An exit code mapped to AV_SCAN_ABORTED causes the function to consider the file to have an usupported format,
 * which is probably imune to virusses. This causes the file to pass.
 * An exit code mapped to AV_NO_VIRUS will cause the file to pass, meaning no virus was found.
 * All other codes (like AV_VIRUS_FOUND) will cause the function to report a virus.
 * You may use "*" as a key in the array to catch all exit codes not mapped otherwise.
 *
 * "messagepattern" is a perl regular expression to extract the meaningful part of the scanners
 * output. The relevant part should be matched as group one (\1).
 * If not defined or the pattern does not match, the full message is shown to the user.
 */
$wgAntivirusSetup = array(

	#setup for clamav
	'clamav' => array (
		'command' => "clamscan --no-summary ",

		'codemap' => array (
			"0" =>  AV_NO_VIRUS, # no virus
			"1" =>  AV_VIRUS_FOUND, # virus found
			"52" => AV_SCAN_ABORTED, # unsupported file format (probably imune)
			"*" =>  AV_SCAN_FAILED, # else scan failed
		),

		'messagepattern' => '/.*?:(.*)/sim',
	),

	#setup for f-prot
	'f-prot' => array (
		'command' => "f-prot ",

		'codemap' => array (
			"0" => AV_NO_VIRUS, # no virus
			"3" => AV_VIRUS_FOUND, # virus found
			"6" => AV_VIRUS_FOUND, # virus found
			"*" => AV_SCAN_FAILED, # else scan failed
		),

		'messagepattern' => '/.*?Infection:(.*)$/m',
	),
);


/** Determines if a failed virus scan (AV_SCAN_FAILED) will cause the file to be rejected. */
$wgAntivirusRequired= true;

/** Determines if the mime type of uploaded files should be checked */
$wgVerifyMimeType= true;

/** Sets the mime type definition file to use by MimeMagic.php. */
$wgMimeTypeFile= "includes/mime.types";
#$wgMimeTypeFile= "/etc/mime.types";
#$wgMimeTypeFile= NULL; #use built-in defaults only.

/** Sets the mime type info file to use by MimeMagic.php. */
$wgMimeInfoFile= "includes/mime.info";
#$wgMimeInfoFile= NULL; #use built-in defaults only.

/** Switch for loading the FileInfo extension by PECL at runtime.
 * This should be used only if fileinfo is installed as a shared object
 * or a dynamic libary
 */
$wgLoadFileinfoExtension= false;

/** Sets an external mime detector program. The command must print only
 * the mime type to standard output.
 * The name of the file to process will be appended to the command given here.
 * If not set or NULL, mime_content_type will be used if available.
 */
$wgMimeDetectorCommand= NULL; # use internal mime_content_type function, available since php 4.3.0
#$wgMimeDetectorCommand= "file -bi"; #use external mime detector (Linux)

/** Switch for trivial mime detection. Used by thumb.php to disable all fance
 * things, because only a few types of images are needed and file extensions
 * can be trusted.
 */
$wgTrivialMimeDetection= false;

/**
 * Additional XML types we can allow via mime-detection.
 * array = ( 'rootElement' => 'associatedMimeType' )
 */
$wgXMLMimeTypes = array(
		'http://www.w3.org/2000/svg:svg'    			=> 'image/svg+xml',
		'svg'                               			=> 'image/svg+xml',
		'http://www.lysator.liu.se/~alla/dia/:diagram' 	=> 'application/x-dia-diagram',
		'http://www.w3.org/1999/xhtml:html'				=> 'text/html', // application/xhtml+xml?
		'html'                              			=> 'text/html', // application/xhtml+xml?
);

/**
 * To set 'pretty' URL paths for actions other than
 * plain page views, add to this array. For instance:
 *   'edit' => "$wgScriptPath/edit/$1"
 *
 * There must be an appropriate script or rewrite rule
 * in place to handle these URLs.
 */
$wgActionPaths = array();

/**
 * If you operate multiple wikis, you can define a shared upload path here.
 * Uploads to this wiki will NOT be put there - they will be put into
 * $wgUploadDirectory.
 * If $wgUseSharedUploads is set, the wiki will look in the shared repository if
 * no file of the given name is found in the local repository (for [[Image:..]],
 * [[Media:..]] links). Thumbnails will also be looked for and generated in this
 * directory.
 *
 * Note that these configuration settings can now be defined on a per-
 * repository basis for an arbitrary number of file repositories, using the
 * $wgForeignFileRepos variable.
 */
$wgUseSharedUploads = false;
/** Full path on the web server where shared uploads can be found */
$wgSharedUploadPath = "http://commons.wikimedia.org/shared/images";
/** Fetch commons image description pages and display them on the local wiki? */
$wgFetchCommonsDescriptions = false;
/** Path on the file system where shared uploads can be found. */
$wgSharedUploadDirectory = "/var/www/wiki3/images";
/** DB name with metadata about shared directory. Set this to false if the uploads do not come from a wiki. */
$wgSharedUploadDBname = false;
/** Optional table prefix used in database. */
$wgSharedUploadDBprefix = '';
/** Cache shared metadata in memcached. Don't do this if the commons wiki is in a different memcached domain */
$wgCacheSharedUploads = true;
/** Allow for upload to be copied from an URL. Requires Special:Upload?source=web */
$wgAllowCopyUploads = false;
/**
 * Max size for uploads, in bytes.  Currently only works for uploads from URL
 * via CURL (see $wgAllowCopyUploads).  The only way to impose limits on
 * normal uploads is currently to edit php.ini.
 */
$wgMaxUploadSize = 1024*1024*100; # 100MB

/**
 * Point the upload navigation link to an external URL
 * Useful if you want to use a shared repository by default
 * without disabling local uploads (use $wgEnableUploads = false for that)
 * e.g. $wgUploadNavigationUrl = 'http://commons.wikimedia.org/wiki/Special:Upload';
 */
$wgUploadNavigationUrl = false;

/**
 * Give a path here to use thumb.php for thumbnail generation on client request, instead of
 * generating them on render and outputting a static URL. This is necessary if some of your
 * apache servers don't have read/write access to the thumbnail path.
 *
 * Example:
 *   $wgThumbnailScriptPath = "{$wgScriptPath}/thumb{$wgScriptExtension}";
 */
$wgThumbnailScriptPath = false;
$wgSharedThumbnailScriptPath = false;

/**
 * Set the following to false especially if you have a set of files that need to
 * be accessible by all wikis, and you do not want to use the hash (path/a/aa/)
 * directory layout.
 */
$wgHashedSharedUploadDirectory = true;

/**
 * Base URL for a repository wiki. Leave this blank if uploads are just stored
 * in a shared directory and not meant to be accessible through a separate wiki.
 * Otherwise the image description pages on the local wiki will link to the
 * image description page on this wiki.
 *
 * Please specify the namespace, as in the example below.
 */
$wgRepositoryBaseUrl = "http://commons.wikimedia.org/wiki/Image:";

#
# Email settings
#

/**
 * Site admin email address
 * Default to wikiadmin@SERVER_NAME
 */
$wgEmergencyContact = 'wikiadmin@' . $wgServerName;

/**
 * Password reminder email address
 * The address we should use as sender when a user is requesting his password
 * Default to apache@SERVER_NAME
 */
$wgPasswordSender	= 'MediaWiki Mail <apache@' . $wgServerName . '>';

/**
 * dummy address which should be accepted during mail send action
 * It might be necessay to adapt the address or to set it equal
 * to the $wgEmergencyContact address
 */
#$wgNoReplyAddress	= $wgEmergencyContact;
$wgNoReplyAddress	= 'reply@not.possible';

/**
 * Set to true to enable the e-mail basic features:
 * Password reminders, etc. If sending e-mail on your
 * server doesn't work, you might want to disable this.
 */
$wgEnableEmail = true;

/**
 * Set to true to enable user-to-user e-mail.
 * This can potentially be abused, as it's hard to track.
 */
$wgEnableUserEmail = true;

/**
 * Set to true to put the sending user's email in a Reply-To header
 * instead of From. ($wgEmergencyContact will be used as From.)
 *
 * Some mailers (eg sSMTP) set the SMTP envelope sender to the From value,
 * which can cause problems with SPF validation and leak recipient addressses
 * when bounces are sent to the sender.
 */
$wgUserEmailUseReplyTo = false;

/**
 * Minimum time, in hours, which must elapse between password reminder
 * emails for a given account. This is to prevent abuse by mail flooding.
 */
$wgPasswordReminderResendTime = 24;

/**
 * SMTP Mode
 * For using a direct (authenticated) SMTP server connection.
 * Default to false or fill an array :
 * <code>
 * "host" => 'SMTP domain',
 * "IDHost" => 'domain for MessageID',
 * "port" => "25",
 * "auth" => true/false,
 * "username" => user,
 * "password" => password
 * </code>
 */
$wgSMTP				= false;


/**@{
 * Database settings
 */
/** database host name or ip address */
$wgDBserver         = 'localhost';
/** database port number */
$wgDBport           = '';
/** name of the database */
$wgDBname           = 'wikidb';
/** */
$wgDBconnection     = '';
/** Database username */
$wgDBuser           = 'wikiuser';
/** Database user's password */
$wgDBpassword       = '';
/** Database type */
$wgDBtype           = 'mysql';

/** Search type
 * Leave as null to select the default search engine for the
 * selected database type (eg SearchMySQL), or set to a class
 * name to override to a custom search engine.
 */
$wgSearchType	    = null;

/** Table name prefix */
$wgDBprefix         = '';
/** MySQL table options to use during installation or update */
$wgDBTableOptions   = 'ENGINE=InnoDB';

/** Mediawiki schema */
$wgDBmwschema       = 'mediawiki';
/** Tsearch2 schema */
$wgDBts2schema      = 'public';

/** To override default SQLite data directory ($docroot/../data) */
$wgSQLiteDataDir    = '';

/**
 * Make all database connections secretly go to localhost. Fool the load balancer
 * thinking there is an arbitrarily large cluster of servers to connect to.
 * Useful for debugging.
 */
$wgAllDBsAreLocalhost = false;

/**@}*/


/** Live high performance sites should disable this - some checks acquire giant mysql locks */
$wgCheckDBSchema = true;


/**
 * Shared database for multiple wikis. Commonly used for storing a user table
 * for single sign-on. The server for this database must be the same as for the
 * main database.
 * For backwards compatibility the shared prefix is set to the same as the local
 * prefix, and the user table is listed in the default list of shared tables.
 *
 * $wgSharedTables may be customized with a list of tables to share in the shared
 * datbase. However it is advised to limit what tables you do share as many of
 * MediaWiki's tables may have side effects if you try to share them.
 * EXPERIMENTAL
 */
$wgSharedDB     = null;
$wgSharedPrefix = false; # Defaults to $wgDBprefix
$wgSharedTables = array( 'user' );

/**
 * Database load balancer
 * This is a two-dimensional array, an array of server info structures
 * Fields are:
 *   host:        Host name
 *   dbname:      Default database name
 *   user:        DB user
 *   password:    DB password
 *   type:        "mysql" or "postgres"
 *   load:        ratio of DB_SLAVE load, must be >=0, the sum of all loads must be >0
 *   groupLoads:  array of load ratios, the key is the query group name. A query may belong
 *                to several groups, the most specific group defined here is used.
 *
 *   flags:       bit field
 *                   DBO_DEFAULT -- turns on DBO_TRX only if !$wgCommandLineMode (recommended)
 *                   DBO_DEBUG -- equivalent of $wgDebugDumpSql
 *                   DBO_TRX -- wrap entire request in a transaction
 *                   DBO_IGNORE -- ignore errors (not useful in LocalSettings.php)
 *                   DBO_NOBUFFER -- turn off buffering (not useful in LocalSettings.php)
 *
 *   max lag:     (optional) Maximum replication lag before a slave will taken out of rotation
 *   max threads: (optional) Maximum number of running threads
 *
 *   These and any other user-defined properties will be assigned to the mLBInfo member
 *   variable of the Database object.
 *
 * Leave at false to use the single-server variables above. If you set this
 * variable, the single-server variables will generally be ignored (except
 * perhaps in some command-line scripts).
 *
 * The first server listed in this array (with key 0) will be the master. The
 * rest of the servers will be slaves. To prevent writes to your slaves due to
 * accidental misconfiguration or MediaWiki bugs, set read_only=1 on all your
 * slaves in my.cnf. You can set read_only mode at runtime using:
 *
 *     SET @@read_only=1;
 *
 * Since the effect of writing to a slave is so damaging and difficult to clean
 * up, we at Wikimedia set read_only=1 in my.cnf on all our DB servers, even
 * our masters, and then set read_only=0 on masters at runtime.
 */
$wgDBservers		= false;

/**
 * Load balancer factory configuration
 * To set up a multi-master wiki farm, set the class here to something that
 * can return a LoadBalancer with an appropriate master on a call to getMainLB().
 * The class identified here is responsible for reading $wgDBservers,
 * $wgDBserver, etc., so overriding it may cause those globals to be ignored.
 *
 * The LBFactory_Multi class is provided for this purpose, please see
 * includes/db/LBFactory_Multi.php for configuration information.
 */
$wgLBFactoryConf    = array( 'class' => 'LBFactory_Simple' );

/** How long to wait for a slave to catch up to the master */
$wgMasterWaitTimeout = 10;

/** File to log database errors to */
$wgDBerrorLog		= false;

/** When to give an error message */
$wgDBClusterTimeout = 10;

/**
 * Scale load balancer polling time so that under overload conditions, the database server
 * receives a SHOW STATUS query at an average interval of this many microseconds
 */
$wgDBAvgStatusPoll = 2000;

/**
 * wgDBminWordLen :
 * MySQL 3.x : used to discard words that MySQL will not return any results for
 * shorter values configure mysql directly.
 * MySQL 4.x : ignore it and configure mySQL
 * See: http://dev.mysql.com/doc/mysql/en/Fulltext_Fine-tuning.html
 */
$wgDBminWordLen     = 4;
/** Set to true if using InnoDB tables */
$wgDBtransactions	= false;
/** Set to true for compatibility with extensions that might be checking.
 * MySQL 3.23.x is no longer supported. */
$wgDBmysql4			= true;

/**
 * Set to true to engage MySQL 4.1/5.0 charset-related features;
 * for now will just cause sending of 'SET NAMES=utf8' on connect.
 *
 * WARNING: THIS IS EXPERIMENTAL!
 *
 * May break if you're not using the table defs from mysql5/tables.sql.
 * May break if you're upgrading an existing wiki if set differently.
 * Broken symptoms likely to include incorrect behavior with page titles,
 * usernames, comments etc containing non-ASCII characters.
 * Might also cause failures on the object cache and other things.
 *
 * Even correct usage may cause failures with Unicode supplementary
 * characters (those not in the Basic Multilingual Plane) unless MySQL
 * has enhanced their Unicode support.
 */
$wgDBmysql5			= false;

/**
 * Other wikis on this site, can be administered from a single developer
 * account.
 * Array numeric key => database name
 */
$wgLocalDatabases = array();

/** @{
 * Object cache settings
 * See Defines.php for types
 */
$wgMainCacheType = CACHE_NONE;
$wgMessageCacheType = CACHE_ANYTHING;
$wgParserCacheType = CACHE_ANYTHING;
/**@}*/

$wgParserCacheExpireTime = 86400;

$wgSessionsInMemcached = false;

/**@{
 * Memcached-specific settings
 * See docs/memcached.txt
 */
$wgUseMemCached     = false;
$wgMemCachedDebug   = false; ///< Will be set to false in Setup.php, if the server isn't working
$wgMemCachedServers = array( '127.0.0.1:11000' );
$wgMemCachedPersistent = false;
/**@}*/

/**
 * Directory for local copy of message cache, for use in addition to memcached
 */
$wgLocalMessageCache = false;
/**
 * Defines format of local cache
 * true - Serialized object
 * false - PHP source file (Warning - security risk)
 */
$wgLocalMessageCacheSerialized = true;

/**
 * Directory for compiled constant message array databases
 * WARNING: turning anything on will just break things, aaaaaah!!!!
 */
$wgCachedMessageArrays = false;

# Language settings
#
/** Site language code, should be one of ./languages/Language(.*).php */
$wgLanguageCode = 'en';

/**
 * Some languages need different word forms, usually for different cases.
 * Used in Language::convertGrammar().
 */
$wgGrammarForms = array();
#$wgGrammarForms['en']['genitive']['car'] = 'car\'s';

/** Treat language links as magic connectors, not inline links */
$wgInterwikiMagic = true;

/** Hide interlanguage links from the sidebar */
$wgHideInterlanguageLinks = false;

/** List of language names or overrides for default names in Names.php */
$wgExtraLanguageNames = array();

/** We speak UTF-8 all the time now, unless some oddities happen */
$wgInputEncoding  = 'UTF-8';
$wgOutputEncoding = 'UTF-8';
$wgEditEncoding   = '';

/**
 * Locale for LC_CTYPE, to work around http://bugs.php.net/bug.php?id=45132
 * For Unix-like operating systems, set this to to a locale that has a UTF-8 
 * character set. Only the character set is relevant.
 */
$wgShellLocale = 'en_US.utf8';

/**
 * Set this to eg 'ISO-8859-1' to perform character set
 * conversion when loading old revisions not marked with
 * "utf-8" flag. Use this when converting wiki to UTF-8
 * without the burdensome mass conversion of old text data.
 *
 * NOTE! This DOES NOT touch any fields other than old_text.
 * Titles, comments, user names, etc still must be converted
 * en masse in the database before continuing as a UTF-8 wiki.
 */
$wgLegacyEncoding   = false;

/**
 * If set to true, the MediaWiki 1.4 to 1.5 schema conversion will
 * create stub reference rows in the text table instead of copying
 * the full text of all current entries from 'cur' to 'text'.
 *
 * This will speed up the conversion step for large sites, but
 * requires that the cur table be kept around for those revisions
 * to remain viewable.
 *
 * maintenance/migrateCurStubs.php can be used to complete the
 * migration in the background once the wiki is back online.
 *
 * This option affects the updaters *only*. Any present cur stub
 * revisions will be readable at runtime regardless of this setting.
 */
$wgLegacySchemaConversion = false;

$wgMimeType			= 'text/html';
$wgJsMimeType			= 'text/javascript';
$wgDocType			= '-//W3C//DTD XHTML 1.0 Transitional//EN';
$wgDTD				= 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd';
$wgXhtmlDefaultNamespace	= 'http://www.w3.org/1999/xhtml';

/**
 * Permit other namespaces in addition to the w3.org default.
 * Use the prefix for the key and the namespace for the value. For
 * example:
 * $wgXhtmlNamespaces['svg'] = 'http://www.w3.org/2000/svg';
 * Normally we wouldn't have to define this in the root <html>
 * element, but IE needs it there in some circumstances.
 */
$wgXhtmlNamespaces		= array();

/** Enable to allow rewriting dates in page text.
 * DOES NOT FORMAT CORRECTLY FOR MOST LANGUAGES */
$wgUseDynamicDates  = false;
/** Enable dates like 'May 12' instead of '12 May', this only takes effect if
 * the interface is set to English
 */
$wgAmericanDates    = false;
/**
 * For Hindi and Arabic use local numerals instead of Western style (0-9)
 * numerals in interface.
 */
$wgTranslateNumerals = true;

/**
 * Translation using MediaWiki: namespace.
 * This will increase load times by 25-60% unless memcached is installed.
 * Interface messages will be loaded from the database.
 */
$wgUseDatabaseMessages = true;

/**
 * Expiry time for the message cache key
 */
$wgMsgCacheExpiry	= 86400;

/**
 * Maximum entry size in the message cache, in bytes
 */
$wgMaxMsgCacheEntrySize = 10000;

/**
 * Set to false if you are thorough system admin who always remembers to keep
 * serialized files up to date to save few mtime calls.
 */
$wgCheckSerialized = true;

/** Whether to enable language variant conversion. */
$wgDisableLangConversion = false;

/** Default variant code, if false, the default will be the language code */
$wgDefaultLanguageVariant = false;

/**
 * Show a bar of language selection links in the user login and user
 * registration forms; edit the "loginlanguagelinks" message to
 * customise these
 */
$wgLoginLanguageSelector = false;

/**
 * Whether to use zhdaemon to perform Chinese text processing
 * zhdaemon is under developement, so normally you don't want to
 * use it unless for testing
 */
$wgUseZhdaemon = false;
$wgZhdaemonHost="localhost";
$wgZhdaemonPort=2004;


# Miscellaneous configuration settings
#

$wgLocalInterwiki   = 'w';
$wgInterwikiExpiry = 10800; # Expiry time for cache of interwiki table

/** Interwiki caching settings.
	$wgInterwikiCache specifies path to constant database file
		This cdb database is generated by dumpInterwiki from maintenance
		and has such key formats:
			dbname:key - a simple key (e.g. enwiki:meta)
			_sitename:key - site-scope key (e.g. wiktionary:meta)
			__global:key - global-scope key (e.g. __global:meta)
			__sites:dbname - site mapping (e.g. __sites:enwiki)
		Sites mapping just specifies site name, other keys provide
			"local url" data layout.
	$wgInterwikiScopes specify number of domains to check for messages:
		1 - Just wiki(db)-level
		2 - wiki and global levels
		3 - site levels
	$wgInterwikiFallbackSite - if unable to resolve from cache
 */
$wgInterwikiCache = false;
$wgInterwikiScopes = 3;
$wgInterwikiFallbackSite = 'wiki';

/**
 * If local interwikis are set up which allow redirects,
 * set this regexp to restrict URLs which will be displayed
 * as 'redirected from' links.
 *
 * It might look something like this:
 * $wgRedirectSources = '!^https?://[a-z-]+\.wikipedia\.org/!';
 *
 * Leave at false to avoid displaying any incoming redirect markers.
 * This does not affect intra-wiki redirects, which don't change
 * the URL.
 */
$wgRedirectSources = false;


$wgShowIPinHeader	= true; # For non-logged in users
$wgMaxSigChars		= 255;  # Maximum number of Unicode characters in signature
$wgMaxArticleSize	= 2048; # Maximum article size in kilobytes
# Maximum number of bytes in username. You want to run the maintenance
# script ./maintenancecheckUsernames.php once you have changed this value
$wgMaxNameChars		= 255;

$wgMaxPPNodeCount = 1000000;  # A complexity limit on template expansion

/**
 * Maximum recursion depth for templates within templates.
 * The current parser adds two levels to the PHP call stack for each template,
 * and xdebug limits the call stack to 100 by default. So this should hopefully
 * stop the parser before it hits the xdebug limit.
 */
$wgMaxTemplateDepth = 40;
$wgMaxPPExpandDepth = 40;

$wgExtraSubtitle	= '';
$wgSiteSupportPage	= ''; # A page where you users can receive donations

/***
 * If this lock file exists, the wiki will be forced into read-only mode.
 * Its contents will be shown to users as part of the read-only warning
 * message.
 */
$wgReadOnlyFile         = false; ///< defaults to "{$wgUploadDirectory}/lock_yBgMBwiR";

/**
 * The debug log file should be not be publicly accessible if it is used, as it
 * may contain private data. */
$wgDebugLogFile         = '';

$wgDebugRedirects		= false;
$wgDebugRawPage         = false; # Avoid overlapping debug entries by leaving out CSS

$wgDebugComments        = false;
$wgReadOnly             = null;
$wgLogQueries           = false;

/**
 * Write SQL queries to the debug log
 */
$wgDebugDumpSql         = false;

/**
 * Set to an array of log group keys to filenames.
 * If set, wfDebugLog() output for that group will go to that file instead
 * of the regular $wgDebugLogFile. Useful for enabling selective logging
 * in production.
 */
$wgDebugLogGroups       = array();

/**
 * Show the contents of $wgHooks in Special:Version
 */
$wgSpecialVersionShowHooks =  false;

/**
 * Whether to show "we're sorry, but there has been a database error" pages.
 * Displaying errors aids in debugging, but may display information useful
 * to an attacker.
 */
$wgShowSQLErrors        = false;

/**
 * If true, some error messages will be colorized when running scripts on the
 * command line; this can aid picking important things out when debugging.
 * Ignored when running on Windows or when output is redirected to a file.
 */
$wgColorErrors          = true;

/**
 * If set to true, uncaught exceptions will print a complete stack trace
 * to output. This should only be used for debugging, as it may reveal
 * private information in function parameters due to PHP's backtrace
 * formatting.
 */
$wgShowExceptionDetails = false;

/**
 * Expose backend server host names through the API and various HTML comments
 */
$wgShowHostnames = false;

/**
 * Use experimental, DMOZ-like category browser
 */
$wgUseCategoryBrowser   = false;

/**
 * Keep parsed pages in a cache (objectcache table, turck, or memcached)
 * to speed up output of the same page viewed by another user with the
 * same options.
 *
 * This can provide a significant speedup for medium to large pages,
 * so you probably want to keep it on.
 */
$wgEnableParserCache = true;

/**
 * If on, the sidebar navigation links are cached for users with the
 * current language set. This can save a touch of load on a busy site
 * by shaving off extra message lookups.
 *
 * However it is also fragile: changing the site configuration, or
 * having a variable $wgArticlePath, can produce broken links that
 * don't update as expected.
 */
$wgEnableSidebarCache = false;

/**
 * Expiry time for the sidebar cache, in seconds
 */
$wgSidebarCacheExpiry = 86400;

/**
 * Under which condition should a page in the main namespace be counted
 * as a valid article? If $wgUseCommaCount is set to true, it will be
 * counted if it contains at least one comma. If it is set to false
 * (default), it will only be counted if it contains at least one [[wiki
 * link]]. See http://meta.wikimedia.org/wiki/Help:Article_count
 *
 * Retroactively changing this variable will not affect
 * the existing count (cf. maintenance/recount.sql).
 */
$wgUseCommaCount = false;

/**
 * wgHitcounterUpdateFreq sets how often page counters should be updated, higher
 * values are easier on the database. A value of 1 causes the counters to be
 * updated on every hit, any higher value n cause them to update *on average*
 * every n hits. Should be set to either 1 or something largish, eg 1000, for
 * maximum efficiency.
 */
$wgHitcounterUpdateFreq = 1;

# Basic user rights and block settings
$wgSysopUserBans        = true; # Allow sysops to ban logged-in users
$wgSysopRangeBans       = true; # Allow sysops to ban IP ranges
$wgAutoblockExpiry      = 86400; # Number of seconds before autoblock entries expire
$wgBlockAllowsUTEdit    = false; # Blocks allow users to edit their own user talk page
$wgSysopEmailBans       = true; # Allow sysops to ban users from accessing Emailuser

# Pages anonymous user may see as an array, e.g.:
# array ( "Main Page", "Wikipedia:Help");
# Special:Userlogin and Special:Resetpass are always whitelisted.
# NOTE: This will only work if $wgGroupPermissions['*']['read']
# is false -- see below. Otherwise, ALL pages are accessible,
# regardless of this setting.
# Also note that this will only protect _pages in the wiki_.
# Uploaded files will remain readable. Make your upload
# directory name unguessable, or use .htaccess to protect it.
$wgWhitelistRead = false;

/**
 * Should editors be required to have a validated e-mail
 * address before being allowed to edit?
 */
$wgEmailConfirmToEdit=false;

/**
 * Permission keys given to users in each group.
 * All users are implicitly in the '*' group including anonymous visitors;
 * logged-in users are all implicitly in the 'user' group. These will be
 * combined with the permissions of all groups that a given user is listed
 * in in the user_groups table.
 *
 * Note: Don't set $wgGroupPermissions = array(); unless you know what you're
 * doing! This will wipe all permissions, and may mean that your users are
 * unable to perform certain essential tasks or access new functionality
 * when new permissions are introduced and default grants established.
 *
 * Functionality to make pages inaccessible has not been extensively tested
 * for security. Use at your own risk!
 *
 * This replaces wgWhitelistAccount and wgWhitelistEdit
 */
$wgGroupPermissions = array();

// Implicit group for all visitors
$wgGroupPermissions['*'    ]['createaccount']    = true;
$wgGroupPermissions['*'    ]['read']             = true;
$wgGroupPermissions['*'    ]['edit']             = true;
$wgGroupPermissions['*'    ]['createpage']       = true;
$wgGroupPermissions['*'    ]['createtalk']       = true;
$wgGroupPermissions['*'    ]['writeapi']         = true;

// Implicit group for all logged-in accounts
$wgGroupPermissions['user' ]['move']             = true;
$wgGroupPermissions['user' ]['move-subpages']    = true;
$wgGroupPermissions['user' ]['read']             = true;
$wgGroupPermissions['user' ]['edit']             = true;
$wgGroupPermissions['user' ]['createpage']       = true;
$wgGroupPermissions['user' ]['createtalk']       = true;
$wgGroupPermissions['user' ]['writeapi']         = true;
$wgGroupPermissions['user' ]['upload']           = true;
$wgGroupPermissions['user' ]['reupload']         = true;
$wgGroupPermissions['user' ]['reupload-shared']  = true;
$wgGroupPermissions['user' ]['minoredit']        = true;
$wgGroupPermissions['user' ]['purge']            = true; // can use ?action=purge without clicking "ok"

// Implicit group for accounts that pass $wgAutoConfirmAge
$wgGroupPermissions['autoconfirmed']['autoconfirmed'] = true;

// Users with bot privilege can have their edits hidden
// from various log pages by default
$wgGroupPermissions['bot'  ]['bot']              = true;
$wgGroupPermissions['bot'  ]['autoconfirmed']    = true;
$wgGroupPermissions['bot'  ]['nominornewtalk']   = true;
$wgGroupPermissions['bot'  ]['autopatrol']       = true;
$wgGroupPermissions['bot'  ]['suppressredirect'] = true;
$wgGroupPermissions['bot'  ]['apihighlimits']    = true;
$wgGroupPermissions['bot'  ]['writeapi']         = true;
#$wgGroupPermissions['bot'  ]['editprotected']    = true; // can edit all protected pages without cascade protection enabled

// Most extra permission abilities go to this group
$wgGroupPermissions['sysop']['block']            = true;
$wgGroupPermissions['sysop']['createaccount']    = true;
$wgGroupPermissions['sysop']['delete']           = true;
$wgGroupPermissions['sysop']['bigdelete']        = true; // can be separately configured for pages with > $wgDeleteRevisionsLimit revs
$wgGroupPermissions['sysop']['deletedhistory']   = true; // can view deleted history entries, but not see or restore the text
$wgGroupPermissions['sysop']['undelete']         = true;
$wgGroupPermissions['sysop']['editinterface']    = true;
$wgGroupPermissions['sysop']['editusercssjs']    = true;
$wgGroupPermissions['sysop']['import']           = true;
$wgGroupPermissions['sysop']['importupload']     = true;
$wgGroupPermissions['sysop']['move']             = true;
$wgGroupPermissions['sysop']['move-subpages']    = true;
$wgGroupPermissions['sysop']['patrol']           = true;
$wgGroupPermissions['sysop']['autopatrol']       = true;
$wgGroupPermissions['sysop']['protect']          = true;
$wgGroupPermissions['sysop']['proxyunbannable']  = true;
$wgGroupPermissions['sysop']['rollback']         = true;
$wgGroupPermissions['sysop']['trackback']        = true;
$wgGroupPermissions['sysop']['upload']           = true;
$wgGroupPermissions['sysop']['reupload']         = true;
$wgGroupPermissions['sysop']['reupload-shared']  = true;
$wgGroupPermissions['sysop']['unwatchedpages']   = true;
$wgGroupPermissions['sysop']['autoconfirmed']    = true;
$wgGroupPermissions['sysop']['upload_by_url']    = true;
$wgGroupPermissions['sysop']['ipblock-exempt']   = true;
$wgGroupPermissions['sysop']['blockemail']       = true;
$wgGroupPermissions['sysop']['markbotedits']     = true;
$wgGroupPermissions['sysop']['suppressredirect'] = true;
$wgGroupPermissions['sysop']['apihighlimits']    = true;
$wgGroupPermissions['sysop']['browsearchive']    = true;
$wgGroupPermissions['sysop']['noratelimit']      = true;
#$wgGroupPermissions['sysop']['mergehistory']     = true;

// Permission to change users' group assignments
$wgGroupPermissions['bureaucrat']['userrights']  = true;
$wgGroupPermissions['bureaucrat']['noratelimit'] = true;
// Permission to change users' groups assignments across wikis
#$wgGroupPermissions['bureaucrat']['userrights-interwiki'] = true;

#$wgGroupPermissions['sysop']['deleterevision']  = true;
// To hide usernames from users and Sysops
#$wgGroupPermissions['suppress']['hideuser'] = true;
// To hide revisions/log items from users and Sysops
#$wgGroupPermissions['suppress']['suppressrevision'] = true;
// For private suppression log access
#$wgGroupPermissions['suppress']['suppressionlog'] = true;

/**
 * The developer group is deprecated, but can be activated if need be
 * to use the 'lockdb' and 'unlockdb' special pages. Those require
 * that a lock file be defined and creatable/removable by the web
 * server.
 */
# $wgGroupPermissions['developer']['siteadmin'] = true;


/**
 * Implicit groups, aren't shown on Special:Listusers or somewhere else
 */
$wgImplicitGroups = array( '*', 'user', 'autoconfirmed' );

/**
 * These are the groups that users are allowed to add to or remove from
 * their own account via Special:Userrights.
 */
$wgGroupsAddToSelf = array();
$wgGroupsRemoveFromSelf = array();

/**
 * Set of available actions that can be restricted via action=protect
 * You probably shouldn't change this.
 * Translated trough restriction-* messages.
 */
$wgRestrictionTypes = array( 'edit', 'move' );

/**
 * Rights which can be required for each protection level (via action=protect)
 *
 * You can add a new protection level that requires a specific
 * permission by manipulating this array. The ordering of elements
 * dictates the order on the protection form's lists.
 *
 * '' will be ignored (i.e. unprotected)
 * 'sysop' is quietly rewritten to 'protect' for backwards compatibility
 */
$wgRestrictionLevels = array( '', 'autoconfirmed', 'sysop' );

/**
 * Set the minimum permissions required to edit pages in each
 * namespace.  If you list more than one permission, a user must
 * have all of them to edit pages in that namespace.
 */
$wgNamespaceProtection = array();
$wgNamespaceProtection[ NS_MEDIAWIKI ] = array( 'editinterface' );

/**
 * Pages in namespaces in this array can not be used as templates.
 * Elements must be numeric namespace ids.
 * Among other things, this may be useful to enforce read-restrictions
 * which may otherwise be bypassed by using the template machanism.
 */
$wgNonincludableNamespaces = array();

/**
 * Number of seconds an account is required to age before
 * it's given the implicit 'autoconfirm' group membership.
 * This can be used to limit privileges of new accounts.
 *
 * Accounts created by earlier versions of the software
 * may not have a recorded creation date, and will always
 * be considered to pass the age test.
 *
 * When left at 0, all registered accounts will pass.
 */
$wgAutoConfirmAge = 0;
//$wgAutoConfirmAge = 600;     // ten minutes
//$wgAutoConfirmAge = 3600*24; // one day

# Number of edits an account requires before it is autoconfirmed
# Passing both this AND the time requirement is needed
$wgAutoConfirmCount = 0;
//$wgAutoConfirmCount = 50;

/**
 * Automatically add a usergroup to any user who matches certain conditions.
 * The format is
 *   array( '&' or '|' or '^', cond1, cond2, ... )
 * where cond1, cond2, ... are themselves conditions; *OR*
 *   APCOND_EMAILCONFIRMED, *OR*
 *   array( APCOND_EMAILCONFIRMED ), *OR*
 *   array( APCOND_EDITCOUNT, number of edits ), *OR*
 *   array( APCOND_AGE, seconds since registration ), *OR*
 *   similar constructs defined by extensions.
 *
 * If $wgEmailAuthentication is off, APCOND_EMAILCONFIRMED will be true for any
 * user who has provided an e-mail address.
 */
$wgAutopromote = array(
	'autoconfirmed' => array( '&',
		array( APCOND_EDITCOUNT, &$wgAutoConfirmCount ),
		array( APCOND_AGE, &$wgAutoConfirmAge ),
	),
);

/**
 * These settings can be used to give finer control over who can assign which
 * groups at Special:Userrights.  Example configuration:
 *
 * // Bureaucrat can add any group
 * $wgAddGroups['bureaucrat'] = true;
 * // Bureaucrats can only remove bots and sysops
 * $wgRemoveGroups['bureaucrat'] = array( 'bot', 'sysop' );
 * // Sysops can make bots
 * $wgAddGroups['sysop'] = array( 'bot' );
 * // Sysops can disable other sysops in an emergency, and disable bots
 * $wgRemoveGroups['sysop'] = array( 'sysop', 'bot' );
 */
$wgAddGroups = $wgRemoveGroups = array();


/**
 * A list of available rights, in addition to the ones defined by the core.
 * For extensions only.
 */
$wgAvailableRights = array();

/**
 * Optional to restrict deletion of pages with higher revision counts
 * to users with the 'bigdelete' permission. (Default given to sysops.)
 */
$wgDeleteRevisionsLimit = 0;

/**
 * Used to figure out if a user is "active" or not. User::isActiveEditor()
 * sees if a user has made at least $wgActiveUserEditCount number of edits
 * within the last $wgActiveUserDays days.
 */
$wgActiveUserEditCount = 30;
$wgActiveUserDays = 30;

# Proxy scanner settings
#

/**
 * If you enable this, every editor's IP address will be scanned for open HTTP
 * proxies.
 *
 * Don't enable this. Many sysops will report "hostile TCP port scans" to your
 * ISP and ask for your server to be shut down.
 *
 * You have been warned.
 */
$wgBlockOpenProxies = false;
/** Port we want to scan for a proxy */
$wgProxyPorts = array( 80, 81, 1080, 3128, 6588, 8000, 8080, 8888, 65506 );
/** Script used to scan */
$wgProxyScriptPath = "$IP/includes/proxy_check.php";
/** */
$wgProxyMemcExpiry = 86400;
/** This should always be customised in LocalSettings.php */
$wgSecretKey = false;
/** big list of banned IP addresses, in the keys not the values */
$wgProxyList = array();
/** deprecated */
$wgProxyKey = false;

/** Number of accounts each IP address may create, 0 to disable.
 * Requires memcached */
$wgAccountCreationThrottle = 0;

# Client-side caching:

/** Allow client-side caching of pages */
$wgCachePages       = true;

/**
 * Set this to current time to invalidate all prior cached pages. Affects both
 * client- and server-side caching.
 * You can get the current date on your server by using the command:
 *   date +%Y%m%d%H%M%S
 */
$wgCacheEpoch = '20030516000000';

/**
 * Bump this number when changing the global style sheets and JavaScript.
 * It should be appended in the query string of static CSS and JS includes,
 * to ensure that client-side caches don't keep obsolete copies of global
 * styles.
 */
$wgStyleVersion = '164';


# Server-side caching:

/**
 * This will cache static pages for non-logged-in users to reduce
 * database traffic on public sites.
 * Must set $wgShowIPinHeader = false
 */
$wgUseFileCache = false;

/** Directory where the cached page will be saved */
$wgFileCacheDirectory = false; ///< defaults to "{$wgUploadDirectory}/cache";

/**
 * When using the file cache, we can store the cached HTML gzipped to save disk
 * space. Pages will then also be served compressed to clients that support it.
 * THIS IS NOT COMPATIBLE with ob_gzhandler which is now enabled if supported in
 * the default LocalSettings.php! If you enable this, remove that setting first.
 *
 * Requires zlib support enabled in PHP.
 */
$wgUseGzip = false;

/** Whether MediaWiki should send an ETag header */
$wgUseETag = false;

# Email notification settings
#

/** For email notification on page changes */
$wgPasswordSender = $wgEmergencyContact;

# true: from page editor if s/he opted-in
# false: Enotif mails appear to come from $wgEmergencyContact
$wgEnotifFromEditor	= false;

// TODO move UPO to preferences probably ?
# If set to true, users get a corresponding option in their preferences and can choose to enable or disable at their discretion
# If set to false, the corresponding input form on the user preference page is suppressed
# It call this to be a "user-preferences-option (UPO)"
$wgEmailAuthentication				= true; # UPO (if this is set to false, texts referring to authentication are suppressed)
$wgEnotifWatchlist		= false; # UPO
$wgEnotifUserTalk		= false;	# UPO
$wgEnotifRevealEditorAddress	= false;	# UPO; reply-to address may be filled with page editor's address (if user allowed this in the preferences)
$wgEnotifMinorEdits		= true;	# UPO; false: "minor edits" on pages do not trigger notification mails.
#							# Attention: _every_ change on a user_talk page trigger a notification mail (if the user is not yet notified)

# Send a generic mail instead of a personalised mail for each user.  This
# always uses UTC as the time zone, and doesn't include the username.
#
# For pages with many users watching, this can significantly reduce mail load.
# Has no effect when using sendmail rather than SMTP;

$wgEnotifImpersonal = false;

# Maximum number of users to mail at once when using impersonal mail.  Should
# match the limit on your mail server.
$wgEnotifMaxRecips = 500;

# Send mails via the job queue.
$wgEnotifUseJobQ = false;

/**
 * Array of usernames who will be sent a notification email for every change which occurs on a wiki
 */
$wgUsersNotifiedOnAllChanges = array();

/** Show watching users in recent changes, watchlist and page history views */
$wgRCShowWatchingUsers 				= false; # UPO
/** Show watching users in Page views */
$wgPageShowWatchingUsers 			= false;
/** Show the amount of changed characters in recent changes */
$wgRCShowChangedSize				= true;

/**
 * If the difference between the character counts of the text
 * before and after the edit is below that value, the value will be
 * highlighted on the RC page.
 */
$wgRCChangedSizeThreshold			= -500;

/**
 * Show "Updated (since my last visit)" marker in RC view, watchlist and history
 * view for watched pages with new changes */
$wgShowUpdatedMarker 				= true;

$wgCookieExpiration = 2592000;

/** Clock skew or the one-second resolution of time() can occasionally cause cache
 * problems when the user requests two pages within a short period of time. This
 * variable adds a given number of seconds to vulnerable timestamps, thereby giving
 * a grace period.
 */
$wgClockSkewFudge = 5;

# Squid-related settings
#

/** Enable/disable Squid */
$wgUseSquid = false;

/** If you run Squid3 with ESI support, enable this (default:false): */
$wgUseESI = false;

/** Internal server name as known to Squid, if different */
# $wgInternalServer = 'http://yourinternal.tld:8000';
$wgInternalServer = $wgServer;

/**
 * Cache timeout for the squid, will be sent as s-maxage (without ESI) or
 * Surrogate-Control (with ESI). Without ESI, you should strip out s-maxage in
 * the Squid config. 18000 seconds = 5 hours, more cache hits with 2678400 = 31
 * days
 */
$wgSquidMaxage = 18000;

/**
 * Default maximum age for raw CSS/JS accesses
 */
$wgForcedRawSMaxage = 300;

/**
 * List of proxy servers to purge on changes; default port is 80. Use IP addresses.
 *
 * When MediaWiki is running behind a proxy, it will trust X-Forwarded-For
 * headers sent/modified from these proxies when obtaining the remote IP address
 *
 * For a list of trusted servers which *aren't* purged, see $wgSquidServersNoPurge.
 */
$wgSquidServers = array();

/**
 * As above, except these servers aren't purged on page changes; use to set a
 * list of trusted proxies, etc.
 */
$wgSquidServersNoPurge = array();

/** Maximum number of titles to purge in any one client operation */
$wgMaxSquidPurgeTitles = 400;

/** HTCP multicast purging */
$wgHTCPPort = 4827;
$wgHTCPMulticastTTL = 1;
# $wgHTCPMulticastAddress = "224.0.0.85";
$wgHTCPMulticastAddress = false;

# Cookie settings:
#
/**
 * Set to set an explicit domain on the login cookies eg, "justthis.domain. org"
 * or ".any.subdomain.net"
 */
$wgCookieDomain = '';
$wgCookiePath = '/';
$wgCookieSecure = ($wgProto == 'https');
$wgDisableCookieCheck = false;

/**
 * Set $wgCookiePrefix to use a custom one. Setting to false sets the default of
 * using the database name.
 */
$wgCookiePrefix = false;

/**
 * Set authentication cookies to HttpOnly to prevent access by JavaScript,
 * in browsers that support this feature. This can mitigates some classes of
 * XSS attack.
 *
 * Only supported on PHP 5.2 or higher.
 */
$wgCookieHttpOnly = version_compare("5.2", PHP_VERSION, "<");

/**
 * If the requesting browser matches a regex in this blacklist, we won't
 * send it cookies with HttpOnly mode, even if $wgCookieHttpOnly is on.
 */
$wgHttpOnlyBlacklist = array(
	// Internet Explorer for Mac; sometimes the cookies work, sometimes
	// they don't. It's difficult to predict, as combinations of path
	// and expiration options affect its parsing.
	'/^Mozilla\/4\.0 \(compatible; MSIE \d+\.\d+; Mac_PowerPC\)/',
);

/** A list of cookies that vary the cache (for use by extensions) */
$wgCacheVaryCookies = array();

/** Override to customise the session name */
$wgSessionName = false;

/**  Whether to allow inline image pointing to other websites */
$wgAllowExternalImages = false;

/** If the above is false, you can specify an exception here. Image URLs
  * that start with this string are then rendered, while all others are not.
  * You can use this to set up a trusted, simple repository of images.
  *
  * Example:
  * $wgAllowExternalImagesFrom = 'http://127.0.0.1/';
  */
$wgAllowExternalImagesFrom = '';

/** Allows to move images and other media files. Experemintal, not sure if it always works */
$wgAllowImageMoving = false;

/** Disable database-intensive features */
$wgMiserMode = false;
/** Disable all query pages if miser mode is on, not just some */
$wgDisableQueryPages = false;
/** Number of rows to cache in 'querycache' table when miser mode is on */
$wgQueryCacheLimit = 1000;
/** Number of links to a page required before it is deemed "wanted" */
$wgWantedPagesThreshold = 1;
/** Enable slow parser functions */
$wgAllowSlowParserFunctions = false;

/**
 * Maps jobs to their handling classes; extensions
 * can add to this to provide custom jobs
 */
$wgJobClasses = array(
	'refreshLinks' => 'RefreshLinksJob',
	'htmlCacheUpdate' => 'HTMLCacheUpdateJob',
	'html_cache_update' => 'HTMLCacheUpdateJob', // backwards-compatible
	'sendMail' => 'EmaillingJob',
	'enotifNotify' => 'EnotifNotifyJob',
	'fixDoubleRedirect' => 'DoubleRedirectJob',
);

/**
 * To use inline TeX, you need to compile 'texvc' (in the 'math' subdirectory of
 * the MediaWiki package and have latex, dvips, gs (ghostscript), andconvert
 * (ImageMagick) installed and available in the PATH.
 * Please see math/README for more information.
 */
$wgUseTeX = false;
/** Location of the texvc binary */
$wgTexvc = './math/texvc';

#
# Profiling / debugging
#
# You have to create a 'profiling' table in your database before using
# profiling see maintenance/archives/patch-profiling.sql .
#
# To enable profiling, edit StartProfiler.php

/** Only record profiling info for pages that took longer than this */
$wgProfileLimit = 0.0;
/** Don't put non-profiling info into log file */
$wgProfileOnly = false;
/** Log sums from profiling into "profiling" table in db. */
$wgProfileToDatabase = false;
/** If true, print a raw call tree instead of per-function report */
$wgProfileCallTree = false;
/** Should application server host be put into profiling table */
$wgProfilePerHost = false;

/** Settings for UDP profiler */
$wgUDPProfilerHost = '127.0.0.1';
$wgUDPProfilerPort = '3811';

/** Detects non-matching wfProfileIn/wfProfileOut calls */
$wgDebugProfiling = false;
/** Output debug message on every wfProfileIn/wfProfileOut */
$wgDebugFunctionEntry = 0;
/** Lots of debugging output from SquidUpdate.php */
$wgDebugSquid = false;

/*
 * Destination for wfIncrStats() data...
 * 'cache' to go into the system cache, if enabled (memcached)
 * 'udp' to be sent to the UDP profiler (see $wgUDPProfilerHost)
 * false to disable
 */
$wgStatsMethod = 'cache';

/** Whereas to count the number of time an article is viewed.
 * Does not work if pages are cached (for example with squid).
 */
$wgDisableCounters = false;

$wgDisableTextSearch = false;
$wgDisableSearchContext = false;


/**
 * Set to true to have nicer highligted text in search results,
 * by default off due to execution overhead
 */
$wgAdvancedSearchHighlighting = false;

/**
 * Regexp to match word boundaries, defaults for non-CJK languages
 * should be empty for CJK since the words are not separate
 */
$wgSearchHighlightBoundaries = version_compare("5.1", PHP_VERSION, "<")? '[\p{Z}\p{P}\p{C}]'
	: '[ ,.;:!?~!@#$%\^&*\(\)+=\-\\|\[\]"\'<>\n\r\/{}]'; // PHP 5.0 workaround

/**
 * Template for OpenSearch suggestions, defaults to API action=opensearch
 *
 * Sites with heavy load would tipically have these point to a custom
 * PHP wrapper to avoid firing up mediawiki for every keystroke
 *
 * Placeholders: {searchTerms}
 *
 */
$wgOpenSearchTemplate = false;

/**
 * Enable suggestions while typing in search boxes
 * (results are passed around in OpenSearch format)
 */
$wgEnableMWSuggest = false;

/**
 *  Template for internal MediaWiki suggestion engine, defaults to API action=opensearch
 *
 *  Placeholders: {searchTerms}, {namespaces}, {dbname}
 *
 */
$wgMWSuggestTemplate = false;

/**
 * If you've disabled search semi-permanently, this also disables updates to the
 * table. If you ever re-enable, be sure to rebuild the search table.
 */
$wgDisableSearchUpdate = false;
/** Uploads have to be specially set up to be secure */
$wgEnableUploads = false;
/**
 * Show EXIF data, on by default if available.
 * Requires PHP's EXIF extension: http://www.php.net/manual/en/ref.exif.php
 *
 * NOTE FOR WINDOWS USERS:
 * To enable EXIF functions, add the folloing lines to the
 * "Windows extensions" section of php.ini:
 *
 * extension=extensions/php_mbstring.dll
 * extension=extensions/php_exif.dll
 */
$wgShowEXIF = function_exists( 'exif_read_data' );

/**
 * Set to true to enable the upload _link_ while local uploads are disabled.
 * Assumes that the special page link will be bounced to another server where
 * uploads do work.
 */
$wgRemoteUploads = false;
$wgDisableAnonTalk = false;
/**
 * Do DELETE/INSERT for link updates instead of incremental
 */
$wgUseDumbLinkUpdate = false;

/**
 * Anti-lock flags - bitfield
 *   ALF_PRELOAD_LINKS
 *       Preload links during link update for save
 *   ALF_PRELOAD_EXISTENCE
 *       Preload cur_id during replaceLinkHolders
 *   ALF_NO_LINK_LOCK
 *       Don't use locking reads when updating the link table. This is
 *       necessary for wikis with a high edit rate for performance
 *       reasons, but may cause link table inconsistency
 *   ALF_NO_BLOCK_LOCK
 *       As for ALF_LINK_LOCK, this flag is a necessity for high-traffic
 *       wikis.
 */
$wgAntiLockFlags = 0;

/**
 * Path to the GNU diff3 utility. If the file doesn't exist, edit conflicts will
 * fall back to the old behaviour (no merging).
 */
$wgDiff3 = '/usr/bin/diff3';

/**
 * Path to the GNU diff utility.
 */
$wgDiff = '/usr/bin/diff';

/**
 * We can also compress text stored in the 'text' table. If this is set on, new
 * revisions will be compressed on page save if zlib support is available. Any
 * compressed revisions will be decompressed on load regardless of this setting
 * *but will not be readable at all* if zlib support is not available.
 */
$wgCompressRevisions = false;

/**
 * This is the list of preferred extensions for uploading files. Uploading files
 * with extensions not in this list will trigger a warning.
 */
$wgFileExtensions = array( 'png', 'gif', 'jpg', 'jpeg' );

/** Files with these extensions will never be allowed as uploads. */
$wgFileBlacklist = array(
	# HTML may contain cookie-stealing JavaScript and web bugs
	'html', 'htm', 'js', 'jsb', 'mhtml', 'mht',
	# PHP scripts may execute arbitrary code on the server
	'php', 'phtml', 'php3', 'php4', 'php5', 'phps',
	# Other types that may be interpreted by some servers
	'shtml', 'jhtml', 'pl', 'py', 'cgi',
	# May contain harmful executables for Windows victims
	'exe', 'scr', 'dll', 'msi', 'vbs', 'bat', 'com', 'pif', 'cmd', 'vxd', 'cpl' );

/** Files with these mime types will never be allowed as uploads
 * if $wgVerifyMimeType is enabled.
 */
$wgMimeTypeBlacklist= array(
	# HTML may contain cookie-stealing JavaScript and web bugs
	'text/html', 'text/javascript', 'text/x-javascript',  'application/x-shellscript',
	# PHP scripts may execute arbitrary code on the server
	'application/x-php', 'text/x-php',
	# Other types that may be interpreted by some servers
	'text/x-python', 'text/x-perl', 'text/x-bash', 'text/x-sh', 'text/x-csh',
	# Windows metafile, client-side vulnerability on some systems
	'application/x-msmetafile'
);

/** This is a flag to determine whether or not to check file extensions on upload. */
$wgCheckFileExtensions = true;

/**
 * If this is turned off, users may override the warning for files not covered
 * by $wgFileExtensions.
 */
$wgStrictFileExtensions = true;

/** Warn if uploaded files are larger than this (in bytes), or false to disable*/
$wgUploadSizeWarning = false;

/** For compatibility with old installations set to false */
$wgPasswordSalt = true;

/** Which namespaces should support subpages?
 * See Language.php for a list of namespaces.
 */
$wgNamespacesWithSubpages = array(
	NS_TALK           => true,
	NS_USER           => true,
	NS_USER_TALK      => true,
	NS_PROJECT_TALK   => true,
	NS_IMAGE_TALK     => true,
	NS_MEDIAWIKI_TALK => true,
	NS_TEMPLATE_TALK  => true,
	NS_HELP_TALK      => true,
	NS_CATEGORY_TALK  => true
);

$wgNamespacesToBeSearchedDefault = array(
	NS_MAIN           => true,
);

/**
 * Site notice shown at the top of each page
 *
 * This message can contain wiki text, and can also be set through the
 * MediaWiki:Sitenotice page. You can also provide a separate message for
 * logged-out users using the MediaWiki:Anonnotice page.
 */
$wgSiteNotice = '';

#
# Images settings
#

/**
 * Plugins for media file type handling.
 * Each entry in the array maps a MIME type to a class name
 */
$wgMediaHandlers = array(
	'image/jpeg' => 'BitmapHandler',
	'image/png' => 'BitmapHandler',
	'image/gif' => 'BitmapHandler',
	'image/x-ms-bmp' => 'BmpHandler',
	'image/x-bmp' => 'BmpHandler',
	'image/svg+xml' => 'SvgHandler', // official
	'image/svg' => 'SvgHandler', // compat
	'image/vnd.djvu' => 'DjVuHandler', // official
	'image/x.djvu' => 'DjVuHandler', // compat
	'image/x-djvu' => 'DjVuHandler', // compat
);


/**
 * Resizing can be done using PHP's internal image libraries or using
 * ImageMagick or another third-party converter, e.g. GraphicMagick.
 * These support more file formats than PHP, which only supports PNG,
 * GIF, JPG, XBM and WBMP.
 *
 * Use Image Magick instead of PHP builtin functions.
 */
$wgUseImageMagick		= false;
/** The convert command shipped with ImageMagick */
$wgImageMagickConvertCommand    = '/usr/bin/convert';

/** Sharpening parameter to ImageMagick */
$wgSharpenParameter = '0x0.4';

/** Reduction in linear dimensions below which sharpening will be enabled */
$wgSharpenReductionThreshold = 0.85;

/**
 * Use another resizing converter, e.g. GraphicMagick
 * %s will be replaced with the source path, %d with the destination
 * %w and %h will be replaced with the width and height
 *
 * An example is provided for GraphicMagick
 * Leave as false to skip this
 */
#$wgCustomConvertCommand = "gm convert %s -resize %wx%h %d"
$wgCustomConvertCommand = false;

# Scalable Vector Graphics (SVG) may be uploaded as images.
# Since SVG support is not yet standard in browsers, it is
# necessary to rasterize SVGs to PNG as a fallback format.
#
# An external program is required to perform this conversion:
$wgSVGConverters = array(
	'ImageMagick' => '$path/convert -background white -geometry $width $input PNG:$output',
	'sodipodi' => '$path/sodipodi -z -w $width -f $input -e $output',
	'inkscape' => '$path/inkscape -z -w $width -f $input -e $output',
	'batik' => 'java -Djava.awt.headless=true -jar $path/batik-rasterizer.jar -w $width -d $output $input',
	'rsvg' => '$path/rsvg -w$width -h$height $input $output',
	'imgserv' => '$path/imgserv-wrapper -i svg -o png -w$width $input $output',
	);
/** Pick one of the above */
$wgSVGConverter = 'ImageMagick';
/** If not in the executable PATH, specify */
$wgSVGConverterPath = '';
/** Don't scale a SVG larger than this */
$wgSVGMaxSize = 2048;
/**
 * Don't thumbnail an image if it will use too much working memory
 * Default is 50 MB if decompressed to RGBA form, which corresponds to
 * 12.5 million pixels or 3500x3500
 */
$wgMaxImageArea = 1.25e7;
/**
 * If rendered thumbnail files are older than this timestamp, they
 * will be rerendered on demand as if the file didn't already exist.
 * Update if there is some need to force thumbs and SVG rasterizations
 * to rerender, such as fixes to rendering bugs.
 */
$wgThumbnailEpoch = '20030516000000';

/**
 * If set, inline scaled images will still produce <img> tags ready for
 * output instead of showing an error message.
 *
 * This may be useful if errors are transitory, especially if the site
 * is configured to automatically render thumbnails on request.
 *
 * On the other hand, it may obscure error conditions from debugging.
 * Enable the debug log or the 'thumbnail' log group to make sure errors
 * are logged to a file for review.
 */
$wgIgnoreImageErrors = false;

/**
 * Allow thumbnail rendering on page view. If this is false, a valid
 * thumbnail URL is still output, but no file will be created at
 * the target location. This may save some time if you have a
 * thumb.php or 404 handler set up which is faster than the regular
 * webserver(s).
 */
$wgGenerateThumbnailOnParse = true;

/** Obsolete, always true, kept for compatibility with extensions */
$wgUseImageResize = true;


/** Set $wgCommandLineMode if it's not set already, to avoid notices */
if( !isset( $wgCommandLineMode ) ) {
	$wgCommandLineMode = false;
}

/** For colorized maintenance script output, is your terminal background dark ? */
$wgCommandLineDarkBg = false;

#
# Recent changes settings
#

/** Log IP addresses in the recentchanges table; can be accessed only by extensions (e.g. CheckUser) or a DB admin */
$wgPutIPinRC = true;

/**
 * Recentchanges items are periodically purged; entries older than this many
 * seconds will go.
 * For one week : 7 * 24 * 3600
 */
$wgRCMaxAge = 7 * 24 * 3600;

/**
 * Filter $wgRCLinkDays by $wgRCMaxAge to avoid showing links for numbers higher than what will be stored.
 * Note that this is disabled by default because we sometimes do have RC data which is beyond the limit
 * for some reason, and some users may use the high numbers to display that data which is still there.
 */
$wgRCFilterByAge = false;

/**
 * List of Days and Limits options to list in the Special:Recentchanges and Special:Recentchangeslinked pages.
 */
$wgRCLinkLimits = array( 50, 100, 250, 500 );
$wgRCLinkDays   = array( 1, 3, 7, 14, 30 );

# Send RC updates via UDP
$wgRC2UDPAddress = false;
$wgRC2UDPPort = false;
$wgRC2UDPPrefix = '';
$wgRC2UDPOmitBots = false;

# Enable user search in Special:Newpages
# This is really a temporary hack around an index install bug on some Wikipedias.
# Kill it once fixed.
$wgEnableNewpagesUserFilter = true;

#
# Copyright and credits settings
#

/** RDF metadata toggles */
$wgEnableDublinCoreRdf = false;
$wgEnableCreativeCommonsRdf = false;

/** Override for copyright metadata.
 * TODO: these options need documentation
 */
$wgRightsPage = NULL;
$wgRightsUrl = NULL;
$wgRightsText = NULL;
$wgRightsIcon = NULL;

/** Set this to some HTML to override the rights icon with an arbitrary logo */
$wgCopyrightIcon = NULL;

/** Set this to true if you want detailed copyright information forms on Upload. */
$wgUseCopyrightUpload = false;

/** Set this to false if you want to disable checking that detailed copyright
 * information values are not empty. */
$wgCheckCopyrightUpload = true;

/**
 * Set this to the number of authors that you want to be credited below an
 * article text. Set it to zero to hide the attribution block, and a negative
 * number (like -1) to show all authors. Note that this will require 2-3 extra
 * database hits, which can have a not insignificant impact on performance for
 * large wikis.
 */
$wgMaxCredits = 0;

/** If there are more than $wgMaxCredits authors, show $wgMaxCredits of them.
 * Otherwise, link to a separate credits page. */
$wgShowCreditsIfMax = true;



/**
 * Set this to false to avoid forcing the first letter of links to capitals.
 * WARNING: may break links! This makes links COMPLETELY case-sensitive. Links
 * appearing with a capital at the beginning of a sentence will *not* go to the
 * same place as links in the middle of a sentence using a lowercase initial.
 */
$wgCapitalLinks = true;

/**
 * List of interwiki prefixes for wikis we'll accept as sources for
 * Special:Import (for sysops). Since complete page history can be imported,
 * these should be 'trusted'.
 *
 * If a user has the 'import' permission but not the 'importupload' permission,
 * they will only be able to run imports through this transwiki interface.
 */
$wgImportSources = array();

/**
 * Optional default target namespace for interwiki imports.
 * Can use this to create an incoming "transwiki"-style queue.
 * Set to numeric key, not the name.
 *
 * Users may override this in the Special:Import dialog.
 */
$wgImportTargetNamespace = null;

/**
 * If set to false, disables the full-history option on Special:Export.
 * This is currently poorly optimized for long edit histories, so is
 * disabled on Wikimedia's sites.
 */
$wgExportAllowHistory = true;

/**
 * If set nonzero, Special:Export requests for history of pages with
 * more revisions than this will be rejected. On some big sites things
 * could get bogged down by very very long pages.
 */
$wgExportMaxHistory = 0;

$wgExportAllowListContributors = false ;


/** Text matching this regular expression will be recognised as spam
 * See http://en.wikipedia.org/wiki/Regular_expression */
$wgSpamRegex = false;
/** Similarly you can get a function to do the job. The function will be given
 * the following args:
 *   - a Title object for the article the edit is made on
 *   - the text submitted in the textarea (wpTextbox1)
 *   - the section number.
 * The return should be boolean indicating whether the edit matched some evilness:
 *  - true : block it
 *  - false : let it through
 *
 * For a complete example, have a look at the SpamBlacklist extension.
 */
$wgFilterCallback = false;

/** Go button goes straight to the edit screen if the article doesn't exist. */
$wgGoToEdit = false;

/** Allow raw, unchecked HTML in <html>...</html> sections.
 * THIS IS VERY DANGEROUS on a publically editable site, so USE wgGroupPermissions
 * TO RESTRICT EDITING to only those that you trust
 */
$wgRawHtml = false;

/**
 * $wgUseTidy: use tidy to make sure HTML output is sane.
 * Tidy is a free tool that fixes broken HTML.
 * See http://www.w3.org/People/Raggett/tidy/
 * $wgTidyBin should be set to the path of the binary and
 * $wgTidyConf to the path of the configuration file.
 * $wgTidyOpts can include any number of parameters.
 *
 * $wgTidyInternal controls the use of the PECL extension to use an in-
 *   process tidy library instead of spawning a separate program.
 *   Normally you shouldn't need to override the setting except for
 *   debugging. To install, use 'pear install tidy' and add a line
 *   'extension=tidy.so' to php.ini.
 */
$wgUseTidy = false;
$wgAlwaysUseTidy = false;
$wgTidyBin = 'tidy';
$wgTidyConf = $IP.'/includes/tidy.conf';
$wgTidyOpts = '';
$wgTidyInternal = extension_loaded( 'tidy' );

/**
 * Put tidy warnings in HTML comments
 * Only works for internal tidy.
 */
$wgDebugTidy = false;

/**
 * Validate the overall output using tidy and refuse
 * to display the page if it's not valid.
 */
$wgValidateAllHtml = false;

/** See list of skins and their symbolic names in languages/Language.php */
$wgDefaultSkin = 'monobook';

/**
 * Settings added to this array will override the default globals for the user
 * preferences used by anonymous visitors and newly created accounts.
 * For instance, to disable section editing links:
 * $wgDefaultUserOptions ['editsection'] = 0;
 *
 */
$wgDefaultUserOptions = array(
	'quickbar'                => 1,
	'underline'               => 2,
	'cols'                    => 80,
	'rows'                    => 25,
	'searchlimit'             => 20,
	'contextlines'            => 5,
	'contextchars'            => 50,
	'disablesuggest'          => 0,
	'ajaxsearch'              => 0,
	'skin'                    => false,
	'math'                    => 1,
	'usenewrc'                => 0,
	'rcdays'                  => 7,
	'rclimit'                 => 50,
	'wllimit'                 => 250,
	'hideminor'               => 0,
	'highlightbroken'         => 1,
	'stubthreshold'           => 0,
	'previewontop'            => 1,
	'previewonfirst'          => 0,
	'editsection'             => 1,
	'editsectiononrightclick' => 0,
	'editondblclick'          => 0,
	'editwidth'               => 0,
	'showtoc'                 => 1,
	'showtoolbar'             => 1,
	'minordefault'            => 0,
	'date'                    => 'default',
	'imagesize'               => 2,
	'thumbsize'               => 2,
	'rememberpassword'        => 0,
	'enotifwatchlistpages'    => 0,
	'enotifusertalkpages'     => 1,
	'enotifminoredits'        => 0,
	'enotifrevealaddr'        => 0,
	'shownumberswatching'     => 1,
	'fancysig'                => 0,
	'externaleditor'          => 0,
	'externaldiff'            => 0,
	'showjumplinks'           => 1,
	'numberheadings'          => 0,
	'uselivepreview'          => 0,
	'watchlistdays'           => 3.0,
	'extendwatchlist'         => 0,
	'watchlisthideminor'      => 0,
	'watchlisthidebots'       => 0,
	'watchlisthideown'        => 0,
	'watchcreations'          => 0,
	'watchdefault'            => 0,
	'watchmoves'              => 0,
	'watchdeletion'           => 0,
);

/** Whether or not to allow and use real name fields. Defaults to true. */
$wgAllowRealName = true;

/*****************************************************************************
 *  Extensions
 */

/**
 * A list of callback functions which are called once MediaWiki is fully initialised
 */
$wgExtensionFunctions = array();

/**
 * Extension functions for initialisation of skins. This is called somewhat earlier
 * than $wgExtensionFunctions.
 */
$wgSkinExtensionFunctions = array();

/**
 * Extension messages files
 * Associative array mapping extension name to the filename where messages can be found.
 * The file must create a variable called $messages.
 * When the messages are needed, the extension should call wfLoadExtensionMessages().
 *
 * Example:
 *    $wgExtensionMessagesFiles['ConfirmEdit'] = dirname(__FILE__).'/ConfirmEdit.i18n.php';
 *
 */
$wgExtensionMessagesFiles = array();

/**
 * Aliases for special pages provided by extensions.
 * Associative array mapping special page to array of aliases. First alternative
 * for each special page will be used as the normalised name for it. English
 * aliases will be added to the end of the list so that they always work. The
 * file must define a variable $aliases.
 *
 * Example:
 *    $wgExtensionAliasesFiles['Translate'] = dirname(__FILE__).'/Translate.alias.php';
 */
$wgExtensionAliasesFiles = array();

/**
 * Parser output hooks.
 * This is an associative array where the key is an extension-defined tag
 * (typically the extension name), and the value is a PHP callback.
 * These will be called as an OutputPageParserOutput hook, if the relevant
 * tag has been registered with the parser output object.
 *
 * Registration is done with $pout->addOutputHook( $tag, $data ).
 *
 * The callback has the form:
 *    function outputHook( $outputPage, $parserOutput, $data ) { ... }
 */
$wgParserOutputHooks = array();

/**
 * List of valid skin names.
 * The key should be the name in all lower case, the value should be a display name.
 * The default skins will be added later, by Skin::getSkinNames(). Use
 * Skin::getSkinNames() as an accessor if you wish to have access to the full list.
 */
$wgValidSkinNames = array();

/**
 * Special page list.
 * See the top of SpecialPage.php for documentation.
 */
$wgSpecialPages = array();

/**
 * Array mapping class names to filenames, for autoloading.
 */
$wgAutoloadClasses = array();

/**
 * An array of extension types and inside that their names, versions, authors,
 * urls, descriptions and pointers to localized description msgs. Note that
 * the version, url, description and descriptionmsg key can be omitted.
 *
 * <code>
 * $wgExtensionCredits[$type][] = array(
 * 	'name' => 'Example extension',
 *  'version' => 1.9,
 *  'svn-revision' => '$LastChangedRevision: 41545 $',
 *	'author' => 'Foo Barstein',
 *	'url' => 'http://wwww.example.com/Example%20Extension/',
 *	'description' => 'An example extension',
 *	'descriptionmsg' => 'exampleextension-desc',
 * );
 * </code>
 *
 * Where $type is 'specialpage', 'parserhook', 'variable', 'media' or 'other'.
 */
$wgExtensionCredits = array();
/*
 * end extensions
 ******************************************************************************/

/**
 * Allow user Javascript page?
 * This enables a lot of neat customizations, but may
 * increase security risk to users and server load.
 */
$wgAllowUserJs = false;

/**
 * Allow user Cascading Style Sheets (CSS)?
 * This enables a lot of neat customizations, but may
 * increase security risk to users and server load.
 */
$wgAllowUserCss = false;

/** Use the site's Javascript page? */
$wgUseSiteJs = true;

/** Use the site's Cascading Style Sheets (CSS)? */
$wgUseSiteCss = true;

/** Filter for Special:Randompage. Part of a WHERE clause */
$wgExtraRandompageSQL = false;

/** Allow the "info" action, very inefficient at the moment */
$wgAllowPageInfo = false;

/** Maximum indent level of toc. */
$wgMaxTocLevel = 999;

/** Name of the external diff engine to use */
$wgExternalDiffEngine = false;

/** Use RC Patrolling to check for vandalism */
$wgUseRCPatrol = true;

/** Use new page patrolling to check new pages on Special:Newpages */
$wgUseNPPatrol = true;

/** Provide syndication feeds (RSS, Atom) for, e.g., Recentchanges, Newpages */
$wgFeed = true;

/** Set maximum number of results to return in syndication feeds (RSS, Atom) for
 * eg Recentchanges, Newpages. */
$wgFeedLimit = 50;

/** _Minimum_ timeout for cached Recentchanges feed, in seconds.
 * A cached version will continue to be served out even if changes
 * are made, until this many seconds runs out since the last render.
 *
 * If set to 0, feed caching is disabled. Use this for debugging only;
 * feed generation can be pretty slow with diffs.
 */
$wgFeedCacheTimeout = 60;

/** When generating Recentchanges RSS/Atom feed, diffs will not be generated for
 * pages larger than this size. */
$wgFeedDiffCutoff = 32768;


/**
 * Additional namespaces. If the namespaces defined in Language.php and
 * Namespace.php are insufficient, you can create new ones here, for example,
 * to import Help files in other languages.
 * PLEASE  NOTE: Once you delete a namespace, the pages in that namespace will
 * no longer be accessible. If you rename it, then you can access them through
 * the new namespace name.
 *
 * Custom namespaces should start at 100 to avoid conflicting with standard
 * namespaces, and should always follow the even/odd main/talk pattern.
 */
#$wgExtraNamespaces =
#	array(100 => "Hilfe",
#	      101 => "Hilfe_Diskussion",
#	      102 => "Aide",
#	      103 => "Discussion_Aide"
#	      );
$wgExtraNamespaces = NULL;

/**
 * Namespace aliases
 * These are alternate names for the primary localised namespace names, which
 * are defined by $wgExtraNamespaces and the language file. If a page is
 * requested with such a prefix, the request will be redirected to the primary
 * name.
 *
 * Set this to a map from namespace names to IDs.
 * Example:
 *    $wgNamespaceAliases = array(
 *        'Wikipedian' => NS_USER,
 *        'Help' => 100,
 *    );
 */
$wgNamespaceAliases = array();

/**
 * Limit images on image description pages to a user-selectable limit. In order
 * to reduce disk usage, limits can only be selected from a list.
 * The user preference is saved as an array offset in the database, by default
 * the offset is set with $wgDefaultUserOptions['imagesize']. Make sure you
 * change it if you alter the array (see bug 8858).
 * This is the list of settings the user can choose from:
 */
$wgImageLimits = array (
	array(320,240),
	array(640,480),
	array(800,600),
	array(1024,768),
	array(1280,1024),
	array(10000,10000) );

/**
 * Adjust thumbnails on image pages according to a user setting. In order to
 * reduce disk usage, the values can only be selected from a list. This is the
 * list of settings the user can choose from:
 */
$wgThumbLimits = array(
	120,
	150,
	180,
	200,
	250,
	300
);

/**
 * Adjust width of upright images when parameter 'upright' is used
 * This allows a nicer look for upright images without the need to fix the width
 * by hardcoded px in wiki sourcecode.
 */
$wgThumbUpright = 0.75;

/**
 *  On  category pages, show thumbnail gallery for images belonging to that
 * category instead of listing them as articles.
 */
$wgCategoryMagicGallery = true;

/**
 * Paging limit for categories
 */
$wgCategoryPagingLimit = 200;

/**
 * Browser Blacklist for unicode non compliant browsers
 * Contains a list of regexps : "/regexp/"  matching problematic browsers
 */
$wgBrowserBlackList = array(
	/**
	 * Netscape 2-4 detection
	 * The minor version may contain strings such as "Gold" or "SGoldC-SGI"
	 * Lots of non-netscape user agents have "compatible", so it's useful to check for that
	 * with a negative assertion. The [UIN] identifier specifies the level of security
	 * in a Netscape/Mozilla browser, checking for it rules out a number of fakers.
	 * The language string is unreliable, it is missing on NS4 Mac.
	 *
	 * Reference: http://www.psychedelix.com/agents/index.shtml
	 */
	'/^Mozilla\/2\.[^ ]+ [^(]*?\((?!compatible).*; [UIN]/',
	'/^Mozilla\/3\.[^ ]+ [^(]*?\((?!compatible).*; [UIN]/',
	'/^Mozilla\/4\.[^ ]+ [^(]*?\((?!compatible).*; [UIN]/',

	/**
	 * MSIE on Mac OS 9 is teh sux0r, converts þ to <thorn>, ð to <eth>, Þ to <THORN> and Ð to <ETH>
	 *
	 * Known useragents:
	 * - Mozilla/4.0 (compatible; MSIE 5.0; Mac_PowerPC)
	 * - Mozilla/4.0 (compatible; MSIE 5.15; Mac_PowerPC)
	 * - Mozilla/4.0 (compatible; MSIE 5.23; Mac_PowerPC)
	 * - [...]
	 *
	 * @link http://en.wikipedia.org/w/index.php?title=User%3A%C6var_Arnfj%F6r%F0_Bjarmason%2Ftestme&diff=12356041&oldid=12355864
	 * @link http://en.wikipedia.org/wiki/Template%3AOS9
	 */
	'/^Mozilla\/4\.0 \(compatible; MSIE \d+\.\d+; Mac_PowerPC\)/',

	/**
	 * Google wireless transcoder, seems to eat a lot of chars alive
	 * http://it.wikipedia.org/w/index.php?title=Luciano_Ligabue&diff=prev&oldid=8857361
	 */
	'/^Mozilla\/4\.0 \(compatible; MSIE 6.0; Windows NT 5.0; Google Wireless Transcoder;\)/'
);

/**
 * Fake out the timezone that the server thinks it's in. This will be used for
 * date display and not for what's stored in the DB. Leave to null to retain
 * your server's OS-based timezone value. This is the same as the timezone.
 *
 * This variable is currently used ONLY for signature formatting, not for
 * anything else.
 */
# $wgLocaltimezone = 'GMT';
# $wgLocaltimezone = 'PST8PDT';
# $wgLocaltimezone = 'Europe/Sweden';
# $wgLocaltimezone = 'CET';
$wgLocaltimezone = null;

/**
 * Set an offset from UTC in minutes to use for the default timezone setting
 * for anonymous users and new user accounts.
 *
 * This setting is used for most date/time displays in the software, and is
 * overrideable in user preferences. It is *not* used for signature timestamps.
 *
 * You can set it to match the configured server timezone like this:
 *   $wgLocalTZoffset = date("Z") / 60;
 *
 * If your server is not configured for the timezone you want, you can set
 * this in conjunction with the signature timezone and override the TZ
 * environment variable like so:
 *   $wgLocaltimezone="Europe/Berlin";
 *   putenv("TZ=$wgLocaltimezone");
 *   $wgLocalTZoffset = date("Z") / 60;
 *
 * Leave at NULL to show times in universal time (UTC/GMT).
 */
$wgLocalTZoffset = null;


/**
 * When translating messages with wfMsg(), it is not always clear what should be
 * considered UI messages and what shoud be content messages.
 *
 * For example, for regular wikipedia site like en, there should be only one
 * 'mainpage', therefore when getting the link of 'mainpage', we should treate
 * it as content of the site and call wfMsgForContent(), while for rendering the
 * text of the link, we call wfMsg(). The code in default behaves this way.
 * However, sites like common do offer different versions of 'mainpage' and the
 * like for different languages. This array provides a way to override the
 * default behavior. For example, to allow language specific mainpage and
 * community portal, set
 *
 * $wgForceUIMsgAsContentMsg = array( 'mainpage', 'portal-url' );
 */
$wgForceUIMsgAsContentMsg = array();


/**
 * Authentication plugin.
 */
$wgAuth = null;

/**
 * Global list of hooks.
 * Add a hook by doing:
 *     $wgHooks['event_name'][] = $function;
 * or:
 *     $wgHooks['event_name'][] = array($function, $data);
 * or:
 *     $wgHooks['event_name'][] = array($object, 'method');
 */
$wgHooks = array();

/**
 * The logging system has two levels: an event type, which describes the
 * general category and can be viewed as a named subset of all logs; and
 * an action, which is a specific kind of event that can exist in that
 * log type.
 */
$wgLogTypes = array( '',
	'block',
	'protect',
	'rights',
	'delete',
	'upload',
	'move',
	'import',
	'patrol',
	'merge',
	'suppress',
);

/**
 * This restricts log access to those who have a certain right
 * Users without this will not see it in the option menu and can not view it
 * Restricted logs are not added to recent changes
 * Logs should remain non-transcludable
 */
$wgLogRestrictions = array(
	'suppress' => 'suppressionlog'
);

/**
 * Lists the message key string for each log type. The localized messages
 * will be listed in the user interface.
 *
 * Extensions with custom log types may add to this array.
 */
$wgLogNames = array(
	''        => 'all-logs-page',
	'block'   => 'blocklogpage',
	'protect' => 'protectlogpage',
	'rights'  => 'rightslog',
	'delete'  => 'dellogpage',
	'upload'  => 'uploadlogpage',
	'move'    => 'movelogpage',
	'import'  => 'importlogpage',
	'patrol'  => 'patrol-log-page',
	'merge'   => 'mergelog',
	'suppress' => 'suppressionlog',
);

/**
 * Lists the message key string for descriptive text to be shown at the
 * top of each log type.
 *
 * Extensions with custom log types may add to this array.
 */
$wgLogHeaders = array(
	''        => 'alllogstext',
	'block'   => 'blocklogtext',
	'protect' => 'protectlogtext',
	'rights'  => 'rightslogtext',
	'delete'  => 'dellogpagetext',
	'upload'  => 'uploadlogpagetext',
	'move'    => 'movelogpagetext',
	'import'  => 'importlogpagetext',
	'patrol'  => 'patrol-log-header',
	'merge'   => 'mergelogpagetext',
	'suppress' => 'suppressionlogtext',
);

/**
 * Lists the message key string for formatting individual events of each
 * type and action when listed in the logs.
 *
 * Extensions with custom log types may add to this array.
 */
$wgLogActions = array(
	'block/block'       => 'blocklogentry',
	'block/unblock'     => 'unblocklogentry',
	'protect/protect'   => 'protectedarticle',
	'protect/modify'    => 'modifiedarticleprotection',
	'protect/unprotect' => 'unprotectedarticle',
	'rights/rights'     => 'rightslogentry',
	'delete/delete'     => 'deletedarticle',
	'delete/restore'    => 'undeletedarticle',
	'delete/revision'   => 'revdelete-logentry',
	'delete/event'      => 'logdelete-logentry',
	'upload/upload'     => 'uploadedimage',
	'upload/overwrite'  => 'overwroteimage',
	'upload/revert'     => 'uploadedimage',
	'move/move'         => '1movedto2',
	'move/move_redir'   => '1movedto2_redir',
	'import/upload'     => 'import-logentry-upload',
	'import/interwiki'  => 'import-logentry-interwiki',
	'merge/merge'       => 'pagemerge-logentry',
	'suppress/revision' => 'revdelete-logentry',
	'suppress/file'     => 'revdelete-logentry',
	'suppress/event'    => 'logdelete-logentry',
	'suppress/delete'   => 'suppressedarticle',
	'suppress/block'	=> 'blocklogentry',
);

/**
 * The same as above, but here values are names of functions,
 * not messages
 */
$wgLogActionsHandlers = array();

/**
 * List of special pages, followed by what subtitle they should go under
 * at Special:SpecialPages
 */
$wgSpecialPageGroups = array(
	'DoubleRedirects'           => 'maintenance',
	'BrokenRedirects'           => 'maintenance',
	'Lonelypages'               => 'maintenance',
	'Uncategorizedpages'        => 'maintenance',
	'Uncategorizedcategories'   => 'maintenance',
	'Uncategorizedimages'       => 'maintenance',
	'Uncategorizedtemplates'    => 'maintenance',
	'Unusedcategories'          => 'maintenance',
	'Unusedimages'              => 'maintenance',
	'Protectedpages'            => 'maintenance',
	'Protectedtitles'           => 'maintenance',
	'Unusedtemplates'           => 'maintenance',
	'Withoutinterwiki'          => 'maintenance',
	'Longpages'                 => 'maintenance',
	'Shortpages'                => 'maintenance',
	'Ancientpages'              => 'maintenance',
	'Deadendpages'              => 'maintenance',
	'Wantedpages'               => 'maintenance',
	'Wantedcategories'          => 'maintenance',
	'Unwatchedpages'            => 'maintenance',
	'Fewestrevisions'           => 'maintenance',

	'Userlogin'                 => 'login',
	'Userlogout'                => 'login',
	'CreateAccount'             => 'login',

	'Recentchanges'             => 'changes',
	'Recentchangeslinked'       => 'changes',
	'Watchlist'                 => 'changes',
	'Newimages'                 => 'changes',
	'Newpages'                  => 'changes',
	'Log'                       => 'changes',

	'Upload'                    => 'media',
	'Imagelist'                 => 'media',
	'MIMEsearch'                => 'media',
	'FileDuplicateSearch'       => 'media',
	'Filepath'                  => 'media',

	'Listusers'                 => 'users',
	'Listgrouprights'           => 'users',
	'Ipblocklist'               => 'users',
	'Contributions'             => 'users',
	'Emailuser'                 => 'users',
	'Listadmins'                => 'users',
	'Listbots'                  => 'users',
	'Userrights'                => 'users',
	'Blockip'                   => 'users',
	'Preferences'               => 'users',
	'Resetpass'                 => 'users',

	'Mostlinked'                => 'highuse',
	'Mostlinkedcategories'      => 'highuse',
	'Mostlinkedtemplates'       => 'highuse',
	'Mostcategories'            => 'highuse',
	'Mostimages'                => 'highuse',
	'Mostrevisions'             => 'highuse',

	'Allpages'                  => 'pages',
	'Prefixindex'               => 'pages',
	'Listredirects'             => 'pages',
	'Categories'                => 'pages',
	'Disambiguations'           => 'pages',

	'Randompage'                => 'redirects',
	'Randomredirect'            => 'redirects',
	'Mypage'                    => 'redirects',
	'Mytalk'                    => 'redirects',
	'Mycontributions'           => 'redirects',
	'Search'                    => 'redirects',

	'Movepage'                  => 'pagetools',
	'MergeHistory'              => 'pagetools',
	'Revisiondelete'            => 'pagetools',
	'Undelete'                  => 'pagetools',
	'Export'                    => 'pagetools',
	'Import'                    => 'pagetools',
	'Whatlinkshere'             => 'pagetools',

	'Statistics'                => 'wiki',
	'Version'                   => 'wiki',
	'Lockdb'                    => 'wiki',
	'Unlockdb'                  => 'wiki',
	'Allmessages'               => 'wiki',
	'Popularpages'              => 'wiki',

	'Specialpages'              => 'other',
	'Blockme'                   => 'other',
	'Booksources'               => 'other',
);

/**
 * Experimental preview feature to fetch rendered text
 * over an XMLHttpRequest from JavaScript instead of
 * forcing a submit and reload of the whole page.
 * Leave disabled unless you're testing it.
 */
$wgLivePreview = false;

/**
 * Disable the internal MySQL-based search, to allow it to be
 * implemented by an extension instead.
 */
$wgDisableInternalSearch = false;

/**
 * Set this to a URL to forward search requests to some external location.
 * If the URL includes '$1', this will be replaced with the URL-encoded
 * search term.
 *
 * For example, to forward to Google you'd have something like:
 * $wgSearchForwardUrl = 'http://www.google.com/search?q=$1' .
 *                       '&domains=http://example.com' .
 *                       '&sitesearch=http://example.com' .
 *                       '&ie=utf-8&oe=utf-8';
 */
$wgSearchForwardUrl = null;

/**
 * If true, external URL links in wiki text will be given the
 * rel="nofollow" attribute as a hint to search engines that
 * they should not be followed for ranking purposes as they
 * are user-supplied and thus subject to spamming.
 */
$wgNoFollowLinks = true;

/**
 * Namespaces in which $wgNoFollowLinks doesn't apply.
 * See Language.php for a list of namespaces.
 */
$wgNoFollowNsExceptions = array();

/**
 * Default robot policy.
 * The default policy is to encourage indexing and following of links.
 * It may be overridden on a per-namespace and/or per-page basis.
 */
$wgDefaultRobotPolicy = 'index,follow';

/**
 * Robot policies per namespaces.
 * The default policy is given above, the array is made of namespace
 * constants as defined in includes/Defines.php
 * Example:
 *   $wgNamespaceRobotPolicies = array( NS_TALK => 'noindex' );
 */
$wgNamespaceRobotPolicies = array();

/**
 * Robot policies per article.
 * These override the per-namespace robot policies.
 * Must be in the form of an array where the key part is a properly
 * canonicalised text form title and the value is a robot policy.
 * Example:
 *   $wgArticleRobotPolicies = array( 'Main Page' => 'noindex' );
 */
$wgArticleRobotPolicies = array();

/**
 * Specifies the minimal length of a user password. If set to
 * 0, empty passwords are allowed.
 */
$wgMinimalPasswordLength = 0;

/**
 * Activate external editor interface for files and pages
 * See http://meta.wikimedia.org/wiki/Help:External_editors
 */
$wgUseExternalEditor = true;

/** Whether or not to sort special pages in Special:Specialpages */

$wgSortSpecialPages = true;

/**
 * Specify the name of a skin that should not be presented in the
 * list of available skins.
 * Use for blacklisting a skin which you do not want to remove
 * from the .../skins/ directory
 */
$wgSkipSkin = '';
$wgSkipSkins = array(); # More of the same

/**
 * Array of disabled article actions, e.g. view, edit, dublincore, delete, etc.
 */
$wgDisabledActions = array();

/**
 * Disable redirects to special pages and interwiki redirects, which use a 302 and have no "redirected from" link
 */
$wgDisableHardRedirects = false;

/**
 * Use http.dnsbl.sorbs.net to check for open proxies
 */
$wgEnableSorbs = false;
$wgSorbsUrl = 'http.dnsbl.sorbs.net.';

/**
 * Proxy whitelist, list of addresses that are assumed to be non-proxy despite what the other
 * methods might say
 */
$wgProxyWhitelist = array();

/**
 * Simple rate limiter options to brake edit floods.
 * Maximum number actions allowed in the given number of seconds;
 * after that the violating client receives HTTP 500 error pages
 * until the period elapses.
 *
 * array( 4, 60 ) for a maximum of 4 hits in 60 seconds.
 *
 * This option set is experimental and likely to change.
 * Requires memcached.
 */
$wgRateLimits = array(
	'edit' => array(
		'anon'   => null, // for any and all anonymous edits (aggregate)
		'user'   => null, // for each logged-in user
		'newbie' => null, // for each recent (autoconfirmed) account; overrides 'user'
		'ip'     => null, // for each anon and recent account
		'subnet' => null, // ... with final octet removed
		),
	'move' => array(
		'user'   => null,
		'newbie' => null,
		'ip'     => null,
		'subnet' => null,
		),
	'mailpassword' => array(
		'anon' => NULL,
		),
	'emailuser' => array(
		'user' => null,
		),
	);

/**
 * Set to a filename to log rate limiter hits.
 */
$wgRateLimitLog = null;

/**
 * Array of groups which should never trigger the rate limiter
 *
 * @deprecated as of 1.13.0, the preferred method is using
 *  $wgGroupPermissions[]['noratelimit']. However, this will still
 *  work if desired.
 *
 *  $wgRateLimitsExcludedGroups = array( 'sysop', 'bureaucrat' );
 */
$wgRateLimitsExcludedGroups = array();

/**
 * On Special:Unusedimages, consider images "used", if they are put
 * into a category. Default (false) is not to count those as used.
 */
$wgCountCategorizedImagesAsUsed = false;

/**
 * External stores allow including content
 * from non database sources following URL links
 *
 * Short names of ExternalStore classes may be specified in an array here:
 * $wgExternalStores = array("http","file","custom")...
 *
 * CAUTION: Access to database might lead to code execution
 */
$wgExternalStores = false;

/**
 * An array of external mysql servers, e.g.
 * $wgExternalServers = array( 'cluster1' => array( 'srv28', 'srv29', 'srv30' ) );
 * Used by LBFactory_Simple, may be ignored if $wgLBFactoryConf is set to another class.
 */
$wgExternalServers = array();

/**
 * The place to put new revisions, false to put them in the local text table.
 * Part of a URL, e.g. DB://cluster1
 *
 * Can be an array instead of a single string, to enable data distribution. Keys
 * must be consecutive integers, starting at zero. Example:
 *
 * $wgDefaultExternalStore = array( 'DB://cluster1', 'DB://cluster2' );
 *
 */
$wgDefaultExternalStore = false;

/**
 * Revision text may be cached in $wgMemc to reduce load on external storage
 * servers and object extraction overhead for frequently-loaded revisions.
 *
 * Set to 0 to disable, or number of seconds before cache expiry.
 */
$wgRevisionCacheExpiry = 0;

/**
 * list of trusted media-types and mime types.
 * Use the MEDIATYPE_xxx constants to represent media types.
 * This list is used by Image::isSafeFile
 *
 * Types not listed here will have a warning about unsafe content
 * displayed on the images description page. It would also be possible
 * to use this for further restrictions, like disabling direct
 * [[media:...]] links for non-trusted formats.
 */
$wgTrustedMediaFormats= array(
	MEDIATYPE_BITMAP, //all bitmap formats
	MEDIATYPE_AUDIO,  //all audio formats
	MEDIATYPE_VIDEO,  //all plain video formats
	"image/svg+xml",  //svg (only needed if inline rendering of svg is not supported)
	"application/pdf",  //PDF files
	#"application/x-shockwave-flash", //flash/shockwave movie
);

/**
 * Allow special page inclusions such as {{Special:Allpages}}
 */
$wgAllowSpecialInclusion = true;

/**
 * Timeout for HTTP requests done via CURL
 */
$wgHTTPTimeout = 3;

/**
 * Proxy to use for CURL requests.
 */
$wgHTTPProxy = false;

/**
 * Enable interwiki transcluding.  Only when iw_trans=1.
 */
$wgEnableScaryTranscluding = false;
/**
 * Expiry time for interwiki transclusion
 */
$wgTranscludeCacheExpiry = 3600;

/**
 * Support blog-style "trackbacks" for articles.  See
 * http://www.sixapart.com/pronet/docs/trackback_spec for details.
 */
$wgUseTrackbacks = false;

/**
 * Enable filtering of categories in Recentchanges
 */
$wgAllowCategorizedRecentChanges = false ;

/**
 * Number of jobs to perform per request. May be less than one in which case
 * jobs are performed probabalistically. If this is zero, jobs will not be done
 * during ordinary apache requests. In this case, maintenance/runJobs.php should
 * be run periodically.
 */
$wgJobRunRate = 1;

/**
 * Number of rows to update per job
 */
$wgUpdateRowsPerJob = 500;

/**
 * Number of rows to update per query
 */
$wgUpdateRowsPerQuery = 10;

/**
 * Enable AJAX framework
 */
$wgUseAjax = true;

/**
 * Enable auto suggestion for the search bar
 * Requires $wgUseAjax to be true too.
 * Causes wfSajaxSearch to be added to $wgAjaxExportList
 */
$wgAjaxSearch = false;

/**
 * List of Ajax-callable functions.
 * Extensions acting as Ajax callbacks must register here
 */
$wgAjaxExportList = array( );

/**
 * Enable watching/unwatching pages using AJAX.
 * Requires $wgUseAjax to be true too.
 * Causes wfAjaxWatch to be added to $wgAjaxExportList
 */
$wgAjaxWatch = true;

/**
 * Enable AJAX check for file overwrite, pre-upload
 */
$wgAjaxUploadDestCheck = true;

/**
 * Enable previewing licences via AJAX
 */
$wgAjaxLicensePreview = true;

/**
 * Allow DISPLAYTITLE to change title display
 */
$wgAllowDisplayTitle = true;

/**
 * Array of usernames which may not be registered or logged in from
 * Maintenance scripts can still use these
 */
$wgReservedUsernames = array(
	'MediaWiki default', // Default 'Main Page' and MediaWiki: message pages
	'Conversion script', // Used for the old Wikipedia software upgrade
	'Maintenance script', // Maintenance scripts which perform editing, image import script
	'Template namespace initialisation script', // Used in 1.2->1.3 upgrade
	'msg:double-redirect-fixer', // Automatic double redirect fix
);

/**
 * MediaWiki will reject HTMLesque tags in uploaded files due to idiotic browsers which can't
 * perform basic stuff like MIME detection and which are vulnerable to further idiots uploading
 * crap files as images. When this directive is on, <title> will be allowed in files with
 * an "image/svg+xml" MIME type. You should leave this disabled if your web server is misconfigured
 * and doesn't send appropriate MIME types for SVG images.
 */
$wgAllowTitlesInSVG = false;

/**
 * Array of namespaces which can be deemed to contain valid "content", as far
 * as the site statistics are concerned. Useful if additional namespaces also
 * contain "content" which should be considered when generating a count of the
 * number of articles in the wiki.
 */
$wgContentNamespaces = array( NS_MAIN );

/**
 * Maximum amount of virtual memory available to shell processes under linux, in KB.
 */
$wgMaxShellMemory = 102400;

/**
 * Maximum file size created by shell processes under linux, in KB
 * ImageMagick convert for example can be fairly hungry for scratch space
 */
$wgMaxShellFileSize = 102400;

/**
 * DJVU settings
 * Path of the djvudump executable
 * Enable this and $wgDjvuRenderer to enable djvu rendering
 */
# $wgDjvuDump = 'djvudump';
$wgDjvuDump = null;

/**
 * Path of the ddjvu DJVU renderer
 * Enable this and $wgDjvuDump to enable djvu rendering
 */
# $wgDjvuRenderer = 'ddjvu';
$wgDjvuRenderer = null;

/**
 * Path of the djvutoxml executable
 * This works like djvudump except much, much slower as of version 3.5.
 *
 * For now I recommend you use djvudump instead. The djvuxml output is
 * probably more stable, so we'll switch back to it as soon as they fix
 * the efficiency problem.
 * http://sourceforge.net/tracker/index.php?func=detail&aid=1704049&group_id=32953&atid=406583
 */
# $wgDjvuToXML = 'djvutoxml';
$wgDjvuToXML = null;


/**
 * Shell command for the DJVU post processor
 * Default: pnmtopng, since ddjvu generates ppm output
 * Set this to false to output the ppm file directly.
 */
$wgDjvuPostProcessor = 'pnmtojpeg';
/**
 * File extension for the DJVU post processor output
 */
$wgDjvuOutputExtension = 'jpg';

/**
 * Enable the MediaWiki API for convenient access to
 * machine-readable data via api.php
 *
 * See http://www.mediawiki.org/wiki/API
 */
$wgEnableAPI = true;

/**
 * Allow the API to be used to perform write operations
 * (page edits, rollback, etc.) when an authorised user
 * accesses it
 */
$wgEnableWriteAPI = false;

/**
 * API module extensions
 * Associative array mapping module name to class name.
 * Extension modules may override the core modules.
 */
$wgAPIModules = array();
$wgAPIMetaModules = array();
$wgAPIPropModules = array();
$wgAPIListModules = array();

/**
 * Maximum amount of rows to scan in a DB query in the API
 * The default value is generally fine
 */
$wgAPIMaxDBRows = 5000;

/**
 * Parser test suite files to be run by parserTests.php when no specific
 * filename is passed to it.
 *
 * Extensions may add their own tests to this array, or site-local tests
 * may be added via LocalSettings.php
 *
 * Use full paths.
 */
$wgParserTestFiles = array(
	"$IP/maintenance/parserTests.txt",
);

/**
 * Break out of framesets. This can be used to prevent external sites from
 * framing your site with ads.
 */
$wgBreakFrames = false;

/**
 * Set this to an array of special page names to prevent
 * maintenance/updateSpecialPages.php from updating those pages.
 */
$wgDisableQueryPageUpdate = false;

/**
 * Disable output compression (enabled by default if zlib is available)
 */
$wgDisableOutputCompression = false;

/**
 * If lag is higher than $wgSlaveLagWarning, show a warning in some special
 * pages (like watchlist).  If the lag is higher than $wgSlaveLagCritical,
 * show a more obvious warning.
 */
$wgSlaveLagWarning = 10;
$wgSlaveLagCritical = 30;

/**
 * Parser configuration. Associative array with the following members:
 *
 *  class             The class name
 *
 *  preprocessorClass The preprocessor class. Two classes are currently available:
 *                    Preprocessor_Hash, which uses plain PHP arrays for tempoarary
 *                    storage, and Preprocessor_DOM, which uses the DOM module for
 *                    temporary storage. Preprocessor_DOM generally uses less memory;
 *                    the speed of the two is roughly the same.
 *
 *                    If this parameter is not given, it uses Preprocessor_DOM if the
 *                    DOM module is available, otherwise it uses Preprocessor_Hash.
 *
 *                    Has no effect on Parser_OldPP.
 *
 * The entire associative array will be passed through to the constructor as
 * the first parameter. Note that only Setup.php can use this variable --
 * the configuration will change at runtime via $wgParser member functions, so
 * the contents of this variable will be out-of-date. The variable can only be
 * changed during LocalSettings.php, in particular, it can't be changed during
 * an extension setup function.
 */
$wgParserConf = array(
	'class' => 'Parser',
	#'preprocessorClass' => 'Preprocessor_Hash',
);

/**
 * Hooks that are used for outputting exceptions.  Format is:
 *   $wgExceptionHooks[] = $funcname
 * or:
 *   $wgExceptionHooks[] = array( $class, $funcname )
 * Hooks should return strings or false
 */
$wgExceptionHooks = array();

/**
 * Page property link table invalidation lists. Should only be set by exten-
 * sions.
 */
$wgPagePropLinkInvalidations = array(
	'hiddencat' => 'categorylinks',
);

/**
 * Maximum number of links to a redirect page listed on
 * Special:Whatlinkshere/RedirectDestination
 */
$wgMaxRedirectLinksRetrieved = 500;

/**
 * Maximum number of calls per parse to expensive parser functions such as
 * PAGESINCATEGORY.
 */
$wgExpensiveParserFunctionLimit = 100;

/**
 * Maximum number of pages to move at once when moving subpages with a page.
 */
$wgMaximumMovedPages = 100;

/**
 * Array of namespaces to generate a sitemap for when the
 * maintenance/generateSitemap.php script is run, or false if one is to be ge-
 * nerated for all namespaces.
 */
$wgSitemapNamespaces = false;


/**
 * If user doesn't specify any edit summary when making a an edit, MediaWiki
 * will try to automatically create one. This feature can be disabled by set-
 * ting this variable false.
 */
$wgUseAutomaticEditSummaries = true;
