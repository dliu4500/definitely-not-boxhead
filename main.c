#include <lpc17xx.h>
#include "GLCD.h"
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
	unsigned short pBitmap[9];
	unsigned short pInvBitmap[9];
	int x, y;	
	int direction, health; 
}player_t;

struct zombie_t{
	int x, y;
	struct zombie_t *next;
};

//Global Variables

OS_MUT zombieMut;
OS_MUT playerMut;

player_t player;
bool map[60][80];
struct zombie_t *head = NULL;
int randomSeed;
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
	player->health = 8;
	player->x = 120;
	player->y = 160;
	
	LPC_GPIO2->FIOSET |= (1 << 6);	
	LPC_GPIO2->FIOSET |= (1 << 5);
	LPC_GPIO2->FIOSET |= (1 << 4);
	LPC_GPIO2->FIOSET |= (1 << 3);
	LPC_GPIO2->FIOSET |= (1 << 2);
	LPC_GPIO1->FIOSET |= (1 << 31);
	LPC_GPIO1->FIOSET |= (1 << 29);
	LPC_GPIO1->FIOSET |= (1 << 28);

}


void updatePlayerMovement(){	
	int left = LPC_GPIO1->FIOPIN>>23 & 1;
	int right = LPC_GPIO1->FIOPIN>>25 & 1;
	int up = LPC_GPIO1->FIOPIN>>24 & 1;
	int down = LPC_GPIO1->FIOPIN>>26 & 1;
	
	if(!left && player.x > 2){
		player.direction = 4;
		if(!map[(player.x - 2) / 4][(player.y) / 4]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.x -= 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			//printf("left %d , %d \n", player.x, player.prevX);
		}
	}
	else if(!right && player.x < 236){
		player.direction = 2;
		if(!map[(player.x + 4) / 4][player.y / 4]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.x += 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			//printf("right %d , %d \n", player.x, player.prevX);
		}
	}
	else if(!up && player.y < 316){
		player.direction = 1;
		if(!map[(player.x) / 4][(player.y + 4) / 4]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.y += 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			//printf("up %d , %d \n", player.y, player.prevY);
		}
	}
	else if(!down && player.y > 2){
		player.direction = 3;
		if(!map[(player.x) / 4][(player.y - 2) / 4]){
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pInvBitmap));
			player.y -= 2;
			GLCD_Bitmap(player.y,player.x, 3, 3, (unsigned char*)(player.pBitmap));
			//printf("down %d , %d \n", player.y, player.prevY);
		}
	}
}

void zombieErase(int x, int y){
		GLCD_Bitmap(y,x, 3, 3, (unsigned char*)(zInvBitmap));
}

void checkZombieCollision(int direction, int length){
	struct zombie_t * dummy = head;
	struct zombie_t * the_bitch_dummy = head;
	
	while(dummy != NULL){
		if(direction == 1 && abs( player.x - (dummy->x) ) < 2 && player.y < (dummy->y) ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(the_bitch_dummy);
				dummy = head;
				the_bitch_dummy = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				the_bitch_dummy->next = dummy->next;
				free(dummy);
				dummy = the_bitch_dummy->next;
			}
		}
		else if(direction == 2 && abs( player.y - (dummy->y) ) < 2 && player.x < (dummy->x) ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(the_bitch_dummy);
				dummy = head;
				the_bitch_dummy = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				the_bitch_dummy->next = dummy->next;
				free(dummy);
				dummy = the_bitch_dummy->next;
			}
		}
		else if(direction == 3 && abs( player.x - (dummy->x) ) < 2 && player.y > (dummy->y) ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(the_bitch_dummy);
				dummy = head;
				the_bitch_dummy = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				the_bitch_dummy->next = dummy->next;
				free(dummy);
				dummy = the_bitch_dummy->next;
			}
		}
		else if(direction == 4 && abs( player.y - (dummy->y) ) < 2 && player.x > (dummy->x) ){
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(the_bitch_dummy);
				dummy = head;
				the_bitch_dummy = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				the_bitch_dummy->next = dummy->next;
				free(dummy);
				dummy = the_bitch_dummy->next;
			}
		}
		else{
			the_bitch_dummy = dummy;
			//if(dummy != NULL){
				dummy = dummy->next;
			//}
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
		while(shotCoordinate <= 320 && !map[player.x / 4][shotCoordinate / 4]){
			// check if hit zombie, if hit, delete zombie, and manipulate linked list 
			shotCoordinate++;
		}
		checkZombieCollision(player.direction, shotLength);
		shotLength = shotCoordinate - player.y;
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 2){
				shotCoordinate = player.x + 2;
		while(shotCoordinate <= 240 && !map[shotCoordinate / 4][player.y / 4]){
			shotCoordinate++;
		}
		checkZombieCollision(player.direction, shotLength);
		shotLength = shotCoordinate - player.x;
		drawShot(player.direction, shotLength);
	}
	else if(player.direction == 3){
				shotCoordinate = player.y;
		while(shotCoordinate >= 0 && !map[player.x / 4][shotCoordinate / 4]){
			shotCoordinate--;
		}
		checkZombieCollision(player.direction, shotLength);
		shotLength =  player.y - shotCoordinate;
		drawShot(player.direction, shotLength);
	}
	else{
			shotCoordinate = player.x;
		while(shotCoordinate >= 0 && !map[shotCoordinate / 4][player.y / 4]){
			shotCoordinate--;
		}
		checkZombieCollision(player.direction, shotLength);
		shotLength = player.x - shotCoordinate;
		drawShot(player.direction, shotLength);
	}
}

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
		printf("hello");
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
	
	zInvBitmap[0] = White;
	zInvBitmap[1] = White;
	zInvBitmap[2] = White;
	zInvBitmap[3] = White;
	zInvBitmap[4] = White;
	zInvBitmap[5] = White;
	zInvBitmap[6] = White;
	zInvBitmap[7] = White;
	zInvBitmap[8] = White;
	
}

