<?php
# Copyright (C) 2006-2007 Greg Sabino Mullane <greg@turnstep.com>
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

/**
 * @file
 * @ingroup Search
 */

/**
 * Search engine hook base class for Postgres
 * @ingroup Search
 */
class SearchPostgres extends SearchEngine {

	function __construct( $db ) {
		$this->db = $db;
	}

	/**
	 * Perform a full text search query via tsearch2 and return a result set.
	 * Currently searches a page's current title (page.page_title) and
	 * latest revision article text (pagecontent.old_text)
	 *
	 * @param string $term - Raw search term
	 * @return PostgresSearchResultSet
	 * @access public
	 */
	function searchTitle( $term ) {
		$q = $this->searchQuery( $term , 'titlevector', 'page_title' );
		$olderror = error_reporting(E_ERROR);
		$resultSet = $this->db->resultObject( $this->db->query( $q, 'SearchPostgres', true ) );
		error_reporting($olderror);
		if (!$resultSet) {
			// Needed for "Query requires full scan, GIN doesn't support it"
			return new SearchResultTooMany();
		}
		return new PostgresSearchResultSet( $resultSet, $this->searchTerms );
	}
	function searchText( $term ) {
		$q = $this->searchQuery( $term, 'textvector', 'old_text' );
		$olderror = error_reporting(E_ERROR);
		$resultSet = $this->db->resultObject( $this->db->query( $q, 'SearchPostgres', true ) );
		error_reporting($olderror);
		if (!$resultSet) {
			return new SearchResultTooMany();
		}
		return new PostgresSearchResultSet( $resultSet, $this->searchTerms );
	}


	/*
	 * Transform the user's search string into a better form for tsearch2
	*/
	function parseQuery( $term ) {

		wfDebug( "parseQuery received: $term" );

		## No backslashes allowed
		$term = preg_replace('/\\\/', '', $term);

		## Collapse parens into nearby words:
		$term = preg_replace('/\s*\(\s*/', ' (', $term);
		$term = preg_replace('/\s*\)\s*/', ') ', $term);

		## Treat colons as word separators:
		$term = preg_replace('/:/', ' ', $term);

		$searchstring = '';
		$m = array();
		if( preg_match_all('/([-!]?)(\S+)\s*/', $term, $m, PREG_SET_ORDER ) ) {
			foreach( $m as $terms ) {
				if (strlen($terms[1])) {
					$searchstring .= ' & !';
				}
				if (strtolower($terms[2]) === 'and') {
					$searchstring .= ' & ';
				}
				else if (strtolower($terms[2]) === 'or' or $terms[2] === '|') {
					$searchstring .= ' | ';
				}
				else if (strtolower($terms[2]) === 'not') {
					$searchstring .= ' & !';
				}
				else {
					$searchstring .= " & $terms[2]";
				}
			}
		}

		## Strip out leading junk
		$searchstring = preg_replace('/^[\s\&\|]+/', '', $searchstring);

		## Remove any doubled-up operators
		$searchstring = preg_replace('/([\!\&\|]) +(?:[\&\|] +)+/', "$1 ", $searchstring);

		## Remove any non-spaced operators (e.g. "Zounds!")
		$searchstring = preg_replace('/([^ ])[\!\&\|]/', "$1", $searchstring);

		## Remove any trailing whitespace or operators
		$searchstring = preg_replace('/[\s\!\&\|]+$/', '', $searchstring);

		## Remove unnecessary quotes around everything
		$searchstring = preg_replace('/^[\'"](.*)[\'"]$/', "$1", $searchstring);

		## Quote the whole thing
		$searchstring = $this->db->addQuotes($searchstring);

		wfDebug( "parseQuery returned: $searchstring" );

		return $searchstring;

	}

