# Windows编译说明
## 1. 拉取最新代码
用cmd打开当前项目下命令行终端，拉取最新程序
```bash
git pull
```

## 2. 创建并进入构建目录
```bash
mkdir build
cd build
```

## 3. 生成 Visual Studio 2022 的 64 位 .sln
```bash
cmake -G "Visual Studio 17 2022" -A x64 ..

# 如果使用的是 VS 2019，请运行这一行：
# cmake -G "Visual Studio 16 2019" -A x64 ..
```

## 4. 进入build文件夹打开 cubebraid_sdk.sln 进行demo程序编译

# Ubuntu 编译说明
## 安装依赖
```bash
sudo apt update
sudo apt install -y build-essential cmake
```

### 安装 Protobuf(版本 >= 3.21.0)
#### 源码编译
```bash
# 安装构建依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git libtool autoconf automake pkg-config

# 载 Protobuf 源码及子模块
# 切换到临时工作目录
cd ~

# 克隆源码（以 v21.12 稳定版本为例）
git clone --recursive -b v21.12 https://github.com/protocolbuffers/protobuf.git
cd protobuf

# 使用 CMake 编译与安装
# 创建构建目录
mkdir build && cd build

# 配置 CMake 项目
# -Dprotobuf_BUILD_TESTS=OFF：关闭测试，加快编译过程
# -DCMAKE_BUILD_TYPE=Release：生成 release 优化版本
# -DBUILD_SHARED_LIBS=ON：生成动态链接库 (.so)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -Dprotobuf_BUILD_TESTS=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# 开始编译（-j$(nproc) 使用全核加速）
make -j$(nproc)

# 安装至系统目录 (/usr/local)
sudo make install

# 更新动态链接库缓存
sudo ldconfig

# 验证安装与版本
protoc --version

# 检查可执行文件路径
which protoc
```


## 创建并进入构建目录
```bash
mkdir build
cd build
```
## 编译项目
打开当前项目下命令行终端，拉取最新程序
```bash
git pull
```

```bash
# 用 CMake 生成 Makefile
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译 Demo 程序
# 使用 make 进行编译（-j4 表示使用 4 个 CPU 核心并行编译，可根据实际配置调整）
make -j4

# 运行 Demo 程序
# 进入 Demo 输出目录
cd SDK_Demos

# 运行对应的 Demo（以 logger_demo 为例）
./logger_demo
```


# SDK API说明

## 1. AGV SDK API说明
API接口说明见： [AGV Controller SDK API 接口文档](https://github.com/GJXS1980/cubebraid_sdk/blob/main/doc/AGV_SDK_API_Documentation.md)

## 2. JsonParameter SDK API说明
API接口说明见：[JSONParameter SDK API 接口文档](https://github.com/GJXS1980/cubebraid_sdk/blob/main/doc/JsonParameter_SDK_API_Documentation.md)

## 其它API更新中...