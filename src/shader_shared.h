

#ifdef m_app
	#define shader_v2 alignas(8) s_v2
	#define shader_v4 alignas(16) s_v4
	#define shader_float alignas(4) float
	#define shader_int alignas(4) int
	#define shader_bool alignas(4) b32
#else
	#define shader_v2 vec2
	#define shader_v4 vec4
	#define shader_float float
	#define shader_int int
	#define shader_bool bool
	#define m_equals(val)
#endif


struct s_transform
{
	shader_bool do_clip;
	shader_bool use_texture;
	shader_bool flip_x;
	shader_int layer;
	shader_int sublayer;
	shader_float mix_weight;
	shader_v2 pos;
	shader_v2 origin_offset;
	shader_v2 draw_size;
	shader_v2 texture_size;
	shader_v2 clip_pos;
	shader_v2 clip_size;
	shader_v2 uv_min;
	shader_v2 uv_max;
	shader_v4 color;
	shader_v4 mix_color m_equals(v4(1));
};

