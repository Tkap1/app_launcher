
enum e_json
{
	e_json_object,
	e_json_integer,
	e_json_float,
	e_json_string,
	e_json_bool,
	e_json_array,
	e_json_null,
};

struct s_string_parse
{
	b8 success;
	char* first_char;
	char* last_char;
	char* continuation;
	char* result;
	int len;
};

struct s_json
{
	e_json type;
	char* key;
	s_json* next;

	union
	{
		b8 bool_val;
		s_json* object;
		s_json* array;
		int integer;
		float float_val;
		char* str;
	};
};

func s_json* parse_json(char* str);
func void print_json(s_json* json, int indentation);
func s_json* json_get(s_json* json, char* key_name, e_json in_type);
func s_string_parse parse_string(char* str, b8 do_alloc);