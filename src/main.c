/*
 * Saiba Ultimate: Guerreiro do Multiverso
 * First playable PSP prototype — text-rendered combat scaffold.
 * Controls: D-pad Left/Right select character; X confirm/attack;
 * O special attack; Triangle guard; Start return/restart.
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("Saiba Ultimate", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

typedef enum { SCREEN_TITLE, SCREEN_SELECT, SCREEN_BATTLE, SCREEN_WIN, SCREEN_LOSE } Screen;
typedef struct {
    const char *name;
    const char *style;
    int hp;
    int energy;
    int attack;
    int special;
} Fighter;

static Fighter roster[] = {
    { "Goku", "Punhos e energia ki", 100, 3, 12, 28 },
    { "Ichigo Kurosaki", "Zangetsu e golpes espirituais", 100, 3, 14, 26 },
    { "Roronoa Zoro", "Santoryu — estilo de tres espadas", 110, 3, 13, 24 }
};
#define ROSTER_COUNT ((int)(sizeof(roster) / sizeof(roster[0])))

static Screen screen = SCREEN_TITLE;
static int selected = 0;
static int player_hp = 100, enemy_hp = 100, energy = 3;
static int frame_count = 0;
static int guard_frames = 0;
static char message[96] = "O multiverso esta a entrar em colapso...";

static void reset_battle(void) {
    player_hp = roster[selected].hp;
    enemy_hp = 100;
    energy = roster[selected].energy;
    frame_count = 0;
    guard_frames = 0;
    snprintf(message, sizeof(message), "Um guerreiro misterioso bloqueia o caminho!");
}

static void draw_bar(const char *label, int value, int max_value) {
    int filled = value * 20 / max_value;
    if (filled < 0) filled = 0;
    if (filled > 20) filled = 20;
    pspDebugScreenPrintf("%s [", label);
    for (int i = 0; i < 20; ++i) pspDebugScreenPrintf(i < filled ? "#" : "-");
    pspDebugScreenPrintf("] %d/%d\n", value, max_value);
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
            pspDebugScreenPrintf("Uma fenda rasga o ceu e mistura universos.\n");
            pspDebugScreenPrintf("Tres guerreiros sentem uma energia desconhecida.\n");
            pspDebugScreenPrintf("A primeira batalha vai decidir o destino\n");
            pspDebugScreenPrintf("deste novo mundo.\n\n");
            pspDebugScreenPrintf("Pressiona X para comecar\n");
            pspDebugScreenPrintf("Pressiona START para sair (a qualquer momento)\n");
            if (pressed & PSP_CTRL_CROSS) screen = SCREEN_SELECT;
            break;

        case SCREEN_SELECT:
            pspDebugScreenPrintf("ESCOLHE O TEU GUERREIRO\n");
            pspDebugScreenPrintf("D-PAD ESQUERDA/DIREITA para escolher; X confirmar\n\n");
            for (int i = 0; i < ROSTER_COUNT; ++i) {
                pspDebugScreenPrintf("%s %s\n", i == selected ? ">>" : "  ", roster[i].name);
                pspDebugScreenPrintf("   Estilo: %s\n", roster[i].style);
                if (i == 0) pspDebugScreenPrintf("   Estado inicial: sem transformacoes\n");
                pspDebugScreenPrintf("\n");
            }
            if ((pressed & PSP_CTRL_LEFT) && selected > 0) selected--;
            if ((pressed & PSP_CTRL_RIGHT) && selected < ROSTER_COUNT - 1) selected++;
            if (pressed & PSP_CTRL_CROSS) { reset_battle(); screen = SCREEN_BATTLE; }
            break;

        case SCREEN_BATTLE:
            pspDebugScreenPrintf("CAPITULO 1 — COMBATE DE TREINO\n");
            pspDebugScreenPrintf("Heroi: %s\n", roster[selected].name);
            draw_bar("JOGADOR", player_hp, roster[selected].hp);
            draw_bar("RIVAL  ", enemy_hp, 100);
            pspDebugScreenPrintf("Energia especial: %d/3\n\n", energy);
            pspDebugScreenPrintf("X  Ataque normal (-%d HP ao rival)\n", roster[selected].attack);
            pspDebugScreenPrintf("O  Tecnica especial (-%d HP; custa 1 energia)\n", roster[selected].special);
            pspDebugScreenPrintf("TRIANGULO  Defender do proximo golpe\n");
            pspDebugScreenPrintf("START  Reiniciar combate\n\n");
            pspDebugScreenPrintf("Situacao: %s\n", message);

            if (pressed & PSP_CTRL_START) reset_battle();
            if ((pressed & PSP_CTRL_TRIANGLE) && guard_frames == 0) {
                guard_frames = 1;
                snprintf(message, sizeof(message), "Preparaste a defesa para o proximo golpe.");
            } else if (pressed & PSP_CTRL_CROSS) {
                enemy_hp -= roster[selected].attack;
                if (enemy_hp < 0) enemy_hp = 0;
                snprintf(message, sizeof(message), "%s acerta um ataque direto!", roster[selected].name);
            } else if (pressed & PSP_CTRL_CIRCLE) {
                if (energy > 0) {
                    enemy_hp -= roster[selected].special;
                    if (enemy_hp < 0) enemy_hp = 0;
                    energy--;
                    snprintf(message, sizeof(message), "Tecnica especial! Energia restante: %d.", energy);
                } else {
                    snprintf(message, sizeof(message), "Sem energia! Usa ataques normais para continuar.");
                }
            }

            frame_count++;
            if (frame_count >= 45) {
                frame_count = 0;
                if (guard_frames) {
                    guard_frames = 0;
                    snprintf(message, sizeof(message), "Defesa eficaz: anulaste o ataque do rival.");
                } else {
                    player_hp -= 8;
                    if (player_hp < 0) player_hp = 0;
                    snprintf(message, sizeof(message), "O rival contra-ataca e causa 8 de dano.");
                }
            }
            if (enemy_hp <= 0) screen = SCREEN_WIN;
            else if (player_hp <= 0) screen = SCREEN_LOSE;
            sceDisplayWaitVblankStart();
            break;

        case SCREEN_WIN:
            pspDebugScreenPrintf("VITORIA!\n\n");
            pspDebugScreenPrintf("%s venceu o primeiro confronto.\n", roster[selected].name);
            pspDebugScreenPrintf("O portal continua aberto... a aventura so comecou.\n\n");
            pspDebugScreenPrintf("X: escolher outro guerreiro\nSTART: repetir combate\n");
            if (pressed & PSP_CTRL_CROSS) screen = SCREEN_SELECT;
            if (pressed & PSP_CTRL_START) { reset_battle(); screen = SCREEN_BATTLE; }
            break;

        case SCREEN_LOSE:
            pspDebugScreenPrintf("DERROTA\n\n");
            pspDebugScreenPrintf("O rival foi mais forte desta vez.\n");
            pspDebugScreenPrintf("Tenta outra estrategia e gere a tua energia.\n\n");
            pspDebugScreenPrintf("X: tentar novamente\n");
            if (pressed & PSP_CTRL_CROSS) { reset_battle(); screen = SCREEN_BATTLE; }
            break;
        }

        sceDisplayWaitVblankStart();
        sceKernelDelayThread(1000);
    }
    return 0;
}
