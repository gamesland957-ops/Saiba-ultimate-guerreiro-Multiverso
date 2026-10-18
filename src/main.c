/*
 * Saiba Ultimate: Guerreiro do Multiverso
 * Primeiro prototipo grafico PSP: Aldeia Africaca / Saje Capanji.
 * Renderizacao 2D por PSP GU; arte procedural provisoria, sprites finais por integrar.
 */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspaudio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

PSP_MODULE_INFO("Saiba Ultimate", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define W 480
#define H 272
#define STRIDE 512
#define BUF_SIZE (512 * 272 * 4)
#define WHITE 0xffffffff
#define BLACK 0xff15121b
#define RED 0xff3030b8
#define GOLD 0xff35d8ff
#define GREEN 0xff369b45
#define DIRT 0xff356d9b

typedef struct { unsigned int color; float x,y,z; } Vertex;
typedef struct { float x,y; int hp; int alive; int phase; } Enemy;
typedef enum { INTRO_COVER, INTRO_ROMS, INTRO_GC, TITLE, SETTINGS, TRAINING, PLAY, PORTAL, GAMEOVER } State;

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int __attribute__((aligned(16))) fb0[STRIDE*H];
static unsigned int __attribute__((aligned(16))) fb1[STRIDE*H];
static State state=INTRO_COVER;\nstatic int menuIndex=0, settingIndex=0, soundEnabled=1, language=0, introFrames=0;
static float sx=112.0f, sy=188.0f, walk=0.0f, attack=0.0f, glow=0.0f;
static int hp=100, energy=25, tools=5, score=0, mission=1, frame=0, invuln=0;
static Enemy enemies[5]={{285,190,30,1,0},{355,170,25,1,1},{405,205,35,1,2},{310,130,25,1,3},{210,150,30,1,4}};
static int audioChannel=-1;

static unsigned int rgba(int r,int g,int b,int a){return ((unsigned int)a<<24)|((unsigned int)b<<16)|((unsigned int)g<<8)|(unsigned int)r;}
static void rect(float x,float y,float w,float h,unsigned int c){
 Vertex *v=(Vertex*)sceGuGetMemory(2*sizeof(Vertex));
 v[0].x=x;v[0].y=y;v[0].z=0;v[0].color=c;
 v[1].x=x+w;v[1].y=y+h;v[1].z=0;v[1].color=c;
 sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
}
static void tri(float x1,float y1,float x2,float y2,float x3,float y3,unsigned int c){
 Vertex *v=(Vertex*)sceGuGetMemory(3*sizeof(Vertex));
 v[0]=(Vertex){c,x1,y1,0};v[1]=(Vertex){c,x2,y2,0};v[2]=(Vertex){c,x3,y3,0};
 sceGuDrawArray(GU_TRIANGLES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,3,0,v);
}
static void line(float x1,float y1,float x2,float y2,unsigned int c){
 Vertex *v=(Vertex*)sceGuGetMemory(2*sizeof(Vertex));
 v[0]=(Vertex){c,x1,y1,0};v[1]=(Vertex){c,x2,y2,0};
 sceGuDrawArray(GU_LINES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
}
static void circle(float x,float y,float r,unsigned int c){
 for(int i=0;i<12;i++){
  float a=(float)i*0.523599f,b=(float)(i+1)*0.523599f;
  tri(x,y,x+cosf(a)*r,y+sinf(a)*r,x+cosf(b)*r,y+sinf(b)*r,c);
 }
}
static void sound(int kind){
 if(audioChannel<0||!soundEnabled)return;
 static short samples[1024];
 int hz=kind==0?180:(kind==1?520:95);
 for(int i=0;i<1024;i++){
  float env=1.0f-(float)i/1024.0f;
  samples[i]=(short)(sinf((float)i*(float)hz*6.2831853f/44100.0f)*env*4500.0f);
 }
 sceAudioOutputPannedBlocking(audioChannel,PSP_AUDIO_VOLUME_MAX,PSP_AUDIO_VOLUME_MAX,samples);
}
static void hut(float x,float y,float scale){
 rect(x,y,76*scale,42*scale,rgba(157,87,48,255));
 tri(x-7*scale,y,x+38*scale,y-31*scale,x+83*scale,y,rgba(78,42,31,255));
 rect(x+8*scale,y+13*scale,14*scale,29*scale,rgba(70,39,29,255));
 rect(x+48*scale,y+11*scale,18*scale,12*scale,rgba(58,95,112,255));
 line(x+1*scale,y+7*scale,x+74*scale,y+7*scale,rgba(233,164,78,255));
}
static void tree(float x,float y,float s){
 rect(x-5*s,y-40*s,10*s,48*s,rgba(91,57,37,255));
 circle(x,y-48*s,22*s,rgba(39,106,53,255));
 circle(x-15*s,y-43*s,15*s,rgba(48,125,57,255));
 circle(x+15*s,y-42*s,15*s,rgba(48,125,57,255));
}

static const unsigned char font5x7[38][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14},{0,0,0,31,0,0,0},{0,4,4,0,4,4,0}
};
static void text5(int x,int y,const char *str,unsigned int color,int scale){
 for(;*str;str++,x+=6*scale){
  char c=*str;if(c>='a'&&c<='z')c-=32;
  int idx=-1;
  if(c>='A'&&c<='Z')idx=c-'A';else if(c>='0'&&c<='9')idx=26+c-'0';else if(c=='-')idx=36;else if(c==':')idx=37;
  if(idx<0)continue;
  for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(font5x7[idx][row]&(1<<(4-col)))rect(x+col*scale,y+row*scale,scale,scale,color);
 }
}

static void village(void){
 rect(0,0,W,H,rgba(128,190,220,255));
 circle(410,38,19,rgba(255,224,140,255));
 rect(0,83,W,74,rgba(116,151,94,255));
 rect(0,152,W,120,rgba(174,119,67,255));
 /* dusty village lane and stones */
 tri(125,272,225,143,293,143,rgba(193,143,84,255));
 tri(125,272,293,143,398,272,rgba(193,143,84,255));
 rect(0,249,W,23,rgba(139,91,54,255));
 hut(18,122,0.82f);hut(111,104,0.70f);hut(350,116,0.83f);
 tree(260,139,0.85f);tree(455,139,0.72f);tree(77,155,0.55f);
 /* market awnings */
 tri(165,164,201,164,183,137,rgba(176,52,45,255));
 tri(201,164,236,164,218,137,rgba(226,183,76,255));
 rect(166,164,69,4,rgba(91,49,34,255));
 rect(169,170,8,14,rgba(207,128,58,255));rect(185,170,8,14,rgba(87,140,78,255));
 rect(202,170,8,14,rgba(194,77,59,255));rect(218,170,8,14,rgba(223,180,75,255));
}
static void npc(float x,float y){
 circle(x,y-25,8,rgba(93,54,35,255));
 rect(x-8,y-17,16,22,rgba(53,91,125,255));
 tri(x-8,y-14,x+8,y-14,x,y-2,rgba(222,172,95,255));
 rect(x-7,y+5,5,13,rgba(54,43,35,255));rect(x+2,y+5,5,13,rgba(54,43,35,255));
}
static void saje(float x,float y,int moving,int attacking){
 float bob=moving?sinf(walk)*2.5f:0;
 /* sword on back, then black/red tunic and legs */
 line(x+7,y-40+bob,x+22,y-66+bob,rgba(205,213,220,255));
 line(x+4,y-41+bob,x+19,y-67+bob,rgba(90,100,113,255));
 rect(x-10,y-23+bob,7,20,rgba(38,29,28,255));rect(x+3,y-23+bob,7,20,rgba(38,29,28,255));
 rect(x-11,y-42+bob,23,24,rgba(26,25,32,255));
 tri(x-11,y-40+bob,x+12,y-40+bob,x,y-19+bob,rgba(166,38,44,255));
 /* African geometric trim */
 line(x-8,y-35+bob,x+8,y-35+bob,GOLD);line(x-7,y-29+bob,x+7,y-29+bob,GOLD);
 tri(x-7,y-33+bob,x,y-38+bob,x+7,y-33+bob,rgba(255,205,69,255));
 rect(x-15,y-39+bob,5,17,rgba(25,24,31,255));rect(x+12,y-39+bob,5,17,rgba(25,24,31,255));
 circle(x,y-50+bob,10,rgba(87,49,34,255));
 /* short, tightly curled hair */
 for(int i=-2;i<=2;i++)for(int j=0;j<2;j++)circle(x+i*3.3f,y-58+bob+j*2,2.5f,rgba(28,21,20,255));
 /* determined eyes and white facial markings */
 rect(x-5,y-52+bob,3,2,WHITE);rect(x+3,y-52+bob,3,2,WHITE);
 line(x-6,y-48+bob,x-3,y-45+bob,WHITE);line(x+6,y-48+bob,x+3,y-45+bob,WHITE);
 circle(x-6,y-39+bob,2,rgba(225,174,63,255));circle(x+6,y-39+bob,2,rgba(225,174,63,255));
 if(attacking>0){
  line(x+13,y-38+bob,x+34,y-50+bob,rgba(228,232,242,255));
  line(x+17,y-32+bob,x+39,y-44+bob,rgba(255,205,72,255));
  circle(x+38,y-46+bob,9,rgba(255,222,90,100));
 } else {
  line(x+13,y-37+bob,x+21,y-28+bob,rgba(184,45,48,255));
 }
 if(glow>0){circle(x,y-39+bob,22+fmodf(glow,7),rgba(255,205,70,90));}
}
static void demon(float x,float y,int phase){
 float wob=sinf((float)frame*0.12f+phase)*3;
 circle(x,y-32+wob,11,rgba(50,34,52,255));
 tri(x-8,y-39+wob,x-14,y-52+wob,x-3,y-43+wob,rgba(50,34,52,255));
 tri(x+5,y-42+wob,x+13,y-54+wob,x+10,y-37+wob,rgba(50,34,52,255));
 rect(x-12,y-24+wob,24,25,rgba(60,37,64,255));
 tri(x-12,y-19+wob,x+12,y-19+wob,x,y+3+wob,rgba(43,27,50,255));
 rect(x-8,y-34+wob,4,3,rgba(255,71,52,255));rect(x+4,y-34+wob,4,3,rgba(255,71,52,255));
 line(x-8,y+1+wob,x-14,y+12+wob,rgba(45,30,45,255));line(x+8,y+1+wob,x+14,y+12+wob,rgba(45,30,45,255));
 rect(x-12,y-43+wob,24,2,rgba(255,85,58,180));
}
static void bar(float x,float y,float width,float value,float max,unsigned int c){
 rect(x,y,width,6,rgba(30,24,31,230));
 if(value<0)value=0;if(value>max)value=max;
 rect(x+1,y+1,(width-2)*value/max,4,c);
}
static void hud(void){
 rect(8,8,180,39,rgba(24,23,31,210));
 saje(24,43,0,0);
 /* cover the tiny preview's lower edge with HUD panel */
 rect(39,14,143,7,rgba(60,47,49,255));bar(40,15,140,hp,100,rgba(56,202,91,255));
 rect(40,27,140,5,rgba(60,47,49,255));bar(40,27,140,energy,100,rgba(48,195,246,255));
 rect(292,8,180,31,rgba(24,23,31,210));
 for(int i=0;i<5;i++)circle(308+i*13,20,4, i<tools?rgba(255,205,73,255):rgba(95,85,75,255));
 for(int i=0;i<5;i++)rect(374+i*15,14,10,10,i<score?rgba(255,180,55,255):rgba(91,77,72,255));
}
static void render(void){
 sceGuStart(GU_DIRECT,list);
 sceGuClearColor(rgba(18,16,31,255));sceGuClear(GU_COLOR_BUFFER_BIT);
 village();
 if(state==INTRO_COVER){
  /* Title splash: the final high-resolution poster is the art reference; this is a PSP-safe interim composition. */
  rect(0,0,W,H,rgba(10,8,25,190));
  circle(240,87,66,rgba(255,105,22,70));circle(240,87,48,rgba(38,119,255,100));
  text5(54,38,"SOBA ULTIMATO",rgba(255,195,48,255),3);
  text5(48,68,"GUERREIRO MULTIVERSO",WHITE,2);
  saje(240,178,1,0);
  text5(120,226,"PRESSIONE X",WHITE,2);
 }else if(state==INTRO_ROMS){
  rect(0,0,W,H,rgba(8,9,18,245));circle(240,119,66,rgba(255,180,20,80));circle(240,119,51,rgba(255,180,20,100));
  text5(105,93,"GAME PLAY",WHITE,3);text5(151,129,"ROMS",rgba(255,195,48,255),4);
 }else if(state==INTRO_GC){
  rect(0,0,W,H,rgba(12,9,15,245));text5(111,84,"G C",rgba(255,195,48,255),5);
  text5(116,143,"COMPANY",WHITE,3);line(100,176,380,176,GOLD);
 }else if(state==TITLE||state==SETTINGS||state==TRAINING){
  rect(10,8,460,36,rgba(16,13,24,235));text5(24,17,"SOBA ULTIMATO",GOLD,2);
  rect(18,55,211,196,rgba(17,14,25,225));saje(118,171,1,0);
  text5(29,207,"SAJE CAPANJI",WHITE,1);
  rect(242,54,220,198,rgba(16,13,24,235));
  if(state==TITLE){
   text5(265,66,"MENU PRINCIPAL",GOLD,2);
   const char *items[5]={"NOVO JOGO","CONTINUAR","CAMPO DE","MISSAO","DEFINICAO"};
   for(int i=0;i<5;i++){int y=96+i*29;rect(253,y-5,196,24,i==menuIndex?rgba(153,91,21,245):rgba(40,35,48,220));if(i==menuIndex)rect(253,y-5,4,24,GOLD);text5(266,y,items[i],WHITE,1);}
   text5(257,235,"X CONFIRMAR",GOLD,1);
  }else if(state==SETTINGS){
   text5(262,66,"DEFINICAO",GOLD,2);
   text5(258,103,language?"IDIOMA: EN":"IDIOMA: PT",WHITE,1);
   text5(258,139,soundEnabled?"SOM: LIGADO":"SOM: DESLIGADO",WHITE,1);
   text5(258,175,"EFEITOS SONOROS",WHITE,1);
   text5(258,224,"O VOLTAR",GOLD,1);
   rect(250,settingIndex==0?94:settingIndex==1?130:166,204,23,rgba(151,90,20,100));
  }else{
   text5(261,68,"TREINAMENTO",GOLD,2);text5(258,112,"BATALHA NA ARENA",WHITE,1);text5(258,142,"X INICIAR",WHITE,1);text5(258,224,"O VOLTAR",GOLD,1);
  }
 }else if(state==PORTAL){
  circle(397,117,35,rgba(31,130,255,55));circle(397,117,27,rgba(39,170,255,120));circle(397,117,18,rgba(89,214,255,190));rect(393,88,8,59,rgba(190,242,255,210));
  circle(424,149,9,rgba(225,174,117,255));tri(414,144,418,126,428,141,rgba(30,27,34,255));tri(423,139,435,126,431,144,rgba(30,27,34,255));rect(416,158,16,24,rgba(38,90,218,255));
 }
 for(int i=0;i<5;i++)if(enemies[i].alive&&(state==PLAY||state==TRAINING))demon(enemies[i].x,enemies[i].y,enemies[i].phase);
 if(state==PLAY||state==TRAINING||state==PORTAL)saje(sx,sy,walk!=0,attack>0);
 if(attack>0){circle(sx+30,sy-42,22,rgba(255,205,66,90));for(int i=0;i<4;i++)line(sx+12,sy-35-i*4,sx+42,sy-47-i*2,rgba(255,231,126,220));}
 if(state==PLAY||state==TRAINING||state==PORTAL)hud();
 if(state==GAMEOVER){rect(90,80,300,100,rgba(20,20,30,230));text5(160,105,"FIM DE JOGO",WHITE,2);text5(159,145,"X REINICIAR",GOLD,1);}
 sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();sceGuSwapBuffers();
}
static void reset_game(void){
 sx=112;sy=188;hp=100;energy=25;tools=5;score=0;mission=1;walk=0;attack=0;glow=0;state=PLAY;
 Enemy fresh[5]={{285,190,30,1,0},{355,170,25,1,1},{405,205,35,1,2},{310,130,25,1,3},{210,150,30,1,4}};
 memcpy(enemies,fresh,sizeof(fresh));
}
int main(void){
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 sceGuInit();
 sceGuStart(GU_DIRECT,list);
 sceGuDrawBuffer(GU_PSM_8888,(void*)0,STRIDE);
 sceGuDispBuffer(W,H,(void*)0x88000,STRIDE);
 sceGuDepthBuffer((void*)0x110000,STRIDE);
 sceGuOffset(2048-(W/2),2048-(H/2));sceGuViewport(2048,2048,W,H);
 sceGuDepthRange(65535,0);sceGuScissor(0,0,W,H);sceGuEnable(GU_SCISSOR_TEST);
 sceGuDisable(GU_DEPTH_TEST);sceGuDisable(GU_CULL_FACE);sceGuEnable(GU_BLEND);
 sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
 sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();sceGuDisplay(GU_TRUE);
 audioChannel=sceAudioChReserve(-1,1024,PSP_AUDIO_FORMAT_MONO);
 SceCtrlData pad;unsigned int prev=0;
 while(1){
  sceCtrlReadBufferPositive(&pad,1);unsigned int pressed=pad.Buttons&~prev;prev=pad.Buttons;frame++;
  if(state==INTRO_COVER){if(++introFrames>180||(pressed&PSP_CTRL_CROSS)){state=INTRO_ROMS;introFrames=0;}}
  else if(state==INTRO_ROMS){if(++introFrames>120||(pressed&PSP_CTRL_CROSS)){state=INTRO_GC;introFrames=0;}}
  else if(state==INTRO_GC){if(++introFrames>120||(pressed&PSP_CTRL_CROSS)){state=TITLE;introFrames=0;}}
  else if(state==TITLE){
   if(pressed&(PSP_CTRL_UP|PSP_CTRL_LEFT)){menuIndex=(menuIndex+4)%5;sound(0);}
   if(pressed&(PSP_CTRL_DOWN|PSP_CTRL_RIGHT)){menuIndex=(menuIndex+1)%5;sound(0);}
   if(pressed&PSP_CTRL_CIRCLE){state=TITLE;menuIndex=0;}
   if(pressed&PSP_CTRL_CROSS){
    if(menuIndex==0){reset_game();}
    else if(menuIndex==1){reset_game();} /* save-data system will replace this temporary fallback */
    else if(menuIndex==2){reset_game();state=TRAINING;}
    else if(menuIndex==3){reset_game();}
    else if(menuIndex==4){state=SETTINGS;settingIndex=0;}
   }
  }else if(state==SETTINGS){
   if(pressed&(PSP_CTRL_UP|PSP_CTRL_LEFT))settingIndex=(settingIndex+2)%3;
   if(pressed&(PSP_CTRL_DOWN|PSP_CTRL_RIGHT))settingIndex=(settingIndex+1)%3;
   if(pressed&PSP_CTRL_CROSS){if(settingIndex==0)language=!language;else soundEnabled=!soundEnabled;}
   if(pressed&PSP_CTRL_CIRCLE)state=TITLE;
  }else if(state==TRAINING||state==PLAY){
   float dx=0,dy=0;
   if(pad.Buttons&PSP_CTRL_LEFT)dx=-1;if(pad.Buttons&PSP_CTRL_RIGHT)dx=1;
   if(pad.Buttons&PSP_CTRL_UP)dy=-1;if(pad.Buttons&PSP_CTRL_DOWN)dy=1;
   if(pad.Lx<105)dx=-1;if(pad.Lx>150)dx=1;if(pad.Ly<105)dy=-1;if(pad.Ly>150)dy=1;
   if(dx||dy){sx+=dx*1.7f;sy+=dy*1.2f;walk+=0.24f;}else walk=0;
   if(sx<20)sx=20;if(sx>460)sx=460;if(sy<112)sy=112;if(sy>247)sy=247;
   if(attack>0)attack-=1.0f;if(glow>0)glow-=0.7f;if(invuln>0)invuln--;
   if(pressed&PSP_CTRL_CIRCLE){attack=7;sound(0);int nearest=-1;float best=75;for(int i=0;i<5;i++)if(enemies[i].alive){float d=fabsf(enemies[i].x-sx)+fabsf(enemies[i].y-sy);if(d<best){best=d;nearest=i;}}if(nearest>=0){enemies[nearest].hp-=15;if(enemies[nearest].hp<=0){enemies[nearest].alive=0;score++;}else if(invuln==0){hp-=4;invuln=25;}}}
   if(pressed&PSP_CTRL_TRIANGLE){if(energy>=20){energy-=20;glow=15;attack=9;sound(1);for(int i=0;i<5;i++)if(enemies[i].alive&&fabsf(enemies[i].x-sx)<95&&fabsf(enemies[i].y-sy)<65){enemies[i].hp-=22;if(enemies[i].hp<=0){enemies[i].alive=0;score++;}}}else if(energy<100)energy+=8;}
   if((pad.Buttons&PSP_CTRL_TRIANGLE)==0&&energy<100)energy++;
   if((pressed&PSP_CTRL_SQUARE)&&tools>0){tools--;sound(2);for(int i=0;i<5;i++)if(enemies[i].alive&&fabsf(enemies[i].x-sx)<110){enemies[i].hp-=18;if(enemies[i].hp<=0){enemies[i].alive=0;score++;}}attack=6;}
   if(pressed&PSP_CTRL_RTRIGGER)invuln=40;
   if((pressed&PSP_CTRL_CROSS)&&score>=5){state=PORTAL;sound(1);}
   if(pressed&PSP_CTRL_CIRCLE && state==TRAINING){} 
   if(hp<=0)state=GAMEOVER;
  }else if(state==PORTAL){if(pressed&PSP_CTRL_CROSS){mission=2;state=TITLE;menuIndex=0;}}
  else if(state==GAMEOVER){if(pressed&PSP_CTRL_CROSS)reset_game();if(pressed&PSP_CTRL_CIRCLE){state=TITLE;menuIndex=0;}}
  render();sceKernelDelayThread(16000);
 }
 sceGuTerm();return 0;
}
