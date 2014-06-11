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


#include <algorithm>
#include <limits>

#include "../../include/objectives/objective.hpp"

namespace ghost
{
  /*************/
  /* Objective */
  /*************/
  int Objective::heuristicValue( const std::vector< double > &vecGlobalCosts, 
				 double &bestEstimatedCost,
				 int &bestValue ) const
  {
    int best = 0;
    double bestHelp = numeric_limits<int>::max();

    for( int i = 0; i < vecGlobalCosts.size(); ++i )
    {
      if(      vecGlobalCosts[i] < bestEstimatedCost
	  || ( vecGlobalCosts[i] == bestEstimatedCost
	       && vecGlobalCosts[i] < numeric_limits<int>::max()
	       && ( i == 0 || heuristicValueHelper.at( i ) < bestHelp ) ) )
      {
	bestEstimatedCost = vecGlobalCosts[i];
	bestValue = i - 1;
	if( heuristicValueHelper.at( i ) < bestHelp )
	  bestHelp = heuristicValueHelper.at( i );
	best = i;
      }
    }

    return best;
  }

  void Objective::initHelper( int size )
  {
    heuristicValueHelper = std::vector<double>( size, numeric_limits<int>::max() );
  }

  void Objective::resetHelper()
  {
    std::fill( heuristicValueHelper.begin(), heuristicValueHelper.end(), numeric_limits<int>::max() );
  }
}