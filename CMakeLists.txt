cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(ICE DESCRIPTION "ICE Framework" VERSION 0.1.0 LANGUAGES CXX)

option(ICE_INSTALL_LIBRARY "Install library" ${PROJECT_IS_TOP_LEVEL})
option(ICE_INSTALL_HEADERS "Install header files" ${ICE_INSTALL_LIBRARY})
option(ICE_INSTALL_CONFIGS "Install CMake configuration files" ${ICE_INSTALL_LIBRARY})
option(ICE_INSTALL_VERSION "Set VERSION and SOVERSION target properties" OFF)

configure_file(src/ice/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/ice/version.hpp LF)

file(GLOB_RECURSE headers include/*.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/src/ice/version.hpp)

set(sources
  src/ice/error.cpp
  src/ice/version.cpp)

if(WIN32 AND BUILD_SHARED_LIBS)
  configure_file(res/ice.rc.in ${CMAKE_CURRENT_BINARY_DIR}/ice.rc CRLF)
  list(APPEND sources ${CMAKE_CURRENT_BINARY_DIR}/ice.rc res/ice.manifest)
endif()

add_library(ice ${headers} ${sources})
target_compile_features(ice PRIVATE cxx_std_23 PUBLIC cxx_std_20)
target_include_directories(ice PRIVATE src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(ice PRIVATE ICE_EXPORTS PUBLIC ICE_SHARED)
endif()

if(WIN32)
  target_compile_definitions(ice PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
  set_target_properties(ice PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif()

find_package(Threads REQUIRED)
target_link_libraries(ice PRIVATE Threads::Threads)

if(UNIX AND CMAKE_DL_LIBS)
  target_link_libraries(ice PRIVATE ${CMAKE_DL_LIBS})
endif()

if(ICE_INSTALL_LIBRARY)
  if(ICE_INSTALL_VERSION AND BUILD_SHARED_LIBS)
    set_target_properties(ice PROPERTIES VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
  endif()

  include(GNUInstallDirs)
  install(TARGETS ice EXPORT ice-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  
  if(ICE_INSTALL_CONFIGS)
    include(CMakePackageConfigHelpers)
    install(EXPORT ice-targets FILE ice-targets.cmake NAMESPACE ice::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ice)

    configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/res/ice.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ice)

    write_basic_package_version_file(
      ${CMAKE_CURRENT_BINARY_DIR}/ice-config-version.cmake
      VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)

    install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/ice-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/ice-config-version.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ice)
  endif()
endif()

if(ICE_INSTALL_HEADERS)
  install(DIRECTORY include/ice
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/ice/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ice)
endif()

add_library(ice::ice ALIAS ice)
