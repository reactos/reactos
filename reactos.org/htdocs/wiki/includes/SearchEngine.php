<?php
/**
 * @defgroup Search Search
 *
 * @file
 * @ingroup Search
 */

/**
 * Contain a class for special pages
 * @ingroup Search
 */
class SearchEngine {
	var $limit = 10;
	var $offset = 0;
	var $searchTerms = array();
	var $namespaces = array( NS_MAIN );
	var $showRedirects = false;

	/**
	 * Perform a full text search query and return a result set.
	 * If title searches are not supported or disabled, return null.
	 *
	 * @param string $term - Raw search term
	 * @return SearchResultSet
	 * @access public
	 * @abstract
	 */
	function searchText( $term ) {
		return null;
	}

	/**
	 * Perform a title-only search query and return a result set.
	 * If title searches are not supported or disabled, return null.
	 *
	 * @param string $term - Raw search term
	 * @return SearchResultSet
	 * @access public
	 * @abstract
	 */
	function searchTitle( $term ) {
		return null;
	}
	
	/**
	 * If an exact title match can be find, or a very slightly close match,
	 * return the title. If no match, returns NULL.
	 *
	 * @param string $term
	 * @return Title
	 */
	public static function getNearMatch( $searchterm ) {
		global $wgContLang;

		$allSearchTerms = array($searchterm);

		if($wgContLang->hasVariants()){
			$allSearchTerms = array_merge($allSearchTerms,$wgContLang->convertLinkToAllVariants($searchterm));
		}

		foreach($allSearchTerms as $term){

			# Exact match? No need to look further.
			$title = Title::newFromText( $term );
			if (is_null($title))
				return NULL;

			if ( $title->getNamespace() == NS_SPECIAL || $title->isExternal() 
			     || $title->exists() ) {
				return $title;
			}

			# Now try all lower case (i.e. first letter capitalized)
			#
			$title = Title::newFromText( $wgContLang->lc( $term ) );
			if ( $title && $title->exists() ) {
				return $title;
			}

			# Now try capitalized string
			#
			$title = Title::newFromText( $wgContLang->ucwords( $term ) );
			if ( $title && $title->exists() ) {
				return $title;
			}

			# Now try all upper case
			#
			$title = Title::newFromText( $wgContLang->uc( $term ) );
			if ( $title && $title->exists() ) {
				return $title;
			}

			# Now try Word-Caps-Breaking-At-Word-Breaks, for hyphenated names etc
			$title = Title::newFromText( $wgContLang->ucwordbreaks($term) );
			if ( $title && $title->exists() ) {
				return $title;
			}

			global $wgCapitalLinks, $wgContLang;
			if( !$wgCapitalLinks ) {
				// Catch differs-by-first-letter-case-only
				$title = Title::newFromText( $wgContLang->ucfirst( $term ) );
				if ( $title && $title->exists() ) {
					return $title;
				}
				$title = Title::newFromText( $wgContLang->lcfirst( $term ) );
				if ( $title && $title->exists() ) {
					return $title;
				}
			}

			// Give hooks a chance at better match variants
			$title = null;
			if( !wfRunHooks( 'SearchGetNearMatch', array( $term, &$title ) ) ) {
				return $title;
			}
		}

		$title = Title::newFromText( $searchterm );

		# Entering an IP address goes to the contributions page
		if ( ( $title->getNamespace() == NS_USER && User::isIP($title->getText() ) )
			|| User::isIP( trim( $searchterm ) ) ) {
			return SpecialPage::getTitleFor( 'Contributions', $title->getDBkey() );
		}


		# Entering a user goes to the user page whether it's there or not
		if ( $title->getNamespace() == NS_USER ) {
			return $title;
		}

		# Go to images that exist even if there's no local page.
		# There may have been a funny upload, or it may be on a shared
		# file repository such as Wikimedia Commons.
		if( $title->getNamespace() == NS_IMAGE ) {
			$image = wfFindFile( $title );
			if( $image ) {
				return $title;
			}
		}

		# MediaWiki namespace? Page may be "implied" if not customized.
		# Just return it, with caps forced as the message system likes it.
		if( $title->getNamespace() == NS_MEDIAWIKI ) {
			return Title::makeTitle( NS_MEDIAWIKI, $wgContLang->ucfirst( $title->getText() ) );
		}

		# Quoted term? Try without the quotes...
		$matches = array();
		if( preg_match( '/^"([^"]+)"$/', $searchterm, $matches ) ) {
			return SearchEngine::getNearMatch( $matches[1] );
		}

		return NULL;
	}

