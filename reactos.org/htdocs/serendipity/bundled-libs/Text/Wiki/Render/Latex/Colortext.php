<?php

class Text_Wiki_Render_Latex_Colortext extends Text_Wiki_Render {
    
    var $colors = array(
        'aqua',
        'black',
        'blue',
        'fuchsia',
        'gray',
        'green',
        'lime',
        'maroon',
        'navy',
        'olive',
        'purple',
        'red',
        'silver',
        'teal',
        'white',
        'yellow'
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
        return 'Colortext: NI';
        
        $type = $options['type'];
        $color = $options['color'];
        
        if (! in_array($color, $this->colors)) {
            $color = '#' . $color;
        }
        
        if ($type == 'start') {
            return "<span style=\"color: $color;\">";
        }
        
        if ($options['type'] == 'end') {
            return '</span>';
        }
    }
}
?>