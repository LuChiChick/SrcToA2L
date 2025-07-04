#include "Core_Functions.hpp"
#include "Tool_Functions.hpp"
#include "Config.hpp"

extern "C"
{
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
}

// 参数解析
void solve_args(int argc, char *argv[])
{
    // 处理输入部分
    {
        const char *reference_A2L_name_str = nullptr; // 参考文件名
        const char *output_name = nullptr;            // 输出文件名

        // 循环处理指令
        for (int count = 1; count < argc; count++)
        {
            // 匹配到参考a2l文件
            if (!strcmp(argv[count], "-r"))
            {
                if (count + 1 >= argc)
                {
                    print_log(LOG_FAILURE, "No reference .a2l file input.\n");
                    break;
                }
                input_reference_A2L_file = fopen(argv[count + 1], "rb");
                if (input_reference_A2L_file == nullptr)
                {
                    print_log(LOG_FAILURE, "Input reference .a2l file <%s> open failed.\n", argv[count + 1]);
                    break;
                }

                // 获取参考文件文件名
                reference_A2L_name_str = argv[count + 1];

                print_log(LOG_SUCCESS, "%-31s %s\n", "Input reference .a2l file:", argv[count + 1]);
                count++;
                continue;
            }

            // 匹配到链接map文件
            else if (!strcmp(argv[count], "-l"))
            {
                if (count + 1 >= argc)
                    break;
                input_map_file = fopen(argv[count + 1], "rb");
                if (input_map_file == nullptr)
                {
                    print_log(LOG_FAILURE, "Input link file <%s> open failed.\n", argv[count + 1]);
                    break;
                }

                print_log(LOG_SUCCESS, "%-31s %s\n", "Input link .map file:", argv[count + 1]);
                count++;
                continue;
            }

            // 匹配到自定义输出文件名
            else if (!strcmp(argv[count], "-o"))
            {
                if (count + 1 >= argc)
                    break;
                output_name = argv[count + 1];
                print_log(LOG_SUCCESS, "%-31s %s\n", "Set output filename:", argv[count + 1]);
                count++;
                continue;
            }

            // 其它输入作为源文件输入
            print_log(LOG_SUCCESS, "%-31s %s\n", "Input source filename:", argv[count]);

            // 处理源文件链表
            {
                static file_node *target_node = nullptr;
                if (source_file_list_head == nullptr)
                {
                    source_file_list_head = (file_node *)malloc(sizeof(file_node));
                    source_file_list_head->file_name_str = nullptr;
                    source_file_list_head->p_next = nullptr;
                    target_node = source_file_list_head;
                }
                else
                {
                    target_node->p_next = (file_node *)malloc(sizeof(file_node));
                    target_node = target_node->p_next;
                    target_node->file_name_str = nullptr;
                    target_node->p_next = nullptr;
                }

                // 记录文件名
                target_node->file_name_str = argv[count];
            }
        }

        // 检查文件列表状态
        if (source_file_list_head == nullptr)
        {
            print_log(LOG_FAILURE, "No source file input.\n");
            free(source_file_list_head);
            fcloseall();
            exit(0);
        }

        // 检查参考文件输入文件状态
        if (input_reference_A2L_file == nullptr)
        {
            fcloseall();
            exit(0);
        }

        // 处理输出文件名称
        if (output_name != nullptr)
        {
            output_target_A2L_file = fopen(output_name, "wb+");
            char *buff = (char *)malloc(strlen(output_name) + strlen(OUTPUT_MIDDLEWARE_SUFFIX) + 1);
            sprintf(buff, "%s%s", output_name, OUTPUT_MIDDLEWARE_SUFFIX);
            output_middleware_file = fopen(buff, "wb+");
            free(buff);
        }
        else if (reference_A2L_name_str != nullptr)
        {
            // 拼接默认的输出文件名
            const char *prefix = OUTPUT_A2L_DEFAULT_PREFIX;
            char *buffer = (char *)malloc(strlen(reference_A2L_name_str) + strlen(prefix) + strlen(OUTPUT_MIDDLEWARE_SUFFIX) + 1);

            // 获取路径部分长度和不带路径的文件名起始位置
            int path_length = strlen(reference_A2L_name_str);
            const char *file_name_no_dir = reference_A2L_name_str + path_length - 1;

            // 遍历指针没有回到起始位置且没有遇到正反斜杠
            while (file_name_no_dir + 1 != reference_A2L_name_str && *file_name_no_dir != '\\' && *file_name_no_dir != '/')
            {
                path_length--;
                file_name_no_dir--;
            }
            file_name_no_dir++;

            // 开始拼接
            for (int count = 0; count < path_length; count++)
                buffer[count] = reference_A2L_name_str[count];
            sprintf(buffer + path_length, "%s%s", prefix, file_name_no_dir);

            print_log(LOG_NORMAL, "%-31s %s\n", "Default output filename:", buffer);

            // 打开输出文件
            output_target_A2L_file = fopen(buffer, "wb+");

            // 打开中间件文件
            sprintf(buffer + path_length + strlen(prefix) + strlen(file_name_no_dir), OUTPUT_MIDDLEWARE_SUFFIX);
            output_middleware_file = fopen(buffer, "wb+");

            // 释放临时指针
            free(buffer);
        }
        // 检查map链接文件文件状态
        if (input_map_file == nullptr)
            print_log(LOG_WARN, "%-31s %s\n", "No link .map file input.", "Address will be set to 0x00000000");
    }
}

