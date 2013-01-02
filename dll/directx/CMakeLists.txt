
add_subdirectory(amstream)
add_subdirectory(d3d8thk)
#add_subdirectory(bdaplgin) #disabled in trunk
#add_subdirectory(d3d8) #disabled in trunk, see wine subfolder
#add_subdirectory(d3d9) #disabled in trunk, see wine subfolder
#add_subdirectory(ddraw) #disabled in trunk, see wine subfolder
add_subdirectory(devenum)
add_subdirectory(dinput)
add_subdirectory(dinput8)
add_subdirectory(dmusic)
add_subdirectory(dplay)
add_subdirectory(dplayx)
add_subdirectory(dsound)
#add_subdirectory(dsound_new) #disabled in trunk
add_subdirectory(dxdiagn)
if(NOT MSVC)
    add_subdirectory(ksproxy) #FIXME: fix msvc build
endif()
add_subdirectory(ksuser)
add_subdirectory(msdmo)
#add_subdirectory(msdvbnp) #disabled in trunk
#add_subdirectory(msvidctl) #disabled in trunk
add_subdirectory(qedit)
add_subdirectory(quartz)
add_subdirectory(wine)
