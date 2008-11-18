<?php
// $Id: Image.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to embed the contents of a URL
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

class Text_Wiki_Parse_Image extends Text_Wiki_Parse {
    
    
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
                'attr' => array());
        } else {
            // everything after the space is attribute arguments
            $options = array(
                'src' => substr($matches[2], 0, $pos),
                'attr' => $this->getAttrs(substr($matches[2], $pos+1))
            );
        }
        
        return $this->wiki->addToken($this->rule, $options);
    }
}
?>