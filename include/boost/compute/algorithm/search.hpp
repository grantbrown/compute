//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://kylelutz.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_SEARCH_HPP
#define BOOST_COMPUTE_ALGORITHM_SEARCH_HPP

#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/detail/search_all.hpp>
#include <boost/compute/algorithm/find_if.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/system.hpp>

namespace boost {
namespace compute {

///
/// \brief Substring matching algorithm
///
/// Searches for the first match of the pattern [p_first, p_last)
/// in text [t_first, t_last).
/// \return Iterator pointing to beginning of first occurence
///
/// \param p_first Iterator pointing to start of pattern
/// \param p_last Iterator pointing to end of pattern
/// \param t_first Iterator pointing to start of text
/// \param t_last Iterator pointing to end of text
/// \param queue Queue on which to execute
///
template<class PatternIterator, class TextIterator>
inline TextIterator search(PatternIterator p_first,
                           PatternIterator p_last,
                           TextIterator t_first,
                           TextIterator t_last,
                           command_queue &queue = system::default_queue())
{
    vector<uint_> matching_indices(detail::iterator_range_size(t_first, t_last),
                                    queue.get_context());

    detail::search_kernel<PatternIterator,
                          TextIterator,
                          vector<uint_>::iterator> kernel;

    kernel.set_range(p_first, p_last, t_first, t_last, matching_indices.begin());
    kernel.exec(queue);

    using boost::compute::_1;

    vector<uint_>::iterator index = find_if(matching_indices.begin(),
                                            matching_indices.end(),
                                            _1 == 1,
                                            queue);

    return t_first + detail::iterator_range_size(matching_indices.begin(), index);
}

} //end compute namespace
} //end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_SEARCH_HPP
