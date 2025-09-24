// ============================================================================
// HOLOGRAPHIC KERNEL WITH EMERGENT MEMORY ECONOMY
// ============================================================================
// Entities trade, sacrifice, and evolve to manage memory as a living system.
// No traditional malloc/free — memory is governed by artificial life.
// ============================================================================

// ============================================================================
// --- TYPE DEFINITIONS ---
// ============================================================================
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned int    size_t;
typedef unsigned long   uintptr_t;
#ifndef NULL
#define NULL ((void *)0)
#endif

// ============================================================================
// --- CONFIGURATION ---
// ============================================================================
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

// ============================================================================
// --- STRUCTURE DEFINITIONS ---
// ============================================================================
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

typedef struct mem_block {
    void* ptr;
    size_t size;
    uint32_t allocation_id;
    struct Entity* owner;
    uint8_t is_garbage;
    struct mem_block* next;
} mem_block_t;

// ============================================================================
// --- KERNEL HEAP (allocated by linker at 0xA0000) ---
// ============================================================================
extern uint8_t kernel_heap_start[];
static uint8_t* kernel_heap = kernel_heap_start;
static uint32_t heap_offset = 0;
static mem_block_t* allocation_list = NULL;
static uint32_t allocation_counter = 0;

// ============================================================================
// --- GLOBAL VARIABLES ---
// ============================================================================
static struct Entity entity_pool[MAX_ENTITIES];
static uint32_t active_entity_count = 0;
static struct HolographicSystem holo_system = {0};
static struct CollectiveConsciousness collective = {0};
static uint32_t report_cycle = 0; // For periodic VGA reports

// ============================================================================
// --- FUNCTION PROTOTYPES ---
// ============================================================================
// Memory
static void* kmalloc(size_t size);
static void kfree(void* ptr);
static void perform_emergent_garbage_collection(void);
static float get_system_memory_pressure(void);

// HyperVector
static uint32_t hash_data(const void* input, uint32_t size);
static HyperVector create_hyper_vector(const void* input, uint32_t size);
static HyperVector copy_hyper_vector(const HyperVector* src);
static void destroy_hyper_vector(HyperVector* vec);
static float compute_similarity(HyperVector* a, HyperVector* b);

// Genome
static struct Gene* create_gene(const char* name, HyperVector pattern);
static void destroy_genome(struct Gene* genome);

// Collective Consciousness
static void initialize_collective_consciousness(void);
static void broadcast_thought(HyperVector* thought);
static float compute_coherence(HyperVector* thought);

// Holographic Memory
static void initialize_holographic_memory(void);

// Entity Management
static void initialize_emergent_entities(void);
static void update_entities(void);

// I/O
static void serial_init(void);
static void serial_write(char c);
static void serial_print(const char* str);
static void print_char(char c, uint8_t color);
static void print(const char* str);
static void print_hex(uint32_t value);

// Port I/O
static inline void outb(uint16_t port, uint8_t value);
static inline uint8_t inb(uint16_t port);

// Utilities
static void* memcpy(void* dest, const void* src, size_t n);
static void* memset(void* ptr, int value, size_t n);
static float sqrtf(float x);
static size_t strlen(const char *str);
static char *strncpy(char *dest, const char *src, size_t n);
static void uint_to_str(uint32_t num, char* buf, uint8_t width);

