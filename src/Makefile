# 匹配所有 .cpp、.c 和 .cc 文件
SRCS = $(wildcard *.cpp) $(wildcard *.c) $(wildcard *.cc)

# 将源文件统一替换为 .o 文件
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.cc=.o)

# 编译参数和头文件路径
FLAGS = -lpthread -lcurl -lfaad -lz -ldl -lcrypto -lrtmp
INCLUDE = -I ../json

# 最终目标
talkShar : $(OBJS)
	@g++ -g $^ -o $@ $(INCLUDE) $(FLAGS)
	@echo 编译完成...

# .cpp 文件编译规则
%.o : %.cpp
	@g++ -g -std=c++17 -c $< -o $@

# .cc 文件编译规则
%.o : %.cc
	@g++ -g -std=c++17 -c $< -o $@

# .c 文件编译规则
%.o : %.c
	@gcc -g -std=c99 -c $< -o $@

# 清理中间文件
clean:
	@rm -rf *.o talkShar
