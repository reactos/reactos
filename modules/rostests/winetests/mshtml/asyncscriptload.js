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

var tests = [];
var head = document.getElementsByTagName("head")[0];

/* Dynamically created script element is downloaded as soon as src property is set,
 * but it doesn't block document onload event. */
var detached_elem_executed = false;
var detached_elem = document.createElement("script");
detached_elem.src = "jsstream.php?detached_script";

async_test("detached_script_elem", function() {
    var oncomplete_called = false;
    detached_elem.onreadystatechange = guard(function() {
        if(detached_elem.readyState == "complete") {
            ok(detached_elem_executed, "detached element not executed before readyState complete");
            oncomplete_called = true;
            next_test();
            return;
        }

        ok(!detached_elem_executed, "detached element executed");
        if(detached_elem.readyState == "loaded") {
            head.appendChild(detached_elem);
            ok(detached_elem_executed, "detached element not yet executed");
            ok(detached_elem.readyState == "complete", "detached_elem.readyState = " + detached_elem.readyState + " expected complete");
            ok(!oncomplete_called, "oncomplete not called");
        }
    });

    external.writeStream("detached_script", 'detached_elem_executed = true;');
});

/* Dynamically created script elements are evaluated as soon as they are loaded, no matter
 * how they are ordered in the tree. */
var attached_elem1_executed = false;
var attached_elem1 = document.createElement("script");
attached_elem1.src = "jsstream.php?attached_script1";
head.appendChild(attached_elem1);

var attached_elem2_executed = false;
var attached_elem2 = document.createElement("script");
attached_elem2.src = "jsstream.php?attached_script2";
head.appendChild(attached_elem2);

async_test("attached_script_elem", function() {
    attached_elem1.onreadystatechange = guard(function() {
        ok(attached_elem1.readyState == "loaded", "attached_elem1.readyState = " + attached_elem2.readyState);
        ok(attached_elem1_executed, "attached element 1 not executed before readyState complete");
        next_test();
    });

    attached_elem2.onreadystatechange = guard(function() {
        ok(attached_elem2.readyState == "loaded", "attached_elem2.readyState = " + attached_elem2.readyState);
        ok(attached_elem2_executed, "attached element 2 not executed before readyState complete");

        external.writeStream("attached_script1", 'attached_elem1_executed = true;');
    });

    external.writeStream("attached_script2", 'attached_elem2_executed = true;');
});

async_test("append_script", function() {
    var elem = document.createElement("script");
    var ready_states = "";

    elem.onreadystatechange = guard(function() {
        ready_states += elem.readyState + ",";
        if(elem.readyState == "loaded")
            next_test();
    });

    document.body.appendChild(elem);
    elem.src = "jsstream.php?simple";
    external.writeStream("simple", " ");
});

function unexpected_load(e) {
    ok(false, "onload event before executing script");
}

guard(function() {
    var elem = document.createElement("script");
    document.head.appendChild(elem);
    elem.src = "jsstream.php?blockload";

    window.addEventListener("load", unexpected_load, true);

    setTimeout(guard(function() {
        external.writeStream("blockload", "window.removeEventListener('load', unexpected_load, true);");
    }), 100);
})();
