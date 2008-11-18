<?php

/*
 * Created on Sep 19, 2006
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2006 Yuri Astrakhan <Firstname><Lastname>@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

if (!defined('MEDIAWIKI')) {
	// Eclipse helper - will be ignored in production
	require_once ('ApiBase.php');
}

/**
 * This is the abstract base class for API formatters.
 *
 * @ingroup API
 */
abstract class ApiFormatBase extends ApiBase {

	private $mIsHtml, $mFormat, $mUnescapeAmps, $mHelp, $mCleared;

	/**
	* Create a new instance of the formatter.
	* If the format name ends with 'fm', wrap its output in the proper HTML.
	*/
	public function __construct($main, $format) {
		parent :: __construct($main, $format);

		$this->mIsHtml = (substr($format, -2, 2) === 'fm'); // ends with 'fm'
		if ($this->mIsHtml)
			$this->mFormat = substr($format, 0, -2); // remove ending 'fm'
		else
			$this->mFormat = $format;
		$this->mFormat = strtoupper($this->mFormat);
		$this->mCleared = false;
	}

	/**
	 * Overriding class returns the mime type that should be sent to the client.
	 * This method is not called if getIsHtml() returns true.
	 * @return string
	 */
	public abstract function getMimeType();

	/**
	 * If formatter outputs data results as is, the results must first be sanitized.
	 * An XML formatter on the other hand uses special tags, such as "_element" for special handling,
	 * and thus needs to override this function to return true.
	 */
	public function getNeedsRawData() {
		return false;
	}

	/**
	 * Specify whether or not ampersands should be escaped to '&amp;' when rendering. This
	 * should only be set to true for the help message when rendered in the default (xmlfm)
	 * format. This is a temporary special-case fix that should be removed once the help
	 * has been reworked to use a fully html interface.
	 *
	 * @param boolean Whether or not ampersands should be escaped.
	 */
	public function setUnescapeAmps ( $b ) {
		$this->mUnescapeAmps = $b;
	}

	/**
	 * Returns true when an HTML filtering printer should be used.
	 * The default implementation assumes that formats ending with 'fm'
	 * should be formatted in HTML.
	 */
	public function getIsHtml() {
		return $this->mIsHtml;
	}

	/**
	 * Initialize the printer function and prepares the output headers, etc.
	 * This method must be the first outputing method during execution.
	 * A help screen's header is printed for the HTML-based output
	 */
	function initPrinter($isError) {
		$isHtml = $this->getIsHtml();
		$mime = $isHtml ? 'text/html' : $this->getMimeType();
		$script = wfScript( 'api' );

		// Some printers (ex. Feed) do their own header settings,
		// in which case $mime will be set to null
		if (is_null($mime))
			return; // skip any initialization

		header("Content-Type: $mime; charset=utf-8");

		if ($isHtml) {
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<?php if ($this->mUnescapeAmps) {
?>	<title>MediaWiki API</title>
<?php } else {
?>	<title>MediaWiki API Result</title>
<?php } ?>
</head>
<body>
<?php


			if( !$isError ) {
?>
<br/>
<small>
You are looking at the HTML representation of the <?php echo( $this->mFormat ); ?> format.<br/>
HTML is good for debugging, but probably is not suitable for your application.<br/>
See <a href='http://www.mediawiki.org/wiki/API'>complete documentation</a>, or
<a href='<?php echo( $script ); ?>'>API help</a> for more information.
</small>
<?php


			}
?>
<pre>
<?php


		}
	}

	/**
	 * Finish printing. Closes HTML tags.
	 */
	public function closePrinter() {
		if ($this->getIsHtml()) {
?>

</pre>
</body>
</html>
<?php


		}
	}

	/**
	 * The main format printing function. Call it to output the result string to the user.
	 * This function will automatically output HTML when format name ends in 'fm'.
	 */
	public function printText($text) {
		if ($this->getIsHtml())
			echo $this->formatHTML($text);
		else
		{
			// For non-HTML output, clear all errors that might have been
			// displayed if display_errors=On
			// Do this only once, of course
			if(!$this->mCleared)
			{
				ob_clean();
				$this->mCleared = true;
			}
			echo $text;
		}
	}

