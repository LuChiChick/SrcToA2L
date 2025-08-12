#include "Tool_Functions.hpp"
#include "Global_Variables.hpp"

extern "C"
{
#include "string.h"
#include "ctype.h"
#include "stdarg.h"
#include "time.h"
#include "stdlib.h"
}

// 读取文件一行
size_t f_getline(FILE *file, char *buffer, const size_t buffer_len)
{
    if (buffer == nullptr || file == nullptr)
        return 0;

    memset(buffer, '\0', buffer_len);

    size_t count = 0;

    // 循环读取
    while (true)
    {
        // 超长判定
        if (count + 1 == buffer_len)
            return buffer_len;

        // 正常读取
        char ch = fgetc(file);

        // 仅文件结尾时
        if (ch == EOF && count == 0)
            return 0;

        // 成功换行
        if (ch == '\n')
        {
            buffer[count] = '\n';
            return count + 1;
        }

        // 没有换行但是遇到了文件结尾
        if (ch == EOF)
        {
            buffer[count] = '\n';
            return count;
        }

        // 其它情况下正常复制
        buffer[count] = ch;
        count++;
    }
}

// 读取下一个有效词组
size_t f_getword(FILE *file, char *buffer, const size_t buffer_len)
{
    if (buffer == nullptr || file == nullptr)
        return 0;
    size_t read_count = 0;

    // 过滤空白部分
    read_count = f_seek_skip_comments_and_blanks(file);
    memset(buffer, '\0', buffer_len);

    size_t write_count = 0;
    while (true)
    {
        char ch = fgetc(file);

        if (write_count >= buffer_len) // 超长
        {
            fseek(file, -read_count, SEEK_CUR);
            return 0;
        }

        if (ch == EOF && read_count == 0) // 仅文件结尾时
            return 0;

        if (!isalnum(ch) && ch != '_' && read_count == 0) // 遇到的第一个就是非有效字符
        {
            fseek(file, -(read_count + 1), SEEK_CUR);
            return 0;
        }

        if (!isalnum(ch) && ch != '_') // 非字符类
        {
            // 回退指针后正常返回
            fseek(file, -1, SEEK_CUR);
            return read_count;
        }

        buffer[write_count] = ch;
        write_count++;
        read_count++;
    }
}

// 获取文件代码行(以;为分界的代码逻辑行，忽略中途的注释)
size_t f_get_codeline(FILE *file, char *buffer, const size_t buffer_len)
{
    if (buffer == nullptr || file == nullptr)
        return 0;
    size_t read_count = 0;

    // 过滤空白和注释部分
    read_count = f_seek_skip_comments_and_blanks(file);
    memset(buffer, '\0', buffer_len);

    size_t write_count = 0;

    while (true)
    {
        char ch = fgetc(file);

        if (write_count >= buffer_len) // 超长
        {
            fseek(file, -(read_count), SEEK_CUR);
            return 0;
        }

        if (ch == EOF && read_count == 0) // 仅文件结尾时
            return 0;

        if (ch == ';') // 代码结尾
        {
            buffer[write_count] = ';';
            return read_count + 1;
        }

        if (ch == EOF) // 文件结尾
        {
            buffer[write_count] = '\0';
            return read_count;
        }

        buffer[write_count] = ch;
        write_count++;
        read_count++;
    }
}

// 前进到下一行
size_t f_seek_nextline(FILE *file)
{
    if (file == nullptr)
        return 0;

    size_t count = 0;
    while (true)
    {
        char ch = fgetc(file);

        if (ch == '\n')
        {
            count++;
            break;
        }
        if (ch == EOF)
            break;

        count++;
    }

    return count;
}

// 跳转到下一个非空字符
size_t f_seek_skip_blanks(FILE *file)
{
    if (file == nullptr)
        return 0;

    size_t count = 0;
    while (true)
    {
        char ch = fgetc(file);
        if (ch == ' ' || ch == '\r' || ch == '\n')
        {
            count++;
            continue;
        }

        if (ch == EOF)
            return count;

        break;
    }

    // 回退指针
    fseek(file, -1, SEEK_CUR);
    return count;
}

