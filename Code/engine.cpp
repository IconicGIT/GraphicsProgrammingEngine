//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include "assimpModelLoading.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);


    //put attributes automatically
    int attCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attCount);

    std::vector<GLchar> nameData(256); // Buffer to store attribute names
    std::cout << program.programName << " Attributes:   ------------------------------------------------------" << std::endl;

    for (GLint i = 0; i < attCount; ++i) {
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveAttrib(program.handle, i, static_cast<GLsizei>(nameData.size()), &length, &size, &type, nameData.data());

        // Get attribute name
        std::string attributeName(nameData.data(), length);

        // Get attribute location
        u8 location = glGetAttribLocation(program.handle, attributeName.c_str());

        u8 attribSize = 0;
        switch (type) {
        case GL_FLOAT:
            attribSize = 1;
            break;
        case GL_FLOAT_VEC2:
            attribSize = 2;

            break;
        case GL_FLOAT_VEC3:
            attribSize = 3;

            break;
        case GL_FLOAT_VEC4:
            attribSize = 4;

            break;
            // Add cases for other types as needed
        default:
            attribSize = 0;

            break;
        }

        program.vertexShaderLayout.attributes.push_back({ location, attribSize });

        // Store or process the attribute information as needed
        std::cout << "Attribute " << i << ": Name = " << attributeName << ", Size = " << size << ", Type = " << type << ", Location = " << (int)location << std::endl;
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //error on glEnum, idk why
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;


    Image image = LoadImage(filepath);
    

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;
        tex.image = image;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);

        std::cout << "Loaded " + std::string(filepath) << std::endl;
        return texIdx;
    }
    else
    {
        std::cout << "Failed loading " + std::string(filepath) << std::endl;
        return UINT32_MAX;
    }
}



void ErrorGuardOGL::CheckGLError(const char* around) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << file << ":" << line << " (" << around << ") - ";
        switch (error) {
        case GL_INVALID_ENUM:
            std::cerr << "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument." << std::endl;
            break;
        case GL_INVALID_VALUE:
            std::cerr << "GL_INVALID_VALUE: A numeric argument is out of range." << std::endl;
            break;
        case GL_INVALID_OPERATION:
            std::cerr << "GL_INVALID_OPERATION: The specified operation is not allowed in the current state." << std::endl;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete." << std::endl;
            break;
        case GL_OUT_OF_MEMORY:
            std::cerr << "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command." << std::endl;
            break;
        default:
            std::cerr << "Unknown error: " << error << std::endl;
            break;
        }
        if (msg)
            std::cerr << "Message: " << msg << std::endl;
    }
}


void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;
    ELOG("OpenGL debug message: %s", message);
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        ELOG(" - source: GL_DEBUG_SOURCE_API"); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break;
    case GL_DEBUG_SOURCE_APPLICATION:
        ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break;
    case GL_DEBUG_SOURCE_OTHER:
        ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break;
    }
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;
    case GL_DEBUG_TYPE_PORTABILITY:
        ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break;
    case GL_DEBUG_TYPE_MARKER:
        ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break;
    case GL_DEBUG_TYPE_POP_GROUP:
        ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break;
    case GL_DEBUG_TYPE_OTHER:
        ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break;
    }
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break;
    case GL_DEBUG_SEVERITY_LOW:
        ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        ELOG(" - severity: GL_DEBUG_SEVERITY_NOTIFICATION"); break;
    }
}


GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    
    Submesh& submesh = mesh.submeshes[submeshIndex];

    //try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    GLuint vaoHandle = 0;

    // Create a new vao for this submesh/program
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    // We have to link all vertex inputs attributes to attributes in the vertex buffer
    for (u32 i = 0; i < program.vertexShaderLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;
        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            
            if (program.vertexShaderLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                //std::cout << (int)program.vertexShaderLayout.attributes[i].location << " - " << (int)submesh.vertexBufferLayout.attributes[j].location << std::endl;
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
                const u32 stride = submesh.vertexBufferLayout.stride;

                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);

                ErrorGuardOGL error("FindVAO()", __FILE__, __LINE__);

                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }
        assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
    }
    glBindVertexArray(0);

    //store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

