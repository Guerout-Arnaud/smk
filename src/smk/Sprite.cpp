// Copyright 2019 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <smk/Framebuffer.hpp>
#include <smk/OpenGL.hpp>
#include <smk/Shader.hpp>
#include <smk/Sprite.hpp>
#include <smk/VertexArray.hpp>
#include <vector>

namespace smk {

Sprite::Sprite(const Texture& texture) : Sprite() {
  Transformable::SetTexture(texture);
  SetTextureRectangle({0.f, 0.f, float(texture.width()), float(texture.height())});
}

Sprite::Sprite(const Texture& texture, const Rectangle rectangle) {
  Transformable::SetTexture(texture);
  SetTextureRectangle(rectangle);
}

Sprite::Sprite(const Framebuffer& framebuffer) {
  Transformable::SetTexture(framebuffer.color_texture);
  float l = 0.f;
  float r = 1.f;
  float t = 0.f;
  float b = 1.f;
  float www = framebuffer.color_texture.width();
  float hhh = framebuffer.color_texture.height();
  SetVertexArray(VertexArray(std::vector<Vertex>({
      {{0.f, 0.f}, {l, b}},
      {{0.f, hhh}, {l, t}},
      {{www, hhh}, {r, t}},
      {{0.f, 0.f}, {l, b}},
      {{www, hhh}, {r, t}},
      {{www, 0.f}, {r, b}},
  })));
}

void Sprite::SetTextureRectangle(const Rectangle& rectangle) {
  float l = (rectangle.left + 0.5) / texture()->width();
  float r = (rectangle.right - 0.5) / texture()->width();
  float t = (rectangle.top + 0.5) / texture()->height();
  float b = (rectangle.bottom - 0.5) / texture()->height();
  float www = rectangle.width();
  float hhh = rectangle.height();
  SetVertexArray(VertexArray(std::vector<Vertex>({
      {{0.f, 0.f}, {l, t}},
      {{0.f, hhh}, {l, b}},
      {{www, hhh}, {r, b}},
      {{0.f, 0.f}, {l, t}},
      {{www, hhh}, {r, b}},
      {{www, 0.f}, {r, t}},
  })));
}

}  // namespace smk
