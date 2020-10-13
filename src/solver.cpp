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
 * Copyright (C) 2014-2020 Florian Richoux
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

#include "solver.hpp"

using namespace ghost;

Solver::Solver( std::vector<Variable>&	vecVariables, 
                std::vector<std::shared_ptr<Constraint>>&	vecConstraints,
                std::shared_ptr<Objective>	objective,
                bool permutationProblem )
	: _vecVariables	( vecVariables ), 
	  _vecConstraints	( vecConstraints ),
	  _objective ( objective ),
	  _weakTabuList	( vecVariables.size() ),
	  _isOptimization	( objective == nullptr ? false : true ),
	  _permutationProblem	( permutationProblem ),
	  _number_variables	( vecVariables.size() )
{
	for( auto& var : vecVariables )
		for( auto& ctr : vecConstraints )
			if( ctr->has_variable( var ) )
				_mapVarCtr[ var ].push_back( ctr );
}

Solver::Solver( std::vector<Variable>&	vecVariables, 
                std::vector<std::shared_ptr<Constraint>>&	vecConstraints,
                bool permutationProblem )
	: Solver( vecVariables, vecConstraints, nullptr, permutationProblem )
{ }
  

bool Solver::solve( double&	finalCost,
                    std::vector<int>& finalSolution,
                    double satTimeout,
                    double optTimeout,
                    bool no_random_starting_point )
{
	//satTimeout *= 1000; // timeouts in microseconds
	if( optTimeout == 0 )
		optTimeout = satTimeout * 10;
	//else
	//optTimeout *= 1000;

	// The only parameter of Solver<Variable, Constraint>::solve outside timeouts
	int tabuTimeLocalMin = std::max( 1, _number_variables / 2); // _number_variables - 1;
	int tabuTimeSelected = std::max( 1, tabuTimeLocalMin / 2);

	_varOffset = _vecVariables[0]._id;
	for( auto& v : _vecVariables )
		if( v._id < _varOffset )
			_varOffset = v._id;
    
	_ctrOffset = _vecConstraints[0]->get_id();
	for( auto& c : _vecConstraints )
		if( c->get_id() < _ctrOffset )
			_ctrOffset = c->get_id();

#if defined(TRACE)
	cout << "varOffset: " << _varOffset << ", ctrOffset: " << _ctrOffset << "\n";
#endif

	
	std::chrono::duration<double,std::micro> elapsedTime(0);
	std::chrono::duration<double,std::micro> elapsedTimeOptLoop(0);
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> startOptLoop;
	std::chrono::time_point<std::chrono::steady_clock> startPostprocess;
	start = std::chrono::steady_clock::now();

	std::chrono::duration<double,std::micro> timerPostProcessSat(0);
	std::chrono::duration<double,std::micro> timerPostProcessOpt(0);

	// random_device	rd;
	// mt19937		rng( rd() );
	
	if( _objective == nullptr )
		_objective = std::make_shared< NullObjective >();
    
	int optLoop = 0;
	int satLoop = 0;

	double costBeforePostProc = std::numeric_limits<double>::max();
  
	Variable* worstVariable;
	double currentSatCost;
	double currentOptCost;
	std::vector< double > costConstraints( _vecConstraints.size(), 0. );
	std::vector< double > costVariables( _number_variables, 0. );
	std::vector< double > costNonTabuVariables( _number_variables, 0. );

	// In case finalSolution is not a vector of the correct size,
	// ie, equals to the number of variables.
	finalSolution.resize( _number_variables );
  
	_bestSatCost = std::numeric_limits<double>::max();
	_bestOptCost = std::numeric_limits<double>::max();
  
	do // optimization loop
	{
		startOptLoop = std::chrono::steady_clock::now();
		++optLoop;

		// start from a random configuration, if no_random_starting_point is false
		if( !no_random_starting_point )
			set_initial_configuration( 10 );
		else
			// From the second turn in this loop, start from a random configuration
			// TODO: What if the user REALLY wants to start searching from his/her own starting point?
			no_random_starting_point = false;

#if defined(TRACE)
		std::cout << "Generate new config: ";
		for( auto& v : _vecVariables )
			std::cout << v.get_value() << " ";
		std::cout << "\n";
#endif
		
		// Reset weak tabu list
		fill( _weakTabuList.begin(), _weakTabuList.end(), 0 );

		// Reset the best satisfaction cost
		_bestSatCostOptLoop = std::numeric_limits<double>::max();

		do // satisfaction loop 
		{
			++satLoop;

			// Reset variables and constraints costs
			std::fill( costConstraints.begin(), costConstraints.end(), 0. );
			std::fill( costVariables.begin(), costVariables.end(), 0. );

			currentSatCost = compute_constraints_costs( costConstraints );

#if defined(TRACE)
			std::cout << "Cost of constraints: ";
			for( int i = 0; i < costConstraints.size(); ++i )
				std::cout << "c[" << i << "]=" << costConstraints[i] << ", ";
			std::cout << "\n";
#endif

			compute_variables_costs( costConstraints, costVariables, costNonTabuVariables, currentSatCost );

#if defined(TRACE)
			std::cout << "Cost of variables: ";
			for( int i = 0; i < costVariables.size(); ++i )
				std::cout << _vecVariables[i].get_name() << "=" << costVariables[i] << ", ";
			std::cout << "\n";
#endif
			
			bool freeVariables = false;
			decay_weak_tabu_list( freeVariables );

#if defined(TRACE)
			std::cout << "Tabu list: ";
			for( auto& t : _weakTabuList )
				std::cout << t << " ";
			std::cout << "\n";
#endif

#if !defined(EXPERIMENTAL)
			auto worstVariableList = compute_worst_variables( freeVariables, costVariables );

#if defined(TRACE)
			std::cout << "Candidate variables: ";
			for( auto& w : worstVariableList )
				std::cout << w->get_name() << " ";
			std::cout << "\n";
#endif

			if( worstVariableList.size() > 1 )
				worstVariable = _rng.pick( worstVariableList );
			else
				worstVariable = worstVariableList[0];

#if defined(TRACE)
			std::cout << "Picked variable: " << worstVariable->get_name() << "\n";
#endif

#else
			if( freeVariables )
			{
				// discrete_distribution<int> distribution { costNonTabuVariables.begin(), costNonTabuVariables.end() };
				// worstVariable = &(_vecVariables[ distribution( rng ) ]);
				worstVariable = &(_vecVariables[ _rng.variate<int, std::discrete_distribution>( costNonTabuVariables ) ]);
			}
			else
			{
				// discrete_distribution<int> distribution { costVariables.begin(), costVariables.end() };
				// worstVariable = &(_vecVariables[ distribution( rng ) ]);
				worstVariable = &(_vecVariables[ _rng.variate<int, std::discrete_distribution>( costVariables ) ]);
			}      
#endif
			
			if( _permutationProblem )
				permutation_move( worstVariable, costConstraints, costVariables, costNonTabuVariables, currentSatCost );
			else
				local_move( worstVariable, costConstraints, costVariables, costNonTabuVariables, currentSatCost );

			if( _bestSatCostOptLoop > currentSatCost )
			{
				_bestSatCostOptLoop = currentSatCost;

#if defined(TRACE)
				std::cout << "New cost: " << currentSatCost << ", New config: ";
				for( auto& v : _vecVariables )
					std::cout << v.get_value() << " ";
				std::cout << "\n";
#endif
				if( _bestSatCost >= _bestSatCostOptLoop && _bestSatCost > 0 )
				{
					_bestSatCost = _bestSatCostOptLoop;
					for( auto& v : _vecVariables )
						finalSolution[ v._id - _varOffset ] = v.get_value();
				}
				// freeze the variable a bit
				_weakTabuList[ worstVariable->_id - _varOffset ] = tabuTimeSelected;
			}
			else // local minima
				// Mark worstVariable as weak tabu for tabuTimeLocalMin iterations.
				_weakTabuList[ worstVariable->_id - _varOffset ] = tabuTimeLocalMin;

			// for rounding errors
			if( _bestSatCostOptLoop  < 1.0e-10 )
			{
				_bestSatCostOptLoop = 0;
				_bestSatCost = 0;
			}
			
			elapsedTimeOptLoop = std::chrono::steady_clock::now() - startOptLoop;
			elapsedTime = std::chrono::steady_clock::now() - start;
		} // satisfaction loop
		while( _bestSatCostOptLoop > 0. && elapsedTimeOptLoop.count() < satTimeout && elapsedTime.count() < optTimeout );

		if( _bestSatCostOptLoop == 0. )
		{
			currentOptCost = _objective->cost( _vecVariables );
			if( _bestOptCost > currentOptCost )
			{
				update_better_configuration( _bestOptCost, currentOptCost, finalSolution );
				
				startPostprocess = std::chrono::steady_clock::now();
				_objective->postprocess_satisfaction( _vecVariables, _bestOptCost, finalSolution );
				timerPostProcessSat = std::chrono::steady_clock::now() - startPostprocess;
			}
		}
    
		elapsedTime = std::chrono::steady_clock::now() - start;
	} // optimization loop
	//while( elapsedTime.count() < optTimeout );
	while( elapsedTime.count() < optTimeout && ( _isOptimization || _bestOptCost > 0. ) );

	if( _bestSatCost == 0. && _isOptimization )
	{
		costBeforePostProc = _bestOptCost;

		startPostprocess = std::chrono::steady_clock::now();
		_objective->postprocess_optimization( _vecVariables, _bestOptCost, finalSolution );
		timerPostProcessOpt = std::chrono::steady_clock::now() - startPostprocess;							     
	}

	if( _isOptimization )
	{
		if( _bestOptCost < 0 )
		{
			_bestOptCost = -_bestOptCost;
			costBeforePostProc = -costBeforePostProc;
		}
    
		finalCost = _bestOptCost;
	}
	else
		finalCost = _bestSatCost;

	// Set the variables to the best solution values.
	// Useful if the user prefer to directly use the vector of Variables
	// to manipulate and exploit the solution.
	for( auto& v : _vecVariables )
		v.set_value( finalSolution[ v._id - _varOffset ] );

#if defined(DEBUG) || defined(TRACE) || defined(BENCH)
	std::cout << "############" << "\n";
      
	if( !_isOptimization )
		std::cout << "SATISFACTION run" << "\n";
	else
		std::cout << "OPTIMIZATION run with objective " << _objective->get_name() << "\n";

	std::cout << "Elapsed time: " << elapsedTime.count() / 1000 << "ms\n"
	          << "Satisfaction cost: " << _bestSatCost << "\n"
	          << "Number of optization loops: " << optLoop << "\n"
	          << "Number of satisfaction loops: " << satLoop << "\n";

	if( _isOptimization )
		std::cout << "Optimization cost: " << _bestOptCost << "\n"
		          << "Opt Cost BEFORE post-processing: " << costBeforePostProc << "\n";
  
	if( timerPostProcessSat.count() > 0 )
		std::cout << "Satisfaction post-processing time: " << timerPostProcessSat.count() / 1000 << "\n"; 

	if( timerPostProcessOpt.count() > 0 )
		std::cout << "Optimization post-processing time: " << timerPostProcessOpt.count() / 1000 << "\n"; 

	std::cout << "\n";
#endif
          
	return _bestSatCost == 0.;
}

