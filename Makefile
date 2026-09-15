CXX      = clang++
NAME     = Matt_daemon
OBJ_DIR  = obj
SRC_DIR  = src
INCLUDES = includes

SRCS = $(addprefix $(SRC_DIR)/, main.cpp MattDaemon.cpp Tintin_reporter.cpp Client.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

CXXFLAGS = -Wall -Wextra -Werror -I$(INCLUDES) -g3 -MMD -MP
LDFLAGS  =

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(OBJS:.o=.d)

.PHONY: all clean fclean re
