/*
 * GHOST (General meta-Heuristic Optimization Solving Tool) is a C++ library 
 * designed for StarCraft: Brood war. 
 * GHOST is a meta-heuristic solver aiming to solve any kind of combinatorial 
 * and optimization RTS-related problems represented by a CSP. 
 * It is an extension of the project Wall-in.
 * Please visit https://github.com/richoux/GHOST for further information.
 * 
 * Copyright (C) 2014 Florian Richoux
 *
 * This file is part of GHOST.
 * GHOST is free software: you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * GHOST is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with GHOST. If not, see http://www.gnu.org/licenses/.
 */


#include <numeric>
#include <typeinfo>

#include "../../include/domain/domain.hpp"

namespace ghost
{
  Domain::Domain( int size, int numberVariables )
    : size(size),
      initialDomain(vector<int>( size )),
      domains(vector< vector<int> >( numberVariables ))
  {
    std::iota( begin(initialDomain), end(initialDomain), -1 );
    for( int i = 0; i < numberVariables; ++i )
      std::copy( begin(initialDomain), end(initialDomain), domains[i] );
  }

  Domain::Domain( int size, int numberVariables, const vector< int > &initialDomain )
    : size(size),
      initialDomain(initialDomain),
      domains(vector< vector<int> >( numberVariables ))
  {
    for( int i = 0; i < numberVariables; ++i )
      std::copy( begin(initialDomain), end(initialDomain), domains[i] );
  }
  
  
  // int Domain::v_randomValue( const Variable &variable ) const
  // {
  //   vector<int> possibilities = domains[ variable.getId() ];
  //   return possibilities[ random.getRandNum( possibilities.size() ) ];
  // }
  
  // vector<int> Domain::v_possibleValues( const Variable &variable ) const
  // {
  //   return domains[ variable.getId() ];
  // }

  // void Domain::v_resetDomain( const Variable &variable )
  // {
  //   domains[ variable.getId() ] = initialDomain;
  // }

  void Domain::v_resetAllDomains()
  {
    for( auto& d : domains )
      d = initialDomain;
  }
  
  ostream& operator<<( ostream&, const Domain &domain )
  {
    os << "Domain type: " <<  typeid(domain).name() << endl
       << "Size: " <<  domain.size << endl;
    for( int i = 0; i < domains.size; ++i )
    {
      os << "Domain[" << i << "]: ";
      for( auto& e : domains[i] )
	os << e << " ";
      os << endl;
    }
    os << endl;
    return os;
  }
}