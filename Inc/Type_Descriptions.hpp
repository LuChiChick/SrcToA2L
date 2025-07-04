#ifndef __TYPE_DESCRIPTIONS_HPP__
#define __TYPE_DESCRIPTIONS_HPP__

extern "C"
{
#include "stdint.h"
}

#include "Config.hpp"

// 宏定义链表
typedef struct define_node_struct
{
    char define_str[VARIABLE_NAME_LENGTH_MAX] = {'\0'};  // define 名串
    char context_str[VARIABLE_NAME_LENGTH_MAX] = {'\0'}; // define 内容串
    define_node_struct *p_next = nullptr;
} define_node;

// 文件链表
typedef struct file_node_struct
{
    char *file_name_str = nullptr;
    file_node_struct *p_next = nullptr;
} file_node;

// 错误类型枚举
typedef enum
{
    ERROR_NONE,                 // 无错误
    ERROR_OUT_OF_LENGTH,        // 超出长度
    ERROR_ILLEGAL_POINTER,      // 非法指针
    ERROR_ILLEGAL_WORD_SECTION, // 非法词段
    ERROR_END_OF_FILE,          // 文件结尾
} error_type_enum;

// 变量类型枚举
typedef enum
{
    TYPE_UNKNOWN,       // 未知类型
    TYPE_NOT_SUPPORTED, // 不支持的类型
    STRUCTURE,          // 结构体类型
    UBYTE,              // uint8_t,bool
    UWORD,              // uint16_t
    ULONG,              // uint32_t
    SBYTE,              // int8_t
    SWORD,              // int16_t
    SLONG,              // int32_t
    FLOAT32,            // float
    FLOAT64,            // double
} variable_type_enum;

// 变量信息结构体
typedef struct
{
    char name_str[VARIABLE_NAME_LENGTH_MAX] = {'\0'};          // 变量名
    const char *type_name_str = nullptr;                       // 变量类型字符串
    size_t element_count = 0;                                  // 子元素计数(仅数组>1,单个变量该值为1)
    size_t single_element_size = 0;                            // 单个子元素大小(单位字节)
    uint32_t start_addr_32 = 0x00000000;                       // 变量起始地址(32位地址,4字节)
    variable_type_enum type = TYPE_NOT_SUPPORTED;              // 变量类型
    char A2L_type_str[A2L_TYPE_STR_LENGTH_MAX] = {'\0'};       // 类型字符串
    char A2L_max_limit_str[A2L_LIMIT_STR_LENGTH_MAX] = {'\0'}; // 上限字符串
    char A2L_min_limit_str[A2L_LIMIT_STR_LENGTH_MAX] = {'\0'}; // 下限字符串
} variable_info;

// 元素信息记录链表
typedef struct sub_element_node_struct
{
    variable_info element_info;                       // 元素信息
    struct sub_element_node_struct *p_next = nullptr; // 链表指针
} sub_element_node;

// 复合类型记录链表
typedef struct type_node_struct
{
    char type_name_str[TYPE_NAME_LENGTH_MAX] = {'\0'}; // 类型名
    sub_element_node *element_list_head = nullptr;     // 子成员链表
    variable_type_enum type = TYPE_UNKNOWN;            // 类型标记
    struct type_node_struct *p_next = nullptr;         // 链表指针
} type_node;

// 日志类型记录
typedef enum
{
    LOG_NORMAL,  // 常规类型
    LOG_SUCCESS, // 成功类型
    LOG_FAILURE, // 失败类型
    LOG_ERROR,   // 错误类型
    LOG_WARN,    // 警告类型
} log_type_enum;
#endif