	/**
	* Says pretty-printer that it should use *bold* and $italics$ formatting
	*/
	public function setHelp( $help = true ) {
		$this->mHelp = true;
	}

	/**
	* Prety-print various elements in HTML format, such as xml tags and URLs.
	* This method also replaces any '<' with &lt;
	*/
	protected function formatHTML($text) {
		// Escape everything first for full coverage
		$text = htmlspecialchars($text);

		// encode all comments or tags as safe blue strings
		$text = preg_replace('/\&lt;(!--.*?--|.*?)\&gt;/', '<span style="color:blue;">&lt;\1&gt;</span>', $text);
		// identify URLs
		$protos = "http|https|ftp|gopher";
		# This regex hacks around bug 13218 (&quot; included in the URL)
		$text = preg_replace("#(($protos)://.*?)(&quot;)?([ \\'\"()<\n])#", '<a href="\\1">\\1</a>\\3\\4', $text);
		// identify requests to api.php
		$text = preg_replace("#api\\.php\\?[^ \\()<\n\t]+#", '<a href="\\0">\\0</a>', $text);
		if( $this->mHelp ) {
			// make strings inside * bold
			$text = ereg_replace("\\*[^<>\n]+\\*", '<b>\\0</b>', $text);
			// make strings inside $ italic
			$text = ereg_replace("\\$[^<>\n]+\\$", '<b><i>\\0</i></b>', $text);
		}

		/* Temporary fix for bad links in help messages. As a special case,
		 * XML-escaped metachars are de-escaped one level in the help message
		 * for legibility. Should be removed once we have completed a fully-html
		 * version of the help message. */
		if ( $this->mUnescapeAmps )
			$text = preg_replace( '/&amp;(amp|quot|lt|gt);/', '&\1;', $text );

		return $text;
	}

	/**
	 * Returns usage examples for this format.
	 */
	protected function getExamples() {
		return 'api.php?action=query&meta=siteinfo&siprop=namespaces&format=' . $this->getModuleName();
	}

	public function getDescription() {
		return $this->getIsHtml() ? ' (pretty-print in HTML)' : '';
	}

	public static function getBaseVersion() {
		return __CLASS__ . ': $Id: ApiFormatBase.php 36153 2008-06-10 15:20:22Z tstarling $';
	}
}

/**
 * This printer is used to wrap an instance of the Feed class
 * @ingroup API
 */
class ApiFormatFeedWrapper extends ApiFormatBase {

	public function __construct($main) {
		parent :: __construct($main, 'feed');
	}

	/**
	 * Call this method to initialize output data. See self::execute()
	 */
	public static function setResult($result, $feed, $feedItems) {
		// Store output in the Result data.
		// This way we can check during execution if any error has occured
		$data = & $result->getData();
		$data['_feed'] = $feed;
		$data['_feeditems'] = $feedItems;
	}

	/**
	 * Feed does its own headers
	 */
	public function getMimeType() {
		return null;
	}

	/**
	 * Optimization - no need to sanitize data that will not be needed
	 */
	public function getNeedsRawData() {
		return true;
	}

	/**
	 * This class expects the result data to be in a custom format set by self::setResult()
	 * $result['_feed']		 - an instance of one of the $wgFeedClasses classes
	 * $result['_feeditems'] - an array of FeedItem instances
	 */
	public function execute() {
		$data = $this->getResultData();
		if (isset ($data['_feed']) && isset ($data['_feeditems'])) {
			$feed = $data['_feed'];
			$items = $data['_feeditems'];

			$feed->outHeader();
			foreach ($items as & $item)
				$feed->outItem($item);
			$feed->outFooter();
		} else {
			// Error has occured, print something useful
			ApiBase::dieDebug( __METHOD__, 'Invalid feed class/item' );
		}
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiFormatBase.php 36153 2008-06-10 15:20:22Z tstarling $';
	}
}
