///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//#ifdef TEXTURED_GEOMETRY
//
//#if defined(VERTEX) ///////////////////////////////////////////////////
//
//// TODO: Write your vertex shader here
//
//
//struct Light
//{
//	unsigned int type;
//    vec3 color;
//    vec3 direction;
//    vec3 position;
//};
//
//
//
//layout(location = 0) in vec3 aPosition;	// world space
//layout(location = 1) in vec3 aNormal;	// world space
//layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;
//
//layout (binding = 0, std140) uniform globalParams
//{
//	vec3			uCameraPosition;
//	unsigned int	uLightCount;
//	Light			uLight[16];
//};
//
//layout(binding = 1, std140) uniform localParams
//{
//	mat4 uWorldMatrix;
//	mat4 uWorldViewProjectionMatrix;
//};
//
//out vec2 vTexCoord;
//out vec3 vPosition;
//out vec3 vNormal;
//out vec3 vViewDir;
//out vec3 vLightDir;
//out vec3 vLightCol;
//
//
//void main()
//{
//	vTexCoord = aTexCoord;
//	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
//	vNormal =	vec3(uWorldMatrix * vec4(aNormal, 0.0));
//	vViewDir = uCameraPosition - vPosition;
//	vLightCol = uLight[0].color;
//
//	if (uLight[0].type == 1)
//	{
//		vLightDir = normalize(uLight[0].direction);
//	}
//
//	if (uLight[0].type == 2)
//	{
//		vLightDir = normalize(uLight[0].position - vPosition);
//	}
//
//	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
//}
//
//#elif defined(FRAGMENT) ///////////////////////////////////////////////
//
//// TODO: Write your fragment shader here
//
//in vec2 vTexCoord;
//in vec3 vPosition;
//in vec3 vNormal;
//in vec3 vViewDir;
//in vec3 vLightDir;
//in vec3 vLightCol;
//
//uniform sampler2D uTexture;
//
//layout(location = 0) out vec4 oColor;
//
//void main()
//{
//	vec4 col = texture(uTexture, vTexCoord);
//
//
//	float lightEff = dot(vNormal, vLightDir);
//	vec3 col_ = mix(vec3(0), col.xyz, lightEff);
//	
//	oColor = vec4(col_, 1);
//}
//
//#endif
//#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

#define MAX_LIGHT_COUNT 8
#ifdef DEFERRED_RENDERING

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
	Light			uLight[MAX_LIGHT_COUNT];
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
flat out unsigned int vLightCount;
out vec3 vLightDir[MAX_LIGHT_COUNT];
out vec3 vLightCol[MAX_LIGHT_COUNT];


void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal =	vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	vLightCount = uLightCount;

	if (uLightCount > 0)
		for	(unsigned int i = 0; i < uLightCount; i++)
		{
			if (uLight[i].type == 1)
			{
				vLightDir[i] = normalize(uLight[i].direction);
			}

			if (uLight[i].type == 2)
			{
				vLightDir[i] = normalize(uLight[i].position - vPosition);
			}

			vLightCol[i] = uLight[i].color;
		}

	

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;
flat in unsigned int vLightCount;
in vec3 vLightDir[MAX_LIGHT_COUNT];
in vec3 vLightCol[MAX_LIGHT_COUNT];

uniform sampler2D uTexture;

layout(location = 0) out vec4 oPosition;
layout(location = 1) out vec4 oColor;
layout(location = 2) out vec4 oNormal;

void main()
{
	vec4 col = texture(uTexture, vTexCoord);
	vec3 totalColor = vec3(0);

	if (vLightCount > 0)
		for	(unsigned int i = 0; i < vLightCount; i++)
		{
			float lightEff = dot(vNormal, vLightDir[i]);
			totalColor += lightEff * vec3(col) * vLightCol[i];//mix(vec3(0), col.xyz, lightEff);
		}
	
	oPosition = vec4(vPosition, 1);
	oColor = vec4(totalColor, 1);
	oNormal = vec4(normalize(vNormal), 1);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#define MAX_LIGHT_COUNT 8
#ifdef FORWARD_RENDERING

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
	Light			uLight[MAX_LIGHT_COUNT];
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
flat out unsigned int vLightCount;
out vec3 vLightDir[MAX_LIGHT_COUNT];
out vec3 vLightCol[MAX_LIGHT_COUNT];


void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal =	vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	vLightCount = uLightCount;

	if (uLightCount > 0)
		for	(unsigned int i = 0; i < uLightCount; i++)
		{
			if (uLight[i].type == 1)
			{
				vLightDir[i] = normalize(uLight[i].direction);
			}

			if (uLight[i].type == 2)
			{
				vLightDir[i] = normalize(uLight[i].position - vPosition);
			}

			vLightCol[i] = uLight[i].color;
		}

	

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;
flat in unsigned int vLightCount;
in vec3 vLightDir[MAX_LIGHT_COUNT];
in vec3 vLightCol[MAX_LIGHT_COUNT];

uniform sampler2D uTexture;

layout(location = 0) out vec4 oPosition;

void main()
{
	vec4 col = texture(uTexture, vTexCoord);
	vec3 totalColor = vec3(0);

	if (vLightCount > 0)
		for	(unsigned int i = 0; i < vLightCount; i++)
		{
			float lightEff = dot(vNormal, vLightDir[i]);
			totalColor += lightEff * vec3(col) * vLightCol[i];//mix(vec3(0), col.xyz, lightEff);
		}
	
	oColor = vec4(totalColor, 1);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



#ifdef SCREEN_RECT

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here


layout(location = 0) in vec3 aPosition;	// world space
layout(location = 1) in vec3 aNormal;	// world space
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec2 vTexCoord;


void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition.x, aPosition.y, 0.0 , 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

out vec4 oColor;
in vec2 vTexCoord;

uniform float a;
uniform sampler2D screenTexture;


void main()
{
	oColor = texture(screenTexture, vTexCoord);
//	float average = (oColor.r + oColor.g + oColor.b) /3.0;
//	oColor = vec4(average,average,average,1.0);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////