<?php
/**
 * Category objects are immutable, strictly speaking.  If you call methods that change the database, like to refresh link counts, the objects will be appropriately reinitialized.  Member variables are lazy-initialized.
 *
 * TODO: Move some stuff from CategoryPage.php to here, and use that.
 *
 * @author Simetrical
 */

class Category {
	/** Name of the category, normalized to DB-key form */
	private $mName = null;
	private $mID = null;
	/** Category page title */
	private $mTitle = null;
	/** Counts of membership (cat_pages, cat_subcats, cat_files) */
	private $mPages = null, $mSubcats = null, $mFiles = null;

	private function __construct() {}

	/**
	 * Set up all member variables using a database query.
	 * @return bool True on success, false on failure.
	 */
	protected function initialize() {
		if ( $this->mName === null && $this->mTitle ) 
			$this->mName = $title->getDBKey();

		if( $this->mName === null && $this->mID === null ) {
			throw new MWException( __METHOD__.' has both names and IDs null' );
		} elseif( $this->mID === null ) {
			$where = array( 'cat_title' => $this->mName );
		} elseif( $this->mName === null ) {
			$where = array( 'cat_id' => $this->mID );
		} else {
			# Already initialized
			return true;
		}
		$dbr = wfGetDB( DB_SLAVE );
		$row = $dbr->selectRow(
			'category',
			array( 'cat_id', 'cat_title', 'cat_pages', 'cat_subcats',
				'cat_files' ),
			$where,
			__METHOD__
		);
		if( !$row ) {
			# Okay, there were no contents.  Nothing to initialize.
			if ( $this->mTitle ) {
				# If there is a title object but no record in the category table, treat this as an empty category
				$this->mID      = false;
				$this->mName    = $this->mTitle->getDBKey();
				$this->mPages   = 0;
				$this->mSubcats = 0;
				$this->mFiles   = 0;

				return true;
			} else {
				return false; # Fail
			}
		}
		$this->mID      = $row->cat_id;
		$this->mName    = $row->cat_title;
		$this->mPages   = $row->cat_pages;
		$this->mSubcats = $row->cat_subcats;
		$this->mFiles   = $row->cat_files;

		# (bug 13683) If the count is negative, then 1) it's obviously wrong
		# and should not be kept, and 2) we *probably* don't have to scan many
		# rows to obtain the correct figure, so let's risk a one-time recount.
		if( $this->mPages < 0 || $this->mSubcats < 0 ||
		$this->mFiles < 0 ) {
			$this->refreshCounts();
		}

		return true;
	}

	/**
	 * Factory function.
	 *
	 * @param array $name A category name (no "Category:" prefix).  It need
	 *   not be normalized, with spaces replaced by underscores.
	 * @return mixed Category, or false on a totally invalid name
	 */
	public static function newFromName( $name ) {
		$cat = new self();
		$title = Title::makeTitleSafe( NS_CATEGORY, $name );
		if( !is_object( $title ) ) {
			return false;
		}

		$cat->mTitle = $title;
		$cat->mName = $title->getDBKey();

		return $cat;
	}

	/**
	 * Factory function.
	 *
	 * @param array $title Title for the category page
	 * @return mixed Category, or false on a totally invalid name
	 */
	public static function newFromTitle( $title ) {
		$cat = new self();

		$cat->mTitle = $title;
		$cat->mName = $title->getDBKey();

		return $cat;
	}

	/**
	 * Factory function.
	 *
	 * @param array $id A category id
	 * @return Category
	 */
	public static function newFromID( $id ) {
		$cat = new self();
		$cat->mID = intval( $id );
		return $cat;
	}

