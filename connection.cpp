#include <sstream>
#include <cctype>
#include <cassert>
#include "client_util.h"
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  rio_readinitb(&m_fdbuf, m_fd);
}

void Connection::connect(const std::string &hostname, int port) {
  m_fd = open_clientfd(hostname.c_str(), std::to_string(port).c_str());
  rio_readinitb(&m_fdbuf, m_fd);
}

Connection::~Connection() {
  if (is_open()) {
    close();
  }
}

bool Connection::is_open() const {
  return m_fd > 0;
}

void Connection::close() {
  if (is_open()) {
    Close(m_fd);
  }
  m_fd = -1;
}

bool Connection::send(const Message &msg) {
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR; 
    return false;
  }

  std::string msg_str = msg.to_string();
  ssize_t bytes_written = rio_writen(m_fd, msg_str.c_str(), msg_str.size());
  if (bytes_written == static_cast<ssize_t>(msg_str.size())) {
    m_last_result = SUCCESS;
    return true;
  } else {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
}

bool Connection::receive(Message &msg) {
  char buf[Message::MAX_LEN+1];
  ssize_t n = rio_readlineb(&m_fdbuf, buf, Message::MAX_LEN);
  if (n <= 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  std::string tempTag ="";
  std::string tempData ="";
  bool hasColon = false;
  bool hasNewLine = false;

  for (unsigned int i = 0; i < Message::MAX_LEN + 1; i++) {
    if (buf[i] == ':') {
      hasColon = true;
    } else if (buf[i] == '\n') {
      hasNewLine = true;
    }
  }

  if (!hasColon || !hasNewLine) {
    m_last_result = INVALID_MSG;
    return false;
  }

  int colon = findFirstColon(buf);
  int newLine = findFirstNewLine(buf);

  if (colon < 0 || newLine < 0 || colon > newLine) {
    m_last_result = INVALID_MSG;
    return false;
  }

  for (int i = 0; i < colon; i++) {
    tempTag.push_back(buf[i]);
  }
  msg.tag = tempTag;
  if (msg.tag != TAG_LEAVE && msg.tag != TAG_QUIT) {
    for (int i = colon + 1; i < newLine; i++) {
      tempData.push_back(buf[i]);
    }
  }
  msg.data = tempData;

  if (!msg.isValid()) {
    m_last_result = INVALID_MSG;
    return false;
  }

  m_last_result = SUCCESS;
  return true;
}

int Connection::findFirstColon(char buf[]) {
  int colon;
  for (unsigned int i = 0; i < Message::MAX_LEN + 1; i++) {
    if (buf[i] == ':') {
      colon = i;
      return colon;
    }
  }

  return -1;
}

int Connection::findFirstNewLine(char buf[]) {
  int newLine;
  for (unsigned int i = 0; i < Message::MAX_LEN + 1; i++) {
    if (buf[i] == '\n') {
      newLine = i;
      return newLine;
    }
  }

  return -1;
}