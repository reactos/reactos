<?php

/**
 * The "Categoryfinder" class takes a list of articles, creates an internal
 * representation of all their parent categories (as well as parents of
 * parents etc.). From this representation, it determines which of these
 * articles are in one or all of a given subset of categories.
 *
 * Example use :
 * <code>
 * 	# Determines whether the article with the page_id 12345 is in both
 * 	# "Category 1" and "Category 2" or their subcategories, respectively
 *
 * 	$cf = new Categoryfinder ;
 * 	$cf->seed (
 * 		array ( 12345 ) ,
 * 		array ( "Category 1","Category 2" ) ,
 * 		"AND"
 * 	) ;
 * 	$a = $cf->run() ;
 * 	print implode ( "," , $a ) ;
 * </code>
 *
 */
class Categoryfinder {

	var $articles = array () ; # The original article IDs passed to the seed function
	var $deadend = array () ; # Array of DBKEY category names for categories that don't have a page
	var $parents = array () ; # Array of [ID => array()]
	var $next = array () ; # Array of article/category IDs
	var $targets = array () ; # Array of DBKEY category names
	var $name2id = array () ;
	var $mode ; # "AND" or "OR"
	var $dbr ; # Read-DB slave

	/**
	 * Constructor (currently empty).
	 */
	function __construct() {
	}

	/**
	 * Initializes the instance. Do this prior to calling run().
	 * @param $article_ids Array of article IDs
	 * @param $categories FIXME
	 * @param $mode String: FIXME, default 'AND'.
	 */
	function seed ( $article_ids , $categories , $mode = "AND" ) {
		$this->articles = $article_ids ;
		$this->next = $article_ids ;
		$this->mode = $mode ;

		# Set the list of target categories; convert them to DBKEY form first
		$this->targets = array () ;
		foreach ( $categories AS $c ) {
			$ct = Title::newFromText ( $c , NS_CATEGORY ) ;
			$c = $ct->getDBkey () ;
			$this->targets[$c] = $c ;
		}
	}

	/**
	 * Iterates through the parent tree starting with the seed values,
	 * then checks the articles if they match the conditions
	 * @return array of page_ids (those given to seed() that match the conditions)
	 */
	function run () {
		$this->dbr = wfGetDB( DB_SLAVE );
		while ( count ( $this->next ) > 0 ) {
			$this->scan_next_layer () ;
		}

		# Now check if this applies to the individual articles
		$ret = array () ;
		foreach ( $this->articles AS $article ) {
			$conds = $this->targets ;
			if ( $this->check ( $article , $conds ) ) {
				# Matches the conditions
				$ret[] = $article ;
			}
		}
		return $ret ;
	}

	/**
	 * This functions recurses through the parent representation, trying to match the conditions
	 * @param $id The article/category to check
	 * @param $conds The array of categories to match
	 * @return bool Does this match the conditions?
	 */
	function check ( $id , &$conds ) {
		# Shortcut (runtime paranoia): No contitions=all matched
		if ( count ( $conds ) == 0 ) return true ;

		if ( !isset ( $this->parents[$id] ) ) return false ;

		# iterate through the parents
		foreach ( $this->parents[$id] AS $p ) {
			$pname = $p->cl_to ;

			# Is this a condition?
			if ( isset ( $conds[$pname] ) ) {
				# This key is in the category list!
				if ( $this->mode == "OR" ) {
					# One found, that's enough!
					$conds = array () ;
					return true ;
				} else {
					# Assuming "AND" as default
					unset ( $conds[$pname] ) ;
					if ( count ( $conds ) == 0 ) {
						# All conditions met, done
						return true ;
					}
				}
			}

			# Not done yet, try sub-parents
			if ( !isset ( $this->name2id[$pname] ) ) {
				# No sub-parent
				continue ;
			}
			$done = $this->check ( $this->name2id[$pname] , $conds ) ;
			if ( $done OR count ( $conds ) == 0 ) {
				# Subparents have done it!
				return true ;
			}
		}
		return false ;
	}

	/**
	 * Scans a "parent layer" of the articles/categories in $this->next
	 */
	function scan_next_layer () {
		$fname = "Categoryfinder::scan_next_layer" ;

		# Find all parents of the article currently in $this->next
		$layer = array () ;
		$res = $this->dbr->select(
				/* FROM   */ 'categorylinks',
				/* SELECT */ '*',
				/* WHERE  */ array( 'cl_from' => $this->next ),
				$fname."-1"
		);
		while ( $o = $this->dbr->fetchObject( $res ) ) {
			$k = $o->cl_to ;

			# Update parent tree
			if ( !isset ( $this->parents[$o->cl_from] ) ) {
				$this->parents[$o->cl_from] = array () ;
			}
			$this->parents[$o->cl_from][$k] = $o ;

			# Ignore those we already have
			if ( in_array ( $k , $this->deadend ) ) continue ;
			if ( isset ( $this->name2id[$k] ) ) continue ;

			# Hey, new category!
			$layer[$k] = $k ;
		}
		$this->dbr->freeResult( $res ) ;

		$this->next = array() ;

		# Find the IDs of all category pages in $layer, if they exist
		if ( count ( $layer ) > 0 ) {
			$res = $this->dbr->select(
					/* FROM   */ 'page',
					/* SELECT */ 'page_id,page_title',
					/* WHERE  */ array( 'page_namespace' => NS_CATEGORY , 'page_title' => $layer ),
					$fname."-2"
			);
			while ( $o = $this->dbr->fetchObject( $res ) ) {
				$id = $o->page_id ;
				$name = $o->page_title ;
				$this->name2id[$name] = $id ;
				$this->next[] = $id ;
				unset ( $layer[$name] ) ;
			}
			$this->dbr->freeResult( $res ) ;
			}

		# Mark dead ends
		foreach ( $layer AS $v ) {
			$this->deadend[$v] = $v ;
		}
	}

} # END OF CLASS "Categoryfinder"
