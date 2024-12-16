#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 640
#endif
#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 480
#endif

// NOTE: FORCED SCREEN SIZE
const uint16_t SCREEN_WIDTH = 64;
const uint16_t SCREEN_HEIGHT = 32;

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT]; // dont give it data for cool effect
    bool running = true;
    while (running != false) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                goto end;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderCopyEx(
            renderer,
            texture,
            NULL,
            NULL,
            0.0,
            NULL,
            SDL_FLIP_VERTICAL);
        SDL_RenderPresent(renderer);
    }
end:

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

