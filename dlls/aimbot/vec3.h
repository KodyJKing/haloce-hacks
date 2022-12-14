#pragma once

typedef struct { float yaw, pitch; } Angles;

struct Vec3 { 
    float x, y, z;
    inline Vec3 operator+(Vec3 v) { return { x + v.x, y + v.y, z + v.z }; }
    inline Vec3 operator-(Vec3 v) { return { x - v.x, y - v.y, z - v.z }; }
    inline Vec3 operator*(float s) { return { x * s, y * s, z * s }; }
    inline Vec3 operator/(float s) { return { x / s, y / s, z / s }; }

    inline float length() { return sqrtf(x * x + y * y + z * z); }
    inline float dot(Vec3 v) { return x * v.x + y * v.y + z * v.z; }
    inline Vec3 cross(Vec3 v) {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x,
        };
    }
    inline Vec3 unit() {
        float invLen = 1.0f / (this->length());
        return { x * invLen, y * invLen, z * invLen };
    }

    inline Vec3 lerp(Vec3 v, float t) {
        return *this * (1 - t) + v * t;
    }

    inline Angles toAngles() {

        float r = sqrtf(x * x + y * y);
        return {
            atan2f(y, x),
            atan2f(z, r)
        };
    }

    void print() {
        printf("< %f.4, %f.4, %f.4 >", x, y, z);
    }
};
