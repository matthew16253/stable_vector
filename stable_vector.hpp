#pragma once

#include <cassert>
#include <concepts>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <list>
#include <algorithm>
#include <ranges>
#include <scoped_allocator>
#include <type_traits>





namespace my_adt
{
	template <typename T>
	concept pointer_type = std::is_pointer_v<T>;

	template <pointer_type T>
	struct add_const_to_pointer
	{
		using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>;
	};
	template <pointer_type T>
	using add_const_to_pointer_t = add_const_to_pointer<T>::type;







	template <typename Allocator, typename T>
	concept is_allocator_for = std::same_as<typename Allocator::value_type, T> && requires(Allocator alloc, T* ptr)
	{
		{ alloc.allocate(0) } -> std::convertible_to<T*>;
		{ alloc.deallocate(ptr, 0) };
	};

	template <typename T, typename Allocator>
	concept valid_vector_chunk_template_args = is_allocator_for<Allocator, T>;



	namespace detail
	{
		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		class vector_chunk;
	}


	template <typename T, typename Allocator, typename ChunkAllocator>
	concept valid_stable_vector_template_args = is_allocator_for<Allocator, T> && is_allocator_for<ChunkAllocator, detail::vector_chunk<T, Allocator>>;

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	class stable_vector;

	
	namespace detail
	{
		template <typename T, typename Allocator = std::allocator<T>>
		requires valid_vector_chunk_template_args<T, Allocator>
		class vector_chunk
		{
			public:
				class iterator;
				class const_iterator;
				class reverse_iterator;
				class const_reverse_iterator;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>::raw_iterator& a, vector_chunk<U, SwapAllocator>::raw_iterator& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>::iterator& a, vector_chunk<U, SwapAllocator>::iterator& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>::const_iterator& a, vector_chunk<U, SwapAllocator>::const_iterator& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>::reverse_iterator& a, vector_chunk<U, SwapAllocator>::reverse_iterator& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>::const_reverse_iterator& a, vector_chunk<U, SwapAllocator>::const_reverse_iterator& b) noexcept;
			
			private:
				class raw_iterator;

				struct capacity_tag {};
				struct size_tag {};
				struct vector_chunk_ptr_deleter
				{
					private:
						Allocator& m_alloc_ref;
						std::size_t m_size;
						std::size_t m_capacity;

					public:
						constexpr vector_chunk_ptr_deleter(vector_chunk_ptr_deleter&& other) noexcept : m_alloc_ref(other.m_alloc_ref), m_size(other.m_size), m_capacity(other.m_capacity)  {}

						vector_chunk_ptr_deleter& operator=(vector_chunk_ptr_deleter&& other)
						{
							m_alloc_ref = other.m_alloc_ref;
							m_size = other.m_size;
							m_capacity = other.m_capacity;

							return *this;
						}

						constexpr vector_chunk_ptr_deleter(Allocator& alloc_ref, std::size_t size, std::size_t capacity) noexcept : m_alloc_ref{alloc_ref}, m_size{size}, m_capacity{capacity}  {}

						constexpr void update_size(std::size_t size) noexcept
						{
							m_size = size;
						}
						constexpr void update_capacity(std::size_t capacity) noexcept
						{
							m_capacity = capacity;
						}

						constexpr void operator()(T* ptr) noexcept
						{
							if (m_capacity != 0)
							{
								for (std::size_t index = 0; index < m_size; index++)
								{
									std::allocator_traits<Allocator>::destroy(m_alloc_ref, ptr);
								}
								std::allocator_traits<Allocator>::deallocate(m_alloc_ref, ptr, m_capacity);
							}
						}
				};


				using chunk_deleter = vector_chunk_ptr_deleter;
				using m_begin_pointer = std::unique_ptr<T[], chunk_deleter>;


				Allocator m_allocator;

				m_begin_pointer m_begin;
				std::size_t m_size;
				std::size_t m_capacity;

				constexpr void update_deleter_size() noexcept;

				constexpr void copy_initialize(const vector_chunk<T, Allocator>& other);

				constexpr raw_iterator raw_begin() noexcept;
				constexpr raw_iterator raw_end() noexcept;
				constexpr raw_iterator raw_cap_end() noexcept;
				constexpr raw_iterator raw_before_end() noexcept;
				constexpr raw_iterator raw_before_begin() noexcept;



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

						constexpr bool operator==(raw_iterator other) const noexcept;
						constexpr bool operator==(iterator other) const noexcept;
						constexpr bool operator==(const_iterator other) const noexcept;
						constexpr bool operator==(reverse_iterator other) const noexcept;
						constexpr bool operator==(const_reverse_iterator other) const noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SwapAllocator>
						friend constexpr void swap(vector_chunk<U, SwapAllocator>::raw_iterator& a, vector_chunk<U, SwapAllocator>::raw_iterator& b) noexcept;

						friend class iterator;
						friend class const_iterator;
						friend class reverse_iterator;
						friend class const_reverse_iterator;

				};

			public:
				explicit constexpr vector_chunk();
				explicit constexpr vector_chunk(const Allocator& allocator);
				explicit constexpr vector_chunk(std::size_t n, size_tag, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::size_t n, const T& val, size_tag, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::size_t n, capacity_tag, const Allocator& allocator = Allocator{});
				template <std::input_iterator It>
				explicit constexpr vector_chunk(It first, It last, const Allocator& allocator = Allocator{});
				explicit constexpr vector_chunk(std::initializer_list<T> init_list, const Allocator& allocator = Allocator{});
				template <std::input_iterator Begin, std::sentinel_for<Begin> Sent>
				explicit constexpr vector_chunk(std::from_range_t, Begin first, Sent last, const Allocator& allocator = Allocator{});
				template <std::ranges::input_range Range>
				explicit constexpr vector_chunk(std::from_range_t, Range&& range, const Allocator& allocator = Allocator{});

				constexpr vector_chunk(const vector_chunk<T, Allocator>& other);
				constexpr vector_chunk(const vector_chunk<T, Allocator>& other, const std::type_identity<Allocator>& allocator);
				constexpr vector_chunk(vector_chunk<T, Allocator>&& other);
				constexpr vector_chunk(vector_chunk<T, Allocator>&& other, const std::type_identity<Allocator>& allocator);