// 跳过注释和空白内容(不跳过识别段)
size_t f_seek_skip_comments_and_blanks(FILE *file)
{
    if (file == nullptr)
        return 0;

    size_t count = 0;

    bool find_pattern_section = false;

    while (true)
    {
        // 跳转到下一个有效字符
        count += f_seek_skip_blanks(file);
        char ch = fgetc(file);

        // 检测到注释内容
        if (ch == '/')
        {
            ch = fgetc(file);

            // 是单行注释,直接跳转行
            if (ch == '/')
            {
                fseek(file, -2, SEEK_CUR);
                count += f_seek_nextline(file);
            }
            // 是多行注释，判断是不是识别段（识别段在一行内结束）
            else
            {
                // 回退读行
                fseek(file, -2, SEEK_CUR);
                char segment_buff[SEGMENT_BUFF_LENGTH] = "\0";
                size_t read_length = f_getline(file, segment_buff, sizeof(segment_buff));

                // 目标段正匹配且在起始位置
                if (strstr(segment_buff, START_OF_MEASURMENT_PATTERN_STR) == segment_buff)
                    find_pattern_section = true;
                if (strstr(segment_buff, END_OF_MEASURMENT_PATTERN_STR) == segment_buff)
                    find_pattern_section = true;
                if (strstr(segment_buff, START_OF_CALIBRATION_PATTERN_STR) == segment_buff)
                    find_pattern_section = true;
                if (strstr(segment_buff, END_OF_CALIBRATION_PATTERN_STR) == segment_buff)
                    find_pattern_section = true;

                // 发现匹配段
                if (find_pattern_section)
                {
                    fseek(file, -read_length, SEEK_CUR);
                    break;
                }
                else
                    fseek(file, -(read_length - 2), SEEK_CUR); // 回退到第第二位，避免单行的多行注释引发的问题

                // 读取到多行注释结束位置
                while (fgetc(file) != '/')
                    count++;
                count++;

                // 继续下个处理循环
                continue;
            }
        }
        // 文件结尾
        else if (ch == EOF)
            break;
        // 非注释内容
        else
        {
            // 回退指针并退出
            fseek(file, -1, SEEK_CUR);
            break;
        }
    }

    return count;
}

// 解析变量类型
variable_type_enum solve_variable_type(const char *type_str)
{
    // 基础类型解析
    {
        if (!strcmp(type_str, "bool"))
            return UBYTE;
        if (!strcmp(type_str, "boolean_t"))
            return UBYTE;

        if (!strcmp(type_str, "uint8_t"))
            return UBYTE;

        if (!strcmp(type_str, "uint16_t"))
            return UWORD;

        if (!strcmp(type_str, "uint32_t"))
            return ULONG;

        if (!strcmp(type_str, "int8_t"))
            return SBYTE;

        if (!strcmp(type_str, "int16_t"))
            return SWORD;

        if (!strcmp(type_str, "int"))
            return SLONG;
        if (!strcmp(type_str, "int32_t"))
            return SLONG;

        if (!strcmp(type_str, "float"))
            return FLOAT32;
        if (!strcmp(type_str, "double"))
            return FLOAT64;
    }

    // 遍历复合类型
    type_node *target_node = type_list_head;
    while (target_node != nullptr)
    {
        if (!strcmp(type_str, target_node->type_name_str))
            return target_node->type;
        target_node = target_node->p_next;
    }

    return TYPE_UNKNOWN;
}

