NAME        = webserv

CXX         = c++
CXXFLAGS    = -std=c++98 -Wall -Wextra -Werror -I src

SRCDIR      = src
SOURCES     = \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/config/ConfigParser.cpp \
	$(SRCDIR)/config/ServerConfig.cpp \
	$(SRCDIR)/config/LocationConfig.cpp \
	$(SRCDIR)/core/ServerManager.cpp \
	$(SRCDIR)/core/Client.cpp \
	$(SRCDIR)/http/HttpRequest.cpp \
	$(SRCDIR)/http/HttpResponse.cpp \
	$(SRCDIR)/http/Router.cpp \
	$(SRCDIR)/handlers/StaticHandler.cpp \
	$(SRCDIR)/handlers/CgiHandler.cpp \
	$(SRCDIR)/handlers/UploadHandler.cpp

OBJDIR      = obj
OBJECTS     = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJECTS)
	@echo "Built $(NAME) successfully."

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
