
log_manager make_logger(allocator* a) {

	log_manager ret;

	ret.out = make_vector<log_out>(4, a);
	ret.message_queue = make_queue<log_message>(8, a);
	global_state->api->platform_create_mutex(&ret.queue_mutex, false);
	global_state->api->platform_create_semaphore(&ret.logging_semaphore, 0, INT32_MAX);

	ret.alloc = a;
	ret.scratch = MAKE_ARENA("log scratch", 2048, a, true);

	return ret;
}

void logger_start(log_manager* log) {

	log->thread_param.out 				= &log->out;
	log->thread_param.message_queue 	= &log->message_queue;
	log->thread_param.queue_mutex		= &log->queue_mutex;
	log->thread_param.logging_semaphore = &log->logging_semaphore;
	log->thread_param.running 			= true;
	log->thread_param.alloc 			= log->alloc;
	log->thread_param.scratch			= &log->scratch;

	global_state->api->platform_create_thread(&log->logging_thread, &logging_thread, &log->thread_param, false);
}

void logger_stop(log_manager* log) {

	log->thread_param.running = false;

	global_state->api->platform_signal_semaphore(&log->logging_semaphore, 1);
	global_state->api->platform_join_thread(&log->logging_thread, -1);
	global_state->api->platform_destroy_thread(&log->logging_thread);

	log->thread_param.out 				= NULL;
	log->thread_param.message_queue 	= NULL;
	log->thread_param.queue_mutex		= NULL;
	log->thread_param.logging_semaphore = NULL;
	log->thread_param.alloc 			= NULL;
	log->thread_param.scratch			= NULL;
}

void destroy_logger(log_manager* log) {

	if(log->thread_param.running) {
		logger_stop(log);
	}

	destroy_vector(&log->out);
	destroy_queue(&log->message_queue);
	global_state->api->platform_destroy_mutex(&log->queue_mutex);
	global_state->api->platform_destroy_semaphore(&log->logging_semaphore);
	DESTROY_ARENA(&log->scratch);
	log->alloc = NULL;
}

void logger_push_context(log_manager* log, string context) {

	stack_push(&this_thread_data.context_stack, context);
}

void logger_pop_context(log_manager* log) {


	stack_pop(&this_thread_data.context_stack);
}

void logger_add_file(log_manager* log, platform_file file, log_level level) {

	log_out lfile;
	lfile.file = file;
	lfile.level = level;
	vector_push(&log->out, lfile);

	logger_print_header(log, lfile);
}

void logger_add_output(log_manager* log, log_out out) {

	vector_push(&log->out, out);
	if(!out.custom) {
		logger_print_header(log, out);
	}
}

void logger_print_header(log_manager* log, log_out out) {

	PUSH_ALLOC(log->alloc) {
		
		string header = make_stringf(string_literal("%-8s [%-24s] [%-20s] [%-5s] %-2s\r\n"), "time", "thread/context", "file:line", "level", "message");

		global_state->api->platform_write_file(&out.file, (void*)header.c_str, header.len - 1);

		free_string(header);

	} POP_ALLOC();
}

void logger_msgf(log_manager* log, string fmt, log_level level, code_context context, ...) {

	log_message lmsg;

	lmsg.arena = MAKE_ARENA("msg arena", 512, log->alloc, true);
	PUSH_ALLOC(&lmsg.arena) {

		va_list args;
		va_start(args, context);
		lmsg.msg = make_vstringf(fmt, args);
		va_end(args);

		lmsg.publisher = context;
		lmsg.level = level;
		lmsg.data = this_thread_data;
		lmsg.data.context_stack = make_stack_copy_trim(lmsg.data.context_stack, &lmsg.arena);
		lmsg.data.name = make_copy_string(lmsg.data.name);
		
		global_state->api->platform_aquire_mutex(&log->queue_mutex, -1);
		queue_push(&log->message_queue, lmsg);
		global_state->api->platform_release_mutex(&log->queue_mutex);
		global_state->api->platform_signal_semaphore(&log->logging_semaphore, 1);

		if(level == log_error) {
			if(global_state->api->platform_is_debugging()) {
				__debugbreak();	
			}
#ifdef BLOCK_ON_ERROR
			global_state->api->platform_join_thread(&log->logging_thread, -1);
#endif
		}
		if(level == log_fatal) {
			if(global_state->api->platform_is_debugging()) {
				__debugbreak();	
			}
			global_state->api->platform_join_thread(&log->logging_thread, -1);
		}

	} POP_ALLOC();
}

