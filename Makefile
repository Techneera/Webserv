NAME = webserv
COMPILER = c++
FLAGS = -Wall -Wextra -Werror -std=c++98 -I $(INCLUDE)
MAIN = main.cpp
EVENT_LOOP = event_loop/event_loop.cpp
INCLUDES = event_loop
INCLUDE = $(INCLUDES:%=src/%)
SCRIPTS = $(MAIN) \
		  $(EVENT_LOOP)
SRCS = $(SCRIPTS:%.cpp=src/%.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

%.o: %.cpp
	$(COMPILER) $(FLAGS) -c $< -o $@

.SECONDEXPANSION:
$(NAME): $$(OBJS)
	$(COMPILER) $(FLAGS) $(OBJS) -o $@

clean:
	rm -rf $(OBJS) src/*.o
	rm -rf bin

fclean: clean
	rm -rf $(NAME)

re: fclean all

debug: fclean
debug: FLAGS= -g -Wall -Wextra
debug: all

# In order to be able to test several different:
# 1 - create module main
# 2 - module main name format: _<module_name>.cpp
# 3 - modules run make test_<module_name>
_%: fclean
	@mkdir bin
	@$(MAKE) FLAGS="-g -Wall -Wextra" MAIN=main_$(@:_%=%).cpp NAME=$(@:_%=bin/%)
	rm -rf src/*.o

.PHONY: all clean fclean re
