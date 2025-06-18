/**\
 *素材来源：
 *洛琪希图片以及背景：https://www.bilibili.com/opus/481545660152627561   (原作截图但是我扣了图)
 *背景音乐:【【无职】洛琪希真是太可爱啦！】 https://www.bilibili.com/video/BV1ty4y1n7cu/?share_source=copy_web&vd_source=037e5b544530f055fe448f702a14f015   (我剪了一下)
\**/


#define SDL_MAIN_HANDLED
#define MINIAUDIO_IMPLEMENTATION
#include <iostream>
#include <string.h>
#include <fstream>
#include <thread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "JSON/json.hpp"
#include "AUDIO/miniaudio.h"
#define BLOCK_SIZE 40

const char TITLE[100] = "RoxySnake";

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int SPEED = BLOCK_SIZE;
const int FRAME_DELAY = 120;
using json = nlohmann::json;

bool menu_running = true;
bool running = false;
bool paused = false;

bool food_exists = false;
bool* bonus_exists = (bool*)malloc(sizeof(bool));

int score_multiplier = 100;

typedef struct snake{
    int xPos;
    int yPos;
    snake* next;
}Snake;
enum Key {
    Up, Down, Left, Right
};
int score = 0;
Snake* head = nullptr;
Key currentKey = Key::Right;
int* food_xPos = (int*)malloc(sizeof(int));
int* food_yPos = (int*)malloc(sizeof(int));
int snake_length = 1;
int speed_multiplier = 1;
Uint32 frame_start = 0;
SDL_Rect* food_rect = (SDL_Rect*)malloc(sizeof(SDL_Rect));
ma_engine engine;
bool save_file_exists = false;

bool background_music_playing = true;
void menu(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font);

SDL_Window* initWindow() {
    SDL_Window* window = nullptr;
    window = SDL_CreateWindow(TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        return nullptr;
    }
    SDL_Surface* icon = SDL_LoadBMP("roxy.bmp");
    SDL_SetWindowIcon(window, icon);
    return window;
}

SDL_Renderer* initRenderer(SDL_Window* window) {
    SDL_Renderer* renderer = nullptr;
    renderer = SDL_CreateRenderer(window , -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        return nullptr;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_RenderSetIntegerScale(renderer,SDL_FALSE);
    return renderer;
}

void initial_game() {
    score = 0;
    head = (Snake*)malloc(sizeof(Snake));
    head->xPos = 100;
    head->yPos = 100;
    head->next = nullptr;
    currentKey = Key::Right;
    std::ifstream save_file;
    if (std::filesystem::exists("save_file.json")) {
        save_file_exists = true;
    }
    else {
        save_file_exists = false;
    }
}

void save_game(Snake* head,int score) {
    json data;
    Snake* current = head;
    int index = 0;
    while (current != nullptr) {
        data["Snake"][index]["xPos"] = current->xPos;
        data["Snake"][index]["yPos"] = current->yPos;
        index++;
        current = current->next;
    }
    data["score"] = score;
    data["Key"] = currentKey;
    std::ofstream save_file;
    if (std::filesystem::exists("save_file.json")) {
        std::filesystem::remove_all("save_file.json");
    }
    save_file.open("save_file.json",std::ios::trunc);
    save_file << data;
    save_file.close();
}

void load_game() {
    json data;
    std::ifstream save_file;
    if (!std::filesystem::exists("save_file.json")) {
        //std::cerr << "Save file not found" << std::endl;
        return;
    }
    save_file.open("save_file.json",std::ios::in);
    save_file >> data;
    save_file.close();
    int index = 0;
    head->xPos = data["Snake"][0]["xPos"];
    head->yPos = data["Snake"][0]["yPos"];
    Snake* last = head;
    index++;
    while (data["Snake"][index]["xPos"] != nullptr) {
        //std::cout << "snake" << index << std::endl;
        Snake* current = (Snake*)malloc(sizeof(Snake));
        current->xPos = data["Snake"][index]["xPos"];
        current->yPos = data["Snake"][index]["yPos"];
        last->next = current;
        index++;
        last = current;
    }
    last->next = nullptr;
    int data_score = data["score"];
    score += data_score;
    currentKey = data["Key"];
}
void save_success(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Rect save_rect = {(SCREEN_WIDTH-200)/2,(SCREEN_HEIGHT-600)/2,200,40};
    SDL_Surface* save_text = TTF_RenderUTF8_Blended(font, "游戏已保存" , SDL_Color{255,255,255,255});
    SDL_Texture* save_texture = SDL_CreateTextureFromSurface(renderer, save_text);
    SDL_RenderCopy(renderer, save_texture, NULL, &save_rect);
    SDL_FreeSurface(save_text);
    SDL_DestroyTexture(save_texture);
    SDL_RenderPresent(renderer);
}
void input(SDL_Event event) {
    Key key = currentKey;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                if (head->next!=nullptr && head->yPos > head->next->yPos) {
                    break;
                }
                key = Key::Up;
                break;
            case SDLK_DOWN:
                if (head->next!=nullptr && head->yPos < head->next->yPos) {
                    break;
                }
                key = Key::Down;
                break;
            case SDLK_LEFT:
                if (head->next!=nullptr && head->xPos > head->next->xPos) {
                    break;
                }
                key = Key::Left;
                break;
            case SDLK_RIGHT:
                if (head->next!=nullptr && head->xPos < head->next->xPos) {
                    break;
                }
                key = Key::Right;
                break;
            case SDLK_SPACE:
                speed_multiplier = 2;
                break;
            default:
                key = currentKey;
                break;
        }
    }
    if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                speed_multiplier = 1;
                break;
            default:
                break;
        }
    }
    currentKey = key;
}

