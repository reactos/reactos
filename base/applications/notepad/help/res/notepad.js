function resolve_images(common_res_url) {
  // NOTE: We can't use DOMContentLoaded yet! Please use window.onload instead.
  window.onload = (function() {
    // Common resources
    var common_roslogo_url = common_res_url + "roslogo-32x32.png";
    document.getElementById("top_script").href = common_res_url + "common.js";
    document.getElementById("top_stylesheet").href = common_res_url + "common.css";
    document.getElementById("top_favicon").href = common_roslogo_url;

    // TODO: Embed and/or reference image(s).
    // To embed image data, please use this: https://dopiaza.org/goodies/data-uri-generator/
    var app_icon_url = "./res/notepad-32x32.png";
    document.getElementById("app_icon").src = app_icon_url;
    document.getElementById("roslogo").src = common_roslogo_url;
  });
}
