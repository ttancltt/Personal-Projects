#version 400 core

#define CELLNO_SHIFT 7

#define CELLMASK_HIDDEN 256
#define CELLMASK_ALPHA  512

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform isamplerBuffer TriangleInfo;
uniform isamplerBuffer CellMask;

flat out int primitive;

void main()
{
    int info, ind, mask;
    int prim = gl_PrimitiveIDIn;

    info = texelFetch (TriangleInfo, gl_PrimitiveIDIn).r;
    ind = info >> CELLNO_SHIFT; // cell no.
    mask = texelFetch (CellMask, ind).r;
    if ((mask & CELLMASK_HIDDEN) > 0)
        prim = -1; // skip this triangle
    if (mask == CELLMASK_ALPHA) // cell is completely transparent
        prim = -1; // skip this triangle

    // vertex passthrough
    for (int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        primitive = prim;
        EmitVertex();
    }
}
