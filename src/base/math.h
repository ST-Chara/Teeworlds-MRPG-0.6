/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <algorithm>
#include <cmath>
#include <cstdlib>

constexpr float pi = 3.1415926535897932384626433f;

template <typename T>
constexpr inline T clamp(T val, T min, T max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

constexpr inline int round_to_int(float f)
{
	return f > 0 ? (int)(f + 0.5f) : (int)(f - 0.5f);
}

constexpr inline int round_truncate(float f)
{
	return (int)f;
}

template<typename T, typename TB>
constexpr inline T mix(const T a, const T b, TB amount)
{
	return a + (b - a) * amount;
}

template<typename T, typename TB>
inline T bezier(const T p0, const T p1, const T p2, const T p3, TB amount)
{
	// De-Casteljau Algorithm
	const T c10 = mix(p0, p1, amount);
	const T c11 = mix(p1, p2, amount);
	const T c12 = mix(p2, p3, amount);

	const T c20 = mix(c10, c11, amount);
	const T c21 = mix(c11, c12, amount);

	return mix(c20, c21, amount); // c30
}

inline float random_float()
{
	return rand() / (float)(RAND_MAX);
}

inline float random_float(float min, float max)
{
	return min + random_float() * (max - min);
}

inline float random_float(float max)
{
	return random_float(0.0f, max);
}

inline float centrelized_frandom(float center, float range) { return (center - range) + (random_float(range * 2.0f)); }

inline float random_angle()
{
	return 2.0f * pi * (rand() / std::nextafter((float)RAND_MAX, std::numeric_limits<float>::max()));
}

constexpr int fxpscale = 1 << 10;

// float to fixed
constexpr inline int f2fx(float v)
{
	return round_to_int(v * fxpscale);
}
constexpr inline float fx2f(int v)
{
	return v / (float)fxpscale;
}

// int to fixed
constexpr inline int i2fx(int v)
{
	return v * fxpscale;
}
constexpr inline int fx2i(int v)
{
	return v / fxpscale;
}

inline unsigned long long computeExperience(unsigned Level)
{
	if(Level == 1)
		return 18;
	return Level * (static_cast<unsigned long long>(Level) - 1) * 24;
}

class fxp
{
	int value;

public:
	void set(int v)
	{
		value = v;
	}
	int get() const
	{
		return value;
	}
	fxp &operator=(int v)
	{
		value = i2fx(v);
		return *this;
	}
	fxp &operator=(float v)
	{
		value = f2fx(v);
		return *this;
	}
	operator int() const
	{
		return fx2i(value);
	}
	operator float() const
	{
		return fx2f(value);
	}
};

template<typename T>
constexpr inline T minimum(T a, T b)
{
	return std::min(a, b);
}
template<typename T>
constexpr inline T minimum(T a, T b, T c)
{
	return std::min(std::min(a, b), c);
}
template<typename T>
constexpr inline T maximum(T a, T b)
{
	return std::max(a, b);
}
template<typename T>
constexpr inline T maximum(T a, T b, T c)
{
	return std::max(std::max(a, b), c);
}
template<typename T>
constexpr inline T absolute(T a)
{
	return a < T(0) ? -a : a;
}

template<typename T>
constexpr inline T in_range(T a, T lower, T upper)
{
	return lower <= a && a <= upper;
}
template<typename T>
constexpr inline T in_range(T a, T upper)
{
	return in_range(a, 0, upper);
}

// percents
template < typename T> // char is arithmetic type we must exclude it 'a' / 'd' etc
using PercentArithmetic = typename std::enable_if < std::is_arithmetic  < T >::value && !std::is_same < T, char >::value, T >::type;

template <typename T> // derive from the number of percent e.g. ((100, 10%) = 10)
PercentArithmetic<T> translate_to_percent_rest(T value, float percent) { return (T)(((double)value / 100.0f) * (T)percent); }

template <typename T> // add to the number a percentage e.g. ((100, 10%) = 110)
PercentArithmetic<T> add_percent_to_source(T* pvalue, float percent)
{
	*pvalue = ((T)((double)*pvalue) * (1.0f + ((T)percent / 100.0f)));
	return (T)(*pvalue);
}

template <typename T> // translate from the first to the second in percent e.g. ((10, 5) = 50%)
PercentArithmetic<T> translate_to_percent(T from, T value) { return (T)(((double)value / (double)from) * 100.0f); }

template <typename T> // translate from the first to the second in percent e.g. ((10, 5, 50) = 25%)
PercentArithmetic<T> translate_to_percent(T from, T value, float maximum_percent) { return (T)(((double)value / (double)from) * maximum_percent); }

// translate to commas
template<typename integer, const char separator = ',', const unsigned num = 3>
static std::enable_if_t<std::is_integral_v<integer>, std::string > get_commas(integer Number)
{
	std::string NumberString = std::to_string(Number);

	if(NumberString.length() > num)
	{
		for(auto it = NumberString.rbegin(); (num + 1) <= std::distance(it, NumberString.rend());)
		{
			std::advance(it, num);
			NumberString.insert(it.base(), separator);
		}
	}

	return NumberString;
}

#endif // BASE_MATH_H
