

// @Note(tkap, 24/04/2023): function from chatGPT, slightly modified by me
func u32 hash(char* str)
{
	assert(str);
	assert(*str);

	u32 hash = 5381;
	while(true)
	{
		int c = *str++;
		if(!c) { break; }
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

template <typename T>
T* s_hashtable<T>::add(char* key, T value)
{
	assert(key);
	assert(*key);

	u32 index = hash(key) % c_max_hashtable_elements;
	int iterations = 0;
	while(true)
	{
		if(key_arr[index].len == 0)
		{
			key_arr[index].from_cstr(key);
			value_arr[index] = value;
			removed_arr[index] = false;
			return &value_arr[index];
		}
		else if(key_arr[index].equals(key))
		{
			assert(!"Tried to insert duplicate element into hashtable");
		}
		index = circular_index(index + 1, c_max_hashtable_elements);
		iterations++;
		if(iterations >= c_max_hashtable_elements) { assert(false && "Hashtable is full or something went wrong"); }
	}
	return null;
}

template <typename T>
void s_hashtable<T>::remove(char* key)
{
	assert(key);
	assert(*key);

	u32 index = hash(key) % c_max_hashtable_elements;
	int iterations = 0;
	while(true)
	{
		if(key_arr[index].len > 0 && key_arr[index].equals(key))
		{
			removed_arr[index] = true;
			key_arr[index].len = 0;
			return;
		}
		index = circular_index(index + 1, c_max_hashtable_elements);
		iterations++;
		if(iterations >= c_max_hashtable_elements) { assert(false && "Element not found"); }
	}
	return null;
}

template <typename T>
T* s_hashtable<T>::get(char* key)
{
	assert(key);
	assert(*key);

	u32 index = hash(key) % c_max_hashtable_elements;
	int iterations = 0;
	while(true)
	{
		auto temp_key = &key_arr[index];

		if(temp_key->len == 0)
		{
			return null;
		}

		else if(temp_key->equals(key))
		{
			return &value_arr[index];
		}

		else if(removed_arr[index])
		{
			index = circular_index(index + 1, c_max_hashtable_elements);
			iterations++;
			if(iterations >= c_max_hashtable_elements) { assert(false && "Something went wrong"); }
		}


		invalid_else;
	}
	return null;
}