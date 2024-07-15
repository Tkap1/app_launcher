
func char* skip_whitespace(char* str)
{
	while(true) {
		if(*str == '0') { break; }
		else if(*str <= ' ') { str += 1; }
		else { break; }
	}
	return str;
}

func char* parse_json_key(char** out_str)
{
	char* str = *out_str;
	str = skip_whitespace(str);
	assert(*str == '"');
	str += 1;
	char* start = str;
	char* key = NULL;
	while(true) {
		if(*str == '\0') { assert(false); }
		else if(*str == '"' && str[-1] != '\\') {
			u64 len = str - start;
			key = (char*)malloc(len + 1);
			memcpy(key, start, len);
			key[len] = 0;
			break;
		}
		else { str += 1; }
	}
	str += 1;
	str = skip_whitespace(str);
	assert(*str == ':');
	str += 1;
	*out_str = str;
	return key;
}

func s_json* parse_json_object(char** out_str)
{
	char* str = *out_str;
	str = skip_whitespace(str);
	s_json* result = NULL;
	if(*str == '{') {
		str += 1;
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_object;
		s_json** curr = &result->object;
		while(true) {
			str = skip_whitespace(str);
			if(*str == '}') { break; }
			char* key = parse_json_key(&str);
			s_json* child = parse_json_object(&str);
			assert(child);
			child->key = key;
			*curr = child;
			curr = &child->next;
			str = skip_whitespace(str);
			if(*str != ',') { break; }
			str += 1;
		}
		str += 1;
	}
	else if(strncmp(str, "true", 4) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_bool;
		result->bool_val = true;
		str += 4;
	}
	else if(strncmp(str, "false", 5) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_bool;
		result->bool_val = false;
		str += 5;
	}
	else if(strncmp(str, "null", 4) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_null;
		str += 4;
	}
	else if(*str == '"') {
		s_string_parse parse = parse_string(str, true);
		assert(parse.success);
		str = parse.continuation;
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_string;
		result->str = parse.result;
	}
	else if(is_number(*str) || *str == '-') {
		result = (s_json*)calloc(1, sizeof(*result));
		char* end = null;
		float val = strtof(str, &end);
		b8 is_float = false;
		char* cursor = str;
		while(cursor < end) {
			if(*cursor == '.') {
				is_float = true;
				break;
			}
			cursor += 1;
		}
		str = end;
		if(is_float) {
			result->type = e_json_float;
			result->float_val = val;
		}
		else {
			result->type = e_json_integer;
			result->integer = (int)val;
		}
	}
	else if('[') {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_array;
		str += 1;
		s_json** curr = &result->array;
		while(true) {
			str = skip_whitespace(str);
			if(*str == ']') { break; }
			s_json* child = parse_json_object(&str);
			assert(child);
			*curr = child;
			curr = &child->next;
			str = skip_whitespace(str);
			if(*str != ',') { break; }
			str += 1;
		}

		assert(*str == ']');
		str += 1;
	}
	*out_str = str;
	return result;
}

func s_json* parse_json(char* str)
{
	return parse_json_object(&str);
}

func void print_json(s_json* json, int indentation)
{
	assert(json);
	char spaces[64] = zero;
	for(int i = 0; i < indentation; i += 1) {
		spaces[i * 2] = ' ';
		spaces[i * 2 + 1] = ' ';
	}
	for(s_json* j = json; j; j = j->next) {
		printf("%s", spaces);
		if(j->key) {
			printf("%s: ", j->key);
		}
		switch(j->type) {
			case e_json_bool: {
				printf("%s\n", j->bool_val ? "true" : "false");
			} break;
			case e_json_integer: {
				printf("%i\n", j->integer);
			} break;
			case e_json_float: {
				printf("%f\n", j->float_val);
			} break;
			case e_json_string: {
				printf("\"%s\"\n", j->str);
			} break;
			case e_json_object: {
				printf("{\n");
				print_json(j->object, indentation + 1);
				printf("%s}\n", spaces);
			} break;
			case e_json_array: {
				printf("[\n");
				if(j->array) {
					print_json(j->array, indentation + 1);
				}
				printf("%s]\n", spaces);
			} break;
			case e_json_null: {
				printf("null\n");
			} break;
			invalid_default_case;
		}
	}
}

func s_json* json_get(s_json* json, char* key_name, e_json in_type)
{
	assert(json);
	for(s_json* j = json; j; j = j->next) {
		if(!j->key) {
			if(j->object) {
				return json_get(j->object, key_name, in_type);
			}
		}
		if(j->type == in_type && strcmp(j->key, key_name) == 0) {
			return j;
		}
	}
	return NULL;
}

func s_string_parse parse_string(char* str, b8 do_alloc)
{
	s_string_parse result = {};
	if(*str != '"') { return {}; }
	str += 1;
	result.first_char = str;
	while(true) {
		if(*str == '\0') { return {}; }
		else if(*str == '"' && str[-1] != '\\') {
			result.success = true;
			result.last_char = str - 1;
			result.len = (int)(result.last_char - result.first_char) + 1;
			result.continuation = str + 1;
			if(do_alloc && result.last_char >= result.first_char) {
				result.result = (char*)malloc(result.len + 1);
				memcpy(result.result, result.first_char, result.len);
				result.result[result.len] = 0;
			}
			break;
		}
		else { str += 1; }
	}
	return result;
}