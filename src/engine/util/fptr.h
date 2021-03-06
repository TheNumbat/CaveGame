
#pragma once

#include "../ds/string.h"
#include "../platform/platform_api.h"

struct _FPTR {
	void* func;
	string name;
};

#define MAX_FPTRS 128

struct func_ptr_state {

	_FPTR all_ptrs[128];
	u32 num_ptrs = 0;

	platform_dll* this_dll = null;
	platform_mutex mut;

	void reload_all();
};

extern func_ptr_state* global_func;

template<typename T, typename... args>
struct func_ptr {

	func_ptr(_FPTR* d = null) {data = d;}

	_FPTR* data = null;

	T operator()(args... arg) {

		return ((T(*)(args...))data->func)(arg...);
	}

	operator bool() {
		return data != null;
	}

	void set(_FPTR* f) {
		data = f;
	}
};

_FPTR* _fptr(void* func, string name);
_FPTR* FPTR_STR(string name);

void setup_fptrs();
void cleanup_fptrs();

#define FPTR(name) _fptr((void*)&name, #name##_)

