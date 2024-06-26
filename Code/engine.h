//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>
#include "BufferManagement.h"


#define BINDING(b) b

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;
typedef glm::mat4x4 mat4x4;
typedef glm::quat quat;

const mat4x4 IdentityMatrix = mat4x4(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    Image       image;
    GLuint      handle;
    std::string filepath;
};



struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexShaderLayout;
    
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMeshes,
    Mode_Count
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};




struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32> indices;
    u32 vertexOffset;
    u32 indexOffset;

    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;

    
};

enum LightType
{
    DIRECTIONAL_LIGHT = 1,
    POINT_LIGHT = 2,
};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
};


struct LightObject
{
    std::string name;
    u32 Idx;
    Light light;
    Mesh mesh;

    //mat4x4 worldMatrix;
    //mat4x4 worldViewProjectionMatrix;
};

struct SceneObject
{
    std::string name;
    Mesh mesh;
    u32 modelIdx;
    mat4x4 worldMatrix;
    mat4x4 worldViewProjectionMatrix;

    vec3 rotationEuler;
    quat rotationQuat;

    u32 localParamsOffset;
    u32 localParamsSize;
};
struct Material
{
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct App;

struct Camera
{

    float speed;
    float sensitivity;
    vec3 X, Y, Z, Position, currentReference;
    mat4x4 ViewMatrix, ViewMatrixInverse;
    float zNear;
    float zFar;
    float fov;

    void UpdateCamera(App* app);
    void CalculateViewMatrix();
    void SetValues();
};


struct DeferredTexture
{
    std::string name;
    GLuint idx;
};
struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>        textures;
    std::vector<Program>        programs;
    std::vector<Model>          models;
    std::vector<Material>       materials;
    std::vector<SceneObject>    sceneObjects;
    std::vector<LightObject>    lightObjects;

    //premade models
    Mesh m_quad;


    // program indices
    u32 forwardRenderingProgramIdx;
    u32 deferredRenderingProgramIdx;
    u32 screenRectProgramIdx;

    bool rendering_deferred = false;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture = 1004;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint screenQuadVao;
    Mesh quadMesh;

    Camera camera;

    int maxUniformBufferSize;
    int uniformBlockAlignment;
    Buffer uniformBuffer;
    u32 globalParamsOffset;
    u32 globalParamsSize;


    GLuint combinedAttachmentHandle;
    GLuint positionAttachmentHandle;
    GLuint colorAttachmentHandle;
    GLuint normalAttachmentHandle;
    GLuint depthAttachmentHandle;
    GLuint depthReadHandle;
    GLuint framebufferHandle;

    std::vector<DeferredTexture> deferredTextures;
    int currentBuffer = 0;


};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

u32 CreateLight(App* app, LightType type, vec3 position, vec3 direction, vec3 color);
vec3 rotate(const vec3& vector, float degrees, const vec3& axis);
void ManageSceneObjectRotation(SceneObject& scObj);

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

class ErrorGuardOGL {
public:
    ErrorGuardOGL(const char* message, const char* file, int line) : msg(message), file(file), line(line) {
        CheckGLError("BEGIN");
    }

    ~ErrorGuardOGL() {
        CheckGLError("END");
    }

    void CheckGLError(const char* around);

    const char* msg;
    const char* file;
    int line;
};

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam);

u32 LoadTexture2D(App* app, const char* filepath);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

mat4x4 SetPosition(const vec3& translation);

void SetScaling(mat4x4& M, float x, float y, float z);