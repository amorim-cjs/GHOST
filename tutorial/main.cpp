#include <vector>
#include <functional>
#include <memory>
#include <iostream>

#include <ghost/variable.hpp>
#include <ghost/constraint.hpp>
#include <ghost/solver.hpp>

#if defined OBJECTIVE
#include <ghost/objective.hpp>
#endif

#include "object_data.hpp"
#include "constraint_capacity.hpp"

#if defined OBJECTIVE
#include "max_value.hpp"
#else
#include "at_least.hpp"
#endif

int main()
{
/*
 * Defining variables and eventual additional data about variables (here, object_data)
 */
	std::vector<ghost::Variable> variables;
	std::vector<ObjectData> object_data;

	variables.emplace_back( std::string("bottle"), 0, 51 );
	variables.emplace_back( std::string("sandwich"), 0, 11 );

	object_data.emplace_back( 1, 500 );
	object_data.emplace_back( 1.25, 650 );

/*
 * Defining constraints
 */
	
// Let's make a knapsack with a capacity of 30
	Capacity capacity( variables, object_data, 30 );
	
#if defined OBJECTIVE
	std::vector<std::variant<Capacity>> constraints { capacity };
	
	/*
	 * Defining the objective function
	 */
	
	std::unique_ptr<ghost::Objective> objective = std::make_unique<MaxValue>( variables, object_data );
#else
// We won't accept any object combinations with a total value below 15000
	AtLeast at_least_value( variables, object_data, 15000 );
	
	std::vector<std::variant<Capacity, AtLeast>> constraints{ capacity, at_least_value };
#endif
	
/*
 * Defining the solver and calling it
 */

#if defined OBJECTIVE
	ghost::Solver<Capacity> solver( variables, constraints, std::move( objective ) );
#else
	ghost::Solver<Capacity, AtLeast> solver( variables, constraints );
#endif
	
// cost will store the optimal cost found bu the solver
// solution will store the value of variables giving this score
	double cost = 0.;
	std::vector<int> solution( variables.size(), 0 );

// Run the solver with a 300 microseconds budget (i.e., 0.3 millisecond)
// After 0.3 millisecond, the solver will write in cost and solution the best solution it has found.
	solver.solve( cost, solution, 300 );

	std::cout << "Cost: " << cost << "\nSolution:";
	for( auto v : solution )
		std::cout << " " << v;
	std::cout << "\n";
}
