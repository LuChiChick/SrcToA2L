#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#define VARIABLE_NAME_LENGTH_MAX 100
#define ELEMENT_NAME_LENGTH_MAX 100
#define TYPE_NAME_LENGTH_MAX 100

#define A2L_TYPE_STR_LENGTH_MAX 100
#define A2L_LIMIT_STR_LENGTH_MAX 100

#define SEGMENT_BUFF_LENGTH 5000

// 输出文件默认前缀
#define OUTPUT_A2L_DEFAULT_PREFIX "[NEW]"
// 中间件尾缀
#define OUTPUT_MIDDLEWARE_SUFFIX ".middleware.txt"

// 相关识别串
const char measurement_begin[] = "/*Start_Measurements*/";
const char measurement_end[] = "/*End_Measurements*/";
const char calibration_begin[] = "/*Start_Calibration*/";
const char calibration_end[] = "/*End_Calibration*/";
const char typedef_begin[] = "/*Start_TypeDef*/";
const char typedef_end[] = "/*End_TypeDef*/";

const char a2l_module_end[] = "/end MODULE";

// 段输出标识串
const char auto_generated_measurement_start[] = "\r\n/***********   Start of auto generated measurement blocks    ***********/\r\n\r\n";
const char auto_generated_measurement_end[] = "\r\n/***********   End of auto generated measurement blocks    ***********/\r\n\r\n";
const char auto_generated_calibration_start[] = "\r\n/***********   Start of auto generated calibration blocks    ***********/\r\n\r\n";
const char auto_generated_calibration_end[] = "\r\n/***********   End of auto generated calibration blocks    ***********/\r\n\r\n";

#endif