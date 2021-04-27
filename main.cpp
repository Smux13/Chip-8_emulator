#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <filesystem>
#include <algorithm>

#define NDEBUG

double frameDelay{};

#include "Chip-8.h"
#include "sdl_file_chooser.h"
#include "DoubleAsker.h"
#include "sound.h"


int main()
{
    #ifdef NDEBUG
        // Disconnect the stream buffer from cout.
        std::cout.rdbuf(nullptr);
    #endif

    FileChooser fileChooser{"./roms", "ch8"};
    std::string romFilename{fileChooser.get()};
    
    // If the user canceled the file selection, quit.
    if (romFilename.size() == 0)
        return 0;
    
    DoubleAsker doubleAskerDialog;
    double emulationSpeed{doubleAskerDialog.get()};
    
    // If the user canceled the entering of the emulation speed, quit.
    if (emulationSpeed <= 0)
        return 0;

    std::cout << "Filename: " << romFilename << std::endl;
    std::cout << "Emulation speed: " << emulationSpeed << std::endl;

    Chip8 chip8{romFilename};
    chip8.whenWindowResized(64*20, 32*20);
    
    std::cout << std::hex;
    
    frameDelay = 1000.0/500/emulationSpeed;
    
    bool isRunning{true};
    bool isPaused{false};
    bool wasPaused{false};
    bool isSteppingMode{false};
    bool shouldStep{false}; // no effect when not in stepping mode
    double renderUpdateCountdown{0}; // Decremented and when 0, the renderer is updated
    while (isRunning && !chip8.hasExited())
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            isPaused = !isPaused;
                            isSteppingMode = false;
                            break;
                        case SDLK_F12:
                            isRunning = false;
                            break;
                        case SDLK_F11:
                            chip8.toggleFullscreen();
                            break;
                        case SDLK_F10:
                            chip8.toggleDebugMode();
                            break;
                        case SDLK_F9:
                            chip8.toggleCursor();
                            break;
                        case SDLK_F6:
                            shouldStep = true;
                            break;
                        case SDLK_F5:
                            isSteppingMode = !isSteppingMode;
                            isPaused = false;
                            break;
                    }
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.windowID == chip8.getWindowID())
                    {
                        switch (event.window.event)
                        {
                        case SDL_WINDOWEVENT_RESIZED:
                            chip8.whenWindowResized(event.window.data1, event.window.data2);
                            continue;
                            break;

                        case SDL_WINDOWEVENT_CLOSE:
                            isRunning = false;
                            continue;
                            break;
                        }
                    }
                    break;
            }
        }

        if (isPaused || (isSteppingMode && !shouldStep))
        {
            wasPaused = true;

            chip8.renderFrameBuffer();
            chip8.renderDebugInfoIfInDebugMode();

            if (isPaused)
                chip8.setPaused();
            else
                chip8.setDebugTitle();

            chip8.updateRenderer();

            // If paused, slow down the cycle to save processing power
            if (isPaused)
                SDL_Delay(frameDelay*500);
            else
                SDL_Delay(frameDelay);

            // If paused or the stepping key was not pressed, don't execute instruction
            if (isPaused || !shouldStep)
                continue;
        }

        if (wasPaused)
        {
            // To make the pixels light again.
            chip8.renderFrameBuffer();

            wasPaused = false;

            // We don't need a redraw for a while
            renderUpdateCountdown = 16.67;
        }

        chip8.clearLastRegisterOperationFlags();
        chip8.clearIsReadingKeyStateFlag();

        chip8.emulateCycle();
        
        // Mark that we executed an instruction since the last step
        shouldStep = false;

        if (chip8.getRenderFlag() || renderUpdateCountdown <= 0)
        {
            chip8.renderFrameBuffer();
            renderUpdateCountdown = 16.67;
        }
        
        chip8.renderDebugInfoIfInDebugMode();
        chip8.updateRenderer();

        SDL_Delay(frameDelay);

        renderUpdateCountdown -= frameDelay;
    }

    chip8.deinit();
}
