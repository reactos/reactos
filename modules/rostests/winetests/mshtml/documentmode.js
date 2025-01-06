/*
 * Copyright 2016 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

function test_elem_props() {
    var elem = document.documentElement;

    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in elem, prop + " not found in element.");
        else
            ok(!(prop in elem), prop + " found in element.");
    }

    var v = document.documentMode;

    test_exposed("querySelectorAll", v >= 8);

    next_test();
}

function test_doc_mode() {
    var opt = parseInt(document.location.search.substring(1));

    trace("Testing document mode " + opt);

    if(opt > document.documentMode) {
        win_skip("Document mode not supported (expected " + opt + " got " + document.documentMode + ")");
        reportSuccess();
        return;
    }

    ok(opt === document.documentMode, "documentMode = " + document.documentMode);

    if(document.documentMode > 5)
        ok(document.compatMode === "CSS1Compat", "document.compatMode = " + document.compatMode);
    else
        ok(document.compatMode === "BackCompat", "document.compatMode = " + document.compatMode);

    next_test();
}

var tests = [
    test_doc_mode,
    test_elem_props
];
