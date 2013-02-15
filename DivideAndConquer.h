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
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "mpi.h"

#include "./Types.h"
#include "./UserClasses.h"
#include "./Threads.h"


#ifndef DAC_DIVIDE_AND_CONQUER
#define DAC_DIVIDE_AND_CONQUER

#define DAC_DEBUG
#define DEFAULT_THREAD_NUM 1

#define tSend(N) (tSetup + N * tTrasm)
#define tCalc(N) (N * tSeq)


#define SAFE_PUSH(T, Q, E) \
	T.lock.lock(); \
	  T.num_task.post(); \
	  T.Q.push(E); \
	T.lock.unlock();

#define dbgShowVar(M,V) std::cout<<M<<V<<std::endl;\
                        std::cout.flush();
#define dbgShowMsg(M) std::cout<<M<<std::endl;\
                      std::cout.flush();

namespace dac {

template <class T>
struct Task
{
  int task_id;
  int pos;
//  bool local;
  Chunk<T> data;
  
  Task()  { }
  Task(int id, int p, const Chunk<T> &dati)
    : task_id(id), pos(p), data(dati)
  { }
  ~Task() {}
};

/**********************************************
* Main class definition
* template type D: the divide working type;
* template type C: the combine working type;
**********************************************/
template<class D, class C=D>
class DivideAndConquer
{
private:
  /********************************************
  *Structures for internal usage
  ********************************************/
  enum Tags {DIVIDE = 21, COMBINE = 22, KILL = 23, TUNING = 24};
  enum OpModes {LOCAL = 11, DISTRIBUITED = 12};
  
	template <class T>
	struct pendingCombines {
		int task_id;
		int order_id;
		int chunks_left;
//		bool local;
		Chunk<T> *chunks;
		
		pendingCombines(){}
		
		pendingCombines(int tid, int oid, int chunk_num)
		  : task_id(tid), order_id(oid), chunks_left(chunk_num)
		{ chunks = new Chunk<T>[chunk_num];	}
		
		~pendingCombines(){
		}
	};

	//template <class D, class C>
//	struct taskQueue {
//		std::queue<Task<D>*> divideTasks;
//		std::queue<Task<C>*> combineTasks;
//		
//		Semaphore num_task;
//		Mutex lock;
//		
//		taskQueue(){}
//		
//		~taskQueue(){}
//	};
	

private: //private fields
//  int optThr;

  bool initialized;
  const char *inputFileName, *outputFileName;
  double tSetup, tTrasm, tSeq;
  int myNodeId, nodesNumber;
  int divisionDegree;
  
  // thread exclusive-access fields
  int key;
  std::map<int, pendingCombines<C> > combinesMap;
  
	// end thread exclusive-access fields
  
  MPI::Intracomm comm;
  
  // user-provided classes
  Divide<D> *divider;
  Combine<C> *combiner;
  BaseCase<D, C> *baseSolver;
  // end user-provided classes
  
private: //private methods
//	unsigned int getRand();
//	unsigned int getSeq();
//	unsigned int schedule();
	
	void localDac();
	Chunk<C> seqDac(Chunk<D> data);
	void divideBehaviour(Task<D> task);
	void combineBehaviour(Task<C> task);
	
	void checkForIncoming();
	void checkForOutgoing(OpModes mode);

protected:
  virtual void tuneRts();
  virtual void readInputFromFile(Chunk<D> &chunk);
  virtual bool writeOutputIntoFile(Chunk<C> &chunk);
	virtual	void masterRoutine();

public:
  DivideAndConquer();
  DivideAndConquer(int divideDegree);
  virtual ~DivideAndConquer();

public:
  virtual void initialize(int &argc, char **&argv);
  
  virtual void setDivisionDegree(int divideDegree);
  
  virtual void setInputFile(std::string &fileName);
  virtual void setInputFile(const char *fileName);
  virtual void setOutputFile(std::string &fileName);
  virtual void setOutputFile(const char *fileName);
  
