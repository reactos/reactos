/* The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Bugzilla Bug Tracking System.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): Christian Reis <kiko@async.com.br>
 */

/* this file contains functions to update form controls based on a
 * collection of javascript arrays containing strings */

/* selectClassification reads the selection from f.classification and updates
 * f.product accordingly
 *     - f: a form containing classification, product, component, varsion and
 *       target_milestone select boxes.
 * globals (3vil!):
 *     - prods, indexed by classification name
 *     - first_load: boolean, specifying if it is the first time we load
 *       the query page.
 *     - last_sel: saves our last selection list so we know what has
 *       changed, and optimize for additions.
 */
function selectClassification(classfield, product, component, version, milestone) {
    /* this is to avoid handling events that occur before the form
     * itself is ready, which could happen in buggy browsers.
     */
    if (!classfield) {
        return;
    }

    /* if this is the first load and nothing is selected, no need to
     * merge and sort all components; perl gives it to us sorted.
     */
    if ((first_load) && (classfield.selectedIndex == -1)) {
        first_load = false;
        return;
    }
    
    /* don't reset first_load as done in selectProduct.  That's because we
       want selectProduct to handle the first_load attribute
    */

    /* - sel keeps the array of classifications we are selected. 
     * - merging says if it is a full list or just a list of classifications 
     *   that were added to the current selection.
     */
    var merging = false;
    var sel = Array();

    /* if nothing selected, pick all */
    var findall = classfield.selectedIndex == -1;
    sel = get_selection(classfield, findall, false);
    if (!findall) {
        /* save sel for the next invocation of selectClassification() */
        var tmp = sel;
    
        /* this is an optimization: if we have just added classifications to an
         * existing selection, no need to clear the form controls and add 
         * everybody again; just merge the new ones with the existing 
         * options.
	 */
        if ((last_sel.length > 0) && (last_sel.length < sel.length)) {
            sel = fake_diff_array(sel, last_sel);
            merging = true;
        }
        last_sel = tmp;
    }
    /* save original options selected */
    var saved_prods = get_selection(product, false, true);

    /* do the actual fill/update, reselect originally selected options */
    updateSelect(prods, sel, product, merging);
    restoreSelection(product, saved_prods);
    selectProduct(product, component, version, milestone);
}


/* selectProduct reads the selection from the product control and
 * updates version, component and milestone controls accordingly.
 * 
 *     - product, component, version and milestone: form controls
 *
 * globals (3vil!):
 *     - cpts, vers, tms: array of arrays, indexed by product name. the
 *       subarrays contain a list of names to be fed to the respective
 *       selectboxes. For bugzilla, these are generated with perl code
 *       at page start.
 *     - first_load: boolean, specifying if it is the first time we load
 *       the query page.
 *     - last_sel: saves our last selection list so we know what has
 *       changed, and optimize for additions. 
 */
function selectProduct(product, component, version, milestone) {

    if (!product) {
        /* this is to avoid handling events that occur before the form
         * itself is ready, which could happen in buggy browsers. */
        return;
    }

    /* if this is the first load and nothing is selected, no need to
     * merge and sort all components; perl gives it to us sorted. */
    if ((first_load) && (product.selectedIndex == -1)) {
        first_load = false;
        return;
    }

    /* turn first_load off. this is tricky, since it seems to be
     * redundant with the above clause. It's not: if when we first load
     * the page there is _one_ element selected, it won't fall into that
     * clause, and first_load will remain 1. Then, if we unselect that
     * item, selectProduct will be called but the clause will be valid
     * (since selectedIndex == -1), and we will return - incorrectly -
     * without merge/sorting. */
    first_load = false;

    /* - sel keeps the array of products we are selected.
     * - merging says if it is a full list or just a list of products that
     *   were added to the current selection. */
    var merging = false;
    var sel = Array();

    /* if nothing selected, pick all */
    var findall = product.selectedIndex == -1;
    if (useclassification) {
        /* update index based on the complete product array */
        sel = get_selection(product, findall, true);
        for (var i=0; i<sel.length; i++) {
           sel[i] = prods[sel[i]];
        }
    } else {
        sel = get_selection(product, findall, false);
    }
    if (!findall) {
        /* save sel for the next invocation of selectProduct() */
        var tmp = sel;

        /* this is an optimization: if we have just added products to an
         *  existing selection, no need to clear the form controls and add
         *  everybody again; just merge the new ones with the existing
         *  options. */
        if ((last_sel.length > 0) && (last_sel.length < sel.length)) {
            sel = fake_diff_array(sel, last_sel);
            merging = true;
        }
        last_sel = tmp;
    }

    /* do the actual fill/update */
    if (component) {
        var saved_cpts = get_selection(component, false, true);
        updateSelect(cpts, sel, component, merging);
        restoreSelection(component, saved_cpts);
    }

    if (version) {
        var saved_vers = get_selection(version, false, true);
        updateSelect(vers, sel, version, merging);
        restoreSelection(version, saved_vers);
    }

    if (milestone) {
        var saved_tms = get_selection(milestone, false, true);
        updateSelect(tms, sel, milestone, merging);
        restoreSelection(milestone, saved_tms);
    }
}


