#pragma once

#include <cassert>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <list>
#include <algorithm>
#include <ranges>
#include <type_traits>





namespace my_adt
{
	template <typename T, typename Allocator, typename ChunkListAllocator>
	class stable_vector;

	template <typename T>
	concept pointer_type =  std::is_pointer_v<T>;

	template <pointer_type T>
	struct add_const_to_pointer
	{
		using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>;
	};
	template <pointer_type T>
	using add_const_to_pointer_t = add_const_to_pointer<T>::type;

	namespace detail
	{
		template <typename T, typename Allocator = std::allocator<T>>
		class vector_chunk
		{
			public:
				class iterator;
				class const_iterator;
				class reverse_iterator;
				class const_reverse_iterator;
			
			private:
				class raw_iterator;

				struct capacity_tag {};
				struct size_tag {};
				struct vector_chunk_ptr_deleter
				{
					private:
						Allocator& m_alloc_ref;
						std::size_t& m_size;
						std::size_t& m_capacity;

					public:
						constexpr vector_chunk_ptr_deleter(vector_chunk_ptr_deleter&& other) noexcept : m_alloc_ref(other.m_alloc_ref), m_size(other.m_size), m_capacity(other.m_capacity)  {}
						vector_chunk_ptr_deleter& operator=(vector_chunk_ptr_deleter&& other)
						{
							m_alloc_ref = other.m_alloc_ref;
							m_size = other.m_size;
							m_capacity = other.m_capacity;

							return *this;
						}
						constexpr vector_chunk_ptr_deleter(Allocator& alloc_ref, std::size_t& size, std::size_t& capacity) noexcept : m_alloc_ref{alloc_ref}, m_size{size}, m_capacity{capacity}  {}

						constexpr void operator()(T* ptr) noexcept
						{
							for (std::size_t index = 0; index < m_size; index++)
							{
								std::allocator_traits<Allocator>::destroy(m_alloc_ref, ptr);
							}
							std::allocator_traits<Allocator>::deallocate(m_alloc_ref, ptr, m_capacity);
						}
				};


				using chunk_deleter = vector_chunk_ptr_deleter;
				using m_begin_pointer = std::unique_ptr<T[], chunk_deleter>;


				Allocator m_allocator;

				m_begin_pointer m_begin;
				std::size_t m_size;
				std::size_t m_capacity;

				constexpr chunk_deleter make_default_chunk_deleter() noexcept;

				constexpr void copy_initialize(const vector_chunk<T, Allocator>& other);

				constexpr raw_iterator raw_begin() noexcept;
				constexpr raw_iterator raw_end() noexcept;
				constexpr raw_iterator raw_cap_end() noexcept;
				constexpr raw_iterator raw_before_end() noexcept;



				class raw_iterator
				{
					public:
						using iterator_category = std::bidirectional_iterator_tag;
						using value_type = T;
						using difference_type = long long;
						using pointer = T*;
						using reference = T&;

					private:
						pointer m_ptr;
						
					public:
						constexpr raw_iterator() noexcept;
						constexpr raw_iterator(pointer ptr) noexcept;

						constexpr raw_iterator& operator++() noexcept;
						constexpr raw_iterator operator++(int) noexcept;

						constexpr raw_iterator& operator--() noexcept;
						constexpr raw_iterator operator--(int) noexcept;

						constexpr bool operator==(raw_iterator other) noexcept;
						constexpr bool operator!=(raw_iterator other) noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

				};

			public:
				explicit constexpr vector_chunk();
				explicit constexpr vector_chunk(const Allocator& allocator);
				explicit constexpr vector_chunk(std::size_t n, size_tag, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::size_t n, const T& val, size_tag, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::size_t n, capacity_tag, const Allocator& allocator = Allocator{});
				template <typename It>
				explicit constexpr vector_chunk(It first, It last, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::initializer_list<T> init_list, const Allocator& allocator = Allocator{});
				template <typename Begin, typename Sent>
				explicit constexpr vector_chunk(std::from_range_t, Begin first, Sent last, const Allocator& allocator = Allocator{});
				template <typename Range>
				explicit constexpr vector_chunk(std::from_range_t, Range&& range, const Allocator& allocator = Allocator{});

				constexpr vector_chunk(const vector_chunk<T, Allocator>& other);
				constexpr vector_chunk(const vector_chunk<T, Allocator>& other, const std::type_identity<Allocator>& allocator);
				constexpr vector_chunk(vector_chunk<T, Allocator>&& other);
				constexpr vector_chunk(vector_chunk<T, Allocator>&& other, const std::type_identity<Allocator>& allocator);

				constexpr vector_chunk<T, Allocator>& operator=(vector_chunk<T, Allocator> other);
				constexpr vector_chunk<T, Allocator>& operator=(std::initializer_list<T> init_list);

				template <typename... Args>
				constexpr bool emplace_back(Args&&... args);
				constexpr bool push_back(const T& val);
				constexpr bool push_back(T&& val);
				constexpr bool pop_back();

				constexpr bool resize(std::size_t n);

				constexpr void clear();

				constexpr bool empty() noexcept;
				constexpr bool full() noexcept;
				constexpr bool has_capacity() noexcept;

				constexpr iterator begin();
				constexpr iterator end();
				constexpr iterator cap_end();
				constexpr iterator before_end();

