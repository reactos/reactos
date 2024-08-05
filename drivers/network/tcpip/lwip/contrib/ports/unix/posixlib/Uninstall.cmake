cmake_minimum_required(VERSION 3.13)
set(MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

if(NOT EXISTS ${MANIFEST})
	message(FATAL_ERROR "Cannot find install manifest: '${MANIFEST}'")
endif()

file(STRINGS ${MANIFEST} files)
foreach(file ${files})
	if(EXISTS ${file} OR IS_SYMLINK ${file})
		message(STATUS "Removing: ${file}")

		execute_process(
				COMMAND rm -f ${file}
				RESULT_VARIABLE retcode
			)

		if(NOT "${retcode}" STREQUAL "0")
			message(WARNING "Failed to remove: ${file}")
		endif()
	endif()
endforeach(file)
