/**
 * @file SdlSoundBuffer.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLSOUNDBUFFER_HPP_
#define SDLSOUNDBUFFER_HPP_

//Sdl
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

//Interface
#include "ISoundBuffer.hpp"

#include <iostream>
#include <string>

/**
 * @brief Un echantillon decode d'avance, partageable par plusieurs sons.
 *
 * SDL3_mixer a coupe en deux ce que SDL2 melangeait : le MIX_Audio est la
 * MATIERE - decodee une fois, partageable - et le MIX_Track est la LECTURE.
 * C'est exactement le decoupage que le contrat decrit entre ISoundBuffer et
 * ISound, la ou SDL2 obligeait a bricoler avec des numeros de canal.
 *
 * predecode = true : tout est mis en memoire au chargement, ce qui est ce
 * qu'on veut d'un bruitage court joue souvent.
 */
class SdlSoundBuffer : public audio::ISoundBuffer {

    public:
        SdlSoundBuffer(MIX_Mixer *mixer, std::string path) {
            _audio = MIX_LoadAudio(mixer, path.c_str(), true);
            if (!_audio)
                std::cerr << "Failed to load sound: " << SDL_GetError() << std::endl;
        }

        ~SdlSoundBuffer() {
            if (_audio)
                MIX_DestroyAudio(_audio);
        }

        bool isReady() const override { return _audio != nullptr; }

        /**
         * @brief La duree, en secondes.
         *
         * SDL3_mixer la donne, la ou SDL2 obligeait a la deduire de la
         * taille brute et du format ouvert.
         */
        float getLength() const override {
            if (!_audio)
                return 0.f;

            const Sint64 frames = MIX_GetAudioDuration(_audio);

            if (frames <= 0)
                return 0.f;
            return static_cast<float>(MIX_AudioFramesToMS(_audio, frames)) / 1000.f;
        }

        MIX_Audio *handle() const { return _audio; }

    private:
        MIX_Audio *_audio = nullptr;
};

/** @} */

#endif /* !SDLSOUNDBUFFER_HPP_ */
