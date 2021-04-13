#section vertex
#version 430 core
layout (location = 0) in vec3 aPos;

out vec2 fragPos;

void main() {
    fragPos = vec2((aPos.x + 1.0) / 2.0, (aPos.y + 1.0) / 2.0);
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}


#section fragment
#version 430 core

out vec4 FragColor;

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;

in vec2 fragPos;

void main() {
    vec3 albedo = texture(albedo, fragPos).rgb;

    FragColor = vec4(albedo, 1.0);
}