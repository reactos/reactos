<?php
/**
 * Check a language file.
 *
 * @file
 * @ingroup MaintenanceLanguage
 */

require_once( dirname(__FILE__).'/../commandLine.inc' );
require_once( 'checkLanguage.inc' );
require_once( 'languages.inc' );

$cli = new CheckLanguageCLI( $options );
$cli->execute();
