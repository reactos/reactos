<?php
/**
 * Script to change users skins on the fly.
 * This is for at least MediaWiki 1.10alpha (r19611) and have not been
 * tested with previous versions. It should probably work with 1.7+.
 *
 * Made on an original idea by Fooey (freenode)
 *
 * @file
 * @ingroup Maintenance
 * @author Ashar Voultoiz <hashar@altern.org>
 */

// This is a command line script, load tools and parse args
require_once( 'userOptions.inc' );

// Load up our tool system, exit with usage() if options are not fine
$uo = new userOptions( $options, $args );

$uo->run();

print "Done.\n";

