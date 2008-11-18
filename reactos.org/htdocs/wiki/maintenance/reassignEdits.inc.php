<?php

/**
 * Support functions for the reassignEdits script
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 * @licence GNU General Public Licence 2.0 or later
 */

/**
 * Reassign edits from one user to another
 *
 * @param $from User to take edits from
 * @param $to User to assign edits to
 * @param $rc Update the recent changes table
 * @param $report Don't change things; just echo numbers
 * @return integer Number of entries changed, or that would be changed
 */
function reassignEdits( &$from, &$to, $rc = false, $report = false ) {
	$dbw = wfGetDB( DB_MASTER );
	$dbw->immediateBegin();
	$fname = 'reassignEdits';
		
	# Count things
	out( "Checking current edits..." );
	$res = $dbw->select( 'revision', 'COUNT(*) AS count', userConditions( $from, 'rev_user', 'rev_user_text' ), $fname );
	$row = $dbw->fetchObject( $res );
	$cur = $row->count;
	out( "found {$cur}.\n" );
	
	out( "Checking deleted edits..." );
	$res = $dbw->select( 'archive', 'COUNT(*) AS count', userConditions( $from, 'ar_user', 'ar_user_text' ), $fname );
	$row = $dbw->fetchObject( $res );
	$del = $row->count;
	out( "found {$del}.\n" );
	
	# Don't count recent changes if we're not supposed to
	if( $rc ) {
		out( "Checking recent changes..." );
		$res = $dbw->select( 'recentchanges', 'COUNT(*) AS count', userConditions( $from, 'rc_user', 'rc_user_text' ), $fname );
		$row = $dbw->fetchObject( $res );
		$rec = $row->count;
		out( "found {$rec}.\n" );
	} else {
		$rec = 0;
	}
	
	$total = $cur + $del + $rec;
	out( "\nTotal entries to change: {$total}\n" );
	
	if( !$report ) {
		if( $total ) {
			# Reassign edits
			out( "\nReassigning current edits..." );
			$res = $dbw->update( 'revision', userSpecification( $to, 'rev_user', 'rev_user_text' ), userConditions( $from, 'rev_user', 'rev_user_text' ), $fname );
			out( "done.\nReassigning deleted edits..." );
			$res = $dbw->update( 'archive', userSpecification( $to, 'ar_user', 'ar_user_text' ), userConditions( $from, 'ar_user', 'ar_user_text' ), $fname );
			out( "done.\n" );
			# Update recent changes if required
			if( $rc ) {
				out( "Updating recent changes..." );
				$res = $dbw->update( 'recentchanges', userSpecification( $to, 'rc_user', 'rc_user_text' ), userConditions( $from, 'rc_user', 'rc_user_text' ), $fname );
				out( "done.\n" );
			}
		}	
	}
	
	$dbw->immediateCommit();
	return (int)$total;	
}

/**
 * Return the most efficient set of user conditions
 * i.e. a user => id mapping, or a user_text => text mapping
 *
 * @param $user User for the condition
 * @param $idfield Field name containing the identifier
 * @param $utfield Field name containing the user text
 * @return array
 */
function userConditions( &$user, $idfield, $utfield ) {
	return $user->getId() ? array( $idfield => $user->getId() ) : array( $utfield => $user->getName() );
}

/**
 * Return user specifications
 * i.e. user => id, user_text => text
 *
 * @param $user User for the spec
 * @param $idfield Field name containing the identifier
 * @param $utfield Field name containing the user text
 * @return array
 */
function userSpecification( &$user, $idfield, $utfield ) {
	return array( $idfield => $user->getId(), $utfield => $user->getName() );
}

/**
 * Echo output if $wgSilent is off
 *
 * @param $output Output to echo
 * @return bool True if the output was echoed
 */
function out( $output ) {
	global $wgSilent;
	if( !$wgSilent ) {
		echo( $output );
		return true;
	} else {
		return false;
	}
}

/**
 * Mutator for $wgSilent
 *
 * @param $silent Switch on $wgSilent
 */
function silent( $silent = true ) {
	global $wgSilent;
	$wgSilent = $silent;
}

/**
 * Initialise the user object
 *
 * @param $username Username or IP address
 * @return User
 */
function initialiseUser( $username ) {
	if( User::isIP( $username ) ) {
		$user = new User();
		$user->setId( 0 );
		$user->setName( $username );
	} else {
		$user = User::newFromName( $username );
	}
	$user->load();
	return $user;
}

