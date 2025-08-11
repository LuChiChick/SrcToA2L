#include "Global_Variables.hpp"

extern "C"
{
#include "stdio.h"
}

// 地址对齐长度
size_t addr_alignment_size = DEFAULT_ADDR_ALIGNMENT_SIZE;

// 类型列表
type_node *type_list_head = nullptr;

// 宏定义列表
define_node *define_list_head = nullptr;

// 源文件及头文件列表
file_node *source_and_header_file_list_head = nullptr;

// 文件列表
FILE *input_reference_A2L_file = nullptr;
FILE *input_map_file = nullptr;
FILE *output_target_A2L_file = nullptr;
FILE *output_middleware_file = nullptr;
