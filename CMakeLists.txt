cmake_minimum_required(VERSION 3.20)
project(MPCBEAliyunSub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dependencies ----
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# ---- Global compile definitions (safe for everyone) ----
add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0601
)

# ---- DirectShow baseclasses (static lib) ----
# IMPORTANT: do NOT define NOMINMAX here — the legacy baseclass code uses
# the min/max macros from <windows.h>
add_library(strmbase STATIC
    baseclasses/combase.cpp
    baseclasses/wxdebug.cpp
    baseclasses/wxutil.cpp
    baseclasses/wxlist.cpp
    baseclasses/winutil.cpp
    baseclasses/ctlutil.cpp
    baseclasses/amfilter.cpp
    baseclasses/transfrm.cpp
    baseclasses/mtype.cpp
    baseclasses/outputq.cpp
    baseclasses/dllentry.cpp
    baseclasses/dllsetup.cpp
    baseclasses/pstream.cpp
    baseclasses/refclock.cpp
    baseclasses/schedule.cpp
    baseclasses/sysclock.cpp
)
target_include_directories(strmbase PUBLIC baseclasses)
target_compile_definitions(strmbase PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
)
target_link_libraries(strmbase PUBLIC winmm strmiids msdmo)
if(MSVC)
    target_compile_options(strmbase PRIVATE
        /wd4311 /wd4312 /wd4996 /wd4459 /wd4100 /wd4201
        /wd4511 /wd4512 /wd4514 /wd4127 /wd4244
    )
endif()

# ---- Plugin DLL ----
add_library(mpc-be-aliyun-sub SHARED
    src/PluginConfig.h    src/PluginConfig.cpp
    src/AliyunClient.h    src/AliyunClient.cpp
    src/SubtitleOverlay.h  src/SubtitleOverlay.cpp
    src/ConfigDialog.h    src/ConfigDialog.cpp
    src/MainFilter.h      src/MainFilter.cpp
    src/resource.h
    src/resource.rc
    src/exports.def
)
target_include_directories(mpc-be-aliyun-sub PRIVATE src baseclasses)
target_compile_definitions(mpc-be-aliyun-sub PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX   # only for OUR code — keeps std::min/max clean
)
target_link_libraries(mpc-be-aliyun-sub PRIVATE
    strmbase
    CURL::libcurl
    nlohmann_json::nlohmann_json
    strmiids quartz ole32 oleaut32 winmm
)
if(MSVC)
    target_link_options(mpc-be-aliyun-sub PRIVATE
        "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/exports.def"
    )
endif()
set_target_properties(mpc-be-aliyun-sub PROPERTIES PREFIX "")
