NAME = parser
COMPILER = g++
FLAGS = -Wall -Wextra
MODULES = ConfigParser.cpp \
		  main.cpp
SRCS = $(MODULES:%.cpp=srcs/%.cpp)
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

.PHONY: all clean fclean re

