<?php
/**
 * @file
 * Blocks and bans object
 */

/**
 * The block class
 * All the functions in this class assume the object is either explicitly
 * loaded or filled. It is not load-on-demand. There are no accessors.
 *
 * Globals used: $wgAutoblockExpiry, $wgAntiLockFlags
 *
 * @todo This could be used everywhere, but it isn't.
 */
class Block
{
	/* public*/ var $mAddress, $mUser, $mBy, $mReason, $mTimestamp, $mAuto, $mId, $mExpiry,
				$mRangeStart, $mRangeEnd, $mAnonOnly, $mEnableAutoblock, $mHideName,
				$mBlockEmail, $mByName, $mAngryAutoblock;
	/* private */ var $mNetworkBits, $mIntegerAddr, $mForUpdate, $mFromMaster;

	const EB_KEEP_EXPIRED = 1;
	const EB_FOR_UPDATE = 2;
	const EB_RANGE_ONLY = 4;

	function __construct( $address = '', $user = 0, $by = 0, $reason = '',
		$timestamp = '' , $auto = 0, $expiry = '', $anonOnly = 0, $createAccount = 0, $enableAutoblock = 0,
		$hideName = 0, $blockEmail = 0 )
	{
		$this->mId = 0;
		# Expand valid IPv6 addresses
		$address = IP::sanitizeIP( $address );
		$this->mAddress = $address;
		$this->mUser = $user;
		$this->mBy = $by;
		$this->mReason = $reason;
		$this->mTimestamp = wfTimestamp(TS_MW,$timestamp);
		$this->mAuto = $auto;
		$this->mAnonOnly = $anonOnly;
		$this->mCreateAccount = $createAccount;
		$this->mExpiry = self::decodeExpiry( $expiry );
		$this->mEnableAutoblock = $enableAutoblock;
		$this->mHideName = $hideName;
		$this->mBlockEmail = $blockEmail;
		$this->mForUpdate = false;
		$this->mFromMaster = false;
		$this->mByName = false;
		$this->mAngryAutoblock = false;
		$this->initialiseRange();
	}

	static function newFromDB( $address, $user = 0, $killExpired = true )
	{
		$block = new Block();
		$block->load( $address, $user, $killExpired );
		if ( $block->isValid() ) {
			return $block;
		} else {
			return null;
		}
	}

	static function newFromID( $id )
	{
		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->resultObject( $dbr->select( 'ipblocks', '*',
			array( 'ipb_id' => $id ), __METHOD__ ) );
		$block = new Block;
		if ( $block->loadFromResult( $res ) ) {
			return $block;
		} else {
			return null;
		}
	}

	function clear()
	{
		$this->mAddress = $this->mReason = $this->mTimestamp = '';
		$this->mId = $this->mAnonOnly = $this->mCreateAccount =
			$this->mEnableAutoblock = $this->mAuto = $this->mUser =
			$this->mBy = $this->mHideName = $this->mBlockEmail = 0;
		$this->mByName = false;
	}

	/**
	 * Get the DB object and set the reference parameter to the query options
	 */
	function &getDBOptions( &$options )
	{
		global $wgAntiLockFlags;
		if ( $this->mForUpdate || $this->mFromMaster ) {
			$db = wfGetDB( DB_MASTER );
			if ( !$this->mForUpdate || ($wgAntiLockFlags & ALF_NO_BLOCK_LOCK) ) {
				$options = array();
			} else {
				$options = array( 'FOR UPDATE' );
			}
		} else {
			$db = wfGetDB( DB_SLAVE );
			$options = array();
		}
		return $db;
	}

