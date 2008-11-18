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
// $Id: heading.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find source text marked to
* be a heading element, as defined by text on a line by itself prefixed
* with a number of plus signs (+). The heading text itself is left in
* the source, but is prefixed and suffixed with delimited tokens marking
* the start and end of the heading.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_heading extends Text_Wiki_Rule {
    
    
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
    
    var $regex = '/^(\+{1,6}) (.*)/m';
    
    
    /**
    * 
    * Generates a replacement for the matched text.  Token options are:
    * 
    * 'type' => ['start'|'end'] The starting or ending point of the
    * heading text.  The text itself is left in the source.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return string A pair of delimited tokens to be used as a
    * placeholder in the source text surrounding the heading text.
    *
    */
    
    function process(&$matches)
    {
        $start = $this->addToken(
            array(
                'type' => 'start',
                'level' => strlen($matches[1]),
                'text' => $matches[2]
            )
        );
        
        $end = $this->addToken(
            array(
                'type' => 'end',
                'level' => strlen($matches[1])
            )
        );
        
        return $start . $matches[2] . $end . "\n";
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
        // get nice variable names (type, level)
        extract($options);
        
        if ($type == 'start') {
            return "<h$level>";
        }
        
        if ($type == 'end') {
            return "</h$level>\n";
        }
    }
}
?>
