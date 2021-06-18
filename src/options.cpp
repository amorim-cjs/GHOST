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

#include <thread>

#include "options.hpp"

using namespace ghost;

Options::Options()
	: custom_starting_point( false ),
	  resume_search( false ),
	  parallel_runs( false ),
	  number_threads( std::max( 1, static_cast<int>( std::thread::hardware_concurrency() ) ) ), // std::thread::hardware_concurrency() returns 0 if it is not able to detect the number of threads
	  print( std::make_shared<Print>() ),
	  tabu_time_local_min( -1 ),
	  tabu_time_selected( -1 ),
	  reset_threshold( -1 ),
	  restart_threshold( -1 ),
	  percent_to_reset( -1 ),
	  number_start_samplings( 10 )
{ }

Options::Options( const Options& other )
	: custom_starting_point( other.custom_starting_point ),
	  resume_search( other.resume_search ),
	  parallel_runs( other.parallel_runs ),
	  number_threads( other.number_threads ),
	  print( other.print ),
	  tabu_time_local_min( other.tabu_time_local_min ),
	  tabu_time_selected( other.tabu_time_selected ),
	  reset_threshold( other.reset_threshold ),
	  restart_threshold( other.restart_threshold ),
	  percent_to_reset( other.percent_to_reset ),
	  number_start_samplings( other.number_start_samplings )
{ }
		
Options::Options( Options&& other )
	: custom_starting_point( other.custom_starting_point ),
	  resume_search( other.resume_search ),
	  parallel_runs( other.parallel_runs ),
	  number_threads( other.number_threads ),
	  print( std::move( other.print ) ),
	  tabu_time_local_min( other.tabu_time_local_min ),
	  tabu_time_selected( other.tabu_time_selected ),
	  reset_threshold( other.reset_threshold ),
	  restart_threshold( other.restart_threshold ),
	  percent_to_reset( other.percent_to_reset ),
	  number_start_samplings( other.number_start_samplings )
{	}
		
Options& Options::operator=( Options& other )
{
	if( this != &other )
	{
		custom_starting_point = other.custom_starting_point;
		resume_search = other.resume_search;
		parallel_runs = other.parallel_runs;
		number_threads = other.number_threads;
		std::swap( print, other.print );
		tabu_time_local_min = other.tabu_time_local_min;
		tabu_time_selected = other.tabu_time_selected;
		reset_threshold = other.reset_threshold;
		restart_threshold = other.restart_threshold;
		percent_to_reset = other.percent_to_reset;
		number_start_samplings = other.number_start_samplings;
	}
	
	return *this;
}
