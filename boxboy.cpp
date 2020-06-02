//FEATURES
/*
	1- lasers are supported . (the description of how to add lasers is listed in the section below)
	2- boxboy can move around with created boxes. (so for example he can even use them to create hooks and climb up the walls)
	3- boxboy movement animation is enhanced. (the original pictures didnt turn out well, trust me ! )
	4- boxboy can move created sugars after tossing them.
	5- in box creation mode, the sugars that are about to be destroyed will start blinking. 
	6- the map doesn't have to be enclosed in blocks. if boxboy falls out of the map, it dies ; if sugars fall out , they get destroyed.
	7- 4 permade map (level_1 through level_4) are in the project's directory. you can use them. also feel free to create your own maps !
*/

// ADDING LASERS:
/*
	a laser is noted by one of the four characters 'l', 'u', 'r' or 'd' that represent respectively the first letter of words :
	Left, Up, Right or Down. a "left" laser only shoots towards its left side, an "up" laser only towards up side and so on.
*/

// CONTROLS: 
/*
*	1- arrow keys for movement
*	2- space for jumping
*	3- 'x' keydown to enter box creation mode.
*	4- 'x' keyup   to exit  box creation mode ( if possible ).
*	5- arrow keys while in box creation mode to create boxes
*	6- 'z' to throw created boxes to left, and 'c' to throw to right
*	7- space while in box creation mode or while stuck in the air to travel to last box
*	8- 'v' to open the door
*/


// list of future upgrades :  
/*
*       shutters
*		flashing crowns
*		more maps
*		advanced map reader (no need for width-height / double character for shutters)
*/


/*************************************************************************************************************************************/
/********************************************************the main code****************************************************************/
/*************************************************************************************************************************************/

#include "rsdl.h"
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <iostream>
#include <cstdlib>

using namespace std;


//the size of a cube
#define SIZE 60

#define WINDOW_WIDTH	900	
#define WINDOW_HEIGHT	600

#define ROW_SIZE	(	(WINDOW_WIDTH)	/	(SIZE)	)
#define COL_SIZE	(	(WINDOW_HEIGHT)	/	(SIZE)	)

#define IMG_DIR "images/"

//colors
#define BLOCK_COLOR BLACK
#define BACKGROUND_COLOR RGB(232,232,232) //gray-ish
#define TRANSPARENT_COLOR CYAN

//physics (NOTE : these values don't have float precision. use integers)
#define V_X (5)
#define V_Y (-11) // -11
#define V_Y_SUGAR (-7)
#define V_X_SUGAR (3)
#define GRAVITATIONAL_ACCELERATION +1 //+1

//animtaion latencies
#define SUGAR_FLASHING_LATENCY 3
#define RUNNING_LATENCY 4
#define LASER_BLIT_LATENCY 2

//collision type
#define NO_COLLISION 0
#define HORIZONTAL 1
#define VERTICAL 2
#define HORIZONTAL_AND_VERTICAL 3

//intersection type
#define IN -1
#define OUT -2

//game states
#define QUIT 0
#define WIN 1
#define DEATH 2
#define CONTINUE 3


#define N_ASCII_CHARS 256

//directions
#define LEFT	0
#define UP		1
#define RIGHT	2
#define DOWN	3

//boxboy states
#define STANDING	0
#define RUNNING		1
#define MID_AIR		2
#define BOXING		3
#define STUCK_ON_AIR 4

//sugar types
#define OLD 0
#define FRESH 1

//sugar states
#define NORMAL		0
#define FLASHING	1

//animation frame names
#define READY_TO_RUN 8


/*************************************************************************************************************************************/
/********************************************************   structs  *****************************************************************/
/*************************************************************************************************************************************/

typedef SDL_Rect Camera;

struct Precise_Rect
{
	double x,y,w,h;
};

struct Block
{
	SDL_Rect rect; 
	bool update_due;
	string img_name;
};

struct BoxBoy
{
	int vx,vy;
	
	int state;
	int direction;

	int running_frame_id;

	bool attached;
	SDL_Rect rect;
	bool update_due;
	string img_name;

};

struct Door
{
	SDL_Rect rect;
	bool update_due;
	string img_name;
};

struct Crown
{
	SDL_Rect rect;
	bool update_due;
	string img_name;

};

struct Trap
{
	SDL_Rect rect;
	bool update_due;
	string img_name;
};

struct Sugar
{
	SDL_Rect rect;
	int vx,vy;

	string img_name;
	bool update_due;
};

struct Laser
{
	int direction;
	SDL_Rect base;
	int frame_offset,blit_latency;
};

struct Gamedata
{
	BoxBoy	bb;
	Door door;

	vector <Sugar> sugars;
	vector <Block> blocks;
	vector <Crown> crowns;
	vector <Trap> traps;
	vector <Laser> lasers;

	window* game_window;
	SDL_Rect map_rect;

	string world_name;
	int sugar_limit;
	int rewards_count;
	int score;

	Camera camera;

	int state;
	int sugar_types,sugar_states,sugar_flashing_frames;

	int map_width,map_height;

	bool keysheld[N_ASCII_CHARS];
};

/*************************************************************************************************************************************/
/******************************************************utility functions**************************************************************/
/*************************************************************************************************************************************/


inline double my_abs(double val)
{
	return (val>=0) ? val : (-1)*val ;
}
inline int my_abs(int val)
{
	return (val>=0) ? val : (-1)*val ;
}

SDL_Rect cast_to_SDL(Precise_Rect r)
{
	SDL_Rect result;

	double floor_x,floor_y;

	floor_x=(int)r.x;
	floor_y=(int)r.y;

	if ( floor_x + 1 - r.x < 0.001)
	{
		result.x=(int)(floor_x+1);
	}
	else
	{
		result.x=(int)(floor_x);
	}

	if (floor_y + 1 - r.y < 0.001)
	{
		result.y=(int)(floor_y+1);
	}
	else
	{
		result.y=(int)(floor_y);
	}

	result.w=(int)r.w;
	result.h=(int)r.h;

	return result;
}
Precise_Rect cast_to_precise(SDL_Rect r)
{
	Precise_Rect result;

	result.x=r.x;
	result.y=r.y;
	result.w=r.w;
	result.h=r.h;

	return result;
}

inline bool mutually_exclusive(SDL_Rect r1, SDL_Rect r2)
{
	return (r1.x >= r2.x+r2.w || r1.x+r1.w <= r2.x || r1.y >= r2.y+r2.h || r1.y+r1.h <= r2.y);
}

inline bool r1_completely_in_r2(SDL_Rect r1, SDL_Rect r2)
{
	return r1.x >= r2.x && r1.x+r1.w <= r2.x+r2.w && r1.y >= r2.y && r1.y+r1.h <= r2.y+r2.h ;
}

bool collide(SDL_Rect r1, SDL_Rect r2)
{
	return !mutually_exclusive(r1,r2);
}

bool collide(SDL_Rect what,const vector <SDL_Rect>& barriers)
{
	for (unsigned int i=0;i<barriers.size();i++)
	{
		if (collide(what,barriers[i]))
			return true;
	}
	return false;
}

bool collide(SDL_Rect mover,int vx,int vy,SDL_Rect barrier)
{
	mover.x+=vx;
	mover.y+=vy;
	return collide (mover,barrier);
}

bool collide(SDL_Rect mover,int vx,int vy,const vector <SDL_Rect>& barriers)
{
	mover.x+=vx;
	mover.y+=vy;

	for (unsigned int i=0;i<barriers.size();i++)
	{
		if (collide (mover,barriers[i]))
			return true;
	}
	return false;
}

SDL_Rect create_std_rect_from_coords(int i,int j)
{
	SDL_Rect result;

	result.x=j*SIZE;
	result.y=i*SIZE;

	result.w=result.h=SIZE;

	return result;
}

