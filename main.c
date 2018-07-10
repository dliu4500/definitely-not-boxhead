#include <lpc17xx.h>
#include "GLCD.h"
#include "bitmaps.h"
#include <RTL.h>
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include <time.h>

//Task Headers
__task void playerMovement();
__task void playerShoot();
__task void zombieMovement();
__task void spawnZombies();
__task void gameover();

// Structs
typedef struct{
	int x, y;	
	int direction, health; 
}player_t;

struct zombie_t{
	int x, y;
	struct zombie_t *next;
};

//Global Variables******************************************************

// mutexs to protect player and zombie variables
OS_MUT zombieMut;
OS_MUT playerMut;

// player called globally 
player_t player;

// initiate map to be 1/4 the size
bool map[60][80];

struct zombie_t *head = NULL;

int randomSeed;

// white bitmaps used to erase bitmaps created by zombie and player
unsigned short zInvBitmap[361]; 
unsigned short pInvBitmap[400];

//PLAYER FUNCTIONS*******************************************************
// initiate inverse player bitmaps, health LEDs and position of player
void playerInit(){
	int count = 400;
	int i;
	for(i = 0; i < count; i++){
		pInvBitmap[i] = White;
	}
	
	player.health = 8;
	player.x = 100;
	player.y = 190;
	
	LPC_GPIO2->FIOSET |= (1 << 6);	
	LPC_GPIO2->FIOSET |= (1 << 5);
	LPC_GPIO2->FIOSET |= (1 << 4);
	LPC_GPIO2->FIOSET |= (1 << 3);
	LPC_GPIO2->FIOSET |= (1 << 2);
	LPC_GPIO1->FIOSET |= (1 << 31);
	LPC_GPIO1->FIOSET |= (1 << 29);
	LPC_GPIO1->FIOSET |= (1 << 28);
}


// determine player movement from joystick input
void updatePlayerMovement(){	
	
	int left = LPC_GPIO1->FIOPIN>>23 & 1;
	int right = LPC_GPIO1->FIOPIN>>25 & 1;
	int up = LPC_GPIO1->FIOPIN>>24 & 1;
	int down = LPC_GPIO1->FIOPIN>>26 & 1;
	
	if(!left && player.x > 2){ // moving left and not at edge of LCD
		player.direction = 4;
		if(!map[(player.x - 3) / 4][(player.y) / 4] && !map[(player.x - 3) / 4][(player.y + 20) / 4]){ // check map collisions
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pInvBitmap));
			player.x -= 2;
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pBitmapLeft));
		}
	}
	else if(!right && player.x < 220){
		player.direction = 2;
		if(!map[(player.x + 20 + 3) / 4][player.y / 4] && !map[(player.x + 20 + 3) / 4][(player.y + 20) / 4]){
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pInvBitmap));
			player.x += 2;
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pBitmapRight));
		}
	}
	else if(!up && player.y < 300){
		player.direction = 1;
		if(!map[(player.x) / 4][(player.y + 20 + 3) / 4] && !map[(player.x + 20) / 4][(player.y + 20 + 3) / 4]){
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pInvBitmap));
			player.y += 2;
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pBitmapUp));
		}
	}
	else if(!down && player.y > 2){
		player.direction = 3;
		if(!map[(player.x) / 4][(player.y - 3) / 4] && !map[(player.x + 20) / 4][(player.y - 3) / 4]){
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pInvBitmap));
			player.y -= 2;
			GLCD_Bitmap(player.y,player.x, 20, 20, (unsigned char*)(pBitmapDown));
		}
	}
}

// erases zombie bitmap 
void zombieErase(int x, int y){
		GLCD_Bitmap(y,x, 19, 19, (unsigned char*)(zInvBitmap));
}

