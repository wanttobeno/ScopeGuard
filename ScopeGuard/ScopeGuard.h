#pragma once
#include <type_traits>  // decay 
#include<exception>		//uncaught_exceptions
namespace  watchman
{

#if defined(_MSC_VER) && (_MSC_VER >= 1800) // MSVC++ 2015
#define WATCHMAN_EXCEPTION_COUNT_USE_STD
#else
	// Raise an error when trying to use this on unsupported platforms.
#error "Unsupported platform, don't include this header."
#endif

	namespace detail
	{
		/**
		* Used to check if a new uncaught exception was thrown by monitoring the
		* number of uncaught exceptions.
		*
		* Usage:
		*  - create a new UncaughtExceptionCounter object
		*  - call isNewUncaughtException() on the new object to check if a new
		*    uncaught exception was thrown since the object was created
		*/
		class UncaughtExceptionCounter
		{
		public:
			UncaughtExceptionCounter() noexcept
				: exceptionCount_(getUncaughtExceptionCount()) {}

			UncaughtExceptionCounter(const UncaughtExceptionCounter& other) noexcept
				: exceptionCount_(other.exceptionCount_) {}

			bool isNewUncaughtException() noexcept
			{
				return getUncaughtExceptionCount() > exceptionCount_;
			}

		private:
			int getUncaughtExceptionCount() noexcept;

			int exceptionCount_;
		};

		/**
		* Returns the number of uncaught exceptions.
		*
		* This function is based on Evgeny Panasyuk's implementation from here:
		* http://fburl.com/15190026
		*/
		inline int UncaughtExceptionCounter::getUncaughtExceptionCount() noexcept
		{
#if defined(WATCHMAN_EXCEPTION_COUNT_USE_CXA_GET_GLOBALS)
			// __cxa_get_globals returns a __cxa_eh_globals* (defined in unwind-cxx.h).
			// The offset below returns __cxa_eh_globals::uncaughtExceptions.
			return *(reinterpret_cast<unsigned int*>(
			static_cast<char*>(static_cast<void*>(__cxxabiv1::__cxa_get_globals())) +
			sizeof(void*)));
#elif defined(WATCHMAN_EXCEPTION_COUNT_USE_GETPTD)
			// _getptd() returns a _tiddata* (defined in mtdll.h).
			// The offset below returns _tiddata::_ProcessingThrow.
			return *(reinterpret_cast<int*>(
			static_cast<char*>(static_cast<void*>(_getptd())) + sizeof(void*) * 28 +
			0x4 * 8));
#elif defined(WATCHMAN_EXCEPTION_COUNT_USE_STD)
			return std::uncaught_exceptions();
#endif
		}

	} // namespace detail

	namespace detail
	{
		class ScopeGuardImplBase
		{
		public:
			void dismiss() noexcept
			{
				dismissed_ = true;
			}

		protected:
			ScopeGuardImplBase() noexcept : dismissed_(false) {}

			static ScopeGuardImplBase makeEmptyScopeGuard() noexcept
			{
				return ScopeGuardImplBase{};
			}

				template <typename T>
			static const T& asConst(const T& t) noexcept
			{
				return t;
			}

			bool dismissed_;
		};

		template <typename FunctionType>
		class ScopeGuardImpl : public ScopeGuardImplBase
		{
		public:
			explicit ScopeGuardImpl(FunctionType& fn) noexcept(
				std::is_nothrow_copy_constructible<FunctionType>::value)
				: ScopeGuardImpl(
				asConst(fn),
				makeFailsafe(
				std::is_nothrow_copy_constructible<FunctionType>{},
				&fn)) {}

			explicit ScopeGuardImpl(const FunctionType& fn) noexcept(
				std::is_nothrow_copy_constructible<FunctionType>::value)
				: ScopeGuardImpl(
				fn,
				makeFailsafe(
				std::is_nothrow_copy_constructible<FunctionType>{},
				&fn)) {}

			explicit ScopeGuardImpl(FunctionType&& fn) noexcept(
				std::is_nothrow_move_constructible<FunctionType>::value)
				: ScopeGuardImpl(
				std::move_if_noexcept(fn),
				makeFailsafe(
				std::is_nothrow_move_constructible<FunctionType>{},
				&fn)) {}

			ScopeGuardImpl(ScopeGuardImpl&& other) noexcept(
				std::is_nothrow_move_constructible<FunctionType>::value)
				: function_(std::move_if_noexcept(other.function_)) {
				// If the above line attempts a copy and the copy throws, other is
				// left owning the cleanup action and will execute it (or not) depending
				// on the value of other.dismissed_. The following lines only execute
				// if the move/copy succeeded, in which case *this assumes ownership of
				// the cleanup action and dismisses other.
				dismissed_ = other.dismissed_;
				other.dismissed_ = true;
			}

