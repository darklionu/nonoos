#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>

#define W 80
#define H 80

static bool get_displays(std::vector<SDL_Rect> &displays) {
    const int count = SDL_GetNumVideoDisplays();
    if (count < 0) {
        fprintf(stderr, "Impossible de détecter les écrans : %s\n", SDL_GetError());
        return false;
    }

    displays.clear();
    for (int i = 0; i < count; ++i) {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(i, &bounds) != 0) {
            fprintf(stderr, "Impossible de lire l'écran %d : %s\n", i, SDL_GetError());
            return false;
        }
        if (bounds.w >= W && bounds.h >= H)
            displays.push_back(bounds);
    }

    if (displays.empty()) {
        fprintf(stderr, "Aucun écran n'est assez grand pour afficher le sprite.\n");
        return false;
    }
    return true;
}

static void random_position(const std::vector<SDL_Rect> &displays, int &x, int &y) {
    const SDL_Rect &bounds = displays[rand() % displays.size()];
    x = bounds.x + rand() % (bounds.w - W + 1);
    y = bounds.y + rand() % (bounds.h - H + 1);
}
bool dragging = false;
int drag_offset_x = 0, drag_offset_y = 0;
if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
    dragging = true;
    drag_offset_x = e.button.x;
    drag_offset_y = e.button.y;
} else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
    dragging = false;
} else if (e.type == SDL_MOUSEMOTION && dragging) {
    x = e.motion.x - drag_offset_x;
    y = e.motion.y - drag_offset_y;
    SDL_SetWindowPosition(win, x, y);
}

static bool position_is_visible(const std::vector<SDL_Rect> &displays, int x, int y) {
    for (const SDL_Rect &bounds : displays) {
        if (x >= bounds.x && y >= bounds.y &&
            x + W <= bounds.x + bounds.w && y + H <= bounds.y + bounds.h)
            return true;
    }
    return false;
}

static int step(int from, int to) {          // avance de 3 px vers la cible
    if (abs(to - from) < 3) return to;
    return from + (to > from ? 3 : -3);
}

// Remplit une ellipse dans une surface ARGB (remplace XFillArc)
static void fill_ellipse(SDL_Surface *s, int x, int y, int w, int h, Uint32 color) {
    float rx = w / 2.0f, ry = h / 2.0f, cx = x + rx, cy = y + ry;
    Uint32 *px = (Uint32 *)s->pixels;
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) {
            float dx = (i + 0.5f - cx) / rx, dy = (j + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.0f && i >= 0 && j >= 0 && i < s->w && j < s->h)
                px[j * (s->pitch / 4) + i] = color;
        }
}

