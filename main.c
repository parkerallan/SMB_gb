#include <gb/gb.h>
#include <gb/metasprites.h>
#include <gbdk/font.h>
#include <stdio.h>
#include "GameCharacter.h"
#include "Metasprites.h"
#include "Mario.h"
#include "WorldTiles.h"
#include "SmallMap.h"
#include "TitleMap.h"

#define GRAVITY 2
#define JUMP_STRENGTH -16
#define FLOOR_Y 128
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define WORLD_WIDTH 160//328 3376
#define WORLD_HEIGHT 144//240 480
#define NUM_NON_COLLIDABLE_TILES (sizeof(nonCollidableTiles) / sizeof(nonCollidableTiles[0]))

UBYTE attacking;
UBYTE jumping;
UBYTE walking;
UBYTE onTitle;
UBYTE debug;
int8_t lives = 4;
int16_t score = 0;
int8_t world = 1;
int8_t level = 1;
//int16_t player_x = 80;
int16_t player_y = FLOOR_Y;
int16_t velocity_y;
const unsigned char nonCollidableTiles[] = {0x00, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};

struct GameCharacter mario;
struct GameCharacter goomba;
struct GameCharacter mushroom;
struct GameCharacter fireflower;

void performanceDelay(uint8_t numloops) {
    uint8_t i;
    for(i = 0; i < numloops; i++) {
        wait_vbl_done();
    }     
}

// void update_camera(uint16_t player_x, uint16_t player_y) {
//     uint16_t camera_x, camera_y;

//     // Calculate the camera position
//     camera_x = player_x - SCREEN_WIDTH / 2;
//     camera_y = player_y - SCREEN_HEIGHT / 2;

//     // Ensure the camera doesn't go out of bounds
//     if (camera_x < 0) {
//         camera_x = 0;
//     }
//     if (camera_y < 0) {
//         camera_y = 0;
//     }
//     if (camera_x > WORLD_WIDTH - SCREEN_WIDTH) {
//         camera_x = WORLD_WIDTH - SCREEN_WIDTH;
//     }
//     if (camera_y > WORLD_HEIGHT - SCREEN_HEIGHT) {
//         camera_y = WORLD_HEIGHT - SCREEN_HEIGHT;
//     }

//     // Move the background to follow the player
//     move_bkg(camera_x, camera_y);
// }

void setupMario() {
    mario.x = 72;
    mario.y = 128;
    mario.width = 16;
    mario.height = 16;

    move_metasprite_ex(mario_metasprite, 0, 0, 0, mario.x, mario.y);
}

// void setupMario2() {
//     mario.width = 16;
//     mario.height = 32;

//     move_metasprite_ex(mario2_metasprite, 0, 0, 0, mario.x, mario.y);
// }

// void setupGoomba() {
//     goomba.x = 80;
//     goomba.y = 130;
//     goomba.width = 16;
//     goomba.height = 16;

//     move_metasprite_ex(goomba_metasprite, 0, 0, 0, goomba.x, goomba.y);
// }

// void setupMushroom() {
//     mushroom.x = 80; //specify tile indexes for fireflower
//     mushroom.y = 130;
//     mushroom.width = 16;
//     mushroom.height = 16;

//     move_metasprite_ex(mushroom_metasprite, 0, 0, 0, mushroom.x, mushroom.y);
// }

// void setupFireflower() {
//     fireflower.x = 80; //specify tile indexes for fireflower
//     fireflower.y = 130;
//     fireflower.width = 16;
//     fireflower.height = 16;

// }

int16_t check_floor_collision(int16_t y) {
    if (y >= FLOOR_Y) {
        return FLOOR_Y;
    }
    return -1; // No collision
}

void jump() {
    if (!jumping) {
        velocity_y = JUMP_STRENGTH;
        jumping = 1;
    }

    if (jumping) {
        velocity_y += GRAVITY;
        player_y += velocity_y;

        int16_t floor_y = check_floor_collision(player_y); 
        if (floor_y != -1) { //change floor y for collision of tile
            player_y = floor_y;
            velocity_y = 0;
            jumping = 0;
        }
    }

    if (jumping) {
        move_metasprite_ex(mario_jump_metasprite, 0, 0, 0, mario.x, player_y);
    } else {
        move_metasprite_ex(mario_metasprite, 0, 0, 0, mario.x, player_y);
    }
}

// fireball for Mario
void attack() {
    attacking = 1;
}

void titleScreen(){
    set_bkg_data(0, 123, WorldTiles);
    set_bkg_tiles(0, 0, 20, 18, TitleMap);
    waitpad(J_START | J_A);
}

void stage1_1(){
    set_sprite_data(0, 20, MarioTiles);
    set_bkg_data(0, 123, WorldTiles);
    set_bkg_submap(0, 0, 20, 18, SmallMap, 20);
}

void worldScreen(){
    printf("\n\n\n\n\n\n\n      World %u-%u", world, level);
    performanceDelay(60);
}

void pauseScreen(){
    printf("\n\n\n\n\n\n\n        Paused");
}

void gameoverScreen(){
    printf("\n\n\n\n\n\n\n       Game Over");
    printf("\n\n\n\n\n\n\n      Score: %u", score);
}

void livesScreen(){
    printf("\n\n\n\n\n\n\n      Lives: %u", lives);
}

// UBYTE isHit(struct GameCharacter* mario, struct GameCharacter* object) { //item collision
//     return (mario->x >= object->x && mario->x <= object->x + object->width) && (mario->y >= object->y && mario->y <= object->y + object->height) || (object->x >= mario->x && object->x <= mario->x + mario->width) && (object->y >= mario->y && object->y <= mario->y + mario->height);
// }

