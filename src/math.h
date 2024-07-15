
global constexpr float c_pi = 3.14159265359f;

struct s_v2i
{
	int x;
	int y;
};

template <typename T>
func s_v2i v2i(T x, T y)
{
	return {(int)x, (int)y};
}

template <typename T>
func s_v2i v2i(T v)
{
	return {(int)v, (int)v};
}


func s_v2 operator-(s_v2 a, s_v2 b)
{
	return {
		a.x - b.x,
		a.y - b.y
	};
}

func s_v2 operator*(s_v2 a, s_v2 b)
{
	return {
		a.x * b.x,
		a.y * b.y
	};
}

func s_v2 operator/(s_v2 a, float b)
{
	return {
		a.x / b,
		a.y / b
	};
}

func s_v2 operator+=(s_v2& a, s_v2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

func s_v2 operator-=(s_v2& a, s_v2 b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

func s_v2 operator*=(s_v2& a, s_v2 b)
{
	a.x *= b.x;
	a.y *= b.y;
}

func s_v2 operator*=(s_v2& a, float b)
{
	a.x *= b;
	a.y *= b;
}

func float sinf2(float t)
{
	return sinf(t) * 0.5f + 0.5f;
}

func float sin_range(float t, float min_val, float max_val)
{
	return min_val + sinf2(t) * (max_val - min_val);
}

func float v2_length_squared(s_v2 v)
{
	return v.x * v.x + v.y * v.y;
}

func float v2_length(s_v2 v)
{
	return sqrtf(v2_length_squared(v));
}

func float v2_distance(s_v2 a, s_v2 b)
{
	s_v2 c = a - b;
	return v2_length(c);
}

func float v2_distance_squared(s_v2 a, s_v2 b)
{
	s_v2 c;
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	return v2_length_squared(c);
}

func s_v2 v2_normalized(s_v2 v)
{
	s_v2 result = v;
	float length = v2_length(v);
	if(!floats_equal(length, 0))
	{
		result.x /= length;
		result.y /= length;
	}
	return result;
}

func void add_clamp(float* val, float min_val, float max_val, float to_add)
{
	*val = clamp(*val + to_add, min_val, max_val);
}


func float handle_advanced_easing(float x, float x_start, float x_end, float duration)
{
	assert(duration > 0);
	assert(duration <= 1);
	x = at_most(1.0f, x / duration);
	return ilerp(x_start, x_end, x);
}

func float ease_in_expo(float x)
{
	if(floats_equal(x, 0)) { return 0; }
	return powf(2, 10 * x - 10);
}

func float ease_in_quad(float x)
{
	return x * x;
}

func float ease_out_quad(float x)
{
	float x2 = 1 - x;
	return 1 - x2 * x2;
}

func float ease_out_expo(float x)
{
	if(floats_equal(x, 1)) { return 1; }
	return 1 - powf(2, -10 * x);
}

func float ease_out_elastic(float x)
{
	constexpr float c4 = (2 * c_pi) / 3;
	if(floats_equal(x, 0) || floats_equal(x, 1)) { return x; }
	return powf(2, -5 * x) * sinf((x * 5 - 0.75f) * c4) + 1;
}

func float ease_out_elastic2(float x)
{
	constexpr float c4 = (2 * c_pi) / 3;
	if(floats_equal(x, 0) || floats_equal(x, 1)) { return x; }
	return powf(2, -10 * x) * sinf((x * 10 - 0.75f) * c4) + 1;
}

#define m_advanced_easings \
X(ease_in_expo) \
X(ease_in_quad) \
X(ease_out_quad) \
X(ease_out_expo) \
X(ease_out_elastic) \
X(ease_out_elastic2) \

#define X(name) \
func float name##_advanced(float x, float x_start, float x_end, float target_start, float target_end, float duration) \
{ \
	x = handle_advanced_easing(x, x_start, x_end, duration); \
	return lerp(target_start, target_end, name(x)); \
}
m_advanced_easings
#undef X
