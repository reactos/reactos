<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: url.php,v 1.3 2004/12/02 10:54:32 nohn Exp $

/**
* 
* This class implements a Text_Wiki_Rule to find source text marked as a
* URL.  Various URL markings are supported: inline (the URL by itself),
* numbered or footnote reference (where the URL is enclosed in square brackets), and
* named reference (where the URL is enclosed in square brackets and has a
* name included inside the brackets).  E.g.:
*
* inline    -- http://example.com
* numbered  -- [http://example.com]
* described -- [http://example.com Example Description]
*
* When rendering a URL token, this will convert URLs pointing to a .gif,
* .jpg, or .png image into an inline <img /> tag (for the 'xhtml'
* format).
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_url extends Text_Wiki_Rule {
    
    
    /**
    * 
    * When doing numbered references (footnote-style references), we
    * need to keep a running count of how many references there are.
    * 
    * @access public
    * 
    * @var int
    * 
    */
    
    var $footnoteCount = 0;
    
    
    /**
    * 
    * An array of filename extensions that indicate a file is an image.
    * 
    * @access public
    * 
    * @var array
    * 
    */
    
    var $img_ext = array('.jpg', '.png', '.gif');
    
    
    function Text_Wiki_Rule_url(&$obj, $name)
    {
    	parent::Text_Wiki_Rule($obj, $name);
    	
        $this->regex = 
        	"(http:\/\/|https:\/\/|ftp:\/\/|gopher:\/\/|news:\/\/|mailto:)" . // protocols
	        "(" . 
	        "[^ \\/\"\'{$this->_wiki->delim}]*\\/" . // no spaces, \, /, ", or single quotes;
	        ")*" . 
	        "[^ \\t\\n\\/\"\'{$this->_wiki->delim}]*" .
	        "[A-Za-z0-9\\/?=&~_]";
		
    }
    
    
    /**
    * 
    * A somewhat complex parsing method to find three different kinds
    * of URLs in the source text.
    *
    * @access public
    * 
    */
    
    function parse()
    {
        // -------------------------------------------------------------
        // 
        // Described-reference (named) URLs.
        // 
        
        // the regular expression for this kind of URL
        $tmp_regex = '/\[(' . $this->regex . ') ([^\]]+)\]/';
        
        // use a custom callback processing method to generate
        // the replacement text for matches.
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'processDescr'),
            $this->_wiki->_source
        );
        
        
        // -------------------------------------------------------------
        // 
        // Numbered-reference (footnote-style) URLs.
        // 
        
        // the regular expression for this kind of URL
        $tmp_regex = '/\[(' . $this->regex . ')\]/U';
        
        // use a custom callback processing method to generate
        // the replacement text for matches.
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'processFootnote'),
            $this->_wiki->_source
        );
        
        
        // -------------------------------------------------------------
        // 
        // Normal inline URLs.
        // 
        
        // the regular expression for this kind of URL
		
		$tmp_regex = '/(^|[^A-Za-z])(' . $this->regex . ')(.*?)/';
		
        // use the standard callback for inline URLs
        $this->_wiki->_source = preg_replace_callback(
            $tmp_regex,
            array(&$this, 'process'),
            $this->_wiki->_source
        );
    }
    
    
    /**
    * 
    * Process inline URLs and return replacement text with a delimited
    * token.
    * 
    * Token options are:
    *     'type' => ['inline'|'footnote'|'descr'] the type of URL
    *     'href' => the URL link href portion
    *     'text' => the displayed text of the URL link
    * 
    * @param array &$matches
    * 
    * @param array $matches An array of matches from the parse() method
    * as generated by preg_replace_callback.  $matches[0] is the full
    * matched string, $matches[1] is the first matched pattern,
    * $matches[2] is the second matched pattern, and so on.
    * 
    * @return string The processed text replacement.
    * 
    */ 
    
    function process(&$matches)
    {
        // set options
        $options = array(
            'type' => 'inline',
            'href' => $matches[2],
            'text' => $matches[2]
        );
        
        // tokenize
        return $matches[1] . $this->addToken($options) . $matches[5];
    }
    
    
    /**
    * 
    * Process numbered (footnote) URLs and return replacement text with
    * a delimited token.
    * 
    * Token options are:
    *     'type' => ['inline'|'footnote'|'descr'] the type of URL
    *     'href' => the URL link href portion
    *     'text' => the displayed text of the URL link
    * 
    * @param array &$matches
    * 
    * @param array $matches An array of matches from the parse() method
    * as generated by preg_replace_callback.  $matches[0] is the full
    * matched string, $matches[1] is the first matched pattern,
    * $matches[2] is the second matched pattern, and so on.
    * 
    * @return string The processed text replacement.
    * 
    */ 
    
    function processFootnote(&$matches)
    {
        // keep a running count for footnotes 
        $this->footnoteCount++;
        
        // set options
        $options = array(
            'type' => 'footnote',
            'href' => $matches[1],
            'text' => $this->footnoteCount
        );
        
        // tokenize
        return $this->addToken($options);
    }
    
    
    /**
    * 
    * Process described-reference (named-reference) URLs and return
    * replacement text with a delimited token.
    * 
    * Token options are:
    *     'type' => ['inline'|'footnote'|'descr'] the type of URL
    *     'href' => the URL link href portion
    *     'text' => the displayed text of the URL link
    * 
    * @param array &$matches
    * 
    * @param array $matches An array of matches from the parse() method
    * as generated by preg_replace_callback.  $matches[0] is the full
    * matched string, $matches[1] is the first matched pattern,
    * $matches[2] is the second matched pattern, and so on.
    * 
    * @return string The processed text replacement.
    * 
    */ 
    
    function processDescr(&$matches)
    {
        // set options
        $options = array(
            'type' => 'descr',
            'href' => $matches[1],
            'text' => $matches[4]
        );
        
        // tokenize
        return $this->addToken($options);
    }
    
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function renderXhtml($options)
    {
        // create local variables from the options array (text,
        // href, type)
        extract($options);
        
        // find the rightmost dot and determine the filename
        // extension.
        $pos = strrpos($href, '.');
        $ext = strtolower(substr($href, $pos));
        
        // does the filename extension indicate an image file?
        if (in_array($ext, $this->img_ext)) {
            
            // create alt text for the image
            if (! isset($text) || $text == '') {
                $text = basename($href);
            }
            
            // generate an image tag
            $output = "<img src=\"$href\" alt=\"$text\" />";
            
        } else {
        	
        	// allow for alternative targets
        	if (isset($this->_conf['target']) &&
        		trim($this->_conf['target']) != '') {
        		$target = 'target="' . $this->_conf['target'] . '"';
        	} else {
        		$target = '';
        	}
        	
            // generate a regular link (not an image)
            $output = "<a $target href=\"$href\">$text</a>";
            
            // make numbered references look like footnotes
            if ($type == 'footnote') {
                $output = '<sup>' . $output . '</sup>';
            }
        }
        
        return $output;
    }
}
?>
