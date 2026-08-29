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

var test_content_loaded = (function() {
    var calls = "";

    function unexpected_call() {
        ok(false, "unexpected call");
    }

    function record_call(msg) {
        return function() { calls += msg + ","; };
    }

    window.addEventListener("DOMContentLoaded", record_call("window.capture"), true);
    window.addEventListener("DOMContentLoaded", record_call("window.bubble"), false);
    document.addEventListener("DOMContentLoaded", record_call("document.capture"), true);
    document.addEventListener("DOMContentLoaded", record_call("document.bubble"), false);

    document.attachEvent("onDOMContentLoaded", unexpected_call);
    document.attachEvent("DOMContentLoaded", unexpected_call);

    return function() {
        ok(calls === "window.capture,document.capture,document.bubble,window.bubble,",
           "calls = " + calls);
        next_test();
    };
})();

async_test("content_loaded", test_content_loaded);

sync_test("listener_order", function() {
    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    var calls;
    function record_call(msg) {
        return function() { calls += msg + ","; };
    }

    window.addEventListener("click", record_call("window.click(capture)"), true);
    window.addEventListener("click", record_call("window.click(bubble)"), false);

    document.attachEvent("onclick", record_call("document.click(attached)"));
    document.addEventListener("click", record_call("document.click(capture)"), true);
    document.addEventListener("click", record_call("document.click(bubble)"), false);
    document.onclick = record_call("document.onclick");

    document.body.onclick = record_call("body.onclick");
    document.body.addEventListener("click", record_call("body.click(capture)"), true);
    document.body.addEventListener("click", record_call("body.click(bubble)"), false);
    document.body.addEventListener("click", record_call("body.click(bubble2)"));
    document.body.attachEvent("onclick", record_call("body.click(attached)"));

    div.attachEvent("onclick", record_call("div.click(attached)"));
    div.addEventListener("click", record_call("div.click(bubble)"), false);
    div.onclick = record_call("div.onclick");
    div.addEventListener("click", record_call("div.click(capture1)"), true);
    div.addEventListener("click", record_call("div.click(capture2)"), true);

    calls = "";
    div.click();
    ok(calls === "window.click(capture),document.click(capture),body.click(capture),"
       + "div.click(attached),div.click(bubble),div.onclick,div.click(capture1),"
       + "div.click(capture2),body.onclick,body.click(bubble),body.click(bubble2),"
       + "body.click(attached),document.click(attached),document.click(bubble),"
       + "document.onclick,window.click(bubble),", "calls = " + calls);

    div.onclick = record_call("new div.onclick");

    calls = "";
    div.click();
    ok(calls === "window.click(capture),document.click(capture),body.click(capture),"
       + "div.click(attached),div.click(bubble),new div.onclick,div.click(capture1),"
       + "div.click(capture2),body.onclick,body.click(bubble),body.click(bubble2),"
       + "body.click(attached),document.click(attached),document.click(bubble),"
       + "document.onclick,window.click(bubble),", "calls = " + calls);

    var e = document.createEvent("Event");
    e.initEvent("click", true, true);

    calls = "";
    div.dispatchEvent(e);
    ok(calls === "window.click(capture),document.click(capture),body.click(capture),"
       + "div.click(bubble),new div.onclick,div.click(capture1),div.click(capture2),"
       + "body.onclick,body.click(bubble),body.click(bubble2),document.click(bubble),"
       + "document.onclick,window.click(bubble),", "calls = " + calls);
});

sync_test("add_listener_in_listener", function() {
    var calls;

    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;

    function record_call(msg) {
        return function() { calls += msg + ","; };
    }

    div1.addEventListener("click", function() {
        div2.addEventListener("click", function() {
            calls += "div2.click";
            /* if we add more listeners here, whey won't be invoked */
            div2.onclick = function() { calls += "click2,"; };
            div2.addEventListener("click", function() { calls += "click3,"; }, false);
            div2.attachEvent("onclick", function() { calls += "click4,"; });
        }, true);
    }, true);

    calls = "";
    div2.click();
    ok(calls === "div2.click", "calls = " + calls);
});

