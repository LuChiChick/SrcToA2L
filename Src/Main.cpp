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
    printf("\n\n");
    log_printf(LOG_SYS_INFO, "SrcToA2L Ver1.1");
    log_printf(LOG_SYS_INFO, "Auther: LuChiChick");
    log_printf(LOG_SYS_INFO, "%s\n%s\n%s\n\n", "Open source links:",
               "                              ├─Github:              https://git.luchichick.cn/LuChiChick/SrcToA2L",
               "                              └─Personal Git System: https://git.luchichick.cn/LuChiChick/SrcToA2L");

    log_printf(LOG_SYS_INFO, "Start argument solve.\n\n");

    // 处理输入部分
    solve_args(argc, argv);

    // 进行宏定义解析
    printf("\n\n");
    log_printf(LOG_SYS_INFO, "Start constant value definition solve.\n\n");
    solve_defines();

    // 进行类型解析
    printf("\n\n");
    log_printf(LOG_SYS_INFO, "Start compound type definition solve.\n\n");
    solve_types();

    // 处理中间件
    printf("\n\n");
    log_printf(LOG_SYS_INFO, "Start calibration and measurement solve.\n\n");
    solve_middleware();

    // 处理最终输出
    if (input_reference_A2L_file != nullptr)
    {
        printf("\n\n");
        log_printf(LOG_SYS_INFO, "Start merging middleware into reference A2L file.\n\n");
        solve_A2L_output();
    }

    log_printf(LOG_SYS_INFO, "Done.");
    clean_and_exit(0);
    return 0;
}
