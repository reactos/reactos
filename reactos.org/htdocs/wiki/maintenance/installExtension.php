<?php
/**
 * Copyright (C) 2006 Daniel Kinzler, brightbyte.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'target', 'repository', 'repos' );

require_once( 'commandLine.inc' );

define('EXTINST_NOPATCH', 0);
define('EXTINST_WRITEPATCH', 6);
define('EXTINST_HOTPATCH', 10);

/**
 * @ingroup Maintenance
 */
class InstallerRepository {
	var $path;
	
	function InstallerRepository( $path ) {
		$this->path = $path;
	}

	function printListing( ) {
		trigger_error( 'override InstallerRepository::printListing()', E_USER_ERROR );
	}        

	function getResource( $name ) {
		trigger_error( 'override InstallerRepository::getResource()', E_USER_ERROR );
	}        
	
	static function makeRepository( $path, $type = NULL ) {
		if ( !$type ) {
			$m = array();
			preg_match( '!(([-+\w]+)://)?.*?(\.[-\w\d.]+)?$!', $path, $m );
			$proto = @$m[2];
			
			if ( !$proto ) {
				$type = 'dir';
			} else if ( ( $proto == 'http' || $proto == 'https' ) && preg_match( '!([^\w]svn|svn[^\w])!i', $path) ) {
				$type = 'svn'; #HACK!
			} else  {
				$type = $proto;
			}
		}
		
		if ( $type == 'dir' || $type == 'file' ) { return new LocalInstallerRepository( $path ); }
		else if ( $type == 'http' || $type == 'http' ) { return new WebInstallerRepository( $path ); }
		else { return new SVNInstallerRepository( $path ); }
	}
}

/**
 * @ingroup Maintenance
 */
class LocalInstallerRepository extends InstallerRepository {

	function LocalInstallerRepository ( $path ) {
		InstallerRepository::InstallerRepository( $path );
	}

	function printListing( ) {
		$ff = glob( "{$this->path}/*" );
		if ( $ff === false || $ff === NULL ) {
			ExtensionInstaller::error( "listing directory {$this->path} failed!" );
			return false;
		}
		
		foreach ( $ff as $f ) {
			$n = basename($f);
			
			if ( !is_dir( $f ) ) {
				$m = array();
				if ( !preg_match( '/(.*)\.(tgz|tar\.gz|zip)/', $n, $m ) ) continue;
				$n = $m[1];
			}

			print "\t$n\n";
		}
	}        

	function getResource( $name ) {
		$path = $this->path . '/' . $name;

		if ( !file_exists( $path ) || !is_dir( $path ) ) $path = $this->path . '/' . $name . '.tgz';
		if ( !file_exists( $path ) ) $path = $this->path . '/' . $name . '.tar.gz';
		if ( !file_exists( $path ) ) $path = $this->path . '/' . $name . '.zip';

		return new LocalInstallerResource( $path );
	}        
}

/**
 * @ingroup Maintenance
 */
class WebInstallerRepository extends InstallerRepository {

	function WebInstallerRepository ( $path ) {
		InstallerRepository::InstallerRepository( $path );
	}

	function printListing( ) {
		ExtensionInstaller::note( "listing index from {$this->path}..." );
		
		$txt = @file_get_contents( $this->path . '/index.txt' );
		if ( $txt ) {
			print $txt;
			print "\n";
		}
		else {
			$txt = file_get_contents( $this->path );
			if ( !$txt ) {
				ExtensionInstaller::error( "listing index from {$this->path} failed!" );
				print ( $txt );
				return false;
			}

			$m = array();
			$ok = preg_match_all( '!<a\s[^>]*href\s*=\s*['."'".'"]([^/'."'".'"]+)\.tgz['."'".'"][^>]*>.*?</a>!si', $txt, $m, PREG_SET_ORDER ); 
			if ( !$ok ) {
				ExtensionInstaller::error( "listing index from {$this->path} does not match!" );
				print ( $txt );
				return false;
			}
			
			foreach ( $m as $l ) {
				$n = $l[1];
				print "\t$n\n";
			}
		}
	}        

	function getResource( $name ) {
		$path = $this->path . '/' . $name . '.tgz';
		return new WebInstallerResource( $path );
	}        
}

/**
 * @ingroup Maintenance
 */