vector <SDL_Rect> get_block_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_sugar_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		result.push_back(gamedata.sugars[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_trap_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		result.push_back(gamedata.traps[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_trap_and_block_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		result.push_back(gamedata.traps[i].rect);
	}

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}


	return result;
}

vector <SDL_Rect> get_block_and_sugar_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		result.push_back(gamedata.sugars[i].rect);
	}

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_block_and_sugar_and_trap_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		result.push_back(gamedata.sugars[i].rect);
	}

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		result.push_back(gamedata.traps[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_block_and_laser_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.lasers.size();i++)
	{
		result.push_back(gamedata.lasers[i].base);
	}

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}

	return result;
}

vector <SDL_Rect> get_trap_and_block_and_laser_rects(const Gamedata& gamedata)
{
	vector <SDL_Rect> result;

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		result.push_back(gamedata.traps[i].rect);
	}

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		result.push_back(gamedata.blocks[i].rect);
	}

	for (unsigned int i=0;i<gamedata.lasers.size();i++)
	{
		result.push_back(gamedata.lasers[i].base);
	}


	return result;
}

vector <SDL_Rect> get_laser_barriers(const Gamedata& gamedata)
{
	return get_block_and_sugar_and_trap_rects(gamedata);
}

inline vector <SDL_Rect> get_boxboy_barriers(const Gamedata& gamedata)
{
	return get_block_and_laser_rects(gamedata);
}

vector <SDL_Rect> get_effective_laser_path(const Gamedata& gamedata, int laser_id)
{
	Laser laser=gamedata.lasers[laser_id];

	int dx=0,dy=0;

	switch (laser.direction)
	{
	case LEFT:
		dx=-60;
		break;

	case RIGHT:
		dx=+60;
		break;

	case UP:
		dy=-60;
		break;

	case DOWN:
		dy=+60;
		break;

	}

	SDL_Rect base= laser.base;

	if (laser.direction == UP || laser.direction == DOWN)
	{
		base.w=18;
		base.x+=21;
	}

	if (laser.direction == LEFT || laser.direction == RIGHT)
	{
		base.h=18;
		base.y+=21;
	}

	vector <SDL_Rect> laser_barriers=get_laser_barriers(gamedata);
	vector <SDL_Rect> result;
	
	bool pushed_atleast_one_rect=false;

	while (true)
	{
		base.x+=dx;
		base.y+=dy;

		if (!r1_completely_in_r2(base,gamedata.map_rect))
		{
			if (pushed_atleast_one_rect)
			{
				return result;
			}
			else
			{
				continue;
			}	
		}

		if (collide(base,laser_barriers))
		{
			result.push_back(base);
			return result;
		}

		result.push_back(base);
		pushed_atleast_one_rect=true;
	}

	return result;

}

vector <SDL_Rect> get_full_laser_path(const Gamedata& gamedata, int laser_id)
{
	Laser laser=gamedata.lasers[laser_id];

	int dx=0,dy=0;

	switch (laser.direction)
	{
	case LEFT:
		dx=-60;
		break;

	case RIGHT:
		dx=+60;
		break;

	case UP:
		dy=-60;
		break;

	case DOWN:
		dy=+60;
		break;

	}

	SDL_Rect base= laser.base;

	if (laser.direction == UP || laser.direction == DOWN)
	{
		base.w=18;
		base.x+=21;
	}

	if (laser.direction == LEFT || laser.direction == RIGHT)
	{
		base.h=18;
		base.y+=21;
	}

	vector <SDL_Rect> blocks=get_trap_and_block_rects(gamedata);
	vector <SDL_Rect> result;
	
	bool pushed_atleast_one_rect=false;
	while (true)
	{
		base.x+=dx;
		base.y+=dy;

		if (!r1_completely_in_r2(base,gamedata.map_rect))
		{
			if (pushed_atleast_one_rect)
			{
				return result;
			}
			else
			{
				continue;
			}	
		}

		if (collide(base,blocks) )
		{
			return result;
		}

		result.push_back(base);
		pushed_atleast_one_rect=true;
	}

	return result;

}

vector <SDL_Rect> get_effective_laser_paths(const Gamedata& gamedata)
{
	vector <SDL_Rect> result,temp;

	for (unsigned int i=0;i<gamedata.lasers.size();i++)
	{
		temp=get_effective_laser_path(gamedata,i);
		result.insert(result.end(),temp.begin(),temp.end());
	}

	return result;
}

inline vector <SDL_Rect> get_boxboy_killers(const Gamedata& gamedata)
{
	vector <SDL_Rect> result = get_trap_rects(gamedata);
	
	vector <SDL_Rect> lasers = get_effective_laser_paths(gamedata);

	result.insert(result.end(),lasers.begin(),lasers.end());

	return result;
}

vector <SDL_Rect> get_sugar_barriers(const Gamedata& gamedata)
{
	vector <SDL_Rect> result=get_trap_and_block_and_laser_rects(gamedata);

	//if boxboy is not boxing but has some sugars attached , then boxboy is not a barrier for those sugars
	if (!(gamedata.bb.attached && gamedata.bb.state != BOXING))
	{
		result.push_back(gamedata.bb.rect);
	}
	
	return result;
}

SDL_Rect create_bb_rect_from_coords(int i,int j)
{
	SDL_Rect result;

	result.x=j*SIZE;
	result.y=i*SIZE-15;

	result.w=SIZE;
	result.h=75;

	return result;
}

Block create_block_from_coords(int i,int j)
{
	Block result;

	result.rect=create_std_rect_from_coords(i,j);

	result.update_due=true;

	return result;
}

Crown create_crown_from_coords(int i,int j)
{
	Crown result;

	result.rect=create_std_rect_from_coords(i,j);

	result.update_due=true;

	return result;

}

Door create_door_from_coords(int i,int j)
{
	Door result;

	result.rect=create_std_rect_from_coords(i,j);

	result.update_due=true;

	return result;

}

BoxBoy create_boxboy_from_coords(int i,int j)
{
	BoxBoy result;

	result.rect=create_bb_rect_from_coords(i,j);
	result.vx=result.vy=0;

	result.update_due=true;

	result.state=STANDING;

	result.direction=RIGHT;

	result.running_frame_id=0;

	result.attached=false;
	

	return result;
}

Trap create_trap_from_coords(int i, int j)
{
	Trap result;

	result.rect=create_std_rect_from_coords(i,j);

	result.update_due=true;

	return result;
}

Laser create_laser_from_coords(int i,int j,int direction)
{
	Laser result;
	result.base= create_std_rect_from_coords(i,j);
	result.direction=direction;

	return result;
}

void queue_for_update(Gamedata& gamedata)
{
	gamedata.bb.update_due= gamedata.door.update_due = true;

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		gamedata.blocks[i].update_due=true;
	}

	for (unsigned int i=0;i<gamedata.crowns.size();i++)
	{
		gamedata.crowns[i].update_due=true;
	}

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		gamedata.traps[i].update_due=true;
	}

}

void image_initializer(Gamedata& gamedata)
{
	gamedata.bb.img_name=  IMG_DIR "right/standing.bmp";

	gamedata.door.img_name= IMG_DIR "door/door.bmp";

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		gamedata.blocks[i].img_name= IMG_DIR "block/block.bmp";
	}

	for (unsigned int i=0;i<gamedata.crowns.size();i++)
	{
		gamedata.crowns[i].img_name= IMG_DIR "crown/crown.bmp";
	}

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		gamedata.traps[i].img_name= IMG_DIR "trap/t3.bmp" ;
	}
}

void init_keysheld(Gamedata& gamedata)
{
	for (unsigned int i=0;i<N_ASCII_CHARS;i++)
	{
		gamedata.keysheld[i]=false;
	}
}

