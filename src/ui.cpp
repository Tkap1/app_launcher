
// @TODO(tkap, 24/04/2023): Remove elements not drawn from the hashtable

func void ui_request_hovered(s_ui_id uid)
{
	if(ui.pressed_this_frame.id && ui.pressed_this_frame.layer >= uid.layer) { return; }
	if(ui.active_this_frame.id && ui.active_this_frame.layer >= uid.layer) { return; }
	ui.hovered_this_frame = uid;
	ui.pressed_this_frame = zero;
	ui.active_this_frame = zero;
}

func void ui_request_pressed(s_ui_id uid)
{
	if(ui.active_this_frame.id && ui.active_this_frame.layer >= uid.layer) { return; }
	ui.hovered_this_frame = zero;
	ui.pressed_this_frame = uid;
	ui.active_this_frame = zero;
}

func void ui_request_active(s_ui_id uid, b8 active_persists_across_frames)
{
	// @TODO(tkap, 24/04/2023): Handle different layers?
	unreferenced(uid);
	ui.hovered_this_frame = zero;
	ui.pressed_this_frame = zero;
	if(active_persists_across_frames)
	{
		ui.active_this_frame = uid;
	}
}

func b8 handle_ui_interaction(s_ui_id uid, s_v2 pos, s_v2 size, b8 active_persists_across_frames, b8 hover_only)
{
	b8 result = false;
	b8 hovered = point_in_rect_topleft(mouse, pos, size);
	if(hovered)
	{
		ui_request_hovered(uid);
	}

	if(!hover_only)
	{
		if(uid.id == ui.hovered.id)
		{
			if(hovered && is_key_pressed(left_button))
			{
				ui_request_pressed(uid);
			}
		}

		if(uid.id == ui.pressed.id)
		{
			if(is_key_released(left_button))
			{
				if(hovered)
				{
					ui_request_active(uid, active_persists_across_frames);
					result = true;
				}
				else
				{
					ui_request_pressed(zero);
				}
			}
		}
	}
	return result;
}

func void handle_ui_lerping(u32 id, b8 clicked, s_v2 in_size, s_v4 in_color, s_v2* out_size, s_v4* out_color)
{
	if(ui.hovered.id == id)
	{
		*out_color = lerp_color(*out_color, brighter(in_color, 1.2f), 0.2f);
		if(out_size)
		{
			*out_size = lerp(*out_size, in_size * 1.1f, 0.2f);
		}
	}
	else
	{
		*out_color = lerp_color(*out_color, in_color, 0.2f);
		if(out_size)
		{
			*out_size = lerp(*out_size, in_size, 0.2f);
		}
	}
	if(ui.pressed.id == id)
	{
		*out_color = lerp_color(*out_color, darker(in_color, 1.2f), 0.2f);
	}

	if(clicked)
	{
		*out_color = v4(0.5f, 1.0f, 0.5f);
	}
}

func s_ui_result ui_button(char* id_str, s_v2 pos, s_v2 in_size)
{
	s_ui_result result = zero;

	u32 id = hash(id_str);
	int layer = ui_get_layer_level();
	s_ui_id uid = {.id = id, .layer = layer};

	s_ui_data* data = ui.table.get(id_str);
	if(!data)
	{
		s_ui_data temp = zero;
		temp.size = in_size;
		temp.color = c_button_color;
		data = ui.table.add(id_str, temp);
	}

	s_v2 text_pos = pos + in_size / 2;
	b8 active = handle_ui_interaction(uid, pos, in_size, false, false);
	handle_ui_lerping(id, active, in_size, c_button_color, &data->size, &data->color);
	pos -= (data->size - in_size) / 2;

	draw_rect(pos, layer, data->size, data->color, {.origin_offset = c_origin_topleft});
	draw_text(id_str, text_pos, layer, c_text_color, e_font_medium, true, {.sublayer = 1});

	result.hovered = ui.hovered.id == id;
	result.pressed = ui.pressed.id == id;
	result.clicked = active;

	return result;
}

func s_ui_result ui_label(char* id_str, s_v2 pos)
{
	s_ui_result result = zero;

	u32 id = hash(id_str);
	int layer = ui_get_layer_level();
	s_ui_id uid = {.id = id, .layer = layer};

	s_ui_data* data = ui.table.get(id_str);
	if(!data)
	{
		s_ui_data temp = zero;
		temp.color = c_text_color;
		data = ui.table.add(id_str, temp);
	}

	s_v2 size = get_text_size(id_str, e_font_medium);
	b8 active = handle_ui_interaction(uid, pos, size, false, true);
	handle_ui_lerping(id, active, size, c_button_color, &data->size, &data->color);

	draw_text(id_str, pos, layer, c_text_color, e_font_medium, true);

	result.hovered = ui.hovered.id == id;
	result.pressed = ui.pressed.id == id;
	result.clicked = active;

	return result;
}

