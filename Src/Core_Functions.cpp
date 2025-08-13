#include "Config.hpp"
#include "Core_Functions.hpp"
#include "Tool_Functions.hpp"
#include "Global_Variables.hpp"

extern "C"
{
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"
}

// 参数解析
void solve_args(int argc, char *argv[])
{
    // 文件名指针记录
    const char *p_reference_file_name = nullptr;
    const char *p_map_file_name = nullptr;

    // 遍历命令参数
    for (int count = 1; count < argc; count++)
    {
        // 是指令行
        if (argv[count][0] == '-')
        {

            // -a 字节对齐长度设置
            if (!strcmp(argv[count], "-a"))
            {
                // 错误输入处理
                if (count + 1 == argc)
                    break;
                if (argv[count + 1][0] == '-')
                    continue;

                size_t value = 0;
                sscanf(argv[count + 1], "%u", &value);

                // 利用2进制中2的整次幂仅有1位为1判定是否为合法字节对齐
                if ((value & (value - 1)) == 0 && value != 0)
                {
                    log_printf(LOG_SUCCESS, "Alignment size set to %u.", value);
                    addr_alignment_size = value;
                }
                else
                    log_printf(LOG_FAILURE, "Illegal alignment arg \"%s\", set to default %u.", argv[count + 1], addr_alignment_size);
                count++;
                continue;
            }

            // -r 参考A2L输入
            if (!strcmp(argv[count], "-r"))
            {
                // 错误输入处理
                if (count + 1 == argc)
                    break;
                if (argv[count + 1][0] == '-')
                    continue;

                // 已经打开了一个参考文件
                if (input_reference_A2L_file != nullptr)
                    log_printf(LOG_WARN, "Only single .a2l reference file supported,last will be use as input.");

                // 尝试打开文件
                input_reference_A2L_file = fopen(argv[count + 1], "rb");
                if (input_reference_A2L_file == nullptr)
                    log_printf(LOG_FAILURE, "Reference file \"%s\" open failed.", argv[count + 1]);
                else
                {
                    log_printf(LOG_SUCCESS, "Reference file \"%s\" open succeed.", argv[count + 1]);
                    p_reference_file_name = argv[count + 1];
                }
                count++;
                continue;
            }

            // -m 链接map输入
            if (!strcmp(argv[count], "-m"))
            {
                // 错误输入处理
                if (count + 1 == argc)
                    break;
                if (argv[count + 1][0] == '-')
                    continue;

                // 已经打开了一个链接文件
                if (input_map_file != nullptr)
                    log_printf(LOG_WARN, "Only single .map file supported,last will be use as input.");

                // 尝试打开文件
                input_map_file = fopen(argv[count + 1], "rb");
                if (input_map_file == nullptr)
                    log_printf(LOG_FAILURE, "Map file \"%s\" open failed.", argv[count + 1]);
                else
                {
                    log_printf(LOG_SUCCESS, "Map file \"%s\" open succeed.", argv[count + 1]);
                    p_map_file_name = argv[count + 1];
                }
                count++;
                continue;
            }

            log_printf(LOG_WARN, "Unknown argument \"%s\".", argv[count]);
            continue;
        }

        // 其余的作为源文件输入
        {
            // 排查重复文件
            bool is_file_duplicated = false;
            for (file_node *p_file_node = source_and_header_file_list_head; p_file_node != nullptr; p_file_node = p_file_node->p_next)
            {
                if (!strcmp(argv[count], p_file_node->file_name_str))
                {
                    is_file_duplicated = true;
                    log_printf(LOG_WARN, "Source or header file \"%s\" duplicated.", argv[count]);
                    break;
                }
            }
            if (is_file_duplicated)
                continue;

            // 检测文件是否存在
            FILE *target_file = fopen(argv[count], "rb");
            if (target_file == nullptr)
            {
                log_printf(LOG_FAILURE, "Source or header file \"%s\" open failed.", argv[count]);
                continue;
            }

            // 成功打开文件
            static file_node *target_file_node = nullptr;

            // 开辟新的空间
            if (source_and_header_file_list_head == nullptr)
            {
                source_and_header_file_list_head = (file_node *)malloc(sizeof(file_node));
                target_file_node = source_and_header_file_list_head;
            }
            else
            {
                target_file_node->p_next = (file_node *)malloc(sizeof(file_node));
                target_file_node = target_file_node->p_next;
            }

            // 记录文件链表
            target_file_node->file_name_str = argv[count];
            target_file_node->p_next = nullptr;

            // 关闭临时的文件指针
            fclose(target_file);

            // 日志
            log_printf(LOG_SUCCESS, "Source or header file \"%s\" open succeed.", argv[count]);
        }
    }

    // 没有任何文件输入
    if (source_and_header_file_list_head == nullptr)
    {
        log_printf(LOG_FAILURE, "No source or header file input.");
        // 退出并返回错误状态
        clean_and_exit(-1);
    }

    // 无链接map文件时
    if (input_map_file == nullptr)
        log_printf(LOG_WARN, "No .map file input,address will be set to 0x00000000");

    // 无参考A2L文件时
    if (input_reference_A2L_file == nullptr)
        log_printf(LOG_WARN, "No .a2l reference file input,only middleware will be generated.");

    // 拼接输出部分字符串
    char *output_file_name = nullptr;
    if (p_reference_file_name != nullptr)
    {
        // 分配空间
        output_file_name = (char *)malloc(strlen(p_reference_file_name) + strlen(OUTPUT_A2L_PREFIX) + 1);
        // 获取路径部分长度和不带路径的文件名起始位置
        int path_length = strlen(p_reference_file_name);
        const char *file_name_no_dir = p_reference_file_name + path_length - 1;
        // 遍历指针没有回到起始位置且没有遇到正反斜杠
        while (file_name_no_dir + 1 != p_reference_file_name && *file_name_no_dir != '\\' && *file_name_no_dir != '/')
        {
            path_length--;
            file_name_no_dir--;
        }
        file_name_no_dir++;

        // 开始拼接
        for (int count = 0; count < path_length; count++)
            output_file_name[count] = p_reference_file_name[count];
        sprintf(output_file_name + path_length, "%s%s", OUTPUT_A2L_PREFIX, file_name_no_dir);
    }

    // 输出工作流
    {
        printf("\n");
        log_printf(LOG_INFO, "Workflow details:");
        printf("                              ├─Alignment size: %u\n", addr_alignment_size);
        printf("                              ├─Source or header files:\n");

        // 遍历文件链表
        file_node *p_file = source_and_header_file_list_head;
        while (p_file != nullptr)
        {
            if (p_file->p_next != nullptr)
                printf("                              │     ├─%s\n", p_file->file_name_str);
            else
                printf("                              │     └─%s\n", p_file->file_name_str);

            p_file = p_file->p_next;
        }

        printf("                              ├─Reference file:\n");
        if (p_reference_file_name == nullptr)
            printf("                              │     └─(NULL)\n");
        else
            printf("                              │     └─%s\n", p_reference_file_name);

        printf("                              ├─Map file:\n");
        if (p_map_file_name == nullptr)
            printf("                              │     └─(NULL)\n");
        else
            printf("                              │     └─%s\n", p_map_file_name);

        printf("                              ├─Merged output file:\n");
        if (p_reference_file_name == nullptr)
            printf("                              │     └─(NULL)\n");
        else
            printf("                              │     └─%s\n", output_file_name);

        printf("                              └─Middleware file:\n");
        if (p_reference_file_name == nullptr)
            printf("                                    └─.\\%s\n", OUTPUT_DEFAULT_MIDDLEWARE_FILE_NAME);
        else
            printf("                                    └─%s%s\n\n", output_file_name, OUTPUT_MIDDLEWARE_SUFFIX);
    }

    // 打开输出文件
    {
        // 参考文件存在时
        if (p_reference_file_name != nullptr)
        {
            // 打开合并输出文件
            output_target_A2L_file = fopen(output_file_name, "wb");
            if (output_target_A2L_file == nullptr)
            {
                log_printf(LOG_FAILURE, "Merged output file \"%s\" create failed.", output_file_name);
                free(output_file_name);
                clean_and_exit(-1);
            }
            else
                log_printf(LOG_SUCCESS, "Merged output file \"%s\" create succeed.", output_file_name);

            // 打开中间件
            char *buff = (char *)malloc(strlen(output_file_name) + strlen(OUTPUT_MIDDLEWARE_SUFFIX) + 1);
            sprintf(buff, "%s%s", output_file_name, OUTPUT_MIDDLEWARE_SUFFIX);
            output_middleware_file = fopen(buff, "wb+");

            if (output_middleware_file == nullptr)
            {
                log_printf(LOG_FAILURE, "Middleware output file \"%s\" create failed.", buff);
                free(output_file_name);
                free(buff);
                clean_and_exit(-1);
            }
            else
                log_printf(LOG_SUCCESS, "Middleware output file \"%s\" create succeed.", buff);

            free(output_file_name);
            free(buff);
        }
        // 不存在时打开默认的中间件输出
        else
        {
            output_middleware_file = fopen(OUTPUT_DEFAULT_MIDDLEWARE_FILE_NAME, "wb");

            if (output_middleware_file == nullptr)
            {
                log_printf(LOG_FAILURE, "Middleware output file \"%s\" create failed.", OUTPUT_DEFAULT_MIDDLEWARE_FILE_NAME);
                clean_and_exit(-1);
            }
            else
                log_printf(LOG_SUCCESS, "Middleware output file \"%s\" create succeed.", OUTPUT_DEFAULT_MIDDLEWARE_FILE_NAME);
        }
    }
}

