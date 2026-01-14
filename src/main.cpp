#include "tklib.h"

#include <dwmapi.h>
#include <xaudio2.h>

#pragma warning(push, 0)
#include <gl/GL.h>
#include "external/glcorearb.h"
#include "external/wglext.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT assert
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert assert
#include "external/stb_truetype.h"
#pragma warning(pop)

#if 0
#define malloc(a) malloc(a); printf("malloc %llu bytes at %s:%i\n", (u64)a, __FILE__, __LINE__);
#define calloc(a, b) calloc(a, b); printf("calloc %llu bytes at %s:%i\n", (u64)(a * b), __FILE__, __LINE__);
#define free(a) free(a); printf("free at %s:%i\n", __FILE__, __LINE__);
#endif

#include "config.h"
#define m_equals(...) = __VA_ARGS__
#include "shader_shared.h"
#include "math.h"
#include "hashtable.h"
#include "ui.h"
#include "draw.h"
#include "audio.h"
#include "main.h"
#include "json.h"

#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT) \
X(PFNWGLGETSWAPINTERVALEXTPROC, wglGetSwapIntervalEXT)

#define X(type, name) global type name = null;
m_gl_funcs
#undef X

global s_xaudio xaudio = zero;
global int g_width = 0;
global int g_height = 0;
global s_v2 g_window_size = c_base_res;
global s_v2 g_window_center = g_window_size * 0.5f;
global s_sarray<s_transform, 8192> transforms;
global s_input g_input = zero;
global float total_time = 0;
global float delta = 0;
global s_v2 mouse = zero;
global s_carray<s_font, e_font_count> g_font_arr;
global s_carray<s_sarray<s_transform, 8192>, e_font_count> text_arr;

global s_str<1024> thread_str;

global s_sarray<s_str<600>, 4> messages;
global s_lin_arena g_arena = zero;
global s_lin_arena g_frame_arena = zero;

global s_ui ui;
global s_sarray<s_char_event, 64> char_event_arr;

global HWND g_window;
global b8 g_window_shown = false;

global int app_state = 0;

global s_sarray<s_element, 128> g_elements;
global float g_element_times[128] = zero;

#include "ui.cpp"
#include "draw.cpp"
#include "hashtable.cpp"
#include "audio.cpp"
#include "json.cpp"