	/**
	 * Construct the full SQL query to do the search.
	 * @param string $filteredTerm
	 * @param string $fulltext
	 * @private
	 */
	function searchQuery( $term, $fulltext, $colname ) {
		global $wgDBversion;

		if ( !isset( $wgDBversion ) ) {
			$this->db->getServerVersion();
			$wgDBversion = $this->db->numeric_version;
		}
		$prefix = $wgDBversion < 8.3 ? "'default'," : '';

		$searchstring = $this->parseQuery( $term );

		## We need a separate query here so gin does not complain about empty searches
		$SQL = "SELECT to_tsquery($prefix $searchstring)";
		$res = $this->db->doQuery($SQL);
		if (!$res) {
			## TODO: Better output (example to catch: one 'two)
			die ("Sorry, that was not a valid search string. Please go back and try again");
		}
		$top = pg_fetch_result($res,0,0);

		if ($top === "") { ## e.g. if only stopwords are used XXX return something better
			$query = "SELECT page_id, page_namespace, page_title, 0 AS score ".
				"FROM page p, revision r, pagecontent c WHERE p.page_latest = r.rev_id " .
				"AND r.rev_text_id = c.old_id AND 1=0";
		}
		else {
			$m = array();
			if( preg_match_all("/'([^']+)'/", $top, $m, PREG_SET_ORDER ) ) {
				foreach( $m as $terms ) {
					$this->searchTerms[$terms[1]] = $terms[1];
				}
			}

			$rankscore = $wgDBversion > 8.2 ? 5 : 1;
			$rank = $wgDBversion < 8.3 ? 'rank' : 'ts_rank';
			$query = "SELECT page_id, page_namespace, page_title, ".
			"$rank($fulltext, to_tsquery($prefix $searchstring), $rankscore) AS score ".
			"FROM page p, revision r, pagecontent c WHERE p.page_latest = r.rev_id " .
			"AND r.rev_text_id = c.old_id AND $fulltext @@ to_tsquery($prefix $searchstring)";
		}

		## Redirects
		if (! $this->showRedirects)
			$query .= ' AND page_is_redirect = 0';

		## Namespaces - defaults to 0
		if( !is_null($this->namespaces) ){ // null -> search all
			if ( count($this->namespaces) < 1)
				$query .= ' AND page_namespace = 0';
			else {
				$namespaces = implode( ',', $this->namespaces );
				$query .= " AND page_namespace IN ($namespaces)";
			}
		}

		$query .= " ORDER BY score DESC, page_id DESC";

		$query .= $this->db->limitResult( '', $this->limit, $this->offset );

		wfDebug( "searchQuery returned: $query" );

		return $query;
	}

	## Most of the work of these two functions are done automatically via triggers

	function update( $pageid, $title, $text ) {
		## We don't want to index older revisions
		$SQL = "UPDATE pagecontent SET textvector = NULL WHERE old_id = ".
				"(SELECT rev_text_id FROM revision WHERE rev_page = $pageid ".
				"ORDER BY rev_text_id DESC LIMIT 1 OFFSET 1)";
		$this->db->doQuery($SQL);
		return true;
	}

	function updateTitle( $id, $title ) {
		return true;
	}

} ## end of the SearchPostgres class

/**
 * @ingroup Search
 */
class PostgresSearchResult extends SearchResult {
	function __construct( $row ) {
		parent::__construct($row);
		$this->score = $row->score;
	}
	function getScore() {
		return $this->score;
	}
}

/**
 * @ingroup Search
 */
class PostgresSearchResultSet extends SearchResultSet {
	function __construct( $resultSet, $terms ) {
		$this->mResultSet = $resultSet;
		$this->mTerms = $terms;
	}

	function termMatches() {
		return $this->mTerms;
	}

	function numRows() {
		return $this->mResultSet->numRows();
	}

	function next() {
		$row = $this->mResultSet->fetchObject();
		if( $row === false ) {
			return false;
		} else {
			return new PostgresSearchResult( $row );
		}
	}
}
