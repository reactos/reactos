<?php
/**
 * Simple script that try to find documented hook and hooks actually
 * in the code and show what's missing.
 * 
 * This script assumes that:
 * - hooks names in hooks.txt are at the beginning of a line and single quoted.
 * - hooks names in code are the first parameter of wfRunHooks.
 *
 * if --online option is passed, the script will compare the hooks in the code
 * with the ones at http://www.mediawiki.org/wiki/Manual:Hooks
 *
 * Any instance of wfRunHooks that doesn't meet these parameters will be noted.
 *
 * @file
 * @ingroup Maintenance
 *
 * @author Ashar Voultoiz <hashar@altern.org>
 * @copyright Copyright © Ashar voultoiz
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public Licence 2.0 or later
 */

/** This is a command line script*/
include('commandLine.inc');

# GLOBALS

$doc = $IP . '/docs/hooks.txt';
$pathinc = array(
	$IP.'/',
	$IP.'/includes/',
	$IP.'/includes/api/',
	$IP.'/includes/db/',
	$IP.'/includes/filerepo/',
	$IP.'/includes/parser/',
	$IP.'/includes/specials/',
	$IP.'/languages/',
	$IP.'/maintenance/',
	$IP.'/skins/',
);

# FUNCTIONS

/**
 * @return array of documented hooks
 */
function getHooksFromDoc() {
	global $doc, $options;
	$m = array();
	if( isset( $options['online'] ) ){
		$content = Http::get( 'http://www.mediawiki.org/w/index.php?title=Manual:Hooks&action=raw' );
		preg_match_all( '/\[\[\/([a-zA-Z0-9-_:]+)\|/', $content, $m );
	} else {
		$content = file_get_contents( $doc );
		preg_match_all( "/\n'(.*?)'/", $content, $m );
	}
	return array_unique( $m[1] );
}

/**
 * Get hooks from a PHP file
 * @param $file Full filename to the PHP file.
 * @return array of hooks found.
 */
function getHooksFromFile( $file ) {
	$content = file_get_contents( $file );
	$m = array();
	preg_match_all( '/wfRunHooks\(\s*([\'"])(.*?)\1/', $content, $m);
	return $m[2];
}

/**
 * Get hooks from the source code.
 * @param $path Directory where the include files can be found
 * @return array of hooks found.
 */
function getHooksFromPath( $path ) {
	$hooks = array();
	if( $dh = opendir($path) ) {
		while(($file = readdir($dh)) !== false) {
			if( filetype($path.$file) == 'file' ) {
				$hooks = array_merge( $hooks, getHooksFromFile($path.$file) );
			}
		}
		closedir($dh);
	}
	return $hooks;
}

/**
 * Get bad hooks (where the hook name could not be determined) from a PHP file
 * @param $file Full filename to the PHP file.
 * @return array of bad wfRunHooks() lines
 */
function getBadHooksFromFile( $file ) {
	$content = file_get_contents( $file );
	$m = array();
	# We want to skip the "function wfRunHooks()" one.  :)
	preg_match_all( '/(?<!function )wfRunHooks\(\s*[^\s\'"].*/', $content, $m);
	$list = array();
	foreach( $m[0] as $match ){
		$list[] = $match . "(" . $file . ")";
	}
	return $list;
}

/**
 * Get bad hooks from the source code.
 * @param $path Directory where the include files can be found
 * @return array of bad wfRunHooks() lines
 */
function getBadHooksFromPath( $path ) {
	$hooks = array();
	if( $dh = opendir($path) ) {
		while(($file = readdir($dh)) !== false) {
			# We don't want to read this file as it contains bad calls to wfRunHooks()
			if( filetype( $path.$file ) == 'file' && !$path.$file == __FILE__ ) {
				$hooks = array_merge( $hooks, getBadHooksFromFile($path.$file) );
			}
		}
		closedir($dh);
	}
	return $hooks;
}

/**
 * Nicely output the array
 * @param $msg A message to show before the value
 * @param $arr An array
 * @param $sort Boolean : wheter to sort the array (Default: true)
 */
function printArray( $msg, $arr, $sort = true ) {
	if($sort) asort($arr); 
	foreach($arr as $v) echo "$msg: $v\n";
}

# MAIN

$documented = getHooksFromDoc($doc);
$potential = array();
$bad = array();
foreach( $pathinc as $dir ) {
	$potential = array_merge( $potential, getHooksFromPath( $dir ) );
	$bad = array_merge( $bad, getBadHooksFromPath( $dir ) );
}

$potential = array_unique( $potential );
$bad = array_unique( $bad );
$todo = array_diff( $potential, $documented );
$deprecated = array_diff( $documented, $potential );

// let's show the results:
printArray('undocumented', $todo );
printArray('not found', $deprecated );
printArray('unclear hook calls', $bad );
 
if ( count( $todo ) == 0 && count( $deprecated ) == 0 && count( $bad ) == 0 ) 
	echo "Looks good!\n";