// 解析宏定义
void solve_defines(void)
{
    file_node *target_file_node = source_and_header_file_list_head;
    define_node *define_show_begin_node = nullptr;
    // 循环处理文件链表

    log_printf(LOG_INFO, "Solved definition details:");

    while (target_file_node != nullptr)
    {

        FILE *target_file = nullptr;
        target_file = fopen(target_file_node->file_name_str, "rb");

        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环按行解析
        while (true)
        {
            // 跳过空白读行
            f_seek_skip_blanks(target_file);
            if (f_getline(target_file, segment_buff, sizeof(segment_buff)) == 0)
                break;

            // 查找到define
            if (strstr(segment_buff, "#define") == segment_buff)
            {
                // 检查是否为数值类型的#define
                {
                    bool illegal_define = false;
                    char *p_str = segment_buff + strlen("#define");
                    // 跳过#define后的空格
                    while (*p_str == ' ')
                        p_str++;
                    // 检查第一个非空格段是否为合法段(宏定义名)
                    while (*p_str != ' ')
                    {
                        if (isalnum(*p_str) || *p_str == '_')
                            p_str++;
                        else
                        {
                            // 不是数字、字母、下划线，是换行符或者别的东西
                            illegal_define = true;
                            break;
                        }
                    }
                    // 跳过第二个空格段
                    while (*p_str == ' ')
                        p_str++;

                    // 检查后续的整段内容是否为正十进制数值
                    if (!isdigit(*p_str) && !(*p_str == '+' && isdigit(*(p_str + 1))))
                        illegal_define = true;

                    // 验证合法性
                    if (illegal_define)
                        continue;
                }

                // 分配节点空间
                static define_node *target_define_node;
                if (define_list_head == nullptr)
                {
                    define_list_head = (define_node *)malloc(sizeof(define_node));
                    target_define_node = define_list_head;
                    define_show_begin_node = define_list_head;
                }
                else
                {
                    target_define_node->p_next = (define_node *)malloc(sizeof(define_node));
                    target_define_node = target_define_node->p_next;
                    if (define_show_begin_node == nullptr)
                        define_show_begin_node = target_define_node;
                }

                memset(target_define_node->define_str, '\0', sizeof(target_define_node->define_str));
                target_define_node->value = 0;
                target_define_node->p_next = nullptr;

                // 记录信息
                sscanf(segment_buff, "%*s%s%u", target_define_node->define_str, &target_define_node->value);
            }
        }

        // 关闭文件
        fclose(target_file);

        // 日志输出
        if (target_file_node->p_next == nullptr)
        {
            printf("                              └─%s\n", target_file_node->file_name_str);
            while (true)
            {
                if (define_show_begin_node == nullptr)
                {
                    printf("                                    └─(NULL)\n");
                    break;
                }

                if (define_show_begin_node->p_next == nullptr)
                {
                    printf("                                    └─%s -> %u\n", define_show_begin_node->define_str, define_show_begin_node->value);
                    define_show_begin_node = nullptr;
                    break;
                }

                printf("                                    ├─%s -> %u\n", define_show_begin_node->define_str, define_show_begin_node->value);
                define_show_begin_node = define_show_begin_node->p_next;
            }
        }
        else
        {
            printf("                              ├─%s\n", target_file_node->file_name_str);
            while (true)
            {
                if (define_show_begin_node == nullptr)
                {
                    printf("                              │     └─(NULL)\n");
                    break;
                }

                if (define_show_begin_node->p_next == nullptr)
                {
                    printf("                              │     └─%s -> %u\n", define_show_begin_node->define_str, define_show_begin_node->value);
                    define_show_begin_node = nullptr;
                    break;
                }

                printf("                              │     ├─%s -> %u\n", define_show_begin_node->define_str, define_show_begin_node->value);
                define_show_begin_node = define_show_begin_node->p_next;
            }
        }

        // 更新指针
        target_file_node = target_file_node->p_next;
    }
}