sync_test("remove_listener_in_listener", function() {
    var calls;

    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    function record_call(msg) {
        return function() { calls += msg + ","; };
    }

    var capture = record_call("capture"), bubble = record_call("bubble");

    div.addEventListener("click", function() {
        div.removeEventListener("click", capture, true);
        div.removeEventListener("click", bubble, false);
        calls += "remove,";
    }, true);

    div.addEventListener("click", capture, true);
    div.addEventListener("click", bubble, false);
    div.onclick = record_call("onclick");

    calls = "";
    div.click();
    ok(calls === "remove,capture,bubble,onclick,", "calls = " + calls);
});

sync_test("add_remove_listener", function() {
    var calls;

    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    function listener() {
        calls += "listener,";
    }

    /* if the same listener is added twice, second one won't be called */
    div.addEventListener("click", listener, false);
    div.addEventListener("click", listener, false);

    calls = "";
    div.click();
    ok(calls === "listener,", "calls = " + calls);

    /* remove capture listener, it won't do anything */
    div.removeEventListener("click", listener, true);

    calls = "";
    div.click();
    ok(calls === "listener,", "calls = " + calls);

    /* remove listener once, it won't called anymore */
    div.removeEventListener("click", listener, false);

    calls = "";
    div.click();
    ok(calls === "", "calls = " + calls);

    div.removeEventListener("click", listener, false);

    /* test implicit capture removeEventListener argument */
    div.addEventListener("click", listener, false);
    div.removeEventListener("click", listener);

    calls = "";
    div.click();
    ok(calls === "", "calls = " + calls);

    /* test undefined function argument */
    div.addEventListener("click", undefined, false);

    calls = "";
    div.click();
    ok(calls === "", "calls = " + calls);

    div.addEventListener("click", listener, false);
    div.removeEventListener("click", undefined);

    calls = "";
    div.click();
    ok(calls === "listener,", "calls = " + calls);
    div.removeEventListener("click", listener);
});

sync_test("event_phase", function() {
    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;
    var last_event;

    function check_phase(expected_phase) {
        return function(e) {
            if(last_event)
                ok(last_event === e, "last_event != e");
            else
                last_event = e;
            ok(e.eventPhase === expected_phase,
               "eventPhase = " + e.eventPhase + " expedted " + expected_phase);
        };
    }

    div1.addEventListener("click", check_phase(1), true);
    div1.addEventListener("click", check_phase(3), false);
    div1.onclick = check_phase(3);
    div2.addEventListener("click", check_phase(2), true);
    div2.addEventListener("click", check_phase(2), false);
    div2.onclick = check_phase(2);

    div2.click();
    ok(last_event.eventPhase === 3, "last_event.eventPhase = " + last_event.eventPhase);
});

