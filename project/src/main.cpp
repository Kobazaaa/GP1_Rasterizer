//External includes
#ifdef ENABLE_VLD
#include "vld.h"
#endif
#include "SDL.h"
#include "SDL_surface.h"
#undef main

//Standard includes
#include <iostream>

//Project includes
#include "Timer.h"
#include "Renderer.h"

using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}
void SetConsoleColor(int textColorCode)
{
	std::cout << "\033[" << textColorCode << "m";
}
void ResetConsoleColor()
{
	std::cout << "\033[0m";
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Rasterizer - **Dereyne Kobe - (2DAE10)**",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();

	// Start Benchmark
	// TODO pTimer->StartBenchmark();

	SetConsoleColor(33);
	std::cout << "===== Shortcuts =====\n";
	std::cout << "F1 - Toggle FPS in Console [OFF/ON]\n";
	std::cout << "F4 - Toggle Depth Buffer Visualization [OFF/ON]\n";
	std::cout << "F5 - Toggle Rotation [ON/OFF]\n";
	std::cout << "F6 - Toggle Normal Map [ON/OFF]\n";
	std::cout << "F7 - Cycle Shading Mode [Combined - Observed Area - Diffuse - Specular]\n";
	std::cout << "F8 - Toggle Wireframes [OFF/ON]\n\n";
	ResetConsoleColor();

	bool displayFPS = false;
	float printTimer = 0.f;
	bool isLooping = true;
	bool takeScreenshot = false;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				if (e.key.keysym.scancode == SDL_SCANCODE_X)
					takeScreenshot = true;
				if (e.key.keysym.scancode == SDL_SCANCODE_F1)
					displayFPS = !displayFPS;
				if (e.key.keysym.scancode == SDL_SCANCODE_F4)
					pRenderer->ToggleDepthBufferVisualization();
				if (e.key.keysym.scancode == SDL_SCANCODE_F5)
					pRenderer->ToggleMeshRotation();
				if (e.key.keysym.scancode == SDL_SCANCODE_F6)
					pRenderer->ToggleNormalMap();
				if (e.key.keysym.scancode == SDL_SCANCODE_F7)
					pRenderer->CycleShadingMode();
				if (e.key.keysym.scancode == SDL_SCANCODE_F8)
					pRenderer->ToggleWireFrames();
				break;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		if(displayFPS)
		{
			printTimer += pTimer->GetElapsed();
			if (printTimer >= 1.f)
			{
				printTimer = 0.f;
				std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
			}
		}

		//Save screenshot after full render
		if (takeScreenshot)
		{
			if (!pRenderer->SaveBufferToImage())
				std::cout << "Screenshot saved!" << std::endl;
			else
				std::cout << "Something went wrong. Screenshot not saved!" << std::endl;
			takeScreenshot = false;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}