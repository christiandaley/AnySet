export module any_set;

import <atomic>;
import <unordered_map>;

namespace
{
	// utility functions
	
	const size_t get_id () noexcept
	{
		static std::atomic<size_t> id = 0;
		return id++;
	}
	 
	template <typename T>
	size_t type_id () noexcept
	{
		static const auto id = get_id ();
		return id;
	}

	// fast & simple implementation of "any" with small storage optimization

	union storage
	{
		constexpr storage () noexcept :
			m_heap (nullptr)
		{}

		void* m_heap;
		std::aligned_storage_t<sizeof (void*), alignof (void*)> m_stack;
	};

	template <typename T>
	constexpr bool construct_in_place_v =
		std::is_nothrow_move_constructible_v<T> &&
		sizeof (T) <= sizeof (storage::m_stack) &&
		alignof (T) <= alignof (decltype (storage::m_stack));

	enum class Op
	{
		Get,
		Copy,
		Move,
		Destroy
	};

	template <typename T>
	void* manage (Op op, storage& storage1, storage* storage2)
	{
		if constexpr (construct_in_place_v<T>)
		{
			switch (op)
			{
			case Op::Get:
				return &storage1.m_stack;
			case Op::Copy:
				new (&storage1.m_stack) T (*reinterpret_cast<const T*> (&storage2->m_stack));
				return nullptr;
			case Op::Move:
				new (&storage1.m_stack) T (std::move (*reinterpret_cast<T*> (&storage2->m_stack)));
				return nullptr;
			case Op::Destroy:
				reinterpret_cast<T*> (&storage1.m_stack)->~T ();
				return nullptr;
			default:
				return nullptr;
			}
		}
		else
		{
			switch (op)
			{
			case Op::Get:
				return storage1.m_heap;
			case Op::Copy:
				storage1.m_heap = new T (*static_cast<const T*> (storage2->m_heap));
				return nullptr;
			case Op::Move:
				storage1.m_heap = storage2->m_heap;
				storage2->m_heap = nullptr;
				return nullptr;
			case Op::Destroy:
				delete static_cast<T*> (storage1.m_heap);
				return nullptr;
			default:
				return nullptr;
			}
		}
	}

	class any_impl
	{
	public:
		template <typename T, typename... Args>
		inline static any_impl create (Args&&... args)
		{
			any_impl data;
			data.m_manage = &manage<T>;

			if constexpr (construct_in_place_v<T>)
			{
				new (&data.m_storage.m_stack) T (std::forward<Args> (args)...);
			}
			else
			{
				data.m_storage.m_heap = new T (std::forward<Args> (args)...);
			}

			return data;
		}

		inline void* get () noexcept
		{
			return m_manage (Op::Get, m_storage, nullptr);
		}

		any_impl (const any_impl& other) :
			m_manage (other.m_manage)
		{
			m_manage (Op::Copy, m_storage, const_cast<storage*> (&other.m_storage));
		}

		any_impl (any_impl&& other) noexcept :
			m_manage (other.m_manage)
		{
			m_manage (Op::Move, m_storage, &other.m_storage);
		}

		any_impl& operator= (const any_impl&) = delete;
		any_impl& operator= (any_impl&&) = delete;
		
		~any_impl ()
		{
			m_manage (Op::Destroy, m_storage, nullptr);
		}

	private:
		any_impl () noexcept = default;

		storage m_storage;
		void* (*m_manage) (Op, storage&, storage*);
	};
}

export namespace utility
{
	class type_not_found : public std::exception
	{
	public:
		virtual const char* what () const noexcept override
		{
			return "type not found";
		}
	};

	class any_set
	{
	public:
		any_set () noexcept = default;
		any_set (const any_set&) = default;
		any_set (any_set&&) noexcept = default;
		any_set& operator= (const any_set&) = default;
		any_set& operator= (any_set&&) noexcept = default;
		~any_set () noexcept = default;

		template <typename T>
		using value_type = std::decay_t<std::remove_cv_t<T>>;

		template <typename T>
		using const_value_type = const value_type<T>;

		template <typename T>
		using pointer_type = value_type<T>*;

		template <typename T>
		using const_pointer_type = const_value_type<T>*;

		template <typename T>
		using reference_type = value_type<T>&;

		template <typename T>
		using const_reference_type = const_value_type<T>&;

		template <typename T>
		inline auto get () const noexcept -> const_pointer_type<T>;

		template <typename T>
		inline auto get () noexcept -> pointer_type<T>;

		template <typename T>
		inline auto get_ref () const -> const_reference_type<T>;

		template <typename T>
		inline auto get_ref () -> reference_type<T>;

		template <typename T>
		inline bool insert (T&& value);

		template <typename T, typename... Args>
		inline bool emplace (Args&&... args);

		template <typename T, typename U, typename... Args>
		inline bool emplace (std::initializer_list<U> init_list, Args&&...);

		inline size_t size () const noexcept;

		inline bool empty () const noexcept;

		inline void clear () noexcept;

		template <typename... Args>
		inline size_t erase ();
	private:
		template <typename T>
		inline any_impl* find () noexcept;

		std::unordered_map<size_t, any_impl> m_map;
	};

	template <typename T>
	inline auto any_set::get () const noexcept -> const_pointer_type<T>
	{
		return const_cast<any_set*> (this)->get<T> ();
	}

	template <typename T>
	inline auto any_set::get () noexcept -> pointer_type<T>
	{
		if (auto data_p = find<value_type<T>> ())
		{
			return static_cast<pointer_type<T>> (data_p->get ());
		}

		return nullptr;
	}

	template <typename T>
	inline auto any_set::get_ref () const -> const_reference_type<T>
	{
		return const_cast<any_set*> (this)->get_ref<T> ();
	}

	template <typename T>
	inline auto any_set::get_ref () -> reference_type<T>
	{
		if (auto ptr = get<T> ())
		{
			return *ptr;
		}

		throw type_not_found ();
	}

	template <typename T>
	inline bool any_set::insert (T&& value)
	{
		using Vt = value_type<T>;

		auto data = any_impl::create<Vt> (std::forward<T> (value));
		auto [it, inserted] = m_map.insert ({ type_id<Vt> (), std::move (data) });
		return inserted;
	}

	template <typename T, typename... Args>
	inline bool any_set::emplace (Args&&... args)
	{
		using Vt = value_type<T>;

		auto data = any_impl::create<Vt> (std::forward<Args> (args)...);
		auto [it, inserted] = m_map.emplace (type_id<Vt> (), std::move (data));
		return inserted;
	}

	template <typename T, typename U, typename... Args>
	inline bool any_set::emplace (std::initializer_list<U> init_list, Args&&... args)
	{
		using Vt = value_type<T>;

		auto data = any_impl::create<Vt> (std::move (init_list), std::forward<Args> (args)...);
		auto [it, inserted] = m_map.emplace (type_id<Vt> (), std::move (data));
		return inserted;
	}

	inline size_t any_set::size () const noexcept
	{
		return m_map.size ();
	}

	inline bool any_set::empty () const noexcept
	{
		return m_map.empty ();
	}

	inline void any_set::clear () noexcept
	{
		m_map.clear ();
	}

	template <typename... Args>
	inline size_t any_set::erase ()
	{
		return (m_map.erase (type_id<value_type<Args>> ()) + ...);
	}

	template <typename T>
	inline any_impl* any_set::find () noexcept
	{
		static_assert (std::is_same_v<T, value_type<T>>);
		if (auto it = m_map.find (type_id<T> ()); it != m_map.end ())
		{
			return &it->second;
		}

		return nullptr;
	}
}