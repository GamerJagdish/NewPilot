cmake_minimum_required(VERSION 3.15)
project(NewPilot LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(NewPilot WIN32
    src/main.cpp
    src/config.h
    src/config.cpp
    src/action_runner.h
    src/action_runner.cpp
    src/app_enumerator.h
    src/app_enumerator.cpp
    src/settings_ui.h
    src/settings_ui.cpp
    src/tray_icon.h
    src/tray_icon.cpp
    src/resource.h
    src/resources.rc
)

if (MSVC)
    set_property(TARGET NewPilot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(NewPilot PRIVATE /O2 /GL /Gw /Gy)
    target_link_options(NewPilot PRIVATE /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS)
endif()

target_link_libraries(NewPilot PRIVATE
    comctl32
    shlwapi
    ole32
    shell32
    user32
    advapi32
)
