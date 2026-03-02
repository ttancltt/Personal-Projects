#version 430 core
#extension GL_NV_uniform_buffer_std430_layout : enable

// Per-triangle information (triangle_info field):
// bit  [0] = isInner
// bits [1..3] = <edge0, edge1, edge2>
// bits [4..6] = <frontier0, frontier1, frontier2>
// bits [7..31] = cell_no (limited to 2^25 = 32M cells)
#define ISINNER 1
#define EDGE0 2
#define EDGE1 4
#define EDGE2 8
#define FRONTIER_SHIFT 4
#define CELLNO_SHIFT 7

#define CELLMASK_HIDDEN   256
#define CELLMASK_ALPHA    512
#define CELLMASK_CUT     1024
#define CELLMASK_CLIPPED 2048

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (std140) uniform Clipping
{
    float clipping_zcoord;
    float clipping_zproj;
    int clipping_active;
    float width;
    float height;
};

uniform isamplerBuffer TriangleInfo;

layout (std430) buffer MaskBuffer
{
    uint maskData[];
};

void main ()
{    
    vec4 me, other;
    float t[3], s = 0.0;
    int v;

    for (int i = 0; i < gl_in.length(); i++) {
        t[i] = step (clipping_zproj, gl_in[i].gl_Position.w);
        s += t[i];
    }

    int info = texelFetch (TriangleInfo, gl_PrimitiveIDIn).r;
    int cell = info >> CELLNO_SHIFT; // cell no.

    // Hide triangle if cut or in front of cut plane
    if (s < 3.0) {
        atomicOr (maskData[cell], CELLMASK_CLIPPED);
        if (s > 0.0)
            atomicOr (maskData[cell], CELLMASK_CUT);
    }
}
