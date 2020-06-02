#ifndef __RSDL_H__
#define __RSDL_H__

#include <SDL2/SDL.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
using namespace std;

class RGB {
public:
	RGB(int r, int g, int b) {
		if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
			throw "Invalid RGB color!";
		red = r;
		green = g;
		blue = b;
	}
	int red;
	int green;
	int blue;
};

const RGB WHITE(255, 255, 255);
const RGB YELLOW(255, 255, 0);
const RGB MAGENTA(255, 0, 255);
const RGB RED(255, 0, 0);
const RGB CYAN(0, 255, 255);
const RGB GREEN(0, 255, 0);
const RGB BLUE(0, 0, 255);
const RGB BLACK(0, 0, 0);

class ImageLoad{
public:
	ImageLoad(string add, SDL_Surface* surface) : address(add), sdl_surface(surface) {};
	string address;
	SDL_Surface* sdl_surface;
};
class window {
public:
	window(int width = 640, int height = 480, const string& title = "SDL_tutorial");
	~window();
	void draw_point(int x, int y, RGB color);
	void draw_line(int x1, int y1, int x2, int y2, RGB color);
	void draw_rect(int x, int y, int w, int h, RGB color);
	void fill_rect(int x, int y, int w, int h, RGB color);
	void draw_rect(SDL_Rect r, RGB color);
	void fill_rect(SDL_Rect r, RGB color);
	void draw_bmp(string filename, SDL_Rect* src,  SDL_Rect* dst);
	void draw_transparent_bmp(string filename, int x, int y, RGB color);
	void draw_transparent_bmp(string filename, SDL_Rect* src, SDL_Rect* dst, RGB color);
	void draw_circle(int x, int y, int r, RGB color);
	void fill_circle(int x, int y, int r, RGB color);
	SDL_Surface* find_in_cache(string add);
	void update_screen();
private:
	SDL_Window* win;	
	SDL_Renderer* renderer;
	SDL_Surface* surface;
	vector<ImageLoad*> bmp_cache;
	void set_color(RGB color);
	void dump_err() { cout << SDL_GetError() << endl; }

	static int instance_count;
	static void init();
};


#define HANDLE(A)   SDL_Event e; while (SDL_PollEvent(&e) != 0) { A }
//#define QUIT(Code) if (e.type == SDL_QUIT) { Code ; }
#define LCLICK(Code) if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) { int mouse_x = e.button.x; int mouse_y = e.button.y; Code ; }
#define RCLICK(Code) if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) { int mouse_x = e.button.x; int mouse_y = e.button.y; Code ; }
#define KEY_PRESS(Char, Code) if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_##Char) { Code ; }
#define LRELEASE(Code) if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) { int mouse_x = e.button.x; int mouse_y = e.button.y; Code ; }
#define RRELEASE(Code) if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT) { int mouse_x = e.button.x; int mouse_y = e.button.y; Code ; }

#define DELAY(Millis) SDL_Delay(Millis)

#endif
