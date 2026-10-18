/*
 * Saiba Ultimate: Guerreiro do Multiverso
 * Capitulo 1 — A Fenda Entre Mundos
 * PSP SDK / PPSSPP prototype. Explore Africaca, speak with NPCs,
 * follow the elders, fight basic demons, then meet Goku at the portal.
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("Saiba Ultimate", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

typedef enum { TITLE, EXPLORE, DIALOGUE, FIGHT, MISSION_CLEAR, MISSION_TWO, GAME_OVER } GameState;
typedef struct { const char *name; const char *desc; int exits[4]; int npc; int demons; } Area;
typedef struct { const char *name; const char *line; int elder; } NPC;
typedef struct { const char *name; int hp; int attack; } Demon;

static Area areas[] = {
 {"Praca das Baobas", "Uma praca central com baobas antigos, bancas e casas de barro vermelho.", {1,2,-1,-1}, 0, 0},
 {"Rua dos Teceloes", "Tecidos coloridos balancam entre casas; ouves passos e rosnados numa viela.", {0,3,-1,-1}, 1, 2},
 {"Mercado de Africaca", "Cestos, frutas e ceramica espalhados. Os moradores procuram abrigo.", {0,4,-1,-1}, 2, 2},
 {"Beco das Forjas", "O cheiro de ferro quente mistura-se com fumo negro vindo de um portal menor.", {1,5,-1,-1}, 3, 3},
 {"Rua dos Ancioes", "Uma avenida estreita leva ao conselho da aldeia; ha marcas de garras nas paredes.", {2,5,-1,-1}, 4, 1},
 {"Santuario do Vento", "Panos rituais rodeiam pedras ancestrais. Uma luz dourada pulsa ao longe.", {3,4,-1,-1}, 5, 2}
};
#define AREA_COUNT 6
static NPC npcs[] = {
 {"Malaika, a curandeira", "\"Saje! Os demonios sairam de uma fenda durante a noite. Ajuda os nossos!\""},
 {"Kito, aprendiz de ferreiro", "\"Vi-os a bloquear a Rua dos Teceloes. Os golpes comuns funcionam, mas mantem distancia!\""},
 {"Nia, comerciante", "\"Levaram os alimentos e assustaram as familias. Ha criaturas no mercado e no beco!\""},
 {"Bako, guarda da aldeia", "\"A passagem do santuario esta perigosa. Derrota os demonios e segue para norte!\""},
 {"Ancia Amara", "\"Saje Capanji, os quatro elementos vivem em ti. A aldeia precisa da tua coragem.\""},
 {"Ancia Kwezi", "\"Segue as marcas douradas ate ao portal. Depois do ultimo demonio, alguem de outro mundo espera por ti.\""}
};
static Demon demons[] = {
 {"Demonio de Cinzas", 24, 5}, {"Lacaio Sombrio", 28, 6},
 {"Demonio Rastejante", 22, 4}, {"Demonio da Fenda", 32, 7}
};
static GameState state = TITLE;
static int area_id=0, hp=120, energy=2, tools=5, defeated=0, talked[6]={0};
static int fight_area=-1, demon_index=0, demon_hp=0, dialogue_npc=-1, chapter2_started=0;
static char message[160]="";
static unsigned int tick=0;

static void say(const char *s) { snprintf(message,sizeof(message),"%s",s); }
static void begin_fight(int area) {
 fight_area=area; demon_index=0; demon_hp=demons[(tick+area)%4].hp;
 state=FIGHT;
 snprintf(message,sizeof(message),"Um demonio bloqueia o caminho! O: atacar, Triangulo: poder elemental.");
}
static void next_demon(void) {
 defeated++;
 int left=areas[fight_area].demons-(defeated%10);
 (void)left;
 if (demon_index+1 < areas[fight_area].demons) {
   demon_index++;
   demon_hp=demons[(tick+demon_index+fight_area)%4].hp;
   snprintf(message,sizeof(message),"Outro demonio surge! %s.",demons[(tick+demon_index+fight_area)%4].name);
 } else {
   areas[fight_area].demons=0;
   state=EXPLORE;
   snprintf(message,sizeof(message),"Zona segura. Os moradores podem voltar a sair.");
 }
}
static void draw_bar(const char *label,int value,int max) {
 int n=max>0?value*16/max:0; if(n<0)n=0;if(n>16)n=16;
 pspDebugScreenPrintf("%s [",label);
 for(int i=0;i<16;i++) pspDebugScreenPrintf(i<n?"#":"-");
 pspDebugScreenPrintf("] %d/%d\n",value,max);
}
static void open_npc(int id) {
 dialogue_npc=id;
 state=DIALOGUE;
 snprintf(message,sizeof(message),"%s",npcs[id].line);
 if(id>=4 && !talked[id]) {
   talked[id]=1;
   if(id==4) say("\"Saje, os ancioes estao no santuario. Limpa as ruas e vem falar connosco!\"");
   if(id==5) say("\"O ultimo demonio protege o portal. Vence-o e encontra quem esta do outro lado.\"");
 }
}
static void draw_world(void) {
 pspDebugScreenPrintf("CAPITULO 1: A FENDA ENTRE MUNDOS | DIFICULDADE: FACIL\n");
 pspDebugScreenPrintf("SAJE CAPANJI   ");
 draw_bar("VIDA",hp,120);
 pspDebugScreenPrintf("Energia: %d/5   Ferramentas: %d   Demonios vencidos: %d\n\n",energy,tools,defeated);
 pspDebugScreenPrintf("LOCAL: %s\n%s\n\n",areas[area_id].name,areas[area_id].desc);
 pspDebugScreenPrintf("Pessoas por perto: %s\n",npcs[areas[area_id].npc].name);
 pspDebugScreenPrintf("Saidas: CIMA / BAIXO / ESQUERDA / DIREITA\n");
 pspDebugScreenPrintf("X: falar/interagir  O: ataque se houver demonios\n");
 pspDebugScreenPrintf("TRIANGULO: concentrar elemento  QUADRADO: ferramenta\n");
 pspDebugScreenPrintf("R: defesa  START: reiniciar capitulo\n\n");
 pspDebugScreenPrintf("%s\n",message);
}
int main(void) {
 SceCtrlData pad; unsigned int prev=0;
 sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG); pspDebugScreenInit();
 while(1) {
  sceCtrlReadBufferPositive(&pad,1);
  unsigned int pressed=pad.Buttons & ~prev; prev=pad.Buttons;
  pspDebugScreenSetXY(0,0); pspDebugScreenClear();
  pspDebugScreenPrintf("===============================================\n SAIBA ULTIMATE: GUERREIRO DO MULTIVERSO\n===============================================\n\n");
  switch(state) {
  case TITLE:
   pspDebugScreenPrintf("CAPITULO 1 — A FENDA ENTRE MUNDOS\n\nALDEIA AFRICACA\nA noite caiu sobre a aldeia. Uma fenda abriu-se no ceu\nlibertando demonios enviados para destruir e oprimir o povo.\nSaje Capanji, guardiao dos quatro elementos, corre para ajudar.\n\nX: iniciar missao\n");
   if(pressed&PSP_CTRL_CROSS){state=EXPLORE;area_id=0;hp=120;energy=2;tools=5;defeated=0;say("Malaika: \"Saje! Ainda ha gente presa nas ruas!\"");}
   break;
  case EXPLORE:
   draw_world();
   if(pressed&PSP_CTRL_START){state=TITLE;memset(talked,0,sizeof(talked));for(int i=0;i<AREA_COUNT;i++)areas[i].demons=(int[]){0,2,2,3,1,2}[i];say("A aventura foi reiniciada.");break;}
   if(pressed&PSP_CTRL_UP){int n=areas[area_id].exits[0];if(n>=0){area_id=n;say("Avancas pela aldeia. Ouves os gritos dos demonios ao longe.");}else say("Nao ha passagem nessa direcao.");}
   if(pressed&PSP_CTRL_DOWN){int n=areas[area_id].exits[1];if(n>=0){area_id=n;say("Exploras outra rua de Africaca.");}else say("A rua termina aqui.");}
   if(pressed&PSP_CTRL_LEFT){int n=areas[area_id].exits[2];if(n>=0){area_id=n;say("Segues pela rua lateral.");}else say("Nao ha passagem nessa direcao.");}
   if(pressed&PSP_CTRL_RIGHT){int n=areas[area_id].exits[3];if(n>=0){area_id=n;say("Segues pela rua lateral.");}else say("Nao ha passagem nessa direcao.");}
   if((pressed&PSP_CTRL_CROSS)){
    int n=areas[area_id].npc;
    if((area_id==4||area_id==5) && areas[area_id].demons>0) say("Os demonios impedem-te de chegar aos ancioes. Limpa esta zona!");
    else if((area_id==4||area_id==5) && defeated<10) open_npc(n);
    else if(area_id==5 && areas[area_id].demons==0) {state=MISSION_CLEAR;say("A aldeia esta segura. Um guerreiro de cabelo espetado surge junto ao portal.");}
    else open_npc(n);
   }
   if((pressed&PSP_CTRL_CIRCLE)&&areas[area_id].demons>0)begin_fight(area_id);
   if((pressed&PSP_CTRL_TRIANGLE)&&energy<5){energy++;say("Saje inspira. O ar, a agua, o fogo e a terra respondem; energia +1.");}
   if((pressed&PSP_CTRL_SQUARE)&&tools>0&&areas[area_id].demons>0){tools--;begin_fight(area_id);demon_hp-=10;if(demon_hp<0)demon_hp=0;say("FERRAMENTA! A lamina atinge o demonio (-10).");}
   if((pressed&PSP_CTRL_RTRIGGER))say("Saje ergue a guarda. O proximo golpe sera reduzido.");
   if(areas[area_id].demons>0 && (pressed&(PSP_CTRL_CIRCLE|PSP_CTRL_SQUARE))==0 && (tick++%240)==0){hp-=3;if(hp<1)state=GAME_OVER;say("Um demonio ataca de longe! Perdes 3 pontos de vida.");}
   if(area_id==5 && areas[5].demons==0 && defeated>=10){say("Os ancioes esperam por ti. Pressiona X para falar com eles.");}
   break;
  case DIALOGUE:
   pspDebugScreenPrintf("AFRICACA — CONVERSA\n\n%s\n\n",message);
   pspDebugScreenPrintf("X: continuar / terminar conversa\n");
   if(pressed&PSP_CTRL_CROSS){state=EXPLORE;if(dialogue_npc==5&&defeated>=10){state=MISSION_CLEAR;say("Ao longe, um portal azul abre-se. Um guerreiro desconhecido aproxima-se.");}else say("A conversa terminou. Continua a explorar Africaca.");}
   break;
  case FIGHT: {
   int di=(tick+demon_index+fight_area)%4;
   pspDebugScreenPrintf("COMBATE — AFRICACA | MODO FACIL\n");
   draw_bar("SAJE",hp,120);draw_bar(demons[di].name,demon_hp,demons[di].hp);
   pspDebugScreenPrintf("Energia: %d/5 | Ferramentas: %d\n\n",energy,tools);
   pspDebugScreenPrintf("O: golpe  TRIANGULO: tecnica elemental\nQUADRADO: ferramenta  R: bloquear  X: recuar\n\n%s\n",message);
   if(pressed&PSP_CTRL_CROSS){state=EXPLORE;say("Recuas para criar distancia.");}
   if(pressed&PSP_CTRL_RTRIGGER){say("Defesa! Saje protege-se do contra-ataque.");}
   if(pressed&PSP_CTRL_TRIANGLE){if(energy<5)energy++;say("Os quatro elementos giram a volta de Saje. Energia +1.");}
   if((pressed&PSP_CTRL_SQUARE)&&tools>0){tools--;demon_hp-=12;say("A ferramenta acerta o alvo! -12 HP.");}
   if(pressed&PSP_CTRL_CIRCLE){int dmg=14;if(energy>0&&(pad.Buttons&PSP_CTRL_TRIANGLE)){dmg=32;energy--;say("RAJADA DOS QUATRO ELEMENTOS! -32 HP.");}else say("Saje desfere um combo elemental! -14 HP.");demon_hp-=dmg;}
   if(demon_hp<=0){next_demon();break;}
   if(pressed&(PSP_CTRL_CIRCLE|PSP_CTRL_SQUARE)){hp-=demons[di].attack;if(hp<1)state=GAME_OVER;}
   break; }
  case MISSION_CLEAR:
   pspDebugScreenPrintf("MISSAO 1 CONCLUIDA: AFRICACA RESISTE!\n\n");
   pspDebugScreenPrintf("Os ultimos demonios desaparecem em fumo negro.\nAs portas abrem-se e os habitantes saem dos esconderijos.\nOs ancioes agradecem a Saje por salvar a aldeia.\n\n");
   pspDebugScreenPrintf("Junto a um portal azul esta Goku, em forma base.\nGoku: \"Tu tambem sentiste esta energia? O portal liga\nmundos diferentes. Precisamos de descobrir quem o abriu.\"\n\n");
   pspDebugScreenPrintf("X: iniciar Missao 2 a partir do portal\n");
   if(pressed&PSP_CTRL_CROSS){state=MISSION_TWO;chapter2_started=1;say("Goku: \"Vamos atravessar juntos. Fica atento, Saje.\"");}
   break;
  case MISSION_TWO:
   pspDebugScreenPrintf("CAPITULO 1 — MISSAO 2: ALEM DO PORTAL\n\n");
   pspDebugScreenPrintf("PONTO DE CONTINUACAO: PORTAL AZUL, AFRICACA\nSaje e Goku estao lado a lado. A passagem pulsa e\nmostra ruinas de um mundo desconhecido.\n\n%s\n\n",message);
   pspDebugScreenPrintf("A continuacao narrativa esta preparada; a proxima\netapa acrescentara a area do outro lado do portal.\n\n");
   pspDebugScreenPrintf("START: voltar ao inicio do capitulo\n");
   if(pressed&PSP_CTRL_START){state=TITLE;chapter2_started=0;}
   break;
  case GAME_OVER:
   pspDebugScreenPrintf("SAJE FOI DERROTADO\n\nAfricaca ainda precisa de ti.\nX: tentar novamente\n");
   if(pressed&PSP_CTRL_CROSS){hp=120;energy=2;tools=5;defeated=0;area_id=0;for(int i=0;i<AREA_COUNT;i++)areas[i].demons=(int[]){0,2,2,3,1,2}[i];state=EXPLORE;say("Malaika: \"Levanta-te, Saje. A aldeia conta contigo!\"");}
   break;
  }
  sceDisplayWaitVblankStart();sceKernelDelayThread(1000);
 }
 return 0;
}
