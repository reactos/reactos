<?php
/**
 * @file
 * @ingroup MaintenanceLanguage
 */

define( 'MEDIAWIKI', 1 );
define( 'NOT_REALLY_MEDIAWIKI', 1 );

class Language {}
foreach ( glob( 'Language*.php' ) as $file ) {
	if ( $file != 'Language.php' ) {
		require_once( $file );
	}
}

$removedFunctions = array( 'date', 'time', 'timeanddate', 'formatMonth', 'formatDay', 
	'getMonthName', 'getMonthNameGen', 'getMonthAbbreviation', 'getWeekdayName', 
	'userAdjust', 'dateFormat', 'timeSeparator', 'timeDateSeparator', 'timeBeforeDate',
	'monthByLatinNumber', 'getSpecialMonthName',

	'commafy'
);

$numRemoved = 0;
$total = 0;
$classes = get_declared_classes();
ksort( $classes );
foreach ( $classes as $class ) {
	if ( !preg_match( '/^Language/', $class ) || $class == 'Language' || $class == 'LanguageConverter' ) {
		continue;
	}

	print "$class\n";
	$methods = get_class_methods( $class );
	print_r( $methods );

	if ( !count( array_diff( $methods, $removedFunctions ) ) ) {
		print "removed\n";
		$numRemoved++;
	}
	$total++;
	print "\n";
}

print "$numRemoved will be removed out of $total\n";


