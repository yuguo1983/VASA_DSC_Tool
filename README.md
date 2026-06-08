# VASA_DSC_Tool

VASA DSC Tool — 基于 VESA DSC（Display Stream Compression）1.2 标准的参考编解码工具（Reference Codec GUI），提供图形化操作界面和命令行两种使用方式。

---

## 目录结构

```
VASA_DSC_Tool/
├── README.md                 # 本文件（操作说明）
├── DSC_GUI.pro               # Qt 项目文件
├── main.cpp                  # Qt 入口
├── mainwindow.h / .cpp       # 主窗口（GUI 逻辑）
├── dsc_engine.h / .cpp       # 引擎封装（调用 C 编解码器）
├── cfg/                      # 配置文件与测试数据
│   ├── *.cfg                 #   DSC 速率控制（RC）参数文件
│   ├── *.dpx                 #   DPX 测试图像
│   ├── test.cfg              #   示例配置文件
│   ├── test_list.txt         #   文件列表
│   └── README.TXT            #   原始文档（英文）
└── src/                      # DSC 1.2 核心算法（C 语言）
    ├── codec_main.c          #   主入口
    ├── dsc_codec.c / .h      #   DSC 编解码核心
    ├── dsc_types.h           #   类型定义
    ├── dsc_utils.c / .h      #   工具函数
    ├── cmd_parse.c / .h      #   参数解析
    ├── dpx.c / .h            #   DPX 文件读写
    ├── fifo.c / .h           #   FIFO 缓冲区
    ├── logging.c / .h        #   日志
    ├── multiplex.c / .h      #   复用
    ├── psnr.c / .h           #   PSNR 计算
    ├── utl.c / .h            #   通用工具
    └── vdo.h                 #   视频数据类型
```

---

## 系统要求

- **操作系统**：Windows（Win32/Win64，已编译）或 Linux/macOS
- **构建工具**：Qt 5.15+（含 `core`、`gui`、`widgets` 模块）
- **编译器**：支持 C++17 和 C99 的编译器（MSVC / GCC / Clang）
- **已编译的可执行文件**：`release/DSC_GUI.exe`

### 构建方法

```bash
# 在 Qt Creator 中打开 DSC_GUI.pro，或命令行：
mkdir build && cd build
qmake ../DSC_GUI.pro
make          # Linux/macOS
nmake         # Windows (MSVC)
```

---

## 操作说明

启动后主窗口标题为 **DSC 1.2 Reference Codec**，窗口分为 **5 个标签页**（Tab）：File I/O、Codec、Format、Rate Control、Run / Log。

---

### 1. File I/O（文件输入输出）

| 字段 | 说明 |
|---|---|
| **SRC_DIR** | 输入目录，存放待编码的图像文件（支持 `DPX`、`PPM`、`BMP`、`YUV` 格式） |
| **Output Directory** | 输出目录，生成的结果文件（.dsc / .dpx / .bmp 等）存放位置 |

操作步骤：
1. 点击 **Browse...** 选择输入目录。
2. 点击 **Browse...** 选择输出目录（不指定则输出到程序所在目录）。

> **输入格式说明**：程序会自动扫描输入目录中以下格式的文件：
> - **DPX** (`.dpx`) — 主格式，支持 8/10/12/16 bpc
> - **BMP** (`.bmp`) — 仅支持 **24-bit 无压缩** 的标准 BMP（不支持 32-bit Alpha、RLE 压缩等变体），读取后转为 8-bit RGB
> - **PPM** (`.ppm`) — 简单图像格式
> - **YUV** (`.yuv`) — 需在 Codec 标签页指定 PIC_WIDTH / PIC_HEIGHT

---

### 2. Codec（编解码参数）

#### FUNCTION（工作模式）

| 选项 | 说明 |
|---|---|
| **0 — Encode + Decode** | 编码后立即解码，验证压缩正确性，**不保留 .dsc 文件** |
| **1 — Encode Only** | 仅编码，生成 `.dsc` 码流文件（需指定 RC 配置文件） |
| **2 — Decode Only** | 仅解码 `.dsc` 文件为图像 |

> 选择 Decode Only 时，编码相关参数（BPC、BPP、Slice 等）会自动禁用。

#### 核心参数

| 参数 | 范围 | 默认值 | 说明 |
|---|---|---|---|
| **BITS_PER_COMPONENT** | 8 ~ 16 | 8 | 每分量位数（bpc） |
| **BITS_PER_PIXEL** | 4.0 ~ 15.0 | 12.0 | 目标码率（bpp），步进 0.5 |
| **PIC_WIDTH** | 0 ~ 65535 | 0 | 图像宽度（0 = 自动从输入获取） |
| **PIC_HEIGHT** | 0 ~ 65535 | 0 | 图像高度（0 = 自动从输入获取） |
| **SLICE_WIDTH** | 0 ~ 65535 | 0 | 切片宽度（0 = 全宽） |
| **SLICE_HEIGHT** | 0 ~ 65535 | 0 | 切片高度（0 = 全高） |
| **LINE_BUFFER_BPC** | 8 ~ 16 | 16 | 行缓冲位数 |

