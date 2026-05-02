#pragma once
#include <string>
#include <ctime>
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

enum ClientState {
    CS_READING,
    CS_PROCESSING,
    CS_SENDING,
    CS_CLOSE
};

class Client {
public:
    Client(int fd, const std::string& ip, const ServerConfig* srv);
    ~Client();

    int  getFd() const;
    ClientState getState() const;
    void setState(ClientState s);

    bool readData();
    bool writeData();
    bool isSendDone() const;

    HttpRequest&  getRequest();
    HttpResponse& getResponse();

    const ServerConfig* getServerConfig() const;
    void setServerConfig(const ServerConfig* srv);

    time_t getLastActivity() const;
    void   updateActivity();

    bool isKeepAlive() const;
    void reset();

    const std::string& getIp() const;

private:
    int _fd;
    std::string _ip;
    ClientState _state;
    const ServerConfig* _srv;
    HttpRequest _request;
    HttpResponse _response;
    std::string _sendBuf;
    size_t _sendOffset;
    time_t _lastActivity;
};
