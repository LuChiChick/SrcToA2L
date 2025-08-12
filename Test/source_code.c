// .\Test\source_code.c
#include "stdint.h"
#include "typedef.h"

#define VALUE_S 2

/*start_of_calibrations*/
uint8_t c_u8;
uint16_t c_u16;
const volatile float c_float;
/*end_of_calibrations*/

/*start_of_measurements*/
double m_double = 1.23;
uint16_t m_u16[2];

Test_Struct S[VALUE_S];
/*end_of_measurements*/