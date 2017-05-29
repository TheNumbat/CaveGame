
void setup_events(game_state* state) {
	
	state->event_queue = make_queue<platform_event>(256, &state->default_platform_allocator);
	state->api->platform_set_queue_callback(&event_enqueue, &state->event_queue);
}

void end_events(game_state* state) {

	destroy_queue(&state->event_queue);
}

void filter_dupe_window_events(queue<platform_event>* queue) {

	if(queue_empty(queue)) return;

	bool found_move = false, found_resize = false;
	
	for(i32 i = (i32)queue->contents.size - 1; i >= 0; i--) {
		platform_event* evt = vector_get(&queue->contents, i);
		if(evt->type == event_window)  {
			if(evt->window.op == window_moved) {
				if(found_move) {
					vector_erase(&queue->contents, i);
				} else {
					found_move = true;
				}
			} else if(evt->window.op == window_resized) {
				if(found_resize) {
					vector_erase(&queue->contents, i);
				} else {
					found_resize = true;
				}
			}
		}
	}
}

bool run_events(game_state* state) {

	state->api->platform_queue_messages(&state->window);
	filter_dupe_window_events(&state->event_queue);

	while(!queue_empty(&state->event_queue)) {

		platform_event evt = queue_pop(&state->event_queue);

		if(evt.type == event_window && evt.window.op == window_close) {
			return false;
		}

		else if(evt.type == event_key && evt.key.code == key_w) {
			if(evt.key.flags & key_flag_ctrl && evt.key.flags & key_flag_alt) {
				if(evt.key.flags & key_flag_press)
					LOG_DEBUG("Ctrl Alt Press");
				if(evt.key.flags & key_flag_release)
					LOG_DEBUG("Ctrl Alt Release");
				if(evt.key.flags & key_flag_repeat)
					LOG_DEBUG("Ctrl Alt Repeat");
			}
		}

		else if(evt.type == event_window && evt.window.op == window_resized) {
			LOG_DEBUG_F("window resized w: %i h: %i", evt.window.x, evt.window.y);
			state->window_w = evt.window.x;
			state->window_h = evt.window.y;
		}

		else if(evt.type == event_window && evt.window.op == window_moved) {
			LOG_DEBUG_F("window moved x: %i y: %i", evt.window.x, evt.window.y);
		}

		else if(evt.type == event_mouse && evt.mouse.flags & mouse_flag_move) {
			// LOG_DEBUG_F("mouse at x: %i y: %i", evt.mouse.x, evt.mouse.y);
		}

		else if(evt.type == event_mouse && evt.mouse.flags & mouse_flag_mclick && evt.mouse.flags & mouse_flag_double) {
			LOG_DEBUG_F("Double mclick");
		}


		else if(evt.type == event_mouse && evt.mouse.flags & mouse_flag_press && evt.mouse.flags & mouse_flag_mclick) {
			LOG_DEBUG_F("press mclick");
		}
		else if(evt.type == event_mouse && evt.mouse.flags & mouse_flag_release && evt.mouse.flags & mouse_flag_mclick) {
			LOG_DEBUG_F("release mclick");
		}
	}

	return true;
}

void event_enqueue(void* data, platform_event evt) {

	queue<platform_event>* q = (queue<platform_event>*)data;

	queue_push(q, evt);
}