#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "googletest_gmock_main" for configuration "Debug"
set_property(TARGET googletest_gmock_main APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(googletest_gmock_main PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "googletest_gmock"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib${POLAR_LIBDIR_SUFFIX}/libgmock_maind${CMAKE_SHARED_LIBRARY_SUFFIX}"
  IMPORTED_SONAME_DEBUG "libgmock_maind${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )

list(APPEND _IMPORT_CHECK_TARGETS googletest_gmock_main )
list(APPEND _IMPORT_CHECK_FILES_FOR_googletest_gmock_main "${_IMPORT_PREFIX}/lib${POLAR_LIBDIR_SUFFIX}/libgmock_maind${CMAKE_SHARED_LIBRARY_SUFFIX}" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
