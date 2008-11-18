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
// $Id: paragraph.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki rule to find sections of the source
* text that are paragraphs.  A para is any line not starting with a token
* delimiter, followed by two newlines.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_paragraph extends Text_Wiki_Rule {
    
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
    
    var $regex = "/^.*?\n\n/m";
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'start' => The starting point of the paragraph.
    * 
    * 'end' => The endinging point of the paragraph.
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
    	$delim = $this->_wiki->delim;
    	
    	// was anything there?
    	if (trim($matches[0]) == '') {
    		return '';
    	}
    	
    	// does the match start with a delimiter?
    	if (substr($matches[0], 0, 1) != $delim) { 
    		// no.
            $start = $this->addToken(array('type' => 'start'));
            $end = $this->addToken(array('type' => 'end'));
            return $start . trim($matches[0]) . $end;
        }
        
        // the line starts with a delimiter.  read in the delimited
        // token number, check the token, and see if we should
        // skip it.
        
        // loop starting at the second character (we already know
        // the first is a delimiter) until we find another
        // delimiter; the text between them is a token key number.
        $key = '';
		$len = strlen($matches[0]);
		for ($i = 1; $i < $len; $i++) {
        	$char = $matches[0]{$i};
        	if ($char == $delim) {
        		break;
        	} else {
        		$key .= $char;
        	}
        }
        
        // look at the token and see if it's skippable (if we skip,
        // it will not be marked as a paragraph)
        $token_type = $this->_wiki->_tokens[$key][0];
        if (in_array($token_type, $this->_conf['skip'])) {
        	return $matches[0];
        } else {
            $start = $this->addToken(array('type' => 'start'));
            $end = $this->addToken(array('type' => 'end'));
            return $start . trim($matches[0]) . $end;
        }
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
        extract($options); //type
        
        if ($type == 'start') {
            return '<p>';
        }
        
        if ($type == 'end') {
            return "</p>\n\n";
        }
    }
}
?>
