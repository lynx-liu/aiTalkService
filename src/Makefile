# 匹配所有 .cpp、.c 和 .cc 文件
SRCS = $(wildcard *.cpp) $(wildcard *.c) $(wildcard *.cc)

# 将源文件统一替换为 .o 文件
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.cc=.o)

# 链接和运行时标志
FLAGS = -lpthread -lcurl -lfaad -lz -ldl -lcrypto -lrtmp
# 直接使用 libfvad 生成的静态库路径，避免依赖 -L/-l 查找
LIBFVAD = ../libfvad/build/libfvad.a

# 头文件包含路径
INCLUDE = -I ../json -I ../libfvad -I ../libfvad/vad -I ../libfvad/signal_processing

# 最终目标
talkShar : $(OBJS) | libfvad
	@g++ -g $^ $(LIBFVAD) -o $@ $(INCLUDE) $(FLAGS)
	@echo 编译完成...

# 使用cmake编译libfvad
libfvad:
	@echo "Checking libfvad..."
	@if [ ! -d ../libfvad ]; then \
		echo "libfvad directory not found at ../libfvad"; exit 1; \
	fi
	+@mkdir -p ../libfvad/build
	+@cd ../libfvad/build && cmake .. >/dev/null 2>&1 || true
	+@$(MAKE) -C ../libfvad/build || true
	@if [ -f ../libfvad/build/libfvad.a ]; then \
		echo "libfvad built"; \
	else \
		echo "libfvad build failed"; \
	fi

# .cpp 文件编译规则
%.o : %.cpp
	@g++ -g -std=c++11 -c $< -o $@

# .cc 文件编译规则
%.o : %.cc
	@g++ -g -std=c++11 -c $< -o $@

# .c 文件编译规则
%.o : %.c
	@gcc -g -std=c99 -c $< -o $@

# 清理中间文件
clean:
	@rm -rf *.o talkShar
	@if [ -d ../libfvad/build ]; then \
		+$(MAKE) -C ../libfvad/build clean >/dev/null 2>&1 || true; \
		rm -rf ../libfvad/build; \
	fi
