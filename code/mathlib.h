/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h

#include <stdexcept> // For std::out_of_range

typedef float vec_t;

class vec3_t {
public:
	float x, y, z;

	// Default constructor
	vec3_t() : x(0), y(0), z(0) {}

	vec3_t(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}

	// Constructor from float array
	explicit vec3_t(const float* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}

	// Array accessor
	float& operator[](int index) {
		switch (index) {
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default: throw std::out_of_range("Index out of range");
		}
	}

	float* Ptr() const {
		return (float *) &x;
	}

	// Vector addition
	vec3_t operator+(const vec3_t & other) const {
		return vec3_t(x + other.x, y + other.y, z + other.z);
	}

	// Scalar addition
	vec3_t operator+(float scalar) const {
		return vec3_t(x + scalar, y + scalar, z + scalar );
	}

	// Vector multiplication
	vec3_t operator*(const vec3_t & other) const {
		return vec3_t(x * other.x, y * other.y, z * other.z );
	}

	// Scalar multiplication
	vec3_t operator*(float scalar) const {
		return vec3_t(x * scalar, y * scalar, z * scalar );
	}

	// Conversion operator to float*
	operator float* () {
		return &x; // Return address of x, which is the first element
	}

	// If const correctness is a concern, provide a const version as well
	operator const float* () const {
		return &x; // Return address of x, but for const contexts
	}
};

class vec5_t {
public:
	float x, y, z, v, w;

	// Array accessor
	float& operator[](int index) {
		switch (index) {
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return v;
		case 4: return w;
		default: throw std::out_of_range("Index out of range");
		}
	}

	// Vector addition
	vec5_t operator+(const vec5_t& other) const {
		return { x + other.x, y + other.y, z + other.z, v + other.v, w + other.w };
	}

	// Scalar addition
	vec5_t operator+(float scalar) const {
		return { x + scalar, y + scalar, z + scalar, v + scalar, w + scalar };
	}

	// Vector multiplication
	vec5_t operator*(const vec5_t& other) const {
		return { x * other.x, y * other.y, z * other.z, v * other.v, w * other.w };
	}

	// Scalar multiplication
	vec5_t operator*(float scalar) const {
		return { x * scalar, y * scalar, z * scalar, v * scalar, w * scalar };
	}
};

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

struct mplane_s;

extern vec3_t vec3_origin;
extern	int nanmask;

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}

void VectorMA(const vec3_t& veca, float scale, const vec3_t& vecb, vec3_t& vecc);

vec_t _DotProduct (const vec3_t & v1, const vec3_t & v2);
void _VectorSubtract(const vec3_t& veca, const vec3_t& vecb, vec3_t& out);
void _VectorAdd (const vec3_t & veca, const vec3_t & vecb, vec3_t & out);
void _VectorCopy (const vec3_t & in, vec3_t &out);

void PerpendicularVector(vec3_t& dst, const vec3_t& src);
int VectorCompare (const vec3_t & v1, const vec3_t & v2);
vec_t Length (float *v);
void CrossProduct (const vec3_t & v1, const vec3_t & v2, vec3_t & cross);
float VectorNormalize (vec3_t &v);		// returns vector length
void VectorInverse (vec3_t &v);
void VectorScale (const vec3_t & in, vec_t scale, vec3_t & out);
int Q_log2(int val);

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod (double numer, double denom, int *quotient,
		int *rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor (int i1, int i2);

void AngleVectors(const vec3_t & angles, vec3_t & forward, vec3_t & right, vec3_t & up);
int BoxOnPlaneSide (const vec3_t & emins, const vec3_t & emaxs, struct mplane_s *plane);
float	anglemod(float a);

void RotatePointAroundVector(vec3_t& dst, const vec3_t & dir, const vec3_t & point, float degrees);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))
