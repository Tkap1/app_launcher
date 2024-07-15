

#define ui_scoped_layer__(a) s_scoped_layer_destructor ___sld##a; ui_push_layer(false)
#define ui_scoped_layer_(a) ui_scoped_layer__(a)
#define ui_scoped_layer() ui_scoped_layer_(__LINE__)

enum e_string_input_result
{
	e_string_input_result_none,
	e_string_input_result_cancel,
	e_string_input_result_clear,
	e_string_input_result_submit,
};

struct s_ui_result
{
	b8 checked;
	b8 hovered;
	b8 pressed;
	b8 clicked;
	b8 submitted;
	b8 canceled;
	s_str<64> input;
};

struct s_ui_layer
{
	b8 blocking;
};

template <int max_chars>
struct s_input_str
{
	b8 initialized;
	float last_edit_time;
	float last_action_time;
	int cursor;
	int len;
	s_v2 cursor_visual_pos;
	float last_input_time;
	char data[max_chars + 1];

	void from_cstr(char* in)
	{
		int in_len = (int)strlen(in);
		assert(in_len < sizeof(data));
		memcpy(data, in, in_len);
		len = in_len;
		data[len] = 0;
	}

	void clear()
	{
		cursor = -1;
		len = 0;
	}
};

struct s_ui_data
{
	// @Note(tkap, 24/04/2023): Used by checkboxes
	b8 checked;

	s_str<64> input;
	int input_cursor = -1;
	float last_input_time;

	s_v2 size;
	s_v4 color;
};


struct s_ui_id
{
	u32 id;
	int layer;
};

struct s_ui
{
	s_sarray<s_ui_layer, 16> layer_arr;
	s_ui_id hovered;
	s_ui_id hovered_this_frame;
	s_ui_id pressed;
	s_ui_id pressed_this_frame;
	s_ui_id active;
	s_ui_id active_this_frame;
	s_hashtable<s_ui_data> table;
};

func void ui_pop_layer();
func int ui_get_layer_level();

struct s_scoped_layer_destructor
{
	~s_scoped_layer_destructor()
	{
		ui_pop_layer();
	}
};