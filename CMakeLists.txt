cmake_minimum_required(VERSION 3.16)
project(Observatory VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(Observatory
    main.cpp
    mainwindow.cpp
    mainwindow.h
    observatory.cpp
    observatory.h
)

# Копируем catalog.json в директорию сборки
add_custom_command(TARGET Observatory POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/catalog.json
        $<TARGET_FILE_DIR:Observatory>
)

target_link_libraries(Observatory
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
