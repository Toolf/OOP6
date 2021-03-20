cmake_minimum_required (VERSION 3.20)

project (CUSTOM_ALLOCATOR C)

set(CMAKE_C_STANDARD 11)


include_directories("include/")


set(SOURCES src/main.c src/allocator.c src/rb_tree.c src/arena.c src/header.c)

IF (WIN32)
    list(APPEND SOURCES src/win_kernal.c)
ELSE(UNIX)
    list(APPEND SOURCES src/unix_kernal.c)
ENDIF()

add_executable(main ${SOURCES})