#ifdef m_no_console
int APIENTRY WinMain(HINSTANCE instance, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
#else
int main(int argc, char** argv)
#endif
{
	#ifdef m_no_console
	unreferenced(hInstPrev);
	unreferenced(cmdline);
	unreferenced(cmdshow);
	#else
	unreferenced(argc);
	unreferenced(argv);
	HINSTANCE instance = GetModuleHandle(null);
	#endif

	init_win32_time();

	// @Note(tkap, 14/05/2023): Prevent multiple instances of this program running at the same time
	CreateEvent(NULL, FALSE, FALSE, "app_launcher_event");
	if(GetLastError() == ERROR_ALREADY_EXISTS) {
		return 1;
	}

	char* class_name = "app_launcher";
	g_arena = make_lin_arena(20 * c_mb, true);
	g_frame_arena.capacity = 10 * c_mb;
	g_frame_arena.memory = (u8*)g_arena.get(g_frame_arena.capacity);

	init_audio(&g_arena);

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = null;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		dummy start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC;
		window_class.lpfnWndProc = DefWindowProc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		check(RegisterClassEx(&window_class));

		HWND window = CreateWindowEx(
			0,
			class_name,
			"TKCHAT",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			null,
			null,
			instance,
			null
		);
		assert(window != INVALID_HANDLE_VALUE);

		HDC dc = GetDC(window);

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int format = ChoosePixelFormat(dc, &pfd);
		check(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
		check(SetPixelFormat(dc, format, &pfd));

		HGLRC glrc = wglCreateContext(dc);
		check(wglMakeCurrent(dc, glrc));

		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load_gl_func("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load_gl_func("wglChoosePixelFormatARB");

		check(wglMakeCurrent(null, null));
		check(wglDeleteContext(glrc));
		check(ReleaseDC(window, dc));
		check(DestroyWindow(window));
		check(UnregisterClass(class_name, instance));

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		dummy end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	HDC dc = null;
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		window start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc = window_proc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		window_class.hCursor = LoadCursor(null, IDC_ARROW);

		check(RegisterClassEx(&window_class));

		g_window = CreateWindowEx(
			WS_EX_LAYERED,
			// WS_EX_LAYERED | WS_EX_TOPMOST,
			// WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
			// 0,
			class_name,
			"Search",
			// WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			// WS_POPUP | WS_VISIBLE,
			WS_POPUP,
			CW_USEDEFAULT, CW_USEDEFAULT, (int)c_base_res.x, (int)c_base_res.y,
			null,
			null,
			instance,
			null
		);
		assert(g_window != INVALID_HANDLE_VALUE);

		SetLayeredWindowAttributes(g_window, RGB(0, 0, 0), 0, LWA_COLORKEY);

		dc = GetDC(g_window);

		int pixel_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_TRANSPARENT_ARB, TRUE,
			WGL_COLOR_BITS_ARB, 24,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			0
		};

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		int format;
		u32 num_formats;
		check(wglChoosePixelFormatARB(dc, pixel_attribs, null, 1, &format, &num_formats));
		check(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
		SetPixelFormat(dc, format, &pfd);

		int gl_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			0
		};
		HGLRC glrc = wglCreateContextAttribsARB(dc, null, gl_attribs);
		check(wglMakeCurrent(dc, glrc));

		#define X(type, name) name = (type)load_gl_func(#name);
		m_gl_funcs
		#undef X

		glDebugMessageCallback(gl_debug_callback, null);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		window end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// stbi_set_flip_vertically_on_load(true);

	set_window_size_to_monitor_size(g_window, WS_POPUP);

	RegisterHotKey(g_window, 1, MOD_CONTROL | MOD_SHIFT, key_k);

	g_font_arr[e_font_small] = load_font("assets/consola.ttf", 24, &g_arena);
	g_font_arr[e_font_medium] = load_font("assets/consola.ttf", 36, &g_arena);
	g_font_arr[e_font_big] = load_font("assets/consola.ttf", 72, &g_arena);

	u32 vao;
	u32 ssbo;
	u32 program;
	{
		g_arena.push();
		u32 vertex = glCreateShader(GL_VERTEX_SHADER);
		u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
		char* header = "#version 430 core\n";
		char* vertex_src = read_file_quick("shaders/vertex.vertex", &g_arena);
		char* fragment_src = read_file_quick("shaders/fragment.fragment", &g_arena);
		char* vertex_src_arr[] = {header, read_file_quick("src/shader_shared.h", &g_arena), vertex_src};
		char* fragment_src_arr[] = {header, read_file_quick("src/shader_shared.h", &g_arena), fragment_src};
		glShaderSource(vertex, array_count(vertex_src_arr), vertex_src_arr, null);
		glShaderSource(fragment, array_count(fragment_src_arr), fragment_src_arr, null);
		glCompileShader(vertex);
		char buffer[1024] = zero;
		check_for_shader_errors(vertex, buffer);
		glCompileShader(fragment);
		check_for_shader_errors(fragment, buffer);
		program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);
		glUseProgram(program);
		g_arena.pop();
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);

	auto input_str = make_input_str<MAX_PATH - 1>();
	s_state1_data state1_data = zero;
	int element_selected = 0;
	int prev_element_selected = element_selected;

	e_font font_type = e_font_medium;
	float font_size = g_font_arr[font_type].size;
	s_v2 scroll_v = zero;

	MSG win_msg = zero;
	b8 running = true;

	wglSwapIntervalEXT(1);

	b8 read_json = true;

	f64 before = 0;
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		frame start start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	while(running) {

		{
			f64 now = get_seconds_since_start();
			delta = (float)(now - before);
			before = now;
			total_time += delta;
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		window messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		while(PeekMessageA(&win_msg, null, 0, 0, PM_REMOVE) != 0)
		// while(GetMessageA(&win_msg, null, 0, 0) != 0)
		{
			if(win_msg.message == WM_QUIT) {
				running = false;
				break;
			}
			else if(win_msg.message == WM_HOTKEY) {
				if(g_window_shown) {
					hide_window();
				}
				else {
					show_window();
				}
			}
			TranslateMessage(&win_msg);
			DispatchMessage(&win_msg);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		window messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		if(!g_window_shown) { Sleep(10); continue; };

		// if(is_key_pressed(key_f)) {
		// 	play_sound(xaudio.pop);
		// }

		if(read_json) {
			read_json = false;
			g_elements.count = 0;
			char* text = read_file_quick("data.json", &g_frame_arena);
			assert(text);
			s_json* json = parse_json(text, &g_arena);
			assert(json);
			json = json->object;
			for_json(j, json) {
				s_element e = zero;
				e.name = j->key;
				{
					s_json* temp = json_get(j->object, "type", e_json_string);
					if(temp) {
						e.type.add(temp->str);
					}
					else {
						temp = json_get(j->object, "type", e_json_array);
						if(temp) {
							for_json(j2, temp->array) {
								e.type.add(j2->str);
							}
						}
					}
				}
				assert(e.type.count > 0);

				{
					s_json* temp = json_get(j->object, "path", e_json_string);
					if(temp) {
						e.path.add(temp->str);
					}
					else {
						temp = json_get(j->object, "path", e_json_array);
						if(temp) {
							for_json(j2, temp->array) {
								e.path.add(j2->str);
							}
						}
					}
				}
				assert(e.path.count > 0);

				{
					s_json* temp = json_get(j->object, "working_dir", e_json_string);
					if(temp) {
						e.working_dir.add(temp->str);
					}
					else {
						temp = json_get(j->object, "working_dir", e_json_array);
						if(temp) {
							for_json(j2, temp->array) {
								e.working_dir.add(j2->str);
							}
						}
					}
				}

				{
					s_json* temp = json_get(j->object, "requires_input", e_json_bool);
					e.requires_input = temp && temp->bool_val;
				}
				g_elements.add(e);
			}
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		app update start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			e_string_input_result input_result = zero;
			e_string_input_result specific_app_input_result = zero;
			switch(app_state) {
				case 0: {
					input_result = handle_string_input(&input_str, null, null);
					if(input_result == e_string_input_result_cancel) {
						hide_window();
					}
					s_score_and_index* sorted_elements = (s_score_and_index*)g_frame_arena.get_zero(sizeof(s_element) * g_elements.count);
					int sorted_elements_count = 0;
					int min_score = 0;
					if(input_str.len > 0) { min_score = 1; }
					for(int i = 0; i < g_elements.count; i++) {
						int score = string_similarity(input_str.data, g_elements[i].name);
						if(score >= min_score) {
							arr_add(sorted_elements, {.score = score, .index = i}, &sorted_elements_count);
						}
					}
					qsort(sorted_elements, sorted_elements_count, sizeof(s_score_and_index), qsort_files);

					s_rect elements_rect = make_rect_points(0.25f, 0.15f, 0.75f, 0.95f, g_window_size);
					float big_font_size = g_font_arr[e_font_big].size;
					draw_search_bar2(&input_str, v2(elements_rect.center_x(), elements_rect.pos.y - big_font_size), e_font_big, 1);
					if(sorted_elements_count > 0) {
						if(is_key_pressed(key_down)) {
							element_selected += 1;
						}
						if(is_key_pressed(key_up)) {
							element_selected -= 1;
						}
					}
					element_selected = circular_index(element_selected, sorted_elements_count);

					if(sorted_elements_count > 0) {
						int temp = sorted_elements[element_selected].index;
						if(prev_element_selected != temp) {
							play_sound(xaudio.pop);
						}
						prev_element_selected = temp;
					}


					float element_spacing = font_size * 1.5f;
					float selected_pos = (element_selected + 1) * element_spacing + elements_rect.pos.y;
					float diff = selected_pos - (elements_rect.bottom() * 0.5f);
					int scroll = 0;
					if(diff > 0) {
						scroll = ceilfi(diff / element_spacing);
					}
					s_v2 target_scroll_v = v2(0.0f, scroll * element_spacing);
					scroll_v = lerp_snap(scroll_v, target_scroll_v, delta * 15, 1.0f);

					s_v2 element_pos = elements_rect.pos + elements_rect.size * v2(0.5f, 0.0f);
					if(sorted_elements_count > 0) {
						for(int ias_i = 0; ias_i < sorted_elements_count; ias_i += 1) {
							s_score_and_index ias = sorted_elements[ias_i];

							element_pos.y += element_spacing;
							s_element element = g_elements[ias.index];
							// s_v4 color = rgb(0xD56D3D);
							b8 selected = ias_i == element_selected;
							if(selected) {
								add_clamp(&g_element_times[ias.index], 0, 1, delta * 4);
								// color = brighter(rgb(0xF49D51), 1.2f);
								if(input_result == e_string_input_result_submit) {
									for(int step_i = 0; step_i < element.type.count; step_i += 1) {

										if(strcmp(element.type[step_i], "explorer") == 0) {
											if(element.requires_input) {
												app_state = 1;
												state1_data = zero;
												state1_data.element = element;
												state1_data.input_str = make_input_str<MAX_PATH - 1>();
											}
											else {
												input_str.clear();
												hide_window();
												char* result = format_text("explorer \"%s\"", element.path[step_i]);
												system(result);
											}
										}
										else if(strcmp(element.type[step_i], "run") == 0) {
											input_str.clear();
											hide_window();
											system(element.path[step_i]);
										}
										else if(strcmp(element.type[step_i], "run_in_folder") == 0) {
											input_str.clear();
											hide_window();
											char* result = format_text("start \"\" /d \"%s\" \"%s\"", element.working_dir[step_i], element.path[step_i]);
											system(result);
										}
									}
								}
							}
							else {
								add_clamp(&g_element_times[ias.index], 0, 1, -delta * 6);
							}

							s_value_interp<s_v4> color_interp[] = {
								{0.0f, rgb(0xD56D3D)},
								{0.33f, rgb(0xffffff)},
								{1.0f, brighter(rgb(0xD56D3D), 2.5f)},
							};

							s_value_interp<float> spacing_interp[] = {
								{0.0f, 0.0f},
								{0.33f, 1.75f},
								{1.0f, 1.5f},
							};
							s_v4 color = do_interp(g_element_times[ias.index], color_interp, array_count(color_interp));
							float spacing = do_interp(selected ? g_element_times[ias.index] : 0.0f, spacing_interp, array_count(spacing_interp));
							draw_text(
								element.name, element_pos - scroll_v, 2, color, font_type, true,
								{.do_clip = true, .clip_pos = elements_rect.pos, .clip_size = elements_rect.size},
								{.size_multiplier = spacing}
							);
						}
					}

				} break;

				case 1: {
					specific_app_input_result = handle_string_input(&state1_data.input_str, null, null);
					if(specific_app_input_result == e_string_input_result_cancel) {
						app_state = 0;
					}

					draw_text(
						state1_data.element.name, g_window_size * v2(0.5f, 0.1f), 2, brighter(rgb(0xD56D3D), 2.5f), e_font_big, true
					);

					s_v2 pos = g_window_center;
					s_v2 size = g_window_size * v2(0.5f, 0.5f);
					draw_search_bar(&state1_data.input_str, pos - v2(size.x * 0.5f, size.y * 0.5f + g_font_arr[e_font_medium].size), v2(size.x, g_font_arr[e_font_medium].size), e_font_medium, true, 3);
					if(specific_app_input_result == e_string_input_result_submit) {
						s_element e = state1_data.element;
						assert(e.type.count == 1);
						assert(strcmp(e.type[0], "explorer") == 0);
						char* temp = format_text(e.path[0], state1_data.input_str.data);
						char* result = format_text("explorer \"%s\"", temp);
						system(result);
						hide_window();
					}

				} break;
				invalid_default_case;
			}

		}

		soft_reset_ui();
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		app update end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		render start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			// glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, g_width, g_height);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_GREATER);

			int location = glGetUniformLocation(program, "window_size");
			glUniform2fv(location, 1, &g_window_size.x);

			if(transforms.count > 0)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*transforms.elements) * transforms.count, transforms.elements);
				glDrawArraysInstanced(GL_TRIANGLES, 0, 6, transforms.count);
				transforms.count = 0;
			}

			for(int font_i = 0; font_i < e_font_count; font_i++) {
				if(text_arr[font_i].count > 0) {
					s_font* font = &g_font_arr[font_i];
					glBindTexture(GL_TEXTURE_2D, font->texture.id);
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
					glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
					text_arr[font_i].count = 0;
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		render end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		for(int k_i = 0; k_i < c_max_keys; k_i++) {
			g_input.keys[k_i].count = 0;
		}

		char_event_arr.count = 0;

		g_frame_arena.used = 0;

		SwapBuffers(dc);

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		frame start end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	return 0;
}

LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch(msg)
	{

		case WM_CLOSE:
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;

		case WM_ACTIVATE:
		{
			DWORD status = LOWORD(wparam);
			if(status == WA_INACTIVE) {
				hide_window();
			}
		} break;

		case WM_CHAR:
		{
			char_event_arr.add({.is_symbol = false, .c = (int)wparam});
		} break;

		case WM_SIZE:
		{
			g_width = LOWORD(lparam);
			g_height = HIWORD(lparam);
			g_window_size = v2(g_width, g_height);
			g_window_center = g_window_size * 0.5f;
		} break;

		case WM_LBUTTONDOWN:
		{
			g_input.keys[left_button].is_down = true;
			g_input.keys[left_button].count += 1;
		} break;

		case WM_LBUTTONUP:
		{
			g_input.keys[left_button].is_down = false;
			g_input.keys[left_button].count += 1;
		} break;

		case WM_RBUTTONDOWN:
		{
			g_input.keys[right_button].is_down = true;
			g_input.keys[right_button].count += 1;
		} break;

		case WM_RBUTTONUP:
		{
			g_input.keys[right_button].is_down = false;
			g_input.keys[right_button].count += 1;
		} break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			int key = (int)remap_key_if_necessary(wparam, lparam);
			b8 is_down = !(bool)((HIWORD(lparam) >> 15) & 1);
			if(key < c_max_keys)
			{
				s_stored_input si = zero;
				si.key = key;
				si.is_down = is_down;
				apply_event_to_input(&g_input, si);
			}

			if(is_down)
			{
				char_event_arr.add({.is_symbol = true, .c = (int)wparam});
			}
		} break;

		default:
		{
			result = DefWindowProc(window, msg, wparam, lparam);
		}
	}

	return result;
}


func PROC load_gl_func(char* name)
{
	PROC result = wglGetProcAddress(name);
	if(!result)
	{
		printf("Failed to load %s\n", name);
		assert(false);
	}
	return result;
}

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	unreferenced(userParam);
	unreferenced(length);
	unreferenced(id);
	unreferenced(type);
	unreferenced(source);
	if(severity >= GL_DEBUG_SEVERITY_HIGH)
	{
		printf("GL ERROR: %s\n", message);
		assert(false);
	}
}

