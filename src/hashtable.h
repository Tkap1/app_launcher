

global constexpr u32 c_max_hashtable_elements = 8192;

template <typename T>
struct s_hashtable
{
	b8 removed_arr[c_max_hashtable_elements];
	s_str<16> key_arr[c_max_hashtable_elements];
	T value_arr[c_max_hashtable_elements];

	T* add(char* key, T value);
	void remove(char* key);
	T* get(char* key);
};

func u32 hash(char* str);