// checks and erases zombies if they are hit by gun projectile
void checkZombieCollision(int direction, int length){
	struct zombie_t * dummy = head;
	struct zombie_t * trailer = head;
	while(dummy != NULL){ // iterate through zombie linked list 
		if(direction == 1 && abs( player.x - (dummy->x) ) < 10 && player.y < (dummy->y) && (player.y + length) > dummy->y ){ // check if zombie is above player and is hit by gun 
			if(dummy == head){ // delete cases
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(trailer);
				dummy = head;
				trailer = head;
			}
			else{ 
				zombieErase(dummy->x, dummy->y);
				trailer->next = dummy->next;
				free(dummy);
				dummy = trailer->next;
			}
		}
		else if(direction == 2 && abs( player.y - (dummy->y) ) < 10 && player.x < (dummy->x) && (player.x + length) > dummy->x ){ // same but if zombie is on the right
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(trailer);
				dummy = head;
				trailer = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				trailer->next = dummy->next;
				free(dummy);
				dummy = trailer->next;
			}
		}
		else if(direction == 3 && abs( player.x - (dummy->x) ) < 10 && player.y > (dummy->y) && (player.y - length) < dummy->y ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(trailer);
				dummy = head;
				trailer = head;
			}
			else{ 
				zombieErase(dummy->x, dummy->y);
				trailer->next = dummy->next;
				free(dummy);
				dummy = trailer->next;
			}
		}
		else if(direction == 4 && abs( player.y - (dummy->y) ) < 10 && player.x > (dummy->x) && (player.x - length) < dummy->x ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(trailer);
				dummy = head;
				trailer = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				trailer->next = dummy->next;
				free(dummy);
				dummy = trailer->next;
			}
		}
		else{ // iterate as normal 
			trailer = dummy;
			dummy = dummy->next;
		}
	}
}