//mat4x4 SetPosition(const vec3& translation)
//{
//    return translate(translation);
//}
//
//mat4x4 SetScale(const vec3& scaling)
//{
//    return glm::scale(scaling);
//}

u32 CreateLight(App* app, LightType type, vec3 position, vec3 direction, vec3 color)
{
    app->lightObjects.push_back(LightObject{});

    LightObject &lo = app->lightObjects.back();

    u32 lIdx = (u32)app->lightObjects.size() - 1u;

    lo.name = "Light " + std::to_string(lIdx);

    Light &l = lo.light;
    l.position = position;
    
    l.type = type;
    l.direction = direction;
    l.color = color;

    lo.Idx;

    return lo.Idx;
}



vec3 rotate(const vec3& vector, float degrees, const vec3& axis)
{
    // Convert degrees to radians
    float radians = glm::radians(degrees);

    // Create a rotation matrix
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), radians, axis);

    // Transform the vector using the rotation matrix
    glm::vec4 rotatedVector = rotationMatrix * glm::vec4(vector, 1.0f);

    // Return the rotated vector
    return glm::vec3(rotatedVector);
}

vec3 GetTranslation(const mat4x4 &M)
{
    //return(vec3(M[12], M[13], M[14]));
    return(vec3(M[3].x, M[3].y, M[3].z));
}

void SetTranslation(mat4x4& M, float x, float y, float z)
{
    M[3].x = x;
    M[3].y = y;
    M[3].z = z;
    //return(vec3(M[12], M[13], M[14]));
}

void SetTranslation(mat4x4& M, vec3 position)
{
    M[3].x = position.x;
    M[3].y = position.y;
    M[3].z = position.z;
    //return(vec3(M[12], M[13], M[14]));
}

vec3 GetScaling(mat4x4& M)
{
    //return (vec3(M[0], M[5], M[10]));
    return (vec3(M[0].x, M[1].y, M[2].z));
}

void SetScaling(mat4x4& M, float x, float y, float z)
{
    M[0].x = x;
    M[1].y = y;
    M[2].z = z;
}

void Camera::CalculateViewMatrix()
{
    ViewMatrix = mat4x4(X.x, Y.x, Z.x, 0.0f, X.y, Y.y, Z.y, 0.0f, X.z, Y.z, Z.z, 0.0f, -dot(X, Position), -dot(Y, Position), -dot(Z, Position), 1.0f);
    ViewMatrixInverse = inverse(ViewMatrix);
}

void Camera::UpdateCamera(App* app)
{
    vec3 newPos(0, 0, 0);
    float spd = speed * app->deltaTime;

    // keyboard movement


    if (app->input.keys[Key::K_Q])
    {
        newPos.y -= spd;
    }

    if (app->input.keys[Key::K_E])
    {
        newPos.y += spd;
    }

    if (app->input.keys[Key::K_W])
    {
        newPos -= Z * spd;
    }

    if (app->input.keys[Key::K_S])
    {
        newPos += Z * spd;
    }

    if (app->input.keys[Key::K_A])
    {
        newPos -= X * spd;
    }

    if (app->input.keys[Key::K_D])
    {
        newPos += X * spd;
    }

    Position += newPos;
    currentReference += newPos;

    


    //if (app->input.keys[Key::K_SPACE])
    //{
    //    std::cout << "Position: " << Position.x << " " << Position.y << " " << Position.z << std::endl;
    //}


    //mouse movement

    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && app->input.mouseButtons[MouseButton::RIGHT])
    {
        float dx = app->input.mouseDelta.x;
        float dy = app->input.mouseDelta.y;
        //std::cout << "[R] Mouse Position: " << dx << " " << dy << " " << std::endl;

        Position -= currentReference;

        if (dx != 0)
        {
            float DeltaX = (float)dx * sensitivity;

            X = rotate(X, DeltaX, vec3(0.0f, 1.0f, 0.0f));
            Y = rotate(Y, DeltaX, vec3(0.0f, 1.0f, 0.0f));
            Z = rotate(Z, DeltaX, vec3(0.0f, 1.0f, 0.0f));
        }

        if (dy != 0)
        {
            float DeltaY = (float)dy * sensitivity;

            Y = rotate(Y, DeltaY, X);
            Z = rotate(Z, DeltaY, X);

            if (Y.y < 0.0f)
            {
                Z = vec3(0.0f, Z.y > 0.0f ? 1.0f : -1.0f, 0.0f);
                Y = cross(Z, X);
            }

        }
        Position = currentReference + Z * length(Position);

    }


    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && app->input.mouseButtons[MouseButton::LEFT])
    {
        float dx = app->input.mouseDelta.x;
        float dy = app->input.mouseDelta.y;
        //std::cout << "[L] Mouse Position: " << dx << " " << dy << " " << std::endl;

        if (dx != 0)
        {
            float DeltaX = (float)dx * sensitivity * 0.5f;

            X = rotate(X, DeltaX, vec3(0.0f, 1.0f, 0.0f));
            Y = rotate(Y, DeltaX, vec3(0.0f, 1.0f, 0.0f));
            Z = rotate(Z, DeltaX, vec3(0.0f, 1.0f, 0.0f));
        }

        if (dy != 0)
        {
            float DeltaY = (float)dy * sensitivity * 0.5f;

            Y = rotate(Y, DeltaY, X);
            Z = rotate(Z, DeltaY, X);

            if (Y.y < 0.0f)
            {
                Z = vec3(0.0f, Z.y > 0.0f ? 1.0f : -1.0f, 0.0f);
                Y = cross(Z, X);
            }
        }
        currentReference = Position - Z * length(Position);
    }

    CalculateViewMatrix();


}

