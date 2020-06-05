#build file for CMake, see http://www.cmake.org/

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(pawn)

set(CMAKE_MACOSX_RPATH ON)

#set(CMAKE_INSTALL_RPATH "@executable_path")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


ADD_SUBDIRECTORY(./compiler)
ADD_SUBDIRECTORY(./amx)