	public static function legalSearchChars() {
		return "A-Za-z_'0-9\\x80-\\xFF\\-";
	}

	/**
	 * Set the maximum number of results to return
	 * and how many to skip before returning the first.
	 *
	 * @param int $limit
	 * @param int $offset
	 * @access public
	 */
	function setLimitOffset( $limit, $offset = 0 ) {
		$this->limit = intval( $limit );
		$this->offset = intval( $offset );
	}

	/**
	 * Set which namespaces the search should include.
	 * Give an array of namespace index numbers.
	 *
	 * @param array $namespaces
	 * @access public
	 */
	function setNamespaces( $namespaces ) {
		$this->namespaces = $namespaces;
	}

	/**
	 * Parse some common prefixes: all (search everything)
	 * or namespace names
	 *
	 * @param string $query
	 */
	function replacePrefixes( $query ){
		global $wgContLang;

		if( strpos($query,':') === false )
			return $query; // nothing to do

		$parsed = $query;
		$allkeyword = wfMsgForContent('searchall').":";
		if( strncmp($query, $allkeyword, strlen($allkeyword)) == 0 ){
			$this->namespaces = null;
			$parsed = substr($query,strlen($allkeyword));
		} else if( strpos($query,':') !== false ) {
			$prefix = substr($query,0,strpos($query,':'));
			$index = $wgContLang->getNsIndex($prefix);
			if($index !== false){
				$this->namespaces = array($index);
				$parsed = substr($query,strlen($prefix)+1);
			}
		}
		if(trim($parsed) == '')
			return $query; // prefix was the whole query

		return $parsed;
	}

	/**
	 * Make a list of searchable namespaces and their canonical names.
	 * @return array
	 */
	public static function searchableNamespaces() {
		global $wgContLang;
		$arr = array();
		foreach( $wgContLang->getNamespaces() as $ns => $name ) {
			if( $ns >= NS_MAIN ) {
				$arr[$ns] = $name;
			}
		}
		return $arr;
	}
	
	/**
	 * Extract default namespaces to search from the given user's
	 * settings, returning a list of index numbers.
	 *
	 * @param User $user
	 * @return array
	 * @static 
	 */
	public static function userNamespaces( &$user ) {
		$arr = array();
		foreach( SearchEngine::searchableNamespaces() as $ns => $name ) {
			if( $user->getOption( 'searchNs' . $ns ) ) {
				$arr[] = $ns;
			}
		}
		return $arr;
	}
	
	/**
	 * Find snippet highlight settings for a given user
	 *
	 * @param User $user
	 * @return array contextlines, contextchars 
	 * @static
	 */
	public static function userHighlightPrefs( &$user ){
		//$contextlines = $user->getOption( 'contextlines',  5 );
		//$contextchars = $user->getOption( 'contextchars', 50 );
		$contextlines = 2; // Hardcode this. Old defaults sucked. :)
		$contextchars = 75; // same as above.... :P
		return array($contextlines, $contextchars);
	}
	
	/**
	 * An array of namespaces indexes to be searched by default
	 * 
	 * @return array 
	 * @static
	 */
	public static function defaultNamespaces(){
		global $wgNamespacesToBeSearchedDefault;
		
		return array_keys($wgNamespacesToBeSearchedDefault, true);
	}

