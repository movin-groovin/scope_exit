
#include <type_traits>
#include <utility>
#include <tuple>



namespace mpl
{
	// CFT - callable type of functor with overloaded 'operator()'(manually setted)
	// F - type of callable object(derived)
	// Args... - derived types of arguments
	template <typename CFT, typename F, typename ... Args>
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
		template <typename T, typename XCFT>
		struct get_param_types
		{	
			template <typename XT, typename U>
			struct extract;
			//
			template <typename XT, typename R, typename ... XArgs>
			struct extract<XT, R(XArgs...)>
			{
				using type = R(XT::*)(XArgs...);
				using for_call_type = XT&;
				
				static constexpr bool const_interface = false;
			};
			//
			template <typename XT, typename R, typename ... XArgs>
			struct extract<XT, R(XArgs...) const>
			{
				using type = R(XT::*)(XArgs...) const;
				using for_call_type = XT const&;
				
				static constexpr bool const_interface = true;
			};
			
			using type = typename extract<T, XCFT>::type;
			using for_call_type = typename extract<T, XCFT>::for_call_type;
			
			static constexpr bool const_interface = extract<T, XCFT>::const_interface;
		};
		//
		template <typename T>
		struct get_param_types<T, void>
		{
			using type = decltype( &T::operator() );
			using for_call_type = void;
			
			static constexpr bool const_interface = false;
		};
		//
		template <typename R, typename ... FArgs>
		struct get_param_types<R(*)(FArgs...), void>
		{
			using type = R(FArgs...);
			using for_call_type = void;
			
			static constexpr bool const_interface = false;
		};
		
		//
		template <size_t ... I>
		void call(std::true_type, std::index_sequence<I...>)
		{
			static_cast<for_call_type>(m_f)(
				std::forward<Args>( std::get<I>(m_args) ) ...
			);
		}
		//
		template <size_t ... I>
		void call(std::false_type, std::index_sequence<I...>)
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
			using selection_type = typename std::conditional<
				std::is_same<CFT, void>::value,
				std::false_type,
				std::true_type
			>::type;
			if(m_active) {
				call(
					selection_type(),
					std::make_index_sequence<sizeof ... (Args)>()
				);
			}
			return;
		}
		
		void activate() noexcept { m_active = true; }
		
		void disactivate() noexcept { m_active = false; }
		
	private:
		using f_type = typename std::remove_reference<F>::type;
		using call_type = typename get_param_types< f_type, CFT >::type;
		using for_call_type = typename get_param_types< f_type, CFT >::for_call_type;
		
		static constexpr bool const_interface = get_param_types< f_type, CFT >::const_interface;
		
		static_assert(
			!(std::is_const<f_type>::value && !const_interface),
			"Can't call non const method for cons object"
		);
		
	private:
		F m_f;
		std::tuple<Args...> m_args;
		bool m_active;
		// for templates instantiation only
		bool m_check = callable_checker< call_type, Args... >::type::value &&
							types_checker<Args...>::value;
	};
	
	
	template <typename CFT, typename F, typename ... Args>
	auto make_scope_exit(F && f, Args && ... args)
	{
		static_assert(
			std::is_class< typename std::remove_reference<F>::type >::value,
			"You can specify CFT only for classes and structures"
		);
		return scope_exit<CFT, F, Args...>( std::forward<F>(f), std::forward<Args>(args)... );
	}
	
	template <typename F, typename ... Args>
	auto make_scope_exit(F && f, Args && ... args)
	{
		return scope_exit<void, F, Args...>( std::forward<F>(f), std::forward<Args>(args)... );
	}
	//
}





