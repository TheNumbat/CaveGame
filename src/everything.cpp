
inline code_context _make_context(string file, string function, i32 line) {

	code_context ret;
#ifdef DO_PROF
	ret.file = np_substring(file, np_string_last_slash(file) + 1, file.len - 1);
	ret.function = function;
	ret.line = line;
#endif
	
	return ret;
}

template<typename... Targs>
void _begin_thread(string fmt, allocator* alloc, code_context start, Targs... args) {
	make_type_table(alloc);
	this_thread_data.alloc_stack = stack<allocator*>::make(8, alloc);
	this_thread_data.start_context = start;
	PUSH_ALLOC(alloc);
	this_thread_data.name = string::makef(fmt, args...);
}

void end_thread() { 
	this_thread_data.name.destroy();
	POP_ALLOC();

	this_thread_data.alloc_stack.destroy();
	type_table.destroy();
}
