# Source and include directories
SRCS_DIR = srcs
INCS_DIR = incs
OBJ_DIR = obj

# Source files and corresponding object files
SRCS = $(SRCS_DIR)/main.cpp $(SRCS_DIR)/ConfigParser.cpp $(SRCS_DIR)/Config.cpp $(SRCS_DIR)/Utils.cpp $(SRCS_DIR)/Location.cpp $(SRCS_DIR)/Server.cpp $(SRCS_DIR)/RequestParser.cpp $(SRCS_DIR)/ErrorResponse.cpp $(SRCS_DIR)/CGIRunner.cpp $(SRCS_DIR)/CacheManager.cpp
OBJS = $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJ_DIR)/$(SRCS_DIR)/%.o)

# Project name
NAME = webserv

# Compiler and flags
CC = c++
CFLAGS = #-Wall -Wextra -Werror -std=c++98 -g
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

siege: all
	siege -b -t3S http://localhost:8083

run: all
	./webserv confs/default.conf

.PHONY: all clean fclean re debug asan
