#include <cassert>
#include <ctime>
#include "message.h"
#include "message_queue.h"
#include <semaphore.h>

MessageQueue::MessageQueue() {
  pthread_mutex_init(&m_lock, NULL);
  sem_init(&m_avail, 0, 0);
}

MessageQueue::~MessageQueue() {
  pthread_mutex_destroy(&m_lock);
  sem_destroy(&m_avail);
  std::deque<Message *>::iterator it;
  for (it = m_messages.begin(); it != m_messages.end(); it++) {
    delete (*it);
  }
}

void MessageQueue::enqueue(Message *msg) {
  Guard guard(m_lock);
  m_messages.push_back(msg);
  sem_post(&m_avail);
}

Message *MessageQueue::dequeue() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  // compute a time one second in the future
  ts.tv_sec += 1;

  if (sem_timedwait(&m_avail, &ts) == -1) {
    return nullptr;
  }

  Guard guard(m_lock);
  Message *msg = m_messages.front();
  m_messages.pop_front();
  return msg;
}
