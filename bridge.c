#include "common.h"
static Env env;static Video video;static Audio audio;static Batch batch;static Poll poll;static Input input;static struct Log logcb;
static H job;static char cache[MAX_PATH_CHARS],self[MAX_PATH_CHARS],worker_path[MAX_PATH_CHARS],core_path[MAX_PATH_CHARS],game_path[MAX_PATH_CHARS];static char sysdir[MAX_PATH_CHARS],savedir[MAX_PATH_CHARS],username[128];static unsigned language,ports[MAXP]={1,1,1,1};
static char opts[MAX_OPTS][VALUE_MAX];static unsigned epoch=1,generation=0;
static struct retro_core_option_v2_definition defs[MAX_OPTS+1];static struct retro_core_options_v2 options={s_native_categories,defs};
static struct {H mapping,go,done,proc;struct Shared* sh;char base[128],logpath[MAX_PATH_CHARS];} workers[MAXP];
static unsigned width=960,height=544,players=2,selection=1,pressed=0;static bool menu=true,release_gate=true;static double fps=60.0/1.001,rate=44100.;static bool active,failed;static unsigned first_frame_mask,run_count;static uint32_t *screen;static char cmdline[2*MAX_PATH_CHARS+256];static uint16_t wpath[MAX_PATH_CHARS];static unsigned char chunk[65536];
static void log_str(int level,const char*s){if(logcb.log)logcb.log(level,"[PPSSPP MultiSplit Vulkan 1.0] %s\n",s);}
static void log_worker(unsigned i,const char*s){if(logcb.log)logcb.log(3,"[PPSSPP MultiSplit Vulkan 1.0 P%u] %s | stage: %s | worker log: %s\n",i+1,s,workers[i].sh?workers[i].sh->stage:"no shared state",workers[i].logpath);}
static void mkdir_utf8(const char*p){int n=MultiByteToWideChar(65001,0,p,-1,wpath,MAX_PATH_CHARS);if(n>0)CreateDirectoryW(wpath,0);}
static void join(char*d,size_t cap,const char*base,const char*s){copystr(d,cap,base);size_t n=slen(d);if(n&&d[n-1]!='/'&&d[n-1]!='\\')append(d,cap,"\\");append(d,cap,s);}
static void register_options(void){unsigned n=option_count();zero(defs,sizeof(defs));for(unsigned i=0;i<n;++i){defs[i]=s_native_definitions[i];const char*fixed=fixed_option(defs[i].key);if(fixed){for(unsigned j=0;j<128;++j){defs[i].values[j].value=0;defs[i].values[j].label=0;}defs[i].values[0].value=fixed;defs[i].default_value=fixed;defs[i].info="Fixed for local MultiSplit Vulkan.";if(eq(defs[i].key,"ppsspp_enable_builtin_pro_ad_hoc_server"))defs[i].info="Only player 1 hosts the local server; all other players are clients.";if(mac_pos(defs[i].key))defs[i].info="Automatically assigned to each player; displayed value is for P1.";}}if(env){env(67,&options);if(logcb.log)logcb.log(1,"[PPSSPP MultiSplit Vulkan 1.0] Registered %u original option definitions (fixed Vulkan, local WLAN, VR off, fixed frame timing).\n",n);}}
static void sync_options(void){unsigned n=option_count();for(unsigned i=0;i<n;++i){struct Var v={defs[i].key,0};const char*f=fixed_option(v.key);if(!f&&env&&env(15,&v)&&v.value)f=v.value;if(!f)f=defs[i].default_value;copystr(opts[i],VALUE_MAX,f);}++epoch;}
static void geometry_set(unsigned w,unsigned h){if(!w||!h||w>2*WMAX||h>2*HMAX)return;if(w==width&&h==height)return;width=w;height=h;struct Geo geo={w,h,2*WMAX,2*HMAX,(float)w/(float)h};if(env)env(37,&geo);}
static void layout_size(unsigned tw,unsigned th,unsigned *w,unsigned *h){*w=selection==0?tw:2*tw;*h=selection==1?th:2*th;}
static void geometry_from_options(void){int j=option_index("ppsspp_internal_resolution");unsigned scale=1;if(j>=0){const char*s=opts[j];unsigned x=0;for(unsigned i=0;s[i]>='0'&&s[i]<='9';++i)x=x*10+(s[i]-'0');if(x>=480)scale=x/480;if(scale>4)scale=4;}unsigned w,h;layout_size(480*scale,272*scale,&w,&h);geometry_set(w,h);}
static bool extract_file(H f,const char*dest,uint64_t n){H out=CreateFileA(dest,0x40000000u,0,0,2,0,0);if(out==BAD_HANDLE)return false;bool ok=true;while(n){DWORD part=(DWORD)(n>sizeof(chunk)?sizeof(chunk):n),r=0,w=0;if(!ReadFile(f,chunk,part,&r,0)||r!=part||!WriteFile(out,chunk,part,&w,0)||w!=part){ok=false;break;}n-=part;}CloseHandle(out);if(!ok)DeleteFileA(dest);return ok;}
/* Windows Firewall rules identify applications by the absolute executable path.
 * Keep the executable under LocalAppData, never in a random session folder.
 * The PPSSPP engine and logs continue to have an isolated session directory. */
