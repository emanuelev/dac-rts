/*
 * Types.h
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
#include <cstddef>

#include "mpi.h"

#include "./Exception.h"
#include "./UserClasses.h"

#ifndef DAC_TYPES
#define DAC_TYPES

namespace dac {
template <bool v>
struct Bool2Type {
  enum {value = v};
};

/*****************************************
* Structure used for mapping base types on MPI::Datatype
* For user-defined classes/structs returns MPI::BYTE
*****************************************/
template <class T>
struct MpiType {
  inline static const MPI::Datatype &get() { return MPI::BYTE; }
};

/*****************************************
* Mapping of base types on MPI::Datatype struct
*****************************************/
template<>
struct MpiType<char> {
  inline static const MPI::Datatype &get() { return MPI::CHAR; }
};

template<>
struct MpiType<signed char> {
  inline static const MPI::Datatype &get() { return MPI::SIGNED_CHAR; }
};

template<>
struct MpiType<unsigned char> {
  inline static const MPI::Datatype &get() { return MPI::UNSIGNED_CHAR; }
};

template<>
struct MpiType<short> {
  inline static const MPI::Datatype &get() { return MPI::SHORT; }
};

template<>
struct MpiType<unsigned short> {
  inline static const MPI::Datatype &get() { return MPI::UNSIGNED_SHORT; }
};

template<>
struct MpiType<int> {
  inline static const MPI::Datatype &get() { return MPI::INT; }
};

template<>
struct MpiType<unsigned> {
  inline static const MPI::Datatype &get() { return MPI::UNSIGNED; }
};

template<>
struct MpiType<long> {
  inline static const MPI::Datatype &get() { return MPI::LONG; }
};

template<>
struct MpiType<unsigned long> {
  inline static const MPI::Datatype &get() { return MPI::UNSIGNED_LONG; }
};

template<>
struct MpiType<float> {
  inline static const MPI::Datatype &get() { return MPI::FLOAT; }
};

template<>
struct MpiType<double> {
  inline static const MPI::Datatype &get() { return MPI::DOUBLE; }
};

template<>
struct MpiType<long double> {
  inline static const MPI::Datatype &get() { return MPI::LONG_DOUBLE; }
};

/*****************************************
* Function used for test if a type is serializable
* 
* A type is serializable iff it extends SerializableData class (defined in UserClasses.h)
* base types are not serializable.
*****************************************/
#define IS_SERIALIZABLE(U) \
  (Conversion<const U*, const SerializableData*>::exists && \
  !Conversion<const SerializableData*, const void*>::sameType)

template <class T, class U>
struct Conversion {
private:
  typedef char Small;
  class Big { char dummy[2]; };
  
  static Small Test(U);
  static Big Test(...);
  static T MakeT();
  
public:
  enum { exists = sizeof(Test(MakeT())) == sizeof(Small) };
  enum { sameType = false };
};

template<class T>
struct Conversion<T, T> {
	enum {exists = true, sameType = true};
};



}

#endif
