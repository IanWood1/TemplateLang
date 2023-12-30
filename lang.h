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

using eval_dtype = double;
using input_dtype = int64_t;

template <input_dtype v>
struct value;

#define IS_EXPR(Arg)                                                        \
  static_assert(std::is_invocable_v<Arg, const Context&>,                   \
                "template param is not an expr (needs to be callable with " \
                "context) for template param");
#define ARE_EXPR(Args)                                                      \
  static_assert((std::is_invocable_v<Args, const Context&> && ...),         \
                "template param pack is not an expr (needs to be callable " \
                "with context) for template params");
#define IS_DTYPE(Arg)                            \
  static_assert(std::is_same_v<eval_dtype, Arg>, \
                "template param is not a eval_dtype for template param");
#define ARE_DTYPE(Args)                          \
  static_assert(                                 \
      (std::is_same_v<eval_dtype, Args> && ...), \
      "template param pack is not a eval_dtype for template params");

namespace impl {
template <class Program>
struct context {
  using mem_t = std::array<eval_dtype, Program::nargs>;

  template <class... Args>
  constexpr context(Args... args) : global_context_({args...}) {
    ARE_DTYPE(Args);
  }

  constexpr eval_dtype get_context(int idx) const {
    return global_context_[idx];
  }

  constexpr Program& get_program() { return program_; }
  using program_t = Program;

 private:
  const mem_t global_context_;
  Program program_;
};

using step_default = value<1>;
using add_identity = value<0>;
}  // namespace impl

template <class T, std::size_t num_args>
struct program {
  static constexpr std::size_t nargs = num_args;
  using prog = T;
  using local_context = impl::context<program<T, nargs>>;

  template <class... Args>
  constexpr eval_dtype run(
      Args... args) const {  // note: decltype(auto) works with
                             // mvsc, but not with clang???
    ARE_DTYPE(Args);
    return prog{}(local_context(args...));
  }
};

template <int idx>
struct variable {
  static constexpr eval_dtype val = idx;
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    return context.get_context(idx);
  }
};
template <input_dtype v>
struct value {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    return static_cast<eval_dtype>(v);
  }
};

// arithmetic operators
template <class Arg, class... Args>
struct add {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Arg);
    if constexpr (sizeof...(Args) > 0)
      return Arg{}(context) + add<Args...>{}(context);
    return Arg()(context);
  }
};

template <class Arg, class... Args>
struct mul {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Arg);
    if constexpr (sizeof...(Args) > 0)
      return Arg{}(context)*mul<Args...>{}(context);
    return Arg()(context);
  }
};

template <class Arg>
struct neg {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Arg);
    return -Arg{}(context);
  }
};

template <class Arg>
struct inv {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Arg);
    return 1 / Arg{}(context);
  }
};

// function operators
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
  constexpr eval_dtype operator()(const Context& context) {
    using p = Context::program_t;
    return typename Context::program_t{}.run(Args{}(context)...);
  }
};

// comparison operators
template <class Lhs, class Rhs>
struct lt {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) < Rhs{}(context);
  }
};

template <class Lhs, class Rhs>
struct lteq {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) <= Rhs{}(context);
  }
};

template <class Lhs, class Rhs>
struct eq {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Lhs);
    IS_EXPR(Rhs);
    return Lhs{}(context) == Rhs{}(context);
  }
};

// control flow operators
template <class Cond, class TrueExpr, class FalseExpr = impl::add_identity>
struct if_ {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Cond);
    IS_EXPR(TrueExpr);
    IS_EXPR(FalseExpr);
    if (Cond{}(context)) {
      return TrueExpr{}(context);
    }
    return FalseExpr{}(context);
  }
};

// generator: class with operator(const Context&) that returns a value for
// index and is_done(int) that returns true if the generator is done
template <class Func, class Start, class End, class Step = impl::step_default>
struct gen {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Start);
    IS_EXPR(End);
    if (!is_started_) {
      current_ = Start{}(context);
      is_started_ = true;
    }
    eval_dtype val = current_;
    current_ += Step{}(context);
    return Func{}(context).run(val);
  }

  template <class Context>
  constexpr bool is_done(const Context& context) {
    if (!is_started_) {
      current_ = Start{}(context);
      is_started_ = true;
    }
    return current_ == End{}(context);
  }

 private:
  bool is_started_ = false;
  eval_dtype current_;
};

template <class Gen, class Func>
struct compose {  // maybe map is a better name?
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    return local_func(context).run(local_gen(context));
  }

  template <class Context>
  constexpr bool is_done(const Context& context) {
    return local_gen.is_done(context);
  }

  Gen local_gen;
  Func local_func;
};

// generator consumers
template <class Gen>
struct sum {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    eval_dtype accum = 0;
    auto gen = Gen{};
    while (!gen.is_done(context)) {
      accum += gen(context);
    }
    return accum;
  }
};

// iteration operators
template <class Func, class Start, class End, class Accum = impl::add_identity>
struct reduce_range {
  template <class Context>
  constexpr eval_dtype operator()(const Context& context) {
    IS_EXPR(Start);
    IS_EXPR(End);
    IS_EXPR(Accum);
    eval_dtype accum = Accum{}(context);
    int64_t start = static_cast<int64_t>(Start{}(context));
    int64_t end = static_cast<int64_t>(End{}(context));
    for (int64_t i = start; i < end; ++i) {
      accum = Func{}(context).run(static_cast<eval_dtype>(i), accum);
    }
    return accum;
  }
};
