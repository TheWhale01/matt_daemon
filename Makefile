CXX= clang++
NAME= Matt_daemon
OBJ_DIR= obj/
SRC_DIR= src/
INCLUDES= includes/
SRCS= $(addprefix $(SRC_DIR), main.cpp MattDaemon.cpp Tintin_reporter.cpp)
OBJS= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(SRCS))
CXXFLAGS= -Wall -Wextra -Werror -I $(INCLUDES) -g3

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