func void apply_event_to_input(s_input* in_input, s_stored_input event)
{
	in_input->keys[event.key].is_down = event.is_down;
	in_input->keys[event.key].count += 1;
}

func b8 check_for_shader_errors(u32 id, char* out_error)
{
	int compile_success;
	char info_log[1024];
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);

	if(!compile_success)
	{
		glGetShaderInfoLog(id, 1024, null, info_log);
		printf("Failed to compile shader:\n%s", info_log);

		if(out_error)
		{
			strcpy(out_error, info_log);
		}

		return false;
	}
	return true;
}


func s_v2 get_text_size_with_count(char* text, e_font font_id, int count)
{
	assert(count >= 0);
	if(count <= 0) { return zero; }
	s_font* font = &g_font_arr[font_id];

	s_v2 size = zero;
	size.y = font->size;

	for(int char_i = 0; char_i < count; char_i++)
	{
		char c = text[char_i];
		s_glyph glyph = font->glyph_arr[c];
		if(char_i == count - 1 && c != ' ')
		{
			size.x += glyph.width;
		}
		else
		{
			size.x += glyph.advance_width * font->scale;
		}
	}

	return size;
}

func s_v2 get_text_size(char* text, e_font font_id)
{
	return get_text_size_with_count(text, font_id, (int)strlen(text));
}

