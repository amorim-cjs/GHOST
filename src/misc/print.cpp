/*
 * GHOST (General meta-Heuristic Optimization Solving Tool) is a C++ library 
 * designed to help developers to model and implement optimization problem 
 * solving. It contains a meta-heuristic solver aiming to solve any kind of
 * combinatorial and optimization real-time problems represented by a CSP/COP/CFN. 
 *
 * GHOST has been first developped to help making AI for the RTS game
 * StarCraft: Brood war, but can be used for any kind of applications where 
 * solving combinatorial and optimization problems within some tenth of 
 * milliseconds is needed. It is a generalization of the Wall-in project.
 * Please visit https://github.com/richoux/GHOST for further information.
 * 
 * Copyright (C) 2014-2021 Florian Richoux
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

#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>

#include "misc/print.hpp"

using namespace ghost;

void Print::print_candidate( const std::vector<Variable>& variables ) const
{
	if( !variables.empty() )
	{
		std::cout << "Variables:\n";
		int max_element = std::numeric_limits<int>::min();
		for( const auto& var : variables )
			if( max_element < var.get_value() )
				max_element = var.get_value();
		
		int indent_values = std::ceil( std::log10( max_element ) ) + 1;
		int indent_indexes = std::ceil( std::log10( static_cast<int>( variables.size() ) ) );
		for( int i = 0 ; i < static_cast<int>( variables.size() ) ; ++i )
		{
			if( i % 10 == 0 )
			{
				if( i != 0 )
					std::cout << "\n";
				std::cout << "v[" << std::setw( indent_indexes ) << i << "]:" << std::setw( indent_values ) << variables[i].get_value();
			}
			else
				std::cout << ", v[" << std::setw( indent_indexes ) << i << "]:" << std::setw( indent_values ) << variables[i].get_value();
		}
		std::cout << "\n";
	}
}