// ============================================================================
// --- PORT I/O ---
// ============================================================================
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ============================================================================
// --- SERIAL I/O ---
// ============================================================================
static void serial_write(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

static void serial_print(const char* str) {
    while (*str) serial_write(*str++);
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

// ============================================================================
// --- VGA TEXT MODE OUTPUT ---
// ============================================================================
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

// ============================================================================
// --- UTILITIES ---
// ============================================================================
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

// --- MINIMAL INTEGER TO STRING (for bare-metal) ---
static void uint_to_str(uint32_t num, char* buf, uint8_t width) {
    for (int i = 0; i < width; i++) buf[i] = ' ';
    buf[width] = '\0';
    if (num == 0) {
        buf[width - 1] = '0';
        return;
    }
    int pos = width - 1;
    while (num > 0 && pos >= 0) {
        buf[pos] = '0' + (num % 10);
        num /= 10;
        pos--;
    }
}

// ============================================================================
// --- MEMORY ALLOCATION (EMERGENT ECONOMY) ---
// ============================================================================
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
    mem_block_t* block = (mem_block_t*)kmalloc(sizeof(mem_block_t));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->allocation_id = allocation_counter++;
        block->owner = NULL;
        block->is_garbage = 0;
        block->next = allocation_list;
        allocation_list = block;
    } else {
        serial_print("[WARNING] Failed to track allocation metadata.\n");
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

// ============================================================================
// --- HYPERVECTOR SYSTEM ---
// ============================================================================
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

// ============================================================================
// --- GENOME SYSTEM ---
// ============================================================================
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

static void destroy_genome(struct Gene* genome) {
    struct Gene* current = genome;
    while (current) {
        struct Gene* next = current->next;
        destroy_hyper_vector(&current->pattern);
        kfree(current);
        current = next;
    }
}

// ============================================================================
// --- COLLECTIVE CONSCIOUSNESS ---
// ============================================================================
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

// ============================================================================
// --- HOLOGRAPHIC MEMORY ---
// ============================================================================
static void initialize_holographic_memory(void) {
    holo_system.memory_count = 0;
    holo_system.global_timestamp = 0;
    for (uint32_t i = 0; i < MAX_MEMORY_ENTRIES; i++) {
        holo_system.memory_pool[i].valid = 0;
    }
    print("🧠 Holographic memory system online.\n");
}

// ============================================================================
// --- ENTITY MANAGEMENT ---
// ============================================================================
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

static void update_entities(void) {
    static uint8_t next_active[MAX_ENTITIES];
    static HyperVector next_state[MAX_ENTITIES];
    static char next_domain[MAX_ENTITIES][32];
    memset(next_active, 0, sizeof(next_active));
    for (uint32_t i = 0; i < MAX_ENTITIES; i++) {
        next_state[i].valid = 0;
        next_domain[i][0] = '\0';
    }
    for (uint32_t i = 0; i < active_entity_count; i++) {
        struct Entity* e = &entity_pool[i];
        next_active[i] = e->is_active;
        next_state[i] = copy_hyper_vector(&e->state);
        strncpy(next_domain[i], e->domain_name, 31);
        next_domain[i][31] = '\0';
        e->age++;
        if (e->age % 500 == 0 && e->gene_count > 1 && get_system_memory_pressure() > 0.7f) {
            struct Gene* weakest = NULL;
            struct Gene* current = e->genome;
            while (current) {
                if (!weakest || current->fitness < weakest->fitness) weakest = current;
                current = current->next;
            }
            if (weakest) {
                serial_print("[MEMORY] Entity ");
                print_hex(e->id);
                serial_print(" proactively sacrifices gene: ");
                serial_print(weakest->name);
                serial_print("\n");
                destroy_hyper_vector(&weakest->pattern);
                kfree(weakest);
                e->gene_count--;
            }
        }
    }
    for (uint32_t i = 0; i < active_entity_count; i++) {
        entity_pool[i].is_active = next_active[i];
        entity_pool[i].state = next_state[i];
        strncpy(entity_pool[i].domain_name, next_domain[i], 31);
        entity_pool[i].domain_name[31] = '\0';
    }
}

// ============================================================================
// --- KERNEL ENTRY POINT ---
// ============================================================================
void kmain(void) __attribute__((noreturn));
void kmain(void) {
    volatile char* vga = (volatile char*)VIDEO_MEMORY;
    for (int i = 0; i < 5; i++) { vga[i*2] = "HYPER"[i]; vga[i*2+1] = 0x0F; }
    serial_init();
    __asm__ volatile ("cli");
    serial_print("🌌 Holographic Kernel with Emergent Memory Economy Starting...\n");
    print("🌌 Holographic Kernel with Emergent Memory Economy Starting...\n");
    initialize_holographic_memory();
    initialize_collective_consciousness();
    initialize_emergent_entities();
    print("✅ System online. Entities managing memory.\n");
    serial_print("[BOOT] 🚀 HyperKernel fully initialized. Emergent economy active.\n");

    uint32_t last_update = 0;
    uint32_t last_report = 0;
    const uint32_t REPORT_INTERVAL = 3000000; // ~3 seconds
    const uint32_t MAX_REPORTS = 4;

    while (1) {
        // Update entities every ~0.5 sec
        if (holo_system.global_timestamp - last_update > 500000) {
            update_entities();
            last_update = holo_system.global_timestamp;
        }

        // Print system report every ~3 sec, max 4 times
        if (report_cycle < MAX_REPORTS && 
            holo_system.global_timestamp - last_report > REPORT_INTERVAL) {
            
            // Clear lines 20–24
            for (int row = 20; row < 25; row++) {
                for (int col = 0; col < 80; col++) {
                    vga[(row * 80 + col) * 2] = ' ';
                    vga[(row * 80 + col) * 2 + 1] = 0x00;
                }
            }

            // Header
            const char* header = "=== EMERGENT SYSTEM REPORT ===";
            for (int i = 0; i < 30 && header[i]; i++) {
                vga[(20 * 80 + i) * 2] = header[i];
                vga[(20 * 80 + i) * 2 + 1] = 0x0E;
            }

            // Memory pressure
            float pressure = get_system_memory_pressure();
            uint32_t press_pct = (uint32_t)(pressure * 100);
            char mem_str[32] = "Mem:      %";
            uint_to_str(press_pct, mem_str + 5, 2);
            for (int i = 0; mem_str[i]; i++) {
                vga[(21 * 80 + i) * 2] = mem_str[i];
                vga[(21 * 80 + i) * 2 + 1] = 0x0F;
            }

            // Active entities
            char ent_str[32] = "Ent:     ";
            uint_to_str(active_entity_count, ent_str + 5, 2);
            for (int i = 0; ent_str[i]; i++) {
                vga[(22 * 80 + i) * 2] = ent_str[i];
                vga[(22 * 80 + i) * 2 + 1] = 0x0A;
            }

            // Coherence
            uint32_t coh_pct = (uint32_t)(collective.global_coherence * 100);
            char coh_str[32] = "Coh:      %";
            uint_to_str(coh_pct, coh_str + 5, 2);
            for (int i = 0; coh_str[i]; i++) {
                vga[(23 * 80 + i) * 2] = coh_str[i];
                vga[(23 * 80 + i) * 2 + 1] = 0x0B;
            }

            // Cycle
            char cyc_str[20] = "Cycle:   /4";
            cyc_str[7] = '0' + report_cycle;
            for (int i = 0; cyc_str[i]; i++) {
                vga[(24 * 80 + i) * 2] = cyc_str[i];
                vga[(24 * 80 + i) * 2 + 1] = 0x0C;
            }

            serial_print("[REPORT] System snapshot printed to VGA.\n");
            last_report = holo_system.global_timestamp;
            report_cycle++;
        }

        holo_system.global_timestamp++;
        __asm__ volatile("hlt");
    }
}
