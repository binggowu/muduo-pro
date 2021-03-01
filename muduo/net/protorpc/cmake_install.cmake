# Install script for directory: /home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/lib/libmuduo_protorpc.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/muduo/net/protorpc" TYPE FILE FILES
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/RpcCodec.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/RpcChannel.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/RpcServer.h"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/rpc.proto"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/rpcservice.proto"
    "/home/abing/Learning/muduo_tut/muduo-0.9.1-beta/muduo/muduo/net/protorpc/rpc.pb.h"
    )
endif()

