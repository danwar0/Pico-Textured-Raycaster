#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/time.h"

#include "Infrared/Infrared.h"
#include "LCD/LCD_1in3.h"
#include "DEV_Config.h"

//Colour Is Formatted:
//Blue is bits 3,4,5,6,7
//Red is bits 8,9,10,11,12
//Green is bits 13,14,15,0,1,2

typedef struct{
    int x, y;
    int dir; //degrees
    int velocity;
}Player;

#define PI 3.141592654

#define ACCURACY 1
#define ACCURACYSCALE 1000 

#define CELLRESOLUTION 1000
#define TRIGSCALE 10000

#define TEXTUREWIDTH 4
#define TEXTUREHEIGHT 4

//GLOBAL VARIABLES
const uint UP = 2;
const uint DOWN = 18;
const uint LEFT = 16;
const uint RIGHT = 20;
const uint CTRL = 3;
const uint A = 15;
const uint B = 17;
const uint X = 19;
const uint Y = 21;

//LOOKUP TABLES

int* sinTable; //SCALED BY TRIGSCALE
int* cosTable; //SCALED BY TRIGSCALE
int* tanTable; //SCALED BY TRIGSCALE

int* fovAngleTable;

Player player;

int MAP[10][10] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,2,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,3,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

int FLOOR[10][10] = {
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
};


UWORD texture1[100] = {
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0xffff,
    0xffff, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff,
    0xffff, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
};

UWORD floorTexture[100] = {
    0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 
    0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 
    0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 
    0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 
    0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 
    0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 
    0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 
    0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 
    0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 
    0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 0x00ff, 0xff00, 
};



UWORD textureA[16] = {
    0xff00, 0x00ff, 0xff00, 0x00ff,
    0x00ff, 0xff00, 0x00ff, 0xff00,
    0xff00, 0x00ff, 0xff00, 0x00ff,
    0x00ff, 0xff00, 0x00ff, 0xff00
};

UWORD textureB[16] = {
    0xf00f, 0x0ff0, 0xf00f, 0x0ff0,
    0x0ff0, 0xf00f, 0x0ff0, 0xf00f,
    0xf00f, 0x0ff0, 0xf00f, 0x0ff0,
    0x0ff0, 0xf00f, 0x0ff0, 0xf00f
};

UWORD textureC[16] = {
    0x0000, 0xffff, 0x0000, 0xffff,
    0xffff, 0x0000, 0xffff, 0x0000,
    0x0000, 0xffff, 0x0000, 0xffff,
    0xffff, 0x0000, 0xffff, 0x0000
};



UWORD getTextureColour(UWORD* texture, int x, int y){
    if(x < 0 || y < 0){
        return 0;
    }
    if(x > TEXTUREWIDTH || y > TEXTUREHEIGHT){
        return 0;
    }
    if(x == TEXTUREWIDTH){
        x-=1;
    }
    if(y == TEXTUREHEIGHT){
        y-=1;
    }

    return texture[y*TEXTUREWIDTH+x];
}



int getCell(int x, int y){
    int arrX = x / CELLRESOLUTION;
    int arrY = y / CELLRESOLUTION;
    if(arrX < 0 || arrY < 0 || arrX >= 10 || arrY >= 10){
        return 1;
    }
    return MAP[arrY][arrX];
}

int cellFloor(int val){
    return (val / CELLRESOLUTION) * CELLRESOLUTION;
}

int cellCeil(int val){
    return (val / CELLRESOLUTION) * CELLRESOLUTION + CELLRESOLUTION;
}