class SVNInstallerRepository extends InstallerRepository {

	function SVNInstallerRepository ( $path ) {
		InstallerRepository::InstallerRepository( $path );
	}

	function printListing( ) {
		ExtensionInstaller::note( "SVN list {$this->path}..." );
		$code = null; // Shell Exec return value.
		$txt = wfShellExec( 'svn ls ' . escapeshellarg( $this->path ), $code );
		if ( $code !== 0 ) {
			ExtensionInstaller::error( "svn list for {$this->path} failed!" );
			return false;
		}
		
		$ll = preg_split('/(\s*[\r\n]\s*)+/', $txt);
		
		foreach ( $ll as $line ) {
			$m = array();
			if ( !preg_match('!^(.*)/$!', $line, $m) ) continue;
			$n = $m[1];
			          
			print "\t$n\n";
		}
	}        

	function getResource( $name ) {
		$path = $this->path . '/' . $name;
		return new SVNInstallerResource( $path );
	}        
}

/**
 * @ingroup Maintenance
 */
class InstallerResource {
	var $path;
	var $isdir;
	var $islocal;
	
	function InstallerResource( $path, $isdir, $islocal ) {
		$this->path = $path;
		
		$this->isdir= $isdir;
		$this->islocal = $islocal;

		$m = array();
		preg_match( '!([-+\w]+://)?.*?(\.[-\w\d.]+)?$!', $path, $m );

		$this->protocol = @$m[1];
		$this->extensions = @$m[2];

		if ( $this->extensions ) $this->extensions = strtolower( $this->extensions );
	}

	function fetch( $target ) {
		trigger_error( 'override InstallerResource::fetch()', E_USER_ERROR );
	}        

	function extract( $file, $target ) {
		
		if ( $this->extensions == '.tgz' || $this->extensions == '.tar.gz' ) { #tgz file
			ExtensionInstaller::note( "extracting $file..." );
			$code = null; // shell Exec return value.
			wfShellExec( 'tar zxvf ' . escapeshellarg( $file ) . ' -C ' . escapeshellarg( $target ), $code );
			
			if ( $code !== 0 ) {
				ExtensionInstaller::error( "failed to extract $file!" );
				return false;
			}
		}
		else if ( $this->extensions == '.zip' ) { #zip file
			ExtensionInstaller::note( "extracting $file..." );
			$code = null; // shell Exec return value.
			wfShellExec( 'unzip ' . escapeshellarg( $file ) . ' -d ' . escapeshellarg( $target ) , $code );
			
			if ( $code !== 0 ) {
				ExtensionInstaller::error( "failed to extract $file!" );
				return false;
			}
		}
		else { 
			ExtensionInstaller::error( "unknown extension {$this->extensions}!" );
			return false;
		}

		return true;
	}        

	/*static*/ function makeResource( $url ) {
		$m = array();
		preg_match( '!(([-+\w]+)://)?.*?(\.[-\w\d.]+)?$!', $url, $m );
		$proto = @$m[2];
		$ext = @$m[3];
		if ( $ext ) $ext = strtolower( $ext );
		
		if ( !$proto ) { return new LocalInstallerResource( $url, $ext ? false : true ); }
		else if ( $ext && ( $proto == 'http' || $proto == 'http' || $proto == 'ftp' ) ) { return new WebInstallerResource( $url ); }
		else { return new SVNInstallerResource( $url ); }
	}
}

/**
 * @ingroup Maintenance
 */
class LocalInstallerResource extends InstallerResource {
	function LocalInstallerResource( $path ) {
		InstallerResource::InstallerResource( $path, is_dir( $path ), true );
	}
        
	function fetch( $target ) {
		if ( $this->isdir ) return ExtensionInstaller::copyDir( $this->path, dirname( $target ) );
		else return $this->extract( $this->path, dirname( $target ) );
	}
        
}

/**
 * @ingroup Maintenance
 */
class WebInstallerResource extends InstallerResource {
	function WebInstallerResource( $path ) {
		InstallerResource::InstallerResource( $path, false, false );
	}
        
