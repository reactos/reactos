<?php

# MediaWiki web-based config/installation
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>, 2006 Rob Church <robchur@gmail.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

error_reporting( E_ALL );
header( "Content-type: text/html; charset=utf-8" );
@ini_set( "display_errors", true );

define("ROSCMS_PATH", "../../roscms/");

# In case of errors, let output be clean.
$wgRequestTime = microtime( true );

# Attempt to set up the include path, to fix problems with relative includes
$IP = dirname( dirname( __FILE__ ) );
define( 'MW_INSTALL_PATH', $IP );

# Define an entry point and include some files
define( "MEDIAWIKI", true );
define( "MEDIAWIKI_INSTALL", true );

// Run version checks before including other files
// so people don't see a scary parse error.
require_once( "$IP/install-utils.inc" );
install_version_checks();

require_once( "$IP/includes/Defines.php" );
require_once( "$IP/includes/DefaultSettings.php" );
require_once( "$IP/includes/AutoLoader.php" );
require_once( "$IP/includes/MagicWord.php" );
require_once( "$IP/includes/Namespace.php" );
require_once( "$IP/includes/ProfilerStub.php" );
require_once( "$IP/includes/GlobalFunctions.php" );
require_once( "$IP/includes/Hooks.php" );

# If we get an exception, the user needs to know
# all the details
$wgShowExceptionDetails = true;

## Databases we support:

$ourdb = array();
$ourdb['mysql']['fullname']      = 'MySQL';
$ourdb['mysql']['havedriver']    = 0;
$ourdb['mysql']['compile']       = 'mysql';
$ourdb['mysql']['bgcolor']       = '#ffe5a7';
$ourdb['mysql']['rootuser']      = 'root';

$ourdb['postgres']['fullname']   = 'PostgreSQL';
$ourdb['postgres']['havedriver'] = 0;
$ourdb['postgres']['compile']    = 'pgsql';
$ourdb['postgres']['bgcolor']    = '#aaccff';
$ourdb['postgres']['rootuser']   = 'postgres';

$ourdb['sqlite']['fullname']      = 'SQLite';
$ourdb['sqlite']['havedriver']    = 0;
$ourdb['sqlite']['compile']       = 'pdo_sqlite';
$ourdb['sqlite']['bgcolor']       = '#b1ebb1';
$ourdb['sqlite']['rootuser']      = '';

$ourdb['mssql']['fullname']      = 'MSSQL';
$ourdb['mssql']['havedriver']    = 0;
$ourdb['mssql']['compile']       = 'mssql not ready'; # Change to 'mssql' after includes/DatabaseMssql.php added;
$ourdb['mssql']['bgcolor']       = '#ffc0cb';
$ourdb['mssql']['rootuser']      = 'administrator';

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr">
<head>
	<meta http-equiv="Content-type" content="text/html; charset=utf-8" />
	<title>MediaWiki <?php echo( $wgVersion ); ?> Installation</title>
	<style type="text/css">

		@import "../skins/monobook/main.css";

		.env-check {
			font-size: 90%;
			margin: 1em 0 1em 2.5em;
		}

		.config-section {
			margin-top: 2em;
		}

		.config-section label.column {
			clear: left;
			font-weight: bold;
			width: 13em;
			float: left;
			text-align: right;
			padding-right: 1em;
			padding-top: .2em;
		}

		.config-input {
			clear: left;
			zoom: 100%; /* IE hack */
		}

		.config-section .config-desc {
			clear: left;
			margin: 0 0 2em 18em;
			padding-top: 1em;
			font-size: 85%;
		}

		.iput-text, .iput-password {
			width: 14em;
			margin-right: 1em;
		}

		.error {
			color: red;
			background-color: #fff;
			font-weight: bold;
			left: 1em;
			font-size: 100%;
		}

		.error-top {
			color: red;
			background-color: #FFF0F0;
			border: 2px solid red;
			font-size: 130%;
			font-weight: bold;
			padding: 1em 1.5em;
			margin: 2em 0 1em;
		}

		ul.plain {
			list-style-type: none;
			list-style-image: none;
			float: left;
			margin: 0;
			padding: 0;
		}

		.btn-install {
			font-weight: bold;
			font-size: 110%;
			padding: .2em .3em;
		}

		.license {
			font-size: 85%;
			padding-top: 3em;
		}
		
		span.success-message {
			font-weight: bold;
			font-size: 110%;
			color: green;
		}
		.success-box {
			font-size: 130%;
		}

	</style>
	<script type="text/javascript">
	<!--
	function hideall() {
		<?php foreach (array_keys($ourdb) as $db) {
		echo "\n		var i = document.getElementById('$db'); if (i) i.style.display='none';";
		}
		?>

	}
	function toggleDBarea(id,defaultroot) {
		hideall();
		var dbarea = document.getElementById(id);
		if (dbarea) dbarea.style.display = (dbarea.style.display == 'none') ? 'block' : 'none';
		var db = document.getElementById('RootUser');
		if (defaultroot) {
<?php foreach (array_keys($ourdb) as $db) {
			echo "			if (id == '$db') { db.value = '".$ourdb[$db]['rootuser']."';}\n";
}?>
		}
	}
	// -->
	</script>
</head>

<body>
<div id="globalWrapper">
<div id="column-content">
<div id="content">
<div id="bodyContent">

<h1>MediaWiki <?php print $wgVersion ?> Installation</h1>

<?php
$mainListOpened = false; # Is the main list (environement checking) opend ? Used by dieout

/* Check for existing configurations and bug out! */

