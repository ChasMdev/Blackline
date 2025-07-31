cmake_minimum_required(VERSION 3.31)
project(Injector)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_VS_PLATFORM_NAME "x64")

add_executable(Injector main.cpp
        Update.h
        main.h
        encryption.cpp
        ICHooker.asm
        ICHooker.h
        ICHooker.obj
        Injection.cpp
        Injection.h
        json.hpp
        ntdlldefs.cpp
        ntdlldefs.h
        Roblox.h
        SyscalCaller.h
        SyscallCaller.asm
        syscallcaller.obj
        ThreadPool.h
        Tools.cpp
        Tools.h
        Tools_.asm
        Tools_.obj
        TPInjection.cpp
        TPInjection.h
        WorkerFactory.h
        global.h
        Utils.cpp
        Utils.h)

add_custom_command(TARGET Injector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/../Testing
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Injector> ${CMAKE_SOURCE_DIR}/../Testing/
        COMMENT "Creating Testing directory and copying Injector"
)
