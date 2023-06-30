// 2023 by Stephan Roslen

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "GenericTypeErasure.h"

namespace TypeErasureDetail::Shape {

struct ShapeTECommon {
 protected:
  template<typename ShapeT>
  ShapeTECommon(const std::type_identity<ShapeT>)
      : mDrawFunction{createDrawFunction<ShapeT>()}, mDrawFreeFunction(createDrawFreeFunction<ShapeT>()),
        mSetNameFunction(createSetNameFunction<ShapeT>()), mSetNameFreeFunction(createSetNameFreeFunction<ShapeT>()) {}

  ShapeTECommon(const ShapeTECommon&) = default;
  ShapeTECommon(ShapeTECommon&&)      = default;

  ShapeTECommon& operator=(const ShapeTECommon&) = default;
  ShapeTECommon& operator=(ShapeTECommon&&)      = default;

  using DrawFunctionSignature     = void (*)(const void*);
  using DrawFreeFunctionSignature = void (*)(const void*);

  using SetNameFunctionSignature     = void (*)(void*, std::string);
  using SetNameFreeFunctionSignature = void (*)(void*, std::string);

  template<typename ShapeT>
  static DrawFunctionSignature createDrawFunction() {
    static const DrawFunctionSignature fn{[](const void* ptr) { static_cast<const ShapeT*>(ptr)->draw(); }};
    return fn;
  }

  template<typename ShapeT>
  static DrawFreeFunctionSignature createDrawFreeFunction() {
    static const DrawFreeFunctionSignature fn{[](const void* ptr) { draw(*static_cast<const ShapeT*>(ptr)); }};
    return fn;
  }

  template<typename ShapeT>
  static SetNameFunctionSignature createSetNameFunction() {
    static const SetNameFunctionSignature fn{
        [](void* ptr, std::string value) { static_cast<ShapeT*>(ptr)->setName(std::move(value)); }};
    return fn;
  }

  template<typename ShapeT>
  static SetNameFreeFunctionSignature createSetNameFreeFunction() {
    static const SetNameFreeFunctionSignature fn{
        [](void* ptr, std::string value) { setName(*static_cast<ShapeT*>(ptr), std::move(value)); }};
    return fn;
  }

  TypeErasureDetail::Generic::FunctionPtr<DrawFunctionSignature> mDrawFunction;
  TypeErasureDetail::Generic::FunctionPtr<DrawFreeFunctionSignature> mDrawFreeFunction;

  TypeErasureDetail::Generic::FunctionPtr<SetNameFunctionSignature> mSetNameFunction;
  TypeErasureDetail::Generic::FunctionPtr<SetNameFreeFunctionSignature> mSetNameFreeFunction;
};

template<TypeErasureDetail::Generic::TypeErasureSetup setup>
struct ShapeTE;

template<typename T>
concept IsShapeTE =
    std::is_same_v<std::remove_cvref_t<T>, ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::Owning>>
    || std::is_same_v<std::remove_cvref_t<T>, ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::Ref>>
    || std::is_same_v<std::remove_cvref_t<T>, ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::ConstRef>>;

template<typename T>
concept IsNoShapeTE = !IsShapeTE<T>;

template<TypeErasureDetail::Generic::TypeErasureSetup setup>
struct ShapeTE : TypeErasureDetail::Generic::BaseTE<setup>, ShapeTECommon {
 private:
  using Base   = TypeErasureDetail::Generic::BaseTE<setup>;
  using Common = ShapeTECommon;

  using Base::constPropagation;
  using Base::rawPtrAccess;

 public:
  template<IsNoShapeTE ShapeT>
  ShapeTE(ShapeT&& shape)
      : Base{std::forward<ShapeT>(shape)}, Common{std::type_identity<std::remove_cvref_t<ShapeT>>{}} {}

  ShapeTE(const ShapeTE&) = default;
  ShapeTE(ShapeTE&&)      = default;

  ShapeTE& operator=(const ShapeTE&) = default;
  ShapeTE& operator=(ShapeTE&&)      = default;

  void draw() const { mDrawFunction(rawPtrAccess()); }
  friend void draw(const ShapeTE& shape) { shape.mDrawFreeFunction(shape.rawPtrAccess()); }

  void setName(std::string value)
    requires(constPropagation != Generic::ConstPropagation::AlwaysConst)
  {
    mSetNameFunction(rawPtrAccess(), std::move(value));
  }
  friend void setName(ShapeTE& shape, std::string value)
    requires(constPropagation != Generic::ConstPropagation::AlwaysConst)
  {
    shape.mSetNameFreeFunction(shape.rawPtrAccess(), std::move(value));
  }

  void setName(std::string value) const
    requires(constPropagation == Generic::ConstPropagation::Bypass)
  {
    mSetNameFunction(rawPtrAccess(), std::move(value));
  }
  friend void setName(const ShapeTE& shape, std::string value)
    requires(constPropagation != Generic::ConstPropagation::Bypass)
  {
    shape.mSetNameFreeFunction(shape.rawPtrAccess(), std::move(value));
  }
};

} // namespace TypeErasureDetail::Shape

using Shape         = TypeErasureDetail::Shape::ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::Owning>;
using ShapeRef      = TypeErasureDetail::Shape::ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::Ref>;
using ShapeConstRef = TypeErasureDetail::Shape::ShapeTE<TypeErasureDetail::Generic::TypeErasureSetup::ConstRef>;
