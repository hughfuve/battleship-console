battleship-console 
==================

Example console battleship game project to experiment with STRUCT and typedef in C.
was deliberately trying to avoid the use of classes and CPP but seems I couldnt help myself
when it came to vectors and strings. 

runs under windows using pdcurses ... 
tested with gcc 4.8.1
Tried to make it so it could compile under anything with minimal tweaks.


So here's a kind of psuedo code description of the application... 

It didnt quite end up like this.. but this shows how the process of making it started and unfolded.

The app development starts with a text description, where we try to identify the nouns and verbs and adjectives for
properties, methods and objects. Indents are used677 to try and show hierarchy and dependency.
};	  
APPLICATION DESCRIPTION
(why the hell does it wrap this block of text and not the other parts of the doc???? stupid friggn thing)
The GAMEWORLD is {composed} of 
	PLAYERS who can be a 
		COMPUTER_PLAYER or a 
  		HUMAN_PLAYER
			The HUMAN_PLAYER {views} a PLAYER_VIEW that {shows}  
				a 10x10ENEMY_GAMESPACE 
				the ENEMY_GAMESPACE {shows} 
					the current HISTORY_OF_ATTACKS by the PLAYER into ENEMY_GAMESPACE	
					History is represented as a variable number of HITS and MISSES to VECTORS within the game_space.
					History is probably best recorded as a 10x10 array of vectors, if we wanted a chronological recording of the attacks, but we probably dont need that here, so we can just record status within a 10x10 array.	a 10x10PLAYER_GAMESPACE
				the PLAYER_GAMESPACE {shows} 
					the current HISTORY_OF_ATTACKS by the COMPUTER into PLAYER_GAMESPACE		
				these GAMESPACES are made of 
					TILETYPES which are of types
						WATER
						SHIP_PIECE			(part of a ship)
						Front Piece ?
						Mid Piece ?
						Rear Piece ?
					EXPLOSION_ONSHIP
					EXPLOSION_ONWATER
					enum TILE  { NOTHING=0,CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER, WATER,
								CARRIER_HIT, BATTLESHIP_HIT, CRUISER_HIT, SUBMARINE_HIT, DESTROYER_HIT, WATER_HIT,
								WATER_MISS
							};		

				These TILES are used to show the HISTORY OF ATTACKS
					COORDINATES are represented as [x_row, y_column] or [x,y]
					Where 
						[0,0] = top left corner.
						[9,0] = top right corner.
						[1,0] = 1 square to the right of top left corner.
						[0,1] = 1 square below top left corner.
						etc
				The VIEW			
		The most consistent ASCII codes to represent ships (across different terminals)
	are codes in range #000-127. So by using a letter code for the ships CKSBD and X's to 
	represent the hits and O's for misses, we will render on any machine. 
	If we offset hits 1 square to the right and use 2 chars for each vector, we can 
	get around the scrunched up aspect ratio of most text based systems, and still
	make out hits and misses on the craft. 
	
	
		  0 1 2 3 4 5 6 7 8 9 
		A . .O. . . . . . . .  [----------- TARGET ---------------]
		B . . C C C C C . . O   X = {0-9} : 0   DIFFICULTY: MEDIUM/HARD/EASY/CHEATS
		C . . . . . . . . . .	Y = {A-J} : X   
		D . .O. . . . B . . .   ROUND     : 9
		E . . . . . D B . . .  [----------- STATUS ---------------]
		F . . KX. . D B . . .  [                                  ]
		G . . KX. . . B S SXS  [                                  ]
		H . . KX. . O . . . .  [                                  ]
		I . O K . . . . . . .  [                                  ]
		J . . . . . . . . . .  [                                  ]		
		                       [                                  ]									   
		  0 1 2 3 4 5 6 7 8 9  [                                  ]									   
		A . . . . . . . . . .  [                                  ]									   
		B . O . . . . . . . .  [                                  ]									   
		C . . O . . .DX . . .  [                                  ]									   
		D . .CXCX O .DX O . .  [                                  ]
		E . . . . . . . . . .  [                                  ]		
		F . . . . . O . . . .  [                                  ]		
		G . . . . . . . . . .  [ Computer Fires                   ]
		H . . . . . . . . . .  [ Computer Misses at A,1           ]
		I . . . . . . . . . .  [ Player Aims at G,2               ]		
		J . . . . . . . . . .  [ Player Fires and Hits DESTROYER  ]		
                               [----------------------------------]		
		[Select Target and press enter to fire at enemy           ]

		I did originally consider a # or a universal code for a ship piece, but then there
		is no way to tell where one type of ship starts and another ends, so you have to go
		for unique codes or colors for each ship type.
		
		The easiest and most flexible way to manipulate your view area is to create a string 
		template, and use {codes} to define where you will place your data, and you just use
		str replace calls to insert your data into the template then you can use sprintf() calls
		to display your game area.
		
		Standard terminal size is 80 columns x 24 rows.. so we should work in that limitation
		
