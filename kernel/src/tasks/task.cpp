#include "task.hpp"
#include "../scheduler/scheduler.hpp"
#include <anarch/critical>

namespace Alux {

bool Task::Retain() {
  anarch::ScopedCritical critical;
  anarch::ScopedLock scope(lifeLock);
  if (killed && !holdCount) return false;
  ++retainCount;
  return true;
}

void Task::Release() {
  anarch::ScopedCritical critical;
  anarch::ScopedLock scope(lifeLock);
  --retainCount;
  if (!retainCount && !holdCount && killed) {
    ThrowAway();
  }
}

bool Task::Hold() {
  anarch::ScopedCritical critical;
  anarch::ScopedLock scope(lifeLock);
  if (killed) return false;
  ++holdCount;
  return true;
}

void Task::Unhold() {
  anarch::ScopedCritical critical;
  anarch::ScopedLock scope(lifeLock);
  --holdCount;
  if (!retainCount && !holdCount && killed) {
    ThrowAway();
  }
}

void Task::Kill(int status) {
  anarch::ScopedCritical critical;
  anarch::ScopedLock scope(lifeLock);
  if (killed) return;
  killStatus = status;
  killed = true;
}

Scheduler & Task::GetScheduler() const {
  return scheduler;
}

Identifier Task::GetUserIdentifier() const {
  return uid;
}

Task::ThreadMap & Task::GetThreads() {
  return threads;
}

Task::Task(Identifier u, Scheduler & s)
  : GarbageObject(s.GetGarbageCollector()), hashMapLink(*this), uid(u),
    scheduler(s), threads(0x100) {
}

bool Task::Init() {
  AssertNoncritical();
  return scheduler.GetTaskIdMap().Add(*this);
}

void Task::Deinit() {
  AssertNoncritical();
  scheduler.GetTaskIdMap().Remove(*this);
  
  // unschedule threads from the scheduler
  for (int i = 0; i < threads.GetMap().GetBucketCount(); ++i) {
    auto & bucket = threads.GetMap().GetBucket(i);
    for (auto iter = bucket.GetStart(); iter != bucket.GetEnd(); ++iter) {
      Thread & th = *iter;
      scheduler.Remove(th);
      th.Dealloc();
    }
  }
}

}