void center_camera(Gamedata& gamedata)
{
	int initial_x= gamedata.camera.x;
	int initial_y= gamedata.camera.y;

	bool map_width_too_small  = gamedata.map_width * SIZE  <= gamedata.camera.w;
	bool map_height_too_small = gamedata.map_height * SIZE  <= gamedata.camera.h;

	if ( ! map_width_too_small )
	{
		gamedata.camera.x= gamedata.bb.rect.x+ (gamedata.bb.rect.w/2) - (gamedata.camera.w/2) ;

		if (gamedata.camera.x < 0)
		{
			gamedata.camera.x=0;
		}
		if (gamedata.camera.x + gamedata.camera.w > gamedata.map_width * SIZE && gamedata.map_width * SIZE > gamedata.camera.w)
		{
			gamedata.camera.x= gamedata.map_width * SIZE - gamedata.camera.w ;
		}

	}
	else
	{
		gamedata.camera.x=0;
	}

	if ( ! map_height_too_small)
	{
		gamedata.camera.y= gamedata.bb.rect.y+ (gamedata.bb.rect.h/2) - (gamedata.camera.h/2) ;

		if (gamedata.camera.y + gamedata.camera.h > gamedata.map_height * SIZE)
		{
			gamedata.camera.y= gamedata.map_height * SIZE - gamedata.camera.h ;
		}

		if (gamedata.camera.y < 0 && gamedata.map_height && SIZE > gamedata.camera.h)
		{
			gamedata.camera.y=0;
		}
	}
	else
	{
		gamedata.camera.y= gamedata.map_height * SIZE - gamedata.camera.h;
	}
	

	//if camera changed, update everything
	if (gamedata.camera.x != initial_x || gamedata.camera.y != initial_y)
	{
		gamedata.game_window->fill_rect(0,0,WINDOW_WIDTH,WINDOW_HEIGHT,BACKGROUND_COLOR);
		queue_for_update(gamedata);
	}

}

void initialize_gamedata(Gamedata& gamedata,window* _game_window)
{
	//window 
	gamedata.game_window=_game_window;
	gamedata.game_window->fill_rect(0,0,WINDOW_WIDTH,WINDOW_HEIGHT,BACKGROUND_COLOR);
	gamedata.map_rect.x=gamedata.map_rect.y=0;
	gamedata.map_rect.h=gamedata.map_height* SIZE;
	gamedata.map_rect.w=gamedata.map_width * SIZE;


	//camera
	SDL_Rect cam = {0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
	gamedata.camera=cam;
	center_camera(gamedata);

	gamedata.state=CONTINUE;

	gamedata.sugar_types=OLD;
	gamedata.sugar_states=NORMAL;
	gamedata.sugar_flashing_frames=0;

	gamedata.score=0;
	
	image_initializer(gamedata);

	queue_for_update(gamedata);

	init_keysheld(gamedata);

	//lasers
	for (unsigned int i=0;i<gamedata.lasers.size();i++)
	{
		gamedata.lasers[i].frame_offset=i%2;
		gamedata.lasers[i].blit_latency= LASER_BLIT_LATENCY;
	}
	
}

void process_map_character(Gamedata& gamedata,char input,int i,int j,bool& bb_found)
{
	switch (input)
			{
				case 'b':
				case 'B':
					gamedata.blocks.push_back(create_block_from_coords(i,j));
					break;

				case 'x':
				case 'X':
					gamedata.crowns.push_back(create_crown_from_coords(i,j));
					break;

				case 'e':
				case 'E':
					gamedata.door=create_door_from_coords(i,j);
					break;

				case 's':
				case 'S':
					bb_found=true;
					gamedata.bb=create_boxboy_from_coords(i,j);
					break;

				case 't':
				case 'T':
					gamedata.traps.push_back(create_trap_from_coords(i,j));
					break;


				case 'w':
				case 'W':
					break;	//do nothing

				case 'l':
					gamedata.lasers.push_back(create_laser_from_coords(i,j,LEFT));
					break;

				case 'r':
					gamedata.lasers.push_back(create_laser_from_coords(i,j,RIGHT));
					break;

				case 'u':
					gamedata.lasers.push_back(create_laser_from_coords(i,j,UP));
					break;

				case 'd':
					gamedata.lasers.push_back(create_laser_from_coords(i,j,DOWN));
					break;

				default:
				cout<<"bad character at i : "<<i<<" j : "<<j<<"   (  "<<input<<"  )   in input reception process"<<endl;
				abort();
			}
}

// void start_music( string file_name)
// {
// 	Mix_Music* music = NULL;

// 	if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 ) 
// 	{
// 		printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() ); 
// 		abort();
// 	}

// 	music = Mix_LoadMUS( file_name.c_str() ); 

// 	if( music == NULL ) 
// 	{ 
// 		printf( "Failed to load music! SDL_mixer Error: %s\n", Mix_GetError() ); 
// 	}

// 	Mix_PlayMusic ( music , -1 );

// }

void load_gamedata(Gamedata& gamedata,window* game_window)
{

	cin>>gamedata.world_name;
	cin.ignore();

	cin>>gamedata.sugar_limit;
	cin.ignore();

	cin>>gamedata.rewards_count;
	cin.ignore();

	int bb_i,bb_j;

	cin>>bb_j>>bb_i;
	cin.ignore();

	int	door_i,door_j;

	cin>>door_j>>door_i;
	cin.ignore();

	
	cin>>gamedata.map_width>>gamedata.map_height;
	cin.ignore();

	char current_char;

	bool bb_found=false;

	for (int i=0;i<gamedata.map_height;i++)
	{
		for (int j=0;j<gamedata.map_width;j++)
		{
			cin>>current_char;

			process_map_character(gamedata,current_char,i,j,bb_found);
		}
		cin.ignore();
	}

	if (!bb_found)
	{
		cerr<<"no boxboy character found in the map"<<endl;
		abort();
	}


	initialize_gamedata(gamedata,game_window);
	// start_music("music/Winding-Down.mp3");
}

void test_show_block(window& w,const Block& block)
{
	w.fill_rect(block.rect.x,block.rect.y,block.rect.w,block.rect.h,BLACK);
}

SDL_Rect part_in_camera(SDL_Rect rect, Camera cam)
{

	SDL_Rect result=rect;

	if (mutually_exclusive(cam,rect))
	{
		result.x=result.y=OUT;
		return rect;
	}

	//else we need to cut the rectangle
	if (cam.x > result.x)
	{
		result.w= ( result.x + result.w - cam.x );
		result.x=cam.x;
	}
	if (cam.x+cam.w-1 < result.x+result.w-1)
	{
		result.w= ( cam.x + cam.w - result.x);
	}

	if (cam.y > result.y)
	{
		result.h= ( result.y + result.h - cam.y );
		result.y=cam.y;
	}
	if (cam.y+cam.h-1 < result.y+result.h-1)
	{
		result.h = ( cam.y + cam.h  - result.y);
	}

	return result;

}

void print_gamedata(const Gamedata& gamedata,window& w)
{
	cout<<"world name : "<<gamedata.world_name<<endl;
	cout<<"sugar limit : "<<gamedata.sugar_limit<<endl;
	cout<<"rewards_count :"<<gamedata.rewards_count<<endl;

	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		test_show_block(w,gamedata.blocks[i]);
	}



	//string world_name;
	//int sugar_limit;
	//int rewards_count;

	//bb stands for BoxBoy
	BoxBoy	bb;

	Door door;

	vector <Block> blocks;
}

inline SDL_Rect rect1_relative_to_rect2(SDL_Rect rect1,SDL_Rect rect2)
{
	rect1.x=rect1.x-rect2.x;
	rect1.y=rect1.y-rect2.y;
	return rect1;
}

