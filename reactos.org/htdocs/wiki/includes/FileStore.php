<?php

/**
 * @todo document (needs one-sentence top-level class description).
 */
class FileStore {
	const DELETE_ORIGINAL = 1;

	/**
	 * Fetch the FileStore object for a given storage group
	 */
	static function get( $group ) {
		global $wgFileStore;

		if( isset( $wgFileStore[$group] ) ) {
			$info = $wgFileStore[$group];
			return new FileStore( $group,
				$info['directory'],
				$info['url'],
				intval( $info['hash'] ) );
		} else {
			return null;
		}
	}

	private function __construct( $group, $directory, $path, $hash ) {
		$this->mGroup = $group;
		$this->mDirectory = $directory;
		$this->mPath = $path;
		$this->mHashLevel = $hash;
	}

	/**
	 * Acquire a lock; use when performing write operations on a store.
	 * This is attached to your master database connection, so if you
	 * suffer an uncaught error the lock will be released when the
	 * connection is closed.
	 *
	 * @todo Probably only works on MySQL. Abstract to the Database class?
	 */
	static function lock() {
		global $wgDBtype;
		if ($wgDBtype != 'mysql')
			return true;
		$dbw = wfGetDB( DB_MASTER );
		$lockname = $dbw->addQuotes( FileStore::lockName() );
		$result = $dbw->query( "SELECT GET_LOCK($lockname, 5) AS lockstatus", __METHOD__ );
		$row = $dbw->fetchObject( $result );
		$dbw->freeResult( $result );

		if( $row->lockstatus == 1 ) {
			return true;
		} else {
			wfDebug( __METHOD__." failed to acquire lock\n" );
			return false;
		}
	}

	/**
	 * Release the global file store lock.
	 */
	static function unlock() {
		global $wgDBtype;
		if ($wgDBtype != 'mysql')
			return true;
		$dbw = wfGetDB( DB_MASTER );
		$lockname = $dbw->addQuotes( FileStore::lockName() );
		$result = $dbw->query( "SELECT RELEASE_LOCK($lockname)", __METHOD__ );
		$dbw->fetchObject( $result );
		$dbw->freeResult( $result );
	}

	private static function lockName() {
		return 'MediaWiki.' . wfWikiID() . '.FileStore';
	}

	/**
	 * Copy a file into the file store from elsewhere in the filesystem.
	 * Should be protected by FileStore::lock() to avoid race conditions.
	 *
	 * @param $key storage key string
	 * @param $flags
	 *  DELETE_ORIGINAL - remove the source file on transaction commit.
	 *
	 * @throws FSException if copy can't be completed
	 * @return FSTransaction
	 */
	function insert( $key, $sourcePath, $flags=0 ) {
		$destPath = $this->filePath( $key );
		return $this->copyFile( $sourcePath, $destPath, $flags );
	}

	/**
	 * Copy a file from the file store to elsewhere in the filesystem.
	 * Should be protected by FileStore::lock() to avoid race conditions.
	 *
	 * @param $key storage key string
	 * @param $flags
	 *  DELETE_ORIGINAL - remove the source file on transaction commit.
	 *
	 * @throws FSException if copy can't be completed
	 * @return FSTransaction on success
	 */
	function export( $key, $destPath, $flags=0 ) {
		$sourcePath = $this->filePath( $key );
		return $this->copyFile( $sourcePath, $destPath, $flags );
	}

	private function copyFile( $sourcePath, $destPath, $flags=0 ) {
		if( !file_exists( $sourcePath ) ) {
			// Abort! Abort!
			throw new FSException( "missing source file '$sourcePath'" );
		}

		$transaction = new FSTransaction();

		if( $flags & self::DELETE_ORIGINAL ) {
			$transaction->addCommit( FSTransaction::DELETE_FILE, $sourcePath );
		}

		if( file_exists( $destPath ) ) {
			// An identical file is already present; no need to copy.
		} else {
			if( !file_exists( dirname( $destPath ) ) ) {
				wfSuppressWarnings();
				$ok = mkdir( dirname( $destPath ), 0777, true );
				wfRestoreWarnings();

				if( !$ok ) {
					throw new FSException(
						"failed to create directory for '$destPath'" );
				}
			}

			wfSuppressWarnings();
			$ok = copy( $sourcePath, $destPath );
			wfRestoreWarnings();

			if( $ok ) {
				wfDebug( __METHOD__." copied '$sourcePath' to '$destPath'\n" );
				$transaction->addRollback( FSTransaction::DELETE_FILE, $destPath );
			} else {
				throw new FSException(
					__METHOD__." failed to copy '$sourcePath' to '$destPath'" );
			}
		}

		return $transaction;
	}

	/**
	 * Delete a file from the file store.
	 * Caller's responsibility to make sure it's not being used by another row.
	 *
	 * File is not actually removed until transaction commit.
	 * Should be protected by FileStore::lock() to avoid race conditions.
	 *
	 * @param $key storage key string
	 * @throws FSException if file can't be deleted
	 * @return FSTransaction
	 */
	function delete( $key ) {
		$destPath = $this->filePath( $key );
		if( false === $destPath ) {
			throw new FSException( "file store does not contain file '$key'" );
		} else {
			return FileStore::deleteFile( $destPath );
		}
	}

