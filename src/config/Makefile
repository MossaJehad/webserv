CC =  c++ 
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c++98
ARFLAGS = rcs
NAME = webserve
all: $(NAME)
SRCS = main.cpp Config.cpp configParser.cpp Socket.cpp ClientConnection.cpp WebServer.cpp

OBJS =  $(SRCS:.cpp=.o)

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@
clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re