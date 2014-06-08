#ifndef __SCHEDULER_SCHEDULER_HPP__
#define __SCHEDULER_SCHEDULER_HPP__

#include <scheduler/job.hpp>
#include <cstdint>

namespace OS {

namespace Scheduler {

class Scheduler {
public:
  /**
   * This will create a scheduler of whatever base class has been determined by
   * this build.
   * @noncritical
   */
  static void Initialize();
  
  /**
   * Returns the global initialized scheduler.
   * @ambicritical
   */
  static Scheduler & GetGlobal();
  
  /**
   * Called when a Job is first constructed before it is used, so that you can
   * allocate userInfo for it.
   * @noncritical
   */
  virtual UserInfo * InfoForJob(Job * aJob) = 0;
  
  /**
   * Called when a JobGroup is first constructed before it is used, so that you
   * can allocate userInfo for it.
   * @noncritical
   */
  virtual UserInfo * InfoForJobGroup(JobGroup * aJobGroup) = 0;
  
  /**
   * Set a timer for the current job. After you call this, you should call
   * Tick() from the same critical section.
   * @critical
   */
  virtual void SetTimer(uint64_t fireTime, bool prec) = 0;
  
  /**
   * Set a timer that will never go off. This timer can be cancelled with a
   * call to UnsetTimer().
   * @critical
   */
  virtual void SetInfiniteTimer() = 0;
  
  /**
   * Unset a timer for some job. If a timer was set for the job, this will
   * rescheduler the job regularly.
   * @return true if a timer was removed; false otherwise
   * @critical
   */
  virtual bool UnsetTimer(Job * job) = 0;
  
  /**
   * Called when the current timeout fires.
   * @critical
   */
  virtual void Tick() = 0;
  
  /**
   * @critical
   */
  virtual void AddJob(Job * job) = 0;
  
  /**
   * @critical
   */
  virtual void DeleteJob(Job * job) = 0;
};

}

}

#endif
