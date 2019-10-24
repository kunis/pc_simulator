
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <SDL2/SDL.h>
#include "uart.h"
#include "bitmap.h"
#define MONITOR_HOR_RES 176
#define MONITOR_VER_RES 176

#define SDL_REFR_PERIOD	    50	/*ms*/
static SDL_Window * window;
static SDL_Renderer * renderer;
static SDL_Texture * texture;
static uint32_t tft_fb[MONITOR_HOR_RES * MONITOR_VER_RES];
static volatile bool sdl_inited = false;
static volatile bool sdl_refr_qry = false;
static volatile bool sdl_quit_qry = false;
static int sdl_refr(void * param);

int quit_filter (void *userdata, SDL_Event * event);

void monitor_init(void)
{
    SDL_CreateThread(sdl_refr, "sdl_refr", NULL);

    while(sdl_inited == false); 
}

extern void screen_save(void);
void mouse_handler(SDL_Event *event)
{
    switch (event->type) {
        case SDL_MOUSEBUTTONUP:
                
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button != SDL_BUTTON_LEFT) {
                 screen_save();
            }
            break;
        case SDL_MOUSEMOTION:
            break;
    }

}


static int sdl_refr(void * param)
{
    (void)param;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetEventFilter(quit_filter, NULL);
    window = SDL_CreateWindow("emWIN Simulator",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            MONITOR_HOR_RES, MONITOR_VER_RES, 0);       /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, MONITOR_HOR_RES, MONITOR_VER_RES);

    memset(tft_fb, 77, MONITOR_HOR_RES * MONITOR_VER_RES * sizeof(uint32_t));
    SDL_UpdateTexture(texture, NULL, tft_fb, MONITOR_HOR_RES * sizeof(uint32_t));
    sdl_refr_qry = true;
    sdl_inited = true;

    while(sdl_quit_qry == false) {
        if(sdl_refr_qry != false) {
            sdl_refr_qry = false;
            printf("update\r\n");
            SDL_UpdateTexture(texture, NULL, tft_fb, MONITOR_HOR_RES * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        SDL_Event event;
	    while(SDL_PollEvent(&event)) {
            mouse_handler(&event);
	    }
        SDL_Delay(SDL_REFR_PERIOD);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    exit(0);

    return 0;
}

static uint32_t to_rgb888(uint32_t rgb)
{
    uint16_t r,g,b,a;
    r = rgb&0xFF;
    g = (rgb>>8)&0xFF;
    b = (rgb>>16)&0xFF;
    a = (rgb>>24)&0xFF;
    return (((uint32_t)r)<<16)|(g<<8)|b|(rgb&0xFF000000);
}

void monitor_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    int i =0;
    if(x2 < 0) return;
    if(y2 < 0) return;
    if(x1 > MONITOR_HOR_RES - 1) return;
    if(y1 > MONITOR_VER_RES - 1) return;

    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > MONITOR_HOR_RES - 1 ? MONITOR_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > MONITOR_VER_RES - 1 ? MONITOR_VER_RES - 1 : y2;

    int32_t x;
    int32_t y;
    uint32_t color24;

    for(y = act_y1; y <= act_y2; y++) {
        for(x = act_x1; x <= act_x2; x++) {
            color24 = to_rgb888(tft_fb[i]);
            tft_fb[y * MONITOR_HOR_RES + x] = color24;
            i++;
        }
    }


    sdl_refr_qry = true;
}

int quit_filter (void *userdata, SDL_Event * event)
{
    (void)userdata;

	if(event->type == SDL_QUIT) {
		sdl_quit_qry = true;
	}

	return 1;
}

void screen_save(void)
{
    BITMAPFILEHEADER    header = { 0 };
    BITMAPINFOHEADER    info = { 0 };
    uint8_t *pBitmap = malloc(176 * 176 * 3);
    int fd  = -1;

    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biWidth = 176;
    info.biHeight = 176;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = 0;

    uint32_t dwBitmapSize = 176 * 176 * 4;
     

    fd = creat("capture.bmp",0666);

    if (fd < 0)
    {
        free(pBitmap);
        return;
    }

    header.bfSize = dwBitmapSize + sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    header.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    header.bfType = 0x4D42;        // BM

    uint32_t    dwWritten = 0;
    write(fd, &header, sizeof(BITMAPFILEHEADER));
    write(fd, &info, sizeof(BITMAPINFOHEADER));
    int i = MONITOR_HOR_RES * MONITOR_VER_RES -1;
    int x, y;
    for (y = 175; y >=0;y--)
        for(x = 0;x<176;x++){
        uint32_t pixel = *(tft_fb + y*176 +x);
        write(fd, &pixel, 4);
        
    }

    close(fd);

    free(pBitmap);
}

int main(int argc,char *argv[])
{

    uint8_t sync = 0;
    uint32_t ret=0;
    hm_init_uart();

    monitor_init();
    while(1){
        sync = 0;
        ret = uart_transfer(&sync,1,UART_READ,0xFFFFFFFF);
        if((ret != 1)||(sync != 0x7F)){
            printf("%c",sync);
            continue;
        }
        printf("got sync\r\n");
        uart_transfer(tft_fb,MONITOR_HOR_RES*MONITOR_VER_RES*4,UART_READ,20*1000);
        monitor_fill(0,0,MONITOR_HOR_RES-1,MONITOR_VER_RES-1);
    }
    return 0;
}
