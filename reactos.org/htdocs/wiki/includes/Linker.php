<?php
/**
 * Split off some of the internal bits from Skin.php. These functions are used
 * for primarily page content: links, embedded images, table of contents. Links
 * are also used in the skin. For the moment, Skin is a descendent class of
 * Linker.  In the future, it should probably be further split so that every
 * other bit of the wiki doesn't have to go loading up Skin to get at it.
 *
 * @ingroup Skins
 */
class Linker {

	/**
	 * Flags for userToolLinks()
	 */
	const TOOL_LINKS_NOBLOCK = 1;

	function __construct() {}

	/**
	 * @deprecated
	 */
	function postParseLinkColour( $s = null ) {
		return null;
	}

	/**
	 * Get the appropriate HTML attributes to add to the "a" element of an ex-
	 * ternal link, as created by [wikisyntax].
	 *
	 * @param string $title  The (unescaped) title text for the link
	 * @param string $unused Unused
	 * @param string $class  The contents of the class attribute; if an empty
	 *   string is passed, which is the default value, defaults to 'external'.
	 */
	function getExternalLinkAttributes( $title, $unused = null, $class='' ) {
		return $this->getLinkAttributesInternal( $title, $class, 'external' );
	}

	/**
	 * Get the appropriate HTML attributes to add to the "a" element of an in-
	 * terwiki link.
	 *
	 * @param string $title  The title text for the link, URL-encoded (???) but
	 *   not HTML-escaped
	 * @param string $unused Unused
	 * @param string $class  The contents of the class attribute; if an empty
	 *   string is passed, which is the default value, defaults to 'external'.
	 */
	function getInterwikiLinkAttributes( $title, $unused = null, $class='' ) {
		global $wgContLang;

		# FIXME: We have a whole bunch of handling here that doesn't happen in
		# getExternalLinkAttributes, why?
		$title = urldecode( $title );
		$title = $wgContLang->checkTitleEncoding( $title );
		$title = preg_replace( '/[\\x00-\\x1f]/', ' ', $title );

		return $this->getLinkAttributesInternal( $title, $class, 'external' );
	}

	/**
	 * Get the appropriate HTML attributes to add to the "a" element of an in-
	 * ternal link.
	 *
	 * @param string $title  The title text for the link, URL-encoded (???) but
	 *   not HTML-escaped
	 * @param string $unused Unused
	 * @param string $class  The contents of the class attribute, default none
	 */
	function getInternalLinkAttributes( $title, $unused = null, $class='' ) {
		$title = urldecode( $title );
		$title = str_replace( '_', ' ', $title );
		return $this->getLinkAttributesInternal( $title, $class );
	}

	/**
	 * Get the appropriate HTML attributes to add to the "a" element of an in-
	 * ternal link, given the Title object for the page we want to link to.
	 *
	 * @param Title  $nt     The Title object
	 * @param string $unused Unused
	 * @param string $class  The contents of the class attribute, default none
	 * @param mixed  $title  Optional (unescaped) string to use in the title
	 *   attribute; if false, default to the name of the page we're linking to
	 */
	function getInternalLinkAttributesObj( $nt, $unused = null, $class = '', $title = false ) {
		if( $title === false ) {
			$title = $nt->getPrefixedText();
		}
		return $this->getLinkAttributesInternal( $title, $class );
	}

	/**
	 * Common code for getLinkAttributesX functions
	 */
	private function getLinkAttributesInternal( $title, $class, $classDefault = false ) {
		$title = htmlspecialchars( $title );
		if( $class === '' and $classDefault !== false ) {
			# FIXME: Parameter defaults the hard way!  We should just have
			# $class = 'external' or whatever as the default in the externally-
			# exposed functions, not $class = ''.
			$class = $classDefault;
		}
		$class = htmlspecialchars( $class );
		$r = '';
		if( $class !== '' ) {
			$r .= " class=\"$class\"";
		}
		$r .= " title=\"$title\"";
		return $r;
	}

	/**
	 * Return the CSS colour of a known link
	 *
	 * @param Title $t
	 * @param integer $threshold user defined threshold
	 * @return string CSS class
	 */
	function getLinkColour( $t, $threshold ) {
		$colour = '';
		if ( $t->isRedirect() ) {
			# Page is a redirect
			$colour = 'mw-redirect';
		} elseif ( $threshold > 0 && $t->getLength() < $threshold && MWNamespace::isContent( $t->getNamespace() ) ) {
			# Page is a stub
			$colour = 'stub';
		}
		return $colour;
	}