const string viewTemplate = 
		"  0 1 2 3 4 5 6 7 8 9                                      \n"
		"A A0A1A2A3A4A5A6A7A8A9 [----------- TARGET ---------------]\n"
		"B B0B1B2B3B4B5B6B7B8B9  X = {0-9} : x   DIFFICULTY: {diff} \n"
		"C C0C1C2C3C4C5C6C7C8C9	 Y = {A-J} : y                      \n"
		"D D0D1D2D3D4D5D6D7D8D9  ROUND     : rrr                    \n"
		"E E0E1E2E3E4E5E6E7E8E9 [----------- STATUS ---------------]\n"
		"F F0F1F2F3F4F5F6F7F8F9 [{row 17}]\n"
		"G G0G1G2G3G4G5G6G7G8G9 [{row 16}]\n"
		"H H0H1H2H3H4H5H6H7H8H9 [{row 15}]\n"
		"I I0I1I2I3I4I5I6I7I8I9 [{row 14}]\n"
		"J J0J1J2J3J4J5J6J7J8J9 [{row 13}]\n"
		"                       [{row 12}]\n"
		"  0 1 2 3 4 5 6 7 8 9  [{row 11}]\n"
		"A a0a1a2a3a4a5a6a7a8a9 [{row 10}]\n"
		"B b0b1b2b3b4b5b6b7b8b9 [{row 9}]\n"
		"C c0c1c2c3c4c5c6c7c8c9 [{row 8}]\n"
		"D d0d1d2d3d4d5d6d7d8d9 [{row 7}]\n"
		"E e0e1e2e3e4e5e6e7e8e9 [{row 6}]\n"
		"F f0f1f2f3f4f5f6f7f8f9 [{row 5}]\n"
		"G g0g1g2g3g4g5g6g7g8g9 [{row 4}]\n"
		"H h0h1h2h3h4h5h6h7h8h9 [{row 3}]\n"
		"I i0i1i2i3i4i5i6i7i8i9 [{row 2}]\n"
		"J j0j1j2j3j4j5j6j7j8j9 [{row 1}]\n"        
		"[{player message}]\n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n"
		" padding                                                   \n";
		** with enough optional padding for a double buffer, or a little overflow protection.
		
					
		Then you just use a str_replace function to inject strings and build the display.
		
		something like ...
		
		viewBuffer = str_replace("{diff}",difficulties[difficulty], viewBuffer);
		viewBuffer = str_replace("x", attackVectorX,viewBuffer);
		viewBuffer = str_replace("y", attackVectorY,viewBuffer);
		viewBuffer = str_replace("rrr",sprintf("%03d",round),viewBuffer);

		printf("%s",viewBuffer);
		
		
	the PLAYER_VIEW also {shows} a STATUS_REPORT for the GAMEWORLD that shows
    a vertical scrolling history.	
		
		CURRENT_GAME_STATUS which is represented as a text message of one of...
			"It is PLAYER {TURN} to {AIM} and {FIRE}"
			"It is COMPUTER {TURN} to {AIM} and {FIRE}"
			"GAME IS READY TO START"
			"THE GAME IS OVER"
			"LETS CELEBRATE THE WINNER"
			"CONFIGURATION MODE"

			"COMPUTER FIRES AND HITS [SHIPTYPE] @B1!"
			"COMPUTER FIRES AND MISSES @ A1"
			"COMPUTER SINKS [SHIPTYPE]", shipType
		
			"PLAYER HITS %s!" , ShipType (cruiser, battleship, destroyer,sub,carrier)
			"PLAYER SINKS %s!" , ShipType (cruiser, battleship, destroyer,sub,carrier)

			
		PLAYER_OPTIONS:
			"Ready for coordinates"
			"Choose new Coordinate or [return] to Fire"
			"Missed"			
			"Computer Fires ..."
			
			
		CURRENT COORDINATE:  A-J 0-9	
		TURN_NUMBER	
		DIFFICULTY 
	each player receives 5 SHIPS of varying TILE_COUNTs
	   Type 1 = 5 TILE CARRIER
	   Type 2 = 4 TILE BATTLESHIP
	   Type 3 = 3 TILE CRUISER
	   Type 4 = 3 TILE SUBMARINE
	   Type 5 = 2 TILE DESTROYER
	   
			Each TILE of each SHIP can be in one of several 
				STATES
					DAMAGED
					UNDAMAGED
			Each SHIP is ANCHORed from the frontmost tile of the boat
			The [[anchoring]] places each SHIP into one of 4 ORIENTATIONs
				HORIZONTAL_ORIENTATIONS
					RIGHT    anchor is positioned to the east 
					LEFT     anchor is positioned to the west 
				VERTICAL ORIENTATIONS
					TOP      anchor is positioned to the north 
					BOTTOM   anchor is positioned to the south
					  
	
