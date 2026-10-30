#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <math.h>
#include "training3d.h"

#define GRID_X 24
#define GRID_Z 28
#define CELL 10.0f
#define TERRAIN_VERTS ((GRID_X - 1) * (GRID_Z - 1) * 6)

typedef struct {
    unsigned int color;
    float x, y, z;
} TerrainVertex;

static TerrainVertex terrain[TERRAIN_VERTS] __attribute__((aligned(16)));
static int terrain_ready = 0;

static unsigned int color_rgba(int r, int g, int b, int a)
{
    return ((unsigned int)a << 24) | ((unsigned int)b << 16) |
           ((unsigned int)g << 8) | (unsigned int)r;
}

static float terrain_height(float x, float z)
{
    float hills = 2.2f * sinf(x * 0.035f) * cosf(z * 0.027f);
    float ridge_a = 13.0f * expf(-((x + 73.0f) * (x + 73.0f)) / 850.0f)
                    * expf(-((z + 92.0f) * (z + 92.0f)) / 3000.0f);
    float ridge_b = 17.0f * expf(-((x - 78.0f) * (x - 78.0f)) / 1050.0f)
                    * expf(-((z + 145.0f) * (z + 145.0f)) / 4200.0f);
    float clearing = expf(-((x * x) / 2100.0f) - (((z + 18.0f) * (z + 18.0f)) / 1500.0f));
    return hills + ridge_a + ridge_b - clearing * 2.0f;
}

static unsigned int terrain_color(float x, float z, float y)
{
    if (y > 12.0f)
        return color_rgba(112, 105, 91, 255);
    if (z < -165.0f)
        return color_rgba(65, 91, 65, 255);
    if (fabsf(x) < 19.0f && z > -115.0f)
        return color_rgba(171, 133, 83, 255);
    if (y > 6.0f)
        return color_rgba(105, 119, 70, 255);
    return color_rgba(91, 126, 66, 255);
}

static void add_vertex(int *n, float x, float z)
{
    float y = terrain_height(x, z);
    terrain[*n].x = x;
    terrain[*n].y = y;
    terrain[*n].z = z;
    terrain[*n].color = terrain_color(x, z, y);
    ++*n;
}

static void build_terrain(void)
{
    int n = 0;
    int ix, iz;
    if (terrain_ready)
        return;

    for (iz = 0; iz < GRID_Z - 1; ++iz) {
        for (ix = 0; ix < GRID_X - 1; ++ix) {
            float x0 = ((float)ix - (float)(GRID_X - 1) * 0.5f) * CELL;
            float x1 = x0 + CELL;
            float z0 = 35.0f - (float)iz * CELL;
            float z1 = z0 - CELL;

            /* Dois triangulos por quadrado: terreno realmente tridimensional. */
            add_vertex(&n, x0, z0);
            add_vertex(&n, x1, z0);
            add_vertex(&n, x0, z1);

            add_vertex(&n, x1, z0);
            add_vertex(&n, x1, z1);
            add_vertex(&n, x0, z1);
        }
    }
    sceKernelDcacheWritebackRange(terrain, sizeof(terrain));
    terrain_ready = 1;
}

static void draw_distant_ridge(float x, float z, float width,
                               float peak, unsigned int color)
{
    TerrainVertex ridge[3] __attribute__((aligned(16)));
    ridge[0].color = color;
    ridge[0].x = x - width; ridge[0].y = -1.0f; ridge[0].z = z;
    ridge[1].color = color;
    ridge[1].x = x; ridge[1].y = peak; ridge[1].z = z - 12.0f;
    ridge[2].color = color;
    ridge[2].x = x + width; ridge[2].y = -1.0f; ridge[2].z = z;
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        3, 0, ridge);
}


/* Elementos estruturais do campo: caminhos, plataformas e marcos de treino. */
static void draw_ground_quad(float x0, float z0, float x1, float z1,
                             float lift, unsigned int color)
{
    TerrainVertex v[6] __attribute__((aligned(16)));
    float xa = x0, xb = x1, za = z0, zb = z1;
    float ya = terrain_height(xa, za) + lift;
    float yb = terrain_height(xb, za) + lift;
    float yc = terrain_height(xa, zb) + lift;
    float yd = terrain_height(xb, zb) + lift;
    v[0] = (TerrainVertex){color, xa, ya, za};
    v[1] = (TerrainVertex){color, xb, yb, za};
    v[2] = (TerrainVertex){color, xa, yc, zb};
    v[3] = (TerrainVertex){color, xb, yb, za};
    v[4] = (TerrainVertex){color, xb, yd, zb};
    v[5] = (TerrainVertex){color, xa, yc, zb};
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        6, 0, v);
}

static void draw_training_disc(float cx, float cz, float radius,
                               unsigned int color)
{
    TerrainVertex v[24 * 3] __attribute__((aligned(16)));
    int n = 0, i;
    for (i = 0; i < 24; ++i) {
        float a = (float)i * 0.261799f;
        float b = (float)(i + 1) * 0.261799f;
        float x1 = cx + cosf(a) * radius;
        float z1 = cz + sinf(a) * radius;
        float x2 = cx + cosf(b) * radius;
        float z2 = cz + sinf(b) * radius;
        float y0 = terrain_height(cx, cz) + 0.65f;
        v[n++] = (TerrainVertex){color, cx, y0, cz};
        v[n++] = (TerrainVertex){color, x1, terrain_height(x1,z1)+0.65f, z1};
        v[n++] = (TerrainVertex){color, x2, terrain_height(x2,z2)+0.65f, z2};
    }
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        n, 0, v);
}