// 解析宏定义
void solve_defines(void)
{
    file_node *target_node = source_file_list_head;

    // 循环处理文件链表
    while (target_node != nullptr)
    {

        FILE *target_file = nullptr;
        target_file = fopen(target_node->file_name_str, "rb");
        // 处理无效的文件输入
        if (target_file == nullptr)
        {
            print_log(LOG_FAILURE, "Source file \"%s\" open failed.\n", target_node->file_name_str);
            target_node = target_node->p_next;
            continue;
        }

        // 成功打开文件
        print_log(LOG_NORMAL, "%-31s %s\n\n", "Start define solving:", target_node->file_name_str);

        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环按行解析
        while (true)
        {
            error_type_enum err = f_getline(target_file, segment_buff, sizeof(segment_buff));
            if (err != ERROR_NONE)
                break;

            // 查找到define
            if (strstr(segment_buff, "#define"))
            {
                printf("#define%s", segment_buff + strlen("#define"));

                // 分配节点空间
                static define_node *target_node;
                if (define_list_head == nullptr)
                {
                    define_list_head = (define_node *)malloc(sizeof(define_node));
                    target_node = define_list_head;
                    memset(target_node->define_str, '\0', sizeof(target_node->define_str));
                    memset(target_node->context_str, '\0', sizeof(target_node->context_str));
                    target_node->p_next = nullptr;
                }
                else
                {
                    target_node->p_next = (define_node *)malloc(sizeof(define_node));
                    target_node = target_node->p_next;
                    memset(target_node->define_str, '\0', sizeof(target_node->define_str));
                    memset(target_node->context_str, '\0', sizeof(target_node->context_str));
                    target_node->p_next = nullptr;
                }

                // 记录信息
                sscanf(segment_buff, "%*s%s%s", target_node->define_str, target_node->context_str);
            }
        }

        fclose(target_file);
        target_node = target_node->p_next;
    }
}