void move(Snake* head,Key key) {
    Snake* p = head;
    int temp_xPos = p->xPos;
    int temp_yPos = p->yPos;
    while (p->next != nullptr) {
        int temp_xPos_ = p->next->xPos;
        int temp_yPos_ = p->next->yPos;
        p->next->xPos = temp_xPos;
        p->next->yPos = temp_yPos;
        temp_xPos = temp_xPos_;
        temp_yPos = temp_yPos_;
        p = p->next;
    }
    switch (key) {
        case Key::Up:
            head->yPos -= SPEED;
            break;
        case Key::Down:
            head->yPos += SPEED;
            break;
        case Key::Left:
            head->xPos -= SPEED;
            break;
        case Key::Right:
            head->xPos += SPEED;
            break;
    }
    head->xPos = (((head->xPos + SCREEN_WIDTH)%SCREEN_WIDTH)/BLOCK_SIZE)*BLOCK_SIZE;
    head->yPos = (((head->yPos + SCREEN_HEIGHT)%SCREEN_HEIGHT)/BLOCK_SIZE)*BLOCK_SIZE;
}


void Update(SDL_Window* window, SDL_Renderer* renderer, Snake* head) {
    move(head,currentKey);
    Snake* p = head;

    while (p!=nullptr) {
        SDL_SetRenderDrawColor(renderer,86,170,255,255);
        SDL_Rect rect = {p->xPos,p->yPos,BLOCK_SIZE,BLOCK_SIZE};
        SDL_RenderFillRect(renderer, &rect);
        p = p->next;
    }
    SDL_Surface* head_surface = SDL_LoadBMP("head.bmp");
    SDL_Texture* head_texture = SDL_CreateTextureFromSurface(renderer, head_surface);
    SDL_SetTextureScaleMode(head_texture,SDL_ScaleModeBest);
    SDL_Rect head_rect = {head->xPos-1*BLOCK_SIZE,head->yPos-1*BLOCK_SIZE ,3*BLOCK_SIZE,3*BLOCK_SIZE};
    SDL_RenderCopy(renderer, head_texture, NULL, &head_rect);
    SDL_FreeSurface(head_surface);
    SDL_DestroyTexture(head_texture);
}

