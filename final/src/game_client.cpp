#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "game_client.h"

#define SEM_MODE 0666

int P(int s)
{
   struct sembuf sop;
   sop.sem_num = 0;
   sop.sem_op = -1;
   sop.sem_flg = 0;
   if(semop(s,&sop,1) < 0)
        exit(printf("sem error\n"));
   else
        return 0;
}
int V(int s)
{
   struct sembuf sop;
   sop.sem_num = 0;
   sop.sem_op = 1;
   sop.sem_flg = 0;
   if(semop(s,&sop,1) < 0)
        exit(printf("sem error\n"));
   else
        return 0;
   
}

/* Constructer and Distructer */
GameClient::GameClient()
{
    this->stage_pointer = 2;
    this->keypad_input[0] = '\0';
    this->keypad_input[1] = '\0';
    this->secondhit = false;
    this->hide_and_play = false;//display section
    
    this->key = 66600;
    /*Create Semaphore for lcd display*/ 
    while(1)
    {
        if((this->lcd_semid = semget(this->key, 1, IPC_CREAT|SEM_MODE))<0){
            printf("Semaphore get error!\n");
            if(this->key == 66610) /*Try 10 semaphore key*/
                exit(printf("Semaphore Create failed\n"));
            else
                this->key++;
        }
        else
        {
            if(semctl(this->lcd_semid,0,SETVAL,1)<0)
                exit(printf("Can't init semaphore...\n"));
            break;
        }
        
    }
    printf("finish init game....\n");
}
GameClient::~GameClient()
{
    semctl(this->lcd_semid,0,IPC_RMID,0);
}

void GameClient::setup(int server_fd, int io_fd)
{
    this->server_fd = server_fd;
    this->io_fd = io_fd;
}



/* Interaction with server 
* readServer() will keep listening to server and is called by other thread in main function
* msghandler() will execute the server's command depand on current stage. 
*/
void GameClient::readServer()
{
    while(1)
    {
        this->rcv_size = read(this->server_fd,this->rcvmsg, sizeof(this->rcvmsg));
        server_pkt pkt;
        memcpy(&pkt,this->rcvmsg,sizeof(server_pkt));
        printf("Receive Message...");
        int zero[pattern_num]={0};
        this->hide_and_play = pkt.hide_p;
        if(this->stage_pointer == 2)
        {
            if (pkt.gameState == GAME_STATE_INIT)
            {
                this->draw_moles(zero);
            }
            else if(pkt.gameState == GAME_STATE_PLAYING)
            {
                if(pkt.newR == true){
                    this->secondhit = false;
                }//clear second hit for new round
                if(pkt.newS == true){
                    this->secondhit = false;
                }//clear second hit for new section
                int flag[pattern_num] = {0};
                // int flag_2 = 0;
                for (size_t i = 0; i < NUM_MOLES; i++)
                {
                    if(pkt.mole_states[i] == MOLE)
                        flag[0] |= 0x01<<i;
                    else if(pkt.mole_states[i] == HIT)
                        flag[1] |= 0x01<<i;
                }
                this->draw_moles(flag);
                printf("Score = %d\n",pkt.score);
            }
            else if(pkt.gameState == GAME_STATE_END)
            {
                this->keypad_input[1] = '#';
                this->secondhit = false;
                printf("game_state_end\n");
                return;
            }
            
        }
    }
}

void GameClient::sendServer()
{
    write(this->server_fd, this->sndmsg, strlen(this->sndmsg));
    write(this->server_fd, this->sndmsg2, strlen(this->sndmsg));
}



// Other function
void GameClient::draw()
{
    ioctl(this->io_fd, LCD_IOCTL_DRAW_FULL_IMAGE, &(this->graph));
    printf("DRAW()\n");
}

