#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;    
    float shininess;
}; 

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// Inputs from vertex shader
in vec3 FragPos;  
in vec3 Normal; 
in vec2 TexCoords;

// Textures
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform sampler2D texture7;
uniform sampler2D texture8;
uniform sampler2D texture9;
uniform sampler2D texture10;

// Aggiunto colore diffuso
uniform vec4 diffuseColor;
uniform sampler2D texture_diffuse1;
uniform bool useTexture;

// Uniform to determine which texture to use
uniform int textureID;

// Lighting uniforms
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform vec3 laserColor;
uniform bool isLaser;

out vec4 FragColor;

vec2 distortUV(vec2 uv, float time) {
    float distortion = sin(uv.y * 10.0 + time * 5.0) * 0.02; // Distorsione sinusoidale
    return uv + vec2(distortion, 0.0);
}

void main()
{
    if (isLaser) {
        // Calcola la distanza dal centro del laser (supponendo che TexCoords siano coordinate normalizzate)
        float gradient = 1.0 - abs(TexCoords.y - 0.5) * 2.0; // Gradiente verticale
        vec3 laserGradientColor = laserColor * gradient; // Applica il gradiente al colore del laser

        FragColor = vec4(laserGradientColor, 1.0);
        return;
    }

    // Codice esistente per texture e illuminazione
    vec4 texColor;
    if (textureID == 0)
        texColor = texture(texture0, TexCoords);
    else if (textureID == 1)
        texColor = texture(texture1, TexCoords);
    else if (textureID == 2)
        texColor = texture(texture2, TexCoords);
    else if (textureID == 3)
        texColor = texture(texture3, TexCoords);
    else if (textureID == 4)
        texColor = texture(texture4, TexCoords);
    else if (textureID == 5)
        texColor = texture(texture5, TexCoords);
    else if (textureID == 6)
        texColor = texture(texture6, TexCoords);
    else if (textureID == 7)
        texColor = texture(texture7, TexCoords);
    else if (textureID == 8)
        texColor = texture(texture8, TexCoords);
    else if (textureID == 9)
        texColor = texture(texture9, TexCoords);
    else if (textureID == 10)
        texColor = texture(texture10, TexCoords);
    else
        texColor = vec4(1.0);

    // Lighting calculations
    vec3 ambient = light.ambient * material.ambient;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  

    vec3 lighting = ambient + diffuse + specular;

    vec3 colorOutput = useTexture ? texColor.rgb : diffuseColor.rgb;
    vec3 result = lighting * texColor.rgb;

    FragColor = vec4(result, texColor.a);
}