				constexpr const_iterator cbegin() const;
				constexpr const_iterator cend() const;
				constexpr const_iterator ccap_end() const;
				constexpr const_iterator cbefore_end() const;

				constexpr reverse_iterator rbegin();
				constexpr reverse_iterator rend();
				constexpr reverse_iterator rcap_end();
				constexpr reverse_iterator rbefore_end();

				constexpr const_reverse_iterator crbegin() const;
				constexpr const_reverse_iterator crend() const;
				constexpr const_reverse_iterator crcap_end() const;
				constexpr const_reverse_iterator crbefore_end() const;
				
				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>& a, vector_chunk<U, SwapAllocator>& b) noexcept;

				template <typename U, typename SVAllocator, typename SVChunkListAllocator>
				friend class my_adt::stable_vector;



				class iterator : private raw_iterator
				{
					public:
						using iterator_category = std::bidirectional_iterator_tag;
						using value_type = raw_iterator::value_type;
						using difference_type = raw_iterator::difference_type;
						using pointer = raw_iterator::pointer;
						using reference = raw_iterator::reference;
						

						constexpr iterator() noexcept;
						constexpr iterator(raw_iterator it) noexcept;
						constexpr iterator(pointer ptr) noexcept;

						constexpr iterator& operator++() noexcept;
						constexpr iterator operator++(int) noexcept;

						constexpr iterator& operator--() noexcept;
						constexpr iterator operator--(int) noexcept;

						constexpr bool operator==(iterator other) noexcept;
						constexpr bool operator!=(iterator other) noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkListAllocator>
						friend class my_adt::stable_vector;

				};

				class const_iterator : private raw_iterator
				{
					public:
						using iterator_category = std::bidirectional_iterator_tag;
						using value_type = std::add_const_t<typename raw_iterator::value_type>;
						using difference_type = raw_iterator::difference_type;
						using pointer = add_const_to_pointer_t<typename raw_iterator::pointer>;
						using reference = std::add_const_t<typename raw_iterator::reference>;


						constexpr const_iterator() noexcept;
						constexpr const_iterator(raw_iterator it);
						constexpr const_iterator(pointer ptr) noexcept;

						constexpr const_iterator& operator++() noexcept;
						constexpr const_iterator operator++(int) noexcept;

						constexpr const_iterator& operator--() noexcept;
						constexpr const_iterator operator--(int) noexcept;

						constexpr bool operator==(const_iterator other) noexcept;
						constexpr bool operator!=(const_iterator other) noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkListAllocator>
						friend class my_adt::stable_vector;

				};

				class reverse_iterator : private raw_iterator
				{
					public:
						using iterator_category = std::bidirectional_iterator_tag;
						using value_type = raw_iterator::value_type;
						using difference_type = raw_iterator::difference_type;
						using pointer = raw_iterator::pointer;
						using reference = raw_iterator::reference;


						constexpr reverse_iterator() noexcept;
						constexpr reverse_iterator(raw_iterator it);
						constexpr reverse_iterator(pointer ptr) noexcept;

						constexpr reverse_iterator& operator++() noexcept;
						constexpr reverse_iterator operator++(int) noexcept;

						constexpr reverse_iterator& operator--() noexcept;
						constexpr reverse_iterator operator--(int) noexcept;

						constexpr bool operator==(reverse_iterator other) noexcept;
						constexpr bool operator!=(reverse_iterator other) noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkListAllocator>
						friend class my_adt::stable_vector;

				};

				class const_reverse_iterator : private raw_iterator
				{
					public:
						using iterator_category = std::bidirectional_iterator_tag;
						using value_type = std::add_const_t<typename raw_iterator::value_type>;
						using difference_type = raw_iterator::difference_type;
						using pointer = add_const_to_pointer_t<typename raw_iterator::pointer>;
						using reference = std::add_const_t<typename raw_iterator::reference>;


						constexpr const_reverse_iterator() noexcept;
						constexpr const_reverse_iterator(raw_iterator it);
						constexpr const_reverse_iterator(pointer ptr) noexcept;

						constexpr const_reverse_iterator& operator++() noexcept;
						constexpr const_reverse_iterator operator++(int) noexcept;

						constexpr const_reverse_iterator& operator--() noexcept;
						constexpr const_reverse_iterator operator--(int) noexcept;

