#include <stdio.h>
#include <stdlib.h>

#include "MathGeoLib.h"
#include "myassert.h"
#include "TestRunner.h"
#include <cmath>

#if __cplusplus >= 201103L
TEST(CXX11StdFinite)
{
	// When using MathGeoLib, users should still be able to invoke C++11 std::isfinite function.
	// http://en.cppreference.com/w/cpp/numeric/math/isfinite
	assert(std::isfinite(5.f));
	assert(std::isfinite(5.0));

	using namespace std;

	assert(isfinite(5.f));
	assert(isfinite(5.0));


#ifndef EMSCRIPTEN // long double is not supported.
	assert(std::isfinite(5.0L));
	assert(isfinite(5.0L));
#endif

}
#endif

TEST(IsFinite)
{
	assert(IsFinite(5));
	assert(IsFinite(5.f));
	assert(IsFinite(5.0));
#ifndef EMSCRIPTEN // long double is not supported.
	assert(IsFinite(5.0L));
#endif

	assert(!IsFinite(FLOAT_NAN));
	assert(!IsFinite(FLOAT_INF));
	assert(IsFinite(FLT_MAX));

	assert(!IsFinite((double)FLOAT_NAN));
	assert(!IsFinite((double)FLOAT_INF));
	assert(IsFinite((double)FLT_MAX));

#ifndef EMSCRIPTEN // long double is not supported.
	assert(!IsFinite((long double)FLOAT_NAN));
	assert(!IsFinite((long double)FLOAT_INF));
	assert(IsFinite((long double)FLT_MAX));
#endif
}

TEST(IsNan)
{
	assert(!IsNan(5.f));
	assert(!IsNan(5.0));
#ifndef EMSCRIPTEN // long double is not supported.
	assert(!IsNan(5.0L));
#endif

	assert(IsNan(FLOAT_NAN));
	assert(!IsNan(FLOAT_INF));
	assert(!IsNan(FLT_MAX));

	assert(IsNan((double)FLOAT_NAN));
	assert(!IsNan((double)FLOAT_INF));
	assert(!IsNan((double)FLT_MAX));

#ifndef EMSCRIPTEN // long double is not supported.
	assert(IsNan((long double)FLOAT_NAN));
	assert(!IsNan((long double)FLOAT_INF));
	assert(!IsNan((long double)FLT_MAX));
#endif
}

TEST(IsInf)
{
	assert(!IsInf(5.f));
	assert(!IsInf(5.0));
#ifndef EMSCRIPTEN // long double is not supported.
	assert(!IsInf(5.0L));
#endif

	assert(!IsInf(FLOAT_NAN));
	assert(IsInf(FLOAT_INF));
	assert(IsInf(-FLOAT_INF));
	assert(!IsInf(FLT_MAX));

	assert(!IsInf((double)FLOAT_NAN));
	assert(IsInf((double)FLOAT_INF));
	assert(IsInf(-(double)FLOAT_INF));
	assert(!IsInf((double)FLT_MAX));

#ifndef EMSCRIPTEN // long double is not supported.
	assert(!IsInf((long double)FLOAT_NAN));
	assert(IsInf((long double)FLOAT_INF));
	assert(IsInf(-(long double)FLOAT_INF));
	assert(!IsInf((long double)FLT_MAX));
#endif
}

TEST(ReinterpretAsInt)
{
	assert(ReinterpretAsInt(0.0f) == 0x00000000);
	assert(ReinterpretAsInt(1.0f) == 0x3F800000);
	assert(ReinterpretAsInt(2.0f) == 0x40000000);
	assert(ReinterpretAsInt(-1.0f) == 0xBF800000);
	assert(ReinterpretAsInt(FLOAT_INF) == 0x7F800000);
}

TEST(ReinterpretAsFloat)
{
	assert(ReinterpretAsFloat(0x00000000) == 0.0f);
	assert(ReinterpretAsFloat(0x3F800000) == 1.0f);
	assert(ReinterpretAsFloat(0x40000000) == 2.0f);
	assert(ReinterpretAsFloat(0xBF800000) == -1.0f);
	assert(ReinterpretAsFloat(0x7F800000) == FLOAT_INF);
	assert(IsNan(ReinterpretAsFloat(0x7F800001)));
}