	/**
	 * Get a ban from the DB, with either the given address or the given username
	 *
	 * @param string $address The IP address of the user, or blank to skip IP blocks
	 * @param integer $user The user ID, or zero for anonymous users
	 * @param bool $killExpired Whether to delete expired rows while loading
	 *
	 */
	function load( $address = '', $user = 0, $killExpired = true )
	{
		wfDebug( "Block::load: '$address', '$user', $killExpired\n" );

		$options = array();
		$db =& $this->getDBOptions( $options );

		if ( 0 == $user && $address == '' ) {
			# Invalid user specification, not blocked
			$this->clear();
			return false;
		}

		# Try user block
		if ( $user ) {
			$res = $db->resultObject( $db->select( 'ipblocks', '*', array( 'ipb_user' => $user ),
				__METHOD__, $options ) );
			if ( $this->loadFromResult( $res, $killExpired ) ) {
				return true;
			}
		}

		# Try IP block
		# TODO: improve performance by merging this query with the autoblock one
		# Slightly tricky while handling killExpired as well
		if ( $address ) {
			$conds = array( 'ipb_address' => $address, 'ipb_auto' => 0 );
			$res = $db->resultObject( $db->select( 'ipblocks', '*', $conds, __METHOD__, $options ) );
			if ( $this->loadFromResult( $res, $killExpired ) ) {
				if ( $user && $this->mAnonOnly ) {
					# Block is marked anon-only
					# Whitelist this IP address against autoblocks and range blocks
					$this->clear();
					return false;
				} else {
					return true;
				}
			}
		}

		# Try range block
		if ( $this->loadRange( $address, $killExpired, $user ) ) {
			if ( $user && $this->mAnonOnly ) {
				$this->clear();
				return false;
			} else {
				return true;
			}
		}

		# Try autoblock
		if ( $address ) {
			$conds = array( 'ipb_address' => $address, 'ipb_auto' => 1 );
			if ( $user ) {
				$conds['ipb_anon_only'] = 0;
			}
			$res = $db->resultObject( $db->select( 'ipblocks', '*', $conds, __METHOD__, $options ) );
			if ( $this->loadFromResult( $res, $killExpired ) ) {
				return true;
			}
		}

		# Give up
		$this->clear();
		return false;
	}

	/**
	 * Fill in member variables from a result wrapper
	 */
	function loadFromResult( ResultWrapper $res, $killExpired = true )
	{
		$ret = false;
		if ( 0 != $res->numRows() ) {
			# Get first block
			$row = $res->fetchObject();
			$this->initFromRow( $row );

			if ( $killExpired ) {
				# If requested, delete expired rows
				do {
					$killed = $this->deleteIfExpired();
					if ( $killed ) {
						$row = $res->fetchObject();
						if ( $row ) {
							$this->initFromRow( $row );
						}
					}
				} while ( $killed && $row );

				# If there were any left after the killing finished, return true
				if ( $row ) {
					$ret = true;
				}
			} else {
				$ret = true;
			}
		}
		$res->free();
		return $ret;
	}

	/**
	 * Search the database for any range blocks matching the given address, and
	 * load the row if one is found.
	 */
	function loadRange( $address, $killExpired = true, $user = 0 )
	{
		$iaddr = IP::toHex( $address );
		if ( $iaddr === false ) {
			# Invalid address
			return false;
		}

		# Only scan ranges which start in this /16, this improves search speed
		# Blocks should not cross a /16 boundary.
		$range = substr( $iaddr, 0, 4 );

		$options = array();
		$db =& $this->getDBOptions( $options );
		$conds = array(
			"ipb_range_start LIKE '$range%'",
			"ipb_range_start <= '$iaddr'",
			"ipb_range_end >= '$iaddr'"
		);

		if ( $user ) {
			$conds['ipb_anon_only'] = 0;
		}

		$res = $db->resultObject( $db->select( 'ipblocks', '*', $conds, __METHOD__, $options ) );
		$success = $this->loadFromResult( $res, $killExpired );
		return $success;
	}

	/**
	 * Determine if a given integer IPv4 address is in a given CIDR network
	 * @deprecated Use IP::isInRange
	 */
	function isAddressInRange( $addr, $range ) {
		return IP::isInRange( $addr, $range );
	}

