/**
 * @file SdlGamepad.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLGAMEPAD_HPP_
#define SDLGAMEPAD_HPP_

//Sdl
#include <SDL3/SDL.h>

//Interface
#include "IGamepad.hpp"

#include <unordered_map>

class SdlWindow;

/**
 * @brief La manette numero _index, ouverte a la construction.
 *
 * SDL_Gamepad normalise deja les boutons : pas de table d'indices
 * bruts a deviner comme chez sfml, l'enum du contrat se pose dessus
 * presque un pour un.
 */
class SdlGamepad : public graphic::IGamepad {

    public:
        SdlGamepad(const SdlWindow &window, int index = 0) : _window(window), _index(index) {
            /* SDL3 n'ouvre plus par rang mais par identifiant d'instance :
             * il faut donc demander la liste, ce qui a le merite de ne
             * jamais ouvrir une manette debranchee entre temps. */
            int count = 0;
            SDL_JoystickID *pads = SDL_GetGamepads(&count);

            if (!pads)
                return;
            if (_index < count && SDL_IsGamepad(pads[_index]))
                _pad = SDL_OpenGamepad(pads[_index]);
            SDL_free(pads);
        }

        ~SdlGamepad() override {
            if (_pad)
                SDL_CloseGamepad(_pad);
        }

        bool isAvailable() const override {
            return _pad != nullptr && SDL_GamepadConnected(_pad);
        }

        //definis dans SdlWindow.hpp, une fois SdlWindow complete
        bool isButtonPressed(Button button) const override;
        bool isButtonReleased(Button button) const override;

        bool isButtonDown(Button button) const override {
            return isAvailable() && SDL_GetGamepadButton(_pad, _buttons.at(button)) != 0;
        }
        bool isButtonUp(Button button) const override { return !isButtonDown(button); }

        float getAxisMovement(Axis axis) const override {
            if (!isAvailable())
                return 0.f;
            //SDL rend un Sint16 : on ramene dans [-1, 1] comme le contrat
            return SDL_GetGamepadAxis(_pad, _axes.at(axis)) / 32767.f;
        }

        friend class SdlWindow;

    private:
        const SdlWindow &_window;
        int _index;
        SDL_Gamepad *_pad = nullptr;

        const std::unordered_map<IGamepad::Button, SDL_GamepadButton> _buttons = {
            {IGamepad::BUTTON_A, SDL_GAMEPAD_BUTTON_SOUTH},
            {IGamepad::BUTTON_B, SDL_GAMEPAD_BUTTON_EAST},
            {IGamepad::BUTTON_X, SDL_GAMEPAD_BUTTON_WEST},
            {IGamepad::BUTTON_Y, SDL_GAMEPAD_BUTTON_NORTH},
            {IGamepad::BUTTON_LEFT_BUMPER, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
            {IGamepad::BUTTON_RIGHT_BUMPER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
            {IGamepad::BUTTON_BACK, SDL_GAMEPAD_BUTTON_BACK},
            {IGamepad::BUTTON_START, SDL_GAMEPAD_BUTTON_START},
            {IGamepad::BUTTON_LEFT_THUMB, SDL_GAMEPAD_BUTTON_LEFT_STICK},
            {IGamepad::BUTTON_RIGHT_THUMB, SDL_GAMEPAD_BUTTON_RIGHT_STICK},
            {IGamepad::BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_UP},
            {IGamepad::BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
            {IGamepad::BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_DOWN},
            {IGamepad::BUTTON_DPAD_LEFT, SDL_GAMEPAD_BUTTON_DPAD_LEFT},
        };

        const std::unordered_map<IGamepad::Axis, SDL_GamepadAxis> _axes = {
            {IGamepad::AXIS_LEFT_X, SDL_GAMEPAD_AXIS_LEFTX},
            {IGamepad::AXIS_LEFT_Y, SDL_GAMEPAD_AXIS_LEFTY},
            {IGamepad::AXIS_RIGHT_X, SDL_GAMEPAD_AXIS_RIGHTX},
            {IGamepad::AXIS_RIGHT_Y, SDL_GAMEPAD_AXIS_RIGHTY},
            {IGamepad::AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
            {IGamepad::AXIS_RIGHT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
        };
};

/** @} */

#endif /* !SDLGAMEPAD_HPP_ */