// 类型解析
void solve_types(void)
{
    file_node *target_node = source_file_list_head;

    // 循环处理文件链表
    while (target_node != nullptr)
    {

        FILE *target_file = nullptr;
        target_file = fopen(target_node->file_name_str, "rb");
        // 处理无效的文件输入
        if (target_file == nullptr)
        {
            print_log(LOG_FAILURE, "Source file \"%s\" open failed.\n", target_node->file_name_str);
            target_node = target_node->p_next;
            continue;
        }

        // 成功打开文件
        print_log(LOG_NORMAL, "%-31s %s\n\n", "Start type solving:", target_node->file_name_str);

        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环按行解析
        while (true)
        {
            error_type_enum err = f_getline(target_file, segment_buff, sizeof(segment_buff));
            if (err != ERROR_NONE)
                break;

            // 检测到类型定义起始
            if (strstr(segment_buff, typedef_begin))
            {

                printf("\n%s\n\n", typedef_begin);

                // 循环处理
                while (true)
                {
                    size_t seek_len = 0;
                    error_type_enum err = f_getline(target_file, segment_buff, sizeof(segment_buff), &seek_len);
                    if (err != ERROR_NONE)
                        break;

                    // 类型定义结束行
                    if (strstr(segment_buff, typedef_end))
                    {
                        printf("\n%s\n\n", typedef_end);
                        break;
                    }

                    // 检测到typedef
                    if (strstr(segment_buff, "typedef"))
                    {
                        static type_node *target_type_node;

                        // 类型链表头为空时初始化类型链表头
                        if (type_list_head == nullptr)
                        {
                            type_list_head = (type_node *)malloc(sizeof(type_node));
                            type_list_head->element_list_head = nullptr;
                            type_list_head->p_next = nullptr;
                            memset(type_list_head->type_name_str, '\0', sizeof(type_list_head->type_name_str));
                            target_type_node = type_list_head;
                        }
                        else
                        {
                            // 扩展到下一张链表
                            type_node *temp_node = (type_node *)malloc(sizeof(type_node));
                            target_type_node->p_next = temp_node;
                            target_type_node = temp_node;
                        }

                        // 回退文件指针到typedef结尾
                        fseek(target_file, -(seek_len - ((strstr(segment_buff, "typedef") - segment_buff) + strlen("typedef"))), SEEK_CUR);

                        // 读取下一个有效词组
                        f_getword(target_file, segment_buff, sizeof(segment_buff), &seek_len);
                        // 表示struct起始
                        if (!strcmp(segment_buff, "struct"))
                        {
                            target_type_node->type = STRUCTURE;
                            // 直接结构体类型名
                            if (f_getword(target_file, segment_buff, sizeof(segment_buff), &seek_len) == ERROR_NONE)
                                sprintf(target_type_node->type_name_str, segment_buff);
                        }
                        // 跳转到定义内
                        while (fgetc(target_file) != '{')
                            ;

                        // 新建第一个子元素节点
                        sub_element_node *element_node = (sub_element_node *)malloc(sizeof(sub_element_node));
                        element_node->p_next = nullptr;
                        target_type_node->element_list_head = element_node;

                        // 复合类型解析
                        while (true)
                        {
                            f_seek_skip_blank(target_file);
                            f_get_codeline(target_file, segment_buff, sizeof(segment_buff), &seek_len);

                            // 解析成员类型
                            element_node->element_info = solve_base_variable(segment_buff);

                            /**
                             * @todo
                             * 去掉临时输出，改用制表符输出
                             */
                            if (element_node->element_info.element_count == 1)
                                printf(".%s\n", element_node->element_info.name_str);
                            else
                                printf(".%s[%d]\n", element_node->element_info.name_str, element_node->element_info.element_count);

                            f_seek_skip_blank(target_file);
                            if (fgetc(target_file) == '}')
                                break;
                            else
                            {
                                fseek(target_file, -1, SEEK_CUR);
                                element_node->p_next = (sub_element_node *)malloc(sizeof(sub_element_node));
                                element_node = element_node->p_next;
                                element_node->p_next = nullptr;
                            }
                        }

                        // 处理类型别名(一个或多个)
                        while (true)
                        {
                            if (f_getword(target_file, segment_buff, sizeof(segment_buff), &seek_len) == ERROR_NONE)
                            {
                                // 当前还没有类型名
                                if (target_type_node->type_name_str[0] == '\0')
                                {
                                    sprintf(target_type_node->type_name_str, segment_buff);
                                }
                                else
                                {
                                    target_type_node->p_next = (type_node *)malloc(sizeof(type_node));
                                    memset(target_type_node->p_next->type_name_str, '\0', sizeof(target_type_node->type_name_str));
                                    target_type_node->p_next->element_list_head = target_type_node->element_list_head;
                                    target_type_node->p_next->type = target_type_node->type;
                                    target_type_node->p_next->p_next = nullptr;
                                    sprintf(target_type_node->p_next->type_name_str, segment_buff);
                                    target_type_node = target_type_node->p_next;
                                }
                            }
                            f_seek_skip_blank(target_file);
                            if (fgetc(target_file) == ';')
                                break;
                        }
                    }
                }
            }
        }

        fclose(target_file);
        target_node = target_node->p_next;
    }
}

