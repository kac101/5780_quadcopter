# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/f/capstone/build/_deps/picotool-src"
  "/mnt/f/capstone/build/_deps/picotool-build"
  "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix"
  "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/tmp"
  "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
  "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/src"
  "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/f/capstone/build/_deps/picotool-subbuild/picotool-populate-prefix/src/picotool-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