	/**
	 * Return a 'cleaned up' search string
	 *
	 * @return string
	 * @access public
	 */
	function filter( $text ) {
		$lc = $this->legalSearchChars();
		return trim( preg_replace( "/[^{$lc}]/", " ", $text ) );
	}
	/**
	 * Load up the appropriate search engine class for the currently
	 * active database backend, and return a configured instance.
	 *
	 * @fixme Ask the database class for his default search class
	 * instead of knowing about every backend here.
	 * @return SearchEngine
	 */
	public static function create() {
		global $wgDBtype, $wgSearchType;
		if( $wgSearchType ) {
			$class = $wgSearchType;
		} elseif( $wgDBtype == 'mysql' ) {
			$class = 'SearchMySQL';
		} else if ( $wgDBtype == 'postgres' ) {
			$class = 'SearchPostgres';
		} else if ( $wgDBtype == 'oracle' ) {
			$class = 'SearchOracle';
		} else {
			$class = 'SearchEngineDummy';
		}
		$search = new $class( wfGetDB( DB_SLAVE ) );
		$search->setLimitOffset(0,0);
		return $search;
	}

	/**
	 * Create or update the search index record for the given page.
	 * Title and text should be pre-processed.
	 *
	 * @param int $id
	 * @param string $title
	 * @param string $text
	 * @abstract
	 */
	function update( $id, $title, $text ) {
		// no-op
	}

	/**
	 * Update a search index record's title only.
	 * Title should be pre-processed.
	 *
	 * @param int $id
	 * @param string $title
	 * @abstract
	 */
	function updateTitle( $id, $title ) {
		// no-op
	}
	
	/**
	 * Get OpenSearch suggestion template
	 * 
	 * @return string
	 * @static 
	 */
	public static function getOpenSearchTemplate() {
		global $wgOpenSearchTemplate, $wgServer, $wgScriptPath;
		if($wgOpenSearchTemplate)		
			return $wgOpenSearchTemplate;
		else{ 
			$ns = implode(',',SearchEngine::defaultNamespaces());
			if(!$ns) $ns = "0";
			return $wgServer . $wgScriptPath . '/api.php?action=opensearch&search={searchTerms}&namespace='.$ns;
		}
	}
	
	/**
	 * Get internal MediaWiki Suggest template 
	 * 
	 * @return string
	 * @static
	 */
	public static function getMWSuggestTemplate() {
		global $wgMWSuggestTemplate, $wgServer, $wgScriptPath;
		if($wgMWSuggestTemplate)		
			return $wgMWSuggestTemplate;
		else 
			return $wgServer . $wgScriptPath . '/api.php?action=opensearch&search={searchTerms}&namespace={namespaces}';
	}
}

/**
 * @ingroup Search
 */
class SearchResultSet {
	/**
	 * Fetch an array of regular expression fragments for matching
	 * the search terms as parsed by this engine in a text extract.
	 *
	 * @return array
	 * @access public
	 * @abstract
	 */
	function termMatches() {
		return array();
	}

	function numRows() {
		return 0;
	}

	/**
	 * Return true if results are included in this result set.
	 * @return bool
	 * @abstract
	 */
	function hasResults() {
		return false;
	}

	/**
	 * Some search modes return a total hit count for the query
	 * in the entire article database. This may include pages
	 * in namespaces that would not be matched on the given
	 * settings.
	 *
	 * Return null if no total hits number is supported.
	 *
	 * @return int
	 * @access public
	 */
	function getTotalHits() {
		return null;
	}

	/**
	 * Some search modes return a suggested alternate term if there are
	 * no exact hits. Returns true if there is one on this set.
	 *
	 * @return bool
	 * @access public
	 */
	function hasSuggestion() {
		return false;
	}

	/**
	 * @return string suggested query, null if none
	 */
	function getSuggestionQuery(){
		return null;
	}

	/**
	 * @return string highlighted suggested query, '' if none
	 */
	function getSuggestionSnippet(){
		return '';
	}
	
	/**
	 * Return information about how and from where the results were fetched,
	 * should be useful for diagnostics and debugging 
	 *
	 * @return string
	 */
	function getInfo() {
		return null;
	}
	
	/**
	 * Return a result set of hits on other (multiple) wikis associated with this one
	 *
	 * @return SearchResultSet
	 */
	function getInterwikiResults() {
		return null;
	}
	
	/**
	 * Check if there are results on other wikis
	 *
	 * @return boolean
	 */
	function hasInterwikiResults() {
		return $this->getInterwikiResults() != null;
	}
	

	/**
	 * Fetches next search result, or false.
	 * @return SearchResult
	 * @access public
	 * @abstract
	 */
	function next() {
		return false;
	}

