//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://kylelutz.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_VECTOR_HPP
#define BOOST_COMPUTE_CONTAINER_VECTOR_HPP

#include <vector>
#include <cstddef>
#include <iterator>
#include <exception>

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST) && \
    !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#include <boost/compute/buffer.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/copy_n.hpp>
#include <boost/compute/algorithm/fill_n.hpp>
#include <boost/compute/container/allocator.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/detail/buffer_value.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {

/// \class vector
/// \brief A resizable array of values.
///
/// The vector<T> class stores a dynamic array of values. Internally, the data
/// is stored in an OpenCL buffer object.
///
/// The internal storage is allocated in a specific OpenCL context which is
/// passed as an argument to the constructor when the vector is created.
///
/// \see buffer
template<class T, class Alloc = allocator<T> >
class vector
{
public:
    typedef T value_type;
    typedef Alloc allocator_type;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    typedef detail::buffer_value<T> reference;
    typedef const detail::buffer_value<T> const_reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef buffer_iterator<T> iterator;
    typedef buffer_iterator<T> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    explicit vector(const context &context = system::default_context())
        : m_size(0),
          m_allocator(context)
    {
        m_data = m_allocator.allocate(_minimum_capacity());
    }

    explicit vector(size_type count,
                    const context &context = system::default_context())
        : m_size(count),
          m_allocator(context)
    {
        m_data = m_allocator.allocate((std::max)(count, _minimum_capacity()));
    }