void logger_msg(log_manager* log, string msg, log_level level, code_context context) {

	log_message lmsg;

	lmsg.arena = MAKE_ARENA("msg arena", 512, log->alloc, true);
	PUSH_ALLOC(&lmsg.arena) {

		lmsg.msg = make_copy_string(msg);
		lmsg.publisher = context;
		lmsg.level = level;
		lmsg.data = this_thread_data;
		lmsg.data.context_stack = make_stack_copy_trim(lmsg.data.context_stack, &lmsg.arena);
		lmsg.data.name = make_copy_string(lmsg.data.name);

		global_state->api->platform_aquire_mutex(&log->queue_mutex, -1);
		queue_push(&log->message_queue, lmsg);
		global_state->api->platform_release_mutex(&log->queue_mutex);
		global_state->api->platform_signal_semaphore(&log->logging_semaphore, 1);

		if(level == log_error) {
			if(global_state->api->platform_is_debugging()) {
				__debugbreak();	
			}
#ifdef BLOCK_ON_ERROR
			global_state->api->platform_join_thread(&log->logging_thread, -1);
#endif
		}
		if(level == log_fatal) {
			if(global_state->api->platform_is_debugging()) {
				__debugbreak();	
			}
			global_state->api->platform_join_thread(&log->logging_thread, -1);
		}

	} POP_ALLOC();
}

string log_fmt_msg(log_message* msg) {

	string time = make_string(9);
	global_state->api->platform_get_timef(string_literal("hh:mm:ss"), &time);
		
	string thread_contexts = make_cat_string(msg->data.name, string_literal("/"));
	for(u32 j = 0; j < msg->data.context_stack.contents.size; j++) {
		string temp = make_cat_strings(3, thread_contexts, *vector_get(&msg->data.context_stack.contents, j), string_literal("/"));
		free_string(thread_contexts);
		thread_contexts = temp;
	}

	string file_line = make_stringf(string_literal("%s:%u"), msg->publisher.file.c_str, msg->publisher.line);

	string level;
	switch(msg->level) {
	case log_debug:
		level = string_literal("DEBUG");
		break;
	case log_info:
		level = string_literal("INFO");
		break;
	case log_warn:
		level = string_literal("WARN");
		break;
	case log_error:
		level = string_literal("ERROR");
		break;
	case log_fatal:
		level = string_literal("FATAL");
		break;
	case log_ogl:
		level = string_literal("OGL");
		break;
	case log_alloc:
		level = string_literal("ALLOC");
		break;
	}

	string output = make_stringf(string_literal("%-8s [%-24s] [%-20s] [%-5s] %*s\r\n"), time.c_str, thread_contexts.c_str, file_line.c_str, level.c_str, 3 * msg->data.context_stack.contents.size + msg->msg.len - 1, msg->msg.c_str);

	free_string(time);
	free_string(thread_contexts);
	free_string(file_line);
	free_string(level);

	return output;	
}

void logger_rem_output(log_manager* log, log_out out) {

	vector_erase(&log->out, out);
}

bool operator==(log_out l, log_out r) {
	if(!(l.level == r.level && l.custom == r.custom)) return false;
	if(l.custom && l.write == r.write) return true;
	return l.file == r.file;
}

i32 logging_thread(void* data_) {

	log_thread_param* data = (log_thread_param*)data_;	

	begin_thread(string_literal("log"), &global_state->suppressed_platform_allocator);

	while(data->running) {

		for(;;) {
		
			global_state->api->platform_aquire_mutex(data->queue_mutex, -1);

			if(queue_empty(data->message_queue)) {
				global_state->api->platform_release_mutex(data->queue_mutex);
				break;
			}
			log_message msg = queue_pop(data->message_queue);
			
			global_state->api->platform_release_mutex(data->queue_mutex);

			if(msg.msg.c_str != NULL) {
				
				string output;
				PUSH_ALLOC(data->scratch) {
					
					output = log_fmt_msg(&msg);
				
					FORVEC(*data->out,
						if(it->level <= msg.level) {
							if(it->custom) {
								it->write(&msg, output);
							} else {
								global_state->api->platform_write_file(&it->file, (void*)output.c_str, output.len - 1);
							}
						}
					)

					free_string(output);

				} POP_ALLOC();
				RESET_ARENA(data->scratch);

				if(msg.level == log_fatal) {
					// die
					exit(1);
				}

				DESTROY_ARENA(&msg.arena);
			}
		}

		global_state->api->platform_wait_semaphore(data->logging_semaphore, -1);
	}

	end_thread();

	return 0;
}
