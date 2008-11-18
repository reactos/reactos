<?php
/**
 * Special handling for category description pages
 * Modelled after ImagePage.php
 *
 */

if( !defined( 'MEDIAWIKI' ) )
	die( 1 );

/**
 */
class CategoryPage extends Article {
	function view() {
		global $wgRequest, $wgUser;

		$diff = $wgRequest->getVal( 'diff' );
		$diffOnly = $wgRequest->getBool( 'diffonly', $wgUser->getOption( 'diffonly' ) );

		if ( isset( $diff ) && $diffOnly )
			return Article::view();

		if(!wfRunHooks('CategoryPageView', array(&$this))) return;

		if ( NS_CATEGORY == $this->mTitle->getNamespace() ) {
			$this->openShowCategory();
		}

		Article::view();

		# If the article we've just shown is in the "Image" namespace,
		# follow it with the history list and link list for the image
		# it describes.

		if ( NS_CATEGORY == $this->mTitle->getNamespace() ) {
			$this->closeShowCategory();
		}
	}

	/**
	 * This page should not be cached if 'from' or 'until' has been used
	 * @return bool
	 */
	function isFileCacheable() {
		global $wgRequest;

		return ( ! Article::isFileCacheable()
				|| $wgRequest->getVal( 'from' )
				|| $wgRequest->getVal( 'until' )
		) ? false : true;
	}

	function openShowCategory() {
		# For overloading
	}

	function closeShowCategory() {
		global $wgOut, $wgRequest;
		$from = $wgRequest->getVal( 'from' );
		$until = $wgRequest->getVal( 'until' );

		$viewer = new CategoryViewer( $this->mTitle, $from, $until );
		$wgOut->addHTML( $viewer->getHTML() );
	}
}

class CategoryViewer {
	var $title, $limit, $from, $until,
		$articles, $articles_start_char,
		$children, $children_start_char,
		$showGallery, $gallery,
		$skin;
	/** Category object for this page */
	private $cat;

	function __construct( $title, $from = '', $until = '' ) {
		global $wgCategoryPagingLimit;
		$this->title = $title;
		$this->from = $from;
		$this->until = $until;
		$this->limit = $wgCategoryPagingLimit;
		$this->cat = Category::newFromName( $title->getDBKey() );
	}

	/**
	 * Format the category data list.
	 *
	 * @param string $from -- return only sort keys from this item on
	 * @param string $until -- don't return keys after this point.
	 * @return string HTML output
	 * @private
	 */
	function getHTML() {
		global $wgOut, $wgCategoryMagicGallery, $wgCategoryPagingLimit;
		wfProfileIn( __METHOD__ );

		$this->showGallery = $wgCategoryMagicGallery && !$wgOut->mNoGallery;

		$this->clearCategoryState();
		$this->doCategoryQuery();
		$this->finaliseCategoryState();

		$r = $this->getCategoryTop() .
			$this->getSubcategorySection() .
			$this->getPagesSection() .
			$this->getImageSection() .
			$this->getCategoryBottom();

		// Give a proper message if category is empty
		if ( $r == '' ) {
			$r = wfMsgExt( 'category-empty', array( 'parse' ) );
		}

		wfProfileOut( __METHOD__ );
		return $r;
	}

	function clearCategoryState() {
		$this->articles = array();
		$this->articles_start_char = array();
		$this->children = array();
		$this->children_start_char = array();
		if( $this->showGallery ) {
			$this->gallery = new ImageGallery();
			$this->gallery->setHideBadImages();
		}
	}

	function getSkin() {
		if ( !$this->skin ) {
			global $wgUser;
			$this->skin = $wgUser->getSkin();
		}
		return $this->skin;
	}

	/**
	 * Add a subcategory to the internal lists, using a Category object
	 */
	function addSubcategoryObject( $cat, $sortkey, $pageLength ) {
		$title = $cat->getTitle();
		$this->addSubcategory( $title, $sortkey, $pageLength );
	}

	/**
	 * Add a subcategory to the internal lists, using a title object 
	 * @deprectated kept for compatibility, please use addSubcategoryObject instead
	 */
	function addSubcategory( $title, $sortkey, $pageLength ) {
		global $wgContLang;
		// Subcategory; strip the 'Category' namespace from the link text.
		$this->children[] = $this->getSkin()->makeKnownLinkObj(
			$title, $wgContLang->convertHtml( $title->getText() ) );

		$this->children_start_char[] = $this->getSubcategorySortChar( $title, $sortkey );
	}