// 处理中间件
void solve_middleware(void)
{
    file_node *target_node = source_file_list_head;

    // 循环处理输入文件链表
    while (target_node != nullptr)
    {
        FILE *target_file = nullptr;
        target_file = fopen(target_node->file_name_str, "rb");
        if (target_file == nullptr)
        {
            print_log(LOG_FAILURE, "Source file \"%s\" load failed.\n", target_node->file_name_str);
            target_node = target_node->p_next;
            continue;
        }

        print_log(LOG_NORMAL, "%-31s %s\n\n", "Start variable solving:", target_node->file_name_str);
        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环获取输入文件行
        while (f_getline(target_file, segment_buff, sizeof(segment_buff)) == ERROR_NONE)
        {
            // 当前行为观测量起始位行
            if (strstr(segment_buff, measurement_begin))
            {
                printf("\n%s\n\n", measurement_begin);
                fprintf(output_middleware_file, auto_generated_measurement_start);

                // 清空段缓冲区
                memset(segment_buff, '\0', sizeof(segment_buff));

                while (true)
                {
                    // 获取下一行
                    size_t seek_len = 0;
                    f_getline(target_file, segment_buff, sizeof(segment_buff), &seek_len);

                    // 观测量结束行
                    if (strstr(segment_buff, measurement_end))
                    {
                        printf("\n%s\n\n", measurement_end);
                        fprintf(output_middleware_file, auto_generated_measurement_end);
                        break;
                    }
                    // 非结束行，使用代码行进行处理，先回退，跳过空行后再重新读
                    fseek(target_file, -seek_len, SEEK_CUR);
                    f_seek_skip_blank(target_file);
                    memset(segment_buff, '\0', sizeof(segment_buff));
                    f_get_codeline(target_file, segment_buff, sizeof(segment_buff), &seek_len);

                    // 解析元素变量信息
                    {
                        variable_info info = solve_variable(segment_buff);

                        // 成功解析
                        if (info.type != TYPE_NOT_SUPPORTED)
                        {
                            // 结构体解析
                            if (info.type == STRUCTURE)
                            {
                                // 匹配类型描述链表位置
                                type_node *target_type = type_list_head;
                                while (target_type != nullptr)
                                {
                                    if (!strcmp(target_type->type_name_str, info.type_name_str))
                                    {
                                        info.type_name_str = target_type->type_name_str;
                                        break;
                                    }
                                    target_type = target_type->p_next;
                                }

                                sub_element_node *sub_element_list = target_type->element_list_head;

                                // 获取地址
                                // info = get_element_addr(info, Input_Map);

                                // 调试信息
                                if (info.start_addr_32 != 0)
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }
                                else
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_WARN, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_WARN, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }

                                // 单变量
                                if (info.element_count == 1)
                                {
                                    int addr_offset = 0;
                                    while (sub_element_list != nullptr)
                                    {
                                        // 计算地址偏移
                                        if (addr_offset % sub_element_list->element_info.single_element_size != 0)
                                            addr_offset += sub_element_list->element_info.single_element_size - (addr_offset % sub_element_list->element_info.single_element_size);

                                        // 子元素非数组
                                        if (sub_element_list->element_info.element_count == 1)
                                        {
                                            fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                            fprintf(output_middleware_file, "    %s.%s\r\n", info.name_str, sub_element_list->element_info.name_str);  // 名称
                                            fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                             // 描述
                                            fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_type_str);                // 数据类型
                                            fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                          // 转换式(保留原始值)
                                            fprintf(output_middleware_file, "    %s\r\n", "0");                                                        // 分辨率
                                            fprintf(output_middleware_file, "    %s\r\n", "0");                                                        // 精度误差
                                            fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_min_limit_str);           // 下限
                                            fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_max_limit_str);           // 上限
                                            fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32 + addr_offset); // ECU 地址

                                            fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾

                                            addr_offset += sub_element_list->element_info.single_element_size;
                                        }
                                        // 子元素是数组
                                        else
                                        {
                                            // 计算地址偏移
                                            if (addr_offset % sub_element_list->element_info.single_element_size != 0)
                                                addr_offset += sub_element_list->element_info.single_element_size - (addr_offset % sub_element_list->element_info.single_element_size);

                                            for (size_t count = 0; count < sub_element_list->element_info.element_count; count++)
                                            {

                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                                fprintf(output_middleware_file, "    %s.%s[%d]\r\n", info.name_str, sub_element_list->element_info.name_str, count);                                                    // 名称
                                                fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                                                                          // 描述
                                                fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_type_str);                                                                             // 数据类型
                                                fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                                                                                       // 转换式(保留原始值)
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                                                     // 分辨率
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                                                     // 精度误差
                                                fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_min_limit_str);                                                                        // 下限
                                                fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_max_limit_str);                                                                        // 上限
                                                fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32 + addr_offset + count * sub_element_list->element_info.single_element_size); // ECU 地址

                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                                            }
                                            addr_offset += sub_element_list->element_info.element_count * sub_element_list->element_info.single_element_size;
                                        }

                                        sub_element_list = sub_element_list->p_next;
                                    }
                                }
                                // 类型是数组
                                else
                                {
                                    int addr_offset = 0;

                                    for (size_t outter_count = 0; outter_count < info.element_count; outter_count++)
                                    {
                                        sub_element_node *list = sub_element_list;
                                        while (list != nullptr)
                                        {
                                            // 计算地址偏移
                                            if (addr_offset % list->element_info.single_element_size != 0)
                                                addr_offset += list->element_info.single_element_size - (addr_offset % list->element_info.single_element_size);

                                            // 子元素非数组
                                            if (list->element_info.element_count == 1)
                                            {
                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                                fprintf(output_middleware_file, "    %s[%d].%s\r\n", info.name_str, outter_count, list->element_info.name_str); // 名称
                                                fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                  // 描述
                                                fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_type_str);                                 // 数据类型
                                                fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                               // 转换式(保留原始值)
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                             // 分辨率
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                             // 精度误差
                                                fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_min_limit_str);                            // 下限
                                                fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_max_limit_str);                            // 上限
                                                fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32 + addr_offset);      // ECU 地址

                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾

                                                addr_offset += list->element_info.single_element_size;
                                            }
                                            // 子元素是数组
                                            else
                                            {
                                                // 计算地址偏移
                                                if (addr_offset % list->element_info.single_element_size != 0)
                                                    addr_offset += list->element_info.single_element_size - (addr_offset % list->element_info.single_element_size);

                                                for (size_t count = 0; count < list->element_info.element_count; count++)
                                                {

                                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                                    fprintf(output_middleware_file, "    %s[%d].%s[%d]\r\n", info.name_str, outter_count, list->element_info.name_str, count);                                  // 名称
                                                    fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                                                              // 描述
                                                    fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_type_str);                                                                             // 数据类型
                                                    fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                                                                           // 转换式(保留原始值)
                                                    fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                                         // 分辨率
                                                    fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                                         // 精度误差
                                                    fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_min_limit_str);                                                                        // 下限
                                                    fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_max_limit_str);                                                                        // 上限
                                                    fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32 + addr_offset + count * list->element_info.single_element_size); // ECU 地址

                                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                                                }
                                                addr_offset += list->element_info.element_count * list->element_info.single_element_size;
                                            }

                                            list = list->p_next;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // 获取地址
                                // info = get_element_addr(info, Input_Map);

                                // 调试信息
                                if (info.start_addr_32 != 0)
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }
                                else
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_WARN, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_WARN, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }

                                // 单变量
                                if (info.element_count == 1)
                                {
                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                    fprintf(output_middleware_file, "    %s\r\n", info.name_str);                                // 名称
                                    fprintf(output_middleware_file, "    \"auto generated\"\r\n");                               // 描述
                                    fprintf(output_middleware_file, "    %s\r\n", info.A2L_type_str);                            // 数据类型
                                    fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                            // 转换式(保留原始值)
                                    fprintf(output_middleware_file, "    %s\r\n", "0");                                          // 分辨率
                                    fprintf(output_middleware_file, "    %s\r\n", "0");                                          // 精度误差
                                    fprintf(output_middleware_file, "    %s\r\n", info.A2L_min_limit_str);                       // 下限
                                    fprintf(output_middleware_file, "    %s\r\n", info.A2L_max_limit_str);                       // 上限
                                    fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32); // ECU 地址

                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                                }
                                else
                                {
                                    for (size_t count = 0; count < info.element_count; count++)
                                    {
                                        fprintf(output_middleware_file, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                        fprintf(output_middleware_file, "    %s[%d]\r\n", info.name_str, count);                                                        // 名称
                                        fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                                  // 描述
                                        fprintf(output_middleware_file, "    %s\r\n", info.A2L_type_str);                                                               // 数据类型
                                        fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                                               // 转换式(保留原始值)
                                        fprintf(output_middleware_file, "    %s\r\n", "0");                                                                             // 分辨率
                                        fprintf(output_middleware_file, "    %s\r\n", "0");                                                                             // 精度误差
                                        fprintf(output_middleware_file, "    %s\r\n", info.A2L_min_limit_str);                                                          // 下限
                                        fprintf(output_middleware_file, "    %s\r\n", info.A2L_max_limit_str);                                                          // 上限
                                        fprintf(output_middleware_file, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.start_addr_32 + count * info.single_element_size); // ECU 地址

                                        fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                                    }
                                }
                            }
                        }
                        else if (info.name_str[0] != '\0') // 解析失败且变量名不为空
                        {
                            if (info.element_count == 1)
                                print_log(LOG_FAILURE, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                            else
                                print_log(LOG_FAILURE, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                        }
                    }

                    // 先跳行，防止段尾位注释引起异常
                    f_seek_nextline(target_file);
                    // 跳过空白段
                    f_seek_skip_blank(target_file);
                    // 清空段缓冲区
                    memset(segment_buff, '\0', sizeof(segment_buff));
                }
            }

            // 当前行为标定量起始位行
            if (strstr(segment_buff, calibration_begin))
            {
                printf("\n%s\n\n", calibration_begin);
                fprintf(output_middleware_file, auto_generated_calibration_start);

                // 清空段缓冲区
                memset(segment_buff, '\0', sizeof(segment_buff));

                while (true)
                {
                    // 获取下一行
                    size_t seek_len = 0;
                    f_getline(target_file, segment_buff, sizeof(segment_buff), &seek_len);

                    // 标定量结束行
                    if (strstr(segment_buff, calibration_end))
                    {
                        printf("\n%s\n\n", calibration_end);
                        fprintf(output_middleware_file, auto_generated_calibration_end);
                        break;
                    }

                    // 非结束行，使用代码行进行处理，先回退，跳过空行后再重新读
                    fseek(target_file, -seek_len, SEEK_CUR);
                    f_seek_skip_blank(target_file);
                    memset(segment_buff, '\0', sizeof(segment_buff));
                    f_get_codeline(target_file, segment_buff, sizeof(segment_buff), &seek_len);

                    // 解析元素变量信息
                    {
                        variable_info info = solve_variable(segment_buff);

                        // 成功解析
                        if (info.type != TYPE_NOT_SUPPORTED)
                        {

                            if (info.type == STRUCTURE)
                            {
                                // 匹配类型描述链表位置
                                type_node *target_type = type_list_head;
                                while (target_type != nullptr)
                                {
                                    if (!strcmp(target_type->type_name_str, info.type_name_str))
                                    {
                                        info.type_name_str = target_type->type_name_str;
                                        break;
                                    }
                                    target_type = target_type->p_next;
                                }

                                sub_element_node *sub_element_list = target_type->element_list_head;

                                // 获取地址
                                // info = get_element_addr(info, Input_Map);

                                // 调试信息
                                if (info.start_addr_32 != 0)
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }
                                else
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_WARN, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_WARN, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }

                                // 单变量
                                if (info.element_count == 1)
                                {
                                    int addr_offset = 0;
                                    while (sub_element_list != nullptr)
                                    {
                                        // 计算地址偏移
                                        if (addr_offset % sub_element_list->element_info.single_element_size != 0)
                                            addr_offset += sub_element_list->element_info.single_element_size - (addr_offset % sub_element_list->element_info.single_element_size);

                                        // 子元素非数组
                                        if (sub_element_list->element_info.element_count == 1)
                                        {
                                            fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                            fprintf(output_middleware_file, "    %s.%s\r\n", info.name_str, sub_element_list->element_info.name_str); // 名称
                                            fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                            // 描述
                                            fprintf(output_middleware_file, "    %s\r\n", "VALUE");                                                   // 值类型(数值、数组、曲线)
                                            fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32 + addr_offset);                      // ECU 地址
                                            fprintf(output_middleware_file, "    Scalar_%s\r\n", sub_element_list->element_info.A2L_type_str);        // 数据类型
                                            fprintf(output_middleware_file, "    %s\r\n", "0");                                                       // 允许最大差分
                                            fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                         // 转换式(保留原始值)
                                            fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_min_limit_str);          // 下限
                                            fprintf(output_middleware_file, "    %s\r\n\r\n", sub_element_list->element_info.A2L_max_limit_str);      // 上限

                                            fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾

                                            addr_offset += sub_element_list->element_info.single_element_size;
                                        }
                                        // 子元素是数组
                                        else
                                        {
                                            // 计算地址偏移
                                            if (addr_offset % sub_element_list->element_info.single_element_size != 0)
                                                addr_offset += sub_element_list->element_info.single_element_size - (addr_offset % sub_element_list->element_info.single_element_size);

                                            for (size_t count = 0; count < sub_element_list->element_info.element_count; count++)
                                            {
                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                                fprintf(output_middleware_file, "    %s.%s[%d]\r\n", info.name_str, sub_element_list->element_info.name_str, count);                              // 名称
                                                fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                                                    // 描述
                                                fprintf(output_middleware_file, "    %s\r\n", "VALUE");                                                                                           // 值类型(数值、数组、曲线)
                                                fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32 + addr_offset + count * sub_element_list->element_info.single_element_size); // ECU 地址
                                                fprintf(output_middleware_file, "    Scalar_%s\r\n", sub_element_list->element_info.A2L_type_str);                                                // 数据类型
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                               // 允许最大差分
                                                fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                                                                 // 转换式(保留原始值)
                                                fprintf(output_middleware_file, "    %s\r\n", sub_element_list->element_info.A2L_min_limit_str);                                                  // 下限
                                                fprintf(output_middleware_file, "    %s\r\n\r\n", sub_element_list->element_info.A2L_max_limit_str);                                              // 上限

                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                                            }
                                            addr_offset += sub_element_list->element_info.element_count * sub_element_list->element_info.single_element_size;
                                        }

                                        sub_element_list = sub_element_list->p_next;
                                    }
                                }
                                // 类型是数组
                                else
                                {
                                    int addr_offset = 0;

                                    for (size_t outter_count = 0; outter_count < info.element_count; outter_count++)
                                    {
                                        sub_element_node *list = sub_element_list;
                                        while (list != nullptr)
                                        {
                                            // 计算地址偏移
                                            if (addr_offset % list->element_info.single_element_size != 0)
                                                addr_offset += list->element_info.single_element_size - (addr_offset % list->element_info.single_element_size);

                                            // 子元素非数组
                                            if (list->element_info.element_count == 1)
                                            {
                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                                fprintf(output_middleware_file, "    %s[%d].%s\r\n", info.name_str, outter_count, list->element_info.name_str); // 名称
                                                fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                  // 描述
                                                fprintf(output_middleware_file, "    %s\r\n", "VALUE");                                                         // 值类型(数值、数组、曲线)
                                                fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32 + addr_offset);                            // ECU 地址
                                                fprintf(output_middleware_file, "    Scalar_%s\r\n", list->element_info.A2L_type_str);                          // 数据类型
                                                fprintf(output_middleware_file, "    %s\r\n", "0");                                                             // 允许最大差分
                                                fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                               // 转换式(保留原始值)
                                                fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_min_limit_str);                            // 下限
                                                fprintf(output_middleware_file, "    %s\r\n\r\n", list->element_info.A2L_max_limit_str);                        // 上限

                                                fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾

                                                addr_offset += list->element_info.single_element_size;
                                            }
                                            // 子元素是数组
                                            else
                                            {
                                                // 计算地址偏移
                                                if (addr_offset % list->element_info.single_element_size != 0)
                                                    addr_offset += list->element_info.single_element_size - (addr_offset % list->element_info.single_element_size);

                                                for (size_t count = 0; count < list->element_info.element_count; count++)
                                                {

                                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                                    fprintf(output_middleware_file, "    %s[%d].%s[%d]\r\n", info.name_str, outter_count, list->element_info.name_str, count);            // 名称
                                                    fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                                                        // 描述
                                                    fprintf(output_middleware_file, "    %s\r\n", "VALUE");                                                                               // 值类型(数值、数组、曲线)
                                                    fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32 + addr_offset + count * list->element_info.single_element_size); // ECU 地址
                                                    fprintf(output_middleware_file, "    Scalar_%s\r\n", list->element_info.A2L_type_str);                                                // 数据类型
                                                    fprintf(output_middleware_file, "    %s\r\n", "0");                                                                                   // 允许最大差分
                                                    fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                                                     // 转换式(保留原始值)
                                                    fprintf(output_middleware_file, "    %s\r\n", list->element_info.A2L_min_limit_str);                                                  // 下限
                                                    fprintf(output_middleware_file, "    %s\r\n\r\n", list->element_info.A2L_max_limit_str);                                              // 上限

                                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                                                }
                                                addr_offset += list->element_info.element_count * list->element_info.single_element_size;
                                            }

                                            list = list->p_next;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // 获取地址
                                // info = get_element_addr(info, Input_Map);

                                // 调试信息
                                if (info.start_addr_32 != 0)
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_SUCCESS, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }
                                else
                                {
                                    if (info.element_count == 1)
                                        print_log(LOG_WARN, "0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                                    else
                                        print_log(LOG_WARN, "0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                                }

                                // 单变量
                                if (info.element_count == 1)
                                {
                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                    fprintf(output_middleware_file, "    %s\r\n", info.name_str);              // 名称
                                    fprintf(output_middleware_file, "    \"auto generated\"\r\n");             // 描述
                                    fprintf(output_middleware_file, "    %s\r\n", "VALUE");                    // 值类型(数值、数组、曲线)
                                    fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32);     // ECU 地址
                                    fprintf(output_middleware_file, "    Scalar_%s\r\n", info.A2L_type_str);   // 数据类型
                                    fprintf(output_middleware_file, "    %s\r\n", "0");                        // 允许最大差分
                                    fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");          // 转换式(保留原始值)
                                    fprintf(output_middleware_file, "    %s\r\n", info.A2L_min_limit_str);     // 下限
                                    fprintf(output_middleware_file, "    %s\r\n\r\n", info.A2L_max_limit_str); // 上限

                                    fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                                }
                                else
                                {
                                    for (size_t count = 0; count < info.element_count; count++)
                                    {
                                        fprintf(output_middleware_file, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                        fprintf(output_middleware_file, "    %s[%d]\r\n", info.name_str, count);                                  // 名称
                                        fprintf(output_middleware_file, "    \"auto generated\"\r\n");                                            // 描述
                                        fprintf(output_middleware_file, "    %s\r\n", "VALUE");                                                   // 值类型(数值、数组、曲线)
                                        fprintf(output_middleware_file, "    0x%08x\r\n", info.start_addr_32 + count * info.single_element_size); // ECU 地址
                                        fprintf(output_middleware_file, "    Scalar_%s\r\n", info.A2L_type_str);                                  // 数据类型
                                        fprintf(output_middleware_file, "    %s\r\n", "0");                                                       // 允许最大差分
                                        fprintf(output_middleware_file, "    %s\r\n", "NO_COMPU_METHOD");                                         // 转换式(保留原始值)
                                        fprintf(output_middleware_file, "    %s\r\n", info.A2L_min_limit_str);                                    // 下限
                                        fprintf(output_middleware_file, "    %s\r\n\r\n", info.A2L_max_limit_str);                                // 上限

                                        fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                                    }
                                }
                            }
                        }
                        else if (info.name_str[0] != '\0') // 解析失败且变量名不为空
                        {
                            if (info.element_count == 1)
                                printf("[< FAIL >] 0x%08x %-20s %s;\n", info.start_addr_32, info.A2L_type_str, info.name_str);
                            else
                                printf("[< FAIL >] 0x%08x %-20s %s[%d];\n", info.start_addr_32, info.A2L_type_str, info.name_str, info.element_count);
                        }
                    }

                    // 先跳行，防止段尾位注释引起异常
                    f_seek_nextline(target_file);
                    // 跳过空白段
                    f_seek_skip_blank(target_file);
                    // 清空段缓冲区
                    memset(segment_buff, '\0', sizeof(segment_buff));
                }
            }
        }

        fclose(target_file);
        target_node = target_node->p_next;
    }
}

