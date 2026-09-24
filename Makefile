NAME=webserv

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iincludes
B_SRC = src/RequestHandler.cpp src/ResponseHandler.cpp
A_SRC = parser/LocationParser.cpp parser/ServerParser.cpp src/ServerManager.cpp
SRC = main.cpp $(B_SRC) $(A_SRC)
OBJS = $(SRC:.cpp=.o)

TEST_NAME = webserv_test
TEST_SRC = tests/test_router.cpp $(B_SRC)
TEST_OBJS = $(TEST_SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	c++  -o $(NAME) $(CXXFLAGS) $(OBJS) 

run: $(NAME)
	clear
	./$(NAME)

test: $(TEST_NAME)
	./$(TEST_NAME)

$(TEST_NAME): $(TEST_OBJS)
	$(CXX) -o $(TEST_NAME) $(CXXFLAGS) $(TEST_OBJS)

leak:
	valgrind ./$(NAME) 

clean:
	rm -rf $(OBJS) $(TEST_OBJS)
	
fclean: clean
	rm -rf $(NAME) $(TEST_NAME)

re: fclean all

.PHONY: all run leak test clean fclean re
