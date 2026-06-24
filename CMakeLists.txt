cmake_minimum_required(VERSION 3.20)
project(MPCBEAliyunSub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dependencies ----
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# ---- DirectShow baseclasses (static lib) ----
add_library(strmbase STATIC
    baseclasses/streams.h
    baseclasses/wxdebug.h    baseclasses/wxdebug.cpp
    baseclasses/wxutil.h     baseclasses/wxutil.cpp
    baseclasses/msgthrd.h
    baseclasses/combase.h    baseclasses/combase.cpp
    baseclasses/amfilter.h   baseclasses/amfilter.cpp
    baseclasses/transfrm.h   baseclasses/transfrm.cpp
)
# empty file — exclude from build
set_source_files_properties(
    baseclasses/msgthrd.cpp  baseclasses/strmbase.vcxproj
    PROPERTIES HEADER_FILE_ONLY TRUE
)
target_include_directories(strmbase PUBLIC baseclasses)
target_link_libraries(strmbase PUBLIC winmm strmiids msdmo)
if(MSVC)
    target_compile_definitions(strmbase PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---- Plugin DLL ----
add_library(mpc-be-aliyun-sub SHARED
    src/PluginConfig.h    src/PluginConfig.cpp
    src/AliyunClient.h    src/AliyunClient.cpp
    src/SubtitleOverlay.h  src/SubtitleOverlay.cpp
    src/ConfigDialog.h    src/ConfigDialog.cpp
    src/MainFilter.h      src/MainFilter.cpp
    src/exports.def
)
target_include_directories(mpc-be-aliyun-sub PRIVATE src baseclasses)
target_link_libraries(mpc-be-aliyun-sub PRIVATE
    strmbase
    CURL::libcurl
    nlohmann_json::nlohmann_json
    strmiids quartz ole32 oleaut32 winmm
)
if(MSVC)
    target_compile_definitions(mpc-be-aliyun-sub PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_options(mpc-be-aliyun-sub PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/exports.def")
endif()
set_target_properties(mpc-be-aliyun-sub PROPERTIES PREFIX "")
