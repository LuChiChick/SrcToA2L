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
    log_printf(LOG_SYS_INFO, "SrcToA2L Ver1.5");
    log_printf(LOG_SYS_INFO, "Auther: LuChiChick");
    log_printf(LOG_SYS_INFO, "%s\n%s\n%s\n\n", "Open source links:",
               "                              ├─Github:              https://github.com/LuChiChick/SrcToA2L",
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

    // 中间件处理
    {
        // 处理记录布局
        printf("\n\n");
        log_printf(LOG_SYS_INFO, "Start record layout solve.\n\n");
        solve_record_layout();

        // 处理标定量和观测量
        printf("\n\n");
        log_printf(LOG_SYS_INFO, "Start calibrations and measurements solve.\n\n");
        solve_calibrations_and_measurements();
    }

    // 处理最终输出
    if (input_reference_A2L_file != nullptr)
    {
        printf("\n\n");
        log_printf(LOG_SYS_INFO, "Start merging middleware into reference A2L file.\n\n");
        solve_A2L_merge();
    }

    log_printf(LOG_SYS_INFO, "Done.");
    clean_and_exit(0);
    return 0;
}
