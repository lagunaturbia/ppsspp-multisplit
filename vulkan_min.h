#pragma once
/* Minimal Vulkan 1.x declarations, Windows x64. Handles are pointer-sized. */
typedef void *VK; typedef uint32_t U;typedef int32_t I;typedef uint64_t Q;typedef void *(*GPA)(VK,const char*);
struct App {U type;const void*next;const char*app;U appver;const char*engine;U enginever,api;};
struct InstanceCI {U type;const void*next;U flags;const struct App*app;U layers;const char*const*layer;U exts;const char*const*ext;};
struct Ext {char name[256];U version;};
struct Range {U aspect,baseMip,levels,baseLayer,layers;};
struct ViewCI {U type;const void*next;U flags;VK image;U viewType,format;U swizzle[4];struct Range range;};
struct RetroImage {VK view;U layout;struct ViewCI ci;};
struct Context {VK gpu,device,queue;U queue_index;VK present;U present_index;};
struct Neg {U type,version;const struct App*(*app)(void);bool (*create)(struct Context*,VK,VK,VK,GPA,const char**,U,const char**,U,const void*);void (*destroy)(void);void*create_instance;void*create_device2;};
struct Interface {U type,version;void*handle;VK instance,gpu,device;GPA gdpa,gipa;VK queue;U qindex;void (*set_image)(void*,const struct RetroImage*,U,const VK*,U);U (*index)(void*);U (*mask)(void*);void (*set_cmds)(void*,U,const VK*);void (*wait)(void*);void (*lock)(void*);void (*unlock)(void*);void (*signal)(void*,VK);};
struct MemType {U flags,heap;};struct Heap {Q size;U flags;};struct MemProps {U count;struct MemType types[32];U heaps;struct Heap heap[16];};
struct Requirements {Q size,alignment;U bits;};
struct BufferCI {U type;const void*next;U flags;Q size;U usage,sharing,count;const U*families;};
struct Alloc {U type;const void*next;Q size;U memoryType;};
struct PoolCI {U type;const void*next;U flags,family;};
struct CmdAlloc {U type;const void*next;VK pool;U level,count;};
struct CmdBegin {U type;const void*next;U flags;const void*inherit;};
struct ImageBarrier {U type;const void*next;U src,dst,oldLayout,newLayout,srcFamily,dstFamily;VK image;struct Range range;};
struct BufferBarrier {U type;const void*next;U src,dst,srcFamily,dstFamily;VK buffer;Q offset,size;};
struct Copy {Q offset;U rowLength,imageHeight;struct {U aspect,mip,baseLayer,layers;} sub;I xyz[3];U extent[3];};
struct Submit {U type;const void*next;U waitCount;const VK*wait;const U*stages;U cmdCount;const VK*cmd;U signalCount;const VK*signal;};
_Static_assert(sizeof(struct ViewCI)==80 && sizeof(struct RetroImage)==96 && offsetof(struct Interface,set_image)==72,"Vulkan/libretro ABI");
_Static_assert(sizeof(struct MemProps)==520 && sizeof(struct Copy)==56 && sizeof(struct Submit)==72 && sizeof(struct ImageBarrier)==72,"Vulkan ABI structures");
#define VK_OK 0
#define VK_API_1_0 (1u<<22)
#define VK_API_1_4 ((1u<<22)|(4u<<12))
#define VK_IGNORED 0xffffffffu
#define VK_GENERAL 1u
#define VK_SHADER_READ 5u
#define VK_TRANSFER_SRC 6u
#define VK_ACCESS_MEMORY_WRITE 0x10000u
#define VK_ACCESS_MEMORY_READ 0x8000u
#define VK_ACCESS_TRANSFER_READ 0x800u
#define VK_ACCESS_TRANSFER_WRITE 0x1000u
#define VK_ACCESS_HOST_READ 0x2000u
#define VK_STAGE_ALL 0x10000u
#define VK_STAGE_TRANSFER 0x1000u
#define VK_STAGE_HOST 0x4000u
#define VK_BGRA8 44u
#define VK_BGRA8_SRGB 50u
#define VK_RGBA8 37u
#define VK_RGBA8_SRGB 43u
