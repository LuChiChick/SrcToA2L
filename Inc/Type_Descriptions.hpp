#ifndef __TYPE_DESCRIPTIONS_HPP__
#define __TYPE_DESCRIPTIONS_HPP__

extern "C"
{
#include "stdint.h"
}

#include "Config.hpp"

// 宏定义链表
typedef struct define_list_node_struct
{
    char define_str[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'}; // define 名串
    unsigned int value = 0;
    define_list_node_struct *p_next = nullptr;
} define_node;

// 文件链表
typedef struct file_list_node_struct
{
    char *file_name_str = nullptr;
    file_list_node_struct *p_next = nullptr;
} file_node;

// 变量类型枚举
typedef enum
{
    TYPE_UNKNOWN, // 未知类型或不支持类型
    STRUCTURE,    // 结构体类型
    UBYTE,        // uint8_t,bool
    UWORD,        // uint16_t
    ULONG,        // uint32_t
    SBYTE,        // int8_t
    SWORD,        // int16_t
    SLONG,        // int32_t
    FLOAT32,      // float
    FLOAT64,      // double
} variable_type_enum;

// 变量信息结构体
typedef struct
{
    char name_str[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'};      // 变量名
    char type_name_str[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'}; // 类型名串
    size_t element_count = 0;                                  // 子元素计数(仅数组>1,单个变量该值为1,未解析或解析失败为0)
    variable_type_enum type = TYPE_UNKNOWN;                    // 变量类型
} variable_info;

// 元素信息记录链表
typedef struct sub_element_list_node_struct
{
    variable_info element_info;                            // 元素信息
    struct sub_element_list_node_struct *p_next = nullptr; // 链表指针
} sub_element_node;

// 复合类型记录链表
typedef struct type_list_node_struct
{
    char type_name_str[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'}; // 类型名
    sub_element_node *element_list_head = nullptr;             // 子成员链表
    variable_type_enum type = TYPE_UNKNOWN;                    // 类型标记
    struct type_list_node_struct *p_next = nullptr;            // 链表指针
} type_node;

// 日志类型记录
typedef enum
{
    LOG_INFO,     // 常规信息类型
    LOG_SYS_INFO, // 系统消息类型
    LOG_SUCCESS,  // 成功类型
    LOG_FAILURE,  // 失败类型
    LOG_WARN,     // 警告类型
} log_type_enum;
#endif