	function initFromRow( $row )
	{
		$this->mAddress = $row->ipb_address;
		$this->mReason = $row->ipb_reason;
		$this->mTimestamp = wfTimestamp(TS_MW,$row->ipb_timestamp);
		$this->mUser = $row->ipb_user;
		$this->mBy = $row->ipb_by;
		$this->mAuto = $row->ipb_auto;
		$this->mAnonOnly = $row->ipb_anon_only;
		$this->mCreateAccount = $row->ipb_create_account;
		$this->mEnableAutoblock = $row->ipb_enable_autoblock;
		$this->mBlockEmail = $row->ipb_block_email;
		$this->mHideName = $row->ipb_deleted;
		$this->mId = $row->ipb_id;
		$this->mExpiry = self::decodeExpiry( $row->ipb_expiry );
		if ( isset( $row->user_name ) ) {
			$this->mByName = $row->user_name;
		} else {
			$this->mByName = $row->ipb_by_text;
		}
		$this->mRangeStart = $row->ipb_range_start;
		$this->mRangeEnd = $row->ipb_range_end;
	}

	function initialiseRange()
	{
		$this->mRangeStart = '';
		$this->mRangeEnd = '';

		if ( $this->mUser == 0 ) {
			list( $this->mRangeStart, $this->mRangeEnd ) = IP::parseRange( $this->mAddress );
		}
	}

	/**
	 * Callback with a Block object for every block
	 * @return integer number of blocks;
	 */
	/*static*/ function enumBlocks( $callback, $tag, $flags = 0 )
	{
		global $wgAntiLockFlags;

		$block = new Block();
		if ( $flags & Block::EB_FOR_UPDATE ) {
			$db = wfGetDB( DB_MASTER );
			if ( $wgAntiLockFlags & ALF_NO_BLOCK_LOCK ) {
				$options = '';
			} else {
				$options = 'FOR UPDATE';
			}
			$block->forUpdate( true );
		} else {
			$db = wfGetDB( DB_SLAVE );
			$options = '';
		}
		if ( $flags & Block::EB_RANGE_ONLY ) {
			$cond = " AND ipb_range_start <> ''";
		} else {
			$cond = '';
		}

		$now = wfTimestampNow();

		list( $ipblocks, $user ) = $db->tableNamesN( 'ipblocks', 'user' );

		$sql = "SELECT $ipblocks.*,user_name FROM $ipblocks,$user " .
			"WHERE user_id=ipb_by $cond ORDER BY ipb_timestamp DESC $options";
		$res = $db->query( $sql, 'Block::enumBlocks' );
		$num_rows = $db->numRows( $res );

		while ( $row = $db->fetchObject( $res ) ) {
			$block->initFromRow( $row );
			if ( ( $flags & Block::EB_RANGE_ONLY ) && $block->mRangeStart == '' ) {
				continue;
			}

			if ( !( $flags & Block::EB_KEEP_EXPIRED ) ) {
				if ( $block->mExpiry && $now > $block->mExpiry ) {
					$block->delete();
				} else {
					call_user_func( $callback, $block, $tag );
				}
			} else {
				call_user_func( $callback, $block, $tag );
			}
		}
		$db->freeResult( $res );
		return $num_rows;
	}

	function delete()
	{
		if (wfReadOnly()) {
			return false;
		}
		if ( !$this->mId ) {
			throw new MWException( "Block::delete() now requires that the mId member be filled\n" );
		}

		$dbw = wfGetDB( DB_MASTER );
		$dbw->delete( 'ipblocks', array( 'ipb_id' => $this->mId ), __METHOD__ );
		return $dbw->affectedRows() > 0;
	}

