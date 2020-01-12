// Copyright 2019 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <smk/Shader.hpp>

#include <cstdlib>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

namespace smk {

using namespace glm;

const std::string shader_header =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
    "precision mediump float;\n"
    "precision mediump int;\n"
    "precision mediump sampler2DArray;\n";
#else
    "#version 330\n";
#endif

// static
/// @brief Load a shader from a file.
/// @param filename The text filename where the shader is written.
/// @param type
///   Either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER. It can also be
///   any other shader type defined by OpenGL.
///   See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateShader.xhtml
//
/// @see Shader::FromString
/// @see Shader::FromFile
Shader Shader::FromFile(const std::string& filename, GLenum type) {
  std::ifstream file(filename);
  return Shader::FromString(std::string(std::istreambuf_iterator<char>(file),
                                        std::istreambuf_iterator<char>()),
                            type);
}

// static
/// @brief Load a shader from a std::string.
/// @param content The string representing the shader.
/// @param type
///   Either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER. It can also be
///   any other shader type defined by OpenGL.
///   See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateShader.xhtml
//
/// @see Shader::FromString
/// @see Shader::FromFile
Shader Shader::FromString(const std::string& content, GLenum type) {
  std::vector<char> buffer;
  for (const auto& c : shader_header)
    buffer.push_back(c);
  for (const auto& c : content)
    buffer.push_back(c);
  buffer.push_back('\0');
  return Shader(std::move(buffer), type);
}

/// @brief The GPU shader handle.
/// @return The OpenGL shader id. If the Shader is invalid, returns zero.
GLuint Shader::GetHandle() const {
  return handle_;
}


Shader::Shader() = default;
Shader::Shader(std::vector<char> content, GLenum type) {
  // creation
  handle_ = glCreateShader(type);
  if (handle_ == 0) {
    std::cerr << "[Error] Impossible to create a new Shader" << std::endl;
    throw std::runtime_error("[Error] Impossible to create a new Shader");
  }

  // code source assignation
  const char* shaderText(&content[0]);
  glShaderSource(handle_, 1, (const GLchar**)&shaderText, NULL);

  // compilation
  glCompileShader(handle_);

  // compilation check
  GLint compile_status;
  glGetShaderiv(handle_, GL_COMPILE_STATUS, &compile_status);
  if (compile_status != GL_TRUE) {
    GLsizei logsize = 0;
    glGetShaderiv(handle_, GL_INFO_LOG_LENGTH, &logsize);

    char* log = new char[logsize + 1];
    glGetShaderInfoLog(handle_, logsize, &logsize, log);

    std::cout << "[Error] compilation error: " << std::endl;
    std::cout << log << std::endl;

    exit(EXIT_FAILURE);
  }
}

Shader::~Shader() {
  if (!handle_)
    return;
  glDeleteShader(handle_);
  handle_ = 0;
}

Shader::Shader(Shader&& other) {
  this->operator=(std::move(other));
}

void Shader::operator=(Shader&& other) {
  std::swap(handle_, other.handle_);
}

/// @brief The constructor. The ShaderProgram is initially invalid. You need to
/// call @ref AddShader and @ref Link before being able to use it.
ShaderProgram::ShaderProgram() = default;

/// @brief Add a Shader to the program list. This must called multiple time for
/// each shader components before calling @ref Link.
/// @param shader The Shader to be added to the program list.
void ShaderProgram::AddShader(const Shader& shader) {
  if (!handle_) {
    handle_ = glCreateProgram();
    if (!handle_)
      std::cerr << "[Error] Impossible to create a new Shader" << std::endl;
  }

  glAttachShader(handle_, shader.GetHandle());
}

/// @brief Add a Shader to the program list.
void ShaderProgram::Link() {
  glLinkProgram(handle_);
  GLint result;
  glGetProgramiv(handle_, GL_LINK_STATUS, &result);
  if (result != GL_TRUE) {
    std::cout << "[Error] linkage error" << std::endl;

    GLsizei logsize = 0;
    glGetProgramiv(handle_, GL_INFO_LOG_LENGTH, &logsize);

    char* log = new char[logsize];
    glGetProgramInfoLog(handle_, logsize, &logsize, log);

    std::cout << log << std::endl;
  }
}

/// @brief Return the uniform ID.
/// @param name The uniform name in the Shader.
/// @return The GPU uniform ID. Return 0 and display an error if not found.
GLint ShaderProgram::Uniform(const std::string& name) {
  auto it = uniforms_.find(name);
  if (it == uniforms_.end()) {
    // uniforms_ that is not referenced
    GLint r = glGetUniformLocation(handle_, name.c_str());

    if (r == GL_INVALID_OPERATION || r < 0) {
      std::cerr << "[Error] Uniform " << name << " doesn't exist in program"
                << std::endl;
    }
    // add it anyways
    uniforms_[name] = r;

    return r;
  } else
    return it->second;
}

/// @brief Return the GPU attribute handle.
/// @param name The attribute name in the Shader.
/// @return The GPU attribute ID. Return 0 and display an error if not found.
GLint ShaderProgram::Attribute(const std::string& name) {
  GLint attrib = glGetAttribLocation(handle_, name.c_str());
  if (attrib == GL_INVALID_OPERATION || attrib < 0)
    std::cerr << "[Error] Attribute " << name << " doesn't exist in program"
              << std::endl;

  return attrib;
}

/// @brief
///   Set an OpenGL attribute properties.
/// @param name
///   Attribute name in the Shader.
/// @param size
///   Specify the number of component per object. One of {1,2,3,4}.
/// @param stride
///   Specify the byte offset in between consecutive attribute of
///   the same kind.
/// @param offset
///   Offset of the attribute in the struct.
/// @param type
///   The type of data. For instance GL_FLOAT.
/// @return
///   The GPU attribute ID. Return 0 and display an error if not found.
///
/// @see glVertexAttribPointer
void ShaderProgram::SetAttribute(const std::string& name,
                                 GLint size,
                                 GLsizei stride,
                                 GLuint offset,
                                 GLboolean normalize,
                                 GLenum type) {
  GLint loc = Attribute(name);
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, size, type, normalize, stride,
                        reinterpret_cast<void*>(offset));
}