	/**
	* Get the character to be used for sorting subcategories.
	* If there's a link from Category:A to Category:B, the sortkey of the resulting
	* entry in the categorylinks table is Category:A, not A, which it SHOULD be.
	* Workaround: If sortkey == "Category:".$title, than use $title for sorting,
	* else use sortkey...
	*/
	function getSubcategorySortChar( $title, $sortkey ) {
		global $wgContLang;

		if( $title->getPrefixedText() == $sortkey ) {
			$firstChar = $wgContLang->firstChar( $title->getDBkey() );
		} else {
			$firstChar = $wgContLang->firstChar( $sortkey );
		}

		return $wgContLang->convert( $firstChar );
	}

	/**
	 * Add a page in the image namespace
	 */
	function addImage( Title $title, $sortkey, $pageLength, $isRedirect = false ) {
		if ( $this->showGallery ) {
			if( $this->flip ) {
				$this->gallery->insert( $title );
			} else {
				$this->gallery->add( $title );
			}
		} else {
			$this->addPage( $title, $sortkey, $pageLength, $isRedirect );
		}
	}

	/**
	 * Add a miscellaneous page
	 */
	function addPage( $title, $sortkey, $pageLength, $isRedirect = false ) {
		global $wgContLang;
		$this->articles[] = $isRedirect
			? '<span class="redirect-in-category">' . $this->getSkin()->makeKnownLinkObj( $title ) . '</span>'
			: $this->getSkin()->makeSizeLinkObj( $pageLength, $title );
		$this->articles_start_char[] = $wgContLang->convert( $wgContLang->firstChar( $sortkey ) );
	}

	function finaliseCategoryState() {
		if( $this->flip ) {
			$this->children            = array_reverse( $this->children );
			$this->children_start_char = array_reverse( $this->children_start_char );
			$this->articles            = array_reverse( $this->articles );
			$this->articles_start_char = array_reverse( $this->articles_start_char );
		}
	}

	function doCategoryQuery() {
		$dbr = wfGetDB( DB_SLAVE );
		if( $this->from != '' ) {
			$pageCondition = 'cl_sortkey >= ' . $dbr->addQuotes( $this->from );
			$this->flip = false;
		} elseif( $this->until != '' ) {
			$pageCondition = 'cl_sortkey < ' . $dbr->addQuotes( $this->until );
			$this->flip = true;
		} else {
			$pageCondition = '1 = 1';
			$this->flip = false;
		}
		$res = $dbr->select(
			array( 'page', 'categorylinks', 'category' ),
			array( 'page_title', 'page_namespace', 'page_len', 'page_is_redirect', 'cl_sortkey',
				'cat_id', 'cat_title', 'cat_subcats', 'cat_pages', 'cat_files' ),
			array( $pageCondition,
			       'cl_to' => $this->title->getDBkey() ),
			__METHOD__,
			array( 'ORDER BY' => $this->flip ? 'cl_sortkey DESC' : 'cl_sortkey',
			       'USE INDEX' => array( 'categorylinks' => 'cl_sortkey' ),
			       'LIMIT'    => $this->limit + 1 ),
			array( 'categorylinks'  => array( 'INNER JOIN', 'cl_from = page_id' ),
		               'category' => array( 'LEFT JOIN', 'cat_title = page_title AND page_namespace = ' . NS_CATEGORY ) ) );

		$count = 0;
		$this->nextPage = null;
		while( $x = $dbr->fetchObject ( $res ) ) {
			if( ++$count > $this->limit ) {
				// We've reached the one extra which shows that there are
				// additional pages to be had. Stop here...
				$this->nextPage = $x->cl_sortkey;
				break;
			}

			$title = Title::makeTitle( $x->page_namespace, $x->page_title );

			if( $title->getNamespace() == NS_CATEGORY ) {
				$cat = Category::newFromRow( $x, $title );
				$this->addSubcategoryObject( $cat, $x->cl_sortkey, $x->page_len );
			} elseif( $this->showGallery && $title->getNamespace() == NS_IMAGE ) {
				$this->addImage( $title, $x->cl_sortkey, $x->page_len, $x->page_is_redirect );
			} else {
				$this->addPage( $title, $x->cl_sortkey, $x->page_len, $x->page_is_redirect );
			}
		}
		$dbr->freeResult( $res );
	}

	function getCategoryTop() {
		$r = '';
		if( $this->until != '' ) {
			$r .= $this->pagingLinks( $this->title, $this->nextPage, $this->until, $this->limit );
		} elseif( $this->nextPage != '' || $this->from != '' ) {
			$r .= $this->pagingLinks( $this->title, $this->from, $this->nextPage, $this->limit );
		}
		return $r == ''
			? $r
			: "<br style=\"clear:both;\"/>\n" . $r;
	}

