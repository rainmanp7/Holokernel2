// ============================================================================
// HOLOGRAPHIC KERNEL WITH EMERGENT MEMORY ECONOMY — VGA-ONLY BOOT DIAGNOSTICS
// ============================================================================
// All boot output goes to VGA. No serial dependency.
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
// --- NEW: Pre-allocated metadata pool ---
#define MAX_ALLOCATIONS 512 // Adjust based on expected max allocations
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
// --- METADATA BLOCK STRUCTURE ---
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
// --- NEW: Pre-allocated metadata pool and free list ---
static mem_block_t metadata_pool[MAX_ALLOCATIONS];
static mem_block_t* free_metadata_list = NULL;
static mem_block_t* allocation_list = NULL; // Head of active allocations
static uint32_t allocation_counter = 0;
// --- NEW: Track garbage collected metadata blocks ---
static uint32_t gc_metadata_count = 0;
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
static void print_char_vga(char c, uint8_t color, int row, int col); // New: Direct row/col write
static void print_str_vga(const char* str, uint8_t color, int row, int start_col);
static void print_hex_vga(uint32_t value, uint8_t color, int row, int start_col);
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
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    // Do NOT print 'SER' here — we want pure VGA output
}
// ============================================================================
// --- VGA TEXT MODE OUTPUT (DIRECT WRITE) ---
// ============================================================================
static void print_char_vga(char c, uint8_t color, int row, int col) {
    volatile char* video = (volatile char*)VIDEO_MEMORY;
    if (row < 0 || row >= 25 || col < 0 || col >= 80) return;
    int pos = (row * 80 + col) * 2;
    video[pos] = c;
    video[pos + 1] = color;
}
static void print_str_vga(const char* str, uint8_t color, int row, int start_col) {
    int col = start_col;
    while (*str && col < 80) {
        print_char_vga(*str, color, row, col);
        str++;
        col++;
    }
}
static void print_hex_vga(uint32_t value, uint8_t color, int row, int start_col) {
    char hex_chars[] = "0123456789ABCDEF";
    int col = start_col;
    print_char_vga('0', color, row, col++); // '0'
    print_char_vga('x', color, row, col++); // 'x'
    for (int i = 0; i < 8; i++) {
        int shift = (7 - i) * 4;
        char digit = hex_chars[(value >> shift) & 0xF];
        print_char_vga(digit, color, row, col++);
    }
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
// --- MEMORY ALLOCATION (EMERGENT ECONOMY - FIXED METADATA) ---
// ============================================================================
// --- NEW: Initialize the metadata pool ---
static void initialize_metadata_pool() {
    for (int i = 0; i < MAX_ALLOCATIONS - 1; i++) {
        metadata_pool[i].next = &metadata_pool[i + 1];
    }
    metadata_pool[MAX_ALLOCATIONS - 1].next = NULL; // Last one points to NULL
    free_metadata_list = &metadata_pool[0]; // Start with first block as free
}
static void* kmalloc(size_t size) {
    if (size == 0) return NULL; // Handle zero-size allocation
    // FIXED: Correct 4-byte alignment for i386
    size_t aligned_size = (size + 3) & ~3;
    if (heap_offset + aligned_size >= KERNEL_HEAP_SIZE) {
        // Print error to VGA
        print_str_vga("[CRITICAL] kmalloc FAILED! Requested: ", 0x0C, 15, 0);
        print_hex_vga(size, 0x0C, 15, 35);
        print_str_vga(" bytes. Free space: ", 0x0C, 15, 50);
        print_hex_vga(KERNEL_HEAP_SIZE - heap_offset, 0x0C, 15, 65);
        print_str_vga(" bytes.", 0x0C, 15, 75);
        return NULL;
    }
    // --- NEW: Get metadata block from pre-allocated pool ---
    mem_block_t* block = free_metadata_list;
    if (!block) {
        print_str_vga("[CRITICAL] kmalloc FAILED! No metadata blocks available.", 0x0C, 16, 0);
        return NULL; // Cannot track allocation without metadata
    }
    free_metadata_list = block->next; // Remove from free list
    void* ptr = &kernel_heap[heap_offset];
    heap_offset += aligned_size;
    // Initialize the metadata block
    block->ptr = ptr;
    block->size = aligned_size;
    block->allocation_id = allocation_counter++;
    block->owner = NULL;
    block->is_garbage = 0;
    // Add to active allocation list
    block->next = allocation_list;
    allocation_list = block;
    return ptr;
}
static void kfree(void* ptr) {
    if (!ptr) return;
    mem_block_t* current = allocation_list;
    mem_block_t* prev = NULL;
    while (current) {
        if (current->ptr == ptr) {
            // --- NEW: Mark as garbage, don't immediately free metadata ---
            current->is_garbage = 1;
            // Optionally, trigger GC if pressure is high
            if ((float)(KERNEL_HEAP_SIZE - heap_offset) / (float)KERNEL_HEAP_SIZE < 0.2f) {
                perform_emergent_garbage_collection();
            }
            return; // Found and marked, exit
        }
        prev = current;
        current = current->next;
    }
    // If we get here, ptr was not found in the allocation list
    print_str_vga("[WARNING] kfree: Attempted to free unallocated pointer: 0x", 0x0C, 17, 0);
    print_hex_vga((uint32_t)ptr, 0x0C, 17, 40);
}
static float get_system_memory_pressure(void) {
    // ORIGINAL CALCULATION IS CORRECT AND SIMPLE
    return 1.0f - ((float)(KERNEL_HEAP_SIZE - heap_offset) / (float)KERNEL_HEAP_SIZE);
}
static void perform_emergent_garbage_collection(void) {
    print_str_vga("[MEMORY] 🌱 Initiating Emergent Garbage Collection...", 0x0E, 18, 0);
    HyperVector pressure_thought = create_hyper_vector("SYSTEM_MEMORY_PRESSURE", 22);
    if (pressure_thought.valid) {
        broadcast_thought(&pressure_thought);
        destroy_hyper_vector(&pressure_thought);
    }
    uint32_t entities_deactivated = 0;
    uint32_t genes_destroyed = 0;
    // --- NEW: Process garbage list and return metadata to pool ---
    mem_block_t* current = allocation_list;
    mem_block_t* prev = NULL;
    while (current) {
        if (current->is_garbage) {
            // Free the actual data block memory
            // Since we manage a simple linear heap, we don't "free" the data in the traditional sense.
            // We rely on the emergent entities to manage pressure.
            // However, we *can* reclaim the metadata block.
            if (prev) {
                prev->next = current->next;
            } else {
                allocation_list = current->next; // Update head if necessary
            }
            mem_block_t* to_reclaim = current;
            current = current->next; // Move to next before reclaiming
            // Add metadata block back to the free pool
            to_reclaim->next = free_metadata_list;
            free_metadata_list = to_reclaim;
            gc_metadata_count++; // Increment the counter
        } else {
            prev = current;
            current = current->next;
        }
    }
    print_str_vga("[MEMORY] 🔄 GC Complete. Metadata blocks reclaimed.", 0x0E, 19, 0);
}
// ============================================================================
// --- HYPERVECTOR SYSTEM ---
// ============================================================================
static uint32_t hash_data(const void* input, uint32_t size) {
    if (!input || size == 0) return 0; // Add safety check
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
    if (!input || size == 0) { // Add safety check
        vec.valid = 0;
        return vec;
    }
    vec.capacity = INITIAL_DIMENSIONS;
    vec.data = (float*)kmalloc(vec.capacity * sizeof(float));
    if (!vec.data) {
        print_str_vga("[ERROR] create_hyper_vector: Out of memory!", 0x0C, 15, 0);
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
        kfree(vec->data); // This calls kfree, which might trigger GC, but GC now handles metadata safely
        vec->data = NULL;
        vec->capacity = 0;    // ADD THIS: Clear metadata
        vec->active_dims = 0; // ADD THIS: Clear metadata
        vec->valid = 0;       // ADD THIS: Mark invalid
    }
}
static float compute_similarity(HyperVector* a, HyperVector* b) {
    // ENHANCED NULL/BOUNDS CHECKING
    if (!a || !b || !a->valid || !b->valid || !a->data || !b->data) return 0.0f;
    if (a->active_dims == 0 || b->active_dims == 0) return 0.0f; // ADD THIS: Check for empty vectors
    uint32_t min_dims = (a->active_dims < b->active_dims) ? a->active_dims : b->active_dims;
    // ADD SAFETY CAP (This is implicitly handled by min_dims calculation, but let's be explicit about capacity)
    if (min_dims > a->capacity || min_dims > b->capacity) {
        // This should ideally not happen if active_dims <= capacity, but as a safeguard:
        min_dims = (a->capacity < b->capacity) ? a->capacity : b->capacity;
    }
    if (min_dims == 0) return 0.0f; // Safety check after capacity adjustment
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
        print_str_vga("[ERROR] create_gene: Out of memory!", 0x0C, 15, 0);
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
    print_str_vga("[COLLECTIVE] 🌀 Consciousness initialized", 0x0A, 10, 0);
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
    print_str_vga("[BROADCAST] 💭 Thought added, coherence: ", 0x0E, 11, 0);
    print_hex_vga((uint32_t)(coherence * 1000), 0x0E, 11, 35);
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
    print_str_vga("🧠 Holographic memory system online.", 0x0B, 12, 0);
}
// ============================================================================
// --- ENTITY MANAGEMENT ---
// ============================================================================
static void initialize_emergent_entities(void) {
    print_str_vga("🧬 Initializing emergent entity pool...", 0x0D, 13, 0);
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
        print_str_vga("  Entity ", 0x0F, 14, 0);
        print_hex_vga(e->id, 0x0F, 14, 8);
        print_str_vga(" initialized.", 0x0F, 14, 16);
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
    // BOUNDARY CHECK ADDED (CORRECTLY)
    uint32_t safe_count = (active_entity_count < MAX_ENTITIES) ? active_entity_count : MAX_ENTITIES;
    for (uint32_t i = 0; i < safe_count; i++) {
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
                print_str_vga("[MEMORY] Entity ", 0x0C, 15, 0);
                print_hex_vga(e->id, 0x0C, 15, 15);
                print_str_vga(" proactively sacrifices gene: ", 0x0C, 15, 25);
                print_str_vga(weakest->name, 0x0C, 15, 50);
                destroy_hyper_vector(&weakest->pattern);
                kfree(weakest);
                e->gene_count--;
            }
        }
    }
    for (uint32_t i = 0; i < safe_count; i++) {
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
    // --- NEW: Initialize metadata pool first ---
    initialize_metadata_pool();
    volatile char* vga = (volatile char*)VIDEO_MEMORY;
    // Clear screen
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 80; col++) {
            vga[(row * 80 + col) * 2] = ' ';
            vga[(row * 80 + col) * 2 + 1] = 0x00;
        }
    }
    // Print HYPER at top
    for (int i = 0; i < 5; i++) {
        vga[i * 2] = "HYPER"[i];
        vga[i * 2 + 1] = 0x0F;
    }
    serial_init(); // Configure serial port, but do not print anything to it
    __asm__ volatile ("cli");
    // Print boot message to VGA
    print_str_vga("🌌 Holographic Kernel with Emergent Memory Economy Starting...", 0x0E, 5, 0);
    // Initialize systems
    print_str_vga("Initializing holographic memory...", 0x0B, 6, 0);
    initialize_holographic_memory();
    print_str_vga("Initializing collective consciousness...", 0x0A, 7, 0);
    initialize_collective_consciousness();
    print_str_vga("Initializing emergent entities...", 0x0D, 8, 0);
    initialize_emergent_entities();
    print_str_vga("✅ System online. Entities managing memory.", 0x0F, 9, 0);
    // Boot complete indicator
    print_str_vga("[BOOT] 🚀 HyperKernel fully initialized. Emergent economy active.", 0x0F, 10, 0);
    // Test stack integrity
    uint32_t test_val = 0xDEADBEEF;
    __asm__ volatile (
        "pushl %0
\t"
        "popl %%eax
\t"
        "cmp %0, %%eax
\t"
        "jne stack_error
\t"
        "jmp stack_ok
\t"
        "stack_error:
\t"
        "mov $0x0C, %%al
\t"
        "mov $0x00, %%dl
\t"
        "out dx, al
\t"
        "stack_ok:
\t"
        :
        : "r"(test_val)
        : "eax"
    );
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
            // Header with Timestamp
            print_str_vga("=== EMERGENT SYSTEM REPORT === TS:0x", 0x0E, 20, 0);
            print_hex_vga(holo_system.global_timestamp, 0x0E, 20, 28);
            // Memory pressure
            float pressure = get_system_memory_pressure();
            uint32_t press_pct = (uint32_t)(pressure * 100);
            char mem_str[32] = "Mem:      %";
            uint_to_str(press_pct, mem_str + 5, 2);
            print_str_vga(mem_str, 0x0F, 21, 0);
            // Active entities
            char ent_str[32] = "Ent:     ";
            uint_to_str(active_entity_count, ent_str + 5, 2);
            print_str_vga(ent_str, 0x0A, 22, 0);
            // Coherence
            uint32_t coh_pct = (uint32_t)(collective.global_coherence * 100);
            char coh_str[32] = "Coh:      %";
            uint_to_str(coh_pct, coh_str + 5, 2);
            print_str_vga(coh_str, 0x0B, 23, 0);
            // Cycle, GC Count, Timestamp
            char cyc_str[32] = "Cycle:   /4";
            cyc_str[7] = '0' + (report_cycle % 10);
            cyc_str[8] = '\0';
            print_str_vga(cyc_str, 0x0C, 24, 0);
            print_str_vga(" GC:", 0x0C, 24, 10);
            char gc_buf[16];
            uint_to_str(gc_metadata_count, gc_buf, 6);
            print_str_vga(gc_buf, 0x0C, 24, 14);
            last_report = holo_system.global_timestamp;
            report_cycle++;
        }
        holo_system.global_timestamp++;
        __asm__ volatile("hlt");
    }
}
