# **************************************************************************** #
#                                   SETTINGS                                   #
# **************************************************************************** #

NAME        = ircserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++17 -g

INC_DIR     = includes
SRC_DIR     = src

SRC_FILES   = Server.cpp \
              Client.cpp \
              Channel.cpp \
              Parser.cpp \
              CommandHandler.cpp \
              main.cpp

SRCS        = $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJS        = $(SRCS:.cpp=.o)

# **************************************************************************** #
#                                   RULES                                      #
# **************************************************************************** #

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) $(OBJS) -o $(NAME)
	@echo "✔ Build complete: $(NAME)"

# Compile .cpp to .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJS)
	@echo "✔ Object files removed"

fclean: clean
	rm -f $(NAME)
	@echo "✔ Executable removed"

re: fclean all

.PHONY: all clean fclean re
