NAME=webserv

CXX = c++
CXXFLAGS = -g -std=c++98 -Wextra -Wall -Werror -Iincludes
B_SRC = src/RequestHandler.cpp src/ResponseHandler.cpp src/Mime.cpp src/HttpResponse.cpp
SRC = main.cpp $(B_SRC)
OBJS = $(SRC:.cpp=.o)

TEST_NAME = webserv_test
TEST_SRC = tests/test_router.cpp $(B_SRC)
TEST_OBJS = $(TEST_SRC:.cpp=.o)

DEMO_NAME = webserv_deneme
DEMO_SRC = tests/deneme_b2.cpp $(B_SRC)
DEMO_OBJS = $(DEMO_SRC:.cpp=.o)

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

deneme: $(DEMO_NAME)
	./$(DEMO_NAME)

$(DEMO_NAME): $(DEMO_OBJS)
	$(CXX) -o $(DEMO_NAME) $(CXXFLAGS) $(DEMO_OBJS)

leak:
	valgrind ./$(NAME)

clean:
	rm -rf $(OBJS) $(TEST_OBJS) $(DEMO_OBJS)

fclean: clean
	rm -rf $(NAME) $(TEST_NAME) $(DEMO_NAME)

re: fclean all

.PHONY: all run leak test deneme clean fclean re
