# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/f/capstone/build/_deps/picotool-src"
  "/mnt/f/capstone/build/_deps/picotool-build"
  "/mnt/f/capstone/build/_deps"
  "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/tmp"
  "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/src/picotoolBuild-stamp"
  "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/src"
  "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/src/picotoolBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/src/picotoolBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/f/capstone/build/pico-sdk/src/rp2350/boot_stage2/picotool/src/picotoolBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
