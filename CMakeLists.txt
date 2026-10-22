cmake_minimum_required(VERSION 3.5)

project(cubebraid_sdk CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# -----------------------------------------------------------------
# 全局统一设置所有编译目标的输出目录
# -----------------------------------------------------------------
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/SDK_Demos")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIR}")

# 头文件包含路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# -----------------------------------------------------------------
# 区分平台的库文件检索路径
# -----------------------------------------------------------------
if(WIN32)
    set(SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/win")
else()
    set(SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux")
endif()

message(STATUS "Current SDK Link Directory: ${SDK_LIB_DIR}")

# 添加全局库文件搜索路径，确保链接器优先去该目录寻找
link_directories(${SDK_LIB_DIR})

# 辅助宏：同时尝试带 lib 前缀和不带 lib 前缀的形式查找
macro(find_sdk_library VAR_NAME LIB_NAME)
    unset(${VAR_NAME} CACHE) # 清除上一次缓存
    find_library(${VAR_NAME}
        NAMES ${LIB_NAME} lib${LIB_NAME}
        PATHS ${SDK_LIB_DIR}
        NO_DEFAULT_PATH
    )
    if(NOT ${VAR_NAME})
        # 若在指定目录没找到，去系统默认路径再找一次
        find_library(${VAR_NAME} NAMES ${LIB_NAME} lib${LIB_NAME})
    endif()

    if(NOT ${VAR_NAME})
        message(FATAL_ERROR "【错误】未找到库文件: ${LIB_NAME}，请检查 ${SDK_LIB_DIR} 目录下是否存在对应文件！")
    else()
        message(STATUS "Found ${LIB_NAME}: ${${VAR_NAME}}")
    endif()
endmacro()

option(USE_OPENCV "Enable OpenCV support" ON)

# ============================================================
# 依赖库查找：OpenCV (可选)
# ============================================================
if(WIN32)
    if(USE_OPENCV)
        find_package(OpenCV REQUIRED)
        if(OpenCV_FOUND)
            message(STATUS "已找到 OpenCV 版本: ${OpenCV_VERSION}")
            include_directories(${OpenCV_INCLUDE_DIRS})
        endif()
    endif()
else()
    if(USE_OPENCV)
        find_package(OpenCV REQUIRED)

        message(STATUS "----------------------------------------")
        message(STATUS "OpenCV")
        message(STATUS "Version : ${OpenCV_VERSION}")
        message(STATUS "----------------------------------------")

        include_directories(${OpenCV_INCLUDE_DIRS})
    endif()
endif()

# 查找 SDK 自身的动态库
find_sdk_library(LIB_LOGGER "LoggerSDK")
find_sdk_library(LIB_PLC "PLC_SDK")
find_sdk_library(LIB_INCLINOMETER "InclinometerSDK")
find_sdk_library(LIB_JSON_PARAM "JsonParameterSDK")
find_sdk_library(LIB_KAWASAKI "KawasakiKinematicsSDK")
find_sdk_library(LIB_ROBOT "RobotSDK")
find_sdk_library(LIB_AGV "AGV_SDK")
find_sdk_library(LIB_CAMERA "CameraSDK")
find_sdk_library(LIB_DLSEG "DlSegSDK")

# Linux 下额外查找第三方依赖库
if(UNIX AND NOT APPLE)
    # 查找 snap7 (解决 TS7Client 缺失问题)
    # find_sdk_library(LIB_SNAP7 "snap7")

    # 查找 snap7：显式列出所有可能的动态库名（解决文件名大小写或带 .so.1 后缀问题）
    find_library(LIB_SNAP7
        NAMES snap7 Snap7 libsnap7 libSnap7 snap7.so.1 libsnap7.so.1
        PATHS ${SDK_LIB_DIR}
        NO_DEFAULT_PATH
    )
    
    # 保底：去系统路径查找
    if(NOT LIB_SNAP7)
        find_library(LIB_SNAP7 NAMES snap7 Snap7 libsnap7 libSnap7 snap7.so.1 libsnap7.so.1)
    endif()

    if(NOT LIB_SNAP7)
        message(FATAL_ERROR "【错误】未在 ${SDK_LIB_DIR} 或系统中找到 Snap7 库（如 libsnap7.so / libSnap7.so / libsnap7.so.1）")
    else()
        message(STATUS "Found Snap7: ${LIB_SNAP7}")
    endif()

    # ============================================================
    # 【核心修复】Linux 下查找 PCL 和 OpenMP 库，解决 CameraSDK 符号丢失问题
    # ============================================================
    find_package(PCL REQUIRED COMPONENTS common io features segmentation)
    find_package(OpenMP REQUIRED)

    if(PCL_FOUND)
        message(STATUS "Found PCL: ${PCL_VERSION}")
        include_directories(${PCL_INCLUDE_DIRS})
        link_directories(${PCL_LIBRARY_DIRS})
        add_definitions(${PCL_DEFINITIONS})
    endif()
endif()

# -----------------------------------------------------------------
# 3. Linux (Ubuntu) 专属设置：动态库运行时搜索路径 (RPATH)
# -----------------------------------------------------------------
if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/linux:$ORIGIN/../lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# -----------------------------------------------------------------
# 4. 构建各个 Demo 目标
# -----------------------------------------------------------------

#### sensor SDK 测试demo
add_executable(sensorsdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/SensorSDK/sensorsdk_demo.cpp)
target_link_libraries(sensorsdk_demo PRIVATE ${LIB_INCLINOMETER})

#### robot SDK 测试demo
add_executable(RobotSDK_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/RobotSDK/RobotSDK_demo.cpp)
target_link_libraries(RobotSDK_demo PRIVATE ${LIB_ROBOT})

#### AGV SDK 测试demo
add_executable(agv_sdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/AGV_SDK/agv_sdk_demo.cpp)
target_link_libraries(agv_sdk_demo PRIVATE ${LIB_AGV})

#### logger SDK 测试demo
add_executable(logger_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/LoggerSDK/logger_demo.cpp)
target_link_libraries(logger_demo PRIVATE ${LIB_LOGGER})

#### JsonParameter SDK 测试demo
add_executable(jsonsdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/JsonSDK/jsonsdk_demo.cpp)
target_link_libraries(jsonsdk_demo PRIVATE ${LIB_JSON_PARAM})

#### Kawasaki robot SDK 测试demo
add_executable(kawasaki_kinematics_sdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/KawasakiSDK/kawasaki_kinematics_sdk_demo.cpp)
target_link_libraries(kawasaki_kinematics_sdk_demo PRIVATE ${LIB_KAWASAKI})

#### Camera SDK 测试demo
add_executable(CameraSDK_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/CameraSDK/CameraSDK_demo.cpp)

if(UNIX AND NOT APPLE)
    # Linux 下需显式补充链接 CameraSDK 隐式依赖的 PCL 以及 OpenMP
    target_link_libraries(CameraSDK_demo PRIVATE ${LIB_CAMERA} ${PCL_LIBRARIES} OpenMP::OpenMP_CXX)
else()
    target_link_libraries(CameraSDK_demo PRIVATE ${LIB_CAMERA})
endif()

#### DlSeg SDK 测试demo
add_executable(dlseg_sdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/DL_SDK/dlseg_sdk_demo.cpp)
if(UNIX AND NOT APPLE)
    target_link_libraries(dlseg_sdk_demo PRIVATE ${LIB_DLSEG} ${OpenCV_LIBS} OpenMP::OpenMP_CXX)
else()
    target_link_libraries(dlseg_sdk_demo PRIVATE ${LIB_DLSEG} ${OpenCV_LIBS})
endif()


#### PLC SDK 测试demo (同时链接 PLC_SDK 与 snap7)
add_executable(plc_sdk_demo ${PROJECT_SOURCE_DIR}/src/CubeBraidSDK/PLC_SDK/plc_sdk_demo.cpp)
if(UNIX AND NOT APPLE)
    target_link_libraries(plc_sdk_demo PRIVATE ${LIB_PLC} ${LIB_SNAP7})
else()
    target_link_libraries(plc_sdk_demo PRIVATE ${LIB_PLC})
endif()