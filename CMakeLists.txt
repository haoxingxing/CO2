cmake_minimum_required(VERSION 3.5)
project(CO2 LANGUAGES C CXX)

# Libspeexdsp for aec
include_directories("speexdsp/include")
add_subdirectory(speexdsp/libspeexdsp)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets Multimedia MultimediaWidgets Network REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if(ANDROID)
  add_library(CO2 SHARED
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    p2pnetwork.cpp
    p2pnetwork.h
    "tools.h"
  )
elseif(WIN32)
  add_executable(CO2 WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    p2pnetwork.cpp
    p2pnetwork.h
   "tools.h")
else()
    add_executable(CO2
      main.cpp
      mainwindow.cpp
      mainwindow.h
      mainwindow.ui
      p2pnetwork.cpp
      p2pnetwork.h
      "tools.h"
    )
endif()
target_link_libraries(CO2  PRIVATE Qt5::Widgets Qt5::MultimediaWidgets Qt5::Network Qt5::Multimedia speexdsp)