	/**
	 * This function is a shortcut to makeLinkObj(Title::newFromText($title),...). Do not call
	 * it if you already have a title object handy. See makeLinkObj for further documentation.
	 *
	 * @param $title String: the text of the title
	 * @param $text  String: link text
	 * @param $query String: optional query part
	 * @param $trail String: optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeLink( $title, $text = '', $query = '', $trail = '' ) {
		wfProfileIn( __METHOD__ );
	 	$nt = Title::newFromText( $title );
		if ( $nt instanceof Title ) {
			$result = $this->makeLinkObj( $nt, $text, $query, $trail );
		} else {
			wfDebug( 'Invalid title passed to Linker::makeLink(): "'.$title."\"\n" );
			$result = $text == "" ? $title : $text;
		}

		wfProfileOut( __METHOD__ );
		return $result;
	}

	/**
	 * This function is a shortcut to makeKnownLinkObj(Title::newFromText($title),...). Do not call
	 * it if you already have a title object handy. See makeKnownLinkObj for further documentation.
	 *
	 * @param $title String: the text of the title
	 * @param $text  String: link text
	 * @param $query String: optional query part
	 * @param $trail String: optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeKnownLink( $title, $text = '', $query = '', $trail = '', $prefix = '',$aprops = '') {
		$nt = Title::newFromText( $title );
		if ( $nt instanceof Title ) {
			return $this->makeKnownLinkObj( $nt, $text, $query, $trail, $prefix , $aprops );
		} else {
			wfDebug( 'Invalid title passed to Linker::makeKnownLink(): "'.$title."\"\n" );
			return $text == '' ? $title : $text;
		}
	}

	/**
	 * This function is a shortcut to makeBrokenLinkObj(Title::newFromText($title),...). Do not call
	 * it if you already have a title object handy. See makeBrokenLinkObj for further documentation.
	 *
	 * @param string $title The text of the title
	 * @param string $text Link text
	 * @param string $query Optional query part
	 * @param string $trail Optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeBrokenLink( $title, $text = '', $query = '', $trail = '' ) {
		$nt = Title::newFromText( $title );
		if ( $nt instanceof Title ) {
			return $this->makeBrokenLinkObj( $nt, $text, $query, $trail );
		} else {
			wfDebug( 'Invalid title passed to Linker::makeBrokenLink(): "'.$title."\"\n" );
			return $text == '' ? $title : $text;
		}
	}

	/**
	 * @deprecated use makeColouredLinkObj
	 *
	 * This function is a shortcut to makeStubLinkObj(Title::newFromText($title),...). Do not call
	 * it if you already have a title object handy. See makeStubLinkObj for further documentation.
	 *
	 * @param $title String: the text of the title
	 * @param $text  String: link text
	 * @param $query String: optional query part
	 * @param $trail String: optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeStubLink( $title, $text = '', $query = '', $trail = '' ) {
		$nt = Title::newFromText( $title );
		if ( $nt instanceof Title ) {
			return $this->makeStubLinkObj( $nt, $text, $query, $trail );
		} else {
			wfDebug( 'Invalid title passed to Linker::makeStubLink(): "'.$title."\"\n" );
			return $text == '' ? $title : $text;
		}
	}

	/**
	 * Make a link for a title which may or may not be in the database. If you need to
	 * call this lots of times, pre-fill the link cache with a LinkBatch, otherwise each
	 * call to this will result in a DB query.
	 *
	 * @param $nt     Title: the title object to make the link from, e.g. from
	 *                      Title::newFromText.
	 * @param $text  String: link text
	 * @param $query String: optional query part
	 * @param $trail String: optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 * @param $prefix String: optional prefix. As trail, only before instead of after.
	 */
	function makeLinkObj( $nt, $text= '', $query = '', $trail = '', $prefix = '' ) {
		global $wgUser;
		wfProfileIn( __METHOD__ );

		if ( !$nt instanceof Title ) {
			# Fail gracefully
			wfProfileOut( __METHOD__ );
			return "<!-- ERROR -->{$prefix}{$text}{$trail}";
		}

		if ( $nt->isExternal() ) {
			$u = $nt->getFullURL();
			$link = $nt->getPrefixedURL();
			if ( '' == $text ) { $text = $nt->getPrefixedText(); }
			$style = $this->getInterwikiLinkAttributes( $link, $text, 'extiw' );

			$inside = '';
			if ( '' != $trail ) {
				$m = array();
				if ( preg_match( '/^([a-z]+)(.*)$$/sD', $trail, $m ) ) {
					$inside = $m[1];
					$trail = $m[2];
				}
			}
			$t = "<a href=\"{$u}\"{$style}>{$text}{$inside}</a>";

			wfProfileOut( __METHOD__ );
			return $t;
		} elseif ( $nt->isAlwaysKnown() ) {
			# Image links, special page links and self-links with fragments are always known.
			$retVal = $this->makeKnownLinkObj( $nt, $text, $query, $trail, $prefix );
		} else {
			wfProfileIn( __METHOD__.'-immediate' );

			# Handles links to special pages which do not exist in the database:
			if( $nt->getNamespace() == NS_SPECIAL ) {
				if( SpecialPage::exists( $nt->getDBkey() ) ) {
					$retVal = $this->makeKnownLinkObj( $nt, $text, $query, $trail, $prefix );
				} else {
					$retVal = $this->makeBrokenLinkObj( $nt, $text, $query, $trail, $prefix );
				}
				wfProfileOut( __METHOD__.'-immediate' );
				wfProfileOut( __METHOD__ );
				return $retVal;
			}

			# Work out link colour immediately
			$aid = $nt->getArticleID() ;
			if ( 0 == $aid ) {
				$retVal = $this->makeBrokenLinkObj( $nt, $text, $query, $trail, $prefix );
			} else {
				$colour = '';
				if ( $nt->isContentPage() ) {
					$threshold = $wgUser->getOption('stubthreshold');
					$colour = $this->getLinkColour( $nt, $threshold );
				}
				$retVal = $this->makeColouredLinkObj( $nt, $colour, $text, $query, $trail, $prefix );
			}
			wfProfileOut( __METHOD__.'-immediate' );
		}
		wfProfileOut( __METHOD__ );
		return $retVal;
	}

	/**
	 * Make a link for a title which definitely exists. This is faster than makeLinkObj because
	 * it doesn't have to do a database query. It's also valid for interwiki titles and special
	 * pages.
	 *
	 * @param $nt Title object of target page
	 * @param $text   String: text to replace the title
	 * @param $query  String: link target
	 * @param $trail  String: text after link
	 * @param $prefix String: text before link text
	 * @param $aprops String: extra attributes to the a-element
	 * @param $style  String: style to apply - if empty, use getInternalLinkAttributesObj instead
	 * @return the a-element
	 */
	function makeKnownLinkObj( $title, $text = '', $query = '', $trail = '', $prefix = '' , $aprops = '', $style = '' ) {
		wfProfileIn( __METHOD__ );

		if ( !$title instanceof Title ) {
			# Fail gracefully
			wfProfileOut( __METHOD__ );
			return "<!-- ERROR -->{$prefix}{$text}{$trail}";
		}

		$nt = $this->normaliseSpecialPage( $title );

		$u = $nt->escapeLocalURL( $query );
		if ( $nt->getFragment() != '' ) {
			if( $nt->getPrefixedDbkey() == '' ) {
				$u = '';
				if ( '' == $text ) {
					$text = htmlspecialchars( $nt->getFragment() );
				}
			}
			$u .= $nt->getFragmentForURL();
		}
		if ( $text == '' ) {
			$text = htmlspecialchars( $nt->getPrefixedText() );
		}
		if ( $style == '' ) {
			$style = $this->getInternalLinkAttributesObj( $nt, $text );
		}

		if ( $aprops !== '' ) $aprops = ' ' . $aprops;

		list( $inside, $trail ) = Linker::splitTrail( $trail );
		$r = "<a href=\"{$u}\"{$style}{$aprops}>{$prefix}{$text}{$inside}</a>{$trail}";
		wfProfileOut( __METHOD__ );
		return $r;
	}

	/**
	 * Make a red link to the edit page of a given title.
	 *
	 * @param $nt Title object of the target page
	 * @param $text  String: Link text
	 * @param $query String: Optional query part
	 * @param $trail String: Optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeBrokenLinkObj( $title, $text = '', $query = '', $trail = '', $prefix = '' ) {
		wfProfileIn( __METHOD__ );

		if ( !$title instanceof Title ) {
			# Fail gracefully
			wfProfileOut( __METHOD__ );
			return "<!-- ERROR -->{$prefix}{$text}{$trail}";
		}

		$nt = $this->normaliseSpecialPage( $title );

		if( $nt->getNamespace() == NS_SPECIAL ) {
			$q = $query;
		} else if ( '' == $query ) {
			$q = 'action=edit&redlink=1';
		} else {
			$q = 'action=edit&redlink=1&'.$query;
		}
		$u = $nt->escapeLocalURL( $q );

		$titleText = $nt->getPrefixedText();
		if ( '' == $text ) {
			$text = htmlspecialchars( $titleText );
		}
		$titleAttr = wfMsg( 'red-link-title', $titleText );
		$style = $this->getInternalLinkAttributesObj( $nt, $text, 'new', $titleAttr );
		list( $inside, $trail ) = Linker::splitTrail( $trail );

		wfRunHooks( 'BrokenLink', array( &$this, $nt, $query, &$u, &$style, &$prefix, &$text, &$inside, &$trail ) );
		$s = "<a href=\"{$u}\"{$style}>{$prefix}{$text}{$inside}</a>{$trail}";

		wfProfileOut( __METHOD__ );
		return $s;
	}

	/**
	 * @deprecated use makeColouredLinkObj
	 *
	 * Make a brown link to a short article.
	 *
	 * @param $nt Title object of the target page
	 * @param $text  String: link text
	 * @param $query String: optional query part
	 * @param $trail String: optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeStubLinkObj( $nt, $text = '', $query = '', $trail = '', $prefix = '' ) {
		return $this->makeColouredLinkObj( $nt, 'stub', $text, $query, $trail, $prefix );
	}

	/**
	 * Make a coloured link.
	 *
	 * @param $nt Title object of the target page
	 * @param $colour Integer: colour of the link
	 * @param $text   String:  link text
	 * @param $query  String:  optional query part
	 * @param $trail  String:  optional trail. Alphabetic characters at the start of this string will
	 *                      be included in the link text. Other characters will be appended after
	 *                      the end of the link.
	 */
	function makeColouredLinkObj( $nt, $colour, $text = '', $query = '', $trail = '', $prefix = '' ) {

		if($colour != ''){
			$style = $this->getInternalLinkAttributesObj( $nt, $text, $colour );
		} else $style = '';
		return $this->makeKnownLinkObj( $nt, $text, $query, $trail, $prefix, '', $style );
	}

