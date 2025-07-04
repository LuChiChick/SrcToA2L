#ifndef __GLOBAL_VARIABLES_HPP__
#define __GLOBAL_VARIABLES_HPP__

#include "Type_Descriptions.hpp"
extern "C"
{
#include "stdio.h"
}

// 类型列表
extern type_node *type_list_head;

// 宏定义列表
extern define_node *define_list_head;

// 源文件列表
extern file_node *source_file_list_head;

// 文件指针
extern FILE *input_reference_A2L_file;
extern FILE *input_map_file;
extern FILE *output_target_A2L_file;
extern FILE *output_middleware_file;

#endif