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
// $Id: image.php,v 1.2 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to embed the contents of a URL
* inside the page.  Typically used to get script output.
*
* This rule is inherently not secure; it allows cross-site scripting to
* occur if the embedded output has <script> or other similar tags.  Be
* careful.
*
* In the future, we'll add a rule config options to set the base embed
* path so that it is limited to one directory.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_image extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to find source text matching this
    * rule.
    * 
    * @access public
    * 
    * @var string
    * 
    */
    
    var $regex = '/(\[\[image )(.+?)(\]\])/i';
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'src' => The image source, typically a relative path name.
    *
    * 'opts' => Any macro options following the source.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token number to be used as a placeholder in
    * the source text.
    *
    */
    
    function process(&$matches)
    {
    	$pos = strpos($matches[2], ' ');
    	
    	if ($pos === false) {
	        $options = array(
	        	'src' => $matches[2],
	        	'args' => array());
    	} else {
    		// everything after the space is macro options
    		$options = array(
    			'src' => substr($matches[2], 0, $pos),
    			'args' => $this->getMacroArgs(substr($matches[2], $pos+1))
    		);
    	}
    	
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
    	$src = '"' . $this->_conf['base'] . $options['src'] . '"';
    	
    	if (isset($options['args']['link'])) {
			// this image has a wikilink
    		$href = $this->_wiki->getRuleConf('wikilink', 'view_url') .
    			$options['args']['link'];
    	} else {
    		// image is not linked
    		$href = null;
    	}
    	
    	// unset these so they don't show up as attributes
    	unset($options['args']['src']);
    	unset($options['args']['link']);
    	
    	$attr = '';
    	foreach ($options['args'] as $key => $val) {
    		$attr .= "$key=\"$val\" ";
    	}
    	
    	if ($href) {
    		return "<a href=\"$href\"><img src=$src $attr/></a>";
    	} else {
	    	return "<img src=$src $attr/>";
	    }
	}
}
?>
