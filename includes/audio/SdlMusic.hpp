/**
 * @file SdlMusic.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLMUSIC_HPP_
#define SDLMUSIC_HPP_

//Sdl
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

//Interface
#include "IMusic.hpp"

#include <iostream>
#include <string>

/**
 * @brief Un flux long, decode au fil de la lecture, sur sa propre piste.
 *
 * SDL3_mixer n'a plus de canal musique unique : une musique est une piste
 * comme une autre, avec le tampon charge en flux plutot qu'en entier. Deux
 * musiques peuvent donc jouer ensemble, ce que SDL2 rendait impossible - et
 * plus besoin de verifier avant chaque ordre que le canal est encore le
 * notre.
 *
 * predecode = false : le fichier est lu au fur et a mesure, ce qui evite de
 * mettre plusieurs minutes d'audio en memoire.
 */
class SdlMusic : public audio::IMusic {

    public:
        SdlMusic(MIX_Mixer *mixer, std::string path) {
            _audio = MIX_LoadAudio(mixer, path.c_str(), false);
            if (!_audio) {
                std::cerr << "Failed to load music: " << SDL_GetError() << std::endl;
                return;
            }
            _track = MIX_CreateTrack(mixer);
            if (_track)
                MIX_SetTrackAudio(_track, _audio);
        }

        ~SdlMusic() override {
            if (_track)
                MIX_DestroyTrack(_track);
            if (_audio)
                MIX_DestroyAudio(_audio);
        }

        bool isReady() const override { return _audio != nullptr && _track != nullptr; }

        void play() override {
            if (!isReady())
                return;
            if (MIX_TrackPaused(_track)) {
                MIX_ResumeTrack(_track);
                return;
            }
            MIX_SetTrackLoops(_track, _loop ? -1 : 0);
            MIX_PlayTrack(_track, 0);
        }

        void pause() override {
            if (_track)
                MIX_PauseTrack(_track);
        }

        void stop() override {
            if (_track)
                MIX_StopTrack(_track, 0);
        }

        /// SDL_mixer decode sur son propre fil : rien a pousser par frame
        void update() override {}

        void setVolume(float volume) override {
            _volume = volume;
            if (_track)
                MIX_SetTrackGain(_track, volume);
        }

        float getVolume() const override { return _volume; }

        /* La boucle se pose sur la piste, donc elle peut changer en cours de
         * lecture - la ou SDL2 la figeait au lancement. */
        void setLoop(bool loop) override {
            _loop = loop;
            if (_track)
                MIX_SetTrackLoops(_track, loop ? -1 : 0);
        }

        bool getLoop() const override { return _loop; }

        void setTime(float position) override {
            if (_track)
                MIX_SetTrackPlaybackPosition(_track, MIX_TrackMSToFrames(_track, static_cast<Sint64>(position * 1000.f)));
        }

        /// la vraie position de lecture, que SDL2 ne savait pas donner
        float getTime() const override {
            if (!_track)
                return 0.f;
            return static_cast<float>(MIX_TrackFramesToMS(_track, MIX_GetTrackPlaybackPosition(_track))) / 1000.f;
        }

        float getLength() const override {
            if (!_audio)
                return 0.f;

            const Sint64 frames = MIX_GetAudioDuration(_audio);

            if (frames <= 0)
                return 0.f;
            return static_cast<float>(MIX_AudioFramesToMS(_audio, frames)) / 1000.f;
        }

        void setPosition(Vector3f position) override {
            _position = position;
            if (!_track)
                return;

            const MIX_Point3D point{static_cast<float>(position.x),
                                    static_cast<float>(position.y),
                                    static_cast<float>(position.z)};

            MIX_SetTrack3DPosition(_track, &point);
        }

        Vector3f getPosition() const override { return _position; }

        void setVelocity(Vector3f velocity) override { _velocity = velocity; }
        Vector3f getVelocity() const override { return _velocity; }

    private:
        MIX_Audio *_audio = nullptr;
        MIX_Track *_track = nullptr;

        float _volume = 1.f;
        bool _loop = false;
        Vector3f _position{0, 0, 0};
        Vector3f _velocity{0, 0, 0};
};

/** @} */

#endif /* !SDLMUSIC_HPP_ */
