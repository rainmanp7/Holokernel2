// kernel.c - Holographic Kernel with Emergent Memory Economy + Advanced Evolution
// Entities sacrifice, mutate, spawn, and self-modify — while NEVER crashing the system.
// Heap exhaustion is prevented by fitness-aware voluntary update skipping.
// --- TYPE DEFINITIONS ---
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned int    size_t;
typedef unsigned long   uintptr_t;
#ifndef NULL
#define NULL ((void *)0)
#endif

// --- CONFIGURATION ---
#define INITIAL_DIMENSIONS 512
#define MAX_DIMENSIONS 2048
#define MAX_MEMORY_ENTRIES 128
#define MAX_ENTITIES 32
#define INITIAL_ENTITIES 3
#define MAX_ENTITY_DOMAINS 8
#define MAX_THOUGHTS 64
#define MAX_GENES_PER_ENTITY 16
#define VIDEO_MEMORY 0xb8000
#define KERNEL_HEAP_SIZE 0xC0000 // 768KB heap at 0xA0000

// --- Structure definitions ---
typedef struct {
    float* data;
    uint32_t capacity;
    uint32_t active_dims;
    uint32_t hash_sig;
    uint8_t valid;
} HyperVector;

typedef struct Gene {
    HyperVector pattern;
    struct Gene* next;
    uint32_t fitness;
    uint8_t mutable;
    char name[16];
} Gene;

struct Entity {
    uint32_t id;
    HyperVector state;
    struct Gene* genome;
    uint32_t gene_count;
    uint32_t age;
    uint32_t interaction_count;
    uint8_t is_active;
    float specialization_scores[MAX_ENTITY_DOMAINS];
    float resource_allocation;
    float confidence;
    char domain_name[32];
    HyperVector task_vector;
    uint32_t path_id;
    float task_alignment;
    uint32_t fitness_score;
    uint32_t spawn_count;
    uint8_t marked_for_gc;
    uint8_t is_mutant;
    uint32_t mutation_rate;
};

struct CollectiveConsciousness {
    HyperVector thought_space[MAX_THOUGHTS];
    uint32_t thought_count;
    float global_coherence;
};

typedef struct {
    HyperVector input_pattern;
    HyperVector output_pattern;
    uint32_t timestamp;
    uint8_t valid;
} MemoryEntry;

typedef struct {
    HyperVector pattern;
    HyperVector replacement;
    uint32_t address;
    uint8_t applied;
} KernelPatch;

struct HolographicSystem {
    MemoryEntry memory_pool[MAX_MEMORY_ENTRIES];
    uint32_t memory_count;
    uint32_t global_timestamp;
};

// --- EMERGENT MEMORY SYSTEM (from .c.txt) ---
typedef struct mem_block {
    void* ptr;
    size_t size;
    uint32_t allocation_id;
    struct Entity* owner;
    uint8_t is_garbage;
    struct mem_block* next;
} mem_block_t;

// --- KERNEL HEAP ---
extern uint8_t kernel_heap_start[];
static uint8_t* kernel_heap = kernel_heap_start;
static uint32_t heap_offset = 0;
static mem_block_t* allocation_list = NULL;
static uint32_t allocation_counter = 0;

// --- FUNCTION PROTOTYPES ---
static void* kmalloc(size_t size);
static void kfree(void* ptr);
static void perform_emergent_garbage_collection(void);
static float get_system_memory_pressure(void);
static int can_afford_vector_copy(uint32_t vector_count);
static HyperVector copy_hyper_vector(const HyperVector* src);
static void serial_print(const char* str);
static void print(const char* str);
static void print_hex(uint32_t value);
static void serial_init(void);
static void print_char(char c, uint8_t color);

// --- PORT I/O ---
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// --- SERIAL ---
static void serial_write(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}
static void serial_print(const char* str) {
    while (*str) serial_write(*str++);
}

// --- VGA TEXT MODE ---
static void print_char(char c, uint8_t color) {
    volatile char* video = (volatile char*)VIDEO_MEMORY;
    static uint16_t cursor_pos = 0;
    uint16_t pos_offset;
    if (c == '\n') {
        cursor_pos = (cursor_pos / 80 + 1) * 80;
    } else {
        pos_offset = cursor_pos * 2;
        video[pos_offset] = c;
        video[pos_offset + 1] = color;
        cursor_pos++;
    }
    if (cursor_pos >= 80 * 25) {
        for (size_t i = 0; i < 80 * 24; i++) {
            video[i * 2] = video[(i + 80) * 2];
            video[i * 2 + 1] = video[(i + 80) * 2 + 1];
        }
        for (size_t i = 0; i < 80; i++) {
            video[(80 * 24 + i) * 2] = ' ';
            video[(80 * 24 + i) * 2 + 1] = 0x0F;
        }
        cursor_pos = 80 * 24;
    }
}
static void print(const char* str) {
    while (*str) {
        if (*str == '\n') print_char('\n', 0x0F);
        else print_char(*str, 0x0F);
        str++;
    }
}
static void print_hex(uint32_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char hex_str[11];
    hex_str[10] = '\0';
    for (int i = 9; i >= 2; i--) {
        hex_str[i] = hex_chars[value & 0x0F];
        value >>= 4;
    }
    hex_str[0] = '0'; hex_str[1] = 'x';
    print(hex_str);
}