	/**
	 * Factory function, for constructing a Category object from a result set
	 *
	 * @param $row result set row, must contain the cat_xxx fields. If the fields are null, 
	 *        the resulting Category object will represent an empty category if a title object
	 *        was given. If the fields are null and no title was given, this method fails and returns false.
	 * @param $title optional title object for the category represented by the given row.
	 *        May be provided if it is already known, to avoid having to re-create a title object later.
	 * @return Category
	 */
	public static function newFromRow( $row, $title = null ) {
		$cat = new self();
		$cat->mTitle = $title;


		# NOTE: the row often results from a LEFT JOIN on categorylinks. This may result in 
		#       all the cat_xxx fields being null, if the category page exists, but nothing
		#       was ever added to the category. This case should be treated linke an empty
		#       category, if possible.

		if ( $row->cat_title === null ) {
			if ( $title === null ) {
				# the name is probably somewhere in the row, for example as page_title,
				# but we can't know that here...
				return false;
			} else {
				$cat->mName = $title->getDBKey(); # if we have a title object, fetch the category name from there
			}

			$cat->mID =   false;
			$cat->mSubcats = 0;
			$cat->mPages   = 0;
			$cat->mFiles   = 0;
		} else {
			$cat->mName    = $row->cat_title;
			$cat->mID      = $row->cat_id;
			$cat->mSubcats = $row->cat_subcats;
			$cat->mPages   = $row->cat_pages;
			$cat->mFiles   = $row->cat_files;
		}

		return $cat;
	}

	/** @return mixed DB key name, or false on failure */
	public function getName() { return $this->getX( 'mName' ); }
	/** @return mixed Category ID, or false on failure */
	public function getID() { return $this->getX( 'mID' ); }
	/** @return mixed Total number of member pages, or false on failure */
	public function getPageCount() { return $this->getX( 'mPages' ); }
	/** @return mixed Number of subcategories, or false on failure */
	public function getSubcatCount() { return $this->getX( 'mSubcats' ); }
	/** @return mixed Number of member files, or false on failure */
	public function getFileCount() { return $this->getX( 'mFiles' ); }

	/**
	 * @return mixed The Title for this category, or false on failure.
	 */
	public function getTitle() {
		if( $this->mTitle ) return $this->mTitle;
		
		if( !$this->initialize() ) {
			return false;
		}

		$this->mTitle = Title::makeTitleSafe( NS_CATEGORY, $this->mName );
		return $this->mTitle;
	}

	/** Generic accessor */
	private function getX( $key ) {
		if( !$this->initialize() ) {
			return false;
		}
		return $this->{$key};
	}

	/**
	 * Refresh the counts for this category.
	 *
	 * @return bool True on success, false on failure
	 */
	public function refreshCounts() {
		if( wfReadOnly() ) {
			return false;
		}
		$dbw = wfGetDB( DB_MASTER );
		$dbw->begin();
		# Note, we must use names for this, since categorylinks does.
		if( $this->mName === null ) {
			if( !$this->initialize() ) {
				return false;
			}
		} else {
			# Let's be sure that the row exists in the table.  We don't need to
			# do this if we got the row from the table in initialization!
			$dbw->insert(
				'category',
				array( 'cat_title' => $this->mName ),
				__METHOD__,
				'IGNORE'
			);
		}

		$cond1 = $dbw->conditional( 'page_namespace='.NS_CATEGORY, 1, 'NULL' );
		$cond2 = $dbw->conditional( 'page_namespace='.NS_IMAGE, 1, 'NULL' );
		$result = $dbw->selectRow(
			array( 'categorylinks', 'page' ),
			array( 'COUNT(*) AS pages',
				   "COUNT($cond1) AS subcats",
				   "COUNT($cond2) AS files"
			),
			array( 'cl_to' => $this->mName, 'page_id = cl_from' ),
			__METHOD__,
			'LOCK IN SHARE MODE'
		);
		$ret = $dbw->update(
			'category',
			array(
				'cat_pages' => $result->pages,
				'cat_subcats' => $result->subcats,
				'cat_files' => $result->files
			),
			array( 'cat_title' => $this->mName ),
			__METHOD__
		);
		$dbw->commit();

		# Now we should update our local counts.
		$this->mPages   = $result->pages;
		$this->mSubcats = $result->subcats;
		$this->mFiles   = $result->files;

		return $ret;
	}
}
