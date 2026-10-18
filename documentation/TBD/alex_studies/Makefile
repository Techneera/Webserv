NAME = miniserver
COMPILER = cc
FLAGS = -Wall -Wextra -Werror -g
SRCS = main.c
OBJS = $(SRCS:.c=.o)
INCLUDES = includes/

all: $(NAME)

%.o: %.c
	$(COMPILER) $(FLAGS) -I $(INCLUDES) -c $< -o $@

$(NAME): $(OBJS)
	$(COMPILER) $(FLAGS) -I $(INCLUDES) $(OBJS) -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

