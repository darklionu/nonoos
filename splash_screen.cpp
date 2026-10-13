#include <SDL2/SDL.h>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <iostream>

#define W 80
#define H 80

static void fill_ellipse(SDL_Surface *surface, int x, int y, int w, int h, Uint32 color) {
    const float rx = w / 2.0f;
    const float ry = h / 2.0f;
    const float cx = x + rx;
    const float cy = y + ry;
    Uint32 *pixels = static_cast<Uint32 *>(surface->pixels);

    for (int j = y; j < y + h; ++j) {
        for (int i = x; i < x + w; ++i) {
            if (i < 0 || j < 0 || i >= surface->w || j >= surface->h)
                continue;

            const float dx = (i + 0.5f - cx) / rx;
            const float dy = (j + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.0f)
                pixels[j * (surface->pitch / 4) + i] = color;
        }
    }
}

static SDL_Surface *create_mascot_sprite() {
    SDL_Surface *sprite = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!sprite) {
        std::cerr << "Erreur SDL_CreateRGBSurfaceWithFormat: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_FillRect(sprite, nullptr, 0x00000000);

    fill_ellipse(sprite, 0, 0, W, H, 0xFF50A0FF);
    fill_ellipse(sprite, 22, 25, 14, 17, 0xFFFFFFFF);
    fill_ellipse(sprite, 44, 25, 14, 17, 0xFFFFFFFF);
    fill_ellipse(sprite, 26, 30, 6, 8, 0xFF000000);
    fill_ellipse(sprite, 48, 30, 6, 8, 0xFF000000);

    return sprite;
}

static void draw_progress_bar(SDL_Surface *screen, int x, int y, int w, int h, int progress) {
    SDL_Rect bg = { x, y, w, h };
    SDL_Rect fill = { x, y, std::max(0, std::min(progress, w)), h };

    SDL_FillRect(screen, &bg, SDL_MapRGB(screen->format, 50, 50, 60));
    SDL_FillRect(screen, &fill, SDL_MapRGB(screen->format, 75, 210, 120));
}

static void draw_loading_screen(SDL_Window *window, SDL_Surface *sprite, int progress, int frame) {
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    if (!screen)
        return;

    SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, 12, 16, 24));

    const int bob = (frame / 8) % 2 ? 8 : 0;
    SDL_Rect mascotRect = { 120, 30 + bob, W, H };
    SDL_BlitSurface(sprite, nullptr, screen, &mascotRect);

    SDL_Rect title = { 80, 120, 0, 0 };
    SDL_Rect subtitle = { 90, 145, 0, 0 };
    // On n'utilise pas SDL_ttf ici pour rester autonome.
    // On affiche simplement une zone colorée et une barre de progression.

    draw_progress_bar(screen, 60, 175, 220, 18, progress);

    SDL_Rect glow = { 60 + std::max(0, std::min(progress, 220)) - 4, 172, 8, 8 };
    SDL_FillRect(screen, &glow, SDL_MapRGB(screen->format, 255, 255, 255));

    SDL_UpdateWindowSurface(window);
}

static bool is_ready(int progress) {
    // Remplace cette condition par les vrais checks d'initialisation.
    return progress >= 100;
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Impossible d'initialiser SDL : " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Surface *sprite = create_mascot_sprite();
    if (!sprite) {
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Boot",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        340,
        260,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
    );

    if (!window) {
        std::cerr << "Impossible de créer la fenêtre : " << SDL_GetError() << std::endl;
        SDL_FreeSurface(sprite);
        SDL_Quit();
        return 1;
    }

    int progress = 0;
    int frame = 0;

    while (!is_ready(progress)) {
        draw_loading_screen(window, sprite, progress, frame);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_DestroyWindow(window);
                SDL_FreeSurface(sprite);
                SDL_Quit();
                return 0;
            }
        }

        SDL_Delay(30);
        progress += 2;
        if (progress > 100)
            progress = 100;
        ++frame;
    }

    draw_loading_screen(window, sprite, 100, frame);
    SDL_Delay(400);

    SDL_DestroyWindow(window);
    SDL_FreeSurface(sprite);
    SDL_Quit();
    return 0;
}
