# ESP32 IDF 最小工程框架

一个最小可编译的 ESP-IDF 工程模板。

## 目录结构

```
.
├── CMakeLists.txt          # 顶层工程文件
├── sdkconfig.defaults      # 默认配置项
├── main/
│   ├── CMakeLists.txt      # main 组件构建文件
│   └── main.c              # 程序入口 app_main()
└── README.md
```

## 前置条件

安装并加载 ESP-IDF 环境（v5.x），加载后才能使用 `idf.py`：

```bash
. $HOME/esp/esp-idf/export.sh
```

## 常用命令

```bash
# 设置目标芯片（默认 esp32，可选 esp32s3 / esp32c3 等）
idf.py set-target esp32

# 配置（可选）
idf.py menuconfig

# 编译
idf.py build

# 烧录并查看串口日志（按实际端口替换 /dev/ttyUSB0）
idf.py -p /dev/ttyUSB0 flash monitor
```