static void draw_training_post(float x, float z, float height,
                               unsigned int color)
{
    TerrainVertex v[36] __attribute__((aligned(16)));
    float y = terrain_height(x,z) + 0.3f;
    float w = 1.5f;
    float x0=x-w, x1=x+w, z0=z-w, z1=z+w, top=y+height;
    TerrainVertex p[8] = {
        {color,x0,y,z0},{color,x1,y,z0},{color,x1,y,z1},{color,x0,y,z1},
        {color,x0,top,z0},{color,x1,top,z0},{color,x1,top,z1},{color,x0,top,z1}
    };
    const unsigned char faces[36] = {
        0,1,5, 0,5,4, 1,2,6, 1,6,5,
        2,3,7, 2,7,6, 3,0,4, 3,4,7,
        4,5,6, 4,6,7, 0,3,2, 0,2,1
    };
    int i;
    for(i=0;i<36;i++) v[i]=p[faces[i]];
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        36, 0, v);
}

static void draw_training_layout(void)
{
    unsigned int sand = color_rgba(174, 139, 91, 255);
    unsigned int edge = color_rgba(83, 67, 52, 255);
    unsigned int stone = color_rgba(112, 119, 112, 255);
    unsigned int gold = color_rgba(225, 173, 68, 255);
    unsigned int blue = color_rgba(62, 133, 164, 255);
    int i;

    /* Caminho principal liga a entrada ao núcleo de treino. */
    draw_ground_quad(-8.0f, 36.0f, 8.0f, -18.0f, 0.28f, sand);
    draw_ground_quad(-30.0f, -18.0f, 30.0f, -13.0f, 0.32f, sand);

    /* Área central delimitada por uma moldura baixa de pedra. */
    draw_ground_quad(-27.0f, -62.0f, 27.0f, -14.0f, 0.42f, edge);
    draw_ground_quad(-23.0f, -58.0f, 23.0f, -18.0f, 0.55f, sand);
    draw_ground_quad(-20.0f, -55.0f, 20.0f, -21.0f, 0.63f,
                     color_rgba(184,151,105,255));

    /* Três estações distintas: combate, energia e controlo de precisão. */
    draw_training_disc(-62.0f, -29.0f, 13.0f, blue);
    draw_training_disc(62.0f, -29.0f, 13.0f, gold);
    draw_training_disc(0.0f, -133.0f, 16.0f, stone);
    draw_ground_quad(-62.0f, -17.0f, -48.0f, -41.0f, 0.3f, sand);
    draw_ground_quad(48.0f, -17.0f, 62.0f, -41.0f, 0.3f, sand);
    draw_ground_quad(-7.0f, -104.0f, 7.0f, -117.0f, 0.3f, sand);

    /* Postes de treino e marcos de orientação. */
    draw_training_post(-62.0f,-29.0f,12.0f,stone);
    draw_training_post(62.0f,-29.0f,12.0f,stone);
    draw_training_post(0.0f,-133.0f,17.0f,blue);
    for(i=0;i<7;i++) {
        float z = 24.0f - (float)i * 13.0f;
        draw_training_post(-13.0f,z,4.5f,edge);
        draw_training_post(13.0f,z,4.5f,edge);
    }
    /* Balizas douradas nos quatro cantos da zona central. */
    draw_training_post(-25.0f,-16.0f,7.0f,gold);
    draw_training_post(25.0f,-16.0f,7.0f,gold);
    draw_training_post(-25.0f,-60.0f,7.0f,gold);
    draw_training_post(25.0f,-60.0f,7.0f,gold);
}

void training3d_render(void)
{
    ScePspFVector3 eye = { 0.0f, 30.0f, 72.0f };
    ScePspFVector3 target = { 0.0f, 1.0f, -48.0f };
    ScePspFVector3 up = { 0.0f, 1.0f, 0.0f };

    build_terrain();

    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuDepthMask(GU_FALSE);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_TEXTURE_2D);

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(55.0f, 480.0f / 272.0f, 0.5f, 650.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&eye, &target, &up);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    /* Silhuetas de montanhas ao fundo para dar escala e profundidade. */
    draw_distant_ridge(-105.0f, -205.0f, 95.0f, 52.0f,
                       color_rgba(65, 88, 76, 255));
    draw_distant_ridge(45.0f, -220.0f, 120.0f, 68.0f,
                       color_rgba(76, 98, 79, 255));
    draw_distant_ridge(155.0f, -190.0f, 82.0f, 43.0f,
                       color_rgba(58, 82, 72, 255));

    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        TERRAIN_VERTS, 0, terrain);

    /* A estrutura é desenhada sobre o relevo, com teste de profundidade activo. */
    draw_training_layout();

    /* Manter escrita de profundidade activa para que o clear do frame seguinte funcione. */
    sceGuDepthMask(GU_FALSE);
    sceGuDisable(GU_DEPTH_TEST);
}