void Init(App* app)
{
    ErrorGuardOGL error("Init()", __FILE__, __LINE__);


    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
        glDebugMessageCallback(OnGlError, app);


    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);


    // set camera variables

    app->camera.SetValues();


   
    // - programs (and retrieve uniform indices)
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY2");
    app->screenRectProgramIdx = LoadProgram(app, "shaders.glsl", "SCREEN_RECT");
    
    {


        // TODO: Initialize your resources here!
        // - vertex buffers
        // - element/index buffers
        // - vaos
        // - programs (and retrieve uniform indices)
        // - textures




        float vertices[] = 
        {
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
        };


        // - define vertex buffers
        //const VertexV3V2 vertices[] =
        //{
        //    {glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0,0)}, // bottom left
        //    {glm::vec3( 0.5, -0.5, 0.0), glm::vec2(1,0)}, // bottom right
        //    {glm::vec3( 0.5,  0.5, 0.0), glm::vec2(1,1)}, // top right
        //    {glm::vec3(-0.5,  0.5, 0.0), glm::vec2(0,1)}, // top left
        //};

        u32 indices[] = { 0,1,2, 0,2,3 };


        //const u16 indices[] =
        //{
        //    0, 1, 2,
        //    0, 2, 3
        //};

        // - init vertex buffers

        //format and order of the vertex in vertex shader
        VertexBufferLayout vertexBufferLayout;
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 }); // 0: location, 3: components (3 floats per position), 0: offset
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 2, 3 * sizeof(float) }); // 1: location, 3: components (2 floats per texture coord), 0: offset (from start to this location, 3 floats come before this)
        vertexBufferLayout.stride = 5 * sizeof(float); //3 for position + 2 for tex coords

       

        

        LoadModel(app, "Models/ScreenQuad/screenQuad.obj", vec3(0, 0, 0));

        //Mesh quadMesh = app->sceneObjects[0].mesh;
        //Submesh quadSubMesh = quadMesh.submeshes[0];

        //// Generate and bind VBO for vertices
        //glGenBuffers(1, &app->embeddedVertices);
        //glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) , vertices, GL_STATIC_DRAW);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        //// Generate and bind EBO for indices
        //glGenBuffers(1, &app->embeddedElements);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadSubMesh.indices), &quadSubMesh.indices, GL_STATIC_DRAW);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Generate and bind VAO
        //glGenVertexArrays(1, &app->screenQuadVao);
        //glBindVertexArray(app->screenQuadVao);

        //// Bind VBO and set attribute pointers
        //glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
        //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
        //glEnableVertexAttribArray(0);
        //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
        //glEnableVertexAttribArray(1);

        //// Bind EBO
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);

        //// Unbind VAO
        //glBindVertexArray(0);

        //app->screenQuadVao = FindVAO(quadMesh, 0, app->programs[app->screenRectProgramIdx]);

        ////build submesh
        //Submesh submesh = {};
        //submesh.vertexBufferLayout = vertexBufferLayout;
        //submesh.vertices.swap(vertices);
        //submesh.indices.swap(indices);

        //Mesh m_quad;
        //m_quad.submeshes.push_back(submesh);

        //m_quad.vertexBufferHandle = app->embeddedVertices;
        //m_quad.indexBufferHandle = app->embeddedElements;

        //Vao vao;
        //vao.handle = app->vao;
        //vao.programHandle = app->programs[app->screenRectProgramIdx].handle;

        //m_quad.submeshes[0].vaos.push_back(vao); 

        //app->m_quad = m_quad;
    }


    




    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];


    //print attribute blocks
    //int activeShader = app->programs[0].handle;
    //int blockIndex = glGetUniformBlockIndex(activeShader, "localParams");
    //int blockSize;
    //
    //glGetActiveUniformBlockiv(activeShader, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
    //
    //std::cout << "block index: " << blockIndex << " of size: " << blockSize << std::endl;

    

    app->programUniformTexture = glGetUniformLocation(app->programs[app->screenRectProgramIdx].handle, "screenTexture");
 

    


    // - textures
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
    app->diceTexIdx = LoadTexture2D(app, "dice.png");


    app->mode = Mode_TexturedMeshes;

    glEnable(GL_DEPTH_TEST);


    //set uniform buffers
    //glGenBuffers(1, &app->uniformBufferHandle);
    //glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBufferHandle);
    //glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    app->uniformBuffer = CreateUniformBuffer(app->maxUniformBufferSize);


    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // load models
    
    
    LoadModel(app, "Models/Patrick/Patrick.obj", vec3(0, 0, 3));
    LoadModel(app, "Models/Patrick/Patrick.obj", vec3(0, 0, -3));

    //load lights

    CreateLight(app, DIRECTIONAL_LIGHT, vec3(0, 2, 0), vec3(1), vec3(1));
    CreateLight(app, POINT_LIGHT, vec3(0, 2, -3), vec3(1), vec3(1));
    CreateLight(app, POINT_LIGHT, vec3(0, 2, 3), vec3(1), vec3(1));

    
    //set screen rect

    

    // framebuffers
    app->colorAttachmentHandle;
    glGenTextures(1, &app->colorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    //app->depthAttachmentHandle;
    //glGenTextures(1, &app->depthAttachmentHandle);
    //glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glBindTexture(GL_TEXTURE_2D, 0);

    //attach frame buffers 
    app->framebufferHandle;
    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->colorAttachmentHandle, 0);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->depthAttachmentHandle, 0);


    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Error " << framebufferStatus << ": ";
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                              ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:                  ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:          ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:                 ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:                 ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                            ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:                 ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:               ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
            break;
        }
    }

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);
    ErrorGuardOGL errorFB("Framebuffers", __FILE__, __LINE__);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