	/**
	 * Frees the result set, if applicable.
	 * @ access public
	 */
	function free() {
		// ...
	}
}


/**
 * @ingroup Search
 */
class SearchResultTooMany {
	## Some search engines may bail out if too many matches are found
}


/**
 * @fixme This class is horribly factored. It would probably be better to have
 * a useful base class to which you pass some standard information, then let
 * the fancy self-highlighters extend that.
 * @ingroup Search
 */
class SearchResult {
	var $mRevision = null;

	function SearchResult( $row ) {
		$this->mTitle = Title::makeTitle( $row->page_namespace, $row->page_title );
		if( !is_null($this->mTitle) )
			$this->mRevision = Revision::newFromTitle( $this->mTitle );
	}
	
	/**
	 * Check if this is result points to an invalid title
	 *
	 * @return boolean
	 * @access public
	 */
	function isBrokenTitle(){
		if( is_null($this->mTitle) )
			return true;
		return false;
	}
	
	/**
	 * Check if target page is missing, happens when index is out of date
	 * 
	 * @return boolean
	 * @access public
	 */
	function isMissingRevision(){
		if( !$this->mRevision )
			return true;
		return false;
	}

	/**
	 * @return Title
	 * @access public
	 */
	function getTitle() {
		return $this->mTitle;
	}

	/**
	 * @return double or null if not supported
	 */
	function getScore() {
		return null;
	}

	/**
	 * Lazy initialization of article text from DB
	 */
	protected function initText(){
		if( !isset($this->mText) ){
			$this->mText = $this->mRevision->getText();
		}
	}
	
	/**
	 * @param array $terms terms to highlight
	 * @return string highlighted text snippet, null (and not '') if not supported 
	 */
	function getTextSnippet($terms){
		global $wgUser, $wgAdvancedSearchHighlighting;
		$this->initText();
		list($contextlines,$contextchars) = SearchEngine::userHighlightPrefs($wgUser);
		$h = new SearchHighlighter();
		if( $wgAdvancedSearchHighlighting )
			return $h->highlightText( $this->mText, $terms, $contextlines, $contextchars );
		else
			return $h->highlightSimple( $this->mText, $terms, $contextlines, $contextchars );
	}
	
	/**
	 * @param array $terms terms to highlight
	 * @return string highlighted title, '' if not supported
	 */
	function getTitleSnippet($terms){
		return '';
	}

	/**
	 * @param array $terms terms to highlight
	 * @return string highlighted redirect name (redirect to this page), '' if none or not supported
	 */
	function getRedirectSnippet($terms){
		return '';
	}

	/**
	 * @return Title object for the redirect to this page, null if none or not supported
	 */
	function getRedirectTitle(){
		return null;
	}

	/**
	 * @return string highlighted relevant section name, null if none or not supported
	 */
	function getSectionSnippet(){
		return '';
	}

	/**
	 * @return Title object (pagename+fragment) for the section, null if none or not supported
	 */
	function getSectionTitle(){
		return null;
	}

	/**
	 * @return string timestamp
	 */
	function getTimestamp(){
		return $this->mRevision->getTimestamp();
	}

	/**
	 * @return int number of words
	 */
	function getWordCount(){
		$this->initText();
		return str_word_count( $this->mText );
	}

	/**
	 * @return int size in bytes
	 */
	function getByteSize(){
		$this->initText();
		return strlen( $this->mText );
	}
	
	/**
	 * @return boolean if hit has related articles
	 */
	function hasRelated(){
		return false;
	}
	
	/**
	 * @return interwiki prefix of the title (return iw even if title is broken)
	 */
	function getInterwikiPrefix(){
		return '';
	}
}

/**
 * Highlight bits of wikitext
 * 
 * @ingroup Search
 */
class SearchHighlighter {	
	var $mCleanWikitext = true;
	
	function SearchHighlighter($cleanupWikitext = true){
		$this->mCleanWikitext = $cleanupWikitext;
	}
	
