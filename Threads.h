/*
 * DivideAndConquer.h
 * This file is part of Generic Runtime Support for Parallel Divide and Conquer
 *
 * Copyright (C) 2010 - Lorenzo Anardu, Emanuele Vespa
 *
 * Generic Runtime Support for Parallel Divide and Conquer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Generic Runtime Support for Parallel Divide and Conquer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Generic Runtime Support for Parallel Divide and Conquer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "./Exception.h"

#ifndef DAC_THREADS
#define DAC_THREADS

//#include "./Threads.cpp"

namespace dac {

class Semaphore
{
  int val;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  
public:
  Semaphore();
  Semaphore(int v);
  virtual ~Semaphore();
  
  virtual void wait();
  virtual void tryWait();
  virtual void post();
  virtual int getValue();
};

class Mutex
{
  pthread_mutex_t mutex;
  pthread_mutexattr_t attr;
  
public:
  Mutex();
  virtual ~Mutex();
  
  virtual void lock();
  virtual void tryLock();
  virtual void unlock();
};

class Thread
{
  pthread_t thread;
  pthread_attr_t attributes;
  void *(*startRoutine)(void*);
  
  int retVal;
  
public:
  Thread();
  virtual ~Thread();
  
  virtual void setStackSize(int sz);
  virtual int getStackSize();
  
  virtual void setRoutine(void *(*start_routine)(void*));
  
  virtual void exit();
  virtual size_t join();
  virtual void kill();
  virtual void run(void *params);
};

Semaphore::Semaphore() {
  val = 0;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

Semaphore::Semaphore(int v) {
  val = v;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

Semaphore::~Semaphore() {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}

void Semaphore::wait() {
  pthread_mutex_lock(&mutex);
  while(val==0)
    pthread_cond_wait(&cond, &mutex);
  val--;
  pthread_mutex_unlock(&mutex);
}

void Semaphore::tryWait(){
  
  pthread_mutex_lock(&mutex);
  if(val > 0) val--;
  pthread_mutex_unlock(&mutex);
}

void Semaphore::post(){
  pthread_mutex_lock(&mutex);
  val++;
  if(val == 1) pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
}

int Semaphore::getValue(){
  return val;
}

/**************************************
 * Mutex class implementation
 *************************************/
Mutex::Mutex(){
  int error = pthread_mutexattr_init(&attr);
  if(error != 0) throw NoResourcesException("Mut_attr");
  error = pthread_mutex_init(&mutex, &attr);
  if(error != 0)
    switch(error) {
      case EAGAIN :
      case ENOMEM : throw NoResourcesException("Mut_constr");
        break;
      case EPERM : throw NoPrivilegesException();
        break;
      case EBUSY : throw ReInitException();
        break;
      default : throw MutexException("Mutex()");
    }
}

Mutex::~Mutex(){
//  int error = 
  pthread_mutex_destroy(&mutex);
  pthread_mutexattr_destroy(&attr);
//  if(error != 0) {
//    if(error == EBUSY) throw BusySemaphoreException();
//    else throw MutexException("~Mutex()");
//  }
}

void Mutex::lock() {
  int error = pthread_mutex_lock(&mutex);
  if(error != 0)
    switch(error) {
      case EDEADLK : throw DeadlockException();
        break;
      case EAGAIN : throw MaxLockException();
        break;
      default : throw MutexException("lock()");
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
      default : throw MutexException("tryLock()");
    }
}

void Mutex::unlock() {
  int error = pthread_mutex_unlock(&mutex);
  if(error != 0) {
    if(error == EPERM) throw NoPrivilegesException();
    else throw MutexException("unlock()");
  }
}

/**************************************
 * Thread class implementation
 *************************************/
Thread::Thread() {
  retVal = 0;
  startRoutine = NULL;
  int error = pthread_attr_init(&attributes); //default attributes
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  if(error != 0) throw NoResourcesException("3d_constr");
}

Thread::~Thread() {
  startRoutine = NULL;
  pthread_attr_destroy(&attributes);
  exit();
}

void Thread::setStackSize(int sz) {
  int error = pthread_attr_setstacksize(&attributes, sz);
  if(error != 0) throw StackSizeException();
}

int Thread::getStackSize() {
  size_t  ret;
  pthread_attr_getstacksize(&attributes, &ret);
  return ret;
}

void Thread::setRoutine(void *(*start_routine)(void*)) {
  startRoutine = start_routine;
}

void Thread::run(void *params) {
  int error = 0;
  if(startRoutine != NULL) { error = pthread_create(&thread, &attributes, startRoutine, params);  }//FIXME passargli this?
  else throw StartRoutineException();
  if(error != 0)
    switch (error) {
		case EAGAIN : throw NoResourcesException("3d_run");
			break;
		case EINVAL : throw InvalidAttrException();
			break;
		case EPERM : throw NoPrivilegesException();
			break;
		default: 
			break;
    }
}

void Thread::kill() {
  pthread_cancel(thread);
}

void Thread::exit() {
  pthread_exit(&retVal);
}

size_t Thread::join() {
  int error;
  size_t ret=0;
  error = pthread_join(thread, NULL);
  if(error != 0)
    switch (error) {
		case EINVAL : throw NoJoinableException();
			break;
		case EDEADLK : throw DeadlockException();
			break;
		default : ;
    }
  return ret;
}

} //end dac namespace
#endif //DAD_THREADS