int main(int, char **) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Impossible d'initialiser SDL : %s\n", SDL_GetError());
        return 1;
    }
    srand(time(NULL));

    std::vector<SDL_Rect> displays;
    if (!get_displays(displays)) {
        SDL_Quit();
        return 1;
    }

    // Sprite : fond transparent, corps bleu, yeux
    SDL_Surface *sprite = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!sprite) {
        fprintf(stderr, "Impossible de créer le sprite : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_FillRect(sprite, NULL, 0x00000000);
    fill_ellipse(sprite, 0, 0, W, H, 0xFF50A0FF);      // corps
    fill_ellipse(sprite, 22, 25, 14, 17, 0xFFFFFFFF);  // yeux
    fill_ellipse(sprite, 44, 25, 14, 17, 0xFFFFFFFF);
    fill_ellipse(sprite, 26, 30, 6, 8, 0xFF000000);    // pupilles
    fill_ellipse(sprite, 48, 30, 6, 8, 0xFF000000);

    // Fenêtre sans bordure, au-dessus des autres, en forme de sprite
    int x, y, tx, ty;
    random_position(displays, x, y);
    random_position(displays, tx, ty);
    SDL_Window *win = SDL_CreateShapedWindow("pet", x, y, W, H,
                                             SDL_WINDOW_ALWAYS_ON_TOP);
    if (win) {
        SDL_WindowShapeMode mode;
        mode.mode = ShapeModeDefault;               // alpha >= cutoff => visible
        mode.parameters.binarizationCutoff = 1;
        SDL_SetWindowShape(win, sprite, &mode);
    } else {
        // Repli (ex. Wayland natif) : fenêtre rectangulaire sans bordure
        win = SDL_CreateWindow("pet", x, y, W, H,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
        if (!win) {
            fprintf(stderr, "Impossible de créer la fenêtre : %s\n", SDL_GetError());
            SDL_FreeSurface(sprite);
            SDL_Quit();
            return 1;
        }
    }

    int chatting = 0, tick = 0, running = 1;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_DISPLAYEVENT) {
                if (!get_displays(displays)) {
                    running = 0;
                    break;
                }
                if (!position_is_visible(displays, x, y)) {
                    random_position(displays, x, y);
                    SDL_SetWindowPosition(win, x, y);
                }
                random_position(displays, tx, ty);
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    chatting = !chatting;           // clic gauche : ouvrir/fermer le chat
                    if (chatting) {
                        // ici : lancer ta fenêtre de chat
                    }
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    running = 0;                    // clic droit : quitter
                }
            }
        }

        if (!chatting && !dragging) {
            if (x == tx && y == ty) {
                random_position(displays, tx, ty);
            }
            x = step(x, tx);
            y = step(y, ty);
            SDL_SetWindowPosition(win, x, y);
        }
        if (++tick % 100 == 0) SDL_RaiseWindow(win);

        // Redessin
        SDL_Surface *ws = SDL_GetWindowSurface(win);
        if (ws) {
            SDL_FillRect(ws, NULL, 0);
            SDL_BlitSurface(sprite, NULL, ws, NULL);
            SDL_UpdateWindowSurface(win);
        }
        SDL_Delay(30);                              // ~33 images par seconde
    }
    while (SDL_PollEvent(&e)){
        if(e.type == SDL_KEYDOWN){
            running = 0;
        }
        else if ((e.key.keysym.sym == SDLK_c) && (e.key.keysym.mod & KMOD_CTRL)) {
            chatting = !chatting;
            if (chatting) {
                // ouvrir ta fenêtre de chat
            }
        else if ((e.key.keysym.sym == SDLK_r) && (e.key.keysym.mod & KMOD_CTRL)) {
            // ferme la fentre 
        }
    }
    

    SDL_FreeSurface(sprite);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

// Fonction de dessin des yeux qui suivent la souris
auto draw_eyes = [&](int mouse_x, int mouse_y) {
    SDL_FillRect(sprite, NULL, 0x00000000);
    fill_ellipse(sprite, 0, 0, W, H, 0xFF50A0FF);      // corps

    // yeux blancs
    fill_ellipse(sprite, 22, 25, 14, 17, 0xFFFFFFFF);
    fill_ellipse(sprite, 44, 25, 14, 17, 0xFFFFFFFF);

    // centre des yeux
    const int left_eye_x = 29;
    const int left_eye_y = 33;
    const int right_eye_x = 51;
    const int right_eye_y = 33;

    auto draw_pupil = [&](int cx, int cy) {
        int dx = mouse_x - cx;
        int dy = mouse_y - cy;
        float dist = sqrtf((float)dx * dx + (float)dy * dy);

        const float max_offset = 4.0f;
        if (dist > 0.0001f) {
            float scale = max_offset / dist;
            if (scale > 1.0f) scale = 1.0f;
            dx = (int)(dx * scale);
            dy = (int)(dy * scale);
        } else {
            dx = 0;
            dy = 0;
        }

        int pupil_x = cx + dx;
        int pupil_y = cy + dy;

        fill_ellipse(sprite, pupil_x - 3, pupil_y - 4, 6, 8, 0xFF000000);
    };

    draw_pupil(left_eye_x, left_eye_y);
    draw_pupil(right_eye_x, right_eye_y);
};

int mouse_x = 0, mouse_y = 0;
SDL_GetMouseState(&mouse_x, &mouse_y);

// Convertit la souris en coordonnées locales du sprite
// Si la souris est en dehors du sprite, on la clamp.
mouse_x = mouse_x < 0 ? 0 : mouse_x;
mouse_y = mouse_y < 0 ? 0 : mouse_y;
mouse_x = mouse_x > W ? W : mouse_x;
mouse_y = mouse_y > H ? H : mouse_y;

// On redessine le sprite avec les pupilles pointées vers la souris
draw_eyes(mouse_x, mouse_y);

std::string askRobot(const std::string& message) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return "Erreur: impossible de se connecter au robot.";
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
        close(sock);
        return "Erreur: adresse IP invalide.";
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        close(sock);
        return "Erreur: le robot n'est pas démarré.";
    }

    send(sock, message.c_str(), message.size(), 0);

    char buffer[4096];
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    std::string response = "Erreur: aucune réponse.";
    if (received > 0) {
        buffer[received] = '\0';
        response = std::string(buffer, received);
    }

    close(sock);
    return response;
}