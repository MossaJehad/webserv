NAME        = webserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

INCLUDES    = -Isrc/core \
              -Isrc/config \
              -Isrc/net \
              -Isrc/connection \
              -Isrc/http \
              -Isrc/routing \
              -Isrc/handlers \
              -Isrc/cgi \
              -Isrc/util

SRCS        = src/main.cpp \
              src/core/Signal.cpp \
              src/core/Webserv.cpp \
              src/config/ConfigTypes.hpp \
              src/config/LocationConfig.cpp \
              src/config/ServerConfig.cpp \
              src/config/ConfigTokenizer.cpp \
              src/config/ConfigParser.cpp \
              src/config/ConfigValidator.cpp \
              src/net/Socket.cpp \
              src/net/PollRegistry.cpp \
              src/net/Listener.cpp \
              src/net/Reactor.cpp \
              src/connection/IoBuffer.cpp \
              src/connection/Connection.cpp \
              src/connection/ConnectionManager.cpp \
              src/http/HttpStatus.cpp \
              src/http/HttpHeaders.cpp \
              src/http/HttpRequest.cpp \
              src/http/HttpResponse.cpp \
              src/http/ChunkedDecoder.cpp \
              src/http/RequestParser.cpp \
              src/http/ResponseSerializer.cpp \
              src/routing/RequestContext.cpp \
              src/routing/Router.cpp \
              src/handlers/ErrorResponse.cpp \
              src/handlers/AutoindexHandler.cpp \
              src/handlers/RedirectHandler.cpp \
              src/handlers/StaticFileHandler.cpp \
              src/handlers/UploadHandler.cpp \
              src/handlers/DeleteHandler.cpp \
              src/handlers/HandlerFactory.cpp \
              src/cgi/CgiEnvironment.cpp \
              src/cgi/CgiResponseParser.cpp \
              src/cgi/CgiProcess.cpp \
              src/util/Logger.cpp \
              src/util/StringUtils.cpp \
              src/util/FileSystem.cpp \
              src/util/MimeTypes.cpp \
              src/util/Time.cpp

# Filter out non-.cpp files from compilation
CPP_SRCS    = $(filter %.cpp, $(SRCS))
OBJS        = $(CPP_SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
