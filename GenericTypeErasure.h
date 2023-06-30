// 2023 by Stephan Roslen

#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace TypeErasureDetail::Generic {

enum class TypeErasureSetup { Owning, Ref, ConstRef };
enum class ConstPropagation { Propagate, Bypass, AlwaysConst };

consteval ConstPropagation deriveConstPropagation(const TypeErasureSetup setup) {
  using enum TypeErasureSetup;
  using enum ConstPropagation;
  switch (setup) {
  case Owning: return Propagate;
  case Ref: return Bypass;
  case ConstRef: return AlwaysConst;
  }
}

struct Nothing {
};

template<typename Signature, Signature defaultValue = nullptr>
struct FunctionPtr {
  FunctionPtr(Signature ptr) : mPtr{ptr} {};

  FunctionPtr(const FunctionPtr&) = default;
  FunctionPtr(FunctionPtr&& other) noexcept : mPtr{std::exchange(other.mPtr, defaultValue)} {}

  FunctionPtr& operator=(const FunctionPtr&) = default;
  FunctionPtr& operator=(FunctionPtr&& other) noexcept {
    if (std::addressof(other) == this) {
      return *this;
    }
    mPtr = std::exchange(other.mPtr, defaultValue);
    return *this;
  }

  template<typename... Ts>
  decltype(auto) operator()(Ts&&... ts) const {
    if (!mPtr) {
      throw std::bad_function_call{};
    }
    return mPtr(std::forward<Ts>(ts)...);
  }

 private:
  Signature mPtr;
};

template<TypeErasureSetup setup>
consteval auto typeErasureSetupPtrTypeHelper(const std::integral_constant<TypeErasureSetup, setup>) {
  if constexpr (setup == TypeErasureSetup::Owning) {
    return std::type_identity<std::unique_ptr<void, void (*)(const void*)>>{};
  } else if constexpr (setup == TypeErasureSetup::ConstRef) {
    return std::type_identity<const void*>{};
  } else {
    static_assert(setup == TypeErasureSetup::Ref);
    return std::type_identity<void*>{};
  }
}

template<TypeErasureSetup setup>
using TypeErasureSetupPtrType =
    typename decltype(typeErasureSetupPtrTypeHelper(std::integral_constant<TypeErasureSetup, setup>{}))::type;

template<TypeErasureSetup setup, bool constAccess>
consteval auto typeErasureSetupAccessPtrTypeHelper(const std::integral_constant<TypeErasureSetup, setup>,
                                                   const std::bool_constant<constAccess>) {
  if constexpr (setup == TypeErasureSetup::Owning) {
    if constexpr (constAccess) {
      return std::type_identity<const std::unique_ptr<const void, void (*)(const void*)>&>{};
    } else {
      return std::type_identity<const std::unique_ptr<void, void (*)(const void*)>&>{};
    }
  } else if constexpr (setup == TypeErasureSetup::ConstRef || constAccess) {
    return std::type_identity<const void*>{};
  } else {
    static_assert(setup == TypeErasureSetup::Ref && !constAccess);
    return std::type_identity<void*>{};
  }
}

template<TypeErasureSetup setup, bool constAccess>
using TypeErasureSetupAccessPtrType =
    typename decltype(typeErasureSetupAccessPtrTypeHelper(std::integral_constant<TypeErasureSetup, setup>{},
                                                          std::bool_constant<constAccess>{}))::type;

using OwningPtrType = std::unique_ptr<void, void (*)(const void*)>;

inline OwningPtrType noopDuplicator(const OwningPtrType&) {
  return OwningPtrType{nullptr, [](const void*) {}};
}

template<TypeErasureSetup setup>
struct BaseTE {
 protected:
  template<typename TypeT>
  BaseTE(TypeT&& value)
    requires(setup == TypeErasureSetup::Owning)
      : mData{new std::remove_cvref_t<TypeT>(std::forward<TypeT>(value)), createDeleter<std::remove_cvref_t<TypeT>>()},
        mDuplicator{createDuplicator<std::remove_cvref_t<TypeT>>()} {}

  template<typename TypeT>
  BaseTE(TypeT& value)
    requires(setup != TypeErasureSetup::Owning)
      : mData{std::addressof(value)}, mDuplicator{} {}

  BaseTE(const BaseTE&)
    requires(setup != TypeErasureSetup::Owning)
  = default;
  BaseTE(const BaseTE& other)
    requires(setup == TypeErasureSetup::Owning)
      : mData{other.mDuplicator(other.mData)}, mDuplicator{other.mDuplicator} {}
  BaseTE(BaseTE&& other) = default;

  BaseTE& operator=(const BaseTE&)
    requires(setup != TypeErasureSetup::Owning)
  = default;
  BaseTE& operator=(const BaseTE& other)
    requires(setup == TypeErasureSetup::Owning)
  {
    if (std::addressof(other) == this) {
      return *this;
    }
    mDuplicator = other.mDuplicator;
    mData       = mDuplicator(other.mData);
    return *this;
  }

  BaseTE& operator=(BaseTE&&)
    requires(setup != TypeErasureSetup::Owning)
  = default;
  BaseTE& operator=(BaseTE&& other) = default;

  constexpr static ConstPropagation constPropagation = deriveConstPropagation(setup);

  void* rawPtrAccess()
    requires(constPropagation == ConstPropagation::Propagate)
  {
    return mData.get();
  }

  const void* rawPtrAccess() const
    requires(constPropagation == ConstPropagation::Propagate)
  {
    return mData.get();
  }

  const void* rawPtrAccess() const
    requires(constPropagation == ConstPropagation::AlwaysConst)
  {
    return mData;
  }

  void* rawPtrAccess() const
    requires(constPropagation == ConstPropagation::Bypass)
  {
    return mData;
  }

  using UsedPtrType         = TypeErasureSetupPtrType<setup>;
  using DuplicatorSignature = OwningPtrType (*)(const OwningPtrType&);

  using DuplicatorType =
      std::conditional_t<setup == TypeErasureSetup::Owning, FunctionPtr<DuplicatorSignature, noopDuplicator>, Nothing>;

  template<typename TypeT>
  static constexpr DuplicatorSignature createDuplicator()
    requires(setup == TypeErasureSetup::Owning)
  {
    return [](const OwningPtrType& ptr) {
      return OwningPtrType{new TypeT(*static_cast<TypeT*>(ptr.get())), ptr.get_deleter()};
    };
  }

  template<typename TypeT>
  static constexpr auto createDeleter()
    requires(setup == TypeErasureSetup::Owning)
  {
    return [](const void* ptr) { delete static_cast<const TypeT*>(ptr); };
  }

  UsedPtrType mData;
  [[no_unique_address]] DuplicatorType mDuplicator;
};

} // namespace TypeErasureDetail::Generic