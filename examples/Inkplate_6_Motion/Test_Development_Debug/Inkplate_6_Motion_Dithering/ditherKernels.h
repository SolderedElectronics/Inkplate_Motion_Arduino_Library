// Dithering Kernel
typedef struct {
    int8_t x_offset;  // X offset from the current pixel
    int8_t y_offset;  // Y offset from the current pixel
    uint8_t weight;   // Weight of the error to propagate (normalized to sum = 16)
} KernelElement;

const KernelElement FS_KERNEL[] = {
    {1, 0, 7},  // Right
    {-1, 1, 3}, // Bottom-left
    {0, 1, 5},  // Bottom
    {1, 1, 1}   // Bottom-right
};
const KernelElement JJN_KERNEL[] = {
    {1, 0, 7}, {2, 0, 5},  // Current row
    {-2, 1, 3}, {-1, 1, 5}, {0, 1, 7}, {1, 1, 5}, {2, 1, 3}, // Next row
    {-2, 2, 1}, {-1, 2, 3}, {0, 2, 5}, {1, 2, 3}, {2, 2, 1}  // Two rows down
};
const KernelElement STUCKI_KERNEL[] = {
    {1, 0, 8}, {2, 0, 4},  // Current row
    {-2, 1, 2}, {-1, 1, 4}, {0, 1, 8}, {1, 1, 4}, {2, 1, 2}, // Next row
    {-2, 2, 1}, {-1, 2, 2}, {0, 2, 4}, {1, 2, 2}, {2, 2, 1}  // Two rows down
};
const KernelElement SIERRA_KERNEL[] = {
    {1, 0, 5}, {2, 0, 3},  // Current row
    {-2, 1, 2}, {-1, 1, 4}, {0, 1, 5}, {1, 1, 4}, {2, 1, 2}, // Next row
    {-1, 2, 2}, {0, 2, 3}, {1, 2, 2}                         // Two rows down
};
const KernelElement SIERRA_LITE_KERNEL[] = {
    {1, 0, 2},  // Right
    {-1, 1, 1}, {0, 1, 1}, {1, 1, 1} // Bottom row
};
const KernelElement ATKINSON_KERNEL[] = {
    {1, 0, 1}, {2, 0, 1},       // Current row
    {-1, 1, 1}, {0, 1, 1}, {1, 1, 1}, // Next row
    {0, 2, 1}                   // Two rows down
};
const KernelElement BURKES_KERNEL[] = {
    {1, 0, 8}, {2, 0, 4},  // Current row
    {-2, 1, 2}, {-1, 1, 4}, {0, 1, 8}, {1, 1, 4}, {2, 1, 2}  // Next row
};

// Number of elements in the kernel
#define FS_KERNEL_SIZE (sizeof(FS_KERNEL) / sizeof(FS_KERNEL[0]))
#define JJN_KERNEL_SIZE (sizeof(JJN_KERNEL) / sizeof(JJN_KERNEL[0]))
#define STUCKI_KERNEL_SIZE (sizeof(STUCKI_KERNEL) / sizeof(STUCKI_KERNEL[0]))
#define SIERRA_KERNEL_SIZE (sizeof(SIERRA_KERNEL) / sizeof(SIERRA_KERNEL[0]))
#define SIERRA_LITE_KERNEL_SIZE (sizeof(SIERRA_LITE_KERNEL) / sizeof(SIERRA_LITE_KERNEL[0]))
#define ATKINSON_KERNEL_SIZE (sizeof(ATKINSON_KERNEL) / sizeof(ATKINSON_KERNEL[0]))
#define BURKES_KERNEL_SIZE (sizeof(BURKES_KERNEL) / sizeof(BURKES_KERNEL[0]))