func s_font load_font(char* path, float font_size, s_lin_arena* arena)
{
	arena->push();

	s_font font = zero;
	font.size = font_size;

	u8* file_data = (u8*)read_file_quick(path, arena);
	assert(file_data);

	stbtt_fontinfo info = zero;
	stbtt_InitFont(&info, file_data, 0);

	stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);

	font.scale = stbtt_ScaleForPixelHeight(&info, font_size);
	constexpr int max_chars = 128;
	s_sarray<u8*, max_chars> bitmap_arr;
	constexpr int padding = 10;
	int total_width = padding;
	int total_height = 0;
	for(int char_i = 0; char_i < max_chars; char_i++) {
		s_glyph glyph = zero;
		u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
		stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, null);

		total_width += glyph.width + padding;
		total_height = max(glyph.height + padding * 2, total_height);

		font.glyph_arr[char_i] = glyph;
		bitmap_arr.add(bitmap);
	}

	u8* gl_bitmap = (u8*)arena->get_zero(sizeof(u8) * 4 * total_width * total_height);

	int current_x = padding;
	for(int char_i = 0; char_i < max_chars; char_i++) {
		s_glyph* glyph = &font.glyph_arr[char_i];
		u8* bitmap = bitmap_arr[char_i];
		for(int y = 0; y < glyph->height; y++) {
			for(int x = 0; x < glyph->width; x++) {
				u8 src_pixel = bitmap[x + y * glyph->width];
				u8* dst_pixel = &gl_bitmap[((current_x + x) + (padding + y) * total_width) * 4];
				dst_pixel[0] = src_pixel;
				dst_pixel[1] = src_pixel;
				dst_pixel[2] = src_pixel;
				dst_pixel[3] = src_pixel;
			}
		}

		glyph->uv_min.x = current_x / (float)total_width;
		glyph->uv_max.x = (current_x + glyph->width) / (float)total_width;

		glyph->uv_min.y = padding / (float)total_height;

		// @Note(tkap, 17/05/2023): For some reason uv_max.y is off by 1 pixel (checked the texture in renderoc), which causes the text to be slightly miss-positioned
		// in the Y axis. "glyph->height - 1" fixes it.
		glyph->uv_max.y = (padding + glyph->height - 1) / (float)total_height;

		// @Note(tkap, 17/05/2023): Otherwise the line above makes the text be cut off at the bottom by 1 pixel...
		glyph->uv_max.y += 0.01f;

		current_x += glyph->width + padding;
	}

	font.texture = load_texture_from_data(gl_bitmap, total_width, total_height, GL_LINEAR);

	foreach_val(bitmap_i, bitmap, bitmap_arr)
	{
		stbtt_FreeBitmap(bitmap, null);
	}

	arena->pop();

	return font;
}

