cmake_minimum_required(VERSION 3.10)
project(SysPilot VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(syspilot
    src/monitor.c
    src/analyzer.c
    src/scheduler.c
    src/main.c
)

if(UNIX)
    target_link_libraries(syspilot m)
endif()