sync_test("stop_propagation", function() {
    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;

    var calls;
    function record_call(msg) {
        return function() { calls += msg + ","; };
    }

    function stop_propagation(e) {
        calls += "stop,";
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble before stopPropagation = " + e.cancelBubble);
        e.stopPropagation();
        e.cancelBubble_winetest = true;
        ok(e.cancelBubble === true, "cancelBubble after stopPropagation = " + e.cancelBubble);
        ok(e.bubbles === true, "bubbles = " + e.bubbles);
        ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
    }

    function stop_immediate_propagation(e) {
        calls += "immediateStop,";
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble before stopImmediatePropagation = " + e.cancelBubble);
        e.stopImmediatePropagation();
        e.cancelBubble_winetest = true;
        ok(e.cancelBubble === true, "cancelBubble after stopImmediatePropagation = " + e.cancelBubble);
        ok(e.bubbles === true, "bubbles = " + e.bubbles);
        ok(e.cancelable === true, "cancelable = " + e.cancelable);
        ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
    }

    function stop_immediate_propagation_cancel_bubble_false(e) {
        calls += "immediateStop and cancelBubble(false),";
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble before stopImmediatePropagation = " + e.cancelBubble);
        e.stopImmediatePropagation();
        e.cancelBubble = false;
        e.cancelBubble_winetest = e.eventPhase < 2 ? true : false;
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble after stopImmediatePropagation and setting cancelBubble to false = " + e.cancelBubble);
        ok(e.bubbles === true, "bubbles = " + e.bubbles);
        ok(e.cancelable === true, "cancelable = " + e.cancelable);
        ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
    }

    function cancel_bubble_impl(i) {
        return function(e) {
            calls += "cancelBubble["+i+"],";
            ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble before setting cancelBubble = " + e.cancelBubble);
            e.cancelBubble = true;
            if(e.eventPhase < 2)
                ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble after setting cancelBubble during capture phase = " + e.cancelBubble);
            else
                ok(e.cancelBubble === true, "cancelBubble after setting cancelBubble during bubble phase = " + e.cancelBubble);
            e.cancelBubble_winetest = e.cancelBubble;
            ok(e.bubbles === true, "bubbles = " + e.bubbles);
            ok(e.cancelable === true, "cancelable = " + e.cancelable);
            ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
        }
    }

    var cancel_bubble = [];
    for(var i = 0; i < 4; i++)
        cancel_bubble.push(cancel_bubble_impl(i));

    function cancel_bubble_false(e) {
        calls += "cancelBubble(false),";
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble before setting cancelBubble to false = " + e.cancelBubble);
        e.cancelBubble = false;
        if(e.cancelBubble_winetest && e.eventPhase !== 1)
            delete e.cancelBubble_winetest;
        ok(e.cancelBubble === (e.cancelBubble_winetest ? true : false), "cancelBubble after setting cancelBubble to false = " + e.cancelBubble);
        ok(e.bubbles === true, "bubbles = " + e.bubbles);
        ok(e.cancelable === true, "cancelable = " + e.cancelable);
        ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
    }

    div1.addEventListener("click", stop_immediate_propagation_cancel_bubble_false, true);
    div1.addEventListener("click", stop_propagation, true);
    div1.addEventListener("click", cancel_bubble[0], true);
    div1.addEventListener("click", cancel_bubble_false, true);
    div1.addEventListener("click", record_call("div1.click(capture)"), true);

    div2.addEventListener("click", stop_immediate_propagation, true);
    div2.addEventListener("click", cancel_bubble[1], true);
    div2.addEventListener("click", stop_propagation, true);
    div2.addEventListener("click", record_call("div2.click(capture)"), true);

    div1.addEventListener("click", cancel_bubble[2], false);
    div1.addEventListener("click", cancel_bubble_false, false);
    div1.addEventListener("click", stop_propagation, false);
    div1.addEventListener("click", record_call("div1.click(bubble)"), false);

    div2.addEventListener("click", stop_immediate_propagation_cancel_bubble_false, false);
    div2.addEventListener("click", stop_propagation, false);
    div2.addEventListener("click", cancel_bubble[3], false);
    div2.addEventListener("click", cancel_bubble_false, false);
    div2.addEventListener("click", record_call("div2.click(bubble)"), false);

    calls = "";
    div2.click();
    ok(calls === "immediateStop and cancelBubble(false),", "calls = " + calls);

    div1.removeEventListener("click", stop_immediate_propagation_cancel_bubble_false, true);
    calls = "";
    div2.click();
    ok(calls === "stop,cancelBubble[0],cancelBubble(false),div1.click(capture),", "calls = " + calls);

    div1.removeEventListener("click", stop_propagation, true);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble[0],cancelBubble(false),div1.click(capture),immediateStop,", "calls = " + calls);

    div1.removeEventListener("click", cancel_bubble[0], true);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),immediateStop,", "calls = " + calls);

    div2.removeEventListener("click", stop_immediate_propagation, true);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),cancelBubble[1],stop,div2.click(capture),immediateStop and cancelBubble(false),cancelBubble[2],",  // weird native behavior
       "calls = " + calls);

    div2.removeEventListener("click", stop_immediate_propagation_cancel_bubble_false, false);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),cancelBubble[1],stop,div2.click(capture),stop,cancelBubble[3],cancelBubble(false),div2.click(bubble),cancelBubble[2],cancelBubble(false),stop,div1.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", cancel_bubble_false, false);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),cancelBubble[1],stop,div2.click(capture),stop,cancelBubble[3],div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", cancel_bubble[1], true);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),stop,div2.click(capture),stop,cancelBubble[3],div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", stop_propagation, true);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),div2.click(capture),stop,cancelBubble[3],div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", stop_propagation, false);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),div2.click(capture),cancelBubble[3],div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", cancel_bubble[3], false);
    calls = "";
    div2.click();
    ok(calls === "cancelBubble(false),div1.click(capture),div2.click(capture),div2.click(bubble),cancelBubble[2],cancelBubble(false),stop,div1.click(bubble),",
       "calls = " + calls);
});

