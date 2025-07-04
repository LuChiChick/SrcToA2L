#include "Global_Variables.hpp"

extern "C"
{
#include "stdio.h"
}

// 类型列表
type_node *type_list_head = nullptr;

// 宏定义列表
define_node *define_list_head = nullptr;

// 源文件列表
file_node *source_file_list_head = nullptr;

// 文件列表
FILE *input_reference_A2L_file = nullptr;
FILE *input_map_file = nullptr;
FILE *output_target_A2L_file = nullptr;
FILE *output_middleware_file = nullptr;