void GameClient::draw_graph(int graph_id, int x, int y)
{
    P(this->lcd_semid);
    int c,r;
    switch (graph_id)
    {
        case MOLE:
            //this->graph.data[81] = (unsigned short)0xff00;
            for(c = 0;c < column; c++){
                for(r = 0; r < row; r++){
                    this->graph.data[(y+r)*16 + x + c] = mole_map[r][c];
                }
            }       
            break;
        case DUST:
            for(c = 0;c < column; c++){
                for(r = 0; r < row; r++){
                    this->graph.data[(y+r)*16 + x + c] = dust_map[r][c];
                }
            }       
            break;
        case HIT:
            for(c = 0;c < column; c++){
                for(r = 0; r < row; r++){
                    this->graph.data[(y+r)*16 + x + c] = hit_map[r][c];
                }
            }       
            break;
        case BLOCK:
            for(c = 0;c < column; c++){
                for(r = 0; r < row; r++){
                    this->graph.data[(y+r)*16 + x + c] = block_map[r][c];
                }
            }       
            break;
        // case MENU:
        //     lcd_full_image_info graph;
        //     for(c = 0 ;c < 0x800;c++)
        //         graph.data[c] = menu_map[c];
        //     ioctl(this->io_fd, LCD_IOCTL_DRAW_FULL_IMAGE, &(graph));
        default:
            break;
    }
    V(this->lcd_semid);
}

void GameClient::draw_moles(int state[])
{
    for(int i=0;i<NUM_MOLES;i++)
    {
        if((state[0]>>i)&(0x01))
        {
            this->draw_graph(MOLE,mole_x[i] ,mole_y[i]);
            state[1]>>i;
        }
        else if((state[1]>>i)&(0x01))
        {
            this->draw_graph(HIT,mole_x[i] ,mole_y[i]);
            state[0]>>i;
        }
        else
        {
            this->draw_graph(DUST,mole_x[i] ,mole_y[i]);       
        }
    }
    this->draw();
}

int GameClient::read_pad()
{
    unsigned short key;
    if( ioctl(this->io_fd, KEY_IOCTL_CHECK_EMTPY, &key) < 0){
        usleep(100000); //Sleep 0.1s
        return 0;
    }
	ioctl(this->io_fd, KEY_IOCTL_GET_CHAR, &key);
    //change to char and print out
    this->keypad_input[0] = key & 0xff;
    this->keypad_input[1] = '\0';
    return 1;
}

void GameClient::run()
{
    while(1)
    {
        if(this->stage_pointer == 2)
        {
            printf("Stage 2\n");
            //this->draw_moles(0x00);
            int i;
            char key0;
            while(1)
            {
                if(this->read_pad())
                {
                    key0 = this->keypad_input[0];
                    if( ( (key0 > '0') && (key0 <= '9') ) || (key0 == '*') || (key0 == '#') || (key0 == '0'))
                    {   
                        if(this->secondhit == false){//first hit
                            if((key0 > '0') && (key0 <= '9'))
                                i = key0-'1';
                            else if(key0 == '*')
                                i = 9;
                            else if(key0 == '0')
                                i = 10;
                            else if(key0 == '#')
                                i = 11;
                            if(this->hide_and_play == true)
                                this->draw_graph(BLOCK,mole_x[i] ,mole_y[i]);
                            this->draw();
                            sprintf(this->sndmsg,"%c",key0);
                            this->secondhit = true;
                        }else{
                            if((key0 > '0') && (key0 <= '9'))
                                i = key0-'1';
                            else if(key0 == '*')
                                i = 9;
                            else if(key0 == '0')
                                i = 10;
                            else if(key0 == '#')
                                i = 11;
                            if(this->hide_and_play == true)
                                this->draw_graph(BLOCK,mole_x[i] ,mole_y[i]);
                            this->draw();
                            sprintf(this->sndmsg2,"%c",key0);
                            this->secondhit = false;
                            this->sendServer();
                        }
                    }
                }
                if(this->keypad_input[1] == '#') // When time's up, change keypad_input[1] to '#'
                    return;
            }
            this->stage_pointer = 2;
        }
    }
}
