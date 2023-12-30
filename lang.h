#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using dtype = int;

#define IS_EXPR(Arg)                                                        \
  static_assert(std::is_invocable_v<Arg, const Context&>,                   \
                "template param is not an expr (needs to be callable with " \
                "context) for template param");
#define ARE_EXPR(Args)                                                      \
  static_assert((std::is_invocable_v<Args, const Context&> && ...),         \
                "template param pack is not an expr (needs to be callable " \
                "with context) for template params");
#define IS_DTYPE(Arg)                       \
  static_assert(std::is_same_v<dtype, Arg>, \
                "template param is not a dtype for template param");
#define ARE_DTYPE(Args)                               \
  static_assert((std::is_same_v<dtype, Args> && ...), \
                "template param pack is not a dtype for template params");

namespace impl {
template <class Program>
struct context {
  using mem_t = std::array<dtype, Program::nargs>;

  template <class... Args>
  constexpr context(Args... args) : global_context_({args...}) {
    ARE_DTYPE(Args);
  }

  constexpr dtype get_context(int idx) const { return global_context_[idx]; }

  constexpr Program& get_program() { return program_; }
  using program_t = Program;

 private:
  const mem_t global_context_;
  Program program_;
};
}  // namespace impl
template <class T, std::size_t num_args>
struct program {
  static constexpr std::size_t nargs = num_args;
  using prog = T;
  using local_context = impl::context<program<T, nargs>>;

  template <class... Args>
  constexpr dtype run(Args... args) const {  // note: decltype(auto) works with
                                             // mvsc, but not with clang???
    ARE_DTYPE(Args);
    return prog{}(local_context(args...));
  }
};

template <int idx>
struct variable {
  static constexpr dtype val = idx;
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    return context.get_context(idx);
  }
};
template <dtype v>
struct value {
  static constexpr dtype val = v;
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    return val;
  }
};

template <class Arg, class... Args>
struct add {
  // static check that arg has a call operator

  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Arg);
    if constexpr (sizeof...(Args) > 0)
      return Arg{}(context) + add<Args...>{}(context);
    return Arg()(context);
  }
};

template <class Body, class... Params>
struct function {
  template <class Context>
  constexpr decltype(auto) operator()(const Context& context) {
    IS_EXPR(Body);
    return program<Body, sizeof...(Params)>{};
  }
};

template <class Func, class... Args>
struct call {
  template <class Context>
  constexpr decltype(auto) operator()(const Context& context) {
    return Func{}(context).run(Args{}(context)...);
  }
};

template <class... Args>
struct recurse {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    using p = Context::program_t;
    return typename Context::program_t{}.run(Args{}(context)...);
  }
};

// comparison operators

template <class Lhs, class Rhs>
struct lt {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) < Rhs{}(context);
  }
};

template <class Lhs, class Rhs>
struct lteq {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) <= Rhs{}(context);
  }
};

template <class Lhs, class Rhs>
struct eq {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) == Rhs{}(context);
  }
};

template <class Cond, class TrueExpr, class FalseExpr = value<0>>
struct if_ {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Cond);
    IS_EXPR(TrueExpr);
    IS_EXPR(FalseExpr);
    if (Cond{}(context)) {
      return TrueExpr{}(context);
    }
    return FalseExpr{}(context);
  }
};

template <class Func, class Start, class End, class Accum = value<0>>
struct reduce {
  template <class Context>
  constexpr dtype operator()(const Context& context) {
    IS_EXPR(Start);
    IS_EXPR(End);
    IS_EXPR(Accum);
    dtype accum = Accum{}(context);
    dtype start = Start{}(context);
    dtype end = End{}(context);
    for (dtype i = start; i <= end; ++i) {
      accum = Func{}(context).run(i, accum);
    }
    return accum;
  }
};