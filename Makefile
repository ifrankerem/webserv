NAME=webserv

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -Iincludes -g
# -MMD: her .o icin yanina .d dosyasi yazar, icinde o dosyanin dahil ettigi
#       header'lar listelidir. -MP: silinen bir header make'i kilitlemesin diye
#       her header icin bos kural ekler.
DEPFLAGS = -MMD -MP
B_SRC = src/RequestHandler.cpp src/ResponseHandler.cpp src/Mime.cpp src/HttpResponse.cpp src/Utils.cpp
A_SRC = parser/LocationParser.cpp parser/ServerParser.cpp src/ServerManager.cpp src/Client.cpp src/HttpParser.cpp
SRC = main.cpp $(B_SRC) $(A_SRC)
OBJS = $(SRC:.cpp=.o)

# make router -> B1 otomatik testleri (gecti/kaldi sayisi basar)
ROUTER_NAME = webserv_router
ROUTER_SRC = tests/test_router.cpp $(B_SRC)
ROUTER_OBJS = $(ROUTER_SRC:.cpp=.o)

# make b2 -> statik dosya oyun alani (ciplak cevabi basar)
B2_NAME = webserv_b2
B2_SRC = tests/deneme_b2.cpp $(B_SRC)
B2_OBJS = $(B2_SRC:.cpp=.o)

# make b3 -> upload/DELETE oyun alani
B3_NAME = webserv_b3
B3_SRC = tests/deneme_b3.cpp $(B_SRC)
B3_OBJS = $(B3_SRC:.cpp=.o)

# make b4 -> CGI oyun alani
B4_NAME = webserv_b4
B4_SRC = tests/deneme_b4.cpp $(B_SRC)
B4_OBJS = $(B4_SRC:.cpp=.o)

# Butun .o dosyalarinin bagimlilik listeleri
DEPS = $(OBJS:.o=.d) $(ROUTER_OBJS:.o=.d) $(B2_OBJS:.o=.d) $(B3_OBJS:.o=.d) $(B4_OBJS:.o=.d)

all: $(NAME)

# Kendi derleme kuralimiz: built-in kural .d uretmiyor.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) -o $(NAME) $(CXXFLAGS) $(OBJS)

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

leak:
	valgrind ./$(NAME)

clean:
	rm -rf $(OBJS) $(ROUTER_OBJS) $(B2_OBJS) $(B3_OBJS) $(B4_OBJS) $(DEPS)

fclean: clean
	rm -rf $(NAME) $(ROUTER_NAME) $(B2_NAME) $(B3_NAME) $(B4_NAME)

re: fclean all

# Header degisince ilgili .o yeniden derlensin diye bagimlilik listelerini oku.
# Ilk derlemede .d dosyalari henuz yok, bu yuzden '-' ile hatayi yut.
-include $(DEPS)

.PHONY: all run leak router b2 b3 b4 clean fclean re