void blit_element(Gamedata& gamedata,SDL_Rect position,const string& image_name, bool& update_due)
{
	SDL_Rect position_in_cam=part_in_camera(position,gamedata.camera);

	//if it falls out, show nothing.
	if (position_in_cam.x==OUT)
		return;

	//pic = position in camera
	SDL_Rect pic_relative_to_position = rect1_relative_to_rect2(position_in_cam,position);
	SDL_Rect pic_relative_to_cam= rect1_relative_to_rect2(position_in_cam,gamedata.camera);

	gamedata.game_window->draw_transparent_bmp(image_name,&pic_relative_to_position,&pic_relative_to_cam,TRANSPARENT_COLOR);
}

void blit_element_always(Gamedata& gamedata,SDL_Rect position,const string& image_name)
{
	SDL_Rect position_in_cam=part_in_camera(position,gamedata.camera);

	//if it falls out, show nothing.
	if (position_in_cam.x==OUT)
		return;

	//pic = position in camera
	SDL_Rect pic_relative_to_position = rect1_relative_to_rect2(position_in_cam,position);
	SDL_Rect pic_relative_to_cam= rect1_relative_to_rect2(position_in_cam,gamedata.camera);

	gamedata.game_window->draw_transparent_bmp(image_name,&pic_relative_to_position,&pic_relative_to_cam,TRANSPARENT_COLOR);
}

string int_to_string(int val,int strlen)
{
	if (val==0)
		return string(strlen,'0');

	vector <int> digits;

	while(val>0)
	{
		digits.push_back(val%10);
		val/=10;
	}

	string result;

	for (unsigned int i=0;i< strlen-digits.size();i++)
		result.push_back('0');

	for (int i=digits.size()-1;i>=0;i--)
		result.push_back(digits[i]+48);

	return result;
}

string bb_running_frame(BoxBoy& b)
{
	static int counter=0;
	counter++;

	if (b.running_frame_id==READY_TO_RUN)
	{
		b.running_frame_id=0;
		return "08.bmp";
	}

	int last_id=b.running_frame_id;

	if ( counter >= RUNNING_LATENCY)
	{
		b.running_frame_id++;
		counter=0;
	}
	b.running_frame_id%=8;

	return int_to_string(b.running_frame_id,2) + string(".bmp");
}

string bb_image_name(Gamedata& gamedata)
{
	string result= string(IMG_DIR) + ((gamedata.bb.direction == RIGHT) ? "right/" : "left/") ;

	switch (gamedata.bb.state)
	{
	case  STANDING:
	case  STUCK_ON_AIR:
		result+="standing.bmp";
		break;

	case RUNNING:
		result+= string("running/") + bb_running_frame(gamedata.bb);
		break;

	case MID_AIR:
		result+="jump.bmp";
		break;

	case BOXING:
		result+="make_box.bmp";
		break;

	default:
		cerr<<"unrecognized state for boxboy : "<<gamedata.bb.state<<endl;
		exit(EXIT_FAILURE);
	}

	return result;
}

string sugar_flashing_frame(Gamedata& gamedata)
{
	static int count=-1;
	static bool is_ascending=true;
	count++;

	if (count >= SUGAR_FLASHING_LATENCY)
	{
		count=0;

		if (is_ascending)
		{
			gamedata.sugar_flashing_frames++;
			if (gamedata.sugar_flashing_frames>8)
			{
				gamedata.sugar_flashing_frames--;
				is_ascending=false;
			}
		}
		else
		{
			gamedata.sugar_flashing_frames--;
			if (gamedata.sugar_flashing_frames<0)
			{
				gamedata.sugar_flashing_frames++;
				is_ascending=true;
			}

		}
	}
	
	
	string result=string("flashing/") + int_to_string(gamedata.sugar_flashing_frames,2) + string(".bmp");
	return result;
}

string sugar_image_name(Gamedata& gamedata)
{
	string result= string(IMG_DIR) +string ("sugar/");

	switch (gamedata.sugar_states)
	{
	case NORMAL:
		result += "sugar.bmp";
		break;

	case FLASHING:
		result += sugar_flashing_frame(gamedata);
		break;

	default :
		cerr<<"invalid state "<<gamedata.sugar_states<<" for sugar at sugar_image_name funcion" <<endl;
		break;
	}

	return result;
}

void blit_sugars_always(Gamedata& gamedata)
{
	string sugar_img_name = sugar_image_name(gamedata);

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		SDL_Rect position_in_cam=part_in_camera(gamedata.sugars[i].rect,gamedata.camera);

		//pic = position in camera
		SDL_Rect pic_relative_to_position = rect1_relative_to_rect2(position_in_cam,gamedata.sugars[i].rect);
		SDL_Rect pic_relative_to_cam= rect1_relative_to_rect2(position_in_cam,gamedata.camera);

		gamedata.game_window->draw_transparent_bmp(sugar_img_name , &pic_relative_to_position , &pic_relative_to_cam  , TRANSPARENT_COLOR);
	}
	
}

void blit_boxboy(Gamedata& gamedata)
{
	//retrun boxboy doesn't need updating
	if (!gamedata.bb.update_due)
		return;

	if (gamedata.bb.state != RUNNING)
	{
		gamedata.bb.update_due=false;
	}
	
	SDL_Rect position_in_cam=part_in_camera(gamedata.bb.rect,gamedata.camera);

	//pic = position in camera
	SDL_Rect pic_relative_to_position = rect1_relative_to_rect2(position_in_cam,gamedata.bb.rect);
	SDL_Rect pic_relative_to_cam= rect1_relative_to_rect2(position_in_cam,gamedata.camera);

	gamedata.game_window->draw_transparent_bmp(bb_image_name(gamedata) , &pic_relative_to_position , &pic_relative_to_cam  , TRANSPARENT_COLOR);
}

void blit_boxboy_always(Gamedata& gamedata)
{	
	SDL_Rect position_in_cam=part_in_camera(gamedata.bb.rect,gamedata.camera);

	//pic = position in camera
	SDL_Rect pic_relative_to_position = rect1_relative_to_rect2(position_in_cam,gamedata.bb.rect);
	SDL_Rect pic_relative_to_cam= rect1_relative_to_rect2(position_in_cam,gamedata.camera);

	gamedata.game_window->draw_transparent_bmp(bb_image_name(gamedata) , &pic_relative_to_position , &pic_relative_to_cam  , TRANSPARENT_COLOR);
}

string dir_to_string(int direction)
{
	switch (direction)
	{
	case UP:
		return "up/";
	case DOWN:
		return "down/";
	case LEFT:
		return "left/";
	case RIGHT:
		return "right/";
	default:
		cerr<<"bad direction : "<<direction<<" at dir_to_string function."<<endl;
		return "";
	}
}

void erase(Gamedata& gamedata, SDL_Rect position)
{
	SDL_Rect position_in_cam=part_in_camera(position,gamedata.camera);

	//if it falls out, show nothing.
	if (position_in_cam.x==OUT)
		return;
 
	gamedata.game_window->fill_rect(  rect1_relative_to_rect2(position_in_cam,gamedata.camera)  ,  BACKGROUND_COLOR );
}

inline string get_laser_path_pic_name(int direction, int path_id)
{
	return string(IMG_DIR "laser/") + dir_to_string(direction) + string("path_") + int_to_string(path_id,1) + string(".bmp");
}

inline string get_laser_base_pic_name(int direction)
{
	return string(IMG_DIR "laser/") + dir_to_string(direction) + string("base.bmp") ;
}

