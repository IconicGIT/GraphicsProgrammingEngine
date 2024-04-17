///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here


struct Light
{
	unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};



layout(location = 0) in vec3 aPosition;	// world space
layout(location = 1) in vec3 aNormal;	// world space
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout (binding = 0, std140) uniform globalParams
{
	vec3			uCameraPosition;
	unsigned int	uLightCount;
	Light			uLight[16];
};

layout(binding = 1, std140) uniform localParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;
out vec3 vLightDir;
out vec3 vLightCol;


void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal =	vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	vLightCol = uLight[0].color;

	if (uLight[0].type == 1)
	{
		vLightDir = normalize(uLight[0].direction);
	}

	if (uLight[0].type == 2)
	{
		vLightDir = normalize(uLight[0].position - vPosition);
	}

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;
in vec3 vLightDir;
in vec3 vLightCol;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	vec4 col = texture(uTexture, vTexCoord);


	float lightEff = dot(vNormal, vLightDir);
	vec3 col_ = mix(vec3(0), col.xyz, lightEff);
	
	oColor = vec4(col_, 1);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
