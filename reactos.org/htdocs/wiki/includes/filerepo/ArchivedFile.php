<?php

/**
 * @ingroup Media
 */
class ArchivedFile
{
	/**#@+
	 * @private
	 */
	var $id, # filearchive row ID
		$title, # image title
		$name, # image name
		$group,	# FileStore storage group
		$key, # FileStore sha1 key
		$size, # file dimensions
		$bits,	# size in bytes
		$width, # width
		$height, # height
		$metadata, # metadata string
		$mime, # mime type
		$media_type, # media type
		$description, # upload description
		$user, # user ID of uploader
		$user_text, # user name of uploader
		$timestamp, # time of upload
		$dataLoaded, # Whether or not all this has been loaded from the database (loadFromXxx)
		$deleted; # Bitfield akin to rev_deleted

	/**#@-*/

	function ArchivedFile( $title, $id=0, $key='' ) {
		if( !is_object($title) ) {
			throw new MWException( 'ArchivedFile constructor given bogus title.' );
		}
		$this->id = -1;
		$this->title = $title;
		$this->name = $title->getDBkey();
		$this->group = '';
		$this->key = '';
		$this->size = 0;
		$this->bits = 0;
		$this->width = 0;
		$this->height = 0;
		$this->metadata = '';
		$this->mime = "unknown/unknown";
		$this->media_type = '';
		$this->description = '';
		$this->user = 0;
		$this->user_text = '';
		$this->timestamp = NULL;
		$this->deleted = 0;
		$this->dataLoaded = false;
	}

	/**
	 * Loads a file object from the filearchive table
	 * @return ResultWrapper
	 */
	public function load() {
		if ( $this->dataLoaded ) {
			return true;
		}
		$conds = ($this->id) ? "fa_id = {$this->id}" : "fa_storage_key = '{$this->key}'";
		if( $this->title->getNamespace() == NS_IMAGE ) {
			$dbr = wfGetDB( DB_SLAVE );
			$res = $dbr->select( 'filearchive',
				array(
					'fa_id',
					'fa_name',
					'fa_archive_name',
					'fa_storage_key',
					'fa_storage_group',
					'fa_size',
					'fa_bits',
					'fa_width',
					'fa_height',
					'fa_metadata',
					'fa_media_type',
					'fa_major_mime',
					'fa_minor_mime',
					'fa_description',
					'fa_user',
					'fa_user_text',
					'fa_timestamp',
					'fa_deleted' ),
				array(
					'fa_name' => $this->title->getDBkey(),
					$conds ),
				__METHOD__,
				array( 'ORDER BY' => 'fa_timestamp DESC' ) );

			if ( $dbr->numRows( $res ) == 0 ) {
			// this revision does not exist?
				return;
			}
			$ret = $dbr->resultObject( $res );
			$row = $ret->fetchObject();

			// initialize fields for filestore image object
			$this->id = intval($row->fa_id);
			$this->name = $row->fa_name;
			$this->archive_name = $row->fa_archive_name;
			$this->group = $row->fa_storage_group;
			$this->key = $row->fa_storage_key;
			$this->size = $row->fa_size;
			$this->bits = $row->fa_bits;
			$this->width = $row->fa_width;
			$this->height = $row->fa_height;
			$this->metadata = $row->fa_metadata;
			$this->mime = "$row->fa_major_mime/$row->fa_minor_mime";
			$this->media_type = $row->fa_media_type;
			$this->description = $row->fa_description;
			$this->user = $row->fa_user;
			$this->user_text = $row->fa_user_text;
			$this->timestamp = $row->fa_timestamp;
			$this->deleted = $row->fa_deleted;
		} else {
			throw new MWException( 'This title does not correspond to an image page.' );
			return;
		}
		$this->dataLoaded = true;

		return true;
	}

