// config.hpp
#pragma once

#ifndef ICE_DEBUG
#  ifdef NDEBUG
#    define ICE_DEBUG 0
#  else
#    define ICE_DEBUG 1
#  endif
#endif

#if ICE_DEBUG
#  define ICE_ASSERT(expression) assert(expression)
#else
#  define ICE_ASSERT(expression)
#endif

#ifdef _WIN32
#  define ICE_EXPORT __declspec(dllexport)
#  define ICE_IMPORT __declspec(dllimport)
#else
#  define ICE_EXPORT __attribute__((__visibility__("default")))
#  define ICE_IMPORT
#endif

#ifdef ICE_SHARED
#  ifdef ICE_EXPORTS
#    define ICE_API ICE_EXPORT
#  else
#    define ICE_API ICE_IMPORT
#  endif
#else
#  define ICE_API
#endif

#ifdef __has_feature
#  if __has_feature(cxx_exceptions)
#    define ICE_EXCEPTIONS 1
#  endif
#elif defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#  define ICE_EXCEPTIONS 1
#endif
#ifndef ICE_EXCEPTIONS
#  define ICE_EXCEPTIONS 0
#endif

#ifdef __has_feature
#  if __has_feature(cxx_rtti)
#    define ICE_RTTI 1
#  endif
#elif defined(_CPPRTTI)
#  define ICE_RTTI 1
#endif
#ifndef ICE_RTTI
#  define ICE_RTTI 0
#endif

#ifdef __has_builtin
#  if __has_builtin(__builtin_expect)
#    define ICE_LIKELY(expr) __builtin_expect(expr, 1)
#  endif
#endif
#ifndef ICE_LIKELY
#  define ICE_LIKELY(expr) expr
#endif

#ifdef __has_builtin
#  if __has_builtin(__builtin_expect)
#    define ICE_UNLIKELY(expr) __builtin_expect(expr, 0)
#  endif
#endif
#ifndef ICE_UNLIKELY
#  define ICE_UNLIKELY(expr) expr
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#  define ICE_ALWAYS_INLINE inline __forceinline
#  define ICE_NOINLINE __declspec(noinline)
#else
#  define ICE_ALWAYS_INLINE inline __attribute__((always_inline))
#  define ICE_NOINLINE __attribute__((noinline))
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#  define ICE_OFFSETOF(s, m) ((size_t)(&reinterpret_cast<char const volatile&>((((s*)0)->m))))
#else
#  define ICE_OFFSETOF __builtin_offsetof
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#  define ICE_FUNCTION __FUNCSIG__
#else
#  define ICE_FUNCTION __PRETTY_FUNCTION__
#endif

#include <ciso646>

#if defined(_LIBCPP_BEGIN_NAMESPACE_STD) && defined(_LIBCPP_END_NAMESPACE_STD)
_LIBCPP_BEGIN_NAMESPACE_STD

template <class, class>
struct formatter;

_LIBCPP_END_NAMESPACE_STD
#elif defined(_STD_BEGIN) && defined(_STD_END)
_STD_BEGIN

template <class, class>
struct formatter;

_STD_END
#else
#  include <format>
#endif