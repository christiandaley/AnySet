export module AnySet;

import <any>;
import <unordered_map>;
import <typeindex>;
import <typeinfo>;

export namespace Utility
{
	template <typename T>
	constexpr inline void check_value_type ()
	{
		static_assert (!std::is_reference_v<T>, "Cannot store references in AnySet");
		static_assert (!std::is_const_v<T>, "Cannot store const values in AnySet");
	}

	template <typename T>
	constexpr inline std::type_index get_index () noexcept
	{
		return std::type_index (typeid (T));
	}

	class AnySet
	{
	public:
		AnySet () = default;
		AnySet (const AnySet&) = default;
		AnySet (AnySet&&) = default;
		AnySet& operator= (const AnySet&) = default;
		AnySet& operator= (AnySet&&) = default;
		~AnySet () = default;

		template <typename T>
		const T* get () const noexcept;

		template <typename T>
		T* get () noexcept;

		template <typename T>
		bool insert (T&& value) noexcept;
	private:
		std::unordered_map<std::type_index, std::any> m_map;

		template <typename T>
		std::any* find () noexcept;
	};

	template <typename T>
	auto AnySet::get () const noexcept -> const T*
	{
		check_value_type<T> ();
		return const_cast<AnySet*> (this)->get ();
	}

	template <typename T>
	auto AnySet::get () noexcept -> T*
	{
		check_value_type<T> ();
		return std::any_cast<T> (find<T> ());
	}

	template <typename T>
	bool AnySet::insert (T&& value) noexcept
	{
		auto any_value = std::any (std::forward<T> (value));
		auto [it, inserted] = m_map.insert ({ get_index<T> (), std::move (any_value) });
		return inserted;
	}

	template <typename T>
	std::any* AnySet::find () noexcept
	{
		check_value_type<T> ();
		if (auto it = m_map.find (get_index<T> ()); it != m_map.end ())
		{
			return &it->second;
		}

		return nullptr;
	}
}