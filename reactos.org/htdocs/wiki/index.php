<?php

/**
 * This is the main web entry point for MediaWiki.
 *
 * If you are reading this in your web browser, your server is probably
 * not configured correctly to run PHP applications!
 *
 * See the README, INSTALL, and UPGRADE files for basic setup instructions
 * and pointers to the online documentation.
 *
 * http://www.mediawiki.org/
 *
 * ----------
 *
 * Copyright (C) 2001-2008 Magnus Manske, Brion Vibber, Lee Daniel Crocker,
 * Tim Starling, Erik Möller, Gabriel Wicke, Ævar Arnfjörð Bjarmason,
 * Niklas Laxström, Domas Mituzas, Rob Church, Yuri Astrakhan, Aryeh Gregor,
 * Aaron Schulz and others.
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
 */


# Initialise common code
$preIP = dirname( __FILE__ );
require_once( "$preIP/includes/WebStart.php" );

# Initialize MediaWiki base class
require_once( "$preIP/includes/Wiki.php" );
$mediaWiki = new MediaWiki();

wfProfileIn( 'main-misc-setup' );
OutputPage::setEncodings(); # Not really used yet

$maxLag = $wgRequest->getVal( 'maxlag' );
if ( !is_null( $maxLag ) ) {
	if ( !$mediaWiki->checkMaxLag( $maxLag ) ) {
		exit;
	}
}

# Query string fields
$action = $wgRequest->getVal( 'action', 'view' );
$title = $wgRequest->getVal( 'title' );

$wgTitle = $mediaWiki->checkInitialQueries( $title, $action );
if ($wgTitle == NULL) {
	unset( $wgTitle );
}

wfProfileOut( 'main-misc-setup' );

#
# Send Ajax requests to the Ajax dispatcher.
#
if ( $wgUseAjax && $action == 'ajax' ) {
	require_once( $IP . '/includes/AjaxDispatcher.php' );

	$dispatcher = new AjaxDispatcher();
	$dispatcher->performAction();
	$mediaWiki->restInPeace();
	exit;
}

# Setting global variables in mediaWiki
$mediaWiki->setVal( 'action', $action );
$mediaWiki->setVal( 'CommandLineMode', $wgCommandLineMode );
$mediaWiki->setVal( 'DisabledActions', $wgDisabledActions );
$mediaWiki->setVal( 'DisableHardRedirects', $wgDisableHardRedirects );
$mediaWiki->setVal( 'DisableInternalSearch', $wgDisableInternalSearch );
$mediaWiki->setVal( 'EnableCreativeCommonsRdf', $wgEnableCreativeCommonsRdf );
$mediaWiki->setVal( 'EnableDublinCoreRdf', $wgEnableDublinCoreRdf );
$mediaWiki->setVal( 'JobRunRate', $wgJobRunRate );
$mediaWiki->setVal( 'Server', $wgServer );
$mediaWiki->setVal( 'SquidMaxage', $wgSquidMaxage );
$mediaWiki->setVal( 'UseExternalEditor', $wgUseExternalEditor );
$mediaWiki->setVal( 'UsePathInfo', $wgUsePathInfo );

$mediaWiki->initialize( $wgTitle, $wgArticle, $wgOut, $wgUser, $wgRequest );
$mediaWiki->finalCleanup ( $wgDeferredUpdateList, $wgOut );

# Not sure when $wgPostCommitUpdateList gets set, so I keep this separate from finalCleanup
$mediaWiki->doUpdates( $wgPostCommitUpdateList );

$mediaWiki->restInPeace();