	/**
	 * Default implementation of wikitext highlighting
	 *
	 * @param string $text
	 * @param array $terms Terms to highlight (unescaped)
	 * @param int $contextlines
	 * @param int $contextchars
	 * @return string
	 */
	public function highlightText( $text, $terms, $contextlines, $contextchars ) {
		global $wgLang, $wgContLang;
		global $wgSearchHighlightBoundaries;
		$fname = __METHOD__;
		
		if($text == '')
			return '';
				
		// spli text into text + templates/links/tables
		$spat = "/(\\{\\{)|(\\[\\[[^\\]:]+:)|(\n\\{\\|)";
		// first capture group is for detecting nested templates/links/tables/references
		$endPatterns = array(
			1 => '/(\{\{)|(\}\})/', // template
			2 => '/(\[\[)|(\]\])/', // image
			3 => "/(\n\\{\\|)|(\n\\|\\})/"); // table
			 
		// FIXME: this should prolly be a hook or something
		if(function_exists('wfCite')){
			$spat .= '|(<ref>)'; // references via cite extension
			$endPatterns[4] = '/(<ref>)|(<\/ref>)/';
		}
		$spat .= '/';
		$textExt = array(); // text extracts
		$otherExt = array();  // other extracts
		wfProfileIn( "$fname-split" );
		$start = 0;
		$textLen = strlen($text);
		$count = 0; // sequence number to maintain ordering
		while( $start < $textLen ){
			// find start of template/image/table
			if( preg_match( $spat, $text, $matches, PREG_OFFSET_CAPTURE, $start ) ){
				$epat = '';	
				foreach($matches as $key => $val){
					if($key > 0 && $val[1] != -1){
						if($key == 2){
							// see if this is an image link
							$ns = substr($val[0],2,-1);
							if( $wgContLang->getNsIndex($ns) != NS_IMAGE )
								break;
							
						}
						$epat = $endPatterns[$key];
						$this->splitAndAdd( $textExt, $count, substr( $text, $start, $val[1] - $start ) );						
						$start = $val[1];
						break;
					}
				}
				if( $epat ){
					// find end (and detect any nested elements)
					$level = 0; 
					$offset = $start + 1;
					$found = false;
					while( preg_match( $epat, $text, $endMatches, PREG_OFFSET_CAPTURE, $offset ) ){
						if( array_key_exists(2,$endMatches) ){
							// found end
							if($level == 0){
								$len = strlen($endMatches[2][0]);
								$off = $endMatches[2][1];
								$this->splitAndAdd( $otherExt, $count, 
									substr( $text, $start, $off + $len  - $start ) );
								$start = $off + $len;
								$found = true;
								break;
							} else{
								// end of nested element
								$level -= 1;
							}
						} else{
							// nested
							$level += 1;
						}
						$offset = $endMatches[0][1] + strlen($endMatches[0][0]);
					}
					if( ! $found ){
						// couldn't find appropriate closing tag, skip
						$this->splitAndAdd( $textExt, $count, substr( $text, $start, strlen($matches[0][0]) ) );
						$start += strlen($matches[0][0]);
					}
					continue;
				}
			}
			// else: add as text extract
			$this->splitAndAdd( $textExt, $count, substr($text,$start) );
			break;
		}
		
		$all = $textExt + $otherExt; // these have disjunct key sets
		
		wfProfileOut( "$fname-split" );
		
		// prepare regexps
		foreach( $terms as $index => $term ) {
			$terms[$index] = preg_quote( $term, '/' );			
			// manually do upper/lowercase stuff for utf-8 since PHP won't do it
			if(preg_match('/[\x80-\xff]/', $term) ){
				$terms[$index] = preg_replace_callback('/./us',array($this,'caseCallback'),$terms[$index]);
			}
			
			
		}
		$anyterm = implode( '|', $terms );
		$phrase = implode("$wgSearchHighlightBoundaries+", $terms );

		// FIXME: a hack to scale contextchars, a correct solution
		// would be to have contextchars actually be char and not byte
		// length, and do proper utf-8 substrings and lengths everywhere,
		// but PHP is making that very hard and unclean to implement :(
		$scale = strlen($anyterm) / mb_strlen($anyterm);
		$contextchars = intval( $contextchars * $scale );
		
		$patPre = "(^|$wgSearchHighlightBoundaries)";
		$patPost = "($wgSearchHighlightBoundaries|$)"; 
		
		$pat1 = "/(".$phrase.")/ui";
		$pat2 = "/$patPre(".$anyterm.")$patPost/ui";
		
		wfProfileIn( "$fname-extract" );
		
		$left = $contextlines;

		$snippets = array();
		$offsets = array();		
		
		// show beginning only if it contains all words
		$first = 0;
		$firstText = '';
		foreach($textExt as $index => $line){
			if(strlen($line)>0 && $line[0] != ';' && $line[0] != ':'){
				$firstText = $this->extract( $line, 0, $contextchars * $contextlines );
				$first = $index;
				break;
			}
		}
		if( $firstText ){
			$succ = true;
			// check if first text contains all terms
			foreach($terms as $term){
				if( ! preg_match("/$patPre".$term."$patPost/ui", $firstText) ){
					$succ = false;
					break;
				}
			}
			if( $succ ){
				$snippets[$first] = $firstText;
				$offsets[$first] = 0; 
			}
		}
		if( ! $snippets ) {		
			// match whole query on text 
			$this->process($pat1, $textExt, $left, $contextchars, $snippets, $offsets);
			// match whole query on templates/tables/images
			$this->process($pat1, $otherExt, $left, $contextchars, $snippets, $offsets);
			// match any words on text
			$this->process($pat2, $textExt, $left, $contextchars, $snippets, $offsets);
			// match any words on templates/tables/images
			$this->process($pat2, $otherExt, $left, $contextchars, $snippets, $offsets);
			
			ksort($snippets);
		}
		
		// add extra chars to each snippet to make snippets constant size
		$extended = array();						
		if( count( $snippets ) == 0){
			// couldn't find the target words, just show beginning of article
			$targetchars = $contextchars * $contextlines;
			$snippets[$first] = '';
			$offsets[$first] = 0;
		} else{
			// if begin of the article contains the whole phrase, show only that !!	
			if( array_key_exists($first,$snippets) && preg_match($pat1,$snippets[$first]) 
			    && $offsets[$first] < $contextchars * 2 ){
				$snippets = array ($first => $snippets[$first]);
			}
			
			// calc by how much to extend existing snippets
			$targetchars = intval( ($contextchars * $contextlines) / count ( $snippets ) );
		}  

		foreach($snippets as $index => $line){
			$extended[$index] = $line;
			$len = strlen($line);
			if( $len < $targetchars - 20 ){
				// complete this line
				if($len < strlen( $all[$index] )){
					$extended[$index] = $this->extract( $all[$index], $offsets[$index], $offsets[$index]+$targetchars, $offsets[$index]);
					$len = strlen( $extended[$index] );
				}
				
				// add more lines
				$add = $index + 1;
				while( $len < $targetchars - 20 
				       && array_key_exists($add,$all) 
				       && !array_key_exists($add,$snippets) ){
				    $offsets[$add] = 0;
				    $tt = "\n".$this->extract( $all[$add], 0, $targetchars - $len, $offsets[$add] );
					$extended[$add] = $tt;
					$len += strlen( $tt );
					$add++; 					
				}
			} 
		}
		
		//$snippets = array_map('htmlspecialchars', $extended);
		$snippets = $extended;
		$last = -1;
		$extract = '';
		foreach($snippets as $index => $line){
			if($last == -1) 
				$extract .= $line; // first line
			elseif($last+1 == $index && $offsets[$last]+strlen($snippets[$last]) >= strlen($all[$last]))
				$extract .= " ".$line; // continous lines
			else
				$extract .= '<b> ... </b>' . $line;

			$last = $index;
		}
		if( $extract )
			$extract .= '<b> ... </b>';
		
		$processed = array();
		foreach($terms as $term){
			if( ! isset($processed[$term]) ){
				$pat3 = "/$patPre(".$term.")$patPost/ui"; // highlight word  
				$extract = preg_replace( $pat3,
			  		"\\1<span class='searchmatch'>\\2</span>\\3", $extract );
				$processed[$term] = true;
			}
		}
		
		wfProfileOut( "$fname-extract" );
		
		return $extract;
	}
	
