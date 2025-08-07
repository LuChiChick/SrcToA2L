# 目标文件名称
TARGET_FILE_NAME = SrcToA2L

# 编译路径
BUILD_DIR = Build

# 工具链前缀
PREFIX = \

# 调试选项
DEBUG = 1

# GUI或CUI编译选项
GUI = 0

# C编译标准，等号后面不能有空格
C_STD =c17

# C++编译标准，等号后面不能有空格
CXX_STD =c++17

#输入源文件字符编码定义，等号后不能有空格
INPUT_CHAR_CODING =UTF-8

#输出单字节字符编码定义，等号后不能有空格
OUTPUT_CHAR_CODING =GBK

#输出宽字符编码定义，等号后不能有空格
OUTPUT_WCHAR_CODING =UTF-16LE

# 优化等级
OPT = -Og

# C编译工具
CC = $(PREFIX)gcc

# C++编译工具
CXX = $(PREFIX)g++

# Win32 资源文件编译工具
WIN32_RES = windres

# 链接库
LIB_LINK = 	\

# C定义
C_DEFS =  \
-D_UNICODE	\
-DUNICODE

# C头文件目录
C_INCLUDES =  \

# C源文件
C_SOURCES =  \

# C++定义
CXX_DEFS =  \

# C++ 头文件目录
CXX_INCLUDES =  \
-IInc	\

# C++源文件
CXX_SOURCES = \
Src/Main.cpp 				\
Src/Tool_Functions.cpp 		\
Src/Core_Functions.cpp		\
Src/Global_Variables.cpp	\

# 资源文件
WIN32_RES_LISTS = \

# C编译选项
CFLAGS = $(C_DEFS) $(C_INCLUDES) $(OPT) -std=$(C_STD) \
		-finput-charset=$(INPUT_CHAR_CODING) \
		-fexec-charset=$(OUTPUT_CHAR_CODING) \
		-fwide-exec-charset=$(OUTPUT_WCHAR_CODING)  \
		-static \
		-Wall \

# 生成依赖相关信息
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# 调试选项
ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

# GUI与CUI选项
ifeq ($(GUI), 1)
CFLAGS += -mwindows
else
CFLAGS += -mconsole
endif

# C++编译选项
CXXFLAGS = -lstdc++ $(CFLAGS:$(C_STD)=$(CXX_STD)) $(CXX_DEFS) $(CXX_INCLUDES)

# 链接器选项
LDFLAGS = -Wl,-Map,$(BUILD_DIR)/$(TARGET_FILE_NAME).map

# C目标文件目录关联
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# C++目标文件目录关联
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CXX_SOURCES)))

# 资源文件目录关联
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(WIN32_RES_LISTS:.rc=.o)))
vpath %.rc $(sort $(dir $(WIN32_RES_LISTS)))

# Make编译任务
all: $(BUILD_DIR)/$(TARGET_FILE_NAME).exe

# C目标文件编译
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo ====== C Source File "$<" Compiling... ======
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# C++ 目标文件编译编译
$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR) 
	@echo ====== C++ Source File "$<" Compiling... ====== 
	$(CXX) -c $(CXXFLAGS) -Wa,-a,-ad,-ahlms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@

# 资源文件编译编译
$(BUILD_DIR)/%.o: %.rc Makefile | $(BUILD_DIR) 
	@echo ====== Win32 Resources File "$<" Compiling... ====== 
	$(WIN32_RES) $< -o $@

# 生成可执行文件
$(BUILD_DIR)/$(TARGET_FILE_NAME).exe: $(OBJECTS) Makefile
	@echo ====== All File Compiled. Now Linking... ====== 
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $@ $(LIB_LINK)
	@echo ====== Program Link Finished ====== 

# 生成编译目录	
$(BUILD_DIR):
	mkdir $@

# 清除任务
clean:$(BUILD_DIR) 
	powershell rm -r $(BUILD_DIR)

# 运行任务
run:all
	$(BUILD_DIR)/$(TARGET_FILE_NAME).exe

# 发行编译
release:$(BUILD_DIR) $(RELEASE_DIR)
	powershell rm -r $(BUILD_DIR)
	make DEBUG=0
	powershell rm $(BUILD_DIR)/*.o
	powershell rm $(BUILD_DIR)/*.d
	powershell rm $(BUILD_DIR)/*.lst
	powershell rm $(BUILD_DIR)/*.map

# 依赖关系
-include $(wildcard $(BUILD_DIR)/*.d)