	function getSubcategorySection() {
		# Don't show subcategories section if there are none.
		$r = '';
		$rescnt = count( $this->children );
		$dbcnt = $this->cat->getSubcatCount();
		$countmsg = $this->getCountMessage( $rescnt, $dbcnt, 'subcat' );
		if( $rescnt > 0 ) {
			# Showing subcategories
			$r .= "<div id=\"mw-subcategories\">\n";
			$r .= '<h2>' . wfMsg( 'subcategories' ) . "</h2>\n";
			$r .= $countmsg;
			$r .= $this->formatList( $this->children, $this->children_start_char );
			$r .= "\n</div>";
		}
		return $r;
	}

	function getPagesSection() {
		$ti = htmlspecialchars( $this->title->getText() );
		# Don't show articles section if there are none.
		$r = '';

		# FIXME, here and in the other two sections: we don't need to bother
		# with this rigamarole if the entire category contents fit on one page
		# and have already been retrieved.  We can just use $rescnt in that
		# case and save a query and some logic.
		$dbcnt = $this->cat->getPageCount() - $this->cat->getSubcatCount()
			- $this->cat->getFileCount();
		$rescnt = count( $this->articles );
		$countmsg = $this->getCountMessage( $rescnt, $dbcnt, 'article' );

		if( $rescnt > 0 ) {
			$r = "<div id=\"mw-pages\">\n";
			$r .= '<h2>' . wfMsg( 'category_header', $ti ) . "</h2>\n";
			$r .= $countmsg;
			$r .= $this->formatList( $this->articles, $this->articles_start_char );
			$r .= "\n</div>";
		}
		return $r;
	}

	function getImageSection() {
		if( $this->showGallery && ! $this->gallery->isEmpty() ) {
			$dbcnt = $this->cat->getFileCount();
			$rescnt = $this->gallery->count();
			$countmsg = $this->getCountMessage( $rescnt, $dbcnt, 'file' );

			return "<div id=\"mw-category-media\">\n" .
			'<h2>' . wfMsg( 'category-media-header', htmlspecialchars($this->title->getText()) ) . "</h2>\n" .
			$countmsg . $this->gallery->toHTML() . "\n</div>";
		} else {
			return '';
		}
	}

	function getCategoryBottom() {
		if( $this->until != '' ) {
			return $this->pagingLinks( $this->title, $this->nextPage, $this->until, $this->limit );
		} elseif( $this->nextPage != '' || $this->from != '' ) {
			return $this->pagingLinks( $this->title, $this->from, $this->nextPage, $this->limit );
		} else {
			return '';
		}
	}

	/**
	 * Format a list of articles chunked by letter, either as a
	 * bullet list or a columnar format, depending on the length.
	 *
	 * @param array $articles
	 * @param array $articles_start_char
	 * @param int   $cutoff
	 * @return string
	 * @private
	 */
	function formatList( $articles, $articles_start_char, $cutoff = 6 ) {
		if ( count ( $articles ) > $cutoff ) {
			return $this->columnList( $articles, $articles_start_char );
		} elseif ( count($articles) > 0) {
			// for short lists of articles in categories.
			return $this->shortList( $articles, $articles_start_char );
		}
		return '';
	}

	/**
	 * Format a list of articles chunked by letter in a three-column
	 * list, ordered vertically.
	 *
	 * @param array $articles
	 * @param array $articles_start_char
	 * @return string
	 * @private
	 */
	function columnList( $articles, $articles_start_char ) {
		// divide list into three equal chunks
		$chunk = (int) (count ( $articles ) / 3);

		// get and display header
		$r = '<table width="100%"><tr valign="top">';

		$prev_start_char = 'none';

		// loop through the chunks
		for($startChunk = 0, $endChunk = $chunk, $chunkIndex = 0;
			$chunkIndex < 3;
			$chunkIndex++, $startChunk = $endChunk, $endChunk += $chunk + 1)
		{
			$r .= "<td>\n";
			$atColumnTop = true;

			// output all articles in category
			for ($index = $startChunk ;
				$index < $endChunk && $index < count($articles);
				$index++ )
			{
				// check for change of starting letter or begining of chunk
				if ( ($index == $startChunk) ||
					 ($articles_start_char[$index] != $articles_start_char[$index - 1]) )

				{
					if( $atColumnTop ) {
						$atColumnTop = false;
					} else {
						$r .= "</ul>\n";
					}
					$cont_msg = "";
					if ( $articles_start_char[$index] == $prev_start_char )
						$cont_msg = ' ' . wfMsgHtml( 'listingcontinuesabbrev' );
					$r .= "<h3>" . htmlspecialchars( $articles_start_char[$index] ) . "$cont_msg</h3>\n<ul>";
					$prev_start_char = $articles_start_char[$index];
				}

				$r .= "<li>{$articles[$index]}</li>";
			}
			if( !$atColumnTop ) {
				$r .= "</ul>\n";
			}
			$r .= "</td>\n";


		}
		$r .= '</tr></table>';
		return $r;
	}

