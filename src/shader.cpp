#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

void checkShaderCompilation(unsigned int shader, const char *path);
void checkProgramLink(unsigned int program, const char *vertexPath, const char *fragmentPath);

unsigned int loadShader(const char *vertexPath, const char *fragmentPath) {
    std::ifstream vertexFileStream;
    std::ifstream fragmentFileStream;

    std::string vertexString;
    std::string fragmentString;

    const char *vertexCode;
    const char *fragmentCode;

    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int shaderProgram;

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

void checkShaderCompilation(unsigned int shader, const char *path) {
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

void checkProgramLink(unsigned int program, const char *vertexPath, const char *fragmentPath) {
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