#### Chroma Format（色彩格式）

| 选项 | 说明 |
|---|---|
| **RGB** | 编码为 RGB |
| **YCbCr 4:4:4** | 全采样 YCbCr |
| **YCbCr 4:2:2 (Simple)** | 简单 4:2:2 下采样（需 USE_YUV_INPUT） |
| **YCbCr 4:2:2 (Native)** | 原生 4:2:2 模式（需 NATIVE_422） |
| **YCbCr 4:2:0 (Native)** | 原生 4:2:0 模式（需 NATIVE_420） |

#### 开关选项

| 选项 | 说明 |
|---|---|
| **BLOCK_PRED_ENABLE** | 启用块预测（默认开启，可提升病理图像质量） |
| **VBR_ENABLE** | 可变码率模式 |
| **USE_YUV_INPUT** | 以 YCbCr 而非 RGB 编码 |
| **SIMPLE_422** | 简单 4:2:2 模式 |
| **NATIVE_422** | 原生 4:2:2 模式 |
| **NATIVE_420** | 原生 4:2:0 模式 |

---

### 3. Format（文件格式）

#### DPX Read（DPX 读取）

| 参数 | 说明 |
|---|---|
| **DPXR_PAD_ENDS** | 读取时填充数据位末端（0/1，默认 1） |
| **DPXR_DATUM_ORDER** | 读取时数据顺序（0/1，默认 1） |
| **DPXR_FORCE_BE** | 强制大端读取（0/1，默认 0） |

#### DPX Write（DPX 写入）

| 参数 | 说明 |
|---|---|
| **DPXW_PAD_ENDS** | 写入时填充数据位末端（0/1，默认 1） |
| **DPXW_DATUM_ORDER** | 写入时数据顺序（0/1，默认 1） |
| **DPXW_FORCE_PACKING** | 强制打包模式（0/1，默认 1） |

#### 输出文件格式

| 选项 | 说明 |
|---|---|
| **SWAP_R_AND_B** | 交换输入 R/B 通道 |
| **SWAP_R_AND_B_OUT** | 交换输出 R/B 通道 |
| **PPM_FILE_OUTPUT** | 输出 PPM 格式 |
| **DPX_FILE_OUTPUT** | 输出 DPX 格式（默认开启） |
| **BMP_FILE_OUTPUT** | 输出 BMP 格式 |
| **BMP_DSC_OUTPUT** | 将 DSC 码流嵌入 BMP 文件 |
| **YUV_FILE_OUTPUT** | 输出 YUV 格式 |
| **PRINT_PPS** | 在日志中打印 PPS（Picture Parameter Set） |

---

### 4. Rate Control（码率控制）

| 参数 | 范围 | 默认值 | 说明 |
|---|---|---|---|
| **INCLUDE (RC .cfg file)** | — | — | 外部 RC 参数文件（可选，最后加载可覆盖上方值） |
| **RC_MODEL_SIZE** | 64 ~ 4096 | 640 | 码率模型大小 |
| **INITIAL_XMIT_DELAY** | 0 ~ 1023 | 112 | 初始发送延迟 |
| **INITIAL_DEC_DELAY** | 0 ~ 1023 | 128 | 初始解码延迟（对应 INITIAL_FULLNESS_OFFSET） |
| **RC_EDGE_FACTOR** | 0 ~ 15 | 4 | 边缘因子 |
| **RC_QUANT_INCR_LIMIT0** | 0 ~ 31 | 7 | 量化增量上限 0 |
| **RC_QUANT_INCR_LIMIT1** | 0 ~ 31 | 4 | 量化增量上限 1 |

`cfg/` 目录中提供了适用于不同 bpc/bpp 组合的预置 RC 配置文件（如 `rc_10bpc_8bpp.cfg`），可通过 **Browse...** 按钮加载。

---

### 5. Run / Log（运行与日志）

- **Run** 按钮（绿色 ▶） — 启动编解码任务
- **Stop** 按钮（红色 ■） — 请求停止（注意：编解码运行在同一进程中，无法安全中断）
- 日志窗口以 `Consolas` 等宽字体显示实时输出，包括各帧的 PSNR、码率等统计信息

> 运行前系统会自动校验：
> - 必须选择输入目录（SRC_DIR）
> - 仅编码模式（FUNCTION=1）必须指定 RC 配置文件

---

### 菜单栏

| 菜单项 | 快捷键 | 说明 |
|---|---|---|
| **File → New Config** | Ctrl+N | 恢复所有参数为默认值 |
| **File → Open Config...** | Ctrl+O | 加载已有的 `.cfg` 配置文件 |
| **File → Save Config** | Ctrl+S | 保存当前参数到配置文件 |
| **File → Save Config As...** | Ctrl+Shift+S | 另存为新的配置文件 |
| **File → Exit** | Ctrl+Q | 退出程序 |