float imageSize = 30;

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    
    ImGui::Text("Loaded Textures");

    if (ImGui::TreeNode("Textures"))
    {
        ImGui::DragFloat("Image Size", &imageSize, 0.1, 10, 100, "%.2f");
        if (app->textures.size() > 0)
            for (size_t i = 0; i < app->textures.size(); i++)
            {
                ImGui::Separator();
                Texture& t = app->textures[i];

                ImVec2 size = ImVec2(imageSize / t.image.size.x, imageSize / t.image.size.x);

                ImGui::Image((void*)t.handle, ImVec2(t.image.size.x * size.x, t.image.size.y * size.y));
                ImGui::SameLine();
                ImGui::Text(t.filepath.c_str());
            }
        ImGui::TreePop();
    }
    ImGui::Separator();

    ImGui::Text("Scene lights");
    ImGui::Separator();

    if (app->lightObjects.size() > 0)
        for (size_t i = 0; i < app->lightObjects.size(); i++)
        {
            LightObject& scObj = app->lightObjects[i];

            std::string lType = "";
            if (scObj.light.type == 1)
                lType = "(Directional)";
            if (scObj.light.type == 2)
                lType = "(Point)";

            std::string scObjName = scObj.name + " " + lType + "##" + std::to_string(i);

            if (ImGui::TreeNode(scObjName.c_str()))
            {
                if (scObj.light.type == DIRECTIONAL_LIGHT)
                    ImGui::DragFloat3("Direction", &scObj.light.direction[0], 0.05f, 0.0f, 0.0f, "%.2f");

                if (scObj.light.type == POINT_LIGHT)
                    ImGui::DragFloat3("Position", &scObj.light.position[0], 0.05f, 0.0f, 0.0f, "%.2f");

                ImGui::ColorEdit3("Color", &scObj.light.color[0]);

                std::cout << scObj.light.color[0]  << " " << scObj.light.color[1] << " " << scObj.light.color[2] << std::endl;

                ImGui::TreePop();
            }
        }
    ImGui::Separator();

    ImGui::Text("Scene Objects");
    ImGui::Separator();
    if (app->sceneObjects.size() > 0)
    for (size_t i = 0; i < app->sceneObjects.size(); i++)
    {
        SceneObject& scObj = app->sceneObjects[i];
        std::string scObjName = scObj.name + "##" + std::to_string(i);
        if (ImGui::TreeNode(scObjName.c_str()))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                ImGui::DragFloat3("Translation", &scObj.worldMatrix[3][0], 0.05f, 0.0f, 0.0f, "%.2f");

                vec3 newScale = GetScaling(scObj.worldMatrix);
                if (ImGui::DragFloat3("Scaling", &newScale[0], 0.05f, 0.0f, 0.0f, "%.2f"))
                {
                    SetScaling(scObj.worldMatrix, newScale.x, newScale.y, newScale.z);
                }

                ManageSceneObjectRotation(scObj);

                //float newPositionY = scObj.worldMatrix.translation().y;
                //float newPositionZ = scObj.worldMatrix.translation().z;
            }

            ImGui::TreePop();
        }
    }
    ImGui::Separator();


    //ImGui::DragFloat3("CamPos", &app->camera.Position[0], 0.1f, -1000, 1000, "%f", 0);
    //ImGui::DragFloat3("CamRef", &app->camera.currentReference[0], 0.1f, -1000, 1000, "%f", 0);
    //
    //
    //ImGui::DragFloat3("X:", &app->camera.X[0], 0.1f, 0, 1000, "%f", 0);
    //ImGui::DragFloat3("Y:", &app->camera.Y[0], 0.1f, 0, 1000, "%f", 0);
    //ImGui::DragFloat3("Z:", &app->camera.Z[0], 0.1f, 0, 1000, "%f", 0);

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    
    app->camera.UpdateCamera(app);
    //std::cout << 1.f / app->deltaTime << " / " << 1.f / app->deltaTime * 10.f << " / " << 1.f / app->deltaTime * 100.f << " / " << 1.f / app->deltaTime * 1000.f<< std::endl;

    for (u64 i = 0; i < app->programs.size(); i++)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }
    

    MapBuffer(app->uniformBuffer, GL_WRITE_ONLY);

    // update global params
    app->globalParamsOffset = app->uniformBuffer.head;

    //push camera position
    PushVec3(app->uniformBuffer, app->camera.Position);

    //push light count
    PushUInt(app->uniformBuffer, app->lightObjects.size());

    if (app->lightObjects.size() > 0);
    for (size_t i = 0; i < app->lightObjects.size(); i++)
    {
        AlignHead(app->uniformBuffer, sizeof(vec4));

        Light& light = app->lightObjects[i].light;

        PushUInt(app->uniformBuffer, light.type);
        PushVec3(app->uniformBuffer, light.color);
        PushVec3(app->uniformBuffer, light.direction);
        PushVec3(app->uniformBuffer, light.position);
    }

    app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset;


    // update local params
    if (app->sceneObjects.size() > 0)
        for (size_t i = 0; i < app->sceneObjects.size(); i++)
        {
            SceneObject& sceneObject = app->sceneObjects[i];
            // update transform
                float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
                mat4x4 projectionMatrix = glm::perspective(glm::radians(app->camera.fov), aspectRatio, app->camera.zNear, app->camera.zFar);
                mat4x4 view = glm::lookAt(app->camera.Position, app->camera.currentReference, vec3(0, 1, 0));
            
                //update transform and view matrices
                //sceneObject.worldMatrix = IdentityMatrix; --> only when setting the mesh/object;
                mat4x4 worldMat = sceneObject.worldMatrix;
                mat4x4 viewMat = sceneObject.worldViewProjectionMatrix = projectionMatrix * view * sceneObject.worldMatrix;

                AlignHead(app->uniformBuffer, app->uniformBlockAlignment);

                sceneObject.localParamsOffset = app->uniformBuffer.head;
                PushMat4(app->uniformBuffer, worldMat);
                PushMat4(app->uniformBuffer, viewMat);
                sceneObject.localParamsSize = app->uniformBuffer.head - sceneObject.localParamsOffset;
        }

    UnmapBuffer(app->uniformBuffer);

    //u32 bufferHead = app->globalParamsSize;
    //
    //for (size_t i = 0; i < app->sceneObjects.size(); i++)
    //{
    //    SceneObject &sceneObject = app->sceneObjects[i];
    //
    //    //update transform
    //    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    //    mat4x4 projectionMatrix = glm::perspective(glm::radians(app->camera.fov), aspectRatio, app->camera.zNear, app->camera.zFar);
    //    mat4x4 view = glm::lookAt(app->camera.Position, app->camera.currentReference, vec3(0, 1, 0));
    //
    //    //update transform and view matrices
    //    //sceneObject.worldMatrix = IdentityMatrix; --> only when setting the mesh/object;
    //    sceneObject.worldViewProjectionMatrix = projectionMatrix * view * sceneObject.worldMatrix;
    //
    //    //opengl stuff
    //    glBindBuffer(GL_UNIFORM_BUFFER, app->uniformBuffer.handle);
    //    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    //    bufferHead = Align(bufferHead, app->uniformBlockAlignment);
    //
    //    sceneObject.localParamsOffset = bufferHead;
    //
    //    memcpy(bufferData + bufferHead, glm::value_ptr(sceneObject.worldMatrix), sizeof(mat4x4));
    //    bufferHead += sizeof(mat4x4);
    //
    //    memcpy(bufferData + bufferHead, glm::value_ptr(sceneObject.worldViewProjectionMatrix), sizeof(mat4x4));
    //    bufferHead += sizeof(mat4x4);
    //
    //    sceneObject.localParamsSize = bufferHead - sceneObject.localParamsOffset;
    //
    //
    //    glUnmapBuffer(GL_UNIFORM_BUFFER);
    //    
    //
    //}
    //glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                ErrorGuardOGL error("Render() [Mode_TexturedQuad]", __FILE__, __LINE__);

                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!

                
                // - clear the framebuffer
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // - set the viewport
                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                // - set the blending state
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // - bind the texture into unit 0
                glActiveTexture(GL_TEXTURE0);
                GLuint textureHandle = app->textures[app->diceTexIdx].handle;
                glBindTexture(GL_TEXTURE_2D, textureHandle);

                // - bind the program 
                Program& currentProgram = app->programs[app->screenRectProgramIdx];
                glUseProgram(currentProgram.handle);
                

                //   (...and make its texture sample from unit 0)
                glUniform1i(app->programUniformTexture, 0);

                // - bind the vao
                Mesh quadMesh = app->sceneObjects[0].mesh;
                Submesh quadSubMesh = quadMesh.submeshes[0];
                app->screenQuadVao = FindVAO(quadMesh, 0, currentProgram);
                glBindVertexArray(app->screenQuadVao);

                //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
                //Submesh& submesh = app->sceneObjects[0].mesh.submeshes[0];
                //glDrawElements(GL_TRIANGLES, quadSubMesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)quadSubMesh.indexOffset);

                glBindVertexArray(0);
                glUseProgram(0);
            }
            break;

        case Mode_TexturedMeshes:
        {
            //ErrorGuardOGL error("Render() [Mode_TexturedMeshes]", __FILE__, __LINE__);

            //bind frameBuffer object
            glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);


            // - clear the framebuffer
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // - set the viewport
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            // - set the blending state
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            //use light icons shader
            

            //if (app->lightObjects.size() > 0)
            //    for (size_t i = 0; i < app->lightObjects.size(); i++)
            //    {
            //        ErrorGuardOGL error("aaaa", __FILE__, __LINE__);
            //        std::string groupName = "Light mesh" + std::to_string(app->lightObjects[i].Idx);
            //        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, groupName.c_str());
            //
            //        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);
            //
            //        glPopDebugGroup();
            //
            //    }
            
            

            ////use mesh textured shader
            Program currentProgram = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(currentProgram.handle);
            glEnable(GL_DEPTH_TEST);


            
            /*GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);*/



            //ErrorGuardOGL error("FBff", __FILE__, __LINE__);


            

            //load global uniforms
            if (app->lightObjects.size() > 0)
            for (size_t i = 0; i < app->lightObjects.size(); i++)
            {
                LightObject& lo = app->lightObjects[i];
                std::string groupName = "Light" + std::to_string(lo.Idx);
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, groupName.c_str());

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

                glPopDebugGroup();

            }

            //draw meshes
            if (app->sceneObjects.size() > 0)
            for (size_t m = 1; m < app->sceneObjects.size(); m++)
            {
                SceneObject& scObj = app->sceneObjects[m];

                Model& model = app->models[scObj.modelIdx]; //app->model does not exist
                Mesh& mesh = scObj.mesh;

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    std::string groupName = "Object" + std::to_string(model.meshIdx);
                    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, groupName.c_str());

                    //use uniform buffer
                    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, scObj.localParamsOffset, scObj.localParamsSize);

                    GLuint vao = FindVAO(mesh, i, currentProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialldx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialldx];

                    Texture* tex = &app->textures[submeshMaterial.albedoTextureIdx];


                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tex->handle);
                    glUniform1i(app->programUniformTexture, 0);
                    //std::cout << "Using texture: " << std::string(tex->filepath) << std::endl;

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    glPopDebugGroup();
                }
                //std::cout << "Next Render call --------------------------------------------------------------------" << std::endl;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            ////draw screen rect

            glDisable(GL_DEPTH_TEST);


            // - bind the texture into unit 0
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            // - bind the program 
            currentProgram = app->programs[app->screenRectProgramIdx];
            glUseProgram(currentProgram.handle);


            //   (...and make its texture sample from unit 0)
            glUniform1i(app->programUniformTexture, 0);

            // - bind the vao
            Mesh quadMesh = app->sceneObjects[0].mesh;
            Submesh quadSubMesh = quadMesh.submeshes[0];
            app->screenQuadVao = FindVAO(quadMesh, 0, currentProgram);
            glBindVertexArray(app->screenQuadVao);

            //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
            //Submesh& submesh = app->sceneObjects[0].mesh.submeshes[0];
            glDrawElements(GL_TRIANGLES, quadSubMesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)quadSubMesh.indexOffset);

        }
            break;

        default:;
    }
}