static bool stable_worker_path(void){
 static char local[MAX_PATH_CHARS];DWORD n=GetEnvironmentVariableA("LOCALAPPDATA",local,sizeof(local));
 if(!n||n>=sizeof(local)-80){log_str(3,"LOCALAPPDATA is missing or too long");return false;}
 join(worker_path,sizeof(worker_path),local,"PPSSPP-MultiSplit");
 if(!CreateDirectoryA(worker_path,0)){
  /* CreateDirectoryA also reports ERROR_ALREADY_EXISTS (183) for a preexisting directory. */
  if(GetLastError()!=183u){log_str(3,"Cannot create persistent PPSSPP-MultiSplit program directory");return false;}
 }
 append(worker_path,sizeof(worker_path),"\\psp_worker.exe");return true;
}
static unsigned char compare_bytes[65536];
static bool file_matches(H source,int64_t offset,uint64_t n,const char*path){
 H old=CreateFileA(path,0x80000000u,7,0,3,0,0);
 if(old==BAD_HANDLE)return false;
 bool same=GetFileSize(old,0)==n;
 if(same&&!SetFilePointerEx(source,offset,0,0))same=false;
 uint64_t remain=n;
 while(same&&remain){DWORD part=(DWORD)(remain>sizeof(chunk)?sizeof(chunk):remain),r=0,k=0;
  if(!ReadFile(source,chunk,part,&r,0)||r!=part||!ReadFile(old,compare_bytes,part,&k,0)||k!=part){same=false;break;}
  for(DWORD i=0;i<part;++i)if(chunk[i]!=compare_bytes[i]){same=false;break;}
  remain-=part;
 }
 CloseHandle(old);return same;
}
static bool extract_payloads(void){
 H module=0;if(!GetModuleHandleExA(6,(const char*)&extract_payloads,&module)||!GetModuleFileNameA(module,self,sizeof(self)))return false;
 DWORD l=GetTempPathA(sizeof(cache),cache);if(!l||l>=sizeof(cache)-160)return false;
 append(cache,sizeof(cache),"PPSSPP-MultiSplit-VK-");num(cache,sizeof(cache),GetCurrentProcessId());append(cache,sizeof(cache),"-");num(cache,sizeof(cache),GetTickCount64());append(cache,sizeof(cache),"-");num(cache,sizeof(cache),++generation);
 if(!CreateDirectoryA(cache,0)||!stable_worker_path())return false;
 join(core_path,sizeof(core_path),cache,"ppsspp_libretro.dll");
 H f=CreateFileA(self,0x80000000u,3,0,3,0,0);if(f==BAD_HANDLE)return false;
 DWORD size=GetFileSize(f,0);unsigned char footer[32];DWORD r=0;bool ok=false;
 if(size>32&&SetFilePointerEx(f,-32,0,2)&&ReadFile(f,footer,32,&r,0)&&r==32){
  const char magic[]="PSPVKDUOPAYLOAD1";bool match=true;for(unsigned i=0;i<16;++i)if(footer[i]!=(unsigned char)magic[i])match=false;
  uint64_t exe=0,core=0;memcpy(&exe,footer+16,8);memcpy(&core,footer+24,8);
  if(match&&exe&&core&&exe+core<(uint64_t)size-32){
   int64_t begin=(int64_t)size-32-(int64_t)exe-(int64_t)core;
   bool same=file_matches(f,begin,exe,worker_path);
   if(same){log_str(1,"Found identical helper executable in persistent LocalAppData directory");}
   else {if(!SetFilePointerEx(f,begin,0,0)){CloseHandle(f);return false;}if(!extract_file(f,worker_path,exe)){CloseHandle(f);log_str(3,"Cannot update persistent worker executable (possibly in use)");return false;}log_str(1,"Installed helper executable at a stable path for Windows Firewall");}
   if(SetFilePointerEx(f,begin+(int64_t)exe,0,0))ok=extract_file(f,core_path,core);
  }
 }
 CloseHandle(f);return ok;
}
/* A kill-on-close job prevents a crashed frontend from leaving emulators behind. */
struct JobBasic {int64_t process_time,job_time;DWORD flags;size_t min_ws,max_ws;DWORD active_limit;uintptr_t affinity;DWORD priority,scheduling;};
struct JobExt {struct JobBasic basic;uint64_t io[6];size_t pm,jm,peakp,peakj;};
_Static_assert(sizeof(struct JobExt)==144,"job ABI");
static void make_job(void){job=CreateJobObjectA(0,0);if(!job)return;struct JobExt info;zero(&info,sizeof(info));info.basic.flags=0x2000u;if(!SetInformationJobObject(job,9,&info,sizeof(info))){CloseHandle(job);job=0;}}
static void stop_all(void){active=false;for(unsigned i=0;i<MAXP;++i)if(workers[i].proc&&workers[i].sh&&workers[i].go&&WaitForSingleObject(workers[i].proc,0)==WAIT_TIMEOUT){workers[i].sh->command=CMD_STOP;SetEvent(workers[i].go);}for(unsigned i=0;i<MAXP;++i){if(workers[i].proc){DWORD wait=WaitForSingleObject(workers[i].proc,12000);if(wait!=0){log_worker(i,"Forced termination after shutdown timeout");TerminateProcess(workers[i].proc,110);WaitForSingleObject(workers[i].proc,2000);}CloseHandle(workers[i].proc);}if(workers[i].go)CloseHandle(workers[i].go);if(workers[i].done)CloseHandle(workers[i].done);if(workers[i].sh)UnmapViewOfFile(workers[i].sh);if(workers[i].mapping)CloseHandle(workers[i].mapping);zero(&workers[i],sizeof(workers[i]));}if(job){CloseHandle(job);job=0;}if(*core_path)DeleteFileA(core_path);/* Keep persistent worker EXE: firewall permission remains valid across launches. */core_path[0]=worker_path[0]=0;/* Logs are deliberately retained in cache directory. */cache[0]=0;}
static void fail_all(const char*s){log_str(3,s);failed=true;stop_all();}
static bool wait_worker(unsigned i,DWORD timeout){H arr[2]={workers[i].done,workers[i].proc};DWORD w=WaitForMultipleObjects(2,arr,0,timeout);if(w==0&&workers[i].sh->state==STATE_READY)return true;DWORD ec=0;if(workers[i].proc)GetExitCodeProcess(workers[i].proc,&ec);if(logcb.log)logcb.log(3,"[PPSSPP MultiSplit Vulkan 1.0 P%u] wait=%lu state=%u exit=0x%08lx\n",i+1,w,workers[i].sh?workers[i].sh->state:999,ec);log_worker(i,w==WAIT_TIMEOUT?"Worker timeout":"Worker stopped or failed");return false;}
static bool spawn(unsigned i){
 char*base=workers[i].base;copystr(base,sizeof(workers[i].base),"Local\\PSPVK-");num(base,sizeof(workers[i].base),GetCurrentProcessId());append(base,sizeof(workers[i].base),"-");num(base,sizeof(workers[i].base),GetTickCount64());append(base,sizeof(workers[i].base),"-");num(base,sizeof(workers[i].base),generation);append(base,sizeof(workers[i].base),"-");num(base,sizeof(workers[i].base),i+1);
 char name[256];object_name(name,sizeof(name),base,"-map");workers[i].mapping=CreateFileMappingA(BAD_HANDLE,0,4,0,sizeof(struct Shared),name);if(!workers[i].mapping)return false;
 struct Shared*s=MapViewOfFile(workers[i].mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*s));workers[i].sh=s;if(!s)return false;zero(s,sizeof(*s));s->magic=MAGIC;s->player=i;s->parent_pid=GetCurrentProcessId();s->epoch=epoch;s->language=language;s->fps=fps;s->rate=rate;copystr(s->username,sizeof(s->username),username);copystr(s->core,sizeof(s->core),core_path);copystr(s->game,sizeof(s->game),game_path);copystr(s->system,sizeof(s->system),sysdir);copystr(s->save,sizeof(s->save),savedir);append(s->save,sizeof(s->save),"\\P");num(s->save,sizeof(s->save),i+1);mkdir_utf8(s->save);memcpy(s->opts,opts,sizeof(opts));char player_log[16]="P";num(player_log,sizeof(player_log),i+1);append(player_log,sizeof(player_log),".log");join(workers[i].logpath,sizeof(workers[i].logpath),cache,player_log);copystr(s->logpath,sizeof(s->logpath),workers[i].logpath);
 object_name(name,sizeof(name),base,"-go");workers[i].go=CreateEventA(0,0,0,name);object_name(name,sizeof(name),base,"-done");workers[i].done=CreateEventA(0,0,0,name);if(!workers[i].go||!workers[i].done)return false;
 copystr(cmdline,sizeof(cmdline),"\"");append(cmdline,sizeof(cmdline),worker_path);append(cmdline,sizeof(cmdline),"\" ");append(cmdline,sizeof(cmdline),base);struct Startup startup;zero(&startup,sizeof(startup));startup.cb=sizeof(startup);struct Process proc;zero(&proc,sizeof(proc));if(!CreateProcessA(worker_path,cmdline,0,0,0,0x08000004u,0,cache,&startup,&proc)){if(logcb.log)logcb.log(3,"[MultiSplit Vulkan] CreateProcess failed: %lu\n",GetLastError());return false;}workers[i].proc=proc.process;if(job&&!AssignProcessToJobObject(job,proc.process)){if(logcb.log)logcb.log(2,"[MultiSplit Vulkan] Job assignment failed (worker still monitors parent): %lu\n",GetLastError());}if(ResumeThread(proc.thread)==0xffffffffu){CloseHandle(proc.thread);return false;}CloseHandle(proc.thread);if(logcb.log)logcb.log(1,"[PPSSPP MultiSplit Vulkan 1.0 P%u] Spawned hidden process %lu. Log: %s\n",i+1,proc.pid,workers[i].logpath);return true;
}
static bool start_all(void){stop_all();players=selection<2?2:selection+1;sync_options();geometry_from_options();zero(screen,4*(size_t)FRAME_MAX*4);if(!extract_payloads()){fail_all("Could not extract embedded worker and PPSSPP DLL");return false;}make_job();/* Launch in order so PPSSPP assigns native instance IDs in order. */for(unsigned i=0;i<players;++i){if(!spawn(i)){log_worker(i,"Could not create worker");fail_all("Worker launch failed");return false;}if(!wait_worker(i,90000)){fail_all("Vulkan worker initialization failed");return false;}}struct Shared*s=workers[0].sh;if(s->fps>1&&s->fps<240)fps=s->fps;if(s->rate>8000&&s->rate<192000)rate=s->rate;active=true;menu=false;failed=false;first_frame_mask=0;run_count=0;if(logcb.log)logcb.log(1,"[PPSSPP MultiSplit Vulkan 1.0] %u independent sessions initialized. Local Ad Hoc enabled.\n",players);return true;}
static void gather_config(void){const char*s=0;if(env)env(9,&s);copystr(sysdir,sizeof(sysdir),s?s:".");s=0;if(env)env(31,&s);if(!s||!*s)s=sysdir;join(savedir,sizeof(savedir),s,"PPSSPP-MultiSplit-Vulkan");mkdir_utf8(savedir);s=0;if(env)env(38,&s);copystr(username,sizeof(username),s?s:"");language=0;if(env)env(39,&language);}
static const struct Controller pad[]={{"PSP Controller",1}};static const struct Controllers controllers[]={{pad,1},{pad,1},{pad,1},{pad,1},{0,0}};
static const struct Desc descriptors[]={
#define DES(P) {P,1,0,0,"Cross"},{P,1,0,1,"Square"},{P,1,0,2,"Select"},{P,1,0,3,"Start"},{P,1,0,4,"D-Pad Up"},{P,1,0,5,"D-Pad Down"},{P,1,0,6,"D-Pad Left"},{P,1,0,7,"D-Pad Right"},{P,1,0,8,"Circle"},{P,1,0,9,"Triangle"},{P,1,0,10,"L"},{P,1,0,11,"R"},{P,5,0,0,"Analog X"},{P,5,0,1,"Analog Y"},{P,5,1,0,"Right Analog X"},{P,5,1,1,"Right Analog Y"}
 DES(0),DES(1),DES(2),DES(3),{0,0,0,0,0}
#undef DES
};
EXP unsigned retro_api_version(void){return 1;}
EXP void retro_set_environment(Env cb){env=cb;if(cb){cb(27,&logcb);bool no=false;cb(18,&no);cb(35,(void*)controllers);register_options();}}
EXP void retro_set_video_refresh(Video cb){video=cb;}EXP void retro_set_audio_sample(Audio cb){audio=cb;}EXP void retro_set_audio_sample_batch(Batch cb){batch=cb;}EXP void retro_set_input_poll(Poll cb){poll=cb;}EXP void retro_set_input_state(Input cb){input=cb;}
EXP void retro_set_controller_port_device(unsigned port,unsigned dev){if(port<MAXP)ports[port]=dev;}
EXP void retro_init(void){if(env){env(27,&logcb);env(11,(void*)descriptors);}}
EXP void retro_deinit(void){stop_all();if(screen){VirtualFree(screen,0,0x8000);screen=0;}}
EXP void retro_get_system_info(struct Sys*i){if(i)*i=(struct Sys){"PPSSPP MultiSplit Vulkan","1.0 (2-4 independent Vulkan PSPs)","elf|iso|cso|prx|pbp|chd",true,false};}
EXP void retro_get_system_av_info(struct AV*i){if(i)*i=(struct AV){{width,height,2*WMAX,2*HMAX,(float)width/(float)height},{fps,rate}};}
static uint32_t menu_pixels[960u*544u];
static const uint8_t font[39][7]={
 {14,17,17,31,17,17,17},/*A*/ {30,17,17,30,17,17,30},/*B*/
 {14,17,16,16,16,17,14},/*C*/ {30,17,17,17,17,17,30},/*D*/
 {31,16,16,30,16,16,31},/*E*/ {31,16,16,30,16,16,16},/*F*/
 {14,17,16,23,17,17,14},/*G*/ {17,17,17,31,17,17,17},/*H*/
 {31,4,4,4,4,4,31},/*I*/ {7,2,2,2,18,18,12},/*J*/
 {17,18,20,24,20,18,17},/*K*/ {16,16,16,16,16,16,31},/*L*/
 {17,27,21,21,17,17,17},/*M*/ {17,25,21,19,17,17,17},/*N*/
 {14,17,17,17,17,17,14},/*O*/ {30,17,17,30,16,16,16},/*P*/
 {14,17,17,17,21,18,13},/*Q*/ {30,17,17,30,20,18,17},/*R*/
 {15,16,16,14,1,1,30},/*S*/ {31,4,4,4,4,4,4},/*T*/
 {17,17,17,17,17,17,14},/*U*/ {17,17,17,17,17,10,4},/*V*/
 {17,17,17,21,21,21,10},/*W*/ {17,17,10,4,10,17,17},/*X*/
 {17,17,10,4,4,4,4},/*Y*/ {31,1,2,4,8,16,31},/*Z*/
 {14,17,19,21,25,17,14},/*0*/ {4,12,4,4,4,4,14},/*1*/
 {14,17,1,2,4,8,31},/*2*/ {30,1,1,14,1,1,30},/*3*/
 {2,6,10,18,31,2,2},/*4*/ {31,16,16,30,1,1,30},/*5*/
 {14,16,16,30,17,17,14},/*6*/ {31,1,2,4,8,8,8},/*7*/
 {14,17,17,14,17,17,14},/*8*/ {14,17,17,15,1,1,14},/*9*/
 {0,0,0,31,0,0,0},/*-*/ {0,4,4,0,4,4,0},/*:*/ {0,0,0,0,0,0,0},/*space*/
};
static const uint32_t bg=0xFF141823, panel=0xFF242D3E, cyan=0xFF4CD7EE,white=0xFFE8F0FA,grey=0xFFA9B5C8;
static void rect(int x,int y,int w,int h,uint32_t c){
  if(x<0||y<0||w<=0||h<=0)return;
  if(x+w>960)w=960-x;if(y+h>544)h=544-y;
  for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)menu_pixels[(size_t)yy*960+xx]=c;
}
static void glyph(int x,int y,char ch,int scale,uint32_t c){
  unsigned z=38;
  if(ch>='A'&&ch<='Z')z=(unsigned)(ch-'A');
  else if(ch>='0'&&ch<='9')z=(unsigned)(26+ch-'0');
  else if(ch=='-')z=36;else if(ch==':')z=37;
  for(int row=0;row<7;row++)for(int col=0;col<5;col++)if((font[z][row]>>(4-col))&1u)rect(x+col*scale,y+row*scale,scale,scale,c);
}
static void label_at(int x,int y,const char*s,int scale,uint32_t c){
  if(!s)return;for(size_t i=0;s[i]&&i<90;i++){char ch=s[i];if(ch>='a'&&ch<='z')ch-=32;glyph(x+(int)i*6*scale,y,ch,scale,c);}
}
static void center_label(int y,const char*s,int scale,uint32_t c){int w=(int)slen(s)*6*scale;label_at((960-w)/2,y,s,scale,c);}
static void render_menu(void){
  for(size_t k=0;k<960u*544u;k++)menu_pixels[k]=bg;
  rect(0,0,960,10,cyan);
  center_label(37,"PPSSPP MULTISPLIT VULKAN",4,white);
  center_label(89,"SELECCIONA UN MODO",2,grey);
  const char*opts[]={"2 JUGADORES - VERTICAL","2 JUGADORES - HORIZONTAL","3 JUGADORES","4 JUGADORES"};
  for(int j=0;j<4;j++){
    int y=155+j*75;
    rect(137,y,686,62,(unsigned)j==selection?0xFF336276:panel);
    if((unsigned)j==selection)rect(137,y,9,62,cyan);
    label_at(175,y+16,opts[j],3,(unsigned)j==selection?white:grey);
  }
  center_label(484,"ARRIBA ABAJO PARA ELEGIR",2,grey);
  center_label(513,"X O START PARA INICIAR",2,cyan);
  if(failed){rect(80,449,800,36,0xFF602D3B);center_label(458,"ERROR - PULSA X PARA REINTENTAR",2,white);}
}

