<?php
/**
 * Dumb program that tries to get the memory usage
 * for each language file.
 *
 * @file
 * @ingroup MaintenanceLanguage
 */

/** This is a command line script */
require_once(dirname(__FILE__).'/../commandLine.inc');
require_once(dirname(__FILE__).'/languages.inc');

$langtool = new languages();

if ( ! function_exists( 'memory_get_usage' ) )
	wfDie( "You must compile PHP with --enable-memory-limit\n" );

$memlast = $memstart = memory_get_usage();

print 'Base memory usage: '.$memstart."\n";

foreach ( $langtool->getLanguages() as $langcode ) {
	Language::factory( $langcode );
	$memstep = memory_get_usage();
	printf( "%12s: %d\n", $langcode, ($memstep- $memlast) );
	$memlast = $memstep;
}

$memend = memory_get_usage();

echo ' Total Usage: '.($memend - $memstart)."\n";

