#include <SDL2/SDL.h>
#include <iostream>

int main() {
    // Initialise le sous-système vidéo de SDL.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Erreur SDL : " << SDL_GetError() << '\n';
        return 1;
    }

    // Crée une fenêtre de 640 × 400 pixels, centrée à l'écran.
    SDL_Window* window = SDL_CreateWindow(
        "Interface minimale",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 400, SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Erreur fenêtre : " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    // Récupère la surface de dessin de la fenêtre.
    SDL_Surface* screen = SDL_GetWindowSurface(window);
    if (!screen) {
        std::cerr << "Erreur surface : " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Rect bouton{250, 180, 140, 50};
    bool active = false;
    bool quitter = false;
    int codeRetour = 0;

    // Traite les événements jusqu'à la fermeture de la fenêtre.
    while (!quitter) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quitter = true;
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                const int x = event.button.x;
                const int y = event.button.y;

                // Vérifie si le clic se trouve dans le bouton.
                if (x >= bouton.x && x < bouton.x + bouton.w &&
                    y >= bouton.y && y < bouton.y + bouton.h) {
                    active = !active;
                }
            }
        }

        // Dessine le fond, le panneau et le bouton.
        SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, 24, 28, 38));

        SDL_Rect panneau{80, 70, 480, 260};
        SDL_FillRect(screen, &panneau, SDL_MapRGB(screen->format, 42, 48, 62));

        const Uint32 couleurBouton = active
            ? SDL_MapRGB(screen->format, 70, 190, 120)
            : SDL_MapRGB(screen->format, 70, 130, 220);
        SDL_FillRect(screen, &bouton, couleurBouton);

        // Met à jour l'affichage et signale toute erreur.
        if (SDL_UpdateWindowSurface(window) != 0) {
            std::cerr << "Erreur mise à jour : " << SDL_GetError() << '\n';
            codeRetour = 1;
            quitter = true;
        }

        SDL_Delay(16);
    }

    // Libère les ressources SDL.
    SDL_DestroyWindow(window);
    SDL_Quit();
    return codeRetour;
}