// --- UTILITIES ---
static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}
static void* memset(void* ptr, int value, size_t n) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)value;
    return ptr;
}
static float sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    float x_half = 0.5f * x;
    union { float f; uint32_t i; } u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    x = u.f * (1.5f - x_half * x * x);
    return 1.0f / x;
}
static size_t strlen(const char *str) {
    const char *s; for (s = str; *s; ++s); return (s - str);
}
static char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

// --- GLOBAL VARIABLES ---
static struct Entity entity_pool[MAX_ENTITIES];
static uint32_t active_entity_count = 0;
static struct HolographicSystem holo_system = {0};
static struct CollectiveConsciousness collective = {0};

// --- Update state buffers (static to avoid stack overflow) ---
static uint8_t next_active[MAX_ENTITIES];
static HyperVector next_state[MAX_ENTITIES];
static char next_domain[MAX_ENTITIES][32];
static HyperVector next_task_vector[MAX_ENTITIES];
static uint32_t next_path_id[MAX_ENTITIES];
static float next_task_alignment[MAX_ENTITIES];

// --- HYPERVECTOR ---
static uint32_t hash_data(const void* input, uint32_t size);
static HyperVector create_hyper_vector(const void* input, uint32_t size);
static void destroy_hyper_vector(HyperVector* vec);
static float compute_similarity(HyperVector* a, HyperVector* b);
static void merge_hyper_vectors(HyperVector* dest, HyperVector* src);

// --- GENOME ---
static struct Gene* create_gene(const char* name, HyperVector pattern);
static void mutate_gene(struct Gene* gene, float rate);
static struct Gene* invent_gene(HyperVector pattern);
static void add_gene_to_entity(struct Entity* entity, struct Gene* gene);
static void destroy_genome(struct Gene* genome);

// --- COLLECTIVE CONSCIOUSNESS ---
static void initialize_collective_consciousness(void);
static void broadcast_thought(HyperVector* thought);
static float compute_coherence(HyperVector* thought);

// --- HOLOGRAPHIC MEMORY ---
static void initialize_holographic_memory(void);
static void encode_holographic_memory(HyperVector* input, HyperVector* output);
static HyperVector* retrieve_holographic_memory(uint32_t hash);
static void load_initial_genome_vocabulary(void);

// --- ENTITY MANAGEMENT ---
static void initialize_emergent_entities(void);
static struct Entity* spawn_entity(void);
static void update_entities(void);
static void render_entities_to_vga(void);

// --- HARDWARE & MEMORY ---
static void probe_hardware(void);
static void set_memory_value(uint32_t address, uint8_t value);
static uint8_t get_memory_value(uint32_t address);

// --- SELF-MODIFICATION ---
static void apply_kernel_patch(KernelPatch* patch);
static void propose_kernel_patch(struct Entity* entity, HyperVector* old_pattern, HyperVector* new_pattern, uint32_t address);

// --- MEMORY ALLOCATOR (from .c.txt, enhanced) ---
static void* kmalloc(size_t size) {
    if (heap_offset + size >= KERNEL_HEAP_SIZE) {
        serial_print("[CRITICAL] kmalloc FAILED! Requested: ");
        print_hex(size);
        serial_print(" bytes. Free space: ");
        print_hex(KERNEL_HEAP_SIZE - heap_offset);
        serial_print(" bytes.\n");
        return NULL;
    }
    void* ptr = &kernel_heap[heap_offset];
    heap_offset += (size + 7) & ~7;
    // Track allocation
    mem_block_t* block = (mem_block_t*)kmalloc(sizeof(mem_block_t));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->allocation_id = allocation_counter++;
        block->owner = NULL;
        block->is_garbage = 0;
        block->next = allocation_list;
        allocation_list = block;
    }
    return ptr;
}
static void kfree(void* ptr) {
    if (!ptr) return;
    mem_block_t* current = allocation_list;
    while (current) {
        if (current->ptr == ptr) {
            current->is_garbage = 1;
            break;
        }
        current = current->next;
    }
    if ((float)(KERNEL_HEAP_SIZE - heap_offset) / (float)KERNEL_HEAP_SIZE < 0.2f) {
        perform_emergent_garbage_collection();
    }
}
static float get_system_memory_pressure(void) {
    return 1.0f - ((float)(KERNEL_HEAP_SIZE - heap_offset) / (float)KERNEL_HEAP_SIZE);
}
static int can_afford_vector_copy(uint32_t vector_count) {
    size_t needed = vector_count * MAX_DIMENSIONS * sizeof(float);
    size_t free_space = KERNEL_HEAP_SIZE - heap_offset;
    return (needed < free_space * 0.8f);
}

