SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o,$(SRCS))
FLAGS = -lcrypto -lpthread -lavformat -lavcodec -lavutil -lswresample -lcurl -lfaad -lrtmp
INCLUDE = -I ../json

talkShar : $(OBJS)
	@g++ -g $^ -o $@ $(INCLUDE) $(FLAGS)
	@echo 编译完成...

$(OBJS) : %.o : %.cpp 
	@g++ -g -std=c++11 -c $^ -o $@

clean:
	@rm -rf *.o talkShar

