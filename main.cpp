#include "DivideAndConquer.h"

#include <string>
#include <cstring>

#define TYPE int

using namespace dac;

/*************************************
* main utilized for testing
*************************************/
class MSDivide : public Divide<TYPE>
{
public:
  virtual std::vector<Chunk<TYPE> > divide(Chunk<TYPE> &chunk) { //divide works well
    //dbgShowMsg("DIVIDING")
    std::vector<Chunk<TYPE> > ret;
    int size1 = chunk.size / 2, size2 = chunk.size - size1;
    Chunk<TYPE> temp[2];
       
    temp[0].size=size1;
    temp[0].buf = (TYPE*)calloc(size1,sizeof(TYPE));
    temp[1].size=size2;
    temp[1].buf = (TYPE*)calloc(size2,sizeof(TYPE));
    if(temp[0].buf == NULL || temp[1].buf == NULL) throw NoMemoryException();
    int i;
    for(i=0;i<size1;i++)
      temp[0].buf[i] = chunk.buf[i];
    for(int j=0;j<size2;j++,i++)
      temp[1].buf[j] = chunk.buf[i];
    
//    for(unsigned i=0; i<2500;i++)
//      for(unsigned j=0; j<500;j++) ;
    
    ret.push_back(temp[0]);
    ret.push_back(temp[1]);
    return ret;
  }
};

class MSCombine : public Combine<TYPE>
{
public:
  virtual Chunk<TYPE> combine(std::vector<Chunk<TYPE> > chunks) { //now it works!!
//  dbgShowMsg("COMBINING")
    Chunk<TYPE> ret;
    unsigned i=0, j=0, k=0;
    TYPE *v1=chunks[0].buf, *v2=chunks[1].buf;
//    
//    dbgShowMsg("chunks[0]");
//    for(int l=0;l<chunks[0].size;l++) 
//      std::cout<<chunks[0].buf[l]<<", ";
//    std::cout<<std::endl;
    
    ret.size = chunks[0].size + chunks[1].size;
    ret.buf = (TYPE*)calloc(ret.size, sizeof(TYPE));
    if(ret.buf == NULL) throw NoMemoryException();
    
    while(j<chunks[0].size && k<chunks[1].size) {
      int min = (v1[j]<=v2[k]) ? v1[j++] : v2[k++];
      ret.buf[i++] = min;
    }
    
    if(j==chunks[0].size) {
      while(k<chunks[1].size)
        ret.buf[i++] = v2[k++];
    }
    if(k==chunks[1].size) {
      while(j<chunks[0].size)
        ret.buf[i++] = v1[j++];
    }
//    for(unsigned i=0; i<2500;i++)
//      for(unsigned j=0; j<500;j++) ;
    return ret;
  }
};

class MSBaseCase : public BaseCase<TYPE>
{
public:
  virtual bool isBaseCase(Chunk<TYPE> chunk) { //baseSolve works well
  //std::cout<<"IS BASE? returning"<<(chunk.size == 2)<<std::endl;
    return (chunk.size <= 2);
  }
  virtual Chunk<TYPE> baseSolve(Chunk<TYPE> chunk) { //baseSolve works well
//    dbgShowMsg("SOLVING BASE")
    Chunk<TYPE> ret;
    
    ret.size=chunk.size;
    ret.buf=(TYPE*)calloc(ret.size, sizeof(TYPE));
    if(ret.buf == NULL) throw NoMemoryException();
    if(ret.size == 1) { ret.buf[0] = chunk.buf[0]; return ret; }
    
    if(chunk.buf[0]>chunk.buf[1]) {ret.buf[0]=chunk.buf[1]; ret.buf[1]=chunk.buf[0];}
    else {ret.buf[0]=chunk.buf[0]; ret.buf[1]=chunk.buf[1];}
		
//		for (int i = 0; i < 5000; i++);
//			for (int j = 0 ; j < 500; j++) ;
    
    return ret;
  }
};

void provaSendRecv(void);

int main(int argc, char **argv)
{
  DivideAndConquer<TYPE> dac(2);
  MSDivide d;
  MSCombine c;
  MSBaseCase b;

  dac.setDivider(&d);
  dac.setCombiner(&c);
  dac.setBaseHandler(&b);
  dac.setInputFile("/tmp/input.bin");
  dac.setOutputFile("/tmp/output.bin");
  dac.setDivisionDegree(2);
//  dac.setThreshold(100);
//  dac.setThreadNum(1);
  
  try {
    dac.start(argc, argv);
  } catch (MPI::Exception e) {
    std::cout << e.Get_error_string() << std::endl;
  }
  catch (NoResourcesException e) {
    std::cout << e.suggestion() << std::endl;
  } 
  
  
//  MPI::Init(argc, argv);
//  provaSendRecv();
//  MPI::COMM_WORLD.Barrier();
//  MPI::Finalize();
  
  return 0;
}


