#include "common.h"
#include "vulkan_min.h"
#include <stdarg.h>
API int __cdecl _vsnprintf(char*,size_t,const char*,va_list);
static struct Shared *sh;
static H mapping,go,done,parent,logfile,core,vkloader,winsock;
static int (WIN *wsa_cleanup)(void);static bool wsa_started;
static bool init_network(void){winsock=LoadLibraryA("ws2_32.dll");if(!winsock)return false;int (WIN *startup)(WORD,void*)=(void*)GetProcAddress(winsock,"WSAStartup");wsa_cleanup=(void*)GetProcAddress(winsock,"WSACleanup");uint64_t wsadata[64]={0};if(!startup||!wsa_cleanup||startup(0x0202,wsadata)!=0)return false;wsa_started=true;return true;}
static struct HW hw;
static struct Neg neg;
static bool have_hw,have_neg,ready,failed;
static void *qlock,*ilock,*llock;
static unsigned last_epoch;
static void stage(const char*s){copystr(sh->stage,sizeof(sh->stage),s);}
static void fail(const char*s){if(!failed){stage(s);failed=true;sh->state=STATE_FAIL;}}
static void log_msg(int level,const char*fmt,...){(void)level;if(!logfile||logfile==BAD_HANDLE)return;char b[2048];va_list args;va_start(args,fmt);int n=_vsnprintf(b,sizeof(b)-2,fmt,args);va_end(args);if(n<0||n>(int)sizeof(b)-2)n=sizeof(b)-2;b[n++]='\n';DWORD written;AcquireSRWLockExclusive(&llock);WriteFile(logfile,b,(DWORD)n,&written,0);ReleaseSRWLockExclusive(&llock);}
static struct Context vkctx;
static struct Interface iface;
static VK instance,buf,memory,pool,command_buffer;
static void *mapped;
static Q buffer_capacity;
static GPA gipa,gdpa;
static struct RetroImage image;
static bool image_ready;
static U image_sems_count,image_family,core_cmd_count;
static VK image_sems[16],core_cmds[16],signal_sem;
static struct Ext exts[256];
static U zero_features[64];
#define F(name,ret,...) static ret (*name)(__VA_ARGS__)
F(vkDestroyInstance,void,VK,const void*);
F(vkDestroyDevice,void,VK,const void*);
F(vkGetPhysicalDeviceMemoryProperties,void,VK,struct MemProps*);
F(vkCreateBuffer,I,VK,const struct BufferCI*,const void*,VK*);
F(vkDestroyBuffer,void,VK,VK,const void*);
F(vkGetBufferMemoryRequirements,void,VK,VK,struct Requirements*);
F(vkAllocateMemory,I,VK,const struct Alloc*,const void*,VK*);
F(vkFreeMemory,void,VK,VK,const void*);
F(vkBindBufferMemory,I,VK,VK,VK,Q);
F(vkMapMemory,I,VK,VK,Q,Q,U,void**);
F(vkUnmapMemory,void,VK,VK);
F(vkCreateCommandPool,I,VK,const struct PoolCI*,const void*,VK*);
F(vkDestroyCommandPool,void,VK,VK,const void*);
F(vkAllocateCommandBuffers,I,VK,const struct CmdAlloc*,VK*);
F(vkResetCommandPool,I,VK,VK,U);
F(vkBeginCommandBuffer,I,VK,const struct CmdBegin*);
F(vkEndCommandBuffer,I,VK);
F(vkCmdPipelineBarrier,void,VK,U,U,U,U,const void*,U,const struct BufferBarrier*,U,const struct ImageBarrier*);
F(vkCmdCopyImageToBuffer,void,VK,VK,U,VK,U,const struct Copy*);
F(vkQueueSubmit,I,VK,U,const struct Submit*,VK);
F(vkQueueWaitIdle,I,VK);
F(vkDeviceWaitIdle,I,VK);
#undef F
static void lockq(void*h){(void)h;AcquireSRWLockExclusive(&qlock);}
static void unlockq(void*h){(void)h;ReleaseSRWLockExclusive(&qlock);}
static void set_image(void*h,const struct RetroImage*img,U n,const VK*sems,U fam){(void)h;AcquireSRWLockExclusive(&ilock);if(!img||n>16){image_ready=false;}else{image=*img;image_sems_count=n;for(U i=0;i<n;++i)image_sems[i]=sems[i];image_family=fam;image_ready=true;}ReleaseSRWLockExclusive(&ilock);}
static U sync_index(void*h){(void)h;return 0;}
static U sync_mask(void*h){(void)h;return 1;}
static void sync_wait(void*h){(void)h;/* The sole image has been consumed synchronously before the next retro_run. */}
static void set_cmds(void*h,U n,const VK*c){(void)h;AcquireSRWLockExclusive(&ilock);core_cmd_count=n<=16?n:0;for(U i=0;i<core_cmd_count;++i)core_cmds[i]=c[i];ReleaseSRWLockExclusive(&ilock);}
static void set_signal(void*h,VK sem){(void)h;AcquireSRWLockExclusive(&ilock);signal_sem=sem;ReleaseSRWLockExclusive(&ilock);}
static uintptr_t getfb(void){return 0;}
static void *getproc(const char*s){if(eq(s,"vkGetInstanceProcAddr"))return (void*)gipa;if(eq(s,"vkGetDeviceProcAddr"))return (void*)gdpa;return gipa?gipa(instance,s):0;}
static bool has_ext(const char*s,U n){for(U i=0;i<n;++i)if(eq(exts[i].name,s))return true;return false;}
static bool init_vk(void){
  if(!have_hw||!have_neg||!neg.create||!hw.reset){fail("PPSSPP did not negotiate a Vulkan hardware context");return false;}
  stage("Loading Vulkan loader");vkloader=LoadLibraryA("vulkan-1.dll");if(!vkloader){fail("vulkan-1.dll not available");return false;}
  gipa=(GPA)GetProcAddress(vkloader,"vkGetInstanceProcAddr");if(!gipa){fail("vkGetInstanceProcAddr missing");return false;}
  I (*enum_ext)(const char*,U*,struct Ext*)=(void*)gipa(0,"vkEnumerateInstanceExtensionProperties");
  I (*enum_ver)(U*)=(void*)gipa(0,"vkEnumerateInstanceVersion");
  I (*create_instance)(const struct InstanceCI*,const void*,VK*)=(void*)gipa(0,"vkCreateInstance");
  if(!enum_ext||!create_instance){fail("Vulkan instance entry points unavailable");return false;}
  U api=VK_API_1_0;if(enum_ver)enum_ver(&api);if(api>VK_API_1_4)api=VK_API_1_4;
  U n=256;I res=enum_ext(0,&n,exts);if(res!=0&&res!=5){fail("Cannot enumerate Vulkan instance extensions");return false;}if(n>256)n=256;
  const char* candidates[]={"VK_KHR_surface","VK_KHR_win32_surface","VK_KHR_get_physical_device_properties2","VK_EXT_debug_utils","VK_KHR_external_memory_capabilities","VK_KHR_external_semaphore_capabilities"};
  const char* enabled[6];U ec=0;for(U i=0;i<6;++i)if(has_ext(candidates[i],n))enabled[ec++]=candidates[i];
  struct App app={0,0,"PPSSPP MultiSplit",1,"PPSSPP",1,api};const struct App* orig=neg.app?neg.app():0;if(orig){app=*orig;app.api=api;}
  struct InstanceCI ci={1,0,0,&app,0,0,ec,enabled};stage("Creating independent VkInstance");if(create_instance(&ci,0,&instance)!=0||!instance){fail("vkCreateInstance failed");return false;}
  vkDestroyInstance=(void*)gipa(instance,"vkDestroyInstance");
  gdpa=(GPA)gipa(instance,"vkGetDeviceProcAddr");vkGetPhysicalDeviceMemoryProperties=(void*)gipa(instance,"vkGetPhysicalDeviceMemoryProperties");
  if(!gdpa||!vkGetPhysicalDeviceMemoryProperties||!vkDestroyInstance){fail("Vulkan instance functions unavailable");return false;}
  stage("PPSSPP create_device negotiation");const char* empty[1]={0};if(!neg.create(&vkctx,instance,0,0,gipa,empty,0,empty,0,zero_features)||!vkctx.device||!vkctx.queue||!vkctx.gpu){fail("PPSSPP Vulkan create_device failed");return false;}
  #define LOAD(n) do{n=(void*)gdpa(vkctx.device,#n);if(!n){fail("Missing " #n);return false;}}while(0)
  LOAD(vkDestroyDevice);LOAD(vkCreateBuffer);LOAD(vkDestroyBuffer);LOAD(vkGetBufferMemoryRequirements);LOAD(vkAllocateMemory);LOAD(vkFreeMemory);LOAD(vkBindBufferMemory);LOAD(vkMapMemory);LOAD(vkUnmapMemory);LOAD(vkCreateCommandPool);LOAD(vkDestroyCommandPool);LOAD(vkAllocateCommandBuffers);LOAD(vkResetCommandPool);LOAD(vkBeginCommandBuffer);LOAD(vkEndCommandBuffer);LOAD(vkCmdPipelineBarrier);LOAD(vkCmdCopyImageToBuffer);LOAD(vkQueueSubmit);LOAD(vkQueueWaitIdle);LOAD(vkDeviceWaitIdle);
  #undef LOAD
  struct PoolCI pc={39,0,0,vkctx.queue_index};if(vkCreateCommandPool(vkctx.device,&pc,0,&pool)!=0){fail("Cannot create Vulkan transfer command pool");return false;}
  struct CmdAlloc ca={40,0,pool,0,1};if(vkAllocateCommandBuffers(vkctx.device,&ca,&command_buffer)!=0){fail("Cannot allocate transfer command buffer");return false;}
  iface=(struct Interface){0,5,sh,instance,vkctx.gpu,vkctx.device,gdpa,gipa,vkctx.queue,vkctx.queue_index,set_image,sync_index,sync_mask,set_cmds,sync_wait,lockq,unlockq,set_signal};
  stage("PPSSPP Vulkan context_reset");hw.reset();stage("Vulkan context ready");ready=true;log_msg(1,"[MultiSplit] Independent VkInstance and VkDevice initialized");return true;
}
static void release_buffer(void){if(mapped&&memory&&vkUnmapMemory)vkUnmapMemory(vkctx.device,memory);mapped=0;if(buf&&vkDestroyBuffer)vkDestroyBuffer(vkctx.device,buf,0);buf=0;if(memory&&vkFreeMemory)vkFreeMemory(vkctx.device,memory,0);memory=0;buffer_capacity=0;}
static bool ensure_buffer(Q bytes){if(mapped&&bytes<=buffer_capacity)return true;release_buffer();struct BufferCI bc={12,0,0,bytes,2,0,0,0};if(vkCreateBuffer(vkctx.device,&bc,0,&buf)!=0)return false;struct Requirements req;vkGetBufferMemoryRequirements(vkctx.device,buf,&req);struct MemProps props;zero(&props,sizeof(props));vkGetPhysicalDeviceMemoryProperties(vkctx.gpu,&props);U type=VK_IGNORED;for(U i=0;i<props.count&&i<32;++i)if((req.bits&(1u<<i))&&(props.types[i].flags&6u)==6u){type=i;if(props.types[i].flags&8u)break;}if(type==VK_IGNORED)return false;struct Alloc a={5,0,req.size,type};if(vkAllocateMemory(vkctx.device,&a,0,&memory)!=0||vkBindBufferMemory(vkctx.device,buf,memory,0)!=0||vkMapMemory(vkctx.device,memory,0,bytes,0,&mapped)!=0)return false;buffer_capacity=bytes;return true;}
static bool read_frame(unsigned w,unsigned h){
 if(!ready||!w||!h)return false;if(w>4800||h>2720){fail("Vulkan frame dimensions outside supported range");return false;}
 struct RetroImage im;U ns,nc,family;VK sems[16],cmds[17],signal;
 AcquireSRWLockExclusive(&ilock);bool exists=image_ready;im=image;ns=image_sems_count;nc=core_cmd_count;family=image_family;signal=signal_sem;for(U i=0;i<ns;++i)sems[i]=image_sems[i];for(U i=0;i<nc;++i)cmds[i]=core_cmds[i];image_ready=false;image_sems_count=0;core_cmd_count=0;signal_sem=0;ReleaseSRWLockExclusive(&ilock);
 if(!exists)return true; /* Boot frame or duplicate */
 if(!im.ci.image||!(im.layout==VK_SHADER_READ||im.layout==VK_GENERAL)||(family!=VK_IGNORED&&family!=vkctx.queue_index)){fail("Unsupported Vulkan presentation layout or queue family");return false;}
 U fmt=im.ci.format;if(fmt!=VK_BGRA8&&fmt!=VK_BGRA8_SRGB&&fmt!=VK_RGBA8&&fmt!=VK_RGBA8_SRGB){fail("Unsupported Vulkan image format");return false;}
 lockq(0);bool ok=false;Q bytes=(Q)w*h*4;
 if(!ensure_buffer(bytes)||vkResetCommandPool(vkctx.device,pool,0)!=0)goto out;
 struct CmdBegin begin={42,0,1,0};if(vkBeginCommandBuffer(command_buffer,&begin)!=0)goto out;
 struct Range range={1,im.ci.range.baseMip,1,im.ci.range.baseLayer,1};
 struct ImageBarrier ib={45,0,VK_ACCESS_MEMORY_WRITE|VK_ACCESS_MEMORY_READ,VK_ACCESS_TRANSFER_READ,im.layout,VK_TRANSFER_SRC,VK_IGNORED,VK_IGNORED,im.ci.image,range};
 vkCmdPipelineBarrier(command_buffer,VK_STAGE_ALL,VK_STAGE_TRANSFER,0,0,0,0,0,1,&ib);
 struct Copy region={0,0,0,{1,range.baseMip,range.baseLayer,1},{0,0,0},{w,h,1}};
 vkCmdCopyImageToBuffer(command_buffer,im.ci.image,VK_TRANSFER_SRC,buf,1,&region);
 ib.src=VK_ACCESS_TRANSFER_READ;ib.dst=VK_ACCESS_MEMORY_READ;ib.oldLayout=VK_TRANSFER_SRC;ib.newLayout=im.layout;
 vkCmdPipelineBarrier(command_buffer,VK_STAGE_TRANSFER,VK_STAGE_ALL,0,0,0,0,0,1,&ib);
 struct BufferBarrier bb={44,0,VK_ACCESS_TRANSFER_WRITE,VK_ACCESS_HOST_READ,VK_IGNORED,VK_IGNORED,buf,0,bytes};
 vkCmdPipelineBarrier(command_buffer,VK_STAGE_TRANSFER,VK_STAGE_HOST,0,0,0,1,&bb,0,0);
 if(vkEndCommandBuffer(command_buffer)!=0)goto out;
 U stages[16];for(U i=0;i<ns;++i)stages[i]=VK_STAGE_ALL;
 cmds[nc]=command_buffer;struct Submit sub={4,0,nc?0:ns,sems,stages,nc+1,cmds,signal?1:0,&signal};
 if(vkQueueSubmit(vkctx.queue,1,&sub,0)!=0||vkQueueWaitIdle(vkctx.queue)!=0)goto out;
 unsigned ow=w,oh=h;if(ow>WMAX||oh>HMAX){if((Q)w*HMAX>(Q)h*WMAX){ow=WMAX;oh=(unsigned)((Q)h*ow/w);}else{oh=HMAX;ow=(unsigned)((Q)w*oh/h);}}if(!ow||!oh)goto out;
 const uint32_t*src=mapped;uint32_t*dst=sh->pixels;
 if(ow==w&&oh==h&&(fmt==VK_BGRA8||fmt==VK_BGRA8_SRGB))memcpy(dst,src,(size_t)bytes);
 else for(U y=0;y<oh;++y)for(U x=0;x<ow;++x){uint32_t c=src[(size_t)(y*h/oh)*w+x*w/ow];if(fmt==VK_RGBA8||fmt==VK_RGBA8_SRGB)c=(c&0xff00ff00u)|((c&0xffu)<<16)|((c>>16)&0xffu);dst[y*ow+x]=c;}
 sh->width=ow;sh->height=oh;sh->rendered++;ok=true;
 out:unlockq(0);if(!ok)fail("Vulkan readback failed (buffer, submission or device)");return ok;
}
static bool environment(unsigned cmd,void*d){
 switch(cmd){
 case 1:case 6:case 11:case 16:case 18:case 35:case 53:case 54:case 55:case 67:case 68:case 69:case 70:return true;
 case 3:if(d){*(bool*)d=true;return true;}return false;
 case 7:fail("PPSSPP requested shutdown (see worker log)");return true;
 case 9:if(d){*(const char**)d=sh->system;return true;}return false;
 case 10:return d&&*(int*)d==1;
 case 14:if(!d||((struct HW*)d)->type!=RETRO_HW_VULKAN)return false;hw=*(struct HW*)d;have_hw=true;((struct HW*)d)->getfb=getfb;((struct HW*)d)->getproc=getproc;return true;
 case 15:if(d){struct Var*v=d;int i=option_index(v->key);const char*f=player_option(sh->player,v->key);v->value=f?f:(i>=0?sh->opts[i]:0);return v->value!=0;}return false;
 case 17:if(d){*(bool*)d=last_epoch!=sh->epoch;last_epoch=sh->epoch;return true;}return false;
 case 27:if(d){((struct Log*)d)->log=log_msg;return true;}return false;
 case 30:case 31:if(d){*(const char**)d=sh->save;return true;}return false;
 case 32:if(d){struct AV*a=d;sh->fps=a->timing.fps;sh->rate=a->timing.rate;return true;}return false;
 case 37:return true;
 case 38:if(d){*(const char**)d=sh->username;return true;}return false;
 case 39:if(d){*(unsigned*)d=sh->language;return true;}return false;
 case GET_HW_IFACE:if(d&&iface.device){*(struct Interface**)d=&iface;return true;}return false;
 case SET_NEGOTIATE:if(d){struct Neg*n=d;if(n->type!=0||n->version<1||!n->create)return false;zero(&neg,sizeof(neg));memcpy(&neg,n,offsetof(struct Neg,create_instance));have_neg=true;return true;}return false;
 case GET_NEGOTIATE_SUPPORT:if(d){U*n=d;if(n[0]!=0)return false;n[1]=1;return true;}return false;
 case (49u|RETRO_EXPERIMENTAL):if(d){*(bool*)d=sh->fastforward!=0;return true;}return false;
 case (51u|RETRO_EXPERIMENTAL):return true;
 case 52:if(d){*(unsigned*)d=2;return true;}return false;
 case 56:if(d){*(unsigned*)d=RETRO_HW_VULKAN;return true;}return false;
 case 59:if(d){*(unsigned*)d=0;return true;}return false;
 case 74:if(d){*(bool*)d=true;return true;}return false;
 default:return false;
 }
}
static void video(const void*d,unsigned w,unsigned h,size_t pitch){(void)pitch;if(d==(const void*)(intptr_t)-1){if(!failed)read_frame(w,h);}else if(d){fail("PPSSPP attempted software video instead of Vulkan");}}
static void sample(int16_t a,int16_t b){if(sh->player!=0)return;U n=sh->audio_count;if(n<AUDIO_MAX){sh->audio[2*n]=a;sh->audio[2*n+1]=b;sh->audio_count=n+1;}}
static size_t batch(const int16_t*d,size_t n){if(sh->player==0){size_t free=AUDIO_MAX-sh->audio_count,copy=n<free?n:free;if(copy){memcpy(sh->audio+2*sh->audio_count,d,copy*4);sh->audio_count+=(U)copy;}}return n;}
static void poll(void){}
static int16_t input(unsigned p,unsigned dev,unsigned idx,unsigned id){if(p)return 0;if((dev&255)==1){if(id==256){unsigned bits=0;for(U i=0;i<16;++i)if(sh->buttons[i])bits|=1u<<i;return (int16_t)bits;}if(id<16)return sh->buttons[id];}if((dev&255)==5&&idx<2&&id<2)return sh->analog[idx][id];return 0;}
static void (*unload)(void),(*deinit)(void);static bool loaded,inited;
static void cleanup(void){stage("Closing PPSSPP session");if(vkctx.device&&vkDeviceWaitIdle)vkDeviceWaitIdle(vkctx.device);release_buffer();if(pool&&vkDestroyCommandPool)vkDestroyCommandPool(vkctx.device,pool,0);pool=0;if(loaded&&unload)unload();if(inited&&deinit)deinit();if(have_neg&&neg.destroy&&vkctx.device)neg.destroy();if(vkctx.device&&vkDestroyDevice)vkDestroyDevice(vkctx.device,0);if(instance&&vkDestroyInstance)vkDestroyInstance(instance,0);if(core)FreeLibrary(core);if(vkloader)FreeLibrary(vkloader);if(wsa_started&&wsa_cleanup)wsa_cleanup();if(winsock)FreeLibrary(winsock);}
static bool boot(void (**run)(void)){
 stage("Loading original PPSSPP core");core=LoadLibraryA(sh->core);if(!core){fail("Could not load bundled PPSSPP DLL");return false;}
 #define FN(var,name) do{var=(void*)GetProcAddress(core,name);if(!var){fail("Missing " name);return false;}}while(0)
 void (*setenv)(Env),(*setvid)(Video),(*seta)(Audio),(*setb)(Batch),(*setp)(Poll),(*seti)(Input),(*init)(void),(*getav)(struct AV*),(*setport)(unsigned,unsigned);bool (*load)(const struct Game*);
 FN(setenv,"retro_set_environment");FN(setvid,"retro_set_video_refresh");FN(seta,"retro_set_audio_sample");FN(setb,"retro_set_audio_sample_batch");FN(setp,"retro_set_input_poll");FN(seti,"retro_set_input_state");FN(init,"retro_init");FN(deinit,"retro_deinit");FN(load,"retro_load_game");FN(unload,"retro_unload_game");FN((*run),"retro_run");FN(getav,"retro_get_system_av_info");FN(setport,"retro_set_controller_port_device");
 #undef FN
 setenv(environment);setvid(video);seta(sample);setb(batch);setp(poll);seti(input);stage("PPSSPP retro_init");init();inited=true;setport(0,1);
 struct Game g={sh->game,0,0,0};stage("PPSSPP retro_load_game");if(!load(&g)){fail("PPSSPP rejected content");return false;}loaded=true;
 struct AV av;zero(&av,sizeof(av));getav(&av);sh->fps=av.timing.fps;sh->rate=av.timing.rate;if(!init_vk())return false;
 last_epoch=sh->epoch;return !failed;
}
void worker_entry(void){
 const char*base=GetCommandLineA();const char*p=base;while(*p){if(*p==' ')base=p+1;++p;}char name[256];object_name(name,sizeof(name),base,"-map");mapping=OpenFileMappingA(FILE_MAP_ALL_ACCESS,0,name);if(!mapping)ExitProcess(101);sh=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*sh));if(!sh||sh->magic!=MAGIC)ExitProcess(102);
 object_name(name,sizeof(name),base,"-go");go=OpenEventA(0x100002u,0,name);object_name(name,sizeof(name),base,"-done");done=OpenEventA(0x100002u,0,name);parent=OpenProcess(0x100000u,0,sh->parent_pid);if(!go||!done||!parent)ExitProcess(103);
 logfile=CreateFileA(sh->logpath,0x40000000u,1,0,2,0,0);
 if(!init_network()){fail("WSAStartup failed");log_msg(3,"[MultiSplit] %s",sh->stage);SetEvent(done);ExitProcess(106);}log_msg(1,"[MultiSplit] Player %u: WLAN localhost, Ad Hoc server %s, port offset 10000",sh->player+1,sh->player?"off":"on");
 void (*run)(void)=0;if(!boot(&run)){log_msg(3,"[MultiSplit] %s",sh->stage);SetEvent(done);ExitProcess(104);}sh->state=STATE_READY;SetEvent(done);
 H waits[2]={go,parent};for(;;){DWORD wait=WaitForMultipleObjects(2,waits,0,INF);if(wait!=0||sh->command==CMD_STOP)break;if(sh->command!=CMD_RUN){fail("Invalid worker command");SetEvent(done);break;}sh->audio_count=0;stage("PPSSPP retro_run");run();if(failed){log_msg(3,"[MultiSplit] %s",sh->stage);SetEvent(done);ExitProcess(105);}if(sh->rendered)stage("Frame completed (Vulkan)");else stage("Booting; no frame yet");SetEvent(done);}
 cleanup();sh->state=STATE_STOP;stage("Stopped cleanly");SetEvent(done);if(logfile&&logfile!=BAD_HANDLE)CloseHandle(logfile);ExitProcess(0);
}
