# SDL3 et ses trois satellites, par pkg-config.
#
# Pas de FetchContent ici, contrairement a sfml : SDL se compile contre les
# frameworks du systeme (Cocoa, CoreAudio, Metal) et l'installation par le
# gestionnaire de paquets est celle que les satellites - ttf, image, mixer -
# ont deja trouvee. Les rebatir separement donnerait deux SDL dans le meme
# processus, chacun avec son etat global.
#
#   brew install sdl3 sdl3_ttf sdl3_image sdl3_mixer
#   apt  install libsdl3-dev libsdl3-{ttf,image,mixer}-dev
#
# Les noms pkg-config ont change entre 2 et 3 : SDL2_ttf est devenu
# sdl3-ttf, tout en minuscules et avec un tiret.

set(name sdl3)

if (NOT TARGET Sdl3::All)
  find_package(PkgConfig REQUIRED)

  pkg_check_modules(SDL3       REQUIRED IMPORTED_TARGET sdl3)
  pkg_check_modules(SDL3_TTF   REQUIRED IMPORTED_TARGET sdl3-ttf)
  pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)
  pkg_check_modules(SDL3_MIXER REQUIRED IMPORTED_TARGET sdl3-mixer)

  add_library(Sdl3::All INTERFACE IMPORTED)
  set_target_properties(Sdl3::All PROPERTIES
    INTERFACE_LINK_LIBRARIES "PkgConfig::SDL3;PkgConfig::SDL3_TTF;PkgConfig::SDL3_IMAGE;PkgConfig::SDL3_MIXER"
    # sans ca, SDL.h remplace main() par SDL_main() et le point d'entree
    # d'un test ou d'un executable hote ne se lie plus
    INTERFACE_COMPILE_DEFINITIONS "SDL_MAIN_HANDLED"
  )
endif()