/// @brief Set an OpenGL attribute properties, assuming data are float.
/// @see SetAttribute.
void ShaderProgram::SetAttribute(const std::string& name,
                                 GLint size,
                                 GLsizei stride,
                                 GLuint offset,
                                 GLboolean normalize) {
  SetAttribute(name, size, stride, offset, normalize, GL_FLOAT);
}

/// @brief Set an OpenGL attribute properties, assuming data are float.
/// @see SetAttribute.
void ShaderProgram::SetAttribute(const std::string& name,
                                 GLint size,
                                 GLsizei stride,
                                 GLuint offset,
                                 GLenum type) {
  SetAttribute(name, size, stride, offset, false, type);
}

/// @brief Set an OpenGL attribute properties, assuming data are float.
/// @see SetAttribute.
void ShaderProgram::SetAttribute(const std::string& name,
                                 GLint size,
                                 GLsizei stride,
                                 GLuint offset) {
  SetAttribute(name, size, stride, offset, false, GL_FLOAT);
}

/// @brief Assign shader vec3 uniform 
/// @param x First vec3 component.
/// @param y Second vec3 component.
/// @param y Third vec3 component
/// @overload
void ShaderProgram::SetUniform(const std::string& name,
                               float x,
                               float y,
                               float z) {
  glUniform3f(Uniform(name), x, y, z);
}

/// @brief Assign shader vec3 uniform 
/// @param v vec3 value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, const vec3& v) {
  glUniform3fv(Uniform(name), 1, value_ptr(v));
}

/// @brief Assign shader vec4 uniform 
/// @param v vec4 value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, const vec4& v) {
  glUniform4fv(Uniform(name), 1, value_ptr(v));
}

/// @brief Assign shader mat4 uniform 
/// @param m mat4 value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, const mat4& m) {
  glUniformMatrix4fv(Uniform(name), 1, GL_FALSE, value_ptr(m));
}

/// @brief Assign shader mat3 uniform 
/// @param m mat3 value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, const mat3& m) {
  glUniformMatrix3fv(Uniform(name), 1, GL_FALSE, value_ptr(m));
}

/// @brief Assign shader float uniform 
/// @param val float value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, float val) {
  glUniform1f(Uniform(name), val);
}

/// @brief Assign shader int uniform 
/// @param val int value
/// @overload
void ShaderProgram::SetUniform(const std::string& name, int val) {
  glUniform1i(Uniform(name), val);
}

ShaderProgram::~ShaderProgram() {
  if (!handle_)
    return;
  glDeleteProgram(handle_);
  handle_ = 0;
}

/// @brief Bind the ShaderProgram. Future draw will use it. This unbind any
/// previously bound ShaderProgram.
void ShaderProgram::Use() const {
  glUseProgram(handle_);
}

/// @brief Unbind the ShaderProgram.
void ShaderProgram::Unuse() const {
  glUseProgram(0);
}

/// @brief The GPU handle to the ShaderProgram.
/// @return The GPU handle to the ShaderProgram.
GLuint ShaderProgram::GetHandle() const {
  return handle_;
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) {
  this->operator=(std::move(other));
}

void ShaderProgram::operator=(ShaderProgram&& other) {
  std::swap(handle_, other.handle_);
  std::swap(uniforms_, other.uniforms_);
}

}  // namespace smk