// 变量解析
variable_info solve_variable_info(const char *code_line_str)
{
    variable_info info;

    char buff[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'};
    size_t offset = 0;
    // 跳过前方的修饰段
    while (true)
    {
        sscanf(code_line_str + offset, "%s", buff);

        // 不是下列任何的前置修饰符号时跳出
        if (!(!strcmp("const", buff) || !strcmp("static", buff) || !strcmp("volatile", buff)))
            break;

        // 跳过当前修饰词
        while (code_line_str[offset] != ' ' && code_line_str[offset] != '\0')
            offset++;
        // 跳过修饰词后的空白字符,前进到有效字符(字母或下划线开头的变量名)
        while (!(isalpha(code_line_str[offset]) || code_line_str[offset] == '_') && code_line_str[offset] != '\0')
            offset++;

        memset(buff, '\0', sizeof(buff));
    }

    // 读取前段内容并获取元素类型
    info.type = solve_variable_type(buff);
    sprintf(info.type_name_str, buff);

    // 读取后段内容
    sscanf(code_line_str + offset + strlen(buff), "%s", buff);

    // 获取名字和长度
    for (int count = 0; count < VARIABLE_NAME_STR_LENGTH_MAX; count++)
    {
        if (buff[count] == '[') // 识别到数组
        {
            for (int n = count + 1; n < VARIABLE_NAME_STR_LENGTH_MAX; n++)
            {
                // 到达数组定义结尾
                if (buff[n] == ']')
                    break;
                if (isdigit(buff[n]))
                    info.element_count = info.element_count * 10 + buff[n] - '0';
                else
                {
                    // 处理宏定义常量,暂时将不支持的量定义为1

                    // 提取宏字符串
                    char define_str[VARIABLE_NAME_STR_LENGTH_MAX] = {'\0'};
                    for (int count = 0; count < VARIABLE_NAME_STR_LENGTH_MAX; count++)
                        if (buff[n + count] == ']')
                            break;
                        else
                            define_str[count] = buff[n + count];

                    // 进行匹配
                    define_node *target_define_node = define_list_head;
                    while (target_define_node != nullptr)
                    {
                        // 找到宏定义了
                        if (!strcmp(target_define_node->define_str, define_str))
                        {
                            info.element_count = target_define_node->value;
                            break;
                        }
                        target_define_node = target_define_node->p_next;
                    }
                    // 遍历完成但没有找到
                    if (target_define_node == nullptr)
                    {
                        // printf("\n");
                        // log_printf(LOG_WARN, "%s [%s -> 1]\n", "Unknown value context,set to default:", define_str);
                        // printf("\n");
                        info.element_count = 0;
                    }
                    break;
                }
            }

            break;
        }

        if (buff[count] == ' ' || buff[count] == '=' || buff[count] == ';') // 是单个变量
        {
            info.element_count = 1;
            break;
        }

        info.name_str[count] = buff[count]; // 复制变量名
    }

    // 不支持类型直接回传
    if (info.type == TYPE_UNKNOWN)
        info.element_count = 0;

    return info;
}

// 获取变量地址
uint32_t get_variable_addr32(const char *v_name_str)
{
    uint32_t addr32 = 0;
    // 查找地址
    if (input_map_file != nullptr)
    {
        // 回退到起始位置
        fseek(input_map_file, 0, SEEK_SET);
        char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

        bool find = false;
        // 循环读行
        while (f_getline(input_map_file, segment_buff, sizeof(segment_buff)) != 0 && !find)
        {
            // 匹配到变量名称
            if (strstr(segment_buff, v_name_str))
            {
                char *lpTarget = strstr(segment_buff, v_name_str);
                size_t len = strlen(v_name_str);

                // 匹配到的名称是后段中的子段内容---> xxxxx[name]xx
                if (lpTarget[len] != ' ' && lpTarget[len] != '\r' && lpTarget[len] != '\n')
                    continue;
                // 匹配到的名称是前段中的字段内容---> xx[name]xxxxx
                if (*(lpTarget - 1) != '_' && *(lpTarget - 1) != ' ' && *(lpTarget - 1) != '@')
                    continue;

                find = true;

                // 回退指针到地址串起始位置
                while (*(lpTarget - 1) != ' ')
                    lpTarget--;
                while (*(lpTarget - 1) == ' ')
                    lpTarget--;
                while (*(lpTarget - 1) != ' ')
                    lpTarget--;

                char addr_str[20];
                sscanf(lpTarget, "%s", addr_str);

                // 16进制转换
                for (int count = 0; count < 20; count++)
                {
                    if (!isalnum(addr_str[count]))
                        break;

                    if (isalpha(addr_str[count]))
                        addr32 = addr32 * 16 + toupper(addr_str[count]) - 'A' + 10;
                    else
                        addr32 = addr32 * 16 + addr_str[count] - '0';
                }
            }
        }
    }

    return addr32;
}

// 日志输出
void log_printf(log_type_enum log_type, const char *p_format_str, ...)
{
    time_t timestamp;
    time(&timestamp);
    struct tm *timeinfo;
    timeinfo = localtime(&timestamp);

    // 输出时间戳
    printf("[%02d:%02d:%02d]@", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    // 输出日志类型
    switch (log_type)
    {
    case LOG_SUCCESS:
        printf("[-    OK    -]  ");
        break;
    case LOG_WARN:
        printf("[+   WARN   +]  ");
        break;
    case LOG_FAILURE:
        printf("[<   FAIL   >]  ");
        break;
    case LOG_INFO:
        printf("[    INFO    ]  ");
        break;
    case LOG_SYS_INFO:
        printf("[# SYS_INFO #]  ");
        break;
    }

    // 传递不定参数
    va_list args;
    va_start(args, p_format_str);
    vprintf(p_format_str, args);
    printf("\n");
}

// 清理和退出
void clean_and_exit(int exit_code)
{
    // 关闭文件
    fclose(input_reference_A2L_file);
    fclose(input_map_file);
    fclose(output_target_A2L_file);
    fclose(output_middleware_file);

    // 释放分配的全局内存
    {
        type_node *p_type_node = type_list_head;
        while (p_type_node != nullptr)
        {
            type_node *temp = p_type_node;
            p_type_node = p_type_node->p_next;
            free(temp);
        }

        define_node *p_define_node = define_list_head;
        while (p_define_node != nullptr)
        {
            define_node *temp = p_define_node;
            p_define_node = p_define_node->p_next;
            free(temp);
        }

        file_node *p_file_node = source_and_header_file_list_head;
        while (p_file_node != nullptr)
        {
            file_node *temp = p_file_node;
            p_file_node = p_file_node->p_next;
            free(temp);
        }
    }

    log_printf(LOG_SYS_INFO, "All resources cleaned.Now exit.");
    exit(exit_code);
}

// 输出标定量
void f_print_calibration(FILE *file, variable_info v_info)
{
    // 类型名称
    const char *type_str[] = {
        "TYPE_UNKNOWN", // 未知类型
        "STRUCTURE",    // 结构体类型
        "UBYTE",        // uint8_t,bool,boolean_t
        "UWORD",        // uint16_t
        "ULONG",        // uint32_t
        "SBYTE",        // int8_t
        "SWORD",        // int16_t
        "SLONG",        // int32_t
        "FLOAT32_IEEE", // float
        "FLOAT64_IEEE", // double
    };

    // 下限字符串
    const char *min_str[] = {
        "0",           // 未知类型
        "0",           // 结构体类型
        "0",           // uint8_t,bool,boolean_t
        "0",           // uint16_t
        "0",           // uint32_t
        "-128",        // int8_t
        "-32768",      // int16_t
        "-2147483648", // int32_t
        "-3.4E+38",    // float
        "-1.7E+308",   // double
    };

    // 上限字符串
    const char *max_str[] = {
        "0",          // 未知类型
        "0",          // 结构体类型
        "255",        // uint8_t,bool,boolean_t
        "65535",      // uint16_t
        "4294967295", // uint32_t
        "127",        // int8_t
        "32767",      // int16_t
        "2147483647", // int32_t
        "3.4E+38",    // float
        "1.7E+308",   // double
    };

    // 类型长度
    const size_t type_size[] = {
        0, // 未知类型
        0, // 结构体类型
        1, // uint8_t,bool,boolean_t
        2, // uint16_t
        4, // uint32_t
        1, // int8_t
        2, // int16_t
        4, // int32_t
        4, // float
        8, // double
    };

    // 获取地址
    uint32_t start_addr_32 = get_variable_addr32(v_info.name_str);
    uint32_t addr_offset = 0;   // 地址偏移
    uint32_t alignment_max = 0; // 出现的最大对齐（用于结构体末尾补齐空位）

    // 遍历每个元素
    for (size_t count = 0; count < v_info.element_count; count++)
    {
        // 匹配类型描述链表位置
        type_node *target_type = type_list_head;
        // 数组子元素列表
        sub_element_node *element_node = nullptr;

        // 结构体类型时查找类型链表
        if (v_info.type == STRUCTURE)
        {
            while (target_type != nullptr)
            {
                if (!strcmp(target_type->type_name_str, v_info.type_name_str))
                    break;
                target_type = target_type->p_next;
            }
            element_node = target_type->element_list_head;
        }

        // 输出名称及类型
        char *out_name = nullptr;
        variable_type_enum out_type = TYPE_UNKNOWN;

        do
        {
            size_t sub_element_count = 1;
            // 分配名称空间
            if (v_info.type == STRUCTURE)
            {
                out_name = (char *)malloc(strlen(v_info.name_str) + strlen(element_node->element_info.name_str) + 10);
                sub_element_count = element_node->element_info.element_count;
                out_type = element_node->element_info.type;
            }
            else
            {
                out_name = (char *)malloc(strlen(v_info.name_str) + 10);
                out_type = v_info.type;
            }

            for (size_t sub_count = 0; sub_count < sub_element_count; sub_count++)
            {
                // 计算出现的最大对齐
                if (alignment_max < (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size))
                    alignment_max = (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size);

                // 地址对齐
                if (addr_offset % (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size) != 0)
                    addr_offset += (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size) - (addr_offset % (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size));

                // 拼接输出名称
                if (v_info.type == STRUCTURE)
                {
                    if (v_info.element_count == 1)
                    {
                        if (sub_element_count == 1)
                            sprintf(out_name, "%s.%s", v_info.name_str, element_node->element_info.name_str);
                        else
                            sprintf(out_name, "%s.%s[%d]", v_info.name_str, element_node->element_info.name_str, sub_count);
                    }
                    else
                    {
                        if (sub_element_count == 1)
                            sprintf(out_name, "%s[%d].%s", v_info.name_str, count, element_node->element_info.name_str);
                        else
                            sprintf(out_name, "%s[%d].%s[%d]", v_info.name_str, count, element_node->element_info.name_str, sub_count);
                    }
                }
                else
                {
                    if (v_info.element_count == 1)
                        sprintf(out_name, "%s", v_info.name_str);
                    else
                        sprintf(out_name, "%s[%d]", v_info.name_str, count);
                }

                fprintf(output_middleware_file, "%s\r\n", "/begin CHARACTERISTIC");                                             // 标定量头
                fprintf(output_middleware_file, "    /* Name                   */    %s\r\n", out_name);                        // 名称
                fprintf(output_middleware_file, "    /* Long Identifier        */    \"Auto generated by SrcToA2L\"\r\n");      // 描述
                fprintf(output_middleware_file, "    /* Type                   */    %s\r\n", "VALUE");                         // 值类型(数值、数组、曲线)
                fprintf(output_middleware_file, "    /* ECU Address            */    0x%08X\r\n", start_addr_32 + addr_offset); // ECU 地址
                fprintf(output_middleware_file, "    /* Record Layout          */    Scalar_%s\r\n", type_str[out_type]);       // 数据类型
                fprintf(output_middleware_file, "    /* Maximum Difference     */    %s\r\n", "0");                             // 允许最大差分
                fprintf(output_middleware_file, "    /* Conversion Method      */    %s\r\n", "NO_COMPU_METHOD");               // 转换式(保留原始值)
                fprintf(output_middleware_file, "    /* Lower Limit            */    %s\r\n", min_str[out_type]);               // 下限
                fprintf(output_middleware_file, "    /* Upper Limit            */    %s\r\n", max_str[out_type]);               // 上限
                fprintf(output_middleware_file, "%s\r\n\r\n", "/end CHARACTERISTIC");                                           // 标定量尾

                // 递增地址偏移
                addr_offset += type_size[out_type];
            }

            free(out_name);

            if (element_node != nullptr)
                element_node = element_node->p_next;
        } while (element_node != nullptr);

        // 补齐结构体末尾的空余字节偏移
        if (v_info.type == STRUCTURE)
            if (addr_offset % (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size) != 0)
                addr_offset += (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size) - (addr_offset % (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size));
    }

    // 输出日志
    if (start_addr_32 != 0 || input_map_file == nullptr)
    {
        if (v_info.element_count > 1)
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_SUCCESS, "%-15s %-15s 0x%08X+%08X    %s[%d]", "Calibration", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str, v_info.element_count);
        else
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_SUCCESS, "%-15s %-15s 0x%08X+%08X    %s", "Calibration", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str);
    }
    else
    {
        if (v_info.element_count > 1)
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_WARN, "%-15s %-15s 0x%08X    %s[%d]", "Calibration", type_str[v_info.type], start_addr_32, v_info.name_str, v_info.element_count);
        else
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_WARN, "%-15s %-15s 0x%08X    %s", "Calibration", type_str[v_info.type], start_addr_32, v_info.name_str);
    }
}