void blit_laser_always(Gamedata& gamedata, int laser_index)
{
	Laser* laser=&gamedata.lasers[laser_index];

	vector <SDL_Rect> effective_path= get_effective_laser_path(gamedata,laser_index);
	vector <SDL_Rect> full_path = get_full_laser_path(gamedata,laser_index);
	
	//blit the laser base regardless of speed
	blit_element_always(gamedata,laser->base,get_laser_base_pic_name(laser->direction));

	//watch speed for blitting the laser path
	laser->blit_latency--;
	if (laser->blit_latency == -1)
	{
		laser->blit_latency=LASER_BLIT_LATENCY;

		for (unsigned int i=0;i<full_path.size();i++)
		{
			erase(gamedata,full_path[i]);
		}

		for (unsigned int i=0;i<effective_path.size();i++)
		{
			blit_element_always(gamedata,effective_path[i], get_laser_path_pic_name(laser->direction,(laser->frame_offset+i)%2));
		}


		//increment offset
		laser->frame_offset++;
		laser->frame_offset%=2;
	}

}

void blit_lasers_always(Gamedata& gamedata)
{
	for (unsigned int i=0;i<gamedata.lasers.size();i++)
	{
		blit_laser_always(gamedata,i);
	}
}

void show_all(Gamedata& gamedata)
{
	//show all map elements according to their priority

	//blit all crowns
	for (unsigned int i=0;i<gamedata.crowns.size();i++)
	{
		blit_element_always(gamedata  ,  gamedata.crowns[i].rect  ,  gamedata.crowns[i].img_name);
	}

	//blit door
	blit_element_always(gamedata  ,  gamedata.door.rect  ,  gamedata.door.img_name );

	//blit all traps
	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		blit_element_always(gamedata  ,  gamedata.traps[i].rect  ,  gamedata.traps[i].img_name );
	}

	//blit lasers
	blit_lasers_always(gamedata);

	//blit boxboy
	blit_boxboy_always(gamedata);

	//blit all sugars
	blit_sugars_always(gamedata);

	//blit all blocks
	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		blit_element_always(gamedata  ,  gamedata.blocks[i].rect  ,  gamedata.blocks[i].img_name  );
	}


	gamedata.game_window->update_screen();	

}

inline string direction(BoxBoy b)
{
	return (b.direction==LEFT) ? "left/" : "right/" ;
}

void bb_stop(Gamedata& gamedata)
{
	gamedata.bb.vx=0;
	
	if (gamedata.bb.state != MID_AIR)
	{
		gamedata.bb.img_name= string(IMG_DIR) + direction(gamedata.bb) + string("standing.bmp")  ;
	}

	gamedata.bb.update_due=true;
	
}

void enter_boxing(Gamedata& gamedata)
{
	gamedata.bb.state=BOXING;


	gamedata.sugar_states=FLASHING;
	gamedata.sugar_flashing_frames=-1;

	gamedata.bb.vx=0;

	gamedata.bb.img_name= string(IMG_DIR) + direction(gamedata.bb) + string("make_box.bmp")  ;

	erase(gamedata,gamedata.bb.rect);
	gamedata.bb.rect.y+=15;
	gamedata.bb.rect.h-=15;

	if (gamedata.bb.attached)
	{
		for (unsigned int i=0;i<gamedata.sugars.size();i++)
		{
			erase(gamedata,gamedata.sugars[i].rect);
			gamedata.sugars[i].rect.y+=15;
		}
	}

	gamedata.bb.update_due=true;
}

void move_only_sugars(Gamedata& gamedata, int dx, int dy)
{
	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		erase(gamedata, gamedata.sugars[i].rect );
		gamedata.sugars[i].rect.x += dx;
		gamedata.sugars[i].rect.y += dy;
	}
}

SDL_Rect move_rect(SDL_Rect what,int _vx,int _vy,vector <SDL_Rect>& barriers, bool& collided_vertically, bool& collided_horizontally)
{
	Precise_Rect precise_position=cast_to_precise(what);

	double vx=_vx;
	double vy=_vy;
	int move_n_times;

	//scale vx and vy so that they are not greater than 1
	if (my_abs(_vx) > my_abs(_vy))
	{
		move_n_times = my_abs(_vx);
		vy= vy / my_abs(vx);
		vx= vx / my_abs(vx);
	}
	else
	{
		move_n_times = my_abs(_vy);
		vx = vx / my_abs(vy);
		vy = vy / my_abs(vy);	
	}

	Precise_Rect new_pos=precise_position; // d = double

	collided_horizontally=collided_vertically=false;

	for (int i=0;i<move_n_times;i++)
	{
		if (!collided_horizontally)
		{
			//move horizontally
			new_pos.x+=vx;
			new_pos.y+=0;

			if (collide(cast_to_SDL(new_pos),barriers))
			{
				collided_horizontally=true;
				new_pos.x-=vx;
			}
		}

		if (!collided_vertically)
		{
			//move vertically
			new_pos.x+=0;
			new_pos.y+=vy;

			if (collide(cast_to_SDL(new_pos),barriers))
			{
				collided_vertically=true;
				new_pos.y-=vy;
			}
		}
	}

	return cast_to_SDL(new_pos);
}

bool bb_can_stand_up(Gamedata& gamedata)
{
	if (gamedata.bb.state != BOXING)
		return true;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		if (collide(gamedata.sugars[i].rect,0,-15,get_sugar_barriers(gamedata)))
			return false;
	}
	if (collide(gamedata.bb.rect,0,-15,get_boxboy_barriers(gamedata)))
		return false;

	return true;
}

void quit_boxing(Gamedata& gamedata)
{
	if (bb_can_stand_up(gamedata))
	{
		gamedata.sugar_types=OLD;
		gamedata.sugar_states=NORMAL;

		erase(gamedata,gamedata.bb.rect);
		gamedata.bb.rect.y-=15;
		gamedata.bb.rect.h+=15;

		if (gamedata.bb.attached)
		{
			move_only_sugars(gamedata,0,-15);
		}

		gamedata.bb.state=STANDING;

		gamedata.bb.update_due=true;
	}
}

SDL_Rect find_prev_sugar(Gamedata& gamedata)
{
	SDL_Rect result;

	if (gamedata.sugars.size()==0)
	{
		result.x=result.y=-100;
		result.w=result.h=SIZE;
		return result;
	}

	if (gamedata.sugars.size()==1)
	{
		return gamedata.bb.rect;
	}

	return gamedata.sugars[gamedata.sugars.size()-2].rect;
}

inline bool equals (SDL_Rect r1,SDL_Rect r2)
{
	return r1.x==r2.x && r1.y==r2.y && r1.w == r2.w && r1.h == r2.h;
}

SDL_Rect neighbour_rect(SDL_Rect original_rect,int direction)
{
	switch (direction)
	{
	case LEFT:
		original_rect.x-=SIZE;
		break;

	case UP:
		original_rect.y-=SIZE;
		break;

	case RIGHT:
		original_rect.x+=SIZE;
		break;

	case DOWN:
		original_rect.y+=SIZE;
		break;
	}

	return original_rect;
}

bool sugar_collides(Gamedata& gamedata, SDL_Rect sugar)
{
	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		if (collide(sugar,gamedata.blocks[i].rect))
			return true;
	}

	for (unsigned int i=0;i<gamedata.traps.size();i++)
	{
		if (collide(sugar,gamedata.traps[i].rect))
			return true;
	}

	if (collide(sugar,gamedata.bb.rect))
		return true;

	return false;
}

Sugar create_sugar_from_rect(SDL_Rect rect)
{
	Sugar result;

	result.rect=rect;

	result.vx=result.vy=0;

	result.img_name= IMG_DIR "sugar/sugar.bmp";

	result.update_due=true;
	
	return result;
}