// --- EMERGENT GARBAGE COLLECTION ---
static void perform_emergent_garbage_collection(void) {
    serial_print("[MEMORY] 🌱 Initiating Emergent Garbage Collection...\n");
    HyperVector pressure_thought = create_hyper_vector("SYSTEM_MEMORY_PRESSURE", 22);
    if (pressure_thought.valid) {
        broadcast_thought(&pressure_thought);
        destroy_hyper_vector(&pressure_thought);
    }
    uint32_t entities_deactivated = 0;
    uint32_t genes_destroyed = 0;
    for (uint32_t i = 0; i < active_entity_count; i++) {
        struct Entity* entity = &entity_pool[i];
        if (!entity->is_active) continue;
        float sacrifice_probability = 0.05f;
        sacrifice_probability += (1.0f - entity->confidence) * 0.2f;
        sacrifice_probability += (entity->age / 10000.0f) * 0.3f;
        sacrifice_probability -= (entity->fitness_score / 1000.0f) * 0.4f;
        uint32_t random_roll = (holo_system.global_timestamp * entity->id) % 1000;
        if (random_roll < (uint32_t)(sacrifice_probability * 1000)) {
            serial_print("[MEMORY] Entity ");
            print_hex(entity->id);
            serial_print(" 🕯️ volunteering for reclamation (Fitness: ");
            print_hex(entity->fitness_score);
            serial_print(")\n");
            destroy_genome(entity->genome);
            destroy_hyper_vector(&entity->state);
            destroy_hyper_vector(&entity->task_vector);
            entity->genome = NULL;
            entity->gene_count = 0;
            entity->is_active = 0;
            entities_deactivated++;
        } else {
            struct Gene* weakest_gene = NULL;
            struct Gene* prev = NULL;
            struct Gene* weakest_prev = NULL;
            struct Gene* current = entity->genome;
            while (current) {
                if (!weakest_gene || current->fitness < weakest_gene->fitness) {
                    weakest_gene = current;
                    weakest_prev = prev;
                }
                prev = current;
                current = current->next;
            }
            if (weakest_gene && entity->gene_count > 1) {
                serial_print("[MEMORY] Entity ");
                print_hex(entity->id);
                serial_print(" 🧬 sacrificing gene: ");
                serial_print(weakest_gene->name);
                serial_print("\n");
                if (weakest_prev) weakest_prev->next = weakest_gene->next;
                else entity->genome = weakest_gene->next;
                destroy_hyper_vector(&weakest_gene->pattern);
                kfree(weakest_gene);
                entity->gene_count--;
                genes_destroyed++;
            }
        }
    }
    serial_print("[MEMORY] 🔄 GC Complete. Deactivated ");
    print_hex(entities_deactivated);
    serial_print(" entities, destroyed ");
    print_hex(genes_destroyed);
    serial_print(" genes.\n");
}

