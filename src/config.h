
global constexpr s_v2 c_origin_topleft = v2(1.0f, -1.0f);
global constexpr s_v2 c_origin_bottomleft = v2(1.0f, 1.0f);
global constexpr s_v2 c_origin_center = v2(0, 0);
global constexpr s_v2 c_base_res = v2(1280, 720);

global constexpr int c_max_keys = 1024;
global constexpr int max_keys_in_queue = 32;
global constexpr int max_chars_in_queue = 32;

global constexpr int left_button = 1000;
global constexpr int right_button = 1001;

global constexpr int key_backspace = 0x08;
global constexpr int key_tab = 0x09;
global constexpr int key_enter = 0x0D;
global constexpr int key_alt = 0x12;
global constexpr int key_escape = 0x1B;
global constexpr int key_space = 0x20;
global constexpr int key_end = 0x23;
global constexpr int key_home = 0x24;
global constexpr int key_left = 0x25;
global constexpr int key_up = 0x26;
global constexpr int key_right = 0x27;
global constexpr int key_down = 0x28;
global constexpr int key_delete = 0x2E;

global constexpr int key_0 = 0x30;
global constexpr int key_1 = 0x31;
global constexpr int key_2 = 0x32;
global constexpr int key_3 = 0x33;
global constexpr int key_4 = 0x34;
global constexpr int key_5 = 0x35;
global constexpr int key_6 = 0x36;
global constexpr int key_7 = 0x37;
global constexpr int key_8 = 0x38;
global constexpr int key_9 = 0x39;
global constexpr int key_a = 0x41;
global constexpr int key_b = 0x42;
global constexpr int key_c = 0x43;
global constexpr int key_d = 0x44;
global constexpr int key_e = 0x45;
global constexpr int key_f = 0x46;
global constexpr int key_g = 0x47;
global constexpr int key_h = 0x48;
global constexpr int key_i = 0x49;
global constexpr int key_j = 0x4A;
global constexpr int key_k = 0x4B;
global constexpr int key_l = 0x4C;
global constexpr int key_m = 0x4D;
global constexpr int key_n = 0x4E;
global constexpr int key_o = 0x4F;
global constexpr int key_p = 0x50;
global constexpr int key_q = 0x51;
global constexpr int key_r = 0x52;
global constexpr int key_s = 0x53;
global constexpr int key_t = 0x54;
global constexpr int key_u = 0x55;
global constexpr int key_v = 0x56;
global constexpr int key_w = 0x57;
global constexpr int key_x = 0x58;
global constexpr int key_y = 0x59;
global constexpr int key_z = 0x5A;
global constexpr int key_add = 0x6B;
global constexpr int key_subtract = 0x6D;

global constexpr int key_f1 = 0x70;
global constexpr int key_f2 = 0x71;
global constexpr int key_f3 = 0x72;
global constexpr int key_f4 = 0x73;
global constexpr int key_f5 = 0x74;
global constexpr int key_f6 = 0x75;
global constexpr int key_f7 = 0x76;
global constexpr int key_f8 = 0x77;
global constexpr int key_f9 = 0x78;
global constexpr int key_f10 = 0x79;
global constexpr int key_f11 = 0x7A;
global constexpr int key_f12 = 0x7B;

global constexpr int key_left_shift = 0xA0;
global constexpr int key_right_shift = 0xA1;
global constexpr int key_left_control = 0xA2;
global constexpr int key_right_control = 0xA3;

global constexpr s_v4 c_button_color = rgb(0x929576);
global constexpr s_v4 c_text_color = rgb(0xFAD592);

global constexpr char* c_folder_blacklist[] = {
	".git",
	".vscode",
	".cache",
};

// global constexpr char* c_extension_blacklist[] = {
// 	".bin",
// 	".idx",
// };

global constexpr char* c_extension_whitelist[] = {
	".txt",
	".c",
	".cpp",
	".h",
	".hpp",
	".py",
	".ahk",
	".bat",
	".enums",
	".funcs",
	".globals",
	".fragment",
	".vertex",
	".geometry",
};

global constexpr char* vscode_path = "C:/Program Files/Microsoft VS Code/Code.exe";