<?php

/**
 * This class checks if user can get extra rights
 * because of conditions specified in $wgAutopromote
 */
class Autopromote {
	/**
	 * Get the groups for the given user based on $wgAutopromote.
	 *
	 * @param $user The user to get the groups for
	 * @return array Array of groups to promote to.
	 */
	public static function getAutopromoteGroups( User $user ) {
		global $wgAutopromote;
		$promote = array();
		foreach( $wgAutopromote as $group => $cond ) {
			if( self::recCheckCondition( $cond, $user ) )
				$promote[] = $group;
		}
		
		wfRunHooks( 'GetAutoPromoteGroups', array($user, &$promote) );
		
		return $promote;
	}

	/**
	 * Recursively check a condition.  Conditions are in the form
	 *   array( '&' or '|' or '^', cond1, cond2, ... )
	 * where cond1, cond2, ... are themselves conditions; *OR*
	 *   APCOND_EMAILCONFIRMED, *OR*
	 *   array( APCOND_EMAILCONFIRMED ), *OR*
	 *   array( APCOND_EDITCOUNT, number of edits ), *OR*
	 *   array( APCOND_AGE, seconds since registration ), *OR*
	 *   similar constructs defined by extensions.
	 * This function evaluates the former type recursively, and passes off to
	 * self::checkCondition for evaluation of the latter type.
	 *
	 * @param $cond Mixed: a condition, possibly containing other conditions
	 * @param $user The user to check the conditions against
	 * @return bool Whether the condition is true
	 */
	private static function recCheckCondition( $cond, User $user ) {
		$validOps = array( '&', '|', '^', '!' );
		if( is_array( $cond ) && count( $cond ) >= 2 && in_array( $cond[0], $validOps ) ) {
			# Recursive condition
			if( $cond[0] == '&' ) {
				foreach( array_slice( $cond, 1 ) as $subcond )
					if( !self::recCheckCondition( $subcond, $user ) )
						return false;
				return true;
			} elseif( $cond[0] == '|' ) {
				foreach( array_slice( $cond, 1 ) as $subcond )
					if( self::recCheckCondition( $subcond, $user ) )
						return true;
				return false;
			} elseif( $cond[0] == '^' ) {
				$res = null;
				foreach( array_slice( $cond, 1 ) as $subcond ) {
					if( is_null( $res ) )
						$res = self::recCheckCondition( $subcond, $user );
					else
						$res = ($res xor self::recCheckCondition( $subcond, $user ));
				}
				return $res;
			} elseif ( $cond[0] = '!' ) {
				foreach( array_slice( $cond, 1 ) as $subcond )
					if( self::recCheckCondition( $subcond, $user ) )
						return false;
				return true;
			}
		}
		# If we got here, the array presumably does not contain other condi-
		# tions; it's not recursive.  Pass it off to self::checkCondition.
		if( !is_array( $cond ) )
			$cond = array( $cond );
		return self::checkCondition( $cond, $user );
	}

	/**
	 * As recCheckCondition, but *not* recursive.  The only valid conditions
	 * are those whose first element is APCOND_EMAILCONFIRMED/APCOND_EDITCOUNT/
	 * APCOND_AGE.  Other types will throw an exception if no extension evalu-
	 * ates them.
	 *
	 * @param $cond Array: A condition, which must not contain other conditions
	 * @param $user The user to check the condition against
	 * @return bool Whether the condition is true for the user
	 */
	private static function checkCondition( $cond, User $user ) {
		if( count( $cond ) < 1 )
			return false;
		switch( $cond[0] ) {
			case APCOND_EMAILCONFIRMED:
				if( User::isValidEmailAddr( $user->getEmail() ) ) {
					global $wgEmailAuthentication;
					if( $wgEmailAuthentication ) {
						return (bool)$user->getEmailAuthenticationTimestamp();
					} else {
						return true;
					}
				}
				return false;
			case APCOND_EDITCOUNT:
				return $user->getEditCount() >= $cond[1];
			case APCOND_AGE:
				$age = time() - wfTimestampOrNull( TS_UNIX, $user->getRegistration() );
				return $age >= $cond[1];
			case APCOND_INGROUPS:
				$groups = array_slice( $cond, 1 );
				return count( array_intersect( $groups, $user->getGroups() ) ) == count( $groups );
			default:
				$result = null;
				wfRunHooks( 'AutopromoteCondition', array( $cond[0], array_slice( $cond, 1 ), $user, &$result ) );
				if( $result === null ) {
					throw new MWException( "Unrecognized condition {$cond[0]} for autopromotion!" );
				}
				return $result ? true : false;
		}
	}
}
