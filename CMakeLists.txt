cmake_minimum_required(VERSION 3.15)
project(MPCBEAliyunSub VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

set(DS_LIBS strmiids.lib quartz.lib ole32.lib oleaut32.lib)

add_library(mpc-be-aliyun-sub SHARED 
    src/MainFilter.cpp 
    src/AliyunClient.cpp 
    src/OverlayWindow.cpp 
    src/ConfigWindow.cpp
)

target_link_libraries(mpc-be-aliyun-sub 
    PRIVATE 
    CURL::libcurl 
    nlohmann_json::nlohmann_json 
    ${DS_LIBS}
)

set_target_properties(mpc-be-aliyun-sub PROPERTIES PREFIX "")
