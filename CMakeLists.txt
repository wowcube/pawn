#build file for CMake, see http://www.cmake.org/

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(pawn)

set(CMAKE_MACOSX_RPATH 1)

ADD_SUBDIRECTORY(./compiler)
ADD_SUBDIRECTORY(./amx)
