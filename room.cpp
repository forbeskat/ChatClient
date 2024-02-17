#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

#include <memory>
#include <utility>

Room::Room(const std::string &room_name)
  : room_name(room_name) {
  pthread_mutex_init(&lock, NULL);
}

Room::~Room() {
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  Guard guard(lock);
  members.insert(user);
}

void Room::remove_member(User *user) {
  Guard guard(lock);
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  Guard guard(lock);
  std::string payload = get_room_name() + ":" + sender_username + ":" + message_text;
  std::set<User *>::iterator it;
  for (it = members.begin(); it != members.end(); it++) {
    // queue a new message to be given to the receiver
    // but don't send it to itself
    if ((*it)->username != sender_username) {
      Message* message = new Message(TAG_DELIVERY, payload);
      (*it)->mqueue.enqueue(message);
    }
  }
}
