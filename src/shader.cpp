#include <fstream>
#include <sstream>
#include <iostream>
#include "shader.h"
#include "scene.h"

void checkShaderCompilation(unsigned int shader, std::string path) {
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR in: " << path << "\n"
                  << infoLog
                  << "\n -- --------------------------------------------------- -- "
                  << std::endl;
    }
}

void checkProgramLink(unsigned int program, std::string vertexPath, std::string fragmentPath) {
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_PROGRAM_LINK_ERROR between: " << vertexPath << " <<>> " << fragmentPath << "\n"
                  << infoLog
                  << "\n -- --------------------------------------------------- -- "
                  << std::endl;
    }
}

unsigned int loadShader(std::string vertexFile, std::string fragmentFile) {
    std::ifstream vertexFileStream;
    std::ifstream fragmentFileStream;

    std::string vertexString;
    std::string fragmentString;

    std::string vertexPath = shaderPath + vertexFile;
    std::string fragmentPath = shaderPath + fragmentFile;

    const char *vertexCode;
    const char *fragmentCode;

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgram;

    vertexFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vertexFileStream.open(vertexPath);
        fragmentFileStream.open(fragmentPath);

        vertexString = std::string(std::istreambuf_iterator<char>(vertexFileStream), std::istreambuf_iterator<char>());
        fragmentString = std::string(std::istreambuf_iterator<char>(fragmentFileStream), std::istreambuf_iterator<char>());

        vertexFileStream.close();
        fragmentFileStream.close();
    } catch (std::ifstream::failure e) {
        std::cerr << "ERROR::SHADER::COULD_NOT_OPEN_OR_READ_FILE: " << e.what() << std::endl;
    }

    vertexCode = vertexString.c_str();
    fragmentCode = fragmentString.c_str();

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertexShader, 1, &vertexCode, NULL);
    glShaderSource(fragmentShader, 1, &fragmentCode, NULL);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    checkShaderCompilation(vertexShader, vertexPath);
    checkShaderCompilation(fragmentShader, fragmentPath);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    checkProgramLink(shaderProgram, vertexPath, fragmentPath);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void PrintActiveUniforms(GLuint program) {
    GLint numUniforms = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);

    std::cout << "Active Uniforms in Program " << program << ":\n";

    char nameBuffer[256];
    for (GLint i = 0; i < numUniforms; ++i) {
        GLsizei length = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveUniform(program, i, sizeof(nameBuffer), &length, &size, &type, nameBuffer);
        GLint location = glGetUniformLocation(program, nameBuffer);

        std::cout << "  #" << i << ": " << nameBuffer
                  << " | Type: 0x" << std::hex << type
                  << " | Size: " << std::dec << size
                  << " | Location: " << location << '\n';
    }
}

void loadEditorShaders(RenderState *renderer) {
    renderer->pickingShader = loadShader("pickingshader.vs", "pickingshader.fs");
    renderer->debugShader = loadShader("debugShader.vs", "debugShader.fs");
}

void loadShaders(RenderState *scene) {
    scene->depthShader = loadShader("depthprepassshader.vs", "depthprepassshader.fs");
    scene->lightingShader = loadShader("pbrlitshader.vs", "pbrlitshader.fs");
    scene->ssaoShader = loadShader("SSAOshader.vs", "SSAOshader.fs");
    scene->shadowBlurShader = loadShader("SSAOshader.vs", "shadowmapblurshader.fs");
    scene->simpleBlurShader = loadShader("SSAOshader.vs", "SSAOblurshader.fs");
    scene->blurShader = loadShader("gaussianblurshader.vs", "gaussianblurshader.fs");
    scene->postProcessShader = loadShader("postprocessshader.vs", "postprocessshader.fs");
}