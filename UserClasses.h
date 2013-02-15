/*
 * UserClasses.h
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
#include <cstdlib>
#include <vector>

#ifndef DAC_USER_CLASSES
#define DAC_USER_CLASSES

namespace dac {

template <class T>
struct Chunk 
{
  unsigned size; //expressed in number of elements of the buffer
	T* buf;
  
  Chunk() {
    size = 0;
    buf = NULL;
  }
  
  virtual ~Chunk() {
    size = 0;
  }
};

/*******************************************
* abstract class for divider
*******************************************/
template <class D>
class Divide
{
public:
  virtual std::vector<Chunk<D> > divide(Chunk<D> &chunk) = 0;
};

/*******************************************
* abstract class for combiner
*******************************************/
template <class C>
class Combine
{
public:
  virtual Chunk<C> combine(std::vector<Chunk<C> > chunks) = 0;
};

/*******************************************
* abstract class for base case handler
*******************************************/
template <class D, class C=D>
class BaseCase
{
public:
  virtual bool isBaseCase(Chunk<D>) = 0;
  virtual Chunk<C> baseSolve(Chunk<D>) = 0;
};

//template <class T>
class SerializableData {
public:
  SerializableData() {}
  virtual ~SerializableData() {}
  
  /* returns the size of the serialized class, expressed in bytes */
  virtual int getSize() = 0;
  
  /* buffer is already allocated into serialize method
   * bufSize returns the size of buffer expressed in bytes*/
  virtual bool serialize(void* buffer, int bufSize) = 0;  
  
  /* buffer is already allocated into deSerialize method
   * bufSize represents the size of buffer expressed in bytes*/ 
  virtual bool deSerialize(void* buffer, int bufSize) = 0;
};

}

#endif
