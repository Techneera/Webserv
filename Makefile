NAME = webserv

COMPILER = c++

FLAGS = -Wall -Wextra -Werror -std=c++98 $(INCLUDE_FLAGS)

MAIN = main.cpp

CONFIGURATION_FILE = configuration_file/configuration_file.cpp

SIGNAL = signal/signal.cpp

EVENT_LOOP = event_loop/event_loop.cpp

INCLUDES = event_loop \
		   configuration_file \
		   signal

INCLUDE = $(INCLUDES:%=src/%)

INCLUDE_FLAGS = $(addprefix -I ,$(INCLUDE))

SCRIPTS = $(MAIN) \
		  $(CONFIGURATION_FILE) \
		  $(EVENT_LOOP) \
		  $(SIGNAL)

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
	rm -rf build

fclean: clean
	rm -rf $(NAME)

re: fclean all

debug: fclean
debug: FLAGS= -g -Wall -Wextra $(INCLUDE_FLAGS)
debug: all

# In order to be able to test several different:
# 1 - create module main in src dir
# 2 - module main name format: main_<module_name>.cpp
# 3 - run make test_<module_name>
_%: fclean
	@mkdir build
	@$(MAKE) FLAGS="-g -Wall -Wextra $(INCLUDE_FLAGS)" \
		MAIN=main_$(@:_%=%).cpp NAME=$(@:_%=build/%)
	rm -rf src/*.o

.PHONY: all clean fclean re
