cmake_minimum_required(VERSION 3.20)
project(MPCBEAliyunSub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dependencies ----
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# ---- Compile definitions for the whole project ----
add_compile_definitions(
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0601
)

# ---- DirectShow baseclasses (static lib) ----
# Include all .cpp files except those using deprecated DirectDraw / unnecessary components
add_library(strmbase STATIC
    # Core
    baseclasses/combase.cpp
    baseclasses/wxdebug.cpp
    baseclasses/wxutil.cpp
    baseclasses/wxlist.cpp
    baseclasses/winutil.cpp
    baseclasses/ctlutil.cpp
    # Filter base
    baseclasses/amfilter.cpp
    baseclasses/transfrm.cpp
    baseclasses/mtype.cpp
    # Output
    baseclasses/outputq.cpp
    # COM / DLL
    baseclasses/dllentry.cpp
    baseclasses/dllsetup.cpp
    baseclasses/pstream.cpp
    # Clock
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
    src/exports.def
)
target_include_directories(mpc-be-aliyun-sub PRIVATE src baseclasses)
target_compile_definitions(mpc-be-aliyun-sub PRIVATE
    _CRT_SECURE_NO_WARNINGS
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
