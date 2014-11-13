/*
 Most the objects of this program (world View, player, computerAI, messages) 
    Are managed by their own re-entrant HANDLER function
        The methods of the handler are controlled by COMMANDS which are identified through enum'd constants.
        The properties of the object reside in an Object STRUCT.
            This allows for the objects to potentially have multiple instances which each hold unique structs and therefore unique properties.
        The dependent objects of the object might also have structs of their own embedded inside the parent Object Struct, 
            like the ship properties of ships that belong to a player will have data Structs of their own, inside the player struct.
            (actually I handled the ships a little different, and their commands are embedded into their name, because the function arguments needed more flexibility)
    
*/



#include <conio.h>
#include <cstring>
#include <curses.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>


using namespace std;

enum TILE        { NOTHING=0,CARRIER,     BATTLESHIP,     CRUISER,     SUBMARINE,     DESTROYER,     WATER,
                            CARRIER_HIT, BATTLESHIP_HIT, CRUISER_HIT, SUBMARINE_HIT, DESTROYER_HIT, WATER_MISS,
                            WATER_HIT, TRY_AGAIN };				
enum STATE       { IDLE=0, WAITING, STARTING, CLOSING, QUITTING, EXPLODING, HIT, MISS,DESTROYED,NORTH,SOUTH,EAST,WEST};
enum COMMAND     { INIT=0, SERVICE, RESET, END, RESTART, FIRE, QUIT, POSITION, SHOW , MAINLOOP, TERMINATE, ADD, CHECK_TARGET};
enum ORIENTATION { HORIZONTAL=0, VERTICAL };
enum DIFF        { EASY=0, MEDIUM, HARD, STUPID };
enum COLORS      {BLACK=0,RED,GREEN,YELLOW,BLUE,CYAN,MAGENTA,WHITE};
enum SEEKMODE    {SEARCH=0,DESTROY};

typedef struct {
    STATE       state;
    TILE        tileType;   
    ORIENTATION orientation;  
    int         length;					//size of the ship 2,3,4,5 tiles.
    TILE        tiles[6];
    int         xpos;
    int         ypos;
    int         hits;
} SHIP;

typedef struct {
    STATE       state;
    TILE        targetType;
    int         targetVectorX;
    int         targetVectorY;	
    bool        directionKnown; 
    int         hits;
    bool        changeDirection;
} TARGET;


typedef struct {
    STATE       state; 
    SHIP        ships[6];					//not a pointer to a SHIP struct but the allocated space
    TARGET      targets[6];
    TILE        playerGameSpace[10][10]; 
    TILE        enemyGameSpace[10][10];  	//history of what the player knows about enemy	
    char        attackVectorX;				//currently aimed at attack vector X
    char        attackVectorY;	
    int         lost;
} PLAYER;

typedef struct {
    STATE       state; 
    DIFF        difficulty;
    int         round;		
    char        key;	
} WORLD;


/* PROTOTYPES */	

bool            worldHandler(COMMAND cmd, WORLD *world);
int             playerHandler(COMMAND cmd, PLAYER *player);
void            cursesHandler(COMMAND cmd);
int             messageHandler(COMMAND cmd, string message);
int             computerAIHandler(COMMAND cmd, PLAYER *computer);

void            ship_INIT(SHIP * shipObj, TILE tileType);
void            ship_SET_RANDOM_POSITION(SHIP * shipObj);
int             ship_CHECK_SHIP_POSITION(SHIP * shipObj,TILE gameSpace[][10]);
void            ship_PLACE_ON_MAP(SHIP * shipObj,TILE gameSpace[][10]);
int             ship_TAKE_HIT(SHIP * shipObj);

bool            target_CHANGE_DIRECTION(TARGET * target);
TILE            target_CHECK_TARGET(TARGET * target, PLAYER * player, TILE targetTile);

bool            getInput(char *c);

string          &str_replace(const string &search, const string &replace, string &subject);
vector<string>  split(const string &s, char delim);
vector<string>  &split(const string &s, char delim, vector<string> &elems);

