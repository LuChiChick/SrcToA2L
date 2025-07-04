#ifndef __CORE_FUNCTIONS_HPP__
#define __CORE_FUNCTIONS_HPP__

#include "Global_Variables.hpp"
#include "Config.hpp"

// 参数解析
void solve_args(int argc, char *argv[]);

// 解析宏定义
void solve_defines(void);

// 类型解析
void solve_types(void);

// 处理中间件
void solve_middleware(void);

// 处理最终A2L输出
void solve_A2L_output(void);

#endif