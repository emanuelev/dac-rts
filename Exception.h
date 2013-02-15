#ifndef DAC_EXCEPTION
#define DAC_EXCEPTION

class Exception
{
public:
	virtual ~Exception() {};
	virtual const char *toString() const = 0;
	virtual const char *suggestion() const = 0;
};

/***************************************
 * TYPES EXCEPTIONS 
 **************************************/
 
class InvalidTypeException : public Exception
{
public:
  const char *toString() const {
    return "InvalidTypeException";
  }
  
  const char *suggestion() const {
    return "Maybe you are using a class which does not extend SerializableData.";
  }
};

/***************************************
 * FILES EXCEPTIONS 
 **************************************/
 
class FileNotFoundException : public Exception
{
  const char *method;
public:
  FileNotFoundException(const char *where) {
    method = where;
  }
  
  virtual ~FileNotFoundException() {
  }
  
  const char *toString() const {
    return "FileNotFoundException";
  }
  
  const char *suggestion() const {
    std::string res("Maybe you have not set some file path (method ");
    res += method;
    res += ").";
    return res.c_str();
  }
};

/***************************************
 * COMMUNICATION EXCEPTIONS 
 **************************************/
 
class InvalidIdException : public Exception
{
  int value;
public:
  InvalidIdException(int v) {
    value = v;
  }
  
  const char *toString() const {
    return "InvalidIdException";
  }
  
  const char *suggestion() const {
    std::string s("Id in a send operation was wrong. Value=");
    s += value;
    s += ".";
    return s.c_str();
  }
};

/***************************************
 * MULTITHREADING EXCEPTIONS 
 **************************************/
class SemaphoreException : public Exception
{
public:  
  const char *toString() const {
    return "SemaphoreException";
  }
  
  const char *suggestion() const {
    return "You are using Semaphore wrong. Or there is some implementation problem.";
  }
};

class SemaphoreValueException : public Exception
{
public: 
  const char *toString() const {
    return "SemaphoreValueException";
  }
  
  const char *suggestion() const {
    return "Maximum value allowed was reached.";
  }
};

class NoResourcesException : public Exception
{
  const char *who;
public:  
  NoResourcesException(const char *w) {
    who = w;
    std::cout<<w<<std::endl;
  }
  const char *toString() const {
    return "NoResourcesException";
  }
  
  const char *suggestion() const {
    std::string str("Some resource missing, or reached the maximum number of Semaphores. On ");
    str += who;
    return str.c_str();
  }
};

class NoPrivilegesException : public Exception
{
public:  
  const char *toString() const {
    return "NoProvilegesException";
  }
  
  const char *suggestion() const {
    return "The process has not the privileges to perform the operation upon a Semaphore/Mutex.";
  }
};

class BusySemaphoreException : public Exception
{
public:  
  const char *toString() const {
    return "BusySemaphoreException";
  }
  
  const char *suggestion() const {
    return "You are trying to destroy a Semaphore/Mutex in which some process is blocked on.";
  }
};

class DeadlockException : public Exception
{
public:  
  const char *toString() const {
    return "DeadlockException";
  }
  
  const char *suggestion() const {
    return "A deadlock condition was detected.";
  }
};

class LockedSemaphoreException : public Exception
{
public:  
  const char *toString() const {
    return "LockedSemaphoreException";
  }
  
  const char *suggestion() const {
    return "You are trying to perform a tryWait()/tryLock() upon a locked Semaphore/Mutex.";
  }
};

class MutexException : public Exception
{
public:
  MutexException(const char *where) {
    std::cout << where << std::endl;
  }
  const char *toString() const {
    return "MutexException";
  }
  
  const char *suggestion() const {
    return "You are using Mutex wrong. Or there is some implementation problem.";
  }
};

class ReInitException : public Exception
{
public:  
  const char *toString() const {
    return "ReInitException";
  }
  
  const char *suggestion() const {
    return "You are trying to initialize a Mutex which was previously initializes, but not yet destroyed.";
  }
};

class MaxLockException : public Exception
{
public:  
  const char *toString() const {
    return "MaxLockException";
  }
  
  const char *suggestion() const {
    return "lock()/tryLock() cannot be executed because the maximum number of pending locks was reached.";
  }
};

class StackSizeException : public Exception
{
public:    
  const char *toString() const {
    return "StackSizeException";
  }
  
  const char *suggestion() const {
    return "You are trying to set a stack size which is less than PTHREAD_STACK_MIN or higher than a system-imposed limit.";
  }
};

class InvalidAttrException : public Exception
{
public:  
  const char *toString() const {
    return "InvalidAttrException";
  }
  
  const char *suggestion() const {
    return "Attributes used for create the thread are invalid.";
  }
};

class StartRoutineException : public Exception
{
public:  
  const char *toString() const {
    return "StartRoutineException";
  }
  
  const char *suggestion() const {
    return "You are trying to launch a thread without any routine to be executed.";
  }
};

class NoJoinableException : public Exception
{
public:  
  const char *toString() const {
    return "NoJoinableException";
  }
  
  const char *suggestion() const {
    return "You are trying to join a non-joinable thread.";
  }
};

class NoMemoryException : public Exception
{
public:  
  const char *toString() const {
    return "NoMemoryException";
  }
  
  const char *suggestion() const {
    return "malloc/calloc/new returned NULL";
  }
};
#endif
