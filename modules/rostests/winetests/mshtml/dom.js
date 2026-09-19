/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

sync_test("url", function() {
    ok(document.URL === "http://winetest.example.org/index.html?dom.js", "document.URL = " + document.URL);
    ok(!("documentURI" in document), "documentURI in document");
});

sync_test("input_selection", function() {
    var input = document.createElement("input");
    input.type = "text";
    input.value = "test";
    document.body.appendChild(input);

    function test_range(start, end) {
        ok(input.selectionStart === start, "input.selectionStart = " + input.selectionStart + " expected " + start);
        ok(input.selectionEnd === end, "input.selectionEnd = " + input.selectionEnd + " expected " + end);
    }

    test_range(0, 0);

    input.selectionStart = 2;
    test_range(2, 2);

    input.selectionStart = -1;
    test_range(0, 2);

    input.selectionStart = 10;
    test_range(4, 4);

    input.selectionEnd = 2;
    test_range(2, 2);

    input.selectionEnd = -1;
    test_range(0, 0);

    input.selectionEnd = 10;
    test_range(0, 4);

    input.setSelectionRange(2, 3);
    test_range(2, 3);

    input.setSelectionRange(-1, 10);
    test_range(0, 4);

    input.setSelectionRange(3, 3);
    test_range(3, 3);
});

sync_test("textContent", function() {
    var text = document.createTextNode("test");
    ok(text.textContent === "test", "text.textContent = " + text.textContent);

    var div = document.createElement("div");
    document.body.appendChild(div);
    div.innerHTML = "abc<script>/* */</script><div>text</div>";
    ok(div.textContent === "abc/* */text", "div.textContent = " + div.textContent);

    div.textContent = "test";
    ok(div.textContent === "test", "div.textContent = " + div.textContent);
    ok(div.childNodes.length === 1, "div.childNodes.length = " + div.childNodes.length);
    ok(div.firstChild.textContent === "test", "div.firstChild.textContent = " + div.firstChild.textContent);

    div.textContent = "";
    ok(div.textContent === "", "div.textContent = " + div.textContent);
    ok(div.childNodes.length === 0, "div.childNodes.length = " + div.childNodes.length);

    div.textContent = null;
    ok(div.textContent === "", "div.textContent = " + div.textContent);
    div.textContent = 11;
    ok(div.textContent === "11", "div.textContent = " + div.textContent);
    div.textContent = 10.5;
    ok(div.textContent === "10.5", "div.textContent = " + div.textContent);

    ok(document.textContent === null, "document.textContent = " + document.textContent);
});

sync_test("ElementTraversal", function() {
    var div = document.createElement("div");
    div.innerHTML = "abc<b>bold</b><script>/* */</script><div>text</div>def";
    ok(div.childElementCount === 3, "div.childElementCount = " + div.childElementCount);
    ok(div.firstElementChild.outerHTML === "<b>bold</b>",
            "div.firstElementChild.outerHTML = " + div.firstElementChild.outerHTML);
    ok(div.lastElementChild.outerHTML === "<div>text</div>",
            "div.lastElementChild.outerHTML = " + div.lastElementChild.outerHTML);
    ok(div.firstElementChild.nextElementSibling.outerHTML === "<script>/* */</script>",
            "div.firstElementChild.nextElementSibling.outerHTML = " + div.firstElementChild.nextElementSibling.outerHTML);
    ok(div.lastElementChild.nextElementSibling === null,
            "div.lastElementChild.nextElementSibling = " + div.lastElementChild.nextElementSibling);
    ok(div.lastElementChild.previousElementSibling.outerHTML === "<script>/* */</script>",
            "div.lastElementChild.previousElementSibling.outerHTML = " + div.lastElementChild.previousElementSibling.outerHTML);
    ok(div.firstElementChild.previousElementSibling === null,
            "div.firstElementChild.previousElementSibling = " + div.firstElementChild.previousElementSibling);

    div.innerHTML = "abc";
    ok(div.childElementCount === 0, "div.childElementCount = " + div.childElementCount);
    ok(div.firstElementChild === null, "div.firstElementChild = " + div.firstElementChild);
    ok(div.lastElementChild === null, "div.lastElementChild = " + div.lastElementChild);

    ok(!("childElementCount" in document), "childElementCount found in document");
    ok(!("firstElementChild" in document), "firstElementChild found in document");
    ok(!("nextElementSibling" in document), "nextElementSibling found in document");
});

sync_test("head", function() {
    var h = document.head;
    ok(h.tagName === "HEAD", "h.tagName = " + h.tagName);
    ok(h === document.getElementsByTagName("head")[0], "unexpected head element");
});

