# Source and include directories
SRCS_DIR = srcs
INCS_DIR = incs
OBJ_DIR = obj

# Source files and corresponding object files
SRCS = $(SRCS_DIR)/main.cpp $(SRCS_DIR)/ConfigParser.cpp $(SRCS_DIR)/Config.cpp $(SRCS_DIR)/Utils.cpp
OBJS = $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJ_DIR)/$(SRCS_DIR)/%.o)

# Project name
NAME = webserv

# Compiler and flags
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -g
INCLUDES = -I$(INCS_DIR)

# Build targets
all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

# Object file compilation rule
$(OBJ_DIR)/$(SRCS_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJ_DIR)/$(SRCS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Ensure object directory structure
$(OBJ_DIR)/$(SRCS_DIR):
	@mkdir -p $(OBJ_DIR)/$(SRCS_DIR)

# Clean object files
clean:
	rm -f $(OBJS)

# Clean all, including the binary
fclean: clean
	rm -f $(NAME)

# Rebuild everything
re: fclean all

# Debug target for GDB
debug: CFLAGS += -DDEBUG
debug: re

# ASan target with address sanitizer enabled
asan: CFLAGS += -fsanitize=address
asan: re

.PHONY: all clean fclean re debug asan