double Solver::compute_constraints_costs( std::vector<double>& costConstraints ) const
{
	double satisfactionCost = 0.;
	double cost;
  
	for( auto& c : _vecConstraints )
	{
		cost = c->cost();
		costConstraints[ c->get_id() - _ctrOffset ] = cost;
		satisfactionCost += cost;    
	}

	return satisfactionCost;
}

void Solver::compute_variables_costs( const std::vector<double>&	costConstraints,
                                      std::vector<double>&	costVariables,
                                      std::vector<double>&	costNonTabuVariables,
                                      const double currentSatCost ) const
{
	int id;
    
	std::fill( costNonTabuVariables.begin(), costNonTabuVariables.end(), 0. );

	for( auto& v : _vecVariables )
	{
		id = v._id - _varOffset;
    
		for( auto& c : _mapVarCtr[ v ] )
			costVariables[ id ] += costConstraints[ c->get_id() - _ctrOffset ];

		if( _weakTabuList[ id ] == 0 )
			costNonTabuVariables[ id ] = costVariables[ id ];
	}
}

void Solver::set_initial_configuration( int samplings )
{
	if( !_permutationProblem )
	{
		if( samplings == 1 )
		{
			monte_carlo_sampling();
		}
		else
		{
			// To avoid weird samplings numbers like 0 or -1
			samplings = std::max( 2, samplings );
    
			double bestSatCost = std::numeric_limits<double>::max();
			double currentSatCost;

			std::vector<int> bestValues( _number_variables, 0 );
    
			for( int i = 0 ; i < samplings ; ++i )
			{
				monte_carlo_sampling();
				currentSatCost = 0.;
				for( auto& c : _vecConstraints )
					currentSatCost += c->cost();
      
				if( bestSatCost > currentSatCost )
					update_better_configuration( bestSatCost, currentSatCost, bestValues );

				if( currentSatCost == 0. )
					break;
			}

			for( int i = 0; i < _number_variables; ++i )
				_vecVariables[ i ].set_value( bestValues[ i ] );
    
			// for( auto& v : _vecVariables )
			//   v.set_value( bestValues[ v._id - _varOffset ] );
		}
	}
	else
	{
		// To avoid weird samplings numbers like 0 or -1
		samplings = std::max( 1, samplings );
		
		double bestSatCost = std::numeric_limits<double>::max();
		double currentSatCost;
		
		std::vector<int> bestValues( _number_variables, 0 );
		
		for( int i = 0 ; i < samplings ; ++i )
		{
			random_permutations();
			currentSatCost = 0.;
			for( auto& c : _vecConstraints )
				currentSatCost += c->cost();
			
			if( bestSatCost > currentSatCost )
				update_better_configuration( bestSatCost, currentSatCost, bestValues );
			
			if( currentSatCost == 0. )
				break;
		}
		
		for( int i = 0; i < _number_variables; ++i )
			_vecVariables[ i ].set_value( bestValues[ i ] );
	}
}