    vector(size_type count,
           const T &value,
           command_queue &queue = system::default_queue())
        : m_size(count),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(count, _minimum_capacity()));

        ::boost::compute::fill_n(begin(), count, value, queue);
    }

    template<class InputIterator>
    vector(InputIterator first,
           InputIterator last,
           command_queue &queue = system::default_queue())
        : m_size(detail::iterator_range_size(first, last)),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(first, last, begin(), queue);
    }

    vector(const vector<T> &other)
        : m_size(other.m_size),
          m_allocator(other.m_allocator)
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        if(!other.empty()){
            command_queue queue = default_queue();
            ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
            queue.finish();
        }
    }

    #if !defined(BOOST_NO_RVALUE_REFERENCES)
    vector(vector<T> &&vector)
        : m_data(std::move(vector.m_data)),
          m_size(vector.m_size),
          m_allocator(std::move(vector.m_allocator))
    {
        vector.m_size = 0;
    }
    #endif // !defined(BOOST_NO_RVALUE_REFERENCES)

    template<class OtherAlloc>
    vector(const std::vector<T, OtherAlloc> &vector,
           command_queue &queue = system::default_queue())
        : m_size(vector.size()),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(vector.begin(), vector.end(), begin(), queue);
    }

    #if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST) && \
        !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
    vector(std::initializer_list<T> list,
           command_queue &queue = system::default_queue())
        : m_size(list.size()),
          m_allocator(queue.get_context())
    {
        m_data = m_allocator.allocate((std::max)(m_size, _minimum_capacity()));

        ::boost::compute::copy(list.begin(), list.end(), begin(), queue);
    }
    #endif // !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    ~vector()
    {
        if(m_size){
            m_allocator.deallocate(m_data, m_size);
        }
    }

    vector<T>& operator=(const vector<T> &other)
    {
        if(this != &other){
            command_queue queue = default_queue();
            resize(other.size(), queue);
            ::boost::compute::copy(other.begin(), other.end(), begin(), queue);
            queue.finish();
        }

        return *this;
    }

    template<class OtherAlloc>
    vector<T>& operator=(const std::vector<T, OtherAlloc> &vector)
    {
        command_queue queue = default_queue();
        resize(vector.size(), queue);
        ::boost::compute::copy(vector.begin(), vector.end(), begin(), queue);
        queue.finish();
        return *this;
    }

    iterator begin()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), 0);
    }

    const_iterator begin() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), 0);
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    iterator end()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), m_size);
    }

    const_iterator end() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_data.get_buffer(), m_size);
    }

    const_iterator cend() const
    {
        return end();
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end() - 1);
    }

    const_reverse_iterator rbegin() const
    {
        return reverse_iterator(end() - 1);
    }

    const_reverse_iterator crbegin() const
    {
        return rbegin();
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin() - 1);
    }

    const_reverse_iterator rend() const
    {
        return reverse_iterator(begin() - 1);
    }

    const_reverse_iterator crend() const
    {
        return rend();
    }

    size_type size() const
    {
        return m_size;
    }

    size_type max_size() const
    {
        return m_allocator.max_size();
    }

    void resize(size_type size, command_queue &queue)
    {
        if(size < capacity()){
            m_size = size;
        }
        else {
            // allocate new buffer
            pointer new_data =
                m_allocator.allocate(
                    static_cast<size_type>(
                        static_cast<float>(size) * _growth_factor()
                    )
                );

            // copy old values to the new buffer
            ::boost::compute::copy(m_data, m_data + m_size, new_data, queue);

            // free old memory
            m_allocator.deallocate(m_data, m_size);

            // set new data and size
            m_data = new_data;
            m_size = size;
        }
    }

    void resize(size_type size)
    {
        command_queue queue = default_queue();
        resize(size, queue);
        queue.finish();
    }

    bool empty() const
    {
        return m_size == 0;
    }

    size_type capacity() const
    {
        return m_data.get_buffer().size() / sizeof(T);
    }

    void reserve(size_type size, command_queue &queue)
    {
        (void) size;
        (void) queue;
    }

    void reserve(size_type size)
    {
        command_queue queue = default_queue();
        reserve(size, queue);
        queue.finish();
    }

    void shrink_to_fit(command_queue &queue)
    {
        (void) queue;
    }

    void shrink_to_fit()
    {
        command_queue queue = default_queue();
        shrink_to_fit(queue);
        queue.finish();
    }

    reference operator[](size_type index)
    {
        return *(begin() + static_cast<difference_type>(index));
    }

    const_reference operator[](size_type index) const
    {
        return *(begin() + static_cast<difference_type>(index));
    }

    reference at(size_type index)
    {
        if(index >= size()){
            BOOST_THROW_EXCEPTION(std::out_of_range("index out of range"));
        }

        return operator[](index);
    }

    const_reference at(size_type index) const
    {
        if(index >= size()){
            BOOST_THROW_EXCEPTION(std::out_of_range("index out of range"));
        }

        return operator[](index);
    }

    reference front()
    {
        return *begin();
    }

    const_reference front() const
    {
        return *begin();
    }

    reference back()
    {
        return *(end() - static_cast<difference_type>(1));
    }

    const_reference back() const
    {
        return *(end() - static_cast<difference_type>(1));
    }

    template<class InputIterator>
    void assign(InputIterator first,
                InputIterator last,
                command_queue &queue)
    {
        ::boost::compute::copy(first, last, begin(), queue);
    }

    template<class InputIterator>
    void assign(InputIterator first, InputIterator last)
    {
        command_queue queue = default_queue();
        assign(first, last, queue);
        queue.finish();
    }

    void assign(size_type n, const T &value, command_queue &queue)
    {
        ::boost::compute::fill_n(begin(), n, value, queue);
    }

    void assign(size_type n, const T &value)
    {
        command_queue queue = default_queue();
        assign(n, value, queue);
        queue.finish();
    }

    void push_back(const T &value, command_queue &queue)
    {
        insert(end(), value, queue);
    }

    void push_back(const T &value)
    {
        command_queue queue = default_queue();
        push_back(value, queue);
        queue.finish();
    }

    void pop_back(command_queue &queue)
    {
        resize(size() - 1, queue);
    }

    void pop_back()
    {
        command_queue queue = default_queue();
        pop_back(queue);
        queue.finish();
    }

    iterator insert(iterator position, const T &value, command_queue &queue)
    {
        if(position == end()){
            resize(m_size + 1, queue);
            position = begin() + position.get_index();
            ::boost::compute::copy_n(&value, 1, position, queue);
        }
        else {
            ::boost::compute::vector<T> tmp(position, end(), queue);
            resize(m_size + 1, queue);
            position = begin() + position.get_index();
            ::boost::compute::copy_n(&value, 1, position, queue);
            ::boost::compute::copy(tmp.begin(), tmp.end(), position + 1, queue);
        }

        return position + 1;
    }

    iterator insert(iterator position, const T &value)
    {
        command_queue queue = default_queue();
        iterator iter = insert(position, value, queue);
        queue.finish();
        return iter;
    }

    void insert(iterator position,
                size_type count,
                const T &value,
                command_queue &queue)
    {
        ::boost::compute::vector<T> tmp(position, end(), queue);
        resize(size() + count, queue);

        position = begin() + position.get_index();

        ::boost::compute::fill_n(position, count, value, queue);
        ::boost::compute::copy(
            tmp.begin(),
            tmp.end(),
            position + static_cast<difference_type>(count),
            queue
        );
    }

    void insert(iterator position, size_type count, const T &value)
    {
        command_queue queue = default_queue();
        insert(position, count, value, queue);
        queue.finish();
    }

    template<class InputIterator>
    void insert(iterator position,
                InputIterator first,
                InputIterator last,
                command_queue &queue)
    {
        ::boost::compute::vector<T> tmp(position, end(), queue);

        size_type count = detail::iterator_range_size(first, last);
        resize(size() + count, queue);

        position = begin() + position.get_index();

        ::boost::compute::copy(first, last, position, queue);
        ::boost::compute::copy(
            tmp.begin(),
            tmp.end(),
            position + static_cast<difference_type>(count),
            queue
        );
    }

    template<class InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last)
    {
        command_queue queue = default_queue();
        insert(position, first, last, queue);
        queue.finish();
    }

    iterator erase(iterator position, command_queue &queue)
    {
        return erase(position, position + 1, queue);
    }

    iterator erase(iterator position)
    {
        command_queue queue = default_queue();
        iterator iter = erase(position, queue);
        queue.finish();
        return iter;
    }

    iterator erase(iterator first, iterator last, command_queue &queue)
    {
        if(last != end()){
            ::boost::compute::vector<T> tmp(last, end(), queue);
            ::boost::compute::copy(tmp.begin(), tmp.end(), first, queue);
        }

        difference_type count = std::distance(first, last);
        resize(size() - static_cast<size_type>(count), queue);

        return begin() + first.get_index() + count;
    }

    iterator erase(iterator first, iterator last)
    {
        command_queue queue = default_queue();
        iterator iter = erase(first, last, queue);
        queue.finish();
        return iter;
    }

    void swap(vector<T> &other)
    {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

    void clear()
    {
        m_size = 0;
    }

    allocator_type get_allocator() const
    {
        return m_allocator;
    }

    /// Returns the buffer.
    const buffer& get_buffer() const
    {
        return m_data.get_buffer();
    }

    /// \internal_
    ///
    /// Returns a command queue usable to issue commands for the vector's
    /// memory buffer. This is used when a member function is called without
    /// speicifying an existing command queue to use.
    command_queue default_queue() const
    {
        const context &context = m_allocator.get_context();
        command_queue queue(context, context.get_device());
        return queue;
    }

private:
    /// \internal_
    BOOST_CONSTEXPR size_type _minimum_capacity() const { return 4; }

    /// \internal_
    BOOST_CONSTEXPR float _growth_factor() const { return 1.5; }

private:
    pointer m_data;
    size_type m_size;
    allocator_type m_allocator;
};

namespace detail {

// set_kernel_arg specialization for vector<T>
template<class T, class Alloc>
struct set_kernel_arg<vector<T, Alloc> >
{
    void operator()(kernel &kernel_, size_t index, const vector<T, Alloc> &vector)
    {
        kernel_.set_arg(index, vector.get_buffer());
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_VECTOR_HPP
