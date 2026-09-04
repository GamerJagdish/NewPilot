cmake_minimum_required(VERSION 3.15)
project(NewPilotSetup LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(NewPilotSetup WIN32
    src/main.cpp
    src/resource.h
    src/resources.rc
    src/installer.manifest
)

set_target_properties(NewPilotSetup PROPERTIES
    OUTPUT_NAME "NewPilot-Setup"
)

if (MSVC)
    set_property(TARGET NewPilotSetup PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(NewPilotSetup PRIVATE /O2 /GL /Gw /Gy)
    target_link_options(NewPilotSetup PRIVATE 
        /LTCG 
        /OPT:REF 
        /OPT:ICF 
        /SUBSYSTEM:WINDOWS
        /MANIFESTUAC:NO
    )
endif()

target_link_libraries(NewPilotSetup PRIVATE
    crypt32
    shell32
    advapi32
    user32
    ole32
    comctl32
)
