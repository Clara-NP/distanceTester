# 顶层工程 CMake 文件，所有 ESP-IDF 工程都需要
cmake_minimum_required(VERSION 3.16)

# 引入 ESP-IDF 构建系统
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 定义工程名（生成的固件名为 ${PROJECT_NAME}.bin）
project(test_app)