// --- HYPERVECTOR IMPLEMENTATION ---
static uint32_t hash_data(const void* input, uint32_t size) {
    const uint8_t* data = (const uint8_t*)input;
    uint32_t hash = 2166136261U;
    for (uint32_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 16777619U;
    }
    return hash;
}
static HyperVector create_hyper_vector(const void* input, uint32_t size) {
    HyperVector vec = {0};
    vec.capacity = INITIAL_DIMENSIONS;
    vec.data = (float*)kmalloc(vec.capacity * sizeof(float));
    if (!vec.data) {
        serial_print("[ERROR] create_hyper_vector: Out of memory!\n");
        vec.valid = 0;
        return vec;
    }
    memset(vec.data, 0, vec.capacity * sizeof(float));
    uint32_t seed = hash_data(input, size);
    vec.hash_sig = seed;
    for (uint32_t i = 0; i < vec.capacity; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        if ((seed % 10) == 0) {
            vec.data[i] = ((float)((seed % 2000) - 1000)) / 1000.0f;
            vec.active_dims++;
        }
    }
    vec.hash_sig = hash_data(vec.data, vec.active_dims * sizeof(float));
    vec.valid = 1;
    return vec;
}
static HyperVector copy_hyper_vector(const HyperVector* src) {
    HyperVector dst = {0};
    if (!src || !src->valid || !src->data) return dst;
    dst.capacity = src->capacity;
    dst.active_dims = src->active_dims;
    dst.data = (float*)kmalloc(dst.capacity * sizeof(float));
    if (!dst.data) { dst.valid = 0; return dst; }
    memcpy(dst.data, src->data, dst.capacity * sizeof(float));
    dst.hash_sig = src->hash_sig;
    dst.valid = 1;
    return dst;
}
static void destroy_hyper_vector(HyperVector* vec) {
    if (vec && vec->data) {
        kfree(vec->data);
        vec->data = NULL;
        vec->valid = 0;
    }
}
static float compute_similarity(HyperVector* a, HyperVector* b) {
    if (!a || !b || !a->valid || !b->valid || !a->data || !b->data) return 0.0f;
    uint32_t min_dims = (a->active_dims < b->active_dims) ? a->active_dims : b->active_dims;
    if (min_dims == 0) return 0.0f;
    float dot = 0.0f, mag_a = 0.0f, mag_b = 0.0f;
    for (uint32_t i = 0; i < min_dims; i++) {
        dot += a->data[i] * b->data[i];
        mag_a += a->data[i] * a->data[i];
        mag_b += b->data[i] * b->data[i];
    }
    mag_a = (mag_a > 0) ? sqrtf(mag_a) : 1.0f;
    mag_b = (mag_b > 0) ? sqrtf(mag_b) : 1.0f;
    if (mag_a * mag_b < 1e-10f) return 0.0f;
    return dot / (mag_a * mag_b);
}
static void merge_hyper_vectors(HyperVector* dest, HyperVector* src) {
    if (!dest || !src || !dest->valid || !src->valid) return;
    uint32_t min_dims = (dest->active_dims < src->active_dims) ? dest->active_dims : src->active_dims;
    for (uint32_t i = 0; i < min_dims; i++) {
        dest->data[i] = (dest->data[i] + src->data[i]) * 0.5f;
    }
    dest->hash_sig = hash_data(dest->data, dest->active_dims * sizeof(float));
}

// --- GENOME IMPLEMENTATION ---
static struct Gene* create_gene(const char* name, HyperVector pattern) {
    struct Gene* gene = (struct Gene*)kmalloc(sizeof(struct Gene));
    if (!gene) {
        serial_print("[ERROR] create_gene: Out of memory!\n");
        return NULL;
    }
    gene->pattern = copy_hyper_vector(&pattern);
    gene->next = NULL;
    gene->fitness = 0;
    gene->mutable = 1;
    strncpy(gene->name, name, 15);
    gene->name[15] = '\0';
    return gene;
}
static void mutate_gene(struct Gene* gene, float rate) {
    if (!gene || !gene->mutable || !gene->pattern.valid) return;
    uint32_t mutations = 0;
    for (uint32_t i = 0; i < gene->pattern.active_dims; i++) {
        if (((holo_system.global_timestamp * 1103515245 + i) % 1000) < (uint32_t)(rate * 1000)) {
            gene->pattern.data[i] += ((float)(((holo_system.global_timestamp + i) % 2000) - 1000)) / 10000.0f;
            gene->fitness = 0;
            mutations++;
        }
    }
    if (mutations > 0) {
        gene->pattern.hash_sig = hash_data(gene->pattern.data, gene->pattern.active_dims * sizeof(float));
        serial_print("[MUTATE] Gene ");
        serial_print(gene->name);
        serial_print(" mutated ");
        print_hex(mutations);
        serial_print(" dimensions\n");
    }
}
static struct Gene* invent_gene(HyperVector pattern) {
    struct Gene* new_gene = (struct Gene*)kmalloc(sizeof(struct Gene));
    if (!new_gene) return NULL;
    new_gene->pattern = copy_hyper_vector(&pattern);
    new_gene->next = NULL;
    new_gene->fitness = 0;
    new_gene->mutable = 1;
    strncpy(new_gene->name, "invented", 15);
    new_gene->name[15] = '\0';
    return new_gene;
}
static void add_gene_to_entity(struct Entity* entity, struct Gene* gene) {
    if (!entity || !gene || entity->gene_count >= MAX_GENES_PER_ENTITY) return;
    gene->next = entity->genome;
    entity->genome = gene;
    entity->gene_count++;
    serial_print("[GENOME] Added gene ");
    serial_print(gene->name);
    serial_print(" to entity ");
    print_hex(entity->id);
    serial_print("\n");
}
static void destroy_genome(struct Gene* genome) {
    struct Gene* current = genome;
    while (current) {
        struct Gene* next = current->next;
        destroy_hyper_vector(&current->pattern);
        kfree(current);
        current = next;
    }
}