void generate_food(SDL_Window* window, SDL_Renderer* renderer) {

    if (!food_exists) {

        bool flag;
        do {
            Snake* p = head;
            flag = true;
            *food_xPos = (int)((SCREEN_WIDTH * 0.1 + rand() % SCREEN_WIDTH * 0.8)/BLOCK_SIZE) * BLOCK_SIZE;
            *food_yPos = (int)((SCREEN_HEIGHT * 0.1 + rand() % SCREEN_HEIGHT * 0.8)/BLOCK_SIZE) * BLOCK_SIZE;
            //std::cout << "Food Position Refresh To: ("<< *food_xPos <<"," << *food_yPos <<")"<< std::endl;
            while (p->next!=nullptr) {
                if (*food_xPos == p->xPos && *food_yPos == p->yPos) {
                    flag = false;
                }
                p = p->next;
            }
        }while (!flag);
        bool bonus_food = false;
        int rand_num = rand() % 100;
        if (rand_num < 20) {
            bonus_food = true;
        }
        *bonus_exists = bonus_food;
        //std::cout << "Head Position: " << head->xPos << ", " << head->yPos << std::endl;
        //std::cout << "Food Position: " << *food_xPos << ", " << *food_yPos << std::endl;
        *food_rect = {*food_xPos,*food_yPos,BLOCK_SIZE,BLOCK_SIZE};
        if (bonus_food) {
            score_multiplier = 150;
        }
        else {
            score_multiplier = 100;
        }
        food_exists = true;
    }
    SDL_SetRenderDrawColor(renderer, 255, 0 ,0 ,255);
    if (*bonus_exists) {
        SDL_SetRenderDrawColor(renderer, 0, 255 ,0 ,255);
    }
    SDL_RenderFillRect(renderer, food_rect);
    //printf("Food Position: %d,%d\n",*food_xPos,*food_yPos);
}

Uint32 eat_time = NULL;
int score_add_x = NULL;
int score_add_y = NULL;
int score_add = NULL;
void eat_food(SDL_Window* window, SDL_Renderer* renderer) {
    if (food_exists) {
        if (abs(head->xPos - *food_xPos) < BLOCK_SIZE && abs(head->yPos - *food_yPos) < BLOCK_SIZE) {
            food_exists = false;
            //printf("Food Eating");
            Snake* new_body = (Snake*)malloc(sizeof(Snake));
            Snake* tail = head;
            int index = 1;
            while (tail->next != nullptr) {
                tail = tail->next;
                index++;
            }
            snake_length = index;
            //std::cout << snake_length << std::endl;
            tail->next = new_body;
            new_body->next = nullptr;
            score_add = snake_length * score_multiplier;
            score += score_add;
            eat_time = SDL_GetTicks();
            score_add_x = head->xPos;
            score_add_y = head->yPos;
            //std::cout << "FOOD EAT! SCORE UP!" << std::endl << "YOUR SCORE IS: " << score << std::endl;
        }
    }
}

void score_up(SDL_Window* window, SDL_Renderer* renderer,TTF_Font* font) {
    if (SDL_GetTicks() - eat_time < 500 && eat_time != NULL) {
        int length_of_score_add = 1;
        int temp = score_add;
        while (temp > 9) {
            temp /= 10;
            length_of_score_add ++;
        }
        char score_add_str[100];
        sprintf(score_add_str,"+%d",score_add);
        SDL_Rect rect = {score_add_x,score_add_y,20+length_of_score_add*20,30};
        SDL_Surface* add_score = TTF_RenderUTF8_Blended(font,score_add_str,SDL_Color{255,200,0,255});
        SDL_Texture* add_score_texture = SDL_CreateTextureFromSurface(renderer, add_score);
        SDL_SetTextureScaleMode(add_score_texture,SDL_ScaleModeBest);
        SDL_RenderCopy(renderer, add_score_texture, NULL, &rect);
        SDL_DestroyTexture(add_score_texture);

    }
}

void quit_game(SDL_Window* window, SDL_Renderer* renderer,Snake* head) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Snake *current = head;
    while (current!=nullptr) {
        Snake* p = current->next;
        free(current);
        current = p;
    }
    free(head);
    free(food_xPos);
    free(food_yPos);
    free(food_rect);
    menu_running = false;
    running = false;
    ma_engine_uninit(&engine);
    background_music_playing = false;
    TTF_Quit();
    SDL_Quit();
}

