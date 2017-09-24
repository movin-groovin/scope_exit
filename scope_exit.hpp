
#include <type_traits>
#include <utility>
#include <tuple>



namespace mpl
{
	// need to do:
	// 1.) using F, that is functor with overloaded operator(). (2 and more operaotr() in class of F)
	//
	template <typename F, typename ... Args>
	class scope_exit final
	{
	private:
		static constexpr bool logical_and()
		{
			return true;
		}
		//
		template <typename T, typename ... TArgs>
		static constexpr bool logical_and(T t, TArgs ... targs)
		{
			return t && logical_and(targs...);
		}
		
		//
		template <typename T>
		struct base_check_dcm // destructor_copy_move
		{
			static_assert(
				std::is_nothrow_destructible<T>::value || std::is_same<T, void>::value,
				"T must be nothrow_destructible"
			);
			static_assert(
				std::is_nothrow_copy_constructible<T>::value || std::is_same<T, void>::value,
				"T must be nothrow_copy_constructible"
			);
			static_assert(
				std::is_nothrow_move_constructible<T>::value || std::is_same<T, void>::value,
				"T must be nothrow_move_constructible"
			);
			
			static constexpr bool value = true;
		};
		
		//
		template <typename T1, typename T2>
		struct base_check_dt1ct1t2 // destructor_type1_convert_typ1_type2
		{
			static_assert(
				std::is_nothrow_destructible<T1>::value || std::is_same<T1, void>::value,
				"T1 must be nothrow_destructible"
			);
			static_assert(
				std::is_nothrow_constructible<T1, T2>::value,
				"T1 must be nothrow_destructible"
			);
			
			static constexpr bool value = true;
		};
		
		//
		template <typename ... XArgs>
		struct types_checker
		{
			// check for nothrow XArgs:
			// 1.) destructor
			// 2.) copy constructor if exists
			// 3.) move constructor if exists
			
			static constexpr bool value = scope_exit::logical_and( base_check_dcm<XArgs>::value ... );
		};
		
		//
		template <typename L1, typename L2>
		struct binary_types_checker;
		//
		template < template<typename...> class L1, typename ... L1Args, template<typename...> class L2, typename ... L2Args >
		struct binary_types_checker< L1<L1Args...>, L2<L2Args...> >
		{
			// check for nothrow:
			// 1.) destructor L1Args
			// 2.) constructor L1Args from L2Args (rvalue or lvalue L2Args)
			
			static constexpr bool value = scope_exit::logical_and( base_check_dt1ct1t2< L1Args, L2Args >::value ... );
		};
		
		//
		template <typename XF, typename ... XArgs>
		struct callable_checker;
		// not const operator()
		template <typename R, typename C, typename ... FArgs, typename ... XArgs>
		struct callable_checker< R (C::*)(FArgs...), XArgs... >
		{
			// check for nothrow R
			// 1.) destructor
			// 2.) copy constructor if exists
			// 3.) move constructor if exists
			
			using type = std::integral_constant<
				size_t,
				types_checker<FArgs...>::value &&
					binary_types_checker< std::tuple<FArgs...>, std::tuple<XArgs...> >::value &&
					types_checker<R>::value
			>;
		};
		// const operator()
		template <typename R, typename C, typename ... FArgs, typename ... XArgs>
		struct callable_checker< R (C::*)(FArgs...) const, XArgs... >
		{
			// check for nothrow R
			// 1.) destructor
			// 2.) copy constructor if exists
			// 3.) move constructor if exists
			
			using type = std::integral_constant<
				size_t,
				types_checker<FArgs...>::value &&
					binary_types_checker< std::tuple<FArgs...>, std::tuple<XArgs...> >::value &&
					types_checker<R>::value
			>;
		};
		// independent functions
		template <typename R, typename ... FArgs, typename ... XArgs>
		struct callable_checker< R(FArgs...), XArgs... >
		{
			// check for nothrow R
			// 1.) destructor
			// 2.) copy constructor if exists
			// 3.) move constructor if exists
			
			using type = std::integral_constant<
				size_t,
				types_checker<FArgs...>::value &&
					binary_types_checker< std::tuple<FArgs...>, std::tuple<XArgs...> >::value &&
					types_checker<R>::value
			>;
		};
		
		//
		template <typename T>
		struct get_param_types
		{
			using type = decltype( &T::operator() );
		};
		//
		template <typename R, typename ... FArgs>
		struct get_param_types<R(*)(FArgs...)>
		{
			using type = R(FArgs...);
		};
		
		//
		template <size_t ... I>
		void call(std::index_sequence<I...>)
		{
			m_f( std::forward<Args>( std::get<I>(m_args) ) ... );
		}
		
	public:
		scope_exit(F && f, Args && ... args):
			m_f(std::forward<F>(f)),
			m_args(std::forward<Args>(args)...),
			m_active(true)
		{
			return;
		}
		
		~ scope_exit()
		{
			if(m_active) call( std::make_index_sequence<sizeof ... (Args)>() );
		}
		
		void activate() noexcept { m_active = true; }
		
		void disactivate() noexcept { m_active = false; }
		
	private:
		F m_f;
		std::tuple<Args...> m_args;
		bool m_active;
		// for templates instantiation only
		bool m_check = callable_checker<
			typename get_param_types<F>::type,
			Args...
		>::type::value && types_checker<Args...>::value;
	};
	
	
	template <typename F, typename ... Args>
	auto make_scope_exit(F && f, Args && ... args)
	{
		return scope_exit<F, Args...>( std::forward<F>(f), std::forward<Args>(args)... );
	}
}