						constexpr bool operator==(const_reverse_iterator other) noexcept;
						constexpr bool operator!=(const_reverse_iterator other) noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkListAllocator>
						friend class my_adt::stable_vector;

				};
		};
	}







	template <typename T, typename Allocator = std::allocator<T>, typename ChunkListAllocator = std::allocator<detail::vector_chunk<T, Allocator>>>
	class stable_vector
	{
		public:
			class iterator;
			class const_iterator;
			class reverse_iterator;
			class const_reverse_iterator;

		private:
			class raw_iterator;
			struct uninit_tag {};

			using chunk = detail::vector_chunk<T, Allocator>;
			using list = std::list<chunk, ChunkListAllocator>;
			using list_iterator = list::iterator;


			const Allocator m_allocator;
			list m_chunks;
			std::size_t m_size;
			std::size_t m_capacity;
			iterator m_end;

			explicit constexpr stable_vector(uninit_tag);
			explicit constexpr stable_vector(uninit_tag, const Allocator& allocator);
			explicit constexpr stable_vector(uninit_tag, const Allocator& allocator, const ChunkListAllocator& chunk_allocator);

			constexpr void copy_initialize(const stable_vector<T, Allocator, ChunkListAllocator>& other);

			constexpr bool chunks_full() noexcept;
			constexpr bool current_chunk_empty() noexcept;
			constexpr bool current_chunk_full() noexcept;
			constexpr bool end_at_chunk_start() noexcept;
			constexpr bool current_chunk_has_capacity() noexcept;

			constexpr void push_chunk(std::size_t size);
			constexpr void push_empty_chunk();

			constexpr bool has_capacity() noexcept;
			constexpr bool one_away_from_full() noexcept;

			constexpr raw_iterator raw_begin() noexcept;
			constexpr raw_iterator raw_end() noexcept;
			constexpr raw_iterator raw_before_end() noexcept;



			class raw_iterator
			{
				public:
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = T;
					using difference_type = long long;
					using pointer = T*;
					using reference = T&;

				private:
					using list_iterator_type = list_iterator;
					using chunk_iterator_type = chunk::iterator;


					list_iterator_type m_list_iterator;
					chunk_iterator_type m_chunk_iterator;


					constexpr bool at_chunk_start() noexcept;
					
					constexpr void go_to_previous_chunk_end() noexcept;

					constexpr void increment() noexcept;
					constexpr void decrement() noexcept;

					constexpr list_iterator_type& get_list_iterator() noexcept;
					constexpr chunk_iterator_type& get_chunk_iterator() noexcept;
					
				public:
					constexpr raw_iterator() noexcept;
					constexpr raw_iterator(list_iterator_type list_iterator, chunk_iterator_type ptr) noexcept;
					
					constexpr raw_iterator& operator++() noexcept;
					constexpr raw_iterator operator++(int) noexcept;

					constexpr raw_iterator& operator--() noexcept;
					constexpr raw_iterator operator--(int) noexcept;

					constexpr bool operator==(raw_iterator other) noexcept;
					constexpr bool operator!=(raw_iterator other) noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkListAllocator>;
			};

		public:
			explicit constexpr stable_vector();
			explicit constexpr stable_vector(const Allocator& allocator);
			explicit constexpr stable_vector(const Allocator& allocator, const ChunkListAllocator& chunk_allocator);
			explicit constexpr stable_vector(std::size_t n, const Allocator& allocator = Allocator{}, const ChunkListAllocator& chunk_allocator = ChunkListAllocator{});
			explicit constexpr stable_vector(std::size_t n, const T& val, const Allocator& allocator = Allocator{}, const ChunkListAllocator& chunk_allocator = ChunkListAllocator{});
			template <typename It>
			explicit constexpr stable_vector(It first, It last, const Allocator& allocator = Allocator{}, const ChunkListAllocator& chunk_allocator = ChunkListAllocator{});
			explicit constexpr stable_vector(std::initializer_list<T> init_list, const Allocator& allocator, const ChunkListAllocator& chunk_allocator);
			template <typename Begin, typename Sent>
			explicit constexpr stable_vector(std::from_range_t, Begin first, Sent last, const Allocator& allocator = Allocator{}, const ChunkListAllocator& chunk_allocator = ChunkListAllocator{});
			template <typename Range>
			explicit constexpr stable_vector(std::from_range_t, Range&& range, const Allocator& allocator = Allocator{}, const ChunkListAllocator& chunk_allocator = ChunkListAllocator{});

			constexpr stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other);
			constexpr stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other, const std::type_identity<Allocator>& allocator);
			constexpr stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other, const std::type_identity<Allocator>& allocator, const std::type_identity<ChunkListAllocator>& chunk_allocator);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other, const std::type_identity<Allocator>& allocator);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other, const std::type_identity<Allocator>& allocator, const std::type_identity<ChunkListAllocator>& chunk_allocator);

			constexpr stable_vector<T, Allocator, ChunkListAllocator>& operator=(stable_vector<T, Allocator, ChunkListAllocator> other);
			constexpr stable_vector<T, Allocator, ChunkListAllocator>& operator=(std::initializer_list<T> init_list);

			void assign(std::size_t n, const T& val);
			template <typename It>
			void assign(It first, It last);
			void assign(std::initializer_list<T> init_list);

			template <typename Range>
			void assign_range(Range&& range);
			

			template <typename... Args>
			constexpr void emplace_back(Args&&... args);
			constexpr void push_back(const T& other);
			constexpr void push_back(T&& other);
			constexpr void pop_back();

			constexpr T& front();
			constexpr T& back();

			constexpr void reserve_extra(std::size_t n);
			constexpr void clear();

			constexpr bool empty() noexcept;

			constexpr std::size_t size() noexcept;

			constexpr iterator begin() noexcept;
			constexpr iterator end() noexcept;
			constexpr iterator before_end() noexcept;

			constexpr iterator cbegin() noexcept;
			constexpr iterator cend() noexcept;
			constexpr iterator cbefore_end() noexcept;

			constexpr iterator rbegin() noexcept;
			constexpr iterator rend() noexcept;
			constexpr iterator rbefore_end() noexcept;

			constexpr iterator crbegin() noexcept;
			constexpr iterator crend() noexcept;
			constexpr iterator crbefore_end() noexcept;


			template <typename U, typename SwapAllocator, typename SwapChunkListAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkListAllocator>& a, stable_vector<U, SwapAllocator, SwapChunkListAllocator>& b) noexcept;


			class iterator : private raw_iterator
			{						
				public:
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = raw_iterator::value_type;
					using difference_type = raw_iterator::difference_type;
					using pointer = raw_iterator::pointer;
					using reference = raw_iterator::reference;

					constexpr iterator() noexcept;
					constexpr iterator(raw_iterator it);
					constexpr iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type ptr) noexcept;

					constexpr iterator& operator++() noexcept;
					constexpr iterator operator++(int) noexcept;

					constexpr iterator& operator--() noexcept;
					constexpr iterator operator--(int) noexcept;

					constexpr bool operator==(iterator other) noexcept;
					constexpr bool operator!=(iterator other) noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkListAllocator>;
			};

			class const_iterator : private raw_iterator
			{						
				public:
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = std::add_const_t<typename raw_iterator::value_type>;
					using difference_type = raw_iterator::difference_type;
					using pointer = add_const_to_pointer_t<typename raw_iterator::pointer>;
					using reference = std::add_const_t<typename raw_iterator::reference>;

					constexpr const_iterator() noexcept;
					constexpr const_iterator(raw_iterator it);
					constexpr const_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type ptr) noexcept;

					constexpr const_iterator& operator++() noexcept;
					constexpr const_iterator operator++(int) noexcept;

					constexpr const_iterator& operator--() noexcept;
					constexpr const_iterator operator--(int) noexcept;

					constexpr bool operator==(const_iterator other) noexcept;
					constexpr bool operator!=(const_iterator other) noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkListAllocator>;
			};

			class reverse_iterator : private raw_iterator
			{						
				public:
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = raw_iterator::value_type;
					using difference_type = raw_iterator::difference_type;
					using pointer = raw_iterator::pointer;
					using reference = raw_iterator::reference;

					constexpr reverse_iterator() noexcept;
					constexpr reverse_iterator(raw_iterator it);
					constexpr reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type ptr) noexcept;

					constexpr reverse_iterator& operator++() noexcept;
					constexpr reverse_iterator operator++(int) noexcept;

					constexpr reverse_iterator& operator--() noexcept;
					constexpr reverse_iterator operator--(int) noexcept;

					constexpr bool operator==(reverse_iterator other) noexcept;
					constexpr bool operator!=(reverse_iterator other) noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkListAllocator>;
			};

			class const_reverse_iterator : private raw_iterator
			{						
				public:
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = std::add_const_t<typename raw_iterator::value_type>;
					using difference_type = raw_iterator::difference_type;
					using pointer = add_const_to_pointer_t<typename raw_iterator::pointer>;
					using reference = std::add_const_t<typename raw_iterator::reference>;

					constexpr const_reverse_iterator() noexcept;
					constexpr const_reverse_iterator(raw_iterator it);
					constexpr const_reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type ptr) noexcept;

					constexpr const_reverse_iterator& operator++() noexcept;
					constexpr const_reverse_iterator operator++(int) noexcept;

					constexpr const_reverse_iterator& operator--() noexcept;
					constexpr const_reverse_iterator operator--(int) noexcept;

					constexpr bool operator==(const_reverse_iterator other) noexcept;
					constexpr bool operator!=(const_reverse_iterator other) noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkListAllocator>;
			};
	};




	namespace detail
	{
		template <typename T, typename Allocator>
		constexpr void swap(vector_chunk<T, Allocator>& a, vector_chunk<T, Allocator>& b) noexcept
		{
			std::swap(a.m_begin, b.m_begin);
			std::swap(a.m_size, b.m_size);
			std::swap(a.m_capacity, b.m_capacity);
			swap(a.m_allocator, b.m_allocator);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::chunk_deleter detail::vector_chunk<T, Allocator>::make_default_chunk_deleter() noexcept
		{
			return chunk_deleter{m_allocator, m_size, m_capacity};
		}

		template <typename T, typename Allocator>
		constexpr void detail::vector_chunk<T, Allocator>::copy_initialize(const vector_chunk<T, Allocator>& other)
		{
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, other.m_size), chunk_deleter{m_allocator, other.m_size, other.m_capacity});

			std::uninitialized_copy(other.begin(), other.end(), new_begin);

			m_begin = std::move(new_begin);
			m_begin.get_deleter() = make_default_chunk_deleter();

			m_capacity = other.m_size;
			m_size = other.m_size;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_begin() noexcept
		{
			return raw_iterator{m_begin.get()};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_end() noexcept
		{
			if (empty()) return raw_begin();
			else return raw_iterator{m_begin.get() + m_size};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_cap_end() noexcept
		{
			return raw_iterator{m_begin.get() + m_capacity};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_before_end() noexcept
		{
			if (empty()) return raw_begin();
			else return raw_iterator{m_begin.get() + m_size - 1};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk() : m_allocator{}, m_begin{nullptr, chunk_deleter{m_allocator, m_size, m_capacity}}, m_size{0}, m_capacity{0} {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const Allocator& other) : m_allocator{other}, m_begin{nullptr, chunk_deleter{m_allocator, m_size, m_capacity}}, m_size{0}, m_capacity{0} {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::size_t n, size_tag, const Allocator& allocator) : vector_chunk{allocator}
		{
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, n), chunk_deleter{m_allocator, n, n});
			std::uninitialized_default_construct_n(new_begin, n);

			m_begin = std::move(new_begin);
			m_begin.get_deleter() = make_default_chunk_deleter();

			m_capacity = n;
			m_size = n;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::size_t n, capacity_tag, const Allocator& allocator) : vector_chunk{allocator}
		{
			if (n > 0)
			{
				m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, n), chunk_deleter{m_allocator, m_size, m_capacity});

				m_begin = std::move(new_begin);
				m_capacity = n;
			}
		}

		template <typename T, typename Allocator>
		template <typename It>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(It first, It last, const Allocator& allocator) : vector_chunk{allocator}
		{
			std::size_t size = std::distance(first, last);
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, size), chunk_deleter{m_allocator, size, size});

			std::uninitialized_copy(first, last, m_begin.get());

			m_begin = std::move(new_begin);
			m_begin.get_deleter() = make_default_chunk_deleter();

			m_capacity = size;
			m_size = size;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::initializer_list<T> init_list, const Allocator& allocator) : vector_chunk{init_list.begin(), init_list.end(), allocator}  {}

		template <typename T, typename Allocator>
		template <typename Begin, typename Sent>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::from_range_t, Begin first, Sent last, const Allocator& allocator) : vector_chunk{allocator}
		{
			std::size_t size = std::ranges::distance(first, last);
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, size), chunk_deleter{m_allocator, size, size});

			std::ranges::uninitialized_copy(first, last, new_begin.get(), new_begin.get() + size);

			m_begin = std::move(new_begin);
			m_begin.get_deleter() = make_default_chunk_deleter();

			m_capacity = size;
			m_size = size;
		}

		template <typename T, typename Allocator>
		template <typename Range>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::from_range_t, Range&& range, const Allocator& allocator) : vector_chunk{range.begin(), range.end(), allocator}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const vector_chunk<T, Allocator>& other) : vector_chunk{Allocator{}}
		{
			copy_initialize(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const vector_chunk<T, Allocator>& other, const std::type_identity<Allocator>& allocator) : vector_chunk{allocator}
		{
			copy_initialize(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(detail::vector_chunk<T, Allocator>&& other) : m_allocator(Allocator{}), m_begin(std::move(other.begin())),
																												m_size(other.m_size), m_capacity(other.m_capacity)
		{
			other.m_size = 0;
			other.m_capacity = 0;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(detail::vector_chunk<T, Allocator>&& other,
																const std::type_identity<Allocator>& allocator) : m_allocator(allocator), m_begin(std::move(other.begin())),
																													m_size(other.m_size), m_capacity(other.m_capacity)
		{
			other.m_size = 0;
			other.m_capacity = 0;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>& detail::vector_chunk<T, Allocator>::operator=(vector_chunk<T, Allocator> other)
		{
			swap(*this, other);
			return *this;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>& detail::vector_chunk<T, Allocator>::operator=(std::initializer_list<T> init_list)
		{
			static_assert(false);
		}

		template <typename T, typename Allocator>
		template <typename... Args>
		constexpr bool detail::vector_chunk<T, Allocator>::emplace_back(Args&&... args)
		{
			if (full())
			{
				return false;
			}
			else
			{
				std::allocator_traits<Allocator>::construct(m_allocator, m_begin.get() + m_size, std::forward<Args>(args)...);
				m_size++;

				return true;
			}
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::push_back(const T& val)
		{
			return emplace_back(val);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::push_back(T&& val)
		{
			return emplace_back(std::move(val));
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::pop_back()
		{
			if (m_size == 0)
			{
				return false;
			}
			else
			{
				std::allocator_traits<Allocator>::destroy(m_allocator, m_begin.get() + m_size - 1);
				m_size--;
				
				return true;
			}
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::resize(std::size_t n)
		{
			if (n > m_capacity)
			{
				return false;
			}
			else if (n < m_size)
			{
				for (std::size_t index = n; index < m_size; index++)
				{
					std::allocator_traits<Allocator>::destroy(m_allocator, m_begin.get() + index);
				}
			}
			else if (n > m_size)
			{
				for (std::size_t index = m_size; index < n; index++)
				{
					std::allocator_traits<Allocator>::construct(m_allocator, m_begin.get() + index);
				}
			}
			return true;
		}

		template <typename T, typename Allocator>
		constexpr void detail::vector_chunk<T, Allocator>::clear()
		{
			for (std::size_t index = 0; index < m_size; index++)
			{
				std::allocator_traits<Allocator>::destroy(m_allocator, m_begin.get() + index);
			}
			m_size = 0;
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::empty() noexcept
		{
			return m_size == 0;
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::full() noexcept
		{
			return m_size == m_capacity;
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::has_capacity() noexcept
		{
			return m_capacity != 0;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::begin()
		{
			return iterator{m_begin.get()};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::end()
		{
			return iterator{m_begin.get() + m_size};
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::cap_end()
		{
			return iterator{m_begin.get() + m_capacity};
		}





		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::raw_iterator() noexcept : m_ptr{nullptr}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::raw_iterator(pointer ptr) noexcept : m_ptr{ptr}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator& detail::vector_chunk<T, Allocator>::raw_iterator::operator++() noexcept
		{
			m_ptr++;
			return *this;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_iterator::operator++(int) noexcept
		{
			return raw_iterator(m_ptr++);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator& detail::vector_chunk<T, Allocator>::raw_iterator::operator--() noexcept
		{
			m_ptr--;
			return *this;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_iterator::operator--(int) noexcept
		{
			return raw_iterator(m_ptr--);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(raw_iterator other) noexcept
		{
			return m_ptr == other.m_ptr;
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator!=(raw_iterator other) noexcept
		{
			return !operator==(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::reference detail::vector_chunk<T, Allocator>::raw_iterator::operator*() noexcept
		{
			return *m_ptr;
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::pointer detail::vector_chunk<T, Allocator>::raw_iterator::operator->() noexcept
		{
			return m_ptr;
		}












		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		constexpr vector_chunk<T, Allocator>::iterator::iterator(raw_iterator it) noexcept : raw_iterator{it}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator& detail::vector_chunk<T, Allocator>::iterator::operator++() noexcept
		{
			return static_cast<iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator& detail::vector_chunk<T, Allocator>::iterator::operator--() noexcept
		{
			return static_cast<iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator==(iterator other) noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator!=(iterator other) noexcept
		{
			return raw_iterator::operator!=(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::reference detail::vector_chunk<T, Allocator>::iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::pointer detail::vector_chunk<T, Allocator>::iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}











		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::const_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::const_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator& detail::vector_chunk<T, Allocator>::const_iterator::operator++() noexcept
		{
			return static_cast<const_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator detail::vector_chunk<T, Allocator>::const_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator& detail::vector_chunk<T, Allocator>::const_iterator::operator--() noexcept
		{
			return static_cast<const_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator detail::vector_chunk<T, Allocator>::const_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator==(const_iterator other) noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator!=(const_iterator other) noexcept
		{
			return raw_iterator::operator!=(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::reference detail::vector_chunk<T, Allocator>::const_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::pointer detail::vector_chunk<T, Allocator>::const_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}








		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reverse_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reverse_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator& detail::vector_chunk<T, Allocator>::reverse_iterator::operator++() noexcept
		{
			return static_cast<reverse_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator detail::vector_chunk<T, Allocator>::reverse_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator& detail::vector_chunk<T, Allocator>::reverse_iterator::operator--() noexcept
		{
			return static_cast<reverse_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator detail::vector_chunk<T, Allocator>::reverse_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator==(reverse_iterator other) noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator!=(reverse_iterator other) noexcept
		{
			return raw_iterator::operator!=(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reference detail::vector_chunk<T, Allocator>::reverse_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::pointer detail::vector_chunk<T, Allocator>::reverse_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}











		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::const_reverse_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::const_reverse_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator& detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator++() noexcept
		{
			return static_cast<const_reverse_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator& detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator--() noexcept
		{
			return static_cast<const_reverse_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator==(const_reverse_iterator other) noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator!=(const_reverse_iterator other) noexcept
		{
			return raw_iterator::operator!=(other);
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::reference detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::pointer detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}
	}






	

	




	template <typename T, typename Allocator, typename ChunkListAllocator> 
	constexpr void swap(stable_vector<T, Allocator, ChunkListAllocator>& a, stable_vector<T, Allocator, ChunkListAllocator>& b) noexcept
	{
		std::swap(a.m_chunks, b.m_chunks);
		std::swap(a.m_size, b.m_size);
		std::swap(a.m_capacity, b.m_capacity);
		swap(a.m_end, b.m_end);
		static_assert(false, "wth is this, fix ASAP");
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(uninit_tag) : m_allocator{}, m_chunks{}, m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(uninit_tag, const Allocator& allocator) : m_allocator{allocator}, m_chunks{ChunkListAllocator{}}, m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(uninit_tag, const Allocator& allocator,
																			 const ChunkListAllocator& chunk_allocator) : m_allocator{allocator}, m_chunks{chunk_allocator}, m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::copy_initialize(const stable_vector<T, Allocator, ChunkListAllocator>& other)
	{
		chunk new_chunk{other.size(), typename chunk::capacity_tag{}};
		std::uninitialized_copy(other.begin(), other.end(), new_chunk.begin());
		m_chunks.emplace_back(std::move(new_chunk));
		try { push_empty_chunk(); }
		catch (const std::exception& e)
		{
			m_chunks.pop_back();
			throw;
		}

		m_size = other.size();
		m_capacity = other.size();
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}


	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::chunks_full() noexcept
	{
		return m_chunks.back().full();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::current_chunk_empty() noexcept
	{
		chunk& last_chunk = *(m_end.get_list_iterator());
		return last_chunk.empty();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::current_chunk_full() noexcept
	{
		chunk& last_chunk = *(m_end.get_list_iterator());
		return last_chunk.full();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::end_at_chunk_start() noexcept
	{
		chunk& current_chunk = *(m_end.get_list_iterator());
		return current_chunk.begin() == m_end.get_chunk_iterator();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::current_chunk_has_capacity() noexcept
	{
		chunk& current_chunk = *(m_end.get_list_iterator());
		return current_chunk.has_capacity();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::push_chunk(std::size_t n)
	{
		m_chunks.emplace_back(n, typename chunk::capacity_tag{});

		m_capacity += n;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::push_empty_chunk()
	{
		m_chunks.emplace_back();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::has_capacity() noexcept
	{
		return m_capacity != 0;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::one_away_from_full() noexcept
	{
		return m_capacity == (m_size + 1);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector() : stable_vector{uninit_tag{}}
	{
		push_empty_chunk();
		m_end = begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(const Allocator& allocator) : stable_vector{uninit_tag{}, allocator}
	{
		push_empty_chunk();
		m_end = begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(const Allocator& allocator, const ChunkListAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		push_empty_chunk();
		m_end = begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(std::size_t n, const Allocator& allocator, const ChunkListAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		m_chunks.emplace_back(n, typename chunk::size_tag{});
		try { push_empty_chunk(); }
		catch (std::exception& e)
		{
			m_chunks.pop_back();
			throw;
		}

		m_size = n;
		m_capacity = n;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	template <typename It>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(It first, It last, const Allocator& allocator, const ChunkListAllocator& chunk_allocator) : stable_vector{uninit_tag{}}
	{
		std::size_t size = std::distance(first, last);
		m_chunks.emplace_back(first, last);
		try { push_empty_chunk(); }
		catch (const std::exception& e)
		{
			m_chunks.pop_back();
			throw;
		}

		m_size = size;
		m_capacity = size;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(std::initializer_list<T> init_list, const Allocator& allocator,
																			 const ChunkListAllocator& chunk_allocator) : stable_vector{init_list.begin(), init_list.end(), allocator, chunk_allocator}  {}
	
	template <typename T, typename Allocator, typename ChunkListAllocator>
	template <typename Begin, typename Sent>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(std::from_range_t, Begin first, Sent last,
																			 const Allocator& allocator, const ChunkListAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		std::size_t size = std::ranges::distance(first, last) + 1;
		m_chunks.emplace_back(std::from_range_t{}, first, last);

		try { push_empty_chunk(); }
		catch (const std::exception& e)
		{
			m_chunks.pop_back();
			throw;
		}

		m_size = size;
		m_capacity = size;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	template <typename Range>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(std::from_range_t, Range&& range, const Allocator& allocator,
																			 const ChunkListAllocator& chunk_allocator) : stable_vector{std::from_range_t{}, std::ranges::begin(range),
																			 														   std::ranges::end(range), allocator, chunk_allocator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other) : stable_vector{uninit_tag{}}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other, const std::type_identity<Allocator>& allocator) : stable_vector{uninit_tag{}, allocator}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkListAllocator>& other, const std::type_identity<Allocator>& allocator,
																			 const std::type_identity<ChunkListAllocator>& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other) : m_allocator{}, m_chunks{std::move(other.m_chunks), ChunkListAllocator{}}, m_size{other.m_size},
																																		m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other,
																			 const std::type_identity<Allocator>& allocator) : m_allocator{allocator}, m_chunks{std::move(other.m_chunks), ChunkListAllocator{}}, m_size{other.m_size},
																															   m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::stable_vector(stable_vector<T, Allocator, ChunkListAllocator>&& other,
																			 const std::type_identity<Allocator>& allocator,
																			 const std::type_identity<ChunkListAllocator>& chunk_allocator) : m_allocator{allocator}, m_chunks{std::move(other.m_chunks), chunk_allocator}, m_size{other.m_size},
																																			  m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>& stable_vector<T, Allocator, ChunkListAllocator>::operator=(stable_vector<T, Allocator, ChunkListAllocator> other)
	{
		swap(*this, other);
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	template <typename... Args>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::emplace_back(Args&&... args)
	{
		T new_elem = T(std::forward<Args>(args)...);

		chunk& last_chunk = *(m_end.get_list_iterator());

		if (!has_capacity())
		{
			// transform 0-capacity chunk into chunk with capacity
			last_chunk = chunk{1, typename chunk::capacity_tag{}};
			m_capacity++;

			m_end.get_chunk_iterator() = last_chunk.begin(); // recalibrate m_end
		}
		else if (!current_chunk_has_capacity())
		{
			// transform 0-capacity chunk into chunk with capacity
			last_chunk = chunk{m_size, typename chunk::capacity_tag{}};
			m_capacity += m_size;

			m_end.get_chunk_iterator() = last_chunk.begin(); // recalibrate m_end
		}

		last_chunk.push_back(std::move(new_elem));

		if (current_chunk_full())
		{
			try { push_empty_chunk(); }
			catch (const std::exception& e)
			{
				last_chunk.pop_back();
				throw;
			}
		}

		m_end++;
		m_size++;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::push_back(const T& val)
	{
		emplace_back(val);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::push_back(T&& val)
	{
		emplace_back(std::move(val));
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::pop_back()
	{
		iterator prev = before_end();
		chunk& current_chunk = *(prev.get_list_iterator());
		current_chunk.pop_back();

		m_end--;
		m_size--;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::reserve_extra(std::size_t n)
	{
		static_assert(false);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::clear()
	{
		m_chunks.clear();
		
		m_size = 0;
		m_end = begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::empty() noexcept
	{
		return m_size == 0;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr std::size_t stable_vector<T, Allocator, ChunkListAllocator>::size() noexcept
	{
		return m_size;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::begin() noexcept
	{
		return raw_begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::end() noexcept
	{
		return raw_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::before_end() noexcept
	{
		return raw_before_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::cbegin() noexcept
	{
		return raw_begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::cend() noexcept
	{
		return raw_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::cbefore_end() noexcept
	{
		return raw_before_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::rbegin() noexcept
	{
		return raw_begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::rend() noexcept
	{
		return raw_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::rbefore_end() noexcept
	{
		return raw_before_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::crbegin() noexcept
	{
		return raw_begin();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::crend() noexcept
	{
		return raw_end();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::crbefore_end() noexcept
	{
		return raw_before_end();
	}






	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::increment() noexcept
	{
		chunk& current_chunk = *m_list_iterator;
		chunk_iterator_type next_chunk_iterator = std::next(m_chunk_iterator);

		if (next_chunk_iterator == current_chunk.cap_end())
		{
			chunk& next_chunk = *(++m_list_iterator);
			m_chunk_iterator = next_chunk.begin();
		}
		else
		{
			m_chunk_iterator++;
		}
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr void stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::decrement() noexcept
	{
		chunk& current_chunk = *m_list_iterator;

		if (m_chunk_iterator == current_chunk.begin())
		{
			chunk& prev_chunk = *(--m_list_iterator);
			m_chunk_iterator = std::prev(prev_chunk.cap_end());
		}
		else
		{
			m_chunk_iterator--;
		}
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::list_iterator_type& stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::get_list_iterator() noexcept
	{
		return m_list_iterator;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::chunk_iterator_type& stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::get_chunk_iterator() noexcept
	{
		return m_chunk_iterator;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator stable_vector<T, Allocator, ChunkListAllocator>::raw_begin() noexcept
	{
		return raw_iterator{m_chunks.begin(), m_chunks.front().begin()};
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator stable_vector<T, Allocator, ChunkListAllocator>::raw_end() noexcept
	{
		if (!has_capacity())
		{
			return raw_begin();
		}
		else
		{
			return m_end;
		}
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator stable_vector<T, Allocator, ChunkListAllocator>::raw_before_end() noexcept
	{
		if (!has_capacity())
		{
			return raw_begin();
		}
		else
		{
			return std::prev(raw_end());
		}
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::raw_iterator() noexcept : m_list_iterator{}, m_chunk_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::raw_iterator(list_iterator_type list_iterator,
																						  chunk_iterator_type chunk_iterator)  noexcept : m_list_iterator{list_iterator},
																						  												  m_chunk_iterator{chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator& stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator++() noexcept
	{
		increment();
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator++(int) noexcept
	{
		stable_vector<T, Allocator, ChunkListAllocator>::iterator prev_it = *this;
		increment();
		return prev_it;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator& stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator--() noexcept
	{
		decrement();
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator--(int) noexcept
	{
		stable_vector<T, Allocator, ChunkListAllocator>::iterator prev_it = *this;
		decrement();
		return prev_it;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator==(raw_iterator other) noexcept
	{
		return (m_list_iterator == other.m_list_iterator)  &&  (m_chunk_iterator == other.m_chunk_iterator);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator!=(raw_iterator other) noexcept
	{
		return !operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::reference stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator*() noexcept
	{
		return *m_chunk_iterator;
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::pointer stable_vector<T, Allocator, ChunkListAllocator>::raw_iterator::operator->() noexcept
	{
		return &*m_chunk_iterator;
	}





	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator::iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator::iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator::iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator& stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator++() noexcept
	{
		return static_cast<iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator++(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator& stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator--() noexcept
	{
		return static_cast<iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator--(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator==(iterator other) noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator!=(iterator other) noexcept
	{
		return raw_iterator::operator!=(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator::reference stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::iterator::pointer stable_vector<T, Allocator, ChunkListAllocator>::iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}









	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::const_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::const_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::const_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator& stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator++() noexcept
	{
		return static_cast<const_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator& stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator--() noexcept
	{
		return static_cast<const_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator==(const_iterator other) noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator!=(const_iterator other) noexcept
	{
		return raw_iterator::operator!=(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::reference stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::pointer stable_vector<T, Allocator, ChunkListAllocator>::const_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}












	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::reverse_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::reverse_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator& stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator++() noexcept
	{
		return static_cast<reverse_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator& stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator--() noexcept
	{
		return static_cast<reverse_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator==(reverse_iterator other) noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator!=(reverse_iterator other) noexcept
	{
		return raw_iterator::operator!=(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::reference stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::pointer stable_vector<T, Allocator, ChunkListAllocator>::reverse_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}









	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::const_reverse_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::const_reverse_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::const_reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator& stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator++() noexcept
	{
		return static_cast<const_reverse_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator& stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator--() noexcept
	{
		return static_cast<const_reverse_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator==(const_reverse_iterator other) noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator!=(const_reverse_iterator other) noexcept
	{
		return raw_iterator::operator!=(other);
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::reference stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkListAllocator>
	constexpr stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::pointer stable_vector<T, Allocator, ChunkListAllocator>::const_reverse_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}
}