/* updateSelect(array, sel, target, merging)
 *
 * Adds to the target select object all elements in array that
 * correspond to the elements selected in source.
 * - array should be a array of arrays, indexed by number. the
 *   array should contain the elements that correspond to that
 *   product.
 * - sel is a list of selected items, either whole or a diff
 *   depending on merging.
 * - target should be the target select object.
 * - merging (boolean) determines if we are mergine in a diff or
 *   substituting the whole selection. a diff is used to optimize adding
 *   selections.
 *
 * Example (compsel is a select form control)
 *
 *     var components = Array();
 *     components[1] = [ 'ComponentA', 'ComponentB' ];
 *     components[2] = [ 'ComponentC', 'ComponentD' ];
 *     source = [ 2 ];
 *     updateSelect(components, source, compsel, 0, 0);
 *
 * would clear compsel and add 'ComponentC' and 'ComponentD' to it.
 *
 */

function updateSelect(array, sel, target, merging) {

    var i, item;

    /* If we have no versions/components/milestones */
    if (array.length < 1) {
        target.options.length = 0;
        return false;
    }

    if (merging) {
        /* array merging/sorting in the case of multiple selections */
        /* merge in the current options with the first selection */
        item = merge_arrays(array[sel[0]], target.options, 1);

        /* merge the rest of the selection with the results */
        for (i = 1 ; i < sel.length ; i++) {
            item = merge_arrays(array[sel[i]], item, 0);
        }
    } else if ( sel.length > 1 ) {
        /* here we micro-optimize for two arrays to avoid merging with a
         * null array */
        item = merge_arrays(array[sel[0]],array[sel[1]], 0);

        /* merge the arrays. not very good for multiple selections. */
        for (i = 2; i < sel.length; i++) {
            item = merge_arrays(item, array[sel[i]], 0);
        }
    } else { /* single item in selection, just get me the list */
        item = array[sel[0]];
    }

    /* clear select */
    target.options.length = 0;

    /* load elements of list into select */
    for (i = 0; i < item.length; i++) {
        target.options[i] = new Option(item[i], item[i]);
    }
    return true;
}


/* Selects items in control that have index defined in sel
 *    - control: SELECT control to be restored
 *    - selnames: array of indexes in select form control */
function restoreSelection(control, selnames) {
    /* right. this sucks. but I see no way to avoid going through the
     * list and comparing to the contents of the control. */
    for (var j=0; j < selnames.length; j++) {
        for (var i=0; i < control.options.length; i++) {
            if (control.options[i].value == selnames[j]) {
                control.options[i].selected = true;
            }
        }
    }
}


/* Returns elements in a that are not in b.
 * NOT A REAL DIFF: does not check the reverse.
 *    - a,b: arrays of values to be compare. */
function fake_diff_array(a, b) {
    var newsel = new Array();
    var found = false;

    /* do a boring array diff to see who's new */
    for (var ia in a) {
        for (var ib in b) {
            if (a[ia] == b[ib]) {
                found = true;
            }
        }
        if (!found) {
            newsel[newsel.length] = a[ia];
        }
        found = false;
    }
    return newsel;
}

/* takes two arrays and sorts them by string, returning a new, sorted
 * array. the merge removes dupes, too.
 *    - a, b: arrays to be merge.
 *    - b_is_select: if true, then b is actually an optionitem and as
 *      such we need to use item.value on it. */
function merge_arrays(a, b, b_is_select) {
    var pos_a = 0;
    var pos_b = 0;
    var ret = new Array();
    var bitem, aitem;

    /* iterate through both arrays and add the larger item to the return
     * list. remove dupes, too. Use toLowerCase to provide
     * case-insensitivity. */
    while ((pos_a < a.length) && (pos_b < b.length)) {
        if (b_is_select) {
            bitem = b[pos_b].value;
        } else {
            bitem = b[pos_b];
        }
        aitem = a[pos_a];

        /* smaller item in list a */
        if (aitem.toLowerCase() < bitem.toLowerCase()) {
            ret[ret.length] = aitem;
            pos_a++;
        } else {
            /* smaller item in list b */
            if (aitem.toLowerCase() > bitem.toLowerCase()) {
                ret[ret.length] = bitem;
                pos_b++;
            } else {
                /* list contents are equal, inc both counters. */
                ret[ret.length] = aitem;
                pos_a++;
                pos_b++;
            }
        }
    }

    /* catch leftovers here. these sections are ugly code-copying. */
    if (pos_a < a.length) {
        for (; pos_a < a.length ; pos_a++) {
            ret[ret.length] = a[pos_a];
        }
    }

    if (pos_b < b.length) {
        for (; pos_b < b.length; pos_b++) {
            if (b_is_select) {
                bitem = b[pos_b].value;
            } else {
                bitem = b[pos_b];
            }
            ret[ret.length] = bitem;
        }
    }
    return ret;
}

/* Returns an array of indexes or values from a select form control.
 *    - control: select control from which to find selections
 *    - findall: boolean, store all options when true or just the selected
 *      indexes
 *    - want_values: boolean; we store values when true and indexes when
 *      false */
function get_selection(control, findall, want_values) {
    var ret = new Array();

    if ((!findall) && (control.selectedIndex == -1)) {
        return ret;
    }

    for (var i=0; i<control.length; i++) {
        if (findall || control.options[i].selected) {
            ret[ret.length] = want_values ? control.options[i].value : i;
        }
    }
    return ret;
}