func s_ui_result ui_input_box(char* id_str, s_v2 pos, s_v2 in_size, char* default_input, s_ui_data optional = zero)
{
	s_ui_result result = zero;

	u32 id = hash(id_str);
	int layer = ui_get_layer_level();
	s_ui_id uid = {.id = id, .layer = layer};

	s_ui_data* data = ui.table.get(id_str);
	if(!data)
	{
		s_ui_data temp = optional;
		temp.size = in_size;
		temp.color = c_button_color;
		if(default_input)
		{
			temp.input.from_cstr(default_input);
		}
		data = ui.table.add(id_str, temp);
	}

	float font_size = g_font_arr[e_font_medium].size;
	s_v2 text_pos = pos;
	text_pos.y += in_size.y / 2;
	text_pos.y -= font_size / 2;
	b8 active = handle_ui_interaction(uid, pos, in_size, true, false);
	handle_ui_lerping(id, active, in_size, c_button_color, null, &data->color);
	pos -= (data->size - in_size) / 2;

	if(id == ui.active.id)
	{
		if(data->input_cursor == -1)
		{
			data->input_cursor = data->input.len;
		}
		while(true)
		{
			s_char_event event = get_char_event();
			if(!event.c) { break; }
			data->last_input_time = total_time;
			if(event.is_symbol)
			{
				if(event.c == VK_LEFT)
				{
					data->input_cursor = at_least(0, data->input_cursor - 1);
				}
				else if(event.c == VK_RIGHT)
				{
					data->input_cursor = at_most(data->input.len, data->input_cursor + 1);
				}
				else if(event.c == VK_HOME)
				{
					data->input_cursor = 0;
				}
				else if(event.c == VK_END)
				{
					data->input_cursor = data->input.len;
				}
				else if(event.c == VK_DELETE)
				{
					int num_chars_right = data->input.len - data->input_cursor;
					if(num_chars_right > 0)
					{
						memmove(&data->input.data[data->input_cursor], &data->input.data[data->input_cursor + 1], num_chars_right);
						data->input.data[--data->input.len] = 0;
					}
				}
				else if(event.c == VK_ESCAPE)
				{
					ui.active_this_frame = zero;
					result.canceled = true;
					break;
				}
			}
			else
			{
				if(event.c < 128 && data->input.len < data->input.get_max_chars())
				{
					if(event.c == '\r')
					{
						ui.active_this_frame = zero;
						result.submitted = true;
						break;
					}
					else if(event.c == '\b')
					{
						if(data->input_cursor > 0)
						{
							data->input_cursor--;
							int to_copy = data->input.len - data->input_cursor;
							memmove(&data->input.data[data->input_cursor], &data->input.data[data->input_cursor + 1], to_copy);
							data->input.data[--data->input.len] = 0;
						}
					}
					else
					{
						int num_chars_right = data->input.len - data->input_cursor;
						if(num_chars_right > 0)
						{
							memmove(&data->input.data[data->input_cursor + 1], &data->input.data[data->input_cursor], num_chars_right);
						}
						data->input.data[data->input_cursor++] = (char)event.c;
						data->input.data[++data->input.len] = 0;
					}
				}
			}
		}

		s_v2 cursor_pos = text_pos;
		if(data->input.len > 0)
		{
			s_v2 text_size = get_text_size_with_count(data->input.data, e_font_medium, data->input_cursor);
			cursor_pos.x += text_size.x;
		}
		float cursor_width = 11;
		float diff = cursor_pos.x + cursor_width - (pos.x + in_size.x);
		if(diff > 0)
		{
			cursor_pos.x -= diff;
			text_pos.x -= diff;
		}
		float time_since_last_input = fabsf(data->last_input_time - total_time);
		if(fmodf(total_time, 1) > 0.5f || time_since_last_input <= 1)
		{
			float v = 0;
			if(time_since_last_input <= 1)
			{
				v = 1.0f - powf(time_since_last_input, 0.5f);
			}
			draw_rect(cursor_pos, layer, v2(cursor_width, font_size), v4(v, 1.0f, v), {.sublayer = 2, .origin_offset = c_origin_topleft});
		}
	}

	draw_rect(pos, layer, data->size, data->color, {.origin_offset = c_origin_topleft});
	if(data->input.len > 0)
	{
		draw_text(data->input.data, text_pos, layer, c_text_color, e_font_medium, false, {.do_clip = true, .sublayer = 1, .clip_pos = pos, .clip_size = data->size});
	}

	result.hovered = ui.hovered.id == id;
	result.pressed = ui.pressed.id == id;
	result.clicked = active;
	result.input = data->input;

	return result;
}