	function fetch( $target ) {
		$tmp = wfTempDir() . '/' . basename( $this->path );
		
		ExtensionInstaller::note( "downloading {$this->path}..." );
		$ok = copy( $this->path, $tmp );
		
		if ( !$ok ) {
			ExtensionInstaller::error( "failed to download {$this->path}" );
			return false;
		}
		
		$this->extract( $tmp, dirname( $target ) );
		unlink($tmp);
		
		return true;
	}        
}

/**
 * @ingroup Maintenance
 */
class SVNInstallerResource extends InstallerResource {
	function SVNInstallerResource( $path ) {
		InstallerResource::InstallerResource( $path, true, false );
	}
        
	function fetch( $target ) {
		ExtensionInstaller::note( "SVN checkout of {$this->path}..." );
		$code = null; // shell exec return val.
		wfShellExec( 'svn co ' . escapeshellarg( $this->path ) . ' ' . escapeshellarg( $target ), $code );

		if ( $code !== 0 ) {
			ExtensionInstaller::error( "checkout failed for {$this->path}!" );
			return false;
		}
		
		return true;
	}        
}

/**
 * @ingroup Maintenance
 */
class ExtensionInstaller {
	var $source;
	var $target;
	var $name;
	var $dir;
	var $tasks;

	function ExtensionInstaller( $name, $source, $target ) {
		if ( !is_object( $source ) ) $source = InstallerResource::makeResource( $source );

		$this->name = $name;
		$this->source = $source;
		$this->target = realpath( $target );
		$this->extdir = "$target/extensions";
		$this->dir = "{$this->extdir}/$name";
		$this->incpath = "extensions/$name";
		$this->tasks = array();
		
		#TODO: allow a subdir different from "extensions"
		#TODO: allow a config file different from "LocalSettings.php"
	}

	static function note( $msg ) {
		print "$msg\n";
	}

	static function warn( $msg ) {
		print "WARNING: $msg\n";
	}

	static function error( $msg ) {
		print "ERROR: $msg\n";
	}

	function prompt( $msg ) {
		if ( function_exists( 'readline' ) ) {
			$s = readline( $msg );
		}
		else {
			if ( !@$this->stdin ) $this->stdin = fopen( 'php://stdin', 'r' );
			if ( !$this->stdin ) die( "Failed to open stdin for user interaction!\n" );
			
			print $msg;
			flush();
			
			$s = fgets( $this->stdin );
		}
		
		$s = trim( $s );
		return $s;                
	}

	function confirm( $msg ) {
		while ( true ) {        
			$s = $this->prompt( $msg . " [yes/no]: ");
			$s = strtolower( trim($s) );
			
			if ( $s == 'yes' || $s == 'y' ) { return true; }
			else if ( $s == 'no' || $s == 'n' ) { return false; }
			else { print "bad response: $s\n"; }
		}
	}

	function deleteContents( $dir ) {
		$ff = glob( $dir . "/*" );
		if ( !$ff ) return;

		foreach ( $ff as $f ) {
			if ( is_dir( $f ) && !is_link( $f ) ) $this->deleteContents( $f );
			unlink( $f );
		}
	}
        
	function copyDir( $dir, $tgt ) {
		$d = $tgt . '/' . basename( $dir );
		
		if ( !file_exists( $d ) ) {
			$ok = mkdir( $d );
			if ( !$ok ) {
				ExtensionInstaller::error( "failed to create director $d" );
				return false;
			}
		}

		$ff = glob( $dir . "/*" );
		if ( $ff === false || $ff === NULL ) return false;

		foreach ( $ff as $f ) {
			if ( is_dir( $f ) && !is_link( $f ) ) {
				$ok = ExtensionInstaller::copyDir( $f, $d );
				if ( !$ok ) return false;
			}
			else {
				$t = $d . '/' . basename( $f );
				$ok = copy( $f, $t );

				if ( !$ok ) {
					ExtensionInstaller::error( "failed to copy $f to $t" );
					return false;
				}
			}
		}
		
		return true;
	}

	function setPermissions( $dir, $dirbits, $filebits ) {
		if ( !chmod( $dir, $dirbits ) ) ExtensionInstaller::warn( "faield to set permissions for $dir" );
        
		$ff = glob( $dir . "/*" );
		if ( $ff === false || $ff === NULL ) return false;

		foreach ( $ff as $f ) {
			$n= basename( $f );
			if ( $n{0} == '.' ) continue; #HACK: skip dot files
			
			if ( is_link( $f ) ) continue; #skip link
			
			if ( is_dir( $f ) ) {
				ExtensionInstaller::setPermissions( $f, $dirbits, $filebits );
			}
			else {
				if ( !chmod( $f, $filebits ) ) ExtensionInstaller::warn( "faield to set permissions for $f" );
			}
		}
		
		return true;
	}