async_test("iframe", function() {
    document.body.innerHTML = '<iframe src="runscript.html?frame.js"></iframe>'
    var iframe = document.body.firstChild;

    iframe.onload = guard(function() {
        var r = iframe.contentWindow.global_object.get_global_value();
        ok(r === "global value", "get_global_value() returned " + r);

        var f = iframe.contentWindow.global_object.get_global_value;
        ok(f() === "global value", "f() returned " + f());

        next_test();
    });
});

async_test("iframe_location", function() {
    document.body.innerHTML = '<iframe src="emptyfile"></iframe>'
    var iframe = document.body.firstChild;

    iframe.onload = function() {
        ok(iframe.contentWindow.location.pathname === "/emptyfile",
           "path = " + iframe.contentWindow.location.pathname);
        iframe.onload = function () {
            ok(iframe.contentWindow.location.pathname === "/empty/file",
               "path = " + iframe.contentWindow.location.pathname);
            next_test();
        }
        iframe.src = "empty/file";
    }
});

sync_test("anchor", function() {
    var anchor_tests = [
        { href: "http://www.winehq.org:123/about",
          protocol: "http:", host: "www.winehq.org:123", path: "/about" },
        { href: "https://www.winehq.org:123/about",
          protocol: "https:", host: "www.winehq.org:123", path: "/about" },
        { href: "about:blank",
          protocol: "about:", host: "", path: "/blank", todo_pathname: 1 },
        { href: "unknown:path",
          protocol: "unknown:", host: "", path: "path" },
        { href: "ftp:path",
          protocol: "ftp:", host: "", path: "path" },
        { href: "mailto:path",
          protocol: "mailto:", host: "", path: "path" },
        { href: "ws:path",
          protocol: "ws:", host: "", path: "path" },
        { href: "file:path",
          protocol: "file:", host: "", path: "/path", todo_pathname: 1 },
        { href: "file:///c:/dir/file.html",
          protocol: "file:", host: "", path: "/c:/dir/file.html" },
        { href: "http://www.winehq.org/about",
          protocol: "http:", host: "www.winehq.org", path: "/about" },
        { href: "https://www.winehq.org/about",
          protocol: "https:", host: "www.winehq.org", path: "/about" },
    ];

    for(var i in anchor_tests) {
        var t = anchor_tests[i];
        document.body.innerHTML = '<a href="' + t.href + '">';
        var anchor = document.body.firstChild;
        ok(anchor.protocol === t.protocol, "anchor(" + t.href + ").protocol = " + anchor.protocol);
        ok(anchor.host === t.host, "anchor(" + t.href + ").host = " + anchor.host);
        todo_wine_if("todo_pathname" in t).
        ok(anchor.pathname === t.path, "anchor(" + t.href + ").pathname = " + anchor.pathname);
    }
});

sync_test("getElementsByClassName", function() {
    var elems;

    document.body.innerHTML = '<div class="class1">'
        + '<div class="class1"></div>'
        + '<a id="class1" class="class2"></a>'
        + '</div>'
        + '<script class="class1"></script>';

    elems = document.getElementsByClassName("class1");
    ok(elems.length === 3, "elems.length = " + elems.length);
    ok(elems[0].tagName === "DIV", "elems[0].tagName = " + elems[0].tagName);
    ok(elems[1].tagName === "DIV", "elems[1].tagName = " + elems[1].tagName);
    ok(elems[2].tagName === "SCRIPT", "elems[2].tagName = " + elems[2].tagName);

    elems = document.getElementsByClassName("class2");
    ok(elems.length === 1, "elems.length = " + elems.length);
    ok(elems[0].tagName === "A", "elems[0].tagName = " + elems[0].tagName);

    elems = document.getElementsByClassName("classnotfound");
    ok(elems.length == 0, "elems.length = " + elems.length);
});