// 处理最终A2L输出
void solve_A2L_output(void)
{
    // 段缓冲区
    char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

    // 读取并复制参考文件直到 a2l MODULE 结尾
    while (f_getline(input_reference_A2L_file, segment_buff, sizeof(segment_buff)) == ERROR_NONE)
    {
        // 当前行为 a2l MODULE 结尾
        if (strstr(segment_buff, a2l_module_end))
        {
            // 回退文件指针到上一行结尾
            fseek(input_reference_A2L_file, -2, SEEK_CUR);
            while (fgetc(input_reference_A2L_file) != '\n')
                fseek(input_reference_A2L_file, -2, SEEK_CUR);
            break;
        }

        // 输出行到文件
        // fprintf(Output_Target, segment_buff);    // 太大300会段溢出
        for (int count = 0; count < SEGMENT_BUFF_LENGTH; count++) // 逐个输出
        {
            fputc(segment_buff[count], output_target_A2L_file);
            if (segment_buff[count] == '\n')
                break;
        }

        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }
    fseek(output_middleware_file, 0, SEEK_SET);

    char ch = fgetc(output_middleware_file);
    while (ch != EOF)
    {
        fputc(ch, output_target_A2L_file);
        ch = fgetc(output_middleware_file);
    }

    // 输出参考文件的剩余部分
    while (f_getline(input_reference_A2L_file, segment_buff, sizeof(segment_buff)) == ERROR_NONE)
    {
        // 输出行到文件
        fprintf(output_target_A2L_file, segment_buff);
        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }
}
// end