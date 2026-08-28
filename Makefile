CXX      = c++
CXXFLAGS = -std=c++98 -pedantic -g

NAME        = webserv
CLIENT_NAME = client

COMMON_SRC = sockt.cpp listensockt.cpp clientsockt.cpp parsing.cpp
SERVER_SRC = server_main.cpp $(COMMON_SRC)
CLIENT_SRC = client_main.cpp $(COMMON_SRC)

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJ) -o $(NAME)

$(CLIENT_NAME): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJ) -o $(CLIENT_NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)

fclean: clean
	rm -f $(NAME) $(CLIENT_NAME)

re: fclean all

.PHONY: all clean fclean re