			~ScopeGuardImpl() noexcept{
				if (!dismissed_) {
					execute();
				}
			}

		private:
			static ScopeGuardImplBase makeFailsafe(std::true_type, const void*) noexcept
			{
				return makeEmptyScopeGuard();
			}

				template <typename Fn>
			static auto makeFailsafe(std::false_type, Fn* fn) noexcept
				->ScopeGuardImpl<decltype(std::ref(*fn))>{
				return ScopeGuardImpl<decltype(std::ref(*fn))>{std::ref(*fn)};
			}

				template <typename Fn>
			explicit ScopeGuardImpl(Fn&& fn, ScopeGuardImplBase&& failsafe)
				: ScopeGuardImplBase{}, function_(std::forward<Fn>(fn))
			{
				failsafe.dismiss();
			}

			void* operator new(std::size_t) = delete;

			void execute() noexcept
			{
				function_();
			}

			FunctionType function_;
		};

		template <typename F>
		using ScopeGuardImplDecay = ScopeGuardImpl<typename std::decay<F>::type>;

	} // namespace detail


	namespace detail
	{

#if defined(WATCHMAN_EXCEPTION_COUNT_USE_CXA_GET_GLOBALS) || \
    defined(WATCHMAN_EXCEPTION_COUNT_USE_GETPTD) ||          \
    defined(WATCHMAN_EXCEPTION_COUNT_USE_STD)

		/**
		* ScopeGuard used for executing a function when leaving the current scope
		* depending on the presence of a new uncaught exception.
		*
		* If the executeOnException template parameter is true, the function is
		* executed if a new uncaught exception is present at the end of the scope.
		* If the parameter is false, then the function is executed if no new uncaught
		* exceptions are present at the end of the scope.
		*
		* Used to implement SCOPE_FAIL and SCOPE_SUCCES below.
		*/
		template <typename FunctionType, bool executeOnException>
		class ScopeGuardForNewException
		{
		public:
			explicit ScopeGuardForNewException(const FunctionType& fn) : function_(fn) {}

			explicit ScopeGuardForNewException(FunctionType&& fn)
				: function_(std::move(fn)) {}

			ScopeGuardForNewException(ScopeGuardForNewException&& other)
				: function_(std::move(other.function_)),
				exceptionCounter_(std::move(other.exceptionCounter_)) {}

			~ScopeGuardForNewException() noexcept(executeOnException)
			{
				if (executeOnException == exceptionCounter_.isNewUncaughtException()) {
					function_();
				}
			}

		private:
			ScopeGuardForNewException(const ScopeGuardForNewException& other) = delete;

			void* operator new(std::size_t) = delete;

			FunctionType function_;
			UncaughtExceptionCounter exceptionCounter_;
		};

		/**
		* Internal use for the macro SCOPE_FAIL below
		*/
		enum class ScopeGuardOnFail {};

		template <typename FunctionType>
		ScopeGuardForNewException<typename std::decay<FunctionType>::type, true>
			operator+(detail::ScopeGuardOnFail, FunctionType&& fn)
		{
			return ScopeGuardForNewException<
				typename std::decay<FunctionType>::type,
				true>(std::forward<FunctionType>(fn));
		}

		/**
		* Internal use for the macro SCOPE_SUCCESS below
		*/
		enum class ScopeGuardOnSuccess {};

		template <typename FunctionType>
		ScopeGuardForNewException<typename std::decay<FunctionType>::type, false>
			operator+(ScopeGuardOnSuccess, FunctionType&& fn) {
			return ScopeGuardForNewException<
				typename std::decay<FunctionType>::type,
				false>(std::forward<FunctionType>(fn));
		}

#endif // native uncaught_exception() supported

		/**
		* Internal use for the macro SCOPE_EXIT below
		*/
		enum class ScopeGuardOnExit {};

		template <typename FunctionType>
		ScopeGuardImpl<typename std::decay<FunctionType>::type> operator+(
			detail::ScopeGuardOnExit,
			FunctionType&& fn)
		{
			return ScopeGuardImpl<typename std::decay<FunctionType>::type>(
				std::forward<FunctionType>(fn));
		}
	} // namespace detail


} // namespace watchman

// Helpers for pasting __LINE__ for symbol generation
#define w_paste2(pre, post) pre##post
#define w_paste1(pre, post) w_paste2(pre, post)
#define w_gen_symbol(pre) w_paste1(pre, __LINE__)

#define SCOPE_EXIT                      \
  auto w_gen_symbol(SCOPE_EXIT_STATE) = \
      ::watchman::detail::ScopeGuardOnExit() + [&]() noexcept