	/**
	* Insert a block into the block table.
	* @return Whether or not the insertion was successful.
	*/
	function insert()
	{
		wfDebug( "Block::insert; timestamp {$this->mTimestamp}\n" );
		$dbw = wfGetDB( DB_MASTER );

		# Unset ipb_anon_only for user blocks, makes no sense
		if ( $this->mUser ) {
			$this->mAnonOnly = 0;
		}

		# Unset ipb_enable_autoblock for IP blocks, makes no sense
		if ( !$this->mUser ) {
			$this->mEnableAutoblock = 0;
			$this->mBlockEmail = 0; //Same goes for email...
		}

		if( !$this->mByName ) {
			if( $this->mBy ) {
				$this->mByName = User::whoIs( $this->mBy );
			} else {
				global $wgUser;
				$this->mByName = $wgUser->getName();
			}
		}

		# Don't collide with expired blocks
		Block::purgeExpired();

		$ipb_id = $dbw->nextSequenceValue('ipblocks_ipb_id_val');
		$dbw->insert( 'ipblocks',
			array(
				'ipb_id' => $ipb_id,
				'ipb_address' => $this->mAddress,
				'ipb_user' => $this->mUser,
				'ipb_by' => $this->mBy,
				'ipb_by_text' => $this->mByName,
				'ipb_reason' => $this->mReason,
				'ipb_timestamp' => $dbw->timestamp($this->mTimestamp),
				'ipb_auto' => $this->mAuto,
				'ipb_anon_only' => $this->mAnonOnly,
				'ipb_create_account' => $this->mCreateAccount,
				'ipb_enable_autoblock' => $this->mEnableAutoblock,
				'ipb_expiry' => self::encodeExpiry( $this->mExpiry, $dbw ),
				'ipb_range_start' => $this->mRangeStart,
				'ipb_range_end' => $this->mRangeEnd,
				'ipb_deleted'	=> $this->mHideName,
				'ipb_block_email' => $this->mBlockEmail
			), 'Block::insert', array( 'IGNORE' )
		);
		$affected = $dbw->affectedRows();

		if ($affected)
			$this->doRetroactiveAutoblock();

		return $affected;
	}

	/**
	* Retroactively autoblocks the last IP used by the user (if it is a user)
	* blocked by this Block.
	*@return Whether or not a retroactive autoblock was made.
	*/
	function doRetroactiveAutoblock() {
		$dbr = wfGetDB( DB_SLAVE );
		#If autoblock is enabled, autoblock the LAST IP used
		# - stolen shamelessly from CheckUser_body.php

		if ($this->mEnableAutoblock && $this->mUser) {
			wfDebug("Doing retroactive autoblocks for " . $this->mAddress . "\n");
			
			$options = array( 'ORDER BY' => 'rc_timestamp DESC' );
			$conds = array( 'rc_user_text' => $this->mAddress );
			
			if ($this->mAngryAutoblock) {
				// Block any IP used in the last 7 days. Up to five IPs.
				$conds[] = 'rc_timestamp < ' . $dbr->addQuotes( $dbr->timestamp( time() - (7*86400) ) );
				$options['LIMIT'] = 5;
			} else {
				// Just the last IP used.
				$options['LIMIT'] = 1;
			}

			$res = $dbr->select( 'recentchanges', array( 'rc_ip' ), $conds,
				__METHOD__ ,  $options);

			if ( !$dbr->numRows( $res ) ) {
				#No results, don't autoblock anything
				wfDebug("No IP found to retroactively autoblock\n");
			} else {
				while ( $row = $dbr->fetchObject( $res ) ) {
					if ( $row->rc_ip )
						$this->doAutoblock( $row->rc_ip );
				}
			}
		}
	}

