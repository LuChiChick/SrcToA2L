#ifndef __TOOL_FUNCTIONS_HPP__
#define __TOOL_FUNCTIONS_HPP__

extern "C"
{
#include "stdint.h"
#include "stdio.h"
}

#include "Type_Descriptions.hpp"

// 读取文件一行行
error_type_enum f_getline(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len = nullptr);

// 读取下一个有效词组
error_type_enum f_getword(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len = nullptr);

// 获取文件代码行(以;为分界的代码逻辑行，忽略中途的注释)
error_type_enum f_get_codeline(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len = nullptr);

// 前进到下一行
size_t f_seek_nextline(FILE *file);

// 跳转到下一个非空字符
size_t f_seek_skip_blank(FILE *file);

// 基础变量类型解析
variable_type_enum get_variable_base_type(const char *str);

// 基础变量解析
variable_info solve_base_variable(const char *str);

// 解析变量类型
variable_type_enum get_variable_type(const char *str);

// 变量解析
variable_info solve_variable(const char *str);

// 日志输出
void print_log(log_type_enum log_type, const char *p_format_str, ...);

#endif