bool game_over_back_to_menu_button_focus = false;
void game_over(SDL_Window* window, SDL_Renderer* renderer, int score,TTF_Font* font) {
    char scoreString[100];
    sprintf(scoreString, "%d", score);
    char message[200] = "游戏结束! 你的得分是: ";
    strcat(message, scoreString);
    int length_of_score = 1;
    int temp = score;
    while (temp > 9) {
        temp /= 10;
        length_of_score++;
    }
    //std::cout << message << std::endl;

    SDL_Rect game_over_rect = {(SCREEN_WIDTH-(11*50+length_of_score*25))/2,(SCREEN_HEIGHT-300)/2,11*50+length_of_score*25,50};
    SDL_Surface* game_over = TTF_RenderUTF8_Blended(font, message, SDL_Color{255,255,255,255});
    SDL_Texture* game_over_texture = SDL_CreateTextureFromSurface(renderer,game_over);
    SDL_RenderCopy(renderer,game_over_texture, NULL, &game_over_rect);
    SDL_DestroyTexture(game_over_texture);

    SDL_Rect back_to_menu_rect ={(SCREEN_WIDTH-5*50)/2,SCREEN_HEIGHT/2,5*50,50};
    SDL_Surface* back_to_menu = TTF_RenderUTF8_Blended(font, "返回主菜单", SDL_Color{255,255,255,255});
    SDL_Texture* back_to_menu_texture = SDL_CreateTextureFromSurface(renderer,back_to_menu);


    SDL_SetRenderDrawColor(renderer,86,170,255,255);
    SDL_RenderFillRect(renderer,&back_to_menu_rect);//蓝色

    SDL_RenderCopy(renderer,back_to_menu_texture, NULL, &back_to_menu_rect);
    SDL_DestroyTexture(back_to_menu_texture);


    SDL_RenderPresent(renderer);
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (game_over_back_to_menu_button_focus) {
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderDrawRect(renderer,&back_to_menu_rect);
        }
        else {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_RenderDrawRect(renderer,&back_to_menu_rect);
        }
        SDL_RenderPresent(renderer);
        if (event.type == SDL_QUIT) {
            quit_game(window, renderer,head);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.x > back_to_menu_rect.x && event.button.x < back_to_menu_rect.x+back_to_menu_rect.w &&
                event.button.y > back_to_menu_rect.y && event.button.y < back_to_menu_rect.y+back_to_menu_rect.h) {
                //std::cout << "back_to_menu" << std::endl;
                initial_game();
                menu_running = true;
                running = false;
                menu(window,renderer,font);
                break;
            }
        }
        if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.x > back_to_menu_rect.x && event.motion.x < back_to_menu_rect.x+back_to_menu_rect.w &&
                event.motion.y > back_to_menu_rect.y && event.motion.y < back_to_menu_rect.y+back_to_menu_rect.h) {
                game_over_back_to_menu_button_focus = true;
            }
            else {
                game_over_back_to_menu_button_focus = false;
            }
        }
    }
}

void snake_collision(SDL_Window* window, SDL_Renderer* renderer, Snake* head,TTF_Font* font) {
    Snake* p = head;
    bool collision = false;
    if (p->next!=nullptr) {
        p = p->next;
    }
    while (p->next != nullptr && p != head) {
        if (head->xPos == p->xPos && head->yPos == p->yPos) {
            collision = true;
        }
        p = p->next;
    }
    if (collision) {
        game_over(window, renderer, score,font);
    }
}



