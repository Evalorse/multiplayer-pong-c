#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

typedef struct GameState
{
    int score[2];
    Paddle paddles[2];
    Ball ball;
} GameState;

typedef struct thread_info
{
    int user_count;
    int sock[2];
    GameState *gs;
} thread_info;

void *receive_input(void *data)
{
    thread_info *ti = ((thread_info*)data);
    int s = ti->sock[ti->user_count];
    printf("ti sock user count in thread %d\n", ti->sock[ti->user_count]);
    GameState *gs = ti->gs;

    char in_buf[256];
    char out_buf[256];
    char command[2];

    int n;
    while((n = read(s, command, sizeof(command))) > 0)
    {
        printf("read bytes : %d\n", n);
        perror("read");
        printf("%s\n", command);

        if (command[0] == 'w')
            if (gs->paddles[0].y <= 0)
                gs->paddles[0].y = 0;
            else
                gs->paddles[0].y -= 1./2;
        if (command[0] == 's')
            if (gs->paddles[0].y >= 480 - gs->paddles[0].h)
                gs->paddles[0].y = 480 - gs->paddles[0].h;
            else
                gs->paddles[0].y += 1./2;
        if (command[1] == 'o')
            if (gs->paddles[1].y <= 0)
                gs->paddles[1].y = 0;
            else
                gs->paddles[1].y -= 1./2;
        if (command[1] == 'l')
            if (gs->paddles[1].y >= 480 - gs->paddles[1].h)
                gs->paddles[1].y = 480 - gs->paddles[1].h;
            else
                gs->paddles[1].y += 1./2;

        sprintf(out_buf, "%f;%f;%f;%f;%f;%f;%f;%f;%d;%d",
                                      gs->ball.x,
                                      gs->ball.y,
                                      gs->ball.vx,
                                      gs->ball.vy,
                                      gs->paddles[0].x,
                                      gs->paddles[0].y,
                                      gs->paddles[1].x,
                                      gs->paddles[1].y,
                                      gs->score[0],
                                      gs->score[1]);
        out_buf[strlen(out_buf)] = '\0';
        write(s, out_buf, sizeof(out_buf));
    }
    
    return NULL;
}

void *ball(void *ti)
{
    GameState *gs = ((thread_info*)ti)->gs;

	while (1)
    {
        if ((*(GameState*)gs).ball.x + (*(GameState*)gs).ball.w >= 640 - (*(GameState*)gs).paddles[1].w && (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h / 2 >= (*(GameState*)gs).paddles[1].y && (*(GameState*)gs).ball.y < (*(GameState*)gs).paddles[1].y + (*(GameState*)gs).paddles[1].h)
            (*(GameState*)gs).ball.vx *= -1;

        if ((*(GameState*)gs).ball.x <= (*(GameState*)gs).paddles[0].w && (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h / 2 >= (*(GameState*)gs).paddles[0].y && (*(GameState*)gs).ball.y < (*(GameState*)gs).paddles[0].y + (*(GameState*)gs).paddles[0].h)
            (*(GameState*)gs).ball.vx *= -1;

        if ((*(GameState*)gs).ball.y <= 0 || (*(GameState*)gs).ball.y + (*(GameState*)gs).ball.h >= 480)
            (*(GameState*)gs).ball.vy *= -1;

        (*(GameState*)gs).ball.x += ((*(GameState*)gs).ball.vx);
        (*(GameState*)gs).ball.y += ((*(GameState*)gs).ball.vy);

        if(gs->ball.x < 0) {
            gs->ball.x = 315;
            gs->ball.y = 235;

            gs->ball.vx = 0;
            gs->ball.vy = 0;

            gs->score[1]++;
        }
        if (gs->ball.x + gs->ball.w > 640) {
            //gs->ball.state = 0; // See what i can do with that to reset ball position
            gs->ball.x = 315;
            gs->ball.y = 235;

            gs->ball.vx = 0;
            gs->ball.vy = 0;

            gs->score[0]++;
        }
	}
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
    GameState game_state;

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
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock > 0)
    {

    }
    else
    {
        perror("Socket: ");
    }

    struct sockaddr_in serv, cli;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(8080);

    if(bind(sock, (struct sockaddr*)&serv, sizeof(serv)) > 0)
    {

    }
    else
    {
        perror("Bind: ");
    }

    unsigned int len = sizeof(cli);
    listen(sock, 10);

    pthread_t p[2];
    pthread_t ball_thread;

    thread_info ti;
    ti.gs = &game_state;
    ti.user_count = 0;
    ti.sock[0] = 0;
    ti.sock[1] = 0;
    char buffer[128];

    while (1)
    {
        ti.sock[ti.user_count] = accept(sock, (struct sockaddr*)&cli, &len);
        perror("Accept:");
        pthread_create(&p[ti.user_count], NULL, receive_input, (void*)&ti);
        if (ti.user_count == 1)
        {
            pthread_create(&ball_thread, NULL, ball, (void*)&ti);
        }
        if (ti.user_count < 2)
            ti.user_count++;
    }
    
    return 0;
}
