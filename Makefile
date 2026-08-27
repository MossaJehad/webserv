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

CPP_SRCS    = $(filter %.cpp, $(SRCS))
OBJS        = $(CPP_SRCS:.cpp=.o)

# ANSI Color & Style Palette
CLR_RESET   = \033[0m
CLR_BOLD    = \033[1m
CLR_DIM     = \033[2m
CLR_CYAN    = \033[36m
CLR_GREEN   = \033[32m
CLR_YELLOW  = \033[33m
CLR_BLUE    = \033[34m
CLR_MAGENTA = \033[35m
CLR_RED     = \033[31m

all: $(NAME)

$(NAME): $(OBJS)
	@printf "\n$(CLR_BOLD)$(CLR_MAGENTA)⚡ Linking executable: $(NAME)...$(CLR_RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "$(CLR_BOLD)$(CLR_GREEN)╔════════════════════════════════════════════════════════════════════╗$(CLR_RESET)\n"
	@printf "$(CLR_BOLD)$(CLR_GREEN)║  🚀  $(NAME) built successfully! Launch with: ./$(NAME)            ║$(CLR_RESET)\n"
	@printf "$(CLR_BOLD)$(CLR_GREEN)╚════════════════════════════════════════════════════════════════════╝$(CLR_RESET)\n"

%.o: %.cpp
	@printf "$(CLR_BOLD)$(CLR_CYAN)  ⚙️  [Compiling]$(CLR_RESET) $(CLR_DIM)$<$(CLR_RESET)\n"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@printf "$(CLR_BOLD)$(CLR_YELLOW)🧹  Cleaning object files (.o)...$(CLR_RESET)\n"
	@rm -f $(OBJS)
	@printf "$(CLR_BOLD)$(CLR_GREEN)✨  Object files successfully removed!$(CLR_RESET)\n"

fclean:
	@printf "$(CLR_BOLD)$(CLR_RED)🗑️   Purging all build artifacts and $(NAME) executable...$(CLR_RESET)\n"
	@rm -f $(OBJS)
	@rm -f $(NAME)
	@printf "$(CLR_BOLD)$(CLR_GREEN)✨  Full clean complete!$(CLR_RESET)\n"

re:
	@printf "$(CLR_BOLD)$(CLR_MAGENTA)🔄  Rebuilding $(NAME) from scratch...$(CLR_RESET)\n\n"
	@$(MAKE) --no-print-directory fclean
	@printf "\n"
	@$(MAKE) --no-print-directory all

.PHONY: all clean fclean re