if( file_exists( "../LocalSettings.php" ) ) {
	$script = defined('MW_INSTALL_PHP5_EXT') ? 'index.php5' : 'index.php';
	dieout( "<p><strong>Setup has completed, <a href='../$script'>your wiki</a> is configured.</strong></p>
	<p>Please delete the /config directory for extra security.</p>" );
}

if( file_exists( "./LocalSettings.php" ) ) {
	writeSuccessMessage();
	dieout( '' );
}

if( !is_writable( "." ) ) {
	dieout( "<h2>Can't write config file, aborting</h2>

	<p>In order to configure the wiki you have to make the <tt>config</tt> subdirectory
	writable by the web server. Once configuration is done you'll move the created
	<tt>LocalSettings.php</tt> to the parent directory, and for added safety you can
	then remove the <tt>config</tt> subdirectory entirely.</p>

	<p>To make the directory writable on a Unix/Linux system:</p>

	<pre>
	cd <i>/path/to/wiki</i>
	chmod a+w config
	</pre>
	
	<p>Afterwards retry to start the <a href=\"\">setup</a>.</p>" );
}


require_once( "$IP/install-utils.inc" );
require_once( "$IP/maintenance/updaters.inc" );

class ConfigData {
	function getEncoded( $data ) {
		# removing latin1 support, no need...
		return $data;
	}
	function getSitename() { return $this->getEncoded( $this->Sitename ); }
	function getSysopName() { return $this->getEncoded( $this->SysopName ); }
	function getSysopPass() { return $this->getEncoded( $this->SysopPass ); }

	function setSchema( $schema, $engine ) {
		$this->DBschema = $schema;
		if ( !preg_match( '/^\w*$/', $engine ) ){
			$engine = 'InnoDB';
		}
		switch ( $this->DBschema ) {
			case 'mysql5':
				$this->DBTableOptions = "ENGINE=$engine, DEFAULT CHARSET=utf8";
				$this->DBmysql5 = 'true';
				break;
			case 'mysql5-binary':
				$this->DBTableOptions = "ENGINE=$engine, DEFAULT CHARSET=binary";
				$this->DBmysql5 = 'true';
				break;
			default:
				$this->DBTableOptions = "TYPE=$engine";
				$this->DBmysql5 = 'false';
		}
		$this->DBengine = $engine;

		# Set the global for use during install
		global $wgDBTableOptions;
		$wgDBTableOptions = $this->DBTableOptions;
	}
}

?>

<ul>
	<li>
		<b>Don't forget security updates!</b> Keep an eye on the
		<a href="http://lists.wikimedia.org/mailman/listinfo/mediawiki-announce">low-traffic
		release announcements mailing list</a>.
	</li>
</ul>


<h2>Checking environment...</h2>
<p><em>Please include all of the lines below when reporting installation problems.</em></p>
<ul class="env-check">
<?php
$mainListOpened = true;

$endl = "
";
define( 'MW_NO_OUTPUT_BUFFER', 1 );
$conf = new ConfigData;

install_version_checks();
$self = 'Installer'; # Maintenance script name, to please Setup.php

print "<li>PHP " . phpversion() . " installed</li>\n";

error_reporting( 0 );
$phpdatabases = array();
foreach (array_keys($ourdb) as $db) {
	$compname = $ourdb[$db]['compile'];
	if( extension_loaded( $compname ) || ( mw_have_dl() && dl( "{$compname}." . PHP_SHLIB_SUFFIX ) ) ) {
		array_push($phpdatabases, $db);
		$ourdb[$db]['havedriver'] = 1;
	}
}
error_reporting( E_ALL );

if (!$phpdatabases) {
	print "Could not find a suitable database driver!<ul>";
	foreach (array_keys($ourdb) AS $db) {
		$comp = $ourdb[$db]['compile'];
		$full = $ourdb[$db]['fullname'];
		print "<li>For <b>$full</b>, compile PHP using <b>--with-$comp</b>, "
			."or install the $comp.so module</li>\n";
	}
	echo '</ul>';
	dieout( '' );
}

print "<li>Found database drivers for:";
$DefaultDBtype = '';
foreach (array_keys($ourdb) AS $db) {
	if ($ourdb[$db]['havedriver']) {
		if ( $DefaultDBtype == '' ) {
			$DefaultDBtype = $db;
		}
		print "  ".$ourdb[$db]['fullname'];
	}
}
print "</li>\n";

if( wfIniGetBool( "register_globals" ) ) {
	?>
	<li>
		<div style="font-size:110%">
		<strong class="error">Warning:</strong>
		<strong>PHP's <tt><a href="http://php.net/register_globals">register_globals</a></tt> option is enabled. Disable it if you can.</strong>
		</div>
		MediaWiki will work, but your server is more exposed to PHP-based security vulnerabilities.
	</li>
	<?php
}

$fatal = false;

if( wfIniGetBool( "magic_quotes_runtime" ) ) {
	$fatal = true;
	?><li class='error'><strong>Fatal: <a href='http://www.php.net/manual/en/ref.info.php#ini.magic-quotes-runtime'>magic_quotes_runtime</a> is active!</strong>
	This option corrupts data input unpredictably; you cannot install or use
	MediaWiki unless this option is disabled.</li>
	<?php
}

if( wfIniGetBool( "magic_quotes_sybase" ) ) {
	$fatal = true;
	?><li class='error'><strong>Fatal: <a href='http://www.php.net/manual/en/ref.sybase.php#ini.magic-quotes-sybase'>magic_quotes_sybase</a> is active!</strong>
	This option corrupts data input unpredictably; you cannot install or use
	MediaWiki unless this option is disabled.</li>
	<?php
}

if( wfIniGetBool( "mbstring.func_overload" ) ) {
	$fatal = true;
	?><li class='error'><strong>Fatal: <a href='http://www.php.net/manual/en/ref.mbstring.php#mbstring.overload'>mbstring.func_overload</a> is active!</strong>
	This option causes errors and may corrupt data unpredictably;
	you cannot install or use MediaWiki unless this option is disabled.</li>
	<?php
}

if( wfIniGetBool( "zend.ze1_compatibility_mode" ) ) {
	$fatal = true;
	?><li class="error"><strong>Fatal: <a href="http://www.php.net/manual/en/ini.core.php">zend.ze1_compatibility_mode</a> is active!</strong>
	This option causes horrible bugs with MediaWiki; you cannot install or use
	MediaWiki unless this option is disabled.</li>
	<?php
}


if( $fatal ) {
	dieout( "Cannot install MediaWiki." );
}

if( wfIniGetBool( "safe_mode" ) ) {
	$conf->safeMode = true;
	?>
	<li><b class='error'>Warning:</b> <strong>PHP's
	<a href='http://www.php.net/features.safe-mode'>safe mode</a> is active.</strong>
	You may have problems caused by this, particularly if using image uploads.
	</li>
	<?php
} else {
	$conf->safeMode = false;
}

$sapi = php_sapi_name();
print "<li>PHP server API is $sapi; ";
$script = defined('MW_INSTALL_PHP5_EXT') ? 'index.php5' : 'index.php';
if( $wgUsePathInfo ) {
 print "ok, using pretty URLs (<tt>$script/Page_Title</tt>)";
} else {
	print "using ugly URLs (<tt>$script?title=Page_Title</tt>)";
}
print "</li>\n";

$conf->xml = function_exists( "utf8_encode" );
if( $conf->xml ) {
	print "<li>Have XML / Latin1-UTF-8 conversion support.</li>\n";
} else {
	dieout( "PHP's XML module is missing; the wiki requires functions in
		this module and won't work in this configuration.
		If you're running Mandrake, install the php-xml package." );
}

# Check for session support
if( !function_exists( 'session_name' ) )
	dieout( "PHP's session module is missing. MediaWiki requires session support in order to function." );

# session.save_path doesn't *have* to be set, but if it is, and it's
# not valid/writable/etc. then it can cause problems
$sessionSavePath = mw_get_session_save_path();
$ssp = htmlspecialchars( $sessionSavePath );
# Warn the user if it's not set, but let them proceed
if( !$sessionSavePath ) {
	print "<li><strong>Warning:</strong> A value for <tt>session.save_path</tt>
	has not been set in PHP.ini. If the default value causes problems with
	saving session data, set it to a valid path which is read/write/execute
	for the user your web server is running under.</li>";
} elseif ( is_dir( $sessionSavePath ) && is_writable( $sessionSavePath ) ) {
	# All good? Let the user know
	print "<li>Session save path (<tt>{$ssp}</tt>) appears to be valid.</li>";
} else {
	# Something not right? Warn the user, but let them proceed
	print "<li><strong>Warning:</strong> Your <tt>session.save_path</tt> value (<tt>{$ssp}</tt>)
		appears to be invalid or is not writable. PHP needs to be able to save data to
		this location for correct session operation.</li>";
}

# Check for PCRE support
if( !function_exists( 'preg_match' ) )
	dieout( "The PCRE support module appears to be missing. MediaWiki requires the
	Perl-compatible regular expression functions." );

$memlimit = ini_get( "memory_limit" );
$conf->raiseMemory = false;
if( empty( $memlimit ) || $memlimit == -1 ) {
	print "<li>PHP is configured with no <tt>memory_limit</tt>.</li>\n";
} else {
	print "<li>PHP's <tt>memory_limit</tt> is " . htmlspecialchars( $memlimit ) . ". ";
	$n = intval( $memlimit );
	if( preg_match( '/^([0-9]+)[Mm]$/', trim( $memlimit ), $m ) ) {
		$n = intval( $m[1] * (1024*1024) );
	}
	if( $n < 20*1024*1024 ) {
		print "Attempting to raise limit to 20M... ";
		if( false === ini_set( "memory_limit", "20M" ) ) {
			print "failed.<br /><b>" . htmlspecialchars( $memlimit ) . " seems too low, installation may fail!</b>";
		} else {
			$conf->raiseMemory = true;
			print "ok.";
		}
	}
	print "</li>\n";
}

$conf->turck = function_exists( 'mmcache_get' );
if ( $conf->turck ) {
	print "<li><a href=\"http://turck-mmcache.sourceforge.net/\">Turck MMCache</a> installed</li>\n";
}

$conf->xcache = function_exists( 'xcache_get' );
if( $conf->xcache )
	print "<li><a href=\"http://trac.lighttpd.net/xcache/\">XCache</a> installed</li>\n";

$conf->apc = function_exists('apc_fetch');
if ($conf->apc ) {
	print "<li><a href=\"http://www.php.net/apc\">APC</a> installed</li>\n";
}

$conf->eaccel = function_exists( 'eaccelerator_get' );
if ( $conf->eaccel ) {
	$conf->turck = 'eaccelerator';
	print "<li><a href=\"http://eaccelerator.sourceforge.net/\">eAccelerator</a> installed</li>\n";
}

$conf->dba = function_exists( 'dba_open' );

if( !( $conf->turck || $conf->eaccel || $conf->apc || $conf->xcache ) ) {
	echo( '<li>Couldn\'t find <a href="http://turck-mmcache.sourceforge.net">Turck MMCache</a>,
		<a href="http://eaccelerator.sourceforge.net">eAccelerator</a>,
		<a href="http://www.php.net/apc">APC</a> or <a href="http://trac.lighttpd.net/xcache/">XCache</a>;
		cannot use these for object caching.</li>' );
}

$conf->diff3 = false;
$diff3locations = array_merge(
	array(
		"/usr/bin",
		"/usr/local/bin",
		"/opt/csw/bin",
		"/usr/gnu/bin",
		"/usr/sfw/bin" ),
	explode( PATH_SEPARATOR, getenv( "PATH" ) ) );
$diff3names = array( "gdiff3", "diff3", "diff3.exe" );

$diff3versioninfo = array( '$1 --version 2>&1', 'diff3 (GNU diffutils)' );
foreach ($diff3locations as $loc) {
	$exe = locate_executable($loc, $diff3names, $diff3versioninfo);
	if ($exe !== false) {
		$conf->diff3 = $exe;
		break;
	}
}

if ($conf->diff3)
	print "<li>Found GNU diff3: <tt>$conf->diff3</tt>.</li>";
else
	print "<li>GNU diff3 not found.</li>";

$conf->ImageMagick = false;
$imcheck = array( "/usr/bin", "/opt/csw/bin", "/usr/local/bin", "/sw/bin", "/opt/local/bin" );
foreach( $imcheck as $dir ) {
	$im = "$dir/convert";
	if( @file_exists( $im ) ) {
		print "<li>Found ImageMagick: <tt>$im</tt>; image thumbnailing will be enabled if you enable uploads.</li>\n";
		$conf->ImageMagick = $im;
		break;
	}
}

$conf->HaveGD = function_exists( "imagejpeg" );
if( $conf->HaveGD ) {
	print "<li>Found GD graphics library built-in";
	if( !$conf->ImageMagick ) {
		print ", image thumbnailing will be enabled if you enable uploads";
	}
	print ".</li>\n";
} else {
	if( !$conf->ImageMagick ) {
		print "<li>Couldn't find GD library or ImageMagick; image thumbnailing disabled.</li>\n";
	}
}

$conf->IP = dirname( dirname( __FILE__ ) );
print "<li>Installation directory: <tt>" . htmlspecialchars( $conf->IP ) . "</tt></li>\n";


// PHP_SELF isn't available sometimes, such as when PHP is CGI but
// cgi.fix_pathinfo is disabled. In that case, fall back to SCRIPT_NAME
// to get the path to the current script... hopefully it's reliable. SIGH
$path = ($_SERVER["PHP_SELF"] === '')
	? $_SERVER["SCRIPT_NAME"]
	: $_SERVER["PHP_SELF"];

$conf->ScriptPath = preg_replace( '{^(.*)/config.*$}', '$1', $path );
print "<li>Script URI path: <tt>" . htmlspecialchars( $conf->ScriptPath ) . "</tt></li>\n";



// We may be installing from *.php5 extension file, if so, print message
$conf->ScriptExtension = '.php';
if (defined('MW_INSTALL_PHP5_EXT')) {
    $conf->ScriptExtension = '.php5';
    print "<li>Installing MediaWiki with <tt>php5</tt> file extensions</li>\n";
} else {
    print "<li>Installing MediaWiki with <tt>php</tt> file extensions</li>\n";
}


print "<li style='font-weight:bold;color:green;font-size:110%'>Environment checked. You can install MediaWiki.</li>\n";
	$conf->posted = ($_SERVER["REQUEST_METHOD"] == "POST");

	$conf->Sitename = ucfirst( importPost( "Sitename", "" ) );
	$defaultEmail = empty( $_SERVER["SERVER_ADMIN"] )
		? 'root@localhost'
		: $_SERVER["SERVER_ADMIN"];
	$conf->EmergencyContact = importPost( "EmergencyContact", $defaultEmail );
	$conf->DBtype = importPost( "DBtype", $DefaultDBtype );

	$conf->DBserver = importPost( "DBserver", "localhost" );
	$conf->DBname = importPost( "DBname", "wikidb" );
	$conf->DBuser = importPost( "DBuser", "wikiuser" );
	$conf->DBpassword = importPost( "DBpassword" );
	$conf->DBpassword2 = importPost( "DBpassword2" );
	$conf->SysopName = importPost( "SysopName", "WikiSysop" );
	$conf->SysopPass = importPost( "SysopPass" );
	$conf->SysopPass2 = importPost( "SysopPass2" );
	$conf->RootUser = importPost( "RootUser", "root" );
	$conf->RootPW = importPost( "RootPW", "" );
	$useRoot = importCheck( 'useroot', false );
	$conf->LanguageCode = importPost( "LanguageCode", "en" );
	## MySQL specific:
	$conf->DBprefix     = importPost( "DBprefix" );
	$conf->setSchema( 
		importPost( "DBschema", "mysql5-binary" ), 
		importPost( "DBengine", "InnoDB" ) );

	## Postgres specific:
	$conf->DBport      = importPost( "DBport",      "5432" );
	$conf->DBmwschema  = importPost( "DBmwschema",  "mediawiki" );
	$conf->DBts2schema = importPost( "DBts2schema", "public" );
	
	## SQLite specific
	$conf->SQLiteDataDir = importPost( "SQLiteDataDir", "" );
	
	## MSSQL specific
	// We need a second field so it doesn't overwrite the MySQL one
	$conf->DBprefix2 = importPost( "DBprefix2" );

	$conf->ShellLocale = getShellLocale( $conf->LanguageCode );

/* Check for validity */
$errs = array();

if( preg_match( '/^$|^mediawiki$|#/i', $conf->Sitename ) ) {
	$errs["Sitename"] = "Must not be blank or \"MediaWiki\" and may not contain \"#\"";
}
if( $conf->DBuser == "" ) {
	$errs["DBuser"] = "Must not be blank";
}
if( ($conf->DBtype == 'mysql') && (strlen($conf->DBuser) > 16) ) {
	$errs["DBuser"] = "Username too long";
}
if( $conf->DBpassword == "" && $conf->DBtype != "postgres" ) {
	$errs["DBpassword"] = "Must not be blank";
}
if( $conf->DBpassword != $conf->DBpassword2 ) {
	$errs["DBpassword2"] = "Passwords don't match!";
}
if( !preg_match( '/^[A-Za-z_0-9]*$/', $conf->DBprefix ) ) {
	$errs["DBprefix"] = "Invalid table prefix";
}

error_reporting( E_ALL );

/**
 * Initialise $wgLang and $wgContLang to something so we can
 * call case-folding methods. Per Brion, this is English for
 * now, although we could be clever and initialise to the
 * user-selected language.
 */
$wgContLang = Language::factory( 'en' );
$wgLang = $wgContLang;

/**
 * We're messing about with users, so we need a stub
 * authentication plugin...
 */
$wgAuth = new AuthPlugin();

/**
 * Validate the initial administrator account; username,
 * password checks, etc.
 */
if( $conf->SysopName ) {
	# Check that the user can be created
	$u = User::newFromName( $conf->SysopName );
	if( is_a($u, 'User') ) { // please do not use instanceof, it breaks PHP4
		# Various password checks
		if( $conf->SysopPass != '' ) {
			if( $conf->SysopPass == $conf->SysopPass2 ) {
				if( !$u->isValidPassword( $conf->SysopPass ) ) {
					$errs['SysopPass'] = "Bad password";
				}
			} else {
				$errs['SysopPass2'] = "Passwords don't match";
			}
		} else {
			$errs['SysopPass'] = "Cannot be blank";
		}
		unset( $u );
	} else {
		$errs['SysopName'] = "Bad username";
	}
}

$conf->License = importRequest( "License", "none" );
if( $conf->License == "gfdl" ) {
	$conf->RightsUrl = "http://www.gnu.org/copyleft/fdl.html";
	$conf->RightsText = "GNU Free Documentation License 1.2";
	$conf->RightsCode = "gfdl";
	$conf->RightsIcon = '${wgScriptPath}/skins/common/images/gnu-fdl.png';
} elseif( $conf->License == "none" ) {
	$conf->RightsUrl = $conf->RightsText = $conf->RightsCode = $conf->RightsIcon = "";
} else {
	$conf->RightsUrl = importRequest( "RightsUrl", "" );
	$conf->RightsText = importRequest( "RightsText", "" );
	$conf->RightsCode = importRequest( "RightsCode", "" );
	$conf->RightsIcon = importRequest( "RightsIcon", "" );
}

$conf->Shm = importRequest( "Shm", "none" );
$conf->MCServers = importRequest( "MCServers" );

/* Test memcached servers */

if ( $conf->Shm == 'memcached' && $conf->MCServers ) {
	$conf->MCServerArray = array_map( 'trim', explode( ',', $conf->MCServers ) );
	foreach ( $conf->MCServerArray as $server ) {
		$error = testMemcachedServer( $server );
		if ( $error ) {
			$errs["MCServers"] = $error;
			break;
		}
	}
} else if ( $conf->Shm == 'memcached' ) {
	$errs["MCServers"] = "Please specify at least one server if you wish to use memcached";
}

/* default values for installation */
$conf->Email     = importRequest("Email", "email_enabled");
$conf->Emailuser = importRequest("Emailuser", "emailuser_enabled");
$conf->Enotif    = importRequest("Enotif", "enotif_allpages");
$conf->Eauthent  = importRequest("Eauthent", "eauthent_enabled");

if( $conf->posted && ( 0 == count( $errs ) ) ) {
	do { /* So we can 'continue' to end prematurely */
		$conf->Root = ($conf->RootPW != "");

		/* Load up the settings and get installin' */
		$local = writeLocalSettings( $conf );
		echo "<li style=\"list-style: none\">\n";
		echo "<p><b>Generating configuration file...</b></p>\n";
		echo "</li>\n";		

		$wgCommandLineMode = false;
		chdir( ".." );
		$ok = eval( $local );
		if( $ok === false ) {
			dieout( "<p>Errors in generated configuration; " .
				"most likely due to a bug in the installer... " .
				"Config file was: </p>" .
				"<pre>" .
				htmlspecialchars( $local ) .
				"</pre>" );
		}
		$conf->DBtypename = '';
		foreach (array_keys($ourdb) as $db) {
			if ($conf->DBtype === $db)
				$conf->DBtypename = $ourdb[$db]['fullname'];
		}
		if ( ! strlen($conf->DBtype)) {
			$errs["DBpicktype"] = "Please choose a database type";
			continue;
		}

		if (! $conf->DBtypename) {
			$errs["DBtype"] = "Unknown database type '$conf->DBtype'";
			continue;
		}
		print "<li>Database type: {$conf->DBtypename}</li>\n";
		$dbclass = 'Database'.ucfirst($conf->DBtype);
		$wgDBtype = $conf->DBtype;
		$wgDBadminuser = "root";
		$wgDBadminpassword = $conf->RootPW;

		## Mysql specific:
		$wgDBprefix = $conf->DBprefix;

		## Postgres specific:
		$wgDBport      = $conf->DBport;
		$wgDBmwschema  = $conf->DBmwschema;
		$wgDBts2schema = $conf->DBts2schema;

		if( $conf->DBprefix2 != '' ) {
			// For MSSQL
			$wgDBprefix = $conf->DBprefix2;
		}

		$wgCommandLineMode = true;
		if (! defined ( 'STDERR' ) )
			define( 'STDERR', fopen("php://stderr", "wb"));
		$wgUseDatabaseMessages = false; /* FIXME: For database failure */
		require_once( "$IP/includes/Setup.php" );
		chdir( "config" );

		$wgTitle = Title::newFromText( "Installation script" );
		error_reporting( E_ALL );
		print "<li>Loading class: $dbclass</li>\n";
		$dbc = new $dbclass;

		if( $conf->DBtype == 'mysql' ) {
			$mysqlOldClient = version_compare( mysql_get_client_info(), "4.1.0", "lt" );
			if( $mysqlOldClient ) {
				print "<li><b>PHP is linked with old MySQL client libraries. If you are
					using a MySQL 4.1 server and have problems connecting to the database,
					see <a href='http://dev.mysql.com/doc/mysql/en/old-client.html'
			 		>http://dev.mysql.com/doc/mysql/en/old-client.html</a> for help.</b></li>\n";
			}
			$ok = true; # Let's be optimistic

			# Decide if we're going to use the superuser or the regular database user
			$conf->Root = $useRoot;
			if( $conf->Root ) {
				$db_user = $conf->RootUser;
				$db_pass = $conf->RootPW;
			} else {
				$db_user = $wgDBuser;
				$db_pass = $wgDBpassword;
			}

			# Attempt to connect
			echo( "<li>Attempting to connect to database server as $db_user..." );
			$wgDatabase = Database::newFromParams( $wgDBserver, $db_user, $db_pass, '', 1 );

			# Check the connection and respond to errors
			if( $wgDatabase->isOpen() ) {
				# Seems OK
				$ok = true;
				$wgDBadminuser = $db_user;
				$wgDBadminpassword = $db_pass;
				echo( "success.</li>\n" );
				$wgDatabase->ignoreErrors( true );
				$myver = $wgDatabase->getServerVersion();
			} else {
				# There were errors, report them and back out
				$ok = false;
				$errno = mysql_errno();
				$errtx = htmlspecialchars( mysql_error() );
				switch( $errno ) {
					case 1045:
					case 2000:
						echo( "failed due to authentication errors. Check passwords.</li>" );
						if( $conf->Root ) {
							# The superuser details are wrong
							$errs["RootUser"] = "Check username";
							$errs["RootPW"] = "and password";
						} else {
							# The regular user details are wrong
							$errs["DBuser"] = "Check username";
							$errs["DBpassword"] = "and password";
						}
						break;
					case 2002:
					case 2003:
					default:
						# General connection problem
						echo( "failed with error [$errno] $errtx.</li>\n" );
						$errs["DBserver"] = "Connection failed";
						break;
				} # switch
			} #conn. att.

			if( !$ok ) { continue; }

		} else { # not mysql
			error_reporting( E_ALL );
			$wgSuperUser = '';
			## Possible connect as a superuser
			if( $useRoot && $conf->DBtype != 'sqlite' ) {
				$wgDBsuperuser = $conf->RootUser;
				echo( "<li>Attempting to connect to database \"postgres\" as superuser \"$wgDBsuperuser\"..." );
				$wgDatabase = $dbc->newFromParams($wgDBserver, $wgDBsuperuser, $conf->RootPW, "postgres", 1);
				if (!$wgDatabase->isOpen()) {
					print " error: " . $wgDatabase->lastError() . "</li>\n";
					$errs["DBserver"] = "Could not connect to database as superuser";
					$errs["RootUser"] = "Check username";
					$errs["RootPW"] = "and password";
					continue;
				}
				$wgDatabase->initial_setup($conf->RootPW, 'postgres');
			}
			echo( "<li>Attempting to connect to database \"$wgDBname\" as \"$wgDBuser\"..." );
			$wgDatabase = $dbc->newFromParams($wgDBserver, $wgDBuser, $wgDBpassword, $wgDBname, 1);
			if (!$wgDatabase->isOpen()) {
				print " error: " . $wgDatabase->lastError() . "</li>\n";
			} else {
				$myver = $wgDatabase->getServerVersion();
			}
			if (is_callable(array($wgDatabase, 'initial_setup'))) $wgDatabase->initial_setup('', $wgDBname);
		} 

		if ( !$wgDatabase->isOpen() ) {
			$errs["DBserver"] = "Couldn't connect to database";
			continue;
		}

		print "<li>Connected to $myver";
		if ($conf->DBtype == 'mysql') {
			if( version_compare( $myver, "4.0.14" ) < 0 ) {
				print "</li>\n";
				dieout( "-- mysql 4.0.14 or later required. Aborting." );
			}
			$mysqlNewAuth = version_compare( $myver, "4.1.0", "ge" );
			if( $mysqlNewAuth && $mysqlOldClient ) {
				print "; <b class='error'>You are using MySQL 4.1 server, but PHP is linked
					to old client libraries; if you have trouble with authentication, see
					<a href='http://dev.mysql.com/doc/mysql/en/old-client.html'
					>http://dev.mysql.com/doc/mysql/en/old-client.html</a> for help.</b>";
			}
			if( $wgDBmysql5 ) {
				if( $mysqlNewAuth ) {
					print "; enabling MySQL 4.1/5.0 charset mode";
				} else {
					print "; <b class='error'>MySQL 4.1/5.0 charset mode enabled,
						but older version detected; will likely fail.</b>";
				}
			}
			print "</li>\n";

			@$sel = $wgDatabase->selectDB( $wgDBname );
			if( $sel ) {
				print "<li>Database <tt>" . htmlspecialchars( $wgDBname ) . "</tt> exists</li>\n";
			} else {
				$err = mysql_errno();
				$databaseSafe = htmlspecialchars( $wgDBname );
				if( $err == 1102 /* Invalid database name */ ) {
					print "<ul><li><strong>{$databaseSafe}</strong> is not a valid database name.</li></ul>";
					continue;
				} elseif( $err != 1049 /* Database doesn't exist */ ) {
					print "<ul><li>Error selecting database <strong>{$databaseSafe}</strong>: {$err} ";
					print htmlspecialchars( mysql_error() ) . "</li></ul>";
					continue;
				}
				print "<li>Attempting to create database...</li>";
				$res = $wgDatabase->query( "CREATE DATABASE `$wgDBname`" );
				if( !$res ) {
					print "<li>Couldn't create database <tt>" .
						htmlspecialchars( $wgDBname ) .
						"</tt>; try with root access or check your username/pass.</li>\n";
					$errs["RootPW"] = "&lt;- Enter";
					continue;
				}
				print "<li>Created database <tt>" . htmlspecialchars( $wgDBname ) . "</tt></li>\n";
			}
			$wgDatabase->selectDB( $wgDBname );
		}
		else if ($conf->DBtype == 'postgres') {
			if( version_compare( $myver, "PostgreSQL 8.0" ) < 0 ) {
				dieout( "<b>Postgres 8.0 or later is required</b>. Aborting." );
			}
		}

		if( $wgDatabase->tableExists( "cur" ) || $wgDatabase->tableExists( "revision" ) ) {
			print "<li>There are already MediaWiki tables in this database. Checking if updates are needed...</li>\n";

			if ( $conf->DBtype == 'mysql') {
				# Determine existing default character set
				if ( $wgDatabase->tableExists( "revision" ) ) {
					$revision = $wgDatabase->escapeLike( $conf->DBprefix . 'revision' );
					$res = $wgDatabase->query( "SHOW TABLE STATUS LIKE '$revision'" );
					$row = $wgDatabase->fetchObject( $res );
					if ( !$row ) {
						echo "<li>SHOW TABLE STATUS query failed!</li>\n";
						$existingSchema = false;
						$existingEngine = false;
					} else {
						if ( preg_match( '/^latin1/', $row->Collation ) ) {
							$existingSchema = 'mysql4';
						} elseif ( preg_match( '/^utf8/', $row->Collation ) ) {
							$existingSchema = 'mysql5';
						} elseif ( preg_match( '/^binary/', $row->Collation ) ) {
							$existingSchema = 'mysql5-binary';
						} else {
							$existingSchema = false;
							echo "<li><strong>Warning:</strong> Unrecognised existing collation</li>\n";
						}
						if ( isset( $row->Engine ) ) {
							$existingEngine = $row->Engine;
						} else {
							$existingEngine = $row->Type;
						}
					}
					if ( $existingSchema && $existingSchema != $conf->DBschema ) {
						print "<li><strong>Warning:</strong> you requested the {$conf->DBschema} schema, " .
							"but the existing database has the $existingSchema schema. This upgrade script ". 
							"can't convert it, so it will remain $existingSchema.</li>\n";
						$conf->setSchema( $existingSchema, $conf->DBengine );
					}
					if ( $existingEngine && $existingEngine != $conf->DBengine ) {
						print "<li><strong>Warning:</strong> you requested the {$conf->DBengine} storage " .
							"engine, but the existing database uses the $existingEngine engine. This upgrade " .
							"script can't convert it, so it will remain $existingEngine.</li>\n";
						$conf->setSchema( $conf->DBschema, $existingEngine );
					}
				}

				# Create user if required
				if ( $conf->Root ) {
					$conn = $dbc->newFromParams( $wgDBserver, $wgDBuser, $wgDBpassword, $wgDBname, 1 );
					if ( $conn->isOpen() ) {
						print "<li>DB user account ok</li>\n";
						$conn->close();
					} else {
						print "<li>Granting user permissions...";
						if( $mysqlOldClient && $mysqlNewAuth ) {
							print " <b class='error'>If the next step fails, see <a href='http://dev.mysql.com/doc/mysql/en/old-client.html'>http://dev.mysql.com/doc/mysql/en/old-client.html</a> for help.</b>";
						}
						print "</li>\n";
						dbsource( "../maintenance/users.sql", $wgDatabase );
					}
				}
			}
			print "</ul><pre>\n";
			chdir( ".." );
			flush();
			do_all_updates();
			chdir( "config" );
			print "</pre>\n";
			print "<ul><li>Finished update checks.</li>\n";
		} else {
			# Determine available storage engines if possible
			if ( $conf->DBtype == 'mysql' && version_compare( $myver, "4.1.2", "ge" ) ) {
				$res = $wgDatabase->query( 'SHOW ENGINES' );
				$found = false;
				while ( $row = $wgDatabase->fetchObject( $res ) ) {
					if ( $row->Engine == $conf->DBengine ) {
						$found = true;
						break;
					}
				}
				$wgDatabase->freeResult( $res );
				if ( !$found && $conf->DBengine != 'MyISAM' ) {
					echo "<li><strong>Warning:</strong> {$conf->DBengine} storage engine not available, " .
						"using MyISAM instead</li>\n";
					$conf->setSchema( $conf->DBschema, 'MyISAM' );
				}
			}

			# FIXME: Check for errors
			print "<li>Creating tables...";
			if ($conf->DBtype == 'mysql') {
				dbsource( "../maintenance/tables.sql", $wgDatabase );
				dbsource( "../maintenance/interwiki.sql", $wgDatabase );
			} elseif (is_callable(array($wgDatabase, 'setup_database'))) {
				$wgDatabase->setup_database();
			}
			else {
				$errs["DBtype"] = "Do not know how to handle database type '$conf->DBtype'";
				continue;
			}

			print " done.</li>\n";

			print "<li>Initializing statistics...</li>\n";
			$wgDatabase->insert( 'site_stats',
				array ( 'ss_row_id'        => 1,
						'ss_total_views'   => 0,
						'ss_total_edits'   => 1, # Main page first edit
						'ss_good_articles' => 0, # Main page is not a good article - no internal link
						'ss_total_pages'   => 1, # Main page
						'ss_users'         => $conf->SysopName ? 1 : 0, # Sysop account, if created
						'ss_admins'        => $conf->SysopName ? 1 : 0, # Sysop account, if created
						'ss_images'        => 0 ) );

			# Set up the "regular user" account *if we can, and if we need to*
			if( $conf->Root and $conf->DBtype == 'mysql') {
				# See if we need to
				$wgDatabase2 = $dbc->newFromParams( $wgDBserver, $wgDBuser, $wgDBpassword, $wgDBname, 1 );
				if( $wgDatabase2->isOpen() ) {
					# Nope, just close the test connection and continue
					$wgDatabase2->close();
					echo( "<li>User $wgDBuser exists. Skipping grants.</li>\n" );
				} else {
					# Yes, so run the grants
					echo( "<li>Granting user permissions to $wgDBuser on $wgDBname..." );
					dbsource( "../maintenance/users.sql", $wgDatabase );
					echo( "success.</li>\n" );
				}
			}

			if( $conf->SysopName ) {
				$u = User::newFromName( $conf->getSysopName() );
				if ( !$u ) {
					print "<li><strong class=\"error\">Warning:</strong> Skipped sysop account creation - invalid username!</li>\n";
				}
				else if ( 0 == $u->idForName() ) {
					$u->addToDatabase();
					$u->setPassword( $conf->getSysopPass() );
					$u->saveSettings();

					$u->addGroup( "sysop" );
					$u->addGroup( "bureaucrat" );

					print "<li>Created sysop account <tt>" .
						htmlspecialchars( $conf->SysopName ) . "</tt>.</li>\n";
				} else {
					print "<li>Could not create user - already exists!</li>\n";
				}
			} else {
				print "<li>Skipped sysop account creation, no name given.</li>\n";
			}

			$titleobj = Title::newFromText( wfMsgNoDB( "mainpage" ) );
			$article = new Article( $titleobj );
			$newid = $article->insertOn( $wgDatabase );
			$revision = new Revision( array(
				'page'      => $newid,
				'text'      => wfMsg( 'mainpagetext' ) . "\n\n" . wfMsgNoTrans( 'mainpagedocfooter' ),
				'comment'   => '',
				'user'      => 0,
				'user_text' => 'MediaWiki default',
				) );
			$revid = $revision->insertOn( $wgDatabase );
			$article->updateRevisionOn( $wgDatabase, $revision );
		}

		/* Write out the config file now that all is well */
		print "<li style=\"list-style: none\">\n";
		print "<p>Creating LocalSettings.php...</p>\n\n";
		$localSettings = "<" . "?php$endl$local";
		// Fix up a common line-ending problem (due to CVS on Windows)
		$localSettings = str_replace( "\r\n", "\n", $localSettings );
		$f = fopen( "LocalSettings.php", 'xt' );

		if( $f == false ) {
			print( "</li>\n" );
			dieout( "<p>Couldn't write out LocalSettings.php. Check that the directory permissions are correct and that there isn't already a file of that name here...</p>\n" .
			"<p>Here's the file that would have been written, try to paste it into place manually:</p>\n" .
			"<pre>\n" . htmlspecialchars( $localSettings ) . "</pre>\n" );
		}
		if(fwrite( $f, $localSettings ) ) {
			fclose( $f );
			print "<hr/>\n";
			writeSuccessMessage();
			print "</li>\n";
		} else {
			fclose( $f );
			dieout( "<p class='error'>An error occured while writing the config/LocalSettings.php file. Check user rights and disk space then try again.</p></li>\n" );
		}

	} while( false );
}

print "</ul>\n";
$mainListOpened = false;

if( count( $errs ) ) {
	/* Display options form */

	if( $conf->posted ) {
		echo "<p class='error-top'>Something's not quite right yet; make sure everything below is filled out correctly.</p>\n";
	}
?>

<form action="<?php echo defined('MW_INSTALL_PHP5_EXT') ? 'index.php5' : 'index.php'; ?>" name="config" method="post">

<h2>Site config</h2>

<div class="config-section">
	<div class="config-input">
		<?php aField( $conf, "Sitename", "Wiki name:" ); ?>
	</div>
	<p class="config-desc">
		Preferably a short word without punctuation, i.e. "Wikipedia".<br />
		Will appear as the namespace name for "meta" pages, and throughout the interface.
	</p>
	<div class="config-input"><?php aField( $conf, "EmergencyContact", "Contact e-mail:" ); ?></div>
	<p class="config-desc">
		Displayed to users in some error messages, used as the return address for password reminders, and used as the default sender address of e-mail notifications.
	</p>

	<div class="config-input">
		<label class='column' for="LanguageCode">Language:</label>
		<select id="LanguageCode" name="LanguageCode"><?php
			$list = getLanguageList();
			foreach( $list as $code => $name ) {
				$sel = ($code == $conf->LanguageCode) ? 'selected="selected"' : '';
				echo "\n\t\t<option value=\"$code\" $sel>$name</option>";
			}
			echo "\n";
		?>
		</select>
	</div>
	<p class="config-desc">
		Select the language for your wiki's interface. Some localizations aren't fully complete. Unicode (UTF-8) is used for all localizations.
	</p>

	<div class="config-input">
		<label class='column'>Copyright/license:</label>

		<ul class="plain">
		<li><?php aField( $conf, "License", "No license metadata", "radio", "none" ); ?></li>
		<li><?php aField( $conf, "License", "GNU Free Documentation License 1.2 (Wikipedia-compatible)", "radio", "gfdl" ); ?></li>
		<li><?php
			aField( $conf, "License", "A Creative Commons license - ", "radio", "cc" );
			$partner = "MediaWiki";
   $script = defined('MW_INSTALL_PHP5_EXT') ? 'index.php5' : 'index.php';
			$exit = urlencode( "$wgServer{$conf->ScriptPath}/config/$script?License=cc&RightsUrl=[license_url]&RightsText=[license_name]&RightsCode=[license_code]&RightsIcon=[license_button]" );
			$icon = urlencode( "$wgServer$wgUploadPath/wiki.png" );
			$ccApp = htmlspecialchars( "http://creativecommons.org/license/?partner=$partner&exit_url=$exit&partner_icon_url=$icon" );
			print "<a href=\"$ccApp\" target='_blank'>choose</a>";
			if( $conf->License == "cc" ) { ?>
			<ul>
			<li><?php aField( $conf, "RightsIcon", "<img src=\"" . htmlspecialchars( $conf->RightsIcon ) . "\" alt='(Creative Commons icon)' />", "hidden" ); ?></li>
			<li><?php aField( $conf, "RightsText", htmlspecialchars( $conf->RightsText ), "hidden" ); ?></li>
			<li><?php aField( $conf, "RightsCode", "code: " . htmlspecialchars( $conf->RightsCode ), "hidden" ); ?></li>
			<li><?php aField( $conf, "RightsUrl", "<a href=\"" . htmlspecialchars( $conf->RightsUrl ) . "\">" . htmlspecialchars( $conf->RightsUrl ) . "</a>", "hidden" ); ?></li>
			</ul>
			<?php } ?>
			</li>
		</ul>
	</div>
	<p class="config-desc">
		A notice, icon, and machine-readable copyright metadata will be displayed for the license you pick.
	</p>


	<div class="config-input">
		<?php aField( $conf, "SysopName", "Admin username:" ) ?>
	</div>
	<div class="config-input">
		<?php aField( $conf, "SysopPass", "Password:", "password" ) ?>
	</div>
	<div class="config-input">
		<?php aField( $conf, "SysopPass2", "Password confirm:", "password" ) ?>
	</div>
	<p class="config-desc">
		An admin can lock/delete pages, block users from editing, and do other maintenance tasks.<br />
		A new account will be added only when creating a new wiki database.
		<br /><br />
		The password cannot be the same as the username.
	</p>

	<div class="config-input">
		<label class='column'>Object caching:</label>

		<ul class="plain">
		<li><?php aField( $conf, "Shm", "No caching", "radio", "none" ); ?></li>
		<?php
			if ( $conf->turck ) {
				echo "<li>";
				aField( $conf, "Shm", "Turck MMCache", "radio", "turck" );
				echo "</li>\n";
			}
			if( $conf->xcache ) {
				echo "<li>";
				aField( $conf, 'Shm', 'XCache', 'radio', 'xcache' );
				echo "</li>\n";
			}
			if ( $conf->apc ) {
				echo "<li>";
				aField( $conf, "Shm", "APC", "radio", "apc" );
				echo "</li>\n";
			}
			if ( $conf->eaccel ) {
				echo "<li>";
				aField( $conf, "Shm", "eAccelerator", "radio", "eaccel" );
				echo "</li>\n";
			}
			if ( $conf->dba ) {
				echo "<li>";
				aField( $conf, "Shm", "DBA (not recommended)", "radio", "dba" );
				echo "</li>";
			}
		?>
		<li><?php aField( $conf, "Shm", "Memcached", "radio", "memcached" ); ?></li>
		</ul>
		<div style="clear:left"><?php aField( $conf, "MCServers", "Memcached servers:", "text" ) ?></div>
	</div>
	<p class="config-desc">
		An object caching system such as memcached will provide a significant performance boost,
		but needs to be installed. Provide the server addresses and ports in a comma-separated list.
		<br /><br />
		MediaWiki can also detect and support eAccelerator, Turck MMCache, APC, and XCache, but
		these should not be used if the wiki will be running on multiple application servers.
		<br/><br/>
		DBA (Berkeley-style DB) is generally slower than using no cache at all, and is only 
		recommended for testing.
	</p>
</div>

<h2>E-mail, e-mail notification and authentication setup</h2>

<div class="config-section">
	<div class="config-input">
		<label class='column'>E-mail features (global):</label>
		<ul class="plain">
		<li><?php aField( $conf, "Email", "Enabled", "radio", "email_enabled" ); ?></li>
		<li><?php aField( $conf, "Email", "Disabled", "radio", "email_disabled" ); ?></li>
		</ul>
	</div>
	<p class="config-desc">
		Use this to disable all e-mail functions (password reminders, user-to-user e-mail, and e-mail notifications)
		if sending mail doesn't work on your server.
	</p>

	<div class="config-input">
		<label class='column'>User-to-user e-mail:</label>
		<ul class="plain">
		<li><?php aField( $conf, "Emailuser", "Enabled", "radio", "emailuser_enabled" ); ?></li>
		<li><?php aField( $conf, "Emailuser", "Disabled", "radio", "emailuser_disabled" ); ?></li>
		</ul>
	</div>
	<p class="config-desc">
		The user-to-user e-mail feature (Special:Emailuser) lets the wiki act as a relay to allow users to exchange e-mail without publicly advertising their e-mail address.
	</p>
	<div class="config-input">
		<label class='column'>E-mail notification about changes:</label>
		<ul class="plain">
		<li><?php aField( $conf, "Enotif", "Disabled", "radio", "enotif_disabled" ); ?></li>
		<li><?php aField( $conf, "Enotif", "Enabled for changes to user discussion pages only", "radio", "enotif_usertalk" ); ?></li>
		<li><?php aField( $conf, "Enotif", "Enabled for changes to user discussion pages, and to pages on watchlists (not recommended for large wikis)", "radio", "enotif_allpages" ); ?></li>
		</ul>
	</div>
	<div class="config-desc">
		<p>
		For this feature to work, an e-mail address must be present for the user account, and the notification
		options in the user's preferences must be enabled. Also note the 
		authentication option below. When testing the feature, keep in mind that your own changes will never trigger notifications to be sent to yourself.</p>

		<p>There are additional options for fine tuning in /includes/DefaultSettings.php; copy these to your LocalSettings.php and edit them there to change them.</p>
	</div>

	<div class="config-input">
		<label class='column'>E-mail address authentication:</label>
		<ul class="plain">
		<li><?php aField( $conf, "Eauthent", "Disabled", "radio", "eauthent_disabled" ); ?></li>
		<li><?php aField( $conf, "Eauthent", "Enabled", "radio", "eauthent_enabled" ); ?></li>
		</ul>
	</div>
	<div class="config-desc">
		<p>If this option is enabled, users have to confirm their e-mail address using a magic link sent to them whenever they set or change it, and only authenticated e-mail addresses can receive mails from other users and/or
		change notification mails. Setting this option is <b>recommended</b> for public wikis because of potential abuse of the e-mail features above.</p>
	</div>

</div>

<h2>Database config</h2>

<div class="config-section">
<div class="config-input">
	<label class='column'>Database type:</label>
<?php if (isset($errs['DBpicktype'])) print "\t<span class='error'>$errs[DBpicktype]</span>\n"; ?>
	<ul class='plain'><?php 
		database_picker($conf); 
	?></ul>
	</div>

	<div class="config-input" style="clear:left">
	<?php aField( $conf, "DBserver", "Database host:" ); ?>
	</div>
	<p class="config-desc">
		If your database server isn't on your web server, enter the name or IP address here.
	</p>

	<div class="config-input"><?php aField( $conf, "DBname", "Database name:" ); ?></div>
	<div class="config-input"><?php aField( $conf, "DBuser", "DB username:" ); ?></div>
	<div class="config-input"><?php aField( $conf, "DBpassword", "DB password:", "password" ); ?></div>
	<div class="config-input"><?php aField( $conf, "DBpassword2", "DB password confirm:", "password" ); ?></div>
	<p class="config-desc">
		If you only have a single user account and database available,
		enter those here. If you have database root access (see below)
		you can specify new accounts/databases to be created. This account 
		will not be created if it pre-exists. If this is the case, ensure that it
		has SELECT, INSERT, UPDATE, and DELETE permissions on the MediaWiki database.
	</p>

	<div class="config-input">
		<label class="column">Superuser account:</label>
		<input type="checkbox" name="useroot" id="useroot" <?php if( $useRoot ) { ?>checked="checked" <?php } ?> />
		&nbsp;<label for="useroot">Use superuser account</label>
	</div>
	<div class="config-input"><?php aField( $conf, "RootUser", "Superuser name:", "text" ); ?></div>
	<div class="config-input"><?php aField( $conf, "RootPW", "Superuser password:", "password" ); ?></div>

	<p class="config-desc">
		If the database user specified above does not exist, or does not have access to create
		the database (if needed) or tables within it, please check the box and provide details
		of a superuser account,	such as <strong>root</strong>, which does.
	</p>

	<?php database_switcher('mysql'); ?>
	<div class="config-input"><?php aField( $conf, "DBprefix", "Database table prefix:" ); ?></div>
	<div class="config-desc">
		<p>If you need to share one database between multiple wikis, or
		between MediaWiki and another web application, you may choose to
		add a prefix to all the table names to avoid conflicts.</p>

		<p>Avoid exotic characters; something like <tt>mw_</tt> is good.</p>
	</div>

	<div class="config-input"><label class="column">Storage Engine</label>
		<div>Select one:</div>
		<ul class="plain">
		<li><?php aField( $conf, "DBengine", "InnoDB", "radio", "InnoDB" ); ?></li>
		<li><?php aField( $conf, "DBengine", "MyISAM", "radio", "MyISAM" ); ?></li>
		</ul>
	</div>
	<p class="config-desc">
		InnoDB is best for public web installations, since it has good concurrency 
		support. MyISAM may be faster in single-user installations. MyISAM databases 
		tend to get corrupted more often than InnoDB databases.
	</p>
	<div class="config-input"><label class="column">Database character set</label>
		<div>Select one:</div>
		<ul class="plain">
		<li><?php aField( $conf, "DBschema", "MySQL 4.1/5.0 binary", "radio", "mysql5-binary" ); ?></li>
		<li><?php aField( $conf, "DBschema", "MySQL 4.1/5.0 UTF-8", "radio", "mysql5" ); ?></li>
		<li><?php aField( $conf, "DBschema", "MySQL 4.0 backwards-compatible UTF-8", "radio", "mysql4" ); ?></li>
		</ul>
	</div>
	<p class="config-desc">
		This option is ignored on upgrade, the same character set will be kept. 
		<br/><br/>
		<b>WARNING:</b> If you use <b>backwards-compatible UTF-8</b> on MySQL 4.1+, and subsequently back up the database with <tt>mysqldump</tt>, it may destroy all non-ASCII characters, irreversibly corrupting your backups!.
		<br/><br/>
		In <b>binary mode</b>, MediaWiki stores UTF-8 text to the database in binary fields. This is more efficient than MySQL's UTF-8 mode, and allows you to use the full range of Unicode characters. In <b>UTF-8 mode</b>, MySQL will know what character set your data is in, and can present and convert it appropriately, but it won't let you store characters above the <a target="_blank" href="http://en.wikipedia.org/wiki/Mapping_of_Unicode_character_planes">Basic Multilingual Plane</a>.
	</p>
	</fieldset>

	<?php database_switcher('postgres'); ?>
	<div class="config-input"><?php aField( $conf, "DBport", "Database port:" ); ?></div>
	<div class="config-input"><?php aField( $conf, "DBmwschema", "Schema for mediawiki:" ); ?></div>
	<div class="config-input"><?php aField( $conf, "DBts2schema", "Schema for tsearch2:" ); ?></div>
	<div class="config-desc">
		<p>The username specified above (at "DB username") will have its search path set to the above schemas, 
		so it is recommended that you create a new user. The above schemas are generally correct: 
        only change them if you are sure you need to.</p>
	</div>
	</fieldset>

	<?php database_switcher('sqlite'); ?>
	<div class="config-desc">
		<b>NOTE:</b> SQLite only uses the <i>Database name</i> setting above, the user, password and root settings are ignored.
	</div>
	<div class="config-input"><?php
		aField( $conf, "SQLiteDataDir", "SQLite data directory:" );
	?></div>
	<div class="config-desc">
		<p>SQLite stores table data into files in the filesystem.
		If you do not provide an explicit path, a "data" directory in
		the parent of your document root will be used.</p>
		
		<p>This directory must exist and be writable by the web server.</p>
	</div>
	</fieldset>

	<?php database_switcher('mssql'); ?>
	<div class="config-input"><?php
		aField( $conf, "DBprefix2", "Database table prefix:" );
	?></div>
	<div class="config-desc">
		<p>If you need to share one database between multiple wikis, or
		between MediaWiki and another web application, you may choose to
		add a prefix to all the table names to avoid conflicts.</p>

		<p>Avoid exotic characters; something like <tt>mw_</tt> is good.</p>
	</div>
	</fieldset>

	<div class="config-input" style="padding:2em 0 3em">
		<label class='column'>&nbsp;</label>
		<input type="submit" value="Install MediaWiki!" class="btn-install" />
	</div>
</div>
</form>
<script type="text/javascript">
window.onload = toggleDBarea('<?php echo $conf->DBtype; ?>',
<?php
	## If they passed in a root user name, don't populate it on page load
	echo strlen(importPost('RootUser', '')) ? 0 : 1;
?>);
</script>
<?php
}

/* -------------------------------------------------------------------------------------- */
function writeSuccessMessage() {
 $script = defined('MW_INSTALL_PHP5_EXT') ? 'index.php5' : 'index.php';
	if ( wfIniGetBool( 'safe_mode' ) && !ini_get( 'open_basedir' ) ) {
		echo <<<EOT
<div class="success-box">
<p>Installation successful!</p>
<p>To complete the installation, please do the following:
<ol>
	<li>Download config/LocalSettings.php with your FTP client or file manager</li>
	<li>Upload it to the parent directory</li>
	<li>Delete config/LocalSettings.php</li>
	<li>Start using <a href='../$script'>your wiki</a>!
</ol>
<p>If you are in a shared hosting environment, do <strong>not</strong> just move LocalSettings.php
remotely. LocalSettings.php is currently owned by the user your webserver is running under,
which means that anyone on the same server can read your database password! Downloading
it and uploading it again will hopefully change the ownership to a user ID specific to you.</p>
</div>
EOT;
	} else {
		echo <<<EOT
<div class="success-box">
<p>
<span class="success-message">Installation successful!</span>
Move the <tt>config/LocalSettings.php</tt> file to the parent directory, then follow
<a href="../$script"> this link</a> to your wiki.</p>
<p>You should change file permissions for <tt>LocalSettings.php</tt> as required to
prevent other users on the server reading passwords and altering configuration data.</p>
</div>
EOT;
	}
}


function escapePhpString( $string ) {
	if ( is_array( $string ) || is_object( $string ) ) {
		return false;
	}
	return strtr( $string,
		array(
			"\n" => "\\n",
			"\r" => "\\r",
			"\t" => "\\t",
			"\\" => "\\\\",
			"\$" => "\\\$",
			"\"" => "\\\""
		));
}

function writeLocalSettings( $conf ) {
	$conf->PasswordSender = $conf->EmergencyContact;
	$magic = ($conf->ImageMagick ? "" : "# ");
	$convert = ($conf->ImageMagick ? $conf->ImageMagick : "/usr/bin/convert" );
	$rights = ($conf->RightsUrl) ? "" : "# ";
	$hashedUploads = $conf->safeMode ? '' : '# ';

	if ( $conf->ShellLocale ) {
		$locale = '';
	} else {
		$locale = '# ';
		$conf->ShellLocale = 'en_US.UTF-8';
	}

	switch ( $conf->Shm ) {
		case 'memcached':
			$cacheType = 'CACHE_MEMCACHED';
			$mcservers = var_export( $conf->MCServerArray, true );
			break;
		case 'turck':
		case 'xcache':
		case 'apc':
		case 'eaccel':
			$cacheType = 'CACHE_ACCEL';
			$mcservers = 'array()';
			break;
		case 'dba':
			$cacheType = 'CACHE_DBA';
			$mcservers = 'array()';
			break;
		default:
			$cacheType = 'CACHE_NONE';
			$mcservers = 'array()';
	}

	if ( $conf->Email == 'email_enabled' ) {
		$enableemail = 'true';
		$enableuseremail = ( $conf->Emailuser == 'emailuser_enabled' ) ? 'true' : 'false' ;
		$eauthent = ( $conf->Eauthent == 'eauthent_enabled' ) ? 'true' : 'false' ;
		switch ( $conf->Enotif ) {
			case 'enotif_usertalk':
				$enotifusertalk = 'true';
				$enotifwatchlist = 'false';
				break;
			case 'enotif_allpages':
				$enotifusertalk = 'true';
				$enotifwatchlist = 'true';
				break;
			default:
				$enotifusertalk = 'false';
				$enotifwatchlist = 'false';
		}
	} else {
		$enableuseremail = 'false';
		$enableemail = 'false';
		$eauthent = 'false';
		$enotifusertalk = 'false';
		$enotifwatchlist = 'false';
	}

	$file = @fopen( "/dev/urandom", "r" );
	if ( $file ) {
		$secretKey = bin2hex( fread( $file, 32 ) );
		fclose( $file );
	} else {
		$secretKey = "";
		for ( $i=0; $i<8; $i++ ) {
			$secretKey .= dechex(mt_rand(0, 0x7fffffff));
		}
		print "<li>Warning: \$wgSecretKey key is insecure, generated with mt_rand(). Consider changing it manually.</li>\n";
	}

	# Add slashes to strings for double quoting
	$slconf = array_map( "escapePhpString", get_object_vars( $conf ) );
	if( $conf->License == 'gfdl' ) {
		# Needs literal string interpolation for the current style path
		$slconf['RightsIcon'] = $conf->RightsIcon;
	}

	if( $conf->DBtype == 'mysql' ) {
		$dbsettings =
"# MySQL specific settings
\$wgDBprefix         = \"{$slconf['DBprefix']}\";

# MySQL table options to use during installation or update
\$wgDBTableOptions   = \"{$slconf['DBTableOptions']}\";

# Experimental charset support for MySQL 4.1/5.0.
\$wgDBmysql5 = {$conf->DBmysql5};";
	} elseif( $conf->DBtype == 'postgres' ) {
		$dbsettings =
"# Postgres specific settings
\$wgDBport           = \"{$slconf['DBport']}\";
\$wgDBmwschema       = \"{$slconf['DBmwschema']}\";
\$wgDBts2schema      = \"{$slconf['DBts2schema']}\";";
	} elseif( $conf->DBtype == 'sqlite' ) {
		$dbsettings =
"# SQLite-specific settings
\$wgSQLiteDataDir    = \"{$slconf['SQLiteDataDir']}\";";
	} elseif( $conf->DBtype == 'mssql' ) {
		$dbsettings =
"# MSSQL specific settings
\$wgDBprefix         = \"{$slconf['DBprefix2']}\";";
	} else {
		// ummm... :D
		$dbsettings = '';
	}


	$localsettings = "
# This file was automatically generated by the MediaWiki installer.
# If you make manual changes, please keep track in case you need to
# recreate them later.
#
# See includes/DefaultSettings.php for all configurable settings
# and their default values, but don't forget to make changes in _this_
# file, not there.
#
# Further documentation for configuration settings may be found at:
# http://www.mediawiki.org/wiki/Manual:Configuration_settings

# If you customize your file layout, set \$IP to the directory that contains
# the other MediaWiki files. It will be used as a base to locate files.
if( defined( 'MW_INSTALL_PATH' ) ) {
	\$IP = MW_INSTALL_PATH;
} else {
	\$IP = dirname( __FILE__ );
}

\$path = array( \$IP, \"\$IP/includes\", \"\$IP/languages\" );
set_include_path( implode( PATH_SEPARATOR, \$path ) . PATH_SEPARATOR . get_include_path() );

require_once( \"\$IP/includes/DefaultSettings.php\" );

# If PHP's memory limit is very low, some operations may fail.
" . ($conf->raiseMemory ? '' : '# ' ) . "ini_set( 'memory_limit', '20M' );" . "

if ( \$wgCommandLineMode ) {
	if ( isset( \$_SERVER ) && array_key_exists( 'REQUEST_METHOD', \$_SERVER ) ) {
		die( \"This script must be run from the command line\\n\" );
	}
}
## Uncomment this to disable output compression
# \$wgDisableOutputCompression = true;

\$wgSitename         = \"{$slconf['Sitename']}\";

## The URL base path to the directory containing the wiki;
## defaults for all runtime URL paths are based off of this.
## For more information on customizing the URLs please see:
## http://www.mediawiki.org/wiki/Manual:Short_URL
\$wgScriptPath       = \"{$slconf['ScriptPath']}\";
\$wgScriptExtension  = \"{$slconf['ScriptExtension']}\";

## UPO means: this is also a user preference option

\$wgEnableEmail      = $enableemail;
\$wgEnableUserEmail  = $enableuseremail; # UPO

\$wgEmergencyContact = \"{$slconf['EmergencyContact']}\";
\$wgPasswordSender = \"{$slconf['PasswordSender']}\";

\$wgEnotifUserTalk = $enotifusertalk; # UPO
\$wgEnotifWatchlist = $enotifwatchlist; # UPO
\$wgEmailAuthentication = $eauthent;

## Database settings
\$wgDBtype           = \"{$slconf['DBtype']}\";
\$wgDBserver         = \"{$slconf['DBserver']}\";
\$wgDBname           = \"{$slconf['DBname']}\";
\$wgDBuser           = \"{$slconf['DBuser']}\";
\$wgDBpassword       = \"{$slconf['DBpassword']}\";

{$dbsettings}

## Shared memory settings
\$wgMainCacheType = $cacheType;
\$wgMemCachedServers = $mcservers;

## To enable image uploads, make sure the 'images' directory
## is writable, then set this to true:
\$wgEnableUploads       = false;
{$magic}\$wgUseImageMagick = true;
{$magic}\$wgImageMagickConvertCommand = \"{$convert}\";

## If you use ImageMagick (or any other shell command) on a
## Linux server, this will need to be set to the name of an
## available UTF-8 locale
{$locale}\$wgShellLocale = \"{$slconf['ShellLocale']}\";

## If you want to use image uploads under safe mode,
## create the directories images/archive, images/thumb and
## images/temp, and make them all writable. Then uncomment
## this, if it's not already uncommented:
{$hashedUploads}\$wgHashedUploadDirectory = false;

## If you have the appropriate support software installed
## you can enable inline LaTeX equations:
\$wgUseTeX           = false;

\$wgLocalInterwiki   = \$wgSitename;

\$wgLanguageCode = \"{$slconf['LanguageCode']}\";

\$wgProxyKey = \"$secretKey\";

## Default skin: you can change the default skin. Use the internal symbolic
## names, ie 'standard', 'nostalgia', 'cologneblue', 'monobook':
\$wgDefaultSkin = 'monobook';

## For attaching licensing metadata to pages, and displaying an
## appropriate copyright notice / icon. GNU Free Documentation
## License and Creative Commons licenses are supported so far.
{$rights}\$wgEnableCreativeCommonsRdf = true;
\$wgRightsPage = \"\"; # Set to the title of a wiki page that describes your license/copyright
\$wgRightsUrl = \"{$slconf['RightsUrl']}\";
\$wgRightsText = \"{$slconf['RightsText']}\";
\$wgRightsIcon = \"{$slconf['RightsIcon']}\";
# \$wgRightsCode = \"{$slconf['RightsCode']}\"; # Not yet used

\$wgDiff3 = \"{$slconf['diff3']}\";

# When you make changes to this configuration file, this will make
# sure that cached pages are cleared.
\$wgCacheEpoch = max( \$wgCacheEpoch, gmdate( 'YmdHis', @filemtime( __FILE__ ) ) );
"; ## End of setting the $localsettings string

	// Keep things in Unix line endings internally;
	// the system will write out as local text type.
	return str_replace( "\r\n", "\n", $localsettings );
}

function dieout( $text ) {
	global $mainListOpened;
	if( $mainListOpened ) echo( "</ul>" );
	if( $text != '' && substr( $text, 0, 2 ) != '<p'  && substr( $text, 0, 2 ) != '<h' ){
		echo "<p>$text</p>\n";
	} else {
		echo $text;
	}
	die( "\n\n</div>\n</div>\n</div>\n</div>\n</body>\n</html>" );
}

function importVar( &$var, $name, $default = "" ) {
	if( isset( $var[$name] ) ) {
		$retval = $var[$name];
		if ( get_magic_quotes_gpc() ) {
			$retval = stripslashes( $retval );
		}
	} else {
		$retval = $default;
	}
	return $retval;
}

function importPost( $name, $default = "" ) {
	return importVar( $_POST, $name, $default );
}

function importCheck( $name ) {
	return isset( $_POST[$name] );
}

function importRequest( $name, $default = "" ) {
	return importVar( $_REQUEST, $name, $default );
}

$radioCount = 0;

function aField( &$conf, $field, $text, $type = "text", $value = "", $onclick = '' ) {
	global $radioCount;
	if( $type != "" ) {
		$xtype = "type=\"$type\"";
	} else {
		$xtype = "";
	}

	$id = $field;
	$nolabel = ($type == "radio") || ($type == "hidden");

	if ($type == 'radio')
		$id .= $radioCount++;

	if( !$nolabel ) {
		echo "<label class='column' for=\"$id\">$text</label>";
	}

	if( $type == "radio" && $value == $conf->$field ) {
		$checked = "checked='checked'";
	} else {
		$checked = "";
	}
	echo "<input $xtype name=\"$field\" id=\"$id\" class=\"iput-$type\" $checked ";
	if ($onclick) {
		echo " onclick='toggleDBarea(\"$value\",1)' " ;
	}
	echo "value=\"";
	if( $type == "radio" ) {
		echo htmlspecialchars( $value );
	} else {
		echo htmlspecialchars( $conf->$field );
	}


	echo "\" />";
	if( $nolabel ) {
		echo "<label for=\"$id\">$text</label>";
	}

	global $errs;
	if(isset($errs[$field])) echo "<span class='error'>" . $errs[$field] . "</span>\n";
}

function getLanguageList() {
	global $wgLanguageNames, $IP;
	if( !isset( $wgLanguageNames ) ) {
		require_once( "$IP/languages/Names.php" );
	}

	$codes = array();

	$d = opendir( "../languages/messages" );
	/* In case we are called from the root directory */
	if (!$d)
		$d = opendir( "languages/messages");
	while( false !== ($f = readdir( $d ) ) ) {
		$m = array();
		if( preg_match( '/Messages([A-Z][a-z_]+)\.php$/', $f, $m ) ) {
			$code = str_replace( '_', '-', strtolower( $m[1] ) );
			if( isset( $wgLanguageNames[$code] ) ) {
				$name = $code . ' - ' . $wgLanguageNames[$code];
			} else {
				$name = $code;
			}
			$codes[$code] = $name;
		}
	}
	closedir( $d );
	ksort( $codes );
	return $codes;
}

#Check for location of an executable
# @param string $loc single location to check
# @param array $names filenames to check for.
# @param mixed $versioninfo array of details to use when checking version, use false for no version checking
function locate_executable($loc, $names, $versioninfo = false) {
	if (!is_array($names))
		$names = array($names);

	foreach ($names as $name) {
		$command = "$loc".DIRECTORY_SEPARATOR."$name";
		if (@file_exists($command)) {
			if (!$versioninfo)
				return $command;

			$file = str_replace('$1', $command, $versioninfo[0]);
			if (strstr(`$file`, $versioninfo[1]) !== false)
				return $command;
		}
	}
	return false;
}

# Test a memcached server
function testMemcachedServer( $server ) {
	$hostport = explode(":", $server);
	$errstr = false;
	$fp = false;
	if ( !function_exists( 'fsockopen' ) ) {
		$errstr = "Can't connect to memcached, fsockopen() not present";
	}
	if ( !$errstr && count( $hostport ) != 2 ) {
		$errstr = 'Please specify host and port';
	}
	if ( !$errstr ) {
		list( $host, $port ) = $hostport;
		$errno = 0;
		$fsockerr = '';

		$fp = @fsockopen( $host, $port, $errno, $fsockerr, 1.0 );
		if ( $fp === false ) {
			$errstr = "Cannot connect to memcached on $host:$port : $fsockerr";
		}
	}
	if ( !$errstr ) {
		$command = "version\r\n";
		$bytes = fwrite( $fp, $command );
		if ( $bytes != strlen( $command ) ) {
			$errstr = "Cannot write to memcached socket on $host:$port";
		}
	}
	if ( !$errstr ) {
		$expected = "VERSION ";
		$response = fread( $fp, strlen( $expected ) );
		if ( $response != $expected ) {
			$errstr = "Didn't get correct memcached response from $host:$port";
		}
	}
	if ( $fp ) {
		fclose( $fp );
	}
	if ( !$errstr ) {
		echo "<li>Connected to memcached on $host:$port successfully";
	}
	return $errstr;
}

function database_picker($conf) {
	global $ourdb;
	print "\n";
	foreach(array_keys($ourdb) as $db) {
		if ($ourdb[$db]['havedriver']) {
			print "\t<li>";
			aField( $conf, "DBtype", $ourdb[$db]['fullname'], 'radio', $db, 'onclick');
			print "</li>\n";
		}
	}
	print "\n\t";
}

function database_switcher($db) {
	global $ourdb;
	$color = $ourdb[$db]['bgcolor'];
	$full = $ourdb[$db]['fullname'];
	print "<fieldset id='$db'><legend>$full specific options</legend>\n";
}

function printListItem( $item ) {
	print "<li>$item</li>";
}

# Determine a suitable value for $wgShellLocale
function getShellLocale( $wikiLanguage ) {
	# Give up now if we're in safe mode or open_basedir
	# It's theoretically possible but tricky to work with
	if ( wfIniGetBool( "safe_mode" ) || ini_get( 'open_basedir' ) ) {
		return false;
	}

	$os = php_uname( 's' );
	$supported = array( 'Linux', 'SunOS', 'HP-UX' ); # Tested these
	if ( !in_array( $os, $supported ) ) {
		return false;
	}

	# Get a list of available locales
	$lines = $ret = false;
	exec( '/usr/bin/locale -a', $lines, $ret );
	if ( $ret ) {
		return false;
	}

	$lines = array_map( 'trim', $lines );
	$candidatesByLocale = array();
	$candidatesByLang = array();
	foreach ( $lines as $line ) {
		if ( $line === '' ) {
			continue;
		}
		if ( !preg_match( '/^([a-zA-Z]+)(_[a-zA-Z]+|)\.(utf8|UTF-8)(@[a-zA-Z_]*|)$/i', $line, $m ) ) {
			continue;
		}
		list( $all, $lang, $territory, $charset, $modifier ) = $m;
		$candidatesByLocale[$m[0]] = $m;
		$candidatesByLang[$lang][] = $m;
	}

	# Try the current value of LANG
	if ( isset( $candidatesByLocale[ getenv( 'LANG' ) ] ) ) {
		return getenv( 'LANG' );
	}

	# Try the most common ones
	$commonLocales = array( 'en_US.UTF-8', 'en_US.utf8', 'de_DE.UTF-8', 'de_DE.utf8' );
	foreach ( $commonLocales as $commonLocale ) {
		if ( isset( $candidatesByLocale[$commonLocale] ) ) {
			return $commonLocale;
		}
	}

	# Is there an available locale in the Wiki's language?
	if ( isset( $candidatesByLang[$wikiLang] ) ) {
		$m = reset( $candidatesByLang[$wikiLang] );
		return $m[0];
	}

	# Are there any at all?
	if ( count( $candidatesByLocale ) ) {
		$m = reset( $candidatesByLocale );
		return $m[0];
	}

	# Give up
	return false;
}

?>

	<div class="license">
	<hr/>
	<p>This program is free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation; either version 2 of the License, or
	 (at your option) any later version.</p>

	 <p>This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.</p>

	 <p>You should have received <a href="../COPYING">a copy of the GNU General Public License</a>
	 along with this program; if not, write to the Free Software
	 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
	 or <a href="http://www.gnu.org/copyleft/gpl.html">read it online</a></p>
	</div>

</div></div></div>


<div id="column-one">
	<div class="portlet" id="p-logo">
	  <a style="background-image: url(../skins/common/images/mediawiki.png);"
	    href="http://www.mediawiki.org/"
	    title="Main Page"></a>
	</div>
	<script type="text/javascript"> if (window.isMSIE55) fixalpha(); </script>
	<div class='portlet'><div class='pBody'>
		<ul>
			<li><strong><a href="http://www.mediawiki.org/">MediaWiki home</a></strong></li>
			<li><a href="../README">Readme</a></li>
			<li><a href="../RELEASE-NOTES">Release notes</a></li>
			<li><a href="../docs/">Documentation</a></li>
			<li><a href="http://www.mediawiki.org/wiki/Help:Contents">User's Guide</a></li>
			<li><a href="http://www.mediawiki.org/wiki/Manual:Contents">Administrator's Guide</a></li>
			<li><a href="http://www.mediawiki.org/wiki/Manual:FAQ">FAQ</a></li>
		</ul>
		<p style="font-size:90%;margin-top:1em">MediaWiki is Copyright © 2001-2008 by Magnus Manske, Brion Vibber,
		 Lee Daniel Crocker, Tim Starling, Erik Möller, Gabriel Wicke, Ævar Arnfjörð Bjarmason, Niklas Laxström,
		 Domas Mituzas, Rob Church, Yuri Astrakhan, Aryeh Gregor, Aaron Schulz and others.</p>
	</div></div>
</div>

</div>

</body>
</html>