sync_test("createElementNS", function() {
    var svg_ns = "http://www.w3.org/2000/svg";
    var elem;

    elem = document.createElementNS(null, "test");
    ok(elem.tagName === "test", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === null, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS(svg_ns, "test");
    ok(elem.tagName === "test", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === svg_ns, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS(svg_ns, "svg");
    ok(elem.tagName === "svg", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === svg_ns, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS("test", "svg");
    ok(elem.tagName === "svg", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === "test", "elem.namespaceURI = " + elem.namespaceURI);
});

sync_test("query_selector", function() {
    document.body.innerHTML = '<div class="class1">'
        + '<div class="class1"></div>'
        + '<a id="class1" class="class2"></a>'
        + '</div>'
        + '<script class="class1"></script>';

    var frag = document.createDocumentFragment()
    var e = document.createElement("div");
    e.innerHTML = '<div class="class3"></div><a id="class3" class="class4"></a></div>';
    frag.appendChild(e);
    var e = document.createElement("script");
    e.className = "class3";
    frag.appendChild(e);

    e = document.querySelector("nomatch");
    ok(e === null, "e = " + e);
    e = document.body.querySelector("nomatch");
    ok(e === null, "e = " + e);
    e = frag.querySelector("nomatch");
    ok(e === null, "e = " + e);

    e = document.querySelector(".class1");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);
    e = document.body.querySelector(".class1");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);
    ok(e.msMatchesSelector(".class1") === true, "msMatchesSelector returned " + e.msMatchesSelector(".class1"));
    ok(e.msMatchesSelector(".class2") === false, "msMatchesSelector returned " + e.msMatchesSelector(".class2"));
    e = document.querySelector(".class3");
    ok(e === null, "e = " + e);
    e = document.body.querySelector(".class3");
    ok(e === null, "e = " + e);

    e = frag.querySelector(".class3");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);
    e = frag.querySelector(".class4");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);
    e = frag.querySelector(".class1");
    ok(e === null, "e = " + e);
    e = frag.querySelector(".class2");
    ok(e === null, "e = " + e);

    e = document.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);
    e = document.body.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);
    e = frag.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);

    e = document.querySelectorAll(".class1");
    ok(e.length === 3, "e.length = " + e.length);
    e = document.body.querySelectorAll(".class1");
    ok(e.length === 3, "e.length = " + e.length);
    e = document.querySelectorAll(".class2");
    ok(e.length === 1, "e.length = " + e.length);
    e = document.body.querySelectorAll(".class2");
    ok(e.length === 1, "e.length = " + e.length);
    e = frag.querySelectorAll(".class3");
    ok(e.length === 2, "e.length = " + e.length);
    e = frag.querySelectorAll(".class4");
    ok(e.length === 1, "e.length = " + e.length);
});

sync_test("compare_position", function() {
    document.body.innerHTML = '<div><div></div><div></div></div>';

    var parent = document.body.firstChild;
    var child1 = parent.firstChild;
    var child2 = child1.nextSibling;
    var elem = document.createElement("div");

    function compare_position(node1, node2, expected_result, ignore_mask) {
        var cmp = node1.compareDocumentPosition(node2);
        ok((cmp & ~ignore_mask) == expected_result,
           "compareDocumentPosition returned " + cmp + " expected " + expected_result);
    }

    compare_position(child1, child2, 4);
    compare_position(child2, child1, 2);
    compare_position(parent, child1, 0x14);
    compare_position(parent, child2, 0x14);
    compare_position(parent, elem, 0x21, 6);
    compare_position(elem, parent, 0x21, 6);
});

sync_test("rects", function() {
    document.body.innerHTML = '<div>test</div>';
    var elem = document.body.firstChild;
    var rects = elem.getClientRects();
    var rect = elem.getBoundingClientRect();

    ok(rects.length === 1, "rect.length = " + rects.length);
    ok(rects[0].top === rect.top, "rects[0].top = " + rects[0].top + " rect.top = " + rect.top);
    ok(rects[0].bottom === rect.bottom, "rects[0].bottom = " + rects[0].bottom + " rect.bottom = " + rect.bottom);

    ok("" + rects[0] === "[object ClientRect]", "rects[0] = " + rects[0]);
    ok(rects.hasOwnProperty("0"), 'rects.hasOwnProperty("0") = ' + rects.hasOwnProperty("0"));
    todo_wine.
    ok(rects.hasOwnProperty("1"), 'rects.hasOwnProperty("1") = ' + rects.hasOwnProperty("1"));
    var desc = Object.getOwnPropertyDescriptor(rects, "0");
    ok(desc.writable === true, "writable = " + desc.writable);
    todo_wine.
    ok(desc.enumerable === true, "enumerable = " + desc.enumerable);
    ok(desc.configurable === true, "configurable = " + desc.configurable);
    ok("" + desc.value === "[object ClientRect]", "desc.value = " + desc.value);

    ok(rect.height === rect.bottom - rect.top, "rect.height = " + rect.height + " rect.bottom = " + rect.bottom + " rect.top = " + rect.top);
    ok(rect.width === rect.right - rect.left, "rect.width = " + rect.width + " rect.right = " + rect.right + " rect.left = " + rect.left);

    elem = document.createElement("style");
    rects = elem.getClientRects();
    ok(rects.length === 0, "rect.length = " + rects.length);
});

sync_test("document_lastModified", function() {
    // actually it seems to be rounded up from about ~250ms above a sec, but be more conservative with the check
    var diff = Date.parse(document.lastModified) - performance.timing.navigationStart;
    ok(diff > -1000 && diff < 1000, "lastModified too far from navigationStart: " + diff);
});

