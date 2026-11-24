cmake_minimum_required(VERSION 3.24)
project(iRx LANGUAGES C CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "iRx Windows builds require the Microsoft Visual C++ compiler.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "iRx currently targets Win32 (x86). Configure with -A Win32.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

find_package(ZLIB REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(unofficial-enet CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)

set(IRX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source/src")

file(GLOB IRX_CLIENT_SOURCES CONFIGURE_DEPENDS
    "${IRX_SOURCE_DIR}/*.cpp"
    "${IRX_SOURCE_DIR}/bot/*.cpp"
    "${IRX_SOURCE_DIR}/discord/*.cpp"
)

list(FILTER IRX_CLIENT_SOURCES EXCLUDE REGEX "/master\\.cpp$")

add_executable(iRx WIN32 ${IRX_CLIENT_SOURCES})

target_include_directories(iRx PRIVATE
    "${IRX_SOURCE_DIR}"
    "${IRX_SOURCE_DIR}/bot"
    "${IRX_SOURCE_DIR}/discord"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
)

target_compile_definitions(iRx PRIVATE
    WIN32
    _WIN32_WINNT=0x0601
    WINVER=0x0601
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
)

target_compile_options(iRx PRIVATE
    /W3
    /EHsc
    /permissive-
    /Zc:__cplusplus
    /wd4018
    /wd4244
    /wd4267
    /wd4305
    /wd4996
)

target_link_libraries(iRx PRIVATE
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
    unofficial::enet::enet
    OpenAL::OpenAL
    Vorbis::vorbisfile
    ws2_32
    winmm
    winhttp
    opengl32
    shell32
    advapi32
    dbghelp
    user32
    gdi32
    imm32
    version
    setupapi
    ole32
    oleaut32
    uuid
)

set_target_properties(iRx PROPERTIES
    OUTPUT_NAME "iRx"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/out"
)

set(VCPKG_RUNTIME_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
file(GLOB VCPKG_RUNTIME_DLLS "${VCPKG_RUNTIME_DIR}/*.dll")
if(VCPKG_RUNTIME_DLLS)
    add_custom_command(TARGET iRx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${VCPKG_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:iRx>"
        COMMAND_EXPAND_LISTS
    )
endif()

message(STATUS "iRx compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "iRx architecture: Win32 / x86")
message(STATUS "iRx dependencies: vcpkg manifest")
