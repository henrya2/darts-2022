#pragma once

#include <darts/common.h>

// Three vectors forming an orthonormal basis in 3D
template <typename T>
struct ONB
{
    // Three ortho-normal vectors that form the basis for a local coordinate system
    Vec3<T> s; // The tangent vector
    Vec3<T> t; // The bi-tangent vector
    Vec3<T> n; // The normal vector

    ONB() = default;

    /*
        Build an ONB from a single vector.

        Sets ONB::n to a normalized version of \p n_ and computes ONB::s and ONB::t automatically to form a right-handed
        orthonormal basis
    */
    ONB(const Vec3<T> n_)
    {
        n = n_;
        std::tie(s, t) = coordinate_system(n);
    }

    /*
        Initialize an ONB from a surface tangent \p s and normal \p n.

        \param [in] s_   Surface tangent
        \param [in] n_   Surface normal
    */
    ONB(const Vec3<T> &s_, const Vec3<T> &n_)
    {
        s = s_;
        n = n_;
        t = cross(n, s);
    }

    // Initialize an ONB from three orthonormal vectors
    ONB(const Vec3<T> &s, const Vec3<T> &t, const Vec3<T> &n)
        : s(s)
        , t(t)
        , n(n)
    {

    }

    // Convert from world coordinates to local coordinates
    Vec3<T> to_local(const Vec3<T> &v) const
    {
        return la::mul(la::transpose(Mat33<T>(s, t, n)), v);
    }

    // Convert from local coordinates to world coordinates
    Vec3<T> to_world(const Vec3<T> &v) const
    {
        return la::mul(Mat33<T>(s, t, n), v);
    }
};

using ONBf = ONB<float>;
using ONBd = ONB<double>;