	/**
	 * Split text into lines and add it to extracts array
	 *
	 * @param array $extracts index -> $line
	 * @param int $count
	 * @param string $text
	 */
	function splitAndAdd(&$extracts, &$count, $text){
		$split = explode( "\n", $this->mCleanWikitext? $this->removeWiki($text) : $text );
		foreach($split as $line){
			$tt = trim($line);
			if( $tt )
				$extracts[$count++] = $tt;
		}
	}
	
	/**
	 * Do manual case conversion for non-ascii chars
	 *
	 * @param unknown_type $matches
	 */
	function caseCallback($matches){
		global $wgContLang;
		if( strlen($matches[0]) > 1 ){
			return '['.$wgContLang->lc($matches[0]).$wgContLang->uc($matches[0]).']';
		} else
			return $matches[0];
	}
	
	/**
	 * Extract part of the text from start to end, but by
	 * not chopping up words
	 * @param string $text
	 * @param int $start
	 * @param int $end
	 * @param int $posStart (out) actual start position
	 * @param int $posEnd (out) actual end position
	 * @return string  
	 */
	function extract($text, $start, $end, &$posStart = null, &$posEnd = null ){
		global $wgContLang;		
		
		if( $start != 0)
			$start = $this->position( $text, $start, 1 );
		if( $end >= strlen($text) )
			$end = strlen($text);
		else
			$end = $this->position( $text, $end );
			
		if(!is_null($posStart))
			$posStart = $start;
		if(!is_null($posEnd))
			$posEnd = $end;
		
		if($end > $start)
			return substr($text, $start, $end-$start);
		else
			return '';
	} 
	
