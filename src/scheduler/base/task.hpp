#ifndef __SCHEDULER_TASK_HPP__
#define __SCHEDULER_TASK_HPP__

#include <scheduler/base/thread.hpp>
#include <scheduler-specific/task-info.hpp>
#include <cstdint>
#include <common>

namespace OS {

class Scheduler;

class Task {
public:
  TaskInfo userInfo;
  
  Task(); // @noncritical
  virtual ~Task(); // @noncritical
  virtual void Delete() = 0; // @noncritical
  
  void AddThread(Thread * th); // @critical
  void RemoveThread(Thread * th); // @critical
  
  bool Retain(); // @critical
  void Release(); // @critical
  bool Hold(); // @critical
  void Unhold(); // @critical
  void Kill(uint64_t status); // @critical

private:
  uint64_t threadsLock OS_ALIGNED(8); // @critical
  Thread * firstThread;
  
  uint64_t stateLock OS_ALIGNED(8); // @critical
  uint64_t retainCount;
  uint64_t holdCount;
  uint64_t killStatus;
  bool isKilled;
  
  void Terminate(); // @critical
};

}

#endif