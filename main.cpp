#include <iostream>

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
}

#define SEGMENT_BUFF_LENGTH 1000        // 行段长度
#define ELEMENT_INFO_NAME_LENGTH 50     // 变量名长度上限
#define ELEMENT_INFO_TYPE_STR_LENTH 20  // 变量类型字符串长度上限
#define ELEMENT_INFO_LIMIT_STR_LENTH 20 // 变量上下限字符串长度上限

// 输入输出文件指针
FILE *Input_Source = nullptr;
FILE *Input_Reference = nullptr;
FILE *Input_Map = nullptr;
FILE *Output_Target = nullptr;

// 相关识别串
const char measurement_begin[] = "// ##begin_measurement";
const char measurement_end[] = "// ##end_measurement";
const char calibration_begin[] = "// ##begin_calibration";
const char calibration_end[] = "// ##end_calibration";
const char a2l_module_end[] = "/end MODULE";

// 段输出标识串
const char auto_generated_measurement_start[] = "\r\n/***********   Start of auto generated measurement blocks    ***********/\r\n\r\n";
const char auto_generated_measurement_end[] = "\r\n/***********   End of auto generated measurement blocks    ***********/\r\n\r\n";
const char auto_generated_calibration_start[] = "\r\n/***********   Start of auto generated calibration blocks    ***********/\r\n\r\n";
const char auto_generated_calibration_end[] = "\r\n/***********   End of auto generated calibration blocks    ***********/\r\n\r\n";

// 变量类型枚举
typedef enum
{
    TYPE_NOT_SUPPORTED, // 不支持的类型
    UBYTE,              // uint8_t,bool
    UWORD,              // uint16_t
    ULONG,              // uint32_t
    SBYTE,              // int8_t
    SWORD,              // int16_t
    SLONG,              // int32_t
    FLOAT32,            // float
    FLOAT64,            // double

} Element_Type_Enum;

// 元素信息结构体
typedef struct
{
    char name[ELEMENT_INFO_NAME_LENGTH] = {'\0'};              // 变量名
    size_t count = 0;                                          // 变量成员数（仅数组>1）
    size_t element_len = 0;                                    // 单元素长度(单位:字节)
    uint32_t addr = 0x00000000;                                // 元素起始地址
    Element_Type_Enum type = TYPE_NOT_SUPPORTED;               // 变量类型
    char type_str[ELEMENT_INFO_TYPE_STR_LENTH] = {'\0'};       // 类型字符串
    char max_limit_str[ELEMENT_INFO_LIMIT_STR_LENTH] = {'\0'}; // 上限字符串
    char min_limit_str[ELEMENT_INFO_LIMIT_STR_LENTH] = {'\0'}; // 下限字符串
} Element_Info;

// 文件链表
typedef struct File_List_Struct
{
    const char *file_name = nullptr;    // 文件名
    File_List_Struct *p_next = nullptr; // 下一表

} File_List;

// 获取文件的一行，成功时返回字符串指针，失败（文件结尾or超出长度）时回退文件指针
char *f_getline(FILE *file, char *s, size_t len)
{
    size_t count = 0;
    while (true)
    {
        char ch = fgetc(file);

        if (count >= len) // 超长
        {
            fseek(file, -count, SEEK_CUR);
            printf("[- WARN -] single line out of length\n");
            return nullptr;
        }

        if (ch == EOF && count == 0) // 仅文件结尾
            return nullptr;

        if (ch == '\n' || ch == EOF) // 成功换行or文件结尾
        {
            s[count] = '\n';
            return s;
        }

        s[count] = ch;
        count++;
    }
}

