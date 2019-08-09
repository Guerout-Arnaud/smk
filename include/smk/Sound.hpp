// Copyright 2019 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef SMK_SOUND_HPP
#define SMK_SOUND_HPP

#include <smk/SoundBuffer.hpp>

namespace smk {

class Sound {
 public:
  // Please make sure to init OpenAL in main() by creating a smk::Audio.
  Sound();
  Sound(const SoundBuffer& buffer);
  ~Sound();
  void SetBuffer(const SoundBuffer& buffer);
  const SoundBuffer* buffer() { return buffer_; }

  void Play();
  void Stop();
  void SetLoop(bool looping);
  bool is_playing() { return is_playing_; };

  // The gain applied to the source. Default is 1.
  void SetVolume(float volume);

  // -- Move-only resource ---
  Sound(Sound&&);
  Sound(const Sound&) = delete;
  void operator=(Sound&&);
  void operator=(const Sound&) = delete;

 private:
  const SoundBuffer* buffer_ = nullptr;
  unsigned int source_ = 0;
  bool is_playing_ = false;

  void EnsureSourceIsCreated();
};

}  // namespace smk

#endif /* end of include guard: SMK_SOUND_HPP */
