<?php
/**
 * Compress the text of a wiki.
 *
 * Usage:
 *
 * Non-wikimedia
 * php compressOld.php [options...]
 *
 * Wikimedia
 * php compressOld.php <database> [options...]
 *
 * Options are:
 *  -t <type>           set compression type to either:
 *                          gzip: compress revisions independently
 *                          concat: concatenate revisions and compress in chunks (default)
 *  -c <chunk-size>     maximum number of revisions in a concat chunk
 *  -b <begin-date>     earliest date to check for uncompressed revisions
 *  -e <end-date>       latest revision date to compress
 *  -s <start-id>       the old_id to start from
 *  -f <max-factor>     the maximum ratio of compressed chunk bytes to uncompressed avg. revision bytes
 *  -h <threshold>      is a minimum number of KB, where <max-factor> cuts in
 *  --extdb <cluster>   store specified revisions in an external cluster (untested)
 *
 * @file
 * @ingroup Maintenance ExternalStorage
 */

$optionsWithArgs = array( 't', 'c', 's', 'f', 'h', 'extdb', 'endid', 'e' );
require_once( dirname(__FILE__) . '/../commandLine.inc' );
require_once( "compressOld.inc" );

if( !function_exists( "gzdeflate" ) ) {
	print "You must enable zlib support in PHP to compress old revisions!\n";
	print "Please see http://www.php.net/manual/en/ref.zlib.php\n\n";
	wfDie();
}

$defaults = array(
	't' => 'concat',
	'c' => 20,
	's' => 0,
	'f' => 5,
	'h' => 100,
	'b' => '',
    'e' => '',
    'extdb' => '',
    'endid' => false,
);

$options = $options + $defaults;

if ( $options['t'] != 'concat' && $options['t'] != 'gzip' ) {
	print "Type \"{$options['t']}\" not supported\n";
}

if ( $options['extdb'] != '' ) {
	print "Compressing database $wgDBname to external cluster {$options['extdb']}\n" . str_repeat('-', 76) . "\n\n";
} else {
	print "Compressing database $wgDBname\n" . str_repeat('-', 76) . "\n\n";
}

$success = true;
if ( $options['t'] == 'concat' ) {
    $success = compressWithConcat( $options['s'], $options['c'], $options['f'], $options['h'], $options['b'],
        $options['e'], $options['extdb'], $options['endid'] );
} else {
	compressOldPages( $options['s'], $options['extdb'] );
}

if ( $success ) {
	print "Done.\n";
}

exit();