void pause(SDL_Window* window, SDL_Renderer* renderer,TTF_Font* font) {
    bool pause_continue_button_focus = false;
    bool pause_save_button_focus = false;
    bool pause_quit_button_focus = false;
    paused = true;
    SDL_Rect pause_menu = {(SCREEN_WIDTH-400)/2,(SCREEN_HEIGHT-400)/2,400,400};
    SDL_Rect continue_button = {(SCREEN_WIDTH-200)/2,(SCREEN_HEIGHT-250)/2, 200,50};
    SDL_Rect save_button = {(SCREEN_WIDTH-200)/2,(SCREEN_HEIGHT-50)/2, 200,50};
    SDL_Rect quit_button = {(SCREEN_WIDTH-250)/2,(SCREEN_HEIGHT+150)/2, 250,50};

    SDL_Surface* continue_text = TTF_RenderUTF8_Blended(font, "返回游戏", SDL_Color{255,255,255,255});
    SDL_Surface* save_text = TTF_RenderUTF8_Blended(font, "保存游戏", SDL_Color{255,255,255,255});
    SDL_Surface* quit_text = TTF_RenderUTF8_Blended(font, "返回主菜单", SDL_Color{255,255,255,255});

    SDL_Texture* continue_text_texture = SDL_CreateTextureFromSurface(renderer, continue_text);
    SDL_Texture* save_text_texture = SDL_CreateTextureFromSurface(renderer, save_text);
    SDL_Texture* quit_text_texture = SDL_CreateTextureFromSurface(renderer, quit_text);

    SDL_SetTextureScaleMode(continue_text_texture,SDL_ScaleModeBest);
    SDL_SetTextureScaleMode(save_text_texture,SDL_ScaleModeBest);
    SDL_SetTextureScaleMode(quit_text_texture,SDL_ScaleModeBest);


    //菜单界面
    SDL_SetRenderDrawColor(renderer, 255, 255 ,255 ,50);//半透明白色
    SDL_RenderFillRect(renderer,&pause_menu);

    //色块
    SDL_SetRenderDrawColor(renderer, 255, 170 ,86 ,255);//橙色
    SDL_RenderFillRect(renderer,&continue_button);
    SDL_SetRenderDrawColor(renderer, 153, 153 ,255 ,255);//紫色
    SDL_RenderFillRect(renderer,&save_button);
    SDL_SetRenderDrawColor(renderer, 86,170,255,255);//蓝色
    SDL_RenderFillRect(renderer,&quit_button);

    //字
    SDL_RenderCopy(renderer, continue_text_texture, NULL, &continue_button);
    SDL_RenderCopy(renderer, save_text_texture, NULL, &save_button);
    SDL_RenderCopy(renderer, quit_text_texture, NULL, &quit_button);
    SDL_FreeSurface(continue_text);
    SDL_FreeSurface(save_text);
    SDL_FreeSurface(quit_text);
    SDL_DestroyTexture(continue_text_texture);
    SDL_DestroyTexture(save_text_texture);
    SDL_DestroyTexture(quit_text_texture);

    if (paused) {

        SDL_Event event;
        while (SDL_WaitEvent(&event)) {
            if (pause_continue_button_focus) {
                SDL_SetRenderDrawColor(renderer, 255, 255 ,255 ,255);
                SDL_RenderDrawRect(renderer,&continue_button);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 0 ,0 ,255);
                SDL_RenderDrawRect(renderer,&continue_button);
            }
            if (pause_save_button_focus) {
                SDL_SetRenderDrawColor(renderer, 255, 255 ,255 ,255);
                SDL_RenderDrawRect(renderer,&save_button);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 0 ,0 ,255);
                SDL_RenderDrawRect(renderer,&save_button);
            }
            if (pause_quit_button_focus) {
                SDL_SetRenderDrawColor(renderer, 255, 255 ,255 ,255);
                SDL_RenderDrawRect(renderer,&quit_button);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 0 ,0 ,255);
                SDL_RenderDrawRect(renderer,&quit_button);
            }
            SDL_RenderPresent(renderer);
            if (event.type == SDL_QUIT) {
                quit_game(window, renderer, 0);
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    paused = false;
                    SDL_Delay(200);
                    break;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.x > continue_button.x && event.button.x < continue_button.x + continue_button.w &&
                    event.button.y > continue_button.y && event.button.y < continue_button.y + continue_button.h) {
                    paused = false;
                    SDL_Delay(200);
                    break;
                    }
                if (event.button.x > save_button.x && event.button.x < save_button.x + save_button.w &&
                    event.button.y > save_button.y && event.button.y < save_button.y + save_button.h) {
                    save_game(head,score);
                    save_success(window,renderer,font);
                    }
                if (event.button.x > quit_button.x && event.button.x < quit_button.x + quit_button.w &&
                    event.button.y > quit_button.y && event.button.y < quit_button.y + quit_button.h) {
                    //返回主菜单
                    initial_game();
                    menu_running = true;
                    running = false;
                    paused = false;
                    menu(window,renderer,font);
                    break;

                    }
            }
            if (event.type == SDL_MOUSEMOTION) {
                if (event.motion.x > continue_button.x && event.motion.x < continue_button.x + continue_button.w &&
                    event.motion.y > continue_button.y && event.motion.y < continue_button.y + continue_button.h ) {
                    pause_continue_button_focus = true;
                    }
                else if (event.motion.x > save_button.x && event.motion.x < save_button.x + save_button.w &&
                    event.motion.y > save_button.y && event.motion.y < save_button.y + save_button.h ) {
                    pause_save_button_focus = true;
                    }
                else if (event.motion.x > quit_button.x && event.motion.x < quit_button.x + quit_button.w &&
                    event.motion.y > quit_button.y && event.motion.y < quit_button.y + quit_button.h ) {
                    pause_quit_button_focus = true;
                    }
                else {
                    pause_continue_button_focus = false;
                    pause_save_button_focus = false;
                    pause_quit_button_focus = false;
                }
            }
        }


    }

}



