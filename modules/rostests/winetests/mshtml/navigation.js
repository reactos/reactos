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

function nav_parent_test() {
    external.trace("Running _parent navigation tests...");

    var iframe = document.getElementById("testframe");
    var subframe = iframe.contentWindow.document.createElement("iframe");

    subframe.onload = function() {
        var doc = subframe.contentWindow.document;
        doc.body.innerHTML = '<a href="blank2.html" id="aid" target="_parent">test</a>';
        doc.getElementById("aid").click();
    }

    iframe.onload = function() {
        iframe.onload = null;
        var href = iframe.contentWindow.location.href;
	ok(/.*blank2.html/.test(href), "Unexpected href " + href);
	next_test();
    }

    iframe.contentWindow.document.body.appendChild(subframe);
    subframe.src = "blank.html";
}

function window_navigate_test() {
    external.trace("Running window.navigate() tests...");

    var iframe = document.getElementById("testframe");

    iframe.onload = function() {
        iframe.onload = null;
        var href = iframe.contentWindow.location.href;
	ok(href === "about:blank", "Unexpected href " + href);
	next_test();
    }

    iframe.contentWindow.navigate("about:blank");
}

function window_open_self_test() {
    external.trace("Running window.open(_self) tests...");

    var iframe = document.getElementById("testframe");
    var iframe_window = iframe.contentWindow;

    iframe.onload = function() {
        iframe.onload = null;
        var href = iframe.contentWindow.location.href;
        ok(/.*blank.html\?window_open_self/.test(href), "Unexpected href " + href);
        ok(iframe.contentWindow === iframe_window, "iframe.contentWindow !== iframe_window");
	next_test();
    }

    iframe_window.open("blank.html?window_open_self", "_self");
}

function detached_src_test() {
    var iframe = document.createElement("iframe");
    var onload_called = false;

    iframe.onload = function() {
        onload_called = true;
        next_test();
    }

    iframe.src = "blank.html";
    document.body.appendChild(iframe);
    ok(onload_called === false, "called onload too early?");
}

function init_test_iframe() {
    var iframe = document.createElement("iframe");

    iframe.onload = next_test;
    iframe.id = "testframe";
    iframe.src = "about:blank";
    document.body.appendChild(iframe);
}

var tests = [
    init_test_iframe,
    nav_parent_test,
    window_navigate_test,
    window_open_self_test,
    detached_src_test
];