// draws the gun shot 
void drawShot(int direction, int length){
	int i;
	unsigned short shotBitmap[1];
	shotBitmap[0] = Magenta;
	if(direction == 1){
		for(i = 0; i < length-20; i++){ // iterate until the calculated length 
			GLCD_Bitmap(player.y + 20 + i, player.x + 10, 1, 1, (unsigned char*)shotBitmap); // offsets so that the gun shoots from a location that makes sense (middle of character)
		}
		for(i = 0; i < length-20; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 20 + i, player.x + 10, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else if(direction == 2){
		for(i = 0; i < length-20; i++){
			GLCD_Bitmap(player.y + 10, player.x + 20 + i, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length-20; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 10, player.x + 20 + i, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else if(direction == 3){
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y - i, player.x + 10, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y - i, player.x + 10, 1, 1, (unsigned char*)shotBitmap);
		}
	}
	else{
		for(i = 0; i < length; i++){
			GLCD_Bitmap(player.y + 10, player.x - i, 1, 1, (unsigned char*)shotBitmap);
		}
		for(i = 0; i < length; i++){
			shotBitmap[0] = White;
			GLCD_Bitmap(player.y + 10, player.x - i, 1, 1, (unsigned char*)shotBitmap);
		}
	}
}


// calc length of the shot depending on whether it hits a wall or the end of the LCD screen 
void calcPlayerShot(){
	int shotLength = 0;
	int shotCoordinate;
	
	if(player.direction == 1){ //up
		shotCoordinate = player.y + 20;
		while(shotCoordinate <= 320 && !map[(player.x + 10) / 4][shotCoordinate / 4]){
			shotCoordinate++;
		}
		shotLength = shotCoordinate - player.y;
		checkZombieCollision(player.direction, shotLength);		
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 2){
				shotCoordinate = player.x + 20;
		while(shotCoordinate <= 240 && !map[shotCoordinate / 4][(player.y +10)/ 4]){
			shotCoordinate++;
		}
		shotLength = shotCoordinate - player.x;
		checkZombieCollision(player.direction, shotLength);
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 3){
				shotCoordinate = player.y;
		while(shotCoordinate >= 0 && !map[(player.x + 10) / 4][shotCoordinate / 4]){
			shotCoordinate--;
		}
		shotLength =  player.y - shotCoordinate;
		checkZombieCollision(player.direction, shotLength);
		drawShot(player.direction, shotLength);
	}
	else{
			shotCoordinate = player.x;
		while(shotCoordinate >= 0 && !map[shotCoordinate / 4][(player.y + 10) / 4]){
			shotCoordinate--;
		}
		shotLength = player.x - shotCoordinate;
		checkZombieCollision(player.direction, shotLength);
		drawShot(player.direction, shotLength);
	}
}

// decrements player health if hit by zombie (by turning off the left most LED), creates an end game tasks when health reaches 0 
void decrementHealth(){
	player.health--;
	
	if(player.health > 0) LPC_GPIO2->FIOSET |= (1 << 6);
	else LPC_GPIO2->FIOCLR |= (1 << 6);
	
	if(player.health > 1) LPC_GPIO2->FIOSET |= (1 << 5);
	else LPC_GPIO2->FIOCLR |= (1 << 5);
	
	if(player.health > 2) LPC_GPIO2->FIOSET |= (1 << 4);
	else LPC_GPIO2->FIOCLR |= (1 << 4);
	
	if(player.health > 3) LPC_GPIO2->FIOSET |= (1 << 3);
	else LPC_GPIO2->FIOCLR |= (1 << 3);
	
	if(player.health > 4) LPC_GPIO2->FIOSET |= (1 << 2);
	else LPC_GPIO2->FIOCLR |= (1 << 2);
	
	if(player.health > 5) LPC_GPIO1->FIOSET |= (1 << 31);
	else LPC_GPIO1->FIOCLR |= (1 << 31);
	
	if(player.health > 6) LPC_GPIO1->FIOSET |= (1 << 29);
	else LPC_GPIO1->FIOCLR |= (1 << 29);
	
	if(player.health > 7) LPC_GPIO1->FIOSET |= (1 << 28);
	else LPC_GPIO1->FIOCLR |= (1 << 28);
	
	if(player.health <= 0){
		os_tsk_create(gameover, 1);
	}
	
}

//ZOMBIE FUNCTIONS*******************************************************

// initiates inverse zombie bitmap
void zombieInit(){
	int count = 361;
	int i;	
	for(i = 0; i < count; i++){
		zInvBitmap[i] = White;
	}
}

// randomly spawns an unknown number of zombies between 2 to 11 at the edges of the LCD screen 
void spawnZombieWave(){
	struct zombie_t * dummy = head;
	int numZombiesPerWave = (rand() % 10) + 2;
	int i;
	int spawnSide = 0;
	
	if(head == NULL){ // if there are no zombies in the linked list yet
		// create list
		head = malloc(sizeof(struct zombie_t));
		head -> next = NULL;
		spawnSide = (rand() % 4);
		if(spawnSide == 0){
			head -> x = 10; // along x wall
			head -> y = (rand() % 310); // max size of y 
		}
		else if(spawnSide == 1){
			head -> y = 10; // along y wall
			head -> x = (rand() % 230); // max size of x
		}
		else if(spawnSide == 2){
			head -> x = 230;
			head -> y = (rand() % 310);
		}
		else{
			head -> y = 310;
			head -> x = (rand() % 230);
		}
		GLCD_Bitmap(head->y, head->x, 19, 19, (unsigned char*)(zombieBitmap));
		numZombiesPerWave--;
	}
	
	dummy = head;
	
	while(dummy -> next != NULL){ // go to the end of the list 
		dummy = dummy->next;
	} 
	
	// add "numZombiesPerWave" to the end of the linked list at random locations around the LCD
	for (i = 0; i < numZombiesPerWave; i++){
		struct zombie_t *derek_at_8am = malloc(sizeof(struct zombie_t));
		dummy -> next = derek_at_8am;
	
		spawnSide = (rand() % 4);
		if(spawnSide == 0){
			derek_at_8am -> x = 1;
			derek_at_8am -> y = (rand() % 320);
		}
		else if(spawnSide == 1){
			derek_at_8am -> y = 1;
			derek_at_8am -> x = (rand() % 240);
		}
		else if(spawnSide == 2){
			derek_at_8am -> x = 239;
			derek_at_8am -> y = (rand() % 320);
		}
		else{
			derek_at_8am -> y = 319;
			derek_at_8am -> x = (rand() % 240);
		}
		GLCD_Bitmap(derek_at_8am->y, derek_at_8am->x, 19, 19, (unsigned char*)(zombieBitmap));
		dummy = dummy -> next;
	}
		
	dummy -> next = NULL; 
	
}

// moves zombies to a location closer to the player depending on their delta X/Y
void moveZombies(){
	struct zombie_t * dummy = head;
	struct zombie_t * trailer = head;
	
	while(dummy != NULL){ // iterate through entire zombie linked list 
		
		int deltaX = player.x - dummy->x;
		int deltaY = player.y - dummy->y;
		
		zombieErase(dummy->x, dummy->y);
		
		if(abs(deltaX) > abs(deltaY)){ // if the zombie is further in the x direction ...
			// check if the new location will hit a wall
			if(deltaX > 0 && !map[((dummy->x) + deltaX/abs(deltaX) + 20)  / 4][(dummy->y) / 4] && !map[((dummy->x) + deltaX/abs(deltaX) + 20)  / 4][(dummy->y + 20) / 4]){ 
				dummy->x += deltaX / abs(deltaX);
			}
			else if(deltaX < 0 && !map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y) / 4] && !map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y + 20) / 4]){
				dummy->x += deltaX / abs(deltaX);
			}
			else if(deltaY > 0 && !map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY) + 20) / 4] && !map[(dummy->x + 20)/ 4][((dummy->y) + deltaY/abs(deltaY) + 20) / 4]){
					dummy->y += deltaY / abs(deltaY);
			}
			else if(deltaY < 0 && !map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4] && !map[(dummy->x + 20)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4]){
					dummy->y += deltaY / abs(deltaY);
			}
			GLCD_Bitmap(dummy->y,dummy->x, 19, 19, (unsigned char*)(zombieBitmap));
		}
		else {
			if(deltaY > 0 && !map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY) + 20) / 4] && !map[(dummy->x + 20)/ 4][((dummy->y) + deltaY/abs(deltaY) + 20) / 4]){
					dummy->y += deltaY / abs(deltaY);
			}
			else if(deltaY < 0 && !map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4] & !map[(dummy->x + 20)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4]){
					dummy->y += deltaY / abs(deltaY);
			}
			else if(deltaX > 0 && !map[((dummy->x) + deltaX/abs(deltaX) + 20)  / 4][(dummy->y) / 4] && !map[((dummy->x) + deltaX/abs(deltaX) + 20)  / 4][(dummy->y + 20) / 4]){
				dummy->x += deltaX / abs(deltaX);
			}
			else if(deltaX < 0 && !map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y) / 4] && !map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y + 20) / 4]){
				dummy->x += deltaX / abs(deltaX);
			}			
			GLCD_Bitmap(dummy->y,dummy->x, 19, 19, (unsigned char*)(zombieBitmap));
		}

		// checks if any part of the zombie will touch the player 
		if(((dummy->x) > (player.x) && (dummy->x) < (player.x + 20) && (dummy->y) > (player.y) && (dummy->y) < (player.y + 20)) || //top right
			 ((dummy->x) > (player.x) && (dummy->x) < (player.x + 20) && (dummy->y + 20) > (player.y) && (dummy->y + 20) < (player.y + 20)) || //bottom right
			 ((dummy->x + 20) > (player.x) && (dummy->x + 20) < (player.x + 20) && (dummy->y + 20) > (player.y) && (dummy->y + 20) < (player.y + 20)) || // bottom left
			 ((dummy->x + 20) > (player.x) && (dummy->x + 20) < (player.x + 20) && (dummy->y) > (player.y) && (dummy->y) < (player.y + 20))//top left
			){
			decrementHealth();
		// delete zombie if it touches player 
			if(dummy == head){ 
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(trailer);
				dummy = head;
				trailer = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				trailer->next = dummy->next;
				free(dummy);
				dummy = trailer->next;
			}
		}
		// iterate as normal 
		else{
			trailer = dummy;
			dummy = dummy->next;
		}
		
	}

}

