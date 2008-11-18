<?php
/**
 * Convert from the old links schema (string->ID) to the new schema (ID->ID)
 * The wiki should be put into read-only mode while this script executes
 *
 * @file
 * @ingroup Maintenance
 */

/** */
require_once( "commandLine.inc" );
require_once( "convertLinks.inc" );

convertLinks();