  virtual void setDivider(Divide<D> *divider);
  virtual void setCombiner(Combine<C> *combiner);
  virtual void setBaseHandler(BaseCase<D, C> *baseHandler);
  
  virtual void start(int &argc, char **&argv);
};

//template <class T, void(T::*fn)()>
//void *thunk(void *p);

/****************************************
* MAIN CLASS IMPLEMENTATION
****************************************/

/****************************************
* Constructors
****************************************/
template<class D, class C>
DivideAndConquer<D,C>::DivideAndConquer()
{
  initialized = false;
  tSetup = tTrasm = tSeq = key = 0;
  myNodeId = nodesNumber = -1;
  inputFileName = outputFileName = NULL;
  divisionDegree = 0;
}

template<class D, class C>
DivideAndConquer<D,C>::DivideAndConquer(int divideDegree) //fixme insert a default threadNum
  : divisionDegree(divideDegree)
{
  initialized = false;
  tSetup = tTrasm = tSeq = key = 0;
  myNodeId = nodesNumber = -1;
  inputFileName = outputFileName = NULL;
}

/****************************************
* Destructor
****************************************/
template<class D, class C>
DivideAndConquer<D,C>::~DivideAndConquer()
{ }

/****************************************
* Initializes MPI and tunes parameters
****************************************/
template<class D, class C>
void DivideAndConquer<D, C>::initialize(int &argc, char **&argv)
{
  if(!initialized) {
    MPI::Init(argc, argv);
    
    comm = MPI::COMM_WORLD.Dup();
    myNodeId=comm.Get_rank();
    nodesNumber=comm.Get_size();
    
    initialized=true;
    tuneRts();
  }
}

/****************************************
* Tunes tSetup and tTrasm with a dummy send/recv
****************************************/
template <class D, class C>
void DivideAndConquer<D, C>::tuneRts()
{
  double start, end;
  double tSend1, tSend2;
  double times[3]; //used for broadcasting of tSetup and tTrasm 
  Task<D> dummy;
  D val[2];
  
  if(nodesNumber == 1) return;
  
  if(initialized) { //dummy test, this method is invoked by initialize method
    comm.Barrier();
    if(myNodeId == 0) {
      dummy.data.size = 1;
      dummy.data.buf = val;
      start = MPI::Wtime(); //evaluates tSend(1)
      send(1, dummy, comm, TUNING, true);
      end = MPI::Wtime();
      tSend1 = end - start; //tSend(1) expressed in clock ticks number
      
      dummy.data.size = 2;
      start = MPI::Wtime(); //evaluates tSend(2)
      send(1, dummy, comm, TUNING, true);
      end = MPI::Wtime();
      tSend2 = end - start; //tSend(2) expressed in clock ticks number
      
      times[1] = fabs(tSend2 - tSend1); //tTrasm
      times[0] = fabs(tSend1 - times[1]); //tSetup
    }
    else if(myNodeId == 1) {
      MPI::Status s;
      comm.Probe(0, TUNING, s);
      recv(0, &dummy, s.Get_count(MPI::BYTE), comm, TUNING, true);
      comm.Probe(0, TUNING, s);
      recv(0, &dummy, s.Get_count(MPI::BYTE), comm, TUNING, true);
    }
    
    Chunk<D> chunk; //estimates times of user functions
    chunk.size = 100;
    chunk.buf = new D[100];
    
    start = MPI::Wtime();
    seqDac(chunk);
    end = MPI::Wtime();
    
    times[2] = (end - start) / 100;
    
    comm.Bcast(times, 3, MPI::DOUBLE, 0); //process with rank 0 Bcasts tSetup and tTrasm
    tSetup = times[0];
    tTrasm = times[1];
    tSeq = times[2];
    
    if(myNodeId == 0)
        std::cout << "PROCESS " << myNodeId << std::endl \
      << "tSetup=" << tSetup << std::endl \
      << "tTrasm=" << tTrasm << std::endl \
      << "tSeq=" << tSeq << std::endl;
  }
}

/********************************************
*Setter: sets the degree of the division tree
********************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setDivisionDegree(int divideDegree)
{
  this->divisionDegree = divideDegree;
}

/********************************************
*Setter: sets the path to input file (2 overloads)
********************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setInputFile(std::string &fileName)
{
  this->inputFileName = fileName.c_str();
}

template <class D, class C>
void DivideAndConquer<D, C>::setInputFile(const char *fileName)
{
  this->inputFileName = fileName;
}

/********************************************
*Setter: sets the path to output file (2 overloads)
********************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setOutputFile(std::string &fileName)
{
  this->outputFileName = fileName.c_str();
}

template <class D, class C>
void DivideAndConquer<D, C>::setOutputFile(const char *fileName)
{
  this->outputFileName = fileName;
}

/********************************************
 *Setter: sets the divider class
 *******************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setDivider(Divide<D> *divider)
{
  this->divider = divider;
}

/********************************************
 *Setter: sets the combiner class
 *******************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setCombiner(Combine<C> *combiner)
{
  this->combiner = combiner;
}

/********************************************
 *Setter: sets the base case handler class
 *******************************************/ 
template <class D, class C>
void DivideAndConquer<D, C>::setBaseHandler(BaseCase<D, C> *baseHandler)
{
  this->baseSolver = baseHandler;
}

/********************************************
*Performs the input read from the file.
*return: pointer to a buffer containing the data or 0 if something goes wrong
********************************************/
template <class D, class C>
void DivideAndConquer<D, C>::readInputFromFile(Chunk<D> &chunk)
{
//  int file_size;
  MPI::Datatype inputType = MpiType<D>().get();
  MPI::Status status;
  
  if(inputFileName == 0) return; //throw exception?
  
  if(myNodeId == 0) { //process 0 executes the first step
    #ifdef DAC_DEBUG
    std::cout << "PROCESS 0 - Reading from file " << inputFileName << std::endl;
    #endif
    
    std::fstream file(inputFileName, std::fstream::in | std::fstream::binary);
    
    if(!file.good() || file.eof() || !file.is_open()) { std::cout << "Error opening file" <<std::endl; }
    else { std::cout << "File opened well" <<std::endl; }

    
    file.seekg(0, std::ios_base::beg);
    std::ifstream::pos_type begin_pos = file.tellg();
    file.seekg(0, std::ios_base::end);
    int file_size = static_cast<int>(file.tellg() - begin_pos);
		std::cout << "FILE SIZE: " << file_size << std::endl;
    file.seekg(0, std::ios_base::beg);
    
    if(file_size == 0) throw FileNotFoundException("setInputFile");
    
    chunk.size = file_size/sizeof(D); //chunk size expressed in elements number
    chunk.buf = (D*)malloc(file_size);
    if(chunk.buf == NULL) MPI_Abort(comm, 221);
    file.read((char*)chunk.buf,file_size);
    file.close();
  }    
}

/********************************************
 * Performs the output write into the file.
 * return: pointer to a buffer containing the data or 0 if something goes wrong
 *******************************************/
template <class D, class C>
bool DivideAndConquer<D, C>::writeOutputIntoFile(Chunk<C> &chunk)
{
  MPI::Datatype outputType = MpiType<C>().get();
  MPI::Status status;
  
  if(outputFileName == 0) outputFileName = std::string("../output.bin").c_str(); //throw exception?
    
  if(myNodeId==0){ //FIXME va tolto?
  #ifdef DAC_DEBUG
  std::cout << "PROCESS " << myNodeId << " - Writing into file " << outputFileName << std::endl;
  #endif
  
  std::fstream file(outputFileName, std::fstream::out | std::fstream::binary);
  
  file.write((char*)chunk.buf,chunk.size*sizeof(C));
  file.close();
  }
  
  if(status.Get_error() != 0) return false;
  else return true;
}

/**************************************
 * Initializes the environment and starts the D&C.
 *************************************/
template <class D, class C>
void DivideAndConquer<D, C>::start(int &argc, char **&argv)
{
  double start, end;
  initialize(argc, argv);
  start = MPI::Wtime();
  if((nodesNumber == 1)) localDac();
  else masterRoutine();
  end = MPI::Wtime();
  if(myNodeId == 0) {
    std::ostringstream res;
    res << nodesNumber << "\t" << (end-start) << std::endl;
    std::string str = res.str();
    std::fstream file("/tmp/dac_stats.txt", std::fstream::out | std::fstream::binary | std::fstream::app);
    file.write(str.c_str(),str.size());
    file.close();
    std::cout << "Completion time = " << (end-start) << std::endl;
  }
  comm.Barrier();
  MPI::Finalize();
}

/**************************************
 * Performs the D&C in local.
 *************************************/
template <class D, class C>
void DivideAndConquer<D, C>::localDac()
{
  Chunk<D> in;
  Chunk<C> out;
  
  std::cout << "SEQUENTIAL D&C" << std::endl;
  
  readInputFromFile(in);
  out = seqDac(in);
  writeOutputIntoFile(out); 
  
}

/**************************************
 * Executes sequentially the D&C.
 *************************************/
template <class D, class C>
Chunk<C> DivideAndConquer<D, C>::seqDac(Chunk<D> data)
{
  Chunk<C> res;
  if(baseSolver->isBaseCase(data)) res = baseSolver->baseSolve(data);
  else {
    std::vector<Chunk<D> > chunks = divider->divide(data);
    std::vector<Chunk<C> > pendingChunks;
    
    //free(data.buf);
    for(unsigned int i = 0; i<chunks.size(); i++) {
      Chunk<D> temp = chunks[i];
      Chunk<C> chunk = seqDac(temp);
      free(temp.buf); //FIXME?
      pendingChunks.push_back(chunk);
    }
    res = combiner->combine(pendingChunks);
  }
  return res;
}

/**************************************
 * Parallel D&C behaviour.
 * This method is executed by the master thread of any process
 * The master is the only thread allowed to communicate with other processes
 *************************************/
template <class D, class C>
void DivideAndConquer<D, C>::masterRoutine() {
	
	unsigned sent = 0, recvd = 0;
	MPI::Status s;
	std::queue<int> readyCombines;
	std::queue<Task <D> *> chunksQueue;
	Task<D> *inputTask = new Task<D>();
	
		if(myNodeId == 0) { // node 0 behaviour
			inputTask->task_id = -1;
			inputTask->pos = 0;
			
			readInputFromFile(inputTask->data);
			chunksQueue.push(inputTask);
						
			while (chunksQueue.size() < nodesNumber) { //divide
				
				inputTask = chunksQueue.front();
				
				std::vector<Chunk<D> > chunks = divider->divide(inputTask->data);
				pendingCombines<C> pending(inputTask->task_id, inputTask->pos, divisionDegree);
				combinesMap.insert(std::pair<int,pendingCombines<C> >(key, pending));
				
				chunksQueue.pop();				
				
				for(unsigned int i = 0; i < chunks.size(); i++) {
					Task<D> *t = new Task<D>();
					t->task_id = key;
					t->pos = i;
					t->data = chunks[i];					
					chunksQueue.push(t);			
				}
				key++;
			} //end divide
			
			for(int i = 1; i < nodesNumber; i++){ // sends task to processing nodes;
				Task<D> *t = chunksQueue.front();
    		send(i, *t, comm, DIVIDE, false);
    		chunksQueue.pop();
    		sent++;
			} //end send
    }
		else {
		  comm.Probe(0, DIVIDE, s);//check for input divides
		  int dataSizeInBytes = s.Get_count(MPI::BYTE);
		  int elements = (dataSizeInBytes - 3 * sizeof(int)) / sizeof(D);
		  
		  inputTask->data.buf = (D*)calloc(elements, sizeof(D));
		  if(inputTask->data.buf == NULL) MPI_Abort(comm, 221);
		  recv(0, inputTask, dataSizeInBytes, comm, DIVIDE);
		}
		
		if(myNodeId == 0) {
			while(!chunksQueue.empty()){
				Task<D> *task = chunksQueue.front();
				
				Chunk<C> res = seqDac(task->data);
				
				pendingCombines<C> &pending = combinesMap[task->task_id];
				pending.chunks[task->pos] = res;
				pending.chunks_left--;
				
			  if(pending.chunks_left == 0)
			    readyCombines.push(task->task_id);
				
				chunksQueue.pop();
			}
	  }
	  else {
	    Task<C> res;
			
		  double st=MPI::Wtime();
		  res.data = seqDac(inputTask->data);
		  res.task_id = inputTask->task_id;
		  res.pos = inputTask->pos;
//		  res.local = inputTask->local;
		  std::cout<<"WORKER " << myNodeId << " ELAB TIME = "<<(MPI::Wtime()-st)<<std::endl;
		  
		  send(0, res, comm, COMBINE, false);
	  }
	  
	  if(myNodeId == 0) {
		  while(sent > recvd) {	//wait for results from workers	
			  comm.Probe(MPI::ANY_SOURCE, COMBINE, s); //check for input combines
        
				int dataSizeInBytes = s.Get_count(MPI::BYTE);
				int elements = (dataSizeInBytes - 3 * sizeof(int)) / sizeof(C);
				Task<C> *task = new Task<C>();
				  
  			task->data.buf = (C*)calloc(elements, sizeof(C));
	  	  if(task->data.buf == NULL) MPI_Abort(comm, 221);
			  recv(s.Get_source(), task, dataSizeInBytes, comm, COMBINE);
			  recvd++;
				  
			  if(combinesMap.find(task->task_id)!=combinesMap.end()){
		  	  pendingCombines<C> &pending = combinesMap[task->task_id]; //first, save the pending chunk
			    pending.chunks[task->pos] = task->data;
				  pending.chunks_left--;
				  
				  if(pending.chunks_left == 0) 
				    readyCombines.push(task->task_id);
				}
		  } //end recv results
		  
		  while(!readyCombines.empty()) { //final combine
		    pendingCombines<C> &pending = combinesMap[readyCombines.front()];
		    
		    std::vector<Chunk<C> > chunks;
	      chunks.resize(divisionDegree);
	      std::copy(pending.chunks, pending.chunks + divisionDegree, chunks.begin());
	      
		    Chunk<C> res =  combiner->combine(chunks);
		    std::cout<<"task_id="<<pending.task_id<<std::endl;
		    std::cout.flush();
		    if(pending.task_id == -1) {
		      writeOutputIntoFile(res);
		      return;
		    }
		    
		    pendingCombines<C> &temp = combinesMap[pending.task_id];
		    temp.chunks[pending.order_id] = res;
		    temp.chunks_left--;
		    
		    if(temp.chunks_left == 0)
		      readyCombines.push(pending.task_id);
		      
		    for(unsigned i = 0; i<chunks.size(); i++) 
		      free(chunks[i].buf);
		    
		    combinesMap.erase(readyCombines.front());
		    readyCombines.pop();
		  } //end combine phase
		}//end node 0
}

	
/************************************************
 * Gets a random process id, different from the process'self one.
 ***********************************************/
// template <class D, class C>
//unsigned int DivideAndConquer<D, C>::getRand(){
//  static int rand = myNodeId;
//	int temp = rand;
//		
//	while(temp == rand)
//		temp = std::rand()%comm.Get_size();
//		
//	rand = temp;
//	return rand;
//}

/******************************************
 * Gets the next process id, different from the process'self one.
 *****************************************/
//template <class C, class D>
//unsigned int DivideAndConquer<C, D>::getSeq()
//{
//  static int next = myNodeId;
//	
//	next = (next+1) % nodesNumber;
//	if(next == myNodeId) next = (next+1) % nodesNumber;
//	return next;
//}

/*******************************************
 * TODO Implements some schedulation policy.
 ******************************************/
//template <class C, class D>
//unsigned int DivideAndConquer<C, D>::schedule() //TODO chenges method according to some schedulation policy
//{
//  return getSeq();
//}

/*******************************************
 * UTILITY FUNCTIONS
 ******************************************/

/*******************************************
 * Sends a Chunk<T> after serializing it.
 *
 * T is a user-defined serializable type.
 ******************************************/
template <class T> 
void send(int destId, Task<T> &task, const MPI::Intracomm& comm, int tag, Bool2Type<true>, bool test) //MPI::BYTE
{
  int length = 3 * sizeof(int) + task.data.size * sizeof(T), pos = 0;
  char *buf = (char*)malloc(length);
  if(buf == NULL) MPI_Abort(comm, 221);
  
//  MPI::INT.Pack(&task.channel, 1, buf, length, pos, comm);
//  MPI::INT.Pack(&task.owner, 1, buf, length, pos, comm);
  MPI::INT.Pack(&task.task_id, 1, buf, length, pos, comm);
  MPI::INT.Pack(&task.pos, 1, buf, length, pos, comm);
//  MPI::BOOL.Unpack(&task.local, 1, buf, length, pos, comm);
  MPI::INT.Pack(&task.data.size, 1, buf, length, pos, comm);
//  MPI::DOUBLE.Pack(&task.comp_time, 1, buf, length, pos, comm);
  //TODO eventually a chunkId field must be packed here
  {
    char *tempBuf = NULL;
    int size = 0;
    
    for(int i = 0; i<task.data.size; i++) {
      T temp = task.data.buf[i];
      size = temp.getSize();
      tempBuf = (char*)malloc(size);
      if(tempBuf == NULL) MPI_Abort(comm, 221);
      temp.serialize(tempBuf, size);
      MPI::BYTE.Pack(tempBuf, size, buf, length, pos, comm);
      //free(tempBuf);
      size = 0;
    }
  }
  
  if(test) comm.Ssend(buf, pos, MPI::PACKED, destId, tag);
  else comm.Isend(buf, pos, MPI::PACKED, destId, tag);
  free(buf);
}

/*******************************************
 * Sends a Chunk<T> after serializing it.
 *
 * T is a base type.
 ******************************************/
template <class T> 
void send(int destId, Task<T> &task, const MPI::Intracomm& comm, int tag, Bool2Type<false>, bool test) //MPI base types
{
  int length = 3 * sizeof(int) + task.data.size * sizeof(T), pos = 0;
  void *buf = malloc(length);
  if(buf == NULL) MPI_Abort(comm,1);
  
  MPI::INT.Pack(&task.task_id, 1, buf, length, pos, comm);
  MPI::INT.Pack(&task.pos, 1, buf, length, pos, comm);
//  MPI::BOOL.Pack(&task.local, 1, buf, length, pos, comm);
  MPI::INT.Pack(&task.data.size, 1, buf, length, pos, comm);
  (MpiType<T>().get()).Pack(task.data.buf, task.data.size, buf, length, pos, comm);
  
  if(test) comm.Ssend(buf, pos, MPI::PACKED, destId, tag); //used for tuning the RTS
  else comm.Ssend(buf, pos, MPI::PACKED, destId, tag);
  free(buf);
}

/*******************************************
 * Sends a Chunk<T> after serializing it.
 ******************************************/
template <class T> 
void send(int destId, Task<T> &task, const MPI::Intracomm& comm, int tag, bool test = false)
{
//  std::cout << "Process sending to "<<destId<<" "<<task.data.size<<" Elms"<<std::endl;
  if(destId == -1) throw InvalidIdException(-1);
  send(destId, task, comm, tag, Bool2Type<IS_SERIALIZABLE(T)>(), test);
}


/*******************************************
 * Receives a Chunk<T> and deserializes it.
 *
 * lenght is the message lenght expressed in bytes.
 * T is a user-defined serializable type.
 ******************************************/
template <class T> 
void recv(int mittId, Task<T> *task, int length, const MPI::Intracomm& comm, int tag, Bool2Type<true>, bool allocate) //FIXME status is needed?
{
  void *buf = malloc(length);
  int pos = 0;
  MPI::Status status;
  if(buf == NULL) MPI_Abort(comm, 221);
  
  comm.Recv(buf, length, MPI::PACKED, mittId, tag, status);
  
//  MPI::INT.Unpack(buf, length, &(task->owner), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->task_id), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->pos), 1, pos, comm);
//  MPI::BOOL.Unpack(buf, length, &(task->local), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->data.size), 1, pos, comm);
//  MPI::DOUBLE.Unpack(buf, length, &(task->comp_time), 1, pos, comm);
  if(allocate) { 
    task->data.buf = (T*)calloc(task->data.size, sizeof(T));
    if(task->data.buf == NULL) MPI_Abort(comm, 221);
  }
  //TODO eventually a chunkId field must be unpacked here
  { //MPI::BYTE
    T temp = T();
    int size = temp.getSize(); //size in bytes
    char *tempBuf = (char*)malloc(size);
    
    task->data.buf = (T*)calloc(task->data.size, sizeof(T));
    if(task->data.buf == NULL) MPI_Abort(comm, 221);
    for(int i = 0; i<task->data.size; i++) {
      MPI::BYTE.Unpack(buf, length, tempBuf, size, pos, comm);
      temp.deSerialize(tempBuf, size);
      memcpy(&(task->data.buf[i]),&temp,sizeof(T));
    }
  }
  free(buf);
}

/*******************************************
 * Receives a Chunk<T> and deserializes it.
 *
 * lenght is the message lenght expressed in bytes.
 * T is a base type.
 ******************************************/
template <class T> 
void recv(int mittId, Task<T> *task, int length, const MPI::Intracomm& comm, int tag, Bool2Type<false>, bool allocate) //FIXME status is needed?
{
  void *buf = malloc(length);
  int pos = 0;
  MPI::Status status;
  if(buf == NULL) MPI_Abort(comm, 221);
  
//  dbgShowMsg("RECEIVING");
  comm.Recv(buf, length, MPI::PACKED, mittId, tag, status);
//  dbgShowVar("RECEIVED ",length)
//  std::cout<<"Recv status="<<status.Get_error()<<std::endl;
//  MPI::INT.Unpack(buf, length, &(task->owner), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->task_id), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->pos), 1, pos, comm);
//  MPI::BOOL.Unpack(buf, length, &(task->local), 1, pos, comm);
  MPI::INT.Unpack(buf, length, &(task->data.size), 1, pos, comm);
//  MPI::DOUBLE.Unpack(buf, length, &(task->comp_time), 1, pos, comm);
  //TODO eventually a chunkId field must be unpacked here
  if(allocate) {
    task->data.buf = (T*)malloc(task->data.size);
    if(task->data.buf == NULL) MPI_Abort(comm, 221);
  }
  (MpiType<T>().get()).Unpack(buf, length, (task->data.buf), task->data.size, pos, comm);
  
  free(buf); //FIXME
}

/*******************************************
 * Receives a Chunk<T> and deserializes it.
 *
 * lenght is the message lenght expressed in bytes.
 ******************************************/
template <class T>
void recv(int mittId, Task<T> *task, int length, const MPI::Intracomm& comm, int tag, bool allocate = false)
{
//	std::cout << "DEBUG: receiving from : " << mittId;
  recv(mittId, task, length, comm, tag, Bool2Type<IS_SERIALIZABLE(T)>(), allocate);
//  std::cout << " "<<task->data.size<<" Elms"<<std::endl;
}

//template <class T, void(T::*fn)()>
//void *thunk(void *p)
//{
//  (static_cast<T*>(p)->*fn)();
//  return 0;
//}

} //end of dac namespace dac

#endif
