/**
 * @file SdlAudioModule.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @copyright Copyright (c) 2026
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLAUDIO_MODULE_HPP
#define SDLAUDIO_MODULE_HPP

#include "IAudioModule.hpp"

#include "audio/SdlMusic.hpp"
#include "audio/SdlSound.hpp"
#include "audio/SdlSoundBuffer.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

/**
 * @brief SDL3_mixer derriere IAudioModule.
 *
 * SDL3 a remplace le peripherique global de SDL2 par un MIX_Mixer explicite
 * qu'on possede. C'est lui qui fabrique les tampons et les pistes, donc le
 * module le tient et le passe a chaque objet qu'il cree.
 *
 * Ce module est independant du module graphique : on peut prendre le son de
 * SDL3 et l'image de sfml.
 */
class SdlAudioModule : public IAudioModule {

public:
    SdlAudioModule() = default;
    ~SdlAudioModule() { stop(); }

    const char *type() const override { return IAudioModule::contract; }
    const char *name() const override { return "sdl3"; }

    // music
    audio::IMusic *createMusic(std::string path) override {
        if (!start())
            return nullptr;
        _opened++;
        return new SdlMusic(_mixer, path);
    }
    void deleteMusic(audio::IMusic *music) override { drop(music); }

    // sound buffer
    audio::ISoundBuffer *createSoundBuffer(std::string path) override {
        if (!start())
            return nullptr;
        _opened++;
        return new SdlSoundBuffer(_mixer, path);
    }
    void deleteSoundBuffer(audio::ISoundBuffer *buffer) override { drop(buffer); }

    // sound
    audio::ISound *createSound(audio::ISoundBuffer *buffer) override {
        if (!buffer || !start())
            return nullptr;
        _opened++;
        return new SdlSound(_mixer, *static_cast<SdlSoundBuffer *>(buffer));
    }
    void deleteSound(audio::ISound *sound) override { drop(sound); }

private:
    /**
     * @brief Ouvre le melangeur au premier son demande.
     *
     * Rien dans le constructeur du module : celui-ci s'execute au dlopen,
     * bien avant qu'on sache si ce vendor sera choisi, et ouvrir le
     * peripherique a ce moment le confisquerait aux autres.
     */
    bool start() {
        if (_started)
            return true;
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            return false;
        if (!MIX_Init()) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            return false;
        }
        /* MIX_CreateMixerDevice et pas MIX_CreateMixer : le second genere
         * vers un tampon memoire qu'il faut pousser soi-meme, et exige un
         * format. C'est celui-ci qui ouvre le peripherique de sortie.
         *
         * DEFAULT_PLAYBACK plutot qu'un peripherique nomme : SDL suit alors
         * le defaut du systeme, casque branche en cours de route compris.
         * nullptr pour le format : SDL_mixer convertit de toute facon. */
        _mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!_mixer) {
            MIX_Quit();
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            return false;
        }
        _started = true;
        return true;
    }

    void stop() {
        if (!_started)
            return;
        MIX_DestroyMixer(_mixer);
        _mixer = nullptr;
        MIX_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        _started = false;
    }

    /** @brief Detruit, et rend le peripherique quand plus rien ne joue. */
    template <typename T>
    void drop(T *object) {
        if (!object)
            return;
        delete object;
        if (_opened > 0)
            _opened--;
        if (_opened == 0)
            stop();
    }

    MIX_Mixer *_mixer = nullptr;
    unsigned _opened = 0;
    bool _started = false;
};

/** @} */

#endif /* !SDLAUDIO_MODULE_HPP */
