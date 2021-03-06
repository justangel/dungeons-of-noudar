//
// Created by monty on 10/10/16.
//
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __APPLE__
#if TARGET_IOS
#import <OpenAl/al.h>
#import <OpenAl/alc.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#endif
#else

#include <AL/al.h>
#include <AL/alc.h>

#endif

#include "SoundClip.h"

namespace odb {
    SoundClip::SoundClip(unsigned char *buffer, int size, int offset, int bits, int channels, int frequency) {

        ALenum format = 0;

        if (bits == 8) {
            if (channels == 1) {
                format = AL_FORMAT_MONO8;
            } else if (channels == 2) {
                format = AL_FORMAT_STEREO8;
            }
        } else if (bits == 16) {
            if (channels == 1) {
                format = AL_FORMAT_MONO16;
            } else if (channels == 2) {
                format = AL_FORMAT_STEREO16;
            }
        }

        ALuint buffers[1];
        alGenBuffers(1, buffers);
        mBufferHandle = buffers[ 0 ];
        alBufferData(mBufferHandle, format, &buffer[offset], size - offset, frequency);
    }
}
