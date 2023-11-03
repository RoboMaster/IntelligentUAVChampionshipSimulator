set(Target ${This})

message("")
message(STATUS "Start building : " ${Target})

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

# 设置包含路径
include_directories(
    ${ROOT_DIR}/include
    # ${ROOT_DIR}/thirdParties/websocketpp
    ${ROOT_DIR}/thirdParties/jsoncpp/include
    ${Boost_INCLUDE_DIR}
    ${Threads_INCLUDE_DIR}
    )

# # 添加当前文件夹下所有文件为源文件
# aux_source_directory(. SRCS)

# 生成动态库

 
add_library(GNSSSimulator SHARED gnss_simulator.cpp simulator.cpp)
add_library(GNSSSimulatorStatic STATIC gnss_simulator.cpp simulator.cpp)

add_dependencies(GNSSSimulator jsoncpp_lib)
add_dependencies(GNSSSimulatorStatic jsoncpp_static)

IF(CMAKE_BUILD_TYPE MATCHES Release)
    target_link_libraries(GNSSSimulator LINK_PUBLIC ${CMAKE_BINARY_DIR}/Lib/Release/libjsoncpp.so)
    target_link_libraries(GNSSSimulatorStatic LINK_PUBLIC ${CMAKE_BINARY_DIR}/lib/Release/libjsoncpp.a)
ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    target_link_libraries(GNSSSimulator LINK_PUBLIC ${CMAKE_BINARY_DIR}/Lib/Debug/libjsoncppd.so)
    target_link_libraries(GNSSSimulatorStatic LINK_PUBLIC ${CMAKE_BINARY_DIR}/lib/Debug/libjsoncppd.a)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)
                                

add_executable(${Target} main.cpp crc.cpp)
add_dependencies(${Target} GNSSSimulatorStatic)

IF(${CMAKE_BUILD_TYPE} MATCHES "Release")
    target_link_libraries(${Target} PRIVATE
    ${CMAKE_BINARY_DIR}/lib/Release/libjsoncpp.a
    ${CMAKE_BINARY_DIR}/lib/Release/libGNSSSimulatorStatic.a 
    ${CMAKE_BINARY_DIR}/Lib/Release/libglog.so
    Threads::Threads)
ENDIF(${CMAKE_BUILD_TYPE} MATCHES "Release")

IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    target_link_libraries(${Target} PRIVATE
    ${CMAKE_BINARY_DIR}/lib/Debug/libjsoncppd.a
    ${CMAKE_BINARY_DIR}/lib/Debug/libGNSSSimulatorStaticd.a 
    ${CMAKE_BINARY_DIR}/Lib/Debug/libglogd.so
    Threads::Threads)
ENDIF(${CMAKE_BUILD_TYPE} MATCHES "Debug")

message(STATUS "Stop building : " ${Target} "\n")