func s_ui_result ui_checkbox(char* id_str, s_v2 pos, s_v2 in_size)
{
	s_ui_result result = zero;

	u32 id = hash(id_str);
	int layer = ui_get_layer_level();
	s_ui_id uid = {.id = id, .layer = layer};

	s_ui_data* data = ui.table.get(id_str);
	if(!data)
	{
		s_ui_data temp = zero;
		temp.size = in_size;
		temp.color = c_button_color;
		data = ui.table.add(id_str, temp);
	}

	s_v2 text_pos = pos + in_size / 2;
	b8 active = handle_ui_interaction(uid, pos, in_size, false, false);
	handle_ui_lerping(id, active, in_size, c_button_color, &data->size, &data->color);
	pos -= (data->size - in_size) / 2;

	if(active)
	{
		data->checked = !data->checked;
	}

	draw_rect(pos, layer, data->size, data->color, {.origin_offset = c_origin_topleft});

	if(data->checked)
	{
		draw_texture(pos, layer, data->size, v4(1), {.sublayer = 1, .origin_offset = c_origin_topleft});
	}

	result.checked = data->checked;
	result.hovered = ui.hovered.id == id;
	result.pressed = ui.pressed.id == id;
	result.clicked = active;

	return result;
}

func void soft_reset_ui()
{
	ui.hovered = zero;
	if(ui.hovered_this_frame.id)
	{
		ui.hovered = ui.hovered_this_frame;
	}
	ui.pressed = ui.pressed_this_frame;
	ui.active = ui.active_this_frame;
	ui.hovered_this_frame = zero;
	assert(ui.layer_arr.count == 0 && "There must be a pop_layer missing");
}

func void ui_push_layer(b8 blocking)
{
	s_ui_layer layer = zero;
	layer.blocking = blocking;
	ui.layer_arr.add(layer);
}

func void ui_pop_layer()
{
	ui.layer_arr.pop();
}

func int ui_get_layer_level()
{
	return ui.layer_arr.count;
}

template <int max_chars>
func e_string_input_result handle_string_input(s_input_str<max_chars>* input, b8* out_was_edited, b8* out_any_action)
{
	b8 was_edited = false;
	b8 any_action = false;
	e_string_input_result result = e_string_input_result_none;
	if(input->cursor == -1) {
		input->cursor = input->len;
	}
	while(true) {
		s_char_event event = get_char_event();
		if(!event.c) { break; }
		input->last_input_time = total_time;
		if(event.is_symbol) {
			if(event.c == VK_LEFT) {
				input->cursor = at_least(0, input->cursor - 1);
				any_action = true;
			}
			else if(event.c == VK_RIGHT) {
				input->cursor = at_most(input->len, input->cursor + 1);
				any_action = true;
			}
			else if(event.c == VK_HOME) {
				input->cursor = 0;
				any_action = true;
			}
			else if(event.c == VK_END) {
				input->cursor = input->len;
				any_action = true;
			}
			else if(event.c == VK_DELETE) {
				int num_chars_right = input->len - input->cursor;
				if(num_chars_right > 0) {
					was_edited = true;
					memmove(&input->data[input->cursor], &input->data[input->cursor + 1], num_chars_right);
					input->data[--input->len] = 0;
				}
			}
			else if(event.c == VK_ESCAPE) {
				if(input->len > 0) {
					was_edited = true;
					result = e_string_input_result_clear;
				}
				else {
					result = e_string_input_result_cancel;
				}
				input->data[0] = 0;
				input->len = 0;
				input->cursor = -1;
				break;
			}
			else if(event.c == VK_RETURN) {
				result = e_string_input_result_submit;
				break;
			}
		}
		else {
			if(event.c < 128 && input->len < max_chars) {
				if(event.c == '\r') {
					ui.active_this_frame = zero;
					break;
				}
				else if(event.c == '\t') {
				}
				else if(event.c == '\b') {
					if(input->cursor > 0) {
						was_edited = true;
						input->cursor--;
						int to_copy = input->len - input->cursor;
						memmove(&input->data[input->cursor], &input->data[input->cursor + 1], to_copy);
						input->data[--input->len] = 0;
					}
				}
				else {
					was_edited = true;
					int num_chars_right = input->len - input->cursor;
					if(num_chars_right > 0)
					{
						memmove(&input->data[input->cursor + 1], &input->data[input->cursor], num_chars_right);
					}
					input->data[input->cursor++] = (char)event.c;
					input->data[++input->len] = 0;
				}
			}
		}
	}
	any_action = any_action || was_edited;
	if(out_was_edited) { *out_was_edited = was_edited; }
	if(out_any_action) { *out_any_action = any_action; }
	float time = (float)get_seconds_since_start();
	if(was_edited) {
		input->last_edit_time = time;
	}
	if(any_action) {
		input->last_action_time = time;
	}
	return result;
}

template <int max_chars>
func s_input_str<max_chars> make_input_str()
{
	s_input_str<max_chars> result = zero;
	result.cursor = -1;
	return result;
}