void Solver::monte_carlo_sampling()
{
	for( auto& v : _vecVariables )
		v.pick_random_value();
}

void Solver::random_permutations()
{
	for( unsigned int i = 0; i < _vecVariables.size() - 1; ++i )
		for( unsigned int j = i + 1; j < _vecVariables.size(); ++j )
		{
			// About 50% to do a swap for each couple (var_i, var_j)
			if( _rng.uniform( 0, 1 ) == 0 )
			{
				std::swap( _vecVariables[i]._index, _vecVariables[j]._index );
				std::swap( _vecVariables[i]._cache_value, _vecVariables[j]._cache_value );
			}
		}
}

void Solver::decay_weak_tabu_list( bool& freeVariables ) 
{
	for( int i = 0 ; i < (int)_weakTabuList.size() ; ++i )
	{
		if( _weakTabuList[i] == 0 )
			freeVariables = true;
		else
			--_weakTabuList[i];

		assert( _weakTabuList[i] >= 0 );
	}
}

void Solver::update_better_configuration( double&	best,
                                          const double current,
                                          std::vector<int>& configuration )
{
	best = current;

	for( int i = 0; i < _number_variables; ++i )
		configuration[ i ] = _vecVariables[ i ].get_value();
	// for( auto& v : _vecVariables )
	//   configuration[ v._id - _varOffset ] = v.get_value();
}