// UBYTE isHitEnemy(struct GameCharacter* mario, struct GameCharacter* object) { //enemy collision jumping on top
//     return (mario->x >= object->x && mario->x <= object->x + object->width) && (mario->y >= object->y && mario->y <= object->y + object->height) || (object->x >= mario->x && object->x <= mario->x + mario->width) && (object->y >= mario->y && object->y <= mario->y + mario->height);
// } //adjust this

UBYTE isNonCollidable(unsigned char tile) {
    for (UBYTE i = 0; i < NUM_NON_COLLIDABLE_TILES; i++) {
        if (nonCollidableTiles[i] == tile) {
            return 1; // Tile is non-collidable
        }
    }
    return 0; // Tile is collidable
}

UBYTE canMove(uint16_t newplayer_x, uint16_t newplayer_y) { //background collision
    uint16_t indexTLx, indexTLy, tileIndex;

    indexTLx = (newplayer_x) / 8;
    indexTLy = (newplayer_y - 16) / 8;
    tileIndex = 20 * indexTLy + indexTLx;

    if(debug){
        printf("X: %u, Y: %u\n", (uint16_t)(newplayer_x), (uint16_t)(newplayer_y));
        printf("TX: %u, TY: %u, Tile: %u\n", (uint16_t)(indexTLx), (uint16_t)(indexTLy), (uint16_t)(tileIndex));
        printf("Tile value: %u\n", (uint16_t)(SmallMap[tileIndex]));
    }

    return isNonCollidable(SmallMap[tileIndex]);
}

void walkAnimation() {
    static UBYTE frame = 0;
    frame++;
    walking = 1;
    if (frame == 1) {
        move_metasprite_ex(mario_walk_frame1, 0, 0, 0, mario.x, mario.y);
    } else if (frame == 2) {
        move_metasprite_ex(mario_walk_frame2, 0, 0, 0, mario.x, mario.y);
    } else if (frame == 3) {
        move_metasprite_ex(mario_walk_frame3, 0, 0, 0, mario.x, mario.y);
        walking = 0;
        frame = 0;
    }
}

void revwalkAnimation() {
    static UBYTE frame = 0;
    frame++;
    walking = 1;
    if (frame == 1) {
        move_metasprite_flipx(mario_walk_frame3, 0, 0, 0, mario.x, mario.y);
    } else if (frame == 2) {
        move_metasprite_flipx(mario_walk_frame2, 0, 0, 0, mario.x, mario.y);
    } else if (frame == 3) {
        move_metasprite_flipx(mario_walk_frame1, 0, 0, 0, mario.x, mario.y);
        frame = 0;
        walking = 0;
    }
}

void main() {
    DISPLAY_ON;
    SHOW_BKG;
    titleScreen();
    worldScreen();
    SHOW_SPRITES;
    stage1_1();
    setupMario();

    while(1) {
        // if(isHit(&mario, &goomba)){
        //     if(mario.height == 32){
        //         mario.height = 16;
        //         move_metasprite_ex(mario_metasprite, 0, 0, 0, mario.x, mario.y);
        //     }
        //     if(mario.height == 16){
        //         HIDE_BKG;
        //         HIDE_SPRITES;
        //         if(lives > 0){
        //             lives--;
        //             livesScreen();
        //             performanceDelay(60);
        //             stage1_1();
        //         }
        //         else {
        //             gameoverScreen();
        //             performanceDelay(60);
        //             titleScreen();
        //         }
        //     }
        // }

        // if(isHit(&mario, &mushroom)){
        //     setupMario2();
        // }
        // if((joypad() & J_START) && onTitle){
        //     score = 0;
        //     world = 1;
        //     level = 1;
        //     onTitle = 0;
        //     //stage1_1();
        // }
        // if((joypad() & J_START) && lives == 0){
        //     //titleScreen();
        //     onTitle = 1;
        // }
        // if((joypad() & J_START) && lives > 0){
        //     //pauseScreen();
        // }
        if((joypad() & J_B)){
            debug = 1;
        }
        if((joypad() & J_SELECT)){
            debug = 0;
        }
        if((joypad() & J_A) || jumping == 1) {
            if(canMove(mario.x, mario.y - 4)){
                if(move_metasprite_flipx(mario_metasprite, 0, 0, 0, mario.x, mario.y)){
                    move_metasprite_flipx(mario_jump_metasprite, 0, 0, 0, mario.x, player_y);
                }
                jump();
            }
        }
        // if(joypad() & J_B) { //|| attacking == 1
        //     if(isHit(&mario, &fireflower)){
        //     attack();
        // }
        if(joypad() & J_LEFT) {
            if(canMove(mario.x - 4, mario.y)){
                if (mario.x > 22) {
                    mario.x -= 4;
                    revwalkAnimation();
                    if(!walking) {
                        move_metasprite_flipx(mario_metasprite, 0, 0, 0, mario.x, mario.y);
                    }
                    if (jumping) {
                        move_metasprite_flipx(mario_jump_metasprite, 0, 0, 0, mario.x, player_y);
                    }
                }
            }
        }
        if(joypad() & J_RIGHT) {
            if(canMove(mario.x + 4, mario.y)){
                mario.x += 4;
                walkAnimation();
                if(!walking) {
                    move_metasprite_ex(mario_metasprite, 0, 0, 0, mario.x, mario.y);
                }
                if (jumping) {
                    move_metasprite_ex(mario_jump_metasprite, 0, 0, 0, mario.x, player_y);
                }
            }
        }
        //update_camera(mario.x, mario.y);
        if (mario.x + mario.width > WORLD_WIDTH) mario.x = WORLD_WIDTH - mario.width;
        performanceDelay(4);
    }
}
