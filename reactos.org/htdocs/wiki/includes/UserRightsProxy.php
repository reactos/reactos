<?php


/**
 * Cut-down copy of User interface for local-interwiki-database
 * user rights manipulation.
 */
class UserRightsProxy {
	private function __construct( $db, $database, $name, $id ) {
		$this->db = $db;
		$this->database = $database;
		$this->name = $name;
		$this->id = intval( $id );
	}

	/**
	 * Confirm the selected database name is a valid local interwiki database name.
	 * @return bool
	 */
	public static function validDatabase( $database ) {
		global $wgLocalDatabases;
		return in_array( $database, $wgLocalDatabases );
	}

	public static function whoIs( $database, $id ) {
		$user = self::newFromId( $database, $id );
		if( $user ) {
			return $user->name;
		} else {
			return false;
		}
	}

	/**
	 * Factory function; get a remote user entry by ID number.
	 * @return UserRightsProxy or null if doesn't exist
	 */
	public static function newFromId( $database, $id ) {
		return self::newFromLookup( $database, 'user_id', intval( $id ) );
	}

	public static function newFromName( $database, $name ) {
		return self::newFromLookup( $database, 'user_name', $name );
	}

	private static function newFromLookup( $database, $field, $value ) {
		$db = self::getDB( $database );
		if( $db ) {
			$row = $db->selectRow( 'user',
				array( 'user_id', 'user_name' ),
				array( $field => $value ),
				__METHOD__ );
			if( $row !== false ) {
				return new UserRightsProxy( $db, $database,
					$row->user_name,
					intval( $row->user_id ) );
			}
		}
		return null;
	}

	/**
	 * Open a database connection to work on for the requested user.
	 * This may be a new connection to another database for remote users.
	 * @param $database string
	 * @return Database or null if invalid selection
	 */
	public static function getDB( $database ) {
		global $wgLocalDatabases, $wgDBname;
		if( self::validDatabase( $database ) ) {
			if( $database == $wgDBname ) {
				// Hmm... this shouldn't happen though. :)
				return wfGetDB( DB_MASTER );
			} else {
				return wfGetDB( DB_MASTER, array(), $database );
			}
		}
		return null;
	}

	public function getId() {
		return $this->id;
	}

	public function isAnon() {
		return $this->getId() == 0;
	}

	public function getName() {
		return $this->name . '@' . $this->database;
	}

	public function getUserPage() {
		return Title::makeTitle( NS_USER, $this->getName() );
	}

	// Replaces getUserGroups()
	function getGroups() {
		$res = $this->db->select( 'user_groups',
			array( 'ug_group' ),
			array( 'ug_user' => $this->id ),
			__METHOD__ );
		$groups = array();
		while( $row = $this->db->fetchObject( $res ) ) {
			$groups[] = $row->ug_group;
		}
		return $groups;
	}

	// replaces addUserGroup
	function addGroup( $group ) {
		$this->db->insert( 'user_groups',
			array(
				'ug_user' => $this->id,
				'ug_group' => $group,
			),
			__METHOD__,
			array( 'IGNORE' ) );
	}

	// replaces removeUserGroup
	function removeGroup( $group ) {
		$this->db->delete( 'user_groups',
			array(
				'ug_user' => $this->id,
				'ug_group' => $group,
			),
			__METHOD__ );
	}

	// replaces touchUser
	function invalidateCache() {
		$this->db->update( 'user',
			array( 'user_touched' => $this->db->timestamp() ),
			array( 'user_id' => $this->id ),
			__METHOD__ );

		global $wgMemc;
		if ( function_exists( 'wfForeignMemcKey' ) ) {
			$key = wfForeignMemcKey( $this->database, false, 'user', 'id', $this->id );
		} else {
			$key = "$this->database:user:id:" . $this->id;
		}
		$wgMemc->delete( $key );
	}
}