void display_score(SDL_Window* window, SDL_Renderer* renderer,TTF_Font* font) {
    int length_of_score = 1;
    int temp = score;
    //获取分数的长度
    while (temp > 9) {
        temp /= 10;
        length_of_score++;
    }
    SDL_Rect score_rect = {10, 10, 120, 40};
    SDL_Rect score_to_text_rect = {130, 10, length_of_score * 25, 40};
    char scoreString[50];
    sprintf(scoreString, "%d", score);
    char score_String[50] = "分数: ";
    SDL_Surface* score_text = TTF_RenderUTF8_Blended(font, score_String , SDL_Color{255,255,255,200});//白色
    SDL_Texture* score_text_texture = SDL_CreateTextureFromSurface(renderer, score_text);
    SDL_SetTextureScaleMode(score_text_texture, SDL_ScaleModeBest);

    SDL_Surface* score_to_text = TTF_RenderUTF8_Blended(font, scoreString, SDL_Color{0,95,191,255});//深蓝色(得分)
    SDL_Texture* score_to_text_texture = SDL_CreateTextureFromSurface(renderer, score_to_text);
    SDL_SetTextureScaleMode(score_to_text_texture, SDL_ScaleModeBest);

    SDL_RenderCopy(renderer, score_text_texture, NULL, &score_rect);
    SDL_RenderCopy(renderer, score_to_text_texture, NULL, &score_to_text_rect);
    SDL_FreeSurface(score_text);
    SDL_FreeSurface(score_to_text);
    SDL_DestroyTexture(score_text_texture);
    SDL_DestroyTexture(score_to_text_texture);

    SDL_Rect pause_tutorial_rect = {SCREEN_WIDTH-200,10,160,20};
    SDL_Surface* pause_tutorial_surface = TTF_RenderUTF8_Blended(font, "按下ESC暂停游戏", SDL_Color{255,255,255,150});
    SDL_Texture* pause_tutorial_texture = SDL_CreateTextureFromSurface(renderer, pause_tutorial_surface);
    SDL_SetTextureScaleMode(pause_tutorial_texture, SDL_ScaleModeBest);
    SDL_Rect rush_tutorial_rect = {SCREEN_WIDTH-160,40,120,20};
    SDL_Surface* rush_tutorial_surface = TTF_RenderUTF8_Blended(font, "长按空格加速", SDL_Color{255,255,255,150});
    SDL_Texture* rush_tutorial_texture = SDL_CreateTextureFromSurface(renderer, rush_tutorial_surface);
    SDL_SetTextureScaleMode(rush_tutorial_texture, SDL_ScaleModeBest);
    SDL_RenderCopy(renderer, pause_tutorial_texture, NULL, &pause_tutorial_rect);
    SDL_RenderCopy(renderer, rush_tutorial_texture, NULL, &rush_tutorial_rect);
    SDL_FreeSurface(rush_tutorial_surface);
    SDL_FreeSurface(pause_tutorial_surface);
    SDL_DestroyTexture(rush_tutorial_texture);
    SDL_DestroyTexture(pause_tutorial_texture);

}

