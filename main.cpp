//
//  main.cpp
//  queued_thread_control
//
//  Created by Christopher Goebel on 6/9/19.
//  Copyright © 2019 Christopher Goebel. All rights reserved.
//


#include "ThreadSafeQueue.hpp"

#include <iostream>
#include <thread>


int main(int argc, const char * argv[]) {
  
  locked_opt_queue<long> counter_queue;
  
  std::thread{
    [&](){
      while (true) {
        auto item = counter_queue.get();
        if (not item) {
          break;
        }
        std::cout << *item << '\n';  // call print queue here
      }
      std::cout << "done thread\n";
    }
  }.detach();
  
  counter_queue.push(1333);
  counter_queue.join();
  
  printf("abc\n");
}
