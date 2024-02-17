Synchronization Report:

The critical sections were handeled in our server implementation using the Guard object given to us,
allowing the ability to lock and unlock the mutex. A critical section can be defined as any section
of code where a thread will need to access shared data between threads, and a mutex lock is used to 
prevent threads from accessing shared data simultaneously. 

Critical sections in this assignment include the dequeue and enqueue functions in message_queue.cpp,
the Room member functions of add_member, remove_member, and broadcast_message, and the room map in the 
server object. Additionally, the message_queue and messages themselves are shared data.  
Anytime a client needs to access the room object, it is important to use synchronization
to prevent two or more threads from accessing data at the same time. We determined the critical sections
by identifying sections where multiple threads may have the ability to access the data simultaneously,
which is not what we want, and thus why we use the Guard object. 

For our synchronization primitives, we used both mutexes and semaphores. The mutexes were necessary
to protect data from concurrent access, particularly single shared resources such as a user trying to join a room.
Semaphores as integers were necessary for the message queue to allow the receiver to "sleep" until messages
became available, and the sempahores acted as a counter for this purpose. The semaphore would block a thread
when it went below zero, essentially guaranteeing that different threads wouldn't be trying to access data
that was not available.

Deadlocks can occur in cases where either the mutex is not unlocked or when two threads are waiting for the mutex that the other is holding. It is possible for a thread to be holding multiple mutexes, like in the case of having multiple senders trying to simultaneously send messages to the same room and thus trying to gain access to the same users. If a sender is holding a mutex on a room as it adds messages to the queue, it is going to lock the contents of the message queue. Holding multiple locks at the same time is not an issue in the context of this assignment, however, if multiple threads are doing it out of order, then this is what causes the deadlock to arise. To avoid deadlocks, it is important to make sure the threads only lock one mutex at a time, as long as they are doing it in a consistent order. The nature of this homework allows for the threads to only lock one mutex at a time as only senders are holding mutexes at the same time, thus we don't have to worry about avoiding these types of deadlocks.

A data race happens when two threads access the same mutable object without synchronization, while a race condition
happens when the order of events affects the correctness of the program. We avoid data races and race conditions
by enforcing mutual exclusion in our critical sections using the methods described above to prohibit more than one 
process from accessing shared memory at the same time.


MS1 contributions:

Katherine: wrote receiver and connection
Annie: set up program structures, wrote sender and connection, debugged

MS2 contributions:

Katherine: wrote handle_receive, synchronization stuff, debugging
Annie: wrote handle_send, refactored server, debugging
Dave Hovemeyer: the goat