sync_test("prevent_default", function() {
    document.body.innerHTML = '<div><a href="about:blank"></a></div>';
    var div = document.body.firstChild;
    var a = div.firstChild;
    var calls;

    div.addEventListener("click", function(e) {
        ok(e.defaultPrevented === false, "e.defaultPrevented = " + e.defaultPrevented);
        e.preventDefault();
        ok(e.defaultPrevented === e.cancelable, "e.defaultPrevented = " + e.defaultPrevented);
        calls += "div,";
    }, true);

    a.addEventListener("click", function(e) {
        calls += "a,";
        ok(e.defaultPrevented === true, "e.defaultPrevented = " + e.defaultPrevented);
        ok(e.isTrusted === true, "isTrusted = " + e.isTrusted);
    }, true);

    calls = "";
    a.click();
    ok(calls === "div,a,", "calls = " + calls);

    var e = document.createEvent("Event");
    e.initEvent("click", true, false);

    calls = "";
    div.dispatchEvent(e);
    ok(calls === "div,", "calls = " + calls);

    e = document.createEvent("Event");
    e.initEvent("click", false, true);

    calls = "";
    div.dispatchEvent(e);
    ok(calls === "div,", "calls = " + calls);

    document.body.innerHTML = '<div></div>';
    var elem = document.body.firstChild;
    var e, r;

    elem.onclick = function(event) {
        event.preventDefault();
    }
    e = document.createEvent("Event");
    e.initEvent("click", true, true);
    r = elem.dispatchEvent(e);
    ok(r === false, "dispatchEvent returned " + r);

    elem.onclick = function(event) {
        event.preventDefault();
        ok(event.defaultPrevented === false, "defaultPrevented");
    }
    e = document.createEvent("Event");
    e.initEvent("click", true, false);
    r = elem.dispatchEvent(e);
    ok(r === true, "dispatchEvent returned " + r);

    elem.onclick = function(event) {
        event.stopPropagation();
    }
    e = document.createEvent("Event");
    e.initEvent("click", true, true);
    r = elem.dispatchEvent(e);
    ok(r === true, "dispatchEvent returned " + r);

    e = document.createEvent("Event");
    e.initEvent("click", false, true);
    e.preventDefault();
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);

    e = document.createEvent("Event");
    e.initEvent("click", false, true);
    elem.onclick = null;
    r = elem.dispatchEvent(e);
    ok(r === true, "dispatchEvent returned " + r);
    e.preventDefault();
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
});

