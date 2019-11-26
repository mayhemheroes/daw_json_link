
include( ExternalProject )
externalproject_add(
  header_libraries_prj
  GIT_REPOSITORY "https://github.com/beached/header_libraries.git"
  SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/header_libraries"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DGLEAN_INSTALL_ROOT=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
  utf_range_prj
  DEPENDS header_libraries_prj
  GIT_REPOSITORY "https://github.com/beached/utf_range.git"
  SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/utf_range"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DGLEAN_INSTALL_ROOT=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
  date_prj
  GIT_REPOSITORY "https://github.com/beached/date.git"
  SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/date"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DGLEAN_INSTALL_ROOT=${CMAKE_BINARY_DIR}/install  -DUSE_SYSTEM_TZ_DB=ON -DENABLE_DATE_TESTING=OFF -DCMAKE_CXX_STANDARD=17
)

externalproject_add(
  iso8601_parsing_prj
  DEPENDS date_prj header_libraries_prj
  GIT_REPOSITORY "https://github.com/beached/iso8601_parsing.git"
  SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/iso8601_parsing"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install -DGLEAN_INSTALL_ROOT=${CMAKE_BINARY_DIR}/install
)

include_directories( SYSTEM "${CMAKE_BINARY_DIR}/install/include" )
link_directories( "${CMAKE_BINARY_DIR}/install/lib" )
set( DEP_PROJECT_DEPS utf_range_prj date_prj iso8601_parsing_prj header_libraries_prj )