void raycast(int angle, int* distanceOutput, int* cellIdOutput, bool* isHorizontalOutput, int* textureX){
    int fx, fy, dx, dy;
    int currentStep = 0;
    if(angle == 0){
        //directly right
        fx = cellCeil(player.x) - player.x;
        while(getCell(player.x+fx+CELLRESOLUTION*currentStep + ACCURACY, player.y) == 0){
            currentStep++;
        }
        *distanceOutput = fx+CELLRESOLUTION*currentStep;
        *cellIdOutput = getCell(player.x+fx+CELLRESOLUTION*currentStep + ACCURACY, player.y);
        *isHorizontalOutput = false;
        *textureX = player.y - cellFloor(player.y);
        return;
    }
    if(angle == 180){
        //directly left
        fx = player.x - cellFloor(player.x);
        while(getCell(player.x-fx-CELLRESOLUTION*currentStep - ACCURACY, player.y) == 0){
            currentStep++;
        }
        *distanceOutput = fx+CELLRESOLUTION*currentStep;
        *cellIdOutput = getCell(player.x-fx-CELLRESOLUTION*currentStep - ACCURACY, player.y);
        *isHorizontalOutput = false;
        *textureX = cellCeil(player.y) - player.y;
        return;
    }
    if(angle == 90){
        //directly up
        fy = player.y - cellFloor(player.y);
        while(getCell(player.x, player.y-fy-CELLRESOLUTION*currentStep - ACCURACY) == 0){
            currentStep++;
        }
        *distanceOutput = fy+CELLRESOLUTION*currentStep;
        *cellIdOutput = getCell(player.x, player.y-fy-CELLRESOLUTION*currentStep - ACCURACY);
        *isHorizontalOutput = true;
        *textureX = player.x - cellFloor(player.x);
        return;
    }
    if(angle == 270){
        //directly down
        fy = cellCeil(player.y) - player.y;
        while(getCell(player.x, player.y+fy+CELLRESOLUTION*currentStep + ACCURACY) == 0){
            currentStep++;
        }
        *distanceOutput = fy+CELLRESOLUTION*currentStep;
        *cellIdOutput = getCell(player.x, player.y+fy+CELLRESOLUTION*currentStep + ACCURACY);
        *isHorizontalOutput = true;
        *textureX = cellCeil(player.x) - player.x;
        return;
    }


    int horizontalDistance, verticalDistance;
    int horizontalVal, verticalVal;
    int horizontalIntersectionX, horizontalIntersectionY;
    int verticalIntersectionX, verticalIntersectionY;

    //horizontal
    currentStep = 0;
    if(angle > 0 && angle < 180){
        //up
        fy = player.y - cellFloor(player.y);
        fx = fy*TRIGSCALE/tanTable[angle];
        dx = CELLRESOLUTION*TRIGSCALE/tanTable[angle];
        horizontalIntersectionX = player.x+fx;
        horizontalIntersectionY = player.y-fy;
        while(getCell(horizontalIntersectionX, (horizontalIntersectionY*ACCURACYSCALE - ACCURACY)/ACCURACYSCALE) == 0){
            currentStep++;
            horizontalIntersectionX = player.x+fx+dx*currentStep;
            horizontalIntersectionY = player.y-fy-CELLRESOLUTION*currentStep;
        }

        int fd = fabs(fy*TRIGSCALE / sinTable[angle]);
        int dd = fabs(CELLRESOLUTION*TRIGSCALE / sinTable[angle]);

        horizontalDistance = fd+currentStep*dd;
        horizontalVal = getCell(horizontalIntersectionX, (horizontalIntersectionY*ACCURACYSCALE - ACCURACY)/ACCURACYSCALE);
    }else{
        //down
        fy = cellCeil(player.y) - player.y;
        fx = fy*TRIGSCALE/tanTable[angle];
        dx = CELLRESOLUTION*TRIGSCALE/tanTable[angle];
        horizontalIntersectionX = player.x-fx;
        horizontalIntersectionY = player.y+fy;
        while(getCell(horizontalIntersectionX, (horizontalIntersectionY*ACCURACYSCALE + ACCURACY)/ACCURACYSCALE) == 0){
            currentStep++;
            horizontalIntersectionX = player.x-fx-dx*currentStep;
            horizontalIntersectionY = player.y+fy+CELLRESOLUTION*currentStep;
        }

        float fd = fabs(fy*TRIGSCALE / sinTable[angle]);
        float dd = fabs(CELLRESOLUTION*TRIGSCALE / sinTable[angle]);

        horizontalDistance = fd+currentStep*dd;
        horizontalVal = getCell(horizontalIntersectionX, (horizontalIntersectionY*ACCURACYSCALE + ACCURACY)/ACCURACYSCALE);
    }

    //vertical
    currentStep = 0;
    if(angle > 90 && angle < 270){
        //left
        fx = player.x - cellFloor(player.x);
        fy = fx*tanTable[angle]/TRIGSCALE;
        dy = CELLRESOLUTION*tanTable[angle]/TRIGSCALE;
        verticalIntersectionX = player.x-fx;
        verticalIntersectionY = player.y+fy;
        while(getCell((verticalIntersectionX*ACCURACYSCALE - ACCURACY)/ACCURACYSCALE, verticalIntersectionY) == 0){
            currentStep++;
            verticalIntersectionX = player.x-fx-CELLRESOLUTION*currentStep;
            verticalIntersectionY = player.y+fy+dy*currentStep;
        }

        float fd = fabs(fx*TRIGSCALE / cosTable[angle]);
        float dd = fabs(CELLRESOLUTION*TRIGSCALE / cosTable[angle]);

        verticalDistance = fd+currentStep*dd;
        verticalVal = getCell((verticalIntersectionX*ACCURACYSCALE - ACCURACY)/ACCURACYSCALE, verticalIntersectionY);
    }else{
        //right
        fx = cellCeil(player.x) - player.x;
        fy = fx*tanTable[angle]/TRIGSCALE;
        dy = CELLRESOLUTION*tanTable[angle]/TRIGSCALE;
        verticalIntersectionX = player.x+fx;
        verticalIntersectionY = player.y-fy;
        while(getCell((verticalIntersectionX*ACCURACYSCALE + ACCURACY)/ACCURACYSCALE, verticalIntersectionY) == 0){
            currentStep++;
            verticalIntersectionX = player.x+fx+CELLRESOLUTION*currentStep;
            verticalIntersectionY = player.y-fy-dy*currentStep;
        }

        float fd = fabs(fx*TRIGSCALE / cosTable[angle]);
        float dd = fabs(CELLRESOLUTION*TRIGSCALE / cosTable[angle]);

        verticalDistance = fd+currentStep*dd;
        verticalVal = getCell((verticalIntersectionX*ACCURACYSCALE + ACCURACY)/ACCURACYSCALE, verticalIntersectionY);
    }
    



    if(horizontalDistance < verticalDistance){
        //horizontal
        *distanceOutput = horizontalDistance;
        *cellIdOutput = horizontalVal;
        *isHorizontalOutput = true;
        if(angle > 0 && angle < 180){
            //up
            *textureX = horizontalIntersectionX - cellFloor(horizontalIntersectionX);
        }else{
            //down
            *textureX = cellCeil(horizontalIntersectionX) - horizontalIntersectionX;
        }
    }else{
        //vertical
        *distanceOutput = verticalDistance;
        *cellIdOutput = verticalVal;
        *isHorizontalOutput = false;
        if(angle > 90 && angle < 270){
            //left
            *textureX = cellCeil(verticalIntersectionY) - verticalIntersectionY;
        }else{
            //right
            *textureX = verticalIntersectionY - cellFloor(verticalIntersectionY);
        }
    }

}




