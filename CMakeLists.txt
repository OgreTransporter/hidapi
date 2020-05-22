cmake_minimum_required(VERSION 3.10.2)

file(READ "configure.ac" _file_content)
string(REGEX MATCHALL "\\[HIDAPI_([A-Z]+)\\],([ ]+)([0-9]+)" _lib_version ${_file_content})
foreach(rxver ${_lib_version})
  string(REGEX MATCH "\\[HIDAPI_([A-Z]+)\\],([ ]+)([0-9]+)" _line_ver ${rxver})
  set(HIDAPI_${CMAKE_MATCH_1} ${CMAKE_MATCH_3})
endforeach()
set(VERSION_STRING "${HIDAPI_MAJOR}.${HIDAPI_MINOR}.${HIDAPI_RELEASE}")
message("Configure HIDAPI ${VERSION_STRING}")

project(HIDAPI LANGUAGES C CXX VERSION ${VERSION_STRING})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX "d")
endif()
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(HIDAPI_INC ${CMAKE_CURRENT_SOURCE_DIR}/hidapi)
set(HIDAPI_HDR hidapi/hidapi.h)
if(WIN32)
  set(HIDAPI_SRC windows/hid.c)
  set(HIDAPI_LIB setupapi.lib)
else()
  if((CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") OR (CMAKE_SYSTEM_NAME STREQUAL "kFreeBSD"))
    set(HIDAPI_INTERFACE "libusb")
  elseif(APPLE)
    # TODO: Add find IOHidManager
    set(HIDAPI_SRC mac/hid.c)
  else()
    set(HIDAPI_INTERFACE "libusb" CACHE STRING "Interface chosen for use by HIDAPI")
    set_property(CACHE HIDAPI_INTERFACE PROPERTY STRINGS libusb hidraw)
  endif()
  if(HIDAPI_INTERFACE EQUAL "libusb")
    find_package(LibUSB REQUIRED)
	list(APPEND HIDAPI_INC ${LIBUSB_INCLUDE_DIR})
    set(HIDAPI_SRC libusb/hid.c)
    set(HIDAPI_LIB ${LIBUSB_LIBRARY})
  else()
    set(HIDAPI_SRC linux/hid.c)
  endif()
endif()

add_library(hidapi ${HIDAPI_SRC} ${HIDAPI_HDR})
target_link_libraries(hidapi PUBLIC ${HIDAPI_LIB})
target_include_directories(hidapi PUBLIC ${HIDAPI_INC})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(hidapi PUBLIC -DHIDAPI_EXPORTS)
else()
  target_compile_definitions(hidapi PUBLIC -DHIDAPI_STATIC)
endif()
install(TARGETS hidapi)
install(FILES hidapi/hidapi.h DESTINATION include)

option(BUILD_TOOLS "Build test programs" ON)
if(BUILD_TOOLS)
  find_package(Qt5 COMPONENTS Widgets)
  if(Qt5_FOUND)
    add_executable(hidapi-testgui
      testgui/main.cpp
      testgui/mainwindow.cpp
      testgui/mainwindow.h
      testgui/mainwindow.ui
	  ${HIDAPI_SRC}
	  ${HIDAPI_HDR}
    )
    target_compile_definitions(hidapi-testgui PUBLIC -DHIDAPI_STATIC)
    target_include_directories(hidapi-testgui PUBLIC ${HIDAPI_INC} ${Qt5_INCLUDE_DIR})
    target_link_libraries(hidapi-testgui PUBLIC ${HIDAPI_LIB} Qt5::Widgets)
    install(TARGETS hidapi-testgui)
    if(WIN32)
      get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
      get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
      find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
      add_custom_command(TARGET hidapi-testgui
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
        COMMAND set PATH="${_qt_bin_dir}"
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" --verbose 0 --no-compiler-runtime --no-translations --no-angle --no-opengl-sw "$<TARGET_FILE:hidapi-testgui>"
      )
      install(DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
        DESTINATION bin
      )
      if(MSVC)
        set(vsqtdbg "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Project ToolsVersion=\"15.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n  <PropertyGroup>\n    <LocalDebuggerEnvironment>PATH=")
        set(vsqtdbg "${vsqtdbg}${_qt_bin_dir}")
        set(vsqtdbg "${vsqtdbg}\;$(Path)\n$(LocalDebuggerEnvironment)</LocalDebuggerEnvironment>\n    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>\n  </PropertyGroup>\n</Project>")
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/hidapi-testgui.vcxproj.user" ${vsqtdbg})
      endif()
    endif()
  endif()
  add_executable(hidtest ${HIDAPI_SRC} ${HIDAPI_HDR} hidtest/test.c)
  target_compile_definitions(hidtest PUBLIC -DHIDAPI_STATIC)
  target_include_directories(hidtest PUBLIC ${HIDAPI_INC})
  target_link_libraries(hidtest PUBLIC ${HIDAPI_LIB})
  install(TARGETS hidtest)
endif()

option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" OFF)
find_package(Doxygen)

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation.")
  endif()
  if(DOXYGEN_DOT_FOUND)
    set(HAVE_DOT "YES")
  else()
    set(HAVE_DOT "NO")
  endif()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc DESTINATION shared)
endif()