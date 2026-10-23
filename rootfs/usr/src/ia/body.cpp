#include <SDL2/SDL.h>

#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kWidth = 80;
constexpr int kHeight = 80;

bool get_displays(std::vector<SDL_Rect>& displays) {
    const int count = SDL_GetNumVideoDisplays();
    if (count < 0) {
        std::fprintf(stderr, "Impossible de détecter les écrans : %s\n", SDL_GetError());
        return false;
    }

    displays.clear();
    for (int i = 0; i < count; ++i) {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(i, &bounds) != 0) {
            std::fprintf(stderr, "Impossible de lire l'écran %d : %s\n", i, SDL_GetError());
            return false;
        }
        if (bounds.w >= kWidth && bounds.h >= kHeight) {
            displays.push_back(bounds);
        }
    }

    if (displays.empty()) {
        std::fprintf(stderr, "Aucun écran n'est assez grand pour afficher le sprite.\n");
        return false;
    }
    return true;
}

void random_position(const std::vector<SDL_Rect>& displays, int& x, int& y) {
    const SDL_Rect& bounds = displays[static_cast<std::size_t>(std::rand()) % displays.size()];
    x = bounds.x + std::rand() % (bounds.w - kWidth + 1);
    y = bounds.y + std::rand() % (bounds.h - kHeight + 1);
}

bool position_is_visible(const std::vector<SDL_Rect>& displays, int x, int y) {
    for (const SDL_Rect& bounds : displays) {
        if (x >= bounds.x && y >= bounds.y &&
            x + kWidth <= bounds.x + bounds.w &&
            y + kHeight <= bounds.y + bounds.h) {
            return true;
        }
    }
    return false;
}

int step_toward(int from, int to) {
    if (std::abs(to - from) < 3) {
        return to;
    }
    return from + (to > from ? 3 : -3);
}

void fill_ellipse(SDL_Surface* surface, int x, int y, int width, int height, Uint32 color) {
    const float radius_x = width / 2.0F;
    const float radius_y = height / 2.0F;
    const float center_x = x + radius_x;
    const float center_y = y + radius_y;

    for (int row = y; row < y + height; ++row) {
        for (int column = x; column < x + width; ++column) {
            if (column < 0 || row < 0 || column >= surface->w || row >= surface->h) {
                continue;
            }
            const float dx = (column + 0.5F - center_x) / radius_x;
            const float dy = (row + 0.5F - center_y) / radius_y;
            if (dx * dx + dy * dy <= 1.0F) {
                auto* pixels = static_cast<Uint32*>(surface->pixels);
                pixels[row * (surface->pitch / static_cast<int>(sizeof(Uint32))) + column] = color;
            }
        }
    }
}

void draw_pet(SDL_Surface* sprite, int mouse_x, int mouse_y) {
    SDL_FillRect(sprite, nullptr, 0x00000000);
    fill_ellipse(sprite, 0, 0, kWidth, kHeight, 0xFF50A0FF);
    fill_ellipse(sprite, 22, 25, 14, 17, 0xFFFFFFFF);
    fill_ellipse(sprite, 44, 25, 14, 17, 0xFFFFFFFF);

    const auto draw_pupil = [sprite, mouse_x, mouse_y](int center_x, int center_y) {
        float dx = static_cast<float>(mouse_x - center_x);
        float dy = static_cast<float>(mouse_y - center_y);
        const float distance = std::sqrt(dx * dx + dy * dy);
        constexpr float max_offset = 4.0F;
        if (distance > max_offset) {
            const float scale = max_offset / distance;
            dx *= scale;
            dy *= scale;
        }
        fill_ellipse(sprite,
                     center_x + static_cast<int>(dx) - 3,
                     center_y + static_cast<int>(dy) - 4,
                     6, 8, 0xFF000000);
    };

    draw_pupil(29, 33);
    draw_pupil(51, 33);
}

}  // namespace

