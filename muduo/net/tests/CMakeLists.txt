add_executable(echoserver_unittest EchoServer_unittest.cc)
target_link_libraries(echoserver_unittest muduo_net)

add_executable(echoclient_unittest EchoClient_unittest.cc)
target_link_libraries(echoclient_unittest muduo_net)

add_executable(eventloop_unittest EventLoop_unittest.cc)
target_link_libraries(eventloop_unittest muduo_net)

add_executable(eventloopthreadpool_unittest EventLoopThreadPool_unittest.cc)
target_link_libraries(eventloopthreadpool_unittest muduo_net)

if(BOOSTTEST_LIBRARY)
add_executable(buffer_unittest Buffer_unittest.cc)
target_link_libraries(buffer_unittest muduo_net boost_unit_test_framework)

add_executable(inetaddress_unittest InetAddress_unittest.cc)
target_link_libraries(inetaddress_unittest muduo_net boost_unit_test_framework)
endif()

add_executable(timerqueue_unittest TimerQueue_unittest.cc)
target_link_libraries(timerqueue_unittest muduo_net)

add_executable(Reactor_test01 Reactor_test01.cc)
target_link_libraries(Reactor_test01 muduo_net)

add_executable(Reactor_test02 Reactor_test02.cc)
target_link_libraries(Reactor_test02 muduo_net)

add_executable(Reactor_test03 Reactor_test03.cc)
target_link_libraries(Reactor_test03 muduo_net)