	function fetchExtension( ) {
		if ( $this->source->islocal && $this->source->isdir && realpath( $this->source->path ) === $this->dir ) {
			$this->note( "files are already in the extension dir" );
			return true;
		}

		if ( file_exists( $this->dir ) && glob( $this->dir . "/*" ) ) {
			if ( $this->confirm( "{$this->dir} exists and is not empty.\nDelete all files in that directory?" ) ) {
				$this->deleteContents( $this->dir );
			}                        
			else {
				return false;
			}                        
		}

		$ok = $this->source->fetch( $this->dir );
		if ( !$ok ) return false;

		if ( !file_exists( $this->dir ) && glob( $this->dir . "/*" ) ) {
			$this->error( "{$this->dir} does not exist or is empty. Something went wrong, sorry." );
			return false;
		}

		if ( file_exists( $this->dir . '/README' ) ) $this->tasks[] = "read the README file in {$this->dir}";
		if ( file_exists( $this->dir . '/INSTALL' ) ) $this->tasks[] = "read the INSTALL file in {$this->dir}";
		if ( file_exists( $this->dir . '/RELEASE-NOTES' ) ) $this->tasks[] = "read the RELEASE-NOTES file in {$this->dir}";

		#TODO: configure this smartly...?
		$this->setPermissions( $this->dir, 0755, 0644 );

		$this->note( "fetched extension to {$this->dir}" );
		return true;
	}

	function patchLocalSettings( $mode ) {
		#NOTE: if we get a better way to hook up extensions, that should be used instead.

		$f = $this->dir . '/install.settings';
		$t = $this->target . '/LocalSettings.php';
		
		#TODO: assert version ?!
		#TODO: allow custom installer scripts + sql patches
		
		if ( !file_exists( $f ) ) {
			self::warn( "No install.settings file provided!" );
			$this->tasks[] = "Please read the instructions and edit LocalSettings.php manually to activate the extension.";
			return '?';
		}
		else {
			self::note( "applying settings patch..." );
		}
		
		$settings = file_get_contents( $f );
		                
		if ( !$settings ) {
			self::error( "failed to read settings from $f!" );
			return false;
		}
		                
		$settings = str_replace( '{{path}}', $this->incpath, $settings );
		
		if ( $mode == EXTINST_NOPATCH ) {
			$this->tasks[] = "Please put the following into your LocalSettings.php:" . "\n$settings\n";
			self::note( "Skipping patch phase, automatic patching is off." );
			return true;
		}
		
		if ( $mode == EXTINST_HOTPATCH ) {
			#NOTE: keep php extension for backup file!
			$bak = $this->target . '/LocalSettings.install-' . $this->name . '-' . wfTimestamp(TS_MW) . '.bak.php';
			                
			$ok = copy( $t, $bak );
			                
			if ( !$ok ) {
				self::warn( "failed to create backup of LocalSettings.php!" );
				return false;
			}
			else {
				self::note( "created backup of LocalSettings.php at $bak" );
			}
		}
		                
		$localsettings = file_get_contents( $t );
		                
		if ( !$settings ) {
			self::error( "failed to read $t for patching!" );
			return false;
		}
		                
		$marker = "<@< extension {$this->name} >@>";
		$blockpattern = "/\n\s*#\s*BEGIN\s*$marker.*END\s*$marker\s*/smi";
		
		if ( preg_match( $blockpattern, $localsettings ) ) {
			$localsettings = preg_replace( $blockpattern, "\n", $localsettings );
			$this->warn( "removed old configuration block for extension {$this->name}!" );
		}
		
		$newblock= "\n# BEGIN $marker\n$settings\n# END $marker\n";
		
		$localsettings = preg_replace( "/\?>\s*$/si", "$newblock?>", $localsettings );
		
		if ( $mode != EXTINST_HOTPATCH ) {
			$t = $this->target . '/LocalSettings.install-' . $this->name . '-' . wfTimestamp(TS_MW) . '.php';
		}
		
		$ok = file_put_contents( $t, $localsettings );
		
		if ( !$ok ) {
			self::error( "failed to patch $t!" );
			return false;
		}
		else if ( $mode == EXTINST_HOTPATCH ) {
			self::note( "successfully patched $t" );
		}
		else  {
			self::note( "created patched settings file $t" );
			$this->tasks[] = "Replace your current LocalSettings.php with ".basename($t);
		}
		
		return true;
	}

