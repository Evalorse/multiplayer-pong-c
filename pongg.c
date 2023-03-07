#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

// Online pong

typedef struct Paddle {
	float x,y;
	float w,h;
} Paddle;

typedef struct Ball {
	float x, y;
	int w, h;
	float vx, vy;
	int state; // dead / alive (i.e. in play or not)
} Ball;

typedef struct GameState {
    int sock;
	int score[2];
	SDL_Renderer **renderer;
	Paddle paddles[2];
	Ball ball;
    char command[2];
} GameState;

typedef struct render_ti
{
    SDL_Renderer *renderer;
    GameState *game_state;
} render_ti;

typedef struct events_ti
{
    SDL_Window *window;
    GameState *game_state;

} events_ti;

void *send_data(void *data)
{
    GameState *game_state = (GameState*)data;
    int s = game_state->sock;
    printf("send data sock : %d\n", s);
    while (1)
        write(s, game_state->command, 3);

    return NULL;
}

void *receive_data(void *data)
{
    GameState *game_state = (GameState*)data;
    char buffer[256];
    int n;
    int s = game_state->sock;

    printf("recv data sock : %d\n", s);
    while((n = read(s, buffer, sizeof(buffer))))
    {
        perror("Read:");
        sscanf(buffer, "%f;%f;%f;%f;%f;%f;%f;%f;%d;%d",
                                      &game_state->ball.x,
                                      &game_state->ball.y,
                                      &game_state->ball.vx,
                                      &game_state->ball.vy,
                                      &game_state->paddles[0].x,
                                      &game_state->paddles[0].y,
                                      &game_state->paddles[1].x,
                                      &game_state->paddles[1].y,
                                      &game_state->score[0],
                                      &game_state->score[1]
                                      );
    }

    return NULL;
}

void doRender(SDL_Renderer *renderer, GameState *game_state)
{
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_Rect paddles[2] = {
                                {
                                    game_state->paddles[0].x,
                                    game_state->paddles[0].y,
                                    game_state->paddles[0].w,
                                    game_state->paddles[0].h,
                                },
                                {
                                    game_state->paddles[1].x,
                                    game_state->paddles[1].y,
                                    game_state->paddles[1].w,
                                    game_state->paddles[1].h,
                                }
                              };
        SDL_Rect net = {315, 0, 10, 60};

        SDL_Rect ball = {game_state->ball.x, game_state->ball.y, game_state->ball.w, game_state->ball.h};

        for (int i = 0; i < 5; i++) {
            SDL_RenderFillRect(renderer, &net);
            net.y += net.h + 50;
        }

        SDL_Rect score = {20,40,10, 30};
        for (int i = 0; i < game_state->score[0]; i++) {
            SDL_RenderFillRect(renderer, &score);
            score.x += 20;
        }

        score.x = 610;
        score.y = 40;
        score.w = 10;
        score.h = 30;

        for (int i = 0; i < game_state->score[1]; i++) {
            SDL_RenderFillRect(renderer, &score);
            score.x -= 20;
        }

        SDL_RenderFillRect(renderer, &ball);
        SDL_RenderFillRect(renderer, paddles);
        SDL_RenderFillRect(renderer, &paddles[1]);

        SDL_RenderPresent(renderer);
}

int keyState[4] = {0};
// void processEvents(SDL_Window *window, GameState* game_state)
void *processEvents(void *data)
{
    GameState *game_state = ((events_ti*)data)->game_state;
    SDL_Window *window = ((events_ti*)data)->window;

    SDL_Event event;
    int key = 0;

    while (SDL_PollEvent(&event)) {
        key = event.key.keysym.sym;
        switch(event.type) {
            case SDL_WINDOWEVENT_CLOSE:
                if(window) {
                    SDL_DestroyWindow(window);
                    window = NULL;
                }
                break;

            case SDL_QUIT:
                break;
            
            case SDL_KEYDOWN:
                if(key == SDLK_ESCAPE) {
                    break;
                }
                if (key == SDLK_SPACE) {
                    game_state->ball.vx = 1;
                    game_state->ball.vy = 1;
                }
                if(key == SDLK_w)
                    game_state->command[0] = 'w';
                if(key == SDLK_s)
                    game_state->command[0] = 's';
                if(key == SDLK_o)
                    game_state->command[0] = 'o';
                if(key == SDLK_l)
                    game_state->command[0] = 'l';
                break;
            
            case SDL_KEYUP:
                if(key == SDLK_w)
                    game_state->command[0] = 'x';
                if(key == SDLK_s)
                    game_state->command[0] = 'x';
                if(key == SDLK_o)
                    game_state->command[0] = 'x';
                if(key == SDLK_l)
                    game_state->command[0] = 'x';
                break;	
        }
        game_state->command[1] = '\0';
    }

    return NULL;
}