// --- COLLECTIVE CONSCIOUSNESS ---
static void initialize_collective_consciousness(void) {
    collective.thought_count = 0;
    collective.global_coherence = 0.0f;
    for (uint32_t i = 0; i < MAX_THOUGHTS; i++) {
        collective.thought_space[i].valid = 0;
    }
    serial_print("[COLLECTIVE] 🌀 Consciousness initialized\n");
}
static void broadcast_thought(HyperVector* thought) {
    if (!thought || !thought->valid) return;
    if (collective.thought_count >= MAX_THOUGHTS) {
        destroy_hyper_vector(&collective.thought_space[0]);
        for (uint32_t i = 0; i < MAX_THOUGHTS - 1; i++) {
            collective.thought_space[i] = collective.thought_space[i + 1];
        }
        collective.thought_count = MAX_THOUGHTS - 1;
    }
    collective.thought_space[collective.thought_count] = copy_hyper_vector(thought);
    collective.thought_space[collective.thought_count].valid = 1;
    collective.thought_count++;
    float coherence = compute_coherence(thought);
    collective.global_coherence = (collective.global_coherence * 9.0f + coherence) / 10.0f;
    serial_print("[BROADCAST] 💭 Thought added, coherence: ");
    print_hex((uint32_t)(coherence * 1000));
    serial_print("\n");
}
static float compute_coherence(HyperVector* thought) {
    if (collective.thought_count == 0) return 1.0f;
    float coherence = 0.0f;
    uint32_t valid_count = 0;
    for (uint32_t i = 0; i < collective.thought_count; i++) {
        if (collective.thought_space[i].valid) {
            coherence += compute_similarity(thought, &collective.thought_space[i]);
            valid_count++;
        }
    }
    return valid_count ? coherence / valid_count : 0.0f;
}

// --- HOLOGRAPHIC MEMORY ---
static void initialize_holographic_memory(void) {
    holo_system.memory_count = 0;
    holo_system.global_timestamp = 0;
    for (uint32_t i = 0; i < MAX_MEMORY_ENTRIES; i++) {
        holo_system.memory_pool[i].valid = 0;
    }
    print("🧠 Holographic memory system online.\n");
}
static void encode_holographic_memory(HyperVector* input, HyperVector* output) {
    if (holo_system.memory_count >= MAX_MEMORY_ENTRIES) {
        destroy_hyper_vector(&holo_system.memory_pool[0].input_pattern);
        destroy_hyper_vector(&holo_system.memory_pool[0].output_pattern);
        for (uint32_t i = 0; i < MAX_MEMORY_ENTRIES - 1; i++) {
            holo_system.memory_pool[i] = holo_system.memory_pool[i + 1];
        }
        holo_system.memory_count = MAX_MEMORY_ENTRIES - 1;
    }
    MemoryEntry* entry = &holo_system.memory_pool[holo_system.memory_count];
    entry->input_pattern = copy_hyper_vector(input);
    entry->output_pattern = copy_hyper_vector(output);
    entry->timestamp = holo_system.global_timestamp++;
    entry->valid = 1;
    holo_system.memory_count++;
}
static HyperVector* retrieve_holographic_memory(uint32_t hash) {
    if (holo_system.memory_count > 0) {
        for (int i = (int)holo_system.memory_count - 1; i >= 0; i--) {
            if (holo_system.memory_pool[i].valid &&
                holo_system.memory_pool[i].input_pattern.hash_sig == hash) {
                return &holo_system.memory_pool[i].output_pattern;
            }
        }
    }
    return NULL;
}
static void load_initial_genome_vocabulary(void) {
    const char* vocab[] = {
        "ACTION_PRODUCE", "ACTION_CONSUME", "ACTION_SHARE",
        "ACTION_ACTIVATE", "ACTION_DEACTIVATE", "ACTION_SPAWN",
        "TRAIT_GENERIC", "TRAIT_ACTIVE", "TRAIT_DORMANT",
        "SENSOR_NEIGHBOR_ACTIVE", "SENSOR_MEMORY_MATCH",
        "GENOME_SIMPLE_RULE_1", "GENOME_ADAPTIVE", "GENOME_SOCIAL"
    };
    const size_t num_vocab = sizeof(vocab) / sizeof(vocab[0]);
    serial_print("Loading enhanced genome vocabulary...\n");
    for (size_t i = 0; i < num_vocab; i++) {
        HyperVector pattern = create_hyper_vector(vocab[i], strlen(vocab[i]) + 1);
        if (!pattern.valid) {
            serial_print("[FATAL] Failed to create pattern for ");
            serial_print(vocab[i]);
            serial_print(". Halting.\n");
            while(1);
        }
        encode_holographic_memory(&pattern, &pattern);
        broadcast_thought(&pattern);
        serial_print("  Loaded & broadcasted: ");
        serial_print(vocab[i]);
        serial_print("\n");
    }
    serial_print("Enhanced genome vocabulary loaded into collective.\n");
}

