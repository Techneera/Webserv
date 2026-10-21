NAME = miniserver
COMPILER = c++
FLAGS = -std=c++98 -Wall -Wextra -Werror
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
INCLUDES = includes/

all: $(NAME)

%.o: %.cpp
	$(COMPILER) $(FLAGS) -I $(INCLUDES) -c $< -o $@

$(NAME): $(OBJS)
	$(COMPILER) $(FLAGS) -I $(INCLUDES) $(OBJS) -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

debug: FLAGS = -std=c++98 -Wall -Wextra -g
debug: fclean
debug: all

re: fclean all