void create_sugar(Gamedata& gamedata, int direction)
{
	SDL_Rect new_rect;

	if (gamedata.sugar_types==OLD)
	{
		//if new sugar can be created
		new_rect=neighbour_rect(gamedata.bb.rect,direction);
		if (collide(new_rect,get_sugar_barriers(gamedata)) || gamedata.sugar_limit == 0 )
		{
			return;
		}

		//if we reach here, sugar creation has been successful
		for (unsigned int i=0;i<gamedata.sugars.size();i++)
		{
			erase(gamedata,gamedata.sugars[i].rect);
		}
		gamedata.sugars.clear();
		gamedata.sugar_states=NORMAL;

		//create a new fresh sugar
		Sugar new_sugar=create_sugar_from_rect(new_rect);
		gamedata.sugars.push_back(new_sugar);

		gamedata.sugar_types=FRESH;
		gamedata.bb.attached=true;
	}
	else
	{
		new_rect=neighbour_rect(gamedata.sugars.back().rect,direction);

		//if the direction leads us to the last sugar ... 
		if ( equals(   new_rect , find_prev_sugar(gamedata)  )  )
		{
			erase(gamedata, gamedata.sugars.back().rect);
			gamedata.sugars.pop_back();

			if (gamedata.keysheld['x']==false && bb_can_stand_up(gamedata))
			{
				quit_boxing(gamedata);
			}

			return;
		}


		//if new sugar can be created
		if (gamedata.sugars.size() == gamedata.sugar_limit || sugar_collides(gamedata,new_rect))
		{
			return;
		}

		//create a new sugar
		Sugar new_sugar=create_sugar_from_rect(new_rect);
		gamedata.sugars.push_back(new_sugar);

	}
}

void keydown_x(Gamedata& gamedata)
{
	if (gamedata.keysheld['x']==true)
		return;

	gamedata.keysheld['x']=true;

	if (gamedata.bb.state==BOXING || gamedata.bb.state==MID_AIR)
		return;

	enter_boxing(gamedata);
}

void keyup_x(Gamedata& gamedata)
{
	gamedata.keysheld['x']=false;

	if (gamedata.bb.state != BOXING)
		return;

	quit_boxing(gamedata);
}

void keydown_v(Gamedata& gamedata)
{
	if (collide(gamedata.bb.rect,gamedata.door.rect))
	{
		gamedata.state=WIN;
	}
}

void keydown_right(Gamedata& gamedata)
{
	gamedata.keysheld[RIGHT]=true;

	if (gamedata.keysheld[LEFT]==true)
	{
		gamedata.keysheld[LEFT]=false;
	}

	if (gamedata.bb.state == BOXING)
	{
		create_sugar(gamedata,RIGHT);
	}
	else
	{
		gamedata.bb.direction=RIGHT;

		gamedata.bb.vx= V_X;

		if ( gamedata.bb.state != MID_AIR && gamedata.bb.state != RUNNING )
		{
			//animate boxboy
			gamedata.bb.state = RUNNING;
			gamedata.bb.running_frame_id=READY_TO_RUN;
		}
	}

}

void keyup_right(Gamedata& gamedata)
{
	if (gamedata.keysheld[RIGHT]==false)
		return;
	
	gamedata.keysheld[RIGHT]=false;

	gamedata.bb.vx=0;

	if (gamedata.bb.state==RUNNING)
	{
		gamedata.bb.state=STANDING;
	}
	
}

void keydown_left(Gamedata& gamedata)
{
	gamedata.keysheld[LEFT]=true;

	if (gamedata.keysheld[RIGHT]==true)
	{
		gamedata.keysheld[RIGHT]=false;
	}

	if (gamedata.bb.state == BOXING)
	{
		create_sugar(gamedata,LEFT);
	}
	else
	{
		gamedata.bb.direction=LEFT;
		gamedata.bb.vx= (-1)*V_X;

		if ( gamedata.bb.state != MID_AIR && gamedata.bb.state != RUNNING )
		{
			//animate boxboy
			gamedata.bb.state = RUNNING;
			gamedata.bb.running_frame_id=READY_TO_RUN;
		}
	}

}

void keyup_left(Gamedata& gamedata)
{
	if (gamedata.keysheld[LEFT]==false)
		return;
	
	gamedata.keysheld[LEFT]=false;

	gamedata.bb.vx=0;

	if (gamedata.bb.state==RUNNING)
	{
		gamedata.bb.state=STANDING;
	}
}

void swap_with_last_sugar(Gamedata& gamedata)
{
	if (gamedata.sugars.size()==0)
		return ;

	erase(gamedata,gamedata.bb.rect);
	gamedata.bb.rect.x= gamedata.sugars.back().rect.x;
	gamedata.bb.rect.y= gamedata.sugars.back().rect.y;
	
	if (gamedata.bb.state == BOXING)
		gamedata.bb.rect.h+=15;

	if (collide(gamedata.bb.rect,get_boxboy_barriers(gamedata)))
	{
		gamedata.bb.rect.y-=15;
	}

	gamedata.bb.state= MID_AIR;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		erase(gamedata,gamedata.sugars[i].rect);
	}

	gamedata.sugars.clear();

	gamedata.bb.attached=false;
	gamedata.sugar_types=OLD;

}

void keydown_space(Gamedata& gamedata)
{
	switch (gamedata.bb.state)
	{
	case MID_AIR:
		return;

	case RUNNING:
	case STANDING:
		gamedata.bb.state=MID_AIR;
		gamedata.bb.vy=V_Y;
		break;

	case BOXING:
	case STUCK_ON_AIR:
		swap_with_last_sugar(gamedata);
		break;

	default:
		cerr<<"invalid state for boxboy at keydown_space function"<<endl;
		exit(EXIT_FAILURE);
	}

}

void keydown_up(Gamedata& gamedata)
{
	if (gamedata.keysheld['x']==true)
	{
		create_sugar(gamedata,UP);
		return;
	}
}

void keydown_down(Gamedata& gamedata)
{
	if (gamedata.keysheld['x']==true)
	{
		create_sugar(gamedata,DOWN);
		return;
	}
}

void throw_sugars(Gamedata& gamedata,int direction)
{
	if (gamedata.bb.attached == false || gamedata.bb.state == BOXING)
		return;

	gamedata.bb.attached=false;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		gamedata.sugars[i].vx= (direction == RIGHT) ? V_X_SUGAR : (-1)*(V_X_SUGAR) ;
		gamedata.sugars[i].vy= V_Y_SUGAR;
	}

}

void keydown_c(Gamedata& gamedata)
{
	throw_sugars(gamedata,RIGHT);
}

void keydown_z(Gamedata& gamedata)
{
	throw_sugars(gamedata,LEFT);
}

void handle_events(Gamedata& gamedata)
{
	SDL_Event _event;

	while (SDL_PollEvent(&_event))
	{
		if (_event.type==SDL_QUIT)
		{
			gamedata.state=QUIT;
			return;
		}

		if(_event.type==SDL_KEYDOWN)
		{
			switch (_event.key.keysym.sym)
			{

			case SDLK_RIGHT:
				keydown_right(gamedata);
				break;

			case SDLK_LEFT:
				keydown_left(gamedata);
				break;

			case SDLK_UP:
				keydown_up(gamedata);
				break;

			case SDLK_DOWN:
				keydown_down(gamedata);
				break;

			case SDLK_x:
				keydown_x(gamedata);
				break;

			case SDLK_SPACE:
				keydown_space(gamedata);
				break;
			
			case SDLK_c:
				keydown_c(gamedata);
				break;

			case SDLK_z:
				keydown_z(gamedata);
				break;

			case SDLK_v:
				keydown_v(gamedata);
				break;
			}
			
		}

		if (_event.type==SDL_KEYUP)
		{
			switch (_event.key.keysym.sym)
			{
			case SDLK_x:
				keyup_x(gamedata);
				break;

			case SDLK_RIGHT:
				keyup_right(gamedata);
				break;

			case SDLK_LEFT:
				keyup_left(gamedata);
				break;
			}
		}
	}
}