void Camera::SetValues()
{

    X = vec3(1.0f, 0.0f, 0.0f);
    Y = vec3(0.0f, 1.0f, 0.0f);
    Z = vec3(0.0f, 0.0f, 1.0f);

    sensitivity = 0.5f;
    Position = vec3(5, 5.0, 5.0f);
    currentReference = vec3(0.0f, 0.0f, 0.0f);
    speed = 10.f;
    zNear = 0.1f;
    zFar = 1000.f;
    fov = 60.f;;

    CalculateViewMatrix();
}


void ManageSceneObjectRotation(SceneObject &scObj)
{
    vec3 newRot = vec3(scObj.rotationEuler[0], scObj.rotationEuler[1], scObj.rotationEuler[2]);

    if (ImGui::DragFloat3("Rotation", &newRot[0], 0.05f, 0.0f, 0.0f, "%.2f"))
    {
        vec3 toRot = scObj.rotationEuler - newRot;
        scObj.worldMatrix = glm::rotate(scObj.worldMatrix, glm::radians(toRot[0]), vec3(1, 0, 0));
        scObj.worldMatrix = glm::rotate(scObj.worldMatrix, glm::radians(toRot[1]), vec3(0, 1, 0));
        scObj.worldMatrix = glm::rotate(scObj.worldMatrix, glm::radians(toRot[2]), vec3(0, 1, 0));

        scObj.rotationEuler = newRot;
    }

    
}