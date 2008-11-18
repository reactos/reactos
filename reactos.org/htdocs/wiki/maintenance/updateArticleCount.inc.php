<?php
/**
 * Support class for the updateArticleCount.php maintenance script
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

class ArticleCounter {

	var $dbr;
	var $namespaces;
	
	function ArticleCounter() {
		global $wgContentNamespaces;
		$this->namespaces = $wgContentNamespaces;
		$this->dbr = wfGetDB( DB_SLAVE );
	}
	
	/**
	 * Produce a comma-delimited set of namespaces
	 * Includes paranoia
	 *
	 * @return string
	 */
	function makeNsSet() {
		foreach( $this->namespaces as $namespace )
			$namespaces[] = intval( $namespace );
		return implode( ', ', $namespaces );
	}
	
	/**
	 * Produce SQL for the query
	 *
	 * @return string
	 */
	function makeSql() {
		list( $page, $pagelinks ) = $this->dbr->tableNamesN( 'page', 'pagelinks' );
		$nsset = $this->makeNsSet();
		return "SELECT DISTINCT page_namespace,page_title FROM $page,$pagelinks " .
			"WHERE pl_from=page_id and page_namespace IN ( $nsset ) " .
			"AND page_is_redirect = 0 AND page_len > 0";
	}
	
	/**
	 * Count the number of valid content pages in the wiki
	 *
	 * @return mixed Integer, or false if there's a problem
	 */
	function count() {
		$res = $this->dbr->query( $this->makeSql(), __METHOD__ );
		if( $res ) {
			$count = $this->dbr->numRows( $res );
			$this->dbr->freeResult( $res );
			return $count;
		} else {
			# Look out for this when handling the result
			#    - Actually it's unreachable, !$res throws an exception -- TS
			return false; 
		}
	}

}