//***********************************************************************************************

//Tasks
__task void playerMovement(){
	os_tsk_create(playerShoot, 1);
	
	while(1){
		os_mut_wait(&playerMut, 0xffff);
		updatePlayerMovement();
		os_mut_release(&playerMut);
		os_dly_wait(3);
		os_tsk_pass(); 
	}
}

__task void playerShoot(){
	os_tsk_create(zombieMovement, 1);
	while(1){

		if(!(LPC_GPIO2->FIOPIN>>10 & 1)){	
			os_mut_wait(&playerMut, 0xffff);
			os_mut_wait(&zombieMut, 0xffff);
			calcPlayerShot();
			os_mut_release(&zombieMut);
			os_mut_release(&playerMut);
			
		}
		os_dly_wait(5);
		os_tsk_pass();
	}
}

__task void zombieMovement(){
	os_tsk_create(spawnZombies, 1);
	while(1){
		os_dly_wait(5);
		os_mut_wait(&playerMut, 0xffff);
		os_mut_wait(&zombieMut, 0xffff);
		moveZombies();
		os_mut_release(&zombieMut);
		os_mut_release(&playerMut);
		os_tsk_pass();
	}
}

__task void spawnZombies(){
	while(1){
		os_mut_wait(&zombieMut, 0xffff);
		spawnZombieWave();
		os_mut_release(&zombieMut);
		os_dly_wait(1500);
		os_tsk_pass();
	}
}