	/**
	 * Generate either a normal exists-style link or a stub link, depending
	 * on the given page size.
	 *
 	 * @param $size Integer
 	 * @param $nt Title object.
 	 * @param $text String
 	 * @param $query String
 	 * @param $trail String
 	 * @param $prefix String
 	 * @return string HTML of link
	 */
	function makeSizeLinkObj( $size, $nt, $text = '', $query = '', $trail = '', $prefix = '' ) {
		global $wgUser;
		$threshold = intval( $wgUser->getOption( 'stubthreshold' ) );
		$colour = ( $size < $threshold ) ? 'stub' : '';
		return $this->makeColouredLinkObj( $nt, $colour, $text, $query, $trail, $prefix );
	}

	/**
	 * Make appropriate markup for a link to the current article. This is currently rendered
	 * as the bold link text. The calling sequence is the same as the other make*LinkObj functions,
	 * despite $query not being used.
	 */
	function makeSelfLinkObj( $nt, $text = '', $query = '', $trail = '', $prefix = '' ) {
		if ( '' == $text ) {
			$text = htmlspecialchars( $nt->getPrefixedText() );
		}
		list( $inside, $trail ) = Linker::splitTrail( $trail );
		return "<strong class=\"selflink\">{$prefix}{$text}{$inside}</strong>{$trail}";
	}

	function normaliseSpecialPage( Title $title ) {
		if ( $title->getNamespace() == NS_SPECIAL ) {
			list( $name, $subpage ) = SpecialPage::resolveAliasWithSubpage( $title->getDBkey() );
			if ( !$name ) return $title;
			return SpecialPage::getTitleFor( $name, $subpage );
		} else {
			return $title;
		}
	}

	/** @todo document */
	function fnamePart( $url ) {
		$basename = strrchr( $url, '/' );
		if ( false === $basename ) {
			$basename = $url;
		} else {
			$basename = substr( $basename, 1 );
		}
		return $basename;
	}

	/** Obsolete alias */
	function makeImage( $url, $alt = '' ) {
		return $this->makeExternalImage( $url, $alt );
	}

	/** @todo document */
	function makeExternalImage( $url, $alt = '' ) {
		if ( '' == $alt ) {
			$alt = $this->fnamePart( $url );
		}
		$img = '';
		$success = wfRunHooks('LinkerMakeExternalImage', array( &$url, &$alt, &$img ) );
		if(!$success) {
			wfDebug("Hook LinkerMakeExternalImage changed the output of external image with url {$url} and alt text {$alt} to {$img}", true);
			return $img;
		}
		return Xml::element( 'img',
			array(
				'src' => $url,
				'alt' => $alt ) );
	}

	/**
	 * Creates the HTML source for images
	 * @deprecated use makeImageLink2
	 *
	 * @param object $title
	 * @param string $label label text
	 * @param string $alt alt text
	 * @param string $align horizontal alignment: none, left, center, right)
	 * @param array $handlerParams Parameters to be passed to the media handler
	 * @param boolean $framed shows image in original size in a frame
	 * @param boolean $thumb shows image as thumbnail in a frame
	 * @param string $manualthumb image name for the manual thumbnail
	 * @param string $valign vertical alignment: baseline, sub, super, top, text-top, middle, bottom, text-bottom
	 * @param string $time, timestamp of the file, set as false for current
	 * @return string
	 */
	function makeImageLinkObj( $title, $label, $alt, $align = '', $handlerParams = array(), $framed = false,
	  $thumb = false, $manualthumb = '', $valign = '', $time = false )
	{
		$frameParams = array( 'alt' => $alt, 'caption' => $label );
		if ( $align ) {
			$frameParams['align'] = $align;
		}
		if ( $framed ) {
			$frameParams['framed'] = true;
		}
		if ( $thumb ) {
			$frameParams['thumbnail'] = true;
		}
		if ( $manualthumb ) {
			$frameParams['manualthumb'] = $manualthumb;
		}
		if ( $valign ) {
			$frameParams['valign'] = $valign;
		}
		$file = wfFindFile( $title, $time );
		return $this->makeImageLink2( $title, $file, $frameParams, $handlerParams, $time );
	}

