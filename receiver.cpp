#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

// Formatter for message output
void outputMsgPayload(Message &msg) {
  std::string payload = msg.data;
  int firstColon = payload.find(':');
  int secondColon = payload.find(':', firstColon + 1);
  std::string message = payload.substr(secondColon + 1);
  std::string name = payload.substr(firstColon + 1, secondColon - firstColon - 1);
  std::cout << name << ": " << message << std::endl;
}


int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection c;
  c.connect(server_hostname, server_port);
  if (!c.is_open()) {  // Check if connection was unsuccessful
    std::cerr << "err: Connection failed." << std::endl;
    return -1;
  }

  Message login, response;
  
  login.tag = TAG_RLOGIN;
  login.data = username;
  if (!c.send(login)) { // Check for login message send failure
    std::cerr << "err: Couldn't send login request." << std::endl;
    c.close();
    return -1;
  }

  if (!c.receive(response)) { // Check for ok from server
    std::cerr << "err: Couldn't get OK response from login." << std::endl;
    c.close();
    return -1;
  }

  if (response.tag == TAG_ERR) { // Check if message from server is error message
    std::cerr << response.data << std::endl;
    c.close();
    return -1;
  }

  Message join(TAG_JOIN, room_name); // Try to join a room
  if (!c.send(join)) {
    std::cerr << "err: Couldn't send join request." << std::endl;
    c.close();
    return -1;
  }

  if (!c.receive(response)) { // Check for ok from server
    std::cerr << "err: Couldn't get OK response from join request." << std::endl;
    c.close();
    return -1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << std::endl;
    c.close();
    return -1;
  }

  while (1) { // Infinite loop to receive messages from sender
    if (!c.receive(response)) { 
      std::cerr << "err: Couldn't receive message." << std::endl;
      return -1;
    }

    if (response.tag == TAG_ERR) { // Check if message from server is error message
      std::cerr << response.data << std::endl;
    }

    if (response.tag == "delivery") {
      outputMsgPayload(response);
    } else {
      std::cerr << "err: Not a delivery message." << std::endl;
    }
  }
  
  return 0;
}