sync_test("document_owner", function() {
    var node;

    ok(document.ownerDocument === null, "ownerDocument = " + document.ownerDocument);
    ok(document.body.ownerDocument === document,
       "body.ownerDocument = " + document.body.ownerDocument);
    ok(document.documentElement.ownerDocument === document,
       "documentElement.ownerDocument = " + document.documentElement.ownerDocument);

    node = document.createElement("test");
    ok(node.ownerDocument === document, "element.ownerDocument = " + node.ownerDocument);

    node = document.createDocumentFragment();
    ok(node.ownerDocument === document, "fragment.ownerDocument = " + node.ownerDocument);

    node = document.createTextNode("test");
    ok(node.ownerDocument === document, "text.ownerDocument = " + node.ownerDocument);
});

sync_test("style_properties", function() {
    document.body.innerHTML = '<div>test</div><svg></svg>';
    var elem = document.body.firstChild;
    var style = elem.style;
    var current_style = elem.currentStyle;
    var computed_style = window.getComputedStyle(elem);
    var val;

    style.cssFloat = "left";
    ok(style.cssFloat === "left", "cssFloat = " + style.cssFloat);
    ok(style.getPropertyValue("float") === "left",
       'style.getPropertyValue("float") = ' + style.getPropertyValue("float"));
    ok(style.getPropertyValue("cssFloat") === "",
       'style.getPropertyValue("cssFloat") = ' + style.getPropertyValue("cssFloat"));

    val = style.removeProperty("float");
    ok(val === "left", "removeProperty() returned " + val);
    ok(style.cssFloat === "", "cssFloat = " + style.cssFloat);

    style.cssFloat = "left";
    val = style.removeProperty("FloaT");
    ok(val === "left", "removeProperty() returned " + val);
    ok(style.cssFloat === "", "cssFloat = " + style.cssFloat);

    style.cssFloat = "left";
    val = style.removeProperty("cssFloat");
    ok(val === "", "removeProperty() returned " + val);
    ok(style.cssFloat === "left", "cssFloat = " + style.cssFloat);
    ok(style["float"] === "left", "float = " + style["float"]);

    style.testVal = "test";
    val = style.removeProperty("testVal");
    ok(val === "", "removeProperty() returned " + val);
    ok(style.testVal === "test", "testVal = " + style.testVal);

    val = style.getPropertyValue("testVal");
    ok(val === "", 'style.getPropertyValue("testVal") = ' + val);
    ok(style.testVal === "test", "testVal = " + style.testVal);

    style.setProperty("testVal", "1px");
    val = style.getPropertyValue("testVal");
    ok(val === "", 'style.getPropertyValue("testVal") = ' + val);
    ok(style.testVal === "test", "testVal = " + style.testVal);

    style.setProperty("test", "1px");
    val = style.getPropertyValue("test");
    ok(val === "", 'style.getPropertyValue("test") = ' + val);
    ok(!("test" in style), "test in style");

    style["z-index"] = 1;
    ok(style.zIndex === 1, "zIndex = " + style.zIndex);
    ok(style["z-index"] === 1, "z-index = " + style["z-index"]);
    ok(style.getPropertyValue("z-index") === "1",
       'style.getPropertyValue("x-index") = ' + style.getPropertyValue("z-index"));
    ok(style.getPropertyValue("zIndex") === "",
       'style.getPropertyValue("xIndex") = ' + style.getPropertyValue("zIndex"));

    style.setProperty("border-width", "5px");
    ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);

    try {
        style.setProperty("border-width", 6);
        ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);
    }catch(e) {
        win_skip("skipping setProperty tests on too old IE version");
        return;
    }

    style.setProperty("border-width", "7px", "test");
    ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "6px", "");
    ok(style.borderWidth === "6px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "7px", "important");
    ok(style.borderWidth === "7px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "8px", undefined);
    ok(style.borderWidth === "7px", "style.borderWidth = " + style.borderWidth);

    style.clip = "rect(1px 1px 10px 10px)";
    ok(style.clip === "rect(1px, 1px, 10px, 10px)", "style.clip = " + style.clip);
    ok(current_style.clip === "rect(1px, 1px, 10px, 10px)",
       "current_style.clip = " + current_style.clip);
    ok(computed_style.clip === "rect(1px, 1px, 10px, 10px)",
       "computed_style.clip = " + current_style.clip);

    style.zIndex = 2;
    ok(current_style.zIndex === 2, "current_style.zIndex = " + current_style.zIndex);
    ok(computed_style.zIndex === 2, "computed_style.zIndex = " + computed_style.zIndex);

    try {
        current_style.zIndex = 1;
        ok(false, "expected exception");
    }catch(e) {
        todo_wine.
        ok(e.name === "NoModificationAllowedError", "setting current_style.zIndex threw " + e.name);
    }

    try {
        computed_style.zIndex = 1;
        ok(false, "expected exception");
    }catch(e) {
        todo_wine.
        ok(e.name === "NoModificationAllowedError", "setting computed_style.zIndex threw " + e.name);
    }

    /* prop not found in any IHTMLCurrentStyle* interfaces, but exposed from common CSSStyleDeclarationPrototype */
    try {
        current_style.perspective = 1;
        ok(false, "expected exception");
    }catch(e) {
        todo_wine.
        ok(e.name === "NoModificationAllowedError", "setting current_style.perspective threw " + e.name);
    }

    try {
        computed_style.perspective = 1;
        ok(false, "expected exception");
    }catch(e) {
        todo_wine.
        ok(e.name === "NoModificationAllowedError", "setting computed_style.perspective threw " + e.name);
    }

    elem = elem.nextSibling;
    computed_style = window.getComputedStyle(elem);

    elem.style.zIndex = 4;
    ok(computed_style.zIndex === 4, "computed_style.zIndex = " + computed_style.zIndex);

    window.getComputedStyle(elem, null);

    /* ms* prefixed styles alias */
    var list = [
        [ "transform", "translate(5px, 5px)" ],
        [ "transition", "background-color 0.5s linear 0.1s" ]
    ];
    for(var i = 0; i < list.length; i++) {
        var s = list[i][0], v = list[i][1], ms = "ms" + s[0].toUpperCase() + s.substring(1);
        style[s] = v;
        ok(style[s] === v, "style." + s + " = " + style[s] + ", expected " + v);
        ok(style[ms] === v, "style." + ms + " = " + style[ms] + ", expected " + v);
        elem.style[ms] = v;
        ok(elem.style[s] === v, "elem.style." + s + " = " + elem.style[s] + ", expected " + v);
        ok(elem.style[ms] === v, "elem.style." + ms + " = " + elem.style[ms] + ", expected " + v);
    }
});