std::string askRobot(const std::string& message) {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return "Erreur : impossible de créer la connexion au robot.";
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
        close(socket_fd);
        return "Erreur : adresse IP invalide.";
    }

    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        close(socket_fd);
        return "Erreur : le robot n'est pas démarré.";
    }

    std::size_t sent = 0;
    while (sent < message.size()) {
        const ssize_t result = send(socket_fd, message.data() + sent, message.size() - sent, 0);
        if (result <= 0) {
            close(socket_fd);
            return "Erreur : impossible d'envoyer le message au robot.";
        }
        sent += static_cast<std::size_t>(result);
    }

    char buffer[4096];
    const ssize_t received = recv(socket_fd, buffer, sizeof(buffer), 0);
    close(socket_fd);
    if (received <= 0) {
        return "Erreur : aucune réponse du robot.";
    }
    return std::string(buffer, static_cast<std::size_t>(received));
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "Impossible d'initialiser SDL : %s\n", SDL_GetError());
        return 1;
    }
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::vector<SDL_Rect> displays;
    if (!get_displays(displays)) {
        SDL_Quit();
        return 1;
    }

    SDL_Surface* sprite = SDL_CreateRGBSurfaceWithFormat(
        0, kWidth, kHeight, 32, SDL_PIXELFORMAT_ARGB8888);
    if (sprite == nullptr) {
        std::fprintf(stderr, "Impossible de créer le sprite : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    draw_pet(sprite, kWidth / 2, kHeight / 2);

    int x, y, target_x, target_y;
    random_position(displays, x, y);
    random_position(displays, target_x, target_y);

    SDL_Window* window = SDL_CreateShapedWindow(
        "pet", x, y, kWidth, kHeight, SDL_WINDOW_ALWAYS_ON_TOP);
    if (window != nullptr) {
        SDL_WindowShapeMode shape_mode{};
        shape_mode.mode = ShapeModeDefault;
        shape_mode.parameters.binarizationCutoff = 1;
        if (SDL_SetWindowShape(window, sprite, &shape_mode) != 0) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
    }
    if (window == nullptr) {
        window = SDL_CreateWindow(
            "pet", x, y, kWidth, kHeight,
            SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    }
    if (window == nullptr) {
        std::fprintf(stderr, "Impossible de créer la fenêtre : %s\n", SDL_GetError());
        SDL_FreeSurface(sprite);
        SDL_Quit();
        return 1;
    }

    bool chatting = false;
    bool dragging = false;
    bool drag_moved = false;
    int tick = 0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_DISPLAYEVENT) {
                if (!get_displays(displays)) {
                    running = false;
                    break;
                }
                if (!position_is_visible(displays, x, y)) {
                    random_position(displays, x, y);
                    SDL_SetWindowPosition(window, x, y);
                }
                random_position(displays, target_x, target_y);
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT) {
                dragging = true;
                drag_moved = false;
                SDL_GetWindowPosition(window, &x, &y);
            } else if (event.type == SDL_MOUSEBUTTONUP &&
                       event.button.button == SDL_BUTTON_LEFT) {
                if (dragging && !drag_moved) {
                    chatting = !chatting;
                }
                dragging = false;
            } else if (event.type == SDL_MOUSEMOTION && dragging) {
                if (std::abs(event.motion.xrel) > 0 || std::abs(event.motion.yrel) > 0) {
                    drag_moved = true;
                    x += event.motion.xrel;
                    y += event.motion.yrel;
                    SDL_SetWindowPosition(window, x, y);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_RIGHT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN &&
                       (event.key.keysym.mod & KMOD_CTRL) != 0) {
                if (event.key.keysym.sym == SDLK_c) {
                    chatting = !chatting;
                } else if (event.key.keysym.sym == SDLK_r) {
                    chatting = false;
                }
            }
        }

        if (!running) {
            break;
        }

        if (!chatting && !dragging) {
            if (x == target_x && y == target_y) {
                random_position(displays, target_x, target_y);
            }
            x = step_toward(x, target_x);
            y = step_toward(y, target_y);
            SDL_SetWindowPosition(window, x, y);
        }
        if (++tick % 100 == 0) {
            SDL_RaiseWindow(window);
        }

        int mouse_x = 0;
        int mouse_y = 0;
        int window_x = 0;
        int window_y = 0;
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
        SDL_GetWindowPosition(window, &window_x, &window_y);
        mouse_x -= window_x;
        mouse_y -= window_y;
        mouse_x = std::max(0, std::min(mouse_x, kWidth));
        mouse_y = std::max(0, std::min(mouse_y, kHeight));
        draw_pet(sprite, mouse_x, mouse_y);

        SDL_Surface* window_surface = SDL_GetWindowSurface(window);
        if (window_surface != nullptr) {
            SDL_FillRect(window_surface, nullptr, 0);
            SDL_BlitSurface(sprite, nullptr, window_surface, nullptr);
            if (SDL_UpdateWindowSurface(window) != 0) {
                std::fprintf(stderr, "Impossible de mettre à jour la fenêtre : %s\n", SDL_GetError());
                running = false;
            }
        } else {
            std::fprintf(stderr, "Impossible d'obtenir la surface de la fenêtre : %s\n", SDL_GetError());
            running = false;
        }
        SDL_Delay(30);
    }

    SDL_FreeSurface(sprite);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