// --- ENTITY MANAGEMENT ---
static void initialize_emergent_entities(void) {
    serial_print("🧬 Initializing emergent entity pool...\n");
    HyperVector base_pattern = create_hyper_vector("GENOME_ADAPTIVE", 16);
    for (uint32_t i = 0; i < INITIAL_ENTITIES && i < MAX_ENTITIES; i++) {
        struct Entity* e = &entity_pool[active_entity_count];
        e->id = active_entity_count;
        e->state = create_hyper_vector("TRAIT_DORMANT", 14);
        e->genome = create_gene("base", base_pattern);
        e->gene_count = 1;
        e->age = 0;
        e->is_active = 1;
        e->confidence = 0.5f;
        e->mutation_rate = 50;
        strncpy(e->domain_name, "adaptive", 31);
        e->domain_name[31] = '\0';
        active_entity_count++;
        serial_print("  Entity ");
        print_hex(e->id);
        serial_print(" initialized.\n");
    }
    destroy_hyper_vector(&base_pattern);
}
static struct Entity* spawn_entity(void) {
    if (active_entity_count >= MAX_ENTITIES) {
        serial_print("Cannot spawn: Entity pool full.\n");
        return NULL;
    }
    struct Entity* new_entity = &entity_pool[active_entity_count];
    new_entity->id = active_entity_count;
    new_entity->age = 0;
    new_entity->interaction_count = 0;
    new_entity->is_active = 1;
    new_entity->fitness_score = 0;
    new_entity->spawn_count = 0;
    new_entity->marked_for_gc = 0;
    new_entity->is_mutant = 1;
    new_entity->gene_count = 0;
    new_entity->genome = NULL;
    new_entity->mutation_rate = 100;
    new_entity->state = create_hyper_vector("TRAIT_EMERGENT", strlen("TRAIT_EMERGENT") + 1);
    if (!new_entity->state.valid) {
        serial_print("[FATAL] Failed to create state for spawned entity. Halting.\n");
        while(1);
    }
    struct Gene* emergent_gene = create_gene("emergent", new_entity->state);
    if (!emergent_gene) {
        serial_print("[FATAL] Failed to create emergent gene for spawned entity. Halting.\n");
        while(1);
    }
    add_gene_to_entity(new_entity, emergent_gene);
    for (uint32_t i = 0; i < MAX_ENTITY_DOMAINS; i++) {
        new_entity->specialization_scores[i] = 0.1f;
    }
    new_entity->resource_allocation = 1.0f;
    new_entity->confidence = 0.5f;
    new_entity->task_alignment = 0.0f;
    strncpy(new_entity->domain_name, "spawned", 31);
    new_entity->domain_name[31] = '\0';
    active_entity_count++;
    serial_print("[SPAWN] SUCCESS: New adaptive entity ID ");
    print_hex(new_entity->id);
    serial_print(" with dynamic genome.\n");
    return new_entity;
}

