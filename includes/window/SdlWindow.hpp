/**
 * @file SdlWindow.hpp
 * @author Perry Chouteau (perry.chouteau@outlook.com)
 * @brief
 * @date 2026-08-28
 *
 * @addtogroup sdl3
 * @{
 */

#ifndef SDLWINDOW_HPP_
#define SDLWINDOW_HPP_

//Sdl
#include <SDL3/SDL.h>

//Interface
#include "IWindow2.hpp"

//encapsulation
#include "SdlGamepad.hpp"
#include "SdlKeyboard.hpp"
#include "SdlMouse.hpp"
#include "SdlPolygon.hpp"
#include "SdlSprite.hpp"
#include "SdlText.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Une fenetre SDL et son renderer, implemente IWindow2.
 *
 * SDL n'a PAS de file par fenetre : SDL_PollEvent vide une file unique pour
 * tout le processus. Une borne et un jeu qui appelleraient chacun
 * SDL_PollEvent se voleraient donc les evenements de l'autre, et le premier
 * servi gagnerait.
 *
 * D'ou pump() : UNE lecture de la file SDL, et chaque evenement est range
 * dans la fenetre que son windowID designe. Ceux qui n'en ont pas - SDL_EVENT_QUIT,
 * la manette - vont a tout le monde. Chaque fenetre lit ensuite SA file, et
 * personne ne prive personne.
 *
 * Entre pollEvent() et endDraw(), toute lecture est idempotente : c'est la
 * norme du contrat, celle qui laisse une borne et son invite lire la meme
 * touche dans la meme frame.
 */
class SdlWindow : public graphic::IWindow2 {

    public:
        SdlWindow(int32_t screenWidth, int32_t screenHeight, std::string title) {
            /* SDL3 ne prend plus de position : le gestionnaire place la
             * fenetre, et setPosition() reste la pour qui veut choisir. */
            _window = SDL_CreateWindow(title.c_str(), screenWidth, screenHeight,
                                       SDL_WINDOW_HIGH_PIXEL_DENSITY);
            if (!_window)
                return;

            //nullptr : on laisse SDL choisir son pilote, Metal sur macOS
            _renderer = SDL_CreateRenderer(_window, nullptr);
            if (_renderer) {
                //sinon un alpha < 255 est ecrase au lieu d'etre melange
                SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
                /* Une unite de dessin = un pixel logique, quel que soit
                 * l'ecran. Sans ca, sur un ecran retina la souris et le
                 * dessin ne parlent plus du meme repere - le meme piege que
                 * la vue chez sfml. */
                SDL_SetRenderLogicalPresentation(_renderer, screenWidth, screenHeight,
                                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);
            }

            _id = SDL_GetWindowID(_window);
            registry()[_id] = this;
            _lastTicks = SDL_GetTicks();
        }

        ~SdlWindow() {
            registry().erase(_id);
            if (_renderer)
                SDL_DestroyRenderer(_renderer);
            if (_window)
                SDL_DestroyWindow(_window);
        }

        //lifecycle
        bool isOpen() override { return _open && _window != nullptr; }

        void close() override { _open = false; }

        Vector2f getPosition() override {
            int x = 0, y = 0;

            SDL_GetWindowPosition(_window, &x, &y);
            return {static_cast<double>(x), static_cast<double>(y)};
        }

        void setPosition(Vector2f position) override {
            SDL_SetWindowPosition(_window, static_cast<int>(position.x), static_cast<int>(position.y));
        }

        Vector2f getSize() override {
            int w = 0, h = 0;

            SDL_GetWindowSize(_window, &w, &h);
            return {static_cast<double>(w), static_cast<double>(h)};
        }

        void setSize(Vector2f size) override {
            SDL_SetWindowSize(_window, static_cast<int>(size.x), static_cast<int>(size.y));
            if (_renderer)
                SDL_SetRenderLogicalPresentation(_renderer, static_cast<int>(size.x), static_cast<int>(size.y),
                                                 SDL_LOGICAL_PRESENTATION_LETTERBOX);
        }

        void setFrameLimit(int32_t limit) override { _frameLimit = limit; }

        int32_t getDelta() override { return _delta; }