/* DATA TABLES */
const string viewTemplate = 
        "  0 1 2 3 4 5 6 7 8 9  [TAB][ENTER][LEFT][RIGHT][UP][DOWN]  \n"
        "A A0A1A2A3A4A5A6A7A8A9 [-------------  STAT  -------------] \n"
        "B B0B1B2B3B4B5B6B7B8B9  X = {0-9} : [x]   DIFFICULTY: {diff}\n"
        "C C0C1C2C3C4C5C6C7C8C9  Y = {A-J} : [y]                     \n"
        "D D0D1D2D3D4D5D6D7D8D9  ROUND     : rrr                     \n"
        "E E0E1E2E3E4E5E6E7E8E9 \n"
        "F F0F1F2F3F4F5F6F7F8F9 [------------ MESSAGES ------------] \n"
        "G G0G1G2G3G4G5G6G7G8G9 [{row 0}]\n"
        "H H0H1H2H3H4H5H6H7H8H9 [{row 1}]\n"
        "I I0I1I2I3I4I5I6I7I8I9 [{row 2}]\n"
        "J J0J1J2J3J4J5J6J7J8J9 [{row 3}]\n"
        "                       [{row 4}]\n"
        "  0 1 2 3 4 5 6 7 8 9  [{row 5}]\n"
        "A a0a1a2a3a4a5a6a7a8a9 [{row 6}]\n"
        "B b0b1b2b3b4b5b6b7b8b9 [{row 7}]\n"
        "C c0c1c2c3c4c5c6c7c8c9 [{row 8}]\n"
        "D d0d1d2d3d4d5d6d7d8d9 [{row 9}]\n"
        "E e0e1e2e3e4e5e6e7e8e9 [{row 10}]\n"
        "F f0f1f2f3f4f5f6f7f8f9 [{row 11}]\n"
        "G g0g1g2g3g4g5g6g7g8g9 [{row 12}]\n"
        "H h0h1h2h3h4h5h6h7h8h9 [{row 13}]\n"
        "I i0i1i2i3i4i5i6i7i8i9 [{row 14}]\n"
        "J j0j1j2j3j4j5j6j7j8j9 [{row 15}]\n"
        "[---------------------------------------------------------] \n"
        " [Carrier] [Battleship] [K-Cruiser] [Submarine] [Destroyer] \n"
        " \n"
        " \n"
        " \n"
        " \n";
		
int             partLengths[]			={1,5,4,4,3,2,1,		
											4,4,3,2,1,1,0};
							//REF http://www.theasciicode.com.ar/extended-ascii-code/graphic-character-low-density-dotted-ascii-code-176.html
string          tileSet[] 				= {"  ", "C ", "B ", "K ", "S ", "D ", ". ",
												 "CX", "BX", "KX", "SX", "DX", ".O", ".X"};
string          tileTypes[]  			= {"NOTHING","CARRIER",    "BATTLESHIP",    "CRUISER",    "SUBMARINE",    "DESTROYER",    "WATER",
           							     "CARRIER_HIT","BATTLESHIP_HIT","CRUISER_HIT","SUBMARINE_HIT","DESTROYER_HIT","WATER_MISS","WATER_HIT"};

										//see enum STATE
string          stateTypes[]			={"IDLE","WAITING","STARTING","CLOSING","QUITTING","EXPLODING","HIT","MISS","DESTROYED","NORTH","SOUTH","EAST","WEST"};

										 
const string    difficulties[]			={"EASY  ","MEDIUM","HARD  ","STUPID"};
const char      enemyRows[]				={'A','B','C','D','E','F','G','H','I','J'};
const char      enemyCols[]				={'0','1','2','3','4','5','6','7','8','9'};								
const char      playerRows[]			={'a','b','c','d','e','f','g','h','i','j'};
const char      playerCols[]			={'0','1','2','3','4','5','6','7','8','9'};
			
							 
char            attackXCoordinates[]	={'0','1','2','3','4','5','6','7','8','9'};
char            attackYCoordinates[]	={'a','b','c','d','e','f','g','h','i','j'};
							 