class QDivide : public Divide<TYPE>
{
	
public:
	virtual std::vector<Chunk<TYPE> > divide(Chunk<TYPE> &chunk) { 

    std::vector<Chunk<TYPE> > ret;
    
		int pivot;
		pivot = chunk.size / 2;
		TYPE *groupl = NULL, *groupg = NULL, *groupe = NULL;
		int count1 = 0, count2 = 0, count3 = 0;
		
		
		for (unsigned i = 0; i < chunk.size; i++) { // creating groups
			
			if (chunk.buf[i] < chunk.buf[pivot]) {
			
				count1++;
				groupl = (TYPE *) realloc (groupl, sizeof(TYPE) * count1);
				groupl[count1-1] = chunk.buf[i];			
				
			}
			
			if (chunk.buf[i] > chunk.buf[pivot]) {
				
				count2++;
				groupg = (TYPE *) realloc (groupl, sizeof(TYPE) * count2);
				groupg[count2-1] = chunk.buf[i];			
				
			}
			
			if (chunk.buf[i] == chunk.buf[pivot]) {
				
				count3++;
				groupe = (TYPE *) realloc (groupl, sizeof(TYPE) * count3);
				groupe[count3-1] = chunk.buf[i];			
				
			}
			
		}
    
		Chunk<TYPE> lesser, equal, greater;
		lesser.buf = groupl;
		lesser.size = count1;
		
		equal.buf = groupe;
		equal.size = count2;
		
		greater.buf = groupg;
		greater.size = count3;
		
		
		ret.push_back(lesser);
		ret.push_back(equal);
		ret.push_back(greater);
		
		return ret;
	
	} // end divide      
};


class QCombine : public Combine<TYPE>
{
	
public: 
	
	virtual Chunk<TYPE> combine(std::vector< Chunk<TYPE> > chunks){
		
		
		Chunk<TYPE> ret;
		
		ret.buf = (TYPE *) malloc (sizeof(TYPE) * (chunks[0].size + chunks[1].size + chunks[2].size));
		memcpy(ret.buf, chunks[0].buf, chunks[0].size); 
		memcpy(&ret.buf[chunks[0].size], chunks[1].buf, chunks[1].size);
		memcpy(&ret.buf[chunks[1].size], chunks[2].buf, chunks[2].size);
		
		ret.size = chunks[0].size + chunks[1].size + chunks[2].size;
		
		return ret;		
		
	}
};


class QBaseCase : public BaseCase<TYPE> {
public:
	
	virtual bool isBaseCase(Chunk<TYPE> chunk){
		
		
		if (chunk.size > 1) return false;		
		else 	return true;		
	}
	
	virtual Chunk< TYPE> baseSolve(Chunk<TYPE> chunk){ return chunk; }
	
};
	
	

/*
class MyClass : public SerializableData {
private:
  double f1;
  int f2;
  char f3;
  //char *f3;
public:
  MyClass() {
    f1=3.1416;
    f2=22;
    f3='l';
  }
  virtual ~MyClass() {}
  
  virtual int getSize() {
    return (sizeof(double)+sizeof(int)+sizeof(char));
  }
  
  virtual bool serialize(void* buffer, int bufSize) {
    char *buf = (char*)buffer;
    int pos=0;
    
    memcpy(buf,&f1,sizeof(double));
    pos+=sizeof(double);
    memcpy(buf+pos,&f2,sizeof(int));
    pos+=sizeof(int);
    memcpy(buf+pos,&f3,sizeof(char));
    return true;
  } 
  
  virtual bool deSerialize(void* buffer, int bufSize) {
    char *buf=(char*)buffer;
    memcpy(&f1,buf,sizeof(double));
    memcpy(&f2,buf+sizeof(double),sizeof(int));
    memcpy(&f3,buf+sizeof(double)+sizeof(int),sizeof(char));
    //buffer=buf;
    return true;
  }
  
  friend std::ostream& operator<<(std::ostream& out, MyClass& c) {
    out << c.f1 << ", " << c.f2 << ", " << c.f3 << std::endl;
    return out;
  }
};

#define N 20
#define TYPE double

void provaSendRecv(void) {
  Chunk<TYPE> chunk;
  int myRank = MPI::COMM_WORLD.Get_rank();
  //int procN = MPI::COMM_WORLD.Get_size();
  
  if(myRank == 0) {
    srand(time(NULL));
    chunk.size = N;
    chunk.buf = (TYPE *)malloc(N*sizeof(TYPE));
    std::cout << "IsSer=" << IS_SERIALIZABLE(TYPE)<<std::endl;
    std::cout.flush();
    for(int i=0; i<N; i++) {
      chunk.buf[i] = (TYPE)i/(rand()%N+1);
      std::cout << "chunk.buf["<<i<<"]="<<chunk.buf[i]<<std::endl;
      std::cout.flush();
    }
    std::cout<<std::endl<<"PROC 0 - CALL TO SEND"<<std::endl<<std::endl;
    std::cout.flush();
    send(1,chunk,MPI::COMM_WORLD,2,Bool2Type<IS_SERIALIZABLE(TYPE)>());    
  }
  if(myRank == 1) {
    
    MPI::Status status;
      MPI::COMM_WORLD.Probe(MPI::ANY_SOURCE, 2, status);
      std::cout<<std::endl<<"PROC 1 - CALL TO RECV"<<std::endl;
      std::cout.flush();
      recv(0,&chunk,status.Get_count(MPI::BYTE),MPI::COMM_WORLD,2,Bool2Type<IS_SERIALIZABLE(TYPE)>() );
      
    std::cout<<"Chunk size="<<chunk.size<<std::endl;
    for(int i=0; i<chunk.size; i++) {
      std::cout << "chunk.buf["<<i<<"]="<<chunk.buf[i]<<std::endl;
      std::cout.flush();
    }
  }
  
    std::cout<<"PROCESS "<<myRank<<" - Finito"<<std::endl<<std::endl;
    std::cout.flush();
}//*/
/*
class Prova : public SerializableData
{
public: 
  
  int getSize() {return 0;}
  bool serialize(void* buffer, int* bufSize) {return true;}
  bool deSerialize(void* buffer, int bufSize) {return true;}
};

class Prova2
{
public:
  virtual ~Prova2() {}
  int getSize() {return 0;}
  bool serialize(void* buffer, int* bufSize) {return true;}
  bool deSerialize(void* buffer, int bufSize) {return true;}
};//*/