// 类型解析
void solve_types(void)
{
    // 类型名称
    const char *type_str[] = {
        "TYPE_UNKNOWN",
        "STRUCTURE",
        "UBYTE",
        "UWORD",
        "ULONG",
        "SBYTE",
        "SWORD",
        "SLONG",
        "FLOAT32_IEEE",
        "FLOAT64_IEEE",
    };

    file_node *target_file_node = source_and_header_file_list_head;
    type_node *type_show_begin_node = nullptr;
    log_printf(LOG_INFO, "Solved compound type details:");
    // 循环处理文件链表
    while (target_file_node != nullptr)
    {

        FILE *target_file = nullptr;
        target_file = fopen(target_file_node->file_name_str, "rb");

        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环按行解析
        while (true)
        {
            // 类型链表指针
            static type_node *target_type_node = nullptr;

            // 跳过注释和空白后
            f_seek_skip_comments_and_blanks(target_file);
            if (f_getline(target_file, segment_buff, sizeof(segment_buff)) == 0)
                break;

            // 检测到类型定义起始
            if (strstr(segment_buff, "typedef"))
            {
                char *p_typedef = (strstr(segment_buff, "typedef"));
                // 过滤非独立的typedef
                if (isalnum(*(p_typedef - 1)) || *(p_typedef - 1) == '_' || *(p_typedef + strlen("typedef")) != ' ')
                    break;

                // 过滤处在字符串中的typedef
                if (strstr(segment_buff, "\""))
                {
                    bool pre_quote = false;
                    for (const char *p_str = p_typedef; p_str != segment_buff; p_str--)
                        if (*p_str == '\"')
                            pre_quote = true;

                    bool end_quote = false;
                    for (const char *p_str = p_typedef; p_str != segment_buff + strlen(segment_buff); p_str++)
                        if (*p_str == '\"')
                            end_quote = true;

                    if (pre_quote && end_quote)
                        break;
                }

                // 回退到typedef之后重新读取
                fseek(target_file, -(strlen(p_typedef + strlen("typedef"))), SEEK_CUR);
                f_seek_skip_comments_and_blanks(target_file);
                if (f_getline(target_file, segment_buff, sizeof(segment_buff)) == 0)
                    break;

                // 检查是否是支持的类型
                if (strstr(segment_buff, "struct") != segment_buff)
                {
                    while (fgetc(target_file) != '}')
                        ;
                    continue;
                }

                // 分配空间，类型链表头为空时初始化类型链表头
                if (type_list_head == nullptr)
                {
                    type_list_head = (type_node *)malloc(sizeof(type_node));
                    target_type_node = type_list_head;
                    type_show_begin_node = type_list_head;
                }
                else
                {
                    // 扩展到下一张链表
                    target_type_node->p_next = (type_node *)malloc(sizeof(type_node));
                    target_type_node = target_type_node->p_next;
                    if (type_show_begin_node == nullptr)
                        type_show_begin_node = target_type_node;
                }

                // 节点初始化
                target_type_node->p_next = nullptr;
                target_type_node->type = TYPE_UNKNOWN;
                target_type_node->element_list_head = nullptr;
                memset(target_type_node->type_name_str, '\0', sizeof(target_type_node->type_name_str));

                // 检测到结构体类型
                if (strstr(segment_buff, "struct"))
                {
                    // 非法子成员记录
                    bool illegal_sub_element = false;

                    target_type_node->type = STRUCTURE;

                    // 回退到struct后重新读取
                    fseek(target_file, -(strlen(segment_buff) - strlen("struct")), SEEK_CUR);
                    f_seek_skip_comments_and_blanks(target_file);

                    // 获取直接类型名称
                    f_getword(target_file, segment_buff, sizeof(segment_buff));
                    if (segment_buff[0] != '\0')
                        sprintf(target_type_node->type_name_str, segment_buff);

                    // 跳转到类型定义内
                    while (fgetc(target_file) != '{')
                        ;

                    // 新建第一个子元素节点
                    sub_element_node *target_element_node = nullptr;

                    // 循环读取类型定义
                    while (true)
                    {
                        if (f_get_codeline(target_file, segment_buff, sizeof(segment_buff)) == 0)
                            break;
                        if (segment_buff[0] == '}')
                        {
                            // 回退到结束末尾
                            fseek(target_file, -(strlen(segment_buff) - 1), SEEK_CUR);
                            break;
                        }

                        // 分配空间及初始化
                        if (target_element_node == nullptr)
                        {
                            target_type_node->element_list_head = (sub_element_node *)malloc(sizeof(sub_element_node));
                            target_element_node = target_type_node->element_list_head;
                        }
                        else
                        {
                            target_element_node->p_next = (sub_element_node *)malloc(sizeof(sub_element_node));
                            target_element_node = target_element_node->p_next;
                        }
                        target_element_node->p_next = nullptr;
                        target_element_node->element_info = solve_variable_info(segment_buff);

                        // 非法子成员检查
                        if (target_element_node->element_info.element_count == 0)
                        {
                            illegal_sub_element = true;
                            break;
                        }
                        if (target_element_node->element_info.type == TYPE_UNKNOWN)
                        {
                            illegal_sub_element = true;
                            break;
                        }
                        if (target_element_node->element_info.type == STRUCTURE)
                        {
                            illegal_sub_element = true;
                            break;
                        }
                    }

                    // 存在成员非法
                    if (illegal_sub_element)
                    {
                        // 释放子元素列表
                        while (target_type_node->element_list_head != nullptr)
                        {
                            sub_element_node *p_element = target_type_node->element_list_head;
                            target_type_node->element_list_head = target_type_node->element_list_head->p_next;
                            free(p_element);
                        }

                        // 释放当前的类型节点
                        if (target_type_node == type_list_head)
                        {
                            free(type_list_head);
                            type_list_head = nullptr;
                            target_type_node = nullptr;
                            type_show_begin_node = nullptr;
                        }
                        else
                        {

                            if (type_show_begin_node == target_type_node)
                                type_show_begin_node = nullptr;

                            type_node *p_prev_type_node = type_list_head;
                            while (p_prev_type_node->p_next != target_type_node)
                                p_prev_type_node = p_prev_type_node->p_next;
                            free(target_type_node);
                            target_type_node = p_prev_type_node;
                            target_type_node->p_next = nullptr;
                        }

                        // 跳过剩余的声明部分
                        while (fgetc(target_file) != '}')
                            ;
                        continue;
                    }

                    // 处理剩余部分的别名串
                    while (true)
                    {
                        f_seek_skip_blanks(target_file);
                        if (f_getword(target_file, segment_buff, sizeof(segment_buff)) == 0)
                        {
                            if (fgetc(target_file) == ';')
                                break;
                            continue;
                        }

                        // 已经有直接名串
                        if (target_type_node->type_name_str[0] != '\0')
                        {
                            // 扩展到下一张链表并记录信息
                            target_type_node->p_next = (type_node *)malloc(sizeof(type_node));
                            target_type_node->p_next->p_next = nullptr;
                            target_type_node->p_next->type = target_type_node->type;
                            target_type_node->p_next->element_list_head = target_type_node->element_list_head;
                            target_type_node = target_type_node->p_next;
                            memset(target_type_node->type_name_str, '\0', sizeof(target_type_node->type_name_str));
                        }
                        sprintf(target_type_node->type_name_str, segment_buff);
                    }
                }
            }

            memset(segment_buff, '\0', sizeof(segment_buff));
        }

        fclose(target_file);

        /**
         * @todo
         * 日志输出逻辑需要大改，现在这个写法太傻逼了，层级越多越傻逼（但是写起来是真的快~
         */

        // 日志输出
        {
            if (target_file_node->p_next == nullptr)
            {
                printf("                              └─%s\n", target_file_node->file_name_str);

                printf("                                    ├─Structure:\n");

                while (true)
                {
                    if (type_show_begin_node == nullptr)
                    {
                        printf("                                    │     └─(NULL)\n");
                        printf("                                    └─Other:\n");
                        printf("                                          └─(NULL)\n");
                        break;
                    }

                    if (type_show_begin_node->p_next == nullptr)
                    {
                        printf("                                    │     └─%s\n", type_show_begin_node->type_name_str);

                        // 子成员打印
                        {
                            sub_element_node *p_element = type_show_begin_node->element_list_head;
                            while (p_element != nullptr)
                            {
                                variable_info *info = &p_element->element_info;
                                if (p_element->p_next != nullptr)
                                {
                                    if (info->element_count == 1)
                                        printf("                                    │           ├─%-15s .%s\n", type_str[info->type], info->name_str);
                                    else
                                        printf("                                    │           ├─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                                }
                                else
                                {
                                    if (info->element_count == 1)
                                        printf("                                    │           └─%-15s .%s\n", type_str[info->type], info->name_str);
                                    else
                                        printf("                                    │           └─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                                }
                                p_element = p_element->p_next;
                            }
                        }

                        printf("                                    └─Other:\n");
                        printf("                                          └─(NULL)\n");
                        type_show_begin_node = nullptr;
                        break;
                    }

                    printf("                                    │     ├─%s\n", type_show_begin_node->type_name_str);

                    // 子成员打印
                    {
                        sub_element_node *p_element = type_show_begin_node->element_list_head;
                        while (p_element != nullptr)
                        {
                            variable_info *info = &p_element->element_info;
                            if (p_element->p_next != nullptr)
                            {
                                if (info->element_count == 1)
                                    printf("                                    │     │     ├─%-15s .%s\n", type_str[info->type], info->name_str);
                                else
                                    printf("                                    │     │     ├─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                            }
                            else
                            {
                                if (info->element_count == 1)
                                    printf("                                    │     │     └─%-15s .%s\n", type_str[info->type], info->name_str);
                                else
                                    printf("                                    │     │     └─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                            }
                            p_element = p_element->p_next;
                        }
                    }

                    type_show_begin_node = type_show_begin_node->p_next;
                }
            }
            else
            {
                printf("                              ├─%s\n", target_file_node->file_name_str);

                printf("                              │     ├─Structure:\n");

                while (true)
                {
                    if (type_show_begin_node == nullptr)
                    {
                        printf("                              │     │     └─(NULL)\n");
                        printf("                              │     └─Other:\n");
                        printf("                              │           └─(NULL)\n");
                        break;
                    }

                    if (type_show_begin_node->p_next == nullptr)
                    {
                        printf("                              │     │     └─%s\n", type_show_begin_node->type_name_str);

                        // 子成员打印
                        {
                            sub_element_node *p_element = type_show_begin_node->element_list_head;
                            while (p_element != nullptr)
                            {
                                variable_info *info = &p_element->element_info;
                                if (p_element->p_next != nullptr)
                                {
                                    if (info->element_count == 1)
                                        printf("                              │     │           ├─%-15s .%s\n", type_str[info->type], info->name_str);
                                    else
                                        printf("                              │     │           ├─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                                }
                                else
                                {
                                    if (info->element_count == 1)
                                        printf("                              │     │           └─%-15s .%s\n", type_str[info->type], info->name_str);
                                    else
                                        printf("                              │     │           └─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                                }
                                p_element = p_element->p_next;
                            }
                        }

                        printf("                              │     └─Other:\n");
                        printf("                              │           └─(NULL)\n");
                        type_show_begin_node = nullptr;
                        break;
                    }

                    printf("                              │     │     ├─%s\n", type_show_begin_node->type_name_str);

                    // 子成员打印
                    {
                        sub_element_node *p_element = type_show_begin_node->element_list_head;
                        while (p_element != nullptr)
                        {
                            variable_info *info = &p_element->element_info;
                            if (p_element->p_next != nullptr)
                            {
                                if (info->element_count == 1)
                                    printf("                              │     │     │     ├─%-15s .%s\n", type_str[info->type], info->name_str);
                                else
                                    printf("                              │     │     │     ├─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                            }
                            else
                            {
                                if (info->element_count == 1)
                                    printf("                              │     │     │     └─%-15s .%s\n", type_str[info->type], info->name_str);
                                else
                                    printf("                              │     │     │     └─%-15s .%s[%d]\n", type_str[info->type], info->name_str, info->element_count);
                            }
                            p_element = p_element->p_next;
                        }
                    }

                    type_show_begin_node = type_show_begin_node->p_next;
                }
            }
        }

        // 切换文件
        target_file_node = target_file_node->p_next;
    }
}

// 记录布局解析（标定量使用）
void solve_record_layout(void)
{
    // 记录布局名串
    const char *record_type[8] =
        {
            "Scalar_UBYTE",
            "Scalar_UWORD",
            "Scalar_ULONG",
            "Scalar_SBYTE",
            "Scalar_SWORD",
            "Scalar_SLONG",
            "Scalar_FLOAT32_IEEE",
            "Scalar_FLOAT64_IEEE",
        };

    // 布局存在与否
    bool find_flag[8] =
        {
            false,
            false,
            false,
            false,
            false,
            false,
            false,
            false,
        };

    if (input_reference_A2L_file != nullptr)
    {

        // 回到文件起始
        fseek(input_reference_A2L_file, 0, SEEK_SET);
        // 段缓冲区
        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 从头到尾检查A2L中的RECORD_LAYOUT类型
        while (f_getline(input_reference_A2L_file, segment_buff, sizeof(segment_buff)) != 0)
        {
            // 当前行为 RECORD_LAYOUT
            if (strstr(segment_buff, "RECORD_LAYOUT"))
            {
                // 指针跳转到RECORD_LAYOUT后第一个有效字符
                char *p_record_str = strstr(segment_buff, "RECORD_LAYOUT") + strlen("RECORD_LAYOUT");
                size_t segment_len = strlen(segment_buff);
                while (*p_record_str != '\n' && p_record_str != segment_buff + segment_len)
                {
                    if (isalpha(*p_record_str) || *p_record_str == '_')
                        break;
                    p_record_str++;
                }

                // 挨个检查
                for (int count = 0; count < 8; count++)
                {
                    if (find_flag[count])
                        continue;
                    if (strstr(p_record_str, record_type[count]))
                        find_flag[count] = true;
                }
            }
        }
    }

    bool all_clear = true;
    for (int count = 0; count < 8; count++)
        if (!find_flag[count])
            all_clear = false;

    // 补全输出
    if (!all_clear)
    {
        fprintf(output_middleware_file, "\r\n\r\n");
        fprintf(output_middleware_file, "%s\r\n\r\n\r\n", START_OF_GENERATED_RECORD_LAYOUT_STR);

        for (int count = 0; count < 8; count++)
        {
            if (find_flag[count])
                continue;
            log_printf(LOG_SUCCESS, "Add missing record layout \"%s\"", record_type[count]);

            fprintf(output_middleware_file, "/begin RECORD_LAYOUT    %s\r\n", record_type[count]);
            fprintf(output_middleware_file, "    FNC_VALUES     1    %s COLUMN_DIR DIRECT\r\n", record_type[count] + strlen("Scalar_"));
            fprintf(output_middleware_file, "/end   RECORD_LAYOUT\r\n\r\n");
        }

        fprintf(output_middleware_file, "\r\n");
        fprintf(output_middleware_file, "%s\r\n", END_OF_GENERATED_RECORD_LAYOUT_STR);
    }
}

// 处理标定量和观测量
void solve_calibrations_and_measurements(void)
{
    // 抬头输出
    bool head_output = false;

    file_node *target_file_node = source_and_header_file_list_head;

    // 循环处理输入文件
    while (target_file_node != nullptr)
    {
        printf("\n\n");
        log_printf(LOG_INFO, "Start solving file: \"%s\"\n", target_file_node->file_name_str);
        FILE *target_file = fopen(target_file_node->file_name_str, "rb");
        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        // 循环获取输入文件行
        while (f_getline(target_file, segment_buff, sizeof(segment_buff)) != 0)
        {
            // 检测到标定量起始行
            if (strstr(segment_buff, START_OF_CALIBRATION_PATTERN_STR))
            {
                if (!head_output)
                {
                    fprintf(output_middleware_file, "\r\n\r\n%s\r\n\r\n\r\n", START_OF_GENERATED_CALIBRATION_AND_MEASURMENT_STR);
                    head_output = true;
                }

                do
                {
                    // 跳过空白部分
                    f_seek_skip_comments_and_blanks(target_file);
                    size_t read_count = f_getline(target_file, segment_buff, sizeof(segment_buff));
                    if (strstr(segment_buff, END_OF_CALIBRATION_PATTERN_STR))
                        break;
                    // 回退行并读取代码行
                    fseek(target_file, -read_count, SEEK_CUR);
                    f_get_codeline(target_file, segment_buff, sizeof(segment_buff));

                    variable_info v_info = solve_variable_info(segment_buff);

                    f_print_calibration(output_middleware_file, v_info);
                } while (true);
            }

            // 观测量起始行
            if (strstr(segment_buff, START_OF_MEASURMENT_PATTERN_STR))
            {
                if (!head_output)
                {
                    fprintf(output_middleware_file, "\r\n\r\n%s\r\n\r\n", START_OF_GENERATED_CALIBRATION_AND_MEASURMENT_STR);
                    head_output = true;
                }

                do
                {
                    // 跳过空白部分
                    f_seek_skip_comments_and_blanks(target_file);
                    size_t read_count = f_getline(target_file, segment_buff, sizeof(segment_buff));
                    if (strstr(segment_buff, END_OF_MEASURMENT_PATTERN_STR))
                        break;
                    // 回退行并读取代码行
                    fseek(target_file, -read_count, SEEK_CUR);
                    f_get_codeline(target_file, segment_buff, sizeof(segment_buff));
                    // log_printf(LOG_INFO, segment_buff);
                    f_print_measurement(output_middleware_file, solve_variable_info(segment_buff));
                } while (true);
            }
        }

        printf("\n");
        log_printf(LOG_INFO, "File: \"%s\" solve finished.", target_file_node->file_name_str);
        fclose(target_file);
        target_file_node = target_file_node->p_next;
    }

    if (head_output)
        fprintf(output_middleware_file, "\r\n\r\n%s\r\n\r\n", END_OF_GENERATED_CALIBRATION_AND_MEASURMENT_STR);
}

// 处理最终A2L合并输出
void solve_A2L_merge(void)
{
    // 回到文件起始
    fseek(input_reference_A2L_file, 0, SEEK_SET);

    // 段缓冲区
    char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

    // 读取并复制参考文件直到 a2l MODULE 结尾
    while (f_getline(input_reference_A2L_file, segment_buff, sizeof(segment_buff)) != 0)
    {
        // 当前行为 a2l MODULE 结尾
        if (strstr(segment_buff, A2L_INSERT_PATTERN_STR))
        {
            // 回退文件指针到上一行结尾
            fseek(input_reference_A2L_file, -2, SEEK_CUR);
            while (fgetc(input_reference_A2L_file) != '\n')
                fseek(input_reference_A2L_file, -2, SEEK_CUR);
            break;
        }

        // 输出行到文件
        // fprintf(Output_Target, segment_buff);    // 太大会段溢出,标准输入输出库存在的问题
        for (int count = 0; count < SEGMENT_BUFF_LENGTH; count++) // 逐个输出
        {
            fputc(segment_buff[count], output_target_A2L_file);
            if (segment_buff[count] == '\n')
                break;
        }

        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }

    // 清空缓冲区并回退到文件起始位置
    fseek(output_middleware_file, 0, SEEK_SET);

    char ch = fgetc(output_middleware_file);
    while (ch != EOF)
    {
        fputc(ch, output_target_A2L_file);
        ch = fgetc(output_middleware_file);
    }

    // 输出参考文件的剩余部分
    while (f_getline(input_reference_A2L_file, segment_buff, sizeof(segment_buff)) != 0)
    {
        // 输出行到文件
        fprintf(output_target_A2L_file, segment_buff);
        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }
}

// end