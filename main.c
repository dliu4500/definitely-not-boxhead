#include <lpc17xx.h>
#include "GLCD.h"
#include <RTL.h>
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

//Task Headers
__task void playerMovement();
__task void playerShoot();

// Structs
typedef struct{
	unsigned short pBitmap[9];
	unsigned short pInvBitmap[9];
	int x, y, prevX, prevY;	
	int direction; 
}player_t;

typedef struct{
	int x,y;
	struct zombie_t *next;
}zombie_t;

//Global Variables
player_t player;
bool map[120][160];
zombie_t *head;

unsigned short zBitmap[9];
unsigned short zInvBitmap[9];

//PLAYER FUNCTIONS*******************************************************
void playerInit(player_t *player){
	player->pBitmap[0] = Black;
	player->pBitmap[1] = Black;
	player->pBitmap[2] = Black;
	player->pBitmap[3] = Black;
	player->pBitmap[4] = Black;
	player->pBitmap[6] = Black;
	player->pBitmap[7] = Black;
	player->pBitmap[8] = Black;
	
	player->pInvBitmap[0] = White;
	player->pInvBitmap[1] = White;
	player->pInvBitmap[2] = White;
	player->pInvBitmap[3] = White;
	player->pInvBitmap[4] = White;
	player->pInvBitmap[5] = White;
	player->pInvBitmap[6] = White;
	player->pInvBitmap[7] = White;
	player->pInvBitmap[8] = White;
	
	player->x = 120;
	player->y = 160;
	player->prevX = 160;
	player->prevY = 120;
}

void printPlayer(player_t *player){
	GLCD_Bitmap(player->x,player->y, 3, 3, (unsigned char*)(player->pBitmap));
	GLCD_Bitmap(player->prevX,player->prevY, 3, 3, (unsigned char*)(player->pInvBitmap));
}

void updatePlayerMovement(){	
	int left = LPC_GPIO1->FIOPIN>>23 & 1;
	int right = LPC_GPIO1->FIOPIN>>25 & 1;
	int up = LPC_GPIO1->FIOPIN>>24 & 1;
	int down = LPC_GPIO1->FIOPIN>>26 & 1;
	
	if(!left && player.x > 2){
		player.direction = 4;
		if(!map[(player.x - 2) / 2][(player.y) / 2]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.prevX = player.x;
			player.x -= 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			printf("left %d , %d \n", player.x, player.prevX);
		}
	}
	else if(!right && player.x < 236){
		player.direction = 2;
		if(!map[(player.x + 4) / 2][player.y / 2]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.prevX = player.x;
			player.x += 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			printf("right %d , %d \n", player.x, player.prevX);
		}
	}
	else if(!up && player.y < 316){
		player.direction = 1;
		if(!map[(player.x) / 2][(player.y + 4) / 2]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.prevY = player.y;
			player.y += 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			printf("up %d , %d \n", player.y, player.prevY);
		}
	}
	else if(!down && player.y > 2){
		player.direction = 3;
		if(!map[(player.x) / 2][(player.y - 2) / 2]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.prevY = player.y;
			player.y -= 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			printf("down %d , %d \n", player.y, player.prevY);
		}
	}
}

void drawShot(int direction, int length){
	int i;
	unsigned short shotBitmap[1];
	shotBitmap[0] = Magenta;
	if(direction == 1){
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y + 3 + i, player.x + 1, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 3 + i, player.x + 1, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else if(direction == 2){
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y + 1, player.x + 3 + i, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 1, player.x + 3 + i, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else if(direction == 3){
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y - i, player.x + 1, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y - i, player.x + 1, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else{
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y + 1, player.x - i, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 1, player.x - i, 1, 1, (unsigned char*)shotBitmap);
		}
	}
}

void calcPlayerShot(){
	int shotLength = 0;
	int shotCoordinate;
	
	if(player.direction == 1){ //up
		shotCoordinate = player.y + 2;
		while(shotCoordinate <= 320 && !map[player.x / 2][shotCoordinate / 2]){
			// check if hit zombie, if hit, delete zombie, and manipulate linked list 
			shotCoordinate++;
		}
		shotLength = shotCoordinate - player.y;
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 2){
				shotCoordinate = player.x + 2;
		while(shotCoordinate <= 240 && !map[shotCoordinate / 2][player.y / 2]){
			shotCoordinate++;
		}
		shotLength = shotCoordinate - player.x;
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 3){
				shotCoordinate = player.y;
		while(shotCoordinate >= 0 && !map[player.x / 2][shotCoordinate / 2]){
			shotCoordinate--;
		}
		shotLength =  player.y - shotCoordinate;
		drawShot(player.direction, shotLength);
	}
	else{
			shotCoordinate = player.x;
		while(shotCoordinate >= 0 && !map[shotCoordinate / 2][player.y / 2]){
			shotCoordinate--;
		}
		shotLength = player.x - shotCoordinate;
		drawShot(player.direction, shotLength);
	}
}

//ZOMBIE FUNCTIONS*******************************************************
void zombieInit(){
	zBitmap[0] = Red;
	zBitmap[1] = Red;
	zBitmap[2] = Red;
	zBitmap[3] = Red;
	zBitmap[4] = Red;
	zBitmap[6] = Red;
	zBitmap[7] = Red;
	zBitmap[8] = Red;
}

void spawnZombie(){
	GLCD_Bitmap(100,100, 3, 3, (unsigned char*)(zBitmap));
}

//***********************************************************************************************

//Tasks
__task void playerMovement(){
	os_tsk_create(playerShoot, 1);
	while(1){
		updatePlayerMovement();
		os_tsk_pass();
	}
}

__task void playerShoot(){
	//os_tsk_create()
	while(1){
		if(!(LPC_GPIO2->FIOPIN>>10 & 1)){	
			calcPlayerShot();
			//while(!(LPC_GPIO2->FIOPIN>>10 & 1)){}
		}
		os_tsk_pass();
	}
	
}

//Initialization Functions
void mapInit(){
	int i; // < 70
	int j; //< 90
	unsigned short obstacleBitmap[4];
	
	obstacleBitmap[0] = Blue;
	obstacleBitmap[1] = Blue;
	obstacleBitmap[2] = Blue;
	obstacleBitmap[3] = Blue;
	
	for(i = 0; i <= 120; i++){
		for(j = 0; j <= 160; j++){
			if(i >= 50 && i <= 90 && j >= 100 && j <= 120){
				map[i][j] = 1;
				GLCD_Bitmap(j * 2, i * 2, 2, 2, (unsigned char*)obstacleBitmap);
			}
			else map[i][j] = 0;
		}
	}
}

void initialize(){
	GLCD_Init();
	GLCD_Clear(White);
}

int main(void){
	printf("Start");
	initialize();
	mapInit();
	zombieInit();
	playerInit(&player);
	spawnZombie();
	os_sys_init(playerMovement);
	
	while(1){};
}