// Idea: Since approx sqrt is so fast, run through that and do one manual Newton-Rhapson iteration to improve.
float NewtonRhapsonSqrt(float x)
{
	float estimate = SqrtFast(x);
	return estimate - (estimate*estimate - x) * 0.5f / estimate;
}

#ifdef MATH_SSE
float NewtonRhapsonSSESqrt(float x)
{
	__m128 X = FLOAT_TO_M128(x);
	__m128 estimate = _mm_rcp_ss(_mm_rsqrt_ss(X));
	__m128 e2 = _mm_mul_ss(estimate,estimate);
	__m128 half = _mm_set_ss(0.5f);
	__m128 recipEst = _mm_rcp_ss(estimate);

	return M128_TO_FLOAT(_mm_sub_ss(estimate, _mm_mul_ss(_mm_mul_ss((_mm_sub_ss(e2, X)), half), recipEst)));
}
#endif

float *PosFloatArray()
{
	LCG lcg;
	static float *arr;
	if (!arr)
	{
		arr = new float[testrunner_numItersPerTest+32];
		uintptr_t a = (uintptr_t)arr;
		a = (a + 31) & ~31;
		arr = (float*)a;
		for(int i = 0; i < testrunner_numItersPerTest; ++i)
			arr[i] = lcg.Float(0.f, 100000.f);
	}
	return arr;
}

float *pf = PosFloatArray();
extern float *f;

UNIQUE_TEST(sqrt_precision)
{
	float maxRelError1 = 0.f;
	float maxRelError2 = 0.f;
	float maxRelError3 = 0.f;
#ifdef MATH_SSE
	float maxRelError4 = 0.f;
#endif
	for(int i = 0; i < 1000000; ++i)
	{
		float f = rng.Float(0.f, 1e20f);
		float x = (float)sqrt((double)f); // best precision of the sqrt.

		float x1 = Sqrt(f);
		maxRelError1 = Max(RelativeError(x, x1), maxRelError1);

		float x2 = SqrtFast(f);
		maxRelError2 = Max(RelativeError(x, x2), maxRelError2);

		float x3 = NewtonRhapsonSqrt(f);
		maxRelError3 = Max(RelativeError(x, x3), maxRelError3);

#ifdef MATH_SSE
		float x4 = NewtonRhapsonSSESqrt(f);
		maxRelError4 = Max(RelativeError(x, x4), maxRelError4);
#endif
	}

	LOGI("Max relative error with Sqrt: %e", maxRelError1);
	assert(maxRelError1 < 1e-9f);
	LOGI("Max relative error with SqrtFast: %e", maxRelError2);
	assert(maxRelError2 < 1e-3f);
	LOGI("Max relative error with NewtonRhapsonSqrt: %e", maxRelError3);
	assert(maxRelError3 < 1e-6f);
#ifdef MATH_SSE
	LOGI("Max relative error with NewtonRhapsonSSESqrt: %e", maxRelError4);
	assert(maxRelError4 < 1e-6f);
#endif
}

BENCHMARK(sqrt_sqrtf)
{
	TIMER_BEGIN
	{
		f[i] = sqrtf(pf[i]);
	}
	TIMER_END
}

BENCHMARK(sqrt_Sqrt)
{
	TIMER_BEGIN
	{
		f[i] = Sqrt(pf[i]);
	}
	TIMER_END
}

BENCHMARK(sqrt_SqrtFast)
{
	TIMER_BEGIN
	{
		f[i] = SqrtFast(pf[i]);
	}
	TIMER_END
}

BENCHMARK(sqrt_NewtonRhapsonSqrt)
{
	TIMER_BEGIN
	{
		f[i] = NewtonRhapsonSqrt(pf[i]);
	}
	TIMER_END
}

#ifdef MATH_SSE
BENCHMARK(sqrt_NewtonRhapsonSSESqrt)
{
	TIMER_BEGIN
	{
		f[i] = NewtonRhapsonSSESqrt(pf[i]);
	}
	TIMER_END
}
#endif
