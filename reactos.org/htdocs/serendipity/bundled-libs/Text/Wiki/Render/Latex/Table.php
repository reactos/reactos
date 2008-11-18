<?php

class Text_Wiki_Render_Latex_Table extends Text_Wiki_Render {
    var $cell_id    = 0;
    var $cell_count = 0;
    var $is_spanning = false;
    
    var $conf = array(
                      'css_table' => null,
                      'css_tr' => null,
                      'css_th' => null,
                      'css_td' => null
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
        // make nice variable names (type, attr, span)
        extract($options);

        switch ($type)
            {
            case 'table_start':
                $this->cell_count = $cols;
                
                $tbl_start = '\begin{tabular}{|';
                for ($a=0; $a < $this->cell_count; $a++) {
                    $tbl_start .= 'l|';
                }
                $tbl_start .= "}\n";
                
                return $tbl_start;

            case 'table_end':
                return "\\hline\n\\end{tabular}\n";

            case 'row_start':
                $this->is_spanning = false;
                $this->cell_id = 0;
                return "\\hline\n";

            case 'row_end':
                return "\\\\\n";

            case 'cell_start':
                if ($span > 1) {
                    $col_spec = '';
                    if ($this->cell_id == 0) {
                        $col_spec = '|';
                    }
                    $col_spec .= 'l|';
                        
                    $this->cell_id += $span;
                    $this->is_spanning = true;
                    
                    return "\\multicolumn\{$span}\{$col_spec}{";
                }

                $this->cell_id += 1;
                return '';

            case 'cell_end':
                $out = '';
                if ($this->is_spanning) {
                    $this->is_spanning = false;
                    $out = '}';
                }
                
                if ($this->cell_id != $this->cell_count) {
                    $out .= ' & ';
                }

                return $out;

            default:
                return '';

            }
    }
}
?>