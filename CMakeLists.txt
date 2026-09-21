cmake_minimum_required(VERSION 3.16)
project(todo-list LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt5 ---
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Test)

# --- Исходники ---
set(SOURCES
    sources/task.cpp
    sources/todolistapp.cpp
    sources/edittaskdialog.cpp
)

set(HEADERS
    headers/task.h
    headers/todolistapp.h
    headers/edittaskdialog.h
)

# --- Основное приложение ---
add_executable(todo-list
    main.cpp
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(todo-list PRIVATE headers)
target_link_libraries(todo-list PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

# --- Тесты ---
enable_testing()

add_executable(test_task
    tests/test_task.cpp
    sources/task.cpp
    headers/task.h
)

target_include_directories(test_task PRIVATE headers)
target_link_libraries(test_task PRIVATE Qt5::Core Qt5::Gui Qt5::Test)

add_test(NAME test_task COMMAND test_task)