	/**
	* Autoblocks the given IP, referring to this Block.
	* @param string $autoblockip The IP to autoblock.
	* @param bool $justInserted The main block was just inserted
	* @return bool Whether or not an autoblock was inserted.
	*/
	function doAutoblock( $autoblockip, $justInserted = false ) {
		# If autoblocks are disabled, go away.
		if ( !$this->mEnableAutoblock ) {
			return;
		}

		# Check for presence on the autoblock whitelist
		# TODO cache this?
		$lines = explode( "\n", wfMsgForContentNoTrans( 'autoblock_whitelist' ) );

		$ip = $autoblockip;

		wfDebug("Checking the autoblock whitelist..\n");

		foreach( $lines as $line ) {
			# List items only
			if ( substr( $line, 0, 1 ) !== '*' ) {
				continue;
			}

			$wlEntry = substr($line, 1);
			$wlEntry = trim($wlEntry);

			wfDebug("Checking $ip against $wlEntry...");

			# Is the IP in this range?
			if (IP::isInRange( $ip, $wlEntry )) {
				wfDebug(" IP $ip matches $wlEntry, not autoblocking\n");
				#$autoblockip = null; # Don't autoblock a whitelisted IP.
				return; #This /SHOULD/ introduce a dummy block - but
					# I don't know a safe way to do so. -werdna
			} else {
				wfDebug( " No match\n" );
			}
		}
		
		## Allow hooks to cancel the autoblock.
		if (!wfRunHooks( 'AbortAutoblock', array( $autoblockip, &$this ) )) {
			wfDebug( "Autoblock aborted by hook." );
			return false;
		}

		# It's okay to autoblock. Go ahead and create/insert the block.

		$ipblock = Block::newFromDB( $autoblockip );
		if ( $ipblock ) {
			# If the user is already blocked. Then check if the autoblock would
			# exceed the user block. If it would exceed, then do nothing, else
			# prolong block time
			if ($this->mExpiry &&
			($this->mExpiry < Block::getAutoblockExpiry($ipblock->mTimestamp))) {
				return;
			}
			# Just update the timestamp
			if ( !$justInserted ) {
				$ipblock->updateTimestamp();
			}
			return;
		} else {
			$ipblock = new Block;
		}

		# Make a new block object with the desired properties
		wfDebug( "Autoblocking {$this->mAddress}@" . $autoblockip . "\n" );
		$ipblock->mAddress = $autoblockip;
		$ipblock->mUser = 0;
		$ipblock->mBy = $this->mBy;
		$ipblock->mByName = $this->mByName;
		$ipblock->mReason = wfMsgForContent( 'autoblocker', $this->mAddress, $this->mReason );
		$ipblock->mTimestamp = wfTimestampNow();
		$ipblock->mAuto = 1;
		$ipblock->mCreateAccount = $this->mCreateAccount;
		# Continue suppressing the name if needed
		$ipblock->mHideName = $this->mHideName;

		# If the user is already blocked with an expiry date, we don't
		# want to pile on top of that!
		if($this->mExpiry) {
			$ipblock->mExpiry = min ( $this->mExpiry, Block::getAutoblockExpiry( $this->mTimestamp ));
		} else {
			$ipblock->mExpiry = Block::getAutoblockExpiry( $this->mTimestamp );
		}
		# Insert it
		return $ipblock->insert();
	}

	function deleteIfExpired()
	{
		$fname = 'Block::deleteIfExpired';
		wfProfileIn( $fname );
		if ( $this->isExpired() ) {
			wfDebug( "Block::deleteIfExpired() -- deleting\n" );
			$this->delete();
			$retVal = true;
		} else {
			wfDebug( "Block::deleteIfExpired() -- not expired\n" );
			$retVal = false;
		}
		wfProfileOut( $fname );
		return $retVal;
	}

	function isExpired()
	{
		wfDebug( "Block::isExpired() checking current " . wfTimestampNow() . " vs $this->mExpiry\n" );
		if ( !$this->mExpiry ) {
			return false;
		} else {
			return wfTimestampNow() > $this->mExpiry;
		}
	}

	function isValid()
	{
		return $this->mAddress != '';
	}

	function updateTimestamp()
	{
		if ( $this->mAuto ) {
			$this->mTimestamp = wfTimestamp();
			$this->mExpiry = Block::getAutoblockExpiry( $this->mTimestamp );

			$dbw = wfGetDB( DB_MASTER );
			$dbw->update( 'ipblocks',
				array( /* SET */
					'ipb_timestamp' => $dbw->timestamp($this->mTimestamp),
					'ipb_expiry' => $dbw->timestamp($this->mExpiry),
				), array( /* WHERE */
					'ipb_address' => $this->mAddress
				), 'Block::updateTimestamp'
			);
		}
	}

	/*
	function getIntegerAddr()
	{
		return $this->mIntegerAddr;
	}

	function getNetworkBits()
	{
		return $this->mNetworkBits;
	}*/

	/**
	 * @return The blocker user ID.
	 */
	public function getBy() {
		return $this->mBy;
	}

	/**
	 * @return The blocker user name.
	 */
	function getByName()
	{
		return $this->mByName;
	}

	function forUpdate( $x = NULL ) {
		return wfSetVar( $this->mForUpdate, $x );
	}

	function fromMaster( $x = NULL ) {
		return wfSetVar( $this->mFromMaster, $x );
	}

	function getRedactedName() {
		if ( $this->mAuto ) {
			return '#' . $this->mId;
		} else {
			return $this->mAddress;
		}
	}