func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering)
{
	assert(data);
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	s_texture texture = zero;
	texture.id = id;
	texture.size = v2(width, height);
	return texture;
}

func s_texture load_texture_from_file(char* path, u32 filtering)
{
	int width, height, num_channels;
	void* data = stbi_load(path, &width, &height, &num_channels, 4);
	s_texture texture = load_texture_from_data(data, width, height, filtering);
	stbi_image_free(data);
	return texture;
}

func b8 is_key_down(int key)
{
	assert(key < c_max_keys);
	return g_input.keys[key].is_down;
}

func b8 is_key_up(int key)
{
	assert(key < c_max_keys);
	return !g_input.keys[key].is_down;
}

func b8 is_key_pressed(int key)
{
	assert(key < c_max_keys);
	return (g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func b8 is_key_released(int key)
{
	assert(key < c_max_keys);
	return (!g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func s_char_event get_char_event()
{
	if(!char_event_arr.is_empty())
	{
		return char_event_arr.remove_and_shift(0);
	}
	return zero;
}

func int string_similarity(char* needle, char* haystack)
{
	int needle_len = (int)strlen(needle);
	int haystack_len = (int)strlen(haystack);

	if(needle_len <= 0) { return 0; }
	if(haystack_len <= 0) { return 0; }

	int score = 0;
	int sequence_bonus = 0;
	int needle_index = 0;
	int haystack_index = 0;
	int matches = 0;

	// @Note(tkap, 16/05/2023): Bonus if first character matches
	if(tolower(needle[0]) == tolower(haystack[0])) { score += 1; }

	while(needle_index < needle_len) {
		char needle_c = (char)tolower(needle[needle_index]);
		while(haystack_index < haystack_len)
		{
			char haystack_c = (char)tolower(haystack[haystack_index]);
			if(needle_c == haystack_c) {
				matches += 1;
				score += 2 + sequence_bonus;
				sequence_bonus += 7;
				needle_index += 1;
				haystack_index += 1;
				break;
			}
			else {
				haystack_index += 1;
				sequence_bonus = 0;
				score -= 1;
			}
		}
		if(haystack_index >= haystack_len) { break; }
	}

	assert(matches <= needle_len);

	if(needle_index >= needle_len && haystack_index < haystack_len) {
		score -= haystack_len - needle_len;
	}

	if(matches == needle_len) { score = at_least(1, score); }
	else if(matches != needle_len) { score = 0; }

	return score;
}

func void draw_search_bar(s_input_str<MAX_PATH - 1>* search, s_v2 pos, s_v2 size, e_font font_type, b8 do_cursor, int layer)
{
	float font_size = g_font_arr[font_type].size;
	draw_rect(pos, layer, v2(size.x, font_size), darker(rgb(0x6B4738), 1.2f), {.origin_offset = c_origin_topleft});
	pos.x += 4;

	if(search->len > 0) {
		draw_text(search->data, pos, layer + 1, rgb(0xCBA678), font_type, false);
	}

	// @Note(tkap, 14/05/2023): Draw cursor
	if(do_cursor) {
		if(search->cursor >= 0) {
			s_v2 text_size = get_text_size_with_count(search->data, font_type, search->cursor);
			s_v2 cursor_pos = v2(
				pos.x + text_size.x,
				pos.y
			);
			if(!search->initialized) {
				search->initialized = true;
				search->cursor_visual_pos = cursor_pos;
			}
			else {
				search->cursor_visual_pos = lerp_snap(search->cursor_visual_pos, cursor_pos, delta * 20, 1);
			}

			s_v2 cursor_size = v2(11.0f, g_font_arr[font_type].size);
			draw_rect(search->cursor_visual_pos, layer + 9, cursor_size, rgb(0xABC28F), {.origin_offset = c_origin_topleft});
		}
	}
}

func void draw_search_bar2(s_input_str<MAX_PATH - 1>* search, s_v2 pos, e_font font_type, int layer)
{
	float font_size = g_font_arr[font_type].size;

	if(search->len > 0) {
		draw_text(search->data, pos, layer + 1, rgb(0xCBA678), font_type, true);
	}

	if(search->cursor >= 0) {
		s_v2 full_text_size = get_text_size(search->data, font_type);
		s_v2 partial_text_size = get_text_size_with_count(search->data, font_type, search->cursor);
		s_v2 cursor_pos = v2(
			-full_text_size.x * 0.5f + pos.x + partial_text_size.x,
			pos.y - font_size * 0.5f
		);

		s_v2 cursor_size = v2(15.0f, font_size);
		float t = (float)get_seconds_since_start() - max(search->last_action_time, search->last_edit_time);
		b8 blink = false;
		constexpr float c_blink_rate = 0.75f;
		if(t > 0.75f && fmodf(t, c_blink_rate) >= c_blink_rate / 2) {
			blink = true;
		}
		float t2 = clamp((float)get_seconds_since_start() - search->last_edit_time, 0.0f, 1.0f);
		s_v4 color = lerp(rgb(0xffdddd), darker(rgb(0xABC28F), 1.25f), 1 - powf(1 - t2, 3));
		float extra_height = ease_out_elastic2_advanced(t2, 0, 1, 20, 0, 0.75f);
		cursor_size.y += extra_height;
		cursor_pos.y -= extra_height / 2;

		if(!search->initialized) {
			search->initialized = true;
			search->cursor_visual_pos = cursor_pos;
		}
		else {
			search->cursor_visual_pos = lerp_snap(search->cursor_visual_pos, cursor_pos, delta * 20, 1);
		}

		if(!blink) {
			draw_rect(search->cursor_visual_pos, layer + 9, cursor_size, color, {.origin_offset = c_origin_topleft});
		}
	}
}

DWORD WINAPI system_call(void* param)
{
	unreferenced(param);
	system(thread_str.data);
	return 0;
}

func void enable_blur_behind(HWND hwnd)
{
	DWM_BLURBEHIND bb = {0};

	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = true;
	bb.hRgnBlur = NULL;

	check(SUCCEEDED(DwmEnableBlurBehindWindow(hwnd, &bb)));

	typedef struct _WINCOMPATTRDATA {
    DWORD attribute;
    PVOID pData;
    ULONG dataSize;
	} WINCOMPATTRDATA;

	typedef struct _ACCENT_POLICY {
			int nAccentState;
			int nFlags;
			int nColor;
			int nAnimationId;
	} ACCENT_POLICY;

	typedef BOOL(WINAPI *pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

	HMODULE hUser = LoadLibraryEx(TEXT("user32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

	ACCENT_POLICY accent = { 3, 0, 0, 0 };
	WINCOMPATTRDATA data = { 19, &accent, sizeof(accent) };

	if(SetWindowCompositionAttribute) {
		SetWindowCompositionAttribute(hwnd, &data);
	}

	FreeLibrary(hUser);

}

func void show_window()
{
	g_window_shown = true;

	// @Note(tkap, 14/05/2023): https://stackoverflow.com/a/34414846
	HWND hCurWnd = GetForegroundWindow();
	DWORD dwMyID = GetCurrentThreadId();
	DWORD dwCurID = GetWindowThreadProcessId(hCurWnd, NULL);
	AttachThreadInput(dwCurID, dwMyID, TRUE);
	SetWindowPos(g_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	SetWindowPos(g_window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	SetForegroundWindow(g_window);
	SetFocus(g_window);
	SetActiveWindow(g_window);
	AttachThreadInput(dwCurID, dwMyID, FALSE);

	enable_blur_behind(g_window);
}

func void hide_window()
{
	g_window_shown = false;
	app_state = 0;
	ShowWindow(g_window, SW_HIDE);
}

// @Note(tkap, 16/05/2023): https://stackoverflow.com/a/15977613
func WPARAM remap_key_if_necessary(WPARAM vk, LPARAM lparam)
{
	WPARAM new_vk = vk;
	UINT scancode = (lparam & 0x00ff0000) >> 16;
	int extended  = (lparam & 0x01000000) != 0;

	switch(vk)
	{
		case VK_SHIFT:
		{
			new_vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
		} break;

		case VK_CONTROL:
		{
			new_vk = extended ? VK_RCONTROL : VK_LCONTROL;
		} break;

		case VK_MENU:
		{
			new_vk = extended ? VK_RMENU : VK_LMENU;
		} break;

		default:
		{
			new_vk = vk;
		} break;
	}

	return new_vk;
}

template <typename t>
func t do_interp(float time, s_value_interp<t>* value_arr, int value_count)
{
	assert(value_count >= 2);
	int left = 0;
	int right = 1;
	for(int i = 0; i < value_count - 1; i++) {
		if(value_arr[i + 1].time >= time) {
			left = i;
			right = at_most(value_count - 1, i + 1);
			break;
		}
	}
	s_value_interp left_val = value_arr[left];
	s_value_interp right_val = value_arr[right];
	return lerp(left_val.color, right_val.color, ilerp(left_val.time, right_val.time, time));
}

template <typename t>
func void arr_add(t* arr, t element, int* count)
{
	arr[*count] = element;
	*count += 1;
}