// 输出观测量
void f_print_measurement(FILE *file, variable_info v_info)
{
    // 类型名称
    const char *type_str[] = {
        "TYPE_UNKNOWN", // 未知类型
        "STRUCTURE",    // 结构体类型
        "UBYTE",        // uint8_t,bool,boolean_t
        "UWORD",        // uint16_t
        "ULONG",        // uint32_t
        "SBYTE",        // int8_t
        "SWORD",        // int16_t
        "SLONG",        // int32_t
        "FLOAT32_IEEE", // float
        "FLOAT64_IEEE", // double
    };

    // 下限字符串
    const char *min_str[] = {
        "0",           // 未知类型
        "0",           // 结构体类型
        "0",           // uint8_t,bool,boolean_t
        "0",           // uint16_t
        "0",           // uint32_t
        "-128",        // int8_t
        "-32768",      // int16_t
        "-2147483648", // int32_t
        "-3.4E+38",    // float
        "-1.7E+308",   // double
    };

    // 上限字符串
    const char *max_str[] = {
        "0",          // 未知类型
        "0",          // 结构体类型
        "255",        // uint8_t,bool,boolean_t
        "65535",      // uint16_t
        "4294967295", // uint32_t
        "127",        // int8_t
        "32767",      // int16_t
        "2147483647", // int32_t
        "3.4E+38",    // float
        "1.7E+308",   // double
    };

    // 类型长度
    const size_t type_size[] = {
        0, // 未知类型
        0, // 结构体类型
        1, // uint8_t,bool,boolean_t
        2, // uint16_t
        4, // uint32_t
        1, // int8_t
        2, // int16_t
        4, // int32_t
        4, // float
        8, // double
    };

    // 获取地址
    uint32_t start_addr_32 = get_variable_addr32(v_info.name_str);
    uint32_t addr_offset = 0;   // 地址偏移
    uint32_t alignment_max = 0; // 出现的最大对齐（用于结构体末尾补齐空位）

    // 遍历每个元素
    for (size_t count = 0; count < v_info.element_count; count++)
    {
        // 匹配类型描述链表位置
        type_node *target_type = type_list_head;
        // 数组子元素列表
        sub_element_node *element_node = nullptr;

        // 结构体类型时查找类型链表
        if (v_info.type == STRUCTURE)
        {
            while (target_type != nullptr)
            {
                if (!strcmp(target_type->type_name_str, v_info.type_name_str))
                    break;
                target_type = target_type->p_next;
            }
            element_node = target_type->element_list_head;
        }

        // 输出名称及类型
        char *out_name = nullptr;
        variable_type_enum out_type = TYPE_UNKNOWN;

        do
        {
            size_t sub_element_count = 1;
            // 分配名称空间
            if (v_info.type == STRUCTURE)
            {
                out_name = (char *)malloc(strlen(v_info.name_str) + strlen(element_node->element_info.name_str) + 10);
                sub_element_count = element_node->element_info.element_count;
                out_type = element_node->element_info.type;
            }
            else
            {
                out_name = (char *)malloc(strlen(v_info.name_str) + 10);
                out_type = v_info.type;
            }

            for (size_t sub_count = 0; sub_count < sub_element_count; sub_count++)
            {
                // 计算出现的最大对齐
                if (alignment_max < (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size))
                    alignment_max = (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size);

                // 地址对齐
                if (addr_offset % (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size) != 0)
                    addr_offset += (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size) - (addr_offset % (type_size[out_type] < addr_alignment_size ? type_size[out_type] : addr_alignment_size));

                // 拼接输出名称
                if (v_info.type == STRUCTURE)
                {
                    if (v_info.element_count == 1)
                    {
                        if (sub_element_count == 1)
                            sprintf(out_name, "%s.%s", v_info.name_str, element_node->element_info.name_str);
                        else
                            sprintf(out_name, "%s.%s[%d]", v_info.name_str, element_node->element_info.name_str, sub_count);
                    }
                    else
                    {
                        if (sub_element_count == 1)
                            sprintf(out_name, "%s[%d].%s", v_info.name_str, count, element_node->element_info.name_str);
                        else
                            sprintf(out_name, "%s[%d].%s[%d]", v_info.name_str, count, element_node->element_info.name_str, sub_count);
                    }
                }
                else
                {
                    if (v_info.element_count == 1)
                        sprintf(out_name, "%s", v_info.name_str);
                    else
                        sprintf(out_name, "%s[%d]", v_info.name_str, count);
                }

                fprintf(output_middleware_file, "%s\r\n", "/begin MEASUREMENT");                                                                     // 观测量头
                fprintf(output_middleware_file, "    /* Name                   */    %s\r\n", out_name);                                             // 名称
                fprintf(output_middleware_file, "    /* Long identifier        */    \"Auto generated by SrcToA2L\"\r\n");                           // 描述
                fprintf(output_middleware_file, "    /* Data type              */    %s\r\n", type_str[out_type]);                                   // 数据类型
                fprintf(output_middleware_file, "    /* Conversion method      */    %s\r\n", "NO_COMPU_METHOD");                                    // 转换式(保留原始值)
                fprintf(output_middleware_file, "    /* Resolution (Not used)  */    %s\r\n", "0");                                                  // 分辨率
                fprintf(output_middleware_file, "    /* Accuracy (Not used)    */    %s\r\n", "0");                                                  // 精度误差
                fprintf(output_middleware_file, "    /* Lower limit            */    %s\r\n", min_str[out_type]);                                    // 下限
                fprintf(output_middleware_file, "    /* Upper limit            */    %s\r\n", max_str[out_type]);                                    // 上限
                fprintf(output_middleware_file, "    /* ECU Address            */    %s    0x%08X\r\n", "ECU_ADDRESS", start_addr_32 + addr_offset); // ECU 地址
                fprintf(output_middleware_file, "%s\r\n\r\n", "/end MEASUREMENT");                                                                   // 观测量尾

                // 递增地址偏移
                addr_offset += type_size[out_type];
            }

            free(out_name);

            if (element_node != nullptr)
                element_node = element_node->p_next;
        } while (element_node != nullptr);

        // 补齐结构体末尾的空余字节偏移
        if (v_info.type == STRUCTURE)
            if (addr_offset % (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size) != 0)
                addr_offset += (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size) - (addr_offset % (alignment_max < addr_alignment_size ? alignment_max : addr_alignment_size));
    }

    // 输出日志
    if (start_addr_32 != 0 || input_map_file == nullptr)
    {
        if (v_info.element_count > 1)
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_SUCCESS, "%-15s %-15s 0x%08X+%08X    %s[%d]", "Measurement", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str, v_info.element_count);
        else
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_SUCCESS, "%-15s %-15s 0x%08X+%08X    %s", "Measurement", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str);
    }
    else
    {
        if (v_info.element_count > 1)
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_WARN, "%-15s %-15s 0x%08X+%08X    %s[%d]", "Measurement", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str, v_info.element_count);
        else
            log_printf(v_info.type == TYPE_UNKNOWN ? LOG_FAILURE : LOG_WARN, "%-15s %-15s 0x%08X+%08X    %s", "Measurement", type_str[v_info.type], start_addr_32, addr_offset, v_info.name_str);
    }
}

// End