sync_test("init_event", function() {
    var e = document.createEvent("Event");
    var calls;

    ok(e.type === "", "type = " + e.type);
    ok(e.cancelable === false, "cancelable = " + e.cancelable);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);
    ok(e.isTrusted === false, "isTrusted = " + e.isTrusted);

    e.initEvent("test", true, false);
    ok(e.type === "test", "type = " + e.type);
    ok(e.cancelable === false, "cancelable = " + e.cancelable);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);
    ok(e.isTrusted === false, "isTrusted = " + e.isTrusted);

    e.preventDefault();
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);

    e.initEvent("NewTest", false, true);
    ok(e.type === "NewTest", "type = " + e.type);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);

    document.body.innerHTML = '<div></div>';
    var elem = document.body.firstChild;

    elem.addEventListener("NewTest", function(event) {
        ok(e === event, "e != event");
        ok(e.isTrusted === false, "isTrusted = " + e.isTrusted);

        e.preventDefault();
        ok(e.defaultPrevented === true, "defaultPrevented = " + e.defaultPrevented);

        /* initEvent no longer has effect */
        event.initEvent("test", true, false);
        ok(event.type === "NewTest", "event.type = " + event.type);
        ok(event.bubbles === false, "bubbles = " + event.bubbles);
        ok(event.cancelable === true, "cancelable = " + event.cancelable);
        ok(e.defaultPrevented === true, "defaultPrevented = " + e.defaultPrevented);

        calls++;
    }, true);

    calls = 0;
    elem.dispatchEvent(e);
    ok(calls === 1, "calls = " + calls);
    ok(e.type === "NewTest", "event.type = " + e.type);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.target === elem, "target != elem");
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);

    /* initEvent no longer has any effect except resetting defaultPrevented */
    e.initEvent("test", true, false);
    ok(e.type === "NewTest", "type = " + e.type);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.target === elem, "target != elem");
    ok(e.defaultPrevented === false, "defaultPrevented = " + e.defaultPrevented);

    calls = 0;
    elem.dispatchEvent(e);
    ok(calls === 1, "calls = " + calls);
    ok(e.type === "NewTest", "event.type = " + e.type);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.target === elem, "target != elem");

    document.body.dispatchEvent(e);
    ok(e.target === document.body, "target != body");
});

sync_test("current_target", function() {
    document.body.innerHTML = '<div><div></div></div>';
    var parent = document.body.firstChild;
    var child = parent.firstChild;
    var calls;
    var e;

    function expect_current_target(expected_target) {
        return function(event) {
            ok(event.currentTarget === expected_target, "unexpected currentTarget");
            calls++;
        }
    }

    parent.addEventListener("test", expect_current_target(parent), true);
    parent.addEventListener("test", expect_current_target(parent), false);
    child.addEventListener("test", expect_current_target(child), true);
    child.addEventListener("test", expect_current_target(child), false);

    e = document.createEvent("Event");
    e.initEvent("test", true, true);
    ok(e.currentTarget === null, "currentTarget != null");

    calls = 0;
    child.dispatchEvent(e);
    ok(calls === 4, "calls = " + calls + " expected 4");
    ok(e.currentTarget === null, "currentTarget != null");
});

sync_test("dispatch_event", function() {
    document.body.innerHTML = '<div><div></div></div>';
    var parent = document.body.firstChild;
    var child = parent.firstChild;
    var calls;
    var e;

    function record_call(msg) {
        return function(event) {
            ok(event === e, "event != e");
            ok(event.target === child, "target != child");
            ok(event.srcElement === child, "srcElement != child");
            calls += msg + ",";
        };
    }

    parent.addEventListener("click", record_call("parent.click(capture)"), true);
    parent.addEventListener("click", record_call("parent.click(bubble)"), false);
    child.addEventListener("click", record_call("child.click(capture)"), true);
    child.addEventListener("click", record_call("child.click(bubble)"), false);
    parent.addEventListener("testing", record_call("parent.testing(capture)"), true);
    parent.addEventListener("testing", record_call("parent.testing(bubble)"), false);
    child.addEventListener("testing", record_call("child.testing(capture)"), true);
    child.addEventListener("testing", record_call("child.testing(bubble)"), false);

    e = document.createEvent("Event");
    e.initEvent("click", true, true);
    ok(e.target === null, "e.target != null");
    ok(e.srcElement === null, "e.srcElement != null");

    calls = "";
    child.dispatchEvent(e);
    ok(calls === "parent.click(capture),child.click(capture),child.click(bubble),"
       + "parent.click(bubble),", "calls = " + calls);
    ok(e.target === child, "e.target != child");
    ok(e.srcElement === child, "e.srcElement != child");
    ok(e.currentTarget === null, "e.currentTarget != null");

    e = document.createEvent("Event");
    e.initEvent("click", false, true);

    calls = "";
    child.dispatchEvent(e);
    ok(calls === "parent.click(capture),child.click(capture),child.click(bubble),",
       "calls = " + calls);

    /* again, without reinitialization */
    calls = "";
    child.dispatchEvent(e);
    ok(calls === "parent.click(capture),child.click(capture),child.click(bubble),",
       "calls = " + calls);

    e = document.createEvent("Event");
    e.initEvent("testing", true, true);

    calls = "";
    child.dispatchEvent(e);
    ok(calls === "parent.testing(capture),child.testing(capture),"
       + "child.testing(bubble),parent.testing(bubble),", "calls = " + calls);

    var xhr = new XMLHttpRequest();
    xhr.addEventListener("testing", function(event) {
        ok(event === e, "event != e");
        ok(event.target === xhr, "target != child");
        ok(event.srcElement === null, "srcElement != child");
        calls += "xhr.testing";
    }, true);

    calls = "";
    xhr.dispatchEvent(e);
    ok(calls === "xhr.testing", "calls = " + calls);
});

