#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }
  std::string server_hostname;
  int server_port;
  std::string username;
  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  Connection c;
  c.connect(server_hostname, server_port);
  if (!c.is_open()) {  //check for valid connection
    std::cerr << "error: connection failed";
    return -1;
  }
  Message slogin(TAG_SLOGIN, username);
  if (!c.send(slogin)) { //attempt to log the user in as a sender
    std::cerr << "error: Can not send login message." << std::endl;
    c.close();
    return -1;
  }

  Message response; //attempt to get a response to the logging in
  if (!c.receive(response)) {
    c.close();
    return -1;
  }
  if (response.tag == TAG_ERR) {
    std::cerr << response.data << std::endl;
    c.close();
    return -1;
  }

  std::string line;
  while (1) { 
    std::getline(std::cin, line);
    std::stringstream ss(line);
    std::string command;
    std::string payload;
    ss >> command; //read in first token which should be the tag to determine what to do with each input
    Message message;
    bool receive_accepted;

    if (command == "/quit") { //we should ideally be quitting with an OK from the server
      message.tag = TAG_QUIT;
      message.data = "";
      c.send(message);
      receive_accepted = c.receive(response);
      if (!receive_accepted) {
        std::cerr << "err: did not receive OK message" << std::endl;
        return -1;
      } else if (response.tag == TAG_ERR) {
        std::cerr << response.data <<std::endl;
        return -1;
      }

    } else if (command == "/join") {
      ss >> payload; //get room
      message.tag = TAG_JOIN;
      message.data = payload;

      std::cout << payload << std::endl;
      if (payload == "") { //no room
        std::cerr << "err: can't join this room" << std::endl;
      } else {
        c.send(message); //attempt to join room
      }

      receive_accepted = c.receive(response);
      if (!receive_accepted) {
        std::cerr << "err: can't reconcile join request" << std::endl;
      }

      if (response.tag == TAG_ERR) {
        std::cerr << response.data <<std::endl;
      }

    } else if (command == "/leave") {
      message.tag = TAG_LEAVE;
      c.send(message); //attempt to leave

    } else { //we assume the program is trying to sendall something
      message.tag = TAG_SENDALL;
      std::getline(ss, payload); //make sure to grab all of the rest of the line as payload
      message.data = command + payload;

      if (!c.send(message)) { //error in sending the message
        std::cerr << "err: can't send this message" << std::endl;
        c.close();
        return -1;
      }

      if (!c.receive(response)) { //error in receiving the response to the message
        std::cerr<< response.data << std::endl;
        c.close();
        return -1;
      }
      
      if (response.tag == TAG_ERR) { //some other error
        std::cerr << response.data << std::endl;
      }
    }  

    if (command == "/quit") {
      return 0;
    }

  }
  
  c.close();
  return 0;
}