#if !defined(EXPERIMENTAL)
std::vector< Variable* > Solver::compute_worst_variables( bool freeVariables,
                                                          const std::vector<double>& costVariables )
{
	// Here, we look at neighbor configurations with the lowest cost.
	std::vector< Variable* > worstVariableList;
	double worstVariableCost = 0.;
	int id;
  
	for( auto& v : _vecVariables )
	{
		id = v._id - _varOffset;
		if( !freeVariables || _weakTabuList[ id ] == 0 )
		{
			if( worstVariableCost < costVariables[ id ] )
			{
				worstVariableCost = costVariables[ id ];
				worstVariableList.clear();
				worstVariableList.push_back( &v );
			}
			else 
				if( worstVariableCost == costVariables[ id ] )
					worstVariableList.push_back( &v );	  
		}
	}
  
	return worstVariableList;
}
#endif
  
// NO VALUE BACKED-UP!
double Solver::simulate_local_move_cost( Variable* variable,
                                         double	value,
                                         const std::vector<double>& costConstraints,
                                         double	currentSatCost ) const
{
	double newCurrentSatCost = currentSatCost;

	variable->set_value( value );
	for( auto& c : _mapVarCtr[ *variable ] )
		newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );

	return newCurrentSatCost;
}

double Solver::simulate_permutation_cost( Variable*	worstVariable,
                                          Variable&	otherVariable,
                                          const std::vector<double>&	costConstraints,
                                          double currentSatCost ) const
{
	double newCurrentSatCost = currentSatCost;
	std::vector<bool> done( costConstraints.size(), false );

	std::swap( worstVariable->_index, otherVariable._index );
	std::swap( worstVariable->_cache_value, otherVariable._cache_value );
    
	for( auto& c : _mapVarCtr[ *worstVariable ] )
	{
		newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );
		done[ c->get_id() - _ctrOffset ] = true;
	}

	// The following was commented to avoid branch misses, but it appears to be slower than
	// the commented block that follows.
	for( auto& c : _mapVarCtr[ otherVariable ] )
		if( !done[ c->get_id() - _ctrOffset ] )
			newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );

	// vector< shared_ptr<Constraint> > diff;
	// std::set_difference( _mapVarCtr[ otherVariable ].begin(), _mapVarCtr[ otherVariable ].end(),
	//                      _mapVarCtr[ *worstVariable ].begin(), _mapVarCtr[ *worstVariable ].end(),
	//                      std::inserter( diff, diff.begin() ) );

	// for( auto& c : diff )
	// 	newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );

	// We must roll back to the previous state before returning the new cost value. 
	std::swap( worstVariable->_index, otherVariable._index );
	std::swap( worstVariable->_cache_value, otherVariable._cache_value );

	return newCurrentSatCost;
}

