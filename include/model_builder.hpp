/*
 * GHOST (General meta-Heuristic Optimization Solving Tool) is a C++ framework 
 * designed to help developers to model and implement optimization problem 
 * solving. It contains a meta-heuristic solver aiming to solve any kind of
 * combinatorial and optimization real-time problems represented by a CSP/COP/EFSP/EFOP. 
 *
 * First developped to solve game-related optimization problems, GHOST can be used for
 * any kind of applications where solving combinatorial and optimization problems. In
 * particular, it had been designed to be able to solve not-too-complex problem instances
 * within some milliseconds, making it very suitable for highly reactive or embedded systems.
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

#pragma once

#include <vector>
#include <memory>

#include "model.hpp"

namespace ghost
{
	class ModelBuilder
	{
		template<typename ModelBuilderType> friend class Solver;

		Model build_model();
		
	protected:
		std::vector<Variable> variables; 
		std::vector<std::shared_ptr<Constraint>> constraints; 
		std::shared_ptr<Objective> objective;
		std::shared_ptr<AuxiliaryData> auxiliary_data;

	public:
		virtual ~ModelBuilder() = default;

		virtual void declare_variables() = 0;
		virtual void declare_constraints() = 0;
		virtual void declare_objective();
		virtual void declare_auxiliary_data();

		void create_n_variables( int number, const std::vector<int>& domain, int index = 0 );
		void create_n_variables( int number, int starting_value, std::size_t size, int index = 0 );

		inline int get_number_variables() { return static_cast<int>( variables.size() ); }
	};
}
