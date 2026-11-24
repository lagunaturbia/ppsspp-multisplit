#include <stdio.h>
#include <stdlib.h>
#define __declspec(x)
#define __stdcall
#include "bridge.c"
static int16_t joy[MAXP][16],stick[MAXP][2][2],keyboard[512];
static struct Shared state;
static int16_t fake_input(unsigned p,unsigned d,unsigned index,unsigned id){
 if(d==3)return p==0&&id<512?keyboard[id]:0;
 if(p>=MAXP)return 0;
 if(d==1&&id<16)return joy[p][id];
 if(d==5&&index<2&&id<2)return stick[p][index][id];return 0;
}
static void check(int b,const char*m){if(!b){fprintf(stderr,"FAIL: %s\n",m);exit(1);}}
static void init(void){zero(joy,sizeof(joy));zero(stick,sizeof(stick));zero(keyboard,sizeof(keyboard));zero(&state,sizeof(state));for(unsigned i=0;i<MAXP;i++)ports[i]=1;selection=1;input=fake_input;reset_menu_input();}
int main(void){
 for(unsigned p=0;p<MAXP;p++){
  init();for(unsigned i=0;i<MAXP;i++)ports[i]=i==p?1:0;
  check(!menu_input_step(),"initial neutral");joy[p][5]=1;check(!menu_input_step()&&selection==2,"single pad navigates");check(!menu_input_step()&&selection==2,"no held repeat");joy[p][5]=0;joy[p][0]=1;check(menu_input_step(),"single pad confirms");check(input_ports[0]==p,"single pad routes to P1");joy[p][0]=0;joy[p][9]=1;stick[p][0][0]=12345;read_player_input(0,&state);check(state.buttons[9]==1&&state.analog[0][0]==12345,"P1 gameplay");for(unsigned i=1;i<MAXP;i++){read_player_input(i,&state);check(!state.buttons[9]&&!state.analog[0][0],"idle other players");}printf("PASS one controller on port %u\n",p+1);
 }
 init();joy[0][3]=1;check(!menu_input_step(),"held launch button ignored");joy[1][5]=1;check(!menu_input_step()&&selection==2,"held P1 does not block P2");joy[1][0]=1;check(menu_input_step()&&input_ports[0]==1,"P2 confirms despite held P1");puts("PASS independent edges");
 init();check(!menu_input_step(),"keyboard neutral");keyboard[273]=1;check(!menu_input_step()&&selection==0,"keyboard up");keyboard[13]=1;check(menu_input_step()&&input_ports[0]==0,"keyboard enter");puts("PASS keyboard fallback");
 init();check(!menu_input_step(),"multiple pads neutral");joy[0][5]=1;check(!menu_input_step(),"P1 navigation");joy[2][0]=1;check(menu_input_step(),"other pad confirms");for(unsigned i=0;i<MAXP;i++)check(input_ports[i]==i,"standard order retained");puts("PASS standard four-port routing");
 init();ports[0]=0;check(!menu_input_step(),"stale none neutral");joy[0][0]=1;check(menu_input_step(),"stale none confirms");joy[0][9]=1;read_player_input(0,&state);check(state.buttons[9]==1,"stale none still sends actual input to P1");puts("PASS stale NONE on P1");
 init();for(unsigned i=0;i<MAXP;i++)ports[i]=0;check(!menu_input_step(),"no pads");check(!menu_input_step(),"no pads idle");ports[2]=1;joy[2][0]=1;check(menu_input_step()&&input_ports[0]==2,"hotplug while in selector");reset_menu_input();check(!menu_input_step(),"reset ignores held confirm");joy[2][0]=0;check(!menu_input_step(),"release");joy[2][0]=1;check(menu_input_step(),"reset allows new confirm");puts("PASS hotplug and reset");
 return 0;
}
