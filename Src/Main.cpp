extern "C"
{
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
}

#include "Config.hpp"
#include "Tool_Functions.hpp"
#include "Global_Variables.hpp"
#include "Core_Functions.hpp"

int main(int argc, char *argv[])
{
    // 处理输入部分
    solve_args(argc, argv);

    printf("\n\n");

    // 进行宏定义解析
    solve_defines();

    printf("\n\n");

    // 进行类型解析
    solve_types();

    // 处理中间件
    solve_middleware();

    // 处理最终输出
    solve_A2L_output();

    print_log(LOG_SUCCESS, "Done.\nVer0.3");
    return 0;
}