#include "vec3.h"

#pragma once

struct Quaternion {
    float x, y, z, w;

    inline Quaternion operator+(Quaternion q) { return { x + q.x, y + q.y, z + q.z, w + q.w }; }
    inline Quaternion operator-(Quaternion q) { return { x - q.x, y - q.y, z - q.z, w - q.w }; }
    inline Quaternion operator*(float s) { return { x * s, y * s, z * s, w * s }; }
    inline Quaternion operator/(float s) { return { x / s, y / s, z / s, w / s }; }

    inline float length() { return sqrtf(x * x + y * y + z * z + w * w); }
    inline float lengthSq() { return x * x + y * y + z * z + w * w; }

    inline Quaternion inverse() {
        float invSq = 1 / this->lengthSq();
        return { -x * invSq, -y * invSq, -z * invSq, w * invSq };
    }

    inline Quaternion conjugate() {
        return { -x, -y, -z, w };
    }

    inline Quaternion unit() {
        float invLen = 1 / this->length();
        return { x * invLen, y * invLen, z * invLen, w * invLen };
    }

    inline float angle() {
        return acosf(w) * 2.0f;
    }

    inline Vec3 axis() {
        Vec3 axis = {x, y, z};
        return axis.unit();
    }

    static inline Quaternion fromAxisAngle(Vec3 axis, float angle) {
        Vec3 v = axis * sinf(angle / 2.0f);
        float s = cosf(angle / 2.0f);
        return { v.x, v.y, v.z, s };
    }

    Quaternion operator*(Quaternion q) {
        float a1 = w;   float b1 = x;   float c1 = y;   float d1 = z;
        float a2 = q.w; float b2 = q.x; float c2 = q.y; float d2 = q.z;
        return {
            a1 * b2 + b1 * a2 + c1 * d2 - d1 * c2,
            a1 * c2 - b1 * d2 + c1 * a2 + d1 * b2,
            a1 * d2 + b1 * c2 - c2 * b2 + d1 * a2,
            a1 * a2 - b1 * b2 - c1 * c2 - d1 * d2,
        };
    }

    inline Quaternion lerp(Quaternion target, float t) {
        return *this + (target - *this) * t;
    }

    inline Quaternion lerpAndNormalize(Quaternion target, float t) {
        return this->lerp(target, t).unit();
    }

    // Assumes both quaternions are normalized.
    Quaternion slerp(Quaternion target, float t) {
        Quaternion rel = target * this->conjugate();
        float angle = rel.angle() * t;
        if (angle < 0.1f || isnan(angle))
            return this->lerpAndNormalize(target, t);
        return Quaternion::fromAxisAngle(rel.axis(), angle) * *this;
    }

};
