#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

struct fvector {
    float x, y, z;
    fvector() : x(0), y(0), z(0) {}
    fvector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    fvector operator+(const fvector& o) const { return { x + o.x, y + o.y, z + o.z }; }
    fvector operator-(const fvector& o) const { return { x - o.x, y - o.y, z - o.z }; }
    fvector operator*(float s) const { return { x * s, y * s, z * s }; }
    float dot(const fvector& o) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float distance(const fvector& o) const { return (*this - o).length(); }
};

struct fvector2d {
    float x, y;
    fvector2d() : x(0), y(0) {}
    fvector2d(float _x, float _y) : x(_x), y(_y) {}
    fvector2d operator+(const fvector2d& o) const { return { x + o.x, y + o.y }; }
    fvector2d operator-(const fvector2d& o) const { return { x - o.x, y - o.y }; }
    float length() const { return std::sqrt(x * x + y * y); }
};

struct frotator {
    float pitch, yaw, roll;
    frotator() : pitch(0), yaw(0), roll(0) {}
    frotator(float p, float y, float r) : pitch(p), yaw(y), roll(r) {}
};

struct fquat {
    float x, y, z, w;
};

struct ftransform {
    fquat   rotation;
    fvector translation;
    float   pad0;
    fvector scale;
    float   pad1;
};
static_assert(sizeof(ftransform) == 0x30, "ftransform size mismatch");

struct fmatrix {
    float m[4][4];
};

inline fmatrix mat_mul(const fmatrix& a, const fmatrix& b) {
    fmatrix out{};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.m[i][j] =
                a.m[i][0] * b.m[0][j] +
                a.m[i][1] * b.m[1][j] +
                a.m[i][2] * b.m[2][j] +
                a.m[i][3] * b.m[3][j];
        }
    }
    return out;
}

inline fmatrix transform_to_matrix_with_scale(const ftransform& t) {
    fmatrix m{};
    m.m[3][0] = t.translation.x;
    m.m[3][1] = t.translation.y;
    m.m[3][2] = t.translation.z;

    float x2 = t.rotation.x + t.rotation.x;
    float y2 = t.rotation.y + t.rotation.y;
    float z2 = t.rotation.z + t.rotation.z;
    float xx2 = t.rotation.x * x2;
    float yy2 = t.rotation.y * y2;
    float zz2 = t.rotation.z * z2;

    m.m[0][0] = (1.0f - (yy2 + zz2)) * t.scale.x;
    m.m[1][1] = (1.0f - (xx2 + zz2)) * t.scale.y;
    m.m[2][2] = (1.0f - (xx2 + yy2)) * t.scale.z;

    float yz2 = t.rotation.y * z2;
    float wx2 = t.rotation.w * x2;
    m.m[2][1] = (yz2 - wx2) * t.scale.z;
    m.m[1][2] = (yz2 + wx2) * t.scale.y;

    float xy2 = t.rotation.x * y2;
    float wz2 = t.rotation.w * z2;
    m.m[1][0] = (xy2 - wz2) * t.scale.y;
    m.m[0][1] = (xy2 + wz2) * t.scale.x;

    float xz2 = t.rotation.x * z2;
    float wy2 = t.rotation.w * y2;
    m.m[2][0] = (xz2 + wy2) * t.scale.z;
    m.m[0][2] = (xz2 - wy2) * t.scale.x;

    m.m[0][3] = 0.f;
    m.m[1][3] = 0.f;
    m.m[2][3] = 0.f;
    m.m[3][3] = 1.f;
    return m;
}

inline fmatrix rotator_to_matrix(const frotator& r) {
    constexpr float d2r = 3.14159265358979323846f / 180.f;
    float sp = sinf(r.pitch * d2r), cp = cosf(r.pitch * d2r);
    float sy = sinf(r.yaw   * d2r), cy = cosf(r.yaw   * d2r);
    float sr = sinf(r.roll  * d2r), cr = cosf(r.roll  * d2r);
    fmatrix m{};
    m.m[0][0] =  cp * cy;
    m.m[0][1] =  cp * sy;
    m.m[0][2] =  sp;
    m.m[0][3] =  0.f;
    m.m[1][0] =  sr * sp * cy - cr * sy;
    m.m[1][1] =  sr * sp * sy + cr * cy;
    m.m[1][2] = -sr * cp;
    m.m[1][3] =  0.f;
    m.m[2][0] = -(cr * sp * cy + sr * sy);
    m.m[2][1] =  cy * sr - cr * sp * sy;
    m.m[2][2] =  cr * cp;
    m.m[2][3] =  0.f;
    m.m[3][0] =  0.f;
    m.m[3][1] =  0.f;
    m.m[3][2] =  0.f;
    m.m[3][3] =  1.f;
    return m;
}

inline float normalize_axis(float a) {
    a = fmodf(a, 360.f);
    if (a > 180.f) a -= 360.f;
    if (a < -180.f) a += 360.f;
    return a;
}

inline frotator quat_to_rotator(const fquat& q) {
    constexpr float r2d = 180.f / 3.14159265358979323846f;
    float singularity = q.z * q.x - q.w * q.y;
    float yaw_y = 2.f * (q.w * q.z + q.x * q.y);
    float yaw_x = 1.f - 2.f * (q.y * q.y + q.z * q.z);
    frotator r;
    r.yaw = atan2f(yaw_y, yaw_x) * r2d;
    if (singularity < -0.4999995f) {
        r.pitch = -90.f;
        r.roll = normalize_axis(-r.yaw - 2.f * atan2f(q.x, q.w) * r2d);
    } else if (singularity > 0.4999995f) {
        r.pitch = 90.f;
        r.roll = normalize_axis(r.yaw - 2.f * atan2f(q.x, q.w) * r2d);
    } else {
        r.pitch = asinf(2.f * singularity) * r2d;
        r.roll = atan2f(-2.f * (q.w * q.x + q.y * q.z), 1.f - 2.f * (q.x * q.x + q.y * q.y)) * r2d;
    }
    return r;
}

struct flinearcolor {
    float r, g, b, a;
    flinearcolor() : r(0), g(0), b(0), a(0) {}
    flinearcolor(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
};

template <class T>
struct tarray {
    uint64_t data;
    int32_t count;
    int32_t max;
};

struct fname {
    uint32_t comparison_index;
    uint32_t number;
};
