<?php
/**
 * Alternate 1.4 -> 1.5 schema upgrade.
 * This does only the main tables + UTF-8 and is designed to allow upgrades to
 * interleave with other updates on the replication stream so that large wikis
 * can be upgraded without disrupting other services.
 *
 * Note: this script DOES NOT apply every update, nor will it probably handle
 * much older versions, etc.
 * Run this, FOLLOWED BY update.php, for upgrading from 1.4.5 release to 1.5.
 *
 * @file
 * @ingroup Maintenance
 */

$options = array( 'step', 'noimages' );

require_once( 'commandLine.inc' );
require_once( 'FiveUpgrade.inc' );

$upgrade = new FiveUpgrade();
$step = isset( $options['step'] ) ? $options['step'] : null;
$upgrade->upgrade( $step );