sync_test("recursive_dispatch", function() {
    document.body.innerHTML = '<div></div><div></div>';
    var elem1 = document.body.firstChild;
    var elem2 = elem1.nextSibling;
    var calls;

    var e = document.createEvent("Event");
    ok(e.eventPhase === 0, "eventPhase = " + e.eventPhase);

    e.initEvent("test", true, true);
    ok(e.eventPhase === 0, "eventPhase = " + e.eventPhase);

    elem1.addEventListener("test", function(event_arg) {
        calls += "elem1.test,";
        ok(event_arg === e, "event_arg != e");
        try {
            elem2.dispatchEvent(e);
            ok(false, "expected exception");
        }catch(exception) {}
    }, true);

    elem2.addEventListener("test", function() {
        ok(false, "unexpected recursive event call");
    }, true);

    calls = "";
    elem1.dispatchEvent(e);
    ok(calls === "elem1.test,", "calls = " + calls);
    ok(e.eventPhase === 3, "eventPhase = " + e.eventPhase);
});

sync_test("time_stamp", function() {
    document.body.innerHTML = '<div></div>';
    var elem = document.body.firstChild;
    var calls, last_time_stamp;

    elem.onclick = function(event) {
        ok(event.timeStamp === last_time_stamp, "timeStamp = " + event.timeStamp);
        calls++;
    }

    var e = document.createEvent("Event");
    ok(typeof(e.timeStamp) === "number", "typeof(timeStamp) = " + typeof(e.timeStamp));
    ok(e.timeStamp > 0, "timeStamp = " + e.timeStamp);

    var now = (new Date()).getTime();
    last_time_stamp = e.timeStamp;
    ok(Math.abs(now - last_time_stamp) < 3, "timeStamp " + last_time_stamp + " != now " + now);

    e.initEvent("click", true, true);
    ok(e.timeStamp === last_time_stamp, "timeStamp = " + e.timeStamp);
    calls = 0;
    elem.dispatchEvent(e);
    ok(calls === 1, "calls = " + calls);
    ok(e.timeStamp === last_time_stamp, "timeStamp = " + e.timeStamp);

    elem.onclick = function(event) {
        ok(event.timeStamp > 0, "timeStamp = " + event.timeStamp);
        trace("timestamp " + event.timeStamp);
        calls++;
    }

    calls = 0;
    elem.click();
    ok(calls === 1, "calls = " + calls);
});

