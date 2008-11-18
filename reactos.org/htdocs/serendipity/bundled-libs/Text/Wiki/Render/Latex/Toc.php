<?php

class Text_Wiki_Render_Latex_Toc extends Text_Wiki_Render {
    
    
    
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
        if($options['type'] == 'list_start') {
            return "\\tableofcontents\n\n";
        }
        
        return '';
    }

}
?>