void Solver::local_move( Variable* variable,
                         std::vector<double>& costConstraints,
                         std::vector<double>& costVariables,
                         std::vector<double>& costNonTabuVariables,
                         double& currentSatCost )
{
	// Here, we look at values in the variable domain
	// leading to the lowest satisfaction cost.
	double newCurrentSatCost = 0.0;
	std::vector< int > bestValuesList;
	int bestValue;
	double bestCost = std::numeric_limits<double>::max();
  
	for( auto& val : variable->possible_values() )
	{
		newCurrentSatCost = simulate_local_move_cost( variable, val, costConstraints, currentSatCost );
		if( bestCost > newCurrentSatCost )
		{
			bestCost = newCurrentSatCost;
			bestValuesList.clear();
			bestValuesList.push_back( val );
		}
		else 
			if( bestCost == newCurrentSatCost )
				bestValuesList.push_back( val );	  
	}

	// If several values lead to the same best satisfaction cost,
	// call Objective::heuristic_value has a tie-break.
	// By default, Objective::heuristic_value returns the value
	// improving the most the optimization cost, or a random value
	// among values improving the most the optimization cost if there
	// are some ties.
	if( bestValuesList.size() > 1 )
		bestValue = _objective->heuristic_value( _vecVariables, *variable, bestValuesList );
	else
		bestValue = bestValuesList[0];

	variable->set_value( bestValue );
	currentSatCost = bestCost;
	// for( auto& c : _mapVarCtr[ *variable ] )
	//   costConstraints[ c->get_id() - _ctrOffset ] = c->cost();

	// compute_variables_costs( costConstraints, costVariables, costNonTabuVariables, currentSatCost );
}

