#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

struct ClientInfo {
  int connection_fd;
  Server *server;
  Connection *c;

  ~ClientInfo(){
    delete c; //we destroy the individual connection but it's possible that others are still using the same server
  }
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

// Helper function to send ok messages
bool send_ok_msg(Connection * c, std::string payload) {
  Message msg(TAG_OK, payload);
  return c->send(msg);
}

// Helper function to disconnect a receiver in the case of an error
void receiver_disconnect(User* user, ClientInfo* client) {
  Room* room = client->server->find_or_create_room(user->room_name);
  room->remove_member(user);
  delete user;
}

void handle_receive(Message msg, ClientInfo* data, User* user) {
  
  if (!data->c->receive(msg)) {
    Message msg(TAG_ERR, "join room failed.");
    data->c->send(msg);
    return receiver_disconnect(user, data);

  } else if (msg.tag != TAG_JOIN) { //need to join a room first
    Message msg(TAG_ERR, "message was not a join room message.");
    data->c->send(msg);
    return receiver_disconnect(user, data);

  } else { 
    user->room_name = msg.data;
    Room* room = data->server->find_or_create_room(user->room_name);
    room->add_member(user);

    // Send ok message to receiver for room join
    bool joinSuccess = send_ok_msg(data->c, "joined room successfully, ready to receive messages.");
    if (!joinSuccess) return receiver_disconnect(user, data);

    while (1) {
      // Now we can start receiving messages
      Message* grabbed_msg = user->mqueue.dequeue();
      if (grabbed_msg != nullptr) {
        if (!data->c->send(*grabbed_msg)) {
          // Could not send message, so we need to tear down the receiver thread + remove receiver
          return receiver_disconnect(user, data);
        }
      }
    }
  }
}

void handle_send(Message msg, ClientInfo* data, User* user) {
  Room* room = nullptr;
  Message msg_loop;
  while (1) {
    if (!data->c->receive(msg_loop)) { //handle oopsies in msg from sender
      Connection::Result result = data->c->get_last_result();
      if (result == Connection::EOF_OR_ERROR) {
        Message msg(TAG_ERR, "error message received");
        data->c->send(msg);
        return;
      }
      if (result == Connection::INVALID_MSG) {
        Message msg(TAG_ERR, "invalid message received");
        data->c->send(msg);
        return;
      }
      Message msg(TAG_ERR, "server could not receive message.");
      data->c->send(msg);
      continue;
    } else {
      if (msg_loop.tag == TAG_ERR) { //there is some sort of disconnect in communication
        Message msg(TAG_ERR, msg_loop.data);
        data->c->send(msg);
        return;
      } else if (msg.data.length() == Message::MAX_LEN) { //length of desired message exceeds max limit
        Message msg(TAG_ERR, "message is too long");
        data->c->send(msg);
      } else if (msg_loop.tag == TAG_QUIT) { //calling it a day. remove from room (if in room) and return
        Message msg(TAG_OK, "quit");
        if (room != nullptr) {
          room->remove_member(user);
          room=nullptr;
        }
        data->c->send(msg);
        return;
      } else if (msg_loop.tag == TAG_JOIN) {
          if (room != nullptr) { //already in a room. remove from prev first then join user into the desired room
            room->remove_member(user);
          }
          room = data->server->find_or_create_room(msg_loop.data);
          room->add_member(user);
          
          if (!(data->c->send(Message(TAG_OK, "joined the room")))) { //error in joining room
            room->remove_member(user);
            return; 
          }   
      } else if (room == nullptr) { //not currently in a room but trying to do something other than joining
        if (msg.tag != TAG_JOIN) {
          Message msg(TAG_ERR, "join a room first");
          data->c->send(msg);
        }
      
      } else if (msg_loop.tag == TAG_LEAVE) {
        if (room != nullptr) { //leaving room given there is a room to leave
          room->remove_member(user);
          room = nullptr;
          if (!data->c->send(Message(TAG_OK, "leaving room"))){
            return;
          }
        } else { //not in a room, no room to leave. this should generally be picked up by the clause above but just in case
          Message msg(TAG_ERR, "not in a room");
          if (! data->c->send(msg)) return;
        }
      } else if (msg_loop.tag == TAG_SENDALL) { //broadcast message
          if (room != nullptr) { //must be in room to broadcast
              room->broadcast_message(user->username, msg_loop.data);
              if (!data->c->send(Message(TAG_OK, "message broadcasted"))) {
                return;
              }
          } else {
            if (!data->c->send(Message(TAG_ERR, "cannot broadcast because no room to broadcast to"))) return;
          }
      } else {
        Message msg(TAG_ERR, "invalid message");
        if (!data->c->send(msg)) {
          return;
        }
      }
    }
  } 
}

void *worker(void *arg) {
  pthread_detach(pthread_self()); //detach thread
  ClientInfo* data = static_cast<ClientInfo*>(arg); //use a static cast to convert arg from a void* to whatever is needed

  //attempt to receive a properly formatted message
  Message msg;
  if (!data->c->receive(msg)) {
    if (data->c->get_last_result() == Connection::INVALID_MSG) {
      Message msg(TAG_ERR, "invalid message");
      data->c->send(msg);
    } else {
      Message msg(TAG_ERR, "unfounded message");
      data->c->send(msg);
    }
    return nullptr;
  }

  //if it's not a login then the first message is no good
  if(!(msg.tag == TAG_RLOGIN || msg.tag == TAG_SLOGIN)) {
    Message msg(TAG_ERR,"must send login first");
    data->c->send(msg);
    return nullptr;
  }

  //attempt to nicely log the user in
  if(!data->c->send(Message(TAG_OK,"welcome " + msg.data))) {
    return nullptr; //cannot send welcome message
  }

  //communicate w the client
  if (msg.tag == TAG_RLOGIN) { //receiver handling
    User* user = new User(msg.data);
    handle_receive(msg, data, user);
  } else if (msg.tag == TAG_SLOGIN) { //sender handling
    User* user = new User(msg.data);
    handle_send(msg, data, user);
  } else { //this would not be allowed
    Message msg(TAG_ERR, "did not receive login.");
    data->c->send(msg);
    return nullptr;
  }

  //delete data;
  return nullptr;
}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  pthread_mutex_init(&m_lock, NULL);
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen() {
  m_ssock = open_listenfd(std::to_string(m_port).c_str());
  if (m_ssock < 0) { //socket not opened successfully
    std::cerr<<"Error accepting client connection.";
    return false;
  }
  return true; //socket opened successfully
}

void Server::handle_client_requests() {
  while(1) {
    int client_fd = accept(m_ssock, NULL, NULL);
    if (client_fd <= 0) { //connection is bad
      std::cerr << "Error accepting client connection." << std::endl;
      return;
    } else { //connection is good
      ClientInfo* data = new ClientInfo; //set up custom datastructure
      data->server = this;
      data-> connection_fd = client_fd;
      data->c = new Connection(client_fd);
      pthread_t thread;
      if (pthread_create(&thread, NULL, worker, data) != 0) {
        std::cerr << "Error creating thread." << std::endl;
        delete data;
        return;
      }
    }
  }
}

Room *Server::find_or_create_room(const std::string &room_name) {
  Guard guard(m_lock);
  auto r = m_rooms.find(room_name);
  if (r!=m_rooms.end()) { //found room
    return r->second;
  }
  //room not found so create a new room
  m_rooms[room_name] = new Room(room_name);
  return m_rooms[room_name];
}