int state_handler(Gamedata& gamedata)
{
	switch (gamedata.state)
	{
	case QUIT:
		return QUIT;

	case DEATH:
		show_all(gamedata);
		gamedata.game_window->draw_bmp("images/death.bmp",NULL,NULL);
		SDL_Delay(1000);
		gamedata.game_window->update_screen();
		SDL_Delay(2000);
		return QUIT;

	case WIN:
		show_all(gamedata);
		SDL_Delay(1000);
		if (gamedata.score==gamedata.rewards_count)
		{
			gamedata.game_window->draw_bmp("images/full_score_win.bmp",NULL,NULL);
		}
		else
		{
			gamedata.game_window->draw_bmp("images/low_score_win.bmp",NULL,NULL);
		}
		SDL_Delay(500);
		gamedata.game_window->update_screen();
		SDL_Delay(2000);
		return QUIT;

	case CONTINUE : 
		return CONTINUE;

	default :
		return CONTINUE;
	}

}

bool collides_with_blocks(Gamedata& gamedata, SDL_Rect what)
{
	for (unsigned int i=0;i<gamedata.blocks.size();i++)
	{
		if (collide(gamedata.blocks[i].rect,what))
			return true;
	}
	return false;
}

void stick_r1_to_r2_horizontally(SDL_Rect& r1, SDL_Rect r2)
{
	if (r1.x < r2.x)
	{
		r1.x= r2.x - r1.w;
	}
	if (r1.x > r2.x)
	{
		r1.x= r2.x + r2.w;
	}
}

void stick_r1_to_r2_vertically(SDL_Rect& r1, SDL_Rect r2)
{
	if (r1.y < r2.y)
	{
		r1.y= r2.y - r1.h;
	}
	if (r1.y > r2.y)
	{
		r1.y= r2.y + r2.h;
	}
}

bool collide_vertically(SDL_Rect target, SDL_Rect mover, double y, int vy)
{
	if ( mover.y+ mover.h <= target.y || mover.y >= target.y +target.h)
	{
		SDL_Rect new_pos=mover;
		new_pos.y = (int) (y+vy);

		if (collide(new_pos,target))
		{
			return true;
		}
	}

	return false;
}

inline bool r1_is_on_r2(SDL_Rect r1, SDL_Rect r2)
{
	return r1.y+r1.h == r2.y  && r1.x+r1.w > r2.x && r1.x < r2.x+r2.w;
}

bool rect_is_on_block_or_sugar(Gamedata& gamedata,SDL_Rect rect)
{
	for (unsigned int i=0; i < gamedata.blocks.size() ;i++)
	{
		if (r1_is_on_r2(rect,gamedata.blocks[i].rect))
			return true;
	}

	for (unsigned int i=0; i < gamedata.sugars.size() ;i++)
	{
		if (r1_is_on_r2(rect,gamedata.sugars[i].rect))
			return true;
	}

	return false;

}

bool rect_is_on_block_or_trap_or_laserbase(Gamedata& gamedata,SDL_Rect rect)
{
	vector <SDL_Rect> temp= get_trap_and_block_and_laser_rects(gamedata);
	
	for (unsigned int i=0; i < temp.size() ;i++)
	{
		if (r1_is_on_r2(rect,temp[i]))
			return true;
	}

	return false;

}

bool rect_is_on_block(Gamedata& gamedata,SDL_Rect rect)
{


	for (unsigned int i=0; i < gamedata.blocks.size() ;i++)
	{
		if (r1_is_on_r2(rect,gamedata.blocks[i].rect))
			return true;
	}

	return false;

}

void set_sugars_speed(Gamedata& gamedata,int vx,int vy)
{
	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		gamedata.sugars[i].vx=vx;
		gamedata.sugars[i].vy=vy;
	}
}

void move_sugars(Gamedata& gamedata)
{
	if (gamedata.sugars.size() == 0)
		return;

	//erase previous sugar positions
	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		erase(gamedata,gamedata.sugars[i].rect);
	}

	//move sugars according to barriers
	bool vertical_col,horizontal_col;

	int common_dx=gamedata.sugars[0].vx ,common_dy=gamedata.sugars[0].vy;

	vector <SDL_Rect> sugar_barriers=get_sugar_barriers(gamedata);
	vector <SDL_Rect> new_rects(gamedata.sugars.size());


	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		new_rects[i]=move_rect(gamedata.sugars[i].rect,gamedata.sugars[i].vx,gamedata.sugars[i].vy,sugar_barriers,vertical_col,horizontal_col);
		if (vertical_col)
		{
			common_dy=new_rects[i].y-gamedata.sugars[i].rect.y;
			for (unsigned int i=0;i<gamedata.sugars.size();i++)
			{
				gamedata.sugars[i].vy=0;
			}
		}
		if (horizontal_col)
		{
			common_dx=new_rects[i].x-gamedata.sugars[i].rect.x;
			for (unsigned int i=0;i<gamedata.sugars.size();i++)
			{
				gamedata.sugars[i].vx=0;
			}
		}
	}

	//check to see if on ground
	bool on_ground=false;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		if (rect_is_on_block_or_trap_or_laserbase(gamedata, gamedata.sugars[i].rect) )
		{
			on_ground=true;
		}
	}
	if (on_ground)
	{
		for (unsigned int i=0;i<gamedata.sugars.size();i++)
		{
			gamedata.sugars[i].vx=0;
		}
	}


	move_only_sugars(gamedata,common_dx,common_dy);


	center_camera(gamedata);

}

SDL_Rect move_boxboy_rect(Gamedata& gamedata, SDL_Rect what,int _vx,int _vy,const vector <SDL_Rect>& barriers, bool& collided_vertically, bool& collided_horizontally)
{
	Precise_Rect precise_position=cast_to_precise(what);

	double vx=_vx;
	double vy=_vy;
	int move_n_times;

	//scale vx and vy so that they are not greater than 1
	if (my_abs(_vx) > my_abs(_vy))
	{
		move_n_times = my_abs(_vx);
		vy= vy / my_abs(vx);
		vx= vx / my_abs(vx);
	}
	else
	{
		move_n_times = my_abs(_vy);
		vx = vx / my_abs(vy);
		vy = vy / my_abs(vy);	
	}

	Precise_Rect new_pos=precise_position; // d = double

	collided_horizontally=collided_vertically=false;

	for (int i=0;i<move_n_times;i++)
	{
		if (!collided_horizontally)
		{
			//move horizontally
			new_pos.x+=vx;
			new_pos.y+=0;

			if (collide(cast_to_SDL(new_pos),barriers))
			{
				collided_horizontally=true;
				new_pos.x-=vx;
			}
			else
			{
				if (collide(cast_to_SDL(new_pos),get_sugar_rects(gamedata)) ) //if collided with a sugar cube, 
				{
					//try to move those sugars
					set_sugars_speed(gamedata,((vx >0) ? 1 : -1),0);
					move_sugars(gamedata);
					
					if (collide(cast_to_SDL(new_pos),get_sugar_rects(gamedata)) )//if failed to move sugars due to other barriers
					{
						collided_horizontally=true;
						new_pos.x-=vx;
					}
				}
			}
		}

		if (!collided_vertically)
		{
			//move vertically
			new_pos.x+=0;
			new_pos.y+=vy;

			if (collide(cast_to_SDL(new_pos),barriers))
			{
				collided_vertically=true;
				new_pos.y-=vy;
			}
			else
			{
				if (collide(cast_to_SDL(new_pos),get_sugar_rects(gamedata)) ) //if collided with a sugar cube, 
				{
					//try to move those sugars
					set_sugars_speed(gamedata,0,((vy >0) ? 1 : -1));
					move_sugars(gamedata);
					
					if (collide(cast_to_SDL(new_pos),get_sugar_rects(gamedata)) )//if failed to move sugars due to other barriers
					{
						collided_vertically=true;
						new_pos.y-=vy;
					}
				}
			}
		}
	}

	return cast_to_SDL(new_pos);
}