	/**
	 * Given parameters derived from [[Image:Foo|options...]], generate the
	 * HTML that that syntax inserts in the page.
	 *
	 * @param Title $title Title object
	 * @param File $file File object, or false if it doesn't exist
	 *
	 * @param array $frameParams Associative array of parameters external to the media handler.
	 *     Boolean parameters are indicated by presence or absence, the value is arbitrary and
	 *     will often be false.
	 *          thumbnail       If present, downscale and frame
	 *          manualthumb     Image name to use as a thumbnail, instead of automatic scaling
	 *          framed          Shows image in original size in a frame
	 *          frameless       Downscale but don't frame
	 *          upright         If present, tweak default sizes for portrait orientation
	 *          upright_factor  Fudge factor for "upright" tweak (default 0.75)
	 *          border          If present, show a border around the image
	 *          align           Horizontal alignment (left, right, center, none)
	 *          valign          Vertical alignment (baseline, sub, super, top, text-top, middle,
	 *                          bottom, text-bottom)
	 *          alt             Alternate text for image (i.e. alt attribute). Plain text.
	 *          caption         HTML for image caption.
	 *
	 * @param array $handlerParams Associative array of media handler parameters, to be passed
	 *       to transform(). Typical keys are "width" and "page".
	 * @param string $time, timestamp of the file, set as false for current
	 * @param string $query, query params for desc url
	 * @return string HTML for an image, with links, wrappers, etc.
	 */
	function makeImageLink2( Title $title, $file, $frameParams = array(), $handlerParams = array(), $time = false, $query = "" ) {
		$res = null;
		if( !wfRunHooks( 'ImageBeforeProduceHTML', array( &$this, &$title,
		&$file, &$frameParams, &$handlerParams, &$time, &$res ) ) ) {
			return $res;
		}

		global $wgContLang, $wgUser, $wgThumbLimits, $wgThumbUpright;
		if ( $file && !$file->allowInlineDisplay() ) {
			wfDebug( __METHOD__.': '.$title->getPrefixedDBkey()." does not allow inline display\n" );
			return $this->makeKnownLinkObj( $title );
		}

		// Shortcuts
		$fp =& $frameParams;
		$hp =& $handlerParams;

		// Clean up parameters
		$page = isset( $hp['page'] ) ? $hp['page'] : false;
		if ( !isset( $fp['align'] ) ) $fp['align'] = '';
		if ( !isset( $fp['alt'] ) ) $fp['alt'] = '';

		$prefix = $postfix = '';

		if ( 'center' == $fp['align'] )
		{
			$prefix  = '<div class="center">';
			$postfix = '</div>';
			$fp['align']   = 'none';
		}
		if ( $file && !isset( $hp['width'] ) ) {
			$hp['width'] = $file->getWidth( $page );

			if( isset( $fp['thumbnail'] ) || isset( $fp['framed'] ) || isset( $fp['frameless'] ) || !$hp['width'] ) {
				$wopt = $wgUser->getOption( 'thumbsize' );

				if( !isset( $wgThumbLimits[$wopt] ) ) {
					 $wopt = User::getDefaultOption( 'thumbsize' );
				}

				// Reduce width for upright images when parameter 'upright' is used
				if ( isset( $fp['upright'] ) && $fp['upright'] == 0 ) {
					$fp['upright'] = $wgThumbUpright;
				}
				// Use width which is smaller: real image width or user preference width
				// For caching health: If width scaled down due to upright parameter, round to full __0 pixel to avoid the creation of a lot of odd thumbs
				$prefWidth = isset( $fp['upright'] ) ?
					round( $wgThumbLimits[$wopt] * $fp['upright'], -1 ) :
					$wgThumbLimits[$wopt];
				if ( $hp['width'] <= 0 || $prefWidth < $hp['width'] ) {
					$hp['width'] = $prefWidth;
				}
			}
		}

		if ( isset( $fp['thumbnail'] ) || isset( $fp['manualthumb'] ) || isset( $fp['framed'] ) ) {

			# Create a thumbnail. Alignment depends on language
			# writing direction, # right aligned for left-to-right-
			# languages ("Western languages"), left-aligned
			# for right-to-left-languages ("Semitic languages")
			#
			# If  thumbnail width has not been provided, it is set
			# to the default user option as specified in Language*.php
			if ( $fp['align'] == '' ) {
				$fp['align'] = $wgContLang->isRTL() ? 'left' : 'right';
			}
			return $prefix.$this->makeThumbLink2( $title, $file, $fp, $hp, $time, $query ).$postfix;
		}

		if ( $file && isset( $fp['frameless'] ) ) {
			$srcWidth = $file->getWidth( $page );
			# For "frameless" option: do not present an image bigger than the source (for bitmap-style images)
			# This is the same behaviour as the "thumb" option does it already.
			if ( $srcWidth && !$file->mustRender() && $hp['width'] > $srcWidth ) {
				$hp['width'] = $srcWidth;
			}
		}

		if ( $file && $hp['width'] ) {
			# Create a resized image, without the additional thumbnail features
			$thumb = $file->transform( $hp );
		} else {
			$thumb = false;
		}

		if ( !$thumb ) {
			$s = $this->makeBrokenImageLinkObj( $title, '', '', '', '', $time==true );
		} else {
			$s = $thumb->toHtml( array(
				'desc-link' => true,
				'desc-query' => $query,
				'alt' => $fp['alt'],
				'valign' => isset( $fp['valign'] ) ? $fp['valign'] : false ,
				'img-class' => isset( $fp['border'] ) ? 'thumbborder' : false ) );
		}
		if ( '' != $fp['align'] ) {
			$s = "<div class=\"float{$fp['align']}\"><span>{$s}</span></div>";
		}
		return str_replace("\n", ' ',$prefix.$s.$postfix);
	}

	/**
	 * Make HTML for a thumbnail including image, border and caption
	 * @param Title $title
	 * @param File $file File object or false if it doesn't exist
	 */
	function makeThumbLinkObj( Title $title, $file, $label = '', $alt, $align = 'right', $params = array(), $framed=false , $manualthumb = "" ) {
		$frameParams = array(
			'alt' => $alt,
			'caption' => $label,
			'align' => $align
		);
		if ( $framed ) $frameParams['framed'] = true;
		if ( $manualthumb ) $frameParams['manualthumb'] = $manualthumb;
		return $this->makeThumbLink2( $title, $file, $frameParams, $params );
	}

	function makeThumbLink2( Title $title, $file, $frameParams = array(), $handlerParams = array(), $time = false, $query = "" ) {
		global $wgStylePath, $wgContLang;
		$exists = $file && $file->exists();

		# Shortcuts
		$fp =& $frameParams;
		$hp =& $handlerParams;

		$page = isset( $hp['page'] ) ? $hp['page'] : false;
		if ( !isset( $fp['align'] ) ) $fp['align'] = 'right';
		if ( !isset( $fp['alt'] ) ) $fp['alt'] = '';
		if ( !isset( $fp['caption'] ) ) $fp['caption'] = '';

		if ( empty( $hp['width'] ) ) {
			// Reduce width for upright images when parameter 'upright' is used
			$hp['width'] = isset( $fp['upright'] ) ? 130 : 180;
		}
		$thumb = false;

		if ( !$exists ) {
			$outerWidth = $hp['width'] + 2;
		} else {
			if ( isset( $fp['manualthumb'] ) ) {
				# Use manually specified thumbnail
				$manual_title = Title::makeTitleSafe( NS_IMAGE, $fp['manualthumb'] );
				if( $manual_title ) {
					$manual_img = wfFindFile( $manual_title );
					if ( $manual_img ) {
						$thumb = $manual_img->getUnscaledThumb();
					} else {
						$exists = false;
					}
				}
			} elseif ( isset( $fp['framed'] ) ) {
				// Use image dimensions, don't scale
				$thumb = $file->getUnscaledThumb( $page );
			} else {
				# Do not present an image bigger than the source, for bitmap-style images
				# This is a hack to maintain compatibility with arbitrary pre-1.10 behaviour
				$srcWidth = $file->getWidth( $page );
				if ( $srcWidth && !$file->mustRender() && $hp['width'] > $srcWidth ) {
					$hp['width'] = $srcWidth;
				}
				$thumb = $file->transform( $hp );
			}

			if ( $thumb ) {
				$outerWidth = $thumb->getWidth() + 2;
			} else {
				$outerWidth = $hp['width'] + 2;
			}
		}

		if( $page ) {
			$query = $query ? '&page=' . urlencode( $page ) : 'page=' . urlencode( $page );
		}
		$url = $title->getLocalURL( $query );

		$more = htmlspecialchars( wfMsg( 'thumbnail-more' ) );

		$s = "<div class=\"thumb t{$fp['align']}\"><div class=\"thumbinner\" style=\"width:{$outerWidth}px;\">";
		if( !$exists ) {
			$s .= $this->makeBrokenImageLinkObj( $title, '', '', '', '', $time==true );
			$zoomicon = '';
		} elseif ( !$thumb ) {
			$s .= htmlspecialchars( wfMsg( 'thumbnail_error', '' ) );
			$zoomicon = '';
		} else {
			$s .= $thumb->toHtml( array(
				'alt' => $fp['alt'],
				'img-class' => 'thumbimage',
				'desc-link' => true,
				'desc-query' => $query ) );
			if ( isset( $fp['framed'] ) ) {
				$zoomicon="";
			} else {
				$zoomicon =  '<div class="magnify">'.
					'<a href="'.$url.'" class="internal" title="'.$more.'">'.
					'<img src="'.$wgStylePath.'/common/images/magnify-clip.png" ' .
					'width="15" height="11" alt="" /></a></div>';
			}
		}
		$s .= '  <div class="thumbcaption">'.$zoomicon.$fp['caption']."</div></div></div>";
		return str_replace("\n", ' ', $s);
	}

