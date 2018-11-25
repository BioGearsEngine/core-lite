#ifndef BIOGEARS_CONTAINER_GRAPH_H
#define BIOGEARS_CONTAINER_GRAPH_H

//-------------------------------------------------------------------------------------------
//- Copyright 2017 Applied Research Associates, Inc.
//- Licensed under the Apache License, Version 2.0 (the "License"); you may not use
//- this file except in compliance with the License. You may obtain a copy of the License
//- at:
//- http://www.apache.org/licenses/LICENSE-2.0
//- Unless required by applicable law or agreed to in writing, software distributed under
//- the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//- CONDITIONS OF ANY KIND, either express or implied. See the License for the
//-  specific language governing permissions and limitations under the License.
//-------------------------------------------------------------------------------------------

//! \author Steven A White
//! \date   Nov 21 2018
//! \copyright Copyright 2018 Applied Research Associates, Inc.
//! \license This project is released under the Apache 2.0 license.
//! \file 
//!
//! Simple Tree Structure for storing hiarchial data.
//!

#include <vector> 

namespace biogears {

template <typename Data>
class Tree {
public:
  using value_type = typename Data;
  using reference = value_type&;
  using const_reference = const value_type&;
  using child_vector = std::vector<Tree<Data>>;
  using iterator = typename child_vector::iterator;
  using const_iterator = typename child_vector::const_iterator;

  Tree() = default ;
  Tree(const Data& );
  Tree(Data&&);
  Tree(const Tree&);
  Tree(Tree&&);
  //template< typename ... PARAMS >
  //explicit Tree(const PARAMS&... params);

  virtual ~Tree() = default;

  const Data& value() const;
  Tree<Data>& value(const Data&) &&;
  void  value( const Data& ) &;

  auto children() const & -> const typename child_vector&;
  auto children() && -> typename  child_vector&;

  void children( const std::vector<Tree>& );
  void children( std::vector<Tree>&& );

  auto begin() const -> typename child_vector::iterator;
  auto end() const -> typename child_vector::iterator;
  
  template< class... Args >
  decltype(auto) emplace_back( const Args&... args )
  {
    return _children.emplace_back(args...);
  }
  
  template< class... Args >
  decltype(auto) emplace( typename child_vector::const_iterator pos, Args&&... args )
  {
    return _children.emplace(pos, args...);
  }

  auto operator[]( size_t pos ) -> typename reference;
  auto operator[]( size_t pos ) const -> typename const_reference;

  auto operator=(const Tree&) -> typename Tree&;
  auto operator=(Tree&&) -> typename Tree&;
protected:
  Data _value;
  std::vector<Tree> _children;
};
}

#endif //BIOGEARS_CONTAINER_GRAPH_H