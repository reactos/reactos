<?php
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * Attempt to import existing log pages into the log tables.
 *
 * Not yet complete.
 *
 * @file
 * @todo document
 * @ingroup Maintenance
 */

/** */
require_once( 'GlobalFunctions.php' );
require_once( 'Database.php' );
require_once( 'Article.php' );
require_once( 'LogPage.php' );

/**
 * Log importer
 * @todo document
 * @ingroup Maintenance
 */
class LogImporter {
	var $dummy = false;

	function LogImporter( $type ) {
		$this->type = $type;
		$this->db = wfGetDB( DB_MASTER );
		$this->actions = $this->setupActions();
	}

	function setupActions() {
		$actions = array();
		foreach( LogPage::validActions( $this->type ) as $action ) {
			$key = "{$this->type}/$action";
			$actions[$key] = $this->makeLineRegexp( $this->type, $action );
		}
		return $actions;
	}

	function makeLineRegexp( $type, $action ) {
		$linkRegexp = '(?:\[\[)?([^|\]]+?)(?:\|[^\]]+?)?(?:\]\])?';
		$linkRegexp2 = '\[\[([^|\]]+?)(?:\|[^\]]+?)?\]\]';

		$text = LogPage::actionText( $type, $action );
		$text = preg_quote( $text, '/' );
		$text = str_replace( '\$1', $linkRegexp, $text );
		$text = '^(.*?) ' . $linkRegexp2 . ' ' . $text;
		$text .= '(?: <em>\((.*)\)<\/em>)?';
		$text = "/$text/";
		return $text;
	}

	function importText( $text ) {
		if( $this->dummy ) {
			print $text;
			var_dump( $this->actions );
		}
		$lines = explode( '<li>', $text );
		foreach( $lines as $line ) {
			$matches = array();
			if( preg_match( '!^(.*)</li>!', $line, $matches ) ) {
				$this->importLine( $matches[1] );
			}
		}
	}

	function fixDate( $date ) {
		# Yuck! Parsing multilingual date formats??!!!!???!!??!
		# 01:55, 23 Aug 2004 - won't take in strtotimr
		# "Aug 23 2004 01:55" - seems ok
		# TODO: multilingual attempt to extract from the data in Language
		$matches = array();
		if( preg_match( '/^(\d+:\d+(?::\d+)?), (.*)$/', $date, $matches ) ) {
			$date = $matches[2] . ' ' . $matches[1];
		}
		$n = strtotime( $date ) + date("Z");
		# print gmdate( 'D, d M Y H:i:s T', $n ) . "\n";
		$timestamp = wfTimestamp( TS_MW, $n );
		return $timestamp;
	}

	function importLine( $line ) {
		foreach( $this->actions as $action => $regexp ) {
			$matches = array();
			if( preg_match( $regexp, $line, $matches ) ) {
				if( $this->dummy ) {
					#var_dump( $matches );
				}
				$date = $this->fixDate( $matches[1] );
				$user = Title::newFromText( $matches[2] );
				$target = Title::newFromText( $matches[3] );
				if( isset( $matches[4] ) ) {
					$comment = $matches[4];
				} else {
					$comment = '';
				}

				$insert = array(
					'log_type' => $this->type,
					'log_action' => preg_replace( '!^.*/!', '', $action ),
					'log_timestamp' => $date,
					'log_user' => intval( User::idFromName( $user->getText() ) ),
					'log_namespace' => $target->getNamespace(),
					'log_title' => $target->getDBkey(),
					'log_comment' => wfUnescapeWikiText( $comment ),
				);
				if( $this->dummy ) {
					var_dump( $insert );
				} else {
					# FIXME: avoid duplicates!
					$this->db->insert( 'logging', $insert );
				}
				break;
			}
		}
	}
}

function wfUnescapeWikiText( $text ) {
	$text = str_replace(
		array( '&#91;', '&#124;', '&#39;', 'ISBN&#32;', '&#58;//' , "\n&#61;", '&#123;&#123;' ),
		array( '[',		'|',	  "'",	   'ISBN '	  , '://'	  , "\n=", '{{' ),
		$text );
	return $text;
}
