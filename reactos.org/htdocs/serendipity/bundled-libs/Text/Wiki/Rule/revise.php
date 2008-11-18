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
// $Id: revise.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked for
* revision.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_revise extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to parse the source text and find
    * matches conforming to this rule.  Used by the parse() method.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    * 
    */
    
    var $regex = "/\@\@({*?.*}*?)\@\@/U";
    
    
    /**
    * 
    * The characters to use as marking text to be stricken.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    *
    */
    
    var $delmark = '---';
    
    
    /**
    * 
    * The characters to use as marking text to be added.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    * 
    */
    
    var $insmark = '+++';
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' => ['start'|'end'] The starting or ending point of the
    * inserted text.  The text itself is left in the source.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return string A pair of delimited tokens to be used as a
    * placeholder in the source text surrounding the teletype text.
    *
    */
    
    function process(&$matches)
    {
        $output = '';
        $src = $matches[1];
        $delmark = $this->delmark;
        $insmark = $this->insmark;
        
        // '---' must be before '+++' (if they both appear)
        $del = strpos($src, $delmark);
        $ins = strpos($src, $insmark);
        
        // if neither is found, return right away
        if ($del === false && $ins === false) {
            return $matches[0];
        }
        
        // handle text to be deleted
        if ($del !== false) {
            
            // move forward to the end of the deletion mark
            $del += strlen($delmark);
            
            if ($ins === false) {
                // there is no insertion text following
                $text = substr($src, $del);
            } else {
                // there is insertion text following,
                // mitigate the length
                $text = substr($src, $del, $ins - $del);
            }
            
            $output .= $this->addToken(array('type' => 'del_start'));
            $output .= $text;
            $output .= $this->addToken(array('type' => 'del_end'));
        }
        
        // handle text to be inserted
        if ($ins !== false) {
            
            // move forward to the end of the insert mark
            $ins += strlen($insmark);
            $text = substr($src, $ins);
            
            $output .= $this->addToken(array('type' => 'ins_start'));
            $output .= $text;
            $output .= $this->addToken(array('type' => 'ins_end'));
        }
        
        return $output;
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
        if ($options['type'] == 'del_start') {
            return '<del>';
        }
        
        if ($options['type'] == 'del_end') {
            return '</del>';
        }
        
        if ($options['type'] == 'ins_start') {
            return '<ins>';
        }
        
        if ($options['type'] == 'ins_end') {
            return '</ins>';
        }
    }
}
?>
