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

// 记录布局解析（标定量使用）
void solve_record_layout(void);

// 处理标定量和观测量
void solve_calibrations_and_measurements(void);

// 处理最终A2L合并输出
void solve_A2L_merge(void);

#endif