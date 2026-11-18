/*
 * Copyright 2022 Gabriel Ivăncescu for CodeWeavers
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

async_test("reload", function() {
    if(sessionStorage.getItem("skip reload test")) {
        ok(performance.navigation.type === 1, "navigation type = " + performance.navigation.type);
        next_test();
        return;
    }

    var script = document.createElement("script");
    script.src ="http://winetest.different.org/jsstream.php?reload";
    document.getElementsByTagName("head")[0].appendChild(script);

    external.writeStream("reload",
    '   try {' +
    '       window.location.reload();' +
    '       sessionStorage.setItem("skip reload test", true);' +
    '   }catch(e) {' +
    '       ok(false, "reload with different origin threw " + e.number);' +
    '   }' +
    '   next_test()'
    );
});