PLAYER          playerObj;
PLAYER          computerObj;
WORLD           worldObj;
string          viewBuffer;
char            tmpBuffer[256];			//temp buffer for constructing display elements

	
	// *******************************************************		
	int main (){
	// *******************************************************	
		cursesHandler(INIT);

		worldHandler(INIT, &worldObj);
		
		worldHandler(MAINLOOP, &worldObj);		
		
		printf("\n program aborted");
				
		worldHandler(TERMINATE, &worldObj);
	}
	
	// *******************************************************
	bool worldHandler(COMMAND cmd, WORLD *world){
	// *******************************************************
		//enum COMMAND     { INIT, RESET, END, RESTART, FIRE, QUIT };
		int count;
		PLAYER * player  = &playerObj;
		PLAYER * computer= &computerObj;
				
		switch (cmd){
			default:
			break;
			
			case INIT:	//constructor
				world->key=' ';
				world->state=IDLE;
				world->difficulty=MEDIUM;
				world->round=1;
				
				playerHandler(INIT, player);                //set up the player world				
                computerAIHandler(INIT , computer);         //set up the world view of the computer AI
				messageHandler(INIT, " "); 
                
						
			break;
			
			case MAINLOOP:
				count=0;
				while(world->key != 'q'){ //loop forever
					while(!getInput(&world->key)){						
						worldHandler(SHOW, world);						
					}
					count =0;
					worldHandler(SERVICE, world);
				}
			break;
			
			case SERVICE:  // services all events for the main world 
								
				playerHandler(SERVICE, player);
                
                computerAIHandler(SERVICE, computer);
                
                            if(computer->lost > 4){																
								//                  [------------ MESSAGES ------------] 
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"           PLAYER WINS!!!  ");
								messageHandler(ADD,"            BATTLE SHIPS ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");								
					
								messageHandler(ADD,"        [TAB] to restart ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"            Well Done! ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								
							}
                            
							if(player->lost > 4){
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"           COMPUTER WINS!!!  ");
								messageHandler(ADD,"             BATTLE SHIPS ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");								
					
								messageHandler(ADD,"        [TAB] to restart ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"            Try Again ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
								messageHandler(ADD,"     ");
							}
					

			break;
			case TERMINATE:
				cursesHandler(TERMINATE);
				playerHandler(TERMINATE, player); 
				playerHandler(TERMINATE, player); 
			break;
			case SHOW:       //the world
				int         targetY;
				int         targetX;
				chtype      c;
				
				
												//Initialize the viewBuffer
				viewBuffer = viewTemplate;      // I know , I know .. globals .. normally this would all be encapsulated in classes
												// but this lets the SHOW COMMANDs function within the handlers.
				
				
				
				playerHandler(SHOW, player);

				/* SHOW THE STATUS MESSAGES */
				messageHandler(SHOW,"");
				
				
				/* OUTPUT THE viewBuffer */
				//We have to chop up each line or curses buffer will overflow and crash
				vector<string> outputArray=split(viewBuffer, (char) '\n'); //split(const string &s, char delim)				
				for(int row=0;row<25;row++){
					move(row,0);					
					printw("%s",outputArray[row].c_str());					
				}
				
				refresh();	
				/* 
				  Show the cross hairs 
				  Take the users current attackVector and display a crosshair over the enemy map
				*/
				
				attron(A_REVERSE);  //---   Turn on the specified attribute for subsequently output characters. The parameter can be one of several pre-defined constants, including: A_BOLD (bold characters), A_UNDERLINE (underlined characters), and A_REVERSE (reverse video, that is, foreground and background colors reversed).	
					
				for(int row=0;row<11;row++){
					targetY = (int) row; //(player->attackVectorY & 0x7f) - 0x61;
					targetX = (int) (player->attackVectorX & 0x7f) - 0x30;
					move(targetY,(targetX*2)+2);
					c = inch( );
					move(targetY,(targetX*2)+2);
					addch(c);		
					
					//printw("%s",outputArray[row].c_str());					
				}
				for(int col=0;col<22;col++){
					targetY = (int) (player->attackVectorY & 0x7f) - 0x61;
					targetX = (int) col;
					move(targetY+1,targetX);
					c = inch( );
					move(targetY+1,targetX);
					addch(c);
					
					//printw("%s",outputArray[row].c_str());					
				}
			
				attroff(A_REVERSE); // ---   Turn off the specified attribute for subsequently output characters. The parameter is one of the same constants as in the attron() function.
				refresh();				
				
			break;
			
		}
		return 0;
	}

    // *******************************************************
	int computerAIHandler(COMMAND cmd, PLAYER *computer){
	// *******************************************************
        WORLD   *   world		= &worldObj;								
		PLAYER  *   player 	    = &playerObj;		
		TARGET  *   target;
        TILE        result;
		int         numTargets;				
		int         targetType;
        
        int targetY = (int) (player->attackVectorY & 0x7f) - 0x61;
		int targetX = (int) (player->attackVectorX & 0x7f) - 0x30;		
		
        
        switch(cmd){
            default:
            break;
            case INIT:
                                                            //The computer AI is also a player, so it has all the same structures and views as a human player.
                playerHandler(INIT, computer);              //set up the Computer AI Player world
                                                    
            break;
            case SERVICE:
                            //only called when a keystroke has happened, the human player gets first dibs at the keystrokes
                            //and the computer then responds to the players actions.
                            
                            //nothing to do...
                            //except maybe talk smack once every (100/4) keystrokes.
                          
                          
                          if(rand() % 200 == 1){
                                messageHandler(ADD,"Comp,'OMG you are so bad at this'");
                            }
                            if(rand() % 200 == 2){
                                messageHandler(ADD,"Comp,'really, find another game'");
                            }
                            if(rand() % 200 == 3){
                                messageHandler(ADD,"Comp,'Im posting this on facebook'");
                            }
                            if(rand() % 200 == 4){
                                messageHandler(ADD,"Comp,'Are you even trying?'");
                            }                          
                          
            break;
            case FIRE:      //Handles the AI's decision making when taking a shot.    
                            //see description.txt for the logic behind what it does.
                            bool  debug = false;
                            int trys;
                            int steps;
                    		numTargets=0;
							result=TRY_AGAIN;
							trys=0;
                            
							//FIRST CHECK TO SEE IF THE COMPUTER HAS ANY TARGETS
							for(targetType = (int) CARRIER; targetType < (int) WATER; targetType++){							
								steps=0;
                                
								target = &computer->targets[(int)targetType];
								target->changeDirection=false;								
								
								if(target->state!=IDLE){                                //NORTH,SOUTH,EAST,WEST,HIT is our targetting direction
									numTargets = (int) targetType;						//records a flag of the targetType we are targetting
									//we are hunting this ship
                                    
									while(result==TRY_AGAIN ){
										sprintf(tmpBuffer,"TARGET %s %s",tileTypes[(int)targetType].c_str(),stateTypes[(int)target->state].c_str());
										messageHandler(ADD,tmpBuffer);
									
										target->changeDirection=false;
										steps=0;												
										switch(target->state){
											case NORTH:												
												while(result==TRY_AGAIN && target->changeDirection==false ){
												
													computer->attackVectorX = attackXCoordinates[target->targetVectorX];
													if(target->targetVectorY-steps > -1 && steps<partLengths[(int)targetType]){
														computer->attackVectorY = attackYCoordinates[target->targetVectorY-steps];														
														
														result =(TILE) target_CHECK_TARGET(target, computer, (TILE) targetType);
                                                        
                                                        if(result == TRY_AGAIN && debug){
                                                            sprintf(tmpBuffer,"TRY AGAIN [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}else{													
														//because out of bounds we need to redirect the search direction
														target->changeDirection=target_CHANGE_DIRECTION(target);														
                                                        if(debug){
                                                            sprintf(tmpBuffer,"out of bounds [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}
													steps++;
                                                    if(trys++ > 99){
                                                        break;
                                                    }
												}
											break;
											case EAST:												
												while(result==TRY_AGAIN && target->changeDirection==false ){
											
													computer->attackVectorY = attackYCoordinates[target->targetVectorY];
													if(target->targetVectorX-steps >-1 && steps<partLengths[(int)targetType]){
														computer->attackVectorX = attackXCoordinates[target->targetVectorX-steps];														
														
														result =(TILE) target_CHECK_TARGET(target, computer, (TILE) targetType);													    
                                                        if(result == TRY_AGAIN && debug){
                                                            sprintf(tmpBuffer,"TRY AGAIN [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}else{
														//because out of bounds we need to redirect the search direction
														target->changeDirection=target_CHANGE_DIRECTION(target);
                                                        if(debug){
                                                            sprintf(tmpBuffer,"out of bounds [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}
													steps++;
                                                    if(trys++ > 99){
                                                        break;
                                                    }
												}										
											break;
											case SOUTH:																							
												while(result==TRY_AGAIN && target->changeDirection==false ){
												
													computer->attackVectorX = attackXCoordinates[target->targetVectorX];
													if(target->targetVectorY+steps <10 && steps<partLengths[(int)targetType]){
														computer->attackVectorY = attackYCoordinates[target->targetVectorY+steps];														
														
														result =(TILE) target_CHECK_TARGET(target, computer, (TILE) targetType);
                                                        if(result == TRY_AGAIN && debug){
                                                            sprintf(tmpBuffer,"TRY AGAIN [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													    
													}else{
														//because out of bounds we need to redirect the search direction
														target->changeDirection=target_CHANGE_DIRECTION(target);
                                                        if(debug){
                                                            sprintf(tmpBuffer,"out of bounds [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}
													steps++;
                                                    if(trys++ > 99){
                                                        break;
                                                    }
												}
											break;
											case WEST:												
												while(result==TRY_AGAIN && target->changeDirection==false ){
												
													computer->attackVectorY = attackYCoordinates[target->targetVectorY];
													if(target->targetVectorX+steps <10 && steps<partLengths[(int)targetType]){
														computer->attackVectorX = attackXCoordinates[target->targetVectorX+steps];
														
													    result =(TILE) target_CHECK_TARGET(target, computer, (TILE) targetType);
                                                        if(result == TRY_AGAIN && debug){
                                                            sprintf(tmpBuffer,"TRY AGAIN [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}else{
														//because out of bounds we need to redirect the search direction
														target->changeDirection=target_CHANGE_DIRECTION(target);
                                                        if(debug){
                                                            sprintf(tmpBuffer,"out of bounds [%c, %c][%d %d]  %d",computer->attackVectorX,computer->attackVectorY,target->targetVectorX,target->targetVectorY,steps);
                                                            messageHandler(ADD,tmpBuffer);
                                                        }
													}
													steps++;
                                                    if(trys++ > 99){
                                                        break;
                                                    }
												}
											break;								
										}
                                        if(trys++ > 99){
                                            break;
                                        }
										if(target->changeDirection){
										//we have a direction change request .. soo then what?
										// if result also == try_again ...
										}
									}//while TRY AGAIN									
								}//if target idle
								if(result!=TRY_AGAIN || trys > 99){
                                    if (trys > 99){
                                        messageHandler(ADD,"ERROR Failed 100 trys to target");
                                        return 1;
                                    }
                                    break;
								}								
							}
							
                                                 
							if(numTargets==0){ // there are no targets so go into search mode.
								result	=TRY_AGAIN;
								targetType=(int) NOTHING;
                                trys=0;
								while(result==TRY_AGAIN ){
									computer->attackVectorX = attackXCoordinates[rand() % 10];
									computer->attackVectorY = attackYCoordinates[rand() % 10];														
									
									result =(TILE) target_CHECK_TARGET(target, computer, (TILE) targetType);
                                    if (trys++ > 99){
                                        messageHandler(ADD,"ERROR Failed 100 trys to search");
                                        return 1;
                                    }
                                }
                                
							}
							sprintf(tmpBuffer,"Computer Fires at [%c, %c]",computer->attackVectorX,computer->attackVectorY);
							messageHandler(ADD,tmpBuffer);
							
							if((int)result>0 and (int)result<6){
								sprintf(tmpBuffer,"Computer hits %s",tileTypes[(int)result].c_str());
								messageHandler(ADD,tmpBuffer);								
								
								//check for first time hit
								target = &computer->targets[(int)result];
								if(target->state == IDLE || target->hits==0){
									//first time hit
									STATE directions[]={NORTH,SOUTH,EAST,WEST};									
								
									target->state = (STATE) directions[rand() % 4];
									target->hits=1;
									target->directionKnown=false;
									target->targetVectorX= (int) (computer->attackVectorX & 0x7f) - 0x30;		
									target->targetVectorY= (int) (computer->attackVectorY & 0x7f) - 0x61;																		
								}else{	
										target->hits++;
										if((int)result==(int) targetType){
											target->directionKnown=true;
										}
										/* potential problem here...
											if the the ship is targetting a carrier say
											   and then it hits a sub instead.. then this is technically a miss
											   the target algorithm needs to change direction
												but it also needs to record the hit on the sub and leave that in a state to be targetted too
											so we only record direction as known if we have hit a target that we were actually looking for.
											otherwise for accidental hits, direction is unknown.											
										*/		
								}
								
								if(ship_TAKE_HIT(&player->ships[(TILE)result])==0){
									sprintf(tmpBuffer,"COMPUTER DESTROYS %s",tileTypes[(int)result].c_str());
									messageHandler(ADD,tmpBuffer);
									
									player->lost++;				
																		
									target->hits++;
									target->state=IDLE;  // set this target back to idle it is done.
									
								}
							}
							
							if(result==WATER){
								sprintf(tmpBuffer,"Computer Misses");
								messageHandler(ADD,tmpBuffer);
							}
            break;
        
        }
		
        return 0;

    }
    
	// *******************************************************
	int playerHandler(COMMAND cmd, PLAYER *player){
	// *******************************************************
		//enum COMMAND     { INIT, RESET, END, RESTART, FIRE, QUIT };
		WORLD * world		= &worldObj;						
		PLAYER * computer	= &computerObj;		
		TARGET * target;

		int targetY = (int) (player->attackVectorY & 0x7f) - 0x61;
		int targetX = (int) (player->attackVectorX & 0x7f) - 0x30;		
		int row,col,trys;
		TILE result;
		int numTargets;				
				
		switch (cmd){
			default:
			break;
			
			case SERVICE:			
				
				
				switch(world->key){
					case (char) KEY_LEFT:
						if(targetX>0){
							player->attackVectorX-=1;
						}
					break;
					case (char) KEY_RIGHT:
						if(targetX<9){
							player->attackVectorX+=1;
						}
					break;
					case (char) KEY_UP:
						if(targetY>0){
							player->attackVectorY-=1;
						}
					break;
					case (char) KEY_DOWN:
						if(targetY<9){
							player->attackVectorY+=1;
						}
					break;
					case (char) 0x0d:
							//TAKE A SHOT AT THE ENEMY
							if(playerHandler(FIRE, player)){  // if return 1 then there was a failed attempt at a shot and we need to try again
                                return 1;
                            }
                            
							//Successful attack so TAKE A HIT FROM THE ENEMY
                            computerAIHandler(FIRE, computer);
                    		
							world->round++;							
							
					break;
					
					case 'a':
					case 'b':
					case 'c':
					case 'd':
					case 'e':
					case 'f':
					case 'g':
					case 'h':
					case 'i':
					case 'j':
						player->attackVectorY=world->key;
					break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						player->attackVectorX=world->key;
					break;
	//				case 0x0d:		//ENTER KEY
						
	//				break;
	//				case 0x20:		//SPACE BAR
						
	//				break;
					case 0x09:		//TAB KEY
						worldHandler(INIT, world);
					default:
					break;					
				}				
			
			break;
			case FIRE:
                            int targetType;
							
                            sprintf(tmpBuffer,"Player Shoots at [%c, %c]",player->attackVectorX,player->attackVectorY);
							messageHandler(ADD,tmpBuffer);
							
							//COLLISION CHECK AND REPORT
							//result=(TILE) playerHandler(CHECK_TARGET,player);
							result =(TILE) target_CHECK_TARGET(target, player, NOTHING);
							
							if(result==TRY_AGAIN){
								sprintf(tmpBuffer,"Shoot Again, already targetted");
								messageHandler(ADD,tmpBuffer);
								return 1;
							}
							if((int)result>0 and (int)result<6){
								sprintf(tmpBuffer,"Player hits %s",tileTypes[(int)result].c_str());
								messageHandler(ADD,tmpBuffer);
								if(ship_TAKE_HIT(&computer->ships[(TILE)result])==0){
									sprintf(tmpBuffer,"PLAYER DESTROYS %s",tileTypes[(int)result].c_str());
									messageHandler(ADD,tmpBuffer);
									computer->lost++;									
								}
							}
							
							if(result==WATER){
								sprintf(tmpBuffer,"Player Misses");
								messageHandler(ADD,tmpBuffer);
							}
					
            break;
            
			case INIT:	//constructor				
				
				player->state=IDLE;
				player->attackVectorX='0';			
				player->attackVectorY='a';
				player->lost=0;				
				
				for(row=0;row<10;row++){
					for(col=0;col<10;col++){
						player->playerGameSpace[col][row]=WATER;
						player->enemyGameSpace[col][row]=WATER;						
					}
				}
			
			
			/*    { NOTHING=0,CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER, WATER,*/
				//create and place ships for the player at random locations
				for(int tileType = (int) CARRIER;tileType < (int) WATER;tileType++){
					ship_INIT(&player->ships[tileType], (TILE) tileType);
					
					target = &player->targets[(int) tileType];
					target->state=IDLE;
					target->hits=0;
					target->directionKnown=false;
					
					ship_SET_RANDOM_POSITION(&player->ships[tileType]);
					trys=0;
					while( ship_CHECK_SHIP_POSITION(&player->ships[tileType],&player->playerGameSpace[0])==-1 && trys++ < 100){
						ship_SET_RANDOM_POSITION(&player->ships[tileType]);
					}
					
					if(trys>100){
						printf("ERROR: over 100 tries to place a ship on player map");
						exit(1);
					}else{
						ship_PLACE_ON_MAP(&player->ships[tileType],&player->playerGameSpace[0]);
					}					
				}
			break;
			case POSITION:
				
			
			break;
			case SHOW:				
				WORLD *world = &worldObj;								
												
				
				char 			buffer[20];				//temp buffers for constructing display elements
				char 			vector[10];								
				
				
				//REPLACE TEMPLATE CODES WITH VALUES 				
				viewBuffer = str_replace("{diff}",difficulties[world->difficulty], viewBuffer);
				viewBuffer = str_replace("[x]", string(1,player->attackVectorX),viewBuffer);
				viewBuffer = str_replace("[y]", string(1,player->attackVectorY),viewBuffer);
				sprintf(buffer,"%03d",world->round);
				viewBuffer = str_replace("rrr",buffer,viewBuffer);
				//viewBuffer = str_replace("{player message}",world->status,viewBuffer);  // removed it

				//Note replacement Vectors for the PLAYER_GAMESPACE and ENEMY_GAMESPACE are easily locatable as
				// (UpperCase Letter, Numeric)
				// (lowerCase Letter, Numeric)
				//So to update a Vector we can just use something like
		
				for(row =0; row<10; row++){
					for(col = 0; col<10; col++){
						vector[0]=playerRows[row];
						vector[1]=playerCols[col];
						vector[2]=0;
						viewBuffer = str_replace(vector, tileSet[player->playerGameSpace[col][row]], viewBuffer);
					}
				}
						
				for(row =0; row<10; row++){
					for(col = 0; col<10; col++){
						vector[0]=enemyRows[row];
						vector[1]=enemyCols[col];
						vector[2]=0;
						viewBuffer = str_replace(vector, tileSet[player->enemyGameSpace[col][row]], viewBuffer);
					}
				}

			
			break;		
		}
		return 0;
	}

	// *******************************************************
	TILE target_CHECK_TARGET(TARGET * target, PLAYER * player, TILE targetTile){
	// checks attack vector to see if we hit target, and also updates targetting AI direction on miss of targetTile type
	// *******************************************************
		
		int targetY = (int) (player->attackVectorY & 0x7f) - 0x61;
		int targetX = (int) (player->attackVectorX & 0x7f) - 0x30;		
		
		TILE testTile;
		TILE historyTile;
		PLAYER * enemyPlayer;
			
		if(player == &computerObj){
			enemyPlayer = &playerObj;					
		}else{
			enemyPlayer = &computerObj;
		}
				
		
		historyTile	=player->enemyGameSpace[targetX][targetY];				
				
				//enum TILE  		 { NOTHING=0,CARRIER,     BATTLESHIP,     CRUISER,     SUBMARINE,     DESTROYER,     WATER,
				//             CARRIER_HIT, BATTLESHIP_HIT, CRUISER_HIT, SUBMARINE_HIT, DESTROYER_HIT, WATER_MISS,
				//             WATER_HIT , TRY_AGAIN
							 
		switch(historyTile){  			//check to make sure we have not hit this square already
			case CARRIER_HIT:
			case BATTLESHIP_HIT:
			case CRUISER_HIT:
			case SUBMARINE_HIT:
			case DESTROYER_HIT:		
				//if the target does not match, we need to change search directions
				if((bool)targetTile && (int) targetTile+6 != (int) historyTile){
					target_CHANGE_DIRECTION(target);
				}						
				return TRY_AGAIN;	// 2= Already Targetted
			break;
			case WATER_MISS:  		
				if((bool)targetTile){
					target_CHANGE_DIRECTION(target);
				}
				return  TRY_AGAIN;	// 2= Already Targetted
			break;
		}
		
		//check to see if we have hit something in the enemy's game space 
		testTile	=enemyPlayer->playerGameSpace[targetX][targetY];
				
		switch(testTile){  
			case CARRIER:
			case BATTLESHIP:
			case CRUISER:
			case SUBMARINE:
			case DESTROYER:		
				if((bool)targetTile && (int) testTile != (int) targetTile){  //we have missed our target, we should change direction
					target_CHANGE_DIRECTION(target);
				}						
				enemyPlayer->playerGameSpace[targetX][targetY]=(TILE) (testTile+6);  //Show the Attack on enemys board
				
				player->enemyGameSpace[targetX][targetY]=(TILE) (testTile+6);		//record attack in your history		
				return (TILE) testTile;	 // 1 .. 5 = Hit A TARGET
			break;
			case WATER:
				if((bool)targetTile && (int) testTile != (int) targetTile){  //we have missed our target, we should change direction
					target_CHANGE_DIRECTION(target);
				}						
				enemyPlayer->playerGameSpace[targetX][targetY]=(TILE) WATER_MISS;	//show attack on enemys board
				
				player->enemyGameSpace[targetX][targetY]=(TILE) WATER_MISS;			//record attack in your history				
				return (TILE) WATER;
			break;
		}
					
		return (TILE) NOTHING;
	}
	
	// *******************************************************
	bool target_CHANGE_DIRECTION(TARGET * target){               //manages seek direction change for the targeting AI
	// *******************************************************
		switch(target->state){
			case NORTH:
				if(target->directionKnown){
					target->state=SOUTH;
					target->changeDirection=true;
					return true;
				}else{
					target->state=EAST;
					target->changeDirection=true;
					return true;
				}
			break;
			case SOUTH:
				if(target->directionKnown){
					target->state=NORTH;
					target->changeDirection=true;
					return true;
				}else{
					target->state=WEST;
					target->changeDirection=true;
					return true;
				}
			break;
			case EAST:
				if(target->directionKnown){
					target->state=WEST;
					target->changeDirection=true;
					return true;					
				}else{
					target->state=SOUTH;
					target->changeDirection=true;
					return true;																	
				}
				break;
			case WEST:
				if(target->directionKnown){
					target->state=EAST;
					target->changeDirection=true;
					return true;																	
				}else{
					target->state=NORTH;
					target->changeDirection=true;
					return true;																	
				}
			break;
			}
		target->changeDirection=false;
		return false;
	}

	// *******************************************************
	int ship_TAKE_HIT(SHIP * shipObj){
	// Returns 0 if ship is destroyed
	// *******************************************************			
		shipObj->hits++;
		if((int)partLengths[shipObj->tileType] == shipObj->hits){
			shipObj->state=DESTROYED;
			return 0;
		}
		return (int) (partLengths[shipObj->tileType] - shipObj->hits);
	}
	
	// *******************************************************
	void ship_INIT(SHIP * shipObj, TILE tileType=NOTHING){
	// *******************************************************	
		int i;
		shipObj->state		= IDLE;			
		shipObj->tileType	= tileType;						
		shipObj->length		= partLengths[tileType];
		shipObj->hits		= 0;

		for(i=0;i<5;i++){						//clear the tiles out		
			shipObj->tiles[i]= WATER;
		}
			
		for(i=0;i<partLengths[tileType];i++){	//set the non water tile types
			shipObj->tiles[i]= tileType;
		}
		
	}
	
	// *******************************************************
	void ship_SET_RANDOM_POSITION(SHIP * shipObj){
	// *******************************************************			
		int v1 = rand() % (10 - shipObj->length - 1);
		int v2 = rand() % 10;			
		shipObj->orientation= (ORIENTATION) (rand() % 2);			//set a random orientation
			
		switch(shipObj->orientation){
			case VERTICAL: //vertical placement anchored at top
				shipObj->xpos=v2;
				shipObj->ypos=v1;
			break;			
			case HORIZONTAL: //horizontal placement anchored to the LEFT
				shipObj->xpos=v1;
				shipObj->ypos=v2;
			break;
		}
	}
	
	
	// *******************************************************
	int ship_CHECK_SHIP_POSITION(SHIP * shipObj,TILE gameSpace[][10]){
	// *******************************************************			
		int xpos;
		int ypos;
		int length = shipObj->length;
		
		// step through each tile position in the game space for WATER
		// return true if there is free water and we are within bounds
		// return false on error
		
		switch(shipObj->orientation){			
			case VERTICAL: //vertical placement anchored at top
				for(int i=0;i<length;i++){
					xpos= shipObj->xpos;
					ypos=(shipObj->ypos) + i;					
					if(gameSpace[xpos][ypos]!=WATER || ypos>9){
						return -1;
					}
				}
				return 1;
			break;
			case HORIZONTAL: //horizontal placement anchored to the left
				for(int i=0;i<length;i++){
					xpos=(shipObj->xpos) + i;
					ypos= shipObj->ypos;
					if(gameSpace[xpos][ypos]!=WATER || xpos>9){
						return -1;
					}
				}
				return 1;
			break;
		}
		return -1;
	}
	
	// *******************************************************
	void ship_PLACE_ON_MAP(SHIP * shipObj,TILE gameSpace[][10]){
	// *******************************************************			
		int xpos;
		int ypos;
		int length = shipObj->length;
		TILE tileType = shipObj->tileType;
		
		// step through each tile position in the game_space and add the ships TILE
		
		switch(shipObj->orientation){			
			case VERTICAL: //vertical placement anchored at top
				for(int i=0;i<length;i++){
					gameSpace[shipObj->xpos][(shipObj->ypos) + i]=tileType;
				}								
			break;
			case HORIZONTAL: //horizontal placement anchored to the left
				for(int i=0;i<length;i++){
					gameSpace[(shipObj->xpos) + i][shipObj->ypos]=tileType;
				}			
			break;			
		}
	}

	
	// *******************************************************
	// Read Keyboard input without stopping using old C conio.h function
	//REF: thanks to ... http://www.cplusplus.com/forum/general/16335/
	
	//  (not sure how compatible conio.h is) 
	//  should probably use curses to do this instead of kbhit(). (http://www.mkssoftware.com/docs/man3/curs_getch.3.asp)
	bool getInput(char *c){
	// *******************************************************
		if(kbhit()){
			*c = getch();
			return true;		
		}else{
			return false;
		}
	}

	// *******************************************************	
	//REF: thanks to ... http://invisible-island.net/ncurses/ncurses-intro.html#using
	//     Gives us GUI  like display capability on a terminal / console
	void cursesHandler(COMMAND cmd){
	// *******************************************************	
		switch(cmd){
			case INIT:
				//(void) signal(SIGNINT, finish);  	/* arrange interupts to terminate */
				(void) initscr();					/* init the library */
				keypad(stdscr, TRUE);				/* enable keyboard mapping */
				(void) nonl();                 		/* tell curses not to do NL-CR/NL on output */				
				(void) cbreak();               		/* take input chars one at a time, no wait for \n */
				//(void) echo();						/* echo the input in color */
				if(has_colors()){
					start_color();
					
					
					init_pair(1, COLOR_RED,     COLOR_BLACK);
					init_pair(2, COLOR_GREEN,   COLOR_BLACK);
					init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
					init_pair(4, COLOR_BLUE,    COLOR_BLACK);
					init_pair(5, COLOR_CYAN,    COLOR_BLACK);
					init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
					init_pair(7, COLOR_WHITE,   COLOR_BLACK);
										
				}				
			break;
			case TERMINATE:
				(void) endwin();
			break;
		}
	
	}
	
	// *******************************************************	
	int messageHandler(COMMAND cmd, string message=""){
	// *******************************************************	
		static vector<string> messageHistory;   	
		
		static int totalMessages;
		char buffer[10];
		int row;
		
		switch(cmd){
			case INIT:				
				for(row=0;row<15;row++){ //flush the messages for a new game
					messageHandler(ADD," ");
				}
				
				//                  [------------ MESSAGES ------------] 
				messageHandler(ADD,"             WELCOME TO ");
				messageHandler(ADD,"            BATTLE SHIPS ");
				messageHandler(ADD,"     ");
				messageHandler(ADD,"     ");
				messageHandler(ADD,"        Hit [Enter] to Attack ");
				messageHandler(ADD,"          Cursor to Target");
				messageHandler(ADD,"        [TAB] to change ships ");
				messageHandler(ADD,"     ");
				messageHandler(ADD,"        Good Luck - Have Fun!");
				messageHandler(ADD,"     ");
				messageHandler(ADD,"     ");
				messageHandler(ADD,"     ");
				
				
				
			//break;
			case ADD:
				if(messageHistory.size()>15){
					for(row=0;row<messageHistory.size()-1;row++){
						messageHistory[row]=messageHistory[row+1];
					}
					messageHistory.pop_back();
				}
				messageHistory.push_back(message);				
				
			break;
			case SHOW:
				for(row=0;row<messageHistory.size();row++){
					//"0123456789012345678901234567890
					//"J j0j1j2j3j4j5j6j7j8j9 [{row 1}]\n"
					sprintf(buffer,"{row %d}",row);
					
					while(messageHistory[row].length()<34){	//fill in white space
						messageHistory[row]+=' ';
					}
					viewBuffer = str_replace(buffer, messageHistory[row], viewBuffer);					
				}
				for(row=row;row<16;row++){
					sprintf(buffer,"{row %d}",row);
					viewBuffer = str_replace(buffer, "                                  ", viewBuffer);					
				}
			break;
		}
		
	
	}


	// *******************************************************	
	//REF: thanks to ... http://stackoverflow.com/questions/236129/split-a-string-in-c
	vector<string> &split(const string &s, char delim, vector<string> &elems) {
	// *******************************************************	
    stringstream ss(s);
    string item;
		while (getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}
	// *******************************************************	
	//REF: thanks to ... http://stackoverflow.com/questions/236129/split-a-string-in-c
	vector<string> split(const string &s, char delim) {
	// *******************************************************	
		vector<string> elems;
		split(s, delim, elems);
		return elems;
	}
	
	// *******************************************************
	//REF: thanks to ... http://www.zedwood.com/article/cpp-str_replace-function
 	string& str_replace(const string &search, const string &replace, string &subject){
	// *******************************************************
		string buffer;
 
		int sealeng = search.length();
		int strleng = subject.length();
 
		if (sealeng==0){
			return subject;//no change
		}
		for(int i=0, j=0; i<strleng; j=0 ) {
			while (i+j<strleng && j<sealeng && subject[i+j]==search[j]){
				j++;
			}
			if (j==sealeng) { //found 'search'
				buffer.append(replace);
				i+=sealeng;
			} else {
				buffer.append( &subject[i++], 1);
			}
		}
		subject = buffer;
		return subject;
	}	

	