				constexpr vector_chunk<T, Allocator>& operator=(const vector_chunk<T, Allocator>& other);
				constexpr vector_chunk<T, Allocator>& operator=(vector_chunk<T, Allocator>&& other);
				constexpr vector_chunk<T, Allocator>& operator=(std::initializer_list<T> init_list);

				template <typename... Args>
				requires std::constructible_from<T, Args...>
				constexpr bool emplace_back(Args&&... args);
				constexpr bool push_back(const T& val);
				constexpr bool push_back(T&& val);
				constexpr bool pop_back();

				constexpr bool resize(std::size_t n) noexcept;

				constexpr void clear() noexcept;

				constexpr bool empty() const noexcept;
				constexpr bool full() const noexcept;
				constexpr bool has_capacity() const noexcept;

				constexpr iterator begin();
				constexpr iterator end();
				constexpr iterator cap_end();

				constexpr const_iterator cbegin() const;
				constexpr const_iterator cend() const;
				constexpr const_iterator ccap_end() const;

				constexpr reverse_iterator rbegin();
				constexpr reverse_iterator rend();

				constexpr const_reverse_iterator crbegin() const;
				constexpr const_reverse_iterator crend() const;
				
				template <typename U, typename SwapAllocator>
				friend constexpr void swap(vector_chunk<U, SwapAllocator>& a, vector_chunk<U, SwapAllocator>& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap_for_copy_assignment(vector_chunk<U, SwapAllocator>& a, vector_chunk<U, SwapAllocator>& b) noexcept;

				template <typename U, typename SwapAllocator>
				friend constexpr void swap_for_move_assignment(vector_chunk<U, SwapAllocator>& a, vector_chunk<U, SwapAllocator>& b) noexcept;

				template <typename U, typename SVAllocator, typename SVChunkAllocator>
				requires valid_stable_vector_template_args<U, SVAllocator, SVChunkAllocator>
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

						constexpr bool operator==(raw_iterator other) const noexcept;
						constexpr bool operator==(iterator other) const noexcept;
						constexpr bool operator==(const_iterator other) const noexcept;
						constexpr bool operator==(reverse_iterator other) const noexcept;
						constexpr bool operator==(const_reverse_iterator other) const noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkAllocator>
						requires valid_stable_vector_template_args<U, SVAllocator, SVChunkAllocator>
						friend class my_adt::stable_vector;

						template <typename U, typename SwapAllocator>
						friend constexpr void swap(vector_chunk<U, SwapAllocator>::iterator& a, vector_chunk<U, SwapAllocator>::iterator& b) noexcept;

						friend class raw_iterator;
						friend class const_iterator;
						friend class reverse_iterator;
						friend class const_reverse_iterator;

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

						constexpr bool operator==(raw_iterator other) const noexcept;
						constexpr bool operator==(iterator other) const noexcept;
						constexpr bool operator==(const_iterator other) const noexcept;
						constexpr bool operator==(reverse_iterator other) const noexcept;
						constexpr bool operator==(const_reverse_iterator other) const noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkAllocator>
						requires valid_stable_vector_template_args<U, SVAllocator, SVChunkAllocator>
						friend class my_adt::stable_vector;

						template <typename U, typename SwapAllocator>
						friend constexpr void swap(vector_chunk<U, SwapAllocator>::const_iterator& a, vector_chunk<U, SwapAllocator>::const_iterator& b) noexcept;

						friend class raw_iterator;
						friend class iterator;
						friend class reverse_iterator;
						friend class const_reverse_iterator;

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

						constexpr bool operator==(raw_iterator other) const noexcept;
						constexpr bool operator==(iterator other) const noexcept;
						constexpr bool operator==(const_iterator other) const noexcept;
						constexpr bool operator==(reverse_iterator other) const noexcept;
						constexpr bool operator==(const_reverse_iterator other) const noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkAllocator>
						requires valid_stable_vector_template_args<U, SVAllocator, SVChunkAllocator>
						friend class my_adt::stable_vector;

						template <typename U, typename SwapAllocator>
						friend constexpr void swap(vector_chunk<U, SwapAllocator>::reverse_iterator& a, vector_chunk<U, SwapAllocator>::reverse_iterator& b) noexcept;

						friend class raw_iterator;
						friend class iterator;
						friend class const_iterator;
						friend class const_reverse_iterator;

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

						constexpr bool operator==(raw_iterator other) const noexcept;
						constexpr bool operator==(iterator other) const noexcept;
						constexpr bool operator==(const_iterator other) const noexcept;
						constexpr bool operator==(reverse_iterator other) const noexcept;
						constexpr bool operator==(const_reverse_iterator other) const noexcept;

						constexpr reference operator*() noexcept;
						constexpr pointer operator->() noexcept;

						template <typename U, typename SVAllocator, typename SVChunkAllocator>
						requires valid_stable_vector_template_args<U, SVAllocator, SVChunkAllocator>
						friend class my_adt::stable_vector;

						template <typename U, typename SwapAllocator>
						friend constexpr void swap(vector_chunk<U, SwapAllocator>::const_reverse_iterator& a, vector_chunk<U, SwapAllocator>::const_reverse_iterator& b) noexcept;

						friend class raw_iterator;
						friend class iterator;
						friend class const_iterator;
						friend class reverse_iterator;

				};
		};
	}









	template <typename T, typename Allocator = std::allocator<T>, typename ChunkAllocator = std::allocator<detail::vector_chunk<T, Allocator>>>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	class stable_vector
	{
		public:
			class iterator;
			class const_iterator;
			class reverse_iterator;
			class const_reverse_iterator;

			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::raw_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::raw_iterator& b) noexcept;
			
			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::iterator& b) noexcept;

			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_iterator& b) noexcept;

			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::reverse_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::reverse_iterator& b) noexcept;

			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_reverse_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_reverse_iterator& b) noexcept;

		private:
			class raw_iterator;
			struct uninit_tag {};

			using chunk = detail::vector_chunk<T, Allocator>;
			using list = std::list<chunk, ChunkAllocator>;
			using list_iterator = list::iterator;


			const Allocator m_allocator;
			list m_chunks;
			std::size_t m_size;
			std::size_t m_capacity;
			iterator m_end;

			explicit constexpr stable_vector(uninit_tag);
			explicit constexpr stable_vector(uninit_tag, const Allocator& allocator);
			explicit constexpr stable_vector(uninit_tag, const Allocator& allocator, const ChunkAllocator& chunk_allocator);

			constexpr void init_empty_chunks();

			constexpr void copy_initialize(const stable_vector<T, Allocator, ChunkAllocator>& other);

			constexpr bool chunks_full() const noexcept;
			constexpr bool current_chunk_empty() const noexcept;
			constexpr bool current_chunk_full() const noexcept;
			constexpr bool end_at_chunk_start() const noexcept;
			constexpr bool current_chunk_has_capacity() const noexcept;

			constexpr void push_chunk(std::size_t size);
			constexpr void push_empty_chunk();

			constexpr bool has_capacity() const noexcept;
			constexpr bool one_away_from_full() const noexcept;

			constexpr raw_iterator raw_begin() noexcept;
			constexpr raw_iterator raw_end() noexcept;
			constexpr raw_iterator raw_before_end() noexcept;
			constexpr raw_iterator raw_before_begin() noexcept;



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


					constexpr bool at_chunk_start() const noexcept;
					
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

					constexpr bool operator==(raw_iterator other) const noexcept;
					constexpr bool operator==(iterator other) const noexcept;
					constexpr bool operator==(const_iterator other) const noexcept;
					constexpr bool operator==(reverse_iterator other) const noexcept;
					constexpr bool operator==(const_reverse_iterator other) const noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkAllocator>;

					template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
					friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::raw_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::raw_iterator& b) noexcept;

					friend class iterator;
					friend class const_iterator;
					friend class reverse_iterator;
					friend class const_reverse_iterator;
			};

		public:
			explicit constexpr stable_vector();
			explicit constexpr stable_vector(const Allocator& allocator);
			explicit constexpr stable_vector(const Allocator& allocator, const ChunkAllocator& chunk_allocator);
			explicit constexpr stable_vector(std::size_t n, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});
			explicit constexpr stable_vector(std::size_t n, const T& val, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});
			template <std::input_iterator It>
			explicit constexpr stable_vector(It first, It last, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});
			explicit constexpr stable_vector(std::initializer_list<T> init_list, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});
			template <std::input_iterator Begin, std::sentinel_for<Begin> Sent>
			explicit constexpr stable_vector(std::from_range_t, Begin first, Sent last, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});
			template <std::ranges::input_range Range>
			explicit constexpr stable_vector(std::from_range_t, Range&& range, const Allocator& allocator = Allocator{}, const ChunkAllocator& chunk_allocator = ChunkAllocator{});

			constexpr stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other);
			constexpr stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other, const std::type_identity<Allocator>& allocator);
			constexpr stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other, const std::type_identity<Allocator>& allocator, const std::type_identity<ChunkAllocator>& chunk_allocator);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other, const std::type_identity<Allocator>& allocator);
			constexpr stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other, const std::type_identity<Allocator>& allocator, const std::type_identity<ChunkAllocator>& chunk_allocator);

			constexpr stable_vector<T, Allocator, ChunkAllocator>& operator=(const stable_vector<T, Allocator, ChunkAllocator>& other);
			constexpr stable_vector<T, Allocator, ChunkAllocator>& operator=(stable_vector<T, Allocator, ChunkAllocator>&& other);
			constexpr stable_vector<T, Allocator, ChunkAllocator>& operator=(std::initializer_list<T> init_list);

			void assign(std::size_t n, const T& val);
			template <std::input_iterator It>
			void assign(It first, It last);
			void assign(std::initializer_list<T> init_list);

			template <std::ranges::input_range Range>
			void assign_range(Range&& range);
			

			template <typename... Args>
			requires std::constructible_from<T, Args...>
			constexpr void emplace_back(Args&&... args);
			constexpr void push_back(const T& other);
			constexpr void push_back(T&& other);
			constexpr void pop_back();

			constexpr T& front();
			constexpr T& back();

			constexpr void reserve_extra(std::size_t n);
			constexpr void clear();

			constexpr bool empty() const noexcept;

			constexpr std::size_t size() const noexcept;

			constexpr iterator begin() noexcept;
			constexpr iterator end() noexcept;

			constexpr const_iterator cbegin() const noexcept;
			constexpr const_iterator cend() const noexcept;

			constexpr reverse_iterator rbegin() noexcept;
			constexpr reverse_iterator rend() noexcept;

			constexpr const_reverse_iterator crbegin() const noexcept;
			constexpr const_reverse_iterator crend() const noexcept;


			template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
			friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>& b) noexcept;


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

					constexpr bool operator==(raw_iterator other) const noexcept;
					constexpr bool operator==(iterator other) const noexcept;
					constexpr bool operator==(const_iterator other) const noexcept;
					constexpr bool operator==(reverse_iterator other) const noexcept;
					constexpr bool operator==(const_reverse_iterator other) const noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkAllocator>;

					template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
					friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::iterator& b) noexcept;

					friend class raw_iterator;
					friend class const_iterator;
					friend class reverse_iterator;
					friend class const_reverse_iterator;
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

					constexpr bool operator==(raw_iterator other) const noexcept;
					constexpr bool operator==(iterator other) const noexcept;
					constexpr bool operator==(const_iterator other) const noexcept;
					constexpr bool operator==(reverse_iterator other) const noexcept;
					constexpr bool operator==(const_reverse_iterator other) const noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkAllocator>;

					template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
					friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_iterator& b) noexcept;

					friend class raw_iterator;
					friend class iterator;
					friend class reverse_iterator;
					friend class const_reverse_iterator;
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

					constexpr bool operator==(raw_iterator other) const noexcept;
					constexpr bool operator==(iterator other) const noexcept;
					constexpr bool operator==(const_iterator other) const noexcept;
					constexpr bool operator==(reverse_iterator other) const noexcept;
					constexpr bool operator==(const_reverse_iterator other) const noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkAllocator>;

					template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
					friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::reverse_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::reverse_iterator& b) noexcept;

					friend class raw_iterator;
					friend class iterator;
					friend class const_iterator;
					friend class const_reverse_iterator;
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

					constexpr bool operator==(raw_iterator other) const noexcept;
					constexpr bool operator==(iterator other) const noexcept;
					constexpr bool operator==(const_iterator other) const noexcept;
					constexpr bool operator==(reverse_iterator other) const noexcept;
					constexpr bool operator==(const_reverse_iterator other) const noexcept;

					constexpr reference operator*() noexcept;
					constexpr pointer operator->() noexcept;

					friend class stable_vector<T, Allocator, ChunkAllocator>;

					template <typename U, typename SwapAllocator, typename SwapChunkAllocator>
					friend constexpr void swap(stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_reverse_iterator& a, stable_vector<U, SwapAllocator, SwapChunkAllocator>::const_reverse_iterator& b) noexcept;

					friend class raw_iterator;
					friend class iterator;
					friend class const_iterator;
					friend class reverse_iterator;
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
			if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap) swap(a.m_allocator, b.m_allocator);
		}

		template <typename T, typename Allocator>
		constexpr void swap_for_copy_assignment(vector_chunk<T, Allocator>& a, vector_chunk<T, Allocator>& b) noexcept
		{
			std::swap(a.m_begin, b.m_begin);
			std::swap(a.m_size, b.m_size);
			std::swap(a.m_capacity, b.m_capacity);
			if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment) swap(a.m_allocator, b.m_allocator);
		}

		template <typename T, typename Allocator>
		constexpr void swap_for_move_assignment(vector_chunk<T, Allocator>& a, vector_chunk<T, Allocator>& b) noexcept
		{
			std::swap(a.m_begin, b.m_begin);
			std::swap(a.m_size, b.m_size);
			std::swap(a.m_capacity, b.m_capacity);
			if constexpr (std::allocator_traits<Allocator>::propagate_on_container_move_assignment) swap(a.m_allocator, b.m_allocator);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr void vector_chunk<T, Allocator>::update_deleter_size() noexcept
		{
			m_begin.get_deleter().update_size(m_size);
			m_begin.get_deleter().update_capacity(m_capacity);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr void detail::vector_chunk<T, Allocator>::copy_initialize(const vector_chunk<T, Allocator>& other)
		{
			std::size_t new_capacity = other.m_size;
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, new_capacity), chunk_deleter{m_allocator, 0, new_capacity});

			std::uninitialized_copy(other.begin(), other.end(), new_begin);

			m_begin = std::move(new_begin);

			m_capacity = other.m_size;
			m_size = other.m_size;

			update_deleter_size();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_begin() noexcept
		{
			return raw_iterator{m_begin.get()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_end() noexcept
		{
			return raw_iterator{m_begin.get() + m_size};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_cap_end() noexcept
		{
			return raw_iterator{m_begin.get() + m_capacity};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_before_end() noexcept
		{
			if (empty()) return raw_begin();
			else return std::prev(raw_end());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_before_begin() noexcept
		{
			if (empty()) return raw_begin();
			else return std::prev(raw_begin());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk() : m_allocator{}, m_begin{nullptr, chunk_deleter{m_allocator, 0, 0}}, m_size{0}, m_capacity{0} {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const Allocator& other) : m_allocator{other}, m_begin{nullptr, chunk_deleter{m_allocator, 0, 0}}, m_size{0}, m_capacity{0} {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::size_t n, size_tag, const Allocator& allocator) : vector_chunk{allocator}
		{
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, n), chunk_deleter{m_allocator, 0, n});
			std::uninitialized_default_construct_n(new_begin, n);

			m_begin = std::move(new_begin);

			m_capacity = n;
			m_size = n;

			update_deleter_size();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::size_t n, capacity_tag, const Allocator& allocator) : vector_chunk{allocator}
		{
			if (n > 0)
			{
				m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, n), chunk_deleter{m_allocator, 0, n});

				m_begin = std::move(new_begin);
				
				m_capacity = n;
				// m_size = 0

				update_deleter_size();
			}
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		template <std::input_iterator It>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(It first, It last, const Allocator& allocator) : vector_chunk{allocator}
		{
			std::size_t size = std::distance(first, last);
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, size), chunk_deleter{m_allocator, 0, size});

			std::uninitialized_copy(first, last, new_begin.get());

			m_begin = std::move(new_begin);

			m_capacity = size;
			m_size = size;

			update_deleter_size();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::initializer_list<T> init_list, const Allocator& allocator) : vector_chunk{init_list.begin(), init_list.end(), allocator}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		template <std::input_iterator Begin, std::sentinel_for<Begin> Sent>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::from_range_t, Begin first, Sent last, const Allocator& allocator) : vector_chunk{allocator}
		{
			std::size_t size = std::ranges::distance(first, last);
			m_begin_pointer new_begin(std::allocator_traits<Allocator>::allocate(m_allocator, size), chunk_deleter{m_allocator, 0, size});

			std::ranges::uninitialized_copy(first, last, new_begin.get(), new_begin.get() + size);

			m_begin = std::move(new_begin);

			m_capacity = size;
			m_size = size;

			update_deleter_size();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		template <std::ranges::input_range Range>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(std::from_range_t, Range&& range, const Allocator& allocator) : vector_chunk{range.begin(), range.end(), allocator}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const vector_chunk<T, Allocator>& other) : vector_chunk{other.m_allocator}
		{
			copy_initialize(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(const vector_chunk<T, Allocator>& other, const std::type_identity<Allocator>& allocator) : vector_chunk{allocator}
		{
			copy_initialize(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(detail::vector_chunk<T, Allocator>&& other) : m_allocator(other.m_allocator), m_begin(std::move(other.m_begin)),
																												 m_size(other.m_size), m_capacity(other.m_capacity)
		{
			other.m_size = 0;
			other.m_capacity = 0;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::vector_chunk(detail::vector_chunk<T, Allocator>&& other,
																const std::type_identity<Allocator>& allocator) : m_allocator(allocator), m_begin(std::move(other.begin())),
																												  m_size(other.m_size), m_capacity(other.m_capacity)
		{
			other.m_size = 0;
			other.m_capacity = 0;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>& detail::vector_chunk<T, Allocator>::operator=(const vector_chunk<T, Allocator>& other)
		{
			vector_chunk<T, Allocator> other_copy = other;
			swap_for_copy_assignment(*this, other_copy);
			return *this;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>& detail::vector_chunk<T, Allocator>::operator=(vector_chunk<T, Allocator>&& other)
		{
			vector_chunk<T, Allocator> other_copy = other;
			swap_for_move_assignment(*this, other_copy);
			return *this;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>& detail::vector_chunk<T, Allocator>::operator=(std::initializer_list<T> init_list)
		{
			static_assert(false);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		template <typename... Args>
		requires std::constructible_from<T, Args...>
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
				update_deleter_size();

				return true;
			}
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::push_back(const T& val)
		{
			return emplace_back(val);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::push_back(T&& val)
		{
			return emplace_back(std::move(val));
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
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
				update_deleter_size();
				
				return true;
			}
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::resize(std::size_t n) noexcept
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
					m_size = n;

					update_deleter_size();
				}
			}
			else if (n > m_size)
			{
				for (std::size_t index = m_size; index < n; index++)
				{
					std::allocator_traits<Allocator>::construct(m_allocator, m_begin.get() + index);
					m_size = n;

					update_deleter_size();
				}
			}
			return true;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr void detail::vector_chunk<T, Allocator>::clear() noexcept
		{
			for (std::size_t index = 0; index < m_size; index++)
			{
				std::allocator_traits<Allocator>::destroy(m_allocator, m_begin.get() + index);
			}
			m_size = 0;
			update_deleter_size();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::empty() const noexcept
		{
			return m_size == 0;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::full() const noexcept
		{
			return m_size == m_capacity;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::has_capacity() const noexcept
		{
			return m_capacity != 0;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::begin()
		{
			return iterator{raw_begin()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::end()
		{
			return iterator{raw_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::cap_end()
		{
			return iterator{raw_cap_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::const_iterator vector_chunk<T, Allocator>::cbegin() const
		{
			return const_iterator{raw_begin()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::const_iterator vector_chunk<T, Allocator>::cend() const
		{
			return const_iterator{raw_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::const_iterator vector_chunk<T, Allocator>::ccap_end() const
		{
			return const_iterator{raw_cap_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::reverse_iterator vector_chunk<T, Allocator>::rbegin()
		{
			return reverse_iterator{raw_before_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::reverse_iterator vector_chunk<T, Allocator>::rend()
		{
			return reverse_iterator{raw_before_begin()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::const_reverse_iterator vector_chunk<T, Allocator>::crbegin() const
		{
			return const_reverse_iterator{raw_before_end()};
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::const_reverse_iterator vector_chunk<T, Allocator>::crend() const
		{
			return const_reverse_iterator{raw_before_begin()};
		}





		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::raw_iterator() noexcept : m_ptr{nullptr}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::raw_iterator(pointer ptr) noexcept : m_ptr{ptr}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator& detail::vector_chunk<T, Allocator>::raw_iterator::operator++() noexcept
		{
			m_ptr++;
			return *this;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_iterator::operator++(int) noexcept
		{
			return raw_iterator(m_ptr++);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator& detail::vector_chunk<T, Allocator>::raw_iterator::operator--() noexcept
		{
			m_ptr--;
			return *this;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator detail::vector_chunk<T, Allocator>::raw_iterator::operator--(int) noexcept
		{
			return raw_iterator(m_ptr--);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(raw_iterator other) const noexcept
		{
			return m_ptr == other.m_ptr;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(iterator other) const noexcept
		{
			return operator==(static_cast<raw_iterator>(other));
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(const_iterator other) const noexcept
		{
			return operator==(static_cast<raw_iterator>(other));
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(reverse_iterator other) const noexcept
		{
			return operator==(static_cast<raw_iterator>(other));
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::raw_iterator::operator==(const_reverse_iterator other) const noexcept
		{
			return operator==(static_cast<raw_iterator>(other));
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::reference detail::vector_chunk<T, Allocator>::raw_iterator::operator*() noexcept
		{
			return *m_ptr;
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::raw_iterator::pointer detail::vector_chunk<T, Allocator>::raw_iterator::operator->() noexcept
		{
			return m_ptr;
		}

		template <typename T, typename Allocator>
		constexpr void swap(typename vector_chunk<T, Allocator>::raw_iterator& a, typename vector_chunk<T, Allocator>::raw_iterator& b) noexcept
		{
			std::swap(a.m_ptr, b.m_ptr);
		}












		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr vector_chunk<T, Allocator>::iterator::iterator(raw_iterator it) noexcept : raw_iterator{it}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator& detail::vector_chunk<T, Allocator>::iterator::operator++() noexcept
		{
			return static_cast<iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator& detail::vector_chunk<T, Allocator>::iterator::operator--() noexcept
		{
			return static_cast<iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator detail::vector_chunk<T, Allocator>::iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator==(iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator==(const_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator==(reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::iterator::operator==(const_reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::reference detail::vector_chunk<T, Allocator>::iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::iterator::pointer detail::vector_chunk<T, Allocator>::iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}

		template <typename T, typename Allocator>
		constexpr void swap(typename vector_chunk<T, Allocator>::iterator& a, typename vector_chunk<T, Allocator>::iterator& b) noexcept
		{
			swap<T, Allocator>(static_cast<vector_chunk<T, Allocator>::raw_iterator&>(a), static_cast<vector_chunk<T, Allocator>::raw_iterator&>(b));
		}











		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::const_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::const_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator& detail::vector_chunk<T, Allocator>::const_iterator::operator++() noexcept
		{
			return static_cast<const_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator detail::vector_chunk<T, Allocator>::const_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator& detail::vector_chunk<T, Allocator>::const_iterator::operator--() noexcept
		{
			return static_cast<const_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator detail::vector_chunk<T, Allocator>::const_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator==(iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator==(const_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator==(reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_iterator::operator==(const_reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::reference detail::vector_chunk<T, Allocator>::const_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_iterator::pointer detail::vector_chunk<T, Allocator>::const_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}

		template <typename T, typename Allocator>
		constexpr void swap(typename vector_chunk<T, Allocator>::const_iterator& a, typename vector_chunk<T, Allocator>::const_iterator& b) noexcept
		{
			swap(static_cast<vector_chunk<T, Allocator>::raw_iterator&>(a), static_cast<vector_chunk<T, Allocator>::raw_iterator&>(b));
		}








		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reverse_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reverse_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator& detail::vector_chunk<T, Allocator>::reverse_iterator::operator++() noexcept
		{
			return static_cast<reverse_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator detail::vector_chunk<T, Allocator>::reverse_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator& detail::vector_chunk<T, Allocator>::reverse_iterator::operator--() noexcept
		{
			return static_cast<reverse_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator detail::vector_chunk<T, Allocator>::reverse_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator==(iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}
		
		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator==(const_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator==(reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::reverse_iterator::operator==(const_reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::reference detail::vector_chunk<T, Allocator>::reverse_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::reverse_iterator::pointer detail::vector_chunk<T, Allocator>::reverse_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}

		template <typename T, typename Allocator>
		constexpr void swap(typename vector_chunk<T, Allocator>::reverse_iterator& a, typename vector_chunk<T, Allocator>::reverse_iterator& b) noexcept
		{
			swap(static_cast<vector_chunk<T, Allocator>::raw_iterator&>(a), static_cast<vector_chunk<T, Allocator>::raw_iterator&>(b));
		}











		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::const_reverse_iterator() noexcept : raw_iterator{}  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::const_reverse_iterator(pointer ptr) noexcept : raw_iterator(ptr)  {}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator& detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator++() noexcept
		{
			return static_cast<const_reverse_iterator&>(raw_iterator::operator--());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator++(int) noexcept
		{
			return raw_iterator::operator--(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator& detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator--() noexcept
		{
			return static_cast<const_reverse_iterator&>(raw_iterator::operator++());
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator--(int) noexcept
		{
			return raw_iterator::operator++(0);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator==(iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator==(const_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator==(reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr bool detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator==(const_reverse_iterator other) const noexcept
		{
			return raw_iterator::operator==(other);
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::reference detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator*() noexcept
		{
			return raw_iterator::operator*();
		}

		template <typename T, typename Allocator>
		requires valid_vector_chunk_template_args<T, Allocator>
		constexpr detail::vector_chunk<T, Allocator>::const_reverse_iterator::pointer detail::vector_chunk<T, Allocator>::const_reverse_iterator::operator->() noexcept
		{
			return raw_iterator::operator->();
		}

		template <typename T, typename Allocator>
		constexpr void swap(typename vector_chunk<T, Allocator>::const_reverse_iterator& a, typename vector_chunk<T, Allocator>::const_reverse_iterator& b) noexcept
		{
			swap(static_cast<vector_chunk<T, Allocator>::raw_iterator&>(a), static_cast<vector_chunk<T, Allocator>::raw_iterator&>(b));
		}
	}






	

	




	template <typename T, typename Allocator, typename ChunkAllocator> 
	constexpr void swap(stable_vector<T, Allocator, ChunkAllocator>& a, stable_vector<T, Allocator, ChunkAllocator>& b) noexcept
	{
		std::swap(a.m_chunks, b.m_chunks);
		std::swap(a.m_size, b.m_size);
		std::swap(a.m_capacity, b.m_capacity);
		swap<T, Allocator, ChunkAllocator>(a.m_end, b.m_end);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(uninit_tag) : m_allocator{}, m_chunks{}, m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(uninit_tag, const Allocator& allocator) : m_allocator{allocator}, m_chunks{}, m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(uninit_tag, const Allocator& allocator,
																			 const ChunkAllocator& chunk_allocator) : m_allocator{allocator}, m_chunks(chunk_allocator), m_size{0}, m_capacity{0}, m_end{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::init_empty_chunks()
	{
		push_empty_chunk();
		push_empty_chunk();

		list_iterator last_chunk_it = std::prev(m_chunks.end());
		chunk& last_chunk = *last_chunk_it;
		m_end = iterator{last_chunk_it, last_chunk.begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::copy_initialize(const stable_vector<T, Allocator, ChunkAllocator>& other)
	{
		init_empty_chunks();

		chunk new_chunk{other.size(), typename chunk::capacity_tag{}};
		std::uninitialized_copy(other.cbegin(), other.cend(), new_chunk.begin());
		new_chunk.m_size = other.size();
		new_chunk.update_deleter_size();
		m_chunks.emplace(std::prev(m_chunks.end()), std::move(new_chunk));

		m_size = other.size();
		m_capacity = other.size();
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}


	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::chunks_full() const noexcept
	{
		return m_chunks.back().full();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::current_chunk_empty() const noexcept
	{
		chunk& last_chunk = *(m_end.get_list_iterator());
		return last_chunk.empty();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::current_chunk_full() const noexcept
	{
		chunk& last_chunk = *(m_end.get_list_iterator());
		return last_chunk.full();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::end_at_chunk_start() const noexcept
	{
		chunk& current_chunk = *(m_end.get_list_iterator());
		return current_chunk.begin() == m_end.get_chunk_iterator();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::current_chunk_has_capacity() const noexcept
	{
		chunk& current_chunk = *(m_end.get_list_iterator());
		return current_chunk.has_capacity();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::push_chunk(std::size_t n)
	{
		m_chunks.emplace_back(n, typename chunk::capacity_tag{});

		m_capacity += n;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::push_empty_chunk()
	{
		m_chunks.emplace_back();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::has_capacity() const noexcept
	{
		return m_capacity != 0;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::one_away_from_full() const noexcept
	{
		return m_capacity == (m_size + 1);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector() : stable_vector{uninit_tag{}}
	{
		init_empty_chunks();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(const Allocator& allocator) : stable_vector{uninit_tag{}, allocator}
	{
		init_empty_chunks();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(const Allocator& allocator, const ChunkAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		init_empty_chunks();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(std::size_t n, const Allocator& allocator, const ChunkAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		init_empty_chunks();

		m_chunks.emplace(std::prev(m_chunks.end()), n, typename chunk::size_tag{});

		m_size = n;
		m_capacity = n;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	template <std::input_iterator It>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(It first, It last, const Allocator& allocator, const ChunkAllocator& chunk_allocator) : stable_vector{uninit_tag{}}
	{
		init_empty_chunks();

		std::size_t size = std::distance(first, last);
		m_chunks.emplace(std::prev(m_chunks.end()), first, last);
		
		m_size = size;
		m_capacity = size;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(std::initializer_list<T> init_list, const Allocator& allocator,
																			 const ChunkAllocator& chunk_allocator) : stable_vector{init_list.begin(), init_list.end(), allocator, chunk_allocator}  {}
	
	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	template <std::input_iterator Begin, std::sentinel_for<Begin> Sent>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(std::from_range_t, Begin first, Sent last,
																			 const Allocator& allocator, const ChunkAllocator& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		init_empty_chunks();

		std::size_t size = std::ranges::distance(first, last);
		m_chunks.emplace(std::prev(m_chunks.end()), std::from_range_t{}, first, last);

		m_size = size;
		m_capacity = size;
		m_end = iterator{std::prev(m_chunks.end()), m_chunks.back().begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	template <std::ranges::input_range Range>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(std::from_range_t, Range&& range, const Allocator& allocator,
																			 const ChunkAllocator& chunk_allocator) : stable_vector{std::from_range_t{}, std::ranges::begin(range),
																			 														   std::ranges::end(range), allocator, chunk_allocator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other) : stable_vector{uninit_tag{}}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other, const std::type_identity<Allocator>& allocator) : stable_vector{uninit_tag{}, allocator}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(const stable_vector<T, Allocator, ChunkAllocator>& other, const std::type_identity<Allocator>& allocator,
																			 const std::type_identity<ChunkAllocator>& chunk_allocator) : stable_vector{uninit_tag{}, allocator, chunk_allocator}
	{
		copy_initialize(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other) : m_allocator{}, m_chunks(std::move(other.m_chunks), ChunkAllocator{}), m_size{other.m_size},
																																		m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other,
																			 const std::type_identity<Allocator>& allocator) : m_allocator{allocator}, m_chunks(std::move(other.m_chunks)), ChunkAllocator{}, m_size{other.m_size},
																															   m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::stable_vector(stable_vector<T, Allocator, ChunkAllocator>&& other,
																			 const std::type_identity<Allocator>& allocator,
																			 const std::type_identity<ChunkAllocator>& chunk_allocator) : m_allocator{allocator}, m_chunks{std::move(other.m_chunks), chunk_allocator}, m_size{other.m_size},
																																			  m_capacity{other.m_capacity}, m_end{other.m_end}
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_end = iterator{};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>& stable_vector<T, Allocator, ChunkAllocator>::operator=(const stable_vector<T, Allocator, ChunkAllocator>& other)
	{
		stable_vector<T, Allocator, ChunkAllocator> other_copy = other;
		swap(*this, other);
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>& stable_vector<T, Allocator, ChunkAllocator>::operator=(stable_vector<T, Allocator, ChunkAllocator>&& other)
	{
		stable_vector<T, Allocator, ChunkAllocator> other_copy = other;
		swap(*this, other);
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>& stable_vector<T, Allocator, ChunkAllocator>::operator=(std::initializer_list<T> init_list)
	{
		stable_vector<T, Allocator, ChunkAllocator> new_vec(init_list);
		swap(*this, new_vec);
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	template <typename... Args>
	requires std::constructible_from<T,  Args...>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::emplace_back(Args&&... args)
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

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::push_back(const T& val)
	{
		emplace_back(val);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::push_back(T&& val)
	{
		emplace_back(std::move(val));
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::pop_back()
	{
		iterator prev = std::prev(end());
		chunk& current_chunk = *(prev.get_list_iterator());
		current_chunk.pop_back();

		m_end--;
		m_size--;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::reserve_extra(std::size_t n)
	{
		static_assert(false);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::clear()
	{
		m_chunks.clear();
		
		m_size = 0;
		m_end = begin();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::empty() const noexcept
	{
		return m_size == 0;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr std::size_t stable_vector<T, Allocator, ChunkAllocator>::size() const noexcept
	{
		return m_size;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator stable_vector<T, Allocator, ChunkAllocator>::begin() noexcept
	{
		return iterator{raw_begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator stable_vector<T, Allocator, ChunkAllocator>::end() noexcept
	{
		return iterator{raw_end()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator stable_vector<T, Allocator, ChunkAllocator>::cbegin() const noexcept
	{
		return const_iterator{const_cast<stable_vector<T, Allocator, ChunkAllocator>&>(*this).raw_begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator stable_vector<T, Allocator, ChunkAllocator>::cend() const noexcept
	{
		return const_iterator{const_cast<stable_vector<T, Allocator, ChunkAllocator>&>(*this).raw_end()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::rbegin() noexcept
	{
		return reverse_iterator{raw_before_end()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::rend() noexcept
	{
		return reverse_iterator{raw_before_begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::crbegin() const noexcept
	{
		return const_reverse_iterator{const_cast<stable_vector<T, Allocator, ChunkAllocator>&>(*this).raw_before_end()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::crend() const noexcept
	{
		return const_reverse_iterator{const_cast<stable_vector<T, Allocator, ChunkAllocator>&>(*this).raw_before_begin()};
	}






	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::increment() noexcept
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

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr void stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::decrement() noexcept
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

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::list_iterator_type& stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::get_list_iterator() noexcept
	{
		return m_list_iterator;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::chunk_iterator_type& stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::get_chunk_iterator() noexcept
	{
		return m_chunk_iterator;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_begin() noexcept
	{
		list_iterator second_it = std::next(m_chunks.begin());
		chunk& second = *second_it;
		return raw_iterator{second_it, second.begin()};
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_end() noexcept
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

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_before_end() noexcept
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

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_before_begin() noexcept
	{
		if (!has_capacity())
		{
			return raw_begin();
		}
		else
		{
			return std::prev(raw_begin());
		}
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::raw_iterator() noexcept : m_list_iterator{}, m_chunk_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::raw_iterator(list_iterator_type list_iterator,
																						  chunk_iterator_type chunk_iterator)  noexcept : m_list_iterator{list_iterator},
																						  												  m_chunk_iterator{chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator& stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator++() noexcept
	{
		increment();
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator++(int) noexcept
	{
		stable_vector<T, Allocator, ChunkAllocator>::iterator prev_it = *this;
		increment();
		return prev_it;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator& stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator--() noexcept
	{
		decrement();
		return *this;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator--(int) noexcept
	{
		stable_vector<T, Allocator, ChunkAllocator>::iterator prev_it = *this;
		decrement();
		return prev_it;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator==(raw_iterator other) const noexcept
	{
		return (m_list_iterator == other.m_list_iterator)  &&  (m_chunk_iterator == other.m_chunk_iterator);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator==(iterator other) const noexcept
	{
		return operator==(static_cast<raw_iterator>(other));
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator==(const_iterator other) const noexcept
	{
		return operator==(static_cast<raw_iterator>(other));
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator==(reverse_iterator other) const noexcept
	{
		return operator==(static_cast<raw_iterator>(other));
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator==(const_reverse_iterator other) const noexcept
	{
		return operator==(static_cast<raw_iterator>(other));
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::reference stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator*() noexcept
	{
		return *m_chunk_iterator;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::pointer stable_vector<T, Allocator, ChunkAllocator>::raw_iterator::operator->() noexcept
	{
		return &*m_chunk_iterator;
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	constexpr void swap(typename stable_vector<T, Allocator, ChunkAllocator>::raw_iterator& a, typename stable_vector<T, Allocator, ChunkAllocator>::raw_iterator& b) noexcept
	{
		swap(a.m_list_iterator, b.m_list_iterator);
		swap<T, Allocator>(a.m_chunk_iterator, b.m_chunk_iterator);
	}






	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator::iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator::iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator::iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator& stable_vector<T, Allocator, ChunkAllocator>::iterator::operator++() noexcept
	{
		return static_cast<iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator stable_vector<T, Allocator, ChunkAllocator>::iterator::operator++(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator& stable_vector<T, Allocator, ChunkAllocator>::iterator::operator--() noexcept
	{
		return static_cast<iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator stable_vector<T, Allocator, ChunkAllocator>::iterator::operator--(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::iterator::operator==(iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::iterator::operator==(const_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::iterator::operator==(reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::iterator::operator==(const_reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator::reference stable_vector<T, Allocator, ChunkAllocator>::iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::iterator::pointer stable_vector<T, Allocator, ChunkAllocator>::iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	constexpr void swap(typename stable_vector<T, Allocator, ChunkAllocator>::iterator& a, typename stable_vector<T, Allocator, ChunkAllocator>::iterator& b) noexcept
	{
		swap<T, Allocator, ChunkAllocator>(static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(a), static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(b));
	}









	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator::const_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator::const_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator::const_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator& stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator++() noexcept
	{
		return static_cast<const_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator& stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator--() noexcept
	{
		return static_cast<const_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator==(iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator==(const_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}
	
	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator==(reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator==(const_reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator::reference stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_iterator::pointer stable_vector<T, Allocator, ChunkAllocator>::const_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	constexpr void swap(typename stable_vector<T, Allocator, ChunkAllocator>::const_iterator& a, typename stable_vector<T, Allocator, ChunkAllocator>::const_iterator& b) noexcept
	{
		swap(static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(a), static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(b));
	}












	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::reverse_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::reverse_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator& stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator++() noexcept
	{
		return static_cast<reverse_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator& stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator--() noexcept
	{
		return static_cast<reverse_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator==(iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator==(const_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator==(reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator==(const_reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::reference stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::pointer stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	constexpr void swap(typename stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator& a, typename stable_vector<T, Allocator, ChunkAllocator>::reverse_iterator& b) noexcept
	{
		swap(static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(a), static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(b));
	}









	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::const_reverse_iterator() noexcept : raw_iterator{}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::const_reverse_iterator(raw_iterator it) : raw_iterator{it}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::const_reverse_iterator(raw_iterator::list_iterator_type list_iterator, raw_iterator::chunk_iterator_type chunk_iterator) noexcept : raw_iterator{list_iterator, chunk_iterator}  {}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator& stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator++() noexcept
	{
		return static_cast<const_reverse_iterator&>(raw_iterator::operator--());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator++(int) noexcept
	{
		return raw_iterator::operator--(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator& stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator--() noexcept
	{
		return static_cast<const_reverse_iterator&>(raw_iterator::operator++());
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator--(int) noexcept
	{
		return raw_iterator::operator++(0);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator==(iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator==(const_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator==(reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr bool stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator==(const_reverse_iterator other) const noexcept
	{
		return raw_iterator::operator==(other);
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::reference stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator*() noexcept
	{
		return raw_iterator::operator*();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	requires valid_stable_vector_template_args<T, Allocator, ChunkAllocator>
	constexpr stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::pointer stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator::operator->() noexcept
	{
		return raw_iterator::operator->();
	}

	template <typename T, typename Allocator, typename ChunkAllocator>
	constexpr void swap(typename stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator& a, typename stable_vector<T, Allocator, ChunkAllocator>::const_reverse_iterator& b) noexcept
	{
		swap(static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(a), static_cast<stable_vector<T, Allocator, ChunkAllocator>::raw_iterator&>(b));
	}
}