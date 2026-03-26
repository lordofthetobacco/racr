#include "shader.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { SDL_Log("Cannot open shader file: %s", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static GLuint compile(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        SDL_Log("Shader compile error:\n%s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint shader_create(const char *vert_path, const char *frag_path) {
    char *vsrc = read_file(vert_path);
    char *fsrc = read_file(frag_path);
    if (!vsrc || !fsrc) { free(vsrc); free(fsrc); return 0; }

    GLuint vs = compile(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc);
    free(vsrc);
    free(fsrc);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        SDL_Log("Shader link error:\n%s", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

void shader_use(GLuint program) {
    glUseProgram(program);
}

void shader_set_mat4(GLuint program, const char *name, const mat4 *m) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m->m);
}

void shader_set_vec3(GLuint program, const char *name, vec3 v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z);
}

void shader_set_vec4(GLuint program, const char *name, vec4 v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform4f(loc, v.x, v.y, v.z, v.w);
}

void shader_set_float(GLuint program, const char *name, float v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform1f(loc, v);
}

void shader_set_int(GLuint program, const char *name, int v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) glUniform1i(loc, v);
}

void shader_destroy(GLuint program) {
    if (program) glDeleteProgram(program);
}
