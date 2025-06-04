#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>

#include <ember_ecs/ecs.h>
#include <ember_ecs/array_stack.h>

#define INITIAL_CAPACITY 32

typedef struct {
	uint32_t type_count;
	uint32_t cap;
	size_t size;
	size_t *data_size_array;
	size_t *data_offset_array;
	void *data;
} ComponentStore;

typedef struct {
	uint32_t *mask_array;
	uint32_t *flag_array;
	uint32_t count;
	uint32_t cap;
} EntityStore;

typedef struct {
	ComponentStore component_store;
	EntityStore entity_store;
	QueryResult query_result;
	ArrayStack *entity_pool;
} State;

static State state = {0};

void
ecs_init(uint32_t n, ...)
{
	uint32_t i;
	va_list ap;
	size_t sizes[32];
	size_t offsets[32];
	size_t size = 0;

	va_start(ap, n);
	for (i = 0; i < n; ++i) {
		sizes[i] = va_arg(ap, size_t);
		offsets[i] = size;
		size += sizes[i];
	}
	va_end(ap);

	state.entity_pool = as_create(sizeof(uint32_t));

	state.component_store.type_count = n;
	state.component_store.cap = INITIAL_CAPACITY;
	state.component_store.data = malloc(INITIAL_CAPACITY * size);
	state.component_store.data_size_array = malloc(n * sizeof(size_t));
	state.component_store.data_offset_array = malloc(n * sizeof(size_t));
	state.component_store.size = size;
	memcpy(state.component_store.data_size_array, sizes, n * sizeof(size_t));
	memcpy(state.component_store.data_offset_array, offsets, n * sizeof(size_t));

	state.entity_store.count = 0;
	state.entity_store.cap = INITIAL_CAPACITY;
	state.entity_store.mask_array = malloc(INITIAL_CAPACITY * sizeof(uint32_t));
	state.entity_store.flag_array = malloc(INITIAL_CAPACITY * sizeof(uint32_t));
	
    state.query_result.list = malloc(INITIAL_CAPACITY * sizeof(uint32_t));
}

Entity
ecs_create()
{
	Entity entity;
	uint32_t id;
	if (state.entity_pool->count > 0) {
		id = *(uint32_t*)as_pop(state.entity_pool);
	} else {
		id = state.entity_store.count++;
		if (state.entity_store.cap == id) {
			uint32_t *new_flag_array = realloc(state.entity_store.flag_array, state.entity_store.cap * 2 * sizeof(uint32_t));
			uint32_t *new_mask_array = realloc(state.entity_store.mask_array, state.entity_store.cap * 2 * sizeof(uint32_t));
			void *new_data = realloc(state.component_store.data, state.component_store.cap * 2 * state.component_store.size);
			uint32_t *new_query_result_list = realloc(state.query_result.list, state.entity_store.cap * 2 * sizeof(uint32_t));
			if (NULL == new_flag_array || NULL == new_mask_array || NULL == new_data || NULL == new_query_result_list) {
				printf("Realloc fail %s:%d\n", __FILE__, __LINE__);
				exit(1);
			} else {
				state.entity_store.flag_array = new_flag_array;
				state.entity_store.mask_array = new_mask_array;
				state.query_result.list = new_query_result_list;
				state.entity_store.cap *= 2;

				state.component_store.data = new_data;
				state.component_store.cap *= 2;
			}
		}
	}

	state.entity_store.mask_array[id] = 0;
	state.entity_store.flag_array[id] = ENTITY_FLAG_ALIVE;
	entity.id = id;
	return entity;
}

void *
ecs_get(uint32_t entity_id, uint32_t component_id)
{
	return (uint8_t*)state.component_store.data + (entity_id * state.component_store.size + state.component_store.data_offset_array[component_id]);
}

void
ecs_add(uint32_t entity_id, uint32_t component_id, void *data) {
	size_t size = state.component_store.data_size_array[component_id];
	void *ptr = ecs_get(entity_id, component_id);
	state.entity_store.mask_array[entity_id] |= (1 << component_id);
	if (size == 0) return;
	memcpy(ptr, data, size);
} 

void
ecs_remove(uint32_t entity_id, uint32_t component_id)
{
	state.entity_store.mask_array[entity_id] &= ~(1 << component_id);
}

uint32_t
ecs_has(uint32_t entity_id, uint32_t component_id)
{
	return 0 != (state.entity_store.mask_array[entity_id] & (1 << component_id));
}

void
ecs_kill(uint32_t entity_id)
{
	if (0 != (state.entity_store.flag_array[entity_id] & ENTITY_FLAG_ALIVE)) {
		state.entity_store.flag_array[entity_id] &= ~ENTITY_FLAG_ALIVE;
		state.entity_store.mask_array[entity_id] = 0;
		as_push(state.entity_pool, &entity_id);
	}
}

QueryResult *
ecs_query(uint32_t n, ...)
{
	va_list ap;
	uint32_t i, mask = 0;

	state.query_result.count = 0;

	va_start(ap, n);
	for (i = 0; i < n; ++i) {
		mask |= (1 << va_arg(ap, uint32_t));
	}
	va_end(ap);

	for (i = 0; i < state.entity_store.count; ++i) {
		if (0 != (state.entity_store.flag_array[i] & ENTITY_FLAG_ALIVE) && mask == (state.entity_store.mask_array[i] & mask)) {
			state.query_result.list[state.query_result.count++] = i;
		}
	}
	return &state.query_result;
}
