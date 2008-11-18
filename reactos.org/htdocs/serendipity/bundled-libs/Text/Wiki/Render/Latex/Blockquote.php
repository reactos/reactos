<?php

class Text_Wiki_Render_Latex_Blockquote extends Text_Wiki_Render {
    
    var $conf = array('css' => null);
    
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
    
        // starting
        if ($type == 'start') {
            return "\\begin{quote}\n";
        }
        
        // ending
        if ($type == 'end') {
            return "\\end{quote}\n\n";
        }
    }
}
?>