/*
void *ball(void *gs)
{
	while (1) {
	if ((*(GameState*)gs).ball.x + (*(GameState*)gs).ball.w >= 640 - (*(GameState*)gs).paddles[1].w && (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h / 2 >= (*(GameState*)gs).paddles[1].y && (*(GameState*)gs).ball.y < (*(GameState*)gs).paddles[1].y + (*(GameState*)gs).paddles[1].h)
		(*(GameState*)gs).ball.vx *= -1;

	if ((*(GameState*)gs).ball.x <= (*(GameState*)gs).paddles[0].w && (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h / 2 >= (*(GameState*)gs).paddles[0].y && (*(GameState*)gs).ball.y < (*(GameState*)gs).paddles[0].y + (*(GameState*)gs).paddles[0].h)
		(*(GameState*)gs).ball.vx *= -1;

	if ((*(GameState*)gs).ball.y <= 0 || (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h >= 480)
		(*(GameState*)gs).ball.vy *= -1;

	(*(GameState*)gs).ball.x += ((*(GameState*)gs).ball.vx);
	(*(GameState*)gs).ball.y += ((*(GameState*)gs).ball.vy);

	GameState *game_state = (GameState*)gs;

	if(game_state->ball.x < 0) {
		game_state->ball.x = 315;
		game_state->ball.y = 235;

		game_state->ball.vx = 0;
		game_state->ball.vy = 0;

		game_state->score[1]++;
	}
	if (game_state->ball.x + game_state->ball.w > 640) {
		//game_state->ball.state = 0; // See what i can do with that to reset ball position
		game_state->ball.x = 315;
		game_state->ball.y = 235;

		game_state->ball.vx = 0;
		game_state->ball.vy = 0;

		game_state->score[0]++;
	}
	SDL_Delay(2);
	}
}
*/


int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock > 0)
    {

    }
    else
        perror("Socket: ");
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv.sin_port = htons(8080);

    if(connect(sock, (struct sockaddr *)&serv, sizeof(serv)) == 0)
    {

    }
    else
        perror("Connect");
    
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    events_ti Events_ti;

	GameState game_state;
    game_state.sock = sock;

	game_state.paddles[0].x = 0;
	game_state.paddles[0].y = 240 - 75;
	game_state.paddles[0].w = 30;
	game_state.paddles[0].h = 150;

	game_state.paddles[1].x = 640 - 30;
	game_state.paddles[1].y = 240 - 75;
	game_state.paddles[1].w = 30;
	game_state.paddles[1].h = 150;

	game_state.ball.x = 315;
	game_state.ball.y = 315;

	game_state.ball.w = 10;
	game_state.ball.h = 10;

	game_state.ball.vx = 0;
	game_state.ball.vy = 0;

	game_state.ball.state = 0; // Ball alive

	game_state.score[0] = 0;
	game_state.score[1] = 0;

	SDL_Init(SDL_INIT_VIDEO);


	Events_ti.window = SDL_CreateWindow("Game Window",
					SDL_WINDOWPOS_UNDEFINED,
					SDL_WINDOWPOS_UNDEFINED,
					640,
					480,
					0);

	renderer = SDL_CreateRenderer(Events_ti.window, -1,
					SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    pthread_t send;
    pthread_t receive;
    pthread_t events;

    Events_ti.game_state = &game_state;

    pthread_create(&send, NULL, send_data, (void*)&game_state);
    pthread_create(&receive, NULL, receive_data, (void*)&game_state);
    pthread_create(&events, NULL, processEvents, (void*)&Events_ti);

    while (1)
    {
        doRender(renderer, &game_state);
    }

    pthread_join(send, NULL);
    pthread_join(receive, NULL);
    pthread_join(events, NULL);

	return 0;
}