	function printNotices( ) {
		if ( !$this->tasks ) {
			$this->note( "Installation is complete, no pending tasks" );
		}
		else {
			$this->note( "" );
			$this->note( "PENDING TASKS:" );
			$this->note( "" );
			   
			foreach ( $this->tasks as $t ) {
				$this->note ( "* " . $t );
			}
			
			$this->note( "" );
		}
		
		return true;
	}
        
}

$tgt = isset ( $options['target'] ) ? $options['target'] : $IP;

$repos = @$options['repository'];
if ( !$repos ) $repos = @$options['repos'];
if ( !$repos ) $repos = @$wgExtensionInstallerRepository;

if ( !$repos && file_exists("$tgt/.svn") && is_dir("$tgt/.svn") ) {
	$svn = file_get_contents( "$tgt/.svn/entries" );
	
	$m = array();
	if ( preg_match( '!url="(.*?)"!', $svn, $m ) ) {
		$repos = dirname( $m[1] ) . '/extensions';
	}
}

if ( !$repos ) $repos = 'http://svn.wikimedia.org/svnroot/mediawiki/trunk/extensions';

if( !isset( $args[0] ) && !@$options['list'] ) {
	die( "USAGE: installExtension.php [options] <name> [source]\n" .
		"OPTIONS: \n" . 
		"    --list            list available extensions. <name> is ignored / may be omitted.\n" .
		"    --repository <n>  repository to fetch extensions from. May be a local directoy,\n" .
		"                      an SVN repository or a HTTP directory\n" .
		"    --target <dir>    mediawiki installation directory to use\n" .
		"    --nopatch         don't create a patched LocalSettings.php\n" .
		"    --hotpatch        patched LocalSettings.php directly (creates a backup)\n" .
		"SOURCE: specifies the package source directly. If given, the repository is ignored.\n" . 
		"        The source my be a local file (tgz or zip) or directory, the URL of a\n" .
		"        remote file (tgz or zip), or a SVN path.\n" 
                );
}

$repository = InstallerRepository::makeRepository( $repos );

if ( isset( $options['list'] ) ) {
	$repository->printListing();
	exit(0);
}

$name = $args[0];

$src = isset( $args[1] ) ? $args[1] : $repository->getResource( $name );

#TODO: detect $source mismatching $name !!

$mode = EXTINST_WRITEPATCH;
if ( isset( $options['nopatch'] ) || @$wgExtensionInstallerNoPatch ) { $mode = EXTINST_NOPATCH; }
else if ( isset( $options['hotpatch'] ) || @$wgExtensionInstallerHotPatch ) { $mode = EXTINST_HOTPATCH; }

if ( !file_exists( "$tgt/LocalSettings.php" ) ) {
	die("can't find $tgt/LocalSettings.php\n");
}

if ( $mode == EXTINST_HOTPATCH && !is_writable( "$tgt/LocalSettings.php" ) ) {
	die("can't write to  $tgt/LocalSettings.php\n");
}

if ( !file_exists( "$tgt/extensions" ) ) {
	die("can't find $tgt/extensions\n");
}

if ( !is_writable( "$tgt/extensions" ) ) {
	die("can't write to  $tgt/extensions\n");
}

$installer = new ExtensionInstaller( $name, $src, $tgt );

$installer->note( "Installing extension {$installer->name} from {$installer->source->path} to {$installer->dir}" );

print "\n";
print "\tTHIS TOOL IS EXPERIMENTAL!\n";
print "\tEXPECT THE UNEXPECTED!\n";
print "\n";

if ( !$installer->confirm("continue") ) die("aborted\n");

$ok = $installer->fetchExtension();

if ( $ok ) $ok = $installer->patchLocalSettings( $mode );

if ( $ok ) $ok = $installer->printNotices();

if ( $ok ) $installer->note( "$name extension installed." );