	/**
	 * Make a "broken" link to an image
	 *
	 * @param Title $title Image title
	 * @param string $text Link label
	 * @param string $query Query string
	 * @param string $trail Link trail
	 * @param string $prefix Link prefix
	 * @param bool $time, a file of a certain timestamp was requested
	 * @return string
	 */
	public function makeBrokenImageLinkObj( $title, $text = '', $query = '', $trail = '', $prefix = '', $time = false ) {
		global $wgEnableUploads;
		if( $title instanceof Title ) {
			wfProfileIn( __METHOD__ );
			$currentExists = $time ? ( wfFindFile( $title ) != false ) : false;
			if( $wgEnableUploads && !$currentExists ) {
				$upload = SpecialPage::getTitleFor( 'Upload' );
				if( $text == '' )
					$text = htmlspecialchars( $title->getPrefixedText() );
				$redir = RepoGroup::singleton()->getLocalRepo()->checkRedirect( $title );
				if( $redir ) {
					return $this->makeKnownLinkObj( $title, $text, $query, $trail, $prefix );
				}
				$q = 'wpDestFile=' . $title->getPartialUrl();
				if( $query != '' )
					$q .= '&' . $query;
				list( $inside, $trail ) = self::splitTrail( $trail );
				$style = $this->getInternalLinkAttributesObj( $title, $text, 'new' );
				wfProfileOut( __METHOD__ );
				return '<a href="' . $upload->escapeLocalUrl( $q ) . '"'
					. $style . '>' . $prefix . $text . $inside . '</a>' . $trail;
			} else {
				wfProfileOut( __METHOD__ );
				return $this->makeKnownLinkObj( $title, $text, $query, $trail, $prefix );
			}
		} else {
			return "<!-- ERROR -->{$prefix}{$text}{$trail}";
		}
	}

	/** @deprecated use Linker::makeMediaLinkObj() */
	function makeMediaLink( $name, $unused = '', $text = '', $time = false ) {
		$nt = Title::makeTitleSafe( NS_IMAGE, $name );
		return $this->makeMediaLinkObj( $nt, $text, $time );
	}

	/**
	 * Create a direct link to a given uploaded file.
	 *
	 * @param $title Title object.
	 * @param $text String: pre-sanitized HTML
	 * @param $time string: time image was created
	 * @return string HTML
	 *
	 * @public
	 * @todo Handle invalid or missing images better.
	 */
	function makeMediaLinkObj( $title, $text = '', $time = false ) {
		if( is_null( $title ) ) {
			### HOTFIX. Instead of breaking, return empty string.
			return $text;
		} else {
			$img  = wfFindFile( $title, $time );
			if( $img ) {
				$url  = $img->getURL();
				$class = 'internal';
			} else {
				$upload = SpecialPage::getTitleFor( 'Upload' );
				$url = $upload->getLocalUrl( 'wpDestFile=' . urlencode( $title->getDBkey() ) );
				$class = 'new';
			}
			$alt = htmlspecialchars( $title->getText() );
			if( $text == '' ) {
				$text = $alt;
			}
			$u = htmlspecialchars( $url );
			return "<a href=\"{$u}\" class=\"$class\" title=\"{$alt}\">{$text}</a>";
		}
	}

	/** @todo document */
	function specialLink( $name, $key = '' ) {
		global $wgContLang;

		if ( '' == $key ) { $key = strtolower( $name ); }
		$pn = $wgContLang->ucfirst( $name );
		return $this->makeKnownLink( $wgContLang->specialPage( $pn ),
		  wfMsg( $key ) );
	}

	/** @todo document */
	function makeExternalLink( $url, $text, $escape = true, $linktype = '', $ns = null ) {
		$style = $this->getExternalLinkAttributes( $url, $text, 'external ' . $linktype );
		global $wgNoFollowLinks, $wgNoFollowNsExceptions;
		if( $wgNoFollowLinks && !(isset($ns) && in_array($ns, $wgNoFollowNsExceptions)) ) {
			$style .= ' rel="nofollow"';
		}
		$url = htmlspecialchars( $url );
		if( $escape ) {
			$text = htmlspecialchars( $text );
		}
		$link = '';
		$success = wfRunHooks('LinkerMakeExternalLink', array( &$url, &$text, &$link ) );
		if(!$success) {
			wfDebug("Hook LinkerMakeExternalLink changed the output of link with url {$url} and text {$text} to {$link}", true);
			return $link;
		}
		return '<a href="'.$url.'"'.$style.'>'.$text.'</a>';
	}

	/**
	 * Make user link (or user contributions for unregistered users)
	 * @param $userId   Integer: user id in database.
	 * @param $userText String: user name in database
	 * @return string HTML fragment
	 * @private
	 */
	function userLink( $userId, $userText ) {
		$encName = htmlspecialchars( $userText );
		if( $userId == 0 ) {
			$contribsPage = SpecialPage::getTitleFor( 'Contributions', $userText );
			return $this->makeKnownLinkObj( $contribsPage,
				$encName);
		} else {
			$userPage = Title::makeTitle( NS_USER, $userText );
			return $this->makeLinkObj( $userPage, $encName );
		}
	}

	/**
	 * Generate standard user tool links (talk, contributions, block link, etc.)
	 *
	 * @param int $userId User identifier
	 * @param string $userText User name or IP address
	 * @param bool $redContribsWhenNoEdits Should the contributions link be red if the user has no edits?
	 * @param int $flags Customisation flags (e.g. self::TOOL_LINKS_NOBLOCK)
	 * @param int $edits, user edit count (optional, for performance)
	 * @return string
	 */
	public function userToolLinks( $userId, $userText, $redContribsWhenNoEdits = false, $flags = 0, $edits=null ) {
		global $wgUser, $wgDisableAnonTalk, $wgSysopUserBans;
		$talkable = !( $wgDisableAnonTalk && 0 == $userId );
		$blockable = ( $wgSysopUserBans || 0 == $userId ) && !$flags & self::TOOL_LINKS_NOBLOCK;

		$items = array();
		if( $talkable ) {
			$items[] = $this->userTalkLink( $userId, $userText );
		}
		if( $userId ) {
			// check if the user has an edit
			if( $redContribsWhenNoEdits ) {
				$count = !is_null($edits) ? $edits : User::edits( $userId );
				$style = ($count == 0) ? " class='new'" : '';
			} else {
				$style = '';
			}
			$contribsPage = SpecialPage::getTitleFor( 'Contributions', $userText );

			$items[] = $this->makeKnownLinkObj( $contribsPage, wfMsgHtml( 'contribslink' ), '', '', '', '', $style );
		}
		if( $blockable && $wgUser->isAllowed( 'block' ) ) {
			$items[] = $this->blockLink( $userId, $userText );
		}

		if( $items ) {
			return ' (' . implode( ' | ', $items ) . ')';
		} else {
			return '';
		}
	}

	/**
	 * Alias for userToolLinks( $userId, $userText, true );
	 * @param int $userId User identifier
	 * @param string $userText User name or IP address
	 * @param int $edits, user edit count (optional, for performance)
	 */
	public function userToolLinksRedContribs( $userId, $userText, $edits=null ) {
		return $this->userToolLinks( $userId, $userText, true, 0, $edits );
	}


	/**
	 * @param $userId Integer: user id in database.
	 * @param $userText String: user name in database.
	 * @return string HTML fragment with user talk link
	 * @private
	 */
	function userTalkLink( $userId, $userText ) {
		$userTalkPage = Title::makeTitle( NS_USER_TALK, $userText );
		$userTalkLink = $this->makeLinkObj( $userTalkPage, wfMsgHtml( 'talkpagelinktext' ) );
		return $userTalkLink;
	}