	/**
	 * Find a nonletter near a point (index) in the text
	 *
	 * @param string $text
	 * @param int $point
	 * @param int $offset to found index
	 * @return int nearest nonletter index, or beginning of utf8 char if none
	 */
	function position($text, $point, $offset=0 ){
		$tolerance = 10;
		$s = max( 0, $point - $tolerance );
		$l = min( strlen($text), $point + $tolerance ) - $s;
		$m = array();
		if( preg_match('/[ ,.!?~!@#$%^&*\(\)+=\-\\\|\[\]"\'<>]/', substr($text,$s,$l), $m, PREG_OFFSET_CAPTURE ) ){
			return $m[0][1] + $s + $offset;
		} else{
			// check if point is on a valid first UTF8 char
			$char = ord( $text[$point] );
			while( $char >= 0x80 && $char < 0xc0 ) {
				// skip trailing bytes
				$point++;
				if($point >= strlen($text))
					return strlen($text);
				$char = ord( $text[$point] );
			}
			return $point;
			
		}
	}
	
	/**
	 * Search extracts for a pattern, and return snippets
	 *
	 * @param string $pattern regexp for matching lines
	 * @param array $extracts extracts to search   
	 * @param int $linesleft number of extracts to make
	 * @param int $contextchars length of snippet
	 * @param array $out map for highlighted snippets
	 * @param array $offsets map of starting points of snippets
	 * @protected
	 */
	function process( $pattern, $extracts, &$linesleft, &$contextchars, &$out, &$offsets ){
		if($linesleft == 0)
			return; // nothing to do
		foreach($extracts as $index => $line){			
			if( array_key_exists($index,$out) )
				continue; // this line already highlighted
				
			$m = array();
			if ( !preg_match( $pattern, $line, $m, PREG_OFFSET_CAPTURE ) )
				continue;
				
			$offset = $m[0][1];
			$len = strlen($m[0][0]);
			if($offset + $len < $contextchars)
				$begin = 0; 
			elseif( $len > $contextchars)
				$begin = $offset;
			else
				$begin = $offset + intval( ($len - $contextchars) / 2 );
			
			$end = $begin + $contextchars;
			
			$posBegin = $begin;
			// basic snippet from this line
			$out[$index] = $this->extract($line,$begin,$end,$posBegin);
			$offsets[$index] = $posBegin;
			$linesleft--;			
			if($linesleft == 0)
				return;
		}
	}
	