//red from 0-31
//green from 0-63
//blue from 0-31
UWORD makeColour(uint red, uint green, uint blue){
    if(red > 0b11111){
        red = 0b11111;
    };
    if(green > 0b111111){
        green = 0b111111;
    }
    if(blue > 0b11111){
        blue = 0b11111;
    }
    return 0b0000000000000000 | (blue & 0b11111)<<8  | (red & 0b11111)<<3 | (green & 0b111000)>>3 | (green & 0b111)<<13;
}

void clearScreenBuffer(UWORD* screenBuffer){
    for(int i = 0; i < 57600; i++){
        screenBuffer[i] = 0;
    }
}

void drawBackground(UWORD* screenBuffer){
    for(int i = 0; i < 28800; i++){
        screenBuffer[i] = 0b1111110110111111;
    }
    for(int i = 28800; i < 57600; i++){
        screenBuffer[i] = makeColour(31/4,63/4,31/4);
    }
}



int main(){
    DEV_Module_Init();
    LCD_1IN3_Init(HORIZONTAL);

    //Set Up Player
    player.x = 4*CELLRESOLUTION + CELLRESOLUTION/2;
    player.y = 4*CELLRESOLUTION + CELLRESOLUTION/2;
    player.dir = 0;
    player.velocity = CELLRESOLUTION/10 * 2;

    //Set Up Trig Tables
    sinTable = malloc(sizeof(int)*360);
    cosTable = malloc(sizeof(int)*360);
    tanTable = malloc(sizeof(int)*360);
    for(int i = 0; i < 360; i++){
        sinTable[i] = (int)(sin((i / 180.0) * PI)*TRIGSCALE);
        cosTable[i] = (int)(cos((i / 180.0) * PI)*TRIGSCALE);
        tanTable[i] = (int)(tan((i / 180.0) * PI)*TRIGSCALE);
    }

    //Set Up Angles For Screen Rays
    fovAngleTable = malloc(sizeof(int)*240);
    for(int i = 0; i < 240; i++){
        fovAngleTable[i] = (atan(i/120.0 - 1.0) / PI) * 180.0; 
    }

    //Set Up Controls
    SET_Infrared_PIN(UP);
    SET_Infrared_PIN(DOWN);
    SET_Infrared_PIN(LEFT);
    SET_Infrared_PIN(RIGHT);

    SET_Infrared_PIN(B);
    SET_Infrared_PIN(X);

    //Set Up Screen Buffer
    UWORD* screenBuffer = malloc(sizeof(UWORD)*57600);
    for(int i = 0; i < 57600; i++){
        screenBuffer[i] = 0xff00;
    }

    LCD_1IN3_Display(screenBuffer);

    while(1){
        //Controls
        if(DEV_Digital_Read(UP) == 0){
            player.x += (player.velocity*cosTable[player.dir])/TRIGSCALE;
            player.y -= (player.velocity*sinTable[player.dir])/TRIGSCALE;
        }
        if(DEV_Digital_Read(DOWN) == 0){
            player.x -= (player.velocity*cosTable[player.dir])/TRIGSCALE;
            player.y += (player.velocity*sinTable[player.dir])/TRIGSCALE;
        }
        if(DEV_Digital_Read(RIGHT) == 0){
            player.x -= (player.velocity*sinTable[player.dir])/TRIGSCALE;
            player.y -= (player.velocity*cosTable[player.dir])/TRIGSCALE;
        }
        if(DEV_Digital_Read(LEFT) == 0){
            player.x += (player.velocity*sinTable[player.dir])/TRIGSCALE;
            player.y += (player.velocity*cosTable[player.dir])/TRIGSCALE;
        }
        
        if(DEV_Digital_Read(B) == 0){
            player.dir += 5;
            if(player.dir >= 360){
                player.dir = 0;
            }
        }
        if(DEV_Digital_Read(X) == 0){
            player.dir -= 5;
            if(player.dir < 0){
                player.dir = 359;
            }
        }
        
        
        //Render
        drawBackground(screenBuffer);

        for(int i = 0; i < 240; i++){
            int angle = player.dir + fovAngleTable[i];
            if(angle < 0){
                angle += 360;
            }
            if(angle >= 360){
                angle -= 360;
            }

            int distance;
            int cellID;
            bool isHorizontal;
            int textureX;

            raycast(angle, &distance, &cellID, &isHorizontal, &textureX);

            distance = (distance * cosTable[abs(fovAngleTable[i])]) / TRIGSCALE;

            int columnHeight = ((120*CELLRESOLUTION) / distance);
            if(columnHeight > 240){
                columnHeight = 240;
            }

            UWORD* currentTexture;
            if(isHorizontal){
                currentTexture = textureA;
            }else{
                currentTexture = textureB;
            }
            
            //draw wall column
            for(int j = 0; j < columnHeight; j++){
                int tex1 = textureX*TEXTUREWIDTH/CELLRESOLUTION;
                int tex2 = j*TEXTUREHEIGHT/columnHeight;
                if(tex1 < 0){
                    tex1 += TEXTUREWIDTH;
                }
                if(tex2 < 0){
                    tex2 += TEXTUREHEIGHT;
                }
                UWORD pixelCol = getTextureColour(currentTexture, tex1, tex2);
                //UWORD pixelCol = 0xffff;
                screenBuffer[(120-columnHeight/2+j)*240+i] = pixelCol;
            }


            //draw floor
            int floorStartY = 120+columnHeight/2;
            for(int f = floorStartY; f < 240; f++){
                int floorY = 60*CELLRESOLUTION / (f - 120); //positive
                int floorX = (floorY * (i - 120)) / 120; //can be negative

                //rotate floorX and floorY
                int rotatedFloorX = (floorX*cosTable[player.dir])/TRIGSCALE+(floorY*sinTable[player.dir])/TRIGSCALE;
                int rotatedFloorY = (-1*floorX*sinTable[player.dir])/TRIGSCALE+(floorY*cosTable[player.dir])/TRIGSCALE;

                //translate floorX and floorY
                rotatedFloorX -= player.y;
                rotatedFloorY += player.x;

                int floorTextureX = rotatedFloorX - cellFloor(rotatedFloorX);
                int floorTextureY = rotatedFloorY - cellFloor(rotatedFloorY);

                int fx = floorTextureX*TEXTUREWIDTH / CELLRESOLUTION;
                int fy = floorTextureY*TEXTUREHEIGHT / CELLRESOLUTION;

                if(fx < 0){
                    fx += TEXTUREWIDTH;
                }
                if(fy < 0){
                    fy += TEXTUREHEIGHT;
                }

                UWORD pixelCol = getTextureColour(textureC, fx, fy);

                screenBuffer[f*240+i] = pixelCol;
            }
            
            


        }

        LCD_1IN3_Display(screenBuffer);
    
    }

    return 0;
}