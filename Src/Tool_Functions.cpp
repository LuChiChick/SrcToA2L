#include "Tool_Functions.hpp"
#include "Global_Variables.hpp"

extern "C"
{
#include "string.h"
#include "ctype.h"
#include "stdarg.h"
}

// 读取文件下一行
error_type_enum f_getline(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len)
{
    if (buffer == nullptr || file == nullptr)
    {
        *seek_len = 0;
        return ERROR_ILLEGAL_POINTER;
    }

    memset(buffer, '\0', buffer_len);

    size_t count = 0;

    while (true)
    {
        char ch = fgetc(file);

        if (count >= buffer_len) // 超长
        {
            fseek(file, -count, SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_OUT_OF_LENGTH;
        }

        if (ch == EOF && count == 0) // 仅文件结尾时
        {
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_END_OF_FILE;
        }

        if (ch == '\n' || ch == EOF) // 成功换行or文件结尾
        {
            buffer[count] = '\n';
            if (seek_len != nullptr)
                *seek_len = count + 1;
            return ERROR_NONE;
        }

        buffer[count] = ch;
        count++;
    }
}

// 获取文件代码行(以;为分界的代码逻辑行，忽略中途的注释)
error_type_enum f_get_codeline(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len)
{
    if (buffer == nullptr || file == nullptr)
    {
        *seek_len = 0;
        return ERROR_ILLEGAL_POINTER;
    }

    memset(buffer, '\0', buffer_len);

    size_t skip_count = 0;

    // 跳过空白内容和注释
    while (true)
    {
        skip_count += f_seek_skip_blank(file);

        // 忽略注释内容
        if (fgetc(file) == '/')
        {
            skip_count++;
            // 是单行注释，连续遇到两个'/'
            if (fgetc(file) == '/')
            {
                skip_count++;
                skip_count += f_seek_nextline(file);
            }
            // 是多行注释，直接跳过中间内容直到遇到下一个'/'
            else
            {
                while (fgetc(file) != '/')
                    skip_count++;
                skip_count++;
            }
        }
        else
        {
            fseek(file, -1, SEEK_CUR);
            break;
        }
    }

    size_t code_line_count = 0;
    while (true)
    {
        char ch = fgetc(file);

        if (code_line_count >= buffer_len) // 超长
        {
            fseek(file, -(skip_count + code_line_count), SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_OUT_OF_LENGTH;
        }

        if (ch == EOF && code_line_count == 0) // 仅文件结尾时
        {
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_END_OF_FILE;
        }

        if (ch == ';') // 代码结尾
        {
            buffer[code_line_count] = ';';
            if (seek_len != nullptr)
                *seek_len = code_line_count + skip_count + 1;
            return ERROR_NONE;
        }

        if (ch == EOF) // 文件结尾
        {
            buffer[code_line_count] = '\0';
            if (seek_len != nullptr)
                *seek_len = code_line_count + skip_count;
            return ERROR_NONE;
        }

        buffer[code_line_count] = ch;
        code_line_count++;
    }
}

// 读取下一个有效词组
error_type_enum f_getword(FILE *file, char *buffer, const size_t buffer_len, size_t *seek_len)
{
    if (buffer == nullptr || file == nullptr)
    {
        *seek_len = 0;
        return ERROR_ILLEGAL_POINTER;
    }

    size_t read_count = 0;
    // 过滤空白
    while (true)
    {
        char ch = fgetc(file);
        if (read_count >= buffer_len) // 超长
        {
            fseek(file, -read_count, SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_OUT_OF_LENGTH;
        }

        if (ch == EOF && read_count == 0) // 仅文件结尾时
        {
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_END_OF_FILE;
        }

        // 过滤空白
        if (ch == ' ' || ch == '\r' || ch == '\n')
        {
            read_count++;
            continue;
        }

        // 遇到非空格非换行的有效字符
        fseek(file, -1, SEEK_CUR);
        break;
    }

    size_t count = 0;
    memset(buffer, '\0', buffer_len);

    while (true)
    {
        char ch = fgetc(file);

        if (read_count >= buffer_len) // 超长
        {
            fseek(file, -read_count, SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_OUT_OF_LENGTH;
        }

        if (ch == EOF && count == 0) // 仅文件结尾时
        {
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_END_OF_FILE;
        }

        if (!isalnum(ch) && ch != '_' && count == 0) // 遇到的第一个就是非有效字符
        {
            fseek(file, -(read_count + 1), SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = 0;
            return ERROR_ILLEGAL_WORD_SECTION;
        }

        if (!isalnum(ch) && ch != '_') // 非字符类
        {
            // 回退指针后正常返回
            fseek(file, -1, SEEK_CUR);
            if (seek_len != nullptr)
                *seek_len = read_count;
            return ERROR_NONE;
        }

        buffer[count] = ch;
        count++;
        read_count++;
    }
}

// 前进到下一行
size_t f_seek_nextline(FILE *file)
{
    size_t count = 0;
    while (true)
    {
        char ch = fgetc(file);
        if (ch == '\n' || ch == EOF)
            break;

        count++;
    }

    return count;
}

// 跳转到下一个非空字符
size_t f_seek_skip_blank(FILE *file)
{
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

// 基础变量类型解析
variable_type_enum get_variable_base_type(const char *str)
{
    if (!strcmp(str, "bool"))
        return UBYTE;
    if (!strcmp(str, "boolean_t"))
        return UBYTE;

    if (!strcmp(str, "uint8_t"))
        return UBYTE;
    if (!strcmp(str, "uint16_t"))
        return UWORD;
    if (!strcmp(str, "uint32_t"))
        return ULONG;

    if (!strcmp(str, "int8_t"))
        return SBYTE;
    if (!strcmp(str, "int16_t"))
        return SWORD;
    if (!strcmp(str, "int32_t"))
        return SLONG;

    if (!strcmp(str, "float"))
        return FLOAT32;
    if (!strcmp(str, "double"))
        return FLOAT64;

    return TYPE_NOT_SUPPORTED;
}

// 基础变量解析
variable_info solve_base_variable(const char *str)
{
    variable_info info;

    // 读取前段内容并获取元素类型
    char buff[VARIABLE_NAME_LENGTH_MAX] = {'\0'};
    size_t offset = 0;

    // 跳过前方的修饰段
    while (true)
    {
        sscanf(str + offset, "%s", buff);

        // 不是下列任何的前置修饰符号时跳出
        if (!(!strcmp("const", buff) || !strcmp("static", buff) || !strcmp("volatile", buff)))
            break;

        // 是修饰段
        while (str[offset] != ' ' && str[offset] != '\0') // 跳过当前词组段
            offset++;
        while (!isalnum(str[offset]) && str[offset] != '\0') // 前进到有效字符
            offset++;

        memset(buff, '\0', sizeof(buff));
    }

    info.type = get_variable_base_type(buff);

    // 更新字符串信息
    switch (info.type)
    {
    case UBYTE:
        sprintf(info.A2L_type_str, "UBYTE");
        sprintf(info.A2L_min_limit_str, "0");
        sprintf(info.A2L_max_limit_str, "255");
        info.single_element_size = 1;
        break;
    case UWORD:
        sprintf(info.A2L_type_str, "UWORD");
        sprintf(info.A2L_min_limit_str, "0");
        sprintf(info.A2L_max_limit_str, "65535");
        info.single_element_size = 2;
        break;
    case ULONG:
        sprintf(info.A2L_type_str, "ULONG");
        sprintf(info.A2L_min_limit_str, "0");
        sprintf(info.A2L_max_limit_str, "4294967295");
        info.single_element_size = 4;
        break;
    case SBYTE:
        sprintf(info.A2L_type_str, "SBYTE");
        sprintf(info.A2L_min_limit_str, "-128");
        sprintf(info.A2L_max_limit_str, "127");
        info.single_element_size = 1;
    case SWORD:
        sprintf(info.A2L_type_str, "SWORD");
        sprintf(info.A2L_min_limit_str, "-32768");
        sprintf(info.A2L_max_limit_str, "32767");
        info.single_element_size = 2;
        break;
    case SLONG:
        sprintf(info.A2L_type_str, "SLONG");
        sprintf(info.A2L_min_limit_str, "-2147483648");
        sprintf(info.A2L_max_limit_str, "2147483647");
        info.single_element_size = 4;
        break;
    case FLOAT32:
        sprintf(info.A2L_type_str, "FLOAT32_IEEE");
        sprintf(info.A2L_min_limit_str, "-3.4E+38");
        sprintf(info.A2L_max_limit_str, "3.4E+38");
        info.single_element_size = 4;
        break;
    case FLOAT64:
        sprintf(info.A2L_type_str, "FLOAT64_IEEE");
        sprintf(info.A2L_min_limit_str, "-1.7E+308");
        sprintf(info.A2L_max_limit_str, "1.7E+308");
        info.single_element_size = 8;
        break;
    default:
        sprintf(info.A2L_type_str, "UNSUPPORTED");
        sprintf(info.A2L_min_limit_str, "0");
        sprintf(info.A2L_max_limit_str, "0");
        info.single_element_size = 0;
    }

    // 读取后段内容
    sscanf(str + offset + strlen(buff), "%s", buff);

    // 获取名字和长度
    for (int count = 0; count < VARIABLE_NAME_LENGTH_MAX; count++)
    {
        if (buff[count] == '[') // 识别到数组
        {
            for (int n = count + 1; n < VARIABLE_NAME_LENGTH_MAX; n++)
            {
                if (buff[n] == ']')
                    break;
                if (isdigit(buff[n]))
                    info.element_count = info.element_count * 10 + buff[n] - '0';
                else
                {
                    /**
                     * @todo
                     * 添加宏定义识别，先暂时将不支持的量定义为1
                     */
                    // 处理宏定义常量
                    char define_str[VARIABLE_NAME_LENGTH_MAX] = {'\0'};
                    for (int count = 0; count < VARIABLE_NAME_LENGTH_MAX; count++)
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
                            int value = 1;
                            sscanf(target_define_node->context_str, "%d", &value);
                            info.element_count = value;
                            break;
                        }
                        target_define_node = target_define_node->p_next;
                    }
                    // 完全匹配但没有找到
                    if (target_define_node == nullptr)
                    {
                        printf("\n");
                        print_log(LOG_WARN, "%s [%s -> 1]\n", "Unknown value context,set to default:", define_str);
                        printf("\n");
                        info.element_count = 1;
                    }
                    break;
                }
            }

            break;
        }

        if (buff[count] == ' ' || buff[count] == '=' || buff[count] == ';') // 是单个变量
        {
            if (info.element_count == 0)
                info.element_count = 1;
            break;
        }

        info.name_str[count] = buff[count]; // 复制变量名
    }

    return info;
}

// 解析变量类型
variable_type_enum get_variable_type(const char *str)
{
    variable_type_enum type = get_variable_base_type(str);
    if (type != TYPE_NOT_SUPPORTED)
        return type;

    // 遍历复合类型
    type_node *target_node = type_list_head;
    while (target_node != nullptr)
    {
        if (!strcmp(str, target_node->type_name_str))
            return target_node->type;
        target_node = target_node->p_next;
    }

    return TYPE_NOT_SUPPORTED;
}

// 变量解析
variable_info solve_variable(const char *str)
{
    variable_info info;

    char buff[VARIABLE_NAME_LENGTH_MAX] = {'\0'};
    size_t offset = 0;

    // 跳过前方的修饰段
    while (true)
    {
        sscanf(str + offset, "%s", buff);

        // 不是下列任何的前置修饰符号时跳出
        if (!(!strcmp("const", buff) || !strcmp("static", buff) || !strcmp("volatile", buff)))
            break;

        // 是修饰段
        while (str[offset] != ' ' && str[offset] != '\0') // 跳过当前词组段
            offset++;
        while (!isalnum(str[offset]) && str[offset] != '\0') // 前进到有效字符
            offset++;

        memset(buff, '\0', sizeof(buff));
    }

    // 读取前段内容并获取元素类型
    info.type = get_variable_type(buff);

    // 判断是否为结构体类型
    if (info.type == STRUCTURE)
    {
        sprintf(info.A2L_type_str, "STRUCTURE");

        // 匹配类型描述链表位置
        type_node *target_type = type_list_head;
        while (target_type != nullptr)
        {
            if (!strcmp(target_type->type_name_str, buff))
            {
                info.type_name_str = target_type->type_name_str;
                break;
            }
            target_type = target_type->p_next;
        }

        // 读取后段内容
        sscanf(str + offset + strlen(buff), "%s", buff);

        // 获取名字和长度
        for (int count = 0; count < VARIABLE_NAME_LENGTH_MAX; count++)
        {
            if (buff[count] == '[') // 识别到数组
            {
                for (int n = count + 1; n < VARIABLE_NAME_LENGTH_MAX; n++)
                {
                    if (buff[n] == ']')
                        break;
                    if (isdigit(buff[n]))
                        info.element_count = info.element_count * 10 + buff[n] - '0';
                    else
                    {

                        /**
                         * @todo
                         * 添加宏定义识别，先暂时将不支持的量定义为1
                         */
                        // 处理宏定义常量
                        char define_str[VARIABLE_NAME_LENGTH_MAX] = {'\0'};
                        for (int count = 0; count < VARIABLE_NAME_LENGTH_MAX; count++)
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
                                int value = 1;
                                sscanf(target_define_node->context_str, "%d", &value);
                                info.element_count = value;
                                break;
                            }
                            target_define_node = target_define_node->p_next;
                        }
                        // 完全匹配但没有找到
                        if (target_define_node == nullptr)
                        {
                            printf("\n");
                            print_log(LOG_WARN, "%s [%s -> 1]\n", "Unknown value context,set to default:", define_str);
                            printf("\n");
                            info.element_count = 1;
                        }
                        break;
                    }
                }

                break;
            }

            if (buff[count] == ' ' || buff[count] == '=' || buff[count] == ';') // 是单个变量
            {
                if (info.element_count == 0)
                    info.element_count = 1;
                break;
            }

            info.name_str[count] = buff[count]; // 复制变量名
        }

        return info;
    }
    return solve_base_variable(str);
}

// 日志输出
void print_log(log_type_enum log_type, const char *p_format_str, ...)
{
    switch (log_type)
    {
    case LOG_SUCCESS:
        printf("[-  OK  -] ");
        break;
    case LOG_WARN:
        printf("[+ WARN +] ");
        break;
    case LOG_FAILURE:
        printf("[< FAIL >] ");
        break;
    case LOG_NORMAL:
        printf("[  INFO  ] ");
        break;
    case LOG_ERROR:
        printf("[# ERRO #] ");
        break;
    }

    // 传递不定参数
    va_list args;
    va_start(args, p_format_str);
    vprintf(p_format_str, args);
}