        /**
         * @brief Vide la file SDL une fois, puis rend l'etat de CETTE fenetre.
         *
         * Rappele dans la meme frame, il trouve la file SDL vide, n'ajoute
         * rien, et repond la meme chose.
         */
        bool pollEvent() override {
            pump();
            return !_events.empty();
        }

        void eventClose() override {
            for (const SDL_Event &event : _events) {
                if (event.type == SDL_EVENT_QUIT ||
                    event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                    (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)) {
                    _open = false;
                    return;
                }
            }
        }

        //2D
        void beginDraw() override {
            if (!_renderer)
                return;
            SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
            SDL_RenderClear(_renderer);
        }

        void endDraw() override {
            if (_renderer)
                SDL_RenderPresent(_renderer);

            const uint64_t now = SDL_GetTicks();

            _delta = static_cast<int32_t>(now - _lastTicks);

            /* SDL n'a pas de limiteur : on attend nous-memes de quoi tenir
             * la cadence demandee, sinon la boucle tourne a vide et mange
             * un coeur entier. */
            if (_frameLimit > 0) {
                const int32_t budget = 1000 / _frameLimit;

                if (_delta < budget) {
                    SDL_Delay(static_cast<uint32_t>(budget - _delta));
                    _delta = budget;
                }
            }

            _lastTicks = SDL_GetTicks();
            _events.clear();   //la frontiere de frame, cf. IWindow::pollEvent
        }

        void drawPoly(graphic::IPolygon *polygon) override;
        void drawSprite(graphic::ISprite *sprite) override;
        void drawText(graphic::IText *text) override;

        friend class SdlKeyboard;
        friend class SdlMouse;
        friend class SdlGamepad;

    private:
        /** @brief Les fenetres vivantes, par identifiant SDL. */
        static std::unordered_map<uint32_t, SdlWindow *> &registry() {
            static std::unordered_map<uint32_t, SdlWindow *> windows;

            return windows;
        }

        /**
         * @brief Vide la file SDL et repartit chaque evenement par fenetre.
         *
         * La seule lecture de SDL_PollEvent de tout le vendor. Ce qui ne
         * designe aucune fenetre - fermeture demandee au processus, manette
         * branchee - va a toutes : aucune ne sait laquelle est concernee, et
         * les ignorer serait pire que les repeter.
         */
        static void pump() {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                const uint32_t target = windowOf(event);

                if (target == 0) {
                    for (auto &[id, window] : registry())
                        window->push(event);
                    continue;
                }

                const auto found = registry().find(target);

                if (found == registry().end())
                    continue;   //une fenetre deja detruite : l'evenement tombe
                found->second->push(event);
            }
        }

        /** @brief La fenetre visee, ou 0 quand l'evenement s'adresse a tous. */
        static uint32_t windowOf(const SDL_Event &event) {
            /* SDL3 a eclate SDL_WINDOWEVENT en un type par cas, tous ranges
             * dans une plage : les prendre d'un bloc evite d'en oublier un
             * a chaque version. */
            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
                return event.window.windowID;

            switch (event.type) {
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:              return event.key.windowID;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:      return event.button.windowID;
                case SDL_EVENT_MOUSE_MOTION:        return event.motion.windowID;
                case SDL_EVENT_MOUSE_WHEEL:         return event.wheel.windowID;
                case SDL_EVENT_TEXT_INPUT:          return event.text.windowID;
                default:                     return 0;
            }
        }

        /**
         * @brief Range l'evenement dans la frame de cette fenetre.
         *
         * La file est bornee : une fenetre ouverte mais jamais dessinee
         * n'appelle jamais endDraw(), donc rien ne la viderait et pump()
         * continuerait d'y empiler jusqu'a manger la memoire. Passe la
         * borne, le plus ancien part - une frame en compte quelques uns,
         * donc ce plafond ne se voit jamais en usage normal.
         */
        void push(const SDL_Event &event) {
            if (_events.size() >= MAX_EVENTS)
                _events.erase(_events.begin());
            _events.push_back(event);
            feedEvent(event);
        }

        /* L'etat reconstruit DEPUIS LES EVENEMENTS, jamais par une requete
         * globale comme SDL_GetKeyboardState() : celle-la est commune au
         * processus, donc deux fenetres y liraient la meme chose meme si
         * une seule a le focus.
         *
         * Seul ce qu'aucun evenement ne peut dire est garde : une touche
         * reste "enfoncee" entre son KEYDOWN et son KEYUP. Les fronts, eux,
         * se relisent dans _events, qui porte la frame. */
        void feedEvent(const SDL_Event &event) {
            switch (event.type) {
                case SDL_EVENT_KEY_DOWN:
                    if (!event.key.repeat)
                        _keysDown[event.key.scancode] = true;
                    break;
                case SDL_EVENT_KEY_UP:
                    _keysDown[event.key.scancode] = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    _mouseDown[event.button.button] = true;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    _mouseDown[event.button.button] = false;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    _mousePosition = {static_cast<double>(event.motion.x), static_cast<double>(event.motion.y)};
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    //sans ca, une touche relachee hors focus resterait "enfoncee"
                    _keysDown.fill(false);
                    _mouseDown.fill(false);
                    break;
                default:
                    break;
            }
        }

        /// de quoi tenir plusieurs frames chargees, et pas une fuite
        static constexpr size_t MAX_EVENTS = 1024;

        SDL_Window *_window = nullptr;
        SDL_Renderer *_renderer = nullptr;
        uint32_t _id = 0;
        bool _open = true;

        /* Les evenements de la frame. Rempli par pump(), vide par endDraw() :
         * entre les deux, tout le monde y lit la meme chose. */
        std::vector<SDL_Event> _events;

        uint64_t _lastTicks = 0;
        int32_t _delta = 0;
        int32_t _frameLimit = 0;

        std::array<bool, SDL_SCANCODE_COUNT> _keysDown{};
        std::array<bool, 8> _mouseDown{};
        Vector2f _mousePosition{0, 0};
};