// 获取类型
Element_Type_Enum get_element_type(const char *str)
{
    if (!strcmp(str, "bool"))
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

// 解析元素类型
Element_Info solve_element(const char *str)
{
    Element_Info info;

    // 读取前段内容并获取元素类型
    char buff[ELEMENT_INFO_NAME_LENGTH] = {'\0'};
    sscanf(str, "%s", buff);
    info.type = get_element_type(buff);

    // 更新字符串信息
    switch (info.type)
    {
    case UBYTE:
        sprintf(info.type_str, "UBYTE");
        sprintf(info.min_limit_str, "0");
        sprintf(info.max_limit_str, "255");
        info.element_len = 1;
        break;
    case UWORD:
        sprintf(info.type_str, "UWORD");
        sprintf(info.min_limit_str, "0");
        sprintf(info.max_limit_str, "65535");
        info.element_len = 2;
        break;
    case ULONG:
        sprintf(info.type_str, "ULONG");
        sprintf(info.min_limit_str, "0");
        sprintf(info.max_limit_str, "4294967295");
        info.element_len = 4;
        break;
    case SBYTE:
        sprintf(info.type_str, "SBYTE");
        sprintf(info.min_limit_str, "-128");
        sprintf(info.max_limit_str, "127");
        info.element_len = 1;
    case SWORD:
        sprintf(info.type_str, "SWORD");
        sprintf(info.min_limit_str, "-32768");
        sprintf(info.max_limit_str, "32767");
        info.element_len = 2;
        break;
    case SLONG:
        sprintf(info.type_str, "SLONG");
        sprintf(info.min_limit_str, "-2147483648");
        sprintf(info.max_limit_str, "2147483647");
        info.element_len = 4;
        break;
    case FLOAT32:
        sprintf(info.type_str, "FLOAT32_IEEE");
        sprintf(info.min_limit_str, "-3.4E+38");
        sprintf(info.max_limit_str, "3.4E+38");
        info.element_len = 4;
        break;
    case FLOAT64:
        sprintf(info.type_str, "FLOAT64_IEEE");
        sprintf(info.min_limit_str, "-1.7E+308");
        sprintf(info.max_limit_str, "1.7E+308");
        info.element_len = 8;
        break;
    default:
        sprintf(info.type_str, "UNSUPPORTED");
        sprintf(info.min_limit_str, "0");
        sprintf(info.max_limit_str, "0");
        info.element_len = 0;
    }

    // 读取后段内容
    sscanf(str + strlen(buff), "%s", buff);

    // 获取名字和长度
    for (int count = 0; count < ELEMENT_INFO_NAME_LENGTH; count++)
    {
        if (buff[count] == '[') // 识别到数组
        {
            for (int n = count; n < ELEMENT_INFO_NAME_LENGTH; n++)
            {
                if (buff[n] == ']')
                    break;
                if (isdigit(buff[n]))
                    info.count = info.count * 10 + buff[n] - '0';
            }

            break;
        }

        if (buff[count] == ' ' || buff[count] == '=' || buff[count] == ';') // 是单个变量
            break;

        info.name[count] = buff[count]; // 复制变量名
    }

    return info;
}

// 获取元素地址
Element_Info get_element_addr(Element_Info info, FILE *map_file)
{
    // 回到文件头
    fseek(map_file, 0, SEEK_SET);
    // 段缓冲区
    char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

    bool find = false;

    // 读取并复制参考文件直到 a2l MODULE 结尾
    while (f_getline(map_file, segment_buff, sizeof(segment_buff)) != nullptr && !find)
    {

        // 匹配到变量名称
        if (strstr(segment_buff, info.name))
        {
            char *lpTarget = strstr(segment_buff, info.name);
            size_t len = strlen(info.name);

            // 匹配到的名称是后段中的子段内容---> xxxxx[name]xx
            if (lpTarget[len] != ' ' && lpTarget[len] != '\r' && lpTarget[len] != '\n')
                continue;
            // 匹配到的名称是前段中的字段内容---> xx[name]xxxxx
            if (*(lpTarget - 1) != '_' && *(lpTarget - 1) != ' ' && *(lpTarget - 1) != '@')
                continue;

            find = true;

            char buff[20];
            sscanf(segment_buff, "%*s%s", &buff);

            // 16进制转换
            for (int count = 0; count < 20; count++)
            {
                if (buff[count] == '+')
                    break;

                if (isalpha(buff[count]))
                    info.addr = info.addr * 16 + toupper(buff[count]) - 'A' + 10;
                else
                    info.addr = info.addr * 16 + buff[count] - '0';
            }
        }

        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }

    return info;
}

// 入口
int main(int m_argc, char *m_argv[])
{

    // int m_argc = 9;
    // char str[m_argc][20] = {
    //     {".\\new.exe"},
    //     {".\\input.c"},
    //     {".\\inputB.c"},
    //     {"-r"},
    //     {".\\PVER.a2l"},
    //     {"-l"},
    //     {".\\PVER.map"},
    //     // {"-o"},
    //     // {".\\[NEW]PVER.a2l"},
    // };
    // char *(m_argv[m_argc]);
    // for (int count = 0; count < m_argc; count++)
    //     m_argv[count] = str[count];

    // 文件列表初始
    File_List *list = (File_List *)malloc(sizeof(File_List));
    list->file_name = nullptr;
    list->p_next = nullptr;
    File_List *listHead = list;

    // 处理输入部分
    {
        const char *reference_name = nullptr; // 参考文件名
        const char *output_name = nullptr;    // 输出文件名

        // 循环处理指令
        for (int count = 1; count < m_argc; count++)
        {
            // 匹配到参考a2l文件
            if (!strcmp(m_argv[count], "-r"))
            {
                if (count + 1 >= m_argc)
                    break;
                Input_Reference = fopen(m_argv[count + 1], "rb");
                if (Input_Reference == nullptr)
                {
                    printf("[- FAIL -] Input reference file <%s> open failed.\n", m_argv[count + 1]);
                    break;
                }

                // 获取不带路径的文件名
                reference_name = m_argv[count + 1] + strlen(m_argv[count + 1]);
                while (reference_name + 1 != m_argv[count + 1] && *reference_name != '\\' && *reference_name != '/')
                    reference_name--;
                reference_name++;

                printf("[-  OK  -] %-31s %s\n", "Input reference .a2l file:", m_argv[count + 1]);
                count++;
                continue;
            }

            // 匹配到链接map文件
            else if (!strcmp(m_argv[count], "-l"))
            {
                if (count + 1 >= m_argc)
                    break;
                Input_Map = fopen(m_argv[count + 1], "rb");
                if (Input_Map == nullptr)
                {
                    printf("[- FAIL -] Input link file <%s> open failed.\n", m_argv[count + 1]);
                    break;
                }

                printf("[-  OK  -] %-31s %s\n", "Input link .map file:", m_argv[count + 1]);
                count++;
                continue;
            }

            // 匹配到自定义输出文件名
            else if (!strcmp(m_argv[count], "-o"))
            {
                if (count + 1 >= m_argc)
                    break;
                output_name = m_argv[count + 1];
                printf("[-  OK  -] %-31s %s\n", "Set output filename:", m_argv[count + 1]);
                count++;
                continue;
            }

            // 其它输入作为源文件输入
            printf("[-  OK  -] %-31s %s\n", "Input source filename:", m_argv[count]);
            list->file_name = m_argv[count];
            list->p_next = (File_List *)malloc(sizeof(File_List));
            list = list->p_next;
            list->file_name = nullptr;
            list->p_next = nullptr;
        }

        // 检查文件列表状态
        if (listHead->file_name == nullptr)
        {
            printf("[- FAIL -] No source file input.\n");
            free(listHead);
            fcloseall();
            return 0;
        }

        // 检查参考文件输入文件状态
        if (Input_Reference == nullptr)
        {
            printf("[- FAIL -] No reference .a2l file input.\n");
            fcloseall();
            return 0;
        }

        // 检查map链接文件文件状态
        if (Input_Map == nullptr)
            printf("[- WARN -] No link .map file input.\n");

        // 处理输出文件名称
        if (output_name != nullptr)
            Output_Target = fopen(output_name, "wb+");
        else if (reference_name != nullptr)
        {
            // 拼接默认的输出文件名
            const char *prefix = "[NEW]";
            char *buffer = (char *)malloc(strlen(reference_name) + strlen(prefix) + 1);
            sprintf(buffer, "%s%s", prefix, reference_name);

            printf("[> NOTE <] %-31s %s\n", "Default output filename:", buffer);

            Output_Target = fopen(buffer, "wb+");

            // 释放临时指针
            free(buffer);
        }
    }

    // 打开输入源文件(二进制形式，否则RF/CRLF引发计数问题)
    Input_Source = fopen("./input.c", "rb");

    // 段缓冲区
    char segment_buff[SEGMENT_BUFF_LENGTH] = {'\0'};

    // 读取并复制参考文件直到 a2l MODULE 结尾
    while (f_getline(Input_Reference, segment_buff, sizeof(segment_buff)) != nullptr)
    {
        // 当前行为 a2l MODULE 结尾
        if (strstr(segment_buff, a2l_module_end))
        {
            // 回退文件指针到上一行结尾
            fseek(Input_Reference, -2, SEEK_CUR);
            while (fgetc(Input_Reference) != '\n')
                fseek(Input_Reference, -2, SEEK_CUR);
            break;
        }

        // 输出行到文件
        // fprintf(Output_Target, segment_buff);    // 太大300会段溢出
        for (int count = 0; count < SEGMENT_BUFF_LENGTH; count++) // 逐个输出
        {
            fputc(segment_buff[count], Output_Target);
            if (segment_buff[count] == '\n')
                break;
        }

        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }

    list = listHead;
    // 循环处理输入文件链表
    while (list->p_next != nullptr)
    {
        Input_Source = fopen(list->file_name, "rb");
        if (Input_Source == nullptr)
        {
            printf("[< FAIL >] Soure file \"%s\" load failed.\n", list->file_name);

            fclose(Input_Source);
            list = (File_List *)list->p_next;
            continue;
        }

        printf("\n\n[-  OK  -] Soure file \"%s\" load succeed.\n", list->file_name);

        // 循环获取输入文件行
        while (f_getline(Input_Source, segment_buff, sizeof(segment_buff)) != nullptr)
        {
            // 当前行为观测量起始位行
            if (strstr(segment_buff, measurement_begin))
            {
                printf("\n%s\n\n", measurement_begin);
                fprintf(Output_Target, auto_generated_measurement_start);

                // 清空段缓冲区
                memset(segment_buff, '\0', sizeof(segment_buff));

                while (true)
                {
                    // 获取下一行
                    f_getline(Input_Source, segment_buff, sizeof(segment_buff));

                    // 观测量结束行
                    if (strstr(segment_buff, measurement_end))
                    {
                        printf("\n%s\n\n", measurement_end);
                        fprintf(Output_Target, auto_generated_measurement_end);
                        break;
                    }
                    // 解析元素变量信息
                    {
                        Element_Info info = solve_element(segment_buff);

                        // 成功解析
                        if (info.type != TYPE_NOT_SUPPORTED)
                        {
                            // 获取地址
                            info = get_element_addr(info, Input_Map);

                            // 调试信息
                            if (info.addr != 0)
                            {
                                if (info.count == 0)
                                    printf("[-  OK  -] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                                else
                                    printf("[-  OK  -] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                            }
                            else
                            {
                                if (info.count == 0)
                                    printf("[< WARN >] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                                else
                                    printf("[< WARN >] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                            }

                            // 单变量
                            if (info.count == 0)
                            {
                                fprintf(Output_Target, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                fprintf(Output_Target, "    %s\r\n", info.name);                           // 名称
                                fprintf(Output_Target, "    \"auto generated\"\r\n");                      // 描述
                                fprintf(Output_Target, "    %s\r\n", info.type_str);                       // 数据类型
                                fprintf(Output_Target, "    %s\r\n", "NO_COMPU_METHOD");                   // 转换式(保留原始值)
                                fprintf(Output_Target, "    %s\r\n", "0");                                 // 分辨率
                                fprintf(Output_Target, "    %s\r\n", "0");                                 // 精度误差
                                fprintf(Output_Target, "    %s\r\n", info.min_limit_str);                  // 下限
                                fprintf(Output_Target, "    %s\r\n", info.max_limit_str);                  // 上限
                                fprintf(Output_Target, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.addr); // ECU 地址

                                fprintf(Output_Target, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                            }
                            else
                            {
                                for (int count = 0; count < info.count; count++)
                                {
                                    fprintf(Output_Target, "%s\r\n\r\n", "/begin MEASUREMENT"); // 观测量头

                                    fprintf(Output_Target, "    %s[%d]\r\n", info.name, count);                                           // 名称
                                    fprintf(Output_Target, "    \"auto generated\"\r\n");                                                 // 描述
                                    fprintf(Output_Target, "    %s\r\n", info.type_str);                                                  // 数据类型
                                    fprintf(Output_Target, "    %s\r\n", "NO_COMPU_METHOD");                                              // 转换式(保留原始值)
                                    fprintf(Output_Target, "    %s\r\n", "0");                                                            // 分辨率
                                    fprintf(Output_Target, "    %s\r\n", "0");                                                            // 精度误差
                                    fprintf(Output_Target, "    %s\r\n", info.min_limit_str);                                             // 下限
                                    fprintf(Output_Target, "    %s\r\n", info.max_limit_str);                                             // 上限
                                    fprintf(Output_Target, "    %s 0x%08x\r\n\r\n", "ECU_ADDRESS", info.addr + count * info.element_len); // ECU 地址

                                    fprintf(Output_Target, "%s\r\n\r\n", "/end MEASUREMENT"); // 观测量尾
                                }
                            }
                        }
                        else if (info.name[0] != '\0') // 解析失败且变量名不为空
                        {
                            if (info.count == 0)
                                printf("[< FAIL >] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                            else
                                printf("[< FAIL >] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                        }
                    }

                    // 清空段缓冲区
                    memset(segment_buff, '\0', sizeof(segment_buff));
                }
            }

            // 当前行为标定量起始位行
            if (strstr(segment_buff, calibration_begin))
            {
                printf("\n%s\n\n", calibration_begin);
                fprintf(Output_Target, auto_generated_calibration_start);

                // 清空段缓冲区
                memset(segment_buff, '\0', sizeof(segment_buff));

                while (true)
                {
                    // 获取下一行
                    f_getline(Input_Source, segment_buff, sizeof(segment_buff));

                    // 标定量结束行
                    if (strstr(segment_buff, calibration_end))
                    {
                        printf("\n%s\n\n", calibration_end);
                        fprintf(Output_Target, auto_generated_calibration_end);
                        break;
                    }
                    // 解析元素变量信息
                    {
                        Element_Info info = solve_element(segment_buff);

                        // 成功解析
                        if (info.type != TYPE_NOT_SUPPORTED)
                        {
                            // 获取地址
                            info = get_element_addr(info, Input_Map);

                            // 调试信息
                            if (info.addr != 0)
                            {
                                if (info.count == 0)
                                    printf("[-  OK  -] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                                else
                                    printf("[-  OK  -] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                            }
                            else
                            {
                                if (info.count == 0)
                                    printf("[< WARN >] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                                else
                                    printf("[< WARN >] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                            }

                            // 单变量
                            if (info.count == 0)
                            {
                                fprintf(Output_Target, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                fprintf(Output_Target, "    %s\r\n", info.name);              // 名称
                                fprintf(Output_Target, "    \"auto generated\"\r\n");         // 描述
                                fprintf(Output_Target, "    %s\r\n", "VALUE");                // 值类型(数值、数组、曲线)
                                fprintf(Output_Target, "    0x%08x\r\n", info.addr);          // ECU 地址
                                fprintf(Output_Target, "    Scalar_%s\r\n", info.type_str);   // 数据类型
                                fprintf(Output_Target, "    %s\r\n", "0");                    // 允许最大差分
                                fprintf(Output_Target, "    %s\r\n", "NO_COMPU_METHOD");      // 转换式(保留原始值)
                                fprintf(Output_Target, "    %s\r\n", info.min_limit_str);     // 下限
                                fprintf(Output_Target, "    %s\r\n\r\n", info.max_limit_str); // 上限

                                fprintf(Output_Target, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                            }
                            else
                            {
                                for (int count = 0; count < info.count; count++)
                                {
                                    fprintf(Output_Target, "%s\r\n\r\n", "/begin CHARACTERISTIC"); // 标定量头

                                    fprintf(Output_Target, "    %s[%d]\r\n", info.name, count);                     // 名称
                                    fprintf(Output_Target, "    \"auto generated\"\r\n");                           // 描述
                                    fprintf(Output_Target, "    %s\r\n", "VALUE");                                  // 值类型(数值、数组、曲线)
                                    fprintf(Output_Target, "    0x%08x\r\n", info.addr + count * info.element_len); // ECU 地址
                                    fprintf(Output_Target, "    Scalar_%s\r\n", info.type_str);                     // 数据类型
                                    fprintf(Output_Target, "    %s\r\n", "0");                                      // 允许最大差分
                                    fprintf(Output_Target, "    %s\r\n", "NO_COMPU_METHOD");                        // 转换式(保留原始值)
                                    fprintf(Output_Target, "    %s\r\n", info.min_limit_str);                       // 下限
                                    fprintf(Output_Target, "    %s\r\n\r\n", info.max_limit_str);                   // 上限

                                    fprintf(Output_Target, "%s\r\n\r\n", "/end CHARACTERISTIC"); // 标定量尾
                                }
                            }
                        }
                        else if (info.name[0] != '\0') // 解析失败且变量名不为空
                        {

                            if (info.count == 0)
                                printf("[< FAIL >] 0x%08x %-20s %s;\n", info.addr, info.type_str, info.name);
                            else
                                printf("[< FAIL >] 0x%08x %-20s %s[%d];\n", info.addr, info.type_str, info.name, info.count);
                        }
                    }

                    // 清空段缓冲区
                    memset(segment_buff, '\0', sizeof(segment_buff));
                }
            }
        }

        fclose(Input_Source);
        list = (File_List *)list->p_next;
    }

    // 清空段缓冲区
    memset(segment_buff, '\0', sizeof(segment_buff));

    // 输出参考文件的剩余部分
    // 读取并复制参考文件直到 a2l MODULE 结尾
    while (f_getline(Input_Reference, segment_buff, sizeof(segment_buff)) != nullptr)
    {
        // 输出行到文件
        fprintf(Output_Target, segment_buff);
        // 清空段缓冲区
        memset(segment_buff, '\0', sizeof(segment_buff));
    }

    // 关闭文件
    fcloseall();

    // 释放文件指针
    list = listHead;
    while (list != nullptr)
    {
        File_List *next = (File_List *)list->p_next;
        free(list);
        list = next;
    }

    return 0;
}