	/**
	 * Delete a non-managed file on a transactional basis.
	 *
	 * File is not actually removed until transaction commit.
	 * Should be protected by FileStore::lock() to avoid race conditions.
	 *
	 * @param $path file to remove
	 * @throws FSException if file can't be deleted
	 * @return FSTransaction
	 *
	 * @todo Might be worth preliminary permissions check
	 */
	static function deleteFile( $path ) {
		if( file_exists( $path ) ) {
			$transaction = new FSTransaction();
			$transaction->addCommit( FSTransaction::DELETE_FILE, $path );
			return $transaction;
		} else {
			throw new FSException( "cannot delete missing file '$path'" );
		}
	}

	/**
	 * Stream a contained file directly to HTTP output.
	 * Will throw a 404 if file is missing; 400 if invalid key.
	 * @return true on success, false on failure
	 */
	function stream( $key ) {
		$path = $this->filePath( $key );
		if( $path === false ) {
			wfHttpError( 400, "Bad request", "Invalid or badly-formed filename." );
			return false;
		}

		if( file_exists( $path ) ) {
			// Set the filename for more convenient save behavior from browsers
			// FIXME: Is this safe?
			header( 'Content-Disposition: inline; filename="' . $key . '"' );

			require_once 'StreamFile.php';
			wfStreamFile( $path );
		} else {
			return wfHttpError( 404, "Not found",
				"The requested resource does not exist." );
		}
	}

	/**
	 * Confirm that the given file key is valid.
	 * Note that a valid key may refer to a file that does not exist.
	 *
	 * Key should consist of a 31-digit base-36 SHA-1 hash and
	 * an optional alphanumeric extension, all lowercase.
	 * The whole must not exceed 64 characters.
	 *
	 * @param $key
	 * @return boolean
	 */
	static function validKey( $key ) {
		return preg_match( '/^[0-9a-z]{31,32}(\.[0-9a-z]{1,31})?$/', $key );
	}


	/**
	 * Calculate file storage key from a file on disk.
	 * You must pass an extension to it, as some files may be calculated
	 * out of a temporary file etc.
	 *
	 * @param $path to file
	 * @param $extension
	 * @return string or false if could not open file or bad extension
	 */
	static function calculateKey( $path, $extension ) {
		wfSuppressWarnings();
		$hash = sha1_file( $path );
		wfRestoreWarnings();
		if( $hash === false ) {
			wfDebug( __METHOD__.": couldn't hash file '$path'\n" );
			return false;
		}

		$base36 = wfBaseConvert( $hash, 16, 36, 31 );
		if( $extension == '' ) {
			$key = $base36;
		} else {
			$key = $base36 . '.' . $extension;
		}

		// Sanity check
		if( self::validKey( $key ) ) {
			return $key;
		} else {
			wfDebug( __METHOD__.": generated bad key '$key'\n" );
			return false;
		}
	}

	/**
	 * Return filesystem path to the given file.
	 * Note that the file may or may not exist.
	 * @return string or false if an invalid key
	 */
	function filePath( $key ) {
		if( self::validKey( $key ) ) {
			return $this->mDirectory . DIRECTORY_SEPARATOR .
				$this->hashPath( $key, DIRECTORY_SEPARATOR );
		} else {
			return false;
		}
	}

	/**
	 * Return URL path to the given file, if the store is public.
	 * @return string or false if not public
	 */
	function urlPath( $key ) {
		if( $this->mUrl && self::validKey( $key ) ) {
			return $this->mUrl . '/' . $this->hashPath( $key, '/' );
		} else {
			return false;
		}
	}

	private function hashPath( $key, $separator ) {
		$parts = array();
		for( $i = 0; $i < $this->mHashLevel; $i++ ) {
			$parts[] = $key{$i};
		}
		$parts[] = $key;
		return implode( $separator, $parts );
	}
}

/**
 * Wrapper for file store transaction stuff.
 *
 * FileStore methods may return one of these for undoable operations;
 * you can then call its rollback() or commit() methods to perform
 * final cleanup if dependent database work fails or succeeds.
 */
class FSTransaction {
	const DELETE_FILE = 1;

	/**
	 * Combine more items into a fancier transaction
	 */
	function add( FSTransaction $transaction ) {
		$this->mOnCommit = array_merge(
			$this->mOnCommit, $transaction->mOnCommit );
		$this->mOnRollback = array_merge(
			$this->mOnRollback, $transaction->mOnRollback );
	}

	/**
	 * Perform final actions for success.
	 * @return true if actions applied ok, false if errors
	 */
	function commit() {
		return $this->apply( $this->mOnCommit );
	}

	/**
	 * Perform final actions for failure.
	 * @return true if actions applied ok, false if errors
	 */
	function rollback() {
		return $this->apply( $this->mOnRollback );
	}

	// --- Private and friend functions below...

	function __construct() {
		$this->mOnCommit = array();
		$this->mOnRollback = array();
	}

	function addCommit( $action, $path ) {
		$this->mOnCommit[] = array( $action, $path );
	}

	function addRollback( $action, $path ) {
		$this->mOnRollback[] = array( $action, $path );
	}

	private function apply( $actions ) {
		$result = true;
		foreach( $actions as $item ) {
			list( $action, $path ) = $item;
			if( $action == self::DELETE_FILE ) {
				wfSuppressWarnings();
				$ok = unlink( $path );
				wfRestoreWarnings();
				if( $ok )
					wfDebug( __METHOD__.": deleting file '$path'\n" );
				else
					wfDebug( __METHOD__.": failed to delete file '$path'\n" );
				$result = $result && $ok;
			}
		}
		return $result;
	}
}

/**
 * @ingroup Exception
 */
class FSException extends MWException { }