void menu(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    char MENU_TITLE[50] = "洛琪希开饭啦!";
    TTF_SetFontOutline(font,10);
    SDL_Surface* title_outline_surface = TTF_RenderUTF8_Blended(font, MENU_TITLE, SDL_Color{86,170,255,255});
    SDL_Texture* title_outline_texture = SDL_CreateTextureFromSurface(renderer, title_outline_surface);
    SDL_SetTextureScaleMode(title_outline_texture, SDL_ScaleModeBest);
    TTF_SetFontOutline(font,0);
    SDL_Surface* title_surface = TTF_RenderUTF8_Blended(font, MENU_TITLE, SDL_Color{255,255,255,255});
    SDL_Texture* title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
    SDL_SetTextureScaleMode(title_texture, SDL_ScaleModeBest);

    SDL_Surface* start_text = TTF_RenderUTF8_Blended(font, "开始游戏", SDL_Color{255,255,255,255});
    SDL_Texture* start_text_texture = SDL_CreateTextureFromSurface(renderer, start_text);
    SDL_SetTextureScaleMode(start_text_texture,SDL_ScaleModeBest);

    SDL_Surface* load_text = TTF_RenderUTF8_Blended(font, "继续游戏", SDL_Color{255,255,255,255});
    SDL_Texture* load_text_texture = SDL_CreateTextureFromSurface(renderer, load_text);
    SDL_SetTextureScaleMode(load_text_texture,SDL_ScaleModeBest);

    SDL_Surface* end_text = TTF_RenderUTF8_Blended(font, "退出游戏", SDL_Color{255,255,255,255});
    SDL_Texture* end_text_texture = SDL_CreateTextureFromSurface(renderer, end_text);
    SDL_SetTextureScaleMode(end_text_texture,SDL_ScaleModeBest);

    SDL_Rect title_rect = {(SCREEN_WIDTH-660)/2,150,660,100};
    SDL_Rect start_button = {(SCREEN_WIDTH-200)/2, 350, 200, 50};
    SDL_Rect load_button = {(SCREEN_WIDTH-200)/2, 470, 200, 50};
    SDL_Rect end_button = {(SCREEN_WIDTH-200)/2, 590, 200, 50};

    SDL_Rect menu_background = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    SDL_Surface* menu_background_surface = SDL_LoadBMP("roxy.bmp");
    SDL_Texture* menu_background_texture = SDL_CreateTextureFromSurface(renderer, menu_background_surface);
    SDL_SetTextureScaleMode(menu_background_texture, SDL_ScaleModeBest);

    SDL_Rect menu_panel = {(SCREEN_WIDTH-600)/2, (SCREEN_HEIGHT-600)/2, 600, 600};
    bool start_button_focus = false;
    bool load_button_focus = false;
    bool end_button_focus = false;

    while (menu_running) {
        //清屏
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);



        SDL_RenderCopy(renderer, menu_background_texture, NULL, &menu_background);


        //色块

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);//一个半透明的方块
        SDL_RenderFillRect(renderer, &menu_panel);

        SDL_SetRenderDrawColor(renderer, 255, 170 ,86 ,255);//橙色
        SDL_RenderFillRect(renderer, &start_button);
        if (save_file_exists) {
            SDL_SetRenderDrawColor(renderer, 153, 153, 255, 255);//紫色
        }
        else {
            SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);//灰色
        }
        SDL_RenderFillRect(renderer, &load_button);
        SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);//灰色
        SDL_RenderFillRect(renderer, &end_button);
        //字
        SDL_RenderCopy(renderer, title_outline_texture, NULL, &title_rect);
        SDL_RenderCopy(renderer, title_texture,NULL, &title_rect);
        SDL_RenderCopy(renderer, start_text_texture, NULL, &start_button);
        SDL_RenderCopy(renderer, load_text_texture, NULL, &load_button);
        SDL_RenderCopy(renderer, end_text_texture, NULL, &end_button);


        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                //开始游戏
                if (event.button.x >= start_button.x && event.button.x <= start_button.x + start_button.w &&
                    event.button.y >= start_button.y && event.button.y <= start_button.y + start_button.h) {
                    running = true;
                    menu_running = false;
                    SDL_FreeSurface(menu_background_surface);
                    SDL_FreeSurface(title_outline_surface);
                    SDL_FreeSurface(title_surface);
                    SDL_FreeSurface(start_text);
                    SDL_FreeSurface(end_text);
                    SDL_FreeSurface(load_text);
                    SDL_DestroyTexture(menu_background_texture);
                    SDL_DestroyTexture(title_outline_texture);
                    SDL_DestroyTexture(title_texture);
                    SDL_DestroyTexture(start_text_texture);
                    SDL_DestroyTexture(load_text_texture);
                    SDL_DestroyTexture(end_text_texture);
                    //std::cout << "Start button clicked" << std::endl;
                    break;
                    }
                if (event.button.x >= load_button.x && event.button.x <= load_button.x + load_button.w &&
                    event.button.y >= load_button.y && event.button.y <= load_button.y + load_button.h && save_file_exists) {
                    load_game();
                    running = true;
                    menu_running = false;
                    SDL_FreeSurface(menu_background_surface);
                    SDL_FreeSurface(title_outline_surface);
                    SDL_FreeSurface(title_surface);
                    SDL_FreeSurface(start_text);
                    SDL_FreeSurface(end_text);
                    SDL_FreeSurface(load_text);
                    SDL_DestroyTexture(menu_background_texture);
                    SDL_DestroyTexture(title_outline_texture);
                    SDL_DestroyTexture(title_texture);
                    SDL_DestroyTexture(start_text_texture);
                    SDL_DestroyTexture(load_text_texture);
                    SDL_DestroyTexture(end_text_texture);

                    break;
                    }
                if (event.button.x >= end_button.x && event.button.x <= end_button.x + end_button.w &&
                    event.button.y >= end_button.y && event.button.y <= end_button.y + end_button.h) {
                    //std::cout << "End button clicked" << std::endl;
                    quit_game(window, renderer,head);
                    }
            }
            if (event.type == SDL_MOUSEMOTION) {

                if (event.motion.x > start_button.x && event.motion.x < start_button.x + start_button.w &&
                        event.motion.y > start_button.y && event.motion.y < start_button.y + start_button.h) {
                    start_button_focus = true;
                        }
                else if (event.motion.x > load_button.x && event.motion.x < load_button.x + load_button.w &&
                        event.motion.y > load_button.y && event.motion.y < load_button.y + load_button.h) {
                    load_button_focus = true;
                        }
                else if (event.motion.x > end_button.x && event.motion.x < end_button.x + end_button.w &&
                        event.motion.y > end_button.y && event.motion.y < end_button.y + end_button.h) {
                    end_button_focus = true;
                        }
                else {
                    start_button_focus = false;
                    load_button_focus = false;
                    end_button_focus = false;
                }

            }
            if (event.type == SDL_QUIT) {
                quit_game(window, renderer,head);
            }

        }
        if (start_button_focus) {
            SDL_RenderDrawRect(renderer, &start_button);
        }
        if (load_button_focus && save_file_exists) {
            SDL_RenderDrawRect(renderer, &load_button);
        }
        if (end_button_focus) {
            SDL_RenderDrawRect(renderer, &end_button);
        }
        SDL_RenderPresent(renderer);
    }
}


