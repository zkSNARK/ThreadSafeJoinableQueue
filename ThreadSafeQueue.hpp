//
//  ThreadSafeQueue.hpp
//  Project: queued_thread_control
//
//  Created by Christopher Goebel on 6/9/19.
//  Copyright © 2019 Christopher Goebel. All rights reserved.
//

#pragma once

#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <optional>


/**
 *  locked_opt_queue class.
 *
 *  A wrapper around std::queue which provides safe threaded access.
 *
 *  Attempts to mimic the behavior of the python queue.
 */
template <typename T>
class ThreadSafeQueue
{
public:
  
  /**
   * Destructor.  Invalidate so that any threads waiting on the
   * condition are notified.
   */
  ~ThreadSafeQueue() {
    stop();
  }
  
  /**
   * Wait on queue condition variable indefinitely until the queue
   * is marked for stop or a value is pushed into the queue.
   */
  std::optional<T> get() {
    std::unique_lock<std::mutex> lock { m_mutex };
    
    // if the queue is empty and has been joined, the queue
    // is no longer valid (could throw here).  Currently
    // returning a failed option.
    if (m_joined and m_queue.size() == 0) {
      return {};
    }
    
    // use wait(lock, predicate) to handle spurious wakeup and
    // termination condition.  We wake up when the queue is not
    // empty or the queue has been joined and is empty.
    m_condition.wait(lock,
      [this]() {
        return !m_queue.empty() or (m_joined and m_queue.empty());
      }
    );
    
    // If we woke up from the condition variable and the queue
    // has been joined and is empty, that meand the queue is 
    // invalid.  Again, could throw here. 
    // We could get here due to spurious wake up or due to the
    // notify from the join call.
    if (m_joined and m_queue.size() == 0) {
      return {};        // return a failed option
    }
    
    T out = std::move(m_queue.front());
    m_queue.pop();
    
    m_condition.notify_all();
    return out;
  }
  
  
  /**
   * Push a new value into the queue.  Returns false if queue
   * is stopped.
   */
  bool push(T value)
  {
    std::unique_lock<std::mutex> lock { m_mutex };
    
    if(m_joined) {
      return false;
    }
    
    m_queue.push(std::move(value));
    lock.unlock();
    
    m_condition.notify_one();
    return true;
  }
  
  /**
   * Check whether or not the queue is empty.
   */
  bool empty() const
  {
    std::scoped_lock<std::mutex> lock { m_mutex };
    return m_queue.empty();
  }
  
  /**
   * A queue is 'complete' when it is both marked for join and empty.
   *
   */
  bool complete(){
    std::scoped_lock<std::mutex> lock { m_mutex };
    return m_joined and m_queue.empty();
  }
  
  /**
   * Shut the queue down by marking the valid bit false and
   * notify any waiting threads.
   *
   * Stop only marks the queue for stopped state.  Doing this
   * disables any further pushes into the queue and
   */
  void stop()
  {
    std::unique_lock<std::mutex> lock { m_mutex };
    m_joined = true;
    
    lock.unlock();
    
    m_condition.notify_all();
  }
  
  
  /**
   * join allows a user to wait on the completion of the
   * jobs remaining in the queue
   */
  void join() {
    std::unique_lock<std::mutex> lock { m_mutex };
    
    m_joined = true;
    
    if (m_queue.size() == 0) {
      return;
    }
    
    // use wait(lock, predicate) to handle spurious wakeup and
    // termination condition
    m_condition.wait(lock,
      [this]() {
        return m_joined and m_queue.empty();
      }
    );
  }
  
  /**
   * Returns the current size of the queue.  Users should be
   * aware that this does not show users true state because
   * as soon as this function exits, the size can change do
   * to the release of the lock.
   */
  inline auto size() const
  {
    std::scoped_lock<std::mutex> lock { m_mutex };
    return m_queue.size();
  }
  
private:
  std::queue<T> m_queue;
  
  // the mutable tag allows const functions to modify the mutex...
  // it tells the const functions "except me... I'm not const."
  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  std::atomic_bool m_joined { false };
};