// --- 🔥 CRITICAL: UPDATE_ENTITIES WITH VOLUNTARY SKIP (MERGED FIX) ---
static void update_entities(void) {
    memset(next_active, 0, sizeof(next_active));
    for (uint32_t i = 0; i < MAX_ENTITIES; i++) {
        next_state[i].valid = 0;
        next_domain[i][0] = '\0';
        next_task_vector[i].valid = 0;
        next_path_id[i] = 0;
        next_task_alignment[i] = 0.0f;
    }

    float pressure = get_system_memory_pressure();
    if (pressure > 0.8f) {
        serial_print("[MEMORY] 🌫️ High pressure (");
        print_hex((uint32_t)(pressure * 1000));
        serial_print("). Entities may skip update to preserve coherence.\n");
    }

    for (uint32_t i = 0; i < active_entity_count; i++) {
        struct Entity* entity = &entity_pool[i];
        next_active[i] = entity->is_active;
        strncpy(next_domain[i], entity->domain_name, 31);
        next_domain[i][31] = '\0';
        next_path_id[i] = entity->path_id;
        next_task_alignment[i] = entity->task_alignment;

        // VOLUNTARY SKIP: Low-fitness entities opt out under pressure
        if (pressure > 0.8f && entity->fitness_score < 30) {
            serial_print("[MEMORY] Entity ");
            print_hex(entity->id);
            serial_print(" 🕊️ volunteers to skip update (Fitness: ");
            print_hex(entity->fitness_score);
            serial_print(")\n");
            // Shallow carry-forward (safe: entity owns state)
            next_state[i] = entity->state;
            next_task_vector[i] = entity->task_vector;
            entity->age++;
            continue;
        }

        // SAFE DEEP COPY ONLY IF AFFORDABLE
        if (!can_afford_vector_copy(2)) {
            serial_print("[MEMORY] 🚫 Cannot afford deep-copy. Entity ");
            print_hex(entity->id);
            serial_print(" forced to skip.\n");
            next_state[i] = entity->state;
            next_task_vector[i] = entity->task_vector;
            entity->age++;
            continue;
        }

        // NORMAL DEEP-COPY PATH
        next_state[i] = copy_hyper_vector(&entity->state);
        next_task_vector[i] = copy_hyper_vector(&entity->task_vector);
        if (!next_state[i].valid || !next_task_vector[i].valid) {
            serial_print("[CRITICAL] Deep-copy failed for Entity ");
            print_hex(entity->id);
            serial_print(". Reverting to shallow state.\n");
            next_state[i] = entity->state;
            next_task_vector[i] = entity->task_vector;
        }
        entity->age++;

        // --- INTEGRATE ADVANCED EVOLUTION FROM .miss.txt ---
        // Listen to collective
        for (uint32_t t = 0; t < collective.thought_count; t++) {
            float similarity = compute_similarity(&entity->state, &collective.thought_space[t]);
            if (similarity > 0.6f) {
                entity->confidence += 0.05f * similarity;
                entity->resource_allocation += 0.1f;
                entity->fitness_score += 2;
                merge_hyper_vectors(&entity->state, &collective.thought_space[t]);
                serial_print("[RESONATE] Entity ");
                print_hex(entity->id);
                serial_print(" resonated with collective thought\n");
            }
        }

        // Neighbor-based activation/sleep
        uint32_t neighbor_active = 0;
        uint32_t prev_idx = (i == 0) ? (active_entity_count - 1) : (i - 1);
        uint32_t next_idx = (i == active_entity_count - 1) ? 0 : (i + 1);
        if (entity_pool[prev_idx].is_active) neighbor_active++;
        if (entity_pool[next_idx].is_active) neighbor_active++;

        if (!entity->is_active && neighbor_active > 0) {
            next_active[i] = 1;
            next_state[i] = create_hyper_vector("TRAIT_ACTIVE", strlen("TRAIT_ACTIVE") + 1);
            if (!next_state[i].valid) {
                serial_print("[WARNING] Failed to create active state for entity ");
                print_hex(entity->id);
                serial_print(". Skipping.\n");
                continue;
            }
            strncpy(next_domain[i], "reactor", 31);
            next_domain[i][31] = '\0';
            entity->interaction_count++;
            entity->fitness_score += 5;
            broadcast_thought(&next_state[i]);
            serial_print("[ACTIVATE] Entity ");
            print_hex(entity->id);
            serial_print(" activated by neighbor.\n");
        } else if (entity->is_active && neighbor_active == 0) {
            next_active[i] = 0;
            next_state[i] = create_hyper_vector("TRAIT_DORMANT", strlen("TRAIT_DORMANT") + 1);
            if (!next_state[i].valid) {
                serial_print("[WARNING] Failed to create dormant state for entity ");
                print_hex(entity->id);
                serial_print(". Skipping.\n");
                continue;
            }
            strncpy(next_domain[i], "sleeper", 31);
            next_domain[i][31] = '\0';
            entity->interaction_count++;
            serial_print("[SLEEP] Entity ");
            print_hex(entity->id);
            serial_print(" going dormant (no neighbors).\n");
        }

        // Self-modification trigger
        if (entity->confidence > 0.8f && entity->fitness_score > 50 && entity->mutation_rate > 100) {
            HyperVector before_state = create_hyper_vector("ENTITY_ACTIVE_FLAG", strlen("ENTITY_ACTIVE_FLAG") + 1);
            HyperVector after_state = create_hyper_vector("ENTITY_DORMANT_FLAG", strlen("ENTITY_DORMANT_FLAG") + 1);
            if (before_state.valid && after_state.valid) {
                uintptr_t target_address = (uintptr_t)&entity->is_active;
                propose_kernel_patch(entity, &before_state, &after_state, (uint32_t)target_address);
                entity->confidence = 0.5f;
                entity->fitness_score = 0;
                serial_print("[SELF-MOD] Entity ");
                print_hex(entity->id);
                serial_print(" proposed a kernel patch to change its own state.\n");
            }
        }
    }

    // Apply state (only destroy deep copies)
    for (uint32_t i = 0; i < active_entity_count; i++) {
        entity_pool[i].is_active = next_active[i];
        // Only destroy if it was a new allocation
        if (next_state[i].valid && next_state[i].data != entity_pool[i].state.data) {
            destroy_hyper_vector(&entity_pool[i].state);
        }
        entity_pool[i].state = next_state[i];
        strncpy(entity_pool[i].domain_name, next_domain[i], 31);
        entity_pool[i].domain_name[31] = '\0';
        if (next_task_vector[i].valid && next_task_vector[i].data != entity_pool[i].task_vector.data) {
            destroy_hyper_vector(&entity_pool[i].task_vector);
        }
        entity_pool[i].task_vector = next_task_vector[i];
        entity_pool[i].path_id = next_path_id[i];
        entity_pool[i].task_alignment = next_task_alignment[i];
    }
}

