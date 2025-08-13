<details>
<summary>[ENG] Click here</summary>

### To be continue.
~~Try translator.~~

</details>

## 这是个啥  
顾名思义，`SrcToA2L` 是用来将源码中的内容转换为A2L文件中标定量、观测量的工具，通常这些内容借助 `Simulink` 模型的 `AUTOSAR` 组件或其他工具直接生成，但某些场合下还是比较麻烦的,借助这个工具，可以简单的将源码中的标记内容处理并生成到A2L中作为观测量、标定量使用；  

`SrcToA2L` 是一个非GUI程序，需要通过终端传入参数进行调用，可以通过将 `SrcToA2L.exe` 所在目录添加到 `PATH` 的方式实现让任意位置运行的终端都可以调用，或根据特定工程需要将`SrcToA2L.exe`复制一份并编写 `bat` 脚本实现一键处理，本工具可以很方便地集成入 `CI/CD` 工作流； 

工具支持结构体类型解析（内嵌结构体成员的结构体除外）、地址匹配、4字节地址对齐、常量宏定义识别、记录布局补全；
## 如何使用

你可以在仓库的Release处找到已经预构建好的可执行文件，也可以自行构建；  

工具需要知道你要将哪些内容生成为标定量，哪些内容生成为观测量，为此你需要按照如下示例将特定的标记段插入原始代码中：  

```c
// .\Test\source_code.c
#include "stdint.h"
#include "typedef.h"

#define VALUE_S 2

/*start_of_calibrations*/
uint8_t c_u8;
uint16_t c_u16;
const volatile float c_float;
/*end_of_calibrations*/

/*start_of_measurements*/
double m_double = 1.23;
uint16_t m_u16[2];

Test_Struct S[VALUE_S];
/*end_of_measurements*/
```
如上，在 `calibrations` 标记段中的内容将被解析为标定量，在 `measurements` 标记段中的内容将会被解析为观测量，它们以注释形式插入源码，不会影响原始代码编译过程；  

在 `Test` 目录下准备了一些测试用的文件，使用终端运行如下指令即可将 `.c`、`.h` 中标记段内容解析为对应的观测量、标定量：

```shell
SrcToA2L.exe .\Test\typedef.h .\Test\source_code.c -r .\Test\Minimal_A2L.a2l -m .\Test\test_map.map
```

其中，紧跟着 `-r` 与 `-m` 后的参数分别指定了要合并写入的A2L文件与用于查找地址的map文件，命令执行完成后会在传入的A2L文件所在目录生成中间件与合并后的A2L文件，但 `-r` 与 `-m` 并不是必要的，你也可以只执行如下命令：

```shell
SrcToA2L.exe .\Test\typedef.h .\Test\source_code.c
```  

这条指令将仅从传入的源文件、头文件中提取需要生成的内容，并将结果输出到当前调用命令行的工作目录下生成的临时中间件 `Middleware.txt`； 

## 指令参数详解

`SrcToA2L` 的行为受传入的指令参数控制，其中 `-X` 形式的参数为控制类型参数，共有以下几种：  

- `-r` 指定为合并写入的参考A2L文件；
- `-m` 指定为用于查找地址的map文件；
- `-a` 指定字节对齐长度（仅为1或2的正整次幂），不指定或参数错误时默认为4字节对齐；  

其它传入的参数视为用于解析的源文件、头文件，A2L与map文件仅传入的最后一个有效；

**`SrcToA2L` 不会破坏任何用于输入的文件，仅以只读模式打开**；  

## 注意事项
本工具为了省略多余操作，除需特定处理的 `calibrations` 与 `measurements` 标记段外，结构体、宏定义、地址匹配等的解析识别是通过全文扫描提取关键字进行的，由于代码风格千奇百怪，解析处理过程有可能会遇到预料之外的BUG（尤其是在生产环境下），为避免发生此类情况发生，**最好事先将用于处理的代码进行格式化**，或特别留意 `define`、`typedef` 等关键字周边的情况；    

如有意外情况发生，可以参考终端日志查看问题发生位置以修改输入文件，或对本工具源码进行修改，构建适应特定环境的版本；  

此外，由于嵌入式平台对类型长度敏感，**本工具仅支持部分`stdint.h`中字节长度固定的标准类型，并将部分常用类型解析为固定长度**，如下所示：
```c++
// Src/Tool_Functions.cpp

typedef enum
{
    TYPE_UNKNOWN, // 未知类型或不支持类型
    STRUCTURE,    // 结构体类型
    UBYTE,        // uint8_t,bool,boolean_t    8 位 1字节
    UWORD,        // uint16_t                  16位 2字节
    ULONG,        // uint32_t                  32位 4字节
    SBYTE,        // int8_t                    8 位 1字节
    SWORD,        // int16_t                   16位 2字节
    SLONG,        // int32_t,int               32位 4字节
    FLOAT32,      // float                     32位 4字节
    FLOAT64,      // double                    64位 8字节
} variable_type_enum;
```
对于结构体，**其所有子成员必须为支持的基础数据类型，且不存在结构体成员**（目前没有支持嵌套结构体的计划，除非我在生产环境遇到了）；  

类型匹配详情可以在以下函数中查看，你也可以修改这个函数以支持更多的类型：  
```c++
// Src/Tool_Functions.cpp

// 解析变量类型
variable_type_enum solve_variable_type(const char *type_str)...
```

对于宏定义，**仅支持正整值直接宏定义，不支持嵌套宏定义**，如下所示：
```c++
#define VALUE_1 100              // √
#define VALUE_2 -2               // X
#define VALUE_3 VALUE_1          // X
```
## 关于构建
本仓库所使用的构建平台及环境为如下:  
OS：Windows11 Professional 24H2 (26100.4652)  
Tool Chain：[mingw-w64\i686-14.2.0-release-win32-dwarf-ucrt-rt_v12-rev2](https://github.com/niXman/mingw-builds-binaries/releases/tag/14.2.0-rt_v12-rev2)  

构建关系由 `Makefile` 组织，使用 `C17`、`C++17` 标准，但由于并未使用高级特性，构建标准可自行调整（按理说 `C98`、`C++98` 应该也行）；  
完善构建环境后，在工作目录下使用 `make`、`mingw32-make` 即可进行构建；

本工具仅使用标准库，完全具备跨平台支持，但在其他平台、环境下需要对 `Makefile` 进行少量修改；  
目前已知使用64位或某些分发版本的编译器时会出现 `size_t` 类型不统一导致的编译警告，但不影响主要功能；
## 主要目录及说明
    Git-Storage        本仓库目录
        ├─.vscode           用于vscode的配置文件
        │   ├─c_cpp_properties.json    VsCode感知引擎配置（需要C/C++插件）
        │   ├─launch.json              VsCode调试运行配置（需要C/C++插件）
        │   └─tasks.json               VsCode预构建脚本
        ├─Inc               头文件
        │   └─(...)
        ├─Src               源文件
        │   └─(...)
        ├─Test              测试用文件
        │   ├─typedef.h                测试的类型定义头文件，含有一个简单的非同成员类型结构体
        │   ├─source_code.c            测试用源文件，含有需要输出的标定量、观测量
        │   ├─Minimal_A2L.a2l          测试用最小体积A2L，CANape等Vector工具可读
        │   └─test_map.map             测试用map文件，由Green Hills编译器生成，只裁剪了测试用例用到的部分
        ├─.gitignore        仓库忽略文件
        └─Makefile          编译构建所用的Makefile