	/**
	 * @param $userId Integer: userid
	 * @param $userText String: user name in database.
	 * @return string HTML fragment with block link
	 * @private
	 */
	function blockLink( $userId, $userText ) {
		$blockPage = SpecialPage::getTitleFor( 'Blockip', $userText );
		$blockLink = $this->makeKnownLinkObj( $blockPage,
			wfMsgHtml( 'blocklink' ) );
		return $blockLink;
	}

	/**
	 * Generate a user link if the current user is allowed to view it
	 * @param $rev Revision object.
	 * @param $isPublic, bool, show only if all users can see it
	 * @return string HTML
	 */
	function revUserLink( $rev, $isPublic = false ) {
		if( $rev->isDeleted( Revision::DELETED_USER ) && $isPublic ) {
			$link = wfMsgHtml( 'rev-deleted-user' );
		} else if( $rev->userCan( Revision::DELETED_USER ) ) {
			$link = $this->userLink( $rev->getRawUser(), $rev->getRawUserText() );
		} else {
			$link = wfMsgHtml( 'rev-deleted-user' );
		}
		if( $rev->isDeleted( Revision::DELETED_USER ) ) {
			return '<span class="history-deleted">' . $link . '</span>';
		}
		return $link;
	}

	/**
	 * Generate a user tool link cluster if the current user is allowed to view it
	 * @param $rev Revision object.
	 * @param $isPublic, bool, show only if all users can see it
	 * @return string HTML
	 */
	function revUserTools( $rev, $isPublic = false ) {
		if( $rev->isDeleted( Revision::DELETED_USER ) && $isPublic ) {
			$link = wfMsgHtml( 'rev-deleted-user' );
		} else if( $rev->userCan( Revision::DELETED_USER ) ) {
			$link = $this->userLink( $rev->getRawUser(), $rev->getRawUserText() ) .
			' ' . $this->userToolLinks( $rev->getRawUser(), $rev->getRawUserText() );
		} else {
			$link = wfMsgHtml( 'rev-deleted-user' );
		}
		if( $rev->isDeleted( Revision::DELETED_USER ) ) {
			return ' <span class="history-deleted">' . $link . '</span>';
		}
		return $link;
	}

	/**
	 * This function is called by all recent changes variants, by the page history,
	 * and by the user contributions list. It is responsible for formatting edit
	 * comments. It escapes any HTML in the comment, but adds some CSS to format
	 * auto-generated comments (from section editing) and formats [[wikilinks]].
	 *
	 * @author Erik Moeller <moeller@scireview.de>
	 *
	 * Note: there's not always a title to pass to this function.
	 * Since you can't set a default parameter for a reference, I've turned it
	 * temporarily to a value pass. Should be adjusted further. --brion
	 *
	 * @param string $comment
	 * @param mixed $title Title object (to generate link to the section in autocomment) or null
	 * @param bool $local Whether section links should refer to local page
	 */
	function formatComment($comment, $title = NULL, $local = false) {
		wfProfileIn( __METHOD__ );

		# Sanitize text a bit:
		$comment = str_replace( "\n", " ", $comment );
		$comment = htmlspecialchars( $comment );

		# Render autocomments and make links:
		$comment = $this->formatAutoComments( $comment, $title, $local );
		$comment = $this->formatLinksInComment( $comment );

		wfProfileOut( __METHOD__ );
		return $comment;
	}

	/**
	 * The pattern for autogen comments is / * foo * /, which makes for
	 * some nasty regex.
	 * We look for all comments, match any text before and after the comment,
	 * add a separator where needed and format the comment itself with CSS
	 * Called by Linker::formatComment.
	 *
	 * @param string $comment Comment text
	 * @param object $title An optional title object used to links to sections
	 * @return string $comment formatted comment
	 *
	 * @todo Document the $local parameter.
	 */
	private function formatAutocomments( $comment, $title = NULL, $local = false ) {
		$match = array();
		while (preg_match('!(.*)/\*\s*(.*?)\s*\*/(.*)!', $comment,$match)) {
			$pre=$match[1];
			$auto=$match[2];
			$post=$match[3];
			$link='';
			if( $title ) {
				$section = $auto;

				# Generate a valid anchor name from the section title.
				# Hackish, but should generally work - we strip wiki
				# syntax, including the magic [[: that is used to
				# "link rather than show" in case of images and
				# interlanguage links.
				$section = str_replace( '[[:', '', $section );
				$section = str_replace( '[[', '', $section );
				$section = str_replace( ']]', '', $section );
				if ( $local ) {
					$sectionTitle = Title::newFromText( '#' . $section);
				} else {
					$sectionTitle = wfClone( $title );
					$sectionTitle->mFragment = $section;
				}
				$link = $this->makeKnownLinkObj( $sectionTitle, wfMsgForContent( 'sectionlink' ) );
			}
			$auto = $link . $auto;
			if( $pre ) {
				# written summary $presep autocomment (summary /* section */)
				$auto = wfMsgExt( 'autocomment-prefix', array( 'escapenoentities', 'content' ) ) . $auto;
			}
			if( $post ) {
				# autocomment $postsep written summary (/* section */ summary)
				$auto .= wfMsgExt( 'colon-separator', array( 'escapenoentities', 'content' ) );
			}
			$auto = '<span class="autocomment">' . $auto . '</span>';
			$comment = $pre . $auto . $post;
		}

		return $comment;
	}

	/**
	 * Formats wiki links and media links in text; all other wiki formatting
	 * is ignored
	 *
	 * @fixme doesn't handle sub-links as in image thumb texts like the main parser
	 * @param string $comment Text to format links in
	 * @return string
	 */
	public function formatLinksInComment( $comment ) {
		return preg_replace_callback(
			'/\[\[:?(.*?)(\|(.*?))*\]\]([^[]*)/',
			array( $this, 'formatLinksInCommentCallback' ),
			$comment );
	}

	protected function formatLinksInCommentCallback( $match ) {
		global $wgContLang;

		$medians = '(?:' . preg_quote( MWNamespace::getCanonicalName( NS_MEDIA ), '/' ) . '|';
		$medians .= preg_quote( $wgContLang->getNsText( NS_MEDIA ), '/' ) . '):';

		$comment = $match[0];

		# fix up urlencoded title texts (copied from Parser::replaceInternalLinks)
		if( strpos( $match[1], '%' ) !== false ) {
			$match[1] = str_replace( array('<', '>'), array('&lt;', '&gt;'), urldecode($match[1]) );
		}

		# Handle link renaming [[foo|text]] will show link as "text"
		if( "" != $match[3] ) {
			$text = $match[3];
		} else {
			$text = $match[1];
		}
		$submatch = array();
		if( preg_match( '/^' . $medians . '(.*)$/i', $match[1], $submatch ) ) {
			# Media link; trail not supported.
			$linkRegexp = '/\[\[(.*?)\]\]/';
			$thelink = $this->makeMediaLink( $submatch[1], "", $text );
		} else {
			# Other kind of link
			if( preg_match( $wgContLang->linkTrail(), $match[4], $submatch ) ) {
				$trail = $submatch[1];
			} else {
				$trail = "";
			}
			$linkRegexp = '/\[\[(.*?)\]\]' . preg_quote( $trail, '/' ) . '/';
			if (isset($match[1][0]) && $match[1][0] == ':')
				$match[1] = substr($match[1], 1);
			$thelink = $this->makeLink( $match[1], $text, "", $trail );
		}
		$comment = preg_replace( $linkRegexp, StringUtils::escapeRegexReplacement( $thelink ), $comment, 1 );

		return $comment;
	}