void SdlWindow::drawPoly(graphic::IPolygon *polygon) {
    SdlPolygon *sdlPolygon = static_cast<SdlPolygon *>(polygon);

    if (!_renderer || sdlPolygon->_vertices.empty())
        return;

    /* SDL_RenderGeometry n'a pas de transformation : il faut lui donner des
     * coordonnees ecran. On recopie les sommets locaux en ajoutant la
     * position, dans un tampon qui appartient au polygone - donc aucune
     * allocation apres la premiere frame. */
    const float x = static_cast<float>(sdlPolygon->_position.x);
    const float y = static_cast<float>(sdlPolygon->_position.y);

    for (size_t i = 0; i < sdlPolygon->_vertices.size(); i++) {
        sdlPolygon->_screen[i] = sdlPolygon->_vertices[i];
        sdlPolygon->_screen[i].position.x += x;
        sdlPolygon->_screen[i].position.y += y;
    }

    SDL_RenderGeometry(_renderer, nullptr,
                       sdlPolygon->_screen.data(), static_cast<int>(sdlPolygon->_screen.size()),
                       nullptr, 0);
}

void SdlWindow::drawSprite(graphic::ISprite *sprite) {
    SdlSprite *sdlSprite = static_cast<SdlSprite *>(sprite);
    SDL_Texture *texture = sdlSprite->_texture.handle(_renderer);

    if (!texture)
        return;

    /* SDL3 est passe au flottant partout : plus d'arrondi au pixel entier
     * entre ce que le contrat donne et ce qui est trace. */
    const SDL_FRect source{
        sdlSprite->_crop.x, sdlSprite->_crop.y,
        sdlSprite->_crop.w, sdlSprite->_crop.h};
    const SDL_FRect destination{
        static_cast<float>(sdlSprite->_position.x), static_cast<float>(sdlSprite->_position.y),
        static_cast<float>(sdlSprite->_size.x), static_cast<float>(sdlSprite->_size.y)};

    SDL_RenderTextureRotated(_renderer, texture, &source, &destination,
                             sdlSprite->_rotation, nullptr, SDL_FLIP_NONE);
}

