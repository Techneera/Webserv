NAME = webserv
COMPILER = c++
FLAGS = -Wall -Wextra -Werror
MAIN = main.cpp
SCRIPTS = $(MAIN)
SRCS = $(SCRIPTS:%.cpp=src/%.cpp)
OBJS = $(SRCS:.cpp=.o)
INCLUDES = include/

all: $(NAME)

%.o: %.cpp
	$(COMPILER) $(FLAGS) -I $(INCLUDES) -c $< -o $@

.SECONDEXPANSION:
$(NAME): $$(OBJS)
	$(COMPILER) $(FLAGS) $(OBJS) -o $@

clean:
	rm -rf $(OBJS)
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
	@echo "compiling module $(@)"

.PHONY: all clean fclean re