void spawnZombieWave(){
	struct zombie_t * dummy = head;
	int numZombiesPerWave = (rand() % 6) + 20;
	int i;
	int spawnSide = 0;
	
	if(head == NULL){
		// create list

		head = malloc(sizeof(struct zombie_t));
		head -> next = NULL;
		spawnSide = (rand() % 4);
		if(spawnSide == 0){
			head -> x = 10;
			head -> y = (rand() % 310);
		}
		else if(spawnSide == 1){
			head -> y = 10;
			head -> x = (rand() % 230);
		}
		else if(spawnSide == 2){
			head -> x = 230;
			head -> y = (rand() % 310);
		}
		else{
			head -> y = 310;
			head -> x = (rand() % 230);
		}
		GLCD_Bitmap(head->y, head->x, 3, 3, (unsigned char*)(zBitmap));
		numZombiesPerWave--;
	}
	
	dummy = head;
	
	while(dummy -> next != NULL){
		dummy = dummy->next;
	} 
	
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
		GLCD_Bitmap(derek_at_8am->y, derek_at_8am->x, 3, 3, (unsigned char*)(zBitmap));
		dummy = dummy -> next;
	}
		
	dummy -> next = NULL; // edited code 
	
}

void moveZombies(){
	struct zombie_t * dummy = head;
	struct zombie_t * bitch = head;
	//int count = 0;
	
	while(dummy != NULL){
		int deltaX = player.x - dummy->x;
		int deltaY = player.y - dummy->y;
		
		zombieErase(dummy->x, dummy->y);
		
		if(abs(deltaX) > abs(deltaY)){
			if(!map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y) / 4]){
				dummy->x += deltaX / abs(deltaX);
			}
			else{
				if(!map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4]){
					dummy->y += deltaY / abs(deltaY);
				}
			}
			GLCD_Bitmap(dummy->y,dummy->x, 3, 3, (unsigned char*)(zBitmap));
		}
		else {
			if(!map[(dummy->x)/ 4][((dummy->y) + deltaY/abs(deltaY)) / 4]){
				dummy->y += deltaY / abs(deltaY);
			}
			else{
				if(!map[((dummy->x) + deltaX/abs(deltaX)) / 4][(dummy->y) / 4]){
					dummy->x += deltaX / abs(deltaX);
				}
			}			
			GLCD_Bitmap(dummy->y,dummy->x, 3, 3, (unsigned char*)(zBitmap));
		}
		
		if(dummy->x == player.x && dummy->y == player.y){
			decrementHealth();
			if(dummy == head){
				zombieErase(dummy->x, dummy->y);
				head = head->next;
				free(bitch);
				dummy = head;
				bitch = head;
			}
			else{
				zombieErase(dummy->x, dummy->y);
				bitch->next = dummy->next;
				free(dummy);
				dummy = bitch->next;
			}
		}
		else{
			bitch = dummy;
			//if(dummy != NULL){
				dummy = dummy->next;
			//}
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
		os_dly_wait(10);
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
		os_dly_wait(15);
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
		os_dly_wait(2000);
		os_tsk_pass();
	}
}

__task void gameover(){
	while(1){
		printf("heyho");
		//GLCD_SetBackColor(31);
		GLCD_SetTextColor(Blue);
		GLCD_DisplayString(5, 5, 1, "Game Over");
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
	
	for(i = 0; i <= 60; i++){
		for(j = 0; j <= 80; j++){
			if(i >= 40 && i <= 50 && j >= 60 && j <= 70){
				map[i][j] = 1;
				GLCD_Bitmap(j * 4, i * 4, 2, 2, (unsigned char*)obstacleBitmap);
			}
			else map[i][j] = 0;
		}
	}
}

void initialize(){
	GLCD_Init();
	GLCD_Clear(White);
	
	// setting LED directions
	LPC_GPIO1->FIODIR |= (1 << 28);
	LPC_GPIO1->FIODIR |= (1 << 29);
	LPC_GPIO1->FIODIR |= (1 << 31);
	LPC_GPIO2->FIODIR |= 0x0000007C;
}

int acquireRand(){
	int count;
	while((LPC_GPIO2->FIOPIN>>10 & 1)){
		count++;
	}
	return count;
}

int main(void){
	printf("Start");
	initialize();
	randomSeed = acquireRand();
	//printf("%d \n", randomSeed);
	srand(randomSeed);
	printf(" %d ", rand());
	mapInit();
	zombieInit();
	playerInit(&player);
	os_sys_init(playerMovement);
	os_mut_init(&zombieMut);
	os_mut_init(&playerMut);
	while(1){};
}