void SdlWindow::drawText(graphic::IText *text) {
    SdlText *sdlText = static_cast<SdlText *>(text);
    SDL_Texture *texture = sdlText->handle(_renderer);

    if (!texture)
        return;

    const SDL_FRect destination{
        static_cast<float>(sdlText->_position.x), static_cast<float>(sdlText->_position.y),
        static_cast<float>(sdlText->_width), static_cast<float>(sdlText->_height)};

    SDL_RenderTextureRotated(_renderer, texture, nullptr, &destination,
                             sdlText->_rotation, nullptr, SDL_FLIP_NONE);
}

/* Entrees.
 *
 * isKeyPressed / isKeyReleased : les evenements de la frame sont encore
 *         dans _events, donc un front s'y relit. Rien n'est consomme, donc
 *         n'importe quel nombre de lecteurs obtient la meme reponse.
 * isKeyDown / isKeyUp          : l'etat que la fenetre tient, puisqu'aucun
 *         evenement seul ne peut dire qu'une touche est toujours enfoncee.
 */

/** @brief true si un evenement de la frame satisfait le predicat. */
template <typename Match>
static bool anyEvent(const std::vector<SDL_Event> &events, uint32_t type, Match match) {
    for (const SDL_Event &event : events)
        if (event.type == type && match(event))
            return true;
    return false;
}

std::vector<graphic::IKeyboard::Keys> SdlKeyboard::whichKeyDown() const {
    std::vector<Keys> keys;

    for (const auto &[key, code] : _keys)
        if (_window._keysDown[code])
            keys.push_back(key);
    return keys;
}

bool SdlKeyboard::isKeyPressed(Keys key) const {
    const SDL_Scancode code = _keys.at(key);

    return anyEvent(_window._events, SDL_EVENT_KEY_DOWN,
        [code](const SDL_Event &event) { return event.key.scancode == code && !event.key.repeat; });
}

bool SdlKeyboard::isKeyReleased(Keys key) const {
    const SDL_Scancode code = _keys.at(key);

    return anyEvent(_window._events, SDL_EVENT_KEY_UP,
        [code](const SDL_Event &event) { return event.key.scancode == code; });
}

bool SdlKeyboard::isKeyDown(Keys key) const { return _window._keysDown[_keys.at(key)]; }
bool SdlKeyboard::isKeyUp(Keys key) const { return !isKeyDown(key); }

bool SdlMouse::isButtonPressed(Buttons key) const {
    const uint8_t button = _buttons.at(key);

    return anyEvent(_window._events, SDL_EVENT_MOUSE_BUTTON_DOWN,
        [button](const SDL_Event &event) { return event.button.button == button; });
}

bool SdlMouse::isButtonReleased(Buttons key) const {
    const uint8_t button = _buttons.at(key);

    return anyEvent(_window._events, SDL_EVENT_MOUSE_BUTTON_UP,
        [button](const SDL_Event &event) { return event.button.button == button; });
}

bool SdlMouse::isButtonDown(Buttons key) const { return _window._mouseDown[_buttons.at(key)]; }
bool SdlMouse::isButtonUp(Buttons key) const { return !isButtonDown(key); }

Vector2f SdlMouse::getPosition() const { return _window._mousePosition; }

void SdlMouse::setPosition(Vector2f position) {
    _window._mousePosition = position;
    SDL_WarpMouseInWindow(_window._window, static_cast<float>(position.x), static_cast<float>(position.y));
}

float SdlMouse::GetMouseWheelMove() const {
    float delta = 0.f;

    //additionne : deux crans dans la meme frame, sinon on en perd un
    for (const SDL_Event &event : _window._events)
        if (event.type == SDL_EVENT_MOUSE_WHEEL)
            delta += event.wheel.y;   //deja flottant en SDL3
    return delta;
}

bool SdlGamepad::isButtonPressed(Button button) const {
    const SDL_GamepadButton raw = _buttons.at(button);

    return anyEvent(_window._events, SDL_EVENT_GAMEPAD_BUTTON_DOWN,
        [raw](const SDL_Event &event) { return event.gbutton.button == raw; });
}

bool SdlGamepad::isButtonReleased(Button button) const {
    const SDL_GamepadButton raw = _buttons.at(button);

    return anyEvent(_window._events, SDL_EVENT_GAMEPAD_BUTTON_UP,
        [raw](const SDL_Event &event) { return event.gbutton.button == raw; });
}

/** @} */

#endif /* !SDLWINDOW_HPP_ */
