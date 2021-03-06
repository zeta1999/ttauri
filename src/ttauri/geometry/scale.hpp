// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "matrix.hpp"
#include "identity.hpp"
#include "translate.hpp"

namespace tt {
namespace geo {

template<int D>
class scale {
public:
    static_assert(D == 2 || D == 3, "Only 2D or 3D scale-matrices are supported");

    constexpr scale(scale const &) noexcept = default;
    constexpr scale(scale &&) noexcept = default;
    constexpr scale &operator=(scale const &) noexcept = default;
    constexpr scale &operator=(scale &&) noexcept = default;

    [[nodiscard]] constexpr explicit operator f32x4() const noexcept
    {
        tt_axiom(is_valid());
        return _v;
    }

    [[nodiscard]] constexpr explicit scale(f32x4 const &v) noexcept : _v(v)
    {
        tt_axiom(is_valid());
    }

    [[nodiscard]] constexpr operator matrix<D>() const noexcept
    {
        tt_axiom(is_valid());
        return matrix<D>{_v.x000(), _v._0y00(), _v._00z0(), _v._000w()};
    }

    [[nodiscard]] constexpr scale() noexcept : _v(1.0, 1.0, 1.0, 1.0) {}

    [[nodiscard]] constexpr scale(identity const &) noexcept : _v(1.0, 1.0, 1.0, 1.0) {}

    [[nodiscard]] constexpr scale(float value) noexcept requires(D == 2) : _v(value, value, 1.0, 1.0) {}

    [[nodiscard]] constexpr scale(float value) noexcept requires(D == 3) : _v(value, value, value, 1.0) {}

    [[nodiscard]] constexpr scale(float x, float y) noexcept requires(D == 2) : _v(x, y, 1.0, 1.0) {}

    [[nodiscard]] constexpr scale(float x, float y, float z = 1.0) noexcept requires(D == 3) : _v(x, y, z, 1.0) {}

    /** Get a uniform-scale-transform to scale an extent to another extent.
     * @param src_extent The extent to transform
     * @param dst_extent The extent to scale to.
     * @return a scale to transform the src_extent to the dst_extent.
     */
    template<int E, int F>
    requires(E <= D && F <= D) [[nodiscard]] static constexpr scale uniform(vector<E> src_extent, vector<F> dst_extent) noexcept
    {
        tt_axiom(dst_extent.x() != 0.0f && src_extent.x() != 0.0f);
        tt_axiom(dst_extent.y() != 0.0f && src_extent.y() != 0.0f);

        if constexpr (D == 2) {
            ttlet non_uniform_scale = static_cast<f32x4>(dst_extent).xyxy() / static_cast<f32x4>(src_extent).xyxy();
            ttlet uniform_scale = std::min(non_uniform_scale.x(), non_uniform_scale.y());
            return scale{uniform_scale};

        } else if constexpr (D == 3) {
            tt_axiom(dst_extent.z() != 0.0f && src_extent.z() != 0.0f);
            ttlet non_uniform_scale = static_cast<f32x4>(dst_extent).xyzx() / static_cast<f32x4>(src_extent).xyzx();
            ttlet uniform_scale = std::min({non_uniform_scale.x(), non_uniform_scale.y(), non_uniform_scale.z()});
            return scale{uniform_scale};

        } else {
            tt_static_no_default();
        }
    }

    [[nodiscard]] constexpr f32x4 operator*(f32x4 const &rhs) const noexcept
    {
        tt_axiom(is_valid());
        return _v * rhs;
    }

    template<int E>
    [[nodiscard]] constexpr vector<E> operator*(vector<E> const &rhs) const noexcept
    {
        tt_axiom(is_valid() && rhs.is_valid());
        return vector<E>{_v * static_cast<f32x4>(rhs)};
    }

    template<int E>
    [[nodiscard]] constexpr point<E> operator*(point<E> const &rhs) const noexcept
    {
        tt_axiom(is_valid() && rhs.is_valid());
        return point<E>{_v * static_cast<f32x4>(rhs)};
    }

    [[nodiscard]] constexpr aarect operator*(aarect const &rhs) const noexcept requires(D == 2)
    {
        return aarect::p0p3(_v * rhs.p0(), _v * rhs.p3());
    }

    [[nodiscard]] constexpr rect operator*(rect const &rhs) const noexcept
    {
        return rect{_v * rhs.corner<0>(), _v * rhs.corner<1>(), _v * rhs.corner<2>(), _v * rhs.corner<3>()};
    }

    [[nodiscard]] constexpr scale operator*(identity const &) const noexcept
    {
        tt_axiom(is_valid());
        return *this;
    }

    template<int E>
    [[nodiscard]] constexpr auto operator*(scale<E> const &rhs) const noexcept
    {
        tt_axiom(is_valid() && rhs.is_valid());
        return scale<std::max(D, E)>{_v * static_cast<f32x4>(rhs)};
    }

    template<int E>
    [[nodiscard]] constexpr bool operator==(scale<E> const &rhs) const noexcept
    {
        tt_axiom(is_valid() && rhs.is_valid());
        return {_v == static_cast<f32x4>(rhs)};
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return _v.w() == 1.0f && (D == 3 || _v.z() == 1.0f);
    }

private:
    f32x4 _v;
};

template<int D>
[[nodiscard]] constexpr matrix<D> matrix<D>::uniform(aarect src_rectangle, aarect dst_rectangle, alignment alignment) noexcept
{
    ttlet scale = tt::geo::scale<D>::uniform(vector2{src_rectangle.extent()}, vector2{dst_rectangle.extent()});
    ttlet scaled_rectangle = scale * src_rectangle;
    ttlet translation = translate<D>::align(scaled_rectangle, dst_rectangle, alignment);
    return translation * scale;
}


} // namespace geo

using scale2 = geo::scale<2>;
using scale3 = geo::scale<3>;

} // namespace tt