	/** 
	 * Basic wikitext removal
	 * @protected
	 */
	function removeWiki($text) {
		$fname = __METHOD__;
		wfProfileIn( $fname );
		
		//$text = preg_replace("/'{2,5}/", "", $text);
		//$text = preg_replace("/\[[a-z]+:\/\/[^ ]+ ([^]]+)\]/", "\\2", $text);
		//$text = preg_replace("/\[\[([^]|]+)\]\]/", "\\1", $text);
		//$text = preg_replace("/\[\[([^]]+\|)?([^|]]+)\]\]/", "\\2", $text);
		//$text = preg_replace("/\\{\\|(.*?)\\|\\}/", "", $text);
		//$text = preg_replace("/\\[\\[[A-Za-z_-]+:([^|]+?)\\]\\]/", "", $text);
		$text = preg_replace("/\\{\\{([^|]+?)\\}\\}/", "", $text);
		$text = preg_replace("/\\{\\{([^|]+\\|)(.*?)\\}\\}/", "\\2", $text);
		$text = preg_replace("/\\[\\[([^|]+?)\\]\\]/", "\\1", $text);		
		$text = preg_replace_callback("/\\[\\[([^|]+\\|)(.*?)\\]\\]/", array($this,'linkReplace'), $text);
		//$text = preg_replace("/\\[\\[([^|]+\\|)(.*?)\\]\\]/", "\\2", $text);
		$text = preg_replace("/<\/?[^>]+>/", "", $text);
		$text = preg_replace("/'''''/", "", $text);
		$text = preg_replace("/('''|<\/?[iIuUbB]>)/", "", $text);
		$text = preg_replace("/''/", "", $text);
		
		wfProfileOut( $fname );
		return $text;
	}
	
	/**
	 * callback to replace [[target|caption]] kind of links, if
	 * the target is category or image, leave it
	 *
	 * @param array $matches
	 */
	function linkReplace($matches){
		$colon = strpos( $matches[1], ':' ); 
		if( $colon === false )
			return $matches[2]; // replace with caption
		global $wgContLang;
		$ns = substr( $matches[1], 0, $colon );
		$index = $wgContLang->getNsIndex($ns);
		if( $index !== false && ($index == NS_IMAGE || $index == NS_CATEGORY) )
			return $matches[0]; // return the whole thing 
		else
			return $matches[2];
		
	}

	/**
     * Simple & fast snippet extraction, but gives completely unrelevant
     * snippets
     *
     * @param string $text
     * @param array $terms
     * @param int $contextlines
     * @param int $contextchars
     * @return string
     */
    public function highlightSimple( $text, $terms, $contextlines, $contextchars ) {
        global $wgLang, $wgContLang;
        $fname = __METHOD__;
    
        $lines = explode( "\n", $text );
        
        $terms = implode( '|', $terms );
        $terms = str_replace( '/', "\\/", $terms);
        $max = intval( $contextchars ) + 1;
        $pat1 = "/(.*)($terms)(.{0,$max})/i";

        $lineno = 0;

        $extract = "";
        wfProfileIn( "$fname-extract" );
        foreach ( $lines as $line ) {
            if ( 0 == $contextlines ) {
                break;
            }
            ++$lineno;
            $m = array();
            if ( ! preg_match( $pat1, $line, $m ) ) {
                continue;
            }
            --$contextlines;
            $pre = $wgContLang->truncate( $m[1], -$contextchars, ' ... ' );

            if ( count( $m ) < 3 ) {
                $post = '';
            } else {
                $post = $wgContLang->truncate( $m[3], $contextchars, ' ... ' );
            }

            $found = $m[2];

            $line = htmlspecialchars( $pre . $found . $post );
            $pat2 = '/(' . $terms . ")/i";
            $line = preg_replace( $pat2,
              "<span class='searchmatch'>\\1</span>", $line );

            $extract .= "${line}\n";
        }
        wfProfileOut( "$fname-extract" );
        
        return $extract;
    }
	
}

/**
 * Dummy class to be used when non-supported Database engine is present.
 * @fixme Dummy class should probably try something at least mildly useful,
 * such as a LIKE search through titles.
 * @ingroup Search
 */
class SearchEngineDummy extends SearchEngine {
	// no-op
}
