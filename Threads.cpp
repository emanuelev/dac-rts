//#include "./Threads.h"

/**************************************
 * Semaphore class implementation
 *************************************/
Semaphore::Semaphore() {
  int res = sem_init(&semaphore, 0, 0);
  if(res == -1)
    switch(errno) {
      case ENOSPC : throw NoResourcesException();
        break;
      case EPERM : throw NoPriviligesException();
        break;
      default : throw SemaphoreException();
    }
}

Semaphore::Semaphore(int val) {
  int res = sem_init(&semaphore, 0, val);
  if(res == -1)
    switch(errno) {
      case EINVAL : throw SemaphoreValueException(SEM_VALUE_MAX);
        break;
      case ENOSPC : throw NoResourcesException();
        break;
      case EPERM : throw NoPriviligesException();
        break;
      default : throw SemaphoreException();
    }
}

Semaphore::~Semaphore() {
  int res = sem_destroy(&semaphore);
  if(res == -1)
    if(errno == EBUSY) throw BusySemaphoreException();
    else throw SemaphoreException();
}

void Semaphore::wait() {
  int res = sem_wait(&semaphore);
  if(res == -1)
    if(errno == EDEADLK) throw DeadlockException();
    else throw SemaphoreException();
}

void Semaphore::tryWait(){
  int res = sem_trywait(&semaphore);
  if(res == -1)
    switch(errno) {
      case EDEADLK : throw DeadlockException();
        break;
      case EAGAIN : throw LockedSemaphoreException();
        break;
      default : throw SemaphoreException();
    }
}

void Semaphore::post(){
  sem_post(&semaphore);
}

int Semaphore::getValue(){
  int ret;
  sem_getvalue(&semaphore, &ret);
  return ret;
}

/**************************************
 * Mutex class implementation
 *************************************/
Mutex::Mutex(){
  int error = pthread_mutexattr_init(&attr);
  if(error == 0) throw NoResourcesException();
  error = pthread_mutex_init(&mutex, &attr);
  if(error != 0)
    switch(error) {
      case EAGAIN :
      case ENOMEM : throw NoResourcesException();
        break;
      case EPERM : throw NoPrivilegesException();
        break;
      case EBUSY : throw ReInitException();
        break;
      default : throw MutexException();
    }
}

Mutex::~Mutex(){
  int error = pthread_mutex_destroy(&mutex);
  pthread_mutexattr_destroy(&attr);
  if(error != 0)
    if(error == EBUSY) throw BusySemaphoreException();
    else throw MutexException();
}
  
void Mutex::lock() {
  int error = pthread_mutex_lock(&mutex);
  if(error != 0)
    switch(error) {
      case EDEADLK : throw DeadlockException();
        break;
      case EAGAIN : throw MaxLockException();
        break;
      default : throw MutexException();
    }
}

void Mutex::tryLock() {
  int error = pthread_mutex_trylock(&mutex);
  if(error != 0)
    switch(error) {
      case EBUSY : throw LockedSemaphoreException();
        break;
      case EDEADLK : throw DeadlockException();
        break;
      case EAGAIN : throw MaxLockException();
        break;
      default : throw MutexException();
    }
}

void Mutex::unlock() {
  int error = pthread_mutex_unlock(&mutex);
  if(error != 0)
    if(error == EPERM) throw NoProvilegesException();
    else throw MutexException();
}

/**************************************
 * Thread class implementation
 *************************************/
Thread::Thread() {
  retVal = 0;
  startRoutine = NULL;
  int error = pthread_attr_init(&attributes); //default attributes
  if(error != 0) throw NoResourcesException();
}

Thread::~Thread() {
  startRoutine = NULL;
  pthread_attr_destroy(&attributes);
  exit();
}

void Thread::setStackSize(unsigned int sz) {
  int error = pthread_attr_setstacksize(&attributes, sz);
  if(error != 0) throw StackSizeExcleption(PTHERAD_STACK_MIN);
}

unsigned int Thread::getStackSize() {
  int ret;
  pthread_attr_getstacksize(&attributes, &ret);
  return ret;
}
  
void Thread::setRoutine(void *(*start_routine)(void*)) {
  startRoutine = start_routine;
}
  
void Thread::run() {
  int error = 0;
  if(startRoutine != NULL) error = pthread_create(&thread, &attributes, startRoutine, NULL);  //FIXME passargli this?
  else throw StartRoutineException();
  if(error != 0)
    switch {
      case EAGAIN : throw NoResourcesException();
        break;
      case EINVAL : throw InvalidAttrException();
        break;
      case EPERM : throw NoPrivilegesException();
        break;
      default: ;
    }
}

void Thread::kill() {
  pthread_cancel(thread);
}

void Thread::exit() {
  pthread_exit(&retVal);
}

int Thread::join() {
  int *ret, error;
  error = pthread_join(thread, &ret);
  if(error != 0)
    switch {
      case EINVAL : throw NoJoinableException();
        break;
      case EDEADLK : throw DeadlockException();
        break;
      default : ;
    }
  return (*ret);
}