sync_test("stylesheets", function() {
    document.body.innerHTML = '<style>.div { margin-right: 1px; }</style>';
    var elem = document.body.firstChild;

    ok(document.styleSheets.length === 1, "document.styleSheets.length = " + document.styleSheets.length);

    var stylesheet = document.styleSheets.item(0);
    ok(stylesheet.rules.length === 1, "stylesheet.rules.length = " + stylesheet.rules.length);
    ok(typeof(stylesheet.rules.item(0)) === "object",
       "typeof(stylesheet.rules.item(0)) = " + typeof(stylesheet.rules.item(0)));

    stylesheet = document.styleSheets[0];
    ok(stylesheet.rules.length === 1, "document.styleSheets[0].rules.length = " + stylesheet.rules.length);

    try {
        stylesheet.rules.item(1);
        ok(false, "expected exception");
    }catch(e) {}

    ok(stylesheet.href === null, "stylesheet.href = " + stylesheet.href);

    var id = stylesheet.insertRule(".input { margin-left: 1px; }", 0);
    ok(id === 0, "id = " + id);
    ok(document.styleSheets.length === 1, "document.styleSheets.length = " + document.styleSheets.length);

    try {
        stylesheet.insertRule(".input { margin-left: 1px; }", 3);
        ok(false, "expected exception");
    }catch(e) {}

    id = stylesheet.addRule(".p", "margin-top: 2px");
    ok(id === 2, "id = " + id);
    ok(document.styleSheets.length === 1, "document.styleSheets.length = " + document.styleSheets.length);
    ok(stylesheet.rules.length === 3, "stylesheet.rules.length = " + stylesheet.rules.length);

    id = stylesheet.addRule(".pre", "border: none", -1);
    ok(id === 3, "id = " + id);
    ok(stylesheet.rules.length === 4, "stylesheet.rules.length = " + stylesheet.rules.length);

    id = stylesheet.addRule(".h1", " ", 0);
    ok(id === 0, "id = " + id);
    ok(stylesheet.rules.length === 5, "stylesheet.rules.length = " + stylesheet.rules.length);

    id = stylesheet.addRule(".h2", "color: black", 8);
    ok(id === 5, "id = " + id);
    ok(stylesheet.rules.length === 6, "stylesheet.rules.length = " + stylesheet.rules.length);

    try {
        stylesheet.addRule("", "border: none", 0);
        ok(false, "expected exception");
    }catch(e) {}
    try {
        stylesheet.addRule(".img", "", 0);
        ok(false, "expected exception");
    }catch(e) {}
});

