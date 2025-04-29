cmake_minimum_required(VERSION 3.10)
project(DocPool)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加包含路径（让编译器找到 log_system 的头文件）
include_directories(log_system)

# 添加可执行文件（直接指定 test.cpp 路径）
add_executable(test_logsystem
    log_system/examples/test.cpp
)

# 链接库（根据你的代码需要）
target_link_libraries(test_logsystem
    pthread
    jsoncpp
)

# 设置输出目录（非必须，默认会生成在 build 目录）
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})