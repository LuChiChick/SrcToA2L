#ifndef __TOOL_FUNCTIONS_HPP__
#define __TOOL_FUNCTIONS_HPP__

extern "C"
{
#include "stdint.h"
#include "stdio.h"
}

#include "Type_Descriptions.hpp"

// 读取文件一行
size_t f_getline(FILE *file, char *buffer, const size_t buffer_len);

// 读取下一个有效词组
size_t f_getword(FILE *file, char *buffer, const size_t buffer_len);

// 获取文件代码行(以;为分界的代码逻辑行，忽略中途的注释)
size_t f_get_codeline(FILE *file, char *buffer, const size_t buffer_len);

// 前进到下一行
size_t f_seek_nextline(FILE *file);

// 跳转到下一个非空字符
size_t f_seek_skip_blanks(FILE *file);

// 跳过注释和空白内容(不跳过识别段)
size_t f_seek_skip_comments_and_blanks(FILE *file);

// 解析变量类型
variable_type_enum solve_variable_type(const char *type_str);

// 变量解析
variable_info solve_variable_info(const char *code_line_str);

// 获取变量地址
uint32_t get_variable_addr32(const char *v_name_str);

// 输出标定量
void f_print_calibration(FILE *file, variable_info v_info);

// 输出观测量
void f_print_measurement(FILE *file, variable_info v_info);

// 日志输出
void log_printf(log_type_enum log_type, const char *p_format_str, ...);

// 清理和退出
void clean_and_exit(int exit_code);

#endif