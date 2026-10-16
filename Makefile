NAME = webserv
COMPILER = c++
FLAGS = -Wall -Wextra -Werror
SCRIPTS = main.cpp 
SRCS = $(SCRIPTS:%.cpp=srcs/%.cpp)
OBJS = $(SRCS:.cpp=.o)
INCLUDES = includes/

all: $(NAME)

%.o: %.cpp
	$(COMPILER) $(FLAGS) -I $(INCLUDES) -c $< -o $@

$(NAME): $(OBJS)
	$(COMPILER) $(FLAGS) $(OBJS) -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

debug: fclean
debug: FLAGS= -g -Wall -Wextra
debug: all

.PHONY: all clean fclean re
