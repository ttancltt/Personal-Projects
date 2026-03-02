#version 430 core
#extension GL_NV_uniform_buffer_std430_layout : enable

#define CELLNO_SHIFT 7

#define CELLMASK_HIDDEN   256
#define CELLMASK_ALPHA    512
#define CELLMASK_CUT     1024
#define CELLMASK_CLIPPED 2048

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform isamplerBuffer TriangleInfo;

layout (std430) buffer CellBuffer
{
    uint cellMask[];
};

flat out int primitive;

void main()
{
    int info, ind;
    uint mask;
    int prim = gl_PrimitiveIDIn;

    info = texelFetch (TriangleInfo, gl_PrimitiveIDIn).r;
    ind = info >> CELLNO_SHIFT; // cell no.

    mask = cellMask[ind];
    if ((mask & (CELLMASK_HIDDEN + CELLMASK_CLIPPED)) > 0)
        prim = -1; // skip this triangle

    // vertex passthrough
    for (int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        primitive = prim;
        EmitVertex();
    }
}