	/**
	 * Wrap a comment in standard punctuation and formatting if
	 * it's non-empty, otherwise return empty string.
	 *
	 * @param string $comment
	 * @param mixed $title Title object (to generate link to section in autocomment) or null
	 * @param bool $local Whether section links should refer to local page
	 *
	 * @return string
	 */
	function commentBlock( $comment, $title = NULL, $local = false ) {
		// '*' used to be the comment inserted by the software way back
		// in antiquity in case none was provided, here for backwards
		// compatability, acc. to brion -ævar
		if( $comment == '' || $comment == '*' ) {
			return '';
		} else {
			$formatted = $this->formatComment( $comment, $title, $local );
			return " <span class=\"comment\">($formatted)</span>";
		}
	}

	/**
	 * Wrap and format the given revision's comment block, if the current
	 * user is allowed to view it.
	 *
	 * @param Revision $rev
	 * @param bool $local Whether section links should refer to local page
	 * @param $isPublic, show only if all users can see it
	 * @return string HTML
	 */
	function revComment( Revision $rev, $local = false, $isPublic = false ) {
		if( $rev->isDeleted( Revision::DELETED_COMMENT ) && $isPublic ) {
			$block = " <span class=\"comment\">" . wfMsgHtml( 'rev-deleted-comment' ) . "</span>";
		} else if( $rev->userCan( Revision::DELETED_COMMENT ) ) {
			$block = $this->commentBlock( $rev->getRawComment(), $rev->getTitle(), $local );
		} else {
			$block = " <span class=\"comment\">" . wfMsgHtml( 'rev-deleted-comment' ) . "</span>";
		}
		if( $rev->isDeleted( Revision::DELETED_COMMENT ) ) {
			return " <span class=\"history-deleted\">$block</span>";
		}
		return $block;
	}

	public function formatRevisionSize( $size ) {
		if ( $size == 0 ) {
			$stxt = wfMsgExt( 'historyempty', 'parsemag' );
		} else {
			global $wgLang;
			$stxt = wfMsgExt( 'nbytes', 'parsemag', $wgLang->formatNum( $size ) );
			$stxt = "($stxt)";
		}
		$stxt = htmlspecialchars( $stxt );
		return "<span class=\"history-size\">$stxt</span>";
	}

	/** @todo document */
	function tocIndent() {
		return "\n<ul>";
	}

	/** @todo document */
	function tocUnindent($level) {
		return "</li>\n" . str_repeat( "</ul>\n</li>\n", $level>0 ? $level : 0 );
	}

	/**
	 * parameter level defines if we are on an indentation level
	 */
	function tocLine( $anchor, $tocline, $tocnumber, $level ) {
		return "\n<li class=\"toclevel-$level\"><a href=\"#" .
			$anchor . '"><span class="tocnumber">' .
			$tocnumber . '</span> <span class="toctext">' .
			$tocline . '</span></a>';
	}

	/** @todo document */
	function tocLineEnd() {
		return "</li>\n";
 	}

	/** @todo document */
	function tocList($toc) {
		global $wgJsMimeType;
		$title = wfMsgHtml('toc') ;
		return
		   '<table id="toc" class="toc" summary="' . $title .'"><tr><td>'
		 . '<div id="toctitle"><h2>' . $title . "</h2></div>\n"
		 . $toc
		 # no trailing newline, script should not be wrapped in a
		 # paragraph
		 . "</ul>\n</td></tr></table>"
		 . '<script type="' . $wgJsMimeType . '">'
		 . ' if (window.showTocToggle) {'
		 . ' var tocShowText = "' . wfEscapeJsString( wfMsg('showtoc') ) . '";'
		 . ' var tocHideText = "' . wfEscapeJsString( wfMsg('hidetoc') ) . '";'
		 . ' showTocToggle();'
		 . ' } '
		 . "</script>\n";
	}

	/**
	 * Used to generate section edit links that point to "other" pages
	 * (sections that are really part of included pages).
	 *
	 * @param $title Title string.
	 * @param $section Integer: section number.
	 */
	public function editSectionLinkForOther( $title, $section ) {
		$title = Title::newFromText( $title );
		return $this->doEditSectionLink( $title, $section, '', 'EditSectionLinkForOther' );
	}

	/**
	 * @param $nt Title object.
	 * @param $section Integer: section number.
	 * @param $hint Link String: title, or default if omitted or empty
	 */
	public function editSectionLink( Title $nt, $section, $hint='' ) {
		if( $hint != '' ) {
			$hint = wfMsgHtml( 'editsectionhint', htmlspecialchars( $hint ) );
			$hint = " title=\"$hint\"";
		}
		return $this->doEditSectionLink( $nt, $section, $hint, 'EditSectionLink' );
	}

	/**
	 * Implement editSectionLink and editSectionLinkForOther.
	 *
	 * @param $nt      Title object
	 * @param $section Integer, section number
	 * @param $hint    String, for HTML title attribute
	 * @param $hook    String, name of hook to run
	 * @return         String, HTML to use for edit link
	 */
	protected function doEditSectionLink( Title $nt, $section, $hint, $hook ) {
		global $wgContLang;
		$editurl = '&section='.$section;
		$url = $this->makeKnownLinkObj(
			$nt,
			htmlspecialchars(wfMsg('editsection')),
			'action=edit'.$editurl,
			'', '', '',  $hint
		);
		$result = null;

		// The two hooks have slightly different interfaces . . .
		if( $hook == 'EditSectionLink' ) {
			wfRunHooks( 'EditSectionLink', array( &$this, $nt, $section, $hint, $url, &$result ) );
		} elseif( $hook == 'EditSectionLinkForOther' ) {
			wfRunHooks( 'EditSectionLinkForOther', array( &$this, $nt, $section, $url, &$result ) );
		}

		// For reverse compatibility, add the brackets *after* the hook is run,
		// and even add them to hook-provided text.
		if( is_null( $result ) ) {
			$result = wfMsgHtml( 'editsection-brackets', $url );
		} else {
			$result = wfMsgHtml( 'editsection-brackets', $result );
		}
		return "<span class=\"editsection\">$result</span>";
	}

	/**
	 * Create a headline for content
	 *
	 * @param int    $level   The level of the headline (1-6)
	 * @param string $attribs Any attributes for the headline, starting with a space and ending with '>'
	 *                        This *must* be at least '>' for no attribs
	 * @param string $anchor  The anchor to give the headline (the bit after the #)
	 * @param string $text    The text of the header
	 * @param string $link    HTML to add for the section edit link
	 *
	 * @return string HTML headline
	 */
	public function makeHeadline( $level, $attribs, $anchor, $text, $link ) {
		return "<a name=\"$anchor\"></a><h$level$attribs$link <span class=\"mw-headline\">$text</span></h$level>";
	}

	/**
	 * Split a link trail, return the "inside" portion and the remainder of the trail
	 * as a two-element array
	 *
	 * @static
	 */
	static function splitTrail( $trail ) {
		static $regex = false;
		if ( $regex === false ) {
			global $wgContLang;
			$regex = $wgContLang->linkTrail();
		}
		$inside = '';
		if ( '' != $trail ) {
			$m = array();
			if ( preg_match( $regex, $trail, $m ) ) {
				$inside = $m[1];
				$trail = $m[2];
			}
		}
		return array( $inside, $trail );
	}