sync_test("storage", function() {
    ok(typeof(window.sessionStorage) === "object",
       "typeof(window.sessionStorage) = " + typeof(window.sessionStorage));
    ok(typeof(window.localStorage) === "object" || typeof(window.localStorage) === "unknown",
       "typeof(window.localStorage) = " + typeof(window.localStorage));

    var item = sessionStorage.getItem("nonexisting");
    ok(item === null, "'nonexisting' item = " + item);
    item = sessionStorage["nonexisting"];
    ok(item === undefined, "[nonexisting] item = " + item);
    ok(!("nonexisting" in sessionStorage), "nonexisting in sessionStorage");

    sessionStorage.setItem("foobar", 42);
    ok("foobar" in sessionStorage, "foobar not in sessionStorage");
    ok(sessionStorage.hasOwnProperty("foobar"), "foobar not prop of sessionStorage");
    item = sessionStorage.getItem("foobar");
    ok(item === "42", "'foobar' item = " + item);
    item = sessionStorage["foobar"];
    ok(item === "42", "[foobar] item = " + item);
    sessionStorage.removeItem("foobar");
    item = sessionStorage["foobar"];
    ok(item === undefined, "[foobar] item after removal = " + item);

    sessionStorage["barfoo"] = true;
    ok("barfoo" in sessionStorage, "barfoo not in sessionStorage");
    ok(sessionStorage.hasOwnProperty("barfoo"), "barfoo not prop of sessionStorage");
    item = sessionStorage["barfoo"];
    ok(item === "true", "[barfoo] item = " + item);
    item = sessionStorage.getItem("barfoo");
    ok(item === "true", "'barfoo' item = " + item);
});

async_test("animation", function() {
    document.body.innerHTML =
        "<style>" +
        "  @keyframes testAnimation {0% { opacity: 0; } 100% { opacity: 1; }}" +
        "  .testAnimation { animation-name: testAnimation; animation-duration: 0.01s; }" +
        "</style>";
    var div = document.createElement("div");
    div.addEventListener("animationstart", function() {
        div.addEventListener("animationend", next_test);
    });
    document.body.appendChild(div);
    div.className = "testAnimation";
});

sync_test("navigator", function() {
    ok(typeof(window.navigator) === "object",
       "typeof(window.navigator) = " + typeof(window.navigator));

    var v = window.navigator;
    ok(v === window.navigator, "v != window.navigator");
    v.testProp = true;
    ok(window.navigator.testProp, "window.navigator.testProp = " + window.navigator.testProp);
});

sync_test("elem_props", function() {
    var elem = document.body;

    ok(elem.accessKey === "", "accessKey = " + elem.accessKey);
    elem.accessKey = "q";
    ok(elem.accessKey === "q", "accessKey = " + elem.accessKey + " expected q");
});

async_test("animation_frame", function() {
    var id = requestAnimationFrame(function(x) { ok(false, "request was supposed to be cancelled"); });
    id = cancelAnimationFrame(id);
    ok(id === undefined, "cancelAnimationFrame returned " + id);

    id = requestAnimationFrame(function(x) {
        ok(this === window, "this != window");
        ok(typeof(x) === "number", "x = " + x);
        ok(arguments.length === 1, "arguments.length = " + arguments.length);
        next_test();
    });
    cancelAnimationFrame(0);
    clearInterval(id);
    clearTimeout(id);
    ok(typeof(id) === "number", "id = " + id);
    ok(id !== 0, "id = 0");
});

sync_test("title", function() {
    var elem = document.createElement("div");
    ok(elem.title === "", "div.title = " + elem.title);
    ok(elem.getAttribute("title") === null, "title attribute = " + elem.getAttribute("title"));
    elem.title = "test";
    ok(elem.title === "test", "div.title = " + elem.title);
    ok(elem.getAttribute("title") === "test", "title attribute = " + elem.getAttribute("title"));

    var orig = document.title;
    document.title = "w i n e test";
    var title = document.getElementsByTagName("title")[0];
    ok(title.text === "w i n e test", "<title> element text = " + title.text);
    title.text = "winetest";
    ok(title.text === "winetest", "<title> element text after change = " + title.text);
    ok(document.title === "winetest", "document.title after <title> change = " + document.title);

    elem = document.createElement("title");
    ok(elem.text === "", "detached <title> element text = " + elem.text);
    elem.text = "foobar";
    ok(elem.text === "foobar", "detached <title> element text after change = " + elem.text);
    ok(document.title === "winetest", "document.title after detached <title> change = " + document.title);

    title.parentNode.replaceChild(elem, title);
    ok(document.title === "foobar", "document.title after <title> replaced = " + document.title);
    document.title = orig;
});

