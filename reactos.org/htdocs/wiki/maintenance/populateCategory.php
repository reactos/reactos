<?php
/**
 * @file 
 * @ingroup Maintenance
 * @author Simetrical
 */

$optionsWithArgs = array( 'begin', 'max-slave-lag', 'throttle' );

require_once "commandLine.inc";
require_once "populateCategory.inc";

if( isset( $options['help'] ) ) {
	echo <<<TEXT
This script will populate the category table, added in MediaWiki 1.13.  It will
print out progress indicators every 1000 categories it adds to the table.  The
script is perfectly safe to run on large, live wikis, and running it multiple
times is harmless.  You may want to use the throttling options if it's causing
too much load; they will not affect correctness.

If the script is stopped and later resumed, you can use the --begin option with
the last printed progress indicator to pick up where you left off.  This is
safe, because any newly-added categories before this cutoff will have been
added after the software update and so will be populated anyway.

When the script has finished, it will make a note of this in the database, and
will not run again without the --force option.

Usage:
    php populateCategory.php [--max-slave-lag <seconds>] [--begin <name>]
[--throttle <seconds>] [--force]

    --begin: Only do categories whose names are alphabetically after the pro-
vided name.  Default: empty (start from beginning).
    --max-slave-lag: If slave lag exceeds this many seconds, wait until it
drops before continuing.  Default: 10.
    --throttle: Wait this many milliseconds after each category.  Default: 0.
    --force: Run regardless of whether the database says it's been run already.
TEXT;
	exit( 0 );
}

$defaults = array(
	'begin' => '',
	'max-slave-lag' => 10,
	'throttle' => 0,
	'force' => false
);
$options = array_merge( $defaults, $options );

populateCategory( $options['begin'], $options['max-slave-lag'],
	$options['throttle'], $options['force'] );
