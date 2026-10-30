NAME=webserv

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -g -Iincludes -Iincludes/core -Iincludes/event -Iincludes/http -Iincludes/http/modules

CORE_SRC = src/core/LocationParser.cpp src/core/ServerParser.cpp src/core/Mime.cpp src/core/Utils.cpp
EVENT_SRC = src/event/ServerManager.cpp src/event/Client.cpp
HTTP_SRC = src/http/HttpParser.cpp src/http/HttpResponse.cpp src/http/RequestHandler.cpp
MODULES_SRC = src/http/modules/ResponseHandler.cpp src/http/modules/MultipartParser.cpp

B_SRC = src/http/RequestHandler.cpp $(MODULES_SRC) src/core/Mime.cpp src/http/HttpResponse.cpp src/core/Utils.cpp
A_SRC = src/core/LocationParser.cpp src/core/ServerParser.cpp $(EVENT_SRC) src/http/HttpParser.cpp
SRC = main.cpp $(CORE_SRC) $(EVENT_SRC) $(HTTP_SRC) $(MODULES_SRC)
OBJS = $(SRC:.cpp=.o)

ROUTER_NAME = webserv_router
ROUTER_SRC = tests/test_b1_router.cpp $(B_SRC)
ROUTER_OBJS = $(ROUTER_SRC:.cpp=.o)

B2_NAME = webserv_b2
B2_SRC = tests/test_b2_static.cpp $(B_SRC)
B2_OBJS = $(B2_SRC:.cpp=.o)

B3_NAME = webserv_b3
B3_SRC = tests/test_b3_upload_delete.cpp $(B_SRC)
B3_OBJS = $(B3_SRC:.cpp=.o)

B4_NAME = webserv_b4
B4_SRC = tests/test_b4_cgi.cpp $(B_SRC)
B4_OBJS = $(B4_SRC:.cpp=.o)

TEST_CONF_NAME = test_config
TEST_CONF_SRC = tests/test_config.cpp src/core/ServerParser.cpp src/core/LocationParser.cpp src/core/Utils.cpp
TEST_CONF_OBJS = $(TEST_CONF_SRC:.cpp=.o)

TEST_SETUP_NAME = test_setup
TEST_SETUP_SRC = tests/test_setup.cpp $(A_SRC) $(B_SRC)
TEST_SETUP_OBJS = $(TEST_SETUP_SRC:.cpp=.o)

TEST_PARSER_NAME = test_parser
TEST_PARSER_SRC = tests/test_parser.cpp src/http/HttpParser.cpp src/core/Utils.cpp
TEST_PARSER_OBJS = $(TEST_PARSER_SRC:.cpp=.o)

MULTI_CGI_NAME = webserv_multi_cgi
MULTI_CGI_SRC = tests/test_multi_cgi.cpp $(A_SRC) $(B_SRC)
MULTI_CGI_OBJS = $(MULTI_CGI_SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $(NAME) $(CXXFLAGS) $(OBJS)

run: $(NAME)
	clear
	./$(NAME) conf/default.conf

test: $(TEST_CONF_NAME) $(TEST_SETUP_NAME) $(TEST_PARSER_NAME) $(ROUTER_NAME) $(B2_NAME) $(B3_NAME) $(B4_NAME) $(MULTI_CGI_NAME)

$(TEST_CONF_NAME): $(TEST_CONF_OBJS)
	$(CXX) -o $(TEST_CONF_NAME) $(CXXFLAGS) $(TEST_CONF_OBJS)

$(TEST_SETUP_NAME): $(TEST_SETUP_OBJS)
	$(CXX) -o $(TEST_SETUP_NAME) $(CXXFLAGS) $(TEST_SETUP_OBJS)

$(TEST_PARSER_NAME): $(TEST_PARSER_OBJS)
	$(CXX) -o $(TEST_PARSER_NAME) $(CXXFLAGS) $(TEST_PARSER_OBJS)

$(ROUTER_NAME): $(ROUTER_OBJS)
	$(CXX) -o $(ROUTER_NAME) $(CXXFLAGS) $(ROUTER_OBJS)

$(B2_NAME): $(B2_OBJS)
	$(CXX) -o $(B2_NAME) $(CXXFLAGS) $(B2_OBJS)

$(B3_NAME): $(B3_OBJS)
	$(CXX) -o $(B3_NAME) $(CXXFLAGS) $(B3_OBJS)

$(B4_NAME): $(B4_OBJS)
	$(CXX) -o $(B4_NAME) $(CXXFLAGS) $(B4_OBJS)

$(MULTI_CGI_NAME): $(MULTI_CGI_OBJS)
	$(CXX) -o $(MULTI_CGI_NAME) $(CXXFLAGS) $(MULTI_CGI_OBJS)

leak:
	valgrind ./$(NAME) conf/default.conf

clean:
	rm -rf $(OBJS) $(ROUTER_OBJS) $(B2_OBJS) $(B3_OBJS) $(B4_OBJS) $(MULTI_CGI_OBJS) $(MANUAL_OBJS) $(TEST_CONF_OBJS) $(TEST_SETUP_OBJS) $(TEST_PARSER_OBJS)

fclean: clean
	rm -rf $(NAME) $(ROUTER_NAME) $(B2_NAME) $(B3_NAME) $(B4_NAME) $(MULTI_CGI_NAME) $(MANUAL_NAME) $(TEST_CONF_NAME) $(TEST_SETUP_NAME) $(TEST_PARSER_NAME)

42:
	kill -9 $WS 2>/dev/null; clear && make && ./webserv conf/tester_42.conf > /dev/null 2>&1 & WS=$!; echo '' | /usr/bin/time -p ./tester_42 http://127.0.0.1:9000 

re: fclean all

.PHONY: all run leak router b2 b3 b4 multi_cgi manual test clean fclean re
