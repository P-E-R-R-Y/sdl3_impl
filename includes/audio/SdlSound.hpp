/**
 * @file SdlSound.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLSOUND_HPP_
#define SDLSOUND_HPP_

//Sdl
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

//Interface
#include "ISound.hpp"

//encapsulation
#include "SdlSoundBuffer.hpp"

/**
 * @brief Une lecture d'un tampon, sur sa propre piste.
 *
 * Chaque son a SA piste, creee a la construction et gardee jusqu'a la fin.
 * Plus de canal a reserver au premier play() ni a verifier avant chaque
 * ordre, comme il le fallait en SDL2 : la piste est a nous, donc pause() et
 * stop() ne peuvent plus toucher le son de quelqu'un d'autre.
 */
class SdlSound : public audio::ISound {

    public:
        SdlSound(MIX_Mixer *mixer, SdlSoundBuffer &buffer) : _buffer(buffer) {
            _track = MIX_CreateTrack(mixer);
            if (_track && buffer.isReady())
                MIX_SetTrackAudio(_track, buffer.handle());
        }

        ~SdlSound() override {
            if (_track)
                MIX_DestroyTrack(_track);
        }

        bool isReady() const override { return _track != nullptr && _buffer.isReady(); }

        void play() override {
            if (!isReady())
                return;
            if (MIX_TrackPaused(_track)) {
                MIX_ResumeTrack(_track);
                return;
            }
            MIX_PlayTrack(_track, 0);   //0 : aucune option, la piste dit deja tout
        }

        void pause() override {
            if (_track)
                MIX_PauseTrack(_track);
        }

        void stop() override {
            if (_track)
                MIX_StopTrack(_track, 0);   //0 : coupe net, sans fondu
        }

        /// dans [0, 1] comme le contrat ; MIX_SetTrackGain accepte au-dela
        void setVolume(float volume) override {
            _volume = volume;
            if (_track)
                MIX_SetTrackGain(_track, volume);
        }

        float getVolume() const override { return _volume; }

        /* Vraie spatialisation, contrairement a SDL2 qui n'avait qu'une
         * panoramique gauche/droite : SDL3_mixer place la source dans
         * l'espace et calcule lui-meme ce que chaque oreille entend. */
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

        //aucun effet Doppler dans SDL3_mixer : retenue, jamais appliquee
        void setVelocity(Vector3f velocity) override { _velocity = velocity; }
        Vector3f getVelocity() const override { return _velocity; }

    private:
        SdlSoundBuffer &_buffer;
        MIX_Track *_track = nullptr;
        float _volume = 1.f;
        Vector3f _position{0, 0, 0};
        Vector3f _velocity{0, 0, 0};
};

/** @} */

#endif /* !SDLSOUND_HPP_ */
