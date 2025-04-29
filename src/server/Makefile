# 定义编译器和工具
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Os -pipe
LDFLAGS  := -L. -pipe
LDLIBS   := -lpthread -lstdc++fs -ljsoncpp -lbundle -levent
TARGETS  := test gdb_test

# 调试版本选项（带调试符号）
DEBUG_CXXFLAGS := -g
DEBUG_LDFLAGS  := -g

# 自动生成依赖文件
DEPFLAGS = -MMD -MP
SOURCES  = Test.cpp
OBJECTS  = $(SOURCES:.cpp=.o)
DEPS     = $(SOURCES:.cpp=.d)

# 默认构建所有目标
all: $(TARGETS)

# 通用编译规则：生成 .o 文件（自动处理依赖）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# 生成 release 版本（test）
test: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# 生成 debug 版本（gdb_test）
gdb_test: CXXFLAGS += $(DEBUG_CXXFLAGS)
gdb_test: LDFLAGS += $(DEBUG_LDFLAGS)
gdb_test: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# 包含自动生成的依赖文件
-include $(DEPS)

# 清理生成的文件
clean:
	$(RM) $(TARGETS) $(OBJECTS) $(DEPS) ./deep_storage ./low_storage ./logfile storage.data

.PHONY: all clean