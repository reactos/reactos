tinyMCE.importPluginLanguagePack('autosave');
var TinyMCE_AutoSavePlugin={
getInfo:function(){
return{
longname:'Auto save',
author:'Moxiecode Systems AB',
authorurl:'http://tinymce.moxiecode.com',
infourl:'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/autosave',
version:tinyMCE.majorVersion+"."+tinyMCE.minorVersion
};
},
_beforeUnloadHandler:function(){
var n,inst,anyDirty=false,msg=tinyMCE.getLang("lang_autosave_unload_msg");
if(tinyMCE.getParam("fullscreen_is_enabled"))
return;
for(n in tinyMCE.instances){
inst=tinyMCE.instances[n];
if(tinyMCE.isInstance(inst)){
if(inst.isDirty()){
if(roscmseditorsavecache!=inst.getHTML()){
alert('drin');
return;
}
else{
alert('ned drin');
return;
}
}
}
else{
continue;
}
}
return;
}
};
window.onbeforeunload=TinyMCE_AutoSavePlugin._beforeUnloadHandler;
tinyMCE.addPlugin("autosave",TinyMCE_AutoSavePlugin);
