<?php

class Text_Wiki_Render_Xhtml_Blockquote extends Text_Wiki_Render {
    
    var $conf = array(
        'css' => null
    );
    
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
    
    function token($options)
    {
        $type = $options['type'];
        $level = $options['level'];
    
        // set up indenting so that the results look nice; we do this
        // in two steps to avoid str_pad mathematics.  ;-)
        $pad = str_pad('', $level, "\t");
        $pad = str_replace("\t", '    ', $pad);
        
        // pick the css type
        $css = $this->formatConf(' class="%s"', 'css');
        
        // starting
        if ($type == 'start') {
            return "$pad<blockquote$css>";
        }
        
        // ending
        if ($type == 'end') {
            return $pad . "</blockquote>\n";
        }
    }
}
?>