sync_test("disabled", function() {
    var elem = document.createElement("div");
    document.body.appendChild(elem);
    ok(elem.disabled === false, "div.disabled = " + elem.disabled);
    ok(elem.getAttribute("disabled") === null, "disabled attribute = " + elem.getAttribute("disabled") + " expected null");

    elem.disabled = true;
    ok(elem.disabled === true, "div.disabled = " + elem.disabled);
    ok(elem.getAttribute("disabled") === "", "disabled attribute = " + elem.getAttribute("disabled") + " expected \"\"");

    elem.disabled = false;
    ok(elem.disabled === false, "div.disabled = " + elem.disabled);
    ok(elem.getAttribute("disabled") === null, "disabled attribute = " + elem.getAttribute("disabled") + " expected null");

    elem.setAttribute("disabled", "false");
    ok(elem.disabled === true, "div.disabled = " + elem.disabled);
    ok(elem.getAttribute("disabled") === "false", "disabled attribute = " + elem.getAttribute("disabled"));

    elem.removeAttribute("disabled");
    ok(elem.disabled === false, "div.disabled = " + elem.disabled);
    ok(elem.getAttribute("disabled") === null, "disabled attribute = " + elem.getAttribute("disabled") + " expected null");
});

sync_test("hasAttribute", function() {
    document.body.innerHTML = '<div attr="test"></div>';
    var elem = document.body.firstChild, r;

    r = elem.hasAttribute("attr");
    ok(r === true, "hasAttribute(attr) returned " + r);
    r = elem.hasAttribute("attr2");
    ok(r === false, "hasAttribute(attr2) returned " + r);

    elem.setAttribute("attr2", "abc");
    r = elem.hasAttribute("attr2");
    ok(r === true, "hasAttribute(attr2) returned " + r);

    elem.removeAttribute("attr");
    r = elem.hasAttribute("attr");
    ok(r === false, "hasAttribute(attr) returned " + r);
});

