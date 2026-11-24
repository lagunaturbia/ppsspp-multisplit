#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define WIN __stdcall
#define API __declspec(dllimport)
#define EXP __declspec(dllexport)
typedef void *H; typedef unsigned long DWORD; typedef int BOOL; typedef unsigned short WORD;
#define BAD_HANDLE ((H)(intptr_t)-1)
#define WAIT_TIMEOUT 258u
#define WAIT_FAILED 0xffffffffu
#define INF 0xffffffffu
#define FILE_MAP_ALL_ACCESS 0xf001fu
#define MAX_PATH_CHARS 4096
#define MAX_OPTS 170
#define VALUE_MAX 160
#define MAXP 4u
#define WMAX 1920u
#define HMAX 1088u
#define FRAME_MAX (WMAX*HMAX)
#define AUDIO_MAX 16384u
#define RETRO_EXPERIMENTAL 0x10000u
#define RETRO_HW_VULKAN 6u
#define GET_HW_IFACE (41u|RETRO_EXPERIMENTAL)
#define SET_NEGOTIATE (43u|RETRO_EXPERIMENTAL)
#define GET_NEGOTIATE_SUPPORT (73u|RETRO_EXPERIMENTAL)
#define MAGIC 0x56505332u
#define CMD_RUN 1u
#define CMD_STOP 2u
#define STATE_INIT 0u
#define STATE_READY 1u
#define STATE_FAIL 2u
#define STATE_STOP 3u
struct Game {const char *path; const void *data; size_t size; const char *meta;};
struct Sys {const char *name,*version,*extensions;bool fullpath,block_extract;};
struct Geo {unsigned width,height,maxw,maxh;float aspect;};
struct Timing {double fps,rate;};
struct AV {struct Geo geo;struct Timing timing;};
struct Var {const char *key,*value;};
struct Controller {const char *desc;unsigned id;};
struct Controllers {const struct Controller *types;unsigned count;};
struct Desc {unsigned port,device,index,id;const char *description;};
struct Log {void (*log)(int,const char*,...);};
struct HW {unsigned type;void (*reset)(void);uintptr_t (*getfb)(void);void *(*getproc)(const char*);bool depth,stencil,bottom_left;unsigned major,minor;bool cache;void (*destroy)(void);bool debug;};
_Static_assert(sizeof(struct HW)==64 && offsetof(struct HW,destroy)==48,"hw ABI");
struct retro_core_option_value {const char *value,*label;};
struct retro_core_option_v2_category {const char *key,*desc,*info;};
struct retro_core_option_v2_definition {const char *key,*desc,*desc_categorized,*info,*info_categorized,*category_key;struct retro_core_option_value values[128];const char *default_value;};
struct retro_core_options_v2 {struct retro_core_option_v2_category *categories;struct retro_core_option_v2_definition *definitions;};
#include "native_options_generated.h"
_Static_assert(sizeof(struct retro_core_option_v2_definition)==2104,"options ABI");
typedef bool (*Env)(unsigned,void*);typedef void (*Video)(const void*,unsigned,unsigned,size_t);typedef void (*Audio)(int16_t,int16_t);typedef size_t (*Batch)(const int16_t*,size_t);typedef void (*Poll)(void);typedef int16_t (*Input)(unsigned,unsigned,unsigned,unsigned);
struct Shared {uint32_t magic,player,parent_pid;volatile uint32_t command,state,epoch;uint32_t language,fastforward,width,height,audio_count,rendered;char stage[256];char core[MAX_PATH_CHARS],game[MAX_PATH_CHARS],system[MAX_PATH_CHARS],save[MAX_PATH_CHARS],logpath[MAX_PATH_CHARS],username[128];char opts[MAX_OPTS][VALUE_MAX];int16_t buttons[16],analog[2][2];double fps,rate;int16_t audio[AUDIO_MAX*2];uint32_t pixels[FRAME_MAX];};
API H WIN GetModuleHandleA(const char*);API BOOL WIN GetModuleHandleExA(DWORD,const char*,H*);API DWORD WIN GetModuleFileNameA(H,char*,DWORD);API H WIN LoadLibraryA(const char*);API BOOL WIN FreeLibrary(H);API void *WIN GetProcAddress(H,const char*);
API H WIN CreateFileA(const char*,DWORD,DWORD,void*,DWORD,DWORD,H);API BOOL WIN ReadFile(H,void*,DWORD,DWORD*,void*);API BOOL WIN WriteFile(H,const void*,DWORD,DWORD*,void*);API DWORD WIN GetFileSize(H,DWORD*);API BOOL WIN SetFilePointerEx(H,int64_t,int64_t*,DWORD);API BOOL WIN CloseHandle(H);API BOOL WIN DeleteFileA(const char*);API BOOL WIN CreateDirectoryA(const char*,void*);API BOOL WIN CreateDirectoryW(const uint16_t*,void*);API BOOL WIN RemoveDirectoryA(const char*);API DWORD WIN GetTempPathA(DWORD,char*);API DWORD WIN GetEnvironmentVariableA(const char*,char*,DWORD);API int WIN MultiByteToWideChar(unsigned,DWORD,const char*,int,uint16_t*,int);
API H WIN CreateFileMappingA(H,void*,DWORD,DWORD,DWORD,const char*);API H WIN OpenFileMappingA(DWORD,BOOL,const char*);API void *WIN MapViewOfFile(H,DWORD,DWORD,DWORD,size_t);API BOOL WIN UnmapViewOfFile(const void*);API H WIN CreateEventA(void*,BOOL,BOOL,const char*);API H WIN OpenEventA(DWORD,BOOL,const char*);API BOOL WIN SetEvent(H);API DWORD WIN WaitForSingleObject(H,DWORD);API DWORD WIN WaitForMultipleObjects(DWORD,const H*,BOOL,DWORD);API H WIN OpenProcess(DWORD,BOOL,DWORD);API DWORD WIN GetCurrentProcessId(void);API uint64_t WIN GetTickCount64(void);API DWORD WIN GetLastError(void);API void WIN ExitProcess(unsigned);API BOOL WIN TerminateProcess(H,unsigned);API BOOL WIN GetExitCodeProcess(H,DWORD*);API DWORD WIN ResumeThread(H);API char *WIN GetCommandLineA(void);API void WIN AcquireSRWLockExclusive(void**);API void WIN ReleaseSRWLockExclusive(void**);
API H WIN CreateJobObjectA(void*,const char*);API BOOL WIN SetInformationJobObject(H,int,void*,DWORD);API BOOL WIN AssignProcessToJobObject(H,H);
struct Startup {DWORD cb;char *reserved,*desktop,*title;DWORD x,y,xsize,ysize,xchars,ychars,fill,flags;WORD show,reserved2len;unsigned char *reserved2;H stdin_h,stdout_h,stderr_h;};
struct Process {H process,thread;DWORD pid,tid;};
_Static_assert(sizeof(struct Startup)==104 && sizeof(struct Process)==24,"win ABI");
API BOOL WIN CreateProcessA(const char*,char*,void*,void*,BOOL,DWORD,void*,const char*,struct Startup*,struct Process*);
API void *WIN VirtualAlloc(void*,size_t,DWORD,DWORD);API BOOL WIN VirtualFree(void*,size_t,DWORD);
static size_t slen(const char*s){size_t n=0;if(s)while(s[n])++n;return n;}
static bool eq(const char*a,const char*b){if(!a||!b)return false;while(*a&&*b)if(*a++!=*b++)return false;return *a==*b;}
static void copystr(char*d,size_t n,const char*s){size_t i=0;if(!n)return;if(s)for(;i+1<n&&s[i];++i)d[i]=s[i];d[i]=0;}
static void append(char*d,size_t n,const char*s){size_t i=slen(d);if(i<n)copystr(d+i,n-i,s);}
static void num(char*d,size_t n,uint64_t v){char t[24];unsigned i=0;do{t[i++]=(char)('0'+v%10);v/=10;}while(v&&i<sizeof(t));while(i){char k[2]={t[--i],0};append(d,n,k);}}
typedef uint64_t __attribute__((aligned(1),may_alias)) unaligned_u64;
void *memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;while(n>=8){*(unaligned_u64*)a=*(const unaligned_u64*)b;a+=8;b+=8;n-=8;}while(n--)*a++=*b++;return d;}
void *memset(void*d,int c,size_t n){unsigned char*a=d;uint64_t v=(uint64_t)(unsigned char)c*0x0101010101010101ull;while(n>=8){*(unaligned_u64*)a=v;a+=8;n-=8;}while(n--)*a++=(unsigned char)c;return d;}
static void zero(void*d,size_t n){memset(d,0,n);}
static void object_name(char *out,size_t n,const char*base,const char*suffix){copystr(out,n,base);append(out,n,suffix);}
static unsigned option_count(void){unsigned n=0;while(n<MAX_OPTS&&s_native_definitions[n].key)++n;return n;}
static int option_index(const char*k){unsigned n=option_count();for(unsigned i=0;i<n;++i)if(eq(k,s_native_definitions[i].key))return (int)i;return -1;}
static unsigned mac_pos(const char*k){const char*p="ppsspp_change_mac_address";unsigned n=0;while(p[n]){if(k[n]!=p[n])return 0;++n;}if(k[n]<'0'||k[n]>'9'||k[n+1]<'0'||k[n+1]>'9'||k[n+2])return 0;unsigned pos=(k[n]-'0')*10u+k[n+1]-'0';return pos<=12?pos:0;}
static const char* fixed_option(const char*k){if(eq(k,"ppsspp_backend"))return "vulkan";if(eq(k,"ppsspp_software_rendering")||eq(k,"ppsspp_enable_upnp")||eq(k,"ppsspp_detect_vsync_swap_interval"))return "disabled";if(eq(k,"ppsspp_enable_wlan")||eq(k,"ppsspp_enable_builtin_pro_ad_hoc_server"))return "enabled";if(eq(k,"ppsspp_change_pro_ad_hoc_server_address"))return "localhost";if(eq(k,"ppsspp_port_offset"))return "10000";if(eq(k,"ppsspp_vr_mode"))return "Off";unsigned pos=mac_pos(k);if(pos)return pos==2?"2":pos==12?"1":"0";return 0;}
static const char* player_option(unsigned player,const char*k){if(eq(k,"ppsspp_enable_builtin_pro_ad_hoc_server"))return player?"disabled":"enabled";unsigned pos=mac_pos(k);if(pos==12){static const char *digits[]={"1","2","3","4"};return player<MAXP?digits[player]:"1";}return fixed_option(k);}

int _fltused=0;