	/**
	 * Format a list of articles chunked by letter in a bullet list.
	 * @param array $articles
	 * @param array $articles_start_char
	 * @return string
	 * @private
	 */
	function shortList( $articles, $articles_start_char ) {
		$r = '<h3>' . htmlspecialchars( $articles_start_char[0] ) . "</h3>\n";
		$r .= '<ul><li>'.$articles[0].'</li>';
		for ($index = 1; $index < count($articles); $index++ )
		{
			if ($articles_start_char[$index] != $articles_start_char[$index - 1])
			{
				$r .= "</ul><h3>" . htmlspecialchars( $articles_start_char[$index] ) . "</h3>\n<ul>";
			}

			$r .= "<li>{$articles[$index]}</li>";
		}
		$r .= '</ul>';
		return $r;
	}

	/**
	 * @param Title  $title
	 * @param string $first
	 * @param string $last
	 * @param int    $limit
	 * @param array  $query - additional query options to pass
	 * @return string
	 * @private
	 */
	function pagingLinks( $title, $first, $last, $limit, $query = array() ) {
		global $wgLang;
		$sk = $this->getSkin();
		$limitText = $wgLang->formatNum( $limit );

		$prevLink = htmlspecialchars( wfMsg( 'prevn', $limitText ) );
		if( $first != '' ) {
			$prevLink = $sk->makeLinkObj( $title, $prevLink,
				wfArrayToCGI( $query + array( 'until' => $first ) ) );
		}
		$nextLink = htmlspecialchars( wfMsg( 'nextn', $limitText ) );
		if( $last != '' ) {
			$nextLink = $sk->makeLinkObj( $title, $nextLink,
				wfArrayToCGI( $query + array( 'from' => $last ) ) );
		}

		return "($prevLink) ($nextLink)";
	}

	/**
	 * What to do if the category table conflicts with the number of results
	 * returned?  This function says what.  It works the same whether the
	 * things being counted are articles, subcategories, or files.
	 *
	 * Note for grepping: uses the messages category-article-count,
	 * category-article-count-limited, category-subcat-count,
	 * category-subcat-count-limited, category-file-count,
	 * category-file-count-limited.
	 *
	 * @param int $rescnt The number of items returned by our database query.
	 * @param int $dbcnt The number of items according to the category table.
	 * @param string $type 'subcat', 'article', or 'file'
	 * @return string A message giving the number of items, to output to HTML.
	 */
	private function getCountMessage( $rescnt, $dbcnt, $type ) {
		global $wgLang;
		# There are three cases:
		#   1) The category table figure seems sane.  It might be wrong, but
		#      we can't do anything about it if we don't recalculate it on ev-
		#      ery category view.
		#   2) The category table figure isn't sane, like it's smaller than the
		#      number of actual results, *but* the number of results is less
		#      than $this->limit and there's no offset.  In this case we still
		#      know the right figure.
		#   3) We have no idea.
		$totalrescnt = count( $this->articles ) + count( $this->children ) +
			($this->showGallery ? $this->gallery->count() : 0);
		if($dbcnt == $rescnt || (($totalrescnt == $this->limit || $this->from
		|| $this->until) && $dbcnt > $rescnt)){
			# Case 1: seems sane.
			$totalcnt = $dbcnt;
		} elseif($totalrescnt < $this->limit && !$this->from && !$this->until){
			# Case 2: not sane, but salvageable.
			$totalcnt = $rescnt;
		} else {
			# Case 3: hopeless.  Don't give a total count at all.
			return wfMsgExt("category-$type-count-limited", 'parse',
				$wgLang->formatNum( $rescnt ) );
		}
		return wfMsgExt( "category-$type-count", 'parse', $wgLang->formatNum( $rescnt ),
			$wgLang->formatNum( $totalcnt ) );
	}
}
