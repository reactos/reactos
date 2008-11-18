<?php

/**
 * Class containing static functions for working with
 * logs of patrol events
 *
 * @author Rob Church <robchur@gmail.com>
 */
class PatrolLog {

	/**
	 * Record a log event for a change being patrolled
	 *
	 * @param mixed $change Change identifier or RecentChange object
	 * @param bool $auto Was this patrol event automatic?
	 */
	public static function record( $change, $auto = false ) {
		if( !( is_object( $change ) && $change instanceof RecentChange ) ) {
			$change = RecentChange::newFromId( $change );
			if( !is_object( $change ) )
				return false;
		}
		$title = Title::makeTitleSafe( $change->getAttribute( 'rc_namespace' ),
					$change->getAttribute( 'rc_title' ) );
		if( is_object( $title ) ) {
			$params = self::buildParams( $change, $auto );
			$log = new LogPage( 'patrol', false ); # False suppresses RC entries
			$log->addEntry( 'patrol', $title, '', $params );
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Generate the log action text corresponding to a patrol log item
	 *
	 * @param Title $title Title of the page that was patrolled
	 * @param array $params Log parameters (from logging.log_params)
	 * @param Skin $skin Skin to use for building links, etc.
	 * @return string
	 */
	public static function makeActionText( $title, $params, $skin ) {
		# This is a bit of a hack, but...if $skin is not a Skin, then *do nothing*
		# -- this is fine, because the action text we would be queried for under
		# these conditions would have gone into recentchanges, which we aren't
		# supposed to be updating
		if( is_object( $skin ) ) {
			list( $cur, /* $prev */, $auto ) = $params;
			# Standard link to the page in question
			$link = $skin->makeLinkObj( $title );
			if( $title->exists() ) {
				# Generate a diff link
				$bits[] = 'oldid=' . urlencode( $cur );
				$bits[] = 'diff=prev';
				$bits = implode( '&', $bits );
				$diff = $skin->makeKnownLinkObj( $title, htmlspecialchars( wfMsg( 'patrol-log-diff', $cur ) ), $bits );
			} else {
				# Don't bother with a diff link, it's useless
				$diff = htmlspecialchars( wfMsg( 'patrol-log-diff', $cur ) );
			}
			# Indicate whether or not the patrolling was automatic
			$auto = $auto ? wfMsgHtml( 'patrol-log-auto' ) : '';
			# Put it all together
			return wfMsgHtml( 'patrol-log-line', $diff, $link, $auto );
		} else {
			return '';
		}
	}

	/**
	 * Prepare log parameters for a patrolled change
	 *
	 * @param RecentChange $change RecentChange to represent
	 * @param bool $auto Whether the patrol event was automatic
	 * @return array
	 */
	private static function buildParams( $change, $auto ) {
		return array(
			$change->getAttribute( 'rc_this_oldid' ),
			$change->getAttribute( 'rc_last_oldid' ),
			(int)$auto
		);
	}
}
