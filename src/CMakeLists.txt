cmake_minimum_required(VERSION 3.10)
project(gtk_hello_world C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)

include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})
add_compile_options(${GTK_CFLAGS_OTHER})

add_executable(file_manager main.c
        utils.c
        utils.h
        ui_builder.c
        ui_builder.h)
target_link_libraries(file_manager ${GTK_LIBRARIES})