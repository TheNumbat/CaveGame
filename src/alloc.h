
#pragma once

struct allocator {
	func_ptr<void*,u64,allocator*,code_context>				allocate_;
	func_ptr<void*,void*,u64,u64,allocator*,code_context> 	reallocate_;
	func_ptr<void,void*,allocator*,code_context> 			free_;

	void destroy();

	code_context context;
	bool suppress_messages = false;
	string name;
};

#define PUSH_ALLOC(a)	_push_alloc(a);
#define POP_ALLOC() 	_pop_alloc();
#define CURRENT_ALLOC()	_current_alloc()

#define KILOBYTES(m) (m*1024)
#define MEGABYTES(m) (KILOBYTES(m*1024))
#define GIGABYTES(m) (MEGABYTES(m*1024))

inline allocator* _current_alloc();
inline void _pop_alloc();
inline void _push_alloc(allocator* a);
inline allocator* _current_alloc();

struct platform_allocator : public allocator {
	void* (*platform_allocate)(u64 bytes) 				= null;
	void  (*platform_free)(void* mem)	  				= null;
	void* (*platform_reallocate)(void* mem, u64 bytes)	= null;
};

CALLBACK void* platform_allocate(u64 bytes, allocator* this_, code_context context);
CALLBACK void  platform_free(void* mem, allocator* this_, code_context context);
CALLBACK void* platform_reallocate(void* mem, u64, u64 bytes, allocator* this_, code_context context);

#define MAKE_PLATFORM_ALLOCATOR(n) make_platform_allocator(string::literal(n), CONTEXT)
inline platform_allocator make_platform_allocator(string name, code_context context);

struct arena_allocator : public allocator {
	allocator* backing 	= null;
	void* memory 		= null;
	u64 used			= 0;
	u64 size			= 0;
};

CALLBACK void* arena_allocate(u64 bytes, allocator* this_, code_context context);
CALLBACK void* arena_reallocate(void* mem, u64 sz, u64 bytes, allocator* this_, code_context context); // same as allocate, can't free from arena
CALLBACK void  arena_free(void*, allocator*, code_context); // does nothing

#define DESTROY_ARENA(a) arena_destroy(a, CONTEXT) 
void arena_destroy(arena_allocator* a, code_context context);

#define	RESET_ARENA(a) arena_reset(a, CONTEXT) 
void arena_reset(arena_allocator* a, code_context context);

#define MAKE_ARENA_FROM_CONTEXT(n, size, s) make_arena_allocator_from_context(n, size, s, CONTEXT) 
arena_allocator make_arena_allocator_from_context(string name, u64 size, bool suppress, code_context context);

#define MAKE_ARENA(n, size, a, s) make_arena_allocator(n, size, a, s, CONTEXT) 
arena_allocator make_arena_allocator(string name, u64 size, allocator* backing, bool suppress, code_context context);

#define malloc(b) 	((CURRENT_ALLOC()->allocate_)((u64)b, CURRENT_ALLOC(), CONTEXT)) 
#define free(m) 	((CURRENT_ALLOC()->free_)((void*)m, CURRENT_ALLOC(), CONTEXT)) 

#define memcpy(s,d,i) _memcpy(s,d,i)
void _memcpy(void* source, void* dest, u64 size);
#define memset(m,s,v) _memset(m,s,v)
void _memset(void* mem, u64 size, u8 val);