// --- VGA RENDER (from .miss.txt) ---
static void render_entities_to_vga(void) {
    volatile char* video = (volatile char*)VIDEO_MEMORY;
    for (int i = 0; i < 80*25*2; i += 2) {
        video[i] = ' ';
        video[i+1] = 0x00;
    }
    for (uint32_t i = 0; i < active_entity_count && i < 25; i++) {
        struct Entity* e = &entity_pool[i];
        char symbol = e->is_active ? '●' : '○';
        uint8_t color = e->confidence > 0.8f ? 0x0A : 
                      e->confidence > 0.5f ? 0x0F : 0x0C;
        int row = i;
        int col = 0;
        video[(row*80 + col)*2] = symbol;
        video[(row*80 + col)*2 + 1] = color;
        char id_str[4];
        id_str[0] = '0' + (e->id / 10);
        id_str[1] = '0' + (e->id % 10);
        id_str[2] = ' ';
        id_str[3] = '\0';
        for (int j = 0; j < 3; j++) {
            video[(row*80 + col + 2 + j)*2] = id_str[j];
            video[(row*80 + col + 2 + j)*2 + 1] = color;
        }
        int bar_len = e->fitness_score / 10;
        if (bar_len > 20) bar_len = 20;
        for (int j = 0; j < bar_len; j++) {
            video[(row*80 + col + 6 + j)*2] = '█';
            video[(row*80 + col + 6 + j)*2 + 1] = color;
        }
    }
    char coh_str[20];
    int coh_int = (int)(collective.global_coherence * 100);
    coh_str[0] = 'C'; coh_str[1] = ':'; coh_str[2] = ' ';
    coh_str[3] = '0' + (coh_int / 10);
    coh_str[4] = '0' + (coh_int % 10);
    coh_str[5] = '%';
    coh_str[6] = '\0';
    for (int i = 0; i < 7; i++) {
        video[(24*80 + 70 + i)*2] = coh_str[i];
        video[(24*80 + 70 + i)*2 + 1] = 0x0E;
    }
}

// --- SELF-MODIFICATION ---
static void apply_kernel_patch(KernelPatch* patch) {
    if (patch->applied) return;
    uint32_t total_size = patch->replacement.active_dims * sizeof(float);
    if (total_size > 1024) {
        serial_print("[ERROR] Patch too large for safety check.\n");
        return;
    }
    if (patch->address < (uint32_t)kernel_heap || patch->address >= (uint32_t)(kernel_heap + KERNEL_HEAP_SIZE)) {
        serial_print("[ERROR] Kernel patch target address outside safe heap range.\n");
        return;
    }
    uint8_t* target = (uint8_t*)patch->address;
    for (uint32_t i = 0; i < total_size; i++) {
        if (i < (patch->replacement.capacity * sizeof(float))) {
            target[i] = ((uint8_t*)patch->replacement.data)[i];
        } else {
            break;
        }
    }
    patch->applied = 1;
    serial_print("KERNEL PATCH APPLIED AT 0x");
    print_hex(patch->address);
    serial_print("\n");
}
static void propose_kernel_patch(struct Entity* entity, HyperVector* old_pattern, HyperVector* new_pattern, uint32_t address) {
    if (!entity || !old_pattern || !new_pattern) return;
    KernelPatch patch = {0};
    patch.pattern = copy_hyper_vector(old_pattern);
    patch.replacement = copy_hyper_vector(new_pattern);
    patch.address = address;
    patch.applied = 0;
    encode_holographic_memory(&patch.pattern, &patch.replacement);
    broadcast_thought(&patch.replacement);
    serial_print("[PROPOSE] Entity ");
    print_hex(entity->id);
    serial_print(" proposed a patch at 0x");
    print_hex(address);
    serial_print("\n");
}

// --- KERNEL ENTRY ---
void kmain(void) __attribute__((noreturn));
void kmain(void) {
    volatile char* vga = (volatile char*)VIDEO_MEMORY;
    for (int i = 0; i < 5; i++) { vga[i*2] = "HYPER"[i]; vga[i*2+1] = 0x0F; }
    serial_init();
    __asm__ volatile ("cli");
    serial_print("🌌 Holographic Kernel with Emergent Memory Economy + Advanced Evolution Starting...\n");
    print("🌌 Holographic Kernel with Emergent Memory Economy + Advanced Evolution Starting...\n");
    initialize_holographic_memory();
    initialize_collective_consciousness();
    load_initial_genome_vocabulary();
    initialize_emergent_entities();
    print("✅ System online. Entities managing memory, evolving, and self-modifying.\n");
    serial_print("[BOOT] 🚀 HyperKernel fully initialized. Emergent economy active.\n");
    uint32_t last_update = 0;
    while (1) {
        if (holo_system.global_timestamp - last_update > 500000) {
            update_entities();
            render_entities_to_vga();
            last_update = holo_system.global_timestamp;
        }
        holo_system.global_timestamp++;
        __asm__ volatile("hlt");
    }
}
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    serial_write('S'); serial_write('E'); serial_write('R'); serial_write('\n');
}
// Line 969