static void enter_menu(void){stop_all();menu=true;failed=false;pressed=0;release_gate=true;selection=1;players=2;first_frame_mask=run_count=0;fps=60.0/1.001;rate=44100.;if(screen)zero(screen,4*(size_t)FRAME_MAX*4);geometry_set(960,544);}
EXP bool retro_load_game(const struct Game*g){if(!g||!g->path||!*g->path||slen(g->path)>=sizeof(game_path))return false;int pixel=1;if(!env||!env(10,&pixel))return false;copystr(game_path,sizeof(game_path),g->path);gather_config();if(!screen)screen=VirtualAlloc(0,4*(size_t)FRAME_MAX*4,0x3000,4);if(!screen)return false;enter_menu();sync_options();return true;}
EXP void retro_unload_game(void){stop_all();menu=true;failed=false;game_path[0]=0;}
static void compose(void){unsigned tw=0,th=0;for(unsigned i=0;i<players;++i){struct Shared*s=workers[i].sh;if(s->width>tw&&s->width<=WMAX)tw=s->width;if(s->height>th&&s->height<=HMAX)th=s->height;}if(!tw||!th){if(video){zero(screen,(size_t)width*height*4);video(screen,width,height,width*4);}return;}unsigned w,h;layout_size(tw,th,&w,&h);geometry_set(w,h);/* Always clear the three-player canvas; also clear mismatched/booting tiles. */bool clear=players==3;for(unsigned i=0;i<players;++i){struct Shared*s=workers[i].sh;if(s->width!=tw||s->height!=th)clear=true;}if(clear)zero(screen,(size_t)width*height*4);for(unsigned i=0;i<players;++i){struct Shared*s=workers[i].sh;if(!s->width||!s->height||s->width>WMAX||s->height>HMAX)continue;unsigned x=0,y=0;if(selection==0)y=i*th;else if(selection==1)x=i*tw;else if(selection==2&&i==2){x=tw/2;y=th;}else{x=(i%2)*tw;y=(i/2)*th;}unsigned xoff=x+(tw-s->width)/2,yoff=y+(th-s->height)/2;for(unsigned j=0;j<s->height;++j)memcpy(screen+(size_t)(j+yoff)*width+xoff,s->pixels+(size_t)j*s->width,s->width*4);}if(video)video(screen,width,height,width*4);struct Shared*s=workers[0].sh;unsigned n=s->audio_count;if(n>AUDIO_MAX)n=AUDIO_MAX;if(n){if(batch)batch(s->audio,n);else if(audio)for(unsigned i=0;i<n;++i)audio(s->audio[2*i],s->audio[2*i+1]);}}
EXP void retro_run(void){bool updated=false;if(env&&env(17,&updated)&&updated)sync_options();if(poll)poll();if(menu||!active){menu=true;geometry_set(960,544);unsigned k=0;if(input){k=(input(0,1,0,4)?1u:0u)|(input(0,1,0,5)?2u:0u)|((input(0,1,0,0)||input(0,1,0,8)||input(0,1,0,3))?4u:0u);}if(release_gate){if(!k)release_gate=false;pressed=k;}else{if((k&1u)&&!(pressed&1u))selection=(selection+3)%4;if((k&2u)&&!(pressed&2u))selection=(selection+1)%4;bool confirm=(k&4u)&&!(pressed&4u);pressed=k;if(confirm){if(video){zero(screen,960u*544u*4);video(screen,960,544,960*4);}if(start_all())return;geometry_set(960,544);}}render_menu();if(video)video(menu_pixels,960,544,960*4);return;}
 bool fast=false;if(env)env(49u|RETRO_EXPERIMENTAL,&fast);for(unsigned i=0;i<players;++i){struct Shared*s=workers[i].sh;if(s->epoch!=epoch){memcpy(s->opts,opts,sizeof(opts));s->epoch=epoch;}s->fastforward=fast;for(unsigned j=0;j<16;++j)s->buttons[j]=input&&ports[i]?input(i,1,0,j):0;for(unsigned j=0;j<2;++j)for(unsigned k=0;k<2;++k)s->analog[j][k]=input&&ports[i]?input(i,5,j,k):0;s->command=CMD_RUN;SetEvent(workers[i].go);}for(unsigned i=0;i<players;++i)if(!wait_worker(i,30000)){fail_all("One PSP did not complete a frame; returning to selector.");menu=true;release_gate=true;return;}for(unsigned i=0;i<players;++i)if(workers[i].sh->rendered&&!(first_frame_mask&(1u<<i))){first_frame_mask|=1u<<i;if(logcb.log)logcb.log(1,"[PPSSPP MultiSplit Vulkan 1.0 P%u] First Vulkan frame copied: %ux%u\n",i+1,workers[i].sh->width,workers[i].sh->height);}if(++run_count==600&&first_frame_mask!=((1u<<players)-1u))log_str(2,"Not all PSPs have delivered a Vulkan frame yet. Check worker logs.");compose();}
EXP void retro_reset(void){if(!*game_path)return;enter_menu();log_str(1,"Restart: all PSPs closed; returned to player selector.");}
EXP unsigned retro_get_region(void){return 0;}EXP size_t retro_serialize_size(void){return 0;}EXP bool retro_serialize(void*d,size_t n){(void)d;(void)n;return false;}EXP bool retro_unserialize(const void*d,size_t n){(void)d;(void)n;return false;}EXP void retro_cheat_reset(void){}EXP void retro_cheat_set(unsigned i,bool b,const char*s){(void)i;(void)b;(void)s;}EXP void*retro_get_memory_data(unsigned i){(void)i;return 0;}EXP size_t retro_get_memory_size(unsigned i){(void)i;return 0;}EXP bool retro_load_game_special(unsigned t,const struct Game*g,size_t n){(void)t;(void)g;(void)n;return false;}
