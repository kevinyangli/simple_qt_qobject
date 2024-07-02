cmake_minimum_required(VERSION 3.8)

project(SimpleQObject VERSION 1.0 LANGUAGES CXX)

# 指定 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(simple_qobject_test 
		src/simple_qobject.cpp
		src/simple_qobject.h
		src/simple_qobject_test.cpp)