sync_test("classList", function() {
    var elem = document.createElement("div");
    var classList = elem.classList, i, r;

    var props = [ "add", "contains", "item", "length", "remove", "toggle" ];
    for(i = 0; i < props.length; i++)
        ok(props[i] in classList, props[i] + " not found in classList.");

    props = [ "entries", "forEach", "keys", "replace", "supports", "value", "values"];
    for(i = 0; i < props.length; i++)
        ok(!(props[i] in classList), props[i] + " found in classList.");

    classList.add("a");
    ok(elem.className === "a", "Expected className 'a', got " + elem.className);
    ok(classList.length === 1, "Expected length 1 for className 'a', got " + classList.length);

    classList.add("b");
    ok(elem.className === "a b", "Expected className 'a b', got " + elem.className);
    ok(classList.length === 2, "Expected length 2 for className 'a b', got " + classList.length);

    classList.add("c");
    ok(elem.className === "a b c", "Expected className 'a b c', got " + elem.className);
    ok(classList.length === 3, "Expected length 3 for className 'a b c', got " + classList.length);

    classList.add(4);
    ok(elem.className === "a b c 4", "Expected className 'a b c 4', got " + elem.className);
    ok(classList.length === 4, "Expected length 4 for className 'a b c 4', got " + classList.length);

    classList.add("c");
    ok(elem.className === "a b c 4", "(2) Expected className 'a b c 4', got " + elem.className);
    ok(("" + classList) === "a b c 4", "Expected classList value 'a b c 4', got " + classList);
    ok(classList.toString() === "a b c 4", "Expected classList toString 'a b c 4', got " + classList.toString());

    var exception = false

    try
    {
        classList.add();
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception && elem.className === "a b c 4", "Expected exception, className 'a b c 4', got exception "
            + exception + ", className" + elem.className);

    exception = false
    try
    {
        classList.add("");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.add(\"\")");

    exception = false
    try
    {
        classList.add("e f");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.add(\"e f\")");

    exception = false;
    try
    {
        classList.contains();
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.contains()");

    exception = false;
    try
    {
        classList.contains("");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.contains(\"\")");

    exception = false;
    try
    {
        classList.contains("a b");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.contains(\"a b\")");

    ok(classList.contains("4") === true, "contains: expected '4' to return true");
    ok(classList.contains("b") === true, "contains: expected 'b' to return true");
    ok(classList.contains("d") === false, "contains: expected 'd' to return false");

    r = classList.item(-1);
    ok(r === null, "item(-1) = " + r);
    r = classList.item(0);
    ok(r === "a", "item(0) = " + r);
    r = classList.item(1);
    ok(r === "b", "item(1) = " + r);
    r = classList.item(2);
    ok(r === "c", "item(2) = " + r);
    r = classList.item(3);
    ok(r === "4", "item(3) = " + r);
    r = classList.item(4);
    ok(r === null, "item(4) = " + r);

    classList.remove("e");
    ok(elem.className === "a b c 4", "remove: expected className 'a b c 4', got " + elem.className);

    exception = false
    try
    {
        classList.remove("e f");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "remove: expected exception for classList.remove(\"e f\")");

    exception = false
    try
    {
        classList.remove("");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "remove: expected exception for classList.remove(\"\")");

    classList.remove("d");
    ok(elem.className === "a b c 4", "remove: expected className 'a b c 4', got " + elem.className);

    classList.remove("c");
    ok(elem.className === "a b 4", "remove: expected className 'a b 4', got " + elem.className);

    classList.remove(4);
    ok(elem.className === "a b", "remove: expected className 'a b', got " + elem.className);

    classList.remove('a');
    ok(elem.className === "b", "remove: expected className 'b', got " + elem.className);

    classList.remove("a");
    ok(elem.className === "b", "remove (2): expected className 'b', got " + elem.className);

    classList.remove("b");
    ok(elem.className === "", "remove: expected className '', got " + elem.className);

    exception = false;
    try
    {
        classList.toggle();
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.toggle()");

    exception = false;
    try
    {
        classList.toggle("");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.toggle(\"\")");

    exception = false;
    try
    {
        classList.toggle("a b");
    }
    catch(e)
    {
        exception = true;
    }
    ok(exception, "Expected exception for classList.toggle(\"a b\")");

    // toggle's second arg is not implemented by IE, and ignored
    r = classList.toggle("abc");
    ok(r === true, "toggle('abc') returned " + r);
    ok(elem.className === "abc", "toggle('abc'): got className " + elem.className);

    r = classList.toggle("def", false);
    ok(r === true, "toggle('def', false) returned " + r);
    ok(elem.className === "abc def", "toggle('def', false): got className " + elem.className);

    r = classList.toggle("123", 1234);
    ok(r === true, "toggle('123', 1234) returned " + r);
    ok(elem.className === "abc def 123", "toggle('123', 1234): got className " + elem.className);

    r = classList.toggle("def", true);
    ok(r === false, "toggle('def', true) returned " + r);
    ok(elem.className === "abc 123", "toggle('def', true): got className " + elem.className);

    r = classList.toggle("123", null);
    ok(r === false, "toggle('123', null) returned " + r);
    ok(elem.className === "abc", "toggle('123', null): got className " + elem.className);

    elem.className = "  testclass    foobar  ";
    ok(classList.length === 2, "Expected length 2 for className '  testclass    foobar  ', got " + classList.length);
    ok(("" + classList) === "  testclass    foobar  ", "Expected classList value '  testclass    foobar  ', got " + classList);
    ok(classList.toString() === "  testclass    foobar  ", "Expected classList toString '  testclass    foobar  ', got " + classList.toString());

    r = classList[-1];
    ok(r === null, "classList[-1] = " + r);
    r = classList[0];
    ok(r === "testclass", "classList[0] = " + r);
    r = classList[1];
    ok(r === "foobar", "classList[1] = " + r);
    r = classList[2];
    ok(r === null, "classList[2] = " + r);

    classList[0] = "barfoo";
    classList[2] = "added";
    ok(classList.toString() === "  testclass    foobar  ", "Expected classList toString to not be changed after setting indexed props, got " + classList.toString());

    try
    {
        classList[0]();
        ok(false, "Expected exception calling classList[0]");
    }
    catch(e)
    {
        ok(e.number === 0xa138a - 0x80000000, "Calling classList[0] threw " + e.number);
    }

    try
    {
        new classList[0]();
        ok(false, "Expected exception calling classList[0] as constructor");
    }
    catch(e)
    {
        ok(e.number === 0xa01bd - 0x80000000, "Calling classList[0] as constructor threw " + e.number);
    }
});

sync_test("importNode", function() {
    var node, node2, orig_node, doc = document.implementation.createHTMLDocument("TestDoc");
    doc.body.innerHTML = '<div id="test"><span/></div>';
    orig_node = doc.getElementById("test");

    node = document.importNode(orig_node, false);
    ok(node !== orig_node, "node = orig_node");
    ok(orig_node.hasChildNodes() === true, "orig_node does not have child nodes");
    ok(orig_node.parentNode === doc.body, "orig_node.parentNode = " + orig_node.parentNode);
    ok(node.hasChildNodes() === false, "node has child nodes with shallow import");
    ok(node.parentNode === null, "node.parentNode = " + node.parentNode);

    node = document.importNode(orig_node, true);
    ok(node !== orig_node, "node = orig_node");
    ok(orig_node.hasChildNodes() === true, "orig_node does not have child nodes");
    ok(orig_node.parentNode === doc.body, "orig_node.parentNode = " + orig_node.parentNode);
    ok(node.hasChildNodes() === true, "node does not have child nodes with deep import");
    ok(node.parentNode === null, "node.parentNode = " + node.parentNode);

    node2 = document.importNode(node, false);
    ok(node !== node2, "node = node2");
    ok(node.hasChildNodes() === true, "node does not have child nodes");
    ok(node.parentNode === null, "node.parentNode = " + node.parentNode);
    ok(node2.hasChildNodes() === false, "node2 has child nodes");
    ok(node2.parentNode === null, "node2.parentNode = " + node2.parentNode);
});
