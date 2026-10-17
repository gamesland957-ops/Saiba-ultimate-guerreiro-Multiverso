/*
 * Saiba Ultimate: Guerreiro do Multiverso
 * PSP prototype: arena missions + one-on-one boss duels.
 * Controls: X jump; O attack/combos; Triangle charge energy;
 * Square use tools; R guard/evasion; L lock target; Start restart.
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("Saiba Ultimate", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

typedef enum { SCREEN_TITLE, SCREEN_SELECT, SCREEN_MISSION, SCREEN_BOSS, SCREEN_WIN, SCREEN_LOSE } Screen;
typedef struct {
    const char *name;
    const char *style;
    int hp;
    int energy;
    int attack;
    int special;
} Fighter;

static Fighter roster[] = {
    { "Saje Capanji", "Quatro elementos; energia dourada; Susanoo ancestral", 120, 2, 14, 32 },
    { "Goku", "Punhos e energia ki (forma base)", 100, 3, 12, 28 },
    { "Ichigo Kurosaki", "Zangetsu e golpes espirituais", 100, 3, 14, 26 },
    { "Roronoa Zoro", "Santoryu — estilo de tres espadas", 110, 3, 13, 24 }
};
#define ROSTER_COUNT ((int)(sizeof(roster) / sizeof(roster[0])))
#define MAX_ENEMIES 5

static Screen screen = SCREEN_TITLE;
static int selected = 0;
static int player_hp, energy, enemies_left, boss_hp, tools, combo, jump_state;
static int guard_frames, target_locked, ticks;
static char message[112] = "Saje sente a fenda entre mundos despertar.";

static void reset_mission(void) {
    player_hp = roster[selected].hp;
    energy = roster[selected].energy;
    enemies_left = MAX_ENEMIES;
    boss_hp = 120;
    tools = 5;
    combo = jump_state = guard_frames = target_locked = ticks = 0;
    snprintf(message, sizeof(message), "A arena esta cheia de inimigos. Avanca!");
}
static void draw_bar(const char *label, int value, int max_value) {
    int filled = max_value > 0 ? value * 20 / max_value : 0;
    if (filled < 0) filled = 0;
    if (filled > 20) filled = 20;
    pspDebugScreenPrintf("%s [", label);
    for (int i = 0; i < 20; ++i) pspDebugScreenPrintf(i < filled ? "#" : "-");
    pspDebugScreenPrintf("] %d/%d\n", value, max_value);
}
static void take_damage(int amount) {
    if (guard_frames > 0) {
        guard_frames = 0;
        snprintf(message, sizeof(message), "Esquiva/guarda perfeita: nenhum dano!");
    } else {
        player_hp -= amount;
        if (player_hp < 0) player_hp = 0;
        snprintf(message, sizeof(message), "Foste atingido: -%d HP. Mantem-te em movimento!", amount);
    }
}
int main(void) {
    SceCtrlData pad;
    unsigned int previous = 0;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    pspDebugScreenInit();
    while (1) {
        sceCtrlReadBufferPositive(&pad, 1);
        unsigned int pressed = pad.Buttons & ~previous;
        previous = pad.Buttons;
        pspDebugScreenSetXY(0, 0);
        pspDebugScreenClear();
        pspDebugScreenPrintf("==============================================\n");
        pspDebugScreenPrintf("   SAIBA ULTIMATE: GUERREIRO DO MULTIVERSO\n");
        pspDebugScreenPrintf("==============================================\n\n");
        switch (screen) {
        case SCREEN_TITLE:
            pspDebugScreenPrintf("CAPITULO 1: A FENDA ENTRE MUNDOS\n\n");
            pspDebugScreenPrintf("Saje Capanji e o primeiro protagonista.\n");
            pspDebugScreenPrintf("O seu poder elementar desperta quando uma fenda\n");
            pspDebugScreenPrintf("mistura mundos e liberta guerreiros desconhecidos.\n\n");
            pspDebugScreenPrintf("X: comecar aventura\n");
            if (pressed & PSP_CTRL_CROSS) { selected = 0; reset_mission(); screen = SCREEN_MISSION; }
            break;
        case SCREEN_SELECT:
            pspDebugScreenPrintf("ESCOLHE O GUERREIRO\nD-PAD esquerda/direita; X confirmar\n\n");
            for (int i = 0; i < ROSTER_COUNT; ++i) {
                pspDebugScreenPrintf("%s %s\n   %s\n\n", i == selected ? ">>" : "  ", roster[i].name, roster[i].style);
            }
            if ((pressed & PSP_CTRL_LEFT) && selected > 0) selected--;
            if ((pressed & PSP_CTRL_RIGHT) && selected < ROSTER_COUNT - 1) selected++;
            if (pressed & PSP_CTRL_CROSS) { reset_mission(); screen = SCREEN_MISSION; }
            break;
        case SCREEN_MISSION:
            pspDebugScreenPrintf("MISSAO: ARENA DE INIMIGOS\nHeroi: %s\n", roster[selected].name);
            draw_bar("JOGADOR", player_hp, roster[selected].hp);
            pspDebugScreenPrintf("Inimigos restantes: %d/%d\n", enemies_left, MAX_ENEMIES);
            pspDebugScreenPrintf("Energia: %d/5 | Ferramentas: %d | Combo: x%d\n", energy, tools, combo);
            pspDebugScreenPrintf("X: saltar  O: atacar  TRIANGULO: carregar energia\n");
            pspDebugScreenPrintf("QUADRADO: lancar ferramenta  R: defesa/esquiva\n");
            pspDebugScreenPrintf("L: fixar alvo  START: reiniciar missao\n\n");
            pspDebugScreenPrintf("%s\n", message);
            if (pressed & PSP_CTRL_START) reset_mission();
            if (pressed & PSP_CTRL_CROSS) {
                jump_state = (jump_state + 1) % 3;
                snprintf(message, sizeof(message), jump_state == 0 ? "Aterragem!" : (jump_state == 1 ? "Salto! X novamente para salto duplo." : "Salto duplo!"));
            }
            if (pressed & PSP_CTRL_TRIANGLE) {
                if (energy < 5) energy++;
                snprintf(message, sizeof(message), "Energia concentrada: %d/5.", energy);
            }
            if (pressed & PSP_CTRL_RTRIGGER) {
                guard_frames = 1;
                snprintf(message, sizeof(message), "Preparado para bloquear/esquivar do proximo ataque.");
            }
            if (pressed & PSP_CTRL_LTRIGGER) {
                target_locked = !target_locked;
                snprintf(message, sizeof(message), target_locked ? "Alvo fixado." : "Fixacao de alvo desligada.");
            }
            if ((pressed & PSP_CTRL_SQUARE) && tools > 0 && enemies_left > 0) {
                tools--;
                enemies_left--;
                combo = 0;
                snprintf(message, sizeof(message), "Ferramenta lancada! Um inimigo foi afastado.");
            }
            if ((pressed & PSP_CTRL_CIRCLE) && enemies_left > 0) {
                combo++;
                enemies_left -= (combo >= 3 ? 2 : 1);
                if (enemies_left < 0) enemies_left = 0;
                snprintf(message, sizeof(message), "Combo x%d! Golpe acertou a multidão.", combo);
            }
            if (enemies_left == 0) {
                snprintf(message, sizeof(message), "Arena limpa! O chefe da fenda aproxima-se...");
                screen = SCREEN_BOSS;
            } else if (++ticks >= 8) {
                ticks = 0;
                take_damage(guard_frames ? 0 : 7);
            }
            if (player_hp <= 0) screen = SCREEN_LOSE;
            break;
        case SCREEN_BOSS:
            pspDebugScreenPrintf("CHEFE: DUELO UM CONTRA UM\nHeroi: %s\n", roster[selected].name);
            draw_bar("JOGADOR", player_hp, roster[selected].hp);
            draw_bar("CHEFE  ", boss_hp, 120);
            pspDebugScreenPrintf("Energia: %d/5 | Ferramentas: %d\n\n", energy, tools);
            pspDebugScreenPrintf("X: saltar  O: combo  TRIANGULO: carregar\n");
            pspDebugScreenPrintf("QUADRADO: ferramenta  R: defender  START: reiniciar\n\n");
            pspDebugScreenPrintf("%s\n", message);
            if (pressed & PSP_CTRL_START) { reset_mission(); screen = SCREEN_MISSION; }
            if (pressed & PSP_CTRL_CROSS) snprintf(message, sizeof(message), "Salto evasivo! Aproxima-te para atacar.");
            if (pressed & PSP_CTRL_TRIANGLE) { if (energy < 5) energy++; snprintf(message, sizeof(message), "Energia carregada: %d/5.", energy); }
            if (pressed & PSP_CTRL_RTRIGGER) { guard_frames = 1; snprintf(message, sizeof(message), "Postura defensiva preparada."); }
            if ((pressed & PSP_CTRL_SQUARE) && tools > 0) { tools--; boss_hp -= 8; snprintf(message, sizeof(message), "Ferramenta acerta o chefe: -8 HP."); }
            if (pressed & PSP_CTRL_CIRCLE) {
                if ((pad.Buttons & PSP_CTRL_TRIANGLE) && energy > 0) {
                    energy--; boss_hp -= roster[selected].special;
                    snprintf(message, sizeof(message), "TECNICA ESPECIAL! -%d HP.", roster[selected].special);
                } else { boss_hp -= roster[selected].attack; snprintf(message, sizeof(message), "Sequencia de golpes: -%d HP.", roster[selected].attack); }
            }
            if (boss_hp < 0) boss_hp = 0;
            if (++ticks >= 10) { ticks = 0; take_damage(guard_frames ? 0 : 10); }
            if (boss_hp <= 0) screen = SCREEN_WIN;
            else if (player_hp <= 0) screen = SCREEN_LOSE;
            break;
        case SCREEN_WIN:
            pspDebugScreenPrintf("VITORIA!\n\n");
            pspDebugScreenPrintf("%s venceu o guardiao da fenda!\n", roster[selected].name);
            pspDebugScreenPrintf("A historia continua para alem deste portal...\n\n");
            pspDebugScreenPrintf("X: escolher personagem  START: repetir missao\n");
            if (pressed & PSP_CTRL_CROSS) screen = SCREEN_SELECT;
            if (pressed & PSP_CTRL_START) { reset_mission(); screen = SCREEN_MISSION; }
            break;
        case SCREEN_LOSE:
            pspDebugScreenPrintf("DERROTA\n\nTenta outra estrategia e gere a energia.\n");
            pspDebugScreenPrintf("X: tentar novamente\n");
            if (pressed & PSP_CTRL_CROSS) { reset_mission(); screen = SCREEN_MISSION; }
            break;
        }
        sceDisplayWaitVblankStart();
        sceKernelDelayThread(1000);
    }
    return 0;
}