	/**
	 * Encode expiry for DB
	 */
	static function encodeExpiry( $expiry, $db ) {
		if ( $expiry == '' || $expiry == Block::infinity() ) {
			return Block::infinity();
		} else {
			return $db->timestamp( $expiry );
		}
	}

	/**
	 * Decode expiry which has come from the DB
	 */
	static function decodeExpiry( $expiry, $timestampType = TS_MW ) {
		if ( $expiry == '' || $expiry == Block::infinity() ) {
			return Block::infinity();
		} else {
			return wfTimestamp( $timestampType, $expiry );
		}
	}

	static function getAutoblockExpiry( $timestamp )
	{
		global $wgAutoblockExpiry;
		return wfTimestamp( TS_MW, wfTimestamp( TS_UNIX, $timestamp ) + $wgAutoblockExpiry );
	}

	/**
	 * Gets rid of uneeded numbers in quad-dotted/octet IP strings
	 * For example, 127.111.113.151/24 -> 127.111.113.0/24
	 */
	static function normaliseRange( $range ) {
		$parts = explode( '/', $range );
		if ( count( $parts ) == 2 ) {
			// IPv6
			if ( IP::isIPv6($range) && $parts[1] >= 64 && $parts[1] <= 128 ) {
				$bits = $parts[1];
				$ipint = IP::toUnsigned6( $parts[0] );
				# Native 32 bit functions WONT work here!!!
				# Convert to a padded binary number
				$network = wfBaseConvert( $ipint, 10, 2, 128 );
				# Truncate the last (128-$bits) bits and replace them with zeros
				$network = str_pad( substr( $network, 0, $bits ), 128, 0, STR_PAD_RIGHT );
				# Convert back to an integer
				$network = wfBaseConvert( $network, 2, 10 );
				# Reform octet address
				$newip = IP::toOctet( $network );
				$range = "$newip/{$parts[1]}";
			} // IPv4
			else if ( IP::isIPv4($range) && $parts[1] >= 16 && $parts[1] <= 32 ) {
				$shift = 32 - $parts[1];
				$ipint = IP::toUnsigned( $parts[0] );
				$ipint = $ipint >> $shift << $shift;
				$newip = long2ip( $ipint );
				$range = "$newip/{$parts[1]}";
			}
		}
		return $range;
	}

	/**
	 * Purge expired blocks from the ipblocks table
	 */
	static function purgeExpired() {
		$dbw = wfGetDB( DB_MASTER );
		$dbw->delete( 'ipblocks', array( 'ipb_expiry < ' . $dbw->addQuotes( $dbw->timestamp() ) ), __METHOD__ );
	}

	static function infinity() {
		# This is a special keyword for timestamps in PostgreSQL, and
		# works with CHAR(14) as well because "i" sorts after all numbers.
		return 'infinity';

		/*
		static $infinity;
		if ( !isset( $infinity ) ) {
			$dbr = wfGetDB( DB_SLAVE );
			$infinity = $dbr->bigTimestamp();
		}
		return $infinity;
		 */
	}
	
	/**
	 * Convert a DB-encoded expiry into a real string that humans can read.
	 */
	static function formatExpiry( $encoded_expiry ) {
	
		static $msg = null;
		
		if( is_null( $msg ) ) {
			$msg = array();
			$keys = array( 'infiniteblock', 'expiringblock' );
			foreach( $keys as $key ) {
				$msg[$key] = wfMsgHtml( $key );
			}
		}
		
		$expiry = Block::decodeExpiry( $encoded_expiry );
		if ($expiry == 'infinity') {
			$expirystr = $msg['infiniteblock'];
		} else {
			global $wgLang;
			$expiretimestr = $wgLang->timeanddate( $expiry, true );
			$expirystr = wfMsgReplaceArgs( $msg['expiringblock'], array($expiretimestr) );
		}

		return $expirystr;
	}
	
	/**
	 * Convert a typed-in expiry time into something we can put into the database.
	 */
	static function parseExpiryInput( $expiry_input ) {
		if ( $expiry_input == 'infinite' || $expiry_input == 'indefinite' ) {
			$expiry = 'infinity';
		} else {
			$expiry = strtotime( $expiry_input );
			if ($expiry < 0 || $expiry === false) {
				return false;
			}
		}
		
		return $expiry;
	}

}