sync_test("mouse_event", function() {
    var e;

    e = document.createEvent("MouseEvent");
    ok(e.screenX === 0, "screenX = " + e.screenX);
    ok(e.screenY === 0, "screenY = " + e.screenY);
    ok(e.clientX === 0, "clientX = " + e.clientX);
    ok(e.clientY === 0, "clientY = " + e.clientY);
    ok(e.offsetX === 0, "offsetX = " + e.offsetX);
    ok(e.offsetY === 0, "offsetY = " + e.offsetY);
    ok(e.ctrlKey === false, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === false, "altKey = " + e.altKey);
    ok(e.shiftKey === false, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === false, "metaKey = " + e.metaKey);
    ok(e.button === 0, "button = " + e.button);
    ok(e.buttons === 0, "buttons = " + e.buttons);
    ok(e.pageX === 0, "pageX = " + e.pageX);
    ok(e.pageY === 0, "pageY = " + e.pageY);
    ok(e.which === 1, "which = " + e.which);
    ok(e.relatedTarget === null, "relatedTarget = " + e.relatedTarget);
    ok(e.toElement === null, "toElement = " + e.toElement);
    ok(e.fromElement === null, "fromElement = " + e.fromElement);

    e.initMouseEvent("test", true, true, window, 1, 2, 3, 4, 5, false, false, false, false, 1, document);
    ok(e.type === "test", "type = " + e.type);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.detail === 1, "detail = " + e.detail);
    todo_wine.
    ok(e.view === window, "view != window");
    ok(e.screenX === 2, "screenX = " + e.screenX);
    ok(e.screenY === 3, "screenY = " + e.screenY);
    ok(e.clientX === 4, "clientX = " + e.clientX);
    ok(e.clientY === 5, "clientY = " + e.clientY);
    ok(e.ctrlKey === false, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === false, "altKey = " + e.altKey);
    ok(e.shiftKey === false, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === false, "metaKey = " + e.metaKey);
    ok(e.button === 1, "button = " + e.button);
    ok(e.buttons === 0, "buttons = " + e.buttons);
    ok(e.which === 2, "which = " + e.which);
    ok(e.relatedTarget === document, "relatedTarget = " + e.relatedTarget);

    e.initMouseEvent("test", false, false, window, 9, 8, 7, 6, 5, true, true, true, true, 127, document.body);
    ok(e.type === "test", "type = " + e.type);
    ok(e.cancelable === false, "cancelable = " + e.cancelable);
    ok(e.bubbles === false, "bubbles = " + e.bubbles);
    ok(e.detail === 9, "detail = " + e.detail);
    ok(e.screenX === 8, "screenX = " + e.screenX);
    ok(e.screenY === 7, "screenY = " + e.screenY);
    ok(e.clientX === 6, "clientX = " + e.clientX);
    ok(e.clientY === 5, "clientY = " + e.clientY);
    ok(e.ctrlKey === true, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === true, "altKey = " + e.altKey);
    ok(e.shiftKey === true, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === true, "metaKey = " + e.metaKey);
    ok(e.button === 127, "button = " + e.button);
    ok(e.which === 128, "which = " + e.which);
    ok(e.relatedTarget === document.body, "relatedTarget = " + e.relatedTarget);

    e.initEvent("testevent", true, true);
    ok(e.type === "testevent", "type = " + e.type);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.detail === 9, "detail = " + e.detail);
    ok(e.screenX === 8, "screenX = " + e.screenX);
    ok(e.screenY === 7, "screenY = " + e.screenY);
    ok(e.clientX === 6, "clientX = " + e.clientX);
    ok(e.clientY === 5, "clientY = " + e.clientY);
    ok(e.ctrlKey === true, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === true, "altKey = " + e.altKey);
    ok(e.shiftKey === true, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === true, "metaKey = " + e.metaKey);
    ok(e.button === 127, "button = " + e.button);

    e.initUIEvent("testevent", true, true, window, 6);
    ok(e.type === "testevent", "type = " + e.type);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.detail === 6, "detail = " + e.detail);
    ok(e.screenX === 8, "screenX = " + e.screenX);
    ok(e.screenY === 7, "screenY = " + e.screenY);
    ok(e.clientX === 6, "clientX = " + e.clientX);
    ok(e.clientY === 5, "clientY = " + e.clientY);
    ok(e.ctrlKey === true, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === true, "altKey = " + e.altKey);
    ok(e.shiftKey === true, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === true, "metaKey = " + e.metaKey);
    ok(e.button === 127, "button = " + e.button);
});

sync_test("ui_event", function() {
    var e;

    e = document.createEvent("UIEvent");
    ok(e.detail === 0, "detail = " + e.detail);

    e.initUIEvent("test", true, true, window, 3);
    ok(e.type === "test", "type = " + e.type);
    ok(e.cancelable === true, "cancelable = " + e.cancelable);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.detail === 3, "detail = " + e.detail);
    todo_wine.
    ok(e.view === window, "view != window");
});

