NAME=webserv

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -g

B_SRC = src/RequestHandler.cpp src/ResponseHandler.cpp src/Mime.cpp src/HttpResponse.cpp src/Utils.cpp src/MultipartParser.cpp
A_SRC = parser/LocationParser.cpp parser/ServerParser.cpp src/ServerManager.cpp src/Client.cpp src/HttpParser.cpp
SRC = main.cpp $(B_SRC) $(A_SRC)
OBJS = $(SRC:.cpp=.o)

# make router -> B1 otomatik testleri (gecti/kaldi sayisi basar)
ROUTER_NAME = webserv_router
ROUTER_SRC = tests/test_b1_router.cpp $(B_SRC)
ROUTER_OBJS = $(ROUTER_SRC:.cpp=.o)

# make b2 -> statik dosya oyun alani (ciplak cevabi basar)
B2_NAME = webserv_b2
B2_SRC = tests/test_b2_static.cpp $(B_SRC)
B2_OBJS = $(B2_SRC:.cpp=.o)

# make b3 -> upload/DELETE oyun alani
B3_NAME = webserv_b3
B3_SRC = tests/test_b3_upload_delete.cpp $(B_SRC)
B3_OBJS = $(B3_SRC:.cpp=.o)

# make b4 -> CGI oyun alani
B4_NAME = webserv_b4
B4_SRC = tests/test_b4_cgi.cpp $(B_SRC)
B4_OBJS = $(B4_SRC:.cpp=.o)

# make test -> Taraf A CI Testleri
TEST_CONF_NAME = test_config
TEST_CONF_SRC = tests/test_config.cpp parser/ServerParser.cpp parser/LocationParser.cpp src/Utils.cpp
TEST_CONF_OBJS = $(TEST_CONF_SRC:.cpp=.o)

TEST_SETUP_NAME = test_setup
TEST_SETUP_SRC = tests/test_setup.cpp $(A_SRC) $(B_SRC)
TEST_SETUP_OBJS = $(TEST_SETUP_SRC:.cpp=.o)

TEST_PARSER_NAME = test_parser
TEST_PARSER_SRC = tests/test_parser.cpp src/HttpParser.cpp
TEST_PARSER_OBJS = $(TEST_PARSER_SRC:.cpp=.o)

# make manual -> Manuel test sunucusu (tarayici paneli icin)
# Kullanim: ./webserv_manual conf/manual_test.conf
MANUAL_NAME = webserv_manual
MANUAL_SRC = tests/manual_server_main.cpp $(B_SRC) $(A_SRC)
MANUAL_OBJS = $(MANUAL_SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) -o $(NAME) $(CXXFLAGS) $(OBJS)

manual: $(MANUAL_NAME)

$(MANUAL_NAME): $(MANUAL_OBJS)
	$(CXX) -o $(MANUAL_NAME) $(CXXFLAGS) $(MANUAL_OBJS)

run: $(NAME)
	clear
	./$(NAME)

router: $(ROUTER_NAME)
	./$(ROUTER_NAME)

b2: $(B2_NAME)
	./$(B2_NAME)

b3: $(B3_NAME)
	./$(B3_NAME)

b4: $(B4_NAME)
	./$(B4_NAME)

$(ROUTER_NAME): $(ROUTER_OBJS)
	$(CXX) -o $(ROUTER_NAME) $(CXXFLAGS) $(ROUTER_OBJS)

$(B2_NAME): $(B2_OBJS)
	$(CXX) -o $(B2_NAME) $(CXXFLAGS) $(B2_OBJS)

$(B3_NAME): $(B3_OBJS)
	$(CXX) -o $(B3_NAME) $(CXXFLAGS) $(B3_OBJS)

$(B4_NAME): $(B4_OBJS)
	$(CXX) -o $(B4_NAME) $(CXXFLAGS) $(B4_OBJS)

test: $(TEST_CONF_NAME) $(TEST_SETUP_NAME) $(TEST_PARSER_NAME)

$(TEST_CONF_NAME): $(TEST_CONF_OBJS)
	$(CXX) -o $(TEST_CONF_NAME) $(CXXFLAGS) $(TEST_CONF_OBJS)

$(TEST_SETUP_NAME): $(TEST_SETUP_OBJS)
	$(CXX) -o $(TEST_SETUP_NAME) $(CXXFLAGS) $(TEST_SETUP_OBJS)

$(TEST_PARSER_NAME): $(TEST_PARSER_OBJS)
	$(CXX) -o $(TEST_PARSER_NAME) $(CXXFLAGS) $(TEST_PARSER_OBJS)

leak:
	valgrind ./$(NAME)

clean:
	rm -rf $(OBJS) $(ROUTER_OBJS) $(B2_OBJS) $(B3_OBJS) $(B4_OBJS) $(MANUAL_OBJS) $(TEST_CONF_OBJS) $(TEST_SETUP_OBJS) $(TEST_PARSER_OBJS)

fclean: clean
	rm -rf $(NAME) $(ROUTER_NAME) $(B2_NAME) $(B3_NAME) $(B4_NAME) $(MANUAL_NAME) $(TEST_CONF_NAME) $(TEST_SETUP_NAME) $(TEST_PARSER_NAME)

re: fclean all

.PHONY: all run leak router b2 b3 b4 manual test clean fclean re