---

## 配置文件（.cfg）格式说明

配置文件为纯文本格式，每行一个 `KEY VALUE` 对，支持 `//` 或 `#` 注释。示例如下：

```ini
// DSC 1.2 Codec Configuration
FUNCTION            0
SRC_DIR             C:/images/input
OUT_DIR             C:/images/output
BITS_PER_COMPONENT  10
BITS_PER_PIXEL      8.00
PIC_WIDTH           1920
PIC_HEIGHT          1080
SLICE_WIDTH         0
SLICE_HEIGHT        0
LINE_BUFFER_BPC     16
USE_YUV_INPUT       1
BLOCK_PRED_ENABLE   1
VBR_ENABLE          0
SIMPLE_422          0
NATIVE_422          0
NATIVE_420          0

// DPX read
DPXR_PAD_ENDS       1
DPXR_DATUM_ORDER    1
DPXR_FORCE_BE       0
// DPX write
DPXW_PAD_ENDS       1
DPXW_DATUM_ORDER    1
DPXW_FORCE_PACKING  1
SWAP_R_AND_B        1
SWAP_R_AND_B_OUT    1

PPM_FILE_OUTPUT     0
DPX_FILE_OUTPUT     1
BMP_FILE_OUTPUT     0
BMP_DSC_OUTPUT      0
YUV_FILE_OUTPUT     0

RC_MODEL_SIZE          640
INITIAL_DELAY          112
INITIAL_FULLNESS_OFFSET 128
RC_EDGE_FACTOR         4
RC_QUANT_INCR_LIMIT0   7
RC_QUANT_INCR_LIMIT1   4

// 外部 RC 文件（可选，将覆盖上面的 RC 参数）
INCLUDE              cfg/rc_10bpc_8bpp.cfg
```

> **注意**：`INCLUDE` 必须放在文件末尾，因为它将覆盖之前设置的 RC 参数。

### 常用预置 RC 配置文件

`cfg/` 目录下按 `rc_{bpc}bpc_{bpp}bpp[_420][_422].cfg` 命名：
- `rc_8bpc_6bpp.cfg` — 8bpc 6bpp
- `rc_10bpc_8bpp.cfg` — 10bpc 8bpp
- `rc_12bpc_12bpp.cfg` — 12bpc 12bpp
- 以及对应的 4:2:0 / 4:2:2 变体

---

## 命令行用法

程序也可脱离 GUI 以命令行模式运行：

```bash
DSC_GUI -F <配置文件路径>
# 或直接调用底层 codec：
DSC -F <配置文件路径>
```

参数：
- `-F` — 指定配置文件路径
- 引擎内部通过 `parse_line` 解析配置文件中的 `KEY VALUE` 对

---

## 输出文件说明

运行结束后，输出目录中可能产生以下文件：

| 文件 | 说明 |
|---|---|
| `<name>.ref.dpx` | 编码器输入的参考图像（DPX 格式） |
| `<name>.out.dpx` | 解码器输出的重建图像 |
| `<name>.dsc` | DSC 压缩码流（仅 FUNCTION=1 时生成） |
| `<name>.bmp` | BMP 格式输出（若开启 BMP_FILE_OUTPUT） |
| `<name>.ppm` | PPM 格式输出（若开启 PPM_FILE_OUTPUT） |
| `<name>.yuv` | YUV 格式输出（若开启 YUV_FILE_OUTPUT） |
| `log.txt` | 详细日志（含 PSNR 统计） |

PSNR 信息会同时显示在 **Run / Log** 标签页的日志窗口中。

---

## 测试数据

`cfg/` 目录包含：
- **测试图像**：`TEST.dpx`、`test_picture1.dpx`、`test_picture2.dpx`、`some_other_test.dpx`
- **配置文件**：`test.cfg`、`test_dsc_1_1.cfg`、`420mode.cfg` 等
- **文件列表**：`test_list.txt`
- **变更记录**：`CHANGELOG.TXT`

### 快速测试流程

1. 启动程序
2. **File I/O** 标签页：SRC_DIR 选择 `cfg/` 目录，Output Directory 选择一个空目录
3. **Codec** 标签页：FUNCTION 保持 `0 - Encode + Decode`
4. **Rate Control** 标签页：点击 INCLUDE 的 Browse，选择 `cfg/rc_8bpc_6bpp.cfg`
5. **Run / Log** 标签页：点击 **Run**

---

## 许可说明

本代码基于 Broadcom Corporation 贡献给 VESA 的源代码，用于实现 VESA Display Stream Compression（DSC）1.2 标准。使用时请遵守原代码的许可条款，详见源文件头部声明。