void score_colliding_crowns(Gamedata& gamedata)
{
	for (unsigned int i=0;i<gamedata.crowns.size();i++)
	{
		if (collide(gamedata.bb.rect,gamedata.crowns[i].rect))
		{
			erase(gamedata, gamedata.crowns[i].rect );
			gamedata.crowns.erase(gamedata.crowns.begin()+i);
			i--;
			gamedata.score++;
		}
	}
}

void move_boxboy_alone(Gamedata& gamedata)
{

	if ( gamedata.bb.vx == 0.01 && gamedata.bb.vy == 0.01 )
	{
		return ;
	}

	erase(gamedata, gamedata.bb.rect);

	bool vertical_col,horizontal_col;
	SDL_Rect new_rect=move_boxboy_rect(gamedata,gamedata.bb.rect, gamedata.bb.vx , gamedata.bb.vy , get_boxboy_barriers(gamedata) ,vertical_col,horizontal_col);
	gamedata.bb.rect=new_rect;

	//prevent speed accumulation due to gravity that leads to falling off blocks like crazy ! 
	if (vertical_col)
		gamedata.bb.vy=0;


	//check to see if on ground
	if (rect_is_on_block_or_sugar(gamedata,gamedata.bb.rect) && gamedata.bb.state == MID_AIR)
	{
		if (my_abs(gamedata.bb.vx) < 0.01)
		{
			gamedata.bb.state= STANDING;
		}
		else
		{
			gamedata.bb.state= RUNNING;
		}
	}


	//handle killer collision
	vector <SDL_Rect> killers= get_boxboy_killers(gamedata);
	
	new_rect.h-=15; //we do this so that lasers dont act on boxboy's legs
	if (collide(new_rect,killers))
	{
		gamedata.state=DEATH;
		return;
	}
	new_rect.h+=15;

	//crown collision
	score_colliding_crowns(gamedata);

	//getting out of the map
	if (!r1_completely_in_r2(gamedata.bb.rect,gamedata.camera))
	{
		gamedata.state=DEATH;
		return;
	}

	center_camera(gamedata);
}

inline double rect_distance(SDL_Rect r1,SDL_Rect r2)
{
	int dx = r1.x-r2.x;
	int dy = r1.y-r2.y;

	return sqrt( (double) (dx*dx + dy*dy)); 
}

void update_boxboy_attached(Gamedata& gamedata,int dx,int dy)
{
	gamedata.bb.rect.x+=dx;
	gamedata.bb.rect.y+=dy;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		gamedata.sugars[i].rect.x+=dx;
		gamedata.sugars[i].rect.y+=dy;
	}
}

void update_sugars(Gamedata& gamedata,int dx,int dy)
{
	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		gamedata.sugars[i].rect.x+=dx;
		gamedata.sugars[i].rect.y+=dy;
	}
}

void move_boxboy_attached(Gamedata& gamedata)
{
	if (gamedata.bb.vx == 0 && gamedata.bb.vy==0 )
	{
		return ;
	}


	//erase previous boxboy and sugar positions
	erase(gamedata,gamedata.bb.rect);
	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		erase(gamedata,gamedata.sugars[i].rect);
	}


	//move boxboy and sugars according to barriers
	bool vertical_col,horizontal_col;

	SDL_Rect new_rect;

	//sugars :
	int common_dx=gamedata.bb.vx,common_dy=gamedata.bb.vy;
	vector <SDL_Rect> sugar_barriers=get_sugar_barriers(gamedata);
	vector <SDL_Rect> new_rects(gamedata.sugars.size());


	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		new_rects[i]=move_rect(gamedata.sugars[i].rect,gamedata.bb.vx,gamedata.bb.vy,sugar_barriers,vertical_col,horizontal_col);
		if (vertical_col)
		{
			common_dy=new_rects[i].y-gamedata.sugars[i].rect.y;
			gamedata.bb.vy=0;
		}
		if (horizontal_col)
		{
			common_dx=new_rects[i].x-gamedata.sugars[i].rect.x;
		}
	}

	vector <SDL_Rect> box_boy_barriers = get_boxboy_barriers(gamedata);
	new_rect=move_rect( gamedata.bb.rect, gamedata.bb.vx , gamedata.bb.vy , box_boy_barriers,vertical_col,horizontal_col);
	if (vertical_col)
	{
		common_dy=new_rect.y-gamedata.bb.rect.y;
		gamedata.bb.vy=0;
	}
	if (horizontal_col)
	{
		common_dx=new_rect.x-gamedata.bb.rect.x;
	}


	update_boxboy_attached(gamedata,common_dx,common_dy);


	//check to see if on ground

	if (rect_is_on_block_or_sugar(gamedata, gamedata.bb.rect) )
	{
		if (gamedata.bb.state != BOXING)
		{
			if (gamedata.bb.vx == 0)
			{
				gamedata.bb.state=STANDING;
			}
			else
			{
				gamedata.bb.state=RUNNING;
			}
		}
	}
	else
	{
		for (unsigned int i=0;i<gamedata.sugars.size();i++)
		{
			if (rect_is_on_block_or_sugar(gamedata, gamedata.sugars[i].rect) )
			{
				gamedata.bb.state=STUCK_ON_AIR;
			}
		}
	}
	
	

	//handle killer collision
	vector <SDL_Rect> killers= get_boxboy_killers(gamedata);

	gamedata.bb.rect.h-=15;	//we do this so that lasers dont act on boxboy's legs
	if (collide(gamedata.bb.rect,killers))
	{
			gamedata.state=DEATH;
			return;
	}
	gamedata.bb.rect.h+=15;


	//handle crowns scoring
	score_colliding_crowns(gamedata);


	//handle getting out of the map
	if (!r1_completely_in_r2(gamedata.bb.rect,gamedata.camera))
	{
		gamedata.state=DEATH;
		return;
	}
	
	center_camera(gamedata);
}

void gravitate(Gamedata& gamedata)
{
	gamedata.bb.vy += GRAVITATIONAL_ACCELERATION;

	for (unsigned int i=0;i<gamedata.sugars.size();i++)
	{
		gamedata.sugars[i].vy += GRAVITATIONAL_ACCELERATION;
	}
}

void automatic_movements(Gamedata& gamedata)
{
	if (gamedata.bb.attached)
	{
		move_boxboy_attached(gamedata);
	}
	else
	{
		move_boxboy_alone(gamedata);
		move_sugars(gamedata);
	}

	gravitate(gamedata);
}


int main(int argc, char* ags[])
{
	window game_window(WINDOW_WIDTH,WINDOW_HEIGHT,"BoyBoy!");
	Gamedata gamedata;

	load_gamedata(gamedata,&game_window);

	while (true)
	{
		handle_events(gamedata);

		if (state_handler(gamedata) == QUIT)
			break;

		automatic_movements(gamedata);

		if (state_handler(gamedata)== QUIT)
			break;

		show_all(gamedata);

		SDL_Delay(15);
	}
	
	return 0;

}


int main_test( int argc, char* argv[])
{
	if (SDL_Init ( SDL_INIT_EVERYTHING ) == -1)
	{
		cerr<<"sdl failed\n";
		return false;
	}

	Mix_Chunk* gscratch = NULL;
	Mix_Music* gMusic = NULL;

	bool success;

	if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 ) 
	{
		printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() ); 
		success = false; 
	}

	gMusic = Mix_LoadMUS( "music/Winding-Down.mp3" ); 
	if( gMusic == NULL ) 
	{ 
		printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() ); 
    	success = false; 
	}

	gscratch = Mix_LoadWAV( "music/1.wav" );
	if (gscratch == NULL)
	{
		cerr<<"failed to load wav"<<endl;
	}

	//Mix_PlayMusic ( gMusic , -1 );

	Mix_PlayChannel( -1, gscratch , 0 );

	SDL_Window* w = SDL_CreateWindow ( " Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 400,400, SDL_WINDOW_SHOWN);


	char c;
	while ( cin>>c );

	cout<<"reached end of program"<<endl;

}