	/**
	 * Loads a file object from the filearchive table
	 * @return ResultWrapper
	 */
	public static function newFromRow( $row ) {
		$file = new ArchivedFile( Title::makeTitle( NS_IMAGE, $row->fa_name ) );

		$file->id = intval($row->fa_id);
		$file->name = $row->fa_name;
		$file->archive_name = $row->fa_archive_name;
		$file->group = $row->fa_storage_group;
		$file->key = $row->fa_storage_key;
		$file->size = $row->fa_size;
		$file->bits = $row->fa_bits;
		$file->width = $row->fa_width;
		$file->height = $row->fa_height;
		$file->metadata = $row->fa_metadata;
		$file->mime = "$row->fa_major_mime/$row->fa_minor_mime";
		$file->media_type = $row->fa_media_type;
		$file->description = $row->fa_description;
		$file->user = $row->fa_user;
		$file->user_text = $row->fa_user_text;
		$file->timestamp = $row->fa_timestamp;
		$file->deleted = $row->fa_deleted;

		return $file;
	}

	/**
	 * Return the associated title object
	 * @public
	 */
	public function getTitle() {
		return $this->title;
	}

	/**
	 * Return the file name
	 */
	public function getName() {
		return $this->name;
	}

	public function getID() {
		$this->load();
		return $this->id;
	}

	/**
	 * Return the FileStore key
	 */
	public function getKey() {
		$this->load();
		return $this->key;
	}

	/**
	 * Return the FileStore storage group
	 */
	public function getGroup() {
		return $file->group;
	}

	/**
	 * Return the width of the image
	 */
	public function getWidth() {
		$this->load();
		return $this->width;
	}

	/**
	 * Return the height of the image
	 */
	public function getHeight() {
		$this->load();
		return $this->height;
	}

	/**
	 * Get handler-specific metadata
	 */
	public function getMetadata() {
		$this->load();
		return $this->metadata;
	}

	/**
	 * Return the size of the image file, in bytes
	 * @public
	 */
	public function getSize() {
		$this->load();
		return $this->size;
	}

	/**
	 * Return the bits of the image file, in bytes
	 * @public
	 */
	public function getBits() {
		$this->load();
		return $this->bits;
	}

	/**
	 * Returns the mime type of the file.
	 */
	public function getMimeType() {
		$this->load();
		return $this->mime;
	}

	/**
	 * Return the type of the media in the file.
	 * Use the value returned by this function with the MEDIATYPE_xxx constants.
	 */
	public function getMediaType() {
		$this->load();
		return $this->media_type;
	}

	/**
	 * Return upload timestamp.
	 */
	public function getTimestamp() {
		$this->load();
		return $this->timestamp;
	}

	/**
	 * Return the user ID of the uploader.
	 */
	public function getUser() {
		$this->load();
		if( $this->isDeleted( File::DELETED_USER ) ) {
			return 0;
		} else {
			return $this->user;
		}
	}

	/**
	 * Return the user name of the uploader.
	 */
	public function getUserText() {
		$this->load();
		if( $this->isDeleted( File::DELETED_USER ) ) {
			return 0;
		} else {
			return $this->user_text;
		}
	}

	/**
	 * Return upload description.
	 */
	public function getDescription() {
		$this->load();
		if( $this->isDeleted( File::DELETED_COMMENT ) ) {
			return 0;
		} else {
			return $this->description;
		}
	}

	/**
	 * Return the user ID of the uploader.
	 */
	public function getRawUser() {
		$this->load();
		return $this->user;
	}

	/**
	 * Return the user name of the uploader.
	 */
	public function getRawUserText() {
		$this->load();
		return $this->user_text;
	}

	/**
	 * Return upload description.
	 */
	public function getRawDescription() {
		$this->load();
		return $this->description;
	}

	/**
	 * int $field one of DELETED_* bitfield constants
	 * for file or revision rows
	 * @return bool
	 */
	public function isDeleted( $field ) {
		return ($this->deleted & $field) == $field;
	}

	/**
	 * Determine if the current user is allowed to view a particular
	 * field of this FileStore image file, if it's marked as deleted.
	 * @param int $field
	 * @return bool
	 */
	public function userCan( $field ) {
		if( ($this->deleted & $field) == $field ) {
			global $wgUser;
			$permission = ( $this->deleted & File::DELETED_RESTRICTED ) == File::DELETED_RESTRICTED
				? 'suppressrevision'
				: 'deleterevision';
			wfDebug( "Checking for $permission due to $field match on $this->deleted\n" );
			return $wgUser->isAllowed( $permission );
		} else {
			return true;
		}
	}
}