	/**
	 * Generate a rollback link for a given revision.  Currently it's the
	 * caller's responsibility to ensure that the revision is the top one. If
	 * it's not, of course, the user will get an error message.
	 *
	 * If the calling page is called with the parameter &bot=1, all rollback
	 * links also get that parameter. It causes the edit itself and the rollback
	 * to be marked as "bot" edits. Bot edits are hidden by default from recent
	 * changes, so this allows sysops to combat a busy vandal without bothering
	 * other users.
	 *
	 * @param Revision $rev
	 */
	function generateRollback( $rev ) {
		return '<span class="mw-rollback-link">['
			. $this->buildRollbackLink( $rev )
			. ']</span>';
	}

	/**
	 * Build a raw rollback link, useful for collections of "tool" links
	 *
	 * @param Revision $rev
	 * @return string
	 */
	public function buildRollbackLink( $rev ) {
		global $wgRequest, $wgUser;
		$title = $rev->getTitle();
		$extra  = $wgRequest->getBool( 'bot' ) ? '&bot=1' : '';
		$extra .= '&token=' . urlencode( $wgUser->editToken( array( $title->getPrefixedText(),
			$rev->getUserText() ) ) );
		return $this->makeKnownLinkObj(
			$title,
			wfMsgHtml( 'rollbacklink' ),
			'action=rollback&from=' . urlencode( $rev->getUserText() ) . $extra
		);
	}

	/**
	 * Returns HTML for the "templates used on this page" list.
	 *
	 * @param array $templates Array of templates from Article::getUsedTemplate
	 * or similar
	 * @param bool $preview Whether this is for a preview
	 * @param bool $section Whether this is for a section edit
	 * @return string HTML output
	 */
	public function formatTemplates( $templates, $preview = false, $section = false) {
		global $wgUser;
		wfProfileIn( __METHOD__ );

		$sk = $wgUser->getSkin();

		$outText = '';
		if ( count( $templates ) > 0 ) {
			# Do a batch existence check
			$batch = new LinkBatch;
			foreach( $templates as $title ) {
				$batch->addObj( $title );
			}
			$batch->execute();

			# Construct the HTML
			$outText = '<div class="mw-templatesUsedExplanation">';
			if ( $preview ) {
				$outText .= wfMsgExt( 'templatesusedpreview', array( 'parse' ) );
			} elseif ( $section ) {
				$outText .= wfMsgExt( 'templatesusedsection', array( 'parse' ) );
			} else {
				$outText .= wfMsgExt( 'templatesused', array( 'parse' ) );
			}
			$outText .= '</div><ul>';

			usort( $templates, array( 'Title', 'compare' ) );
			foreach ( $templates as $titleObj ) {
				$r = $titleObj->getRestrictions( 'edit' );
				if ( in_array( 'sysop', $r ) ) {
					$protected = wfMsgExt( 'template-protected', array( 'parseinline' ) );
				} elseif ( in_array( 'autoconfirmed', $r ) ) {
					$protected = wfMsgExt( 'template-semiprotected', array( 'parseinline' ) );
				} else {
					$protected = '';
				}
				$outText .= '<li>' . $sk->makeLinkObj( $titleObj ) . ' ' . $protected . '</li>';
			}
			$outText .= '</ul>';
		}
		wfProfileOut( __METHOD__  );
		return $outText;
	}

	/**
	 * Returns HTML for the "hidden categories on this page" list.
	 *
	 * @param array $hiddencats Array of hidden categories from Article::getHiddenCategories
	 * or similar
	 * @return string HTML output
	 */
	public function formatHiddenCategories( $hiddencats) {
		global $wgUser, $wgLang;
		wfProfileIn( __METHOD__ );

		$sk = $wgUser->getSkin();

		$outText = '';
		if ( count( $hiddencats ) > 0 ) {
			# Construct the HTML
			$outText = '<div class="mw-hiddenCategoriesExplanation">';
			$outText .= wfMsgExt( 'hiddencategories', array( 'parse' ), $wgLang->formatnum( count( $hiddencats ) ) );
			$outText .= '</div><ul>';

			foreach ( $hiddencats as $titleObj ) {
				$outText .= '<li>' . $sk->makeKnownLinkObj( $titleObj ) . '</li>'; # If it's hidden, it must exist - no need to check with a LinkBatch
			}
			$outText .= '</ul>';
		}
		wfProfileOut( __METHOD__  );
		return $outText;
	}

	/**
	 * Format a size in bytes for output, using an appropriate
	 * unit (B, KB, MB or GB) according to the magnitude in question
	 *
	 * @param $size Size to format
	 * @return string
	 */
	public function formatSize( $size ) {
		global $wgLang;
		return htmlspecialchars( $wgLang->formatSize( $size ) );
	}

	/**
	 * Given the id of an interface element, constructs the appropriate title
	 * and accesskey attributes from the system messages.  (Note, this is usu-
	 * ally the id but isn't always, because sometimes the accesskey needs to
	 * go on a different element than the id, for reverse-compatibility, etc.)
	 *
	 * @param string $name Id of the element, minus prefixes.
	 * @return string title and accesskey attributes, ready to drop in an
	 *   element (e.g., ' title="This does something [x]" accesskey="x"').
	 */
	public function tooltipAndAccesskey( $name ) {
		wfProfileIn( __METHOD__ );
		$attribs = array();

		$tooltip = wfMsg( "tooltip-$name" );
		if( !wfEmptyMsg( "tooltip-$name", $tooltip ) && $tooltip != '-' ) {
			// Compatibility: formerly some tooltips had [alt-.] hardcoded
			$tooltip = preg_replace( "/ ?\[alt-.\]$/", '', $tooltip );
			$attribs['title'] = $tooltip;
		}

		$accesskey = wfMsg( "accesskey-$name" );
		if( $accesskey && $accesskey != '-' &&
		!wfEmptyMsg( "accesskey-$name", $accesskey ) ) {
			if( isset( $attribs['title'] ) ) {
				$attribs['title'] .= " [$accesskey]";
			}
			$attribs['accesskey'] = $accesskey;
		}

		$ret = Xml::expandAttributes( $attribs );
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	/**
	 * Given the id of an interface element, constructs the appropriate title
	 * attribute from the system messages.  (Note, this is usually the id but
	 * isn't always, because sometimes the accesskey needs to go on a different
	 * element than the id, for reverse-compatibility, etc.)
	 *
	 * @param string $name    Id of the element, minus prefixes.
	 * @param mixed  $options null or the string 'withaccess' to add an access-
	 *   key hint
	 * @return string title attribute, ready to drop in an element
	 * (e.g., ' title="This does something"').
	 */
	public function tooltip( $name, $options = null ) {
		wfProfileIn( __METHOD__ );

		$attribs = array();

		$tooltip = wfMsg( "tooltip-$name" );
		if( !wfEmptyMsg( "tooltip-$name", $tooltip ) && $tooltip != '-' ) {
			$attribs['title'] = $tooltip;
		}

		if( isset( $attribs['title'] ) && $options == 'withaccess' ) {
			$accesskey = wfMsg( "accesskey-$name" );
			if( $accesskey && $accesskey != '-' &&
			!wfEmptyMsg( "accesskey-$name", $accesskey ) ) {
				$attribs['title'] .= " [$accesskey]";
			}
		}

		$ret = Xml::expandAttributes( $attribs );
		wfProfileOut( __METHOD__ );
		return $ret;
	}
}
