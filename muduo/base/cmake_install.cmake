# Install script for directory: /home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/build/debug-install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/lib/libmuduo_base.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/base" TYPE FILE FILES
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/AsyncLogging.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Atomic.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/BlockingQueue.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/BoundedBlockingQueue.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Condition.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/CountDownLatch.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/CurrentThread.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Date.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Exception.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/FileUtil.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/LogFile.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/LogStream.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Logging.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Mutex.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/ProcessInfo.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Singleton.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/StringPiece.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Thread.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/ThreadLocal.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/ThreadLocalSingleton.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/ThreadPool.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/TimeZone.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Timestamp.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/Types.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/copyable.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/base/tests/cmake_install.cmake")

endif()

