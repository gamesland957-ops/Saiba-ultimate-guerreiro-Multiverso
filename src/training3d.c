#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <math.h>
#include "training3d.h"

#define GRID_X 24
#define GRID_Z 28
#define CELL 10.0f
#define TERRAIN_VERTS (GRID_X * GRID_Z * 6)

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
    TerrainVertex ridge[3];
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

    sceGuDepthMask(GU_TRUE);
    sceGuDisable(GU_DEPTH_TEST);
}