__task void gameover(){
	while(1){
		GLCD_SetTextColor(Blue);
		GLCD_DisplayString(5, 5, 1, "Game Over");
		os_tsk_pass();
	}
}

//Initialization Functions
void mapInit(){
	int i; 
	int j; 
	
	for(i = 0; i <= 60; i++){
		for(j = 0; j <= 80; j++){
			if(((i >= 10) && (i < 15) && (j >= 40) && (j < 70)) ||
				 ((i >= 15) && (i < 20) && (j >= 65) && (j < 70)) || 
				 ((i >= 30) && (i < 50) && (j >= 15) && (j < 25)) ||
				 ((i >= 45) && (i < 50) && (j >= 25) && (j < 30)) ||
				 ((i >= 25) && (i < 35) && (j >= 35) && (j < 45)) || 
				 ((i >= 35) && (i < 50) && (j >= 55) && (j < 70)) ||
				 ((i >= 10) && (i < 20) && (j >= 10) && (j < 15))
			){
				map[i][j] = 1;
				if((i*4) % 20 == 0 && (j*4) % 20 == 0){
					GLCD_Bitmap(j * 4, i * 4, 20, 20, (unsigned char*)treeBitmap);
				}
			}
			else{ 
				map[i][j] = 0;
			}
		}
	}
}

void initialize(){
	// initiate and set GLCD background to white 
	GLCD_Init();
	GLCD_Clear(White);
	GLCD_DisplayString(3, 2, 1, "Press the button");
	GLCD_DisplayString(4, 3, 1, "to begin game.");
	// setting LED directions
	LPC_GPIO1->FIODIR |= (1 << 28);
	LPC_GPIO1->FIODIR |= (1 << 29);
	LPC_GPIO1->FIODIR |= (1 << 31);
	LPC_GPIO2->FIODIR |= 0x0000007C;
}

// acquire a "random" number triggered by the player hitting the button to start the game 
int acquireRand(){
	int count;
	while((LPC_GPIO2->FIOPIN>>10 & 1)){
		count++;
	}
	GLCD_Clear(White);
	return count;
}

int main(void){
	//printf("Start");
	initialize();
	randomSeed = acquireRand();
	srand(randomSeed);
	mapInit();
	zombieInit();
	playerInit(&player);
	os_sys_init(playerMovement);
	os_mut_init(&zombieMut);
	os_mut_init(&playerMut);
	while(1){};
}