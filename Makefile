# CONFIG
NAME      := ircserv
CXX       := c++
CXXFLAGS  := -Wall -Wextra -Werror -std=c++17 -Iincludes
DEBUG_FLAGS := -g -O0

SRC_DIR   := src
OBJ_DIR   := obj

# Source files
SRCS := main.cpp \
				./server/Server.cpp ./server/ChannelHelpers.cpp ./server/ClientHandling.cpp \
				Channel.cpp CommandHandler.cpp Parser.cpp Client.cpp CommandHandlerHelpers.cpp \
				CommandHandlerChannel.cpp CommandHandlerMode.cpp

SRC_PATHS := $(addprefix $(SRC_DIR)/, $(SRCS))
OBJ_PATHS := $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEP_FILES := $(OBJ_PATHS:.o=.d)

all: $(NAME)

$(NAME): $(OBJ_PATHS)
	@echo "Linking $(NAME)..."
	@$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Removing object files..."
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "Removing executable..."
	@rm -f $(NAME)

debug: clean
	@echo "Building debug version..."
	@$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SRC_PATHS) -o $(NAME)_debug

re: fclean all

-include $(DEP_FILES)

.PHONY: all clean fclean re
