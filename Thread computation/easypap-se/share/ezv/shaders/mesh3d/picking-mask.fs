#version 400 core

out float FragColor;

flat in int primitive;

void main()
{
    if (primitive < 0)
        discard;

    FragColor = float (primitive + 1);
}
