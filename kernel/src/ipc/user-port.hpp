#ifndef __ALUX_USER_PORT_HPP__
#define __ALUX_USER_PORT_HPP__

#include "port.hpp"
#include <ansa/linked-list>

namespace Alux {

class Thread;

class UserPort : public Port {
public:
  UserPort * New(Thread &); // @noncritical
  virtual void Delete(); // @noncritical
  
protected:
  friend class Thread;
  ansa::LinkedList<UserPort>::Link waitingLink;
  
  virtual void Send(const Data & data); // @noncritical
  
private:
  UserPort(Thread &);
  
  Thread & thread;
  
  anarch::NoncriticalLock dataLock;
  bool signaled = false;
  Data data;
};

}

#endif