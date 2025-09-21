#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = texture(texSampler, fragTexCoord);
    outColor.rgb = pow(outColor.rgb, vec3(1.0/2.2));

    if (outColor.w < 0.8)
    {
        discard;
    }
}
