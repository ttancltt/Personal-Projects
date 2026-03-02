#version 400 core

#define MAX_CPU 256

layout (location = 0) in vec2 pos;

layout (std140) uniform Matrices
{
    mat4 mvp;
    mat4 ortho;
    mat4 vp_unclipped;
    mat4 mvp_unclipped;
    mat4 mv;
};

layout (std140) uniform CpuInfo
{
    float x_offset;
    float y_offset;
    float y_stride;
};

uniform isamplerBuffer RGBAColors;
uniform float ratios[MAX_CPU];
uniform int binding[MAX_CPU];
uniform float thickness[MAX_CPU];

out vec4 theColor;

void main()
{
    float ratio = ratios[gl_InstanceID];
    vec2 coord;
    int pu = binding[gl_InstanceID];
    float thickness = thickness[gl_InstanceID];

    coord.x = x_offset + ratio * pos.x;
    coord.y = y_offset + pos.y * thickness + pu * y_stride;

    gl_Position = ortho * vec4 (coord, -1.0, 1.0);

    int col = texelFetch (RGBAColors, gl_InstanceID).r;
    theColor = unpackUnorm4x8 (uint (col));
}