sync_test("keyboard_event", function() {
    var e;

    e = document.createEvent("KeyboardEvent");

    e.initEvent("test", true, true);
    ok(e.key === "", "key = " + e.key);
    ok(e.keyCode === 0, "keyCode = " + e.keyCode);
    ok(e.charCode === 0, "charCode = " + e.charCode);
    ok(e.repeat === false, "repeat = " + e.repeat);
    ok(e.ctrlKey === false, "ctrlKey = " + e.ctrlKey);
    ok(e.altKey === false, "altKey = " + e.altKey);
    ok(e.shiftKey === false, "shiftKey = " + e.shiftKey);
    ok(e.metaKey === false, "metaKey = " + e.metaKey);
    ok(e.location === 0, "location = " + e.location);
    ok(e.detail === 0, "detail = " + e.detail);
    ok(e.which === 0, "which = " + e.which);
    ok(e.locale === "", "locale = " + e.locale);
});

sync_test("custom_event", function() {
    var e = document.createEvent("CustomEvent");

    ok(e.detail === undefined, "detail = " + e.detail);

    e.initCustomEvent("test", true, false, 123);
    ok(e.type === "test", "type = " + e.type);
    ok(e.bubbles === true, "bubbles = " + e.bubbles);
    ok(e.cancelable === false, "cancelable = " + e.cancelable);
    ok(e.detail === 123, "detail = " + e.detail);
});

async_test("error_event", function() {
    document.body.innerHTML = '<div><img></img></div>';
    var div = document.body.firstChild;
    var img = div.firstChild;
    var calls = "";

    function record_call(msg) {
        return function() { calls += msg + ","; };
    }
    var win_onerror = record_call("window.onerror");
    var doc_onerror = record_call("doc.onerror");
    var body_onerror = record_call("body.onerror");

    window.addEventListener("error", win_onerror, true);
    document.addEventListener("error", doc_onerror, true);
    document.body.addEventListener("error", body_onerror, true);
    div.addEventListener("error", record_call("div.onerror"), true);

    div.addEventListener("error", function() {
        ok(calls === "window.onerror,doc.onerror,body.onerror,div.onerror,", "calls = " + calls);

        window.removeEventListener("error", win_onerror, true);
        document.removeEventListener("error", doc_onerror, true);
        document.body.removeEventListener("error", body_onerror, true);
        next_test();
    }, true);

    img.src = "about:blank";
});

async_test("detached_img_error_event", function() {
    var img = new Image();
    img.onerror = function() {
        next_test();
    }
    img.src = "about:blank";
});

async_test("img_wrong_content_type", function() {
    var img = new Image();
    img.onload = function() {
        ok(img.width === 2, "width = " + img.width);
        ok(img.height === 2, "height = " + img.height);
        next_test();
    }
    img.src = "img.png?content-type=image/jpeg";
});

async_test("message event", function() {
    var listener_called = false, iframe = document.createElement("iframe");

    window.addEventListener("message", function(e) {
        if(listener_called) {
            ok(e.data === "echo", "e.data (diff origin) = " + e.data);
            ok(e.source === iframe.contentWindow, "e.source (diff origin) not iframe.contentWindow");
            ok(e.origin === "http://winetest.different.org:1234", "e.origin (diff origin) = " + e.origin);
            next_test();
            return;
        }
        listener_called = true;
        e.initMessageEvent("blah", true, true, "barfoo", "wine", 1234, window);
        ok(e.data === "test", "e.data = " + e.data);
        ok(e.bubbles === false, "bubbles = " + e.bubbles);
        ok(e.cancelable === false, "cancelable = " + e.cancelable);
        ok(e.source === window, "e.source = " + e.source);
        ok(e.origin === "http://winetest.example.org", "e.origin = " + e.origin);

        iframe.onload = function() { iframe.contentWindow.postMessage("echo", "hTtP://WinEtesT.difFerent.ORG:1234"); }
        iframe.src = "http://winetest.different.org:1234/xhr_iframe.html";
        document.body.appendChild(iframe);
    });

    window.postMessage("test", "httP://wineTest.example.org");
    ok(listener_called == false, "listener already called");
});