void Solver::permutation_move( Variable* variable,
                               std::vector<double>& costConstraints,
                               std::vector<double>& costVariables,
                               std::vector<double>& costNonTabuVariables,
                               double& currentSatCost )
{
	// Here, we look at values in the variable domain
	// leading to the lowest satisfaction cost.
	double newCurrentSatCost = 0.0;
	std::vector< Variable* > bestVarToSwapList;
	Variable* bestVarToSwap;
	double bestCost = std::numeric_limits<double>::max();

#if defined(TRACE)
	std::cout << "Current cost before permutation: " << currentSatCost << "\n";
#endif

	
	for( auto& otherVariable : _vecVariables )
	{
		// Next line is replaced by a simpler conditional since there were A LOT of branch misses!
		//if( otherVariable._id == variable->_id || otherVariable._index == variable->_index )
		if( otherVariable._id == variable->_id )
			continue;
    
		newCurrentSatCost = simulate_permutation_cost( variable, otherVariable, costConstraints, currentSatCost );

#if defined(TRACE)
		std::cout << "Cost if permutation between " << variable->_id << " and " << otherVariable._id << ": " << newCurrentSatCost << "\n";
#endif

		if( bestCost > newCurrentSatCost )
		{
#if defined(TRACE)
			std::cout << "This is a new best cost.\n";
#endif
			
			bestCost = newCurrentSatCost;
			bestVarToSwapList.clear();
			bestVarToSwapList.push_back( &otherVariable );
		}
		else 
			if( bestCost == newCurrentSatCost )
			{
				bestVarToSwapList.push_back( &otherVariable );
#if defined(TRACE)
				std::cout << "Tie cost with the best one.\n";
#endif
			}
	}

	// // If the best cost found so far leads to a plateau,
	// // then we have 10% of chance to escapte from the plateau
	// // by picking up a random variable (giving a worst global cost)
	// // to permute with.
	// if( bestCost == currentSatCost && _rng.uniform( 0, 99 ) < 10 )
	// {
	// 	do
	// 	{
	// 		bestVarToSwap = _rng.pick( _vecVariables );
	// 	} while( bestVarToSwap._id == variable->_id || std::find_if( bestVarToSwapList.begin(), bestVarToSwapList.end(), [&bestVarToSwap](auto& v){ return v._id == bestVarToSwap._id; } ) != bestVarToSwapList.end() );
	// }
	// else
	// {
	// If several values lead to the same best satisfaction cost,
	// call Objective::heuristic_value has a tie-break.
	// By default, Objective::heuristic_value returns the value
	// improving the most the optimization cost, or a random value
	// among values improving the most the optimization cost if there
	// are some ties.
	if( bestVarToSwapList.size() > 1 )
		bestVarToSwap = _objective->heuristic_value( bestVarToSwapList );
	else
		bestVarToSwap = bestVarToSwapList[0];
	// }

#if defined(TRACE)
	std::cout << "Permutation will be done between " << variable->_id << " and " << bestVarToSwap->_id << ".\n";
#endif

	std::swap( variable->_index, bestVarToSwap->_index );
	std::swap( variable->_cache_value, bestVarToSwap->_cache_value );

	currentSatCost = bestCost;
	// vector<bool> compted( costConstraints.size(), false );
  
	// for( auto& c : _mapVarCtr[ *variable ] )
	// {
	// 	newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );
	// 	compted[ c->get_id() - _ctrOffset ] = true;
	// }
  
	// for( auto& c : _mapVarCtr[ bestVarToSwap ] )
	// 	if( !compted[ c->get_id() - _ctrOffset ] )
	// 		newCurrentSatCost += ( c->cost() - costConstraints[ c->get_id() - _ctrOffset ] );

	// compute_variables_costs( costConstraints, costVariables, costNonTabuVariables, newCurrentSatCost );
}