void play_background_music() {
    ma_sound background_music1,background_music2;
    ma_sound_init_from_file(&engine,"background_music_1.wav",MA_SOUND_FLAG_ASYNC,0,0,&background_music1);
    ma_sound_init_from_file(&engine,"background_music_2.wav",MA_SOUND_FLAG_ASYNC,0,0,&background_music2);
    ma_sound_start(&background_music1);
    while (ma_sound_is_playing(&background_music1)) {
        ma_sleep(10);
    }
    while (background_music_playing) {
        ma_sound_start(&background_music2);
        while (ma_sound_is_playing(&background_music2)) {
            ma_sleep(10);
        }
    }
    ma_sound_stop(&background_music1);
    ma_sound_stop(&background_music2);
    ma_sound_uninit(&background_music1);
    ma_sound_uninit(&background_music2);
}



int main() {
    initial_game();

    srand(time(0));
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() < 0) {
        std::cout << TTF_GetError() << std::endl;
        return 1;
    }
    ma_engine_init(NULL,&engine);

    SDL_Window* window = initWindow();
    SDL_Renderer* renderer = initRenderer(window);
    TTF_Font* font = TTF_OpenFont("MizukiGothic-Regular.ttf", 100);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    std::thread audio_thread(play_background_music);
    audio_thread.detach();
    menu(window, renderer, font);

    while (running && !paused) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                input(event);
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        pause(window, renderer, font);
                    }
                }

                //std::cout << "KEYPRESSED: " << event.key.keysym.sym << std::endl;
            }

        }
        if (SDL_GetTicks() - frame_start > FRAME_DELAY / speed_multiplier) {
            SDL_SetRenderDrawColor(renderer, 0, 0 ,0 ,255);
            SDL_RenderClear(renderer);
            //背景
            SDL_Rect background_rect = {0,0,SCREEN_WIDTH,SCREEN_WIDTH};
            SDL_Surface* background_surface = SDL_LoadBMP("background.bmp");
            SDL_Texture* background_texture = SDL_CreateTextureFromSurface(renderer, background_surface);
            SDL_RenderCopy(renderer, background_texture, NULL, &background_rect);
            SDL_FreeSurface(background_surface);
            SDL_DestroyTexture(background_texture);

            generate_food(window, renderer);
            Update(window, renderer, head);
            display_score(window,renderer,font);
            eat_food(window, renderer);
            score_up(window,renderer,font);
            SDL_RenderPresent(renderer);
            snake_collision(window, renderer, head,font);
            frame_start = SDL_GetTicks();
            //std::cout << "CurrentFrame: " << frame_start << std::endl;
        }

        //std::cout << "SCORE: " << score << std::endl;

        //std::cout << frame_end - frame_start << std::endl;
    }
    quit_game(window, renderer,head);

    return 0;
}