GAMEPLAY PROCEEDS AS FOLLOWS
 The objects of GAMEWORLD are {constructed} and {intialized}

 INITIALIZE_NEW_GAME
	Send the *INIT* COMMAND to all objects for a new game.

 The GAMEWORLD LOGIC is placed into a service loop.

 Within the SERVICE LOOP 
	
	GAME_WORLD Mode is serviced
		On GAME_START:  (player hits a game start key)
			INIT_NEW_GAME (clear ships and game space)
			SET MODE=PLACE_SHIPS
			
		On WAITING_TO_START
			... do something wait for start key...
			... show status.
		On ARE_YOU_SURE? You want to quit?
			if [Y] exit();
			
		ON_PLACE_SHIPS	
			Just place them randomly.. 
				and let the player choose if they want to start with this set.
			check when placing a ship that there is no collision and we are in bounds.
				choose a random orientation (N,S,E,W)
				for carrier choose 2 numbers 
					v1= rand between 0 and {rand() % 6 (totalSquares - (shipLength-1))} or (10-4) ;
					v2= rand between 0 and 9
						switch (orientation)
							case N then vertical placement upwards
								x=v2
								y=v1+(shipLength-1)
							case S then vertical placement downwards
								x=v2
								y=v1
							case E then horizontal placement to right
								x=v1+(shipLength-1)
								y=v2							
							case E then horizontal placement to left
								x=v1
								y=v2
					
				for cruiser choose 	2 numbers
					one between 0 and (10-(shipLength-1))
					one between 0 and 9
			
		On IN_PLAY ...
			
	PLAYER_ACTIONS are serviced
		On Player Chooses an ACTION 
			[Set Coordinate 		 ({A-J or 0-9})
			[FIRE!]				     ({spacebar})
				If hit, 
					report shiptype
				else
					report history of miss
			[EXIT]                   ({esc},{q})
				[Y] yes I want to quit.
			[RESTART]                ({enter})
			[SET DIFFICULTY]         ({+} / {-})
			[Automatically SEEK a random target] ({?})
			[Automatically HUNT a random target] ({ctrl-?)
			
	SERVICE all object logic 
					
	{show} PLAYER_VIEW 
		{show} GAME_SPACES
			{show} ENEMY_GAMESPACE
			{show} PLAYER_GAMESPACE
		{show} GAME_STATUS	(use multiple lines to show 5+ message event history)
		{show} PLAYER_OPTIONS
		{show} CURRENT_PLAYER_AIM_COORDINATES
		{show} DIFFICULTY SETTING (debug mode)
		{show} Turn Number
   
   SERVICE_COMPUTER_AI
     
   
COMPUTER AI.		
	The computer AI operates in one of several modes.
		SEEK MODE
			When in SEEK MODE the computer has no likely prospects, and shoots randomly or 
			in a controlled sequence over the map until it makes a hit.
			
			The hit or miss as an update to PLAYER GAMESPACE.
			
			The	hit is reported to the GAME STATUS and the shiptype is reported.
			
			The status of the ship is updated to indicated the square that is hit.
			
			If a ship is sunk, then a report to game status.
			
		DESTROY MODE
			If we have registered any hits of unsunk ships then we enter DESTROY MODE instead of SEEK MODE
			
			
			At first the computer tries to work out if the ship is oriented horizontally or vertically.
			once the computer finds the end of the ship, it hunts in the other direction until the ship
			is sunk. It does this by identifying if more than 1 hit has been made and then records that the
			orientation must be relative to the direction it is searching in. horizontal or vertical.
			
			just focusing around the target area is probably not adequate...
				we have to consider what happens when you are targetting a cruiser say, and you hit a sub, now you have
			2 targets, and possibly more coming up.
				the AI will need to have 5 targets in it's sites at any one time.
				we will likely declare the target within the player object.
				 TARGET targets[6];
				targets will likely need at least an initial vector
				we could just fire in the closest proximity to the vector traversing the horizontal and verticals but we will need quite the data structure for this search.
				
				 struct TARGET {
				 targetVectorX
				 targetVectorY
				 currentAxisDirection {n,s,w,e}   ... choose the first direction at random
				 north[5]
				 south[5]
				 east[5]     ** turns out we dont need to keep a history, so these arrays are un-needed.
				 west[5]
				 directionKnown ? flag.
				 hits
				 }
				 
				 on each turn
					check status of each target. Player owns the target data
						if there are any found targets
							go into destroy mode 
						else
							go into search mode
					if destroy mode.		
						walk along currentAxisDirection checking each space and filling in the axis maps, until miss
						if already searched there, then record the status and look in the next spot
						if a miss then next turn
							if directionKnown
								invert direction n .. s .. n, e .. w .. e , etc
							else
								next axis .. go n .. e .. s .. w .. n etc just go clockwise.
						if hit then record the hit and continue along axis to next turn.
							if more than 1 hit on any axis then flag that direction is known, future searches will maintain axis.
							if destroyed, (hits > #Tiles for ShipType) mark as solved.
					
			
		COMEBACK SEEK MODE ** not yet implemented.. just an idea..
			This is an alternative SEEK MODE with the goal of 
			artificially providing a more exciting experience to the player.
			
			We want to create a situation where the player feels like they were able to come back 
			from the dead.
			
			The algorithm first selects a percentage chance for the computer to succeed or fail, 
			from a table determined by the number of hits that the player has made.

			If no hits, then the player and computer have an even and random chance.
			
			If the player makes more than one hit, then the percentage chance of the computers SEEK success
			is taken from a table of size (total Number of hits by player)
			
			For a short while the computer is progressively given the advantage. 
			
			But as the game progresses to the last ships the computers chances are diminished to near zero. 
			
			Thus enabling the player to catch up and make stunning come backs.
			
			If determined that the hit will be successful, the computer cheats and looks in the tables
			for a hit. The computer then switches to a standard hunt mode.
			
		SALVO MODE
			You get to shoot as many shots as you currently have ships, so if you have 4 ships you
			shoot 4 shots at a time.
			
			After your shots, you then get to see what the damage is.
			
			
	

