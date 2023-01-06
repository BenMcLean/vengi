set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VENDOR "Martin Gerhardy")
set(CPACK_PACKAGE_DESCRIPTION "Voxel editor and format converter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Voxel tools")
set(CPACK_RESOURCE_FILE_LICENSE ${ROOT_DIR}/LICENSE)
set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
#set(CPACK_SOURCE_IGNORE_FILES "~$")
#set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
#set(CPACK_PACKAGE_CONTACT "martin.gerhardy@gmail.com")
#set(CPACK_PACKAGE_ICON )
#set(CPACK_INSTALL_CMAKE_PROJECTS )
#set(CPACK_PACKAGE_INSTALL_DIRECTORY "${ROOT_PROJECT_NAME}")

set(CPACK_NSIS_DISPLAY_NAME "vengi voxel tools")
set(CPACK_NSIS_URL_INFO_ABOUT "https://mgerhardy.github.io/vengi")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_COMPRESSOR bzip2)
set(CPACK_NSIS_MENU_LINKS "https://mgerhardy.github.io/vengi" "vengi help")

if (APPLE)
	set(CPACK_GENERATOR "DragNDrop")
elseif (UNIX)
	set(CPACK_GENERATOR "TGZ;TBZ2")
	find_host_program(RPMBUILD rpmbuild)
	if (RPMBUILD)
		list(APPEND CPACK_GENERATOR RPM)
		set(CPACK_RPM_COMPONENT_INSTALL ON)
		set(CPACK_RPM_PACKAGE_LICENSE GPL)
		set(CPACK_RPM_PACKAGE_REQUIRES)
	endif()
	find_host_program(NSIS makensis)
	if (NSIS)
		list(APPEND CPACK_GENERATOR NSIS)
	endif()
	set(CPACK_SOURCE_GENERATOR "TGZ")
elseif (WIN32)
	set(CPACK_GENERATOR "ZIP;NSIS")
	set(CPACK_SOURCE_GENERATOR "ZIP")
endif()
